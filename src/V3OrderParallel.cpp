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
ساكن فراغ check_معلومات_Hazards(ترتيب_MTask_گراف& m_Task_گراف) {
    // Expensive, so only with '--debug-partition'
    إذا (!m_Task_گراف.slowAsserts()) أرجع;

    // Order MTasks by their stable ids, so the report is deterministic
    بنية MTaskIdLessThan final {
        منطقي عامل()(ثابت LogicMTask* ap, ثابت LogicMTask* bp) ثابت { أرجع *ap < *bp; }
    };
    بنية VarInfo final {
        منطقي m_ڏٺل = خطأ;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ثابت VNUser1_اندريون_Use user1_اندريون_Use;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::ويڪٽر<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    لكل (V3_گراف_راس& vtx : m_Task_گراف.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        لكل (ثابت ترتيب_Move_راس& mVtx : mtask.راس_List()) {
            ثابت ترتيب_Logic_راس* ثابت lVtxp = mVtx.logicp();
            إذا (!lVtxp) استمر;  // A variable vertex, which performs no access itself
            لكل (ثابت ترتيب_Logic_راس::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ثابت vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                إذا (!varInfo.m_ڏٺل) {
                    varInfo.m_ڏٺل = سچ;
                    vscps.ڌڪو_پٺ(vscp);
                }
                ثابت خودڪار pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                إذا (!pair.ٻيو && pair.پهريون->ٻيو != acc.m_access) {
                    pair.پهريون->ٻيو = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* پهريون_Hazardp = خالي_اشارو;  // First variable with a hazard, for the error below
    لكل (AstVarScope* ثابت vscp : vscps) {
        ثابت خودڪار& byMTask = varInfos(vscp).m_byMTask;
        لكل (خودڪار aIt = byMTask.begin(); aIt != byMTask.پڇاڙي(); ++aIt) {
            لكل (خودڪار bIt = std::ايندڙ(aIt); bIt != byMTask.پڇاڙي(); ++bIt) {
                // Concurrent reads are not a hazard
                إذا (aIt->ٻيو.isReadOnly() && bIt->ٻيو.isReadOnly()) استمر;
                LogicMTask* ثابت ap = aIt->پهريون;
                LogicMTask* ثابت bp = bIt->پهريون;
                إذا (m_Task_گراف.رستو_Exists(ap, bp, خالي_اشارو)) استمر;
                إذا (m_Task_گراف.رستو_Exists(bp, ap, خالي_اشارو)) استمر;
                // LCOV_EXCL_START
                إذا (!پهريون_Hazardp) پهريون_Hazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->ٻيو.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->ٻيو.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    إذا (پهريون_Hazardp) پهريون_Hazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

ساكن std::unique_ptr<ترتيب_MTask_گراف> حصا_ڪريو(ترتيب_Move_گراف& move_گراف) {
    // Build the initial MTask graph. Initially, each MTask just wraps one OrderMoveVertex. We will
    // merge MTasks together and eventually each MTask will wrap a large number of OrderMoveVertex
    // (and the logic nodes therein).
    std::unique_ptr<ترتيب_MTask_گراف> mTaskGraphp = ترتيب_MTask_گراف::build(move_گراف);
    mTaskGraphp->hash_گراف_Debug("initial MTask graph");

    // Add edges to eliminate data hazards
    ترتيب_MTask_گراف::fix_معلومات_Hazards(*mTaskGraphp);
    mTaskGraphp->hash_گراف_Debug("MTask graph after fixDataHazards()");

    // Merge MTask nodes together, repeatedly, until the critical path budget is reached. Coarsens
    // the graph, usually by several orders of magnitude. Some tests disable this for stability,
    // it should always be enabled in production.
    إذا (v3Global.opt.ڌاڳا_Coarsen()) {
        ثابت صحيح n_ڌاڳا = v3Global.opt.ڌاڳا();
        UASSERT(n_ڌاڳا >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ثابت uint64_t fudge_انگ = 3;
        ثابت uint64_t fudgeDen = 5;
        ثابت uint64_t حد_بندي = (mTaskGraphp->ڪل_لاڳت() * fudge_انگ) / (n_ڌاڳا * fudgeDen);
        UINFO(4, "Partitioner set critical path limit = " << حد_بندي);

        ترتيب_MTask_گراف::contract(*mTaskGraphp, حد_بندي);
        mTaskGraphp->hash_گراف_Debug("MTask graph after contract()");
    }

    // Remove MTasks that have no logic in them, rerouting the edges
    mTaskGraphp->remove_خالي_جايون_MTasks();
    mTaskGraphp->hash_گراف_Debug("MTask graph after removeEmptyMTasks()");

    // Note this is OrderMTaskGraph::removeTransitiveEdges, which maintains graph consistency
    mTaskGraphp->remove_متعدي_ڪنارا();
    mTaskGraphp->hash_گراف_Debug("MTask graph after removeTransitiveEdges()");

    // Check for data hazards the partitioning left unordered
    check_معلومات_Hazards(*mTaskGraphp);

    // Set OrderMoveVertex::userp to indicate the mtask it is part of.
    move_گراف.userClearVertices();
    لكل (V3_گراف_راس* ثابت vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ثابت mtaskp = vtxp->as<LogicMTask>();
        ترتيب_Move_راس::List& راس_List = mtaskp->راس_List();
        طالما (ترتيب_Move_راس* ثابت mVtxp = راس_List.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    أرجع mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

صنف Dpi_ڌاڳا_Visitor final : عام VNVisitorConst {
    صحيح m_ڌاڳا = 1;  // Max number of threads used by this mtask

    // METHODS
    فراغ گهمو(AstCFunc* nodep) override {
        m_ڌاڳا = std::max(m_ڌاڳا, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_CCall* nodep) override { iterateConst(nodep->funcp()); }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    واضح Dpi_ڌاڳا_Visitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~Dpi_ڌاڳا_Visitor() override = افتراضي;
    VL_UNCOPYABLE(Dpi_ڌاڳا_Visitor);

عام:
    // Number of threads occupied by the given MTask
    ساكن صحيح apply(ثابت ExecMTask* mTaskp) {
        أرجع Dpi_ڌاڳا_Visitor{mTaskp->funcp()}.m_ڌاڳا;
    }
};

//######################################################################
// Entry point

Ast_ڳنڍ_Stmt* V3_ترتيب::createParallel(ترتيب_Move_گراف& move_گراف, ثابت std::string& ٽيگ,
                                     منطقي slow) {
    UINFO(2, "  Constructing parallel code for '" + ٽيگ + "'");

    // For nondeterminism debugging
    move_گراف.hash_گراف_Debug("V3Order::createParallel input OrderMoveGraph");
    move_گراف.ترتيب_گراف_2().hash_گراف_Debug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ثابت std::unique_ptr<ترتيب_MTask_گراف> mTaskGraphp = حصا_ڪريو(move_گراف);
    إذا (dump_گراف_Level() >= 9) move_گراف.dumpDotFilePrefixed(ٽيگ + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    لكل (V3_گراف_راس* ثابت vtxp : move_گراف.vertices().unlinkable()) {
        ترتيب_Move_راس* ثابت mVtxp = vtxp->as<ترتيب_Move_راس>();
        إذا (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->reroute_ڪنارا(&move_گراف);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_گراف), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    لكل (V3_گراف_راس& vtx : move_گراف.vertices()) {
        ترتيب_Move_راس* ثابت mVtxp = vtx.as<ترتيب_Move_راس>();
        LogicMTask* ثابت mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->راس_List().link_پٺ(mVtxp);
        // Remove edges crossing between MTasks
        لكل (V3_گراف_ڪنارو* ثابت edgep : mVtxp->ٻاهريون_ڪنارا().unlinkable()) {
            ثابت ترتيب_Move_راس* ثابت منزل_MVtxp = edgep->چوٽي()->as<ترتيب_Move_راس>();
            إذا (mtaskp != منزل_MVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    إذا (dump_گراف_Level() >= 9) move_گراف.dumpDotFilePrefixed(ٽيگ + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_سٽ* ثابت flp = v3Global.rootp()->fileline();
    AstScope* ثابت scopep = v3Global.rootp()->چوٽي_Scopep()->scopep();
    Ast_Exec_گراف* ثابت execGraphp = جديد Ast_Exec_گراف{flp, ٽيگ, move_گراف.ترتيب_گراف_2().trigVscp()};
    V3_گراف* ثابت depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ثابت LogicMTask*, ExecMTask*> logic_MTask_منزل_Exec_MTask;
    ترتيب_Move_گراف_Serializer serializer{move_گراف};
    V3_ترتيب_CFunc_Emitter emitter{move_گراف.ترتيب_گراف_2().trigVscp(), ٽيگ, slow};
    // Sort LogicMTask vertices by their serial IDs.
    بنية MTaskVxIdLessThan final {
        منطقي عامل()(ثابت V3_گراف_راس* lhsp, ثابت V3_گراف_راس* rhsp) ثابت {
            أرجع lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    گراف_وهڪرو<MTaskVxIdLessThan> mtask_وهڪرو{mTaskGraphp.get()};
    طالما (ثابت V3_گراف_راس* ثابت vtxp = mtask_وهڪرو.nextp()) {
        ثابت LogicMTask* ثابت cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ثابت mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        إذا (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->راس_List().خالي_جايون(), mTaskp,
                        "Entry and exit vertices should have no logic");
            استمر;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        طالما (ترتيب_Move_راس* ثابت mVtxp = mTaskp->راس_List().unlinkFront()) {
            إذا (mVtxp->اندريون_خالي_جايون()) serializer.جوڙيو_ٻج(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        ترتيب_Move_Dom_Scope* prevDomScopep = خالي_اشارو;
        طالما (ترتيب_Move_راس* ثابت mVtxp = serializer.get_ايندڙ()) {
            // We only really care about logic vertices
            إذا (ترتيب_Logic_راس* ثابت logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                ترتيب_Move_Dom_Scope* ثابت domScopep = &mVtxp->domScope();
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
        إذا (!v3Global.opt.hier_بلاڪون().خالي_جايون()) {
            execMTaskp->ڌاڳا(Dpi_ڌاڳا_Visitor::apply(execMTaskp));
        }
        ثابت منطقي new_داخلا = logic_MTask_منزل_Exec_MTask.emplace(mTaskp, execMTaskp).ٻيو;
        UASSERT_OBJ(new_داخلا, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << ٽيگ << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ثابت callp = جديد AstCCall{flp, execMTaskp->funcp()};
        callp->dtype_سيٽ_Void();
        execGraphp->جوڙيو_Callsp(callp);

        // Add the dependency edges between ExecMTasks
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : mTaskp->اندريون_ڪنارا()) {
            ثابت V3_گراف_راس* fromVxp = ڪنارو.fromp();
            ثابت LogicMTask* ثابت fromp = fromVxp->as<ثابت LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            إذا (fromp == mTaskGraphp->entryp()) استمر;
            جديد V3_گراف_ڪنارو{depGraphp, logic_MTask_منزل_Exec_MTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    لكل (V3_گراف_راس* ثابت vtxp : move_گراف.vertices().unlinkable()) {
        إذا (!vtxp->as<ترتيب_Move_راس>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_گراف), vtxp);
        }
    }

    أرجع execGraphp;
}
