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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3OrderMTaskGraph.h"

#ضمن "V3Global.h"
#ضمن "V3InstrCount.h"

#ضمن <algorithm>
#ضمن <memory>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// LogicMTask

uint32_t LogicMTask::s_nextId = 1;  // Start at 1, for historic reasons

LogicMTask::LogicMTask(OrderMTaskGraph& graph, OrderMoveVertex* mVtxp)
    : V3GraphVertex{&graph} {
    UASSERT(s_nextId < 0xFFFFFFFFUL, "Too many LogicMTask instances");
    𐤀𐤌 (!mVtxp) أرجع;
    m_mVertices.linkBack(mVtxp);
    𐤀𐤌 (ثابت OrderLogicVertex* ثابت olvp = mVtxp->logicp()) {
        m_cost += V3InstrCount::count(olvp->nodep(), true);
    }
}

//######################################################################
// OrderMTaskGraph

OrderMTaskGraph::OrderMTaskGraph(OrderMoveGraph& moveGraph)
    : m_moveGraph{moveGraph}
    , m_entryp{جديد LogicMTask{*هذا, nullptr}}
    , m_exitp{جديد LogicMTask{*هذا, nullptr}}
    , m_slowAsserts{v3Global.opt.debugPartition()} {}

منطقي OrderMTaskGraph::pathExistsImpl(LogicMTask* fromp, LogicMTask* top,
                                     ثابت MTaskEdge* excludedEdgep) {
    UDEBUGONLY(UASSERT_OBJ(fromp->m_generation != m_currentGeneration, fromp,
                           "Should not visit an MTask twice in the same search"););
    // Mark visited.
    fromp->m_generation = m_currentGeneration;

    // Base case: we found a path.
    𐤀𐤌 (fromp == top) أرجع true;

    // Base case: fromp is too late, cannot possibly be a prereq for top.
    𐤀𐤌 (fromp->cpExclusive<GraphWay::REVERSE>() < top->cpInclusive<GraphWay::REVERSE>()) {
        أرجع خطأ;
    }
    𐤀𐤌 (fromp->cpInclusive<GraphWay::FORWARD>() > top->cpExclusive<GraphWay::FORWARD>()) {
        أرجع خطأ;
    }

    // Recursively look for a path
    لكل (ثابت V3GraphEdge& follow : fromp->outEdges()) {
        𐤀𐤌 (&follow == excludedEdgep) استمر;
        LogicMTask* ثابت nextp = static_cast<LogicMTask*>(follow.top());
        // Don't visit the same MTask twice in the same search.
        𐤀𐤌 (nextp->m_generation == m_currentGeneration) استمر;
        𐤀𐤌 (pathExistsImpl(nextp, top, nullptr)) أرجع true;
    }
    أرجع خطأ;
}

template <GraphWay::en N_Way>
فراغ OrderMTaskGraph::propagatePush(LogicMTask* mtaskp) {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};
    ثابت uint64_t inclusiveCp = mtaskp->cpInclusive<way>();

    لكل (V3GraphEdge& graphEdge : mtaskp->edges<way>()) {
        MTaskEdge& edge = static_cast<MTaskEdge&>(graphEdge);

        LogicMTask* ثابت relativep = edge.furtherMTaskp<N_Way>();
        EdgeHeap::Node& edgeHeapNode = edge.m_edgeHeapNode[inv];
        𐤀𐤌 (inclusiveCp > edgeHeapNode.key().m_cp) {
            relativep->m_edgeHeap[inv].increaseKey(&edgeHeapNode, inclusiveCp);
        }

        ثابت uint64_t relativeCp = relativep->cpExclusive<way>();

        𐤀𐤌 (relativeCp >= inclusiveCp) استمر;

        // relativep's critical path is out of step with its longest !wayward edge.
        // Schedule that to be resolved.
        ثابت uint64_t increment = inclusiveCp - relativeCp;

        PropagatePendingHeap::Node*& pendingNodepRef = relativep->m_propagateHeapNodep;
        𐤀𐤌 (PropagatePendingHeap::Node* ثابت nodep = pendingNodepRef) {
            // Already in heap. Increase the increment if needed.
            𐤀𐤌 (increment > nodep->key().m_increment) {
                m_pendingHeap.increaseKey(nodep, increment);
            }
            استمر;
        }

        // Add to heap
        PropagatePendingHeap::Node* ثابت nodep = m_pendingNodePool.alloc();
        pendingNodepRef = nodep;
        m_pendingHeap.insert(nodep, {increment, relativep->id(), relativep});
    }
}

