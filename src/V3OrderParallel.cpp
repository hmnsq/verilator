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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ExecGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3GraphStream.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderCFuncEmitter.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderInternal.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderMTaskGraph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
ਸਥਿਰ ਖਾਲੀ check_ਅੰਕੜੇ_Hazards(ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ& m_Task_ਗ੍ਰਾਫ) {
    // Expensive, so only with '--debug-partition'
    ਜੇ (!m_Task_ਗ੍ਰਾਫ.slowAsserts()) ਵਾਪਸ;

    // Order MTasks by their stable ids, so the report is deterministic
    ਬਣਤਰ MTaskIdLessThan final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ LogicMTask* ap, ਅਚਲ LogicMTask* bp) ਅਚਲ { ਵਾਪਸ *ap < *bp; }
    };
    ਬਣਤਰ VarInfo final {
        ਤਾਰਕਿਕ m_ਵੇਖਿਆ = ਝੂਠ;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::ਸਦਿਸ਼<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_Task_ਗ੍ਰਾਫ.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        ਲਈ (ਅਚਲ ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼& mVtx : mtask.ਸ਼ੀਰਸ਼_List()) {
            ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp = mVtx.logicp();
            ਜੇ (!lVtxp) ਜਾਰੀ;  // A variable vertex, which performs no access itself
            ਲਈ (ਅਚਲ ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ਅਚਲ vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                ਜੇ (!varInfo.m_ਵੇਖਿਆ) {
                    varInfo.m_ਵੇਖਿਆ = ਸੱਚ;
                    vscps.ਧੱਕੋ_ਪਿੱਠ(vscp);
                }
                ਅਚਲ ਸਵੈਚਾਲਿਤ pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                ਜੇ (!pair.ਦੂਜਾ && pair.ਪਹਿਲਾ->ਦੂਜਾ != acc.m_access) {
                    pair.ਪਹਿਲਾ->ਦੂਜਾ = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* ਪਹਿਲਾ_Hazardp = ਖਾਲੀ_ਸੂਚਕ;  // First variable with a hazard, for the error below
    ਲਈ (AstVarScope* ਅਚਲ vscp : vscps) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ& byMTask = varInfos(vscp).m_byMTask;
        ਲਈ (ਸਵੈਚਾਲਿਤ aIt = byMTask.begin(); aIt != byMTask.ਸਿਰਾ(); ++aIt) {
            ਲਈ (ਸਵੈਚਾਲਿਤ bIt = std::ਅਗਲਾ(aIt); bIt != byMTask.ਸਿਰਾ(); ++bIt) {
                // Concurrent reads are not a hazard
                ਜੇ (aIt->ਦੂਜਾ.isReadOnly() && bIt->ਦੂਜਾ.isReadOnly()) ਜਾਰੀ;
                LogicMTask* ਅਚਲ ap = aIt->ਪਹਿਲਾ;
                LogicMTask* ਅਚਲ bp = bIt->ਪਹਿਲਾ;
                ਜੇ (m_Task_ਗ੍ਰਾਫ.ਰਸਤਾ_Exists(ap, bp, ਖਾਲੀ_ਸੂਚਕ)) ਜਾਰੀ;
                ਜੇ (m_Task_ਗ੍ਰਾਫ.ਰਸਤਾ_Exists(bp, ap, ਖਾਲੀ_ਸੂਚਕ)) ਜਾਰੀ;
                // LCOV_EXCL_START
                ਜੇ (!ਪਹਿਲਾ_Hazardp) ਪਹਿਲਾ_Hazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->ਦੂਜਾ.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->ਦੂਜਾ.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    ਜੇ (ਪਹਿਲਾ_Hazardp) ਪਹਿਲਾ_Hazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

ਸਥਿਰ std::unique_ptr<ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ> ਹਿੱਸੇ_ਕਰੋ(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ) {
    // Build the initial MTask graph. Initially, each MTask just wraps one OrderMoveVertex. We will
    // merge MTasks together and eventually each MTask will wrap a large number of OrderMoveVertex
    // (and the logic nodes therein).
    std::unique_ptr<ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ> mTaskGraphp = ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::build(move_ਗ੍ਰਾਫ);
    mTaskGraphp->hash_ਗ੍ਰਾਫ_Debug("initial MTask graph");

    // Add edges to eliminate data hazards
    ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::fix_ਅੰਕੜੇ_Hazards(*mTaskGraphp);
    mTaskGraphp->hash_ਗ੍ਰਾਫ_Debug("MTask graph after fixDataHazards()");

    // Merge MTask nodes together, repeatedly, until the critical path budget is reached. Coarsens
    // the graph, usually by several orders of magnitude. Some tests disable this for stability,
    // it should always be enabled in production.
    ਜੇ (v3Global.opt.ਧਾਗੇ_Coarsen()) {
        ਅਚਲ ਪੂਰਨ n_ਧਾਗੇ = v3Global.opt.ਧਾਗੇ();
        UASSERT(n_ਧਾਗੇ >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ਅਚਲ uint64_t fudge_ਅੰਕ = 3;
        ਅਚਲ uint64_t fudgeDen = 5;
        ਅਚਲ uint64_t ਹੱਦਬੰਦੀ = (mTaskGraphp->ਕੁੱਲ_ਲਾਗਤ() * fudge_ਅੰਕ) / (n_ਧਾਗੇ * fudgeDen);
        UINFO(4, "Partitioner set critical path limit = " << ਹੱਦਬੰਦੀ);

        ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ::contract(*mTaskGraphp, ਹੱਦਬੰਦੀ);
        mTaskGraphp->hash_ਗ੍ਰਾਫ_Debug("MTask graph after contract()");
    }

    // Remove MTasks that have no logic in them, rerouting the edges
    mTaskGraphp->remove_ਖਾਲੀ_ਥਾਵਾਂ_MTasks();
    mTaskGraphp->hash_ਗ੍ਰਾਫ_Debug("MTask graph after removeEmptyMTasks()");

    // Note this is OrderMTaskGraph::removeTransitiveEdges, which maintains graph consistency
    mTaskGraphp->remove_ਸੰਕ੍ਰਾਮਕ_ਕਿਨਾਰੇ();
    mTaskGraphp->hash_ਗ੍ਰਾਫ_Debug("MTask graph after removeTransitiveEdges()");

    // Check for data hazards the partitioning left unordered
    check_ਅੰਕੜੇ_Hazards(*mTaskGraphp);

    // Set OrderMoveVertex::userp to indicate the mtask it is part of.
    move_ਗ੍ਰਾਫ.userClearVertices();
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ਅਚਲ mtaskp = vtxp->as<LogicMTask>();
        ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼::List& ਸ਼ੀਰਸ਼_List = mtaskp->ਸ਼ੀਰਸ਼_List();
        ਜਦੋਂ (ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = ਸ਼ੀਰਸ਼_List.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    ਵਾਪਸ mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

ਵਰਗ Dpi_ਧਾਗੇ_Visitor final : ਜਨਤਕ VNVisitorConst {
    ਪੂਰਨ m_ਧਾਗੇ = 1;  // Max number of threads used by this mtask

    // METHODS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        m_ਧਾਗੇ = std::max(m_ਧਾਗੇ, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override { iterateConst(nodep->funcp()); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ Dpi_ਧਾਗੇ_Visitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~Dpi_ਧਾਗੇ_Visitor() override = ਮੂਲ;
    VL_UNCOPYABLE(Dpi_ਧਾਗੇ_Visitor);

ਜਨਤਕ:
    // Number of threads occupied by the given MTask
    ਸਥਿਰ ਪੂਰਨ apply(ਅਚਲ ExecMTask* mTaskp) {
        ਵਾਪਸ Dpi_ਧਾਗੇ_Visitor{mTaskp->funcp()}.m_ਧਾਗੇ;
    }
};

//######################################################################
// Entry point

Ast_ਗੰਢ_Stmt* V3_ਕ੍ਰਮ::createParallel(ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ& move_ਗ੍ਰਾਫ, ਅਚਲ std::string& ਟੈਗ,
                                     ਤਾਰਕਿਕ slow) {
    UINFO(2, "  Constructing parallel code for '" + ਟੈਗ + "'");

    // For nondeterminism debugging
    move_ਗ੍ਰਾਫ.hash_ਗ੍ਰਾਫ_Debug("V3Order::createParallel input OrderMoveGraph");
    move_ਗ੍ਰਾਫ.ਕ੍ਰਮ_ਗ੍ਰਾਫ_2().hash_ਗ੍ਰਾਫ_Debug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ਅਚਲ std::unique_ptr<ਕ੍ਰਮ_MTask_ਗ੍ਰਾਫ> mTaskGraphp = ਹਿੱਸੇ_ਕਰੋ(move_ਗ੍ਰਾਫ);
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) move_ਗ੍ਰਾਫ.dumpDotFilePrefixed(ਟੈਗ + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : move_ਗ੍ਰਾਫ.vertices().unlinkable()) {
        ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = vtxp->as<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>();
        ਜੇ (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->reroute_ਕਿਨਾਰੇ(&move_ਗ੍ਰਾਫ);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_ਗ੍ਰਾਫ), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : move_ਗ੍ਰਾਫ.vertices()) {
        ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = vtx.as<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>();
        LogicMTask* ਅਚਲ mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->ਸ਼ੀਰਸ਼_List().link_ਪਿੱਠ(mVtxp);
        // Remove edges crossing between MTasks
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : mVtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
            ਅਚਲ ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ ਮੰਜ਼ਿਲ_MVtxp = edgep->ਸਿਖਰ()->as<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>();
            ਜੇ (mtaskp != ਮੰਜ਼ਿਲ_MVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) move_ਗ੍ਰਾਫ.dumpDotFilePrefixed(ਟੈਗ + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_ਸਤਰ* ਅਚਲ flp = v3Global.rootp()->fileline();
    AstScope* ਅਚਲ scopep = v3Global.rootp()->ਸਿਖਰ_Scopep()->scopep();
    Ast_Exec_ਗ੍ਰਾਫ* ਅਚਲ execGraphp = ਨਵਾਂ Ast_Exec_ਗ੍ਰਾਫ{flp, ਟੈਗ, move_ਗ੍ਰਾਫ.ਕ੍ਰਮ_ਗ੍ਰਾਫ_2().trigVscp()};
    V3_ਗ੍ਰਾਫ* ਅਚਲ depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ਅਚਲ LogicMTask*, ExecMTask*> logic_MTask_ਮੰਜ਼ਿਲ_Exec_MTask;
    ਕ੍ਰਮ_Move_ਗ੍ਰਾਫ_Serializer serializer{move_ਗ੍ਰਾਫ};
    V3_ਕ੍ਰਮ_CFunc_Emitter emitter{move_ਗ੍ਰਾਫ.ਕ੍ਰਮ_ਗ੍ਰਾਫ_2().trigVscp(), ਟੈਗ, slow};
    // Sort LogicMTask vertices by their serial IDs.
    ਬਣਤਰ MTaskVxIdLessThan final {
        ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* rhsp) ਅਚਲ {
            ਵਾਪਸ lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    ਗ੍ਰਾਫ_ਧਾਰਾ<MTaskVxIdLessThan> mtask_ਧਾਰਾ{mTaskGraphp.get()};
    ਜਦੋਂ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = mtask_ਧਾਰਾ.nextp()) {
        ਅਚਲ LogicMTask* ਅਚਲ cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ਅਚਲ mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        ਜੇ (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->ਸ਼ੀਰਸ਼_List().ਖਾਲੀ_ਥਾਵਾਂ(), mTaskp,
                        "Entry and exit vertices should have no logic");
            ਜਾਰੀ;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        ਜਦੋਂ (ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = mTaskp->ਸ਼ੀਰਸ਼_List().unlinkFront()) {
            ਜੇ (mVtxp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) serializer.ਜੋੜੋ_ਬੀਜ(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        ਕ੍ਰਮ_Move_Dom_Scope* prevDomScopep = ਖਾਲੀ_ਸੂਚਕ;
        ਜਦੋਂ (ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼* ਅਚਲ mVtxp = serializer.get_ਅਗਲਾ()) {
            // We only really care about logic vertices
            ਜੇ (ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                ਕ੍ਰਮ_Move_Dom_Scope* ਅਚਲ domScopep = &mVtxp->domScope();
                ਜੇ (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_ਗ੍ਰਾਫ), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ਅਚਲ execMTaskp = ਨਵਾਂ ExecMTask{execGraphp, scopep, emitter.getStmts()};
        ਜੇ (!v3Global.opt.hier_ਖੰਡਾਂ().ਖਾਲੀ_ਥਾਵਾਂ()) {
            execMTaskp->ਧਾਗੇ(Dpi_ਧਾਗੇ_Visitor::apply(execMTaskp));
        }
        ਅਚਲ ਤਾਰਕਿਕ new_ਇੰਦਰਾਜ = logic_MTask_ਮੰਜ਼ਿਲ_Exec_MTask.emplace(mTaskp, execMTaskp).ਦੂਜਾ;
        UASSERT_OBJ(new_ਇੰਦਰਾਜ, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << ਟੈਗ << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{flp, execMTaskp->funcp()};
        callp->dtype_ਸੈੱਟ_Void();
        execGraphp->ਜੋੜੋ_Callsp(callp);

        // Add the dependency edges between ExecMTasks
        ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : mTaskp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            ਅਚਲ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromVxp = ਕਿਨਾਰਾ.fromp();
            ਅਚਲ LogicMTask* ਅਚਲ fromp = fromVxp->as<ਅਚਲ LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            ਜੇ (fromp == mTaskGraphp->entryp()) ਜਾਰੀ;
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{depGraphp, logic_MTask_ਮੰਜ਼ਿਲ_Exec_MTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : move_ਗ੍ਰਾਫ.vertices().unlinkable()) {
        ਜੇ (!vtxp->as<ਕ੍ਰਮ_Move_ਸ਼ੀਰਸ਼>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_ਗ੍ਰਾਫ), vtxp);
        }
    }

    ਵਾਪਸ execGraphp;
}
