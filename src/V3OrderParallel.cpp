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
ساكن فراغ check_انکڑے_Hazards(ترتیب_MTask_گراف& m_Task_گراف) {
    // Expensive, so only with '--debug-partition'
    إذا (!m_Task_گراف.slowAsserts()) أرجع;

    // Order MTasks by their stable ids, so the report is deterministic
    بنية MTaskIdLessThan final {
        منطقي عامل()(ثابت LogicMTask* ap, ثابت LogicMTask* bp) ثابت { أرجع *ap < *bp; }
    };
    بنية VarInfo final {
        منطقي m_تکیا = خطأ;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ثابت VNUser1_وچ_Use user1_وچ_Use;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::سمتیہ<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    لكل (V3_گراف_راس& vtx : m_Task_گراف.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        لكل (ثابت ترتیب_Move_راس& mVtx : mtask.راس_List()) {
            ثابت ترتیب_Logic_راس* ثابت lVtxp = mVtx.logicp();
            إذا (!lVtxp) استمر;  // A variable vertex, which performs no access itself
            لكل (ثابت ترتیب_Logic_راس::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ثابت vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                إذا (!varInfo.m_تکیا) {
                    varInfo.m_تکیا = سچ;
                    vscps.ڈالو_پٹھ(vscp);
                }
                ثابت خودکار pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                إذا (!pair.دوجا && pair.پہلا->دوجا != acc.m_access) {
                    pair.پہلا->دوجا = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* پہلا_Hazardp = خالی_سوچک;  // First variable with a hazard, for the error below
    لكل (AstVarScope* ثابت vscp : vscps) {
        ثابت خودکار& byMTask = varInfos(vscp).m_byMTask;
        لكل (خودکار aIt = byMTask.begin(); aIt != byMTask.سرا(); ++aIt) {
            لكل (خودکار bIt = std::اگلا(aIt); bIt != byMTask.سرا(); ++bIt) {
                // Concurrent reads are not a hazard
                إذا (aIt->دوجا.isReadOnly() && bIt->دوجا.isReadOnly()) استمر;
                LogicMTask* ثابت ap = aIt->پہلا;
                LogicMTask* ثابت bp = bIt->پہلا;
                إذا (m_Task_گراف.رستہ_Exists(ap, bp, خالی_سوچک)) استمر;
                إذا (m_Task_گراف.رستہ_Exists(bp, ap, خالی_سوچک)) استمر;
                // LCOV_EXCL_START
                إذا (!پہلا_Hazardp) پہلا_Hazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->دوجا.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->دوجا.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    إذا (پہلا_Hazardp) پہلا_Hazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

ساكن std::unique_ptr<ترتیب_MTask_گراف> حصے_کرو(ترتیب_Move_گراف& move_گراف) {
    // Build the initial MTask graph. Initially, each MTask just wraps one OrderMoveVertex. We will
    // merge MTasks together and eventually each MTask will wrap a large number of OrderMoveVertex
    // (and the logic nodes therein).
    std::unique_ptr<ترتیب_MTask_گراف> mTaskGraphp = ترتیب_MTask_گراف::build(move_گراف);
    mTaskGraphp->hash_گراف_Debug("initial MTask graph");

    // Add edges to eliminate data hazards
    ترتیب_MTask_گراف::fix_انکڑے_Hazards(*mTaskGraphp);
    mTaskGraphp->hash_گراف_Debug("MTask graph after fixDataHazards()");

    // Merge MTask nodes together, repeatedly, until the critical path budget is reached. Coarsens
    // the graph, usually by several orders of magnitude. Some tests disable this for stability,
    // it should always be enabled in production.
    إذا (v3Global.opt.دھاگے_Coarsen()) {
        ثابت صحيح n_دھاگے = v3Global.opt.دھاگے();
        UASSERT(n_دھاگے >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ثابت uint64_t fudge_عدد = 3;
        ثابت uint64_t fudgeDen = 5;
        ثابت uint64_t حد_بندی = (mTaskGraphp->کل_لاگت() * fudge_عدد) / (n_دھاگے * fudgeDen);
        UINFO(4, "Partitioner set critical path limit = " << حد_بندی);

        ترتیب_MTask_گراف::contract(*mTaskGraphp, حد_بندی);
        mTaskGraphp->hash_گراف_Debug("MTask graph after contract()");
    }

    // Remove MTasks that have no logic in them, rerouting the edges
    mTaskGraphp->remove_خالی_تھاواں_MTasks();
    mTaskGraphp->hash_گراف_Debug("MTask graph after removeEmptyMTasks()");

    // Note this is OrderMTaskGraph::removeTransitiveEdges, which maintains graph consistency
    mTaskGraphp->remove_متعدی_کنارے();
    mTaskGraphp->hash_گراف_Debug("MTask graph after removeTransitiveEdges()");

    // Check for data hazards the partitioning left unordered
    check_انکڑے_Hazards(*mTaskGraphp);

    // Set OrderMoveVertex::userp to indicate the mtask it is part of.
    move_گراف.userClearVertices();
    لكل (V3_گراف_راس* ثابت vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = vtxp->as<LogicMTask>();
        ترتیب_Move_راس::List& راس_List = mtaskp->راس_List();
        طالما (ترتیب_Move_راس* ثابت mVtxp = راس_List.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    أرجع mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

صنف Dpi_دھاگے_Visitor final : عام VNVisitorConst {
    صحيح m_دھاگے = 1;  // Max number of threads used by this mtask

    // METHODS
    فراغ ویکھو(AstCFunc* nodep) override {
        m_دھاگے = std::max(m_دھاگے, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_گرہ_CCall* nodep) override { iterateConst(nodep->funcp()); }
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    واضح Dpi_دھاگے_Visitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~Dpi_دھاگے_Visitor() override = افتراضي;
    VL_UNCOPYABLE(Dpi_دھاگے_Visitor);

عام:
    // Number of threads occupied by the given MTask
    ساكن صحيح apply(ثابت ExecMTask* mTaskp) {
        أرجع Dpi_دھاگے_Visitor{mTaskp->funcp()}.m_دھاگے;
    }
};

//######################################################################
// Entry point

Ast_گرہ_Stmt* V3_ترتیب::createParallel(ترتیب_Move_گراف& move_گراف, ثابت std::string& ٹیگ,
                                     منطقي slow) {
    UINFO(2, "  Constructing parallel code for '" + ٹیگ + "'");

    // For nondeterminism debugging
    move_گراف.hash_گراف_Debug("V3Order::createParallel input OrderMoveGraph");
    move_گراف.ترتیب_گراف_2().hash_گراف_Debug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ثابت std::unique_ptr<ترتیب_MTask_گراف> mTaskGraphp = حصے_کرو(move_گراف);
    إذا (dump_گراف_Level() >= 9) move_گراف.dumpDotFilePrefixed(ٹیگ + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    لكل (V3_گراف_راس* ثابت vtxp : move_گراف.vertices().unlinkable()) {
        ترتیب_Move_راس* ثابت mVtxp = vtxp->as<ترتیب_Move_راس>();
        إذا (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->reroute_کنارے(&move_گراف);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_گراف), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    لكل (V3_گراف_راس& vtx : move_گراف.vertices()) {
        ترتیب_Move_راس* ثابت mVtxp = vtx.as<ترتیب_Move_راس>();
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->راس_List().link_پٹھ(mVtxp);
        // Remove edges crossing between MTasks
        لكل (V3_گراف_کنارا* ثابت edgep : mVtxp->باہرلا_کنارے().unlinkable()) {
            ثابت ترتیب_Move_راس* ثابت منزل_MVtxp = edgep->چوٹی()->as<ترتیب_Move_راس>();
            إذا (mtaskp != منزل_MVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    إذا (dump_گراف_Level() >= 9) move_گراف.dumpDotFilePrefixed(ٹیگ + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_سطر* ثابت flp = v3Global.rootp()->fileline();
    AstScope* ثابت scopep = v3Global.rootp()->چوٹی_Scopep()->scopep();
    Ast_Exec_گراف* ثابت execGraphp = جديد Ast_Exec_گراف{flp, ٹیگ, move_گراف.ترتیب_گراف_2().trigVscp()};
    V3_گراف* ثابت depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ثابت LogicMTask*, ExecMTask*> logic_MTask_منزل_Exec_MTask;
    ترتیب_Move_گراف_Serializer serializer{move_گراف};
    V3_ترتیب_CFunc_Emitter emitter{move_گراف.ترتیب_گراف_2().trigVscp(), ٹیگ, slow};
    // Sort LogicMTask vertices by their serial IDs.
    بنية MTaskVxIdLessThan final {
        منطقي عامل()(ثابت V3_گراف_راس* lhsp, ثابت V3_گراف_راس* rhsp) ثابت {
            أرجع lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    گراف_دھارا<MTaskVxIdLessThan> mtask_دھارا{mTaskGraphp.get()};
    طالما (ثابت V3_گراف_راس* ثابت vtxp = mtask_دھارا.nextp()) {
        ثابت LogicMTask* ثابت cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ثابت mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        إذا (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->راس_List().خالی_تھاواں(), mTaskp,
                        "Entry and exit vertices should have no logic");
            استمر;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        طالما (ترتیب_Move_راس* ثابت mVtxp = mTaskp->راس_List().unlinkFront()) {
            إذا (mVtxp->وچ_خالی_تھاواں()) serializer.جوڑو_بیج(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        ترتیب_Move_Dom_Scope* prevDomScopep = خالی_سوچک;
        طالما (ترتیب_Move_راس* ثابت mVtxp = serializer.get_اگلا()) {
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
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_گراف), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ثابت execMTaskp = جديد ExecMTask{execGraphp, scopep, emitter.getStmts()};
        إذا (!v3Global.opt.hier_بلاکاں().خالی_تھاواں()) {
            execMTaskp->دھاگے(Dpi_دھاگے_Visitor::apply(execMTaskp));
        }
        ثابت منطقي new_اندراج = logic_MTask_منزل_Exec_MTask.emplace(mTaskp, execMTaskp).دوجا;
        UASSERT_OBJ(new_اندراج, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << ٹیگ << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ثابت callp = جديد AstCCall{flp, execMTaskp->funcp()};
        callp->dtype_سیٹ_Void();
        execGraphp->جوڑو_Callsp(callp);

        // Add the dependency edges between ExecMTasks
        لكل (ثابت V3_گراف_کنارا& کنارا : mTaskp->وچ_کنارے()) {
            ثابت V3_گراف_راس* fromVxp = کنارا.fromp();
            ثابت LogicMTask* ثابت fromp = fromVxp->as<ثابت LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            إذا (fromp == mTaskGraphp->entryp()) استمر;
            جديد V3_گراف_کنارا{depGraphp, logic_MTask_منزل_Exec_MTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    لكل (V3_گراف_راس* ثابت vtxp : move_گراف.vertices().unlinkable()) {
        إذا (!vtxp->as<ترتیب_Move_راس>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_گراف), vtxp);
        }
    }

    أرجع execGraphp;
}
