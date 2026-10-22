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
        कुञ्चिका_अङ्कः.isSigned(असत्य);
        प्रत्यागम cvt_गन्तव्यम्_Str(कुञ्चिका_अङ्कः.विस्तारः()) + ":" + कुञ्चिका_अङ्कः.ascii(सत्य, सत्य);
    }

सार्वजनिक:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_मानम्()
        : m_कुञ्चिका{"1:1'h0"}
        , m_अङ्कः{static_cast<Ast_ग्रन्थिः*>(शून्यसूचकः), 1, 0} {}
    स्पष्ट Fsm_State_मानम्(ध्रुव V3_सङ्ख्या& अङ्कः)
        : m_कुञ्चिका{make_कुञ्चिका(अङ्कः)}
        , m_अङ्कः{अङ्कः} {}

    ध्रुव string& कुञ्चिका() ध्रुव { प्रत्यागम m_कुञ्चिका; }
    ध्रुव V3_सङ्ख्या& अङ्कः() ध्रुव { प्रत्यागम m_अङ्कः; }
    string ascii() ध्रुव { प्रत्यागम m_अङ्कः.ascii(सत्य, सत्य); }
    string warnText() ध्रुव {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        यदि (m_अङ्कः.विस्तारः() <= 32) प्रत्यागम cvt_गन्तव्यम्_Str(m_अङ्कः.गन्तव्यम्_UInt());
        प्रत्यागम ascii();
    }

    तार्किक संक्रियकः==(ध्रुव Fsm_State_मानम्& rhs) ध्रुव { प्रत्यागम m_कुञ्चिका == rhs.m_कुञ्चिका; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
संरचना Fsm_State_मानम्_Hash final {
    आकार_t संक्रियकः()(ध्रुव Fsm_State_मानम्& मानम्) ध्रुव {
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
    AstVarScope* varScopep = शून्यसूचकः;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
संरचना FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = शून्यसूचकः;
    तार्किक सक्रिय_नीचः = असत्य;
};

वर्ग FsmResetArcDesc final {
    Fsm_State_मानम् m_गन्तव्यम्_मानम्;  // Encoded reset target state.
    Ast_ग्रन्थिः* m_nodep = शून्यसूचकः;  // Source node for warnings and emitted metadata.
    Ast_ग्रन्थिः_व्यञ्जकम्* m_valuep = शून्यसूचकः;  // Expression that provided the reset value.

सार्वजनिक:
    FsmResetArcDesc() = स्वभाव;
    FsmResetArcDesc(Fsm_State_मानम् गन्तव्यम्_मानम्, Ast_ग्रन्थिः_नियोजय* nodep)
        : m_गन्तव्यम्_मानम्{गन्तव्यम्_मानम्}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_मानम् गन्तव्यम्_मानम्, Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : m_गन्तव्यम्_मानम्{गन्तव्यम्_मानम्}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_मानम् गन्तव्यम्_मानम्() ध्रुव { प्रत्यागम m_गन्तव्यम्_मानम्; }
    Ast_ग्रन्थिः* nodep() ध्रुव { प्रत्यागम m_nodep; }
    Ast_ग्रन्थिः_व्यञ्जकम्* valuep() ध्रुव { प्रत्यागम m_valuep; }
};

संरचना FsmWrapperRoles final {
    string d_पत्तनम्;
    string q_पत्तनम्;
    string clk_पत्तनम्;
    string rst_पत्तनम्;
    string rstValParam;
    तार्किक has_Rst_सक्रिय_नीचः = असत्य;
    तार्किक rst_सक्रिय_नीचः = असत्य;
};

स्थिर तार्किक fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ध्रुव string& पत्तनम्_Name,
                                                  तार्किक& सक्रिय_नीचः) {
    तार्किक matched = असत्य;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp = itemp->varrefp();
        यदि (!vrefp) प्रत्यागम;
        यदि (vrefp->varp()->name() != पत्तनम्_Name) प्रत्यागम;
        सक्रिय_नीचः = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = सत्य;
    });
    प्रत्यागम matched;
}

स्थिर ध्रुव V3Control::Fsm_पञ्जिकाकोशः_Wrapper* fsm_पञ्जिकाकोशः_Wrapper_Desc(AstCell* cellp) {
    Ast_ग्रन्थिः_Module* ध्रुव modp = cellp->modp();
    ध्रुव string origName = modp->origName();
    यदि (ध्रुव V3Control::Fsm_पञ्जिकाकोशः_Wrapper* ध्रुव descp
        = V3Control::get_Fsm_पञ्जिकाकोशः_Wrapper(origName)) {
        प्रत्यागम descp;
    }
    प्रत्यागम V3Control::get_Fsm_पञ्जिकाकोशः_Wrapper(modp->prettyDehashOrigOrName());
}

स्थिर FsmWrapperRoles rolesFromDesc(ध्रुव V3Control::Fsm_पञ्जिकाकोशः_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_पत्तनम् = desc.d;
    roles.q_पत्तनम् = desc.q;
    roles.clk_पत्तनम् = desc.clock;
    roles.rst_पत्तनम् = desc.reset;
    roles.rstValParam = desc.reset_मानम्;
    प्रत्यागम roles;
}

वर्ग Fsm_पञ्जिकाकोशः_Candidate final {
    AstScope* m_scopep = शून्यसूचकः;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = शून्यसूचकः;  // Register process that commits the state.
    AstVarScope* m_stateVscp = शून्यसूचकः;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = शून्यसूचकः;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_अग्र_Vscp = शून्यसूचकः;  // Next-state variable or same state var for 1-block FSMs.
    std::सदिशम्<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::सदिशम्<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    तार्किक m_hasResetCond = असत्य;  // Whether the FSM had a modeled reset predicate.
    तार्किक m_inclCond = असत्य;  // Whether conditional/default arcs are kept explicitly.
    File_पङ्क्तिः* m_flp = शून्यसूचकः;  // Representative source location.

सार्वजनिक:
    AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }
    शून्य scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ध्रुव { प्रत्यागम m_alwaysp; }
    शून्य alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ध्रुव { प्रत्यागम m_stateVscp; }
    शून्य stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ध्रुव { प्रत्यागम m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    शून्य sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* अग्र_Vscp() ध्रुव { प्रत्यागम m_अग्र_Vscp; }
    शून्य अग्र_Vscp(AstVarScope* vscp) { m_अग्र_Vscp = vscp; }
    ध्रुव std::सदिशम्<FsmSenDesc>& senses() ध्रुव { प्रत्यागम m_senses; }
    std::सदिशम्<FsmSenDesc>& senses() { प्रत्यागम m_senses; }
    ध्रुव FsmResetCondDesc& resetCond() ध्रुव { प्रत्यागम m_resetCond; }
    FsmResetCondDesc& resetCond() { प्रत्यागम m_resetCond; }
    ध्रुव std::सदिशम्<FsmResetArcDesc>& resetArcs() ध्रुव { प्रत्यागम m_resetArcs; }
    std::सदिशम्<FsmResetArcDesc>& resetArcs() { प्रत्यागम m_resetArcs; }
    तार्किक hasResetCond() ध्रुव { प्रत्यागम m_hasResetCond; }
    शून्य hasResetCond(तार्किक flag) { m_hasResetCond = flag; }
    तार्किक inclCond() ध्रुव { प्रत्यागम m_inclCond; }
    शून्य inclCond(तार्किक flag) { m_inclCond = flag; }
    File_पङ्क्तिः* fileline() ध्रुव { प्रत्यागम m_flp; }
    शून्य fileline(File_पङ्क्तिः* flp) { m_flp = flp; }
};

वर्ग FsmComboAlways final {
    AstScope* ध्रुव m_scopep = शून्यसूचकः;  // Owning scope for the combinational process.
    AstAlways* ध्रुव m_alwaysp = शून्यसूचकः;  // Candidate transition process.

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
    गणना वर्ग विधा_2 : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

गुप्त:
    विधा_2 m_विधा;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_मानम् m_मानम्;  // Encoded state value for real state vertices.

रक्षित:
    FsmVertex(V3Graph* graphp, विधा_2 विधा, string label, Fsm_State_मानम् मानम्) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_विधा{विधा},
          m_label{label},
          m_मानम्{मानम्} {}
    ~FsmVertex() override = स्वभाव;

सार्वजनिक:
    विधा_2 विधा() ध्रुव { प्रत्यागम m_विधा; }
    तार्किक isState() ध्रुव { प्रत्यागम m_विधा == विधा_2::STATE; }
    तार्किक isResetAny() ध्रुव { प्रत्यागम m_विधा == विधा_2::RESET_ANY; }
    तार्किक isDefaultAny() ध्रुव { प्रत्यागम m_विधा == विधा_2::DEFAULT_ANY; }
    ध्रुव string& label() ध्रुव { प्रत्यागम m_label; }
    ध्रुव Fsm_State_मानम्& मानम्() ध्रुव { प्रत्यागम m_मानम्; }

    string name() ध्रुव override VL_MT_SAFE { प्रत्यागम m_label + "=" + m_मानम्.ascii(); }
};

वर्ग FsmStateVertex final : सार्वजनिक FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

सार्वजनिक:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_मानम् मानम्) VL_MT_DISABLED
        : FsmVertex{graphp, विधा_2::STATE, label, मानम्} {}
    ~FsmStateVertex() override = स्वभाव;

    string dotColor() ध्रुव override { प्रत्यागम "lightblue"; }
    string dotShape() ध्रुव override { प्रत्यागम "ellipse"; }
};

वर्ग FsmPseudoVertex final : सार्वजनिक FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

सार्वजनिक:
    FsmPseudoVertex(V3Graph* graphp, विधा_2 विधा, string label) VL_MT_DISABLED
        : FsmVertex{graphp, विधा, label, Fsm_State_मानम्{}} {}
    ~FsmPseudoVertex() override = स्वभाव;

    string name() ध्रुव override VL_MT_SAFE { प्रत्यागम label(); }
    string dotColor() ध्रुव override { प्रत्यागम isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ध्रुव override { प्रत्यागम "diamond"; }
};

