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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3AssertNfa.h"

#अन्तर्भाव "V3Assert.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3Task.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <set>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

नामाकाश {

वर्ग SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
संरचना Sva_Vertex_दत्त final {
    AstVar* stateVarp = शून्यसूचकः;  // NBA state register for this vertex
    AstVar* delayRingVarp = शून्यसूचकः;  // Packed occupancy bits or per-slot match counts
    AstVar* delay_Ring_शीर्ष_Varp
        = शून्यसूचकः;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = शून्यसूचकः;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_जीवितम्_गणनम्_Varp = शून्यसूचकः;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = शून्यसूचकः;  // All slots written since the last clear
    AstVar* समाप्तम्_LVarp = शून्यसूचकः;  // SAnd LHS done-latch
    AstVar* समाप्तम्_RVarp = शून्यसूचकः;  // SAnd RHS done-latch
    Ast_ग्रन्थिः_व्यञ्जकम्* stateSigp = शून्यसूचकः;  // Combinational state signal; OWNED during lowering
    Ast_ग्रन्थिः_व्यञ्जकम्* state_गणनम्_Sigp = शून्यसूचकः;  // Cover-sequence match count; OWNED during lowering
    Ast_ग्रन्थिः_व्यञ्जकम्* state_गणनम्_अतिप्रवाहः_Sigp = शून्यसूचकः;  // Count overflow; OWNED during lowering
    तार्किक delayRingCounts = असत्य;  // Ring slots hold cover-sequence match counts
    तार्किक needsReg = असत्य;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
वर्ग SvaStateVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
सार्वजनिक:
    // True if this is the sequence-match terminal vertex
    तार्किक m_is_मेलनम् = असत्य;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    तार्किक m_isFixedDelayRing = असत्य;
    अचिह्नित m_delay_Ring_आकार = 0;  // Number of ring slots. Range: max-min+1.
    Ast_ग्रन्थिः_व्यञ्जकम्* m_delayRingClearCondp = शून्यसूचकः;  // local RHS for pure-boolean range
    Ast_ग्रन्थिः_व्यञ्जकम्* m_delayRingAdvanceCondp = शून्यसूचकः;  // Advance only when this condition holds
    तार्किक m_replayAbortReject = असत्य;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_ग्रन्थिः_व्यञ्जकम्* m_abortClearp = शून्यसूचकः;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_ग्रन्थिः_व्यञ्जकम्* m_abortRejectp = शून्यसूचकः;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    तार्किक m_isUnbounded = असत्य;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    तार्किक m_isAndCombiner = असत्य;
    SvaStateVertex* m_andLhsTermp = शून्यसूचकः;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = शून्यसूचकः;  // RHS sub-NFA terminal vertex
    Ast_ग्रन्थिः_व्यञ्जकम्* m_andLhsCondp = शून्यसूचकः;  // OWNED; LHS final condition (may be nullptr)
    Ast_ग्रन्थिः_व्यञ्जकम्* m_andRhsCondp = शून्यसूचकः;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    तार्किक m_isRejectSink = असत्य;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    तार्किक m_strongPending = असत्य;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    तार्किक m_strongAlwaysRing = असत्य;

    // CONSTRUCTORS
    स्पष्ट SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_वृक्षः(), cp);
        यदि (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_वृक्षः(), m_delayRingClearCondp);
        यदि (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_वृक्षः(), m_delayRingAdvanceCondp);
        यदि (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_वृक्षः(), m_abortClearp);
        यदि (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_वृक्षः(), m_abortRejectp);
        यदि (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_वृक्षः(), m_andLhsCondp);
        यदि (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_वृक्षः(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() ध्रुव override {
        string name = "s" + cvt_गन्तव्यम्_Str(color());
        यदि (m_delay_Ring_आकार) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvt_गन्तव्यम्_Str(m_delay_Ring_आकार) + " slots";
        }
        प्रत्यागम name;
    }
    string dotColor() ध्रुव override {
        यदि (m_is_मेलनम्) प्रत्यागम "red";
        यदि (m_delay_Ring_आकार) प्रत्यागम "blue";
        यदि (m_isAndCombiner) प्रत्यागम "purple";
        प्रत्यागम "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    Sva_Vertex_दत्त* datap() ध्रुव { प्रत्यागम static_cast<Sva_Vertex_दत्त*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
वर्ग SvaTransEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
सार्वजनिक:
    Ast_ग्रन्थिः_व्यञ्जकम्* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    तार्किक m_consumes_आवृत्तिः;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    तार्किक m_rejectOnFail = असत्य;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = शून्यसूचकः;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्, Ast_ग्रन्थिः_व्यञ्जकम्* condp,
                 तार्किक consumes_आवृत्तिः)
        : V3GraphEdge{graphp, fromp, शिखरम्, /*weight=*/1}
        , m_condp{condp}
        , m_consumes_आवृत्तिः{consumes_आवृत्तिः} {}
    ~SvaTransEdge() override {
        यदि (m_condp) VL_DO_DANGLING(m_condp->delete_वृक्षः(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() ध्रुव override { प्रत्यागम m_consumes_आवृत्तिः ? "##1" : "link"; }
    string dotStyle() ध्रुव override { प्रत्यागम m_consumes_आवृत्तिः ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() ध्रुव { प्रत्यागम static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* गन्तव्यम्_Vtxp() ध्रुव { प्रत्यागम static_cast<SvaStateVertex*>(शिखरम्()); }
};

// NFA graph container
वर्ग SvaGraph final {
सार्वजनिक:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = शून्यसूचकः;  // Trigger/start vertex
    SvaStateVertex* m_मेलनम्_Vertexp = शून्यसूचकः;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { प्रत्यागम नूतन SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* create_मेलनम्_Vertex() {
        SvaStateVertex* ध्रुव vtxp = createStateVertex();
        vtxp->m_is_मेलनम् = सत्य;
        m_मेलनम्_Vertexp = vtxp;
        प्रत्यागम vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* सङ्कलय_Clocked_Edge(SvaStateVertex* fromp, SvaStateVertex* शिखरम्,
                                 Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः) {
        प्रत्यागम नूतन SvaTransEdge{&m_graph, fromp, शिखरम्, condp, /*consumesCycle=*/सत्य};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* सङ्कलय_Link(SvaStateVertex* fromp, SvaStateVertex* शिखरम्,
                          Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः) {
        प्रत्यागम नूतन SvaTransEdge{&m_graph, fromp, शिखरम्, condp, /*consumesCycle=*/असत्य};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::सदिशम्<ध्रुव SvaTransEdge*> all_धाराः() ध्रुव {
        std::सदिशम्<ध्रुव SvaTransEdge*> परिणाम;
        प्रति (ध्रुव V3GraphVertex& vtxr : m_graph.vertices()) {
            प्रति (ध्रुव V3GraphEdge& edger : vtxr.बहिर्गमनम्_धाराः()) {
                परिणाम.प्रक्षिप_पृष्ठभागः(static_cast<ध्रुव SvaTransEdge*>(&edger));
            }
        }
        प्रत्यागम परिणाम;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
संरचना Build_परिणाम final {
    SvaStateVertex* पदांशः_Vertexp;  // Primary terminal; contributes to both match and reject
    Ast_ग्रन्थिः_व्यञ्जकम्* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::सदिशम्<SvaStateVertex*> midSources;
    तार्किक errorEmitted = असत्य;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    तार्किक पदांशः_Is_Mid_Merge = असत्य;
    तार्किक valid() ध्रुव { प्रत्यागम पदांशः_Vertexp != शून्यसूचकः; }
    स्थिर Build_परिणाम fail(तार्किक errored = असत्य) { प्रत्यागम {शून्यसूचकः, शून्यसूचकः, {}, errored}; }
    स्थिर Build_परिणाम failWithError() { प्रत्यागम {शून्यसूचकः, शून्यसूचकः, {}, सत्य}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
स्थिर तार्किक hasPropertyControlConjunction(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
    प्रत्यागम nodep->exists([](ध्रुव AstSAnd* andp) { प्रत्यागम andp->propertyControl(); });
}

स्थिर AstConst* newTypedConstp(File_पङ्क्तिः* ध्रुव flp, ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep,
                                ध्रुव uint32_t मानम्) {
    AstConst* ध्रुव constp = नूतन AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->अङ्कः().समुच्चयः_Long(मानम्);
    प्रत्यागम constp;
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* sampled(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
    प्रत्यागम नूतन AstSampled{exprp->fileline(), exprp, exprp->dtypep(), सत्य};
}

स्थिर ध्रुव वर्ण* assertPassOnQuery(तार्किक vacuous) {
    स्थिर constexpr ध्रुव वर्ण* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    प्रत्यागम queries[vacuous];
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertOnCond(File_पङ्क्तिः* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    यदि (!v3Global.opt.assertOn()) { प्रत्यागम नूतन AstConst{flp, AstConst::BitFalse{}}; }
    प्रत्यागम नूतन AstCExpr{flp, AstCExpr::विशुद्धम्{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertKillGet(File_पङ्क्तिः* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    प्रत्यागम नूतन AstCExpr{flp, AstCExpr::विशुद्धम्{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

स्थिर string assert_Action_Control_उपसर्गः(VAssertDirectiveType directiveType) {
    ध्रुव पूर्णाङ्क controlled = !!(static_cast<पूर्णाङ्क>(directiveType)
                              & (static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSERT)
                                 | static_cast<पूर्णाङ्क>(VAssertDirectiveType::COVER)
                                 | static_cast<पूर्णाङ्क>(VAssertDirectiveType::ASSUME)));
    ध्रुव पूर्णाङ्क checkRuntime = controlled & static_cast<पूर्णाङ्क>(v3Global.opt.assertOn());
    प्रत्यागम "("s + std::गन्तव्यम्_string(controlled ^ 1) + " || ("s + std::गन्तव्यम्_string(checkRuntime)
           + " && "s;
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertPassOnCond(File_पङ्क्तिः* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, तार्किक vacuous) {
    प्रत्यागम नूतन AstCExpr{
        flp, AstCExpr::विशुद्धम्{},
        assert_Action_Control_उपसर्गः(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assertFailOnCond(File_पङ्क्तिः* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    प्रत्यागम नूतन AstCExpr{
        flp, AstCExpr::विशुद्धम्{},
        assert_Action_Control_उपसर्गः(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

स्थिर AstIf* newPassOnIf(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* firep, Ast_ग्रन्थिः* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, तार्किक vacuous) {
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp
        = नूतन AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(सत्य);
    ifp->user1(सत्य);
    प्रत्यागम ifp;
}

स्थिर Ast_ग्रन्थिः_Stmt* newIfAssertFailOn(Ast_ग्रन्थिः* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_पङ्क्तिः* ध्रुव flp = bodyp->fileline();
    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = assertFailOnCond(flp, type, directiveType);
    AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(सत्य);
    ifp->user1(सत्य);
    प्रत्यागम ifp;
}

//######################################################################
// NFA Builder

वर्ग SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module to receive hoisted sampled-prop temps
    V3_Unique_नामानि& m_prop_अस्थायी_नामानि;  // Module-shared temp-var name source
    std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> m_temporal_Guard_राशिः;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> m_outer_Abort_राशिः;
    तार्किक m_अन्तर्_Unbounded_Scope = असत्य;  // Sticky: nodes created after inherit liveness
    तार्किक m_markStrongPending = असत्य;  // Mark new vertices as strong s_always in-window
    तार्किक m_isCover = असत्य;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    तार्किक m_isCoverSeq = असत्य;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    तार्किक m_isSeqEvent = असत्य;
    तार्किक m_अन्तर्_Sequence_उपसर्गः = असत्य;  // Prefix composition does not preserve every endpoint
    तार्किक m_want_Bool_पर्णम् = असत्य;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    संरचना RangeDelayRejectInfo final {
        SvaStateVertex* startp = शून्यसूचकः;
        अचिह्नित range = 0;
        पूर्णाङ्क rhsLen = 0;
    };

    शून्य warnEndpointUnsupported(File_पङ्क्तिः* flp, ध्रुव string& what) ध्रुव {
        यदि (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } अन्यथा {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* throughoutCond(Ast_ग्रन्थिः_व्यञ्जकम्* आधारः_Condp, File_पङ्क्तिः* flp) {
        यदि (m_temporal_Guard_राशिः.रिक्तानि()) प्रत्यागम आधारः_Condp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_ग्रन्थिः_व्यञ्जकम्* guardp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp : m_temporal_Guard_राशिः) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = sampled(condp->clone_वृक्षः_विशुद्धम्(असत्य));
            यदि (!guardp) {
                guardp = clonep;
            } अन्यथा {
                guardp = नूतन AstLogAnd{flp, guardp, clonep};
            }
        }
        यदि (आधारः_Condp) { guardp = नूतन AstLogAnd{flp, आधारः_Condp, guardp}; }
        प्रत्यागम guardp;
    }

    स्थिर अचिह्नित getConstUInt(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव constp = V3Const::constifyEdit(exprp->clone_वृक्षः_विशुद्धम्(असत्य));
        ध्रुव AstConst* ध्रुव cp = VN_CAST(constp, Const);
        ध्रुव अचिह्नित val = cp ? cp->गन्तव्यम्_UInt() : 0;
        VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
        प्रत्यागम val;
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
    स्थिर पूर्णाङ्क fixed_दीर्घता(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
            यदि (!delayp || !delayp->is_आवृत्तिः_Delay()) प्रत्यागम -1;
            अचिह्नित delay_आवृत्तयः;
            यदि (delayp->isRangeDelay()) {
                यदि (delayp->isUnbounded()) प्रत्यागम -1;  // LCOV_EXCL_LINE
                ध्रुव अचिह्नित minD = getConstUInt(delayp->lhsp());
                ध्रुव अचिह्नित maxD = getConstUInt(delayp->rhsp());
                यदि (minD != maxD) प्रत्यागम -1;
                delay_आवृत्तयः = minD;
            } अन्यथा {
                delay_आवृत्तयः = getConstUInt(delayp->lhsp());
            }
            पूर्णाङ्क preLen = 0;
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp()) {
                preLen = fixed_दीर्घता(prep);
                यदि (preLen < 0) प्रत्यागम -1;  // LCOV_EXCL_LINE
            }
            ध्रुव पूर्णाङ्क bodyLen = fixed_दीर्घता(sexprp->exprp());
            यदि (bodyLen < 0) प्रत्यागम -1;  // LCOV_EXCL_LINE
            प्रत्यागम preLen + delay_आवृत्तयः + bodyLen;
        }
        यदि (AstSThroughout* ध्रुव throughp = VN_CAST(nodep, SThroughout)) {
            प्रत्यागम fixed_दीर्घता(throughp->rhsp());
        }
        यदि (AstSOr* ध्रुव orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            ध्रुव पूर्णाङ्क lhsLen = fixed_दीर्घता(orp->lhsp());
            ध्रुव पूर्णाङ्क rhsLen = fixed_दीर्घता(orp->rhsp());
            यदि (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) प्रत्यागम -1;
            प्रत्यागम lhsLen;
        }
        यदि (AstSWithin* ध्रुव withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            ध्रुव पूर्णाङ्क lhsLen = fixed_दीर्घता(withinp->lhsp());
            ध्रुव पूर्णाङ्क rhsLen = fixed_दीर्घता(withinp->rhsp());
            यदि (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) प्रत्यागम -1;
            प्रत्यागम rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        यदि (nodep->is_Multi_आवृत्तिः_Sva()) प्रत्यागम -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        प्रत्यागम 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    स्थिर std::pair<पूर्णाङ्क, पूर्णाङ्क> दीर्घता_Range(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
            यदि (!delayp || !delayp->is_आवृत्तिः_Delay()) प्रत्यागम {-1, -1};
            std::pair<पूर्णाङ्क, पूर्णाङ्क> delayRange;
            यदि (delayp->isRangeDelay()) {
                यदि (delayp->isUnbounded()) प्रत्यागम {-1, -1};
                ध्रुव अचिह्नित minD = getConstUInt(delayp->lhsp());
                ध्रुव अचिह्नित maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } अन्यथा {
                ध्रुव अचिह्नित d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<पूर्णाङ्क, पूर्णाङ्क> preRange{0, 0};
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp()) {
                preRange = दीर्घता_Range(prep);
                यदि (preRange.first < 0) प्रत्यागम {-1, -1};
            }
            ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क> bodyRange = दीर्घता_Range(sexprp->exprp());
            यदि (bodyRange.first < 0) प्रत्यागम {-1, -1};
            ध्रुव पूर्णाङ्क variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            यदि (variableParts > 1) प्रत्यागम {-1, -1};
            प्रत्यागम {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        यदि (AstSThroughout* ध्रुव throughp = VN_CAST(nodep, SThroughout)) {
            प्रत्यागम दीर्घता_Range(throughp->rhsp());
        }
        यदि (nodep->is_Multi_आवृत्तिः_Sva()) प्रत्यागम {-1, -1};
        प्रत्यागम {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* realize_At_दीर्घता(Ast_ग्रन्थिः_व्यञ्जकम्* सङ्कार्यम्, पूर्णाङ्क len, पूर्णाङ्क lo) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = सङ्कार्यम्->clone_वृक्षः_विशुद्धम्(असत्य);
        AstDelay* rangeDelayp = शून्यसूचकः;
        clonep->foreach([&](AstDelay* dp) {
            यदि (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        यदि (rangeDelayp) {
            File_पङ्क्तिः* ध्रुव flp = rangeDelayp->fileline();
            ध्रुव अचिह्नित pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(नूतन AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_वृक्षः(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_ग्रन्थिः* ध्रुव oldMaxp = rangeDelayp->rhsp()->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(oldMaxp->delete_वृक्षः(), oldMaxp);
        }
        प्रत्यागम clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, File_पङ्क्तिः* flp, अचिह्नित clone_गणनम्) {
        constexpr अचिह्नित k_Hoist_देहली = 2;
        यदि (clone_गणनम् < k_Hoist_देहली) प्रत्यागम शून्यसूचकः;
        AstVar* ध्रुव अस्थायी_Varp
            = नूतन AstVar{flp, VVarType::MODULETEMP, m_prop_अस्थायी_नामानि.get(exprp), exprp->dtypep()};
        m_modp->सङ्कलय_Stmtsp(अस्थायी_Varp);
        Ast_नियोजय* ध्रुव assignp = नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, अस्थायी_Varp, VAccess::WRITE},
                                                 sampled(exprp->clone_वृक्षः_विशुद्धम्(असत्य))};
        m_modp->सङ्कलय_Stmtsp(नूतन AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, शून्यसूचकः, assignp});
        प्रत्यागम अस्थायी_Varp;
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* sampledRefOrClone(AstVar* hoistVarp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, File_पङ्क्तिः* flp) {
        यदि (hoistVarp) प्रत्यागम नूतन AstVarRef{flp, hoistVarp, VAccess::READ};
        प्रत्यागम sampled(exprp->clone_वृक्षः_विशुद्धम्(असत्य));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* ध्रुव vtxp = m_graph.createStateVertex();
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp : m_temporal_Guard_राशिः) {
            vtxp->m_throughoutConds.प्रक्षिप_पृष्ठभागः(cp->clone_वृक्षः_विशुद्धम्(असत्य));
        }
        यदि (m_अन्तर्_Unbounded_Scope) vtxp->m_isUnbounded = सत्य;
        यदि (m_markStrongPending) vtxp->m_strongPending = सत्य;
        प्रत्यागम vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शिखरम्, Ast_ग्रन्थिः_व्यञ्जकम्* condp,
                              File_पङ्क्तिः* flp) {
        प्रत्यागम m_graph.सङ्कलय_Link(fromp, शिखरम्, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* शिखरम्, File_पङ्क्तिः* flp) {
        प्रत्यागम m_graph.सङ्कलय_Link(fromp, शिखरम्, throughoutCond(शून्यसूचकः, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शिखरम्, Ast_ग्रन्थिः_व्यञ्जकम्* condp,
                              File_पङ्क्तिः* flp) {
        प्रत्यागम m_graph.सङ्कलय_Clocked_Edge(fromp, शिखरम्, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* शिखरम्, File_पङ्क्तिः* flp) {
        प्रत्यागम m_graph.सङ्कलय_Clocked_Edge(fromp, शिखरम्, throughoutCond(शून्यसूचकः, flp));
    }

    SvaStateVertex* सङ्कलय_Delay_Chain(SvaStateVertex* startp, अचिह्नित आकार, File_पङ्क्तिः* flp,
                                  तार्किक isFixed = सत्य, Ast_ग्रन्थिः_व्यञ्जकम्* clearCondp = शून्यसूचकः,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* advanceCondp = शून्यसूचकः) {
        यदि (isFixed && आकार == 0) प्रत्यागम startp;
        UASSERT_OBJ(आकार > 0, startp, "Delay chain needs at least one slot");
        यदि (isFixed && आकार == 1 && !advanceCondp) {
            SvaStateVertex* ध्रुव nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            प्रत्यागम nextp;
        }
        SvaStateVertex* ध्रुव ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delay_Ring_आकार = आकार;
        यदि (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_वृक्षः_विशुद्धम्(असत्य);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        यदि (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } अन्यथा {
            guardedLink(startp, ringVtxp, flp);
        }
        प्रत्यागम ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    तार्किक applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::सदिशम्<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = शून्यसूचकः) {
        File_पङ्क्तिः* ध्रुव flp = sexprp->fileline();
        AstDelay* ध्रुव delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsExprp = sexprp->exprp();
        ध्रुव अचिह्नित minDelay = getConstUInt(delayp->lhsp());
        यदि (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = सङ्कलय_Delay_Chain(currentp, minDelay, flp);
            SvaStateVertex* ध्रुव waitRingp = सङ्कलय_Delay_Chain(currentp, 1, flp, असत्य);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = सत्य;  // Liveness, so no reject.
            m_अन्तर्_Unbounded_Scope = सत्य;
            प्रत्यागम सत्य;
        }
        ध्रुव अचिह्नित maxDelay = getConstUInt(delayp->rhsp());
        यदि (minDelay == maxDelay) {
            currentp = सङ्कलय_Delay_Chain(currentp, minDelay, flp);
            प्रत्यागम सत्य;
        }
        ध्रुव अचिह्नित range = maxDelay - minDelay;
        currentp = सङ्कलय_Delay_Chain(currentp, minDelay, flp);
        ध्रुव तार्किक multi_आवृत्तिः_Rhs = rhsExprp->is_Multi_आवृत्तिः_Sva();
        ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        यदि (m_isCoverSeq
            && (multi_आवृत्तिः_Rhs || m_अन्तर्_Sequence_उपसर्गः
                || (preExprp && preExprp->is_Multi_आवृत्तिः_Sva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            प्रत्यागम असत्य;
        }
        यदि (m_अन्तर्_Sequence_उपसर्गः) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            प्रत्यागम असत्य;
        }
        यदि (multi_आवृत्तिः_Rhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            यदि (rangeRejectInfop) {
                ध्रुव पूर्णाङ्क rhsLen = fixed_दीर्घता(rhsExprp);
                यदि (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* ध्रुव tailp = सङ्कलय_Delay_Chain(currentp, range + 1U, flp, असत्य);
            SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = सत्य;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_अन्तर्_Unbounded_Scope = सत्य;
        } अन्यथा {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.प्रक्षिप_पृष्ठभागः(currentp);
            currentp = सङ्कलय_Delay_Chain(currentp, range + 1U, flp, असत्य,
                                     m_isCoverSeq ? शून्यसूचकः : rhsExprp);
        }
        प्रत्यागम सत्य;
    }

    शून्य सङ्कलय_Finite_Range_Reject(ध्रुव RangeDelayRejectInfo& info, ध्रुव Build_परिणाम& परिणाम,
                              File_पङ्क्तिः* flp) {
        यदि (!info.startp) प्रत्यागम;

        SvaStateVertex* ध्रुव expiryVtxp
            = सङ्कलय_Delay_Chain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* ध्रुव expiryMatchp = scopedCreateVertex();
        std::सदिशम्<SvaStateVertex*> sources = परिणाम.midSources;
        sources.प्रक्षिप_पृष्ठभागः(परिणाम.पदांशः_Vertexp);
        प्रति (SvaStateVertex* ध्रुव srcp : sources) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp
                = परिणाम.finalCondp ? sampled(परिणाम.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)) : शून्यसूचकः;
            SvaStateVertex* ध्रुव successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* ध्रुव historyp
                = सङ्कलय_Delay_Chain(successNowp, info.range + 1U, flp, असत्य);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* ध्रुव sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = सत्य;
        SvaTransEdge* ध्रुव rejectp = m_graph.सङ्कलय_Link(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = सत्य;
        rejectp->m_condVtxp = expiryMatchp;
    }

    Build_परिणाम buildSExpr(AstSExpr* sexprp, SvaStateVertex* प्रविष्टिः_Vtxp,
                           तार्किक is_शिखरम्_Level_चरणः = असत्य) {
        AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
        यदि (!delayp || !delayp->is_आवृत्तिः_Delay()) प्रत्यागम Build_परिणाम::fail();

        File_पङ्क्तिः* ध्रुव flp = sexprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = प्रविष्टिः_Vtxp;
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_अन्तर्_Sequence_उपसर्गः);
            m_अन्तर्_Sequence_उपसर्गः = सत्य;
            ध्रुव Build_परिणाम pre = build_व्यञ्जकम्(preExprp, currentp, is_शिखरम्_Level_चरणः);
            यदि (!pre.valid()) प्रत्यागम Build_परिणाम::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            यदि (pre.finalCondp) {
                SvaStateVertex* ध्रुव condVtxp = scopedCreateVertex();
                SvaTransEdge* ध्रुव edgep = guardedLink(
                    pre.पदांशः_Vertexp, condVtxp, sampled(pre.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)), flp);
                यदि (is_शिखरम्_Level_चरणः && !pre.पदांशः_Vertexp->m_isUnbounded && !m_अन्तर्_Unbounded_Scope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = सत्य;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } अन्यथा {
                currentp = pre.पदांशः_Vertexp;
            }
        }

        // Handle delay
        std::सदिशम्<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        ध्रुव तार्किक सङ्कलय_Range_Reject = is_शिखरम्_Level_चरणः && !m_अन्तर्_Unbounded_Scope;
        यदि (delayp->isRangeDelay()) {
            यदि (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 सङ्कलय_Range_Reject ? &rangeRejectInfo : शून्यसूचकः)) {
                प्रत्यागम Build_परिणाम::failWithError();
            }
        } अन्यथा {
            ध्रुव अचिह्नित delay_आवृत्तयः = getConstUInt(delayp->lhsp());
            currentp = सङ्कलय_Delay_Chain(currentp, delay_आवृत्तयः, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        यदि (exprp->is_Multi_आवृत्तिः_Sva()) {
            ध्रुव Build_परिणाम परिणाम = build_व्यञ्जकम्(exprp, currentp, is_शिखरम्_Level_चरणः);
            यदि (परिणाम.valid()) सङ्कलय_Finite_Range_Reject(rangeRejectInfo, परिणाम, flp);
            प्रत्यागम परिणाम;
        }
        प्रत्यागम {currentp, exprp, std::move(rangeMidSources)};
    }

    Build_परिणाम buildConsRep(AstSConsRep* repp, SvaStateVertex* प्रविष्टिः_Vtxp,
                             तार्किक is_शिखरम्_Level_चरणः = असत्य) {
        File_पङ्क्तिः* ध्रुव flp = repp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        यदि (exprp->is_Multi_आवृत्तिः_Sva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            प्रत्यागम Build_परिणाम::failWithError();
        }
        ध्रुव अचिह्नित minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        अचिह्नित योग_Sites = minN;
        यदि (repp->unbounded()) {
            योग_Sites += 1;
        } अन्यथा यदि (repp->maxCountp()) {
            योग_Sites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* ध्रुव hoistVarp = tryHoistSampled(exprp, flp, योग_Sites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::सदिशम्<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = प्रविष्टिः_Vtxp;
        प्रति (अचिह्नित i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            यदि (i == 1) {
                currentp = सङ्कलय_Delay_Chain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = सत्य;
                यदि (is_शिखरम्_Level_चरणः) {
                    currentp->m_throughoutConds.प्रक्षिप_पृष्ठभागः(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* ध्रुव condVtxp = scopedCreateVertex();
            SvaTransEdge* ध्रुव linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            यदि (is_शिखरम्_Level_चरणः && i == 0) linkp->m_rejectOnFail = सत्य;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        यदि (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.प्रक्षिप_पृष्ठभागः(currentp);
        }

        यदि (repp->unbounded()) {
            यदि (minN == 0) {
                SvaStateVertex* ध्रुव waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } अन्यथा {
                SvaStateVertex* ध्रुव loop_पृष्ठभागः_Vtxp = scopedCreateVertex();
                guardedEdge(currentp, loop_पृष्ठभागः_Vtxp, flp);
                SvaStateVertex* ध्रुव reCheckVtxp = scopedCreateVertex();
                guardedLink(loop_पृष्ठभागः_Vtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loop_पृष्ठभागः_Vtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = सत्य;
            m_अन्तर्_Unbounded_Scope = सत्य;
        } अन्यथा यदि (repp->maxCountp()) {
            ध्रुव अचिह्नित maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            अचिह्नित पुच्छ_Min_N = minN;
            SvaStateVertex* पुच्छ_Startp = currentp;
            यदि (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* ध्रुव अग्र_Vtxp = scopedCreateVertex();
                guardedEdge(currentp, अग्र_Vtxp, flp);
                SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
                guardedLink(अग्र_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                यदि (m_isCoverSeq) consMidSources.प्रक्षिप_पृष्ठभागः(checkVtxp);
                पुच्छ_Startp = checkVtxp;
                पुच्छ_Min_N = 1;
            }
            यदि (maxN > पुच्छ_Min_N) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* ध्रुव अग्र_Vtxp
                    = सङ्कलय_Delay_Chain(पुच्छ_Startp, maxN - पुच्छ_Min_N + 1, flp, असत्य);
                अग्र_Vtxp->m_delayRingClearCondp
                    = नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                अग्र_Vtxp->m_replayAbortReject = सत्य;
                SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
                guardedLink(अग्र_Vtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                यदि (m_isCoverSeq) consMidSources.प्रक्षिप_पृष्ठभागः(checkVtxp);
            }
            currentp = mergeVtxp;
        }
        // finalCond = nullptr (already checked via Links)
        Build_परिणाम res;
        res.पदांशः_Vertexp = currentp;
        res.finalCondp = शून्यसूचकः;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.पदांशः_Is_Mid_Merge = m_isCoverSeq && !res.midSources.रिक्तानि();
        प्रत्यागम res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    Build_परिणाम buildPropAlways(AstPropAlways* nodep, SvaStateVertex* प्रविष्टिः_Vtxp,
                                तार्किक is_शिखरम्_Level_चरणः = असत्य) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव propp = nodep->propp();
        ध्रुव अचिह्नित lo = getConstUInt(nodep->loBoundp());
        यदि (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* ध्रुव livep = सङ्कलय_Delay_Chain(प्रविष्टिः_Vtxp, lo, flp);
            livep->m_isUnbounded = सत्य;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* ध्रुव sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = सत्य;
            SvaTransEdge* ध्रुव rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_वृक्षः_विशुद्धम्(असत्य)), flp);
            यदि (is_शिखरम्_Level_चरणः) rejEdgep->m_rejectOnFail = सत्य;
            प्रत्यागम {livep, शून्यसूचकः, {}};
        }
        ध्रुव अचिह्नित hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* currentp = सङ्कलय_Delay_Chain(प्रविष्टिः_Vtxp, lo, flp);
        SvaStateVertex* ध्रुव checkp = scopedCreateVertex();
        SvaTransEdge* ध्रुव linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_वृक्षः_विशुद्धम्(असत्य)), flp);
        यदि (is_शिखरम्_Level_चरणः) linkp->m_rejectOnFail = सत्य;
        currentp = checkp;
        m_temporal_Guard_राशिः.प्रक्षिप_पृष्ठभागः(propp);
        currentp = सङ्कलय_Delay_Chain(currentp, hi - lo, flp);
        यदि (nodep->isStrong() && currentp->m_delay_Ring_आकार) currentp->m_strongAlwaysRing = सत्य;
        m_temporal_Guard_राशिः.निष्कासय_पृष्ठभागः();
        प्रत्यागम {currentp, propp, {}};
    }

    Build_परिणाम buildGotoRep(AstSGotoRep* repp, SvaStateVertex* प्रविष्टिः_Vtxp) {
        File_पङ्क्तिः* ध्रुव flp = repp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = repp->exprp();
        ध्रुव अचिह्नित minN = getConstUInt(repp->countp());
        यदि (minN == 0) प्रत्यागम Build_परिणाम::fail();
        ध्रुव तार्किक hasMax = repp->maxCountp() != शून्यसूचकः;
        ध्रुव अचिह्नित maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        यदि (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            प्रत्यागम Build_परिणाम::failWithError();
        }

        AstVar* ध्रुव hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* ध्रुव waitVtxp = scopedCreateVertex();
        guardedEdge(प्रविष्टिः_Vtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    नूतन AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        यदि (minN > 1) {
            SvaStateVertex* ध्रुव ringVtxp = सङ्कलय_Delay_Chain(
                currentp, minN - 1, flp, सत्य, शून्यसूचकः, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = सत्य;
            SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        यदि (!hasMax) {
            currentp->m_isUnbounded = सत्य;  // [->N] waits unboundedly
            m_अन्तर्_Unbounded_Scope = सत्य;
            प्रत्यागम {currentp, शून्यसूचकः, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        यदि (maxN > minN) {
            SvaStateVertex* ध्रुव ringVtxp
                = सङ्कलय_Delay_Chain(currentp, maxN - minN + 1, flp, असत्य, शून्यसूचकः,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = सत्य;
            SvaStateVertex* ध्रुव checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = सत्य;  // [->M:N] still has unbounded waits between matches
        m_अन्तर्_Unbounded_Scope = सत्य;
        प्रत्यागम {mergeVtxp, शून्यसूचकः, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    स्थिर तार्किक hasZeroMinRep(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* seqp) {
        प्रत्यागम seqp->exists(
            [](ध्रुव AstSConsRep* repp) { प्रत्यागम getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    स्थिर तार्किक isPlainBoolean(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        प्रत्यागम !condp->exists(
            [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* np) { प्रत्यागम np->is_Multi_आवृत्तिः_Sva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    Build_परिणाम buildOrMerge(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, SvaStateVertex* प्रविष्टिः_Vtxp,
                             File_पङ्क्तिः* flp, तार्किक want_Bool_पर्णम्) {
        ध्रुव Build_परिणाम lhs = build_व्यञ्जकम्(lhsp, प्रविष्टिः_Vtxp);
        ध्रुव Build_परिणाम rhs = build_व्यञ्जकम्(rhsp, प्रविष्टिः_Vtxp);
        यदि (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            प्रत्यागम Build_परिणाम::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        ध्रुव तार्किक booleanOnly = lhs.पदांशः_Vertexp == प्रविष्टिः_Vtxp && lhs.finalCondp
                                 && rhs.पदांशः_Vertexp == प्रविष्टिः_Vtxp && rhs.finalCondp;
        यदि (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            प्रत्यागम Build_परिणाम::failWithError();
        }
        यदि (booleanOnly && want_Bool_पर्णम्) {
            यदि (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव orCondp
                    = नूतन AstLogOr{flp, lhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य),
                                   rhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                प्रत्यागम {प्रविष्टिः_Vtxp, orCondp, {}};
            }
        }
        SvaStateVertex* ध्रुव mergeVtxp = scopedCreateVertex();
        यदि (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(प्रविष्टिः_Vtxp, mergeVtxp,
                        नूतन AstLogOr{flp, sampled(lhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)),
                                     sampled(rhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य))},
                        flp);
        } अन्यथा {
            यदि (lhs.finalCondp) {
                guardedLink(lhs.पदांशः_Vertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)), flp);
            } अन्यथा {
                guardedLink(lhs.पदांशः_Vertexp, mergeVtxp, flp);
            }
            यदि (rhs.finalCondp) {
                guardedLink(rhs.पदांशः_Vertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)), flp);
            } अन्यथा {
                guardedLink(rhs.पदांशः_Vertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        प्रत्यागम {mergeVtxp, शून्यसूचकः, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    Build_परिणाम buildAndCombiner(Ast_ग्रन्थिः_व्यञ्जकम्* lhsExprp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsExprp,
                                 SvaStateVertex* प्रविष्टिः_Vtxp, File_पङ्क्तिः* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        ध्रुव तार्किक savedScope = m_अन्तर्_Unbounded_Scope;
        ध्रुव Build_परिणाम lhs = build_व्यञ्जकम्(lhsExprp, प्रविष्टिः_Vtxp);
        ध्रुव तार्किक lhsScope = m_अन्तर्_Unbounded_Scope;
        m_अन्तर्_Unbounded_Scope = savedScope;
        ध्रुव Build_परिणाम rhs = build_व्यञ्जकम्(rhsExprp, प्रविष्टिः_Vtxp);
        ध्रुव तार्किक rhsScope = m_अन्तर्_Unbounded_Scope;
        m_अन्तर्_Unbounded_Scope = savedScope || lhsScope || rhsScope;
        यदि (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            प्रत्यागम Build_परिणाम::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        यदि (lhs.पदांशः_Vertexp == प्रविष्टिः_Vtxp && rhs.पदांशः_Vertexp == प्रविष्टिः_Vtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstLogAnd{flp, lhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य),
                                                     rhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            प्रत्यागम {प्रविष्टिः_Vtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        यदि (!lhs.midSources.रिक्तानि() || !rhs.midSources.रिक्तानि()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            प्रत्यागम Build_परिणाम::failWithError();
        }
        SvaStateVertex* ध्रुव combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = सत्य;
        combVtxp->m_andLhsTermp = lhs.पदांशः_Vertexp;
        combVtxp->m_andRhsTermp = rhs.पदांशः_Vertexp;
        यदि (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य);
        यदि (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य);
        यदि (lhs.पदांशः_Vertexp->m_isUnbounded || rhs.पदांशः_Vertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = सत्य;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        यदि (!combVtxp->m_isUnbounded) {
            तार्किक needSink = असत्य;
            ध्रुव तार्किक lhs_Multi_आवृत्तिः = (lhs.पदांशः_Vertexp != प्रविष्टिः_Vtxp);
            ध्रुव तार्किक rhs_Multi_आवृत्तिः = (rhs.पदांशः_Vertexp != प्रविष्टिः_Vtxp);
            यदि (lhs.finalCondp && lhs_Multi_आवृत्तिः && !lhs.पदांशः_Vertexp->m_isUnbounded) {
                needSink = सत्य;
            }
            यदि (rhs.finalCondp && rhs_Multi_आवृत्तिः && !rhs.पदांशः_Vertexp->m_isUnbounded) {
                needSink = सत्य;
            }
            यदि (needSink) {
                SvaStateVertex* ध्रुव sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = सत्य;
                यदि (lhs.finalCondp && lhs_Multi_आवृत्तिः && !lhs.पदांशः_Vertexp->m_isUnbounded) {
                    SvaTransEdge* ध्रुव ep = m_graph.सङ्कलय_Link(
                        lhs.पदांशः_Vertexp, sinkVtxp, sampled(lhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)));
                    ep->m_rejectOnFail = सत्य;
                }
                यदि (rhs.finalCondp && rhs_Multi_आवृत्तिः && !rhs.पदांशः_Vertexp->m_isUnbounded) {
                    SvaTransEdge* ध्रुव ep = m_graph.सङ्कलय_Link(
                        rhs.पदांशः_Vertexp, sinkVtxp, sampled(rhs.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)));
                    ep->m_rejectOnFail = सत्य;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        प्रत्यागम {combVtxp, शून्यसूचकः, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    Build_परिणाम buildSWithin(AstSWithin* nodep, SvaStateVertex* प्रविष्टिः_Vtxp,
                             तार्किक is_शिखरम्_Level_चरणः = असत्य) {
        ध्रुव पूर्णाङ्क innerLen = fixed_दीर्घता(nodep->lhsp());
        ध्रुव पूर्णाङ्क outerLen = fixed_दीर्घता(nodep->rhsp());
        यदि (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            प्रत्यागम Build_परिणाम::failWithError();
        }
        यदि (innerLen > outerLen) {
            प्रत्यागम build_Never_मेलनम्_Intersect(
                nodep, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः,
                "the inner sequence is longer than the outer sequence");
        }
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        ध्रुव पूर्णाङ्क slack = outerLen - innerLen;
        Ast_ग्रन्थिः_व्यञ्जकम्* innerOrp = शून्यसूचकः;
        प्रति (पूर्णाङ्क i = 0; i <= slack; ++i) {
            ध्रुव पूर्णाङ्क postPad = slack - i;
            Ast_ग्रन्थिः_व्यञ्जकम्* branchp = nodep->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य);
            यदि (i > 0) {
                AstConst* ध्रुव prePadp = नूतन AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ध्रुव delayp
                    = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(i)}, सत्य};
                AstSExpr* ध्रुव wrapped = नूतन AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtype_समुच्चयः_Bit();
                branchp = wrapped;
            }
            यदि (postPad > 0) {
                AstConst* ध्रुव postTruep = नूतन AstConst{flp, AstConst::BitTrue{}};
                AstDelay* ध्रुव delayp
                    = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(postPad)}, सत्य};
                AstSExpr* ध्रुव wrapped = नूतन AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtype_समुच्चयः_Bit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव outerClonep = nodep->rhsp()->clone_वृक्षः_विशुद्धम्(असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव combinedp = नूतन AstSIntersect{flp, innerOrp, outerClonep};
        Build_परिणाम परिणाम = build_व्यञ्जकम्(combinedp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        VL_DO_DANGLING(combinedp->delete_वृक्षः(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        यदि (परिणाम.valid() && परिणाम.finalCondp && !परिणाम.finalCondp->backp()) {
            SvaStateVertex* ध्रुव wrapVtxp = scopedCreateVertex();
            guardedLink(परिणाम.पदांशः_Vertexp, wrapVtxp, sampled(परिणाम.finalCondp), flp);
            परिणाम = {wrapVtxp, शून्यसूचकः, परिणाम.midSources};
        }
        प्रत्यागम परिणाम;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    स्थिर तार्किक flattenFixedSeq(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क आधारः_विस्थापनम्,
                                std::map<पूर्णाङ्क, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>>& बहिर्गमनम्) {
        यदि (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
            यदि (!delayp || !delayp->is_आवृत्तिः_Delay() || delayp->isUnbounded()) प्रत्यागम असत्य;
            ध्रुव अचिह्नित delay_आवृत्तयः = getConstUInt(delayp->lhsp());
            यदि (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delay_आवृत्तयः)
                प्रत्यागम असत्य;
            पूर्णाङ्क preLen = 0;
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp()) {
                यदि (!flattenFixedSeq(prep, आधारः_विस्थापनम्, बहिर्गमनम्)) प्रत्यागम असत्य;
                preLen = fixed_दीर्घता(prep);
                यदि (preLen < 0) प्रत्यागम असत्य;
            }
            प्रत्यागम flattenFixedSeq(sexprp->exprp(), आधारः_विस्थापनम् + preLen + delay_आवृत्तयः, बहिर्गमनम्);
        }
        यदि (nodep->is_Multi_आवृत्तिः_Sva()) प्रत्यागम असत्य;
        बहिर्गमनम्[आधारः_विस्थापनम्].प्रक्षिप_पृष्ठभागः(nodep);
        प्रत्यागम सत्य;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* conjoinFixedSeqs(Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp, File_पङ्क्तिः* flp) {
        std::map<पूर्णाङ्क, std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*>> checks;
        यदि (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) प्रत्यागम शून्यसूचकः;
        यदि (checks.रिक्तानि()) प्रत्यागम शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = शून्यसूचकः;
        पूर्णाङ्क पूर्व_विस्थापनम् = 0;
        प्रति (ध्रुव स्वचालित& विस्थापनम्_Checks : checks) {
            ध्रुव पूर्णाङ्क विस्थापनम् = विस्थापनम्_Checks.first;
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः;
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव leafp : विस्थापनम्_Checks.second) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = leafp->clone_वृक्षः_विशुद्धम्(असत्य);
                यदि (!condp) {
                    condp = clonep;
                } अन्यथा {
                    condp = नूतन AstLogAnd{flp, condp, clonep};
                    condp->dtype_समुच्चयः_Bit();
                }
            }
            यदि (!resultp) {
                यदि (विस्थापनम् > 0) {
                    AstDelay* ध्रुव delayp = नूतन AstDelay{
                        flp, नूतन AstConst{flp, static_cast<uint32_t>(विस्थापनम्)}, /*isCycle=*/सत्य};
                    resultp
                        = नूतन AstSExpr{flp, नूतन AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtype_समुच्चयः_Bit();
                } अन्यथा {
                    resultp = condp;
                }
            } अन्यथा {
                AstDelay* ध्रुव delayp = नूतन AstDelay{
                    flp, नूतन AstConst{flp, static_cast<uint32_t>(विस्थापनम् - पूर्व_विस्थापनम्)},
                    /*isCycle=*/सत्य};
                resultp = नूतन AstSExpr{flp, resultp, delayp, condp};
                resultp->dtype_समुच्चयः_Bit();
            }
            पूर्व_विस्थापनम् = विस्थापनम्;
        }
        प्रत्यागम resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    संरचना SimpleRanged final {
        तार्किक ok = असत्य;
        Ast_ग्रन्थिः_व्यञ्जकम्* startp = शून्यसूचकः;  // may be null (absent start)
        Ast_ग्रन्थिः_व्यञ्जकम्* endp = शून्यसूचकः;
    };
    स्थिर SimpleRanged asSimpleRanged(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr);
        यदि (!sexprp) प्रत्यागम {};
        AstDelay* ध्रुव delayp = VN_CAST(sexprp->delayp(), Delay);
        यदि (!delayp || !delayp->is_आवृत्तिः_Delay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            प्रत्यागम {};
        यदि (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) प्रत्यागम {};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp();
        यदि (prep && fixed_दीर्घता(prep) != 0) प्रत्यागम {};
        यदि (fixed_दीर्घता(sexprp->exprp()) != 0) प्रत्यागम {};
        प्रत्यागम {सत्य, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    Build_परिणाम build_From_Lowering_वृक्षः(Ast_ग्रन्थिः_व्यञ्जकम्* treep, SvaStateVertex* प्रविष्टिः_Vtxp,
                                      तार्किक is_शिखरम्_Level_चरणः) {
        Build_परिणाम परिणाम = build_व्यञ्जकम्(treep, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        यदि (परिणाम.valid() && परिणाम.finalCondp) {
            परिणाम.finalCondp = परिणाम.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य);
        }
        VL_DO_DANGLING(treep->delete_वृक्षः(), treep);
        प्रत्यागम परिणाम;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    Build_परिणाम build_Never_मेलनम्_Intersect(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, SvaStateVertex* प्रविष्टिः_Vtxp,
                                         तार्किक is_शिखरम्_Level_चरणः, ध्रुव वर्ण* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव falsep = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        प्रत्यागम build_From_Lowering_वृक्षः(falsep, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
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
    Build_परिणाम buildVarLenIntersect(AstSIntersect* nodep, SvaStateVertex* प्रविष्टिः_Vtxp,
                                     तार्किक is_शिखरम्_Level_चरणः) {
        ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क> lhsRange = दीर्घता_Range(nodep->lhsp());
        ध्रुव std::pair<पूर्णाङ्क, पूर्णाङ्क> rhsRange = दीर्घता_Range(nodep->rhsp());
        यदि (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            प्रत्यागम Build_परिणाम::failWithError();
        }
        ध्रुव पूर्णाङ्क lo = std::max(lhsRange.first, rhsRange.first);
        ध्रुव पूर्णाङ्क hi = std::min(lhsRange.second, rhsRange.second);
        यदि (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            प्रत्यागम build_Never_मेलनम्_Intersect(nodep, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः,
                                            "intersect operands have no common length");
        }
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        यदि (lo == hi) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lp = realize_At_दीर्घता(nodep->lhsp(), lo, lhsRange.first);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rp = realize_At_दीर्घता(nodep->rhsp(), lo, rhsRange.first);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_वृक्षः(), lp);
            VL_DO_DANGLING(rp->delete_वृक्षः(), rp);
            यदि (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                प्रत्यागम Build_परिणाम::failWithError();
            }
            प्रत्यागम build_From_Lowering_वृक्षः(conjp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        }
        ध्रुव SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        ध्रुव SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        यदि (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            प्रत्यागम Build_परिणाम::failWithError();
        }
        ध्रुव स्वचालित andBool = [&](Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp) -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव aClonep
                = ap ? ap->clone_वृक्षः_विशुद्धम्(असत्य) : नूतन AstConst{flp, AstConst::BitTrue{}};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bClonep
                = bp ? bp->clone_वृक्षः_विशुद्धम्(असत्य) : नूतन AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* ध्रुव andp = नूतन AstLogAnd{flp, aClonep, bClonep};
            andp->dtype_समुच्चयः_Bit();
            प्रत्यागम andp;
        };
        AstDelay* ध्रुव delayp = नूतन AstDelay{flp, नूतन AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/सत्य};
        delayp->rhsp(नूतन AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* ध्रुव reducedp
            = नूतन AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtype_समुच्चयः_Bit();
        प्रत्यागम build_From_Lowering_वृक्षः(reducedp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
    }

    Build_परिणाम buildThroughout(AstSThroughout* nodep, SvaStateVertex* प्रविष्टिः_Vtxp,
                                तार्किक is_शिखरम्_Level_चरणः = असत्य) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        प्रविष्टिः_Vtxp->m_throughoutConds.प्रक्षिप_पृष्ठभागः(nodep->lhsp()->clone_वृक्षः_विशुद्धम्(असत्य));
        m_temporal_Guard_राशिः.प्रक्षिप_पृष्ठभागः(nodep->lhsp());
        ध्रुव Build_परिणाम परिणाम = build_व्यञ्जकम्(nodep->rhsp(), प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        m_temporal_Guard_राशिः.निष्कासय_पृष्ठभागः();
        प्रत्यागम परिणाम;
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
    Build_परिणाम buildUntil(AstUntil* nodep, SvaStateVertex* प्रविष्टिः_Vtxp, तार्किक is_शिखरम्_Level_चरणः) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        यदि (!is_शिखरम्_Level_चरणः) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            प्रत्यागम Build_परिणाम::failWithError();
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
        ध्रुव स्वचालित hasSeq = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ep) {
            प्रत्यागम ep->exists([](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* np) { प्रत्यागम np->is_Multi_आवृत्तिः_Sva(); });
        };
        यदि (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            प्रत्यागम Build_परिणाम::failWithError();
        }

        ध्रुव तार्किक ov = nodep->isOverlapping();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsBitp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* ध्रुव pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* ध्रुव qHoistp = ov ? शून्यसूचकः : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* ध्रुव waitCp = scopedCreateVertex();
        SvaStateVertex* ध्रुव waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = सत्य;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(प्रविष्टिः_Vtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contCondp
            = नूतन AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            नूतन AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* ध्रुव sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = सत्य;
        Ast_ग्रन्थिः_व्यञ्जकम्* requireCondp;
        यदि (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } अन्यथा {
            requireCondp = नूतन AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* ध्रुव rejEdgep = m_graph.सङ्कलय_Link(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = सत्य;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_ग्रन्थिः_व्यञ्जकम्* acceptCondp;
        यदि (ov) {
            acceptCondp
                = नूतन AstLogAnd{flp, lhsBitp->clone_वृक्षः_विशुद्धम्(असत्य), rhsBitp->clone_वृक्षः_विशुद्धम्(असत्य)};
        } अन्यथा {
            acceptCondp = rhsBitp->clone_वृक्षः_विशुद्धम्(असत्य);
        }
        प्रत्यागम {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_ग्रन्थिः_व्यञ्जकम्* abort_Fire_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* condp, File_पङ्क्तिः* flp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = condp->clone_वृक्षः_विशुद्धम्(असत्य);
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव op : m_outer_Abort_राशिः)
            resultp = नूतन AstLogAnd{flp, resultp, नूतन AstLogNot{flp, op->clone_वृक्षः_विशुद्धम्(असत्य)}};
        प्रत्यागम resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    स्थिर तार्किक chainAccountsSource(ध्रुव SvaStateVertex* srcp,
                                    ध्रुव std::unordered_समुच्चयः<ध्रुव V3GraphEdge*>& pre_धाराः) {
        यदि (srcp->m_delay_Ring_आकार && srcp->m_throughoutConds.रिक्तानि()) प्रत्यागम सत्य;
        प्रति (ध्रुव V3GraphEdge& edger : srcp->अन्तर्_धाराः()) {
            यदि (pre_धाराः.गणनम्(&edger)) अनुवर्त;
            ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
            यदि (tedger.m_consumes_आवृत्तिः) अनुवर्त;
            ध्रुव स्वचालित* ध्रुव fromp = static_cast<ध्रुव SvaStateVertex*>(tedger.fromVtxp());
            यदि (fromp->m_abortRejectp) प्रत्यागम सत्य;
        }
        तार्किक plainNonSink = असत्य;
        तार्किक markedSink = असत्य;
        प्रति (ध्रुव V3GraphEdge& edger : srcp->बहिर्गमनम्_धाराः()) {
            यदि (pre_धाराः.गणनम्(&edger)) अनुवर्त;
            ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
            यदि (tedger.m_consumes_आवृत्तिः) अनुवर्त;
            ध्रुव तार्किक sink = static_cast<ध्रुव SvaStateVertex*>(tedger.गन्तव्यम्_Vtxp())->m_isRejectSink;
            यदि (tedger.m_rejectOnFail) {
                यदि (!sink) प्रत्यागम सत्य;
                markedSink = सत्य;
            } अन्यथा यदि (!sink) {
                यदि (srcp->m_abortRejectp) प्रत्यागम सत्य;
                plainNonSink = सत्य;
            }
        }
        प्रत्यागम plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    शून्य सङ्कलय_Abort_Reject_Edge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_ग्रन्थिः_व्यञ्जकम्* condp,
                            File_पङ्क्तिः* flp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notFirep = नूतन AstLogNot{flp, sampled(abort_Fire_व्यञ्जकम्(condp, flp))};
        m_graph.सङ्कलय_Link(srcp, sinkp, notFirep)->m_rejectOnFail = सत्य;
        प्रत्यागम;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    शून्य gate_Body_धाराः_On_Abort(ध्रुव std::unordered_समुच्चयः<ध्रुव V3GraphEdge*>& pre_धाराः,
                              Ast_ग्रन्थिः_व्यञ्जकम्* condp, VAbort_विधा विधा, File_पङ्क्तिः* flp) {
        प्रति (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            प्रति (V3GraphEdge& edger : vtxr.बहिर्गमनम्_धाराः()) {
                यदि (pre_धाराः.गणनम्(&edger)) अनुवर्त;
                SvaTransEdge* ध्रुव tedgep = static_cast<SvaTransEdge*>(&edger);
                यदि (tedgep->m_rejectOnFail) {
                    यदि (!विधा.isAccept()) अनुवर्त;
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firep = sampled(abort_Fire_व्यञ्जकम्(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? नूतन AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } अन्यथा यदि (tedgep->m_consumes_आवृत्तिः) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notFirep
                        = नूतन AstLogNot{flp, sampled(abort_Fire_व्यञ्जकम्(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? नूतन AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    Build_परिणाम buildAbortOn(Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः_व्यञ्जकम्* bodyp, SvaStateVertex* प्रविष्टिः_Vtxp,
                             VAbort_विधा विधा, File_पङ्क्तिः* flp, तार्किक is_शिखरम्_Level_चरणः) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_समुच्चयः<ध्रुव V3GraphVertex*> preExisting;
        std::unordered_समुच्चयः<ध्रुव V3GraphEdge*> pre_धाराः;
        प्रति (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.निवेशय(&vtxr);
            प्रति (V3GraphEdge& edger : vtxr.बहिर्गमनम्_धाराः()) pre_धाराः.निवेशय(&edger);
        }

        m_outer_Abort_राशिः.प्रक्षिप_पृष्ठभागः(condp);
        ध्रुव Build_परिणाम body_परिणाम = build_व्यञ्जकम्(bodyp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        m_outer_Abort_राशिः.निष्कासय_पृष्ठभागः();
        यदि (!body_परिणाम.valid()) प्रत्यागम body_परिणाम;

        gate_Body_धाराः_On_Abort(pre_धाराः, condp, विधा, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::सदिशम्<SvaStateVertex*> abortSources;
        abortSources.प्रक्षिप_पृष्ठभागः(प्रविष्टिः_Vtxp);
        प्रति (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            यदि (preExisting.गणनम्(&vtxr)) अनुवर्त;
            स्वचालित* ध्रुव sp = static_cast<SvaStateVertex*>(&vtxr);
            यदि (sp->m_delay_Ring_आकार || !sp->m_throughoutConds.रिक्तानि()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firep = abort_Fire_व्यञ्जकम्(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? नूतन AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            यदि (!विधा.isAccept()
                && ((sp->m_delay_Ring_आकार && sp->m_throughoutConds.रिक्तानि())
                    || sp->m_replayAbortReject)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव firep = abort_Fire_व्यञ्जकम्(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? नूतन AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            यदि (sp->m_isRejectSink) अनुवर्त;
            abortSources.प्रक्षिप_पृष्ठभागः(sp);
        }

        स्वचालित sampledAbortFire = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव व्यञ्जकम् = abort_Fire_व्यञ्जकम्(condp, flp);
            प्रत्यागम sampled(व्यञ्जकम्);
        };

        यदि (विधा.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* ध्रुव acceptSinkp = scopedCreateVertex();
            प्रति (SvaStateVertex* ध्रुव srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::सदिशम्<SvaStateVertex*> midSources = body_परिणाम.midSources;
            midSources.प्रक्षिप_पृष्ठभागः(acceptSinkp);
            Ast_ग्रन्थिः_व्यञ्जकम्* finalCondp = body_परिणाम.finalCondp;
            यदि (finalCondp) {
                यदि (finalCondp->backp()) finalCondp = finalCondp->clone_वृक्षः_विशुद्धम्(असत्य);
                finalCondp = नूतन AstLogOr{flp, finalCondp, abort_Fire_व्यञ्जकम्(condp, flp)};
            }
            प्रत्यागम {body_परिणाम.पदांशः_Vertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* ध्रुव rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = सत्य;
        प्रति (SvaStateVertex* ध्रुव srcp : abortSources)
            यदि (!chainAccountsSource(srcp, pre_धाराः))
                सङ्कलय_Abort_Reject_Edge(srcp, rejectSinkp, condp, flp);
        Ast_ग्रन्थिः_व्यञ्जकम्* finalCondp = body_परिणाम.finalCondp;
        यदि (finalCondp) {
            यदि (finalCondp->backp()) finalCondp = finalCondp->clone_वृक्षः_विशुद्धम्(असत्य);
            finalCondp
                = नूतन AstLogAnd{flp, finalCondp, नूतन AstLogNot{flp, abort_Fire_व्यञ्जकम्(condp, flp)}};
        }
        प्रत्यागम {body_परिणाम.पदांशः_Vertexp, finalCondp, body_परिणाम.midSources};
    }

सार्वजनिक:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    स्थिर शून्य freeUnlinkedCondp(Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        यदि (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_वृक्षः(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_ग्रन्थिः_Module* modp, V3_Unique_नामानि& prop_अस्थायी_नामानि,
                  तार्किक isCoverSeq = असत्य, तार्किक isSeqEvent = असत्य, तार्किक isCover = असत्य)
        : m_graph{graph}
        , m_modp{modp}
        , m_prop_अस्थायी_नामानि{prop_अस्थायी_नामानि}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    शून्य resetScope() {
        m_अन्तर्_Unbounded_Scope = असत्य;
        m_temporal_Guard_राशिः.clear();
    }

    Build_परिणाम build_व्यञ्जकम्(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, SvaStateVertex* प्रविष्टिः_Vtxp,
                          तार्किक is_शिखरम्_Level_चरणः = असत्य) {
        VL_RESTORER(m_want_Bool_पर्णम्);
        ध्रुव तार्किक want_Bool_पर्णम् = m_want_Bool_पर्णम्;
        m_want_Bool_पर्णम् = असत्य;
        यदि (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            प्रत्यागम buildSExpr(sexprp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        }
        यदि (AstSConsRep* ध्रुव repp = VN_CAST(nodep, SConsRep)) {
            प्रत्यागम buildConsRep(repp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        }
        यदि (AstPropAlways* ध्रुव alwaysp = VN_CAST(nodep, PropAlways)) {
            प्रत्यागम buildPropAlways(alwaysp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        }
        यदि (AstSGotoRep* ध्रुव repp = VN_CAST(nodep, SGotoRep)) {
            प्रत्यागम buildGotoRep(repp, प्रविष्टिः_Vtxp);
        }
        यदि (AstSThroughout* ध्रुव throughoutp = VN_CAST(nodep, SThroughout)) {
            प्रत्यागम buildThroughout(throughoutp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        }
        यदि (AstSOr* ध्रुव orp = VN_CAST(nodep, SOr)) {
            प्रत्यागम buildOrMerge(orp->lhsp(), orp->rhsp(), प्रविष्टिः_Vtxp, orp->fileline(),
                                want_Bool_पर्णम्);
        }
        यदि (AstLogOr* ध्रुव orp = VN_CAST(nodep, LogOr)) {
            प्रत्यागम buildOrMerge(orp->lhsp(), orp->rhsp(), प्रविष्टिः_Vtxp, orp->fileline(),
                                want_Bool_पर्णम्);
        }
        यदि (AstSAnd* ध्रुव andp = VN_CAST(nodep, SAnd)) {
            प्रत्यागम buildAndCombiner(andp->lhsp(), andp->rhsp(), प्रविष्टिः_Vtxp, andp->fileline());
        }
        यदि (AstSIntersect* ध्रुव intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            ध्रुव पूर्णाङ्क lhsLen = fixed_दीर्घता(intp->lhsp());
            ध्रुव पूर्णाङ्क rhsLen = fixed_दीर्घता(intp->rhsp());
            यदि (lhsLen >= 0 && rhsLen >= 0) {
                यदि (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    प्रत्यागम build_Never_मेलनम्_Intersect(intp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः,
                                                    "intersect operands have no common length");
                }
                यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    प्रत्यागम build_From_Lowering_वृक्षः(conjp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
                }
                प्रत्यागम buildAndCombiner(intp->lhsp(), intp->rhsp(), प्रविष्टिः_Vtxp, intp->fileline());
            }
            प्रत्यागम buildVarLenIntersect(intp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        }
        यदि (AstSWithin* ध्रुव withinp = VN_CAST(nodep, SWithin)) {
            प्रत्यागम buildSWithin(withinp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        }
        यदि (AstAbortOn* ध्रुव ap = VN_CAST(nodep, AbortOn)) {
            प्रत्यागम buildAbortOn(ap->condp(), ap->propp(), प्रविष्टिः_Vtxp, ap->विधा(), ap->fileline(),
                                is_शिखरम्_Level_चरणः);
        }
        यदि (VN_IS(nodep, SNonConsRep)) प्रत्यागम Build_परिणाम::fail();
        यदि (AstImplication* ध्रुव implp = VN_CAST(nodep, Implication)) {
            प्रत्यागम build_Implication_धाराः(implp->lhsp(), implp->rhsp(), प्रविष्टिः_Vtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        यदि (AstUntil* ध्रुव untilp = VN_CAST(nodep, Until)) {
            प्रत्यागम buildUntil(untilp, प्रविष्टिः_Vtxp, is_शिखरम्_Level_चरणः);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        प्रत्यागम {प्रविष्टिः_Vtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    Build_परिणाम build_Implication_धाराः(Ast_ग्रन्थिः_व्यञ्जकम्* antExprp, Ast_ग्रन्थिः_व्यञ्जकम्* bodyExprp,
                                      SvaStateVertex* प्रविष्टिः_Vtxp, तार्किक isOverlapped,
                                      तार्किक isFollowedBy, Ast_ग्रन्थिः* errorNodep, File_पङ्क्तिः* flp) {
        ध्रुव Build_परिणाम ant_परिणाम = build_व्यञ्जकम्(antExprp, प्रविष्टिः_Vtxp);
        यदि (!ant_परिणाम.valid()) प्रत्यागम ant_परिणाम;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        यदि (isFollowedBy && ant_परिणाम.पदांशः_Vertexp != प्रविष्टिः_Vtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            प्रत्यागम Build_परिणाम::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || ant_परिणाम.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* ध्रुव trigVtxp = m_graph.createStateVertex();
        यदि (ant_परिणाम.finalCondp) {
            m_graph.सङ्कलय_Link(ant_परिणाम.पदांशः_Vertexp, trigVtxp,
                            sampled(ant_परिणाम.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            यदि (isFollowedBy) {
                SvaStateVertex* ध्रुव sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = सत्य;
                SvaTransEdge* ध्रुव ep
                    = m_graph.सङ्कलय_Link(ant_परिणाम.पदांशः_Vertexp, sinkVtxp,
                                      sampled(ant_परिणाम.finalCondp->clone_वृक्षः_विशुद्धम्(असत्य)));
                ep->m_rejectOnFail = सत्य;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            यदि (!ant_परिणाम.finalCondp->backp()) {
                VL_DO_DANGLING(ant_परिणाम.finalCondp->delete_वृक्षः(), ant_परिणाम.finalCondp);
            }
        } अन्यथा {
            m_graph.सङ्कलय_Link(ant_परिणाम.पदांशः_Vertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        यदि (!isOverlapped) {
            SvaStateVertex* ध्रुव delayVtxp = m_graph.createStateVertex();
            m_graph.सङ्कलय_Clocked_Edge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_want_Bool_पर्णम्);
        m_want_Bool_पर्णम् = !isFollowedBy && !hasZeroMinRep(antExprp);
        प्रत्यागम build_व्यञ्जकम्(bodyExprp, bodyEntryp, /*isTopLevelStep=*/सत्य);
    }

    Build_परिणाम build(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_want_Bool_पर्णम्);
        m_want_Bool_पर्णम् = सत्य;
        प्रत्यागम build_व्यञ्जकम्(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/सत्य);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

वर्ग SvaNfaLowering final {
    Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module to add state vars and always blocks to
    Ast_ग्रन्थिः_DType* ध्रुव m_u32DTypep;  // Shared unsigned counter dtype
    V3_Unique_नामानि m_नामानि{"__Vnfa"};
    आकार_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    संरचना अधस्तनम्_Ctx final {
        File_पङ्क्तिः* flp;  // Source location for generated AST
        Ast_Sen_वृक्षः* senTreep;  // Clock sensitivity tree
        Ast_ग्रन्थिः_व्यञ्जकम्* disableExprp;  // disable iff expression (may be nullptr)
        Ast_ग्रन्थिः_व्यञ्जकम्* मेलनम्_Condp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* build_मेलनम्_Now(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* stateExprp, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव statep = stateExprp->clone_वृक्षः_विशुद्धम्(असत्य);
        यदि (!condp) प्रत्यागम statep;
        प्रत्यागम नूतन AstLogAnd{flp, statep, sampled(condp->clone_वृक्षः_विशुद्धम्(असत्य))};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* andCond(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        यदि (!condp) प्रत्यागम exprp;
        प्रत्यागम नूतन AstLogAnd{flp, exprp, condp->clone_वृक्षः_विशुद्धम्(असत्य)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* orExprs(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* ap, Ast_ग्रन्थिः_व्यञ्जकम्* bp) {
        यदि (!ap) प्रत्यागम bp;
        प्रत्यागम नूतन AstLogOr{flp, ap, bp};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* सङ्कलय_Countp(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव योग_Countp,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionp,
                                  Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep = शून्यसूचकः) {
        // contribution = enable ? contribution : 0;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enabledContributionp
            = enablep ? नूतन AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        यदि (!योग_Countp) प्रत्यागम enabledContributionp;
        प्रत्यागम नूतन Ast_सङ्कलय{flp, योग_Countp, enabledContributionp};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* gateCheckedCountp(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव countp,
                                          Ast_ग्रन्थिः_व्यञ्जकम्*& overflowp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep) {
        यदि (overflowp) {
            overflowp = नूतन AstLogAnd{flp, enablep->clone_वृक्षः_विशुद्धम्(असत्य), overflowp};
        }
        प्रत्यागम सङ्कलय_Countp(flp, शून्यसूचकः, countp, enablep);
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* सङ्कलय_Checked_Countp(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव योग_Countp,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionp, Ast_ग्रन्थिः_व्यञ्जकम्*& overflowp,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionOverflowp = शून्यसूचकः) {
        यदि (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        यदि (!योग_Countp) प्रत्यागम contributionp;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oldTotalp = योग_Countp->clone_वृक्षः_विशुद्धम्(असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp = नूतन Ast_सङ्कलय{flp, योग_Countp, contributionp};
        overflowp
            = orExprs(flp, overflowp, नूतन AstLt{flp, resultp->clone_वृक्षः_विशुद्धम्(असत्य), oldTotalp});
        प्रत्यागम resultp;
    }
    AstIf* new_गणनम्_अतिप्रवाहः_Warning(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp) ध्रुव {
        AstDisplay* ध्रुव dispp = नूतन AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            शून्यसूचकः, शून्यसूचकः};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        प्रत्यागम ifp;
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* kill_सक्रिय(अधस्तनम्_Ctx& c) {
        प्रत्यागम नूतन AstNeq{c.flp, नूतन AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* not_Kill_सक्रिय(अधस्तनम्_Ctx& c) { प्रत्यागम नूतन AstLogNot{c.flp, kill_सक्रिय(c)}; }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* gateNotKill(अधस्तनम्_Ctx& c, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        यदि (!exprp) प्रत्यागम शून्यसूचकः;
        प्रत्यागम नूतन AstLogAnd{c.flp, exprp, not_Kill_सक्रिय(c)};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* अग्र_Ring_सूचकांक(File_पङ्क्तिः* flp, Ast_ग्रन्थिः_व्यञ्जकम्* idxExprp, uint32_t आकार) {
        ध्रुव स्वचालित u32Const = [flp](uint32_t मानम्) {
            प्रत्यागम नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, मानम्};
        };
        UASSERT_OBJ(आकार > 0, idxExprp, "Ring size must be positive");
        यदि (आकार == 1) {
            VL_DO_DANGLING(idxExprp->delete_वृक्षः(), idxExprp);
            प्रत्यागम u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        Ast_सङ्कलय* ध्रुव addp = नूतन Ast_सङ्कलय{flp, idxExprp->clone_वृक्षः_विशुद्धम्(असत्य), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* ध्रुव condp
            = नूतन AstCond{flp, नूतन AstEq{flp, idxExprp, u32Const(आकार - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        प्रत्यागम condp;
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delayRingSlot(File_पङ्क्तिः* flp, SvaStateVertex* vtxp, Ast_ग्रन्थिः_व्यञ्जकम्* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* ध्रुव ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, ringp, access};
        यदि (vtxp->datap()->delayRingCounts) प्रत्यागम नूतन AstArraySel{flp, refp, idxExprp};
        प्रत्यागम नूतन AstSel{flp, refp, idxExprp, 1};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delay_Ring_शीर्ष_गणनम्(File_पङ्क्तिः* flp, SvaStateVertex* vtxp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव headp
            = नूतन AstVarRef{flp, vtxp->datap()->delay_Ring_शीर्ष_Varp, VAccess::READ};
        यदि (vtxp->datap()->delayRingCounts) प्रत्यागम headp;
        प्रत्यागम नूतन AstExtend{flp, headp, vtxp->datap()->delay_Ring_जीवितम्_गणनम्_Varp->dtypep()->विस्तारः()};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delay_Ring_At_चरमः_सूचकांक(File_पङ्क्तिः* ध्रुव flp, AstVar* ध्रुव idxp,
                                             ध्रुव uint32_t आकार) {
        प्रत्यागम नूतन AstEq{flp, नूतन AstVarRef{flp, idxp, VAccess::READ},
                         नूतन AstConst{flp, AstConst::Widthed_मानम्{}, 32, आकार - 1}};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delay_Ring_Output_गणनम्(File_पङ्क्तिः* ध्रुव flp, SvaStateVertex* ध्रुव vtxp) {
        AstVar* ध्रुव idxp = vtxp->datap()->delayRingIdxVarp;
        ध्रुव uint32_t आकार = vtxp->m_delay_Ring_आकार;
        Ast_ग्रन्थिः_व्यञ्जकम्* outgoingValidp
            = नूतन AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        यदि (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = नूतन AstLogOr{flp, outgoingValidp, delay_Ring_At_चरमः_सूचकांक(flp, idxp, आकार)};
        }
        AstVar* ध्रुव जीवितम्_गणनम्_Varp = vtxp->datap()->delay_Ring_जीवितम्_गणनम्_Varp;
        प्रत्यागम नूतन AstCond{flp, outgoingValidp, delay_Ring_शीर्ष_गणनम्(flp, vtxp),
                           newTypedConstp(flp, जीवितम्_गणनम्_Varp->dtypep(), 0)};
    }
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* delay_Ring_Has_जीवितम्_Threadsp(File_पङ्क्तिः* ध्रुव flp,
                                                 AstVar* ध्रुव जीवितम्_गणनम्_Varp) {
        // active = live_count != 0;
        प्रत्यागम नूतन AstNeq{flp, नूतन AstVarRef{flp, जीवितम्_गणनम्_Varp, VAccess::READ},
                          newTypedConstp(flp, जीवितम्_गणनम्_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    संरचना Signal_समुच्चयः final {
        Ast_ग्रन्थिः_व्यञ्जकम्* terminalActivep = शून्यसूचकः;  // OR of all successful terminal matches
        Ast_ग्रन्थिः_व्यञ्जकम्* मेलनम्_Countp = शून्यसूचकः;  // NFA paths completing the sequence this tick
        Ast_ग्रन्थिः_व्यञ्जकम्* मेलनम्_गणनम्_Overflowp = शून्यसूचकः;  // Match count overflow this tick
        Ast_ग्रन्थिः_व्यञ्जकम्* rejectBasep = शून्यसूचकः;  // Reject when a terminal match fails
        Ast_ग्रन्थिः_व्यञ्जकम्* required_चरणः_Rejectp = शून्यसूचकः;  // Per-source reject from rejectOnFail Links
        Ast_ग्रन्थिः_व्यञ्जकम्* throughoutRejectp = शून्यसूचकः;  // Reject when a throughout guard drops
        Ast_ग्रन्थिः_व्यञ्जकम्* threadFailCountp = शून्यसूचकः;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    स्थिर तार्किक isAsyncDisable(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disableExprp) {
        प्रत्यागम disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](ध्रुव AstSampled*) { प्रत्यागम सत्य; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    शून्य सङ्कलय_State_Always(अधस्तनम्_Ctx& c, Ast_ग्रन्थिः* bodyp, Ast_ग्रन्थिः* clearp) {
        m_modp->सङ्कलय_Stmtsp(
            नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्षः(असत्य), bodyp});
        यदि (!clearp) प्रत्यागम;
        m_modp->सङ्कलय_Stmtsp(नूतन AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            नूतन Ast_Sen_वृक्षः{c.flp, नूतन AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    शून्य emit_State_पञ्जिकाकोशः_Nba(अधस्तनम्_Ctx& c) {
        ध्रुव तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_ग्रन्थिः* bodyp = शून्यसूचकः;
        Ast_ग्रन्थिः* clearp = शून्यसूचकः;
        प्रति (ध्रुव V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ध्रुव SvaStateVertex* ध्रुव vtxp = static_cast<ध्रुव SvaStateVertex*>(&vtxr);
            यदि (!vtxp->datap()->stateVarp) अनुवर्त;

            Ast_ग्रन्थिः_व्यञ्जकम्* अग्र_Statep = शून्यसूचकः;
            प्रति (ध्रुव V3GraphEdge& edger : vtxp->अन्तर्_धाराः()) {
                ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
                यदि (!tedger.m_consumes_आवृत्तिः) अनुवर्त;
                ध्रुव SvaStateVertex* ध्रुव fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_ग्रन्थिः_व्यञ्जकम्* srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                यदि (c.disableExprp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                        = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                    srcSigp = नूतन AstLogAnd{c.flp, srcSigp, notDisp};
                }
                अग्र_Statep = orExprs(c.flp, अग्र_Statep, srcSigp);
            }

            UASSERT_OBJ(अग्र_Statep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            अग्र_Statep = gateNotKill(c, अग्र_Statep);

            Ast_नियोजय_Dly* ध्रुव assignp = नूतन Ast_नियोजय_Dly{
                c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, अग्र_Statep};
            bodyp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(bodyp, assignp);
            यदि (asyncDisable) {
                clearp = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(
                    clearp,
                    नूतन Ast_नियोजय_Dly{
                        c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        नूतन AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        यदि (bodyp) सङ्कलय_State_Always(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    शून्य emitDelayRingUpdates(अधस्तनम्_Ctx& c, ध्रुव तार्किक गणनम्_Matches) {
        ध्रुव तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        प्रति (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
            यदि (!vtxp->datap()->delayRingVarp) अनुवर्त;
            AstVar* ध्रुव idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* ध्रुव जीवितम्_गणनम्_Varp = vtxp->datap()->delay_Ring_जीवितम्_गणनम्_Varp;
            AstVar* ध्रुव wrappedp = vtxp->datap()->delayRingWrappedVarp;
            ध्रुव uint32_t आकार = static_cast<uint32_t>(vtxp->m_delay_Ring_आकार);

            Ast_ग्रन्थिः_व्यञ्जकम्* incomingp = शून्यसूचकः;
            Ast_ग्रन्थिः_व्यञ्जकम्* incomingOverflowp = शून्यसूचकः;
            प्रति (ध्रुव V3GraphEdge& edger : vtxp->अन्तर्_धाराः()) {
                ++m_statDelayRingEdgeVisits;
                ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumes_आवृत्तिः == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                ध्रुव SvaStateVertex* ध्रुव fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!गणनम्_Matches || fromVtxp->datap()->state_गणनम्_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_ग्रन्थिः_व्यञ्जकम्* contribp
                    = गणनम्_Matches
                          ? fromVtxp->datap()->state_गणनम्_Sigp->clone_वृक्षः_विशुद्धम्(असत्य)
                          : नूतन AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य),
                                          m_u32DTypep->विस्तारः()};
                Ast_ग्रन्थिः_व्यञ्जकम्* contribOverflowp
                    = गणनम्_Matches && fromVtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp
                          ? fromVtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp->clone_वृक्षः_विशुद्धम्(असत्य)
                          : शून्यसूचकः;
                यदि (tedger.m_condp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep = tedger.m_condp->clone_वृक्षः_विशुद्धम्(असत्य);
                    contribp = गणनम्_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : सङ्कलय_Countp(c.flp, शून्यसूचकः, contribp, enablep);
                }
                यदि (c.disableExprp) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                        = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                    contribp = गणनम्_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : सङ्कलय_Countp(c.flp, शून्यसूचकः, contribp, notDisp);
                }
                incomingp = गणनम्_Matches ? सङ्कलय_Checked_Countp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : सङ्कलय_Countp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            ध्रुव तार्किक storesCounts = vtxp->datap()->delayRingCounts;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव incomingCountp
                = storesCounts
                      ? incomingp->clone_वृक्षः_विशुद्धम्(असत्य)
                      : नूतन AstExtend{c.flp,
                                      नूतन AstNeq{c.flp, incomingp->clone_वृक्षः_विशुद्धम्(असत्य),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->विस्तारः()};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव storedIncomingp
                = storesCounts ? incomingp
                               : नूतन AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            Ast_नियोजय* ध्रुव writeIncomingp = नूतन Ast_नियोजय{
                c.flp,
                delayRingSlot(c.flp, vtxp, नूतन AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_ग्रन्थिः* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव outgoingp = delay_Ring_Output_गणनम्(c.flp, vtxp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव afterOutgoingp
                = नूतन AstSub{c.flp, नूतन AstVarRef{c.flp, जीवितम्_गणनम्_Varp, VAccess::READ}, outgoingp};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oldAfterOutgoingp = afterOutgoingp->clone_वृक्षः_विशुद्धम्(असत्य);
            Ast_सङ्कलय* ध्रुव अग्र_जीवितम्_Countp = नूतन Ast_सङ्कलय{c.flp, afterOutgoingp, incomingCountp};
            यदि (storesCounts) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव जीवितम्_Overflowp
                    = नूतन AstLt{c.flp, अग्र_जीवितम्_Countp->clone_वृक्षः_विशुद्धम्(असत्य), oldAfterOutgoingp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव overflowp = orExprs(c.flp, incomingOverflowp, जीवितम्_Overflowp);
                updateBodyp = new_गणनम्_अतिप्रवाहः_Warning(c.flp, overflowp);
                updateBodyp->सङ्कलय_अग्र(writeIncomingp);
            } अन्यथा {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_वृक्षः(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_ग्रन्थिः_व्यञ्जकम्* अग्र_शीर्ष_Idxp
                = अग्र_Ring_सूचकांक(c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::READ}, आकार);
            यदि (!vtxp->m_isFixedDelayRing) {
                अग्र_शीर्ष_Idxp = अग्र_Ring_सूचकांक(c.flp, अग्र_शीर्ष_Idxp, आकार);
            }
            updateBodyp->सङ्कलय_अग्र(नूतन Ast_नियोजय_Dly{
                c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->delay_Ring_शीर्ष_Varp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, अग्र_शीर्ष_Idxp)});
            updateBodyp->सङ्कलय_अग्र(नूतन Ast_नियोजय_Dly{
                c.flp, नूतन AstVarRef{c.flp, जीवितम्_गणनम्_Varp, VAccess::WRITE}, अग्र_जीवितम्_Countp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->सङ्कलय_अग्र(
                नूतन Ast_नियोजय_Dly{c.flp, नूतन AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 नूतन AstLogOr{c.flp, नूतन AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delay_Ring_At_चरमः_सूचकांक(c.flp, idxp, आकार)}});
            // idx <= next_idx;
            updateBodyp->सङ्कलय_अग्र(नूतन Ast_नियोजय_Dly{
                c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::WRITE},
                अग्र_Ring_सूचकांक(c.flp, नूतन AstVarRef{c.flp, idxp, VAccess::READ}, आकार)});
            यदि (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = नूतन AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_वृक्षः_विशुद्धम्(असत्य)),
                    updateBodyp};
            }

            Ast_ग्रन्थिः_व्यञ्जकम्* clearCondp = kill_सक्रिय(c);
            यदि (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_वृक्षः_विशुद्धम्(असत्य)));
            }
            यदि (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_वृक्षः_विशुद्धम्(असत्य)));
            }
            यदि (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य));
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* guardp = शून्यसूचकः;
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp : vtxp->m_throughoutConds) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledp = sampled(cp->clone_वृक्षः_विशुद्धम्(असत्य));
                guardp = guardp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            यदि (guardp) clearCondp = orExprs(c.flp, clearCondp, नूतन AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            Ast_नियोजय_Dly* ध्रुव clearCountp
                = नूतन Ast_नियोजय_Dly{c.flp, नूतन AstVarRef{c.flp, जीवितम्_गणनम्_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, जीवितम्_गणनम्_Varp->dtypep(), 0)};
            clearCountp->सङ्कलय_अग्र(नूतन Ast_नियोजय_Dly{c.flp,
                                                  नूतन AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  नूतन AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->सङ्कलय_अग्र(नूतन Ast_नियोजय_Dly{c.flp,
                                                  नूतन AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = नूतन AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            सङ्कलय_State_Always(c, updateBodyp, asyncDisable ? clearCountp->clone_वृक्षः(सत्य) : शून्यसूचकः);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    शून्य emit_And_Combiner_समाप्तम्_Latch_Nba(अधस्तनम्_Ctx& c) {
        ध्रुव तार्किक asyncDisable = isAsyncDisable(c.disableExprp);
        प्रति (ध्रुव V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ध्रुव SvaStateVertex* ध्रुव avp = static_cast<ध्रुव SvaStateVertex*>(&vtxr);
            यदि (!avp->datap()->समाप्तम्_LVarp) अनुवर्त;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            ध्रुव SvaStateVertex* ध्रुव lhsTermp = avp->m_andLhsTermp;
            ध्रुव SvaStateVertex* ध्रुव rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            Ast_नियोजय_Dly* ध्रुव clearLp = नूतन Ast_नियोजय_Dly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->समाप्तम्_LVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitFalse{}}};
            Ast_नियोजय_Dly* ध्रुव clearRp = नूतन Ast_नियोजय_Dly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->समाप्तम्_RVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->सङ्कलय_अग्र(clearRp);

            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मेलनम्_LNowp
                = build_मेलनम्_Now(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मेलनम्_RNowp
                = build_मेलनम्_Now(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_ग्रन्थिः_व्यञ्जकम्* gateLp = मेलनम्_LNowp;
            Ast_ग्रन्थिः_व्यञ्जकम्* gateRp = मेलनम्_RNowp;
            यदि (c.disableExprp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisLp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                gateLp = नूतन AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisRp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                gateRp = नूतन AstLogAnd{c.flp, gateRp, notDisRp};
            }
            Ast_नियोजय_Dly* ध्रुव समुच्चयः_Lp = नूतन Ast_नियोजय_Dly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->समाप्तम्_LVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ध्रुव समुच्चयः_LIfp = नूतन AstIf{c.flp, gateLp, समुच्चयः_Lp, शून्यसूचकः};
            Ast_नियोजय_Dly* ध्रुव समुच्चयः_Rp = नूतन Ast_नियोजय_Dly{
                c.flp, नूतन AstVarRef{c.flp, avp->datap()->समाप्तम्_RVarp, VAccess::WRITE},
                नूतन AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* ध्रुव समुच्चयः_RIfp = नूतन AstIf{c.flp, gateRp, समुच्चयः_Rp, शून्यसूचकः};
            समुच्चयः_LIfp->सङ्कलय_अग्र(समुच्चयः_RIfp);

            Ast_ग्रन्थिः_व्यञ्जकम्* clearCondp = नूतन AstLogOr{c.flp, kill_सक्रिय(c),
                                                   avp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य)};
            यदि (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य));
            }
            AstIf* ध्रुव topp = नूतन AstIf{c.flp, clearCondp, clearLp, समुच्चयः_LIfp};
            सङ्कलय_State_Always(c, topp, asyncDisable ? clearLp->clone_वृक्षः(सत्य) : शून्यसूचकः);
        }
    }

    शून्य emitKillAckNba(अधस्तनम्_Ctx& c) {
        Ast_नियोजय_Dly* ध्रुव ackp
            = नूतन Ast_नियोजय_Dly{c.flp, नूतन AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->सङ्कलय_Stmtsp(
            नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्षः(असत्य), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    शून्य compute_Terminal_मेलनम्_And_Reject(अधस्तनम्_Ctx& c, Signal_समुच्चयः& sigs, ध्रुव तार्किक need_मेलनम्_गणनम्) {
        प्रति (ध्रुव SvaTransEdge* ध्रुव tedgep : c.graph.all_धाराः()) {
            यदि (tedgep->गन्तव्यम्_Vtxp() != c.graph.m_मेलनम्_Vertexp) अनुवर्त;
            SvaStateVertex* ध्रुव fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_ग्रन्थिः_व्यञ्जकम्* srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            यदि (need_मेलनम्_गणनम्) {
                UASSERT_OBJ(fromVtxp->datap()->state_गणनम्_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_ग्रन्थिः_व्यञ्जकम्* contributionp
                    = fromVtxp->datap()->state_गणनम्_Sigp->clone_वृक्षः_विशुद्धम्(असत्य);
                Ast_ग्रन्थिः_व्यञ्जकम्* contributionOverflowp
                    = fromVtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp
                          ? fromVtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp->clone_वृक्षः_विशुद्धम्(असत्य)
                          : शून्यसूचकः;
                यदि (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_वृक्षः_विशुद्धम्(असत्य));
                }
                sigs.मेलनम्_Countp
                    = सङ्कलय_Checked_Countp(c.flp, sigs.मेलनम्_Countp, contributionp,
                                       sigs.मेलनम्_गणनम्_Overflowp, contributionOverflowp);
            }

            यदि (fromVtxp->m_delay_Ring_आकार && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_वृक्षः_विशुद्धम्(असत्य));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_ग्रन्थिः_व्यञ्जकम्* expireContribp = delay_Ring_Output_गणनम्(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_वृक्षः(), srcSigp);
            } अन्यथा यदि (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } अन्यथा {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_वृक्षः_विशुद्धम्(असत्य));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_मेलनम्_Vertexp,
                    "No terminal edge to match vertex");
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newThroughoutThreadFailCountp(अधस्तनम्_Ctx& c, AstVar* ध्रुव delay_Ring_जीवितम्_गणनम्_Varp,
                                               Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव stateExprp,
                                               Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सक्रिय_Thread_Countp
            = delay_Ring_जीवितम्_गणनम्_Varp
                  ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(
                        नूतन AstVarRef{c.flp, delay_Ring_जीवितम्_गणनम्_Varp, VAccess::READ})
                  : नूतन AstExtend{c.flp, stateExprp->clone_वृक्षः_विशुद्धम्(असत्य), m_u32DTypep->विस्तारः()};
        प्रत्यागम सङ्कलय_Countp(c.flp, शून्यसूचकः, सक्रिय_Thread_Countp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    शून्य computeThroughoutReject(अधस्तनम्_Ctx& c, Signal_समुच्चयः& sigs, ध्रुव तार्किक need_Thread_Fail_गणनम्) {
        प्रति (ध्रुव V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            ध्रुव SvaStateVertex* ध्रुव vtxp = static_cast<ध्रुव SvaStateVertex*>(&vtxr);
            ध्रुव स्वचालित& conds = vtxp->m_throughoutConds;
            यदि (conds.रिक्तानि() && !vtxp->m_abortRejectp) अनुवर्त;
            यदि (vtxp->m_isAndCombiner) अनुवर्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* stateExprp = शून्यसूचकः;
            यदि (vtxp->datap()->stateVarp) {
                stateExprp = नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } अन्यथा यदि (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delay_Ring_Has_जीवितम्_Threadsp(c.flp, vtxp->datap()->delay_Ring_जीवितम्_गणनम्_Varp);
            } अन्यथा {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य);
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* guardp = शून्यसूचकः;
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cp : conds) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sp = sampled(cp->clone_वृक्षः_विशुद्धम्(असत्य));
                guardp = guardp ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            यदि (vtxp->m_abortRejectp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notAbortp
                    = नूतन AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_वृक्षः_विशुद्धम्(असत्य))};
                guardp = guardp
                             ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* rejectCondp = नूतन AstLogNot{c.flp, guardp};
            यदि (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notAbortClearp
                    = नूतन AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_वृक्षः_विशुद्धम्(असत्य))};
                rejectCondp = नूतन AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                यदि (vtxp->m_abortRejectp) {
                    rejectCondp = नूतन AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_वृक्षः_विशुद्धम्(असत्य))};
                }
            }
            यदि (need_Thread_Fail_गणनम्) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_जीवितम्_गणनम्_Varp,
                                                    stateExprp, rejectCondp->clone_वृक्षः_विशुद्धम्(असत्य));
                sigs.threadFailCountp = सङ्कलय_Countp(c.flp, sigs.threadFailCountp, contributionp);
                यदि (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव abortAndAdvancep = नूतन AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_वृक्षः_विशुद्धम्(असत्य)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_वृक्षः_विशुद्धम्(असत्य))};
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मेलनम्_Contributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_जीवितम्_गणनम्_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = सङ्कलय_Countp(c.flp, sigs.threadFailCountp, मेलनम्_Contributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             नूतन AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    Signal_समुच्चयः compute_सङ्केताः(अधस्तनम्_Ctx& c, ध्रुव तार्किक need_Thread_Fail_गणनम्,
                             ध्रुव तार्किक need_Throughout_Thread_Fail_गणनम्, ध्रुव तार्किक need_मेलनम्_गणनम्) {
        Signal_समुच्चयः sigs;

        compute_Terminal_मेलनम्_And_Reject(c, sigs, need_मेलनम्_गणनम्);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        प्रति (ध्रुव SvaTransEdge* ध्रुव tedgep : c.graph.all_धाराः()) {
            यदि (!tedgep->m_rejectOnFail) अनुवर्त;
            ध्रुव SvaStateVertex* ध्रुव fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव srcSigp = fromVtxp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः;
            यदि (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य);
                यदि (tedgep->m_condp) {
                    condp = नूतन AstLogOr{c.flp, condp, tedgep->m_condp->clone_वृक्षः_विशुद्धम्(असत्य)};
                }
            } अन्यथा {
                condp = tedgep->m_condp->clone_वृक्षः_विशुद्धम्(असत्य);
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notCondp = नूतन AstLogNot{c.flp, condp};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rawFailp = नूतन AstLogAnd{c.flp, srcSigp, notCondp};
            यदि (need_Thread_Fail_गणनम्) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = सङ्कलय_Countp(
                    c.flp, sigs.threadFailCountp,
                    नूतन AstExtend{c.flp, rawFailp->clone_वृक्षः_विशुद्धम्(असत्य), m_u32DTypep->विस्तारः()});
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव failp = gateNotKill(c, rawFailp);
            sigs.required_चरणः_Rejectp = orExprs(c.flp, sigs.required_चरणः_Rejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_गणनम्);
        यदि (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = सङ्कलय_Countp(c.flp, शून्यसूचकः, sigs.threadFailCountp, not_Kill_सक्रिय(c));
        }
        यदि (sigs.मेलनम्_Countp) {
            यदि (c.मेलनम्_Condp) {
                sigs.मेलनम्_Countp
                    = gateCheckedCountp(c.flp, sigs.मेलनम्_Countp, sigs.मेलनम्_गणनम्_Overflowp,
                                        sampled(c.मेलनम्_Condp->clone_वृक्षः_विशुद्धम्(असत्य)));
            }
            sigs.मेलनम्_Countp = gateCheckedCountp(c.flp, sigs.मेलनम्_Countp, sigs.मेलनम्_गणनम्_Overflowp,
                                                 not_Kill_सक्रिय(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        प्रति (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_ग्रन्थिः_व्यञ्जकम्*& sigp = vtxp->datap()->stateSigp;
            यदि (sigp) VL_DO_DANGLING(sigp->delete_वृक्षः(), sigp);
            Ast_ग्रन्थिः_व्यञ्जकम्*& countp = vtxp->datap()->state_गणनम्_Sigp;
            यदि (countp) VL_DO_DANGLING(countp->delete_वृक्षः(), countp);
            Ast_ग्रन्थिः_व्यञ्जकम्*& overflowp = vtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp;
            यदि (overflowp) VL_DO_DANGLING(overflowp->delete_वृक्षः(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        यदि (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notTermp
                = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
            sigs.terminalActivep = नूतन AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            यदि (sigs.rejectBasep) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                sigs.rejectBasep = नूतन AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            यदि (sigs.throughoutRejectp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                sigs.throughoutRejectp = नूतन AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            यदि (sigs.required_चरणः_Rejectp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notDisp
                    = नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)};
                sigs.required_चरणः_Rejectp = नूतन AstLogAnd{c.flp, sigs.required_चरणः_Rejectp, notDisp};
            }
            यदि (sigs.मेलनम्_Countp) {
                sigs.मेलनम्_Countp = gateCheckedCountp(
                    c.flp, sigs.मेलनम्_Countp, sigs.मेलनम्_गणनम्_Overflowp,
                    नूतन AstLogNot{c.flp, c.disableExprp->clone_वृक्षः_विशुद्धम्(असत्य)});
            }
        }

        यदि (sigs.मेलनम्_गणनम्_Overflowp) {
            m_modp->सङ्कलय_Stmtsp(
                नूतन AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_वृक्षः(असत्य),
                              new_गणनम्_अतिप्रवाहः_Warning(c.flp, sigs.मेलनम्_गणनम्_Overflowp)});
            sigs.मेलनम्_गणनम्_Overflowp = शून्यसूचकः;
        }

        प्रत्यागम sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    शून्य resolveLinks(अधस्तनम्_Ctx& c, Ast_ग्रन्थिः_व्यञ्जकम्* triggerExprp, ध्रुव तार्किक गणनम्_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_वृक्षः_विशुद्धम्(असत्य);
        c.graph.m_startVertexp->datap()->state_गणनम्_Sigp
            = नूतन AstExtend{c.flp, triggerExprp->clone_वृक्षः_विशुद्धम्(असत्य), m_u32DTypep->विस्तारः()};
        प्रति (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
            यदि (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_गणनम्_Sigp = नूतन AstExtend{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->विस्तारः()};
            } अन्यथा यदि (vtxp->datap()->delayRingVarp) {
                यदि (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव countp = delay_Ring_Output_गणनम्(c.flp, vtxp);
                    vtxp->datap()->stateSigp = नूतन AstNeq{c.flp, countp->clone_वृक्षः_विशुद्धम्(असत्य),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_गणनम्_Sigp = countp;
                } अन्यथा {
                    vtxp->datap()->stateSigp
                        = delay_Ring_Has_जीवितम्_Threadsp(c.flp, vtxp->datap()->delay_Ring_जीवितम्_गणनम्_Varp);
                    vtxp->datap()->state_गणनम्_Sigp = नूतन AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_जीवितम्_गणनम्_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        ध्रुव आकार_t passes = 2 * c.graph.m_graph.vertices().आकार() + 2;
        प्रति (आकार_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            प्रति (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
                यदि (!vtxp->m_isAndCombiner) अनुवर्त;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                ध्रुव SvaStateVertex* ध्रुव lhsTermp = vtxp->m_andLhsTermp;
                ध्रुव SvaStateVertex* ध्रुव rhsTermp = vtxp->m_andRhsTermp;
                यदि (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) अनुवर्त;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मेलनम्_Lp
                    = build_मेलनम्_Now(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मेलनम्_Rp
                    = build_मेलनम्_Now(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव समाप्तम्_LOrp = नूतन AstLogOr{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->समाप्तम्_LVarp, VAccess::READ}, मेलनम्_Lp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव समाप्तम्_ROrp = नूतन AstLogOr{
                    c.flp, नूतन AstVarRef{c.flp, vtxp->datap()->समाप्तम्_RVarp, VAccess::READ}, मेलनम्_Rp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bothp = नूतन AstLogAnd{c.flp, समाप्तम्_LOrp, समाप्तम्_ROrp};
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव oneNowp = नूतन AstLogOr{c.flp, मेलनम्_Lp->clone_वृक्षः_विशुद्धम्(असत्य),
                                                          मेलनम्_Rp->clone_वृक्षः_विशुद्धम्(असत्य)};
                यदि (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_वृक्षः(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = नूतन AstLogAnd{c.flp, bothp, oneNowp};
                यदि (vtxp->datap()->state_गणनम्_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गणनम्_Sigp->delete_वृक्षः(),
                                   vtxp->datap()->state_गणनम्_Sigp);
                }
                vtxp->datap()->state_गणनम्_Sigp = नूतन AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य), m_u32DTypep->विस्तारः()};
            }

            प्रति (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* ध्रुव vtxp = static_cast<SvaStateVertex*>(&vtxr);
                यदि (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_is_मेलनम्
                    || vtxp->m_isRejectSink) {
                    अनुवर्त;
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* अग्र_Statep = शून्यसूचकः;
                Ast_ग्रन्थिः_व्यञ्जकम्* अग्र_State_Countp = शून्यसूचकः;
                Ast_ग्रन्थिः_व्यञ्जकम्* अग्र_State_गणनम्_Overflowp = शून्यसूचकः;
                प्रति (ध्रुव V3GraphEdge& er : vtxp->अन्तर्_धाराः()) {
                    ध्रुव SvaTransEdge& te = static_cast<ध्रुव SvaTransEdge&>(er);
                    ध्रुव SvaStateVertex* ध्रुव fromVtxp = te.fromVtxp();
                    यदि (!fromVtxp->datap()->stateSigp) अनुवर्त;
                    UASSERT_OBJ(fromVtxp->datap()->state_गणनम्_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_वृक्षः_विशुद्धम्(असत्य), te.m_condp);
                    अग्र_Statep = orExprs(c.flp, अग्र_Statep, contributionp);
                    Ast_ग्रन्थिः_व्यञ्जकम्* गणनम्_Contributionp
                        = fromVtxp->datap()->state_गणनम्_Sigp->clone_वृक्षः_विशुद्धम्(असत्य);
                    Ast_ग्रन्थिः_व्यञ्जकम्* गणनम्_Contribution_Overflowp
                        = गणनम्_Matches && fromVtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp
                              ? fromVtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp->clone_वृक्षः_विशुद्धम्(असत्य)
                              : शून्यसूचकः;
                    यदि (te.m_condp) {
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव enablep = te.m_condp->clone_वृक्षः_विशुद्धम्(असत्य);
                        गणनम्_Contributionp
                            = गणनम्_Matches
                                  ? gateCheckedCountp(c.flp, गणनम्_Contributionp,
                                                      गणनम्_Contribution_Overflowp, enablep)
                                  : सङ्कलय_Countp(c.flp, शून्यसूचकः, गणनम्_Contributionp, enablep);
                    }
                    अग्र_State_Countp
                        = गणनम्_Matches
                              ? सङ्कलय_Checked_Countp(c.flp, अग्र_State_Countp, गणनम्_Contributionp,
                                                 अग्र_State_गणनम्_Overflowp,
                                                 गणनम्_Contribution_Overflowp)
                              : सङ्कलय_Countp(c.flp, अग्र_State_Countp, गणनम्_Contributionp);
                }
                यदि (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_वृक्षः(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = अग्र_Statep;
                यदि (vtxp->datap()->state_गणनम्_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गणनम्_Sigp->delete_वृक्षः(),
                                   vtxp->datap()->state_गणनम्_Sigp);
                }
                vtxp->datap()->state_गणनम्_Sigp = अग्र_State_Countp;
                यदि (vtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp->delete_वृक्षः(),
                                   vtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp);
                }
                vtxp->datap()->state_गणनम्_अतिप्रवाहः_Sigp = अग्र_State_गणनम्_Overflowp;
            }
        }
    }

सार्वजनिक:
    // Combine terminal/reject signals into final output expression.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* assemble_परिणाम(Ast_ग्रन्थिः_Cover_Or_Assert* ध्रुव assertp, ध्रुव तार्किक negated,
                                       Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मेलनम्_Condp, ध्रुव Signal_समुच्चयः& sigs,
                                       Ast_ग्रन्थिः_व्यञ्जकम्** ध्रुव बहिर्गमनम्_Matchpp) {
        File_पङ्क्तिः* ध्रुव flp = assertp->fileline();
        ध्रुव तार्किक isCover = VN_IS(assertp, Cover);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव terminalActivep = sigs.terminalActivep;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rejectBasep = sigs.rejectBasep;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव throughoutRejectp = sigs.throughoutRejectp;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव required_चरणः_Rejectp = sigs.required_चरणः_Rejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        यदि (negated) {
            यदि (isCover) {
                यदि (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_वृक्षः(), terminalActivep);
                Ast_ग्रन्थिः_व्यञ्जकम्* negRejectp = शून्यसूचकः;
                यदि (मेलनम्_Condp && rejectBasep) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(मेलनम्_Condp->clone_वृक्षः_विशुद्धम्(असत्य));
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव notCondp = नूतन AstLogNot{flp, sampledCondp};
                    negRejectp = नूतन AstLogAnd{flp, rejectBasep, notCondp};
                } अन्यथा यदि (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_वृक्षः(), rejectBasep);
                }
                यदि (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                यदि (required_चरणः_Rejectp)
                    negRejectp = orExprs(flp, negRejectp, required_चरणः_Rejectp);
                प्रत्यागम negRejectp ? negRejectp : नूतन AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_ग्रन्थिः_व्यञ्जकम्* matchp = terminalActivep;
            यदि (मेलनम्_Condp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(मेलनम्_Condp->clone_वृक्षः_विशुद्धम्(असत्य));
                matchp = नूतन AstLogAnd{flp, matchp, sampledCondp};
            }
            यदि (बहिर्गमनम्_Matchpp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* notPMatchp = शून्यसूचकः;
                यदि (मेलनम्_Condp && rejectBasep) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(मेलनम्_Condp->clone_वृक्षः_विशुद्धम्(असत्य));
                    notPMatchp = नूतन AstLogAnd{flp, rejectBasep->clone_वृक्षः_विशुद्धम्(असत्य),
                                               नूतन AstLogNot{flp, sampledCondp}};
                } अन्यथा यदि (rejectBasep) {
                    notPMatchp = rejectBasep->clone_वृक्षः_विशुद्धम्(असत्य);
                }
                यदि (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_वृक्षः_विशुद्धम्(असत्य));
                यदि (required_चरणः_Rejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, required_चरणः_Rejectp->clone_वृक्षः_विशुद्धम्(असत्य));
                *बहिर्गमनम्_Matchpp = notPMatchp;
            }
            यदि (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_वृक्षः(), throughoutRejectp);
            यदि (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_वृक्षः(), rejectBasep);
            यदि (required_चरणः_Rejectp)
                VL_DO_DANGLING(required_चरणः_Rejectp->delete_वृक्षः(), required_चरणः_Rejectp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव परिणाम_Exprp = नूतन AstLogNot{flp, matchp};
            प्रत्यागम परिणाम_Exprp;
        }
        यदि (isCover) {
            यदि (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_वृक्षः(), throughoutRejectp);
            यदि (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_वृक्षः(), rejectBasep);
            यदि (required_चरणः_Rejectp)
                VL_DO_DANGLING(required_चरणः_Rejectp->delete_वृक्षः(), required_चरणः_Rejectp);
            यदि (मेलनम्_Condp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(मेलनम्_Condp->clone_वृक्षः_विशुद्धम्(असत्य));
                प्रत्यागम नूतन AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            प्रत्यागम terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_ग्रन्थिः_व्यञ्जकम्* rejectp = शून्यसूचकः;
        यदि (मेलनम्_Condp && rejectBasep) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sampledCondp = sampled(मेलनम्_Condp->clone_वृक्षः_विशुद्धम्(असत्य));
            rejectp = नूतन AstLogAnd{flp, rejectBasep, नूतन AstLogNot{flp, sampledCondp}};
        } अन्यथा यदि (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_वृक्षः(), rejectBasep);
        }
        यदि (बहिर्गमनम्_Matchpp) {
            Ast_ग्रन्थिः_व्यञ्जकम्* मेलनम्_Exprp = terminalActivep->clone_वृक्षः_विशुद्धम्(असत्य);
            यदि (मेलनम्_Condp) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sp = sampled(मेलनम्_Condp->clone_वृक्षः_विशुद्धम्(असत्य));
                मेलनम्_Exprp = नूतन AstLogAnd{flp, मेलनम्_Exprp, sp};
            }
            *बहिर्गमनम्_Matchpp = मेलनम्_Exprp;
        }
        यदि (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_वृक्षः(), terminalActivep);
        यदि (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        यदि (required_चरणः_Rejectp) rejectp = orExprs(flp, rejectp, required_चरणः_Rejectp);
        यदि (!rejectp) प्रत्यागम नूतन AstConst{flp, AstConst::BitTrue{}};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव परिणाम_Exprp = नूतन AstLogNot{flp, rejectp};
        प्रत्यागम परिणाम_Exprp;
    }

    स्पष्ट SvaNfaLowering(Ast_ग्रन्थिः_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::सङ्कलय_Stat_योग("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    Signal_समुच्चयः अधस्तनम्(Ast_ग्रन्थिः_Cover_Or_Assert* ध्रुव assertp, SvaGraph& graph,
                    Ast_Sen_वृक्षः* ध्रुव senTreep, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव मेलनम्_Condp,
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disableExprp, ध्रुव तार्किक need_Thread_Fail_गणनम्,
                    ध्रुव तार्किक need_Throughout_Thread_Fail_गणनम्) {
        File_पङ्क्तिः* ध्रुव flp = assertp->fileline();
        AstCover* ध्रुव coverp = VN_CAST(assertp, Cover);
        ध्रुव तार्किक isSeqEvent = coverp && coverp->isSeqEvent();
        ध्रुव VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::आन्तरिकम्} : assertp->userType();
        ध्रुव VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::आन्तरिकम्}
                         : assertp->directive();
        ध्रुव std::string आधारः_Name = m_नामानि.get("");

        // Number vertices with sequential colors for array indexing.
        पूर्णाङ्क N = 0;
        प्रति (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::सदिशम्<SvaStateVertex*> vtx(N, शून्यसूचकः);
        प्रति (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::सदिशम्<Sva_Vertex_दत्त> vertex_दत्त(N);
        प्रति (पूर्णाङ्क i = 0; i < N; ++i) vtx[i]->userp(&vertex_दत्त[i]);

        // Identify registered vertices (targets of clocked edges).
        प्रति (पूर्णाङ्क i = 0; i < N; ++i) {
            प्रति (ध्रुव V3GraphEdge& edger : vtx[i]->बहिर्गमनम्_धाराः()) {
                ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
                ध्रुव पूर्णाङ्क गन्तव्यम्_Idx = tedger.गन्तव्यम्_Vtxp()->color();
                यदि (tedger.m_consumes_आवृत्तिः && tedger.गन्तव्यम्_Vtxp() != graph.m_मेलनम्_Vertexp
                    && !tedger.गन्तव्यम्_Vtxp()->m_isRejectSink) {
                    vtx[गन्तव्यम्_Idx]->datap()->needsReg = सत्य;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::सदिशम्<तार्किक> stateMayHaveMultiplicity(N);
        std::सदिशम्<तार्किक> ringNeedsMultiplicity(N);
        std::सदिशम्<पूर्णाङ्क> multiplicityWork;
        ध्रुव तार्किक गणनम्_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        यदि (गणनम्_Matches) {
            प्रति (पूर्णाङ्क i = 0; i < N; ++i) {
                ध्रुव तार्किक गुणजः_Inputs = vtx[i]->अन्तर्_धाराः().आकार() > 1;
                ध्रुव तार्किक isRing = vtx[i]->m_delay_Ring_आकार;
                ringNeedsMultiplicity[i] = isRing && गुणजः_Inputs;
                ध्रुव तार्किक startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : गुणजः_Inputs && !vtx[i]->datap()->needsReg;
                यदि (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = सत्य;
                    multiplicityWork.प्रक्षिप_पृष्ठभागः(i);
                }
            }
            प्रति (आकार_t wi = 0; wi < multiplicityWork.आकार(); ++wi) {
                प्रति (ध्रुव V3GraphEdge& edger : vtx[multiplicityWork[wi]]->बहिर्गमनम्_धाराः()) {
                    ध्रुव SvaTransEdge& tedger = static_cast<ध्रुव SvaTransEdge&>(edger);
                    ध्रुव पूर्णाङ्क ti = tedger.गन्तव्यम्_Vtxp()->color();
                    ध्रुव तार्किक isRing = vtx[ti]->m_delay_Ring_आकार;
                    यदि (isRing) ringNeedsMultiplicity[ti] = सत्य;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    ध्रुव तार्किक stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    यदि (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { अनुवर्त; }
                    stateMayHaveMultiplicity[ti] = सत्य;
                    multiplicityWork.प्रक्षिप_पृष्ठभागः(ti);
                }
            }
        }

        ध्रुव तार्किक asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_पङ्क्तिः* ध्रुव regFlp = asyncDisable ? नूतन File_पङ्क्तिः{flp} : flp;
        यदि (asyncDisable) regFlp->warnOff(V3_Error_संहिता::MULTIDRIVEN, सत्य);

        AstVar* ध्रुव killVarp
            = नूतन AstVar{flp, VVarType::MODULETEMP, आधारः_Name + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->सङ्कलय_Stmtsp(killVarp);
        प्रति (पूर्णाङ्क i = 0; i < N; ++i) {
            यदि (vtx[i]->m_isAndCombiner) {
                ध्रुव std::string आधारः = आधारः_Name + "__a" + std::गन्तव्यम्_string(i);
                AstVar* ध्रुव lp = नूतन AstVar{regFlp, VVarType::MODULETEMP, आधारः + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(lp);
                vtx[i]->datap()->समाप्तम्_LVarp = lp;
                AstVar* ध्रुव rp = नूतन AstVar{regFlp, VVarType::MODULETEMP, आधारः + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(rp);
                vtx[i]->datap()->समाप्तम्_RVarp = rp;
                अनुवर्त;
            }
            यदि (vtx[i]->m_delay_Ring_आकार) {
                ध्रुव std::string आधारः = आधारः_Name + "__d" + std::गन्तव्यम्_string(i);
                ध्रुव तार्किक storesCounts = गणनम्_Matches && ringNeedsMultiplicity[i];
                Ast_ग्रन्थिः_DType* ringDTypep = शून्यसूचकः;
                यदि (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = नूतन AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        नूतन AstRange{flp, static_cast<पूर्णाङ्क>(vtx[i]->m_delay_Ring_आकार - 1), 0}};
                    v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(ringDTypep);
                } अन्यथा {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delay_Ring_आकार,
                                                      vtx[i]->m_delay_Ring_आकार, VSigning::UNSIGNED);
                }
                AstVar* ध्रुव ringp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, आधारः + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* ध्रुव headp
                    = नूतन AstVar{flp, VVarType::MODULETEMP, आधारः + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(headp);
                vtx[i]->datap()->delay_Ring_शीर्ष_Varp = headp;
                // int unsigned idx;
                AstVar* ध्रुव idxp
                    = नूतन AstVar{regFlp, VVarType::MODULETEMP, आधारः + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* ध्रुव जीवितम्_गणनम्_Varp
                    = नूतन AstVar{regFlp, VVarType::MODULETEMP, आधारः + "_liveCount", m_u32DTypep};
                जीवितम्_गणनम्_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(जीवितम्_गणनम्_Varp);
                vtx[i]->datap()->delay_Ring_जीवितम्_गणनम्_Varp = जीवितम्_गणनम्_Varp;
                AstVar* ध्रुव wrappedp = नूतन AstVar{regFlp, VVarType::MODULETEMP,
                                                    आधारः + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->सङ्कलय_Stmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                अनुवर्त;
            }
            यदि (!vtx[i]->datap()->needsReg) अनुवर्त;
            यदि (vtx[i] == graph.m_startVertexp || vtx[i]->m_is_मेलनम्) अनुवर्त;
            ध्रुव std::string varName = आधारः_Name + "__s" + std::गन्तव्यम्_string(i);
            AstVar* ध्रुव varp
                = नूतन AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->सङ्कलय_Stmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        अधस्तनम्_Ctx c{flp,        senTreep,      disableExprp, मेलनम्_Condp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव triggerExprp
            = isSeqEvent ? नूतन AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, गणनम्_Matches);
        VL_DO_DANGLING(triggerExprp->delete_वृक्षः(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emit_State_पञ्जिकाकोशः_Nba(c);
        emitDelayRingUpdates(c, गणनम्_Matches);
        emit_And_Combiner_समाप्तम्_Latch_Nba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        ध्रुव Signal_समुच्चयः sigs
            = compute_सङ्केताः(c, need_Thread_Fail_गणनम्, need_Throughout_Thread_Fail_गणनम्, गणनम्_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_ग्रन्थिः_व्यञ्जकम्* pendingp = शून्यसूचकः;
        प्रति (पूर्णाङ्क i = 0; i < N; ++i) {
            यदि (!vtx[i]->m_strongPending) अनुवर्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* pendingExprp = शून्यसूचकः;
            यदि (vtx[i]->datap()->stateVarp) {
                pendingExprp = नूतन AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } अन्यथा यदि (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delay_Ring_Has_जीवितम्_Threadsp(flp, vtx[i]->datap()->delay_Ring_जीवितम्_गणनम्_Varp);
            } अन्यथा {
                अनुवर्त;
            }
            यदि (!pendingp) {
                pendingp = pendingExprp;
            } अन्यथा {
                pendingp = नूतन AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        यदि (pendingp) {
            AstCExpr* ध्रुव assertOnp
                = नूतन AstCExpr{flp, AstCExpr::विशुद्धम्{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* ध्रुव dispp
                = नूतन AstDisplay{flp, VDisplayType::DT_ERROR, "", शून्यसूचकः, शून्यसूचकः};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_ग्रन्थिः_Stmt* ध्रुव firep = dispp;
            यदि (v3Global.opt.stopFail()) firep->सङ्कलय_अग्र(नूतन AstStop{flp, असत्य});
            m_modp->सङ्कलय_Stmtsp(नूतन AstFinal{flp, नूतन AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        प्रति (पूर्णाङ्क i = 0; i < N; ++i) vtx[i]->userp(शून्यसूचकः);
        प्रत्यागम sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

वर्ग AssertNfaVisitor final : सार्वजनिक VNVisitor {
    // STATE
    Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module being processed
    AstClocking* m_defaultClockingp = शून्यसूचकः;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = शून्यसूचकः;  // Default disable iff
    SvaNfaLowering* m_loweringp = शून्यसूचकः;  // NFA-to-hardware lowering engine
    Ast_Sen_वृक्षः* m_sampled_मानम्_Clockp = शून्यसूचकः;  // Inherited clock during scoped attachment
    V3_Unique_नामानि m_prop_Var_नामानि{"__Vpropvar"};  // Property-local variable names
    V3_Unique_नामानि m_prop_अस्थायी_नामानि{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3_Unique_नामानि m_fail_गणनम्_नामानि{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::समुच्चयः<ध्रुव AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    प्रतिरूपम् <प्रकारनाम T_ग्रन्थिः>
    शून्य दर्शय_Sampled_मानम्(T_ग्रन्थिः* ध्रुव nodep) {
        यदि (m_sampled_मानम्_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_मानम्_Clockp->clone_वृक्षः(सत्य));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    स्थिर शून्य wire_मेलनम्_And_Mid_Sources(SvaGraph& graph, ध्रुव Build_परिणाम& परिणाम, File_पङ्क्तिः* flp) {
        graph.create_मेलनम्_Vertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        यदि (!परिणाम.पदांशः_Is_Mid_Merge) { graph.सङ्कलय_Link(परिणाम.पदांशः_Vertexp, graph.m_मेलनम्_Vertexp); }
        प्रति (SvaStateVertex* srcVtxp : परिणाम.midSources) {
            Ast_ग्रन्थिः_व्यञ्जकम्* condp = शून्यसूचकः;
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tc : srcVtxp->m_throughoutConds) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tcClone = tc->clone_वृक्षः_विशुद्धम्(असत्य);
                condp = condp ? नूतन AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.सङ्कलय_Link(srcVtxp, graph.m_मेलनम्_Vertexp, condp);
            srcVtxp->m_isUnbounded = सत्य;
        }
    }

    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* getSequenceBodyExprp(ध्रुव AstSequence* seqp) {
        Ast_ग्रन्थिः* bodyp = seqp->stmtsp();
        यावत् (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        प्रत्यागम VN_CAST(bodyp, ग्रन्थिः_व्यञ्जकम्);
    }

    स्थिर AstPropSpec* getPropertySpecp(ध्रुव AstProperty* propp) {
        Ast_ग्रन्थिः* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        यावत् (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        प्रत्यागम VN_CAST(stmtp, PropSpec);
    }

    शून्य inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             ध्रुव AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        यदि (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            प्रत्यागम;  // LCOV_EXCL_LINE
        }
        संरचना Guard final {
            std::समुच्चयः<ध्रुव AstProperty*>& setr;
            ध्रुव AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_वृक्षः(असत्य);

        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> पत्तनम्_Map;
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            पत्तनम्_Map[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<ध्रुव AstVar*, AstVar*> localVarMap;
        प्रति (Ast_ग्रन्थिः* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                यदि (!varp->isIO()) {
                    ध्रुव string newName = m_prop_Var_नामानि.get(varp);
                    AstVar* ध्रुव newVarp = नूतन AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->सङ्कलय_Stmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            ध्रुव स्वचालित पत्तनम्_It = पत्तनम्_Map.find(refp->varp());
            यदि (पत्तनम्_It != पत्तनम्_Map.अन्तः_2()) {
                refp->replaceWith(पत्तनम्_It->second->clone_वृक्षः(असत्य));
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
                प्रत्यागम;
            }
            ध्रुव स्वचालित localIt = localVarMap.find(refp->varp());
            यदि (localIt != localVarMap.अन्तः_2()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            प्रक्षिप_Deletep(tconnect.second->exprp()->unlink_Fr_पृष्ठभागः());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        यदि (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            प्रक्षिप_Deletep(propSpecp->disablep()->unlink_Fr_पृष्ठभागः());
        }
        यदि (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlink_Fr_पृष्ठभागः());
        }

        यदि (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            प्रक्षिप_Deletep(propSpecp->sensesp()->unlink_Fr_पृष्ठभागः());
        }
        यदि (outerSpecp->sensesp()) {
            AstSenItem* ध्रुव sensesp = outerSpecp->sensesp();
            sensesp->unlink_Fr_पृष्ठभागः();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(outerSpecp), outerSpecp);
    }

    शून्य inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonedp = bodyExprp->clone_वृक्षः(असत्य);

        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> पत्तनम्_Map;
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            पत्तनम्_Map[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            ध्रुव स्वचालित it = पत्तनम्_Map.find(refp->varp());
            यदि (it != पत्तनम्_Map.अन्तः_2()) {
                refp->replaceWith(it->second->clone_वृक्षः(असत्य));
                VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
            }
        });
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            प्रक्षिप_Deletep(tconnect.second->exprp()->unlink_Fr_पृष्ठभागः());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(असत्य);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    शून्य inlineAllSequenceRefs(Ast_ग्रन्थिः* rootp) {
        तार्किक परिवर्तितम् = सत्य;
        यावत् (परिवर्तितम्) {
            परिवर्तितम् = असत्य;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                यदि (परिवर्तितम्) प्रत्यागम;
                यदि (AstSequence* ध्रुव seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    परिवर्तितम् = सत्य;
                }
            });
        }
    }

    स्थिर तार्किक has_Multi_आवृत्तिः_व्यञ्जकम्(ध्रुव Ast_ग्रन्थिः* nodep) {
        प्रत्यागम nodep->exists([](ध्रुव Ast_ग्रन्थिः* np) {
            यदि (ध्रुव स्वचालित* ध्रुव ep = VN_CAST(np, ग्रन्थिः_व्यञ्जकम्)) प्रत्यागम ep->is_Multi_आवृत्तिः_Sva();
            प्रत्यागम असत्य;
        });
    }

    स्थिर VPropStrength effectiveAssertPropStrength(ध्रुव AstPropSpec* ध्रुव propSpecp) {
        यदि (propSpecp->propStrength() != VPropStrength::DEFAULT) प्रत्यागम propSpecp->propStrength();
        प्रत्यागम propSpecp->fileline()->language() <= V3_Lang_संहिता::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    स्थिर तार्किक is_Bare_शिखरम्_Level_Until(Ast_ग्रन्थिः* propp) {
        Ast_ग्रन्थिः* p = propp;
        यदि (AstPropSpec* ध्रुव specp = VN_CAST(p, PropSpec)) p = specp->propp();
        यावत् (AstLogNot* ध्रुव notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* ध्रुव untilp = VN_CAST(p, Until);
        यदि (!untilp) प्रत्यागम असत्य;
        ध्रुव स्वचालित hasSeq = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ep) {
            प्रत्यागम ep->exists([](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* np) { प्रत्यागम np->is_Multi_आवृत्तिः_Sva(); });
        };
        यदि (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }

    संरचना PropertyParts final {
        Ast_ग्रन्थिः_व्यञ्जकम्* triggerExprp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* seqExprp = शून्यसूचकः;
        तार्किक isOverlapped = सत्य;
        तार्किक hasImplication = असत्य;
        तार्किक isFollowedBy = असत्य;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    स्थिर PropertyParts decomposeProperty(Ast_ग्रन्थिः* propp) {
        PropertyParts parts;
        यदि (AstPropSpec* ध्रुव specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        यदि (AstImplication* ध्रुव implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = सत्य;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } अन्यथा यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(propp, ग्रन्थिः_व्यञ्जकम्)) {
            parts.triggerExprp = शून्यसूचकः;
            parts.seqExprp = exprp;
        }
        प्रत्यागम parts;
    }

    स्थिर तार्किक can_विभज_Implication_Pass_Actions(ध्रुव PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        प्रत्यागम !has_Multi_आवृत्तिः_व्यञ्जकम्(parts.triggerExprp);
    }

    स्थिर शून्य विभज_Implication_Pass_Actions(AstAssert* assertp, ध्रुव PropertyParts& parts,
                                            Ast_ग्रन्थिः_व्यञ्जकम्* nonvacuousMatchp) {
        File_पङ्क्तिः* ध्रुव flp = assertp->fileline();

        Ast_ग्रन्थिः* ध्रुव passsp = assertp->passsp()->unlink_Fr_पृष्ठभागः_With_अग्र();
        Ast_ग्रन्थिः* splitsp = शून्यसूचकः;

        यदि (!parts.isFollowedBy) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव vacuousp
                = नूतन AstLogNot{flp, sampled(parts.triggerExprp->clone_वृक्षः_विशुद्धम्(असत्य))};
            Ast_ग्रन्थिः* ध्रुव vacuousBodyp = passsp->clone_वृक्षः(असत्य);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/सत्य);
        }

        AstIf* ध्रुव nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/असत्य);
        splitsp = splitsp ? Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(splitsp, nonvacuousp)
                          : static_cast<Ast_ग्रन्थिः*>(nonvacuousp);
        assertp->सङ्कलय_Passsp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    शून्य replaceBodyOnBuildError(File_पङ्क्तिः* flp, AstPropSpec* propSpecp, तार्किक errorEmitted) {
        यदि (!errorEmitted) प्रत्यागम;
        Ast_ग्रन्थिः* ध्रुव innerPropp = propSpecp->propp();
        innerPropp->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    तार्किक hoistClockedSeq(AstPropSpec* specp) {
        यावत् (AstSClocked* ध्रुव clockedp = VN_CAST(specp->propp(), SClocked)) {
            यदि (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
                प्रत्यागम सत्य;
            }
            प्रति (ध्रुव AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                यदि (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
                    प्रत्यागम सत्य;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bodyp = clockedp->exprp()->unlink_Fr_पृष्ठभागः();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        ध्रुव AstSClocked* nestedp = शून्यसूचकः;
        specp->propp()->foreach([&](ध्रुव AstSClocked* p) {
            यदि (!nestedp) nestedp = p;
        });
        यदि (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, सत्य);
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    Build_परिणाम buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_ग्रन्थिः_व्यञ्जकम्* seqBodyp,
                                    ध्रुव PropertyParts& parts, File_पङ्क्तिः* flp) {
        यदि (!parts.hasImplication) प्रत्यागम builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        प्रत्यागम builder.build_Implication_धाराः(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    शून्य attachActionHandlers(AstAssert* ध्रुव assertp, Ast_ग्रन्थिः_व्यञ्जकम्* मेलनम्_Exprp,
                              Ast_Sen_वृक्षः* ध्रुव threadFailReplaySenTreep,
                              Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        यदि (मेलनम्_Exprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_ग्रन्थिः* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            File_पङ्क्तिः* ध्रुव flp = assertp->fileline();
            AstIf* ध्रुव ifp = नूतन AstIf{flp, मेलनम्_Exprp, passsp, शून्यसूचकः};
            assertp->सङ्कलय_Passsp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            यदि (assertp->failsp()) assertp->prependFailsp(ifp->clone_वृक्षः(असत्य));
        }

        यदि (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_ग्रन्थिः* ध्रुव failsp = assertp->failsp();
            File_पङ्क्तिः* ध्रुव flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* ध्रुव remaining_Fail_गणनम्_Varp
                = नूतन AstVar{flp, VVarType::MODULETEMP, m_fail_गणनम्_नामानि.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_गणनम्_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->सङ्कलय_Stmtsp(remaining_Fail_गणनम्_Varp);
            Ast_ग्रन्थिः* ध्रुव replayStmtsp = नूतन Ast_नियोजय{
                flp, नूतन AstVarRef{flp, remaining_Fail_गणनम्_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* ध्रुव replayLoopp = नूतन AstLoop{flp};
            replayLoopp->सङ्कलय_Stmtsp(नूतन AstLoopTest{
                flp, replayLoopp,
                नूतन AstGt{flp, नूतन AstVarRef{flp, remaining_Fail_गणनम्_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_गणनम्_Varp->dtypep(), 1)}});
            replayLoopp->सङ्कलय_Stmtsp(newIfAssertFailOn(failsp->clone_वृक्षः(सत्य), assertp->directive(),
                                                     assertp->userType()));
            AstSub* ध्रुव decrementedFailCountp
                = नूतन AstSub{flp, नूतन AstVarRef{flp, remaining_Fail_गणनम्_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_गणनम्_Varp->dtypep(), 1)};
            replayLoopp->सङ्कलय_Stmtsp(
                नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, remaining_Fail_गणनम्_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->सङ्कलय_अग्र(replayLoopp);
            m_modp->सङ्कलय_Stmtsp(
                नूतन AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    शून्य substitute_मेलनम्_Item_Ref(AstVarRef* refp, अचिह्नित K,
                                ध्रुव std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*>& मेलनम्_Map) {
        ध्रुव स्वचालित it = मेलनम्_Map.find(refp->varp());
        यदि (it == मेलनम्_Map.अन्तः_2()) प्रत्यागम;
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = it->second->clone_वृक्षः_विशुद्धम्(असत्य);
        यदि (K > 0) {
            AstConst* ध्रुव ticksp = नूतन AstConst{refp->fileline(), AstConst::Widthed_मानम्{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* ध्रुव pastp = नूतन AstPast{refp->fileline(), newp, ticksp, शून्यसूचकः,
                                               /* propertyTiming */ सत्य};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(refp), refp);
        प्रत्यागम;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    पूर्णाङ्क
    walk_Substitute_मेलनम्_वस्तूनि(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, अचिह्नित K,
                             ध्रुव std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*>& मेलनम्_वस्तूनि) {
        यदि (AstSExpr* ध्रुव sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* ध्रुव delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->is_आवृत्तिः_Delay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            यदि (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                प्रत्यागम -1;
            }
            ध्रुव अचिह्नित delay_आवृत्तयः = VN_AS(delayp->lhsp(), Const)->गन्तव्यम्_UInt();
            पूर्णाङ्क preLen = 0;
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव prep = sexprp->preExprp()) {
                preLen = walk_Substitute_मेलनम्_वस्तूनि(prep, K, मेलनम्_वस्तूनि);
                यदि (preLen < 0) प्रत्यागम -1;
            }
            ध्रुव पूर्णाङ्क bodyLen
                = walk_Substitute_मेलनम्_वस्तूनि(sexprp->exprp(), K + preLen + delay_आवृत्तयः, मेलनम्_वस्तूनि);
            यदि (bodyLen < 0) प्रत्यागम -1;
            प्रत्यागम preLen + delay_आवृत्तयः + bodyLen;
        }
        यदि (nodep->is_Multi_आवृत्तिः_Sva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            प्रत्यागम -1;
        }
        std::सदिशम्<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.प्रक्षिप_पृष्ठभागः(p); });
        प्रति (AstVarRef* ध्रुव refp : refs) substitute_मेलनम्_Item_Ref(refp, K, मेलनम्_वस्तूनि);
        प्रत्यागम 0;
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
    तार्किक lift_मेलनम्_Item_Substitutions(PropertyParts& parts, Ast_ग्रन्थिः_व्यञ्जकम्* seqBodyp) {
        यदि (!parts.hasImplication) प्रत्यागम असत्य;
        Ast_व्यञ्जकम्_Stmt* ध्रुव व्यञ्जकम्_Stmtp = VN_CAST(parts.triggerExprp, व्यञ्जकम्_Stmt);
        यदि (!व्यञ्जकम्_Stmtp) प्रत्यागम असत्य;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<ध्रुव AstVar*, Ast_ग्रन्थिः_व्यञ्जकम्*> मेलनम्_वस्तूनि;
        प्रति (Ast_ग्रन्थिः* stmtp = व्यञ्जकम्_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_नियोजय* ध्रुव assignp = VN_AS(stmtp, नियोजय);
            AstVarRef* ध्रुव lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            मेलनम्_वस्तूनि[lhsRefp->varp()] = assignp->rhsp();
        }
        ध्रुव अचिह्नित startK = parts.isOverlapped ? 0 : 1;
        ध्रुव पूर्णाङ्क दीर्घता_2 = walk_Substitute_मेलनम्_वस्तूनि(seqBodyp, startK, मेलनम्_वस्तूनि);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_वृक्षः();
        यदि (दीर्घता_2 < 0) प्रत्यागम सत्य;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव antBoolp = व्यञ्जकम्_Stmtp->resultp()->unlink_Fr_पृष्ठभागः();
        व्यञ्जकम्_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(व्यञ्जकम्_Stmtp), व्यञ्जकम्_Stmtp);
        parts.triggerExprp = antBoolp;
        प्रत्यागम असत्य;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    स्थिर ध्रुव वर्ण* unsupportedPropertyControl(ध्रुव Ast_ग्रन्थिः_Cover_Or_Assert* assertp,
                                                  ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* seqBodyp, तार्किक negated) {
        यदि (!hasPropertyControlConjunction(seqBodyp)) प्रत्यागम शून्यसूचकः;
        यदि (negated) प्रत्यागम "negation";
        यदि (VN_IS(assertp, Cover)) प्रत्यागम "cover";
        यदि (VN_AS(assertp, Assert)->passsp()) प्रत्यागम "a pass action";
        प्रत्यागम शून्यसूचकः;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_ग्रन्थिः_Cover_Or_Assert* assertp) {
        यदि (AstPropSpec* ध्रुव specp = VN_CAST(assertp->propp(), PropSpec)) {
            यदि (AstFuncRef* ध्रुव funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                यदि (ध्रुव AstProperty* ध्रुव propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        यदि (AstPropSpec* ध्रुव specp = VN_CAST(assertp->propp(), PropSpec)) {
            यदि (hoistClockedSeq(specp)) प्रत्यागम शून्यसूचकः;
        }

        AstPropSpec* ध्रुव propp = VN_AS(assertp->propp(), PropSpec);
        यदि (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/सत्य);
            प्रत्यागम शून्यसूचकः;
        }

        यदि (!has_Multi_आवृत्तिः_व्यञ्जकम्(propp)) प्रत्यागम शून्यसूचकः;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        यदि (propp->exists([](ध्रुव AstFuncRef* refp) { प्रत्यागम VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(प्रक्षिप_Deletep(assertp->unlink_Fr_पृष्ठभागः()), assertp);
            प्रत्यागम शून्यसूचकः;
        }
        यदि (is_Bare_शिखरम्_Level_Until(propp)) प्रत्यागम शून्यसूचकः;
        प्रत्यागम propp;
    }

    शून्य processAssertion(Ast_ग्रन्थिः_Cover_Or_Assert* assertp) {
        यदि (assertp->तात्कालिकम्()) प्रत्यागम;

        AstPropSpec* ध्रुव propp = prepareAssertionProp(assertp);
        यदि (!propp) प्रत्यागम;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_ग्रन्थिः_व्यञ्जकम्* seqBodyp = parts.seqExprp;
        तार्किक negated = असत्य;
        यावत् (AstLogNot* ध्रुव notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        ध्रुव वर्ण* ध्रुव propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        यदि (lift_मेलनम्_Item_Substitutions(parts, seqBodyp)) {
            AstPropSpec* ध्रुव psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/सत्य);
            प्रत्यागम;
        }

        AstCover* ध्रुव coverp = VN_CAST(assertp, Cover);
        ध्रुव तार्किक isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        ध्रुव तार्किक isSeqEvent = coverp && coverp->isSeqEvent();
        ध्रुव तार्किक गणनम्_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        यदि (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_वृक्षः(सत्य));
        }
        यदि (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_वृक्षः_विशुद्धम्(सत्य));
        }
        यदि (!propp->sensesp()) प्रत्यागम;
        Ast_Sen_वृक्षः* senTreep
            = नूतन Ast_Sen_वृक्षः{propp->fileline(), propp->sensesp()->clone_वृक्षः(सत्य)};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_मानम्_Clockp);
            m_sampled_मानम्_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_पङ्क्तिः* ध्रुव flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_prop_अस्थायी_नामानि,
                              isCoverSeq, isSeqEvent, coverp != शून्यसूचकः};

        ध्रुव Build_परिणाम परिणाम = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        यदि (परिणाम.valid()) wire_मेलनम्_And_Mid_Sources(graph, परिणाम, flp);
        यदि (!परिणाम.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, परिणाम.errorEmitted);
            VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
            प्रत्यागम;
        }
        // After the build, so a construct the builder rejects reports itself.
        यदि (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/सत्य);
            VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
            प्रत्यागम;
        }
        यदि (गणनम्_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(परिणाम.finalCondp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
            Ast_ग्रन्थिः* ध्रुव innerPropp = propp->propp();
            innerPropp->replaceWith(नूतन AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(innerPropp), innerPropp);
            प्रत्यागम;
        }

        AstAssert* ध्रुव assertAssertp = VN_CAST(assertp, Assert);
        ध्रुव तार्किक विभज_Implication_Passsp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && can_विभज_Implication_Pass_Actions(parts);
        ध्रुव तार्किक need_मेलनम् = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || विभज_Implication_Passsp);

        ध्रुव तार्किक needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        ध्रुव स्वचालित सङ्केताः
            = m_loweringp->अधस्तनम्(assertp, graph, senTreep, परिणाम.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_ग्रन्थिः_व्यञ्जकम्* मेलनम्_Exprp = शून्यसूचकः;
        Ast_ग्रन्थिः_व्यञ्जकम्* outputExprp = m_loweringp->assemble_परिणाम(
            assertp, negated, परिणाम.finalCondp, सङ्केताः, need_मेलनम् ? &मेलनम्_Exprp : शून्यसूचकः);
        यदि (गणनम्_Matches) {
            UASSERT_OBJ(सङ्केताः.मेलनम्_Countp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_वृक्षः(), outputExprp);
            propp->मेलनम्_Countp(सङ्केताः.मेलनम्_Countp);
            outputExprp = नूतन AstNeq{flp, सङ्केताः.मेलनम्_Countp->clone_वृक्षः_विशुद्धम्(असत्य),
                                     newTypedConstp(flp, सङ्केताः.मेलनम्_Countp->dtypep(), 0)};
        }

        Ast_Sen_वृक्षः* ध्रुव threadFailReplaySenTreep
            = सङ्केताः.threadFailCountp ? senTreep->clone_वृक्षः(असत्य) : शून्यसूचकः;

        VL_DO_DANGLING(प्रक्षिप_Deletep(senTreep), senTreep);
        यदि (परिणाम.finalCondp && !परिणाम.finalCondp->backp()) प्रक्षिप_Deletep(परिणाम.finalCondp);

        यदि (विभज_Implication_Passsp) {
            विभज_Implication_Pass_Actions(assertAssertp, parts, मेलनम्_Exprp);
        } अन्यथा {
            attachActionHandlers(assertAssertp, मेलनम्_Exprp, threadFailReplaySenTreep,
                                 सङ्केताः.threadFailCountp);
        }

        Ast_ग्रन्थिः* ध्रुव innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << पङ्क्त्यन्तः);

        यदि (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        यदि (!VN_IS(nodep, Class)) m_defaultClockingp = शून्यसूचकः;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstClocking* nodep) override {
        यदि (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Gen_खण्डम्* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstDefaultDisable* nodep) override {}
    शून्य दर्शय(AstFell* nodep) override { दर्शय_Sampled_मानम्(nodep); }
    शून्य दर्शय(AstPast* nodep) override { दर्शय_Sampled_मानम्(nodep); }
    शून्य दर्शय(AstRose* nodep) override { दर्शय_Sampled_मानम्(nodep); }
    शून्य दर्शय(AstStable* nodep) override { दर्शय_Sampled_मानम्(nodep); }
    शून्य दर्शय(AstAssert* nodep) override { processAssertion(nodep); }
    शून्य दर्शय(AstCover* nodep) override { processAssertion(nodep); }
    शून्य दर्शय(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstAssertIntrinsic* nodep) override {}
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    स्पष्ट AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

शून्य V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << पङ्क्त्यन्तः);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्षः("assertnfa", 0, dump_वृक्षः_Either_Level() >= 3);
}
