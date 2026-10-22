// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: NFA-based multi-cycle SVA assertion evaluation
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3AssertNfa's Transformations:
//
//  - Convert multi-cycle SVA sequences/properties into NFA graphs.
//  - Attach inherited assertion clocks before moving sampled-value functions.
//  - Emit module-level state registers driven by AstAlways blocks.
//  - Replace converted assertions with combinational match/reject checks
//    so V3AssertPre sees no multi-cycle SExpr (unsupported ones fall through).
//
//  Members marked OWNED hold an AST tree this pass allocated and must delete;
//  they are not linked into the netlist.
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3AssertNfa.h"

#అన్తర్భావ "V3Assert.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3Task.h"
#అన్తర్భావ "V3UniqueNames.h"

#అన్తర్భావ <set>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

నామాకాశ {

తరగతి Sva_State_శీర్షం;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
నిర్మాణం Sva_శీర్షం_దత్తాంశం final {
    AstVar* stateVarp = శూన్యసూచిక;  // NBA state register for this vertex
    AstVar* delayRingVarp = శూన్యసూచిక;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_తల_Varp
        = శూన్యసూచిక;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = శూన్యసూచిక;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_సజీవ_లెక్క_Varp = శూన్యసూచిక;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = శూన్యసూచిక;  // All slots written since the last clear
    AstVar* పూర్తి_LVarp = శూన్యసూచిక;  // SAnd LHS done-latch
    AstVar* పూర్తి_RVarp = శూన్యసూచిక;  // SAnd RHS done-latch
    Ast_కణుపు_సమాసం* stateSigp = శూన్యసూచిక;  // Combinational state signal; OWNED during lowering
    Ast_కణుపు_సమాసం* state_లెక్క_Sigp = శూన్యసూచిక;  // Cover-sequence match count; OWNED during lowering
    Ast_కణుపు_సమాసం* state_లెక్క_అధికప్రవాహం_Sigp = శూన్యసూచిక;  // Count overflow; OWNED during lowering
    తార్కిక delayRingCounts = అబద్ధం;  // Ring slots hold cover-sequence match counts
    తార్కిక needsReg = అబద్ధం;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
తరగతి Sva_State_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Sva_State_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
బహిరంగ:
    // True if this is the sequence-match terminal vertex
    తార్కిక m_is_సరిపోలిక = అబద్ధం;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::సదిశ<Ast_కణుపు_సమాసం*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    తార్కిక m_isFixedDelayRing = అబద్ధం;
    గుర్తులేని m_delay_Ring_కొలత = 0;  // Number of ring slots. Range: max-min+1.
    Ast_కణుపు_సమాసం* m_delayRingClearCondp = శూన్యసూచిక;  // local RHS for pure-boolean range
    Ast_కణుపు_సమాసం* m_delayRingAdvanceCondp = శూన్యసూచిక;  // Advance only when this condition holds
    తార్కిక m_replayAbortReject = అబద్ధం;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_కణుపు_సమాసం* m_abortClearp = శూన్యసూచిక;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_కణుపు_సమాసం* m_abortRejectp = శూన్యసూచిక;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    తార్కిక m_isUnbounded = అబద్ధం;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    తార్కిక m_isAndCombiner = అబద్ధం;
    Sva_State_శీర్షం* m_andLhsTermp = శూన్యసూచిక;  // LHS sub-NFA terminal vertex
    Sva_State_శీర్షం* m_andRhsTermp = శూన్యసూచిక;  // RHS sub-NFA terminal vertex
    Ast_కణుపు_సమాసం* m_andLhsCondp = శూన్యసూచిక;  // OWNED; LHS final condition (may be nullptr)
    Ast_కణుపు_సమాసం* m_andRhsCondp = శూన్యసూచిక;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    తార్కిక m_isRejectSink = అబద్ధం;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    తార్కిక m_strongPending = అబద్ధం;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    తార్కిక m_strongAlwaysRing = అబద్ధం;

    // CONSTRUCTORS
    స్పష్టమైన Sva_State_శీర్షం(V3_లేఖాచిత్రం* graphp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp} {}
    ~Sva_State_శీర్షం() override {
        కొరకు (Ast_కణుపు_సమాసం* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_వృక్షం(), cp);
        ఒకవేళ (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_వృక్షం(), m_delayRingClearCondp);
        ఒకవేళ (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_వృక్షం(), m_delayRingAdvanceCondp);
        ఒకవేళ (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_వృక్షం(), m_abortClearp);
        ఒకవేళ (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_వృక్షం(), m_abortRejectp);
        ఒకవేళ (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_వృక్షం(), m_andLhsCondp);
        ఒకవేళ (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_వృక్షం(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() స్థిరరాశి override {
        string name = "s" + cvt_గమ్యం_Str(color());
        ఒకవేళ (m_delay_Ring_కొలత) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_గమ్యం_Str(m_delay_Ring_కొలత) + " slots";
        }
        తిరిగి name;
    }
    string dotColor() స్థిరరాశి override {
        ఒకవేళ (m_is_సరిపోలిక) తిరిగి "red";
        ఒకవేళ (m_delay_Ring_కొలత) తిరిగి "blue";
        ఒకవేళ (m_isAndCombiner) తిరిగి "purple";
        తిరిగి "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_శీర్షం_దత్తాంశం* datap() స్థిరరాశి { తిరిగి static_cast<Sva_శీర్షం_దత్తాంశం*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
తరగతి Sva_Trans_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    VL_RTTI_IMPL(Sva_Trans_అంచు, V3_లేఖాచిత్రం_అంచు)
బహిరంగ:
    Ast_కణుపు_సమాసం* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    తార్కిక m_consumes_చక్రం;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    తార్కిక m_rejectOnFail = అబద్ధం;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    Sva_State_శీర్షం* m_condVtxp = శూన్యసూచిక;

    // CONSTRUCTORS
    Sva_Trans_అంచు(V3_లేఖాచిత్రం* graphp, V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, Ast_కణుపు_సమాసం* condp,
                 తార్కిక consumes_చక్రం)
        : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_చక్రం{consumes_చక్రం} {}
    ~Sva_Trans_అంచు() override {
        ఒకవేళ (m_condp) VL_DO_DANGLING(m_condp->delete_వృక్షం(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() స్థిరరాశి override { తిరిగి m_consumes_చక్రం ? "##1" : "link"; }
    string dotStyle() స్థిరరాశి override { తిరిగి m_consumes_చక్రం ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    Sva_State_శీర్షం* fromVtxp() స్థిరరాశి { తిరిగి static_cast<Sva_State_శీర్షం*>(fromp()); }
    Sva_State_శీర్షం* గమ్యం_Vtxp() స్థిరరాశి { తిరిగి static_cast<Sva_State_శీర్షం*>(శిఖరం()); }
};

// NFA graph container
తరగతి Sva_లేఖాచిత్రం final {
బహిరంగ:
    V3_లేఖాచిత్రం m_లేఖాచిత్రం;  // Owns all vertices and edges
    Sva_State_శీర్షం* m_ప్రారంభం_Vertexp = శూన్యసూచిక;  // Trigger/start vertex
    Sva_State_శీర్షం* m_సరిపోలిక_Vertexp = శూన్యసూచిక;  // Sequence-match terminal vertex

    // Create a new state vertex
    Sva_State_శీర్షం* create_State_శీర్షం() { తిరిగి కొత్త Sva_State_శీర్షం{&m_లేఖాచిత్రం}; }
    // Create the match terminal vertex
    Sva_State_శీర్షం* create_సరిపోలిక_శీర్షం() {
        Sva_State_శీర్షం* స్థిరరాశి vtxp = create_State_శీర్షం();
        vtxp->m_is_సరిపోలిక = నిజం;
        m_సరిపోలిక_Vertexp = vtxp;
        తిరిగి vtxp;
    }
    // Add a clocked transition edge (##1)
    Sva_Trans_అంచు* కూడు_Clocked_అంచు(Sva_State_శీర్షం* fromp, Sva_State_శీర్షం* శిఖరం,
                                 Ast_కణుపు_సమాసం* condp = శూన్యసూచిక) {
        తిరిగి కొత్త Sva_Trans_అంచు{&m_లేఖాచిత్రం, fromp, శిఖరం, condp, /*consumesCycle=*/నిజం};
    }
    // Add a combinational link (##0 / boolean condition)
    Sva_Trans_అంచు* కూడు_Link(Sva_State_శీర్షం* fromp, Sva_State_శీర్షం* శిఖరం,
                          Ast_కణుపు_సమాసం* condp = శూన్యసూచిక) {
        తిరిగి కొత్త Sva_Trans_అంచు{&m_లేఖాచిత్రం, fromp, శిఖరం, condp, /*consumesCycle=*/అబద్ధం};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::సదిశ<స్థిరరాశి Sva_Trans_అంచు*> all_అంచులు() స్థిరరాశి {
        std::సదిశ<స్థిరరాశి Sva_Trans_అంచు*> ఫలితం;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtxr : m_లేఖాచిత్రం.vertices()) {
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& edger : vtxr.బహిర్గతం_అంచులు()) {
                ఫలితం.నెట్టు_వెనుక(static_cast<స్థిరరాశి Sva_Trans_అంచు*>(&edger));
            }
        }
        తిరిగి ఫలితం;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
నిర్మాణం Build_ఫలితం final {
    Sva_State_శీర్షం* పదం_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_కణుపు_సమాసం* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::సదిశ<Sva_State_శీర్షం*> midSources;
    తార్కిక errorEmitted = అబద్ధం;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    తార్కిక పదం_Is_Mid_విలీనం = అబద్ధం;
    తార్కిక valid() స్థిరరాశి { తిరిగి పదం_Vertexp != శూన్యసూచిక; }
    స్థిర Build_ఫలితం fail(తార్కిక errored = అబద్ధం) { తిరిగి {శూన్యసూచిక, శూన్యసూచిక, {}, errored}; }
    స్థిర Build_ఫలితం failWithError() { తిరిగి {శూన్యసూచిక, శూన్యసూచిక, {}, నిజం}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
స్థిర తార్కిక hasPropertyControlConjunction(స్థిరరాశి Ast_కణుపు_సమాసం* nodep) {
    తిరిగి nodep->exists([](స్థిరరాశి AstSAnd* andp) { తిరిగి andp->propertyControl(); });
}

స్థిర AstConst* newTypedConstp(File_పంక్తి* స్థిరరాశి flp, స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep,
                                స్థిరరాశి uint32_t విలువ) {
    AstConst* స్థిరరాశి constp = కొత్త AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->అంకె().సమితి_Long(విలువ);
    తిరిగి constp;
}

స్థిర Ast_కణుపు_సమాసం* sampled(Ast_కణుపు_సమాసం* exprp) {
    తిరిగి కొత్త AstSampled{exprp->fileline(), exprp, exprp->dtypep(), నిజం};
}

స్థిర స్థిరరాశి అక్షరం* assertPassOnQuery(తార్కిక vacuous) {
    స్థిర constexpr స్థిరరాశి అక్షరం* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    తిరిగి queries[vacuous];
}

స్థిర Ast_కణుపు_సమాసం* assertOnCond(File_పంక్తి* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    ఒకవేళ (!v3Global.opt.assertOn()) { తిరిగి కొత్త AstConst{flp, AstConst::బిట్టు_False{}}; }
    తిరిగి కొత్త AstCExpr{flp, AstCExpr::శుద్ధ{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

స్థిర Ast_కణుపు_సమాసం* assertKillGet(File_పంక్తి* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    తిరిగి కొత్త AstCExpr{flp, AstCExpr::శుద్ధ{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

స్థిర string assert_Action_Control_ఉపసర్గ(VAssertDirectiveType directiveType) {
    స్థిరరాశి పూర్ణాంకం controlled = !!(static_cast<పూర్ణాంకం>(directiveType)
                              & (static_cast<పూర్ణాంకం>(VAssertDirectiveType::ASSERT)
                                 | static_cast<పూర్ణాంకం>(VAssertDirectiveType::COVER)
                                 | static_cast<పూర్ణాంకం>(VAssertDirectiveType::ASSUME)));
    స్థిరరాశి పూర్ణాంకం checkRuntime = controlled & static_cast<పూర్ణాంకం>(v3Global.opt.assertOn());
    తిరిగి "("s + std::గమ్యం_string(controlled ^ 1) + " || ("s + std::గమ్యం_string(checkRuntime)
           + " && "s;
}

స్థిర Ast_కణుపు_సమాసం* assertPassOnCond(File_పంక్తి* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, తార్కిక vacuous) {
    తిరిగి కొత్త AstCExpr{
        flp, AstCExpr::శుద్ధ{},
        assert_Action_Control_ఉపసర్గ(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

స్థిర Ast_కణుపు_సమాసం* assertFailOnCond(File_పంక్తి* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    తిరిగి కొత్త AstCExpr{
        flp, AstCExpr::శుద్ధ{},
        assert_Action_Control_ఉపసర్గ(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

స్థిర AstIf* newPassOnIf(File_పంక్తి* flp, Ast_కణుపు_సమాసం* firep, Ast_కణుపు* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, తార్కిక vacuous) {
    Ast_కణుపు_సమాసం* స్థిరరాశి condp
        = కొత్త AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(నిజం);
    ifp->user1(నిజం);
    తిరిగి ifp;
}

స్థిర Ast_కణుపు_Stmt* newIfAssertFailOn(Ast_కణుపు* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_పంక్తి* స్థిరరాశి flp = bodyp->fileline();
    Ast_కణుపు_సమాసం* స్థిరరాశి condp = assertFailOnCond(flp, type, directiveType);
    AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(నిజం);
    ifp->user1(నిజం);
    తిరిగి ifp;
}

//######################################################################
// NFA Builder

తరగతి SvaNfaBuilder final {
    Sva_లేఖాచిత్రం& m_లేఖాచిత్రం;  // NFA graph being built
    Ast_కణుపు_Module* స్థిరరాశి m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_పేర్లు& m_prop_తాత్కాలిక_పేర్లు;  // Module-shared temp-var name source
    std::సదిశ<Ast_కణుపు_సమాసం*> m_temporal_Guard_దొంతర;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::సదిశ<Ast_కణుపు_సమాసం*> m_outer_Abort_దొంతర;
    తార్కిక m_లోపల_Unbounded_Scope = అబద్ధం;  // Sticky: nodes created after inherit liveness
    తార్కిక m_markStrongPending = అబద్ధం;  // Mark new vertices as strong s_always in-window
    తార్కిక m_isCover = అబద్ధం;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    తార్కిక m_isCoverSeq = అబద్ధం;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    తార్కిక m_isSeqEvent = అబద్ధం;
    తార్కిక m_లోపల_Sequence_ఉపసర్గ = అబద్ధం;  // Prefix composition does not preserve every endpoint
    తార్కిక m_want_Bool_ఆకు = అబద్ధం;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    నిర్మాణం RangeDelayRejectInfo final {
        Sva_State_శీర్షం* startp = శూన్యసూచిక;
        గుర్తులేని range = 0;
        పూర్ణాంకం rhsLen = 0;
    };

    శూన్య warnEndpointUnsupported(File_పంక్తి* flp, స్థిరరాశి string& what) స్థిరరాశి {
        ఒకవేళ (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } లేకపోతే {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_కణుపు_సమాసం* throughoutCond(Ast_కణుపు_సమాసం* పునాది_Condp, File_పంక్తి* flp) {
        ఒకవేళ (m_temporal_Guard_దొంతర.ఖాళీలు()) తిరిగి పునాది_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_కణుపు_సమాసం* guardp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి condp : m_temporal_Guard_దొంతర) {
            Ast_కణుపు_సమాసం* స్థిరరాశి clonep = sampled(condp->clone_వృక్షం_శుద్ధ(అబద్ధం));
            ఒకవేళ (!guardp) {
                guardp = clonep;
            } లేకపోతే {
                guardp = కొత్త AstLogAnd{flp, guardp, clonep};
            }
        }
        ఒకవేళ (పునాది_Condp) { guardp = కొత్త AstLogAnd{flp, పునాది_Condp, guardp}; }
        తిరిగి guardp;
    }

    స్థిర గుర్తులేని getConstUInt(Ast_కణుపు_సమాసం* exprp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి constp = V3Const::constifyEdit(exprp->clone_వృక్షం_శుద్ధ(అబద్ధం));
        స్థిరరాశి AstConst* స్థిరరాశి cp = VN_CAST(constp, Const);
        స్థిరరాశి గుర్తులేని val = cp ? cp->గమ్యం_UInt() : 0;
        VL_DO_DANGLING(constp->delete_వృక్షం(), constp);
        తిరిగి val;
    }

    // Static fixed-length analysis: clock ticks from entry to terminal, or -1.
    // Used by SIntersect to verify IEEE 1800-2023 16.9.6 equal-length precondition.
    //
    // Supported:
    //  - Boolean leaf (default)              -> 0
    //  - AstSExpr with fixed cycle delay     -> pre + N + body
    //  - AstSExpr with range delay M==N      -> pre + N + body
    //  - AstSThroughout                      -> length of rhsp (the seq)
    // Unsupported (returns -1):
    //  - Range delays with M != N            -> variable
    //  - Unbounded waits ##[M:$]             -> infinite/variable
    //  - ConsRep / SGotoRep / SAnd / SOr     -> defer (rare in intersect)
    //  - SIntersect nested in SIntersect     -> defer
    స్థిర పూర్ణాంకం fixed_నిడివి(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (AstSExpr* స్థిరరాశి sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* స్థిరరాశి delayp = VN_CAST(sexprp->delayp(), Delay);
            ఒకవేళ (!delayp || !delayp->is_చక్రం_Delay()) తిరిగి -1;
            గుర్తులేని delay_చక్రాలు;
            ఒకవేళ (delayp->isRangeDelay()) {
                ఒకవేళ (delayp->isUnbounded()) తిరిగి -1;  // LCOV_EXCL_LINE
                స్థిరరాశి గుర్తులేని minD = getConstUInt(delayp->lhsp());
                స్థిరరాశి గుర్తులేని maxD = getConstUInt(delayp->rhsp());
                ఒకవేళ (minD != maxD) తిరిగి -1;
                delay_చక్రాలు = minD;
            } లేకపోతే {
                delay_చక్రాలు = getConstUInt(delayp->lhsp());
            }
            పూర్ణాంకం preLen = 0;
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి prep = sexprp->preExprp()) {
                preLen = fixed_నిడివి(prep);
                ఒకవేళ (preLen < 0) తిరిగి -1;  // LCOV_EXCL_LINE
            }
            స్థిరరాశి పూర్ణాంకం bodyLen = fixed_నిడివి(sexprp->exprp());
            ఒకవేళ (bodyLen < 0) తిరిగి -1;  // LCOV_EXCL_LINE
            తిరిగి preLen + delay_చక్రాలు + bodyLen;
        }
        ఒకవేళ (AstSThroughout* స్థిరరాశి throughp = VN_CAST(nodep, SThroughout)) {
            తిరిగి fixed_నిడివి(throughp->rhsp());
        }
        ఒకవేళ (AstSOr* స్థిరరాశి orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            స్థిరరాశి పూర్ణాంకం lhsLen = fixed_నిడివి(orp->lhsp());
            స్థిరరాశి పూర్ణాంకం rhsLen = fixed_నిడివి(orp->rhsp());
            ఒకవేళ (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) తిరిగి -1;
            తిరిగి lhsLen;
        }
        ఒకవేళ (AstSWithin* స్థిరరాశి withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            స్థిరరాశి పూర్ణాంకం lhsLen = fixed_నిడివి(withinp->lhsp());
            స్థిరరాశి పూర్ణాంకం rhsLen = fixed_నిడివి(withinp->rhsp());
            ఒకవేళ (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) తిరిగి -1;
            తిరిగి rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        ఒకవేళ (nodep->is_Multi_చక్రం_Sva()) తిరిగి -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        తిరిగి 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    స్థిర std::pair<పూర్ణాంకం, పూర్ణాంకం> నిడివి_Range(Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (AstSExpr* స్థిరరాశి sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* స్థిరరాశి delayp = VN_CAST(sexprp->delayp(), Delay);
            ఒకవేళ (!delayp || !delayp->is_చక్రం_Delay()) తిరిగి {-1, -1};
            std::pair<పూర్ణాంకం, పూర్ణాంకం> delayRange;
            ఒకవేళ (delayp->isRangeDelay()) {
                ఒకవేళ (delayp->isUnbounded()) తిరిగి {-1, -1};
                స్థిరరాశి గుర్తులేని minD = getConstUInt(delayp->lhsp());
                స్థిరరాశి గుర్తులేని maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } లేకపోతే {
                స్థిరరాశి గుర్తులేని d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<పూర్ణాంకం, పూర్ణాంకం> preRange{0, 0};
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి prep = sexprp->preExprp()) {
                preRange = నిడివి_Range(prep);
                ఒకవేళ (preRange.మొదటి < 0) తిరిగి {-1, -1};
            }
            స్థిరరాశి std::pair<పూర్ణాంకం, పూర్ణాంకం> bodyRange = నిడివి_Range(sexprp->exprp());
            ఒకవేళ (bodyRange.మొదటి < 0) తిరిగి {-1, -1};
            స్థిరరాశి పూర్ణాంకం variableParts = (preRange.మొదటి != preRange.రెండవ)
                                      + (delayRange.మొదటి != delayRange.రెండవ)
                                      + (bodyRange.మొదటి != bodyRange.రెండవ);
            ఒకవేళ (variableParts > 1) తిరిగి {-1, -1};
            తిరిగి {preRange.మొదటి + delayRange.మొదటి + bodyRange.మొదటి,
                    preRange.రెండవ + delayRange.రెండవ + bodyRange.రెండవ};
        }
        ఒకవేళ (AstSThroughout* స్థిరరాశి throughp = VN_CAST(nodep, SThroughout)) {
            తిరిగి నిడివి_Range(throughp->rhsp());
        }
        ఒకవేళ (nodep->is_Multi_చక్రం_Sva()) తిరిగి {-1, -1};
        తిరిగి {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    స్థిర Ast_కణుపు_సమాసం* realize_At_నిడివి(Ast_కణుపు_సమాసం* నిర్వాహ్యం, పూర్ణాంకం len, పూర్ణాంకం lo) {
        Ast_కణుపు_సమాసం* స్థిరరాశి clonep = నిర్వాహ్యం->clone_వృక్షం_శుద్ధ(అబద్ధం);
        AstDelay* rangeDelayp = శూన్యసూచిక;
        clonep->foreach([&](AstDelay* dp) {
            ఒకవేళ (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        ఒకవేళ (rangeDelayp) {
            File_పంక్తి* స్థిరరాశి flp = rangeDelayp->fileline();
            స్థిరరాశి గుర్తులేని pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_కణుపు_సమాసం* స్థిరరాశి oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(కొత్త AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_వృక్షం(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_కణుపు* స్థిరరాశి oldMaxp = rangeDelayp->rhsp()->unlink_Fr_వెనుక();
            VL_DO_DANGLING(oldMaxp->delete_వృక్షం(), oldMaxp);
        }
        తిరిగి clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_కణుపు_సమాసం* exprp, File_పంక్తి* flp, గుర్తులేని clone_లెక్క) {
        constexpr గుర్తులేని k_Hoist_పరిమితి = 2;
        ఒకవేళ (clone_లెక్క < k_Hoist_పరిమితి) తిరిగి శూన్యసూచిక;
        AstVar* స్థిరరాశి తాత్కాలిక_Varp
            = కొత్త AstVar{flp, VVarType::MODULETEMP, m_prop_తాత్కాలిక_పేర్లు.get(exprp), exprp->dtypep()};
        m_modp->కూడు_Stmtsp(తాత్కాలిక_Varp);
        Ast_కేటాయించు* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, తాత్కాలిక_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_వృక్షం_శుద్ధ(అబద్ధం))};
        m_modp->కూడు_Stmtsp(కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, శూన్యసూచిక, assignp});
        తిరిగి తాత్కాలిక_Varp;
    }

    స్థిర Ast_కణుపు_సమాసం* sampledRefOrClone(AstVar* hoistVarp, Ast_కణుపు_సమాసం* exprp, File_పంక్తి* flp) {
        ఒకవేళ (hoistVarp) తిరిగి కొత్త AstVarRef{flp, hoistVarp, VAccess::READ};
        తిరిగి sampled(exprp->clone_వృక్షం_శుద్ధ(అబద్ధం));
    }

    // Create vertex and inherit temporal guards from the current scope.
    Sva_State_శీర్షం* scoped_Create_శీర్షం() {
        Sva_State_శీర్షం* స్థిరరాశి vtxp = m_లేఖాచిత్రం.create_State_శీర్షం();
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి cp : m_temporal_Guard_దొంతర) {
            vtxp->m_throughoutConds.నెట్టు_వెనుక(cp->clone_వృక్షం_శుద్ధ(అబద్ధం));
        }
        ఒకవేళ (m_లోపల_Unbounded_Scope) vtxp->m_isUnbounded = నిజం;
        ఒకవేళ (m_markStrongPending) vtxp->m_strongPending = నిజం;
        తిరిగి vtxp;
    }

    // AND current temporal guards into every edge/link.
    Sva_Trans_అంచు* guardedLink(Sva_State_శీర్షం* fromp, Sva_State_శీర్షం* శిఖరం, Ast_కణుపు_సమాసం* condp,
                              File_పంక్తి* flp) {
        తిరిగి m_లేఖాచిత్రం.కూడు_Link(fromp, శిఖరం, throughoutCond(condp, flp));
    }
    Sva_Trans_అంచు* guardedLink(Sva_State_శీర్షం* fromp, Sva_State_శీర్షం* శిఖరం, File_పంక్తి* flp) {
        తిరిగి m_లేఖాచిత్రం.కూడు_Link(fromp, శిఖరం, throughoutCond(శూన్యసూచిక, flp));
    }
    Sva_Trans_అంచు* guarded_అంచు(Sva_State_శీర్షం* fromp, Sva_State_శీర్షం* శిఖరం, Ast_కణుపు_సమాసం* condp,
                              File_పంక్తి* flp) {
        తిరిగి m_లేఖాచిత్రం.కూడు_Clocked_అంచు(fromp, శిఖరం, throughoutCond(condp, flp));
    }
    Sva_Trans_అంచు* guarded_అంచు(Sva_State_శీర్షం* fromp, Sva_State_శీర్షం* శిఖరం, File_పంక్తి* flp) {
        తిరిగి m_లేఖాచిత్రం.కూడు_Clocked_అంచు(fromp, శిఖరం, throughoutCond(శూన్యసూచిక, flp));
    }

    Sva_State_శీర్షం* కూడు_Delay_Chain(Sva_State_శీర్షం* startp, గుర్తులేని కొలత, File_పంక్తి* flp,
                                  తార్కిక isFixed = నిజం, Ast_కణుపు_సమాసం* clearCondp = శూన్యసూచిక,
                                  Ast_కణుపు_సమాసం* advanceCondp = శూన్యసూచిక) {
        ఒకవేళ (isFixed && కొలత == 0) తిరిగి startp;
        UASSERT_OBJ(కొలత > 0, startp, "Delay chain needs at least one slot");
        ఒకవేళ (isFixed && కొలత == 1 && !advanceCondp) {
            Sva_State_శీర్షం* స్థిరరాశి nextp = scoped_Create_శీర్షం();
            guarded_అంచు(startp, nextp, flp);
            తిరిగి nextp;
        }
        Sva_State_శీర్షం* స్థిరరాశి ringVtxp = scoped_Create_శీర్షం();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_కొలత = కొలత;
        ఒకవేళ (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        ఒకవేళ (isFixed) {
            guarded_అంచు(startp, ringVtxp, flp);
        } లేకపోతే {
            guardedLink(startp, ringVtxp, flp);
        }
        తిరిగి ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    తార్కిక applyRangeDelay(AstSExpr* sexprp, Sva_State_శీర్షం*& currentp,
                         std::సదిశ<Sva_State_శీర్షం*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = శూన్యసూచిక) {
        File_పంక్తి* స్థిరరాశి flp = sexprp->fileline();
        AstDelay* స్థిరరాశి delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsExprp = sexprp->exprp();
        స్థిరరాశి గుర్తులేని minDelay = getConstUInt(delayp->lhsp());
        ఒకవేళ (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = కూడు_Delay_Chain(currentp, minDelay, flp);
            Sva_State_శీర్షం* స్థిరరాశి waitRingp = కూడు_Delay_Chain(currentp, 1, flp, అబద్ధం);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            Sva_State_శీర్షం* స్థిరరాశి విలీనం_Vtxp = scoped_Create_శీర్షం();
            guardedLink(currentp, విలీనం_Vtxp, flp);
            guardedLink(waitRingp, విలీనం_Vtxp, flp);
            currentp = విలీనం_Vtxp;
            currentp->m_isUnbounded = నిజం;  // Liveness, so no reject.
            m_లోపల_Unbounded_Scope = నిజం;
            తిరిగి నిజం;
        }
        స్థిరరాశి గుర్తులేని maxDelay = getConstUInt(delayp->rhsp());
        ఒకవేళ (minDelay == maxDelay) {
            currentp = కూడు_Delay_Chain(currentp, minDelay, flp);
            తిరిగి నిజం;
        }
        స్థిరరాశి గుర్తులేని range = maxDelay - minDelay;
        currentp = కూడు_Delay_Chain(currentp, minDelay, flp);
        స్థిరరాశి తార్కిక multi_చక్రం_Rhs = rhsExprp->is_Multi_చక్రం_Sva();
        స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        ఒకవేళ (m_isCoverSeq
            && (multi_చక్రం_Rhs || m_లోపల_Sequence_ఉపసర్గ
                || (preExprp && preExprp->is_Multi_చక్రం_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (m_లోపల_Sequence_ఉపసర్గ) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (multi_చక్రం_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            ఒకవేళ (rangeRejectInfop) {
                స్థిరరాశి పూర్ణాంకం rhsLen = fixed_నిడివి(rhsExprp);
                ఒకవేళ (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            Sva_State_శీర్షం* స్థిరరాశి tailp = కూడు_Delay_Chain(currentp, range + 1U, flp, అబద్ధం);
            Sva_State_శీర్షం* స్థిరరాశి విలీనం_Vtxp = scoped_Create_శీర్షం();
            విలీనం_Vtxp->m_isUnbounded = నిజం;
            guardedLink(currentp, విలీనం_Vtxp, flp);
            guardedLink(tailp, విలీనం_Vtxp, flp);
            currentp = విలీనం_Vtxp;
            m_లోపల_Unbounded_Scope = నిజం;
        } లేకపోతే {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.నెట్టు_వెనుక(currentp);
            currentp = కూడు_Delay_Chain(currentp, range + 1U, flp, అబద్ధం,
                                     m_isCoverSeq ? శూన్యసూచిక : rhsExprp);
        }
        తిరిగి నిజం;
    }

    శూన్య కూడు_Finite_Range_Reject(స్థిరరాశి RangeDelayRejectInfo& info, స్థిరరాశి Build_ఫలితం& ఫలితం,
                              File_పంక్తి* flp) {
        ఒకవేళ (!info.startp) తిరిగి;

        Sva_State_శీర్షం* స్థిరరాశి expiryVtxp
            = కూడు_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        Sva_State_శీర్షం* స్థిరరాశి expiryMatchp = scoped_Create_శీర్షం();
        std::సదిశ<Sva_State_శీర్షం*> sources = ఫలితం.midSources;
        sources.నెట్టు_వెనుక(ఫలితం.పదం_Vertexp);
        కొరకు (Sva_State_శీర్షం* స్థిరరాశి srcp : sources) {
            Ast_కణుపు_సమాసం* స్థిరరాశి condp
                = ఫలితం.finalCondp ? sampled(ఫలితం.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)) : శూన్యసూచిక;
            Sva_State_శీర్షం* స్థిరరాశి successNowp = scoped_Create_శీర్షం();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            Sva_State_శీర్షం* స్థిరరాశి historyp
                = కూడు_Delay_Chain(successNowp, info.range + 1U, flp, అబద్ధం);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        Sva_State_శీర్షం* స్థిరరాశి sinkVtxp = m_లేఖాచిత్రం.create_State_శీర్షం();
        sinkVtxp->m_isRejectSink = నిజం;
        Sva_Trans_అంచు* స్థిరరాశి rejectp = m_లేఖాచిత్రం.కూడు_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = నిజం;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_ఫలితం buildSExpr(AstSExpr* sexprp, Sva_State_శీర్షం* నమోదు_Vtxp,
                           తార్కిక is_శిఖరం_Level_అడుగు = అబద్ధం) {
        AstDelay* స్థిరరాశి delayp = VN_CAST(sexprp->delayp(), Delay);
        ఒకవేళ (!delayp || !delayp->is_చక్రం_Delay()) తిరిగి Build_ఫలితం::fail();

        File_పంక్తి* స్థిరరాశి flp = sexprp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        Sva_State_శీర్షం* currentp = నమోదు_Vtxp;
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_లోపల_Sequence_ఉపసర్గ);
            m_లోపల_Sequence_ఉపసర్గ = నిజం;
            స్థిరరాశి Build_ఫలితం pre = build_సమాసం(preExprp, currentp, is_శిఖరం_Level_అడుగు);
            ఒకవేళ (!pre.valid()) తిరిగి Build_ఫలితం::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            ఒకవేళ (pre.finalCondp) {
                Sva_State_శీర్షం* స్థిరరాశి condVtxp = scoped_Create_శీర్షం();
                Sva_Trans_అంచు* స్థిరరాశి edgep = guardedLink(
                    pre.పదం_Vertexp, condVtxp, sampled(pre.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)), flp);
                ఒకవేళ (is_శిఖరం_Level_అడుగు && !pre.పదం_Vertexp->m_isUnbounded && !m_లోపల_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = నిజం;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } లేకపోతే {
                currentp = pre.పదం_Vertexp;
            }
        }

        // Handle delay
        std::సదిశ<Sva_State_శీర్షం*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        స్థిరరాశి తార్కిక కూడు_Range_Reject = is_శిఖరం_Level_అడుగు && !m_లోపల_Unbounded_Scope;
        ఒకవేళ (delayp->isRangeDelay()) {
            ఒకవేళ (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 కూడు_Range_Reject ? &rangeRejectInfo : శూన్యసూచిక)) {
                తిరిగి Build_ఫలితం::failWithError();
            }
        } లేకపోతే {
            స్థిరరాశి గుర్తులేని delay_చక్రాలు = getConstUInt(delayp->lhsp());
            currentp = కూడు_Delay_Chain(currentp, delay_చక్రాలు, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        ఒకవేళ (exprp->is_Multi_చక్రం_Sva()) {
            స్థిరరాశి Build_ఫలితం ఫలితం = build_సమాసం(exprp, currentp, is_శిఖరం_Level_అడుగు);
            ఒకవేళ (ఫలితం.valid()) కూడు_Finite_Range_Reject(rangeRejectInfo, ఫలితం, flp);
            తిరిగి ఫలితం;
        }
        తిరిగి {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_ఫలితం buildConsRep(AstSConsRep* repp, Sva_State_శీర్షం* నమోదు_Vtxp,
                             తార్కిక is_శిఖరం_Level_అడుగు = అబద్ధం) {
        File_పంక్తి* స్థిరరాశి flp = repp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        ఒకవేళ (exprp->is_Multi_చక్రం_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            తిరిగి Build_ఫలితం::failWithError();
        }
        స్థిరరాశి గుర్తులేని minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        గుర్తులేని మొత్తం_Sites = minN;
        ఒకవేళ (repp->unbounded()) {
            మొత్తం_Sites += 1;
        } లేకపోతే ఒకవేళ (repp->maxCountp()) {
            మొత్తం_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* స్థిరరాశి hoistVarp = tryHoistSampled(exprp, flp, మొత్తం_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::సదిశ<Sva_State_శీర్షం*> consMidSources;

        Sva_State_శీర్షం* currentp = నమోదు_Vtxp;
        కొరకు (గుర్తులేని i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            ఒకవేళ (i == 1) {
                currentp = కూడు_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = కొత్త AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = నిజం;
                ఒకవేళ (is_శిఖరం_Level_అడుగు) {
                    currentp->m_throughoutConds.నెట్టు_వెనుక(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            Sva_State_శీర్షం* స్థిరరాశి condVtxp = scoped_Create_శీర్షం();
            Sva_Trans_అంచు* స్థిరరాశి linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            ఒకవేళ (is_శిఖరం_Level_అడుగు && i == 0) linkp->m_rejectOnFail = నిజం;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        ఒకవేళ (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.నెట్టు_వెనుక(currentp);
        }

        ఒకవేళ (repp->unbounded()) {
            ఒకవేళ (minN == 0) {
                Sva_State_శీర్షం* స్థిరరాశి waitVtxp = scoped_Create_శీర్షం();
                guarded_అంచు(currentp, waitVtxp, flp);
                Sva_State_శీర్షం* స్థిరరాశి checkVtxp = scoped_Create_శీర్షం();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guarded_అంచు(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } లేకపోతే {
                Sva_State_శీర్షం* స్థిరరాశి loop_వెనుక_Vtxp = scoped_Create_శీర్షం();
                guarded_అంచు(currentp, loop_వెనుక_Vtxp, flp);
                Sva_State_శీర్షం* స్థిరరాశి reCheckVtxp = scoped_Create_శీర్షం();
                guardedLink(loop_వెనుక_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guarded_అంచు(reCheckVtxp, loop_వెనుక_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = నిజం;
            m_లోపల_Unbounded_Scope = నిజం;
        } లేకపోతే ఒకవేళ (repp->maxCountp()) {
            స్థిరరాశి గుర్తులేని maxN = getConstUInt(repp->maxCountp());
            Sva_State_శీర్షం* స్థిరరాశి విలీనం_Vtxp = scoped_Create_శీర్షం();
            guardedLink(currentp, విలీనం_Vtxp, flp);
            గుర్తులేని తోక_Min_N = minN;
            Sva_State_శీర్షం* తోక_Startp = currentp;
            ఒకవేళ (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                Sva_State_శీర్షం* స్థిరరాశి తదుపరి_Vtxp = scoped_Create_శీర్షం();
                guarded_అంచు(currentp, తదుపరి_Vtxp, flp);
                Sva_State_శీర్షం* స్థిరరాశి checkVtxp = scoped_Create_శీర్షం();
                guardedLink(తదుపరి_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, విలీనం_Vtxp, flp);
                ఒకవేళ (m_isCoverSeq) consMidSources.నెట్టు_వెనుక(checkVtxp);
                తోక_Startp = checkVtxp;
                తోక_Min_N = 1;
            }
            ఒకవేళ (maxN > తోక_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                Sva_State_శీర్షం* స్థిరరాశి తదుపరి_Vtxp
                    = కూడు_Delay_Chain(తోక_Startp, maxN - తోక_Min_N + 1, flp, అబద్ధం);
                తదుపరి_Vtxp->m_delayRingClearCondp
                    = కొత్త AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                తదుపరి_Vtxp->m_replayAbortReject = నిజం;
                Sva_State_శీర్షం* స్థిరరాశి checkVtxp = scoped_Create_శీర్షం();
                guardedLink(తదుపరి_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, విలీనం_Vtxp, flp);
                ఒకవేళ (m_isCoverSeq) consMidSources.నెట్టు_వెనుక(checkVtxp);
            }
            currentp = విలీనం_Vtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_ఫలితం res;
        res.పదం_Vertexp = currentp;
        res.finalCondp = శూన్యసూచిక;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.పదం_Is_Mid_విలీనం = m_isCoverSeq && !res.midSources.ఖాళీలు();
        తిరిగి res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_ఫలితం buildPropAlways(AstPropAlways* nodep, Sva_State_శీర్షం* నమోదు_Vtxp,
                                తార్కిక is_శిఖరం_Level_అడుగు = అబద్ధం) {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి propp = nodep->propp();
        స్థిరరాశి గుర్తులేని lo = getConstUInt(nodep->loBoundp());
        ఒకవేళ (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            Sva_State_శీర్షం* స్థిరరాశి livep = కూడు_Delay_Chain(నమోదు_Vtxp, lo, flp);
            livep->m_isUnbounded = నిజం;
            guarded_అంచు(livep, livep, flp);  // stay active every subsequent cycle
            Sva_State_శీర్షం* స్థిరరాశి sinkp = m_లేఖాచిత్రం.create_State_శీర్షం();
            sinkp->m_isRejectSink = నిజం;
            Sva_Trans_అంచు* స్థిరరాశి rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_వృక్షం_శుద్ధ(అబద్ధం)), flp);
            ఒకవేళ (is_శిఖరం_Level_అడుగు) rejEdgep->m_rejectOnFail = నిజం;
            తిరిగి {livep, శూన్యసూచిక, {}};
        }
        స్థిరరాశి గుర్తులేని hi = getConstUInt(nodep->hiBoundp());
        // Strong s_always[m:n]: mark every in-window registered vertex so an
        // attempt still mid-window at end-of-simulation is reported as a liveness
        // failure (IEEE strong: the n+1 ticks must exist). An attempt that has
        // completed earlier in the trace has already cleared its state, so it is
        // not flagged; an attempt whose final tick coincides with $finish is still
        // flagged, matching the strong reference. Weak always[m:n] is not marked.
        VL_RESTORER(m_markStrongPending);
        m_markStrongPending = nodep->isStrong();
        // Check the first in-window tick, then reuse a guarded fixed-delay ring
        // for the remaining ticks instead of creating one state per cycle.
        Sva_State_శీర్షం* currentp = కూడు_Delay_Chain(నమోదు_Vtxp, lo, flp);
        Sva_State_శీర్షం* స్థిరరాశి checkp = scoped_Create_శీర్షం();
        Sva_Trans_అంచు* స్థిరరాశి linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_వృక్షం_శుద్ధ(అబద్ధం)), flp);
        ఒకవేళ (is_శిఖరం_Level_అడుగు) linkp->m_rejectOnFail = నిజం;
        currentp = checkp;
        m_temporal_Guard_దొంతర.నెట్టు_వెనుక(propp);
        currentp = కూడు_Delay_Chain(currentp, hi - lo, flp);
        ఒకవేళ (nodep->isStrong() && currentp->m_delay_Ring_కొలత) currentp->m_strongAlwaysRing = నిజం;
        m_temporal_Guard_దొంతర.తీయి_వెనుక();
        తిరిగి {currentp, propp, {}};
    }

    Build_ఫలితం buildGotoRep(AstSGotoRep* repp, Sva_State_శీర్షం* నమోదు_Vtxp) {
        File_పంక్తి* స్థిరరాశి flp = repp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = repp->exprp();
        స్థిరరాశి గుర్తులేని minN = getConstUInt(repp->countp());
        ఒకవేళ (minN == 0) తిరిగి Build_ఫలితం::fail();
        స్థిరరాశి తార్కిక hasMax = repp->maxCountp() != శూన్యసూచిక;
        స్థిరరాశి గుర్తులేని maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        ఒకవేళ (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            తిరిగి Build_ఫలితం::failWithError();
        }

        AstVar* స్థిరరాశి hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        Sva_State_శీర్షం* స్థిరరాశి waitVtxp = scoped_Create_శీర్షం();
        guarded_అంచు(నమోదు_Vtxp, waitVtxp, flp);
        guarded_అంచు(waitVtxp, waitVtxp,
                    కొత్త AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        Sva_State_శీర్షం* currentp = scoped_Create_శీర్షం();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        ఒకవేళ (minN > 1) {
            Sva_State_శీర్షం* స్థిరరాశి ringVtxp = కూడు_Delay_Chain(
                currentp, minN - 1, flp, నిజం, శూన్యసూచిక, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = నిజం;
            Sva_State_శీర్షం* స్థిరరాశి checkVtxp = scoped_Create_శీర్షం();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        ఒకవేళ (!hasMax) {
            currentp->m_isUnbounded = నిజం;  // [->N] waits unboundedly
            m_లోపల_Unbounded_Scope = నిజం;
            తిరిగి {currentp, శూన్యసూచిక, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        Sva_State_శీర్షం* స్థిరరాశి విలీనం_Vtxp = scoped_Create_శీర్షం();
        guardedLink(currentp, విలీనం_Vtxp, flp);  // accept at match_M
        ఒకవేళ (maxN > minN) {
            Sva_State_శీర్షం* స్థిరరాశి ringVtxp
                = కూడు_Delay_Chain(currentp, maxN - minN + 1, flp, అబద్ధం, శూన్యసూచిక,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = నిజం;
            Sva_State_శీర్షం* స్థిరరాశి checkVtxp = scoped_Create_శీర్షం();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, విలీనం_Vtxp, flp);
        }
        విలీనం_Vtxp->m_isUnbounded = నిజం;  // [->M:N] still has unbounded waits between matches
        m_లోపల_Unbounded_Scope = నిజం;
        తిరిగి {విలీనం_Vtxp, శూన్యసూచిక, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    స్థిర తార్కిక hasZeroMinRep(స్థిరరాశి Ast_కణుపు_సమాసం* seqp) {
        తిరిగి seqp->exists(
            [](స్థిరరాశి AstSConsRep* repp) { తిరిగి getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    స్థిర తార్కిక isPlainBoolean(స్థిరరాశి Ast_కణుపు_సమాసం* condp) {
        తిరిగి !condp->exists(
            [](స్థిరరాశి Ast_కణుపు_సమాసం* np) { తిరిగి np->is_Multi_చక్రం_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_ఫలితం build_Or_విలీనం(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, Sva_State_శీర్షం* నమోదు_Vtxp,
                             File_పంక్తి* flp, తార్కిక want_Bool_ఆకు) {
        స్థిరరాశి Build_ఫలితం lhs = build_సమాసం(lhsp, నమోదు_Vtxp);
        స్థిరరాశి Build_ఫలితం rhs = build_సమాసం(rhsp, నమోదు_Vtxp);
        ఒకవేళ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            తిరిగి Build_ఫలితం::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        స్థిరరాశి తార్కిక booleanOnly = lhs.పదం_Vertexp == నమోదు_Vtxp && lhs.finalCondp
                                 && rhs.పదం_Vertexp == నమోదు_Vtxp && rhs.finalCondp;
        ఒకవేళ (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            తిరిగి Build_ఫలితం::failWithError();
        }
        ఒకవేళ (booleanOnly && want_Bool_ఆకు) {
            ఒకవేళ (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_కణుపు_సమాసం* స్థిరరాశి orCondp
                    = కొత్త AstLogOr{flp, lhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                   rhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                తిరిగి {నమోదు_Vtxp, orCondp, {}};
            }
        }
        Sva_State_శీర్షం* స్థిరరాశి విలీనం_Vtxp = scoped_Create_శీర్షం();
        ఒకవేళ (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(నమోదు_Vtxp, విలీనం_Vtxp,
                        కొత్త AstLogOr{flp, sampled(lhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)),
                                     sampled(rhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం))},
                        flp);
        } లేకపోతే {
            ఒకవేళ (lhs.finalCondp) {
                guardedLink(lhs.పదం_Vertexp, విలీనం_Vtxp,
                            sampled(lhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)), flp);
            } లేకపోతే {
                guardedLink(lhs.పదం_Vertexp, విలీనం_Vtxp, flp);
            }
            ఒకవేళ (rhs.finalCondp) {
                guardedLink(rhs.పదం_Vertexp, విలీనం_Vtxp,
                            sampled(rhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)), flp);
            } లేకపోతే {
                guardedLink(rhs.పదం_Vertexp, విలీనం_Vtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        తిరిగి {విలీనం_Vtxp, శూన్యసూచిక, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_ఫలితం buildAndCombiner(Ast_కణుపు_సమాసం* lhsExprp, Ast_కణుపు_సమాసం* rhsExprp,
                                 Sva_State_శీర్షం* నమోదు_Vtxp, File_పంక్తి* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        స్థిరరాశి తార్కిక savedScope = m_లోపల_Unbounded_Scope;
        స్థిరరాశి Build_ఫలితం lhs = build_సమాసం(lhsExprp, నమోదు_Vtxp);
        స్థిరరాశి తార్కిక lhsScope = m_లోపల_Unbounded_Scope;
        m_లోపల_Unbounded_Scope = savedScope;
        స్థిరరాశి Build_ఫలితం rhs = build_సమాసం(rhsExprp, నమోదు_Vtxp);
        స్థిరరాశి తార్కిక rhsScope = m_లోపల_Unbounded_Scope;
        m_లోపల_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        ఒకవేళ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            తిరిగి Build_ఫలితం::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        ఒకవేళ (lhs.పదం_Vertexp == నమోదు_Vtxp && rhs.పదం_Vertexp == నమోదు_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstLogAnd{flp, lhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                     rhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            తిరిగి {నమోదు_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        ఒకవేళ (!lhs.midSources.ఖాళీలు() || !rhs.midSources.ఖాళీలు()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            తిరిగి Build_ఫలితం::failWithError();
        }
        Sva_State_శీర్షం* స్థిరరాశి combVtxp = scoped_Create_శీర్షం();
        combVtxp->m_isAndCombiner = నిజం;
        combVtxp->m_andLhsTermp = lhs.పదం_Vertexp;
        combVtxp->m_andRhsTermp = rhs.పదం_Vertexp;
        ఒకవేళ (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        ఒకవేళ (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        ఒకవేళ (lhs.పదం_Vertexp->m_isUnbounded || rhs.పదం_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = నిజం;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        ఒకవేళ (!combVtxp->m_isUnbounded) {
            తార్కిక needSink = అబద్ధం;
            స్థిరరాశి తార్కిక lhs_Multi_చక్రం = (lhs.పదం_Vertexp != నమోదు_Vtxp);
            స్థిరరాశి తార్కిక rhs_Multi_చక్రం = (rhs.పదం_Vertexp != నమోదు_Vtxp);
            ఒకవేళ (lhs.finalCondp && lhs_Multi_చక్రం && !lhs.పదం_Vertexp->m_isUnbounded) {
                needSink = నిజం;
            }
            ఒకవేళ (rhs.finalCondp && rhs_Multi_చక్రం && !rhs.పదం_Vertexp->m_isUnbounded) {
                needSink = నిజం;
            }
            ఒకవేళ (needSink) {
                Sva_State_శీర్షం* స్థిరరాశి sinkVtxp = m_లేఖాచిత్రం.create_State_శీర్షం();
                sinkVtxp->m_isRejectSink = నిజం;
                ఒకవేళ (lhs.finalCondp && lhs_Multi_చక్రం && !lhs.పదం_Vertexp->m_isUnbounded) {
                    Sva_Trans_అంచు* స్థిరరాశి ep = m_లేఖాచిత్రం.కూడు_Link(
                        lhs.పదం_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)));
                    ep->m_rejectOnFail = నిజం;
                }
                ఒకవేళ (rhs.finalCondp && rhs_Multi_చక్రం && !rhs.పదం_Vertexp->m_isUnbounded) {
                    Sva_Trans_అంచు* స్థిరరాశి ep = m_లేఖాచిత్రం.కూడు_Link(
                        rhs.పదం_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)));
                    ep->m_rejectOnFail = నిజం;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        తిరిగి {combVtxp, శూన్యసూచిక, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_ఫలితం buildSWithin(AstSWithin* nodep, Sva_State_శీర్షం* నమోదు_Vtxp,
                             తార్కిక is_శిఖరం_Level_అడుగు = అబద్ధం) {
        స్థిరరాశి పూర్ణాంకం innerLen = fixed_నిడివి(nodep->lhsp());
        స్థిరరాశి పూర్ణాంకం outerLen = fixed_నిడివి(nodep->rhsp());
        ఒకవేళ (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            తిరిగి Build_ఫలితం::failWithError();
        }
        ఒకవేళ (innerLen > outerLen) {
            తిరిగి build_Never_సరిపోలిక_Intersect(
                nodep, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు,
                "the inner sequence is longer than the outer sequence");
        }
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        స్థిరరాశి పూర్ణాంకం slack = outerLen - innerLen;
        Ast_కణుపు_సమాసం* innerOrp = శూన్యసూచిక;
        కొరకు (పూర్ణాంకం i = 0; i <= slack; ++i) {
            స్థిరరాశి పూర్ణాంకం postPad = slack - i;
            Ast_కణుపు_సమాసం* branchp = nodep->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
            ఒకవేళ (i > 0) {
                AstConst* స్థిరరాశి prePadp = కొత్త AstConst{flp, AstConst::బిట్టు_True{}};
                AstDelay* స్థిరరాశి delayp
                    = కొత్త AstDelay{flp, కొత్త AstConst{flp, static_cast<uint32_t>(i)}, నిజం};
                AstSExpr* స్థిరరాశి wrapped = కొత్త AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_సమితి_బిట్టు();
                branchp = wrapped;
            }
            ఒకవేళ (postPad > 0) {
                AstConst* స్థిరరాశి postTruep = కొత్త AstConst{flp, AstConst::బిట్టు_True{}};
                AstDelay* స్థిరరాశి delayp
                    = కొత్త AstDelay{flp, కొత్త AstConst{flp, static_cast<uint32_t>(postPad)}, నిజం};
                AstSExpr* స్థిరరాశి wrapped = కొత్త AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_సమితి_బిట్టు();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_కణుపు_సమాసం* స్థిరరాశి outerClonep = nodep->rhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_కణుపు_సమాసం* స్థిరరాశి combinedp = కొత్త AstSIntersect{flp, innerOrp, outerClonep};
        Build_ఫలితం ఫలితం = build_సమాసం(combinedp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        VL_DO_DANGLING(combinedp->delete_వృక్షం(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        ఒకవేళ (ఫలితం.valid() && ఫలితం.finalCondp && !ఫలితం.finalCondp->backp()) {
            Sva_State_శీర్షం* స్థిరరాశి wrapVtxp = scoped_Create_శీర్షం();
            guardedLink(ఫలితం.పదం_Vertexp, wrapVtxp, sampled(ఫలితం.finalCondp), flp);
            ఫలితం = {wrapVtxp, శూన్యసూచిక, ఫలితం.midSources};
        }
        తిరిగి ఫలితం;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    స్థిర తార్కిక flattenFixedSeq(Ast_కణుపు_సమాసం* nodep, పూర్ణాంకం పునాది_స్థానభ్రంశం,
                                std::map<పూర్ణాంకం, std::సదిశ<Ast_కణుపు_సమాసం*>>& బహిర్గతం) {
        ఒకవేళ (AstSExpr* స్థిరరాశి sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* స్థిరరాశి delayp = VN_CAST(sexprp->delayp(), Delay);
            ఒకవేళ (!delayp || !delayp->is_చక్రం_Delay() || delayp->isUnbounded()) తిరిగి అబద్ధం;
            స్థిరరాశి గుర్తులేని delay_చక్రాలు = getConstUInt(delayp->lhsp());
            ఒకవేళ (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_చక్రాలు)
                తిరిగి అబద్ధం;
            పూర్ణాంకం preLen = 0;
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి prep = sexprp->preExprp()) {
                ఒకవేళ (!flattenFixedSeq(prep, పునాది_స్థానభ్రంశం, బహిర్గతం)) తిరిగి అబద్ధం;
                preLen = fixed_నిడివి(prep);
                ఒకవేళ (preLen < 0) తిరిగి అబద్ధం;
            }
            తిరిగి flattenFixedSeq(sexprp->exprp(), పునాది_స్థానభ్రంశం + preLen + delay_చక్రాలు, బహిర్గతం);
        }
        ఒకవేళ (nodep->is_Multi_చక్రం_Sva()) తిరిగి అబద్ధం;
        బహిర్గతం[పునాది_స్థానభ్రంశం].నెట్టు_వెనుక(nodep);
        తిరిగి నిజం;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    స్థిర Ast_కణుపు_సమాసం* conjoinFixedSeqs(Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp, File_పంక్తి* flp) {
        std::map<పూర్ణాంకం, std::సదిశ<Ast_కణుపు_సమాసం*>> checks;
        ఒకవేళ (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) తిరిగి శూన్యసూచిక;
        ఒకవేళ (checks.ఖాళీలు()) తిరిగి శూన్యసూచిక;
        Ast_కణుపు_సమాసం* resultp = శూన్యసూచిక;
        పూర్ణాంకం prev_స్థానభ్రంశం = 0;
        కొరకు (స్థిరరాశి స్వయంచాలక& స్థానభ్రంశం_Checks : checks) {
            స్థిరరాశి పూర్ణాంకం స్థానభ్రంశం = స్థానభ్రంశం_Checks.మొదటి;
            Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి leafp : స్థానభ్రంశం_Checks.రెండవ) {
                Ast_కణుపు_సమాసం* స్థిరరాశి clonep = leafp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                ఒకవేళ (!condp) {
                    condp = clonep;
                } లేకపోతే {
                    condp = కొత్త AstLogAnd{flp, condp, clonep};
                    condp->dtype_సమితి_బిట్టు();
                }
            }
            ఒకవేళ (!resultp) {
                ఒకవేళ (స్థానభ్రంశం > 0) {
                    AstDelay* స్థిరరాశి delayp = కొత్త AstDelay{
                        flp, కొత్త AstConst{flp, static_cast<uint32_t>(స్థానభ్రంశం)}, /*isCycle=*/నిజం};
                    resultp
                        = కొత్త AstSExpr{flp, కొత్త AstConst{flp, AstConst::బిట్టు_True{}}, delayp, condp};
                    resultp->dtype_సమితి_బిట్టు();
                } లేకపోతే {
                    resultp = condp;
                }
            } లేకపోతే {
                AstDelay* స్థిరరాశి delayp = కొత్త AstDelay{
                    flp, కొత్త AstConst{flp, static_cast<uint32_t>(స్థానభ్రంశం - prev_స్థానభ్రంశం)},
                    /*isCycle=*/నిజం};
                resultp = కొత్త AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_సమితి_బిట్టు();
            }
            prev_స్థానభ్రంశం = స్థానభ్రంశం;
        }
        తిరిగి resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    నిర్మాణం SimpleRanged final {
        తార్కిక ok = అబద్ధం;
        Ast_కణుపు_సమాసం* startp = శూన్యసూచిక;  // may be null (absent start)
        Ast_కణుపు_సమాసం* endp = శూన్యసూచిక;
    };
    స్థిర SimpleRanged asSimpleRanged(Ast_కణుపు_సమాసం* nodep) {
        AstSExpr* స్థిరరాశి sexprp = VN_CAST(nodep, SExpr);
        ఒకవేళ (!sexprp) తిరిగి {};
        AstDelay* స్థిరరాశి delayp = VN_CAST(sexprp->delayp(), Delay);
        ఒకవేళ (!delayp || !delayp->is_చక్రం_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            తిరిగి {};
        ఒకవేళ (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) తిరిగి {};
        Ast_కణుపు_సమాసం* స్థిరరాశి prep = sexprp->preExprp();
        ఒకవేళ (prep && fixed_నిడివి(prep) != 0) తిరిగి {};
        ఒకవేళ (fixed_నిడివి(sexprp->exprp()) != 0) తిరిగి {};
        తిరిగి {నిజం, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_ఫలితం build_From_Lowering_వృక్షం(Ast_కణుపు_సమాసం* treep, Sva_State_శీర్షం* నమోదు_Vtxp,
                                      తార్కిక is_శిఖరం_Level_అడుగు) {
        Build_ఫలితం ఫలితం = build_సమాసం(treep, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        ఒకవేళ (ఫలితం.valid() && ఫలితం.finalCondp) {
            ఫలితం.finalCondp = ఫలితం.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        }
        VL_DO_DANGLING(treep->delete_వృక్షం(), treep);
        తిరిగి ఫలితం;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_ఫలితం build_Never_సరిపోలిక_Intersect(Ast_కణుపు_సమాసం* nodep, Sva_State_శీర్షం* నమోదు_Vtxp,
                                         తార్కిక is_శిఖరం_Level_అడుగు, స్థిరరాశి అక్షరం* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_కణుపు_సమాసం* స్థిరరాశి falsep = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}};
        తిరిగి build_From_Lowering_వృక్షం(falsep, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
    }

    // Lower `seq1 intersect seq2` when an operand's match length varies
    // (IEEE 1800-2023 16.9.6: both match over one window, equal start and end).
    // The common length range is [lo,hi] = intersection of the two operands'
    // achievable lengths. The equal-length combiner is avoided -- it mis-handles
    // operands with an internal boolean check -- by lowering to plain sequences:
    //  - lo == hi (one shared length, e.g. one fixed + one ranged operand):
    //    pin each operand to that length and conjoin them cycle-by-cycle into a
    //    single fixed sequence.
    //  - lo < hi with simple `bool ##[m:n] bool` operands: collapse to one
    //    ranged delay `(start1 & start2) ##[lo:hi] (end1 & end2)`. (An OR of
    //    per-length branches cannot reject correctly -- a single missed length
    //    would fail the whole intersect, cf. Lesson 48.)
    //  - otherwise unsupported (clean error, not the legacy fall-through crash).
    Build_ఫలితం buildVarLenIntersect(AstSIntersect* nodep, Sva_State_శీర్షం* నమోదు_Vtxp,
                                     తార్కిక is_శిఖరం_Level_అడుగు) {
        స్థిరరాశి std::pair<పూర్ణాంకం, పూర్ణాంకం> lhsRange = నిడివి_Range(nodep->lhsp());
        స్థిరరాశి std::pair<పూర్ణాంకం, పూర్ణాంకం> rhsRange = నిడివి_Range(nodep->rhsp());
        ఒకవేళ (lhsRange.మొదటి < 0 || rhsRange.మొదటి < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            తిరిగి Build_ఫలితం::failWithError();
        }
        స్థిరరాశి పూర్ణాంకం lo = std::max(lhsRange.మొదటి, rhsRange.మొదటి);
        స్థిరరాశి పూర్ణాంకం hi = std::min(lhsRange.రెండవ, rhsRange.రెండవ);
        ఒకవేళ (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            తిరిగి build_Never_సరిపోలిక_Intersect(nodep, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు,
                                            "intersect operands have no common length");
        }
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ఒకవేళ (lo == hi) {
            Ast_కణుపు_సమాసం* స్థిరరాశి lp = realize_At_నిడివి(nodep->lhsp(), lo, lhsRange.మొదటి);
            Ast_కణుపు_సమాసం* స్థిరరాశి rp = realize_At_నిడివి(nodep->rhsp(), lo, rhsRange.మొదటి);
            Ast_కణుపు_సమాసం* స్థిరరాశి conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_వృక్షం(), lp);
            VL_DO_DANGLING(rp->delete_వృక్షం(), rp);
            ఒకవేళ (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                తిరిగి Build_ఫలితం::failWithError();
            }
            తిరిగి build_From_Lowering_వృక్షం(conjp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        }
        స్థిరరాశి SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        స్థిరరాశి SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        ఒకవేళ (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            తిరిగి Build_ఫలితం::failWithError();
        }
        స్థిరరాశి స్వయంచాలక andBool = [&](Ast_కణుపు_సమాసం* ap, Ast_కణుపు_సమాసం* bp) -> Ast_కణుపు_సమాసం* {
            Ast_కణుపు_సమాసం* స్థిరరాశి aClonep
                = ap ? ap->clone_వృక్షం_శుద్ధ(అబద్ధం) : కొత్త AstConst{flp, AstConst::బిట్టు_True{}};
            Ast_కణుపు_సమాసం* స్థిరరాశి bClonep
                = bp ? bp->clone_వృక్షం_శుద్ధ(అబద్ధం) : కొత్త AstConst{flp, AstConst::బిట్టు_True{}};
            AstLogAnd* స్థిరరాశి andp = కొత్త AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_సమితి_బిట్టు();
            తిరిగి andp;
        };
        AstDelay* స్థిరరాశి delayp = కొత్త AstDelay{flp, కొత్త AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/నిజం};
        delayp->rhsp(కొత్త AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* స్థిరరాశి reducedp
            = కొత్త AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_సమితి_బిట్టు();
        తిరిగి build_From_Lowering_వృక్షం(reducedp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
    }

    Build_ఫలితం buildThroughout(AstSThroughout* nodep, Sva_State_శీర్షం* నమోదు_Vtxp,
                                తార్కిక is_శిఖరం_Level_అడుగు = అబద్ధం) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        నమోదు_Vtxp->m_throughoutConds.నెట్టు_వెనుక(nodep->lhsp()->clone_వృక్షం_శుద్ధ(అబద్ధం));
        m_temporal_Guard_దొంతర.నెట్టు_వెనుక(nodep->lhsp());
        స్థిరరాశి Build_ఫలితం ఫలితం = build_సమాసం(nodep->rhsp(), నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        m_temporal_Guard_దొంతర.తీయి_వెనుక();
        తిరిగి ఫలితం;
    }

    // until / until_with per IEEE 1800-2023 16.12.12.
    // Topology: combinational wait vertex with self-feeding state register.
    //   entry  --link[T]--> waitC
    //   waitR  --link[T]--> waitC                          (back-loop)
    //   waitC  --edge[##1, sampled(p) && !sampled(q)]--> waitR  (continue)
    //   waitC  --link[REQUIRE, rejectOnFail]--> sink            (per-cycle fail)
    //   waitC  --link[T]--> match  (added by wireMatchAndMidSources;
    //                               accept condition rides via finalCondp)
    // waitC is m_isUnbounded so the terminal-match link contributes only to
    // terminalActive, not to rejectBase (which would otherwise spuriously fire
    // every cycle q is false). Per-cycle reject comes from the explicit
    // rejectOnFail link to the sink vertex.
    //
    // Non-overlapping (p until q):
    //   REQUIRE = sampled(p) || sampled(q)   accept = sampled(q)
    // Overlapping (p until_with q):
    //   REQUIRE = sampled(p)                 accept = sampled(p) && sampled(q)
    // Strong forms use the same checks and mark the registered wait state as
    // an end-of-simulation liveness obligation.
    Build_ఫలితం buildUntil(AstUntil* nodep, Sva_State_శీర్షం* నమోదు_Vtxp, తార్కిక is_శిఖరం_Level_అడుగు) {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ఒకవేళ (!is_శిఖరం_Level_అడుగు) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            తిరిగి Build_ఫలితం::failWithError();
        }
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp();
        స్థిరరాశి స్వయంచాలక hasSeq = [](స్థిరరాశి Ast_కణుపు_సమాసం* ep) {
            తిరిగి ep->exists([](స్థిరరాశి Ast_కణుపు_సమాసం* np) { తిరిగి np->is_Multi_చక్రం_Sva(); });
        };
        ఒకవేళ (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            తిరిగి Build_ఫలితం::failWithError();
        }

        స్థిరరాశి తార్కిక ov = nodep->isOverlapping();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsBitp = nodep->lhsp();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* స్థిరరాశి pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* స్థిరరాశి qHoistp = ov ? శూన్యసూచిక : tryHoistSampled(rhsBitp, flp, 2);

        Sva_State_శీర్షం* స్థిరరాశి waitCp = scoped_Create_శీర్షం();
        Sva_State_శీర్షం* స్థిరరాశి waitRp = scoped_Create_శీర్షం();
        waitCp->m_isUnbounded = నిజం;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(నమోదు_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_కణుపు_సమాసం* స్థిరరాశి contCondp
            = కొత్త AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            కొత్త AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guarded_అంచు(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        Sva_State_శీర్షం* స్థిరరాశి sinkVtxp = m_లేఖాచిత్రం.create_State_శీర్షం();
        sinkVtxp->m_isRejectSink = నిజం;
        Ast_కణుపు_సమాసం* requireCondp;
        ఒకవేళ (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } లేకపోతే {
            requireCondp = కొత్త AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        Sva_Trans_అంచు* స్థిరరాశి rejEdgep = m_లేఖాచిత్రం.కూడు_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = నిజం;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_కణుపు_సమాసం* acceptCondp;
        ఒకవేళ (ov) {
            acceptCondp
                = కొత్త AstLogAnd{flp, lhsBitp->clone_వృక్షం_శుద్ధ(అబద్ధం), rhsBitp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
        } లేకపోతే {
            acceptCondp = rhsBitp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        }
        తిరిగి {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_కణుపు_సమాసం* abort_Fire_సమాసం(Ast_కణుపు_సమాసం* condp, File_పంక్తి* flp) {
        Ast_కణుపు_సమాసం* resultp = condp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి op : m_outer_Abort_దొంతర)
            resultp = కొత్త AstLogAnd{flp, resultp, కొత్త AstLogNot{flp, op->clone_వృక్షం_శుద్ధ(అబద్ధం)}};
        తిరిగి resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    స్థిర తార్కిక chain_Accounts_ఆధారం(స్థిరరాశి Sva_State_శీర్షం* srcp,
                                    స్థిరరాశి std::unordered_సమితి<స్థిరరాశి V3_లేఖాచిత్రం_అంచు*>& pre_అంచులు) {
        ఒకవేళ (srcp->m_delay_Ring_కొలత && srcp->m_throughoutConds.ఖాళీలు()) తిరిగి నిజం;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& edger : srcp->లోపల_అంచులు()) {
            ఒకవేళ (pre_అంచులు.లెక్క(&edger)) కొనసాగు;
            స్థిరరాశి Sva_Trans_అంచు& tedger = static_cast<స్థిరరాశి Sva_Trans_అంచు&>(edger);
            ఒకవేళ (tedger.m_consumes_చక్రం) కొనసాగు;
            స్థిరరాశి స్వయంచాలక* స్థిరరాశి fromp = static_cast<స్థిరరాశి Sva_State_శీర్షం*>(tedger.fromVtxp());
            ఒకవేళ (fromp->m_abortRejectp) తిరిగి నిజం;
        }
        తార్కిక plainNonSink = అబద్ధం;
        తార్కిక markedSink = అబద్ధం;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& edger : srcp->బహిర్గతం_అంచులు()) {
            ఒకవేళ (pre_అంచులు.లెక్క(&edger)) కొనసాగు;
            స్థిరరాశి Sva_Trans_అంచు& tedger = static_cast<స్థిరరాశి Sva_Trans_అంచు&>(edger);
            ఒకవేళ (tedger.m_consumes_చక్రం) కొనసాగు;
            స్థిరరాశి తార్కిక sink = static_cast<స్థిరరాశి Sva_State_శీర్షం*>(tedger.గమ్యం_Vtxp())->m_isRejectSink;
            ఒకవేళ (tedger.m_rejectOnFail) {
                ఒకవేళ (!sink) తిరిగి నిజం;
                markedSink = నిజం;
            } లేకపోతే ఒకవేళ (!sink) {
                ఒకవేళ (srcp->m_abortRejectp) తిరిగి నిజం;
                plainNonSink = నిజం;
            }
        }
        తిరిగి plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    శూన్య కూడు_Abort_Reject_అంచు(Sva_State_శీర్షం* srcp, Sva_State_శీర్షం* sinkp, Ast_కణుపు_సమాసం* condp,
                            File_పంక్తి* flp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి notFirep = కొత్త AstLogNot{flp, sampled(abort_Fire_సమాసం(condp, flp))};
        m_లేఖాచిత్రం.కూడు_Link(srcp, sinkp, notFirep)->m_rejectOnFail = నిజం;
        తిరిగి;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    శూన్య gate_Body_అంచులు_On_Abort(స్థిరరాశి std::unordered_సమితి<స్థిరరాశి V3_లేఖాచిత్రం_అంచు*>& pre_అంచులు,
                              Ast_కణుపు_సమాసం* condp, VAbort_రకం రకం, File_పంక్తి* flp) {
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : m_లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            కొరకు (V3_లేఖాచిత్రం_అంచు& edger : vtxr.బహిర్గతం_అంచులు()) {
                ఒకవేళ (pre_అంచులు.లెక్క(&edger)) కొనసాగు;
                Sva_Trans_అంచు* స్థిరరాశి tedgep = static_cast<Sva_Trans_అంచు*>(&edger);
                ఒకవేళ (tedgep->m_rejectOnFail) {
                    ఒకవేళ (!రకం.isAccept()) కొనసాగు;
                    Ast_కణుపు_సమాసం* స్థిరరాశి firep = sampled(abort_Fire_సమాసం(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? కొత్త AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } లేకపోతే ఒకవేళ (tedgep->m_consumes_చక్రం) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి notFirep
                        = కొత్త AstLogNot{flp, sampled(abort_Fire_సమాసం(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? కొత్త AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_ఫలితం buildAbortOn(Ast_కణుపు_సమాసం* condp, Ast_కణుపు_సమాసం* bodyp, Sva_State_శీర్షం* నమోదు_Vtxp,
                             VAbort_రకం రకం, File_పంక్తి* flp, తార్కిక is_శిఖరం_Level_అడుగు) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_సమితి<స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం*> preExisting;
        std::unordered_సమితి<స్థిరరాశి V3_లేఖాచిత్రం_అంచు*> pre_అంచులు;
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : m_లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            preExisting.చొప్పించు(&vtxr);
            కొరకు (V3_లేఖాచిత్రం_అంచు& edger : vtxr.బహిర్గతం_అంచులు()) pre_అంచులు.చొప్పించు(&edger);
        }

        m_outer_Abort_దొంతర.నెట్టు_వెనుక(condp);
        స్థిరరాశి Build_ఫలితం body_ఫలితం = build_సమాసం(bodyp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        m_outer_Abort_దొంతర.తీయి_వెనుక();
        ఒకవేళ (!body_ఫలితం.valid()) తిరిగి body_ఫలితం;

        gate_Body_అంచులు_On_Abort(pre_అంచులు, condp, రకం, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::సదిశ<Sva_State_శీర్షం*> abortSources;
        abortSources.నెట్టు_వెనుక(నమోదు_Vtxp);
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : m_లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            ఒకవేళ (preExisting.లెక్క(&vtxr)) కొనసాగు;
            స్వయంచాలక* స్థిరరాశి sp = static_cast<Sva_State_శీర్షం*>(&vtxr);
            ఒకవేళ (sp->m_delay_Ring_కొలత || !sp->m_throughoutConds.ఖాళీలు()) {
                Ast_కణుపు_సమాసం* స్థిరరాశి firep = abort_Fire_సమాసం(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? కొత్త AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            ఒకవేళ (!రకం.isAccept()
                && ((sp->m_delay_Ring_కొలత && sp->m_throughoutConds.ఖాళీలు())
                    || sp->m_replayAbortReject)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి firep = abort_Fire_సమాసం(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? కొత్త AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            ఒకవేళ (sp->m_isRejectSink) కొనసాగు;
            abortSources.నెట్టు_వెనుక(sp);
        }

        స్వయంచాలక sampledAbortFire = [&]() -> Ast_కణుపు_సమాసం* {
            Ast_కణుపు_సమాసం* స్థిరరాశి సమాసం = abort_Fire_సమాసం(condp, flp);
            తిరిగి sampled(సమాసం);
        };

        ఒకవేళ (రకం.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            Sva_State_శీర్షం* స్థిరరాశి acceptSinkp = scoped_Create_శీర్షం();
            కొరకు (Sva_State_శీర్షం* స్థిరరాశి srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::సదిశ<Sva_State_శీర్షం*> midSources = body_ఫలితం.midSources;
            midSources.నెట్టు_వెనుక(acceptSinkp);
            Ast_కణుపు_సమాసం* finalCondp = body_ఫలితం.finalCondp;
            ఒకవేళ (finalCondp) {
                ఒకవేళ (finalCondp->backp()) finalCondp = finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                finalCondp = కొత్త AstLogOr{flp, finalCondp, abort_Fire_సమాసం(condp, flp)};
            }
            తిరిగి {body_ఫలితం.పదం_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        Sva_State_శీర్షం* స్థిరరాశి rejectSinkp = m_లేఖాచిత్రం.create_State_శీర్షం();
        rejectSinkp->m_isRejectSink = నిజం;
        కొరకు (Sva_State_శీర్షం* స్థిరరాశి srcp : abortSources)
            ఒకవేళ (!chain_Accounts_ఆధారం(srcp, pre_అంచులు))
                కూడు_Abort_Reject_అంచు(srcp, rejectSinkp, condp, flp);
        Ast_కణుపు_సమాసం* finalCondp = body_ఫలితం.finalCondp;
        ఒకవేళ (finalCondp) {
            ఒకవేళ (finalCondp->backp()) finalCondp = finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            finalCondp
                = కొత్త AstLogAnd{flp, finalCondp, కొత్త AstLogNot{flp, abort_Fire_సమాసం(condp, flp)}};
        }
        తిరిగి {body_ఫలితం.పదం_Vertexp, finalCondp, body_ఫలితం.midSources};
    }

బహిరంగ:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    స్థిర శూన్య freeUnlinkedCondp(Ast_కణుపు_సమాసం* condp) {
        ఒకవేళ (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_వృక్షం(), condp);
    }

    SvaNfaBuilder(Sva_లేఖాచిత్రం& లేఖాచిత్రం, Ast_కణుపు_Module* modp, V3_Unique_పేర్లు& prop_తాత్కాలిక_పేర్లు,
                  తార్కిక isCoverSeq = అబద్ధం, తార్కిక isSeqEvent = అబద్ధం, తార్కిక isCover = అబద్ధం)
        : m_లేఖాచిత్రం{లేఖాచిత్రం}
        , m_modp{modp}
        , m_prop_తాత్కాలిక_పేర్లు{prop_తాత్కాలిక_పేర్లు}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    శూన్య resetScope() {
        m_లోపల_Unbounded_Scope = అబద్ధం;
        m_temporal_Guard_దొంతర.clear();
    }

    Build_ఫలితం build_సమాసం(Ast_కణుపు_సమాసం* nodep, Sva_State_శీర్షం* నమోదు_Vtxp,
                          తార్కిక is_శిఖరం_Level_అడుగు = అబద్ధం) {
        VL_RESTORER(m_want_Bool_ఆకు);
        స్థిరరాశి తార్కిక want_Bool_ఆకు = m_want_Bool_ఆకు;
        m_want_Bool_ఆకు = అబద్ధం;
        ఒకవేళ (AstSExpr* స్థిరరాశి sexprp = VN_CAST(nodep, SExpr)) {
            తిరిగి buildSExpr(sexprp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        }
        ఒకవేళ (AstSConsRep* స్థిరరాశి repp = VN_CAST(nodep, SConsRep)) {
            తిరిగి buildConsRep(repp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        }
        ఒకవేళ (AstPropAlways* స్థిరరాశి alwaysp = VN_CAST(nodep, PropAlways)) {
            తిరిగి buildPropAlways(alwaysp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        }
        ఒకవేళ (AstSGotoRep* స్థిరరాశి repp = VN_CAST(nodep, SGotoRep)) {
            తిరిగి buildGotoRep(repp, నమోదు_Vtxp);
        }
        ఒకవేళ (AstSThroughout* స్థిరరాశి throughoutp = VN_CAST(nodep, SThroughout)) {
            తిరిగి buildThroughout(throughoutp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        }
        ఒకవేళ (AstSOr* స్థిరరాశి orp = VN_CAST(nodep, SOr)) {
            తిరిగి build_Or_విలీనం(orp->lhsp(), orp->rhsp(), నమోదు_Vtxp, orp->fileline(),
                                want_Bool_ఆకు);
        }
        ఒకవేళ (AstLogOr* స్థిరరాశి orp = VN_CAST(nodep, LogOr)) {
            తిరిగి build_Or_విలీనం(orp->lhsp(), orp->rhsp(), నమోదు_Vtxp, orp->fileline(),
                                want_Bool_ఆకు);
        }
        ఒకవేళ (AstSAnd* స్థిరరాశి andp = VN_CAST(nodep, SAnd)) {
            తిరిగి buildAndCombiner(andp->lhsp(), andp->rhsp(), నమోదు_Vtxp, andp->fileline());
        }
        ఒకవేళ (AstSIntersect* స్థిరరాశి intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            స్థిరరాశి పూర్ణాంకం lhsLen = fixed_నిడివి(intp->lhsp());
            స్థిరరాశి పూర్ణాంకం rhsLen = fixed_నిడివి(intp->rhsp());
            ఒకవేళ (lhsLen >= 0 && rhsLen >= 0) {
                ఒకవేళ (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    తిరిగి build_Never_సరిపోలిక_Intersect(intp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు,
                                                    "intersect operands have no common length");
                }
                ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    తిరిగి build_From_Lowering_వృక్షం(conjp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
                }
                తిరిగి buildAndCombiner(intp->lhsp(), intp->rhsp(), నమోదు_Vtxp, intp->fileline());
            }
            తిరిగి buildVarLenIntersect(intp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        }
        ఒకవేళ (AstSWithin* స్థిరరాశి withinp = VN_CAST(nodep, SWithin)) {
            తిరిగి buildSWithin(withinp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        }
        ఒకవేళ (AstAbortOn* స్థిరరాశి ap = VN_CAST(nodep, AbortOn)) {
            తిరిగి buildAbortOn(ap->condp(), ap->propp(), నమోదు_Vtxp, ap->రకం(), ap->fileline(),
                                is_శిఖరం_Level_అడుగు);
        }
        ఒకవేళ (VN_IS(nodep, SNonConsRep)) తిరిగి Build_ఫలితం::fail();
        ఒకవేళ (AstImplication* స్థిరరాశి implp = VN_CAST(nodep, Implication)) {
            తిరిగి build_Implication_అంచులు(implp->lhsp(), implp->rhsp(), నమోదు_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        ఒకవేళ (AstUntil* స్థిరరాశి untilp = VN_CAST(nodep, Until)) {
            తిరిగి buildUntil(untilp, నమోదు_Vtxp, is_శిఖరం_Level_అడుగు);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        తిరిగి {నమోదు_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_ఫలితం build_Implication_అంచులు(Ast_కణుపు_సమాసం* antExprp, Ast_కణుపు_సమాసం* bodyExprp,
                                      Sva_State_శీర్షం* నమోదు_Vtxp, తార్కిక isOverlapped,
                                      తార్కిక isFollowedBy, Ast_కణుపు* errorNodep, File_పంక్తి* flp) {
        స్థిరరాశి Build_ఫలితం ant_ఫలితం = build_సమాసం(antExprp, నమోదు_Vtxp);
        ఒకవేళ (!ant_ఫలితం.valid()) తిరిగి ant_ఫలితం;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        ఒకవేళ (isFollowedBy && ant_ఫలితం.పదం_Vertexp != నమోదు_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            తిరిగి Build_ఫలితం::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_ఫలితం.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        Sva_State_శీర్షం* స్థిరరాశి trigVtxp = m_లేఖాచిత్రం.create_State_శీర్షం();
        ఒకవేళ (ant_ఫలితం.finalCondp) {
            m_లేఖాచిత్రం.కూడు_Link(ant_ఫలితం.పదం_Vertexp, trigVtxp,
                            sampled(ant_ఫలితం.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            ఒకవేళ (isFollowedBy) {
                Sva_State_శీర్షం* స్థిరరాశి sinkVtxp = m_లేఖాచిత్రం.create_State_శీర్షం();
                sinkVtxp->m_isRejectSink = నిజం;
                Sva_Trans_అంచు* స్థిరరాశి ep
                    = m_లేఖాచిత్రం.కూడు_Link(ant_ఫలితం.పదం_Vertexp, sinkVtxp,
                                      sampled(ant_ఫలితం.finalCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)));
                ep->m_rejectOnFail = నిజం;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            ఒకవేళ (!ant_ఫలితం.finalCondp->backp()) {
                VL_DO_DANGLING(ant_ఫలితం.finalCondp->delete_వృక్షం(), ant_ఫలితం.finalCondp);
            }
        } లేకపోతే {
            m_లేఖాచిత్రం.కూడు_Link(ant_ఫలితం.పదం_Vertexp, trigVtxp);
        }
        resetScope();

        Sva_State_శీర్షం* bodyEntryp = trigVtxp;
        ఒకవేళ (!isOverlapped) {
            Sva_State_శీర్షం* స్థిరరాశి delayVtxp = m_లేఖాచిత్రం.create_State_శీర్షం();
            m_లేఖాచిత్రం.కూడు_Clocked_అంచు(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_ఆకు);
        m_want_Bool_ఆకు = !isFollowedBy && !hasZeroMinRep(antExprp);
        తిరిగి build_సమాసం(bodyExprp, bodyEntryp, /*isTopLevelStep=*/నిజం);
    }

    Build_ఫలితం build(Ast_కణుపు_సమాసం* exprp) {
        m_లేఖాచిత్రం.m_ప్రారంభం_Vertexp = scoped_Create_శీర్షం();
        VL_RESTORER(m_want_Bool_ఆకు);
        m_want_Bool_ఆకు = నిజం;
        తిరిగి build_సమాసం(exprp, m_లేఖాచిత్రం.m_ప్రారంభం_Vertexp, /*isTopLevelStep=*/నిజం);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

తరగతి SvaNfaLowering final {
    Ast_కణుపు_Module* స్థిరరాశి m_modp;  // Module to add state vars and always blocks to
    Ast_కణుపు_DType* స్థిరరాశి m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_పేర్లు m_పేర్లు{"__Vnfa"};
    కొలత_t m_stat_Delay_Ring_అంచు_Visits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    నిర్మాణం క్రింది_Ctx final {
        File_పంక్తి* flp;  // Source location for generated AST
        Ast_Sen_వృక్షం* senTreep;  // Clock sensitivity tree
        Ast_కణుపు_సమాసం* disableExprp;  // disable iff expression (may be nullptr)
        Ast_కణుపు_సమాసం* సరిపోలిక_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        Sva_లేఖాచిత్రం& లేఖాచిత్రం;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    స్థిర Ast_కణుపు_సమాసం* build_సరిపోలిక_Now(File_పంక్తి* flp, Ast_కణుపు_సమాసం* stateExprp, Ast_కణుపు_సమాసం* condp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి statep = stateExprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        ఒకవేళ (!condp) తిరిగి statep;
        తిరిగి కొత్త AstLogAnd{flp, statep, sampled(condp->clone_వృక్షం_శుద్ధ(అబద్ధం))};
    }
    స్థిర Ast_కణుపు_సమాసం* andCond(File_పంక్తి* flp, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* condp) {
        ఒకవేళ (!condp) తిరిగి exprp;
        తిరిగి కొత్త AstLogAnd{flp, exprp, condp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    స్థిర Ast_కణుపు_సమాసం* orExprs(File_పంక్తి* flp, Ast_కణుపు_సమాసం* ap, Ast_కణుపు_సమాసం* bp) {
        ఒకవేళ (!ap) తిరిగి bp;
        తిరిగి కొత్త AstLogOr{flp, ap, bp};
    }
    స్థిర Ast_కణుపు_సమాసం* కూడు_Countp(File_పంక్తి* స్థిరరాశి flp, Ast_కణుపు_సమాసం* స్థిరరాశి మొత్తం_Countp,
                                  Ast_కణుపు_సమాసం* స్థిరరాశి contributionp,
                                  Ast_కణుపు_సమాసం* స్థిరరాశి enablep = శూన్యసూచిక) {
        // contribution = enable ? contribution : 0;
        Ast_కణుపు_సమాసం* స్థిరరాశి enabledContributionp
            = enablep ? కొత్త AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        ఒకవేళ (!మొత్తం_Countp) తిరిగి enabledContributionp;
        తిరిగి కొత్త Ast_కూడు{flp, మొత్తం_Countp, enabledContributionp};
    }
    స్థిర Ast_కణుపు_సమాసం* gateCheckedCountp(File_పంక్తి* స్థిరరాశి flp, Ast_కణుపు_సమాసం* స్థిరరాశి countp,
                                          Ast_కణుపు_సమాసం*& overflowp, Ast_కణుపు_సమాసం* స్థిరరాశి enablep) {
        ఒకవేళ (overflowp) {
            overflowp = కొత్త AstLogAnd{flp, enablep->clone_వృక్షం_శుద్ధ(అబద్ధం), overflowp};
        }
        తిరిగి కూడు_Countp(flp, శూన్యసూచిక, countp, enablep);
    }
    స్థిర Ast_కణుపు_సమాసం* కూడు_Checked_Countp(File_పంక్తి* స్థిరరాశి flp, Ast_కణుపు_సమాసం* స్థిరరాశి మొత్తం_Countp,
                                         Ast_కణుపు_సమాసం* స్థిరరాశి contributionp, Ast_కణుపు_సమాసం*& overflowp,
                                         Ast_కణుపు_సమాసం* స్థిరరాశి contributionOverflowp = శూన్యసూచిక) {
        ఒకవేళ (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        ఒకవేళ (!మొత్తం_Countp) తిరిగి contributionp;
        Ast_కణుపు_సమాసం* స్థిరరాశి oldTotalp = మొత్తం_Countp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_కణుపు_సమాసం* స్థిరరాశి resultp = కొత్త Ast_కూడు{flp, మొత్తం_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, కొత్త AstLt{flp, resultp->clone_వృక్షం_శుద్ధ(అబద్ధం), oldTotalp});
        తిరిగి resultp;
    }
    AstIf* new_లెక్క_అధికప్రవాహం_Warning(File_పంక్తి* స్థిరరాశి flp, Ast_కణుపు_సమాసం* స్థిరరాశి condp) స్థిరరాశి {
        AstDisplay* స్థిరరాశి dispp = కొత్త AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            శూన్యసూచిక, శూన్యసూచిక};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        తిరిగి ifp;
    }
    స్థిర Ast_కణుపు_సమాసం* kill_చురుకైన(క్రింది_Ctx& c) {
        తిరిగి కొత్త AstNeq{c.flp, కొత్త AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    స్థిర Ast_కణుపు_సమాసం* not_Kill_చురుకైన(క్రింది_Ctx& c) { తిరిగి కొత్త AstLogNot{c.flp, kill_చురుకైన(c)}; }
    స్థిర Ast_కణుపు_సమాసం* gateNotKill(క్రింది_Ctx& c, Ast_కణుపు_సమాసం* exprp) {
        ఒకవేళ (!exprp) తిరిగి శూన్యసూచిక;
        తిరిగి కొత్త AstLogAnd{c.flp, exprp, not_Kill_చురుకైన(c)};
    }
    స్థిర Ast_కణుపు_సమాసం* తదుపరి_Ring_సూచీ(File_పంక్తి* flp, Ast_కణుపు_సమాసం* idxExprp, uint32_t కొలత) {
        స్థిరరాశి స్వయంచాలక u32Const = [flp](uint32_t విలువ) {
            తిరిగి కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, విలువ};
        };
        UASSERT_OBJ(కొలత > 0, idxExprp, "Ring size must be positive");
        ఒకవేళ (కొలత == 1) {
            VL_DO_DANGLING(idxExprp->delete_వృక్షం(), idxExprp);
            తిరిగి u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_కూడు* స్థిరరాశి addp = కొత్త Ast_కూడు{flp, idxExprp->clone_వృక్షం_శుద్ధ(అబద్ధం), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* స్థిరరాశి condp
            = కొత్త AstCond{flp, కొత్త AstEq{flp, idxExprp, u32Const(కొలత - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        తిరిగి condp;
    }
    స్థిర Ast_కణుపు_సమాసం* delay_Ring_స్థానం(File_పంక్తి* flp, Sva_State_శీర్షం* vtxp, Ast_కణుపు_సమాసం* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* స్థిరరాశి ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, ringp, access};
        ఒకవేళ (vtxp->datap()->delayRingCounts) తిరిగి కొత్త AstArraySel{flp, refp, idxExprp};
        తిరిగి కొత్త AstSel{flp, refp, idxExprp, 1};
    }
    స్థిర Ast_కణుపు_సమాసం* delay_Ring_తల_లెక్క(File_పంక్తి* flp, Sva_State_శీర్షం* vtxp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి headp
            = కొత్త AstVarRef{flp, vtxp->datap()->delay_Ring_తల_Varp, VAccess::READ};
        ఒకవేళ (vtxp->datap()->delayRingCounts) తిరిగి headp;
        తిరిగి కొత్త AstExtend{flp, headp, vtxp->datap()->delay_Ring_సజీవ_లెక్క_Varp->dtypep()->వెడల్పు()};
    }
    స్థిర Ast_కణుపు_సమాసం* delay_Ring_At_చివరి_సూచీ(File_పంక్తి* స్థిరరాశి flp, AstVar* స్థిరరాశి idxp,
                                             స్థిరరాశి uint32_t కొలత) {
        తిరిగి కొత్త AstEq{flp, కొత్త AstVarRef{flp, idxp, VAccess::READ},
                         కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, 32, కొలత - 1}};
    }
    స్థిర Ast_కణుపు_సమాసం* delay_Ring_Output_లెక్క(File_పంక్తి* స్థిరరాశి flp, Sva_State_శీర్షం* స్థిరరాశి vtxp) {
        AstVar* స్థిరరాశి idxp = vtxp->datap()->delayRingIdxVarp;
        స్థిరరాశి uint32_t కొలత = vtxp->m_delay_Ring_కొలత;
        Ast_కణుపు_సమాసం* outgoingValidp
            = కొత్త AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        ఒకవేళ (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = కొత్త AstLogOr{flp, outgoingValidp, delay_Ring_At_చివరి_సూచీ(flp, idxp, కొలత)};
        }
        AstVar* స్థిరరాశి సజీవ_లెక్క_Varp = vtxp->datap()->delay_Ring_సజీవ_లెక్క_Varp;
        తిరిగి కొత్త AstCond{flp, outgoingValidp, delay_Ring_తల_లెక్క(flp, vtxp),
                           newTypedConstp(flp, సజీవ_లెక్క_Varp->dtypep(), 0)};
    }
    స్థిర Ast_కణుపు_సమాసం* delay_Ring_Has_సజీవ_Threadsp(File_పంక్తి* స్థిరరాశి flp,
                                                 AstVar* స్థిరరాశి సజీవ_లెక్క_Varp) {
        // active = live_count != 0;
        తిరిగి కొత్త AstNeq{flp, కొత్త AstVarRef{flp, సజీవ_లెక్క_Varp, VAccess::READ},
                          newTypedConstp(flp, సజీవ_లెక్క_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    నిర్మాణం Signal_సమితి final {
        Ast_కణుపు_సమాసం* terminalActivep = శూన్యసూచిక;  // OR of all successful terminal matches
        Ast_కణుపు_సమాసం* సరిపోలిక_Countp = శూన్యసూచిక;  // NFA paths completing the sequence this tick
        Ast_కణుపు_సమాసం* సరిపోలిక_లెక్క_Overflowp = శూన్యసూచిక;  // Match count overflow this tick
        Ast_కణుపు_సమాసం* rejectBasep = శూన్యసూచిక;  // Reject when a terminal match fails
        Ast_కణుపు_సమాసం* required_అడుగు_Rejectp = శూన్యసూచిక;  // Per-source reject from rejectOnFail Links
        Ast_కణుపు_సమాసం* throughoutRejectp = శూన్యసూచిక;  // Reject when a throughout guard drops
        Ast_కణుపు_సమాసం* threadFailCountp = శూన్యసూచిక;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    స్థిర తార్కిక isAsyncDisable(స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి disableExprp) {
        తిరిగి disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](స్థిరరాశి AstSampled*) { తిరిగి నిజం; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    శూన్య కూడు_State_Always(క్రింది_Ctx& c, Ast_కణుపు* bodyp, Ast_కణుపు* clearp) {
        m_modp->కూడు_Stmtsp(
            కొత్త AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_వృక్షం(అబద్ధం), bodyp});
        ఒకవేళ (!clearp) తిరిగి;
        m_modp->కూడు_Stmtsp(కొత్త AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            కొత్త Ast_Sen_వృక్షం{c.flp, కొత్త Ast_Sen_అంశం{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    శూన్య emit_State_రిజిస్టరు_Nba(క్రింది_Ctx& c) {
        స్థిరరాశి తార్కిక asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_కణుపు* bodyp = శూన్యసూచిక;
        Ast_కణుపు* clearp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtxr : c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి vtxp = static_cast<స్థిరరాశి Sva_State_శీర్షం*>(&vtxr);
            ఒకవేళ (!vtxp->datap()->stateVarp) కొనసాగు;

            Ast_కణుపు_సమాసం* తదుపరి_Statep = శూన్యసూచిక;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& edger : vtxp->లోపల_అంచులు()) {
                స్థిరరాశి Sva_Trans_అంచు& tedger = static_cast<స్థిరరాశి Sva_Trans_అంచు&>(edger);
                ఒకవేళ (!tedger.m_consumes_చక్రం) కొనసాగు;
                స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_కణుపు_సమాసం* srcSigp = fromVtxp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                ఒకవేళ (c.disableExprp) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి notDisp
                        = కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    srcSigp = కొత్త AstLogAnd{c.flp, srcSigp, notDisp};
                }
                తదుపరి_Statep = orExprs(c.flp, తదుపరి_Statep, srcSigp);
            }

            UASSERT_OBJ(తదుపరి_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            తదుపరి_Statep = gateNotKill(c, తదుపరి_Statep);

            Ast_కేటాయించు_Dly* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు_Dly{
                c.flp, కొత్త AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, తదుపరి_Statep};
            bodyp = Ast_కణుపు::కూడు_తదుపరి_Null(bodyp, assignp);
            ఒకవేళ (asyncDisable) {
                clearp = Ast_కణుపు::కూడు_తదుపరి_Null(
                    clearp,
                    కొత్త Ast_కేటాయించు_Dly{
                        c.flp, కొత్త AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        కొత్త AstConst{c.flp, AstConst::బిట్టు_False{}}});
            }
        }
        ఒకవేళ (bodyp) కూడు_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    శూన్య emitDelayRingUpdates(క్రింది_Ctx& c, స్థిరరాశి తార్కిక లెక్క_Matches) {
        స్థిరరాశి తార్కిక asyncDisable = isAsyncDisable(c.disableExprp);
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            Sva_State_శీర్షం* స్థిరరాశి vtxp = static_cast<Sva_State_శీర్షం*>(&vtxr);
            ఒకవేళ (!vtxp->datap()->delayRingVarp) కొనసాగు;
            AstVar* స్థిరరాశి idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* స్థిరరాశి సజీవ_లెక్క_Varp = vtxp->datap()->delay_Ring_సజీవ_లెక్క_Varp;
            AstVar* స్థిరరాశి wrappedp = vtxp->datap()->delayRingWrappedVarp;
            స్థిరరాశి uint32_t కొలత = static_cast<uint32_t>(vtxp->m_delay_Ring_కొలత);

            Ast_కణుపు_సమాసం* incomingp = శూన్యసూచిక;
            Ast_కణుపు_సమాసం* incomingOverflowp = శూన్యసూచిక;
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& edger : vtxp->లోపల_అంచులు()) {
                ++m_stat_Delay_Ring_అంచు_Visits;
                స్థిరరాశి Sva_Trans_అంచు& tedger = static_cast<స్థిరరాశి Sva_Trans_అంచు&>(edger);
                UASSERT_OBJ(tedger.m_consumes_చక్రం == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!లెక్క_Matches || fromVtxp->datap()->state_లెక్క_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_కణుపు_సమాసం* contribp
                    = లెక్క_Matches
                          ? fromVtxp->datap()->state_లెక్క_Sigp->clone_వృక్షం_శుద్ధ(అబద్ధం)
                          : కొత్త AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                          m_u32DTypep->వెడల్పు()};
                Ast_కణుపు_సమాసం* contribOverflowp
                    = లెక్క_Matches && fromVtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp
                          ? fromVtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp->clone_వృక్షం_శుద్ధ(అబద్ధం)
                          : శూన్యసూచిక;
                ఒకవేళ (tedger.m_condp) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి enablep = tedger.m_condp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    contribp = లెక్క_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : కూడు_Countp(c.flp, శూన్యసూచిక, contribp, enablep);
                }
                ఒకవేళ (c.disableExprp) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి notDisp
                        = కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                    contribp = లెక్క_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : కూడు_Countp(c.flp, శూన్యసూచిక, contribp, notDisp);
                }
                incomingp = లెక్క_Matches ? కూడు_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : కూడు_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            స్థిరరాశి తార్కిక storesCounts = vtxp->datap()->delayRingCounts;
            Ast_కణుపు_సమాసం* స్థిరరాశి incomingCountp
                = storesCounts
                      ? incomingp->clone_వృక్షం_శుద్ధ(అబద్ధం)
                      : కొత్త AstExtend{c.flp,
                                      కొత్త AstNeq{c.flp, incomingp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->వెడల్పు()};
            Ast_కణుపు_సమాసం* స్థిరరాశి storedIncomingp
                = storesCounts ? incomingp
                               : కొత్త AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_కేటాయించు* స్థిరరాశి writeIncomingp = కొత్త Ast_కేటాయించు{
                c.flp,
                delay_Ring_స్థానం(c.flp, vtxp, కొత్త AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_కణుపు* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_కణుపు_సమాసం* స్థిరరాశి outgoingp = delay_Ring_Output_లెక్క(c.flp, vtxp);
            Ast_కణుపు_సమాసం* స్థిరరాశి afterOutgoingp
                = కొత్త AstSub{c.flp, కొత్త AstVarRef{c.flp, సజీవ_లెక్క_Varp, VAccess::READ}, outgoingp};
            Ast_కణుపు_సమాసం* స్థిరరాశి oldAfterOutgoingp = afterOutgoingp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            Ast_కూడు* స్థిరరాశి తదుపరి_సజీవ_Countp = కొత్త Ast_కూడు{c.flp, afterOutgoingp, incomingCountp};
            ఒకవేళ (storesCounts) {
                Ast_కణుపు_సమాసం* స్థిరరాశి సజీవ_Overflowp
                    = కొత్త AstLt{c.flp, తదుపరి_సజీవ_Countp->clone_వృక్షం_శుద్ధ(అబద్ధం), oldAfterOutgoingp};
                Ast_కణుపు_సమాసం* స్థిరరాశి overflowp = orExprs(c.flp, incomingOverflowp, సజీవ_Overflowp);
                updateBodyp = new_లెక్క_అధికప్రవాహం_Warning(c.flp, overflowp);
                updateBodyp->కూడు_తదుపరి(writeIncomingp);
            } లేకపోతే {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_వృక్షం(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_కణుపు_సమాసం* తదుపరి_తల_Idxp
                = తదుపరి_Ring_సూచీ(c.flp, కొత్త AstVarRef{c.flp, idxp, VAccess::READ}, కొలత);
            ఒకవేళ (!vtxp->m_isFixedDelayRing) {
                తదుపరి_తల_Idxp = తదుపరి_Ring_సూచీ(c.flp, తదుపరి_తల_Idxp, కొలత);
            }
            updateBodyp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు_Dly{
                c.flp, కొత్త AstVarRef{c.flp, vtxp->datap()->delay_Ring_తల_Varp, VAccess::WRITE},
                delay_Ring_స్థానం(c.flp, vtxp, తదుపరి_తల_Idxp)});
            updateBodyp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు_Dly{
                c.flp, కొత్త AstVarRef{c.flp, సజీవ_లెక్క_Varp, VAccess::WRITE}, తదుపరి_సజీవ_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->కూడు_తదుపరి(
                కొత్త Ast_కేటాయించు_Dly{c.flp, కొత్త AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 కొత్త AstLogOr{c.flp, కొత్త AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_చివరి_సూచీ(c.flp, idxp, కొలత)}});
            // idx <= next_idx;
            updateBodyp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు_Dly{
                c.flp, కొత్త AstVarRef{c.flp, idxp, VAccess::WRITE},
                తదుపరి_Ring_సూచీ(c.flp, కొత్త AstVarRef{c.flp, idxp, VAccess::READ}, కొలత)});
            ఒకవేళ (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = కొత్త AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)),
                    updateBodyp};
            }

            Ast_కణుపు_సమాసం* clearCondp = kill_చురుకైన(c);
            ఒకవేళ (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_వృక్షం_శుద్ధ(అబద్ధం)));
            }
            ఒకవేళ (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_వృక్షం_శుద్ధ(అబద్ధం)));
            }
            ఒకవేళ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం));
            }
            Ast_కణుపు_సమాసం* guardp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి cp : vtxp->m_throughoutConds) {
                Ast_కణుపు_సమాసం* స్థిరరాశి sampledp = sampled(cp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                guardp = guardp ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            ఒకవేళ (guardp) clearCondp = orExprs(c.flp, clearCondp, కొత్త AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_కేటాయించు_Dly* స్థిరరాశి clearCountp
                = కొత్త Ast_కేటాయించు_Dly{c.flp, కొత్త AstVarRef{c.flp, సజీవ_లెక్క_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, సజీవ_లెక్క_Varp->dtypep(), 0)};
            clearCountp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు_Dly{c.flp,
                                                  కొత్త AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  కొత్త AstConst{c.flp, AstConst::బిట్టు_False{}}});
            clearCountp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు_Dly{c.flp,
                                                  కొత్త AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = కొత్త AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            కూడు_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_వృక్షం(నిజం) : శూన్యసూచిక);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    శూన్య emit_And_Combiner_పూర్తి_Latch_Nba(క్రింది_Ctx& c) {
        స్థిరరాశి తార్కిక asyncDisable = isAsyncDisable(c.disableExprp);
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtxr : c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి avp = static_cast<స్థిరరాశి Sva_State_శీర్షం*>(&vtxr);
            ఒకవేళ (!avp->datap()->పూర్తి_LVarp) కొనసాగు;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి lhsTermp = avp->m_andLhsTermp;
            స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            Ast_కేటాయించు_Dly* స్థిరరాశి clearLp = కొత్త Ast_కేటాయించు_Dly{
                c.flp, కొత్త AstVarRef{c.flp, avp->datap()->పూర్తి_LVarp, VAccess::WRITE},
                కొత్త AstConst{c.flp, AstConst::బిట్టు_False{}}};
            Ast_కేటాయించు_Dly* స్థిరరాశి clearRp = కొత్త Ast_కేటాయించు_Dly{
                c.flp, కొత్త AstVarRef{c.flp, avp->datap()->పూర్తి_RVarp, VAccess::WRITE},
                కొత్త AstConst{c.flp, AstConst::బిట్టు_False{}}};
            clearLp->కూడు_తదుపరి(clearRp);

            Ast_కణుపు_సమాసం* స్థిరరాశి సరిపోలిక_LNowp
                = build_సరిపోలిక_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_కణుపు_సమాసం* స్థిరరాశి సరిపోలిక_RNowp
                = build_సరిపోలిక_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_కణుపు_సమాసం* gateLp = సరిపోలిక_LNowp;
            Ast_కణుపు_సమాసం* gateRp = సరిపోలిక_RNowp;
            ఒకవేళ (c.disableExprp) {
                Ast_కణుపు_సమాసం* స్థిరరాశి notDisLp
                    = కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                gateLp = కొత్త AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_కణుపు_సమాసం* స్థిరరాశి notDisRp
                    = కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                gateRp = కొత్త AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_కేటాయించు_Dly* స్థిరరాశి సమితి_Lp = కొత్త Ast_కేటాయించు_Dly{
                c.flp, కొత్త AstVarRef{c.flp, avp->datap()->పూర్తి_LVarp, VAccess::WRITE},
                కొత్త AstConst{c.flp, AstConst::బిట్టు_True{}}};
            AstIf* స్థిరరాశి సమితి_LIfp = కొత్త AstIf{c.flp, gateLp, సమితి_Lp, శూన్యసూచిక};
            Ast_కేటాయించు_Dly* స్థిరరాశి సమితి_Rp = కొత్త Ast_కేటాయించు_Dly{
                c.flp, కొత్త AstVarRef{c.flp, avp->datap()->పూర్తి_RVarp, VAccess::WRITE},
                కొత్త AstConst{c.flp, AstConst::బిట్టు_True{}}};
            AstIf* స్థిరరాశి సమితి_RIfp = కొత్త AstIf{c.flp, gateRp, సమితి_Rp, శూన్యసూచిక};
            సమితి_LIfp->కూడు_తదుపరి(సమితి_RIfp);

            Ast_కణుపు_సమాసం* clearCondp = కొత్త AstLogOr{c.flp, kill_చురుకైన(c),
                                                   avp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            ఒకవేళ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం));
            }
            AstIf* స్థిరరాశి topp = కొత్త AstIf{c.flp, clearCondp, clearLp, సమితి_LIfp};
            కూడు_State_Always(c, topp, asyncDisable ? clearLp->clone_వృక్షం(నిజం) : శూన్యసూచిక);
        }
    }

    శూన్య emitKillAckNba(క్రింది_Ctx& c) {
        Ast_కేటాయించు_Dly* స్థిరరాశి ackp
            = కొత్త Ast_కేటాయించు_Dly{c.flp, కొత్త AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->కూడు_Stmtsp(
            కొత్త AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_వృక్షం(అబద్ధం), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    శూన్య compute_Terminal_సరిపోలిక_And_Reject(క్రింది_Ctx& c, Signal_సమితి& sigs, స్థిరరాశి తార్కిక need_సరిపోలిక_లెక్క) {
        కొరకు (స్థిరరాశి Sva_Trans_అంచు* స్థిరరాశి tedgep : c.లేఖాచిత్రం.all_అంచులు()) {
            ఒకవేళ (tedgep->గమ్యం_Vtxp() != c.లేఖాచిత్రం.m_సరిపోలిక_Vertexp) కొనసాగు;
            Sva_State_శీర్షం* స్థిరరాశి fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_కణుపు_సమాసం* srcSigp = fromVtxp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            ఒకవేళ (need_సరిపోలిక_లెక్క) {
                UASSERT_OBJ(fromVtxp->datap()->state_లెక్క_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_కణుపు_సమాసం* contributionp
                    = fromVtxp->datap()->state_లెక్క_Sigp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                Ast_కణుపు_సమాసం* contributionOverflowp
                    = fromVtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp
                          ? fromVtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp->clone_వృక్షం_శుద్ధ(అబద్ధం)
                          : శూన్యసూచిక;
                ఒకవేళ (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                }
                sigs.సరిపోలిక_Countp
                    = కూడు_Checked_Countp(c.flp, sigs.సరిపోలిక_Countp, contributionp,
                                       sigs.సరిపోలిక_లెక్క_Overflowp, contributionOverflowp);
            }

            ఒకవేళ (fromVtxp->m_delay_Ring_కొలత && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_కణుపు_సమాసం* expireContribp = delay_Ring_Output_లెక్క(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_వృక్షం(), srcSigp);
            } లేకపోతే ఒకవేళ (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } లేకపోతే {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.లేఖాచిత్రం.m_సరిపోలిక_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_కణుపు_సమాసం* newThroughoutThreadFailCountp(క్రింది_Ctx& c, AstVar* స్థిరరాశి delay_Ring_సజీవ_లెక్క_Varp,
                                               Ast_కణుపు_సమాసం* స్థిరరాశి stateExprp,
                                               Ast_కణుపు_సమాసం* స్థిరరాశి enablep) {
        Ast_కణుపు_సమాసం* స్థిరరాశి చురుకైన_Thread_Countp
            = delay_Ring_సజీవ_లెక్క_Varp
                  ? static_cast<Ast_కణుపు_సమాసం*>(
                        కొత్త AstVarRef{c.flp, delay_Ring_సజీవ_లెక్క_Varp, VAccess::READ})
                  : కొత్త AstExtend{c.flp, stateExprp->clone_వృక్షం_శుద్ధ(అబద్ధం), m_u32DTypep->వెడల్పు()};
        తిరిగి కూడు_Countp(c.flp, శూన్యసూచిక, చురుకైన_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    శూన్య computeThroughoutReject(క్రింది_Ctx& c, Signal_సమితి& sigs, స్థిరరాశి తార్కిక need_Thread_Fail_లెక్క) {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtxr : c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి vtxp = static_cast<స్థిరరాశి Sva_State_శీర్షం*>(&vtxr);
            స్థిరరాశి స్వయంచాలక& conds = vtxp->m_throughoutConds;
            ఒకవేళ (conds.ఖాళీలు() && !vtxp->m_abortRejectp) కొనసాగు;
            ఒకవేళ (vtxp->m_isAndCombiner) కొనసాగు;
            Ast_కణుపు_సమాసం* stateExprp = శూన్యసూచిక;
            ఒకవేళ (vtxp->datap()->stateVarp) {
                stateExprp = కొత్త AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } లేకపోతే ఒకవేళ (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_సజీవ_Threadsp(c.flp, vtxp->datap()->delay_Ring_సజీవ_లెక్క_Varp);
            } లేకపోతే {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            }
            Ast_కణుపు_సమాసం* guardp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి cp : conds) {
                Ast_కణుపు_సమాసం* స్థిరరాశి sp = sampled(cp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                guardp = guardp ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            ఒకవేళ (vtxp->m_abortRejectp) {
                Ast_కణుపు_సమాసం* స్థిరరాశి notAbortp
                    = కొత్త AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_వృక్షం_శుద్ధ(అబద్ధం))};
                guardp = guardp
                             ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_కణుపు_సమాసం* rejectCondp = కొత్త AstLogNot{c.flp, guardp};
            ఒకవేళ (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_కణుపు_సమాసం* స్థిరరాశి notAbortClearp
                    = కొత్త AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_వృక్షం_శుద్ధ(అబద్ధం))};
                rejectCondp = కొత్త AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                ఒకవేళ (vtxp->m_abortRejectp) {
                    rejectCondp = కొత్త AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_వృక్షం_శుద్ధ(అబద్ధం))};
                }
            }
            ఒకవేళ (need_Thread_Fail_లెక్క) {
                Ast_కణుపు_సమాసం* స్థిరరాశి contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_సజీవ_లెక్క_Varp,
                                                    stateExprp, rejectCondp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                sigs.threadFailCountp = కూడు_Countp(c.flp, sigs.threadFailCountp, contributionp);
                ఒకవేళ (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_కణుపు_సమాసం* స్థిరరాశి abortAndAdvancep = కొత్త AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_వృక్షం_శుద్ధ(అబద్ధం)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_వృక్షం_శుద్ధ(అబద్ధం))};
                    Ast_కణుపు_సమాసం* స్థిరరాశి సరిపోలిక_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_సజీవ_లెక్క_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = కూడు_Countp(c.flp, sigs.threadFailCountp, సరిపోలిక_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             కొత్త AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_సమితి compute_సంకేతాలు(క్రింది_Ctx& c, స్థిరరాశి తార్కిక need_Thread_Fail_లెక్క,
                             స్థిరరాశి తార్కిక need_Throughout_Thread_Fail_లెక్క, స్థిరరాశి తార్కిక need_సరిపోలిక_లెక్క) {
        Signal_సమితి sigs;

        compute_Terminal_సరిపోలిక_And_Reject(c, sigs, need_సరిపోలిక_లెక్క);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        కొరకు (స్థిరరాశి Sva_Trans_అంచు* స్థిరరాశి tedgep : c.లేఖాచిత్రం.all_అంచులు()) {
            ఒకవేళ (!tedgep->m_rejectOnFail) కొనసాగు;
            స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_కణుపు_సమాసం* స్థిరరాశి srcSigp = fromVtxp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;
            ఒకవేళ (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                ఒకవేళ (tedgep->m_condp) {
                    condp = కొత్త AstLogOr{c.flp, condp, tedgep->m_condp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                }
            } లేకపోతే {
                condp = tedgep->m_condp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            }
            Ast_కణుపు_సమాసం* స్థిరరాశి notCondp = కొత్త AstLogNot{c.flp, condp};
            Ast_కణుపు_సమాసం* స్థిరరాశి rawFailp = కొత్త AstLogAnd{c.flp, srcSigp, notCondp};
            ఒకవేళ (need_Thread_Fail_లెక్క) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = కూడు_Countp(
                    c.flp, sigs.threadFailCountp,
                    కొత్త AstExtend{c.flp, rawFailp->clone_వృక్షం_శుద్ధ(అబద్ధం), m_u32DTypep->వెడల్పు()});
            }
            Ast_కణుపు_సమాసం* స్థిరరాశి failp = gateNotKill(c, rawFailp);
            sigs.required_అడుగు_Rejectp = orExprs(c.flp, sigs.required_అడుగు_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_లెక్క);
        ఒకవేళ (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = కూడు_Countp(c.flp, శూన్యసూచిక, sigs.threadFailCountp, not_Kill_చురుకైన(c));
        }
        ఒకవేళ (sigs.సరిపోలిక_Countp) {
            ఒకవేళ (c.సరిపోలిక_Condp) {
                sigs.సరిపోలిక_Countp
                    = gateCheckedCountp(c.flp, sigs.సరిపోలిక_Countp, sigs.సరిపోలిక_లెక్క_Overflowp,
                                        sampled(c.సరిపోలిక_Condp->clone_వృక్షం_శుద్ధ(అబద్ధం)));
            }
            sigs.సరిపోలిక_Countp = gateCheckedCountp(c.flp, sigs.సరిపోలిక_Countp, sigs.సరిపోలిక_లెక్క_Overflowp,
                                                 not_Kill_చురుకైన(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            Sva_State_శీర్షం* స్థిరరాశి vtxp = static_cast<Sva_State_శీర్షం*>(&vtxr);
            Ast_కణుపు_సమాసం*& sigp = vtxp->datap()->stateSigp;
            ఒకవేళ (sigp) VL_DO_DANGLING(sigp->delete_వృక్షం(), sigp);
            Ast_కణుపు_సమాసం*& countp = vtxp->datap()->state_లెక్క_Sigp;
            ఒకవేళ (countp) VL_DO_DANGLING(countp->delete_వృక్షం(), countp);
            Ast_కణుపు_సమాసం*& overflowp = vtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp;
            ఒకవేళ (overflowp) VL_DO_DANGLING(overflowp->delete_వృక్షం(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        ఒకవేళ (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_కణుపు_సమాసం* స్థిరరాశి notTermp
                = కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
            sigs.terminalActivep = కొత్త AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            ఒకవేళ (sigs.rejectBasep) {
                Ast_కణుపు_సమాసం* స్థిరరాశి notDisp
                    = కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                sigs.rejectBasep = కొత్త AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            ఒకవేళ (sigs.throughoutRejectp) {
                Ast_కణుపు_సమాసం* స్థిరరాశి notDisp
                    = కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                sigs.throughoutRejectp = కొత్త AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            ఒకవేళ (sigs.required_అడుగు_Rejectp) {
                Ast_కణుపు_సమాసం* స్థిరరాశి notDisp
                    = కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                sigs.required_అడుగు_Rejectp = కొత్త AstLogAnd{c.flp, sigs.required_అడుగు_Rejectp, notDisp};
            }
            ఒకవేళ (sigs.సరిపోలిక_Countp) {
                sigs.సరిపోలిక_Countp = gateCheckedCountp(
                    c.flp, sigs.సరిపోలిక_Countp, sigs.సరిపోలిక_లెక్క_Overflowp,
                    కొత్త AstLogNot{c.flp, c.disableExprp->clone_వృక్షం_శుద్ధ(అబద్ధం)});
            }
        }

        ఒకవేళ (sigs.సరిపోలిక_లెక్క_Overflowp) {
            m_modp->కూడు_Stmtsp(
                కొత్త AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_వృక్షం(అబద్ధం),
                              new_లెక్క_అధికప్రవాహం_Warning(c.flp, sigs.సరిపోలిక_లెక్క_Overflowp)});
            sigs.సరిపోలిక_లెక్క_Overflowp = శూన్యసూచిక;
        }

        తిరిగి sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    శూన్య resolveLinks(క్రింది_Ctx& c, Ast_కణుపు_సమాసం* triggerExprp, స్థిరరాశి తార్కిక లెక్క_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.లేఖాచిత్రం.m_ప్రారంభం_Vertexp->datap()->stateSigp = triggerExprp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        c.లేఖాచిత్రం.m_ప్రారంభం_Vertexp->datap()->state_లెక్క_Sigp
            = కొత్త AstExtend{c.flp, triggerExprp->clone_వృక్షం_శుద్ధ(అబద్ధం), m_u32DTypep->వెడల్పు()};
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            Sva_State_శీర్షం* స్థిరరాశి vtxp = static_cast<Sva_State_శీర్షం*>(&vtxr);
            ఒకవేళ (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = కొత్త AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_లెక్క_Sigp = కొత్త AstExtend{
                    c.flp, కొత్త AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->వెడల్పు()};
            } లేకపోతే ఒకవేళ (vtxp->datap()->delayRingVarp) {
                ఒకవేళ (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_కణుపు_సమాసం* స్థిరరాశి countp = delay_Ring_Output_లెక్క(c.flp, vtxp);
                    vtxp->datap()->stateSigp = కొత్త AstNeq{c.flp, countp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_లెక్క_Sigp = countp;
                } లేకపోతే {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_సజీవ_Threadsp(c.flp, vtxp->datap()->delay_Ring_సజీవ_లెక్క_Varp);
                    vtxp->datap()->state_లెక్క_Sigp = కొత్త AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_సజీవ_లెక్క_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        స్థిరరాశి కొలత_t passes = 2 * c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices().కొలత() + 2;
        కొరకు (కొలత_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
                Sva_State_శీర్షం* స్థిరరాశి vtxp = static_cast<Sva_State_శీర్షం*>(&vtxr);
                ఒకవేళ (!vtxp->m_isAndCombiner) కొనసాగు;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి lhsTermp = vtxp->m_andLhsTermp;
                స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి rhsTermp = vtxp->m_andRhsTermp;
                ఒకవేళ (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) కొనసాగు;
                Ast_కణుపు_సమాసం* స్థిరరాశి సరిపోలిక_Lp
                    = build_సరిపోలిక_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_కణుపు_సమాసం* స్థిరరాశి సరిపోలిక_Rp
                    = build_సరిపోలిక_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_కణుపు_సమాసం* స్థిరరాశి పూర్తి_LOrp = కొత్త AstLogOr{
                    c.flp, కొత్త AstVarRef{c.flp, vtxp->datap()->పూర్తి_LVarp, VAccess::READ}, సరిపోలిక_Lp};
                Ast_కణుపు_సమాసం* స్థిరరాశి పూర్తి_ROrp = కొత్త AstLogOr{
                    c.flp, కొత్త AstVarRef{c.flp, vtxp->datap()->పూర్తి_RVarp, VAccess::READ}, సరిపోలిక_Rp};
                Ast_కణుపు_సమాసం* స్థిరరాశి bothp = కొత్త AstLogAnd{c.flp, పూర్తి_LOrp, పూర్తి_ROrp};
                Ast_కణుపు_సమాసం* స్థిరరాశి oneNowp = కొత్త AstLogOr{c.flp, సరిపోలిక_Lp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                                          సరిపోలిక_Rp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
                ఒకవేళ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_వృక్షం(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = కొత్త AstLogAnd{c.flp, bothp, oneNowp};
                ఒకవేళ (vtxp->datap()->state_లెక్క_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_లెక్క_Sigp->delete_వృక్షం(),
                                   vtxp->datap()->state_లెక్క_Sigp);
                }
                vtxp->datap()->state_లెక్క_Sigp = కొత్త AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం), m_u32DTypep->వెడల్పు()};
            }

            కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : c.లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
                Sva_State_శీర్షం* స్థిరరాశి vtxp = static_cast<Sva_State_శీర్షం*>(&vtxr);
                ఒకవేళ (vtxp == c.లేఖాచిత్రం.m_ప్రారంభం_Vertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_సరిపోలిక
                    || vtxp->m_isRejectSink) {
                    కొనసాగు;
                }
                Ast_కణుపు_సమాసం* తదుపరి_Statep = శూన్యసూచిక;
                Ast_కణుపు_సమాసం* తదుపరి_State_Countp = శూన్యసూచిక;
                Ast_కణుపు_సమాసం* తదుపరి_State_లెక్క_Overflowp = శూన్యసూచిక;
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& er : vtxp->లోపల_అంచులు()) {
                    స్థిరరాశి Sva_Trans_అంచు& te = static_cast<స్థిరరాశి Sva_Trans_అంచు&>(er);
                    స్థిరరాశి Sva_State_శీర్షం* స్థిరరాశి fromVtxp = te.fromVtxp();
                    ఒకవేళ (!fromVtxp->datap()->stateSigp) కొనసాగు;
                    UASSERT_OBJ(fromVtxp->datap()->state_లెక్క_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_కణుపు_సమాసం* స్థిరరాశి contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_వృక్షం_శుద్ధ(అబద్ధం), te.m_condp);
                    తదుపరి_Statep = orExprs(c.flp, తదుపరి_Statep, contributionp);
                    Ast_కణుపు_సమాసం* లెక్క_Contributionp
                        = fromVtxp->datap()->state_లెక్క_Sigp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    Ast_కణుపు_సమాసం* లెక్క_Contribution_Overflowp
                        = లెక్క_Matches && fromVtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp
                              ? fromVtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp->clone_వృక్షం_శుద్ధ(అబద్ధం)
                              : శూన్యసూచిక;
                    ఒకవేళ (te.m_condp) {
                        Ast_కణుపు_సమాసం* స్థిరరాశి enablep = te.m_condp->clone_వృక్షం_శుద్ధ(అబద్ధం);
                        లెక్క_Contributionp
                            = లెక్క_Matches
                                  ? gateCheckedCountp(c.flp, లెక్క_Contributionp,
                                                      లెక్క_Contribution_Overflowp, enablep)
                                  : కూడు_Countp(c.flp, శూన్యసూచిక, లెక్క_Contributionp, enablep);
                    }
                    తదుపరి_State_Countp
                        = లెక్క_Matches
                              ? కూడు_Checked_Countp(c.flp, తదుపరి_State_Countp, లెక్క_Contributionp,
                                                 తదుపరి_State_లెక్క_Overflowp,
                                                 లెక్క_Contribution_Overflowp)
                              : కూడు_Countp(c.flp, తదుపరి_State_Countp, లెక్క_Contributionp);
                }
                ఒకవేళ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_వృక్షం(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = తదుపరి_Statep;
                ఒకవేళ (vtxp->datap()->state_లెక్క_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_లెక్క_Sigp->delete_వృక్షం(),
                                   vtxp->datap()->state_లెక్క_Sigp);
                }
                vtxp->datap()->state_లెక్క_Sigp = తదుపరి_State_Countp;
                ఒకవేళ (vtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp->delete_వృక్షం(),
                                   vtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp);
                }
                vtxp->datap()->state_లెక్క_అధికప్రవాహం_Sigp = తదుపరి_State_లెక్క_Overflowp;
            }
        }
    }

బహిరంగ:
    // Combine terminal/reject signals into final output expression.
    స్థిర Ast_కణుపు_సమాసం* assemble_ఫలితం(Ast_కణుపు_Cover_Or_Assert* స్థిరరాశి assertp, స్థిరరాశి తార్కిక negated,
                                       Ast_కణుపు_సమాసం* స్థిరరాశి సరిపోలిక_Condp, స్థిరరాశి Signal_సమితి& sigs,
                                       Ast_కణుపు_సమాసం** స్థిరరాశి బహిర్గతం_Matchpp) {
        File_పంక్తి* స్థిరరాశి flp = assertp->fileline();
        స్థిరరాశి తార్కిక isCover = VN_IS(assertp, Cover);
        Ast_కణుపు_సమాసం* స్థిరరాశి terminalActivep = sigs.terminalActivep;
        Ast_కణుపు_సమాసం* స్థిరరాశి rejectBasep = sigs.rejectBasep;
        Ast_కణుపు_సమాసం* స్థిరరాశి throughoutRejectp = sigs.throughoutRejectp;
        Ast_కణుపు_సమాసం* స్థిరరాశి required_అడుగు_Rejectp = sigs.required_అడుగు_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        ఒకవేళ (negated) {
            ఒకవేళ (isCover) {
                ఒకవేళ (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_వృక్షం(), terminalActivep);
                Ast_కణుపు_సమాసం* negRejectp = శూన్యసూచిక;
                ఒకవేళ (సరిపోలిక_Condp && rejectBasep) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి sampledCondp = sampled(సరిపోలిక_Condp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                    Ast_కణుపు_సమాసం* స్థిరరాశి notCondp = కొత్త AstLogNot{flp, sampledCondp};
                    negRejectp = కొత్త AstLogAnd{flp, rejectBasep, notCondp};
                } లేకపోతే ఒకవేళ (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_వృక్షం(), rejectBasep);
                }
                ఒకవేళ (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                ఒకవేళ (required_అడుగు_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_అడుగు_Rejectp);
                తిరిగి negRejectp ? negRejectp : కొత్త AstConst{flp, AstConst::బిట్టు_False{}};
            }
            // Negated assert/assume: output = !match.
            Ast_కణుపు_సమాసం* matchp = terminalActivep;
            ఒకవేళ (సరిపోలిక_Condp) {
                Ast_కణుపు_సమాసం* స్థిరరాశి sampledCondp = sampled(సరిపోలిక_Condp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                matchp = కొత్త AstLogAnd{flp, matchp, sampledCondp};
            }
            ఒకవేళ (బహిర్గతం_Matchpp) {
                Ast_కణుపు_సమాసం* notPMatchp = శూన్యసూచిక;
                ఒకవేళ (సరిపోలిక_Condp && rejectBasep) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి sampledCondp = sampled(సరిపోలిక_Condp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                    notPMatchp = కొత్త AstLogAnd{flp, rejectBasep->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                               కొత్త AstLogNot{flp, sampledCondp}};
                } లేకపోతే ఒకవేళ (rejectBasep) {
                    notPMatchp = rejectBasep->clone_వృక్షం_శుద్ధ(అబద్ధం);
                }
                ఒకవేళ (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                ఒకవేళ (required_అడుగు_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_అడుగు_Rejectp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                *బహిర్గతం_Matchpp = notPMatchp;
            }
            ఒకవేళ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_వృక్షం(), throughoutRejectp);
            ఒకవేళ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_వృక్షం(), rejectBasep);
            ఒకవేళ (required_అడుగు_Rejectp)
                VL_DO_DANGLING(required_అడుగు_Rejectp->delete_వృక్షం(), required_అడుగు_Rejectp);
            Ast_కణుపు_సమాసం* స్థిరరాశి ఫలితం_Exprp = కొత్త AstLogNot{flp, matchp};
            తిరిగి ఫలితం_Exprp;
        }
        ఒకవేళ (isCover) {
            ఒకవేళ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_వృక్షం(), throughoutRejectp);
            ఒకవేళ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_వృక్షం(), rejectBasep);
            ఒకవేళ (required_అడుగు_Rejectp)
                VL_DO_DANGLING(required_అడుగు_Rejectp->delete_వృక్షం(), required_అడుగు_Rejectp);
            ఒకవేళ (సరిపోలిక_Condp) {
                Ast_కణుపు_సమాసం* స్థిరరాశి sampledCondp = sampled(సరిపోలిక_Condp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                తిరిగి కొత్త AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            తిరిగి terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_కణుపు_సమాసం* rejectp = శూన్యసూచిక;
        ఒకవేళ (సరిపోలిక_Condp && rejectBasep) {
            Ast_కణుపు_సమాసం* స్థిరరాశి sampledCondp = sampled(సరిపోలిక_Condp->clone_వృక్షం_శుద్ధ(అబద్ధం));
            rejectp = కొత్త AstLogAnd{flp, rejectBasep, కొత్త AstLogNot{flp, sampledCondp}};
        } లేకపోతే ఒకవేళ (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_వృక్షం(), rejectBasep);
        }
        ఒకవేళ (బహిర్గతం_Matchpp) {
            Ast_కణుపు_సమాసం* సరిపోలిక_Exprp = terminalActivep->clone_వృక్షం_శుద్ధ(అబద్ధం);
            ఒకవేళ (సరిపోలిక_Condp) {
                Ast_కణుపు_సమాసం* స్థిరరాశి sp = sampled(సరిపోలిక_Condp->clone_వృక్షం_శుద్ధ(అబద్ధం));
                సరిపోలిక_Exprp = కొత్త AstLogAnd{flp, సరిపోలిక_Exprp, sp};
            }
            *బహిర్గతం_Matchpp = సరిపోలిక_Exprp;
        }
        ఒకవేళ (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_వృక్షం(), terminalActivep);
        ఒకవేళ (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        ఒకవేళ (required_అడుగు_Rejectp) rejectp = orExprs(flp, rejectp, required_అడుగు_Rejectp);
        ఒకవేళ (!rejectp) తిరిగి కొత్త AstConst{flp, AstConst::బిట్టు_True{}};
        Ast_కణుపు_సమాసం* స్థిరరాశి ఫలితం_Exprp = కొత్త AstLogNot{flp, rejectp};
        తిరిగి ఫలితం_Exprp;
    }

    స్పష్టమైన SvaNfaLowering(Ast_కణుపు_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::కూడు_Stat_కూడిక("Assertions, NFA delay ring edge visits", m_stat_Delay_Ring_అంచు_Visits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_సమితి క్రింది(Ast_కణుపు_Cover_Or_Assert* స్థిరరాశి assertp, Sva_లేఖాచిత్రం& లేఖాచిత్రం,
                    Ast_Sen_వృక్షం* స్థిరరాశి senTreep, Ast_కణుపు_సమాసం* స్థిరరాశి సరిపోలిక_Condp,
                    Ast_కణుపు_సమాసం* స్థిరరాశి disableExprp, స్థిరరాశి తార్కిక need_Thread_Fail_లెక్క,
                    స్థిరరాశి తార్కిక need_Throughout_Thread_Fail_లెక్క) {
        File_పంక్తి* స్థిరరాశి flp = assertp->fileline();
        AstCover* స్థిరరాశి coverp = VN_CAST(assertp, Cover);
        స్థిరరాశి తార్కిక isSeqEvent = coverp && coverp->isSeqEvent();
        స్థిరరాశి VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::అంతర్గత} : assertp->userType();
        స్థిరరాశి VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::అంతర్గత}
                         : assertp->directive();
        స్థిరరాశి std::string పునాది_Name = m_పేర్లు.get("");

        // Number vertices with sequential colors for array indexing.
        పూర్ణాంకం N = 0;
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) { vtxr.color(N++); }
        std::సదిశ<Sva_State_శీర్షం*> vtx(N, శూన్యసూచిక);
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtxr : లేఖాచిత్రం.m_లేఖాచిత్రం.vertices()) {
            vtx[vtxr.color()] = static_cast<Sva_State_శీర్షం*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::సదిశ<Sva_శీర్షం_దత్తాంశం> శీర్షం_దత్తాంశం(N);
        కొరకు (పూర్ణాంకం i = 0; i < N; ++i) vtx[i]->userp(&శీర్షం_దత్తాంశం[i]);

        // Identify registered vertices (targets of clocked edges).
        కొరకు (పూర్ణాంకం i = 0; i < N; ++i) {
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& edger : vtx[i]->బహిర్గతం_అంచులు()) {
                స్థిరరాశి Sva_Trans_అంచు& tedger = static_cast<స్థిరరాశి Sva_Trans_అంచు&>(edger);
                స్థిరరాశి పూర్ణాంకం గమ్యం_Idx = tedger.గమ్యం_Vtxp()->color();
                ఒకవేళ (tedger.m_consumes_చక్రం && tedger.గమ్యం_Vtxp() != లేఖాచిత్రం.m_సరిపోలిక_Vertexp
                    && !tedger.గమ్యం_Vtxp()->m_isRejectSink) {
                    vtx[గమ్యం_Idx]->datap()->needsReg = నిజం;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::సదిశ<తార్కిక> stateMayHaveMultiplicity(N);
        std::సదిశ<తార్కిక> ringNeedsMultiplicity(N);
        std::సదిశ<పూర్ణాంకం> multiplicityWork;
        స్థిరరాశి తార్కిక లెక్క_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        ఒకవేళ (లెక్క_Matches) {
            కొరకు (పూర్ణాంకం i = 0; i < N; ++i) {
                స్థిరరాశి తార్కిక గుణిజం_Inputs = vtx[i]->లోపల_అంచులు().కొలత() > 1;
                స్థిరరాశి తార్కిక isRing = vtx[i]->m_delay_Ring_కొలత;
                ringNeedsMultiplicity[i] = isRing && గుణిజం_Inputs;
                స్థిరరాశి తార్కిక startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : గుణిజం_Inputs && !vtx[i]->datap()->needsReg;
                ఒకవేళ (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = నిజం;
                    multiplicityWork.నెట్టు_వెనుక(i);
                }
            }
            కొరకు (కొలత_t wi = 0; wi < multiplicityWork.కొలత(); ++wi) {
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& edger : vtx[multiplicityWork[wi]]->బహిర్గతం_అంచులు()) {
                    స్థిరరాశి Sva_Trans_అంచు& tedger = static_cast<స్థిరరాశి Sva_Trans_అంచు&>(edger);
                    స్థిరరాశి పూర్ణాంకం ti = tedger.గమ్యం_Vtxp()->color();
                    స్థిరరాశి తార్కిక isRing = vtx[ti]->m_delay_Ring_కొలత;
                    ఒకవేళ (isRing) ringNeedsMultiplicity[ti] = నిజం;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    స్థిరరాశి తార్కిక stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    ఒకవేళ (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { కొనసాగు; }
                    stateMayHaveMultiplicity[ti] = నిజం;
                    multiplicityWork.నెట్టు_వెనుక(ti);
                }
            }
        }

        స్థిరరాశి తార్కిక asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_పంక్తి* స్థిరరాశి regFlp = asyncDisable ? కొత్త File_పంక్తి{flp} : flp;
        ఒకవేళ (asyncDisable) regFlp->warnOff(V3_Error_సంకేతం::MULTIDRIVEN, నిజం);

        AstVar* స్థిరరాశి killVarp
            = కొత్త AstVar{flp, VVarType::MODULETEMP, పునాది_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->కూడు_Stmtsp(killVarp);
        కొరకు (పూర్ణాంకం i = 0; i < N; ++i) {
            ఒకవేళ (vtx[i]->m_isAndCombiner) {
                స్థిరరాశి std::string పునాది = పునాది_Name + "__a" + std::గమ్యం_string(i);
                AstVar* స్థిరరాశి lp = కొత్త AstVar{regFlp, VVarType::MODULETEMP, పునాది + "_doneL",
                                              m_modp->కనుగొను_బిట్టు_DType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(lp);
                vtx[i]->datap()->పూర్తి_LVarp = lp;
                AstVar* స్థిరరాశి rp = కొత్త AstVar{regFlp, VVarType::MODULETEMP, పునాది + "_doneR",
                                              m_modp->కనుగొను_బిట్టు_DType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(rp);
                vtx[i]->datap()->పూర్తి_RVarp = rp;
                కొనసాగు;
            }
            ఒకవేళ (vtx[i]->m_delay_Ring_కొలత) {
                స్థిరరాశి std::string పునాది = పునాది_Name + "__d" + std::గమ్యం_string(i);
                స్థిరరాశి తార్కిక storesCounts = లెక్క_Matches && ringNeedsMultiplicity[i];
                Ast_కణుపు_DType* ringDTypep = శూన్యసూచిక;
                ఒకవేళ (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = కొత్త AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        కొత్త AstRange{flp, static_cast<పూర్ణాంకం>(vtx[i]->m_delay_Ring_కొలత - 1), 0}};
                    v3Global.rootp()->typeTablep()->కూడు_Typesp(ringDTypep);
                } లేకపోతే {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->కనుగొను_బిట్టు_DType(vtx[i]->m_delay_Ring_కొలత,
                                                      vtx[i]->m_delay_Ring_కొలత, VSigning::UNSIGNED);
                }
                AstVar* స్థిరరాశి ringp
                    = కొత్త AstVar{flp, VVarType::MODULETEMP, పునాది + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* స్థిరరాశి headp
                    = కొత్త AstVar{flp, VVarType::MODULETEMP, పునాది + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->కనుగొను_బిట్టు_DType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_తల_Varp = headp;
                // int unsigned idx;
                AstVar* స్థిరరాశి idxp
                    = కొత్త AstVar{regFlp, VVarType::MODULETEMP, పునాది + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* స్థిరరాశి సజీవ_లెక్క_Varp
                    = కొత్త AstVar{regFlp, VVarType::MODULETEMP, పునాది + "_liveCount", m_u32DTypep};
                సజీవ_లెక్క_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(సజీవ_లెక్క_Varp);
                vtx[i]->datap()->delay_Ring_సజీవ_లెక్క_Varp = సజీవ_లెక్క_Varp;
                AstVar* స్థిరరాశి wrappedp = కొత్త AstVar{regFlp, VVarType::MODULETEMP,
                                                    పునాది + "_wrapped", m_modp->కనుగొను_బిట్టు_DType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->కూడు_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                కొనసాగు;
            }
            ఒకవేళ (!vtx[i]->datap()->needsReg) కొనసాగు;
            ఒకవేళ (vtx[i] == లేఖాచిత్రం.m_ప్రారంభం_Vertexp || vtx[i]->m_is_సరిపోలిక) కొనసాగు;
            స్థిరరాశి std::string varName = పునాది_Name + "__s" + std::గమ్యం_string(i);
            AstVar* స్థిరరాశి varp
                = కొత్త AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->కనుగొను_బిట్టు_DType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->కూడు_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        క్రింది_Ctx c{flp,        senTreep,      disableExprp, సరిపోలిక_Condp,
                   assertType, directiveType, killVarp,     లేఖాచిత్రం};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_కణుపు_సమాసం* స్థిరరాశి triggerExprp
            = isSeqEvent ? కొత్త AstConst{flp, AstConst::బిట్టు_True{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, లెక్క_Matches);
        VL_DO_DANGLING(triggerExprp->delete_వృక్షం(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_రిజిస్టరు_Nba(c);
        emitDelayRingUpdates(c, లెక్క_Matches);
        emit_And_Combiner_పూర్తి_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        స్థిరరాశి Signal_సమితి sigs
            = compute_సంకేతాలు(c, need_Thread_Fail_లెక్క, need_Throughout_Thread_Fail_లెక్క, లెక్క_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_కణుపు_సమాసం* pendingp = శూన్యసూచిక;
        కొరకు (పూర్ణాంకం i = 0; i < N; ++i) {
            ఒకవేళ (!vtx[i]->m_strongPending) కొనసాగు;
            Ast_కణుపు_సమాసం* pendingExprp = శూన్యసూచిక;
            ఒకవేళ (vtx[i]->datap()->stateVarp) {
                pendingExprp = కొత్త AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } లేకపోతే ఒకవేళ (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_సజీవ_Threadsp(flp, vtx[i]->datap()->delay_Ring_సజీవ_లెక్క_Varp);
            } లేకపోతే {
                కొనసాగు;
            }
            ఒకవేళ (!pendingp) {
                pendingp = pendingExprp;
            } లేకపోతే {
                pendingp = కొత్త AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        ఒకవేళ (pendingp) {
            AstCExpr* స్థిరరాశి assertOnp
                = కొత్త AstCExpr{flp, AstCExpr::శుద్ధ{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* స్థిరరాశి dispp
                = కొత్త AstDisplay{flp, VDisplayType::DT_ERROR, "", శూన్యసూచిక, శూన్యసూచిక};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_కణుపు_Stmt* స్థిరరాశి firep = dispp;
            ఒకవేళ (v3Global.opt.stopFail()) firep->కూడు_తదుపరి(కొత్త AstStop{flp, అబద్ధం});
            m_modp->కూడు_Stmtsp(కొత్త AstFinal{flp, కొత్త AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        కొరకు (పూర్ణాంకం i = 0; i < N; ++i) vtx[i]->userp(శూన్యసూచిక);
        తిరిగి sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

తరగతి AssertNfaVisitor final : బహిరంగ VNVisitor {
    // STATE
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module being processed
    AstClocking* m_defaultClockingp = శూన్యసూచిక;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = శూన్యసూచిక;  // Default disable iff
    SvaNfaLowering* m_loweringp = శూన్యసూచిక;  // NFA-to-hardware lowering engine
    Ast_Sen_వృక్షం* m_sampled_విలువ_Clockp = శూన్యసూచిక;  // Inherited clock during scoped attachment
    V3_Unique_పేర్లు m_prop_Var_పేర్లు{"__Vpropvar"};  // Property-local variable names
    V3_Unique_పేర్లు m_prop_తాత్కాలిక_పేర్లు{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_పేర్లు m_fail_లెక్క_పేర్లు{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::సమితి<స్థిరరాశి AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    మూస <రకం_పేరు T_కణుపు>
    శూన్య సందర్శించు_Sampled_విలువ(T_కణుపు* స్థిరరాశి nodep) {
        ఒకవేళ (m_sampled_విలువ_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_విలువ_Clockp->clone_వృక్షం(నిజం));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    స్థిర శూన్య wire_సరిపోలిక_And_Mid_Sources(Sva_లేఖాచిత్రం& లేఖాచిత్రం, స్థిరరాశి Build_ఫలితం& ఫలితం, File_పంక్తి* flp) {
        లేఖాచిత్రం.create_సరిపోలిక_శీర్షం();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        ఒకవేళ (!ఫలితం.పదం_Is_Mid_విలీనం) { లేఖాచిత్రం.కూడు_Link(ఫలితం.పదం_Vertexp, లేఖాచిత్రం.m_సరిపోలిక_Vertexp); }
        కొరకు (Sva_State_శీర్షం* srcVtxp : ఫలితం.midSources) {
            Ast_కణుపు_సమాసం* condp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు_సమాసం* స్థిరరాశి tc : srcVtxp->m_throughoutConds) {
                Ast_కణుపు_సమాసం* స్థిరరాశి tcClone = tc->clone_వృక్షం_శుద్ధ(అబద్ధం);
                condp = condp ? కొత్త AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            లేఖాచిత్రం.కూడు_Link(srcVtxp, లేఖాచిత్రం.m_సరిపోలిక_Vertexp, condp);
            srcVtxp->m_isUnbounded = నిజం;
        }
    }

    స్థిర Ast_కణుపు_సమాసం* getSequenceBodyExprp(స్థిరరాశి AstSequence* seqp) {
        Ast_కణుపు* bodyp = seqp->stmtsp();
        అయితే (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        తిరిగి VN_CAST(bodyp, కణుపు_సమాసం);
    }

    స్థిర AstPropSpec* getPropertySpecp(స్థిరరాశి AstProperty* propp) {
        Ast_కణుపు* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        అయితే (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        తిరిగి VN_CAST(stmtp, PropSpec);
    }

    శూన్య inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             స్థిరరాశి AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        ఒకవేళ (!m_inliningProps.emplace(propyp).రెండవ) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            తిరిగి;  // LCOV_EXCL_LINE
        }
        నిర్మాణం Guard final {
            std::సమితి<స్థిరరాశి AstProperty*>& setr;
            స్థిరరాశి AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_వృక్షం(అబద్ధం);

        స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<స్థిరరాశి AstVar*, Ast_కణుపు_సమాసం*> పోర్టు_Map;
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            పోర్టు_Map[tconnect.మొదటి] = tconnect.రెండవ->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<స్థిరరాశి AstVar*, AstVar*> localVarMap;
        కొరకు (Ast_కణుపు* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (!varp->isIO()) {
                    స్థిరరాశి string newName = m_prop_Var_పేర్లు.get(varp);
                    AstVar* స్థిరరాశి newVarp = కొత్త AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->కూడు_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            స్థిరరాశి స్వయంచాలక పోర్టు_It = పోర్టు_Map.కనుగొను(refp->varp());
            ఒకవేళ (పోర్టు_It != పోర్టు_Map.చివర()) {
                refp->replaceWith(పోర్టు_It->రెండవ->clone_వృక్షం(అబద్ధం));
                VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
                తిరిగి;
            }
            స్థిరరాశి స్వయంచాలక localIt = localVarMap.కనుగొను(refp->varp());
            ఒకవేళ (localIt != localVarMap.చివర()) refp->varp(localIt->రెండవ);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            నెట్టు_Deletep(tconnect.రెండవ->exprp()->unlink_Fr_వెనుక());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        ఒకవేళ (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            నెట్టు_Deletep(propSpecp->disablep()->unlink_Fr_వెనుక());
        }
        ఒకవేళ (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_వెనుక());
        }

        ఒకవేళ (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            నెట్టు_Deletep(propSpecp->sensesp()->unlink_Fr_వెనుక());
        }
        ఒకవేళ (outerSpecp->sensesp()) {
            Ast_Sen_అంశం* స్థిరరాశి sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_వెనుక();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(నెట్టు_Deletep(outerSpecp), outerSpecp);
    }

    శూన్య inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_కణుపు_సమాసం* స్థిరరాశి clonedp = bodyExprp->clone_వృక్షం(అబద్ధం);

        స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<స్థిరరాశి AstVar*, Ast_కణుపు_సమాసం*> పోర్టు_Map;
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            పోర్టు_Map[tconnect.మొదటి] = tconnect.రెండవ->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            స్థిరరాశి స్వయంచాలక it = పోర్టు_Map.కనుగొను(refp->varp());
            ఒకవేళ (it != పోర్టు_Map.చివర()) {
                refp->replaceWith(it->రెండవ->clone_వృక్షం(అబద్ధం));
                VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
            }
        });
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            నెట్టు_Deletep(tconnect.రెండవ->exprp()->unlink_Fr_వెనుక());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(నెట్టు_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(అబద్ధం);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    శూన్య inlineAllSequenceRefs(Ast_కణుపు* rootp) {
        తార్కిక మారింది = నిజం;
        అయితే (మారింది) {
            మారింది = అబద్ధం;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                ఒకవేళ (మారింది) తిరిగి;
                ఒకవేళ (AstSequence* స్థిరరాశి seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    మారింది = నిజం;
                }
            });
        }
    }

    స్థిర తార్కిక has_Multi_చక్రం_సమాసం(స్థిరరాశి Ast_కణుపు* nodep) {
        తిరిగి nodep->exists([](స్థిరరాశి Ast_కణుపు* np) {
            ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి ep = VN_CAST(np, కణుపు_సమాసం)) తిరిగి ep->is_Multi_చక్రం_Sva();
            తిరిగి అబద్ధం;
        });
    }

    స్థిర VPropStrength effectiveAssertPropStrength(స్థిరరాశి AstPropSpec* స్థిరరాశి propSpecp) {
        ఒకవేళ (propSpecp->propStrength() != VPropStrength::DEFAULT) తిరిగి propSpecp->propStrength();
        తిరిగి propSpecp->fileline()->language() <= V3_Lang_సంకేతం::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    స్థిర తార్కిక is_Bare_శిఖరం_Level_Until(Ast_కణుపు* propp) {
        Ast_కణుపు* p = propp;
        ఒకవేళ (AstPropSpec* స్థిరరాశి specp = VN_CAST(p, PropSpec)) p = specp->propp();
        అయితే (AstLogNot* స్థిరరాశి notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* స్థిరరాశి untilp = VN_CAST(p, Until);
        ఒకవేళ (!untilp) తిరిగి అబద్ధం;
        స్థిరరాశి స్వయంచాలక hasSeq = [](స్థిరరాశి Ast_కణుపు_సమాసం* ep) {
            తిరిగి ep->exists([](స్థిరరాశి Ast_కణుపు_సమాసం* np) { తిరిగి np->is_Multi_చక్రం_Sva(); });
        };
        ఒకవేళ (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }

    నిర్మాణం PropertyParts final {
        Ast_కణుపు_సమాసం* triggerExprp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* seqExprp = శూన్యసూచిక;
        తార్కిక isOverlapped = నిజం;
        తార్కిక hasImplication = అబద్ధం;
        తార్కిక isFollowedBy = అబద్ధం;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    స్థిర PropertyParts decomposeProperty(Ast_కణుపు* propp) {
        PropertyParts parts;
        ఒకవేళ (AstPropSpec* స్థిరరాశి specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        ఒకవేళ (AstImplication* స్థిరరాశి implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = నిజం;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } లేకపోతే ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_CAST(propp, కణుపు_సమాసం)) {
            parts.triggerExprp = శూన్యసూచిక;
            parts.seqExprp = exprp;
        }
        తిరిగి parts;
    }

    స్థిర తార్కిక can_విభజించు_Implication_Pass_Actions(స్థిరరాశి PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        తిరిగి !has_Multi_చక్రం_సమాసం(parts.triggerExprp);
    }

    స్థిర శూన్య విభజించు_Implication_Pass_Actions(AstAssert* assertp, స్థిరరాశి PropertyParts& parts,
                                            Ast_కణుపు_సమాసం* nonvacuousMatchp) {
        File_పంక్తి* స్థిరరాశి flp = assertp->fileline();

        Ast_కణుపు* స్థిరరాశి passsp = assertp->passsp()->unlink_Fr_వెనుక_With_తదుపరి();
        Ast_కణుపు* splitsp = శూన్యసూచిక;

        ఒకవేళ (!parts.isFollowedBy) {
            Ast_కణుపు_సమాసం* స్థిరరాశి vacuousp
                = కొత్త AstLogNot{flp, sampled(parts.triggerExprp->clone_వృక్షం_శుద్ధ(అబద్ధం))};
            Ast_కణుపు* స్థిరరాశి vacuousBodyp = passsp->clone_వృక్షం(అబద్ధం);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/నిజం);
        }

        AstIf* స్థిరరాశి nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/అబద్ధం);
        splitsp = splitsp ? Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(splitsp, nonvacuousp)
                          : static_cast<Ast_కణుపు*>(nonvacuousp);
        assertp->కూడు_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    శూన్య replaceBodyOnBuildError(File_పంక్తి* flp, AstPropSpec* propSpecp, తార్కిక errorEmitted) {
        ఒకవేళ (!errorEmitted) తిరిగి;
        Ast_కణుపు* స్థిరరాశి innerPropp = propSpecp->propp();
        innerPropp->replaceWith(కొత్త AstConst{flp, AstConst::బిట్టు_False{}});
        VL_DO_DANGLING(నెట్టు_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    తార్కిక hoistClockedSeq(AstPropSpec* specp) {
        అయితే (AstSClocked* స్థిరరాశి clockedp = VN_CAST(specp->propp(), SClocked)) {
            ఒకవేళ (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, నిజం);
                తిరిగి నిజం;
            }
            కొరకు (స్థిరరాశి Ast_Sen_అంశం* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), Sen_అంశం)) {
                ఒకవేళ (!sp->అంచు_Type().an_అంచు()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, నిజం);
                    తిరిగి నిజం;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_వెనుక_With_తదుపరి());
            Ast_కణుపు_సమాసం* స్థిరరాశి bodyp = clockedp->exprp()->unlink_Fr_వెనుక();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(నెట్టు_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        స్థిరరాశి AstSClocked* nestedp = శూన్యసూచిక;
        specp->propp()->foreach([&](స్థిరరాశి AstSClocked* p) {
            ఒకవేళ (!nestedp) nestedp = p;
        });
        ఒకవేళ (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, నిజం);
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_ఫలితం build_Assertion_లేఖాచిత్రం(SvaNfaBuilder& builder, Sva_లేఖాచిత్రం& లేఖాచిత్రం, Ast_కణుపు_సమాసం* seqBodyp,
                                    స్థిరరాశి PropertyParts& parts, File_పంక్తి* flp) {
        ఒకవేళ (!parts.hasImplication) తిరిగి builder.build(seqBodyp);

        లేఖాచిత్రం.m_ప్రారంభం_Vertexp = లేఖాచిత్రం.create_State_శీర్షం();
        తిరిగి builder.build_Implication_అంచులు(parts.triggerExprp, seqBodyp, లేఖాచిత్రం.m_ప్రారంభం_Vertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    శూన్య attachActionHandlers(AstAssert* స్థిరరాశి assertp, Ast_కణుపు_సమాసం* సరిపోలిక_Exprp,
                              Ast_Sen_వృక్షం* స్థిరరాశి threadFailReplaySenTreep,
                              Ast_కణుపు_సమాసం* స్థిరరాశి threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        ఒకవేళ (సరిపోలిక_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_కణుపు* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_వెనుక_With_తదుపరి();
            File_పంక్తి* స్థిరరాశి flp = assertp->fileline();
            AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, సరిపోలిక_Exprp, passsp, శూన్యసూచిక};
            assertp->కూడు_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            ఒకవేళ (assertp->failsp()) assertp->prependFailsp(ifp->clone_వృక్షం(అబద్ధం));
        }

        ఒకవేళ (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_కణుపు* స్థిరరాశి failsp = assertp->failsp();
            File_పంక్తి* స్థిరరాశి flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* స్థిరరాశి మిగిలిన_Fail_లెక్క_Varp
                = కొత్త AstVar{flp, VVarType::MODULETEMP, m_fail_లెక్క_పేర్లు.get(""),
                             m_modp->కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32)};
            మిగిలిన_Fail_లెక్క_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->కూడు_Stmtsp(మిగిలిన_Fail_లెక్క_Varp);
            Ast_కణుపు* స్థిరరాశి replayStmtsp = కొత్త Ast_కేటాయించు{
                flp, కొత్త AstVarRef{flp, మిగిలిన_Fail_లెక్క_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* స్థిరరాశి replayLoopp = కొత్త AstLoop{flp};
            replayLoopp->కూడు_Stmtsp(కొత్త AstLoopTest{
                flp, replayLoopp,
                కొత్త AstGt{flp, కొత్త AstVarRef{flp, మిగిలిన_Fail_లెక్క_Varp, VAccess::READ},
                          newTypedConstp(flp, మిగిలిన_Fail_లెక్క_Varp->dtypep(), 1)}});
            replayLoopp->కూడు_Stmtsp(newIfAssertFailOn(failsp->clone_వృక్షం(నిజం), assertp->directive(),
                                                     assertp->userType()));
            AstSub* స్థిరరాశి decrementedFailCountp
                = కొత్త AstSub{flp, కొత్త AstVarRef{flp, మిగిలిన_Fail_లెక్క_Varp, VAccess::READ},
                             newTypedConstp(flp, మిగిలిన_Fail_లెక్క_Varp->dtypep(), 1)};
            replayLoopp->కూడు_Stmtsp(
                కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, మిగిలిన_Fail_లెక్క_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->కూడు_తదుపరి(replayLoopp);
            m_modp->కూడు_Stmtsp(
                కొత్త AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    శూన్య substitute_సరిపోలిక_అంశం_Ref(AstVarRef* refp, గుర్తులేని K,
                                స్థిరరాశి std::unordered_map<స్థిరరాశి AstVar*, Ast_కణుపు_సమాసం*>& సరిపోలిక_Map) {
        స్థిరరాశి స్వయంచాలక it = సరిపోలిక_Map.కనుగొను(refp->varp());
        ఒకవేళ (it == సరిపోలిక_Map.చివర()) తిరిగి;
        Ast_కణుపు_సమాసం* newp = it->రెండవ->clone_వృక్షం_శుద్ధ(అబద్ధం);
        ఒకవేళ (K > 0) {
            AstConst* స్థిరరాశి ticksp = కొత్త AstConst{refp->fileline(), AstConst::Widthed_విలువ{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* స్థిరరాశి pastp = కొత్త AstPast{refp->fileline(), newp, ticksp, శూన్యసూచిక,
                                               /* propertyTiming */ నిజం};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
        తిరిగి;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    పూర్ణాంకం
    walk_Substitute_సరిపోలిక_వస్తువులు(Ast_కణుపు_సమాసం* nodep, గుర్తులేని K,
                             స్థిరరాశి std::unordered_map<స్థిరరాశి AstVar*, Ast_కణుపు_సమాసం*>& సరిపోలిక_వస్తువులు) {
        ఒకవేళ (AstSExpr* స్థిరరాశి sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* స్థిరరాశి delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_చక్రం_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            ఒకవేళ (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                తిరిగి -1;
            }
            స్థిరరాశి గుర్తులేని delay_చక్రాలు = VN_AS(delayp->lhsp(), Const)->గమ్యం_UInt();
            పూర్ణాంకం preLen = 0;
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి prep = sexprp->preExprp()) {
                preLen = walk_Substitute_సరిపోలిక_వస్తువులు(prep, K, సరిపోలిక_వస్తువులు);
                ఒకవేళ (preLen < 0) తిరిగి -1;
            }
            స్థిరరాశి పూర్ణాంకం bodyLen
                = walk_Substitute_సరిపోలిక_వస్తువులు(sexprp->exprp(), K + preLen + delay_చక్రాలు, సరిపోలిక_వస్తువులు);
            ఒకవేళ (bodyLen < 0) తిరిగి -1;
            తిరిగి preLen + delay_చక్రాలు + bodyLen;
        }
        ఒకవేళ (nodep->is_Multi_చక్రం_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            తిరిగి -1;
        }
        std::సదిశ<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.నెట్టు_వెనుక(p); });
        కొరకు (AstVarRef* స్థిరరాశి refp : refs) substitute_సరిపోలిక_అంశం_Ref(refp, K, సరిపోలిక_వస్తువులు);
        తిరిగి 0;
    }

    // Lower property-local match-item assignments before NFA construction.
    // Without this, the antecedent's AstExprStmt(<x = rhs_expr>, antBool)
    // survives into every NFA edge as a continuous-alias side-effect, so the
    // local-var temp tracks the current cycle's rhs_expr rather than the
    // antecedent-match cycle's value -- wrong for `|-> ##N` and `|=> ##N`
    // with N > 0 (issue #7587). Each consequent reference to the local var
    // is replaced with `$past(rhs_expr, K)` where K = (overlapped ? 0 : 1)
    // plus any accumulated `##N` delay. Returns true if E_UNSUPPORTED was
    // emitted; caller must replace the body with BitFalse and bail.
    తార్కిక lift_సరిపోలిక_అంశం_Substitutions(PropertyParts& parts, Ast_కణుపు_సమాసం* seqBodyp) {
        ఒకవేళ (!parts.hasImplication) తిరిగి అబద్ధం;
        Ast_సమాసం_Stmt* స్థిరరాశి సమాసం_Stmtp = VN_CAST(parts.triggerExprp, సమాసం_Stmt);
        ఒకవేళ (!సమాసం_Stmtp) తిరిగి అబద్ధం;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<స్థిరరాశి AstVar*, Ast_కణుపు_సమాసం*> సరిపోలిక_వస్తువులు;
        కొరకు (Ast_కణుపు* stmtp = సమాసం_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_కేటాయించు* స్థిరరాశి assignp = VN_AS(stmtp, కేటాయించు);
            AstVarRef* స్థిరరాశి lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            సరిపోలిక_వస్తువులు[lhsRefp->varp()] = assignp->rhsp();
        }
        స్థిరరాశి గుర్తులేని ప్రారంభం_K = parts.isOverlapped ? 0 : 1;
        స్థిరరాశి పూర్ణాంకం నిడివి = walk_Substitute_సరిపోలిక_వస్తువులు(seqBodyp, ప్రారంభం_K, సరిపోలిక_వస్తువులు);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_వృక్షం();
        ఒకవేళ (నిడివి < 0) తిరిగి నిజం;
        Ast_కణుపు_సమాసం* స్థిరరాశి antBoolp = సమాసం_Stmtp->resultp()->unlink_Fr_వెనుక();
        సమాసం_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(నెట్టు_Deletep(సమాసం_Stmtp), సమాసం_Stmtp);
        parts.triggerExprp = antBoolp;
        తిరిగి అబద్ధం;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    స్థిర స్థిరరాశి అక్షరం* unsupportedPropertyControl(స్థిరరాశి Ast_కణుపు_Cover_Or_Assert* assertp,
                                                  స్థిరరాశి Ast_కణుపు_సమాసం* seqBodyp, తార్కిక negated) {
        ఒకవేళ (!hasPropertyControlConjunction(seqBodyp)) తిరిగి శూన్యసూచిక;
        ఒకవేళ (negated) తిరిగి "negation";
        ఒకవేళ (VN_IS(assertp, Cover)) తిరిగి "cover";
        ఒకవేళ (VN_AS(assertp, Assert)->passsp()) తిరిగి "a pass action";
        తిరిగి శూన్యసూచిక;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_కణుపు_Cover_Or_Assert* assertp) {
        ఒకవేళ (AstPropSpec* స్థిరరాశి specp = VN_CAST(assertp->propp(), PropSpec)) {
            ఒకవేళ (AstFuncRef* స్థిరరాశి funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                ఒకవేళ (స్థిరరాశి AstProperty* స్థిరరాశి propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        ఒకవేళ (AstPropSpec* స్థిరరాశి specp = VN_CAST(assertp->propp(), PropSpec)) {
            ఒకవేళ (hoistClockedSeq(specp)) తిరిగి శూన్యసూచిక;
        }

        AstPropSpec* స్థిరరాశి propp = VN_AS(assertp->propp(), PropSpec);
        ఒకవేళ (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/నిజం);
            తిరిగి శూన్యసూచిక;
        }

        ఒకవేళ (!has_Multi_చక్రం_సమాసం(propp)) తిరిగి శూన్యసూచిక;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        ఒకవేళ (propp->exists([](స్థిరరాశి AstFuncRef* refp) { తిరిగి VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(నెట్టు_Deletep(assertp->unlink_Fr_వెనుక()), assertp);
            తిరిగి శూన్యసూచిక;
        }
        ఒకవేళ (is_Bare_శిఖరం_Level_Until(propp)) తిరిగి శూన్యసూచిక;
        తిరిగి propp;
    }

    శూన్య processAssertion(Ast_కణుపు_Cover_Or_Assert* assertp) {
        ఒకవేళ (assertp->తక్షణ()) తిరిగి;

        AstPropSpec* స్థిరరాశి propp = prepareAssertionProp(assertp);
        ఒకవేళ (!propp) తిరిగి;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_కణుపు_సమాసం* seqBodyp = parts.seqExprp;
        తార్కిక negated = అబద్ధం;
        అయితే (AstLogNot* స్థిరరాశి notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        స్థిరరాశి అక్షరం* స్థిరరాశి propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        ఒకవేళ (lift_సరిపోలిక_అంశం_Substitutions(parts, seqBodyp)) {
            AstPropSpec* స్థిరరాశి psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/నిజం);
            తిరిగి;
        }

        AstCover* స్థిరరాశి coverp = VN_CAST(assertp, Cover);
        స్థిరరాశి తార్కిక isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        స్థిరరాశి తార్కిక isSeqEvent = coverp && coverp->isSeqEvent();
        స్థిరరాశి తార్కిక లెక్క_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        ఒకవేళ (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_వృక్షం(నిజం));
        }
        ఒకవేళ (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_వృక్షం_శుద్ధ(నిజం));
        }
        ఒకవేళ (!propp->sensesp()) తిరిగి;
        Ast_Sen_వృక్షం* senTreep
            = కొత్త Ast_Sen_వృక్షం{propp->fileline(), propp->sensesp()->clone_వృక్షం(నిజం)};
        Ast_కణుపు_సమాసం* స్థిరరాశి disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_విలువ_Clockp);
            m_sampled_విలువ_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_పంక్తి* స్థిరరాశి flp = assertp->fileline();

        Sva_లేఖాచిత్రం లేఖాచిత్రం;
        SvaNfaBuilder builder{లేఖాచిత్రం,      m_modp,     m_prop_తాత్కాలిక_పేర్లు,
                              isCoverSeq, isSeqEvent, coverp != శూన్యసూచిక};

        స్థిరరాశి Build_ఫలితం ఫలితం = build_Assertion_లేఖాచిత్రం(builder, లేఖాచిత్రం, seqBodyp, parts, flp);
        ఒకవేళ (ఫలితం.valid()) wire_సరిపోలిక_And_Mid_Sources(లేఖాచిత్రం, ఫలితం, flp);
        ఒకవేళ (!ఫలితం.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, ఫలితం.errorEmitted);
            VL_DO_DANGLING(నెట్టు_Deletep(senTreep), senTreep);
            తిరిగి;
        }
        // After the build, so a construct the builder rejects reports itself.
        ఒకవేళ (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/నిజం);
            VL_DO_DANGLING(నెట్టు_Deletep(senTreep), senTreep);
            తిరిగి;
        }
        ఒకవేళ (లెక్క_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(ఫలితం.finalCondp);
            VL_DO_DANGLING(నెట్టు_Deletep(senTreep), senTreep);
            Ast_కణుపు* స్థిరరాశి innerPropp = propp->propp();
            innerPropp->replaceWith(కొత్త AstConst{flp, AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(innerPropp), innerPropp);
            తిరిగి;
        }

        AstAssert* స్థిరరాశి assertAssertp = VN_CAST(assertp, Assert);
        స్థిరరాశి తార్కిక విభజించు_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_విభజించు_Implication_Pass_Actions(parts);
        స్థిరరాశి తార్కిక need_సరిపోలిక = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || విభజించు_Implication_Passsp);

        స్థిరరాశి తార్కిక needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        స్థిరరాశి స్వయంచాలక సంకేతాలు
            = m_loweringp->క్రింది(assertp, లేఖాచిత్రం, senTreep, ఫలితం.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_కణుపు_సమాసం* సరిపోలిక_Exprp = శూన్యసూచిక;
        Ast_కణుపు_సమాసం* outputExprp = m_loweringp->assemble_ఫలితం(
            assertp, negated, ఫలితం.finalCondp, సంకేతాలు, need_సరిపోలిక ? &సరిపోలిక_Exprp : శూన్యసూచిక);
        ఒకవేళ (లెక్క_Matches) {
            UASSERT_OBJ(సంకేతాలు.సరిపోలిక_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_వృక్షం(), outputExprp);
            propp->సరిపోలిక_Countp(సంకేతాలు.సరిపోలిక_Countp);
            outputExprp = కొత్త AstNeq{flp, సంకేతాలు.సరిపోలిక_Countp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                                     newTypedConstp(flp, సంకేతాలు.సరిపోలిక_Countp->dtypep(), 0)};
        }

        Ast_Sen_వృక్షం* స్థిరరాశి threadFailReplaySenTreep
            = సంకేతాలు.threadFailCountp ? senTreep->clone_వృక్షం(అబద్ధం) : శూన్యసూచిక;

        VL_DO_DANGLING(నెట్టు_Deletep(senTreep), senTreep);
        ఒకవేళ (ఫలితం.finalCondp && !ఫలితం.finalCondp->backp()) నెట్టు_Deletep(ఫలితం.finalCondp);

        ఒకవేళ (విభజించు_Implication_Passsp) {
            విభజించు_Implication_Pass_Actions(assertAssertp, parts, సరిపోలిక_Exprp);
        } లేకపోతే {
            attachActionHandlers(assertAssertp, సరిపోలిక_Exprp, threadFailReplaySenTreep,
                                 సంకేతాలు.threadFailCountp);
        }

        Ast_కణుపు* స్థిరరాశి innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(నెట్టు_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << పంక్తి_ముగింపు);

        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) లేఖాచిత్రం.m_లేఖాచిత్రం.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        ఒకవేళ (!VN_IS(nodep, Class)) m_defaultClockingp = శూన్యసూచిక;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstClocking* nodep) override {
        ఒకవేళ (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstDefaultDisable* nodep) override {}
    శూన్య సందర్శించు(AstFell* nodep) override { సందర్శించు_Sampled_విలువ(nodep); }
    శూన్య సందర్శించు(AstPast* nodep) override { సందర్శించు_Sampled_విలువ(nodep); }
    శూన్య సందర్శించు(AstRose* nodep) override { సందర్శించు_Sampled_విలువ(nodep); }
    శూన్య సందర్శించు(AstStable* nodep) override { సందర్శించు_Sampled_విలువ(nodep); }
    శూన్య సందర్శించు(AstAssert* nodep) override { processAssertion(nodep); }
    శూన్య సందర్శించు(AstCover* nodep) override { processAssertion(nodep); }
    శూన్య సందర్శించు(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstAssertIntrinsic* nodep) override {}
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    స్పష్టమైన AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

శూన్య V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << పంక్తి_ముగింపు);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_వృక్షం("assertnfa", 0, dump_వృక్షం_Either_Level() >= 3);
}
