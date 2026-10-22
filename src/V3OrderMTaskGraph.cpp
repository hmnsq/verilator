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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3OrderMTaskGraph.h"

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3InstrCount.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_nextId = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp)
    : V3GraphVertex{&graph} {
    UASSERT(s_nextId < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    सचे (!mVtxp) पटिनिवत्त;
    m_mVertices.linkBack(mVtxp);
    सचे (ध्रुव OrderLogicVertex* ध्रुव olvp = mVtxp->logicp()) {
        m_cost += V3_Instr_गणनम्::गणनम्(olvp->nodep(), सच्च);
    }
}

//######################################################################
// OrderMTaskGraph

OrderMTaskGraph::OrderMTaskGraph(OrderMoveGraph& moveGraph)
    : m_moveGraph{moveGraph}
    , m_entryp{नूतन LogicMTask{*एतत्, nullptr}}
    , m_exitp{नूतन LogicMTask{*एतत्, nullptr}}
    , m_slowAsserts{v3Global.opt.debugPartition()} {}

तार्किक OrderMTaskGraph::pathExistsImpl(LogicMTask* fromp, LogicMTask* शिखरम्,
                                     ध्रुव MTaskEdge* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    सचे (fromp == शिखरम्) पटिनिवत्त सच्च;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    सचे (fromp->cpExclusive<GraphWay::REVERSE>() < शिखरम्->cpInclusive<GraphWay::REVERSE>()) {
        पटिनिवत्त मिच्छा;
    }
    सचे (fromp->cpInclusive<GraphWay::FORWARD>() > शिखरम्->cpExclusive<GraphWay::FORWARD>()) {
        पटिनिवत्त मिच्छा;
    }

    // Recursively look for a path
    अत्थाय (ध्रुव V3GraphEdge& follow : fromp->outEdges()) {
        सचे (&follow == excludedEdgep) अनुवर्त;
        LogicMTask* ध्रुव nextp = static_cast<LogicMTask*>(follow.शिखरम्());
        // Don't visit the same MTask twice in the same search.
        सचे (nextp->m_generation == m_currentGeneration) अनुवर्त;
        सचे (pathExistsImpl(nextp, शिखरम्, nullptr)) पटिनिवत्त सच्च;
    }
    पटिनिवत्त मिच्छा;
}

template <GraphWay::en N_Way>
शून्य OrderMTaskGraph::propagate_प्रक्षिप(LogicMTask* mtaskp) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};
    ध्रुव uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    अत्थाय (V3GraphEdge& graphEdge : mtaskp->edges<way>()) {
        MTaskEdge& edge = static_cast<MTaskEdge&>(graphEdge);

        LogicMTask* ध्रुव relativep = edge.furtherMTaskp<N_Way>();
        EdgeHeap::ग्रन्थिः_2& edge_Heap_ग्रन्थिः = edge.m_edge_Heap_ग्रन्थिः[inv];
        सचे (inclusiveCp > edge_Heap_ग्रन्थिः.कुञ्चिका().m_cp) {
            relativep->m_edgeHeap[inv].increase_कुञ्चिका(&edge_Heap_ग्रन्थिः, inclusiveCp);
        }

        ध्रुव uint64_t relativeCp = relativep->cpExclusive<way>();

        सचे (relativeCp >= inclusiveCp) अनुवर्त;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ध्रुव uint64_t increment = inclusiveCp - relativeCp;

        PropagatePendingHeap::ग्रन्थिः_2*& pendingNodepRef = relativep->m_propagateHeapNodep;
        सचे (PropagatePendingHeap::ग्रन्थिः_2* ध्रुव nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            सचे (increment > nodep->कुञ्चिका().m_increment) {
                m_pendingHeap.increase_कुञ्चिका(nodep, increment);
            }
            अनुवर्त;
        }

        // Add to heap
        PropagatePendingHeap::ग्रन्थिः_2* ध्रुव nodep = m_pending_ग्रन्थिः_Pool.alloc();
        pendingNodepRef = nodep;
        m_pendingHeap.निवेशय(nodep, {increment, relativep->id(), relativep});
    }
}

