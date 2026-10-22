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
صنف Fsm_State_قدر final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_کلید;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_نمبر m_عدد;  // Original folded value, preserving width for lowered comparisons

    ساكن string make_کلید(ثابت V3_نمبر& عدد) {
        V3_نمبر کلید_عدد = عدد;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        کلید_عدد.isSigned(خطأ);
        أرجع cvt_منزل_Str(کلید_عدد.چوڑائی()) + ":" + کلید_عدد.ascii(سچ, سچ);
    }

عام:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_قدر()
        : m_کلید{"1:1'h0"}
        , m_عدد{static_cast<Ast_گرہ*>(خالی_سوچک), 1, 0} {}
    واضح Fsm_State_قدر(ثابت V3_نمبر& عدد)
        : m_کلید{make_کلید(عدد)}
        , m_عدد{عدد} {}

    ثابت string& کلید() ثابت { أرجع m_کلید; }
    ثابت V3_نمبر& عدد() ثابت { أرجع m_عدد; }
    string ascii() ثابت { أرجع m_عدد.ascii(سچ, سچ); }
    string warn_متن() ثابت {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        إذا (m_عدد.چوڑائی() <= 32) أرجع cvt_منزل_Str(m_عدد.منزل_UInt());
        أرجع ascii();
    }

    منطقي عامل==(ثابت Fsm_State_قدر& rhs) ثابت { أرجع m_کلید == rhs.m_کلید; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
بنية Fsm_State_قدر_Hash final {
    ناپ_t عامل()(ثابت Fsm_State_قدر& قدر) ثابت {
        أرجع std::hash<string>{}(قدر.کلید());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
بنية FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en کنارا_Type = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = خالی_سوچک;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
بنية FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = خالی_سوچک;
    منطقي سرگرم_کم = خطأ;
};

صنف FsmResetArcDesc final {
    Fsm_State_قدر m_منزل_قدر;  // Encoded reset target state.
    Ast_گرہ* m_nodep = خالی_سوچک;  // Source node for warnings and emitted metadata.
    Ast_گرہ_اظہاریہ* m_valuep = خالی_سوچک;  // Expression that provided the reset value.

عام:
    FsmResetArcDesc() = افتراضي;
    FsmResetArcDesc(Fsm_State_قدر منزل_قدر, Ast_گرہ_سونپو* nodep)
        : m_منزل_قدر{منزل_قدر}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_قدر منزل_قدر, Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* valuep)
        : m_منزل_قدر{منزل_قدر}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_قدر منزل_قدر() ثابت { أرجع m_منزل_قدر; }
    Ast_گرہ* nodep() ثابت { أرجع m_nodep; }
    Ast_گرہ_اظہاریہ* valuep() ثابت { أرجع m_valuep; }
};

بنية FsmWrapperRoles final {
    string d_پورٹ;
    string q_پورٹ;
    string clk_پورٹ;
    string rst_پورٹ;
    string rstValParam;
    منطقي has_Rst_سرگرم_کم = خطأ;
    منطقي rst_سرگرم_کم = خطأ;
};

ساكن منطقي fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ثابت string& پورٹ_Name,
                                                  منطقي& سرگرم_کم) {
    منطقي matched = خطأ;
    cellp->modp()->foreach([&](Ast_Sen_شے* itemp) {
        Ast_گرہ_Var_Ref* ثابت vrefp = itemp->varrefp();
        إذا (!vrefp) أرجع;
        إذا (vrefp->varp()->name() != پورٹ_Name) أرجع;
        سرگرم_کم = itemp->کنارا_Type() == VEdgeType::ET_NEGEDGE;
        matched = سچ;
    });
    أرجع matched;
}

ساكن ثابت V3Control::Fsm_رجسٹر_Wrapper* fsm_رجسٹر_Wrapper_Desc(AstCell* cellp) {
    Ast_گرہ_Module* ثابت modp = cellp->modp();
    ثابت string origName = modp->origName();
    إذا (ثابت V3Control::Fsm_رجسٹر_Wrapper* ثابت descp
        = V3Control::get_Fsm_رجسٹر_Wrapper(origName)) {
        أرجع descp;
    }
    أرجع V3Control::get_Fsm_رجسٹر_Wrapper(modp->prettyDehashOrigOrName());
}

ساكن FsmWrapperRoles rolesFromDesc(ثابت V3Control::Fsm_رجسٹر_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_پورٹ = desc.d;
    roles.q_پورٹ = desc.q;
    roles.clk_پورٹ = desc.clock;
    roles.rst_پورٹ = desc.reset;
    roles.rstValParam = desc.reset_قدر;
    أرجع roles;
}

