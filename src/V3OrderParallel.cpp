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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3ExecGraph.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3GraphStream.h"
#અન્તર્ભાવ "V3OrderCFuncEmitter.h"
#અન્તર્ભાવ "V3OrderInternal.h"
#અન્તર્ભાવ "V3OrderMTaskGraph.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
સ્થિર શૂન્ય checkDataHazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    જો (!mTaskGraph.slowAsserts()) પાછા;

    // Order MTasks by their stable ids, so the report is deterministic
    સંરચના MTaskIdLessThan final {
        તાર્કિક operator()(અચલ LogicMTask* ap, અચલ LogicMTask* bp) અચલ { પાછા *ap < *bp; }
    };
    સંરચના VarInfo final {
        તાર્કિક m_seen = ખોટું;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    અચલ VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    માટે (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        માટે (અચલ OrderMoveVertex& mVtx : mtask.vertexList()) {
            અચલ OrderLogicVertex* અચલ lVtxp = mVtx.logicp();
            જો (!lVtxp) ચાલુ;  // A variable vertex, which performs no access itself
            માટે (અચલ OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* અચલ vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                જો (!varInfo.m_seen) {
                    varInfo.m_seen = સાચું;
                    vscps.ધકેલો_back(vscp);
                }
                અચલ auto pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                જો (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    માટે (AstVarScope* અચલ vscp : vscps) {
        અચલ auto& byMTask = varInfos(vscp).m_byMTask;
        માટે (auto aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            માટે (auto bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                જો (aIt->second.isReadOnly() && bIt->second.isReadOnly()) ચાલુ;
                LogicMTask* અચલ ap = aIt->first;
                LogicMTask* અચલ bp = bIt->first;
                જો (mTaskGraph.pathExists(ap, bp, nullptr)) ચાલુ;
                જો (mTaskGraph.pathExists(bp, ap, nullptr)) ચાલુ;
                // LCOV_EXCL_START
                જો (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    જો (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

સ્થિર std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
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
    જો (v3Global.opt.threadsCoarsen()) {
        અચલ પૂર્ણાંક nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        અચલ uint64_t fudge_અંક = 3;
        અચલ uint64_t fudgeDen = 5;
        અચલ uint64_t limit = (mTaskGraphp->totalCost() * fudge_અંક) / (nThreads * fudgeDen);
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
    માટે (V3GraphVertex* અચલ vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* અચલ mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        જ્યારે (OrderMoveVertex* અચલ mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    પાછા mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

વર્ગ DpiThreadsVisitor final : જાહેર VNVisitorConst {
    પૂર્ણાંક m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    શૂન્ય visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override { iterateConst(nodep->funcp()); }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = મૂળભૂત;
    VL_UNCOPYABLE(DpiThreadsVisitor);

જાહેર:
    // Number of threads occupied by the given MTask
    સ્થિર પૂર્ણાંક apply(અચલ ExecMTask* mTaskp) {
        પાછા DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_ગાંઠ_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, અચલ std::string& tag,
                                     તાર્કિક slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    અચલ std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    જો (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    માટે (V3GraphVertex* અચલ vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* અચલ mVtxp = vtxp->as<OrderMoveVertex>();
        જો (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    માટે (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* અચલ mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* અચલ mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        માટે (V3GraphEdge* અચલ edgep : mVtxp->outEdges().unlinkable()) {
            અચલ OrderMoveVertex* અચલ toMVtxp = edgep->ટોચ()->as<OrderMoveVertex>();
            જો (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    જો (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_લીટી* અચલ flp = v3Global.rootp()->fileline();
    AstScope* અચલ scopep = v3Global.rootp()->ટોચ_Scopep()->scopep();
    AstExecGraph* અચલ execGraphp = નવું AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* અચલ depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<અચલ LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    સંરચના MTaskVxIdLessThan final {
        તાર્કિક operator()(અચલ V3GraphVertex* lhsp, અચલ V3GraphVertex* rhsp) અચલ {
            પાછા lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    જ્યારે (અચલ V3GraphVertex* અચલ vtxp = mtaskStream.nextp()) {
        અચલ LogicMTask* અચલ cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* અચલ mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        જો (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            ચાલુ;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        જ્યારે (OrderMoveVertex* અચલ mVtxp = mTaskp->vertexList().unlinkFront()) {
            જો (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        જ્યારે (OrderMoveVertex* અચલ mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            જો (OrderLogicVertex* અચલ logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* અચલ domScopep = &mVtxp->domScope();
                જો (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* અચલ execMTaskp = નવું ExecMTask{execGraphp, scopep, emitter.getStmts()};
        જો (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        અચલ તાર્કિક newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* અચલ callp = નવું AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        માટે (અચલ V3GraphEdge& edge : mTaskp->inEdges()) {
            અચલ V3GraphVertex* fromVxp = edge.fromp();
            અચલ LogicMTask* અચલ fromp = fromVxp->as<અચલ LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            જો (fromp == mTaskGraphp->entryp()) ચાલુ;
            નવું V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    માટે (V3GraphVertex* અચલ vtxp : moveGraph.vertices().unlinkable()) {
        જો (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    પાછા execGraphp;
}
