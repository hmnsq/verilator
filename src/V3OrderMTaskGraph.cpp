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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3OrderMTaskGraph.h"

#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3InstrCount.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_nextId = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp)
    : V3GraphVertex{&graph} {
    UASSERT(s_nextId < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    എങ്കിൽ (!mVtxp) തിരികെ;
    m_mVertices.linkBack(mVtxp);
    എങ്കിൽ (സ്ഥിരം OrderLogicVertex* സ്ഥിരം olvp = mVtxp->logicp()) {
        m_cost += V3_Instr_എണ്ണം::എണ്ണം(olvp->nodep(), സത്യം);
    }
}

//######################################################################
// OrderMTaskGraph

OrderMTaskGraph::OrderMTaskGraph(OrderMoveGraph& moveGraph)
    : m_moveGraph{moveGraph}
    , m_entryp{പുതിയ LogicMTask{*ഇത്, nullptr}}
    , m_exitp{പുതിയ LogicMTask{*ഇത്, nullptr}}
    , m_slowAsserts{v3Global.opt.debugPartition()} {}

തർക്കം OrderMTaskGraph::pathExistsImpl(LogicMTask* fromp, LogicMTask* മുകൾ,
                                     സ്ഥിരം MTaskEdge* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    എങ്കിൽ (fromp == മുകൾ) തിരികെ സത്യം;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    എങ്കിൽ (fromp->cpExclusive<GraphWay::REVERSE>() < മുകൾ->cpInclusive<GraphWay::REVERSE>()) {
        തിരികെ അസത്യം;
    }
    എങ്കിൽ (fromp->cpInclusive<GraphWay::FORWARD>() > മുകൾ->cpExclusive<GraphWay::FORWARD>()) {
        തിരികെ അസത്യം;
    }

    // Recursively look for a path
    വേണ്ടി (സ്ഥിരം V3GraphEdge& follow : fromp->outEdges()) {
        എങ്കിൽ (&follow == excludedEdgep) തുടരുക;
        LogicMTask* സ്ഥിരം nextp = static_cast<LogicMTask*>(follow.മുകൾ());
        // Don't visit the same MTask twice in the same search.
        എങ്കിൽ (nextp->m_generation == m_currentGeneration) തുടരുക;
        എങ്കിൽ (pathExistsImpl(nextp, മുകൾ, nullptr)) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}

template <GraphWay::en N_Way>
ശൂന്യം OrderMTaskGraph::propagate_തള്ളുക(LogicMTask* mtaskp) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};
    സ്ഥിരം uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    വേണ്ടി (V3GraphEdge& graphEdge : mtaskp->edges<way>()) {
        MTaskEdge& edge = static_cast<MTaskEdge&>(graphEdge);

        LogicMTask* സ്ഥിരം relativep = edge.furtherMTaskp<N_Way>();
        EdgeHeap::കണ്ണി_2& edge_Heap_കണ്ണി = edge.m_edge_Heap_കണ്ണി[inv];
        എങ്കിൽ (inclusiveCp > edge_Heap_കണ്ണി.താക്കോൽ().m_cp) {
            relativep->m_edgeHeap[inv].increase_താക്കോൽ(&edge_Heap_കണ്ണി, inclusiveCp);
        }

        സ്ഥിരം uint64_t relativeCp = relativep->cpExclusive<way>();

        എങ്കിൽ (relativeCp >= inclusiveCp) തുടരുക;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        സ്ഥിരം uint64_t increment = inclusiveCp - relativeCp;

        PropagatePendingHeap::കണ്ണി_2*& pendingNodepRef = relativep->m_propagateHeapNodep;
        എങ്കിൽ (PropagatePendingHeap::കണ്ണി_2* സ്ഥിരം nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            എങ്കിൽ (increment > nodep->താക്കോൽ().m_increment) {
                m_pendingHeap.increase_താക്കോൽ(nodep, increment);
            }
            തുടരുക;
        }

        // Add to heap
        PropagatePendingHeap::കണ്ണി_2* സ്ഥിരം nodep = m_pending_കണ്ണി_Pool.alloc();
        pendingNodepRef = nodep;
        m_pendingHeap.തിരുകുക(nodep, {increment, relativep->id(), relativep});
    }
}

