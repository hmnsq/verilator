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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Gate.h"

#అన్తర్భావ "V3AstUserAllocator.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3DupFinder.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <list>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_లోతు = 20;

//######################################################################
// Gate graph classes

తరగతి Gate_Either_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Gate_Either_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    తార్కిక m_reducible = నిజం;  // True if this node should be able to be eliminated
    తార్కిక m_dedupable = నిజం;  // True if this node should be able to be deduped
    తార్కిక m_consumed = అబద్ధం;  // Output goes to something meaningful
    తార్కిక m_staticInit = అబద్ధం;  // True if this node is a static initializer
బహిరంగ:
    స్పష్టమైన Gate_Either_శీర్షం(V3_లేఖాచిత్రం* graphp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp} {}
    ~Gate_Either_శీర్షం() override = అప్రమేయ;

    // ACCESSORS
    తార్కిక reducible() స్థిరరాశి { తిరిగి m_reducible; }
    తార్కిక dedupable() స్థిరరాశి { తిరిగి m_dedupable; }
    తార్కిక consumed() స్థిరరాశి { తిరిగి m_consumed; }
    తార్కిక staticInit() స్థిరరాశి { తిరిగి m_staticInit; }
    శూన్య సమితి_Consumed(స్థిరరాశి అక్షరం* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = నిజం;
    }
    శూన్య సమితి_Static_Init() { m_staticInit = నిజం; }
    శూన్య clearReducible(స్థిరరాశి అక్షరం* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = అబద్ధం;
    }
    శూన్య clearDedupable(స్థిరరాశి అక్షరం* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = అబద్ధం;
    }
    శూన్య clearReducibleAndDedupable(స్థిరరాశి అక్షరం* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() స్థిరరాశి override { తిరిగి m_consumed ? "" : "dotted"; }
};

తరగతి Gate_Var_శీర్షం final : బహిరంగ Gate_Either_శీర్షం {
    VL_RTTI_IMPL(Gate_Var_శీర్షం, Gate_Either_శీర్షం)
    AstVarScope* స్థిరరాశి m_varScp;
    తార్కిక m_is_శిఖరం = అబద్ధం;
    తార్కిక m_isClock = అబద్ధం;
    Ast_కణుపు* m_rstSyncNodep = శూన్యసూచిక;  // Used as reset and not in SenItem, in clocked always
    Ast_కణుపు* m_rstAsyncNodep = శూన్యసూచిక;  // Used as reset and in SenItem, in clocked always
బహిరంగ:
    Gate_Var_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* varScp)
        : Gate_Either_శీర్షం{graphp}
        , m_varScp{varScp} {}
    ~Gate_Var_శీర్షం() override = అప్రమేయ;

    // ACCESSORS
    AstVarScope* varScp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_varScp; }
    తార్కిక is_శిఖరం() స్థిరరాశి { తిరిగి m_is_శిఖరం; }
    శూన్య సమితి_Is_శిఖరం() { m_is_శిఖరం = నిజం; }
    తార్కిక isClock() స్థిరరాశి { తిరిగి m_isClock; }
    శూన్య సమితి_Is_Clock() { m_isClock = నిజం; }
    Ast_కణుపు* rstSyncNodep() స్థిరరాశి { తిరిగి m_rstSyncNodep; }
    శూన్య rstSyncNodep(Ast_కణుపు* nodep) { m_rstSyncNodep = nodep; }
    Ast_కణుపు* rstAsyncNodep() స్థిరరాశి { తిరిగి m_rstAsyncNodep; }
    శూన్య rstAsyncNodep(Ast_కణుపు* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    శూన్య propagateAttrFrom(Gate_Var_శీర్షం* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        ఒకవేళ (fromp->isClock()) సమితి_Is_Clock();
    }

    // DOT debug
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి varScp()->name(); }
    std::string dotColor() స్థిరరాశి override { తిరిగి "green"; }
};

తరగతి Gate_Logic_శీర్షం final : బహిరంగ Gate_Either_శీర్షం {
    VL_RTTI_IMPL(Gate_Logic_శీర్షం, Gate_Either_శీర్షం)
    Ast_కణుపు* స్థిరరాశి m_nodep;
    Ast_చురుకైన* స్థిరరాశి m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    స్థిరరాశి తార్కిక m_slow;  // In slow block
బహిరంగ:
    Gate_Logic_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep, Ast_చురుకైన* activep, తార్కిక slow)
        : Gate_Either_శీర్షం{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~Gate_Logic_శీర్షం() override = అప్రమేయ;

    // ACCESSORS
    File_పంక్తి* fileline() స్థిరరాశి override { తిరిగి nodep()->fileline(); }
    Ast_కణుపు* nodep() స్థిరరాశి { తిరిగి m_nodep; }
    Ast_చురుకైన* activep() స్థిరరాశి { తిరిగి m_activep; }
    తార్కిక slow() స్థిరరాశి { తిరిగి m_slow; }

    // DOT debug
    std::string name() స్థిరరాశి override { తిరిగి m_nodep->fileline()->ascii(); }
    std::string dotColor() స్థిరరాశి override { తిరిగి "red"; }
};

తరగతి Gate_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    std::string dotLabel() స్థిరరాశి override { తిరిగి std::గమ్యం_string(బరువు_2()); }

బహిరంగ:
    Gate_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2)
        : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, బరువు_2} {}
};

తరగతి Gate_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;

