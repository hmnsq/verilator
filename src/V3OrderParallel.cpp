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
ساكن فراغ check_معلومات_Hazards(ترتیب_MTask_ګراف& m_Task_ګراف) {
    // Expensive, so only with '--debug-partition'
    إذا (!m_Task_ګراف.slowAsserts()) أرجع;

    // Order MTasks by their stable ids, so the report is deterministic
    بنية MTaskIdLessThan final {
        منطقي چلوونکی()(ثابت LogicMTask* ap, ثابت LogicMTask* bp) ثابت { أرجع *ap < *bp; }
    };
    بنية VarInfo final {
        منطقي m_کتل_شوی = خطأ;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ثابت VNUser1_دننه_Use user1_دننه_Use;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::ویکټور<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    لكل (V3_ګراف_راس& vtx : m_Task_ګراف.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        لكل (ثابت ترتیب_Move_راس& mVtx : mtask.راس_List()) {
            ثابت ترتیب_Logic_راس* ثابت lVtxp = mVtx.logicp();
            إذا (!lVtxp) استمر;  // A variable vertex, which performs no access itself
            لكل (ثابت ترتیب_Logic_راس::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ثابت vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                إذا (!varInfo.m_کتل_شوی) {
                    varInfo.m_کتل_شوی = ریښتیا;
                    vscps.ټېل_شا(vscp);
                }
                ثابت خپلکاری pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                إذا (!pair.دوهم && pair.لومړی->دوهم != acc.m_access) {
                    pair.لومړی->دوهم = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* لومړی_Hazardp = تش_اشاره;  // First variable with a hazard, for the error below
    لكل (AstVarScope* ثابت vscp : vscps) {
        ثابت خپلکاری& byMTask = varInfos(vscp).m_byMTask;
        لكل (خپلکاری aIt = byMTask.begin(); aIt != byMTask.پای_ټکی(); ++aIt) {
            لكل (خپلکاری bIt = std::بل(aIt); bIt != byMTask.پای_ټکی(); ++bIt) {
                // Concurrent reads are not a hazard
                إذا (aIt->دوهم.isReadOnly() && bIt->دوهم.isReadOnly()) استمر;
                LogicMTask* ثابت ap = aIt->لومړی;
                LogicMTask* ثابت bp = bIt->لومړی;
                إذا (m_Task_ګراف.لار_Exists(ap, bp, تش_اشاره)) استمر;
                إذا (m_Task_ګراف.لار_Exists(bp, ap, تش_اشاره)) استمر;
                // LCOV_EXCL_START
                إذا (!لومړی_Hazardp) لومړی_Hazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->دوهم.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->دوهم.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    إذا (لومړی_Hazardp) لومړی_Hazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

ساكن std::unique_ptr<ترتیب_MTask_ګراف> برخه_کول(ترتیب_Move_ګراف& move_ګراف) {
    // Build the initial MTask graph. Initially, each MTask just wraps one OrderMoveVertex. We will
    // merge MTasks together and eventually each MTask will wrap a large number of OrderMoveVertex
    // (and the logic nodes therein).
    std::unique_ptr<ترتیب_MTask_ګراف> mTaskGraphp = ترتیب_MTask_ګراف::build(move_ګراف);
    mTaskGraphp->hash_ګراف_Debug("initial MTask graph");

    // Add edges to eliminate data hazards
    ترتیب_MTask_ګراف::fix_معلومات_Hazards(*mTaskGraphp);
    mTaskGraphp->hash_ګراف_Debug("MTask graph after fixDataHazards()");

    // Merge MTask nodes together, repeatedly, until the critical path budget is reached. Coarsens
    // the graph, usually by several orders of magnitude. Some tests disable this for stability,
    // it should always be enabled in production.
    إذا (v3Global.opt.تارونه_Coarsen()) {
        ثابت صحيح n_تارونه = v3Global.opt.تارونه();
        UASSERT(n_تارونه >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ثابت uint64_t fudge_عدد = 3;
        ثابت uint64_t fudgeDen = 5;
        ثابت uint64_t حد = (mTaskGraphp->ټول_لګښت() * fudge_عدد) / (n_تارونه * fudgeDen);
        UINFO(4, "Partitioner set critical path limit = " << حد);

        ترتیب_MTask_ګراف::contract(*mTaskGraphp, حد);
        mTaskGraphp->hash_ګراف_Debug("MTask graph after contract()");
    }

    // Remove MTasks that have no logic in them, rerouting the edges
    mTaskGraphp->remove_تشه_MTasks();
    mTaskGraphp->hash_ګراف_Debug("MTask graph after removeEmptyMTasks()");

    // Note this is OrderMTaskGraph::removeTransitiveEdges, which maintains graph consistency
    mTaskGraphp->remove_متعدي_څنډې();
    mTaskGraphp->hash_ګراف_Debug("MTask graph after removeTransitiveEdges()");

    // Check for data hazards the partitioning left unordered
    check_معلومات_Hazards(*mTaskGraphp);

    // Set OrderMoveVertex::userp to indicate the mtask it is part of.
    move_ګراف.userClearVertices();
    لكل (V3_ګراف_راس* ثابت vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = vtxp->as<LogicMTask>();
        ترتیب_Move_راس::List& راس_List = mtaskp->راس_List();
        طالما (ترتیب_Move_راس* ثابت mVtxp = راس_List.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    أرجع mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

صنف Dpi_تارونه_Visitor final : عام VNVisitorConst {
    صحيح m_تارونه = 1;  // Max number of threads used by this mtask

    // METHODS
    فراغ لیدنه(AstCFunc* nodep) override {
        m_تارونه = std::max(m_تارونه, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    فراغ لیدنه(Ast_غوټه_CCall* nodep) override { iterateConst(nodep->funcp()); }
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    څرګند Dpi_تارونه_Visitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~Dpi_تارونه_Visitor() override = افتراضي;
    VL_UNCOPYABLE(Dpi_تارونه_Visitor);

عام:
    // Number of threads occupied by the given MTask
    ساكن صحيح apply(ثابت ExecMTask* mTaskp) {
        أرجع Dpi_تارونه_Visitor{mTaskp->funcp()}.m_تارونه;
    }
};

//######################################################################
// Entry point

Ast_غوټه_Stmt* V3_ترتیب::createParallel(ترتیب_Move_ګراف& move_ګراف, ثابت std::string& نښان,
                                     منطقي slow) {
    UINFO(2, "  Constructing parallel code for '" + نښان + "'");

    // For nondeterminism debugging
    move_ګراف.hash_ګراف_Debug("V3Order::createParallel input OrderMoveGraph");
    move_ګراف.ترتیب_ګراف_2().hash_ګراف_Debug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ثابت std::unique_ptr<ترتیب_MTask_ګراف> mTaskGraphp = برخه_کول(move_ګراف);
    إذا (dump_ګراف_Level() >= 9) move_ګراف.dumpDotFilePrefixed(نښان + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    لكل (V3_ګراف_راس* ثابت vtxp : move_ګراف.vertices().unlinkable()) {
        ترتیب_Move_راس* ثابت mVtxp = vtxp->as<ترتیب_Move_راس>();
        إذا (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->reroute_څنډې(&move_ګراف);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_ګراف), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    لكل (V3_ګراف_راس& vtx : move_ګراف.vertices()) {
        ترتیب_Move_راس* ثابت mVtxp = vtx.as<ترتیب_Move_راس>();
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->راس_List().link_شا(mVtxp);
        // Remove edges crossing between MTasks
        لكل (V3_ګراف_څنډه* ثابت edgep : mVtxp->بهر_څنډې().unlinkable()) {
            ثابت ترتیب_Move_راس* ثابت منزل_MVtxp = edgep->سر()->as<ترتیب_Move_راس>();
            إذا (mtaskp != منزل_MVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    إذا (dump_ګراف_Level() >= 9) move_ګراف.dumpDotFilePrefixed(نښان + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_کرښه* ثابت flp = v3Global.rootp()->fileline();
    AstScope* ثابت scopep = v3Global.rootp()->سر_Scopep()->scopep();
    Ast_Exec_ګراف* ثابت execGraphp = جديد Ast_Exec_ګراف{flp, نښان, move_ګراف.ترتیب_ګراف_2().trigVscp()};
    V3_ګراف* ثابت depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ثابت LogicMTask*, ExecMTask*> logic_MTask_منزل_Exec_MTask;
    ترتیب_Move_ګراف_Serializer serializer{move_ګراف};
    V3_ترتیب_CFunc_Emitter emitter{move_ګراف.ترتیب_ګراف_2().trigVscp(), نښان, slow};
    // Sort LogicMTask vertices by their serial IDs.
    بنية MTaskVxIdLessThan final {
        منطقي چلوونکی()(ثابت V3_ګراف_راس* lhsp, ثابت V3_ګراف_راس* rhsp) ثابت {
            أرجع lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    ګراف_بهیر<MTaskVxIdLessThan> mtask_بهیر{mTaskGraphp.get()};
    طالما (ثابت V3_ګراف_راس* ثابت vtxp = mtask_بهیر.nextp()) {
        ثابت LogicMTask* ثابت cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ثابت mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        إذا (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->راس_List().تشه(), mTaskp,
                        "Entry and exit vertices should have no logic");
            استمر;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        طالما (ترتیب_Move_راس* ثابت mVtxp = mTaskp->راس_List().unlinkFront()) {
            إذا (mVtxp->دننه_تشه()) serializer.جمع_کړه_تخم(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        ترتیب_Move_Dom_Scope* prevDomScopep = تش_اشاره;
        طالما (ترتیب_Move_راس* ثابت mVtxp = serializer.get_بل()) {
            // We only really care about logic vertices
            إذا (ترتیب_Logic_راس* ثابت logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                ترتیب_Move_Dom_Scope* ثابت domScopep = &mVtxp->domScope();
                إذا (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_ګراف), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ثابت execMTaskp = جديد ExecMTask{execGraphp, scopep, emitter.getStmts()};
        إذا (!v3Global.opt.hier_بلاکونه().تشه()) {
            execMTaskp->تارونه(Dpi_تارونه_Visitor::apply(execMTaskp));
        }
        ثابت منطقي new_ثبت = logic_MTask_منزل_Exec_MTask.emplace(mTaskp, execMTaskp).دوهم;
        UASSERT_OBJ(new_ثبت, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << نښان << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ثابت callp = جديد AstCCall{flp, execMTaskp->funcp()};
        callp->dtype_سېټ_Void();
        execGraphp->جمع_کړه_Callsp(callp);

        // Add the dependency edges between ExecMTasks
        لكل (ثابت V3_ګراف_څنډه& څنډه : mTaskp->دننه_څنډې()) {
            ثابت V3_ګراف_راس* fromVxp = څنډه.fromp();
            ثابت LogicMTask* ثابت fromp = fromVxp->as<ثابت LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            إذا (fromp == mTaskGraphp->entryp()) استمر;
            جديد V3_ګراف_څنډه{depGraphp, logic_MTask_منزل_Exec_MTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    لكل (V3_ګراف_راس* ثابت vtxp : move_ګراف.vertices().unlinkable()) {
        إذا (!vtxp->as<ترتیب_Move_راس>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_ګراف), vtxp);
        }
    }

    أرجع execGraphp;
}