template <GraphWay::en N_Way>
ശൂന്യം OrderMTaskGraph::propagateResolve() {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    ആകുമ്പോൾ (!m_pendingHeap.empty()) {
        // Pop max element from heap
        PropagatePendingHeap::കണ്ണി_2* സ്ഥിരം maxp = m_pendingHeap.max();
        m_pendingHeap.remove(maxp);
        // Pick up values
        LogicMTask* സ്ഥിരം mtaskp = maxp->താക്കോൽ().m_mtaskp;
        സ്ഥിരം uint64_t cpGrowBy = maxp->താക്കോൽ().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_കണ്ണി_Pool.സ്വതന്ത്രം(maxp);
        mtaskp->m_propagateHeapNodep = nullptr;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        സ്ഥിരം uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        എങ്കിൽ (VL_UNLIKELY(m_slowAsserts)) {
            സ്ഥിരം uint64_t edgeCp = mtaskp->m_edgeHeap[inv].max()->താക്കോൽ().m_cp;
            UASSERT_OBJ(edgeCp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_തള്ളുക<N_Way>(mtaskp);
    }
}

uint64_t OrderMTaskGraph::totalCost() സ്ഥിരം {
    uint64_t cost = 0;
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) cost += static_cast<സ്ഥിരം LogicMTask&>(vtx).cost();
    തിരികെ cost;
}

ശൂന്യം OrderMTaskGraph::addEdge(LogicMTask* fromp, LogicMTask* മുകൾ) {
    UASSERT_OBJ(fromp != മുകൾ, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->hasEdgeTo(മുകൾ), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    പുതിയ MTaskEdge{ഇത്, fromp, മുകൾ};

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
    എങ്കിൽ (fromp->cpInclusive<GraphWay::FORWARD>() > മുകൾ->cpExclusive<GraphWay::FORWARD>()) {
        propagate<GraphWay::FORWARD>(fromp);
    }
    എങ്കിൽ (മുകൾ->cpInclusive<GraphWay::REVERSE>() > fromp->cpExclusive<GraphWay::REVERSE>()) {
        propagate<GraphWay::REVERSE>(മുകൾ);
    }
}

ശൂന്യം OrderMTaskGraph::mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    ആകുമ്പോൾ (MTaskEdge* സ്ഥിരം edgep = static_cast<MTaskEdge*>(donorp->outEdges().frontp())) {
        LogicMTask* സ്ഥിരം relativep = edgep->toMTaskp();

        relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);

        എങ്കിൽ (relativep == recipientp || recipientp->hasEdgeTo(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } അല്ലെങ്കിൽ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->addDependent(relativep);
            recipientp->stealRelativeEdge<GraphWay::FORWARD>(edgep);
            relativep->addRelativeEdge<GraphWay::REVERSE>(edgep);
        }
    }

    // Process incoming edges of donor
    ആകുമ്പോൾ (MTaskEdge* സ്ഥിരം edgep = static_cast<MTaskEdge*>(donorp->inEdges().frontp())) {
        LogicMTask* സ്ഥിരം relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);

        എങ്കിൽ (relativep == recipientp || relativep->hasEdgeTo(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } അല്ലെങ്കിൽ {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_മുകൾ(recipientp);
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
    സ്ഥിരം uint64_t newCpFwd = recipientp->cpExclusiveFromEdges<GraphWay::FORWARD>();
    സ്ഥിരം uint64_t newCpRev = recipientp->cpExclusiveFromEdges<GraphWay::REVERSE>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<GraphWay::FORWARD>(newCpFwd);
    propagate<GraphWay::FORWARD>(recipientp);
    recipientp->cpExclusive<GraphWay::REVERSE>(newCpRev);
    propagate<GraphWay::REVERSE>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(ഇത്), donorp);
}

ശൂന്യം OrderMTaskGraph::removeTransitiveEdges() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    വേണ്ടി (V3GraphVertex& vtx : vertices()) {
        വേണ്ടി (V3GraphEdge* സ്ഥിരം graphEdgep : vtx.outEdges().unlinkable()) {
            MTaskEdge* സ്ഥിരം edgep = static_cast<MTaskEdge*>(graphEdgep);
            LogicMTask* സ്ഥിരം fromp = edgep->fromMTaskp();
            LogicMTask* സ്ഥിരം മുകൾ = edgep->toMTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            എങ്കിൽ (!pathExists(fromp, മുകൾ, edgep)) തുടരുക;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(മുകൾ);
            fromp->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            മുകൾ->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

ശൂന്യം OrderMTaskGraph::removeEmptyMTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : vertices().unlinkable()) {
        LogicMTask* സ്ഥിരം mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        എങ്കിൽ (mtaskp == m_entryp || mtaskp == m_exitp) തുടരുക;

        // Keep any MTask that holds logic
        തർക്കം empty = സത്യം;
        വേണ്ടി (സ്ഥിരം OrderMoveVertex& mVtx : mtaskp->vertexList()) {
            എങ്കിൽ (mVtx.logicp()) {
                empty = അസത്യം;
                നിർത്തുക;
            }
        }
        എങ്കിൽ (!empty) തുടരുക;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->cost(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        വേണ്ടി (V3GraphEdge& inEdge : mtaskp->inEdges()) {
            LogicMTask* സ്ഥിരം fromp = static_cast<MTaskEdge&>(inEdge).fromMTaskp();
            വേണ്ടി (V3GraphEdge& outEdge : mtaskp->outEdges()) {
                LogicMTask* സ്ഥിരം മുകൾ = static_cast<MTaskEdge&>(outEdge).toMTaskp();
                എങ്കിൽ (!fromp->hasEdgeTo(മുകൾ)) addEdge(fromp, മുകൾ);
            }
        }

        // Remove incoming edges of 'mtaskp'
        ആകുമ്പോൾ (MTaskEdge* സ്ഥിരം edgep = static_cast<MTaskEdge*>(mtaskp->inEdges().frontp())) {
            LogicMTask* സ്ഥിരം relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        ആകുമ്പോൾ (MTaskEdge* സ്ഥിരം edgep = static_cast<MTaskEdge*>(mtaskp->outEdges().frontp())) {
            LogicMTask* സ്ഥിരം relativep = edgep->toMTaskp();
            relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(ഇത്), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
template <GraphWay::en N_Way>
ശൂന്യം OrderMTaskGraph::validateWay() സ്ഥിരം {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv = way.invert();
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) {
        സ്ഥിരം LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cpCost = 0;
        std::unordered_set<സ്ഥിരം V3GraphVertex*> relatives;
        വേണ്ടി (സ്ഥിരം V3GraphEdge& graphEdge : mtask.edges<inv>()) {
            സ്ഥിരം MTaskEdge& edge = *graphEdge.as<MTaskEdge>();
            സ്ഥിരം LogicMTask& relative = *(edge.furtherp<inv>()->template as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(edge.weight() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            സ്ഥിരം തർക്കം first = relatives.തിരുകുക(&relative).second;
            UASSERT_OBJ(first, &mtask, "Should be no redundant edges in MTask graph");
            സ്ഥിരം uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(edge.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(edge.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cpCost = std::max(cpCost, inclusiveCp);
        }
        സ്ഥിരം uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cpCost, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cpExclusiveFromEdges<N_Way>() == cpCost, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

ശൂന്യം OrderMTaskGraph::validate() സ്ഥിരം {
    എങ്കിൽ (!m_slowAsserts) തിരികെ;

    validateWay<GraphWay::FORWARD>();
    validateWay<GraphWay::REVERSE>();

    // Check the dependents set of each MTask agrees with its out-edges
    വേണ്ടി (സ്ഥിരം V3GraphVertex& vtx : vertices()) {
        സ്ഥിരം LogicMTask& mtask = *vtx.as<LogicMTask>();
        size_t nDependents = 0;
        വേണ്ടി (സ്ഥിരം V3GraphEdge& graphEdge : mtask.outEdges()) {
            LogicMTask* സ്ഥിരം മുകൾ = graphEdge.as<MTaskEdge>()->toMTaskp();
            UASSERT_OBJ(mtask.hasEdgeTo(മുകൾ), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.size() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

വർഗ്ഗം OrderMTaskGraphBuilder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    സ്ഥിരം VNUser1InUse m_user1InUse;

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
    സ്ഥിതം തർക്കം bypassOk(OrderMoveVertex* mvtxp) {
        // Need to keep all logic vertices
        എങ്കിൽ (mvtxp->logicp()) തിരികെ അസത്യം;
        // Count fan-in, up to 3
        unsigned fanIn = 0;
        auto& inEdges = mvtxp->inEdges();
        വേണ്ടി (auto it = inEdges.begin(); it != inEdges.end(); ++it) {
            എങ്കിൽ (++fanIn == 3) നിർത്തുക;
        }
        // If fanIn no more than one, bypass
        എങ്കിൽ (fanIn <= 1) തിരികെ സത്യം;
        // Count fan-out, up to 3
        unsigned fanOut = 0;
        auto& outEdges = mvtxp->outEdges();
        വേണ്ടി (auto it = outEdges.begin(); it != outEdges.end(); ++it) {
            എങ്കിൽ (++fanOut == 3) നിർത്തുക;
        }
        // If fan-out no more than one, bypass
        എങ്കിൽ (fanOut <= 1) തിരികെ സത്യം;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        തിരികെ fanIn + fanOut == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    ശൂന്യം addEdge(LogicMTask* srcp, LogicMTask* dstp) {
        എങ്കിൽ (srcp->hasEdgeTo(dstp)) തിരികെ;  // Don't create redundant edges.
        m_mtaskGraph.addEdge(srcp, dstp);
    }

    // CONSTRUCTORS
    explicit OrderMTaskGraphBuilder(OrderMTaskGraph& mtaskGraph)
        : m_mtaskGraph{mtaskGraph} {

        // Create the LogicMTasks for each OrderMoveVertex
        വേണ്ടി (V3GraphVertex& vtx : mtaskGraph.moveGraph().vertices()) {
            OrderMoveVertex& mVtx = static_cast<OrderMoveVertex&>(vtx);
            എങ്കിൽ (bypassOk(&mVtx)) {
                mVtx.userp(nullptr);  // Set to nullptr to mark as bypassed
            } അല്ലെങ്കിൽ {
                mVtx.userp(പുതിയ LogicMTask{mtaskGraph, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& entry = *mtaskGraph.entryp();
        LogicMTask& exit = *mtaskGraph.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        വേണ്ടി (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            എങ്കിൽ (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) തുടരുക;

            OrderMoveVertex::List& vertexList = mtask.vertexList();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(vertexList.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            OrderMoveVertex* സ്ഥിരം mVtxp = vertexList.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            വേണ്ടി (സ്ഥിരം V3GraphEdge& dEdge : mVtxp->outEdges()) {
                V3GraphVertex* സ്ഥിരം മുകൾ = dEdge.മുകൾ();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                എങ്കിൽ (LogicMTask* സ്ഥിരം otherp = static_cast<LogicMTask*>(മുകൾ->userp())) {
                    addEdge(&mtask, otherp);
                    തുടരുക;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                വേണ്ടി (സ്ഥിരം V3GraphEdge& tEdge : മുകൾ->outEdges()) {
                    LogicMTask* സ്ഥിരം transp = static_cast<LogicMTask*>(tEdge.മുകൾ()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    addEdge(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        വേണ്ടി (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            എങ്കിൽ (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) തുടരുക;
            // Add the entry/exit edges if not otherwise connected
            എങ്കിൽ (mtask.inEmpty()) addEdge(&entry, &mtask);
            എങ്കിൽ (mtask.outEmpty()) addEdge(&mtask, &exit);
        }
    }
    ~OrderMTaskGraphBuilder() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(OrderMTaskGraphBuilder);
    VL_UNMOVABLE(OrderMTaskGraphBuilder);

പൊതു:
    സ്ഥിതം ശൂന്യം apply(OrderMTaskGraph& mtaskGraph) { OrderMTaskGraphBuilder{mtaskGraph}; }
};

std::unique_ptr<OrderMTaskGraph> OrderMTaskGraph::build(OrderMoveGraph& moveGraph) {
    std::unique_ptr<OrderMTaskGraph> resp{പുതിയ OrderMTaskGraph{moveGraph}};
    OrderMTaskGraphBuilder::apply(*resp);
    resp->validate();
    തിരികെ resp;
}
