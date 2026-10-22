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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3AssertNfa.h"

#അന്തര്ഭാവ "V3Assert.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3Task.h"
#അന്തര്ഭാവ "V3UniqueNames.h"

#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// NFA Graph Data Structures (V3Graph-derived per upstream convention)

നാമാകാശ {

വർഗ്ഗം SvaStateVertex;

// Per-vertex algorithm data, stored via V3GraphVertex::userp() during lowering
ഘടന SvaVertexData final {
    AstVar* stateVarp = nullptr;  // NBA state register for this vertex
    AstVar* delayRingVarp = nullptr;  // Packed occupancy bits or per-slot match counts
    AstVar* delayRingHeadVarp
        = nullptr;  // Cached outgoing slot; isolates readers from in-place writes
    AstVar* delayRingIdxVarp = nullptr;  // Next slot written in delayRingVarp
    AstVar* delay_Ring_Live_എണ്ണം_Varp = nullptr;  // Number of threads in delayRingVarp
    AstVar* delayRingWrappedVarp = nullptr;  // All slots written since the last clear
    AstVar* doneLVarp = nullptr;  // SAnd LHS done-latch
    AstVar* doneRVarp = nullptr;  // SAnd RHS done-latch
    Ast_കണ്ണി_വ്യഞ്ജകം* stateSigp = nullptr;  // Combinational state signal; OWNED during lowering
    Ast_കണ്ണി_വ്യഞ്ജകം* state_എണ്ണം_Sigp = nullptr;  // Cover-sequence match count; OWNED during lowering
    Ast_കണ്ണി_വ്യഞ്ജകം* state_എണ്ണം_Overflow_Sigp = nullptr;  // Count overflow; OWNED during lowering
    തർക്കം delayRingCounts = അസത്യം;  // Ring slots hold cover-sequence match counts
    തർക്കം needsReg = അസത്യം;  // True if vertex has incoming clocked edge
};

// NFA state vertex -- one per NFA position in the sequence evaluation
വർഗ്ഗം SvaStateVertex final : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(SvaStateVertex, V3GraphVertex)
പൊതു:
    // True if this is the sequence-match terminal vertex
    തർക്കം m_isMatch = അസത്യം;
    // OWNED throughout-guard condition clones; IEEE 1800-2023 16.9.9
    std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> m_throughoutConds;
    // Nonzero for a ring-buffer vertex for ## delays.
    തർക്കം m_isFixedDelayRing = അസത്യം;
    unsigned m_delayRingSize = 0;  // Number of ring slots. Range: max-min+1.
    Ast_കണ്ണി_വ്യഞ്ജകം* m_delayRingClearCondp = nullptr;  // local RHS for pure-boolean range
    Ast_കണ്ണി_വ്യഞ്ജകം* m_delayRingAdvanceCondp = nullptr;  // Advance only when this condition holds
    തർക്കം m_replayAbortReject = അസത്യം;  // Compressed repetition needs per-thread abort replay
    // OWNED; enclosing-abort fire condition clearing state or suppressing guard rejection
    Ast_കണ്ണി_വ്യഞ്ജകം* m_abortClearp = nullptr;
    // OWNED; reject-abort fire condition rejecting all represented live threads
    Ast_കണ്ണി_വ്യഞ്ജകം* m_abortRejectp = nullptr;
    // Liveness terminal (IEEE weak semantics): reject must not fire from this source
    തർക്കം m_isUnbounded = അസത്യം;
    // Temporal sequence AND combiner; IEEE 1800-2023 16.9.5
    തർക്കം m_isAndCombiner = അസത്യം;
    SvaStateVertex* m_andLhsTermp = nullptr;  // LHS sub-NFA terminal vertex
    SvaStateVertex* m_andRhsTermp = nullptr;  // RHS sub-NFA terminal vertex
    Ast_കണ്ണി_വ്യഞ്ജകം* m_andLhsCondp = nullptr;  // OWNED; LHS final condition (may be nullptr)
    Ast_കണ്ണി_വ്യഞ്ജകം* m_andRhsCondp = nullptr;  // OWNED; RHS final condition (may be nullptr)
    // Reject sink for SAnd rejectOnFail wiring; not a state-signal source
    തർക്കം m_isRejectSink = അസത്യം;
    // In-window vertex of a strong s_always[m:n]: if its state is still set at
    // end-of-simulation the universal-quantifier window never completed, which is
    // a liveness failure (IEEE 1800-2023 16.12.11 strong semantics).
    തർക്കം m_strongPending = അസത്യം;
    // Ring introduced for the checked portion of a bounded strong s_always window.
    തർക്കം m_strongAlwaysRing = അസത്യം;

    // CONSTRUCTORS
    explicit SvaStateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~SvaStateVertex() override {
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* cp : m_throughoutConds) VL_DO_DANGLING(cp->delete_വൃക്ഷം(), cp);
        എങ്കിൽ (m_delayRingClearCondp)
            VL_DO_DANGLING(m_delayRingClearCondp->delete_വൃക്ഷം(), m_delayRingClearCondp);
        എങ്കിൽ (m_delayRingAdvanceCondp)
            VL_DO_DANGLING(m_delayRingAdvanceCondp->delete_വൃക്ഷം(), m_delayRingAdvanceCondp);
        എങ്കിൽ (m_abortClearp) VL_DO_DANGLING(m_abortClearp->delete_വൃക്ഷം(), m_abortClearp);
        എങ്കിൽ (m_abortRejectp) VL_DO_DANGLING(m_abortRejectp->delete_വൃക്ഷം(), m_abortRejectp);
        എങ്കിൽ (m_andLhsCondp) VL_DO_DANGLING(m_andLhsCondp->delete_വൃക്ഷം(), m_andLhsCondp);
        എങ്കിൽ (m_andRhsCondp) VL_DO_DANGLING(m_andRhsCondp->delete_വൃക്ഷം(), m_andRhsCondp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string name() സ്ഥിരം override {
        string name = "s" + cvtToStr(color());
        എങ്കിൽ (m_delayRingSize) {
            name += "\\n";
            name += m_isFixedDelayRing ? "fixed chain " : "range chain ";
            name += cvtToStr(m_delayRingSize) + " slots";
        }
        തിരികെ name;
    }
    string dotColor() സ്ഥിരം override {
        എങ്കിൽ (m_isMatch) തിരികെ "red";
        എങ്കിൽ (m_delayRingSize) തിരികെ "blue";
        എങ്കിൽ (m_isAndCombiner) തിരികെ "purple";
        തിരികെ "black";
    }
    // LCOV_EXCL_STOP
    // Access per-vertex algorithm data (valid only during lowering phase)
    SvaVertexData* datap() സ്ഥിരം { തിരികെ static_cast<SvaVertexData*>(userp()); }
};

// NFA transition edge -- clocked (##1) or combinational link (##0)
വർഗ്ഗം SvaTransEdge final : പൊതു V3GraphEdge {
    VL_RTTI_IMPL(SvaTransEdge, V3GraphEdge)
പൊതു:
    Ast_കണ്ണി_വ്യഞ്ജകം* m_condp;  // Transition condition; nullptr = unconditional; OWNED
    തർക്കം m_consumesCycle;  // true = clocked edge (##1), false = link (##0/boolean)
    // Reject when source is active and condp is false; set only on
    // outermost required-step Link
    തർക്കം m_rejectOnFail = അസത്യം;
    // Optional dynamic condition vertex for m_rejectOnFail. Used when the
    // success condition is another NFA state rather than a static expression.
    SvaStateVertex* m_condVtxp = nullptr;

    // CONSTRUCTORS
    SvaTransEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* മുകൾ, Ast_കണ്ണി_വ്യഞ്ജകം* condp,
                 തർക്കം consumesCycle)
        : V3GraphEdge{graphp, fromp, മുകൾ, /*weight=*/1}
        , m_condp{condp}
        , m_consumesCycle{consumesCycle} {}
    ~SvaTransEdge() override {
        എങ്കിൽ (m_condp) VL_DO_DANGLING(m_condp->delete_വൃക്ഷം(), m_condp);
    }
    // METHODS
    // LCOV_EXCL_START -- Graphviz dump only
    string dotLabel() സ്ഥിരം override { തിരികെ m_consumesCycle ? "##1" : "link"; }
    string dotStyle() സ്ഥിരം override { തിരികെ m_consumesCycle ? "" : "dashed"; }
    // LCOV_EXCL_STOP
    // Typed accessors for NFA vertices
    SvaStateVertex* fromVtxp() സ്ഥിരം { തിരികെ static_cast<SvaStateVertex*>(fromp()); }
    SvaStateVertex* toVtxp() സ്ഥിരം { തിരികെ static_cast<SvaStateVertex*>(മുകൾ()); }
};

// NFA graph container
വർഗ്ഗം SvaGraph final {
പൊതു:
    V3Graph m_graph;  // Owns all vertices and edges
    SvaStateVertex* m_startVertexp = nullptr;  // Trigger/start vertex
    SvaStateVertex* m_matchVertexp = nullptr;  // Sequence-match terminal vertex

    // Create a new state vertex
    SvaStateVertex* createStateVertex() { തിരികെ പുതിയ SvaStateVertex{&m_graph}; }
    // Create the match terminal vertex
    SvaStateVertex* createMatchVertex() {
        SvaStateVertex* സ്ഥിരം vtxp = createStateVertex();
        vtxp->m_isMatch = സത്യം;
        m_matchVertexp = vtxp;
        തിരികെ vtxp;
    }
    // Add a clocked transition edge (##1)
    SvaTransEdge* addClockedEdge(SvaStateVertex* fromp, SvaStateVertex* മുകൾ,
                                 Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr) {
        തിരികെ പുതിയ SvaTransEdge{&m_graph, fromp, മുകൾ, condp, /*consumesCycle=*/സത്യം};
    }
    // Add a combinational link (##0 / boolean condition)
    SvaTransEdge* addLink(SvaStateVertex* fromp, SvaStateVertex* മുകൾ,
                          Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr) {
        തിരികെ പുതിയ SvaTransEdge{&m_graph, fromp, മുകൾ, condp, /*consumesCycle=*/അസത്യം};
    }
    // Collect all edges into a flat vector for iteration.
    // Used by the lowering phase which needs global edge scans.
    std::vector<സ്ഥിരം SvaTransEdge*> allEdges() സ്ഥിരം {
        std::vector<സ്ഥിരം SvaTransEdge*> result;
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtxr : m_graph.vertices()) {
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edger : vtxr.outEdges()) {
                result.തള്ളുക_back(static_cast<സ്ഥിരം SvaTransEdge*>(&edger));
            }
        }
        തിരികെ result;
    }
};

//######################################################################
// Builder result: terminal vertex + optional final condition (match Link condition).
ഘടന BuildResult final {
    SvaStateVertex* termVertexp;  // Primary terminal; contributes to both match and reject
    Ast_കണ്ണി_വ്യഞ്ജകം* finalCondp;  // nullptr = unconditional
    // Mid-window sources for range delays (pure boolean RHS): match-only (isUnbounded)
    std::vector<SvaStateVertex*> midSources;
    തർക്കം errorEmitted = അസത്യം;  // Builder already emitted specific error; skip generic
    // For cover_sequence: when true, midSources already enumerate every
    // end-of-match, so wireMatchAndMidSources must NOT add the main
    // termVtxp -> matchVertex Link (would double-count via the merge vertex).
    തർക്കം termIsMidMerge = അസത്യം;
    തർക്കം valid() സ്ഥിരം { തിരികെ termVertexp != nullptr; }
    സ്ഥിതം BuildResult fail(തർക്കം errored = അസത്യം) { തിരികെ {nullptr, nullptr, {}, errored}; }
    സ്ഥിതം BuildResult failWithError() { തിരികെ {nullptr, nullptr, {}, സത്യം}; }
};

// Parser-marked SAnd of overlapped implications: a property if/else/case.
സ്ഥിതം തർക്കം hasPropertyControlConjunction(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
    തിരികെ nodep->exists([](സ്ഥിരം AstSAnd* andp) { തിരികെ andp->propertyControl(); });
}

സ്ഥിതം AstConst* newTypedConstp(File_വരി* സ്ഥിരം flp, സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep,
                                സ്ഥിരം uint32_t മൂല്യം) {
    AstConst* സ്ഥിരം constp = പുതിയ AstConst{flp, AstConst::DTyped{}, dtypep};
    constp->അക്കം().setLong(മൂല്യം);
    തിരികെ constp;
}

സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* sampled(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
    തിരികെ പുതിയ AstSampled{exprp->fileline(), exprp, exprp->dtypep(), സത്യം};
}

സ്ഥിതം സ്ഥിരം അക്ഷരം* assertPassOnQuery(തർക്കം vacuous) {
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* queries[2]
        = {"ASSERT_CTL_PASS_ON_NONVACUOUS", "ASSERT_CTL_PASS_ON_VACUOUS"};
    തിരികെ queries[vacuous];
}

സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* assertOnCond(File_വരി* flp, VAssertType type,
                                 VAssertDirectiveType directiveType) {
    എങ്കിൽ (!v3Global.opt.assertOn()) { തിരികെ പുതിയ AstConst{flp, AstConst::BitFalse{}}; }
    തിരികെ പുതിയ AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_ON", type, directiveType), 1};
}

സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* assertKillGet(File_വരി* flp, VAssertType type,
                                  VAssertDirectiveType directiveType) {
    തിരികെ പുതിയ AstCExpr{flp, AstCExpr::Pure{},
                        V3AssertCommon::assertCtlGetCall("ASSERT_CTL_KILL", type, directiveType),
                        32};
}

സ്ഥിതം string assertActionControlPrefix(VAssertDirectiveType directiveType) {
    സ്ഥിരം പൂർണ്ണസംഖ്യ controlled = !!(static_cast<പൂർണ്ണസംഖ്യ>(directiveType)
                              & (static_cast<പൂർണ്ണസംഖ്യ>(VAssertDirectiveType::ASSERT)
                                 | static_cast<പൂർണ്ണസംഖ്യ>(VAssertDirectiveType::COVER)
                                 | static_cast<പൂർണ്ണസംഖ്യ>(VAssertDirectiveType::ASSUME)));
    സ്ഥിരം പൂർണ്ണസംഖ്യ checkRuntime = controlled & static_cast<പൂർണ്ണസംഖ്യ>(v3Global.opt.assertOn());
    തിരികെ "("s + std::to_string(controlled ^ 1) + " || ("s + std::to_string(checkRuntime)
           + " && "s;
}

സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* assertPassOnCond(File_വരി* flp, VAssertType type,
                                     VAssertDirectiveType directiveType, തർക്കം vacuous) {
    തിരികെ പുതിയ AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall(assertPassOnQuery(vacuous), type, directiveType)
            + "))"s,
        1};
}

സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* assertFailOnCond(File_വരി* flp, VAssertType type,
                                     VAssertDirectiveType directiveType) {
    തിരികെ പുതിയ AstCExpr{
        flp, AstCExpr::Pure{},
        assertActionControlPrefix(directiveType)
            + V3AssertCommon::assertCtlGetCall("ASSERT_CTL_FAIL_ON", type, directiveType) + "))"s,
        1};
}

സ്ഥിതം AstIf* newPassOnIf(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* firep, Ast_കണ്ണി* bodyp, VAssertType type,
                          VAssertDirectiveType directiveType, തർക്കം vacuous) {
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp
        = പുതിയ AstLogAnd{flp, firep, assertPassOnCond(flp, type, directiveType, vacuous)};
    AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(സത്യം);
    ifp->user1(സത്യം);
    തിരികെ ifp;
}

സ്ഥിതം Ast_കണ്ണി_Stmt* newIfAssertFailOn(Ast_കണ്ണി* bodyp, VAssertDirectiveType directiveType,
                                      VAssertType type) {
    File_വരി* സ്ഥിരം flp = bodyp->fileline();
    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = assertFailOnCond(flp, type, directiveType);
    AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, condp, bodyp};
    ifp->isBoundsCheck(സത്യം);
    ifp->user1(സത്യം);
    തിരികെ ifp;
}

//######################################################################
// NFA Builder

