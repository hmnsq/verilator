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
    string m_ڪنجي;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_عدد m_انگ;  // Original folded value, preserving width for lowered comparisons

    ساكن string make_ڪنجي(ثابت V3_عدد& انگ) {
        V3_عدد ڪنجي_انگ = انگ;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        ڪنجي_انگ.isSigned(خطأ);
        أرجع cvt_منزل_Str(ڪنجي_انگ.ويڪر()) + ":" + ڪنجي_انگ.ascii(سچ, سچ);
    }

عام:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_قدر()
        : m_ڪنجي{"1:1'h0"}
        , m_انگ{static_cast<Ast_ڳنڍ*>(خالي_اشارو), 1, 0} {}
    واضح Fsm_State_قدر(ثابت V3_عدد& انگ)
        : m_ڪنجي{make_ڪنجي(انگ)}
        , m_انگ{انگ} {}

    ثابت string& ڪنجي() ثابت { أرجع m_ڪنجي; }
    ثابت V3_عدد& انگ() ثابت { أرجع m_انگ; }
    string ascii() ثابت { أرجع m_انگ.ascii(سچ, سچ); }
    string warn_متن() ثابت {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        إذا (m_انگ.ويڪر() <= 32) أرجع cvt_منزل_Str(m_انگ.منزل_UInt());
        أرجع ascii();
    }

    منطقي عامل==(ثابت Fsm_State_قدر& rhs) ثابت { أرجع m_ڪنجي == rhs.m_ڪنجي; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
بنية Fsm_State_قدر_Hash final {
    سائيز_t عامل()(ثابت Fsm_State_قدر& قدر) ثابت {
        أرجع std::hash<string>{}(قدر.ڪنجي());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
بنية FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en ڪنارو_Type = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = خالي_اشارو;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
بنية FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = خالي_اشارو;
    منطقي سرگرم_هيٺيون = خطأ;
};

صنف FsmResetArcDesc final {
    Fsm_State_قدر m_منزل_قدر;  // Encoded reset target state.
    Ast_ڳنڍ* m_nodep = خالي_اشارو;  // Source node for warnings and emitted metadata.
    Ast_ڳنڍ_اظهار* m_valuep = خالي_اشارو;  // Expression that provided the reset value.

عام:
    FsmResetArcDesc() = افتراضي;
    FsmResetArcDesc(Fsm_State_قدر منزل_قدر, Ast_ڳنڍ_سونپيو* nodep)
        : m_منزل_قدر{منزل_قدر}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_قدر منزل_قدر, Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* valuep)
        : m_منزل_قدر{منزل_قدر}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_قدر منزل_قدر() ثابت { أرجع m_منزل_قدر; }
    Ast_ڳنڍ* nodep() ثابت { أرجع m_nodep; }
    Ast_ڳنڍ_اظهار* valuep() ثابت { أرجع m_valuep; }
};

بنية FsmWrapperRoles final {
    string d_پورٽ;
    string q_پورٽ;
    string clk_پورٽ;
    string rst_پورٽ;
    string rstValParam;
    منطقي has_Rst_سرگرم_هيٺيون = خطأ;
    منطقي rst_سرگرم_هيٺيون = خطأ;
};

ساكن منطقي fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ثابت string& پورٽ_Name,
                                                  منطقي& سرگرم_هيٺيون) {
    منطقي matched = خطأ;
    cellp->modp()->foreach([&](Ast_Sen_شي* itemp) {
        Ast_ڳنڍ_Var_Ref* ثابت vrefp = itemp->varrefp();
        إذا (!vrefp) أرجع;
        إذا (vrefp->varp()->name() != پورٽ_Name) أرجع;
        سرگرم_هيٺيون = itemp->ڪنارو_Type() == VEdgeType::ET_NEGEDGE;
        matched = سچ;
    });
    أرجع matched;
}

ساكن ثابت V3Control::Fsm_رجسٽر_Wrapper* fsm_رجسٽر_Wrapper_Desc(AstCell* cellp) {
    Ast_ڳنڍ_Module* ثابت modp = cellp->modp();
    ثابت string origName = modp->origName();
    إذا (ثابت V3Control::Fsm_رجسٽر_Wrapper* ثابت descp
        = V3Control::get_Fsm_رجسٽر_Wrapper(origName)) {
        أرجع descp;
    }
    أرجع V3Control::get_Fsm_رجسٽر_Wrapper(modp->prettyDehashOrigOrName());
}

ساكن FsmWrapperRoles rolesFromDesc(ثابت V3Control::Fsm_رجسٽر_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_پورٽ = desc.d;
    roles.q_پورٽ = desc.q;
    roles.clk_پورٽ = desc.clock;
    roles.rst_پورٽ = desc.reset;
    roles.rstValParam = desc.reset_قدر;
    أرجع roles;
}

