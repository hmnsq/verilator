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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Gate.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Const.h"
#ضمن "V3DupFinder.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"

#ضمن <list>
#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_ڈونگھائی = 20;

//######################################################################
// Gate graph classes

صنف Gate_Either_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(Gate_Either_راس, V3_گراف_راس)
    منطقي m_reducible = سچ;  // True if this node should be able to be eliminated
    منطقي m_dedupable = سچ;  // True if this node should be able to be deduped
    منطقي m_consumed = خطأ;  // Output goes to something meaningful
    منطقي m_staticInit = خطأ;  // True if this node is a static initializer
عام:
    واضح Gate_Either_راس(V3_گراف* graphp)
        : V3_گراف_راس{graphp} {}
    ~Gate_Either_راس() override = افتراضي;

    // ACCESSORS
    منطقي reducible() ثابت { أرجع m_reducible; }
    منطقي dedupable() ثابت { أرجع m_dedupable; }
    منطقي consumed() ثابت { أرجع m_consumed; }
    منطقي staticInit() ثابت { أرجع m_staticInit; }
    فراغ سیٹ_Consumed(ثابت حرف* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = سچ;
    }
    فراغ سیٹ_Static_Init() { m_staticInit = سچ; }
    فراغ clearReducible(ثابت حرف* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = خطأ;
    }
    فراغ clearDedupable(ثابت حرف* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = خطأ;
    }
    فراغ clearReducibleAndDedupable(ثابت حرف* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ثابت override { أرجع m_consumed ? "" : "dotted"; }
};

