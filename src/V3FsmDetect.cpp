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

#ضمن "V3PchAstNoMT.h"

#ضمن "V3FsmDetect.h"

#ضمن "V3Ast.h"
#ضمن "V3Control.h"
#ضمن "V3Graph.h"
#ضمن "V3SharedTmps.h"

#ضمن <algorithm>
#ضمن <cctype>
#ضمن <map>
#ضمن <memory>
#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
صنف Fsm_State_ارزښت final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_کلی;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_شمېره m_عدد;  // Original folded value, preserving width for lowered comparisons

    ساكن string make_کلی(ثابت V3_شمېره& عدد) {
        V3_شمېره کلی_عدد = عدد;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        کلی_عدد.isSigned(خطأ);
        أرجع cvt_منزل_Str(کلی_عدد.پلنوالی()) + ":" + کلی_عدد.ascii(ریښتیا, ریښتیا);
    }

عام:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_ارزښت()
        : m_کلی{"1:1'h0"}
        , m_عدد{static_cast<Ast_غوټه*>(تش_اشاره), 1, 0} {}
    څرګند Fsm_State_ارزښت(ثابت V3_شمېره& عدد)
        : m_کلی{make_کلی(عدد)}
        , m_عدد{عدد} {}

    ثابت string& کلی() ثابت { أرجع m_کلی; }
    ثابت V3_شمېره& عدد() ثابت { أرجع m_عدد; }
    string ascii() ثابت { أرجع m_عدد.ascii(ریښتیا, ریښتیا); }
    string warn_متن() ثابت {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        إذا (m_عدد.پلنوالی() <= 32) أرجع cvt_منزل_Str(m_عدد.منزل_UInt());
        أرجع ascii();
    }

    منطقي چلوونکی==(ثابت Fsm_State_ارزښت& rhs) ثابت { أرجع m_کلی == rhs.m_کلی; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
بنية Fsm_State_ارزښت_Hash final {
    اندازه_t چلوونکی()(ثابت Fsm_State_ارزښت& ارزښت) ثابت {
        أرجع std::hash<string>{}(ارزښت.کلی());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
بنية FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en څنډه_Type = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = تش_اشاره;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
بنية FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = تش_اشاره;
    منطقي فعال_ټیټ = خطأ;
};

صنف FsmResetArcDesc final {
    Fsm_State_ارزښت m_منزل_ارزښت;  // Encoded reset target state.
    Ast_غوټه* m_nodep = تش_اشاره;  // Source node for warnings and emitted metadata.
    Ast_غوټه_بیان* m_valuep = تش_اشاره;  // Expression that provided the reset value.

عام:
    FsmResetArcDesc() = افتراضي;
    FsmResetArcDesc(Fsm_State_ارزښت منزل_ارزښت, Ast_غوټه_ټاکل* nodep)
        : m_منزل_ارزښت{منزل_ارزښت}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_ارزښت منزل_ارزښت, Ast_غوټه* nodep, Ast_غوټه_بیان* valuep)
        : m_منزل_ارزښت{منزل_ارزښت}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_ارزښت منزل_ارزښت() ثابت { أرجع m_منزل_ارزښت; }
    Ast_غوټه* nodep() ثابت { أرجع m_nodep; }
    Ast_غوټه_بیان* valuep() ثابت { أرجع m_valuep; }
};

بنية FsmWrapperRoles final {
    string d_پورټ;
    string q_پورټ;
    string clk_پورټ;
    string rst_پورټ;
    string rstValParam;
    منطقي has_Rst_فعال_ټیټ = خطأ;
    منطقي rst_فعال_ټیټ = خطأ;
};

ساكن منطقي fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ثابت string& پورټ_Name,
                                                  منطقي& فعال_ټیټ) {
    منطقي matched = خطأ;
    cellp->modp()->foreach([&](Ast_Sen_توکی* itemp) {
        Ast_غوټه_Var_Ref* ثابت vrefp = itemp->varrefp();
        إذا (!vrefp) أرجع;
        إذا (vrefp->varp()->name() != پورټ_Name) أرجع;
        فعال_ټیټ = itemp->څنډه_Type() == VEdgeType::ET_NEGEDGE;
        matched = ریښتیا;
    });
    أرجع matched;
}

ساكن ثابت V3Control::Fsm_ثبتوونکی_Wrapper* fsm_ثبتوونکی_Wrapper_Desc(AstCell* cellp) {
    Ast_غوټه_Module* ثابت modp = cellp->modp();
    ثابت string origName = modp->origName();
    إذا (ثابت V3Control::Fsm_ثبتوونکی_Wrapper* ثابت descp
        = V3Control::get_Fsm_ثبتوونکی_Wrapper(origName)) {
        أرجع descp;
    }
    أرجع V3Control::get_Fsm_ثبتوونکی_Wrapper(modp->prettyDehashOrigOrName());
}

ساكن FsmWrapperRoles rolesFromDesc(ثابت V3Control::Fsm_ثبتوونکی_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_پورټ = desc.d;
    roles.q_پورټ = desc.q;
    roles.clk_پورټ = desc.clock;
    roles.rst_پورټ = desc.reset;
    roles.rstValParam = desc.reset_ارزښت;
    أرجع roles;
}

صنف Fsm_ثبتوونکی_Candidate final {
    AstScope* m_scopep = تش_اشاره;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = تش_اشاره;  // Register process that commits the state.
    AstVarScope* m_stateVscp = تش_اشاره;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = تش_اشاره;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_بل_Vscp = تش_اشاره;  // Next-state variable or same state var for 1-block FSMs.
    std::ویکټور<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::ویکټور<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    منطقي m_hasResetCond = خطأ;  // Whether the FSM had a modeled reset predicate.
    منطقي m_inclCond = خطأ;  // Whether conditional/default arcs are kept explicitly.
    File_کرښه* m_flp = تش_اشاره;  // Representative source location.

عام:
    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
    فراغ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ثابت { أرجع m_stateVscp; }
    فراغ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ثابت { أرجع m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    فراغ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* بل_Vscp() ثابت { أرجع m_بل_Vscp; }
    فراغ بل_Vscp(AstVarScope* vscp) { m_بل_Vscp = vscp; }
    ثابت std::ویکټور<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::ویکټور<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    ثابت std::ویکټور<FsmResetArcDesc>& resetArcs() ثابت { أرجع m_resetArcs; }
    std::ویکټور<FsmResetArcDesc>& resetArcs() { أرجع m_resetArcs; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    File_کرښه* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_کرښه* flp) { m_flp = flp; }
};

صنف FsmComboAlways final {
    AstScope* ثابت m_scopep = تش_اشاره;  // Owning scope for the combinational process.
    AstAlways* ثابت m_alwaysp = تش_اشاره;  // Candidate transition process.

عام:
    FsmComboAlways() = افتراضي;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ثابت { أرجع m_scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
};

صنف Fsm_ګراف;

صنف Fsm_راس VL_NOT_FINAL : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Fsm_راس, V3_ګراف_راس)

عام:
    تعداد صنف ډول_2 : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

خاص:
    ډول_2 m_ډول;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_ارزښت m_ارزښت;  // Encoded state value for real state vertices.

محمي:
    Fsm_راس(V3_ګراف* graphp, ډول_2 ډول, string label, Fsm_State_ارزښت ارزښت) VL_MT_DISABLED
        : V3_ګراف_راس{graphp},
          m_ډول{ډول},
          m_label{label},
          m_ارزښت{ارزښت} {}
    ~Fsm_راس() override = افتراضي;

عام:
    ډول_2 ډول() ثابت { أرجع m_ډول; }
    منطقي isState() ثابت { أرجع m_ډول == ډول_2::STATE; }
    منطقي isResetAny() ثابت { أرجع m_ډول == ډول_2::RESET_ANY; }
    منطقي isDefaultAny() ثابت { أرجع m_ډول == ډول_2::DEFAULT_ANY; }
    ثابت string& label() ثابت { أرجع m_label; }
    ثابت Fsm_State_ارزښت& ارزښت() ثابت { أرجع m_ارزښت; }

    string name() ثابت override VL_MT_خوندي { أرجع m_label + "=" + m_ارزښت.ascii(); }
};

صنف Fsm_State_راس final : عام Fsm_راس {
    VL_RTTI_IMPL(Fsm_State_راس, Fsm_راس)

عام:
    Fsm_State_راس(V3_ګراف* graphp, string label, Fsm_State_ارزښت ارزښت) VL_MT_DISABLED
        : Fsm_راس{graphp, ډول_2::STATE, label, ارزښت} {}
    ~Fsm_State_راس() override = افتراضي;

    string dotColor() ثابت override { أرجع "lightblue"; }
    string dotShape() ثابت override { أرجع "ellipse"; }
};

