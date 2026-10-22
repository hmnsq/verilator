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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3ExecGraph.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3GraphStream.h"
#అన్తర్భావ "V3OrderCFuncEmitter.h"
#అన్తర్భావ "V3OrderInternal.h"
#అన్తర్భావ "V3OrderMTaskGraph.h"

#అన్తర్భావ <map>
#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
స్థిర శూన్య check_దత్తాంశం_Hazards(క్రమం_MTask_లేఖాచిత్రం& m_Task_లేఖాచిత్రం) {
    // Expensive, so only with '--debug-partition'
    ఒకవేళ (!m_Task_లేఖాచిత్రం.slowAsserts()) తిరిగి;

    // Order MTasks by their stable ids, so the report is deterministic
    నిర్మాణం MTaskIdLessThan final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి LogicMTask* ap, స్థిరరాశి LogicMTask* bp) స్థిరరాశి { తిరిగి *ap < *bp; }
    };
    నిర్మాణం VarInfo final {
        తార్కిక m_చూసిన = అబద్ధం;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::సదిశ<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_Task_లేఖాచిత్రం.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        కొరకు (స్థిరరాశి క్రమం_Move_శీర్షం& mVtx : mtask.శీర్షం_List()) {
            స్థిరరాశి క్రమం_Logic_శీర్షం* స్థిరరాశి lVtxp = mVtx.logicp();
            ఒకవేళ (!lVtxp) కొనసాగు;  // A variable vertex, which performs no access itself
            కొరకు (స్థిరరాశి క్రమం_Logic_శీర్షం::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* స్థిరరాశి vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                ఒకవేళ (!varInfo.m_చూసిన) {
                    varInfo.m_చూసిన = నిజం;
                    vscps.నెట్టు_వెనుక(vscp);
                }
                స్థిరరాశి స్వయంచాలక pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                ఒకవేళ (!pair.రెండవ && pair.మొదటి->రెండవ != acc.m_access) {
                    pair.మొదటి->రెండవ = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* మొదటి_Hazardp = శూన్యసూచిక;  // First variable with a hazard, for the error below
    కొరకు (AstVarScope* స్థిరరాశి vscp : vscps) {
        స్థిరరాశి స్వయంచాలక& byMTask = varInfos(vscp).m_byMTask;
        కొరకు (స్వయంచాలక aIt = byMTask.begin(); aIt != byMTask.చివర(); ++aIt) {
            కొరకు (స్వయంచాలక bIt = std::తదుపరి(aIt); bIt != byMTask.చివర(); ++bIt) {
                // Concurrent reads are not a hazard
                ఒకవేళ (aIt->రెండవ.isReadOnly() && bIt->రెండవ.isReadOnly()) కొనసాగు;
                LogicMTask* స్థిరరాశి ap = aIt->మొదటి;
                LogicMTask* స్థిరరాశి bp = bIt->మొదటి;
                ఒకవేళ (m_Task_లేఖాచిత్రం.మార్గం_Exists(ap, bp, శూన్యసూచిక)) కొనసాగు;
                ఒకవేళ (m_Task_లేఖాచిత్రం.మార్గం_Exists(bp, ap, శూన్యసూచిక)) కొనసాగు;
                // LCOV_EXCL_START
                ఒకవేళ (!మొదటి_Hazardp) మొదటి_Hazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->రెండవ.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->రెండవ.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    ఒకవేళ (మొదటి_Hazardp) మొదటి_Hazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

స్థిర std::unique_ptr<క్రమం_MTask_లేఖాచిత్రం> విభజన(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం) {
    // Build the initial MTask graph. Initially, each MTask just wraps one OrderMoveVertex. We will
    // merge MTasks together and eventually each MTask will wrap a large number of OrderMoveVertex
    // (and the logic nodes therein).
    std::unique_ptr<క్రమం_MTask_లేఖాచిత్రం> mTaskGraphp = క్రమం_MTask_లేఖాచిత్రం::build(move_లేఖాచిత్రం);
    mTaskGraphp->hash_లేఖాచిత్రం_Debug("initial MTask graph");

    // Add edges to eliminate data hazards
    క్రమం_MTask_లేఖాచిత్రం::fix_దత్తాంశం_Hazards(*mTaskGraphp);
    mTaskGraphp->hash_లేఖాచిత్రం_Debug("MTask graph after fixDataHazards()");

    // Merge MTask nodes together, repeatedly, until the critical path budget is reached. Coarsens
    // the graph, usually by several orders of magnitude. Some tests disable this for stability,
    // it should always be enabled in production.
    ఒకవేళ (v3Global.opt.దారాలు_Coarsen()) {
        స్థిరరాశి పూర్ణాంకం n_దారాలు = v3Global.opt.దారాలు();
        UASSERT(n_దారాలు >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        స్థిరరాశి uint64_t fudge_అంకె = 3;
        స్థిరరాశి uint64_t fudgeDen = 5;
        స్థిరరాశి uint64_t హద్దు = (mTaskGraphp->మొత్తం_ఖర్చు() * fudge_అంకె) / (n_దారాలు * fudgeDen);
        UINFO(4, "Partitioner set critical path limit = " << హద్దు);

        క్రమం_MTask_లేఖాచిత్రం::contract(*mTaskGraphp, హద్దు);
        mTaskGraphp->hash_లేఖాచిత్రం_Debug("MTask graph after contract()");
    }

    // Remove MTasks that have no logic in them, rerouting the edges
    mTaskGraphp->remove_ఖాళీలు_MTasks();
    mTaskGraphp->hash_లేఖాచిత్రం_Debug("MTask graph after removeEmptyMTasks()");

    // Note this is OrderMTaskGraph::removeTransitiveEdges, which maintains graph consistency
    mTaskGraphp->remove_సంక్రమణ_అంచులు();
    mTaskGraphp->hash_లేఖాచిత్రం_Debug("MTask graph after removeTransitiveEdges()");

    // Check for data hazards the partitioning left unordered
    check_దత్తాంశం_Hazards(*mTaskGraphp);

    // Set OrderMoveVertex::userp to indicate the mtask it is part of.
    move_లేఖాచిత్రం.userClearVertices();
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* స్థిరరాశి mtaskp = vtxp->as<LogicMTask>();
        క్రమం_Move_శీర్షం::List& శీర్షం_List = mtaskp->శీర్షం_List();
        అయితే (క్రమం_Move_శీర్షం* స్థిరరాశి mVtxp = శీర్షం_List.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    తిరిగి mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

తరగతి Dpi_దారాలు_Visitor final : బహిరంగ VNVisitorConst {
    పూర్ణాంకం m_దారాలు = 1;  // Max number of threads used by this mtask

    // METHODS
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        m_దారాలు = std::max(m_దారాలు, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override { iterateConst(nodep->funcp()); }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    స్పష్టమైన Dpi_దారాలు_Visitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~Dpi_దారాలు_Visitor() override = అప్రమేయ;
    VL_UNCOPYABLE(Dpi_దారాలు_Visitor);

బహిరంగ:
    // Number of threads occupied by the given MTask
    స్థిర పూర్ణాంకం apply(స్థిరరాశి ExecMTask* mTaskp) {
        తిరిగి Dpi_దారాలు_Visitor{mTaskp->funcp()}.m_దారాలు;
    }
};

//######################################################################
// Entry point

Ast_కణుపు_Stmt* V3_క్రమం::createParallel(క్రమం_Move_లేఖాచిత్రం& move_లేఖాచిత్రం, స్థిరరాశి std::string& ట్యాగ్,
                                     తార్కిక slow) {
    UINFO(2, "  Constructing parallel code for '" + ట్యాగ్ + "'");

    // For nondeterminism debugging
    move_లేఖాచిత్రం.hash_లేఖాచిత్రం_Debug("V3Order::createParallel input OrderMoveGraph");
    move_లేఖాచిత్రం.క్రమం_లేఖాచిత్రం_2().hash_లేఖాచిత్రం_Debug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    స్థిరరాశి std::unique_ptr<క్రమం_MTask_లేఖాచిత్రం> mTaskGraphp = విభజన(move_లేఖాచిత్రం);
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) move_లేఖాచిత్రం.dumpDotFilePrefixed(ట్యాగ్ + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : move_లేఖాచిత్రం.vertices().unlinkable()) {
        క్రమం_Move_శీర్షం* స్థిరరాశి mVtxp = vtxp->as<క్రమం_Move_శీర్షం>();
        ఒకవేళ (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->reroute_అంచులు(&move_లేఖాచిత్రం);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_లేఖాచిత్రం), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : move_లేఖాచిత్రం.vertices()) {
        క్రమం_Move_శీర్షం* స్థిరరాశి mVtxp = vtx.as<క్రమం_Move_శీర్షం>();
        LogicMTask* స్థిరరాశి mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->శీర్షం_List().link_వెనుక(mVtxp);
        // Remove edges crossing between MTasks
        కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : mVtxp->బహిర్గతం_అంచులు().unlinkable()) {
            స్థిరరాశి క్రమం_Move_శీర్షం* స్థిరరాశి గమ్యం_MVtxp = edgep->శిఖరం()->as<క్రమం_Move_శీర్షం>();
            ఒకవేళ (mtaskp != గమ్యం_MVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) move_లేఖాచిత్రం.dumpDotFilePrefixed(ట్యాగ్ + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_పంక్తి* స్థిరరాశి flp = v3Global.rootp()->fileline();
    AstScope* స్థిరరాశి scopep = v3Global.rootp()->శిఖరం_Scopep()->scopep();
    Ast_Exec_లేఖాచిత్రం* స్థిరరాశి execGraphp = కొత్త Ast_Exec_లేఖాచిత్రం{flp, ట్యాగ్, move_లేఖాచిత్రం.క్రమం_లేఖాచిత్రం_2().trigVscp()};
    V3_లేఖాచిత్రం* స్థిరరాశి depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<స్థిరరాశి LogicMTask*, ExecMTask*> logic_MTask_గమ్యం_Exec_MTask;
    క్రమం_Move_లేఖాచిత్రం_Serializer serializer{move_లేఖాచిత్రం};
    V3_క్రమం_CFunc_Emitter emitter{move_లేఖాచిత్రం.క్రమం_లేఖాచిత్రం_2().trigVscp(), ట్యాగ్, slow};
    // Sort LogicMTask vertices by their serial IDs.
    నిర్మాణం MTaskVxIdLessThan final {
        తార్కిక క్రియాకారకం()(స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* lhsp, స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* rhsp) స్థిరరాశి {
            తిరిగి lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    లేఖాచిత్రం_ప్రవాహం<MTaskVxIdLessThan> mtask_ప్రవాహం{mTaskGraphp.get()};
    అయితే (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp = mtask_ప్రవాహం.nextp()) {
        స్థిరరాశి LogicMTask* స్థిరరాశి cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* స్థిరరాశి mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        ఒకవేళ (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->శీర్షం_List().ఖాళీలు(), mTaskp,
                        "Entry and exit vertices should have no logic");
            కొనసాగు;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        అయితే (క్రమం_Move_శీర్షం* స్థిరరాశి mVtxp = mTaskp->శీర్షం_List().unlinkFront()) {
            ఒకవేళ (mVtxp->లోపల_ఖాళీలు()) serializer.కూడు_విత్తనం(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        క్రమం_Move_Dom_Scope* prevDomScopep = శూన్యసూచిక;
        అయితే (క్రమం_Move_శీర్షం* స్థిరరాశి mVtxp = serializer.get_తదుపరి()) {
            // We only really care about logic vertices
            ఒకవేళ (క్రమం_Logic_శీర్షం* స్థిరరాశి logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                క్రమం_Move_Dom_Scope* స్థిరరాశి domScopep = &mVtxp->domScope();
                ఒకవేళ (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_లేఖాచిత్రం), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* స్థిరరాశి execMTaskp = కొత్త ExecMTask{execGraphp, scopep, emitter.getStmts()};
        ఒకవేళ (!v3Global.opt.hier_ఖండాలు().ఖాళీలు()) {
            execMTaskp->దారాలు(Dpi_దారాలు_Visitor::apply(execMTaskp));
        }
        స్థిరరాశి తార్కిక new_నమోదు = logic_MTask_గమ్యం_Exec_MTask.emplace(mTaskp, execMTaskp).రెండవ;
        UASSERT_OBJ(new_నమోదు, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << ట్యాగ్ << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{flp, execMTaskp->funcp()};
        callp->dtype_సమితి_Void();
        execGraphp->కూడు_Callsp(callp);

        // Add the dependency edges between ExecMTasks
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : mTaskp->లోపల_అంచులు()) {
            స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం* fromVxp = అంచు.fromp();
            స్థిరరాశి LogicMTask* స్థిరరాశి fromp = fromVxp->as<స్థిరరాశి LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            ఒకవేళ (fromp == mTaskGraphp->entryp()) కొనసాగు;
            కొత్త V3_లేఖాచిత్రం_అంచు{depGraphp, logic_MTask_గమ్యం_Exec_MTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : move_లేఖాచిత్రం.vertices().unlinkable()) {
        ఒకవేళ (!vtxp->as<క్రమం_Move_శీర్షం>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_లేఖాచిత్రం), vtxp);
        }
    }

    తిరిగి execGraphp;
}
