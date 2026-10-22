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

#அந்தர்பாவ "V3PchAstNoMT.h"

#அந்தர்பாவ "V3FsmDetect.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3SharedTmps.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cctype>
#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
வகுப்பு Fsm_State_மதிப்பு final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_சாவி;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_எண் m_இலக்கம்;  // Original folded value, preserving width for lowered comparisons

    நிலையான string make_சாவி(மாறிலி V3_எண்& இலக்கம்) {
        V3_எண் சாவி_இலக்கம் = இலக்கம்;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        சாவி_இலக்கம்.isSigned(பொய்);
        திரும்பு cvtToStr(சாவி_இலக்கம்.width()) + ":" + சாவி_இலக்கம்.ascii(உண்மை, உண்மை);
    }

பொது:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_மதிப்பு()
        : m_சாவி{"1:1'h0"}
        , m_இலக்கம்{static_cast<Ast_முனை*>(nullptr), 1, 0} {}
    explicit Fsm_State_மதிப்பு(மாறிலி V3_எண்& இலக்கம்)
        : m_சாவி{make_சாவி(இலக்கம்)}
        , m_இலக்கம்{இலக்கம்} {}

    மாறிலி string& சாவி() மாறிலி { திரும்பு m_சாவி; }
    மாறிலி V3_எண்& இலக்கம்() மாறிலி { திரும்பு m_இலக்கம்; }
    string ascii() மாறிலி { திரும்பு m_இலக்கம்.ascii(உண்மை, உண்மை); }
    string warnText() மாறிலி {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        எனில் (m_இலக்கம்.width() <= 32) திரும்பு cvtToStr(m_இலக்கம்.toUInt());
        திரும்பு ascii();
    }

    தருக்க operator==(மாறிலி Fsm_State_மதிப்பு& rhs) மாறிலி { திரும்பு m_சாவி == rhs.m_சாவி; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
கட்டமைப்பு Fsm_State_மதிப்பு_Hash final {
    size_t operator()(மாறிலி Fsm_State_மதிப்பு& மதிப்பு) மாறிலி {
        திரும்பு std::hash<string>{}(மதிப்பு.சாவி());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
கட்டமைப்பு FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
கட்டமைப்பு FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    தருக்க active_கீழ் = பொய்;
};

வகுப்பு FsmResetArcDesc final {
    Fsm_State_மதிப்பு m_to_மதிப்பு;  // Encoded reset target state.
    Ast_முனை* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_முனை_கோவை* m_valuep = nullptr;  // Expression that provided the reset value.

பொது:
    FsmResetArcDesc() = இயல்பு;
    FsmResetArcDesc(Fsm_State_மதிப்பு to_மதிப்பு, Ast_முனை_Assign* nodep)
        : m_to_மதிப்பு{to_மதிப்பு}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_மதிப்பு to_மதிப்பு, Ast_முனை* nodep, Ast_முனை_கோவை* valuep)
        : m_to_மதிப்பு{to_மதிப்பு}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_மதிப்பு to_மதிப்பு() மாறிலி { திரும்பு m_to_மதிப்பு; }
    Ast_முனை* nodep() மாறிலி { திரும்பு m_nodep; }
    Ast_முனை_கோவை* valuep() மாறிலி { திரும்பு m_valuep; }
};

கட்டமைப்பு FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    தருக்க has_Rst_Active_கீழ் = பொய்;
    தருக்க rst_Active_கீழ் = பொய்;
};

நிலையான தருக்க fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, மாறிலி string& portName,
                                                  தருக்க& active_கீழ்) {
    தருக்க matched = பொய்;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_முனை_Var_Ref* மாறிலி vrefp = itemp->varrefp();
        எனில் (!vrefp) திரும்பு;
        எனில் (vrefp->varp()->name() != portName) திரும்பு;
        active_கீழ் = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = உண்மை;
    });
    திரும்பு matched;
}

நிலையான மாறிலி V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_முனை_Module* மாறிலி modp = cellp->modp();
    மாறிலி string origName = modp->origName();
    எனில் (மாறிலி V3Control::FsmRegisterWrapper* மாறிலி descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        திரும்பு descp;
    }
    திரும்பு V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

நிலையான FsmWrapperRoles rolesFromDesc(மாறிலி V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_மதிப்பு;
    திரும்பு roles;
}

வகுப்பு FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    தருக்க m_hasResetCond = பொய்;  // Whether the FSM had a modeled reset predicate.
    தருக்க m_inclCond = பொய்;  // Whether conditional/default arcs are kept explicitly.
    File_வரி* m_flp = nullptr;  // Representative source location.

பொது:
    AstScope* scopep() மாறிலி { திரும்பு m_scopep; }
    வெறுமை scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() மாறிலி { திரும்பு m_alwaysp; }
    வெறுமை alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() மாறிலி { திரும்பு m_stateVscp; }
    வெறுமை stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() மாறிலி { திரும்பு m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    வெறுமை sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() மாறிலி { திரும்பு m_nextVscp; }
    வெறுமை nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    மாறிலி std::vector<FsmSenDesc>& senses() மாறிலி { திரும்பு m_senses; }
    std::vector<FsmSenDesc>& senses() { திரும்பு m_senses; }
    மாறிலி FsmResetCondDesc& resetCond() மாறிலி { திரும்பு m_resetCond; }
    FsmResetCondDesc& resetCond() { திரும்பு m_resetCond; }
    மாறிலி std::vector<FsmResetArcDesc>& resetArcs() மாறிலி { திரும்பு m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { திரும்பு m_resetArcs; }
    தருக்க hasResetCond() மாறிலி { திரும்பு m_hasResetCond; }
    வெறுமை hasResetCond(தருக்க flag) { m_hasResetCond = flag; }
    தருக்க inclCond() மாறிலி { திரும்பு m_inclCond; }
    வெறுமை inclCond(தருக்க flag) { m_inclCond = flag; }
    File_வரி* fileline() மாறிலி { திரும்பு m_flp; }
    வெறுமை fileline(File_வரி* flp) { m_flp = flp; }
};

வகுப்பு FsmComboAlways final {
    AstScope* மாறிலி m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* மாறிலி m_alwaysp = nullptr;  // Candidate transition process.

பொது:
    FsmComboAlways() = இயல்பு;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() மாறிலி { திரும்பு m_scopep; }
    AstAlways* alwaysp() மாறிலி { திரும்பு m_alwaysp; }
};

வகுப்பு FsmGraph;

வகுப்பு FsmVertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

பொது:
    எண்ணிக்கை வகுப்பு Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

தனிப்பட்ட:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_மதிப்பு m_மதிப்பு;  // Encoded state value for real state vertices.

பாதுகாக்கப்பட்ட:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_மதிப்பு மதிப்பு) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_மதிப்பு{மதிப்பு} {}
    ~FsmVertex() override = இயல்பு;

பொது:
    Kind kind() மாறிலி { திரும்பு m_kind; }
    தருக்க isState() மாறிலி { திரும்பு m_kind == Kind::STATE; }
    தருக்க isResetAny() மாறிலி { திரும்பு m_kind == Kind::RESET_ANY; }
    தருக்க isDefaultAny() மாறிலி { திரும்பு m_kind == Kind::DEFAULT_ANY; }
    மாறிலி string& label() மாறிலி { திரும்பு m_label; }
    மாறிலி Fsm_State_மதிப்பு& மதிப்பு() மாறிலி { திரும்பு m_மதிப்பு; }

    string name() மாறிலி override VL_MT_SAFE { திரும்பு m_label + "=" + m_மதிப்பு.ascii(); }
};

வகுப்பு FsmStateVertex final : பொது FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

பொது:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_மதிப்பு மதிப்பு) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, மதிப்பு} {}
    ~FsmStateVertex() override = இயல்பு;

    string dotColor() மாறிலி override { திரும்பு "lightblue"; }
    string dotShape() மாறிலி override { திரும்பு "ellipse"; }
};

வகுப்பு FsmPseudoVertex final : பொது FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

பொது:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_மதிப்பு{}} {}
    ~FsmPseudoVertex() override = இயல்பு;

    string name() மாறிலி override VL_MT_SAFE { திரும்பு label(); }
    string dotColor() மாறிலி override { திரும்பு isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() மாறிலி override { திரும்பு "diamond"; }
};

வகுப்பு FsmArcEdge final : பொது V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    தருக்க m_isReset = பொய்;  // Arc originates from the synthetic reset source.
    தருக்க m_isCond = பொய்;  // Arc came from a conditional next-state split.
    தருக்க m_isDefault = பொய்;  // Arc represents a case default source.
    File_வரி* m_flp = nullptr;  // Source location for emitted coverage metadata.

