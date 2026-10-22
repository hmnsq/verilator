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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3ExecGraph.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3GraphStream.h"
#ಅನ್ತರ್ಭಾವ "V3OrderCFuncEmitter.h"
#ಅನ್ತರ್ಭಾವ "V3OrderInternal.h"
#ಅನ್ತರ್ಭಾವ "V3OrderMTaskGraph.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Data hazard checker

// Reports read-write and write-write pairs on the same variable that
// are not ordered in the MTask graph.
ಸ್ಥಿರ ಶೂನ್ಯ check_ದತ್ತಾಂಶ_Hazards(ಕ್ರಮ_MTask_ನಕ್ಷೆ& m_Task_ನಕ್ಷೆ) {
    // Expensive, so only with '--debug-partition'
    ಆದರೆ (!m_Task_ನಕ್ಷೆ.slowAsserts()) ಹಿಂತಿರುಗಿ;

    // Order MTasks by their stable ids, so the report is deterministic
    ರಚನೆ MTaskIdLessThan final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ LogicMTask* ap, ಸ್ಥಿರಾಂಕ LogicMTask* bp) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ *ap < *bp; }
    };
    ರಚನೆ VarInfo final {
        ತಾರ್ಕಿಕ m_ನೋಡಿದ = ಸುಳ್ಳು;  // Variable already appended to 'vscps'
        // How each MTask accesses the variable, merged over the logic within that MTask
        std::map<LogicMTask*, VAccess, MTaskIdLessThan> m_byMTask;
    };

    // AstVarScope::user1 -> VarInfo instance for the variable (via 'varInfos')
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
    AstUser1Allocator<AstVarScope, VarInfo> varInfos;

    // The variables accessed (in enumerated order, for stability).
    std::ಸದಿಶ<AstVarScope*> vscps;

    // Gather how each MTask accesses each variable
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_Task_ನಕ್ಷೆ.vertices()) {
        LogicMTask& mtask = static_cast<LogicMTask&>(vtx);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Move_ಶೃಂಗ& mVtx : mtask.ಶೃಂಗ_List()) {
            ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp = mVtx.logicp();
            ಆದರೆ (!lVtxp) ಮುಂದುವರಿ;  // A variable vertex, which performs no access itself
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Logic_ಶೃಂಗ::VarAccess& acc : lVtxp->varAccesses()) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = acc.m_vscp;
                VarInfo& varInfo = varInfos(vscp);
                ಆದರೆ (!varInfo.m_ನೋಡಿದ) {
                    varInfo.m_ನೋಡಿದ = ನಿಜ;
                    vscps.ತಳ್ಳು_ಹಿಂಭಾಗ(vscp);
                }
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = varInfo.m_byMTask.emplace(&mtask, acc.m_access);
                // Merge the access kinds if this MTask already accessed this variable
                ಆದರೆ (!pair.ಎರಡನೇ && pair.ಮೊದಲ->ಎರಡನೇ != acc.m_access) {
                    pair.ಮೊದಲ->ಎರಡನೇ = VAccess::READWRITE;
                }
            }
        }
    }

    // Report every unordered pair of accessors where at least one side writes
    AstVarScope* ಮೊದಲ_Hazardp = ಶೂನ್ಯಸೂಚಕ;  // First variable with a hazard, for the error below
    ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : vscps) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& byMTask = varInfos(vscp).m_byMTask;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ aIt = byMTask.begin(); aIt != byMTask.ಕೊನೆ(); ++aIt) {
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ bIt = std::ಮುಂದಿನ(aIt); bIt != byMTask.ಕೊನೆ(); ++bIt) {
                // Concurrent reads are not a hazard
                ಆದರೆ (aIt->ಎರಡನೇ.isReadOnly() && bIt->ಎರಡನೇ.isReadOnly()) ಮುಂದುವರಿ;
                LogicMTask* ಸ್ಥಿರಾಂಕ ap = aIt->ಮೊದಲ;
                LogicMTask* ಸ್ಥಿರಾಂಕ bp = bIt->ಮೊದಲ;
                ಆದರೆ (m_Task_ನಕ್ಷೆ.ಮಾರ್ಗ_Exists(ap, bp, ಶೂನ್ಯಸೂಚಕ)) ಮುಂದುವರಿ;
                ಆದರೆ (m_Task_ನಕ್ಷೆ.ಮಾರ್ಗ_Exists(bp, ap, ಶೂನ್ಯಸೂಚಕ)) ಮುಂದುವರಿ;
                // LCOV_EXCL_START
                ಆದರೆ (!ಮೊದಲ_Hazardp) ಮೊದಲ_Hazardp = vscp;
                UINFO(0, "Data hazard: " << vscp->name() << " " << aIt->ಎರಡನೇ.ascii() << " by mt"
                                         << ap->id() << ", " << bIt->ಎರಡನೇ.ascii() << " by mt"
                                         << bp->id() << " (unordered)");
                // LCOV_EXCL_STOP
            }
        }
    }
    // Fail if any hazards were found
    ಆದರೆ (ಮೊದಲ_Hazardp) ಮೊದಲ_Hazardp->v3fatalSrc("Data hazards found");  // LCOV_EXCL_BR_LINE
}

