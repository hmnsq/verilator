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

#অন্তর্ভাব "V3PchAstNoMT.h"

#অন্তর্ভাব "V3FsmDetect.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Control.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3SharedTmps.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cctype>
#অন্তর্ভাব <map>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
বর্গ Fsm_State_মান final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_চাবি;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_সংখ্যা m_অঙ্ক;  // Original folded value, preserving width for lowered comparisons

    স্থির string make_চাবি(ধ্রুব V3_সংখ্যা& অঙ্ক) {
        V3_সংখ্যা চাবি_অঙ্ক = অঙ্ক;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        চাবি_অঙ্ক.isSigned(অসত্য);
        প্রত্যাগম cvtToStr(চাবি_অঙ্ক.width()) + ":" + চাবি_অঙ্ক.ascii(সত্য, সত্য);
    }

সার্বজনিক:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_মান()
        : m_চাবি{"1:1'h0"}
        , m_অঙ্ক{static_cast<Ast_গ্রন্থি*>(nullptr), 1, 0} {}
    explicit Fsm_State_মান(ধ্রুব V3_সংখ্যা& অঙ্ক)
        : m_চাবি{make_চাবি(অঙ্ক)}
        , m_অঙ্ক{অঙ্ক} {}

    ধ্রুব string& চাবি() ধ্রুব { প্রত্যাগম m_চাবি; }
    ধ্রুব V3_সংখ্যা& অঙ্ক() ধ্রুব { প্রত্যাগম m_অঙ্ক; }
    string ascii() ধ্রুব { প্রত্যাগম m_অঙ্ক.ascii(সত্য, সত্য); }
    string warnText() ধ্রুব {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        যদি (m_অঙ্ক.width() <= 32) প্রত্যাগম cvtToStr(m_অঙ্ক.toUInt());
        প্রত্যাগম ascii();
    }

    তার্কিক operator==(ধ্রুব Fsm_State_মান& rhs) ধ্রুব { প্রত্যাগম m_চাবি == rhs.m_চাবি; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
সংরচনা Fsm_State_মান_Hash final {
    size_t operator()(ধ্রুব Fsm_State_মান& মান) ধ্রুব {
        প্রত্যাগম std::hash<string>{}(মান.চাবি());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
সংরচনা FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
সংরচনা FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    তার্কিক active_নিচের = অসত্য;
};

বর্গ FsmResetArcDesc final {
    Fsm_State_মান m_to_মান;  // Encoded reset target state.
    Ast_গ্রন্থি* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_গ্রন্থি_রাশিমালা* m_valuep = nullptr;  // Expression that provided the reset value.

সার্বজনিক:
    FsmResetArcDesc() = স্বভাব;
    FsmResetArcDesc(Fsm_State_মান to_মান, Ast_গ্রন্থি_Assign* nodep)
        : m_to_মান{to_মান}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_মান to_মান, Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* valuep)
        : m_to_মান{to_মান}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_মান to_মান() ধ্রুব { প্রত্যাগম m_to_মান; }
    Ast_গ্রন্থি* nodep() ধ্রুব { প্রত্যাগম m_nodep; }
    Ast_গ্রন্থি_রাশিমালা* valuep() ধ্রুব { প্রত্যাগম m_valuep; }
};

সংরচনা FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    তার্কিক has_Rst_Active_নিচের = অসত্য;
    তার্কিক rst_Active_নিচের = অসত্য;
};

স্থির তার্কিক fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ধ্রুব string& portName,
                                                  তার্কিক& active_নিচের) {
    তার্কিক matched = অসত্য;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_গ্রন্থি_Var_Ref* ধ্রুব vrefp = itemp->varrefp();
        যদি (!vrefp) প্রত্যাগম;
        যদি (vrefp->varp()->name() != portName) প্রত্যাগম;
        active_নিচের = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = সত্য;
    });
    প্রত্যাগম matched;
}

স্থির ধ্রুব V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_গ্রন্থি_Module* ধ্রুব modp = cellp->modp();
    ধ্রুব string origName = modp->origName();
    যদি (ধ্রুব V3Control::FsmRegisterWrapper* ধ্রুব descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        প্রত্যাগম descp;
    }
    প্রত্যাগম V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

স্থির FsmWrapperRoles rolesFromDesc(ধ্রুব V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_মান;
    প্রত্যাগম roles;
}

বর্গ FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    তার্কিক m_hasResetCond = অসত্য;  // Whether the FSM had a modeled reset predicate.
    তার্কিক m_inclCond = অসত্য;  // Whether conditional/default arcs are kept explicitly.
    File_সারি* m_flp = nullptr;  // Representative source location.

সার্বজনিক:
    AstScope* scopep() ধ্রুব { প্রত্যাগম m_scopep; }
    শূন্য scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ধ্রুব { প্রত্যাগম m_alwaysp; }
    শূন্য alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ধ্রুব { প্রত্যাগম m_stateVscp; }
    শূন্য stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ধ্রুব { প্রত্যাগম m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    শূন্য sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() ধ্রুব { প্রত্যাগম m_nextVscp; }
    শূন্য nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    ধ্রুব std::vector<FsmSenDesc>& senses() ধ্রুব { প্রত্যাগম m_senses; }
    std::vector<FsmSenDesc>& senses() { প্রত্যাগম m_senses; }
    ধ্রুব FsmResetCondDesc& resetCond() ধ্রুব { প্রত্যাগম m_resetCond; }
    FsmResetCondDesc& resetCond() { প্রত্যাগম m_resetCond; }
    ধ্রুব std::vector<FsmResetArcDesc>& resetArcs() ধ্রুব { প্রত্যাগম m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { প্রত্যাগম m_resetArcs; }
    তার্কিক hasResetCond() ধ্রুব { প্রত্যাগম m_hasResetCond; }
    শূন্য hasResetCond(তার্কিক flag) { m_hasResetCond = flag; }
    তার্কিক inclCond() ধ্রুব { প্রত্যাগম m_inclCond; }
    শূন্য inclCond(তার্কিক flag) { m_inclCond = flag; }
    File_সারি* fileline() ধ্রুব { প্রত্যাগম m_flp; }
    শূন্য fileline(File_সারি* flp) { m_flp = flp; }
};

বর্গ FsmComboAlways final {
    AstScope* ধ্রুব m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* ধ্রুব m_alwaysp = nullptr;  // Candidate transition process.

সার্বজনিক:
    FsmComboAlways() = স্বভাব;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ধ্রুব { প্রত্যাগম m_scopep; }
    AstAlways* alwaysp() ধ্রুব { প্রত্যাগম m_alwaysp; }
};

বর্গ FsmGraph;

বর্গ FsmVertex VL_NOT_FINAL : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

সার্বজনিক:
    গণনা বর্গ Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

গুপ্ত:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_মান m_মান;  // Encoded state value for real state vertices.

রক্ষিত:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_মান মান) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_মান{মান} {}
    ~FsmVertex() override = স্বভাব;

সার্বজনিক:
    Kind kind() ধ্রুব { প্রত্যাগম m_kind; }
    তার্কিক isState() ধ্রুব { প্রত্যাগম m_kind == Kind::STATE; }
    তার্কিক isResetAny() ধ্রুব { প্রত্যাগম m_kind == Kind::RESET_ANY; }
    তার্কিক isDefaultAny() ধ্রুব { প্রত্যাগম m_kind == Kind::DEFAULT_ANY; }
    ধ্রুব string& label() ধ্রুব { প্রত্যাগম m_label; }
    ধ্রুব Fsm_State_মান& মান() ধ্রুব { প্রত্যাগম m_মান; }

    string name() ধ্রুব override VL_MT_SAFE { প্রত্যাগম m_label + "=" + m_মান.ascii(); }
};

বর্গ FsmStateVertex final : সার্বজনিক FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

সার্বজনিক:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_মান মান) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, মান} {}
    ~FsmStateVertex() override = স্বভাব;

    string dotColor() ধ্রুব override { প্রত্যাগম "lightblue"; }
    string dotShape() ধ্রুব override { প্রত্যাগম "ellipse"; }
};

বর্গ FsmPseudoVertex final : সার্বজনিক FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

সার্বজনিক:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_মান{}} {}
    ~FsmPseudoVertex() override = স্বভাব;

    string name() ধ্রুব override VL_MT_SAFE { প্রত্যাগম label(); }
    string dotColor() ধ্রুব override { প্রত্যাগম isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ধ্রুব override { প্রত্যাগম "diamond"; }
};

বর্গ FsmArcEdge final : সার্বজনিক V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    তার্কিক m_isReset = অসত্য;  // Arc originates from the synthetic reset source.
    তার্কিক m_isCond = অসত্য;  // Arc came from a conditional next-state split.
    তার্কিক m_isDefault = অসত্য;  // Arc represents a case default source.
    File_সারি* m_flp = nullptr;  // Source location for emitted coverage metadata.