template <GraphWay::en N_Way>
शून्य OrderMTaskGraph::propagateResolve() {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    याव (!m_pendingHeap.empty()) {
        // Pop max element from heap
        PropagatePendingHeap::ग्रन्थिः_2* ध्रुव maxp = m_pendingHeap.max();
        m_pendingHeap.remove(maxp);
        // Pick up values
        LogicMTask* ध्रुव mtaskp = maxp->कुञ्चिका().m_mtaskp;
        ध्रुव uint64_t cpGrowBy = maxp->कुञ्चिका().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_ग्रन्थिः_Pool.मुक्त(maxp);
        mtaskp->m_propagateHeapNodep = nullptr;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ध्रुव uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        सचे (VL_UNLIKELY(m_slowAsserts)) {
            ध्रुव uint64_t edgeCp = mtaskp->m_edgeHeap[inv].max()->कुञ्चिका().m_cp;
            UASSERT_OBJ(edgeCp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_प्रक्षिप<N_Way>(mtaskp);
    }
}

uint64_t OrderMTaskGraph::totalCost() ध्रुव {
    uint64_t cost = 0;
    अत्थाय (ध्रुव V3GraphVertex& vtx : vertices()) cost += static_cast<ध्रुव LogicMTask&>(vtx).cost();
    पटिनिवत्त cost;
}

शून्य OrderMTaskGraph::addEdge(LogicMTask* fromp, LogicMTask* शिखरम्) {
    UASSERT_OBJ(fromp != शिखरम्, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->hasEdgeTo(शिखरम्), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    नूतन MTaskEdge{एतत्, fromp, शिखरम्};

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
    सचे (fromp->cpInclusive<GraphWay::FORWARD>() > शिखरम्->cpExclusive<GraphWay::FORWARD>()) {
        propagate<GraphWay::FORWARD>(fromp);
    }
    सचे (शिखरम्->cpInclusive<GraphWay::REVERSE>() > fromp->cpExclusive<GraphWay::REVERSE>()) {
        propagate<GraphWay::REVERSE>(शिखरम्);
    }
}

शून्य OrderMTaskGraph::mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    याव (MTaskEdge* ध्रुव edgep = static_cast<MTaskEdge*>(donorp->outEdges().frontp())) {
        LogicMTask* ध्रुव relativep = edgep->toMTaskp();

        relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);

        सचे (relativep == recipientp || recipientp->hasEdgeTo(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } अञ्ञथा {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->addDependent(relativep);
            recipientp->stealRelativeEdge<GraphWay::FORWARD>(edgep);
            relativep->addRelativeEdge<GraphWay::REVERSE>(edgep);
        }
    }

    // Process incoming edges of donor
    याव (MTaskEdge* ध्रुव edgep = static_cast<MTaskEdge*>(donorp->inEdges().frontp())) {
        LogicMTask* ध्रुव relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);

        सचे (relativep == recipientp || relativep->hasEdgeTo(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } अञ्ञथा {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_शिखरम्(recipientp);
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
    ध्रुव uint64_t newCpFwd = recipientp->cpExclusiveFromEdges<GraphWay::FORWARD>();
    ध्रुव uint64_t newCpRev = recipientp->cpExclusiveFromEdges<GraphWay::REVERSE>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<GraphWay::FORWARD>(newCpFwd);
    propagate<GraphWay::FORWARD>(recipientp);
    recipientp->cpExclusive<GraphWay::REVERSE>(newCpRev);
    propagate<GraphWay::REVERSE>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(एतत्), donorp);
}

शून्य OrderMTaskGraph::removeTransitiveEdges() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    अत्थाय (V3GraphVertex& vtx : vertices()) {
        अत्थाय (V3GraphEdge* ध्रुव graphEdgep : vtx.outEdges().unlinkable()) {
            MTaskEdge* ध्रुव edgep = static_cast<MTaskEdge*>(graphEdgep);
            LogicMTask* ध्रुव fromp = edgep->fromMTaskp();
            LogicMTask* ध्रुव शिखरम् = edgep->toMTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            सचे (!pathExists(fromp, शिखरम्, edgep)) अनुवर्त;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(शिखरम्);
            fromp->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            शिखरम्->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

शून्य OrderMTaskGraph::removeEmptyMTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    अत्थाय (V3GraphVertex* ध्रुव vtxp : vertices().unlinkable()) {
        LogicMTask* ध्रुव mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        सचे (mtaskp == m_entryp || mtaskp == m_exitp) अनुवर्त;

        // Keep any MTask that holds logic
        तार्किक empty = सच्च;
        अत्थाय (ध्रुव OrderMoveVertex& mVtx : mtaskp->vertexList()) {
            सचे (mVtx.logicp()) {
                empty = मिच्छा;
                भङ्ग;
            }
        }
        सचे (!empty) अनुवर्त;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->cost(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        अत्थाय (V3GraphEdge& inEdge : mtaskp->inEdges()) {
            LogicMTask* ध्रुव fromp = static_cast<MTaskEdge&>(inEdge).fromMTaskp();
            अत्थाय (V3GraphEdge& outEdge : mtaskp->outEdges()) {
                LogicMTask* ध्रुव शिखरम् = static_cast<MTaskEdge&>(outEdge).toMTaskp();
                सचे (!fromp->hasEdgeTo(शिखरम्)) addEdge(fromp, शिखरम्);
            }
        }

        // Remove incoming edges of 'mtaskp'
        याव (MTaskEdge* ध्रुव edgep = static_cast<MTaskEdge*>(mtaskp->inEdges().frontp())) {
            LogicMTask* ध्रुव relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        याव (MTaskEdge* ध्रुव edgep = static_cast<MTaskEdge*>(mtaskp->outEdges().frontp())) {
            LogicMTask* ध्रुव relativep = edgep->toMTaskp();
            relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(एतत्), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
template <GraphWay::en N_Way>
शून्य OrderMTaskGraph::validateWay() ध्रुव {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv = way.invert();
    अत्थाय (ध्रुव V3GraphVertex& vtx : vertices()) {
        ध्रुव LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cpCost = 0;
        std::unordered_set<ध्रुव V3GraphVertex*> relatives;
        अत्थाय (ध्रुव V3GraphEdge& graphEdge : mtask.edges<inv>()) {
            ध्रुव MTaskEdge& edge = *graphEdge.as<MTaskEdge>();
            ध्रुव LogicMTask& relative = *(edge.furtherp<inv>()->template as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(edge.weight() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ध्रुव तार्किक first = relatives.निवेशय(&relative).second;
            UASSERT_OBJ(first, &mtask, "Should be no redundant edges in MTask graph");
            ध्रुव uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(edge.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(edge.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cpCost = std::max(cpCost, inclusiveCp);
        }
        ध्रुव uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cpCost, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cpExclusiveFromEdges<N_Way>() == cpCost, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

शून्य OrderMTaskGraph::validate() ध्रुव {
    सचे (!m_slowAsserts) पटिनिवत्त;

    validateWay<GraphWay::FORWARD>();
    validateWay<GraphWay::REVERSE>();

    // Check the dependents set of each MTask agrees with its out-edges
    अत्थाय (ध्रुव V3GraphVertex& vtx : vertices()) {
        ध्रुव LogicMTask& mtask = *vtx.as<LogicMTask>();
        size_t nDependents = 0;
        अत्थाय (ध्रुव V3GraphEdge& graphEdge : mtask.outEdges()) {
            LogicMTask* ध्रुव शिखरम् = graphEdge.as<MTaskEdge>()->toMTaskp();
            UASSERT_OBJ(mtask.hasEdgeTo(शिखरम्), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.size() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

वग्ग OrderMTaskGraphBuilder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ध्रुव VNUser1InUse m_user1InUse;

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
    स्थिर तार्किक bypassOk(OrderMoveVertex* mvtxp) {
        // Need to keep all logic vertices
        सचे (mvtxp->logicp()) पटिनिवत्त मिच्छा;
        // Count fan-in, up to 3
        unsigned fanIn = 0;
        auto& inEdges = mvtxp->inEdges();
        अत्थाय (auto it = inEdges.begin(); it != inEdges.end(); ++it) {
            सचे (++fanIn == 3) भङ्ग;
        }
        // If fanIn no more than one, bypass
        सचे (fanIn <= 1) पटिनिवत्त सच्च;
        // Count fan-out, up to 3
        unsigned fanOut = 0;
        auto& outEdges = mvtxp->outEdges();
        अत्थाय (auto it = outEdges.begin(); it != outEdges.end(); ++it) {
            सचे (++fanOut == 3) भङ्ग;
        }
        // If fan-out no more than one, bypass
        सचे (fanOut <= 1) पटिनिवत्त सच्च;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        पटिनिवत्त fanIn + fanOut == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    शून्य addEdge(LogicMTask* srcp, LogicMTask* dstp) {
        सचे (srcp->hasEdgeTo(dstp)) पटिनिवत्त;  // Don't create redundant edges.
        m_mtaskGraph.addEdge(srcp, dstp);
    }

    // CONSTRUCTORS
    explicit OrderMTaskGraphBuilder(OrderMTaskGraph& mtaskGraph)
        : m_mtaskGraph{mtaskGraph} {

        // Create the LogicMTasks for each OrderMoveVertex
        अत्थाय (V3GraphVertex& vtx : mtaskGraph.moveGraph().vertices()) {
            OrderMoveVertex& mVtx = static_cast<OrderMoveVertex&>(vtx);
            सचे (bypassOk(&mVtx)) {
                mVtx.userp(nullptr);  // Set to nullptr to mark as bypassed
            } अञ्ञथा {
                mVtx.userp(नूतन LogicMTask{mtaskGraph, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& entry = *mtaskGraph.entryp();
        LogicMTask& exit = *mtaskGraph.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        अत्थाय (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            सचे (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) अनुवर्त;

            OrderMoveVertex::List& vertexList = mtask.vertexList();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(vertexList.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            OrderMoveVertex* ध्रुव mVtxp = vertexList.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            अत्थाय (ध्रुव V3GraphEdge& dEdge : mVtxp->outEdges()) {
                V3GraphVertex* ध्रुव शिखरम् = dEdge.शिखरम्();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                सचे (LogicMTask* ध्रुव otherp = static_cast<LogicMTask*>(शिखरम्->userp())) {
                    addEdge(&mtask, otherp);
                    अनुवर्त;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                अत्थाय (ध्रुव V3GraphEdge& tEdge : शिखरम्->outEdges()) {
                    LogicMTask* ध्रुव transp = static_cast<LogicMTask*>(tEdge.शिखरम्()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    addEdge(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        अत्थाय (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            सचे (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) अनुवर्त;
            // Add the entry/exit edges if not otherwise connected
            सचे (mtask.inEmpty()) addEdge(&entry, &mtask);
            सचे (mtask.outEmpty()) addEdge(&mtask, &exit);
        }
    }
    ~OrderMTaskGraphBuilder() = स्वभाव;
    VL_UNCOPYABLE(OrderMTaskGraphBuilder);
    VL_UNMOVABLE(OrderMTaskGraphBuilder);

सार्वजनिक:
    स्थिर शून्य apply(OrderMTaskGraph& mtaskGraph) { OrderMTaskGraphBuilder{mtaskGraph}; }
};

std::unique_ptr<OrderMTaskGraph> OrderMTaskGraph::build(OrderMoveGraph& moveGraph) {
    std::unique_ptr<OrderMTaskGraph> resp{नूतन OrderMTaskGraph{moveGraph}};
    OrderMTaskGraphBuilder::apply(*resp);
    resp->validate();
    पटिनिवत्त resp;
}