വർഗ്ഗം SvaNfaBuilder final {
    SvaGraph& m_graph;  // NFA graph being built
    Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Module to receive hoisted sampled-prop temps
    V3UniqueNames& m_propTempNames;  // Module-shared temp-var name source
    std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> m_temporal_Guard_അട്ടി;  // Guards active across nested temporal states
    // Outer abort conditions, AND-ed as !cond into inner abort edges
    // (IEEE 1800-2023 16.12.14 outer-wraps-inner).
    std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> m_outer_Abort_അട്ടി;
    തർക്കം m_inUnboundedScope = അസത്യം;  // Sticky: nodes created after inherit liveness
    തർക്കം m_markStrongPending = അസത്യം;  // Mark new vertices as strong s_always in-window
    തർക്കം m_isCover = അസത്യം;  // Cover directives do not fail at end-of-simulation
    // IEEE 1800-2023 16.14.3 cover sequence: each end-of-match fires the action,
    // not just the first. Builder builds parallel-branch (no first-match-wins)
    // topology when true. Default false preserves cover_property semantics.
    തർക്കം m_isCoverSeq = അസത്യം;
    // Unsupported endpoint topology must reject, not ignore, or the wait hangs
    തർക്കം m_isSeqEvent = അസത്യം;
    തർക്കം m_inSequencePrefix = അസത്യം;  // Prefix composition does not preserve every endpoint
    തർക്കം m_wantBoolLeaf = അസത്യം;  // Building a |-> / |=> or top-level body (see buildOrMerge)

    ഘടന RangeDelayRejectInfo final {
        SvaStateVertex* startp = nullptr;
        unsigned range = 0;
        പൂർണ്ണസംഖ്യ rhsLen = 0;
    };

    ശൂന്യം warnEndpointUnsupported(File_വരി* flp, സ്ഥിരം string& what) സ്ഥിരം {
        എങ്കിൽ (m_isSeqEvent) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: sequence used as an event control with " << what);
        } അല്ലെങ്കിൽ {
            flp->v3warn(COVERIGN, "Ignoring unsupported: cover sequence with " << what);
        }
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* throughoutCond(Ast_കണ്ണി_വ്യഞ്ജകം* baseCondp, File_വരി* flp) {
        എങ്കിൽ (m_temporal_Guard_അട്ടി.empty()) തിരികെ baseCondp;
        // AND all active temporal guards (supports nesting)
        // Each must use $sampled values.
        Ast_കണ്ണി_വ്യഞ്ജകം* guardp = nullptr;
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp : m_temporal_Guard_അട്ടി) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം clonep = sampled(condp->clone_വൃക്ഷം_Pure(അസത്യം));
            എങ്കിൽ (!guardp) {
                guardp = clonep;
            } അല്ലെങ്കിൽ {
                guardp = പുതിയ AstLogAnd{flp, guardp, clonep};
            }
        }
        എങ്കിൽ (baseCondp) { guardp = പുതിയ AstLogAnd{flp, baseCondp, guardp}; }
        തിരികെ guardp;
    }

    സ്ഥിതം unsigned getConstUInt(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം constp = V3Const::constifyEdit(exprp->clone_വൃക്ഷം_Pure(അസത്യം));
        സ്ഥിരം AstConst* സ്ഥിരം cp = VN_CAST(constp, Const);
        സ്ഥിരം unsigned val = cp ? cp->toUInt() : 0;
        VL_DO_DANGLING(constp->delete_വൃക്ഷം(), constp);
        തിരികെ val;
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
    സ്ഥിതം പൂർണ്ണസംഖ്യ fixedLength(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (AstSExpr* സ്ഥിരം sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* സ്ഥിരം delayp = VN_CAST(sexprp->delayp(), Delay);
            എങ്കിൽ (!delayp || !delayp->isCycleDelay()) തിരികെ -1;
            unsigned delayCycles;
            എങ്കിൽ (delayp->isRangeDelay()) {
                എങ്കിൽ (delayp->isUnbounded()) തിരികെ -1;  // LCOV_EXCL_LINE
                സ്ഥിരം unsigned minD = getConstUInt(delayp->lhsp());
                സ്ഥിരം unsigned maxD = getConstUInt(delayp->rhsp());
                എങ്കിൽ (minD != maxD) തിരികെ -1;
                delayCycles = minD;
            } അല്ലെങ്കിൽ {
                delayCycles = getConstUInt(delayp->lhsp());
            }
            പൂർണ്ണസംഖ്യ preLen = 0;
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prep = sexprp->preExprp()) {
                preLen = fixedLength(prep);
                എങ്കിൽ (preLen < 0) തിരികെ -1;  // LCOV_EXCL_LINE
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ bodyLen = fixedLength(sexprp->exprp());
            എങ്കിൽ (bodyLen < 0) തിരികെ -1;  // LCOV_EXCL_LINE
            തിരികെ preLen + delayCycles + bodyLen;
        }
        എങ്കിൽ (AstSThroughout* സ്ഥിരം throughp = VN_CAST(nodep, SThroughout)) {
            തിരികെ fixedLength(throughp->rhsp());
        }
        എങ്കിൽ (AstSOr* സ്ഥിരം orp = VN_CAST(nodep, SOr)) {
            // Alternatives must share one end cycle; buildSWithin relies on
            // this to pair the OR with an SIntersect.
            സ്ഥിരം പൂർണ്ണസംഖ്യ lhsLen = fixedLength(orp->lhsp());
            സ്ഥിരം പൂർണ്ണസംഖ്യ rhsLen = fixedLength(orp->rhsp());
            എങ്കിൽ (lhsLen < 0 || rhsLen < 0 || lhsLen != rhsLen) തിരികെ -1;
            തിരികെ lhsLen;
        }
        എങ്കിൽ (AstSWithin* സ്ഥിരം withinp = VN_CAST(nodep, SWithin)) {
            // `seq1 within seq2` ends at seq2's end cycle (IEEE 16.9.10).
            സ്ഥിരം പൂർണ്ണസംഖ്യ lhsLen = fixedLength(withinp->lhsp());
            സ്ഥിരം പൂർണ്ണസംഖ്യ rhsLen = fixedLength(withinp->rhsp());
            എങ്കിൽ (lhsLen < 0 || rhsLen < 0 || lhsLen > rhsLen) തിരികെ -1;
            തിരികെ rhsLen;
        }
        // LCOV_EXCL_START -- defensive: V3AssertPre rejects composite SVA ops
        // nested in an intersect arm before fixedLength runs (clock-context
        // resolution fails). Kept as a guard in case future parser relaxations
        // permit it.
        എങ്കിൽ (nodep->isMultiCycleSva()) തിരികെ -1;
        // LCOV_EXCL_STOP
        // Plain boolean expression (no SVA constructs) -- 0 cycles.
        തിരികെ 0;
    }

    // Contiguous match-length range [lo,hi] for an operand whose length varies
    // from AT MOST ONE ranged cycle delay; {-1,-1} otherwise. Drives the
    // variable-length `intersect` lowering (IEEE 1800-2023 16.9.6): more than
    // one ranged delay would make the per-length realization ambiguous, so it
    // is reported unsupported rather than mis-paired.
    സ്ഥിതം std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ> lengthRange(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (AstSExpr* സ്ഥിരം sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* സ്ഥിരം delayp = VN_CAST(sexprp->delayp(), Delay);
            എങ്കിൽ (!delayp || !delayp->isCycleDelay()) തിരികെ {-1, -1};
            std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ> delayRange;
            എങ്കിൽ (delayp->isRangeDelay()) {
                എങ്കിൽ (delayp->isUnbounded()) തിരികെ {-1, -1};
                സ്ഥിരം unsigned minD = getConstUInt(delayp->lhsp());
                സ്ഥിരം unsigned maxD = getConstUInt(delayp->rhsp());
                delayRange = {minD, maxD};
            } അല്ലെങ്കിൽ {
                സ്ഥിരം unsigned d = getConstUInt(delayp->lhsp());
                delayRange = {d, d};
            }
            std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ> preRange{0, 0};
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prep = sexprp->preExprp()) {
                preRange = lengthRange(prep);
                എങ്കിൽ (preRange.first < 0) തിരികെ {-1, -1};
            }
            സ്ഥിരം std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ> bodyRange = lengthRange(sexprp->exprp());
            എങ്കിൽ (bodyRange.first < 0) തിരികെ {-1, -1};
            സ്ഥിരം പൂർണ്ണസംഖ്യ variableParts = (preRange.first != preRange.second)
                                      + (delayRange.first != delayRange.second)
                                      + (bodyRange.first != bodyRange.second);
            എങ്കിൽ (variableParts > 1) തിരികെ {-1, -1};
            തിരികെ {preRange.first + delayRange.first + bodyRange.first,
                    preRange.second + delayRange.second + bodyRange.second};
        }
        എങ്കിൽ (AstSThroughout* സ്ഥിരം throughp = VN_CAST(nodep, SThroughout)) {
            തിരികെ lengthRange(throughp->rhsp());
        }
        എങ്കിൽ (nodep->isMultiCycleSva()) തിരികെ {-1, -1};
        തിരികെ {0, 0};  // plain boolean -- 0 cycles
    }

    // Clone `operand` with its sole variable ranged cycle delay pinned so the
    // total match length is exactly `len`. `lo` is the operand's minimum length
    // (lengthRange().first). A fixed operand has no such delay and is returned
    // as a plain clone (callers only request its single achievable length).
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* realizeAtLength(Ast_കണ്ണി_വ്യഞ്ജകം* operand, പൂർണ്ണസംഖ്യ len, പൂർണ്ണസംഖ്യ lo) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം clonep = operand->clone_വൃക്ഷം_Pure(അസത്യം);
        AstDelay* rangeDelayp = nullptr;
        clonep->foreach([&](AstDelay* dp) {
            എങ്കിൽ (!rangeDelayp && dp->isRangeDelay() && !dp->isUnbounded()
                && getConstUInt(dp->lhsp()) != getConstUInt(dp->rhsp())) {
                rangeDelayp = dp;
            }
        });
        എങ്കിൽ (rangeDelayp) {
            File_വരി* സ്ഥിരം flp = rangeDelayp->fileline();
            സ്ഥിരം unsigned pinned = getConstUInt(rangeDelayp->lhsp()) + (len - lo);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം oldMinp = rangeDelayp->lhsp();
            oldMinp->replaceWith(പുതിയ AstConst{flp, pinned});
            VL_DO_DANGLING(oldMinp->delete_വൃക്ഷം(), oldMinp);
            // Drop the max bound so it lowers as a fixed `##d`, not `##[d:d]`.
            Ast_കണ്ണി* സ്ഥിരം oldMaxp = rangeDelayp->rhsp()->unlinkFrBack();
            VL_DO_DANGLING(oldMaxp->delete_വൃക്ഷം(), oldMaxp);
        }
        തിരികെ clonep;
    }

    // Cuts AST size from O(N * sizeof(exprp)) to O(N) + O(sizeof(exprp)) by
    // sharing a single `VarRef` across N check edges. Hoist also matches the
    // IEEE 1800-2023 16.9.9 "single preponed-region snapshot" semantic for
    // any exprp -- even an impure one would now evaluate exactly once per
    // clock instead of N times. Orphan temps from failed builds are unused
    // MODULETEMPs and are removed by V3Dead.
    AstVar* tryHoistSampled(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, File_വരി* flp, unsigned clone_എണ്ണം) {
        constexpr unsigned kHoistThreshold = 2;
        എങ്കിൽ (clone_എണ്ണം < kHoistThreshold) തിരികെ nullptr;
        AstVar* സ്ഥിരം tempVarp
            = പുതിയ AstVar{flp, VVarType::MODULETEMP, m_propTempNames.get(exprp), exprp->dtypep()};
        m_modp->addStmtsp(tempVarp);
        AstAssign* സ്ഥിരം assignp = പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, tempVarp, VAccess::WRITE},
                                                 sampled(exprp->clone_വൃക്ഷം_Pure(അസത്യം))};
        m_modp->addStmtsp(പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS_COMB, nullptr, assignp});
        തിരികെ tempVarp;
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* sampledRefOrClone(AstVar* hoistVarp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, File_വരി* flp) {
        എങ്കിൽ (hoistVarp) തിരികെ പുതിയ AstVarRef{flp, hoistVarp, VAccess::READ};
        തിരികെ sampled(exprp->clone_വൃക്ഷം_Pure(അസത്യം));
    }

    // Create vertex and inherit temporal guards from the current scope.
    SvaStateVertex* scopedCreateVertex() {
        SvaStateVertex* സ്ഥിരം vtxp = m_graph.createStateVertex();
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp : m_temporal_Guard_അട്ടി) {
            vtxp->m_throughoutConds.തള്ളുക_back(cp->clone_വൃക്ഷം_Pure(അസത്യം));
        }
        എങ്കിൽ (m_inUnboundedScope) vtxp->m_isUnbounded = സത്യം;
        എങ്കിൽ (m_markStrongPending) vtxp->m_strongPending = സത്യം;
        തിരികെ vtxp;
    }

    // AND current temporal guards into every edge/link.
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* മുകൾ, Ast_കണ്ണി_വ്യഞ്ജകം* condp,
                              File_വരി* flp) {
        തിരികെ m_graph.addLink(fromp, മുകൾ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedLink(SvaStateVertex* fromp, SvaStateVertex* മുകൾ, File_വരി* flp) {
        തിരികെ m_graph.addLink(fromp, മുകൾ, throughoutCond(nullptr, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* മുകൾ, Ast_കണ്ണി_വ്യഞ്ജകം* condp,
                              File_വരി* flp) {
        തിരികെ m_graph.addClockedEdge(fromp, മുകൾ, throughoutCond(condp, flp));
    }
    SvaTransEdge* guardedEdge(SvaStateVertex* fromp, SvaStateVertex* മുകൾ, File_വരി* flp) {
        തിരികെ m_graph.addClockedEdge(fromp, മുകൾ, throughoutCond(nullptr, flp));
    }

    SvaStateVertex* addDelayChain(SvaStateVertex* startp, unsigned size, File_വരി* flp,
                                  തർക്കം isFixed = സത്യം, Ast_കണ്ണി_വ്യഞ്ജകം* clearCondp = nullptr,
                                  Ast_കണ്ണി_വ്യഞ്ജകം* advanceCondp = nullptr) {
        എങ്കിൽ (isFixed && size == 0) തിരികെ startp;
        UASSERT_OBJ(size > 0, startp, "Delay chain needs at least one slot");
        എങ്കിൽ (isFixed && size == 1 && !advanceCondp) {
            SvaStateVertex* സ്ഥിരം nextp = scopedCreateVertex();
            guardedEdge(startp, nextp, flp);
            തിരികെ nextp;
        }
        SvaStateVertex* സ്ഥിരം ringVtxp = scopedCreateVertex();
        ringVtxp->m_isFixedDelayRing = isFixed;
        ringVtxp->m_delayRingSize = size;
        എങ്കിൽ (clearCondp) {
            UASSERT_OBJ(!isFixed, startp, "Fixed delay cannot have a clear condition");
            ringVtxp->m_delayRingClearCondp = clearCondp->clone_വൃക്ഷം_Pure(അസത്യം);
        }
        ringVtxp->m_delayRingAdvanceCondp = advanceCondp;
        എങ്കിൽ (isFixed) {
            guardedEdge(startp, ringVtxp, flp);
        } അല്ലെങ്കിൽ {
            guardedLink(startp, ringVtxp, flp);
        }
        തിരികെ ringVtxp;
    }

    // Build NFA for an SExpr. finalCond = RHS (not yet added as a vertex).
    // isTopLevelStep: marks outermost required boolean check as rejectOnFail.
    // Apply a range delay `##[M:N]` to currentp. Returns true on success. On
    // failure, emits a diagnostic and returns false.
    തർക്കം applyRangeDelay(AstSExpr* sexprp, SvaStateVertex*& currentp,
                         std::vector<SvaStateVertex*>& midSources,
                         RangeDelayRejectInfo* rangeRejectInfop = nullptr) {
        File_വരി* സ്ഥിരം flp = sexprp->fileline();
        AstDelay* സ്ഥിരം delayp = VN_AS(sexprp->delayp(), Delay);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsExprp = sexprp->exprp();
        സ്ഥിരം unsigned minDelay = getConstUInt(delayp->lhsp());
        എങ്കിൽ (delayp->isUnbounded()) {
            // `##[M:$]`: wait M cycles, then retain every matured attempt in a
            // one-slot ring while also exposing the first eligible tick.
            currentp = addDelayChain(currentp, minDelay, flp);
            SvaStateVertex* സ്ഥിരം waitRingp = addDelayChain(currentp, 1, flp, അസത്യം);
            // The ring's live count is updated in NBA, so this link retains the previous
            // tick's live attempts alongside new arrivals for the next tick.
            guardedLink(waitRingp, waitRingp, flp);
            SvaStateVertex* സ്ഥിരം mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(waitRingp, mergeVtxp, flp);
            currentp = mergeVtxp;
            currentp->m_isUnbounded = സത്യം;  // Liveness, so no reject.
            m_inUnboundedScope = സത്യം;
            തിരികെ സത്യം;
        }
        സ്ഥിരം unsigned maxDelay = getConstUInt(delayp->rhsp());
        എങ്കിൽ (minDelay == maxDelay) {
            currentp = addDelayChain(currentp, minDelay, flp);
            തിരികെ സത്യം;
        }
        സ്ഥിരം unsigned range = maxDelay - minDelay;
        currentp = addDelayChain(currentp, minDelay, flp);
        സ്ഥിരം തർക്കം multiCycleRhs = rhsExprp->isMultiCycleSva();
        സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം preExprp = sexprp->preExprp();
        // Prefix and nested-sequence composition can lose endpoints or their multiplicity
        // required by cover sequence (IEEE 1800-2023 16.14.3), regardless of the range width.
        എങ്കിൽ (m_isCoverSeq
            && (multiCycleRhs || m_inSequencePrefix
                || (preExprp && preExprp->isMultiCycleSva()))) {
            warnEndpointUnsupported(flp, "this ranged cycle delay");
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (m_inSequencePrefix) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: Bounded ranged cycle delay in a sequence prefix");
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (multiCycleRhs) {
            // Launch a candidate at every eligible tick. Candidate-local misses are not
            // assertion rejects while a later position can still match.
            എങ്കിൽ (rangeRejectInfop) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ rhsLen = fixedLength(rhsExprp);
                എങ്കിൽ (rhsLen >= 0) *rangeRejectInfop = {currentp, range, rhsLen};
            }
            SvaStateVertex* സ്ഥിരം tailp = addDelayChain(currentp, range + 1U, flp, അസത്യം);
            SvaStateVertex* സ്ഥിരം mergeVtxp = scopedCreateVertex();
            mergeVtxp->m_isUnbounded = സത്യം;
            guardedLink(currentp, mergeVtxp, flp);
            guardedLink(tailp, mergeVtxp, flp);
            currentp = mergeVtxp;
            m_inUnboundedScope = സത്യം;
        } അല്ലെങ്കിൽ {
            // The first eligible tick is explicit. The ring retains attempts for the
            // remaining ticks and exposes the final tick for rejection. Only properties
            // clear on success, since cover sequence counts every endpoint.
            midSources.തള്ളുക_back(currentp);
            currentp = addDelayChain(currentp, range + 1U, flp, അസത്യം,
                                     m_isCoverSeq ? nullptr : rhsExprp);
        }
        തിരികെ സത്യം;
    }

    ശൂന്യം addFiniteRangeReject(സ്ഥിരം RangeDelayRejectInfo& info, സ്ഥിരം BuildResult& result,
                              File_വരി* flp) {
        എങ്കിൽ (!info.startp) തിരികെ;

        SvaStateVertex* സ്ഥിരം expiryVtxp
            = addDelayChain(info.startp, info.range + info.rhsLen, flp);
        SvaStateVertex* സ്ഥിരം expiryMatchp = scopedCreateVertex();
        std::vector<SvaStateVertex*> sources = result.midSources;
        sources.തള്ളുക_back(result.termVertexp);
        വേണ്ടി (SvaStateVertex* സ്ഥിരം srcp : sources) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp
                = result.finalCondp ? sampled(result.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)) : nullptr;
            SvaStateVertex* സ്ഥിരം successNowp = scopedCreateVertex();
            guardedLink(srcp, successNowp, condp, flp);
            // Retain a success through the latest deadline it can satisfy.
            SvaStateVertex* സ്ഥിരം historyp
                = addDelayChain(successNowp, info.range + 1U, flp, അസത്യം);
            guardedLink(successNowp, expiryMatchp, flp);
            guardedLink(historyp, expiryMatchp, flp);
        }

        SvaStateVertex* സ്ഥിരം sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = സത്യം;
        SvaTransEdge* സ്ഥിരം rejectp = m_graph.addLink(expiryVtxp, sinkVtxp);
        rejectp->m_rejectOnFail = സത്യം;
        rejectp->m_condVtxp = expiryMatchp;
    }

    BuildResult buildSExpr(AstSExpr* sexprp, SvaStateVertex* entryVtxp,
                           തർക്കം is_മുകൾ_Level_Step = അസത്യം) {
        AstDelay* സ്ഥിരം delayp = VN_CAST(sexprp->delayp(), Delay);
        എങ്കിൽ (!delayp || !delayp->isCycleDelay()) തിരികെ BuildResult::fail();

        File_വരി* സ്ഥിരം flp = sexprp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = sexprp->exprp();

        // Handle LHS (preExpr)
        SvaStateVertex* currentp = entryVtxp;
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം preExprp = sexprp->preExprp()) {
            VL_RESTORER(m_inSequencePrefix);
            m_inSequencePrefix = സത്യം;
            സ്ഥിരം BuildResult pre = build_വ്യഞ്ജകം(preExprp, currentp, is_മുകൾ_Level_Step);
            എങ്കിൽ (!pre.valid()) തിരികെ BuildResult::fail(pre.errorEmitted);  // LCOV_EXCL_LINE
            എങ്കിൽ (pre.finalCondp) {
                SvaStateVertex* സ്ഥിരം condVtxp = scopedCreateVertex();
                SvaTransEdge* സ്ഥിരം edgep = guardedLink(
                    pre.termVertexp, condVtxp, sampled(pre.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)), flp);
                എങ്കിൽ (is_മുകൾ_Level_Step && !pre.termVertexp->m_isUnbounded && !m_inUnboundedScope) {
                    // Do not mark liveness sources: first boolean check is deferred.
                    edgep->m_rejectOnFail = സത്യം;
                }
                freeUnlinkedCondp(pre.finalCondp);
                currentp = condVtxp;
            } അല്ലെങ്കിൽ {
                currentp = pre.termVertexp;
            }
        }

        // Handle delay
        std::vector<SvaStateVertex*> rangeMidSources;
        RangeDelayRejectInfo rangeRejectInfo;
        സ്ഥിരം തർക്കം addRangeReject = is_മുകൾ_Level_Step && !m_inUnboundedScope;
        എങ്കിൽ (delayp->isRangeDelay()) {
            എങ്കിൽ (!applyRangeDelay(sexprp, currentp, rangeMidSources,
                                 addRangeReject ? &rangeRejectInfo : nullptr)) {
                തിരികെ BuildResult::failWithError();
            }
        } അല്ലെങ്കിൽ {
            സ്ഥിരം unsigned delayCycles = getConstUInt(delayp->lhsp());
            currentp = addDelayChain(currentp, delayCycles, flp);
        }

        // Multi-cycle RHS: recurse (only plain boolean is returned as finalCondp).
        എങ്കിൽ (exprp->isMultiCycleSva()) {
            സ്ഥിരം BuildResult result = build_വ്യഞ്ജകം(exprp, currentp, is_മുകൾ_Level_Step);
            എങ്കിൽ (result.valid()) addFiniteRangeReject(rangeRejectInfo, result, flp);
            തിരികെ result;
        }
        തിരികെ {currentp, exprp, std::move(rangeMidSources)};
    }

    BuildResult buildConsRep(AstSConsRep* repp, SvaStateVertex* entryVtxp,
                             തർക്കം is_മുകൾ_Level_Step = അസത്യം) {
        File_വരി* സ്ഥിരം flp = repp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = repp->exprp();
        // Multi-cycle expr in ConsRep not yet supported; bail to avoid invalid AST.
        എങ്കിൽ (exprp->isMultiCycleSva()) {
            repp->v3warn(E_UNSUPPORTED, "Unsupported: multi-cycle sequence expression inside"
                                        " consecutive repetition (IEEE 1800-2023 16.9.2)");
            തിരികെ BuildResult::failWithError();
        }
        സ്ഥിരം unsigned minN = getConstUInt(repp->countp());

        // Sum sites across prefix + unbounded/range tail so one hoist covers
        // every check edge of this repetition.
        unsigned totalSites = minN;
        എങ്കിൽ (repp->unbounded()) {
            totalSites += 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (repp->maxCountp()) {
            totalSites += getConstUInt(repp->maxCountp()) - minN;
        }
        AstVar* സ്ഥിരം hoistVarp = tryHoistSampled(exprp, flp, totalSites);

        // Cover-sequence (IEEE 1800-2023 16.14.3): collect each end-of-match
        // position so they all fire the action, not just the merged terminal.
        std::vector<SvaStateVertex*> consMidSources;

        SvaStateVertex* currentp = entryVtxp;
        വേണ്ടി (unsigned i = 0; i < minN; ++i) {
            // Keep the first repetition explicit, collapse all remaining checks into the ring.
            എങ്കിൽ (i == 1) {
                currentp = addDelayChain(currentp, minN - 1, flp);
                currentp->m_delayRingClearCondp
                    = പുതിയ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                currentp->m_replayAbortReject = സത്യം;
                എങ്കിൽ (is_മുകൾ_Level_Step) {
                    currentp->m_throughoutConds.തള്ളുക_back(
                        sampledRefOrClone(hoistVarp, exprp, flp));
                }
                i = minN - 1;
            }
            // Every repetition in the minimum prefix is required.
            SvaStateVertex* സ്ഥിരം condVtxp = scopedCreateVertex();
            SvaTransEdge* സ്ഥിരം linkp
                = guardedLink(currentp, condVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            // Only an outermost required step rejects the property. Its first check is explicit,
            // later required checks reject through the ring's throughout guard.
            എങ്കിൽ (is_മുകൾ_Level_Step && i == 0) linkp->m_rejectOnFail = സത്യം;
            currentp = condVtxp;
        }
        // After minN: currentp is the first valid end-of-match position for [*m:n].
        എങ്കിൽ (m_isCoverSeq && (repp->unbounded() || repp->maxCountp())) {
            consMidSources.തള്ളുക_back(currentp);
        }

        എങ്കിൽ (repp->unbounded()) {
            എങ്കിൽ (minN == 0) {
                SvaStateVertex* സ്ഥിരം waitVtxp = scopedCreateVertex();
                guardedEdge(currentp, waitVtxp, flp);
                SvaStateVertex* സ്ഥിരം checkVtxp = scopedCreateVertex();
                guardedLink(waitVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedEdge(checkVtxp, waitVtxp, flp);
                guardedLink(currentp, checkVtxp, flp);
                currentp = checkVtxp;
            } അല്ലെങ്കിൽ {
                SvaStateVertex* സ്ഥിരം loopBackVtxp = scopedCreateVertex();
                guardedEdge(currentp, loopBackVtxp, flp);
                SvaStateVertex* സ്ഥിരം reCheckVtxp = scopedCreateVertex();
                guardedLink(loopBackVtxp, reCheckVtxp, sampledRefOrClone(hoistVarp, exprp, flp),
                            flp);
                guardedEdge(reCheckVtxp, loopBackVtxp, flp);
                guardedLink(reCheckVtxp, currentp, flp);
            }
            currentp->m_isUnbounded = സത്യം;
            m_inUnboundedScope = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (repp->maxCountp()) {
            സ്ഥിരം unsigned maxN = getConstUInt(repp->maxCountp());
            SvaStateVertex* സ്ഥിരം mergeVtxp = scopedCreateVertex();
            guardedLink(currentp, mergeVtxp, flp);
            unsigned tailMinN = minN;
            SvaStateVertex* tailStartp = currentp;
            എങ്കിൽ (minN == 0) {
                // Build the first optional iteration explicitly. Feeding the empty endpoint
                // directly into a range ring would incorrectly keep that match alive.
                SvaStateVertex* സ്ഥിരം nextVtxp = scopedCreateVertex();
                guardedEdge(currentp, nextVtxp, flp);
                SvaStateVertex* സ്ഥിരം checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                എങ്കിൽ (m_isCoverSeq) consMidSources.തള്ളുക_back(checkVtxp);
                tailStartp = checkVtxp;
                tailMinN = 1;
            }
            എങ്കിൽ (maxN > tailMinN) {
                // Add tail-ring only if the tail is non-empty.
                SvaStateVertex* സ്ഥിരം nextVtxp
                    = addDelayChain(tailStartp, maxN - tailMinN + 1, flp, അസത്യം);
                nextVtxp->m_delayRingClearCondp
                    = പുതിയ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)};
                nextVtxp->m_replayAbortReject = സത്യം;
                SvaStateVertex* സ്ഥിരം checkVtxp = scopedCreateVertex();
                guardedLink(nextVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
                guardedLink(checkVtxp, mergeVtxp, flp);
                എങ്കിൽ (m_isCoverSeq) consMidSources.തള്ളുക_back(checkVtxp);
            }
            currentp = mergeVtxp;
        }
        // finalCond = nullptr (already checked via Links)
        BuildResult res;
        res.termVertexp = currentp;
        res.finalCondp = nullptr;
        res.midSources = std::move(consMidSources);
        // mergeVtxp is the OR of all the end-positions we already pushed to
        // midSources, so the main termVtxp -> matchVertex Link would duplicate.
        res.termIsMidMerge = m_isCoverSeq && !res.midSources.empty();
        തിരികെ res;
    }

    // always[lo:hi] / s_always[lo:hi] (IEEE 1800-2023 16.12.11).
    BuildResult buildPropAlways(AstPropAlways* nodep, SvaStateVertex* entryVtxp,
                                തർക്കം is_മുകൾ_Level_Step = അസത്യം) {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം propp = nodep->propp();
        സ്ഥിരം unsigned lo = getConstUInt(nodep->loBoundp());
        എങ്കിൽ (VN_IS(nodep->hiBoundp(), Unbounded)) {
            // Weak always [lo:$]: unbounded upper bound (IEEE 1800-2023 16.12.11).
            // p must hold at every clock tick at least lo cycles after the attempt
            // start; those ticks are not required to exist, so there is no
            // end-of-trace obligation (weak). The self-loop keeps the attempt live
            // every cycle; each observed cycle is a safety obligation, so a false p
            // rejects immediately.
            UASSERT_OBJ(!nodep->isStrong(), nodep, "Unbounded always must be weak (V3Width)");
            SvaStateVertex* സ്ഥിരം livep = addDelayChain(entryVtxp, lo, flp);
            livep->m_isUnbounded = സത്യം;
            guardedEdge(livep, livep, flp);  // stay active every subsequent cycle
            SvaStateVertex* സ്ഥിരം sinkp = m_graph.createStateVertex();
            sinkp->m_isRejectSink = സത്യം;
            SvaTransEdge* സ്ഥിരം rejEdgep
                = guardedLink(livep, sinkp, sampled(propp->clone_വൃക്ഷം_Pure(അസത്യം)), flp);
            എങ്കിൽ (is_മുകൾ_Level_Step) rejEdgep->m_rejectOnFail = സത്യം;
            തിരികെ {livep, nullptr, {}};
        }
        സ്ഥിരം unsigned hi = getConstUInt(nodep->hiBoundp());
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
        SvaStateVertex* currentp = addDelayChain(entryVtxp, lo, flp);
        SvaStateVertex* സ്ഥിരം checkp = scopedCreateVertex();
        SvaTransEdge* സ്ഥിരം linkp
            = guardedLink(currentp, checkp, sampled(propp->clone_വൃക്ഷം_Pure(അസത്യം)), flp);
        എങ്കിൽ (is_മുകൾ_Level_Step) linkp->m_rejectOnFail = സത്യം;
        currentp = checkp;
        m_temporal_Guard_അട്ടി.തള്ളുക_back(propp);
        currentp = addDelayChain(currentp, hi - lo, flp);
        എങ്കിൽ (nodep->isStrong() && currentp->m_delayRingSize) currentp->m_strongAlwaysRing = സത്യം;
        m_temporal_Guard_അട്ടി.എടുക്കുക_back();
        തിരികെ {currentp, propp, {}};
    }

    BuildResult buildGotoRep(AstSGotoRep* repp, SvaStateVertex* entryVtxp) {
        File_വരി* സ്ഥിരം flp = repp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = repp->exprp();
        സ്ഥിരം unsigned minN = getConstUInt(repp->countp());
        എങ്കിൽ (minN == 0) തിരികെ BuildResult::fail();
        സ്ഥിരം തർക്കം hasMax = repp->maxCountp() != nullptr;
        സ്ഥിരം unsigned maxN = hasMax ? getConstUInt(repp->maxCountp()) : minN;
        എങ്കിൽ (m_isCoverSeq) {
            // Several attempts may coalesce in the one-bit wait vertex before reaching the ring,
            // so a cover sequence action block could run too few times.
            warnEndpointUnsupported(flp, "a goto repetition");
            തിരികെ BuildResult::failWithError();
        }

        AstVar* സ്ഥിരം hoistVarp = tryHoistSampled(exprp, flp, 2);

        // The first guardedEdge is the ##1 before waiting for a match. In the wait state, false
        // takes the clocked self-loop, while true takes the zero-delay guardedLink on that tick.
        SvaStateVertex* സ്ഥിരം waitVtxp = scopedCreateVertex();
        guardedEdge(entryVtxp, waitVtxp, flp);
        guardedEdge(waitVtxp, waitVtxp,
                    പുതിയ AstLogNot{flp, sampledRefOrClone(hoistVarp, exprp, flp)}, flp);
        SvaStateVertex* currentp = scopedCreateVertex();
        guardedLink(waitVtxp, currentp, sampledRefOrClone(hoistVarp, exprp, flp), flp);

        എങ്കിൽ (minN > 1) {
            SvaStateVertex* സ്ഥിരം ringVtxp = addDelayChain(
                currentp, minN - 1, flp, സത്യം, nullptr, sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = സത്യം;
            SvaStateVertex* സ്ഥിരം checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            currentp = checkVtxp;
        }
        എങ്കിൽ (!hasMax) {
            currentp->m_isUnbounded = സത്യം;  // [->N] waits unboundedly
            m_inUnboundedScope = സത്യം;
            തിരികെ {currentp, nullptr, {}};
        }

        // [->M:N]: the range ring holds matches from M through N and advances
        // only on expr, preserving arbitrarily long gaps between occurrences.
        SvaStateVertex* സ്ഥിരം mergeVtxp = scopedCreateVertex();
        guardedLink(currentp, mergeVtxp, flp);  // accept at match_M
        എങ്കിൽ (maxN > minN) {
            SvaStateVertex* സ്ഥിരം ringVtxp
                = addDelayChain(currentp, maxN - minN + 1, flp, അസത്യം, nullptr,
                                sampledRefOrClone(hoistVarp, exprp, flp));
            ringVtxp->m_replayAbortReject = സത്യം;
            SvaStateVertex* സ്ഥിരം checkVtxp = scopedCreateVertex();
            guardedLink(ringVtxp, checkVtxp, sampledRefOrClone(hoistVarp, exprp, flp), flp);
            guardedLink(checkVtxp, mergeVtxp, flp);
        }
        mergeVtxp->m_isUnbounded = സത്യം;  // [->M:N] still has unbounded waits between matches
        m_inUnboundedScope = സത്യം;
        തിരികെ {mergeVtxp, nullptr, {}};
    }

    // True if the sequence has a zero-minimum consecutive repetition, which may match empty
    സ്ഥിതം തർക്കം hasZeroMinRep(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* seqp) {
        തിരികെ seqp->exists(
            [](സ്ഥിരം AstSConsRep* repp) { തിരികെ getConstUInt(repp->countp()) == 0; });
    }

    // True if a leaf's condition has no implication or multi-cycle sequence in it
    // (a leaf can be e.g. 'not (x |-> y)')
    സ്ഥിതം തർക്കം isPlainBoolean(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        തിരികെ !condp->exists(
            [](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* np) { തിരികെ np->isMultiCycleSva() || VN_IS(np, Implication); });
    }

    // Build merge vertex for SOr / LogOr: both branches feed into one vertex.
    BuildResult buildOrMerge(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, SvaStateVertex* entryVtxp,
                             File_വരി* flp, തർക്കം wantBoolLeaf) {
        സ്ഥിരം BuildResult lhs = build_വ്യഞ്ജകം(lhsp, entryVtxp);
        സ്ഥിരം BuildResult rhs = build_വ്യഞ്ജകം(rhsp, entryVtxp);
        എങ്കിൽ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            തിരികെ BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP
        // IEEE 1800-2023 16.14.3: a cover sequence counts every end-of-match. A
        // sequence operand of 'or' can end more than once, but only its final
        // end reaches the merge vertex below, so reject sequence operands rather
        // than under-count. Plain boolean disjunction has one end per cycle and
        // is handled by the OR-fold.
        സ്ഥിരം തർക്കം booleanOnly = lhs.termVertexp == entryVtxp && lhs.finalCondp
                                 && rhs.termVertexp == entryVtxp && rhs.finalCondp;
        എങ്കിൽ (m_isCoverSeq && !booleanOnly) {
            warnEndpointUnsupported(flp, "a sequence operand of 'or'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            തിരികെ BuildResult::failWithError();
        }
        എങ്കിൽ (booleanOnly && wantBoolLeaf) {
            എങ്കിൽ (isPlainBoolean(lhs.finalCondp) && isPlainBoolean(rhs.finalCondp)) {
                // As a |-> / |=> or top-level body, a disjunction of two plain booleans must
                // be able to reject: return it as a boolean finalCondp. The merge vertex
                // below drops the attempt silently when both operands are false.
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം orCondp
                    = പുതിയ AstLogOr{flp, lhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം),
                                   rhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)};
                freeUnlinkedCondp(lhs.finalCondp);
                freeUnlinkedCondp(rhs.finalCondp);
                തിരികെ {entryVtxp, orCondp, {}};
            }
        }
        SvaStateVertex* സ്ഥിരം mergeVtxp = scopedCreateVertex();
        എങ്കിൽ (booleanOnly) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsp,
                        "Single-cycle SOr operands must have finalCondp");
            guardedLink(entryVtxp, mergeVtxp,
                        പുതിയ AstLogOr{flp, sampled(lhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)),
                                     sampled(rhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം))},
                        flp);
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (lhs.finalCondp) {
                guardedLink(lhs.termVertexp, mergeVtxp,
                            sampled(lhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)), flp);
            } അല്ലെങ്കിൽ {
                guardedLink(lhs.termVertexp, mergeVtxp, flp);
            }
            എങ്കിൽ (rhs.finalCondp) {
                guardedLink(rhs.termVertexp, mergeVtxp,
                            sampled(rhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)), flp);
            } അല്ലെങ്കിൽ {
                guardedLink(rhs.termVertexp, mergeVtxp, flp);
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        തിരികെ {mergeVtxp, nullptr, {}};
    }

    // Build done-latch combiner for SAnd/SIntersect (IEEE 1800-2023 16.9.5).
    BuildResult buildAndCombiner(Ast_കണ്ണി_വ്യഞ്ജകം* lhsExprp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsExprp,
                                 SvaStateVertex* entryVtxp, File_വരി* flp) {
        // Snapshot-restore scope so LHS liveness does not leak into RHS.
        സ്ഥിരം തർക്കം savedScope = m_inUnboundedScope;
        സ്ഥിരം BuildResult lhs = build_വ്യഞ്ജകം(lhsExprp, entryVtxp);
        സ്ഥിരം തർക്കം lhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope;
        സ്ഥിരം BuildResult rhs = build_വ്യഞ്ജകം(rhsExprp, entryVtxp);
        സ്ഥിരം തർക്കം rhsScope = m_inUnboundedScope;
        m_inUnboundedScope = savedScope || lhsScope || rhsScope;
        എങ്കിൽ (!lhs.valid() || !rhs.valid()) {  // LCOV_EXCL_START -- sub-build fail bail
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            തിരികെ BuildResult::fail(lhs.errorEmitted || rhs.errorEmitted);
        }  // LCOV_EXCL_STOP

        // Single-cycle operands: use boolean AND (done-latch would fire across cycles).
        // If both operands stayed at entry, they must be boolean leaves which
        // buildExpr returns with finalCondp=nodep (non-null).
        എങ്കിൽ (lhs.termVertexp == entryVtxp && rhs.termVertexp == entryVtxp) {
            UASSERT_OBJ(lhs.finalCondp && rhs.finalCondp, lhsExprp,
                        "Single-cycle SAnd operands must have finalCondp");
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstLogAnd{flp, lhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം),
                                                     rhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)};
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            തിരികെ {entryVtxp, condp, {}};
        }
        // Range-delay mid-window sources in either sub-branch would need
        // to be folded into the latch's match-now signal, which the
        // current combiner does not support.
        എങ്കിൽ (!lhs.midSources.empty() || !rhs.midSources.empty()) {
            flp->v3warn(E_UNSUPPORTED,
                        "Unsupported: ranged cycle delay in an operand of property 'and'");
            freeUnlinkedCondp(lhs.finalCondp);
            freeUnlinkedCondp(rhs.finalCondp);
            തിരികെ BuildResult::failWithError();
        }
        SvaStateVertex* സ്ഥിരം combVtxp = scopedCreateVertex();
        combVtxp->m_isAndCombiner = സത്യം;
        combVtxp->m_andLhsTermp = lhs.termVertexp;
        combVtxp->m_andRhsTermp = rhs.termVertexp;
        എങ്കിൽ (lhs.finalCondp) combVtxp->m_andLhsCondp = lhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം);
        എങ്കിൽ (rhs.finalCondp) combVtxp->m_andRhsCondp = rhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം);
        എങ്കിൽ (lhs.termVertexp->m_isUnbounded || rhs.termVertexp->m_isUnbounded) {
            combVtxp->m_isUnbounded = സത്യം;
        }
        // Wire terminal-boolean rejects to a dedicated sink so each side can fail
        // the AND independently. Skip for liveness or single-cycle operands
        // (single-cycle termVertexp == entryVtxp would fire every cycle).
        എങ്കിൽ (!combVtxp->m_isUnbounded) {
            തർക്കം needSink = അസത്യം;
            സ്ഥിരം തർക്കം lhsMultiCycle = (lhs.termVertexp != entryVtxp);
            സ്ഥിരം തർക്കം rhsMultiCycle = (rhs.termVertexp != entryVtxp);
            എങ്കിൽ (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                needSink = സത്യം;
            }
            എങ്കിൽ (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                needSink = സത്യം;
            }
            എങ്കിൽ (needSink) {
                SvaStateVertex* സ്ഥിരം sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = സത്യം;
                എങ്കിൽ (lhs.finalCondp && lhsMultiCycle && !lhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* സ്ഥിരം ep = m_graph.addLink(
                        lhs.termVertexp, sinkVtxp, sampled(lhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)));
                    ep->m_rejectOnFail = സത്യം;
                }
                എങ്കിൽ (rhs.finalCondp && rhsMultiCycle && !rhs.termVertexp->m_isUnbounded) {
                    SvaTransEdge* സ്ഥിരം ep = m_graph.addLink(
                        rhs.termVertexp, sinkVtxp, sampled(rhs.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)));
                    ep->m_rejectOnFail = സത്യം;
                }
            }
        }
        freeUnlinkedCondp(lhs.finalCondp);
        freeUnlinkedCondp(rhs.finalCondp);
        തിരികെ {combVtxp, nullptr, {}};
    }

    // Lower `seq1 within seq2` (IEEE 1800-2023 16.9.10) as:
    //   (OR_{i in 0..slack} 1 ##i seq1 ##(slack-i) 1) intersect seq2
    // Both operands must have fixed length (no ranged cycle delays).
    // The OR lives inside a single SIntersect so one AndCombiner done-latch
    // serves all offsets; lifting it out would double-count matches where
    // multiple offsets accept on the same seq2 end cycle.
    BuildResult buildSWithin(AstSWithin* nodep, SvaStateVertex* entryVtxp,
                             തർക്കം is_മുകൾ_Level_Step = അസത്യം) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ innerLen = fixedLength(nodep->lhsp());
        സ്ഥിരം പൂർണ്ണസംഖ്യ outerLen = fixedLength(nodep->rhsp());
        എങ്കിൽ (innerLen < 0 || outerLen < 0) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: within with ranged cycle-delay operand");
            തിരികെ BuildResult::failWithError();
        }
        എങ്കിൽ (innerLen > outerLen) {
            തിരികെ buildNeverMatchIntersect(
                nodep, entryVtxp, is_മുകൾ_Level_Step,
                "the inner sequence is longer than the outer sequence");
        }
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        സ്ഥിരം പൂർണ്ണസംഖ്യ slack = outerLen - innerLen;
        Ast_കണ്ണി_വ്യഞ്ജകം* innerOrp = nullptr;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i <= slack; ++i) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ postPad = slack - i;
            Ast_കണ്ണി_വ്യഞ്ജകം* branchp = nodep->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം);
            എങ്കിൽ (i > 0) {
                AstConst* സ്ഥിരം prePadp = പുതിയ AstConst{flp, AstConst::BitTrue{}};
                AstDelay* സ്ഥിരം delayp
                    = പുതിയ AstDelay{flp, പുതിയ AstConst{flp, static_cast<uint32_t>(i)}, സത്യം};
                AstSExpr* സ്ഥിരം wrapped = പുതിയ AstSExpr{flp, prePadp, delayp, branchp};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            എങ്കിൽ (postPad > 0) {
                AstConst* സ്ഥിരം postTruep = പുതിയ AstConst{flp, AstConst::BitTrue{}};
                AstDelay* സ്ഥിരം delayp
                    = പുതിയ AstDelay{flp, പുതിയ AstConst{flp, static_cast<uint32_t>(postPad)}, സത്യം};
                AstSExpr* സ്ഥിരം wrapped = പുതിയ AstSExpr{flp, branchp, delayp, postTruep};
                wrapped->dtypeSetBit();
                branchp = wrapped;
            }
            innerOrp = innerOrp ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstSOr{flp, innerOrp, branchp})
                                : branchp;
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം outerClonep = nodep->rhsp()->clone_വൃക്ഷം_Pure(അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം combinedp = പുതിയ AstSIntersect{flp, innerOrp, outerClonep};
        BuildResult result = build_വ്യഞ്ജകം(combinedp, entryVtxp, is_മുകൾ_Level_Step);
        VL_DO_DANGLING(combinedp->delete_വൃക്ഷം(), combinedp);
        // When both operands are plain booleans, buildAndCombiner returns a
        // freshly-allocated AstAnd as finalCondp with no parent. Callers up
        // the chain clone-and-discard finalCondp, so leave it parent-less and
        // it leaks; anchor it in the graph now via an edge.
        എങ്കിൽ (result.valid() && result.finalCondp && !result.finalCondp->backp()) {
            SvaStateVertex* സ്ഥിരം wrapVtxp = scopedCreateVertex();
            guardedLink(result.termVertexp, wrapVtxp, sampled(result.finalCondp), flp);
            result = {wrapVtxp, nullptr, result.midSources};
        }
        തിരികെ result;
    }

    // Collect the boolean leaf checks of a fixed-length sequence keyed by their
    // clock offset from the start. Returns false for anything other than nested
    // AstSExpr with fixed cycle delays over boolean leaves (e.g. throughout).
    സ്ഥിതം തർക്കം flattenFixedSeq(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, പൂർണ്ണസംഖ്യ baseOffset,
                                std::map<പൂർണ്ണസംഖ്യ, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>>& out) {
        എങ്കിൽ (AstSExpr* സ്ഥിരം sexprp = VN_CAST(nodep, SExpr)) {
            AstDelay* സ്ഥിരം delayp = VN_CAST(sexprp->delayp(), Delay);
            എങ്കിൽ (!delayp || !delayp->isCycleDelay() || delayp->isUnbounded()) തിരികെ അസത്യം;
            സ്ഥിരം unsigned delayCycles = getConstUInt(delayp->lhsp());
            എങ്കിൽ (delayp->isRangeDelay() && getConstUInt(delayp->rhsp()) != delayCycles)
                തിരികെ അസത്യം;
            പൂർണ്ണസംഖ്യ preLen = 0;
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prep = sexprp->preExprp()) {
                എങ്കിൽ (!flattenFixedSeq(prep, baseOffset, out)) തിരികെ അസത്യം;
                preLen = fixedLength(prep);
                എങ്കിൽ (preLen < 0) തിരികെ അസത്യം;
            }
            തിരികെ flattenFixedSeq(sexprp->exprp(), baseOffset + preLen + delayCycles, out);
        }
        എങ്കിൽ (nodep->isMultiCycleSva()) തിരികെ അസത്യം;
        out[baseOffset].തള്ളുക_back(nodep);
        തിരികെ സത്യം;
    }

    // Conjoin two equal-length fixed sequences into one: at each clock offset
    // AND the boolean checks of both operands (IEEE 1800-2023 16.9.6 -- both
    // operands match the same window). Returns null if either operand is not a
    // plain fixed sequence of boolean leaves.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* conjoinFixedSeqs(Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp, File_വരി* flp) {
        std::map<പൂർണ്ണസംഖ്യ, std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>> checks;
        എങ്കിൽ (!flattenFixedSeq(lhsp, 0, checks) || !flattenFixedSeq(rhsp, 0, checks)) തിരികെ nullptr;
        എങ്കിൽ (checks.empty()) തിരികെ nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* resultp = nullptr;
        പൂർണ്ണസംഖ്യ prevOffset = 0;
        വേണ്ടി (സ്ഥിരം auto& offsetChecks : checks) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ offset = offsetChecks.first;
            Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം leafp : offsetChecks.second) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം clonep = leafp->clone_വൃക്ഷം_Pure(അസത്യം);
                എങ്കിൽ (!condp) {
                    condp = clonep;
                } അല്ലെങ്കിൽ {
                    condp = പുതിയ AstLogAnd{flp, condp, clonep};
                    condp->dtypeSetBit();
                }
            }
            എങ്കിൽ (!resultp) {
                എങ്കിൽ (offset > 0) {
                    AstDelay* സ്ഥിരം delayp = പുതിയ AstDelay{
                        flp, പുതിയ AstConst{flp, static_cast<uint32_t>(offset)}, /*isCycle=*/സത്യം};
                    resultp
                        = പുതിയ AstSExpr{flp, പുതിയ AstConst{flp, AstConst::BitTrue{}}, delayp, condp};
                    resultp->dtypeSetBit();
                } അല്ലെങ്കിൽ {
                    resultp = condp;
                }
            } അല്ലെങ്കിൽ {
                AstDelay* സ്ഥിരം delayp = പുതിയ AstDelay{
                    flp, പുതിയ AstConst{flp, static_cast<uint32_t>(offset - prevOffset)},
                    /*isCycle=*/സത്യം};
                resultp = പുതിയ AstSExpr{flp, resultp, delayp, condp};
                resultp->dtypeSetBit();
            }
            prevOffset = offset;
        }
        തിരികെ resultp;
    }

    // `seq` is a simple ranged sequence `start ##[m:n] end` (start/end boolean,
    // start may be absent). Used to collapse a both-variable intersect to one
    // ranged delay.
    ഘടന SimpleRanged final {
        തർക്കം ok = അസത്യം;
        Ast_കണ്ണി_വ്യഞ്ജകം* startp = nullptr;  // may be null (absent start)
        Ast_കണ്ണി_വ്യഞ്ജകം* endp = nullptr;
    };
    സ്ഥിതം SimpleRanged asSimpleRanged(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        AstSExpr* സ്ഥിരം sexprp = VN_CAST(nodep, SExpr);
        എങ്കിൽ (!sexprp) തിരികെ {};
        AstDelay* സ്ഥിരം delayp = VN_CAST(sexprp->delayp(), Delay);
        എങ്കിൽ (!delayp || !delayp->isCycleDelay() || !delayp->isRangeDelay() || delayp->isUnbounded())
            തിരികെ {};
        എങ്കിൽ (getConstUInt(delayp->lhsp()) == getConstUInt(delayp->rhsp())) തിരികെ {};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prep = sexprp->preExprp();
        എങ്കിൽ (prep && fixedLength(prep) != 0) തിരികെ {};
        എങ്കിൽ (fixedLength(sexprp->exprp()) != 0) തിരികെ {};
        തിരികെ {സത്യം, prep, sexprp->exprp()};
    }

    // Build the NFA for a synthesized intersect lowering tree, then free it.
    // buildExpr returns the terminal condition (finalCondp) by reference into the
    // tree; detach a clone so the tree can be freed here. The graph already holds
    // clones/hoists of every edge condition, so nothing else dangles.
    BuildResult build_From_Lowering_വൃക്ഷം(Ast_കണ്ണി_വ്യഞ്ജകം* treep, SvaStateVertex* entryVtxp,
                                      തർക്കം is_മുകൾ_Level_Step) {
        BuildResult result = build_വ്യഞ്ജകം(treep, entryVtxp, is_മുകൾ_Level_Step);
        എങ്കിൽ (result.valid() && result.finalCondp) {
            result.finalCondp = result.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം);
        }
        VL_DO_DANGLING(treep->delete_വൃക്ഷം(), treep);
        തിരികെ result;
    }

    // Empty common-length intersection -- unequal fixed lengths, or disjoint
    // ranged lengths. IEEE 1800-2023 16.9.6 requires both operands to match
    // over a window of the same length, so with no common length the intersect
    // simply never matches. This is legal (matching nothing), not an error, so
    // lower to a constant false rather than rejecting legal code.
    BuildResult buildNeverMatchIntersect(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, SvaStateVertex* entryVtxp,
                                         തർക്കം is_മുകൾ_Level_Step, സ്ഥിരം അക്ഷരം* reason) {
        nodep->v3warn(NEVERMATCH, "Sequence can never match because " << reason << ".");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം falsep = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        തിരികെ build_From_Lowering_വൃക്ഷം(falsep, entryVtxp, is_മുകൾ_Level_Step);
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
    BuildResult buildVarLenIntersect(AstSIntersect* nodep, SvaStateVertex* entryVtxp,
                                     തർക്കം is_മുകൾ_Level_Step) {
        സ്ഥിരം std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ> lhsRange = lengthRange(nodep->lhsp());
        സ്ഥിരം std::pair<പൂർണ്ണസംഖ്യ, പൂർണ്ണസംഖ്യ> rhsRange = lengthRange(nodep->rhsp());
        എങ്കിൽ (lhsRange.first < 0 || rhsRange.first < 0) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect with this variable-length operand");
            തിരികെ BuildResult::failWithError();
        }
        സ്ഥിരം പൂർണ്ണസംഖ്യ lo = std::max(lhsRange.first, rhsRange.first);
        സ്ഥിരം പൂർണ്ണസംഖ്യ hi = std::min(lhsRange.second, rhsRange.second);
        എങ്കിൽ (lo > hi) {
            // Disjoint length ranges share no common length -> never matches.
            തിരികെ buildNeverMatchIntersect(nodep, entryVtxp, is_മുകൾ_Level_Step,
                                            "intersect operands have no common length");
        }
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        എങ്കിൽ (lo == hi) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lp = realizeAtLength(nodep->lhsp(), lo, lhsRange.first);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rp = realizeAtLength(nodep->rhsp(), lo, rhsRange.first);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം conjp = conjoinFixedSeqs(lp, rp, flp);
            VL_DO_DANGLING(lp->delete_വൃക്ഷം(), lp);
            VL_DO_DANGLING(rp->delete_വൃക്ഷം(), rp);
            എങ്കിൽ (!conjp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: intersect operand is not a plain boolean sequence");
                തിരികെ BuildResult::failWithError();
            }
            തിരികെ build_From_Lowering_വൃക്ഷം(conjp, entryVtxp, is_മുകൾ_Level_Step);
        }
        സ്ഥിരം SimpleRanged sl = asSimpleRanged(nodep->lhsp());
        സ്ഥിരം SimpleRanged sr = asSimpleRanged(nodep->rhsp());
        എങ്കിൽ (!sl.ok || !sr.ok) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: intersect of two sequences that each vary in length over a"
                          " range with internal structure");
            തിരികെ BuildResult::failWithError();
        }
        സ്ഥിരം auto andBool = [&](Ast_കണ്ണി_വ്യഞ്ജകം* ap, Ast_കണ്ണി_വ്യഞ്ജകം* bp) -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം aClonep
                = ap ? ap->clone_വൃക്ഷം_Pure(അസത്യം) : പുതിയ AstConst{flp, AstConst::BitTrue{}};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bClonep
                = bp ? bp->clone_വൃക്ഷം_Pure(അസത്യം) : പുതിയ AstConst{flp, AstConst::BitTrue{}};
            AstLogAnd* സ്ഥിരം andp = പുതിയ AstLogAnd{flp, aClonep, bClonep};
            andp->dtypeSetBit();
            തിരികെ andp;
        };
        AstDelay* സ്ഥിരം delayp = പുതിയ AstDelay{flp, പുതിയ AstConst{flp, static_cast<uint32_t>(lo)},
                                              /*isCycle=*/സത്യം};
        delayp->rhsp(പുതിയ AstConst{flp, static_cast<uint32_t>(hi)});
        AstSExpr* സ്ഥിരം reducedp
            = പുതിയ AstSExpr{flp, andBool(sl.startp, sr.startp), delayp, andBool(sl.endp, sr.endp)};
        reducedp->dtypeSetBit();
        തിരികെ build_From_Lowering_വൃക്ഷം(reducedp, entryVtxp, is_മുകൾ_Level_Step);
    }

    BuildResult buildThroughout(AstSThroughout* nodep, SvaStateVertex* entryVtxp,
                                തർക്കം is_മുകൾ_Level_Step = അസത്യം) {
        // Mark entryVtxp so "cond false at tick 0" is detected as throughout-drop.
        entryVtxp->m_throughoutConds.തള്ളുക_back(nodep->lhsp()->clone_വൃക്ഷം_Pure(അസത്യം));
        m_temporal_Guard_അട്ടി.തള്ളുക_back(nodep->lhsp());
        സ്ഥിരം BuildResult result = build_വ്യഞ്ജകം(nodep->rhsp(), entryVtxp, is_മുകൾ_Level_Step);
        m_temporal_Guard_അട്ടി.എടുക്കുക_back();
        തിരികെ result;
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
    BuildResult buildUntil(AstUntil* nodep, SvaStateVertex* entryVtxp, തർക്കം is_മുകൾ_Level_Step) {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        എങ്കിൽ (!is_മുകൾ_Level_Step) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            തിരികെ BuildResult::failWithError();
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp();
        സ്ഥിരം auto hasSeq = [](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* ep) {
            തിരികെ ep->exists([](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* np) { തിരികെ np->isMultiCycleSva(); });
        };
        എങ്കിൽ (hasSeq(lhsp) || hasSeq(rhsp)) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: '" << nodep->verilogKwd()
                                                          << "' in complex property expression");
            തിരികെ BuildResult::failWithError();
        }

        സ്ഥിരം തർക്കം ov = nodep->isOverlapping();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsBitp = nodep->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsBitp = nodep->rhsp();
        // p hoist count: continue, require (ov: 1 use; nov: 1 use). At least 2 uses.
        AstVar* സ്ഥിരം pHoistp = tryHoistSampled(lhsBitp, flp, 2);
        // q hoist count: continue (1) + require nov (1) = 2; ov: continue only (1).
        AstVar* സ്ഥിരം qHoistp = ov ? nullptr : tryHoistSampled(rhsBitp, flp, 2);

        SvaStateVertex* സ്ഥിരം waitCp = scopedCreateVertex();
        SvaStateVertex* സ്ഥിരം waitRp = scopedCreateVertex();
        waitCp->m_isUnbounded = സത്യം;
        waitRp->m_strongPending = nodep->isStrong() && !m_isCover;

        // Entry and back-loop Links carry no condition; throughout-folding still applies.
        guardedLink(entryVtxp, waitCp, flp);
        guardedLink(waitRp, waitCp, flp);

        // Continue clocked edge: p && !q advances to next-cycle wait.
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം contCondp
            = പുതിയ AstLogAnd{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                            പുതിയ AstLogNot{flp, sampledRefOrClone(qHoistp, rhsBitp, flp)}};
        guardedEdge(waitCp, waitRp, contCondp, flp);

        // Reject sink: fires when require-condition is false.
        SvaStateVertex* സ്ഥിരം sinkVtxp = m_graph.createStateVertex();
        sinkVtxp->m_isRejectSink = സത്യം;
        Ast_കണ്ണി_വ്യഞ്ജകം* requireCondp;
        എങ്കിൽ (ov) {
            requireCondp = sampledRefOrClone(pHoistp, lhsBitp, flp);
        } അല്ലെങ്കിൽ {
            requireCondp = പുതിയ AstLogOr{flp, sampledRefOrClone(pHoistp, lhsBitp, flp),
                                        sampledRefOrClone(qHoistp, rhsBitp, flp)};
        }
        SvaTransEdge* സ്ഥിരം rejEdgep = m_graph.addLink(waitCp, sinkVtxp, requireCondp);
        rejEdgep->m_rejectOnFail = സത്യം;

        // Accept condition rides via finalCondp; assembleResult $sampled-wraps it.
        Ast_കണ്ണി_വ്യഞ്ജകം* acceptCondp;
        എങ്കിൽ (ov) {
            acceptCondp
                = പുതിയ AstLogAnd{flp, lhsBitp->clone_വൃക്ഷം_Pure(അസത്യം), rhsBitp->clone_വൃക്ഷം_Pure(അസത്യം)};
        } അല്ലെങ്കിൽ {
            acceptCondp = rhsBitp->clone_വൃക്ഷം_Pure(അസത്യം);
        }
        തിരികെ {waitCp, acceptCondp, {}};
    }

    // IEEE 1800-2023 16.12.14 property abort operators. Sync and async share
    // the same NFA encoding: AstSampled already gives matured values at every
    // maturing clocking event, and async firing "between clocks" is not
    // observable in a cycle-based model. VAbortKind selects accept vs reject
    // verdict (sync vs async only changes the user-visible spelling).

    // Build `condp && !outer_1 && !outer_2 ...` (unsampled).
    Ast_കണ്ണി_വ്യഞ്ജകം* abort_Fire_വ്യഞ്ജകം(Ast_കണ്ണി_വ്യഞ്ജകം* condp, File_വരി* flp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* resultp = condp->clone_വൃക്ഷം_Pure(അസത്യം);
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം op : m_outer_Abort_അട്ടി)
            resultp = പുതിയ AstLogAnd{flp, resultp, പുതിയ AstLogNot{flp, op->clone_വൃക്ഷം_Pure(അസത്യം)}};
        തിരികെ resultp;
    }

    // True when unguarded ring-wide rejection or a same-tick Link chain already accounts the
    // attempt: a required-step Link covers both outcomes; followed-by pairs both edges.
    സ്ഥിതം തർക്കം chainAccountsSource(സ്ഥിരം SvaStateVertex* srcp,
                                    സ്ഥിരം std::unordered_set<സ്ഥിരം V3GraphEdge*>& preEdges) {
        എങ്കിൽ (srcp->m_delayRingSize && srcp->m_throughoutConds.empty()) തിരികെ സത്യം;
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edger : srcp->inEdges()) {
            എങ്കിൽ (preEdges.എണ്ണം(&edger)) തുടരുക;
            സ്ഥിരം SvaTransEdge& tedger = static_cast<സ്ഥിരം SvaTransEdge&>(edger);
            എങ്കിൽ (tedger.m_consumesCycle) തുടരുക;
            സ്ഥിരം auto* സ്ഥിരം fromp = static_cast<സ്ഥിരം SvaStateVertex*>(tedger.fromVtxp());
            എങ്കിൽ (fromp->m_abortRejectp) തിരികെ സത്യം;
        }
        തർക്കം plainNonSink = അസത്യം;
        തർക്കം markedSink = അസത്യം;
        വേണ്ടി (സ്ഥിരം V3GraphEdge& edger : srcp->outEdges()) {
            എങ്കിൽ (preEdges.എണ്ണം(&edger)) തുടരുക;
            സ്ഥിരം SvaTransEdge& tedger = static_cast<സ്ഥിരം SvaTransEdge&>(edger);
            എങ്കിൽ (tedger.m_consumesCycle) തുടരുക;
            സ്ഥിരം തർക്കം sink = static_cast<സ്ഥിരം SvaStateVertex*>(tedger.toVtxp())->m_isRejectSink;
            എങ്കിൽ (tedger.m_rejectOnFail) {
                എങ്കിൽ (!sink) തിരികെ സത്യം;
                markedSink = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (!sink) {
                എങ്കിൽ (srcp->m_abortRejectp) തിരികെ സത്യം;
                plainNonSink = സത്യം;
            }
        }
        തിരികെ plainNonSink && markedSink;
    }

    // Reject edge: fires when the source is live and the abort samples true.
    ശൂന്യം addAbortRejectEdge(SvaStateVertex* srcp, SvaStateVertex* sinkp, Ast_കണ്ണി_വ്യഞ്ജകം* condp,
                            File_വരി* flp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notFirep = പുതിയ AstLogNot{flp, sampled(abort_Fire_വ്യഞ്ജകം(condp, flp))};
        m_graph.addLink(srcp, sinkp, notFirep)->m_rejectOnFail = സത്യം;
        തിരികെ;
    }

    // On the fire tick: kill body threads; accept kinds also forgive step misses.
    ശൂന്യം gateBodyEdgesOnAbort(സ്ഥിരം std::unordered_set<സ്ഥിരം V3GraphEdge*>& preEdges,
                              Ast_കണ്ണി_വ്യഞ്ജകം* condp, VAbortKind kind, File_വരി* flp) {
        വേണ്ടി (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            വേണ്ടി (V3GraphEdge& edger : vtxr.outEdges()) {
                എങ്കിൽ (preEdges.എണ്ണം(&edger)) തുടരുക;
                SvaTransEdge* സ്ഥിരം tedgep = static_cast<SvaTransEdge*>(&edger);
                എങ്കിൽ (tedgep->m_rejectOnFail) {
                    എങ്കിൽ (!kind.isAccept()) തുടരുക;
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം firep = sampled(abort_Fire_വ്യഞ്ജകം(condp, flp));
                    tedgep->m_condp
                        = tedgep->m_condp ? പുതിയ AstLogOr{flp, tedgep->m_condp, firep} : firep;
                } അല്ലെങ്കിൽ എങ്കിൽ (tedgep->m_consumesCycle) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notFirep
                        = പുതിയ AstLogNot{flp, sampled(abort_Fire_വ്യഞ്ജകം(condp, flp))};
                    tedgep->m_condp = tedgep->m_condp
                                          ? പുതിയ AstLogAnd{flp, tedgep->m_condp, notFirep}
                                          : notFirep;
                }
            }
        }
    }

    BuildResult buildAbortOn(Ast_കണ്ണി_വ്യഞ്ജകം* condp, Ast_കണ്ണി_വ്യഞ്ജകം* bodyp, SvaStateVertex* entryVtxp,
                             VAbortKind kind, File_വരി* flp, തർക്കം is_മുകൾ_Level_Step) {
        // Snapshot pre-body vertices/edges so post-build diff yields the body's sub-NFA.
        std::unordered_set<സ്ഥിരം V3GraphVertex*> preExisting;
        std::unordered_set<സ്ഥിരം V3GraphEdge*> preEdges;
        വേണ്ടി (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            preExisting.തിരുകുക(&vtxr);
            വേണ്ടി (V3GraphEdge& edger : vtxr.outEdges()) preEdges.തിരുകുക(&edger);
        }

        m_outer_Abort_അട്ടി.തള്ളുക_back(condp);
        സ്ഥിരം BuildResult bodyResult = build_വ്യഞ്ജകം(bodyp, entryVtxp, is_മുകൾ_Level_Step);
        m_outer_Abort_അട്ടി.എടുക്കുക_back();
        എങ്കിൽ (!bodyResult.valid()) തിരികെ bodyResult;

        gateBodyEdgesOnAbort(preEdges, condp, kind, flp);

        // Live-thread sources for the abort edge: entry + new body vertices,
        // minus reject sinks (they carry reject fuel, not live-thread fuel).
        std::vector<SvaStateVertex*> abortSources;
        abortSources.തള്ളുക_back(entryVtxp);
        വേണ്ടി (V3GraphVertex& vtxr : m_graph.m_graph.vertices()) {
            എങ്കിൽ (preExisting.എണ്ണം(&vtxr)) തുടരുക;
            auto* സ്ഥിരം sp = static_cast<SvaStateVertex*>(&vtxr);
            എങ്കിൽ (sp->m_delayRingSize || !sp->m_throughoutConds.empty()) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം firep = abort_Fire_വ്യഞ്ജകം(condp, flp);
                sp->m_abortClearp
                    = sp->m_abortClearp ? പുതിയ AstLogOr{flp, sp->m_abortClearp, firep} : firep;
            }
            എങ്കിൽ (!kind.isAccept()
                && ((sp->m_delayRingSize && sp->m_throughoutConds.empty())
                    || sp->m_replayAbortReject)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം firep = abort_Fire_വ്യഞ്ജകം(condp, flp);
                sp->m_abortRejectp
                    = sp->m_abortRejectp ? പുതിയ AstLogOr{flp, sp->m_abortRejectp, firep} : firep;
            }
            എങ്കിൽ (sp->m_isRejectSink) തുടരുക;
            abortSources.തള്ളുക_back(sp);
        }

        auto sampledAbortFire = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം വ്യഞ്ജകം = abort_Fire_വ്യഞ്ജകം(condp, flp);
            തിരികെ sampled(വ്യഞ്ജകം);
        };

        എങ്കിൽ (kind.isAccept()) {
            // Match-only sink fed by $sampled(abort-fire) from every live source;
            // registered as midSource so it never contributes a reject.
            SvaStateVertex* സ്ഥിരം acceptSinkp = scopedCreateVertex();
            വേണ്ടി (SvaStateVertex* സ്ഥിരം srcp : abortSources)
                guardedLink(srcp, acceptSinkp, sampledAbortFire(), flp);
            std::vector<SvaStateVertex*> midSources = bodyResult.midSources;
            midSources.തള്ളുക_back(acceptSinkp);
            Ast_കണ്ണി_വ്യഞ്ജകം* finalCondp = bodyResult.finalCondp;
            എങ്കിൽ (finalCondp) {
                എങ്കിൽ (finalCondp->backp()) finalCondp = finalCondp->clone_വൃക്ഷം_Pure(അസത്യം);
                finalCondp = പുതിയ AstLogOr{flp, finalCondp, abort_Fire_വ്യഞ്ജകം(condp, flp)};
            }
            തിരികെ {bodyResult.termVertexp, finalCondp, std::move(midSources)};
        }

        // rejectOnFail treats m_condp as the success condition and fires on
        // !condp, so the edge carries !sampledAbortFire().
        SvaStateVertex* സ്ഥിരം rejectSinkp = m_graph.createStateVertex();
        rejectSinkp->m_isRejectSink = സത്യം;
        വേണ്ടി (SvaStateVertex* സ്ഥിരം srcp : abortSources)
            എങ്കിൽ (!chainAccountsSource(srcp, preEdges))
                addAbortRejectEdge(srcp, rejectSinkp, condp, flp);
        Ast_കണ്ണി_വ്യഞ്ജകം* finalCondp = bodyResult.finalCondp;
        എങ്കിൽ (finalCondp) {
            എങ്കിൽ (finalCondp->backp()) finalCondp = finalCondp->clone_വൃക്ഷം_Pure(അസത്യം);
            finalCondp
                = പുതിയ AstLogAnd{flp, finalCondp, പുതിയ AstLogNot{flp, abort_Fire_വ്യഞ്ജകം(condp, flp)}};
        }
        തിരികെ {bodyResult.termVertexp, finalCondp, bodyResult.midSources};
    }

