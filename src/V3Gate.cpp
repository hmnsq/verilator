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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Gate.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DupFinder.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_ਡੂੰਘਾਈ = 20;

//######################################################################
// Gate graph classes

ਵਰਗ Gate_Either_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Gate_Either_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    ਤਾਰਕਿਕ m_reducible = ਸੱਚ;  // True if this node should be able to be eliminated
    ਤਾਰਕਿਕ m_dedupable = ਸੱਚ;  // True if this node should be able to be deduped
    ਤਾਰਕਿਕ m_consumed = ਝੂਠ;  // Output goes to something meaningful
    ਤਾਰਕਿਕ m_staticInit = ਝੂਠ;  // True if this node is a static initializer
ਜਨਤਕ:
    ਸਪਸ਼ਟ Gate_Either_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp} {}
    ~Gate_Either_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // ACCESSORS
    ਤਾਰਕਿਕ reducible() ਅਚਲ { ਵਾਪਸ m_reducible; }
    ਤਾਰਕਿਕ dedupable() ਅਚਲ { ਵਾਪਸ m_dedupable; }
    ਤਾਰਕਿਕ consumed() ਅਚਲ { ਵਾਪਸ m_consumed; }
    ਤਾਰਕਿਕ staticInit() ਅਚਲ { ਵਾਪਸ m_staticInit; }
    ਖਾਲੀ ਸੈੱਟ_Consumed(ਅਚਲ ਅੱਖਰ* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = ਸੱਚ;
    }
    ਖਾਲੀ ਸੈੱਟ_Static_Init() { m_staticInit = ਸੱਚ; }
    ਖਾਲੀ clearReducible(ਅਚਲ ਅੱਖਰ* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = ਝੂਠ;
    }
    ਖਾਲੀ clearDedupable(ਅਚਲ ਅੱਖਰ* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = ਝੂਠ;
    }
    ਖਾਲੀ clearReducibleAndDedupable(ਅਚਲ ਅੱਖਰ* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ਅਚਲ override { ਵਾਪਸ m_consumed ? "" : "dotted"; }
};

ਵਰਗ Gate_Var_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Gate_Either_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Gate_Var_ਸ਼ੀਰਸ਼, Gate_Either_ਸ਼ੀਰਸ਼)
    AstVarScope* ਅਚਲ m_varScp;
    ਤਾਰਕਿਕ m_is_ਸਿਖਰ = ਝੂਠ;
    ਤਾਰਕਿਕ m_isClock = ਝੂਠ;
    Ast_ਗੰਢ* m_rstSyncNodep = ਖਾਲੀ_ਸੂਚਕ;  // Used as reset and not in SenItem, in clocked always
    Ast_ਗੰਢ* m_rstAsyncNodep = ਖਾਲੀ_ਸੂਚਕ;  // Used as reset and in SenItem, in clocked always
ਜਨਤਕ:
    Gate_Var_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* varScp)
        : Gate_Either_ਸ਼ੀਰਸ਼{graphp}
        , m_varScp{varScp} {}
    ~Gate_Var_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // ACCESSORS
    AstVarScope* varScp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_varScp; }
    ਤਾਰਕਿਕ is_ਸਿਖਰ() ਅਚਲ { ਵਾਪਸ m_is_ਸਿਖਰ; }
    ਖਾਲੀ ਸੈੱਟ_Is_ਸਿਖਰ() { m_is_ਸਿਖਰ = ਸੱਚ; }
    ਤਾਰਕਿਕ isClock() ਅਚਲ { ਵਾਪਸ m_isClock; }
    ਖਾਲੀ ਸੈੱਟ_Is_Clock() { m_isClock = ਸੱਚ; }
    Ast_ਗੰਢ* rstSyncNodep() ਅਚਲ { ਵਾਪਸ m_rstSyncNodep; }
    ਖਾਲੀ rstSyncNodep(Ast_ਗੰਢ* nodep) { m_rstSyncNodep = nodep; }
    Ast_ਗੰਢ* rstAsyncNodep() ਅਚਲ { ਵਾਪਸ m_rstAsyncNodep; }
    ਖਾਲੀ rstAsyncNodep(Ast_ਗੰਢ* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    ਖਾਲੀ propagateAttrFrom(Gate_Var_ਸ਼ੀਰਸ਼* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        ਜੇ (fromp->isClock()) ਸੈੱਟ_Is_Clock();
    }

    // DOT debug
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ varScp()->name(); }
    std::string dotColor() ਅਚਲ override { ਵਾਪਸ "green"; }
};

ਵਰਗ Gate_Logic_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Gate_Either_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Gate_Logic_ਸ਼ੀਰਸ਼, Gate_Either_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ* ਅਚਲ m_nodep;
    Ast_ਸਰਗਰਮ* ਅਚਲ m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ਅਚਲ ਤਾਰਕਿਕ m_slow;  // In slow block
ਜਨਤਕ:
    Gate_Logic_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep, Ast_ਸਰਗਰਮ* activep, ਤਾਰਕਿਕ slow)
        : Gate_Either_ਸ਼ੀਰਸ਼{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~Gate_Logic_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // ACCESSORS
    File_ਸਤਰ* fileline() ਅਚਲ override { ਵਾਪਸ nodep()->fileline(); }
    Ast_ਗੰਢ* nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
    Ast_ਸਰਗਰਮ* activep() ਅਚਲ { ਵਾਪਸ m_activep; }
    ਤਾਰਕਿਕ slow() ਅਚਲ { ਵਾਪਸ m_slow; }

    // DOT debug
    std::string name() ਅਚਲ override { ਵਾਪਸ m_nodep->fileline()->ascii(); }
    std::string dotColor() ਅਚਲ override { ਵਾਪਸ "red"; }
};

ਵਰਗ Gate_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    std::string dotLabel() ਅਚਲ override { ਵਾਪਸ std::ਮੰਜ਼ਿਲ_string(ਭਾਰ_2()); }

ਜਨਤਕ:
    Gate_ਕਿਨਾਰਾ(V3_ਗ੍ਰਾਫ* graphp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2)
        : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, ਭਾਰ_2} {}
};

