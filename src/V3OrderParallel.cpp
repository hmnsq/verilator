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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3ExecGraph.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3GraphStream.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderCFuncEmitter.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderInternal.h"
#ଅନ୍ତର୍ଭାଵ "V3OrderMTaskGraph.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
ସ୍ଥିର ଶୂନ୍ୟ checkDataHazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    ଯଦି (!mTaskGraph.slowAsserts()) ଫେରନ୍ତୁ;

    // Order MTasks by their stable ids, so the report is deterministic
    ସଂରଚନା MTaskIdLessThan final {
        ତାର୍କିକ operator()(ଅଚଳ LogicMTask* ap, ଅଚଳ LogicMTask* bp) ଅଚଳ { ଫେରନ୍ତୁ *ap < *bp; }
    };
    ସଂରଚନା VarInfo final {
        ତାର୍କିକ m_seen = ମିଥ୍ୟା;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ଅଚଳ VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    ପାଇଁ (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        ପାଇଁ (ଅଚଳ OrderMoveVertex& mVtx : mtask.vertexList()) {
            ଅଚଳ OrderLogicVertex* ଅଚଳ lVtxp = mVtx.logicp();
            ଯଦି (!lVtxp) ଜାରି;  // A variable vertex, which performs no access itself
            ପାଇଁ (ଅଚଳ OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ଅଚଳ vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                ଯଦି (!varInfo.m_seen) {
                    varInfo.m_seen = ସତ୍ୟ;
                    vscps.ଠେଲ_back(vscp);
                }
                ଅଚଳ auto pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                ଯଦି (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    ପାଇଁ (AstVarScope* ଅଚଳ vscp : vscps) {
        ଅଚଳ auto& byMTask = varInfos(vscp).m_byMTask;
        ପାଇଁ (auto aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            ପାଇଁ (auto bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                ଯଦି (aIt->second.isReadOnly() && bIt->second.isReadOnly()) ଜାରି;
                LogicMTask* ଅଚଳ ap = aIt->first;
                LogicMTask* ଅଚଳ bp = bIt->first;
                ଯଦି (mTaskGraph.pathExists(ap, bp, nullptr)) ଜାରି;
                ଯଦି (mTaskGraph.pathExists(bp, ap, nullptr)) ଜାରି;
                // LCOV_EXCL_START
                ଯଦି (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    ଯଦି (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

ସ୍ଥିର std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
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
    ଯଦି (v3Global.opt.threadsCoarsen()) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ଅଚଳ uint64_t fudge_ଅଙ୍କ = 3;
        ଅଚଳ uint64_t fudgeDen = 5;
        ଅଚଳ uint64_t limit = (mTaskGraphp->totalCost() * fudge_ଅଙ୍କ) / (nThreads * fudgeDen);
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
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ଅଚଳ mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        ଯେତେବେଳେ (OrderMoveVertex* ଅଚଳ mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    ଫେରନ୍ତୁ mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

ଶ୍ରେଣୀ DpiThreadsVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_CCall* nodep) override { iterateConst(nodep->funcp()); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(DpiThreadsVisitor);

ସର୍ବସାଧାରଣ:
    // Number of threads occupied by the given MTask
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା apply(ଅଚଳ ExecMTask* mTaskp) {
        ଫେରନ୍ତୁ DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_ଗଣ୍ଠି_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, ଅଚଳ std::string& tag,
                                     ତାର୍କିକ slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ଅଚଳ std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    ଯଦି (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* ଅଚଳ mVtxp = vtxp->as<OrderMoveVertex>();
        ଯଦି (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    ପାଇଁ (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* ଅଚଳ mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* ଅଚଳ mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : mVtxp->outEdges().unlinkable()) {
            ଅଚଳ OrderMoveVertex* ଅଚଳ toMVtxp = edgep->ଶୀର୍ଷ()->as<OrderMoveVertex>();
            ଯଦି (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    ଯଦି (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_ଧାଡ଼ି* ଅଚଳ flp = v3Global.rootp()->fileline();
    AstScope* ଅଚଳ scopep = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->scopep();
    AstExecGraph* ଅଚଳ execGraphp = ନୂତନ AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* ଅଚଳ depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ଅଚଳ LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    ସଂରଚନା MTaskVxIdLessThan final {
        ତାର୍କିକ operator()(ଅଚଳ V3GraphVertex* lhsp, ଅଚଳ V3GraphVertex* rhsp) ଅଚଳ {
            ଫେରନ୍ତୁ lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    ଯେତେବେଳେ (ଅଚଳ V3GraphVertex* ଅଚଳ vtxp = mtaskStream.nextp()) {
        ଅଚଳ LogicMTask* ଅଚଳ cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ଅଚଳ mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        ଯଦି (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            ଜାରି;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        ଯେତେବେଳେ (OrderMoveVertex* ଅଚଳ mVtxp = mTaskp->vertexList().unlinkFront()) {
            ଯଦି (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        ଯେତେବେଳେ (OrderMoveVertex* ଅଚଳ mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            ଯଦି (OrderLogicVertex* ଅଚଳ logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* ଅଚଳ domScopep = &mVtxp->domScope();
                ଯଦି (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ଅଚଳ execMTaskp = ନୂତନ ExecMTask{execGraphp, scopep, emitter.getStmts()};
        ଯଦି (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        ଅଚଳ ତାର୍କିକ newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        ପାଇଁ (ଅଚଳ V3GraphEdge& edge : mTaskp->inEdges()) {
            ଅଚଳ V3GraphVertex* fromVxp = edge.fromp();
            ଅଚଳ LogicMTask* ଅଚଳ fromp = fromVxp->as<ଅଚଳ LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            ଯଦି (fromp == mTaskGraphp->entryp()) ଜାରି;
            ନୂତନ V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : moveGraph.vertices().unlinkable()) {
        ଯଦି (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    ଫେରନ୍ତୁ execGraphp;
}