template <GraphWay::en N_Way>
فراغ OrderMTaskGraph::propagateResolve() {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv{way.invert()};

    // Each pending MTask is keyed on how much its critical path will grow by. Resolving them in
    // decreasing order of that growth means each MTask needs resolving only once: the growth of a
    // wayward MTask is never larger than the growth of the MTask it was pushed from, so once an
    // MTask has been resolved, no larger growth can be pushed onto it later.
    طالما (!m_pendingHeap.empty()) {
        // Pop max element from heap
        PropagatePendingHeap::Node* ثابت maxp = m_pendingHeap.max();
        m_pendingHeap.remove(maxp);
        // Pick up values
        LogicMTask* ثابت mtaskp = maxp->key().m_mtaskp;
        ثابت uint64_t cpGrowBy = maxp->key().m_increment;
        // Confirm that we only set each node's CP once. That's an important property of this
        // algorithm, which allows it to be far faster than a recursive one.
        UASSERT_OBJ(mtaskp->m_generation != m_currentGeneration, mtaskp, "Set CP on node twice");
        mtaskp->m_generation = m_currentGeneration;
        // Free the heap node, we are done with it
        m_pendingNodePool.حرر(maxp);
        mtaskp->m_propagateHeapNodep = nullptr;
        // Update the critical path of mtaskp, that was out-of-date with respect to its edges
        uint64_t& cpRef = mtaskp->m_cpExclusive[way];
        ثابت uint64_t newCp = cpRef + cpGrowBy;
        // Check that CP matches that of the longest edge wayward of mtaskp.
        𐤀𐤌 (VL_UNLIKELY(m_slowAsserts)) {
            ثابت uint64_t edgeCp = mtaskp->m_edgeHeap[inv].max()->key().m_cp;
            UASSERT_OBJ(edgeCp == newCp, mtaskp, "CP doesn't match longest wayward edge");
        }
        cpRef = newCp;
        propagatePush<N_Way>(mtaskp);
    }
}

uint64_t OrderMTaskGraph::totalCost() ثابت {
    uint64_t cost = 0;
    لكل (ثابت V3GraphVertex& vtx : vertices()) cost += static_cast<ثابت LogicMTask&>(vtx).cost();
    أرجع cost;
}

فراغ OrderMTaskGraph::addEdge(LogicMTask* fromp, LogicMTask* top) {
    UASSERT_OBJ(fromp != top, fromp, "Should not create self-edges");
    UDEBUGONLY(UASSERT_OBJ(!fromp->hasEdgeTo(top), fromp, "Should not create redundant edges"););

    // Create the edge. This inserts it into the edge heap of both endpoints with the correct
    // critical path keys, as the critical paths of the endpoints are still unchanged here.
    جديد MTaskEdge{هذا, fromp, top};

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
    𐤀𐤌 (fromp->cpInclusive<GraphWay::FORWARD>() > top->cpExclusive<GraphWay::FORWARD>()) {
        propagate<GraphWay::FORWARD>(fromp);
    }
    𐤀𐤌 (top->cpInclusive<GraphWay::REVERSE>() > fromp->cpExclusive<GraphWay::REVERSE>()) {
        propagate<GraphWay::REVERSE>(top);
    }
}