صنف Fsm_Pseudo_راس final : عام Fsm_راس {
    VL_RTTI_IMPL(Fsm_Pseudo_راس, Fsm_راس)

عام:
    Fsm_Pseudo_راس(V3_ګراف* graphp, ډول_2 ډول, string label) VL_MT_DISABLED
        : Fsm_راس{graphp, ډول, label, Fsm_State_ارزښت{}} {}
    ~Fsm_Pseudo_راس() override = افتراضي;

    string name() ثابت override VL_MT_خوندي { أرجع label(); }
    string dotColor() ثابت override { أرجع isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ثابت override { أرجع "diamond"; }
};

صنف Fsm_Arc_څنډه final : عام V3_ګراف_څنډه {
    VL_RTTI_IMPL(Fsm_Arc_څنډه, V3_ګراف_څنډه)
    منطقي m_isReset = خطأ;  // Arc originates from the synthetic reset source.
    منطقي m_isCond = خطأ;  // Arc came from a conditional next-state split.
    منطقي m_isDefault = خطأ;  // Arc represents a case default source.
    File_کرښه* m_flp = تش_اشاره;  // Source location for emitted coverage metadata.

عام:
    Fsm_Arc_څنډه(V3_ګراف* graphp, Fsm_راس* fromp, Fsm_State_راس* سر, منطقي isReset, منطقي isCond,
               منطقي isDefault, File_کرښه* flp) VL_MT_DISABLED : V3_ګراف_څنډه{graphp, fromp, سر, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~Fsm_Arc_څنډه() override = افتراضي;

    منطقي isReset() ثابت { أرجع m_isReset; }
    منطقي isCond() ثابت { أرجع m_isCond; }
    منطقي isDefault() ثابت { أرجع m_isDefault; }
    File_کرښه* fileline() ثابت { أرجع m_flp; }

    string dotLabel() ثابت override {
        إذا (m_isReset) أرجع "reset";
        إذا (m_isDefault) أرجع "default";
        إذا (m_isCond) أرجع "cond";
        أرجع "";
    }
    string dotColor() ثابت override {
        إذا (m_isReset) أرجع "darkgreen";
        إذا (m_isDefault) أرجع "orange";
        إذا (m_isCond) أرجع "blue";
        أرجع "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
صنف Fsm_ګراف final : عام V3_ګراف {
    AstScope* m_scopep = تش_اشاره;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = تش_اشاره;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_دننی_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = تش_اشاره;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = تش_اشاره;  // Scoped variable sampled by coverage logic.
    std::ویکټور<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    منطقي m_hasResetCond = خطأ;  // Whether the detected FSM had a reset branch.
    منطقي m_inclCond = خطأ;  // Whether conditional arcs should be kept explicitly.
    File_کرښه* m_flp = تش_اشاره;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_ارزښت, Fsm_State_راس*, Fsm_State_ارزښت_Hash>
        m_stateVertices;  // Value to state map.
    Fsm_Pseudo_راس* m_resetVertexp = تش_اشاره;  // Synthetic ANY source for reset arcs.
    Fsm_Pseudo_راس* m_defaultVertexp = تش_اشاره;  // Synthetic default source for case defaults.

عام:
    Fsm_ګراف() VL_MT_DISABLED
        : m_resetVertexp{جديد Fsm_Pseudo_راس{هذا, Fsm_راس::ډول_2::RESET_ANY, "ANY"}},
          m_defaultVertexp{جديد Fsm_Pseudo_راس{هذا, Fsm_راس::ډول_2::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ثابت { أرجع m_stateAlwaysp; }
    فراغ stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ثابت string& stateVarName() ثابت { أرجع m_stateVarName; }
    فراغ stateVarName(ثابت string& name) { m_stateVarName = name; }
    ثابت string& state_Var_دننی_Name() ثابت { أرجع m_state_Var_دننی_Name; }
    فراغ state_Var_دننی_Name(ثابت string& name) { m_state_Var_دننی_Name = name; }
    AstVarScope* stateVarScopep() ثابت { أرجع m_stateVarScopep; }
    فراغ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ثابت {
        أرجع m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    فراغ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ثابت std::ویکټور<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::ویکټور<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    File_کرښه* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_کرښه* flp) { m_flp = flp; }

    Fsm_State_راس* جمع_کړه_State_راس(string label, Fsm_State_ارزښت ارزښت) VL_MT_DISABLED {
        Fsm_State_راس* ثابت vertexp = جديد Fsm_State_راس{هذا, label, ارزښت};
        m_stateVertices.emplace(ارزښت, vertexp);
        أرجع vertexp;
    }
    Fsm_Pseudo_راس* reset_Any_راس() VL_MT_DISABLED { أرجع m_resetVertexp; }
    Fsm_Pseudo_راس* default_Any_راس() VL_MT_DISABLED { أرجع m_defaultVertexp; }
    Fsm_Arc_څنډه* جمع_کړه_Arc(Fsm_State_ارزښت from_ارزښت, Fsm_State_ارزښت منزل_ارزښت, منطقي isReset, منطقي isCond,
                       منطقي isDefault, File_کرښه* flp) VL_MT_DISABLED {
        Fsm_State_راس* ثابت سر = m_stateVertices.at(منزل_ارزښت);
        Fsm_راس* fromp = تش_اشاره;
        إذا (isReset) {
            fromp = reset_Any_راس();
        } وإلا إذا (isDefault) {
            fromp = default_Any_راس();
        } وإلا {
            fromp = m_stateVertices.at(from_ارزښت);
        }
        أرجع جديد Fsm_Arc_څنډه{هذا, fromp, سر, isReset, isCond, isDefault, flp};
    }

    string name() ثابت VL_MT_خوندي {
        أرجع "FSM "
               + (m_stateVarName.تشه() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_نښان(اندازه_t شاخص) ثابت {
        string نښان = state_Var_دننی_Name();
        لكل (حرف& ch : نښان) {
            إذا (!std::که_توری_یا_عدد(static_cast<بینښه حرف>(ch))) ch = '_';
        }
        أرجع "fsm_" + cvt_منزل_Str(شاخص) + "_" + نښان;
    }
};

بنية DetectedFsm final {
    std::unique_ptr<Fsm_ګراف> graphp;  // Extracted graph for one detected FSM candidate.
};

بنية FsmCaseCandidate final {
    Ast_غوټه* warnNodep = تش_اشاره;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = تش_اشاره;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
بنية FsmStateComparison final {
    AstVarScope* stateVscp = تش_اشاره;  // Compared state variable
    Ast_غوټه_بیان* valuep = تش_اشاره;  // Compared constant value expression
    Fsm_State_ارزښت ارزښت;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
بنية FsmIfBranch final {
    AstIf* ifp = تش_اشاره;  // Source if/else-if node
    Ast_غوټه* stmtsp = تش_اشاره;  // Branch body
    Ast_غوټه_بیان* valuep = تش_اشاره;  // Source state value expression
    Fsm_State_ارزښت from_ارزښت;  // Encoded source state value
    منطقي has_سر_Guard = خطأ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
بنية FsmIfChainCandidate final {
    AstIf* ifp = تش_اشاره;  // Top-level if-chain node
    AstVarScope* compareVscp = تش_اشاره;  // Variable used by every state comparison
    std::ویکټور<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_غوټه* defaultStmtsp = تش_اشاره;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
کارول FsmAliasMap = std::unordered_map<ثابت AstVarScope*, FsmStateComparison>;
کارول Fsm_Cell_پورټ_Map = std::unordered_map<string, AstVarScope*>;
کارول Fsm_Cell_پورټ_Alias_Map = std::unordered_map<ثابت AstCell*, Fsm_Cell_پورټ_Map>;

بنية StateConstLabel final {
    string متن;
    منطقي fromParam = خطأ;
    اندازه_t state_شاخص = 0;
};

بنية FsmStateSpace final {
    std::ویکټور<std::pair<string, Fsm_State_ارزښت>> حالتونه;  // User label and encoded value
    std::unordered_map<Fsm_State_ارزښت, StateConstLabel, Fsm_State_ارزښت_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = تش_اشاره;  // Tracked FSM state variable
    منطقي enumBacked = خطأ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
صنف FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::ویکټور<DetectedFsm> m_fsms;
    std::map<ثابت AstVarScope*, اندازه_t> m_fsm_شاخص;

عام:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ثابت std::map<ثابت AstVarScope*, اندازه_t>::const_iterator it = m_fsm_شاخص.پیدا_کړه(stateVscp);
        إذا (it != m_fsm_شاخص.پای_ټکی()) أرجع m_fsms.at(it->دوهم);
        ثابت اندازه_t شاخص = m_fsms.اندازه();
        m_fsm_شاخص.emplace(stateVscp, شاخص);
        m_fsms.emplace_شا();
        أرجع m_fsms.شا();
    }
    ثابت std::ویکټور<DetectedFsm>& fsms() ثابت { أرجع m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
صنف FsmDetectVisitor final : عام VNVisitor {
    // STATE - for current visit position (use VL_RESTORER)
    FsmState& m_state;
    AstScope* m_scopep = تش_اشاره;
    std::ویکټور<Fsm_ثبتوونکی_Candidate> m_ثبتوونکی_Candidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::ویکټور<FsmComboAlways> m_one_بلاک_Alwayss;
    std::ویکټور<FsmComboAlways> m_comboAlwayss;
    std::ویکټور<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::ویکټور<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_سېټ<ثابت AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    Fsm_Cell_پورټ_Alias_Map m_cell_پورټ_Aliases;
    Fsm_Cell_پورټ_Alias_Map m_cell_پورټ_ماشوم_Aliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ساكن Ast_غوټه_DType* unwrapEnumCandidate(Ast_غوټه_DType* dtypep) {
        أرجع dtypep->skip_Ref_منزل_Enump();
    }

    ساكن string candidateConflictContext(Ast_غوټه* laterNodep,
                                           ثابت FsmCaseCandidate& لومړی_Cand) {
        أرجع '\n' + laterNodep->warnContextPrimary() + لومړی_Cand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + لومړی_Cand.stateVscp->prettyNameQ() + '\n'
               + لومړی_Cand.warnNodep->warnContextSecondary();
    }

    ساكن منطقي rejectFsmWrapperCell(AstCell* cellp, ثابت string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        أرجع خطأ;
    }

    ساكن منطقي simple_Param_State_ارزښت(AstCell* cellp, ثابت string& name, Fsm_State_ارزښت& ارزښت,
                                      Ast_غوټه_بیان*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = تش_اشاره;
        لكل (Ast_غوټه* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isParam() || varp->name() != name) استمر;
            valuepr = VN_AS(varp->valuep(), غوټه_بیان);
            أرجع const_ارزښت_حالت(valuepr, ارزښت) == Const_ارزښت_حالت::OK;
        }
        أرجع خطأ;
    }

    ساكن منطقي ماشوم_پورټ_دننه_Scope(AstVarScope* vscp, AstScope* پلار_Scopep, AstCell*& cellpr) {
        إذا (!vscp->varp()->isIO()) أرجع خطأ;
        AstScope* ثابت scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        إذا (scopep->aboveScopep() != پلار_Scopep) أرجع خطأ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        أرجع ریښتیا;
    }

    ساكن AstVarScope* simple_ټاکل_Var_Scope(Ast_غوټه_بیان* exprp) {
        AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef);
        أرجع vrefp ? vrefp->varScopep() : تش_اشاره;
    }

    فراغ جمع_کړه_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_پورټ_Aliases.emplace(cellp, Fsm_Cell_پورټ_Map{});
        m_cell_پورټ_ماشوم_Aliases.emplace(cellp, Fsm_Cell_پورټ_Map{});
        ثابت std::pair<AstScope*, AstCell*> توکی_2{scopep, cellp};
        إذا (std::پیدا_کړه(m_wrapperCells.cbegin(), m_wrapperCells.cend(), توکی_2)
            != m_wrapperCells.cend()) {
            أرجع;
        }
        m_wrapperCells.emplace_شا(توکی_2);
    }

    فراغ collect_Cell_پورټ_Alias(Ast_ټاکل_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ثابت lhsVscp = simple_ټاکل_Var_Scope(nodep->lhsp());
        AstVarScope* ثابت rhsVscp = simple_ټاکل_Var_Scope(nodep->rhsp());
        إذا (!lhsVscp || !rhsVscp) أرجع;
        AstCell* cellp = تش_اشاره;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        إذا (ماشوم_پورټ_دننه_Scope(lhsVscp, m_scopep, cellp)) {
            إذا (!fsm_ثبتوونکی_Wrapper_Desc(cellp)) أرجع;
            UASSERT_OBJ(lhsVscp->varp()->is_ننوت(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_پورټ_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_پورټ_ماشوم_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            جمع_کړه_Wrapper_Cell(m_scopep, cellp);
        } وإلا إذا (ماشوم_پورټ_دننه_Scope(rhsVscp, m_scopep, cellp)) {
            إذا (!fsm_ثبتوونکی_Wrapper_Desc(cellp)) أرجع;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cell_پورټ_Aliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cell_پورټ_ماشوم_Aliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            جمع_کړه_Wrapper_Cell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ثابت string& پورټ_Name) ثابت {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ثابت Fsm_Cell_پورټ_Map& ports = m_cell_پورټ_Aliases.at(cellp);
        ثابت Fsm_Cell_پورټ_Map::const_iterator پورټ_It = ports.پیدا_کړه(پورټ_Name);
        أرجع پورټ_It == ports.پای_ټکی() ? تش_اشاره : پورټ_It->دوهم;
    }

    AstVarScope* ماشوم_Role_Var_Scope(AstCell* cellp, ثابت string& پورټ_Name) ثابت {
        ثابت Fsm_Cell_پورټ_Map& ports = m_cell_پورټ_ماشوم_Aliases.at(cellp);
        ثابت Fsm_Cell_پورټ_Map::const_iterator پورټ_It = ports.پیدا_کړه(پورټ_Name);
        أرجع پورټ_It == ports.پای_ټکی() ? تش_اشاره : پورټ_It->دوهم;
    }

    صنف ثبتوونکی_Always_Analyzer final {
        AstScope* ثابت m_scopep;  // Scope containing the always block(s) being analyzed

    عام:
        څرګند ثبتوونکی_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::ویکټور<std::pair<AstCase*, Ast_غوټه_بیان*>>
        one_بلاک_Candidates(AstAlways* alwaysp) ثابت {
            std::ویکټور<std::pair<AstCase*, Ast_غوټه_بیان*>> candidates;
            Ast_غوټه* ثابت stmtsp = alwaysp->stmtsp();
            إذا (AstIf* ثابت لومړی_Ifp = VN_CAST(stmtsp, If)) {
                إذا (AstCase* ثابت casep = VN_CAST(لومړی_Ifp->elsesp(), Case)) {
                    candidates.emplace_شا(casep,
                                            FsmDetectVisitor::isSimpleResetCond(لومړی_Ifp->condp())
                                                ? لومړی_Ifp->condp()
                                                : تش_اشاره);
                }
            }
            لكل (Ast_غوټه* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstCase* ثابت casep = VN_CAST(nodep, Case))
                    candidates.emplace_شا(casep, تش_اشاره);
            }
            أرجع candidates;
        }

        std::ویکټور<std::pair<AstIf*, Ast_غوټه_بیان*>>
        one_بلاک_If_Candidates(AstAlways* alwaysp) ثابت {
            std::ویکټور<std::pair<AstIf*, Ast_غوټه_بیان*>> candidates;
            Ast_غوټه* ثابت stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            إذا (AstIf* ثابت لومړی_Ifp = VN_CAST(stmtsp, If)) {
                إذا (AstIf* ثابت chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(لومړی_Ifp->elsesp()), If)) {
                    candidates.emplace_شا(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(لومړی_Ifp->condp())
                                                ? لومړی_Ifp->condp()
                                                : تش_اشاره);
                }
            }
            لكل (Ast_غوټه* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) candidates.emplace_شا(ifp, تش_اشاره);
            }
            أرجع candidates;
        }

        منطقي سمون_ثبتوونکی_Candidate(AstAlways* alwaysp, Fsm_ثبتوونکی_Candidate& cand) ثابت {
            أرجع FsmDetectVisitor::سمون_ثبتوونکی_Always(alwaysp, m_scopep, cand);
        }

        فراغ build_One_بلاک_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_غوټه_بیان* resetCondp,
                                    Fsm_ثبتوونکی_Candidate& reg) ثابت {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.بل_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_ونه(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != تش_اشاره);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ثابت لومړی_Ifp = VN_CAST(alwaysp->stmtsp(), If);
            إذا (لومړی_Ifp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = تش_اشاره;
                ثابت Reset_ټاکل_حالت reset_حالت = FsmDetectVisitor::collectConstStateAssigns(
                    لومړی_Ifp->thensp(), resetStateVscp, reg.resetArcs());
                إذا (reset_حالت == Reset_ټاکل_حالت::تشه_3) {
                    reg.resetArcs().clear();
                } وإلا إذا (reset_حالت == Reset_ټاکل_حالت::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_ارزښت reset_ارزښت;
                    Ast_غوټه* ثابت thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(لومړی_Ifp->thensp());
                    UASSERT_OBJ(thenNodep, لومړی_Ifp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_غوټه_ټاکل* ثابت resetAssp = FsmDetectVisitor::مستقیم_Const_State_ټاکل_غوټه(
                        thenNodep, resetStateVscp, reset_ارزښت);
                    إذا (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_شا(reset_ارزښت, resetAssp);
                    }
                } وإلا إذا (reset_حالت == Reset_ټاکل_حالت::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    منطقي سمون_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_ثبتوونکی_Candidate& cand) ثابت {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_ثبتوونکی_Wrapper_Desc(cellp));

        AstVarScope* ثابت بل_Vscp = roleVarScope(cellp, roles.d_پورټ);
        AstVarScope* ثابت stateVscp = roleVarScope(cellp, roles.q_پورټ);
        إذا (!بل_Vscp || !stateVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ثابت clkVscp = roleVarScope(cellp, roles.clk_پورټ);
        إذا (!clkVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.څنډه_Type = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ټېل_شا(clkSense);

        AstVarScope* resetVscp = تش_اشاره;
        إذا (!roles.rst_پورټ.تشه()) resetVscp = roleVarScope(cellp, roles.rst_پورټ);
        إذا (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            منطقي inferred_فعال_ټیټ = خطأ;
            إذا (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_پورټ, inferred_فعال_ټیټ)) {
                roles.has_Rst_فعال_ټیټ = ریښتیا;
                roles.rst_فعال_ټیټ = inferred_فعال_ټیټ;
            }
        }

        Ast_غوټه_بیان* resetValuep = تش_اشاره;
        Fsm_State_ارزښت reset_ارزښت;
        ثابت منطقي has_Reset_ارزښت
            = !roles.rstValParam.تشه()
              && simple_Param_State_ارزښت(cellp, roles.rstValParam, reset_ارزښت, resetValuep);
        إذا (resetVscp && roles.has_Rst_فعال_ټیټ && has_Reset_ارزښت) {
            FsmSenDesc rstSense;
            rstSense.څنډه_Type = roles.rst_فعال_ټیټ ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ټېل_شا(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().فعال_ټیټ = roles.rst_فعال_ټیټ;
            cand.hasResetCond(ریښتیا);
            cand.resetArcs().emplace_شا(reset_ارزښت, cellp, resetValuep);
        } وإلا إذا (!roles.rst_پورټ.تشه() || !roles.rstValParam.تشه()) {
            string reason;
            إذا (roles.rst_پورټ.تشه()) {
                reason = "reset port is not configured";
            } وإلا إذا (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } وإلا إذا (!roles.has_Rst_فعال_ټیټ) {
                reason = "reset polarity could not be inferred from the wrapper";
            } وإلا إذا (roles.rstValParam.تشه()) {
                reason = "reset_value parameter is not configured";
            } وإلا {
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
        cand.alwaysp(تش_اشاره);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(ماشوم_Role_Var_Scope(cellp, roles.q_پورټ));
        cand.بل_Vscp(بل_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        أرجع ریښتیا;
    }

    صنف ComboAlwaysAnalyzer final {
    عام:
        بنية Combo_سمون final {
            ثابت Fsm_ثبتوونکی_Candidate* matchedp = تش_اشاره;
            Ast_غوټه* warnNodep = تش_اشاره;
        };

    خاص:
        ثابت std::ویکټور<Fsm_ثبتوونکی_Candidate>& m_ثبتوونکی_Candidates;

    عام:
        څرګند ComboAlwaysAnalyzer(ثابت std::ویکټور<Fsm_ثبتوونکی_Candidate>& ثبتوونکی_Candidates)
            : m_ثبتوونکی_Candidates{ثبتوونکی_Candidates} {}

        Combo_سمون سمون_Case(Ast_غوټه* stmtsp, AstCase* casep) ثابت {
            Combo_سمون سمون;
            AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع سمون;
            لكل (ثابت Fsm_ثبتوونکی_Candidate& reg : m_ثبتوونکی_Candidates) {
                إذا (selp->varScopep() == reg.بل_Vscp()) {
                    إذا (!FsmDetectVisitor::has_Canonical_بل_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.بل_Vscp())) {
                        استمر;
                    }
                } وإلا إذا (selp->varScopep() != reg.stateVscp()) {
                    استمر;
                }
                Ast_غوټه* ثابت warnNodep = FsmDetectVisitor::case_Supported_Transition_غوټه(
                    casep, reg.بل_Vscp(), reg.inclCond());
                إذا (!warnNodep) استمر;
                سمون.matchedp = &reg;
                سمون.warnNodep = warnNodep;
            }
            أرجع سمون;
        }

        Combo_سمون سمون_If_Chain(Ast_غوټه* stmtsp, ثابت FsmIfChainCandidate& chain) ثابت {
            Combo_سمون سمون;
            لكل (ثابت Fsm_ثبتوونکی_Candidate& reg : m_ثبتوونکی_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                إذا (chain.compareVscp == reg.بل_Vscp()) {
                    إذا (!FsmDetectVisitor::has_Canonical_بل_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.بل_Vscp())) {
                        استمر;
                    }
                } وإلا إذا (chain.compareVscp != reg.stateVscp()) {
                    استمر;
                }
                Ast_غوټه* ثابت warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_غوټه(chain, reg.بل_Vscp());
                إذا (!warnNodep) استمر;
                سمون.matchedp = &reg;
                سمون.warnNodep = warnNodep;
            }
            أرجع سمون;
        }

        منطقي shouldWarnUnsupported(Ast_غوټه* stmtsp, AstCase* casep) ثابت {
            ثابت AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع خطأ;

            أرجع std::any_د(
                m_ثبتوونکی_Candidates.cbegin(), m_ثبتوونکی_Candidates.cend(),
                [&](ثابت Fsm_ثبتوونکی_Candidate& reg) -> منطقي {
                    ثابت منطقي matches_بل = selp->varScopep() == reg.بل_Vscp();
                    ثابت منطقي matchesState = selp->varScopep() == reg.stateVscp();

                    إذا (!matches_بل && !matchesState) أرجع خطأ;
                    إذا (matches_بل
                        && !FsmDetectVisitor::has_Canonical_بل_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.بل_Vscp())) {
                        أرجع خطأ;
                    }
                    أرجع FsmDetectVisitor::case_Supported_Transition_غوټه(casep, reg.بل_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ساكن منطقي isSimpleResetCond(Ast_غوټه_بیان* condp) { أرجع VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ساكن FsmResetCondDesc describeResetCond(Ast_غوټه_بیان* condp) {
        FsmResetCondDesc desc;
        إذا (AstVarRef* ثابت vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        أرجع desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ساكن std::ویکټور<FsmSenDesc> describe_Sen_ونه(Ast_Sen_ونه* sentreep) {
        std::ویکټور<FsmSenDesc> senses;
        لكل (Ast_Sen_توکی* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), Sen_توکی)) {
            Ast_غوټه_Var_Ref* ثابت vrefp = itemp->varrefp();
            إذا (!vrefp) استمر;
            FsmSenDesc desc;
            desc.څنډه_Type = itemp->څنډه_Type().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ټېل_شا(desc);
        }
        أرجع senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ساكن منطقي isIgnorableStmt(Ast_غوټه* nodep) { أرجع VN_IS(nodep, CoverInc); }

    ساكن Ast_غوټه* skipLeadingIgnorableStmt(Ast_غوټه* nodep) {
        طالما (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ساكن Ast_غوټه* singleMeaningfulStmt(Ast_غوټه* stmtp) {
        Ast_غوټه* resultp = تش_اشاره;
        لكل (Ast_غوټه* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            إذا (isIgnorableStmt(nodep)) استمر;
            إذا (resultp) أرجع تش_اشاره;
            resultp = nodep;
        }
        أرجع resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ساكن Ast_غوټه* singleMeaningfulBranch(Ast_غوټه* branchp) {
        إذا (!branchp) أرجع تش_اشاره;
        أرجع branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ساكن منطقي isPlainComboSentree(ثابت Ast_Sen_ونه* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        لكل (ثابت Ast_Sen_توکی* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), Sen_توکی)) {
            إذا (senp->څنډه_Type() == VEdgeType::ET_بدل_شوی) استمر;
            أرجع خطأ;
        }
        أرجع ریښتیا;
    }

    فراغ warnUnsupportedComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_ثبتوونکی_Candidates};
        Ast_غوټه* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        منطقي warned = خطأ;
        لكل (Ast_غوټه* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            إذا (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = ریښتیا;
            }
            إذا (warned) اكسر;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ساكن Ast_غوټه_ټاکل* مستقیم_State_ټاکل(Ast_غوټه* stmtp, AstVarScope* stateVscp) {
        Ast_غوټه* ثابت nodep = singleMeaningfulBranch(stmtp);
        إذا (!nodep) أرجع تش_اشاره;
        Ast_غوټه_ټاکل* ثابت assp = VN_CAST(nodep, غوټه_ټاکل);
        إذا (!assp) أرجع تش_اشاره;
        AstVarRef* ثابت vrefp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!vrefp || vrefp->varScopep() != stateVscp) أرجع تش_اشاره;
        أرجع assp;
    }

    ساكن Ast_غوټه_ټاکل* غوټه_State_Var_ټاکل(Ast_غوټه* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_غوټه_ټاکل* ثابت assp = VN_CAST(nodep, غوټه_ټاکل);
        إذا (!assp) أرجع تش_اشاره;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنسټ_Fromp(ریښتیا), VarRef);
        AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
        إذا (!rhsp || !lhsp) أرجع تش_اشاره;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        أرجع assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ساكن Ast_غوټه_ټاکل* مستقیم_Cond_State_Var_ټاکل(Ast_غوټه* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_غوټه_بیان*& condp,
                                                   Fsm_State_ارزښت& reset_ارزښت) {
        Ast_غوټه_ټاکل* ثابت assp = VN_CAST(nodep, غوټه_ټاکل);
        إذا (!assp) أرجع تش_اشاره;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنسټ_Fromp(ریښتیا), VarRef);
        AstCond* ثابت rhsp = VN_CAST(assp->rhsp(), Cond);
        إذا (!rhsp || !lhsp) أرجع تش_اشاره;
        AstVarRef* ثابت elsep = VN_CAST(rhsp->elsep(), VarRef);
        إذا (!elsep) أرجع تش_اشاره;
        إذا (const_ارزښت_حالت(rhsp->thenp(), reset_ارزښت) != Const_ارزښت_حالت::OK) أرجع تش_اشاره;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        أرجع assp;
    }

    ساكن Ast_غوټه_ټاکل* مستقیم_Const_State_ټاکل_غوټه(Ast_غوټه* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_ارزښت& ارزښت) {
        Ast_غوټه_ټاکل* ثابت assp = VN_CAST(nodep, غوټه_ټاکل);
        إذا (!assp) أرجع تش_اشاره;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنسټ_Fromp(ریښتیا), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        إذا (const_ارزښت_حالت(assp->rhsp(), ارزښت) != Const_ارزښت_حالت::OK) أرجع تش_اشاره;
        stateVscp = lhsp->varScopep();
        أرجع assp;
    }

    تعداد صنف Reset_ټاکل_حالت : uint8_t {
        تشه_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ساكن Reset_ټاکل_حالت collectConstStateAssigns(Ast_غوټه* stmtp, AstVarScope*& stateVscp,
                                                      std::ویکټور<FsmResetArcDesc>& resetArcs) {
        Ast_غوټه* nodep = skipLeadingIgnorableStmt(stmtp);
        إذا (!nodep) أرجع Reset_ټاکل_حالت::تشه_3;
        لكل (;; nodep = nodep->nextp()) {
            AstVarScope* ټاکل_State_Vscp = تش_اشاره;
            Fsm_State_ارزښت ارزښت;
            Ast_غوټه_ټاکل* ثابت assp = مستقیم_Const_State_ټاکل_غوټه(nodep, ټاکل_State_Vscp, ارزښت);
            إذا (!assp) أرجع Reset_ټاکل_حالت::NONE;
            إذا (!stateVscp) stateVscp = ټاکل_State_Vscp;
            إذا (ټاکل_State_Vscp != stateVscp) أرجع Reset_ټاکل_حالت::NONE;
            إذا (!resetArcs.تشه()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                أرجع Reset_ټاکل_حالت::MULTI_SAME_STATE;
            }
            resetArcs.emplace_شا(ارزښت, assp);
            إذا (!nodep->nextp()) أرجع Reset_ټاکل_حالت::SINGLE;
        }
    }

    ساكن منطقي has_Canonical_بل_State_Default_Before_Case(Ast_غوټه* stmtsp, Ast_غوټه* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* بل_Vscp) {
        Ast_غوټه* ثابت bodyp = skipLeadingIgnorableStmt(stmtsp);
        منطقي sawCanonicalDefault = خطأ;
        لكل (Ast_غوټه* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            إذا (nodep == targetp) أرجع sawCanonicalDefault;
            إذا (Ast_غوټه_ټاکل* ثابت assp = VN_CAST(nodep, غوټه_ټاکل)) {
                AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
                إذا (!lhsp || lhsp->varScopep() != بل_Vscp) استمر;
                إذا (sawCanonicalDefault) {
                    ثابت string بل_Name = بل_Vscp->varp()->prettyNameQ();
                    ثابت string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + بل_Name
                                     + ") when the canonical " + بل_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    أرجع خطأ;
                }
                إذا (!rhsp || rhsp->varScopep() != stateVscp) أرجع خطأ;
                sawCanonicalDefault = ریښتیا;
            }
        }
    }

    ساكن منطقي if_State_Const_ټاکل(Ast_غوټه* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_ارزښت& then_ارزښت, Fsm_State_ارزښت& else_ارزښت) {
        AstIf* ثابت ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        إذا (!ifp || !ifp->elsesp()) أرجع خطأ;
        AstVarScope* thenVscp = تش_اشاره;
        AstVarScope* elseVscp = تش_اشاره;
        Ast_غوټه* ثابت thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_غوټه* ثابت elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        إذا (!thenNodep || !elseNodep) أرجع خطأ;
        إذا (!مستقیم_Const_State_ټاکل_غوټه(thenNodep, thenVscp, then_ارزښت)) أرجع خطأ;
        إذا (!مستقیم_Const_State_ټاکل_غوټه(elseNodep, elseVscp, else_ارزښت)) أرجع خطأ;
        إذا (thenVscp == stateVscp && elseVscp == stateVscp) أرجع ریښتیا;
        إذا (thenVscp != elseVscp) أرجع خطأ;
        Ast_غوټه* ثابت followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = تش_اشاره;
        AstVarScope* finalFromVscp = تش_اشاره;
        Ast_غوټه* ثابت finalNodep = singleMeaningfulBranch(followp);
        إذا (!finalNodep) أرجع خطأ;
        إذا (!غوټه_State_Var_ټاکل(finalNodep, finalStateVscp, finalFromVscp)) أرجع خطأ;
        إذا (finalStateVscp != stateVscp) أرجع خطأ;
        إذا (finalFromVscp != thenVscp) أرجع خطأ;
        أرجع ریښتیا;
    }

    ساكن منطقي مستقیم_State_Cond_Const_ټاکل(Ast_غوټه* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_ارزښت& then_ارزښت, Fsm_State_ارزښت& else_ارزښت) {
        Ast_غوټه_ټاکل* ثابت assp = مستقیم_State_ټاکل(stmtp, stateVscp);
        إذا (!assp) أرجع خطأ;
        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
        إذا (!condp) أرجع خطأ;
        أرجع const_ارزښت_حالت(condp->thenp(), then_ارزښت) == Const_ارزښت_حالت::OK
               && const_ارزښت_حالت(condp->elsep(), else_ارزښت) == Const_ارزښت_حالت::OK;
    }

    ساكن Ast_غوټه* case_توکی_Supported_Arc_غوټه(Ast_Case_توکی* itemp, AstVarScope* stateVscp,
                                             منطقي inclCond) {
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع تش_اشاره;
        }
        Ast_غوټه_ټاکل* ثابت assp = مستقیم_State_ټاکل(itemp->stmtsp(), stateVscp);
        إذا (assp) {
            Fsm_State_ارزښت منزل_ارزښت;
            إذا (const_ارزښت_حالت(assp->rhsp(), منزل_ارزښت) == Const_ارزښت_حالت::OK) أرجع assp;
        }
        Fsm_State_ارزښت then_ارزښت;
        Fsm_State_ارزښت else_ارزښت;
        إذا (مستقیم_State_Cond_Const_ټاکل(itemp->stmtsp(), stateVscp, then_ارزښت, else_ارزښت)) {
            أرجع assp;
        }
        إذا (if_State_Const_ټاکل(itemp->stmtsp(), stateVscp, then_ارزښت, else_ارزښت)) {
            أرجع singleMeaningfulBranch(itemp->stmtsp());
        }
        أرجع تش_اشاره;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ساكن Ast_غوټه* case_Supported_Transition_غوټه(AstCase* casep, AstVarScope* stateVscp,
                                                منطقي inclCond) {
        لكل (Ast_Case_توکی* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_توکی)) {
            إذا (Ast_غوټه* ثابت nodep = case_توکی_Supported_Arc_غوټه(itemp, stateVscp, inclCond))
                أرجع nodep;
        }
        أرجع تش_اشاره;
    }

    ساكن Ast_غوټه* case_توکی_Supported_Arc_غوټه_Like(Ast_غوټه* stmtsp, AstVarScope* stateVscp) {
        إذا (Ast_غوټه_ټاکل* ثابت assp = مستقیم_State_ټاکل(stmtsp, stateVscp)) {
            Fsm_State_ارزښت منزل_ارزښت;
            إذا (const_ارزښت_حالت(assp->rhsp(), منزل_ارزښت) == Const_ارزښت_حالت::OK) أرجع assp;
            Fsm_State_ارزښت then_ارزښت;
            Fsm_State_ارزښت else_ارزښت;
            إذا (مستقیم_State_Cond_Const_ټاکل(stmtsp, stateVscp, then_ارزښت, else_ارزښت)) أرجع assp;
        }
        أرجع تش_اشاره;
    }

    ساكن Ast_غوټه* if_Chain_Supported_Transition_غوټه(ثابت FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        لكل (اندازه_t i = 0; i < chain.branches.اندازه(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            Ast_غوټه* ثابت nodep = case_توکی_Supported_Arc_غوټه_Like(branch.stmtsp, stateVscp);
            إذا (!nodep) أرجع تش_اشاره;
        }
        أرجع chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ساكن string label_For_ارزښت(
        ثابت std::unordered_map<Fsm_State_ارزښت, StateConstLabel, Fsm_State_ارزښت_Hash>& labels,
        ثابت Fsm_State_ارزښت& ارزښت) {
        أرجع labels.at(ارزښت).متن;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    تعداد صنف Const_ارزښت_حالت : uint8_t { OK, NOT_CONST, XZ };

    ساكن Const_ارزښت_حالت const_ارزښت_حالت(Ast_غوټه_بیان* exprp, Fsm_State_ارزښت& ارزښت) {
        ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
        إذا (!constp) أرجع Const_ارزښت_حالت::NOT_CONST;
        ثابت V3_شمېره& عدد = constp->عدد();
        إذا (عدد.isAnyXZ()) أرجع Const_ارزښت_حالت::XZ;
        ارزښت = Fsm_State_ارزښت{عدد};
        أرجع Const_ارزښت_حالت::OK;
    }

    ساكن منطقي خالص_State_Comparison_No_Alias(Ast_غوټه_بیان* exprp, FsmStateComparison& cmp) {
        AstEq* ثابت eqp = VN_CAST(exprp, Eq);
        إذا (!eqp) أرجع خطأ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_غوټه_بیان* valuep = eqp->rhsp();
        إذا (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->بنسټ_Fromp(ریښتیا), VarRef);
            إذا (!vrefp) { أرجع خطأ; }
            valuep = eqp->lhsp();
        }

        Fsm_State_ارزښت ارزښت;
        إذا (const_ارزښت_حالت(valuep, ارزښت) != Const_ارزښت_حالت::OK) أرجع خطأ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.ارزښت = ارزښت;
        أرجع ریښتیا;
    }

    ساكن منطقي خالص_State_Comparison(Ast_غوټه_بیان* exprp, ثابت FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        إذا (خالص_State_Comparison_No_Alias(exprp, cmp)) أرجع ریښتیا;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        إذا (AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) {
            ثابت FsmAliasMap::const_iterator it = aliases.پیدا_کړه(vrefp->varScopep());
            إذا (it == aliases.پای_ټکی()) أرجع خطأ;
            cmp = it->دوهم;
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    ساكن منطقي supported_سر_Level_Guard(Ast_غوټه_بیان* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        إذا (VN_IS(exprp, Or)) أرجع خطأ;
        إذا (VN_IS(exprp, RedAnd)) أرجع خطأ;
        إذا (VN_IS(exprp, RedOr)) أرجع خطأ;
        إذا (VN_IS(exprp, RedXor)) أرجع خطأ;
        أرجع ریښتیا;
    }

    ساكن منطقي resolveIfPredicate(Ast_غوټه_بیان* exprp, ثابت FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, منطقي& hasGuard) {
        std::ویکټور<Ast_غوټه_بیان*> terms;
        std::ویکټور<Ast_غوټه_بیان*> pending;
        pending.ټېل_شا(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        طالما (!pending.تشه()) {
            Ast_غوټه_بیان* ثابت nodep = pending.شا();
            pending.راوباسه_شا();
            إذا (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
                pending.ټېل_شا(andp->rhsp());
                pending.ټېل_شا(andp->lhsp());
            } وإلا {
                terms.ټېل_شا(nodep);
            }
        }

        منطقي sawComparison = خطأ;
        لكل (اندازه_t i = 0; i < terms.اندازه(); ++i) {
            Ast_غوټه_بیان* ثابت termp = terms[i];
            FsmStateComparison غړی_Cmp;
            إذا (خالص_State_Comparison(termp, aliases, غړی_Cmp /*ref*/)) {
                إذا (sawComparison) أرجع خطأ;
                cmp = غړی_Cmp;
                sawComparison = ریښتیا;
                استمر;
            }
            إذا (!supported_سر_Level_Guard(termp)) أرجع خطأ;
            hasGuard = ریښتیا;
        }
        أرجع sawComparison;
    }

    ساكن فراغ جمع_کړه_Alias(FsmAliasMap& aliases, std::unordered_سېټ<ثابت AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ثابت FsmStateComparison& cmp) {
        إذا (ambiguous.پیدا_کړه(aliasVscp) != ambiguous.پای_ټکی()) أرجع;
        ثابت FsmAliasMap::iterator it = aliases.پیدا_کړه(aliasVscp);
        إذا (it == aliases.پای_ټکی()) {
            aliases.emplace(aliasVscp, cmp);
            أرجع;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        إذا (it->دوهم.stateVscp == cmp.stateVscp && it->دوهم.ارزښت == cmp.ارزښت) أرجع;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        أرجع;
    }

    ساكن فراغ collect_Alias_From_ټاکل(Ast_غوټه_ټاکل* assp, FsmAliasMap& aliases,
                                       std::unordered_سېټ<ثابت AstVarScope*>& ambiguous) {
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!lhsp) أرجع;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        إذا (!خالص_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) أرجع;
        جمع_کړه_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_غوټه* stmtsp, Ast_غوټه* limitp) ثابت {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_سېټ<ثابت AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        لكل (Ast_غوټه* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            إذا (Ast_غوټه_ټاکل* ثابت assp = VN_CAST(nodep, غوټه_ټاکل)) {
                collect_Alias_From_ټاکل(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        لكل (ثابت AstVarScope* ثابت vscp : ambiguous) aliases.erase(vscp);
        أرجع aliases;
    }

    ساكن منطقي collectIfChain(AstIf* ifp, ثابت FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_سېټ<string> کتل_شوی_ارزښتونه;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        طالما (ریښتیا) {
            FsmStateComparison cmp;
            منطقي hasGuard = خطأ;
            إذا (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) أرجع خطأ;
            إذا (chain.compareVscp && chain.compareVscp != cmp.stateVscp) أرجع خطأ;
            إذا (!کتل_شوی_ارزښتونه.ورننباسه(cmp.ارزښت.کلی()).دوهم) أرجع خطأ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ټېل_شا(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.ارزښت, hasGuard});

            Ast_غوټه* ثابت elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            إذا (!elseNodep) اكسر;
            إذا (AstIf* ثابت elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                استمر;
            }
            chain.defaultStmtsp = elseNodep;
            اكسر;
        }
        أرجع chain.branches.اندازه() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ساكن منطقي validate_معلوم_State_ارزښت(Ast_غوټه* nodep, ثابت FsmStateSpace& stateSpace,
                                        ثابت Fsm_State_ارزښت& ارزښت, ثابت string& role) {
        إذا (stateSpace.labels.پیدا_کړه(ارزښت) != stateSpace.labels.پای_ټکی()) أرجع ریښتیا;
        إذا (stateSpace.enumBacked) {
            ثابت string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + ارزښت.warn_متن()
                                        + " is not present in the declared enum");
            أرجع خطأ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + ارزښت.warn_متن()
                                    + " is not present in the inferred state space");
        أرجع خطأ;
    }

    ساكن StateConstLabel stateLabelForConst(AstConst* constp) {
        ثابت string name = constp->origParamName();
        إذا (!name.تشه()) أرجع StateConstLabel{Ast_غوټه::prettyName(name), ریښتیا, 0};
        أرجع StateConstLabel{constp->name(), خطأ, 0};
    }

    ساكن فراغ updateStateLabel(FsmStateSpace& stateSpace, ثابت Fsm_State_ارزښت& ارزښت,
                                 ثابت StateConstLabel& label) {
        stateSpace.حالتونه.at(stateSpace.labels.at(ارزښت).state_شاخص).لومړی = label.متن;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ساكن منطقي سمون_ثبتوونکی_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_ثبتوونکی_Candidate& cand) {
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->has_څنډه()) أرجع خطأ;

        Ast_غوټه* ثابت stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_غوټه* ثابت nodep = singleMeaningfulStmt(stmtsp);
        إذا (!nodep) أرجع خطأ;

        AstVarScope* stateVscp = تش_اشاره;
        AstVarScope* بل_Vscp = تش_اشاره;
        إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) أرجع خطأ;
            AstVarScope* resetStateVscp = تش_اشاره;
            ثابت Reset_ټاکل_حالت reset_حالت
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ثابت منطقي تشه_Reset_Branch = reset_حالت == Reset_ټاکل_حالت::تشه_3;
            إذا (تشه_Reset_Branch) {
                cand.resetArcs().clear();
            } وإلا إذا (reset_حالت == Reset_ټاکل_حالت::NONE) {
                cand.resetArcs().clear();
                Fsm_State_ارزښت reset_ارزښت;
                Ast_غوټه* ثابت thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_غوټه_ټاکل* ثابت resetAssp
                    = مستقیم_Const_State_ټاکل_غوټه(thenNodep, resetStateVscp, reset_ارزښت);
                إذا (!resetAssp) أرجع خطأ;
                cand.resetArcs().emplace_شا(reset_ارزښت, resetAssp);
            } وإلا إذا (reset_حالت == Reset_ټاکل_حالت::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_غوټه* ثابت elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            إذا (!غوټه_State_Var_ټاکل(elseNodep, stateVscp, بل_Vscp)) أرجع خطأ;
            إذا (!تشه_Reset_Branch && resetStateVscp != stateVscp) أرجع خطأ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != تش_اشاره);
        } وإلا {
            Ast_غوټه_بیان* resetCondp = تش_اشاره;
            Fsm_State_ارزښت reset_ارزښت;
            // Note the active low 'reset ? next : CONST' form is not accepted
            إذا (Ast_غوټه_ټاکل* ثابت assp
                = مستقیم_Cond_State_Var_ټاکل(nodep, stateVscp, بل_Vscp, resetCondp, reset_ارزښت)) {
                cand.resetArcs().emplace_شا(reset_ارزښت, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != تش_اشاره);
            } وإلا إذا (!غوټه_State_Var_ټاکل(nodep, stateVscp, بل_Vscp)) {
                أرجع خطأ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.بل_Vscp(بل_Vscp);
        cand.senses() = describe_Sen_ونه(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        أرجع ریښتیا;
    }

    ساكن منطقي جمع_کړه_ارزښت_منزل_State_Space(Ast_غوټه* nodep, FsmStateSpace& stateSpace,
                                     ثابت Fsm_State_ارزښت& ارزښت, StateConstLabel label) {
        ثابت خپلکاری labelIt = stateSpace.labels.پیدا_کړه(ارزښت);
        إذا (labelIt != stateSpace.labels.پای_ټکی()) {
            StateConstLabel& existingLabel = labelIt->دوهم;
            إذا (existingLabel.متن != label.متن && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + ارزښت.warn_متن() + ": " + existingLabel.متن
                                            + " and " + label.متن);
                أرجع خطأ;
            }
            إذا (!existingLabel.fromParam && label.fromParam) {
                existingLabel.متن = label.متن;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, ارزښت, label);
            }
        } وإلا {
            StateConstLabel storedLabel = label;
            storedLabel.state_شاخص = stateSpace.حالتونه.اندازه();
            stateSpace.حالتونه.emplace_شا(label.متن, ارزښت);
            stateSpace.labels.emplace(ارزښت, storedLabel);
        }
        أرجع ریښتیا;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ساكن منطقي جمع_کړه_بیان_منزل_State_Space(Ast_غوټه_بیان* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ارزښت ارزښت;
        ثابت Const_ارزښت_حالت حالت = const_ارزښت_حالت(valuep, ارزښت);
        إذا (حالت != Const_ارزښت_حالت::OK) {
            إذا (حالت == Const_ارزښت_حالت::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع جمع_کړه_ارزښت_منزل_State_Space(valuep, stateSpace, ارزښت, stateLabelForConst(constp));
    }

    ساكن منطقي جمع_کړه_Optional_موخه_بیان_منزل_State_Space(Ast_غوټه_بیان* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_ارزښت ارزښت;
        ثابت Const_ارزښت_حالت حالت = const_ارزښت_حالت(valuep, ارزښت);
        إذا (حالت != Const_ارزښت_حالت::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع جمع_کړه_ارزښت_منزل_State_Space(valuep, stateSpace, ارزښت, stateLabelForConst(constp));
    }

    ساكن فراغ جمع_کړه_Reset_Targets_منزل_State_Space(ثابت std::ویکټور<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.منزل_ارزښت().ascii(), خطأ, 0};
            إذا (AstConst* ثابت constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                جمع_کړه_ارزښت_منزل_State_Space(resetArc.nodep(), stateSpace, resetArc.منزل_ارزښت(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ساكن منطقي collectDeclaredStateSpace(Ast_غوټه* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, منطقي& needs_سرچینه_ارزښتونه) {
        AstVar* ثابت stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        إذا (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ثابت منطقي forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        إذا (enump) {
            stateSpace.enumBacked = ریښتیا;
            لكل (Ast_Enum_توکی* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_توکی)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                ثابت Fsm_State_ارزښت ارزښت{constp->عدد()};
                ثابت اندازه_t state_شاخص = stateSpace.حالتونه.اندازه();
                stateSpace.حالتونه.emplace_شا(itemp->name(), ارزښت);
                stateSpace.labels.emplace(ارزښت,
                                          StateConstLabel{itemp->name(), خطأ, state_شاخص});
            }
            أرجع stateSpace.حالتونه.اندازه() >= 2;
        }

        إذا (forced) {
            needs_سرچینه_ارزښتونه = ریښتیا;
            أرجع ریښتیا;
        }

        needs_سرچینه_ارزښتونه = ریښتیا;
        أرجع ریښتیا;
    }

    کينډۍ <ډول_نوم T_ValuepVisitor>
    ساكن منطقي collect_State_Space_From_ارزښتونه(Ast_غوټه* warnNodep, AstVarScope* stateVscp,
                                            ثابت std::ویکټور<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ثابت T_ValuepVisitor& لیدنه_Valueps) {
        منطقي needs_سرچینه_ارزښتونه = خطأ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        إذا (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_سرچینه_ارزښتونه)) {
            أرجع خطأ;
        }
        إذا (!needs_سرچینه_ارزښتونه) أرجع ریښتیا;
        جمع_کړه_Reset_Targets_منزل_State_Space(resetArcs, stateSpace);
        إذا (!لیدنه_Valueps(
                [&](Ast_غوټه_بیان* valuep) { أرجع جمع_کړه_بیان_منزل_State_Space(valuep, stateSpace); })) {
            أرجع خطأ;
        }
        أرجع stateSpace.حالتونه.اندازه() >= 2;
    }

    ساكن منطقي collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* ټاکل_Vscp,
                                  ثابت std::ویکټور<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_ارزښتونه(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, ټاکل_Vscp, &stateSpace](ثابت خپلکاری& لیدنه_Valuep) {
                لكل (Ast_Case_توکی* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Case_توکی)) {
                    إذا (!itemp->isDefault()) {
                        لكل (Ast_غوټه_بیان* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), غوټه_بیان)) {
                            إذا (!لیدنه_Valuep(condp)) أرجع خطأ;
                        }
                    }
                    إذا (Ast_غوټه_ټاکل* ثابت assp
                        = مستقیم_State_ټاکل(itemp->stmtsp(), ټاکل_Vscp)) {
                        Fsm_State_ارزښت then_ارزښت;
                        Fsm_State_ارزښت else_ارزښت;
                        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                        إذا (condp
                            && مستقیم_State_Cond_Const_ټاکل(itemp->stmtsp(), ټاکل_Vscp, then_ارزښت,
                                                          else_ارزښت)) {
                            إذا (!لیدنه_Valuep(condp->thenp())) أرجع خطأ;
                            إذا (!لیدنه_Valuep(condp->elsep())) أرجع خطأ;
                        } وإلا إذا (!جمع_کړه_Optional_موخه_بیان_منزل_State_Space(assp->rhsp(), stateSpace)) {
                            أرجع خطأ;
                        }
                    }
                }
                أرجع ریښتیا;
            });
    }

    ساكن منطقي collectStateSpace(ثابت FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* ټاکل_Vscp,
                                  ثابت std::ویکټور<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_ارزښتونه(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, ټاکل_Vscp, &stateSpace](ثابت خپلکاری& لیدنه_Valuep) {
                لكل (ثابت FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(لیدنه_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_غوټه_ټاکل* ثابت assp = مستقیم_State_ټاکل(branch.stmtsp, ټاکل_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_ارزښت then_ارزښت;
                    Fsm_State_ارزښت else_ارزښت;
                    AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                    إذا (condp) {
                        UASSERT_OBJ(مستقیم_State_Cond_Const_ټاکل(branch.stmtsp, ټاکل_Vscp,
                                                               then_ارزښت, else_ارزښت),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        إذا (!لیدنه_Valuep(condp->thenp())) أرجع خطأ;
                        إذا (!لیدنه_Valuep(condp->elsep())) أرجع خطأ;
                    } وإلا إذا (!جمع_کړه_Optional_موخه_بیان_منزل_State_Space(assp->rhsp(), stateSpace)) {
                        أرجع خطأ;
                    }
                }
                أرجع ریښتیا;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ساكن منطقي emit_Case_توکی_Arcs(Fsm_ګراف& ګراف, Ast_Case_توکی* itemp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, منطقي inclCond) {
        std::ویکټور<std::pair<string, Fsm_State_ارزښت>> froms;
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع خطأ;
            froms.emplace_شا("default", Fsm_State_ارزښت{});
        } وإلا {
            لكل (Ast_غوټه_بیان* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), غوټه_بیان)) {
                Fsm_State_ارزښت ارزښت;
                إذا (const_ارزښت_حالت(condp, ارزښت) != Const_ارزښت_حالت::OK) استمر;
                إذا (!validate_معلوم_State_ارزښت(condp, stateSpace, ارزښت, "source")) أرجع ریښتیا;
                froms.emplace_شا(label_For_ارزښت(stateSpace.labels, ارزښت), ارزښت);
            }
            إذا (froms.تشه()) أرجع خطأ;
        }

        إذا (Ast_غوټه_ټاکل* ثابت assp = مستقیم_State_ټاکل(itemp->stmtsp(), stateVscp)) {
            Fsm_State_ارزښت منزل_ارزښت;
            ثابت Const_ارزښت_حالت حالت = const_ارزښت_حالت(assp->rhsp(), منزل_ارزښت);
            إذا (حالت == Const_ارزښت_حالت::OK) {
                إذا (!validate_معلوم_State_ارزښت(assp, stateSpace, منزل_ارزښت, "target")) أرجع ریښتیا;
                لكل (ثابت std::pair<string, Fsm_State_ارزښت>& from : froms) {
                    ګراف.جمع_کړه_Arc(from.دوهم, منزل_ارزښت, خطأ, خطأ, itemp->isDefault(),
                                 assp->fileline());
                }
                أرجع ریښتیا;
            }
        }

        Fsm_State_ارزښت then_ارزښت;
        Fsm_State_ارزښت else_ارزښت;
        إذا (مستقیم_State_Cond_Const_ټاکل(itemp->stmtsp(), stateVscp, then_ارزښت, else_ارزښت)
            || if_State_Const_ټاکل(itemp->stmtsp(), stateVscp, then_ارزښت, else_ارزښت)) {
            إذا (!validate_معلوم_State_ارزښت(itemp->stmtsp(), stateSpace, then_ارزښت, "target"))
                أرجع ریښتیا;
            إذا (!validate_معلوم_State_ارزښت(itemp->stmtsp(), stateSpace, else_ارزښت, "target"))
                أرجع ریښتیا;
            لكل (ثابت Fsm_State_ارزښت& branch_ارزښت : {then_ارزښت, else_ارزښت}) {
                لكل (ثابت std::pair<string, Fsm_State_ارزښت>& from : froms) {
                    ګراف.جمع_کړه_Arc(from.دوهم, branch_ارزښت, خطأ, ریښتیا, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            أرجع ریښتیا;
        }

        أرجع خطأ;
    }

    ساكن فراغ emitStmtArcsFrom(Fsm_ګراف& ګراف, Ast_غوټه* stmtsp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, Fsm_State_ارزښت from_ارزښت,
                                 منطقي isDefault, منطقي forceCond) {
        Ast_غوټه_ټاکل* ثابت assp = مستقیم_State_ټاکل(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_ارزښت منزل_ارزښت;
        ثابت Const_ارزښت_حالت حالت = const_ارزښت_حالت(assp->rhsp(), منزل_ارزښت);
        إذا (حالت == Const_ارزښت_حالت::OK) {
            إذا (!validate_معلوم_State_ارزښت(assp, stateSpace, منزل_ارزښت, "target")) أرجع;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            ګراف.جمع_کړه_Arc(from_ارزښت, منزل_ارزښت, خطأ, forceCond, isDefault, assp->fileline());
            أرجع;
        }

        Fsm_State_ارزښت then_ارزښت;
        Fsm_State_ارزښت else_ارزښت;
        ثابت منطقي cond_ټاکل
            = مستقیم_State_Cond_Const_ټاکل(stmtsp, stateVscp, then_ارزښت, else_ارزښت);
        UASSERT_OBJ(cond_ټاکل, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        إذا (!validate_معلوم_State_ارزښت(stmtsp, stateSpace, then_ارزښت, "target")) أرجع;
        إذا (!validate_معلوم_State_ارزښت(stmtsp, stateSpace, else_ارزښت, "target")) أرجع;
        لكل (ثابت Fsm_State_ارزښت& branch_ارزښت : {then_ارزښت, else_ارزښت}) {
            ګراف.جمع_کړه_Arc(from_ارزښت, branch_ارزښت, خطأ, ریښتیا, isDefault, stmtsp->fileline());
        }
    }

    ساكن فراغ emitIfChainArcs(Fsm_ګراف& ګراف, ثابت FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ثابت FsmStateSpace& stateSpace) {
        لكل (اندازه_t i = 0; i < chain.branches.اندازه(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            إذا (!validate_معلوم_State_ارزښت(branch.ifp, stateSpace, branch.from_ارزښت, "source"))
                أرجع;
            emitStmtArcsFrom(ګراف, branch.stmtsp, stateVscp, stateSpace, branch.from_ارزښت, خطأ,
                             branch.has_سر_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ساكن فراغ جمع_کړه_Reset_Arcs(Fsm_ګراف& ګراف, ثابت std::ویکټور<FsmResetArcDesc>& resetArcs,
                             ثابت FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            إذا (!validate_معلوم_State_ارزښت(resetArc.nodep(), stateSpace, resetArc.منزل_ارزښت(),
                                         "target"))
                استمر;
            ګراف.جمع_کړه_Arc(Fsm_State_ارزښت{}, resetArc.منزل_ارزښت(), ریښتیا, خطأ, خطأ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    فراغ processCase(AstCase* casep, AstVarScope* ټاکل_Vscp, ثابت Fsm_ثبتوونکی_Candidate& reg) {
        UASSERT_OBJ(ټاکل_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        إذا (!collectStateSpace(casep, stateVscp, ټاکل_Vscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& ثبت = m_state.fsmFor(stateVscp);
        إذا (!ثبت.graphp) {
            ثبت.graphp.reset(جديد Fsm_ګراف{});
            ثبت.graphp->scopep(reg.scopep());
            ثبت.graphp->stateAlwaysp(reg.alwaysp());
            ثبت.graphp->stateVarName(stateVscp->prettyName());
            ثبت.graphp->state_Var_دننی_Name(stateVscp->varp()->name());
            ثبت.graphp->stateVarScopep(stateVscp);
            ثبت.graphp->sampleVarScopep(reg.sampleVscp());
            ثبت.graphp->senses() = reg.senses();
            ثبت.graphp->resetCond() = reg.resetCond();
            ثبت.graphp->hasResetCond(reg.hasResetCond());
            ثبت.graphp->inclCond(reg.inclCond());
            ثبت.graphp->fileline(casep->fileline());
            لكل (ثابت std::pair<string, Fsm_State_ارزښت>& state : stateSpace.حالتونه) {
                ثبت.graphp->جمع_کړه_State_راس(state.لومړی, state.دوهم);
            }
            جمع_کړه_Reset_Arcs(*ثبت.graphp, reg.resetArcs(), stateSpace);
        }
        لكل (Ast_Case_توکی* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_توکی)) {
            emit_Case_توکی_Arcs(*ثبت.graphp, itemp, ټاکل_Vscp, stateSpace,
                             ثبت.graphp->inclCond());
        }
    }

    فراغ processIfChain(ثابت FsmIfChainCandidate& chain, AstVarScope* ټاکل_Vscp,
                        ثابت Fsm_ثبتوونکی_Candidate& reg) {
        UASSERT_OBJ(ټاکل_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        إذا (!collectStateSpace(chain, stateVscp, ټاکل_Vscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& ثبت = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!ثبت.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        ثبت.graphp.reset(جديد Fsm_ګراف{});
        ثبت.graphp->scopep(reg.scopep());
        ثبت.graphp->stateAlwaysp(reg.alwaysp());
        ثبت.graphp->stateVarName(stateVscp->prettyName());
        ثبت.graphp->state_Var_دننی_Name(stateVscp->varp()->name());
        ثبت.graphp->stateVarScopep(stateVscp);
        ثبت.graphp->sampleVarScopep(reg.sampleVscp());
        ثبت.graphp->senses() = reg.senses();
        ثبت.graphp->resetCond() = reg.resetCond();
        ثبت.graphp->hasResetCond(reg.hasResetCond());
        ثبت.graphp->inclCond(reg.inclCond());
        ثبت.graphp->fileline(chain.ifp->fileline());
        لكل (ثابت std::pair<string, Fsm_State_ارزښت>& state : stateSpace.حالتونه) {
            ثبت.graphp->جمع_کړه_State_راس(state.لومړی, state.دوهم);
        }
        جمع_کړه_Reset_Arcs(*ثبت.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*ثبت.graphp, chain, ټاکل_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    فراغ process_One_بلاک_Always(ثابت FsmComboAlways& one_بلاک) {
        ثابت ثبتوونکی_Always_Analyzer analyzer{one_بلاک.scopep()};
        AstAlways* ثابت alwaysp = one_بلاک.alwaysp();
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->has_څنډه()) أرجع;
        ثابت std::ویکټور<std::pair<AstCase*, Ast_غوټه_بیان*>> candidates
            = analyzer.one_بلاک_Candidates(alwaysp);

        FsmCaseCandidate لومړی_Cand;
        لكل (ثابت std::pair<AstCase*, Ast_غوټه_بیان*>& cand : candidates) {
            AstVarRef* ثابت selp = VN_CAST(cand.لومړی->exprp(), VarRef);
            AstVarScope* ثابت vscp = selp ? selp->varScopep() : تش_اشاره;
            إذا (!vscp) استمر;
            إذا (!لومړی_Cand.stateVscp) {
                لومړی_Cand.warnNodep = cand.لومړی;
                لومړی_Cand.stateVscp = vscp;
                Fsm_ثبتوونکی_Candidate reg;
                analyzer.build_One_بلاک_Candidate(alwaysp, vscp, cand.دوهم, reg);
                processCase(cand.لومړی, vscp, reg);
            } وإلا إذا (vscp != لومړی_Cand.stateVscp) {
                cand.لومړی->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.لومړی, لومړی_Cand));
            } وإلا {
                cand.لومړی->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.لومړی, لومړی_Cand));
            }
        }

        إذا (لومړی_Cand.stateVscp) أرجع;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ثابت std::ویکټور<std::pair<AstIf*, Ast_غوټه_بیان*>> ifCandidates
            = analyzer.one_بلاک_If_Candidates(alwaysp);
        لكل (ثابت std::pair<AstIf*, Ast_غوټه_بیان*>& cand : ifCandidates) {
            ثابت FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.لومړی);
            FsmIfChainCandidate chain;
            إذا (!collectIfChain(cand.لومړی, aliases, chain)) استمر;
            AstVarScope* ثابت vscp = chain.compareVscp;
            إذا (!if_Chain_Supported_Transition_غوټه(chain, vscp)) استمر;
            إذا (!لومړی_Cand.stateVscp) {
                لومړی_Cand.warnNodep = cand.لومړی;
                لومړی_Cand.stateVscp = vscp;
                Fsm_ثبتوونکی_Candidate reg;
                analyzer.build_One_بلاک_Candidate(alwaysp, vscp, cand.دوهم, reg);
                processIfChain(chain, vscp, reg);
            } وإلا إذا (vscp != لومړی_Cand.stateVscp) {
                cand.لومړی->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.لومړی, لومړی_Cand));
            } وإلا {
                cand.لومړی->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.لومړی, لومړی_Cand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    ساكن فراغ warnComboSameAlways(Ast_غوټه* warnNodep, ثابت FsmCaseCandidate& لومړی_Cand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, لومړی_Cand));
    }

    فراغ processComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_ثبتوونکی_Candidates};
        Ast_غوټه* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate لومړی_Cand;
        لكل (Ast_غوټه* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            ثابت ComboAlwaysAnalyzer::Combo_سمون سمون = analyzer.سمون_Case(stmtsp, casep);
            ثابت Fsm_ثبتوونکی_Candidate* ثابت matchedp = سمون.matchedp;
            Ast_غوټه* ثابت matchedWarnNodep = سمون.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!لومړی_Cand.stateVscp) {
                ثابت خپلکاری ورننباسه_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!ورننباسه_Pair.دوهم) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ورننباسه_Pair.لومړی->دوهم));
                    استمر;
                }
                لومړی_Cand.warnNodep = matchedWarnNodep;
                لومړی_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->بل_Vscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != لومړی_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, لومړی_Cand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, لومړی_Cand));
        }
        إذا (لومړی_Cand.stateVscp) أرجع;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        لكل (Ast_غوټه* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ثابت ifp = VN_CAST(nodep, If);
            إذا (!ifp) استمر;
            FsmIfChainCandidate chain;
            ثابت FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            إذا (!collectIfChain(ifp, aliases, chain)) استمر;
            ثابت ComboAlwaysAnalyzer::Combo_سمون سمون = analyzer.سمون_If_Chain(stmtsp, chain);
            ثابت Fsm_ثبتوونکی_Candidate* ثابت matchedp = سمون.matchedp;
            Ast_غوټه* ثابت matchedWarnNodep = سمون.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!لومړی_Cand.stateVscp) {
                ثابت std::pair<std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate>::iterator,
                                منطقي>
                    ورننباسه_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!ورننباسه_Pair.دوهم) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  ورننباسه_Pair.لومړی->دوهم));
                    استمر;
                }
                لومړی_Cand.warnNodep = matchedWarnNodep;
                لومړی_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->بل_Vscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != لومړی_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, لومړی_Cand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, لومړی_Cand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    فراغ لیدنه(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    فراغ لیدنه(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_ټاکل) {
            iterateChildren(nodep);
            أرجع;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_بلاک_Alwayss.emplace_شا(m_scopep, nodep);
        ثابت ثبتوونکی_Always_Analyzer analyzer{m_scopep};
        Fsm_ثبتوونکی_Candidate reg;
        إذا (analyzer.سمون_ثبتوونکی_Candidate(nodep, reg)) {
            AstVarScope* ثابت stateVscp = reg.stateVscp();
            ثابت منطقي وموندل
                = std::any_د(m_ثبتوونکی_Candidates.cbegin(), m_ثبتوونکی_Candidates.cend(),
                              [stateVscp](ثابت Fsm_ثبتوونکی_Candidate& existing) {
                                  أرجع existing.stateVscp() == stateVscp;
                              });
            إذا (!وموندل) { m_ثبتوونکی_Candidates.emplace_شا(reg); }
        }
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_شا(m_scopep, nodep);
        } وإلا إذا (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            إذا (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_شا(m_scopep, nodep);
            } وإلا {
                m_nonComboAlwayss.emplace_شا(m_scopep, nodep);
            }
        }
    }

    فراغ لیدنه(Ast_ټاکل_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_ټاکل(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_پورټ_Alias(nodep);
        iterateChildren(nodep);
    }

    فراغ لیدنه(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        إذا (m_scopep && fsm_ثبتوونکی_Wrapper_Desc(nodep)) جمع_کړه_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        لكل (ثابت std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_ثبتوونکی_Candidate reg;
            إذا (سمون_Fsm_Wrapper_Cell(wrapperCell.لومړی, wrapperCell.دوهم, reg)) {
                m_ثبتوونکی_Candidates.emplace_شا(reg);
            }
        }
        لكل (ثابت FsmComboAlways& one_بلاک : m_one_بلاک_Alwayss) process_One_بلاک_Always(one_بلاک);
        لكل (ثابت FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        لكل (ثابت FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
صنف FsmCoverIncBuilder final {
    ثابت Fsm_ګراف& m_ګراف;  // Fsm graph
    std::ویکټور<ثابت Fsm_State_راس*> m_حالتونه;  // States vertexes inside fsm graph
    std::unordered_سېټ<uint64_t>
        m_undefined_حالتونه;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ثابت Fsm_راس* m_defaultVertexp = تش_اشاره;  // Default vertex
    ثابت Fsm_راس* m_resetVertexp = تش_اشاره;  // Reset vertex
    اندازه_t m_څنډې_شمېر = 0;  // Total count of edges
    AstVarScope* ثابت m_prevVscp;  // Previous state of fsm
    File_کرښه* ثابت m_flp;  // Fileline pointer to use across the builder
    ثابت std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_غوټه_Stmt* m_coverIncp = تش_اشاره;  // Increments proper CoverDecl after each change
    Ast_غوټه_Stmt* m_initialCoverIncp = تش_اشاره;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = تش_اشاره;  // List of CoverDecls
    منطقي m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ثابت uint64_t ارزښت) {
        V3_شمېره عدد{static_cast<Ast_غوټه*>(تش_اشاره), m_prevVscp->پلنوالی()};
        عدد.سېټ_Quad(ارزښت);
        عدد.isSigned(m_prevVscp->dtypep()->isSigned());
        أرجع جديد AstConst{m_flp, عدد};
    }

    std::string quad_منزل_بټ_Stirng_Label(ثابت uint64_t val) {
        صحيح پلنوالی = m_prevVscp->پلنوالی();
        UASSERT(پلنوالی < 64, "We should not expand values that wide");
        std::string پایله = std::منزل_string(پلنوالی) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        طالما (--پلنوالی >= 0) پایله.ټېل_شا("01"[(val >> پلنوالی) & 1]);
        أرجع پایله;
    }

    فراغ init_Existing_حالتونه() {
        لكل (ثابت V3_ګراف_راس& vtx : m_ګراف.vertices()) {
            ثابت Fsm_راس* ثابت vertexp = vtx.as<Fsm_راس>();
            m_څنډې_شمېر += vertexp->دننه_څنډې().اندازه();
            إذا (ثابت Fsm_State_راس* ثابت statep = vertexp->cast<Fsm_State_راس>()) {
                m_حالتونه.ټېل_شا(statep);
            } وإلا إذا (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_ګراف.sampleVarScopep(),
                            "Two defaults?");
            } وإلا /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_ګراف.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ثابت صحيح پلنوالی = m_ګراف.sampleVarScopep()->پلنوالی();
        m_isComplete = پلنوالی < 64 && m_حالتونه.اندازه() == (static_cast<اندازه_t>(1) << پلنوالی);
    }

    فراغ init_Undefined_حالتونه() {
        UASSERT_OBJ(m_ګراف.sampleVarScopep()->پلنوالی() < 64, m_ګراف.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ثابت uint64_t combinations_شمېر = static_cast<اندازه_t>(1)
                                           << m_ګراف.sampleVarScopep()->پلنوالی();
        // Rolling trick allows to cut peak memory usage
        منطقي undefRolled = خطأ;
        خپلکاری rollUndefs = [هذا, combinations_شمېر] {
            لكل (uint64_t i = 0; i < combinations_شمېر; ++i) {
                ثابت خپلکاری ورننباسه = m_undefined_حالتونه.ورننباسه(i);
                إذا (!ورننباسه.دوهم) m_undefined_حالتونه.erase(ورننباسه.لومړی);
            }
        };
        لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتونه) {
            ثابت V3_شمېره& عدد = statep->ارزښت().عدد();
            ثابت uint64_t ارزښت = عدد.منزل_UQuad();
            إذا (!m_isComplete) {
                إذا (undefRolled) {
                    UASSERT_OBJ(m_undefined_حالتونه.erase(ارزښت) == 1, m_ګراف.sampleVarScopep(),
                                "Two states has the same value?");
                } وإلا {
                    UASSERT_OBJ(m_undefined_حالتونه.ورننباسه(ارزښت).دوهم, m_ګراف.sampleVarScopep(),
                                "Two states has the same value?");
                    إذا (m_undefined_حالتونه.اندازه() * 2 >= combinations_شمېر) {
                        undefRolled = ریښتیا;
                        rollUndefs();
                    }
                }
            }
        }
        إذا (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_غوټه_بیان* buildResetCond() ثابت {
        Ast_غوټه_بیان* ثابت refp
            = جديد AstVarRef{m_flp, m_ګراف.resetCond().varScopep, VAccess::READ};
        أرجع m_ګراف.resetCond().فعال_ټیټ
                   ? static_cast<Ast_غوټه_بیان*>(جديد AstLogNot{m_flp, refp})
                   : refp;
    }

    اندازه_t get_Arc_شمېر_With_Expansion(ثابت VFsmExpandType expand) ثابت {
        اندازه_t arc_شمېر_With_Expansion = ~0ULL;
        بدل (expand) {
        حالة VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_ګراف.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            اكسر;
        }
        حالة VFsmExpandType::AUTO: {
            arc_شمېر_With_Expansion = m_څنډې_شمېر;
            إذا (m_defaultVertexp) {
                UASSERT_OBJ(m_حالتونه.اندازه() >= m_defaultVertexp->بهر_څنډې().اندازه(),
                            m_ګراف.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_شمېر_With_Expansion -= m_defaultVertexp->بهر_څنډې().اندازه();
                لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتونه) {
                    arc_شمېر_With_Expansion += static_cast<اندازه_t>(statep->بهر_څنډې().تشه())
                                             * m_defaultVertexp->بهر_څنډې().اندازه();
                }
            }
            إذا (m_resetVertexp) {
                UASSERT_OBJ(m_حالتونه.اندازه() >= m_resetVertexp->بهر_څنډې().اندازه(),
                            m_ګراف.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_شمېر_With_Expansion += m_حالتونه.اندازه() - m_resetVertexp->بهر_څنډې().اندازه();
            }
            اكسر;
        }
        حالة VFsmExpandType::AUTO_EXPAND: {
            arc_شمېر_With_Expansion = m_حالتونه.اندازه() * m_حالتونه.اندازه() - m_حالتونه.اندازه();
            اكسر;
        }
        حالة VFsmExpandType::ډک: {
            ثابت صحيح پلنوالی = m_ګراف.sampleVarScopep()->پلنوالی();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            إذا (پلنوالی < 32) {
                ثابت uint64_t combinations_شمېر = static_cast<اندازه_t>(1) << پلنوالی;
                arc_شمېر_With_Expansion = combinations_شمېر * combinations_شمېر - combinations_شمېر;
            }
            اكسر;
        }
        }
        أرجع arc_شمېر_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ثابت std::string& label,
                                           ثابت std::string& fsm_نښان = "") {
        AstCoverOtherDecl* ثابت declp
            = جديد AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_ګراف.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_ګراف.stateVarName(),
                                    "",
                                    label,
                                    fsm_نښان};
        // We run after scoping, so this is declared for one instance only
        declp->فی_Instance(ریښتیا);
        m_declsp = Ast_غوټه::جمع_کړه_بل(m_declsp, declp);
        أرجع declp;
    }

    Ast_غوټه_Stmt* buildCoverIncStmtp(ثابت std::string& fromStateLabel,
                                    ثابت std::string& منزل_State_Label,
                                    ثابت std::string& بنسټ_Label = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ثابت declp = جديد AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_ګراف.stateVarName() + "::" + fromStateLabel + "->" + منزل_State_Label,
            "",
            0,
            m_ګراف.stateVarName(),
            fromStateLabel,
            منزل_State_Label,
            بنسټ_Label};
        // We run after scoping, so this is declared for one instance only
        declp->فی_Instance(ریښتیا);
        m_declsp = Ast_غوټه::جمع_کړه_بل(m_declsp, declp);
        أرجع جديد AstCoverInc{m_flp, declp};
    }
    Ast_غوټه_Stmt* buildExpandedTransitionHandler(ثابت std::string& منزل_State_Label,
                                                ثابت منطقي considerUndefs,
                                                ثابت منطقي expandOnlyWithTerminalVerticies,
                                                ثابت Fsm_State_راس* ثابت منزل_Statep = تش_اشاره) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, تش_اشاره};
        std::unordered_سېټ<ثابت Fsm_State_راس*> handledSoureces;
        إذا (منزل_Statep) {
            لكل (ثابت V3_ګراف_څنډه& څنډه : منزل_Statep->دننه_څنډې()) {
                إذا (ثابت Fsm_State_راس* ثابت fromp = څنډه.fromp()->cast<Fsm_State_راس>()) {
                    إذا (!handledSoureces.emplace(fromp).دوهم) استمر;
                    arcCasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{
                        m_flp, makeConst(fromp->ارزښت().عدد().منزل_UQuad()),
                        (fromp != منزل_Statep) ? buildCoverIncStmtp(fromp->label(), منزل_State_Label)
                                            : تش_اشاره});
                }
            }
        }
        لكل (ثابت Fsm_State_راس* ثابت fromp : m_حالتونه) {
            إذا (handledSoureces.پیدا_کړه(fromp) != handledSoureces.پای_ټکی()) استمر;
            arcCasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{
                m_flp, makeConst(fromp->ارزښت().عدد().منزل_UQuad()),
                (fromp != منزل_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->بهر_څنډې().تشه()))
                    ? buildCoverIncStmtp(fromp->label(), منزل_State_Label, "artificial")
                    : تش_اشاره});
        }
        إذا (!m_isComplete) {
            إذا (considerUndefs) {
                لكل (ثابت uint64_t from : m_undefined_حالتونه) {
                    ثابت std::string& label = quad_منزل_بټ_Stirng_Label(from);
                    arcCasep->جمع_کړه_Itemsp(
                        جديد Ast_Case_توکی{m_flp, makeConst(from),
                                        (label != منزل_State_Label)
                                            ? buildCoverIncStmtp(label, منزل_State_Label, "artificial")
                                            : تش_اشاره});
                }
            } وإلا {
                arcCasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{m_flp, تش_اشاره, تش_اشاره});
            }
        }
        أرجع arcCasep;
    }
    Ast_غوټه_Stmt* buildUnxpandedTransitionHandler(ثابت Fsm_State_راس* ثابت منزل_Statep) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, تش_اشاره};
        لكل (ثابت V3_ګراف_څنډه& څنډه : منزل_Statep->دننه_څنډې()) {
            ثابت Fsm_Arc_څنډه* ثابت arcp = څنډه.as<Fsm_Arc_څنډه>();
            UASSERT_OBJ(!arcp->isReset(), m_ګراف.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_ګراف.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ثابت Fsm_State_راس* ثابت fromp = arcp->fromp()->as<Fsm_State_راس>();
            إذا (fromp != منزل_Statep) {
                arcCasep->جمع_کړه_Itemsp(
                    جديد Ast_Case_توکی{m_flp, makeConst(fromp->ارزښت().عدد().منزل_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), منزل_Statep->label())});
            }
        }
        arcCasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{m_flp, تش_اشاره, تش_اشاره});
        أرجع arcCasep;
    }

عام:
    FsmCoverIncBuilder(ثابت Fsm_ګراف& ګراف, AstVarScope* ثابت prevVscp)
        : m_ګراف{ګراف}
        , m_prevVscp{prevVscp}
        , m_flp{ګراف.fileline()}
        , m_modPrettyName{ګراف.scopep()->modp()->prettyName()} {
        init_Existing_حالتونه();
        AstVarScope* ثابت sampleVscp = ګراف.sampleVarScopep();
        ثابت VFsmExpandType expand
            = ګراف.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? ګراف.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ثابت اندازه_t arc_شمېر_With_Expansion = get_Arc_شمېر_With_Expansion(expand);
            إذا (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_شمېر_With_Expansion == 0ULL
                            || arc_شمېر_With_Expansion
                                   > static_cast<اندازه_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_شمېر_With_Expansion);
                أرجع;
            }
        }

        AstCase* ثابت arcCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, ګراف.sampleVarScopep(), VAccess::READ}, تش_اشاره};
        AstCase* ثابت initialStateCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, ګراف.sampleVarScopep(), VAccess::READ}, تش_اشاره};

        ثابت منطقي expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_سېټ<ثابت Fsm_State_راس*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_سېټ<ثابت Fsm_State_راس*>
            resetTargets;  // States that have edge incoming from reset vertex
        إذا (!expandAll) {
            إذا (m_defaultVertexp) {
                لكل (ثابت V3_ګراف_څنډه& څنډه : m_defaultVertexp->بهر_څنډې()) {
                    defaultTargets.emplace(څنډه.سر()->as<Fsm_State_راس>());
                }
            }
            إذا (m_resetVertexp) {
                لكل (ثابت V3_ګراف_څنډه& څنډه : m_resetVertexp->بهر_څنډې()) {
                    resetTargets.emplace(څنډه.سر()->as<Fsm_State_راس>());
                }
            }
        }
        ثابت منطقي is_ډک_Expansion = expand == VFsmExpandType::ډک;
        إذا (is_ډک_Expansion) init_Undefined_حالتونه();
        لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتونه) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(statep->label());
            initialStateCasep->جمع_کړه_Itemsp(
                جديد Ast_Case_توکی{m_flp, makeConst(statep->ارزښت().عدد().منزل_UQuad()),
                                جديد AstCoverInc{m_flp, declp, ریښتیا}});
            ثابت منطقي expandAllOrReset
                = expandAll || (resetTargets.پیدا_کړه(statep) != resetTargets.پای_ټکی());
            ثابت منطقي defaultExpantion
                = !expandAllOrReset && (defaultTargets.پیدا_کړه(statep) != defaultTargets.پای_ټکی());
            arcCasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{
                m_flp, makeConst(statep->ارزښت().عدد().منزل_UQuad()),
                Ast_غوټه::جمع_کړه_بل<Ast_غوټه_Stmt>(
                    جديد AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_ډک_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        إذا (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            إذا (is_ډک_Expansion) {
                لكل (ثابت uint64_t ارزښت : m_undefined_حالتونه) {
                    ثابت std::string& label = quad_منزل_بټ_Stirng_Label(ارزښت);
                    AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(label);
                    initialStateCasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{
                        m_flp, makeConst(ارزښت), جديد AstCoverInc{m_flp, declp, ریښتیا}});
                    arcCasep->جمع_کړه_Itemsp(
                        جديد Ast_Case_توکی{m_flp, makeConst(ارزښت),
                                        Ast_غوټه::جمع_کړه_بل<Ast_غوټه_Stmt>(
                                            جديد AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, ریښتیا, خطأ))});
                }
            } وإلا {
                // No expansion
                arcCasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{m_flp, تش_اشاره, تش_اشاره});
                initialStateCasep->جمع_کړه_Itemsp(جديد Ast_Case_توکی{m_flp, تش_اشاره, تش_اشاره});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = افتراضي;
    Ast_غوټه_Cover_Decl* getCoverDeclsp() ثابت { أرجع m_declsp; }
    Ast_غوټه_Stmt* getCoverIncp() ثابت { أرجع m_coverIncp; }
    Ast_غوټه_Stmt* getInitialCoverIncp() ثابت { أرجع m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
صنف Fsm_ښکتنی_Visitor final {
    // STATE - across all visitors
    ثابت FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ساكن Ast_Sen_ونه* build_Sen_ونه(File_کرښه* flp, ثابت std::ویکټور<FsmSenDesc>& senses) {
        Ast_Sen_ونه* ثابت sentreep = جديد Ast_Sen_ونه{flp, تش_اشاره};
        لكل (ثابت FsmSenDesc& sense : senses) {
            Ast_Sen_توکی* ثابت senItemp
                = جديد Ast_Sen_توکی{flp, VEdgeType{sense.څنډه_Type},
                                 جديد AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->جمع_کړه_Sensesp(senItemp);
        }
        أرجع sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    فراغ buildOne(ثابت Fsm_ګراف& ګراف) {
        UINFO(1, "buildOne lowering FSM " << ګراف.stateVarName()
                                          << " vertices=" << ګراف.vertices().اندازه() << کرښه_پای);
        AstAlways* ثابت alwaysp = ګراف.stateAlwaysp();
        AstScope* ثابت scopep = ګراف.scopep();
        AstVarScope* ثابت stateVscp = ګراف.stateVarScopep();
        AstVarScope* ثابت sampleVscp = ګراف.sampleVarScopep();
        File_کرښه* ثابت flp = ګراف.fileline();
        Ast_غوټه_Module* ثابت modp = scopep->modp();
        AstVarScope* ثابت prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(ریښتیا);
        sampleVscp->optimizeLifePost(ریښتیا);
        prevVscp->optimizeLifePost(ریښتیا);

        Ast_فعال* ثابت initActivep
            = جديد Ast_فعال{flp, "fsm-coverage-init",
                            جديد Ast_Sen_ونه{flp, جديد Ast_Sen_توکی{flp, Ast_Sen_توکی::Initial{}}}};
        initActivep->sen_ونه_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->جمع_کړه_Stmtsp(جديد AstInitialStatic{
            flp, جديد Ast_ټاکل{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                               جديد AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->جمع_کړه_Blocksp(initActivep);

        AstAlwaysPost* ثابت covPostp = جديد AstAlwaysPost{flp};
        منطقي updatePrevAfterPost = خطأ;
        إذا (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        } وإلا {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = ریښتیا;
            prevVscp->varp()->سېټ_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ثابت FsmCoverIncBuilder coverIncBuilder{ګراف, prevVscp};
            modp->جمع_کړه_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->جمع_کړه_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->جمع_کړه_Stmtsp(
                جديد AstIf{flp,
                          جديد AstNeq{flp, جديد AstVarRef{flp, prevVscp, VAccess::READ},
                                     جديد AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        إذا (updatePrevAfterPost) {
            covPostp->جمع_کړه_Stmtsp(جديد Ast_ټاکل{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_ونه* ثابت sentreep = build_Sen_ونه(flp, ګراف.senses());
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "fsm-coverage", sentreep};
        activep->sen_ونه_Storep(sentreep);
        scopep->جمع_کړه_Blocksp(activep);
        activep->جمع_کړه_Stmtsp(covPostp);
    }

عام:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    څرګند Fsm_ښکتنی_Visitor(ثابت FsmState& state)
        : m_state{state} {
        لكل (ثابت DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

فراغ V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    إذا (dump_ګراف_Level() >= 6) {
        اندازه_t شاخص = 0;
        لكل (ثابت DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_نښان(شاخص++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_ښکتنی_Visitor ښکتنی{state}; }
    V3Global::dump_Check_Global_ونه("fsm-detect", 0, dump_ونه_Either_Level() >= 3);
}
