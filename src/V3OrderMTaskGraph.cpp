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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3OrderMTaskGraph.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3InstrCount.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_nextId = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp)
    : V3GraphVertex{&graph} {
    UASSERT(s_nextId < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    ಯದಿ (!mVtxp) ಪ್ರತ್ಯಾಗಮ;
    m_mVertices.linkBack(mVtxp);
    ಯದಿ (ಧ್ರುವ OrderLogicVertex* ಧ್ರುವ olvp = mVtxp->logicp()) {
        m_cost += V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ(olvp->nodep(), ಸತ್ಯ);
    }
}

//######################################################################
// OrderMTaskGraph

OrderMTaskGraph::OrderMTaskGraph(OrderMoveGraph& moveGraph)
    : m_moveGraph{moveGraph}
    , m_entryp{ನೂತನ LogicMTask{*ಏತತ್, nullptr}}
    , m_exitp{ನೂತನ LogicMTask{*ಏತತ್, nullptr}}
    , m_slowAsserts{v3Global.opt.debugPartition()} {}

ತಾರ್ಕಿಕ OrderMTaskGraph::pathExistsImpl(LogicMTask* fromp, LogicMTask* ತುದಿ,
                                     ಧ್ರುವ MTaskEdge* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    ಯದಿ (fromp == ತುದಿ) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    ಯದಿ (fromp->cpExclusive<GraphWay::REVERSE>() < ತುದಿ->cpInclusive<GraphWay::REVERSE>()) {
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಯದಿ (fromp->cpInclusive<GraphWay::FORWARD>() > ತುದಿ->cpExclusive<GraphWay::FORWARD>()) {
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Recursively look for a path
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& follow : fromp->outEdges()) {
        ಯದಿ (&follow == excludedEdgep) ಅನುವರ್ತ;
        LogicMTask* ಧ್ರುವ nextp = static_cast<LogicMTask*>(follow.ತುದಿ());
        // Don't visit the same MTask twice in the same search.
        ಯದಿ (nextp->m_generation == m_currentGeneration) ಅನುವರ್ತ;
        ಯದಿ (pathExistsImpl(nextp, ತುದಿ, nullptr)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

template <GraphWay::en N_Way>
ಶೂನ್ಯ OrderMTaskGraph::propagate_ತಳ್ಳು(LogicMTask* mtaskp) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};
    ಧ್ರುವ uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    ಪ್ರತಿ (V3GraphEdge& graphEdge : mtaskp->edges<way>()) {
        MTaskEdge& edge = static_cast<MTaskEdge&>(graphEdge);

        LogicMTask* ಧ್ರುವ relativep = edge.furtherMTaskp<N_Way>();
        EdgeHeap::ಗಂಟು_2& edge_Heap_ಗಂಟು = edge.m_edge_Heap_ಗಂಟು[inv];
        ಯದಿ (inclusiveCp > edge_Heap_ಗಂಟು.ಕೀಲಿ().m_cp) {
            relativep->m_edgeHeap[inv].increase_ಕೀಲಿ(&edge_Heap_ಗಂಟು, inclusiveCp);
        }

        ಧ್ರುವ uint64_t relativeCp = relativep->cpExclusive<way>();

        ಯದಿ (relativeCp >= inclusiveCp) ಅನುವರ್ತ;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ಧ್ರುವ uint64_t increment = inclusiveCp - relativeCp;

        PropagatePendingHeap::ಗಂಟು_2*& pendingNodepRef = relativep->m_propagateHeapNodep;
        ಯದಿ (PropagatePendingHeap::ಗಂಟು_2* ಧ್ರುವ nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            ಯದಿ (increment > nodep->ಕೀಲಿ().m_increment) {
                m_pendingHeap.increase_ಕೀಲಿ(nodep, increment);
            }
            ಅನುವರ್ತ;
        }

        // Add to heap
        PropagatePendingHeap::ಗಂಟು_2* ಧ್ರುವ nodep = m_pending_ಗಂಟು_Pool.alloc();
        pendingNodepRef = nodep;
        m_pendingHeap.ಸೇರಿಸು(nodep, {increment, relativep->id(), relativep});
    }
}

template <GraphWay::en N_Way>
ಶೂನ್ಯ OrderMTaskGraph::propagateResolve() {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    ಯಾವತ್ (!m_pendingHeap.empty()) {
        // Pop max element from heap
        PropagatePendingHeap::ಗಂಟು_2* ಧ್ರುವ maxp = m_pendingHeap.max();
        m_pendingHeap.remove(maxp);
        // Pick up values
        LogicMTask* ಧ್ರುವ mtaskp = maxp->ಕೀಲಿ().m_mtaskp;
        ಧ್ರುವ uint64_t cpGrowBy = maxp->ಕೀಲಿ().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_ಗಂಟು_Pool.ಮುಕ್ತ(maxp);
        mtaskp->m_propagateHeapNodep = nullptr;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ಧ್ರುವ uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        ಯದಿ (VL_UNLIKELY(m_slowAsserts)) {
            ಧ್ರುವ uint64_t edgeCp = mtaskp->m_edgeHeap[inv].max()->ಕೀಲಿ().m_cp;
            UASSERT_OBJ(edgeCp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ತಳ್ಳು<N_Way>(mtaskp);
    }
}

uint64_t OrderMTaskGraph::totalCost() ಧ್ರುವ {
    uint64_t cost = 0;
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) cost += static_cast<ಧ್ರುವ LogicMTask&>(vtx).cost();
    ಪ್ರತ್ಯಾಗಮ cost;
}

ಶೂನ್ಯ OrderMTaskGraph::addEdge(LogicMTask* fromp, LogicMTask* ತುದಿ) {
    UASSERT_OBJ(fromp != ತುದಿ, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->hasEdgeTo(ತುದಿ), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    ನೂತನ MTaskEdge{ಏತತ್, fromp, ತುದಿ};

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
    ಯದಿ (fromp->cpInclusive<GraphWay::FORWARD>() > ತುದಿ->cpExclusive<GraphWay::FORWARD>()) {
        propagate<GraphWay::FORWARD>(fromp);
    }
    ಯದಿ (ತುದಿ->cpInclusive<GraphWay::REVERSE>() > fromp->cpExclusive<GraphWay::REVERSE>()) {
        propagate<GraphWay::REVERSE>(ತುದಿ);
    }
}

ಶೂನ್ಯ OrderMTaskGraph::mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    ಯಾವತ್ (MTaskEdge* ಧ್ರುವ edgep = static_cast<MTaskEdge*>(donorp->outEdges().frontp())) {
        LogicMTask* ಧ್ರುವ relativep = edgep->toMTaskp();

        relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);

        ಯದಿ (relativep == recipientp || recipientp->hasEdgeTo(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ಅನ್ಯಥಾ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->addDependent(relativep);
            recipientp->stealRelativeEdge<GraphWay::FORWARD>(edgep);
            relativep->addRelativeEdge<GraphWay::REVERSE>(edgep);
        }
    }

    // Process incoming edges of donor
    ಯಾವತ್ (MTaskEdge* ಧ್ರುವ edgep = static_cast<MTaskEdge*>(donorp->inEdges().frontp())) {
        LogicMTask* ಧ್ರುವ relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);

        ಯದಿ (relativep == recipientp || relativep->hasEdgeTo(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ಅನ್ಯಥಾ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_ತುದಿ(recipientp);
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
    ಧ್ರುವ uint64_t newCpFwd = recipientp->cpExclusiveFromEdges<GraphWay::FORWARD>();
    ಧ್ರುವ uint64_t newCpRev = recipientp->cpExclusiveFromEdges<GraphWay::REVERSE>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<GraphWay::FORWARD>(newCpFwd);
    propagate<GraphWay::FORWARD>(recipientp);
    recipientp->cpExclusive<GraphWay::REVERSE>(newCpRev);
    propagate<GraphWay::REVERSE>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(ಏತತ್), donorp);
}

ಶೂನ್ಯ OrderMTaskGraph::removeTransitiveEdges() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    ಪ್ರತಿ (V3GraphVertex& vtx : vertices()) {
        ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ graphEdgep : vtx.outEdges().unlinkable()) {
            MTaskEdge* ಧ್ರುವ edgep = static_cast<MTaskEdge*>(graphEdgep);
            LogicMTask* ಧ್ರುವ fromp = edgep->fromMTaskp();
            LogicMTask* ಧ್ರುವ ತುದಿ = edgep->toMTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            ಯದಿ (!pathExists(fromp, ತುದಿ, edgep)) ಅನುವರ್ತ;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(ತುದಿ);
            fromp->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            ತುದಿ->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

ಶೂನ್ಯ OrderMTaskGraph::removeEmptyMTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : vertices().unlinkable()) {
        LogicMTask* ಧ್ರುವ mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        ಯದಿ (mtaskp == m_entryp || mtaskp == m_exitp) ಅನುವರ್ತ;

        // Keep any MTask that holds logic
        ತಾರ್ಕಿಕ empty = ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ OrderMoveVertex& mVtx : mtaskp->vertexList()) {
            ಯದಿ (mVtx.logicp()) {
                empty = ಅಸತ್ಯ;
                ಭಙ್ಗ;
            }
        }
        ಯದಿ (!empty) ಅನುವರ್ತ;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->cost(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        ಪ್ರತಿ (V3GraphEdge& inEdge : mtaskp->inEdges()) {
            LogicMTask* ಧ್ರುವ fromp = static_cast<MTaskEdge&>(inEdge).fromMTaskp();
            ಪ್ರತಿ (V3GraphEdge& outEdge : mtaskp->outEdges()) {
                LogicMTask* ಧ್ರುವ ತುದಿ = static_cast<MTaskEdge&>(outEdge).toMTaskp();
                ಯದಿ (!fromp->hasEdgeTo(ತುದಿ)) addEdge(fromp, ತುದಿ);
            }
        }

        // Remove incoming edges of 'mtaskp'
        ಯಾವತ್ (MTaskEdge* ಧ್ರುವ edgep = static_cast<MTaskEdge*>(mtaskp->inEdges().frontp())) {
            LogicMTask* ಧ್ರುವ relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        ಯಾವತ್ (MTaskEdge* ಧ್ರುವ edgep = static_cast<MTaskEdge*>(mtaskp->outEdges().frontp())) {
            LogicMTask* ಧ್ರುವ relativep = edgep->toMTaskp();
            relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(ಏತತ್), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
template <GraphWay::en N_Way>
ಶೂನ್ಯ OrderMTaskGraph::validateWay() ಧ್ರುವ {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv = way.invert();
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) {
        ಧ್ರುವ LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cpCost = 0;
        std::unordered_set<ಧ್ರುವ V3GraphVertex*> relatives;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& graphEdge : mtask.edges<inv>()) {
            ಧ್ರುವ MTaskEdge& edge = *graphEdge.as<MTaskEdge>();
            ಧ್ರುವ LogicMTask& relative = *(edge.furtherp<inv>()->template as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(edge.weight() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ಧ್ರುವ ತಾರ್ಕಿಕ first = relatives.ಸೇರಿಸು(&relative).second;
            UASSERT_OBJ(first, &mtask, "Should be no redundant edges in MTask graph");
            ಧ್ರುವ uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(edge.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(edge.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cpCost = std::max(cpCost, inclusiveCp);
        }
        ಧ್ರುವ uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cpCost, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cpExclusiveFromEdges<N_Way>() == cpCost, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

ಶೂನ್ಯ OrderMTaskGraph::validate() ಧ್ರುವ {
    ಯದಿ (!m_slowAsserts) ಪ್ರತ್ಯಾಗಮ;

    validateWay<GraphWay::FORWARD>();
    validateWay<GraphWay::REVERSE>();

    // Check the dependents set of each MTask agrees with its out-edges
    ಪ್ರತಿ (ಧ್ರುವ V3GraphVertex& vtx : vertices()) {
        ಧ್ರುವ LogicMTask& mtask = *vtx.as<LogicMTask>();
        size_t nDependents = 0;
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& graphEdge : mtask.outEdges()) {
            LogicMTask* ಧ್ರುವ ತುದಿ = graphEdge.as<MTaskEdge>()->toMTaskp();
            UASSERT_OBJ(mtask.hasEdgeTo(ತುದಿ), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.size() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

ವರ್ಗ OrderMTaskGraphBuilder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ಧ್ರುವ VNUser1InUse m_user1InUse;

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
    ಸ್ಥಿರ ತಾರ್ಕಿಕ bypassOk(OrderMoveVertex* mvtxp) {
        // Need to keep all logic vertices
        ಯದಿ (mvtxp->logicp()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Count fan-in, up to 3
        unsigned fanIn = 0;
        auto& inEdges = mvtxp->inEdges();
        ಪ್ರತಿ (auto it = inEdges.begin(); it != inEdges.end(); ++it) {
            ಯದಿ (++fanIn == 3) ಭಙ್ಗ;
        }
        // If fanIn no more than one, bypass
        ಯದಿ (fanIn <= 1) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Count fan-out, up to 3
        unsigned fanOut = 0;
        auto& outEdges = mvtxp->outEdges();
        ಪ್ರತಿ (auto it = outEdges.begin(); it != outEdges.end(); ++it) {
            ಯದಿ (++fanOut == 3) ಭಙ್ಗ;
        }
        // If fan-out no more than one, bypass
        ಯದಿ (fanOut <= 1) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        ಪ್ರತ್ಯಾಗಮ fanIn + fanOut == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    ಶೂನ್ಯ addEdge(LogicMTask* srcp, LogicMTask* dstp) {
        ಯದಿ (srcp->hasEdgeTo(dstp)) ಪ್ರತ್ಯಾಗಮ;  // Don't create redundant edges.
        m_mtaskGraph.addEdge(srcp, dstp);
    }

    // CONSTRUCTORS
    explicit OrderMTaskGraphBuilder(OrderMTaskGraph& mtaskGraph)
        : m_mtaskGraph{mtaskGraph} {

        // Create the LogicMTasks for each OrderMoveVertex
        ಪ್ರತಿ (V3GraphVertex& vtx : mtaskGraph.moveGraph().vertices()) {
            OrderMoveVertex& mVtx = static_cast<OrderMoveVertex&>(vtx);
            ಯದಿ (bypassOk(&mVtx)) {
                mVtx.userp(nullptr);  // Set to nullptr to mark as bypassed
            } ಅನ್ಯಥಾ {
                mVtx.userp(ನೂತನ LogicMTask{mtaskGraph, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& entry = *mtaskGraph.entryp();
        LogicMTask& exit = *mtaskGraph.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        ಪ್ರತಿ (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            ಯದಿ (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) ಅನುವರ್ತ;

            OrderMoveVertex::List& vertexList = mtask.vertexList();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(vertexList.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            OrderMoveVertex* ಧ್ರುವ mVtxp = vertexList.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& dEdge : mVtxp->outEdges()) {
                V3GraphVertex* ಧ್ರುವ ತುದಿ = dEdge.ತುದಿ();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                ಯದಿ (LogicMTask* ಧ್ರುವ otherp = static_cast<LogicMTask*>(ತುದಿ->userp())) {
                    addEdge(&mtask, otherp);
                    ಅನುವರ್ತ;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& tEdge : ತುದಿ->outEdges()) {
                    LogicMTask* ಧ್ರುವ transp = static_cast<LogicMTask*>(tEdge.ತುದಿ()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    addEdge(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        ಪ್ರತಿ (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            ಯದಿ (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) ಅನುವರ್ತ;
            // Add the entry/exit edges if not otherwise connected
            ಯದಿ (mtask.inEmpty()) addEdge(&entry, &mtask);
            ಯದಿ (mtask.outEmpty()) addEdge(&mtask, &exit);
        }
    }
    ~OrderMTaskGraphBuilder() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(OrderMTaskGraphBuilder);
    VL_UNMOVABLE(OrderMTaskGraphBuilder);

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(OrderMTaskGraph& mtaskGraph) { OrderMTaskGraphBuilder{mtaskGraph}; }
};

std::unique_ptr<OrderMTaskGraph> OrderMTaskGraph::build(OrderMoveGraph& moveGraph) {
    std::unique_ptr<OrderMTaskGraph> resp{ನೂತನ OrderMTaskGraph{moveGraph}};
    OrderMTaskGraphBuilder::apply(*resp);
    resp->validate();
    ಪ್ರತ್ಯಾಗಮ resp;
}
