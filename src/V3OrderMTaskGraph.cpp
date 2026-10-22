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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3OrderMTaskGraph.h"

#समावेश "V3Global.h"
#समावेश "V3InstrCount.h"

#समावेश <algorithm>
#समावेश <memory>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_अग्र_Id = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp)
    : V3GraphVertex{&graph} {
    UASSERT(s_अग्र_Id < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    अगर (!mVtxp) वापस;
    m_mVertices.linkBack(mVtxp);
    अगर (स्थिर OrderLogicVertex* स्थिर olvp = mVtxp->logicp()) {
        m_cost += V3_Instr_गिनती::गिनती(olvp->nodep(), true);
    }
}

//######################################################################
// OrderMTaskGraph

OrderMTaskGraph::OrderMTaskGraph(OrderMoveGraph& moveGraph)
    : m_moveGraph{moveGraph}
    , m_entryp{नया LogicMTask{*यह, nullptr}}
    , m_exitp{नया LogicMTask{*यह, nullptr}}
    , m_slowAsserts{v3Global.opt.debugPartition()} {}

bool OrderMTaskGraph::pathExistsImpl(LogicMTask* fromp, LogicMTask* शीर्ष,
                                     स्थिर MTaskEdge* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    अगर (fromp == शीर्ष) वापस true;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    अगर (fromp->cpExclusive<GraphWay::REVERSE>() < शीर्ष->cpInclusive<GraphWay::REVERSE>()) {
        वापस false;
    }
    अगर (fromp->cpInclusive<GraphWay::FORWARD>() > शीर्ष->cpExclusive<GraphWay::FORWARD>()) {
        वापस false;
    }

    // Recursively look for a path
    क्रम (स्थिर V3GraphEdge& follow : fromp->outEdges()) {
        अगर (&follow == excludedEdgep) जारी;
        LogicMTask* स्थिर nextp = static_cast<LogicMTask*>(follow.शीर्ष());
        // Don't visit the same MTask twice in the same search.
        अगर (nextp->m_generation == m_currentGeneration) जारी;
        अगर (pathExistsImpl(nextp, शीर्ष, nullptr)) वापस true;
    }
    वापस false;
}

ढाँचा <GraphWay::en N_Way>
व्योम OrderMTaskGraph::propagate_धकेलो(LogicMTask* mtaskp) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};
    स्थिर uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    क्रम (V3GraphEdge& graphEdge : mtaskp->edges<way>()) {
        MTaskEdge& edge = static_cast<MTaskEdge&>(graphEdge);

        LogicMTask* स्थिर relativep = edge.furtherMTaskp<N_Way>();
        EdgeHeap::गाँठ_2& edge_Heap_गाँठ = edge.m_edge_Heap_गाँठ[inv];
        अगर (inclusiveCp > edge_Heap_गाँठ.कुंजी().m_cp) {
            relativep->m_edgeHeap[inv].increase_कुंजी(&edge_Heap_गाँठ, inclusiveCp);
        }

        स्थिर uint64_t relativeCp = relativep->cpExclusive<way>();

        अगर (relativeCp >= inclusiveCp) जारी;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        स्थिर uint64_t increment = inclusiveCp - relativeCp;

        PropagatePendingHeap::गाँठ_2*& pendingNodepRef = relativep->m_propagateHeapNodep;
        अगर (PropagatePendingHeap::गाँठ_2* स्थिर nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            अगर (increment > nodep->कुंजी().m_increment) {
                m_pendingHeap.increase_कुंजी(nodep, increment);
            }
            जारी;
        }

        // Add to heap
        PropagatePendingHeap::गाँठ_2* स्थिर nodep = m_pending_गाँठ_Pool.alloc();
        pendingNodepRef = nodep;
        m_pendingHeap.जोड़ो(nodep, {increment, relativep->id(), relativep});
    }
}