//######################################################################
// Partitioner implementation

// Partitioner takes the fine-grained OrderMoveGraph from V3Order and collapses
// it into a coarse-grained graph of LogicMTask's, each of which contains of set
// of the logic nodes from the fine-grained graph.

ಸ್ಥಿರ std::unique_ptr<ಕ್ರಮ_MTask_ನಕ್ಷೆ> ವಿಭಜನೆ(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ) {
    // Build the initial MTask graph. Initially, each MTask just wraps one OrderMoveVertex. We will
    // merge MTasks together and eventually each MTask will wrap a large number of OrderMoveVertex
    // (and the logic nodes therein).
    std::unique_ptr<ಕ್ರಮ_MTask_ನಕ್ಷೆ> mTaskGraphp = ಕ್ರಮ_MTask_ನಕ್ಷೆ::build(move_ನಕ್ಷೆ);
    mTaskGraphp->hash_ನಕ್ಷೆ_Debug("initial MTask graph");

    // Add edges to eliminate data hazards
    ಕ್ರಮ_MTask_ನಕ್ಷೆ::fix_ದತ್ತಾಂಶ_Hazards(*mTaskGraphp);
    mTaskGraphp->hash_ನಕ್ಷೆ_Debug("MTask graph after fixDataHazards()");

    // Merge MTask nodes together, repeatedly, until the critical path budget is reached. Coarsens
    // the graph, usually by several orders of magnitude. Some tests disable this for stability,
    // it should always be enabled in production.
    ಆದರೆ (v3Global.opt.ಎಳೆಗಳು_Coarsen()) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ n_ಎಳೆಗಳು = v3Global.opt.ಎಳೆಗಳು();
        UASSERT(n_ಎಳೆಗಳು >= 2, "Should not reach Partitioner when --threads <= 1");

        // Set critical path limit to roughly totalGraphCost / nThreads. Actually set it slighly
        // lower, by a hardcoded fudge factor. This results in a smaller graph, which helps reduce
        // fragmentation when scheduling them. TODO: What does this sentence mean?
        ಸ್ಥಿರಾಂಕ uint64_t fudge_ಅಂಕೆ = 3;
        ಸ್ಥಿರಾಂಕ uint64_t fudgeDen = 5;
        ಸ್ಥಿರಾಂಕ uint64_t ಎಲ್ಲೆ = (mTaskGraphp->ಒಟ್ಟು_ವೆಚ್ಚ() * fudge_ಅಂಕೆ) / (n_ಎಳೆಗಳು * fudgeDen);
        UINFO(4, "Partitioner set critical path limit = " << ಎಲ್ಲೆ);

        ಕ್ರಮ_MTask_ನಕ್ಷೆ::contract(*mTaskGraphp, ಎಲ್ಲೆ);
        mTaskGraphp->hash_ನಕ್ಷೆ_Debug("MTask graph after contract()");
    }

    // Remove MTasks that have no logic in them, rerouting the edges
    mTaskGraphp->remove_ಖಾಲಿಗಳು_MTasks();
    mTaskGraphp->hash_ನಕ್ಷೆ_Debug("MTask graph after removeEmptyMTasks()");

    // Note this is OrderMTaskGraph::removeTransitiveEdges, which maintains graph consistency
    mTaskGraphp->remove_ಸಂಕ್ರಾಮಿಕ_ಅಂಚುಗಳು();
    mTaskGraphp->hash_ನಕ್ಷೆ_Debug("MTask graph after removeTransitiveEdges()");

    // Check for data hazards the partitioning left unordered
    check_ದತ್ತಾಂಶ_Hazards(*mTaskGraphp);

    // Set OrderMoveVertex::userp to indicate the mtask it is part of.
    move_ನಕ್ಷೆ.userClearVertices();
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : mTaskGraphp->vertices().unlinkable()) {
        LogicMTask* ಸ್ಥಿರಾಂಕ mtaskp = vtxp->as<LogicMTask>();
        ಕ್ರಮ_Move_ಶೃಂಗ::List& ಶೃಂಗ_List = mtaskp->ಶೃಂಗ_List();
        ಆಗ (ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = ಶೃಂಗ_List.unlinkFront()) mVtxp->userp(mtaskp);
    }

    // Return the resulting MTask graph
    ಹಿಂತಿರುಗಿ mTaskGraphp;
}

