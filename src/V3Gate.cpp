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

constexpr uint32_t GATE_DEDUP_MAX_ಆಳ = 20;

//######################################################################
// Gate graph classes

ವರ್ಗ Gate_Either_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Gate_Either_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    ತಾರ್ಕಿಕ m_reducible = ನಿಜ;  // True if this node should be able to be eliminated
    ತಾರ್ಕಿಕ m_dedupable = ನಿಜ;  // True if this node should be able to be deduped
    ತಾರ್ಕಿಕ m_consumed = ಸುಳ್ಳು;  // Output goes to something meaningful
    ತಾರ್ಕಿಕ m_staticInit = ಸುಳ್ಳು;  // True if this node is a static initializer
ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Gate_Either_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp} {}
    ~Gate_Either_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    ತಾರ್ಕಿಕ reducible() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_reducible; }
    ತಾರ್ಕಿಕ dedupable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dedupable; }
    ತಾರ್ಕಿಕ consumed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_consumed; }
    ತಾರ್ಕಿಕ staticInit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_staticInit; }
    ಶೂನ್ಯ ಗಣ_Consumed(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = ನಿಜ;
    }
    ಶೂನ್ಯ ಗಣ_Static_Init() { m_staticInit = ನಿಜ; }
    ಶೂನ್ಯ clearReducible(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ clearDedupable(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ clearReducibleAndDedupable(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_consumed ? "" : "dotted"; }
};

ವರ್ಗ Gate_Var_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Gate_Either_ಶೃಂಗ {
    VL_RTTI_IMPL(Gate_Var_ಶೃಂಗ, Gate_Either_ಶೃಂಗ)
    AstVarScope* ಸ್ಥಿರಾಂಕ m_varScp;
    ತಾರ್ಕಿಕ m_is_ತುದಿ = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ m_isClock = ಸುಳ್ಳು;
    Ast_ಗಂಟು* m_rstSyncNodep = ಶೂನ್ಯಸೂಚಕ;  // Used as reset and not in SenItem, in clocked always
    Ast_ಗಂಟು* m_rstAsyncNodep = ಶೂನ್ಯಸೂಚಕ;  // Used as reset and in SenItem, in clocked always
ಸಾರ್ವಜನಿಕ:
    Gate_Var_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* varScp)
        : Gate_Either_ಶೃಂಗ{graphp}
        , m_varScp{varScp} {}
    ~Gate_Var_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    AstVarScope* varScp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_varScp; }
    ತಾರ್ಕಿಕ is_ತುದಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_is_ತುದಿ; }
    ಶೂನ್ಯ ಗಣ_Is_ತುದಿ() { m_is_ತುದಿ = ನಿಜ; }
    ತಾರ್ಕಿಕ isClock() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isClock; }
    ಶೂನ್ಯ ಗಣ_Is_Clock() { m_isClock = ನಿಜ; }
    Ast_ಗಂಟು* rstSyncNodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rstSyncNodep; }
    ಶೂನ್ಯ rstSyncNodep(Ast_ಗಂಟು* nodep) { m_rstSyncNodep = nodep; }
    Ast_ಗಂಟು* rstAsyncNodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rstAsyncNodep; }
    ಶೂನ್ಯ rstAsyncNodep(Ast_ಗಂಟು* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    ಶೂನ್ಯ propagateAttrFrom(Gate_Var_ಶೃಂಗ* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        ಆದರೆ (fromp->isClock()) ಗಣ_Is_Clock();
    }

    // DOT debug
    std::string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ varScp()->name(); }
    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "green"; }
};

ವರ್ಗ Gate_Logic_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Gate_Either_ಶೃಂಗ {
    VL_RTTI_IMPL(Gate_Logic_ಶೃಂಗ, Gate_Either_ಶೃಂಗ)
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;
    Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_slow;  // In slow block
ಸಾರ್ವಜನಿಕ:
    Gate_Logic_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep, Ast_ಸಕ್ರಿಯ* activep, ತಾರ್ಕಿಕ slow)
        : Gate_Either_ಶೃಂಗ{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~Gate_Logic_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ nodep()->fileline(); }
    Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
    Ast_ಸಕ್ರಿಯ* activep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_activep; }
    ತಾರ್ಕಿಕ slow() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_slow; }

    // DOT debug
    std::string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_nodep->fileline()->ascii(); }
    std::string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "red"; }
};

ವರ್ಗ Gate_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    std::string dotLabel() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ std::ಗಮ್ಯ_string(ತೂಕ_2()); }

ಸಾರ್ವಜನಿಕ:
    Gate_ಅಂಚು(V3_ನಕ್ಷೆ* graphp, V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2)
        : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, ತೂಕ_2} {}
};

ವರ್ಗ Gate_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