صنف Fsm_رجسٹر_Candidate final {
    AstScope* m_scopep = خالی_سوچک;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = خالی_سوچک;  // Register process that commits the state.
    AstVarScope* m_stateVscp = خالی_سوچک;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = خالی_سوچک;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_اگلا_Vscp = خالی_سوچک;  // Next-state variable or same state var for 1-block FSMs.
    std::سمتیہ<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::سمتیہ<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    منطقي m_hasResetCond = خطأ;  // Whether the FSM had a modeled reset predicate.
    منطقي m_inclCond = خطأ;  // Whether conditional/default arcs are kept explicitly.
    File_سطر* m_flp = خالی_سوچک;  // Representative source location.

عام:
    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
    فراغ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ثابت { أرجع m_stateVscp; }
    فراغ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ثابت { أرجع m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    فراغ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* اگلا_Vscp() ثابت { أرجع m_اگلا_Vscp; }
    فراغ اگلا_Vscp(AstVarScope* vscp) { m_اگلا_Vscp = vscp; }
    ثابت std::سمتیہ<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::سمتیہ<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs() ثابت { أرجع m_resetArcs; }
    std::سمتیہ<FsmResetArcDesc>& resetArcs() { أرجع m_resetArcs; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    File_سطر* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_سطر* flp) { m_flp = flp; }
};

صنف FsmComboAlways final {
    AstScope* ثابت m_scopep = خالی_سوچک;  // Owning scope for the combinational process.
    AstAlways* ثابت m_alwaysp = خالی_سوچک;  // Candidate transition process.

عام:
    FsmComboAlways() = افتراضي;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ثابت { أرجع m_scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
};

صنف Fsm_گراف;

صنف Fsm_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(Fsm_راس, V3_گراف_راس)

عام:
    تعداد صنف قسم_2 : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

خاص:
    قسم_2 m_قسم;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_قدر m_قدر;  // Encoded state value for real state vertices.

محمي:
    Fsm_راس(V3_گراف* graphp, قسم_2 قسم, string label, Fsm_State_قدر قدر) VL_MT_DISABLED
        : V3_گراف_راس{graphp},
          m_قسم{قسم},
          m_label{label},
          m_قدر{قدر} {}
    ~Fsm_راس() override = افتراضي;

عام:
    قسم_2 قسم() ثابت { أرجع m_قسم; }
    منطقي isState() ثابت { أرجع m_قسم == قسم_2::STATE; }
    منطقي isResetAny() ثابت { أرجع m_قسم == قسم_2::RESET_ANY; }
    منطقي isDefaultAny() ثابت { أرجع m_قسم == قسم_2::DEFAULT_ANY; }
    ثابت string& label() ثابت { أرجع m_label; }
    ثابت Fsm_State_قدر& قدر() ثابت { أرجع m_قدر; }

    string name() ثابت override VL_MT_محفوظ { أرجع m_label + "=" + m_قدر.ascii(); }
};

صنف Fsm_State_راس final : عام Fsm_راس {
    VL_RTTI_IMPL(Fsm_State_راس, Fsm_راس)

عام:
    Fsm_State_راس(V3_گراف* graphp, string label, Fsm_State_قدر قدر) VL_MT_DISABLED
        : Fsm_راس{graphp, قسم_2::STATE, label, قدر} {}
    ~Fsm_State_راس() override = افتراضي;

    string dotColor() ثابت override { أرجع "lightblue"; }
    string dotShape() ثابت override { أرجع "ellipse"; }
};

صنف Fsm_Pseudo_راس final : عام Fsm_راس {
    VL_RTTI_IMPL(Fsm_Pseudo_راس, Fsm_راس)

عام:
    Fsm_Pseudo_راس(V3_گراف* graphp, قسم_2 قسم, string label) VL_MT_DISABLED
        : Fsm_راس{graphp, قسم, label, Fsm_State_قدر{}} {}
    ~Fsm_Pseudo_راس() override = افتراضي;

    string name() ثابت override VL_MT_محفوظ { أرجع label(); }
    string dotColor() ثابت override { أرجع isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ثابت override { أرجع "diamond"; }
};

صنف Fsm_Arc_کنارا final : عام V3_گراف_کنارا {
    VL_RTTI_IMPL(Fsm_Arc_کنارا, V3_گراف_کنارا)
    منطقي m_isReset = خطأ;  // Arc originates from the synthetic reset source.
    منطقي m_isCond = خطأ;  // Arc came from a conditional next-state split.
    منطقي m_isDefault = خطأ;  // Arc represents a case default source.
    File_سطر* m_flp = خالی_سوچک;  // Source location for emitted coverage metadata.

عام:
    Fsm_Arc_کنارا(V3_گراف* graphp, Fsm_راس* fromp, Fsm_State_راس* چوٹی, منطقي isReset, منطقي isCond,
               منطقي isDefault, File_سطر* flp) VL_MT_DISABLED : V3_گراف_کنارا{graphp, fromp, چوٹی, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~Fsm_Arc_کنارا() override = افتراضي;

    منطقي isReset() ثابت { أرجع m_isReset; }
    منطقي isCond() ثابت { أرجع m_isCond; }
    منطقي isDefault() ثابت { أرجع m_isDefault; }
    File_سطر* fileline() ثابت { أرجع m_flp; }

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
صنف Fsm_گراف final : عام V3_گراف {
    AstScope* m_scopep = خالی_سوچک;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = خالی_سوچک;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_اندرونی_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = خالی_سوچک;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = خالی_سوچک;  // Scoped variable sampled by coverage logic.
    std::سمتیہ<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    منطقي m_hasResetCond = خطأ;  // Whether the detected FSM had a reset branch.
    منطقي m_inclCond = خطأ;  // Whether conditional arcs should be kept explicitly.
    File_سطر* m_flp = خالی_سوچک;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_قدر, Fsm_State_راس*, Fsm_State_قدر_Hash>
        m_stateVertices;  // Value to state map.
    Fsm_Pseudo_راس* m_resetVertexp = خالی_سوچک;  // Synthetic ANY source for reset arcs.
    Fsm_Pseudo_راس* m_defaultVertexp = خالی_سوچک;  // Synthetic default source for case defaults.

عام:
    Fsm_گراف() VL_MT_DISABLED
        : m_resetVertexp{جديد Fsm_Pseudo_راس{هذا, Fsm_راس::قسم_2::RESET_ANY, "ANY"}},
          m_defaultVertexp{جديد Fsm_Pseudo_راس{هذا, Fsm_راس::قسم_2::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ثابت { أرجع m_stateAlwaysp; }
    فراغ stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ثابت string& stateVarName() ثابت { أرجع m_stateVarName; }
    فراغ stateVarName(ثابت string& name) { m_stateVarName = name; }
    ثابت string& state_Var_اندرونی_Name() ثابت { أرجع m_state_Var_اندرونی_Name; }
    فراغ state_Var_اندرونی_Name(ثابت string& name) { m_state_Var_اندرونی_Name = name; }
    AstVarScope* stateVarScopep() ثابت { أرجع m_stateVarScopep; }
    فراغ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ثابت {
        أرجع m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    فراغ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ثابت std::سمتیہ<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::سمتیہ<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    File_سطر* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_سطر* flp) { m_flp = flp; }

    Fsm_State_راس* جوڑو_State_راس(string label, Fsm_State_قدر قدر) VL_MT_DISABLED {
        Fsm_State_راس* ثابت vertexp = جديد Fsm_State_راس{هذا, label, قدر};
        m_stateVertices.emplace(قدر, vertexp);
        أرجع vertexp;
    }
    Fsm_Pseudo_راس* reset_Any_راس() VL_MT_DISABLED { أرجع m_resetVertexp; }
    Fsm_Pseudo_راس* default_Any_راس() VL_MT_DISABLED { أرجع m_defaultVertexp; }
    Fsm_Arc_کنارا* جوڑو_Arc(Fsm_State_قدر from_قدر, Fsm_State_قدر منزل_قدر, منطقي isReset, منطقي isCond,
                       منطقي isDefault, File_سطر* flp) VL_MT_DISABLED {
        Fsm_State_راس* ثابت چوٹی = m_stateVertices.at(منزل_قدر);
        Fsm_راس* fromp = خالی_سوچک;
        إذا (isReset) {
            fromp = reset_Any_راس();
        } وإلا إذا (isDefault) {
            fromp = default_Any_راس();
        } وإلا {
            fromp = m_stateVertices.at(from_قدر);
        }
        أرجع جديد Fsm_Arc_کنارا{هذا, fromp, چوٹی, isReset, isCond, isDefault, flp};
    }

    string name() ثابت VL_MT_محفوظ {
        أرجع "FSM "
               + (m_stateVarName.خالی_تھاواں() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_ٹیگ(ناپ_t اشاریہ) ثابت {
        string ٹیگ = state_Var_اندرونی_Name();
        لكل (حرف& ch : ٹیگ) {
            إذا (!std::حرف_یا_ہندسہ(static_cast<بنانشانی حرف>(ch))) ch = '_';
        }
        أرجع "fsm_" + cvt_منزل_Str(اشاریہ) + "_" + ٹیگ;
    }
};

بنية DetectedFsm final {
    std::unique_ptr<Fsm_گراف> graphp;  // Extracted graph for one detected FSM candidate.
};

بنية FsmCaseCandidate final {
    Ast_گرہ* warnNodep = خالی_سوچک;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = خالی_سوچک;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
بنية FsmStateComparison final {
    AstVarScope* stateVscp = خالی_سوچک;  // Compared state variable
    Ast_گرہ_اظہاریہ* valuep = خالی_سوچک;  // Compared constant value expression
    Fsm_State_قدر قدر;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
بنية FsmIfBranch final {
    AstIf* ifp = خالی_سوچک;  // Source if/else-if node
    Ast_گرہ* stmtsp = خالی_سوچک;  // Branch body
    Ast_گرہ_اظہاریہ* valuep = خالی_سوچک;  // Source state value expression
    Fsm_State_قدر from_قدر;  // Encoded source state value
    منطقي has_چوٹی_Guard = خطأ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
بنية FsmIfChainCandidate final {
    AstIf* ifp = خالی_سوچک;  // Top-level if-chain node
    AstVarScope* compareVscp = خالی_سوچک;  // Variable used by every state comparison
    std::سمتیہ<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_گرہ* defaultStmtsp = خالی_سوچک;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
ورتوں FsmAliasMap = std::unordered_map<ثابت AstVarScope*, FsmStateComparison>;
ورتوں Fsm_Cell_پورٹ_Map = std::unordered_map<string, AstVarScope*>;
ورتوں Fsm_Cell_پورٹ_Alias_Map = std::unordered_map<ثابت AstCell*, Fsm_Cell_پورٹ_Map>;

بنية StateConstLabel final {
    string متن;
    منطقي fromParam = خطأ;
    ناپ_t state_اشاریہ = 0;
};

بنية FsmStateSpace final {
    std::سمتیہ<std::pair<string, Fsm_State_قدر>> حالتاں;  // User label and encoded value
    std::unordered_map<Fsm_State_قدر, StateConstLabel, Fsm_State_قدر_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = خالی_سوچک;  // Tracked FSM state variable
    منطقي enumBacked = خطأ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
صنف FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::سمتیہ<DetectedFsm> m_fsms;
    std::map<ثابت AstVarScope*, ناپ_t> m_fsm_اشاریہ;

عام:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ثابت std::map<ثابت AstVarScope*, ناپ_t>::const_iterator it = m_fsm_اشاریہ.لبھو(stateVscp);
        إذا (it != m_fsm_اشاریہ.سرا()) أرجع m_fsms.at(it->دوجا);
        ثابت ناپ_t اشاریہ = m_fsms.ناپ();
        m_fsm_اشاریہ.emplace(stateVscp, اشاریہ);
        m_fsms.emplace_پٹھ();
        أرجع m_fsms.پٹھ();
    }
    ثابت std::سمتیہ<DetectedFsm>& fsms() ثابت { أرجع m_fsms; }
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
    AstScope* m_scopep = خالی_سوچک;
    std::سمتیہ<Fsm_رجسٹر_Candidate> m_رجسٹر_Candidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::سمتیہ<FsmComboAlways> m_one_بلاک_Alwayss;
    std::سمتیہ<FsmComboAlways> m_comboAlwayss;
    std::سمتیہ<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::سمتیہ<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_سیٹ<ثابت AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    Fsm_Cell_پورٹ_Alias_Map m_cell_پورٹ_Aliases;
    Fsm_Cell_پورٹ_Alias_Map m_cell_پورٹ_بچہ_Aliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ساكن Ast_گرہ_DType* unwrapEnumCandidate(Ast_گرہ_DType* dtypep) {
        أرجع dtypep->skip_Ref_منزل_Enump();
    }

    ساكن string candidateConflictContext(Ast_گرہ* laterNodep,
                                           ثابت FsmCaseCandidate& پہلا_Cand) {
        أرجع '\n' + laterNodep->warnContextPrimary() + پہلا_Cand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + پہلا_Cand.stateVscp->prettyNameQ() + '\n'
               + پہلا_Cand.warnNodep->warnContextSecondary();
    }

    ساكن منطقي rejectFsmWrapperCell(AstCell* cellp, ثابت string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        أرجع خطأ;
    }

    ساكن منطقي simple_Param_State_قدر(AstCell* cellp, ثابت string& name, Fsm_State_قدر& قدر,
                                      Ast_گرہ_اظہاریہ*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = خالی_سوچک;
        لكل (Ast_گرہ* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isParam() || varp->name() != name) استمر;
            valuepr = VN_AS(varp->valuep(), گرہ_اظہاریہ);
            أرجع const_قدر_حالت(valuepr, قدر) == Const_قدر_حالت::OK;
        }
        أرجع خطأ;
    }

    ساكن منطقي بچہ_پورٹ_وچ_Scope(AstVarScope* vscp, AstScope* ماپا_Scopep, AstCell*& cellpr) {
        إذا (!vscp->varp()->isIO()) أرجع خطأ;
        AstScope* ثابت scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        إذا (scopep->aboveScopep() != ماپا_Scopep) أرجع خطأ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        أرجع سچ;
    }

    ساكن AstVarScope* simple_سونپو_Var_Scope(Ast_گرہ_اظہاریہ* exprp) {
        AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef);
        أرجع vrefp ? vrefp->varScopep() : خالی_سوچک;
    }

    فراغ جوڑو_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_پورٹ_Aliases.emplace(cellp, Fsm_Cell_پورٹ_Map{});
        m_cell_پورٹ_بچہ_Aliases.emplace(cellp, Fsm_Cell_پورٹ_Map{});
        ثابت std::pair<AstScope*, AstCell*> شے_2{scopep, cellp};
        إذا (std::لبھو(m_wrapperCells.cbegin(), m_wrapperCells.cend(), شے_2)
            != m_wrapperCells.cend()) {
            أرجع;
        }
        m_wrapperCells.emplace_پٹھ(شے_2);
    }

    فراغ collect_Cell_پورٹ_Alias(Ast_سونپو_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ثابت lhsVscp = simple_سونپو_Var_Scope(nodep->lhsp());
        AstVarScope* ثابت rhsVscp = simple_سونپو_Var_Scope(nodep->rhsp());
        إذا (!lhsVscp || !rhsVscp) أرجع;
        AstCell* cellp = خالی_سوچک;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        إذا (بچہ_پورٹ_وچ_Scope(lhsVscp, m_scopep, cellp)) {
            إذا (!fsm_رجسٹر_Wrapper_Desc(cellp)) أرجع;
            UASSERT_OBJ(lhsVscp->varp()->is_داخلہ(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_پورٹ_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_پورٹ_بچہ_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            جوڑو_Wrapper_Cell(m_scopep, cellp);
        } وإلا إذا (بچہ_پورٹ_وچ_Scope(rhsVscp, m_scopep, cellp)) {
            إذا (!fsm_رجسٹر_Wrapper_Desc(cellp)) أرجع;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cell_پورٹ_Aliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cell_پورٹ_بچہ_Aliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            جوڑو_Wrapper_Cell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ثابت string& پورٹ_Name) ثابت {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ثابت Fsm_Cell_پورٹ_Map& ports = m_cell_پورٹ_Aliases.at(cellp);
        ثابت Fsm_Cell_پورٹ_Map::const_iterator پورٹ_It = ports.لبھو(پورٹ_Name);
        أرجع پورٹ_It == ports.سرا() ? خالی_سوچک : پورٹ_It->دوجا;
    }

    AstVarScope* بچہ_Role_Var_Scope(AstCell* cellp, ثابت string& پورٹ_Name) ثابت {
        ثابت Fsm_Cell_پورٹ_Map& ports = m_cell_پورٹ_بچہ_Aliases.at(cellp);
        ثابت Fsm_Cell_پورٹ_Map::const_iterator پورٹ_It = ports.لبھو(پورٹ_Name);
        أرجع پورٹ_It == ports.سرا() ? خالی_سوچک : پورٹ_It->دوجا;
    }

    صنف رجسٹر_Always_Analyzer final {
        AstScope* ثابت m_scopep;  // Scope containing the always block(s) being analyzed

    عام:
        واضح رجسٹر_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::سمتیہ<std::pair<AstCase*, Ast_گرہ_اظہاریہ*>>
        one_بلاک_Candidates(AstAlways* alwaysp) ثابت {
            std::سمتیہ<std::pair<AstCase*, Ast_گرہ_اظہاریہ*>> candidates;
            Ast_گرہ* ثابت stmtsp = alwaysp->stmtsp();
            إذا (AstIf* ثابت پہلا_Ifp = VN_CAST(stmtsp, If)) {
                إذا (AstCase* ثابت casep = VN_CAST(پہلا_Ifp->elsesp(), Case)) {
                    candidates.emplace_پٹھ(casep,
                                            FsmDetectVisitor::isSimpleResetCond(پہلا_Ifp->condp())
                                                ? پہلا_Ifp->condp()
                                                : خالی_سوچک);
                }
            }
            لكل (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstCase* ثابت casep = VN_CAST(nodep, Case))
                    candidates.emplace_پٹھ(casep, خالی_سوچک);
            }
            أرجع candidates;
        }

        std::سمتیہ<std::pair<AstIf*, Ast_گرہ_اظہاریہ*>>
        one_بلاک_If_Candidates(AstAlways* alwaysp) ثابت {
            std::سمتیہ<std::pair<AstIf*, Ast_گرہ_اظہاریہ*>> candidates;
            Ast_گرہ* ثابت stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            إذا (AstIf* ثابت پہلا_Ifp = VN_CAST(stmtsp, If)) {
                إذا (AstIf* ثابت chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(پہلا_Ifp->elsesp()), If)) {
                    candidates.emplace_پٹھ(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(پہلا_Ifp->condp())
                                                ? پہلا_Ifp->condp()
                                                : خالی_سوچک);
                }
            }
            لكل (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) candidates.emplace_پٹھ(ifp, خالی_سوچک);
            }
            أرجع candidates;
        }

        منطقي میل_رجسٹر_Candidate(AstAlways* alwaysp, Fsm_رجسٹر_Candidate& cand) ثابت {
            أرجع FsmDetectVisitor::میل_رجسٹر_Always(alwaysp, m_scopep, cand);
        }

        فراغ build_One_بلاک_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_گرہ_اظہاریہ* resetCondp,
                                    Fsm_رجسٹر_Candidate& reg) ثابت {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.اگلا_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_درخت(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != خالی_سوچک);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ثابت پہلا_Ifp = VN_CAST(alwaysp->stmtsp(), If);
            إذا (پہلا_Ifp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = خالی_سوچک;
                ثابت Reset_سونپو_حالت reset_حالت = FsmDetectVisitor::collectConstStateAssigns(
                    پہلا_Ifp->thensp(), resetStateVscp, reg.resetArcs());
                إذا (reset_حالت == Reset_سونپو_حالت::خالی_تھاواں_3) {
                    reg.resetArcs().clear();
                } وإلا إذا (reset_حالت == Reset_سونپو_حالت::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_قدر reset_قدر;
                    Ast_گرہ* ثابت thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(پہلا_Ifp->thensp());
                    UASSERT_OBJ(thenNodep, پہلا_Ifp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_گرہ_سونپو* ثابت resetAssp = FsmDetectVisitor::سدھا_Const_State_سونپو_گرہ(
                        thenNodep, resetStateVscp, reset_قدر);
                    إذا (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_پٹھ(reset_قدر, resetAssp);
                    }
                } وإلا إذا (reset_حالت == Reset_سونپو_حالت::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    منطقي میل_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_رجسٹر_Candidate& cand) ثابت {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_رجسٹر_Wrapper_Desc(cellp));

        AstVarScope* ثابت اگلا_Vscp = roleVarScope(cellp, roles.d_پورٹ);
        AstVarScope* ثابت stateVscp = roleVarScope(cellp, roles.q_پورٹ);
        إذا (!اگلا_Vscp || !stateVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ثابت clkVscp = roleVarScope(cellp, roles.clk_پورٹ);
        إذا (!clkVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.کنارا_Type = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ڈالو_پٹھ(clkSense);

        AstVarScope* resetVscp = خالی_سوچک;
        إذا (!roles.rst_پورٹ.خالی_تھاواں()) resetVscp = roleVarScope(cellp, roles.rst_پورٹ);
        إذا (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            منطقي inferred_سرگرم_کم = خطأ;
            إذا (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_پورٹ, inferred_سرگرم_کم)) {
                roles.has_Rst_سرگرم_کم = سچ;
                roles.rst_سرگرم_کم = inferred_سرگرم_کم;
            }
        }

        Ast_گرہ_اظہاریہ* resetValuep = خالی_سوچک;
        Fsm_State_قدر reset_قدر;
        ثابت منطقي has_Reset_قدر
            = !roles.rstValParam.خالی_تھاواں()
              && simple_Param_State_قدر(cellp, roles.rstValParam, reset_قدر, resetValuep);
        إذا (resetVscp && roles.has_Rst_سرگرم_کم && has_Reset_قدر) {
            FsmSenDesc rstSense;
            rstSense.کنارا_Type = roles.rst_سرگرم_کم ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ڈالو_پٹھ(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().سرگرم_کم = roles.rst_سرگرم_کم;
            cand.hasResetCond(سچ);
            cand.resetArcs().emplace_پٹھ(reset_قدر, cellp, resetValuep);
        } وإلا إذا (!roles.rst_پورٹ.خالی_تھاواں() || !roles.rstValParam.خالی_تھاواں()) {
            string reason;
            إذا (roles.rst_پورٹ.خالی_تھاواں()) {
                reason = "reset port is not configured";
            } وإلا إذا (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } وإلا إذا (!roles.has_Rst_سرگرم_کم) {
                reason = "reset polarity could not be inferred from the wrapper";
            } وإلا إذا (roles.rstValParam.خالی_تھاواں()) {
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
        cand.alwaysp(خالی_سوچک);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(بچہ_Role_Var_Scope(cellp, roles.q_پورٹ));
        cand.اگلا_Vscp(اگلا_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        أرجع سچ;
    }

    صنف ComboAlwaysAnalyzer final {
    عام:
        بنية Combo_میل final {
            ثابت Fsm_رجسٹر_Candidate* matchedp = خالی_سوچک;
            Ast_گرہ* warnNodep = خالی_سوچک;
        };

    خاص:
        ثابت std::سمتیہ<Fsm_رجسٹر_Candidate>& m_رجسٹر_Candidates;

    عام:
        واضح ComboAlwaysAnalyzer(ثابت std::سمتیہ<Fsm_رجسٹر_Candidate>& رجسٹر_Candidates)
            : m_رجسٹر_Candidates{رجسٹر_Candidates} {}

        Combo_میل میل_Case(Ast_گرہ* stmtsp, AstCase* casep) ثابت {
            Combo_میل میل;
            AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع میل;
            لكل (ثابت Fsm_رجسٹر_Candidate& reg : m_رجسٹر_Candidates) {
                إذا (selp->varScopep() == reg.اگلا_Vscp()) {
                    إذا (!FsmDetectVisitor::has_Canonical_اگلا_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.اگلا_Vscp())) {
                        استمر;
                    }
                } وإلا إذا (selp->varScopep() != reg.stateVscp()) {
                    استمر;
                }
                Ast_گرہ* ثابت warnNodep = FsmDetectVisitor::case_Supported_Transition_گرہ(
                    casep, reg.اگلا_Vscp(), reg.inclCond());
                إذا (!warnNodep) استمر;
                میل.matchedp = &reg;
                میل.warnNodep = warnNodep;
            }
            أرجع میل;
        }

        Combo_میل میل_If_Chain(Ast_گرہ* stmtsp, ثابت FsmIfChainCandidate& chain) ثابت {
            Combo_میل میل;
            لكل (ثابت Fsm_رجسٹر_Candidate& reg : m_رجسٹر_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                إذا (chain.compareVscp == reg.اگلا_Vscp()) {
                    إذا (!FsmDetectVisitor::has_Canonical_اگلا_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.اگلا_Vscp())) {
                        استمر;
                    }
                } وإلا إذا (chain.compareVscp != reg.stateVscp()) {
                    استمر;
                }
                Ast_گرہ* ثابت warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_گرہ(chain, reg.اگلا_Vscp());
                إذا (!warnNodep) استمر;
                میل.matchedp = &reg;
                میل.warnNodep = warnNodep;
            }
            أرجع میل;
        }

        منطقي shouldWarnUnsupported(Ast_گرہ* stmtsp, AstCase* casep) ثابت {
            ثابت AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع خطأ;

            أرجع std::any_دا(
                m_رجسٹر_Candidates.cbegin(), m_رجسٹر_Candidates.cend(),
                [&](ثابت Fsm_رجسٹر_Candidate& reg) -> منطقي {
                    ثابت منطقي matches_اگلا = selp->varScopep() == reg.اگلا_Vscp();
                    ثابت منطقي matchesState = selp->varScopep() == reg.stateVscp();

                    إذا (!matches_اگلا && !matchesState) أرجع خطأ;
                    إذا (matches_اگلا
                        && !FsmDetectVisitor::has_Canonical_اگلا_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.اگلا_Vscp())) {
                        أرجع خطأ;
                    }
                    أرجع FsmDetectVisitor::case_Supported_Transition_گرہ(casep, reg.اگلا_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ساكن منطقي isSimpleResetCond(Ast_گرہ_اظہاریہ* condp) { أرجع VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ساكن FsmResetCondDesc describeResetCond(Ast_گرہ_اظہاریہ* condp) {
        FsmResetCondDesc desc;
        إذا (AstVarRef* ثابت vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        أرجع desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ساكن std::سمتیہ<FsmSenDesc> describe_Sen_درخت(Ast_Sen_درخت* sentreep) {
        std::سمتیہ<FsmSenDesc> senses;
        لكل (Ast_Sen_شے* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), Sen_شے)) {
            Ast_گرہ_Var_Ref* ثابت vrefp = itemp->varrefp();
            إذا (!vrefp) استمر;
            FsmSenDesc desc;
            desc.کنارا_Type = itemp->کنارا_Type().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ڈالو_پٹھ(desc);
        }
        أرجع senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ساكن منطقي isIgnorableStmt(Ast_گرہ* nodep) { أرجع VN_IS(nodep, CoverInc); }

    ساكن Ast_گرہ* skipLeadingIgnorableStmt(Ast_گرہ* nodep) {
        طالما (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ساكن Ast_گرہ* singleMeaningfulStmt(Ast_گرہ* stmtp) {
        Ast_گرہ* resultp = خالی_سوچک;
        لكل (Ast_گرہ* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            إذا (isIgnorableStmt(nodep)) استمر;
            إذا (resultp) أرجع خالی_سوچک;
            resultp = nodep;
        }
        أرجع resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ساكن Ast_گرہ* singleMeaningfulBranch(Ast_گرہ* branchp) {
        إذا (!branchp) أرجع خالی_سوچک;
        أرجع branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ساكن منطقي isPlainComboSentree(ثابت Ast_Sen_درخت* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        لكل (ثابت Ast_Sen_شے* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), Sen_شے)) {
            إذا (senp->کنارا_Type() == VEdgeType::ET_بدلیا) استمر;
            أرجع خطأ;
        }
        أرجع سچ;
    }

    فراغ warnUnsupportedComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_رجسٹر_Candidates};
        Ast_گرہ* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        منطقي warned = خطأ;
        لكل (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            إذا (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = سچ;
            }
            إذا (warned) اكسر;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ساكن Ast_گرہ_سونپو* سدھا_State_سونپو(Ast_گرہ* stmtp, AstVarScope* stateVscp) {
        Ast_گرہ* ثابت nodep = singleMeaningfulBranch(stmtp);
        إذا (!nodep) أرجع خالی_سوچک;
        Ast_گرہ_سونپو* ثابت assp = VN_CAST(nodep, گرہ_سونپو);
        إذا (!assp) أرجع خالی_سوچک;
        AstVarRef* ثابت vrefp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!vrefp || vrefp->varScopep() != stateVscp) أرجع خالی_سوچک;
        أرجع assp;
    }

    ساكن Ast_گرہ_سونپو* گرہ_State_Var_سونپو(Ast_گرہ* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_گرہ_سونپو* ثابت assp = VN_CAST(nodep, گرہ_سونپو);
        إذا (!assp) أرجع خالی_سوچک;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->ادھار_Fromp(سچ), VarRef);
        AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
        إذا (!rhsp || !lhsp) أرجع خالی_سوچک;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        أرجع assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ساكن Ast_گرہ_سونپو* سدھا_Cond_State_Var_سونپو(Ast_گرہ* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_گرہ_اظہاریہ*& condp,
                                                   Fsm_State_قدر& reset_قدر) {
        Ast_گرہ_سونپو* ثابت assp = VN_CAST(nodep, گرہ_سونپو);
        إذا (!assp) أرجع خالی_سوچک;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->ادھار_Fromp(سچ), VarRef);
        AstCond* ثابت rhsp = VN_CAST(assp->rhsp(), Cond);
        إذا (!rhsp || !lhsp) أرجع خالی_سوچک;
        AstVarRef* ثابت elsep = VN_CAST(rhsp->elsep(), VarRef);
        إذا (!elsep) أرجع خالی_سوچک;
        إذا (const_قدر_حالت(rhsp->thenp(), reset_قدر) != Const_قدر_حالت::OK) أرجع خالی_سوچک;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        أرجع assp;
    }

    ساكن Ast_گرہ_سونپو* سدھا_Const_State_سونپو_گرہ(Ast_گرہ* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_قدر& قدر) {
        Ast_گرہ_سونپو* ثابت assp = VN_CAST(nodep, گرہ_سونپو);
        إذا (!assp) أرجع خالی_سوچک;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->ادھار_Fromp(سچ), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        إذا (const_قدر_حالت(assp->rhsp(), قدر) != Const_قدر_حالت::OK) أرجع خالی_سوچک;
        stateVscp = lhsp->varScopep();
        أرجع assp;
    }

    تعداد صنف Reset_سونپو_حالت : uint8_t {
        خالی_تھاواں_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ساكن Reset_سونپو_حالت collectConstStateAssigns(Ast_گرہ* stmtp, AstVarScope*& stateVscp,
                                                      std::سمتیہ<FsmResetArcDesc>& resetArcs) {
        Ast_گرہ* nodep = skipLeadingIgnorableStmt(stmtp);
        إذا (!nodep) أرجع Reset_سونپو_حالت::خالی_تھاواں_3;
        لكل (;; nodep = nodep->nextp()) {
            AstVarScope* سونپو_State_Vscp = خالی_سوچک;
            Fsm_State_قدر قدر;
            Ast_گرہ_سونپو* ثابت assp = سدھا_Const_State_سونپو_گرہ(nodep, سونپو_State_Vscp, قدر);
            إذا (!assp) أرجع Reset_سونپو_حالت::NONE;
            إذا (!stateVscp) stateVscp = سونپو_State_Vscp;
            إذا (سونپو_State_Vscp != stateVscp) أرجع Reset_سونپو_حالت::NONE;
            إذا (!resetArcs.خالی_تھاواں()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                أرجع Reset_سونپو_حالت::MULTI_SAME_STATE;
            }
            resetArcs.emplace_پٹھ(قدر, assp);
            إذا (!nodep->nextp()) أرجع Reset_سونپو_حالت::SINGLE;
        }
    }

    ساكن منطقي has_Canonical_اگلا_State_Default_Before_Case(Ast_گرہ* stmtsp, Ast_گرہ* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* اگلا_Vscp) {
        Ast_گرہ* ثابت bodyp = skipLeadingIgnorableStmt(stmtsp);
        منطقي sawCanonicalDefault = خطأ;
        لكل (Ast_گرہ* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            إذا (nodep == targetp) أرجع sawCanonicalDefault;
            إذا (Ast_گرہ_سونپو* ثابت assp = VN_CAST(nodep, گرہ_سونپو)) {
                AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
                إذا (!lhsp || lhsp->varScopep() != اگلا_Vscp) استمر;
                إذا (sawCanonicalDefault) {
                    ثابت string اگلا_Name = اگلا_Vscp->varp()->prettyNameQ();
                    ثابت string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + اگلا_Name
                                     + ") when the canonical " + اگلا_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    أرجع خطأ;
                }
                إذا (!rhsp || rhsp->varScopep() != stateVscp) أرجع خطأ;
                sawCanonicalDefault = سچ;
            }
        }
    }

    ساكن منطقي if_State_Const_سونپو(Ast_گرہ* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_قدر& then_قدر, Fsm_State_قدر& else_قدر) {
        AstIf* ثابت ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        إذا (!ifp || !ifp->elsesp()) أرجع خطأ;
        AstVarScope* thenVscp = خالی_سوچک;
        AstVarScope* elseVscp = خالی_سوچک;
        Ast_گرہ* ثابت thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_گرہ* ثابت elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        إذا (!thenNodep || !elseNodep) أرجع خطأ;
        إذا (!سدھا_Const_State_سونپو_گرہ(thenNodep, thenVscp, then_قدر)) أرجع خطأ;
        إذا (!سدھا_Const_State_سونپو_گرہ(elseNodep, elseVscp, else_قدر)) أرجع خطأ;
        إذا (thenVscp == stateVscp && elseVscp == stateVscp) أرجع سچ;
        إذا (thenVscp != elseVscp) أرجع خطأ;
        Ast_گرہ* ثابت followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = خالی_سوچک;
        AstVarScope* finalFromVscp = خالی_سوچک;
        Ast_گرہ* ثابت finalNodep = singleMeaningfulBranch(followp);
        إذا (!finalNodep) أرجع خطأ;
        إذا (!گرہ_State_Var_سونپو(finalNodep, finalStateVscp, finalFromVscp)) أرجع خطأ;
        إذا (finalStateVscp != stateVscp) أرجع خطأ;
        إذا (finalFromVscp != thenVscp) أرجع خطأ;
        أرجع سچ;
    }

    ساكن منطقي سدھا_State_Cond_Const_سونپو(Ast_گرہ* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_قدر& then_قدر, Fsm_State_قدر& else_قدر) {
        Ast_گرہ_سونپو* ثابت assp = سدھا_State_سونپو(stmtp, stateVscp);
        إذا (!assp) أرجع خطأ;
        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
        إذا (!condp) أرجع خطأ;
        أرجع const_قدر_حالت(condp->thenp(), then_قدر) == Const_قدر_حالت::OK
               && const_قدر_حالت(condp->elsep(), else_قدر) == Const_قدر_حالت::OK;
    }

    ساكن Ast_گرہ* case_شے_Supported_Arc_گرہ(Ast_Case_شے* itemp, AstVarScope* stateVscp,
                                             منطقي inclCond) {
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع خالی_سوچک;
        }
        Ast_گرہ_سونپو* ثابت assp = سدھا_State_سونپو(itemp->stmtsp(), stateVscp);
        إذا (assp) {
            Fsm_State_قدر منزل_قدر;
            إذا (const_قدر_حالت(assp->rhsp(), منزل_قدر) == Const_قدر_حالت::OK) أرجع assp;
        }
        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        إذا (سدھا_State_Cond_Const_سونپو(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            أرجع assp;
        }
        إذا (if_State_Const_سونپو(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            أرجع singleMeaningfulBranch(itemp->stmtsp());
        }
        أرجع خالی_سوچک;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ساكن Ast_گرہ* case_Supported_Transition_گرہ(AstCase* casep, AstVarScope* stateVscp,
                                                منطقي inclCond) {
        لكل (Ast_Case_شے* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {
            إذا (Ast_گرہ* ثابت nodep = case_شے_Supported_Arc_گرہ(itemp, stateVscp, inclCond))
                أرجع nodep;
        }
        أرجع خالی_سوچک;
    }

    ساكن Ast_گرہ* case_شے_Supported_Arc_گرہ_Like(Ast_گرہ* stmtsp, AstVarScope* stateVscp) {
        إذا (Ast_گرہ_سونپو* ثابت assp = سدھا_State_سونپو(stmtsp, stateVscp)) {
            Fsm_State_قدر منزل_قدر;
            إذا (const_قدر_حالت(assp->rhsp(), منزل_قدر) == Const_قدر_حالت::OK) أرجع assp;
            Fsm_State_قدر then_قدر;
            Fsm_State_قدر else_قدر;
            إذا (سدھا_State_Cond_Const_سونپو(stmtsp, stateVscp, then_قدر, else_قدر)) أرجع assp;
        }
        أرجع خالی_سوچک;
    }

    ساكن Ast_گرہ* if_Chain_Supported_Transition_گرہ(ثابت FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        لكل (ناپ_t i = 0; i < chain.branches.ناپ(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            Ast_گرہ* ثابت nodep = case_شے_Supported_Arc_گرہ_Like(branch.stmtsp, stateVscp);
            إذا (!nodep) أرجع خالی_سوچک;
        }
        أرجع chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ساكن string label_For_قدر(
        ثابت std::unordered_map<Fsm_State_قدر, StateConstLabel, Fsm_State_قدر_Hash>& labels,
        ثابت Fsm_State_قدر& قدر) {
        أرجع labels.at(قدر).متن;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    تعداد صنف Const_قدر_حالت : uint8_t { OK, NOT_CONST, XZ };

    ساكن Const_قدر_حالت const_قدر_حالت(Ast_گرہ_اظہاریہ* exprp, Fsm_State_قدر& قدر) {
        ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
        إذا (!constp) أرجع Const_قدر_حالت::NOT_CONST;
        ثابت V3_نمبر& عدد = constp->عدد();
        إذا (عدد.isAnyXZ()) أرجع Const_قدر_حالت::XZ;
        قدر = Fsm_State_قدر{عدد};
        أرجع Const_قدر_حالت::OK;
    }

    ساكن منطقي خالص_State_Comparison_No_Alias(Ast_گرہ_اظہاریہ* exprp, FsmStateComparison& cmp) {
        AstEq* ثابت eqp = VN_CAST(exprp, Eq);
        إذا (!eqp) أرجع خطأ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_گرہ_اظہاریہ* valuep = eqp->rhsp();
        إذا (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->ادھار_Fromp(سچ), VarRef);
            إذا (!vrefp) { أرجع خطأ; }
            valuep = eqp->lhsp();
        }

        Fsm_State_قدر قدر;
        إذا (const_قدر_حالت(valuep, قدر) != Const_قدر_حالت::OK) أرجع خطأ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.قدر = قدر;
        أرجع سچ;
    }

    ساكن منطقي خالص_State_Comparison(Ast_گرہ_اظہاریہ* exprp, ثابت FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        إذا (خالص_State_Comparison_No_Alias(exprp, cmp)) أرجع سچ;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        إذا (AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) {
            ثابت FsmAliasMap::const_iterator it = aliases.لبھو(vrefp->varScopep());
            إذا (it == aliases.سرا()) أرجع خطأ;
            cmp = it->دوجا;
            أرجع سچ;
        }
        أرجع خطأ;
    }

    ساكن منطقي supported_چوٹی_Level_Guard(Ast_گرہ_اظہاریہ* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        إذا (VN_IS(exprp, Or)) أرجع خطأ;
        إذا (VN_IS(exprp, RedAnd)) أرجع خطأ;
        إذا (VN_IS(exprp, RedOr)) أرجع خطأ;
        إذا (VN_IS(exprp, RedXor)) أرجع خطأ;
        أرجع سچ;
    }

    ساكن منطقي resolveIfPredicate(Ast_گرہ_اظہاریہ* exprp, ثابت FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, منطقي& hasGuard) {
        std::سمتیہ<Ast_گرہ_اظہاریہ*> terms;
        std::سمتیہ<Ast_گرہ_اظہاریہ*> pending;
        pending.ڈالو_پٹھ(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        طالما (!pending.خالی_تھاواں()) {
            Ast_گرہ_اظہاریہ* ثابت nodep = pending.پٹھ();
            pending.نکالو_پٹھ();
            إذا (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
                pending.ڈالو_پٹھ(andp->rhsp());
                pending.ڈالو_پٹھ(andp->lhsp());
            } وإلا {
                terms.ڈالو_پٹھ(nodep);
            }
        }

        منطقي sawComparison = خطأ;
        لكل (ناپ_t i = 0; i < terms.ناپ(); ++i) {
            Ast_گرہ_اظہاریہ* ثابت termp = terms[i];
            FsmStateComparison رکن_Cmp;
            إذا (خالص_State_Comparison(termp, aliases, رکن_Cmp /*ref*/)) {
                إذا (sawComparison) أرجع خطأ;
                cmp = رکن_Cmp;
                sawComparison = سچ;
                استمر;
            }
            إذا (!supported_چوٹی_Level_Guard(termp)) أرجع خطأ;
            hasGuard = سچ;
        }
        أرجع sawComparison;
    }

    ساكن فراغ جوڑو_Alias(FsmAliasMap& aliases, std::unordered_سیٹ<ثابت AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ثابت FsmStateComparison& cmp) {
        إذا (ambiguous.لبھو(aliasVscp) != ambiguous.سرا()) أرجع;
        ثابت FsmAliasMap::iterator it = aliases.لبھو(aliasVscp);
        إذا (it == aliases.سرا()) {
            aliases.emplace(aliasVscp, cmp);
            أرجع;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        إذا (it->دوجا.stateVscp == cmp.stateVscp && it->دوجا.قدر == cmp.قدر) أرجع;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        أرجع;
    }

    ساكن فراغ collect_Alias_From_سونپو(Ast_گرہ_سونپو* assp, FsmAliasMap& aliases,
                                       std::unordered_سیٹ<ثابت AstVarScope*>& ambiguous) {
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!lhsp) أرجع;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        إذا (!خالص_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) أرجع;
        جوڑو_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_گرہ* stmtsp, Ast_گرہ* limitp) ثابت {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_سیٹ<ثابت AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        لكل (Ast_گرہ* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            إذا (Ast_گرہ_سونپو* ثابت assp = VN_CAST(nodep, گرہ_سونپو)) {
                collect_Alias_From_سونپو(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        لكل (ثابت AstVarScope* ثابت vscp : ambiguous) aliases.erase(vscp);
        أرجع aliases;
    }

    ساكن منطقي collectIfChain(AstIf* ifp, ثابت FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_سیٹ<string> تکیا_اقدار;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        طالما (سچ) {
            FsmStateComparison cmp;
            منطقي hasGuard = خطأ;
            إذا (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) أرجع خطأ;
            إذا (chain.compareVscp && chain.compareVscp != cmp.stateVscp) أرجع خطأ;
            إذا (!تکیا_اقدار.داخل_کرو(cmp.قدر.کلید()).دوجا) أرجع خطأ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ڈالو_پٹھ(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.قدر, hasGuard});

            Ast_گرہ* ثابت elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            إذا (!elseNodep) اكسر;
            إذا (AstIf* ثابت elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                استمر;
            }
            chain.defaultStmtsp = elseNodep;
            اكسر;
        }
        أرجع chain.branches.ناپ() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ساكن منطقي validate_معلوم_State_قدر(Ast_گرہ* nodep, ثابت FsmStateSpace& stateSpace,
                                        ثابت Fsm_State_قدر& قدر, ثابت string& role) {
        إذا (stateSpace.labels.لبھو(قدر) != stateSpace.labels.سرا()) أرجع سچ;
        إذا (stateSpace.enumBacked) {
            ثابت string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + قدر.warn_متن()
                                        + " is not present in the declared enum");
            أرجع خطأ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + قدر.warn_متن()
                                    + " is not present in the inferred state space");
        أرجع خطأ;
    }

    ساكن StateConstLabel stateLabelForConst(AstConst* constp) {
        ثابت string name = constp->origParamName();
        إذا (!name.خالی_تھاواں()) أرجع StateConstLabel{Ast_گرہ::prettyName(name), سچ, 0};
        أرجع StateConstLabel{constp->name(), خطأ, 0};
    }

    ساكن فراغ updateStateLabel(FsmStateSpace& stateSpace, ثابت Fsm_State_قدر& قدر,
                                 ثابت StateConstLabel& label) {
        stateSpace.حالتاں.at(stateSpace.labels.at(قدر).state_اشاریہ).پہلا = label.متن;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ساكن منطقي میل_رجسٹر_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_رجسٹر_Candidate& cand) {
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->has_کنارا()) أرجع خطأ;

        Ast_گرہ* ثابت stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_گرہ* ثابت nodep = singleMeaningfulStmt(stmtsp);
        إذا (!nodep) أرجع خطأ;

        AstVarScope* stateVscp = خالی_سوچک;
        AstVarScope* اگلا_Vscp = خالی_سوچک;
        إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) أرجع خطأ;
            AstVarScope* resetStateVscp = خالی_سوچک;
            ثابت Reset_سونپو_حالت reset_حالت
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ثابت منطقي خالی_تھاواں_Reset_Branch = reset_حالت == Reset_سونپو_حالت::خالی_تھاواں_3;
            إذا (خالی_تھاواں_Reset_Branch) {
                cand.resetArcs().clear();
            } وإلا إذا (reset_حالت == Reset_سونپو_حالت::NONE) {
                cand.resetArcs().clear();
                Fsm_State_قدر reset_قدر;
                Ast_گرہ* ثابت thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_گرہ_سونپو* ثابت resetAssp
                    = سدھا_Const_State_سونپو_گرہ(thenNodep, resetStateVscp, reset_قدر);
                إذا (!resetAssp) أرجع خطأ;
                cand.resetArcs().emplace_پٹھ(reset_قدر, resetAssp);
            } وإلا إذا (reset_حالت == Reset_سونپو_حالت::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_گرہ* ثابت elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            إذا (!گرہ_State_Var_سونپو(elseNodep, stateVscp, اگلا_Vscp)) أرجع خطأ;
            إذا (!خالی_تھاواں_Reset_Branch && resetStateVscp != stateVscp) أرجع خطأ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != خالی_سوچک);
        } وإلا {
            Ast_گرہ_اظہاریہ* resetCondp = خالی_سوچک;
            Fsm_State_قدر reset_قدر;
            // Note the active low 'reset ? next : CONST' form is not accepted
            إذا (Ast_گرہ_سونپو* ثابت assp
                = سدھا_Cond_State_Var_سونپو(nodep, stateVscp, اگلا_Vscp, resetCondp, reset_قدر)) {
                cand.resetArcs().emplace_پٹھ(reset_قدر, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != خالی_سوچک);
            } وإلا إذا (!گرہ_State_Var_سونپو(nodep, stateVscp, اگلا_Vscp)) {
                أرجع خطأ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.اگلا_Vscp(اگلا_Vscp);
        cand.senses() = describe_Sen_درخت(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        أرجع سچ;
    }

    ساكن منطقي جوڑو_قدر_منزل_State_Space(Ast_گرہ* nodep, FsmStateSpace& stateSpace,
                                     ثابت Fsm_State_قدر& قدر, StateConstLabel label) {
        ثابت خودکار labelIt = stateSpace.labels.لبھو(قدر);
        إذا (labelIt != stateSpace.labels.سرا()) {
            StateConstLabel& existingLabel = labelIt->دوجا;
            إذا (existingLabel.متن != label.متن && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + قدر.warn_متن() + ": " + existingLabel.متن
                                            + " and " + label.متن);
                أرجع خطأ;
            }
            إذا (!existingLabel.fromParam && label.fromParam) {
                existingLabel.متن = label.متن;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, قدر, label);
            }
        } وإلا {
            StateConstLabel storedLabel = label;
            storedLabel.state_اشاریہ = stateSpace.حالتاں.ناپ();
            stateSpace.حالتاں.emplace_پٹھ(label.متن, قدر);
            stateSpace.labels.emplace(قدر, storedLabel);
        }
        أرجع سچ;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ساكن منطقي جوڑو_اظہاریہ_منزل_State_Space(Ast_گرہ_اظہاریہ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_قدر قدر;
        ثابت Const_قدر_حالت حالت = const_قدر_حالت(valuep, قدر);
        إذا (حالت != Const_قدر_حالت::OK) {
            إذا (حالت == Const_قدر_حالت::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع جوڑو_قدر_منزل_State_Space(valuep, stateSpace, قدر, stateLabelForConst(constp));
    }

    ساكن منطقي جوڑو_Optional_نشانہ_اظہاریہ_منزل_State_Space(Ast_گرہ_اظہاریہ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_قدر قدر;
        ثابت Const_قدر_حالت حالت = const_قدر_حالت(valuep, قدر);
        إذا (حالت != Const_قدر_حالت::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع جوڑو_قدر_منزل_State_Space(valuep, stateSpace, قدر, stateLabelForConst(constp));
    }

    ساكن فراغ جوڑو_Reset_Targets_منزل_State_Space(ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.منزل_قدر().ascii(), خطأ, 0};
            إذا (AstConst* ثابت constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                جوڑو_قدر_منزل_State_Space(resetArc.nodep(), stateSpace, resetArc.منزل_قدر(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ساكن منطقي collectDeclaredStateSpace(Ast_گرہ* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, منطقي& needs_سروت_اقدار) {
        AstVar* ثابت stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        إذا (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ثابت منطقي forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        إذا (enump) {
            stateSpace.enumBacked = سچ;
            لكل (Ast_Enum_شے* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_شے)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                ثابت Fsm_State_قدر قدر{constp->عدد()};
                ثابت ناپ_t state_اشاریہ = stateSpace.حالتاں.ناپ();
                stateSpace.حالتاں.emplace_پٹھ(itemp->name(), قدر);
                stateSpace.labels.emplace(قدر,
                                          StateConstLabel{itemp->name(), خطأ, state_اشاریہ});
            }
            أرجع stateSpace.حالتاں.ناپ() >= 2;
        }

        إذا (forced) {
            needs_سروت_اقدار = سچ;
            أرجع سچ;
        }

        needs_سروت_اقدار = سچ;
        أرجع سچ;
    }

    سانچا <قسم_ناں T_ValuepVisitor>
    ساكن منطقي collect_State_Space_From_اقدار(Ast_گرہ* warnNodep, AstVarScope* stateVscp,
                                            ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ثابت T_ValuepVisitor& ویکھو_Valueps) {
        منطقي needs_سروت_اقدار = خطأ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        إذا (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_سروت_اقدار)) {
            أرجع خطأ;
        }
        إذا (!needs_سروت_اقدار) أرجع سچ;
        جوڑو_Reset_Targets_منزل_State_Space(resetArcs, stateSpace);
        إذا (!ویکھو_Valueps(
                [&](Ast_گرہ_اظہاریہ* valuep) { أرجع جوڑو_اظہاریہ_منزل_State_Space(valuep, stateSpace); })) {
            أرجع خطأ;
        }
        أرجع stateSpace.حالتاں.ناپ() >= 2;
    }

    ساكن منطقي collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* سونپو_Vscp,
                                  ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_اقدار(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, سونپو_Vscp, &stateSpace](ثابت خودکار& ویکھو_Valuep) {
                لكل (Ast_Case_شے* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Case_شے)) {
                    إذا (!itemp->isDefault()) {
                        لكل (Ast_گرہ_اظہاریہ* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), گرہ_اظہاریہ)) {
                            إذا (!ویکھو_Valuep(condp)) أرجع خطأ;
                        }
                    }
                    إذا (Ast_گرہ_سونپو* ثابت assp
                        = سدھا_State_سونپو(itemp->stmtsp(), سونپو_Vscp)) {
                        Fsm_State_قدر then_قدر;
                        Fsm_State_قدر else_قدر;
                        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                        إذا (condp
                            && سدھا_State_Cond_Const_سونپو(itemp->stmtsp(), سونپو_Vscp, then_قدر,
                                                          else_قدر)) {
                            إذا (!ویکھو_Valuep(condp->thenp())) أرجع خطأ;
                            إذا (!ویکھو_Valuep(condp->elsep())) أرجع خطأ;
                        } وإلا إذا (!جوڑو_Optional_نشانہ_اظہاریہ_منزل_State_Space(assp->rhsp(), stateSpace)) {
                            أرجع خطأ;
                        }
                    }
                }
                أرجع سچ;
            });
    }

    ساكن منطقي collectStateSpace(ثابت FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* سونپو_Vscp,
                                  ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_اقدار(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, سونپو_Vscp, &stateSpace](ثابت خودکار& ویکھو_Valuep) {
                لكل (ثابت FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(ویکھو_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_گرہ_سونپو* ثابت assp = سدھا_State_سونپو(branch.stmtsp, سونپو_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_قدر then_قدر;
                    Fsm_State_قدر else_قدر;
                    AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                    إذا (condp) {
                        UASSERT_OBJ(سدھا_State_Cond_Const_سونپو(branch.stmtsp, سونپو_Vscp,
                                                               then_قدر, else_قدر),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        إذا (!ویکھو_Valuep(condp->thenp())) أرجع خطأ;
                        إذا (!ویکھو_Valuep(condp->elsep())) أرجع خطأ;
                    } وإلا إذا (!جوڑو_Optional_نشانہ_اظہاریہ_منزل_State_Space(assp->rhsp(), stateSpace)) {
                        أرجع خطأ;
                    }
                }
                أرجع سچ;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ساكن منطقي emit_Case_شے_Arcs(Fsm_گراف& گراف, Ast_Case_شے* itemp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, منطقي inclCond) {
        std::سمتیہ<std::pair<string, Fsm_State_قدر>> froms;
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع خطأ;
            froms.emplace_پٹھ("default", Fsm_State_قدر{});
        } وإلا {
            لكل (Ast_گرہ_اظہاریہ* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), گرہ_اظہاریہ)) {
                Fsm_State_قدر قدر;
                إذا (const_قدر_حالت(condp, قدر) != Const_قدر_حالت::OK) استمر;
                إذا (!validate_معلوم_State_قدر(condp, stateSpace, قدر, "source")) أرجع سچ;
                froms.emplace_پٹھ(label_For_قدر(stateSpace.labels, قدر), قدر);
            }
            إذا (froms.خالی_تھاواں()) أرجع خطأ;
        }

        إذا (Ast_گرہ_سونپو* ثابت assp = سدھا_State_سونپو(itemp->stmtsp(), stateVscp)) {
            Fsm_State_قدر منزل_قدر;
            ثابت Const_قدر_حالت حالت = const_قدر_حالت(assp->rhsp(), منزل_قدر);
            إذا (حالت == Const_قدر_حالت::OK) {
                إذا (!validate_معلوم_State_قدر(assp, stateSpace, منزل_قدر, "target")) أرجع سچ;
                لكل (ثابت std::pair<string, Fsm_State_قدر>& from : froms) {
                    گراف.جوڑو_Arc(from.دوجا, منزل_قدر, خطأ, خطأ, itemp->isDefault(),
                                 assp->fileline());
                }
                أرجع سچ;
            }
        }

        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        إذا (سدھا_State_Cond_Const_سونپو(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)
            || if_State_Const_سونپو(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            إذا (!validate_معلوم_State_قدر(itemp->stmtsp(), stateSpace, then_قدر, "target"))
                أرجع سچ;
            إذا (!validate_معلوم_State_قدر(itemp->stmtsp(), stateSpace, else_قدر, "target"))
                أرجع سچ;
            لكل (ثابت Fsm_State_قدر& branch_قدر : {then_قدر, else_قدر}) {
                لكل (ثابت std::pair<string, Fsm_State_قدر>& from : froms) {
                    گراف.جوڑو_Arc(from.دوجا, branch_قدر, خطأ, سچ, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            أرجع سچ;
        }

        أرجع خطأ;
    }

    ساكن فراغ emitStmtArcsFrom(Fsm_گراف& گراف, Ast_گرہ* stmtsp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, Fsm_State_قدر from_قدر,
                                 منطقي isDefault, منطقي forceCond) {
        Ast_گرہ_سونپو* ثابت assp = سدھا_State_سونپو(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_قدر منزل_قدر;
        ثابت Const_قدر_حالت حالت = const_قدر_حالت(assp->rhsp(), منزل_قدر);
        إذا (حالت == Const_قدر_حالت::OK) {
            إذا (!validate_معلوم_State_قدر(assp, stateSpace, منزل_قدر, "target")) أرجع;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            گراف.جوڑو_Arc(from_قدر, منزل_قدر, خطأ, forceCond, isDefault, assp->fileline());
            أرجع;
        }

        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        ثابت منطقي cond_سونپو
            = سدھا_State_Cond_Const_سونپو(stmtsp, stateVscp, then_قدر, else_قدر);
        UASSERT_OBJ(cond_سونپو, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        إذا (!validate_معلوم_State_قدر(stmtsp, stateSpace, then_قدر, "target")) أرجع;
        إذا (!validate_معلوم_State_قدر(stmtsp, stateSpace, else_قدر, "target")) أرجع;
        لكل (ثابت Fsm_State_قدر& branch_قدر : {then_قدر, else_قدر}) {
            گراف.جوڑو_Arc(from_قدر, branch_قدر, خطأ, سچ, isDefault, stmtsp->fileline());
        }
    }

    ساكن فراغ emitIfChainArcs(Fsm_گراف& گراف, ثابت FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ثابت FsmStateSpace& stateSpace) {
        لكل (ناپ_t i = 0; i < chain.branches.ناپ(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            إذا (!validate_معلوم_State_قدر(branch.ifp, stateSpace, branch.from_قدر, "source"))
                أرجع;
            emitStmtArcsFrom(گراف, branch.stmtsp, stateVscp, stateSpace, branch.from_قدر, خطأ,
                             branch.has_چوٹی_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ساكن فراغ جوڑو_Reset_Arcs(Fsm_گراف& گراف, ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                             ثابت FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            إذا (!validate_معلوم_State_قدر(resetArc.nodep(), stateSpace, resetArc.منزل_قدر(),
                                         "target"))
                استمر;
            گراف.جوڑو_Arc(Fsm_State_قدر{}, resetArc.منزل_قدر(), سچ, خطأ, خطأ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    فراغ processCase(AstCase* casep, AstVarScope* سونپو_Vscp, ثابت Fsm_رجسٹر_Candidate& reg) {
        UASSERT_OBJ(سونپو_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        إذا (!collectStateSpace(casep, stateVscp, سونپو_Vscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& اندراج = m_state.fsmFor(stateVscp);
        إذا (!اندراج.graphp) {
            اندراج.graphp.reset(جديد Fsm_گراف{});
            اندراج.graphp->scopep(reg.scopep());
            اندراج.graphp->stateAlwaysp(reg.alwaysp());
            اندراج.graphp->stateVarName(stateVscp->prettyName());
            اندراج.graphp->state_Var_اندرونی_Name(stateVscp->varp()->name());
            اندراج.graphp->stateVarScopep(stateVscp);
            اندراج.graphp->sampleVarScopep(reg.sampleVscp());
            اندراج.graphp->senses() = reg.senses();
            اندراج.graphp->resetCond() = reg.resetCond();
            اندراج.graphp->hasResetCond(reg.hasResetCond());
            اندراج.graphp->inclCond(reg.inclCond());
            اندراج.graphp->fileline(casep->fileline());
            لكل (ثابت std::pair<string, Fsm_State_قدر>& state : stateSpace.حالتاں) {
                اندراج.graphp->جوڑو_State_راس(state.پہلا, state.دوجا);
            }
            جوڑو_Reset_Arcs(*اندراج.graphp, reg.resetArcs(), stateSpace);
        }
        لكل (Ast_Case_شے* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {
            emit_Case_شے_Arcs(*اندراج.graphp, itemp, سونپو_Vscp, stateSpace,
                             اندراج.graphp->inclCond());
        }
    }

    فراغ processIfChain(ثابت FsmIfChainCandidate& chain, AstVarScope* سونپو_Vscp,
                        ثابت Fsm_رجسٹر_Candidate& reg) {
        UASSERT_OBJ(سونپو_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        إذا (!collectStateSpace(chain, stateVscp, سونپو_Vscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& اندراج = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!اندراج.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        اندراج.graphp.reset(جديد Fsm_گراف{});
        اندراج.graphp->scopep(reg.scopep());
        اندراج.graphp->stateAlwaysp(reg.alwaysp());
        اندراج.graphp->stateVarName(stateVscp->prettyName());
        اندراج.graphp->state_Var_اندرونی_Name(stateVscp->varp()->name());
        اندراج.graphp->stateVarScopep(stateVscp);
        اندراج.graphp->sampleVarScopep(reg.sampleVscp());
        اندراج.graphp->senses() = reg.senses();
        اندراج.graphp->resetCond() = reg.resetCond();
        اندراج.graphp->hasResetCond(reg.hasResetCond());
        اندراج.graphp->inclCond(reg.inclCond());
        اندراج.graphp->fileline(chain.ifp->fileline());
        لكل (ثابت std::pair<string, Fsm_State_قدر>& state : stateSpace.حالتاں) {
            اندراج.graphp->جوڑو_State_راس(state.پہلا, state.دوجا);
        }
        جوڑو_Reset_Arcs(*اندراج.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*اندراج.graphp, chain, سونپو_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    فراغ process_One_بلاک_Always(ثابت FsmComboAlways& one_بلاک) {
        ثابت رجسٹر_Always_Analyzer analyzer{one_بلاک.scopep()};
        AstAlways* ثابت alwaysp = one_بلاک.alwaysp();
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->has_کنارا()) أرجع;
        ثابت std::سمتیہ<std::pair<AstCase*, Ast_گرہ_اظہاریہ*>> candidates
            = analyzer.one_بلاک_Candidates(alwaysp);

        FsmCaseCandidate پہلا_Cand;
        لكل (ثابت std::pair<AstCase*, Ast_گرہ_اظہاریہ*>& cand : candidates) {
            AstVarRef* ثابت selp = VN_CAST(cand.پہلا->exprp(), VarRef);
            AstVarScope* ثابت vscp = selp ? selp->varScopep() : خالی_سوچک;
            إذا (!vscp) استمر;
            إذا (!پہلا_Cand.stateVscp) {
                پہلا_Cand.warnNodep = cand.پہلا;
                پہلا_Cand.stateVscp = vscp;
                Fsm_رجسٹر_Candidate reg;
                analyzer.build_One_بلاک_Candidate(alwaysp, vscp, cand.دوجا, reg);
                processCase(cand.پہلا, vscp, reg);
            } وإلا إذا (vscp != پہلا_Cand.stateVscp) {
                cand.پہلا->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.پہلا, پہلا_Cand));
            } وإلا {
                cand.پہلا->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.پہلا, پہلا_Cand));
            }
        }

        إذا (پہلا_Cand.stateVscp) أرجع;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ثابت std::سمتیہ<std::pair<AstIf*, Ast_گرہ_اظہاریہ*>> ifCandidates
            = analyzer.one_بلاک_If_Candidates(alwaysp);
        لكل (ثابت std::pair<AstIf*, Ast_گرہ_اظہاریہ*>& cand : ifCandidates) {
            ثابت FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.پہلا);
            FsmIfChainCandidate chain;
            إذا (!collectIfChain(cand.پہلا, aliases, chain)) استمر;
            AstVarScope* ثابت vscp = chain.compareVscp;
            إذا (!if_Chain_Supported_Transition_گرہ(chain, vscp)) استمر;
            إذا (!پہلا_Cand.stateVscp) {
                پہلا_Cand.warnNodep = cand.پہلا;
                پہلا_Cand.stateVscp = vscp;
                Fsm_رجسٹر_Candidate reg;
                analyzer.build_One_بلاک_Candidate(alwaysp, vscp, cand.دوجا, reg);
                processIfChain(chain, vscp, reg);
            } وإلا إذا (vscp != پہلا_Cand.stateVscp) {
                cand.پہلا->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.پہلا, پہلا_Cand));
            } وإلا {
                cand.پہلا->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.پہلا, پہلا_Cand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    ساكن فراغ warnComboSameAlways(Ast_گرہ* warnNodep, ثابت FsmCaseCandidate& پہلا_Cand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, پہلا_Cand));
    }

    فراغ processComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_رجسٹر_Candidates};
        Ast_گرہ* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate پہلا_Cand;
        لكل (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            ثابت ComboAlwaysAnalyzer::Combo_میل میل = analyzer.میل_Case(stmtsp, casep);
            ثابت Fsm_رجسٹر_Candidate* ثابت matchedp = میل.matchedp;
            Ast_گرہ* ثابت matchedWarnNodep = میل.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!پہلا_Cand.stateVscp) {
                ثابت خودکار داخل_کرو_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!داخل_کرو_Pair.دوجا) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  داخل_کرو_Pair.پہلا->دوجا));
                    استمر;
                }
                پہلا_Cand.warnNodep = matchedWarnNodep;
                پہلا_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->اگلا_Vscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != پہلا_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, پہلا_Cand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, پہلا_Cand));
        }
        إذا (پہلا_Cand.stateVscp) أرجع;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        لكل (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ثابت ifp = VN_CAST(nodep, If);
            إذا (!ifp) استمر;
            FsmIfChainCandidate chain;
            ثابت FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            إذا (!collectIfChain(ifp, aliases, chain)) استمر;
            ثابت ComboAlwaysAnalyzer::Combo_میل میل = analyzer.میل_If_Chain(stmtsp, chain);
            ثابت Fsm_رجسٹر_Candidate* ثابت matchedp = میل.matchedp;
            Ast_گرہ* ثابت matchedWarnNodep = میل.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!پہلا_Cand.stateVscp) {
                ثابت std::pair<std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate>::iterator,
                                منطقي>
                    داخل_کرو_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!داخل_کرو_Pair.دوجا) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  داخل_کرو_Pair.پہلا->دوجا));
                    استمر;
                }
                پہلا_Cand.warnNodep = matchedWarnNodep;
                پہلا_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->اگلا_Vscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != پہلا_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, پہلا_Cand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, پہلا_Cand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    فراغ ویکھو(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    فراغ ویکھو(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_سونپو) {
            iterateChildren(nodep);
            أرجع;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_بلاک_Alwayss.emplace_پٹھ(m_scopep, nodep);
        ثابت رجسٹر_Always_Analyzer analyzer{m_scopep};
        Fsm_رجسٹر_Candidate reg;
        إذا (analyzer.میل_رجسٹر_Candidate(nodep, reg)) {
            AstVarScope* ثابت stateVscp = reg.stateVscp();
            ثابت منطقي لبھیا
                = std::any_دا(m_رجسٹر_Candidates.cbegin(), m_رجسٹر_Candidates.cend(),
                              [stateVscp](ثابت Fsm_رجسٹر_Candidate& existing) {
                                  أرجع existing.stateVscp() == stateVscp;
                              });
            إذا (!لبھیا) { m_رجسٹر_Candidates.emplace_پٹھ(reg); }
        }
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_پٹھ(m_scopep, nodep);
        } وإلا إذا (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            إذا (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_پٹھ(m_scopep, nodep);
            } وإلا {
                m_nonComboAlwayss.emplace_پٹھ(m_scopep, nodep);
            }
        }
    }

    فراغ ویکھو(Ast_سونپو_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_سونپو(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_پورٹ_Alias(nodep);
        iterateChildren(nodep);
    }

    فراغ ویکھو(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        إذا (m_scopep && fsm_رجسٹر_Wrapper_Desc(nodep)) جوڑو_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        لكل (ثابت std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_رجسٹر_Candidate reg;
            إذا (میل_Fsm_Wrapper_Cell(wrapperCell.پہلا, wrapperCell.دوجا, reg)) {
                m_رجسٹر_Candidates.emplace_پٹھ(reg);
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
    ثابت Fsm_گراف& m_گراف;  // Fsm graph
    std::سمتیہ<ثابت Fsm_State_راس*> m_حالتاں;  // States vertexes inside fsm graph
    std::unordered_سیٹ<uint64_t>
        m_undefined_حالتاں;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ثابت Fsm_راس* m_defaultVertexp = خالی_سوچک;  // Default vertex
    ثابت Fsm_راس* m_resetVertexp = خالی_سوچک;  // Reset vertex
    ناپ_t m_کنارے_گنتی = 0;  // Total count of edges
    AstVarScope* ثابت m_prevVscp;  // Previous state of fsm
    File_سطر* ثابت m_flp;  // Fileline pointer to use across the builder
    ثابت std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_گرہ_Stmt* m_coverIncp = خالی_سوچک;  // Increments proper CoverDecl after each change
    Ast_گرہ_Stmt* m_initialCoverIncp = خالی_سوچک;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = خالی_سوچک;  // List of CoverDecls
    منطقي m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ثابت uint64_t قدر) {
        V3_نمبر عدد{static_cast<Ast_گرہ*>(خالی_سوچک), m_prevVscp->چوڑائی()};
        عدد.سیٹ_Quad(قدر);
        عدد.isSigned(m_prevVscp->dtypep()->isSigned());
        أرجع جديد AstConst{m_flp, عدد};
    }

    std::string quad_منزل_ثنائی_ہندسہ_Stirng_Label(ثابت uint64_t val) {
        صحيح چوڑائی = m_prevVscp->چوڑائی();
        UASSERT(چوڑائی < 64, "We should not expand values that wide");
        std::string نتیجہ = std::منزل_string(چوڑائی) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        طالما (--چوڑائی >= 0) نتیجہ.ڈالو_پٹھ("01"[(val >> چوڑائی) & 1]);
        أرجع نتیجہ;
    }

    فراغ init_Existing_حالتاں() {
        لكل (ثابت V3_گراف_راس& vtx : m_گراف.vertices()) {
            ثابت Fsm_راس* ثابت vertexp = vtx.as<Fsm_راس>();
            m_کنارے_گنتی += vertexp->وچ_کنارے().ناپ();
            إذا (ثابت Fsm_State_راس* ثابت statep = vertexp->cast<Fsm_State_راس>()) {
                m_حالتاں.ڈالو_پٹھ(statep);
            } وإلا إذا (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_گراف.sampleVarScopep(),
                            "Two defaults?");
            } وإلا /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_گراف.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ثابت صحيح چوڑائی = m_گراف.sampleVarScopep()->چوڑائی();
        m_isComplete = چوڑائی < 64 && m_حالتاں.ناپ() == (static_cast<ناپ_t>(1) << چوڑائی);
    }

    فراغ init_Undefined_حالتاں() {
        UASSERT_OBJ(m_گراف.sampleVarScopep()->چوڑائی() < 64, m_گراف.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ثابت uint64_t combinations_گنتی = static_cast<ناپ_t>(1)
                                           << m_گراف.sampleVarScopep()->چوڑائی();
        // Rolling trick allows to cut peak memory usage
        منطقي undefRolled = خطأ;
        خودکار rollUndefs = [هذا, combinations_گنتی] {
            لكل (uint64_t i = 0; i < combinations_گنتی; ++i) {
                ثابت خودکار داخل_کرو = m_undefined_حالتاں.داخل_کرو(i);
                إذا (!داخل_کرو.دوجا) m_undefined_حالتاں.erase(داخل_کرو.پہلا);
            }
        };
        لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتاں) {
            ثابت V3_نمبر& عدد = statep->قدر().عدد();
            ثابت uint64_t قدر = عدد.منزل_UQuad();
            إذا (!m_isComplete) {
                إذا (undefRolled) {
                    UASSERT_OBJ(m_undefined_حالتاں.erase(قدر) == 1, m_گراف.sampleVarScopep(),
                                "Two states has the same value?");
                } وإلا {
                    UASSERT_OBJ(m_undefined_حالتاں.داخل_کرو(قدر).دوجا, m_گراف.sampleVarScopep(),
                                "Two states has the same value?");
                    إذا (m_undefined_حالتاں.ناپ() * 2 >= combinations_گنتی) {
                        undefRolled = سچ;
                        rollUndefs();
                    }
                }
            }
        }
        إذا (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_گرہ_اظہاریہ* buildResetCond() ثابت {
        Ast_گرہ_اظہاریہ* ثابت refp
            = جديد AstVarRef{m_flp, m_گراف.resetCond().varScopep, VAccess::READ};
        أرجع m_گراف.resetCond().سرگرم_کم
                   ? static_cast<Ast_گرہ_اظہاریہ*>(جديد AstLogNot{m_flp, refp})
                   : refp;
    }

    ناپ_t get_Arc_گنتی_With_Expansion(ثابت VFsmExpandType expand) ثابت {
        ناپ_t arc_گنتی_With_Expansion = ~0ULL;
        بدل (expand) {
        حالة VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_گراف.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            اكسر;
        }
        حالة VFsmExpandType::AUTO: {
            arc_گنتی_With_Expansion = m_کنارے_گنتی;
            إذا (m_defaultVertexp) {
                UASSERT_OBJ(m_حالتاں.ناپ() >= m_defaultVertexp->باہرلا_کنارے().ناپ(),
                            m_گراف.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_گنتی_With_Expansion -= m_defaultVertexp->باہرلا_کنارے().ناپ();
                لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتاں) {
                    arc_گنتی_With_Expansion += static_cast<ناپ_t>(statep->باہرلا_کنارے().خالی_تھاواں())
                                             * m_defaultVertexp->باہرلا_کنارے().ناپ();
                }
            }
            إذا (m_resetVertexp) {
                UASSERT_OBJ(m_حالتاں.ناپ() >= m_resetVertexp->باہرلا_کنارے().ناپ(),
                            m_گراف.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_گنتی_With_Expansion += m_حالتاں.ناپ() - m_resetVertexp->باہرلا_کنارے().ناپ();
            }
            اكسر;
        }
        حالة VFsmExpandType::AUTO_EXPAND: {
            arc_گنتی_With_Expansion = m_حالتاں.ناپ() * m_حالتاں.ناپ() - m_حالتاں.ناپ();
            اكسر;
        }
        حالة VFsmExpandType::بھریا: {
            ثابت صحيح چوڑائی = m_گراف.sampleVarScopep()->چوڑائی();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            إذا (چوڑائی < 32) {
                ثابت uint64_t combinations_گنتی = static_cast<ناپ_t>(1) << چوڑائی;
                arc_گنتی_With_Expansion = combinations_گنتی * combinations_گنتی - combinations_گنتی;
            }
            اكسر;
        }
        }
        أرجع arc_گنتی_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ثابت std::string& label,
                                           ثابت std::string& fsm_ٹیگ = "") {
        AstCoverOtherDecl* ثابت declp
            = جديد AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_گراف.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_گراف.stateVarName(),
                                    "",
                                    label,
                                    fsm_ٹیگ};
        // We run after scoping, so this is declared for one instance only
        declp->فی_Instance(سچ);
        m_declsp = Ast_گرہ::جوڑو_اگلا(m_declsp, declp);
        أرجع declp;
    }

    Ast_گرہ_Stmt* buildCoverIncStmtp(ثابت std::string& fromStateLabel,
                                    ثابت std::string& منزل_State_Label,
                                    ثابت std::string& ادھار_Label = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ثابت declp = جديد AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_گراف.stateVarName() + "::" + fromStateLabel + "->" + منزل_State_Label,
            "",
            0,
            m_گراف.stateVarName(),
            fromStateLabel,
            منزل_State_Label,
            ادھار_Label};
        // We run after scoping, so this is declared for one instance only
        declp->فی_Instance(سچ);
        m_declsp = Ast_گرہ::جوڑو_اگلا(m_declsp, declp);
        أرجع جديد AstCoverInc{m_flp, declp};
    }
    Ast_گرہ_Stmt* buildExpandedTransitionHandler(ثابت std::string& منزل_State_Label,
                                                ثابت منطقي considerUndefs,
                                                ثابت منطقي expandOnlyWithTerminalVerticies,
                                                ثابت Fsm_State_راس* ثابت منزل_Statep = خالی_سوچک) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, خالی_سوچک};
        std::unordered_سیٹ<ثابت Fsm_State_راس*> handledSoureces;
        إذا (منزل_Statep) {
            لكل (ثابت V3_گراف_کنارا& کنارا : منزل_Statep->وچ_کنارے()) {
                إذا (ثابت Fsm_State_راس* ثابت fromp = کنارا.fromp()->cast<Fsm_State_راس>()) {
                    إذا (!handledSoureces.emplace(fromp).دوجا) استمر;
                    arcCasep->جوڑو_Itemsp(جديد Ast_Case_شے{
                        m_flp, makeConst(fromp->قدر().عدد().منزل_UQuad()),
                        (fromp != منزل_Statep) ? buildCoverIncStmtp(fromp->label(), منزل_State_Label)
                                            : خالی_سوچک});
                }
            }
        }
        لكل (ثابت Fsm_State_راس* ثابت fromp : m_حالتاں) {
            إذا (handledSoureces.لبھو(fromp) != handledSoureces.سرا()) استمر;
            arcCasep->جوڑو_Itemsp(جديد Ast_Case_شے{
                m_flp, makeConst(fromp->قدر().عدد().منزل_UQuad()),
                (fromp != منزل_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->باہرلا_کنارے().خالی_تھاواں()))
                    ? buildCoverIncStmtp(fromp->label(), منزل_State_Label, "artificial")
                    : خالی_سوچک});
        }
        إذا (!m_isComplete) {
            إذا (considerUndefs) {
                لكل (ثابت uint64_t from : m_undefined_حالتاں) {
                    ثابت std::string& label = quad_منزل_ثنائی_ہندسہ_Stirng_Label(from);
                    arcCasep->جوڑو_Itemsp(
                        جديد Ast_Case_شے{m_flp, makeConst(from),
                                        (label != منزل_State_Label)
                                            ? buildCoverIncStmtp(label, منزل_State_Label, "artificial")
                                            : خالی_سوچک});
                }
            } وإلا {
                arcCasep->جوڑو_Itemsp(جديد Ast_Case_شے{m_flp, خالی_سوچک, خالی_سوچک});
            }
        }
        أرجع arcCasep;
    }
    Ast_گرہ_Stmt* buildUnxpandedTransitionHandler(ثابت Fsm_State_راس* ثابت منزل_Statep) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, خالی_سوچک};
        لكل (ثابت V3_گراف_کنارا& کنارا : منزل_Statep->وچ_کنارے()) {
            ثابت Fsm_Arc_کنارا* ثابت arcp = کنارا.as<Fsm_Arc_کنارا>();
            UASSERT_OBJ(!arcp->isReset(), m_گراف.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_گراف.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ثابت Fsm_State_راس* ثابت fromp = arcp->fromp()->as<Fsm_State_راس>();
            إذا (fromp != منزل_Statep) {
                arcCasep->جوڑو_Itemsp(
                    جديد Ast_Case_شے{m_flp, makeConst(fromp->قدر().عدد().منزل_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), منزل_Statep->label())});
            }
        }
        arcCasep->جوڑو_Itemsp(جديد Ast_Case_شے{m_flp, خالی_سوچک, خالی_سوچک});
        أرجع arcCasep;
    }

عام:
    FsmCoverIncBuilder(ثابت Fsm_گراف& گراف, AstVarScope* ثابت prevVscp)
        : m_گراف{گراف}
        , m_prevVscp{prevVscp}
        , m_flp{گراف.fileline()}
        , m_modPrettyName{گراف.scopep()->modp()->prettyName()} {
        init_Existing_حالتاں();
        AstVarScope* ثابت sampleVscp = گراف.sampleVarScopep();
        ثابت VFsmExpandType expand
            = گراف.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? گراف.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ثابت ناپ_t arc_گنتی_With_Expansion = get_Arc_گنتی_With_Expansion(expand);
            إذا (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_گنتی_With_Expansion == 0ULL
                            || arc_گنتی_With_Expansion
                                   > static_cast<ناپ_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_گنتی_With_Expansion);
                أرجع;
            }
        }

        AstCase* ثابت arcCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, گراف.sampleVarScopep(), VAccess::READ}, خالی_سوچک};
        AstCase* ثابت initialStateCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, گراف.sampleVarScopep(), VAccess::READ}, خالی_سوچک};

        ثابت منطقي expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_سیٹ<ثابت Fsm_State_راس*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_سیٹ<ثابت Fsm_State_راس*>
            resetTargets;  // States that have edge incoming from reset vertex
        إذا (!expandAll) {
            إذا (m_defaultVertexp) {
                لكل (ثابت V3_گراف_کنارا& کنارا : m_defaultVertexp->باہرلا_کنارے()) {
                    defaultTargets.emplace(کنارا.چوٹی()->as<Fsm_State_راس>());
                }
            }
            إذا (m_resetVertexp) {
                لكل (ثابت V3_گراف_کنارا& کنارا : m_resetVertexp->باہرلا_کنارے()) {
                    resetTargets.emplace(کنارا.چوٹی()->as<Fsm_State_راس>());
                }
            }
        }
        ثابت منطقي is_بھریا_Expansion = expand == VFsmExpandType::بھریا;
        إذا (is_بھریا_Expansion) init_Undefined_حالتاں();
        لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتاں) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(statep->label());
            initialStateCasep->جوڑو_Itemsp(
                جديد Ast_Case_شے{m_flp, makeConst(statep->قدر().عدد().منزل_UQuad()),
                                جديد AstCoverInc{m_flp, declp, سچ}});
            ثابت منطقي expandAllOrReset
                = expandAll || (resetTargets.لبھو(statep) != resetTargets.سرا());
            ثابت منطقي defaultExpantion
                = !expandAllOrReset && (defaultTargets.لبھو(statep) != defaultTargets.سرا());
            arcCasep->جوڑو_Itemsp(جديد Ast_Case_شے{
                m_flp, makeConst(statep->قدر().عدد().منزل_UQuad()),
                Ast_گرہ::جوڑو_اگلا<Ast_گرہ_Stmt>(
                    جديد AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_بھریا_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        إذا (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            إذا (is_بھریا_Expansion) {
                لكل (ثابت uint64_t قدر : m_undefined_حالتاں) {
                    ثابت std::string& label = quad_منزل_ثنائی_ہندسہ_Stirng_Label(قدر);
                    AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(label);
                    initialStateCasep->جوڑو_Itemsp(جديد Ast_Case_شے{
                        m_flp, makeConst(قدر), جديد AstCoverInc{m_flp, declp, سچ}});
                    arcCasep->جوڑو_Itemsp(
                        جديد Ast_Case_شے{m_flp, makeConst(قدر),
                                        Ast_گرہ::جوڑو_اگلا<Ast_گرہ_Stmt>(
                                            جديد AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, سچ, خطأ))});
                }
            } وإلا {
                // No expansion
                arcCasep->جوڑو_Itemsp(جديد Ast_Case_شے{m_flp, خالی_سوچک, خالی_سوچک});
                initialStateCasep->جوڑو_Itemsp(جديد Ast_Case_شے{m_flp, خالی_سوچک, خالی_سوچک});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = افتراضي;
    Ast_گرہ_Cover_Decl* getCoverDeclsp() ثابت { أرجع m_declsp; }
    Ast_گرہ_Stmt* getCoverIncp() ثابت { أرجع m_coverIncp; }
    Ast_گرہ_Stmt* getInitialCoverIncp() ثابت { أرجع m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
صنف Fsm_ہیٹھلا_Visitor final {
    // STATE - across all visitors
    ثابت FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ساكن Ast_Sen_درخت* build_Sen_درخت(File_سطر* flp, ثابت std::سمتیہ<FsmSenDesc>& senses) {
        Ast_Sen_درخت* ثابت sentreep = جديد Ast_Sen_درخت{flp, خالی_سوچک};
        لكل (ثابت FsmSenDesc& sense : senses) {
            Ast_Sen_شے* ثابت senItemp
                = جديد Ast_Sen_شے{flp, VEdgeType{sense.کنارا_Type},
                                 جديد AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->جوڑو_Sensesp(senItemp);
        }
        أرجع sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    فراغ buildOne(ثابت Fsm_گراف& گراف) {
        UINFO(1, "buildOne lowering FSM " << گراف.stateVarName()
                                          << " vertices=" << گراف.vertices().ناپ() << سطر_انت);
        AstAlways* ثابت alwaysp = گراف.stateAlwaysp();
        AstScope* ثابت scopep = گراف.scopep();
        AstVarScope* ثابت stateVscp = گراف.stateVarScopep();
        AstVarScope* ثابت sampleVscp = گراف.sampleVarScopep();
        File_سطر* ثابت flp = گراف.fileline();
        Ast_گرہ_Module* ثابت modp = scopep->modp();
        AstVarScope* ثابت prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(سچ);
        sampleVscp->optimizeLifePost(سچ);
        prevVscp->optimizeLifePost(سچ);

        Ast_سرگرم* ثابت initActivep
            = جديد Ast_سرگرم{flp, "fsm-coverage-init",
                            جديد Ast_Sen_درخت{flp, جديد Ast_Sen_شے{flp, Ast_Sen_شے::Initial{}}}};
        initActivep->sen_درخت_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->جوڑو_Stmtsp(جديد AstInitialStatic{
            flp, جديد Ast_سونپو{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                               جديد AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->جوڑو_Blocksp(initActivep);

        AstAlwaysPost* ثابت covPostp = جديد AstAlwaysPost{flp};
        منطقي updatePrevAfterPost = خطأ;
        إذا (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(جديد Ast_سونپو{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        } وإلا {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = سچ;
            prevVscp->varp()->سیٹ_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ثابت FsmCoverIncBuilder coverIncBuilder{گراف, prevVscp};
            modp->جوڑو_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->جوڑو_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->جوڑو_Stmtsp(
                جديد AstIf{flp,
                          جديد AstNeq{flp, جديد AstVarRef{flp, prevVscp, VAccess::READ},
                                     جديد AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        إذا (updatePrevAfterPost) {
            covPostp->جوڑو_Stmtsp(جديد Ast_سونپو{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_درخت* ثابت sentreep = build_Sen_درخت(flp, گراف.senses());
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "fsm-coverage", sentreep};
        activep->sen_درخت_Storep(sentreep);
        scopep->جوڑو_Blocksp(activep);
        activep->جوڑو_Stmtsp(covPostp);
    }

عام:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    واضح Fsm_ہیٹھلا_Visitor(ثابت FsmState& state)
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
    إذا (dump_گراف_Level() >= 6) {
        ناپ_t اشاریہ = 0;
        لكل (ثابت DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_ٹیگ(اشاریہ++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_ہیٹھلا_Visitor ہیٹھلا{state}; }
    V3Global::dump_Check_Global_درخت("fsm-detect", 0, dump_درخت_Either_Level() >= 3);
}
