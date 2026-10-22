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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3ExecGraph.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3GraphStream.h"
#അന്തര്ഭാവ "V3OrderCFuncEmitter.h"
#അന്തര്ഭാവ "V3OrderInternal.h"
#അന്തര്ഭാവ "V3OrderMTaskGraph.h"

#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
സ്ഥിതം ശൂന്യം checkDataHazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    എങ്കിൽ (!mTaskGraph.slowAsserts()) തിരികെ;

    // Order MTasks by their stable ids, so the report is deterministic
    ഘടന MTaskIdLessThan final {
        തർക്കം operator()(സ്ഥിരം LogicMTask* ap, സ്ഥിരം LogicMTask* bp) സ്ഥിരം { തിരികെ *ap < *bp; }
    };
    ഘടന VarInfo final {
        തർക്കം m_seen = അസത്യം;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    സ്ഥിരം VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    വേണ്ടി (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        വേണ്ടി (സ്ഥിരം OrderMoveVertex& mVtx : mtask.vertexList()) {
            സ്ഥിരം OrderLogicVertex* സ്ഥിരം lVtxp = mVtx.logicp();
            എങ്കിൽ (!lVtxp) തുടരുക;  // A variable vertex, which performs no access itself
            വേണ്ടി (സ്ഥിരം OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* സ്ഥിരം vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                എങ്കിൽ (!varInfo.m_seen) {
                    varInfo.m_seen = സത്യം;
                    vscps.തള്ളുക_back(vscp);
                }
                സ്ഥിരം auto pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                എങ്കിൽ (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    വേണ്ടി (AstVarScope* സ്ഥിരം vscp : vscps) {
        സ്ഥിരം auto& byMTask = varInfos(vscp).m_byMTask;
        വേണ്ടി (auto aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            വേണ്ടി (auto bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                എങ്കിൽ (aIt->second.isReadOnly() && bIt->second.isReadOnly()) തുടരുക;
                LogicMTask* സ്ഥിരം ap = aIt->first;
                LogicMTask* സ്ഥിരം bp = bIt->first;
                എങ്കിൽ (mTaskGraph.pathExists(ap, bp, nullptr)) തുടരുക;
                എങ്കിൽ (mTaskGraph.pathExists(bp, ap, nullptr)) തുടരുക;
                // LCOV_EXCL_START
                എങ്കിൽ (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    എങ്കിൽ (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

സ്ഥിതം std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
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
    എങ്കിൽ (v3Global.opt.threadsCoarsen()) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        സ്ഥിരം uint64_t fudge_അക്കം = 3;
        സ്ഥിരം uint64_t fudgeDen = 5;
        സ്ഥിരം uint64_t limit = (mTaskGraphp->totalCost() * fudge_അക്കം) / (nThreads * fudgeDen);
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
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* സ്ഥിരം mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        ആകുമ്പോൾ (OrderMoveVertex* സ്ഥിരം mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    തിരികെ mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

വർഗ്ഗം DpiThreadsVisitor final : പൊതു VNVisitorConst {
    പൂർണ്ണസംഖ്യ m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    ശൂന്യം visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_CCall* nodep) override { iterateConst(nodep->funcp()); }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(DpiThreadsVisitor);

പൊതു:
    // Number of threads occupied by the given MTask
    സ്ഥിതം പൂർണ്ണസംഖ്യ apply(സ്ഥിരം ExecMTask* mTaskp) {
        തിരികെ DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_കണ്ണി_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, സ്ഥിരം std::string& tag,
                                     തർക്കം slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    സ്ഥിരം std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    എങ്കിൽ (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* സ്ഥിരം mVtxp = vtxp->as<OrderMoveVertex>();
        എങ്കിൽ (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    വേണ്ടി (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* സ്ഥിരം mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* സ്ഥിരം mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : mVtxp->outEdges().unlinkable()) {
            സ്ഥിരം OrderMoveVertex* സ്ഥിരം toMVtxp = edgep->മുകൾ()->as<OrderMoveVertex>();
            എങ്കിൽ (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    എങ്കിൽ (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_വരി* സ്ഥിരം flp = v3Global.rootp()->fileline();
    AstScope* സ്ഥിരം scopep = v3Global.rootp()->മുകൾ_Scopep()->scopep();
    AstExecGraph* സ്ഥിരം execGraphp = പുതിയ AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* സ്ഥിരം depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<സ്ഥിരം LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    ഘടന MTaskVxIdLessThan final {
        തർക്കം operator()(സ്ഥിരം V3GraphVertex* lhsp, സ്ഥിരം V3GraphVertex* rhsp) സ്ഥിരം {
            തിരികെ lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    ആകുമ്പോൾ (സ്ഥിരം V3GraphVertex* സ്ഥിരം vtxp = mtaskStream.nextp()) {
        സ്ഥിരം LogicMTask* സ്ഥിരം cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* സ്ഥിരം mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        എങ്കിൽ (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            തുടരുക;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        ആകുമ്പോൾ (OrderMoveVertex* സ്ഥിരം mVtxp = mTaskp->vertexList().unlinkFront()) {
            എങ്കിൽ (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        ആകുമ്പോൾ (OrderMoveVertex* സ്ഥിരം mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            എങ്കിൽ (OrderLogicVertex* സ്ഥിരം logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* സ്ഥിരം domScopep = &mVtxp->domScope();
                എങ്കിൽ (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* സ്ഥിരം execMTaskp = പുതിയ ExecMTask{execGraphp, scopep, emitter.getStmts()};
        എങ്കിൽ (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        സ്ഥിരം തർക്കം newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edge : mTaskp->inEdges()) {
            സ്ഥിരം V3GraphVertex* fromVxp = edge.fromp();
            സ്ഥിരം LogicMTask* സ്ഥിരം fromp = fromVxp->as<സ്ഥിരം LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            എങ്കിൽ (fromp == mTaskGraphp->entryp()) തുടരുക;
            പുതിയ V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : moveGraph.vertices().unlinkable()) {
        എങ്കിൽ (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    തിരികെ execGraphp;
}
