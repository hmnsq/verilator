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
শ্রেণী Fsm_State_মান final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_চাবি;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_সংখ্যা m_অঙ্ক;  // Original folded value, preserving width for lowered comparisons

    স্থির string make_চাবি(ধ্রুবক V3_সংখ্যা& অঙ্ক) {
        V3_সংখ্যা চাবি_অঙ্ক = অঙ্ক;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        চাবি_অঙ্ক.isSigned(মিথ্যা);
        ফেরত cvtToStr(চাবি_অঙ্ক.width()) + ":" + চাবি_অঙ্ক.ascii(সত্য, সত্য);
    }

সর্বজনীন:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_মান()
        : m_চাবি{"1:1'h0"}
        , m_অঙ্ক{static_cast<Ast_গ্রন্থি*>(nullptr), 1, 0} {}
    explicit Fsm_State_মান(ধ্রুবক V3_সংখ্যা& অঙ্ক)
        : m_চাবি{make_চাবি(অঙ্ক)}
        , m_অঙ্ক{অঙ্ক} {}

    ধ্রুবক string& চাবি() ধ্রুবক { ফেরত m_চাবি; }
    ধ্রুবক V3_সংখ্যা& অঙ্ক() ধ্রুবক { ফেরত m_অঙ্ক; }
    string ascii() ধ্রুবক { ফেরত m_অঙ্ক.ascii(সত্য, সত্য); }
    string warnText() ধ্রুবক {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        যদি (m_অঙ্ক.width() <= 32) ফেরত cvtToStr(m_অঙ্ক.toUInt());
        ফেরত ascii();
    }

    যৌক্তিক operator==(ধ্রুবক Fsm_State_মান& rhs) ধ্রুবক { ফেরত m_চাবি == rhs.m_চাবি; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
কাঠামো Fsm_State_মান_Hash final {
    আকার_t operator()(ধ্রুবক Fsm_State_মান& মান) ধ্রুবক {
        ফেরত std::hash<string>{}(মান.চাবি());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
কাঠামো FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
কাঠামো FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    যৌক্তিক active_নিচের = মিথ্যা;
};

শ্রেণী FsmResetArcDesc final {
    Fsm_State_মান m_to_মান;  // Encoded reset target state.
    Ast_গ্রন্থি* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_গ্রন্থি_রাশিমালা* m_valuep = nullptr;  // Expression that provided the reset value.

সর্বজনীন:
    FsmResetArcDesc() = পূর্বনির্ধারিত;
    FsmResetArcDesc(Fsm_State_মান to_মান, Ast_গ্রন্থি_Assign* nodep)
        : m_to_মান{to_মান}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_মান to_মান, Ast_গ্রন্থি* nodep, Ast_গ্রন্থি_রাশিমালা* valuep)
        : m_to_মান{to_মান}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_মান to_মান() ধ্রুবক { ফেরত m_to_মান; }
    Ast_গ্রন্থি* nodep() ধ্রুবক { ফেরত m_nodep; }
    Ast_গ্রন্থি_রাশিমালা* valuep() ধ্রুবক { ফেরত m_valuep; }
};

কাঠামো FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    যৌক্তিক has_Rst_Active_নিচের = মিথ্যা;
    যৌক্তিক rst_Active_নিচের = মিথ্যা;
};

স্থির যৌক্তিক fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ধ্রুবক string& portName,
                                                  যৌক্তিক& active_নিচের) {
    যৌক্তিক matched = মিথ্যা;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_গ্রন্থি_Var_Ref* ধ্রুবক vrefp = itemp->varrefp();
        যদি (!vrefp) ফেরত;
        যদি (vrefp->varp()->name() != portName) ফেরত;
        active_নিচের = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = সত্য;
    });
    ফেরত matched;
}

স্থির ধ্রুবক V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_গ্রন্থি_Module* ধ্রুবক modp = cellp->modp();
    ধ্রুবক string origName = modp->origName();
    যদি (ধ্রুবক V3Control::FsmRegisterWrapper* ধ্রুবক descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        ফেরত descp;
    }
    ফেরত V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

স্থির FsmWrapperRoles rolesFromDesc(ধ্রুবক V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_মান;
    ফেরত roles;
}

শ্রেণী FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_পরবর্তী_Vscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    যৌক্তিক m_hasResetCond = মিথ্যা;  // Whether the FSM had a modeled reset predicate.
    যৌক্তিক m_inclCond = মিথ্যা;  // Whether conditional/default arcs are kept explicitly.
    File_সারি* m_flp = nullptr;  // Representative source location.

সর্বজনীন:
    AstScope* scopep() ধ্রুবক { ফেরত m_scopep; }
    শূন্য scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ধ্রুবক { ফেরত m_alwaysp; }
    শূন্য alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ধ্রুবক { ফেরত m_stateVscp; }
    শূন্য stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ধ্রুবক { ফেরত m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    শূন্য sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* পরবর্তী_Vscp() ধ্রুবক { ফেরত m_পরবর্তী_Vscp; }
    শূন্য পরবর্তী_Vscp(AstVarScope* vscp) { m_পরবর্তী_Vscp = vscp; }
    ধ্রুবক std::vector<FsmSenDesc>& senses() ধ্রুবক { ফেরত m_senses; }
    std::vector<FsmSenDesc>& senses() { ফেরত m_senses; }
    ধ্রুবক FsmResetCondDesc& resetCond() ধ্রুবক { ফেরত m_resetCond; }
    FsmResetCondDesc& resetCond() { ফেরত m_resetCond; }
    ধ্রুবক std::vector<FsmResetArcDesc>& resetArcs() ধ্রুবক { ফেরত m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { ফেরত m_resetArcs; }
    যৌক্তিক hasResetCond() ধ্রুবক { ফেরত m_hasResetCond; }
    শূন্য hasResetCond(যৌক্তিক flag) { m_hasResetCond = flag; }
    যৌক্তিক inclCond() ধ্রুবক { ফেরত m_inclCond; }
    শূন্য inclCond(যৌক্তিক flag) { m_inclCond = flag; }
    File_সারি* fileline() ধ্রুবক { ফেরত m_flp; }
    শূন্য fileline(File_সারি* flp) { m_flp = flp; }
};

শ্রেণী FsmComboAlways final {
    AstScope* ধ্রুবক m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* ধ্রুবক m_alwaysp = nullptr;  // Candidate transition process.

সর্বজনীন:
    FsmComboAlways() = পূর্বনির্ধারিত;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ধ্রুবক { ফেরত m_scopep; }
    AstAlways* alwaysp() ধ্রুবক { ফেরত m_alwaysp; }
};

শ্রেণী FsmGraph;

শ্রেণী FsmVertex VL_NOT_FINAL : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

সর্বজনীন:
    গণনা শ্রেণী Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

ব্যক্তিগত:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_মান m_মান;  // Encoded state value for real state vertices.

সুরক্ষিত:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_মান মান) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_মান{মান} {}
    ~FsmVertex() override = পূর্বনির্ধারিত;

সর্বজনীন:
    Kind kind() ধ্রুবক { ফেরত m_kind; }
    যৌক্তিক isState() ধ্রুবক { ফেরত m_kind == Kind::STATE; }
    যৌক্তিক isResetAny() ধ্রুবক { ফেরত m_kind == Kind::RESET_ANY; }
    যৌক্তিক isDefaultAny() ধ্রুবক { ফেরত m_kind == Kind::DEFAULT_ANY; }
    ধ্রুবক string& label() ধ্রুবক { ফেরত m_label; }
    ধ্রুবক Fsm_State_মান& মান() ধ্রুবক { ফেরত m_মান; }

    string name() ধ্রুবক override VL_MT_SAFE { ফেরত m_label + "=" + m_মান.ascii(); }
};

শ্রেণী FsmStateVertex final : সর্বজনীন FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

সর্বজনীন:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_মান মান) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, মান} {}
    ~FsmStateVertex() override = পূর্বনির্ধারিত;

    string dotColor() ধ্রুবক override { ফেরত "lightblue"; }
    string dotShape() ধ্রুবক override { ফেরত "ellipse"; }
};

শ্রেণী FsmPseudoVertex final : সর্বজনীন FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

সর্বজনীন:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_মান{}} {}
    ~FsmPseudoVertex() override = পূর্বনির্ধারিত;

    string name() ধ্রুবক override VL_MT_SAFE { ফেরত label(); }
    string dotColor() ধ্রুবক override { ফেরত isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ধ্রুবক override { ফেরত "diamond"; }
};

শ্রেণী FsmArcEdge final : সর্বজনীন V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    যৌক্তিক m_isReset = মিথ্যা;  // Arc originates from the synthetic reset source.
    যৌক্তিক m_isCond = মিথ্যা;  // Arc came from a conditional next-state split.
    যৌক্তিক m_isDefault = মিথ্যা;  // Arc represents a case default source.
    File_সারি* m_flp = nullptr;  // Source location for emitted coverage metadata.