वर्ग FsmArcEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    तार्किक m_isReset = असत्य;  // Arc originates from the synthetic reset source.
    तार्किक m_isCond = असत्य;  // Arc came from a conditional next-state split.
    तार्किक m_isDefault = असत्य;  // Arc represents a case default source.
    File_पङ्क्तिः* m_flp = शून्यसूचकः;  // Source location for emitted coverage metadata.

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
        यदि (m_isReset) प्रत्यागम "reset";
        यदि (m_isDefault) प्रत्यागम "default";
        यदि (m_isCond) प्रत्यागम "cond";
        प्रत्यागम "";
    }
    string dotColor() ध्रुव override {
        यदि (m_isReset) प्रत्यागम "darkgreen";
        यदि (m_isDefault) प्रत्यागम "orange";
        यदि (m_isCond) प्रत्यागम "blue";
        प्रत्यागम "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
वर्ग FsmGraph final : सार्वजनिक V3Graph {
    AstScope* m_scopep = शून्यसूचकः;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = शून्यसूचकः;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_आन्तरिकम्_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = शून्यसूचकः;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = शून्यसूचकः;  // Scoped variable sampled by coverage logic.
    std::सदिशम्<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    तार्किक m_hasResetCond = असत्य;  // Whether the detected FSM had a reset branch.
    तार्किक m_inclCond = असत्य;  // Whether conditional arcs should be kept explicitly.
    File_पङ्क्तिः* m_flp = शून्यसूचकः;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_मानम्, FsmStateVertex*, Fsm_State_मानम्_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = शून्यसूचकः;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = शून्यसूचकः;  // Synthetic default source for case defaults.

सार्वजनिक:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{नूतन FsmPseudoVertex{एतत्, FsmVertex::विधा_2::RESET_ANY, "ANY"}},
          m_defaultVertexp{नूतन FsmPseudoVertex{एतत्, FsmVertex::विधा_2::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }
    शून्य scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ध्रुव { प्रत्यागम m_stateAlwaysp; }
    शून्य stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ध्रुव string& stateVarName() ध्रुव { प्रत्यागम m_stateVarName; }
    शून्य stateVarName(ध्रुव string& name) { m_stateVarName = name; }
    ध्रुव string& state_Var_आन्तरिकम्_Name() ध्रुव { प्रत्यागम m_state_Var_आन्तरिकम्_Name; }
    शून्य state_Var_आन्तरिकम्_Name(ध्रुव string& name) { m_state_Var_आन्तरिकम्_Name = name; }
    AstVarScope* stateVarScopep() ध्रुव { प्रत्यागम m_stateVarScopep; }
    शून्य stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ध्रुव {
        प्रत्यागम m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    शून्य sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ध्रुव std::सदिशम्<FsmSenDesc>& senses() ध्रुव { प्रत्यागम m_senses; }
    std::सदिशम्<FsmSenDesc>& senses() { प्रत्यागम m_senses; }
    ध्रुव FsmResetCondDesc& resetCond() ध्रुव { प्रत्यागम m_resetCond; }
    FsmResetCondDesc& resetCond() { प्रत्यागम m_resetCond; }
    तार्किक hasResetCond() ध्रुव { प्रत्यागम m_hasResetCond; }
    शून्य hasResetCond(तार्किक flag) { m_hasResetCond = flag; }
    तार्किक inclCond() ध्रुव { प्रत्यागम m_inclCond; }
    शून्य inclCond(तार्किक flag) { m_inclCond = flag; }
    File_पङ्क्तिः* fileline() ध्रुव { प्रत्यागम m_flp; }
    शून्य fileline(File_पङ्क्तिः* flp) { m_flp = flp; }

    FsmStateVertex* सङ्कलय_State_Vertex(string label, Fsm_State_मानम् मानम्) VL_MT_DISABLED {
        FsmStateVertex* ध्रुव vertexp = नूतन FsmStateVertex{एतत्, label, मानम्};
        m_stateVertices.emplace(मानम्, vertexp);
        प्रत्यागम vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { प्रत्यागम m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { प्रत्यागम m_defaultVertexp; }
    FsmArcEdge* सङ्कलय_Arc(Fsm_State_मानम् from_मानम्, Fsm_State_मानम् गन्तव्यम्_मानम्, तार्किक isReset, तार्किक isCond,
                       तार्किक isDefault, File_पङ्क्तिः* flp) VL_MT_DISABLED {
        FsmStateVertex* ध्रुव शिखरम् = m_stateVertices.at(गन्तव्यम्_मानम्);
        FsmVertex* fromp = शून्यसूचकः;
        यदि (isReset) {
            fromp = resetAnyVertex();
        } अन्यथा यदि (isDefault) {
            fromp = defaultAnyVertex();
        } अन्यथा {
            fromp = m_stateVertices.at(from_मानम्);
        }
        प्रत्यागम नूतन FsmArcEdge{एतत्, fromp, शिखरम्, isReset, isCond, isDefault, flp};
    }

    string name() ध्रुव VL_MT_SAFE {
        प्रत्यागम "FSM "
               + (m_stateVarName.रिक्तानि() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_चिह्नकम्(आकार_t सूचकांक) ध्रुव {
        string चिह्नकम् = state_Var_आन्तरिकम्_Name();
        प्रति (वर्ण& ch : चिह्नकम्) {
            यदि (!std::अक्षरम्_अङ्कः_वा(static_cast<अचिह्नित वर्ण>(ch))) ch = '_';
        }
        प्रत्यागम "fsm_" + cvt_गन्तव्यम्_Str(सूचकांक) + "_" + चिह्नकम्;
    }
};

संरचना DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

संरचना FsmCaseCandidate final {
    Ast_ग्रन्थिः* warnNodep = शून्यसूचकः;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = शून्यसूचकः;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
संरचना FsmStateComparison final {
    AstVarScope* stateVscp = शून्यसूचकः;  // Compared state variable
    Ast_ग्रन्थिः_व्यञ्जकम्* valuep = शून्यसूचकः;  // Compared constant value expression
    Fsm_State_मानम् मानम्;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
संरचना FsmIfBranch final {
    AstIf* ifp = शून्यसूचकः;  // Source if/else-if node
    Ast_ग्रन्थिः* stmtsp = शून्यसूचकः;  // Branch body
    Ast_ग्रन्थिः_व्यञ्जकम्* valuep = शून्यसूचकः;  // Source state value expression
    Fsm_State_मानम् from_मानम्;  // Encoded source state value
    तार्किक has_शिखरम्_Guard = असत्य;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
संरचना FsmIfChainCandidate final {
    AstIf* ifp = शून्यसूचकः;  // Top-level if-chain node
    AstVarScope* compareVscp = शून्यसूचकः;  // Variable used by every state comparison
    std::सदिशम्<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_ग्रन्थिः* defaultStmtsp = शून्यसूचकः;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
उपयुज्य FsmAliasMap = std::unordered_map<ध्रुव AstVarScope*, FsmStateComparison>;
उपयुज्य Fsm_Cell_पत्तनम्_Map = std::unordered_map<string, AstVarScope*>;
उपयुज्य Fsm_Cell_पत्तनम्_Alias_Map = std::unordered_map<ध्रुव AstCell*, Fsm_Cell_पत्तनम्_Map>;

संरचना StateConstLabel final {
    string text;
    तार्किक fromParam = असत्य;
    आकार_t state_सूचकांक = 0;
};

संरचना FsmStateSpace final {
    std::सदिशम्<std::pair<string, Fsm_State_मानम्>> अवस्थाः;  // User label and encoded value
    std::unordered_map<Fsm_State_मानम्, StateConstLabel, Fsm_State_मानम्_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = शून्यसूचकः;  // Tracked FSM state variable
    तार्किक enumBacked = असत्य;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
वर्ग FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::सदिशम्<DetectedFsm> m_fsms;
    std::map<ध्रुव AstVarScope*, आकार_t> m_fsm_सूचकांक;

सार्वजनिक:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ध्रुव std::map<ध्रुव AstVarScope*, आकार_t>::const_iterator it = m_fsm_सूचकांक.find(stateVscp);
        यदि (it != m_fsm_सूचकांक.अन्तः_2()) प्रत्यागम m_fsms.at(it->second);
        ध्रुव आकार_t सूचकांक = m_fsms.आकार();
        m_fsm_सूचकांक.emplace(stateVscp, सूचकांक);
        m_fsms.emplace_पृष्ठभागः();
        प्रत्यागम m_fsms.पृष्ठभागः();
    }
    ध्रुव std::सदिशम्<DetectedFsm>& fsms() ध्रुव { प्रत्यागम m_fsms; }
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
    AstScope* m_scopep = शून्यसूचकः;
    std::सदिशम्<Fsm_पञ्जिकाकोशः_Candidate> m_पञ्जिकाकोशः_Candidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::सदिशम्<FsmComboAlways> m_one_खण्डम्_Alwayss;
    std::सदिशम्<FsmComboAlways> m_comboAlwayss;
    std::सदिशम्<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::सदिशम्<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ध्रुव AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_समुच्चयः<ध्रुव AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    Fsm_Cell_पत्तनम्_Alias_Map m_cell_पत्तनम्_Aliases;
    Fsm_Cell_पत्तनम्_Alias_Map m_cell_पत्तनम्_Child_Aliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    स्थिर Ast_ग्रन्थिः_DType* unwrapEnumCandidate(Ast_ग्रन्थिः_DType* dtypep) {
        प्रत्यागम dtypep->skip_Ref_गन्तव्यम्_Enump();
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
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक simple_Param_State_मानम्(AstCell* cellp, ध्रुव string& name, Fsm_State_मानम्& मानम्,
                                      Ast_ग्रन्थिः_व्यञ्जकम्*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ध्रुव varp = VN_CAST(stmtp, Var);
            यदि (!varp || !varp->isParam() || varp->name() != name) अनुवर्त;
            valuepr = VN_AS(varp->valuep(), ग्रन्थिः_व्यञ्जकम्);
            प्रत्यागम const_मानम्_स्थितिः(valuepr, मानम्) == Const_मानम्_स्थितिः::OK;
        }
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक child_पत्तनम्_अन्तर्_Scope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        यदि (!vscp->varp()->isIO()) प्रत्यागम असत्य;
        AstScope* ध्रुव scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        यदि (scopep->aboveScopep() != parentScopep) प्रत्यागम असत्य;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        प्रत्यागम सत्य;
    }

    स्थिर AstVarScope* simple_नियोजय_Var_Scope(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        AstVarRef* ध्रुव vrefp = VN_CAST(exprp, VarRef);
        प्रत्यागम vrefp ? vrefp->varScopep() : शून्यसूचकः;
    }

    शून्य सङ्कलय_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_पत्तनम्_Aliases.emplace(cellp, Fsm_Cell_पत्तनम्_Map{});
        m_cell_पत्तनम्_Child_Aliases.emplace(cellp, Fsm_Cell_पत्तनम्_Map{});
        ध्रुव std::pair<AstScope*, AstCell*> item{scopep, cellp};
        यदि (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            प्रत्यागम;
        }
        m_wrapperCells.emplace_पृष्ठभागः(item);
    }

    शून्य collect_Cell_पत्तनम्_Alias(Ast_नियोजय_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ध्रुव lhsVscp = simple_नियोजय_Var_Scope(nodep->lhsp());
        AstVarScope* ध्रुव rhsVscp = simple_नियोजय_Var_Scope(nodep->rhsp());
        यदि (!lhsVscp || !rhsVscp) प्रत्यागम;
        AstCell* cellp = शून्यसूचकः;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        यदि (child_पत्तनम्_अन्तर्_Scope(lhsVscp, m_scopep, cellp)) {
            यदि (!fsm_पञ्जिकाकोशः_Wrapper_Desc(cellp)) प्रत्यागम;
            UASSERT_OBJ(lhsVscp->varp()->is_निवेशः(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_पत्तनम्_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_पत्तनम्_Child_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            सङ्कलय_Wrapper_Cell(m_scopep, cellp);
        } अन्यथा यदि (child_पत्तनम्_अन्तर्_Scope(rhsVscp, m_scopep, cellp)) {
            यदि (!fsm_पञ्जिकाकोशः_Wrapper_Desc(cellp)) प्रत्यागम;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cell_पत्तनम्_Aliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cell_पत्तनम्_Child_Aliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            सङ्कलय_Wrapper_Cell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ध्रुव string& पत्तनम्_Name) ध्रुव {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ध्रुव Fsm_Cell_पत्तनम्_Map& ports = m_cell_पत्तनम्_Aliases.at(cellp);
        ध्रुव Fsm_Cell_पत्तनम्_Map::const_iterator पत्तनम्_It = ports.find(पत्तनम्_Name);
        प्रत्यागम पत्तनम्_It == ports.अन्तः_2() ? शून्यसूचकः : पत्तनम्_It->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, ध्रुव string& पत्तनम्_Name) ध्रुव {
        ध्रुव Fsm_Cell_पत्तनम्_Map& ports = m_cell_पत्तनम्_Child_Aliases.at(cellp);
        ध्रुव Fsm_Cell_पत्तनम्_Map::const_iterator पत्तनम्_It = ports.find(पत्तनम्_Name);
        प्रत्यागम पत्तनम्_It == ports.अन्तः_2() ? शून्यसूचकः : पत्तनम्_It->second;
    }

    वर्ग पञ्जिकाकोशः_Always_Analyzer final {
        AstScope* ध्रुव m_scopep;  // Scope containing the always block(s) being analyzed

    सार्वजनिक:
        स्पष्ट पञ्जिकाकोशः_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::सदिशम्<std::pair<AstCase*, Ast_ग्रन्थिः_व्यञ्जकम्*>>
        one_खण्डम्_Candidates(AstAlways* alwaysp) ध्रुव {
            std::सदिशम्<std::pair<AstCase*, Ast_ग्रन्थिः_व्यञ्जकम्*>> candidates;
            Ast_ग्रन्थिः* ध्रुव stmtsp = alwaysp->stmtsp();
            यदि (AstIf* ध्रुव firstIfp = VN_CAST(stmtsp, If)) {
                यदि (AstCase* ध्रुव casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_पृष्ठभागः(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : शून्यसूचकः);
                }
            }
            प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                यदि (AstCase* ध्रुव casep = VN_CAST(nodep, Case))
                    candidates.emplace_पृष्ठभागः(casep, शून्यसूचकः);
            }
            प्रत्यागम candidates;
        }

        std::सदिशम्<std::pair<AstIf*, Ast_ग्रन्थिः_व्यञ्जकम्*>>
        one_खण्डम्_If_Candidates(AstAlways* alwaysp) ध्रुव {
            std::सदिशम्<std::pair<AstIf*, Ast_ग्रन्थिः_व्यञ्जकम्*>> candidates;
            Ast_ग्रन्थिः* ध्रुव stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            यदि (AstIf* ध्रुव firstIfp = VN_CAST(stmtsp, If)) {
                यदि (AstIf* ध्रुव chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_पृष्ठभागः(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : शून्यसूचकः);
                }
            }
            प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                यदि (AstIf* ध्रुव ifp = VN_CAST(nodep, If)) candidates.emplace_पृष्ठभागः(ifp, शून्यसूचकः);
            }
            प्रत्यागम candidates;
        }

        तार्किक मेलनम्_पञ्जिकाकोशः_Candidate(AstAlways* alwaysp, Fsm_पञ्जिकाकोशः_Candidate& cand) ध्रुव {
            प्रत्यागम FsmDetectVisitor::मेलनम्_पञ्जिकाकोशः_Always(alwaysp, m_scopep, cand);
        }

        शून्य build_One_खण्डम्_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_ग्रन्थिः_व्यञ्जकम्* resetCondp,
                                    Fsm_पञ्जिकाकोशः_Candidate& reg) ध्रुव {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.अग्र_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_वृक्षः(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != शून्यसूचकः);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ध्रुव firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            यदि (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = शून्यसूचकः;
                ध्रुव Reset_नियोजय_स्थितिः reset_स्थितिः = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                यदि (reset_स्थितिः == Reset_नियोजय_स्थितिः::रिक्तानि_3) {
                    reg.resetArcs().clear();
                } अन्यथा यदि (reset_स्थितिः == Reset_नियोजय_स्थितिः::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_मानम् reset_मानम्;
                    Ast_ग्रन्थिः* ध्रुव thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_ग्रन्थिः_नियोजय* ध्रुव resetAssp = FsmDetectVisitor::प्रत्यक्षम्_Const_State_नियोजय_ग्रन्थिः(
                        thenNodep, resetStateVscp, reset_मानम्);
                    यदि (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_पृष्ठभागः(reset_मानम्, resetAssp);
                    }
                } अन्यथा यदि (reset_स्थितिः == Reset_नियोजय_स्थितिः::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    तार्किक मेलनम्_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_पञ्जिकाकोशः_Candidate& cand) ध्रुव {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_पञ्जिकाकोशः_Wrapper_Desc(cellp));

        AstVarScope* ध्रुव अग्र_Vscp = roleVarScope(cellp, roles.d_पत्तनम्);
        AstVarScope* ध्रुव stateVscp = roleVarScope(cellp, roles.q_पत्तनम्);
        यदि (!अग्र_Vscp || !stateVscp) {
            प्रत्यागम rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ध्रुव clkVscp = roleVarScope(cellp, roles.clk_पत्तनम्);
        यदि (!clkVscp) {
            प्रत्यागम rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().प्रक्षिप_पृष्ठभागः(clkSense);

        AstVarScope* resetVscp = शून्यसूचकः;
        यदि (!roles.rst_पत्तनम्.रिक्तानि()) resetVscp = roleVarScope(cellp, roles.rst_पत्तनम्);
        यदि (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            तार्किक inferred_सक्रिय_नीचः = असत्य;
            यदि (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_पत्तनम्, inferred_सक्रिय_नीचः)) {
                roles.has_Rst_सक्रिय_नीचः = सत्य;
                roles.rst_सक्रिय_नीचः = inferred_सक्रिय_नीचः;
            }
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* resetValuep = शून्यसूचकः;
        Fsm_State_मानम् reset_मानम्;
        ध्रुव तार्किक has_Reset_मानम्
            = !roles.rstValParam.रिक्तानि()
              && simple_Param_State_मानम्(cellp, roles.rstValParam, reset_मानम्, resetValuep);
        यदि (resetVscp && roles.has_Rst_सक्रिय_नीचः && has_Reset_मानम्) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_सक्रिय_नीचः ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().प्रक्षिप_पृष्ठभागः(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().सक्रिय_नीचः = roles.rst_सक्रिय_नीचः;
            cand.hasResetCond(सत्य);
            cand.resetArcs().emplace_पृष्ठभागः(reset_मानम्, cellp, resetValuep);
        } अन्यथा यदि (!roles.rst_पत्तनम्.रिक्तानि() || !roles.rstValParam.रिक्तानि()) {
            string reason;
            यदि (roles.rst_पत्तनम्.रिक्तानि()) {
                reason = "reset port is not configured";
            } अन्यथा यदि (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } अन्यथा यदि (!roles.has_Rst_सक्रिय_नीचः) {
                reason = "reset polarity could not be inferred from the wrapper";
            } अन्यथा यदि (roles.rstValParam.रिक्तानि()) {
                reason = "reset_value parameter is not configured";
            } अन्यथा {
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
        cand.alwaysp(शून्यसूचकः);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(childRoleVarScope(cellp, roles.q_पत्तनम्));
        cand.अग्र_Vscp(अग्र_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        प्रत्यागम सत्य;
    }

    वर्ग ComboAlwaysAnalyzer final {
    सार्वजनिक:
        संरचना Combo_मेलनम् final {
            ध्रुव Fsm_पञ्जिकाकोशः_Candidate* matchedp = शून्यसूचकः;
            Ast_ग्रन्थिः* warnNodep = शून्यसूचकः;
        };

    गुप्त:
        ध्रुव std::सदिशम्<Fsm_पञ्जिकाकोशः_Candidate>& m_पञ्जिकाकोशः_Candidates;

    सार्वजनिक:
        स्पष्ट ComboAlwaysAnalyzer(ध्रुव std::सदिशम्<Fsm_पञ्जिकाकोशः_Candidate>& पञ्जिकाकोशः_Candidates)
            : m_पञ्जिकाकोशः_Candidates{पञ्जिकाकोशः_Candidates} {}

        Combo_मेलनम् मेलनम्_Case(Ast_ग्रन्थिः* stmtsp, AstCase* casep) ध्रुव {
            Combo_मेलनम् मेलनम्;
            AstVarRef* ध्रुव selp = VN_CAST(casep->exprp(), VarRef);
            यदि (!selp) प्रत्यागम मेलनम्;
            प्रति (ध्रुव Fsm_पञ्जिकाकोशः_Candidate& reg : m_पञ्जिकाकोशः_Candidates) {
                यदि (selp->varScopep() == reg.अग्र_Vscp()) {
                    यदि (!FsmDetectVisitor::has_Canonical_अग्र_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.अग्र_Vscp())) {
                        अनुवर्त;
                    }
                } अन्यथा यदि (selp->varScopep() != reg.stateVscp()) {
                    अनुवर्त;
                }
                Ast_ग्रन्थिः* ध्रुव warnNodep = FsmDetectVisitor::case_Supported_Transition_ग्रन्थिः(
                    casep, reg.अग्र_Vscp(), reg.inclCond());
                यदि (!warnNodep) अनुवर्त;
                मेलनम्.matchedp = &reg;
                मेलनम्.warnNodep = warnNodep;
            }
            प्रत्यागम मेलनम्;
        }

        Combo_मेलनम् मेलनम्_If_Chain(Ast_ग्रन्थिः* stmtsp, ध्रुव FsmIfChainCandidate& chain) ध्रुव {
            Combo_मेलनम् मेलनम्;
            प्रति (ध्रुव Fsm_पञ्जिकाकोशः_Candidate& reg : m_पञ्जिकाकोशः_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                यदि (chain.compareVscp == reg.अग्र_Vscp()) {
                    यदि (!FsmDetectVisitor::has_Canonical_अग्र_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.अग्र_Vscp())) {
                        अनुवर्त;
                    }
                } अन्यथा यदि (chain.compareVscp != reg.stateVscp()) {
                    अनुवर्त;
                }
                Ast_ग्रन्थिः* ध्रुव warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_ग्रन्थिः(chain, reg.अग्र_Vscp());
                यदि (!warnNodep) अनुवर्त;
                मेलनम्.matchedp = &reg;
                मेलनम्.warnNodep = warnNodep;
            }
            प्रत्यागम मेलनम्;
        }

        तार्किक shouldWarnUnsupported(Ast_ग्रन्थिः* stmtsp, AstCase* casep) ध्रुव {
            ध्रुव AstVarRef* ध्रुव selp = VN_CAST(casep->exprp(), VarRef);
            यदि (!selp) प्रत्यागम असत्य;

            प्रत्यागम std::any_सम्बन्धि(
                m_पञ्जिकाकोशः_Candidates.cbegin(), m_पञ्जिकाकोशः_Candidates.cend(),
                [&](ध्रुव Fsm_पञ्जिकाकोशः_Candidate& reg) -> तार्किक {
                    ध्रुव तार्किक matches_अग्र = selp->varScopep() == reg.अग्र_Vscp();
                    ध्रुव तार्किक matchesState = selp->varScopep() == reg.stateVscp();

                    यदि (!matches_अग्र && !matchesState) प्रत्यागम असत्य;
                    यदि (matches_अग्र
                        && !FsmDetectVisitor::has_Canonical_अग्र_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.अग्र_Vscp())) {
                        प्रत्यागम असत्य;
                    }
                    प्रत्यागम FsmDetectVisitor::case_Supported_Transition_ग्रन्थिः(casep, reg.अग्र_Vscp(),
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
        यदि (AstVarRef* ध्रुव vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        प्रत्यागम desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    स्थिर std::सदिशम्<FsmSenDesc> describe_Sen_वृक्षः(Ast_Sen_वृक्षः* sentreep) {
        std::सदिशम्<FsmSenDesc> senses;
        प्रति (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_ग्रन्थिः_Var_Ref* ध्रुव vrefp = itemp->varrefp();
            यदि (!vrefp) अनुवर्त;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.प्रक्षिप_पृष्ठभागः(desc);
        }
        प्रत्यागम senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    स्थिर तार्किक isIgnorableStmt(Ast_ग्रन्थिः* nodep) { प्रत्यागम VN_IS(nodep, CoverInc); }

    स्थिर Ast_ग्रन्थिः* skipLeadingIgnorableStmt(Ast_ग्रन्थिः* nodep) {
        यावत् (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        प्रत्यागम nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    स्थिर Ast_ग्रन्थिः* singleMeaningfulStmt(Ast_ग्रन्थिः* stmtp) {
        Ast_ग्रन्थिः* resultp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            यदि (isIgnorableStmt(nodep)) अनुवर्त;
            यदि (resultp) प्रत्यागम शून्यसूचकः;
            resultp = nodep;
        }
        प्रत्यागम resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    स्थिर Ast_ग्रन्थिः* singleMeaningfulBranch(Ast_ग्रन्थिः* branchp) {
        यदि (!branchp) प्रत्यागम शून्यसूचकः;
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
            यदि (senp->edgeType() == VEdgeType::ET_परिवर्तितम्) अनुवर्त;
            प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }

    शून्य warnUnsupportedComboAlways(ध्रुव FsmComboAlways& combo) {
        ध्रुव ComboAlwaysAnalyzer analyzer{m_पञ्जिकाकोशः_Candidates};
        Ast_ग्रन्थिः* ध्रुव stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        तार्किक warned = असत्य;
        प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ध्रुव casep = VN_CAST(nodep, Case);
            यदि (!casep) अनुवर्त;
            यदि (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = सत्य;
            }
            यदि (warned) भङ्ग;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    स्थिर Ast_ग्रन्थिः_नियोजय* प्रत्यक्षम्_State_नियोजय(Ast_ग्रन्थिः* stmtp, AstVarScope* stateVscp) {
        Ast_ग्रन्थिः* ध्रुव nodep = singleMeaningfulBranch(stmtp);
        यदि (!nodep) प्रत्यागम शून्यसूचकः;
        Ast_ग्रन्थिः_नियोजय* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_नियोजय);
        यदि (!assp) प्रत्यागम शून्यसूचकः;
        AstVarRef* ध्रुव vrefp = VN_CAST(assp->lhsp(), VarRef);
        यदि (!vrefp || vrefp->varScopep() != stateVscp) प्रत्यागम शून्यसूचकः;
        प्रत्यागम assp;
    }

    स्थिर Ast_ग्रन्थिः_नियोजय* ग्रन्थिः_State_Var_नियोजय(Ast_ग्रन्थिः* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_ग्रन्थिः_नियोजय* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_नियोजय);
        यदि (!assp) प्रत्यागम शून्यसूचकः;
        AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp()->आधारः_Fromp(सत्य), VarRef);
        AstVarRef* ध्रुव rhsp = VN_CAST(assp->rhsp(), VarRef);
        यदि (!rhsp || !lhsp) प्रत्यागम शून्यसूचकः;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        प्रत्यागम assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    स्थिर Ast_ग्रन्थिः_नियोजय* प्रत्यक्षम्_Cond_State_Var_नियोजय(Ast_ग्रन्थिः* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_ग्रन्थिः_व्यञ्जकम्*& condp,
                                                   Fsm_State_मानम्& reset_मानम्) {
        Ast_ग्रन्थिः_नियोजय* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_नियोजय);
        यदि (!assp) प्रत्यागम शून्यसूचकः;
        AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp()->आधारः_Fromp(सत्य), VarRef);
        AstCond* ध्रुव rhsp = VN_CAST(assp->rhsp(), Cond);
        यदि (!rhsp || !lhsp) प्रत्यागम शून्यसूचकः;
        AstVarRef* ध्रुव elsep = VN_CAST(rhsp->elsep(), VarRef);
        यदि (!elsep) प्रत्यागम शून्यसूचकः;
        यदि (const_मानम्_स्थितिः(rhsp->thenp(), reset_मानम्) != Const_मानम्_स्थितिः::OK) प्रत्यागम शून्यसूचकः;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        प्रत्यागम assp;
    }

    स्थिर Ast_ग्रन्थिः_नियोजय* प्रत्यक्षम्_Const_State_नियोजय_ग्रन्थिः(Ast_ग्रन्थिः* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_मानम्& मानम्) {
        Ast_ग्रन्थिः_नियोजय* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_नियोजय);
        यदि (!assp) प्रत्यागम शून्यसूचकः;
        AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp()->आधारः_Fromp(सत्य), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        यदि (const_मानम्_स्थितिः(assp->rhsp(), मानम्) != Const_मानम्_स्थितिः::OK) प्रत्यागम शून्यसूचकः;
        stateVscp = lhsp->varScopep();
        प्रत्यागम assp;
    }

    गणना वर्ग Reset_नियोजय_स्थितिः : uint8_t {
        रिक्तानि_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    स्थिर Reset_नियोजय_स्थितिः collectConstStateAssigns(Ast_ग्रन्थिः* stmtp, AstVarScope*& stateVscp,
                                                      std::सदिशम्<FsmResetArcDesc>& resetArcs) {
        Ast_ग्रन्थिः* nodep = skipLeadingIgnorableStmt(stmtp);
        यदि (!nodep) प्रत्यागम Reset_नियोजय_स्थितिः::रिक्तानि_3;
        प्रति (;; nodep = nodep->nextp()) {
            AstVarScope* नियोजय_State_Vscp = शून्यसूचकः;
            Fsm_State_मानम् मानम्;
            Ast_ग्रन्थिः_नियोजय* ध्रुव assp = प्रत्यक्षम्_Const_State_नियोजय_ग्रन्थिः(nodep, नियोजय_State_Vscp, मानम्);
            यदि (!assp) प्रत्यागम Reset_नियोजय_स्थितिः::NONE;
            यदि (!stateVscp) stateVscp = नियोजय_State_Vscp;
            यदि (नियोजय_State_Vscp != stateVscp) प्रत्यागम Reset_नियोजय_स्थितिः::NONE;
            यदि (!resetArcs.रिक्तानि()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                प्रत्यागम Reset_नियोजय_स्थितिः::MULTI_SAME_STATE;
            }
            resetArcs.emplace_पृष्ठभागः(मानम्, assp);
            यदि (!nodep->nextp()) प्रत्यागम Reset_नियोजय_स्थितिः::SINGLE;
        }
    }

    स्थिर तार्किक has_Canonical_अग्र_State_Default_Before_Case(Ast_ग्रन्थिः* stmtsp, Ast_ग्रन्थिः* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* अग्र_Vscp) {
        Ast_ग्रन्थिः* ध्रुव bodyp = skipLeadingIgnorableStmt(stmtsp);
        तार्किक sawCanonicalDefault = असत्य;
        प्रति (Ast_ग्रन्थिः* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            यदि (nodep == targetp) प्रत्यागम sawCanonicalDefault;
            यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_नियोजय)) {
                AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ध्रुव rhsp = VN_CAST(assp->rhsp(), VarRef);
                यदि (!lhsp || lhsp->varScopep() != अग्र_Vscp) अनुवर्त;
                यदि (sawCanonicalDefault) {
                    ध्रुव string अग्र_Name = अग्र_Vscp->varp()->prettyNameQ();
                    ध्रुव string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + अग्र_Name
                                     + ") when the canonical " + अग्र_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    प्रत्यागम असत्य;
                }
                यदि (!rhsp || rhsp->varScopep() != stateVscp) प्रत्यागम असत्य;
                sawCanonicalDefault = सत्य;
            }
        }
    }

    स्थिर तार्किक if_State_Const_नियोजय(Ast_ग्रन्थिः* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_मानम्& then_मानम्, Fsm_State_मानम्& else_मानम्) {
        AstIf* ध्रुव ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        यदि (!ifp || !ifp->elsesp()) प्रत्यागम असत्य;
        AstVarScope* thenVscp = शून्यसूचकः;
        AstVarScope* elseVscp = शून्यसूचकः;
        Ast_ग्रन्थिः* ध्रुव thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_ग्रन्थिः* ध्रुव elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        यदि (!thenNodep || !elseNodep) प्रत्यागम असत्य;
        यदि (!प्रत्यक्षम्_Const_State_नियोजय_ग्रन्थिः(thenNodep, thenVscp, then_मानम्)) प्रत्यागम असत्य;
        यदि (!प्रत्यक्षम्_Const_State_नियोजय_ग्रन्थिः(elseNodep, elseVscp, else_मानम्)) प्रत्यागम असत्य;
        यदि (thenVscp == stateVscp && elseVscp == stateVscp) प्रत्यागम सत्य;
        यदि (thenVscp != elseVscp) प्रत्यागम असत्य;
        Ast_ग्रन्थिः* ध्रुव followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = शून्यसूचकः;
        AstVarScope* finalFromVscp = शून्यसूचकः;
        Ast_ग्रन्थिः* ध्रुव finalNodep = singleMeaningfulBranch(followp);
        यदि (!finalNodep) प्रत्यागम असत्य;
        यदि (!ग्रन्थिः_State_Var_नियोजय(finalNodep, finalStateVscp, finalFromVscp)) प्रत्यागम असत्य;
        यदि (finalStateVscp != stateVscp) प्रत्यागम असत्य;
        यदि (finalFromVscp != thenVscp) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }

    स्थिर तार्किक प्रत्यक्षम्_State_Cond_Const_नियोजय(Ast_ग्रन्थिः* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_मानम्& then_मानम्, Fsm_State_मानम्& else_मानम्) {
        Ast_ग्रन्थिः_नियोजय* ध्रुव assp = प्रत्यक्षम्_State_नियोजय(stmtp, stateVscp);
        यदि (!assp) प्रत्यागम असत्य;
        AstCond* ध्रुव condp = VN_CAST(assp->rhsp(), Cond);
        यदि (!condp) प्रत्यागम असत्य;
        प्रत्यागम const_मानम्_स्थितिः(condp->thenp(), then_मानम्) == Const_मानम्_स्थितिः::OK
               && const_मानम्_स्थितिः(condp->elsep(), else_मानम्) == Const_मानम्_स्थितिः::OK;
    }

    स्थिर Ast_ग्रन्थिः* case_Item_Supported_Arc_ग्रन्थिः(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             तार्किक inclCond) {
        यदि (itemp->isDefault()) {
            यदि (!inclCond) प्रत्यागम शून्यसूचकः;
        }
        Ast_ग्रन्थिः_नियोजय* ध्रुव assp = प्रत्यक्षम्_State_नियोजय(itemp->stmtsp(), stateVscp);
        यदि (assp) {
            Fsm_State_मानम् गन्तव्यम्_मानम्;
            यदि (const_मानम्_स्थितिः(assp->rhsp(), गन्तव्यम्_मानम्) == Const_मानम्_स्थितिः::OK) प्रत्यागम assp;
        }
        Fsm_State_मानम् then_मानम्;
        Fsm_State_मानम् else_मानम्;
        यदि (प्रत्यक्षम्_State_Cond_Const_नियोजय(itemp->stmtsp(), stateVscp, then_मानम्, else_मानम्)) {
            प्रत्यागम assp;
        }
        यदि (if_State_Const_नियोजय(itemp->stmtsp(), stateVscp, then_मानम्, else_मानम्)) {
            प्रत्यागम singleMeaningfulBranch(itemp->stmtsp());
        }
        प्रत्यागम शून्यसूचकः;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    स्थिर Ast_ग्रन्थिः* case_Supported_Transition_ग्रन्थिः(AstCase* casep, AstVarScope* stateVscp,
                                                तार्किक inclCond) {
        प्रति (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            यदि (Ast_ग्रन्थिः* ध्रुव nodep = case_Item_Supported_Arc_ग्रन्थिः(itemp, stateVscp, inclCond))
                प्रत्यागम nodep;
        }
        प्रत्यागम शून्यसूचकः;
    }

    स्थिर Ast_ग्रन्थिः* case_Item_Supported_Arc_ग्रन्थिः_Like(Ast_ग्रन्थिः* stmtsp, AstVarScope* stateVscp) {
        यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assp = प्रत्यक्षम्_State_नियोजय(stmtsp, stateVscp)) {
            Fsm_State_मानम् गन्तव्यम्_मानम्;
            यदि (const_मानम्_स्थितिः(assp->rhsp(), गन्तव्यम्_मानम्) == Const_मानम्_स्थितिः::OK) प्रत्यागम assp;
            Fsm_State_मानम् then_मानम्;
            Fsm_State_मानम् else_मानम्;
            यदि (प्रत्यक्षम्_State_Cond_Const_नियोजय(stmtsp, stateVscp, then_मानम्, else_मानम्)) प्रत्यागम assp;
        }
        प्रत्यागम शून्यसूचकः;
    }

    स्थिर Ast_ग्रन्थिः* if_Chain_Supported_Transition_ग्रन्थिः(ध्रुव FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        प्रति (आकार_t i = 0; i < chain.branches.आकार(); ++i) {
            ध्रुव FsmIfBranch& branch = chain.branches[i];
            Ast_ग्रन्थिः* ध्रुव nodep = case_Item_Supported_Arc_ग्रन्थिः_Like(branch.stmtsp, stateVscp);
            यदि (!nodep) प्रत्यागम शून्यसूचकः;
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
    गणना वर्ग Const_मानम्_स्थितिः : uint8_t { OK, NOT_CONST, XZ };

    स्थिर Const_मानम्_स्थितिः const_मानम्_स्थितिः(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Fsm_State_मानम्& मानम्) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(exprp, Const);
        यदि (!constp) प्रत्यागम Const_मानम्_स्थितिः::NOT_CONST;
        ध्रुव V3_सङ्ख्या& अङ्कः = constp->अङ्कः();
        यदि (अङ्कः.isAnyXZ()) प्रत्यागम Const_मानम्_स्थितिः::XZ;
        मानम् = Fsm_State_मानम्{अङ्कः};
        प्रत्यागम Const_मानम्_स्थितिः::OK;
    }

    स्थिर तार्किक विशुद्धम्_State_Comparison_No_Alias(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, FsmStateComparison& cmp) {
        AstEq* ध्रुव eqp = VN_CAST(exprp, Eq);
        यदि (!eqp) प्रत्यागम असत्य;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = eqp->rhsp();
        यदि (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->आधारः_Fromp(सत्य), VarRef);
            यदि (!vrefp) { प्रत्यागम असत्य; }
            valuep = eqp->lhsp();
        }

        Fsm_State_मानम् मानम्;
        यदि (const_मानम्_स्थितिः(valuep, मानम्) != Const_मानम्_स्थितिः::OK) प्रत्यागम असत्य;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.मानम् = मानम्;
        प्रत्यागम सत्य;
    }

    स्थिर तार्किक विशुद्धम्_State_Comparison(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        यदि (विशुद्धम्_State_Comparison_No_Alias(exprp, cmp)) प्रत्यागम सत्य;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        यदि (AstVarRef* ध्रुव vrefp = VN_CAST(exprp, VarRef)) {
            ध्रुव FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            यदि (it == aliases.अन्तः_2()) प्रत्यागम असत्य;
            cmp = it->second;
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक supported_शिखरम्_Level_Guard(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        यदि (VN_IS(exprp, Or)) प्रत्यागम असत्य;
        यदि (VN_IS(exprp, RedAnd)) प्रत्यागम असत्य;
        यदि (VN_IS(exprp, RedOr)) प्रत्यागम असत्य;
        यदि (VN_IS(exprp, RedXor)) प्रत्यागम असत्य;
        प्रत्यागम सत्य;
    }

    स्थिर तार्किक resolveIfPredicate(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, ध्रुव FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, तार्किक& hasGuard) {
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> terms;
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> pending;
        pending.प्रक्षिप_पृष्ठभागः(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        यावत् (!pending.रिक्तानि()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep = pending.पृष्ठभागः();
            pending.निष्कासय_पृष्ठभागः();
            यदि (AstAnd* ध्रुव andp = VN_CAST(nodep, And)) {
                pending.प्रक्षिप_पृष्ठभागः(andp->rhsp());
                pending.प्रक्षिप_पृष्ठभागः(andp->lhsp());
            } अन्यथा {
                terms.प्रक्षिप_पृष्ठभागः(nodep);
            }
        }

        तार्किक sawComparison = असत्य;
        प्रति (आकार_t i = 0; i < terms.आकार(); ++i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp = terms[i];
            FsmStateComparison पदांशः_Cmp;
            यदि (विशुद्धम्_State_Comparison(termp, aliases, पदांशः_Cmp /*ref*/)) {
                यदि (sawComparison) प्रत्यागम असत्य;
                cmp = पदांशः_Cmp;
                sawComparison = सत्य;
                अनुवर्त;
            }
            यदि (!supported_शिखरम्_Level_Guard(termp)) प्रत्यागम असत्य;
            hasGuard = सत्य;
        }
        प्रत्यागम sawComparison;
    }

    स्थिर शून्य सङ्कलय_Alias(FsmAliasMap& aliases, std::unordered_समुच्चयः<ध्रुव AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ध्रुव FsmStateComparison& cmp) {
        यदि (ambiguous.find(aliasVscp) != ambiguous.अन्तः_2()) प्रत्यागम;
        ध्रुव FsmAliasMap::iterator it = aliases.find(aliasVscp);
        यदि (it == aliases.अन्तः_2()) {
            aliases.emplace(aliasVscp, cmp);
            प्रत्यागम;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        यदि (it->second.stateVscp == cmp.stateVscp && it->second.मानम् == cmp.मानम्) प्रत्यागम;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        प्रत्यागम;
    }

    स्थिर शून्य collect_Alias_From_नियोजय(Ast_ग्रन्थिः_नियोजय* assp, FsmAliasMap& aliases,
                                       std::unordered_समुच्चयः<ध्रुव AstVarScope*>& ambiguous) {
        AstVarRef* ध्रुव lhsp = VN_CAST(assp->lhsp(), VarRef);
        यदि (!lhsp) प्रत्यागम;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        यदि (!विशुद्धम्_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) प्रत्यागम;
        सङ्कलय_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_ग्रन्थिः* stmtsp, Ast_ग्रन्थिः* limitp) ध्रुव {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_समुच्चयः<ध्रुव AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        प्रति (Ast_ग्रन्थिः* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assp = VN_CAST(nodep, ग्रन्थिः_नियोजय)) {
                collect_Alias_From_नियोजय(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        प्रति (ध्रुव AstVarScope* ध्रुव vscp : ambiguous) aliases.erase(vscp);
        प्रत्यागम aliases;
    }

    स्थिर तार्किक collectIfChain(AstIf* ifp, ध्रुव FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_समुच्चयः<string> आलोकितम्_मानानि;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        यावत् (सत्य) {
            FsmStateComparison cmp;
            तार्किक hasGuard = असत्य;
            यदि (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) प्रत्यागम असत्य;
            यदि (chain.compareVscp && chain.compareVscp != cmp.stateVscp) प्रत्यागम असत्य;
            यदि (!आलोकितम्_मानानि.निवेशय(cmp.मानम्.कुञ्चिका()).second) प्रत्यागम असत्य;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.प्रक्षिप_पृष्ठभागः(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.मानम्, hasGuard});

            Ast_ग्रन्थिः* ध्रुव elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            यदि (!elseNodep) भङ्ग;
            यदि (AstIf* ध्रुव elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                अनुवर्त;
            }
            chain.defaultStmtsp = elseNodep;
            भङ्ग;
        }
        प्रत्यागम chain.branches.आकार() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    स्थिर तार्किक validate_ज्ञातम्_State_मानम्(Ast_ग्रन्थिः* nodep, ध्रुव FsmStateSpace& stateSpace,
                                        ध्रुव Fsm_State_मानम्& मानम्, ध्रुव string& role) {
        यदि (stateSpace.labels.find(मानम्) != stateSpace.labels.अन्तः_2()) प्रत्यागम सत्य;
        यदि (stateSpace.enumBacked) {
            ध्रुव string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + मानम्.warnText()
                                        + " is not present in the declared enum");
            प्रत्यागम असत्य;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + मानम्.warnText()
                                    + " is not present in the inferred state space");
        प्रत्यागम असत्य;
    }

    स्थिर StateConstLabel stateLabelForConst(AstConst* constp) {
        ध्रुव string name = constp->origParamName();
        यदि (!name.रिक्तानि()) प्रत्यागम StateConstLabel{Ast_ग्रन्थिः::prettyName(name), सत्य, 0};
        प्रत्यागम StateConstLabel{constp->name(), असत्य, 0};
    }

    स्थिर शून्य updateStateLabel(FsmStateSpace& stateSpace, ध्रुव Fsm_State_मानम्& मानम्,
                                 ध्रुव StateConstLabel& label) {
        stateSpace.अवस्थाः.at(stateSpace.labels.at(मानम्).state_सूचकांक).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    स्थिर तार्किक मेलनम्_पञ्जिकाकोशः_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_पञ्जिकाकोशः_Candidate& cand) {
        यदि (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) प्रत्यागम असत्य;

        Ast_ग्रन्थिः* ध्रुव stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_ग्रन्थिः* ध्रुव nodep = singleMeaningfulStmt(stmtsp);
        यदि (!nodep) प्रत्यागम असत्य;

        AstVarScope* stateVscp = शून्यसूचकः;
        AstVarScope* अग्र_Vscp = शून्यसूचकः;
        यदि (AstIf* ध्रुव ifp = VN_CAST(nodep, If)) {
            यदि (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) प्रत्यागम असत्य;
            AstVarScope* resetStateVscp = शून्यसूचकः;
            ध्रुव Reset_नियोजय_स्थितिः reset_स्थितिः
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ध्रुव तार्किक रिक्तानि_Reset_Branch = reset_स्थितिः == Reset_नियोजय_स्थितिः::रिक्तानि_3;
            यदि (रिक्तानि_Reset_Branch) {
                cand.resetArcs().clear();
            } अन्यथा यदि (reset_स्थितिः == Reset_नियोजय_स्थितिः::NONE) {
                cand.resetArcs().clear();
                Fsm_State_मानम् reset_मानम्;
                Ast_ग्रन्थिः* ध्रुव thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_ग्रन्थिः_नियोजय* ध्रुव resetAssp
                    = प्रत्यक्षम्_Const_State_नियोजय_ग्रन्थिः(thenNodep, resetStateVscp, reset_मानम्);
                यदि (!resetAssp) प्रत्यागम असत्य;
                cand.resetArcs().emplace_पृष्ठभागः(reset_मानम्, resetAssp);
            } अन्यथा यदि (reset_स्थितिः == Reset_नियोजय_स्थितिः::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_ग्रन्थिः* ध्रुव elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            यदि (!ग्रन्थिः_State_Var_नियोजय(elseNodep, stateVscp, अग्र_Vscp)) प्रत्यागम असत्य;
            यदि (!रिक्तानि_Reset_Branch && resetStateVscp != stateVscp) प्रत्यागम असत्य;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != शून्यसूचकः);
        } अन्यथा {
            Ast_ग्रन्थिः_व्यञ्जकम्* resetCondp = शून्यसूचकः;
            Fsm_State_मानम् reset_मानम्;
            // Note the active low 'reset ? next : CONST' form is not accepted
            यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assp
                = प्रत्यक्षम्_Cond_State_Var_नियोजय(nodep, stateVscp, अग्र_Vscp, resetCondp, reset_मानम्)) {
                cand.resetArcs().emplace_पृष्ठभागः(reset_मानम्, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != शून्यसूचकः);
            } अन्यथा यदि (!ग्रन्थिः_State_Var_नियोजय(nodep, stateVscp, अग्र_Vscp)) {
                प्रत्यागम असत्य;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.अग्र_Vscp(अग्र_Vscp);
        cand.senses() = describe_Sen_वृक्षः(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        प्रत्यागम सत्य;
    }

    स्थिर तार्किक सङ्कलय_मानम्_गन्तव्यम्_State_Space(Ast_ग्रन्थिः* nodep, FsmStateSpace& stateSpace,
                                     ध्रुव Fsm_State_मानम्& मानम्, StateConstLabel label) {
        ध्रुव स्वचालित labelIt = stateSpace.labels.find(मानम्);
        यदि (labelIt != stateSpace.labels.अन्तः_2()) {
            StateConstLabel& existingLabel = labelIt->second;
            यदि (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + मानम्.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                प्रत्यागम असत्य;
            }
            यदि (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, मानम्, label);
            }
        } अन्यथा {
            StateConstLabel storedLabel = label;
            storedLabel.state_सूचकांक = stateSpace.अवस्थाः.आकार();
            stateSpace.अवस्थाः.emplace_पृष्ठभागः(label.text, मानम्);
            stateSpace.labels.emplace(मानम्, storedLabel);
        }
        प्रत्यागम सत्य;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    स्थिर तार्किक सङ्कलय_व्यञ्जकम्_गन्तव्यम्_State_Space(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_मानम् मानम्;
        ध्रुव Const_मानम्_स्थितिः स्थितिः = const_मानम्_स्थितिः(valuep, मानम्);
        यदि (स्थितिः != Const_मानम्_स्थितिः::OK) {
            यदि (स्थितिः == Const_मानम्_स्थितिः::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            प्रत्यागम असत्य;
        }
        AstConst* ध्रुव constp = VN_AS(valuep, Const);
        प्रत्यागम सङ्कलय_मानम्_गन्तव्यम्_State_Space(valuep, stateSpace, मानम्, stateLabelForConst(constp));
    }

    स्थिर तार्किक सङ्कलय_Optional_Target_व्यञ्जकम्_गन्तव्यम्_State_Space(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_मानम् मानम्;
        ध्रुव Const_मानम्_स्थितिः स्थितिः = const_मानम्_स्थितिः(valuep, मानम्);
        यदि (स्थितिः != Const_मानम्_स्थितिः::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            प्रत्यागम असत्य;
        }
        AstConst* ध्रुव constp = VN_AS(valuep, Const);
        प्रत्यागम सङ्कलय_मानम्_गन्तव्यम्_State_Space(valuep, stateSpace, मानम्, stateLabelForConst(constp));
    }

    स्थिर शून्य सङ्कलय_Reset_Targets_गन्तव्यम्_State_Space(ध्रुव std::सदिशम्<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        प्रति (ध्रुव FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.गन्तव्यम्_मानम्().ascii(), असत्य, 0};
            यदि (AstConst* ध्रुव constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                सङ्कलय_मानम्_गन्तव्यम्_State_Space(resetArc.nodep(), stateSpace, resetArc.गन्तव्यम्_मानम्(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    स्थिर तार्किक collectDeclaredStateSpace(Ast_ग्रन्थिः* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, तार्किक& needs_Source_मानानि) {
        AstVar* ध्रुव stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        यदि (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ध्रुव तार्किक forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        यदि (enump) {
            stateSpace.enumBacked = सत्य;
            प्रति (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ध्रुव AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
                ध्रुव Fsm_State_मानम् मानम्{constp->अङ्कः()};
                ध्रुव आकार_t state_सूचकांक = stateSpace.अवस्थाः.आकार();
                stateSpace.अवस्थाः.emplace_पृष्ठभागः(itemp->name(), मानम्);
                stateSpace.labels.emplace(मानम्,
                                          StateConstLabel{itemp->name(), असत्य, state_सूचकांक});
            }
            प्रत्यागम stateSpace.अवस्थाः.आकार() >= 2;
        }

        यदि (forced) {
            needs_Source_मानानि = सत्य;
            प्रत्यागम सत्य;
        }

        needs_Source_मानानि = सत्य;
        प्रत्यागम सत्य;
    }

    प्रतिरूपम् <प्रकारनाम T_ValuepVisitor>
    स्थिर तार्किक collect_State_Space_From_मानानि(Ast_ग्रन्थिः* warnNodep, AstVarScope* stateVscp,
                                            ध्रुव std::सदिशम्<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ध्रुव T_ValuepVisitor& दर्शय_Valueps) {
        तार्किक needs_Source_मानानि = असत्य;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        यदि (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_मानानि)) {
            प्रत्यागम असत्य;
        }
        यदि (!needs_Source_मानानि) प्रत्यागम सत्य;
        सङ्कलय_Reset_Targets_गन्तव्यम्_State_Space(resetArcs, stateSpace);
        यदि (!दर्शय_Valueps(
                [&](Ast_ग्रन्थिः_व्यञ्जकम्* valuep) { प्रत्यागम सङ्कलय_व्यञ्जकम्_गन्तव्यम्_State_Space(valuep, stateSpace); })) {
            प्रत्यागम असत्य;
        }
        प्रत्यागम stateSpace.अवस्थाः.आकार() >= 2;
    }

    स्थिर तार्किक collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* नियोजय_Vscp,
                                  ध्रुव std::सदिशम्<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        प्रत्यागम collect_State_Space_From_मानानि(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, नियोजय_Vscp, &stateSpace](ध्रुव स्वचालित& दर्शय_Valuep) {
                प्रति (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    यदि (!itemp->isDefault()) {
                        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                            यदि (!दर्शय_Valuep(condp)) प्रत्यागम असत्य;
                        }
                    }
                    यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assp
                        = प्रत्यक्षम्_State_नियोजय(itemp->stmtsp(), नियोजय_Vscp)) {
                        Fsm_State_मानम् then_मानम्;
                        Fsm_State_मानम् else_मानम्;
                        AstCond* ध्रुव condp = VN_CAST(assp->rhsp(), Cond);
                        यदि (condp
                            && प्रत्यक्षम्_State_Cond_Const_नियोजय(itemp->stmtsp(), नियोजय_Vscp, then_मानम्,
                                                          else_मानम्)) {
                            यदि (!दर्शय_Valuep(condp->thenp())) प्रत्यागम असत्य;
                            यदि (!दर्शय_Valuep(condp->elsep())) प्रत्यागम असत्य;
                        } अन्यथा यदि (!सङ्कलय_Optional_Target_व्यञ्जकम्_गन्तव्यम्_State_Space(assp->rhsp(), stateSpace)) {
                            प्रत्यागम असत्य;
                        }
                    }
                }
                प्रत्यागम सत्य;
            });
    }

    स्थिर तार्किक collectStateSpace(ध्रुव FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* नियोजय_Vscp,
                                  ध्रुव std::सदिशम्<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        प्रत्यागम collect_State_Space_From_मानानि(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, नियोजय_Vscp, &stateSpace](ध्रुव स्वचालित& दर्शय_Valuep) {
                प्रति (ध्रुव FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(दर्शय_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_ग्रन्थिः_नियोजय* ध्रुव assp = प्रत्यक्षम्_State_नियोजय(branch.stmtsp, नियोजय_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_मानम् then_मानम्;
                    Fsm_State_मानम् else_मानम्;
                    AstCond* ध्रुव condp = VN_CAST(assp->rhsp(), Cond);
                    यदि (condp) {
                        UASSERT_OBJ(प्रत्यक्षम्_State_Cond_Const_नियोजय(branch.stmtsp, नियोजय_Vscp,
                                                               then_मानम्, else_मानम्),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        यदि (!दर्शय_Valuep(condp->thenp())) प्रत्यागम असत्य;
                        यदि (!दर्शय_Valuep(condp->elsep())) प्रत्यागम असत्य;
                    } अन्यथा यदि (!सङ्कलय_Optional_Target_व्यञ्जकम्_गन्तव्यम्_State_Space(assp->rhsp(), stateSpace)) {
                        प्रत्यागम असत्य;
                    }
                }
                प्रत्यागम सत्य;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    स्थिर तार्किक emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 ध्रुव FsmStateSpace& stateSpace, तार्किक inclCond) {
        std::सदिशम्<std::pair<string, Fsm_State_मानम्>> froms;
        यदि (itemp->isDefault()) {
            यदि (!inclCond) प्रत्यागम असत्य;
            froms.emplace_पृष्ठभागः("default", Fsm_State_मानम्{});
        } अन्यथा {
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                Fsm_State_मानम् मानम्;
                यदि (const_मानम्_स्थितिः(condp, मानम्) != Const_मानम्_स्थितिः::OK) अनुवर्त;
                यदि (!validate_ज्ञातम्_State_मानम्(condp, stateSpace, मानम्, "source")) प्रत्यागम सत्य;
                froms.emplace_पृष्ठभागः(label_For_मानम्(stateSpace.labels, मानम्), मानम्);
            }
            यदि (froms.रिक्तानि()) प्रत्यागम असत्य;
        }

        यदि (Ast_ग्रन्थिः_नियोजय* ध्रुव assp = प्रत्यक्षम्_State_नियोजय(itemp->stmtsp(), stateVscp)) {
            Fsm_State_मानम् गन्तव्यम्_मानम्;
            ध्रुव Const_मानम्_स्थितिः स्थितिः = const_मानम्_स्थितिः(assp->rhsp(), गन्तव्यम्_मानम्);
            यदि (स्थितिः == Const_मानम्_स्थितिः::OK) {
                यदि (!validate_ज्ञातम्_State_मानम्(assp, stateSpace, गन्तव्यम्_मानम्, "target")) प्रत्यागम सत्य;
                प्रति (ध्रुव std::pair<string, Fsm_State_मानम्>& from : froms) {
                    graph.सङ्कलय_Arc(from.second, गन्तव्यम्_मानम्, असत्य, असत्य, itemp->isDefault(),
                                 assp->fileline());
                }
                प्रत्यागम सत्य;
            }
        }

        Fsm_State_मानम् then_मानम्;
        Fsm_State_मानम् else_मानम्;
        यदि (प्रत्यक्षम्_State_Cond_Const_नियोजय(itemp->stmtsp(), stateVscp, then_मानम्, else_मानम्)
            || if_State_Const_नियोजय(itemp->stmtsp(), stateVscp, then_मानम्, else_मानम्)) {
            यदि (!validate_ज्ञातम्_State_मानम्(itemp->stmtsp(), stateSpace, then_मानम्, "target"))
                प्रत्यागम सत्य;
            यदि (!validate_ज्ञातम्_State_मानम्(itemp->stmtsp(), stateSpace, else_मानम्, "target"))
                प्रत्यागम सत्य;
            प्रति (ध्रुव Fsm_State_मानम्& branch_मानम् : {then_मानम्, else_मानम्}) {
                प्रति (ध्रुव std::pair<string, Fsm_State_मानम्>& from : froms) {
                    graph.सङ्कलय_Arc(from.second, branch_मानम्, असत्य, सत्य, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            प्रत्यागम सत्य;
        }

        प्रत्यागम असत्य;
    }

    स्थिर शून्य emitStmtArcsFrom(FsmGraph& graph, Ast_ग्रन्थिः* stmtsp, AstVarScope* stateVscp,
                                 ध्रुव FsmStateSpace& stateSpace, Fsm_State_मानम् from_मानम्,
                                 तार्किक isDefault, तार्किक forceCond) {
        Ast_ग्रन्थिः_नियोजय* ध्रुव assp = प्रत्यक्षम्_State_नियोजय(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_मानम् गन्तव्यम्_मानम्;
        ध्रुव Const_मानम्_स्थितिः स्थितिः = const_मानम्_स्थितिः(assp->rhsp(), गन्तव्यम्_मानम्);
        यदि (स्थितिः == Const_मानम्_स्थितिः::OK) {
            यदि (!validate_ज्ञातम्_State_मानम्(assp, stateSpace, गन्तव्यम्_मानम्, "target")) प्रत्यागम;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.सङ्कलय_Arc(from_मानम्, गन्तव्यम्_मानम्, असत्य, forceCond, isDefault, assp->fileline());
            प्रत्यागम;
        }

        Fsm_State_मानम् then_मानम्;
        Fsm_State_मानम् else_मानम्;
        ध्रुव तार्किक cond_नियोजय
            = प्रत्यक्षम्_State_Cond_Const_नियोजय(stmtsp, stateVscp, then_मानम्, else_मानम्);
        UASSERT_OBJ(cond_नियोजय, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        यदि (!validate_ज्ञातम्_State_मानम्(stmtsp, stateSpace, then_मानम्, "target")) प्रत्यागम;
        यदि (!validate_ज्ञातम्_State_मानम्(stmtsp, stateSpace, else_मानम्, "target")) प्रत्यागम;
        प्रति (ध्रुव Fsm_State_मानम्& branch_मानम् : {then_मानम्, else_मानम्}) {
            graph.सङ्कलय_Arc(from_मानम्, branch_मानम्, असत्य, सत्य, isDefault, stmtsp->fileline());
        }
    }

    स्थिर शून्य emitIfChainArcs(FsmGraph& graph, ध्रुव FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ध्रुव FsmStateSpace& stateSpace) {
        प्रति (आकार_t i = 0; i < chain.branches.आकार(); ++i) {
            ध्रुव FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            यदि (!validate_ज्ञातम्_State_मानम्(branch.ifp, stateSpace, branch.from_मानम्, "source"))
                प्रत्यागम;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_मानम्, असत्य,
                             branch.has_शिखरम्_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    स्थिर शून्य सङ्कलय_Reset_Arcs(FsmGraph& graph, ध्रुव std::सदिशम्<FsmResetArcDesc>& resetArcs,
                             ध्रुव FsmStateSpace& stateSpace) {
        प्रति (ध्रुव FsmResetArcDesc& resetArc : resetArcs) {
            यदि (!validate_ज्ञातम्_State_मानम्(resetArc.nodep(), stateSpace, resetArc.गन्तव्यम्_मानम्(),
                                         "target"))
                अनुवर्त;
            graph.सङ्कलय_Arc(Fsm_State_मानम्{}, resetArc.गन्तव्यम्_मानम्(), सत्य, असत्य, असत्य,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    शून्य processCase(AstCase* casep, AstVarScope* नियोजय_Vscp, ध्रुव Fsm_पञ्जिकाकोशः_Candidate& reg) {
        UASSERT_OBJ(नियोजय_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ध्रुव stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        यदि (!collectStateSpace(casep, stateVscp, नियोजय_Vscp, reg.resetArcs(), stateSpace)) प्रत्यागम;
        DetectedFsm& प्रविष्टिः = m_state.fsmFor(stateVscp);
        यदि (!प्रविष्टिः.graphp) {
            प्रविष्टिः.graphp.reset(नूतन FsmGraph{});
            प्रविष्टिः.graphp->scopep(reg.scopep());
            प्रविष्टिः.graphp->stateAlwaysp(reg.alwaysp());
            प्रविष्टिः.graphp->stateVarName(stateVscp->prettyName());
            प्रविष्टिः.graphp->state_Var_आन्तरिकम्_Name(stateVscp->varp()->name());
            प्रविष्टिः.graphp->stateVarScopep(stateVscp);
            प्रविष्टिः.graphp->sampleVarScopep(reg.sampleVscp());
            प्रविष्टिः.graphp->senses() = reg.senses();
            प्रविष्टिः.graphp->resetCond() = reg.resetCond();
            प्रविष्टिः.graphp->hasResetCond(reg.hasResetCond());
            प्रविष्टिः.graphp->inclCond(reg.inclCond());
            प्रविष्टिः.graphp->fileline(casep->fileline());
            प्रति (ध्रुव std::pair<string, Fsm_State_मानम्>& state : stateSpace.अवस्थाः) {
                प्रविष्टिः.graphp->सङ्कलय_State_Vertex(state.first, state.second);
            }
            सङ्कलय_Reset_Arcs(*प्रविष्टिः.graphp, reg.resetArcs(), stateSpace);
        }
        प्रति (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*प्रविष्टिः.graphp, itemp, नियोजय_Vscp, stateSpace,
                             प्रविष्टिः.graphp->inclCond());
        }
    }

    शून्य processIfChain(ध्रुव FsmIfChainCandidate& chain, AstVarScope* नियोजय_Vscp,
                        ध्रुव Fsm_पञ्जिकाकोशः_Candidate& reg) {
        UASSERT_OBJ(नियोजय_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ध्रुव stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        यदि (!collectStateSpace(chain, stateVscp, नियोजय_Vscp, reg.resetArcs(), stateSpace)) प्रत्यागम;
        DetectedFsm& प्रविष्टिः = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!प्रविष्टिः.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        प्रविष्टिः.graphp.reset(नूतन FsmGraph{});
        प्रविष्टिः.graphp->scopep(reg.scopep());
        प्रविष्टिः.graphp->stateAlwaysp(reg.alwaysp());
        प्रविष्टिः.graphp->stateVarName(stateVscp->prettyName());
        प्रविष्टिः.graphp->state_Var_आन्तरिकम्_Name(stateVscp->varp()->name());
        प्रविष्टिः.graphp->stateVarScopep(stateVscp);
        प्रविष्टिः.graphp->sampleVarScopep(reg.sampleVscp());
        प्रविष्टिः.graphp->senses() = reg.senses();
        प्रविष्टिः.graphp->resetCond() = reg.resetCond();
        प्रविष्टिः.graphp->hasResetCond(reg.hasResetCond());
        प्रविष्टिः.graphp->inclCond(reg.inclCond());
        प्रविष्टिः.graphp->fileline(chain.ifp->fileline());
        प्रति (ध्रुव std::pair<string, Fsm_State_मानम्>& state : stateSpace.अवस्थाः) {
            प्रविष्टिः.graphp->सङ्कलय_State_Vertex(state.first, state.second);
        }
        सङ्कलय_Reset_Arcs(*प्रविष्टिः.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*प्रविष्टिः.graphp, chain, नियोजय_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    शून्य process_One_खण्डम्_Always(ध्रुव FsmComboAlways& one_खण्डम्) {
        ध्रुव पञ्जिकाकोशः_Always_Analyzer analyzer{one_खण्डम्.scopep()};
        AstAlways* ध्रुव alwaysp = one_खण्डम्.alwaysp();
        यदि (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) प्रत्यागम;
        ध्रुव std::सदिशम्<std::pair<AstCase*, Ast_ग्रन्थिः_व्यञ्जकम्*>> candidates
            = analyzer.one_खण्डम्_Candidates(alwaysp);

        FsmCaseCandidate firstCand;
        प्रति (ध्रुव std::pair<AstCase*, Ast_ग्रन्थिः_व्यञ्जकम्*>& cand : candidates) {
            AstVarRef* ध्रुव selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* ध्रुव vscp = selp ? selp->varScopep() : शून्यसूचकः;
            यदि (!vscp) अनुवर्त;
            यदि (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                Fsm_पञ्जिकाकोशः_Candidate reg;
                analyzer.build_One_खण्डम्_Candidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } अन्यथा यदि (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } अन्यथा {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        यदि (firstCand.stateVscp) प्रत्यागम;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ध्रुव std::सदिशम्<std::pair<AstIf*, Ast_ग्रन्थिः_व्यञ्जकम्*>> ifCandidates
            = analyzer.one_खण्डम्_If_Candidates(alwaysp);
        प्रति (ध्रुव std::pair<AstIf*, Ast_ग्रन्थिः_व्यञ्जकम्*>& cand : ifCandidates) {
            ध्रुव FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            यदि (!collectIfChain(cand.first, aliases, chain)) अनुवर्त;
            AstVarScope* ध्रुव vscp = chain.compareVscp;
            यदि (!if_Chain_Supported_Transition_ग्रन्थिः(chain, vscp)) अनुवर्त;
            यदि (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                Fsm_पञ्जिकाकोशः_Candidate reg;
                analyzer.build_One_खण्डम्_Candidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } अन्यथा यदि (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } अन्यथा {
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
        ध्रुव ComboAlwaysAnalyzer analyzer{m_पञ्जिकाकोशः_Candidates};
        Ast_ग्रन्थिः* ध्रुव stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ध्रुव casep = VN_CAST(nodep, Case);
            यदि (!casep) अनुवर्त;
            ध्रुव ComboAlwaysAnalyzer::Combo_मेलनम् मेलनम् = analyzer.मेलनम्_Case(stmtsp, casep);
            ध्रुव Fsm_पञ्जिकाकोशः_Candidate* ध्रुव matchedp = मेलनम्.matchedp;
            Ast_ग्रन्थिः* ध्रुव matchedWarnNodep = मेलनम्.warnNodep;
            यदि (!matchedp) अनुवर्त;
            यदि (!firstCand.stateVscp) {
                ध्रुव स्वचालित निवेशय_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                यदि (!निवेशय_Pair.second) {
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
                processCase(casep, matchedp->अग्र_Vscp(), *matchedp);
                अनुवर्त;
            }
            यदि (matchedp->stateVscp() != firstCand.stateVscp) {
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
        यदि (firstCand.stateVscp) प्रत्यागम;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        प्रति (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ध्रुव ifp = VN_CAST(nodep, If);
            यदि (!ifp) अनुवर्त;
            FsmIfChainCandidate chain;
            ध्रुव FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            यदि (!collectIfChain(ifp, aliases, chain)) अनुवर्त;
            ध्रुव ComboAlwaysAnalyzer::Combo_मेलनम् मेलनम् = analyzer.मेलनम्_If_Chain(stmtsp, chain);
            ध्रुव Fsm_पञ्जिकाकोशः_Candidate* ध्रुव matchedp = मेलनम्.matchedp;
            Ast_ग्रन्थिः* ध्रुव matchedWarnNodep = मेलनम्.warnNodep;
            यदि (!matchedp) अनुवर्त;
            यदि (!firstCand.stateVscp) {
                ध्रुव std::pair<std::unordered_map<ध्रुव AstVarScope*, FsmCaseCandidate>::iterator,
                                तार्किक>
                    निवेशय_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                यदि (!निवेशय_Pair.second) {
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
                processIfChain(chain, matchedp->अग्र_Vscp(), *matchedp);
                अनुवर्त;
            }
            यदि (matchedp->stateVscp() != firstCand.stateVscp) {
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
    शून्य दर्शय(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    शून्य दर्शय(AstAlways* nodep) override {
        यदि (nodep->keyword() == VAlwaysKwd::CONT_नियोजय) {
            iterateChildren(nodep);
            प्रत्यागम;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_खण्डम्_Alwayss.emplace_पृष्ठभागः(m_scopep, nodep);
        ध्रुव पञ्जिकाकोशः_Always_Analyzer analyzer{m_scopep};
        Fsm_पञ्जिकाकोशः_Candidate reg;
        यदि (analyzer.मेलनम्_पञ्जिकाकोशः_Candidate(nodep, reg)) {
            AstVarScope* ध्रुव stateVscp = reg.stateVscp();
            ध्रुव तार्किक प्राप्तम्
                = std::any_सम्बन्धि(m_पञ्जिकाकोशः_Candidates.cbegin(), m_पञ्जिकाकोशः_Candidates.cend(),
                              [stateVscp](ध्रुव Fsm_पञ्जिकाकोशः_Candidate& existing) {
                                  प्रत्यागम existing.stateVscp() == stateVscp;
                              });
            यदि (!प्राप्तम्) { m_पञ्जिकाकोशः_Candidates.emplace_पृष्ठभागः(reg); }
        }
        यदि (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_पृष्ठभागः(m_scopep, nodep);
        } अन्यथा यदि (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            यदि (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_पृष्ठभागः(m_scopep, nodep);
            } अन्यथा {
                m_nonComboAlwayss.emplace_पृष्ठभागः(m_scopep, nodep);
            }
        }
    }

    शून्य दर्शय(Ast_नियोजय_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_नियोजय(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_पत्तनम्_Alias(nodep);
        iterateChildren(nodep);
    }

    शून्य दर्शय(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        यदि (m_scopep && fsm_पञ्जिकाकोशः_Wrapper_Desc(nodep)) सङ्कलय_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        प्रति (ध्रुव std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_पञ्जिकाकोशः_Candidate reg;
            यदि (मेलनम्_Fsm_Wrapper_Cell(wrapperCell.first, wrapperCell.second, reg)) {
                m_पञ्जिकाकोशः_Candidates.emplace_पृष्ठभागः(reg);
            }
        }
        प्रति (ध्रुव FsmComboAlways& one_खण्डम् : m_one_खण्डम्_Alwayss) process_One_खण्डम्_Always(one_खण्डम्);
        प्रति (ध्रुव FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        प्रति (ध्रुव FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
वर्ग FsmCoverIncBuilder final {
    ध्रुव FsmGraph& m_graph;  // Fsm graph
    std::सदिशम्<ध्रुव FsmStateVertex*> m_अवस्थाः;  // States vertexes inside fsm graph
    std::unordered_समुच्चयः<uint64_t>
        m_undefined_अवस्थाः;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ध्रुव FsmVertex* m_defaultVertexp = शून्यसूचकः;  // Default vertex
    ध्रुव FsmVertex* m_resetVertexp = शून्यसूचकः;  // Reset vertex
    आकार_t m_धाराः_गणनम् = 0;  // Total count of edges
    AstVarScope* ध्रुव m_पूर्व_Vscp;  // Previous state of fsm
    File_पङ्क्तिः* ध्रुव m_flp;  // Fileline pointer to use across the builder
    ध्रुव std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_ग्रन्थिः_Stmt* m_coverIncp = शून्यसूचकः;  // Increments proper CoverDecl after each change
    Ast_ग्रन्थिः_Stmt* m_initialCoverIncp = शून्यसूचकः;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = शून्यसूचकः;  // List of CoverDecls
    तार्किक m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ध्रुव uint64_t मानम्) {
        V3_सङ्ख्या अङ्कः{static_cast<Ast_ग्रन्थिः*>(शून्यसूचकः), m_पूर्व_Vscp->विस्तारः()};
        अङ्कः.समुच्चयः_Quad(मानम्);
        अङ्कः.isSigned(m_पूर्व_Vscp->dtypep()->isSigned());
        प्रत्यागम नूतन AstConst{m_flp, अङ्कः};
    }

    std::string quad_गन्तव्यम्_Bit_Stirng_Label(ध्रुव uint64_t val) {
        पूर्णाङ्क विस्तारः = m_पूर्व_Vscp->विस्तारः();
        UASSERT(विस्तारः < 64, "We should not expand values that wide");
        std::string परिणाम = std::गन्तव्यम्_string(विस्तारः) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        यावत् (--विस्तारः >= 0) परिणाम.प्रक्षिप_पृष्ठभागः("01"[(val >> विस्तारः) & 1]);
        प्रत्यागम परिणाम;
    }

    शून्य init_Existing_अवस्थाः() {
        प्रति (ध्रुव V3GraphVertex& vtx : m_graph.vertices()) {
            ध्रुव FsmVertex* ध्रुव vertexp = vtx.as<FsmVertex>();
            m_धाराः_गणनम् += vertexp->अन्तर्_धाराः().आकार();
            यदि (ध्रुव FsmStateVertex* ध्रुव statep = vertexp->cast<FsmStateVertex>()) {
                m_अवस्थाः.प्रक्षिप_पृष्ठभागः(statep);
            } अन्यथा यदि (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } अन्यथा /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ध्रुव पूर्णाङ्क विस्तारः = m_graph.sampleVarScopep()->विस्तारः();
        m_isComplete = विस्तारः < 64 && m_अवस्थाः.आकार() == (static_cast<आकार_t>(1) << विस्तारः);
    }

    शून्य init_Undefined_अवस्थाः() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->विस्तारः() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ध्रुव uint64_t combinations_गणनम् = static_cast<आकार_t>(1)
                                           << m_graph.sampleVarScopep()->विस्तारः();
        // Rolling trick allows to cut peak memory usage
        तार्किक undefRolled = असत्य;
        स्वचालित rollUndefs = [एतत्, combinations_गणनम्] {
            प्रति (uint64_t i = 0; i < combinations_गणनम्; ++i) {
                ध्रुव स्वचालित निवेशय = m_undefined_अवस्थाः.निवेशय(i);
                यदि (!निवेशय.second) m_undefined_अवस्थाः.erase(निवेशय.first);
            }
        };
        प्रति (ध्रुव FsmStateVertex* ध्रुव statep : m_अवस्थाः) {
            ध्रुव V3_सङ्ख्या& अङ्कः = statep->मानम्().अङ्कः();
            ध्रुव uint64_t मानम् = अङ्कः.गन्तव्यम्_UQuad();
            यदि (!m_isComplete) {
                यदि (undefRolled) {
                    UASSERT_OBJ(m_undefined_अवस्थाः.erase(मानम्) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } अन्यथा {
                    UASSERT_OBJ(m_undefined_अवस्थाः.निवेशय(मानम्).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    यदि (m_undefined_अवस्थाः.आकार() * 2 >= combinations_गणनम्) {
                        undefRolled = सत्य;
                        rollUndefs();
                    }
                }
            }
        }
        यदि (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* buildResetCond() ध्रुव {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव refp
            = नूतन AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        प्रत्यागम m_graph.resetCond().सक्रिय_नीचः
                   ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstLogNot{m_flp, refp})
                   : refp;
    }

    आकार_t get_Arc_गणनम्_With_Expansion(ध्रुव VFsmExpandType expand) ध्रुव {
        आकार_t arc_गणनम्_With_Expansion = ~0ULL;
        विकल्प (expand) {
        पक्ष VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            भङ्ग;
        }
        पक्ष VFsmExpandType::AUTO: {
            arc_गणनम्_With_Expansion = m_धाराः_गणनम्;
            यदि (m_defaultVertexp) {
                UASSERT_OBJ(m_अवस्थाः.आकार() >= m_defaultVertexp->बहिर्गमनम्_धाराः().आकार(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_गणनम्_With_Expansion -= m_defaultVertexp->बहिर्गमनम्_धाराः().आकार();
                प्रति (ध्रुव FsmStateVertex* ध्रुव statep : m_अवस्थाः) {
                    arc_गणनम्_With_Expansion += static_cast<आकार_t>(statep->बहिर्गमनम्_धाराः().रिक्तानि())
                                             * m_defaultVertexp->बहिर्गमनम्_धाराः().आकार();
                }
            }
            यदि (m_resetVertexp) {
                UASSERT_OBJ(m_अवस्थाः.आकार() >= m_resetVertexp->बहिर्गमनम्_धाराः().आकार(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_गणनम्_With_Expansion += m_अवस्थाः.आकार() - m_resetVertexp->बहिर्गमनम्_धाराः().आकार();
            }
            भङ्ग;
        }
        पक्ष VFsmExpandType::AUTO_EXPAND: {
            arc_गणनम्_With_Expansion = m_अवस्थाः.आकार() * m_अवस्थाः.आकार() - m_अवस्थाः.आकार();
            भङ्ग;
        }
        पक्ष VFsmExpandType::पूर्णम्: {
            ध्रुव पूर्णाङ्क विस्तारः = m_graph.sampleVarScopep()->विस्तारः();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            यदि (विस्तारः < 32) {
                ध्रुव uint64_t combinations_गणनम् = static_cast<आकार_t>(1) << विस्तारः;
                arc_गणनम्_With_Expansion = combinations_गणनम् * combinations_गणनम् - combinations_गणनम्;
            }
            भङ्ग;
        }
        }
        प्रत्यागम arc_गणनम्_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ध्रुव std::string& label,
                                           ध्रुव std::string& fsm_चिह्नकम् = "") {
        AstCoverOtherDecl* ध्रुव declp
            = नूतन AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsm_चिह्नकम्};
        // We run after scoping, so this is declared for one instance only
        declp->प्रत्येकम्_Instance(सत्य);
        m_declsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_declsp, declp);
        प्रत्यागम declp;
    }

    Ast_ग्रन्थिः_Stmt* buildCoverIncStmtp(ध्रुव std::string& fromStateLabel,
                                    ध्रुव std::string& गन्तव्यम्_State_Label,
                                    ध्रुव std::string& आधारः_Label = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ध्रुव declp = नूतन AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_graph.stateVarName() + "::" + fromStateLabel + "->" + गन्तव्यम्_State_Label,
            "",
            0,
            m_graph.stateVarName(),
            fromStateLabel,
            गन्तव्यम्_State_Label,
            आधारः_Label};
        // We run after scoping, so this is declared for one instance only
        declp->प्रत्येकम्_Instance(सत्य);
        m_declsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(m_declsp, declp);
        प्रत्यागम नूतन AstCoverInc{m_flp, declp};
    }
    Ast_ग्रन्थिः_Stmt* buildExpandedTransitionHandler(ध्रुव std::string& गन्तव्यम्_State_Label,
                                                ध्रुव तार्किक considerUndefs,
                                                ध्रुव तार्किक expandOnlyWithTerminalVerticies,
                                                ध्रुव FsmStateVertex* ध्रुव गन्तव्यम्_Statep = शून्यसूचकः) {
        AstCase* ध्रुव arcCasep = नूतन AstCase{
            m_flp, VCaseType::CT_CASE, नूतन AstVarRef{m_flp, m_पूर्व_Vscp, VAccess::READ}, शून्यसूचकः};
        std::unordered_समुच्चयः<ध्रुव FsmStateVertex*> handledSoureces;
        यदि (गन्तव्यम्_Statep) {
            प्रति (ध्रुव V3GraphEdge& edge : गन्तव्यम्_Statep->अन्तर्_धाराः()) {
                यदि (ध्रुव FsmStateVertex* ध्रुव fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    यदि (!handledSoureces.emplace(fromp).second) अनुवर्त;
                    arcCasep->सङ्कलय_Itemsp(नूतन AstCaseItem{
                        m_flp, makeConst(fromp->मानम्().अङ्कः().गन्तव्यम्_UQuad()),
                        (fromp != गन्तव्यम्_Statep) ? buildCoverIncStmtp(fromp->label(), गन्तव्यम्_State_Label)
                                            : शून्यसूचकः});
                }
            }
        }
        प्रति (ध्रुव FsmStateVertex* ध्रुव fromp : m_अवस्थाः) {
            यदि (handledSoureces.find(fromp) != handledSoureces.अन्तः_2()) अनुवर्त;
            arcCasep->सङ्कलय_Itemsp(नूतन AstCaseItem{
                m_flp, makeConst(fromp->मानम्().अङ्कः().गन्तव्यम्_UQuad()),
                (fromp != गन्तव्यम्_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->बहिर्गमनम्_धाराः().रिक्तानि()))
                    ? buildCoverIncStmtp(fromp->label(), गन्तव्यम्_State_Label, "artificial")
                    : शून्यसूचकः});
        }
        यदि (!m_isComplete) {
            यदि (considerUndefs) {
                प्रति (ध्रुव uint64_t from : m_undefined_अवस्थाः) {
                    ध्रुव std::string& label = quad_गन्तव्यम्_Bit_Stirng_Label(from);
                    arcCasep->सङ्कलय_Itemsp(
                        नूतन AstCaseItem{m_flp, makeConst(from),
                                        (label != गन्तव्यम्_State_Label)
                                            ? buildCoverIncStmtp(label, गन्तव्यम्_State_Label, "artificial")
                                            : शून्यसूचकः});
                }
            } अन्यथा {
                arcCasep->सङ्कलय_Itemsp(नूतन AstCaseItem{m_flp, शून्यसूचकः, शून्यसूचकः});
            }
        }
        प्रत्यागम arcCasep;
    }
    Ast_ग्रन्थिः_Stmt* buildUnxpandedTransitionHandler(ध्रुव FsmStateVertex* ध्रुव गन्तव्यम्_Statep) {
        AstCase* ध्रुव arcCasep = नूतन AstCase{
            m_flp, VCaseType::CT_CASE, नूतन AstVarRef{m_flp, m_पूर्व_Vscp, VAccess::READ}, शून्यसूचकः};
        प्रति (ध्रुव V3GraphEdge& edge : गन्तव्यम्_Statep->अन्तर्_धाराः()) {
            ध्रुव FsmArcEdge* ध्रुव arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ध्रुव FsmStateVertex* ध्रुव fromp = arcp->fromp()->as<FsmStateVertex>();
            यदि (fromp != गन्तव्यम्_Statep) {
                arcCasep->सङ्कलय_Itemsp(
                    नूतन AstCaseItem{m_flp, makeConst(fromp->मानम्().अङ्कः().गन्तव्यम्_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), गन्तव्यम्_Statep->label())});
            }
        }
        arcCasep->सङ्कलय_Itemsp(नूतन AstCaseItem{m_flp, शून्यसूचकः, शून्यसूचकः});
        प्रत्यागम arcCasep;
    }

सार्वजनिक:
    FsmCoverIncBuilder(ध्रुव FsmGraph& graph, AstVarScope* ध्रुव पूर्व_Vscp)
        : m_graph{graph}
        , m_पूर्व_Vscp{पूर्व_Vscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        init_Existing_अवस्थाः();
        AstVarScope* ध्रुव sampleVscp = graph.sampleVarScopep();
        ध्रुव VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ध्रुव आकार_t arc_गणनम्_With_Expansion = get_Arc_गणनम्_With_Expansion(expand);
            यदि (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_गणनम्_With_Expansion == 0ULL
                            || arc_गणनम्_With_Expansion
                                   > static_cast<आकार_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_गणनम्_With_Expansion);
                प्रत्यागम;
            }
        }

        AstCase* ध्रुव arcCasep
            = नूतन AstCase{m_flp, VCaseType::CT_CASE,
                          नूतन AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, शून्यसूचकः};
        AstCase* ध्रुव initialStateCasep
            = नूतन AstCase{m_flp, VCaseType::CT_CASE,
                          नूतन AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, शून्यसूचकः};

        ध्रुव तार्किक expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_समुच्चयः<ध्रुव FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_समुच्चयः<ध्रुव FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        यदि (!expandAll) {
            यदि (m_defaultVertexp) {
                प्रति (ध्रुव V3GraphEdge& edge : m_defaultVertexp->बहिर्गमनम्_धाराः()) {
                    defaultTargets.emplace(edge.शिखरम्()->as<FsmStateVertex>());
                }
            }
            यदि (m_resetVertexp) {
                प्रति (ध्रुव V3GraphEdge& edge : m_resetVertexp->बहिर्गमनम्_धाराः()) {
                    resetTargets.emplace(edge.शिखरम्()->as<FsmStateVertex>());
                }
            }
        }
        ध्रुव तार्किक is_पूर्णम्_Expansion = expand == VFsmExpandType::पूर्णम्;
        यदि (is_पूर्णम्_Expansion) init_Undefined_अवस्थाः();
        प्रति (ध्रुव FsmStateVertex* ध्रुव statep : m_अवस्थाः) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ध्रुव declp = buildStateCoverDecl(statep->label());
            initialStateCasep->सङ्कलय_Itemsp(
                नूतन AstCaseItem{m_flp, makeConst(statep->मानम्().अङ्कः().गन्तव्यम्_UQuad()),
                                नूतन AstCoverInc{m_flp, declp, सत्य}});
            ध्रुव तार्किक expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.अन्तः_2());
            ध्रुव तार्किक defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.अन्तः_2());
            arcCasep->सङ्कलय_Itemsp(नूतन AstCaseItem{
                m_flp, makeConst(statep->मानम्().अङ्कः().गन्तव्यम्_UQuad()),
                Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः_Stmt>(
                    नूतन AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_पूर्णम्_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        यदि (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            यदि (is_पूर्णम्_Expansion) {
                प्रति (ध्रुव uint64_t मानम् : m_undefined_अवस्थाः) {
                    ध्रुव std::string& label = quad_गन्तव्यम्_Bit_Stirng_Label(मानम्);
                    AstCoverOtherDecl* ध्रुव declp = buildStateCoverDecl(label);
                    initialStateCasep->सङ्कलय_Itemsp(नूतन AstCaseItem{
                        m_flp, makeConst(मानम्), नूतन AstCoverInc{m_flp, declp, सत्य}});
                    arcCasep->सङ्कलय_Itemsp(
                        नूतन AstCaseItem{m_flp, makeConst(मानम्),
                                        Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः_Stmt>(
                                            नूतन AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, सत्य, असत्य))});
                }
            } अन्यथा {
                // No expansion
                arcCasep->सङ्कलय_Itemsp(नूतन AstCaseItem{m_flp, शून्यसूचकः, शून्यसूचकः});
                initialStateCasep->सङ्कलय_Itemsp(नूतन AstCaseItem{m_flp, शून्यसूचकः, शून्यसूचकः});
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
वर्ग Fsm_अधस्तनम्_Visitor final {
    // STATE - across all visitors
    ध्रुव FsmState& m_state;
    V3SharedTmps m_पूर्व_Tmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    स्थिर Ast_Sen_वृक्षः* build_Sen_वृक्षः(File_पङ्क्तिः* flp, ध्रुव std::सदिशम्<FsmSenDesc>& senses) {
        Ast_Sen_वृक्षः* ध्रुव sentreep = नूतन Ast_Sen_वृक्षः{flp, शून्यसूचकः};
        प्रति (ध्रुव FsmSenDesc& sense : senses) {
            AstSenItem* ध्रुव senItemp
                = नूतन AstSenItem{flp, VEdgeType{sense.edgeType},
                                 नूतन AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->सङ्कलय_Sensesp(senItemp);
        }
        प्रत्यागम sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    शून्य buildOne(ध्रुव FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().आकार() << पङ्क्त्यन्तः);
        AstAlways* ध्रुव alwaysp = graph.stateAlwaysp();
        AstScope* ध्रुव scopep = graph.scopep();
        AstVarScope* ध्रुव stateVscp = graph.stateVarScopep();
        AstVarScope* ध्रुव sampleVscp = graph.sampleVarScopep();
        File_पङ्क्तिः* ध्रुव flp = graph.fileline();
        Ast_ग्रन्थिः_Module* ध्रुव modp = scopep->modp();
        AstVarScope* ध्रुव पूर्व_Vscp
            = m_पूर्व_Tmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(सत्य);
        sampleVscp->optimizeLifePost(सत्य);
        पूर्व_Vscp->optimizeLifePost(सत्य);

        Ast_सक्रिय* ध्रुव initActivep
            = नूतन Ast_सक्रिय{flp, "fsm-coverage-init",
                            नूतन Ast_Sen_वृक्षः{flp, नूतन AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_वृक्षः_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->सङ्कलय_Stmtsp(नूतन AstInitialStatic{
            flp, नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, पूर्व_Vscp, VAccess::WRITE},
                               नूतन AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->सङ्कलय_Blocksp(initActivep);

        AstAlwaysPost* ध्रुव covPostp = नूतन AstAlwaysPost{flp};
        तार्किक update_पूर्व_After_Post = असत्य;
        यदि (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, पूर्व_Vscp, VAccess::WRITE},
                                                 नूतन AstVarRef{flp, sampleVscp, VAccess::READ}});
        } अन्यथा {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            update_पूर्व_After_Post = सत्य;
            पूर्व_Vscp->varp()->समुच्चयः_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ध्रुव FsmCoverIncBuilder coverIncBuilder{graph, पूर्व_Vscp};
            modp->सङ्कलय_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->सङ्कलय_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->सङ्कलय_Stmtsp(
                नूतन AstIf{flp,
                          नूतन AstNeq{flp, नूतन AstVarRef{flp, पूर्व_Vscp, VAccess::READ},
                                     नूतन AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        यदि (update_पूर्व_After_Post) {
            covPostp->सङ्कलय_Stmtsp(नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, पूर्व_Vscp, VAccess::WRITE},
                                              नूतन AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_वृक्षः* ध्रुव sentreep = build_Sen_वृक्षः(flp, graph.senses());
        Ast_सक्रिय* ध्रुव activep = नूतन Ast_सक्रिय{flp, "fsm-coverage", sentreep};
        activep->sen_वृक्षः_Storep(sentreep);
        scopep->सङ्कलय_Blocksp(activep);
        activep->सङ्कलय_Stmtsp(covPostp);
    }

सार्वजनिक:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    स्पष्ट Fsm_अधस्तनम्_Visitor(ध्रुव FsmState& state)
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
    यदि (dumpGraphLevel() >= 6) {
        आकार_t सूचकांक = 0;
        प्रति (ध्रुव DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_चिह्नकम्(सूचकांक++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_अधस्तनम्_Visitor अधस्तनम्{state}; }
    V3Global::dump_Check_Global_वृक्षः("fsm-detect", 0, dump_वृक्षः_Either_Level() >= 3);
}
