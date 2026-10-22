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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Ast.h"
#ضمن "V3AstUserAllocator.h"
#ضمن "V3Control.h"
#ضمن "V3Error.h"
#ضمن "V3ExecGraph.h"
#ضمن "V3Graph.h"
#ضمن "V3GraphStream.h"
#ضمن "V3OrderCFuncEmitter.h"
#ضمن "V3OrderInternal.h"
#ضمن "V3OrderMTaskGraph.h"

#ضمن <map>
#ضمن <memory>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
ساكن فراغ checkDataHazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    ܐܢ (!mTaskGraph.slowAsserts()) ܦܢܐ;

    // Order MTasks by their stable ids, so the report is deterministic
    بنية MTaskIdLessThan final {
        منطقي operator()(ثابت LogicMTask* ap, ثابت LogicMTask* bp) ثابت { ܦܢܐ *ap < *bp; }
    };
    بنية VarInfo final {
        منطقي m_seen = ܕܓܠܐ;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ثابت VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    لكل (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        لكل (ثابت OrderMoveVertex& mVtx : mtask.vertexList()) {
            ثابت OrderLogicVertex* ثابت lVtxp = mVtx.logicp();
            ܐܢ (!lVtxp) استمر;  // A variable vertex, which performs no access itself
            لكل (ثابت OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ثابت vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                ܐܢ (!varInfo.m_seen) {
                    varInfo.m_seen = ܫܪܪܐ;
                    vscps.push_back(vscp);
                }
                ثابت auto pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                ܐܢ (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    لكل (AstVarScope* ثابت vscp : vscps) {
        ثابت auto& byMTask = varInfos(vscp).m_byMTask;
        لكل (auto aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            لكل (auto bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                ܐܢ (aIt->second.isReadOnly() && bIt->second.isReadOnly()) استمر;
                LogicMTask* ثابت ap = aIt->first;
                LogicMTask* ثابت bp = bIt->first;
                ܐܢ (mTaskGraph.pathExists(ap, bp, nullptr)) استمر;
                ܐܢ (mTaskGraph.pathExists(bp, ap, nullptr)) استمر;
                // LCOV_EXCL_START
                ܐܢ (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    ܐܢ (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

ساكن std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
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
    ܐܢ (v3Global.opt.threadsCoarsen()) {
        ثابت صحيح nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ثابت uint64_t fudgeNum = 3;
        ثابت uint64_t fudgeDen = 5;
        ثابت uint64_t limit = (mTaskGraphp->totalCost() * fudgeNum) / (nThreads * fudgeDen);
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
    لكل (V3GraphVertex* ثابت vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        ܟܡܐ (OrderMoveVertex* ثابت mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    ܦܢܐ mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

صنف DpiThreadsVisitor final : عام VNVisitorConst {
    صحيح m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    فراغ visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstNodeCCall* nodep) override { iterateConst(nodep->funcp()); }
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = افتراضي;
    VL_UNCOPYABLE(DpiThreadsVisitor);

عام:
    // Number of threads occupied by the given MTask
    ساكن صحيح apply(ثابت ExecMTask* mTaskp) {
        ܦܢܐ DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

AstNodeStmt* V3Order::createParallel(OrderMoveGraph& moveGraph, ثابت std::string& tag,
                                     منطقي slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ثابت std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    ܐܢ (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    لكل (V3GraphVertex* ثابت vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* ثابت mVtxp = vtxp->as<OrderMoveVertex>();
        ܐܢ (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    لكل (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* ثابت mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        لكل (V3GraphEdge* ثابت edgep : mVtxp->outEdges().unlinkable()) {
            ثابت OrderMoveVertex* ثابت toMVtxp = edgep->top()->as<OrderMoveVertex>();
            ܐܢ (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    ܐܢ (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    FileLine* ثابت flp = v3Global.rootp()->fileline();
    AstScope* ثابت scopep = v3Global.rootp()->topScopep()->scopep();
    AstExecGraph* ثابت execGraphp = جديد AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* ثابت depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ثابت LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    بنية MTaskVxIdLessThan final {
        منطقي operator()(ثابت V3GraphVertex* lhsp, ثابت V3GraphVertex* rhsp) ثابت {
            ܦܢܐ lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    ܟܡܐ (ثابت V3GraphVertex* ثابت vtxp = mtaskStream.nextp()) {
        ثابت LogicMTask* ثابت cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ثابت mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        ܐܢ (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            استمر;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        ܟܡܐ (OrderMoveVertex* ثابت mVtxp = mTaskp->vertexList().unlinkFront()) {
            ܐܢ (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        ܟܡܐ (OrderMoveVertex* ثابت mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            ܐܢ (OrderLogicVertex* ثابت logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* ثابت domScopep = &mVtxp->domScope();
                ܐܢ (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ثابت execMTaskp = جديد ExecMTask{execGraphp, scopep, emitter.getStmts()};
        ܐܢ (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        ثابت منطقي newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ثابت callp = جديد AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        لكل (ثابت V3GraphEdge& edge : mTaskp->inEdges()) {
            ثابت V3GraphVertex* fromVxp = edge.fromp();
            ثابت LogicMTask* ثابت fromp = fromVxp->as<ثابت LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            ܐܢ (fromp == mTaskGraphp->entryp()) استمر;
            جديد V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    لكل (V3GraphVertex* ثابت vtxp : moveGraph.vertices().unlinkable()) {
        ܐܢ (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    ܦܢܐ execGraphp;
}
