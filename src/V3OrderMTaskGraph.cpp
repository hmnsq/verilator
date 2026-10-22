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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3OrderMTaskGraph.h"

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3InstrCount.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_nextId = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp)
    : V3GraphVertex{&graph} {
    UASSERT(s_nextId < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    ଯଦି (!mVtxp) ଫେରନ୍ତୁ;
    m_mVertices.linkBack(mVtxp);
    ଯଦି (ଅଚଳ OrderLogicVertex* ଅଚଳ olvp = mVtxp->logicp()) {
        m_cost += V3_Instr_ଗଣନା::ଗଣନା_2(olvp->nodep(), ସତ୍ୟ);
    }
}

//######################################################################
// OrderMTaskGraph

OrderMTaskGraph::OrderMTaskGraph(OrderMoveGraph& moveGraph)
    : m_moveGraph{moveGraph}
    , m_entryp{ନୂତନ LogicMTask{*ଏହି, nullptr}}
    , m_exitp{ନୂତନ LogicMTask{*ଏହି, nullptr}}
    , m_slowAsserts{v3Global.opt.debugPartition()} {}

ତାର୍କିକ OrderMTaskGraph::pathExistsImpl(LogicMTask* fromp, LogicMTask* ଶୀର୍ଷ,
                                     ଅଚଳ MTaskEdge* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    ଯଦି (fromp == ଶୀର୍ଷ) ଫେରନ୍ତୁ ସତ୍ୟ;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    ଯଦି (fromp->cpExclusive<GraphWay::REVERSE>() < ଶୀର୍ଷ->cpInclusive<GraphWay::REVERSE>()) {
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଯଦି (fromp->cpInclusive<GraphWay::FORWARD>() > ଶୀର୍ଷ->cpExclusive<GraphWay::FORWARD>()) {
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Recursively look for a path
    ପାଇଁ (ଅଚଳ V3GraphEdge& follow : fromp->outEdges()) {
        ଯଦି (&follow == excludedEdgep) ଜାରି;
        LogicMTask* ଅଚଳ nextp = static_cast<LogicMTask*>(follow.ଶୀର୍ଷ());
        // Don't visit the same MTask twice in the same search.
        ଯଦି (nextp->m_generation == m_currentGeneration) ଜାରି;
        ଯଦି (pathExistsImpl(nextp, ଶୀର୍ଷ, nullptr)) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

template <GraphWay::en N_Way>
ଶୂନ୍ୟ OrderMTaskGraph::propagate_ଠେଲ(LogicMTask* mtaskp) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};
    ଅଚଳ uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    ପାଇଁ (V3GraphEdge& graphEdge : mtaskp->edges<way>()) {
        MTaskEdge& edge = static_cast<MTaskEdge&>(graphEdge);

        LogicMTask* ଅଚଳ relativep = edge.furtherMTaskp<N_Way>();
        EdgeHeap::ଗଣ୍ଠି_2& edge_Heap_ଗଣ୍ଠି = edge.m_edge_Heap_ଗଣ୍ଠି[inv];
        ଯଦି (inclusiveCp > edge_Heap_ଗଣ୍ଠି.ଚାବି().m_cp) {
            relativep->m_edgeHeap[inv].increase_ଚାବି(&edge_Heap_ଗଣ୍ଠି, inclusiveCp);
        }

        ଅଚଳ uint64_t relativeCp = relativep->cpExclusive<way>();

        ଯଦି (relativeCp >= inclusiveCp) ଜାରି;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ଅଚଳ uint64_t increment = inclusiveCp - relativeCp;

        PropagatePendingHeap::ଗଣ୍ଠି_2*& pendingNodepRef = relativep->m_propagateHeapNodep;
        ଯଦି (PropagatePendingHeap::ଗଣ୍ଠି_2* ଅଚଳ nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            ଯଦି (increment > nodep->ଚାବି().m_increment) {
                m_pendingHeap.increase_ଚାବି(nodep, increment);
            }
            ଜାରି;
        }

        // Add to heap
        PropagatePendingHeap::ଗଣ୍ଠି_2* ଅଚଳ nodep = m_pending_ଗଣ୍ଠି_Pool.alloc();
        pendingNodepRef = nodep;
        m_pendingHeap.ଭର୍ତ୍ତି(nodep, {increment, relativep->id(), relativep});
    }
}

template <GraphWay::en N_Way>
ଶୂନ୍ୟ OrderMTaskGraph::propagateResolve() {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    ଯେତେବେଳେ (!m_pendingHeap.empty()) {
        // Pop max element from heap
        PropagatePendingHeap::ଗଣ୍ଠି_2* ଅଚଳ maxp = m_pendingHeap.max();
        m_pendingHeap.remove(maxp);
        // Pick up values
        LogicMTask* ଅଚଳ mtaskp = maxp->ଚାବି().m_mtaskp;
        ଅଚଳ uint64_t cpGrowBy = maxp->ଚାବି().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_ଗଣ୍ଠି_Pool.ମୁକ୍ତ(maxp);
        mtaskp->m_propagateHeapNodep = nullptr;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ଅଚଳ uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        ଯଦି (VL_UNLIKELY(m_slowAsserts)) {
            ଅଚଳ uint64_t edgeCp = mtaskp->m_edgeHeap[inv].max()->ଚାବି().m_cp;
            UASSERT_OBJ(edgeCp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_ଠେଲ<N_Way>(mtaskp);
    }
}

uint64_t OrderMTaskGraph::totalCost() ଅଚଳ {
    uint64_t cost = 0;
    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) cost += static_cast<ଅଚଳ LogicMTask&>(vtx).cost();
    ଫେରନ୍ତୁ cost;
}

ଶୂନ୍ୟ OrderMTaskGraph::addEdge(LogicMTask* fromp, LogicMTask* ଶୀର୍ଷ) {
    UASSERT_OBJ(fromp != ଶୀର୍ଷ, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->hasEdgeTo(ଶୀର୍ଷ), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    ନୂତନ MTaskEdge{ଏହି, fromp, ଶୀର୍ଷ};

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
    ଯଦି (fromp->cpInclusive<GraphWay::FORWARD>() > ଶୀର୍ଷ->cpExclusive<GraphWay::FORWARD>()) {
        propagate<GraphWay::FORWARD>(fromp);
    }
    ଯଦି (ଶୀର୍ଷ->cpInclusive<GraphWay::REVERSE>() > fromp->cpExclusive<GraphWay::REVERSE>()) {
        propagate<GraphWay::REVERSE>(ଶୀର୍ଷ);
    }
}

ଶୂନ୍ୟ OrderMTaskGraph::mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    ଯେତେବେଳେ (MTaskEdge* ଅଚଳ edgep = static_cast<MTaskEdge*>(donorp->outEdges().frontp())) {
        LogicMTask* ଅଚଳ relativep = edgep->toMTaskp();

        relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);

        ଯଦି (relativep == recipientp || recipientp->hasEdgeTo(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ଅନ୍ୟଥା {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->addDependent(relativep);
            recipientp->stealRelativeEdge<GraphWay::FORWARD>(edgep);
            relativep->addRelativeEdge<GraphWay::REVERSE>(edgep);
        }
    }

    // Process incoming edges of donor
    ଯେତେବେଳେ (MTaskEdge* ଅଚଳ edgep = static_cast<MTaskEdge*>(donorp->inEdges().frontp())) {
        LogicMTask* ଅଚଳ relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);

        ଯଦି (relativep == recipientp || relativep->hasEdgeTo(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } ଅନ୍ୟଥା {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_ଶୀର୍ଷ(recipientp);
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
    ଅଚଳ uint64_t newCpFwd = recipientp->cpExclusiveFromEdges<GraphWay::FORWARD>();
    ଅଚଳ uint64_t newCpRev = recipientp->cpExclusiveFromEdges<GraphWay::REVERSE>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<GraphWay::FORWARD>(newCpFwd);
    propagate<GraphWay::FORWARD>(recipientp);
    recipientp->cpExclusive<GraphWay::REVERSE>(newCpRev);
    propagate<GraphWay::REVERSE>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(ଏହି), donorp);
}

ଶୂନ୍ୟ OrderMTaskGraph::removeTransitiveEdges() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    ପାଇଁ (V3GraphVertex& vtx : vertices()) {
        ପାଇଁ (V3GraphEdge* ଅଚଳ graphEdgep : vtx.outEdges().unlinkable()) {
            MTaskEdge* ଅଚଳ edgep = static_cast<MTaskEdge*>(graphEdgep);
            LogicMTask* ଅଚଳ fromp = edgep->fromMTaskp();
            LogicMTask* ଅଚଳ ଶୀର୍ଷ = edgep->toMTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            ଯଦି (!pathExists(fromp, ଶୀର୍ଷ, edgep)) ଜାରି;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(ଶୀର୍ଷ);
            fromp->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            ଶୀର୍ଷ->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

ଶୂନ୍ୟ OrderMTaskGraph::removeEmptyMTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : vertices().unlinkable()) {
        LogicMTask* ଅଚଳ mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        ଯଦି (mtaskp == m_entryp || mtaskp == m_exitp) ଜାରି;

        // Keep any MTask that holds logic
        ତାର୍କିକ empty = ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ OrderMoveVertex& mVtx : mtaskp->vertexList()) {
            ଯଦି (mVtx.logicp()) {
                empty = ମିଥ୍ୟା;
                ଭାଙ୍ଗ;
            }
        }
        ଯଦି (!empty) ଜାରି;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->cost(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        ପାଇଁ (V3GraphEdge& inEdge : mtaskp->inEdges()) {
            LogicMTask* ଅଚଳ fromp = static_cast<MTaskEdge&>(inEdge).fromMTaskp();
            ପାଇଁ (V3GraphEdge& outEdge : mtaskp->outEdges()) {
                LogicMTask* ଅଚଳ ଶୀର୍ଷ = static_cast<MTaskEdge&>(outEdge).toMTaskp();
                ଯଦି (!fromp->hasEdgeTo(ଶୀର୍ଷ)) addEdge(fromp, ଶୀର୍ଷ);
            }
        }

        // Remove incoming edges of 'mtaskp'
        ଯେତେବେଳେ (MTaskEdge* ଅଚଳ edgep = static_cast<MTaskEdge*>(mtaskp->inEdges().frontp())) {
            LogicMTask* ଅଚଳ relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        ଯେତେବେଳେ (MTaskEdge* ଅଚଳ edgep = static_cast<MTaskEdge*>(mtaskp->outEdges().frontp())) {
            LogicMTask* ଅଚଳ relativep = edgep->toMTaskp();
            relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(ଏହି), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
template <GraphWay::en N_Way>
ଶୂନ୍ୟ OrderMTaskGraph::validateWay() ଅଚଳ {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv = way.invert();
    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) {
        ଅଚଳ LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cpCost = 0;
        std::unordered_set<ଅଚଳ V3GraphVertex*> relatives;
        ପାଇଁ (ଅଚଳ V3GraphEdge& graphEdge : mtask.edges<inv>()) {
            ଅଚଳ MTaskEdge& edge = *graphEdge.as<MTaskEdge>();
            ଅଚଳ LogicMTask& relative = *(edge.furtherp<inv>()->template as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(edge.weight() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ଅଚଳ ତାର୍କିକ first = relatives.ଭର୍ତ୍ତି(&relative).second;
            UASSERT_OBJ(first, &mtask, "Should be no redundant edges in MTask graph");
            ଅଚଳ uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(edge.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(edge.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cpCost = std::max(cpCost, inclusiveCp);
        }
        ଅଚଳ uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cpCost, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cpExclusiveFromEdges<N_Way>() == cpCost, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

ଶୂନ୍ୟ OrderMTaskGraph::validate() ଅଚଳ {
    ଯଦି (!m_slowAsserts) ଫେରନ୍ତୁ;

    validateWay<GraphWay::FORWARD>();
    validateWay<GraphWay::REVERSE>();

    // Check the dependents set of each MTask agrees with its out-edges
    ପାଇଁ (ଅଚଳ V3GraphVertex& vtx : vertices()) {
        ଅଚଳ LogicMTask& mtask = *vtx.as<LogicMTask>();
        size_t nDependents = 0;
        ପାଇଁ (ଅଚଳ V3GraphEdge& graphEdge : mtask.outEdges()) {
            LogicMTask* ଅଚଳ ଶୀର୍ଷ = graphEdge.as<MTaskEdge>()->toMTaskp();
            UASSERT_OBJ(mtask.hasEdgeTo(ଶୀର୍ଷ), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.size() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

ଶ୍ରେଣୀ OrderMTaskGraphBuilder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ଅଚଳ VNUser1InUse m_user1InUse;

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
    ସ୍ଥିର ତାର୍କିକ bypassOk(OrderMoveVertex* mvtxp) {
        // Need to keep all logic vertices
        ଯଦି (mvtxp->logicp()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Count fan-in, up to 3
        unsigned fanIn = 0;
        auto& inEdges = mvtxp->inEdges();
        ପାଇଁ (auto it = inEdges.begin(); it != inEdges.end(); ++it) {
            ଯଦି (++fanIn == 3) ଭାଙ୍ଗ;
        }
        // If fanIn no more than one, bypass
        ଯଦି (fanIn <= 1) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Count fan-out, up to 3
        unsigned fanOut = 0;
        auto& outEdges = mvtxp->outEdges();
        ପାଇଁ (auto it = outEdges.begin(); it != outEdges.end(); ++it) {
            ଯଦି (++fanOut == 3) ଭାଙ୍ଗ;
        }
        // If fan-out no more than one, bypass
        ଯଦି (fanOut <= 1) ଫେରନ୍ତୁ ସତ୍ୟ;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        ଫେରନ୍ତୁ fanIn + fanOut == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    ଶୂନ୍ୟ addEdge(LogicMTask* srcp, LogicMTask* dstp) {
        ଯଦି (srcp->hasEdgeTo(dstp)) ଫେରନ୍ତୁ;  // Don't create redundant edges.
        m_mtaskGraph.addEdge(srcp, dstp);
    }

    // CONSTRUCTORS
    explicit OrderMTaskGraphBuilder(OrderMTaskGraph& mtaskGraph)
        : m_mtaskGraph{mtaskGraph} {

        // Create the LogicMTasks for each OrderMoveVertex
        ପାଇଁ (V3GraphVertex& vtx : mtaskGraph.moveGraph().vertices()) {
            OrderMoveVertex& mVtx = static_cast<OrderMoveVertex&>(vtx);
            ଯଦି (bypassOk(&mVtx)) {
                mVtx.userp(nullptr);  // Set to nullptr to mark as bypassed
            } ଅନ୍ୟଥା {
                mVtx.userp(ନୂତନ LogicMTask{mtaskGraph, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& entry = *mtaskGraph.entryp();
        LogicMTask& exit = *mtaskGraph.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        ପାଇଁ (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            ଯଦି (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) ଜାରି;

            OrderMoveVertex::List& vertexList = mtask.vertexList();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(vertexList.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            OrderMoveVertex* ଅଚଳ mVtxp = vertexList.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            ପାଇଁ (ଅଚଳ V3GraphEdge& dEdge : mVtxp->outEdges()) {
                V3GraphVertex* ଅଚଳ ଶୀର୍ଷ = dEdge.ଶୀର୍ଷ();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                ଯଦି (LogicMTask* ଅଚଳ otherp = static_cast<LogicMTask*>(ଶୀର୍ଷ->userp())) {
                    addEdge(&mtask, otherp);
                    ଜାରି;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                ପାଇଁ (ଅଚଳ V3GraphEdge& tEdge : ଶୀର୍ଷ->outEdges()) {
                    LogicMTask* ଅଚଳ transp = static_cast<LogicMTask*>(tEdge.ଶୀର୍ଷ()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    addEdge(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        ପାଇଁ (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            ଯଦି (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) ଜାରି;
            // Add the entry/exit edges if not otherwise connected
            ଯଦି (mtask.inEmpty()) addEdge(&entry, &mtask);
            ଯଦି (mtask.outEmpty()) addEdge(&mtask, &exit);
        }
    }
    ~OrderMTaskGraphBuilder() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(OrderMTaskGraphBuilder);
    VL_UNMOVABLE(OrderMTaskGraphBuilder);

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(OrderMTaskGraph& mtaskGraph) { OrderMTaskGraphBuilder{mtaskGraph}; }
};

std::unique_ptr<OrderMTaskGraph> OrderMTaskGraph::build(OrderMoveGraph& moveGraph) {
    std::unique_ptr<OrderMTaskGraph> resp{ନୂତନ OrderMTaskGraph{moveGraph}};
    OrderMTaskGraphBuilder::apply(*resp);
    resp->validate();
    ଫେରନ୍ତୁ resp;
}