ढाँचा <GraphWay::en N_Way>
व्योम OrderMTaskGraph::propagateResolve() {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    जबतक (!m_pendingHeap.empty()) {
        // Pop max element from heap
        PropagatePendingHeap::गाँठ_2* स्थिर maxp = m_pendingHeap.max();
        m_pendingHeap.हटाओ(maxp);
        // Pick up values
        LogicMTask* स्थिर mtaskp = maxp->कुंजी().m_mtaskp;
        स्थिर uint64_t cpGrowBy = maxp->कुंजी().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pending_गाँठ_Pool.मुक्त(maxp);
        mtaskp->m_propagateHeapNodep = nullptr;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        स्थिर uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        अगर (VL_UNLIKELY(m_slowAsserts)) {
            स्थिर uint64_t edgeCp = mtaskp->m_edgeHeap[inv].max()->कुंजी().m_cp;
            UASSERT_OBJ(edgeCp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagate_धकेलो<N_Way>(mtaskp);
    }
}

uint64_t OrderMTaskGraph::योग_Cost() स्थिर {
    uint64_t cost = 0;
    क्रम (स्थिर V3GraphVertex& vtx : vertices()) cost += static_cast<स्थिर LogicMTask&>(vtx).cost();
    वापस cost;
}

व्योम OrderMTaskGraph::addEdge(LogicMTask* fromp, LogicMTask* शीर्ष) {
    UASSERT_OBJ(fromp != शीर्ष, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->hasEdgeTo(शीर्ष), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    नया MTaskEdge{यह, fromp, शीर्ष};

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
    अगर (fromp->cpInclusive<GraphWay::FORWARD>() > शीर्ष->cpExclusive<GraphWay::FORWARD>()) {
        propagate<GraphWay::FORWARD>(fromp);
    }
    अगर (शीर्ष->cpInclusive<GraphWay::REVERSE>() > fromp->cpExclusive<GraphWay::REVERSE>()) {
        propagate<GraphWay::REVERSE>(शीर्ष);
    }
}

व्योम OrderMTaskGraph::mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    जबतक (MTaskEdge* स्थिर edgep = static_cast<MTaskEdge*>(donorp->outEdges().frontp())) {
        LogicMTask* स्थिर relativep = edgep->toMTaskp();

        relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);

        अगर (relativep == recipientp || recipientp->hasEdgeTo(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } अन्यथा {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->addDependent(relativep);
            recipientp->stealRelativeEdge<GraphWay::FORWARD>(edgep);
            relativep->addRelativeEdge<GraphWay::REVERSE>(edgep);
        }
    }

    // Process incoming edges of donor
    जबतक (MTaskEdge* स्थिर edgep = static_cast<MTaskEdge*>(donorp->inEdges().frontp())) {
        LogicMTask* स्थिर relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);

        अगर (relativep == recipientp || relativep->hasEdgeTo(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } अन्यथा {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relink_शीर्ष(recipientp);
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
    स्थिर uint64_t newCpFwd = recipientp->cpExclusiveFromEdges<GraphWay::FORWARD>();
    स्थिर uint64_t newCpRev = recipientp->cpExclusiveFromEdges<GraphWay::REVERSE>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<GraphWay::FORWARD>(newCpFwd);
    propagate<GraphWay::FORWARD>(recipientp);
    recipientp->cpExclusive<GraphWay::REVERSE>(newCpRev);
    propagate<GraphWay::REVERSE>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(यह), donorp);
}

व्योम OrderMTaskGraph::removeTransitiveEdges() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    क्रम (V3GraphVertex& vtx : vertices()) {
        क्रम (V3GraphEdge* स्थिर graphEdgep : vtx.outEdges().unlinkable()) {
            MTaskEdge* स्थिर edgep = static_cast<MTaskEdge*>(graphEdgep);
            LogicMTask* स्थिर fromp = edgep->fromMTaskp();
            LogicMTask* स्थिर शीर्ष = edgep->toMTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            अगर (!pathExists(fromp, शीर्ष, edgep)) जारी;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(शीर्ष);
            fromp->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            शीर्ष->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

व्योम OrderMTaskGraph::removeEmptyMTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    क्रम (V3GraphVertex* स्थिर vtxp : vertices().unlinkable()) {
        LogicMTask* स्थिर mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        अगर (mtaskp == m_entryp || mtaskp == m_exitp) जारी;

        // Keep any MTask that holds logic
        bool empty = true;
        क्रम (स्थिर OrderMoveVertex& mVtx : mtaskp->vertexList()) {
            अगर (mVtx.logicp()) {
                empty = false;
                अवरोध;
            }
        }
        अगर (!empty) जारी;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->cost(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        क्रम (V3GraphEdge& inEdge : mtaskp->inEdges()) {
            LogicMTask* स्थिर fromp = static_cast<MTaskEdge&>(inEdge).fromMTaskp();
            क्रम (V3GraphEdge& outEdge : mtaskp->outEdges()) {
                LogicMTask* स्थिर शीर्ष = static_cast<MTaskEdge&>(outEdge).toMTaskp();
                अगर (!fromp->hasEdgeTo(शीर्ष)) addEdge(fromp, शीर्ष);
            }
        }

        // Remove incoming edges of 'mtaskp'
        जबतक (MTaskEdge* स्थिर edgep = static_cast<MTaskEdge*>(mtaskp->inEdges().frontp())) {
            LogicMTask* स्थिर relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        जबतक (MTaskEdge* स्थिर edgep = static_cast<MTaskEdge*>(mtaskp->outEdges().frontp())) {
            LogicMTask* स्थिर relativep = edgep->toMTaskp();
            relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(यह), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
ढाँचा <GraphWay::en N_Way>
व्योम OrderMTaskGraph::validateWay() स्थिर {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv = way.invert();
    क्रम (स्थिर V3GraphVertex& vtx : vertices()) {
        स्थिर LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cpCost = 0;
        std::unordered_set<स्थिर V3GraphVertex*> relatives;
        क्रम (स्थिर V3GraphEdge& graphEdge : mtask.edges<inv>()) {
            स्थिर MTaskEdge& edge = *graphEdge.as<MTaskEdge>();
            स्थिर LogicMTask& relative = *(edge.furtherp<inv>()->ढाँचा as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(edge.weight() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            स्थिर bool first = relatives.जोड़ो(&relative).second;
            UASSERT_OBJ(first, &mtask, "Should be no redundant edges in MTask graph");
            स्थिर uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(edge.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(edge.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cpCost = std::max(cpCost, inclusiveCp);
        }
        स्थिर uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cpCost, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cpExclusiveFromEdges<N_Way>() == cpCost, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

व्योम OrderMTaskGraph::validate() स्थिर {
    अगर (!m_slowAsserts) वापस;

    validateWay<GraphWay::FORWARD>();
    validateWay<GraphWay::REVERSE>();

    // Check the dependents set of each MTask agrees with its out-edges
    क्रम (स्थिर V3GraphVertex& vtx : vertices()) {
        स्थिर LogicMTask& mtask = *vtx.as<LogicMTask>();
        माप_प्रकार nDependents = 0;
        क्रम (स्थिर V3GraphEdge& graphEdge : mtask.outEdges()) {
            LogicMTask* स्थिर शीर्ष = graphEdge.as<MTaskEdge>()->toMTaskp();
            UASSERT_OBJ(mtask.hasEdgeTo(शीर्ष), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.आकार() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

श्रेणी OrderMTaskGraphBuilder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    स्थिर VNUser1InUse m_user1InUse;

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
    अटल bool bypassOk(OrderMoveVertex* mvtxp) {
        // Need to keep all logic vertices
        अगर (mvtxp->logicp()) वापस false;
        // Count fan-in, up to 3
        अचिन्हित fanIn = 0;
        स्वतः& inEdges = mvtxp->inEdges();
        क्रम (स्वतः it = inEdges.begin(); it != inEdges.end(); ++it) {
            अगर (++fanIn == 3) अवरोध;
        }
        // If fanIn no more than one, bypass
        अगर (fanIn <= 1) वापस true;
        // Count fan-out, up to 3
        अचिन्हित fanOut = 0;
        स्वतः& outEdges = mvtxp->outEdges();
        क्रम (स्वतः it = outEdges.begin(); it != outEdges.end(); ++it) {
            अगर (++fanOut == 3) अवरोध;
        }
        // If fan-out no more than one, bypass
        अगर (fanOut <= 1) वापस true;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        वापस fanIn + fanOut == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    व्योम addEdge(LogicMTask* srcp, LogicMTask* dstp) {
        अगर (srcp->hasEdgeTo(dstp)) वापस;  // Don't create redundant edges.
        m_mtaskGraph.addEdge(srcp, dstp);
    }

    // CONSTRUCTORS
    explicit OrderMTaskGraphBuilder(OrderMTaskGraph& mtaskGraph)
        : m_mtaskGraph{mtaskGraph} {

        // Create the LogicMTasks for each OrderMoveVertex
        क्रम (V3GraphVertex& vtx : mtaskGraph.moveGraph().vertices()) {
            OrderMoveVertex& mVtx = static_cast<OrderMoveVertex&>(vtx);
            अगर (bypassOk(&mVtx)) {
                mVtx.userp(nullptr);  // Set to nullptr to mark as bypassed
            } अन्यथा {
                mVtx.userp(नया LogicMTask{mtaskGraph, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& entry = *mtaskGraph.entryp();
        LogicMTask& निकास = *mtaskGraph.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        क्रम (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            अगर (VL_UNLIKELY((&mtask == &entry) || (&mtask == &निकास))) जारी;

            OrderMoveVertex::List& vertexList = mtask.vertexList();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(vertexList.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            OrderMoveVertex* स्थिर mVtxp = vertexList.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            क्रम (स्थिर V3GraphEdge& dEdge : mVtxp->outEdges()) {
                V3GraphVertex* स्थिर शीर्ष = dEdge.शीर्ष();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                अगर (LogicMTask* स्थिर otherp = static_cast<LogicMTask*>(शीर्ष->userp())) {
                    addEdge(&mtask, otherp);
                    जारी;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                क्रम (स्थिर V3GraphEdge& tEdge : शीर्ष->outEdges()) {
                    LogicMTask* स्थिर transp = static_cast<LogicMTask*>(tEdge.शीर्ष()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    addEdge(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        क्रम (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            अगर (VL_UNLIKELY((&mtask == &entry) || (&mtask == &निकास))) जारी;
            // Add the entry/exit edges if not otherwise connected
            अगर (mtask.inEmpty()) addEdge(&entry, &mtask);
            अगर (mtask.outEmpty()) addEdge(&mtask, &निकास);
        }
    }
    ~OrderMTaskGraphBuilder() = शेष;
    VL_UNCOPYABLE(OrderMTaskGraphBuilder);
    VL_UNMOVABLE(OrderMTaskGraphBuilder);

खुला:
    अटल व्योम apply(OrderMTaskGraph& mtaskGraph) { OrderMTaskGraphBuilder{mtaskGraph}; }
};

std::unique_ptr<OrderMTaskGraph> OrderMTaskGraph::build(OrderMoveGraph& moveGraph) {
    std::unique_ptr<OrderMTaskGraph> resp{नया OrderMTaskGraph{moveGraph}};
    OrderMTaskGraphBuilder::apply(*resp);
    resp->validate();
    वापस resp;
}