সার্বজনিক:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* শীর্ষ, তার্কিক isReset, তার্কিক isCond,
               তার্কিক isDefault, File_সারি* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, শীর্ষ, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = স্বভাব;

    তার্কিক isReset() ধ্রুব { প্রত্যাগম m_isReset; }
    তার্কিক isCond() ধ্রুব { প্রত্যাগম m_isCond; }
    তার্কিক isDefault() ধ্রুব { প্রত্যাগম m_isDefault; }
    File_সারি* fileline() ধ্রুব { প্রত্যাগম m_flp; }

    string dotLabel() ধ্রুব override {
        যদি (m_isReset) প্রত্যাগম "reset";
        যদি (m_isDefault) প্রত্যাগম "default";
        যদি (m_isCond) প্রত্যাগম "cond";
        প্রত্যাগম "";
    }
    string dotColor() ধ্রুব override {
        যদি (m_isReset) প্রত্যাগম "darkgreen";
        যদি (m_isDefault) প্রত্যাগম "orange";
        যদি (m_isCond) প্রত্যাগম "blue";
        প্রত্যাগম "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
বর্গ FsmGraph final : সার্বজনিক V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    তার্কিক m_hasResetCond = অসত্য;  // Whether the detected FSM had a reset branch.
    তার্কিক m_inclCond = অসত্য;  // Whether conditional arcs should be kept explicitly.
    File_সারি* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_মান, FsmStateVertex*, Fsm_State_মান_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

সার্বজনিক:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{নূতন FsmPseudoVertex{এতত্, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{নূতন FsmPseudoVertex{এতত্, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ধ্রুব { প্রত্যাগম m_scopep; }
    শূন্য scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ধ্রুব { প্রত্যাগম m_stateAlwaysp; }
    শূন্য stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ধ্রুব string& stateVarName() ধ্রুব { প্রত্যাগম m_stateVarName; }
    শূন্য stateVarName(ধ্রুব string& name) { m_stateVarName = name; }
    ধ্রুব string& stateVarInternalName() ধ্রুব { প্রত্যাগম m_stateVarInternalName; }
    শূন্য stateVarInternalName(ধ্রুব string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() ধ্রুব { প্রত্যাগম m_stateVarScopep; }
    শূন্য stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ধ্রুব {
        প্রত্যাগম m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    শূন্য sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ধ্রুব std::vector<FsmSenDesc>& senses() ধ্রুব { প্রত্যাগম m_senses; }
    std::vector<FsmSenDesc>& senses() { প্রত্যাগম m_senses; }
    ধ্রুব FsmResetCondDesc& resetCond() ধ্রুব { প্রত্যাগম m_resetCond; }
    FsmResetCondDesc& resetCond() { প্রত্যাগম m_resetCond; }
    তার্কিক hasResetCond() ধ্রুব { প্রত্যাগম m_hasResetCond; }
    শূন্য hasResetCond(তার্কিক flag) { m_hasResetCond = flag; }
    তার্কিক inclCond() ধ্রুব { প্রত্যাগম m_inclCond; }
    শূন্য inclCond(তার্কিক flag) { m_inclCond = flag; }
    File_সারি* fileline() ধ্রুব { প্রত্যাগম m_flp; }
    শূন্য fileline(File_সারি* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_মান মান) VL_MT_DISABLED {
        FsmStateVertex* ধ্রুব vertexp = নূতন FsmStateVertex{এতত্, label, মান};
        m_stateVertices.emplace(মান, vertexp);
        প্রত্যাগম vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { প্রত্যাগম m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { প্রত্যাগম m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_মান from_মান, Fsm_State_মান to_মান, তার্কিক isReset, তার্কিক isCond,
                       তার্কিক isDefault, File_সারি* flp) VL_MT_DISABLED {
        FsmStateVertex* ধ্রুব শীর্ষ = m_stateVertices.at(to_মান);
        FsmVertex* fromp = nullptr;
        যদি (isReset) {
            fromp = resetAnyVertex();
        } অন্যথা যদি (isDefault) {
            fromp = defaultAnyVertex();
        } অন্যথা {
            fromp = m_stateVertices.at(from_মান);
        }
        প্রত্যাগম নূতন FsmArcEdge{এতত্, fromp, শীর্ষ, isReset, isCond, isDefault, flp};
    }

    string name() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) ধ্রুব {
        string tag = stateVarInternalName();
        প্রতি (বর্ণ& ch : tag) {
            যদি (!std::isalnum(static_cast<unsigned বর্ণ>(ch))) ch = '_';
        }
        প্রত্যাগম "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

সংরচনা DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

সংরচনা FsmCaseCandidate final {
    Ast_গ্রন্থি* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
সংরচনা FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;  // Compared constant value expression
    Fsm_State_মান মান;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
সংরচনা FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_গ্রন্থি* stmtsp = nullptr;  // Branch body
    Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;  // Source state value expression
    Fsm_State_মান from_মান;  // Encoded source state value
    তার্কিক has_শীর্ষ_Guard = অসত্য;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
সংরচনা FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_গ্রন্থি* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<ধ্রুব AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<ধ্রুব AstCell*, FsmCellPortMap>;

সংরচনা StateConstLabel final {
    string text;
    তার্কিক fromParam = অসত্য;
    size_t stateIndex = 0;
};

সংরচনা FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_মান>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_মান, StateConstLabel, Fsm_State_মান_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    তার্কিক enumBacked = অসত্য;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
বর্গ FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<ধ্রুব AstVarScope*, size_t> m_fsmIndex;

সার্বজনিক:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ধ্রুব std::map<ধ্রুব AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        যদি (it != m_fsmIndex.end()) প্রত্যাগম m_fsms.at(it->second);
        ধ্রুব size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        প্রত্যাগম m_fsms.back();
    }
    ধ্রুব std::vector<DetectedFsm>& fsms() ধ্রুব { প্রত্যাগম m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
বর্গ FsmDetectVisitor final : সার্বজনিক VNVisitor {
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
    std::unordered_map<ধ্রুব AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<ধ্রুব AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    স্থির Ast_গ্রন্থি_DType* unwrapEnumCandidate(Ast_গ্রন্থি_DType* dtypep) {
        প্রত্যাগম dtypep->skipRefToEnump();
    }

    স্থির string candidateConflictContext(Ast_গ্রন্থি* laterNodep,
                                           ধ্রুব FsmCaseCandidate& firstCand) {
        প্রত্যাগম '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    স্থির তার্কিক rejectFsmWrapperCell(AstCell* cellp, ধ্রুব string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        প্রত্যাগম অসত্য;
    }

    স্থির তার্কিক simple_Param_State_মান(AstCell* cellp, ধ্রুব string& name, Fsm_State_মান& মান,
                                      Ast_গ্রন্থি_রাশিমালা*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        প্রতি (Ast_গ্রন্থি* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ধ্রুব varp = VN_CAST(stmtp, Var);
            যদি (!varp || !varp->isParam() || varp->name() != name) অনুবর্ত;
            valuepr = VN_AS(varp->valuep(), গ্রন্থি_রাশিমালা);
            প্রত্যাগম const_মান_Status(valuepr, মান) == Const_মান_Status::OK;
        }
        প্রত্যাগম অসত্য;
    }

    স্থির তার্কিক childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        যদি (!vscp->varp()->isIO()) প্রত্যাগম অসত্য;
        AstScope* ধ্রুব scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        যদি (scopep->aboveScopep() != parentScopep) প্রত্যাগম অসত্য;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        প্রত্যাগম সত্য;
    }

    স্থির AstVarScope* simpleAssignVarScope(Ast_গ্রন্থি_রাশিমালা* exprp) {
        AstVarRef* ধ্রুব vrefp = VN_CAST(exprp, VarRef);
        প্রত্যাগম vrefp ? vrefp->varScopep() : nullptr;
    }

    শূন্য addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        ধ্রুব std::pair<AstScope*, AstCell*> item{scopep, cellp};
        যদি (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            প্রত্যাগম;
        }
        m_wrapperCells.emplace_back(item);
    }

    শূন্য collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ধ্রুব lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* ধ্রুব rhsVscp = simpleAssignVarScope(nodep->rhsp());
        যদি (!lhsVscp || !rhsVscp) প্রত্যাগম;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        যদি (childPortInScope(lhsVscp, m_scopep, cellp)) {
            যদি (!fsmRegisterWrapperDesc(cellp)) প্রত্যাগম;
            UASSERT_OBJ(lhsVscp->varp()->is_প্রবেশ(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } অন্যথা যদি (childPortInScope(rhsVscp, m_scopep, cellp)) {
            যদি (!fsmRegisterWrapperDesc(cellp)) প্রত্যাগম;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ধ্রুব string& portName) ধ্রুব {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ধ্রুব FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        ধ্রুব FsmCellPortMap::const_iterator portIt = ports.find(portName);
        প্রত্যাগম portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, ধ্রুব string& portName) ধ্রুব {
        ধ্রুব FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        ধ্রুব FsmCellPortMap::const_iterator portIt = ports.find(portName);
        প্রত্যাগম portIt == ports.end() ? nullptr : portIt->second;
    }

    বর্গ RegisterAlwaysAnalyzer final {
        AstScope* ধ্রুব m_scopep;  // Scope containing the always block(s) being analyzed

    সার্বজনিক:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_গ্রন্থি_রাশিমালা*>>
        oneBlockCandidates(AstAlways* alwaysp) ধ্রুব {
            std::vector<std::pair<AstCase*, Ast_গ্রন্থি_রাশিমালা*>> candidates;
            Ast_গ্রন্থি* ধ্রুব stmtsp = alwaysp->stmtsp();
            যদি (AstIf* ধ্রুব firstIfp = VN_CAST(stmtsp, If)) {
                যদি (AstCase* ধ্রুব casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            প্রতি (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                যদি (AstCase* ধ্রুব casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            প্রত্যাগম candidates;
        }

        std::vector<std::pair<AstIf*, Ast_গ্রন্থি_রাশিমালা*>>
        oneBlockIfCandidates(AstAlways* alwaysp) ধ্রুব {
            std::vector<std::pair<AstIf*, Ast_গ্রন্থি_রাশিমালা*>> candidates;
            Ast_গ্রন্থি* ধ্রুব stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            যদি (AstIf* ধ্রুব firstIfp = VN_CAST(stmtsp, If)) {
                যদি (AstIf* ধ্রুব chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            প্রতি (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                যদি (AstIf* ধ্রুব ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            প্রত্যাগম candidates;
        }

        তার্কিক matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) ধ্রুব {
            প্রত্যাগম FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        শূন্য buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_গ্রন্থি_রাশিমালা* resetCondp,
                                    FsmRegisterCandidate& reg) ধ্রুব {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_বৃক্ষ(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ধ্রুব firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            যদি (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                ধ্রুব ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                যদি (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } অন্যথা যদি (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_মান reset_মান;
                    Ast_গ্রন্থি* ধ্রুব thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_গ্রন্থি_Assign* ধ্রুব resetAssp = FsmDetectVisitor::direct_Const_State_Assign_গ্রন্থি(
                        thenNodep, resetStateVscp, reset_মান);
                    যদি (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_মান, resetAssp);
                    }
                } অন্যথা যদি (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    তার্কিক matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) ধ্রুব {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* ধ্রুব nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* ধ্রুব stateVscp = roleVarScope(cellp, roles.qPort);
        যদি (!nextVscp || !stateVscp) {
            প্রত্যাগম rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ধ্রুব clkVscp = roleVarScope(cellp, roles.clkPort);
        যদি (!clkVscp) {
            প্রত্যাগম rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ঠেলো_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        যদি (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        যদি (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            তার্কিক inferred_Active_নিচের = অসত্য;
            যদি (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_নিচের)) {
                roles.has_Rst_Active_নিচের = সত্য;
                roles.rst_Active_নিচের = inferred_Active_নিচের;
            }
        }

        Ast_গ্রন্থি_রাশিমালা* resetValuep = nullptr;
        Fsm_State_মান reset_মান;
        ধ্রুব তার্কিক has_Reset_মান
            = !roles.rstValParam.empty()
              && simple_Param_State_মান(cellp, roles.rstValParam, reset_মান, resetValuep);
        যদি (resetVscp && roles.has_Rst_Active_নিচের && has_Reset_মান) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_নিচের ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ঠেলো_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_নিচের = roles.rst_Active_নিচের;
            cand.hasResetCond(সত্য);
            cand.resetArcs().emplace_back(reset_মান, cellp, resetValuep);
        } অন্যথা যদি (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            যদি (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } অন্যথা যদি (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } অন্যথা যদি (!roles.has_Rst_Active_নিচের) {
                reason = "reset polarity could not be inferred from the wrapper";
            } অন্যথা যদি (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } অন্যথা {
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
        প্রত্যাগম সত্য;
    }

    বর্গ ComboAlwaysAnalyzer final {
    সার্বজনিক:
        সংরচনা ComboMatch final {
            ধ্রুব FsmRegisterCandidate* matchedp = nullptr;
            Ast_গ্রন্থি* warnNodep = nullptr;
        };

    গুপ্ত:
        ধ্রুব std::vector<FsmRegisterCandidate>& m_registerCandidates;

    সার্বজনিক:
        explicit ComboAlwaysAnalyzer(ধ্রুব std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_গ্রন্থি* stmtsp, AstCase* casep) ধ্রুব {
            ComboMatch match;
            AstVarRef* ধ্রুব selp = VN_CAST(casep->exprp(), VarRef);
            যদি (!selp) প্রত্যাগম match;
            প্রতি (ধ্রুব FsmRegisterCandidate& reg : m_registerCandidates) {
                যদি (selp->varScopep() == reg.nextVscp()) {
                    যদি (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        অনুবর্ত;
                    }
                } অন্যথা যদি (selp->varScopep() != reg.stateVscp()) {
                    অনুবর্ত;
                }
                Ast_গ্রন্থি* ধ্রুব warnNodep = FsmDetectVisitor::case_Supported_Transition_গ্রন্থি(
                    casep, reg.nextVscp(), reg.inclCond());
                যদি (!warnNodep) অনুবর্ত;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            প্রত্যাগম match;
        }

        ComboMatch matchIfChain(Ast_গ্রন্থি* stmtsp, ধ্রুব FsmIfChainCandidate& chain) ধ্রুব {
            ComboMatch match;
            প্রতি (ধ্রুব FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                যদি (chain.compareVscp == reg.nextVscp()) {
                    যদি (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        অনুবর্ত;
                    }
                } অন্যথা যদি (chain.compareVscp != reg.stateVscp()) {
                    অনুবর্ত;
                }
                Ast_গ্রন্থি* ধ্রুব warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_গ্রন্থি(chain, reg.nextVscp());
                যদি (!warnNodep) অনুবর্ত;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            প্রত্যাগম match;
        }

        তার্কিক shouldWarnUnsupported(Ast_গ্রন্থি* stmtsp, AstCase* casep) ধ্রুব {
            ধ্রুব AstVarRef* ধ্রুব selp = VN_CAST(casep->exprp(), VarRef);
            যদি (!selp) প্রত্যাগম অসত্য;

            প্রত্যাগম std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](ধ্রুব FsmRegisterCandidate& reg) -> তার্কিক {
                    ধ্রুব তার্কিক matchesNext = selp->varScopep() == reg.nextVscp();
                    ধ্রুব তার্কিক matchesState = selp->varScopep() == reg.stateVscp();

                    যদি (!matchesNext && !matchesState) প্রত্যাগম অসত্য;
                    যদি (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        প্রত্যাগম অসত্য;
                    }
                    প্রত্যাগম FsmDetectVisitor::case_Supported_Transition_গ্রন্থি(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    স্থির তার্কিক isSimpleResetCond(Ast_গ্রন্থি_রাশিমালা* condp) { প্রত্যাগম VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    স্থির FsmResetCondDesc describeResetCond(Ast_গ্রন্থি_রাশিমালা* condp) {
        FsmResetCondDesc desc;
        যদি (AstVarRef* ধ্রুব vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        প্রত্যাগম desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    স্থির std::vector<FsmSenDesc> describe_Sen_বৃক্ষ(Ast_Sen_বৃক্ষ* sentreep) {
        std::vector<FsmSenDesc> senses;
        প্রতি (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_গ্রন্থি_Var_Ref* ধ্রুব vrefp = itemp->varrefp();
            যদি (!vrefp) অনুবর্ত;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ঠেলো_back(desc);
        }
        প্রত্যাগম senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    স্থির তার্কিক isIgnorableStmt(Ast_গ্রন্থি* nodep) { প্রত্যাগম VN_IS(nodep, CoverInc); }

    স্থির Ast_গ্রন্থি* skipLeadingIgnorableStmt(Ast_গ্রন্থি* nodep) {
        যাবত্ (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        প্রত্যাগম nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    স্থির Ast_গ্রন্থি* singleMeaningfulStmt(Ast_গ্রন্থি* stmtp) {
        Ast_গ্রন্থি* resultp = nullptr;
        প্রতি (Ast_গ্রন্থি* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            যদি (isIgnorableStmt(nodep)) অনুবর্ত;
            যদি (resultp) প্রত্যাগম nullptr;
            resultp = nodep;
        }
        প্রত্যাগম resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    স্থির Ast_গ্রন্থি* singleMeaningfulBranch(Ast_গ্রন্থি* branchp) {
        যদি (!branchp) প্রত্যাগম nullptr;
        প্রত্যাগম branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    স্থির তার্কিক isPlainComboSentree(ধ্রুব Ast_Sen_বৃক্ষ* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        প্রতি (ধ্রুব AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            যদি (senp->edgeType() == VEdgeType::ET_CHANGED) অনুবর্ত;
            প্রত্যাগম অসত্য;
        }
        প্রত্যাগম সত্য;
    }

    শূন্য warnUnsupportedComboAlways(ধ্রুব FsmComboAlways& combo) {
        ধ্রুব ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_গ্রন্থি* ধ্রুব stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        তার্কিক warned = অসত্য;
        প্রতি (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ধ্রুব casep = VN_CAST(nodep, Case);
            যদি (!casep) অনুবর্ত;
            যদি (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = সত্য;
            }
            যদি (warned) ভঙ্গ;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    স্থির Ast_গ্রন্থি_Assign* directStateAssign(Ast_গ্রন্থি* stmtp, AstVarScope* stateVscp) {
        Ast_গ্রন্থি* ধ্রুব nodep = singleMeaningfulBranch(stmtp);
        যদি (!nodep) প্রত্যাগম nullptr;
        Ast_গ্রন্থি_Assign* ধ্রুব assp = VN_CAST(nodep, গ্রন্থি_Assign);
        যদি (!assp) প্রত্যাগম nullptr;
        AstVarRef* ধ্রুব vrefp = VN_CAST(assp->lhsp(), VarRef);
        যদি (!vrefp || vrefp->varScopep() != stateVscp) প্রত্যাগম nullptr;
        প্রত্যাগম assp;
    }

    স্থির Ast_গ্রন্থি_Assign* গ্রন্থি_State_Var_Assign(Ast_গ্রন্থি* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_গ্রন্থি_Assign* ধ্রুব assp = VN_CAST(nodep, গ্রন্থি_Assign);
        যদি (!assp) প্রত্যাগম nullptr;
        AstVarRef* ধ্রুব lhsp = VN_CAST(assp->lhsp()->baseFromp(সত্য), VarRef);
        AstVarRef* ধ্রুব rhsp = VN_CAST(assp->rhsp(), VarRef);
        যদি (!rhsp || !lhsp) প্রত্যাগম nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        প্রত্যাগম assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    স্থির Ast_গ্রন্থি_Assign* directCondStateVarAssign(Ast_গ্রন্থি* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_গ্রন্থি_রাশিমালা*& condp,
                                                   Fsm_State_মান& reset_মান) {
        Ast_গ্রন্থি_Assign* ধ্রুব assp = VN_CAST(nodep, গ্রন্থি_Assign);
        যদি (!assp) প্রত্যাগম nullptr;
        AstVarRef* ধ্রুব lhsp = VN_CAST(assp->lhsp()->baseFromp(সত্য), VarRef);
        AstCond* ধ্রুব rhsp = VN_CAST(assp->rhsp(), Cond);
        যদি (!rhsp || !lhsp) প্রত্যাগম nullptr;
        AstVarRef* ধ্রুব elsep = VN_CAST(rhsp->elsep(), VarRef);
        যদি (!elsep) প্রত্যাগম nullptr;
        যদি (const_মান_Status(rhsp->thenp(), reset_মান) != Const_মান_Status::OK) প্রত্যাগম nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        প্রত্যাগম assp;
    }

    স্থির Ast_গ্রন্থি_Assign* direct_Const_State_Assign_গ্রন্থি(Ast_গ্রন্থি* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_মান& মান) {
        Ast_গ্রন্থি_Assign* ধ্রুব assp = VN_CAST(nodep, গ্রন্থি_Assign);
        যদি (!assp) প্রত্যাগম nullptr;
        AstVarRef* ধ্রুব lhsp = VN_CAST(assp->lhsp()->baseFromp(সত্য), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        যদি (const_মান_Status(assp->rhsp(), মান) != Const_মান_Status::OK) প্রত্যাগম nullptr;
        stateVscp = lhsp->varScopep();
        প্রত্যাগম assp;
    }

    গণনা বর্গ ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    স্থির ResetAssignStatus collectConstStateAssigns(Ast_গ্রন্থি* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_গ্রন্থি* nodep = skipLeadingIgnorableStmt(stmtp);
        যদি (!nodep) প্রত্যাগম ResetAssignStatus::EMPTY;
        প্রতি (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_মান মান;
            Ast_গ্রন্থি_Assign* ধ্রুব assp = direct_Const_State_Assign_গ্রন্থি(nodep, assignStateVscp, মান);
            যদি (!assp) প্রত্যাগম ResetAssignStatus::NONE;
            যদি (!stateVscp) stateVscp = assignStateVscp;
            যদি (assignStateVscp != stateVscp) প্রত্যাগম ResetAssignStatus::NONE;
            যদি (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                প্রত্যাগম ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(মান, assp);
            যদি (!nodep->nextp()) প্রত্যাগম ResetAssignStatus::SINGLE;
        }
    }

    স্থির তার্কিক hasCanonicalNextStateDefaultBeforeCase(Ast_গ্রন্থি* stmtsp, Ast_গ্রন্থি* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_গ্রন্থি* ধ্রুব bodyp = skipLeadingIgnorableStmt(stmtsp);
        তার্কিক sawCanonicalDefault = অসত্য;
        প্রতি (Ast_গ্রন্থি* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            যদি (nodep == targetp) প্রত্যাগম sawCanonicalDefault;
            যদি (Ast_গ্রন্থি_Assign* ধ্রুব assp = VN_CAST(nodep, গ্রন্থি_Assign)) {
                AstVarRef* ধ্রুব lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ধ্রুব rhsp = VN_CAST(assp->rhsp(), VarRef);
                যদি (!lhsp || lhsp->varScopep() != nextVscp) অনুবর্ত;
                যদি (sawCanonicalDefault) {
                    ধ্রুব string nextName = nextVscp->varp()->prettyNameQ();
                    ধ্রুব string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    প্রত্যাগম অসত্য;
                }
                যদি (!rhsp || rhsp->varScopep() != stateVscp) প্রত্যাগম অসত্য;
                sawCanonicalDefault = সত্য;
            }
        }
    }

    স্থির তার্কিক ifStateConstAssign(Ast_গ্রন্থি* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_মান& then_মান, Fsm_State_মান& else_মান) {
        AstIf* ধ্রুব ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        যদি (!ifp || !ifp->elsesp()) প্রত্যাগম অসত্য;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_গ্রন্থি* ধ্রুব thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_গ্রন্থি* ধ্রুব elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        যদি (!thenNodep || !elseNodep) প্রত্যাগম অসত্য;
        যদি (!direct_Const_State_Assign_গ্রন্থি(thenNodep, thenVscp, then_মান)) প্রত্যাগম অসত্য;
        যদি (!direct_Const_State_Assign_গ্রন্থি(elseNodep, elseVscp, else_মান)) প্রত্যাগম অসত্য;
        যদি (thenVscp == stateVscp && elseVscp == stateVscp) প্রত্যাগম সত্য;
        যদি (thenVscp != elseVscp) প্রত্যাগম অসত্য;
        Ast_গ্রন্থি* ধ্রুব followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_গ্রন্থি* ধ্রুব finalNodep = singleMeaningfulBranch(followp);
        যদি (!finalNodep) প্রত্যাগম অসত্য;
        যদি (!গ্রন্থি_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) প্রত্যাগম অসত্য;
        যদি (finalStateVscp != stateVscp) প্রত্যাগম অসত্য;
        যদি (finalFromVscp != thenVscp) প্রত্যাগম অসত্য;
        প্রত্যাগম সত্য;
    }

    স্থির তার্কিক directStateCondConstAssign(Ast_গ্রন্থি* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_মান& then_মান, Fsm_State_মান& else_মান) {
        Ast_গ্রন্থি_Assign* ধ্রুব assp = directStateAssign(stmtp, stateVscp);
        যদি (!assp) প্রত্যাগম অসত্য;
        AstCond* ধ্রুব condp = VN_CAST(assp->rhsp(), Cond);
        যদি (!condp) প্রত্যাগম অসত্য;
        প্রত্যাগম const_মান_Status(condp->thenp(), then_মান) == Const_মান_Status::OK
               && const_মান_Status(condp->elsep(), else_মান) == Const_মান_Status::OK;
    }

    স্থির Ast_গ্রন্থি* case_Item_Supported_Arc_গ্রন্থি(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             তার্কিক inclCond) {
        যদি (itemp->isDefault()) {
            যদি (!inclCond) প্রত্যাগম nullptr;
        }
        Ast_গ্রন্থি_Assign* ধ্রুব assp = directStateAssign(itemp->stmtsp(), stateVscp);
        যদি (assp) {
            Fsm_State_মান to_মান;
            যদি (const_মান_Status(assp->rhsp(), to_মান) == Const_মান_Status::OK) প্রত্যাগম assp;
        }
        Fsm_State_মান then_মান;
        Fsm_State_মান else_মান;
        যদি (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_মান, else_মান)) {
            প্রত্যাগম assp;
        }
        যদি (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_মান, else_মান)) {
            প্রত্যাগম singleMeaningfulBranch(itemp->stmtsp());
        }
        প্রত্যাগম nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    স্থির Ast_গ্রন্থি* case_Supported_Transition_গ্রন্থি(AstCase* casep, AstVarScope* stateVscp,
                                                তার্কিক inclCond) {
        প্রতি (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            যদি (Ast_গ্রন্থি* ধ্রুব nodep = case_Item_Supported_Arc_গ্রন্থি(itemp, stateVscp, inclCond))
                প্রত্যাগম nodep;
        }
        প্রত্যাগম nullptr;
    }

    স্থির Ast_গ্রন্থি* case_Item_Supported_Arc_গ্রন্থি_Like(Ast_গ্রন্থি* stmtsp, AstVarScope* stateVscp) {
        যদি (Ast_গ্রন্থি_Assign* ধ্রুব assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_মান to_মান;
            যদি (const_মান_Status(assp->rhsp(), to_মান) == Const_মান_Status::OK) প্রত্যাগম assp;
            Fsm_State_মান then_মান;
            Fsm_State_মান else_মান;
            যদি (directStateCondConstAssign(stmtsp, stateVscp, then_মান, else_মান)) প্রত্যাগম assp;
        }
        প্রত্যাগম nullptr;
    }

    স্থির Ast_গ্রন্থি* if_Chain_Supported_Transition_গ্রন্থি(ধ্রুব FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        প্রতি (size_t i = 0; i < chain.branches.size(); ++i) {
            ধ্রুব FsmIfBranch& branch = chain.branches[i];
            Ast_গ্রন্থি* ধ্রুব nodep = case_Item_Supported_Arc_গ্রন্থি_Like(branch.stmtsp, stateVscp);
            যদি (!nodep) প্রত্যাগম nullptr;
        }
        প্রত্যাগম chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    স্থির string label_For_মান(
        ধ্রুব std::unordered_map<Fsm_State_মান, StateConstLabel, Fsm_State_মান_Hash>& labels,
        ধ্রুব Fsm_State_মান& মান) {
        প্রত্যাগম labels.at(মান).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    গণনা বর্গ Const_মান_Status : uint8_t { OK, NOT_CONST, XZ };

    স্থির Const_মান_Status const_মান_Status(Ast_গ্রন্থি_রাশিমালা* exprp, Fsm_State_মান& মান) {
        ধ্রুব AstConst* ধ্রুব constp = VN_CAST(exprp, Const);
        যদি (!constp) প্রত্যাগম Const_মান_Status::NOT_CONST;
        ধ্রুব V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
        যদি (অঙ্ক.isAnyXZ()) প্রত্যাগম Const_মান_Status::XZ;
        মান = Fsm_State_মান{অঙ্ক};
        প্রত্যাগম Const_মান_Status::OK;
    }

    স্থির তার্কিক pureStateComparisonNoAlias(Ast_গ্রন্থি_রাশিমালা* exprp, FsmStateComparison& cmp) {
        AstEq* ধ্রুব eqp = VN_CAST(exprp, Eq);
        যদি (!eqp) প্রত্যাগম অসত্য;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_গ্রন্থি_রাশিমালা* valuep = eqp->rhsp();
        যদি (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(সত্য), VarRef);
            যদি (!vrefp) { প্রত্যাগম অসত্য; }
            valuep = eqp->lhsp();
        }

        Fsm_State_মান মান;
        যদি (const_মান_Status(valuep, মান) != Const_মান_Status::OK) প্রত্যাগম অসত্য;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.মান = মান;
        প্রত্যাগম সত্য;
    }

    স্থির তার্কিক pureStateComparison(Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        যদি (pureStateComparisonNoAlias(exprp, cmp)) প্রত্যাগম সত্য;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        যদি (AstVarRef* ধ্রুব vrefp = VN_CAST(exprp, VarRef)) {
            ধ্রুব FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            যদি (it == aliases.end()) প্রত্যাগম অসত্য;
            cmp = it->second;
            প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    স্থির তার্কিক supported_শীর্ষ_Level_Guard(Ast_গ্রন্থি_রাশিমালা* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        যদি (VN_IS(exprp, Or)) প্রত্যাগম অসত্য;
        যদি (VN_IS(exprp, RedAnd)) প্রত্যাগম অসত্য;
        যদি (VN_IS(exprp, RedOr)) প্রত্যাগম অসত্য;
        যদি (VN_IS(exprp, RedXor)) প্রত্যাগম অসত্য;
        প্রত্যাগম সত্য;
    }

    স্থির তার্কিক resolveIfPredicate(Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুব FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, তার্কিক& hasGuard) {
        std::vector<Ast_গ্রন্থি_রাশিমালা*> terms;
        std::vector<Ast_গ্রন্থি_রাশিমালা*> pending;
        pending.ঠেলো_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        যাবত্ (!pending.empty()) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব nodep = pending.back();
            pending.বের_করো_back();
            যদি (AstAnd* ধ্রুব andp = VN_CAST(nodep, And)) {
                pending.ঠেলো_back(andp->rhsp());
                pending.ঠেলো_back(andp->lhsp());
            } অন্যথা {
                terms.ঠেলো_back(nodep);
            }
        }

        তার্কিক sawComparison = অসত্য;
        প্রতি (size_t i = 0; i < terms.size(); ++i) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব termp = terms[i];
            FsmStateComparison termCmp;
            যদি (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                যদি (sawComparison) প্রত্যাগম অসত্য;
                cmp = termCmp;
                sawComparison = সত্য;
                অনুবর্ত;
            }
            যদি (!supported_শীর্ষ_Level_Guard(termp)) প্রত্যাগম অসত্য;
            hasGuard = সত্য;
        }
        প্রত্যাগম sawComparison;
    }

    স্থির শূন্য addAlias(FsmAliasMap& aliases, std::unordered_set<ধ্রুব AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ধ্রুব FsmStateComparison& cmp) {
        যদি (ambiguous.find(aliasVscp) != ambiguous.end()) প্রত্যাগম;
        ধ্রুব FsmAliasMap::iterator it = aliases.find(aliasVscp);
        যদি (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            প্রত্যাগম;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        যদি (it->second.stateVscp == cmp.stateVscp && it->second.মান == cmp.মান) প্রত্যাগম;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        প্রত্যাগম;
    }

    স্থির শূন্য collectAliasFromAssign(Ast_গ্রন্থি_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<ধ্রুব AstVarScope*>& ambiguous) {
        AstVarRef* ধ্রুব lhsp = VN_CAST(assp->lhsp(), VarRef);
        যদি (!lhsp) প্রত্যাগম;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        যদি (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) প্রত্যাগম;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_গ্রন্থি* stmtsp, Ast_গ্রন্থি* limitp) ধ্রুব {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<ধ্রুব AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        প্রতি (Ast_গ্রন্থি* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            যদি (Ast_গ্রন্থি_Assign* ধ্রুব assp = VN_CAST(nodep, গ্রন্থি_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        প্রতি (ধ্রুব AstVarScope* ধ্রুব vscp : ambiguous) aliases.erase(vscp);
        প্রত্যাগম aliases;
    }

    স্থির তার্কিক collectIfChain(AstIf* ifp, ধ্রুব FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_মানসমূহ;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        যাবত্ (সত্য) {
            FsmStateComparison cmp;
            তার্কিক hasGuard = অসত্য;
            যদি (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) প্রত্যাগম অসত্য;
            যদি (chain.compareVscp && chain.compareVscp != cmp.stateVscp) প্রত্যাগম অসত্য;
            যদি (!seen_মানসমূহ.ঢোকাও(cmp.মান.চাবি()).second) প্রত্যাগম অসত্য;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ঠেলো_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.মান, hasGuard});

            Ast_গ্রন্থি* ধ্রুব elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            যদি (!elseNodep) ভঙ্গ;
            যদি (AstIf* ধ্রুব elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                অনুবর্ত;
            }
            chain.defaultStmtsp = elseNodep;
            ভঙ্গ;
        }
        প্রত্যাগম chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    স্থির তার্কিক validate_Known_State_মান(Ast_গ্রন্থি* nodep, ধ্রুব FsmStateSpace& stateSpace,
                                        ধ্রুব Fsm_State_মান& মান, ধ্রুব string& role) {
        যদি (stateSpace.labels.find(মান) != stateSpace.labels.end()) প্রত্যাগম সত্য;
        যদি (stateSpace.enumBacked) {
            ধ্রুব string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + মান.warnText()
                                        + " is not present in the declared enum");
            প্রত্যাগম অসত্য;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + মান.warnText()
                                    + " is not present in the inferred state space");
        প্রত্যাগম অসত্য;
    }

    স্থির StateConstLabel stateLabelForConst(AstConst* constp) {
        ধ্রুব string name = constp->origParamName();
        যদি (!name.empty()) প্রত্যাগম StateConstLabel{Ast_গ্রন্থি::prettyName(name), সত্য, 0};
        প্রত্যাগম StateConstLabel{constp->name(), অসত্য, 0};
    }

    স্থির শূন্য updateStateLabel(FsmStateSpace& stateSpace, ধ্রুব Fsm_State_মান& মান,
                                 ধ্রুব StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(মান).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    স্থির তার্কিক matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        যদি (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) প্রত্যাগম অসত্য;

        Ast_গ্রন্থি* ধ্রুব stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_গ্রন্থি* ধ্রুব nodep = singleMeaningfulStmt(stmtsp);
        যদি (!nodep) প্রত্যাগম অসত্য;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        যদি (AstIf* ধ্রুব ifp = VN_CAST(nodep, If)) {
            যদি (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) প্রত্যাগম অসত্য;
            AstVarScope* resetStateVscp = nullptr;
            ধ্রুব ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ধ্রুব তার্কিক emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            যদি (emptyResetBranch) {
                cand.resetArcs().clear();
            } অন্যথা যদি (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_মান reset_মান;
                Ast_গ্রন্থি* ধ্রুব thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_গ্রন্থি_Assign* ধ্রুব resetAssp
                    = direct_Const_State_Assign_গ্রন্থি(thenNodep, resetStateVscp, reset_মান);
                যদি (!resetAssp) প্রত্যাগম অসত্য;
                cand.resetArcs().emplace_back(reset_মান, resetAssp);
            } অন্যথা যদি (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_গ্রন্থি* ধ্রুব elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            যদি (!গ্রন্থি_State_Var_Assign(elseNodep, stateVscp, nextVscp)) প্রত্যাগম অসত্য;
            যদি (!emptyResetBranch && resetStateVscp != stateVscp) প্রত্যাগম অসত্য;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } অন্যথা {
            Ast_গ্রন্থি_রাশিমালা* resetCondp = nullptr;
            Fsm_State_মান reset_মান;
            // Note the active low 'reset ? next : CONST' form is not accepted
            যদি (Ast_গ্রন্থি_Assign* ধ্রুব assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_মান)) {
                cand.resetArcs().emplace_back(reset_মান, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } অন্যথা যদি (!গ্রন্থি_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                প্রত্যাগম অসত্য;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_বৃক্ষ(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        প্রত্যাগম সত্য;
    }

    স্থির তার্কিক add_মান_To_State_Space(Ast_গ্রন্থি* nodep, FsmStateSpace& stateSpace,
                                     ধ্রুব Fsm_State_মান& মান, StateConstLabel label) {
        ধ্রুব auto labelIt = stateSpace.labels.find(মান);
        যদি (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            যদি (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + মান.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                প্রত্যাগম অসত্য;
            }
            যদি (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, মান, label);
            }
        } অন্যথা {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, মান);
            stateSpace.labels.emplace(মান, storedLabel);
        }
        প্রত্যাগম সত্য;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    স্থির তার্কিক add_রাশিমালা_To_State_Space(Ast_গ্রন্থি_রাশিমালা* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_মান মান;
        ধ্রুব Const_মান_Status status = const_মান_Status(valuep, মান);
        যদি (status != Const_মান_Status::OK) {
            যদি (status == Const_মান_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            প্রত্যাগম অসত্য;
        }
        AstConst* ধ্রুব constp = VN_AS(valuep, Const);
        প্রত্যাগম add_মান_To_State_Space(valuep, stateSpace, মান, stateLabelForConst(constp));
    }

    স্থির তার্কিক add_Optional_Target_রাশিমালা_To_State_Space(Ast_গ্রন্থি_রাশিমালা* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_মান মান;
        ধ্রুব Const_মান_Status status = const_মান_Status(valuep, মান);
        যদি (status != Const_মান_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            প্রত্যাগম অসত্য;
        }
        AstConst* ধ্রুব constp = VN_AS(valuep, Const);
        প্রত্যাগম add_মান_To_State_Space(valuep, stateSpace, মান, stateLabelForConst(constp));
    }

    স্থির শূন্য addResetTargetsToStateSpace(ধ্রুব std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        প্রতি (ধ্রুব FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_মান().ascii(), অসত্য, 0};
            যদি (AstConst* ধ্রুব constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_মান_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_মান(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    স্থির তার্কিক collectDeclaredStateSpace(Ast_গ্রন্থি* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, তার্কিক& needs_Source_মানসমূহ) {
        AstVar* ধ্রুব stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        যদি (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ধ্রুব তার্কিক forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        যদি (enump) {
            stateSpace.enumBacked = সত্য;
            প্রতি (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ধ্রুব AstConst* ধ্রুব constp = VN_AS(itemp->valuep(), Const);
                ধ্রুব Fsm_State_মান মান{constp->অঙ্ক()};
                ধ্রুব size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), মান);
                stateSpace.labels.emplace(মান,
                                          StateConstLabel{itemp->name(), অসত্য, stateIndex});
            }
            প্রত্যাগম stateSpace.states.size() >= 2;
        }

        যদি (forced) {
            needs_Source_মানসমূহ = সত্য;
            প্রত্যাগম সত্য;
        }

        needs_Source_মানসমূহ = সত্য;
        প্রত্যাগম সত্য;
    }

    template <typename T_ValuepVisitor>
    স্থির তার্কিক collect_State_Space_From_মানসমূহ(Ast_গ্রন্থি* warnNodep, AstVarScope* stateVscp,
                                            ধ্রুব std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ধ্রুব T_ValuepVisitor& visitValueps) {
        তার্কিক needs_Source_মানসমূহ = অসত্য;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        যদি (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_মানসমূহ)) {
            প্রত্যাগম অসত্য;
        }
        যদি (!needs_Source_মানসমূহ) প্রত্যাগম সত্য;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        যদি (!visitValueps(
                [&](Ast_গ্রন্থি_রাশিমালা* valuep) { প্রত্যাগম add_রাশিমালা_To_State_Space(valuep, stateSpace); })) {
            প্রত্যাগম অসত্য;
        }
        প্রত্যাগম stateSpace.states.size() >= 2;
    }

    স্থির তার্কিক collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  ধ্রুব std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        প্রত্যাগম collect_State_Space_From_মানসমূহ(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](ধ্রুব auto& visitValuep) {
                প্রতি (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    যদি (!itemp->isDefault()) {
                        প্রতি (Ast_গ্রন্থি_রাশিমালা* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), গ্রন্থি_রাশিমালা)) {
                            যদি (!visitValuep(condp)) প্রত্যাগম অসত্য;
                        }
                    }
                    যদি (Ast_গ্রন্থি_Assign* ধ্রুব assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_মান then_মান;
                        Fsm_State_মান else_মান;
                        AstCond* ধ্রুব condp = VN_CAST(assp->rhsp(), Cond);
                        যদি (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_মান,
                                                          else_মান)) {
                            যদি (!visitValuep(condp->thenp())) প্রত্যাগম অসত্য;
                            যদি (!visitValuep(condp->elsep())) প্রত্যাগম অসত্য;
                        } অন্যথা যদি (!add_Optional_Target_রাশিমালা_To_State_Space(assp->rhsp(), stateSpace)) {
                            প্রত্যাগম অসত্য;
                        }
                    }
                }
                প্রত্যাগম সত্য;
            });
    }

    স্থির তার্কিক collectStateSpace(ধ্রুব FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  ধ্রুব std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        প্রত্যাগম collect_State_Space_From_মানসমূহ(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](ধ্রুব auto& visitValuep) {
                প্রতি (ধ্রুব FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_গ্রন্থি_Assign* ধ্রুব assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_মান then_মান;
                    Fsm_State_মান else_মান;
                    AstCond* ধ্রুব condp = VN_CAST(assp->rhsp(), Cond);
                    যদি (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_মান, else_মান),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        যদি (!visitValuep(condp->thenp())) প্রত্যাগম অসত্য;
                        যদি (!visitValuep(condp->elsep())) প্রত্যাগম অসত্য;
                    } অন্যথা যদি (!add_Optional_Target_রাশিমালা_To_State_Space(assp->rhsp(), stateSpace)) {
                        প্রত্যাগম অসত্য;
                    }
                }
                প্রত্যাগম সত্য;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    স্থির তার্কিক emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 ধ্রুব FsmStateSpace& stateSpace, তার্কিক inclCond) {
        std::vector<std::pair<string, Fsm_State_মান>> froms;
        যদি (itemp->isDefault()) {
            যদি (!inclCond) প্রত্যাগম অসত্য;
            froms.emplace_back("default", Fsm_State_মান{});
        } অন্যথা {
            প্রতি (Ast_গ্রন্থি_রাশিমালা* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), গ্রন্থি_রাশিমালা)) {
                Fsm_State_মান মান;
                যদি (const_মান_Status(condp, মান) != Const_মান_Status::OK) অনুবর্ত;
                যদি (!validate_Known_State_মান(condp, stateSpace, মান, "source")) প্রত্যাগম সত্য;
                froms.emplace_back(label_For_মান(stateSpace.labels, মান), মান);
            }
            যদি (froms.empty()) প্রত্যাগম অসত্য;
        }

        যদি (Ast_গ্রন্থি_Assign* ধ্রুব assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_মান to_মান;
            ধ্রুব Const_মান_Status status = const_মান_Status(assp->rhsp(), to_মান);
            যদি (status == Const_মান_Status::OK) {
                যদি (!validate_Known_State_মান(assp, stateSpace, to_মান, "target")) প্রত্যাগম সত্য;
                প্রতি (ধ্রুব std::pair<string, Fsm_State_মান>& from : froms) {
                    graph.addArc(from.second, to_মান, অসত্য, অসত্য, itemp->isDefault(),
                                 assp->fileline());
                }
                প্রত্যাগম সত্য;
            }
        }

        Fsm_State_মান then_মান;
        Fsm_State_মান else_মান;
        যদি (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_মান, else_মান)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_মান, else_মান)) {
            যদি (!validate_Known_State_মান(itemp->stmtsp(), stateSpace, then_মান, "target"))
                প্রত্যাগম সত্য;
            যদি (!validate_Known_State_মান(itemp->stmtsp(), stateSpace, else_মান, "target"))
                প্রত্যাগম সত্য;
            প্রতি (ধ্রুব Fsm_State_মান& branch_মান : {then_মান, else_মান}) {
                প্রতি (ধ্রুব std::pair<string, Fsm_State_মান>& from : froms) {
                    graph.addArc(from.second, branch_মান, অসত্য, সত্য, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            প্রত্যাগম সত্য;
        }

        প্রত্যাগম অসত্য;
    }

    স্থির শূন্য emitStmtArcsFrom(FsmGraph& graph, Ast_গ্রন্থি* stmtsp, AstVarScope* stateVscp,
                                 ধ্রুব FsmStateSpace& stateSpace, Fsm_State_মান from_মান,
                                 তার্কিক isDefault, তার্কিক forceCond) {
        Ast_গ্রন্থি_Assign* ধ্রুব assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_মান to_মান;
        ধ্রুব Const_মান_Status status = const_মান_Status(assp->rhsp(), to_মান);
        যদি (status == Const_মান_Status::OK) {
            যদি (!validate_Known_State_মান(assp, stateSpace, to_মান, "target")) প্রত্যাগম;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_মান, to_মান, অসত্য, forceCond, isDefault, assp->fileline());
            প্রত্যাগম;
        }

        Fsm_State_মান then_মান;
        Fsm_State_মান else_মান;
        ধ্রুব তার্কিক condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_মান, else_মান);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        যদি (!validate_Known_State_মান(stmtsp, stateSpace, then_মান, "target")) প্রত্যাগম;
        যদি (!validate_Known_State_মান(stmtsp, stateSpace, else_মান, "target")) প্রত্যাগম;
        প্রতি (ধ্রুব Fsm_State_মান& branch_মান : {then_মান, else_মান}) {
            graph.addArc(from_মান, branch_মান, অসত্য, সত্য, isDefault, stmtsp->fileline());
        }
    }

    স্থির শূন্য emitIfChainArcs(FsmGraph& graph, ধ্রুব FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ধ্রুব FsmStateSpace& stateSpace) {
        প্রতি (size_t i = 0; i < chain.branches.size(); ++i) {
            ধ্রুব FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            যদি (!validate_Known_State_মান(branch.ifp, stateSpace, branch.from_মান, "source"))
                প্রত্যাগম;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_মান, অসত্য,
                             branch.has_শীর্ষ_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    স্থির শূন্য addResetArcs(FsmGraph& graph, ধ্রুব std::vector<FsmResetArcDesc>& resetArcs,
                             ধ্রুব FsmStateSpace& stateSpace) {
        প্রতি (ধ্রুব FsmResetArcDesc& resetArc : resetArcs) {
            যদি (!validate_Known_State_মান(resetArc.nodep(), stateSpace, resetArc.to_মান(),
                                         "target"))
                অনুবর্ত;
            graph.addArc(Fsm_State_মান{}, resetArc.to_মান(), সত্য, অসত্য, অসত্য,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    শূন্য processCase(AstCase* casep, AstVarScope* assignVscp, ধ্রুব FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ধ্রুব stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        যদি (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) প্রত্যাগম;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        যদি (!entry.graphp) {
            entry.graphp.reset(নূতন FsmGraph{});
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
            প্রতি (ধ্রুব std::pair<string, Fsm_State_মান>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        প্রতি (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    শূন্য processIfChain(ধ্রুব FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        ধ্রুব FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ধ্রুব stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        যদি (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) প্রত্যাগম;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(নূতন FsmGraph{});
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
        প্রতি (ধ্রুব std::pair<string, Fsm_State_মান>& state : stateSpace.states) {
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
    শূন্য processOneBlockAlways(ধ্রুব FsmComboAlways& oneBlock) {
        ধ্রুব RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* ধ্রুব alwaysp = oneBlock.alwaysp();
        যদি (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) প্রত্যাগম;
        ধ্রুব std::vector<std::pair<AstCase*, Ast_গ্রন্থি_রাশিমালা*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        প্রতি (ধ্রুব std::pair<AstCase*, Ast_গ্রন্থি_রাশিমালা*>& cand : candidates) {
            AstVarRef* ধ্রুব selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* ধ্রুব vscp = selp ? selp->varScopep() : nullptr;
            যদি (!vscp) অনুবর্ত;
            যদি (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } অন্যথা যদি (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } অন্যথা {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        যদি (firstCand.stateVscp) প্রত্যাগম;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ধ্রুব std::vector<std::pair<AstIf*, Ast_গ্রন্থি_রাশিমালা*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        প্রতি (ধ্রুব std::pair<AstIf*, Ast_গ্রন্থি_রাশিমালা*>& cand : ifCandidates) {
            ধ্রুব FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            যদি (!collectIfChain(cand.first, aliases, chain)) অনুবর্ত;
            AstVarScope* ধ্রুব vscp = chain.compareVscp;
            যদি (!if_Chain_Supported_Transition_গ্রন্থি(chain, vscp)) অনুবর্ত;
            যদি (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } অন্যথা যদি (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } অন্যথা {
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
    স্থির শূন্য warnComboSameAlways(Ast_গ্রন্থি* warnNodep, ধ্রুব FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    শূন্য processComboAlways(ধ্রুব FsmComboAlways& combo) {
        ধ্রুব ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_গ্রন্থি* ধ্রুব stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        প্রতি (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ধ্রুব casep = VN_CAST(nodep, Case);
            যদি (!casep) অনুবর্ত;
            ধ্রুব ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            ধ্রুব FsmRegisterCandidate* ধ্রুব matchedp = match.matchedp;
            Ast_গ্রন্থি* ধ্রুব matchedWarnNodep = match.warnNodep;
            যদি (!matchedp) অনুবর্ত;
            যদি (!firstCand.stateVscp) {
                ধ্রুব auto ঢোকাও_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                যদি (!ঢোকাও_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ঢোকাও_Pair.first->second));
                    অনুবর্ত;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                অনুবর্ত;
            }
            যদি (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                অনুবর্ত;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        যদি (firstCand.stateVscp) প্রত্যাগম;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        প্রতি (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ধ্রুব ifp = VN_CAST(nodep, If);
            যদি (!ifp) অনুবর্ত;
            FsmIfChainCandidate chain;
            ধ্রুব FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            যদি (!collectIfChain(ifp, aliases, chain)) অনুবর্ত;
            ধ্রুব ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            ধ্রুব FsmRegisterCandidate* ধ্রুব matchedp = match.matchedp;
            Ast_গ্রন্থি* ধ্রুব matchedWarnNodep = match.warnNodep;
            যদি (!matchedp) অনুবর্ত;
            যদি (!firstCand.stateVscp) {
                ধ্রুব std::pair<std::unordered_map<ধ্রুব AstVarScope*, FsmCaseCandidate>::iterator,
                                তার্কিক>
                    ঢোকাও_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                যদি (!ঢোকাও_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ঢোকাও_Pair.first->second));
                    অনুবর্ত;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                অনুবর্ত;
            }
            যদি (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                অনুবর্ত;
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
    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    শূন্য visit(AstAlways* nodep) override {
        যদি (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            প্রত্যাগম;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        ধ্রুব RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        যদি (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* ধ্রুব stateVscp = reg.stateVscp();
            ধ্রুব তার্কিক found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](ধ্রুব FsmRegisterCandidate& existing) {
                                  প্রত্যাগম existing.stateVscp() == stateVscp;
                              });
            যদি (!found) { m_registerCandidates.emplace_back(reg); }
        }
        যদি (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } অন্যথা যদি (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            যদি (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } অন্যথা {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    শূন্য visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    শূন্য visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        যদি (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        প্রতি (ধ্রুব std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            যদি (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        প্রতি (ধ্রুব FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        প্রতি (ধ্রুব FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        প্রতি (ধ্রুব FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
বর্গ FsmCoverIncBuilder final {
    ধ্রুব FsmGraph& m_graph;  // Fsm graph
    std::vector<ধ্রুব FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ধ্রুব FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    ধ্রুব FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edges_গুনতি = 0;  // Total count of edges
    AstVarScope* ধ্রুব m_prevVscp;  // Previous state of fsm
    File_সারি* ধ্রুব m_flp;  // Fileline pointer to use across the builder
    ধ্রুব std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_গ্রন্থি_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_গ্রন্থি_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    তার্কিক m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ধ্রুব uint64_t মান) {
        V3_সংখ্যা অঙ্ক{static_cast<Ast_গ্রন্থি*>(nullptr), m_prevVscp->width()};
        অঙ্ক.setQuad(মান);
        অঙ্ক.isSigned(m_prevVscp->dtypep()->isSigned());
        প্রত্যাগম নূতন AstConst{m_flp, অঙ্ক};
    }

    std::string quadToBitStirngLabel(ধ্রুব uint64_t val) {
        পূর্ণাঙ্ক width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        যাবত্ (--width >= 0) result.ঠেলো_back("01"[(val >> width) & 1]);
        প্রত্যাগম result;
    }

    শূন্য initExistingStates() {
        প্রতি (ধ্রুব V3GraphVertex& vtx : m_graph.vertices()) {
            ধ্রুব FsmVertex* ধ্রুব vertexp = vtx.as<FsmVertex>();
            m_edges_গুনতি += vertexp->inEdges().size();
            যদি (ধ্রুব FsmStateVertex* ধ্রুব statep = vertexp->cast<FsmStateVertex>()) {
                m_states.ঠেলো_back(statep);
            } অন্যথা যদি (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } অন্যথা /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ধ্রুব পূর্ণাঙ্ক width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    শূন্য initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ধ্রুব uint64_t combinations_গুনতি = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        তার্কিক undefRolled = অসত্য;
        auto rollUndefs = [এতত্, combinations_গুনতি] {
            প্রতি (uint64_t i = 0; i < combinations_গুনতি; ++i) {
                ধ্রুব auto ঢোকাও = m_undefinedStates.ঢোকাও(i);
                যদি (!ঢোকাও.second) m_undefinedStates.erase(ঢোকাও.first);
            }
        };
        প্রতি (ধ্রুব FsmStateVertex* ধ্রুব statep : m_states) {
            ধ্রুব V3_সংখ্যা& অঙ্ক = statep->মান().অঙ্ক();
            ধ্রুব uint64_t মান = অঙ্ক.toUQuad();
            যদি (!m_isComplete) {
                যদি (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(মান) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } অন্যথা {
                    UASSERT_OBJ(m_undefinedStates.ঢোকাও(মান).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    যদি (m_undefinedStates.size() * 2 >= combinations_গুনতি) {
                        undefRolled = সত্য;
                        rollUndefs();
                    }
                }
            }
        }
        যদি (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_গ্রন্থি_রাশিমালা* buildResetCond() ধ্রুব {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব refp
            = নূতন AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        প্রত্যাগম m_graph.resetCond().active_নিচের
                   ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নূতন AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t get_Arc_গুনতি_With_Expansion(ধ্রুব VFsmExpandType expand) ধ্রুব {
        size_t arc_গুনতি_With_Expansion = ~0ULL;
        বিকল্প (expand) {
        পক্ষ VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            ভঙ্গ;
        }
        পক্ষ VFsmExpandType::AUTO: {
            arc_গুনতি_With_Expansion = m_edges_গুনতি;
            যদি (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_গুনতি_With_Expansion -= m_defaultVertexp->outEdges().size();
                প্রতি (ধ্রুব FsmStateVertex* ধ্রুব statep : m_states) {
                    arc_গুনতি_With_Expansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            যদি (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_গুনতি_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            ভঙ্গ;
        }
        পক্ষ VFsmExpandType::AUTO_EXPAND: {
            arc_গুনতি_With_Expansion = m_states.size() * m_states.size() - m_states.size();
            ভঙ্গ;
        }
        পক্ষ VFsmExpandType::FULL: {
            ধ্রুব পূর্ণাঙ্ক width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            যদি (width < 32) {
                ধ্রুব uint64_t combinations_গুনতি = static_cast<size_t>(1) << width;
                arc_গুনতি_With_Expansion = combinations_গুনতি * combinations_গুনতি - combinations_গুনতি;
            }
            ভঙ্গ;
        }
        }
        প্রত্যাগম arc_গুনতি_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ধ্রুব std::string& label,
                                           ধ্রুব std::string& fsmTag = "") {
        AstCoverOtherDecl* ধ্রুব declp
            = নূতন AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(সত্য);
        m_declsp = Ast_গ্রন্থি::addNext(m_declsp, declp);
        প্রত্যাগম declp;
    }

    Ast_গ্রন্থি_Stmt* buildCoverIncStmtp(ধ্রুব std::string& fromStateLabel,
                                    ধ্রুব std::string& toStateLabel,
                                    ধ্রুব std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ধ্রুব declp = নূতন AstCoverOtherDecl{
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
        declp->perInstance(সত্য);
        m_declsp = Ast_গ্রন্থি::addNext(m_declsp, declp);
        প্রত্যাগম নূতন AstCoverInc{m_flp, declp};
    }
    Ast_গ্রন্থি_Stmt* buildExpandedTransitionHandler(ধ্রুব std::string& toStateLabel,
                                                ধ্রুব তার্কিক considerUndefs,
                                                ধ্রুব তার্কিক expandOnlyWithTerminalVerticies,
                                                ধ্রুব FsmStateVertex* ধ্রুব toStatep = nullptr) {
        AstCase* ধ্রুব arcCasep = নূতন AstCase{
            m_flp, VCaseType::CT_CASE, নূতন AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<ধ্রুব FsmStateVertex*> handledSoureces;
        যদি (toStatep) {
            প্রতি (ধ্রুব V3GraphEdge& edge : toStatep->inEdges()) {
                যদি (ধ্রুব FsmStateVertex* ধ্রুব fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    যদি (!handledSoureces.emplace(fromp).second) অনুবর্ত;
                    arcCasep->addItemsp(নূতন AstCaseItem{
                        m_flp, makeConst(fromp->মান().অঙ্ক().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        প্রতি (ধ্রুব FsmStateVertex* ধ্রুব fromp : m_states) {
            যদি (handledSoureces.find(fromp) != handledSoureces.end()) অনুবর্ত;
            arcCasep->addItemsp(নূতন AstCaseItem{
                m_flp, makeConst(fromp->মান().অঙ্ক().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        যদি (!m_isComplete) {
            যদি (considerUndefs) {
                প্রতি (ধ্রুব uint64_t from : m_undefinedStates) {
                    ধ্রুব std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        নূতন AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } অন্যথা {
                arcCasep->addItemsp(নূতন AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        প্রত্যাগম arcCasep;
    }
    Ast_গ্রন্থি_Stmt* buildUnxpandedTransitionHandler(ধ্রুব FsmStateVertex* ধ্রুব toStatep) {
        AstCase* ধ্রুব arcCasep = নূতন AstCase{
            m_flp, VCaseType::CT_CASE, নূতন AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        প্রতি (ধ্রুব V3GraphEdge& edge : toStatep->inEdges()) {
            ধ্রুব FsmArcEdge* ধ্রুব arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ধ্রুব FsmStateVertex* ধ্রুব fromp = arcp->fromp()->as<FsmStateVertex>();
            যদি (fromp != toStatep) {
                arcCasep->addItemsp(
                    নূতন AstCaseItem{m_flp, makeConst(fromp->মান().অঙ্ক().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(নূতন AstCaseItem{m_flp, nullptr, nullptr});
        প্রত্যাগম arcCasep;
    }

সার্বজনিক:
    FsmCoverIncBuilder(ধ্রুব FsmGraph& graph, AstVarScope* ধ্রুব prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* ধ্রুব sampleVscp = graph.sampleVarScopep();
        ধ্রুব VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ধ্রুব size_t arc_গুনতি_With_Expansion = get_Arc_গুনতি_With_Expansion(expand);
            যদি (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_গুনতি_With_Expansion == 0ULL
                            || arc_গুনতি_With_Expansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_গুনতি_With_Expansion);
                প্রত্যাগম;
            }
        }

        AstCase* ধ্রুব arcCasep
            = নূতন AstCase{m_flp, VCaseType::CT_CASE,
                          নূতন AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* ধ্রুব initialStateCasep
            = নূতন AstCase{m_flp, VCaseType::CT_CASE,
                          নূতন AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        ধ্রুব তার্কিক expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<ধ্রুব FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<ধ্রুব FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        যদি (!expandAll) {
            যদি (m_defaultVertexp) {
                প্রতি (ধ্রুব V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.শীর্ষ()->as<FsmStateVertex>());
                }
            }
            যদি (m_resetVertexp) {
                প্রতি (ধ্রুব V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.শীর্ষ()->as<FsmStateVertex>());
                }
            }
        }
        ধ্রুব তার্কিক isFullExpansion = expand == VFsmExpandType::FULL;
        যদি (isFullExpansion) initUndefinedStates();
        প্রতি (ধ্রুব FsmStateVertex* ধ্রুব statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ধ্রুব declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                নূতন AstCaseItem{m_flp, makeConst(statep->মান().অঙ্ক().toUQuad()),
                                নূতন AstCoverInc{m_flp, declp, সত্য}});
            ধ্রুব তার্কিক expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            ধ্রুব তার্কিক defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(নূতন AstCaseItem{
                m_flp, makeConst(statep->মান().অঙ্ক().toUQuad()),
                Ast_গ্রন্থি::addNext<Ast_গ্রন্থি_Stmt>(
                    নূতন AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        যদি (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            যদি (isFullExpansion) {
                প্রতি (ধ্রুব uint64_t মান : m_undefinedStates) {
                    ধ্রুব std::string& label = quadToBitStirngLabel(মান);
                    AstCoverOtherDecl* ধ্রুব declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(নূতন AstCaseItem{
                        m_flp, makeConst(মান), নূতন AstCoverInc{m_flp, declp, সত্য}});
                    arcCasep->addItemsp(
                        নূতন AstCaseItem{m_flp, makeConst(মান),
                                        Ast_গ্রন্থি::addNext<Ast_গ্রন্থি_Stmt>(
                                            নূতন AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, সত্য, অসত্য))});
                }
            } অন্যথা {
                // No expansion
                arcCasep->addItemsp(নূতন AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(নূতন AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = স্বভাব;
    Ast_গ্রন্থি_Cover_Decl* getCoverDeclsp() ধ্রুব { প্রত্যাগম m_declsp; }
    Ast_গ্রন্থি_Stmt* getCoverIncp() ধ্রুব { প্রত্যাগম m_coverIncp; }
    Ast_গ্রন্থি_Stmt* getInitialCoverIncp() ধ্রুব { প্রত্যাগম m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
বর্গ FsmLowerVisitor final {
    // STATE - across all visitors
    ধ্রুব FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    স্থির Ast_Sen_বৃক্ষ* build_Sen_বৃক্ষ(File_সারি* flp, ধ্রুব std::vector<FsmSenDesc>& senses) {
        Ast_Sen_বৃক্ষ* ধ্রুব sentreep = নূতন Ast_Sen_বৃক্ষ{flp, nullptr};
        প্রতি (ধ্রুব FsmSenDesc& sense : senses) {
            AstSenItem* ধ্রুব senItemp
                = নূতন AstSenItem{flp, VEdgeType{sense.edgeType},
                                 নূতন AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        প্রত্যাগম sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    শূন্য buildOne(ধ্রুব FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* ধ্রুব alwaysp = graph.stateAlwaysp();
        AstScope* ধ্রুব scopep = graph.scopep();
        AstVarScope* ধ্রুব stateVscp = graph.stateVarScopep();
        AstVarScope* ধ্রুব sampleVscp = graph.sampleVarScopep();
        File_সারি* ধ্রুব flp = graph.fileline();
        Ast_গ্রন্থি_Module* ধ্রুব modp = scopep->modp();
        AstVarScope* ধ্রুব prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(সত্য);
        sampleVscp->optimizeLifePost(সত্য);
        prevVscp->optimizeLifePost(সত্য);

        AstActive* ধ্রুব initActivep
            = নূতন AstActive{flp, "fsm-coverage-init",
                            নূতন Ast_Sen_বৃক্ষ{flp, নূতন AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_বৃক্ষ_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(নূতন AstInitialStatic{
            flp, নূতন AstAssign{flp, নূতন AstVarRef{flp, prevVscp, VAccess::WRITE},
                               নূতন AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* ধ্রুব covPostp = নূতন AstAlwaysPost{flp};
        তার্কিক updatePrevAfterPost = অসত্য;
        যদি (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 নূতন AstVarRef{flp, sampleVscp, VAccess::READ}});
        } অন্যথা {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = সত্য;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ধ্রুব FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                নূতন AstIf{flp,
                          নূতন AstNeq{flp, নূতন AstVarRef{flp, prevVscp, VAccess::READ},
                                     নূতন AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        যদি (updatePrevAfterPost) {
            covPostp->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              নূতন AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_বৃক্ষ* ধ্রুব sentreep = build_Sen_বৃক্ষ(flp, graph.senses());
        AstActive* ধ্রুব activep = নূতন AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_বৃক্ষ_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

সার্বজনিক:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(ধ্রুব FsmState& state)
        : m_state{state} {
        প্রতি (ধ্রুব DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

শূন্য V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    যদি (dumpGraphLevel() >= 6) {
        size_t index = 0;
        প্রতি (ধ্রুব DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_বৃক্ষ("fsm-detect", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
