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
    यदि (!mTaskGraph.slowAsserts()) प्रत्यागम;

    // Order MTasks by their stable ids, so the report is deterministic
    संरचना MTaskIdLessThan final {
        तार्किक operator()(ध्रुव LogicMTask* ap, ध्रुव LogicMTask* bp) ध्रुव { प्रत्यागम *ap < *bp; }
    };
    संरचना VarInfo final {
        तार्किक m_seen = असत्य;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ध्रुव VNUser1InUse user1InUse;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::vector<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    प्रति (V3GraphVertex& vtx : mTaskGraph.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        प्रति (ध्रुव OrderMoveVertex& mVtx : mtask.vertexList()) {
            ध्रुव OrderLogicVertex* ध्रुव lVtxp = mVtx.logicp();
            यदि (!lVtxp) अनुवर्त;  // A variable vertex, which performs no access itself
            प्रति (ध्रुव OrderLogicVertex::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ध्रुव vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                यदि (!varInfo.m_seen) {
                    varInfo.m_seen = सत्य;
                    vscps.धकेलो_back(vscp);
                }
                ध्रुव auto pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                यदि (!pair.second && pair.first->second != acc.m_access) {
                    pair.first->second = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* firstHazardp = nullptr;  // First variable with a hazard, for the error below
    प्रति (AstVarScope* ध्रुव vscp : vscps) {
        ध्रुव auto& byMTask = varInfos(vscp).m_byMTask;
        प्रति (auto aIt = byMTask.begin(); aIt != byMTask.end(); ++aIt) {
            प्रति (auto bIt = std::next(aIt); bIt != byMTask.end(); ++bIt) {
                // Concurrent reads are not a hazard
                यदि (aIt->second.isReadOnly() && bIt->second.isReadOnly()) अनुवर्त;
                LogicMTask* ध्रुव ap = aIt->first;
                LogicMTask* ध्रुव bp = bIt->first;
                यदि (mTaskGraph.pathExists(ap, bp, nullptr)) अनुवर्त;
                यदि (mTaskGraph.pathExists(bp, ap, nullptr)) अनुवर्त;
                // LCOV_EXCL_START
                यदि (!firstHazardp) firstHazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->second.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->second.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    यदि (firstHazardp) firstHazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
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
    यदि (v3Global.opt.threadsCoarsen()) {
        ध्रुव पूर्णाङ्क nThreads = v3Global.opt.threads();
        UASSERT(nThreads >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ध्रुव uint64_t fudge_अंक = 3;
        ध्रुव uint64_t fudgeDen = 5;
        ध्रुव uint64_t limit = (mTaskGraphp->totalCost() * fudge_अंक) / (nThreads * fudgeDen);
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
    प्रति (V3GraphVertex* ध्रुव vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ध्रुव mtaskp = vtxp->as<LogicMTask>();
        OrderMoveVertex::List& vertexList = mtaskp->vertexList();
        यावत् (OrderMoveVertex* ध्रुव mVtxp = vertexList.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    प्रत्यागम mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

वर्ग DpiThreadsVisitor final : सार्वजनिक VNVisitorConst {
    पूर्णाङ्क m_threads = 1;  // Max number of threads used by this mtask

    // METHODS
    शून्य visit(AstCFunc* nodep) override {
        m_threads = std::max(m_threads, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    शून्य visit(Ast_गाँठ_CCall* nodep) override { iterateConst(nodep->funcp()); }
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit DpiThreadsVisitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~DpiThreadsVisitor() override = स्वभाव;
    VL_UNCOPYABLE(DpiThreadsVisitor);

सार्वजनिक:
    // Number of threads occupied by the given MTask
    स्थिर पूर्णाङ्क apply(ध्रुव ExecMTask* mTaskp) {
        प्रत्यागम DpiThreadsVisitor{mTaskp->funcp()}.m_threads;
    }
};

//######################################################################
// Entry point

Ast_गाँठ_Stmt* V3Order::createParallel(OrderMoveGraph& moveGraph, ध्रुव std::string& tag,
                                     तार्किक slow) {
    UINFO(2, "  Constructing parallel code for '" + tag + "'");

    // For nondeterminism debugging
    moveGraph.hashGraphDebug("V3Order::createParallel input OrderMoveGraph");
    moveGraph.orderGraph().hashGraphDebug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ध्रुव std::unique_ptr<OrderMTaskGraph> mTaskGraphp = partition(moveGraph);
    यदि (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    प्रति (V3GraphVertex* ध्रुव vtxp : moveGraph.vertices().unlinkable()) {
        OrderMoveVertex* ध्रुव mVtxp = vtxp->as<OrderMoveVertex>();
        यदि (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->rerouteEdges(&moveGraph);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    प्रति (V3GraphVertex& vtx : moveGraph.vertices()) {
        OrderMoveVertex* ध्रुव mVtxp = vtx.as<OrderMoveVertex>();
        LogicMTask* ध्रुव mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->vertexList().linkBack(mVtxp);
        // Remove edges crossing between MTasks
        प्रति (V3GraphEdge* ध्रुव edgep : mVtxp->outEdges().unlinkable()) {
            ध्रुव OrderMoveVertex* ध्रुव toMVtxp = edgep->शीर्ष()->as<OrderMoveVertex>();
            यदि (mtaskp != toMVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    यदि (dumpGraphLevel() >= 9) moveGraph.dumpDotFilePrefixed(tag + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_पंक्ति* ध्रुव flp = v3Global.rootp()->fileline();
    AstScope* ध्रुव scopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();
    AstExecGraph* ध्रुव execGraphp = नूतन AstExecGraph{flp, tag, moveGraph.orderGraph().trigVscp()};
    V3Graph* ध्रुव depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ध्रुव LogicMTask*, ExecMTask*> logicMTaskToExecMTask;
    OrderMoveGraphSerializer serializer{moveGraph};
    V3OrderCFuncEmitter emitter{moveGraph.orderGraph().trigVscp(), tag, slow};
    // Sort LogicMTask vertices by their serial IDs.
    संरचना MTaskVxIdLessThan final {
        तार्किक operator()(ध्रुव V3GraphVertex* lhsp, ध्रुव V3GraphVertex* rhsp) ध्रुव {
            प्रत्यागम lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    GraphStream<MTaskVxIdLessThan> mtaskStream{mTaskGraphp.get()};
    यावत् (ध्रुव V3GraphVertex* ध्रुव vtxp = mtaskStream.nextp()) {
        ध्रुव LogicMTask* ध्रुव cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ध्रुव mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        यदि (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->vertexList().empty(), mTaskp,
                        "Entry and exit vertices should have no logic");
            अनुवर्त;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        यावत् (OrderMoveVertex* ध्रुव mVtxp = mTaskp->vertexList().unlinkFront()) {
            यदि (mVtxp->inEmpty()) serializer.addSeed(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        OrderMoveDomScope* prevDomScopep = nullptr;
        यावत् (OrderMoveVertex* ध्रुव mVtxp = serializer.getNext()) {
            // We only really care about logic vertices
            यदि (OrderLogicVertex* ध्रुव logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                OrderMoveDomScope* ध्रुव domScopep = &mVtxp->domScope();
                यदि (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&moveGraph), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ध्रुव execMTaskp = नूतन ExecMTask{execGraphp, scopep, emitter.getStmts()};
        यदि (!v3Global.opt.hierBlocks().empty()) {
            execMTaskp->threads(DpiThreadsVisitor::apply(execMTaskp));
        }
        ध्रुव तार्किक newEntry = logicMTaskToExecMTask.emplace(mTaskp, execMTaskp).second;
        UASSERT_OBJ(newEntry, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << tag << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ध्रुव callp = नूतन AstCCall{flp, execMTaskp->funcp()};
        callp->dtypeSetVoid();
        execGraphp->addCallsp(callp);

        // Add the dependency edges between ExecMTasks
        प्रति (ध्रुव V3GraphEdge& edge : mTaskp->inEdges()) {
            ध्रुव V3GraphVertex* fromVxp = edge.fromp();
            ध्रुव LogicMTask* ध्रुव fromp = fromVxp->as<ध्रुव LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            यदि (fromp == mTaskGraphp->entryp()) अनुवर्त;
            नूतन V3GraphEdge{depGraphp, logicMTaskToExecMTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    प्रति (V3GraphVertex* ध्रुव vtxp : moveGraph.vertices().unlinkable()) {
        यदि (!vtxp->as<OrderMoveVertex>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&moveGraph), vtxp);
        }
    }

    प्रत्यागम execGraphp;
}