ಸಾರ್ವಜನಿಕ:
    Gate_Var_ಶೃಂಗ* make_Var_ಶೃಂಗ(AstVarScope* vscp) {
        Gate_Var_ಶೃಂಗ* vVtxp = reinterpret_cast<Gate_Var_ಶೃಂಗ*>(vscp->user1p());
        ಆದರೆ (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = ಹೊಸ Gate_Var_ಶೃಂಗ{ಇದು, vscp};
            vscp->user1p(vVtxp);
            ಆದರೆ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->ಗಣ_Consumed("VirtIface");
            }
            ಆದರೆ (vscp->varp()->is_Sig_ಬಹಿರಂಗ()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->ಗಣ_Consumed("SigPublic");
            }
            ಆದರೆ (vscp->varp()->isIO() && vscp->scopep()->is_ತುದಿ()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->ಗಣ_Is_ತುದಿ();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->ಗಣ_Consumed("isTop");
            }
        }
        ಹಿಂತಿರುಗಿ vVtxp;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(Gate_Var_ಶೃಂಗ* srcp, Gate_Logic_ಶೃಂಗ* dstp, ಪೂರ್ಣಾಂಕ ತೂಕ_2) {
        ಹೊಸ Gate_ಅಂಚು{ಇದು, srcp, dstp, ತೂಕ_2};
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಅಂಚು(Gate_Logic_ಶೃಂಗ* srcp, Gate_Var_ಶೃಂಗ* dstp, ಪೂರ್ಣಾಂಕ ತೂಕ_2) {
        ಹೊಸ Gate_ಅಂಚು{ಇದು, srcp, dstp, ತೂಕ_2};
    }
};

//######################################################################
// GateGraph builder

