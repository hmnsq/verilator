// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: OrderMTask graph construction
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3OrderMTaskGraph.h"

#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3InstrCount.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_nextId = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp)
    : V3GraphVertex{&graph} {
    UASSERT(s_nextId < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    যদি (!mVtxp) প্রত্যাগম;
    m_mVertices.linkBack(mVtxp);
    যদি (ধ্রুব OrderLogicVertex* ধ্রুব olvp = mVtxp->logicp()) {
        m_cost += V3_Instr_গুনতি::গুনতি(olvp->nodep(), সত্য);
    }
}

//######################################################################
// OrderMTaskGraph

OrderMTaskGraph::OrderMTaskGraph(OrderMoveGraph& moveGraph)
    : m_moveGraph{moveGraph}
    , m_entryp{নূতন LogicMTask{*এতত্, nullptr}}
    , m_exitp{নূতন LogicMTask{*এতত্, nullptr}}
    , m_slowAsserts{v3Global.opt.debugPartition()} {}

তার্কিক OrderMTaskGraph::pathExistsImpl(LogicMTask* fromp, LogicMTask* শীর্ষ,
                                     ধ্রুব MTaskEdge* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    যদি (fromp == শীর্ষ) প্রত্যাগম সত্য;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    যদি (fromp->cpExclusive<GraphWay::REVERSE>() < শীর্ষ->cpInclusive<GraphWay::REVERSE>()) {
        প্রত্যাগম অসত্য;
    }
    যদি (fromp->cpInclusive<GraphWay::FORWARD>() > শীর্ষ->cpExclusive<GraphWay::FORWARD>()) {
        প্রত্যাগম অসত্য;
    }

    // Recursively look for a path
    প্রতি (ধ্রুব V3GraphEdge& follow : fromp->outEdges()) {
        যদি (&follow == excludedEdgep) অনুবর্ত;
        LogicMTask* ধ্রুব nextp = static_cast<LogicMTask*>(follow.শীর্ষ());
        // Don't visit the same MTask twice in the same search.
        যদি (nextp->m_generation == m_currentGeneration) অনুবর্ত;
        যদি (pathExistsImpl(nextp, শীর্ষ, nullptr)) প্রত্যাগম সত্য;
    }
    প্রত্যাগম অসত্য;
}

template <GraphWay::en N_Way>
শূন্য OrderMTaskGraph::propagate_ঠেলো(LogicMTask* mtaskp) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};
    ধ্রুব uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    প্রতি (V3GraphEdge& graphEdge : mtaskp->edges<way>()) {
        MTaskEdge& edge = static_cast<MTaskEdge&>(graphEdge);

        LogicMTask* ধ্রুব relativep = edge.furtherMTaskp<N_Way>();
        EdgeHeap::গ্রন্থি_2& edge_Heap_গ্রন্থি = edge.m_edge_Heap_গ্রন্থি[inv];
        যদি (inclusiveCp > edge_Heap_গ্রন্থি.চাবি().m_cp) {
            relativep->m_edgeHeap[inv].increase_চাবি(&edge_Heap_গ্রন্থি, inclusiveCp);
        }

        ধ্রুব uint64_t relativeCp = relativep->cpExclusive<way>();

        যদি (relativeCp >= inclusiveCp) অনুবর্ত;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ধ্রুব uint64_t increment = inclusiveCp - relativeCp;

        PropagatePendingHeap::গ্রন্থি_2*& pendingNodepRef = relativep->m_propagateHeapNodep;
        যদি (PropagatePendingHeap::গ্রন্থি_2* ধ্রুব nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            যদি (increment > nodep->চাবি().m_increment) {
                m_pendingHeap.increase_চাবি(nodep, increment);
            }
            অনুবর্ত;
        }

        // Add to heap
        PropagatePendingHeap::গ্রন্থি_2* ধ্রুব nodep = m_pending_গ্রন্থি_Pool.alloc();
        pendingNodepRef = nodep;
        m_pendingHeap.ঢোকাও(nodep, {increment, relativep->id(), relativep});
    }
}