బహిరంగ:
    Gate_Var_శీర్షం* make_Var_శీర్షం(AstVarScope* vscp) {
        Gate_Var_శీర్షం* vVtxp = reinterpret_cast<Gate_Var_శీర్షం*>(vscp->user1p());
        ఒకవేళ (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = కొత్త Gate_Var_శీర్షం{ఇది, vscp};
            vscp->user1p(vVtxp);
            ఒకవేళ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->సమితి_Consumed("VirtIface");
            }
            ఒకవేళ (vscp->varp()->is_Sig_ప్రజా()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->సమితి_Consumed("SigPublic");
            }
            ఒకవేళ (vscp->varp()->isIO() && vscp->scopep()->is_శిఖరం()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->సమితి_Is_శిఖరం();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->సమితి_Consumed("isTop");
            }
        }
        తిరిగి vVtxp;
    }

    శూన్య కూడు_అంచు(Gate_Var_శీర్షం* srcp, Gate_Logic_శీర్షం* dstp, పూర్ణాంకం బరువు_2) {
        కొత్త Gate_అంచు{ఇది, srcp, dstp, బరువు_2};
    }

    శూన్య కూడు_అంచు(Gate_Logic_శీర్షం* srcp, Gate_Var_శీర్షం* dstp, పూర్ణాంకం బరువు_2) {
        కొత్త Gate_అంచు{ఇది, srcp, dstp, బరువు_2};
    }
};

//######################################################################
// GateGraph builder