ವರ್ಗ GateBuildVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE
    Gate_ನಕ್ಷೆ* m_graphp = ಹೊಸ Gate_ನಕ್ಷೆ{};  // The graph being built (var usages/dependencies)
    Gate_Logic_ಶೃಂಗ* m_logicVertexp = ಶೂನ್ಯಸೂಚಕ;  // Current statement being tracked, nullptr=ignored
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    ಸ್ಥಿರಾಂಕ AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope being processed
    Ast_ಸಕ್ರಿಯ* m_activep = ಶೂನ್ಯಸೂಚಕ;  // Current active
    ತಾರ್ಕಿಕ m_ಒಳ_Clocked_ಸಕ್ರಿಯ = ಸುಳ್ಳು;  // Underneath clocked active
    ತಾರ್ಕಿಕ m_ಒಳ_ಅಂಚು_ಸಕ್ರಿಯ = ಸುಳ್ಳು;  // Underneath edge active
    ತಾರ್ಕಿಕ m_ಒಳ_Static_ಸಕ್ರಿಯ = ಸುಳ್ಳು;  // Underneath static active
    ತಾರ್ಕಿಕ m_ಒಳ_Initial_ಸಕ್ರಿಯ = ಸುಳ್ಳು;  // Underneath initial active
    ತಾರ್ಕಿಕ m_ಒಳ_Sen_ಅಂಶ = ಸುಳ್ಳು;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    ಶೂನ್ಯ check_ಗಂಟು(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (nodep->isOutputter()) m_logicVertexp->ಗಣ_Consumed("outputter");
        ಆದರೆ (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->ಗಣ_Consumed("TimingControl");
        }
    }

    ಶೂನ್ಯ iterateLogic(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ slow = ಸುಳ್ಳು, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* nonReducibleReason = ಶೂನ್ಯಸೂಚಕ,
                      ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* consumeReason = ಶೂನ್ಯಸೂಚಕ) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = ಹೊಸ Gate_Logic_ಶೃಂಗ{m_graphp, nodep, m_activep, slow};
        ಆದರೆ (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಒಳ_Clocked_ಸಕ್ರಿಯ) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಒಳ_Static_ಸಕ್ರಿಯ || m_ಒಳ_Initial_ಸಕ್ರಿಯ) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->ಗಣ_Static_Init();
        }
        ಆದರೆ (consumeReason) m_logicVertexp->ಗಣ_Consumed(consumeReason);
        check_ಗಂಟು(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ಒಳ_Clocked_ಸಕ್ರಿಯ);
        VL_RESTORER(m_ಒಳ_ಅಂಚು_ಸಕ್ರಿಯ);
        VL_RESTORER(m_ಒಳ_Static_ಸಕ್ರಿಯ);
        VL_RESTORER(m_ಒಳ_Initial_ಸಕ್ರಿಯ);
        m_activep = nodep;
        m_ಒಳ_Clocked_ಸಕ್ರಿಯ = nodep->hasClocked();
        m_ಒಳ_ಅಂಚು_ಸಕ್ರಿಯ = nodep->sentreep() && nodep->sentreep()->has_ಅಂಚು();
        m_ಒಳ_Static_ಸಕ್ರಿಯ = nodep->hasStatic();
        m_ಒಳ_Initial_ಸಕ್ರಿಯ = nodep->hasInitial();

        // AstVarScope::user2 -> bool: Signal used in SenItem in *this* active block
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
        iterateChildrenConst(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? ಶೂನ್ಯಸೂಚಕ : "Multiple Stmts");
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggle* nodep) override {
        iterateLogic(nodep, ಸುಳ್ಳು, "CoverToggle", "CoverToggle");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        VL_RESTORER(m_ಒಳ_Sen_ಅಂಶ);
        m_ಒಳ_Sen_ಅಂಶ = ನಿಜ;
        ಆದರೆ (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, ಸುಳ್ಳು, ಶೂನ್ಯಸೂಚಕ, "senItem");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (!m_logicVertexp) ಹಿಂತಿರುಗಿ;

        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
        Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vVtxp = m_graphp->make_Var_ಶೃಂಗ(vscp);

        ಆದರೆ (m_ಒಳ_Sen_ಅಂಶ) {
            vVtxp->ಗಣ_Is_Clock();
            vscp->user2(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಒಳ_ಅಂಚು_ಸಕ್ರಿಯ && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            ಆದರೆ (vscp->user2()) {
                ಆದರೆ (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        ಆದರೆ (nodep->access().isWriteOrRW()) m_graphp->ಕೂಡಿಸು_ಅಂಚು(m_logicVertexp, vVtxp, 1);
        ಆದರೆ (nodep->access().isReadOrRW()) m_graphp->ಕೂಡಿಸು_ಅಂಚು(vVtxp, m_logicVertexp, 1);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), ಗಂಟು_ನಿಯೋಜಿಸು)
                      && VN_AS(nodep->backp(), ಗಂಟು_ನಿಯೋಜಿಸು)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (m_logicVertexp) check_ಗಂಟು(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::unique_ptr<Gate_ನಕ್ಷೆ> apply(AstNetlist* netlistp) {
        ಹಿಂತಿರುಗಿ std::unique_ptr<Gate_ನಕ್ಷೆ>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ಸ್ಥಿರ ಶೂನ್ಯ v3GateWarnSyncAsync(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : ನಕ್ಷೆ.vertices()) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvertexp = vtx.cast<Gate_Var_ಶೃಂಗ>()) {
            ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vvertexp->varScp();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ sp = vvertexp->rstSyncNodep();
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ap = vvertexp->rstAsyncNodep();
            ಆದರೆ (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                ಆದರೆ (!vscp->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::SYNCASYNCNET)) {
                    vscp->varp()->user2(ನಿಜ);  // Warn only once per signal
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
    ಸ್ಥಿರಾಂಕ AstVarScope* m_vscp = ಶೂನ್ಯಸೂಚಕ;  // Varscope we're trying to find
    ಪೂರ್ಣಾಂಕ m_ಸ್ಥಾನಾಂತರ = 0;  // Current offset of varscope
    ಪೂರ್ಣಾಂಕ m_ಸಿಕ್ಕಿತು_ಸ್ಥಾನಾಂತರ = 0;  // Found offset of varscope
    ತಾರ್ಕಿಕ m_ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (nodep->varScopep() == m_vscp && !nodep->user2() && !m_ಸಿಕ್ಕಿತು) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(ನಿಜ);
            m_ಸಿಕ್ಕಿತು_ಸ್ಥಾನಾಂತರ = m_ಸ್ಥಾನಾಂತರ;
            m_ಸಿಕ್ಕಿತು = ನಿಜ;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_ಸ್ಥಾನಾಂತರ << ") - " << nodep);
        }
        m_ಸ್ಥಾನಾಂತರ += nodep->dtypep()->ಅಗಲ();
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // PUBLIC METHODS
    ತಾರ್ಕಿಕ concat_ಸ್ಥಾನಾಂತರ(AstConcat* concatp, AstVarScope* vscp, ಪೂರ್ಣಾಂಕ& offsetr) {
        m_vscp = vscp;
        m_ಸ್ಥಾನಾಂತರ = 0;
        m_ಸಿಕ್ಕಿತು = ಸುಳ್ಳು;
        // Iterate
        iterateConst(concatp);
        offsetr = m_ಸಿಕ್ಕಿತು_ಸ್ಥಾನಾಂತರ;
        ಹಿಂತಿರುಗಿ m_ಸಿಕ್ಕಿತು;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

ವರ್ಗ GateOkVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // RETURN STATE
    ತಾರ್ಕಿಕ m_isSimple = ನಿಜ;  // Set false when we know it isn't simple
    std::ಸದಿಶ<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_substitutionp = ಶೂನ್ಯಸೂಚಕ;  // What to replace the variable with

    // STATE
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* m_lhsVarRef = ಶೂನ್ಯಸೂಚಕ;
    ಪೂರ್ಣಾಂಕ m_ops = 0;  // Operation count

    // METHODS
    ಶೂನ್ಯ clearSimple(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* because) {
        ಆದರೆ (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = ಸುಳ್ಳು;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಆದರೆ (!m_isSimple) ಹಿಂತಿರುಗಿ;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        ಆದರೆ (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        ಆದರೆ (nodep->access().isRW()) {
            clearSimple("R/W");
            ಹಿಂತಿರುಗಿ;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
        ಆದರೆ (nodep->access().isWriteOnly()) {
            ಆದರೆ (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // TODO: possible bug, should it be >= 1 as add is below?
            ಆದರೆ (m_readVscps.ಗಾತ್ರ_2() > 1) {
                ಆದರೆ (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ತಳ್ಳು_ಹಿಂಭಾಗ(vscp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (!m_isSimple) ಹಿಂತಿರುಗಿ;

        m_substitutionp = nodep->rhsp();
        ಆದರೆ (!VN_IS(nodep->lhsp(), ಗಂಟು_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildrenConst(nodep);
        }
        ಆದರೆ (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCReset* /*nodep*/) override {
        ಆದರೆ (!m_isSimple) ಹಿಂತಿರುಗಿ;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಹೊಂದಿಕೆ_Masked* nodep) override {
        ಆದರೆ (!m_isSimple) ಹಿಂತಿರುಗಿ;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (!m_isSimple) ಹಿಂತಿರುಗಿ;  // Fastpath

        ಆದರೆ (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->is_ಶುದ್ಧ() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            ಹಿಂತಿರುಗಿ;
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
        ಆದರೆ (!m_substitutionp) {
            clearSimple("No assignment found\n");
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_isSimple && m_lhsVarRef) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp : m_readVscps) {
                ಆದರೆ (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }
    ~GateOkVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // PUBLIC METHODS
    ತಾರ್ಕಿಕ isSimple() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_isSimple; }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substitutionp() ಸ್ಥಿರಾಂಕ {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        ಹಿಂತಿರುಗಿ m_substitutionp;
    }
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVarScope*>& readVscps() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_readVscps; }
    ತಾರ್ಕಿಕ varAssigned(ಸ್ಥಿರಾಂಕ AstVarScope* scopep) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

ವರ್ಗ GateInline final {
    ಬಳಸಿ Substitutions = std::unordered_map<AstVarScope*, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_ಗಂಟು, Substitutions> m_substitutions;

    // STATE
    Gate_ನಕ್ಷೆ& m_ನಕ್ಷೆ;
    ಗಾತ್ರ_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_ಗಂಟು*, ಗಾತ್ರ_t> m_hasPending;
    ಗಾತ್ರ_t m_statInlined = 0;  // Statistic tracking - signals inlined
    ಗಾತ್ರ_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    ಗಾತ್ರ_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isCheap(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Constant is cheap
        ಆದರೆ (VN_IS(exprp, Const)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Variable reference is cheap
        ಆದರೆ (VN_IS(exprp, ಗಂಟು_Var_Ref)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstSel* ಸ್ಥಿರಾಂಕ selp = VN_CAST(exprp, Sel)) {
            ಆದರೆ (!isCheap(selp->fromp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (!selp->isWide()) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಆದರೆ (!VN_IS(selp->lsbp(), Const)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (selp->lsbConst() % VL_EDATASIZE != 0) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // AstArraySel is cheap if the fromp is cheap
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstArraySel* ಸ್ಥಿರಾಂಕ aselp = VN_CAST(exprp, ArraySel)) {
            ಹಿಂತಿರುಗಿ isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ತಾರ್ಕಿಕ shouldInline(Gate_Var_ಶೃಂಗ* vVtxp, Gate_Logic_ಶೃಂಗ* lVtxp, ಗಾತ್ರ_t nReads,
                      Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substp, ತಾರ್ಕಿಕ allow_Multi_ಒಳ) {
        // Always inline constants
        ಆದರೆ (VN_IS(substp, Const)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Don't inline non-constant static initializers - these are scheduled differently
        ಆದರೆ (lVtxp->staticInit()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Inline simple variable references
        ಆದರೆ (VN_IS(substp, VarRef)) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Only inline arrays if a simple variable or constant
        ಆದರೆ (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        ಆದರೆ (nReads == 0) ಹಿಂತಿರುಗಿ ನಿಜ;
        // If it reads one variable, inline if not wide, or if cheap
        ಆದರೆ (nReads == 1 && (!substp->isWide() || isCheap(substp))) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Don't inline on first round if reads more than one variable
        ಆದರೆ (nReads > 1 && !allow_Multi_ಒಳ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        ಪೂರ್ಣಾಂಕ n = 0;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vVtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
            ಸ್ಥಿರಾಂಕ Gate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dstVtxp = ಅಂಚು.ತುದಿ()->as<Gate_Logic_ಶೃಂಗ>();
            // Ignore slow code, or if the destination is not used
            ಆದರೆ (dstVtxp->slow()) ಮುಂದುವರಿ;
            ಆದರೆ (dstVtxp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು() && !dstVtxp->consumed()) ಮುಂದುವರಿ;
            n += ಅಂಚು.ತೂಕ_2();
            ಆದರೆ (n > 1) {
                ++m_statNotInlined;
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ ದಾಖಲೆ_Substitution(AstVarScope* vscp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* substp, Ast_ಗಂಟು* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_substitutions(logicp).emplace(vscp, ಶೂನ್ಯಸೂಚಕ);
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ = substp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
    }

    ಶೂನ್ಯ commitSubstitutions(Ast_ಗಂಟು* logicp) {
        ಆದರೆ (!m_hasPending.erase(logicp)) ಹಿಂತಿರುಗಿ;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.ಖಾಲಿಗಳು(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarScope*> replaced(substitutions.ಗಾತ್ರ_2() * 2);

        ಸ್ಥಿರಾಂಕ std::function<ಶೂನ್ಯ(Ast_ಗಂಟು_Var_Ref*)> ಭೇಟಿ = [&](Ast_ಗಂಟು_Var_Ref* nodep) -> ಶೂನ್ಯ {
            // See if this variable has a substitution
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it = substitutions.ಕಂಡುಹಿಡಿ(vscp);
            ಆದರೆ (it == substitutions.ಕೊನೆ()) ಹಿಂತಿರುಗಿ;

            // Do not substitute circular logic
            ಆದರೆ (!replaced.ಸೇರಿಸು(vscp).ಎರಡನೇ) ಹಿಂತಿರುಗಿ;

            // Substitute nodep with substp
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ substp = it->ಎರಡನೇ;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, ಗಂಟು_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = substp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            ಆದರೆ (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            ಆದರೆ (Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(newp, ಗಂಟು_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            // Recursively substitute the new tree
            newp->foreach(ಭೇಟಿ);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(ಭೇಟಿ);

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : substitutions) pair.ಎರಡನೇ->delete_ವೃಕ್ಷ();
        substitutions.clear();
    }

    ಶೂನ್ಯ optimize_ಸಂಕೇತಗಳು(ತಾರ್ಕಿಕ allow_Multi_ಒಳ) {
        // Consider "inlining" variables
        ಸ್ವಯಂಚಾಲಿತ& vertices = m_ನಕ್ಷೆ.vertices();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ff_ಗಮ್ಯ_Var_Vtx = [&](V3_ನಕ್ಷೆ_ಶೃಂಗ::List::iterator it) {
            ಆಗ (it != vertices.ಕೊನೆ() && !(*it).is<Gate_Var_ಶೃಂಗ>()) ++it;
            ಹಿಂತಿರುಗಿ it;
        };
        V3_ನಕ್ಷೆ_ಶೃಂಗ::List::iterator vIt = ff_ಗಮ್ಯ_Var_Vtx(vertices.begin());
        ಆಗ (vIt != vertices.ಕೊನೆ()) {
            Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vVtxp = (*vIt).as<Gate_Var_ಶೃಂಗ>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ff_ಗಮ್ಯ_Var_Vtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            ಆದರೆ (!vVtxp->ಒಳ_Size1()) ಮುಂದುವರಿ;

            // Can't inline if non-reducible, etc
            ಆದರೆ (!vVtxp->reducible()) ಮುಂದುವರಿ;

            // Grab the driving logic
            Gate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp
                = vVtxp->ಒಳ_ಅಂಚುಗಳು().frontp()->fromp()->as<Gate_Logic_ಶೃಂಗ>();
            ಆದರೆ (!lVtxp->reducible()) ಮುಂದುವರಿ;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ಸ್ಥಿರಾಂಕ GateOkVisitor okVisitor{logicp, ಸುಳ್ಳು, ಸುಳ್ಳು};

            // Was it ok?
            ಆದರೆ (!okVisitor.isSimple()) ಮುಂದುವರಿ;
            // If the varScope is already removed from logicp, no need to try substitution.
            ಆದರೆ (!okVisitor.varAssigned(vVtxp->varScp())) ಮುಂದುವರಿ;

            // Expression we are considering to substitute with
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nReads = okVisitor.readVscps().ಗಾತ್ರ_2();

            // Check if it should be inlined
            ಆದರೆ (!shouldInline(vVtxp, lVtxp, nReads, substp, allow_Multi_ಒಳ)) ಮುಂದುವರಿ;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vVtxp->varScp();

            ಆದರೆ (debug() >= 9) {
                vscp->dump_ವೃಕ್ಷ("substituting: ");
                substp->dump_ವೃಕ್ಷ("        with: ");
            }

            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& readVscpsVec = okVisitor.readVscps();
            ಸ್ಥಿರಾಂಕ std::unordered_ಗಣ<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.ಕೊನೆ()};

            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : vVtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
                Gate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dstVtxp = edgep->ತುದಿ()->as<Gate_Logic_ಶೃಂಗ>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                ಆದರೆ (VN_IS(dstVtxp->nodep(), Sen_ಅಂಶ)) {
                    Ast_ಗಂಟು* nodep = substp;
                    ಆದರೆ (AstNot* ಸ್ಥಿರಾಂಕ notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    ಆದರೆ (!VN_IS(nodep, VarRef)) ಮುಂದುವರಿ;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                ತಾರ್ಕಿಕ canInline = ನಿಜ;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& dedge : dstVtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                    ಸ್ಥಿರಾಂಕ Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ consVVertexp = dedge.ತುದಿ()->as<Gate_Var_ಶೃಂಗ>();
                    ಆದರೆ (readVscps.ಎಣಿಕೆ_2(consVVertexp->varScp())) {
                        canInline = ಸುಳ್ಳು;
                        ನಿಲ್ಲಿಸು;
                    }
                }

                ಆದರೆ (!canInline) ಮುಂದುವರಿ;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                ಆದರೆ (logicp == dstVtxp->nodep()) {
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
                    ಮುಂದುವರಿ;
                }

                ದಾಖಲೆ_Substitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ newVscp : okVisitor.readVscps()) {
                    Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ varvertexp = m_ನಕ್ಷೆ.make_Var_ಶೃಂಗ(newVscp);
                    m_ನಕ್ಷೆ.ಕೂಡಿಸು_ಅಂಚು(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            ಆದರೆ (vVtxp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_ನಕ್ಷೆ), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_ನಕ್ಷೆ), lVtxp);
            }
        }
    }

    ಸ್ಪಷ್ಟ GateInline(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ)
        : m_ನಕ್ಷೆ{ನಕ್ಷೆ} {
        // Find gate interconnect and optimize
        ನಕ್ಷೆ.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimize_ಸಂಕೇತಗಳು(ಸುಳ್ಳು);
        // Then propagate more complicated equations
        optimize_ಸಂಕೇತಗಳು(ನಿಜ);
        // Commit substitutions in insertion order for stability
        ಬಳಸಿ Pair = std::pair<Ast_ಗಂಟು*, ಗಾತ್ರ_t>;
        std::ಸದಿಶ<Pair> pending{m_hasPending.begin(), m_hasPending.ಕೊನೆ()};
        std::ವಿಂಗಡಿಸು(pending.begin(), pending.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ Pair& a, ಸ್ಥಿರಾಂಕ Pair& b) {  //
            ಹಿಂತಿರುಗಿ a.ಎರಡನೇ < b.ಎರಡನೇ;
        });
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : pending) commitSubstitutions(pair.ಮೊದಲ);
    }

    ~GateInline() {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Gate reads replaced", m_statRefs);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ) { GateInline{ನಕ್ಷೆ}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

ವರ್ಗ GateDedupeHash final : ಸಾರ್ವಜನಿಕ V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    ರಚನೆ Aux_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        Ast_ಸಕ್ರಿಯ* activep = ಶೂನ್ಯಸೂಚಕ;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp = ಶೂನ್ಯಸೂಚಕ;
        // Parent AstNodeAssign* for this rhsp
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* parentp = ಶೂನ್ಯಸೂಚಕ;
    };
    AstUser2Allocator<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ, Aux_Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ> m_aux_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ;

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_currRhsp = ಶೂನ್ಯಸೂಚಕ;  // Current node we are searching for duplicates of
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
        ಹಿಂತಿರುಗಿ node1p == node2p || (node1p && node1p->same_ವೃಕ್ಷ(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    ತಾರ್ಕಿಕ isSame(Ast_ಗಂಟು* node1p, Ast_ಗಂಟು* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& aux2 = m_aux_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(VN_AS(node2p, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
        ಹಿಂತಿರುಗಿ m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

ಸಾರ್ವಜನಿಕ:
    GateDedupeHash() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~GateDedupeHash() = ಪೂರ್ವನಿಯೋಜಿತ;

    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* hash_And_ಕಂಡುಹಿಡಿ_Dupe(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* assignp, Ast_ಸಕ್ರಿಯ* activep,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_dupFinder.ಕಂಡುಹಿಡಿ_Duplicate(m_currRhsp, ಇದು);
        ಆದರೆ (it != m_dupFinder.ಕೊನೆ()) ಹಿಂತಿರುಗಿ m_aux_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(VN_AS(it->ಎರಡನೇ, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)).parentp;

        // Insert new node
        m_dupFinder.ಸೇರಿಸು(m_currRhsp);
        m_aux_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ(m_currRhsp) = m_auxCurRhsp;
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
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
    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* m_assignp = ಶೂನ್ಯಸೂಚಕ;  // Assign found for dedupe
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_ifCondp = ಶೂನ್ಯಸೂಚಕ;  // IF condition that assign is under
    ತಾರ್ಕಿಕ m_always = ಸುಳ್ಳು;  // Assign is under an always
    ತಾರ್ಕಿಕ m_dedupable = ನಿಜ;  // Determined the assign to be dedupable

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ನಿಯೋಜಿಸು* nodep) override {
        ಆದರೆ (!m_dedupable) ಹಿಂತಿರುಗಿ;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        ಆದರೆ (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt*) { ಹಿಂತಿರುಗಿ ನಿಜ; });
            ಹಿಂತಿರುಗಿ;
        }

        m_dedupable = ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (!m_dedupable) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (!m_always) {
            m_always = ನಿಜ;
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->stmtsp());
            ಹಿಂತಿರುಗಿ;
        }

        m_dedupable = ಸುಳ್ಳು;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        ಆದರೆ (!m_dedupable) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt*) { ಹಿಂತಿರುಗಿ ನಿಜ; });
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
            ಹಿಂತಿರುಗಿ;
        }

        m_dedupable = ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstComment*) override {}  // NOP
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು*) override { m_dedupable = ಸುಳ್ಳು; }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~GateDedupeVarVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // PUBLIC METHODS
    Ast_ಗಂಟು_Var_Ref* ಕಂಡುಹಿಡಿ_Dupe(Ast_ಗಂಟು* logicp, AstVarScope* ಗ್ರಾಹಕ_Vscp, Ast_ಸಕ್ರಿಯ* activep) {
        m_assignp = ಶೂನ್ಯಸೂಚಕ;
        m_ifCondp = ಶೂನ್ಯಸೂಚಕ;
        m_always = ಸುಳ್ಳು;
        m_dedupable = ನಿಜ;
        iterateConst(logicp);
        ಆದರೆ (m_dedupable && m_assignp) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ lRefp = VN_CAST(lhsp, ಗಂಟು_Var_Ref);
            ಆದರೆ (lRefp && lRefp->varScopep() == ಗ್ರಾಹಕ_Vscp) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ dup
                    = m_ghash.hash_And_ಕಂಡುಹಿಡಿ_Dupe(m_assignp, activep, m_ifCondp)) {
                    ಹಿಂತಿರುಗಿ static_cast<Ast_ಗಂಟು_Var_Ref*>(dup->lhsp());
                }
            }
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

ವರ್ಗ GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;

    // STATE
    ಗಾತ್ರ_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_ಆಳ = 0;  // Iteration depth

    ಶೂನ್ಯ ಭೇಟಿ(Gate_Var_ಶೃಂಗ* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        ಆದರೆ (m_ಆಳ > GATE_DEDUP_MAX_ಆಳ) ಹಿಂತಿರುಗಿ;

        // Check that we haven't been here before
        ಆದರೆ (vVtxp->varScp()->user2_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;

        VL_RESTORER(m_ಆಳ);
        ++m_ಆಳ;
        ಆದರೆ (!vVtxp->ಒಳ_Size1()) ಹಿಂತಿರುಗಿ;

        Ast_ಗಂಟು_Var_Ref* dupRefp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vVtxp->ಒಳ_ಅಂಚುಗಳು()) {
            dupRefp = ಭೇಟಿ(ಅಂಚು.fromp()->as<Gate_Logic_ಶೃಂಗ>(), vVtxp);
        }
        ಆದರೆ (!dupRefp) ಹಿಂತಿರುಗಿ;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        Gate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp = vVtxp->ಒಳ_ಅಂಚುಗಳು().frontp()->fromp()->as<Gate_Logic_ಶೃಂಗ>();
        ಸ್ಥಿರಾಂಕ GateOkVisitor okVisitor{lVtxp->nodep(), ಸುಳ್ಳು, ನಿಜ};
        ಆದರೆ (!okVisitor.isSimple()) ಹಿಂತಿರುಗಿ;

        ++m_statDedupLogic;
        Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dupVVtxp = dupRefp->varScopep()->user1u().ಗಮ್ಯ<Gate_Var_ಶೃಂಗ*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : vVtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
            ಸ್ಥಿರಾಂಕ Gate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಗ್ರಾಹಕ_Vtxp = edgep->ತುದಿ()->as<Gate_Logic_ಶೃಂಗ>();
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ consumerp = ಗ್ರಾಹಕ_Vtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << ಗ್ರಾಹಕ_Vtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            ಆದರೆ (lVtxp == ಗ್ರಾಹಕ_Vtxp) {
                UINFO(9, "skipping as self-recirculates");
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_ಗಂಟು_Var_Ref* refp) {
                    ಆದರೆ (refp->varScopep() != vVtxp->varScp()) ಹಿಂತಿರುಗಿ;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ newp = dupRefp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
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
        ಆಗ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep = vVtxp->ಒಳ_ಅಂಚುಗಳು().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_ಗಂಟು_Var_Ref* ಭೇಟಿ(Gate_Logic_ಶೃಂಗ* lVtxp, ಸ್ಥಿರಾಂಕ Gate_Var_ಶೃಂಗ* ಗ್ರಾಹಕ_Vtxp) {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : lVtxp->ಒಳ_ಅಂಚುಗಳು()) ಭೇಟಿ(ಅಂಚು.fromp()->as<Gate_Var_ಶೃಂಗ>());

        ಆದರೆ (lVtxp->dedupable() && ಗ್ರಾಹಕ_Vtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            ಹಿಂತಿರುಗಿ m_varVisitor.ಕಂಡುಹಿಡಿ_Dupe(lVtxp->nodep(), ಗ್ರಾಹಕ_Vtxp->varScp(), lVtxp->activep());
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    ಸ್ಪಷ್ಟ GateDedupe(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vVtxp = vtx.cast<Gate_Var_ಶೃಂಗ>()) {
                ಆದರೆ (vVtxp->isClock()) ಭೇಟಿ(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vVtxp = vtx.cast<Gate_Var_ಶೃಂಗ>()) {
                ಆದರೆ (vVtxp->is_ತುದಿ() && vVtxp->varScp()->varp()->isWritable()) ಭೇಟಿ(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ) { GateDedupe{ನಕ್ಷೆ}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

ವರ್ಗ Gate_ವಿಲೀನ_Assignments final {
    Gate_ನಕ್ಷೆ& m_ನಕ್ಷೆ;
    ಗಾತ್ರ_t m_stat_ನಿಯೋಜಿಸು_Merged = 0;  // Statistic tracking
    std::ಸದಿಶ<Gate_Logic_ಶೃಂಗ*> m_ಗಮ್ಯ_Remove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* ವಿಲೀನ(AstSel* prevSelp, AstSel* currSelp) {
        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ಸ್ಥಿರಾಂಕ cRefp = VN_CAST(currSelp->fromp(), VarRef);
        ಆದರೆ (!pRefp || !cRefp || !cRefp->same_ಗಂಟು(pRefp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // not the same var

        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ pstart = VN_CAST(prevSelp->lsbp(), Const);
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ cstart = VN_CAST(currSelp->lsbp(), Const);
        ಆದರೆ (!pstart || !cstart) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // too complicated

        ಆದರೆ (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            ಹಿಂತಿರುಗಿ ಹೊಸ AstSel{cRefp->fileline(), cRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು), currSelp->lsbConst(),
                              prevSelp->ಅಗಲ_Const() + currSelp->ಅಗಲ_Const()};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
    }

    ಶೂನ್ಯ process(Gate_Var_ಶೃಂಗ* vVtxp) {
        Gate_Logic_ಶೃಂಗ* prevLVtxp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ನಿಯೋಜಿಸು_W* prevAssignp = ಶೂನ್ಯಸೂಚಕ;

        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : vVtxp->ಒಳ_ಅಂಚುಗಳು().unlinkable()) {
            Gate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp = edgep->fromp()->as<Gate_Logic_ಶೃಂಗ>();
            ಆದರೆ (!lVtxp->ಹೊರಹೋಗು_Size1()) ಮುಂದುವರಿ;

            AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = VN_CAST(lVtxp->nodep(), Always);
            ಆದರೆ (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) ಹಿಂತಿರುಗಿ;
            Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assignp = VN_CAST(alwaysp->stmtsp(), ನಿಯೋಜಿಸು_W);
            ಆದರೆ (!assignp) ಮುಂದುವರಿ;

            ಆದರೆ (!VN_IS(assignp->lhsp(), Sel)) ಮುಂದುವರಿ;

            // First assign with Sel-lhs, or not under the same active
            ಆದರೆ (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                ಮುಂದುವರಿ;
            }

            AstSel* ಸ್ಥಿರಾಂಕ prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ಸ್ಥಿರಾಂಕ currSelp = VN_AS(assignp->lhsp(), Sel);

            ಆದರೆ (AstSel* ಸ್ಥಿರಾಂಕ newSelp = ವಿಲೀನ(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_ವೃಕ್ಷ(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(ಹೊಸ AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                                assignp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtype_Chg_ಅಗಲ_Signed(prevAssignp->ಅಗಲ() + assignp->ಅಗಲ(),
                                                 prevAssignp->ಅಗಲ() + assignp->ಅಗಲ(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_ಗಮ್ಯ_Remove.emplace_ಹಿಂಭಾಗ(lVtxp);

                // Update the graph
                ಆಗ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ iedgep = lVtxp->ಒಳ_ಅಂಚುಗಳು().frontp()) {
                    Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVtxp = iedgep->fromp()->as<Gate_Var_ಶೃಂಗ>();
                    m_ನಕ್ಷೆ.ಕೂಡಿಸು_ಅಂಚು(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_stat_ನಿಯೋಜಿಸು_Merged;
            } ಇಲ್ಲದಿದ್ದರೆ {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    ಸ್ಪಷ್ಟ Gate_ವಿಲೀನ_Assignments(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ)
        : m_ನಕ್ಷೆ{ನಕ್ಷೆ} {
        UINFO(6, "mergeAssigns");
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : ನಕ್ಷೆ.vertices()) {
            ಆದರೆ (Gate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vVtxp = vtx.cast<Gate_Var_ಶೃಂಗ>()) process(vVtxp);
        }
        // Delete merged assignments
        ಗಾಗಿ (Gate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp : m_ಗಮ್ಯ_Remove) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_ನಕ್ಷೆ), lVtxp);
        }
    }

    ~Gate_ವಿಲೀನ_Assignments() {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Gate assign merged", m_stat_ನಿಯೋಜಿಸು_Merged);
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ) { Gate_ವಿಲೀನ_Assignments{ನಕ್ಷೆ}; }
};

//######################################################################
// GateUnused

ವರ್ಗ Gate_ಬಳಸದ final {
    // STATE
    Gate_ನಕ್ಷೆ& m_ನಕ್ಷೆ;

    // METHODS

    ಶೂನ್ಯ markRecurse(Gate_Either_ಶೃಂಗ* vtxp) {
        ಆದರೆ (vtxp->user()) ಹಿಂತಿರುಗಿ;  // Already marked
        vtxp->user(ನಿಜ);
        vtxp->ಗಣ_Consumed("propagated");
        // Walk sources and mark them too
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
            Gate_Either_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVtxp = static_cast<Gate_Either_ಶೃಂಗ*>(ಅಂಚು.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    ಶೂನ್ಯ mark() {
        m_ನಕ್ಷೆ.userClearVertices();
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : m_ನಕ್ಷೆ.vertices()) {
            Gate_Either_ಶೃಂಗ& eVtx = static_cast<Gate_Either_ಶೃಂಗ&>(vtx);
            ಆದರೆ (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ warn_ಬಳಸದ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::UNUSEDLOOP)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ procedurep = VN_CAST(nodep, ಗಂಟು_Procedure)) {
            ಆದರೆ (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ಸ್ಥಿರಾಂಕ AstLoop* ಸ್ಥಿರಾಂಕ loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNUSEDLOOP, ನಿಜ);
                });
        }
    }

    // Remove unused logic
    ಶೂನ್ಯ remove() {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_ನಕ್ಷೆ.vertices().unlinkable()) {
            ಆದರೆ (Gate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp = vtxp->cast<Gate_Logic_ಶೃಂಗ>()) {
                ಆದರೆ (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = lVtxp->nodep();
                    warn_ಬಳಸದ(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlink_Fr_ಹಿಂಭಾಗ();
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    VL_DO_DANGLING(lVtxp->unlinkDelete(&m_ನಕ್ಷೆ), lVtxp);
                }
            }
        }
    }

    ಸ್ಪಷ್ಟ Gate_ಬಳಸದ(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ)
        : m_ನಕ್ಷೆ{ನಕ್ಷೆ} {
        mark();  // Mark all used vertices
        remove();  // Remove unused vertices
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Gate_ನಕ್ಷೆ& ನಕ್ಷೆ) { Gate_ಬಳಸದ{ನಕ್ಷೆ}; }
};

//######################################################################
// Pass entry point

ಶೂನ್ಯ V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<Gate_ನಕ್ಷೆ> graphp = GateBuildVisitor::apply(netlistp);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->remove_Redundant_ಅಂಚುಗಳು_ಮೊತ್ತ(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        Gate_ಬಳಸದ::apply(*graphp);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        ಆದರೆ (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        ಆದರೆ (v3Global.opt.fAssemble()) {
            Gate_ವಿಲೀನ_Assignments::apply(*graphp);
            ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        Gate_ಬಳಸದ::apply(*graphp);
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_ವೃಕ್ಷ("gate", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
