// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Gate optimizations, such as wire elimination
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
// V3Gate's Transformations:
//
// Extract a graph of the *entire* netlist with cells expanded
// Perform constant optimization across the graph
// Create VARSCOPEs for any variables we can rip out
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Gate.h"

#ಅನ್ತರ್ಭಾವ "V3AstUserAllocator.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3DupFinder.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

ವರ್ಗ GateEitherVertex VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    ತಾರ್ಕಿಕ m_reducible = ಸತ್ಯ;  // True if this node should be able to be eliminated
    ತಾರ್ಕಿಕ m_dedupable = ಸತ್ಯ;  // True if this node should be able to be deduped
    ತಾರ್ಕಿಕ m_consumed = ಅಸತ್ಯ;  // Output goes to something meaningful
    ತಾರ್ಕಿಕ m_staticInit = ಅಸತ್ಯ;  // True if this node is a static initializer
ಸಾರ್ವಜನಿಕ:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = ಸ್ವಭಾವ;

    // ACCESSORS
    ತಾರ್ಕಿಕ reducible() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_reducible; }
    ತಾರ್ಕಿಕ dedupable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dedupable; }
    ತಾರ್ಕಿಕ consumed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_consumed; }
    ತಾರ್ಕಿಕ staticInit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_staticInit; }
    ಶೂನ್ಯ setConsumed(ಧ್ರುವ ವರ್ಣ* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = ಸತ್ಯ;
    }
    ಶೂನ್ಯ setStaticInit() { m_staticInit = ಸತ್ಯ; }
    ಶೂನ್ಯ clearReducible(ಧ್ರುವ ವರ್ಣ* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ clearDedupable(ಧ್ರುವ ವರ್ಣ* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ clearReducibleAndDedupable(ಧ್ರುವ ವರ್ಣ* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_consumed ? "" : "dotted"; }
};

ವರ್ಗ GateVarVertex final : ಸಾರ್ವಜನಿಕ GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* ಧ್ರುವ m_varScp;
    ತಾರ್ಕಿಕ m_is_ತುದಿ = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ m_isClock = ಅಸತ್ಯ;
    Ast_ಗಂಟು* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_ಗಂಟು* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
ಸಾರ್ವಜನಿಕ:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = ಸ್ವಭಾವ;

    // ACCESSORS
    AstVarScope* varScp() ಧ್ರುವ VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_varScp; }
    ತಾರ್ಕಿಕ is_ತುದಿ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_is_ತುದಿ; }
    ಶೂನ್ಯ set_Is_ತುದಿ() { m_is_ತುದಿ = ಸತ್ಯ; }
    ತಾರ್ಕಿಕ isClock() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isClock; }
    ಶೂನ್ಯ setIsClock() { m_isClock = ಸತ್ಯ; }
    Ast_ಗಂಟು* rstSyncNodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rstSyncNodep; }
    ಶೂನ್ಯ rstSyncNodep(Ast_ಗಂಟು* nodep) { m_rstSyncNodep = nodep; }
    Ast_ಗಂಟು* rstAsyncNodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rstAsyncNodep; }
    ಶೂನ್ಯ rstAsyncNodep(Ast_ಗಂಟು* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    ಶೂನ್ಯ propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        ಯದಿ (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ varScp()->name(); }
    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "green"; }
};

ವರ್ಗ GateLogicVertex final : ಸಾರ್ವಜನಿಕ GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_ಗಂಟು* ಧ್ರುವ m_nodep;
    AstActive* ಧ್ರುವ m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ಧ್ರುವ ತಾರ್ಕಿಕ m_slow;  // In slow block
ಸಾರ್ವಜನಿಕ:
    GateLogicVertex(V3Graph* graphp, Ast_ಗಂಟು* nodep, AstActive* activep, ತಾರ್ಕಿಕ slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = ಸ್ವಭಾವ;

    // ACCESSORS
    File_ಸಾಲು* fileline() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ nodep()->fileline(); }
    Ast_ಗಂಟು* nodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    AstActive* activep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_activep; }
    ತಾರ್ಕಿಕ slow() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_slow; }

    // DOT debug
    std::string name() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_nodep->fileline()->ascii(); }
    std::string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "red"; }
};