//######################################################################
// DpiThreadsVisitor - Finds number of threads used by an ExecMTask

ವರ್ಗ Dpi_ಎಳೆಗಳು_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    ಪೂರ್ಣಾಂಕ m_ಎಳೆಗಳು = 1;  // Max number of threads used by this mtask

    // METHODS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        m_ಎಳೆಗಳು = std::max(m_ಎಳೆಗಳು, V3Control::getHierWorkers(nodep->cname()));
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_CCall* nodep) override { iterateConst(nodep->funcp()); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Dpi_ಎಳೆಗಳು_Visitor(AstCFunc* nodep) { iterateConst(nodep); }
    ~Dpi_ಎಳೆಗಳು_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(Dpi_ಎಳೆಗಳು_Visitor);

ಸಾರ್ವಜನಿಕ:
    // Number of threads occupied by the given MTask
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ apply(ಸ್ಥಿರಾಂಕ ExecMTask* mTaskp) {
        ಹಿಂತಿರುಗಿ Dpi_ಎಳೆಗಳು_Visitor{mTaskp->funcp()}.m_ಎಳೆಗಳು;
    }
};

//######################################################################
// Entry point

Ast_ಗಂಟು_Stmt* V3_ಕ್ರಮ::createParallel(ಕ್ರಮ_Move_ನಕ್ಷೆ& move_ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್,
                                     ತಾರ್ಕಿಕ slow) {
    UINFO(2, "  Constructing parallel code for '" + ಟ್ಯಾಗ್ + "'");

    // For nondeterminism debugging
    move_ನಕ್ಷೆ.hash_ನಕ್ಷೆ_Debug("V3Order::createParallel input OrderMoveGraph");
    move_ನಕ್ಷೆ.ಕ್ರಮ_ನಕ್ಷೆ_2().hash_ನಕ್ಷೆ_Debug("V3Order::createParallel input OrderGraph");

    // Partition moveGraph into LogicMTask's. The partitioner will set userp() on each logic
    // vertex in the moveGraph to the MTask it belongs to.
    ಸ್ಥಿರಾಂಕ std::unique_ptr<ಕ್ರಮ_MTask_ನಕ್ಷೆ> mTaskGraphp = ವಿಭಜನೆ(move_ನಕ್ಷೆ);
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) move_ನಕ್ಷೆ.dumpDotFilePrefixed(ಟ್ಯಾಗ್ + "_ordermv_mtasks");

    // Some variable OrderMoveVertices are not assigned to an MTask. Reroute and delete these.
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : move_ನಕ್ಷೆ.vertices().unlinkable()) {
        ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = vtxp->as<ಕ್ರಮ_Move_ಶೃಂಗ>();
        ಆದರೆ (!mVtxp->userp()) {
            UASSERT_OBJ(!mVtxp->logicp(), mVtxp, "Logic OrderMoveVertex not assigned to mtask");
            mVtxp->reroute_ಅಂಚುಗಳು(&move_ನಕ್ಷೆ);
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_ನಕ್ಷೆ), mVtxp);
        }
    }

    // Remove all edges from the move graph that cross between MTasks. Add logic to MTask lists.
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : move_ನಕ್ಷೆ.vertices()) {
        ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = vtx.as<ಕ್ರಮ_Move_ಶೃಂಗ>();
        LogicMTask* ಸ್ಥಿರಾಂಕ mtaskp = static_cast<LogicMTask*>(mVtxp->userp());
        // Add to list in MTask, in MoveGraph order. This should not be necessary, but see #4993.
        mtaskp->ಶೃಂಗ_List().link_ಹಿಂಭಾಗ(mVtxp);
        // Remove edges crossing between MTasks
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : mVtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
            ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_MVtxp = edgep->ತುದಿ()->as<ಕ್ರಮ_Move_ಶೃಂಗ>();
            ಆದರೆ (mtaskp != ಗಮ್ಯ_MVtxp->userp()) VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) move_ನಕ್ಷೆ.dumpDotFilePrefixed(ಟ್ಯಾಗ್ + "_ordermv_pruned");

    // Create the AstExecGraph node which represents the execution of the MTask graph.
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = v3Global.rootp()->fileline();
    AstScope* ಸ್ಥಿರಾಂಕ scopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();
    Ast_Exec_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ execGraphp = ಹೊಸ Ast_Exec_ನಕ್ಷೆ{flp, ಟ್ಯಾಗ್, move_ನಕ್ಷೆ.ಕ್ರಮ_ನಕ್ಷೆ_2().trigVscp()};
    V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ depGraphp = execGraphp->depGraphp();

    // Translate the LogicMTask graph into the corresponding ExecMTask graph,
    // which will outlive ordering.
    std::unordered_map<ಸ್ಥಿರಾಂಕ LogicMTask*, ExecMTask*> logic_MTask_ಗಮ್ಯ_Exec_MTask;
    ಕ್ರಮ_Move_ನಕ್ಷೆ_Serializer serializer{move_ನಕ್ಷೆ};
    V3_ಕ್ರಮ_CFunc_Emitter emitter{move_ನಕ್ಷೆ.ಕ್ರಮ_ನಕ್ಷೆ_2().trigVscp(), ಟ್ಯಾಗ್, slow};
    // Sort LogicMTask vertices by their serial IDs.
    ರಚನೆ MTaskVxIdLessThan final {
        ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* rhsp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ lhsp->as<LogicMTask>()->id() < rhsp->as<LogicMTask>()->id();
        }
    };
    ನಕ್ಷೆ_ಹರಿವು<MTaskVxIdLessThan> mtask_ಹರಿವು{mTaskGraphp.get()};
    ಆಗ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = mtask_ಹರಿವು.nextp()) {
        ಸ್ಥಿರಾಂಕ LogicMTask* ಸ್ಥಿರಾಂಕ cMTaskp = vtxp->as<LogicMTask>();
        LogicMTask* ಸ್ಥಿರಾಂಕ mTaskp = const_cast<LogicMTask*>(cMTaskp);

        // The entry and exit vertices only anchor the graph, they hold no logic and
        // must not become ExecMTasks.
        ಆದರೆ (mTaskp == mTaskGraphp->entryp() || mTaskp == mTaskGraphp->exitp()) {
            UASSERT_OBJ(mTaskp->ಶೃಂಗ_List().ಖಾಲಿಗಳು(), mTaskp,
                        "Entry and exit vertices should have no logic");
            ಮುಂದುವರಿ;
        }

        // Add initially ready vertices within this MTask to the serializer as seeds,
        // and unlink them from the vertex list in the MTask as we go. (The serializer
        // uses the list links in the vertex, so must unlink it here.)
        ಆಗ (ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = mTaskp->ಶೃಂಗ_List().unlinkFront()) {
            ಆದರೆ (mVtxp->ಒಳ_ಖಾಲಿಗಳು()) serializer.ಕೂಡಿಸು_ಬೀಜ(mVtxp);
        }

        // Emit all logic within the MTask as they become ready
        ಕ್ರಮ_Move_Dom_Scope* prevDomScopep = ಶೂನ್ಯಸೂಚಕ;
        ಆಗ (ಕ್ರಮ_Move_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mVtxp = serializer.get_ಮುಂದಿನ()) {
            // We only really care about logic vertices
            ಆದರೆ (ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ logicp = mVtxp->logicp()) {
                // Force a new function if the domain or scope changed, for better combining.
                ಕ್ರಮ_Move_Dom_Scope* ಸ್ಥಿರಾಂಕ domScopep = &mVtxp->domScope();
                ಆದರೆ (domScopep != prevDomScopep) emitter.forceNewFunction();
                prevDomScopep = domScopep;
                // Emit the logic under this vertex
                emitter.emitLogic(logicp);
            }
            // Can delete the vertex now
            VL_DO_DANGLING(mVtxp->unlinkDelete(&move_ನಕ್ಷೆ), mVtxp);
        }

        // Create the ExecMTask
        ExecMTask* ಸ್ಥಿರಾಂಕ execMTaskp = ಹೊಸ ExecMTask{execGraphp, scopep, emitter.getStmts()};
        ಆದರೆ (!v3Global.opt.hier_ಖಂಡಗಳು().ಖಾಲಿಗಳು()) {
            execMTaskp->ಎಳೆಗಳು(Dpi_ಎಳೆಗಳು_Visitor::apply(execMTaskp));
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ new_ನಮೂದು = logic_MTask_ಗಮ್ಯ_Exec_MTask.emplace(mTaskp, execMTaskp).ಎರಡನೇ;
        UASSERT_OBJ(new_ನಮೂದು, mTaskp, "LogicMTasks should be processed in dependencyorder");
        UINFO(3, "Final '" << ಟ್ಯಾಗ್ << "' LogicMTask " << mTaskp->id() << " maps to ExecMTask"
                           << execMTaskp->id());

        // For code analysis purposes, we can pretend the AstExecGraph runs the
        // MTasks sequentially, in some topological order that respects edges.
        // The order they are created here happens to be just such an order.
        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{flp, execMTaskp->funcp()};
        callp->dtype_ಗಣ_Void();
        execGraphp->ಕೂಡಿಸು_Callsp(callp);

        // Add the dependency edges between ExecMTasks
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : mTaskp->ಒಳ_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಶೃಂಗ* fromVxp = ಅಂಚು.fromp();
            ಸ್ಥಿರಾಂಕ LogicMTask* ಸ್ಥಿರಾಂಕ fromp = fromVxp->as<ಸ್ಥಿರಾಂಕ LogicMTask>();
            // Skip the entry vertex, which has no ExecMTask
            ಆದರೆ (fromp == mTaskGraphp->entryp()) ಮುಂದುವರಿ;
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{depGraphp, logic_MTask_ಗಮ್ಯ_Exec_MTask.at(fromp), execMTaskp, 1};
        }
    }

    // Delete the remaining variable vertices
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : move_ನಕ್ಷೆ.vertices().unlinkable()) {
        ಆದರೆ (!vtxp->as<ಕ್ರಮ_Move_ಶೃಂಗ>()->logicp()) {
            VL_DO_DANGLING(vtxp->unlinkDelete(&move_ನಕ್ಷೆ), vtxp);
        }
    }

    ಹಿಂತಿರುಗಿ execGraphp;
}