ਵਰਗ Gate_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;

ਜਨਤਕ:
    Gate_Var_ਸ਼ੀਰਸ਼* make_Var_ਸ਼ੀਰਸ਼(AstVarScope* vscp) {
        Gate_Var_ਸ਼ੀਰਸ਼* vVtxp = reinterpret_cast<Gate_Var_ਸ਼ੀਰਸ਼*>(vscp->user1p());
        ਜੇ (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = ਨਵਾਂ Gate_Var_ਸ਼ੀਰਸ਼{ਇਹ, vscp};
            vscp->user1p(vVtxp);
            ਜੇ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->ਸੈੱਟ_Consumed("VirtIface");
            }
            ਜੇ (vscp->varp()->is_Sig_ਜਨਤਕ()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->ਸੈੱਟ_Consumed("SigPublic");
            }
            ਜੇ (vscp->varp()->isIO() && vscp->scopep()->is_ਸਿਖਰ()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->ਸੈੱਟ_Is_ਸਿਖਰ();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->ਸੈੱਟ_Consumed("isTop");
            }
        }
        ਵਾਪਸ vVtxp;
    }

    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(Gate_Var_ਸ਼ੀਰਸ਼* srcp, Gate_Logic_ਸ਼ੀਰਸ਼* dstp, ਪੂਰਨ ਭਾਰ_2) {
        ਨਵਾਂ Gate_ਕਿਨਾਰਾ{ਇਹ, srcp, dstp, ਭਾਰ_2};
    }

    ਖਾਲੀ ਜੋੜੋ_ਕਿਨਾਰਾ(Gate_Logic_ਸ਼ੀਰਸ਼* srcp, Gate_Var_ਸ਼ੀਰਸ਼* dstp, ਪੂਰਨ ਭਾਰ_2) {
        ਨਵਾਂ Gate_ਕਿਨਾਰਾ{ਇਹ, srcp, dstp, ਭਾਰ_2};
    }
};

//######################################################################
// GateGraph builder