പൊതു:
    // Free a dropped sub-result condition that is not linked into the AST
    // (abort folds synthesize unparented finalCondp trees).
    സ്ഥിതം ശൂന്യം freeUnlinkedCondp(Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        എങ്കിൽ (condp && !condp->backp()) VL_DO_DANGLING(condp->delete_വൃക്ഷം(), condp);
    }

    SvaNfaBuilder(SvaGraph& graph, Ast_കണ്ണി_Module* modp, V3UniqueNames& propTempNames,
                  തർക്കം isCoverSeq = അസത്യം, തർക്കം isSeqEvent = അസത്യം, തർക്കം isCover = അസത്യം)
        : m_graph{graph}
        , m_modp{modp}
        , m_propTempNames{propTempNames}
        , m_isCover{isCover}
        , m_isCoverSeq{isCoverSeq}
        , m_isSeqEvent{isSeqEvent} {}

    // Reset scope between antecedent and consequent: liveness must not leak.
    // m_outerAbortStack survives: an abort wrapping the implication covers the
    // consequent too (IEEE 1800-2023 16.12.14).
    ശൂന്യം resetScope() {
        m_inUnboundedScope = അസത്യം;
        m_temporal_Guard_അട്ടി.clear();
    }

    BuildResult build_വ്യഞ്ജകം(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, SvaStateVertex* entryVtxp,
                          തർക്കം is_മുകൾ_Level_Step = അസത്യം) {
        VL_RESTORER(m_wantBoolLeaf);
        സ്ഥിരം തർക്കം wantBoolLeaf = m_wantBoolLeaf;
        m_wantBoolLeaf = അസത്യം;
        എങ്കിൽ (AstSExpr* സ്ഥിരം sexprp = VN_CAST(nodep, SExpr)) {
            തിരികെ buildSExpr(sexprp, entryVtxp, is_മുകൾ_Level_Step);
        }
        എങ്കിൽ (AstSConsRep* സ്ഥിരം repp = VN_CAST(nodep, SConsRep)) {
            തിരികെ buildConsRep(repp, entryVtxp, is_മുകൾ_Level_Step);
        }
        എങ്കിൽ (AstPropAlways* സ്ഥിരം alwaysp = VN_CAST(nodep, PropAlways)) {
            തിരികെ buildPropAlways(alwaysp, entryVtxp, is_മുകൾ_Level_Step);
        }
        എങ്കിൽ (AstSGotoRep* സ്ഥിരം repp = VN_CAST(nodep, SGotoRep)) {
            തിരികെ buildGotoRep(repp, entryVtxp);
        }
        എങ്കിൽ (AstSThroughout* സ്ഥിരം throughoutp = VN_CAST(nodep, SThroughout)) {
            തിരികെ buildThroughout(throughoutp, entryVtxp, is_മുകൾ_Level_Step);
        }
        എങ്കിൽ (AstSOr* സ്ഥിരം orp = VN_CAST(nodep, SOr)) {
            തിരികെ buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        എങ്കിൽ (AstLogOr* സ്ഥിരം orp = VN_CAST(nodep, LogOr)) {
            തിരികെ buildOrMerge(orp->lhsp(), orp->rhsp(), entryVtxp, orp->fileline(),
                                wantBoolLeaf);
        }
        എങ്കിൽ (AstSAnd* സ്ഥിരം andp = VN_CAST(nodep, SAnd)) {
            തിരികെ buildAndCombiner(andp->lhsp(), andp->rhsp(), entryVtxp, andp->fileline());
        }
        എങ്കിൽ (AstSIntersect* സ്ഥിരം intp = VN_CAST(nodep, SIntersect)) {
            // IEEE 1800-2023 16.9.6: both operands match over one window with
            // equal start and end (equal length). Lower to a single sequence
            // that conjoins both operands' per-cycle checks -- correct under
            // concurrent attempts, where the done-latch combiner conflates the
            // two operands' start times and over-accepts. The combiner remains
            // only as a fallback for operands that do not flatten.
            സ്ഥിരം പൂർണ്ണസംഖ്യ lhsLen = fixedLength(intp->lhsp());
            സ്ഥിരം പൂർണ്ണസംഖ്യ rhsLen = fixedLength(intp->rhsp());
            എങ്കിൽ (lhsLen >= 0 && rhsLen >= 0) {
                എങ്കിൽ (lhsLen != rhsLen) {
                    // Unequal fixed lengths share no common length -> never matches.
                    തിരികെ buildNeverMatchIntersect(intp, entryVtxp, is_മുകൾ_Level_Step,
                                                    "intersect operands have no common length");
                }
                എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം conjp
                    = conjoinFixedSeqs(intp->lhsp(), intp->rhsp(), intp->fileline())) {
                    തിരികെ build_From_Lowering_വൃക്ഷം(conjp, entryVtxp, is_മുകൾ_Level_Step);
                }
                തിരികെ buildAndCombiner(intp->lhsp(), intp->rhsp(), entryVtxp, intp->fileline());
            }
            തിരികെ buildVarLenIntersect(intp, entryVtxp, is_മുകൾ_Level_Step);
        }
        എങ്കിൽ (AstSWithin* സ്ഥിരം withinp = VN_CAST(nodep, SWithin)) {
            തിരികെ buildSWithin(withinp, entryVtxp, is_മുകൾ_Level_Step);
        }
        എങ്കിൽ (AstAbortOn* സ്ഥിരം ap = VN_CAST(nodep, AbortOn)) {
            തിരികെ buildAbortOn(ap->condp(), ap->propp(), entryVtxp, ap->kind(), ap->fileline(),
                                is_മുകൾ_Level_Step);
        }
        എങ്കിൽ (VN_IS(nodep, SNonConsRep)) തിരികെ BuildResult::fail();
        എങ്കിൽ (AstImplication* സ്ഥിരം implp = VN_CAST(nodep, Implication)) {
            തിരികെ buildImplicationEdges(implp->lhsp(), implp->rhsp(), entryVtxp,
                                         implp->isOverlapped(), implp->isFollowedBy(),
                                         implp->lhsp(), implp->fileline());
        }
        എങ്കിൽ (AstUntil* സ്ഥിരം untilp = VN_CAST(nodep, Until)) {
            തിരികെ buildUntil(untilp, entryVtxp, is_മുകൾ_Level_Step);
        }
        // Boolean leaf (including LogAnd): return as finalCond
        തിരികെ {entryVtxp, nodep, {}};
    }

    // Wire an implication / followed-by from `entryVtxp`: builds the antecedent,
    // emits the match-link (and for followed-by the reject-sink edge), inserts a
    // delay vertex for non-overlapped forms, and builds the body. Used both for
    // nested AstImplication in pexpr position and for the top-level assertion
    // antecedent -- `errorNodep` anchors the "unsupported sequence antecedent"
    // error, which differs between the two call sites.
    BuildResult buildImplicationEdges(Ast_കണ്ണി_വ്യഞ്ജകം* antExprp, Ast_കണ്ണി_വ്യഞ്ജകം* bodyExprp,
                                      SvaStateVertex* entryVtxp, തർക്കം isOverlapped,
                                      തർക്കം isFollowedBy, Ast_കണ്ണി* errorNodep, File_വരി* flp) {
        സ്ഥിരം BuildResult antResult = build_വ്യഞ്ജകം(antExprp, entryVtxp);
        എങ്കിൽ (!antResult.valid()) തിരികെ antResult;

        // Followed-by requires pure-boolean antecedent for non-vacuous-fail at
        // the attempt-start cycle. IEEE 1800-2023 16.12.9 permits a multi-cycle
        // sequence LHS, so this is an implementation gap rather than illegal SV.
        എങ്കിൽ (isFollowedBy && antResult.termVertexp != entryVtxp) {
            errorNodep->v3warn(E_UNSUPPORTED,
                               "Unsupported: sequence expression as antecedent of followed-by"
                               " (#-# / #=#) (IEEE 1800-2023 16.12.9)");
            തിരികെ BuildResult::failWithError();
        }
        UASSERT_OBJ(!isFollowedBy || antResult.finalCondp, errorNodep,
                    "followed-by antecedent terminal at entry must carry finalCondp");

        // Use raw createStateVertex() so trigVtxp starts without liveness --
        // reaching the antecedent terminal is a definitive event.
        SvaStateVertex* സ്ഥിരം trigVtxp = m_graph.createStateVertex();
        എങ്കിൽ (antResult.finalCondp) {
            m_graph.addLink(antResult.termVertexp, trigVtxp,
                            sampled(antResult.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)));
            // Followed-by non-vacuous fail: rejectOnFail fires when the attempt
            // is live (termVtx reachable) and sampled(antecedent) is false.
            എങ്കിൽ (isFollowedBy) {
                SvaStateVertex* സ്ഥിരം sinkVtxp = m_graph.createStateVertex();
                sinkVtxp->m_isRejectSink = സത്യം;
                SvaTransEdge* സ്ഥിരം ep
                    = m_graph.addLink(antResult.termVertexp, sinkVtxp,
                                      sampled(antResult.finalCondp->clone_വൃക്ഷം_Pure(അസത്യം)));
                ep->m_rejectOnFail = സത്യം;
            }
            // finalCondp is cloned into the Sampled nodes; if the original is
            // not parented anywhere in the AST anymore it must be freed here
            // or ASan flags it as a leak (e.g. t_sequence_bool_ops).
            എങ്കിൽ (!antResult.finalCondp->backp()) {
                VL_DO_DANGLING(antResult.finalCondp->delete_വൃക്ഷം(), antResult.finalCondp);
            }
        } അല്ലെങ്കിൽ {
            m_graph.addLink(antResult.termVertexp, trigVtxp);
        }
        resetScope();

        SvaStateVertex* bodyEntryp = trigVtxp;
        എങ്കിൽ (!isOverlapped) {
            SvaStateVertex* സ്ഥിരം delayVtxp = m_graph.createStateVertex();
            m_graph.addClockedEdge(trigVtxp, delayVtxp);
            bodyEntryp = delayVtxp;
        }
        // An empty antecedent match must not start the consequent, but a zero-minimum
        // repetition's empty match is not handled here; keep the merge vertex for those
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = !isFollowedBy && !hasZeroMinRep(antExprp);
        തിരികെ build_വ്യഞ്ജകം(bodyExprp, bodyEntryp, /*isTopLevelStep=*/സത്യം);
    }

    BuildResult build(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        m_graph.m_startVertexp = scopedCreateVertex();
        VL_RESTORER(m_wantBoolLeaf);
        m_wantBoolLeaf = സത്യം;
        തിരികെ build_വ്യഞ്ജകം(exprp, m_graph.m_startVertexp, /*isTopLevelStep=*/സത്യം);
    }
};

