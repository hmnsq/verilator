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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3ExecGraph.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3GraphStream.h"
#अन्तर्भाव "V3OrderCFuncEmitter.h"
#अन्तर्भाव "V3OrderInternal.h"
#अन्तर्भाव "V3OrderMTaskGraph.h"

#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
स्थिर शून्य checkDataHazards(OrderMTaskGraph& mTaskGraph) {
    // Expensive, so only with '--debug-partition'
    जर (!mTaskGraph.slowAsserts()) परत;

    // Order MTasks by their stable ids, so the report is deterministic
    संरचना MTaskIdLessThan final {
        तार्किक चालक()(स्थिरांक LogicMTask* ap, स्थिरांक LogicMTask* bp) स्थिरांक { परत *ap < *bp; }
    };
    संरचना VarInfo final {
        तार्किक m_seen = असत्य;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    स्थिरांक VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    साठी (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        साठी (स्थिरांक OrderMoveVertex& mVtx : mtask.vertexList()) {
            स्थिरांक OrderLogicVertex* स्थिरांक lVtxp = mVtx.logicp();
            जर (!lVtxp) सुरूठेवा;  // A variable vertex, which performs no access itself
            साठी (स्थिरांक OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* स्थिरांक vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                जर (!varInfo.m_seen) {
                    varInfo.m_seen = सत्य;
                    vscps.ढकला_back(vscp);
                }
                स्थिरांक स्वतः pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                जर (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    साठी (AstVarScope* स्थिरांक vscp : vscps) {
        स्थिरांक स्वतः& byMTask = varInfos(vscp).m_byMTask;
        साठी (स्वतः aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            साठी (स्वतः bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                जर (aIt->second.isReadOnly() && bIt->second.isReadOnly()) सुरूठेवा;
                LogicMTask* स्थिरांक ap = aIt->first;
                LogicMTask* स्थिरांक bp = bIt->first;
                जर (mTaskGraph.pathExists(ap, bp, nullptr)) सुरूठेवा;
                जर (mTaskGraph.pathExists(bp, ap, nullptr)) सुरूठेवा;
                // LCOV_EXCL_START
                जर (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    जर (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

स्थिर std::unique_ptr<OrderMTaskGraph> partition(OrderMoveGraph& moveGraph) {
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
    जर (v3Global.opt.threadsCoarsen()) {
        स्थिरांक पूर्णांक nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        स्थिरांक uint64_t fudge_अंक = 3;
        स्थिरांक uint64_t fudgeDen = 5;
        स्थिरांक uint64_t limit = (mTaskGraphp->totalCost() * fudge_अंक) / (nThreads * fudgeDen);
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
    साठी (V3GraphVertex* स्थिरांक vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* स्थिरांक mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        पर्यंत (OrderMoveVertex* स्थिरांक mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    परत mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

वर्ग DpiThreadsVisitor final : सार्वजनिक VNVisitorConst {
    पूर्णांक m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    शून्य visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाठ_CCall* nodep) override { iterateConst(nodep->funcp()); }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = पूर्वनिर्धारित;
    VL_UNCOPYABLE(DpiThreadsVisitor);

सार्वजनिक:
    // Number of threads occupied by the given MTask
    स्थिर पूर्णांक apply(स्थिरांक ExecMTask* mTaskp) {
        परत DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_गाठ_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, स्थिरांक std::string& tag,
                                     तार्किक slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    स्थिरांक std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    जर (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    साठी (V3GraphVertex* स्थिरांक vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* स्थिरांक mVtxp = vtxp->as<OrderMoveVertex>();
        जर (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    साठी (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* स्थिरांक mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* स्थिरांक mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        साठी (V3GraphEdge* स्थिरांक edgep : mVtxp->outEdges().unlinkable()) {
            स्थिरांक OrderMoveVertex* स्थिरांक toMVtxp = edgep->शीर्ष()->as<OrderMoveVertex>();
            जर (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    जर (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_ओळ* स्थिरांक flp = v3Global.rootp()->fileline();
    AstScope* स्थिरांक scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();
    AstExecGraph* स्थिरांक execGraphp = नवीन AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* स्थिरांक depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<स्थिरांक LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    संरचना MTaskVxIdLessThan final {
        तार्किक चालक()(स्थिरांक V3GraphVertex* lhsp, स्थिरांक V3GraphVertex* rhsp) स्थिरांक {
            परत lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    पर्यंत (स्थिरांक V3GraphVertex* स्थिरांक vtxp = mtaskStream.nextp()) {
        स्थिरांक LogicMTask* स्थिरांक cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* स्थिरांक mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        जर (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            सुरूठेवा;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        पर्यंत (OrderMoveVertex* स्थिरांक mVtxp = mTaskp->vertexList().unlinkFront()) {
            जर (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        पर्यंत (OrderMoveVertex* स्थिरांक mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            जर (OrderLogicVertex* स्थिरांक logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* स्थिरांक domScopep = &mVtxp->domScope();
                जर (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* स्थिरांक execMTaskp = नवीन ExecMTask{execGraphp, scopep, emitter.getStmts()};
        जर (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        स्थिरांक तार्किक newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* स्थिरांक callp = नवीन AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        साठी (स्थिरांक V3GraphEdge& edge : mTaskp->inEdges()) {
            स्थिरांक V3GraphVertex* fromVxp = edge.fromp();
            स्थिरांक LogicMTask* स्थिरांक fromp = fromVxp->as<स्थिरांक LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            जर (fromp == mTaskGraphp->entryp()) सुरूठेवा;
            नवीन V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    साठी (V3GraphVertex* स्थिरांक vtxp : moveGraph.vertices().unlinkable()) {
        जर (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    परत execGraphp;
}
