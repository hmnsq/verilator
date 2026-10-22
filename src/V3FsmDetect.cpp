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

#شامل "V3PchAstNoMT.h"

#شامل "V3FsmDetect.h"

#شامل "V3Ast.h"
#شامل "V3Control.h"
#شامل "V3Graph.h"
#شامل "V3SharedTmps.h"

#شامل <algorithm>
#شامل <cctype>
#شامل <map>
#شامل <memory>
#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

نامخلا {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
جماعت Fsm_State_قدر final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_کلید;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_نمبر m_عدد;  // Original folded value, preserving width for lowered comparisons

    ساکن string make_کلید(ثابت V3_نمبر& عدد) {
        V3_نمبر کلید_عدد = عدد;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        کلید_عدد.isSigned(جھوٹ);
        واپس cvt_مقصد_Str(کلید_عدد.چوڑائی()) + ":" + کلید_عدد.ascii(سچ, سچ);
    }

عوامی:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_قدر()
        : m_کلید{"1:1'h0"}
        , m_عدد{static_cast<Ast_گرہ*>(خالی_اشاریہ), 1, 0} {}
    صریح Fsm_State_قدر(ثابت V3_نمبر& عدد)
        : m_کلید{make_کلید(عدد)}
        , m_عدد{عدد} {}

    ثابت string& کلید() ثابت { واپس m_کلید; }
    ثابت V3_نمبر& عدد() ثابت { واپس m_عدد; }
    string ascii() ثابت { واپس m_عدد.ascii(سچ, سچ); }
    string warn_متن() ثابت {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        اگر (m_عدد.چوڑائی() <= 32) واپس cvt_مقصد_Str(m_عدد.مقصد_UInt());
        واپس ascii();
    }

    منطقی عامل==(ثابت Fsm_State_قدر& rhs) ثابت { واپس m_کلید == rhs.m_کلید; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
ساخت Fsm_State_قدر_Hash final {
    سائز_t عامل()(ثابت Fsm_State_قدر& قدر) ثابت {
        واپس std::hash<string>{}(قدر.کلید());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
ساخت FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en کنارہ_Type = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = خالی_اشاریہ;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
ساخت FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = خالی_اشاریہ;
    منطقی فعال_کم = جھوٹ;
};

جماعت FsmResetArcDesc final {
    Fsm_State_قدر m_مقصد_قدر;  // Encoded reset target state.
    Ast_گرہ* m_nodep = خالی_اشاریہ;  // Source node for warnings and emitted metadata.
    Ast_گرہ_اظہاریہ* m_valuep = خالی_اشاریہ;  // Expression that provided the reset value.

عوامی:
    FsmResetArcDesc() = طےشدہ;
    FsmResetArcDesc(Fsm_State_قدر مقصد_قدر, Ast_گرہ_تفویض* nodep)
        : m_مقصد_قدر{مقصد_قدر}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_قدر مقصد_قدر, Ast_گرہ* nodep, Ast_گرہ_اظہاریہ* valuep)
        : m_مقصد_قدر{مقصد_قدر}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_قدر مقصد_قدر() ثابت { واپس m_مقصد_قدر; }
    Ast_گرہ* nodep() ثابت { واپس m_nodep; }
    Ast_گرہ_اظہاریہ* valuep() ثابت { واپس m_valuep; }
};

ساخت FsmWrapperRoles final {
    string d_پورٹ;
    string q_پورٹ;
    string clk_پورٹ;
    string rst_پورٹ;
    string rstValParam;
    منطقی has_Rst_فعال_کم = جھوٹ;
    منطقی rst_فعال_کم = جھوٹ;
};

ساکن منطقی fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ثابت string& پورٹ_Name,
                                                  منطقی& فعال_کم) {
    منطقی matched = جھوٹ;
    cellp->modp()->foreach([&](Ast_Sen_شے* itemp) {
        Ast_گرہ_Var_Ref* ثابت vrefp = itemp->varrefp();
        اگر (!vrefp) واپس;
        اگر (vrefp->varp()->name() != پورٹ_Name) واپس;
        فعال_کم = itemp->کنارہ_Type() == VEdgeType::ET_NEGEDGE;
        matched = سچ;
    });
    واپس matched;
}

ساکن ثابت V3Control::Fsm_رجسٹر_Wrapper* fsm_رجسٹر_Wrapper_Desc(AstCell* cellp) {
    Ast_گرہ_Module* ثابت modp = cellp->modp();
    ثابت string origName = modp->origName();
    اگر (ثابت V3Control::Fsm_رجسٹر_Wrapper* ثابت descp
        = V3Control::get_Fsm_رجسٹر_Wrapper(origName)) {
        واپس descp;
    }
    واپس V3Control::get_Fsm_رجسٹر_Wrapper(modp->prettyDehashOrigOrName());
}

ساکن FsmWrapperRoles rolesFromDesc(ثابت V3Control::Fsm_رجسٹر_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_پورٹ = desc.d;
    roles.q_پورٹ = desc.q;
    roles.clk_پورٹ = desc.clock;
    roles.rst_پورٹ = desc.reset;
    roles.rstValParam = desc.reset_قدر;
    واپس roles;
}

جماعت Fsm_رجسٹر_Candidate final {
    AstScope* m_scopep = خالی_اشاریہ;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = خالی_اشاریہ;  // Register process that commits the state.
    AstVarScope* m_stateVscp = خالی_اشاریہ;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = خالی_اشاریہ;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_اگلا_Vscp = خالی_اشاریہ;  // Next-state variable or same state var for 1-block FSMs.
    std::سمتیہ<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::سمتیہ<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    منطقی m_hasResetCond = جھوٹ;  // Whether the FSM had a modeled reset predicate.
    منطقی m_inclCond = جھوٹ;  // Whether conditional/default arcs are kept explicitly.
    File_سطر* m_flp = خالی_اشاریہ;  // Representative source location.

عوامی:
    AstScope* scopep() ثابت { واپس m_scopep; }
    خالی scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ثابت { واپس m_alwaysp; }
    خالی alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ثابت { واپس m_stateVscp; }
    خالی stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ثابت { واپس m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    خالی sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* اگلا_Vscp() ثابت { واپس m_اگلا_Vscp; }
    خالی اگلا_Vscp(AstVarScope* vscp) { m_اگلا_Vscp = vscp; }
    ثابت std::سمتیہ<FsmSenDesc>& senses() ثابت { واپس m_senses; }
    std::سمتیہ<FsmSenDesc>& senses() { واپس m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { واپس m_resetCond; }
    FsmResetCondDesc& resetCond() { واپس m_resetCond; }
    ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs() ثابت { واپس m_resetArcs; }
    std::سمتیہ<FsmResetArcDesc>& resetArcs() { واپس m_resetArcs; }
    منطقی hasResetCond() ثابت { واپس m_hasResetCond; }
    خالی hasResetCond(منطقی flag) { m_hasResetCond = flag; }
    منطقی inclCond() ثابت { واپس m_inclCond; }
    خالی inclCond(منطقی flag) { m_inclCond = flag; }
    File_سطر* fileline() ثابت { واپس m_flp; }
    خالی fileline(File_سطر* flp) { m_flp = flp; }
};

جماعت FsmComboAlways final {
    AstScope* ثابت m_scopep = خالی_اشاریہ;  // Owning scope for the combinational process.
    AstAlways* ثابت m_alwaysp = خالی_اشاریہ;  // Candidate transition process.

عوامی:
    FsmComboAlways() = طےشدہ;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ثابت { واپس m_scopep; }
    AstAlways* alwaysp() ثابت { واپس m_alwaysp; }
};

جماعت Fsm_گراف;

جماعت Fsm_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Fsm_راس, V3_گراف_راس)

عوامی:
    شمار جماعت قسم_2 : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

نجی:
    قسم_2 m_قسم;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_قدر m_قدر;  // Encoded state value for real state vertices.

محفوظ:
    Fsm_راس(V3_گراف* graphp, قسم_2 قسم, string label, Fsm_State_قدر قدر) VL_MT_DISABLED
        : V3_گراف_راس{graphp},
          m_قسم{قسم},
          m_label{label},
          m_قدر{قدر} {}
    ~Fsm_راس() override = طےشدہ;

عوامی:
    قسم_2 قسم() ثابت { واپس m_قسم; }
    منطقی isState() ثابت { واپس m_قسم == قسم_2::STATE; }
    منطقی isResetAny() ثابت { واپس m_قسم == قسم_2::RESET_ANY; }
    منطقی isDefaultAny() ثابت { واپس m_قسم == قسم_2::DEFAULT_ANY; }
    ثابت string& label() ثابت { واپس m_label; }
    ثابت Fsm_State_قدر& قدر() ثابت { واپس m_قدر; }

    string name() ثابت override VL_MT_محفوظ { واپس m_label + "=" + m_قدر.ascii(); }
};

جماعت Fsm_State_راس final : عوامی Fsm_راس {
    VL_RTTI_IMPL(Fsm_State_راس, Fsm_راس)

عوامی:
    Fsm_State_راس(V3_گراف* graphp, string label, Fsm_State_قدر قدر) VL_MT_DISABLED
        : Fsm_راس{graphp, قسم_2::STATE, label, قدر} {}
    ~Fsm_State_راس() override = طےشدہ;

    string dotColor() ثابت override { واپس "lightblue"; }
    string dotShape() ثابت override { واپس "ellipse"; }
};

جماعت Fsm_Pseudo_راس final : عوامی Fsm_راس {
    VL_RTTI_IMPL(Fsm_Pseudo_راس, Fsm_راس)

عوامی:
    Fsm_Pseudo_راس(V3_گراف* graphp, قسم_2 قسم, string label) VL_MT_DISABLED
        : Fsm_راس{graphp, قسم, label, Fsm_State_قدر{}} {}
    ~Fsm_Pseudo_راس() override = طےشدہ;

    string name() ثابت override VL_MT_محفوظ { واپس label(); }
    string dotColor() ثابت override { واپس isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ثابت override { واپس "diamond"; }
};

جماعت Fsm_Arc_کنارہ final : عوامی V3_گراف_کنارہ {
    VL_RTTI_IMPL(Fsm_Arc_کنارہ, V3_گراف_کنارہ)
    منطقی m_isReset = جھوٹ;  // Arc originates from the synthetic reset source.
    منطقی m_isCond = جھوٹ;  // Arc came from a conditional next-state split.
    منطقی m_isDefault = جھوٹ;  // Arc represents a case default source.
    File_سطر* m_flp = خالی_اشاریہ;  // Source location for emitted coverage metadata.

عوامی:
    Fsm_Arc_کنارہ(V3_گراف* graphp, Fsm_راس* fromp, Fsm_State_راس* چوٹی, منطقی isReset, منطقی isCond,
               منطقی isDefault, File_سطر* flp) VL_MT_DISABLED : V3_گراف_کنارہ{graphp, fromp, چوٹی, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~Fsm_Arc_کنارہ() override = طےشدہ;

    منطقی isReset() ثابت { واپس m_isReset; }
    منطقی isCond() ثابت { واپس m_isCond; }
    منطقی isDefault() ثابت { واپس m_isDefault; }
    File_سطر* fileline() ثابت { واپس m_flp; }

    string dotLabel() ثابت override {
        اگر (m_isReset) واپس "reset";
        اگر (m_isDefault) واپس "default";
        اگر (m_isCond) واپس "cond";
        واپس "";
    }
    string dotColor() ثابت override {
        اگر (m_isReset) واپس "darkgreen";
        اگر (m_isDefault) واپس "orange";
        اگر (m_isCond) واپس "blue";
        واپس "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
جماعت Fsm_گراف final : عوامی V3_گراف {
    AstScope* m_scopep = خالی_اشاریہ;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = خالی_اشاریہ;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_اندرونی_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = خالی_اشاریہ;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = خالی_اشاریہ;  // Scoped variable sampled by coverage logic.
    std::سمتیہ<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    منطقی m_hasResetCond = جھوٹ;  // Whether the detected FSM had a reset branch.
    منطقی m_inclCond = جھوٹ;  // Whether conditional arcs should be kept explicitly.
    File_سطر* m_flp = خالی_اشاریہ;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_قدر, Fsm_State_راس*, Fsm_State_قدر_Hash>
        m_stateVertices;  // Value to state map.
    Fsm_Pseudo_راس* m_resetVertexp = خالی_اشاریہ;  // Synthetic ANY source for reset arcs.
    Fsm_Pseudo_راس* m_defaultVertexp = خالی_اشاریہ;  // Synthetic default source for case defaults.

عوامی:
    Fsm_گراف() VL_MT_DISABLED
        : m_resetVertexp{نیا Fsm_Pseudo_راس{یہ, Fsm_راس::قسم_2::RESET_ANY, "ANY"}},
          m_defaultVertexp{نیا Fsm_Pseudo_راس{یہ, Fsm_راس::قسم_2::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ثابت { واپس m_scopep; }
    خالی scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ثابت { واپس m_stateAlwaysp; }
    خالی stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ثابت string& stateVarName() ثابت { واپس m_stateVarName; }
    خالی stateVarName(ثابت string& name) { m_stateVarName = name; }
    ثابت string& state_Var_اندرونی_Name() ثابت { واپس m_state_Var_اندرونی_Name; }
    خالی state_Var_اندرونی_Name(ثابت string& name) { m_state_Var_اندرونی_Name = name; }
    AstVarScope* stateVarScopep() ثابت { واپس m_stateVarScopep; }
    خالی stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ثابت {
        واپس m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    خالی sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ثابت std::سمتیہ<FsmSenDesc>& senses() ثابت { واپس m_senses; }
    std::سمتیہ<FsmSenDesc>& senses() { واپس m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { واپس m_resetCond; }
    FsmResetCondDesc& resetCond() { واپس m_resetCond; }
    منطقی hasResetCond() ثابت { واپس m_hasResetCond; }
    خالی hasResetCond(منطقی flag) { m_hasResetCond = flag; }
    منطقی inclCond() ثابت { واپس m_inclCond; }
    خالی inclCond(منطقی flag) { m_inclCond = flag; }
    File_سطر* fileline() ثابت { واپس m_flp; }
    خالی fileline(File_سطر* flp) { m_flp = flp; }

    Fsm_State_راس* جوڑو_State_راس(string label, Fsm_State_قدر قدر) VL_MT_DISABLED {
        Fsm_State_راس* ثابت vertexp = نیا Fsm_State_راس{یہ, label, قدر};
        m_stateVertices.emplace(قدر, vertexp);
        واپس vertexp;
    }
    Fsm_Pseudo_راس* reset_Any_راس() VL_MT_DISABLED { واپس m_resetVertexp; }
    Fsm_Pseudo_راس* default_Any_راس() VL_MT_DISABLED { واپس m_defaultVertexp; }
    Fsm_Arc_کنارہ* جوڑو_Arc(Fsm_State_قدر from_قدر, Fsm_State_قدر مقصد_قدر, منطقی isReset, منطقی isCond,
                       منطقی isDefault, File_سطر* flp) VL_MT_DISABLED {
        Fsm_State_راس* ثابت چوٹی = m_stateVertices.at(مقصد_قدر);
        Fsm_راس* fromp = خالی_اشاریہ;
        اگر (isReset) {
            fromp = reset_Any_راس();
        } ورنہ اگر (isDefault) {
            fromp = default_Any_راس();
        } ورنہ {
            fromp = m_stateVertices.at(from_قدر);
        }
        واپس نیا Fsm_Arc_کنارہ{یہ, fromp, چوٹی, isReset, isCond, isDefault, flp};
    }

    string name() ثابت VL_MT_محفوظ {
        واپس "FSM "
               + (m_stateVarName.خالی_خانے() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_لیبل(سائز_t اشاریہ) ثابت {
        string لیبل = state_Var_اندرونی_Name();
        برائے (حرف& ch : لیبل) {
            اگر (!std::حرف_یا_ہندسہ(static_cast<غیرعلامتی حرف>(ch))) ch = '_';
        }
        واپس "fsm_" + cvt_مقصد_Str(اشاریہ) + "_" + لیبل;
    }
};

ساخت DetectedFsm final {
    std::unique_ptr<Fsm_گراف> graphp;  // Extracted graph for one detected FSM candidate.
};

ساخت FsmCaseCandidate final {
    Ast_گرہ* warnNodep = خالی_اشاریہ;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = خالی_اشاریہ;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
ساخت FsmStateComparison final {
    AstVarScope* stateVscp = خالی_اشاریہ;  // Compared state variable
    Ast_گرہ_اظہاریہ* valuep = خالی_اشاریہ;  // Compared constant value expression
    Fsm_State_قدر قدر;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
ساخت FsmIfBranch final {
    AstIf* ifp = خالی_اشاریہ;  // Source if/else-if node
    Ast_گرہ* stmtsp = خالی_اشاریہ;  // Branch body
    Ast_گرہ_اظہاریہ* valuep = خالی_اشاریہ;  // Source state value expression
    Fsm_State_قدر from_قدر;  // Encoded source state value
    منطقی has_چوٹی_Guard = جھوٹ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
ساخت FsmIfChainCandidate final {
    AstIf* ifp = خالی_اشاریہ;  // Top-level if-chain node
    AstVarScope* compareVscp = خالی_اشاریہ;  // Variable used by every state comparison
    std::سمتیہ<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_گرہ* defaultStmtsp = خالی_اشاریہ;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
استعمال FsmAliasMap = std::unordered_map<ثابت AstVarScope*, FsmStateComparison>;
استعمال Fsm_Cell_پورٹ_Map = std::unordered_map<string, AstVarScope*>;
استعمال Fsm_Cell_پورٹ_Alias_Map = std::unordered_map<ثابت AstCell*, Fsm_Cell_پورٹ_Map>;

ساخت StateConstLabel final {
    string متن;
    منطقی fromParam = جھوٹ;
    سائز_t state_اشاریہ = 0;
};

ساخت FsmStateSpace final {
    std::سمتیہ<std::pair<string, Fsm_State_قدر>> حالتیں;  // User label and encoded value
    std::unordered_map<Fsm_State_قدر, StateConstLabel, Fsm_State_قدر_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = خالی_اشاریہ;  // Tracked FSM state variable
    منطقی enumBacked = جھوٹ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
جماعت FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::سمتیہ<DetectedFsm> m_fsms;
    std::map<ثابت AstVarScope*, سائز_t> m_fsm_اشاریہ;

عوامی:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ثابت std::map<ثابت AstVarScope*, سائز_t>::const_iterator it = m_fsm_اشاریہ.ڈھونڈو(stateVscp);
        اگر (it != m_fsm_اشاریہ.انتہا()) واپس m_fsms.at(it->دوسرا);
        ثابت سائز_t اشاریہ = m_fsms.سائز();
        m_fsm_اشاریہ.emplace(stateVscp, اشاریہ);
        m_fsms.emplace_عقب();
        واپس m_fsms.عقب();
    }
    ثابت std::سمتیہ<DetectedFsm>& fsms() ثابت { واپس m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
جماعت FsmDetectVisitor final : عوامی VNVisitor {
    // STATE - for current visit position (use VL_RESTORER)
    FsmState& m_state;
    AstScope* m_scopep = خالی_اشاریہ;
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
    ساکن Ast_گرہ_DType* unwrapEnumCandidate(Ast_گرہ_DType* dtypep) {
        واپس dtypep->skip_Ref_مقصد_Enump();
    }

    ساکن string candidateConflictContext(Ast_گرہ* laterNodep,
                                           ثابت FsmCaseCandidate& پہلا_Cand) {
        واپس '\n' + laterNodep->warnContextPrimary() + پہلا_Cand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + پہلا_Cand.stateVscp->prettyNameQ() + '\n'
               + پہلا_Cand.warnNodep->warnContextSecondary();
    }

    ساکن منطقی rejectFsmWrapperCell(AstCell* cellp, ثابت string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        واپس جھوٹ;
    }

    ساکن منطقی simple_Param_State_قدر(AstCell* cellp, ثابت string& name, Fsm_State_قدر& قدر,
                                      Ast_گرہ_اظہاریہ*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = خالی_اشاریہ;
        برائے (Ast_گرہ* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت varp = VN_CAST(stmtp, Var);
            اگر (!varp || !varp->isParam() || varp->name() != name) جاری;
            valuepr = VN_AS(varp->valuep(), گرہ_اظہاریہ);
            واپس const_قدر_حالت(valuepr, قدر) == Const_قدر_حالت::OK;
        }
        واپس جھوٹ;
    }

    ساکن منطقی بچہ_پورٹ_اندر_Scope(AstVarScope* vscp, AstScope* والد_Scopep, AstCell*& cellpr) {
        اگر (!vscp->varp()->isIO()) واپس جھوٹ;
        AstScope* ثابت scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        اگر (scopep->aboveScopep() != والد_Scopep) واپس جھوٹ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        واپس سچ;
    }

    ساکن AstVarScope* simple_تفویض_Var_Scope(Ast_گرہ_اظہاریہ* exprp) {
        AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef);
        واپس vrefp ? vrefp->varScopep() : خالی_اشاریہ;
    }

    خالی جوڑو_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_پورٹ_Aliases.emplace(cellp, Fsm_Cell_پورٹ_Map{});
        m_cell_پورٹ_بچہ_Aliases.emplace(cellp, Fsm_Cell_پورٹ_Map{});
        ثابت std::pair<AstScope*, AstCell*> شے_3{scopep, cellp};
        اگر (std::ڈھونڈو(m_wrapperCells.cbegin(), m_wrapperCells.cend(), شے_3)
            != m_wrapperCells.cend()) {
            واپس;
        }
        m_wrapperCells.emplace_عقب(شے_3);
    }

    خالی collect_Cell_پورٹ_Alias(Ast_تفویض_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ثابت lhsVscp = simple_تفویض_Var_Scope(nodep->lhsp());
        AstVarScope* ثابت rhsVscp = simple_تفویض_Var_Scope(nodep->rhsp());
        اگر (!lhsVscp || !rhsVscp) واپس;
        AstCell* cellp = خالی_اشاریہ;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        اگر (بچہ_پورٹ_اندر_Scope(lhsVscp, m_scopep, cellp)) {
            اگر (!fsm_رجسٹر_Wrapper_Desc(cellp)) واپس;
            UASSERT_OBJ(lhsVscp->varp()->is_داخلہ(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_پورٹ_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_پورٹ_بچہ_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            جوڑو_Wrapper_Cell(m_scopep, cellp);
        } ورنہ اگر (بچہ_پورٹ_اندر_Scope(rhsVscp, m_scopep, cellp)) {
            اگر (!fsm_رجسٹر_Wrapper_Desc(cellp)) واپس;
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
        ثابت Fsm_Cell_پورٹ_Map::const_iterator پورٹ_It = ports.ڈھونڈو(پورٹ_Name);
        واپس پورٹ_It == ports.انتہا() ? خالی_اشاریہ : پورٹ_It->دوسرا;
    }

    AstVarScope* بچہ_Role_Var_Scope(AstCell* cellp, ثابت string& پورٹ_Name) ثابت {
        ثابت Fsm_Cell_پورٹ_Map& ports = m_cell_پورٹ_بچہ_Aliases.at(cellp);
        ثابت Fsm_Cell_پورٹ_Map::const_iterator پورٹ_It = ports.ڈھونڈو(پورٹ_Name);
        واپس پورٹ_It == ports.انتہا() ? خالی_اشاریہ : پورٹ_It->دوسرا;
    }

    جماعت رجسٹر_Always_Analyzer final {
        AstScope* ثابت m_scopep;  // Scope containing the always block(s) being analyzed

    عوامی:
        صریح رجسٹر_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::سمتیہ<std::pair<AstCase*, Ast_گرہ_اظہاریہ*>>
        one_بلاک_Candidates(AstAlways* alwaysp) ثابت {
            std::سمتیہ<std::pair<AstCase*, Ast_گرہ_اظہاریہ*>> candidates;
            Ast_گرہ* ثابت stmtsp = alwaysp->stmtsp();
            اگر (AstIf* ثابت پہلا_Ifp = VN_CAST(stmtsp, If)) {
                اگر (AstCase* ثابت casep = VN_CAST(پہلا_Ifp->elsesp(), Case)) {
                    candidates.emplace_عقب(casep,
                                            FsmDetectVisitor::isSimpleResetCond(پہلا_Ifp->condp())
                                                ? پہلا_Ifp->condp()
                                                : خالی_اشاریہ);
                }
            }
            برائے (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                اگر (AstCase* ثابت casep = VN_CAST(nodep, Case))
                    candidates.emplace_عقب(casep, خالی_اشاریہ);
            }
            واپس candidates;
        }

        std::سمتیہ<std::pair<AstIf*, Ast_گرہ_اظہاریہ*>>
        one_بلاک_If_Candidates(AstAlways* alwaysp) ثابت {
            std::سمتیہ<std::pair<AstIf*, Ast_گرہ_اظہاریہ*>> candidates;
            Ast_گرہ* ثابت stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            اگر (AstIf* ثابت پہلا_Ifp = VN_CAST(stmtsp, If)) {
                اگر (AstIf* ثابت chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(پہلا_Ifp->elsesp()), If)) {
                    candidates.emplace_عقب(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(پہلا_Ifp->condp())
                                                ? پہلا_Ifp->condp()
                                                : خالی_اشاریہ);
                }
            }
            برائے (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                اگر (AstIf* ثابت ifp = VN_CAST(nodep, If)) candidates.emplace_عقب(ifp, خالی_اشاریہ);
            }
            واپس candidates;
        }

        منطقی مطابقت_رجسٹر_Candidate(AstAlways* alwaysp, Fsm_رجسٹر_Candidate& cand) ثابت {
            واپس FsmDetectVisitor::مطابقت_رجسٹر_Always(alwaysp, m_scopep, cand);
        }

        خالی build_One_بلاک_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_گرہ_اظہاریہ* resetCondp,
                                    Fsm_رجسٹر_Candidate& reg) ثابت {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.اگلا_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_درخت(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != خالی_اشاریہ);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ثابت پہلا_Ifp = VN_CAST(alwaysp->stmtsp(), If);
            اگر (پہلا_Ifp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = خالی_اشاریہ;
                ثابت Reset_تفویض_حالت reset_حالت = FsmDetectVisitor::collectConstStateAssigns(
                    پہلا_Ifp->thensp(), resetStateVscp, reg.resetArcs());
                اگر (reset_حالت == Reset_تفویض_حالت::خالی_خانے_3) {
                    reg.resetArcs().clear();
                } ورنہ اگر (reset_حالت == Reset_تفویض_حالت::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_قدر reset_قدر;
                    Ast_گرہ* ثابت thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(پہلا_Ifp->thensp());
                    UASSERT_OBJ(thenNodep, پہلا_Ifp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_گرہ_تفویض* ثابت resetAssp = FsmDetectVisitor::براہ_راست_Const_State_تفویض_گرہ(
                        thenNodep, resetStateVscp, reset_قدر);
                    اگر (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_عقب(reset_قدر, resetAssp);
                    }
                } ورنہ اگر (reset_حالت == Reset_تفویض_حالت::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    منطقی مطابقت_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_رجسٹر_Candidate& cand) ثابت {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_رجسٹر_Wrapper_Desc(cellp));

        AstVarScope* ثابت اگلا_Vscp = roleVarScope(cellp, roles.d_پورٹ);
        AstVarScope* ثابت stateVscp = roleVarScope(cellp, roles.q_پورٹ);
        اگر (!اگلا_Vscp || !stateVscp) {
            واپس rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ثابت clkVscp = roleVarScope(cellp, roles.clk_پورٹ);
        اگر (!clkVscp) {
            واپس rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.کنارہ_Type = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ڈالو_عقب(clkSense);

        AstVarScope* resetVscp = خالی_اشاریہ;
        اگر (!roles.rst_پورٹ.خالی_خانے()) resetVscp = roleVarScope(cellp, roles.rst_پورٹ);
        اگر (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            منطقی inferred_فعال_کم = جھوٹ;
            اگر (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_پورٹ, inferred_فعال_کم)) {
                roles.has_Rst_فعال_کم = سچ;
                roles.rst_فعال_کم = inferred_فعال_کم;
            }
        }

        Ast_گرہ_اظہاریہ* resetValuep = خالی_اشاریہ;
        Fsm_State_قدر reset_قدر;
        ثابت منطقی has_Reset_قدر
            = !roles.rstValParam.خالی_خانے()
              && simple_Param_State_قدر(cellp, roles.rstValParam, reset_قدر, resetValuep);
        اگر (resetVscp && roles.has_Rst_فعال_کم && has_Reset_قدر) {
            FsmSenDesc rstSense;
            rstSense.کنارہ_Type = roles.rst_فعال_کم ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ڈالو_عقب(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().فعال_کم = roles.rst_فعال_کم;
            cand.hasResetCond(سچ);
            cand.resetArcs().emplace_عقب(reset_قدر, cellp, resetValuep);
        } ورنہ اگر (!roles.rst_پورٹ.خالی_خانے() || !roles.rstValParam.خالی_خانے()) {
            string reason;
            اگر (roles.rst_پورٹ.خالی_خانے()) {
                reason = "reset port is not configured";
            } ورنہ اگر (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } ورنہ اگر (!roles.has_Rst_فعال_کم) {
                reason = "reset polarity could not be inferred from the wrapper";
            } ورنہ اگر (roles.rstValParam.خالی_خانے()) {
                reason = "reset_value parameter is not configured";
            } ورنہ {
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
        cand.alwaysp(خالی_اشاریہ);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(بچہ_Role_Var_Scope(cellp, roles.q_پورٹ));
        cand.اگلا_Vscp(اگلا_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        واپس سچ;
    }

    جماعت ComboAlwaysAnalyzer final {
    عوامی:
        ساخت Combo_مطابقت final {
            ثابت Fsm_رجسٹر_Candidate* matchedp = خالی_اشاریہ;
            Ast_گرہ* warnNodep = خالی_اشاریہ;
        };

    نجی:
        ثابت std::سمتیہ<Fsm_رجسٹر_Candidate>& m_رجسٹر_Candidates;

    عوامی:
        صریح ComboAlwaysAnalyzer(ثابت std::سمتیہ<Fsm_رجسٹر_Candidate>& رجسٹر_Candidates)
            : m_رجسٹر_Candidates{رجسٹر_Candidates} {}

        Combo_مطابقت مطابقت_Case(Ast_گرہ* stmtsp, AstCase* casep) ثابت {
            Combo_مطابقت مطابقت;
            AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            اگر (!selp) واپس مطابقت;
            برائے (ثابت Fsm_رجسٹر_Candidate& reg : m_رجسٹر_Candidates) {
                اگر (selp->varScopep() == reg.اگلا_Vscp()) {
                    اگر (!FsmDetectVisitor::has_Canonical_اگلا_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.اگلا_Vscp())) {
                        جاری;
                    }
                } ورنہ اگر (selp->varScopep() != reg.stateVscp()) {
                    جاری;
                }
                Ast_گرہ* ثابت warnNodep = FsmDetectVisitor::case_Supported_Transition_گرہ(
                    casep, reg.اگلا_Vscp(), reg.inclCond());
                اگر (!warnNodep) جاری;
                مطابقت.matchedp = &reg;
                مطابقت.warnNodep = warnNodep;
            }
            واپس مطابقت;
        }

        Combo_مطابقت مطابقت_If_Chain(Ast_گرہ* stmtsp, ثابت FsmIfChainCandidate& chain) ثابت {
            Combo_مطابقت مطابقت;
            برائے (ثابت Fsm_رجسٹر_Candidate& reg : m_رجسٹر_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                اگر (chain.compareVscp == reg.اگلا_Vscp()) {
                    اگر (!FsmDetectVisitor::has_Canonical_اگلا_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.اگلا_Vscp())) {
                        جاری;
                    }
                } ورنہ اگر (chain.compareVscp != reg.stateVscp()) {
                    جاری;
                }
                Ast_گرہ* ثابت warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_گرہ(chain, reg.اگلا_Vscp());
                اگر (!warnNodep) جاری;
                مطابقت.matchedp = &reg;
                مطابقت.warnNodep = warnNodep;
            }
            واپس مطابقت;
        }

        منطقی shouldWarnUnsupported(Ast_گرہ* stmtsp, AstCase* casep) ثابت {
            ثابت AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            اگر (!selp) واپس جھوٹ;

            واپس std::any_کا(
                m_رجسٹر_Candidates.cbegin(), m_رجسٹر_Candidates.cend(),
                [&](ثابت Fsm_رجسٹر_Candidate& reg) -> منطقی {
                    ثابت منطقی matches_اگلا = selp->varScopep() == reg.اگلا_Vscp();
                    ثابت منطقی matchesState = selp->varScopep() == reg.stateVscp();

                    اگر (!matches_اگلا && !matchesState) واپس جھوٹ;
                    اگر (matches_اگلا
                        && !FsmDetectVisitor::has_Canonical_اگلا_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.اگلا_Vscp())) {
                        واپس جھوٹ;
                    }
                    واپس FsmDetectVisitor::case_Supported_Transition_گرہ(casep, reg.اگلا_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ساکن منطقی isSimpleResetCond(Ast_گرہ_اظہاریہ* condp) { واپس VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ساکن FsmResetCondDesc describeResetCond(Ast_گرہ_اظہاریہ* condp) {
        FsmResetCondDesc desc;
        اگر (AstVarRef* ثابت vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        واپس desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ساکن std::سمتیہ<FsmSenDesc> describe_Sen_درخت(Ast_Sen_درخت* sentreep) {
        std::سمتیہ<FsmSenDesc> senses;
        برائے (Ast_Sen_شے* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), Sen_شے)) {
            Ast_گرہ_Var_Ref* ثابت vrefp = itemp->varrefp();
            اگر (!vrefp) جاری;
            FsmSenDesc desc;
            desc.کنارہ_Type = itemp->کنارہ_Type().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ڈالو_عقب(desc);
        }
        واپس senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ساکن منطقی isIgnorableStmt(Ast_گرہ* nodep) { واپس VN_IS(nodep, CoverInc); }

    ساکن Ast_گرہ* skipLeadingIgnorableStmt(Ast_گرہ* nodep) {
        جب (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        واپس nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ساکن Ast_گرہ* singleMeaningfulStmt(Ast_گرہ* stmtp) {
        Ast_گرہ* resultp = خالی_اشاریہ;
        برائے (Ast_گرہ* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            اگر (isIgnorableStmt(nodep)) جاری;
            اگر (resultp) واپس خالی_اشاریہ;
            resultp = nodep;
        }
        واپس resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ساکن Ast_گرہ* singleMeaningfulBranch(Ast_گرہ* branchp) {
        اگر (!branchp) واپس خالی_اشاریہ;
        واپس branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ساکن منطقی isPlainComboSentree(ثابت Ast_Sen_درخت* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        برائے (ثابت Ast_Sen_شے* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), Sen_شے)) {
            اگر (senp->کنارہ_Type() == VEdgeType::ET_بدلا) جاری;
            واپس جھوٹ;
        }
        واپس سچ;
    }

    خالی warnUnsupportedComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_رجسٹر_Candidates};
        Ast_گرہ* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        منطقی warned = جھوٹ;
        برائے (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            اگر (!casep) جاری;
            اگر (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = سچ;
            }
            اگر (warned) توڑو;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ساکن Ast_گرہ_تفویض* براہ_راست_State_تفویض(Ast_گرہ* stmtp, AstVarScope* stateVscp) {
        Ast_گرہ* ثابت nodep = singleMeaningfulBranch(stmtp);
        اگر (!nodep) واپس خالی_اشاریہ;
        Ast_گرہ_تفویض* ثابت assp = VN_CAST(nodep, گرہ_تفویض);
        اگر (!assp) واپس خالی_اشاریہ;
        AstVarRef* ثابت vrefp = VN_CAST(assp->lhsp(), VarRef);
        اگر (!vrefp || vrefp->varScopep() != stateVscp) واپس خالی_اشاریہ;
        واپس assp;
    }

    ساکن Ast_گرہ_تفویض* گرہ_State_Var_تفویض(Ast_گرہ* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_گرہ_تفویض* ثابت assp = VN_CAST(nodep, گرہ_تفویض);
        اگر (!assp) واپس خالی_اشاریہ;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنیاد_Fromp(سچ), VarRef);
        AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
        اگر (!rhsp || !lhsp) واپس خالی_اشاریہ;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        واپس assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ساکن Ast_گرہ_تفویض* براہ_راست_Cond_State_Var_تفویض(Ast_گرہ* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_گرہ_اظہاریہ*& condp,
                                                   Fsm_State_قدر& reset_قدر) {
        Ast_گرہ_تفویض* ثابت assp = VN_CAST(nodep, گرہ_تفویض);
        اگر (!assp) واپس خالی_اشاریہ;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنیاد_Fromp(سچ), VarRef);
        AstCond* ثابت rhsp = VN_CAST(assp->rhsp(), Cond);
        اگر (!rhsp || !lhsp) واپس خالی_اشاریہ;
        AstVarRef* ثابت elsep = VN_CAST(rhsp->elsep(), VarRef);
        اگر (!elsep) واپس خالی_اشاریہ;
        اگر (const_قدر_حالت(rhsp->thenp(), reset_قدر) != Const_قدر_حالت::OK) واپس خالی_اشاریہ;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        واپس assp;
    }

    ساکن Ast_گرہ_تفویض* براہ_راست_Const_State_تفویض_گرہ(Ast_گرہ* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_قدر& قدر) {
        Ast_گرہ_تفویض* ثابت assp = VN_CAST(nodep, گرہ_تفویض);
        اگر (!assp) واپس خالی_اشاریہ;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنیاد_Fromp(سچ), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        اگر (const_قدر_حالت(assp->rhsp(), قدر) != Const_قدر_حالت::OK) واپس خالی_اشاریہ;
        stateVscp = lhsp->varScopep();
        واپس assp;
    }

    شمار جماعت Reset_تفویض_حالت : uint8_t {
        خالی_خانے_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ساکن Reset_تفویض_حالت collectConstStateAssigns(Ast_گرہ* stmtp, AstVarScope*& stateVscp,
                                                      std::سمتیہ<FsmResetArcDesc>& resetArcs) {
        Ast_گرہ* nodep = skipLeadingIgnorableStmt(stmtp);
        اگر (!nodep) واپس Reset_تفویض_حالت::خالی_خانے_3;
        برائے (;; nodep = nodep->nextp()) {
            AstVarScope* تفویض_State_Vscp = خالی_اشاریہ;
            Fsm_State_قدر قدر;
            Ast_گرہ_تفویض* ثابت assp = براہ_راست_Const_State_تفویض_گرہ(nodep, تفویض_State_Vscp, قدر);
            اگر (!assp) واپس Reset_تفویض_حالت::NONE;
            اگر (!stateVscp) stateVscp = تفویض_State_Vscp;
            اگر (تفویض_State_Vscp != stateVscp) واپس Reset_تفویض_حالت::NONE;
            اگر (!resetArcs.خالی_خانے()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                واپس Reset_تفویض_حالت::MULTI_SAME_STATE;
            }
            resetArcs.emplace_عقب(قدر, assp);
            اگر (!nodep->nextp()) واپس Reset_تفویض_حالت::SINGLE;
        }
    }

    ساکن منطقی has_Canonical_اگلا_State_Default_Before_Case(Ast_گرہ* stmtsp, Ast_گرہ* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* اگلا_Vscp) {
        Ast_گرہ* ثابت bodyp = skipLeadingIgnorableStmt(stmtsp);
        منطقی sawCanonicalDefault = جھوٹ;
        برائے (Ast_گرہ* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            اگر (nodep == targetp) واپس sawCanonicalDefault;
            اگر (Ast_گرہ_تفویض* ثابت assp = VN_CAST(nodep, گرہ_تفویض)) {
                AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
                اگر (!lhsp || lhsp->varScopep() != اگلا_Vscp) جاری;
                اگر (sawCanonicalDefault) {
                    ثابت string اگلا_Name = اگلا_Vscp->varp()->prettyNameQ();
                    ثابت string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + اگلا_Name
                                     + ") when the canonical " + اگلا_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    واپس جھوٹ;
                }
                اگر (!rhsp || rhsp->varScopep() != stateVscp) واپس جھوٹ;
                sawCanonicalDefault = سچ;
            }
        }
    }

    ساکن منطقی if_State_Const_تفویض(Ast_گرہ* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_قدر& then_قدر, Fsm_State_قدر& else_قدر) {
        AstIf* ثابت ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        اگر (!ifp || !ifp->elsesp()) واپس جھوٹ;
        AstVarScope* thenVscp = خالی_اشاریہ;
        AstVarScope* elseVscp = خالی_اشاریہ;
        Ast_گرہ* ثابت thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_گرہ* ثابت elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        اگر (!thenNodep || !elseNodep) واپس جھوٹ;
        اگر (!براہ_راست_Const_State_تفویض_گرہ(thenNodep, thenVscp, then_قدر)) واپس جھوٹ;
        اگر (!براہ_راست_Const_State_تفویض_گرہ(elseNodep, elseVscp, else_قدر)) واپس جھوٹ;
        اگر (thenVscp == stateVscp && elseVscp == stateVscp) واپس سچ;
        اگر (thenVscp != elseVscp) واپس جھوٹ;
        Ast_گرہ* ثابت followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = خالی_اشاریہ;
        AstVarScope* finalFromVscp = خالی_اشاریہ;
        Ast_گرہ* ثابت finalNodep = singleMeaningfulBranch(followp);
        اگر (!finalNodep) واپس جھوٹ;
        اگر (!گرہ_State_Var_تفویض(finalNodep, finalStateVscp, finalFromVscp)) واپس جھوٹ;
        اگر (finalStateVscp != stateVscp) واپس جھوٹ;
        اگر (finalFromVscp != thenVscp) واپس جھوٹ;
        واپس سچ;
    }

    ساکن منطقی براہ_راست_State_Cond_Const_تفویض(Ast_گرہ* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_قدر& then_قدر, Fsm_State_قدر& else_قدر) {
        Ast_گرہ_تفویض* ثابت assp = براہ_راست_State_تفویض(stmtp, stateVscp);
        اگر (!assp) واپس جھوٹ;
        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
        اگر (!condp) واپس جھوٹ;
        واپس const_قدر_حالت(condp->thenp(), then_قدر) == Const_قدر_حالت::OK
               && const_قدر_حالت(condp->elsep(), else_قدر) == Const_قدر_حالت::OK;
    }

    ساکن Ast_گرہ* case_شے_Supported_Arc_گرہ(Ast_Case_شے* itemp, AstVarScope* stateVscp,
                                             منطقی inclCond) {
        اگر (itemp->isDefault()) {
            اگر (!inclCond) واپس خالی_اشاریہ;
        }
        Ast_گرہ_تفویض* ثابت assp = براہ_راست_State_تفویض(itemp->stmtsp(), stateVscp);
        اگر (assp) {
            Fsm_State_قدر مقصد_قدر;
            اگر (const_قدر_حالت(assp->rhsp(), مقصد_قدر) == Const_قدر_حالت::OK) واپس assp;
        }
        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        اگر (براہ_راست_State_Cond_Const_تفویض(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            واپس assp;
        }
        اگر (if_State_Const_تفویض(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            واپس singleMeaningfulBranch(itemp->stmtsp());
        }
        واپس خالی_اشاریہ;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ساکن Ast_گرہ* case_Supported_Transition_گرہ(AstCase* casep, AstVarScope* stateVscp,
                                                منطقی inclCond) {
        برائے (Ast_Case_شے* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {
            اگر (Ast_گرہ* ثابت nodep = case_شے_Supported_Arc_گرہ(itemp, stateVscp, inclCond))
                واپس nodep;
        }
        واپس خالی_اشاریہ;
    }

    ساکن Ast_گرہ* case_شے_Supported_Arc_گرہ_Like(Ast_گرہ* stmtsp, AstVarScope* stateVscp) {
        اگر (Ast_گرہ_تفویض* ثابت assp = براہ_راست_State_تفویض(stmtsp, stateVscp)) {
            Fsm_State_قدر مقصد_قدر;
            اگر (const_قدر_حالت(assp->rhsp(), مقصد_قدر) == Const_قدر_حالت::OK) واپس assp;
            Fsm_State_قدر then_قدر;
            Fsm_State_قدر else_قدر;
            اگر (براہ_راست_State_Cond_Const_تفویض(stmtsp, stateVscp, then_قدر, else_قدر)) واپس assp;
        }
        واپس خالی_اشاریہ;
    }

    ساکن Ast_گرہ* if_Chain_Supported_Transition_گرہ(ثابت FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        برائے (سائز_t i = 0; i < chain.branches.سائز(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            Ast_گرہ* ثابت nodep = case_شے_Supported_Arc_گرہ_Like(branch.stmtsp, stateVscp);
            اگر (!nodep) واپس خالی_اشاریہ;
        }
        واپس chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ساکن string label_For_قدر(
        ثابت std::unordered_map<Fsm_State_قدر, StateConstLabel, Fsm_State_قدر_Hash>& labels,
        ثابت Fsm_State_قدر& قدر) {
        واپس labels.at(قدر).متن;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    شمار جماعت Const_قدر_حالت : uint8_t { OK, NOT_CONST, XZ };

    ساکن Const_قدر_حالت const_قدر_حالت(Ast_گرہ_اظہاریہ* exprp, Fsm_State_قدر& قدر) {
        ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
        اگر (!constp) واپس Const_قدر_حالت::NOT_CONST;
        ثابت V3_نمبر& عدد = constp->عدد();
        اگر (عدد.isAnyXZ()) واپس Const_قدر_حالت::XZ;
        قدر = Fsm_State_قدر{عدد};
        واپس Const_قدر_حالت::OK;
    }

    ساکن منطقی خالص_State_Comparison_No_Alias(Ast_گرہ_اظہاریہ* exprp, FsmStateComparison& cmp) {
        AstEq* ثابت eqp = VN_CAST(exprp, Eq);
        اگر (!eqp) واپس جھوٹ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_گرہ_اظہاریہ* valuep = eqp->rhsp();
        اگر (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->بنیاد_Fromp(سچ), VarRef);
            اگر (!vrefp) { واپس جھوٹ; }
            valuep = eqp->lhsp();
        }

        Fsm_State_قدر قدر;
        اگر (const_قدر_حالت(valuep, قدر) != Const_قدر_حالت::OK) واپس جھوٹ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.قدر = قدر;
        واپس سچ;
    }

    ساکن منطقی خالص_State_Comparison(Ast_گرہ_اظہاریہ* exprp, ثابت FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        اگر (خالص_State_Comparison_No_Alias(exprp, cmp)) واپس سچ;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        اگر (AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) {
            ثابت FsmAliasMap::const_iterator it = aliases.ڈھونڈو(vrefp->varScopep());
            اگر (it == aliases.انتہا()) واپس جھوٹ;
            cmp = it->دوسرا;
            واپس سچ;
        }
        واپس جھوٹ;
    }

    ساکن منطقی supported_چوٹی_Level_Guard(Ast_گرہ_اظہاریہ* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        اگر (VN_IS(exprp, Or)) واپس جھوٹ;
        اگر (VN_IS(exprp, RedAnd)) واپس جھوٹ;
        اگر (VN_IS(exprp, RedOr)) واپس جھوٹ;
        اگر (VN_IS(exprp, RedXor)) واپس جھوٹ;
        واپس سچ;
    }

    ساکن منطقی resolveIfPredicate(Ast_گرہ_اظہاریہ* exprp, ثابت FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, منطقی& hasGuard) {
        std::سمتیہ<Ast_گرہ_اظہاریہ*> terms;
        std::سمتیہ<Ast_گرہ_اظہاریہ*> pending;
        pending.ڈالو_عقب(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        جب (!pending.خالی_خانے()) {
            Ast_گرہ_اظہاریہ* ثابت nodep = pending.عقب();
            pending.نکالو_عقب();
            اگر (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
                pending.ڈالو_عقب(andp->rhsp());
                pending.ڈالو_عقب(andp->lhsp());
            } ورنہ {
                terms.ڈالو_عقب(nodep);
            }
        }

        منطقی sawComparison = جھوٹ;
        برائے (سائز_t i = 0; i < terms.سائز(); ++i) {
            Ast_گرہ_اظہاریہ* ثابت termp = terms[i];
            FsmStateComparison رکن_Cmp;
            اگر (خالص_State_Comparison(termp, aliases, رکن_Cmp /*ref*/)) {
                اگر (sawComparison) واپس جھوٹ;
                cmp = رکن_Cmp;
                sawComparison = سچ;
                جاری;
            }
            اگر (!supported_چوٹی_Level_Guard(termp)) واپس جھوٹ;
            hasGuard = سچ;
        }
        واپس sawComparison;
    }

    ساکن خالی جوڑو_Alias(FsmAliasMap& aliases, std::unordered_سیٹ<ثابت AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ثابت FsmStateComparison& cmp) {
        اگر (ambiguous.ڈھونڈو(aliasVscp) != ambiguous.انتہا()) واپس;
        ثابت FsmAliasMap::iterator it = aliases.ڈھونڈو(aliasVscp);
        اگر (it == aliases.انتہا()) {
            aliases.emplace(aliasVscp, cmp);
            واپس;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        اگر (it->دوسرا.stateVscp == cmp.stateVscp && it->دوسرا.قدر == cmp.قدر) واپس;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        واپس;
    }

    ساکن خالی collect_Alias_From_تفویض(Ast_گرہ_تفویض* assp, FsmAliasMap& aliases,
                                       std::unordered_سیٹ<ثابت AstVarScope*>& ambiguous) {
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
        اگر (!lhsp) واپس;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        اگر (!خالص_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) واپس;
        جوڑو_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_گرہ* stmtsp, Ast_گرہ* limitp) ثابت {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_سیٹ<ثابت AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        برائے (Ast_گرہ* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            اگر (Ast_گرہ_تفویض* ثابت assp = VN_CAST(nodep, گرہ_تفویض)) {
                collect_Alias_From_تفویض(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        برائے (ثابت AstVarScope* ثابت vscp : ambiguous) aliases.erase(vscp);
        واپس aliases;
    }

    ساکن منطقی collectIfChain(AstIf* ifp, ثابت FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_سیٹ<string> نظر_آیا_اقدار;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        جب (سچ) {
            FsmStateComparison cmp;
            منطقی hasGuard = جھوٹ;
            اگر (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) واپس جھوٹ;
            اگر (chain.compareVscp && chain.compareVscp != cmp.stateVscp) واپس جھوٹ;
            اگر (!نظر_آیا_اقدار.داخل_کرو(cmp.قدر.کلید()).دوسرا) واپس جھوٹ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ڈالو_عقب(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.قدر, hasGuard});

            Ast_گرہ* ثابت elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            اگر (!elseNodep) توڑو;
            اگر (AstIf* ثابت elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                جاری;
            }
            chain.defaultStmtsp = elseNodep;
            توڑو;
        }
        واپس chain.branches.سائز() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ساکن منطقی validate_معلوم_State_قدر(Ast_گرہ* nodep, ثابت FsmStateSpace& stateSpace,
                                        ثابت Fsm_State_قدر& قدر, ثابت string& role) {
        اگر (stateSpace.labels.ڈھونڈو(قدر) != stateSpace.labels.انتہا()) واپس سچ;
        اگر (stateSpace.enumBacked) {
            ثابت string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + قدر.warn_متن()
                                        + " is not present in the declared enum");
            واپس جھوٹ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + قدر.warn_متن()
                                    + " is not present in the inferred state space");
        واپس جھوٹ;
    }

    ساکن StateConstLabel stateLabelForConst(AstConst* constp) {
        ثابت string name = constp->origParamName();
        اگر (!name.خالی_خانے()) واپس StateConstLabel{Ast_گرہ::prettyName(name), سچ, 0};
        واپس StateConstLabel{constp->name(), جھوٹ, 0};
    }

    ساکن خالی updateStateLabel(FsmStateSpace& stateSpace, ثابت Fsm_State_قدر& قدر,
                                 ثابت StateConstLabel& label) {
        stateSpace.حالتیں.at(stateSpace.labels.at(قدر).state_اشاریہ).پہلا = label.متن;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ساکن منطقی مطابقت_رجسٹر_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_رجسٹر_Candidate& cand) {
        اگر (!alwaysp->sentreep() || !alwaysp->sentreep()->has_کنارہ()) واپس جھوٹ;

        Ast_گرہ* ثابت stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_گرہ* ثابت nodep = singleMeaningfulStmt(stmtsp);
        اگر (!nodep) واپس جھوٹ;

        AstVarScope* stateVscp = خالی_اشاریہ;
        AstVarScope* اگلا_Vscp = خالی_اشاریہ;
        اگر (AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            اگر (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) واپس جھوٹ;
            AstVarScope* resetStateVscp = خالی_اشاریہ;
            ثابت Reset_تفویض_حالت reset_حالت
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ثابت منطقی خالی_خانے_Reset_Branch = reset_حالت == Reset_تفویض_حالت::خالی_خانے_3;
            اگر (خالی_خانے_Reset_Branch) {
                cand.resetArcs().clear();
            } ورنہ اگر (reset_حالت == Reset_تفویض_حالت::NONE) {
                cand.resetArcs().clear();
                Fsm_State_قدر reset_قدر;
                Ast_گرہ* ثابت thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_گرہ_تفویض* ثابت resetAssp
                    = براہ_راست_Const_State_تفویض_گرہ(thenNodep, resetStateVscp, reset_قدر);
                اگر (!resetAssp) واپس جھوٹ;
                cand.resetArcs().emplace_عقب(reset_قدر, resetAssp);
            } ورنہ اگر (reset_حالت == Reset_تفویض_حالت::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_گرہ* ثابت elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            اگر (!گرہ_State_Var_تفویض(elseNodep, stateVscp, اگلا_Vscp)) واپس جھوٹ;
            اگر (!خالی_خانے_Reset_Branch && resetStateVscp != stateVscp) واپس جھوٹ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != خالی_اشاریہ);
        } ورنہ {
            Ast_گرہ_اظہاریہ* resetCondp = خالی_اشاریہ;
            Fsm_State_قدر reset_قدر;
            // Note the active low 'reset ? next : CONST' form is not accepted
            اگر (Ast_گرہ_تفویض* ثابت assp
                = براہ_راست_Cond_State_Var_تفویض(nodep, stateVscp, اگلا_Vscp, resetCondp, reset_قدر)) {
                cand.resetArcs().emplace_عقب(reset_قدر, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != خالی_اشاریہ);
            } ورنہ اگر (!گرہ_State_Var_تفویض(nodep, stateVscp, اگلا_Vscp)) {
                واپس جھوٹ;
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
        واپس سچ;
    }

    ساکن منطقی جوڑو_قدر_مقصد_State_Space(Ast_گرہ* nodep, FsmStateSpace& stateSpace,
                                     ثابت Fsm_State_قدر& قدر, StateConstLabel label) {
        ثابت خودکار labelIt = stateSpace.labels.ڈھونڈو(قدر);
        اگر (labelIt != stateSpace.labels.انتہا()) {
            StateConstLabel& existingLabel = labelIt->دوسرا;
            اگر (existingLabel.متن != label.متن && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + قدر.warn_متن() + ": " + existingLabel.متن
                                            + " and " + label.متن);
                واپس جھوٹ;
            }
            اگر (!existingLabel.fromParam && label.fromParam) {
                existingLabel.متن = label.متن;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, قدر, label);
            }
        } ورنہ {
            StateConstLabel storedLabel = label;
            storedLabel.state_اشاریہ = stateSpace.حالتیں.سائز();
            stateSpace.حالتیں.emplace_عقب(label.متن, قدر);
            stateSpace.labels.emplace(قدر, storedLabel);
        }
        واپس سچ;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ساکن منطقی جوڑو_اظہاریہ_مقصد_State_Space(Ast_گرہ_اظہاریہ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_قدر قدر;
        ثابت Const_قدر_حالت حالت_2 = const_قدر_حالت(valuep, قدر);
        اگر (حالت_2 != Const_قدر_حالت::OK) {
            اگر (حالت_2 == Const_قدر_حالت::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            واپس جھوٹ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        واپس جوڑو_قدر_مقصد_State_Space(valuep, stateSpace, قدر, stateLabelForConst(constp));
    }

    ساکن منطقی جوڑو_Optional_ہدف_اظہاریہ_مقصد_State_Space(Ast_گرہ_اظہاریہ* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_قدر قدر;
        ثابت Const_قدر_حالت حالت_2 = const_قدر_حالت(valuep, قدر);
        اگر (حالت_2 != Const_قدر_حالت::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            واپس جھوٹ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        واپس جوڑو_قدر_مقصد_State_Space(valuep, stateSpace, قدر, stateLabelForConst(constp));
    }

    ساکن خالی جوڑو_Reset_Targets_مقصد_State_Space(ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        برائے (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.مقصد_قدر().ascii(), جھوٹ, 0};
            اگر (AstConst* ثابت constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                جوڑو_قدر_مقصد_State_Space(resetArc.nodep(), stateSpace, resetArc.مقصد_قدر(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ساکن منطقی collectDeclaredStateSpace(Ast_گرہ* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, منطقی& needs_ماخذ_اقدار) {
        AstVar* ثابت stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        اگر (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ثابت منطقی forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        اگر (enump) {
            stateSpace.enumBacked = سچ;
            برائے (Ast_Enum_شے* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_شے)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                ثابت Fsm_State_قدر قدر{constp->عدد()};
                ثابت سائز_t state_اشاریہ = stateSpace.حالتیں.سائز();
                stateSpace.حالتیں.emplace_عقب(itemp->name(), قدر);
                stateSpace.labels.emplace(قدر,
                                          StateConstLabel{itemp->name(), جھوٹ, state_اشاریہ});
            }
            واپس stateSpace.حالتیں.سائز() >= 2;
        }

        اگر (forced) {
            needs_ماخذ_اقدار = سچ;
            واپس سچ;
        }

        needs_ماخذ_اقدار = سچ;
        واپس سچ;
    }

    سانچہ <قسم_نام T_ValuepVisitor>
    ساکن منطقی collect_State_Space_From_اقدار(Ast_گرہ* warnNodep, AstVarScope* stateVscp,
                                            ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ثابت T_ValuepVisitor& دورہ_Valueps) {
        منطقی needs_ماخذ_اقدار = جھوٹ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        اگر (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_ماخذ_اقدار)) {
            واپس جھوٹ;
        }
        اگر (!needs_ماخذ_اقدار) واپس سچ;
        جوڑو_Reset_Targets_مقصد_State_Space(resetArcs, stateSpace);
        اگر (!دورہ_Valueps(
                [&](Ast_گرہ_اظہاریہ* valuep) { واپس جوڑو_اظہاریہ_مقصد_State_Space(valuep, stateSpace); })) {
            واپس جھوٹ;
        }
        واپس stateSpace.حالتیں.سائز() >= 2;
    }

    ساکن منطقی collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* تفویض_Vscp,
                                  ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        واپس collect_State_Space_From_اقدار(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, تفویض_Vscp, &stateSpace](ثابت خودکار& دورہ_Valuep) {
                برائے (Ast_Case_شے* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Case_شے)) {
                    اگر (!itemp->isDefault()) {
                        برائے (Ast_گرہ_اظہاریہ* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), گرہ_اظہاریہ)) {
                            اگر (!دورہ_Valuep(condp)) واپس جھوٹ;
                        }
                    }
                    اگر (Ast_گرہ_تفویض* ثابت assp
                        = براہ_راست_State_تفویض(itemp->stmtsp(), تفویض_Vscp)) {
                        Fsm_State_قدر then_قدر;
                        Fsm_State_قدر else_قدر;
                        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                        اگر (condp
                            && براہ_راست_State_Cond_Const_تفویض(itemp->stmtsp(), تفویض_Vscp, then_قدر,
                                                          else_قدر)) {
                            اگر (!دورہ_Valuep(condp->thenp())) واپس جھوٹ;
                            اگر (!دورہ_Valuep(condp->elsep())) واپس جھوٹ;
                        } ورنہ اگر (!جوڑو_Optional_ہدف_اظہاریہ_مقصد_State_Space(assp->rhsp(), stateSpace)) {
                            واپس جھوٹ;
                        }
                    }
                }
                واپس سچ;
            });
    }

    ساکن منطقی collectStateSpace(ثابت FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* تفویض_Vscp,
                                  ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        واپس collect_State_Space_From_اقدار(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, تفویض_Vscp, &stateSpace](ثابت خودکار& دورہ_Valuep) {
                برائے (ثابت FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(دورہ_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_گرہ_تفویض* ثابت assp = براہ_راست_State_تفویض(branch.stmtsp, تفویض_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_قدر then_قدر;
                    Fsm_State_قدر else_قدر;
                    AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                    اگر (condp) {
                        UASSERT_OBJ(براہ_راست_State_Cond_Const_تفویض(branch.stmtsp, تفویض_Vscp,
                                                               then_قدر, else_قدر),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        اگر (!دورہ_Valuep(condp->thenp())) واپس جھوٹ;
                        اگر (!دورہ_Valuep(condp->elsep())) واپس جھوٹ;
                    } ورنہ اگر (!جوڑو_Optional_ہدف_اظہاریہ_مقصد_State_Space(assp->rhsp(), stateSpace)) {
                        واپس جھوٹ;
                    }
                }
                واپس سچ;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ساکن منطقی emit_Case_شے_Arcs(Fsm_گراف& گراف, Ast_Case_شے* itemp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, منطقی inclCond) {
        std::سمتیہ<std::pair<string, Fsm_State_قدر>> froms;
        اگر (itemp->isDefault()) {
            اگر (!inclCond) واپس جھوٹ;
            froms.emplace_عقب("default", Fsm_State_قدر{});
        } ورنہ {
            برائے (Ast_گرہ_اظہاریہ* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), گرہ_اظہاریہ)) {
                Fsm_State_قدر قدر;
                اگر (const_قدر_حالت(condp, قدر) != Const_قدر_حالت::OK) جاری;
                اگر (!validate_معلوم_State_قدر(condp, stateSpace, قدر, "source")) واپس سچ;
                froms.emplace_عقب(label_For_قدر(stateSpace.labels, قدر), قدر);
            }
            اگر (froms.خالی_خانے()) واپس جھوٹ;
        }

        اگر (Ast_گرہ_تفویض* ثابت assp = براہ_راست_State_تفویض(itemp->stmtsp(), stateVscp)) {
            Fsm_State_قدر مقصد_قدر;
            ثابت Const_قدر_حالت حالت_2 = const_قدر_حالت(assp->rhsp(), مقصد_قدر);
            اگر (حالت_2 == Const_قدر_حالت::OK) {
                اگر (!validate_معلوم_State_قدر(assp, stateSpace, مقصد_قدر, "target")) واپس سچ;
                برائے (ثابت std::pair<string, Fsm_State_قدر>& from : froms) {
                    گراف.جوڑو_Arc(from.دوسرا, مقصد_قدر, جھوٹ, جھوٹ, itemp->isDefault(),
                                 assp->fileline());
                }
                واپس سچ;
            }
        }

        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        اگر (براہ_راست_State_Cond_Const_تفویض(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)
            || if_State_Const_تفویض(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            اگر (!validate_معلوم_State_قدر(itemp->stmtsp(), stateSpace, then_قدر, "target"))
                واپس سچ;
            اگر (!validate_معلوم_State_قدر(itemp->stmtsp(), stateSpace, else_قدر, "target"))
                واپس سچ;
            برائے (ثابت Fsm_State_قدر& branch_قدر : {then_قدر, else_قدر}) {
                برائے (ثابت std::pair<string, Fsm_State_قدر>& from : froms) {
                    گراف.جوڑو_Arc(from.دوسرا, branch_قدر, جھوٹ, سچ, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            واپس سچ;
        }

        واپس جھوٹ;
    }

    ساکن خالی emitStmtArcsFrom(Fsm_گراف& گراف, Ast_گرہ* stmtsp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, Fsm_State_قدر from_قدر,
                                 منطقی isDefault, منطقی forceCond) {
        Ast_گرہ_تفویض* ثابت assp = براہ_راست_State_تفویض(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_قدر مقصد_قدر;
        ثابت Const_قدر_حالت حالت_2 = const_قدر_حالت(assp->rhsp(), مقصد_قدر);
        اگر (حالت_2 == Const_قدر_حالت::OK) {
            اگر (!validate_معلوم_State_قدر(assp, stateSpace, مقصد_قدر, "target")) واپس;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            گراف.جوڑو_Arc(from_قدر, مقصد_قدر, جھوٹ, forceCond, isDefault, assp->fileline());
            واپس;
        }

        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        ثابت منطقی cond_تفویض
            = براہ_راست_State_Cond_Const_تفویض(stmtsp, stateVscp, then_قدر, else_قدر);
        UASSERT_OBJ(cond_تفویض, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        اگر (!validate_معلوم_State_قدر(stmtsp, stateSpace, then_قدر, "target")) واپس;
        اگر (!validate_معلوم_State_قدر(stmtsp, stateSpace, else_قدر, "target")) واپس;
        برائے (ثابت Fsm_State_قدر& branch_قدر : {then_قدر, else_قدر}) {
            گراف.جوڑو_Arc(from_قدر, branch_قدر, جھوٹ, سچ, isDefault, stmtsp->fileline());
        }
    }

    ساکن خالی emitIfChainArcs(Fsm_گراف& گراف, ثابت FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ثابت FsmStateSpace& stateSpace) {
        برائے (سائز_t i = 0; i < chain.branches.سائز(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            اگر (!validate_معلوم_State_قدر(branch.ifp, stateSpace, branch.from_قدر, "source"))
                واپس;
            emitStmtArcsFrom(گراف, branch.stmtsp, stateVscp, stateSpace, branch.from_قدر, جھوٹ,
                             branch.has_چوٹی_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ساکن خالی جوڑو_Reset_Arcs(Fsm_گراف& گراف, ثابت std::سمتیہ<FsmResetArcDesc>& resetArcs,
                             ثابت FsmStateSpace& stateSpace) {
        برائے (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            اگر (!validate_معلوم_State_قدر(resetArc.nodep(), stateSpace, resetArc.مقصد_قدر(),
                                         "target"))
                جاری;
            گراف.جوڑو_Arc(Fsm_State_قدر{}, resetArc.مقصد_قدر(), سچ, جھوٹ, جھوٹ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    خالی processCase(AstCase* casep, AstVarScope* تفویض_Vscp, ثابت Fsm_رجسٹر_Candidate& reg) {
        UASSERT_OBJ(تفویض_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        اگر (!collectStateSpace(casep, stateVscp, تفویض_Vscp, reg.resetArcs(), stateSpace)) واپس;
        DetectedFsm& اندراج_2 = m_state.fsmFor(stateVscp);
        اگر (!اندراج_2.graphp) {
            اندراج_2.graphp.reset(نیا Fsm_گراف{});
            اندراج_2.graphp->scopep(reg.scopep());
            اندراج_2.graphp->stateAlwaysp(reg.alwaysp());
            اندراج_2.graphp->stateVarName(stateVscp->prettyName());
            اندراج_2.graphp->state_Var_اندرونی_Name(stateVscp->varp()->name());
            اندراج_2.graphp->stateVarScopep(stateVscp);
            اندراج_2.graphp->sampleVarScopep(reg.sampleVscp());
            اندراج_2.graphp->senses() = reg.senses();
            اندراج_2.graphp->resetCond() = reg.resetCond();
            اندراج_2.graphp->hasResetCond(reg.hasResetCond());
            اندراج_2.graphp->inclCond(reg.inclCond());
            اندراج_2.graphp->fileline(casep->fileline());
            برائے (ثابت std::pair<string, Fsm_State_قدر>& state : stateSpace.حالتیں) {
                اندراج_2.graphp->جوڑو_State_راس(state.پہلا, state.دوسرا);
            }
            جوڑو_Reset_Arcs(*اندراج_2.graphp, reg.resetArcs(), stateSpace);
        }
        برائے (Ast_Case_شے* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شے)) {
            emit_Case_شے_Arcs(*اندراج_2.graphp, itemp, تفویض_Vscp, stateSpace,
                             اندراج_2.graphp->inclCond());
        }
    }

    خالی processIfChain(ثابت FsmIfChainCandidate& chain, AstVarScope* تفویض_Vscp,
                        ثابت Fsm_رجسٹر_Candidate& reg) {
        UASSERT_OBJ(تفویض_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        اگر (!collectStateSpace(chain, stateVscp, تفویض_Vscp, reg.resetArcs(), stateSpace)) واپس;
        DetectedFsm& اندراج_2 = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!اندراج_2.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        اندراج_2.graphp.reset(نیا Fsm_گراف{});
        اندراج_2.graphp->scopep(reg.scopep());
        اندراج_2.graphp->stateAlwaysp(reg.alwaysp());
        اندراج_2.graphp->stateVarName(stateVscp->prettyName());
        اندراج_2.graphp->state_Var_اندرونی_Name(stateVscp->varp()->name());
        اندراج_2.graphp->stateVarScopep(stateVscp);
        اندراج_2.graphp->sampleVarScopep(reg.sampleVscp());
        اندراج_2.graphp->senses() = reg.senses();
        اندراج_2.graphp->resetCond() = reg.resetCond();
        اندراج_2.graphp->hasResetCond(reg.hasResetCond());
        اندراج_2.graphp->inclCond(reg.inclCond());
        اندراج_2.graphp->fileline(chain.ifp->fileline());
        برائے (ثابت std::pair<string, Fsm_State_قدر>& state : stateSpace.حالتیں) {
            اندراج_2.graphp->جوڑو_State_راس(state.پہلا, state.دوسرا);
        }
        جوڑو_Reset_Arcs(*اندراج_2.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*اندراج_2.graphp, chain, تفویض_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    خالی process_One_بلاک_Always(ثابت FsmComboAlways& one_بلاک) {
        ثابت رجسٹر_Always_Analyzer analyzer{one_بلاک.scopep()};
        AstAlways* ثابت alwaysp = one_بلاک.alwaysp();
        اگر (!alwaysp->sentreep() || !alwaysp->sentreep()->has_کنارہ()) واپس;
        ثابت std::سمتیہ<std::pair<AstCase*, Ast_گرہ_اظہاریہ*>> candidates
            = analyzer.one_بلاک_Candidates(alwaysp);

        FsmCaseCandidate پہلا_Cand;
        برائے (ثابت std::pair<AstCase*, Ast_گرہ_اظہاریہ*>& cand : candidates) {
            AstVarRef* ثابت selp = VN_CAST(cand.پہلا->exprp(), VarRef);
            AstVarScope* ثابت vscp = selp ? selp->varScopep() : خالی_اشاریہ;
            اگر (!vscp) جاری;
            اگر (!پہلا_Cand.stateVscp) {
                پہلا_Cand.warnNodep = cand.پہلا;
                پہلا_Cand.stateVscp = vscp;
                Fsm_رجسٹر_Candidate reg;
                analyzer.build_One_بلاک_Candidate(alwaysp, vscp, cand.دوسرا, reg);
                processCase(cand.پہلا, vscp, reg);
            } ورنہ اگر (vscp != پہلا_Cand.stateVscp) {
                cand.پہلا->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.پہلا, پہلا_Cand));
            } ورنہ {
                cand.پہلا->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.پہلا, پہلا_Cand));
            }
        }

        اگر (پہلا_Cand.stateVscp) واپس;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ثابت std::سمتیہ<std::pair<AstIf*, Ast_گرہ_اظہاریہ*>> ifCandidates
            = analyzer.one_بلاک_If_Candidates(alwaysp);
        برائے (ثابت std::pair<AstIf*, Ast_گرہ_اظہاریہ*>& cand : ifCandidates) {
            ثابت FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.پہلا);
            FsmIfChainCandidate chain;
            اگر (!collectIfChain(cand.پہلا, aliases, chain)) جاری;
            AstVarScope* ثابت vscp = chain.compareVscp;
            اگر (!if_Chain_Supported_Transition_گرہ(chain, vscp)) جاری;
            اگر (!پہلا_Cand.stateVscp) {
                پہلا_Cand.warnNodep = cand.پہلا;
                پہلا_Cand.stateVscp = vscp;
                Fsm_رجسٹر_Candidate reg;
                analyzer.build_One_بلاک_Candidate(alwaysp, vscp, cand.دوسرا, reg);
                processIfChain(chain, vscp, reg);
            } ورنہ اگر (vscp != پہلا_Cand.stateVscp) {
                cand.پہلا->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.پہلا, پہلا_Cand));
            } ورنہ {
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
    ساکن خالی warnComboSameAlways(Ast_گرہ* warnNodep, ثابت FsmCaseCandidate& پہلا_Cand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, پہلا_Cand));
    }

    خالی processComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_رجسٹر_Candidates};
        Ast_گرہ* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate پہلا_Cand;
        برائے (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            اگر (!casep) جاری;
            ثابت ComboAlwaysAnalyzer::Combo_مطابقت مطابقت = analyzer.مطابقت_Case(stmtsp, casep);
            ثابت Fsm_رجسٹر_Candidate* ثابت matchedp = مطابقت.matchedp;
            Ast_گرہ* ثابت matchedWarnNodep = مطابقت.warnNodep;
            اگر (!matchedp) جاری;
            اگر (!پہلا_Cand.stateVscp) {
                ثابت خودکار داخل_کرو_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                اگر (!داخل_کرو_Pair.دوسرا) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  داخل_کرو_Pair.پہلا->دوسرا));
                    جاری;
                }
                پہلا_Cand.warnNodep = matchedWarnNodep;
                پہلا_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->اگلا_Vscp(), *matchedp);
                جاری;
            }
            اگر (matchedp->stateVscp() != پہلا_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, پہلا_Cand);
                جاری;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, پہلا_Cand));
        }
        اگر (پہلا_Cand.stateVscp) واپس;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        برائے (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ثابت ifp = VN_CAST(nodep, If);
            اگر (!ifp) جاری;
            FsmIfChainCandidate chain;
            ثابت FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            اگر (!collectIfChain(ifp, aliases, chain)) جاری;
            ثابت ComboAlwaysAnalyzer::Combo_مطابقت مطابقت = analyzer.مطابقت_If_Chain(stmtsp, chain);
            ثابت Fsm_رجسٹر_Candidate* ثابت matchedp = مطابقت.matchedp;
            Ast_گرہ* ثابت matchedWarnNodep = مطابقت.warnNodep;
            اگر (!matchedp) جاری;
            اگر (!پہلا_Cand.stateVscp) {
                ثابت std::pair<std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate>::iterator,
                                منطقی>
                    داخل_کرو_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                اگر (!داخل_کرو_Pair.دوسرا) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  داخل_کرو_Pair.پہلا->دوسرا));
                    جاری;
                }
                پہلا_Cand.warnNodep = matchedWarnNodep;
                پہلا_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->اگلا_Vscp(), *matchedp);
                جاری;
            }
            اگر (matchedp->stateVscp() != پہلا_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, پہلا_Cand);
                جاری;
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
    خالی دورہ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    خالی دورہ(AstAlways* nodep) override {
        اگر (nodep->keyword() == VAlwaysKwd::CONT_تفویض) {
            iterateChildren(nodep);
            واپس;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_بلاک_Alwayss.emplace_عقب(m_scopep, nodep);
        ثابت رجسٹر_Always_Analyzer analyzer{m_scopep};
        Fsm_رجسٹر_Candidate reg;
        اگر (analyzer.مطابقت_رجسٹر_Candidate(nodep, reg)) {
            AstVarScope* ثابت stateVscp = reg.stateVscp();
            ثابت منطقی ملا
                = std::any_کا(m_رجسٹر_Candidates.cbegin(), m_رجسٹر_Candidates.cend(),
                              [stateVscp](ثابت Fsm_رجسٹر_Candidate& existing) {
                                  واپس existing.stateVscp() == stateVscp;
                              });
            اگر (!ملا) { m_رجسٹر_Candidates.emplace_عقب(reg); }
        }
        اگر (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_عقب(m_scopep, nodep);
        } ورنہ اگر (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            اگر (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_عقب(m_scopep, nodep);
            } ورنہ {
                m_nonComboAlwayss.emplace_عقب(m_scopep, nodep);
            }
        }
    }

    خالی دورہ(Ast_تفویض_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_تفویض(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_پورٹ_Alias(nodep);
        iterateChildren(nodep);
    }

    خالی دورہ(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        اگر (m_scopep && fsm_رجسٹر_Wrapper_Desc(nodep)) جوڑو_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        برائے (ثابت std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_رجسٹر_Candidate reg;
            اگر (مطابقت_Fsm_Wrapper_Cell(wrapperCell.پہلا, wrapperCell.دوسرا, reg)) {
                m_رجسٹر_Candidates.emplace_عقب(reg);
            }
        }
        برائے (ثابت FsmComboAlways& one_بلاک : m_one_بلاک_Alwayss) process_One_بلاک_Always(one_بلاک);
        برائے (ثابت FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        برائے (ثابت FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
جماعت FsmCoverIncBuilder final {
    ثابت Fsm_گراف& m_گراف;  // Fsm graph
    std::سمتیہ<ثابت Fsm_State_راس*> m_حالتیں;  // States vertexes inside fsm graph
    std::unordered_سیٹ<uint64_t>
        m_undefined_حالتیں;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ثابت Fsm_راس* m_defaultVertexp = خالی_اشاریہ;  // Default vertex
    ثابت Fsm_راس* m_resetVertexp = خالی_اشاریہ;  // Reset vertex
    سائز_t m_کنارے_گنتی = 0;  // Total count of edges
    AstVarScope* ثابت m_پچھلا_Vscp;  // Previous state of fsm
    File_سطر* ثابت m_flp;  // Fileline pointer to use across the builder
    ثابت std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_گرہ_Stmt* m_coverIncp = خالی_اشاریہ;  // Increments proper CoverDecl after each change
    Ast_گرہ_Stmt* m_initialCoverIncp = خالی_اشاریہ;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = خالی_اشاریہ;  // List of CoverDecls
    منطقی m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ثابت uint64_t قدر) {
        V3_نمبر عدد{static_cast<Ast_گرہ*>(خالی_اشاریہ), m_پچھلا_Vscp->چوڑائی()};
        عدد.سیٹ_Quad(قدر);
        عدد.isSigned(m_پچھلا_Vscp->dtypep()->isSigned());
        واپس نیا AstConst{m_flp, عدد};
    }

    std::string quad_مقصد_ثنائی_ہندسہ_Stirng_Label(ثابت uint64_t val) {
        صحیح چوڑائی = m_پچھلا_Vscp->چوڑائی();
        UASSERT(چوڑائی < 64, "We should not expand values that wide");
        std::string نتیجہ = std::مقصد_string(چوڑائی) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        جب (--چوڑائی >= 0) نتیجہ.ڈالو_عقب("01"[(val >> چوڑائی) & 1]);
        واپس نتیجہ;
    }

    خالی init_Existing_حالتیں() {
        برائے (ثابت V3_گراف_راس& vtx : m_گراف.vertices()) {
            ثابت Fsm_راس* ثابت vertexp = vtx.as<Fsm_راس>();
            m_کنارے_گنتی += vertexp->اندر_کنارے().سائز();
            اگر (ثابت Fsm_State_راس* ثابت statep = vertexp->cast<Fsm_State_راس>()) {
                m_حالتیں.ڈالو_عقب(statep);
            } ورنہ اگر (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_گراف.sampleVarScopep(),
                            "Two defaults?");
            } ورنہ /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_گراف.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ثابت صحیح چوڑائی = m_گراف.sampleVarScopep()->چوڑائی();
        m_isComplete = چوڑائی < 64 && m_حالتیں.سائز() == (static_cast<سائز_t>(1) << چوڑائی);
    }

    خالی init_Undefined_حالتیں() {
        UASSERT_OBJ(m_گراف.sampleVarScopep()->چوڑائی() < 64, m_گراف.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ثابت uint64_t combinations_گنتی = static_cast<سائز_t>(1)
                                           << m_گراف.sampleVarScopep()->چوڑائی();
        // Rolling trick allows to cut peak memory usage
        منطقی undefRolled = جھوٹ;
        خودکار rollUndefs = [یہ, combinations_گنتی] {
            برائے (uint64_t i = 0; i < combinations_گنتی; ++i) {
                ثابت خودکار داخل_کرو = m_undefined_حالتیں.داخل_کرو(i);
                اگر (!داخل_کرو.دوسرا) m_undefined_حالتیں.erase(داخل_کرو.پہلا);
            }
        };
        برائے (ثابت Fsm_State_راس* ثابت statep : m_حالتیں) {
            ثابت V3_نمبر& عدد = statep->قدر().عدد();
            ثابت uint64_t قدر = عدد.مقصد_UQuad();
            اگر (!m_isComplete) {
                اگر (undefRolled) {
                    UASSERT_OBJ(m_undefined_حالتیں.erase(قدر) == 1, m_گراف.sampleVarScopep(),
                                "Two states has the same value?");
                } ورنہ {
                    UASSERT_OBJ(m_undefined_حالتیں.داخل_کرو(قدر).دوسرا, m_گراف.sampleVarScopep(),
                                "Two states has the same value?");
                    اگر (m_undefined_حالتیں.سائز() * 2 >= combinations_گنتی) {
                        undefRolled = سچ;
                        rollUndefs();
                    }
                }
            }
        }
        اگر (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_گرہ_اظہاریہ* buildResetCond() ثابت {
        Ast_گرہ_اظہاریہ* ثابت refp
            = نیا AstVarRef{m_flp, m_گراف.resetCond().varScopep, VAccess::READ};
        واپس m_گراف.resetCond().فعال_کم
                   ? static_cast<Ast_گرہ_اظہاریہ*>(نیا AstLogNot{m_flp, refp})
                   : refp;
    }

    سائز_t get_Arc_گنتی_With_Expansion(ثابت VFsmExpandType expand) ثابت {
        سائز_t arc_گنتی_With_Expansion = ~0ULL;
        انتخاب (expand) {
        حالت VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_گراف.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            توڑو;
        }
        حالت VFsmExpandType::AUTO: {
            arc_گنتی_With_Expansion = m_کنارے_گنتی;
            اگر (m_defaultVertexp) {
                UASSERT_OBJ(m_حالتیں.سائز() >= m_defaultVertexp->برآمد_کنارے().سائز(),
                            m_گراف.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_گنتی_With_Expansion -= m_defaultVertexp->برآمد_کنارے().سائز();
                برائے (ثابت Fsm_State_راس* ثابت statep : m_حالتیں) {
                    arc_گنتی_With_Expansion += static_cast<سائز_t>(statep->برآمد_کنارے().خالی_خانے())
                                             * m_defaultVertexp->برآمد_کنارے().سائز();
                }
            }
            اگر (m_resetVertexp) {
                UASSERT_OBJ(m_حالتیں.سائز() >= m_resetVertexp->برآمد_کنارے().سائز(),
                            m_گراف.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_گنتی_With_Expansion += m_حالتیں.سائز() - m_resetVertexp->برآمد_کنارے().سائز();
            }
            توڑو;
        }
        حالت VFsmExpandType::AUTO_EXPAND: {
            arc_گنتی_With_Expansion = m_حالتیں.سائز() * m_حالتیں.سائز() - m_حالتیں.سائز();
            توڑو;
        }
        حالت VFsmExpandType::بھرا: {
            ثابت صحیح چوڑائی = m_گراف.sampleVarScopep()->چوڑائی();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            اگر (چوڑائی < 32) {
                ثابت uint64_t combinations_گنتی = static_cast<سائز_t>(1) << چوڑائی;
                arc_گنتی_With_Expansion = combinations_گنتی * combinations_گنتی - combinations_گنتی;
            }
            توڑو;
        }
        }
        واپس arc_گنتی_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ثابت std::string& label,
                                           ثابت std::string& fsm_لیبل = "") {
        AstCoverOtherDecl* ثابت declp
            = نیا AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_گراف.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_گراف.stateVarName(),
                                    "",
                                    label,
                                    fsm_لیبل};
        // We run after scoping, so this is declared for one instance only
        declp->فی_Instance(سچ);
        m_declsp = Ast_گرہ::جوڑو_اگلا(m_declsp, declp);
        واپس declp;
    }

    Ast_گرہ_Stmt* buildCoverIncStmtp(ثابت std::string& fromStateLabel,
                                    ثابت std::string& مقصد_State_Label,
                                    ثابت std::string& بنیاد_Label = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ثابت declp = نیا AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_گراف.stateVarName() + "::" + fromStateLabel + "->" + مقصد_State_Label,
            "",
            0,
            m_گراف.stateVarName(),
            fromStateLabel,
            مقصد_State_Label,
            بنیاد_Label};
        // We run after scoping, so this is declared for one instance only
        declp->فی_Instance(سچ);
        m_declsp = Ast_گرہ::جوڑو_اگلا(m_declsp, declp);
        واپس نیا AstCoverInc{m_flp, declp};
    }
    Ast_گرہ_Stmt* buildExpandedTransitionHandler(ثابت std::string& مقصد_State_Label,
                                                ثابت منطقی considerUndefs,
                                                ثابت منطقی expandOnlyWithTerminalVerticies,
                                                ثابت Fsm_State_راس* ثابت مقصد_Statep = خالی_اشاریہ) {
        AstCase* ثابت arcCasep = نیا AstCase{
            m_flp, VCaseType::CT_CASE, نیا AstVarRef{m_flp, m_پچھلا_Vscp, VAccess::READ}, خالی_اشاریہ};
        std::unordered_سیٹ<ثابت Fsm_State_راس*> handledSoureces;
        اگر (مقصد_Statep) {
            برائے (ثابت V3_گراف_کنارہ& کنارہ : مقصد_Statep->اندر_کنارے()) {
                اگر (ثابت Fsm_State_راس* ثابت fromp = کنارہ.fromp()->cast<Fsm_State_راس>()) {
                    اگر (!handledSoureces.emplace(fromp).دوسرا) جاری;
                    arcCasep->جوڑو_Itemsp(نیا Ast_Case_شے{
                        m_flp, makeConst(fromp->قدر().عدد().مقصد_UQuad()),
                        (fromp != مقصد_Statep) ? buildCoverIncStmtp(fromp->label(), مقصد_State_Label)
                                            : خالی_اشاریہ});
                }
            }
        }
        برائے (ثابت Fsm_State_راس* ثابت fromp : m_حالتیں) {
            اگر (handledSoureces.ڈھونڈو(fromp) != handledSoureces.انتہا()) جاری;
            arcCasep->جوڑو_Itemsp(نیا Ast_Case_شے{
                m_flp, makeConst(fromp->قدر().عدد().مقصد_UQuad()),
                (fromp != مقصد_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->برآمد_کنارے().خالی_خانے()))
                    ? buildCoverIncStmtp(fromp->label(), مقصد_State_Label, "artificial")
                    : خالی_اشاریہ});
        }
        اگر (!m_isComplete) {
            اگر (considerUndefs) {
                برائے (ثابت uint64_t from : m_undefined_حالتیں) {
                    ثابت std::string& label = quad_مقصد_ثنائی_ہندسہ_Stirng_Label(from);
                    arcCasep->جوڑو_Itemsp(
                        نیا Ast_Case_شے{m_flp, makeConst(from),
                                        (label != مقصد_State_Label)
                                            ? buildCoverIncStmtp(label, مقصد_State_Label, "artificial")
                                            : خالی_اشاریہ});
                }
            } ورنہ {
                arcCasep->جوڑو_Itemsp(نیا Ast_Case_شے{m_flp, خالی_اشاریہ, خالی_اشاریہ});
            }
        }
        واپس arcCasep;
    }
    Ast_گرہ_Stmt* buildUnxpandedTransitionHandler(ثابت Fsm_State_راس* ثابت مقصد_Statep) {
        AstCase* ثابت arcCasep = نیا AstCase{
            m_flp, VCaseType::CT_CASE, نیا AstVarRef{m_flp, m_پچھلا_Vscp, VAccess::READ}, خالی_اشاریہ};
        برائے (ثابت V3_گراف_کنارہ& کنارہ : مقصد_Statep->اندر_کنارے()) {
            ثابت Fsm_Arc_کنارہ* ثابت arcp = کنارہ.as<Fsm_Arc_کنارہ>();
            UASSERT_OBJ(!arcp->isReset(), m_گراف.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_گراف.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ثابت Fsm_State_راس* ثابت fromp = arcp->fromp()->as<Fsm_State_راس>();
            اگر (fromp != مقصد_Statep) {
                arcCasep->جوڑو_Itemsp(
                    نیا Ast_Case_شے{m_flp, makeConst(fromp->قدر().عدد().مقصد_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), مقصد_Statep->label())});
            }
        }
        arcCasep->جوڑو_Itemsp(نیا Ast_Case_شے{m_flp, خالی_اشاریہ, خالی_اشاریہ});
        واپس arcCasep;
    }

عوامی:
    FsmCoverIncBuilder(ثابت Fsm_گراف& گراف, AstVarScope* ثابت پچھلا_Vscp)
        : m_گراف{گراف}
        , m_پچھلا_Vscp{پچھلا_Vscp}
        , m_flp{گراف.fileline()}
        , m_modPrettyName{گراف.scopep()->modp()->prettyName()} {
        init_Existing_حالتیں();
        AstVarScope* ثابت sampleVscp = گراف.sampleVarScopep();
        ثابت VFsmExpandType expand
            = گراف.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? گراف.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ثابت سائز_t arc_گنتی_With_Expansion = get_Arc_گنتی_With_Expansion(expand);
            اگر (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_گنتی_With_Expansion == 0ULL
                            || arc_گنتی_With_Expansion
                                   > static_cast<سائز_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_گنتی_With_Expansion);
                واپس;
            }
        }

        AstCase* ثابت arcCasep
            = نیا AstCase{m_flp, VCaseType::CT_CASE,
                          نیا AstVarRef{m_flp, گراف.sampleVarScopep(), VAccess::READ}, خالی_اشاریہ};
        AstCase* ثابت initialStateCasep
            = نیا AstCase{m_flp, VCaseType::CT_CASE,
                          نیا AstVarRef{m_flp, گراف.sampleVarScopep(), VAccess::READ}, خالی_اشاریہ};

        ثابت منطقی expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_سیٹ<ثابت Fsm_State_راس*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_سیٹ<ثابت Fsm_State_راس*>
            resetTargets;  // States that have edge incoming from reset vertex
        اگر (!expandAll) {
            اگر (m_defaultVertexp) {
                برائے (ثابت V3_گراف_کنارہ& کنارہ : m_defaultVertexp->برآمد_کنارے()) {
                    defaultTargets.emplace(کنارہ.چوٹی()->as<Fsm_State_راس>());
                }
            }
            اگر (m_resetVertexp) {
                برائے (ثابت V3_گراف_کنارہ& کنارہ : m_resetVertexp->برآمد_کنارے()) {
                    resetTargets.emplace(کنارہ.چوٹی()->as<Fsm_State_راس>());
                }
            }
        }
        ثابت منطقی is_بھرا_Expansion = expand == VFsmExpandType::بھرا;
        اگر (is_بھرا_Expansion) init_Undefined_حالتیں();
        برائے (ثابت Fsm_State_راس* ثابت statep : m_حالتیں) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(statep->label());
            initialStateCasep->جوڑو_Itemsp(
                نیا Ast_Case_شے{m_flp, makeConst(statep->قدر().عدد().مقصد_UQuad()),
                                نیا AstCoverInc{m_flp, declp, سچ}});
            ثابت منطقی expandAllOrReset
                = expandAll || (resetTargets.ڈھونڈو(statep) != resetTargets.انتہا());
            ثابت منطقی defaultExpantion
                = !expandAllOrReset && (defaultTargets.ڈھونڈو(statep) != defaultTargets.انتہا());
            arcCasep->جوڑو_Itemsp(نیا Ast_Case_شے{
                m_flp, makeConst(statep->قدر().عدد().مقصد_UQuad()),
                Ast_گرہ::جوڑو_اگلا<Ast_گرہ_Stmt>(
                    نیا AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_بھرا_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        اگر (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            اگر (is_بھرا_Expansion) {
                برائے (ثابت uint64_t قدر : m_undefined_حالتیں) {
                    ثابت std::string& label = quad_مقصد_ثنائی_ہندسہ_Stirng_Label(قدر);
                    AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(label);
                    initialStateCasep->جوڑو_Itemsp(نیا Ast_Case_شے{
                        m_flp, makeConst(قدر), نیا AstCoverInc{m_flp, declp, سچ}});
                    arcCasep->جوڑو_Itemsp(
                        نیا Ast_Case_شے{m_flp, makeConst(قدر),
                                        Ast_گرہ::جوڑو_اگلا<Ast_گرہ_Stmt>(
                                            نیا AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, سچ, جھوٹ))});
                }
            } ورنہ {
                // No expansion
                arcCasep->جوڑو_Itemsp(نیا Ast_Case_شے{m_flp, خالی_اشاریہ, خالی_اشاریہ});
                initialStateCasep->جوڑو_Itemsp(نیا Ast_Case_شے{m_flp, خالی_اشاریہ, خالی_اشاریہ});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = طےشدہ;
    Ast_گرہ_Cover_Decl* getCoverDeclsp() ثابت { واپس m_declsp; }
    Ast_گرہ_Stmt* getCoverIncp() ثابت { واپس m_coverIncp; }
    Ast_گرہ_Stmt* getInitialCoverIncp() ثابت { واپس m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
جماعت Fsm_زیریں_Visitor final {
    // STATE - across all visitors
    ثابت FsmState& m_state;
    V3SharedTmps m_پچھلا_Tmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ساکن Ast_Sen_درخت* build_Sen_درخت(File_سطر* flp, ثابت std::سمتیہ<FsmSenDesc>& senses) {
        Ast_Sen_درخت* ثابت sentreep = نیا Ast_Sen_درخت{flp, خالی_اشاریہ};
        برائے (ثابت FsmSenDesc& sense : senses) {
            Ast_Sen_شے* ثابت senItemp
                = نیا Ast_Sen_شے{flp, VEdgeType{sense.کنارہ_Type},
                                 نیا AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->جوڑو_Sensesp(senItemp);
        }
        واپس sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    خالی buildOne(ثابت Fsm_گراف& گراف) {
        UINFO(1, "buildOne lowering FSM " << گراف.stateVarName()
                                          << " vertices=" << گراف.vertices().سائز() << سطر_ختم);
        AstAlways* ثابت alwaysp = گراف.stateAlwaysp();
        AstScope* ثابت scopep = گراف.scopep();
        AstVarScope* ثابت stateVscp = گراف.stateVarScopep();
        AstVarScope* ثابت sampleVscp = گراف.sampleVarScopep();
        File_سطر* ثابت flp = گراف.fileline();
        Ast_گرہ_Module* ثابت modp = scopep->modp();
        AstVarScope* ثابت پچھلا_Vscp
            = m_پچھلا_Tmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(سچ);
        sampleVscp->optimizeLifePost(سچ);
        پچھلا_Vscp->optimizeLifePost(سچ);

        Ast_فعال* ثابت initActivep
            = نیا Ast_فعال{flp, "fsm-coverage-init",
                            نیا Ast_Sen_درخت{flp, نیا Ast_Sen_شے{flp, Ast_Sen_شے::Initial{}}}};
        initActivep->sen_درخت_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->جوڑو_Stmtsp(نیا AstInitialStatic{
            flp, نیا Ast_تفویض{flp, نیا AstVarRef{flp, پچھلا_Vscp, VAccess::WRITE},
                               نیا AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->جوڑو_Blocksp(initActivep);

        AstAlwaysPost* ثابت covPostp = نیا AstAlwaysPost{flp};
        منطقی update_پچھلا_After_Post = جھوٹ;
        اگر (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, پچھلا_Vscp, VAccess::WRITE},
                                                 نیا AstVarRef{flp, sampleVscp, VAccess::READ}});
        } ورنہ {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            update_پچھلا_After_Post = سچ;
            پچھلا_Vscp->varp()->سیٹ_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ثابت FsmCoverIncBuilder coverIncBuilder{گراف, پچھلا_Vscp};
            modp->جوڑو_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->جوڑو_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->جوڑو_Stmtsp(
                نیا AstIf{flp,
                          نیا AstNeq{flp, نیا AstVarRef{flp, پچھلا_Vscp, VAccess::READ},
                                     نیا AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        اگر (update_پچھلا_After_Post) {
            covPostp->جوڑو_Stmtsp(نیا Ast_تفویض{flp, نیا AstVarRef{flp, پچھلا_Vscp, VAccess::WRITE},
                                              نیا AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_درخت* ثابت sentreep = build_Sen_درخت(flp, گراف.senses());
        Ast_فعال* ثابت activep = نیا Ast_فعال{flp, "fsm-coverage", sentreep};
        activep->sen_درخت_Storep(sentreep);
        scopep->جوڑو_Blocksp(activep);
        activep->جوڑو_Stmtsp(covPostp);
    }

عوامی:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    صریح Fsm_زیریں_Visitor(ثابت FsmState& state)
        : m_state{state} {
        برائے (ثابت DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

خالی V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    اگر (dump_گراف_Level() >= 6) {
        سائز_t اشاریہ = 0;
        برائے (ثابت DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_لیبل(اشاریہ++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_زیریں_Visitor زیریں{state}; }
    V3Global::dump_Check_Global_درخت("fsm-detect", 0, dump_درخت_Either_Level() >= 3);
}