//######################################################################
// NFA Lowering (converts NFA graph to synthesizable AstAlways blocks)

വർഗ്ഗം SvaNfaLowering final {
    Ast_കണ്ണി_Module* സ്ഥിരം m_modp;  // Module to add state vars and always blocks to
    Ast_കണ്ണി_DType* സ്ഥിരം m_u32DTypep;  // Shared unsigned counter dtype
    V3UniqueNames m_names{"__Vnfa"};
    size_t m_statDelayRingEdgeVisits = 0;  // Delay-ring incoming edges visited

    // Per-lowering shared context (passed to phase sub-functions)
    // Per-vertex lowering state is stored in SvaVertexData and accessed via
    // V3GraphVertex::userp() (see SvaStateVertex::datap()).
    ഘടന LowerCtx final {
        File_വരി* flp;  // Source location for generated AST
        Ast_Sen_വൃക്ഷം* senTreep;  // Clock sensitivity tree
        Ast_കണ്ണി_വ്യഞ്ജകം* disableExprp;  // disable iff expression (may be nullptr)
        Ast_കണ്ണി_വ്യഞ്ജകം* matchCondp;  // Final boolean match condition (may be nullptr)
        VAssertType assertType;  // Assertion type for control tasks
        VAssertDirectiveType directiveType;  // Directive type for control tasks
        AstVar* killVarp;  // Last observed kill generation
        SvaGraph& graph;  // NFA graph
    };

    // Build a match-now expression: stateSig[i] && $sampled(condp)
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* buildMatchNow(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* stateExprp, Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം statep = stateExprp->clone_വൃക്ഷം_Pure(അസത്യം);
        എങ്കിൽ (!condp) തിരികെ statep;
        തിരികെ പുതിയ AstLogAnd{flp, statep, sampled(condp->clone_വൃക്ഷം_Pure(അസത്യം))};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* andCond(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        എങ്കിൽ (!condp) തിരികെ exprp;
        തിരികെ പുതിയ AstLogAnd{flp, exprp, condp->clone_വൃക്ഷം_Pure(അസത്യം)};
    }
    // bp is always non-null; only ap can be null (serving as accumulator).
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* orExprs(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* ap, Ast_കണ്ണി_വ്യഞ്ജകം* bp) {
        എങ്കിൽ (!ap) തിരികെ bp;
        തിരികെ പുതിയ AstLogOr{flp, ap, bp};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* addCountp(File_വരി* സ്ഥിരം flp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം totalCountp,
                                  Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം contributionp,
                                  Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enablep = nullptr) {
        // contribution = enable ? contribution : 0;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enabledContributionp
            = enablep ? പുതിയ AstCond{flp, enablep, contributionp,
                                    newTypedConstp(flp, contributionp->dtypep(), 0)}
                      : contributionp;
        എങ്കിൽ (!totalCountp) തിരികെ enabledContributionp;
        തിരികെ പുതിയ AstAdd{flp, totalCountp, enabledContributionp};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* gateCheckedCountp(File_വരി* സ്ഥിരം flp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം countp,
                                          Ast_കണ്ണി_വ്യഞ്ജകം*& overflowp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enablep) {
        എങ്കിൽ (overflowp) {
            overflowp = പുതിയ AstLogAnd{flp, enablep->clone_വൃക്ഷം_Pure(അസത്യം), overflowp};
        }
        തിരികെ addCountp(flp, nullptr, countp, enablep);
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* addCheckedCountp(File_വരി* സ്ഥിരം flp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം totalCountp,
                                         Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം contributionp, Ast_കണ്ണി_വ്യഞ്ജകം*& overflowp,
                                         Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം contributionOverflowp = nullptr) {
        എങ്കിൽ (contributionOverflowp) { overflowp = orExprs(flp, overflowp, contributionOverflowp); }
        എങ്കിൽ (!totalCountp) തിരികെ contributionp;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം oldTotalp = totalCountp->clone_വൃക്ഷം_Pure(അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultp = പുതിയ AstAdd{flp, totalCountp, contributionp};
        overflowp
            = orExprs(flp, overflowp, പുതിയ AstLt{flp, resultp->clone_വൃക്ഷം_Pure(അസത്യം), oldTotalp});
        തിരികെ resultp;
    }
    AstIf* new_എണ്ണം_Overflow_Warning(File_വരി* സ്ഥിരം flp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp) സ്ഥിരം {
        AstDisplay* സ്ഥിരം dispp = പുതിയ AstDisplay{
            flp, VDisplayType::DT_WARNING,
            "Cover sequence match count overflowed 32-bit storage; results may be incorrect.",
            nullptr, nullptr};
        dispp->fmtp()->timeunit(m_modp->timeunit());
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, condp, dispp};
        ifp->branchPred(VBranchPred::BP_UNLIKELY);
        തിരികെ ifp;
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* killActive(LowerCtx& c) {
        തിരികെ പുതിയ AstNeq{c.flp, പുതിയ AstVarRef{c.flp, c.killVarp, VAccess::READ},
                          assertKillGet(c.flp, c.assertType, c.directiveType)};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* notKillActive(LowerCtx& c) { തിരികെ പുതിയ AstLogNot{c.flp, killActive(c)}; }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* gateNotKill(LowerCtx& c, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        എങ്കിൽ (!exprp) തിരികെ nullptr;
        തിരികെ പുതിയ AstLogAnd{c.flp, exprp, notKillActive(c)};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* nextRingIndex(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* idxExprp, uint32_t size) {
        സ്ഥിരം auto u32Const = [flp](uint32_t മൂല്യം) {
            തിരികെ പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, മൂല്യം};
        };
        UASSERT_OBJ(size > 0, idxExprp, "Ring size must be positive");
        എങ്കിൽ (size == 1) {
            VL_DO_DANGLING(idxExprp->delete_വൃക്ഷം(), idxExprp);
            തിരികെ u32Const(0);
        }
        // idx == size - 1 ? 0 : idx + 1
        AstAdd* സ്ഥിരം addp = പുതിയ AstAdd{flp, idxExprp->clone_വൃക്ഷം_Pure(അസത്യം), u32Const(1)};
        addp->dtypeFrom(idxExprp);
        AstCond* സ്ഥിരം condp
            = പുതിയ AstCond{flp, പുതിയ AstEq{flp, idxExprp, u32Const(size - 1)}, u32Const(0), addp};
        condp->dtypeFrom(idxExprp);
        തിരികെ condp;
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* delayRingSlot(File_വരി* flp, SvaStateVertex* vtxp, Ast_കണ്ണി_വ്യഞ്ജകം* idxExprp,
                                      VAccess access = VAccess::READ) {
        // ring[idx]
        AstVar* സ്ഥിരം ringp = vtxp->datap()->delayRingVarp;
        AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{flp, ringp, access};
        എങ്കിൽ (vtxp->datap()->delayRingCounts) തിരികെ പുതിയ AstArraySel{flp, refp, idxExprp};
        തിരികെ പുതിയ AstSel{flp, refp, idxExprp, 1};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* delay_Ring_Head_എണ്ണം(File_വരി* flp, SvaStateVertex* vtxp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം headp
            = പുതിയ AstVarRef{flp, vtxp->datap()->delayRingHeadVarp, VAccess::READ};
        എങ്കിൽ (vtxp->datap()->delayRingCounts) തിരികെ headp;
        തിരികെ പുതിയ AstExtend{flp, headp, vtxp->datap()->delay_Ring_Live_എണ്ണം_Varp->dtypep()->width()};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* delayRingAtLastIndex(File_വരി* സ്ഥിരം flp, AstVar* സ്ഥിരം idxp,
                                             സ്ഥിരം uint32_t size) {
        തിരികെ പുതിയ AstEq{flp, പുതിയ AstVarRef{flp, idxp, VAccess::READ},
                         പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, 32, size - 1}};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* delay_Ring_Output_എണ്ണം(File_വരി* സ്ഥിരം flp, SvaStateVertex* സ്ഥിരം vtxp) {
        AstVar* സ്ഥിരം idxp = vtxp->datap()->delayRingIdxVarp;
        സ്ഥിരം uint32_t size = vtxp->m_delayRingSize;
        Ast_കണ്ണി_വ്യഞ്ജകം* outgoingValidp
            = പുതിയ AstVarRef{flp, vtxp->datap()->delayRingWrappedVarp, VAccess::READ};
        എങ്കിൽ (!vtxp->m_isFixedDelayRing) {
            outgoingValidp
                = പുതിയ AstLogOr{flp, outgoingValidp, delayRingAtLastIndex(flp, idxp, size)};
        }
        AstVar* സ്ഥിരം live_എണ്ണം_Varp = vtxp->datap()->delay_Ring_Live_എണ്ണം_Varp;
        തിരികെ പുതിയ AstCond{flp, outgoingValidp, delay_Ring_Head_എണ്ണം(flp, vtxp),
                           newTypedConstp(flp, live_എണ്ണം_Varp->dtypep(), 0)};
    }
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* delayRingHasLiveThreadsp(File_വരി* സ്ഥിരം flp,
                                                 AstVar* സ്ഥിരം live_എണ്ണം_Varp) {
        // active = live_count != 0;
        തിരികെ പുതിയ AstNeq{flp, പുതിയ AstVarRef{flp, live_എണ്ണം_Varp, VAccess::READ},
                          newTypedConstp(flp, live_എണ്ണം_Varp->dtypep(), 0)};
    }

    // Phase 3 output signals
    ഘടന SignalSet final {
        Ast_കണ്ണി_വ്യഞ്ജകം* terminalActivep = nullptr;  // OR of all successful terminal matches
        Ast_കണ്ണി_വ്യഞ്ജകം* matchCountp = nullptr;  // NFA paths completing the sequence this tick
        Ast_കണ്ണി_വ്യഞ്ജകം* match_എണ്ണം_Overflowp = nullptr;  // Match count overflow this tick
        Ast_കണ്ണി_വ്യഞ്ജകം* rejectBasep = nullptr;  // Reject when a terminal match fails
        Ast_കണ്ണി_വ്യഞ്ജകം* requiredStepRejectp = nullptr;  // Per-source reject from rejectOnFail Links
        Ast_കണ്ണി_വ്യഞ്ജകം* throughoutRejectp = nullptr;  // Reject when a throughout guard drops
        Ast_കണ്ണി_വ്യഞ്ജകം* threadFailCountp = nullptr;  // Number of threads rejected on this tick
    };

    // Disable condition also aborts attempts between clock events. A constant disable
    // never rises, and $sampled is unsupported in a sensitivity list.
    സ്ഥിതം തർക്കം isAsyncDisable(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം disableExprp) {
        തിരികെ disableExprp && !VN_IS(disableExprp, Const)
               && !disableExprp->exists([](സ്ഥിരം AstSampled*) { തിരികെ സത്യം; });
    }

    // Emit the clocked update block of registers that hold pending attempts. The clearp
    // statements reset the same registers: a disable rising between clock events also
    // aborts pending attempts (IEEE 1800-2023 16.12), so clearp runs on that rising edge
    // in a separate block. It must clear unconditionally, as the disable may already be
    // false again when it runs, and the update block must not run on that edge, as it
    // would advance the attempts as if the clock had ticked. Blocks that hold no attempt
    // state, such as the $assertkill acknowledge in emitKillAckNba, do not go through here.
    // Callers pass a null clearp when the disable cannot rise between clock events.
    ശൂന്യം addStateAlways(LowerCtx& c, Ast_കണ്ണി* bodyp, Ast_കണ്ണി* clearp) {
        m_modp->addStmtsp(
            പുതിയ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_വൃക്ഷം(അസത്യം), bodyp});
        എങ്കിൽ (!clearp) തിരികെ;
        m_modp->addStmtsp(പുതിയ AstAlways{
            c.flp, VAlwaysKwd::ALWAYS,
            പുതിയ Ast_Sen_വൃക്ഷം{c.flp, പുതിയ AstSenItem{c.flp, VEdgeType::ET_POSEDGE,
                                                 c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)}},
            clearp});
    }

    // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
    // Phase 2: State register NBA always block. Each clocked-edge target
    // latches the OR of its incoming contributions.
    ശൂന്യം emitStateRegisterNba(LowerCtx& c) {
        സ്ഥിരം തർക്കം asyncDisable = isAsyncDisable(c.disableExprp);
        Ast_കണ്ണി* bodyp = nullptr;
        Ast_കണ്ണി* clearp = nullptr;
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            സ്ഥിരം SvaStateVertex* സ്ഥിരം vtxp = static_cast<സ്ഥിരം SvaStateVertex*>(&vtxr);
            എങ്കിൽ (!vtxp->datap()->stateVarp) തുടരുക;

            Ast_കണ്ണി_വ്യഞ്ജകം* nextStatep = nullptr;
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edger : vtxp->inEdges()) {
                സ്ഥിരം SvaTransEdge& tedger = static_cast<സ്ഥിരം SvaTransEdge&>(edger);
                എങ്കിൽ (!tedger.m_consumesCycle) തുടരുക;
                സ്ഥിരം SvaStateVertex* സ്ഥിരം fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Clocked-edge source missing stateSig");

                Ast_കണ്ണി_വ്യഞ്ജകം* srcSigp = fromVtxp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം);
                srcSigp = andCond(c.flp, srcSigp, tedger.m_condp);

                എങ്കിൽ (c.disableExprp) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notDisp
                        = പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                    srcSigp = പുതിയ AstLogAnd{c.flp, srcSigp, notDisp};
                }
                nextStatep = orExprs(c.flp, nextStatep, srcSigp);
            }

            UASSERT_OBJ(nextStatep, vtxp,
                        "Registered vertex has no clocked incoming contribution");
            nextStatep = gateNotKill(c, nextStatep);

            AstAssignDly* സ്ഥിരം assignp = പുതിയ AstAssignDly{
                c.flp, പുതിയ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE}, nextStatep};
            bodyp = Ast_കണ്ണി::addNextNull(bodyp, assignp);
            എങ്കിൽ (asyncDisable) {
                clearp = Ast_കണ്ണി::addNextNull(
                    clearp,
                    പുതിയ AstAssignDly{
                        c.flp, പുതിയ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::WRITE},
                        പുതിയ AstConst{c.flp, AstConst::BitFalse{}}});
            }
        }
        എങ്കിൽ (bodyp) addStateAlways(c, bodyp, clearp);
    }

    // Phase 2b: In-place ring storage and registered ring metadata.
    ശൂന്യം emitDelayRingUpdates(LowerCtx& c, സ്ഥിരം തർക്കം എണ്ണം_Matches) {
        സ്ഥിരം തർക്കം asyncDisable = isAsyncDisable(c.disableExprp);
        വേണ്ടി (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* സ്ഥിരം vtxp = static_cast<SvaStateVertex*>(&vtxr);
            എങ്കിൽ (!vtxp->datap()->delayRingVarp) തുടരുക;
            AstVar* സ്ഥിരം idxp = vtxp->datap()->delayRingIdxVarp;
            AstVar* സ്ഥിരം live_എണ്ണം_Varp = vtxp->datap()->delay_Ring_Live_എണ്ണം_Varp;
            AstVar* സ്ഥിരം wrappedp = vtxp->datap()->delayRingWrappedVarp;
            സ്ഥിരം uint32_t size = static_cast<uint32_t>(vtxp->m_delayRingSize);

            Ast_കണ്ണി_വ്യഞ്ജകം* incomingp = nullptr;
            Ast_കണ്ണി_വ്യഞ്ജകം* incomingOverflowp = nullptr;
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edger : vtxp->inEdges()) {
                ++m_statDelayRingEdgeVisits;
                സ്ഥിരം SvaTransEdge& tedger = static_cast<സ്ഥിരം SvaTransEdge&>(edger);
                UASSERT_OBJ(tedger.m_consumesCycle == vtxp->m_isFixedDelayRing, vtxp,
                            "Delay-ring incoming edge kind mismatch");
                സ്ഥിരം SvaStateVertex* സ്ഥിരം fromVtxp = tedger.fromVtxp();
                UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                            "Delay-ring incoming source missing stateSig");
                UASSERT_OBJ(!എണ്ണം_Matches || fromVtxp->datap()->state_എണ്ണം_Sigp, fromVtxp,
                            "Delay-ring incoming source missing stateCountSig");
                // IEEE 1800-2023 16.14.1/16.14.3: assertion alternatives belong to one
                // evaluation; only cover sequence counts each end-of-match.
                Ast_കണ്ണി_വ്യഞ്ജകം* contribp
                    = എണ്ണം_Matches
                          ? fromVtxp->datap()->state_എണ്ണം_Sigp->clone_വൃക്ഷം_Pure(അസത്യം)
                          : പുതിയ AstExtend{c.flp,
                                          fromVtxp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം),
                                          m_u32DTypep->width()};
                Ast_കണ്ണി_വ്യഞ്ജകം* contribOverflowp
                    = എണ്ണം_Matches && fromVtxp->datap()->state_എണ്ണം_Overflow_Sigp
                          ? fromVtxp->datap()->state_എണ്ണം_Overflow_Sigp->clone_വൃക്ഷം_Pure(അസത്യം)
                          : nullptr;
                എങ്കിൽ (tedger.m_condp) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enablep = tedger.m_condp->clone_വൃക്ഷം_Pure(അസത്യം);
                    contribp = എണ്ണം_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, enablep)
                                   : addCountp(c.flp, nullptr, contribp, enablep);
                }
                എങ്കിൽ (c.disableExprp) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notDisp
                        = പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                    contribp = എണ്ണം_Matches
                                   ? gateCheckedCountp(c.flp, contribp, contribOverflowp, notDisp)
                                   : addCountp(c.flp, nullptr, contribp, notDisp);
                }
                incomingp = എണ്ണം_Matches ? addCheckedCountp(c.flp, incomingp, contribp,
                                                            incomingOverflowp, contribOverflowp)
                                         : addCountp(c.flp, incomingp, contribp);
            }
            UASSERT_OBJ(incomingp, vtxp, "Delay ring has no incoming edge");
            സ്ഥിരം തർക്കം storesCounts = vtxp->datap()->delayRingCounts;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം incomingCountp
                = storesCounts
                      ? incomingp->clone_വൃക്ഷം_Pure(അസത്യം)
                      : പുതിയ AstExtend{c.flp,
                                      പുതിയ AstNeq{c.flp, incomingp->clone_വൃക്ഷം_Pure(അസത്യം),
                                                 newTypedConstp(c.flp, incomingp->dtypep(), 0)},
                                      m_u32DTypep->width()};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം storedIncomingp
                = storesCounts ? incomingp
                               : പുതിയ AstNeq{c.flp, incomingp,
                                            newTypedConstp(c.flp, incomingp->dtypep(), 0)};
            // Only this block accesses the storage. Other processes read the cached head, so
            // the ring can be updated in place without an NBA shadow copy.
            AstAssign* സ്ഥിരം writeIncomingp = പുതിയ AstAssign{
                c.flp,
                delayRingSlot(c.flp, vtxp, പുതിയ AstVarRef{c.flp, idxp, VAccess::READ},
                              VAccess::WRITE),
                storedIncomingp};
            Ast_കണ്ണി* updateBodyp = writeIncomingp;
            // live_count <= live_count - outgoing_count + incoming_count;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം outgoingp = delay_Ring_Output_എണ്ണം(c.flp, vtxp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം afterOutgoingp
                = പുതിയ AstSub{c.flp, പുതിയ AstVarRef{c.flp, live_എണ്ണം_Varp, VAccess::READ}, outgoingp};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം oldAfterOutgoingp = afterOutgoingp->clone_വൃക്ഷം_Pure(അസത്യം);
            AstAdd* സ്ഥിരം nextLiveCountp = പുതിയ AstAdd{c.flp, afterOutgoingp, incomingCountp};
            എങ്കിൽ (storesCounts) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം liveOverflowp
                    = പുതിയ AstLt{c.flp, nextLiveCountp->clone_വൃക്ഷം_Pure(അസത്യം), oldAfterOutgoingp};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം overflowp = orExprs(c.flp, incomingOverflowp, liveOverflowp);
                updateBodyp = new_എണ്ണം_Overflow_Warning(c.flp, overflowp);
                updateBodyp->addNext(writeIncomingp);
            } അല്ലെങ്കിൽ {
                VL_DO_DANGLING(oldAfterOutgoingp->delete_വൃക്ഷം(), oldAfterOutgoingp);
            }
            // Cache the outgoing slot for the next index after the write. Reading after
            // writing also handles one-slot rings and two-slot range rings without alias cases.
            Ast_കണ്ണി_വ്യഞ്ജകം* nextHeadIdxp
                = nextRingIndex(c.flp, പുതിയ AstVarRef{c.flp, idxp, VAccess::READ}, size);
            എങ്കിൽ (!vtxp->m_isFixedDelayRing) {
                nextHeadIdxp = nextRingIndex(c.flp, nextHeadIdxp, size);
            }
            updateBodyp->addNext(പുതിയ AstAssignDly{
                c.flp, പുതിയ AstVarRef{c.flp, vtxp->datap()->delayRingHeadVarp, VAccess::WRITE},
                delayRingSlot(c.flp, vtxp, nextHeadIdxp)});
            updateBodyp->addNext(പുതിയ AstAssignDly{
                c.flp, പുതിയ AstVarRef{c.flp, live_എണ്ണം_Varp, VAccess::WRITE}, nextLiveCountp});
            // wrapped <= wrapped || idx == size - 1;
            updateBodyp->addNext(
                പുതിയ AstAssignDly{c.flp, പുതിയ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                 പുതിയ AstLogOr{c.flp, പുതിയ AstVarRef{c.flp, wrappedp, VAccess::READ},
                                              delayRingAtLastIndex(c.flp, idxp, size)}});
            // idx <= next_idx;
            updateBodyp->addNext(പുതിയ AstAssignDly{
                c.flp, പുതിയ AstVarRef{c.flp, idxp, VAccess::WRITE},
                nextRingIndex(c.flp, പുതിയ AstVarRef{c.flp, idxp, VAccess::READ}, size)});
            എങ്കിൽ (vtxp->m_delayRingAdvanceCondp) {
                updateBodyp = പുതിയ AstIf{
                    c.flp, sampled(vtxp->m_delayRingAdvanceCondp->clone_വൃക്ഷം_Pure(അസത്യം)),
                    updateBodyp};
            }

            Ast_കണ്ണി_വ്യഞ്ജകം* clearCondp = killActive(c);
            എങ്കിൽ (vtxp->m_delayRingClearCondp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_delayRingClearCondp->clone_വൃക്ഷം_Pure(അസത്യം)));
            }
            എങ്കിൽ (vtxp->m_abortClearp) {
                clearCondp = orExprs(c.flp, clearCondp,
                                     sampled(vtxp->m_abortClearp->clone_വൃക്ഷം_Pure(അസത്യം)));
            }
            എങ്കിൽ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം));
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* guardp = nullptr;
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp : vtxp->m_throughoutConds) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sampledp = sampled(cp->clone_വൃക്ഷം_Pure(അസത്യം));
                guardp = guardp ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstLogAnd{c.flp, guardp, sampledp})
                                : sampledp;
            }
            എങ്കിൽ (guardp) clearCondp = orExprs(c.flp, clearCondp, പുതിയ AstLogNot{c.flp, guardp});
            // Logically clear the ring without touching its wide storage.
            AstAssignDly* സ്ഥിരം clearCountp
                = പുതിയ AstAssignDly{c.flp, പുതിയ AstVarRef{c.flp, live_എണ്ണം_Varp, VAccess::WRITE},
                                   newTypedConstp(c.flp, live_എണ്ണം_Varp->dtypep(), 0)};
            clearCountp->addNext(പുതിയ AstAssignDly{c.flp,
                                                  പുതിയ AstVarRef{c.flp, wrappedp, VAccess::WRITE},
                                                  പുതിയ AstConst{c.flp, AstConst::BitFalse{}}});
            clearCountp->addNext(പുതിയ AstAssignDly{c.flp,
                                                  പുതിയ AstVarRef{c.flp, idxp, VAccess::WRITE},
                                                  newTypedConstp(c.flp, idxp->dtypep(), 0)});
            updateBodyp = പുതിയ AstIf{c.flp, clearCondp, clearCountp, updateBodyp};

            addStateAlways(c, updateBodyp, asyncDisable ? clearCountp->clone_വൃക്ഷം(സത്യം) : nullptr);
        }
    }

    // Phase 2c: SAnd combiner done-latch always block.
    // NBA semantics ensure doneL/doneR read pre-update values (IEEE 16.9.5).
    ശൂന്യം emitAndCombinerDoneLatchNba(LowerCtx& c) {
        സ്ഥിരം തർക്കം asyncDisable = isAsyncDisable(c.disableExprp);
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            സ്ഥിരം SvaStateVertex* സ്ഥിരം avp = static_cast<സ്ഥിരം SvaStateVertex*>(&vtxr);
            എങ്കിൽ (!avp->datap()->doneLVarp) തുടരുക;
            // doneLVars is non-null only for AndCombiner vertices, which always
            // have both m_andLhsTermp and m_andRhsTermp set at build time.
            UASSERT_OBJ(avp->m_andLhsTermp && avp->m_andRhsTermp, avp,
                        "AndCombiner vertex missing LHS/RHS terminal");
            സ്ഥിരം SvaStateVertex* സ്ഥിരം lhsTermp = avp->m_andLhsTermp;
            സ്ഥിരം SvaStateVertex* സ്ഥിരം rhsTermp = avp->m_andRhsTermp;
            // resolveLinks' 2*N+2 fixed-point pass is guaranteed to populate
            // stateSigp on every AndCombiner and its LHS/RHS terminals.
            UASSERT_OBJ(lhsTermp->datap()->stateSigp && rhsTermp->datap()->stateSigp
                            && avp->datap()->stateSigp,
                        avp, "AndCombiner stateSigp chain unresolved after resolveLinks");

            AstAssignDly* സ്ഥിരം clearLp = പുതിയ AstAssignDly{
                c.flp, പുതിയ AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                പുതിയ AstConst{c.flp, AstConst::BitFalse{}}};
            AstAssignDly* സ്ഥിരം clearRp = പുതിയ AstAssignDly{
                c.flp, പുതിയ AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                പുതിയ AstConst{c.flp, AstConst::BitFalse{}}};
            clearLp->addNext(clearRp);

            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchLNowp
                = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, avp->m_andLhsCondp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchRNowp
                = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, avp->m_andRhsCondp);
            Ast_കണ്ണി_വ്യഞ്ജകം* gateLp = matchLNowp;
            Ast_കണ്ണി_വ്യഞ്ജകം* gateRp = matchRNowp;
            എങ്കിൽ (c.disableExprp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notDisLp
                    = പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                gateLp = പുതിയ AstLogAnd{c.flp, gateLp, notDisLp};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notDisRp
                    = പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                gateRp = പുതിയ AstLogAnd{c.flp, gateRp, notDisRp};
            }
            AstAssignDly* സ്ഥിരം setLp = പുതിയ AstAssignDly{
                c.flp, പുതിയ AstVarRef{c.flp, avp->datap()->doneLVarp, VAccess::WRITE},
                പുതിയ AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* സ്ഥിരം setLIfp = പുതിയ AstIf{c.flp, gateLp, setLp, nullptr};
            AstAssignDly* സ്ഥിരം setRp = പുതിയ AstAssignDly{
                c.flp, പുതിയ AstVarRef{c.flp, avp->datap()->doneRVarp, VAccess::WRITE},
                പുതിയ AstConst{c.flp, AstConst::BitTrue{}}};
            AstIf* സ്ഥിരം setRIfp = പുതിയ AstIf{c.flp, gateRp, setRp, nullptr};
            setLIfp->addNext(setRIfp);

            Ast_കണ്ണി_വ്യഞ്ജകം* clearCondp = പുതിയ AstLogOr{c.flp, killActive(c),
                                                   avp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം)};
            എങ്കിൽ (c.disableExprp) {
                clearCondp = orExprs(c.flp, clearCondp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം));
            }
            AstIf* സ്ഥിരം topp = പുതിയ AstIf{c.flp, clearCondp, clearLp, setLIfp};
            addStateAlways(c, topp, asyncDisable ? clearLp->clone_വൃക്ഷം(സത്യം) : nullptr);
        }
    }

    ശൂന്യം emitKillAckNba(LowerCtx& c) {
        AstAssignDly* സ്ഥിരം ackp
            = പുതിയ AstAssignDly{c.flp, പുതിയ AstVarRef{c.flp, c.killVarp, VAccess::WRITE},
                               assertKillGet(c.flp, c.assertType, c.directiveType)};
        m_modp->addStmtsp(
            പുതിയ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_വൃക്ഷം(അസത്യം), ackp});
    }

    // Phase 3/3a/3b: Compute terminal match/reject signals, required-step reject,
    // throughout-drop reject; clean up intermediate state signals.
    // Phase 3: terminalActive and rejectBase from Links to matchVertex.
    // Builder only adds Links (non-clocked) to matchVertex via addLink in
    // wireMatchAndMidSources. For cover sequence, also count each end-of-match
    // so the action can be replayed without unrolling endpoints.
    ശൂന്യം computeTerminalMatchAndReject(LowerCtx& c, SignalSet& sigs, സ്ഥിരം തർക്കം need_Match_എണ്ണം) {
        വേണ്ടി (സ്ഥിരം SvaTransEdge* സ്ഥിരം tedgep : c.graph.allEdges()) {
            എങ്കിൽ (tedgep->toVtxp() != c.graph.m_matchVertexp) തുടരുക;
            SvaStateVertex* സ്ഥിരം fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp, fromVtxp,
                        "Terminal-link source missing stateSig");

            Ast_കണ്ണി_വ്യഞ്ജകം* srcSigp = fromVtxp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം);
            srcSigp = andCond(c.flp, srcSigp, tedgep->m_condp);
            എങ്കിൽ (need_Match_എണ്ണം) {
                UASSERT_OBJ(fromVtxp->datap()->state_എണ്ണം_Sigp, fromVtxp,
                            "Terminal-link source missing stateCountSig");
                Ast_കണ്ണി_വ്യഞ്ജകം* contributionp
                    = fromVtxp->datap()->state_എണ്ണം_Sigp->clone_വൃക്ഷം_Pure(അസത്യം);
                Ast_കണ്ണി_വ്യഞ്ജകം* contributionOverflowp
                    = fromVtxp->datap()->state_എണ്ണം_Overflow_Sigp
                          ? fromVtxp->datap()->state_എണ്ണം_Overflow_Sigp->clone_വൃക്ഷം_Pure(അസത്യം)
                          : nullptr;
                എങ്കിൽ (tedgep->m_condp) {
                    contributionp = gateCheckedCountp(c.flp, contributionp, contributionOverflowp,
                                                      tedgep->m_condp->clone_വൃക്ഷം_Pure(അസത്യം));
                }
                sigs.matchCountp
                    = addCheckedCountp(c.flp, sigs.matchCountp, contributionp,
                                       sigs.match_എണ്ണം_Overflowp, contributionOverflowp);
            }

            എങ്കിൽ (fromVtxp->m_delayRingSize && !fromVtxp->m_isFixedDelayRing) {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_വൃക്ഷം_Pure(അസത്യം));
                // reject |= (wrapped || idx == size - 1) && head && final_condition;
                Ast_കണ്ണി_വ്യഞ്ജകം* expireContribp = delay_Ring_Output_എണ്ണം(c.flp, fromVtxp);
                expireContribp = andCond(c.flp, expireContribp, tedgep->m_condp);
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, expireContribp);
                VL_DO_DANGLING(srcSigp->delete_വൃക്ഷം(), srcSigp);
            } അല്ലെങ്കിൽ എങ്കിൽ (fromVtxp->m_isUnbounded || fromVtxp->m_isAndCombiner) {
                sigs.terminalActivep = orExprs(c.flp, sigs.terminalActivep, srcSigp);
            } അല്ലെങ്കിൽ {
                sigs.terminalActivep
                    = orExprs(c.flp, sigs.terminalActivep, srcSigp->clone_വൃക്ഷം_Pure(അസത്യം));
                sigs.rejectBasep = orExprs(c.flp, sigs.rejectBasep, srcSigp);
            }
        }
        // wireMatchAndMidSources always adds a Link from result.termVertexp
        // to m_matchVertexp, so the loop above always sets terminalActivep.
        UASSERT_OBJ(sigs.terminalActivep, c.graph.m_matchVertexp,
                    "No terminal edge to match vertex");
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newThroughoutThreadFailCountp(LowerCtx& c, AstVar* സ്ഥിരം delay_Ring_Live_എണ്ണം_Varp,
                                               Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം stateExprp,
                                               Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enablep) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം activeThreadCountp
            = delay_Ring_Live_എണ്ണം_Varp
                  ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(
                        പുതിയ AstVarRef{c.flp, delay_Ring_Live_എണ്ണം_Varp, VAccess::READ})
                  : പുതിയ AstExtend{c.flp, stateExprp->clone_വൃക്ഷം_Pure(അസത്യം), m_u32DTypep->width()};
        തിരികെ addCountp(c.flp, nullptr, activeThreadCountp, enablep);
    }

    // Phase 3b: Throughout-drop and ring-wide abort rejection.
    ശൂന്യം computeThroughoutReject(LowerCtx& c, SignalSet& sigs, സ്ഥിരം തർക്കം need_Thread_Fail_എണ്ണം) {
        വേണ്ടി (സ്ഥിരം V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            സ്ഥിരം SvaStateVertex* സ്ഥിരം vtxp = static_cast<സ്ഥിരം SvaStateVertex*>(&vtxr);
            സ്ഥിരം auto& conds = vtxp->m_throughoutConds;
            എങ്കിൽ (conds.empty() && !vtxp->m_abortRejectp) തുടരുക;
            എങ്കിൽ (vtxp->m_isAndCombiner) തുടരുക;
            Ast_കണ്ണി_വ്യഞ്ജകം* stateExprp = nullptr;
            എങ്കിൽ (vtxp->datap()->stateVarp) {
                stateExprp = പുതിയ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
            } അല്ലെങ്കിൽ എങ്കിൽ (vtxp->datap()->delayRingVarp && vtxp->m_isFixedDelayRing) {
                stateExprp
                    = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_എണ്ണം_Varp);
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(vtxp->datap()->stateSigp, vtxp,
                            "Throughout-conds vertex missing state representation");
                stateExprp = vtxp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം);
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* guardp = nullptr;
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cp : conds) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sp = sampled(cp->clone_വൃക്ഷം_Pure(അസത്യം));
                guardp = guardp ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstLogAnd{c.flp, guardp, sp}) : sp;
            }
            എങ്കിൽ (vtxp->m_abortRejectp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notAbortp
                    = പുതിയ AstLogNot{c.flp, sampled(vtxp->m_abortRejectp->clone_വൃക്ഷം_Pure(അസത്യം))};
                guardp = guardp
                             ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstLogAnd{c.flp, guardp, notAbortp})
                             : notAbortp;
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* rejectCondp = പുതിയ AstLogNot{c.flp, guardp};
            എങ്കിൽ (vtxp->m_abortClearp) {
                // Any abort clears the ring. Accept aborts suppress a simultaneous guard failure;
                // reject aborts are restored below so they still force rejection.
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notAbortClearp
                    = പുതിയ AstLogNot{c.flp, sampled(vtxp->m_abortClearp->clone_വൃക്ഷം_Pure(അസത്യം))};
                rejectCondp = പുതിയ AstLogAnd{c.flp, rejectCondp, notAbortClearp};
                എങ്കിൽ (vtxp->m_abortRejectp) {
                    rejectCondp = പുതിയ AstLogOr{
                        c.flp, rejectCondp, sampled(vtxp->m_abortRejectp->clone_വൃക്ഷം_Pure(അസത്യം))};
                }
            }
            എങ്കിൽ (need_Thread_Fail_എണ്ണം) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം contributionp
                    = newThroughoutThreadFailCountp(c, vtxp->datap()->delay_Ring_Live_എണ്ണം_Varp,
                                                    stateExprp, rejectCondp->clone_വൃക്ഷം_Pure(അസത്യം));
                sigs.threadFailCountp = addCountp(c.flp, sigs.threadFailCountp, contributionp);
                എങ്കിൽ (vtxp->m_abortRejectp && vtxp->m_delayRingAdvanceCondp) {
                    // An advancing ring thread also occupies its same-tick match vertex in the
                    // unrolled NFA, so abort rejection must replay both fail actions.
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം abortAndAdvancep = പുതിയ AstLogAnd{
                        c.flp, sampled(vtxp->m_abortRejectp->clone_വൃക്ഷം_Pure(അസത്യം)),
                        sampled(vtxp->m_delayRingAdvanceCondp->clone_വൃക്ഷം_Pure(അസത്യം))};
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchContributionp = newThroughoutThreadFailCountp(
                        c, vtxp->datap()->delay_Ring_Live_എണ്ണം_Varp, stateExprp, abortAndAdvancep);
                    sigs.threadFailCountp
                        = addCountp(c.flp, sigs.threadFailCountp, matchContributionp);
                }
            }
            sigs.throughoutRejectp = orExprs(c.flp, sigs.throughoutRejectp,
                                             പുതിയ AstLogAnd{c.flp, stateExprp, rejectCondp});
        }
    }

    SignalSet computeSignals(LowerCtx& c, സ്ഥിരം തർക്കം need_Thread_Fail_എണ്ണം,
                             സ്ഥിരം തർക്കം need_Throughout_Thread_Fail_എണ്ണം, സ്ഥിരം തർക്കം need_Match_എണ്ണം) {
        SignalSet sigs;

        computeTerminalMatchAndReject(c, sigs, need_Match_എണ്ണം);

        // Phase 3a: required-step rejection.
        // Builder only sets m_rejectOnFail on non-clocked Links with m_condp
        // or m_condVtxp, and the source always has a resolved stateSig.
        വേണ്ടി (സ്ഥിരം SvaTransEdge* സ്ഥിരം tedgep : c.graph.allEdges()) {
            എങ്കിൽ (!tedgep->m_rejectOnFail) തുടരുക;
            സ്ഥിരം SvaStateVertex* സ്ഥിരം fromVtxp = tedgep->fromVtxp();
            UASSERT_OBJ(fromVtxp->datap()->stateSigp && (tedgep->m_condp || tedgep->m_condVtxp),
                        fromVtxp,
                        "rejectOnFail Link must have condp/condVtxp and source stateSig");
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം srcSigp = fromVtxp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;
            എങ്കിൽ (tedgep->m_condVtxp) {
                UASSERT_OBJ(tedgep->m_condVtxp->datap()->stateSigp, tedgep->m_condVtxp,
                            "rejectOnFail condVtxp missing stateSig");
                condp = tedgep->m_condVtxp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം);
                എങ്കിൽ (tedgep->m_condp) {
                    condp = പുതിയ AstLogOr{c.flp, condp, tedgep->m_condp->clone_വൃക്ഷം_Pure(അസത്യം)};
                }
            } അല്ലെങ്കിൽ {
                condp = tedgep->m_condp->clone_വൃക്ഷം_Pure(അസത്യം);
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notCondp = പുതിയ AstLogNot{c.flp, condp};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rawFailp = പുതിയ AstLogAnd{c.flp, srcSigp, notCondp};
            എങ്കിൽ (need_Thread_Fail_എണ്ണം) {
                // thread_fail_count += fail;
                sigs.threadFailCountp = addCountp(
                    c.flp, sigs.threadFailCountp,
                    പുതിയ AstExtend{c.flp, rawFailp->clone_വൃക്ഷം_Pure(അസത്യം), m_u32DTypep->width()});
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം failp = gateNotKill(c, rawFailp);
            sigs.requiredStepRejectp = orExprs(c.flp, sigs.requiredStepRejectp, failp);
        }

        computeThroughoutReject(c, sigs, need_Throughout_Thread_Fail_എണ്ണം);
        എങ്കിൽ (sigs.threadFailCountp) {
            sigs.threadFailCountp
                = addCountp(c.flp, nullptr, sigs.threadFailCountp, notKillActive(c));
        }
        എങ്കിൽ (sigs.matchCountp) {
            എങ്കിൽ (c.matchCondp) {
                sigs.matchCountp
                    = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_എണ്ണം_Overflowp,
                                        sampled(c.matchCondp->clone_വൃക്ഷം_Pure(അസത്യം)));
            }
            sigs.matchCountp = gateCheckedCountp(c.flp, sigs.matchCountp, sigs.match_എണ്ണം_Overflowp,
                                                 notKillActive(c));
        }
        sigs.terminalActivep = gateNotKill(c, sigs.terminalActivep);
        sigs.rejectBasep = gateNotKill(c, sigs.rejectBasep);
        sigs.throughoutRejectp = gateNotKill(c, sigs.throughoutRejectp);

        // Clean up intermediate state signals. These are orphan subtrees
        // (never linked into the enclosing AST); deleteTree() is immediate
        // which is what we want since stateSigp lifetime ends with this scope.
        വേണ്ടി (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* സ്ഥിരം vtxp = static_cast<SvaStateVertex*>(&vtxr);
            Ast_കണ്ണി_വ്യഞ്ജകം*& sigp = vtxp->datap()->stateSigp;
            എങ്കിൽ (sigp) VL_DO_DANGLING(sigp->delete_വൃക്ഷം(), sigp);
            Ast_കണ്ണി_വ്യഞ്ജകം*& countp = vtxp->datap()->state_എണ്ണം_Sigp;
            എങ്കിൽ (countp) VL_DO_DANGLING(countp->delete_വൃക്ഷം(), countp);
            Ast_കണ്ണി_വ്യഞ്ജകം*& overflowp = vtxp->datap()->state_എണ്ണം_Overflow_Sigp;
            എങ്കിൽ (overflowp) VL_DO_DANGLING(overflowp->delete_വൃക്ഷം(), overflowp);
        }
        // Disable iff gating (IEEE 1800-2023 16.12) on the current level value.
        എങ്കിൽ (c.disableExprp) {
            // terminalActivep is always set, so gate it unconditionally.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notTermp
                = പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
            sigs.terminalActivep = പുതിയ AstLogAnd{c.flp, sigs.terminalActivep, notTermp};
            എങ്കിൽ (sigs.rejectBasep) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notDisp
                    = പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                sigs.rejectBasep = പുതിയ AstLogAnd{c.flp, sigs.rejectBasep, notDisp};
            }
            എങ്കിൽ (sigs.throughoutRejectp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notDisp
                    = പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                sigs.throughoutRejectp = പുതിയ AstLogAnd{c.flp, sigs.throughoutRejectp, notDisp};
            }
            എങ്കിൽ (sigs.requiredStepRejectp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notDisp
                    = പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)};
                sigs.requiredStepRejectp = പുതിയ AstLogAnd{c.flp, sigs.requiredStepRejectp, notDisp};
            }
            എങ്കിൽ (sigs.matchCountp) {
                sigs.matchCountp = gateCheckedCountp(
                    c.flp, sigs.matchCountp, sigs.match_എണ്ണം_Overflowp,
                    പുതിയ AstLogNot{c.flp, c.disableExprp->clone_വൃക്ഷം_Pure(അസത്യം)});
            }
        }

        എങ്കിൽ (sigs.match_എണ്ണം_Overflowp) {
            m_modp->addStmtsp(
                പുതിയ AstAlways{c.flp, VAlwaysKwd::ALWAYS, c.senTreep->clone_വൃക്ഷം(അസത്യം),
                              new_എണ്ണം_Overflow_Warning(c.flp, sigs.match_എണ്ണം_Overflowp)});
            sigs.match_എണ്ണം_Overflowp = nullptr;
        }

        തിരികെ sigs;
    }

    // Phase 1: Resolve combinational Links via fixed-point propagation.
    ശൂന്യം resolveLinks(LowerCtx& c, Ast_കണ്ണി_വ്യഞ്ജകം* triggerExprp, സ്ഥിരം തർക്കം എണ്ണം_Matches) {
        // datap() was freshly allocated in lower() -- all state signals start null.
        c.graph.m_startVertexp->datap()->stateSigp = triggerExprp->clone_വൃക്ഷം_Pure(അസത്യം);
        c.graph.m_startVertexp->datap()->state_എണ്ണം_Sigp
            = പുതിയ AstExtend{c.flp, triggerExprp->clone_വൃക്ഷം_Pure(അസത്യം), m_u32DTypep->width()};
        വേണ്ടി (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
            SvaStateVertex* സ്ഥിരം vtxp = static_cast<SvaStateVertex*>(&vtxr);
            എങ്കിൽ (vtxp->datap()->stateVarp) {
                vtxp->datap()->stateSigp
                    = പുതിയ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ};
                vtxp->datap()->state_എണ്ണം_Sigp = പുതിയ AstExtend{
                    c.flp, പുതിയ AstVarRef{c.flp, vtxp->datap()->stateVarp, VAccess::READ},
                    m_u32DTypep->width()};
            } അല്ലെങ്കിൽ എങ്കിൽ (vtxp->datap()->delayRingVarp) {
                എങ്കിൽ (vtxp->m_isFixedDelayRing) {
                    // state_count = wrapped ? head : 0; state = state_count != 0;
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം countp = delay_Ring_Output_എണ്ണം(c.flp, vtxp);
                    vtxp->datap()->stateSigp = പുതിയ AstNeq{c.flp, countp->clone_വൃക്ഷം_Pure(അസത്യം),
                                                          newTypedConstp(c.flp, m_u32DTypep, 0)};
                    vtxp->datap()->state_എണ്ണം_Sigp = countp;
                } അല്ലെങ്കിൽ {
                    vtxp->datap()->stateSigp
                        = delayRingHasLiveThreadsp(c.flp, vtxp->datap()->delay_Ring_Live_എണ്ണം_Varp);
                    vtxp->datap()->state_എണ്ണം_Sigp = പുതിയ AstVarRef{
                        c.flp, vtxp->datap()->delay_Ring_Live_എണ്ണം_Varp, VAccess::READ};
                }
            }
        }
        // Fixed-point propagation along zero-delay (Link) edges. Rebuild each
        // derived signal from its incoming edges on every pass; appending the same
        // contributions repeatedly makes merge expressions grow exponentially.
        സ്ഥിരം size_t passes = 2 * c.graph.m_graph.vertices().size() + 2;
        വേണ്ടി (size_t pass = 0; pass < passes; ++pass) {
            // Rebuild SAnd combiners once both sub-NFA terminals are available.
            വേണ്ടി (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* സ്ഥിരം vtxp = static_cast<SvaStateVertex*>(&vtxr);
                എങ്കിൽ (!vtxp->m_isAndCombiner) തുടരുക;
                // AndCombiner vertices always have both terminal pointers set.
                UASSERT_OBJ(vtxp->m_andLhsTermp && vtxp->m_andRhsTermp, vtxp,
                            "AndCombiner vertex missing LHS/RHS terminal");
                സ്ഥിരം SvaStateVertex* സ്ഥിരം lhsTermp = vtxp->m_andLhsTermp;
                സ്ഥിരം SvaStateVertex* സ്ഥിരം rhsTermp = vtxp->m_andRhsTermp;
                എങ്കിൽ (!lhsTermp->datap()->stateSigp || !rhsTermp->datap()->stateSigp) തുടരുക;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchLp
                    = buildMatchNow(c.flp, lhsTermp->datap()->stateSigp, vtxp->m_andLhsCondp);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchRp
                    = buildMatchNow(c.flp, rhsTermp->datap()->stateSigp, vtxp->m_andRhsCondp);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം doneLOrp = പുതിയ AstLogOr{
                    c.flp, പുതിയ AstVarRef{c.flp, vtxp->datap()->doneLVarp, VAccess::READ}, matchLp};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം doneROrp = പുതിയ AstLogOr{
                    c.flp, പുതിയ AstVarRef{c.flp, vtxp->datap()->doneRVarp, VAccess::READ}, matchRp};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bothp = പുതിയ AstLogAnd{c.flp, doneLOrp, doneROrp};
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം oneNowp = പുതിയ AstLogOr{c.flp, matchLp->clone_വൃക്ഷം_Pure(അസത്യം),
                                                          matchRp->clone_വൃക്ഷം_Pure(അസത്യം)};
                എങ്കിൽ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_വൃക്ഷം(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = പുതിയ AstLogAnd{c.flp, bothp, oneNowp};
                എങ്കിൽ (vtxp->datap()->state_എണ്ണം_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_എണ്ണം_Sigp->delete_വൃക്ഷം(),
                                   vtxp->datap()->state_എണ്ണം_Sigp);
                }
                vtxp->datap()->state_എണ്ണം_Sigp = പുതിയ AstExtend{
                    c.flp, vtxp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം), m_u32DTypep->width()};
            }

            വേണ്ടി (V3GraphVertex& vtxr : c.graph.m_graph.vertices()) {
                SvaStateVertex* സ്ഥിരം vtxp = static_cast<SvaStateVertex*>(&vtxr);
                എങ്കിൽ (vtxp == c.graph.m_startVertexp || vtxp->datap()->stateVarp
                    || vtxp->datap()->delayRingVarp || vtxp->m_isAndCombiner || vtxp->m_isMatch
                    || vtxp->m_isRejectSink) {
                    തുടരുക;
                }
                Ast_കണ്ണി_വ്യഞ്ജകം* nextStatep = nullptr;
                Ast_കണ്ണി_വ്യഞ്ജകം* nextStateCountp = nullptr;
                Ast_കണ്ണി_വ്യഞ്ജകം* next_State_എണ്ണം_Overflowp = nullptr;
                വേണ്ടി (സ്ഥിരം V3GraphEdge& er : vtxp->inEdges()) {
                    സ്ഥിരം SvaTransEdge& te = static_cast<സ്ഥിരം SvaTransEdge&>(er);
                    സ്ഥിരം SvaStateVertex* സ്ഥിരം fromVtxp = te.fromVtxp();
                    എങ്കിൽ (!fromVtxp->datap()->stateSigp) തുടരുക;
                    UASSERT_OBJ(fromVtxp->datap()->state_എണ്ണം_Sigp, fromVtxp,
                                "Link source missing stateCountSig");
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം contributionp = andCond(
                        c.flp, fromVtxp->datap()->stateSigp->clone_വൃക്ഷം_Pure(അസത്യം), te.m_condp);
                    nextStatep = orExprs(c.flp, nextStatep, contributionp);
                    Ast_കണ്ണി_വ്യഞ്ജകം* എണ്ണം_Contributionp
                        = fromVtxp->datap()->state_എണ്ണം_Sigp->clone_വൃക്ഷം_Pure(അസത്യം);
                    Ast_കണ്ണി_വ്യഞ്ജകം* എണ്ണം_Contribution_Overflowp
                        = എണ്ണം_Matches && fromVtxp->datap()->state_എണ്ണം_Overflow_Sigp
                              ? fromVtxp->datap()->state_എണ്ണം_Overflow_Sigp->clone_വൃക്ഷം_Pure(അസത്യം)
                              : nullptr;
                    എങ്കിൽ (te.m_condp) {
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം enablep = te.m_condp->clone_വൃക്ഷം_Pure(അസത്യം);
                        എണ്ണം_Contributionp
                            = എണ്ണം_Matches
                                  ? gateCheckedCountp(c.flp, എണ്ണം_Contributionp,
                                                      എണ്ണം_Contribution_Overflowp, enablep)
                                  : addCountp(c.flp, nullptr, എണ്ണം_Contributionp, enablep);
                    }
                    nextStateCountp
                        = എണ്ണം_Matches
                              ? addCheckedCountp(c.flp, nextStateCountp, എണ്ണം_Contributionp,
                                                 next_State_എണ്ണം_Overflowp,
                                                 എണ്ണം_Contribution_Overflowp)
                              : addCountp(c.flp, nextStateCountp, എണ്ണം_Contributionp);
                }
                എങ്കിൽ (vtxp->datap()->stateSigp) {
                    VL_DO_DANGLING(vtxp->datap()->stateSigp->delete_വൃക്ഷം(),
                                   vtxp->datap()->stateSigp);
                }
                vtxp->datap()->stateSigp = nextStatep;
                എങ്കിൽ (vtxp->datap()->state_എണ്ണം_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_എണ്ണം_Sigp->delete_വൃക്ഷം(),
                                   vtxp->datap()->state_എണ്ണം_Sigp);
                }
                vtxp->datap()->state_എണ്ണം_Sigp = nextStateCountp;
                എങ്കിൽ (vtxp->datap()->state_എണ്ണം_Overflow_Sigp) {
                    VL_DO_DANGLING(vtxp->datap()->state_എണ്ണം_Overflow_Sigp->delete_വൃക്ഷം(),
                                   vtxp->datap()->state_എണ്ണം_Overflow_Sigp);
                }
                vtxp->datap()->state_എണ്ണം_Overflow_Sigp = next_State_എണ്ണം_Overflowp;
            }
        }
    }

