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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3ExecGraph.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3GraphStream.h"
#அந்தர்பாவ "V3OrderCFuncEmitter.h"
#அந்தர்பாவ "V3OrderInternal.h"
#அந்தர்பாவ "V3OrderMTaskGraph.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
நிலையான வெறுமை checkDataHazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    எனில் (!mTaskGraph.slowAsserts()) திரும்பு;

    // Order MTasks by their stable ids, so the report is deterministic
    கட்டமைப்பு MTaskIdLessThan final {
        தருக்க operator()(மாறிலி LogicMTask* ap, மாறிலி LogicMTask* bp) மாறிலி { திரும்பு *ap < *bp; }
    };
    கட்டமைப்பு VarInfo final {
        தருக்க m_seen = பொய்;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    மாறிலி VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    க்கு (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        க்கு (மாறிலி OrderMoveVertex& mVtx : mtask.vertexList()) {
            மாறிலி OrderLogicVertex* மாறிலி lVtxp = mVtx.logicp();
            எனில் (!lVtxp) தொடர்;  // A variable vertex, which performs no access itself
            க்கு (மாறிலி OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* மாறிலி vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                எனில் (!varInfo.m_seen) {
                    varInfo.m_seen = உண்மை;
                    vscps.தள்ளு_back(vscp);
                }
                மாறிலி auto pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                எனில் (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    க்கு (AstVarScope* மாறிலி vscp : vscps) {
        மாறிலி auto& byMTask = varInfos(vscp).m_byMTask;
        க்கு (auto aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            க்கு (auto bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                எனில் (aIt->second.isReadOnly() && bIt->second.isReadOnly()) தொடர்;
                LogicMTask* மாறிலி ap = aIt->first;
                LogicMTask* மாறிலி bp = bIt->first;
                எனில் (mTaskGraph.pathExists(ap, bp, nullptr)) தொடர்;
                எனில் (mTaskGraph.pathExists(bp, ap, nullptr)) தொடர்;
                // LCOV_EXCL_START
                எனில் (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    எனில் (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

நிலையான std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
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
    எனில் (v3Global.opt.threadsCoarsen()) {
        மாறிலி முழு nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        மாறிலி uint64_t fudge_இலக்கம் = 3;
        மாறிலி uint64_t fudgeDen = 5;
        மாறிலி uint64_t limit = (mTaskGraphp->totalCost() * fudge_இலக்கம்) / (nThreads * fudgeDen);
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
    க்கு (V3GraphVertex* மாறிலி vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* மாறிலி mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        போது (OrderMoveVertex* மாறிலி mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    திரும்பு mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

வகுப்பு DpiThreadsVisitor final : பொது VNVisitorConst {
    முழு m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    வெறுமை visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(Ast_முனை_CCall* nodep) override { iterateConst(nodep->funcp()); }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = இயல்பு;
    VL_UNCOPYABLE(DpiThreadsVisitor);

பொது:
    // Number of threads occupied by the given MTask
    நிலையான முழு apply(மாறிலி ExecMTask* mTaskp) {
        திரும்பு DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_முனை_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, மாறிலி std::string& tag,
                                     தருக்க slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    மாறிலி std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    எனில் (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    க்கு (V3GraphVertex* மாறிலி vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* மாறிலி mVtxp = vtxp->as<OrderMoveVertex>();
        எனில் (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    க்கு (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* மாறிலி mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* மாறிலி mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        க்கு (V3GraphEdge* மாறிலி edgep : mVtxp->outEdges().unlinkable()) {
            மாறிலி OrderMoveVertex* மாறிலி toMVtxp = edgep->உச்சி()->as<OrderMoveVertex>();
            எனில் (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    எனில் (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_வரி* மாறிலி flp = v3Global.rootp()->fileline();
    AstScope* மாறிலி scopep = v3Global.rootp()->உச்சி_Scopep()->scopep();
    AstExecGraph* மாறிலி execGraphp = புதிய AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* மாறிலி depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<மாறிலி LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    கட்டமைப்பு MTaskVxIdLessThan final {
        தருக்க operator()(மாறிலி V3GraphVertex* lhsp, மாறிலி V3GraphVertex* rhsp) மாறிலி {
            திரும்பு lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    போது (மாறிலி V3GraphVertex* மாறிலி vtxp = mtaskStream.nextp()) {
        மாறிலி LogicMTask* மாறிலி cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* மாறிலி mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        எனில் (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            தொடர்;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        போது (OrderMoveVertex* மாறிலி mVtxp = mTaskp->vertexList().unlinkFront()) {
            எனில் (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        போது (OrderMoveVertex* மாறிலி mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            எனில் (OrderLogicVertex* மாறிலி logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* மாறிலி domScopep = &mVtxp->domScope();
                எனில் (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* மாறிலி execMTaskp = புதிய ExecMTask{execGraphp, scopep, emitter.getStmts()};
        எனில் (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        மாறிலி தருக்க newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* மாறிலி callp = புதிய AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        க்கு (மாறிலி V3GraphEdge& edge : mTaskp->inEdges()) {
            மாறிலி V3GraphVertex* fromVxp = edge.fromp();
            மாறிலி LogicMTask* மாறிலி fromp = fromVxp->as<மாறிலி LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            எனில் (fromp == mTaskGraphp->entryp()) தொடர்;
            புதிய V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    க்கு (V3GraphVertex* மாறிலி vtxp : moveGraph.vertices().unlinkable()) {
        எனில் (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    திரும்பு execGraphp;
}