తరగతి GateBuildVisitor final : బహిరంగ VNVisitorConst {
    // STATE
    Gate_లేఖాచిత్రం* m_graphp = కొత్త Gate_లేఖాచిత్రం{};  // The graph being built (var usages/dependencies)
    Gate_Logic_శీర్షం* m_logicVertexp = శూన్యసూచిక;  // Current statement being tracked, nullptr=ignored
    స్థిరరాశి Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    స్థిరరాశి AstScope* m_scopep = శూన్యసూచిక;  // Current scope being processed
    Ast_చురుకైన* m_activep = శూన్యసూచిక;  // Current active
    తార్కిక m_లోపల_Clocked_చురుకైన = అబద్ధం;  // Underneath clocked active
    తార్కిక m_లోపల_అంచు_చురుకైన = అబద్ధం;  // Underneath edge active
    తార్కిక m_లోపల_Static_చురుకైన = అబద్ధం;  // Underneath static active
    తార్కిక m_లోపల_Initial_చురుకైన = అబద్ధం;  // Underneath initial active
    తార్కిక m_లోపల_Sen_అంశం = అబద్ధం;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    శూన్య check_కణుపు(Ast_కణుపు* nodep) {
        ఒకవేళ (nodep->isOutputter()) m_logicVertexp->సమితి_Consumed("outputter");
        ఒకవేళ (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->సమితి_Consumed("TimingControl");
        }
    }

    శూన్య iterateLogic(Ast_కణుపు* nodep, తార్కిక slow = అబద్ధం, స్థిరరాశి అక్షరం* nonReducibleReason = శూన్యసూచిక,
                      స్థిరరాశి అక్షరం* consumeReason = శూన్యసూచిక) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = కొత్త Gate_Logic_శీర్షం{m_graphp, nodep, m_activep, slow};
        ఒకవేళ (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } లేకపోతే ఒకవేళ (m_లోపల_Clocked_చురుకైన) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } లేకపోతే ఒకవేళ (m_లోపల_Static_చురుకైన || m_లోపల_Initial_చురుకైన) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->సమితి_Static_Init();
        }
        ఒకవేళ (consumeReason) m_logicVertexp->సమితి_Consumed(consumeReason);
        check_కణుపు(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_లోపల_Clocked_చురుకైన);
        VL_RESTORER(m_లోపల_అంచు_చురుకైన);
        VL_RESTORER(m_లోపల_Static_చురుకైన);
        VL_RESTORER(m_లోపల_Initial_చురుకైన);
        m_activep = nodep;
        m_లోపల_Clocked_చురుకైన = nodep->hasClocked();
        m_లోపల_అంచు_చురుకైన = nodep->sentreep() && nodep->sentreep()->has_అంచు();
        m_లోపల_Static_చురుకైన = nodep->hasStatic();
        m_లోపల_Initial_చురుకైన = nodep->hasInitial();

        // AstVarScope::user2 -> bool: Signal used in SenItem in *this* active block
        స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
        iterateChildrenConst(nodep);
    }

    శూన్య సందర్శించు(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        స్థిరరాశి తార్కిక slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? శూన్యసూచిక : "Multiple Stmts");
    }
    శూన్య సందర్శించు(AstCoverToggle* nodep) override {
        iterateLogic(nodep, అబద్ధం, "CoverToggle", "CoverToggle");
    }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        VL_RESTORER(m_లోపల_Sen_అంశం);
        m_లోపల_Sen_అంశం = నిజం;
        ఒకవేళ (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } లేకపోతే {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, అబద్ధం, శూన్యసూచిక, "senItem");
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (!m_logicVertexp) తిరిగి;

        AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
        Gate_Var_శీర్షం* స్థిరరాశి vVtxp = m_graphp->make_Var_శీర్షం(vscp);

        ఒకవేళ (m_లోపల_Sen_అంశం) {
            vVtxp->సమితి_Is_Clock();
            vscp->user2(నిజం);
        } లేకపోతే ఒకవేళ (m_లోపల_అంచు_చురుకైన && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            ఒకవేళ (vscp->user2()) {
                ఒకవేళ (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } లేకపోతే {
                ఒకవేళ (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        ఒకవేళ (nodep->access().isWriteOrRW()) m_graphp->కూడు_అంచు(m_logicVertexp, vVtxp, 1);
        ఒకవేళ (nodep->access().isReadOrRW()) m_graphp->కూడు_అంచు(vVtxp, m_logicVertexp, 1);
    }
    శూన్య సందర్శించు(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), కణుపు_కేటాయించు)
                      && VN_AS(nodep->backp(), కణుపు_కేటాయించు)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (m_logicVertexp) check_కణుపు(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    స్పష్టమైన GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

బహిరంగ:
    స్థిర std::unique_ptr<Gate_లేఖాచిత్రం> apply(AstNetlist* netlistp) {
        తిరిగి std::unique_ptr<Gate_లేఖాచిత్రం>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

స్థిర శూన్య v3GateWarnSyncAsync(Gate_లేఖాచిత్రం& లేఖాచిత్రం) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : లేఖాచిత్రం.vertices()) {
        ఒకవేళ (స్థిరరాశి Gate_Var_శీర్షం* స్థిరరాశి vvertexp = vtx.cast<Gate_Var_శీర్షం>()) {
            స్థిరరాశి AstVarScope* స్థిరరాశి vscp = vvertexp->varScp();
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి sp = vvertexp->rstSyncNodep();
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి ap = vvertexp->rstAsyncNodep();
            ఒకవేళ (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                ఒకవేళ (!vscp->fileline()->warnIsOff(V3_Error_సంకేతం::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3_Error_సంకేతం::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3_Error_సంకేతం::SYNCASYNCNET)) {
                    vscp->varp()->user2(నిజం);  // Warn only once per signal
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

తరగతి GateConcatVisitor final : బహిరంగ VNVisitorConst {
    // STATE
    స్థిరరాశి AstVarScope* m_vscp = శూన్యసూచిక;  // Varscope we're trying to find
    పూర్ణాంకం m_స్థానభ్రంశం = 0;  // Current offset of varscope
    పూర్ణాంకం m_దొరికింది_స్థానభ్రంశం = 0;  // Found offset of varscope
    తార్కిక m_దొరికింది = అబద్ధం;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (nodep->varScopep() == m_vscp && !nodep->user2() && !m_దొరికింది) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(నిజం);
            m_దొరికింది_స్థానభ్రంశం = m_స్థానభ్రంశం;
            m_దొరికింది = నిజం;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_స్థానభ్రంశం << ") - " << nodep);
        }
        m_స్థానభ్రంశం += nodep->dtypep()->వెడల్పు();
    }
    శూన్య సందర్శించు(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

బహిరంగ:
    // PUBLIC METHODS
    తార్కిక concat_స్థానభ్రంశం(AstConcat* concatp, AstVarScope* vscp, పూర్ణాంకం& offsetr) {
        m_vscp = vscp;
        m_స్థానభ్రంశం = 0;
        m_దొరికింది = అబద్ధం;
        // Iterate
        iterateConst(concatp);
        offsetr = m_దొరికింది_స్థానభ్రంశం;
        తిరిగి m_దొరికింది;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

తరగతి GateOkVisitor final : బహిరంగ VNVisitorConst {
    // RETURN STATE
    తార్కిక m_isSimple = నిజం;  // Set false when we know it isn't simple
    std::సదిశ<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_కణుపు_సమాసం* m_substitutionp = శూన్యసూచిక;  // What to replace the variable with

    // STATE
    స్థిరరాశి తార్కిక m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    స్థిరరాశి తార్కిక m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    స్థిరరాశి Ast_కణుపు_Var_Ref* m_lhsVarRef = శూన్యసూచిక;
    పూర్ణాంకం m_ops = 0;  // Operation count

    // METHODS
    శూన్య clearSimple(స్థిరరాశి అక్షరం* because) {
        ఒకవేళ (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = అబద్ధం;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (!m_isSimple) తిరిగి;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        ఒకవేళ (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        ఒకవేళ (nodep->access().isRW()) {
            clearSimple("R/W");
            తిరిగి;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
        ఒకవేళ (nodep->access().isWriteOnly()) {
            ఒకవేళ (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } లేకపోతే {
            // TODO: possible bug, should it be >= 1 as add is below?
            ఒకవేళ (m_readVscps.కొలత() > 1) {
                ఒకవేళ (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.నెట్టు_వెనుక(vscp);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (!m_isSimple) తిరిగి;

        m_substitutionp = nodep->rhsp();
        ఒకవేళ (!VN_IS(nodep->lhsp(), కణుపు_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } లేకపోతే ఒకవేళ (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } లేకపోతే {
            iterateChildrenConst(nodep);
        }
        ఒకవేళ (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    శూన్య సందర్శించు(AstCReset* /*nodep*/) override {
        ఒకవేళ (!m_isSimple) తిరిగి;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    శూన్య సందర్శించు(Ast_సరిపోలిక_Masked* nodep) override {
        ఒకవేళ (!m_isSimple) తిరిగి;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (!m_isSimple) తిరిగి;  // Fastpath

        ఒకవేళ (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            తిరిగి;
        }

        ఒకవేళ (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->is_శుద్ధ() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            తిరిగి;
        }

        iterateChildrenConst(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    GateOkVisitor(Ast_కణుపు* nodep, తార్కిక buffersOnly, తార్కిక dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        ఒకవేళ (!m_substitutionp) {
            clearSimple("No assignment found\n");
            తిరిగి;
        }
        ఒకవేళ (m_isSimple && m_lhsVarRef) {
            కొరకు (స్థిరరాశి AstVarScope* స్థిరరాశి vscp : m_readVscps) {
                ఒకవేళ (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    తిరిగి;
                }
            }
        }
    }
    ~GateOkVisitor() override = అప్రమేయ;
    // PUBLIC METHODS
    తార్కిక isSimple() స్థిరరాశి { తిరిగి m_isSimple; }
    Ast_కణుపు_సమాసం* substitutionp() స్థిరరాశి {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        తిరిగి m_substitutionp;
    }
    స్థిరరాశి std::సదిశ<AstVarScope*>& readVscps() స్థిరరాశి { తిరిగి m_readVscps; }
    తార్కిక varAssigned(స్థిరరాశి AstVarScope* scopep) స్థిరరాశి {
        తిరిగి m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

తరగతి GateInline final {
    ఉపయోగించి Substitutions = std::unordered_map<AstVarScope*, Ast_కణుపు_సమాసం*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_కణుపు, Substitutions> m_substitutions;

    // STATE
    Gate_లేఖాచిత్రం& m_లేఖాచిత్రం;
    కొలత_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_కణుపు*, కొలత_t> m_hasPending;
    కొలత_t m_statInlined = 0;  // Statistic tracking - signals inlined
    కొలత_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    కొలత_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    స్థిర తార్కిక isCheap(స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        // Constant is cheap
        ఒకవేళ (VN_IS(exprp, Const)) తిరిగి నిజం;
        // Variable reference is cheap
        ఒకవేళ (VN_IS(exprp, కణుపు_Var_Ref)) తిరిగి నిజం;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        ఒకవేళ (స్థిరరాశి AstSel* స్థిరరాశి selp = VN_CAST(exprp, Sel)) {
            ఒకవేళ (!isCheap(selp->fromp())) తిరిగి అబద్ధం;
            ఒకవేళ (!selp->isWide()) తిరిగి నిజం;
            ఒకవేళ (!VN_IS(selp->lsbp(), Const)) తిరిగి అబద్ధం;
            ఒకవేళ (selp->lsbConst() % VL_EDATASIZE != 0) తిరిగి అబద్ధం;
            తిరిగి నిజం;
        }
        // AstArraySel is cheap if the fromp is cheap
        ఒకవేళ (స్థిరరాశి AstArraySel* స్థిరరాశి aselp = VN_CAST(exprp, ArraySel)) {
            తిరిగి isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        తిరిగి అబద్ధం;
    }

    తార్కిక shouldInline(Gate_Var_శీర్షం* vVtxp, Gate_Logic_శీర్షం* lVtxp, కొలత_t nReads,
                      Ast_కణుపు_సమాసం* substp, తార్కిక allow_Multi_లోపల) {
        // Always inline constants
        ఒకవేళ (VN_IS(substp, Const)) తిరిగి నిజం;
        // Don't inline non-constant static initializers - these are scheduled differently
        ఒకవేళ (lVtxp->staticInit()) తిరిగి అబద్ధం;
        // Inline simple variable references
        ఒకవేళ (VN_IS(substp, VarRef)) తిరిగి నిజం;
        // Only inline arrays if a simple variable or constant
        ఒకవేళ (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) తిరిగి అబద్ధం;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        ఒకవేళ (nReads == 0) తిరిగి నిజం;
        // If it reads one variable, inline if not wide, or if cheap
        ఒకవేళ (nReads == 1 && (!substp->isWide() || isCheap(substp))) తిరిగి నిజం;
        // Don't inline on first round if reads more than one variable
        ఒకవేళ (nReads > 1 && !allow_Multi_లోపల) తిరిగి అబద్ధం;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        పూర్ణాంకం n = 0;
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vVtxp->బహిర్గతం_అంచులు()) {
            స్థిరరాశి Gate_Logic_శీర్షం* స్థిరరాశి dstVtxp = అంచు.శిఖరం()->as<Gate_Logic_శీర్షం>();
            // Ignore slow code, or if the destination is not used
            ఒకవేళ (dstVtxp->slow()) కొనసాగు;
            ఒకవేళ (dstVtxp->బహిర్గతం_ఖాళీలు() && !dstVtxp->consumed()) కొనసాగు;
            n += అంచు.బరువు_2();
            ఒకవేళ (n > 1) {
                ++m_statNotInlined;
                తిరిగి అబద్ధం;
            }
        }
        తిరిగి నిజం;
    }

    శూన్య రికార్డు_Substitution(AstVarScope* vscp, Ast_కణుపు_సమాసం* substp, Ast_కణుపు* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        స్థిరరాశి స్వయంచాలక pair = m_substitutions(logicp).emplace(vscp, శూన్యసూచిక);
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ = substp->clone_వృక్షం_శుద్ధ(అబద్ధం);
    }

    శూన్య commitSubstitutions(Ast_కణుపు* logicp) {
        ఒకవేళ (!m_hasPending.erase(logicp)) తిరిగి;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.ఖాళీలు(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_సమితి<స్థిరరాశి AstVarScope*> replaced(substitutions.కొలత() * 2);

        స్థిరరాశి std::function<శూన్య(Ast_కణుపు_Var_Ref*)> సందర్శించు = [&](Ast_కణుపు_Var_Ref* nodep) -> శూన్య {
            // See if this variable has a substitution
            AstVarScope* స్థిరరాశి vscp = nodep->varScopep();
            స్థిరరాశి స్వయంచాలక& it = substitutions.కనుగొను(vscp);
            ఒకవేళ (it == substitutions.చివర()) తిరిగి;

            // Do not substitute circular logic
            ఒకవేళ (!replaced.చొప్పించు(vscp).రెండవ) తిరిగి;

            // Substitute nodep with substp
            Ast_కణుపు_సమాసం* స్థిరరాశి substp = it->రెండవ;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, కణుపు_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = substp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            ఒకవేళ (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(newp, కణుపు_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            // Recursively substitute the new tree
            newp->foreach(సందర్శించు);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(సందర్శించు);

        Ast_కణుపు* స్థిరరాశి simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : substitutions) pair.రెండవ->delete_వృక్షం();
        substitutions.clear();
    }

    శూన్య optimize_సంకేతాలు(తార్కిక allow_Multi_లోపల) {
        // Consider "inlining" variables
        స్వయంచాలక& vertices = m_లేఖాచిత్రం.vertices();
        స్థిరరాశి స్వయంచాలక ff_గమ్యం_Var_Vtx = [&](V3_లేఖాచిత్రం_శీర్షం::List::iterator it) {
            అయితే (it != vertices.చివర() && !(*it).is<Gate_Var_శీర్షం>()) ++it;
            తిరిగి it;
        };
        V3_లేఖాచిత్రం_శీర్షం::List::iterator vIt = ff_గమ్యం_Var_Vtx(vertices.begin());
        అయితే (vIt != vertices.చివర()) {
            Gate_Var_శీర్షం* స్థిరరాశి vVtxp = (*vIt).as<Gate_Var_శీర్షం>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ff_గమ్యం_Var_Vtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            ఒకవేళ (!vVtxp->లోపల_Size1()) కొనసాగు;

            // Can't inline if non-reducible, etc
            ఒకవేళ (!vVtxp->reducible()) కొనసాగు;

            // Grab the driving logic
            Gate_Logic_శీర్షం* స్థిరరాశి lVtxp
                = vVtxp->లోపల_అంచులు().frontp()->fromp()->as<Gate_Logic_శీర్షం>();
            ఒకవేళ (!lVtxp->reducible()) కొనసాగు;
            Ast_కణుపు* స్థిరరాశి logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            స్థిరరాశి GateOkVisitor okVisitor{logicp, అబద్ధం, అబద్ధం};

            // Was it ok?
            ఒకవేళ (!okVisitor.isSimple()) కొనసాగు;
            // If the varScope is already removed from logicp, no need to try substitution.
            ఒకవేళ (!okVisitor.varAssigned(vVtxp->varScp())) కొనసాగు;

            // Expression we are considering to substitute with
            Ast_కణుపు_సమాసం* స్థిరరాశి substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            స్థిరరాశి కొలత_t nReads = okVisitor.readVscps().కొలత();

            // Check if it should be inlined
            ఒకవేళ (!shouldInline(vVtxp, lVtxp, nReads, substp, allow_Multi_లోపల)) కొనసాగు;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* స్థిరరాశి vscp = vVtxp->varScp();

            ఒకవేళ (debug() >= 9) {
                vscp->dump_వృక్షం("substituting: ");
                substp->dump_వృక్షం("        with: ");
            }

            స్థిరరాశి స్వయంచాలక& readVscpsVec = okVisitor.readVscps();
            స్థిరరాశి std::unordered_సమితి<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.చివర()};

            కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : vVtxp->బహిర్గతం_అంచులు().unlinkable()) {
                Gate_Logic_శీర్షం* స్థిరరాశి dstVtxp = edgep->శిఖరం()->as<Gate_Logic_శీర్షం>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                ఒకవేళ (VN_IS(dstVtxp->nodep(), Sen_అంశం)) {
                    Ast_కణుపు* nodep = substp;
                    ఒకవేళ (AstNot* స్థిరరాశి notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    ఒకవేళ (!VN_IS(nodep, VarRef)) కొనసాగు;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                తార్కిక canInline = నిజం;
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& dedge : dstVtxp->బహిర్గతం_అంచులు()) {
                    స్థిరరాశి Gate_Var_శీర్షం* స్థిరరాశి consVVertexp = dedge.శిఖరం()->as<Gate_Var_శీర్షం>();
                    ఒకవేళ (readVscps.లెక్క(consVVertexp->varScp())) {
                        canInline = అబద్ధం;
                        ఆపు;
                    }
                }

                ఒకవేళ (!canInline) కొనసాగు;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                ఒకవేళ (logicp == dstVtxp->nodep()) {
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
                    కొనసాగు;
                }

                రికార్డు_Substitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                కొరకు (AstVarScope* స్థిరరాశి newVscp : okVisitor.readVscps()) {
                    Gate_Var_శీర్షం* స్థిరరాశి varvertexp = m_లేఖాచిత్రం.make_Var_శీర్షం(newVscp);
                    m_లేఖాచిత్రం.కూడు_అంచు(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            ఒకవేళ (vVtxp->బహిర్గతం_ఖాళీలు()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_లేఖాచిత్రం), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlink_Fr_వెనుక()->delete_వృక్షం(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_లేఖాచిత్రం), lVtxp);
            }
        }
    }

    స్పష్టమైన GateInline(Gate_లేఖాచిత్రం& లేఖాచిత్రం)
        : m_లేఖాచిత్రం{లేఖాచిత్రం} {
        // Find gate interconnect and optimize
        లేఖాచిత్రం.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimize_సంకేతాలు(అబద్ధం);
        // Then propagate more complicated equations
        optimize_సంకేతాలు(నిజం);
        // Commit substitutions in insertion order for stability
        ఉపయోగించి Pair = std::pair<Ast_కణుపు*, కొలత_t>;
        std::సదిశ<Pair> pending{m_hasPending.begin(), m_hasPending.చివర()};
        std::క్రమీకరించు(pending.begin(), pending.చివర(), [](స్థిరరాశి Pair& a, స్థిరరాశి Pair& b) {  //
            తిరిగి a.రెండవ < b.రెండవ;
        });
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : pending) commitSubstitutions(pair.మొదటి);
    }

    ~GateInline() {
        V3Stats::కూడు_Stat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::కూడు_Stat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::కూడు_Stat("Optimizations, Gate reads replaced", m_statRefs);
    }

బహిరంగ:
    స్థిర శూన్య apply(Gate_లేఖాచిత్రం& లేఖాచిత్రం) { GateInline{లేఖాచిత్రం}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

తరగతి GateDedupeHash final : బహిరంగ V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    నిర్మాణం Aux_Ast_కణుపు_సమాసం final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        Ast_చురుకైన* activep = శూన్యసూచిక;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;
        // Parent AstNodeAssign* for this rhsp
        Ast_కణుపు_కేటాయించు* parentp = శూన్యసూచిక;
    };
    AstUser2Allocator<Ast_కణుపు_సమాసం, Aux_Ast_కణుపు_సమాసం> m_aux_కణుపు_సమాసం;

    Ast_కణుపు_సమాసం* m_currRhsp = శూన్యసూచిక;  // Current node we are searching for duplicates of
    Aux_Ast_కణుపు_సమాసం m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    స్థిర తార్కిక same(Ast_కణుపు* node1p, Ast_కణుపు* node2p) {
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
        తిరిగి node1p == node2p || (node1p && node1p->same_వృక్షం(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    తార్కిక isSame(Ast_కణుపు* node1p, Ast_కణుపు* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        స్థిరరాశి స్వయంచాలక& aux2 = m_aux_కణుపు_సమాసం(VN_AS(node2p, కణుపు_సమాసం));
        తిరిగి m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

బహిరంగ:
    GateDedupeHash() = అప్రమేయ;
    ~GateDedupeHash() = అప్రమేయ;

    స్థిరరాశి Ast_కణుపు_కేటాయించు* hash_And_కనుగొను_Dupe(Ast_కణుపు_కేటాయించు* assignp, Ast_చురుకైన* activep,
                                         Ast_కణుపు_సమాసం* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        స్థిరరాశి స్వయంచాలక it = m_dupFinder.కనుగొను_Duplicate(m_currRhsp, ఇది);
        ఒకవేళ (it != m_dupFinder.చివర()) తిరిగి m_aux_కణుపు_సమాసం(VN_AS(it->రెండవ, కణుపు_సమాసం)).parentp;

        // Insert new node
        m_dupFinder.చొప్పించు(m_currRhsp);
        m_aux_కణుపు_సమాసం(m_currRhsp) = m_auxCurRhsp;
        తిరిగి శూన్యసూచిక;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

తరగతి GateDedupeVarVisitor final : బహిరంగ VNVisitorConst {
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
    Ast_కణుపు_కేటాయించు* m_assignp = శూన్యసూచిక;  // Assign found for dedupe
    Ast_కణుపు_సమాసం* m_ifCondp = శూన్యసూచిక;  // IF condition that assign is under
    తార్కిక m_always = అబద్ధం;  // Assign is under an always
    తార్కిక m_dedupable = నిజం;  // Determined the assign to be dedupable

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        ఒకవేళ (!m_dedupable) తిరిగి;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        ఒకవేళ (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_సమాసం_Stmt*) { తిరిగి నిజం; });
            తిరిగి;
        }

        m_dedupable = అబద్ధం;
    }

    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (!m_dedupable) తిరిగి;

        ఒకవేళ (!m_always) {
            m_always = నిజం;
            iterate_And_తదుపరి_Const_Null(nodep->stmtsp());
            తిరిగి;
        }

        m_dedupable = అబద్ధం;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        ఒకవేళ (!m_dedupable) తిరిగి;

        ఒకవేళ (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_సమాసం_Stmt*) { తిరిగి నిజం; });
            iterate_And_తదుపరి_Const_Null(nodep->thensp());
            తిరిగి;
        }

        m_dedupable = అబద్ధం;
    }

    శూన్య సందర్శించు(AstComment*) override {}  // NOP
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు*) override { m_dedupable = అబద్ధం; }

బహిరంగ:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = అప్రమేయ;
    ~GateDedupeVarVisitor() override = అప్రమేయ;
    // PUBLIC METHODS
    Ast_కణుపు_Var_Ref* కనుగొను_Dupe(Ast_కణుపు* logicp, AstVarScope* వినియోగదారు_Vscp, Ast_చురుకైన* activep) {
        m_assignp = శూన్యసూచిక;
        m_ifCondp = శూన్యసూచిక;
        m_always = అబద్ధం;
        m_dedupable = నిజం;
        iterateConst(logicp);
        ఒకవేళ (m_dedupable && m_assignp) {
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి lRefp = VN_CAST(lhsp, కణుపు_Var_Ref);
            ఒకవేళ (lRefp && lRefp->varScopep() == వినియోగదారు_Vscp) {
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_కేటాయించు* స్థిరరాశి dup
                    = m_ghash.hash_And_కనుగొను_Dupe(m_assignp, activep, m_ifCondp)) {
                    తిరిగి static_cast<Ast_కణుపు_Var_Ref*>(dup->lhsp());
                }
            }
        }
        తిరిగి శూన్యసూచిక;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

తరగతి GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;

    // STATE
    కొలత_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_లోతు = 0;  // Iteration depth

    శూన్య సందర్శించు(Gate_Var_శీర్షం* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        ఒకవేళ (m_లోతు > GATE_DEDUP_MAX_లోతు) తిరిగి;

        // Check that we haven't been here before
        ఒకవేళ (vVtxp->varScp()->user2_సమితి_Once()) తిరిగి;

        VL_RESTORER(m_లోతు);
        ++m_లోతు;
        ఒకవేళ (!vVtxp->లోపల_Size1()) తిరిగి;

        Ast_కణుపు_Var_Ref* dupRefp = శూన్యసూచిక;
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vVtxp->లోపల_అంచులు()) {
            dupRefp = సందర్శించు(అంచు.fromp()->as<Gate_Logic_శీర్షం>(), vVtxp);
        }
        ఒకవేళ (!dupRefp) తిరిగి;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        Gate_Logic_శీర్షం* స్థిరరాశి lVtxp = vVtxp->లోపల_అంచులు().frontp()->fromp()->as<Gate_Logic_శీర్షం>();
        స్థిరరాశి GateOkVisitor okVisitor{lVtxp->nodep(), అబద్ధం, నిజం};
        ఒకవేళ (!okVisitor.isSimple()) తిరిగి;

        ++m_statDedupLogic;
        Gate_Var_శీర్షం* స్థిరరాశి dupVVtxp = dupRefp->varScopep()->user1u().గమ్యం<Gate_Var_శీర్షం*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : vVtxp->బహిర్గతం_అంచులు().unlinkable()) {
            స్థిరరాశి Gate_Logic_శీర్షం* స్థిరరాశి వినియోగదారు_Vtxp = edgep->శిఖరం()->as<Gate_Logic_శీర్షం>();
            Ast_కణుపు* స్థిరరాశి consumerp = వినియోగదారు_Vtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << వినియోగదారు_Vtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            ఒకవేళ (lVtxp == వినియోగదారు_Vtxp) {
                UINFO(9, "skipping as self-recirculates");
            } లేకపోతే {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_కణుపు_Var_Ref* refp) {
                    ఒకవేళ (refp->varScopep() != vVtxp->varScp()) తిరిగి;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_కణుపు_Var_Ref* స్థిరరాశి newp = dupRefp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        అయితే (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep = vVtxp->లోపల_అంచులు().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_కణుపు_Var_Ref* సందర్శించు(Gate_Logic_శీర్షం* lVtxp, స్థిరరాశి Gate_Var_శీర్షం* వినియోగదారు_Vtxp) {
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : lVtxp->లోపల_అంచులు()) సందర్శించు(అంచు.fromp()->as<Gate_Var_శీర్షం>());

        ఒకవేళ (lVtxp->dedupable() && వినియోగదారు_Vtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            తిరిగి m_varVisitor.కనుగొను_Dupe(lVtxp->nodep(), వినియోగదారు_Vtxp->varScp(), lVtxp->activep());
        }
        తిరిగి శూన్యసూచిక;
    }

    స్పష్టమైన GateDedupe(Gate_లేఖాచిత్రం& లేఖాచిత్రం) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Gate_Var_శీర్షం* స్థిరరాశి vVtxp = vtx.cast<Gate_Var_శీర్షం>()) {
                ఒకవేళ (vVtxp->isClock()) సందర్శించు(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Gate_Var_శీర్షం* స్థిరరాశి vVtxp = vtx.cast<Gate_Var_శీర్షం>()) {
                ఒకవేళ (vVtxp->is_శిఖరం() && vVtxp->varScp()->varp()->isWritable()) సందర్శించు(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::కూడు_Stat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

బహిరంగ:
    స్థిర శూన్య apply(Gate_లేఖాచిత్రం& లేఖాచిత్రం) { GateDedupe{లేఖాచిత్రం}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

తరగతి Gate_విలీనం_Assignments final {
    Gate_లేఖాచిత్రం& m_లేఖాచిత్రం;
    కొలత_t m_stat_కేటాయించు_Merged = 0;  // Statistic tracking
    std::సదిశ<Gate_Logic_శీర్షం*> m_గమ్యం_Remove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* విలీనం(AstSel* prevSelp, AstSel* currSelp) {
        స్థిరరాశి AstVarRef* స్థిరరాశి pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* స్థిరరాశి cRefp = VN_CAST(currSelp->fromp(), VarRef);
        ఒకవేళ (!pRefp || !cRefp || !cRefp->same_కణుపు(pRefp)) తిరిగి శూన్యసూచిక;  // not the same var

        స్థిరరాశి AstConst* స్థిరరాశి pstart = VN_CAST(prevSelp->lsbp(), Const);
        స్థిరరాశి AstConst* స్థిరరాశి cstart = VN_CAST(currSelp->lsbp(), Const);
        ఒకవేళ (!pstart || !cstart) తిరిగి శూన్యసూచిక;  // too complicated

        ఒకవేళ (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            తిరిగి కొత్త AstSel{cRefp->fileline(), cRefp->clone_వృక్షం(అబద్ధం), currSelp->lsbConst(),
                              prevSelp->వెడల్పు_Const() + currSelp->వెడల్పు_Const()};
        } లేకపోతే {
            తిరిగి శూన్యసూచిక;
        }
    }

    శూన్య process(Gate_Var_శీర్షం* vVtxp) {
        Gate_Logic_శీర్షం* prevLVtxp = శూన్యసూచిక;
        Ast_కేటాయించు_W* prevAssignp = శూన్యసూచిక;

        కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : vVtxp->లోపల_అంచులు().unlinkable()) {
            Gate_Logic_శీర్షం* స్థిరరాశి lVtxp = edgep->fromp()->as<Gate_Logic_శీర్షం>();
            ఒకవేళ (!lVtxp->బహిర్గతం_Size1()) కొనసాగు;

            AstAlways* స్థిరరాశి alwaysp = VN_CAST(lVtxp->nodep(), Always);
            ఒకవేళ (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) తిరిగి;
            Ast_కేటాయించు_W* స్థిరరాశి assignp = VN_CAST(alwaysp->stmtsp(), కేటాయించు_W);
            ఒకవేళ (!assignp) కొనసాగు;

            ఒకవేళ (!VN_IS(assignp->lhsp(), Sel)) కొనసాగు;

            // First assign with Sel-lhs, or not under the same active
            ఒకవేళ (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                కొనసాగు;
            }

            AstSel* స్థిరరాశి prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* స్థిరరాశి currSelp = VN_AS(assignp->lhsp(), Sel);

            ఒకవేళ (AstSel* స్థిరరాశి newSelp = విలీనం(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_వృక్షం(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(కొత్త AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlink_Fr_వెనుక(),
                                                assignp->rhsp()->unlink_Fr_వెనుక()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtype_Chg_వెడల్పు_Signed(prevAssignp->వెడల్పు() + assignp->వెడల్పు(),
                                                 prevAssignp->వెడల్పు() + assignp->వెడల్పు(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_గమ్యం_Remove.emplace_వెనుక(lVtxp);

                // Update the graph
                అయితే (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి iedgep = lVtxp->లోపల_అంచులు().frontp()) {
                    Gate_Var_శీర్షం* స్థిరరాశి fromVtxp = iedgep->fromp()->as<Gate_Var_శీర్షం>();
                    m_లేఖాచిత్రం.కూడు_అంచు(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_stat_కేటాయించు_Merged;
            } లేకపోతే {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    స్పష్టమైన Gate_విలీనం_Assignments(Gate_లేఖాచిత్రం& లేఖాచిత్రం)
        : m_లేఖాచిత్రం{లేఖాచిత్రం} {
        UINFO(6, "mergeAssigns");
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : లేఖాచిత్రం.vertices()) {
            ఒకవేళ (Gate_Var_శీర్షం* స్థిరరాశి vVtxp = vtx.cast<Gate_Var_శీర్షం>()) process(vVtxp);
        }
        // Delete merged assignments
        కొరకు (Gate_Logic_శీర్షం* స్థిరరాశి lVtxp : m_గమ్యం_Remove) {
            Ast_కణుపు* స్థిరరాశి nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_లేఖాచిత్రం), lVtxp);
        }
    }

    ~Gate_విలీనం_Assignments() {
        V3Stats::కూడు_Stat("Optimizations, Gate assign merged", m_stat_కేటాయించు_Merged);
    }

బహిరంగ:
    స్థిర శూన్య apply(Gate_లేఖాచిత్రం& లేఖాచిత్రం) { Gate_విలీనం_Assignments{లేఖాచిత్రం}; }
};

//######################################################################
// GateUnused

తరగతి Gate_వాడని final {
    // STATE
    Gate_లేఖాచిత్రం& m_లేఖాచిత్రం;

    // METHODS

    శూన్య markRecurse(Gate_Either_శీర్షం* vtxp) {
        ఒకవేళ (vtxp->user()) తిరిగి;  // Already marked
        vtxp->user(నిజం);
        vtxp->సమితి_Consumed("propagated");
        // Walk sources and mark them too
        కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->లోపల_అంచులు()) {
            Gate_Either_శీర్షం* స్థిరరాశి fromVtxp = static_cast<Gate_Either_శీర్షం*>(అంచు.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    శూన్య mark() {
        m_లేఖాచిత్రం.userClearVertices();
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            Gate_Either_శీర్షం& eVtx = static_cast<Gate_Either_శీర్షం&>(vtx);
            ఒకవేళ (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    స్థిర శూన్య warn_వాడని(స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep) {
        ఒకవేళ (nodep->fileline()->warnIsOff(V3_Error_సంకేతం::UNUSEDLOOP)) తిరిగి;

        ఒకవేళ (స్థిరరాశి Ast_కణుపు_Procedure* స్థిరరాశి procedurep = VN_CAST(nodep, కణుపు_Procedure)) {
            ఒకవేళ (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](స్థిరరాశి AstLoop* స్థిరరాశి loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNUSEDLOOP, నిజం);
                });
        }
    }

    // Remove unused logic
    శూన్య remove() {
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp : m_లేఖాచిత్రం.vertices().unlinkable()) {
            ఒకవేళ (Gate_Logic_శీర్షం* స్థిరరాశి lVtxp = vtxp->cast<Gate_Logic_శీర్షం>()) {
                ఒకవేళ (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_కణుపు* స్థిరరాశి nodep = lVtxp->nodep();
                    warn_వాడని(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlink_Fr_వెనుక();
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                    VL_DO_DANGLING(lVtxp->unlinkDelete(&m_లేఖాచిత్రం), lVtxp);
                }
            }
        }
    }

    స్పష్టమైన Gate_వాడని(Gate_లేఖాచిత్రం& లేఖాచిత్రం)
        : m_లేఖాచిత్రం{లేఖాచిత్రం} {
        mark();  // Mark all used vertices
        remove();  // Remove unused vertices
    }

బహిరంగ:
    స్థిర శూన్య apply(Gate_లేఖాచిత్రం& లేఖాచిత్రం) { Gate_వాడని{లేఖాచిత్రం}; }
};

//######################################################################
// Pass entry point

శూన్య V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<Gate_లేఖాచిత్రం> graphp = GateBuildVisitor::apply(netlistp);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->remove_Redundant_అంచులు_కూడిక(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        Gate_వాడని::apply(*graphp);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        ఒకవేళ (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        ఒకవేళ (v3Global.opt.fAssemble()) {
            Gate_విలీనం_Assignments::apply(*graphp);
            ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        Gate_వాడని::apply(*graphp);
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_వృక్షం("gate", 0, dump_వృక్షం_Either_Level() >= 3);
}