ਵਰਗ GateBuildVisitor final : ਜਨਤਕ VNVisitorConst {
    // STATE
    Gate_ਗ੍ਰਾਫ* m_graphp = ਨਵਾਂ Gate_ਗ੍ਰਾਫ{};  // The graph being built (var usages/dependencies)
    Gate_Logic_ਸ਼ੀਰਸ਼* m_logicVertexp = ਖਾਲੀ_ਸੂਚਕ;  // Current statement being tracked, nullptr=ignored
    ਅਚਲ Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    ਅਚਲ AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope being processed
    Ast_ਸਰਗਰਮ* m_activep = ਖਾਲੀ_ਸੂਚਕ;  // Current active
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Clocked_ਸਰਗਰਮ = ਝੂਠ;  // Underneath clocked active
    ਤਾਰਕਿਕ m_ਵਿੱਚ_ਕਿਨਾਰਾ_ਸਰਗਰਮ = ਝੂਠ;  // Underneath edge active
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Static_ਸਰਗਰਮ = ਝੂਠ;  // Underneath static active
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Initial_ਸਰਗਰਮ = ਝੂਠ;  // Underneath initial active
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Sen_ਵਸਤੂ = ਝੂਠ;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    ਖਾਲੀ check_ਗੰਢ(Ast_ਗੰਢ* nodep) {
        ਜੇ (nodep->isOutputter()) m_logicVertexp->ਸੈੱਟ_Consumed("outputter");
        ਜੇ (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->ਸੈੱਟ_Consumed("TimingControl");
        }
    }

    ਖਾਲੀ iterateLogic(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ slow = ਝੂਠ, ਅਚਲ ਅੱਖਰ* nonReducibleReason = ਖਾਲੀ_ਸੂਚਕ,
                      ਅਚਲ ਅੱਖਰ* consumeReason = ਖਾਲੀ_ਸੂਚਕ) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = ਨਵਾਂ Gate_Logic_ਸ਼ੀਰਸ਼{m_graphp, nodep, m_activep, slow};
        ਜੇ (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } ਨਹੀਂਤਾਂ ਜੇ (m_ਵਿੱਚ_Clocked_ਸਰਗਰਮ) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } ਨਹੀਂਤਾਂ ਜੇ (m_ਵਿੱਚ_Static_ਸਰਗਰਮ || m_ਵਿੱਚ_Initial_ਸਰਗਰਮ) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->ਸੈੱਟ_Static_Init();
        }
        ਜੇ (consumeReason) m_logicVertexp->ਸੈੱਟ_Consumed(consumeReason);
        check_ਗੰਢ(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_ਵਿੱਚ_Clocked_ਸਰਗਰਮ);
        VL_RESTORER(m_ਵਿੱਚ_ਕਿਨਾਰਾ_ਸਰਗਰਮ);
        VL_RESTORER(m_ਵਿੱਚ_Static_ਸਰਗਰਮ);
        VL_RESTORER(m_ਵਿੱਚ_Initial_ਸਰਗਰਮ);
        m_activep = nodep;
        m_ਵਿੱਚ_Clocked_ਸਰਗਰਮ = nodep->hasClocked();
        m_ਵਿੱਚ_ਕਿਨਾਰਾ_ਸਰਗਰਮ = nodep->sentreep() && nodep->sentreep()->has_ਕਿਨਾਰਾ();
        m_ਵਿੱਚ_Static_ਸਰਗਰਮ = nodep->hasStatic();
        m_ਵਿੱਚ_Initial_ਸਰਗਰਮ = nodep->hasInitial();

        // AstVarScope::user2 -> bool: Signal used in SenItem in *this* active block
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
        iterateChildrenConst(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        ਅਚਲ ਤਾਰਕਿਕ slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? ਖਾਲੀ_ਸੂਚਕ : "Multiple Stmts");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override {
        iterateLogic(nodep, ਝੂਠ, "CoverToggle", "CoverToggle");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Sen_ਵਸਤੂ);
        m_ਵਿੱਚ_Sen_ਵਸਤੂ = ਸੱਚ;
        ਜੇ (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } ਨਹੀਂਤਾਂ {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, ਝੂਠ, ਖਾਲੀ_ਸੂਚਕ, "senItem");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (!m_logicVertexp) ਵਾਪਸ;

        AstVarScope* ਅਚਲ vscp = nodep->varScopep();
        Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vVtxp = m_graphp->make_Var_ਸ਼ੀਰਸ਼(vscp);

        ਜੇ (m_ਵਿੱਚ_Sen_ਵਸਤੂ) {
            vVtxp->ਸੈੱਟ_Is_Clock();
            vscp->user2(ਸੱਚ);
        } ਨਹੀਂਤਾਂ ਜੇ (m_ਵਿੱਚ_ਕਿਨਾਰਾ_ਸਰਗਰਮ && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            ਜੇ (vscp->user2()) {
                ਜੇ (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } ਨਹੀਂਤਾਂ {
                ਜੇ (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        ਜੇ (nodep->access().isWriteOrRW()) m_graphp->ਜੋੜੋ_ਕਿਨਾਰਾ(m_logicVertexp, vVtxp, 1);
        ਜੇ (nodep->access().isReadOrRW()) m_graphp->ਜੋੜੋ_ਕਿਨਾਰਾ(vVtxp, m_logicVertexp, 1);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ)
                      && VN_AS(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (m_logicVertexp) check_ਗੰਢ(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    ਸਥਿਰ std::unique_ptr<Gate_ਗ੍ਰਾਫ> apply(AstNetlist* netlistp) {
        ਵਾਪਸ std::unique_ptr<Gate_ਗ੍ਰਾਫ>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ਸਥਿਰ ਖਾਲੀ v3GateWarnSyncAsync(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : ਗ੍ਰਾਫ.vertices()) {
        ਜੇ (ਅਚਲ Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvertexp = vtx.cast<Gate_Var_ਸ਼ੀਰਸ਼>()) {
            ਅਚਲ AstVarScope* ਅਚਲ vscp = vvertexp->varScp();
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ sp = vvertexp->rstSyncNodep();
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ ap = vvertexp->rstAsyncNodep();
            ਜੇ (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                ਜੇ (!vscp->fileline()->warnIsOff(V3_Error_ਕੋਡ::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3_Error_ਕੋਡ::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3_Error_ਕੋਡ::SYNCASYNCNET)) {
                    vscp->varp()->user2(ਸੱਚ);  // Warn only once per signal
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

ਵਰਗ GateConcatVisitor final : ਜਨਤਕ VNVisitorConst {
    // STATE
    ਅਚਲ AstVarScope* m_vscp = ਖਾਲੀ_ਸੂਚਕ;  // Varscope we're trying to find
    ਪੂਰਨ m_ਵਿੱਥ = 0;  // Current offset of varscope
    ਪੂਰਨ m_ਮਿਲਿਆ_ਵਿੱਥ = 0;  // Found offset of varscope
    ਤਾਰਕਿਕ m_ਮਿਲਿਆ = ਝੂਠ;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (nodep->varScopep() == m_vscp && !nodep->user2() && !m_ਮਿਲਿਆ) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(ਸੱਚ);
            m_ਮਿਲਿਆ_ਵਿੱਥ = m_ਵਿੱਥ;
            m_ਮਿਲਿਆ = ਸੱਚ;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_ਵਿੱਥ << ") - " << nodep);
        }
        m_ਵਿੱਥ += nodep->dtypep()->ਚੌੜਾਈ();
    }
    ਖਾਲੀ ਦੇਖੋ(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // PUBLIC METHODS
    ਤਾਰਕਿਕ concat_ਵਿੱਥ(AstConcat* concatp, AstVarScope* vscp, ਪੂਰਨ& offsetr) {
        m_vscp = vscp;
        m_ਵਿੱਥ = 0;
        m_ਮਿਲਿਆ = ਝੂਠ;
        // Iterate
        iterateConst(concatp);
        offsetr = m_ਮਿਲਿਆ_ਵਿੱਥ;
        ਵਾਪਸ m_ਮਿਲਿਆ;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

ਵਰਗ GateOkVisitor final : ਜਨਤਕ VNVisitorConst {
    // RETURN STATE
    ਤਾਰਕਿਕ m_isSimple = ਸੱਚ;  // Set false when we know it isn't simple
    std::ਸਦਿਸ਼<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_substitutionp = ਖਾਲੀ_ਸੂਚਕ;  // What to replace the variable with

    // STATE
    ਅਚਲ ਤਾਰਕਿਕ m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ਅਚਲ ਤਾਰਕਿਕ m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ਅਚਲ Ast_ਗੰਢ_Var_Ref* m_lhsVarRef = ਖਾਲੀ_ਸੂਚਕ;
    ਪੂਰਨ m_ops = 0;  // Operation count

    // METHODS
    ਖਾਲੀ clearSimple(ਅਚਲ ਅੱਖਰ* because) {
        ਜੇ (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = ਝੂਠ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (!m_isSimple) ਵਾਪਸ;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        ਜੇ (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        ਜੇ (nodep->access().isRW()) {
            clearSimple("R/W");
            ਵਾਪਸ;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ਅਚਲ vscp = nodep->varScopep();
        ਜੇ (nodep->access().isWriteOnly()) {
            ਜੇ (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } ਨਹੀਂਤਾਂ {
            // TODO: possible bug, should it be >= 1 as add is below?
            ਜੇ (m_readVscps.ਮਾਪ() > 1) {
                ਜੇ (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ਧੱਕੋ_ਪਿੱਠ(vscp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (!m_isSimple) ਵਾਪਸ;

        m_substitutionp = nodep->rhsp();
        ਜੇ (!VN_IS(nodep->lhsp(), ਗੰਢ_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } ਨਹੀਂਤਾਂ {
            iterateChildrenConst(nodep);
        }
        ਜੇ (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCReset* /*nodep*/) override {
        ਜੇ (!m_isSimple) ਵਾਪਸ;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੇਲ_Masked* nodep) override {
        ਜੇ (!m_isSimple) ਵਾਪਸ;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (!m_isSimple) ਵਾਪਸ;  // Fastpath

        ਜੇ (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            ਵਾਪਸ;
        }

        ਜੇ (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->is_ਸ਼ੁੱਧ() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            ਵਾਪਸ;
        }

        iterateChildrenConst(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    GateOkVisitor(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ buffersOnly, ਤਾਰਕਿਕ dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        ਜੇ (!m_substitutionp) {
            clearSimple("No assignment found\n");
            ਵਾਪਸ;
        }
        ਜੇ (m_isSimple && m_lhsVarRef) {
            ਲਈ (ਅਚਲ AstVarScope* ਅਚਲ vscp : m_readVscps) {
                ਜੇ (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    ਵਾਪਸ;
                }
            }
        }
    }
    ~GateOkVisitor() override = ਮੂਲ;
    // PUBLIC METHODS
    ਤਾਰਕਿਕ isSimple() ਅਚਲ { ਵਾਪਸ m_isSimple; }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* substitutionp() ਅਚਲ {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        ਵਾਪਸ m_substitutionp;
    }
    ਅਚਲ std::ਸਦਿਸ਼<AstVarScope*>& readVscps() ਅਚਲ { ਵਾਪਸ m_readVscps; }
    ਤਾਰਕਿਕ varAssigned(ਅਚਲ AstVarScope* scopep) ਅਚਲ {
        ਵਾਪਸ m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

ਵਰਗ GateInline final {
    ਵਰਤੋਂ Substitutions = std::unordered_map<AstVarScope*, Ast_ਗੰਢ_ਵਿਅੰਜਕ*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_ਗੰਢ, Substitutions> m_substitutions;

    // STATE
    Gate_ਗ੍ਰਾਫ& m_ਗ੍ਰਾਫ;
    ਮਾਪ_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_ਗੰਢ*, ਮਾਪ_t> m_hasPending;
    ਮਾਪ_t m_statInlined = 0;  // Statistic tracking - signals inlined
    ਮਾਪ_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    ਮਾਪ_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    ਸਥਿਰ ਤਾਰਕਿਕ isCheap(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        // Constant is cheap
        ਜੇ (VN_IS(exprp, Const)) ਵਾਪਸ ਸੱਚ;
        // Variable reference is cheap
        ਜੇ (VN_IS(exprp, ਗੰਢ_Var_Ref)) ਵਾਪਸ ਸੱਚ;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(exprp, Sel)) {
            ਜੇ (!isCheap(selp->fromp())) ਵਾਪਸ ਝੂਠ;
            ਜੇ (!selp->isWide()) ਵਾਪਸ ਸੱਚ;
            ਜੇ (!VN_IS(selp->lsbp(), Const)) ਵਾਪਸ ਝੂਠ;
            ਜੇ (selp->lsbConst() % VL_EDATASIZE != 0) ਵਾਪਸ ਝੂਠ;
            ਵਾਪਸ ਸੱਚ;
        }
        // AstArraySel is cheap if the fromp is cheap
        ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ aselp = VN_CAST(exprp, ArraySel)) {
            ਵਾਪਸ isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        ਵਾਪਸ ਝੂਠ;
    }

    ਤਾਰਕਿਕ shouldInline(Gate_Var_ਸ਼ੀਰਸ਼* vVtxp, Gate_Logic_ਸ਼ੀਰਸ਼* lVtxp, ਮਾਪ_t nReads,
                      Ast_ਗੰਢ_ਵਿਅੰਜਕ* substp, ਤਾਰਕਿਕ allow_Multi_ਵਿੱਚ) {
        // Always inline constants
        ਜੇ (VN_IS(substp, Const)) ਵਾਪਸ ਸੱਚ;
        // Don't inline non-constant static initializers - these are scheduled differently
        ਜੇ (lVtxp->staticInit()) ਵਾਪਸ ਝੂਠ;
        // Inline simple variable references
        ਜੇ (VN_IS(substp, VarRef)) ਵਾਪਸ ਸੱਚ;
        // Only inline arrays if a simple variable or constant
        ਜੇ (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) ਵਾਪਸ ਝੂਠ;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        ਜੇ (nReads == 0) ਵਾਪਸ ਸੱਚ;
        // If it reads one variable, inline if not wide, or if cheap
        ਜੇ (nReads == 1 && (!substp->isWide() || isCheap(substp))) ਵਾਪਸ ਸੱਚ;
        // Don't inline on first round if reads more than one variable
        ਜੇ (nReads > 1 && !allow_Multi_ਵਿੱਚ) ਵਾਪਸ ਝੂਠ;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        ਪੂਰਨ n = 0;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vVtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
            ਅਚਲ Gate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ dstVtxp = ਕਿਨਾਰਾ.ਸਿਖਰ()->as<Gate_Logic_ਸ਼ੀਰਸ਼>();
            // Ignore slow code, or if the destination is not used
            ਜੇ (dstVtxp->slow()) ਜਾਰੀ;
            ਜੇ (dstVtxp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ() && !dstVtxp->consumed()) ਜਾਰੀ;
            n += ਕਿਨਾਰਾ.ਭਾਰ_2();
            ਜੇ (n > 1) {
                ++m_statNotInlined;
                ਵਾਪਸ ਝੂਠ;
            }
        }
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਰਿਕਾਰਡ_Substitution(AstVarScope* vscp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* substp, Ast_ਗੰਢ* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_substitutions(logicp).emplace(vscp, ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ = substp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
    }

    ਖਾਲੀ commitSubstitutions(Ast_ਗੰਢ* logicp) {
        ਜੇ (!m_hasPending.erase(logicp)) ਵਾਪਸ;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.ਖਾਲੀ_ਥਾਵਾਂ(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_ਸੈੱਟ<ਅਚਲ AstVarScope*> replaced(substitutions.ਮਾਪ() * 2);

        ਅਚਲ std::function<ਖਾਲੀ(Ast_ਗੰਢ_Var_Ref*)> ਦੇਖੋ = [&](Ast_ਗੰਢ_Var_Ref* nodep) -> ਖਾਲੀ {
            // See if this variable has a substitution
            AstVarScope* ਅਚਲ vscp = nodep->varScopep();
            ਅਚਲ ਸਵੈਚਾਲਿਤ& it = substitutions.ਲੱਭੋ(vscp);
            ਜੇ (it == substitutions.ਸਿਰਾ()) ਵਾਪਸ;

            // Do not substitute circular logic
            ਜੇ (!replaced.ਪਾਓ(vscp).ਦੂਜਾ) ਵਾਪਸ;

            // Substitute nodep with substp
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ substp = it->ਦੂਜਾ;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, ਗੰਢ_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = substp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            ਜੇ (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(newp, ਗੰਢ_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            // Recursively substitute the new tree
            newp->foreach(ਦੇਖੋ);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(ਦੇਖੋ);

        Ast_ਗੰਢ* ਅਚਲ simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : substitutions) pair.ਦੂਜਾ->delete_ਰੁੱਖ();
        substitutions.clear();
    }

    ਖਾਲੀ optimize_ਸੰਕੇਤ(ਤਾਰਕਿਕ allow_Multi_ਵਿੱਚ) {
        // Consider "inlining" variables
        ਸਵੈਚਾਲਿਤ& vertices = m_ਗ੍ਰਾਫ.vertices();
        ਅਚਲ ਸਵੈਚਾਲਿਤ ff_ਮੰਜ਼ਿਲ_Var_Vtx = [&](V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::List::iterator it) {
            ਜਦੋਂ (it != vertices.ਸਿਰਾ() && !(*it).is<Gate_Var_ਸ਼ੀਰਸ਼>()) ++it;
            ਵਾਪਸ it;
        };
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::List::iterator vIt = ff_ਮੰਜ਼ਿਲ_Var_Vtx(vertices.begin());
        ਜਦੋਂ (vIt != vertices.ਸਿਰਾ()) {
            Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vVtxp = (*vIt).as<Gate_Var_ਸ਼ੀਰਸ਼>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ff_ਮੰਜ਼ਿਲ_Var_Vtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            ਜੇ (!vVtxp->ਵਿੱਚ_Size1()) ਜਾਰੀ;

            // Can't inline if non-reducible, etc
            ਜੇ (!vVtxp->reducible()) ਜਾਰੀ;

            // Grab the driving logic
            Gate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp
                = vVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp()->fromp()->as<Gate_Logic_ਸ਼ੀਰਸ਼>();
            ਜੇ (!lVtxp->reducible()) ਜਾਰੀ;
            Ast_ਗੰਢ* ਅਚਲ logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ਅਚਲ GateOkVisitor okVisitor{logicp, ਝੂਠ, ਝੂਠ};

            // Was it ok?
            ਜੇ (!okVisitor.isSimple()) ਜਾਰੀ;
            // If the varScope is already removed from logicp, no need to try substitution.
            ਜੇ (!okVisitor.varAssigned(vVtxp->varScp())) ਜਾਰੀ;

            // Expression we are considering to substitute with
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ਅਚਲ ਮਾਪ_t nReads = okVisitor.readVscps().ਮਾਪ();

            // Check if it should be inlined
            ਜੇ (!shouldInline(vVtxp, lVtxp, nReads, substp, allow_Multi_ਵਿੱਚ)) ਜਾਰੀ;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ਅਚਲ vscp = vVtxp->varScp();

            ਜੇ (debug() >= 9) {
                vscp->dump_ਰੁੱਖ("substituting: ");
                substp->dump_ਰੁੱਖ("        with: ");
            }

            ਅਚਲ ਸਵੈਚਾਲਿਤ& readVscpsVec = okVisitor.readVscps();
            ਅਚਲ std::unordered_ਸੈੱਟ<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.ਸਿਰਾ()};

            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : vVtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
                Gate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ dstVtxp = edgep->ਸਿਖਰ()->as<Gate_Logic_ਸ਼ੀਰਸ਼>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                ਜੇ (VN_IS(dstVtxp->nodep(), Sen_ਵਸਤੂ)) {
                    Ast_ਗੰਢ* nodep = substp;
                    ਜੇ (AstNot* ਅਚਲ notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    ਜੇ (!VN_IS(nodep, VarRef)) ਜਾਰੀ;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                ਤਾਰਕਿਕ canInline = ਸੱਚ;
                ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& dedge : dstVtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                    ਅਚਲ Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ consVVertexp = dedge.ਸਿਖਰ()->as<Gate_Var_ਸ਼ੀਰਸ਼>();
                    ਜੇ (readVscps.ਗਿਣਤੀ_2(consVVertexp->varScp())) {
                        canInline = ਝੂਠ;
                        ਰੋਕੋ;
                    }
                }

                ਜੇ (!canInline) ਜਾਰੀ;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                ਜੇ (logicp == dstVtxp->nodep()) {
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
                    ਜਾਰੀ;
                }

                ਰਿਕਾਰਡ_Substitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                ਲਈ (AstVarScope* ਅਚਲ newVscp : okVisitor.readVscps()) {
                    Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ varvertexp = m_ਗ੍ਰਾਫ.make_Var_ਸ਼ੀਰਸ਼(newVscp);
                    m_ਗ੍ਰਾਫ.ਜੋੜੋ_ਕਿਨਾਰਾ(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            ਜੇ (vVtxp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_ਗ੍ਰਾਫ), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_ਗ੍ਰਾਫ), lVtxp);
            }
        }
    }

    ਸਪਸ਼ਟ GateInline(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ)
        : m_ਗ੍ਰਾਫ{ਗ੍ਰਾਫ} {
        // Find gate interconnect and optimize
        ਗ੍ਰਾਫ.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimize_ਸੰਕੇਤ(ਝੂਠ);
        // Then propagate more complicated equations
        optimize_ਸੰਕੇਤ(ਸੱਚ);
        // Commit substitutions in insertion order for stability
        ਵਰਤੋਂ Pair = std::pair<Ast_ਗੰਢ*, ਮਾਪ_t>;
        std::ਸਦਿਸ਼<Pair> pending{m_hasPending.begin(), m_hasPending.ਸਿਰਾ()};
        std::ਛਾਂਟੋ(pending.begin(), pending.ਸਿਰਾ(), [](ਅਚਲ Pair& a, ਅਚਲ Pair& b) {  //
            ਵਾਪਸ a.ਦੂਜਾ < b.ਦੂਜਾ;
        });
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : pending) commitSubstitutions(pair.ਪਹਿਲਾ);
    }

    ~GateInline() {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Gate reads replaced", m_statRefs);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) { GateInline{ਗ੍ਰਾਫ}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

ਵਰਗ GateDedupeHash final : ਜਨਤਕ V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    ਬਣਤਰ Aux_Ast_ਗੰਢ_ਵਿਅੰਜਕ final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        Ast_ਸਰਗਰਮ* activep = ਖਾਲੀ_ਸੂਚਕ;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = ਖਾਲੀ_ਸੂਚਕ;
        // Parent AstNodeAssign* for this rhsp
        Ast_ਗੰਢ_ਸੌਂਪੋ* parentp = ਖਾਲੀ_ਸੂਚਕ;
    };
    AstUser2Allocator<Ast_ਗੰਢ_ਵਿਅੰਜਕ, Aux_Ast_ਗੰਢ_ਵਿਅੰਜਕ> m_aux_ਗੰਢ_ਵਿਅੰਜਕ;

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_currRhsp = ਖਾਲੀ_ਸੂਚਕ;  // Current node we are searching for duplicates of
    Aux_Ast_ਗੰਢ_ਵਿਅੰਜਕ m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    ਸਥਿਰ ਤਾਰਕਿਕ same(Ast_ਗੰਢ* node1p, Ast_ਗੰਢ* node2p) {
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
        ਵਾਪਸ node1p == node2p || (node1p && node1p->same_ਰੁੱਖ(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    ਤਾਰਕਿਕ isSame(Ast_ਗੰਢ* node1p, Ast_ਗੰਢ* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ਅਚਲ ਸਵੈਚਾਲਿਤ& aux2 = m_aux_ਗੰਢ_ਵਿਅੰਜਕ(VN_AS(node2p, ਗੰਢ_ਵਿਅੰਜਕ));
        ਵਾਪਸ m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

ਜਨਤਕ:
    GateDedupeHash() = ਮੂਲ;
    ~GateDedupeHash() = ਮੂਲ;

    ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* hash_And_ਲੱਭੋ_Dupe(Ast_ਗੰਢ_ਸੌਂਪੋ* assignp, Ast_ਸਰਗਰਮ* activep,
                                         Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_dupFinder.ਲੱਭੋ_Duplicate(m_currRhsp, ਇਹ);
        ਜੇ (it != m_dupFinder.ਸਿਰਾ()) ਵਾਪਸ m_aux_ਗੰਢ_ਵਿਅੰਜਕ(VN_AS(it->ਦੂਜਾ, ਗੰਢ_ਵਿਅੰਜਕ)).parentp;

        // Insert new node
        m_dupFinder.ਪਾਓ(m_currRhsp);
        m_aux_ਗੰਢ_ਵਿਅੰਜਕ(m_currRhsp) = m_auxCurRhsp;
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

ਵਰਗ GateDedupeVarVisitor final : ਜਨਤਕ VNVisitorConst {
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
    Ast_ਗੰਢ_ਸੌਂਪੋ* m_assignp = ਖਾਲੀ_ਸੂਚਕ;  // Assign found for dedupe
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_ifCondp = ਖਾਲੀ_ਸੂਚਕ;  // IF condition that assign is under
    ਤਾਰਕਿਕ m_always = ਝੂਠ;  // Assign is under an always
    ਤਾਰਕਿਕ m_dedupable = ਸੱਚ;  // Determined the assign to be dedupable

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
        ਜੇ (!m_dedupable) ਵਾਪਸ;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        ਜੇ (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_ਵਿਅੰਜਕ_Stmt*) { ਵਾਪਸ ਸੱਚ; });
            ਵਾਪਸ;
        }

        m_dedupable = ਝੂਠ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (!m_dedupable) ਵਾਪਸ;

        ਜੇ (!m_always) {
            m_always = ਸੱਚ;
            iterate_And_ਅਗਲਾ_Const_Null(nodep->stmtsp());
            ਵਾਪਸ;
        }

        m_dedupable = ਝੂਠ;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        ਜੇ (!m_dedupable) ਵਾਪਸ;

        ਜੇ (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_ਵਿਅੰਜਕ_Stmt*) { ਵਾਪਸ ਸੱਚ; });
            iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
            ਵਾਪਸ;
        }

        m_dedupable = ਝੂਠ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstComment*) override {}  // NOP
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ*) override { m_dedupable = ਝੂਠ; }

ਜਨਤਕ:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = ਮੂਲ;
    ~GateDedupeVarVisitor() override = ਮੂਲ;
    // PUBLIC METHODS
    Ast_ਗੰਢ_Var_Ref* ਲੱਭੋ_Dupe(Ast_ਗੰਢ* logicp, AstVarScope* ਖਪਤਕਾਰ_Vscp, Ast_ਸਰਗਰਮ* activep) {
        m_assignp = ਖਾਲੀ_ਸੂਚਕ;
        m_ifCondp = ਖਾਲੀ_ਸੂਚਕ;
        m_always = ਝੂਠ;
        m_dedupable = ਸੱਚ;
        iterateConst(logicp);
        ਜੇ (m_dedupable && m_assignp) {
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ lRefp = VN_CAST(lhsp, ਗੰਢ_Var_Ref);
            ਜੇ (lRefp && lRefp->varScopep() == ਖਪਤਕਾਰ_Vscp) {
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ dup
                    = m_ghash.hash_And_ਲੱਭੋ_Dupe(m_assignp, activep, m_ifCondp)) {
                    ਵਾਪਸ static_cast<Ast_ਗੰਢ_Var_Ref*>(dup->lhsp());
                }
            }
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

ਵਰਗ GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;

    // STATE
    ਮਾਪ_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_ਡੂੰਘਾਈ = 0;  // Iteration depth

    ਖਾਲੀ ਦੇਖੋ(Gate_Var_ਸ਼ੀਰਸ਼* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        ਜੇ (m_ਡੂੰਘਾਈ > GATE_DEDUP_MAX_ਡੂੰਘਾਈ) ਵਾਪਸ;

        // Check that we haven't been here before
        ਜੇ (vVtxp->varScp()->user2_ਸੈੱਟ_Once()) ਵਾਪਸ;

        VL_RESTORER(m_ਡੂੰਘਾਈ);
        ++m_ਡੂੰਘਾਈ;
        ਜੇ (!vVtxp->ਵਿੱਚ_Size1()) ਵਾਪਸ;

        Ast_ਗੰਢ_Var_Ref* dupRefp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            dupRefp = ਦੇਖੋ(ਕਿਨਾਰਾ.fromp()->as<Gate_Logic_ਸ਼ੀਰਸ਼>(), vVtxp);
        }
        ਜੇ (!dupRefp) ਵਾਪਸ;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        Gate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp = vVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp()->fromp()->as<Gate_Logic_ਸ਼ੀਰਸ਼>();
        ਅਚਲ GateOkVisitor okVisitor{lVtxp->nodep(), ਝੂਠ, ਸੱਚ};
        ਜੇ (!okVisitor.isSimple()) ਵਾਪਸ;

        ++m_statDedupLogic;
        Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ dupVVtxp = dupRefp->varScopep()->user1u().ਮੰਜ਼ਿਲ<Gate_Var_ਸ਼ੀਰਸ਼*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : vVtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
            ਅਚਲ Gate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ ਖਪਤਕਾਰ_Vtxp = edgep->ਸਿਖਰ()->as<Gate_Logic_ਸ਼ੀਰਸ਼>();
            Ast_ਗੰਢ* ਅਚਲ consumerp = ਖਪਤਕਾਰ_Vtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << ਖਪਤਕਾਰ_Vtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            ਜੇ (lVtxp == ਖਪਤਕਾਰ_Vtxp) {
                UINFO(9, "skipping as self-recirculates");
            } ਨਹੀਂਤਾਂ {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_ਗੰਢ_Var_Ref* refp) {
                    ਜੇ (refp->varScopep() != vVtxp->varScp()) ਵਾਪਸ;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_ਗੰਢ_Var_Ref* ਅਚਲ newp = dupRefp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        ਜਦੋਂ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep = vVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_ਗੰਢ_Var_Ref* ਦੇਖੋ(Gate_Logic_ਸ਼ੀਰਸ਼* lVtxp, ਅਚਲ Gate_Var_ਸ਼ੀਰਸ਼* ਖਪਤਕਾਰ_Vtxp) {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : lVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) ਦੇਖੋ(ਕਿਨਾਰਾ.fromp()->as<Gate_Var_ਸ਼ੀਰਸ਼>());

        ਜੇ (lVtxp->dedupable() && ਖਪਤਕਾਰ_Vtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            ਵਾਪਸ m_varVisitor.ਲੱਭੋ_Dupe(lVtxp->nodep(), ਖਪਤਕਾਰ_Vtxp->varScp(), lVtxp->activep());
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    ਸਪਸ਼ਟ GateDedupe(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vVtxp = vtx.cast<Gate_Var_ਸ਼ੀਰਸ਼>()) {
                ਜੇ (vVtxp->isClock()) ਦੇਖੋ(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vVtxp = vtx.cast<Gate_Var_ਸ਼ੀਰਸ਼>()) {
                ਜੇ (vVtxp->is_ਸਿਖਰ() && vVtxp->varScp()->varp()->isWritable()) ਦੇਖੋ(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::ਜੋੜੋ_Stat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) { GateDedupe{ਗ੍ਰਾਫ}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

ਵਰਗ Gate_ਮਿਲਾਓ_Assignments final {
    Gate_ਗ੍ਰਾਫ& m_ਗ੍ਰਾਫ;
    ਮਾਪ_t m_stat_ਸੌਂਪੋ_Merged = 0;  // Statistic tracking
    std::ਸਦਿਸ਼<Gate_Logic_ਸ਼ੀਰਸ਼*> m_ਮੰਜ਼ਿਲ_Remove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* ਮਿਲਾਓ(AstSel* prevSelp, AstSel* currSelp) {
        ਅਚਲ AstVarRef* ਅਚਲ pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ਅਚਲ cRefp = VN_CAST(currSelp->fromp(), VarRef);
        ਜੇ (!pRefp || !cRefp || !cRefp->same_ਗੰਢ(pRefp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // not the same var

        ਅਚਲ AstConst* ਅਚਲ pstart = VN_CAST(prevSelp->lsbp(), Const);
        ਅਚਲ AstConst* ਅਚਲ cstart = VN_CAST(currSelp->lsbp(), Const);
        ਜੇ (!pstart || !cstart) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // too complicated

        ਜੇ (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            ਵਾਪਸ ਨਵਾਂ AstSel{cRefp->fileline(), cRefp->clone_ਰੁੱਖ(ਝੂਠ), currSelp->lsbConst(),
                              prevSelp->ਚੌੜਾਈ_Const() + currSelp->ਚੌੜਾਈ_Const()};
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
    }

    ਖਾਲੀ process(Gate_Var_ਸ਼ੀਰਸ਼* vVtxp) {
        Gate_Logic_ਸ਼ੀਰਸ਼* prevLVtxp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਸੌਂਪੋ_W* prevAssignp = ਖਾਲੀ_ਸੂਚਕ;

        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : vVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().unlinkable()) {
            Gate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp = edgep->fromp()->as<Gate_Logic_ਸ਼ੀਰਸ਼>();
            ਜੇ (!lVtxp->ਬਾਹਰਲਾ_Size1()) ਜਾਰੀ;

            AstAlways* ਅਚਲ alwaysp = VN_CAST(lVtxp->nodep(), Always);
            ਜੇ (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) ਵਾਪਸ;
            Ast_ਸੌਂਪੋ_W* ਅਚਲ assignp = VN_CAST(alwaysp->stmtsp(), ਸੌਂਪੋ_W);
            ਜੇ (!assignp) ਜਾਰੀ;

            ਜੇ (!VN_IS(assignp->lhsp(), Sel)) ਜਾਰੀ;

            // First assign with Sel-lhs, or not under the same active
            ਜੇ (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                ਜਾਰੀ;
            }

            AstSel* ਅਚਲ prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ਅਚਲ currSelp = VN_AS(assignp->lhsp(), Sel);

            ਜੇ (AstSel* ਅਚਲ newSelp = ਮਿਲਾਓ(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_ਰੁੱਖ(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(ਨਵਾਂ AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlink_Fr_ਪਿੱਠ(),
                                                assignp->rhsp()->unlink_Fr_ਪਿੱਠ()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtype_Chg_ਚੌੜਾਈ_Signed(prevAssignp->ਚੌੜਾਈ() + assignp->ਚੌੜਾਈ(),
                                                 prevAssignp->ਚੌੜਾਈ() + assignp->ਚੌੜਾਈ(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_ਮੰਜ਼ਿਲ_Remove.emplace_ਪਿੱਠ(lVtxp);

                // Update the graph
                ਜਦੋਂ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ iedgep = lVtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().frontp()) {
                    Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ fromVtxp = iedgep->fromp()->as<Gate_Var_ਸ਼ੀਰਸ਼>();
                    m_ਗ੍ਰਾਫ.ਜੋੜੋ_ਕਿਨਾਰਾ(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_stat_ਸੌਂਪੋ_Merged;
            } ਨਹੀਂਤਾਂ {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    ਸਪਸ਼ਟ Gate_ਮਿਲਾਓ_Assignments(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ)
        : m_ਗ੍ਰਾਫ{ਗ੍ਰਾਫ} {
        UINFO(6, "mergeAssigns");
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : ਗ੍ਰਾਫ.vertices()) {
            ਜੇ (Gate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vVtxp = vtx.cast<Gate_Var_ਸ਼ੀਰਸ਼>()) process(vVtxp);
        }
        // Delete merged assignments
        ਲਈ (Gate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp : m_ਮੰਜ਼ਿਲ_Remove) {
            Ast_ਗੰਢ* ਅਚਲ nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_ਗ੍ਰਾਫ), lVtxp);
        }
    }

    ~Gate_ਮਿਲਾਓ_Assignments() {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Gate assign merged", m_stat_ਸੌਂਪੋ_Merged);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) { Gate_ਮਿਲਾਓ_Assignments{ਗ੍ਰਾਫ}; }
};

//######################################################################
// GateUnused

ਵਰਗ Gate_ਅਣਵਰਤਿਆ final {
    // STATE
    Gate_ਗ੍ਰਾਫ& m_ਗ੍ਰਾਫ;

    // METHODS

    ਖਾਲੀ markRecurse(Gate_Either_ਸ਼ੀਰਸ਼* vtxp) {
        ਜੇ (vtxp->user()) ਵਾਪਸ;  // Already marked
        vtxp->user(ਸੱਚ);
        vtxp->ਸੈੱਟ_Consumed("propagated");
        // Walk sources and mark them too
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
            Gate_Either_ਸ਼ੀਰਸ਼* ਅਚਲ fromVtxp = static_cast<Gate_Either_ਸ਼ੀਰਸ਼*>(ਕਿਨਾਰਾ.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    ਖਾਲੀ mark() {
        m_ਗ੍ਰਾਫ.userClearVertices();
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : m_ਗ੍ਰਾਫ.vertices()) {
            Gate_Either_ਸ਼ੀਰਸ਼& eVtx = static_cast<Gate_Either_ਸ਼ੀਰਸ਼&>(vtx);
            ਜੇ (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ਸਥਿਰ ਖਾਲੀ warn_ਅਣਵਰਤਿਆ(ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep) {
        ਜੇ (nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::UNUSEDLOOP)) ਵਾਪਸ;

        ਜੇ (ਅਚਲ Ast_ਗੰਢ_Procedure* ਅਚਲ procedurep = VN_CAST(nodep, ਗੰਢ_Procedure)) {
            ਜੇ (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ਅਚਲ AstLoop* ਅਚਲ loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNUSEDLOOP, ਸੱਚ);
                });
        }
    }

    // Remove unused logic
    ਖਾਲੀ remove() {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_ਗ੍ਰਾਫ.vertices().unlinkable()) {
            ਜੇ (Gate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp = vtxp->cast<Gate_Logic_ਸ਼ੀਰਸ਼>()) {
                ਜੇ (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_ਗੰਢ* ਅਚਲ nodep = lVtxp->nodep();
                    warn_ਅਣਵਰਤਿਆ(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlink_Fr_ਪਿੱਠ();
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                    VL_DO_DANGLING(lVtxp->unlinkDelete(&m_ਗ੍ਰਾਫ), lVtxp);
                }
            }
        }
    }

    ਸਪਸ਼ਟ Gate_ਅਣਵਰਤਿਆ(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ)
        : m_ਗ੍ਰਾਫ{ਗ੍ਰਾਫ} {
        mark();  // Mark all used vertices
        remove();  // Remove unused vertices
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Gate_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ) { Gate_ਅਣਵਰਤਿਆ{ਗ੍ਰਾਫ}; }
};

//######################################################################
// Pass entry point

ਖਾਲੀ V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<Gate_ਗ੍ਰਾਫ> graphp = GateBuildVisitor::apply(netlistp);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->remove_Redundant_ਕਿਨਾਰੇ_ਜੋੜ(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        Gate_ਅਣਵਰਤਿਆ::apply(*graphp);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        ਜੇ (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        ਜੇ (v3Global.opt.fAssemble()) {
            Gate_ਮਿਲਾਓ_Assignments::apply(*graphp);
            ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        Gate_ਅਣਵਰਤਿਆ::apply(*graphp);
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_ਰੁੱਖ("gate", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
