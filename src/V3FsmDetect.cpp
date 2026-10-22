// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: FSM coverage detect pass
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// FSM COVERAGE DETECT:
//      Walk clocked always blocks while the original FSM structure is still
//      present, build a per-FSM V3Graph representation of the extracted
//      states/transitions, then immediately lower that completed graph state
//      into the final coverage declarations, previous-state tracking, and
//      active blocks needed to implement FSM state and arc coverage in the
//      generated model.
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"

#अन्तर्भाव "V3FsmDetect.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3SharedTmps.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cctype>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
वर्ग Fsm_State_मानम् final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_कुञ्चिका;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_सङ्ख्या m_अङ्कः;  // Original folded value, preserving width for lowered comparisons

    स्थिर string make_कुञ्चिका(ध्रुव V3_सङ्ख्या& अङ्कः) {
        V3_सङ्ख्या कुञ्चिका_अङ्कः = अङ्कः;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        कुञ्चिका_अङ्कः.isSigned(मिच्छ);
        प्रत्यागम cvtToStr(कुञ्चिका_अङ्कः.width()) + ":" + कुञ्चिका_अङ्कः.ascii(सच्च, सच्च);
    }

सार्वजनिक:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_मानम्()
        : m_कुञ्चिका{"1:1'h0"}
        , m_अङ्कः{static_cast<Ast_ग्रन्थिः*>(nullptr), 1, 0} {}
    explicit Fsm_State_मानम्(ध्रुव V3_सङ्ख्या& अङ्कः)
        : m_कुञ्चिका{make_कुञ्चिका(अङ्कः)}
        , m_अङ्कः{अङ्कः} {}

    ध्रुव string& कुञ्चिका() ध्रुव { प्रत्यागम m_कुञ्चिका; }
    ध्रुव V3_सङ्ख्या& अङ्कः() ध्रुव { प्रत्यागम m_अङ्कः; }
    string ascii() ध्रुव { प्रत्यागम m_अङ्कः.ascii(सच्च, सच्च); }
    string warnText() ध्रुव {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        जइ (m_अङ्कः.width() <= 32) प्रत्यागम cvtToStr(m_अङ्कः.toUInt());
        प्रत्यागम ascii();
    }

    तार्किक operator==(ध्रुव Fsm_State_मानम्& rhs) ध्रुव { प्रत्यागम m_कुञ्चिका == rhs.m_कुञ्चिका; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
संरचना Fsm_State_मानम्_Hash final {
    size_t operator()(ध्रुव Fsm_State_मानम्& मानम्) ध्रुव {
        प्रत्यागम std::hash<string>{}(मानम्.कुञ्चिका());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
संरचना FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
संरचना FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    तार्किक active_नीचः = मिच्छ;
};

वर्ग FsmResetArcDesc final {
    Fsm_State_मानम् m_to_मानम्;  // Encoded reset target state.
    Ast_ग्रन्थिः* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_ग्रन्थिः_व्यञ्जकम्* m_valuep = nullptr;  // Expression that provided the reset value.

सार्वजनिक:
    FsmResetArcDesc() = स्वभाव;
    FsmResetArcDesc(Fsm_State_मानम् to_मानम्, Ast_ग्रन्थिः_Assign* nodep)
        : m_to_मानम्{to_मानम्}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_मानम् to_मानम्, Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : m_to_मानम्{to_मानम्}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_मानम् to_मानम्() ध्रुव { प्रत्यागम m_to_मानम्; }
    Ast_ग्रन्थिः* nodep() ध्रुव { प्रत्यागम m_nodep; }
    Ast_ग्रन्थिः_व्यञ्जकम्* valuep() ध्रुव { प्रत्यागम m_valuep; }
};

संरचना FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    तार्किक has_Rst_Active_नीचः = मिच्छ;
    तार्किक rst_Active_नीचः = मिच्छ;
};

स्थिर तार्किक fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ध्रुव string& portName,
                                                  तार्किक& active_नीचः) {
    तार्किक matched = मिच्छ;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp = itemp->varrefp();
        जइ (!vrefp) प्रत्यागम;
        जइ (vrefp->varp()->name() != portName) प्रत्यागम;
        active_नीचः = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = सच्च;
    });
    प्रत्यागम matched;
}

स्थिर ध्रुव V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_ग्रन्थिः_Module* ध्रुव modp = cellp->modp();
    ध्रुव string origName = modp->origName();
    जइ (ध्रुव V3Control::FsmRegisterWrapper* ध्रुव descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        प्रत्यागम descp;
    }
    प्रत्यागम V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

स्थिर FsmWrapperRoles rolesFromDesc(ध्रुव V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_मानम्;
    प्रत्यागम roles;
}

वर्ग FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    तार्किक m_hasResetCond = मिच्छ;  // Whether the FSM had a modeled reset predicate.
    तार्किक m_inclCond = मिच्छ;  // Whether conditional/default arcs are kept explicitly.
    File_पङ्क्तिः* m_flp = nullptr;  // Representative source location.

सार्वजनिक:
    AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }
    शून्य scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ध्रुव { प्रत्यागम m_alwaysp; }
    शून्य alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ध्रुव { प्रत्यागम m_stateVscp; }
    शून्य stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ध्रुव { प्रत्यागम m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    शून्य sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() ध्रुव { प्रत्यागम m_nextVscp; }
    शून्य nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    ध्रुव std::vector<FsmSenDesc>& senses() ध्रुव { प्रत्यागम m_senses; }
    std::vector<FsmSenDesc>& senses() { प्रत्यागम m_senses; }
    ध्रुव FsmResetCondDesc& resetCond() ध्रुव { प्रत्यागम m_resetCond; }
    FsmResetCondDesc& resetCond() { प्रत्यागम m_resetCond; }
    ध्रुव std::vector<FsmResetArcDesc>& resetArcs() ध्रुव { प्रत्यागम m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { प्रत्यागम m_resetArcs; }
    तार्किक hasResetCond() ध्रुव { प्रत्यागम m_hasResetCond; }
    शून्य hasResetCond(तार्किक flag) { m_hasResetCond = flag; }
    तार्किक inclCond() ध्रुव { प्रत्यागम m_inclCond; }
    शून्य inclCond(तार्किक flag) { m_inclCond = flag; }
    File_पङ्क्तिः* fileline() ध्रुव { प्रत्यागम m_flp; }
    शून्य fileline(File_पङ्क्तिः* flp) { m_flp = flp; }
};

वर्ग FsmComboAlways final {
    AstScope* ध्रुव m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* ध्रुव m_alwaysp = nullptr;  // Candidate transition process.

सार्वजनिक:
    FsmComboAlways() = स्वभाव;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }
    AstAlways* alwaysp() ध्रुव { प्रत्यागम m_alwaysp; }
};

वर्ग FsmGraph;

वर्ग FsmVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

सार्वजनिक:
    गणना वर्ग Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

गुप्त:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_मानम् m_मानम्;  // Encoded state value for real state vertices.

रक्षित:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_मानम् मानम्) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_मानम्{मानम्} {}
    ~FsmVertex() override = स्वभाव;

सार्वजनिक:
    Kind kind() ध्रुव { प्रत्यागम m_kind; }
    तार्किक isState() ध्रुव { प्रत्यागम m_kind == Kind::STATE; }
    तार्किक isResetAny() ध्रुव { प्रत्यागम m_kind == Kind::RESET_ANY; }
    तार्किक isDefaultAny() ध्रुव { प्रत्यागम m_kind == Kind::DEFAULT_ANY; }
    ध्रुव string& label() ध्रुव { प्रत्यागम m_label; }
    ध्रुव Fsm_State_मानम्& मानम्() ध्रुव { प्रत्यागम m_मानम्; }

    string name() ध्रुव override VL_MT_SAFE { प्रत्यागम m_label + "=" + m_मानम्.ascii(); }
};

वर्ग FsmStateVertex final : सार्वजनिक FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

सार्वजनिक:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_मानम् मानम्) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, मानम्} {}
    ~FsmStateVertex() override = स्वभाव;

    string dotColor() ध्रुव override { प्रत्यागम "lightblue"; }
    string dotShape() ध्रुव override { प्रत्यागम "ellipse"; }
};

वर्ग FsmPseudoVertex final : सार्वजनिक FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

सार्वजनिक:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_मानम्{}} {}
    ~FsmPseudoVertex() override = स्वभाव;

    string name() ध्रुव override VL_MT_SAFE { प्रत्यागम label(); }
    string dotColor() ध्रुव override { प्रत्यागम isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ध्रुव override { प्रत्यागम "diamond"; }
};

वर्ग FsmArcEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    तार्किक m_isReset = मिच्छ;  // Arc originates from the synthetic reset source.
    तार्किक m_isCond = मिच्छ;  // Arc came from a conditional next-state split.
    तार्किक m_isDefault = मिच्छ;  // Arc represents a case default source.
    File_पङ्क्तिः* m_flp = nullptr;  // Source location for emitted coverage metadata.

