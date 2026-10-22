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

#అన్తర్భావ "V3PchAstNoMT.h"

#అన్తర్భావ "V3FsmDetect.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3SharedTmps.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cctype>
#అన్తర్భావ <map>
#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
తరగతి Fsm_State_విలువ final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_కీలకం;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_సంఖ్య m_అంకె;  // Original folded value, preserving width for lowered comparisons

    స్థిర string make_కీలకం(స్థిరరాశి V3_సంఖ్య& అంకె) {
        V3_సంఖ్య కీలకం_అంకె = అంకె;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        కీలకం_అంకె.isSigned(అబద్ధం);
        తిరిగి cvt_గమ్యం_Str(కీలకం_అంకె.వెడల్పు()) + ":" + కీలకం_అంకె.ascii(నిజం, నిజం);
    }

బహిరంగ:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_విలువ()
        : m_కీలకం{"1:1'h0"}
        , m_అంకె{static_cast<Ast_కణుపు*>(శూన్యసూచిక), 1, 0} {}
    స్పష్టమైన Fsm_State_విలువ(స్థిరరాశి V3_సంఖ్య& అంకె)
        : m_కీలకం{make_కీలకం(అంకె)}
        , m_అంకె{అంకె} {}

    స్థిరరాశి string& కీలకం() స్థిరరాశి { తిరిగి m_కీలకం; }
    స్థిరరాశి V3_సంఖ్య& అంకె() స్థిరరాశి { తిరిగి m_అంకె; }
    string ascii() స్థిరరాశి { తిరిగి m_అంకె.ascii(నిజం, నిజం); }
    string warn_పాఠ్యం() స్థిరరాశి {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        ఒకవేళ (m_అంకె.వెడల్పు() <= 32) తిరిగి cvt_గమ్యం_Str(m_అంకె.గమ్యం_UInt());
        తిరిగి ascii();
    }

    తార్కిక క్రియాకారకం==(స్థిరరాశి Fsm_State_విలువ& rhs) స్థిరరాశి { తిరిగి m_కీలకం == rhs.m_కీలకం; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
నిర్మాణం Fsm_State_విలువ_Hash final {
    కొలత_t క్రియాకారకం()(స్థిరరాశి Fsm_State_విలువ& విలువ) స్థిరరాశి {
        తిరిగి std::hash<string>{}(విలువ.కీలకం());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
నిర్మాణం FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en అంచు_Type = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = శూన్యసూచిక;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
నిర్మాణం FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = శూన్యసూచిక;
    తార్కిక చురుకైన_దిగువ = అబద్ధం;
};

తరగతి FsmResetArcDesc final {
    Fsm_State_విలువ m_గమ్యం_విలువ;  // Encoded reset target state.
    Ast_కణుపు* m_nodep = శూన్యసూచిక;  // Source node for warnings and emitted metadata.
    Ast_కణుపు_సమాసం* m_valuep = శూన్యసూచిక;  // Expression that provided the reset value.

బహిరంగ:
    FsmResetArcDesc() = అప్రమేయ;
    FsmResetArcDesc(Fsm_State_విలువ గమ్యం_విలువ, Ast_కణుపు_కేటాయించు* nodep)
        : m_గమ్యం_విలువ{గమ్యం_విలువ}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_విలువ గమ్యం_విలువ, Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* valuep)
        : m_గమ్యం_విలువ{గమ్యం_విలువ}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_విలువ గమ్యం_విలువ() స్థిరరాశి { తిరిగి m_గమ్యం_విలువ; }
    Ast_కణుపు* nodep() స్థిరరాశి { తిరిగి m_nodep; }
    Ast_కణుపు_సమాసం* valuep() స్థిరరాశి { తిరిగి m_valuep; }
};

నిర్మాణం FsmWrapperRoles final {
    string d_పోర్టు;
    string q_పోర్టు;
    string clk_పోర్టు;
    string rst_పోర్టు;
    string rstValParam;
    తార్కిక has_Rst_చురుకైన_దిగువ = అబద్ధం;
    తార్కిక rst_చురుకైన_దిగువ = అబద్ధం;
};

స్థిర తార్కిక fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, స్థిరరాశి string& పోర్టు_Name,
                                                  తార్కిక& చురుకైన_దిగువ) {
    తార్కిక matched = అబద్ధం;
    cellp->modp()->foreach([&](Ast_Sen_అంశం* itemp) {
        Ast_కణుపు_Var_Ref* స్థిరరాశి vrefp = itemp->varrefp();
        ఒకవేళ (!vrefp) తిరిగి;
        ఒకవేళ (vrefp->varp()->name() != పోర్టు_Name) తిరిగి;
        చురుకైన_దిగువ = itemp->అంచు_Type() == VEdgeType::ET_NEGEDGE;
        matched = నిజం;
    });
    తిరిగి matched;
}

స్థిర స్థిరరాశి V3Control::Fsm_రిజిస్టరు_Wrapper* fsm_రిజిస్టరు_Wrapper_Desc(AstCell* cellp) {
    Ast_కణుపు_Module* స్థిరరాశి modp = cellp->modp();
    స్థిరరాశి string origName = modp->origName();
    ఒకవేళ (స్థిరరాశి V3Control::Fsm_రిజిస్టరు_Wrapper* స్థిరరాశి descp
        = V3Control::get_Fsm_రిజిస్టరు_Wrapper(origName)) {
        తిరిగి descp;
    }
    తిరిగి V3Control::get_Fsm_రిజిస్టరు_Wrapper(modp->prettyDehashOrigOrName());
}

స్థిర FsmWrapperRoles rolesFromDesc(స్థిరరాశి V3Control::Fsm_రిజిస్టరు_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_పోర్టు = desc.d;
    roles.q_పోర్టు = desc.q;
    roles.clk_పోర్టు = desc.clock;
    roles.rst_పోర్టు = desc.reset;
    roles.rstValParam = desc.reset_విలువ;
    తిరిగి roles;
}

తరగతి Fsm_రిజిస్టరు_Candidate final {
    AstScope* m_scopep = శూన్యసూచిక;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = శూన్యసూచిక;  // Register process that commits the state.
    AstVarScope* m_stateVscp = శూన్యసూచిక;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = శూన్యసూచిక;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_తదుపరి_Vscp = శూన్యసూచిక;  // Next-state variable or same state var for 1-block FSMs.
    std::సదిశ<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::సదిశ<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    తార్కిక m_hasResetCond = అబద్ధం;  // Whether the FSM had a modeled reset predicate.
    తార్కిక m_inclCond = అబద్ధం;  // Whether conditional/default arcs are kept explicitly.
    File_పంక్తి* m_flp = శూన్యసూచిక;  // Representative source location.

బహిరంగ:
    AstScope* scopep() స్థిరరాశి { తిరిగి m_scopep; }
    శూన్య scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() స్థిరరాశి { తిరిగి m_alwaysp; }
    శూన్య alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() స్థిరరాశి { తిరిగి m_stateVscp; }
    శూన్య stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() స్థిరరాశి { తిరిగి m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    శూన్య sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* తదుపరి_Vscp() స్థిరరాశి { తిరిగి m_తదుపరి_Vscp; }
    శూన్య తదుపరి_Vscp(AstVarScope* vscp) { m_తదుపరి_Vscp = vscp; }
    స్థిరరాశి std::సదిశ<FsmSenDesc>& senses() స్థిరరాశి { తిరిగి m_senses; }
    std::సదిశ<FsmSenDesc>& senses() { తిరిగి m_senses; }
    స్థిరరాశి FsmResetCondDesc& resetCond() స్థిరరాశి { తిరిగి m_resetCond; }
    FsmResetCondDesc& resetCond() { తిరిగి m_resetCond; }
    స్థిరరాశి std::సదిశ<FsmResetArcDesc>& resetArcs() స్థిరరాశి { తిరిగి m_resetArcs; }
    std::సదిశ<FsmResetArcDesc>& resetArcs() { తిరిగి m_resetArcs; }
    తార్కిక hasResetCond() స్థిరరాశి { తిరిగి m_hasResetCond; }
    శూన్య hasResetCond(తార్కిక flag) { m_hasResetCond = flag; }
    తార్కిక inclCond() స్థిరరాశి { తిరిగి m_inclCond; }
    శూన్య inclCond(తార్కిక flag) { m_inclCond = flag; }
    File_పంక్తి* fileline() స్థిరరాశి { తిరిగి m_flp; }
    శూన్య fileline(File_పంక్తి* flp) { m_flp = flp; }
};

తరగతి FsmComboAlways final {
    AstScope* స్థిరరాశి m_scopep = శూన్యసూచిక;  // Owning scope for the combinational process.
    AstAlways* స్థిరరాశి m_alwaysp = శూన్యసూచిక;  // Candidate transition process.

బహిరంగ:
    FsmComboAlways() = అప్రమేయ;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() స్థిరరాశి { తిరిగి m_scopep; }
    AstAlways* alwaysp() స్థిరరాశి { తిరిగి m_alwaysp; }
};

తరగతి Fsm_లేఖాచిత్రం;

తరగతి Fsm_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Fsm_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)

బహిరంగ:
    గణన తరగతి రకం_2 : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

అంతరంగ:
    రకం_2 m_రకం;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_విలువ m_విలువ;  // Encoded state value for real state vertices.

రక్షిత:
    Fsm_శీర్షం(V3_లేఖాచిత్రం* graphp, రకం_2 రకం, string label, Fsm_State_విలువ విలువ) VL_MT_DISABLED
        : V3_లేఖాచిత్రం_శీర్షం{graphp},
          m_రకం{రకం},
          m_label{label},
          m_విలువ{విలువ} {}
    ~Fsm_శీర్షం() override = అప్రమేయ;

బహిరంగ:
    రకం_2 రకం() స్థిరరాశి { తిరిగి m_రకం; }
    తార్కిక isState() స్థిరరాశి { తిరిగి m_రకం == రకం_2::STATE; }
    తార్కిక isResetAny() స్థిరరాశి { తిరిగి m_రకం == రకం_2::RESET_ANY; }
    తార్కిక isDefaultAny() స్థిరరాశి { తిరిగి m_రకం == రకం_2::DEFAULT_ANY; }
    స్థిరరాశి string& label() స్థిరరాశి { తిరిగి m_label; }
    స్థిరరాశి Fsm_State_విలువ& విలువ() స్థిరరాశి { తిరిగి m_విలువ; }

    string name() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి m_label + "=" + m_విలువ.ascii(); }
};

తరగతి Fsm_State_శీర్షం final : బహిరంగ Fsm_శీర్షం {
    VL_RTTI_IMPL(Fsm_State_శీర్షం, Fsm_శీర్షం)

బహిరంగ:
    Fsm_State_శీర్షం(V3_లేఖాచిత్రం* graphp, string label, Fsm_State_విలువ విలువ) VL_MT_DISABLED
        : Fsm_శీర్షం{graphp, రకం_2::STATE, label, విలువ} {}
    ~Fsm_State_శీర్షం() override = అప్రమేయ;

    string dotColor() స్థిరరాశి override { తిరిగి "lightblue"; }
    string dotShape() స్థిరరాశి override { తిరిగి "ellipse"; }
};

తరగతి Fsm_Pseudo_శీర్షం final : బహిరంగ Fsm_శీర్షం {
    VL_RTTI_IMPL(Fsm_Pseudo_శీర్షం, Fsm_శీర్షం)

బహిరంగ:
    Fsm_Pseudo_శీర్షం(V3_లేఖాచిత్రం* graphp, రకం_2 రకం, string label) VL_MT_DISABLED
        : Fsm_శీర్షం{graphp, రకం, label, Fsm_State_విలువ{}} {}
    ~Fsm_Pseudo_శీర్షం() override = అప్రమేయ;

    string name() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి label(); }
    string dotColor() స్థిరరాశి override { తిరిగి isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() స్థిరరాశి override { తిరిగి "diamond"; }
};

తరగతి Fsm_Arc_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    VL_RTTI_IMPL(Fsm_Arc_అంచు, V3_లేఖాచిత్రం_అంచు)
    తార్కిక m_isReset = అబద్ధం;  // Arc originates from the synthetic reset source.
    తార్కిక m_isCond = అబద్ధం;  // Arc came from a conditional next-state split.
    తార్కిక m_isDefault = అబద్ధం;  // Arc represents a case default source.
    File_పంక్తి* m_flp = శూన్యసూచిక;  // Source location for emitted coverage metadata.