সর্বজনীন:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* শীর্ষ, যৌক্তিক isReset, যৌক্তিক isCond,
               যৌক্তিক isDefault, File_সারি* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, শীর্ষ, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = পূর্বনির্ধারিত;

    যৌক্তিক isReset() ধ্রুবক { ফেরত m_isReset; }
    যৌক্তিক isCond() ধ্রুবক { ফেরত m_isCond; }
    যৌক্তিক isDefault() ধ্রুবক { ফেরত m_isDefault; }
    File_সারি* fileline() ধ্রুবক { ফেরত m_flp; }

    string dotLabel() ধ্রুবক override {
        যদি (m_isReset) ফেরত "reset";
        যদি (m_isDefault) ফেরত "default";
        যদি (m_isCond) ফেরত "cond";
        ফেরত "";
    }
    string dotColor() ধ্রুবক override {
        যদি (m_isReset) ফেরত "darkgreen";
        যদি (m_isDefault) ফেরত "orange";
        যদি (m_isCond) ফেরত "blue";
        ফেরত "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
শ্রেণী FsmGraph final : সর্বজনীন V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    যৌক্তিক m_hasResetCond = মিথ্যা;  // Whether the detected FSM had a reset branch.
    যৌক্তিক m_inclCond = মিথ্যা;  // Whether conditional arcs should be kept explicitly.
    File_সারি* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_মান, FsmStateVertex*, Fsm_State_মান_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

সর্বজনীন:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{নতুন FsmPseudoVertex{এই, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{নতুন FsmPseudoVertex{এই, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ধ্রুবক { ফেরত m_scopep; }
    শূন্য scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ধ্রুবক { ফেরত m_stateAlwaysp; }
    শূন্য stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ধ্রুবক string& stateVarName() ধ্রুবক { ফেরত m_stateVarName; }
    শূন্য stateVarName(ধ্রুবক string& name) { m_stateVarName = name; }
    ধ্রুবক string& stateVarInternalName() ধ্রুবক { ফেরত m_stateVarInternalName; }
    শূন্য stateVarInternalName(ধ্রুবক string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() ধ্রুবক { ফেরত m_stateVarScopep; }
    শূন্য stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ধ্রুবক {
        ফেরত m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    শূন্য sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ধ্রুবক std::vector<FsmSenDesc>& senses() ধ্রুবক { ফেরত m_senses; }
    std::vector<FsmSenDesc>& senses() { ফেরত m_senses; }
    ধ্রুবক FsmResetCondDesc& resetCond() ধ্রুবক { ফেরত m_resetCond; }
    FsmResetCondDesc& resetCond() { ফেরত m_resetCond; }
    যৌক্তিক hasResetCond() ধ্রুবক { ফেরত m_hasResetCond; }
    শূন্য hasResetCond(যৌক্তিক flag) { m_hasResetCond = flag; }
    যৌক্তিক inclCond() ধ্রুবক { ফেরত m_inclCond; }
    শূন্য inclCond(যৌক্তিক flag) { m_inclCond = flag; }
    File_সারি* fileline() ধ্রুবক { ফেরত m_flp; }
    শূন্য fileline(File_সারি* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_মান মান) VL_MT_DISABLED {
        FsmStateVertex* ধ্রুবক vertexp = নতুন FsmStateVertex{এই, label, মান};
        m_stateVertices.emplace(মান, vertexp);
        ফেরত vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { ফেরত m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { ফেরত m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_মান from_মান, Fsm_State_মান to_মান, যৌক্তিক isReset, যৌক্তিক isCond,
                       যৌক্তিক isDefault, File_সারি* flp) VL_MT_DISABLED {
        FsmStateVertex* ধ্রুবক শীর্ষ = m_stateVertices.at(to_মান);
        FsmVertex* fromp = nullptr;
        যদি (isReset) {
            fromp = resetAnyVertex();
        } অন্যথা যদি (isDefault) {
            fromp = defaultAnyVertex();
        } অন্যথা {
            fromp = m_stateVertices.at(from_মান);
        }
        ফেরত নতুন FsmArcEdge{এই, fromp, শীর্ষ, isReset, isCond, isDefault, flp};
    }

    string name() ধ্রুবক VL_MT_SAFE {
        ফেরত "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(আকার_t সূচক) ধ্রুবক {
        string tag = stateVarInternalName();
        জন্য (অক্ষর& ch : tag) {
            যদি (!std::isalnum(static_cast<unsigned অক্ষর>(ch))) ch = '_';
        }
        ফেরত "fsm_" + cvtToStr(সূচক) + "_" + tag;
    }
};

কাঠামো DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

কাঠামো FsmCaseCandidate final {
    Ast_গ্রন্থি* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
কাঠামো FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;  // Compared constant value expression
    Fsm_State_মান মান;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
কাঠামো FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_গ্রন্থি* stmtsp = nullptr;  // Branch body
    Ast_গ্রন্থি_রাশিমালা* valuep = nullptr;  // Source state value expression
    Fsm_State_মান from_মান;  // Encoded source state value
    যৌক্তিক has_শীর্ষ_Guard = মিথ্যা;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
কাঠামো FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_গ্রন্থি* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<ধ্রুবক AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<ধ্রুবক AstCell*, FsmCellPortMap>;

কাঠামো StateConstLabel final {
    string text;
    যৌক্তিক fromParam = মিথ্যা;
    আকার_t state_সূচক = 0;
};

কাঠামো FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_মান>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_মান, StateConstLabel, Fsm_State_মান_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    যৌক্তিক enumBacked = মিথ্যা;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
শ্রেণী FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<ধ্রুবক AstVarScope*, আকার_t> m_fsm_সূচক;

সর্বজনীন:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ধ্রুবক std::map<ধ্রুবক AstVarScope*, আকার_t>::const_iterator it = m_fsm_সূচক.find(stateVscp);
        যদি (it != m_fsm_সূচক.end()) ফেরত m_fsms.at(it->second);
        ধ্রুবক আকার_t সূচক = m_fsms.আকার_2();
        m_fsm_সূচক.emplace(stateVscp, সূচক);
        m_fsms.emplace_back();
        ফেরত m_fsms.back();
    }
    ধ্রুবক std::vector<DetectedFsm>& fsms() ধ্রুবক { ফেরত m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
শ্রেণী FsmDetectVisitor final : সর্বজনীন VNVisitor {
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
    std::unordered_map<ধ্রুবক AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<ধ্রুবক AstVarScope*> m_ambiguousStateAliases;
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
        ফেরত dtypep->skipRefToEnump();
    }

    স্থির string candidateConflictContext(Ast_গ্রন্থি* laterNodep,
                                           ধ্রুবক FsmCaseCandidate& firstCand) {
        ফেরত '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    স্থির যৌক্তিক rejectFsmWrapperCell(AstCell* cellp, ধ্রুবক string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        ফেরত মিথ্যা;
    }

    স্থির যৌক্তিক simple_Param_State_মান(AstCell* cellp, ধ্রুবক string& name, Fsm_State_মান& মান,
                                      Ast_গ্রন্থি_রাশিমালা*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        জন্য (Ast_গ্রন্থি* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ধ্রুবক varp = VN_CAST(stmtp, Var);
            যদি (!varp || !varp->isParam() || varp->name() != name) চালু;
            valuepr = VN_AS(varp->valuep(), গ্রন্থি_রাশিমালা);
            ফেরত const_মান_Status(valuepr, মান) == Const_মান_Status::OK;
        }
        ফেরত মিথ্যা;
    }

    স্থির যৌক্তিক childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        যদি (!vscp->varp()->isIO()) ফেরত মিথ্যা;
        AstScope* ধ্রুবক scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        যদি (scopep->aboveScopep() != parentScopep) ফেরত মিথ্যা;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        ফেরত সত্য;
    }

    স্থির AstVarScope* simpleAssignVarScope(Ast_গ্রন্থি_রাশিমালা* exprp) {
        AstVarRef* ধ্রুবক vrefp = VN_CAST(exprp, VarRef);
        ফেরত vrefp ? vrefp->varScopep() : nullptr;
    }

    শূন্য addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        ধ্রুবক std::pair<AstScope*, AstCell*> item{scopep, cellp};
        যদি (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            ফেরত;
        }
        m_wrapperCells.emplace_back(item);
    }

    শূন্য collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ধ্রুবক lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* ধ্রুবক rhsVscp = simpleAssignVarScope(nodep->rhsp());
        যদি (!lhsVscp || !rhsVscp) ফেরত;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        যদি (childPortInScope(lhsVscp, m_scopep, cellp)) {
            যদি (!fsmRegisterWrapperDesc(cellp)) ফেরত;
            UASSERT_OBJ(lhsVscp->varp()->is_প্রবেশ(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } অন্যথা যদি (childPortInScope(rhsVscp, m_scopep, cellp)) {
            যদি (!fsmRegisterWrapperDesc(cellp)) ফেরত;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ধ্রুবক string& portName) ধ্রুবক {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ধ্রুবক FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        ধ্রুবক FsmCellPortMap::const_iterator portIt = ports.find(portName);
        ফেরত portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, ধ্রুবক string& portName) ধ্রুবক {
        ধ্রুবক FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        ধ্রুবক FsmCellPortMap::const_iterator portIt = ports.find(portName);
        ফেরত portIt == ports.end() ? nullptr : portIt->second;
    }

    শ্রেণী RegisterAlwaysAnalyzer final {
        AstScope* ধ্রুবক m_scopep;  // Scope containing the always block(s) being analyzed

    সর্বজনীন:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_গ্রন্থি_রাশিমালা*>>
        oneBlockCandidates(AstAlways* alwaysp) ধ্রুবক {
            std::vector<std::pair<AstCase*, Ast_গ্রন্থি_রাশিমালা*>> candidates;
            Ast_গ্রন্থি* ধ্রুবক stmtsp = alwaysp->stmtsp();
            যদি (AstIf* ধ্রুবক firstIfp = VN_CAST(stmtsp, If)) {
                যদি (AstCase* ধ্রুবক casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            জন্য (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                যদি (AstCase* ধ্রুবক casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            ফেরত candidates;
        }

        std::vector<std::pair<AstIf*, Ast_গ্রন্থি_রাশিমালা*>>
        oneBlockIfCandidates(AstAlways* alwaysp) ধ্রুবক {
            std::vector<std::pair<AstIf*, Ast_গ্রন্থি_রাশিমালা*>> candidates;
            Ast_গ্রন্থি* ধ্রুবক stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            যদি (AstIf* ধ্রুবক firstIfp = VN_CAST(stmtsp, If)) {
                যদি (AstIf* ধ্রুবক chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            জন্য (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                যদি (AstIf* ধ্রুবক ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            ফেরত candidates;
        }

        যৌক্তিক matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) ধ্রুবক {
            ফেরত FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        শূন্য buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_গ্রন্থি_রাশিমালা* resetCondp,
                                    FsmRegisterCandidate& reg) ধ্রুবক {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.পরবর্তী_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_বৃক্ষ(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ধ্রুবক firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            যদি (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                ধ্রুবক ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                যদি (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } অন্যথা যদি (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_মান reset_মান;
                    Ast_গ্রন্থি* ধ্রুবক thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_গ্রন্থি_Assign* ধ্রুবক resetAssp = FsmDetectVisitor::direct_Const_State_Assign_গ্রন্থি(
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

    যৌক্তিক matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) ধ্রুবক {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* ধ্রুবক পরবর্তী_Vscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* ধ্রুবক stateVscp = roleVarScope(cellp, roles.qPort);
        যদি (!পরবর্তী_Vscp || !stateVscp) {
            ফেরত rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ধ্রুবক clkVscp = roleVarScope(cellp, roles.clkPort);
        যদি (!clkVscp) {
            ফেরত rejectFsmWrapperCell(
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
            যৌক্তিক inferred_Active_নিচের = মিথ্যা;
            যদি (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_নিচের)) {
                roles.has_Rst_Active_নিচের = সত্য;
                roles.rst_Active_নিচের = inferred_Active_নিচের;
            }
        }

        Ast_গ্রন্থি_রাশিমালা* resetValuep = nullptr;
        Fsm_State_মান reset_মান;
        ধ্রুবক যৌক্তিক has_Reset_মান
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
        cand.পরবর্তী_Vscp(পরবর্তী_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        ফেরত সত্য;
    }

    শ্রেণী ComboAlwaysAnalyzer final {
    সর্বজনীন:
        কাঠামো ComboMatch final {
            ধ্রুবক FsmRegisterCandidate* matchedp = nullptr;
            Ast_গ্রন্থি* warnNodep = nullptr;
        };

    ব্যক্তিগত:
        ধ্রুবক std::vector<FsmRegisterCandidate>& m_registerCandidates;

    সর্বজনীন:
        explicit ComboAlwaysAnalyzer(ধ্রুবক std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_গ্রন্থি* stmtsp, AstCase* casep) ধ্রুবক {
            ComboMatch match;
            AstVarRef* ধ্রুবক selp = VN_CAST(casep->exprp(), VarRef);
            যদি (!selp) ফেরত match;
            জন্য (ধ্রুবক FsmRegisterCandidate& reg : m_registerCandidates) {
                যদি (selp->varScopep() == reg.পরবর্তী_Vscp()) {
                    যদি (!FsmDetectVisitor::has_Canonical_পরবর্তী_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.পরবর্তী_Vscp())) {
                        চালু;
                    }
                } অন্যথা যদি (selp->varScopep() != reg.stateVscp()) {
                    চালু;
                }
                Ast_গ্রন্থি* ধ্রুবক warnNodep = FsmDetectVisitor::case_Supported_Transition_গ্রন্থি(
                    casep, reg.পরবর্তী_Vscp(), reg.inclCond());
                যদি (!warnNodep) চালু;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            ফেরত match;
        }

        ComboMatch matchIfChain(Ast_গ্রন্থি* stmtsp, ধ্রুবক FsmIfChainCandidate& chain) ধ্রুবক {
            ComboMatch match;
            জন্য (ধ্রুবক FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                যদি (chain.compareVscp == reg.পরবর্তী_Vscp()) {
                    যদি (!FsmDetectVisitor::has_Canonical_পরবর্তী_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.পরবর্তী_Vscp())) {
                        চালু;
                    }
                } অন্যথা যদি (chain.compareVscp != reg.stateVscp()) {
                    চালু;
                }
                Ast_গ্রন্থি* ধ্রুবক warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_গ্রন্থি(chain, reg.পরবর্তী_Vscp());
                যদি (!warnNodep) চালু;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            ফেরত match;
        }

        যৌক্তিক shouldWarnUnsupported(Ast_গ্রন্থি* stmtsp, AstCase* casep) ধ্রুবক {
            ধ্রুবক AstVarRef* ধ্রুবক selp = VN_CAST(casep->exprp(), VarRef);
            যদি (!selp) ফেরত মিথ্যা;

            ফেরত std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](ধ্রুবক FsmRegisterCandidate& reg) -> যৌক্তিক {
                    ধ্রুবক যৌক্তিক matches_পরবর্তী = selp->varScopep() == reg.পরবর্তী_Vscp();
                    ধ্রুবক যৌক্তিক matchesState = selp->varScopep() == reg.stateVscp();

                    যদি (!matches_পরবর্তী && !matchesState) ফেরত মিথ্যা;
                    যদি (matches_পরবর্তী
                        && !FsmDetectVisitor::has_Canonical_পরবর্তী_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.পরবর্তী_Vscp())) {
                        ফেরত মিথ্যা;
                    }
                    ফেরত FsmDetectVisitor::case_Supported_Transition_গ্রন্থি(casep, reg.পরবর্তী_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    স্থির যৌক্তিক isSimpleResetCond(Ast_গ্রন্থি_রাশিমালা* condp) { ফেরত VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    স্থির FsmResetCondDesc describeResetCond(Ast_গ্রন্থি_রাশিমালা* condp) {
        FsmResetCondDesc desc;
        যদি (AstVarRef* ধ্রুবক vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        ফেরত desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    স্থির std::vector<FsmSenDesc> describe_Sen_বৃক্ষ(Ast_Sen_বৃক্ষ* sentreep) {
        std::vector<FsmSenDesc> senses;
        জন্য (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_গ্রন্থি_Var_Ref* ধ্রুবক vrefp = itemp->varrefp();
            যদি (!vrefp) চালু;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ঠেলো_back(desc);
        }
        ফেরত senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    স্থির যৌক্তিক isIgnorableStmt(Ast_গ্রন্থি* nodep) { ফেরত VN_IS(nodep, CoverInc); }

    স্থির Ast_গ্রন্থি* skipLeadingIgnorableStmt(Ast_গ্রন্থি* nodep) {
        যখন (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        ফেরত nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    স্থির Ast_গ্রন্থি* singleMeaningfulStmt(Ast_গ্রন্থি* stmtp) {
        Ast_গ্রন্থি* resultp = nullptr;
        জন্য (Ast_গ্রন্থি* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            যদি (isIgnorableStmt(nodep)) চালু;
            যদি (resultp) ফেরত nullptr;
            resultp = nodep;
        }
        ফেরত resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    স্থির Ast_গ্রন্থি* singleMeaningfulBranch(Ast_গ্রন্থি* branchp) {
        যদি (!branchp) ফেরত nullptr;
        ফেরত branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    স্থির যৌক্তিক isPlainComboSentree(ধ্রুবক Ast_Sen_বৃক্ষ* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        জন্য (ধ্রুবক AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            যদি (senp->edgeType() == VEdgeType::ET_CHANGED) চালু;
            ফেরত মিথ্যা;
        }
        ফেরত সত্য;
    }

    শূন্য warnUnsupportedComboAlways(ধ্রুবক FsmComboAlways& combo) {
        ধ্রুবক ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_গ্রন্থি* ধ্রুবক stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        যৌক্তিক warned = মিথ্যা;
        জন্য (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ধ্রুবক casep = VN_CAST(nodep, Case);
            যদি (!casep) চালু;
            যদি (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = সত্য;
            }
            যদি (warned) বিরতি;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    স্থির Ast_গ্রন্থি_Assign* directStateAssign(Ast_গ্রন্থি* stmtp, AstVarScope* stateVscp) {
        Ast_গ্রন্থি* ধ্রুবক nodep = singleMeaningfulBranch(stmtp);
        যদি (!nodep) ফেরত nullptr;
        Ast_গ্রন্থি_Assign* ধ্রুবক assp = VN_CAST(nodep, গ্রন্থি_Assign);
        যদি (!assp) ফেরত nullptr;
        AstVarRef* ধ্রুবক vrefp = VN_CAST(assp->lhsp(), VarRef);
        যদি (!vrefp || vrefp->varScopep() != stateVscp) ফেরত nullptr;
        ফেরত assp;
    }

    স্থির Ast_গ্রন্থি_Assign* গ্রন্থি_State_Var_Assign(Ast_গ্রন্থি* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_গ্রন্থি_Assign* ধ্রুবক assp = VN_CAST(nodep, গ্রন্থি_Assign);
        যদি (!assp) ফেরত nullptr;
        AstVarRef* ধ্রুবক lhsp = VN_CAST(assp->lhsp()->baseFromp(সত্য), VarRef);
        AstVarRef* ধ্রুবক rhsp = VN_CAST(assp->rhsp(), VarRef);
        যদি (!rhsp || !lhsp) ফেরত nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        ফেরত assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    স্থির Ast_গ্রন্থি_Assign* directCondStateVarAssign(Ast_গ্রন্থি* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_গ্রন্থি_রাশিমালা*& condp,
                                                   Fsm_State_মান& reset_মান) {
        Ast_গ্রন্থি_Assign* ধ্রুবক assp = VN_CAST(nodep, গ্রন্থি_Assign);
        যদি (!assp) ফেরত nullptr;
        AstVarRef* ধ্রুবক lhsp = VN_CAST(assp->lhsp()->baseFromp(সত্য), VarRef);
        AstCond* ধ্রুবক rhsp = VN_CAST(assp->rhsp(), Cond);
        যদি (!rhsp || !lhsp) ফেরত nullptr;
        AstVarRef* ধ্রুবক elsep = VN_CAST(rhsp->elsep(), VarRef);
        যদি (!elsep) ফেরত nullptr;
        যদি (const_মান_Status(rhsp->thenp(), reset_মান) != Const_মান_Status::OK) ফেরত nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        ফেরত assp;
    }

    স্থির Ast_গ্রন্থি_Assign* direct_Const_State_Assign_গ্রন্থি(Ast_গ্রন্থি* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_মান& মান) {
        Ast_গ্রন্থি_Assign* ধ্রুবক assp = VN_CAST(nodep, গ্রন্থি_Assign);
        যদি (!assp) ফেরত nullptr;
        AstVarRef* ধ্রুবক lhsp = VN_CAST(assp->lhsp()->baseFromp(সত্য), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        যদি (const_মান_Status(assp->rhsp(), মান) != Const_মান_Status::OK) ফেরত nullptr;
        stateVscp = lhsp->varScopep();
        ফেরত assp;
    }

    গণনা শ্রেণী ResetAssignStatus : uint8_t {
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
        যদি (!nodep) ফেরত ResetAssignStatus::EMPTY;
        জন্য (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_মান মান;
            Ast_গ্রন্থি_Assign* ধ্রুবক assp = direct_Const_State_Assign_গ্রন্থি(nodep, assignStateVscp, মান);
            যদি (!assp) ফেরত ResetAssignStatus::NONE;
            যদি (!stateVscp) stateVscp = assignStateVscp;
            যদি (assignStateVscp != stateVscp) ফেরত ResetAssignStatus::NONE;
            যদি (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                ফেরত ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(মান, assp);
            যদি (!nodep->nextp()) ফেরত ResetAssignStatus::SINGLE;
        }
    }

    স্থির যৌক্তিক has_Canonical_পরবর্তী_State_Default_Before_Case(Ast_গ্রন্থি* stmtsp, Ast_গ্রন্থি* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* পরবর্তী_Vscp) {
        Ast_গ্রন্থি* ধ্রুবক bodyp = skipLeadingIgnorableStmt(stmtsp);
        যৌক্তিক sawCanonicalDefault = মিথ্যা;
        জন্য (Ast_গ্রন্থি* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            যদি (nodep == targetp) ফেরত sawCanonicalDefault;
            যদি (Ast_গ্রন্থি_Assign* ধ্রুবক assp = VN_CAST(nodep, গ্রন্থি_Assign)) {
                AstVarRef* ধ্রুবক lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ধ্রুবক rhsp = VN_CAST(assp->rhsp(), VarRef);
                যদি (!lhsp || lhsp->varScopep() != পরবর্তী_Vscp) চালু;
                যদি (sawCanonicalDefault) {
                    ধ্রুবক string পরবর্তী_Name = পরবর্তী_Vscp->varp()->prettyNameQ();
                    ধ্রুবক string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + পরবর্তী_Name
                                     + ") when the canonical " + পরবর্তী_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    ফেরত মিথ্যা;
                }
                যদি (!rhsp || rhsp->varScopep() != stateVscp) ফেরত মিথ্যা;
                sawCanonicalDefault = সত্য;
            }
        }
    }

    স্থির যৌক্তিক ifStateConstAssign(Ast_গ্রন্থি* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_মান& then_মান, Fsm_State_মান& else_মান) {
        AstIf* ধ্রুবক ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        যদি (!ifp || !ifp->elsesp()) ফেরত মিথ্যা;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_গ্রন্থি* ধ্রুবক thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_গ্রন্থি* ধ্রুবক elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        যদি (!thenNodep || !elseNodep) ফেরত মিথ্যা;
        যদি (!direct_Const_State_Assign_গ্রন্থি(thenNodep, thenVscp, then_মান)) ফেরত মিথ্যা;
        যদি (!direct_Const_State_Assign_গ্রন্থি(elseNodep, elseVscp, else_মান)) ফেরত মিথ্যা;
        যদি (thenVscp == stateVscp && elseVscp == stateVscp) ফেরত সত্য;
        যদি (thenVscp != elseVscp) ফেরত মিথ্যা;
        Ast_গ্রন্থি* ধ্রুবক followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_গ্রন্থি* ধ্রুবক finalNodep = singleMeaningfulBranch(followp);
        যদি (!finalNodep) ফেরত মিথ্যা;
        যদি (!গ্রন্থি_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) ফেরত মিথ্যা;
        যদি (finalStateVscp != stateVscp) ফেরত মিথ্যা;
        যদি (finalFromVscp != thenVscp) ফেরত মিথ্যা;
        ফেরত সত্য;
    }

    স্থির যৌক্তিক directStateCondConstAssign(Ast_গ্রন্থি* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_মান& then_মান, Fsm_State_মান& else_মান) {
        Ast_গ্রন্থি_Assign* ধ্রুবক assp = directStateAssign(stmtp, stateVscp);
        যদি (!assp) ফেরত মিথ্যা;
        AstCond* ধ্রুবক condp = VN_CAST(assp->rhsp(), Cond);
        যদি (!condp) ফেরত মিথ্যা;
        ফেরত const_মান_Status(condp->thenp(), then_মান) == Const_মান_Status::OK
               && const_মান_Status(condp->elsep(), else_মান) == Const_মান_Status::OK;
    }

    স্থির Ast_গ্রন্থি* case_Item_Supported_Arc_গ্রন্থি(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             যৌক্তিক inclCond) {
        যদি (itemp->isDefault()) {
            যদি (!inclCond) ফেরত nullptr;
        }
        Ast_গ্রন্থি_Assign* ধ্রুবক assp = directStateAssign(itemp->stmtsp(), stateVscp);
        যদি (assp) {
            Fsm_State_মান to_মান;
            যদি (const_মান_Status(assp->rhsp(), to_মান) == Const_মান_Status::OK) ফেরত assp;
        }
        Fsm_State_মান then_মান;
        Fsm_State_মান else_মান;
        যদি (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_মান, else_মান)) {
            ফেরত assp;
        }
        যদি (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_মান, else_মান)) {
            ফেরত singleMeaningfulBranch(itemp->stmtsp());
        }
        ফেরত nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    স্থির Ast_গ্রন্থি* case_Supported_Transition_গ্রন্থি(AstCase* casep, AstVarScope* stateVscp,
                                                যৌক্তিক inclCond) {
        জন্য (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            যদি (Ast_গ্রন্থি* ধ্রুবক nodep = case_Item_Supported_Arc_গ্রন্থি(itemp, stateVscp, inclCond))
                ফেরত nodep;
        }
        ফেরত nullptr;
    }

    স্থির Ast_গ্রন্থি* case_Item_Supported_Arc_গ্রন্থি_Like(Ast_গ্রন্থি* stmtsp, AstVarScope* stateVscp) {
        যদি (Ast_গ্রন্থি_Assign* ধ্রুবক assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_মান to_মান;
            যদি (const_মান_Status(assp->rhsp(), to_মান) == Const_মান_Status::OK) ফেরত assp;
            Fsm_State_মান then_মান;
            Fsm_State_মান else_মান;
            যদি (directStateCondConstAssign(stmtsp, stateVscp, then_মান, else_মান)) ফেরত assp;
        }
        ফেরত nullptr;
    }

    স্থির Ast_গ্রন্থি* if_Chain_Supported_Transition_গ্রন্থি(ধ্রুবক FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        জন্য (আকার_t i = 0; i < chain.branches.আকার_2(); ++i) {
            ধ্রুবক FsmIfBranch& branch = chain.branches[i];
            Ast_গ্রন্থি* ধ্রুবক nodep = case_Item_Supported_Arc_গ্রন্থি_Like(branch.stmtsp, stateVscp);
            যদি (!nodep) ফেরত nullptr;
        }
        ফেরত chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    স্থির string label_For_মান(
        ধ্রুবক std::unordered_map<Fsm_State_মান, StateConstLabel, Fsm_State_মান_Hash>& labels,
        ধ্রুবক Fsm_State_মান& মান) {
        ফেরত labels.at(মান).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    গণনা শ্রেণী Const_মান_Status : uint8_t { OK, NOT_CONST, XZ };

    স্থির Const_মান_Status const_মান_Status(Ast_গ্রন্থি_রাশিমালা* exprp, Fsm_State_মান& মান) {
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(exprp, Const);
        যদি (!constp) ফেরত Const_মান_Status::NOT_CONST;
        ধ্রুবক V3_সংখ্যা& অঙ্ক = constp->অঙ্ক();
        যদি (অঙ্ক.isAnyXZ()) ফেরত Const_মান_Status::XZ;
        মান = Fsm_State_মান{অঙ্ক};
        ফেরত Const_মান_Status::OK;
    }

    স্থির যৌক্তিক pureStateComparisonNoAlias(Ast_গ্রন্থি_রাশিমালা* exprp, FsmStateComparison& cmp) {
        AstEq* ধ্রুবক eqp = VN_CAST(exprp, Eq);
        যদি (!eqp) ফেরত মিথ্যা;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_গ্রন্থি_রাশিমালা* valuep = eqp->rhsp();
        যদি (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(সত্য), VarRef);
            যদি (!vrefp) { ফেরত মিথ্যা; }
            valuep = eqp->lhsp();
        }

        Fsm_State_মান মান;
        যদি (const_মান_Status(valuep, মান) != Const_মান_Status::OK) ফেরত মিথ্যা;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.মান = মান;
        ফেরত সত্য;
    }

    স্থির যৌক্তিক pureStateComparison(Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুবক FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        যদি (pureStateComparisonNoAlias(exprp, cmp)) ফেরত সত্য;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        যদি (AstVarRef* ধ্রুবক vrefp = VN_CAST(exprp, VarRef)) {
            ধ্রুবক FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            যদি (it == aliases.end()) ফেরত মিথ্যা;
            cmp = it->second;
            ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }

    স্থির যৌক্তিক supported_শীর্ষ_Level_Guard(Ast_গ্রন্থি_রাশিমালা* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        যদি (VN_IS(exprp, Or)) ফেরত মিথ্যা;
        যদি (VN_IS(exprp, RedAnd)) ফেরত মিথ্যা;
        যদি (VN_IS(exprp, RedOr)) ফেরত মিথ্যা;
        যদি (VN_IS(exprp, RedXor)) ফেরত মিথ্যা;
        ফেরত সত্য;
    }

    স্থির যৌক্তিক resolveIfPredicate(Ast_গ্রন্থি_রাশিমালা* exprp, ধ্রুবক FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, যৌক্তিক& hasGuard) {
        std::vector<Ast_গ্রন্থি_রাশিমালা*> terms;
        std::vector<Ast_গ্রন্থি_রাশিমালা*> pending;
        pending.ঠেলো_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        যখন (!pending.empty()) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক nodep = pending.back();
            pending.বের_করো_back();
            যদি (AstAnd* ধ্রুবক andp = VN_CAST(nodep, And)) {
                pending.ঠেলো_back(andp->rhsp());
                pending.ঠেলো_back(andp->lhsp());
            } অন্যথা {
                terms.ঠেলো_back(nodep);
            }
        }

        যৌক্তিক sawComparison = মিথ্যা;
        জন্য (আকার_t i = 0; i < terms.আকার_2(); ++i) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক termp = terms[i];
            FsmStateComparison termCmp;
            যদি (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                যদি (sawComparison) ফেরত মিথ্যা;
                cmp = termCmp;
                sawComparison = সত্য;
                চালু;
            }
            যদি (!supported_শীর্ষ_Level_Guard(termp)) ফেরত মিথ্যা;
            hasGuard = সত্য;
        }
        ফেরত sawComparison;
    }

    স্থির শূন্য addAlias(FsmAliasMap& aliases, std::unordered_set<ধ্রুবক AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ধ্রুবক FsmStateComparison& cmp) {
        যদি (ambiguous.find(aliasVscp) != ambiguous.end()) ফেরত;
        ধ্রুবক FsmAliasMap::iterator it = aliases.find(aliasVscp);
        যদি (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            ফেরত;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        যদি (it->second.stateVscp == cmp.stateVscp && it->second.মান == cmp.মান) ফেরত;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        ফেরত;
    }

    স্থির শূন্য collectAliasFromAssign(Ast_গ্রন্থি_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<ধ্রুবক AstVarScope*>& ambiguous) {
        AstVarRef* ধ্রুবক lhsp = VN_CAST(assp->lhsp(), VarRef);
        যদি (!lhsp) ফেরত;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        যদি (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) ফেরত;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_গ্রন্থি* stmtsp, Ast_গ্রন্থি* limitp) ধ্রুবক {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<ধ্রুবক AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        জন্য (Ast_গ্রন্থি* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            যদি (Ast_গ্রন্থি_Assign* ধ্রুবক assp = VN_CAST(nodep, গ্রন্থি_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        জন্য (ধ্রুবক AstVarScope* ধ্রুবক vscp : ambiguous) aliases.erase(vscp);
        ফেরত aliases;
    }

    স্থির যৌক্তিক collectIfChain(AstIf* ifp, ধ্রুবক FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_মানসমূহ;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        যখন (সত্য) {
            FsmStateComparison cmp;
            যৌক্তিক hasGuard = মিথ্যা;
            যদি (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) ফেরত মিথ্যা;
            যদি (chain.compareVscp && chain.compareVscp != cmp.stateVscp) ফেরত মিথ্যা;
            যদি (!seen_মানসমূহ.ঢোকাও(cmp.মান.চাবি()).second) ফেরত মিথ্যা;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ঠেলো_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.মান, hasGuard});

            Ast_গ্রন্থি* ধ্রুবক elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            যদি (!elseNodep) বিরতি;
            যদি (AstIf* ধ্রুবক elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                চালু;
            }
            chain.defaultStmtsp = elseNodep;
            বিরতি;
        }
        ফেরত chain.branches.আকার_2() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    স্থির যৌক্তিক validate_Known_State_মান(Ast_গ্রন্থি* nodep, ধ্রুবক FsmStateSpace& stateSpace,
                                        ধ্রুবক Fsm_State_মান& মান, ধ্রুবক string& role) {
        যদি (stateSpace.labels.find(মান) != stateSpace.labels.end()) ফেরত সত্য;
        যদি (stateSpace.enumBacked) {
            ধ্রুবক string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + মান.warnText()
                                        + " is not present in the declared enum");
            ফেরত মিথ্যা;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + মান.warnText()
                                    + " is not present in the inferred state space");
        ফেরত মিথ্যা;
    }

    স্থির StateConstLabel stateLabelForConst(AstConst* constp) {
        ধ্রুবক string name = constp->origParamName();
        যদি (!name.empty()) ফেরত StateConstLabel{Ast_গ্রন্থি::prettyName(name), সত্য, 0};
        ফেরত StateConstLabel{constp->name(), মিথ্যা, 0};
    }

    স্থির শূন্য updateStateLabel(FsmStateSpace& stateSpace, ধ্রুবক Fsm_State_মান& মান,
                                 ধ্রুবক StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(মান).state_সূচক).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    স্থির যৌক্তিক matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        যদি (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) ফেরত মিথ্যা;

        Ast_গ্রন্থি* ধ্রুবক stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_গ্রন্থি* ধ্রুবক nodep = singleMeaningfulStmt(stmtsp);
        যদি (!nodep) ফেরত মিথ্যা;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* পরবর্তী_Vscp = nullptr;
        যদি (AstIf* ধ্রুবক ifp = VN_CAST(nodep, If)) {
            যদি (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) ফেরত মিথ্যা;
            AstVarScope* resetStateVscp = nullptr;
            ধ্রুবক ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ধ্রুবক যৌক্তিক emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            যদি (emptyResetBranch) {
                cand.resetArcs().clear();
            } অন্যথা যদি (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_মান reset_মান;
                Ast_গ্রন্থি* ধ্রুবক thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_গ্রন্থি_Assign* ধ্রুবক resetAssp
                    = direct_Const_State_Assign_গ্রন্থি(thenNodep, resetStateVscp, reset_মান);
                যদি (!resetAssp) ফেরত মিথ্যা;
                cand.resetArcs().emplace_back(reset_মান, resetAssp);
            } অন্যথা যদি (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_গ্রন্থি* ধ্রুবক elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            যদি (!গ্রন্থি_State_Var_Assign(elseNodep, stateVscp, পরবর্তী_Vscp)) ফেরত মিথ্যা;
            যদি (!emptyResetBranch && resetStateVscp != stateVscp) ফেরত মিথ্যা;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } অন্যথা {
            Ast_গ্রন্থি_রাশিমালা* resetCondp = nullptr;
            Fsm_State_মান reset_মান;
            // Note the active low 'reset ? next : CONST' form is not accepted
            যদি (Ast_গ্রন্থি_Assign* ধ্রুবক assp
                = directCondStateVarAssign(nodep, stateVscp, পরবর্তী_Vscp, resetCondp, reset_মান)) {
                cand.resetArcs().emplace_back(reset_মান, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } অন্যথা যদি (!গ্রন্থি_State_Var_Assign(nodep, stateVscp, পরবর্তী_Vscp)) {
                ফেরত মিথ্যা;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.পরবর্তী_Vscp(পরবর্তী_Vscp);
        cand.senses() = describe_Sen_বৃক্ষ(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        ফেরত সত্য;
    }

    স্থির যৌক্তিক add_মান_To_State_Space(Ast_গ্রন্থি* nodep, FsmStateSpace& stateSpace,
                                     ধ্রুবক Fsm_State_মান& মান, StateConstLabel label) {
        ধ্রুবক auto labelIt = stateSpace.labels.find(মান);
        যদি (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            যদি (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + মান.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                ফেরত মিথ্যা;
            }
            যদি (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, মান, label);
            }
        } অন্যথা {
            StateConstLabel storedLabel = label;
            storedLabel.state_সূচক = stateSpace.states.আকার_2();
            stateSpace.states.emplace_back(label.text, মান);
            stateSpace.labels.emplace(মান, storedLabel);
        }
        ফেরত সত্য;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    স্থির যৌক্তিক add_রাশিমালা_To_State_Space(Ast_গ্রন্থি_রাশিমালা* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_মান মান;
        ধ্রুবক Const_মান_Status status = const_মান_Status(valuep, মান);
        যদি (status != Const_মান_Status::OK) {
            যদি (status == Const_মান_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            ফেরত মিথ্যা;
        }
        AstConst* ধ্রুবক constp = VN_AS(valuep, Const);
        ফেরত add_মান_To_State_Space(valuep, stateSpace, মান, stateLabelForConst(constp));
    }

    স্থির যৌক্তিক add_Optional_Target_রাশিমালা_To_State_Space(Ast_গ্রন্থি_রাশিমালা* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_মান মান;
        ধ্রুবক Const_মান_Status status = const_মান_Status(valuep, মান);
        যদি (status != Const_মান_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            ফেরত মিথ্যা;
        }
        AstConst* ধ্রুবক constp = VN_AS(valuep, Const);
        ফেরত add_মান_To_State_Space(valuep, stateSpace, মান, stateLabelForConst(constp));
    }

    স্থির শূন্য addResetTargetsToStateSpace(ধ্রুবক std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        জন্য (ধ্রুবক FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_মান().ascii(), মিথ্যা, 0};
            যদি (AstConst* ধ্রুবক constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_মান_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_মান(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    স্থির যৌক্তিক collectDeclaredStateSpace(Ast_গ্রন্থি* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, যৌক্তিক& needs_Source_মানসমূহ) {
        AstVar* ধ্রুবক stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        যদি (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ধ্রুবক যৌক্তিক forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        যদি (enump) {
            stateSpace.enumBacked = সত্য;
            জন্য (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ধ্রুবক AstConst* ধ্রুবক constp = VN_AS(itemp->valuep(), Const);
                ধ্রুবক Fsm_State_মান মান{constp->অঙ্ক()};
                ধ্রুবক আকার_t state_সূচক = stateSpace.states.আকার_2();
                stateSpace.states.emplace_back(itemp->name(), মান);
                stateSpace.labels.emplace(মান,
                                          StateConstLabel{itemp->name(), মিথ্যা, state_সূচক});
            }
            ফেরত stateSpace.states.আকার_2() >= 2;
        }

        যদি (forced) {
            needs_Source_মানসমূহ = সত্য;
            ফেরত সত্য;
        }

        needs_Source_মানসমূহ = সত্য;
        ফেরত সত্য;
    }

    template <typename T_ValuepVisitor>
    স্থির যৌক্তিক collect_State_Space_From_মানসমূহ(Ast_গ্রন্থি* warnNodep, AstVarScope* stateVscp,
                                            ধ্রুবক std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ধ্রুবক T_ValuepVisitor& visitValueps) {
        যৌক্তিক needs_Source_মানসমূহ = মিথ্যা;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        যদি (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_মানসমূহ)) {
            ফেরত মিথ্যা;
        }
        যদি (!needs_Source_মানসমূহ) ফেরত সত্য;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        যদি (!visitValueps(
                [&](Ast_গ্রন্থি_রাশিমালা* valuep) { ফেরত add_রাশিমালা_To_State_Space(valuep, stateSpace); })) {
            ফেরত মিথ্যা;
        }
        ফেরত stateSpace.states.আকার_2() >= 2;
    }

    স্থির যৌক্তিক collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  ধ্রুবক std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ফেরত collect_State_Space_From_মানসমূহ(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](ধ্রুবক auto& visitValuep) {
                জন্য (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    যদি (!itemp->isDefault()) {
                        জন্য (Ast_গ্রন্থি_রাশিমালা* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), গ্রন্থি_রাশিমালা)) {
                            যদি (!visitValuep(condp)) ফেরত মিথ্যা;
                        }
                    }
                    যদি (Ast_গ্রন্থি_Assign* ধ্রুবক assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_মান then_মান;
                        Fsm_State_মান else_মান;
                        AstCond* ধ্রুবক condp = VN_CAST(assp->rhsp(), Cond);
                        যদি (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_মান,
                                                          else_মান)) {
                            যদি (!visitValuep(condp->thenp())) ফেরত মিথ্যা;
                            যদি (!visitValuep(condp->elsep())) ফেরত মিথ্যা;
                        } অন্যথা যদি (!add_Optional_Target_রাশিমালা_To_State_Space(assp->rhsp(), stateSpace)) {
                            ফেরত মিথ্যা;
                        }
                    }
                }
                ফেরত সত্য;
            });
    }

    স্থির যৌক্তিক collectStateSpace(ধ্রুবক FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  ধ্রুবক std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        ফেরত collect_State_Space_From_মানসমূহ(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](ধ্রুবক auto& visitValuep) {
                জন্য (ধ্রুবক FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_গ্রন্থি_Assign* ধ্রুবক assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_মান then_মান;
                    Fsm_State_মান else_মান;
                    AstCond* ধ্রুবক condp = VN_CAST(assp->rhsp(), Cond);
                    যদি (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_মান, else_মান),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        যদি (!visitValuep(condp->thenp())) ফেরত মিথ্যা;
                        যদি (!visitValuep(condp->elsep())) ফেরত মিথ্যা;
                    } অন্যথা যদি (!add_Optional_Target_রাশিমালা_To_State_Space(assp->rhsp(), stateSpace)) {
                        ফেরত মিথ্যা;
                    }
                }
                ফেরত সত্য;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    স্থির যৌক্তিক emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 ধ্রুবক FsmStateSpace& stateSpace, যৌক্তিক inclCond) {
        std::vector<std::pair<string, Fsm_State_মান>> froms;
        যদি (itemp->isDefault()) {
            যদি (!inclCond) ফেরত মিথ্যা;
            froms.emplace_back("default", Fsm_State_মান{});
        } অন্যথা {
            জন্য (Ast_গ্রন্থি_রাশিমালা* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), গ্রন্থি_রাশিমালা)) {
                Fsm_State_মান মান;
                যদি (const_মান_Status(condp, মান) != Const_মান_Status::OK) চালু;
                যদি (!validate_Known_State_মান(condp, stateSpace, মান, "source")) ফেরত সত্য;
                froms.emplace_back(label_For_মান(stateSpace.labels, মান), মান);
            }
            যদি (froms.empty()) ফেরত মিথ্যা;
        }

        যদি (Ast_গ্রন্থি_Assign* ধ্রুবক assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_মান to_মান;
            ধ্রুবক Const_মান_Status status = const_মান_Status(assp->rhsp(), to_মান);
            যদি (status == Const_মান_Status::OK) {
                যদি (!validate_Known_State_মান(assp, stateSpace, to_মান, "target")) ফেরত সত্য;
                জন্য (ধ্রুবক std::pair<string, Fsm_State_মান>& from : froms) {
                    graph.addArc(from.second, to_মান, মিথ্যা, মিথ্যা, itemp->isDefault(),
                                 assp->fileline());
                }
                ফেরত সত্য;
            }
        }

        Fsm_State_মান then_মান;
        Fsm_State_মান else_মান;
        যদি (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_মান, else_মান)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_মান, else_মান)) {
            যদি (!validate_Known_State_মান(itemp->stmtsp(), stateSpace, then_মান, "target"))
                ফেরত সত্য;
            যদি (!validate_Known_State_মান(itemp->stmtsp(), stateSpace, else_মান, "target"))
                ফেরত সত্য;
            জন্য (ধ্রুবক Fsm_State_মান& branch_মান : {then_মান, else_মান}) {
                জন্য (ধ্রুবক std::pair<string, Fsm_State_মান>& from : froms) {
                    graph.addArc(from.second, branch_মান, মিথ্যা, সত্য, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            ফেরত সত্য;
        }

        ফেরত মিথ্যা;
    }

    স্থির শূন্য emitStmtArcsFrom(FsmGraph& graph, Ast_গ্রন্থি* stmtsp, AstVarScope* stateVscp,
                                 ধ্রুবক FsmStateSpace& stateSpace, Fsm_State_মান from_মান,
                                 যৌক্তিক isDefault, যৌক্তিক forceCond) {
        Ast_গ্রন্থি_Assign* ধ্রুবক assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_মান to_মান;
        ধ্রুবক Const_মান_Status status = const_মান_Status(assp->rhsp(), to_মান);
        যদি (status == Const_মান_Status::OK) {
            যদি (!validate_Known_State_মান(assp, stateSpace, to_মান, "target")) ফেরত;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_মান, to_মান, মিথ্যা, forceCond, isDefault, assp->fileline());
            ফেরত;
        }

        Fsm_State_মান then_মান;
        Fsm_State_মান else_মান;
        ধ্রুবক যৌক্তিক condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_মান, else_মান);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        যদি (!validate_Known_State_মান(stmtsp, stateSpace, then_মান, "target")) ফেরত;
        যদি (!validate_Known_State_মান(stmtsp, stateSpace, else_মান, "target")) ফেরত;
        জন্য (ধ্রুবক Fsm_State_মান& branch_মান : {then_মান, else_মান}) {
            graph.addArc(from_মান, branch_মান, মিথ্যা, সত্য, isDefault, stmtsp->fileline());
        }
    }

    স্থির শূন্য emitIfChainArcs(FsmGraph& graph, ধ্রুবক FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ধ্রুবক FsmStateSpace& stateSpace) {
        জন্য (আকার_t i = 0; i < chain.branches.আকার_2(); ++i) {
            ধ্রুবক FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            যদি (!validate_Known_State_মান(branch.ifp, stateSpace, branch.from_মান, "source"))
                ফেরত;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_মান, মিথ্যা,
                             branch.has_শীর্ষ_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    স্থির শূন্য addResetArcs(FsmGraph& graph, ধ্রুবক std::vector<FsmResetArcDesc>& resetArcs,
                             ধ্রুবক FsmStateSpace& stateSpace) {
        জন্য (ধ্রুবক FsmResetArcDesc& resetArc : resetArcs) {
            যদি (!validate_Known_State_মান(resetArc.nodep(), stateSpace, resetArc.to_মান(),
                                         "target"))
                চালু;
            graph.addArc(Fsm_State_মান{}, resetArc.to_মান(), সত্য, মিথ্যা, মিথ্যা,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    শূন্য processCase(AstCase* casep, AstVarScope* assignVscp, ধ্রুবক FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ধ্রুবক stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        যদি (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) ফেরত;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        যদি (!entry.graphp) {
            entry.graphp.reset(নতুন FsmGraph{});
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
            জন্য (ধ্রুবক std::pair<string, Fsm_State_মান>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        জন্য (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    শূন্য processIfChain(ধ্রুবক FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        ধ্রুবক FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ধ্রুবক stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        যদি (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) ফেরত;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(নতুন FsmGraph{});
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
        জন্য (ধ্রুবক std::pair<string, Fsm_State_মান>& state : stateSpace.states) {
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
    শূন্য processOneBlockAlways(ধ্রুবক FsmComboAlways& oneBlock) {
        ধ্রুবক RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* ধ্রুবক alwaysp = oneBlock.alwaysp();
        যদি (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) ফেরত;
        ধ্রুবক std::vector<std::pair<AstCase*, Ast_গ্রন্থি_রাশিমালা*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        জন্য (ধ্রুবক std::pair<AstCase*, Ast_গ্রন্থি_রাশিমালা*>& cand : candidates) {
            AstVarRef* ধ্রুবক selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* ধ্রুবক vscp = selp ? selp->varScopep() : nullptr;
            যদি (!vscp) চালু;
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

        যদি (firstCand.stateVscp) ফেরত;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ধ্রুবক std::vector<std::pair<AstIf*, Ast_গ্রন্থি_রাশিমালা*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        জন্য (ধ্রুবক std::pair<AstIf*, Ast_গ্রন্থি_রাশিমালা*>& cand : ifCandidates) {
            ধ্রুবক FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            যদি (!collectIfChain(cand.first, aliases, chain)) চালু;
            AstVarScope* ধ্রুবক vscp = chain.compareVscp;
            যদি (!if_Chain_Supported_Transition_গ্রন্থি(chain, vscp)) চালু;
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
    স্থির শূন্য warnComboSameAlways(Ast_গ্রন্থি* warnNodep, ধ্রুবক FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    শূন্য processComboAlways(ধ্রুবক FsmComboAlways& combo) {
        ধ্রুবক ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_গ্রন্থি* ধ্রুবক stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        জন্য (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ধ্রুবক casep = VN_CAST(nodep, Case);
            যদি (!casep) চালু;
            ধ্রুবক ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            ধ্রুবক FsmRegisterCandidate* ধ্রুবক matchedp = match.matchedp;
            Ast_গ্রন্থি* ধ্রুবক matchedWarnNodep = match.warnNodep;
            যদি (!matchedp) চালু;
            যদি (!firstCand.stateVscp) {
                ধ্রুবক auto ঢোকাও_Pair = m_comboPaired.emplace(
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
                    চালু;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->পরবর্তী_Vscp(), *matchedp);
                চালু;
            }
            যদি (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                চালু;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        যদি (firstCand.stateVscp) ফেরত;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        জন্য (Ast_গ্রন্থি* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ধ্রুবক ifp = VN_CAST(nodep, If);
            যদি (!ifp) চালু;
            FsmIfChainCandidate chain;
            ধ্রুবক FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            যদি (!collectIfChain(ifp, aliases, chain)) চালু;
            ধ্রুবক ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            ধ্রুবক FsmRegisterCandidate* ধ্রুবক matchedp = match.matchedp;
            Ast_গ্রন্থি* ধ্রুবক matchedWarnNodep = match.warnNodep;
            যদি (!matchedp) চালু;
            যদি (!firstCand.stateVscp) {
                ধ্রুবক std::pair<std::unordered_map<ধ্রুবক AstVarScope*, FsmCaseCandidate>::iterator,
                                যৌক্তিক>
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
                    চালু;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->পরবর্তী_Vscp(), *matchedp);
                চালু;
            }
            যদি (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                চালু;
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
            ফেরত;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        ধ্রুবক RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        যদি (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* ধ্রুবক stateVscp = reg.stateVscp();
            ধ্রুবক যৌক্তিক found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](ধ্রুবক FsmRegisterCandidate& existing) {
                                  ফেরত existing.stateVscp() == stateVscp;
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

সর্বজনীন:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        জন্য (ধ্রুবক std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            যদি (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        জন্য (ধ্রুবক FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        জন্য (ধ্রুবক FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        জন্য (ধ্রুবক FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
শ্রেণী FsmCoverIncBuilder final {
    ধ্রুবক FsmGraph& m_graph;  // Fsm graph
    std::vector<ধ্রুবক FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ধ্রুবক FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    ধ্রুবক FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    আকার_t m_edges_গুনতি = 0;  // Total count of edges
    AstVarScope* ধ্রুবক m_পূর্ব_Vscp;  // Previous state of fsm
    File_সারি* ধ্রুবক m_flp;  // Fileline pointer to use across the builder
    ধ্রুবক std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_গ্রন্থি_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_গ্রন্থি_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    যৌক্তিক m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ধ্রুবক uint64_t মান) {
        V3_সংখ্যা অঙ্ক{static_cast<Ast_গ্রন্থি*>(nullptr), m_পূর্ব_Vscp->width()};
        অঙ্ক.setQuad(মান);
        অঙ্ক.isSigned(m_পূর্ব_Vscp->dtypep()->isSigned());
        ফেরত নতুন AstConst{m_flp, অঙ্ক};
    }

    std::string quadToBitStirngLabel(ধ্রুবক uint64_t val) {
        পূর্ণসংখ্যা width = m_পূর্ব_Vscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string ফলাফল = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        যখন (--width >= 0) ফলাফল.ঠেলো_back("01"[(val >> width) & 1]);
        ফেরত ফলাফল;
    }

    শূন্য initExistingStates() {
        জন্য (ধ্রুবক V3GraphVertex& vtx : m_graph.vertices()) {
            ধ্রুবক FsmVertex* ধ্রুবক vertexp = vtx.as<FsmVertex>();
            m_edges_গুনতি += vertexp->inEdges().আকার_2();
            যদি (ধ্রুবক FsmStateVertex* ধ্রুবক statep = vertexp->cast<FsmStateVertex>()) {
                m_states.ঠেলো_back(statep);
            } অন্যথা যদি (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } অন্যথা /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ধ্রুবক পূর্ণসংখ্যা width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.আকার_2() == (static_cast<আকার_t>(1) << width);
    }

    শূন্য initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ধ্রুবক uint64_t combinations_গুনতি = static_cast<আকার_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        যৌক্তিক undefRolled = মিথ্যা;
        auto rollUndefs = [এই, combinations_গুনতি] {
            জন্য (uint64_t i = 0; i < combinations_গুনতি; ++i) {
                ধ্রুবক auto ঢোকাও = m_undefinedStates.ঢোকাও(i);
                যদি (!ঢোকাও.second) m_undefinedStates.erase(ঢোকাও.first);
            }
        };
        জন্য (ধ্রুবক FsmStateVertex* ধ্রুবক statep : m_states) {
            ধ্রুবক V3_সংখ্যা& অঙ্ক = statep->মান().অঙ্ক();
            ধ্রুবক uint64_t মান = অঙ্ক.toUQuad();
            যদি (!m_isComplete) {
                যদি (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(মান) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } অন্যথা {
                    UASSERT_OBJ(m_undefinedStates.ঢোকাও(মান).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    যদি (m_undefinedStates.আকার_2() * 2 >= combinations_গুনতি) {
                        undefRolled = সত্য;
                        rollUndefs();
                    }
                }
            }
        }
        যদি (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_গ্রন্থি_রাশিমালা* buildResetCond() ধ্রুবক {
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক refp
            = নতুন AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        ফেরত m_graph.resetCond().active_নিচের
                   ? static_cast<Ast_গ্রন্থি_রাশিমালা*>(নতুন AstLogNot{m_flp, refp})
                   : refp;
    }

    আকার_t get_Arc_গুনতি_With_Expansion(ধ্রুবক VFsmExpandType expand) ধ্রুবক {
        আকার_t arc_গুনতি_With_Expansion = ~0ULL;
        নির্বাচন (expand) {
        ক্ষেত্র VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            বিরতি;
        }
        ক্ষেত্র VFsmExpandType::AUTO: {
            arc_গুনতি_With_Expansion = m_edges_গুনতি;
            যদি (m_defaultVertexp) {
                UASSERT_OBJ(m_states.আকার_2() >= m_defaultVertexp->outEdges().আকার_2(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_গুনতি_With_Expansion -= m_defaultVertexp->outEdges().আকার_2();
                জন্য (ধ্রুবক FsmStateVertex* ধ্রুবক statep : m_states) {
                    arc_গুনতি_With_Expansion += static_cast<আকার_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().আকার_2();
                }
            }
            যদি (m_resetVertexp) {
                UASSERT_OBJ(m_states.আকার_2() >= m_resetVertexp->outEdges().আকার_2(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_গুনতি_With_Expansion += m_states.আকার_2() - m_resetVertexp->outEdges().আকার_2();
            }
            বিরতি;
        }
        ক্ষেত্র VFsmExpandType::AUTO_EXPAND: {
            arc_গুনতি_With_Expansion = m_states.আকার_2() * m_states.আকার_2() - m_states.আকার_2();
            বিরতি;
        }
        ক্ষেত্র VFsmExpandType::FULL: {
            ধ্রুবক পূর্ণসংখ্যা width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            যদি (width < 32) {
                ধ্রুবক uint64_t combinations_গুনতি = static_cast<আকার_t>(1) << width;
                arc_গুনতি_With_Expansion = combinations_গুনতি * combinations_গুনতি - combinations_গুনতি;
            }
            বিরতি;
        }
        }
        ফেরত arc_গুনতি_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ধ্রুবক std::string& label,
                                           ধ্রুবক std::string& fsmTag = "") {
        AstCoverOtherDecl* ধ্রুবক declp
            = নতুন AstCoverOtherDecl{m_flp,
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
        m_declsp = Ast_গ্রন্থি::add_পরবর্তী(m_declsp, declp);
        ফেরত declp;
    }

    Ast_গ্রন্থি_Stmt* buildCoverIncStmtp(ধ্রুবক std::string& fromStateLabel,
                                    ধ্রুবক std::string& toStateLabel,
                                    ধ্রুবক std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ধ্রুবক declp = নতুন AstCoverOtherDecl{
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
        m_declsp = Ast_গ্রন্থি::add_পরবর্তী(m_declsp, declp);
        ফেরত নতুন AstCoverInc{m_flp, declp};
    }
    Ast_গ্রন্থি_Stmt* buildExpandedTransitionHandler(ধ্রুবক std::string& toStateLabel,
                                                ধ্রুবক যৌক্তিক considerUndefs,
                                                ধ্রুবক যৌক্তিক expandOnlyWithTerminalVerticies,
                                                ধ্রুবক FsmStateVertex* ধ্রুবক toStatep = nullptr) {
        AstCase* ধ্রুবক arcCasep = নতুন AstCase{
            m_flp, VCaseType::CT_CASE, নতুন AstVarRef{m_flp, m_পূর্ব_Vscp, VAccess::READ}, nullptr};
        std::unordered_set<ধ্রুবক FsmStateVertex*> handledSoureces;
        যদি (toStatep) {
            জন্য (ধ্রুবক V3GraphEdge& edge : toStatep->inEdges()) {
                যদি (ধ্রুবক FsmStateVertex* ধ্রুবক fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    যদি (!handledSoureces.emplace(fromp).second) চালু;
                    arcCasep->addItemsp(নতুন AstCaseItem{
                        m_flp, makeConst(fromp->মান().অঙ্ক().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        জন্য (ধ্রুবক FsmStateVertex* ধ্রুবক fromp : m_states) {
            যদি (handledSoureces.find(fromp) != handledSoureces.end()) চালু;
            arcCasep->addItemsp(নতুন AstCaseItem{
                m_flp, makeConst(fromp->মান().অঙ্ক().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        যদি (!m_isComplete) {
            যদি (considerUndefs) {
                জন্য (ধ্রুবক uint64_t from : m_undefinedStates) {
                    ধ্রুবক std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        নতুন AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } অন্যথা {
                arcCasep->addItemsp(নতুন AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        ফেরত arcCasep;
    }
    Ast_গ্রন্থি_Stmt* buildUnxpandedTransitionHandler(ধ্রুবক FsmStateVertex* ধ্রুবক toStatep) {
        AstCase* ধ্রুবক arcCasep = নতুন AstCase{
            m_flp, VCaseType::CT_CASE, নতুন AstVarRef{m_flp, m_পূর্ব_Vscp, VAccess::READ}, nullptr};
        জন্য (ধ্রুবক V3GraphEdge& edge : toStatep->inEdges()) {
            ধ্রুবক FsmArcEdge* ধ্রুবক arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ধ্রুবক FsmStateVertex* ধ্রুবক fromp = arcp->fromp()->as<FsmStateVertex>();
            যদি (fromp != toStatep) {
                arcCasep->addItemsp(
                    নতুন AstCaseItem{m_flp, makeConst(fromp->মান().অঙ্ক().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(নতুন AstCaseItem{m_flp, nullptr, nullptr});
        ফেরত arcCasep;
    }

সর্বজনীন:
    FsmCoverIncBuilder(ধ্রুবক FsmGraph& graph, AstVarScope* ধ্রুবক পূর্ব_Vscp)
        : m_graph{graph}
        , m_পূর্ব_Vscp{পূর্ব_Vscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* ধ্রুবক sampleVscp = graph.sampleVarScopep();
        ধ্রুবক VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ধ্রুবক আকার_t arc_গুনতি_With_Expansion = get_Arc_গুনতি_With_Expansion(expand);
            যদি (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_গুনতি_With_Expansion == 0ULL
                            || arc_গুনতি_With_Expansion
                                   > static_cast<আকার_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_গুনতি_With_Expansion);
                ফেরত;
            }
        }

        AstCase* ধ্রুবক arcCasep
            = নতুন AstCase{m_flp, VCaseType::CT_CASE,
                          নতুন AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* ধ্রুবক initialStateCasep
            = নতুন AstCase{m_flp, VCaseType::CT_CASE,
                          নতুন AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        ধ্রুবক যৌক্তিক expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<ধ্রুবক FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<ধ্রুবক FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        যদি (!expandAll) {
            যদি (m_defaultVertexp) {
                জন্য (ধ্রুবক V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.শীর্ষ()->as<FsmStateVertex>());
                }
            }
            যদি (m_resetVertexp) {
                জন্য (ধ্রুবক V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.শীর্ষ()->as<FsmStateVertex>());
                }
            }
        }
        ধ্রুবক যৌক্তিক isFullExpansion = expand == VFsmExpandType::FULL;
        যদি (isFullExpansion) initUndefinedStates();
        জন্য (ধ্রুবক FsmStateVertex* ধ্রুবক statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ধ্রুবক declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                নতুন AstCaseItem{m_flp, makeConst(statep->মান().অঙ্ক().toUQuad()),
                                নতুন AstCoverInc{m_flp, declp, সত্য}});
            ধ্রুবক যৌক্তিক expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            ধ্রুবক যৌক্তিক defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(নতুন AstCaseItem{
                m_flp, makeConst(statep->মান().অঙ্ক().toUQuad()),
                Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি_Stmt>(
                    নতুন AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        যদি (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            যদি (isFullExpansion) {
                জন্য (ধ্রুবক uint64_t মান : m_undefinedStates) {
                    ধ্রুবক std::string& label = quadToBitStirngLabel(মান);
                    AstCoverOtherDecl* ধ্রুবক declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(নতুন AstCaseItem{
                        m_flp, makeConst(মান), নতুন AstCoverInc{m_flp, declp, সত্য}});
                    arcCasep->addItemsp(
                        নতুন AstCaseItem{m_flp, makeConst(মান),
                                        Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি_Stmt>(
                                            নতুন AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, সত্য, মিথ্যা))});
                }
            } অন্যথা {
                // No expansion
                arcCasep->addItemsp(নতুন AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(নতুন AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = পূর্বনির্ধারিত;
    Ast_গ্রন্থি_Cover_Decl* getCoverDeclsp() ধ্রুবক { ফেরত m_declsp; }
    Ast_গ্রন্থি_Stmt* getCoverIncp() ধ্রুবক { ফেরত m_coverIncp; }
    Ast_গ্রন্থি_Stmt* getInitialCoverIncp() ধ্রুবক { ফেরত m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
শ্রেণী FsmLowerVisitor final {
    // STATE - across all visitors
    ধ্রুবক FsmState& m_state;
    V3SharedTmps m_পূর্ব_Tmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    স্থির Ast_Sen_বৃক্ষ* build_Sen_বৃক্ষ(File_সারি* flp, ধ্রুবক std::vector<FsmSenDesc>& senses) {
        Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = নতুন Ast_Sen_বৃক্ষ{flp, nullptr};
        জন্য (ধ্রুবক FsmSenDesc& sense : senses) {
            AstSenItem* ধ্রুবক senItemp
                = নতুন AstSenItem{flp, VEdgeType{sense.edgeType},
                                 নতুন AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        ফেরত sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    শূন্য buildOne(ধ্রুবক FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().আকার_2() << endl);
        AstAlways* ধ্রুবক alwaysp = graph.stateAlwaysp();
        AstScope* ধ্রুবক scopep = graph.scopep();
        AstVarScope* ধ্রুবক stateVscp = graph.stateVarScopep();
        AstVarScope* ধ্রুবক sampleVscp = graph.sampleVarScopep();
        File_সারি* ধ্রুবক flp = graph.fileline();
        Ast_গ্রন্থি_Module* ধ্রুবক modp = scopep->modp();
        AstVarScope* ধ্রুবক পূর্ব_Vscp
            = m_পূর্ব_Tmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(সত্য);
        sampleVscp->optimizeLifePost(সত্য);
        পূর্ব_Vscp->optimizeLifePost(সত্য);

        AstActive* ধ্রুবক initActivep
            = নতুন AstActive{flp, "fsm-coverage-init",
                            নতুন Ast_Sen_বৃক্ষ{flp, নতুন AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_বৃক্ষ_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(নতুন AstInitialStatic{
            flp, নতুন AstAssign{flp, নতুন AstVarRef{flp, পূর্ব_Vscp, VAccess::WRITE},
                               নতুন AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* ধ্রুবক covPostp = নতুন AstAlwaysPost{flp};
        যৌক্তিক update_পূর্ব_After_Post = মিথ্যা;
        যদি (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(নতুন AstAssign{flp, নতুন AstVarRef{flp, পূর্ব_Vscp, VAccess::WRITE},
                                                 নতুন AstVarRef{flp, sampleVscp, VAccess::READ}});
        } অন্যথা {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            update_পূর্ব_After_Post = সত্য;
            পূর্ব_Vscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ধ্রুবক FsmCoverIncBuilder coverIncBuilder{graph, পূর্ব_Vscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                নতুন AstIf{flp,
                          নতুন AstNeq{flp, নতুন AstVarRef{flp, পূর্ব_Vscp, VAccess::READ},
                                     নতুন AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        যদি (update_পূর্ব_After_Post) {
            covPostp->addStmtsp(নতুন AstAssign{flp, নতুন AstVarRef{flp, পূর্ব_Vscp, VAccess::WRITE},
                                              নতুন AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_বৃক্ষ* ধ্রুবক sentreep = build_Sen_বৃক্ষ(flp, graph.senses());
        AstActive* ধ্রুবক activep = নতুন AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_বৃক্ষ_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

সর্বজনীন:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(ধ্রুবক FsmState& state)
        : m_state{state} {
        জন্য (ধ্রুবক DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
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
        আকার_t সূচক = 0;
        জন্য (ধ্রুবক DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(সূচক++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_বৃক্ষ("fsm-detect", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