صنف Gate_Var_راس final : عام Gate_Either_راس {
    VL_RTTI_IMPL(Gate_Var_راس, Gate_Either_راس)
    AstVarScope* ثابت m_varScp;
    منطقي m_is_چوٹی = خطأ;
    منطقي m_isClock = خطأ;
    Ast_گرہ* m_rstSyncNodep = خالی_سوچک;  // Used as reset and not in SenItem, in clocked always
    Ast_گرہ* m_rstAsyncNodep = خالی_سوچک;  // Used as reset and in SenItem, in clocked always
عام:
    Gate_Var_راس(V3_گراف* graphp, AstVarScope* varScp)
        : Gate_Either_راس{graphp}
        , m_varScp{varScp} {}
    ~Gate_Var_راس() override = افتراضي;

    // ACCESSORS
    AstVarScope* varScp() ثابت VL_MT_STABLE { أرجع m_varScp; }
    منطقي is_چوٹی() ثابت { أرجع m_is_چوٹی; }
    فراغ سیٹ_Is_چوٹی() { m_is_چوٹی = سچ; }
    منطقي isClock() ثابت { أرجع m_isClock; }
    فراغ سیٹ_Is_Clock() { m_isClock = سچ; }
    Ast_گرہ* rstSyncNodep() ثابت { أرجع m_rstSyncNodep; }
    فراغ rstSyncNodep(Ast_گرہ* nodep) { m_rstSyncNodep = nodep; }
    Ast_گرہ* rstAsyncNodep() ثابت { أرجع m_rstAsyncNodep; }
    فراغ rstAsyncNodep(Ast_گرہ* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    فراغ propagateAttrFrom(Gate_Var_راس* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        إذا (fromp->isClock()) سیٹ_Is_Clock();
    }

    // DOT debug
    std::string name() ثابت override VL_MT_STABLE { أرجع varScp()->name(); }
    std::string dotColor() ثابت override { أرجع "green"; }
};

صنف Gate_Logic_راس final : عام Gate_Either_راس {
    VL_RTTI_IMPL(Gate_Logic_راس, Gate_Either_راس)
    Ast_گرہ* ثابت m_nodep;
    Ast_سرگرم* ثابت m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ثابت منطقي m_slow;  // In slow block
عام:
    Gate_Logic_راس(V3_گراف* graphp, Ast_گرہ* nodep, Ast_سرگرم* activep, منطقي slow)
        : Gate_Either_راس{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~Gate_Logic_راس() override = افتراضي;

    // ACCESSORS
    File_سطر* fileline() ثابت override { أرجع nodep()->fileline(); }
    Ast_گرہ* nodep() ثابت { أرجع m_nodep; }
    Ast_سرگرم* activep() ثابت { أرجع m_activep; }
    منطقي slow() ثابت { أرجع m_slow; }

    // DOT debug
    std::string name() ثابت override { أرجع m_nodep->fileline()->ascii(); }
    std::string dotColor() ثابت override { أرجع "red"; }
};

صنف Gate_کنارا final : عام V3_گراف_کنارا {
    std::string dotLabel() ثابت override { أرجع std::منزل_string(بھار_2()); }

عام:
    Gate_کنارا(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحيح بھار_2)
        : V3_گراف_کنارا{graphp, fromp, چوٹی, بھار_2} {}
};

صنف Gate_گراف final : عام V3_گراف {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ثابت VNUser1_وچ_Use m_inuser1;

عام:
    Gate_Var_راس* make_Var_راس(AstVarScope* vscp) {
        Gate_Var_راس* vVtxp = reinterpret_cast<Gate_Var_راس*>(vscp->user1p());
        إذا (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = جديد Gate_Var_راس{هذا, vscp};
            vscp->user1p(vVtxp);
            إذا (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->سیٹ_Consumed("VirtIface");
            }
            إذا (vscp->varp()->is_Sig_عوامی()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->سیٹ_Consumed("SigPublic");
            }
            إذا (vscp->varp()->isIO() && vscp->scopep()->is_چوٹی()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->سیٹ_Is_چوٹی();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->سیٹ_Consumed("isTop");
            }
        }
        أرجع vVtxp;
    }

    فراغ جوڑو_کنارا(Gate_Var_راس* srcp, Gate_Logic_راس* dstp, صحيح بھار_2) {
        جديد Gate_کنارا{هذا, srcp, dstp, بھار_2};
    }

    فراغ جوڑو_کنارا(Gate_Logic_راس* srcp, Gate_Var_راس* dstp, صحيح بھار_2) {
        جديد Gate_کنارا{هذا, srcp, dstp, بھار_2};
    }
};

//######################################################################
// GateGraph builder

صنف GateBuildVisitor final : عام VNVisitorConst {
    // STATE
    Gate_گراف* m_graphp = جديد Gate_گراف{};  // The graph being built (var usages/dependencies)
    Gate_Logic_راس* m_logicVertexp = خالی_سوچک;  // Current statement being tracked, nullptr=ignored
    ثابت Ast_گرہ_Module* m_modp = خالی_سوچک;  // Current module
    ثابت AstScope* m_scopep = خالی_سوچک;  // Current scope being processed
    Ast_سرگرم* m_activep = خالی_سوچک;  // Current active
    منطقي m_وچ_Clocked_سرگرم = خطأ;  // Underneath clocked active
    منطقي m_وچ_کنارا_سرگرم = خطأ;  // Underneath edge active
    منطقي m_وچ_Static_سرگرم = خطأ;  // Underneath static active
    منطقي m_وچ_Initial_سرگرم = خطأ;  // Underneath initial active
    منطقي m_وچ_Sen_شے = خطأ;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    فراغ check_گرہ(Ast_گرہ* nodep) {
        إذا (nodep->isOutputter()) m_logicVertexp->سیٹ_Consumed("outputter");
        إذا (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->سیٹ_Consumed("TimingControl");
        }
    }

    فراغ iterateLogic(Ast_گرہ* nodep, منطقي slow = خطأ, ثابت حرف* nonReducibleReason = خالی_سوچک,
                      ثابت حرف* consumeReason = خالی_سوچک) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = جديد Gate_Logic_راس{m_graphp, nodep, m_activep, slow};
        إذا (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } وإلا إذا (m_وچ_Clocked_سرگرم) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } وإلا إذا (m_وچ_Static_سرگرم || m_وچ_Initial_سرگرم) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->سیٹ_Static_Init();
        }
        إذا (consumeReason) m_logicVertexp->سیٹ_Consumed(consumeReason);
        check_گرہ(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ ویکھو(Ast_سرگرم* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_وچ_Clocked_سرگرم);
        VL_RESTORER(m_وچ_کنارا_سرگرم);
        VL_RESTORER(m_وچ_Static_سرگرم);
        VL_RESTORER(m_وچ_Initial_سرگرم);
        m_activep = nodep;
        m_وچ_Clocked_سرگرم = nodep->hasClocked();
        m_وچ_کنارا_سرگرم = nodep->sentreep() && nodep->sentreep()->has_کنارا();
        m_وچ_Static_سرگرم = nodep->hasStatic();
        m_وچ_Initial_سرگرم = nodep->hasInitial();

        // AstVarScope::user2 -> bool: Signal used in SenItem in *this* active block
        ثابت VNUser2_وچ_Use user2_وچ_Use;
        iterateChildrenConst(nodep);
    }

    فراغ ویکھو(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    فراغ ویکھو(Ast_گرہ_Procedure* nodep) override {
        ثابت منطقي slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? خالی_سوچک : "Multiple Stmts");
    }
    فراغ ویکھو(AstCoverToggle* nodep) override {
        iterateLogic(nodep, خطأ, "CoverToggle", "CoverToggle");
    }
    فراغ ویکھو(Ast_Sen_شے* nodep) override {
        VL_RESTORER(m_وچ_Sen_شے);
        m_وچ_Sen_شے = سچ;
        إذا (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } وإلا {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, خطأ, خالی_سوچک, "senItem");
        }
    }
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        إذا (!m_logicVertexp) أرجع;

        AstVarScope* ثابت vscp = nodep->varScopep();
        Gate_Var_راس* ثابت vVtxp = m_graphp->make_Var_راس(vscp);

        إذا (m_وچ_Sen_شے) {
            vVtxp->سیٹ_Is_Clock();
            vscp->user2(سچ);
        } وإلا إذا (m_وچ_کنارا_سرگرم && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            إذا (vscp->user2()) {
                إذا (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } وإلا {
                إذا (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        إذا (nodep->access().isWriteOrRW()) m_graphp->جوڑو_کنارا(m_logicVertexp, vVtxp, 1);
        إذا (nodep->access().isReadOrRW()) m_graphp->جوڑو_کنارا(vVtxp, m_logicVertexp, 1);
    }
    فراغ ویکھو(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), گرہ_سونپو)
                      && VN_AS(nodep->backp(), گرہ_سونپو)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override {
        إذا (m_logicVertexp) check_گرہ(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    واضح GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

عام:
    ساكن std::unique_ptr<Gate_گراف> apply(AstNetlist* netlistp) {
        أرجع std::unique_ptr<Gate_گراف>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ساكن فراغ v3GateWarnSyncAsync(Gate_گراف& گراف) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ثابت VNUser2_وچ_Use user2_وچ_Use;
    لكل (V3_گراف_راس& vtx : گراف.vertices()) {
        إذا (ثابت Gate_Var_راس* ثابت vvertexp = vtx.cast<Gate_Var_راس>()) {
            ثابت AstVarScope* ثابت vscp = vvertexp->varScp();
            ثابت Ast_گرہ* ثابت sp = vvertexp->rstSyncNodep();
            ثابت Ast_گرہ* ثابت ap = vvertexp->rstAsyncNodep();
            إذا (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                إذا (!vscp->fileline()->warnIsOff(V3_Error_کوڈ::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3_Error_کوڈ::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3_Error_کوڈ::SYNCASYNCNET)) {
                    vscp->varp()->user2(سچ);  // Warn only once per signal
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

صنف GateConcatVisitor final : عام VNVisitorConst {
    // STATE
    ثابت AstVarScope* m_vscp = خالی_سوچک;  // Varscope we're trying to find
    صحيح m_وتھ = 0;  // Current offset of varscope
    صحيح m_لبھیا_وتھ = 0;  // Found offset of varscope
    منطقي m_لبھیا = خطأ;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        إذا (nodep->varScopep() == m_vscp && !nodep->user2() && !m_لبھیا) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(سچ);
            m_لبھیا_وتھ = m_وتھ;
            m_لبھیا = سچ;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_وتھ << ") - " << nodep);
        }
        m_وتھ += nodep->dtypep()->چوڑائی();
    }
    فراغ ویکھو(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // PUBLIC METHODS
    منطقي concat_وتھ(AstConcat* concatp, AstVarScope* vscp, صحيح& offsetr) {
        m_vscp = vscp;
        m_وتھ = 0;
        m_لبھیا = خطأ;
        // Iterate
        iterateConst(concatp);
        offsetr = m_لبھیا_وتھ;
        أرجع m_لبھیا;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

صنف GateOkVisitor final : عام VNVisitorConst {
    // RETURN STATE
    منطقي m_isSimple = سچ;  // Set false when we know it isn't simple
    std::سمتیہ<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_گرہ_اظہاریہ* m_substitutionp = خالی_سوچک;  // What to replace the variable with

    // STATE
    ثابت منطقي m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ثابت منطقي m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ثابت Ast_گرہ_Var_Ref* m_lhsVarRef = خالی_سوچک;
    صحيح m_ops = 0;  // Operation count

    // METHODS
    فراغ clearSimple(ثابت حرف* because) {
        إذا (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = خطأ;
    }

    // VISITORS
    فراغ ویکھو(Ast_گرہ_Var_Ref* nodep) override {
        إذا (!m_isSimple) أرجع;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        إذا (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        إذا (nodep->access().isRW()) {
            clearSimple("R/W");
            أرجع;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOnly()) {
            إذا (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } وإلا {
            // TODO: possible bug, should it be >= 1 as add is below?
            إذا (m_readVscps.ناپ() > 1) {
                إذا (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ڈالو_پٹھ(vscp);
        }
    }
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        إذا (!m_isSimple) أرجع;

        m_substitutionp = nodep->rhsp();
        إذا (!VN_IS(nodep->lhsp(), گرہ_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } وإلا إذا (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } وإلا {
            iterateChildrenConst(nodep);
        }
        إذا (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    فراغ ویکھو(AstCReset* /*nodep*/) override {
        إذا (!m_isSimple) أرجع;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    فراغ ویکھو(Ast_میل_Masked* nodep) override {
        إذا (!m_isSimple) أرجع;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    فراغ ویکھو(Ast_گرہ* nodep) override {
        إذا (!m_isSimple) أرجع;  // Fastpath

        إذا (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            أرجع;
        }

        إذا (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->is_خالص() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            أرجع;
        }

        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    GateOkVisitor(Ast_گرہ* nodep, منطقي buffersOnly, منطقي dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        إذا (!m_substitutionp) {
            clearSimple("No assignment found\n");
            أرجع;
        }
        إذا (m_isSimple && m_lhsVarRef) {
            لكل (ثابت AstVarScope* ثابت vscp : m_readVscps) {
                إذا (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    أرجع;
                }
            }
        }
    }
    ~GateOkVisitor() override = افتراضي;
    // PUBLIC METHODS
    منطقي isSimple() ثابت { أرجع m_isSimple; }
    Ast_گرہ_اظہاریہ* substitutionp() ثابت {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        أرجع m_substitutionp;
    }
    ثابت std::سمتیہ<AstVarScope*>& readVscps() ثابت { أرجع m_readVscps; }
    منطقي varAssigned(ثابت AstVarScope* scopep) ثابت {
        أرجع m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

صنف GateInline final {
    ورتوں Substitutions = std::unordered_map<AstVarScope*, Ast_گرہ_اظہاریہ*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ثابت VNUser2_وچ_Use m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_گرہ, Substitutions> m_substitutions;

    // STATE
    Gate_گراف& m_گراف;
    ناپ_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_گرہ*, ناپ_t> m_hasPending;
    ناپ_t m_statInlined = 0;  // Statistic tracking - signals inlined
    ناپ_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    ناپ_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    ساكن منطقي isCheap(ثابت Ast_گرہ_اظہاریہ* exprp) {
        // Constant is cheap
        إذا (VN_IS(exprp, Const)) أرجع سچ;
        // Variable reference is cheap
        إذا (VN_IS(exprp, گرہ_Var_Ref)) أرجع سچ;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        إذا (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            إذا (!isCheap(selp->fromp())) أرجع خطأ;
            إذا (!selp->isWide()) أرجع سچ;
            إذا (!VN_IS(selp->lsbp(), Const)) أرجع خطأ;
            إذا (selp->lsbConst() % VL_EDATASIZE != 0) أرجع خطأ;
            أرجع سچ;
        }
        // AstArraySel is cheap if the fromp is cheap
        إذا (ثابت AstArraySel* ثابت aselp = VN_CAST(exprp, ArraySel)) {
            أرجع isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        أرجع خطأ;
    }

    منطقي shouldInline(Gate_Var_راس* vVtxp, Gate_Logic_راس* lVtxp, ناپ_t nReads,
                      Ast_گرہ_اظہاریہ* substp, منطقي allow_Multi_وچ) {
        // Always inline constants
        إذا (VN_IS(substp, Const)) أرجع سچ;
        // Don't inline non-constant static initializers - these are scheduled differently
        إذا (lVtxp->staticInit()) أرجع خطأ;
        // Inline simple variable references
        إذا (VN_IS(substp, VarRef)) أرجع سچ;
        // Only inline arrays if a simple variable or constant
        إذا (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        إذا (nReads == 0) أرجع سچ;
        // If it reads one variable, inline if not wide, or if cheap
        إذا (nReads == 1 && (!substp->isWide() || isCheap(substp))) أرجع سچ;
        // Don't inline on first round if reads more than one variable
        إذا (nReads > 1 && !allow_Multi_وچ) أرجع خطأ;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        صحيح n = 0;
        لكل (V3_گراف_کنارا& کنارا : vVtxp->باہرلا_کنارے()) {
            ثابت Gate_Logic_راس* ثابت dstVtxp = کنارا.چوٹی()->as<Gate_Logic_راس>();
            // Ignore slow code, or if the destination is not used
            إذا (dstVtxp->slow()) استمر;
            إذا (dstVtxp->باہرلا_خالی_تھاواں() && !dstVtxp->consumed()) استمر;
            n += کنارا.بھار_2();
            إذا (n > 1) {
                ++m_statNotInlined;
                أرجع خطأ;
            }
        }
        أرجع سچ;
    }

    فراغ ریکارڈ_Substitution(AstVarScope* vscp, Ast_گرہ_اظہاریہ* substp, Ast_گرہ* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ثابت خودکار pair = m_substitutions(logicp).emplace(vscp, خالی_سوچک);
        إذا (pair.دوجا) pair.پہلا->دوجا = substp->clone_درخت_خالص(خطأ);
    }

    فراغ commitSubstitutions(Ast_گرہ* logicp) {
        إذا (!m_hasPending.erase(logicp)) أرجع;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.خالی_تھاواں(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_سیٹ<ثابت AstVarScope*> replaced(substitutions.ناپ() * 2);

        ثابت std::function<فراغ(Ast_گرہ_Var_Ref*)> ویکھو = [&](Ast_گرہ_Var_Ref* nodep) -> فراغ {
            // See if this variable has a substitution
            AstVarScope* ثابت vscp = nodep->varScopep();
            ثابت خودکار& it = substitutions.لبھو(vscp);
            إذا (it == substitutions.سرا()) أرجع;

            // Do not substitute circular logic
            إذا (!replaced.داخل_کرو(vscp).دوجا) أرجع;

            // Substitute nodep with substp
            Ast_گرہ_اظہاریہ* ثابت substp = it->دوجا;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, گرہ_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_گرہ_اظہاریہ* ثابت newp = substp->clone_درخت_خالص(خطأ);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            إذا (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            إذا (Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(newp, گرہ_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Recursively substitute the new tree
            newp->foreach(ویکھو);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(ویکھو);

        Ast_گرہ* ثابت simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        لكل (ثابت خودکار& pair : substitutions) pair.دوجا->delete_درخت();
        substitutions.clear();
    }

    فراغ optimize_اشارے(منطقي allow_Multi_وچ) {
        // Consider "inlining" variables
        خودکار& vertices = m_گراف.vertices();
        ثابت خودکار ff_منزل_Var_Vtx = [&](V3_گراف_راس::List::iterator it) {
            طالما (it != vertices.سرا() && !(*it).is<Gate_Var_راس>()) ++it;
            أرجع it;
        };
        V3_گراف_راس::List::iterator vIt = ff_منزل_Var_Vtx(vertices.begin());
        طالما (vIt != vertices.سرا()) {
            Gate_Var_راس* ثابت vVtxp = (*vIt).as<Gate_Var_راس>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ff_منزل_Var_Vtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            إذا (!vVtxp->وچ_Size1()) استمر;

            // Can't inline if non-reducible, etc
            إذا (!vVtxp->reducible()) استمر;

            // Grab the driving logic
            Gate_Logic_راس* ثابت lVtxp
                = vVtxp->وچ_کنارے().frontp()->fromp()->as<Gate_Logic_راس>();
            إذا (!lVtxp->reducible()) استمر;
            Ast_گرہ* ثابت logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ثابت GateOkVisitor okVisitor{logicp, خطأ, خطأ};

            // Was it ok?
            إذا (!okVisitor.isSimple()) استمر;
            // If the varScope is already removed from logicp, no need to try substitution.
            إذا (!okVisitor.varAssigned(vVtxp->varScp())) استمر;

            // Expression we are considering to substitute with
            Ast_گرہ_اظہاریہ* ثابت substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ثابت ناپ_t nReads = okVisitor.readVscps().ناپ();

            // Check if it should be inlined
            إذا (!shouldInline(vVtxp, lVtxp, nReads, substp, allow_Multi_وچ)) استمر;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ثابت vscp = vVtxp->varScp();

            إذا (debug() >= 9) {
                vscp->dump_درخت("substituting: ");
                substp->dump_درخت("        with: ");
            }

            ثابت خودکار& readVscpsVec = okVisitor.readVscps();
            ثابت std::unordered_سیٹ<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.سرا()};

            لكل (V3_گراف_کنارا* ثابت edgep : vVtxp->باہرلا_کنارے().unlinkable()) {
                Gate_Logic_راس* ثابت dstVtxp = edgep->چوٹی()->as<Gate_Logic_راس>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                إذا (VN_IS(dstVtxp->nodep(), Sen_شے)) {
                    Ast_گرہ* nodep = substp;
                    إذا (AstNot* ثابت notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    إذا (!VN_IS(nodep, VarRef)) استمر;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                منطقي canInline = سچ;
                لكل (ثابت V3_گراف_کنارا& dedge : dstVtxp->باہرلا_کنارے()) {
                    ثابت Gate_Var_راس* ثابت consVVertexp = dedge.چوٹی()->as<Gate_Var_راس>();
                    إذا (readVscps.گنتی(consVVertexp->varScp())) {
                        canInline = خطأ;
                        اكسر;
                    }
                }

                إذا (!canInline) استمر;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                إذا (logicp == dstVtxp->nodep()) {
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
                    استمر;
                }

                ریکارڈ_Substitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                لكل (AstVarScope* ثابت newVscp : okVisitor.readVscps()) {
                    Gate_Var_راس* ثابت varvertexp = m_گراف.make_Var_راس(newVscp);
                    m_گراف.جوڑو_کنارا(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            إذا (vVtxp->باہرلا_خالی_تھاواں()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_گراف), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlink_Fr_پٹھ()->delete_درخت(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_گراف), lVtxp);
            }
        }
    }

    واضح GateInline(Gate_گراف& گراف)
        : m_گراف{گراف} {
        // Find gate interconnect and optimize
        گراف.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimize_اشارے(خطأ);
        // Then propagate more complicated equations
        optimize_اشارے(سچ);
        // Commit substitutions in insertion order for stability
        ورتوں Pair = std::pair<Ast_گرہ*, ناپ_t>;
        std::سمتیہ<Pair> pending{m_hasPending.begin(), m_hasPending.سرا()};
        std::ترتیب_دیو(pending.begin(), pending.سرا(), [](ثابت Pair& a, ثابت Pair& b) {  //
            أرجع a.دوجا < b.دوجا;
        });
        لكل (ثابت خودکار& pair : pending) commitSubstitutions(pair.پہلا);
    }

    ~GateInline() {
        V3Stats::جوڑو_Stat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::جوڑو_Stat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::جوڑو_Stat("Optimizations, Gate reads replaced", m_statRefs);
    }

عام:
    ساكن فراغ apply(Gate_گراف& گراف) { GateInline{گراف}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

صنف GateDedupeHash final : عام V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    بنية Aux_Ast_گرہ_اظہاریہ final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        Ast_سرگرم* activep = خالی_سوچک;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_گرہ_اظہاریہ* condp = خالی_سوچک;
        // Parent AstNodeAssign* for this rhsp
        Ast_گرہ_سونپو* parentp = خالی_سوچک;
    };
    AstUser2Allocator<Ast_گرہ_اظہاریہ, Aux_Ast_گرہ_اظہاریہ> m_aux_گرہ_اظہاریہ;

    Ast_گرہ_اظہاریہ* m_currRhsp = خالی_سوچک;  // Current node we are searching for duplicates of
    Aux_Ast_گرہ_اظہاریہ m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    ساكن منطقي same(Ast_گرہ* node1p, Ast_گرہ* node2p) {
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
        أرجع node1p == node2p || (node1p && node1p->same_درخت(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    منطقي isSame(Ast_گرہ* node1p, Ast_گرہ* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ثابت خودکار& aux2 = m_aux_گرہ_اظہاریہ(VN_AS(node2p, گرہ_اظہاریہ));
        أرجع m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

عام:
    GateDedupeHash() = افتراضي;
    ~GateDedupeHash() = افتراضي;

    ثابت Ast_گرہ_سونپو* hash_And_لبھو_Dupe(Ast_گرہ_سونپو* assignp, Ast_سرگرم* activep,
                                         Ast_گرہ_اظہاریہ* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ثابت خودکار it = m_dupFinder.لبھو_Duplicate(m_currRhsp, هذا);
        إذا (it != m_dupFinder.سرا()) أرجع m_aux_گرہ_اظہاریہ(VN_AS(it->دوجا, گرہ_اظہاریہ)).parentp;

        // Insert new node
        m_dupFinder.داخل_کرو(m_currRhsp);
        m_aux_گرہ_اظہاریہ(m_currRhsp) = m_auxCurRhsp;
        أرجع خالی_سوچک;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

صنف GateDedupeVarVisitor final : عام VNVisitorConst {
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
    Ast_گرہ_سونپو* m_assignp = خالی_سوچک;  // Assign found for dedupe
    Ast_گرہ_اظہاریہ* m_ifCondp = خالی_سوچک;  // IF condition that assign is under
    منطقي m_always = خطأ;  // Assign is under an always
    منطقي m_dedupable = سچ;  // Determined the assign to be dedupable

    // VISITORS
    فراغ ویکھو(Ast_گرہ_سونپو* nodep) override {
        إذا (!m_dedupable) أرجع;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        إذا (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_اظہاریہ_Stmt*) { أرجع سچ; });
            أرجع;
        }

        m_dedupable = خطأ;
    }

    فراغ ویکھو(AstAlways* nodep) override {
        إذا (!m_dedupable) أرجع;

        إذا (!m_always) {
            m_always = سچ;
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
            أرجع;
        }

        m_dedupable = خطأ;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    فراغ ویکھو(Ast_گرہ_If* nodep) override {
        إذا (!m_dedupable) أرجع;

        إذا (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_اظہاریہ_Stmt*) { أرجع سچ; });
            iterate_And_اگلا_Const_Null(nodep->thensp());
            أرجع;
        }

        m_dedupable = خطأ;
    }

    فراغ ویکھو(AstComment*) override {}  // NOP
    //--------------------
    فراغ ویکھو(Ast_گرہ*) override { m_dedupable = خطأ; }

عام:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = افتراضي;
    ~GateDedupeVarVisitor() override = افتراضي;
    // PUBLIC METHODS
    Ast_گرہ_Var_Ref* لبھو_Dupe(Ast_گرہ* logicp, AstVarScope* کھپتکار_Vscp, Ast_سرگرم* activep) {
        m_assignp = خالی_سوچک;
        m_ifCondp = خالی_سوچک;
        m_always = خطأ;
        m_dedupable = سچ;
        iterateConst(logicp);
        إذا (m_dedupable && m_assignp) {
            ثابت Ast_گرہ* ثابت lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ثابت Ast_گرہ_Var_Ref* ثابت lRefp = VN_CAST(lhsp, گرہ_Var_Ref);
            إذا (lRefp && lRefp->varScopep() == کھپتکار_Vscp) {
                إذا (ثابت Ast_گرہ_سونپو* ثابت dup
                    = m_ghash.hash_And_لبھو_Dupe(m_assignp, activep, m_ifCondp)) {
                    أرجع static_cast<Ast_گرہ_Var_Ref*>(dup->lhsp());
                }
            }
        }
        أرجع خالی_سوچک;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

صنف GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ثابت VNUser2_وچ_Use m_inuser2;

    // STATE
    ناپ_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_ڈونگھائی = 0;  // Iteration depth

    فراغ ویکھو(Gate_Var_راس* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        إذا (m_ڈونگھائی > GATE_DEDUP_MAX_ڈونگھائی) أرجع;

        // Check that we haven't been here before
        إذا (vVtxp->varScp()->user2_سیٹ_Once()) أرجع;

        VL_RESTORER(m_ڈونگھائی);
        ++m_ڈونگھائی;
        إذا (!vVtxp->وچ_Size1()) أرجع;

        Ast_گرہ_Var_Ref* dupRefp = خالی_سوچک;
        لكل (V3_گراف_کنارا& کنارا : vVtxp->وچ_کنارے()) {
            dupRefp = ویکھو(کنارا.fromp()->as<Gate_Logic_راس>(), vVtxp);
        }
        إذا (!dupRefp) أرجع;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        Gate_Logic_راس* ثابت lVtxp = vVtxp->وچ_کنارے().frontp()->fromp()->as<Gate_Logic_راس>();
        ثابت GateOkVisitor okVisitor{lVtxp->nodep(), خطأ, سچ};
        إذا (!okVisitor.isSimple()) أرجع;

        ++m_statDedupLogic;
        Gate_Var_راس* ثابت dupVVtxp = dupRefp->varScopep()->user1u().منزل<Gate_Var_راس*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        لكل (V3_گراف_کنارا* ثابت edgep : vVtxp->باہرلا_کنارے().unlinkable()) {
            ثابت Gate_Logic_راس* ثابت کھپتکار_Vtxp = edgep->چوٹی()->as<Gate_Logic_راس>();
            Ast_گرہ* ثابت consumerp = کھپتکار_Vtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << کھپتکار_Vtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            إذا (lVtxp == کھپتکار_Vtxp) {
                UINFO(9, "skipping as self-recirculates");
            } وإلا {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_گرہ_Var_Ref* refp) {
                    إذا (refp->varScopep() != vVtxp->varScp()) أرجع;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_گرہ_Var_Ref* ثابت newp = dupRefp->clone_درخت_خالص(خطأ);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_درخت(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        طالما (V3_گراف_کنارا* ثابت edgep = vVtxp->وچ_کنارے().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_گرہ_Var_Ref* ویکھو(Gate_Logic_راس* lVtxp, ثابت Gate_Var_راس* کھپتکار_Vtxp) {
        لكل (V3_گراف_کنارا& کنارا : lVtxp->وچ_کنارے()) ویکھو(کنارا.fromp()->as<Gate_Var_راس>());

        إذا (lVtxp->dedupable() && کھپتکار_Vtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            أرجع m_varVisitor.لبھو_Dupe(lVtxp->nodep(), کھپتکار_Vtxp->varScp(), lVtxp->activep());
        }
        أرجع خالی_سوچک;
    }

    واضح GateDedupe(Gate_گراف& گراف) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        لكل (V3_گراف_راس& vtx : گراف.vertices()) {
            إذا (Gate_Var_راس* ثابت vVtxp = vtx.cast<Gate_Var_راس>()) {
                إذا (vVtxp->isClock()) ویکھو(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        لكل (V3_گراف_راس& vtx : گراف.vertices()) {
            إذا (Gate_Var_راس* ثابت vVtxp = vtx.cast<Gate_Var_راس>()) {
                إذا (vVtxp->is_چوٹی() && vVtxp->varScp()->varp()->isWritable()) ویکھو(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::جوڑو_Stat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

عام:
    ساكن فراغ apply(Gate_گراف& گراف) { GateDedupe{گراف}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

صنف Gate_ملاؤ_Assignments final {
    Gate_گراف& m_گراف;
    ناپ_t m_stat_سونپو_Merged = 0;  // Statistic tracking
    std::سمتیہ<Gate_Logic_راس*> m_منزل_Remove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* ملاؤ(AstSel* prevSelp, AstSel* currSelp) {
        ثابت AstVarRef* ثابت pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ثابت cRefp = VN_CAST(currSelp->fromp(), VarRef);
        إذا (!pRefp || !cRefp || !cRefp->same_گرہ(pRefp)) أرجع خالی_سوچک;  // not the same var

        ثابت AstConst* ثابت pstart = VN_CAST(prevSelp->lsbp(), Const);
        ثابت AstConst* ثابت cstart = VN_CAST(currSelp->lsbp(), Const);
        إذا (!pstart || !cstart) أرجع خالی_سوچک;  // too complicated

        إذا (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            أرجع جديد AstSel{cRefp->fileline(), cRefp->clone_درخت(خطأ), currSelp->lsbConst(),
                              prevSelp->چوڑائی_Const() + currSelp->چوڑائی_Const()};
        } وإلا {
            أرجع خالی_سوچک;
        }
    }

    فراغ process(Gate_Var_راس* vVtxp) {
        Gate_Logic_راس* prevLVtxp = خالی_سوچک;
        Ast_سونپو_W* prevAssignp = خالی_سوچک;

        لكل (V3_گراف_کنارا* ثابت edgep : vVtxp->وچ_کنارے().unlinkable()) {
            Gate_Logic_راس* ثابت lVtxp = edgep->fromp()->as<Gate_Logic_راس>();
            إذا (!lVtxp->باہرلا_Size1()) استمر;

            AstAlways* ثابت alwaysp = VN_CAST(lVtxp->nodep(), Always);
            إذا (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) أرجع;
            Ast_سونپو_W* ثابت assignp = VN_CAST(alwaysp->stmtsp(), سونپو_W);
            إذا (!assignp) استمر;

            إذا (!VN_IS(assignp->lhsp(), Sel)) استمر;

            // First assign with Sel-lhs, or not under the same active
            إذا (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                استمر;
            }

            AstSel* ثابت prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ثابت currSelp = VN_AS(assignp->lhsp(), Sel);

            إذا (AstSel* ثابت newSelp = ملاؤ(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_درخت(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(جديد AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlink_Fr_پٹھ(),
                                                assignp->rhsp()->unlink_Fr_پٹھ()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtype_Chg_چوڑائی_Signed(prevAssignp->چوڑائی() + assignp->چوڑائی(),
                                                 prevAssignp->چوڑائی() + assignp->چوڑائی(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_منزل_Remove.emplace_پٹھ(lVtxp);

                // Update the graph
                طالما (V3_گراف_کنارا* ثابت iedgep = lVtxp->وچ_کنارے().frontp()) {
                    Gate_Var_راس* ثابت fromVtxp = iedgep->fromp()->as<Gate_Var_راس>();
                    m_گراف.جوڑو_کنارا(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_stat_سونپو_Merged;
            } وإلا {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    واضح Gate_ملاؤ_Assignments(Gate_گراف& گراف)
        : m_گراف{گراف} {
        UINFO(6, "mergeAssigns");
        لكل (V3_گراف_راس& vtx : گراف.vertices()) {
            إذا (Gate_Var_راس* ثابت vVtxp = vtx.cast<Gate_Var_راس>()) process(vVtxp);
        }
        // Delete merged assignments
        لكل (Gate_Logic_راس* ثابت lVtxp : m_منزل_Remove) {
            Ast_گرہ* ثابت nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlink_Fr_پٹھ()->delete_درخت(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_گراف), lVtxp);
        }
    }

    ~Gate_ملاؤ_Assignments() {
        V3Stats::جوڑو_Stat("Optimizations, Gate assign merged", m_stat_سونپو_Merged);
    }

عام:
    ساكن فراغ apply(Gate_گراف& گراف) { Gate_ملاؤ_Assignments{گراف}; }
};

//######################################################################
// GateUnused

صنف Gate_بن_ورتیا final {
    // STATE
    Gate_گراف& m_گراف;

    // METHODS

    فراغ markRecurse(Gate_Either_راس* vtxp) {
        إذا (vtxp->user()) أرجع;  // Already marked
        vtxp->user(سچ);
        vtxp->سیٹ_Consumed("propagated");
        // Walk sources and mark them too
        لكل (V3_گراف_کنارا& کنارا : vtxp->وچ_کنارے()) {
            Gate_Either_راس* ثابت fromVtxp = static_cast<Gate_Either_راس*>(کنارا.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    فراغ mark() {
        m_گراف.userClearVertices();
        لكل (V3_گراف_راس& vtx : m_گراف.vertices()) {
            Gate_Either_راس& eVtx = static_cast<Gate_Either_راس&>(vtx);
            إذا (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ساكن فراغ warn_بن_ورتیا(ثابت Ast_گرہ* ثابت nodep) {
        إذا (nodep->fileline()->warnIsOff(V3_Error_کوڈ::UNUSEDLOOP)) أرجع;

        إذا (ثابت Ast_گرہ_Procedure* ثابت procedurep = VN_CAST(nodep, گرہ_Procedure)) {
            إذا (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ثابت AstLoop* ثابت loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3_Error_کوڈ::UNUSEDLOOP, سچ);
                });
        }
    }

    // Remove unused logic
    فراغ remove() {
        لكل (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            إذا (Gate_Logic_راس* ثابت lVtxp = vtxp->cast<Gate_Logic_راس>()) {
                إذا (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_گرہ* ثابت nodep = lVtxp->nodep();
                    warn_بن_ورتیا(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlink_Fr_پٹھ();
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    VL_DO_DANGLING(lVtxp->unlinkDelete(&m_گراف), lVtxp);
                }
            }
        }
    }

    واضح Gate_بن_ورتیا(Gate_گراف& گراف)
        : m_گراف{گراف} {
        mark();  // Mark all used vertices
        remove();  // Remove unused vertices
    }

عام:
    ساكن فراغ apply(Gate_گراف& گراف) { Gate_بن_ورتیا{گراف}; }
};

//######################################################################
// Pass entry point

فراغ V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<Gate_گراف> graphp = GateBuildVisitor::apply(netlistp);
        إذا (dump_گراف_Level() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->remove_Redundant_کنارے_جوڑ(&V3_گراف_کنارا::followAlwaysTrue);
        إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        Gate_بن_ورتیا::apply(*graphp);
        إذا (dump_گراف_Level() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        إذا (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        إذا (v3Global.opt.fAssemble()) {
            Gate_ملاؤ_Assignments::apply(*graphp);
            إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        Gate_بن_ورتیا::apply(*graphp);
        إذا (dump_گراف_Level() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_درخت("gate", 0, dump_درخت_Either_Level() >= 3);
}
