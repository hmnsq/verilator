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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Gate.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3Const.h"
#شامل "V3DupFinder.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <list>
#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_گہرائی = 20;

//######################################################################
// Gate graph classes

جماعت Gate_Either_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Gate_Either_راس, V3_گراف_راس)
    منطقی m_reducible = سچ;  // True if this node should be able to be eliminated
    منطقی m_dedupable = سچ;  // True if this node should be able to be deduped
    منطقی m_consumed = جھوٹ;  // Output goes to something meaningful
    منطقی m_staticInit = جھوٹ;  // True if this node is a static initializer
عوامی:
    صریح Gate_Either_راس(V3_گراف* graphp)
        : V3_گراف_راس{graphp} {}
    ~Gate_Either_راس() override = طےشدہ;

    // ACCESSORS
    منطقی reducible() ثابت { واپس m_reducible; }
    منطقی dedupable() ثابت { واپس m_dedupable; }
    منطقی consumed() ثابت { واپس m_consumed; }
    منطقی staticInit() ثابت { واپس m_staticInit; }
    خالی سیٹ_Consumed(ثابت حرف* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = سچ;
    }
    خالی سیٹ_Static_Init() { m_staticInit = سچ; }
    خالی clearReducible(ثابت حرف* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = جھوٹ;
    }
    خالی clearDedupable(ثابت حرف* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = جھوٹ;
    }
    خالی clearReducibleAndDedupable(ثابت حرف* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ثابت override { واپس m_consumed ? "" : "dotted"; }
};