பொது:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* உச்சி, தருக்க isReset, தருக்க isCond,
               தருக்க isDefault, File_வரி* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, உச்சி, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = இயல்பு;

    தருக்க isReset() மாறிலி { திரும்பு m_isReset; }
    தருக்க isCond() மாறிலி { திரும்பு m_isCond; }
    தருக்க isDefault() மாறிலி { திரும்பு m_isDefault; }
    File_வரி* fileline() மாறிலி { திரும்பு m_flp; }

    string dotLabel() மாறிலி override {
        எனில் (m_isReset) திரும்பு "reset";
        எனில் (m_isDefault) திரும்பு "default";
        எனில் (m_isCond) திரும்பு "cond";
        திரும்பு "";
    }
    string dotColor() மாறிலி override {
        எனில் (m_isReset) திரும்பு "darkgreen";
        எனில் (m_isDefault) திரும்பு "orange";
        எனில் (m_isCond) திரும்பு "blue";
        திரும்பு "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
வகுப்பு FsmGraph final : பொது V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    தருக்க m_hasResetCond = பொய்;  // Whether the detected FSM had a reset branch.
    தருக்க m_inclCond = பொய்;  // Whether conditional arcs should be kept explicitly.
    File_வரி* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_மதிப்பு, FsmStateVertex*, Fsm_State_மதிப்பு_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

பொது:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{புதிய FsmPseudoVertex{இது, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{புதிய FsmPseudoVertex{இது, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() மாறிலி { திரும்பு m_scopep; }
    வெறுமை scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() மாறிலி { திரும்பு m_stateAlwaysp; }
    வெறுமை stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    மாறிலி string& stateVarName() மாறிலி { திரும்பு m_stateVarName; }
    வெறுமை stateVarName(மாறிலி string& name) { m_stateVarName = name; }
    மாறிலி string& stateVarInternalName() மாறிலி { திரும்பு m_stateVarInternalName; }
    வெறுமை stateVarInternalName(மாறிலி string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() மாறிலி { திரும்பு m_stateVarScopep; }
    வெறுமை stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() மாறிலி {
        திரும்பு m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    வெறுமை sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    மாறிலி std::vector<FsmSenDesc>& senses() மாறிலி { திரும்பு m_senses; }
    std::vector<FsmSenDesc>& senses() { திரும்பு m_senses; }
    மாறிலி FsmResetCondDesc& resetCond() மாறிலி { திரும்பு m_resetCond; }
    FsmResetCondDesc& resetCond() { திரும்பு m_resetCond; }
    தருக்க hasResetCond() மாறிலி { திரும்பு m_hasResetCond; }
    வெறுமை hasResetCond(தருக்க flag) { m_hasResetCond = flag; }
    தருக்க inclCond() மாறிலி { திரும்பு m_inclCond; }
    வெறுமை inclCond(தருக்க flag) { m_inclCond = flag; }
    File_வரி* fileline() மாறிலி { திரும்பு m_flp; }
    வெறுமை fileline(File_வரி* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_மதிப்பு மதிப்பு) VL_MT_DISABLED {
        FsmStateVertex* மாறிலி vertexp = புதிய FsmStateVertex{இது, label, மதிப்பு};
        m_stateVertices.emplace(மதிப்பு, vertexp);
        திரும்பு vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { திரும்பு m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { திரும்பு m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_மதிப்பு from_மதிப்பு, Fsm_State_மதிப்பு to_மதிப்பு, தருக்க isReset, தருக்க isCond,
                       தருக்க isDefault, File_வரி* flp) VL_MT_DISABLED {
        FsmStateVertex* மாறிலி உச்சி = m_stateVertices.at(to_மதிப்பு);
        FsmVertex* fromp = nullptr;
        எனில் (isReset) {
            fromp = resetAnyVertex();
        } இல்லையெனில் எனில் (isDefault) {
            fromp = defaultAnyVertex();
        } இல்லையெனில் {
            fromp = m_stateVertices.at(from_மதிப்பு);
        }
        திரும்பு புதிய FsmArcEdge{இது, fromp, உச்சி, isReset, isCond, isDefault, flp};
    }

    string name() மாறிலி VL_MT_SAFE {
        திரும்பு "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) மாறிலி {
        string tag = stateVarInternalName();
        க்கு (எழுத்து& ch : tag) {
            எனில் (!std::isalnum(static_cast<unsigned எழுத்து>(ch))) ch = '_';
        }
        திரும்பு "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

கட்டமைப்பு DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

கட்டமைப்பு FsmCaseCandidate final {
    Ast_முனை* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
கட்டமைப்பு FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_முனை_கோவை* valuep = nullptr;  // Compared constant value expression
    Fsm_State_மதிப்பு மதிப்பு;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
கட்டமைப்பு FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_முனை* stmtsp = nullptr;  // Branch body
    Ast_முனை_கோவை* valuep = nullptr;  // Source state value expression
    Fsm_State_மதிப்பு from_மதிப்பு;  // Encoded source state value
    தருக்க has_உச்சி_Guard = பொய்;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
கட்டமைப்பு FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_முனை* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<மாறிலி AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<மாறிலி AstCell*, FsmCellPortMap>;

கட்டமைப்பு StateConstLabel final {
    string text;
    தருக்க fromParam = பொய்;
    size_t stateIndex = 0;
};

கட்டமைப்பு FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_மதிப்பு>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_மதிப்பு, StateConstLabel, Fsm_State_மதிப்பு_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    தருக்க enumBacked = பொய்;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
வகுப்பு FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<மாறிலி AstVarScope*, size_t> m_fsmIndex;

பொது:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        மாறிலி std::map<மாறிலி AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        எனில் (it != m_fsmIndex.end()) திரும்பு m_fsms.at(it->second);
        மாறிலி size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        திரும்பு m_fsms.back();
    }
    மாறிலி std::vector<DetectedFsm>& fsms() மாறிலி { திரும்பு m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
வகுப்பு FsmDetectVisitor final : பொது VNVisitor {
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
    std::unordered_map<மாறிலி AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<மாறிலி AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    நிலையான Ast_முனை_DType* unwrapEnumCandidate(Ast_முனை_DType* dtypep) {
        திரும்பு dtypep->skipRefToEnump();
    }

    நிலையான string candidateConflictContext(Ast_முனை* laterNodep,
                                           மாறிலி FsmCaseCandidate& firstCand) {
        திரும்பு '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    நிலையான தருக்க rejectFsmWrapperCell(AstCell* cellp, மாறிலி string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        திரும்பு பொய்;
    }

    நிலையான தருக்க simple_Param_State_மதிப்பு(AstCell* cellp, மாறிலி string& name, Fsm_State_மதிப்பு& மதிப்பு,
                                      Ast_முனை_கோவை*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        க்கு (Ast_முனை* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* மாறிலி varp = VN_CAST(stmtp, Var);
            எனில் (!varp || !varp->isParam() || varp->name() != name) தொடர்;
            valuepr = VN_AS(varp->valuep(), முனை_கோவை);
            திரும்பு const_மதிப்பு_Status(valuepr, மதிப்பு) == Const_மதிப்பு_Status::OK;
        }
        திரும்பு பொய்;
    }

    நிலையான தருக்க childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        எனில் (!vscp->varp()->isIO()) திரும்பு பொய்;
        AstScope* மாறிலி scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        எனில் (scopep->aboveScopep() != parentScopep) திரும்பு பொய்;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        திரும்பு உண்மை;
    }

    நிலையான AstVarScope* simpleAssignVarScope(Ast_முனை_கோவை* exprp) {
        AstVarRef* மாறிலி vrefp = VN_CAST(exprp, VarRef);
        திரும்பு vrefp ? vrefp->varScopep() : nullptr;
    }

    வெறுமை addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        மாறிலி std::pair<AstScope*, AstCell*> item{scopep, cellp};
        எனில் (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            திரும்பு;
        }
        m_wrapperCells.emplace_back(item);
    }

    வெறுமை collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* மாறிலி lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* மாறிலி rhsVscp = simpleAssignVarScope(nodep->rhsp());
        எனில் (!lhsVscp || !rhsVscp) திரும்பு;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        எனில் (childPortInScope(lhsVscp, m_scopep, cellp)) {
            எனில் (!fsmRegisterWrapperDesc(cellp)) திரும்பு;
            UASSERT_OBJ(lhsVscp->varp()->is_உள்ளீடு(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } இல்லையெனில் எனில் (childPortInScope(rhsVscp, m_scopep, cellp)) {
            எனில் (!fsmRegisterWrapperDesc(cellp)) திரும்பு;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, மாறிலி string& portName) மாறிலி {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        மாறிலி FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        மாறிலி FsmCellPortMap::const_iterator portIt = ports.find(portName);
        திரும்பு portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, மாறிலி string& portName) மாறிலி {
        மாறிலி FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        மாறிலி FsmCellPortMap::const_iterator portIt = ports.find(portName);
        திரும்பு portIt == ports.end() ? nullptr : portIt->second;
    }

    வகுப்பு RegisterAlwaysAnalyzer final {
        AstScope* மாறிலி m_scopep;  // Scope containing the always block(s) being analyzed

    பொது:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_முனை_கோவை*>>
        oneBlockCandidates(AstAlways* alwaysp) மாறிலி {
            std::vector<std::pair<AstCase*, Ast_முனை_கோவை*>> candidates;
            Ast_முனை* மாறிலி stmtsp = alwaysp->stmtsp();
            எனில் (AstIf* மாறிலி firstIfp = VN_CAST(stmtsp, If)) {
                எனில் (AstCase* மாறிலி casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            க்கு (Ast_முனை* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                எனில் (AstCase* மாறிலி casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            திரும்பு candidates;
        }

        std::vector<std::pair<AstIf*, Ast_முனை_கோவை*>>
        oneBlockIfCandidates(AstAlways* alwaysp) மாறிலி {
            std::vector<std::pair<AstIf*, Ast_முனை_கோவை*>> candidates;
            Ast_முனை* மாறிலி stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            எனில் (AstIf* மாறிலி firstIfp = VN_CAST(stmtsp, If)) {
                எனில் (AstIf* மாறிலி chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            க்கு (Ast_முனை* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                எனில் (AstIf* மாறிலி ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            திரும்பு candidates;
        }

        தருக்க matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) மாறிலி {
            திரும்பு FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        வெறுமை buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_முனை_கோவை* resetCondp,
                                    FsmRegisterCandidate& reg) மாறிலி {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_மரம்(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* மாறிலி firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            எனில் (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                மாறிலி ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                எனில் (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } இல்லையெனில் எனில் (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_மதிப்பு reset_மதிப்பு;
                    Ast_முனை* மாறிலி thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_முனை_Assign* மாறிலி resetAssp = FsmDetectVisitor::direct_Const_State_Assign_முனை(
                        thenNodep, resetStateVscp, reset_மதிப்பு);
                    எனில் (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_மதிப்பு, resetAssp);
                    }
                } இல்லையெனில் எனில் (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    தருக்க matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) மாறிலி {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* மாறிலி nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* மாறிலி stateVscp = roleVarScope(cellp, roles.qPort);
        எனில் (!nextVscp || !stateVscp) {
            திரும்பு rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* மாறிலி clkVscp = roleVarScope(cellp, roles.clkPort);
        எனில் (!clkVscp) {
            திரும்பு rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().தள்ளு_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        எனில் (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        எனில் (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            தருக்க inferred_Active_கீழ் = பொய்;
            எனில் (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_கீழ்)) {
                roles.has_Rst_Active_கீழ் = உண்மை;
                roles.rst_Active_கீழ் = inferred_Active_கீழ்;
            }
        }

        Ast_முனை_கோவை* resetValuep = nullptr;
        Fsm_State_மதிப்பு reset_மதிப்பு;
        மாறிலி தருக்க has_Reset_மதிப்பு
            = !roles.rstValParam.empty()
              && simple_Param_State_மதிப்பு(cellp, roles.rstValParam, reset_மதிப்பு, resetValuep);
        எனில் (resetVscp && roles.has_Rst_Active_கீழ் && has_Reset_மதிப்பு) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_கீழ் ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().தள்ளு_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_கீழ் = roles.rst_Active_கீழ்;
            cand.hasResetCond(உண்மை);
            cand.resetArcs().emplace_back(reset_மதிப்பு, cellp, resetValuep);
        } இல்லையெனில் எனில் (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            எனில் (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } இல்லையெனில் எனில் (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } இல்லையெனில் எனில் (!roles.has_Rst_Active_கீழ்) {
                reason = "reset polarity could not be inferred from the wrapper";
            } இல்லையெனில் எனில் (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } இல்லையெனில் {
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
        திரும்பு உண்மை;
    }

    வகுப்பு ComboAlwaysAnalyzer final {
    பொது:
        கட்டமைப்பு ComboMatch final {
            மாறிலி FsmRegisterCandidate* matchedp = nullptr;
            Ast_முனை* warnNodep = nullptr;
        };

    தனிப்பட்ட:
        மாறிலி std::vector<FsmRegisterCandidate>& m_registerCandidates;

    பொது:
        explicit ComboAlwaysAnalyzer(மாறிலி std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_முனை* stmtsp, AstCase* casep) மாறிலி {
            ComboMatch match;
            AstVarRef* மாறிலி selp = VN_CAST(casep->exprp(), VarRef);
            எனில் (!selp) திரும்பு match;
            க்கு (மாறிலி FsmRegisterCandidate& reg : m_registerCandidates) {
                எனில் (selp->varScopep() == reg.nextVscp()) {
                    எனில் (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        தொடர்;
                    }
                } இல்லையெனில் எனில் (selp->varScopep() != reg.stateVscp()) {
                    தொடர்;
                }
                Ast_முனை* மாறிலி warnNodep = FsmDetectVisitor::case_Supported_Transition_முனை(
                    casep, reg.nextVscp(), reg.inclCond());
                எனில் (!warnNodep) தொடர்;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            திரும்பு match;
        }

        ComboMatch matchIfChain(Ast_முனை* stmtsp, மாறிலி FsmIfChainCandidate& chain) மாறிலி {
            ComboMatch match;
            க்கு (மாறிலி FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                எனில் (chain.compareVscp == reg.nextVscp()) {
                    எனில் (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        தொடர்;
                    }
                } இல்லையெனில் எனில் (chain.compareVscp != reg.stateVscp()) {
                    தொடர்;
                }
                Ast_முனை* மாறிலி warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_முனை(chain, reg.nextVscp());
                எனில் (!warnNodep) தொடர்;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            திரும்பு match;
        }

        தருக்க shouldWarnUnsupported(Ast_முனை* stmtsp, AstCase* casep) மாறிலி {
            மாறிலி AstVarRef* மாறிலி selp = VN_CAST(casep->exprp(), VarRef);
            எனில் (!selp) திரும்பு பொய்;

            திரும்பு std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](மாறிலி FsmRegisterCandidate& reg) -> தருக்க {
                    மாறிலி தருக்க matchesNext = selp->varScopep() == reg.nextVscp();
                    மாறிலி தருக்க matchesState = selp->varScopep() == reg.stateVscp();

                    எனில் (!matchesNext && !matchesState) திரும்பு பொய்;
                    எனில் (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        திரும்பு பொய்;
                    }
                    திரும்பு FsmDetectVisitor::case_Supported_Transition_முனை(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    நிலையான தருக்க isSimpleResetCond(Ast_முனை_கோவை* condp) { திரும்பு VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    நிலையான FsmResetCondDesc describeResetCond(Ast_முனை_கோவை* condp) {
        FsmResetCondDesc desc;
        எனில் (AstVarRef* மாறிலி vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        திரும்பு desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    நிலையான std::vector<FsmSenDesc> describe_Sen_மரம்(Ast_Sen_மரம்* sentreep) {
        std::vector<FsmSenDesc> senses;
        க்கு (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_முனை_Var_Ref* மாறிலி vrefp = itemp->varrefp();
            எனில் (!vrefp) தொடர்;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.தள்ளு_back(desc);
        }
        திரும்பு senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    நிலையான தருக்க isIgnorableStmt(Ast_முனை* nodep) { திரும்பு VN_IS(nodep, CoverInc); }

    நிலையான Ast_முனை* skipLeadingIgnorableStmt(Ast_முனை* nodep) {
        போது (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        திரும்பு nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    நிலையான Ast_முனை* singleMeaningfulStmt(Ast_முனை* stmtp) {
        Ast_முனை* resultp = nullptr;
        க்கு (Ast_முனை* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            எனில் (isIgnorableStmt(nodep)) தொடர்;
            எனில் (resultp) திரும்பு nullptr;
            resultp = nodep;
        }
        திரும்பு resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    நிலையான Ast_முனை* singleMeaningfulBranch(Ast_முனை* branchp) {
        எனில் (!branchp) திரும்பு nullptr;
        திரும்பு branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    நிலையான தருக்க isPlainComboSentree(மாறிலி Ast_Sen_மரம்* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        க்கு (மாறிலி AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            எனில் (senp->edgeType() == VEdgeType::ET_CHANGED) தொடர்;
            திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }

    வெறுமை warnUnsupportedComboAlways(மாறிலி FsmComboAlways& combo) {
        மாறிலி ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_முனை* மாறிலி stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        தருக்க warned = பொய்;
        க்கு (Ast_முனை* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* மாறிலி casep = VN_CAST(nodep, Case);
            எனில் (!casep) தொடர்;
            எனில் (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = உண்மை;
            }
            எனில் (warned) நிறுத்து;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    நிலையான Ast_முனை_Assign* directStateAssign(Ast_முனை* stmtp, AstVarScope* stateVscp) {
        Ast_முனை* மாறிலி nodep = singleMeaningfulBranch(stmtp);
        எனில் (!nodep) திரும்பு nullptr;
        Ast_முனை_Assign* மாறிலி assp = VN_CAST(nodep, முனை_Assign);
        எனில் (!assp) திரும்பு nullptr;
        AstVarRef* மாறிலி vrefp = VN_CAST(assp->lhsp(), VarRef);
        எனில் (!vrefp || vrefp->varScopep() != stateVscp) திரும்பு nullptr;
        திரும்பு assp;
    }

    நிலையான Ast_முனை_Assign* முனை_State_Var_Assign(Ast_முனை* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_முனை_Assign* மாறிலி assp = VN_CAST(nodep, முனை_Assign);
        எனில் (!assp) திரும்பு nullptr;
        AstVarRef* மாறிலி lhsp = VN_CAST(assp->lhsp()->baseFromp(உண்மை), VarRef);
        AstVarRef* மாறிலி rhsp = VN_CAST(assp->rhsp(), VarRef);
        எனில் (!rhsp || !lhsp) திரும்பு nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        திரும்பு assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    நிலையான Ast_முனை_Assign* directCondStateVarAssign(Ast_முனை* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_முனை_கோவை*& condp,
                                                   Fsm_State_மதிப்பு& reset_மதிப்பு) {
        Ast_முனை_Assign* மாறிலி assp = VN_CAST(nodep, முனை_Assign);
        எனில் (!assp) திரும்பு nullptr;
        AstVarRef* மாறிலி lhsp = VN_CAST(assp->lhsp()->baseFromp(உண்மை), VarRef);
        AstCond* மாறிலி rhsp = VN_CAST(assp->rhsp(), Cond);
        எனில் (!rhsp || !lhsp) திரும்பு nullptr;
        AstVarRef* மாறிலி elsep = VN_CAST(rhsp->elsep(), VarRef);
        எனில் (!elsep) திரும்பு nullptr;
        எனில் (const_மதிப்பு_Status(rhsp->thenp(), reset_மதிப்பு) != Const_மதிப்பு_Status::OK) திரும்பு nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        திரும்பு assp;
    }

    நிலையான Ast_முனை_Assign* direct_Const_State_Assign_முனை(Ast_முனை* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_மதிப்பு& மதிப்பு) {
        Ast_முனை_Assign* மாறிலி assp = VN_CAST(nodep, முனை_Assign);
        எனில் (!assp) திரும்பு nullptr;
        AstVarRef* மாறிலி lhsp = VN_CAST(assp->lhsp()->baseFromp(உண்மை), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        எனில் (const_மதிப்பு_Status(assp->rhsp(), மதிப்பு) != Const_மதிப்பு_Status::OK) திரும்பு nullptr;
        stateVscp = lhsp->varScopep();
        திரும்பு assp;
    }

    எண்ணிக்கை வகுப்பு ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    நிலையான ResetAssignStatus collectConstStateAssigns(Ast_முனை* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_முனை* nodep = skipLeadingIgnorableStmt(stmtp);
        எனில் (!nodep) திரும்பு ResetAssignStatus::EMPTY;
        க்கு (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_மதிப்பு மதிப்பு;
            Ast_முனை_Assign* மாறிலி assp = direct_Const_State_Assign_முனை(nodep, assignStateVscp, மதிப்பு);
            எனில் (!assp) திரும்பு ResetAssignStatus::NONE;
            எனில் (!stateVscp) stateVscp = assignStateVscp;
            எனில் (assignStateVscp != stateVscp) திரும்பு ResetAssignStatus::NONE;
            எனில் (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                திரும்பு ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(மதிப்பு, assp);
            எனில் (!nodep->nextp()) திரும்பு ResetAssignStatus::SINGLE;
        }
    }

    நிலையான தருக்க hasCanonicalNextStateDefaultBeforeCase(Ast_முனை* stmtsp, Ast_முனை* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_முனை* மாறிலி bodyp = skipLeadingIgnorableStmt(stmtsp);
        தருக்க sawCanonicalDefault = பொய்;
        க்கு (Ast_முனை* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            எனில் (nodep == targetp) திரும்பு sawCanonicalDefault;
            எனில் (Ast_முனை_Assign* மாறிலி assp = VN_CAST(nodep, முனை_Assign)) {
                AstVarRef* மாறிலி lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* மாறிலி rhsp = VN_CAST(assp->rhsp(), VarRef);
                எனில் (!lhsp || lhsp->varScopep() != nextVscp) தொடர்;
                எனில் (sawCanonicalDefault) {
                    மாறிலி string nextName = nextVscp->varp()->prettyNameQ();
                    மாறிலி string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    திரும்பு பொய்;
                }
                எனில் (!rhsp || rhsp->varScopep() != stateVscp) திரும்பு பொய்;
                sawCanonicalDefault = உண்மை;
            }
        }
    }

    நிலையான தருக்க ifStateConstAssign(Ast_முனை* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_மதிப்பு& then_மதிப்பு, Fsm_State_மதிப்பு& else_மதிப்பு) {
        AstIf* மாறிலி ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        எனில் (!ifp || !ifp->elsesp()) திரும்பு பொய்;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_முனை* மாறிலி thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_முனை* மாறிலி elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        எனில் (!thenNodep || !elseNodep) திரும்பு பொய்;
        எனில் (!direct_Const_State_Assign_முனை(thenNodep, thenVscp, then_மதிப்பு)) திரும்பு பொய்;
        எனில் (!direct_Const_State_Assign_முனை(elseNodep, elseVscp, else_மதிப்பு)) திரும்பு பொய்;
        எனில் (thenVscp == stateVscp && elseVscp == stateVscp) திரும்பு உண்மை;
        எனில் (thenVscp != elseVscp) திரும்பு பொய்;
        Ast_முனை* மாறிலி followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_முனை* மாறிலி finalNodep = singleMeaningfulBranch(followp);
        எனில் (!finalNodep) திரும்பு பொய்;
        எனில் (!முனை_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) திரும்பு பொய்;
        எனில் (finalStateVscp != stateVscp) திரும்பு பொய்;
        எனில் (finalFromVscp != thenVscp) திரும்பு பொய்;
        திரும்பு உண்மை;
    }

    நிலையான தருக்க directStateCondConstAssign(Ast_முனை* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_மதிப்பு& then_மதிப்பு, Fsm_State_மதிப்பு& else_மதிப்பு) {
        Ast_முனை_Assign* மாறிலி assp = directStateAssign(stmtp, stateVscp);
        எனில் (!assp) திரும்பு பொய்;
        AstCond* மாறிலி condp = VN_CAST(assp->rhsp(), Cond);
        எனில் (!condp) திரும்பு பொய்;
        திரும்பு const_மதிப்பு_Status(condp->thenp(), then_மதிப்பு) == Const_மதிப்பு_Status::OK
               && const_மதிப்பு_Status(condp->elsep(), else_மதிப்பு) == Const_மதிப்பு_Status::OK;
    }

    நிலையான Ast_முனை* case_Item_Supported_Arc_முனை(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             தருக்க inclCond) {
        எனில் (itemp->isDefault()) {
            எனில் (!inclCond) திரும்பு nullptr;
        }
        Ast_முனை_Assign* மாறிலி assp = directStateAssign(itemp->stmtsp(), stateVscp);
        எனில் (assp) {
            Fsm_State_மதிப்பு to_மதிப்பு;
            எனில் (const_மதிப்பு_Status(assp->rhsp(), to_மதிப்பு) == Const_மதிப்பு_Status::OK) திரும்பு assp;
        }
        Fsm_State_மதிப்பு then_மதிப்பு;
        Fsm_State_மதிப்பு else_மதிப்பு;
        எனில் (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_மதிப்பு, else_மதிப்பு)) {
            திரும்பு assp;
        }
        எனில் (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_மதிப்பு, else_மதிப்பு)) {
            திரும்பு singleMeaningfulBranch(itemp->stmtsp());
        }
        திரும்பு nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    நிலையான Ast_முனை* case_Supported_Transition_முனை(AstCase* casep, AstVarScope* stateVscp,
                                                தருக்க inclCond) {
        க்கு (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            எனில் (Ast_முனை* மாறிலி nodep = case_Item_Supported_Arc_முனை(itemp, stateVscp, inclCond))
                திரும்பு nodep;
        }
        திரும்பு nullptr;
    }

    நிலையான Ast_முனை* case_Item_Supported_Arc_முனை_Like(Ast_முனை* stmtsp, AstVarScope* stateVscp) {
        எனில் (Ast_முனை_Assign* மாறிலி assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_மதிப்பு to_மதிப்பு;
            எனில் (const_மதிப்பு_Status(assp->rhsp(), to_மதிப்பு) == Const_மதிப்பு_Status::OK) திரும்பு assp;
            Fsm_State_மதிப்பு then_மதிப்பு;
            Fsm_State_மதிப்பு else_மதிப்பு;
            எனில் (directStateCondConstAssign(stmtsp, stateVscp, then_மதிப்பு, else_மதிப்பு)) திரும்பு assp;
        }
        திரும்பு nullptr;
    }

    நிலையான Ast_முனை* if_Chain_Supported_Transition_முனை(மாறிலி FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        க்கு (size_t i = 0; i < chain.branches.size(); ++i) {
            மாறிலி FsmIfBranch& branch = chain.branches[i];
            Ast_முனை* மாறிலி nodep = case_Item_Supported_Arc_முனை_Like(branch.stmtsp, stateVscp);
            எனில் (!nodep) திரும்பு nullptr;
        }
        திரும்பு chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    நிலையான string label_For_மதிப்பு(
        மாறிலி std::unordered_map<Fsm_State_மதிப்பு, StateConstLabel, Fsm_State_மதிப்பு_Hash>& labels,
        மாறிலி Fsm_State_மதிப்பு& மதிப்பு) {
        திரும்பு labels.at(மதிப்பு).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    எண்ணிக்கை வகுப்பு Const_மதிப்பு_Status : uint8_t { OK, NOT_CONST, XZ };

    நிலையான Const_மதிப்பு_Status const_மதிப்பு_Status(Ast_முனை_கோவை* exprp, Fsm_State_மதிப்பு& மதிப்பு) {
        மாறிலி AstConst* மாறிலி constp = VN_CAST(exprp, Const);
        எனில் (!constp) திரும்பு Const_மதிப்பு_Status::NOT_CONST;
        மாறிலி V3_எண்& இலக்கம் = constp->இலக்கம்();
        எனில் (இலக்கம்.isAnyXZ()) திரும்பு Const_மதிப்பு_Status::XZ;
        மதிப்பு = Fsm_State_மதிப்பு{இலக்கம்};
        திரும்பு Const_மதிப்பு_Status::OK;
    }

    நிலையான தருக்க pureStateComparisonNoAlias(Ast_முனை_கோவை* exprp, FsmStateComparison& cmp) {
        AstEq* மாறிலி eqp = VN_CAST(exprp, Eq);
        எனில் (!eqp) திரும்பு பொய்;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_முனை_கோவை* valuep = eqp->rhsp();
        எனில் (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(உண்மை), VarRef);
            எனில் (!vrefp) { திரும்பு பொய்; }
            valuep = eqp->lhsp();
        }

        Fsm_State_மதிப்பு மதிப்பு;
        எனில் (const_மதிப்பு_Status(valuep, மதிப்பு) != Const_மதிப்பு_Status::OK) திரும்பு பொய்;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.மதிப்பு = மதிப்பு;
        திரும்பு உண்மை;
    }

    நிலையான தருக்க pureStateComparison(Ast_முனை_கோவை* exprp, மாறிலி FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        எனில் (pureStateComparisonNoAlias(exprp, cmp)) திரும்பு உண்மை;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        எனில் (AstVarRef* மாறிலி vrefp = VN_CAST(exprp, VarRef)) {
            மாறிலி FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            எனில் (it == aliases.end()) திரும்பு பொய்;
            cmp = it->second;
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    நிலையான தருக்க supported_உச்சி_Level_Guard(Ast_முனை_கோவை* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        எனில் (VN_IS(exprp, Or)) திரும்பு பொய்;
        எனில் (VN_IS(exprp, RedAnd)) திரும்பு பொய்;
        எனில் (VN_IS(exprp, RedOr)) திரும்பு பொய்;
        எனில் (VN_IS(exprp, RedXor)) திரும்பு பொய்;
        திரும்பு உண்மை;
    }

    நிலையான தருக்க resolveIfPredicate(Ast_முனை_கோவை* exprp, மாறிலி FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, தருக்க& hasGuard) {
        std::vector<Ast_முனை_கோவை*> terms;
        std::vector<Ast_முனை_கோவை*> pending;
        pending.தள்ளு_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        போது (!pending.empty()) {
            Ast_முனை_கோவை* மாறிலி nodep = pending.back();
            pending.எடு_back();
            எனில் (AstAnd* மாறிலி andp = VN_CAST(nodep, And)) {
                pending.தள்ளு_back(andp->rhsp());
                pending.தள்ளு_back(andp->lhsp());
            } இல்லையெனில் {
                terms.தள்ளு_back(nodep);
            }
        }

        தருக்க sawComparison = பொய்;
        க்கு (size_t i = 0; i < terms.size(); ++i) {
            Ast_முனை_கோவை* மாறிலி termp = terms[i];
            FsmStateComparison termCmp;
            எனில் (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                எனில் (sawComparison) திரும்பு பொய்;
                cmp = termCmp;
                sawComparison = உண்மை;
                தொடர்;
            }
            எனில் (!supported_உச்சி_Level_Guard(termp)) திரும்பு பொய்;
            hasGuard = உண்மை;
        }
        திரும்பு sawComparison;
    }

    நிலையான வெறுமை addAlias(FsmAliasMap& aliases, std::unordered_set<மாறிலி AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, மாறிலி FsmStateComparison& cmp) {
        எனில் (ambiguous.find(aliasVscp) != ambiguous.end()) திரும்பு;
        மாறிலி FsmAliasMap::iterator it = aliases.find(aliasVscp);
        எனில் (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            திரும்பு;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        எனில் (it->second.stateVscp == cmp.stateVscp && it->second.மதிப்பு == cmp.மதிப்பு) திரும்பு;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        திரும்பு;
    }

    நிலையான வெறுமை collectAliasFromAssign(Ast_முனை_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<மாறிலி AstVarScope*>& ambiguous) {
        AstVarRef* மாறிலி lhsp = VN_CAST(assp->lhsp(), VarRef);
        எனில் (!lhsp) திரும்பு;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        எனில் (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) திரும்பு;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_முனை* stmtsp, Ast_முனை* limitp) மாறிலி {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<மாறிலி AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        க்கு (Ast_முனை* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            எனில் (Ast_முனை_Assign* மாறிலி assp = VN_CAST(nodep, முனை_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        க்கு (மாறிலி AstVarScope* மாறிலி vscp : ambiguous) aliases.erase(vscp);
        திரும்பு aliases;
    }

    நிலையான தருக்க collectIfChain(AstIf* ifp, மாறிலி FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_மதிப்புகள்;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        போது (உண்மை) {
            FsmStateComparison cmp;
            தருக்க hasGuard = பொய்;
            எனில் (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) திரும்பு பொய்;
            எனில் (chain.compareVscp && chain.compareVscp != cmp.stateVscp) திரும்பு பொய்;
            எனில் (!seen_மதிப்புகள்.செருகு(cmp.மதிப்பு.சாவி()).second) திரும்பு பொய்;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.தள்ளு_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.மதிப்பு, hasGuard});

            Ast_முனை* மாறிலி elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            எனில் (!elseNodep) நிறுத்து;
            எனில் (AstIf* மாறிலி elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                தொடர்;
            }
            chain.defaultStmtsp = elseNodep;
            நிறுத்து;
        }
        திரும்பு chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    நிலையான தருக்க validate_Known_State_மதிப்பு(Ast_முனை* nodep, மாறிலி FsmStateSpace& stateSpace,
                                        மாறிலி Fsm_State_மதிப்பு& மதிப்பு, மாறிலி string& role) {
        எனில் (stateSpace.labels.find(மதிப்பு) != stateSpace.labels.end()) திரும்பு உண்மை;
        எனில் (stateSpace.enumBacked) {
            மாறிலி string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + மதிப்பு.warnText()
                                        + " is not present in the declared enum");
            திரும்பு பொய்;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + மதிப்பு.warnText()
                                    + " is not present in the inferred state space");
        திரும்பு பொய்;
    }

    நிலையான StateConstLabel stateLabelForConst(AstConst* constp) {
        மாறிலி string name = constp->origParamName();
        எனில் (!name.empty()) திரும்பு StateConstLabel{Ast_முனை::prettyName(name), உண்மை, 0};
        திரும்பு StateConstLabel{constp->name(), பொய், 0};
    }

    நிலையான வெறுமை updateStateLabel(FsmStateSpace& stateSpace, மாறிலி Fsm_State_மதிப்பு& மதிப்பு,
                                 மாறிலி StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(மதிப்பு).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    நிலையான தருக்க matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        எனில் (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) திரும்பு பொய்;

        Ast_முனை* மாறிலி stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_முனை* மாறிலி nodep = singleMeaningfulStmt(stmtsp);
        எனில் (!nodep) திரும்பு பொய்;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        எனில் (AstIf* மாறிலி ifp = VN_CAST(nodep, If)) {
            எனில் (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) திரும்பு பொய்;
            AstVarScope* resetStateVscp = nullptr;
            மாறிலி ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            மாறிலி தருக்க emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            எனில் (emptyResetBranch) {
                cand.resetArcs().clear();
            } இல்லையெனில் எனில் (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_மதிப்பு reset_மதிப்பு;
                Ast_முனை* மாறிலி thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_முனை_Assign* மாறிலி resetAssp
                    = direct_Const_State_Assign_முனை(thenNodep, resetStateVscp, reset_மதிப்பு);
                எனில் (!resetAssp) திரும்பு பொய்;
                cand.resetArcs().emplace_back(reset_மதிப்பு, resetAssp);
            } இல்லையெனில் எனில் (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_முனை* மாறிலி elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            எனில் (!முனை_State_Var_Assign(elseNodep, stateVscp, nextVscp)) திரும்பு பொய்;
            எனில் (!emptyResetBranch && resetStateVscp != stateVscp) திரும்பு பொய்;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } இல்லையெனில் {
            Ast_முனை_கோவை* resetCondp = nullptr;
            Fsm_State_மதிப்பு reset_மதிப்பு;
            // Note the active low 'reset ? next : CONST' form is not accepted
            எனில் (Ast_முனை_Assign* மாறிலி assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_மதிப்பு)) {
                cand.resetArcs().emplace_back(reset_மதிப்பு, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } இல்லையெனில் எனில் (!முனை_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                திரும்பு பொய்;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_மரம்(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        திரும்பு உண்மை;
    }

    நிலையான தருக்க add_மதிப்பு_To_State_Space(Ast_முனை* nodep, FsmStateSpace& stateSpace,
                                     மாறிலி Fsm_State_மதிப்பு& மதிப்பு, StateConstLabel label) {
        மாறிலி auto labelIt = stateSpace.labels.find(மதிப்பு);
        எனில் (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            எனில் (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + மதிப்பு.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                திரும்பு பொய்;
            }
            எனில் (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, மதிப்பு, label);
            }
        } இல்லையெனில் {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, மதிப்பு);
            stateSpace.labels.emplace(மதிப்பு, storedLabel);
        }
        திரும்பு உண்மை;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    நிலையான தருக்க add_கோவை_To_State_Space(Ast_முனை_கோவை* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_மதிப்பு மதிப்பு;
        மாறிலி Const_மதிப்பு_Status status = const_மதிப்பு_Status(valuep, மதிப்பு);
        எனில் (status != Const_மதிப்பு_Status::OK) {
            எனில் (status == Const_மதிப்பு_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            திரும்பு பொய்;
        }
        AstConst* மாறிலி constp = VN_AS(valuep, Const);
        திரும்பு add_மதிப்பு_To_State_Space(valuep, stateSpace, மதிப்பு, stateLabelForConst(constp));
    }

    நிலையான தருக்க add_Optional_Target_கோவை_To_State_Space(Ast_முனை_கோவை* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_மதிப்பு மதிப்பு;
        மாறிலி Const_மதிப்பு_Status status = const_மதிப்பு_Status(valuep, மதிப்பு);
        எனில் (status != Const_மதிப்பு_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            திரும்பு பொய்;
        }
        AstConst* மாறிலி constp = VN_AS(valuep, Const);
        திரும்பு add_மதிப்பு_To_State_Space(valuep, stateSpace, மதிப்பு, stateLabelForConst(constp));
    }

    நிலையான வெறுமை addResetTargetsToStateSpace(மாறிலி std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        க்கு (மாறிலி FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_மதிப்பு().ascii(), பொய், 0};
            எனில் (AstConst* மாறிலி constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_மதிப்பு_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_மதிப்பு(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    நிலையான தருக்க collectDeclaredStateSpace(Ast_முனை* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, தருக்க& needs_Source_மதிப்புகள்) {
        AstVar* மாறிலி stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        எனில் (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        மாறிலி தருக்க forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        எனில் (enump) {
            stateSpace.enumBacked = உண்மை;
            க்கு (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                மாறிலி AstConst* மாறிலி constp = VN_AS(itemp->valuep(), Const);
                மாறிலி Fsm_State_மதிப்பு மதிப்பு{constp->இலக்கம்()};
                மாறிலி size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), மதிப்பு);
                stateSpace.labels.emplace(மதிப்பு,
                                          StateConstLabel{itemp->name(), பொய், stateIndex});
            }
            திரும்பு stateSpace.states.size() >= 2;
        }

        எனில் (forced) {
            needs_Source_மதிப்புகள் = உண்மை;
            திரும்பு உண்மை;
        }

        needs_Source_மதிப்புகள் = உண்மை;
        திரும்பு உண்மை;
    }

    template <typename T_ValuepVisitor>
    நிலையான தருக்க collect_State_Space_From_மதிப்புகள்(Ast_முனை* warnNodep, AstVarScope* stateVscp,
                                            மாறிலி std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            மாறிலி T_ValuepVisitor& visitValueps) {
        தருக்க needs_Source_மதிப்புகள் = பொய்;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        எனில் (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_மதிப்புகள்)) {
            திரும்பு பொய்;
        }
        எனில் (!needs_Source_மதிப்புகள்) திரும்பு உண்மை;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        எனில் (!visitValueps(
                [&](Ast_முனை_கோவை* valuep) { திரும்பு add_கோவை_To_State_Space(valuep, stateSpace); })) {
            திரும்பு பொய்;
        }
        திரும்பு stateSpace.states.size() >= 2;
    }

    நிலையான தருக்க collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  மாறிலி std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        திரும்பு collect_State_Space_From_மதிப்புகள்(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](மாறிலி auto& visitValuep) {
                க்கு (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    எனில் (!itemp->isDefault()) {
                        க்கு (Ast_முனை_கோவை* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), முனை_கோவை)) {
                            எனில் (!visitValuep(condp)) திரும்பு பொய்;
                        }
                    }
                    எனில் (Ast_முனை_Assign* மாறிலி assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_மதிப்பு then_மதிப்பு;
                        Fsm_State_மதிப்பு else_மதிப்பு;
                        AstCond* மாறிலி condp = VN_CAST(assp->rhsp(), Cond);
                        எனில் (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_மதிப்பு,
                                                          else_மதிப்பு)) {
                            எனில் (!visitValuep(condp->thenp())) திரும்பு பொய்;
                            எனில் (!visitValuep(condp->elsep())) திரும்பு பொய்;
                        } இல்லையெனில் எனில் (!add_Optional_Target_கோவை_To_State_Space(assp->rhsp(), stateSpace)) {
                            திரும்பு பொய்;
                        }
                    }
                }
                திரும்பு உண்மை;
            });
    }

    நிலையான தருக்க collectStateSpace(மாறிலி FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  மாறிலி std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        திரும்பு collect_State_Space_From_மதிப்புகள்(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](மாறிலி auto& visitValuep) {
                க்கு (மாறிலி FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_முனை_Assign* மாறிலி assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_மதிப்பு then_மதிப்பு;
                    Fsm_State_மதிப்பு else_மதிப்பு;
                    AstCond* மாறிலி condp = VN_CAST(assp->rhsp(), Cond);
                    எனில் (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_மதிப்பு, else_மதிப்பு),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        எனில் (!visitValuep(condp->thenp())) திரும்பு பொய்;
                        எனில் (!visitValuep(condp->elsep())) திரும்பு பொய்;
                    } இல்லையெனில் எனில் (!add_Optional_Target_கோவை_To_State_Space(assp->rhsp(), stateSpace)) {
                        திரும்பு பொய்;
                    }
                }
                திரும்பு உண்மை;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    நிலையான தருக்க emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 மாறிலி FsmStateSpace& stateSpace, தருக்க inclCond) {
        std::vector<std::pair<string, Fsm_State_மதிப்பு>> froms;
        எனில் (itemp->isDefault()) {
            எனில் (!inclCond) திரும்பு பொய்;
            froms.emplace_back("default", Fsm_State_மதிப்பு{});
        } இல்லையெனில் {
            க்கு (Ast_முனை_கோவை* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), முனை_கோவை)) {
                Fsm_State_மதிப்பு மதிப்பு;
                எனில் (const_மதிப்பு_Status(condp, மதிப்பு) != Const_மதிப்பு_Status::OK) தொடர்;
                எனில் (!validate_Known_State_மதிப்பு(condp, stateSpace, மதிப்பு, "source")) திரும்பு உண்மை;
                froms.emplace_back(label_For_மதிப்பு(stateSpace.labels, மதிப்பு), மதிப்பு);
            }
            எனில் (froms.empty()) திரும்பு பொய்;
        }

        எனில் (Ast_முனை_Assign* மாறிலி assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_மதிப்பு to_மதிப்பு;
            மாறிலி Const_மதிப்பு_Status status = const_மதிப்பு_Status(assp->rhsp(), to_மதிப்பு);
            எனில் (status == Const_மதிப்பு_Status::OK) {
                எனில் (!validate_Known_State_மதிப்பு(assp, stateSpace, to_மதிப்பு, "target")) திரும்பு உண்மை;
                க்கு (மாறிலி std::pair<string, Fsm_State_மதிப்பு>& from : froms) {
                    graph.addArc(from.second, to_மதிப்பு, பொய், பொய், itemp->isDefault(),
                                 assp->fileline());
                }
                திரும்பு உண்மை;
            }
        }

        Fsm_State_மதிப்பு then_மதிப்பு;
        Fsm_State_மதிப்பு else_மதிப்பு;
        எனில் (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_மதிப்பு, else_மதிப்பு)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_மதிப்பு, else_மதிப்பு)) {
            எனில் (!validate_Known_State_மதிப்பு(itemp->stmtsp(), stateSpace, then_மதிப்பு, "target"))
                திரும்பு உண்மை;
            எனில் (!validate_Known_State_மதிப்பு(itemp->stmtsp(), stateSpace, else_மதிப்பு, "target"))
                திரும்பு உண்மை;
            க்கு (மாறிலி Fsm_State_மதிப்பு& branch_மதிப்பு : {then_மதிப்பு, else_மதிப்பு}) {
                க்கு (மாறிலி std::pair<string, Fsm_State_மதிப்பு>& from : froms) {
                    graph.addArc(from.second, branch_மதிப்பு, பொய், உண்மை, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            திரும்பு உண்மை;
        }

        திரும்பு பொய்;
    }

    நிலையான வெறுமை emitStmtArcsFrom(FsmGraph& graph, Ast_முனை* stmtsp, AstVarScope* stateVscp,
                                 மாறிலி FsmStateSpace& stateSpace, Fsm_State_மதிப்பு from_மதிப்பு,
                                 தருக்க isDefault, தருக்க forceCond) {
        Ast_முனை_Assign* மாறிலி assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_மதிப்பு to_மதிப்பு;
        மாறிலி Const_மதிப்பு_Status status = const_மதிப்பு_Status(assp->rhsp(), to_மதிப்பு);
        எனில் (status == Const_மதிப்பு_Status::OK) {
            எனில் (!validate_Known_State_மதிப்பு(assp, stateSpace, to_மதிப்பு, "target")) திரும்பு;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_மதிப்பு, to_மதிப்பு, பொய், forceCond, isDefault, assp->fileline());
            திரும்பு;
        }

        Fsm_State_மதிப்பு then_மதிப்பு;
        Fsm_State_மதிப்பு else_மதிப்பு;
        மாறிலி தருக்க condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_மதிப்பு, else_மதிப்பு);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        எனில் (!validate_Known_State_மதிப்பு(stmtsp, stateSpace, then_மதிப்பு, "target")) திரும்பு;
        எனில் (!validate_Known_State_மதிப்பு(stmtsp, stateSpace, else_மதிப்பு, "target")) திரும்பு;
        க்கு (மாறிலி Fsm_State_மதிப்பு& branch_மதிப்பு : {then_மதிப்பு, else_மதிப்பு}) {
            graph.addArc(from_மதிப்பு, branch_மதிப்பு, பொய், உண்மை, isDefault, stmtsp->fileline());
        }
    }

    நிலையான வெறுமை emitIfChainArcs(FsmGraph& graph, மாறிலி FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, மாறிலி FsmStateSpace& stateSpace) {
        க்கு (size_t i = 0; i < chain.branches.size(); ++i) {
            மாறிலி FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            எனில் (!validate_Known_State_மதிப்பு(branch.ifp, stateSpace, branch.from_மதிப்பு, "source"))
                திரும்பு;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_மதிப்பு, பொய்,
                             branch.has_உச்சி_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    நிலையான வெறுமை addResetArcs(FsmGraph& graph, மாறிலி std::vector<FsmResetArcDesc>& resetArcs,
                             மாறிலி FsmStateSpace& stateSpace) {
        க்கு (மாறிலி FsmResetArcDesc& resetArc : resetArcs) {
            எனில் (!validate_Known_State_மதிப்பு(resetArc.nodep(), stateSpace, resetArc.to_மதிப்பு(),
                                         "target"))
                தொடர்;
            graph.addArc(Fsm_State_மதிப்பு{}, resetArc.to_மதிப்பு(), உண்மை, பொய், பொய்,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    வெறுமை processCase(AstCase* casep, AstVarScope* assignVscp, மாறிலி FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* மாறிலி stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        எனில் (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) திரும்பு;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        எனில் (!entry.graphp) {
            entry.graphp.reset(புதிய FsmGraph{});
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
            க்கு (மாறிலி std::pair<string, Fsm_State_மதிப்பு>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        க்கு (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    வெறுமை processIfChain(மாறிலி FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        மாறிலி FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* மாறிலி stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        எனில் (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) திரும்பு;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(புதிய FsmGraph{});
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
        க்கு (மாறிலி std::pair<string, Fsm_State_மதிப்பு>& state : stateSpace.states) {
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
    வெறுமை processOneBlockAlways(மாறிலி FsmComboAlways& oneBlock) {
        மாறிலி RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* மாறிலி alwaysp = oneBlock.alwaysp();
        எனில் (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) திரும்பு;
        மாறிலி std::vector<std::pair<AstCase*, Ast_முனை_கோவை*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        க்கு (மாறிலி std::pair<AstCase*, Ast_முனை_கோவை*>& cand : candidates) {
            AstVarRef* மாறிலி selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* மாறிலி vscp = selp ? selp->varScopep() : nullptr;
            எனில் (!vscp) தொடர்;
            எனில் (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } இல்லையெனில் எனில் (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } இல்லையெனில் {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        எனில் (firstCand.stateVscp) திரும்பு;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        மாறிலி std::vector<std::pair<AstIf*, Ast_முனை_கோவை*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        க்கு (மாறிலி std::pair<AstIf*, Ast_முனை_கோவை*>& cand : ifCandidates) {
            மாறிலி FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            எனில் (!collectIfChain(cand.first, aliases, chain)) தொடர்;
            AstVarScope* மாறிலி vscp = chain.compareVscp;
            எனில் (!if_Chain_Supported_Transition_முனை(chain, vscp)) தொடர்;
            எனில் (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } இல்லையெனில் எனில் (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } இல்லையெனில் {
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
    நிலையான வெறுமை warnComboSameAlways(Ast_முனை* warnNodep, மாறிலி FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    வெறுமை processComboAlways(மாறிலி FsmComboAlways& combo) {
        மாறிலி ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_முனை* மாறிலி stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        க்கு (Ast_முனை* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* மாறிலி casep = VN_CAST(nodep, Case);
            எனில் (!casep) தொடர்;
            மாறிலி ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            மாறிலி FsmRegisterCandidate* மாறிலி matchedp = match.matchedp;
            Ast_முனை* மாறிலி matchedWarnNodep = match.warnNodep;
            எனில் (!matchedp) தொடர்;
            எனில் (!firstCand.stateVscp) {
                மாறிலி auto செருகு_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                எனில் (!செருகு_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  செருகு_Pair.first->second));
                    தொடர்;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                தொடர்;
            }
            எனில் (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                தொடர்;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        எனில் (firstCand.stateVscp) திரும்பு;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        க்கு (Ast_முனை* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* மாறிலி ifp = VN_CAST(nodep, If);
            எனில் (!ifp) தொடர்;
            FsmIfChainCandidate chain;
            மாறிலி FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            எனில் (!collectIfChain(ifp, aliases, chain)) தொடர்;
            மாறிலி ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            மாறிலி FsmRegisterCandidate* மாறிலி matchedp = match.matchedp;
            Ast_முனை* மாறிலி matchedWarnNodep = match.warnNodep;
            எனில் (!matchedp) தொடர்;
            எனில் (!firstCand.stateVscp) {
                மாறிலி std::pair<std::unordered_map<மாறிலி AstVarScope*, FsmCaseCandidate>::iterator,
                                தருக்க>
                    செருகு_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                எனில் (!செருகு_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  செருகு_Pair.first->second));
                    தொடர்;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                தொடர்;
            }
            எனில் (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                தொடர்;
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
    வெறுமை visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    வெறுமை visit(AstAlways* nodep) override {
        எனில் (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            திரும்பு;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        மாறிலி RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        எனில் (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* மாறிலி stateVscp = reg.stateVscp();
            மாறிலி தருக்க found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](மாறிலி FsmRegisterCandidate& existing) {
                                  திரும்பு existing.stateVscp() == stateVscp;
                              });
            எனில் (!found) { m_registerCandidates.emplace_back(reg); }
        }
        எனில் (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } இல்லையெனில் எனில் (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            எனில் (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } இல்லையெனில் {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    வெறுமை visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    வெறுமை visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        எனில் (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        க்கு (மாறிலி std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            எனில் (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        க்கு (மாறிலி FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        க்கு (மாறிலி FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        க்கு (மாறிலி FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
வகுப்பு FsmCoverIncBuilder final {
    மாறிலி FsmGraph& m_graph;  // Fsm graph
    std::vector<மாறிலி FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    மாறிலி FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    மாறிலி FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edges_மொத்தம் = 0;  // Total count of edges
    AstVarScope* மாறிலி m_prevVscp;  // Previous state of fsm
    File_வரி* மாறிலி m_flp;  // Fileline pointer to use across the builder
    மாறிலி std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_முனை_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_முனை_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    தருக்க m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(மாறிலி uint64_t மதிப்பு) {
        V3_எண் இலக்கம்{static_cast<Ast_முனை*>(nullptr), m_prevVscp->width()};
        இலக்கம்.setQuad(மதிப்பு);
        இலக்கம்.isSigned(m_prevVscp->dtypep()->isSigned());
        திரும்பு புதிய AstConst{m_flp, இலக்கம்};
    }

    std::string quadToBitStirngLabel(மாறிலி uint64_t val) {
        முழு width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        போது (--width >= 0) result.தள்ளு_back("01"[(val >> width) & 1]);
        திரும்பு result;
    }

    வெறுமை initExistingStates() {
        க்கு (மாறிலி V3GraphVertex& vtx : m_graph.vertices()) {
            மாறிலி FsmVertex* மாறிலி vertexp = vtx.as<FsmVertex>();
            m_edges_மொத்தம் += vertexp->inEdges().size();
            எனில் (மாறிலி FsmStateVertex* மாறிலி statep = vertexp->cast<FsmStateVertex>()) {
                m_states.தள்ளு_back(statep);
            } இல்லையெனில் எனில் (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } இல்லையெனில் /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        மாறிலி முழு width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    வெறுமை initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        மாறிலி uint64_t combinations_மொத்தம் = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        தருக்க undefRolled = பொய்;
        auto rollUndefs = [இது, combinations_மொத்தம்] {
            க்கு (uint64_t i = 0; i < combinations_மொத்தம்; ++i) {
                மாறிலி auto செருகு = m_undefinedStates.செருகு(i);
                எனில் (!செருகு.second) m_undefinedStates.erase(செருகு.first);
            }
        };
        க்கு (மாறிலி FsmStateVertex* மாறிலி statep : m_states) {
            மாறிலி V3_எண்& இலக்கம் = statep->மதிப்பு().இலக்கம்();
            மாறிலி uint64_t மதிப்பு = இலக்கம்.toUQuad();
            எனில் (!m_isComplete) {
                எனில் (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(மதிப்பு) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } இல்லையெனில் {
                    UASSERT_OBJ(m_undefinedStates.செருகு(மதிப்பு).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    எனில் (m_undefinedStates.size() * 2 >= combinations_மொத்தம்) {
                        undefRolled = உண்மை;
                        rollUndefs();
                    }
                }
            }
        }
        எனில் (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_முனை_கோவை* buildResetCond() மாறிலி {
        Ast_முனை_கோவை* மாறிலி refp
            = புதிய AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        திரும்பு m_graph.resetCond().active_கீழ்
                   ? static_cast<Ast_முனை_கோவை*>(புதிய AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t get_Arc_மொத்தம்_With_Expansion(மாறிலி VFsmExpandType expand) மாறிலி {
        size_t arc_மொத்தம்_With_Expansion = ~0ULL;
        தேர்வு (expand) {
        நிலை VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            நிறுத்து;
        }
        நிலை VFsmExpandType::AUTO: {
            arc_மொத்தம்_With_Expansion = m_edges_மொத்தம்;
            எனில் (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_மொத்தம்_With_Expansion -= m_defaultVertexp->outEdges().size();
                க்கு (மாறிலி FsmStateVertex* மாறிலி statep : m_states) {
                    arc_மொத்தம்_With_Expansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            எனில் (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_மொத்தம்_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            நிறுத்து;
        }
        நிலை VFsmExpandType::AUTO_EXPAND: {
            arc_மொத்தம்_With_Expansion = m_states.size() * m_states.size() - m_states.size();
            நிறுத்து;
        }
        நிலை VFsmExpandType::FULL: {
            மாறிலி முழு width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            எனில் (width < 32) {
                மாறிலி uint64_t combinations_மொத்தம் = static_cast<size_t>(1) << width;
                arc_மொத்தம்_With_Expansion = combinations_மொத்தம் * combinations_மொத்தம் - combinations_மொத்தம்;
            }
            நிறுத்து;
        }
        }
        திரும்பு arc_மொத்தம்_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(மாறிலி std::string& label,
                                           மாறிலி std::string& fsmTag = "") {
        AstCoverOtherDecl* மாறிலி declp
            = புதிய AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(உண்மை);
        m_declsp = Ast_முனை::addNext(m_declsp, declp);
        திரும்பு declp;
    }

    Ast_முனை_Stmt* buildCoverIncStmtp(மாறிலி std::string& fromStateLabel,
                                    மாறிலி std::string& toStateLabel,
                                    மாறிலி std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* மாறிலி declp = புதிய AstCoverOtherDecl{
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
        declp->perInstance(உண்மை);
        m_declsp = Ast_முனை::addNext(m_declsp, declp);
        திரும்பு புதிய AstCoverInc{m_flp, declp};
    }
    Ast_முனை_Stmt* buildExpandedTransitionHandler(மாறிலி std::string& toStateLabel,
                                                மாறிலி தருக்க considerUndefs,
                                                மாறிலி தருக்க expandOnlyWithTerminalVerticies,
                                                மாறிலி FsmStateVertex* மாறிலி toStatep = nullptr) {
        AstCase* மாறிலி arcCasep = புதிய AstCase{
            m_flp, VCaseType::CT_CASE, புதிய AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<மாறிலி FsmStateVertex*> handledSoureces;
        எனில் (toStatep) {
            க்கு (மாறிலி V3GraphEdge& edge : toStatep->inEdges()) {
                எனில் (மாறிலி FsmStateVertex* மாறிலி fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    எனில் (!handledSoureces.emplace(fromp).second) தொடர்;
                    arcCasep->addItemsp(புதிய AstCaseItem{
                        m_flp, makeConst(fromp->மதிப்பு().இலக்கம்().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        க்கு (மாறிலி FsmStateVertex* மாறிலி fromp : m_states) {
            எனில் (handledSoureces.find(fromp) != handledSoureces.end()) தொடர்;
            arcCasep->addItemsp(புதிய AstCaseItem{
                m_flp, makeConst(fromp->மதிப்பு().இலக்கம்().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        எனில் (!m_isComplete) {
            எனில் (considerUndefs) {
                க்கு (மாறிலி uint64_t from : m_undefinedStates) {
                    மாறிலி std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        புதிய AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } இல்லையெனில் {
                arcCasep->addItemsp(புதிய AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        திரும்பு arcCasep;
    }
    Ast_முனை_Stmt* buildUnxpandedTransitionHandler(மாறிலி FsmStateVertex* மாறிலி toStatep) {
        AstCase* மாறிலி arcCasep = புதிய AstCase{
            m_flp, VCaseType::CT_CASE, புதிய AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        க்கு (மாறிலி V3GraphEdge& edge : toStatep->inEdges()) {
            மாறிலி FsmArcEdge* மாறிலி arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            மாறிலி FsmStateVertex* மாறிலி fromp = arcp->fromp()->as<FsmStateVertex>();
            எனில் (fromp != toStatep) {
                arcCasep->addItemsp(
                    புதிய AstCaseItem{m_flp, makeConst(fromp->மதிப்பு().இலக்கம்().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(புதிய AstCaseItem{m_flp, nullptr, nullptr});
        திரும்பு arcCasep;
    }

பொது:
    FsmCoverIncBuilder(மாறிலி FsmGraph& graph, AstVarScope* மாறிலி prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* மாறிலி sampleVscp = graph.sampleVarScopep();
        மாறிலி VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            மாறிலி size_t arc_மொத்தம்_With_Expansion = get_Arc_மொத்தம்_With_Expansion(expand);
            எனில் (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_மொத்தம்_With_Expansion == 0ULL
                            || arc_மொத்தம்_With_Expansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_மொத்தம்_With_Expansion);
                திரும்பு;
            }
        }

        AstCase* மாறிலி arcCasep
            = புதிய AstCase{m_flp, VCaseType::CT_CASE,
                          புதிய AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* மாறிலி initialStateCasep
            = புதிய AstCase{m_flp, VCaseType::CT_CASE,
                          புதிய AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        மாறிலி தருக்க expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<மாறிலி FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<மாறிலி FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        எனில் (!expandAll) {
            எனில் (m_defaultVertexp) {
                க்கு (மாறிலி V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.உச்சி()->as<FsmStateVertex>());
                }
            }
            எனில் (m_resetVertexp) {
                க்கு (மாறிலி V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.உச்சி()->as<FsmStateVertex>());
                }
            }
        }
        மாறிலி தருக்க isFullExpansion = expand == VFsmExpandType::FULL;
        எனில் (isFullExpansion) initUndefinedStates();
        க்கு (மாறிலி FsmStateVertex* மாறிலி statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* மாறிலி declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                புதிய AstCaseItem{m_flp, makeConst(statep->மதிப்பு().இலக்கம்().toUQuad()),
                                புதிய AstCoverInc{m_flp, declp, உண்மை}});
            மாறிலி தருக்க expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            மாறிலி தருக்க defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(புதிய AstCaseItem{
                m_flp, makeConst(statep->மதிப்பு().இலக்கம்().toUQuad()),
                Ast_முனை::addNext<Ast_முனை_Stmt>(
                    புதிய AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        எனில் (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            எனில் (isFullExpansion) {
                க்கு (மாறிலி uint64_t மதிப்பு : m_undefinedStates) {
                    மாறிலி std::string& label = quadToBitStirngLabel(மதிப்பு);
                    AstCoverOtherDecl* மாறிலி declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(புதிய AstCaseItem{
                        m_flp, makeConst(மதிப்பு), புதிய AstCoverInc{m_flp, declp, உண்மை}});
                    arcCasep->addItemsp(
                        புதிய AstCaseItem{m_flp, makeConst(மதிப்பு),
                                        Ast_முனை::addNext<Ast_முனை_Stmt>(
                                            புதிய AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, உண்மை, பொய்))});
                }
            } இல்லையெனில் {
                // No expansion
                arcCasep->addItemsp(புதிய AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(புதிய AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = இயல்பு;
    Ast_முனை_Cover_Decl* getCoverDeclsp() மாறிலி { திரும்பு m_declsp; }
    Ast_முனை_Stmt* getCoverIncp() மாறிலி { திரும்பு m_coverIncp; }
    Ast_முனை_Stmt* getInitialCoverIncp() மாறிலி { திரும்பு m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
வகுப்பு FsmLowerVisitor final {
    // STATE - across all visitors
    மாறிலி FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    நிலையான Ast_Sen_மரம்* build_Sen_மரம்(File_வரி* flp, மாறிலி std::vector<FsmSenDesc>& senses) {
        Ast_Sen_மரம்* மாறிலி sentreep = புதிய Ast_Sen_மரம்{flp, nullptr};
        க்கு (மாறிலி FsmSenDesc& sense : senses) {
            AstSenItem* மாறிலி senItemp
                = புதிய AstSenItem{flp, VEdgeType{sense.edgeType},
                                 புதிய AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        திரும்பு sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    வெறுமை buildOne(மாறிலி FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* மாறிலி alwaysp = graph.stateAlwaysp();
        AstScope* மாறிலி scopep = graph.scopep();
        AstVarScope* மாறிலி stateVscp = graph.stateVarScopep();
        AstVarScope* மாறிலி sampleVscp = graph.sampleVarScopep();
        File_வரி* மாறிலி flp = graph.fileline();
        Ast_முனை_Module* மாறிலி modp = scopep->modp();
        AstVarScope* மாறிலி prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(உண்மை);
        sampleVscp->optimizeLifePost(உண்மை);
        prevVscp->optimizeLifePost(உண்மை);

        AstActive* மாறிலி initActivep
            = புதிய AstActive{flp, "fsm-coverage-init",
                            புதிய Ast_Sen_மரம்{flp, புதிய AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_மரம்_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(புதிய AstInitialStatic{
            flp, புதிய AstAssign{flp, புதிய AstVarRef{flp, prevVscp, VAccess::WRITE},
                               புதிய AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* மாறிலி covPostp = புதிய AstAlwaysPost{flp};
        தருக்க updatePrevAfterPost = பொய்;
        எனில் (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 புதிய AstVarRef{flp, sampleVscp, VAccess::READ}});
        } இல்லையெனில் {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = உண்மை;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            மாறிலி FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                புதிய AstIf{flp,
                          புதிய AstNeq{flp, புதிய AstVarRef{flp, prevVscp, VAccess::READ},
                                     புதிய AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        எனில் (updatePrevAfterPost) {
            covPostp->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              புதிய AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_மரம்* மாறிலி sentreep = build_Sen_மரம்(flp, graph.senses());
        AstActive* மாறிலி activep = புதிய AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_மரம்_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

பொது:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(மாறிலி FsmState& state)
        : m_state{state} {
        க்கு (மாறிலி DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

வெறுமை V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    எனில் (dumpGraphLevel() >= 6) {
        size_t index = 0;
        க்கு (மாறிலி DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_மரம்("fsm-detect", 0, dump_மரம்_Either_Level() >= 3);
}
