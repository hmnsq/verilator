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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3AstUserAllocator.h"
#অন্তর্ভাৱ "V3Control.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3ExecGraph.h"
#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3GraphStream.h"
#অন্তর্ভাৱ "V3OrderCFuncEmitter.h"
#অন্তর্ভাৱ "V3OrderInternal.h"
#অন্তর্ভাৱ "V3OrderMTaskGraph.h"

#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
স্থির শূন্য checkDataHazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    যদি (!mTaskGraph.slowAsserts()) ঘূৰাই;

    // Order MTasks by their stable ids, so the report is deterministic
    সংরচনা MTaskIdLessThan final {
        তার্কিক operator()(ধ্রুৱ LogicMTask* ap, ধ্রুৱ LogicMTask* bp) ধ্রুৱ { ঘূৰাই *ap < *bp; }
    };
    সংরচনা VarInfo final {
        তার্কিক m_seen = মিছা;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ধ্রুৱ VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    বাবে (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        বাবে (ধ্রুৱ OrderMoveVertex& mVtx : mtask.vertexList()) {
            ধ্রুৱ OrderLogicVertex* ধ্রুৱ lVtxp = mVtx.logicp();
            যদি (!lVtxp) অনুৱর্ত;  // A variable vertex, which performs no access itself
            বাবে (ধ্রুৱ OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ধ্রুৱ vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                যদি (!varInfo.m_seen) {
                    varInfo.m_seen = সঁচা;
                    vscps.ঠেলো_back(vscp);
                }
                ধ্রুৱ auto pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                যদি (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    বাবে (AstVarScope* ধ্রুৱ vscp : vscps) {
        ধ্রুৱ auto& byMTask = varInfos(vscp).m_byMTask;
        বাবে (auto aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            বাবে (auto bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                যদি (aIt->second.isReadOnly() && bIt->second.isReadOnly()) অনুৱর্ত;
                LogicMTask* ধ্রুৱ ap = aIt->first;
                LogicMTask* ধ্রুৱ bp = bIt->first;
                যদি (mTaskGraph.pathExists(ap, bp, nullptr)) অনুৱর্ত;
                যদি (mTaskGraph.pathExists(bp, ap, nullptr)) অনুৱর্ত;
                // LCOV_EXCL_START
                যদি (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    যদি (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

স্থির std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
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
    যদি (v3Global.opt.threadsCoarsen()) {
        ধ্রুৱ পূর্ণাঙ্ক nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ধ্রুৱ uint64_t fudge_অঙ্ক = 3;
        ধ্রুৱ uint64_t fudgeDen = 5;
        ধ্রুৱ uint64_t limit = (mTaskGraphp->totalCost() * fudge_অঙ্ক) / (nThreads * fudgeDen);
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
    বাবে (V3GraphVertex* ধ্রুৱ vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ধ্রুৱ mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        যেতিয়ালৈ (OrderMoveVertex* ধ্রুৱ mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    ঘূৰাই mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

শ্ৰেণী DpiThreadsVisitor final : সার্ৱজনিক VNVisitorConst {
    পূর্ণাঙ্ক m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    শূন্য visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    শূন্য visit(Ast_গ্ৰন্থি_CCall* nodep) override { iterateConst(nodep->funcp()); }
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = স্ৱভাৱ;
    VL_UNCOPYABLE(DpiThreadsVisitor);

সার্ৱজনিক:
    // Number of threads occupied by the given MTask
    স্থির পূর্ণাঙ্ক apply(ধ্রুৱ ExecMTask* mTaskp) {
        ঘূৰাই DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_গ্ৰন্থি_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, ধ্রুৱ std::string& tag,
                                     তার্কিক slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ধ্রুৱ std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    যদি (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    বাবে (V3GraphVertex* ধ্রুৱ vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* ধ্রুৱ mVtxp = vtxp->as<OrderMoveVertex>();
        যদি (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    বাবে (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* ধ্রুৱ mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* ধ্রুৱ mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        বাবে (V3GraphEdge* ধ্রুৱ edgep : mVtxp->outEdges().unlinkable()) {
            ধ্রুৱ OrderMoveVertex* ধ্রুৱ toMVtxp = edgep->শীৰ্ষ()->as<OrderMoveVertex>();
            যদি (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    যদি (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_সাৰি* ধ্রুৱ flp = v3Global.rootp()->fileline();
    AstScope* ধ্রুৱ scopep = v3Global.rootp()->শীৰ্ষ_Scopep()->scopep();
    AstExecGraph* ধ্রুৱ execGraphp = নূতন AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* ধ্রুৱ depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ধ্রুৱ LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    সংরচনা MTaskVxIdLessThan final {
        তার্কিক operator()(ধ্রুৱ V3GraphVertex* lhsp, ধ্রুৱ V3GraphVertex* rhsp) ধ্রুৱ {
            ঘূৰাই lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    যেতিয়ালৈ (ধ্রুৱ V3GraphVertex* ধ্রুৱ vtxp = mtaskStream.nextp()) {
        ধ্রুৱ LogicMTask* ধ্রুৱ cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ধ্রুৱ mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        যদি (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            অনুৱর্ত;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        যেতিয়ালৈ (OrderMoveVertex* ধ্রুৱ mVtxp = mTaskp->vertexList().unlinkFront()) {
            যদি (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        যেতিয়ালৈ (OrderMoveVertex* ধ্রুৱ mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            যদি (OrderLogicVertex* ধ্রুৱ logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* ধ্রুৱ domScopep = &mVtxp->domScope();
                যদি (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ধ্রুৱ execMTaskp = নূতন ExecMTask{execGraphp, scopep, emitter.getStmts()};
        যদি (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        ধ্রুৱ তার্কিক newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        বাবে (ধ্রুৱ V3GraphEdge& edge : mTaskp->inEdges()) {
            ধ্রুৱ V3GraphVertex* fromVxp = edge.fromp();
            ধ্রুৱ LogicMTask* ধ্রুৱ fromp = fromVxp->as<ধ্রুৱ LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            যদি (fromp == mTaskGraphp->entryp()) অনুৱর্ত;
            নূতন V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    বাবে (V3GraphVertex* ধ্রুৱ vtxp : moveGraph.vertices().unlinkable()) {
        যদি (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    ঘূৰাই execGraphp;
}