പൊതു:
    // Combine terminal/reject signals into final output expression.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* assembleResult(Ast_കണ്ണി_Cover_Or_Assert* സ്ഥിരം assertp, സ്ഥിരം തർക്കം negated,
                                       Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchCondp, സ്ഥിരം SignalSet& sigs,
                                       Ast_കണ്ണി_വ്യഞ്ജകം** സ്ഥിരം outMatchpp) {
        File_വരി* സ്ഥിരം flp = assertp->fileline();
        സ്ഥിരം തർക്കം isCover = VN_IS(assertp, Cover);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം terminalActivep = sigs.terminalActivep;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rejectBasep = sigs.rejectBasep;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം throughoutRejectp = sigs.throughoutRejectp;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം requiredStepRejectp = sigs.requiredStepRejectp;
        // Property negation (IEEE 1800-2023 16.12.1 `not`): invert match/reject.
        എങ്കിൽ (negated) {
            എങ്കിൽ (isCover) {
                എങ്കിൽ (terminalActivep)
                    VL_DO_DANGLING(terminalActivep->delete_വൃക്ഷം(), terminalActivep);
                Ast_കണ്ണി_വ്യഞ്ജകം* negRejectp = nullptr;
                എങ്കിൽ (matchCondp && rejectBasep) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sampledCondp = sampled(matchCondp->clone_വൃക്ഷം_Pure(അസത്യം));
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം notCondp = പുതിയ AstLogNot{flp, sampledCondp};
                    negRejectp = പുതിയ AstLogAnd{flp, rejectBasep, notCondp};
                } അല്ലെങ്കിൽ എങ്കിൽ (rejectBasep) {
                    VL_DO_DANGLING(rejectBasep->delete_വൃക്ഷം(), rejectBasep);
                }
                എങ്കിൽ (throughoutRejectp) negRejectp = orExprs(flp, negRejectp, throughoutRejectp);
                എങ്കിൽ (requiredStepRejectp)
                    negRejectp = orExprs(flp, negRejectp, requiredStepRejectp);
                തിരികെ negRejectp ? negRejectp : പുതിയ AstConst{flp, AstConst::BitFalse{}};
            }
            // Negated assert/assume: output = !match.
            Ast_കണ്ണി_വ്യഞ്ജകം* matchp = terminalActivep;
            എങ്കിൽ (matchCondp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sampledCondp = sampled(matchCondp->clone_വൃക്ഷം_Pure(അസത്യം));
                matchp = പുതിയ AstLogAnd{flp, matchp, sampledCondp};
            }
            എങ്കിൽ (outMatchpp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* notPMatchp = nullptr;
                എങ്കിൽ (matchCondp && rejectBasep) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sampledCondp = sampled(matchCondp->clone_വൃക്ഷം_Pure(അസത്യം));
                    notPMatchp = പുതിയ AstLogAnd{flp, rejectBasep->clone_വൃക്ഷം_Pure(അസത്യം),
                                               പുതിയ AstLogNot{flp, sampledCondp}};
                } അല്ലെങ്കിൽ എങ്കിൽ (rejectBasep) {
                    notPMatchp = rejectBasep->clone_വൃക്ഷം_Pure(അസത്യം);
                }
                എങ്കിൽ (throughoutRejectp)
                    notPMatchp = orExprs(flp, notPMatchp, throughoutRejectp->clone_വൃക്ഷം_Pure(അസത്യം));
                എങ്കിൽ (requiredStepRejectp)
                    notPMatchp
                        = orExprs(flp, notPMatchp, requiredStepRejectp->clone_വൃക്ഷം_Pure(അസത്യം));
                *outMatchpp = notPMatchp;
            }
            എങ്കിൽ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_വൃക്ഷം(), throughoutRejectp);
            എങ്കിൽ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_വൃക്ഷം(), rejectBasep);
            എങ്കിൽ (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_വൃക്ഷം(), requiredStepRejectp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultExprp = പുതിയ AstLogNot{flp, matchp};
            തിരികെ resultExprp;
        }
        എങ്കിൽ (isCover) {
            എങ്കിൽ (throughoutRejectp)
                VL_DO_DANGLING(throughoutRejectp->delete_വൃക്ഷം(), throughoutRejectp);
            എങ്കിൽ (rejectBasep) VL_DO_DANGLING(rejectBasep->delete_വൃക്ഷം(), rejectBasep);
            എങ്കിൽ (requiredStepRejectp)
                VL_DO_DANGLING(requiredStepRejectp->delete_വൃക്ഷം(), requiredStepRejectp);
            എങ്കിൽ (matchCondp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sampledCondp = sampled(matchCondp->clone_വൃക്ഷം_Pure(അസത്യം));
                തിരികെ പുതിയ AstLogAnd{flp, terminalActivep, sampledCondp};
            }
            തിരികെ terminalActivep;
        }
        // Assert/assume: output = !reject
        Ast_കണ്ണി_വ്യഞ്ജകം* rejectp = nullptr;
        എങ്കിൽ (matchCondp && rejectBasep) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sampledCondp = sampled(matchCondp->clone_വൃക്ഷം_Pure(അസത്യം));
            rejectp = പുതിയ AstLogAnd{flp, rejectBasep, പുതിയ AstLogNot{flp, sampledCondp}};
        } അല്ലെങ്കിൽ എങ്കിൽ (rejectBasep) {
            VL_DO_DANGLING(rejectBasep->delete_വൃക്ഷം(), rejectBasep);
        }
        എങ്കിൽ (outMatchpp) {
            Ast_കണ്ണി_വ്യഞ്ജകം* matchExprp = terminalActivep->clone_വൃക്ഷം_Pure(അസത്യം);
            എങ്കിൽ (matchCondp) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sp = sampled(matchCondp->clone_വൃക്ഷം_Pure(അസത്യം));
                matchExprp = പുതിയ AstLogAnd{flp, matchExprp, sp};
            }
            *outMatchpp = matchExprp;
        }
        എങ്കിൽ (terminalActivep) VL_DO_DANGLING(terminalActivep->delete_വൃക്ഷം(), terminalActivep);
        എങ്കിൽ (throughoutRejectp) rejectp = orExprs(flp, rejectp, throughoutRejectp);
        എങ്കിൽ (requiredStepRejectp) rejectp = orExprs(flp, rejectp, requiredStepRejectp);
        എങ്കിൽ (!rejectp) തിരികെ പുതിയ AstConst{flp, AstConst::BitTrue{}};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultExprp = പുതിയ AstLogNot{flp, rejectp};
        തിരികെ resultExprp;
    }

    explicit SvaNfaLowering(Ast_കണ്ണി_Module* modp)
        : m_modp{modp}
        , m_u32DTypep{modp->findBasicDType(VBasicDTypeKwd::UINT32)} {}
    ~SvaNfaLowering() {
        V3Stats::addStatSum("Assertions, NFA delay ring edge visits", m_statDelayRingEdgeVisits);
    }

    // Lower NFA graph to synthesizable AstAlways blocks and raw result signals.
    // Links are combinational; Edges are registered (NBA).
    SignalSet lower(Ast_കണ്ണി_Cover_Or_Assert* സ്ഥിരം assertp, SvaGraph& graph,
                    Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം matchCondp,
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം disableExprp, സ്ഥിരം തർക്കം need_Thread_Fail_എണ്ണം,
                    സ്ഥിരം തർക്കം need_Throughout_Thread_Fail_എണ്ണം) {
        File_വരി* സ്ഥിരം flp = assertp->fileline();
        AstCover* സ്ഥിരം coverp = VN_CAST(assertp, Cover);
        സ്ഥിരം തർക്കം isSeqEvent = coverp && coverp->isSeqEvent();
        സ്ഥിരം VAssertType assertType
            = isSeqEvent ? VAssertType{VAssertType::INTERNAL} : assertp->userType();
        സ്ഥിരം VAssertDirectiveType directiveType
            = isSeqEvent ? VAssertDirectiveType{VAssertDirectiveType::INTERNAL}
                         : assertp->directive();
        സ്ഥിരം std::string baseName = m_names.get("");

        // Number vertices with sequential colors for array indexing.
        പൂർണ്ണസംഖ്യ N = 0;
        വേണ്ടി (V3GraphVertex& vtxr : graph.m_graph.vertices()) { vtxr.color(N++); }
        std::vector<SvaStateVertex*> vtx(N, nullptr);
        വേണ്ടി (V3GraphVertex& vtxr : graph.m_graph.vertices()) {
            vtx[vtxr.color()] = static_cast<SvaStateVertex*>(&vtxr);
        }

        // Allocate per-vertex lowering data (stored via V3GraphVertex::userp()).
        std::vector<SvaVertexData> vertexData(N);
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < N; ++i) vtx[i]->userp(&vertexData[i]);

        // Identify registered vertices (targets of clocked edges).
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < N; ++i) {
            വേണ്ടി (സ്ഥിരം V3GraphEdge& edger : vtx[i]->outEdges()) {
                സ്ഥിരം SvaTransEdge& tedger = static_cast<സ്ഥിരം SvaTransEdge&>(edger);
                സ്ഥിരം പൂർണ്ണസംഖ്യ toIdx = tedger.toVtxp()->color();
                എങ്കിൽ (tedger.m_consumesCycle && tedger.toVtxp() != graph.m_matchVertexp
                    && !tedger.toVtxp()->m_isRejectSink) {
                    vtx[toIdx]->datap()->needsReg = സത്യം;
                }
            }
        }

        // A packed range ring can emit several live endpoints without needing per-slot counts.
        // Widen only a ring whose input can already carry multiplicity.
        std::vector<തർക്കം> stateMayHaveMultiplicity(N);
        std::vector<തർക്കം> ringNeedsMultiplicity(N);
        std::vector<പൂർണ്ണസംഖ്യ> multiplicityWork;
        സ്ഥിരം തർക്കം എണ്ണം_Matches = coverp && coverp->isCoverSeq() && !isSeqEvent;
        എങ്കിൽ (എണ്ണം_Matches) {
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < N; ++i) {
                സ്ഥിരം തർക്കം multipleInputs = vtx[i]->inEdges().size() > 1;
                സ്ഥിരം തർക്കം isRing = vtx[i]->m_delayRingSize;
                ringNeedsMultiplicity[i] = isRing && multipleInputs;
                സ്ഥിരം തർക്കം startsMultiplicity = isRing
                                                    ? !vtx[i]->m_isFixedDelayRing
                                                    : multipleInputs && !vtx[i]->datap()->needsReg;
                എങ്കിൽ (startsMultiplicity) {
                    stateMayHaveMultiplicity[i] = സത്യം;
                    multiplicityWork.തള്ളുക_back(i);
                }
            }
            വേണ്ടി (size_t wi = 0; wi < multiplicityWork.size(); ++wi) {
                വേണ്ടി (സ്ഥിരം V3GraphEdge& edger : vtx[multiplicityWork[wi]]->outEdges()) {
                    സ്ഥിരം SvaTransEdge& tedger = static_cast<സ്ഥിരം SvaTransEdge&>(edger);
                    സ്ഥിരം പൂർണ്ണസംഖ്യ ti = tedger.toVtxp()->color();
                    സ്ഥിരം തർക്കം isRing = vtx[ti]->m_delayRingSize;
                    എങ്കിൽ (isRing) ringNeedsMultiplicity[ti] = സത്യം;
                    // Match and reject sinks have no outgoing edges, so they stop naturally.
                    സ്ഥിരം തർക്കം stopsMultiplicity = !isRing && vtx[ti]->datap()->needsReg;
                    എങ്കിൽ (stateMayHaveMultiplicity[ti] || stopsMultiplicity) { തുടരുക; }
                    stateMayHaveMultiplicity[ti] = സത്യം;
                    multiplicityWork.തള്ളുക_back(ti);
                }
            }
        }

        സ്ഥിരം തർക്കം asyncDisable = isAsyncDisable(disableExprp);
        // Registers cleared on a rising disable are also written by their clocked block.
        File_വരി* സ്ഥിരം regFlp = asyncDisable ? പുതിയ File_വരി{flp} : flp;
        എങ്കിൽ (asyncDisable) regFlp->warnOff(V3ErrorCode::MULTIDRIVEN, സത്യം);

        AstVar* സ്ഥിരം killVarp
            = പുതിയ AstVar{flp, VVarType::MODULETEMP, baseName + "__kill", m_u32DTypep};
        killVarp->lifetime(VLifetime::STATIC_EXPLICIT);
        m_modp->addStmtsp(killVarp);
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < N; ++i) {
            എങ്കിൽ (vtx[i]->m_isAndCombiner) {
                സ്ഥിരം std::string base = baseName + "__a" + std::to_string(i);
                AstVar* സ്ഥിരം lp = പുതിയ AstVar{regFlp, VVarType::MODULETEMP, base + "_doneL",
                                              m_modp->findBitDType()};
                lp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(lp);
                vtx[i]->datap()->doneLVarp = lp;
                AstVar* സ്ഥിരം rp = പുതിയ AstVar{regFlp, VVarType::MODULETEMP, base + "_doneR",
                                              m_modp->findBitDType()};
                rp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(rp);
                vtx[i]->datap()->doneRVarp = rp;
                തുടരുക;
            }
            എങ്കിൽ (vtx[i]->m_delayRingSize) {
                സ്ഥിരം std::string base = baseName + "__d" + std::to_string(i);
                സ്ഥിരം തർക്കം storesCounts = എണ്ണം_Matches && ringNeedsMultiplicity[i];
                Ast_കണ്ണി_DType* ringDTypep = nullptr;
                എങ്കിൽ (storesCounts) {
                    // int unsigned ring [size];
                    ringDTypep = പുതിയ AstUnpackArrayDType{
                        flp, m_u32DTypep,
                        പുതിയ AstRange{flp, static_cast<പൂർണ്ണസംഖ്യ>(vtx[i]->m_delayRingSize - 1), 0}};
                    v3Global.rootp()->typeTablep()->addTypesp(ringDTypep);
                } അല്ലെങ്കിൽ {
                    // bit [size-1:0] ring;
                    ringDTypep = m_modp->findBitDType(vtx[i]->m_delayRingSize,
                                                      vtx[i]->m_delayRingSize, VSigning::UNSIGNED);
                }
                AstVar* സ്ഥിരം ringp
                    = പുതിയ AstVar{flp, VVarType::MODULETEMP, base + "_ring", ringDTypep};
                ringp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(ringp);
                vtx[i]->datap()->delayRingVarp = ringp;
                vtx[i]->datap()->delayRingCounts = storesCounts;
                AstVar* സ്ഥിരം headp
                    = പുതിയ AstVar{flp, VVarType::MODULETEMP, base + "_head",
                                 storesCounts ? m_u32DTypep : m_modp->findBitDType()};
                headp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(headp);
                vtx[i]->datap()->delayRingHeadVarp = headp;
                // int unsigned idx;
                AstVar* സ്ഥിരം idxp
                    = പുതിയ AstVar{regFlp, VVarType::MODULETEMP, base + "_idx", m_u32DTypep};
                idxp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(idxp);
                vtx[i]->datap()->delayRingIdxVarp = idxp;
                // int unsigned live_count;
                AstVar* സ്ഥിരം live_എണ്ണം_Varp
                    = പുതിയ AstVar{regFlp, VVarType::MODULETEMP, base + "_liveCount", m_u32DTypep};
                live_എണ്ണം_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(live_എണ്ണം_Varp);
                vtx[i]->datap()->delay_Ring_Live_എണ്ണം_Varp = live_എണ്ണം_Varp;
                AstVar* സ്ഥിരം wrappedp = പുതിയ AstVar{regFlp, VVarType::MODULETEMP,
                                                    base + "_wrapped", m_modp->findBitDType()};
                wrappedp->lifetime(VLifetime::STATIC_EXPLICIT);
                m_modp->addStmtsp(wrappedp);
                vtx[i]->datap()->delayRingWrappedVarp = wrappedp;
                തുടരുക;
            }
            എങ്കിൽ (!vtx[i]->datap()->needsReg) തുടരുക;
            എങ്കിൽ (vtx[i] == graph.m_startVertexp || vtx[i]->m_isMatch) തുടരുക;
            സ്ഥിരം std::string varName = baseName + "__s" + std::to_string(i);
            AstVar* സ്ഥിരം varp
                = പുതിയ AstVar{regFlp, VVarType::MODULETEMP, varName, m_modp->findBitDType()};
            varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(varp);
            vtx[i]->datap()->stateVarp = varp;
        }

        // Build lowering context for phase sub-functions.
        LowerCtx c{flp,        senTreep,      disableExprp, matchCondp,
                   assertType, directiveType, killVarp,     graph};

        // Phase 1: Resolve combinational Links via fixed-point propagation.
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം triggerExprp
            = isSeqEvent ? പുതിയ AstConst{flp, AstConst::BitTrue{}}
                         : assertOnCond(flp, assertp->userType(), assertp->directive());
        resolveLinks(c, triggerExprp, എണ്ണം_Matches);
        VL_DO_DANGLING(triggerExprp->delete_വൃക്ഷം(), triggerExprp);

        // Phase 2/2b/2c: Emit NBA register updates and in-place delay-ring writes.
        emitStateRegisterNba(c);
        emitDelayRingUpdates(c, എണ്ണം_Matches);
        emitAndCombinerDoneLatchNba(c);
        emitKillAckNba(c);

        // Phase 3/3a/3b: Compute terminal match/reject signals (cleans up stateSig).
        സ്ഥിരം SignalSet sigs
            = computeSignals(c, need_Thread_Fail_എണ്ണം, need_Throughout_Thread_Fail_എണ്ണം, എണ്ണം_Matches);

        // Strong s_always[m:n] end-of-simulation liveness: if any in-window state
        // is still set at $finish, the universal-quantifier window never completed
        // (IEEE 1800-2023 16.12.11 strong semantics). Fire the assertion failure
        // from a final block; V3Assert turns the DT_ERROR display into the standard
        // "Assertion failed in %m" message.
        // pending = |{strong state registers, strong delay live counts != 0};
        Ast_കണ്ണി_വ്യഞ്ജകം* pendingp = nullptr;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < N; ++i) {
            എങ്കിൽ (!vtx[i]->m_strongPending) തുടരുക;
            Ast_കണ്ണി_വ്യഞ്ജകം* pendingExprp = nullptr;
            എങ്കിൽ (vtx[i]->datap()->stateVarp) {
                pendingExprp = പുതിയ AstVarRef{flp, vtx[i]->datap()->stateVarp, VAccess::READ};
            } അല്ലെങ്കിൽ എങ്കിൽ (vtx[i]->m_strongAlwaysRing) {
                pendingExprp
                    = delayRingHasLiveThreadsp(flp, vtx[i]->datap()->delay_Ring_Live_എണ്ണം_Varp);
            } അല്ലെങ്കിൽ {
                തുടരുക;
            }
            എങ്കിൽ (!pendingp) {
                pendingp = pendingExprp;
            } അല്ലെങ്കിൽ {
                pendingp = പുതിയ AstLogOr{flp, pendingp, pendingExprp};
            }
        }
        എങ്കിൽ (pendingp) {
            AstCExpr* സ്ഥിരം assertOnp
                = പുതിയ AstCExpr{flp, AstCExpr::Pure{}, "vlSymsp->_vm_contextp__->assertOn()", 1};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstLogAnd{flp, assertOnp, pendingp};
            AstDisplay* സ്ഥിരം dispp
                = പുതിയ AstDisplay{flp, VDisplayType::DT_ERROR, "", nullptr, nullptr};
            dispp->fmtp()->timeunit(m_modp->timeunit());
            Ast_കണ്ണി_Stmt* സ്ഥിരം firep = dispp;
            എങ്കിൽ (v3Global.opt.stopFail()) firep->addNext(പുതിയ AstStop{flp, അസത്യം});
            m_modp->addStmtsp(പുതിയ AstFinal{flp, പുതിയ AstIf{flp, condp, firep}});
        }

        // Clear userp on every vertex before vertexData is destroyed.
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < N; ++i) vtx[i]->userp(nullptr);
        തിരികെ sigs;
    }
};

}  // namespace