सार्वजनिक:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* शिखरम्, तार्किक isReset, तार्किक isCond,
               तार्किक isDefault, File_पङ्क्तिः* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, शिखरम्, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = स्वभाव;

    तार्किक isReset() ध्रुव { प्रत्यागम m_isReset; }
    तार्किक isCond() ध्रुव { प्रत्यागम m_isCond; }
    तार्किक isDefault() ध्रुव { प्रत्यागम m_isDefault; }
    File_पङ्क्तिः* fileline() ध्रुव { प्रत्यागम m_flp; }

    string dotLabel() ध्रुव override {
        जइ (m_isReset) प्रत्यागम "reset";
        जइ (m_isDefault) प्रत्यागम "default";
        जइ (m_isCond) प्रत्यागम "cond";
        प्रत्यागम "";
    }
    string dotColor() ध्रुव override {
        जइ (m_isReset) प्रत्यागम "darkgreen";
        जइ (m_isDefault) प्रत्यागम "orange";
        जइ (m_isCond) प्रत्यागम "blue";
        प्रत्यागम "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
वर्ग FsmGraph final : सार्वजनिक V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    तार्किक m_hasResetCond = मिच्छ;  // Whether the detected FSM had a reset branch.
    तार्किक m_inclCond = मिच्छ;  // Whether conditional arcs should be kept explicitly.
    File_पङ्क्तिः* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_मानम्, FsmStateVertex*, Fsm_State_मानम्_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

सार्वजनिक:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{नूतन FsmPseudoVertex{एतत्, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{नूतन FsmPseudoVertex{एतत्, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }
    शून्य scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ध्रुव { प्रत्यागम m_stateAlwaysp; }
    शून्य stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ध्रुव string& stateVarName() ध्रुव { प्रत्यागम m_stateVarName; }
    शून्य stateVarName(ध्रुव string& name) { m_stateVarName = name; }
    ध्रुव string& stateVarInternalName() ध्रुव { प्रत्यागम m_stateVarInternalName; }
    शून्य stateVarInternalName(ध्रुव string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() ध्रुव { प्रत्यागम m_stateVarScopep; }
    शून्य stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ध्रुव {
        प्रत्यागम m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    शून्य sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ध्रुव std::vector<FsmSenDesc>& senses() ध्रुव { प्रत्यागम m_senses; }
    std::vector<FsmSenDesc>& senses() { प्रत्यागम m_senses; }
    ध्रुव FsmResetCondDesc& resetCond() ध्रुव { प्रत्यागम m_resetCond; }
    FsmResetCondDesc& resetCond() { प्रत्यागम m_resetCond; }
    तार्किक hasResetCond() ध्रुव { प्रत्यागम m_hasResetCond; }
    शून्य hasResetCond(तार्किक flag) { m_hasResetCond = flag; }
    तार्किक inclCond() ध्रुव { प्रत्यागम m_inclCond; }
    शून्य inclCond(तार्किक flag) { m_inclCond = flag; }
    File_पङ्क्तिः* fileline() ध्रुव { प्रत्यागम m_flp; }
    शून्य fileline(File_पङ्क्तिः* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_मानम् मानम्) VL_MT_DISABLED {
        FsmStateVertex* ध्रुव vertexp = नूतन FsmStateVertex{एतत्, label, मानम्};
        m_stateVertices.emplace(मानम्, vertexp);
        प्रत्यागम vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { प्रत्यागम m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { प्रत्यागम m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_मानम् from_मानम्, Fsm_State_मानम् to_मानम्, तार्किक isReset, तार्किक isCond,
                       तार्किक isDefault, File_पङ्क्तिः* flp) VL_MT_DISABLED {
        FsmStateVertex* ध्रुव शिखरम् = m_stateVertices.at(to_मानम्);
        FsmVertex* fromp = nullptr;
        जइ (isReset) {
            fromp = resetAnyVertex();
        } अण्णहा जइ (isDefault) {
            fromp = defaultAnyVertex();
        } अण्णहा {
            fromp = m_stateVertices.at(from_मानम्);
        }
        प्रत्यागम नूतन FsmArcEdge{एतत्, fromp, शिखरम्, isReset, isCond, isDefault, flp};
    }

    string name() ध्रुव VL_MT_SAFE {
        प्रत्यागम "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) ध्रुव {
        string tag = stateVarInternalName();
        प्रति (वर्ण& ch : tag) {
            जइ (!std::isalnum(static_cast<unsigned वर्ण>(ch))) ch = '_';
        }
        प्रत्यागम "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

संरचना DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

संरचना FsmCaseCandidate final {
    Ast_ग्रन्थिः* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
संरचना FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_ग्रन्थिः_व्यञ्जकम्* valuep = nullptr;  // Compared constant value expression
    Fsm_State_मानम् मानम्;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
संरचना FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_ग्रन्थिः* stmtsp = nullptr;  // Branch body
    Ast_ग्रन्थिः_व्यञ्जकम्* valuep = nullptr;  // Source state value expression
    Fsm_State_मानम् from_मानम्;  // Encoded source state value
    तार्किक has_शिखरम्_Guard = मिच्छ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
संरचना FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_ग्रन्थिः* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<ध्रुव AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<ध्रुव AstCell*, FsmCellPortMap>;

संरचना StateConstLabel final {
    string text;
    तार्किक fromParam = मिच्छ;
    size_t stateIndex = 0;
};

संरचना FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_मानम्>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_मानम्, StateConstLabel, Fsm_State_मानम्_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    तार्किक enumBacked = मिच्छ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
वर्ग FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<ध्रुव AstVarScope*, size_t> m_fsmIndex;

सार्वजनिक:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ध्रुव std::map<ध्रुव AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        जइ (it != m_fsmIndex.end()) प्रत्यागम m_fsms.at(it->second);
        ध्रुव size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        प्रत्यागम m_fsms.back();
    }
    ध्रुव std::vector<DetectedFsm>& fsms() ध्रुव { प्रत्यागम m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
वर्ग FsmDetectVisitor final : सार्वजनिक VNVisitor {
    // STATE - for current visit position (use VL_RESTORER)
    FsmState& m_state;
    AstScope* m_scopep = nullptr;
    std::vector<FsmRegisterCandidate> m_registerCandidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::vector<FsmComboAlways> m_oneBlockAlwayss;
    std::vector<FsmComboAlways> m_comboAlwayss;
    std::vector<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::vector<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ध्रुव AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<ध्रुव AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    स्थिर Ast_ग्रन्थिः_DType* unwrapEnumCandidate(Ast_ग्रन्थिः_DType* dtypep) {
        प्रत्यागम dtypep->skipRefToEnump();
    }

    स्थिर string candidateConflictContext(Ast_ग्रन्थिः* laterNodep,
                                           ध्रुव FsmCaseCandidate& firstCand) {
        प्रत्यागम '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    स्थिर तार्किक rejectFsmWrapperCell(AstCell* cellp, ध्रुव string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        प्रत्यागम मिच्छ;
    }

    स्थिर तार्किक simple_Param_State_मानम्(AstCell* cellp, ध्रुव string& name, Fsm_State_मानम्& मानम्,
                                      Ast_ग्रन्थिः_व्यञ्जकम्*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        प्रति (Ast_ग्रन्थिः* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
            जइ (!varp || !varp->isParam() || varp->name() != name) अनुवर्त;
            valuepr = VN_AS(varp->valuep(), ग्रन्थिः_व्यञ्जकम्);
            प्रत्यागम const_मानम्_Status(valuepr, मानम्) == Const_मानम्_Status::OK;
        }
        प्रत्यागम मिच्छ;
    }

    स्थिर तार्किक childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        जइ (!vscp->varp()->isIO()) प्रत्यागम मिच्छ;
        AstScope* ध्रुव scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        जइ (scopep->aboveScopep() != parentScopep) प्रत्यागम मिच्छ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        प्रत्यागम सच्च;
    }

    स्थिर AstVarScope* simpleAssignVarScope(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        AstVarRef* ध्रुव vrefp = VN_CAST(exprp, VarRef);
        प्रत्यागम vrefp ? vrefp->varScopep() : nullptr;
    }

    शून्य addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        ध्रुव std::pair<AstScope*, AstCell*> item{scopep, cellp};
        जइ (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            प्रत्यागम;
        }
        m_wrapperCells.emplace_back(item);
    }

    शून्य collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ध्रुव lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* ध्रुव rhsVscp = simpleAssignVarScope(nodep->rhsp());
        जइ (!lhsVscp || !rhsVscp) प्रत्यागम;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        जइ (childPortInScope(lhsVscp, m_scopep, cellp)) {
            जइ (!fsmRegisterWrapperDesc(cellp)) प्रत्यागम;
            UASSERT_OBJ(lhsVscp->varp()->is_निवेशः(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } अण्णहा जइ (childPortInScope(rhsVscp, m_scopep, cellp)) {
            जइ (!fsmRegisterWrapperDesc(cellp)) प्रत्यागम;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ध्रुव string& portName) ध्रुव {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ध्रुव FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        ध्रुव FsmCellPortMap::const_iterator portIt = ports.find(portName);
        प्रत्यागम portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, ध्रुव string& portName) ध्रुव {
        ध्रुव FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        ध्रुव FsmCellPortMap::const_iterator portIt = ports.find(portName);
        प्रत्यागम portIt == ports.end() ? nullptr : portIt->second;
    }

    वर्ग RegisterAlwaysAnalyzer final {
        AstScope* ध्रुव m_scopep;  // Scope containing the always block(s) being analyzed

    सार्वजनिक:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_ग्रन्थिः_व्यञ्जकम्*>>
        oneBlockCandidates(AstAlways* alwaysp) ध्रुव {
            std::vector<std::pair<AstCase*, Ast_ग्रन्थिः_व्यञ्जकम्*>> candidates;
            Ast_ग्रन्थिः* ध्रुव stmtsp = alwaysp->stmtsp();
            जइ (AstIf* ध्रुव firstIfp = VN_CAST(stmtsp, If)) {
                जइ (AstCase* ध्रुव casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                जइ (AstCase* ध्रुव casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            प्रत्यागम candidates;
        }

        std::vector<std::pair<AstIf*, Ast_ग्रन्थिः_व्यञ्जकम्*>>
        oneBlockIfCandidates(AstAlways* alwaysp) ध्रुव {
            std::vector<std::pair<AstIf*, Ast_ग्रन्थिः_व्यञ्जकम्*>> candidates;
            Ast_ग्रन्थिः* ध्रुव stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            जइ (AstIf* ध्रुव firstIfp = VN_CAST(stmtsp, If)) {
                जइ (AstIf* ध्रुव chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                जइ (AstIf* ध्रुव ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            प्रत्यागम candidates;
        }

        तार्किक matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) ध्रुव {
            प्रत्यागम FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        शून्य buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_ग्रन्थिः_व्यञ्जकम्* resetCondp,
                                    FsmRegisterCandidate& reg) ध्रुव {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_वृक्षः(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ध्रुव firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            जइ (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                ध्रुव ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                जइ (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } अण्णहा जइ (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_मानम् reset_मानम्;
                    Ast_ग्रन्थिः* ध्रुव thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_ग्रन्थिः_Assign* ध्रुव resetAssp = FsmDetectVisitor::direct_Const_State_Assign_ग्रन्थिः(
                        thenNodep, resetStateVscp, reset_मानम्);
                    जइ (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_मानम्, resetAssp);
                    }
                } अण्णहा जइ (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    तार्किक matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) ध्रुव {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* ध्रुव nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* ध्रुव stateVscp = roleVarScope(cellp, roles.qPort);
        जइ (!nextVscp || !stateVscp) {
            प्रत्यागम rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ध्रुव clkVscp = roleVarScope(cellp, roles.clkPort);
        जइ (!clkVscp) {
            प्रत्यागम rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().प्रक्षिप_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        जइ (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        जइ (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            तार्किक inferred_Active_नीचः = मिच्छ;
            जइ (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_नीचः)) {
                roles.has_Rst_Active_नीचः = सच्च;
                roles.rst_Active_नीचः = inferred_Active_नीचः;
            }
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* resetValuep = nullptr;
        Fsm_State_मानम् reset_मानम्;
        ध्रुव तार्किक has_Reset_मानम्
            = !roles.rstValParam.empty()
              && simple_Param_State_मानम्(cellp, roles.rstValParam, reset_मानम्, resetValuep);
        जइ (resetVscp && roles.has_Rst_Active_नीचः && has_Reset_मानम्) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_नीचः ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().प्रक्षिप_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_नीचः = roles.rst_Active_नीचः;
            cand.hasResetCond(सच्च);
            cand.resetArcs().emplace_back(reset_मानम्, cellp, resetValuep);
        } अण्णहा जइ (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            जइ (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } अण्णहा जइ (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } अण्णहा जइ (!roles.has_Rst_Active_नीचः) {
                reason = "reset polarity could not be inferred from the wrapper";
            } अण्णहा जइ (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } अण्णहा {
                reason = "reset_value parameter is missing or not static";
            }
            cellp->v3warn(COVERIGN,
                          "Ignoring unsupported: fsm_register_wrapper reset arcs require both "
                          "reset polarity and static reset value; "
                              + reason);
        }

        // This candidate represents a register proven through an instance
        // boundary, so there is no parent always_ff body to annotate. Lowering
        // treats null alwaysp as the explicit cell-path contract and builds its
        // sampling block from the recovered clock/reset interface instead.
        cand.scopep(scopep);
        cand.alwaysp(nullptr);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(childRoleVarScope(cellp, roles.qPort));
        cand.nextVscp(nextVscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        प्रत्यागम सच्च;
    }

    वर्ग ComboAlwaysAnalyzer final {
    सार्वजनिक:
        संरचना ComboMatch final {
            ध्रुव FsmRegisterCandidate* matchedp = nullptr;
            Ast_ग्रन्थिः* warnNodep = nullptr;
        };

    गुप्त:
        ध्रुव std::vector<FsmRegisterCandidate>& m_registerCandidates;

    सार्वजनिक:
        explicit ComboAlwaysAnalyzer(ध्रुव std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_ग्रन्थिः* stmtsp, AstCase* casep) ध्रुव {
            ComboMatch match;
            AstVarRef* ध्रुव selp = VN_CAST(casep->exprp(), VarRef);
            जइ (!selp) प्रत्यागम match;
            प्रति (ध्रुव FsmRegisterCandidate& reg : m_registerCandidates) {
                जइ (selp->varScopep() == reg.nextVscp()) {
                    जइ (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        अनुवर्त;
                    }
                } अण्णहा जइ (selp->varScopep() != reg.stateVscp()) {
                    अनुवर्त;
                }
                Ast_ग्रन्थिः* ध्रुव warnNodep = FsmDetectVisitor::case_Supported_Transition_ग्रन्थिः(
                    casep, reg.nextVscp(), reg.inclCond());
                जइ (!warnNodep) अनुवर्त;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            प्रत्यागम match;
        }

        ComboMatch matchIfChain(Ast_ग्रन्थिः* stmtsp, ध्रुव FsmIfChainCandidate& chain) ध्रुव {
            ComboMatch match;
            प्रति (ध्रुव FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                जइ (chain.compareVscp == reg.nextVscp()) {
                    जइ (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        अनुवर्त;
                    }
                } अण्णहा जइ (chain.compareVscp != reg.stateVscp()) {
                    अनुवर्त;
                }
                Ast_ग्रन्थिः* ध्रुव warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_ग्रन्थिः(chain, reg.nextVscp());
                जइ (!warnNodep) अनुवर्त;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            प्रत्यागम match;
        }

        तार्किक shouldWarnUnsupported(Ast_ग्रन्थिः* stmtsp, AstCase* casep) ध्रुव {
            ध्रुव AstVarRef* ध्रुव selp = VN_CAST(casep->exprp(), VarRef);
            जइ (!selp) प्रत्यागम मिच्छ;

            प्रत्यागम std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](ध्रुव FsmRegisterCandidate& reg) -> तार्किक {
                    ध्रुव तार्किक matchesNext = selp->varScopep() == reg.nextVscp();
                    ध्रुव तार्किक matchesState = selp->varScopep() == reg.stateVscp();

                    जइ (!matchesNext && !matchesState) प्रत्यागम मिच्छ;
                    जइ (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        प्रत्यागम मिच्छ;
                    }
                    प्रत्यागम FsmDetectVisitor::case_Supported_Transition_ग्रन्थिः(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    स्थिर तार्किक isSimpleResetCond(Ast_ग्रन्थिः_व्यञ्जकम्* condp) { प्रत्यागम VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    स्थिर FsmResetCondDesc describeResetCond(Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        FsmResetCondDesc desc;
        जइ (AstVarRef* ध्रुव vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        प्रत्यागम desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    स्थिर std::vector<FsmSenDesc> describe_Sen_वृक्षः(Ast_Sen_वृक्षः* sentreep) {
        std::vector<FsmSenDesc> senses;
        प्रति (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp = itemp->varrefp();
            जइ (!vrefp) अनुवर्त;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.प्रक्षिप_back(desc);
        }
        प्रत्यागम senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    स्थिर तार्किक isIgnorableStmt(Ast_ग्रन्थिः* nodep) { प्रत्यागम VN_IS(nodep, CoverInc); }

    स्थिर Ast_ग्रन्थिः* skipLeadingIgnorableStmt(Ast_ग्रन्थिः* nodep) {
        जाव (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        प्रत्यागम nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    स्थिर Ast_ग्रन्थिः* singleMeaningfulStmt(Ast_ग्रन्थिः* stmtp) {
        Ast_ग्रन्थिः* resultp = nullptr;
        प्रति (Ast_ग्रन्थिः* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            जइ (isIgnorableStmt(nodep)) अनुवर्त;
            जइ (resultp) प्रत्यागम nullptr;
            resultp = nodep;
        }
        प्रत्यागम resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    स्थिर Ast_ग्रन्थिः* singleMeaningfulBranch(Ast_ग्रन्थिः* branchp) {
        जइ (!branchp) प्रत्यागम nullptr;
        प्रत्यागम branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    स्थिर तार्किक isPlainComboSentree(ध्रुव Ast_Sen_वृक्षः* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        प्रति (ध्रुव AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            जइ (senp->edgeType() == VEdgeType::ET_CHANGED) अनुवर्त;
            प्रत्यागम मिच्छ;
        }
        प्रत्यागम सच्च;
    }

    शून्य warnUnsupportedComboAlways(ध्रुव FsmComboAlways& combo) {
        ध्रुव ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_ग्रन्थिः* ध्रुव stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        तार्किक warned = मिच्छ;
        प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ध्रुव casep = VN_CAST(nodep, Case);
            जइ (!casep) अनुवर्त;
            जइ (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = सच्च;
            }
            जइ (warned) भङ्ग;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    स्थिर Ast_ग्रन्थिः_Assign* directStateAssign(Ast_ग्रन्थिः* stmtp, AstVarScope* stateVscp) {
        Ast_ग्रन्थिः* ध्रुव nodep = singleMeaningfulBranch(stmtp);
        जइ (!nodep) प्रत्यागम nullptr;
        Ast_ग्रन्थिः_Assign* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_Assign);
        जइ (!assp) प्रत्यागम nullptr;
        AstVarRef* ध्रुव vrefp = VN_CAST(assp->lhsp(), VarRef);
        जइ (!vrefp || vrefp->varScopep() != stateVscp) प्रत्यागम nullptr;
        प्रत्यागम assp;
    }

    स्थिर Ast_ग्रन्थिः_Assign* ग्रन्थिः_State_Var_Assign(Ast_ग्रन्थिः* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_ग्रन्थिः_Assign* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_Assign);
        जइ (!assp) प्रत्यागम nullptr;
        AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp()->baseFromp(सच्च), VarRef);
        AstVarRef* ध्रुव rhsp = VN_CAST(assp->rhsp(), VarRef);
        जइ (!rhsp || !lhsp) प्रत्यागम nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        प्रत्यागम assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    स्थिर Ast_ग्रन्थिः_Assign* directCondStateVarAssign(Ast_ग्रन्थिः* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_ग्रन्थिः_व्यञ्जकम्*& condp,
                                                   Fsm_State_मानम्& reset_मानम्) {
        Ast_ग्रन्थिः_Assign* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_Assign);
        जइ (!assp) प्रत्यागम nullptr;
        AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp()->baseFromp(सच्च), VarRef);
        AstCond* ध्रुव rhsp = VN_CAST(assp->rhsp(), Cond);
        जइ (!rhsp || !lhsp) प्रत्यागम nullptr;
        AstVarRef* ध्रुव elsep = VN_CAST(rhsp->elsep(), VarRef);
        जइ (!elsep) प्रत्यागम nullptr;
        जइ (const_मानम्_Status(rhsp->thenp(), reset_मानम्) != Const_मानम्_Status::OK) प्रत्यागम nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        प्रत्यागम assp;
    }

    स्थिर Ast_ग्रन्थिः_Assign* direct_Const_State_Assign_ग्रन्थिः(Ast_ग्रन्थिः* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_मानम्& मानम्) {
        Ast_ग्रन्थिः_Assign* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_Assign);
        जइ (!assp) प्रत्यागम nullptr;
        AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp()->baseFromp(सच्च), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        जइ (const_मानम्_Status(assp->rhsp(), मानम्) != Const_मानम्_Status::OK) प्रत्यागम nullptr;
        stateVscp = lhsp->varScopep();
        प्रत्यागम assp;
    }

    गणना वर्ग ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    स्थिर ResetAssignStatus collectConstStateAssigns(Ast_ग्रन्थिः* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_ग्रन्थिः* nodep = skipLeadingIgnorableStmt(stmtp);
        जइ (!nodep) प्रत्यागम ResetAssignStatus::EMPTY;
        प्रति (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_मानम् मानम्;
            Ast_ग्रन्थिः_Assign* ध्रुव assp = direct_Const_State_Assign_ग्रन्थिः(nodep, assignStateVscp, मानम्);
            जइ (!assp) प्रत्यागम ResetAssignStatus::NONE;
            जइ (!stateVscp) stateVscp = assignStateVscp;
            जइ (assignStateVscp != stateVscp) प्रत्यागम ResetAssignStatus::NONE;
            जइ (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                प्रत्यागम ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(मानम्, assp);
            जइ (!nodep->nextp()) प्रत्यागम ResetAssignStatus::SINGLE;
        }
    }

    स्थिर तार्किक hasCanonicalNextStateDefaultBeforeCase(Ast_ग्रन्थिः* stmtsp, Ast_ग्रन्थिः* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_ग्रन्थिः* ध्रुव bodyp = skipLeadingIgnorableStmt(stmtsp);
        तार्किक sawCanonicalDefault = मिच्छ;
        प्रति (Ast_ग्रन्थिः* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            जइ (nodep == targetp) प्रत्यागम sawCanonicalDefault;
            जइ (Ast_ग्रन्थिः_Assign* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_Assign)) {
                AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ध्रुव rhsp = VN_CAST(assp->rhsp(), VarRef);
                जइ (!lhsp || lhsp->varScopep() != nextVscp) अनुवर्त;
                जइ (sawCanonicalDefault) {
                    ध्रुव string nextName = nextVscp->varp()->prettyNameQ();
                    ध्रुव string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    प्रत्यागम मिच्छ;
                }
                जइ (!rhsp || rhsp->varScopep() != stateVscp) प्रत्यागम मिच्छ;
                sawCanonicalDefault = सच्च;
            }
        }
    }

    स्थिर तार्किक ifStateConstAssign(Ast_ग्रन्थिः* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_मानम्& then_मानम्, Fsm_State_मानम्& else_मानम्) {
        AstIf* ध्रुव ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        जइ (!ifp || !ifp->elsesp()) प्रत्यागम मिच्छ;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_ग्रन्थिः* ध्रुव thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_ग्रन्थिः* ध्रुव elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        जइ (!thenNodep || !elseNodep) प्रत्यागम मिच्छ;
        जइ (!direct_Const_State_Assign_ग्रन्थिः(thenNodep, thenVscp, then_मानम्)) प्रत्यागम मिच्छ;
        जइ (!direct_Const_State_Assign_ग्रन्थिः(elseNodep, elseVscp, else_मानम्)) प्रत्यागम मिच्छ;
        जइ (thenVscp == stateVscp && elseVscp == stateVscp) प्रत्यागम सच्च;
        जइ (thenVscp != elseVscp) प्रत्यागम मिच्छ;
        Ast_ग्रन्थिः* ध्रुव followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_ग्रन्थिः* ध्रुव finalNodep = singleMeaningfulBranch(followp);
        जइ (!finalNodep) प्रत्यागम मिच्छ;
        जइ (!ग्रन्थिः_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) प्रत्यागम मिच्छ;
        जइ (finalStateVscp != stateVscp) प्रत्यागम मिच्छ;
        जइ (finalFromVscp != thenVscp) प्रत्यागम मिच्छ;
        प्रत्यागम सच्च;
    }

    स्थिर तार्किक directStateCondConstAssign(Ast_ग्रन्थिः* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_मानम्& then_मानम्, Fsm_State_मानम्& else_मानम्) {
        Ast_ग्रन्थिः_Assign* ध्रुव assp = directStateAssign(stmtp, stateVscp);
        जइ (!assp) प्रत्यागम मिच्छ;
        AstCond* ध्रुव condp = VN_CAST(assp->rhsp(), Cond);
        जइ (!condp) प्रत्यागम मिच्छ;
        प्रत्यागम const_मानम्_Status(condp->thenp(), then_मानम्) == Const_मानम्_Status::OK
               && const_मानम्_Status(condp->elsep(), else_मानम्) == Const_मानम्_Status::OK;
    }

    स्थिर Ast_ग्रन्थिः* case_Item_Supported_Arc_ग्रन्थिः(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             तार्किक inclCond) {
        जइ (itemp->isDefault()) {
            जइ (!inclCond) प्रत्यागम nullptr;
        }
        Ast_ग्रन्थिः_Assign* ध्रुव assp = directStateAssign(itemp->stmtsp(), stateVscp);
        जइ (assp) {
            Fsm_State_मानम् to_मानम्;
            जइ (const_मानम्_Status(assp->rhsp(), to_मानम्) == Const_मानम्_Status::OK) प्रत्यागम assp;
        }
        Fsm_State_मानम् then_मानम्;
        Fsm_State_मानम् else_मानम्;
        जइ (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_मानम्, else_मानम्)) {
            प्रत्यागम assp;
        }
        जइ (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_मानम्, else_मानम्)) {
            प्रत्यागम singleMeaningfulBranch(itemp->stmtsp());
        }
        प्रत्यागम nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    स्थिर Ast_ग्रन्थिः* case_Supported_Transition_ग्रन्थिः(AstCase* casep, AstVarScope* stateVscp,
                                                तार्किक inclCond) {
        प्रति (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            जइ (Ast_ग्रन्थिः* ध्रुव nodep = case_Item_Supported_Arc_ग्रन्थिः(itemp, stateVscp, inclCond))
                प्रत्यागम nodep;
        }
        प्रत्यागम nullptr;
    }

    स्थिर Ast_ग्रन्थिः* case_Item_Supported_Arc_ग्रन्थिः_Like(Ast_ग्रन्थिः* stmtsp, AstVarScope* stateVscp) {
        जइ (Ast_ग्रन्थिः_Assign* ध्रुव assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_मानम् to_मानम्;
            जइ (const_मानम्_Status(assp->rhsp(), to_मानम्) == Const_मानम्_Status::OK) प्रत्यागम assp;
            Fsm_State_मानम् then_मानम्;
            Fsm_State_मानम् else_मानम्;
            जइ (directStateCondConstAssign(stmtsp, stateVscp, then_मानम्, else_मानम्)) प्रत्यागम assp;
        }
        प्रत्यागम nullptr;
    }

    स्थिर Ast_ग्रन्थिः* if_Chain_Supported_Transition_ग्रन्थिः(ध्रुव FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        प्रति (size_t i = 0; i < chain.branches.size(); ++i) {
            ध्रुव FsmIfBranch& branch = chain.branches[i];
            Ast_ग्रन्थिः* ध्रुव nodep = case_Item_Supported_Arc_ग्रन्थिः_Like(branch.stmtsp, stateVscp);
            जइ (!nodep) प्रत्यागम nullptr;
        }
        प्रत्यागम chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    स्थिर string label_For_मानम्(
        ध्रुव std::unordered_map<Fsm_State_मानम्, StateConstLabel, Fsm_State_मानम्_Hash>& labels,
        ध्रुव Fsm_State_मानम्& मानम्) {
        प्रत्यागम labels.at(मानम्).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    गणना वर्ग Const_मानम्_Status : uint8_t { OK, NOT_CONST, XZ };

    स्थिर Const_मानम्_Status const_मानम्_Status(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Fsm_State_मानम्& मानम्) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(exprp, Const);
        जइ (!constp) प्रत्यागम Const_मानम्_Status::NOT_CONST;
        ध्रुव V3_सङ्ख्या& अङ्कः = constp->अङ्कः();
        जइ (अङ्कः.isAnyXZ()) प्रत्यागम Const_मानम्_Status::XZ;
        मानम् = Fsm_State_मानम्{अङ्कः};
        प्रत्यागम Const_मानम्_Status::OK;
    }

    स्थिर तार्किक pureStateComparisonNoAlias(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, FsmStateComparison& cmp) {
        AstEq* ध्रुव eqp = VN_CAST(exprp, Eq);
        जइ (!eqp) प्रत्यागम मिच्छ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = eqp->rhsp();
        जइ (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(सच्च), VarRef);
            जइ (!vrefp) { प्रत्यागम मिच्छ; }
            valuep = eqp->lhsp();
        }

        Fsm_State_मानम् मानम्;
        जइ (const_मानम्_Status(valuep, मानम्) != Const_मानम्_Status::OK) प्रत्यागम मिच्छ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.मानम् = मानम्;
        प्रत्यागम सच्च;
    }

    स्थिर तार्किक pureStateComparison(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        जइ (pureStateComparisonNoAlias(exprp, cmp)) प्रत्यागम सच्च;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        जइ (AstVarRef* ध्रुव vrefp = VN_CAST(exprp, VarRef)) {
            ध्रुव FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            जइ (it == aliases.end()) प्रत्यागम मिच्छ;
            cmp = it->second;
            प्रत्यागम सच्च;
        }
        प्रत्यागम मिच्छ;
    }

    स्थिर तार्किक supported_शिखरम्_Level_Guard(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        जइ (VN_IS(exprp, Or)) प्रत्यागम मिच्छ;
        जइ (VN_IS(exprp, RedAnd)) प्रत्यागम मिच्छ;
        जइ (VN_IS(exprp, RedOr)) प्रत्यागम मिच्छ;
        जइ (VN_IS(exprp, RedXor)) प्रत्यागम मिच्छ;
        प्रत्यागम सच्च;
    }

    स्थिर तार्किक resolveIfPredicate(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, तार्किक& hasGuard) {
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> terms;
        std::vector<Ast_ग्रन्थिः_व्यञ्जकम्*> pending;
        pending.प्रक्षिप_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        जाव (!pending.empty()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep = pending.back();
            pending.निष्कासय_back();
            जइ (AstAnd* ध्रुव andp = VN_CAST(nodep, And)) {
                pending.प्रक्षिप_back(andp->rhsp());
                pending.प्रक्षिप_back(andp->lhsp());
            } अण्णहा {
                terms.प्रक्षिप_back(nodep);
            }
        }

        तार्किक sawComparison = मिच्छ;
        प्रति (size_t i = 0; i < terms.size(); ++i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp = terms[i];
            FsmStateComparison termCmp;
            जइ (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                जइ (sawComparison) प्रत्यागम मिच्छ;
                cmp = termCmp;
                sawComparison = सच्च;
                अनुवर्त;
            }
            जइ (!supported_शिखरम्_Level_Guard(termp)) प्रत्यागम मिच्छ;
            hasGuard = सच्च;
        }
        प्रत्यागम sawComparison;
    }

    स्थिर शून्य addAlias(FsmAliasMap& aliases, std::unordered_set<ध्रुव AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ध्रुव FsmStateComparison& cmp) {
        जइ (ambiguous.find(aliasVscp) != ambiguous.end()) प्रत्यागम;
        ध्रुव FsmAliasMap::iterator it = aliases.find(aliasVscp);
        जइ (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            प्रत्यागम;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        जइ (it->second.stateVscp == cmp.stateVscp && it->second.मानम् == cmp.मानम्) प्रत्यागम;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        प्रत्यागम;
    }

    स्थिर शून्य collectAliasFromAssign(Ast_ग्रन्थिः_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<ध्रुव AstVarScope*>& ambiguous) {
        AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp(), VarRef);
        जइ (!lhsp) प्रत्यागम;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        जइ (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) प्रत्यागम;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_ग्रन्थिः* stmtsp, Ast_ग्रन्थिः* limitp) ध्रुव {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<ध्रुव AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        प्रति (Ast_ग्रन्थिः* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            जइ (Ast_ग्रन्थिः_Assign* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        प्रति (ध्रुव AstVarScope* ध्रुव vscp : ambiguous) aliases.erase(vscp);
        प्रत्यागम aliases;
    }

    स्थिर तार्किक collectIfChain(AstIf* ifp, ध्रुव FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_मानानि;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        जाव (सच्च) {
            FsmStateComparison cmp;
            तार्किक hasGuard = मिच्छ;
            जइ (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) प्रत्यागम मिच्छ;
            जइ (chain.compareVscp && chain.compareVscp != cmp.stateVscp) प्रत्यागम मिच्छ;
            जइ (!seen_मानानि.निवेशय(cmp.मानम्.कुञ्चिका()).second) प्रत्यागम मिच्छ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.प्रक्षिप_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.मानम्, hasGuard});

            Ast_ग्रन्थिः* ध्रुव elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            जइ (!elseNodep) भङ्ग;
            जइ (AstIf* ध्रुव elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                अनुवर्त;
            }
            chain.defaultStmtsp = elseNodep;
            भङ्ग;
        }
        प्रत्यागम chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    स्थिर तार्किक validate_Known_State_मानम्(Ast_ग्रन्थिः* nodep, ध्रुव FsmStateSpace& stateSpace,
                                        ध्रुव Fsm_State_मानम्& मानम्, ध्रुव string& role) {
        जइ (stateSpace.labels.find(मानम्) != stateSpace.labels.end()) प्रत्यागम सच्च;
        जइ (stateSpace.enumBacked) {
            ध्रुव string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + मानम्.warnText()
                                        + " is not present in the declared enum");
            प्रत्यागम मिच्छ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + मानम्.warnText()
                                    + " is not present in the inferred state space");
        प्रत्यागम मिच्छ;
    }

    स्थिर StateConstLabel stateLabelForConst(AstConst* constp) {
        ध्रुव string name = constp->origParamName();
        जइ (!name.empty()) प्रत्यागम StateConstLabel{Ast_ग्रन्थिः::prettyName(name), सच्च, 0};
        प्रत्यागम StateConstLabel{constp->name(), मिच्छ, 0};
    }

    स्थिर शून्य updateStateLabel(FsmStateSpace& stateSpace, ध्रुव Fsm_State_मानम्& मानम्,
                                 ध्रुव StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(मानम्).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    स्थिर तार्किक matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        जइ (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) प्रत्यागम मिच्छ;

        Ast_ग्रन्थिः* ध्रुव stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_ग्रन्थिः* ध्रुव nodep = singleMeaningfulStmt(stmtsp);
        जइ (!nodep) प्रत्यागम मिच्छ;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        जइ (AstIf* ध्रुव ifp = VN_CAST(nodep, If)) {
            जइ (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) प्रत्यागम मिच्छ;
            AstVarScope* resetStateVscp = nullptr;
            ध्रुव ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ध्रुव तार्किक emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            जइ (emptyResetBranch) {
                cand.resetArcs().clear();
            } अण्णहा जइ (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_मानम् reset_मानम्;
                Ast_ग्रन्थिः* ध्रुव thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_ग्रन्थिः_Assign* ध्रुव resetAssp
                    = direct_Const_State_Assign_ग्रन्थिः(thenNodep, resetStateVscp, reset_मानम्);
                जइ (!resetAssp) प्रत्यागम मिच्छ;
                cand.resetArcs().emplace_back(reset_मानम्, resetAssp);
            } अण्णहा जइ (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_ग्रन्थिः* ध्रुव elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            जइ (!ग्रन्थिः_State_Var_Assign(elseNodep, stateVscp, nextVscp)) प्रत्यागम मिच्छ;
            जइ (!emptyResetBranch && resetStateVscp != stateVscp) प्रत्यागम मिच्छ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } अण्णहा {
            Ast_ग्रन्थिः_व्यञ्जकम्* resetCondp = nullptr;
            Fsm_State_मानम् reset_मानम्;
            // Note the active low 'reset ? next : CONST' form is not accepted
            जइ (Ast_ग्रन्थिः_Assign* ध्रुव assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_मानम्)) {
                cand.resetArcs().emplace_back(reset_मानम्, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } अण्णहा जइ (!ग्रन्थिः_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                प्रत्यागम मिच्छ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_वृक्षः(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        प्रत्यागम सच्च;
    }

    स्थिर तार्किक add_मानम्_To_State_Space(Ast_ग्रन्थिः* nodep, FsmStateSpace& stateSpace,
                                     ध्रुव Fsm_State_मानम्& मानम्, StateConstLabel label) {
        ध्रुव auto labelIt = stateSpace.labels.find(मानम्);
        जइ (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            जइ (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + मानम्.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                प्रत्यागम मिच्छ;
            }
            जइ (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, मानम्, label);
            }
        } अण्णहा {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, मानम्);
            stateSpace.labels.emplace(मानम्, storedLabel);
        }
        प्रत्यागम सच्च;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    स्थिर तार्किक add_व्यञ्जकम्_To_State_Space(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_मानम् मानम्;
        ध्रुव Const_मानम्_Status status = const_मानम्_Status(valuep, मानम्);
        जइ (status != Const_मानम्_Status::OK) {
            जइ (status == Const_मानम्_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            प्रत्यागम मिच्छ;
        }
        AstConst* ध्रुव constp = VN_AS(valuep, Const);
        प्रत्यागम add_मानम्_To_State_Space(valuep, stateSpace, मानम्, stateLabelForConst(constp));
    }

    स्थिर तार्किक add_Optional_Target_व्यञ्जकम्_To_State_Space(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_मानम् मानम्;
        ध्रुव Const_मानम्_Status status = const_मानम्_Status(valuep, मानम्);
        जइ (status != Const_मानम्_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            प्रत्यागम मिच्छ;
        }
        AstConst* ध्रुव constp = VN_AS(valuep, Const);
        प्रत्यागम add_मानम्_To_State_Space(valuep, stateSpace, मानम्, stateLabelForConst(constp));
    }

    स्थिर शून्य addResetTargetsToStateSpace(ध्रुव std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        प्रति (ध्रुव FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_मानम्().ascii(), मिच्छ, 0};
            जइ (AstConst* ध्रुव constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_मानम्_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_मानम्(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    स्थिर तार्किक collectDeclaredStateSpace(Ast_ग्रन्थिः* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, तार्किक& needs_Source_मानानि) {
        AstVar* ध्रुव stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        जइ (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ध्रुव तार्किक forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        जइ (enump) {
            stateSpace.enumBacked = सच्च;
            प्रति (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ध्रुव AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
                ध्रुव Fsm_State_मानम् मानम्{constp->अङ्कः()};
                ध्रुव size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), मानम्);
                stateSpace.labels.emplace(मानम्,
                                          StateConstLabel{itemp->name(), मिच्छ, stateIndex});
            }
            प्रत्यागम stateSpace.states.size() >= 2;
        }

        जइ (forced) {
            needs_Source_मानानि = सच्च;
            प्रत्यागम सच्च;
        }

        needs_Source_मानानि = सच्च;
        प्रत्यागम सच्च;
    }

    template <typename T_ValuepVisitor>
    स्थिर तार्किक collect_State_Space_From_मानानि(Ast_ग्रन्थिः* warnNodep, AstVarScope* stateVscp,
                                            ध्रुव std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ध्रुव T_ValuepVisitor& visitValueps) {
        तार्किक needs_Source_मानानि = मिच्छ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        जइ (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_मानानि)) {
            प्रत्यागम मिच्छ;
        }
        जइ (!needs_Source_मानानि) प्रत्यागम सच्च;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        जइ (!visitValueps(
                [&](Ast_ग्रन्थिः_व्यञ्जकम्* valuep) { प्रत्यागम add_व्यञ्जकम्_To_State_Space(valuep, stateSpace); })) {
            प्रत्यागम मिच्छ;
        }
        प्रत्यागम stateSpace.states.size() >= 2;
    }

    स्थिर तार्किक collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  ध्रुव std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        प्रत्यागम collect_State_Space_From_मानानि(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](ध्रुव auto& visitValuep) {
                प्रति (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    जइ (!itemp->isDefault()) {
                        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                            जइ (!visitValuep(condp)) प्रत्यागम मिच्छ;
                        }
                    }
                    जइ (Ast_ग्रन्थिः_Assign* ध्रुव assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_मानम् then_मानम्;
                        Fsm_State_मानम् else_मानम्;
                        AstCond* ध्रुव condp = VN_CAST(assp->rhsp(), Cond);
                        जइ (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_मानम्,
                                                          else_मानम्)) {
                            जइ (!visitValuep(condp->thenp())) प्रत्यागम मिच्छ;
                            जइ (!visitValuep(condp->elsep())) प्रत्यागम मिच्छ;
                        } अण्णहा जइ (!add_Optional_Target_व्यञ्जकम्_To_State_Space(assp->rhsp(), stateSpace)) {
                            प्रत्यागम मिच्छ;
                        }
                    }
                }
                प्रत्यागम सच्च;
            });
    }

    स्थिर तार्किक collectStateSpace(ध्रुव FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  ध्रुव std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        प्रत्यागम collect_State_Space_From_मानानि(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](ध्रुव auto& visitValuep) {
                प्रति (ध्रुव FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_ग्रन्थिः_Assign* ध्रुव assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_मानम् then_मानम्;
                    Fsm_State_मानम् else_मानम्;
                    AstCond* ध्रुव condp = VN_CAST(assp->rhsp(), Cond);
                    जइ (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_मानम्, else_मानम्),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        जइ (!visitValuep(condp->thenp())) प्रत्यागम मिच्छ;
                        जइ (!visitValuep(condp->elsep())) प्रत्यागम मिच्छ;
                    } अण्णहा जइ (!add_Optional_Target_व्यञ्जकम्_To_State_Space(assp->rhsp(), stateSpace)) {
                        प्रत्यागम मिच्छ;
                    }
                }
                प्रत्यागम सच्च;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    स्थिर तार्किक emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 ध्रुव FsmStateSpace& stateSpace, तार्किक inclCond) {
        std::vector<std::pair<string, Fsm_State_मानम्>> froms;
        जइ (itemp->isDefault()) {
            जइ (!inclCond) प्रत्यागम मिच्छ;
            froms.emplace_back("default", Fsm_State_मानम्{});
        } अण्णहा {
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                Fsm_State_मानम् मानम्;
                जइ (const_मानम्_Status(condp, मानम्) != Const_मानम्_Status::OK) अनुवर्त;
                जइ (!validate_Known_State_मानम्(condp, stateSpace, मानम्, "source")) प्रत्यागम सच्च;
                froms.emplace_back(label_For_मानम्(stateSpace.labels, मानम्), मानम्);
            }
            जइ (froms.empty()) प्रत्यागम मिच्छ;
        }

        जइ (Ast_ग्रन्थिः_Assign* ध्रुव assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_मानम् to_मानम्;
            ध्रुव Const_मानम्_Status status = const_मानम्_Status(assp->rhsp(), to_मानम्);
            जइ (status == Const_मानम्_Status::OK) {
                जइ (!validate_Known_State_मानम्(assp, stateSpace, to_मानम्, "target")) प्रत्यागम सच्च;
                प्रति (ध्रुव std::pair<string, Fsm_State_मानम्>& from : froms) {
                    graph.addArc(from.second, to_मानम्, मिच्छ, मिच्छ, itemp->isDefault(),
                                 assp->fileline());
                }
                प्रत्यागम सच्च;
            }
        }

        Fsm_State_मानम् then_मानम्;
        Fsm_State_मानम् else_मानम्;
        जइ (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_मानम्, else_मानम्)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_मानम्, else_मानम्)) {
            जइ (!validate_Known_State_मानम्(itemp->stmtsp(), stateSpace, then_मानम्, "target"))
                प्रत्यागम सच्च;
            जइ (!validate_Known_State_मानम्(itemp->stmtsp(), stateSpace, else_मानम्, "target"))
                प्रत्यागम सच्च;
            प्रति (ध्रुव Fsm_State_मानम्& branch_मानम् : {then_मानम्, else_मानम्}) {
                प्रति (ध्रुव std::pair<string, Fsm_State_मानम्>& from : froms) {
                    graph.addArc(from.second, branch_मानम्, मिच्छ, सच्च, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            प्रत्यागम सच्च;
        }

        प्रत्यागम मिच्छ;
    }

    स्थिर शून्य emitStmtArcsFrom(FsmGraph& graph, Ast_ग्रन्थिः* stmtsp, AstVarScope* stateVscp,
                                 ध्रुव FsmStateSpace& stateSpace, Fsm_State_मानम् from_मानम्,
                                 तार्किक isDefault, तार्किक forceCond) {
        Ast_ग्रन्थिः_Assign* ध्रुव assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_मानम् to_मानम्;
        ध्रुव Const_मानम्_Status status = const_मानम्_Status(assp->rhsp(), to_मानम्);
        जइ (status == Const_मानम्_Status::OK) {
            जइ (!validate_Known_State_मानम्(assp, stateSpace, to_मानम्, "target")) प्रत्यागम;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_मानम्, to_मानम्, मिच्छ, forceCond, isDefault, assp->fileline());
            प्रत्यागम;
        }

        Fsm_State_मानम् then_मानम्;
        Fsm_State_मानम् else_मानम्;
        ध्रुव तार्किक condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_मानम्, else_मानम्);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        जइ (!validate_Known_State_मानम्(stmtsp, stateSpace, then_मानम्, "target")) प्रत्यागम;
        जइ (!validate_Known_State_मानम्(stmtsp, stateSpace, else_मानम्, "target")) प्रत्यागम;
        प्रति (ध्रुव Fsm_State_मानम्& branch_मानम् : {then_मानम्, else_मानम्}) {
            graph.addArc(from_मानम्, branch_मानम्, मिच्छ, सच्च, isDefault, stmtsp->fileline());
        }
    }

    स्थिर शून्य emitIfChainArcs(FsmGraph& graph, ध्रुव FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ध्रुव FsmStateSpace& stateSpace) {
        प्रति (size_t i = 0; i < chain.branches.size(); ++i) {
            ध्रुव FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            जइ (!validate_Known_State_मानम्(branch.ifp, stateSpace, branch.from_मानम्, "source"))
                प्रत्यागम;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_मानम्, मिच्छ,
                             branch.has_शिखरम्_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    स्थिर शून्य addResetArcs(FsmGraph& graph, ध्रुव std::vector<FsmResetArcDesc>& resetArcs,
                             ध्रुव FsmStateSpace& stateSpace) {
        प्रति (ध्रुव FsmResetArcDesc& resetArc : resetArcs) {
            जइ (!validate_Known_State_मानम्(resetArc.nodep(), stateSpace, resetArc.to_मानम्(),
                                         "target"))
                अनुवर्त;
            graph.addArc(Fsm_State_मानम्{}, resetArc.to_मानम्(), सच्च, मिच्छ, मिच्छ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    शून्य processCase(AstCase* casep, AstVarScope* assignVscp, ध्रुव FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ध्रुव stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        जइ (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) प्रत्यागम;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        जइ (!entry.graphp) {
            entry.graphp.reset(नूतन FsmGraph{});
            entry.graphp->scopep(reg.scopep());
            entry.graphp->stateAlwaysp(reg.alwaysp());
            entry.graphp->stateVarName(stateVscp->prettyName());
            entry.graphp->stateVarInternalName(stateVscp->varp()->name());
            entry.graphp->stateVarScopep(stateVscp);
            entry.graphp->sampleVarScopep(reg.sampleVscp());
            entry.graphp->senses() = reg.senses();
            entry.graphp->resetCond() = reg.resetCond();
            entry.graphp->hasResetCond(reg.hasResetCond());
            entry.graphp->inclCond(reg.inclCond());
            entry.graphp->fileline(casep->fileline());
            प्रति (ध्रुव std::pair<string, Fsm_State_मानम्>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        प्रति (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    शून्य processIfChain(ध्रुव FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        ध्रुव FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ध्रुव stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        जइ (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) प्रत्यागम;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(नूतन FsmGraph{});
        entry.graphp->scopep(reg.scopep());
        entry.graphp->stateAlwaysp(reg.alwaysp());
        entry.graphp->stateVarName(stateVscp->prettyName());
        entry.graphp->stateVarInternalName(stateVscp->varp()->name());
        entry.graphp->stateVarScopep(stateVscp);
        entry.graphp->sampleVarScopep(reg.sampleVscp());
        entry.graphp->senses() = reg.senses();
        entry.graphp->resetCond() = reg.resetCond();
        entry.graphp->hasResetCond(reg.hasResetCond());
        entry.graphp->inclCond(reg.inclCond());
        entry.graphp->fileline(chain.ifp->fileline());
        प्रति (ध्रुव std::pair<string, Fsm_State_मानम्>& state : stateSpace.states) {
            entry.graphp->addStateVertex(state.first, state.second);
        }
        addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*entry.graphp, chain, assignVscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    शून्य processOneBlockAlways(ध्रुव FsmComboAlways& oneBlock) {
        ध्रुव RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* ध्रुव alwaysp = oneBlock.alwaysp();
        जइ (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) प्रत्यागम;
        ध्रुव std::vector<std::pair<AstCase*, Ast_ग्रन्थिः_व्यञ्जकम्*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        प्रति (ध्रुव std::pair<AstCase*, Ast_ग्रन्थिः_व्यञ्जकम्*>& cand : candidates) {
            AstVarRef* ध्रुव selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* ध्रुव vscp = selp ? selp->varScopep() : nullptr;
            जइ (!vscp) अनुवर्त;
            जइ (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } अण्णहा जइ (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } अण्णहा {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        जइ (firstCand.stateVscp) प्रत्यागम;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ध्रुव std::vector<std::pair<AstIf*, Ast_ग्रन्थिः_व्यञ्जकम्*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        प्रति (ध्रुव std::pair<AstIf*, Ast_ग्रन्थिः_व्यञ्जकम्*>& cand : ifCandidates) {
            ध्रुव FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            जइ (!collectIfChain(cand.first, aliases, chain)) अनुवर्त;
            AstVarScope* ध्रुव vscp = chain.compareVscp;
            जइ (!if_Chain_Supported_Transition_ग्रन्थिः(chain, vscp)) अनुवर्त;
            जइ (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } अण्णहा जइ (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } अण्णहा {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    स्थिर शून्य warnComboSameAlways(Ast_ग्रन्थिः* warnNodep, ध्रुव FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    शून्य processComboAlways(ध्रुव FsmComboAlways& combo) {
        ध्रुव ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_ग्रन्थिः* ध्रुव stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ध्रुव casep = VN_CAST(nodep, Case);
            जइ (!casep) अनुवर्त;
            ध्रुव ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            ध्रुव FsmRegisterCandidate* ध्रुव matchedp = match.matchedp;
            Ast_ग्रन्थिः* ध्रुव matchedWarnNodep = match.warnNodep;
            जइ (!matchedp) अनुवर्त;
            जइ (!firstCand.stateVscp) {
                ध्रुव auto निवेशय_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                जइ (!निवेशय_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  निवेशय_Pair.first->second));
                    अनुवर्त;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                अनुवर्त;
            }
            जइ (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                अनुवर्त;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        जइ (firstCand.stateVscp) प्रत्यागम;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ध्रुव ifp = VN_CAST(nodep, If);
            जइ (!ifp) अनुवर्त;
            FsmIfChainCandidate chain;
            ध्रुव FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            जइ (!collectIfChain(ifp, aliases, chain)) अनुवर्त;
            ध्रुव ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            ध्रुव FsmRegisterCandidate* ध्रुव matchedp = match.matchedp;
            Ast_ग्रन्थिः* ध्रुव matchedWarnNodep = match.warnNodep;
            जइ (!matchedp) अनुवर्त;
            जइ (!firstCand.stateVscp) {
                ध्रुव std::pair<std::unordered_map<ध्रुव AstVarScope*, FsmCaseCandidate>::iterator,
                                तार्किक>
                    निवेशय_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                जइ (!निवेशय_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  निवेशय_Pair.first->second));
                    अनुवर्त;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                अनुवर्त;
            }
            जइ (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                अनुवर्त;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    शून्य visit(AstAlways* nodep) override {
        जइ (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        ध्रुव RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        जइ (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* ध्रुव stateVscp = reg.stateVscp();
            ध्रुव तार्किक found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](ध्रुव FsmRegisterCandidate& existing) {
                                  प्रत्यागम existing.stateVscp() == stateVscp;
                              });
            जइ (!found) { m_registerCandidates.emplace_back(reg); }
        }
        जइ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } अण्णहा जइ (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            जइ (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } अण्णहा {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    शून्य visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    शून्य visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        जइ (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        प्रति (ध्रुव std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            जइ (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        प्रति (ध्रुव FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        प्रति (ध्रुव FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        प्रति (ध्रुव FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
वर्ग FsmCoverIncBuilder final {
    ध्रुव FsmGraph& m_graph;  // Fsm graph
    std::vector<ध्रुव FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ध्रुव FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    ध्रुव FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edges_गणनम् = 0;  // Total count of edges
    AstVarScope* ध्रुव m_prevVscp;  // Previous state of fsm
    File_पङ्क्तिः* ध्रुव m_flp;  // Fileline pointer to use across the builder
    ध्रुव std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_ग्रन्थिः_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_ग्रन्थिः_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    तार्किक m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ध्रुव uint64_t मानम्) {
        V3_सङ्ख्या अङ्कः{static_cast<Ast_ग्रन्थिः*>(nullptr), m_prevVscp->width()};
        अङ्कः.setQuad(मानम्);
        अङ्कः.isSigned(m_prevVscp->dtypep()->isSigned());
        प्रत्यागम नूतन AstConst{m_flp, अङ्कः};
    }

    std::string quadToBitStirngLabel(ध्रुव uint64_t val) {
        पूर्णाङ्क width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        जाव (--width >= 0) result.प्रक्षिप_back("01"[(val >> width) & 1]);
        प्रत्यागम result;
    }

    शून्य initExistingStates() {
        प्रति (ध्रुव V3GraphVertex& vtx : m_graph.vertices()) {
            ध्रुव FsmVertex* ध्रुव vertexp = vtx.as<FsmVertex>();
            m_edges_गणनम् += vertexp->inEdges().size();
            जइ (ध्रुव FsmStateVertex* ध्रुव statep = vertexp->cast<FsmStateVertex>()) {
                m_states.प्रक्षिप_back(statep);
            } अण्णहा जइ (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } अण्णहा /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ध्रुव पूर्णाङ्क width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    शून्य initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ध्रुव uint64_t combinations_गणनम् = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        तार्किक undefRolled = मिच्छ;
        auto rollUndefs = [एतत्, combinations_गणनम्] {
            प्रति (uint64_t i = 0; i < combinations_गणनम्; ++i) {
                ध्रुव auto निवेशय = m_undefinedStates.निवेशय(i);
                जइ (!निवेशय.second) m_undefinedStates.erase(निवेशय.first);
            }
        };
        प्रति (ध्रुव FsmStateVertex* ध्रुव statep : m_states) {
            ध्रुव V3_सङ्ख्या& अङ्कः = statep->मानम्().अङ्कः();
            ध्रुव uint64_t मानम् = अङ्कः.toUQuad();
            जइ (!m_isComplete) {
                जइ (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(मानम्) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } अण्णहा {
                    UASSERT_OBJ(m_undefinedStates.निवेशय(मानम्).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    जइ (m_undefinedStates.size() * 2 >= combinations_गणनम्) {
                        undefRolled = सच्च;
                        rollUndefs();
                    }
                }
            }
        }
        जइ (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* buildResetCond() ध्रुव {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव refp
            = नूतन AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        प्रत्यागम m_graph.resetCond().active_नीचः
                   ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t get_Arc_गणनम्_With_Expansion(ध्रुव VFsmExpandType expand) ध्रुव {
        size_t arc_गणनम्_With_Expansion = ~0ULL;
        विकल्प (expand) {
        पक्ष VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            भङ्ग;
        }
        पक्ष VFsmExpandType::AUTO: {
            arc_गणनम्_With_Expansion = m_edges_गणनम्;
            जइ (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_गणनम्_With_Expansion -= m_defaultVertexp->outEdges().size();
                प्रति (ध्रुव FsmStateVertex* ध्रुव statep : m_states) {
                    arc_गणनम्_With_Expansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            जइ (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_गणनम्_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            भङ्ग;
        }
        पक्ष VFsmExpandType::AUTO_EXPAND: {
            arc_गणनम्_With_Expansion = m_states.size() * m_states.size() - m_states.size();
            भङ्ग;
        }
        पक्ष VFsmExpandType::FULL: {
            ध्रुव पूर्णाङ्क width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            जइ (width < 32) {
                ध्रुव uint64_t combinations_गणनम् = static_cast<size_t>(1) << width;
                arc_गणनम्_With_Expansion = combinations_गणनम् * combinations_गणनम् - combinations_गणनम्;
            }
            भङ्ग;
        }
        }
        प्रत्यागम arc_गणनम्_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ध्रुव std::string& label,
                                           ध्रुव std::string& fsmTag = "") {
        AstCoverOtherDecl* ध्रुव declp
            = नूतन AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(सच्च);
        m_declsp = Ast_ग्रन्थिः::addNext(m_declsp, declp);
        प्रत्यागम declp;
    }

    Ast_ग्रन्थिः_Stmt* buildCoverIncStmtp(ध्रुव std::string& fromStateLabel,
                                    ध्रुव std::string& toStateLabel,
                                    ध्रुव std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ध्रुव declp = नूतन AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_graph.stateVarName() + "::" + fromStateLabel + "->" + toStateLabel,
            "",
            0,
            m_graph.stateVarName(),
            fromStateLabel,
            toStateLabel,
            baseLabel};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(सच्च);
        m_declsp = Ast_ग्रन्थिः::addNext(m_declsp, declp);
        प्रत्यागम नूतन AstCoverInc{m_flp, declp};
    }
    Ast_ग्रन्थिः_Stmt* buildExpandedTransitionHandler(ध्रुव std::string& toStateLabel,
                                                ध्रुव तार्किक considerUndefs,
                                                ध्रुव तार्किक expandOnlyWithTerminalVerticies,
                                                ध्रुव FsmStateVertex* ध्रुव toStatep = nullptr) {
        AstCase* ध्रुव arcCasep = नूतन AstCase{
            m_flp, VCaseType::CT_CASE, नूतन AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<ध्रुव FsmStateVertex*> handledSoureces;
        जइ (toStatep) {
            प्रति (ध्रुव V3GraphEdge& edge : toStatep->inEdges()) {
                जइ (ध्रुव FsmStateVertex* ध्रुव fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    जइ (!handledSoureces.emplace(fromp).second) अनुवर्त;
                    arcCasep->addItemsp(नूतन AstCaseItem{
                        m_flp, makeConst(fromp->मानम्().अङ्कः().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        प्रति (ध्रुव FsmStateVertex* ध्रुव fromp : m_states) {
            जइ (handledSoureces.find(fromp) != handledSoureces.end()) अनुवर्त;
            arcCasep->addItemsp(नूतन AstCaseItem{
                m_flp, makeConst(fromp->मानम्().अङ्कः().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        जइ (!m_isComplete) {
            जइ (considerUndefs) {
                प्रति (ध्रुव uint64_t from : m_undefinedStates) {
                    ध्रुव std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        नूतन AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } अण्णहा {
                arcCasep->addItemsp(नूतन AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        प्रत्यागम arcCasep;
    }
    Ast_ग्रन्थिः_Stmt* buildUnxpandedTransitionHandler(ध्रुव FsmStateVertex* ध्रुव toStatep) {
        AstCase* ध्रुव arcCasep = नूतन AstCase{
            m_flp, VCaseType::CT_CASE, नूतन AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        प्रति (ध्रुव V3GraphEdge& edge : toStatep->inEdges()) {
            ध्रुव FsmArcEdge* ध्रुव arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ध्रुव FsmStateVertex* ध्रुव fromp = arcp->fromp()->as<FsmStateVertex>();
            जइ (fromp != toStatep) {
                arcCasep->addItemsp(
                    नूतन AstCaseItem{m_flp, makeConst(fromp->मानम्().अङ्कः().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(नूतन AstCaseItem{m_flp, nullptr, nullptr});
        प्रत्यागम arcCasep;
    }

सार्वजनिक:
    FsmCoverIncBuilder(ध्रुव FsmGraph& graph, AstVarScope* ध्रुव prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* ध्रुव sampleVscp = graph.sampleVarScopep();
        ध्रुव VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ध्रुव size_t arc_गणनम्_With_Expansion = get_Arc_गणनम्_With_Expansion(expand);
            जइ (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_गणनम्_With_Expansion == 0ULL
                            || arc_गणनम्_With_Expansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_गणनम्_With_Expansion);
                प्रत्यागम;
            }
        }

        AstCase* ध्रुव arcCasep
            = नूतन AstCase{m_flp, VCaseType::CT_CASE,
                          नूतन AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* ध्रुव initialStateCasep
            = नूतन AstCase{m_flp, VCaseType::CT_CASE,
                          नूतन AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        ध्रुव तार्किक expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<ध्रुव FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<ध्रुव FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        जइ (!expandAll) {
            जइ (m_defaultVertexp) {
                प्रति (ध्रुव V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.शिखरम्()->as<FsmStateVertex>());
                }
            }
            जइ (m_resetVertexp) {
                प्रति (ध्रुव V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.शिखरम्()->as<FsmStateVertex>());
                }
            }
        }
        ध्रुव तार्किक isFullExpansion = expand == VFsmExpandType::FULL;
        जइ (isFullExpansion) initUndefinedStates();
        प्रति (ध्रुव FsmStateVertex* ध्रुव statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ध्रुव declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                नूतन AstCaseItem{m_flp, makeConst(statep->मानम्().अङ्कः().toUQuad()),
                                नूतन AstCoverInc{m_flp, declp, सच्च}});
            ध्रुव तार्किक expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            ध्रुव तार्किक defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(नूतन AstCaseItem{
                m_flp, makeConst(statep->मानम्().अङ्कः().toUQuad()),
                Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः_Stmt>(
                    नूतन AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        जइ (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            जइ (isFullExpansion) {
                प्रति (ध्रुव uint64_t मानम् : m_undefinedStates) {
                    ध्रुव std::string& label = quadToBitStirngLabel(मानम्);
                    AstCoverOtherDecl* ध्रुव declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(नूतन AstCaseItem{
                        m_flp, makeConst(मानम्), नूतन AstCoverInc{m_flp, declp, सच्च}});
                    arcCasep->addItemsp(
                        नूतन AstCaseItem{m_flp, makeConst(मानम्),
                                        Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः_Stmt>(
                                            नूतन AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, सच्च, मिच्छ))});
                }
            } अण्णहा {
                // No expansion
                arcCasep->addItemsp(नूतन AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(नूतन AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = स्वभाव;
    Ast_ग्रन्थिः_Cover_Decl* getCoverDeclsp() ध्रुव { प्रत्यागम m_declsp; }
    Ast_ग्रन्थिः_Stmt* getCoverIncp() ध्रुव { प्रत्यागम m_coverIncp; }
    Ast_ग्रन्थिः_Stmt* getInitialCoverIncp() ध्रुव { प्रत्यागम m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
वर्ग FsmLowerVisitor final {
    // STATE - across all visitors
    ध्रुव FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    स्थिर Ast_Sen_वृक्षः* build_Sen_वृक्षः(File_पङ्क्तिः* flp, ध्रुव std::vector<FsmSenDesc>& senses) {
        Ast_Sen_वृक्षः* ध्रुव sentreep = नूतन Ast_Sen_वृक्षः{flp, nullptr};
        प्रति (ध्रुव FsmSenDesc& sense : senses) {
            AstSenItem* ध्रुव senItemp
                = नूतन AstSenItem{flp, VEdgeType{sense.edgeType},
                                 नूतन AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        प्रत्यागम sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    शून्य buildOne(ध्रुव FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* ध्रुव alwaysp = graph.stateAlwaysp();
        AstScope* ध्रुव scopep = graph.scopep();
        AstVarScope* ध्रुव stateVscp = graph.stateVarScopep();
        AstVarScope* ध्रुव sampleVscp = graph.sampleVarScopep();
        File_पङ्क्तिः* ध्रुव flp = graph.fileline();
        Ast_ग्रन्थिः_Module* ध्रुव modp = scopep->modp();
        AstVarScope* ध्रुव prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(सच्च);
        sampleVscp->optimizeLifePost(सच्च);
        prevVscp->optimizeLifePost(सच्च);

        AstActive* ध्रुव initActivep
            = नूतन AstActive{flp, "fsm-coverage-init",
                            नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_वृक्षः_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(नूतन AstInitialStatic{
            flp, नूतन AstAssign{flp, नूतन AstVarRef{flp, prevVscp, VAccess::WRITE},
                               नूतन AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* ध्रुव covPostp = नूतन AstAlwaysPost{flp};
        तार्किक updatePrevAfterPost = मिच्छ;
        जइ (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 नूतन AstVarRef{flp, sampleVscp, VAccess::READ}});
        } अण्णहा {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = सच्च;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ध्रुव FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                नूतन AstIf{flp,
                          नूतन AstNeq{flp, नूतन AstVarRef{flp, prevVscp, VAccess::READ},
                                     नूतन AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        जइ (updatePrevAfterPost) {
            covPostp->addStmtsp(नूतन AstAssign{flp, नूतन AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              नूतन AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_वृक्षः* ध्रुव sentreep = build_Sen_वृक्षः(flp, graph.senses());
        AstActive* ध्रुव activep = नूतन AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_वृक्षः_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

सार्वजनिक:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(ध्रुव FsmState& state)
        : m_state{state} {
        प्रति (ध्रुव DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

शून्य V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    जइ (dumpGraphLevel() >= 6) {
        size_t index = 0;
        प्रति (ध्रुव DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_वृक्षः("fsm-detect", 0, dump_वृक्षः_Either_Level() >= 3);
}