template <GraphWay::en N_Way>
শূন্য OrderMTaskGraph::propagateResolve() {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    যাবত্ (!m_pendingHeap.empty()) {
        // Pop max element from heap
        PropagatePendingHeap::গ্রন্থি_2* ধ্রুব maxp = m_pendingHeap.max();
        m_pendingHeap.remove(maxp);
        // Pick up values
        LogicMTask* ধ্রুব mtaskp = maxp->চাবি().m_mtaskp;
        ধ্রুব uint64_t cpGrowBy = maxp->চাবি().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_গ্রন্থি_Pool.মুক্ত(maxp);
        mtaskp->m_propagateHeapNodep = nullptr;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ধ্রুব uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        যদি (VL_UNLIKELY(m_slowAsserts)) {
            ধ্রুব uint64_t edgeCp = mtaskp->m_edgeHeap[inv].max()->চাবি().m_cp;
            UASSERT_OBJ(edgeCp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ঠেলো<N_Way>(mtaskp);
    }
}

uint64_t OrderMTaskGraph::totalCost() ধ্রুব {
    uint64_t cost = 0;
    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) cost += static_cast<ধ্রুব LogicMTask&>(vtx).cost();
    প্রত্যাগম cost;
}

শূন্য OrderMTaskGraph::addEdge(LogicMTask* fromp, LogicMTask* শীর্ষ) {
    UASSERT_OBJ(fromp != শীর্ষ, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->hasEdgeTo(শীর্ষ), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    নূতন MTaskEdge{এতত্, fromp, শীর্ষ};

    // The path through the new edge might be longer than the current critical path of its
    // endpoints, in which case the critical paths need updating. Note each endpoint is the seed of
    // one propagation, and is updated by the other: the inclusive critical paths of the endpoints
    // themselves did not change (a new out-edge cannot lengthen a path into 'fromp', nor a new
    // in-edge a path out of 'top'), so it is the new relative of each seed whose critical path
    // might need to grow. That is, 'top' is updated wayward of 'fromp' below, and vice versa,
    // together with the relatives of each, transitively.
    //
    // The guards below are an asymptotic optimization. The graph is consistent apart from the new
    // edge, so the new relative is the only relative of either seed that can have a stale critical
    // path, and if it does not need updating the propagation does nothing. It would however still
    // walk all edges of the seed to discover that, which is expensive for a high degree seed.
    যদি (fromp->cpInclusive<GraphWay::FORWARD>() > শীর্ষ->cpExclusive<GraphWay::FORWARD>()) {
        propagate<GraphWay::FORWARD>(fromp);
    }
    যদি (শীর্ষ->cpInclusive<GraphWay::REVERSE>() > fromp->cpExclusive<GraphWay::REVERSE>()) {
        propagate<GraphWay::REVERSE>(শীর্ষ);
    }
}

শূন্য OrderMTaskGraph::mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    যাবত্ (MTaskEdge* ধ্রুব edgep = static_cast<MTaskEdge*>(donorp->outEdges().frontp())) {
        LogicMTask* ধ্রুব relativep = edgep->toMTaskp();

        relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);

        যদি (relativep == recipientp || recipientp->hasEdgeTo(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } অন্যথা {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->addDependent(relativep);
            recipientp->stealRelativeEdge<GraphWay::FORWARD>(edgep);
            relativep->addRelativeEdge<GraphWay::REVERSE>(edgep);
        }
    }

    // Process incoming edges of donor
    যাবত্ (MTaskEdge* ধ্রুব edgep = static_cast<MTaskEdge*>(donorp->inEdges().frontp())) {
        LogicMTask* ধ্রুব relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);

        যদি (relativep == recipientp || relativep->hasEdgeTo(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } অন্যথা {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_শীর্ষ(recipientp);
            relativep->addDependent(recipientp);
            relativep->addRelativeEdge<GraphWay::FORWARD>(edgep);
            recipientp->stealRelativeEdge<GraphWay::REVERSE>(edgep);
        }
    }

    // Move the contents of the donor into the recipient, update its cost
    recipientp->m_mVertices.splice(recipientp->m_mVertices.end(), donorp->m_mVertices);
    recipientp->m_cost += donorp->m_cost;

    // The recipient now holds all edges of the merged MTask, and the critical paths of all its
    // relatives are still up to date, so the critical paths implied by its edges are the critical
    // paths of the merged MTask.
    ধ্রুব uint64_t newCpFwd = recipientp->cpExclusiveFromEdges<GraphWay::FORWARD>();
    ধ্রুব uint64_t newCpRev = recipientp->cpExclusiveFromEdges<GraphWay::REVERSE>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<GraphWay::FORWARD>(newCpFwd);
    propagate<GraphWay::FORWARD>(recipientp);
    recipientp->cpExclusive<GraphWay::REVERSE>(newCpRev);
    propagate<GraphWay::REVERSE>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(এতত্), donorp);
}