//######################################################################
// Top-level visitor

വർഗ്ഗം AssertNfaVisitor final : പൊതു VNVisitor {
    // STATE
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module being processed
    AstClocking* m_defaultClockingp = nullptr;  // Default clocking
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff
    SvaNfaLowering* m_loweringp = nullptr;  // NFA-to-hardware lowering engine
    Ast_Sen_വൃക്ഷം* m_sampled_മൂല്യം_Clockp = nullptr;  // Inherited clock during scoped attachment
    V3UniqueNames m_propVarNames{"__Vpropvar"};  // Property-local variable names
    V3UniqueNames m_propTempNames{"__VnfaSampled"};  // Hoisted $sampled(propp) temps
    V3UniqueNames m_fail_എണ്ണം_Names{"__VnfaRemainingFailCount"};  // Fail replay counter names
    std::set<സ്ഥിരം AstProperty*> m_inliningProps;  // Recursion guard for inlineNamedProperty

    template <typename T_കണ്ണി>
    ശൂന്യം visit_Sampled_മൂല്യം(T_കണ്ണി* സ്ഥിരം nodep) {
        എങ്കിൽ (m_sampled_മൂല്യം_Clockp && !nodep->sentreep()) {
            nodep->sentreep(m_sampled_മൂല്യം_Clockp->clone_വൃക്ഷം(സത്യം));
        }
        iterateChildren(nodep);
    }

    // Wire match vertex and mid-window sources for a successful NFA build.
    സ്ഥിതം ശൂന്യം wireMatchAndMidSources(SvaGraph& graph, സ്ഥിരം BuildResult& result, File_വരി* flp) {
        graph.createMatchVertex();
        // Skip the main term Link when midSources already cover every
        // end-of-match (cover_sequence path); otherwise the per-mid extraction
        // double-counts via the merge vertex.
        എങ്കിൽ (!result.termIsMidMerge) { graph.addLink(result.termVertexp, graph.m_matchVertexp); }
        വേണ്ടി (SvaStateVertex* srcVtxp : result.midSources) {
            Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം tc : srcVtxp->m_throughoutConds) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം tcClone = tc->clone_വൃക്ഷം_Pure(അസത്യം);
                condp = condp ? പുതിയ AstLogAnd{flp, condp, tcClone} : tcClone;
            }
            graph.addLink(srcVtxp, graph.m_matchVertexp, condp);
            srcVtxp->m_isUnbounded = സത്യം;
        }
    }

    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* getSequenceBodyExprp(സ്ഥിരം AstSequence* seqp) {
        Ast_കണ്ണി* bodyp = seqp->stmtsp();
        ആകുമ്പോൾ (bodyp && VN_IS(bodyp, Var)) bodyp = bodyp->nextp();
        തിരികെ VN_CAST(bodyp, കണ്ണി_വ്യഞ്ജകം);
    }

    സ്ഥിതം AstPropSpec* getPropertySpecp(സ്ഥിരം AstProperty* propp) {
        Ast_കണ്ണി* stmtp = propp->stmtsp();
        // V3LinkParse emits InitialStaticStmt for property-local variable
        // initialisers; the InitialAutomaticStmt variant only appears for
        // task/function-scope automatic lifetime, not properties.
        ആകുമ്പോൾ (stmtp
               && (VN_IS(stmtp, Var) || VN_IS(stmtp, InitialStaticStmt)
                   || VN_IS(stmtp, InitialAutomaticStmt))) {  // LCOV_EXCL_LINE
            stmtp = stmtp->nextp();
        }
        തിരികെ VN_CAST(stmtp, PropSpec);
    }

    ശൂന്യം inlineNamedProperty(AstPropSpec* outerSpecp, AstFuncRef* funcrefp,
                             സ്ഥിരം AstProperty* propyp) {
        // Recursion guard: IEEE 1800-2023 16.12.1 forbids recursive properties.
        // V3Width emits "Recursive property call" for direct recursion before this
        // pass runs; this catches any nested-inlining cycle that slips past.
        എങ്കിൽ (!m_inliningProps.emplace(propyp).second) {
            funcrefp->v3error("Illegal recursive property reference");  // LCOV_EXCL_LINE
            തിരികെ;  // LCOV_EXCL_LINE
        }
        ഘടന Guard final {
            std::set<സ്ഥിരം AstProperty*>& setr;
            സ്ഥിരം AstProperty* keyp;
            ~Guard() { setr.erase(keyp); }
        } guard{m_inliningProps, propyp};
        AstPropSpec* propSpecp = getPropertySpecp(propyp);
        UASSERT_OBJ(propSpecp, funcrefp, "Property has no body PropSpec");
        propSpecp = propSpecp->clone_വൃക്ഷം(അസത്യം);

        സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, propyp->stmtsp());
        std::unordered_map<സ്ഥിരം AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*> portMap;
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }

        // Promote property-local variables to module-level temps (IEEE 16.10).
        std::unordered_map<സ്ഥിരം AstVar*, AstVar*> localVarMap;
        വേണ്ടി (Ast_കണ്ണി* stmtp = propyp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (!varp->isIO()) {
                    സ്ഥിരം string newName = m_propVarNames.get(varp);
                    AstVar* സ്ഥിരം newVarp = പുതിയ AstVar{varp->fileline(), VVarType::MODULETEMP,
                                                       newName, varp->dtypep()};
                    newVarp->lifetime(VLifetime::STATIC_EXPLICIT);
                    m_modp->addStmtsp(newVarp);
                    localVarMap[varp] = newVarp;
                }
            }
        }

        propSpecp->foreach([&](AstVarRef* refp) {
            സ്ഥിരം auto portIt = portMap.find(refp->varp());
            എങ്കിൽ (portIt != portMap.end()) {
                refp->replaceWith(portIt->second->clone_വൃക്ഷം(അസത്യം));
                VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
                തിരികെ;
            }
            സ്ഥിരം auto localIt = localVarMap.find(refp->varp());
            എങ്കിൽ (localIt != localVarMap.end()) refp->varp(localIt->second);
        });  // LCOV_EXCL_LINE -- gcov attributes lambda's implicit return to `})`

        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            തള്ളുക_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(propSpecp, funcrefp);

        // Merge disable iff (IEEE 1800-2023 16.12.1)
        എങ്കിൽ (outerSpecp->disablep() && propSpecp->disablep()) {
            outerSpecp->v3error("disable iff expression before property call "
                                "and in its body is not legal");
            തള്ളുക_Deletep(propSpecp->disablep()->unlinkFrBack());
        }
        എങ്കിൽ (outerSpecp->disablep()) {
            propSpecp->disablep(outerSpecp->disablep()->unlinkFrBack());
        }

        എങ്കിൽ (outerSpecp->sensesp() && propSpecp->sensesp()) {
            outerSpecp->v3warn(E_UNSUPPORTED,
                               "Unsupported: Clock event before property call and in its body");
            തള്ളുക_Deletep(propSpecp->sensesp()->unlinkFrBack());
        }
        എങ്കിൽ (outerSpecp->sensesp()) {
            AstSenItem* സ്ഥിരം sensesp = outerSpecp->sensesp();
            sensesp->unlinkFrBack();
            propSpecp->sensesp(sensesp);
        }

        outerSpecp->replaceWith(propSpecp);
        VL_DO_DANGLING(തള്ളുക_Deletep(outerSpecp), outerSpecp);
    }

    ശൂന്യം inlineSequenceRef(AstFuncRef* funcrefp, AstSequence* seqp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bodyExprp = getSequenceBodyExprp(seqp);
        UASSERT_OBJ(bodyExprp, funcrefp, "Sequence has no body expression");
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം clonedp = bodyExprp->clone_വൃക്ഷം(അസത്യം);

        സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(funcrefp, seqp->stmtsp());
        std::unordered_map<സ്ഥിരം AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*> portMap;
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            portMap[tconnect.first] = tconnect.second->exprp();
        }
        clonedp->foreach([&](AstVarRef* refp) {
            സ്ഥിരം auto it = portMap.find(refp->varp());
            എങ്കിൽ (it != portMap.end()) {
                refp->replaceWith(it->second->clone_വൃക്ഷം(അസത്യം));
                VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
            }
        });
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            തള്ളുക_Deletep(tconnect.second->exprp()->unlinkFrBack());
        }
        V3AssertCommon::checkSvaInstanceDelays(clonedp, funcrefp);
        funcrefp->replaceWith(clonedp);
        VL_DO_DANGLING(തള്ളുക_Deletep(funcrefp), funcrefp);
        // Clear referenced flag so V3AssertPre cleanup does not emit
        // spurious UNSUPPORTED for sequences that were already inlined here.
        seqp->isReferenced(അസത്യം);
    }

    // Must run before hasMultiCycleExpr() so NFA sees sequence bodies.
    ശൂന്യം inlineAllSequenceRefs(Ast_കണ്ണി* rootp) {
        തർക്കം changed = സത്യം;
        ആകുമ്പോൾ (changed) {
            changed = അസത്യം;
            rootp->foreach([&](AstFuncRef* funcrefp) {
                എങ്കിൽ (changed) തിരികെ;
                എങ്കിൽ (AstSequence* സ്ഥിരം seqp = VN_CAST(funcrefp->taskp(), Sequence)) {
                    inlineSequenceRef(funcrefp, seqp);
                    changed = സത്യം;
                }
            });
        }
    }

    സ്ഥിതം തർക്കം has_Multi_Cycle_വ്യഞ്ജകം(സ്ഥിരം Ast_കണ്ണി* nodep) {
        തിരികെ nodep->exists([](സ്ഥിരം Ast_കണ്ണി* np) {
            എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം ep = VN_CAST(np, കണ്ണി_വ്യഞ്ജകം)) തിരികെ ep->isMultiCycleSva();
            തിരികെ അസത്യം;
        });
    }

    സ്ഥിതം VPropStrength effectiveAssertPropStrength(സ്ഥിരം AstPropSpec* സ്ഥിരം propSpecp) {
        എങ്കിൽ (propSpecp->propStrength() != VPropStrength::DEFAULT) തിരികെ propSpecp->propStrength();
        തിരികെ propSpecp->fileline()->language() <= V3LangCode::L1800_2005 ? VPropStrength::STRONG
                                                                           : VPropStrength::WEAK;
    }

    // Bare `assert property (p until q)` with boolean operands stays on
    // V3AssertPre's AstLoop lowering, which preserves per-attempt action-block
    // firings that this NFA's single-bit aggregated state cannot. Strong bare
    // forms are also lowered there. NFA still owns sequence operands and any
    // embedding inside a multi-cycle context (implication consequent, or/and
    // operands, etc.).
    സ്ഥിതം തർക്കം is_Bare_മുകൾ_Level_Until(Ast_കണ്ണി* propp) {
        Ast_കണ്ണി* p = propp;
        എങ്കിൽ (AstPropSpec* സ്ഥിരം specp = VN_CAST(p, PropSpec)) p = specp->propp();
        ആകുമ്പോൾ (AstLogNot* സ്ഥിരം notp = VN_CAST(p, LogNot)) p = notp->lhsp();
        AstUntil* സ്ഥിരം untilp = VN_CAST(p, Until);
        എങ്കിൽ (!untilp) തിരികെ അസത്യം;
        സ്ഥിരം auto hasSeq = [](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* ep) {
            തിരികെ ep->exists([](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* np) { തിരികെ np->isMultiCycleSva(); });
        };
        എങ്കിൽ (hasSeq(untilp->lhsp()) || hasSeq(untilp->rhsp())) തിരികെ അസത്യം;
        തിരികെ സത്യം;
    }

    ഘടന PropertyParts final {
        Ast_കണ്ണി_വ്യഞ്ജകം* triggerExprp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* seqExprp = nullptr;
        തർക്കം isOverlapped = സത്യം;
        തർക്കം hasImplication = അസത്യം;
        തർക്കം isFollowedBy = അസത്യം;  // True for #-# / #=# (non-vacuous-fail on antecedent miss)
    };

    സ്ഥിതം PropertyParts decomposeProperty(Ast_കണ്ണി* propp) {
        PropertyParts parts;
        എങ്കിൽ (AstPropSpec* സ്ഥിരം specp = VN_CAST(propp, PropSpec)) { propp = specp->propp(); }
        എങ്കിൽ (AstImplication* സ്ഥിരം implp = VN_CAST(propp, Implication)) {
            parts.hasImplication = സത്യം;
            parts.isOverlapped = implp->isOverlapped();
            parts.isFollowedBy = implp->isFollowedBy();
            parts.triggerExprp = implp->lhsp();
            parts.seqExprp = implp->rhsp();
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_CAST(propp, കണ്ണി_വ്യഞ്ജകം)) {
            parts.triggerExprp = nullptr;
            parts.seqExprp = exprp;
        }
        തിരികെ parts;
    }

    സ്ഥിതം തർക്കം canSplitImplicationPassActions(സ്ഥിരം PropertyParts& parts) {
        UASSERT(parts.hasImplication,
                "Implication pass action split requested without implication");
        UASSERT(parts.triggerExprp, "Implication pass action split requested without trigger");
        // Direct vacuous/nonvacuous classification uses the antecedent value in the current
        // assertion attempt. Leave delayed antecedents on the existing NFA pass path.
        തിരികെ !has_Multi_Cycle_വ്യഞ്ജകം(parts.triggerExprp);
    }

    സ്ഥിതം ശൂന്യം splitImplicationPassActions(AstAssert* assertp, സ്ഥിരം PropertyParts& parts,
                                            Ast_കണ്ണി_വ്യഞ്ജകം* nonvacuousMatchp) {
        File_വരി* സ്ഥിരം flp = assertp->fileline();

        Ast_കണ്ണി* സ്ഥിരം passsp = assertp->passsp()->unlinkFrBackWithNext();
        Ast_കണ്ണി* splitsp = nullptr;

        എങ്കിൽ (!parts.isFollowedBy) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം vacuousp
                = പുതിയ AstLogNot{flp, sampled(parts.triggerExprp->clone_വൃക്ഷം_Pure(അസത്യം))};
            Ast_കണ്ണി* സ്ഥിരം vacuousBodyp = passsp->clone_വൃക്ഷം(അസത്യം);
            splitsp = newPassOnIf(flp, vacuousp, vacuousBodyp, assertp->userType(),
                                  assertp->directive(), /*vacuous=*/സത്യം);
        }

        AstIf* സ്ഥിരം nonvacuousp
            = newPassOnIf(flp, nonvacuousMatchp, passsp, assertp->userType(), assertp->directive(),
                          /*vacuous=*/അസത്യം);
        splitsp = splitsp ? Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(splitsp, nonvacuousp)
                          : static_cast<Ast_കണ്ണി*>(nonvacuousp);
        assertp->addPasssp(splitsp);
    }

    // On a PropSpec-wrapped assertion whose NFA build failed with a semantic
    // error (errorEmitted), replace the body with a BitFalse const so later
    // passes see a well-formed AST. Returns true if replaced.
    ശൂന്യം replaceBodyOnBuildError(File_വരി* flp, AstPropSpec* propSpecp, തർക്കം errorEmitted) {
        എങ്കിൽ (!errorEmitted) തിരികെ;
        Ast_കണ്ണി* സ്ഥിരം innerPropp = propSpecp->propp();
        innerPropp->replaceWith(പുതിയ AstConst{flp, AstConst::BitFalse{}});
        VL_DO_DANGLING(തള്ളുക_Deletep(innerPropp), innerPropp);
    }

    // Hoist a leading clocking event (IEEE 1800-2023 16.7):
    തർക്കം hoistClockedSeq(AstPropSpec* specp) {
        ആകുമ്പോൾ (AstSClocked* സ്ഥിരം clockedp = VN_CAST(specp->propp(), SClocked)) {
            എങ്കിൽ (specp->sensesp()) {
                clockedp->v3warn(E_UNSUPPORTED, "Unsupported: multiclocked sequence or property");
                replaceBodyOnBuildError(specp->fileline(), specp, സത്യം);
                തിരികെ സത്യം;
            }
            വേണ്ടി (സ്ഥിരം AstSenItem* sp = clockedp->sensesp(); sp;
                 sp = VN_CAST(sp->nextp(), SenItem)) {
                എങ്കിൽ (!sp->edgeType().anEdge()) {
                    clockedp->v3warn(E_UNSUPPORTED,
                                     "Unsupported: non-edge clocking event on a sequence; "
                                     "use an edge such as @(posedge clk)");
                    replaceBodyOnBuildError(specp->fileline(), specp, സത്യം);
                    തിരികെ സത്യം;
                }
            }
            specp->sensesp(clockedp->sensesp()->unlinkFrBackWithNext());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bodyp = clockedp->exprp()->unlinkFrBack();
            clockedp->replaceWith(bodyp);
            VL_DO_DANGLING(തള്ളുക_Deletep(clockedp), clockedp);
        }
        // A clocking event anywhere else in the sequence is not supported.
        സ്ഥിരം AstSClocked* nestedp = nullptr;
        specp->propp()->foreach([&](സ്ഥിരം AstSClocked* p) {
            എങ്കിൽ (!nestedp) nestedp = p;
        });
        എങ്കിൽ (nestedp) {
            nestedp->v3warn(E_UNSUPPORTED,
                            "Unsupported: clocking event inside sequence expression");
            replaceBodyOnBuildError(specp->fileline(), specp, സത്യം);
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // Build the NFA graph for a property body, handling both the antecedent
    // |-> consequent and simple sequence cases. Returns the consequent/body
    // BuildResult (invalid on parse/build failure).
    BuildResult buildAssertionGraph(SvaNfaBuilder& builder, SvaGraph& graph, Ast_കണ്ണി_വ്യഞ്ജകം* seqBodyp,
                                    സ്ഥിരം PropertyParts& parts, File_വരി* flp) {
        എങ്കിൽ (!parts.hasImplication) തിരികെ builder.build(seqBodyp);

        graph.m_startVertexp = graph.createStateVertex();
        തിരികെ builder.buildImplicationEdges(parts.triggerExprp, seqBodyp, graph.m_startVertexp,
                                             parts.isOverlapped, parts.isFollowedBy,
                                             parts.triggerExprp, flp);
    }

    // Install pass-action gating and replay simultaneous per-thread failures.
    ശൂന്യം attachActionHandlers(AstAssert* സ്ഥിരം assertp, Ast_കണ്ണി_വ്യഞ്ജകം* matchExprp,
                              Ast_Sen_വൃക്ഷം* സ്ഥിരം threadFailReplaySenTreep,
                              Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം threadFailCountp) {
        // Gate pass handler on match to prevent vacuous-pass firings.
        എങ്കിൽ (matchExprp) {
            // needMatch implies passsp() was non-null when evaluated above;
            // lowering does not mutate the assert's pass-action between the
            // two reads, so passsp() is still non-null here.
            Ast_കണ്ണി* passsp = assertp->passsp();
            UASSERT_OBJ(passsp, assertp, "needMatch set but passsp is null");
            passsp->unlinkFrBackWithNext();
            File_വരി* സ്ഥിരം flp = assertp->fileline();
            AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, matchExprp, passsp, nullptr};
            assertp->addPasssp(ifp);
            // Fail-handler prefix for overlapping instances (IEEE 16.12):
            // fires when reject=1 && match=1 in the same cycle.
            എങ്കിൽ (assertp->failsp()) assertp->prependFailsp(ifp->clone_വൃക്ഷം(അസത്യം));
        }

        എങ്കിൽ (threadFailCountp) {
            UASSERT_OBJ(threadFailReplaySenTreep, assertp,
                        "Thread fail count missing sensitivity tree");
            Ast_കണ്ണി* സ്ഥിരം failsp = assertp->failsp();
            File_വരി* സ്ഥിരം flp = assertp->fileline();
            // IEEE 1800-2023 16.12 requires one action-block evaluation per failed
            // thread. AstAssert handles the first, so replay the rest here.
            AstVar* സ്ഥിരം remaining_Fail_എണ്ണം_Varp
                = പുതിയ AstVar{flp, VVarType::MODULETEMP, m_fail_എണ്ണം_Names.get(""),
                             m_modp->findBasicDType(VBasicDTypeKwd::UINT32)};
            remaining_Fail_എണ്ണം_Varp->lifetime(VLifetime::STATIC_EXPLICIT);
            m_modp->addStmtsp(remaining_Fail_എണ്ണം_Varp);
            Ast_കണ്ണി* സ്ഥിരം replayStmtsp = പുതിയ AstAssign{
                flp, പുതിയ AstVarRef{flp, remaining_Fail_എണ്ണം_Varp, VAccess::WRITE}, threadFailCountp};
            AstLoop* സ്ഥിരം replayLoopp = പുതിയ AstLoop{flp};
            replayLoopp->addStmtsp(പുതിയ AstLoopTest{
                flp, replayLoopp,
                പുതിയ AstGt{flp, പുതിയ AstVarRef{flp, remaining_Fail_എണ്ണം_Varp, VAccess::READ},
                          newTypedConstp(flp, remaining_Fail_എണ്ണം_Varp->dtypep(), 1)}});
            replayLoopp->addStmtsp(newIfAssertFailOn(failsp->clone_വൃക്ഷം(സത്യം), assertp->directive(),
                                                     assertp->userType()));
            AstSub* സ്ഥിരം decrementedFailCountp
                = പുതിയ AstSub{flp, പുതിയ AstVarRef{flp, remaining_Fail_എണ്ണം_Varp, VAccess::READ},
                             newTypedConstp(flp, remaining_Fail_എണ്ണം_Varp->dtypep(), 1)};
            replayLoopp->addStmtsp(
                പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, remaining_Fail_എണ്ണം_Varp, VAccess::WRITE},
                              decrementedFailCountp});
            replayStmtsp->addNext(replayLoopp);
            m_modp->addStmtsp(
                പുതിയ AstAlways{flp, VAlwaysKwd::ALWAYS, threadFailReplaySenTreep, replayStmtsp});
        }
    }

    // Replace one VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). No-op if refp is not in matchMap.
    ശൂന്യം substituteMatchItemRef(AstVarRef* refp, unsigned K,
                                സ്ഥിരം std::unordered_map<സ്ഥിരം AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*>& matchMap) {
        സ്ഥിരം auto it = matchMap.find(refp->varp());
        എങ്കിൽ (it == matchMap.end()) തിരികെ;
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = it->second->clone_വൃക്ഷം_Pure(അസത്യം);
        എങ്കിൽ (K > 0) {
            AstConst* സ്ഥിരം ticksp = പുതിയ AstConst{refp->fileline(), AstConst::Widthed_മൂല്യം{}, 32,
                                                  static_cast<uint32_t>(K)};
            AstPast* സ്ഥിരം pastp = പുതിയ AstPast{refp->fileline(), newp, ticksp, nullptr,
                                               /* propertyTiming */ സത്യം};
            pastp->dtypeFrom(newp);
            newp = pastp;
        }
        refp->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
        തിരികെ;
    }

    // Recursively walk a consequent. Returns cycle length consumed and
    // substitutes each VarRef to a captured local var with $past(rhs, K)
    // (or rhs inline when K == 0). Reports E_UNSUPPORTED on non-constant
    // delays or composite sequence operators and returns -1 on failure.
    പൂർണ്ണസംഖ്യ
    walk_Substitute_Match_ഇനങ്ങൾ(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, unsigned K,
                             സ്ഥിരം std::unordered_map<സ്ഥിരം AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*>& match_ഇനങ്ങൾ) {
        എങ്കിൽ (AstSExpr* സ്ഥിരം sexprp = VN_CAST(nodep, SExpr)) {
            // IEEE 1800-2023 16.9.2: cycle_delay's lhsp is a constant_expression
            // and the delay form in a sequence is always `##N`, folded by
            // V3Const + V3Param before V3AssertNfa. Range form `##[m:n]` is the
            // only user-visible reject here.
            AstDelay* സ്ഥിരം delayp = VN_AS(sexprp->delayp(), Delay);
            UASSERT_OBJ(delayp->isCycleDelay() && VN_IS(delayp->lhsp(), Const), sexprp,
                        "SVA cycle delay must have a constant lhsp");
            എങ്കിൽ (delayp->isRangeDelay()) {
                sexprp->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                              "non-constant cycle delay in consequent"
                                              " (IEEE 1800-2023 16.10)");
                തിരികെ -1;
            }
            സ്ഥിരം unsigned delayCycles = VN_AS(delayp->lhsp(), Const)->toUInt();
            പൂർണ്ണസംഖ്യ preLen = 0;
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം prep = sexprp->preExprp()) {
                preLen = walk_Substitute_Match_ഇനങ്ങൾ(prep, K, match_ഇനങ്ങൾ);
                എങ്കിൽ (preLen < 0) തിരികെ -1;
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ bodyLen
                = walk_Substitute_Match_ഇനങ്ങൾ(sexprp->exprp(), K + preLen + delayCycles, match_ഇനങ്ങൾ);
            എങ്കിൽ (bodyLen < 0) തിരികെ -1;
            തിരികെ preLen + delayCycles + bodyLen;
        }
        എങ്കിൽ (nodep->isMultiCycleSva()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: property local variable used across "
                                         "composite sequence operator in consequent"
                                         " (IEEE 1800-2023 16.10)");
            തിരികെ -1;
        }
        std::vector<AstVarRef*> refs;
        nodep->foreach([&refs](AstVarRef* p) { refs.തള്ളുക_back(p); });
        വേണ്ടി (AstVarRef* സ്ഥിരം refp : refs) substituteMatchItemRef(refp, K, match_ഇനങ്ങൾ);
        തിരികെ 0;
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
    തർക്കം liftMatchItemSubstitutions(PropertyParts& parts, Ast_കണ്ണി_വ്യഞ്ജകം* seqBodyp) {
        എങ്കിൽ (!parts.hasImplication) തിരികെ അസത്യം;
        Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം വ്യഞ്ജകം_Stmtp = VN_CAST(parts.triggerExprp, വ്യഞ്ജകം_Stmt);
        എങ്കിൽ (!വ്യഞ്ജകം_Stmtp) തിരികെ അസത്യം;
        // IEEE 1800-2023 16.10 BNF requires `(expr, match_item {, match_item})`
        // with at least one match item; V3LinkParse only emits ExprStmt for
        // this form and only emits AstAssign with VarRef LHS for each item.
        std::unordered_map<സ്ഥിരം AstVar*, Ast_കണ്ണി_വ്യഞ്ജകം*> match_ഇനങ്ങൾ;
        വേണ്ടി (Ast_കണ്ണി* stmtp = വ്യഞ്ജകം_Stmtp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstAssign* സ്ഥിരം assignp = VN_AS(stmtp, Assign);
            AstVarRef* സ്ഥിരം lhsRefp = VN_AS(assignp->lhsp(), VarRef);
            match_ഇനങ്ങൾ[lhsRefp->varp()] = assignp->rhsp();
        }
        സ്ഥിരം unsigned startK = parts.isOverlapped ? 0 : 1;
        സ്ഥിരം പൂർണ്ണസംഖ്യ length = walk_Substitute_Match_ഇനങ്ങൾ(seqBodyp, startK, match_ഇനങ്ങൾ);
        // Match-item substitution / strip mutates ancestor purity. Release
        // builds don't auto-clear caches on edits, so refresh here.
        VIsCached::clear_Cache_വൃക്ഷം();
        എങ്കിൽ (length < 0) തിരികെ സത്യം;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം antBoolp = വ്യഞ്ജകം_Stmtp->resultp()->unlinkFrBack();
        വ്യഞ്ജകം_Stmtp->replaceWith(antBoolp);
        VL_DO_DANGLING(തള്ളുക_Deletep(വ്യഞ്ജകം_Stmtp), വ്യഞ്ജകം_Stmtp);
        parts.triggerExprp = antBoolp;
        തിരികെ അസത്യം;
    }

    // Outcome counts for a property if/case are wrong in an outcome-multiplying
    // context. Returns that context, or nullptr when the shape is supported.
    സ്ഥിതം സ്ഥിരം അക്ഷരം* unsupportedPropertyControl(സ്ഥിരം Ast_കണ്ണി_Cover_Or_Assert* assertp,
                                                  സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* seqBodyp, തർക്കം negated) {
        എങ്കിൽ (!hasPropertyControlConjunction(seqBodyp)) തിരികെ nullptr;
        എങ്കിൽ (negated) തിരികെ "negation";
        എങ്കിൽ (VN_IS(assertp, Cover)) തിരികെ "cover";
        എങ്കിൽ (VN_AS(assertp, Assert)->passsp()) തിരികെ "a pass action";
        തിരികെ nullptr;
    }

    // Inline property/sequence refs and reject unsupported shapes.
    // Returns the PropSpec to lower, or nullptr when fully handled here.
    AstPropSpec* prepareAssertionProp(Ast_കണ്ണി_Cover_Or_Assert* assertp) {
        എങ്കിൽ (AstPropSpec* സ്ഥിരം specp = VN_CAST(assertp->propp(), PropSpec)) {
            എങ്കിൽ (AstFuncRef* സ്ഥിരം funcrefp = VN_CAST(specp->propp(), FuncRef)) {
                എങ്കിൽ (സ്ഥിരം AstProperty* സ്ഥിരം propyp = VN_CAST(funcrefp->taskp(), Property)) {
                    inlineNamedProperty(specp, funcrefp, propyp);
                }
            }
        }

        inlineAllSequenceRefs(assertp->propp());

        എങ്കിൽ (AstPropSpec* സ്ഥിരം specp = VN_CAST(assertp->propp(), PropSpec)) {
            എങ്കിൽ (hoistClockedSeq(specp)) തിരികെ nullptr;
        }

        AstPropSpec* സ്ഥിരം propp = VN_AS(assertp->propp(), PropSpec);
        എങ്കിൽ (!VN_IS(assertp, Cover)
            && effectiveAssertPropStrength(propp) == VPropStrength::STRONG) {
            propp->v3warn(E_UNSUPPORTED,
                          "Unsupported: strong property in " + assertp->verilogKwd() + ".");
            replaceBodyOnBuildError(assertp->fileline(), propp, /*errorEmitted=*/സത്യം);
            തിരികെ nullptr;
        }

        എങ്കിൽ (!has_Multi_Cycle_വ്യഞ്ജകം(propp)) തിരികെ nullptr;
        // A nested property instance keeps its body behind the call; lowering would drop it.
        എങ്കിൽ (propp->exists([](സ്ഥിരം AstFuncRef* refp) { തിരികെ VN_IS(refp->taskp(), Property); })) {
            assertp->v3warn(E_UNSUPPORTED,
                            "Unsupported: property instance inside a multi-cycle property "
                            "expression");
            VL_DO_DANGLING(തള്ളുക_Deletep(assertp->unlinkFrBack()), assertp);
            തിരികെ nullptr;
        }
        എങ്കിൽ (is_Bare_മുകൾ_Level_Until(propp)) തിരികെ nullptr;
        തിരികെ propp;
    }

    ശൂന്യം processAssertion(Ast_കണ്ണി_Cover_Or_Assert* assertp) {
        എങ്കിൽ (assertp->immediate()) തിരികെ;

        AstPropSpec* സ്ഥിരം propp = prepareAssertionProp(assertp);
        എങ്കിൽ (!propp) തിരികെ;

        PropertyParts parts = decomposeProperty(propp);
        UASSERT_OBJ(parts.seqExprp, propp, "Property body must be an expression");

        // Unwrap `not` (IEEE 1800-2023 16.12.1); odd count -> negated semantics.
        Ast_കണ്ണി_വ്യഞ്ജകം* seqBodyp = parts.seqExprp;
        തർക്കം negated = അസത്യം;
        ആകുമ്പോൾ (AstLogNot* സ്ഥിരം notp = VN_CAST(seqBodyp, LogNot)) {
            negated = !negated;
            seqBodyp = notp->lhsp();
        }

        സ്ഥിരം അക്ഷരം* സ്ഥിരം propertyControlp
            = unsupportedPropertyControl(assertp, seqBodyp, negated);

        // Substitute property-local match-item refs in consequent with
        // $past(rhs, K) before NFA build (IEEE 1800-2023 16.10).
        എങ്കിൽ (liftMatchItemSubstitutions(parts, seqBodyp)) {
            AstPropSpec* സ്ഥിരം psp = VN_CAST(assertp->propp(), PropSpec);
            UASSERT_OBJ(psp, assertp, "Concurrent assertion must have PropSpec");
            replaceBodyOnBuildError(assertp->fileline(), psp, /*errorEmitted=*/സത്യം);
            തിരികെ;
        }

        AstCover* സ്ഥിരം coverp = VN_CAST(assertp, Cover);
        സ്ഥിരം തർക്കം isCoverSeq = coverp && coverp->isCoverSeq();
        // A sequence event control is not an assertion directive; no default
        // disable iff, no assertion control
        സ്ഥിരം തർക്കം isSeqEvent = coverp && coverp->isSeqEvent();
        സ്ഥിരം തർക്കം എണ്ണം_Matches = isCoverSeq && !isSeqEvent;
        // Inherit module defaults (IEEE 14.12, 16.15) when assertion has none.
        എങ്കിൽ (!propp->sensesp() && m_defaultClockingp) {
            propp->sensesp(m_defaultClockingp->sentreep()->sensesp()->clone_വൃക്ഷം(സത്യം));
        }
        എങ്കിൽ (!propp->disablep() && m_defaultDisablep && !isSeqEvent) {
            propp->disablep(m_defaultDisablep->condp()->clone_വൃക്ഷം_Pure(സത്യം));
        }
        എങ്കിൽ (!propp->sensesp()) തിരികെ;
        Ast_Sen_വൃക്ഷം* senTreep
            = പുതിയ Ast_Sen_വൃക്ഷം{propp->fileline(), propp->sensesp()->clone_വൃക്ഷം(സത്യം)};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം disableExprp = propp->disablep();

        // NFA lowering clones repeated operands and may hoist them into an
        // always_comb block. Resolve implicit sampled-value clocks first, while
        // the enclosing assertion clock is still available.
        {
            VL_RESTORER(m_sampled_മൂല്യം_Clockp);
            m_sampled_മൂല്യം_Clockp = senTreep;
            iterate(propp->propp());
        }

        File_വരി* സ്ഥിരം flp = assertp->fileline();

        SvaGraph graph;
        SvaNfaBuilder builder{graph,      m_modp,     m_propTempNames,
                              isCoverSeq, isSeqEvent, coverp != nullptr};

        സ്ഥിരം BuildResult result = buildAssertionGraph(builder, graph, seqBodyp, parts, flp);
        എങ്കിൽ (result.valid()) wireMatchAndMidSources(graph, result, flp);
        എങ്കിൽ (!result.valid()) {
            // Fall through to V3AssertPre for unsupported constructs; only
            // replace the body on real semantic errors. Any hoisted temps
            // from this attempt become orphan MODULETEMPs; V3Dead removes
            // them along with the dead always_comb driver.
            replaceBodyOnBuildError(flp, propp, result.errorEmitted);
            VL_DO_DANGLING(തള്ളുക_Deletep(senTreep), senTreep);
            തിരികെ;
        }
        // After the build, so a construct the builder rejects reports itself.
        എങ്കിൽ (propertyControlp) {
            seqBodyp->v3warn(E_UNSUPPORTED,
                             "Unsupported: temporal property if/case with " << propertyControlp);
            replaceBodyOnBuildError(flp, propp, /*errorEmitted=*/സത്യം);
            VL_DO_DANGLING(തള്ളുക_Deletep(senTreep), senTreep);
            തിരികെ;
        }
        എങ്കിൽ (എണ്ണം_Matches && !v3Global.opt.coverageUser()) {
            SvaNfaBuilder::freeUnlinkedCondp(result.finalCondp);
            VL_DO_DANGLING(തള്ളുക_Deletep(senTreep), senTreep);
            Ast_കണ്ണി* സ്ഥിരം innerPropp = propp->propp();
            innerPropp->replaceWith(പുതിയ AstConst{flp, AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(innerPropp), innerPropp);
            തിരികെ;
        }

        AstAssert* സ്ഥിരം assertAssertp = VN_CAST(assertp, Assert);
        സ്ഥിരം തർക്കം splitImplicationPasssp = assertAssertp && assertAssertp->passsp()
                                            && parts.hasImplication
                                            && canSplitImplicationPassActions(parts);
        സ്ഥിരം തർക്കം needMatch = assertAssertp && assertAssertp->passsp()
                               && (!parts.hasImplication || splitImplicationPasssp);

        സ്ഥിരം തർക്കം needsThreadFailReplay
            = assertAssertp && assertAssertp->failsp() && !parts.hasImplication;
        സ്ഥിരം auto signals
            = m_loweringp->lower(assertp, graph, senTreep, result.finalCondp, disableExprp,
                                 needsThreadFailReplay, needsThreadFailReplay && !negated);
        Ast_കണ്ണി_വ്യഞ്ജകം* matchExprp = nullptr;
        Ast_കണ്ണി_വ്യഞ്ജകം* outputExprp = m_loweringp->assembleResult(
            assertp, negated, result.finalCondp, signals, needMatch ? &matchExprp : nullptr);
        എങ്കിൽ (എണ്ണം_Matches) {
            UASSERT_OBJ(signals.matchCountp, coverp, "Cover sequence missing match count");
            VL_DO_DANGLING(outputExprp->delete_വൃക്ഷം(), outputExprp);
            propp->matchCountp(signals.matchCountp);
            outputExprp = പുതിയ AstNeq{flp, signals.matchCountp->clone_വൃക്ഷം_Pure(അസത്യം),
                                     newTypedConstp(flp, signals.matchCountp->dtypep(), 0)};
        }

        Ast_Sen_വൃക്ഷം* സ്ഥിരം threadFailReplaySenTreep
            = signals.threadFailCountp ? senTreep->clone_വൃക്ഷം(അസത്യം) : nullptr;

        VL_DO_DANGLING(തള്ളുക_Deletep(senTreep), senTreep);
        എങ്കിൽ (result.finalCondp && !result.finalCondp->backp()) തള്ളുക_Deletep(result.finalCondp);

        എങ്കിൽ (splitImplicationPasssp) {
            splitImplicationPassActions(assertAssertp, parts, matchExprp);
        } അല്ലെങ്കിൽ {
            attachActionHandlers(assertAssertp, matchExprp, threadFailReplaySenTreep,
                                 signals.threadFailCountp);
        }

        Ast_കണ്ണി* സ്ഥിരം innerPropp = propp->propp();
        innerPropp->replaceWith(outputExprp);
        VL_DO_DANGLING(തള്ളുക_Deletep(innerPropp), innerPropp);

        UINFO(4, "NFA converted assertion at " << flp << endl);

        എങ്കിൽ (dumpGraphLevel() >= 6) graph.m_graph.dumpDotFilePrefixed("assert-nfa");
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_loweringp);
        VL_RESTORER(m_defaultClockingp);
        VL_RESTORER(m_defaultDisablep);
        m_modp = nodep;
        // A class nested in a module is in the scope of the module's default clocking
        എങ്കിൽ (!VN_IS(nodep, Class)) m_defaultClockingp = nullptr;
        m_defaultDisablep = nodep->defaultDisablep();
        SvaNfaLowering lowering{nodep};
        m_loweringp = &lowering;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstClocking* nodep) override {
        എങ്കിൽ (nodep->isDefault() && !m_defaultClockingp) m_defaultClockingp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        VL_RESTORER(m_defaultDisablep);
        m_defaultDisablep = nodep->defaultDisablep();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstDefaultDisable* nodep) override {}
    ശൂന്യം visit(AstFell* nodep) override { visit_Sampled_മൂല്യം(nodep); }
    ശൂന്യം visit(AstPast* nodep) override { visit_Sampled_മൂല്യം(nodep); }
    ശൂന്യം visit(AstRose* nodep) override { visit_Sampled_മൂല്യം(nodep); }
    ശൂന്യം visit(AstStable* nodep) override { visit_Sampled_മൂല്യം(nodep); }
    ശൂന്യം visit(AstAssert* nodep) override { processAssertion(nodep); }
    ശൂന്യം visit(AstCover* nodep) override { processAssertion(nodep); }
    ശൂന്യം visit(AstRestrict* nodep) override {
        // Restrict property is ignored by simulators (IEEE 1800-2023 16.12.2).
        // Remove here so temporal SExpr don't leak to V3AssertPre.
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstAssertIntrinsic* nodep) override {}
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    explicit AssertNfaVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Top entry point

ശൂന്യം V3AssertNfa::assertNfaAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":" << endl);
    { AssertNfaVisitor{nodep}; }
    V3Global::dump_Check_Global_വൃക്ഷം("assertnfa", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