فراغ OrderMTaskGraph::mergeMTasks(LogicMTask* recipientp, LogicMTask* donorp) {
    UASSERT_OBJ(recipientp != donorp, recipientp, "Should not merge an MTask with itself");

    // Note we redirect the edges before updating the cost and critical paths of the recipient,
    // which means the redirected edges are inserted into the edge heaps of the relatives using the
    // pre-merge values of the recipient. The critical path propagation below then brings all of
    // them up to date. This works because the keys in the edge heaps only ever need increasing:
    // the inclusive critical path of the merged MTask is at least the inclusive critical path of
    // either of the two MTasks it is made of, in both directions.

    // Process outgoing edges of donor
    طالما (MTaskEdge* ثابت edgep = static_cast<MTaskEdge*>(donorp->outEdges().frontp())) {
        LogicMTask* ثابت relativep = edgep->toMTaskp();

        relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);

        𐤀𐤌 (relativep == recipientp || recipientp->hasEdgeTo(relativep)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } وإلا {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from donor -> relative to recipient -> relative.
            edgep->relinkFromp(recipientp);
            recipientp->addDependent(relativep);
            recipientp->stealRelativeEdge<GraphWay::FORWARD>(edgep);
            relativep->addRelativeEdge<GraphWay::REVERSE>(edgep);
        }
    }

    // Process incoming edges of donor
    طالما (MTaskEdge* ثابت edgep = static_cast<MTaskEdge*>(donorp->inEdges().frontp())) {
        LogicMTask* ثابت relativep = edgep->fromMTaskp();

        relativep->removeDependent(donorp);
        relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);

        𐤀𐤌 (relativep == recipientp || relativep->hasEdgeTo(recipientp)) {
            // This is either the edge connecting the two MTasks, which becomes internal to the
            // merged MTask, or is parallel with an existing edge of the recipient. Drop it.
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        } وإلا {
            // No existing edge between recipient and relative of donor.
            // Redirect the edge from relative -> donor to relative -> recipient.
            edgep->relinkTop(recipientp);
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
    ثابت uint64_t newCpFwd = recipientp->cpExclusiveFromEdges<GraphWay::FORWARD>();
    ثابت uint64_t newCpRev = recipientp->cpExclusiveFromEdges<GraphWay::REVERSE>();

    // Set the new critical paths, then propagate the increases to the relatives. Note this also
    // brings the keys of all edges of the merged MTask up to date in the relatives' edge heaps.
    recipientp->cpExclusive<GraphWay::FORWARD>(newCpFwd);
    propagate<GraphWay::FORWARD>(recipientp);
    recipientp->cpExclusive<GraphWay::REVERSE>(newCpRev);
    propagate<GraphWay::REVERSE>(recipientp);

    // Remove the donor from the graph
    VL_DO_DANGLING(donorp->unlinkDelete(هذا), donorp);
}