صنف Fsm_رجسٽر_Candidate final {
    AstScope* m_scopep = خالي_اشارو;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = خالي_اشارو;  // Register process that commits the state.
    AstVarScope* m_stateVscp = خالي_اشارو;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = خالي_اشارو;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_ايندڙ_Vscp = خالي_اشارو;  // Next-state variable or same state var for 1-block FSMs.
    std::ويڪٽر<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::ويڪٽر<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    منطقي m_hasResetCond = خطأ;  // Whether the FSM had a modeled reset predicate.
    منطقي m_inclCond = خطأ;  // Whether conditional/default arcs are kept explicitly.
    File_سٽ* m_flp = خالي_اشارو;  // Representative source location.

عام:
    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
    فراغ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ثابت { أرجع m_stateVscp; }
    فراغ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ثابت { أرجع m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    فراغ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* ايندڙ_Vscp() ثابت { أرجع m_ايندڙ_Vscp; }
    فراغ ايندڙ_Vscp(AstVarScope* vscp) { m_ايندڙ_Vscp = vscp; }
    ثابت std::ويڪٽر<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::ويڪٽر<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    ثابت std::ويڪٽر<FsmResetArcDesc>& resetArcs() ثابت { أرجع m_resetArcs; }
    std::ويڪٽر<FsmResetArcDesc>& resetArcs() { أرجع m_resetArcs; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    File_سٽ* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_سٽ* flp) { m_flp = flp; }
};

صنف FsmComboAlways final {
    AstScope* ثابت m_scopep = خالي_اشارو;  // Owning scope for the combinational process.
    AstAlways* ثابت m_alwaysp = خالي_اشارو;  // Candidate transition process.

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

صنف Fsm_Arc_ڪنارو final : عام V3_گراف_ڪنارو {
    VL_RTTI_IMPL(Fsm_Arc_ڪنارو, V3_گراف_ڪنارو)
    منطقي m_isReset = خطأ;  // Arc originates from the synthetic reset source.
    منطقي m_isCond = خطأ;  // Arc came from a conditional next-state split.
    منطقي m_isDefault = خطأ;  // Arc represents a case default source.
    File_سٽ* m_flp = خالي_اشارو;  // Source location for emitted coverage metadata.

عام:
    Fsm_Arc_ڪنارو(V3_گراف* graphp, Fsm_راس* fromp, Fsm_State_راس* چوٽي, منطقي isReset, منطقي isCond,
               منطقي isDefault, File_سٽ* flp) VL_MT_DISABLED : V3_گراف_ڪنارو{graphp, fromp, چوٽي, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~Fsm_Arc_ڪنارو() override = افتراضي;

    منطقي isReset() ثابت { أرجع m_isReset; }
    منطقي isCond() ثابت { أرجع m_isCond; }
    منطقي isDefault() ثابت { أرجع m_isDefault; }
    File_سٽ* fileline() ثابت { أرجع m_flp; }

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
    AstScope* m_scopep = خالي_اشارو;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = خالي_اشارو;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_اندروني_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = خالي_اشارو;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = خالي_اشارو;  // Scoped variable sampled by coverage logic.
    std::ويڪٽر<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    منطقي m_hasResetCond = خطأ;  // Whether the detected FSM had a reset branch.
    منطقي m_inclCond = خطأ;  // Whether conditional arcs should be kept explicitly.
    File_سٽ* m_flp = خالي_اشارو;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_قدر, Fsm_State_راس*, Fsm_State_قدر_Hash>
        m_stateVertices;  // Value to state map.
    Fsm_Pseudo_راس* m_resetVertexp = خالي_اشارو;  // Synthetic ANY source for reset arcs.
    Fsm_Pseudo_راس* m_defaultVertexp = خالي_اشارو;  // Synthetic default source for case defaults.

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
    ثابت string& state_Var_اندروني_Name() ثابت { أرجع m_state_Var_اندروني_Name; }
    فراغ state_Var_اندروني_Name(ثابت string& name) { m_state_Var_اندروني_Name = name; }
    AstVarScope* stateVarScopep() ثابت { أرجع m_stateVarScopep; }
    فراغ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ثابت {
        أرجع m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    فراغ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ثابت std::ويڪٽر<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::ويڪٽر<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    File_سٽ* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_سٽ* flp) { m_flp = flp; }

    Fsm_State_راس* جوڙيو_State_راس(string label, Fsm_State_قدر قدر) VL_MT_DISABLED {
        Fsm_State_راس* ثابت vertexp = جديد Fsm_State_راس{هذا, label, قدر};
        m_stateVertices.emplace(قدر, vertexp);
        أرجع vertexp;
    }
    Fsm_Pseudo_راس* reset_Any_راس() VL_MT_DISABLED { أرجع m_resetVertexp; }
    Fsm_Pseudo_راس* default_Any_راس() VL_MT_DISABLED { أرجع m_defaultVertexp; }
    Fsm_Arc_ڪنارو* جوڙيو_Arc(Fsm_State_قدر from_قدر, Fsm_State_قدر منزل_قدر, منطقي isReset, منطقي isCond,
                       منطقي isDefault, File_سٽ* flp) VL_MT_DISABLED {
        Fsm_State_راس* ثابت چوٽي = m_stateVertices.at(منزل_قدر);
        Fsm_راس* fromp = خالي_اشارو;
        إذا (isReset) {
            fromp = reset_Any_راس();
        } وإلا إذا (isDefault) {
            fromp = default_Any_راس();
        } وإلا {
            fromp = m_stateVertices.at(from_قدر);
        }
        أرجع جديد Fsm_Arc_ڪنارو{هذا, fromp, چوٽي, isReset, isCond, isDefault, flp};
    }

    string name() ثابت VL_MT_محفوظ {
        أرجع "FSM "
               + (m_stateVarName.خالي_جايون() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_ٽيگ(سائيز_t اشاريو) ثابت {
        string ٽيگ = state_Var_اندروني_Name();
        لكل (حرف& ch : ٽيگ) {
            إذا (!std::اکر_يا_انگ(static_cast<بنانشان حرف>(ch))) ch = '_';
        }
        أرجع "fsm_" + cvt_منزل_Str(اشاريو) + "_" + ٽيگ;
    }
};

بنية DetectedFsm final {
    std::unique_ptr<Fsm_گراف> graphp;  // Extracted graph for one detected FSM candidate.
};

بنية FsmCaseCandidate final {
    Ast_ڳنڍ* warnNodep = خالي_اشارو;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = خالي_اشارو;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
بنية FsmStateComparison final {
    AstVarScope* stateVscp = خالي_اشارو;  // Compared state variable
    Ast_ڳنڍ_اظهار* valuep = خالي_اشارو;  // Compared constant value expression
    Fsm_State_قدر قدر;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
بنية FsmIfBranch final {
    AstIf* ifp = خالي_اشارو;  // Source if/else-if node
    Ast_ڳنڍ* stmtsp = خالي_اشارو;  // Branch body
    Ast_ڳنڍ_اظهار* valuep = خالي_اشارو;  // Source state value expression
    Fsm_State_قدر from_قدر;  // Encoded source state value
    منطقي has_چوٽي_Guard = خطأ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
بنية FsmIfChainCandidate final {
    AstIf* ifp = خالي_اشارو;  // Top-level if-chain node
    AstVarScope* compareVscp = خالي_اشارو;  // Variable used by every state comparison
    std::ويڪٽر<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_ڳنڍ* defaultStmtsp = خالي_اشارو;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
استعمال FsmAliasMap = std::unordered_map<ثابت AstVarScope*, FsmStateComparison>;
استعمال Fsm_Cell_پورٽ_Map = std::unordered_map<string, AstVarScope*>;
استعمال Fsm_Cell_پورٽ_Alias_Map = std::unordered_map<ثابت AstCell*, Fsm_Cell_پورٽ_Map>;

بنية StateConstLabel final {
    string متن;
    منطقي fromParam = خطأ;
    سائيز_t state_اشاريو = 0;
};

بنية FsmStateSpace final {
    std::ويڪٽر<std::pair<string, Fsm_State_قدر>> حالتون;  // User label and encoded value
    std::unordered_map<Fsm_State_قدر, StateConstLabel, Fsm_State_قدر_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = خالي_اشارو;  // Tracked FSM state variable
    منطقي enumBacked = خطأ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
صنف FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::ويڪٽر<DetectedFsm> m_fsms;
    std::map<ثابت AstVarScope*, سائيز_t> m_fsm_اشاريو;

عام:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ثابت std::map<ثابت AstVarScope*, سائيز_t>::const_iterator it = m_fsm_اشاريو.ڳوليو(stateVscp);
        إذا (it != m_fsm_اشاريو.پڇاڙي()) أرجع m_fsms.at(it->ٻيو);
        ثابت سائيز_t اشاريو = m_fsms.سائيز();
        m_fsm_اشاريو.emplace(stateVscp, اشاريو);
        m_fsms.emplace_پٺ();
        أرجع m_fsms.پٺ();
    }
    ثابت std::ويڪٽر<DetectedFsm>& fsms() ثابت { أرجع m_fsms; }
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
    AstScope* m_scopep = خالي_اشارو;
    std::ويڪٽر<Fsm_رجسٽر_Candidate> m_رجسٽر_Candidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::ويڪٽر<FsmComboAlways> m_one_بلاڪ_Alwayss;
    std::ويڪٽر<FsmComboAlways> m_comboAlwayss;
    std::ويڪٽر<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::ويڪٽر<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_سيٽ<ثابت AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    Fsm_Cell_پورٽ_Alias_Map m_cell_پورٽ_Aliases;
    Fsm_Cell_پورٽ_Alias_Map m_cell_پورٽ_ٻار_Aliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ساكن Ast_ڳنڍ_DType* unwrapEnumCandidate(Ast_ڳنڍ_DType* dtypep) {
        أرجع dtypep->skip_Ref_منزل_Enump();
    }

    ساكن string candidateConflictContext(Ast_ڳنڍ* laterNodep,
                                           ثابت FsmCaseCandidate& پهريون_Cand) {
        أرجع '\n' + laterNodep->warnContextPrimary() + پهريون_Cand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + پهريون_Cand.stateVscp->prettyNameQ() + '\n'
               + پهريون_Cand.warnNodep->warnContextSecondary();
    }

    ساكن منطقي rejectFsmWrapperCell(AstCell* cellp, ثابت string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        أرجع خطأ;
    }

    ساكن منطقي simple_Param_State_قدر(AstCell* cellp, ثابت string& name, Fsm_State_قدر& قدر,
                                      Ast_ڳنڍ_اظهار*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = خالي_اشارو;
        لكل (Ast_ڳنڍ* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isParam() || varp->name() != name) استمر;
            valuepr = VN_AS(varp->valuep(), ڳنڍ_اظهار);
            أرجع const_قدر_حالت(valuepr, قدر) == Const_قدر_حالت::OK;
        }
        أرجع خطأ;
    }

    ساكن منطقي ٻار_پورٽ_اندريون_Scope(AstVarScope* vscp, AstScope* والد_Scopep, AstCell*& cellpr) {
        إذا (!vscp->varp()->isIO()) أرجع خطأ;
        AstScope* ثابت scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        إذا (scopep->aboveScopep() != والد_Scopep) أرجع خطأ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        أرجع سچ;
    }

    ساكن AstVarScope* simple_سونپيو_Var_Scope(Ast_ڳنڍ_اظهار* exprp) {
        AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef);
        أرجع vrefp ? vrefp->varScopep() : خالي_اشارو;
    }

    فراغ جوڙيو_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_پورٽ_Aliases.emplace(cellp, Fsm_Cell_پورٽ_Map{});
        m_cell_پورٽ_ٻار_Aliases.emplace(cellp, Fsm_Cell_پورٽ_Map{});
        ثابت std::pair<AstScope*, AstCell*> شي_2{scopep, cellp};
        إذا (std::ڳوليو(m_wrapperCells.cbegin(), m_wrapperCells.cend(), شي_2)
            != m_wrapperCells.cend()) {
            أرجع;
        }
        m_wrapperCells.emplace_پٺ(شي_2);
    }

    فراغ collect_Cell_پورٽ_Alias(Ast_سونپيو_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ثابت lhsVscp = simple_سونپيو_Var_Scope(nodep->lhsp());
        AstVarScope* ثابت rhsVscp = simple_سونپيو_Var_Scope(nodep->rhsp());
        إذا (!lhsVscp || !rhsVscp) أرجع;
        AstCell* cellp = خالي_اشارو;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        إذا (ٻار_پورٽ_اندريون_Scope(lhsVscp, m_scopep, cellp)) {
            إذا (!fsm_رجسٽر_Wrapper_Desc(cellp)) أرجع;
            UASSERT_OBJ(lhsVscp->varp()->is_داخلا(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_پورٽ_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_پورٽ_ٻار_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            جوڙيو_Wrapper_Cell(m_scopep, cellp);
        } وإلا إذا (ٻار_پورٽ_اندريون_Scope(rhsVscp, m_scopep, cellp)) {
            إذا (!fsm_رجسٽر_Wrapper_Desc(cellp)) أرجع;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cell_پورٽ_Aliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cell_پورٽ_ٻار_Aliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            جوڙيو_Wrapper_Cell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ثابت string& پورٽ_Name) ثابت {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ثابت Fsm_Cell_پورٽ_Map& ports = m_cell_پورٽ_Aliases.at(cellp);
        ثابت Fsm_Cell_پورٽ_Map::const_iterator پورٽ_It = ports.ڳوليو(پورٽ_Name);
        أرجع پورٽ_It == ports.پڇاڙي() ? خالي_اشارو : پورٽ_It->ٻيو;
    }

    AstVarScope* ٻار_Role_Var_Scope(AstCell* cellp, ثابت string& پورٽ_Name) ثابت {
        ثابت Fsm_Cell_پورٽ_Map& ports = m_cell_پورٽ_ٻار_Aliases.at(cellp);
        ثابت Fsm_Cell_پورٽ_Map::const_iterator پورٽ_It = ports.ڳوليو(پورٽ_Name);
        أرجع پورٽ_It == ports.پڇاڙي() ? خالي_اشارو : پورٽ_It->ٻيو;
    }

    صنف رجسٽر_Always_Analyzer final {
        AstScope* ثابت m_scopep;  // Scope containing the always block(s) being analyzed

    عام:
        واضح رجسٽر_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::ويڪٽر<std::pair<AstCase*, Ast_ڳنڍ_اظهار*>>
        one_بلاڪ_Candidates(AstAlways* alwaysp) ثابت {
            std::ويڪٽر<std::pair<AstCase*, Ast_ڳنڍ_اظهار*>> candidates;
            Ast_ڳنڍ* ثابت stmtsp = alwaysp->stmtsp();
            إذا (AstIf* ثابت پهريون_Ifp = VN_CAST(stmtsp, If)) {
                إذا (AstCase* ثابت casep = VN_CAST(پهريون_Ifp->elsesp(), Case)) {
                    candidates.emplace_پٺ(casep,
                                            FsmDetectVisitor::isSimpleResetCond(پهريون_Ifp->condp())
                                                ? پهريون_Ifp->condp()
                                                : خالي_اشارو);
                }
            }
            لكل (Ast_ڳنڍ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstCase* ثابت casep = VN_CAST(nodep, Case))
                    candidates.emplace_پٺ(casep, خالي_اشارو);
            }
            أرجع candidates;
        }

        std::ويڪٽر<std::pair<AstIf*, Ast_ڳنڍ_اظهار*>>
        one_بلاڪ_If_Candidates(AstAlways* alwaysp) ثابت {
            std::ويڪٽر<std::pair<AstIf*, Ast_ڳنڍ_اظهار*>> candidates;
            Ast_ڳنڍ* ثابت stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            إذا (AstIf* ثابت پهريون_Ifp = VN_CAST(stmtsp, If)) {
                إذا (AstIf* ثابت chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(پهريون_Ifp->elsesp()), If)) {
                    candidates.emplace_پٺ(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(پهريون_Ifp->condp())
                                                ? پهريون_Ifp->condp()
                                                : خالي_اشارو);
                }
            }
            لكل (Ast_ڳنڍ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) candidates.emplace_پٺ(ifp, خالي_اشارو);
            }
            أرجع candidates;
        }

        منطقي ميلاپ_رجسٽر_Candidate(AstAlways* alwaysp, Fsm_رجسٽر_Candidate& cand) ثابت {
            أرجع FsmDetectVisitor::ميلاپ_رجسٽر_Always(alwaysp, m_scopep, cand);
        }

        فراغ build_One_بلاڪ_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_ڳنڍ_اظهار* resetCondp,
                                    Fsm_رجسٽر_Candidate& reg) ثابت {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.ايندڙ_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_وڻ(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != خالي_اشارو);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ثابت پهريون_Ifp = VN_CAST(alwaysp->stmtsp(), If);
            إذا (پهريون_Ifp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = خالي_اشارو;
                ثابت Reset_سونپيو_حالت reset_حالت = FsmDetectVisitor::collectConstStateAssigns(
                    پهريون_Ifp->thensp(), resetStateVscp, reg.resetArcs());
                إذا (reset_حالت == Reset_سونپيو_حالت::خالي_جايون_3) {
                    reg.resetArcs().clear();
                } وإلا إذا (reset_حالت == Reset_سونپيو_حالت::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_قدر reset_قدر;
                    Ast_ڳنڍ* ثابت thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(پهريون_Ifp->thensp());
                    UASSERT_OBJ(thenNodep, پهريون_Ifp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_ڳنڍ_سونپيو* ثابت resetAssp = FsmDetectVisitor::سڌو_Const_State_سونپيو_ڳنڍ(
                        thenNodep, resetStateVscp, reset_قدر);
                    إذا (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_پٺ(reset_قدر, resetAssp);
                    }
                } وإلا إذا (reset_حالت == Reset_سونپيو_حالت::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    منطقي ميلاپ_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_رجسٽر_Candidate& cand) ثابت {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_رجسٽر_Wrapper_Desc(cellp));

        AstVarScope* ثابت ايندڙ_Vscp = roleVarScope(cellp, roles.d_پورٽ);
        AstVarScope* ثابت stateVscp = roleVarScope(cellp, roles.q_پورٽ);
        إذا (!ايندڙ_Vscp || !stateVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ثابت clkVscp = roleVarScope(cellp, roles.clk_پورٽ);
        إذا (!clkVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.ڪنارو_Type = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ڌڪو_پٺ(clkSense);

        AstVarScope* resetVscp = خالي_اشارو;
        إذا (!roles.rst_پورٽ.خالي_جايون()) resetVscp = roleVarScope(cellp, roles.rst_پورٽ);
        إذا (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            منطقي inferred_سرگرم_هيٺيون = خطأ;
            إذا (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_پورٽ, inferred_سرگرم_هيٺيون)) {
                roles.has_Rst_سرگرم_هيٺيون = سچ;
                roles.rst_سرگرم_هيٺيون = inferred_سرگرم_هيٺيون;
            }
        }

        Ast_ڳنڍ_اظهار* resetValuep = خالي_اشارو;
        Fsm_State_قدر reset_قدر;
        ثابت منطقي has_Reset_قدر
            = !roles.rstValParam.خالي_جايون()
              && simple_Param_State_قدر(cellp, roles.rstValParam, reset_قدر, resetValuep);
        إذا (resetVscp && roles.has_Rst_سرگرم_هيٺيون && has_Reset_قدر) {
            FsmSenDesc rstSense;
            rstSense.ڪنارو_Type = roles.rst_سرگرم_هيٺيون ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ڌڪو_پٺ(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().سرگرم_هيٺيون = roles.rst_سرگرم_هيٺيون;
            cand.hasResetCond(سچ);
            cand.resetArcs().emplace_پٺ(reset_قدر, cellp, resetValuep);
        } وإلا إذا (!roles.rst_پورٽ.خالي_جايون() || !roles.rstValParam.خالي_جايون()) {
            string reason;
            إذا (roles.rst_پورٽ.خالي_جايون()) {
                reason = "reset port is not configured";
            } وإلا إذا (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } وإلا إذا (!roles.has_Rst_سرگرم_هيٺيون) {
                reason = "reset polarity could not be inferred from the wrapper";
            } وإلا إذا (roles.rstValParam.خالي_جايون()) {
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
        cand.alwaysp(خالي_اشارو);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(ٻار_Role_Var_Scope(cellp, roles.q_پورٽ));
        cand.ايندڙ_Vscp(ايندڙ_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        أرجع سچ;
    }

    صنف ComboAlwaysAnalyzer final {
    عام:
        بنية Combo_ميلاپ final {
            ثابت Fsm_رجسٽر_Candidate* matchedp = خالي_اشارو;
            Ast_ڳنڍ* warnNodep = خالي_اشارو;
        };

    خاص:
        ثابت std::ويڪٽر<Fsm_رجسٽر_Candidate>& m_رجسٽر_Candidates;

    عام:
        واضح ComboAlwaysAnalyzer(ثابت std::ويڪٽر<Fsm_رجسٽر_Candidate>& رجسٽر_Candidates)
            : m_رجسٽر_Candidates{رجسٽر_Candidates} {}

        Combo_ميلاپ ميلاپ_Case(Ast_ڳنڍ* stmtsp, AstCase* casep) ثابت {
            Combo_ميلاپ ميلاپ;
            AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع ميلاپ;
            لكل (ثابت Fsm_رجسٽر_Candidate& reg : m_رجسٽر_Candidates) {
                إذا (selp->varScopep() == reg.ايندڙ_Vscp()) {
                    إذا (!FsmDetectVisitor::has_Canonical_ايندڙ_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.ايندڙ_Vscp())) {
                        استمر;
                    }
                } وإلا إذا (selp->varScopep() != reg.stateVscp()) {
                    استمر;
                }
                Ast_ڳنڍ* ثابت warnNodep = FsmDetectVisitor::case_Supported_Transition_ڳنڍ(
                    casep, reg.ايندڙ_Vscp(), reg.inclCond());
                إذا (!warnNodep) استمر;
                ميلاپ.matchedp = &reg;
                ميلاپ.warnNodep = warnNodep;
            }
            أرجع ميلاپ;
        }

        Combo_ميلاپ ميلاپ_If_Chain(Ast_ڳنڍ* stmtsp, ثابت FsmIfChainCandidate& chain) ثابت {
            Combo_ميلاپ ميلاپ;
            لكل (ثابت Fsm_رجسٽر_Candidate& reg : m_رجسٽر_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                إذا (chain.compareVscp == reg.ايندڙ_Vscp()) {
                    إذا (!FsmDetectVisitor::has_Canonical_ايندڙ_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.ايندڙ_Vscp())) {
                        استمر;
                    }
                } وإلا إذا (chain.compareVscp != reg.stateVscp()) {
                    استمر;
                }
                Ast_ڳنڍ* ثابت warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_ڳنڍ(chain, reg.ايندڙ_Vscp());
                إذا (!warnNodep) استمر;
                ميلاپ.matchedp = &reg;
                ميلاپ.warnNodep = warnNodep;
            }
            أرجع ميلاپ;
        }

        منطقي shouldWarnUnsupported(Ast_ڳنڍ* stmtsp, AstCase* casep) ثابت {
            ثابت AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع خطأ;

            أرجع std::any_جو(
                m_رجسٽر_Candidates.cbegin(), m_رجسٽر_Candidates.cend(),
                [&](ثابت Fsm_رجسٽر_Candidate& reg) -> منطقي {
                    ثابت منطقي matches_ايندڙ = selp->varScopep() == reg.ايندڙ_Vscp();
                    ثابت منطقي matchesState = selp->varScopep() == reg.stateVscp();

                    إذا (!matches_ايندڙ && !matchesState) أرجع خطأ;
                    إذا (matches_ايندڙ
                        && !FsmDetectVisitor::has_Canonical_ايندڙ_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.ايندڙ_Vscp())) {
                        أرجع خطأ;
                    }
                    أرجع FsmDetectVisitor::case_Supported_Transition_ڳنڍ(casep, reg.ايندڙ_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ساكن منطقي isSimpleResetCond(Ast_ڳنڍ_اظهار* condp) { أرجع VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ساكن FsmResetCondDesc describeResetCond(Ast_ڳنڍ_اظهار* condp) {
        FsmResetCondDesc desc;
        إذا (AstVarRef* ثابت vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        أرجع desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ساكن std::ويڪٽر<FsmSenDesc> describe_Sen_وڻ(Ast_Sen_وڻ* sentreep) {
        std::ويڪٽر<FsmSenDesc> senses;
        لكل (Ast_Sen_شي* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), Sen_شي)) {
            Ast_ڳنڍ_Var_Ref* ثابت vrefp = itemp->varrefp();
            إذا (!vrefp) استمر;
            FsmSenDesc desc;
            desc.ڪنارو_Type = itemp->ڪنارو_Type().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ڌڪو_پٺ(desc);
        }
        أرجع senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ساكن منطقي isIgnorableStmt(Ast_ڳنڍ* nodep) { أرجع VN_IS(nodep, CoverInc); }

    ساكن Ast_ڳنڍ* skipLeadingIgnorableStmt(Ast_ڳنڍ* nodep) {
        طالما (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ساكن Ast_ڳنڍ* singleMeaningfulStmt(Ast_ڳنڍ* stmtp) {
        Ast_ڳنڍ* resultp = خالي_اشارو;
        لكل (Ast_ڳنڍ* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            إذا (isIgnorableStmt(nodep)) استمر;
            إذا (resultp) أرجع خالي_اشارو;
            resultp = nodep;
        }
        أرجع resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ساكن Ast_ڳنڍ* singleMeaningfulBranch(Ast_ڳنڍ* branchp) {
        إذا (!branchp) أرجع خالي_اشارو;
        أرجع branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ساكن منطقي isPlainComboSentree(ثابت Ast_Sen_وڻ* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        لكل (ثابت Ast_Sen_شي* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), Sen_شي)) {
            إذا (senp->ڪنارو_Type() == VEdgeType::ET_بدليو) استمر;
            أرجع خطأ;
        }
        أرجع سچ;
    }

    فراغ warnUnsupportedComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_رجسٽر_Candidates};
        Ast_ڳنڍ* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        منطقي warned = خطأ;
        لكل (Ast_ڳنڍ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
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
    ساكن Ast_ڳنڍ_سونپيو* سڌو_State_سونپيو(Ast_ڳنڍ* stmtp, AstVarScope* stateVscp) {
        Ast_ڳنڍ* ثابت nodep = singleMeaningfulBranch(stmtp);
        إذا (!nodep) أرجع خالي_اشارو;
        Ast_ڳنڍ_سونپيو* ثابت assp = VN_CAST(nodep, ڳنڍ_سونپيو);
        إذا (!assp) أرجع خالي_اشارو;
        AstVarRef* ثابت vrefp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!vrefp || vrefp->varScopep() != stateVscp) أرجع خالي_اشارو;
        أرجع assp;
    }

    ساكن Ast_ڳنڍ_سونپيو* ڳنڍ_State_Var_سونپيو(Ast_ڳنڍ* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_ڳنڍ_سونپيو* ثابت assp = VN_CAST(nodep, ڳنڍ_سونپيو);
        إذا (!assp) أرجع خالي_اشارو;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنياد_Fromp(سچ), VarRef);
        AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
        إذا (!rhsp || !lhsp) أرجع خالي_اشارو;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        أرجع assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ساكن Ast_ڳنڍ_سونپيو* سڌو_Cond_State_Var_سونپيو(Ast_ڳنڍ* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_ڳنڍ_اظهار*& condp,
                                                   Fsm_State_قدر& reset_قدر) {
        Ast_ڳنڍ_سونپيو* ثابت assp = VN_CAST(nodep, ڳنڍ_سونپيو);
        إذا (!assp) أرجع خالي_اشارو;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنياد_Fromp(سچ), VarRef);
        AstCond* ثابت rhsp = VN_CAST(assp->rhsp(), Cond);
        إذا (!rhsp || !lhsp) أرجع خالي_اشارو;
        AstVarRef* ثابت elsep = VN_CAST(rhsp->elsep(), VarRef);
        إذا (!elsep) أرجع خالي_اشارو;
        إذا (const_قدر_حالت(rhsp->thenp(), reset_قدر) != Const_قدر_حالت::OK) أرجع خالي_اشارو;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        أرجع assp;
    }

    ساكن Ast_ڳنڍ_سونپيو* سڌو_Const_State_سونپيو_ڳنڍ(Ast_ڳنڍ* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_قدر& قدر) {
        Ast_ڳنڍ_سونپيو* ثابت assp = VN_CAST(nodep, ڳنڍ_سونپيو);
        إذا (!assp) أرجع خالي_اشارو;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->بنياد_Fromp(سچ), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        إذا (const_قدر_حالت(assp->rhsp(), قدر) != Const_قدر_حالت::OK) أرجع خالي_اشارو;
        stateVscp = lhsp->varScopep();
        أرجع assp;
    }

    تعداد صنف Reset_سونپيو_حالت : uint8_t {
        خالي_جايون_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ساكن Reset_سونپيو_حالت collectConstStateAssigns(Ast_ڳنڍ* stmtp, AstVarScope*& stateVscp,
                                                      std::ويڪٽر<FsmResetArcDesc>& resetArcs) {
        Ast_ڳنڍ* nodep = skipLeadingIgnorableStmt(stmtp);
        إذا (!nodep) أرجع Reset_سونپيو_حالت::خالي_جايون_3;
        لكل (;; nodep = nodep->nextp()) {
            AstVarScope* سونپيو_State_Vscp = خالي_اشارو;
            Fsm_State_قدر قدر;
            Ast_ڳنڍ_سونپيو* ثابت assp = سڌو_Const_State_سونپيو_ڳنڍ(nodep, سونپيو_State_Vscp, قدر);
            إذا (!assp) أرجع Reset_سونپيو_حالت::NONE;
            إذا (!stateVscp) stateVscp = سونپيو_State_Vscp;
            إذا (سونپيو_State_Vscp != stateVscp) أرجع Reset_سونپيو_حالت::NONE;
            إذا (!resetArcs.خالي_جايون()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                أرجع Reset_سونپيو_حالت::MULTI_SAME_STATE;
            }
            resetArcs.emplace_پٺ(قدر, assp);
            إذا (!nodep->nextp()) أرجع Reset_سونپيو_حالت::SINGLE;
        }
    }

    ساكن منطقي has_Canonical_ايندڙ_State_Default_Before_Case(Ast_ڳنڍ* stmtsp, Ast_ڳنڍ* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* ايندڙ_Vscp) {
        Ast_ڳنڍ* ثابت bodyp = skipLeadingIgnorableStmt(stmtsp);
        منطقي sawCanonicalDefault = خطأ;
        لكل (Ast_ڳنڍ* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            إذا (nodep == targetp) أرجع sawCanonicalDefault;
            إذا (Ast_ڳنڍ_سونپيو* ثابت assp = VN_CAST(nodep, ڳنڍ_سونپيو)) {
                AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
                إذا (!lhsp || lhsp->varScopep() != ايندڙ_Vscp) استمر;
                إذا (sawCanonicalDefault) {
                    ثابت string ايندڙ_Name = ايندڙ_Vscp->varp()->prettyNameQ();
                    ثابت string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + ايندڙ_Name
                                     + ") when the canonical " + ايندڙ_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    أرجع خطأ;
                }
                إذا (!rhsp || rhsp->varScopep() != stateVscp) أرجع خطأ;
                sawCanonicalDefault = سچ;
            }
        }
    }

    ساكن منطقي if_State_Const_سونپيو(Ast_ڳنڍ* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_قدر& then_قدر, Fsm_State_قدر& else_قدر) {
        AstIf* ثابت ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        إذا (!ifp || !ifp->elsesp()) أرجع خطأ;
        AstVarScope* thenVscp = خالي_اشارو;
        AstVarScope* elseVscp = خالي_اشارو;
        Ast_ڳنڍ* ثابت thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_ڳنڍ* ثابت elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        إذا (!thenNodep || !elseNodep) أرجع خطأ;
        إذا (!سڌو_Const_State_سونپيو_ڳنڍ(thenNodep, thenVscp, then_قدر)) أرجع خطأ;
        إذا (!سڌو_Const_State_سونپيو_ڳنڍ(elseNodep, elseVscp, else_قدر)) أرجع خطأ;
        إذا (thenVscp == stateVscp && elseVscp == stateVscp) أرجع سچ;
        إذا (thenVscp != elseVscp) أرجع خطأ;
        Ast_ڳنڍ* ثابت followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = خالي_اشارو;
        AstVarScope* finalFromVscp = خالي_اشارو;
        Ast_ڳنڍ* ثابت finalNodep = singleMeaningfulBranch(followp);
        إذا (!finalNodep) أرجع خطأ;
        إذا (!ڳنڍ_State_Var_سونپيو(finalNodep, finalStateVscp, finalFromVscp)) أرجع خطأ;
        إذا (finalStateVscp != stateVscp) أرجع خطأ;
        إذا (finalFromVscp != thenVscp) أرجع خطأ;
        أرجع سچ;
    }

    ساكن منطقي سڌو_State_Cond_Const_سونپيو(Ast_ڳنڍ* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_قدر& then_قدر, Fsm_State_قدر& else_قدر) {
        Ast_ڳنڍ_سونپيو* ثابت assp = سڌو_State_سونپيو(stmtp, stateVscp);
        إذا (!assp) أرجع خطأ;
        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
        إذا (!condp) أرجع خطأ;
        أرجع const_قدر_حالت(condp->thenp(), then_قدر) == Const_قدر_حالت::OK
               && const_قدر_حالت(condp->elsep(), else_قدر) == Const_قدر_حالت::OK;
    }

    ساكن Ast_ڳنڍ* case_شي_Supported_Arc_ڳنڍ(Ast_Case_شي* itemp, AstVarScope* stateVscp,
                                             منطقي inclCond) {
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع خالي_اشارو;
        }
        Ast_ڳنڍ_سونپيو* ثابت assp = سڌو_State_سونپيو(itemp->stmtsp(), stateVscp);
        إذا (assp) {
            Fsm_State_قدر منزل_قدر;
            إذا (const_قدر_حالت(assp->rhsp(), منزل_قدر) == Const_قدر_حالت::OK) أرجع assp;
        }
        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        إذا (سڌو_State_Cond_Const_سونپيو(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            أرجع assp;
        }
        إذا (if_State_Const_سونپيو(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            أرجع singleMeaningfulBranch(itemp->stmtsp());
        }
        أرجع خالي_اشارو;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ساكن Ast_ڳنڍ* case_Supported_Transition_ڳنڍ(AstCase* casep, AstVarScope* stateVscp,
                                                منطقي inclCond) {
        لكل (Ast_Case_شي* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شي)) {
            إذا (Ast_ڳنڍ* ثابت nodep = case_شي_Supported_Arc_ڳنڍ(itemp, stateVscp, inclCond))
                أرجع nodep;
        }
        أرجع خالي_اشارو;
    }

    ساكن Ast_ڳنڍ* case_شي_Supported_Arc_ڳنڍ_Like(Ast_ڳنڍ* stmtsp, AstVarScope* stateVscp) {
        إذا (Ast_ڳنڍ_سونپيو* ثابت assp = سڌو_State_سونپيو(stmtsp, stateVscp)) {
            Fsm_State_قدر منزل_قدر;
            إذا (const_قدر_حالت(assp->rhsp(), منزل_قدر) == Const_قدر_حالت::OK) أرجع assp;
            Fsm_State_قدر then_قدر;
            Fsm_State_قدر else_قدر;
            إذا (سڌو_State_Cond_Const_سونپيو(stmtsp, stateVscp, then_قدر, else_قدر)) أرجع assp;
        }
        أرجع خالي_اشارو;
    }

    ساكن Ast_ڳنڍ* if_Chain_Supported_Transition_ڳنڍ(ثابت FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        لكل (سائيز_t i = 0; i < chain.branches.سائيز(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            Ast_ڳنڍ* ثابت nodep = case_شي_Supported_Arc_ڳنڍ_Like(branch.stmtsp, stateVscp);
            إذا (!nodep) أرجع خالي_اشارو;
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

    ساكن Const_قدر_حالت const_قدر_حالت(Ast_ڳنڍ_اظهار* exprp, Fsm_State_قدر& قدر) {
        ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
        إذا (!constp) أرجع Const_قدر_حالت::NOT_CONST;
        ثابت V3_عدد& انگ = constp->انگ();
        إذا (انگ.isAnyXZ()) أرجع Const_قدر_حالت::XZ;
        قدر = Fsm_State_قدر{انگ};
        أرجع Const_قدر_حالت::OK;
    }

    ساكن منطقي خالص_State_Comparison_No_Alias(Ast_ڳنڍ_اظهار* exprp, FsmStateComparison& cmp) {
        AstEq* ثابت eqp = VN_CAST(exprp, Eq);
        إذا (!eqp) أرجع خطأ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_ڳنڍ_اظهار* valuep = eqp->rhsp();
        إذا (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->بنياد_Fromp(سچ), VarRef);
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

    ساكن منطقي خالص_State_Comparison(Ast_ڳنڍ_اظهار* exprp, ثابت FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        إذا (خالص_State_Comparison_No_Alias(exprp, cmp)) أرجع سچ;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        إذا (AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) {
            ثابت FsmAliasMap::const_iterator it = aliases.ڳوليو(vrefp->varScopep());
            إذا (it == aliases.پڇاڙي()) أرجع خطأ;
            cmp = it->ٻيو;
            أرجع سچ;
        }
        أرجع خطأ;
    }

    ساكن منطقي supported_چوٽي_Level_Guard(Ast_ڳنڍ_اظهار* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        إذا (VN_IS(exprp, Or)) أرجع خطأ;
        إذا (VN_IS(exprp, RedAnd)) أرجع خطأ;
        إذا (VN_IS(exprp, RedOr)) أرجع خطأ;
        إذا (VN_IS(exprp, RedXor)) أرجع خطأ;
        أرجع سچ;
    }

    ساكن منطقي resolveIfPredicate(Ast_ڳنڍ_اظهار* exprp, ثابت FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, منطقي& hasGuard) {
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> terms;
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> pending;
        pending.ڌڪو_پٺ(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        طالما (!pending.خالي_جايون()) {
            Ast_ڳنڍ_اظهار* ثابت nodep = pending.پٺ();
            pending.ڪڍو_پٺ();
            إذا (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
                pending.ڌڪو_پٺ(andp->rhsp());
                pending.ڌڪو_پٺ(andp->lhsp());
            } وإلا {
                terms.ڌڪو_پٺ(nodep);
            }
        }

        منطقي sawComparison = خطأ;
        لكل (سائيز_t i = 0; i < terms.سائيز(); ++i) {
            Ast_ڳنڍ_اظهار* ثابت termp = terms[i];
            FsmStateComparison رڪن_Cmp;
            إذا (خالص_State_Comparison(termp, aliases, رڪن_Cmp /*ref*/)) {
                إذا (sawComparison) أرجع خطأ;
                cmp = رڪن_Cmp;
                sawComparison = سچ;
                استمر;
            }
            إذا (!supported_چوٽي_Level_Guard(termp)) أرجع خطأ;
            hasGuard = سچ;
        }
        أرجع sawComparison;
    }

    ساكن فراغ جوڙيو_Alias(FsmAliasMap& aliases, std::unordered_سيٽ<ثابت AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ثابت FsmStateComparison& cmp) {
        إذا (ambiguous.ڳوليو(aliasVscp) != ambiguous.پڇاڙي()) أرجع;
        ثابت FsmAliasMap::iterator it = aliases.ڳوليو(aliasVscp);
        إذا (it == aliases.پڇاڙي()) {
            aliases.emplace(aliasVscp, cmp);
            أرجع;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        إذا (it->ٻيو.stateVscp == cmp.stateVscp && it->ٻيو.قدر == cmp.قدر) أرجع;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        أرجع;
    }

    ساكن فراغ collect_Alias_From_سونپيو(Ast_ڳنڍ_سونپيو* assp, FsmAliasMap& aliases,
                                       std::unordered_سيٽ<ثابت AstVarScope*>& ambiguous) {
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!lhsp) أرجع;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        إذا (!خالص_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) أرجع;
        جوڙيو_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_ڳنڍ* stmtsp, Ast_ڳنڍ* limitp) ثابت {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_سيٽ<ثابت AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        لكل (Ast_ڳنڍ* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            إذا (Ast_ڳنڍ_سونپيو* ثابت assp = VN_CAST(nodep, ڳنڍ_سونپيو)) {
                collect_Alias_From_سونپيو(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        لكل (ثابت AstVarScope* ثابت vscp : ambiguous) aliases.erase(vscp);
        أرجع aliases;
    }

    ساكن منطقي collectIfChain(AstIf* ifp, ثابت FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_سيٽ<string> ڏٺل_قدرون;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        طالما (سچ) {
            FsmStateComparison cmp;
            منطقي hasGuard = خطأ;
            إذا (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) أرجع خطأ;
            إذا (chain.compareVscp && chain.compareVscp != cmp.stateVscp) أرجع خطأ;
            إذا (!ڏٺل_قدرون.وجهو(cmp.قدر.ڪنجي()).ٻيو) أرجع خطأ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ڌڪو_پٺ(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.قدر, hasGuard});

            Ast_ڳنڍ* ثابت elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            إذا (!elseNodep) اكسر;
            إذا (AstIf* ثابت elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                استمر;
            }
            chain.defaultStmtsp = elseNodep;
            اكسر;
        }
        أرجع chain.branches.سائيز() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    ساكن منطقي validate_معلوم_State_قدر(Ast_ڳنڍ* nodep, ثابت FsmStateSpace& stateSpace,
                                        ثابت Fsm_State_قدر& قدر, ثابت string& role) {
        إذا (stateSpace.labels.ڳوليو(قدر) != stateSpace.labels.پڇاڙي()) أرجع سچ;
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
        إذا (!name.خالي_جايون()) أرجع StateConstLabel{Ast_ڳنڍ::prettyName(name), سچ, 0};
        أرجع StateConstLabel{constp->name(), خطأ, 0};
    }

    ساكن فراغ updateStateLabel(FsmStateSpace& stateSpace, ثابت Fsm_State_قدر& قدر,
                                 ثابت StateConstLabel& label) {
        stateSpace.حالتون.at(stateSpace.labels.at(قدر).state_اشاريو).پهريون = label.متن;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ساكن منطقي ميلاپ_رجسٽر_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_رجسٽر_Candidate& cand) {
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->has_ڪنارو()) أرجع خطأ;

        Ast_ڳنڍ* ثابت stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_ڳنڍ* ثابت nodep = singleMeaningfulStmt(stmtsp);
        إذا (!nodep) أرجع خطأ;

        AstVarScope* stateVscp = خالي_اشارو;
        AstVarScope* ايندڙ_Vscp = خالي_اشارو;
        إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) أرجع خطأ;
            AstVarScope* resetStateVscp = خالي_اشارو;
            ثابت Reset_سونپيو_حالت reset_حالت
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ثابت منطقي خالي_جايون_Reset_Branch = reset_حالت == Reset_سونپيو_حالت::خالي_جايون_3;
            إذا (خالي_جايون_Reset_Branch) {
                cand.resetArcs().clear();
            } وإلا إذا (reset_حالت == Reset_سونپيو_حالت::NONE) {
                cand.resetArcs().clear();
                Fsm_State_قدر reset_قدر;
                Ast_ڳنڍ* ثابت thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_ڳنڍ_سونپيو* ثابت resetAssp
                    = سڌو_Const_State_سونپيو_ڳنڍ(thenNodep, resetStateVscp, reset_قدر);
                إذا (!resetAssp) أرجع خطأ;
                cand.resetArcs().emplace_پٺ(reset_قدر, resetAssp);
            } وإلا إذا (reset_حالت == Reset_سونپيو_حالت::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_ڳنڍ* ثابت elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            إذا (!ڳنڍ_State_Var_سونپيو(elseNodep, stateVscp, ايندڙ_Vscp)) أرجع خطأ;
            إذا (!خالي_جايون_Reset_Branch && resetStateVscp != stateVscp) أرجع خطأ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != خالي_اشارو);
        } وإلا {
            Ast_ڳنڍ_اظهار* resetCondp = خالي_اشارو;
            Fsm_State_قدر reset_قدر;
            // Note the active low 'reset ? next : CONST' form is not accepted
            إذا (Ast_ڳنڍ_سونپيو* ثابت assp
                = سڌو_Cond_State_Var_سونپيو(nodep, stateVscp, ايندڙ_Vscp, resetCondp, reset_قدر)) {
                cand.resetArcs().emplace_پٺ(reset_قدر, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != خالي_اشارو);
            } وإلا إذا (!ڳنڍ_State_Var_سونپيو(nodep, stateVscp, ايندڙ_Vscp)) {
                أرجع خطأ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.ايندڙ_Vscp(ايندڙ_Vscp);
        cand.senses() = describe_Sen_وڻ(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        أرجع سچ;
    }

    ساكن منطقي جوڙيو_قدر_منزل_State_Space(Ast_ڳنڍ* nodep, FsmStateSpace& stateSpace,
                                     ثابت Fsm_State_قدر& قدر, StateConstLabel label) {
        ثابت خودڪار labelIt = stateSpace.labels.ڳوليو(قدر);
        إذا (labelIt != stateSpace.labels.پڇاڙي()) {
            StateConstLabel& existingLabel = labelIt->ٻيو;
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
            storedLabel.state_اشاريو = stateSpace.حالتون.سائيز();
            stateSpace.حالتون.emplace_پٺ(label.متن, قدر);
            stateSpace.labels.emplace(قدر, storedLabel);
        }
        أرجع سچ;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ساكن منطقي جوڙيو_اظهار_منزل_State_Space(Ast_ڳنڍ_اظهار* valuep, FsmStateSpace& stateSpace) {
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
        أرجع جوڙيو_قدر_منزل_State_Space(valuep, stateSpace, قدر, stateLabelForConst(constp));
    }

    ساكن منطقي جوڙيو_Optional_هدف_اظهار_منزل_State_Space(Ast_ڳنڍ_اظهار* valuep, FsmStateSpace& stateSpace) {
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
        أرجع جوڙيو_قدر_منزل_State_Space(valuep, stateSpace, قدر, stateLabelForConst(constp));
    }

    ساكن فراغ جوڙيو_Reset_Targets_منزل_State_Space(ثابت std::ويڪٽر<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.منزل_قدر().ascii(), خطأ, 0};
            إذا (AstConst* ثابت constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                جوڙيو_قدر_منزل_State_Space(resetArc.nodep(), stateSpace, resetArc.منزل_قدر(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ساكن منطقي collectDeclaredStateSpace(Ast_ڳنڍ* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, منطقي& needs_ذريعو_قدرون) {
        AstVar* ثابت stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        إذا (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ثابت منطقي forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        إذا (enump) {
            stateSpace.enumBacked = سچ;
            لكل (Ast_Enum_شي* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_شي)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                ثابت Fsm_State_قدر قدر{constp->انگ()};
                ثابت سائيز_t state_اشاريو = stateSpace.حالتون.سائيز();
                stateSpace.حالتون.emplace_پٺ(itemp->name(), قدر);
                stateSpace.labels.emplace(قدر,
                                          StateConstLabel{itemp->name(), خطأ, state_اشاريو});
            }
            أرجع stateSpace.حالتون.سائيز() >= 2;
        }

        إذا (forced) {
            needs_ذريعو_قدرون = سچ;
            أرجع سچ;
        }

        needs_ذريعو_قدرون = سچ;
        أرجع سچ;
    }

    سانچو <قسم_نالو T_ValuepVisitor>
    ساكن منطقي collect_State_Space_From_قدرون(Ast_ڳنڍ* warnNodep, AstVarScope* stateVscp,
                                            ثابت std::ويڪٽر<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ثابت T_ValuepVisitor& گهمو_Valueps) {
        منطقي needs_ذريعو_قدرون = خطأ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        إذا (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_ذريعو_قدرون)) {
            أرجع خطأ;
        }
        إذا (!needs_ذريعو_قدرون) أرجع سچ;
        جوڙيو_Reset_Targets_منزل_State_Space(resetArcs, stateSpace);
        إذا (!گهمو_Valueps(
                [&](Ast_ڳنڍ_اظهار* valuep) { أرجع جوڙيو_اظهار_منزل_State_Space(valuep, stateSpace); })) {
            أرجع خطأ;
        }
        أرجع stateSpace.حالتون.سائيز() >= 2;
    }

    ساكن منطقي collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* سونپيو_Vscp,
                                  ثابت std::ويڪٽر<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_قدرون(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, سونپيو_Vscp, &stateSpace](ثابت خودڪار& گهمو_Valuep) {
                لكل (Ast_Case_شي* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Case_شي)) {
                    إذا (!itemp->isDefault()) {
                        لكل (Ast_ڳنڍ_اظهار* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), ڳنڍ_اظهار)) {
                            إذا (!گهمو_Valuep(condp)) أرجع خطأ;
                        }
                    }
                    إذا (Ast_ڳنڍ_سونپيو* ثابت assp
                        = سڌو_State_سونپيو(itemp->stmtsp(), سونپيو_Vscp)) {
                        Fsm_State_قدر then_قدر;
                        Fsm_State_قدر else_قدر;
                        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                        إذا (condp
                            && سڌو_State_Cond_Const_سونپيو(itemp->stmtsp(), سونپيو_Vscp, then_قدر,
                                                          else_قدر)) {
                            إذا (!گهمو_Valuep(condp->thenp())) أرجع خطأ;
                            إذا (!گهمو_Valuep(condp->elsep())) أرجع خطأ;
                        } وإلا إذا (!جوڙيو_Optional_هدف_اظهار_منزل_State_Space(assp->rhsp(), stateSpace)) {
                            أرجع خطأ;
                        }
                    }
                }
                أرجع سچ;
            });
    }

    ساكن منطقي collectStateSpace(ثابت FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* سونپيو_Vscp,
                                  ثابت std::ويڪٽر<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_قدرون(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, سونپيو_Vscp, &stateSpace](ثابت خودڪار& گهمو_Valuep) {
                لكل (ثابت FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(گهمو_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_ڳنڍ_سونپيو* ثابت assp = سڌو_State_سونپيو(branch.stmtsp, سونپيو_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_قدر then_قدر;
                    Fsm_State_قدر else_قدر;
                    AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                    إذا (condp) {
                        UASSERT_OBJ(سڌو_State_Cond_Const_سونپيو(branch.stmtsp, سونپيو_Vscp,
                                                               then_قدر, else_قدر),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        إذا (!گهمو_Valuep(condp->thenp())) أرجع خطأ;
                        إذا (!گهمو_Valuep(condp->elsep())) أرجع خطأ;
                    } وإلا إذا (!جوڙيو_Optional_هدف_اظهار_منزل_State_Space(assp->rhsp(), stateSpace)) {
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
    ساكن منطقي emit_Case_شي_Arcs(Fsm_گراف& گراف, Ast_Case_شي* itemp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, منطقي inclCond) {
        std::ويڪٽر<std::pair<string, Fsm_State_قدر>> froms;
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع خطأ;
            froms.emplace_پٺ("default", Fsm_State_قدر{});
        } وإلا {
            لكل (Ast_ڳنڍ_اظهار* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), ڳنڍ_اظهار)) {
                Fsm_State_قدر قدر;
                إذا (const_قدر_حالت(condp, قدر) != Const_قدر_حالت::OK) استمر;
                إذا (!validate_معلوم_State_قدر(condp, stateSpace, قدر, "source")) أرجع سچ;
                froms.emplace_پٺ(label_For_قدر(stateSpace.labels, قدر), قدر);
            }
            إذا (froms.خالي_جايون()) أرجع خطأ;
        }

        إذا (Ast_ڳنڍ_سونپيو* ثابت assp = سڌو_State_سونپيو(itemp->stmtsp(), stateVscp)) {
            Fsm_State_قدر منزل_قدر;
            ثابت Const_قدر_حالت حالت = const_قدر_حالت(assp->rhsp(), منزل_قدر);
            إذا (حالت == Const_قدر_حالت::OK) {
                إذا (!validate_معلوم_State_قدر(assp, stateSpace, منزل_قدر, "target")) أرجع سچ;
                لكل (ثابت std::pair<string, Fsm_State_قدر>& from : froms) {
                    گراف.جوڙيو_Arc(from.ٻيو, منزل_قدر, خطأ, خطأ, itemp->isDefault(),
                                 assp->fileline());
                }
                أرجع سچ;
            }
        }

        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        إذا (سڌو_State_Cond_Const_سونپيو(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)
            || if_State_Const_سونپيو(itemp->stmtsp(), stateVscp, then_قدر, else_قدر)) {
            إذا (!validate_معلوم_State_قدر(itemp->stmtsp(), stateSpace, then_قدر, "target"))
                أرجع سچ;
            إذا (!validate_معلوم_State_قدر(itemp->stmtsp(), stateSpace, else_قدر, "target"))
                أرجع سچ;
            لكل (ثابت Fsm_State_قدر& branch_قدر : {then_قدر, else_قدر}) {
                لكل (ثابت std::pair<string, Fsm_State_قدر>& from : froms) {
                    گراف.جوڙيو_Arc(from.ٻيو, branch_قدر, خطأ, سچ, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            أرجع سچ;
        }

        أرجع خطأ;
    }

    ساكن فراغ emitStmtArcsFrom(Fsm_گراف& گراف, Ast_ڳنڍ* stmtsp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, Fsm_State_قدر from_قدر,
                                 منطقي isDefault, منطقي forceCond) {
        Ast_ڳنڍ_سونپيو* ثابت assp = سڌو_State_سونپيو(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_قدر منزل_قدر;
        ثابت Const_قدر_حالت حالت = const_قدر_حالت(assp->rhsp(), منزل_قدر);
        إذا (حالت == Const_قدر_حالت::OK) {
            إذا (!validate_معلوم_State_قدر(assp, stateSpace, منزل_قدر, "target")) أرجع;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            گراف.جوڙيو_Arc(from_قدر, منزل_قدر, خطأ, forceCond, isDefault, assp->fileline());
            أرجع;
        }

        Fsm_State_قدر then_قدر;
        Fsm_State_قدر else_قدر;
        ثابت منطقي cond_سونپيو
            = سڌو_State_Cond_Const_سونپيو(stmtsp, stateVscp, then_قدر, else_قدر);
        UASSERT_OBJ(cond_سونپيو, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        إذا (!validate_معلوم_State_قدر(stmtsp, stateSpace, then_قدر, "target")) أرجع;
        إذا (!validate_معلوم_State_قدر(stmtsp, stateSpace, else_قدر, "target")) أرجع;
        لكل (ثابت Fsm_State_قدر& branch_قدر : {then_قدر, else_قدر}) {
            گراف.جوڙيو_Arc(from_قدر, branch_قدر, خطأ, سچ, isDefault, stmtsp->fileline());
        }
    }

    ساكن فراغ emitIfChainArcs(Fsm_گراف& گراف, ثابت FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ثابت FsmStateSpace& stateSpace) {
        لكل (سائيز_t i = 0; i < chain.branches.سائيز(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            إذا (!validate_معلوم_State_قدر(branch.ifp, stateSpace, branch.from_قدر, "source"))
                أرجع;
            emitStmtArcsFrom(گراف, branch.stmtsp, stateVscp, stateSpace, branch.from_قدر, خطأ,
                             branch.has_چوٽي_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ساكن فراغ جوڙيو_Reset_Arcs(Fsm_گراف& گراف, ثابت std::ويڪٽر<FsmResetArcDesc>& resetArcs,
                             ثابت FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            إذا (!validate_معلوم_State_قدر(resetArc.nodep(), stateSpace, resetArc.منزل_قدر(),
                                         "target"))
                استمر;
            گراف.جوڙيو_Arc(Fsm_State_قدر{}, resetArc.منزل_قدر(), سچ, خطأ, خطأ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    فراغ processCase(AstCase* casep, AstVarScope* سونپيو_Vscp, ثابت Fsm_رجسٽر_Candidate& reg) {
        UASSERT_OBJ(سونپيو_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        إذا (!collectStateSpace(casep, stateVscp, سونپيو_Vscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& داخلا_2 = m_state.fsmFor(stateVscp);
        إذا (!داخلا_2.graphp) {
            داخلا_2.graphp.reset(جديد Fsm_گراف{});
            داخلا_2.graphp->scopep(reg.scopep());
            داخلا_2.graphp->stateAlwaysp(reg.alwaysp());
            داخلا_2.graphp->stateVarName(stateVscp->prettyName());
            داخلا_2.graphp->state_Var_اندروني_Name(stateVscp->varp()->name());
            داخلا_2.graphp->stateVarScopep(stateVscp);
            داخلا_2.graphp->sampleVarScopep(reg.sampleVscp());
            داخلا_2.graphp->senses() = reg.senses();
            داخلا_2.graphp->resetCond() = reg.resetCond();
            داخلا_2.graphp->hasResetCond(reg.hasResetCond());
            داخلا_2.graphp->inclCond(reg.inclCond());
            داخلا_2.graphp->fileline(casep->fileline());
            لكل (ثابت std::pair<string, Fsm_State_قدر>& state : stateSpace.حالتون) {
                داخلا_2.graphp->جوڙيو_State_راس(state.پهريون, state.ٻيو);
            }
            جوڙيو_Reset_Arcs(*داخلا_2.graphp, reg.resetArcs(), stateSpace);
        }
        لكل (Ast_Case_شي* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_شي)) {
            emit_Case_شي_Arcs(*داخلا_2.graphp, itemp, سونپيو_Vscp, stateSpace,
                             داخلا_2.graphp->inclCond());
        }
    }

    فراغ processIfChain(ثابت FsmIfChainCandidate& chain, AstVarScope* سونپيو_Vscp,
                        ثابت Fsm_رجسٽر_Candidate& reg) {
        UASSERT_OBJ(سونپيو_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        إذا (!collectStateSpace(chain, stateVscp, سونپيو_Vscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& داخلا_2 = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!داخلا_2.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        داخلا_2.graphp.reset(جديد Fsm_گراف{});
        داخلا_2.graphp->scopep(reg.scopep());
        داخلا_2.graphp->stateAlwaysp(reg.alwaysp());
        داخلا_2.graphp->stateVarName(stateVscp->prettyName());
        داخلا_2.graphp->state_Var_اندروني_Name(stateVscp->varp()->name());
        داخلا_2.graphp->stateVarScopep(stateVscp);
        داخلا_2.graphp->sampleVarScopep(reg.sampleVscp());
        داخلا_2.graphp->senses() = reg.senses();
        داخلا_2.graphp->resetCond() = reg.resetCond();
        داخلا_2.graphp->hasResetCond(reg.hasResetCond());
        داخلا_2.graphp->inclCond(reg.inclCond());
        داخلا_2.graphp->fileline(chain.ifp->fileline());
        لكل (ثابت std::pair<string, Fsm_State_قدر>& state : stateSpace.حالتون) {
            داخلا_2.graphp->جوڙيو_State_راس(state.پهريون, state.ٻيو);
        }
        جوڙيو_Reset_Arcs(*داخلا_2.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*داخلا_2.graphp, chain, سونپيو_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    فراغ process_One_بلاڪ_Always(ثابت FsmComboAlways& one_بلاڪ) {
        ثابت رجسٽر_Always_Analyzer analyzer{one_بلاڪ.scopep()};
        AstAlways* ثابت alwaysp = one_بلاڪ.alwaysp();
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->has_ڪنارو()) أرجع;
        ثابت std::ويڪٽر<std::pair<AstCase*, Ast_ڳنڍ_اظهار*>> candidates
            = analyzer.one_بلاڪ_Candidates(alwaysp);

        FsmCaseCandidate پهريون_Cand;
        لكل (ثابت std::pair<AstCase*, Ast_ڳنڍ_اظهار*>& cand : candidates) {
            AstVarRef* ثابت selp = VN_CAST(cand.پهريون->exprp(), VarRef);
            AstVarScope* ثابت vscp = selp ? selp->varScopep() : خالي_اشارو;
            إذا (!vscp) استمر;
            إذا (!پهريون_Cand.stateVscp) {
                پهريون_Cand.warnNodep = cand.پهريون;
                پهريون_Cand.stateVscp = vscp;
                Fsm_رجسٽر_Candidate reg;
                analyzer.build_One_بلاڪ_Candidate(alwaysp, vscp, cand.ٻيو, reg);
                processCase(cand.پهريون, vscp, reg);
            } وإلا إذا (vscp != پهريون_Cand.stateVscp) {
                cand.پهريون->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.پهريون, پهريون_Cand));
            } وإلا {
                cand.پهريون->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.پهريون, پهريون_Cand));
            }
        }

        إذا (پهريون_Cand.stateVscp) أرجع;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ثابت std::ويڪٽر<std::pair<AstIf*, Ast_ڳنڍ_اظهار*>> ifCandidates
            = analyzer.one_بلاڪ_If_Candidates(alwaysp);
        لكل (ثابت std::pair<AstIf*, Ast_ڳنڍ_اظهار*>& cand : ifCandidates) {
            ثابت FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.پهريون);
            FsmIfChainCandidate chain;
            إذا (!collectIfChain(cand.پهريون, aliases, chain)) استمر;
            AstVarScope* ثابت vscp = chain.compareVscp;
            إذا (!if_Chain_Supported_Transition_ڳنڍ(chain, vscp)) استمر;
            إذا (!پهريون_Cand.stateVscp) {
                پهريون_Cand.warnNodep = cand.پهريون;
                پهريون_Cand.stateVscp = vscp;
                Fsm_رجسٽر_Candidate reg;
                analyzer.build_One_بلاڪ_Candidate(alwaysp, vscp, cand.ٻيو, reg);
                processIfChain(chain, vscp, reg);
            } وإلا إذا (vscp != پهريون_Cand.stateVscp) {
                cand.پهريون->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.پهريون, پهريون_Cand));
            } وإلا {
                cand.پهريون->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.پهريون, پهريون_Cand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    ساكن فراغ warnComboSameAlways(Ast_ڳنڍ* warnNodep, ثابت FsmCaseCandidate& پهريون_Cand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, پهريون_Cand));
    }

    فراغ processComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_رجسٽر_Candidates};
        Ast_ڳنڍ* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate پهريون_Cand;
        لكل (Ast_ڳنڍ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            ثابت ComboAlwaysAnalyzer::Combo_ميلاپ ميلاپ = analyzer.ميلاپ_Case(stmtsp, casep);
            ثابت Fsm_رجسٽر_Candidate* ثابت matchedp = ميلاپ.matchedp;
            Ast_ڳنڍ* ثابت matchedWarnNodep = ميلاپ.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!پهريون_Cand.stateVscp) {
                ثابت خودڪار وجهو_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!وجهو_Pair.ٻيو) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  وجهو_Pair.پهريون->ٻيو));
                    استمر;
                }
                پهريون_Cand.warnNodep = matchedWarnNodep;
                پهريون_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->ايندڙ_Vscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != پهريون_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, پهريون_Cand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, پهريون_Cand));
        }
        إذا (پهريون_Cand.stateVscp) أرجع;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        لكل (Ast_ڳنڍ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ثابت ifp = VN_CAST(nodep, If);
            إذا (!ifp) استمر;
            FsmIfChainCandidate chain;
            ثابت FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            إذا (!collectIfChain(ifp, aliases, chain)) استمر;
            ثابت ComboAlwaysAnalyzer::Combo_ميلاپ ميلاپ = analyzer.ميلاپ_If_Chain(stmtsp, chain);
            ثابت Fsm_رجسٽر_Candidate* ثابت matchedp = ميلاپ.matchedp;
            Ast_ڳنڍ* ثابت matchedWarnNodep = ميلاپ.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!پهريون_Cand.stateVscp) {
                ثابت std::pair<std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate>::iterator,
                                منطقي>
                    وجهو_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!وجهو_Pair.ٻيو) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  وجهو_Pair.پهريون->ٻيو));
                    استمر;
                }
                پهريون_Cand.warnNodep = matchedWarnNodep;
                پهريون_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->ايندڙ_Vscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != پهريون_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, پهريون_Cand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, پهريون_Cand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    فراغ گهمو(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    فراغ گهمو(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_سونپيو) {
            iterateChildren(nodep);
            أرجع;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_بلاڪ_Alwayss.emplace_پٺ(m_scopep, nodep);
        ثابت رجسٽر_Always_Analyzer analyzer{m_scopep};
        Fsm_رجسٽر_Candidate reg;
        إذا (analyzer.ميلاپ_رجسٽر_Candidate(nodep, reg)) {
            AstVarScope* ثابت stateVscp = reg.stateVscp();
            ثابت منطقي مليو
                = std::any_جو(m_رجسٽر_Candidates.cbegin(), m_رجسٽر_Candidates.cend(),
                              [stateVscp](ثابت Fsm_رجسٽر_Candidate& existing) {
                                  أرجع existing.stateVscp() == stateVscp;
                              });
            إذا (!مليو) { m_رجسٽر_Candidates.emplace_پٺ(reg); }
        }
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_پٺ(m_scopep, nodep);
        } وإلا إذا (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            إذا (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_پٺ(m_scopep, nodep);
            } وإلا {
                m_nonComboAlwayss.emplace_پٺ(m_scopep, nodep);
            }
        }
    }

    فراغ گهمو(Ast_سونپيو_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_سونپيو(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_پورٽ_Alias(nodep);
        iterateChildren(nodep);
    }

    فراغ گهمو(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        إذا (m_scopep && fsm_رجسٽر_Wrapper_Desc(nodep)) جوڙيو_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        لكل (ثابت std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_رجسٽر_Candidate reg;
            إذا (ميلاپ_Fsm_Wrapper_Cell(wrapperCell.پهريون, wrapperCell.ٻيو, reg)) {
                m_رجسٽر_Candidates.emplace_پٺ(reg);
            }
        }
        لكل (ثابت FsmComboAlways& one_بلاڪ : m_one_بلاڪ_Alwayss) process_One_بلاڪ_Always(one_بلاڪ);
        لكل (ثابت FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        لكل (ثابت FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
صنف FsmCoverIncBuilder final {
    ثابت Fsm_گراف& m_گراف;  // Fsm graph
    std::ويڪٽر<ثابت Fsm_State_راس*> m_حالتون;  // States vertexes inside fsm graph
    std::unordered_سيٽ<uint64_t>
        m_undefined_حالتون;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ثابت Fsm_راس* m_defaultVertexp = خالي_اشارو;  // Default vertex
    ثابت Fsm_راس* m_resetVertexp = خالي_اشارو;  // Reset vertex
    سائيز_t m_ڪنارا_ڳڻپ = 0;  // Total count of edges
    AstVarScope* ثابت m_prevVscp;  // Previous state of fsm
    File_سٽ* ثابت m_flp;  // Fileline pointer to use across the builder
    ثابت std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_ڳنڍ_Stmt* m_coverIncp = خالي_اشارو;  // Increments proper CoverDecl after each change
    Ast_ڳنڍ_Stmt* m_initialCoverIncp = خالي_اشارو;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = خالي_اشارو;  // List of CoverDecls
    منطقي m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ثابت uint64_t قدر) {
        V3_عدد انگ{static_cast<Ast_ڳنڍ*>(خالي_اشارو), m_prevVscp->ويڪر()};
        انگ.سيٽ_Quad(قدر);
        انگ.isSigned(m_prevVscp->dtypep()->isSigned());
        أرجع جديد AstConst{m_flp, انگ};
    }

    std::string quad_منزل_ٻٽو_انگ_Stirng_Label(ثابت uint64_t val) {
        صحيح ويڪر = m_prevVscp->ويڪر();
        UASSERT(ويڪر < 64, "We should not expand values that wide");
        std::string نتيجو = std::منزل_string(ويڪر) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        طالما (--ويڪر >= 0) نتيجو.ڌڪو_پٺ("01"[(val >> ويڪر) & 1]);
        أرجع نتيجو;
    }

    فراغ init_Existing_حالتون() {
        لكل (ثابت V3_گراف_راس& vtx : m_گراف.vertices()) {
            ثابت Fsm_راس* ثابت vertexp = vtx.as<Fsm_راس>();
            m_ڪنارا_ڳڻپ += vertexp->اندريون_ڪنارا().سائيز();
            إذا (ثابت Fsm_State_راس* ثابت statep = vertexp->cast<Fsm_State_راس>()) {
                m_حالتون.ڌڪو_پٺ(statep);
            } وإلا إذا (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_گراف.sampleVarScopep(),
                            "Two defaults?");
            } وإلا /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_گراف.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ثابت صحيح ويڪر = m_گراف.sampleVarScopep()->ويڪر();
        m_isComplete = ويڪر < 64 && m_حالتون.سائيز() == (static_cast<سائيز_t>(1) << ويڪر);
    }

    فراغ init_Undefined_حالتون() {
        UASSERT_OBJ(m_گراف.sampleVarScopep()->ويڪر() < 64, m_گراف.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ثابت uint64_t combinations_ڳڻپ = static_cast<سائيز_t>(1)
                                           << m_گراف.sampleVarScopep()->ويڪر();
        // Rolling trick allows to cut peak memory usage
        منطقي undefRolled = خطأ;
        خودڪار rollUndefs = [هذا, combinations_ڳڻپ] {
            لكل (uint64_t i = 0; i < combinations_ڳڻپ; ++i) {
                ثابت خودڪار وجهو = m_undefined_حالتون.وجهو(i);
                إذا (!وجهو.ٻيو) m_undefined_حالتون.erase(وجهو.پهريون);
            }
        };
        لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتون) {
            ثابت V3_عدد& انگ = statep->قدر().انگ();
            ثابت uint64_t قدر = انگ.منزل_UQuad();
            إذا (!m_isComplete) {
                إذا (undefRolled) {
                    UASSERT_OBJ(m_undefined_حالتون.erase(قدر) == 1, m_گراف.sampleVarScopep(),
                                "Two states has the same value?");
                } وإلا {
                    UASSERT_OBJ(m_undefined_حالتون.وجهو(قدر).ٻيو, m_گراف.sampleVarScopep(),
                                "Two states has the same value?");
                    إذا (m_undefined_حالتون.سائيز() * 2 >= combinations_ڳڻپ) {
                        undefRolled = سچ;
                        rollUndefs();
                    }
                }
            }
        }
        إذا (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_ڳنڍ_اظهار* buildResetCond() ثابت {
        Ast_ڳنڍ_اظهار* ثابت refp
            = جديد AstVarRef{m_flp, m_گراف.resetCond().varScopep, VAccess::READ};
        أرجع m_گراف.resetCond().سرگرم_هيٺيون
                   ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstLogNot{m_flp, refp})
                   : refp;
    }

    سائيز_t get_Arc_ڳڻپ_With_Expansion(ثابت VFsmExpandType expand) ثابت {
        سائيز_t arc_ڳڻپ_With_Expansion = ~0ULL;
        بدل (expand) {
        حالة VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_گراف.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            اكسر;
        }
        حالة VFsmExpandType::AUTO: {
            arc_ڳڻپ_With_Expansion = m_ڪنارا_ڳڻپ;
            إذا (m_defaultVertexp) {
                UASSERT_OBJ(m_حالتون.سائيز() >= m_defaultVertexp->ٻاهريون_ڪنارا().سائيز(),
                            m_گراف.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_ڳڻپ_With_Expansion -= m_defaultVertexp->ٻاهريون_ڪنارا().سائيز();
                لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتون) {
                    arc_ڳڻپ_With_Expansion += static_cast<سائيز_t>(statep->ٻاهريون_ڪنارا().خالي_جايون())
                                             * m_defaultVertexp->ٻاهريون_ڪنارا().سائيز();
                }
            }
            إذا (m_resetVertexp) {
                UASSERT_OBJ(m_حالتون.سائيز() >= m_resetVertexp->ٻاهريون_ڪنارا().سائيز(),
                            m_گراف.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_ڳڻپ_With_Expansion += m_حالتون.سائيز() - m_resetVertexp->ٻاهريون_ڪنارا().سائيز();
            }
            اكسر;
        }
        حالة VFsmExpandType::AUTO_EXPAND: {
            arc_ڳڻپ_With_Expansion = m_حالتون.سائيز() * m_حالتون.سائيز() - m_حالتون.سائيز();
            اكسر;
        }
        حالة VFsmExpandType::ڀريل: {
            ثابت صحيح ويڪر = m_گراف.sampleVarScopep()->ويڪر();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            إذا (ويڪر < 32) {
                ثابت uint64_t combinations_ڳڻپ = static_cast<سائيز_t>(1) << ويڪر;
                arc_ڳڻپ_With_Expansion = combinations_ڳڻپ * combinations_ڳڻپ - combinations_ڳڻپ;
            }
            اكسر;
        }
        }
        أرجع arc_ڳڻپ_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ثابت std::string& label,
                                           ثابت std::string& fsm_ٽيگ = "") {
        AstCoverOtherDecl* ثابت declp
            = جديد AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_گراف.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_گراف.stateVarName(),
                                    "",
                                    label,
                                    fsm_ٽيگ};
        // We run after scoping, so this is declared for one instance only
        declp->في_Instance(سچ);
        m_declsp = Ast_ڳنڍ::جوڙيو_ايندڙ(m_declsp, declp);
        أرجع declp;
    }

    Ast_ڳنڍ_Stmt* buildCoverIncStmtp(ثابت std::string& fromStateLabel,
                                    ثابت std::string& منزل_State_Label,
                                    ثابت std::string& بنياد_Label = "") {
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
            بنياد_Label};
        // We run after scoping, so this is declared for one instance only
        declp->في_Instance(سچ);
        m_declsp = Ast_ڳنڍ::جوڙيو_ايندڙ(m_declsp, declp);
        أرجع جديد AstCoverInc{m_flp, declp};
    }
    Ast_ڳنڍ_Stmt* buildExpandedTransitionHandler(ثابت std::string& منزل_State_Label,
                                                ثابت منطقي considerUndefs,
                                                ثابت منطقي expandOnlyWithTerminalVerticies,
                                                ثابت Fsm_State_راس* ثابت منزل_Statep = خالي_اشارو) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, خالي_اشارو};
        std::unordered_سيٽ<ثابت Fsm_State_راس*> handledSoureces;
        إذا (منزل_Statep) {
            لكل (ثابت V3_گراف_ڪنارو& ڪنارو : منزل_Statep->اندريون_ڪنارا()) {
                إذا (ثابت Fsm_State_راس* ثابت fromp = ڪنارو.fromp()->cast<Fsm_State_راس>()) {
                    إذا (!handledSoureces.emplace(fromp).ٻيو) استمر;
                    arcCasep->جوڙيو_Itemsp(جديد Ast_Case_شي{
                        m_flp, makeConst(fromp->قدر().انگ().منزل_UQuad()),
                        (fromp != منزل_Statep) ? buildCoverIncStmtp(fromp->label(), منزل_State_Label)
                                            : خالي_اشارو});
                }
            }
        }
        لكل (ثابت Fsm_State_راس* ثابت fromp : m_حالتون) {
            إذا (handledSoureces.ڳوليو(fromp) != handledSoureces.پڇاڙي()) استمر;
            arcCasep->جوڙيو_Itemsp(جديد Ast_Case_شي{
                m_flp, makeConst(fromp->قدر().انگ().منزل_UQuad()),
                (fromp != منزل_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->ٻاهريون_ڪنارا().خالي_جايون()))
                    ? buildCoverIncStmtp(fromp->label(), منزل_State_Label, "artificial")
                    : خالي_اشارو});
        }
        إذا (!m_isComplete) {
            إذا (considerUndefs) {
                لكل (ثابت uint64_t from : m_undefined_حالتون) {
                    ثابت std::string& label = quad_منزل_ٻٽو_انگ_Stirng_Label(from);
                    arcCasep->جوڙيو_Itemsp(
                        جديد Ast_Case_شي{m_flp, makeConst(from),
                                        (label != منزل_State_Label)
                                            ? buildCoverIncStmtp(label, منزل_State_Label, "artificial")
                                            : خالي_اشارو});
                }
            } وإلا {
                arcCasep->جوڙيو_Itemsp(جديد Ast_Case_شي{m_flp, خالي_اشارو, خالي_اشارو});
            }
        }
        أرجع arcCasep;
    }
    Ast_ڳنڍ_Stmt* buildUnxpandedTransitionHandler(ثابت Fsm_State_راس* ثابت منزل_Statep) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, خالي_اشارو};
        لكل (ثابت V3_گراف_ڪنارو& ڪنارو : منزل_Statep->اندريون_ڪنارا()) {
            ثابت Fsm_Arc_ڪنارو* ثابت arcp = ڪنارو.as<Fsm_Arc_ڪنارو>();
            UASSERT_OBJ(!arcp->isReset(), m_گراف.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_گراف.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ثابت Fsm_State_راس* ثابت fromp = arcp->fromp()->as<Fsm_State_راس>();
            إذا (fromp != منزل_Statep) {
                arcCasep->جوڙيو_Itemsp(
                    جديد Ast_Case_شي{m_flp, makeConst(fromp->قدر().انگ().منزل_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), منزل_Statep->label())});
            }
        }
        arcCasep->جوڙيو_Itemsp(جديد Ast_Case_شي{m_flp, خالي_اشارو, خالي_اشارو});
        أرجع arcCasep;
    }

عام:
    FsmCoverIncBuilder(ثابت Fsm_گراف& گراف, AstVarScope* ثابت prevVscp)
        : m_گراف{گراف}
        , m_prevVscp{prevVscp}
        , m_flp{گراف.fileline()}
        , m_modPrettyName{گراف.scopep()->modp()->prettyName()} {
        init_Existing_حالتون();
        AstVarScope* ثابت sampleVscp = گراف.sampleVarScopep();
        ثابت VFsmExpandType expand
            = گراف.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? گراف.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ثابت سائيز_t arc_ڳڻپ_With_Expansion = get_Arc_ڳڻپ_With_Expansion(expand);
            إذا (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_ڳڻپ_With_Expansion == 0ULL
                            || arc_ڳڻپ_With_Expansion
                                   > static_cast<سائيز_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_ڳڻپ_With_Expansion);
                أرجع;
            }
        }

        AstCase* ثابت arcCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, گراف.sampleVarScopep(), VAccess::READ}, خالي_اشارو};
        AstCase* ثابت initialStateCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, گراف.sampleVarScopep(), VAccess::READ}, خالي_اشارو};

        ثابت منطقي expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_سيٽ<ثابت Fsm_State_راس*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_سيٽ<ثابت Fsm_State_راس*>
            resetTargets;  // States that have edge incoming from reset vertex
        إذا (!expandAll) {
            إذا (m_defaultVertexp) {
                لكل (ثابت V3_گراف_ڪنارو& ڪنارو : m_defaultVertexp->ٻاهريون_ڪنارا()) {
                    defaultTargets.emplace(ڪنارو.چوٽي()->as<Fsm_State_راس>());
                }
            }
            إذا (m_resetVertexp) {
                لكل (ثابت V3_گراف_ڪنارو& ڪنارو : m_resetVertexp->ٻاهريون_ڪنارا()) {
                    resetTargets.emplace(ڪنارو.چوٽي()->as<Fsm_State_راس>());
                }
            }
        }
        ثابت منطقي is_ڀريل_Expansion = expand == VFsmExpandType::ڀريل;
        إذا (is_ڀريل_Expansion) init_Undefined_حالتون();
        لكل (ثابت Fsm_State_راس* ثابت statep : m_حالتون) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(statep->label());
            initialStateCasep->جوڙيو_Itemsp(
                جديد Ast_Case_شي{m_flp, makeConst(statep->قدر().انگ().منزل_UQuad()),
                                جديد AstCoverInc{m_flp, declp, سچ}});
            ثابت منطقي expandAllOrReset
                = expandAll || (resetTargets.ڳوليو(statep) != resetTargets.پڇاڙي());
            ثابت منطقي defaultExpantion
                = !expandAllOrReset && (defaultTargets.ڳوليو(statep) != defaultTargets.پڇاڙي());
            arcCasep->جوڙيو_Itemsp(جديد Ast_Case_شي{
                m_flp, makeConst(statep->قدر().انگ().منزل_UQuad()),
                Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ_Stmt>(
                    جديد AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_ڀريل_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        إذا (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            إذا (is_ڀريل_Expansion) {
                لكل (ثابت uint64_t قدر : m_undefined_حالتون) {
                    ثابت std::string& label = quad_منزل_ٻٽو_انگ_Stirng_Label(قدر);
                    AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(label);
                    initialStateCasep->جوڙيو_Itemsp(جديد Ast_Case_شي{
                        m_flp, makeConst(قدر), جديد AstCoverInc{m_flp, declp, سچ}});
                    arcCasep->جوڙيو_Itemsp(
                        جديد Ast_Case_شي{m_flp, makeConst(قدر),
                                        Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ_Stmt>(
                                            جديد AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, سچ, خطأ))});
                }
            } وإلا {
                // No expansion
                arcCasep->جوڙيو_Itemsp(جديد Ast_Case_شي{m_flp, خالي_اشارو, خالي_اشارو});
                initialStateCasep->جوڙيو_Itemsp(جديد Ast_Case_شي{m_flp, خالي_اشارو, خالي_اشارو});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = افتراضي;
    Ast_ڳنڍ_Cover_Decl* getCoverDeclsp() ثابت { أرجع m_declsp; }
    Ast_ڳنڍ_Stmt* getCoverIncp() ثابت { أرجع m_coverIncp; }
    Ast_ڳنڍ_Stmt* getInitialCoverIncp() ثابت { أرجع m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
صنف Fsm_هيٺاهون_Visitor final {
    // STATE - across all visitors
    ثابت FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ساكن Ast_Sen_وڻ* build_Sen_وڻ(File_سٽ* flp, ثابت std::ويڪٽر<FsmSenDesc>& senses) {
        Ast_Sen_وڻ* ثابت sentreep = جديد Ast_Sen_وڻ{flp, خالي_اشارو};
        لكل (ثابت FsmSenDesc& sense : senses) {
            Ast_Sen_شي* ثابت senItemp
                = جديد Ast_Sen_شي{flp, VEdgeType{sense.ڪنارو_Type},
                                 جديد AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->جوڙيو_Sensesp(senItemp);
        }
        أرجع sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    فراغ buildOne(ثابت Fsm_گراف& گراف) {
        UINFO(1, "buildOne lowering FSM " << گراف.stateVarName()
                                          << " vertices=" << گراف.vertices().سائيز() << سٽ_پڄاڻي);
        AstAlways* ثابت alwaysp = گراف.stateAlwaysp();
        AstScope* ثابت scopep = گراف.scopep();
        AstVarScope* ثابت stateVscp = گراف.stateVarScopep();
        AstVarScope* ثابت sampleVscp = گراف.sampleVarScopep();
        File_سٽ* ثابت flp = گراف.fileline();
        Ast_ڳنڍ_Module* ثابت modp = scopep->modp();
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
                            جديد Ast_Sen_وڻ{flp, جديد Ast_Sen_شي{flp, Ast_Sen_شي::Initial{}}}};
        initActivep->sen_وڻ_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->جوڙيو_Stmtsp(جديد AstInitialStatic{
            flp, جديد Ast_سونپيو{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                               جديد AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->جوڙيو_Blocksp(initActivep);

        AstAlwaysPost* ثابت covPostp = جديد AstAlwaysPost{flp};
        منطقي updatePrevAfterPost = خطأ;
        إذا (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        } وإلا {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = سچ;
            prevVscp->varp()->سيٽ_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ثابت FsmCoverIncBuilder coverIncBuilder{گراف, prevVscp};
            modp->جوڙيو_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->جوڙيو_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->جوڙيو_Stmtsp(
                جديد AstIf{flp,
                          جديد AstNeq{flp, جديد AstVarRef{flp, prevVscp, VAccess::READ},
                                     جديد AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        إذا (updatePrevAfterPost) {
            covPostp->جوڙيو_Stmtsp(جديد Ast_سونپيو{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_وڻ* ثابت sentreep = build_Sen_وڻ(flp, گراف.senses());
        Ast_سرگرم* ثابت activep = جديد Ast_سرگرم{flp, "fsm-coverage", sentreep};
        activep->sen_وڻ_Storep(sentreep);
        scopep->جوڙيو_Blocksp(activep);
        activep->جوڙيو_Stmtsp(covPostp);
    }

عام:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    واضح Fsm_هيٺاهون_Visitor(ثابت FsmState& state)
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
        سائيز_t اشاريو = 0;
        لكل (ثابت DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_ٽيگ(اشاريو++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_هيٺاهون_Visitor هيٺاهون{state}; }
    V3Global::dump_Check_Global_وڻ("fsm-detect", 0, dump_وڻ_Either_Level() >= 3);
}