బహిరంగ:
    Fsm_Arc_అంచు(V3_లేఖాచిత్రం* graphp, Fsm_శీర్షం* fromp, Fsm_State_శీర్షం* శిఖరం, తార్కిక isReset, తార్కిక isCond,
               తార్కిక isDefault, File_పంక్తి* flp) VL_MT_DISABLED : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~Fsm_Arc_అంచు() override = అప్రమేయ;

    తార్కిక isReset() స్థిరరాశి { తిరిగి m_isReset; }
    తార్కిక isCond() స్థిరరాశి { తిరిగి m_isCond; }
    తార్కిక isDefault() స్థిరరాశి { తిరిగి m_isDefault; }
    File_పంక్తి* fileline() స్థిరరాశి { తిరిగి m_flp; }

    string dotLabel() స్థిరరాశి override {
        ఒకవేళ (m_isReset) తిరిగి "reset";
        ఒకవేళ (m_isDefault) తిరిగి "default";
        ఒకవేళ (m_isCond) తిరిగి "cond";
        తిరిగి "";
    }
    string dotColor() స్థిరరాశి override {
        ఒకవేళ (m_isReset) తిరిగి "darkgreen";
        ఒకవేళ (m_isDefault) తిరిగి "orange";
        ఒకవేళ (m_isCond) తిరిగి "blue";
        తిరిగి "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
తరగతి Fsm_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    AstScope* m_scopep = శూన్యసూచిక;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = శూన్యసూచిక;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_అంతర్గత_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = శూన్యసూచిక;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = శూన్యసూచిక;  // Scoped variable sampled by coverage logic.
    std::సదిశ<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    తార్కిక m_hasResetCond = అబద్ధం;  // Whether the detected FSM had a reset branch.
    తార్కిక m_inclCond = అబద్ధం;  // Whether conditional arcs should be kept explicitly.
    File_పంక్తి* m_flp = శూన్యసూచిక;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_విలువ, Fsm_State_శీర్షం*, Fsm_State_విలువ_Hash>
        m_stateVertices;  // Value to state map.
    Fsm_Pseudo_శీర్షం* m_resetVertexp = శూన్యసూచిక;  // Synthetic ANY source for reset arcs.
    Fsm_Pseudo_శీర్షం* m_defaultVertexp = శూన్యసూచిక;  // Synthetic default source for case defaults.

బహిరంగ:
    Fsm_లేఖాచిత్రం() VL_MT_DISABLED
        : m_resetVertexp{కొత్త Fsm_Pseudo_శీర్షం{ఇది, Fsm_శీర్షం::రకం_2::RESET_ANY, "ANY"}},
          m_defaultVertexp{కొత్త Fsm_Pseudo_శీర్షం{ఇది, Fsm_శీర్షం::రకం_2::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() స్థిరరాశి { తిరిగి m_scopep; }
    శూన్య scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() స్థిరరాశి { తిరిగి m_stateAlwaysp; }
    శూన్య stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    స్థిరరాశి string& stateVarName() స్థిరరాశి { తిరిగి m_stateVarName; }
    శూన్య stateVarName(స్థిరరాశి string& name) { m_stateVarName = name; }
    స్థిరరాశి string& state_Var_అంతర్గత_Name() స్థిరరాశి { తిరిగి m_state_Var_అంతర్గత_Name; }
    శూన్య state_Var_అంతర్గత_Name(స్థిరరాశి string& name) { m_state_Var_అంతర్గత_Name = name; }
    AstVarScope* stateVarScopep() స్థిరరాశి { తిరిగి m_stateVarScopep; }
    శూన్య stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() స్థిరరాశి {
        తిరిగి m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    శూన్య sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    స్థిరరాశి std::సదిశ<FsmSenDesc>& senses() స్థిరరాశి { తిరిగి m_senses; }
    std::సదిశ<FsmSenDesc>& senses() { తిరిగి m_senses; }
    స్థిరరాశి FsmResetCondDesc& resetCond() స్థిరరాశి { తిరిగి m_resetCond; }
    FsmResetCondDesc& resetCond() { తిరిగి m_resetCond; }
    తార్కిక hasResetCond() స్థిరరాశి { తిరిగి m_hasResetCond; }
    శూన్య hasResetCond(తార్కిక flag) { m_hasResetCond = flag; }
    తార్కిక inclCond() స్థిరరాశి { తిరిగి m_inclCond; }
    శూన్య inclCond(తార్కిక flag) { m_inclCond = flag; }
    File_పంక్తి* fileline() స్థిరరాశి { తిరిగి m_flp; }
    శూన్య fileline(File_పంక్తి* flp) { m_flp = flp; }

    Fsm_State_శీర్షం* కూడు_State_శీర్షం(string label, Fsm_State_విలువ విలువ) VL_MT_DISABLED {
        Fsm_State_శీర్షం* స్థిరరాశి vertexp = కొత్త Fsm_State_శీర్షం{ఇది, label, విలువ};
        m_stateVertices.emplace(విలువ, vertexp);
        తిరిగి vertexp;
    }
    Fsm_Pseudo_శీర్షం* reset_Any_శీర్షం() VL_MT_DISABLED { తిరిగి m_resetVertexp; }
    Fsm_Pseudo_శీర్షం* default_Any_శీర్షం() VL_MT_DISABLED { తిరిగి m_defaultVertexp; }
    Fsm_Arc_అంచు* కూడు_Arc(Fsm_State_విలువ from_విలువ, Fsm_State_విలువ గమ్యం_విలువ, తార్కిక isReset, తార్కిక isCond,
                       తార్కిక isDefault, File_పంక్తి* flp) VL_MT_DISABLED {
        Fsm_State_శీర్షం* స్థిరరాశి శిఖరం = m_stateVertices.at(గమ్యం_విలువ);
        Fsm_శీర్షం* fromp = శూన్యసూచిక;
        ఒకవేళ (isReset) {
            fromp = reset_Any_శీర్షం();
        } లేకపోతే ఒకవేళ (isDefault) {
            fromp = default_Any_శీర్షం();
        } లేకపోతే {
            fromp = m_stateVertices.at(from_విలువ);
        }
        తిరిగి కొత్త Fsm_Arc_అంచు{ఇది, fromp, శిఖరం, isReset, isCond, isDefault, flp};
    }

    string name() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి "FSM "
               + (m_stateVarName.ఖాళీలు() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_ట్యాగ్(కొలత_t సూచీ) స్థిరరాశి {
        string ట్యాగ్ = state_Var_అంతర్గత_Name();
        కొరకు (అక్షరం& ch : ట్యాగ్) {
            ఒకవేళ (!std::అక్షరమా_అంకెనా(static_cast<గుర్తులేని అక్షరం>(ch))) ch = '_';
        }
        తిరిగి "fsm_" + cvt_గమ్యం_Str(సూచీ) + "_" + ట్యాగ్;
    }
};

నిర్మాణం DetectedFsm final {
    std::unique_ptr<Fsm_లేఖాచిత్రం> graphp;  // Extracted graph for one detected FSM candidate.
};

నిర్మాణం FsmCaseCandidate final {
    Ast_కణుపు* warnNodep = శూన్యసూచిక;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = శూన్యసూచిక;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
నిర్మాణం FsmStateComparison final {
    AstVarScope* stateVscp = శూన్యసూచిక;  // Compared state variable
    Ast_కణుపు_సమాసం* valuep = శూన్యసూచిక;  // Compared constant value expression
    Fsm_State_విలువ విలువ;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
నిర్మాణం FsmIfBranch final {
    AstIf* ifp = శూన్యసూచిక;  // Source if/else-if node
    Ast_కణుపు* stmtsp = శూన్యసూచిక;  // Branch body
    Ast_కణుపు_సమాసం* valuep = శూన్యసూచిక;  // Source state value expression
    Fsm_State_విలువ from_విలువ;  // Encoded source state value
    తార్కిక has_శిఖరం_Guard = అబద్ధం;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
నిర్మాణం FsmIfChainCandidate final {
    AstIf* ifp = శూన్యసూచిక;  // Top-level if-chain node
    AstVarScope* compareVscp = శూన్యసూచిక;  // Variable used by every state comparison
    std::సదిశ<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_కణుపు* defaultStmtsp = శూన్యసూచిక;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
ఉపయోగించి FsmAliasMap = std::unordered_map<స్థిరరాశి AstVarScope*, FsmStateComparison>;
ఉపయోగించి Fsm_Cell_పోర్టు_Map = std::unordered_map<string, AstVarScope*>;
ఉపయోగించి Fsm_Cell_పోర్టు_Alias_Map = std::unordered_map<స్థిరరాశి AstCell*, Fsm_Cell_పోర్టు_Map>;

నిర్మాణం StateConstLabel final {
    string పాఠ్యం;
    తార్కిక fromParam = అబద్ధం;
    కొలత_t state_సూచీ = 0;
};

నిర్మాణం FsmStateSpace final {
    std::సదిశ<std::pair<string, Fsm_State_విలువ>> స్థితులు;  // User label and encoded value
    std::unordered_map<Fsm_State_విలువ, StateConstLabel, Fsm_State_విలువ_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = శూన్యసూచిక;  // Tracked FSM state variable
    తార్కిక enumBacked = అబద్ధం;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
తరగతి FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::సదిశ<DetectedFsm> m_fsms;
    std::map<స్థిరరాశి AstVarScope*, కొలత_t> m_fsm_సూచీ;

బహిరంగ:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        స్థిరరాశి std::map<స్థిరరాశి AstVarScope*, కొలత_t>::const_iterator it = m_fsm_సూచీ.కనుగొను(stateVscp);
        ఒకవేళ (it != m_fsm_సూచీ.చివర()) తిరిగి m_fsms.at(it->రెండవ);
        స్థిరరాశి కొలత_t సూచీ = m_fsms.కొలత();
        m_fsm_సూచీ.emplace(stateVscp, సూచీ);
        m_fsms.emplace_వెనుక();
        తిరిగి m_fsms.వెనుక();
    }
    స్థిరరాశి std::సదిశ<DetectedFsm>& fsms() స్థిరరాశి { తిరిగి m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
తరగతి FsmDetectVisitor final : బహిరంగ VNVisitor {
    // STATE - for current visit position (use VL_RESTORER)
    FsmState& m_state;
    AstScope* m_scopep = శూన్యసూచిక;
    std::సదిశ<Fsm_రిజిస్టరు_Candidate> m_రిజిస్టరు_Candidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::సదిశ<FsmComboAlways> m_one_ఖండం_Alwayss;
    std::సదిశ<FsmComboAlways> m_comboAlwayss;
    std::సదిశ<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::సదిశ<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<స్థిరరాశి AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_సమితి<స్థిరరాశి AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    Fsm_Cell_పోర్టు_Alias_Map m_cell_పోర్టు_Aliases;
    Fsm_Cell_పోర్టు_Alias_Map m_cell_పోర్టు_బిడ్డ_Aliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    స్థిర Ast_కణుపు_DType* unwrapEnumCandidate(Ast_కణుపు_DType* dtypep) {
        తిరిగి dtypep->skip_Ref_గమ్యం_Enump();
    }

    స్థిర string candidateConflictContext(Ast_కణుపు* laterNodep,
                                           స్థిరరాశి FsmCaseCandidate& మొదటి_Cand) {
        తిరిగి '\n' + laterNodep->warnContextPrimary() + మొదటి_Cand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + మొదటి_Cand.stateVscp->prettyNameQ() + '\n'
               + మొదటి_Cand.warnNodep->warnContextSecondary();
    }

    స్థిర తార్కిక rejectFsmWrapperCell(AstCell* cellp, స్థిరరాశి string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక simple_Param_State_విలువ(AstCell* cellp, స్థిరరాశి string& name, Fsm_State_విలువ& విలువ,
                                      Ast_కణుపు_సమాసం*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var);
            ఒకవేళ (!varp || !varp->isParam() || varp->name() != name) కొనసాగు;
            valuepr = VN_AS(varp->valuep(), కణుపు_సమాసం);
            తిరిగి const_విలువ_స్థితి(valuepr, విలువ) == Const_విలువ_స్థితి::OK;
        }
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక బిడ్డ_పోర్టు_లోపల_Scope(AstVarScope* vscp, AstScope* మాతృ_Scopep, AstCell*& cellpr) {
        ఒకవేళ (!vscp->varp()->isIO()) తిరిగి అబద్ధం;
        AstScope* స్థిరరాశి scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        ఒకవేళ (scopep->aboveScopep() != మాతృ_Scopep) తిరిగి అబద్ధం;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        తిరిగి నిజం;
    }

    స్థిర AstVarScope* simple_కేటాయించు_Var_Scope(Ast_కణుపు_సమాసం* exprp) {
        AstVarRef* స్థిరరాశి vrefp = VN_CAST(exprp, VarRef);
        తిరిగి vrefp ? vrefp->varScopep() : శూన్యసూచిక;
    }

    శూన్య కూడు_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_పోర్టు_Aliases.emplace(cellp, Fsm_Cell_పోర్టు_Map{});
        m_cell_పోర్టు_బిడ్డ_Aliases.emplace(cellp, Fsm_Cell_పోర్టు_Map{});
        స్థిరరాశి std::pair<AstScope*, AstCell*> అంశం_2{scopep, cellp};
        ఒకవేళ (std::కనుగొను(m_wrapperCells.cbegin(), m_wrapperCells.cend(), అంశం_2)
            != m_wrapperCells.cend()) {
            తిరిగి;
        }
        m_wrapperCells.emplace_వెనుక(అంశం_2);
    }

    శూన్య collect_Cell_పోర్టు_Alias(Ast_కేటాయించు_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* స్థిరరాశి lhsVscp = simple_కేటాయించు_Var_Scope(nodep->lhsp());
        AstVarScope* స్థిరరాశి rhsVscp = simple_కేటాయించు_Var_Scope(nodep->rhsp());
        ఒకవేళ (!lhsVscp || !rhsVscp) తిరిగి;
        AstCell* cellp = శూన్యసూచిక;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        ఒకవేళ (బిడ్డ_పోర్టు_లోపల_Scope(lhsVscp, m_scopep, cellp)) {
            ఒకవేళ (!fsm_రిజిస్టరు_Wrapper_Desc(cellp)) తిరిగి;
            UASSERT_OBJ(lhsVscp->varp()->is_ఆదానం(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_పోర్టు_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_పోర్టు_బిడ్డ_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            కూడు_Wrapper_Cell(m_scopep, cellp);
        } లేకపోతే ఒకవేళ (బిడ్డ_పోర్టు_లోపల_Scope(rhsVscp, m_scopep, cellp)) {
            ఒకవేళ (!fsm_రిజిస్టరు_Wrapper_Desc(cellp)) తిరిగి;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cell_పోర్టు_Aliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cell_పోర్టు_బిడ్డ_Aliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            కూడు_Wrapper_Cell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, స్థిరరాశి string& పోర్టు_Name) స్థిరరాశి {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        స్థిరరాశి Fsm_Cell_పోర్టు_Map& ports = m_cell_పోర్టు_Aliases.at(cellp);
        స్థిరరాశి Fsm_Cell_పోర్టు_Map::const_iterator పోర్టు_It = ports.కనుగొను(పోర్టు_Name);
        తిరిగి పోర్టు_It == ports.చివర() ? శూన్యసూచిక : పోర్టు_It->రెండవ;
    }

    AstVarScope* బిడ్డ_Role_Var_Scope(AstCell* cellp, స్థిరరాశి string& పోర్టు_Name) స్థిరరాశి {
        స్థిరరాశి Fsm_Cell_పోర్టు_Map& ports = m_cell_పోర్టు_బిడ్డ_Aliases.at(cellp);
        స్థిరరాశి Fsm_Cell_పోర్టు_Map::const_iterator పోర్టు_It = ports.కనుగొను(పోర్టు_Name);
        తిరిగి పోర్టు_It == ports.చివర() ? శూన్యసూచిక : పోర్టు_It->రెండవ;
    }

    తరగతి రిజిస్టరు_Always_Analyzer final {
        AstScope* స్థిరరాశి m_scopep;  // Scope containing the always block(s) being analyzed

    బహిరంగ:
        స్పష్టమైన రిజిస్టరు_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::సదిశ<std::pair<AstCase*, Ast_కణుపు_సమాసం*>>
        one_ఖండం_Candidates(AstAlways* alwaysp) స్థిరరాశి {
            std::సదిశ<std::pair<AstCase*, Ast_కణుపు_సమాసం*>> candidates;
            Ast_కణుపు* స్థిరరాశి stmtsp = alwaysp->stmtsp();
            ఒకవేళ (AstIf* స్థిరరాశి మొదటి_Ifp = VN_CAST(stmtsp, If)) {
                ఒకవేళ (AstCase* స్థిరరాశి casep = VN_CAST(మొదటి_Ifp->elsesp(), Case)) {
                    candidates.emplace_వెనుక(casep,
                                            FsmDetectVisitor::isSimpleResetCond(మొదటి_Ifp->condp())
                                                ? మొదటి_Ifp->condp()
                                                : శూన్యసూచిక);
                }
            }
            కొరకు (Ast_కణుపు* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ఒకవేళ (AstCase* స్థిరరాశి casep = VN_CAST(nodep, Case))
                    candidates.emplace_వెనుక(casep, శూన్యసూచిక);
            }
            తిరిగి candidates;
        }

        std::సదిశ<std::pair<AstIf*, Ast_కణుపు_సమాసం*>>
        one_ఖండం_If_Candidates(AstAlways* alwaysp) స్థిరరాశి {
            std::సదిశ<std::pair<AstIf*, Ast_కణుపు_సమాసం*>> candidates;
            Ast_కణుపు* స్థిరరాశి stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            ఒకవేళ (AstIf* స్థిరరాశి మొదటి_Ifp = VN_CAST(stmtsp, If)) {
                ఒకవేళ (AstIf* స్థిరరాశి chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(మొదటి_Ifp->elsesp()), If)) {
                    candidates.emplace_వెనుక(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(మొదటి_Ifp->condp())
                                                ? మొదటి_Ifp->condp()
                                                : శూన్యసూచిక);
                }
            }
            కొరకు (Ast_కణుపు* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                ఒకవేళ (AstIf* స్థిరరాశి ifp = VN_CAST(nodep, If)) candidates.emplace_వెనుక(ifp, శూన్యసూచిక);
            }
            తిరిగి candidates;
        }

        తార్కిక సరిపోలిక_రిజిస్టరు_Candidate(AstAlways* alwaysp, Fsm_రిజిస్టరు_Candidate& cand) స్థిరరాశి {
            తిరిగి FsmDetectVisitor::సరిపోలిక_రిజిస్టరు_Always(alwaysp, m_scopep, cand);
        }

        శూన్య build_One_ఖండం_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_కణుపు_సమాసం* resetCondp,
                                    Fsm_రిజిస్టరు_Candidate& reg) స్థిరరాశి {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.తదుపరి_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_వృక్షం(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != శూన్యసూచిక);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* స్థిరరాశి మొదటి_Ifp = VN_CAST(alwaysp->stmtsp(), If);
            ఒకవేళ (మొదటి_Ifp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = శూన్యసూచిక;
                స్థిరరాశి Reset_కేటాయించు_స్థితి reset_స్థితి = FsmDetectVisitor::collectConstStateAssigns(
                    మొదటి_Ifp->thensp(), resetStateVscp, reg.resetArcs());
                ఒకవేళ (reset_స్థితి == Reset_కేటాయించు_స్థితి::ఖాళీలు_3) {
                    reg.resetArcs().clear();
                } లేకపోతే ఒకవేళ (reset_స్థితి == Reset_కేటాయించు_స్థితి::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_విలువ reset_విలువ;
                    Ast_కణుపు* స్థిరరాశి thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(మొదటి_Ifp->thensp());
                    UASSERT_OBJ(thenNodep, మొదటి_Ifp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_కణుపు_కేటాయించు* స్థిరరాశి resetAssp = FsmDetectVisitor::ప్రత్యక్ష_Const_State_కేటాయించు_కణుపు(
                        thenNodep, resetStateVscp, reset_విలువ);
                    ఒకవేళ (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_వెనుక(reset_విలువ, resetAssp);
                    }
                } లేకపోతే ఒకవేళ (reset_స్థితి == Reset_కేటాయించు_స్థితి::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    తార్కిక సరిపోలిక_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_రిజిస్టరు_Candidate& cand) స్థిరరాశి {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_రిజిస్టరు_Wrapper_Desc(cellp));

        AstVarScope* స్థిరరాశి తదుపరి_Vscp = roleVarScope(cellp, roles.d_పోర్టు);
        AstVarScope* స్థిరరాశి stateVscp = roleVarScope(cellp, roles.q_పోర్టు);
        ఒకవేళ (!తదుపరి_Vscp || !stateVscp) {
            తిరిగి rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* స్థిరరాశి clkVscp = roleVarScope(cellp, roles.clk_పోర్టు);
        ఒకవేళ (!clkVscp) {
            తిరిగి rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.అంచు_Type = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().నెట్టు_వెనుక(clkSense);

        AstVarScope* resetVscp = శూన్యసూచిక;
        ఒకవేళ (!roles.rst_పోర్టు.ఖాళీలు()) resetVscp = roleVarScope(cellp, roles.rst_పోర్టు);
        ఒకవేళ (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            తార్కిక inferred_చురుకైన_దిగువ = అబద్ధం;
            ఒకవేళ (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_పోర్టు, inferred_చురుకైన_దిగువ)) {
                roles.has_Rst_చురుకైన_దిగువ = నిజం;
                roles.rst_చురుకైన_దిగువ = inferred_చురుకైన_దిగువ;
            }
        }

        Ast_కణుపు_సమాసం* resetValuep = శూన్యసూచిక;
        Fsm_State_విలువ reset_విలువ;
        స్థిరరాశి తార్కిక has_Reset_విలువ
            = !roles.rstValParam.ఖాళీలు()
              && simple_Param_State_విలువ(cellp, roles.rstValParam, reset_విలువ, resetValuep);
        ఒకవేళ (resetVscp && roles.has_Rst_చురుకైన_దిగువ && has_Reset_విలువ) {
            FsmSenDesc rstSense;
            rstSense.అంచు_Type = roles.rst_చురుకైన_దిగువ ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().నెట్టు_వెనుక(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().చురుకైన_దిగువ = roles.rst_చురుకైన_దిగువ;
            cand.hasResetCond(నిజం);
            cand.resetArcs().emplace_వెనుక(reset_విలువ, cellp, resetValuep);
        } లేకపోతే ఒకవేళ (!roles.rst_పోర్టు.ఖాళీలు() || !roles.rstValParam.ఖాళీలు()) {
            string reason;
            ఒకవేళ (roles.rst_పోర్టు.ఖాళీలు()) {
                reason = "reset port is not configured";
            } లేకపోతే ఒకవేళ (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } లేకపోతే ఒకవేళ (!roles.has_Rst_చురుకైన_దిగువ) {
                reason = "reset polarity could not be inferred from the wrapper";
            } లేకపోతే ఒకవేళ (roles.rstValParam.ఖాళీలు()) {
                reason = "reset_value parameter is not configured";
            } లేకపోతే {
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
        cand.alwaysp(శూన్యసూచిక);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(బిడ్డ_Role_Var_Scope(cellp, roles.q_పోర్టు));
        cand.తదుపరి_Vscp(తదుపరి_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        తిరిగి నిజం;
    }

    తరగతి ComboAlwaysAnalyzer final {
    బహిరంగ:
        నిర్మాణం Combo_సరిపోలిక final {
            స్థిరరాశి Fsm_రిజిస్టరు_Candidate* matchedp = శూన్యసూచిక;
            Ast_కణుపు* warnNodep = శూన్యసూచిక;
        };

    అంతరంగ:
        స్థిరరాశి std::సదిశ<Fsm_రిజిస్టరు_Candidate>& m_రిజిస్టరు_Candidates;

    బహిరంగ:
        స్పష్టమైన ComboAlwaysAnalyzer(స్థిరరాశి std::సదిశ<Fsm_రిజిస్టరు_Candidate>& రిజిస్టరు_Candidates)
            : m_రిజిస్టరు_Candidates{రిజిస్టరు_Candidates} {}

        Combo_సరిపోలిక సరిపోలిక_Case(Ast_కణుపు* stmtsp, AstCase* casep) స్థిరరాశి {
            Combo_సరిపోలిక సరిపోలిక;
            AstVarRef* స్థిరరాశి selp = VN_CAST(casep->exprp(), VarRef);
            ఒకవేళ (!selp) తిరిగి సరిపోలిక;
            కొరకు (స్థిరరాశి Fsm_రిజిస్టరు_Candidate& reg : m_రిజిస్టరు_Candidates) {
                ఒకవేళ (selp->varScopep() == reg.తదుపరి_Vscp()) {
                    ఒకవేళ (!FsmDetectVisitor::has_Canonical_తదుపరి_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.తదుపరి_Vscp())) {
                        కొనసాగు;
                    }
                } లేకపోతే ఒకవేళ (selp->varScopep() != reg.stateVscp()) {
                    కొనసాగు;
                }
                Ast_కణుపు* స్థిరరాశి warnNodep = FsmDetectVisitor::case_Supported_Transition_కణుపు(
                    casep, reg.తదుపరి_Vscp(), reg.inclCond());
                ఒకవేళ (!warnNodep) కొనసాగు;
                సరిపోలిక.matchedp = &reg;
                సరిపోలిక.warnNodep = warnNodep;
            }
            తిరిగి సరిపోలిక;
        }

        Combo_సరిపోలిక సరిపోలిక_If_Chain(Ast_కణుపు* stmtsp, స్థిరరాశి FsmIfChainCandidate& chain) స్థిరరాశి {
            Combo_సరిపోలిక సరిపోలిక;
            కొరకు (స్థిరరాశి Fsm_రిజిస్టరు_Candidate& reg : m_రిజిస్టరు_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                ఒకవేళ (chain.compareVscp == reg.తదుపరి_Vscp()) {
                    ఒకవేళ (!FsmDetectVisitor::has_Canonical_తదుపరి_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.తదుపరి_Vscp())) {
                        కొనసాగు;
                    }
                } లేకపోతే ఒకవేళ (chain.compareVscp != reg.stateVscp()) {
                    కొనసాగు;
                }
                Ast_కణుపు* స్థిరరాశి warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_కణుపు(chain, reg.తదుపరి_Vscp());
                ఒకవేళ (!warnNodep) కొనసాగు;
                సరిపోలిక.matchedp = &reg;
                సరిపోలిక.warnNodep = warnNodep;
            }
            తిరిగి సరిపోలిక;
        }

        తార్కిక shouldWarnUnsupported(Ast_కణుపు* stmtsp, AstCase* casep) స్థిరరాశి {
            స్థిరరాశి AstVarRef* స్థిరరాశి selp = VN_CAST(casep->exprp(), VarRef);
            ఒకవేళ (!selp) తిరిగి అబద్ధం;

            తిరిగి std::any_యొక్క(
                m_రిజిస్టరు_Candidates.cbegin(), m_రిజిస్టరు_Candidates.cend(),
                [&](స్థిరరాశి Fsm_రిజిస్టరు_Candidate& reg) -> తార్కిక {
                    స్థిరరాశి తార్కిక matches_తదుపరి = selp->varScopep() == reg.తదుపరి_Vscp();
                    స్థిరరాశి తార్కిక matchesState = selp->varScopep() == reg.stateVscp();

                    ఒకవేళ (!matches_తదుపరి && !matchesState) తిరిగి అబద్ధం;
                    ఒకవేళ (matches_తదుపరి
                        && !FsmDetectVisitor::has_Canonical_తదుపరి_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.తదుపరి_Vscp())) {
                        తిరిగి అబద్ధం;
                    }
                    తిరిగి FsmDetectVisitor::case_Supported_Transition_కణుపు(casep, reg.తదుపరి_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    స్థిర తార్కిక isSimpleResetCond(Ast_కణుపు_సమాసం* condp) { తిరిగి VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    స్థిర FsmResetCondDesc describeResetCond(Ast_కణుపు_సమాసం* condp) {
        FsmResetCondDesc desc;
        ఒకవేళ (AstVarRef* స్థిరరాశి vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        తిరిగి desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    స్థిర std::సదిశ<FsmSenDesc> describe_Sen_వృక్షం(Ast_Sen_వృక్షం* sentreep) {
        std::సదిశ<FsmSenDesc> senses;
        కొరకు (Ast_Sen_అంశం* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), Sen_అంశం)) {
            Ast_కణుపు_Var_Ref* స్థిరరాశి vrefp = itemp->varrefp();
            ఒకవేళ (!vrefp) కొనసాగు;
            FsmSenDesc desc;
            desc.అంచు_Type = itemp->అంచు_Type().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.నెట్టు_వెనుక(desc);
        }
        తిరిగి senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    స్థిర తార్కిక isIgnorableStmt(Ast_కణుపు* nodep) { తిరిగి VN_IS(nodep, CoverInc); }

    స్థిర Ast_కణుపు* skipLeadingIgnorableStmt(Ast_కణుపు* nodep) {
        అయితే (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        తిరిగి nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    స్థిర Ast_కణుపు* singleMeaningfulStmt(Ast_కణుపు* stmtp) {
        Ast_కణుపు* resultp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            ఒకవేళ (isIgnorableStmt(nodep)) కొనసాగు;
            ఒకవేళ (resultp) తిరిగి శూన్యసూచిక;
            resultp = nodep;
        }
        తిరిగి resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    స్థిర Ast_కణుపు* singleMeaningfulBranch(Ast_కణుపు* branchp) {
        ఒకవేళ (!branchp) తిరిగి శూన్యసూచిక;
        తిరిగి branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    స్థిర తార్కిక isPlainComboSentree(స్థిరరాశి Ast_Sen_వృక్షం* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        కొరకు (స్థిరరాశి Ast_Sen_అంశం* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), Sen_అంశం)) {
            ఒకవేళ (senp->అంచు_Type() == VEdgeType::ET_మారింది) కొనసాగు;
            తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }

    శూన్య warnUnsupportedComboAlways(స్థిరరాశి FsmComboAlways& combo) {
        స్థిరరాశి ComboAlwaysAnalyzer analyzer{m_రిజిస్టరు_Candidates};
        Ast_కణుపు* స్థిరరాశి stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        తార్కిక warned = అబద్ధం;
        కొరకు (Ast_కణుపు* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* స్థిరరాశి casep = VN_CAST(nodep, Case);
            ఒకవేళ (!casep) కొనసాగు;
            ఒకవేళ (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = నిజం;
            }
            ఒకవేళ (warned) ఆపు;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    స్థిర Ast_కణుపు_కేటాయించు* ప్రత్యక్ష_State_కేటాయించు(Ast_కణుపు* stmtp, AstVarScope* stateVscp) {
        Ast_కణుపు* స్థిరరాశి nodep = singleMeaningfulBranch(stmtp);
        ఒకవేళ (!nodep) తిరిగి శూన్యసూచిక;
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = VN_CAST(nodep, కణుపు_కేటాయించు);
        ఒకవేళ (!assp) తిరిగి శూన్యసూచిక;
        AstVarRef* స్థిరరాశి vrefp = VN_CAST(assp->lhsp(), VarRef);
        ఒకవేళ (!vrefp || vrefp->varScopep() != stateVscp) తిరిగి శూన్యసూచిక;
        తిరిగి assp;
    }

    స్థిర Ast_కణుపు_కేటాయించు* కణుపు_State_Var_కేటాయించు(Ast_కణుపు* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = VN_CAST(nodep, కణుపు_కేటాయించు);
        ఒకవేళ (!assp) తిరిగి శూన్యసూచిక;
        AstVarRef* స్థిరరాశి lhsp = VN_CAST(assp->lhsp()->పునాది_Fromp(నిజం), VarRef);
        AstVarRef* స్థిరరాశి rhsp = VN_CAST(assp->rhsp(), VarRef);
        ఒకవేళ (!rhsp || !lhsp) తిరిగి శూన్యసూచిక;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        తిరిగి assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    స్థిర Ast_కణుపు_కేటాయించు* ప్రత్యక్ష_Cond_State_Var_కేటాయించు(Ast_కణుపు* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_కణుపు_సమాసం*& condp,
                                                   Fsm_State_విలువ& reset_విలువ) {
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = VN_CAST(nodep, కణుపు_కేటాయించు);
        ఒకవేళ (!assp) తిరిగి శూన్యసూచిక;
        AstVarRef* స్థిరరాశి lhsp = VN_CAST(assp->lhsp()->పునాది_Fromp(నిజం), VarRef);
        AstCond* స్థిరరాశి rhsp = VN_CAST(assp->rhsp(), Cond);
        ఒకవేళ (!rhsp || !lhsp) తిరిగి శూన్యసూచిక;
        AstVarRef* స్థిరరాశి elsep = VN_CAST(rhsp->elsep(), VarRef);
        ఒకవేళ (!elsep) తిరిగి శూన్యసూచిక;
        ఒకవేళ (const_విలువ_స్థితి(rhsp->thenp(), reset_విలువ) != Const_విలువ_స్థితి::OK) తిరిగి శూన్యసూచిక;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        తిరిగి assp;
    }

    స్థిర Ast_కణుపు_కేటాయించు* ప్రత్యక్ష_Const_State_కేటాయించు_కణుపు(Ast_కణుపు* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_విలువ& విలువ) {
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = VN_CAST(nodep, కణుపు_కేటాయించు);
        ఒకవేళ (!assp) తిరిగి శూన్యసూచిక;
        AstVarRef* స్థిరరాశి lhsp = VN_CAST(assp->lhsp()->పునాది_Fromp(నిజం), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        ఒకవేళ (const_విలువ_స్థితి(assp->rhsp(), విలువ) != Const_విలువ_స్థితి::OK) తిరిగి శూన్యసూచిక;
        stateVscp = lhsp->varScopep();
        తిరిగి assp;
    }

    గణన తరగతి Reset_కేటాయించు_స్థితి : uint8_t {
        ఖాళీలు_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    స్థిర Reset_కేటాయించు_స్థితి collectConstStateAssigns(Ast_కణుపు* stmtp, AstVarScope*& stateVscp,
                                                      std::సదిశ<FsmResetArcDesc>& resetArcs) {
        Ast_కణుపు* nodep = skipLeadingIgnorableStmt(stmtp);
        ఒకవేళ (!nodep) తిరిగి Reset_కేటాయించు_స్థితి::ఖాళీలు_3;
        కొరకు (;; nodep = nodep->nextp()) {
            AstVarScope* కేటాయించు_State_Vscp = శూన్యసూచిక;
            Fsm_State_విలువ విలువ;
            Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = ప్రత్యక్ష_Const_State_కేటాయించు_కణుపు(nodep, కేటాయించు_State_Vscp, విలువ);
            ఒకవేళ (!assp) తిరిగి Reset_కేటాయించు_స్థితి::NONE;
            ఒకవేళ (!stateVscp) stateVscp = కేటాయించు_State_Vscp;
            ఒకవేళ (కేటాయించు_State_Vscp != stateVscp) తిరిగి Reset_కేటాయించు_స్థితి::NONE;
            ఒకవేళ (!resetArcs.ఖాళీలు()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                తిరిగి Reset_కేటాయించు_స్థితి::MULTI_SAME_STATE;
            }
            resetArcs.emplace_వెనుక(విలువ, assp);
            ఒకవేళ (!nodep->nextp()) తిరిగి Reset_కేటాయించు_స్థితి::SINGLE;
        }
    }

    స్థిర తార్కిక has_Canonical_తదుపరి_State_Default_Before_Case(Ast_కణుపు* stmtsp, Ast_కణుపు* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* తదుపరి_Vscp) {
        Ast_కణుపు* స్థిరరాశి bodyp = skipLeadingIgnorableStmt(stmtsp);
        తార్కిక sawCanonicalDefault = అబద్ధం;
        కొరకు (Ast_కణుపు* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            ఒకవేళ (nodep == targetp) తిరిగి sawCanonicalDefault;
            ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = VN_CAST(nodep, కణుపు_కేటాయించు)) {
                AstVarRef* స్థిరరాశి lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* స్థిరరాశి rhsp = VN_CAST(assp->rhsp(), VarRef);
                ఒకవేళ (!lhsp || lhsp->varScopep() != తదుపరి_Vscp) కొనసాగు;
                ఒకవేళ (sawCanonicalDefault) {
                    స్థిరరాశి string తదుపరి_Name = తదుపరి_Vscp->varp()->prettyNameQ();
                    స్థిరరాశి string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + తదుపరి_Name
                                     + ") when the canonical " + తదుపరి_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    తిరిగి అబద్ధం;
                }
                ఒకవేళ (!rhsp || rhsp->varScopep() != stateVscp) తిరిగి అబద్ధం;
                sawCanonicalDefault = నిజం;
            }
        }
    }

    స్థిర తార్కిక if_State_Const_కేటాయించు(Ast_కణుపు* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_విలువ& then_విలువ, Fsm_State_విలువ& else_విలువ) {
        AstIf* స్థిరరాశి ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        ఒకవేళ (!ifp || !ifp->elsesp()) తిరిగి అబద్ధం;
        AstVarScope* thenVscp = శూన్యసూచిక;
        AstVarScope* elseVscp = శూన్యసూచిక;
        Ast_కణుపు* స్థిరరాశి thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_కణుపు* స్థిరరాశి elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        ఒకవేళ (!thenNodep || !elseNodep) తిరిగి అబద్ధం;
        ఒకవేళ (!ప్రత్యక్ష_Const_State_కేటాయించు_కణుపు(thenNodep, thenVscp, then_విలువ)) తిరిగి అబద్ధం;
        ఒకవేళ (!ప్రత్యక్ష_Const_State_కేటాయించు_కణుపు(elseNodep, elseVscp, else_విలువ)) తిరిగి అబద్ధం;
        ఒకవేళ (thenVscp == stateVscp && elseVscp == stateVscp) తిరిగి నిజం;
        ఒకవేళ (thenVscp != elseVscp) తిరిగి అబద్ధం;
        Ast_కణుపు* స్థిరరాశి followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = శూన్యసూచిక;
        AstVarScope* finalFromVscp = శూన్యసూచిక;
        Ast_కణుపు* స్థిరరాశి finalNodep = singleMeaningfulBranch(followp);
        ఒకవేళ (!finalNodep) తిరిగి అబద్ధం;
        ఒకవేళ (!కణుపు_State_Var_కేటాయించు(finalNodep, finalStateVscp, finalFromVscp)) తిరిగి అబద్ధం;
        ఒకవేళ (finalStateVscp != stateVscp) తిరిగి అబద్ధం;
        ఒకవేళ (finalFromVscp != thenVscp) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }

    స్థిర తార్కిక ప్రత్యక్ష_State_Cond_Const_కేటాయించు(Ast_కణుపు* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_విలువ& then_విలువ, Fsm_State_విలువ& else_విలువ) {
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = ప్రత్యక్ష_State_కేటాయించు(stmtp, stateVscp);
        ఒకవేళ (!assp) తిరిగి అబద్ధం;
        AstCond* స్థిరరాశి condp = VN_CAST(assp->rhsp(), Cond);
        ఒకవేళ (!condp) తిరిగి అబద్ధం;
        తిరిగి const_విలువ_స్థితి(condp->thenp(), then_విలువ) == Const_విలువ_స్థితి::OK
               && const_విలువ_స్థితి(condp->elsep(), else_విలువ) == Const_విలువ_స్థితి::OK;
    }

    స్థిర Ast_కణుపు* case_అంశం_Supported_Arc_కణుపు(Ast_Case_అంశం* itemp, AstVarScope* stateVscp,
                                             తార్కిక inclCond) {
        ఒకవేళ (itemp->isDefault()) {
            ఒకవేళ (!inclCond) తిరిగి శూన్యసూచిక;
        }
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = ప్రత్యక్ష_State_కేటాయించు(itemp->stmtsp(), stateVscp);
        ఒకవేళ (assp) {
            Fsm_State_విలువ గమ్యం_విలువ;
            ఒకవేళ (const_విలువ_స్థితి(assp->rhsp(), గమ్యం_విలువ) == Const_విలువ_స్థితి::OK) తిరిగి assp;
        }
        Fsm_State_విలువ then_విలువ;
        Fsm_State_విలువ else_విలువ;
        ఒకవేళ (ప్రత్యక్ష_State_Cond_Const_కేటాయించు(itemp->stmtsp(), stateVscp, then_విలువ, else_విలువ)) {
            తిరిగి assp;
        }
        ఒకవేళ (if_State_Const_కేటాయించు(itemp->stmtsp(), stateVscp, then_విలువ, else_విలువ)) {
            తిరిగి singleMeaningfulBranch(itemp->stmtsp());
        }
        తిరిగి శూన్యసూచిక;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    స్థిర Ast_కణుపు* case_Supported_Transition_కణుపు(AstCase* casep, AstVarScope* stateVscp,
                                                తార్కిక inclCond) {
        కొరకు (Ast_Case_అంశం* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_అంశం)) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి nodep = case_అంశం_Supported_Arc_కణుపు(itemp, stateVscp, inclCond))
                తిరిగి nodep;
        }
        తిరిగి శూన్యసూచిక;
    }

    స్థిర Ast_కణుపు* case_అంశం_Supported_Arc_కణుపు_Like(Ast_కణుపు* stmtsp, AstVarScope* stateVscp) {
        ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = ప్రత్యక్ష_State_కేటాయించు(stmtsp, stateVscp)) {
            Fsm_State_విలువ గమ్యం_విలువ;
            ఒకవేళ (const_విలువ_స్థితి(assp->rhsp(), గమ్యం_విలువ) == Const_విలువ_స్థితి::OK) తిరిగి assp;
            Fsm_State_విలువ then_విలువ;
            Fsm_State_విలువ else_విలువ;
            ఒకవేళ (ప్రత్యక్ష_State_Cond_Const_కేటాయించు(stmtsp, stateVscp, then_విలువ, else_విలువ)) తిరిగి assp;
        }
        తిరిగి శూన్యసూచిక;
    }

    స్థిర Ast_కణుపు* if_Chain_Supported_Transition_కణుపు(స్థిరరాశి FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        కొరకు (కొలత_t i = 0; i < chain.branches.కొలత(); ++i) {
            స్థిరరాశి FsmIfBranch& branch = chain.branches[i];
            Ast_కణుపు* స్థిరరాశి nodep = case_అంశం_Supported_Arc_కణుపు_Like(branch.stmtsp, stateVscp);
            ఒకవేళ (!nodep) తిరిగి శూన్యసూచిక;
        }
        తిరిగి chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    స్థిర string label_For_విలువ(
        స్థిరరాశి std::unordered_map<Fsm_State_విలువ, StateConstLabel, Fsm_State_విలువ_Hash>& labels,
        స్థిరరాశి Fsm_State_విలువ& విలువ) {
        తిరిగి labels.at(విలువ).పాఠ్యం;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    గణన తరగతి Const_విలువ_స్థితి : uint8_t { OK, NOT_CONST, XZ };

    స్థిర Const_విలువ_స్థితి const_విలువ_స్థితి(Ast_కణుపు_సమాసం* exprp, Fsm_State_విలువ& విలువ) {
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(exprp, Const);
        ఒకవేళ (!constp) తిరిగి Const_విలువ_స్థితి::NOT_CONST;
        స్థిరరాశి V3_సంఖ్య& అంకె = constp->అంకె();
        ఒకవేళ (అంకె.isAnyXZ()) తిరిగి Const_విలువ_స్థితి::XZ;
        విలువ = Fsm_State_విలువ{అంకె};
        తిరిగి Const_విలువ_స్థితి::OK;
    }

    స్థిర తార్కిక శుద్ధ_State_Comparison_No_Alias(Ast_కణుపు_సమాసం* exprp, FsmStateComparison& cmp) {
        AstEq* స్థిరరాశి eqp = VN_CAST(exprp, Eq);
        ఒకవేళ (!eqp) తిరిగి అబద్ధం;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_కణుపు_సమాసం* valuep = eqp->rhsp();
        ఒకవేళ (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->పునాది_Fromp(నిజం), VarRef);
            ఒకవేళ (!vrefp) { తిరిగి అబద్ధం; }
            valuep = eqp->lhsp();
        }

        Fsm_State_విలువ విలువ;
        ఒకవేళ (const_విలువ_స్థితి(valuep, విలువ) != Const_విలువ_స్థితి::OK) తిరిగి అబద్ధం;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.విలువ = విలువ;
        తిరిగి నిజం;
    }

    స్థిర తార్కిక శుద్ధ_State_Comparison(Ast_కణుపు_సమాసం* exprp, స్థిరరాశి FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        ఒకవేళ (శుద్ధ_State_Comparison_No_Alias(exprp, cmp)) తిరిగి నిజం;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        ఒకవేళ (AstVarRef* స్థిరరాశి vrefp = VN_CAST(exprp, VarRef)) {
            స్థిరరాశి FsmAliasMap::const_iterator it = aliases.కనుగొను(vrefp->varScopep());
            ఒకవేళ (it == aliases.చివర()) తిరిగి అబద్ధం;
            cmp = it->రెండవ;
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక supported_శిఖరం_Level_Guard(Ast_కణుపు_సమాసం* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        ఒకవేళ (VN_IS(exprp, Or)) తిరిగి అబద్ధం;
        ఒకవేళ (VN_IS(exprp, RedAnd)) తిరిగి అబద్ధం;
        ఒకవేళ (VN_IS(exprp, RedOr)) తిరిగి అబద్ధం;
        ఒకవేళ (VN_IS(exprp, RedXor)) తిరిగి అబద్ధం;
        తిరిగి నిజం;
    }

    స్థిర తార్కిక resolveIfPredicate(Ast_కణుపు_సమాసం* exprp, స్థిరరాశి FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, తార్కిక& hasGuard) {
        std::సదిశ<Ast_కణుపు_సమాసం*> terms;
        std::సదిశ<Ast_కణుపు_సమాసం*> pending;
        pending.నెట్టు_వెనుక(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        అయితే (!pending.ఖాళీలు()) {
            Ast_కణుపు_సమాసం* స్థిరరాశి nodep = pending.వెనుక();
            pending.తీయి_వెనుక();
            ఒకవేళ (AstAnd* స్థిరరాశి andp = VN_CAST(nodep, And)) {
                pending.నెట్టు_వెనుక(andp->rhsp());
                pending.నెట్టు_వెనుక(andp->lhsp());
            } లేకపోతే {
                terms.నెట్టు_వెనుక(nodep);
            }
        }

        తార్కిక sawComparison = అబద్ధం;
        కొరకు (కొలత_t i = 0; i < terms.కొలత(); ++i) {
            Ast_కణుపు_సమాసం* స్థిరరాశి termp = terms[i];
            FsmStateComparison పదం_Cmp;
            ఒకవేళ (శుద్ధ_State_Comparison(termp, aliases, పదం_Cmp /*ref*/)) {
                ఒకవేళ (sawComparison) తిరిగి అబద్ధం;
                cmp = పదం_Cmp;
                sawComparison = నిజం;
                కొనసాగు;
            }
            ఒకవేళ (!supported_శిఖరం_Level_Guard(termp)) తిరిగి అబద్ధం;
            hasGuard = నిజం;
        }
        తిరిగి sawComparison;
    }

    స్థిర శూన్య కూడు_Alias(FsmAliasMap& aliases, std::unordered_సమితి<స్థిరరాశి AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, స్థిరరాశి FsmStateComparison& cmp) {
        ఒకవేళ (ambiguous.కనుగొను(aliasVscp) != ambiguous.చివర()) తిరిగి;
        స్థిరరాశి FsmAliasMap::iterator it = aliases.కనుగొను(aliasVscp);
        ఒకవేళ (it == aliases.చివర()) {
            aliases.emplace(aliasVscp, cmp);
            తిరిగి;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        ఒకవేళ (it->రెండవ.stateVscp == cmp.stateVscp && it->రెండవ.విలువ == cmp.విలువ) తిరిగి;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        తిరిగి;
    }

    స్థిర శూన్య collect_Alias_From_కేటాయించు(Ast_కణుపు_కేటాయించు* assp, FsmAliasMap& aliases,
                                       std::unordered_సమితి<స్థిరరాశి AstVarScope*>& ambiguous) {
        AstVarRef* స్థిరరాశి lhsp = VN_CAST(assp->lhsp(), VarRef);
        ఒకవేళ (!lhsp) తిరిగి;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        ఒకవేళ (!శుద్ధ_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) తిరిగి;
        కూడు_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_కణుపు* stmtsp, Ast_కణుపు* limitp) స్థిరరాశి {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_సమితి<స్థిరరాశి AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        కొరకు (Ast_కణుపు* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = VN_CAST(nodep, కణుపు_కేటాయించు)) {
                collect_Alias_From_కేటాయించు(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        కొరకు (స్థిరరాశి AstVarScope* స్థిరరాశి vscp : ambiguous) aliases.erase(vscp);
        తిరిగి aliases;
    }

    స్థిర తార్కిక collectIfChain(AstIf* ifp, స్థిరరాశి FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_సమితి<string> చూసిన_విలువలు;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        అయితే (నిజం) {
            FsmStateComparison cmp;
            తార్కిక hasGuard = అబద్ధం;
            ఒకవేళ (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) తిరిగి అబద్ధం;
            ఒకవేళ (chain.compareVscp && chain.compareVscp != cmp.stateVscp) తిరిగి అబద్ధం;
            ఒకవేళ (!చూసిన_విలువలు.చొప్పించు(cmp.విలువ.కీలకం()).రెండవ) తిరిగి అబద్ధం;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.నెట్టు_వెనుక(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.విలువ, hasGuard});

            Ast_కణుపు* స్థిరరాశి elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            ఒకవేళ (!elseNodep) ఆపు;
            ఒకవేళ (AstIf* స్థిరరాశి elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                కొనసాగు;
            }
            chain.defaultStmtsp = elseNodep;
            ఆపు;
        }
        తిరిగి chain.branches.కొలత() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    స్థిర తార్కిక validate_తెలిసిన_State_విలువ(Ast_కణుపు* nodep, స్థిరరాశి FsmStateSpace& stateSpace,
                                        స్థిరరాశి Fsm_State_విలువ& విలువ, స్థిరరాశి string& role) {
        ఒకవేళ (stateSpace.labels.కనుగొను(విలువ) != stateSpace.labels.చివర()) తిరిగి నిజం;
        ఒకవేళ (stateSpace.enumBacked) {
            స్థిరరాశి string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + విలువ.warn_పాఠ్యం()
                                        + " is not present in the declared enum");
            తిరిగి అబద్ధం;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + విలువ.warn_పాఠ్యం()
                                    + " is not present in the inferred state space");
        తిరిగి అబద్ధం;
    }

    స్థిర StateConstLabel stateLabelForConst(AstConst* constp) {
        స్థిరరాశి string name = constp->origParamName();
        ఒకవేళ (!name.ఖాళీలు()) తిరిగి StateConstLabel{Ast_కణుపు::prettyName(name), నిజం, 0};
        తిరిగి StateConstLabel{constp->name(), అబద్ధం, 0};
    }

    స్థిర శూన్య updateStateLabel(FsmStateSpace& stateSpace, స్థిరరాశి Fsm_State_విలువ& విలువ,
                                 స్థిరరాశి StateConstLabel& label) {
        stateSpace.స్థితులు.at(stateSpace.labels.at(విలువ).state_సూచీ).మొదటి = label.పాఠ్యం;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    స్థిర తార్కిక సరిపోలిక_రిజిస్టరు_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_రిజిస్టరు_Candidate& cand) {
        ఒకవేళ (!alwaysp->sentreep() || !alwaysp->sentreep()->has_అంచు()) తిరిగి అబద్ధం;

        Ast_కణుపు* స్థిరరాశి stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_కణుపు* స్థిరరాశి nodep = singleMeaningfulStmt(stmtsp);
        ఒకవేళ (!nodep) తిరిగి అబద్ధం;

        AstVarScope* stateVscp = శూన్యసూచిక;
        AstVarScope* తదుపరి_Vscp = శూన్యసూచిక;
        ఒకవేళ (AstIf* స్థిరరాశి ifp = VN_CAST(nodep, If)) {
            ఒకవేళ (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) తిరిగి అబద్ధం;
            AstVarScope* resetStateVscp = శూన్యసూచిక;
            స్థిరరాశి Reset_కేటాయించు_స్థితి reset_స్థితి
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            స్థిరరాశి తార్కిక ఖాళీలు_Reset_Branch = reset_స్థితి == Reset_కేటాయించు_స్థితి::ఖాళీలు_3;
            ఒకవేళ (ఖాళీలు_Reset_Branch) {
                cand.resetArcs().clear();
            } లేకపోతే ఒకవేళ (reset_స్థితి == Reset_కేటాయించు_స్థితి::NONE) {
                cand.resetArcs().clear();
                Fsm_State_విలువ reset_విలువ;
                Ast_కణుపు* స్థిరరాశి thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_కణుపు_కేటాయించు* స్థిరరాశి resetAssp
                    = ప్రత్యక్ష_Const_State_కేటాయించు_కణుపు(thenNodep, resetStateVscp, reset_విలువ);
                ఒకవేళ (!resetAssp) తిరిగి అబద్ధం;
                cand.resetArcs().emplace_వెనుక(reset_విలువ, resetAssp);
            } లేకపోతే ఒకవేళ (reset_స్థితి == Reset_కేటాయించు_స్థితి::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_కణుపు* స్థిరరాశి elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            ఒకవేళ (!కణుపు_State_Var_కేటాయించు(elseNodep, stateVscp, తదుపరి_Vscp)) తిరిగి అబద్ధం;
            ఒకవేళ (!ఖాళీలు_Reset_Branch && resetStateVscp != stateVscp) తిరిగి అబద్ధం;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != శూన్యసూచిక);
        } లేకపోతే {
            Ast_కణుపు_సమాసం* resetCondp = శూన్యసూచిక;
            Fsm_State_విలువ reset_విలువ;
            // Note the active low 'reset ? next : CONST' form is not accepted
            ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assp
                = ప్రత్యక్ష_Cond_State_Var_కేటాయించు(nodep, stateVscp, తదుపరి_Vscp, resetCondp, reset_విలువ)) {
                cand.resetArcs().emplace_వెనుక(reset_విలువ, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != శూన్యసూచిక);
            } లేకపోతే ఒకవేళ (!కణుపు_State_Var_కేటాయించు(nodep, stateVscp, తదుపరి_Vscp)) {
                తిరిగి అబద్ధం;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.తదుపరి_Vscp(తదుపరి_Vscp);
        cand.senses() = describe_Sen_వృక్షం(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        తిరిగి నిజం;
    }

    స్థిర తార్కిక కూడు_విలువ_గమ్యం_State_Space(Ast_కణుపు* nodep, FsmStateSpace& stateSpace,
                                     స్థిరరాశి Fsm_State_విలువ& విలువ, StateConstLabel label) {
        స్థిరరాశి స్వయంచాలక labelIt = stateSpace.labels.కనుగొను(విలువ);
        ఒకవేళ (labelIt != stateSpace.labels.చివర()) {
            StateConstLabel& existingLabel = labelIt->రెండవ;
            ఒకవేళ (existingLabel.పాఠ్యం != label.పాఠ్యం && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + విలువ.warn_పాఠ్యం() + ": " + existingLabel.పాఠ్యం
                                            + " and " + label.పాఠ్యం);
                తిరిగి అబద్ధం;
            }
            ఒకవేళ (!existingLabel.fromParam && label.fromParam) {
                existingLabel.పాఠ్యం = label.పాఠ్యం;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, విలువ, label);
            }
        } లేకపోతే {
            StateConstLabel storedLabel = label;
            storedLabel.state_సూచీ = stateSpace.స్థితులు.కొలత();
            stateSpace.స్థితులు.emplace_వెనుక(label.పాఠ్యం, విలువ);
            stateSpace.labels.emplace(విలువ, storedLabel);
        }
        తిరిగి నిజం;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    స్థిర తార్కిక కూడు_సమాసం_గమ్యం_State_Space(Ast_కణుపు_సమాసం* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_విలువ విలువ;
        స్థిరరాశి Const_విలువ_స్థితి స్థితి_2 = const_విలువ_స్థితి(valuep, విలువ);
        ఒకవేళ (స్థితి_2 != Const_విలువ_స్థితి::OK) {
            ఒకవేళ (స్థితి_2 == Const_విలువ_స్థితి::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            తిరిగి అబద్ధం;
        }
        AstConst* స్థిరరాశి constp = VN_AS(valuep, Const);
        తిరిగి కూడు_విలువ_గమ్యం_State_Space(valuep, stateSpace, విలువ, stateLabelForConst(constp));
    }

    స్థిర తార్కిక కూడు_Optional_లక్ష్యం_సమాసం_గమ్యం_State_Space(Ast_కణుపు_సమాసం* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_విలువ విలువ;
        స్థిరరాశి Const_విలువ_స్థితి స్థితి_2 = const_విలువ_స్థితి(valuep, విలువ);
        ఒకవేళ (స్థితి_2 != Const_విలువ_స్థితి::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            తిరిగి అబద్ధం;
        }
        AstConst* స్థిరరాశి constp = VN_AS(valuep, Const);
        తిరిగి కూడు_విలువ_గమ్యం_State_Space(valuep, stateSpace, విలువ, stateLabelForConst(constp));
    }

    స్థిర శూన్య కూడు_Reset_Targets_గమ్యం_State_Space(స్థిరరాశి std::సదిశ<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        కొరకు (స్థిరరాశి FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.గమ్యం_విలువ().ascii(), అబద్ధం, 0};
            ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                కూడు_విలువ_గమ్యం_State_Space(resetArc.nodep(), stateSpace, resetArc.గమ్యం_విలువ(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    స్థిర తార్కిక collectDeclaredStateSpace(Ast_కణుపు* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, తార్కిక& needs_ఆధారం_విలువలు) {
        AstVar* స్థిరరాశి stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        ఒకవేళ (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        స్థిరరాశి తార్కిక forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        ఒకవేళ (enump) {
            stateSpace.enumBacked = నిజం;
            కొరకు (Ast_Enum_అంశం* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(itemp->valuep(), Const);
                స్థిరరాశి Fsm_State_విలువ విలువ{constp->అంకె()};
                స్థిరరాశి కొలత_t state_సూచీ = stateSpace.స్థితులు.కొలత();
                stateSpace.స్థితులు.emplace_వెనుక(itemp->name(), విలువ);
                stateSpace.labels.emplace(విలువ,
                                          StateConstLabel{itemp->name(), అబద్ధం, state_సూచీ});
            }
            తిరిగి stateSpace.స్థితులు.కొలత() >= 2;
        }

        ఒకవేళ (forced) {
            needs_ఆధారం_విలువలు = నిజం;
            తిరిగి నిజం;
        }

        needs_ఆధారం_విలువలు = నిజం;
        తిరిగి నిజం;
    }

    మూస <రకం_పేరు T_ValuepVisitor>
    స్థిర తార్కిక collect_State_Space_From_విలువలు(Ast_కణుపు* warnNodep, AstVarScope* stateVscp,
                                            స్థిరరాశి std::సదిశ<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            స్థిరరాశి T_ValuepVisitor& సందర్శించు_Valueps) {
        తార్కిక needs_ఆధారం_విలువలు = అబద్ధం;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        ఒకవేళ (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_ఆధారం_విలువలు)) {
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (!needs_ఆధారం_విలువలు) తిరిగి నిజం;
        కూడు_Reset_Targets_గమ్యం_State_Space(resetArcs, stateSpace);
        ఒకవేళ (!సందర్శించు_Valueps(
                [&](Ast_కణుపు_సమాసం* valuep) { తిరిగి కూడు_సమాసం_గమ్యం_State_Space(valuep, stateSpace); })) {
            తిరిగి అబద్ధం;
        }
        తిరిగి stateSpace.స్థితులు.కొలత() >= 2;
    }

    స్థిర తార్కిక collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* కేటాయించు_Vscp,
                                  స్థిరరాశి std::సదిశ<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        తిరిగి collect_State_Space_From_విలువలు(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, కేటాయించు_Vscp, &stateSpace](స్థిరరాశి స్వయంచాలక& సందర్శించు_Valuep) {
                కొరకు (Ast_Case_అంశం* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Case_అంశం)) {
                    ఒకవేళ (!itemp->isDefault()) {
                        కొరకు (Ast_కణుపు_సమాసం* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), కణుపు_సమాసం)) {
                            ఒకవేళ (!సందర్శించు_Valuep(condp)) తిరిగి అబద్ధం;
                        }
                    }
                    ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assp
                        = ప్రత్యక్ష_State_కేటాయించు(itemp->stmtsp(), కేటాయించు_Vscp)) {
                        Fsm_State_విలువ then_విలువ;
                        Fsm_State_విలువ else_విలువ;
                        AstCond* స్థిరరాశి condp = VN_CAST(assp->rhsp(), Cond);
                        ఒకవేళ (condp
                            && ప్రత్యక్ష_State_Cond_Const_కేటాయించు(itemp->stmtsp(), కేటాయించు_Vscp, then_విలువ,
                                                          else_విలువ)) {
                            ఒకవేళ (!సందర్శించు_Valuep(condp->thenp())) తిరిగి అబద్ధం;
                            ఒకవేళ (!సందర్శించు_Valuep(condp->elsep())) తిరిగి అబద్ధం;
                        } లేకపోతే ఒకవేళ (!కూడు_Optional_లక్ష్యం_సమాసం_గమ్యం_State_Space(assp->rhsp(), stateSpace)) {
                            తిరిగి అబద్ధం;
                        }
                    }
                }
                తిరిగి నిజం;
            });
    }

    స్థిర తార్కిక collectStateSpace(స్థిరరాశి FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* కేటాయించు_Vscp,
                                  స్థిరరాశి std::సదిశ<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        తిరిగి collect_State_Space_From_విలువలు(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, కేటాయించు_Vscp, &stateSpace](స్థిరరాశి స్వయంచాలక& సందర్శించు_Valuep) {
                కొరకు (స్థిరరాశి FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(సందర్శించు_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = ప్రత్యక్ష_State_కేటాయించు(branch.stmtsp, కేటాయించు_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_విలువ then_విలువ;
                    Fsm_State_విలువ else_విలువ;
                    AstCond* స్థిరరాశి condp = VN_CAST(assp->rhsp(), Cond);
                    ఒకవేళ (condp) {
                        UASSERT_OBJ(ప్రత్యక్ష_State_Cond_Const_కేటాయించు(branch.stmtsp, కేటాయించు_Vscp,
                                                               then_విలువ, else_విలువ),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        ఒకవేళ (!సందర్శించు_Valuep(condp->thenp())) తిరిగి అబద్ధం;
                        ఒకవేళ (!సందర్శించు_Valuep(condp->elsep())) తిరిగి అబద్ధం;
                    } లేకపోతే ఒకవేళ (!కూడు_Optional_లక్ష్యం_సమాసం_గమ్యం_State_Space(assp->rhsp(), stateSpace)) {
                        తిరిగి అబద్ధం;
                    }
                }
                తిరిగి నిజం;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    స్థిర తార్కిక emit_Case_అంశం_Arcs(Fsm_లేఖాచిత్రం& లేఖాచిత్రం, Ast_Case_అంశం* itemp, AstVarScope* stateVscp,
                                 స్థిరరాశి FsmStateSpace& stateSpace, తార్కిక inclCond) {
        std::సదిశ<std::pair<string, Fsm_State_విలువ>> froms;
        ఒకవేళ (itemp->isDefault()) {
            ఒకవేళ (!inclCond) తిరిగి అబద్ధం;
            froms.emplace_వెనుక("default", Fsm_State_విలువ{});
        } లేకపోతే {
            కొరకు (Ast_కణుపు_సమాసం* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), కణుపు_సమాసం)) {
                Fsm_State_విలువ విలువ;
                ఒకవేళ (const_విలువ_స్థితి(condp, విలువ) != Const_విలువ_స్థితి::OK) కొనసాగు;
                ఒకవేళ (!validate_తెలిసిన_State_విలువ(condp, stateSpace, విలువ, "source")) తిరిగి నిజం;
                froms.emplace_వెనుక(label_For_విలువ(stateSpace.labels, విలువ), విలువ);
            }
            ఒకవేళ (froms.ఖాళీలు()) తిరిగి అబద్ధం;
        }

        ఒకవేళ (Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = ప్రత్యక్ష_State_కేటాయించు(itemp->stmtsp(), stateVscp)) {
            Fsm_State_విలువ గమ్యం_విలువ;
            స్థిరరాశి Const_విలువ_స్థితి స్థితి_2 = const_విలువ_స్థితి(assp->rhsp(), గమ్యం_విలువ);
            ఒకవేళ (స్థితి_2 == Const_విలువ_స్థితి::OK) {
                ఒకవేళ (!validate_తెలిసిన_State_విలువ(assp, stateSpace, గమ్యం_విలువ, "target")) తిరిగి నిజం;
                కొరకు (స్థిరరాశి std::pair<string, Fsm_State_విలువ>& from : froms) {
                    లేఖాచిత్రం.కూడు_Arc(from.రెండవ, గమ్యం_విలువ, అబద్ధం, అబద్ధం, itemp->isDefault(),
                                 assp->fileline());
                }
                తిరిగి నిజం;
            }
        }

        Fsm_State_విలువ then_విలువ;
        Fsm_State_విలువ else_విలువ;
        ఒకవేళ (ప్రత్యక్ష_State_Cond_Const_కేటాయించు(itemp->stmtsp(), stateVscp, then_విలువ, else_విలువ)
            || if_State_Const_కేటాయించు(itemp->stmtsp(), stateVscp, then_విలువ, else_విలువ)) {
            ఒకవేళ (!validate_తెలిసిన_State_విలువ(itemp->stmtsp(), stateSpace, then_విలువ, "target"))
                తిరిగి నిజం;
            ఒకవేళ (!validate_తెలిసిన_State_విలువ(itemp->stmtsp(), stateSpace, else_విలువ, "target"))
                తిరిగి నిజం;
            కొరకు (స్థిరరాశి Fsm_State_విలువ& branch_విలువ : {then_విలువ, else_విలువ}) {
                కొరకు (స్థిరరాశి std::pair<string, Fsm_State_విలువ>& from : froms) {
                    లేఖాచిత్రం.కూడు_Arc(from.రెండవ, branch_విలువ, అబద్ధం, నిజం, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            తిరిగి నిజం;
        }

        తిరిగి అబద్ధం;
    }

    స్థిర శూన్య emitStmtArcsFrom(Fsm_లేఖాచిత్రం& లేఖాచిత్రం, Ast_కణుపు* stmtsp, AstVarScope* stateVscp,
                                 స్థిరరాశి FsmStateSpace& stateSpace, Fsm_State_విలువ from_విలువ,
                                 తార్కిక isDefault, తార్కిక forceCond) {
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = ప్రత్యక్ష_State_కేటాయించు(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_విలువ గమ్యం_విలువ;
        స్థిరరాశి Const_విలువ_స్థితి స్థితి_2 = const_విలువ_స్థితి(assp->rhsp(), గమ్యం_విలువ);
        ఒకవేళ (స్థితి_2 == Const_విలువ_స్థితి::OK) {
            ఒకవేళ (!validate_తెలిసిన_State_విలువ(assp, stateSpace, గమ్యం_విలువ, "target")) తిరిగి;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            లేఖాచిత్రం.కూడు_Arc(from_విలువ, గమ్యం_విలువ, అబద్ధం, forceCond, isDefault, assp->fileline());
            తిరిగి;
        }

        Fsm_State_విలువ then_విలువ;
        Fsm_State_విలువ else_విలువ;
        స్థిరరాశి తార్కిక cond_కేటాయించు
            = ప్రత్యక్ష_State_Cond_Const_కేటాయించు(stmtsp, stateVscp, then_విలువ, else_విలువ);
        UASSERT_OBJ(cond_కేటాయించు, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        ఒకవేళ (!validate_తెలిసిన_State_విలువ(stmtsp, stateSpace, then_విలువ, "target")) తిరిగి;
        ఒకవేళ (!validate_తెలిసిన_State_విలువ(stmtsp, stateSpace, else_విలువ, "target")) తిరిగి;
        కొరకు (స్థిరరాశి Fsm_State_విలువ& branch_విలువ : {then_విలువ, else_విలువ}) {
            లేఖాచిత్రం.కూడు_Arc(from_విలువ, branch_విలువ, అబద్ధం, నిజం, isDefault, stmtsp->fileline());
        }
    }

    స్థిర శూన్య emitIfChainArcs(Fsm_లేఖాచిత్రం& లేఖాచిత్రం, స్థిరరాశి FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, స్థిరరాశి FsmStateSpace& stateSpace) {
        కొరకు (కొలత_t i = 0; i < chain.branches.కొలత(); ++i) {
            స్థిరరాశి FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            ఒకవేళ (!validate_తెలిసిన_State_విలువ(branch.ifp, stateSpace, branch.from_విలువ, "source"))
                తిరిగి;
            emitStmtArcsFrom(లేఖాచిత్రం, branch.stmtsp, stateVscp, stateSpace, branch.from_విలువ, అబద్ధం,
                             branch.has_శిఖరం_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    స్థిర శూన్య కూడు_Reset_Arcs(Fsm_లేఖాచిత్రం& లేఖాచిత్రం, స్థిరరాశి std::సదిశ<FsmResetArcDesc>& resetArcs,
                             స్థిరరాశి FsmStateSpace& stateSpace) {
        కొరకు (స్థిరరాశి FsmResetArcDesc& resetArc : resetArcs) {
            ఒకవేళ (!validate_తెలిసిన_State_విలువ(resetArc.nodep(), stateSpace, resetArc.గమ్యం_విలువ(),
                                         "target"))
                కొనసాగు;
            లేఖాచిత్రం.కూడు_Arc(Fsm_State_విలువ{}, resetArc.గమ్యం_విలువ(), నిజం, అబద్ధం, అబద్ధం,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    శూన్య processCase(AstCase* casep, AstVarScope* కేటాయించు_Vscp, స్థిరరాశి Fsm_రిజిస్టరు_Candidate& reg) {
        UASSERT_OBJ(కేటాయించు_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* స్థిరరాశి stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ఒకవేళ (!collectStateSpace(casep, stateVscp, కేటాయించు_Vscp, reg.resetArcs(), stateSpace)) తిరిగి;
        DetectedFsm& నమోదు_2 = m_state.fsmFor(stateVscp);
        ఒకవేళ (!నమోదు_2.graphp) {
            నమోదు_2.graphp.reset(కొత్త Fsm_లేఖాచిత్రం{});
            నమోదు_2.graphp->scopep(reg.scopep());
            నమోదు_2.graphp->stateAlwaysp(reg.alwaysp());
            నమోదు_2.graphp->stateVarName(stateVscp->prettyName());
            నమోదు_2.graphp->state_Var_అంతర్గత_Name(stateVscp->varp()->name());
            నమోదు_2.graphp->stateVarScopep(stateVscp);
            నమోదు_2.graphp->sampleVarScopep(reg.sampleVscp());
            నమోదు_2.graphp->senses() = reg.senses();
            నమోదు_2.graphp->resetCond() = reg.resetCond();
            నమోదు_2.graphp->hasResetCond(reg.hasResetCond());
            నమోదు_2.graphp->inclCond(reg.inclCond());
            నమోదు_2.graphp->fileline(casep->fileline());
            కొరకు (స్థిరరాశి std::pair<string, Fsm_State_విలువ>& state : stateSpace.స్థితులు) {
                నమోదు_2.graphp->కూడు_State_శీర్షం(state.మొదటి, state.రెండవ);
            }
            కూడు_Reset_Arcs(*నమోదు_2.graphp, reg.resetArcs(), stateSpace);
        }
        కొరకు (Ast_Case_అంశం* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_అంశం)) {
            emit_Case_అంశం_Arcs(*నమోదు_2.graphp, itemp, కేటాయించు_Vscp, stateSpace,
                             నమోదు_2.graphp->inclCond());
        }
    }

    శూన్య processIfChain(స్థిరరాశి FsmIfChainCandidate& chain, AstVarScope* కేటాయించు_Vscp,
                        స్థిరరాశి Fsm_రిజిస్టరు_Candidate& reg) {
        UASSERT_OBJ(కేటాయించు_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* స్థిరరాశి stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        ఒకవేళ (!collectStateSpace(chain, stateVscp, కేటాయించు_Vscp, reg.resetArcs(), stateSpace)) తిరిగి;
        DetectedFsm& నమోదు_2 = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!నమోదు_2.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        నమోదు_2.graphp.reset(కొత్త Fsm_లేఖాచిత్రం{});
        నమోదు_2.graphp->scopep(reg.scopep());
        నమోదు_2.graphp->stateAlwaysp(reg.alwaysp());
        నమోదు_2.graphp->stateVarName(stateVscp->prettyName());
        నమోదు_2.graphp->state_Var_అంతర్గత_Name(stateVscp->varp()->name());
        నమోదు_2.graphp->stateVarScopep(stateVscp);
        నమోదు_2.graphp->sampleVarScopep(reg.sampleVscp());
        నమోదు_2.graphp->senses() = reg.senses();
        నమోదు_2.graphp->resetCond() = reg.resetCond();
        నమోదు_2.graphp->hasResetCond(reg.hasResetCond());
        నమోదు_2.graphp->inclCond(reg.inclCond());
        నమోదు_2.graphp->fileline(chain.ifp->fileline());
        కొరకు (స్థిరరాశి std::pair<string, Fsm_State_విలువ>& state : stateSpace.స్థితులు) {
            నమోదు_2.graphp->కూడు_State_శీర్షం(state.మొదటి, state.రెండవ);
        }
        కూడు_Reset_Arcs(*నమోదు_2.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*నమోదు_2.graphp, chain, కేటాయించు_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    శూన్య process_One_ఖండం_Always(స్థిరరాశి FsmComboAlways& one_ఖండం) {
        స్థిరరాశి రిజిస్టరు_Always_Analyzer analyzer{one_ఖండం.scopep()};
        AstAlways* స్థిరరాశి alwaysp = one_ఖండం.alwaysp();
        ఒకవేళ (!alwaysp->sentreep() || !alwaysp->sentreep()->has_అంచు()) తిరిగి;
        స్థిరరాశి std::సదిశ<std::pair<AstCase*, Ast_కణుపు_సమాసం*>> candidates
            = analyzer.one_ఖండం_Candidates(alwaysp);

        FsmCaseCandidate మొదటి_Cand;
        కొరకు (స్థిరరాశి std::pair<AstCase*, Ast_కణుపు_సమాసం*>& cand : candidates) {
            AstVarRef* స్థిరరాశి selp = VN_CAST(cand.మొదటి->exprp(), VarRef);
            AstVarScope* స్థిరరాశి vscp = selp ? selp->varScopep() : శూన్యసూచిక;
            ఒకవేళ (!vscp) కొనసాగు;
            ఒకవేళ (!మొదటి_Cand.stateVscp) {
                మొదటి_Cand.warnNodep = cand.మొదటి;
                మొదటి_Cand.stateVscp = vscp;
                Fsm_రిజిస్టరు_Candidate reg;
                analyzer.build_One_ఖండం_Candidate(alwaysp, vscp, cand.రెండవ, reg);
                processCase(cand.మొదటి, vscp, reg);
            } లేకపోతే ఒకవేళ (vscp != మొదటి_Cand.stateVscp) {
                cand.మొదటి->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.మొదటి, మొదటి_Cand));
            } లేకపోతే {
                cand.మొదటి->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.మొదటి, మొదటి_Cand));
            }
        }

        ఒకవేళ (మొదటి_Cand.stateVscp) తిరిగి;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        స్థిరరాశి std::సదిశ<std::pair<AstIf*, Ast_కణుపు_సమాసం*>> ifCandidates
            = analyzer.one_ఖండం_If_Candidates(alwaysp);
        కొరకు (స్థిరరాశి std::pair<AstIf*, Ast_కణుపు_సమాసం*>& cand : ifCandidates) {
            స్థిరరాశి FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.మొదటి);
            FsmIfChainCandidate chain;
            ఒకవేళ (!collectIfChain(cand.మొదటి, aliases, chain)) కొనసాగు;
            AstVarScope* స్థిరరాశి vscp = chain.compareVscp;
            ఒకవేళ (!if_Chain_Supported_Transition_కణుపు(chain, vscp)) కొనసాగు;
            ఒకవేళ (!మొదటి_Cand.stateVscp) {
                మొదటి_Cand.warnNodep = cand.మొదటి;
                మొదటి_Cand.stateVscp = vscp;
                Fsm_రిజిస్టరు_Candidate reg;
                analyzer.build_One_ఖండం_Candidate(alwaysp, vscp, cand.రెండవ, reg);
                processIfChain(chain, vscp, reg);
            } లేకపోతే ఒకవేళ (vscp != మొదటి_Cand.stateVscp) {
                cand.మొదటి->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.మొదటి, మొదటి_Cand));
            } లేకపోతే {
                cand.మొదటి->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.మొదటి, మొదటి_Cand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    స్థిర శూన్య warnComboSameAlways(Ast_కణుపు* warnNodep, స్థిరరాశి FsmCaseCandidate& మొదటి_Cand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, మొదటి_Cand));
    }

    శూన్య processComboAlways(స్థిరరాశి FsmComboAlways& combo) {
        స్థిరరాశి ComboAlwaysAnalyzer analyzer{m_రిజిస్టరు_Candidates};
        Ast_కణుపు* స్థిరరాశి stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate మొదటి_Cand;
        కొరకు (Ast_కణుపు* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* స్థిరరాశి casep = VN_CAST(nodep, Case);
            ఒకవేళ (!casep) కొనసాగు;
            స్థిరరాశి ComboAlwaysAnalyzer::Combo_సరిపోలిక సరిపోలిక = analyzer.సరిపోలిక_Case(stmtsp, casep);
            స్థిరరాశి Fsm_రిజిస్టరు_Candidate* స్థిరరాశి matchedp = సరిపోలిక.matchedp;
            Ast_కణుపు* స్థిరరాశి matchedWarnNodep = సరిపోలిక.warnNodep;
            ఒకవేళ (!matchedp) కొనసాగు;
            ఒకవేళ (!మొదటి_Cand.stateVscp) {
                స్థిరరాశి స్వయంచాలక చొప్పించు_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                ఒకవేళ (!చొప్పించు_Pair.రెండవ) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  చొప్పించు_Pair.మొదటి->రెండవ));
                    కొనసాగు;
                }
                మొదటి_Cand.warnNodep = matchedWarnNodep;
                మొదటి_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->తదుపరి_Vscp(), *matchedp);
                కొనసాగు;
            }
            ఒకవేళ (matchedp->stateVscp() != మొదటి_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, మొదటి_Cand);
                కొనసాగు;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, మొదటి_Cand));
        }
        ఒకవేళ (మొదటి_Cand.stateVscp) తిరిగి;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        కొరకు (Ast_కణుపు* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* స్థిరరాశి ifp = VN_CAST(nodep, If);
            ఒకవేళ (!ifp) కొనసాగు;
            FsmIfChainCandidate chain;
            స్థిరరాశి FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            ఒకవేళ (!collectIfChain(ifp, aliases, chain)) కొనసాగు;
            స్థిరరాశి ComboAlwaysAnalyzer::Combo_సరిపోలిక సరిపోలిక = analyzer.సరిపోలిక_If_Chain(stmtsp, chain);
            స్థిరరాశి Fsm_రిజిస్టరు_Candidate* స్థిరరాశి matchedp = సరిపోలిక.matchedp;
            Ast_కణుపు* స్థిరరాశి matchedWarnNodep = సరిపోలిక.warnNodep;
            ఒకవేళ (!matchedp) కొనసాగు;
            ఒకవేళ (!మొదటి_Cand.stateVscp) {
                స్థిరరాశి std::pair<std::unordered_map<స్థిరరాశి AstVarScope*, FsmCaseCandidate>::iterator,
                                తార్కిక>
                    చొప్పించు_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                ఒకవేళ (!చొప్పించు_Pair.రెండవ) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  చొప్పించు_Pair.మొదటి->రెండవ));
                    కొనసాగు;
                }
                మొదటి_Cand.warnNodep = matchedWarnNodep;
                మొదటి_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->తదుపరి_Vscp(), *matchedp);
                కొనసాగు;
            }
            ఒకవేళ (matchedp->stateVscp() != మొదటి_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, మొదటి_Cand);
                కొనసాగు;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, మొదటి_Cand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    శూన్య సందర్శించు(AstAlways* nodep) override {
        ఒకవేళ (nodep->keyword() == VAlwaysKwd::CONT_కేటాయించు) {
            iterateChildren(nodep);
            తిరిగి;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_ఖండం_Alwayss.emplace_వెనుక(m_scopep, nodep);
        స్థిరరాశి రిజిస్టరు_Always_Analyzer analyzer{m_scopep};
        Fsm_రిజిస్టరు_Candidate reg;
        ఒకవేళ (analyzer.సరిపోలిక_రిజిస్టరు_Candidate(nodep, reg)) {
            AstVarScope* స్థిరరాశి stateVscp = reg.stateVscp();
            స్థిరరాశి తార్కిక దొరికింది
                = std::any_యొక్క(m_రిజిస్టరు_Candidates.cbegin(), m_రిజిస్టరు_Candidates.cend(),
                              [stateVscp](స్థిరరాశి Fsm_రిజిస్టరు_Candidate& existing) {
                                  తిరిగి existing.stateVscp() == stateVscp;
                              });
            ఒకవేళ (!దొరికింది) { m_రిజిస్టరు_Candidates.emplace_వెనుక(reg); }
        }
        ఒకవేళ (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_వెనుక(m_scopep, nodep);
        } లేకపోతే ఒకవేళ (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            ఒకవేళ (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_వెనుక(m_scopep, nodep);
            } లేకపోతే {
                m_nonComboAlwayss.emplace_వెనుక(m_scopep, nodep);
            }
        }
    }

    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_కేటాయించు(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_పోర్టు_Alias(nodep);
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        ఒకవేళ (m_scopep && fsm_రిజిస్టరు_Wrapper_Desc(nodep)) కూడు_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        కొరకు (స్థిరరాశి std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_రిజిస్టరు_Candidate reg;
            ఒకవేళ (సరిపోలిక_Fsm_Wrapper_Cell(wrapperCell.మొదటి, wrapperCell.రెండవ, reg)) {
                m_రిజిస్టరు_Candidates.emplace_వెనుక(reg);
            }
        }
        కొరకు (స్థిరరాశి FsmComboAlways& one_ఖండం : m_one_ఖండం_Alwayss) process_One_ఖండం_Always(one_ఖండం);
        కొరకు (స్థిరరాశి FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        కొరకు (స్థిరరాశి FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
తరగతి FsmCoverIncBuilder final {
    స్థిరరాశి Fsm_లేఖాచిత్రం& m_లేఖాచిత్రం;  // Fsm graph
    std::సదిశ<స్థిరరాశి Fsm_State_శీర్షం*> m_స్థితులు;  // States vertexes inside fsm graph
    std::unordered_సమితి<uint64_t>
        m_undefined_స్థితులు;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    స్థిరరాశి Fsm_శీర్షం* m_defaultVertexp = శూన్యసూచిక;  // Default vertex
    స్థిరరాశి Fsm_శీర్షం* m_resetVertexp = శూన్యసూచిక;  // Reset vertex
    కొలత_t m_అంచులు_లెక్క = 0;  // Total count of edges
    AstVarScope* స్థిరరాశి m_prevVscp;  // Previous state of fsm
    File_పంక్తి* స్థిరరాశి m_flp;  // Fileline pointer to use across the builder
    స్థిరరాశి std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_కణుపు_Stmt* m_coverIncp = శూన్యసూచిక;  // Increments proper CoverDecl after each change
    Ast_కణుపు_Stmt* m_initialCoverIncp = శూన్యసూచిక;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = శూన్యసూచిక;  // List of CoverDecls
    తార్కిక m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(స్థిరరాశి uint64_t విలువ) {
        V3_సంఖ్య అంకె{static_cast<Ast_కణుపు*>(శూన్యసూచిక), m_prevVscp->వెడల్పు()};
        అంకె.సమితి_Quad(విలువ);
        అంకె.isSigned(m_prevVscp->dtypep()->isSigned());
        తిరిగి కొత్త AstConst{m_flp, అంకె};
    }

    std::string quad_గమ్యం_బిట్టు_Stirng_Label(స్థిరరాశి uint64_t val) {
        పూర్ణాంకం వెడల్పు = m_prevVscp->వెడల్పు();
        UASSERT(వెడల్పు < 64, "We should not expand values that wide");
        std::string ఫలితం = std::గమ్యం_string(వెడల్పు) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        అయితే (--వెడల్పు >= 0) ఫలితం.నెట్టు_వెనుక("01"[(val >> వెడల్పు) & 1]);
        తిరిగి ఫలితం;
    }

    శూన్య init_Existing_స్థితులు() {
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_శీర్షం& vtx : m_లేఖాచిత్రం.vertices()) {
            స్థిరరాశి Fsm_శీర్షం* స్థిరరాశి vertexp = vtx.as<Fsm_శీర్షం>();
            m_అంచులు_లెక్క += vertexp->లోపల_అంచులు().కొలత();
            ఒకవేళ (స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి statep = vertexp->cast<Fsm_State_శీర్షం>()) {
                m_స్థితులు.నెట్టు_వెనుక(statep);
            } లేకపోతే ఒకవేళ (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_లేఖాచిత్రం.sampleVarScopep(),
                            "Two defaults?");
            } లేకపోతే /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_లేఖాచిత్రం.sampleVarScopep(),
                            "Two resets?");
            }
        }
        స్థిరరాశి పూర్ణాంకం వెడల్పు = m_లేఖాచిత్రం.sampleVarScopep()->వెడల్పు();
        m_isComplete = వెడల్పు < 64 && m_స్థితులు.కొలత() == (static_cast<కొలత_t>(1) << వెడల్పు);
    }

    శూన్య init_Undefined_స్థితులు() {
        UASSERT_OBJ(m_లేఖాచిత్రం.sampleVarScopep()->వెడల్పు() < 64, m_లేఖాచిత్రం.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        స్థిరరాశి uint64_t combinations_లెక్క = static_cast<కొలత_t>(1)
                                           << m_లేఖాచిత్రం.sampleVarScopep()->వెడల్పు();
        // Rolling trick allows to cut peak memory usage
        తార్కిక undefRolled = అబద్ధం;
        స్వయంచాలక rollUndefs = [ఇది, combinations_లెక్క] {
            కొరకు (uint64_t i = 0; i < combinations_లెక్క; ++i) {
                స్థిరరాశి స్వయంచాలక చొప్పించు = m_undefined_స్థితులు.చొప్పించు(i);
                ఒకవేళ (!చొప్పించు.రెండవ) m_undefined_స్థితులు.erase(చొప్పించు.మొదటి);
            }
        };
        కొరకు (స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి statep : m_స్థితులు) {
            స్థిరరాశి V3_సంఖ్య& అంకె = statep->విలువ().అంకె();
            స్థిరరాశి uint64_t విలువ = అంకె.గమ్యం_UQuad();
            ఒకవేళ (!m_isComplete) {
                ఒకవేళ (undefRolled) {
                    UASSERT_OBJ(m_undefined_స్థితులు.erase(విలువ) == 1, m_లేఖాచిత్రం.sampleVarScopep(),
                                "Two states has the same value?");
                } లేకపోతే {
                    UASSERT_OBJ(m_undefined_స్థితులు.చొప్పించు(విలువ).రెండవ, m_లేఖాచిత్రం.sampleVarScopep(),
                                "Two states has the same value?");
                    ఒకవేళ (m_undefined_స్థితులు.కొలత() * 2 >= combinations_లెక్క) {
                        undefRolled = నిజం;
                        rollUndefs();
                    }
                }
            }
        }
        ఒకవేళ (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_కణుపు_సమాసం* buildResetCond() స్థిరరాశి {
        Ast_కణుపు_సమాసం* స్థిరరాశి refp
            = కొత్త AstVarRef{m_flp, m_లేఖాచిత్రం.resetCond().varScopep, VAccess::READ};
        తిరిగి m_లేఖాచిత్రం.resetCond().చురుకైన_దిగువ
                   ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstLogNot{m_flp, refp})
                   : refp;
    }

    కొలత_t get_Arc_లెక్క_With_Expansion(స్థిరరాశి VFsmExpandType expand) స్థిరరాశి {
        కొలత_t arc_లెక్క_With_Expansion = ~0ULL;
        ఎంపిక (expand) {
        స్థితి VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_లేఖాచిత్రం.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            ఆపు;
        }
        స్థితి VFsmExpandType::AUTO: {
            arc_లెక్క_With_Expansion = m_అంచులు_లెక్క;
            ఒకవేళ (m_defaultVertexp) {
                UASSERT_OBJ(m_స్థితులు.కొలత() >= m_defaultVertexp->బహిర్గతం_అంచులు().కొలత(),
                            m_లేఖాచిత్రం.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_లెక్క_With_Expansion -= m_defaultVertexp->బహిర్గతం_అంచులు().కొలత();
                కొరకు (స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి statep : m_స్థితులు) {
                    arc_లెక్క_With_Expansion += static_cast<కొలత_t>(statep->బహిర్గతం_అంచులు().ఖాళీలు())
                                             * m_defaultVertexp->బహిర్గతం_అంచులు().కొలత();
                }
            }
            ఒకవేళ (m_resetVertexp) {
                UASSERT_OBJ(m_స్థితులు.కొలత() >= m_resetVertexp->బహిర్గతం_అంచులు().కొలత(),
                            m_లేఖాచిత్రం.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_లెక్క_With_Expansion += m_స్థితులు.కొలత() - m_resetVertexp->బహిర్గతం_అంచులు().కొలత();
            }
            ఆపు;
        }
        స్థితి VFsmExpandType::AUTO_EXPAND: {
            arc_లెక్క_With_Expansion = m_స్థితులు.కొలత() * m_స్థితులు.కొలత() - m_స్థితులు.కొలత();
            ఆపు;
        }
        స్థితి VFsmExpandType::నిండిన: {
            స్థిరరాశి పూర్ణాంకం వెడల్పు = m_లేఖాచిత్రం.sampleVarScopep()->వెడల్పు();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            ఒకవేళ (వెడల్పు < 32) {
                స్థిరరాశి uint64_t combinations_లెక్క = static_cast<కొలత_t>(1) << వెడల్పు;
                arc_లెక్క_With_Expansion = combinations_లెక్క * combinations_లెక్క - combinations_లెక్క;
            }
            ఆపు;
        }
        }
        తిరిగి arc_లెక్క_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(స్థిరరాశి std::string& label,
                                           స్థిరరాశి std::string& fsm_ట్యాగ్ = "") {
        AstCoverOtherDecl* స్థిరరాశి declp
            = కొత్త AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_లేఖాచిత్రం.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_లేఖాచిత్రం.stateVarName(),
                                    "",
                                    label,
                                    fsm_ట్యాగ్};
        // We run after scoping, so this is declared for one instance only
        declp->చొప్పున_Instance(నిజం);
        m_declsp = Ast_కణుపు::కూడు_తదుపరి(m_declsp, declp);
        తిరిగి declp;
    }

    Ast_కణుపు_Stmt* buildCoverIncStmtp(స్థిరరాశి std::string& fromStateLabel,
                                    స్థిరరాశి std::string& గమ్యం_State_Label,
                                    స్థిరరాశి std::string& పునాది_Label = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* స్థిరరాశి declp = కొత్త AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_లేఖాచిత్రం.stateVarName() + "::" + fromStateLabel + "->" + గమ్యం_State_Label,
            "",
            0,
            m_లేఖాచిత్రం.stateVarName(),
            fromStateLabel,
            గమ్యం_State_Label,
            పునాది_Label};
        // We run after scoping, so this is declared for one instance only
        declp->చొప్పున_Instance(నిజం);
        m_declsp = Ast_కణుపు::కూడు_తదుపరి(m_declsp, declp);
        తిరిగి కొత్త AstCoverInc{m_flp, declp};
    }
    Ast_కణుపు_Stmt* buildExpandedTransitionHandler(స్థిరరాశి std::string& గమ్యం_State_Label,
                                                స్థిరరాశి తార్కిక considerUndefs,
                                                స్థిరరాశి తార్కిక expandOnlyWithTerminalVerticies,
                                                స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి గమ్యం_Statep = శూన్యసూచిక) {
        AstCase* స్థిరరాశి arcCasep = కొత్త AstCase{
            m_flp, VCaseType::CT_CASE, కొత్త AstVarRef{m_flp, m_prevVscp, VAccess::READ}, శూన్యసూచిక};
        std::unordered_సమితి<స్థిరరాశి Fsm_State_శీర్షం*> handledSoureces;
        ఒకవేళ (గమ్యం_Statep) {
            కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : గమ్యం_Statep->లోపల_అంచులు()) {
                ఒకవేళ (స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి fromp = అంచు.fromp()->cast<Fsm_State_శీర్షం>()) {
                    ఒకవేళ (!handledSoureces.emplace(fromp).రెండవ) కొనసాగు;
                    arcCasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{
                        m_flp, makeConst(fromp->విలువ().అంకె().గమ్యం_UQuad()),
                        (fromp != గమ్యం_Statep) ? buildCoverIncStmtp(fromp->label(), గమ్యం_State_Label)
                                            : శూన్యసూచిక});
                }
            }
        }
        కొరకు (స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి fromp : m_స్థితులు) {
            ఒకవేళ (handledSoureces.కనుగొను(fromp) != handledSoureces.చివర()) కొనసాగు;
            arcCasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{
                m_flp, makeConst(fromp->విలువ().అంకె().గమ్యం_UQuad()),
                (fromp != గమ్యం_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->బహిర్గతం_అంచులు().ఖాళీలు()))
                    ? buildCoverIncStmtp(fromp->label(), గమ్యం_State_Label, "artificial")
                    : శూన్యసూచిక});
        }
        ఒకవేళ (!m_isComplete) {
            ఒకవేళ (considerUndefs) {
                కొరకు (స్థిరరాశి uint64_t from : m_undefined_స్థితులు) {
                    స్థిరరాశి std::string& label = quad_గమ్యం_బిట్టు_Stirng_Label(from);
                    arcCasep->కూడు_Itemsp(
                        కొత్త Ast_Case_అంశం{m_flp, makeConst(from),
                                        (label != గమ్యం_State_Label)
                                            ? buildCoverIncStmtp(label, గమ్యం_State_Label, "artificial")
                                            : శూన్యసూచిక});
                }
            } లేకపోతే {
                arcCasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{m_flp, శూన్యసూచిక, శూన్యసూచిక});
            }
        }
        తిరిగి arcCasep;
    }
    Ast_కణుపు_Stmt* buildUnxpandedTransitionHandler(స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి గమ్యం_Statep) {
        AstCase* స్థిరరాశి arcCasep = కొత్త AstCase{
            m_flp, VCaseType::CT_CASE, కొత్త AstVarRef{m_flp, m_prevVscp, VAccess::READ}, శూన్యసూచిక};
        కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : గమ్యం_Statep->లోపల_అంచులు()) {
            స్థిరరాశి Fsm_Arc_అంచు* స్థిరరాశి arcp = అంచు.as<Fsm_Arc_అంచు>();
            UASSERT_OBJ(!arcp->isReset(), m_లేఖాచిత్రం.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_లేఖాచిత్రం.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి fromp = arcp->fromp()->as<Fsm_State_శీర్షం>();
            ఒకవేళ (fromp != గమ్యం_Statep) {
                arcCasep->కూడు_Itemsp(
                    కొత్త Ast_Case_అంశం{m_flp, makeConst(fromp->విలువ().అంకె().గమ్యం_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), గమ్యం_Statep->label())});
            }
        }
        arcCasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{m_flp, శూన్యసూచిక, శూన్యసూచిక});
        తిరిగి arcCasep;
    }

బహిరంగ:
    FsmCoverIncBuilder(స్థిరరాశి Fsm_లేఖాచిత్రం& లేఖాచిత్రం, AstVarScope* స్థిరరాశి prevVscp)
        : m_లేఖాచిత్రం{లేఖాచిత్రం}
        , m_prevVscp{prevVscp}
        , m_flp{లేఖాచిత్రం.fileline()}
        , m_modPrettyName{లేఖాచిత్రం.scopep()->modp()->prettyName()} {
        init_Existing_స్థితులు();
        AstVarScope* స్థిరరాశి sampleVscp = లేఖాచిత్రం.sampleVarScopep();
        స్థిరరాశి VFsmExpandType expand
            = లేఖాచిత్రం.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? లేఖాచిత్రం.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            స్థిరరాశి కొలత_t arc_లెక్క_With_Expansion = get_Arc_లెక్క_With_Expansion(expand);
            ఒకవేళ (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_లెక్క_With_Expansion == 0ULL
                            || arc_లెక్క_With_Expansion
                                   > static_cast<కొలత_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_లెక్క_With_Expansion);
                తిరిగి;
            }
        }

        AstCase* స్థిరరాశి arcCasep
            = కొత్త AstCase{m_flp, VCaseType::CT_CASE,
                          కొత్త AstVarRef{m_flp, లేఖాచిత్రం.sampleVarScopep(), VAccess::READ}, శూన్యసూచిక};
        AstCase* స్థిరరాశి initialStateCasep
            = కొత్త AstCase{m_flp, VCaseType::CT_CASE,
                          కొత్త AstVarRef{m_flp, లేఖాచిత్రం.sampleVarScopep(), VAccess::READ}, శూన్యసూచిక};

        స్థిరరాశి తార్కిక expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_సమితి<స్థిరరాశి Fsm_State_శీర్షం*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_సమితి<స్థిరరాశి Fsm_State_శీర్షం*>
            resetTargets;  // States that have edge incoming from reset vertex
        ఒకవేళ (!expandAll) {
            ఒకవేళ (m_defaultVertexp) {
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : m_defaultVertexp->బహిర్గతం_అంచులు()) {
                    defaultTargets.emplace(అంచు.శిఖరం()->as<Fsm_State_శీర్షం>());
                }
            }
            ఒకవేళ (m_resetVertexp) {
                కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : m_resetVertexp->బహిర్గతం_అంచులు()) {
                    resetTargets.emplace(అంచు.శిఖరం()->as<Fsm_State_శీర్షం>());
                }
            }
        }
        స్థిరరాశి తార్కిక is_నిండిన_Expansion = expand == VFsmExpandType::నిండిన;
        ఒకవేళ (is_నిండిన_Expansion) init_Undefined_స్థితులు();
        కొరకు (స్థిరరాశి Fsm_State_శీర్షం* స్థిరరాశి statep : m_స్థితులు) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* స్థిరరాశి declp = buildStateCoverDecl(statep->label());
            initialStateCasep->కూడు_Itemsp(
                కొత్త Ast_Case_అంశం{m_flp, makeConst(statep->విలువ().అంకె().గమ్యం_UQuad()),
                                కొత్త AstCoverInc{m_flp, declp, నిజం}});
            స్థిరరాశి తార్కిక expandAllOrReset
                = expandAll || (resetTargets.కనుగొను(statep) != resetTargets.చివర());
            స్థిరరాశి తార్కిక defaultExpantion
                = !expandAllOrReset && (defaultTargets.కనుగొను(statep) != defaultTargets.చివర());
            arcCasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{
                m_flp, makeConst(statep->విలువ().అంకె().గమ్యం_UQuad()),
                Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు_Stmt>(
                    కొత్త AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_నిండిన_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        ఒకవేళ (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            ఒకవేళ (is_నిండిన_Expansion) {
                కొరకు (స్థిరరాశి uint64_t విలువ : m_undefined_స్థితులు) {
                    స్థిరరాశి std::string& label = quad_గమ్యం_బిట్టు_Stirng_Label(విలువ);
                    AstCoverOtherDecl* స్థిరరాశి declp = buildStateCoverDecl(label);
                    initialStateCasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{
                        m_flp, makeConst(విలువ), కొత్త AstCoverInc{m_flp, declp, నిజం}});
                    arcCasep->కూడు_Itemsp(
                        కొత్త Ast_Case_అంశం{m_flp, makeConst(విలువ),
                                        Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు_Stmt>(
                                            కొత్త AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, నిజం, అబద్ధం))});
                }
            } లేకపోతే {
                // No expansion
                arcCasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{m_flp, శూన్యసూచిక, శూన్యసూచిక});
                initialStateCasep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{m_flp, శూన్యసూచిక, శూన్యసూచిక});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = అప్రమేయ;
    Ast_కణుపు_Cover_Decl* getCoverDeclsp() స్థిరరాశి { తిరిగి m_declsp; }
    Ast_కణుపు_Stmt* getCoverIncp() స్థిరరాశి { తిరిగి m_coverIncp; }
    Ast_కణుపు_Stmt* getInitialCoverIncp() స్థిరరాశి { తిరిగి m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
తరగతి Fsm_క్రింది_Visitor final {
    // STATE - across all visitors
    స్థిరరాశి FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    స్థిర Ast_Sen_వృక్షం* build_Sen_వృక్షం(File_పంక్తి* flp, స్థిరరాశి std::సదిశ<FsmSenDesc>& senses) {
        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = కొత్త Ast_Sen_వృక్షం{flp, శూన్యసూచిక};
        కొరకు (స్థిరరాశి FsmSenDesc& sense : senses) {
            Ast_Sen_అంశం* స్థిరరాశి senItemp
                = కొత్త Ast_Sen_అంశం{flp, VEdgeType{sense.అంచు_Type},
                                 కొత్త AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->కూడు_Sensesp(senItemp);
        }
        తిరిగి sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    శూన్య buildOne(స్థిరరాశి Fsm_లేఖాచిత్రం& లేఖాచిత్రం) {
        UINFO(1, "buildOne lowering FSM " << లేఖాచిత్రం.stateVarName()
                                          << " vertices=" << లేఖాచిత్రం.vertices().కొలత() << పంక్తి_ముగింపు);
        AstAlways* స్థిరరాశి alwaysp = లేఖాచిత్రం.stateAlwaysp();
        AstScope* స్థిరరాశి scopep = లేఖాచిత్రం.scopep();
        AstVarScope* స్థిరరాశి stateVscp = లేఖాచిత్రం.stateVarScopep();
        AstVarScope* స్థిరరాశి sampleVscp = లేఖాచిత్రం.sampleVarScopep();
        File_పంక్తి* స్థిరరాశి flp = లేఖాచిత్రం.fileline();
        Ast_కణుపు_Module* స్థిరరాశి modp = scopep->modp();
        AstVarScope* స్థిరరాశి prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(నిజం);
        sampleVscp->optimizeLifePost(నిజం);
        prevVscp->optimizeLifePost(నిజం);

        Ast_చురుకైన* స్థిరరాశి initActivep
            = కొత్త Ast_చురుకైన{flp, "fsm-coverage-init",
                            కొత్త Ast_Sen_వృక్షం{flp, కొత్త Ast_Sen_అంశం{flp, Ast_Sen_అంశం::Initial{}}}};
        initActivep->sen_వృక్షం_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->కూడు_Stmtsp(కొత్త AstInitialStatic{
            flp, కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, prevVscp, VAccess::WRITE},
                               కొత్త AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->కూడు_Blocksp(initActivep);

        AstAlwaysPost* స్థిరరాశి covPostp = కొత్త AstAlwaysPost{flp};
        తార్కిక updatePrevAfterPost = అబద్ధం;
        ఒకవేళ (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 కొత్త AstVarRef{flp, sampleVscp, VAccess::READ}});
        } లేకపోతే {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = నిజం;
            prevVscp->varp()->సమితి_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            స్థిరరాశి FsmCoverIncBuilder coverIncBuilder{లేఖాచిత్రం, prevVscp};
            modp->కూడు_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->కూడు_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->కూడు_Stmtsp(
                కొత్త AstIf{flp,
                          కొత్త AstNeq{flp, కొత్త AstVarRef{flp, prevVscp, VAccess::READ},
                                     కొత్త AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        ఒకవేళ (updatePrevAfterPost) {
            covPostp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              కొత్త AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_వృక్షం* స్థిరరాశి sentreep = build_Sen_వృక్షం(flp, లేఖాచిత్రం.senses());
        Ast_చురుకైన* స్థిరరాశి activep = కొత్త Ast_చురుకైన{flp, "fsm-coverage", sentreep};
        activep->sen_వృక్షం_Storep(sentreep);
        scopep->కూడు_Blocksp(activep);
        activep->కూడు_Stmtsp(covPostp);
    }

బహిరంగ:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    స్పష్టమైన Fsm_క్రింది_Visitor(స్థిరరాశి FsmState& state)
        : m_state{state} {
        కొరకు (స్థిరరాశి DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

శూన్య V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) {
        కొలత_t సూచీ = 0;
        కొరకు (స్థిరరాశి DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_ట్యాగ్(సూచీ++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_క్రింది_Visitor క్రింది{state}; }
    V3Global::dump_Check_Global_వృక్షం("fsm-detect", 0, dump_వృక్షం_Either_Level() >= 3);
}