جماعت Gate_Var_راس final : عوامی Gate_Either_راس {
    VL_RTTI_IMPL(Gate_Var_راس, Gate_Either_راس)
    AstVarScope* ثابت m_varScp;
    منطقی m_is_چوٹی = جھوٹ;
    منطقی m_isClock = جھوٹ;
    Ast_گرہ* m_rstSyncNodep = خالی_اشاریہ;  // Used as reset and not in SenItem, in clocked always
    Ast_گرہ* m_rstAsyncNodep = خالی_اشاریہ;  // Used as reset and in SenItem, in clocked always
عوامی:
    Gate_Var_راس(V3_گراف* graphp, AstVarScope* varScp)
        : Gate_Either_راس{graphp}
        , m_varScp{varScp} {}
    ~Gate_Var_راس() override = طےشدہ;

    // ACCESSORS
    AstVarScope* varScp() ثابت VL_MT_STABLE { واپس m_varScp; }
    منطقی is_چوٹی() ثابت { واپس m_is_چوٹی; }
    خالی سیٹ_Is_چوٹی() { m_is_چوٹی = سچ; }
    منطقی isClock() ثابت { واپس m_isClock; }
    خالی سیٹ_Is_Clock() { m_isClock = سچ; }
    Ast_گرہ* rstSyncNodep() ثابت { واپس m_rstSyncNodep; }
    خالی rstSyncNodep(Ast_گرہ* nodep) { m_rstSyncNodep = nodep; }
    Ast_گرہ* rstAsyncNodep() ثابت { واپس m_rstAsyncNodep; }
    خالی rstAsyncNodep(Ast_گرہ* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    خالی propagateAttrFrom(Gate_Var_راس* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        اگر (fromp->isClock()) سیٹ_Is_Clock();
    }

    // DOT debug
    std::string name() ثابت override VL_MT_STABLE { واپس varScp()->name(); }
    std::string dotColor() ثابت override { واپس "green"; }
};

جماعت Gate_Logic_راس final : عوامی Gate_Either_راس {
    VL_RTTI_IMPL(Gate_Logic_راس, Gate_Either_راس)
    Ast_گرہ* ثابت m_nodep;
    Ast_فعال* ثابت m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ثابت منطقی m_slow;  // In slow block
عوامی:
    Gate_Logic_راس(V3_گراف* graphp, Ast_گرہ* nodep, Ast_فعال* activep, منطقی slow)
        : Gate_Either_راس{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~Gate_Logic_راس() override = طےشدہ;

    // ACCESSORS
    File_سطر* fileline() ثابت override { واپس nodep()->fileline(); }
    Ast_گرہ* nodep() ثابت { واپس m_nodep; }
    Ast_فعال* activep() ثابت { واپس m_activep; }
    منطقی slow() ثابت { واپس m_slow; }

    // DOT debug
    std::string name() ثابت override { واپس m_nodep->fileline()->ascii(); }
    std::string dotColor() ثابت override { واپس "red"; }
};

جماعت Gate_کنارہ final : عوامی V3_گراف_کنارہ {
    std::string dotLabel() ثابت override { واپس std::مقصد_string(وزن_2()); }

عوامی:
    Gate_کنارہ(V3_گراف* graphp, V3_گراف_راس* fromp, V3_گراف_راس* چوٹی, صحیح وزن_2)
        : V3_گراف_کنارہ{graphp, fromp, چوٹی, وزن_2} {}
};

جماعت Gate_گراف final : عوامی V3_گراف {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ثابت VNUser1_اندر_Use m_inuser1;

عوامی:
    Gate_Var_راس* make_Var_راس(AstVarScope* vscp) {
        Gate_Var_راس* vVtxp = reinterpret_cast<Gate_Var_راس*>(vscp->user1p());
        اگر (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = نیا Gate_Var_راس{یہ, vscp};
            vscp->user1p(vVtxp);
            اگر (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->سیٹ_Consumed("VirtIface");
            }
            اگر (vscp->varp()->is_Sig_عوامی()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->سیٹ_Consumed("SigPublic");
            }
            اگر (vscp->varp()->isIO() && vscp->scopep()->is_چوٹی()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->سیٹ_Is_چوٹی();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->سیٹ_Consumed("isTop");
            }
        }
        واپس vVtxp;
    }

    خالی جوڑو_کنارہ(Gate_Var_راس* srcp, Gate_Logic_راس* dstp, صحیح وزن_2) {
        نیا Gate_کنارہ{یہ, srcp, dstp, وزن_2};
    }

    خالی جوڑو_کنارہ(Gate_Logic_راس* srcp, Gate_Var_راس* dstp, صحیح وزن_2) {
        نیا Gate_کنارہ{یہ, srcp, dstp, وزن_2};
    }
};

//######################################################################
// GateGraph builder

جماعت GateBuildVisitor final : عوامی VNVisitorConst {
    // STATE
    Gate_گراف* m_graphp = نیا Gate_گراف{};  // The graph being built (var usages/dependencies)
    Gate_Logic_راس* m_logicVertexp = خالی_اشاریہ;  // Current statement being tracked, nullptr=ignored
    ثابت Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    ثابت AstScope* m_scopep = خالی_اشاریہ;  // Current scope being processed
    Ast_فعال* m_activep = خالی_اشاریہ;  // Current active
    منطقی m_اندر_Clocked_فعال = جھوٹ;  // Underneath clocked active
    منطقی m_اندر_کنارہ_فعال = جھوٹ;  // Underneath edge active
    منطقی m_اندر_Static_فعال = جھوٹ;  // Underneath static active
    منطقی m_اندر_Initial_فعال = جھوٹ;  // Underneath initial active
    منطقی m_اندر_Sen_شے = جھوٹ;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    خالی check_گرہ(Ast_گرہ* nodep) {
        اگر (nodep->isOutputter()) m_logicVertexp->سیٹ_Consumed("outputter");
        اگر (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->سیٹ_Consumed("TimingControl");
        }
    }

    خالی iterateLogic(Ast_گرہ* nodep, منطقی slow = جھوٹ, ثابت حرف* nonReducibleReason = خالی_اشاریہ,
                      ثابت حرف* consumeReason = خالی_اشاریہ) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = نیا Gate_Logic_راس{m_graphp, nodep, m_activep, slow};
        اگر (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } ورنہ اگر (m_اندر_Clocked_فعال) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } ورنہ اگر (m_اندر_Static_فعال || m_اندر_Initial_فعال) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->سیٹ_Static_Init();
        }
        اگر (consumeReason) m_logicVertexp->سیٹ_Consumed(consumeReason);
        check_گرہ(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    خالی دورہ(Ast_فعال* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_اندر_Clocked_فعال);
        VL_RESTORER(m_اندر_کنارہ_فعال);
        VL_RESTORER(m_اندر_Static_فعال);
        VL_RESTORER(m_اندر_Initial_فعال);
        m_activep = nodep;
        m_اندر_Clocked_فعال = nodep->hasClocked();
        m_اندر_کنارہ_فعال = nodep->sentreep() && nodep->sentreep()->has_کنارہ();
        m_اندر_Static_فعال = nodep->hasStatic();
        m_اندر_Initial_فعال = nodep->hasInitial();

        // AstVarScope::user2 -> bool: Signal used in SenItem in *this* active block
        ثابت VNUser2_اندر_Use user2_اندر_Use;
        iterateChildrenConst(nodep);
    }

    خالی دورہ(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        ثابت منطقی slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? خالی_اشاریہ : "Multiple Stmts");
    }
    خالی دورہ(AstCoverToggle* nodep) override {
        iterateLogic(nodep, جھوٹ, "CoverToggle", "CoverToggle");
    }
    خالی دورہ(Ast_Sen_شے* nodep) override {
        VL_RESTORER(m_اندر_Sen_شے);
        m_اندر_Sen_شے = سچ;
        اگر (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } ورنہ {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, جھوٹ, خالی_اشاریہ, "senItem");
        }
    }
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        اگر (!m_logicVertexp) واپس;

        AstVarScope* ثابت vscp = nodep->varScopep();
        Gate_Var_راس* ثابت vVtxp = m_graphp->make_Var_راس(vscp);

        اگر (m_اندر_Sen_شے) {
            vVtxp->سیٹ_Is_Clock();
            vscp->user2(سچ);
        } ورنہ اگر (m_اندر_کنارہ_فعال && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            اگر (vscp->user2()) {
                اگر (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } ورنہ {
                اگر (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        اگر (nodep->access().isWriteOrRW()) m_graphp->جوڑو_کنارہ(m_logicVertexp, vVtxp, 1);
        اگر (nodep->access().isReadOrRW()) m_graphp->جوڑو_کنارہ(vVtxp, m_logicVertexp, 1);
    }
    خالی دورہ(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), گرہ_تفویض)
                      && VN_AS(nodep->backp(), گرہ_تفویض)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (m_logicVertexp) check_گرہ(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    صریح GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

عوامی:
    ساکن std::unique_ptr<Gate_گراف> apply(AstNetlist* netlistp) {
        واپس std::unique_ptr<Gate_گراف>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ساکن خالی v3GateWarnSyncAsync(Gate_گراف& گراف) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ثابت VNUser2_اندر_Use user2_اندر_Use;
    برائے (V3_گراف_راس& vtx : گراف.vertices()) {
        اگر (ثابت Gate_Var_راس* ثابت vvertexp = vtx.cast<Gate_Var_راس>()) {
            ثابت AstVarScope* ثابت vscp = vvertexp->varScp();
            ثابت Ast_گرہ* ثابت sp = vvertexp->rstSyncNodep();
            ثابت Ast_گرہ* ثابت ap = vvertexp->rstAsyncNodep();
            اگر (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                اگر (!vscp->fileline()->warnIsOff(V3_Error_رمز::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3_Error_رمز::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3_Error_رمز::SYNCASYNCNET)) {
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

جماعت GateConcatVisitor final : عوامی VNVisitorConst {
    // STATE
    ثابت AstVarScope* m_vscp = خالی_اشاریہ;  // Varscope we're trying to find
    صحیح m_انحراف = 0;  // Current offset of varscope
    صحیح m_ملا_انحراف = 0;  // Found offset of varscope
    منطقی m_ملا = جھوٹ;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        اگر (nodep->varScopep() == m_vscp && !nodep->user2() && !m_ملا) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(سچ);
            m_ملا_انحراف = m_انحراف;
            m_ملا = سچ;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_انحراف << ") - " << nodep);
        }
        m_انحراف += nodep->dtypep()->چوڑائی();
    }
    خالی دورہ(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }

عوامی:
    // PUBLIC METHODS
    منطقی concat_انحراف(AstConcat* concatp, AstVarScope* vscp, صحیح& offsetr) {
        m_vscp = vscp;
        m_انحراف = 0;
        m_ملا = جھوٹ;
        // Iterate
        iterateConst(concatp);
        offsetr = m_ملا_انحراف;
        واپس m_ملا;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

جماعت GateOkVisitor final : عوامی VNVisitorConst {
    // RETURN STATE
    منطقی m_isSimple = سچ;  // Set false when we know it isn't simple
    std::سمتیہ<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_گرہ_اظہاریہ* m_substitutionp = خالی_اشاریہ;  // What to replace the variable with

    // STATE
    ثابت منطقی m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ثابت منطقی m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ثابت Ast_گرہ_Var_Ref* m_lhsVarRef = خالی_اشاریہ;
    صحیح m_ops = 0;  // Operation count

    // METHODS
    خالی clearSimple(ثابت حرف* because) {
        اگر (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = جھوٹ;
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Var_Ref* nodep) override {
        اگر (!m_isSimple) واپس;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        اگر (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        اگر (nodep->access().isRW()) {
            clearSimple("R/W");
            واپس;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ثابت vscp = nodep->varScopep();
        اگر (nodep->access().isWriteOnly()) {
            اگر (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } ورنہ {
            // TODO: possible bug, should it be >= 1 as add is below?
            اگر (m_readVscps.سائز() > 1) {
                اگر (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ڈالو_عقب(vscp);
        }
    }
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (!m_isSimple) واپس;

        m_substitutionp = nodep->rhsp();
        اگر (!VN_IS(nodep->lhsp(), گرہ_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } ورنہ اگر (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } ورنہ {
            iterateChildrenConst(nodep);
        }
        اگر (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    خالی دورہ(AstCReset* /*nodep*/) override {
        اگر (!m_isSimple) واپس;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    خالی دورہ(Ast_مطابقت_Masked* nodep) override {
        اگر (!m_isSimple) واپس;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override {
        اگر (!m_isSimple) واپس;  // Fastpath

        اگر (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            واپس;
        }

        اگر (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->is_خالص() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            واپس;
        }

        iterateChildrenConst(nodep);
    }

عوامی:
    // CONSTRUCTORS
    GateOkVisitor(Ast_گرہ* nodep, منطقی buffersOnly, منطقی dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        اگر (!m_substitutionp) {
            clearSimple("No assignment found\n");
            واپس;
        }
        اگر (m_isSimple && m_lhsVarRef) {
            برائے (ثابت AstVarScope* ثابت vscp : m_readVscps) {
                اگر (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    واپس;
                }
            }
        }
    }
    ~GateOkVisitor() override = طےشدہ;
    // PUBLIC METHODS
    منطقی isSimple() ثابت { واپس m_isSimple; }
    Ast_گرہ_اظہاریہ* substitutionp() ثابت {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        واپس m_substitutionp;
    }
    ثابت std::سمتیہ<AstVarScope*>& readVscps() ثابت { واپس m_readVscps; }
    منطقی varAssigned(ثابت AstVarScope* scopep) ثابت {
        واپس m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

جماعت GateInline final {
    استعمال Substitutions = std::unordered_map<AstVarScope*, Ast_گرہ_اظہاریہ*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ثابت VNUser2_اندر_Use m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_گرہ, Substitutions> m_substitutions;

    // STATE
    Gate_گراف& m_گراف;
    سائز_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_گرہ*, سائز_t> m_hasPending;
    سائز_t m_statInlined = 0;  // Statistic tracking - signals inlined
    سائز_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    سائز_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    ساکن منطقی isCheap(ثابت Ast_گرہ_اظہاریہ* exprp) {
        // Constant is cheap
        اگر (VN_IS(exprp, Const)) واپس سچ;
        // Variable reference is cheap
        اگر (VN_IS(exprp, گرہ_Var_Ref)) واپس سچ;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        اگر (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            اگر (!isCheap(selp->fromp())) واپس جھوٹ;
            اگر (!selp->isWide()) واپس سچ;
            اگر (!VN_IS(selp->lsbp(), Const)) واپس جھوٹ;
            اگر (selp->lsbConst() % VL_EDATASIZE != 0) واپس جھوٹ;
            واپس سچ;
        }
        // AstArraySel is cheap if the fromp is cheap
        اگر (ثابت AstArraySel* ثابت aselp = VN_CAST(exprp, ArraySel)) {
            واپس isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        واپس جھوٹ;
    }

    منطقی shouldInline(Gate_Var_راس* vVtxp, Gate_Logic_راس* lVtxp, سائز_t nReads,
                      Ast_گرہ_اظہاریہ* substp, منطقی allow_Multi_اندر) {
        // Always inline constants
        اگر (VN_IS(substp, Const)) واپس سچ;
        // Don't inline non-constant static initializers - these are scheduled differently
        اگر (lVtxp->staticInit()) واپس جھوٹ;
        // Inline simple variable references
        اگر (VN_IS(substp, VarRef)) واپس سچ;
        // Only inline arrays if a simple variable or constant
        اگر (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) واپس جھوٹ;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        اگر (nReads == 0) واپس سچ;
        // If it reads one variable, inline if not wide, or if cheap
        اگر (nReads == 1 && (!substp->isWide() || isCheap(substp))) واپس سچ;
        // Don't inline on first round if reads more than one variable
        اگر (nReads > 1 && !allow_Multi_اندر) واپس جھوٹ;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        صحیح n = 0;
        برائے (V3_گراف_کنارہ& کنارہ : vVtxp->برآمد_کنارے()) {
            ثابت Gate_Logic_راس* ثابت dstVtxp = کنارہ.چوٹی()->as<Gate_Logic_راس>();
            // Ignore slow code, or if the destination is not used
            اگر (dstVtxp->slow()) جاری;
            اگر (dstVtxp->برآمد_خالی_خانے() && !dstVtxp->consumed()) جاری;
            n += کنارہ.وزن_2();
            اگر (n > 1) {
                ++m_statNotInlined;
                واپس جھوٹ;
            }
        }
        واپس سچ;
    }

    خالی ریکارڈ_Substitution(AstVarScope* vscp, Ast_گرہ_اظہاریہ* substp, Ast_گرہ* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ثابت خودکار pair = m_substitutions(logicp).emplace(vscp, خالی_اشاریہ);
        اگر (pair.دوسرا) pair.پہلا->دوسرا = substp->clone_درخت_خالص(جھوٹ);
    }

    خالی commitSubstitutions(Ast_گرہ* logicp) {
        اگر (!m_hasPending.erase(logicp)) واپس;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.خالی_خانے(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_سیٹ<ثابت AstVarScope*> replaced(substitutions.سائز() * 2);

        ثابت std::function<خالی(Ast_گرہ_Var_Ref*)> دورہ = [&](Ast_گرہ_Var_Ref* nodep) -> خالی {
            // See if this variable has a substitution
            AstVarScope* ثابت vscp = nodep->varScopep();
            ثابت خودکار& it = substitutions.ڈھونڈو(vscp);
            اگر (it == substitutions.انتہا()) واپس;

            // Do not substitute circular logic
            اگر (!replaced.داخل_کرو(vscp).دوسرا) واپس;

            // Substitute nodep with substp
            Ast_گرہ_اظہاریہ* ثابت substp = it->دوسرا;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, گرہ_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_گرہ_اظہاریہ* ثابت newp = substp->clone_درخت_خالص(جھوٹ);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            اگر (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            اگر (Ast_گرہ_Var_Ref* ثابت varrefp = VN_CAST(newp, گرہ_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Recursively substitute the new tree
            newp->foreach(دورہ);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(دورہ);

        Ast_گرہ* ثابت simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        برائے (ثابت خودکار& pair : substitutions) pair.دوسرا->delete_درخت();
        substitutions.clear();
    }

    خالی optimize_اشارات(منطقی allow_Multi_اندر) {
        // Consider "inlining" variables
        خودکار& vertices = m_گراف.vertices();
        ثابت خودکار ff_مقصد_Var_Vtx = [&](V3_گراف_راس::List::iterator it) {
            جب (it != vertices.انتہا() && !(*it).is<Gate_Var_راس>()) ++it;
            واپس it;
        };
        V3_گراف_راس::List::iterator vIt = ff_مقصد_Var_Vtx(vertices.begin());
        جب (vIt != vertices.انتہا()) {
            Gate_Var_راس* ثابت vVtxp = (*vIt).as<Gate_Var_راس>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ff_مقصد_Var_Vtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            اگر (!vVtxp->اندر_Size1()) جاری;

            // Can't inline if non-reducible, etc
            اگر (!vVtxp->reducible()) جاری;

            // Grab the driving logic
            Gate_Logic_راس* ثابت lVtxp
                = vVtxp->اندر_کنارے().frontp()->fromp()->as<Gate_Logic_راس>();
            اگر (!lVtxp->reducible()) جاری;
            Ast_گرہ* ثابت logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ثابت GateOkVisitor okVisitor{logicp, جھوٹ, جھوٹ};

            // Was it ok?
            اگر (!okVisitor.isSimple()) جاری;
            // If the varScope is already removed from logicp, no need to try substitution.
            اگر (!okVisitor.varAssigned(vVtxp->varScp())) جاری;

            // Expression we are considering to substitute with
            Ast_گرہ_اظہاریہ* ثابت substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ثابت سائز_t nReads = okVisitor.readVscps().سائز();

            // Check if it should be inlined
            اگر (!shouldInline(vVtxp, lVtxp, nReads, substp, allow_Multi_اندر)) جاری;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ثابت vscp = vVtxp->varScp();

            اگر (debug() >= 9) {
                vscp->dump_درخت("substituting: ");
                substp->dump_درخت("        with: ");
            }

            ثابت خودکار& readVscpsVec = okVisitor.readVscps();
            ثابت std::unordered_سیٹ<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.انتہا()};

            برائے (V3_گراف_کنارہ* ثابت edgep : vVtxp->برآمد_کنارے().unlinkable()) {
                Gate_Logic_راس* ثابت dstVtxp = edgep->چوٹی()->as<Gate_Logic_راس>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                اگر (VN_IS(dstVtxp->nodep(), Sen_شے)) {
                    Ast_گرہ* nodep = substp;
                    اگر (AstNot* ثابت notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    اگر (!VN_IS(nodep, VarRef)) جاری;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                منطقی canInline = سچ;
                برائے (ثابت V3_گراف_کنارہ& dedge : dstVtxp->برآمد_کنارے()) {
                    ثابت Gate_Var_راس* ثابت consVVertexp = dedge.چوٹی()->as<Gate_Var_راس>();
                    اگر (readVscps.گنتی(consVVertexp->varScp())) {
                        canInline = جھوٹ;
                        توڑو;
                    }
                }

                اگر (!canInline) جاری;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                اگر (logicp == dstVtxp->nodep()) {
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
                    جاری;
                }

                ریکارڈ_Substitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                برائے (AstVarScope* ثابت newVscp : okVisitor.readVscps()) {
                    Gate_Var_راس* ثابت varvertexp = m_گراف.make_Var_راس(newVscp);
                    m_گراف.جوڑو_کنارہ(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            اگر (vVtxp->برآمد_خالی_خانے()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_گراف), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlink_Fr_عقب()->delete_درخت(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_گراف), lVtxp);
            }
        }
    }

    صریح GateInline(Gate_گراف& گراف)
        : m_گراف{گراف} {
        // Find gate interconnect and optimize
        گراف.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimize_اشارات(جھوٹ);
        // Then propagate more complicated equations
        optimize_اشارات(سچ);
        // Commit substitutions in insertion order for stability
        استعمال Pair = std::pair<Ast_گرہ*, سائز_t>;
        std::سمتیہ<Pair> pending{m_hasPending.begin(), m_hasPending.انتہا()};
        std::ترتیب_دو(pending.begin(), pending.انتہا(), [](ثابت Pair& a, ثابت Pair& b) {  //
            واپس a.دوسرا < b.دوسرا;
        });
        برائے (ثابت خودکار& pair : pending) commitSubstitutions(pair.پہلا);
    }

    ~GateInline() {
        V3Stats::جوڑو_Stat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::جوڑو_Stat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::جوڑو_Stat("Optimizations, Gate reads replaced", m_statRefs);
    }

عوامی:
    ساکن خالی apply(Gate_گراف& گراف) { GateInline{گراف}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

جماعت GateDedupeHash final : عوامی V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    ساخت Aux_Ast_گرہ_اظہاریہ final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        Ast_فعال* activep = خالی_اشاریہ;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_گرہ_اظہاریہ* condp = خالی_اشاریہ;
        // Parent AstNodeAssign* for this rhsp
        Ast_گرہ_تفویض* parentp = خالی_اشاریہ;
    };
    AstUser2Allocator<Ast_گرہ_اظہاریہ, Aux_Ast_گرہ_اظہاریہ> m_aux_گرہ_اظہاریہ;

    Ast_گرہ_اظہاریہ* m_currRhsp = خالی_اشاریہ;  // Current node we are searching for duplicates of
    Aux_Ast_گرہ_اظہاریہ m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    ساکن منطقی same(Ast_گرہ* node1p, Ast_گرہ* node2p) {
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
        واپس node1p == node2p || (node1p && node1p->same_درخت(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    منطقی isSame(Ast_گرہ* node1p, Ast_گرہ* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ثابت خودکار& aux2 = m_aux_گرہ_اظہاریہ(VN_AS(node2p, گرہ_اظہاریہ));
        واپس m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

عوامی:
    GateDedupeHash() = طےشدہ;
    ~GateDedupeHash() = طےشدہ;

    ثابت Ast_گرہ_تفویض* hash_And_ڈھونڈو_Dupe(Ast_گرہ_تفویض* assignp, Ast_فعال* activep,
                                         Ast_گرہ_اظہاریہ* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ثابت خودکار it = m_dupFinder.ڈھونڈو_Duplicate(m_currRhsp, یہ);
        اگر (it != m_dupFinder.انتہا()) واپس m_aux_گرہ_اظہاریہ(VN_AS(it->دوسرا, گرہ_اظہاریہ)).parentp;

        // Insert new node
        m_dupFinder.داخل_کرو(m_currRhsp);
        m_aux_گرہ_اظہاریہ(m_currRhsp) = m_auxCurRhsp;
        واپس خالی_اشاریہ;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

جماعت GateDedupeVarVisitor final : عوامی VNVisitorConst {
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
    Ast_گرہ_تفویض* m_assignp = خالی_اشاریہ;  // Assign found for dedupe
    Ast_گرہ_اظہاریہ* m_ifCondp = خالی_اشاریہ;  // IF condition that assign is under
    منطقی m_always = جھوٹ;  // Assign is under an always
    منطقی m_dedupable = سچ;  // Determined the assign to be dedupable

    // VISITORS
    خالی دورہ(Ast_گرہ_تفویض* nodep) override {
        اگر (!m_dedupable) واپس;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        اگر (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_اظہاریہ_Stmt*) { واپس سچ; });
            واپس;
        }

        m_dedupable = جھوٹ;
    }

    خالی دورہ(AstAlways* nodep) override {
        اگر (!m_dedupable) واپس;

        اگر (!m_always) {
            m_always = سچ;
            iterate_And_اگلا_Const_Null(nodep->stmtsp());
            واپس;
        }

        m_dedupable = جھوٹ;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    خالی دورہ(Ast_گرہ_If* nodep) override {
        اگر (!m_dedupable) واپس;

        اگر (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_اظہاریہ_Stmt*) { واپس سچ; });
            iterate_And_اگلا_Const_Null(nodep->thensp());
            واپس;
        }

        m_dedupable = جھوٹ;
    }

    خالی دورہ(AstComment*) override {}  // NOP
    //--------------------
    خالی دورہ(Ast_گرہ*) override { m_dedupable = جھوٹ; }

عوامی:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = طےشدہ;
    ~GateDedupeVarVisitor() override = طےشدہ;
    // PUBLIC METHODS
    Ast_گرہ_Var_Ref* ڈھونڈو_Dupe(Ast_گرہ* logicp, AstVarScope* صارف_Vscp, Ast_فعال* activep) {
        m_assignp = خالی_اشاریہ;
        m_ifCondp = خالی_اشاریہ;
        m_always = جھوٹ;
        m_dedupable = سچ;
        iterateConst(logicp);
        اگر (m_dedupable && m_assignp) {
            ثابت Ast_گرہ* ثابت lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ثابت Ast_گرہ_Var_Ref* ثابت lRefp = VN_CAST(lhsp, گرہ_Var_Ref);
            اگر (lRefp && lRefp->varScopep() == صارف_Vscp) {
                اگر (ثابت Ast_گرہ_تفویض* ثابت dup
                    = m_ghash.hash_And_ڈھونڈو_Dupe(m_assignp, activep, m_ifCondp)) {
                    واپس static_cast<Ast_گرہ_Var_Ref*>(dup->lhsp());
                }
            }
        }
        واپس خالی_اشاریہ;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

جماعت GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ثابت VNUser2_اندر_Use m_inuser2;

    // STATE
    سائز_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_گہرائی = 0;  // Iteration depth

    خالی دورہ(Gate_Var_راس* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        اگر (m_گہرائی > GATE_DEDUP_MAX_گہرائی) واپس;

        // Check that we haven't been here before
        اگر (vVtxp->varScp()->user2_سیٹ_Once()) واپس;

        VL_RESTORER(m_گہرائی);
        ++m_گہرائی;
        اگر (!vVtxp->اندر_Size1()) واپس;

        Ast_گرہ_Var_Ref* dupRefp = خالی_اشاریہ;
        برائے (V3_گراف_کنارہ& کنارہ : vVtxp->اندر_کنارے()) {
            dupRefp = دورہ(کنارہ.fromp()->as<Gate_Logic_راس>(), vVtxp);
        }
        اگر (!dupRefp) واپس;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        Gate_Logic_راس* ثابت lVtxp = vVtxp->اندر_کنارے().frontp()->fromp()->as<Gate_Logic_راس>();
        ثابت GateOkVisitor okVisitor{lVtxp->nodep(), جھوٹ, سچ};
        اگر (!okVisitor.isSimple()) واپس;

        ++m_statDedupLogic;
        Gate_Var_راس* ثابت dupVVtxp = dupRefp->varScopep()->user1u().مقصد<Gate_Var_راس*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        برائے (V3_گراف_کنارہ* ثابت edgep : vVtxp->برآمد_کنارے().unlinkable()) {
            ثابت Gate_Logic_راس* ثابت صارف_Vtxp = edgep->چوٹی()->as<Gate_Logic_راس>();
            Ast_گرہ* ثابت consumerp = صارف_Vtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << صارف_Vtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            اگر (lVtxp == صارف_Vtxp) {
                UINFO(9, "skipping as self-recirculates");
            } ورنہ {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_گرہ_Var_Ref* refp) {
                    اگر (refp->varScopep() != vVtxp->varScp()) واپس;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_گرہ_Var_Ref* ثابت newp = dupRefp->clone_درخت_خالص(جھوٹ);
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
        جب (V3_گراف_کنارہ* ثابت edgep = vVtxp->اندر_کنارے().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_گرہ_Var_Ref* دورہ(Gate_Logic_راس* lVtxp, ثابت Gate_Var_راس* صارف_Vtxp) {
        برائے (V3_گراف_کنارہ& کنارہ : lVtxp->اندر_کنارے()) دورہ(کنارہ.fromp()->as<Gate_Var_راس>());

        اگر (lVtxp->dedupable() && صارف_Vtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            واپس m_varVisitor.ڈھونڈو_Dupe(lVtxp->nodep(), صارف_Vtxp->varScp(), lVtxp->activep());
        }
        واپس خالی_اشاریہ;
    }

    صریح GateDedupe(Gate_گراف& گراف) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        برائے (V3_گراف_راس& vtx : گراف.vertices()) {
            اگر (Gate_Var_راس* ثابت vVtxp = vtx.cast<Gate_Var_راس>()) {
                اگر (vVtxp->isClock()) دورہ(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        برائے (V3_گراف_راس& vtx : گراف.vertices()) {
            اگر (Gate_Var_راس* ثابت vVtxp = vtx.cast<Gate_Var_راس>()) {
                اگر (vVtxp->is_چوٹی() && vVtxp->varScp()->varp()->isWritable()) دورہ(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::جوڑو_Stat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

عوامی:
    ساکن خالی apply(Gate_گراف& گراف) { GateDedupe{گراف}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

جماعت Gate_ملاؤ_Assignments final {
    Gate_گراف& m_گراف;
    سائز_t m_stat_تفویض_Merged = 0;  // Statistic tracking
    std::سمتیہ<Gate_Logic_راس*> m_مقصد_Remove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* ملاؤ(AstSel* پچھلا_Selp, AstSel* currSelp) {
        ثابت AstVarRef* ثابت pRefp = VN_CAST(پچھلا_Selp->fromp(), VarRef);
        AstVarRef* ثابت cRefp = VN_CAST(currSelp->fromp(), VarRef);
        اگر (!pRefp || !cRefp || !cRefp->same_گرہ(pRefp)) واپس خالی_اشاریہ;  // not the same var

        ثابت AstConst* ثابت pstart = VN_CAST(پچھلا_Selp->lsbp(), Const);
        ثابت AstConst* ثابت cstart = VN_CAST(currSelp->lsbp(), Const);
        اگر (!pstart || !cstart) واپس خالی_اشاریہ;  // too complicated

        اگر (currSelp->msbConst() + 1 == پچھلا_Selp->lsbConst()) {
            واپس نیا AstSel{cRefp->fileline(), cRefp->clone_درخت(جھوٹ), currSelp->lsbConst(),
                              پچھلا_Selp->چوڑائی_Const() + currSelp->چوڑائی_Const()};
        } ورنہ {
            واپس خالی_اشاریہ;
        }
    }

    خالی process(Gate_Var_راس* vVtxp) {
        Gate_Logic_راس* پچھلا_LVtxp = خالی_اشاریہ;
        Ast_تفویض_W* پچھلا_Assignp = خالی_اشاریہ;

        برائے (V3_گراف_کنارہ* ثابت edgep : vVtxp->اندر_کنارے().unlinkable()) {
            Gate_Logic_راس* ثابت lVtxp = edgep->fromp()->as<Gate_Logic_راس>();
            اگر (!lVtxp->برآمد_Size1()) جاری;

            AstAlways* ثابت alwaysp = VN_CAST(lVtxp->nodep(), Always);
            اگر (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) واپس;
            Ast_تفویض_W* ثابت assignp = VN_CAST(alwaysp->stmtsp(), تفویض_W);
            اگر (!assignp) جاری;

            اگر (!VN_IS(assignp->lhsp(), Sel)) جاری;

            // First assign with Sel-lhs, or not under the same active
            اگر (!پچھلا_LVtxp || پچھلا_LVtxp->activep() != lVtxp->activep()) {
                پچھلا_LVtxp = lVtxp;
                پچھلا_Assignp = assignp;
                جاری;
            }

            AstSel* ثابت پچھلا_Selp = VN_AS(پچھلا_Assignp->lhsp(), Sel);
            AstSel* ثابت currSelp = VN_AS(assignp->lhsp(), Sel);

            اگر (AstSel* ثابت newSelp = ملاؤ(پچھلا_Selp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                پچھلا_Selp->replaceWith(newSelp);
                VL_DO_DANGLING(پچھلا_Selp->delete_درخت(), پچھلا_Selp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                پچھلا_Assignp->rhsp(نیا AstConcat{پچھلا_Assignp->rhsp()->fileline(),
                                                پچھلا_Assignp->rhsp()->unlink_Fr_عقب(),
                                                assignp->rhsp()->unlink_Fr_عقب()});
                // Why do we care about the type of an assignment?
                پچھلا_Assignp->dtype_Chg_چوڑائی_Signed(پچھلا_Assignp->چوڑائی() + assignp->چوڑائی(),
                                                 پچھلا_Assignp->چوڑائی() + assignp->چوڑائی(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_مقصد_Remove.emplace_عقب(lVtxp);

                // Update the graph
                جب (V3_گراف_کنارہ* ثابت iedgep = lVtxp->اندر_کنارے().frontp()) {
                    Gate_Var_راس* ثابت fromVtxp = iedgep->fromp()->as<Gate_Var_راس>();
                    m_گراف.جوڑو_کنارہ(fromVtxp, پچھلا_LVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_stat_تفویض_Merged;
            } ورنہ {
                پچھلا_LVtxp = lVtxp;
                پچھلا_Assignp = assignp;
            }
        }
    }

    صریح Gate_ملاؤ_Assignments(Gate_گراف& گراف)
        : m_گراف{گراف} {
        UINFO(6, "mergeAssigns");
        برائے (V3_گراف_راس& vtx : گراف.vertices()) {
            اگر (Gate_Var_راس* ثابت vVtxp = vtx.cast<Gate_Var_راس>()) process(vVtxp);
        }
        // Delete merged assignments
        برائے (Gate_Logic_راس* ثابت lVtxp : m_مقصد_Remove) {
            Ast_گرہ* ثابت nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_گراف), lVtxp);
        }
    }

    ~Gate_ملاؤ_Assignments() {
        V3Stats::جوڑو_Stat("Optimizations, Gate assign merged", m_stat_تفویض_Merged);
    }

عوامی:
    ساکن خالی apply(Gate_گراف& گراف) { Gate_ملاؤ_Assignments{گراف}; }
};

//######################################################################
// GateUnused

جماعت Gate_غیر_مستعمل final {
    // STATE
    Gate_گراف& m_گراف;

    // METHODS

    خالی markRecurse(Gate_Either_راس* vtxp) {
        اگر (vtxp->user()) واپس;  // Already marked
        vtxp->user(سچ);
        vtxp->سیٹ_Consumed("propagated");
        // Walk sources and mark them too
        برائے (V3_گراف_کنارہ& کنارہ : vtxp->اندر_کنارے()) {
            Gate_Either_راس* ثابت fromVtxp = static_cast<Gate_Either_راس*>(کنارہ.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    خالی mark() {
        m_گراف.userClearVertices();
        برائے (V3_گراف_راس& vtx : m_گراف.vertices()) {
            Gate_Either_راس& eVtx = static_cast<Gate_Either_راس&>(vtx);
            اگر (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ساکن خالی warn_غیر_مستعمل(ثابت Ast_گرہ* ثابت nodep) {
        اگر (nodep->fileline()->warnIsOff(V3_Error_رمز::UNUSEDLOOP)) واپس;

        اگر (ثابت Ast_گرہ_Procedure* ثابت procedurep = VN_CAST(nodep, گرہ_Procedure)) {
            اگر (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ثابت AstLoop* ثابت loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3_Error_رمز::UNUSEDLOOP, سچ);
                });
        }
    }

    // Remove unused logic
    خالی remove() {
        برائے (V3_گراف_راس* ثابت vtxp : m_گراف.vertices().unlinkable()) {
            اگر (Gate_Logic_راس* ثابت lVtxp = vtxp->cast<Gate_Logic_راس>()) {
                اگر (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_گرہ* ثابت nodep = lVtxp->nodep();
                    warn_غیر_مستعمل(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlink_Fr_عقب();
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    VL_DO_DANGLING(lVtxp->unlinkDelete(&m_گراف), lVtxp);
                }
            }
        }
    }

    صریح Gate_غیر_مستعمل(Gate_گراف& گراف)
        : m_گراف{گراف} {
        mark();  // Mark all used vertices
        remove();  // Remove unused vertices
    }

عوامی:
    ساکن خالی apply(Gate_گراف& گراف) { Gate_غیر_مستعمل{گراف}; }
};

//######################################################################
// Pass entry point

خالی V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<Gate_گراف> graphp = GateBuildVisitor::apply(netlistp);
        اگر (dump_گراف_Level() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->remove_Redundant_کنارے_مجموع(&V3_گراف_کنارہ::followAlwaysTrue);
        اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        Gate_غیر_مستعمل::apply(*graphp);
        اگر (dump_گراف_Level() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        اگر (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        اگر (v3Global.opt.fAssemble()) {
            Gate_ملاؤ_Assignments::apply(*graphp);
            اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        Gate_غیر_مستعمل::apply(*graphp);
        اگر (dump_گراف_Level() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_درخت("gate", 0, dump_درخت_Either_Level() >= 3);
}
