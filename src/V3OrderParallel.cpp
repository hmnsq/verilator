// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Multi-threaded code partitioning and ordering
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
//
//  Parallel code ordering
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3GraphStream.h"
#ಅನ್ತರ್ಭಾವ "V3OrderCFuncEmitter.h"
#ಅನ್ತರ್ಭಾವ "V3OrderInternal.h"
#ಅನ್ತರ್ಭಾವ "V3OrderMTaskGraph.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
ಸ್ಥಿರ ಶೂನ್ಯ checkDataHazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    ಯದಿ (!mTaskGraph.slowAsserts()) ಪ್ರತ್ಯಾಗಮ;

    // Order MTasks by their stable ids, so the report is deterministic
    ಸಂರಚನಾ MTaskIdLessThan final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ LogicMTask* ap, ಧ್ರುವ LogicMTask* bp) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ *ap < *bp; }
    };
    ಸಂರಚನಾ VarInfo final {
        ತಾರ್ಕಿಕ m_seen = ಅಸತ್ಯ;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ಧ್ರುವ VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    ಪ್ರತಿ (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        ಪ್ರತಿ (ಧ್ರುವ OrderMoveVertex& mVtx : mtask.vertexList()) {
            ಧ್ರುವ OrderLogicVertex* ಧ್ರುವ lVtxp = mVtx.logicp();
            ಯದಿ (!lVtxp) ಅನುವರ್ತ;  // A variable vertex, which performs no access itself
            ಪ್ರತಿ (ಧ್ರುವ OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ಧ್ರುವ vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                ಯದಿ (!varInfo.m_seen) {
                    varInfo.m_seen = ಸತ್ಯ;
                    vscps.ತಳ್ಳು_back(vscp);
                }
                ಧ್ರುವ auto pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                ಯದಿ (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : vscps) {
        ಧ್ರುವ auto& byMTask = varInfos(vscp).m_byMTask;
        ಪ್ರತಿ (auto aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            ಪ್ರತಿ (auto bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                ಯದಿ (aIt->second.isReadOnly() && bIt->second.isReadOnly()) ಅನುವರ್ತ;
                LogicMTask* ಧ್ರುವ ap = aIt->first;
                LogicMTask* ಧ್ರುವ bp = bIt->first;
                ಯದಿ (mTaskGraph.pathExists(ap, bp, nullptr)) ಅನುವರ್ತ;
                ಯದಿ (mTaskGraph.pathExists(bp, ap, nullptr)) ಅನುವರ್ತ;
                // LCOV_EXCL_START
                ಯದಿ (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    ಯದಿ (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

ಸ್ಥಿರ std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
    // Build the initial MTask graph. Initially, each MTask just wraps one OrderMoveVertex. We will
    // merge MTasks together and eventually each MTask will wrap a large number of OrderMoveVertex
    // (and the logic nodes therein).
    std::unique_ptr<OrderMTaskGraph> mTaskGraphp = OrderMTaskGraph::build(moveGraph);
    mTaskGraphp->hashGraphDebug("initial MTask graph");

    // Add edges to eliminate data hazards
    OrderMTaskGraph::fixDataHazards(*mTaskGraphp);
    mTaskGraphp->hashGraphDebug("MTask graph after fixDataHazards()");

    // Merge MTask nodes together, repeatedly, until the critical path budget is reached. Coarsens
    // the graph, usually by several orders of magnitude. Some tests disable this for stability,
    // it should always be enabled in production.
    ಯದಿ (v3Global.opt.threadsCoarsen()) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ಧ್ರುವ uint64_t fudge_ಅಂಕೆ = 3;
        ಧ್ರುವ uint64_t fudgeDen = 5;
        ಧ್ರುವ uint64_t limit = (mTaskGraphp->totalCost() * fudge_ಅಂಕೆ) / (nThreads * fudgeDen);
        UINFO(4, "Partitioner set critical path limit = " << limit);

        OrderMTaskGraph::contract(*mTaskGraphp, limit);
        mTaskGraphp->hashGraphDebug("MTask graph after contract()");
    }

    // Remove MTasks that have no logic in them, rerouting the edges
    mTaskGraphp->removeEmptyMTasks();
    mTaskGraphp->hashGraphDebug("MTask graph after removeEmptyMTasks()");

    // Note this is OrderMTaskGraph::removeTransitiveEdges, which maintains graph consistency
    mTaskGraphp->removeTransitiveEdges();
    mTaskGraphp->hashGraphDebug("MTask graph after removeTransitiveEdges()");

    // Check for data hazards the partitioning left unordered
    checkDataHazards(*mTaskGraphp);

    // Set OrderMoveVertex::userp to indicate the mtask it is part of.
    moveGraph.userClearVertices();
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ಧ್ರುವ mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        ಯಾವತ್ (OrderMoveVertex* ಧ್ರುವ mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    ಪ್ರತ್ಯಾಗಮ mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

ವರ್ಗ DpiThreadsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    ಪೂರ್ಣಾಙ್ಕ m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_CCall* nodep) override { iterateConst(nodep->funcp()); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(DpiThreadsVisitor);

ಸಾರ್ವಜನಿಕ:
    // Number of threads occupied by the given MTask
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ apply(ಧ್ರುವ ExecMTask* mTaskp) {
        ಪ್ರತ್ಯಾಗಮ DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_ಗಂಟು_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, ಧ್ರುವ std::string& tag,
                                     ತಾರ್ಕಿಕ slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ಧ್ರುವ std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    ಯದಿ (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* ಧ್ರುವ mVtxp = vtxp->as<OrderMoveVertex>();
        ಯದಿ (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    ಪ್ರತಿ (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* ಧ್ರುವ mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* ಧ್ರುವ mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : mVtxp->outEdges().unlinkable()) {
            ಧ್ರುವ OrderMoveVertex* ಧ್ರುವ toMVtxp = edgep->ತುದಿ()->as<OrderMoveVertex>();
            ಯದಿ (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    ಯದಿ (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_ಸಾಲು* ಧ್ರುವ flp = v3Global.rootp()->fileline();
    AstScope* ಧ್ರುವ scopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();
    AstExecGraph* ಧ್ರುವ execGraphp = ನೂತನ AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* ಧ್ರುವ depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ಧ್ರುವ LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    ಸಂರಚನಾ MTaskVxIdLessThan final {
        ತಾರ್ಕಿಕ operator()(ಧ್ರುವ V3GraphVertex* lhsp, ಧ್ರುವ V3GraphVertex* rhsp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    ಯಾವತ್ (ಧ್ರುವ V3GraphVertex* ಧ್ರುವ vtxp = mtaskStream.nextp()) {
        ಧ್ರುವ LogicMTask* ಧ್ರುವ cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ಧ್ರುವ mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        ಯದಿ (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            ಅನುವರ್ತ;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        ಯಾವತ್ (OrderMoveVertex* ಧ್ರುವ mVtxp = mTaskp->vertexList().unlinkFront()) {
            ಯದಿ (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        ಯಾವತ್ (OrderMoveVertex* ಧ್ರುವ mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            ಯದಿ (OrderLogicVertex* ಧ್ರುವ logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* ಧ್ರುವ domScopep = &mVtxp->domScope();
                ಯದಿ (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ಧ್ರುವ execMTaskp = ನೂತನ ExecMTask{execGraphp, scopep, emitter.getStmts()};
        ಯದಿ (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : mTaskp->inEdges()) {
            ಧ್ರುವ V3GraphVertex* fromVxp = edge.fromp();
            ಧ್ರುವ LogicMTask* ಧ್ರುವ fromp = fromVxp->as<ಧ್ರುವ LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            ಯದಿ (fromp == mTaskGraphp->entryp()) ಅನುವರ್ತ;
            ನೂತನ V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : moveGraph.vertices().unlinkable()) {
        ಯದಿ (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    ಪ್ರತ್ಯಾಗಮ execGraphp;
}