فراغ OrderMTaskGraph::removeTransitiveEdges() {
    // Removing a transitive edge cannot change any critical path, so none need updating here.
    // Only the edge heaps and the dependent sets need maintaining.
    لكل (V3GraphVertex& vtx : vertices()) {
        لكل (V3GraphEdge* ثابت graphEdgep : vtx.outEdges().unlinkable()) {
            MTaskEdge* ثابت edgep = static_cast<MTaskEdge*>(graphEdgep);
            LogicMTask* ثابت fromp = edgep->fromMTaskp();
            LogicMTask* ثابت top = edgep->toMTaskp();
            // If the MTasks are also connected by some other path, then this is a transitive edge
            𐤀𐤌 (!pathExists(fromp, top, edgep)) استمر;
            // Maintain the additional data structures of the OrderMTaskGraph
            fromp->removeDependent(top);
            fromp->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            top->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    // Confirm the above left the maintained state consistent
    validate();
}

فراغ OrderMTaskGraph::removeEmptyMTasks() {
    // This transform preserves the critical paths as it connects every predecessor of the
    // removed MTask to every successor, and the removed MTask itself has zero cost.
    لكل (V3GraphVertex* ثابت vtxp : vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(vtxp);

        // Keep the entry and exit vertices.
        𐤀𐤌 (mtaskp == m_entryp || mtaskp == m_exitp) استمر;

        // Keep any MTask that holds logic
        منطقي empty = true;
        لكل (ثابت OrderMoveVertex& mVtx : mtaskp->vertexList()) {
            𐤀𐤌 (mVtx.logicp()) {
                empty = خطأ;
                اكسر;
            }
        }
        𐤀𐤌 (!empty) استمر;

        // The MTask holding no logic should have zero cost
        UASSERT_OBJ(!mtaskp->cost(), mtaskp, "MTask holding no logic should have 0 cost");

        // Connect each predecessor directly to each successor.
        لكل (V3GraphEdge& inEdge : mtaskp->inEdges()) {
            LogicMTask* ثابت fromp = static_cast<MTaskEdge&>(inEdge).fromMTaskp();
            لكل (V3GraphEdge& outEdge : mtaskp->outEdges()) {
                LogicMTask* ثابت top = static_cast<MTaskEdge&>(outEdge).toMTaskp();
                𐤀𐤌 (!fromp->hasEdgeTo(top)) addEdge(fromp, top);
            }
        }

        // Remove incoming edges of 'mtaskp'
        طالما (MTaskEdge* ثابت edgep = static_cast<MTaskEdge*>(mtaskp->inEdges().frontp())) {
            LogicMTask* ثابت relativep = edgep->fromMTaskp();
            relativep->removeDependent(mtaskp);
            relativep->removeRelativeEdge<GraphWay::FORWARD>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Remove outgoing edges of 'mtaskp'
        طالما (MTaskEdge* ثابت edgep = static_cast<MTaskEdge*>(mtaskp->outEdges().frontp())) {
            LogicMTask* ثابت relativep = edgep->toMTaskp();
            relativep->removeRelativeEdge<GraphWay::REVERSE>(edgep);
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        // Delete the empty MTask
        VL_DO_DANGLING(mtaskp->unlinkDelete(هذا), mtaskp);
    }
    // Confirm the above left the maintained state consistent
    validate();
}

// Check the critical paths in the given direction, and the critical paths cached in the edge heaps
// in the opposite direction, against those implied by the edges. Note this deliberately iterates
// the edge lists, rather than consulting the edge heaps, so the heaps are validated, not trusted.
template <GraphWay::en N_Way>
فراغ OrderMTaskGraph::validateWay() ثابت {
    constexpr GraphWay way{N_Way};
    constexpr GraphWay inv = way.invert();
    لكل (ثابت V3GraphVertex& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        uint64_t cpCost = 0;
        std::unordered_set<ثابت V3GraphVertex*> relatives;
        لكل (ثابت V3GraphEdge& graphEdge : mtask.edges<inv>()) {
            ثابت MTaskEdge& edge = *graphEdge.as<MTaskEdge>();
            ثابت LogicMTask& relative = *(edge.furtherp<inv>()->template as<LogicMTask>());
            // Run a few asserts on the graph, while we are iterating through...
            UASSERT_OBJ(edge.weight() != 0, &mtask, "Should be no cut edges in MTask graph");
            UASSERT_OBJ(&relative != &mtask, &mtask, "Should be no self edges in MTask graph");
            ثابت منطقي first = relatives.insert(&relative).second;
            UASSERT_OBJ(first, &mtask, "Should be no redundant edges in MTask graph");
            ثابت uint64_t inclusiveCp = relative.cpInclusive<way>();
            // The critical path cached in the edge heap must match that of the relative
            UASSERT_OBJ(edge.cachedCp(inv) == inclusiveCp, &mtask,
                        "Cached critical path does not match the relative");
            // As must the ID it is keyed on, which breaks ties between equal critical paths
            UASSERT_OBJ(edge.cachedId(inv) == relative.id(), &mtask,
                        "Cached ID does not match the relative");
            cpCost = std::max(cpCost, inclusiveCp);
        }
        ثابت uint64_t cp = mtask.cpExclusive<way>();
        UASSERT_OBJ(cp == cpCost, &mtask, "Critical path does not match the edges");
        // The edge heap must yield the same, that is: it must return the largest of its keys
        UASSERT_OBJ(mtask.cpExclusiveFromEdges<N_Way>() == cpCost, &mtask,
                    "Edge heap maximum does not match the edges");
    }
}

فراغ OrderMTaskGraph::validate() ثابت {
    𐤀𐤌 (!m_slowAsserts) أرجع;

    validateWay<GraphWay::FORWARD>();
    validateWay<GraphWay::REVERSE>();

    // Check the dependents set of each MTask agrees with its out-edges
    لكل (ثابت V3GraphVertex& vtx : vertices()) {
        ثابت LogicMTask& mtask = *vtx.as<LogicMTask>();
        size_t nDependents = 0;
        لكل (ثابت V3GraphEdge& graphEdge : mtask.outEdges()) {
            LogicMTask* ثابت top = graphEdge.as<MTaskEdge>()->toMTaskp();
            UASSERT_OBJ(mtask.hasEdgeTo(top), &mtask, "Dependent missing from the dependents set");
            ++nDependents;
        }
        UASSERT_OBJ(mtask.m_dependents.size() == nDependents, &mtask,
                    "Stale entry in the dependents set");
    }
}

//######################################################################
// OrderMTaskGraphBuilder

صنف OrderMTaskGraphBuilder final {
    // NODE STATE
    // Used by V3InstrCount::count within the LogicMTask constructor only
    ثابت VNUser1InUse m_user1InUse;

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
    ساكن منطقي bypassOk(OrderMoveVertex* mvtxp) {
        // Need to keep all logic vertices
        𐤀𐤌 (mvtxp->logicp()) أرجع خطأ;
        // Count fan-in, up to 3
        unsigned fanIn = 0;
        auto& inEdges = mvtxp->inEdges();
        لكل (auto it = inEdges.begin(); it != inEdges.end(); ++it) {
            𐤀𐤌 (++fanIn == 3) اكسر;
        }
        // If fanIn no more than one, bypass
        𐤀𐤌 (fanIn <= 1) أرجع true;
        // Count fan-out, up to 3
        unsigned fanOut = 0;
        auto& outEdges = mvtxp->outEdges();
        لكل (auto it = outEdges.begin(); it != outEdges.end(); ++it) {
            𐤀𐤌 (++fanOut == 3) اكسر;
        }
        // If fan-out no more than one, bypass
        𐤀𐤌 (fanOut <= 1) أرجع true;
        // They can only be (2, 2), (2, 3), (3, 2), (3, 3) at this point, bypass if (2, 2)
        أرجع fanIn + fanOut == 4;
    }

    // Add an edge to the graph, if there is not already an edge between the two vertices.
    فراغ addEdge(LogicMTask* srcp, LogicMTask* dstp) {
        𐤀𐤌 (srcp->hasEdgeTo(dstp)) أرجع;  // Don't create redundant edges.
        m_mtaskGraph.addEdge(srcp, dstp);
    }

    // CONSTRUCTORS
    explicit OrderMTaskGraphBuilder(OrderMTaskGraph& mtaskGraph)
        : m_mtaskGraph{mtaskGraph} {

        // Create the LogicMTasks for each OrderMoveVertex
        لكل (V3GraphVertex& vtx : mtaskGraph.moveGraph().vertices()) {
            OrderMoveVertex& mVtx = static_cast<OrderMoveVertex&>(vtx);
            𐤀𐤌 (bypassOk(&mVtx)) {
                mVtx.userp(nullptr);  // Set to nullptr to mark as bypassed
            } وإلا {
                mVtx.userp(جديد LogicMTask{mtaskGraph, &mVtx});  // Create vertex and set userp
            }
        }

        LogicMTask& entry = *mtaskGraph.entryp();
        LogicMTask& exit = *mtaskGraph.exitp();

        // Create the MTask dependency edges based on the OrderMoveGraph dependencies
        لكل (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);

            // Entry and exit vertices handled separately
            𐤀𐤌 (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) استمر;

            OrderMoveVertex::List& vertexList = mtask.vertexList();
            // At this point, there should only be one OrderMoveVertex per LogicMTask
            UASSERT_OBJ(vertexList.hasSingleElement(), &mtask, "Multiple OrderMoveVertex");
            OrderMoveVertex* ثابت mVtxp = vertexList.frontp();
            UASSERT_OBJ(mVtxp->userp(), &mtask, "Bypassed OrderMoveVertex should not have MTask");

            // Iterate downstream direct dependents
            لكل (ثابت V3GraphEdge& dEdge : mVtxp->outEdges()) {
                V3GraphVertex* ثابت top = dEdge.top();

                // If the opposite end of the edge is not a bypassed vertex, add direct dependency
                𐤀𐤌 (LogicMTask* ثابت otherp = static_cast<LogicMTask*>(top->userp())) {
                    addEdge(&mtask, otherp);
                    استمر;
                }

                // The opposite end of the edge is a bypassed vertex, add transitive dependencies
                لكل (ثابت V3GraphEdge& tEdge : top->outEdges()) {
                    LogicMTask* ثابت transp = static_cast<LogicMTask*>(tEdge.top()->userp());
                    // The Move graph is bipartite (logic <-> var), and logic is never
                    // bypassed, hence 'transp' must be non-nullptr.
                    UASSERT_OBJ(transp, mVtxp, "This cannot be a bypassed vertex");
                    addEdge(&mtask, transp);
                }
            }
        }

        // Create Dependencies to/from the entry/exit vertices, so all vertices are
        // reachable from the entry point and flow to the exit point.
        لكل (V3GraphVertex& vtx : mtaskGraph.vertices()) {
            LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
            𐤀𐤌 (VL_UNLIKELY((&mtask == &entry) || (&mtask == &exit))) استمر;
            // Add the entry/exit edges if not otherwise connected
            𐤀𐤌 (mtask.inEmpty()) addEdge(&entry, &mtask);
            𐤀𐤌 (mtask.outEmpty()) addEdge(&mtask, &exit);
        }
    }
    ~OrderMTaskGraphBuilder() = افتراضي;
    VL_UNCOPYABLE(OrderMTaskGraphBuilder);
    VL_UNMOVABLE(OrderMTaskGraphBuilder);

عام:
    ساكن فراغ apply(OrderMTaskGraph& mtaskGraph) { OrderMTaskGraphBuilder{mtaskGraph}; }
};

std::unique_ptr<OrderMTaskGraph> OrderMTaskGraph::build(OrderMoveGraph& moveGraph) {
    std::unique_ptr<OrderMTaskGraph> resp{جديد OrderMTaskGraph{moveGraph}};
    OrderMTaskGraphBuilder::apply(*resp);
    resp->validate();
    أرجع resp;
}
