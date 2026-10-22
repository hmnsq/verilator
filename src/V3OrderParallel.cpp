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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Ast.h"
#समावेश "V3AstUserAllocator.h"
#समावेश "V3Control.h"
#समावेश "V3Error.h"
#समावेश "V3ExecGraph.h"
#समावेश "V3Graph.h"
#समावेश "V3GraphStream.h"
#समावेश "V3OrderCFuncEmitter.h"
#समावेश "V3OrderInternal.h"
#समावेश "V3OrderMTaskGraph.h"

#समावेश <map>
#समावेश <memory>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
अटल व्योम check_तथ्य_Hazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    अगर (!mTaskGraph.slowAsserts()) वापस;

    // Order MTasks by their stable ids, so the report is deterministic
    काष्ठा MTaskIdLessThan final {
        bool चालक()(स्थिर LogicMTask* ap, स्थिर LogicMTask* bp) स्थिर { वापस *ap < *bp; }
    };
    काष्ठा VarInfo final {
        bool m_seen = false;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    स्थिर VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    क्रम (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        क्रम (स्थिर OrderMoveVertex& mVtx : mtask.vertexList()) {
            स्थिर OrderLogicVertex* स्थिर lVtxp = mVtx.logicp();
            अगर (!lVtxp) जारी;  // A variable vertex, which performs no access itself
            क्रम (स्थिर OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* स्थिर vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                अगर (!varInfo.m_seen) {
                    varInfo.m_seen = true;
                    vscps.धकेलो_back(vscp);
                }
                स्थिर स्वतः pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                अगर (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    क्रम (AstVarScope* स्थिर vscp : vscps) {
        स्थिर स्वतः& byMTask = varInfos(vscp).m_byMTask;
        क्रम (स्वतः aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            क्रम (स्वतः bIt = std::अग्र(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                अगर (aIt->second.isReadOnly() && bIt->second.isReadOnly()) जारी;
                LogicMTask* स्थिर ap = aIt->first;
                LogicMTask* स्थिर bp = bIt->first;
                अगर (mTaskGraph.pathExists(ap, bp, nullptr)) जारी;
                अगर (mTaskGraph.pathExists(bp, ap, nullptr)) जारी;
                // LCOV_EXCL_START
                अगर (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    अगर (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

अटल std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
    // Build the initial MTask graph. Initially, each MTask just wraps one OrderMoveVertex. We will
    // merge MTasks together and eventually each MTask will wrap a large number of OrderMoveVertex
    // (and the logic nodes therein).
    std::unique_ptr<OrderMTaskGraph> mTaskGraphp = OrderMTaskGraph::build(moveGraph);
    mTaskGraphp->hashGraphDebug("initial MTask graph");

    // Add edges to eliminate data hazards
    OrderMTaskGraph::fix_तथ्य_Hazards(*mTaskGraphp);
    mTaskGraphp->hashGraphDebug("MTask graph after fixDataHazards()");

    // Merge MTask nodes together, repeatedly, until the critical path budget is reached. Coarsens
    // the graph, usually by several orders of magnitude. Some tests disable this for stability,
    // it should always be enabled in production.
    अगर (v3Global.opt.threadsCoarsen()) {
        स्थिर पूर्णांक nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        स्थिर uint64_t fudge_अंक = 3;
        स्थिर uint64_t fudgeDen = 5;
        स्थिर uint64_t limit = (mTaskGraphp->योग_Cost() * fudge_अंक) / (nThreads * fudgeDen);
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
    check_तथ्य_Hazards(*mTaskGraphp);

    // Set OrderMoveVertex::userp to indicate the mtask it is part of.
    moveGraph.userClearVertices();
    क्रम (V3GraphVertex* स्थिर vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* स्थिर mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        जबतक (OrderMoveVertex* स्थिर mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    वापस mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

श्रेणी DpiThreadsVisitor final : खुला VNVisitorConst {
    पूर्णांक m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    व्योम visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    व्योम visit(Ast_गाँठ_CCall* nodep) override { iterateConst(nodep->funcp()); }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = शेष;
    VL_UNCOPYABLE(DpiThreadsVisitor);

खुला:
    // Number of threads occupied by the given MTask
    अटल पूर्णांक apply(स्थिर ExecMTask* mTaskp) {
        वापस DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_गाँठ_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, स्थिर std::string& tag,
                                     bool slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    स्थिर std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    अगर (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    क्रम (V3GraphVertex* स्थिर vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* स्थिर mVtxp = vtxp->as<OrderMoveVertex>();
        अगर (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    क्रम (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* स्थिर mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* स्थिर mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        क्रम (V3GraphEdge* स्थिर edgep : mVtxp->outEdges().unlinkable()) {
            स्थिर OrderMoveVertex* स्थिर toMVtxp = edgep->शीर्ष()->as<OrderMoveVertex>();
            अगर (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    अगर (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_पंक्ति* स्थिर flp = v3Global.rootp()->fileline();
    AstScope* स्थिर scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();
    AstExecGraph* स्थिर execGraphp = नया AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* स्थिर depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<स्थिर LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    काष्ठा MTaskVxIdLessThan final {
        bool चालक()(स्थिर V3GraphVertex* lhsp, स्थिर V3GraphVertex* rhsp) स्थिर {
            वापस lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.लो()};
    जबतक (स्थिर V3GraphVertex* स्थिर vtxp = mtaskStream.nextp()) {
        स्थिर LogicMTask* स्थिर cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* स्थिर mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        अगर (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            जारी;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        जबतक (OrderMoveVertex* स्थिर mVtxp = mTaskp->vertexList().unlinkFront()) {
            अगर (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* पूर्व_Dom_Scopep = nullptr;
        जबतक (OrderMoveVertex* स्थिर mVtxp = serializer.get_अग्र()) {
            // We only really care about logic vertices
            अगर (OrderLogicVertex* स्थिर logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* स्थिर domScopep = &mVtxp->domScope();
                अगर (domScopep != पूर्व_Dom_Scopep) emitter.forceNewFunction();
                पूर्व_Dom_Scopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* स्थिर execMTaskp = नया ExecMTask{execGraphp, scopep, emitter.getStmts()};
        अगर (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        स्थिर bool newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* स्थिर callp = नया AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        क्रम (स्थिर V3GraphEdge& edge : mTaskp->inEdges()) {
            स्थिर V3GraphVertex* fromVxp = edge.fromp();
            स्थिर LogicMTask* स्थिर fromp = fromVxp->as<स्थिर LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            अगर (fromp == mTaskGraphp->entryp()) जारी;
            नया V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    क्रम (V3GraphVertex* स्थिर vtxp : moveGraph.vertices().unlinkable()) {
        अगर (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    वापस execGraphp;
}