ವರ್ಗ GateEdge final : ಸಾರ್ವಜನಿಕ V3GraphEdge {
    std::string dotLabel() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ std::to_string(weight()); }

ಸಾರ್ವಜನಿಕ:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight)
        : V3GraphEdge{graphp, fromp, ತುದಿ, weight} {}
};

ವರ್ಗ GateGraph final : ಸಾರ್ವಜನಿಕ V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ಧ್ರುವ VNUser1InUse m_inuser1;

ಸಾರ್ವಜನಿಕ:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        ಯದಿ (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = ನೂತನ GateVarVertex{ಏತತ್, vscp};
            vscp->user1p(vVtxp);
            ಯದಿ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            ಯದಿ (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            ಯದಿ (vscp->varp()->isIO() && vscp->scopep()->is_ತುದಿ()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_ತುದಿ();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        ಪ್ರತ್ಯಾಗಮ vVtxp;
    }

    ಶೂನ್ಯ addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, ಪೂರ್ಣಾಙ್ಕ weight) {
        ನೂತನ GateEdge{ಏತತ್, srcp, dstp, weight};
    }

    ಶೂನ್ಯ addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, ಪೂರ್ಣಾಙ್ಕ weight) {
        ನೂತನ GateEdge{ಏತತ್, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

ವರ್ಗ GateBuildVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    GateGraph* m_graphp = ನೂತನ GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    ಧ್ರುವ Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    ಧ್ರುವ AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    ತಾರ್ಕಿಕ m_inClockedActive = ಅಸತ್ಯ;  // Underneath clocked active
    ತಾರ್ಕಿಕ m_inEdgeActive = ಅಸತ್ಯ;  // Underneath edge active
    ತಾರ್ಕಿಕ m_inStaticActive = ಅಸತ್ಯ;  // Underneath static active
    ತಾರ್ಕಿಕ m_inInitialActive = ಅಸತ್ಯ;  // Underneath initial active
    ತಾರ್ಕಿಕ m_inSenItem = ಅಸತ್ಯ;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    ಶೂನ್ಯ check_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        ಯದಿ (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        ಯದಿ (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    ಶೂನ್ಯ iterateLogic(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ slow = ಅಸತ್ಯ, ಧ್ರುವ ವರ್ಣ* nonReducibleReason = nullptr,
                      ಧ್ರುವ ವರ್ಣ* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = ನೂತನ GateLogicVertex{m_graphp, nodep, m_activep, slow};
        ಯದಿ (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } ಅನ್ಯಥಾ ಯದಿ (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } ಅನ್ಯಥಾ ಯದಿ (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        ಯದಿ (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_ಗಂಟು(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_inClockedActive);
        VL_RESTORER(m_inEdgeActive);
        VL_RESTORER(m_inStaticActive);
        VL_RESTORER(m_inInitialActive);
        m_activep = nodep;
        m_inClockedActive = nodep->hasClocked();
        m_inEdgeActive = nodep->sentreep() && nodep->sentreep()->hasEdge();
        m_inStaticActive = nodep->hasStatic();
        m_inInitialActive = nodep->hasInitial();

        // AstVarScope::user2 -> bool: Signal used in SenItem in *this* active block
        ಧ್ರುವ VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    ಶೂನ್ಯ visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, ಅಸತ್ಯ, "CoverToggle", "CoverToggle");
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = ಸತ್ಯ;
        ಯದಿ (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } ಅನ್ಯಥಾ {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, ಅಸತ್ಯ, nullptr, "senItem");
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (!m_logicVertexp) ಪ್ರತ್ಯಾಗಮ;

        AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();
        GateVarVertex* ಧ್ರುವ vVtxp = m_graphp->makeVarVertex(vscp);

        ಯದಿ (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(ಸತ್ಯ);
        } ಅನ್ಯಥಾ ಯದಿ (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            ಯದಿ (vscp->user2()) {
                ಯದಿ (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } ಅನ್ಯಥಾ {
                ಯದಿ (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        ಯದಿ (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        ಯದಿ (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    ಶೂನ್ಯ visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), ಗಂಟು_Assign)
                      && VN_AS(nodep->backp(), ಗಂಟು_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (m_logicVertexp) check_ಗಂಟು(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        ಪ್ರತ್ಯಾಗಮ std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ಸ್ಥಿರ ಶೂನ್ಯ v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ಧ್ರುವ VNUser2InUse user2InUse;
    ಪ್ರತಿ (V3GraphVertex& vtx : graph.vertices()) {
        ಯದಿ (ಧ್ರುವ GateVarVertex* ಧ್ರುವ vvertexp = vtx.cast<GateVarVertex>()) {
            ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp = vvertexp->varScp();
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ sp = vvertexp->rstSyncNodep();
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ ap = vvertexp->rstAsyncNodep();
            ಯದಿ (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                ಯದಿ (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(ಸತ್ಯ);  // Warn only once per signal
                    vscp->v3warn(SYNCASYNCNET,
                                 "Signal flopped as both synchronous and async: "
                                     << vscp->prettyNameQ() << '\n'
                                     << ap->warnOther() << "... Location of async usage\n"
                                     << ap->warnContextPrimary() << '\n'
                                     << sp->warnOther() << "... Location of sync usage\n"
                                     << sp->warnContextSecondary());
                }
            }
        }
    }
}

//######################################################################
// Find a var's offset in a concatenation (only used by GateClkDecomp)

ವರ್ಗ GateConcatVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    ಧ್ರುವ AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    ಪೂರ್ಣಾಙ್ಕ m_offset = 0;  // Current offset of varscope
    ಪೂರ್ಣಾಙ್ಕ m_found_offset = 0;  // Found offset of varscope
    ತಾರ್ಕಿಕ m_found = ಅಸತ್ಯ;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(ಸತ್ಯ);
            m_found_offset = m_offset;
            m_found = ಸತ್ಯ;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    ಶೂನ್ಯ visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // PUBLIC METHODS
    ತಾರ್ಕಿಕ concatOffset(AstConcat* concatp, AstVarScope* vscp, ಪೂರ್ಣಾಙ್ಕ& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = ಅಸತ್ಯ;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        ಪ್ರತ್ಯಾಗಮ m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

ವರ್ಗ GateOkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // RETURN STATE
    ತಾರ್ಕಿಕ m_isSimple = ಸತ್ಯ;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    ಧ್ರುವ ತಾರ್ಕಿಕ m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ಧ್ರುವ ತಾರ್ಕಿಕ m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* m_lhsVarRef = nullptr;
    ಪೂರ್ಣಾಙ್ಕ m_ops = 0;  // Operation count

    // METHODS
    ಶೂನ್ಯ clearSimple(ಧ್ರುವ ವರ್ಣ* because) {
        ಯದಿ (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = ಅಸತ್ಯ;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (!m_isSimple) ಪ್ರತ್ಯಾಗಮ;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        ಯದಿ (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        ಯದಿ (nodep->access().isRW()) {
            clearSimple("R/W");
            ಪ್ರತ್ಯಾಗಮ;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();
        ಯದಿ (nodep->access().isWriteOnly()) {
            ಯದಿ (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } ಅನ್ಯಥಾ {
            // TODO: possible bug, should it be >= 1 as add is below?
            ಯದಿ (m_readVscps.size() > 1) {
                ಯದಿ (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ತಳ್ಳು_back(vscp);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (!m_isSimple) ಪ್ರತ್ಯಾಗಮ;

        m_substitutionp = nodep->rhsp();
        ಯದಿ (!VN_IS(nodep->lhsp(), ಗಂಟು_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } ಅನ್ಯಥಾ {
            iterateChildrenConst(nodep);
        }
        ಯದಿ (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    ಶೂನ್ಯ visit(AstCReset* /*nodep*/) override {
        ಯದಿ (!m_isSimple) ಪ್ರತ್ಯಾಗಮ;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    ಶೂನ್ಯ visit(AstMatchMasked* nodep) override {
        ಯದಿ (!m_isSimple) ಪ್ರತ್ಯಾಗಮ;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        ಯದಿ (!m_isSimple) ಪ್ರತ್ಯಾಗಮ;  // Fastpath

        ಯದಿ (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            ಪ್ರತ್ಯಾಗಮ;
        }

        iterateChildrenConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    GateOkVisitor(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ buffersOnly, ತಾರ್ಕಿಕ dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        ಯದಿ (!m_substitutionp) {
            clearSimple("No assignment found\n");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_isSimple && m_lhsVarRef) {
            ಪ್ರತಿ (ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp : m_readVscps) {
                ಯದಿ (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }
    ~GateOkVisitor() override = ಸ್ವಭಾವ;
    // PUBLIC METHODS
    ತಾರ್ಕಿಕ isSimple() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_isSimple; }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substitutionp() ಧ್ರುವ {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        ಪ್ರತ್ಯಾಗಮ m_substitutionp;
    }
    ಧ್ರುವ std::vector<AstVarScope*>& readVscps() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_readVscps; }
    ತಾರ್ಕಿಕ varAssigned(ಧ್ರುವ AstVarScope* scopep) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

ವರ್ಗ GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_ಗಂಟು, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_ಗಂಟು*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isCheap(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Constant is cheap
        ಯದಿ (VN_IS(exprp, Const)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Variable reference is cheap
        ಯದಿ (VN_IS(exprp, ಗಂಟು_Var_Ref)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        ಯದಿ (ಧ್ರುವ AstSel* ಧ್ರುವ selp = VN_CAST(exprp, Sel)) {
            ಯದಿ (!isCheap(selp->fromp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (!selp->isWide()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಯದಿ (!VN_IS(selp->lsbp(), Const)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (selp->lsbConst() % VL_EDATASIZE != 0) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // AstArraySel is cheap if the fromp is cheap
        ಯದಿ (ಧ್ರುವ AstArraySel* ಧ್ರುವ aselp = VN_CAST(exprp, ArraySel)) {
            ಪ್ರತ್ಯಾಗಮ isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ತಾರ್ಕಿಕ shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substp, ತಾರ್ಕಿಕ allowMultiIn) {
        // Always inline constants
        ಯದಿ (VN_IS(substp, Const)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Don't inline non-constant static initializers - these are scheduled differently
        ಯದಿ (lVtxp->staticInit()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Inline simple variable references
        ಯದಿ (VN_IS(substp, VarRef)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Only inline arrays if a simple variable or constant
        ಯದಿ (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        ಯದಿ (nReads == 0) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // If it reads one variable, inline if not wide, or if cheap
        ಯದಿ (nReads == 1 && (!substp->isWide() || isCheap(substp))) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Don't inline on first round if reads more than one variable
        ಯದಿ (nReads > 1 && !allowMultiIn) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        ಪೂರ್ಣಾಙ್ಕ n = 0;
        ಪ್ರತಿ (V3GraphEdge& edge : vVtxp->outEdges()) {
            ಧ್ರುವ GateLogicVertex* ಧ್ರುವ dstVtxp = edge.ತುದಿ()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            ಯದಿ (dstVtxp->slow()) ಅನುವರ್ತ;
            ಯದಿ (dstVtxp->outEmpty() && !dstVtxp->consumed()) ಅನುವರ್ತ;
            n += edge.weight();
            ಯದಿ (n > 1) {
                ++m_statNotInlined;
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ recordSubstitution(AstVarScope* vscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substp, Ast_ಗಂಟು* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ಧ್ರುವ auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        ಯದಿ (pair.second) pair.first->second = substp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
    }

    ಶೂನ್ಯ commitSubstitutions(Ast_ಗಂಟು* logicp) {
        ಯದಿ (!m_hasPending.erase(logicp)) ಪ್ರತ್ಯಾಗಮ;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<ಧ್ರುವ AstVarScope*> replaced(substitutions.size() * 2);

        ಧ್ರುವ std::function<ಶೂನ್ಯ(Ast_ಗಂಟು_Var_Ref*)> visit = [&](Ast_ಗಂಟು_Var_Ref* nodep) -> ಶೂನ್ಯ {
            // See if this variable has a substitution
            AstVarScope* ಧ್ರುವ vscp = nodep->varScopep();
            ಧ್ರುವ auto& it = substitutions.find(vscp);
            ಯದಿ (it == substitutions.end()) ಪ್ರತ್ಯಾಗಮ;

            // Do not substitute circular logic
            ಯದಿ (!replaced.ಸೇರಿಸು(vscp).second) ಪ್ರತ್ಯಾಗಮ;

            // Substitute nodep with substp
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, ಗಂಟು_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = substp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            ಯದಿ (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(newp, ಗಂಟು_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_ಗಂಟು* ಧ್ರುವ simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : substitutions) pair.second->delete_ವೃಕ್ಷ();
        substitutions.clear();
    }

    ಶೂನ್ಯ optimizeSignals(ತಾರ್ಕಿಕ allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        ಧ್ರುವ auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            ಯಾವತ್ (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            ಪ್ರತ್ಯಾಗಮ it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        ಯಾವತ್ (vIt != vertices.end()) {
            GateVarVertex* ಧ್ರುವ vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            ಯದಿ (!vVtxp->inSize1()) ಅನುವರ್ತ;

            // Can't inline if non-reducible, etc
            ಯದಿ (!vVtxp->reducible()) ಅನುವರ್ತ;

            // Grab the driving logic
            GateLogicVertex* ಧ್ರುವ lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            ಯದಿ (!lVtxp->reducible()) ಅನುವರ್ತ;
            Ast_ಗಂಟು* ಧ್ರುವ logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ಧ್ರುವ GateOkVisitor okVisitor{logicp, ಅಸತ್ಯ, ಅಸತ್ಯ};

            // Was it ok?
            ಯದಿ (!okVisitor.isSimple()) ಅನುವರ್ತ;
            // If the varScope is already removed from logicp, no need to try substitution.
            ಯದಿ (!okVisitor.varAssigned(vVtxp->varScp())) ಅನುವರ್ತ;

            // Expression we are considering to substitute with
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ಧ್ರುವ size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            ಯದಿ (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) ಅನುವರ್ತ;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ಧ್ರುವ vscp = vVtxp->varScp();

            ಯದಿ (debug() >= 9) {
                vscp->dump_ವೃಕ್ಷ("substituting: ");
                substp->dump_ವೃಕ್ಷ("        with: ");
            }

            ಧ್ರುವ auto& readVscpsVec = okVisitor.readVscps();
            ಧ್ರುವ std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* ಧ್ರುವ dstVtxp = edgep->ತುದಿ()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                ಯದಿ (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_ಗಂಟು* nodep = substp;
                    ಯದಿ (AstNot* ಧ್ರುವ notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    ಯದಿ (!VN_IS(nodep, VarRef)) ಅನುವರ್ತ;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                ತಾರ್ಕಿಕ canInline = ಸತ್ಯ;
                ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    ಧ್ರುವ GateVarVertex* ಧ್ರುವ consVVertexp = dedge.ತುದಿ()->as<GateVarVertex>();
                    ಯದಿ (readVscps.ಎಣಿಕೆ(consVVertexp->varScp())) {
                        canInline = ಅಸತ್ಯ;
                        ಭಙ್ಗ;
                    }
                }

                ಯದಿ (!canInline) ಅನುವರ್ತ;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                ಯದಿ (logicp == dstVtxp->nodep()) {
                    // This is a bit involved. The graph tells us that the logic is circular
                    // (driver is same as sink), however, okVisitor rejects a circular driver
                    // and we would not reach here if the driver logic was actually circular.
                    // The reason we end up here is because during graph building, the driver
                    // was circular, however, after committing some substitutions to it, it
                    // has become non-circular due to V3Const being applied inside
                    // 'commitSubstitutions'. We will trust GateOkVisitor telling the truth
                    // that the logic is not actually circular, meaning this edge is not
                    // actually needed, can just delete it and move on.
                    VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    ಅನುವರ್ತ;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                ಪ್ರತಿ (AstVarScope* ಧ್ರುವ newVscp : okVisitor.readVscps()) {
                    GateVarVertex* ಧ್ರುವ varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            ಯದಿ (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_ವೃಕ್ಷ(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(ಅಸತ್ಯ);
        // Then propagate more complicated equations
        optimizeSignals(ಸತ್ಯ);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_ಗಂಟು*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](ಧ್ರುವ Pair& a, ಧ್ರುವ Pair& b) {  //
            ಪ್ರತ್ಯಾಗಮ a.second < b.second;
        });
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

ವರ್ಗ GateDedupeHash final : ಸಾರ್ವಜನಿಕ V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    ಸಂರಚನಾ Aux_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_ಗಂಟು_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ, Aux_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ> m_aux_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ;

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    ಸ್ಥಿರ ತಾರ್ಕಿಕ same(Ast_ಗಂಟು* node1p, Ast_ಗಂಟು* node2p) {
        // Regarding the complexity of this function 'same':
        // Applying this comparison function to a a set of n trees pairwise is O(n^2) in the
        // number of comparisons (number of pairs). AstNode::sameTree itself, is O(sizeOfTree) in
        // the worst case, which happens if the operands of sameTree are indeed identical copies,
        // which means this line is O(n^2*sizeOfTree), iff you are comparing identical copies of
        // the same tree. In practice the identity comparison over the pointers, and the short
        // circuiting in sameTree means that for comparing the same tree instance to itself, or
        // trees of different types/shapes is a lot closer to O(1), so this 'same' function is
        // Omega(n^2) and O(n^2*sizeOfTree), and in practice as we are mostly comparing the same
        // instance to itself or different trees, the complexity should be closer to the lower
        // bound.
        //
        // Also if you see where this 'same' function is used within isSame, it's only ever
        // comparing AstActive nodes, which are very likely not to compare equals (and for the
        // purposes of V3Gate, we probably only care about them either being identical instances,
        // or having the same sensitivities anyway, so if this becomes a problem, it can be
        // improved which should also speed things up), and AstNodeExpr for if conditions, which
        // are hopefully small.
        ಪ್ರತ್ಯಾಗಮ node1p == node2p || (node1p && node1p->same_ವೃಕ್ಷ(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    ತಾರ್ಕಿಕ isSame(Ast_ಗಂಟು* node1p, Ast_ಗಂಟು* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ಧ್ರುವ auto& aux2 = m_aux_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(VN_AS(node2p, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
        ಪ್ರತ್ಯಾಗಮ m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

ಸಾರ್ವಜನಿಕ:
    GateDedupeHash() = ಸ್ವಭಾವ;
    ~GateDedupeHash() = ಸ್ವಭಾವ;

    ಧ್ರುವ Ast_ಗಂಟು_Assign* hashAndFindDupe(Ast_ಗಂಟು_Assign* assignp, AstActive* activep,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ಧ್ರುವ auto it = m_dupFinder.findDuplicate(m_currRhsp, ಏತತ್);
        ಯದಿ (it != m_dupFinder.end()) ಪ್ರತ್ಯಾಗಮ m_aux_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(VN_AS(it->second, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)).parentp;

        // Insert new node
        m_dupFinder.ಸೇರಿಸು(m_currRhsp);
        m_aux_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(m_currRhsp) = m_auxCurRhsp;
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

ವರ್ಗ GateDedupeVarVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Given a node, it is visited to try to find the AstNodeAssign under
    // it that can used for dedupe.
    // Right now, only the following node trees are supported for dedupe.
    // 1. AstNodeAssign
    // 2. AstAlways -> AstNodeAssign
    //   (Note, the assign must also be the only node under the always)
    // 3. AstAlways -> AstNodeIf -> AstNodeAssign
    //   (Note, the IF must be the only node under the always,
    //    and the assign must be the only node under the if, other than the ifcond)
    // Any other ordering or node type, except for an AstComment, makes it not dedupable
    // AstExprStmt in the subtree of a node also makes the node not dedupable.

    // STATE
    GateDedupeHash m_ghash;  // Hash used to find dupes of rhs of assign
    Ast_ಗಂಟು_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_ifCondp = nullptr;  // IF condition that assign is under
    ತಾರ್ಕಿಕ m_always = ಅಸತ್ಯ;  // Assign is under an always
    ತಾರ್ಕಿಕ m_dedupable = ಸತ್ಯ;  // Determined the assign to be dedupable

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        ಯದಿ (!m_dedupable) ಪ್ರತ್ಯಾಗಮ;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        ಯದಿ (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; });
            ಪ್ರತ್ಯಾಗಮ;
        }

        m_dedupable = ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        ಯದಿ (!m_dedupable) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (!m_always) {
            m_always = ಸತ್ಯ;
            iterateAndNextConstNull(nodep->stmtsp());
            ಪ್ರತ್ಯಾಗಮ;
        }

        m_dedupable = ಅಸತ್ಯ;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        ಯದಿ (!m_dedupable) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; });
            iterateAndNextConstNull(nodep->thensp());
            ಪ್ರತ್ಯಾಗಮ;
        }

        m_dedupable = ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstComment*) override {}  // NOP
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು*) override { m_dedupable = ಅಸತ್ಯ; }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = ಸ್ವಭಾವ;
    ~GateDedupeVarVisitor() override = ಸ್ವಭಾವ;
    // PUBLIC METHODS
    Ast_ಗಂಟು_Var_Ref* findDupe(Ast_ಗಂಟು* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = ಅಸತ್ಯ;
        m_dedupable = ಸತ್ಯ;
        iterateConst(logicp);
        ಯದಿ (m_dedupable && m_assignp) {
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ lRefp = VN_CAST(lhsp, ಗಂಟು_Var_Ref);
            ಯದಿ (lRefp && lRefp->varScopep() == consumerVscp) {
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Assign* ಧ್ರುವ dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    ಪ್ರತ್ಯಾಗಮ static_cast<Ast_ಗಂಟು_Var_Ref*>(dup->lhsp());
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

ವರ್ಗ GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ಧ್ರುವ VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    ಶೂನ್ಯ visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        ಯದಿ (m_depth > GATE_DEDUP_MAX_DEPTH) ಪ್ರತ್ಯಾಗಮ;

        // Check that we haven't been here before
        ಯದಿ (vVtxp->varScp()->user2SetOnce()) ಪ್ರತ್ಯಾಗಮ;

        VL_RESTORER(m_depth);
        ++m_depth;
        ಯದಿ (!vVtxp->inSize1()) ಪ್ರತ್ಯಾಗಮ;

        Ast_ಗಂಟು_Var_Ref* dupRefp = nullptr;
        ಪ್ರತಿ (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        ಯದಿ (!dupRefp) ಪ್ರತ್ಯಾಗಮ;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* ಧ್ರುವ lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        ಧ್ರುವ GateOkVisitor okVisitor{lVtxp->nodep(), ಅಸತ್ಯ, ಸತ್ಯ};
        ಯದಿ (!okVisitor.isSimple()) ಪ್ರತ್ಯಾಗಮ;

        ++m_statDedupLogic;
        GateVarVertex* ಧ್ರುವ dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : vVtxp->outEdges().unlinkable()) {
            ಧ್ರುವ GateLogicVertex* ಧ್ರುವ consumerVtxp = edgep->ತುದಿ()->as<GateLogicVertex>();
            Ast_ಗಂಟು* ಧ್ರುವ consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            ಯದಿ (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } ಅನ್ಯಥಾ {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_ಗಂಟು_Var_Ref* refp) {
                    ಯದಿ (refp->varScopep() != vVtxp->varScp()) ಪ್ರತ್ಯಾಗಮ;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ newp = dupRefp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_ವೃಕ್ಷ(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        ಯಾವತ್ (V3GraphEdge* ಧ್ರುವ edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_ಗಂಟು_Var_Ref* visit(GateLogicVertex* lVtxp, ಧ್ರುವ GateVarVertex* consumerVtxp) {
        ಪ್ರತಿ (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        ಯದಿ (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            ಪ್ರತ್ಯಾಗಮ m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        ಪ್ರತಿ (V3GraphVertex& vtx : graph.vertices()) {
            ಯದಿ (GateVarVertex* ಧ್ರುವ vVtxp = vtx.cast<GateVarVertex>()) {
                ಯದಿ (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        ಪ್ರತಿ (V3GraphVertex& vtx : graph.vertices()) {
            ಯದಿ (GateVarVertex* ಧ್ರುವ vVtxp = vtx.cast<GateVarVertex>()) {
                ಯದಿ (vVtxp->is_ತುದಿ() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

ವರ್ಗ GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        ಧ್ರುವ AstVarRef* ಧ್ರುವ pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ಧ್ರುವ cRefp = VN_CAST(currSelp->fromp(), VarRef);
        ಯದಿ (!pRefp || !cRefp || !cRefp->same_ಗಂಟು(pRefp)) ಪ್ರತ್ಯಾಗಮ nullptr;  // not the same var

        ಧ್ರುವ AstConst* ಧ್ರುವ pstart = VN_CAST(prevSelp->lsbp(), Const);
        ಧ್ರುವ AstConst* ಧ್ರುವ cstart = VN_CAST(currSelp->lsbp(), Const);
        ಯದಿ (!pstart || !cstart) ಪ್ರತ್ಯಾಗಮ nullptr;  // too complicated

        ಯದಿ (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstSel{cRefp->fileline(), cRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
    }

    ಶೂನ್ಯ process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* ಧ್ರುವ lVtxp = edgep->fromp()->as<GateLogicVertex>();
            ಯದಿ (!lVtxp->outSize1()) ಅನುವರ್ತ;

            AstAlways* ಧ್ರುವ alwaysp = VN_CAST(lVtxp->nodep(), Always);
            ಯದಿ (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) ಪ್ರತ್ಯಾಗಮ;
            AstAssignW* ಧ್ರುವ assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            ಯದಿ (!assignp) ಅನುವರ್ತ;

            ಯದಿ (!VN_IS(assignp->lhsp(), Sel)) ಅನುವರ್ತ;

            // First assign with Sel-lhs, or not under the same active
            ಯದಿ (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                ಅನುವರ್ತ;
            }

            AstSel* ಧ್ರುವ prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ಧ್ರುವ currSelp = VN_AS(assignp->lhsp(), Sel);

            ಯದಿ (AstSel* ಧ್ರುವ newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_ವೃಕ್ಷ(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(ನೂತನ AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                ಯಾವತ್ (V3GraphEdge* ಧ್ರುವ iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* ಧ್ರುವ fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } ಅನ್ಯಥಾ {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        ಪ್ರತಿ (V3GraphVertex& vtx : graph.vertices()) {
            ಯದಿ (GateVarVertex* ಧ್ರುವ vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        ಪ್ರತಿ (GateLogicVertex* ಧ್ರುವ lVtxp : m_toRemove) {
            Ast_ಗಂಟು* ಧ್ರುವ nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

ವರ್ಗ GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    ಶೂನ್ಯ markRecurse(GateEitherVertex* vtxp) {
        ಯದಿ (vtxp->user()) ಪ್ರತ್ಯಾಗಮ;  // Already marked
        vtxp->user(ಸತ್ಯ);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        ಪ್ರತಿ (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* ಧ್ರುವ fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    ಶೂನ್ಯ mark() {
        m_graph.userClearVertices();
        ಪ್ರತಿ (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            ಯದಿ (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ warnUnused(ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep) {
        ಯದಿ (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Procedure* ಧ್ರುವ procedurep = VN_CAST(nodep, ಗಂಟು_Procedure)) {
            ಯದಿ (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ಧ್ರುವ AstLoop* ಧ್ರುವ loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, ಸತ್ಯ);
                });
        }
    }

    // Remove unused logic
    ಶೂನ್ಯ remove() {
        ಪ್ರತಿ (V3GraphVertex* ಧ್ರುವ vtxp : m_graph.vertices().unlinkable()) {
            ಯದಿ (GateLogicVertex* ಧ್ರುವ lVtxp = vtxp->cast<GateLogicVertex>()) {
                ಯದಿ (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_ಗಂಟು* ಧ್ರುವ nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
                }
            }
        }
    }

    explicit GateUnused(GateGraph& graph)
        : m_graph{graph} {
        mark();  // Mark all used vertices
        remove();  // Remove unused vertices
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

ಶೂನ್ಯ V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        ಯದಿ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        ಯದಿ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        ಯದಿ (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        ಯದಿ (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        ಯದಿ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_ವೃಕ್ಷ("gate", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