শূন্য OrderMTaskGraph::removeTransitiveEdges() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    প্রতি (V3GraphVertex& vtx : vertices()) {
        প্রতি (V3GraphEdge* ধ্রুব graphEdgep : vtx.outEdges().unlinkable()) {
            MTaskEdge* ধ্রুব edgep = static_cast<MTaskEdge*>(graphEdgep);
            LogicMTask* ধ্রুব fromp = edgep->fromMTaskp();
            LogicMTask* ধ্রুব শীর্ষ = edgep->toMTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            যদি (!pathExists(fromp, শীর্ষ, edgep)) অনুবর্ত;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(শীর্ষ);
            fromp->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            শীর্ষ->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

শূন্য OrderMTaskGraph::removeEmptyMTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    প্রতি (V3GraphVertex* ধ্রুব vtxp : vertices().unlinkable()) {
        LogicMTask* ধ্রুব mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        যদি (mtaskp == m_entryp || mtaskp == m_exitp) অনুবর্ত;

        // Keep any MTask that holds logic
        তার্কিক empty = সত্য;
        প্রতি (ধ্রুব OrderMoveVertex& mVtx : mtaskp->vertexList()) {
            যদি (mVtx.logicp()) {
                empty = অসত্য;
                ভঙ্গ;
            }
        }
        যদি (!empty) অনুবর্ত;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->cost(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        প্রতি (V3GraphEdge& inEdge : mtaskp->inEdges()) {
            LogicMTask* ধ্রুব fromp = static_cast<MTaskEdge&>(inEdge).fromMTaskp();
            প্রতি (V3GraphEdge& outEdge : mtaskp->outEdges()) {
                LogicMTask* ধ্রুব শীর্ষ = static_cast<MTaskEdge&>(outEdge).toMTaskp();
                যদি (!fromp->hasEdgeTo(শীর্ষ)) addEdge(fromp, শীর্ষ);
            }
        }

        // Remove incoming edges of 'mtaskp'
        যাবত্ (MTaskEdge* ধ্রুব edgep = static_cast<MTaskEdge*>(mtaskp->inEdges().frontp())) {
            LogicMTask* ধ্রুব relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        যাবত্ (MTaskEdge* ধ্রুব edgep = static_cast<MTaskEdge*>(mtaskp->outEdges().frontp())) {
            LogicMTask* ধ্রুব relativep = edgep->toMTaskp();
            relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(এতত্), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
template <GraphWay::en N_Way>
শূন্য OrderMTaskGraph::validateWay() ধ্রুব {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv = way.invert();
    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) {
        ধ্রুব LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cpCost = 0;
        std::unordered_set<ধ্রুব V3GraphVertex*> relatives;
        প্রতি (ধ্রুব V3GraphEdge& graphEdge : mtask.edges<inv>()) {
            ধ্রুব MTaskEdge& edge = *graphEdge.as<MTaskEdge>();
            ধ্রুব LogicMTask& relative = *(edge.furtherp<inv>()->template as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(edge.weight() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ধ্রুব তার্কিক first = relatives.ঢোকাও(&relative).second;
            UASSERT_OBJ(first, &mtask, "Should be no redundant edges in MTask graph");
            ধ্রুব uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(edge.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(edge.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cpCost = std::max(cpCost, inclusiveCp);
        }
        ধ্রুব uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cpCost, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cpExclusiveFromEdges<N_Way>() == cpCost, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

শূন্য OrderMTaskGraph::validate() ধ্রুব {
    যদি (!m_slowAsserts) প্রত্যাগম;

    validateWay<GraphWay::FORWARD>();
    validateWay<GraphWay::REVERSE>();

    // Check the dependents set of each MTask agrees with its out-edges
    প্রতি (ধ্রুব V3GraphVertex& vtx : vertices()) {
        ধ্রুব LogicMTask& mtask = *vtx.as<LogicMTask>();
        size_t nDependents = 0;
        প্রতি (ধ্রুব V3GraphEdge& graphEdge : mtask.outEdges()) {
            LogicMTask* ধ্রুব শীর্ষ = graphEdge.as<MTaskEdge>()->toMTaskp();
            UASSERT_OBJ(mtask.hasEdgeTo(শীর্ষ), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.size() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

বর্গ OrderMTaskGraphBuilder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ধ্রুব VNUser1InUse m_user1InUse;

    // MEMBERS
    OrderMTaskGraph& m_mtaskGraph;  // Output OrderMTaskGraph

    // METHODS

    // Predicate function to determine what OrderMoveVertex to bypass when constructing the MTask
    // graph. The OrderMoveGraph is a bipartite graph of:
    // - 1. OrderMoveVertex instances containing logic via OrderLogicVertex
    //      (OrderMoveVertex::logicp() != nullptr)
    // - 2. OrderMoveVertex instances containing an (OrderVarVertex, domain) pair
    // The goal is to order the logic vertices. The second type of variable/domain vertices only
    // carry dependencies and are eventually discarded. In order to reduce the working set size,
    // we 'bypass' and not create LogicMTask vertices for some variable vertices, and instead add
    // the transitive dependencies directly, but only if adding the transitive edges directly does
    // not require more dependency edges than keeping the intermediate vertex. That is, we bypass a
    // variable vertex if fanIn * fanOut <= fanIn + fanOut. This is true if fanIn or fanOut are 1,
    // or if they are both 2. This can significantly reduce the initial size of OrderMTaskGraph.
    স্থির তার্কিক bypassOk(OrderMoveVertex* mvtxp) {
        // Need to keep all logic vertices
        যদি (mvtxp->logicp()) প্রত্যাগম অসত্য;
        // Count fan-in, up to 3
        unsigned fanIn = 0;
        auto& inEdges = mvtxp->inEdges();
        প্রতি (auto it = inEdges.begin(); it != inEdges.end(); ++it) {
            যদি (++fanIn == 3) ভঙ্গ;
        }
        // If fanIn no more than one, bypass
        যদি (fanIn <= 1) প্রত্যাগম সত্য;
        // Count fan-out, up to 3
        unsigned fanOut = 0;
        auto& outEdges = mvtxp->outEdges();
        প্রতি (auto it = outEdges.begin(); it != outEdges.end(); ++it) {
            যদি (++fanOut == 3) ভঙ্গ;
        }
        // If fan-out no more than one, bypass
        যদি (fanOut <= 1) প্রত্যাগম সত্য;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        প্রত্যাগম fanIn + fanOut == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    শূন্য addEdge(LogicMTask* srcp, LogicMTask* dstp) {
        যদি (srcp->hasEdgeTo(dstp)) প্রত্যাগম;  // Don't create redundant edges.
        m_mtaskGraph.addEdge(srcp, dstp);
    }

    // CONSTRUCTORS
    explicit OrderMTaskGraphBuilder(OrderMTaskGraph& mtaskGraph)
        : m_mtaskGraph{mtaskGraph} {

        // Create the LogicMTasks for each OrderMoveVertex
        প্রতি (V3GraphVertex& vtx : mtaskGraph.moveGraph().vertices()) {
            OrderMoveVertex& mVtx = static_cast<OrderMoveVertex&>(vtx);
            যদি (bypassOk(&mVtx)) {
                mVtx.userp(nullptr);  // Set to nullptr to mark as bypassed
            } অন্যথা {
                mVtx.userp(নূতন LogicMTask{mtaskGraph, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& entry = *mtaskGraph.entryp();
        LogicMTask& exit = *mtaskGraph.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        প্রতি (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            যদি (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) অনুবর্ত;

            OrderMoveVertex::List& vertexList = mtask.vertexList();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(vertexList.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            OrderMoveVertex* ধ্রুব mVtxp = vertexList.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            প্রতি (ধ্রুব V3GraphEdge& dEdge : mVtxp->outEdges()) {
                V3GraphVertex* ধ্রুব শীর্ষ = dEdge.শীর্ষ();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                যদি (LogicMTask* ধ্রুব otherp = static_cast<LogicMTask*>(শীর্ষ->userp())) {
                    addEdge(&mtask, otherp);
                    অনুবর্ত;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                প্রতি (ধ্রুব V3GraphEdge& tEdge : শীর্ষ->outEdges()) {
                    LogicMTask* ধ্রুব transp = static_cast<LogicMTask*>(tEdge.শীর্ষ()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    addEdge(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        প্রতি (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            যদি (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) অনুবর্ত;
            // Add the entry/exit edges if not otherwise connected
            যদি (mtask.inEmpty()) addEdge(&entry, &mtask);
            যদি (mtask.outEmpty()) addEdge(&mtask, &exit);
        }
    }
    ~OrderMTaskGraphBuilder() = স্বভাব;
    VL_UNCOPYABLE(OrderMTaskGraphBuilder);
    VL_UNMOVABLE(OrderMTaskGraphBuilder);

সার্বজনিক:
    স্থির শূন্য apply(OrderMTaskGraph& mtaskGraph) { OrderMTaskGraphBuilder{mtaskGraph}; }
};

std::unique_ptr<OrderMTaskGraph> OrderMTaskGraph::build(OrderMoveGraph& moveGraph) {
    std::unique_ptr<OrderMTaskGraph> resp{নূতন OrderMTaskGraph{moveGraph}};
    OrderMTaskGraphBuilder::apply(*resp);
    resp->validate();
    প্রত্যাগম resp;
}
