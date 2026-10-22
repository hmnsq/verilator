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
صنف Fsm_State_مقدار final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_کلید;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_عدد m_رقم;  // Original folded value, preserving width for lowered comparisons

    ساكن string make_کلید(ثابت V3_عدد& رقم) {
        V3_عدد کلید_رقم = رقم;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        کلید_رقم.isSigned(خطأ);
        أرجع cvt_مقصد_Str(کلید_رقم.عرض()) + ":" + کلید_رقم.ascii(درست, درست);
    }

عام:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_مقدار()
        : m_کلید{"1:1'h0"}
        , m_رقم{static_cast<Ast_گره*>(اشاره_تهی), 1, 0} {}
    صریح Fsm_State_مقدار(ثابت V3_عدد& رقم)
        : m_کلید{make_کلید(رقم)}
        , m_رقم{رقم} {}

    ثابت string& کلید() ثابت { أرجع m_کلید; }
    ثابت V3_عدد& رقم() ثابت { أرجع m_رقم; }
    string ascii() ثابت { أرجع m_رقم.ascii(درست, درست); }
    string warn_متن() ثابت {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        إذا (m_رقم.عرض() <= 32) أرجع cvt_مقصد_Str(m_رقم.مقصد_UInt());
        أرجع ascii();
    }

    منطقي عملگر==(ثابت Fsm_State_مقدار& rhs) ثابت { أرجع m_کلید == rhs.m_کلید; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
بنية Fsm_State_مقدار_Hash final {
    اندازه_t عملگر()(ثابت Fsm_State_مقدار& مقدار) ثابت {
        أرجع std::hash<string>{}(مقدار.کلید());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
بنية FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en یال_Type = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = اشاره_تهی;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
بنية FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = اشاره_تهی;
    منطقي فعال_کم = خطأ;
};

صنف FsmResetArcDesc final {
    Fsm_State_مقدار m_مقصد_مقدار;  // Encoded reset target state.
    Ast_گره* m_nodep = اشاره_تهی;  // Source node for warnings and emitted metadata.
    Ast_گره_عبارت* m_valuep = اشاره_تهی;  // Expression that provided the reset value.

عام:
    FsmResetArcDesc() = افتراضي;
    FsmResetArcDesc(Fsm_State_مقدار مقصد_مقدار, Ast_گره_تخصیص_بده* nodep)
        : m_مقصد_مقدار{مقصد_مقدار}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_مقدار مقصد_مقدار, Ast_گره* nodep, Ast_گره_عبارت* valuep)
        : m_مقصد_مقدار{مقصد_مقدار}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_مقدار مقصد_مقدار() ثابت { أرجع m_مقصد_مقدار; }
    Ast_گره* nodep() ثابت { أرجع m_nodep; }
    Ast_گره_عبارت* valuep() ثابت { أرجع m_valuep; }
};

بنية FsmWrapperRoles final {
    string d_پورت;
    string q_پورت;
    string clk_پورت;
    string rst_پورت;
    string rstValParam;
    منطقي has_Rst_فعال_کم = خطأ;
    منطقي rst_فعال_کم = خطأ;
};

ساكن منطقي fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, ثابت string& پورت_Name,
                                                  منطقي& فعال_کم) {
    منطقي matched = خطأ;
    cellp->modp()->foreach([&](Ast_Sen_قلم* itemp) {
        Ast_گره_Var_Ref* ثابت vrefp = itemp->varrefp();
        إذا (!vrefp) أرجع;
        إذا (vrefp->varp()->name() != پورت_Name) أرجع;
        فعال_کم = itemp->یال_Type() == VEdgeType::ET_NEGEDGE;
        matched = درست;
    });
    أرجع matched;
}

ساكن ثابت V3Control::Fsm_ثبات_Wrapper* fsm_ثبات_Wrapper_Desc(AstCell* cellp) {
    Ast_گره_Module* ثابت modp = cellp->modp();
    ثابت string origName = modp->origName();
    إذا (ثابت V3Control::Fsm_ثبات_Wrapper* ثابت descp
        = V3Control::get_Fsm_ثبات_Wrapper(origName)) {
        أرجع descp;
    }
    أرجع V3Control::get_Fsm_ثبات_Wrapper(modp->prettyDehashOrigOrName());
}

ساكن FsmWrapperRoles rolesFromDesc(ثابت V3Control::Fsm_ثبات_Wrapper& desc) {
    FsmWrapperRoles roles;
    roles.d_پورت = desc.d;
    roles.q_پورت = desc.q;
    roles.clk_پورت = desc.clock;
    roles.rst_پورت = desc.reset;
    roles.rstValParam = desc.reset_مقدار;
    أرجع roles;
}

صنف Fsm_ثبات_Candidate final {
    AstScope* m_scopep = اشاره_تهی;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = اشاره_تهی;  // Register process that commits the state.
    AstVarScope* m_stateVscp = اشاره_تهی;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = اشاره_تهی;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_بعدی_Vscp = اشاره_تهی;  // Next-state variable or same state var for 1-block FSMs.
    std::وکتور<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::وکتور<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    منطقي m_hasResetCond = خطأ;  // Whether the FSM had a modeled reset predicate.
    منطقي m_inclCond = خطأ;  // Whether conditional/default arcs are kept explicitly.
    File_سطر* m_flp = اشاره_تهی;  // Representative source location.

عام:
    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
    فراغ alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() ثابت { أرجع m_stateVscp; }
    فراغ stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() ثابت { أرجع m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    فراغ sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* بعدی_Vscp() ثابت { أرجع m_بعدی_Vscp; }
    فراغ بعدی_Vscp(AstVarScope* vscp) { m_بعدی_Vscp = vscp; }
    ثابت std::وکتور<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::وکتور<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    ثابت std::وکتور<FsmResetArcDesc>& resetArcs() ثابت { أرجع m_resetArcs; }
    std::وکتور<FsmResetArcDesc>& resetArcs() { أرجع m_resetArcs; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    File_سطر* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_سطر* flp) { m_flp = flp; }
};

صنف FsmComboAlways final {
    AstScope* ثابت m_scopep = اشاره_تهی;  // Owning scope for the combinational process.
    AstAlways* ثابت m_alwaysp = اشاره_تهی;  // Candidate transition process.

عام:
    FsmComboAlways() = افتراضي;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() ثابت { أرجع m_scopep; }
    AstAlways* alwaysp() ثابت { أرجع m_alwaysp; }
};

صنف Fsm_گراف;

صنف Fsm_رأس VL_NOT_FINAL : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Fsm_رأس, V3_گراف_رأس)

عام:
    تعداد صنف نوع_2 : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

خاص:
    نوع_2 m_نوع;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_مقدار m_مقدار;  // Encoded state value for real state vertices.

محمي:
    Fsm_رأس(V3_گراف* graphp, نوع_2 نوع, string label, Fsm_State_مقدار مقدار) VL_MT_DISABLED
        : V3_گراف_رأس{graphp},
          m_نوع{نوع},
          m_label{label},
          m_مقدار{مقدار} {}
    ~Fsm_رأس() override = افتراضي;

عام:
    نوع_2 نوع() ثابت { أرجع m_نوع; }
    منطقي isState() ثابت { أرجع m_نوع == نوع_2::STATE; }
    منطقي isResetAny() ثابت { أرجع m_نوع == نوع_2::RESET_ANY; }
    منطقي isDefaultAny() ثابت { أرجع m_نوع == نوع_2::DEFAULT_ANY; }
    ثابت string& label() ثابت { أرجع m_label; }
    ثابت Fsm_State_مقدار& مقدار() ثابت { أرجع m_مقدار; }

    string name() ثابت override VL_MT_امن { أرجع m_label + "=" + m_مقدار.ascii(); }
};

صنف Fsm_State_رأس final : عام Fsm_رأس {
    VL_RTTI_IMPL(Fsm_State_رأس, Fsm_رأس)

عام:
    Fsm_State_رأس(V3_گراف* graphp, string label, Fsm_State_مقدار مقدار) VL_MT_DISABLED
        : Fsm_رأس{graphp, نوع_2::STATE, label, مقدار} {}
    ~Fsm_State_رأس() override = افتراضي;

    string dotColor() ثابت override { أرجع "lightblue"; }
    string dotShape() ثابت override { أرجع "ellipse"; }
};

صنف Fsm_Pseudo_رأس final : عام Fsm_رأس {
    VL_RTTI_IMPL(Fsm_Pseudo_رأس, Fsm_رأس)

عام:
    Fsm_Pseudo_رأس(V3_گراف* graphp, نوع_2 نوع, string label) VL_MT_DISABLED
        : Fsm_رأس{graphp, نوع, label, Fsm_State_مقدار{}} {}
    ~Fsm_Pseudo_رأس() override = افتراضي;

    string name() ثابت override VL_MT_امن { أرجع label(); }
    string dotColor() ثابت override { أرجع isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() ثابت override { أرجع "diamond"; }
};

صنف Fsm_Arc_یال final : عام V3_گراف_یال {
    VL_RTTI_IMPL(Fsm_Arc_یال, V3_گراف_یال)
    منطقي m_isReset = خطأ;  // Arc originates from the synthetic reset source.
    منطقي m_isCond = خطأ;  // Arc came from a conditional next-state split.
    منطقي m_isDefault = خطأ;  // Arc represents a case default source.
    File_سطر* m_flp = اشاره_تهی;  // Source location for emitted coverage metadata.

عام:
    Fsm_Arc_یال(V3_گراف* graphp, Fsm_رأس* fromp, Fsm_State_رأس* بالاترین, منطقي isReset, منطقي isCond,
               منطقي isDefault, File_سطر* flp) VL_MT_DISABLED : V3_گراف_یال{graphp, fromp, بالاترین, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~Fsm_Arc_یال() override = افتراضي;

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
    AstScope* m_scopep = اشاره_تهی;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = اشاره_تهی;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_state_Var_داخلی_Name;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = اشاره_تهی;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = اشاره_تهی;  // Scoped variable sampled by coverage logic.
    std::وکتور<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    منطقي m_hasResetCond = خطأ;  // Whether the detected FSM had a reset branch.
    منطقي m_inclCond = خطأ;  // Whether conditional arcs should be kept explicitly.
    File_سطر* m_flp = اشاره_تهی;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_مقدار, Fsm_State_رأس*, Fsm_State_مقدار_Hash>
        m_stateVertices;  // Value to state map.
    Fsm_Pseudo_رأس* m_resetVertexp = اشاره_تهی;  // Synthetic ANY source for reset arcs.
    Fsm_Pseudo_رأس* m_defaultVertexp = اشاره_تهی;  // Synthetic default source for case defaults.

عام:
    Fsm_گراف() VL_MT_DISABLED
        : m_resetVertexp{جديد Fsm_Pseudo_رأس{هذا, Fsm_رأس::نوع_2::RESET_ANY, "ANY"}},
          m_defaultVertexp{جديد Fsm_Pseudo_رأس{هذا, Fsm_رأس::نوع_2::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() ثابت { أرجع m_stateAlwaysp; }
    فراغ stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    ثابت string& stateVarName() ثابت { أرجع m_stateVarName; }
    فراغ stateVarName(ثابت string& name) { m_stateVarName = name; }
    ثابت string& state_Var_داخلی_Name() ثابت { أرجع m_state_Var_داخلی_Name; }
    فراغ state_Var_داخلی_Name(ثابت string& name) { m_state_Var_داخلی_Name = name; }
    AstVarScope* stateVarScopep() ثابت { أرجع m_stateVarScopep; }
    فراغ stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() ثابت {
        أرجع m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    فراغ sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    ثابت std::وکتور<FsmSenDesc>& senses() ثابت { أرجع m_senses; }
    std::وکتور<FsmSenDesc>& senses() { أرجع m_senses; }
    ثابت FsmResetCondDesc& resetCond() ثابت { أرجع m_resetCond; }
    FsmResetCondDesc& resetCond() { أرجع m_resetCond; }
    منطقي hasResetCond() ثابت { أرجع m_hasResetCond; }
    فراغ hasResetCond(منطقي flag) { m_hasResetCond = flag; }
    منطقي inclCond() ثابت { أرجع m_inclCond; }
    فراغ inclCond(منطقي flag) { m_inclCond = flag; }
    File_سطر* fileline() ثابت { أرجع m_flp; }
    فراغ fileline(File_سطر* flp) { m_flp = flp; }

    Fsm_State_رأس* جمع_کن_State_رأس(string label, Fsm_State_مقدار مقدار) VL_MT_DISABLED {
        Fsm_State_رأس* ثابت vertexp = جديد Fsm_State_رأس{هذا, label, مقدار};
        m_stateVertices.emplace(مقدار, vertexp);
        أرجع vertexp;
    }
    Fsm_Pseudo_رأس* reset_Any_رأس() VL_MT_DISABLED { أرجع m_resetVertexp; }
    Fsm_Pseudo_رأس* default_Any_رأس() VL_MT_DISABLED { أرجع m_defaultVertexp; }
    Fsm_Arc_یال* جمع_کن_Arc(Fsm_State_مقدار from_مقدار, Fsm_State_مقدار مقصد_مقدار, منطقي isReset, منطقي isCond,
                       منطقي isDefault, File_سطر* flp) VL_MT_DISABLED {
        Fsm_State_رأس* ثابت بالاترین = m_stateVertices.at(مقصد_مقدار);
        Fsm_رأس* fromp = اشاره_تهی;
        إذا (isReset) {
            fromp = reset_Any_رأس();
        } وإلا إذا (isDefault) {
            fromp = default_Any_رأس();
        } وإلا {
            fromp = m_stateVertices.at(from_مقدار);
        }
        أرجع جديد Fsm_Arc_یال{هذا, fromp, بالاترین, isReset, isCond, isDefault, flp};
    }

    string name() ثابت VL_MT_امن {
        أرجع "FSM "
               + (m_stateVarName.خالی() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dump_برچسب(اندازه_t شاخص) ثابت {
        string برچسب = state_Var_داخلی_Name();
        لكل (حرف& ch : برچسب) {
            إذا (!std::آیا_حرف_یا_رقم(static_cast<بیعلامت حرف>(ch))) ch = '_';
        }
        أرجع "fsm_" + cvt_مقصد_Str(شاخص) + "_" + برچسب;
    }
};

بنية DetectedFsm final {
    std::unique_ptr<Fsm_گراف> graphp;  // Extracted graph for one detected FSM candidate.
};

بنية FsmCaseCandidate final {
    Ast_گره* warnNodep = اشاره_تهی;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = اشاره_تهی;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
بنية FsmStateComparison final {
    AstVarScope* stateVscp = اشاره_تهی;  // Compared state variable
    Ast_گره_عبارت* valuep = اشاره_تهی;  // Compared constant value expression
    Fsm_State_مقدار مقدار;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
بنية FsmIfBranch final {
    AstIf* ifp = اشاره_تهی;  // Source if/else-if node
    Ast_گره* stmtsp = اشاره_تهی;  // Branch body
    Ast_گره_عبارت* valuep = اشاره_تهی;  // Source state value expression
    Fsm_State_مقدار from_مقدار;  // Encoded source state value
    منطقي has_بالاترین_Guard = خطأ;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
بنية FsmIfChainCandidate final {
    AstIf* ifp = اشاره_تهی;  // Top-level if-chain node
    AstVarScope* compareVscp = اشاره_تهی;  // Variable used by every state comparison
    std::وکتور<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_گره* defaultStmtsp = اشاره_تهی;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
استفاده FsmAliasMap = std::unordered_map<ثابت AstVarScope*, FsmStateComparison>;
استفاده Fsm_Cell_پورت_Map = std::unordered_map<string, AstVarScope*>;
استفاده Fsm_Cell_پورت_Alias_Map = std::unordered_map<ثابت AstCell*, Fsm_Cell_پورت_Map>;

بنية StateConstLabel final {
    string متن;
    منطقي fromParam = خطأ;
    اندازه_t state_شاخص = 0;
};

بنية FsmStateSpace final {
    std::وکتور<std::pair<string, Fsm_State_مقدار>> حالتها;  // User label and encoded value
    std::unordered_map<Fsm_State_مقدار, StateConstLabel, Fsm_State_مقدار_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = اشاره_تهی;  // Tracked FSM state variable
    منطقي enumBacked = خطأ;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
صنف FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::وکتور<DetectedFsm> m_fsms;
    std::map<ثابت AstVarScope*, اندازه_t> m_fsm_شاخص;

عام:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        ثابت std::map<ثابت AstVarScope*, اندازه_t>::const_iterator it = m_fsm_شاخص.پیدا_کن(stateVscp);
        إذا (it != m_fsm_شاخص.انتها()) أرجع m_fsms.at(it->دومی);
        ثابت اندازه_t شاخص = m_fsms.اندازه();
        m_fsm_شاخص.emplace(stateVscp, شاخص);
        m_fsms.emplace_پشت();
        أرجع m_fsms.پشت();
    }
    ثابت std::وکتور<DetectedFsm>& fsms() ثابت { أرجع m_fsms; }
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
    AstScope* m_scopep = اشاره_تهی;
    std::وکتور<Fsm_ثبات_Candidate> m_ثبات_Candidates;
    // Deferring one-block detection avoids making continuous alias support
    // depend on whether the assign appears before or after the always block.
    std::وکتور<FsmComboAlways> m_one_بلوک_Alwayss;
    std::وکتور<FsmComboAlways> m_comboAlwayss;
    std::وکتور<FsmComboAlways> m_nonComboAlwayss;
    // Wrapper FSM detection has a second path for designs compiled without
    // inlining. In that shape the state register stays behind an AstCell, so we
    // remember candidate cells and resolve them only after the surrounding
    // transition logic and post-link port wiring have both been seen.
    std::وکتور<std::pair<AstScope*, AstCell*>> m_wrapperCells;
    std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_مجموعه<ثابت AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    Fsm_Cell_پورت_Alias_Map m_cell_پورت_Aliases;
    Fsm_Cell_پورت_Alias_Map m_cell_پورت_فرزند_Aliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    ساكن Ast_گره_DType* unwrapEnumCandidate(Ast_گره_DType* dtypep) {
        أرجع dtypep->skip_Ref_مقصد_Enump();
    }

    ساكن string candidateConflictContext(Ast_گره* laterNodep,
                                           ثابت FsmCaseCandidate& اولی_Cand) {
        أرجع '\n' + laterNodep->warnContextPrimary() + اولی_Cand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + اولی_Cand.stateVscp->prettyNameQ() + '\n'
               + اولی_Cand.warnNodep->warnContextSecondary();
    }

    ساكن منطقي rejectFsmWrapperCell(AstCell* cellp, ثابت string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        أرجع خطأ;
    }

    ساكن منطقي simple_Param_State_مقدار(AstCell* cellp, ثابت string& name, Fsm_State_مقدار& مقدار,
                                      Ast_گره_عبارت*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = اشاره_تهی;
        لكل (Ast_گره* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isParam() || varp->name() != name) استمر;
            valuepr = VN_AS(varp->valuep(), گره_عبارت);
            أرجع const_مقدار_وضعیت(valuepr, مقدار) == Const_مقدار_وضعیت::OK;
        }
        أرجع خطأ;
    }

    ساكن منطقي فرزند_پورت_درون_Scope(AstVarScope* vscp, AstScope* والد_Scopep, AstCell*& cellpr) {
        إذا (!vscp->varp()->isIO()) أرجع خطأ;
        AstScope* ثابت scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        إذا (scopep->aboveScopep() != والد_Scopep) أرجع خطأ;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        أرجع درست;
    }

    ساكن AstVarScope* simple_تخصیص_بده_Var_Scope(Ast_گره_عبارت* exprp) {
        AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef);
        أرجع vrefp ? vrefp->varScopep() : اشاره_تهی;
    }

    فراغ جمع_کن_Wrapper_Cell(AstScope* scopep, AstCell* cellp) {
        m_cell_پورت_Aliases.emplace(cellp, Fsm_Cell_پورت_Map{});
        m_cell_پورت_فرزند_Aliases.emplace(cellp, Fsm_Cell_پورت_Map{});
        ثابت std::pair<AstScope*, AstCell*> قلم_2{scopep, cellp};
        إذا (std::پیدا_کن(m_wrapperCells.cbegin(), m_wrapperCells.cend(), قلم_2)
            != m_wrapperCells.cend()) {
            أرجع;
        }
        m_wrapperCells.emplace_پشت(قلم_2);
    }

    فراغ collect_Cell_پورت_Alias(Ast_تخصیص_بده_W* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* ثابت lhsVscp = simple_تخصیص_بده_Var_Scope(nodep->lhsp());
        AstVarScope* ثابت rhsVscp = simple_تخصیص_بده_Var_Scope(nodep->rhsp());
        إذا (!lhsVscp || !rhsVscp) أرجع;
        AstCell* cellp = اشاره_تهی;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        إذا (فرزند_پورت_درون_Scope(lhsVscp, m_scopep, cellp)) {
            إذا (!fsm_ثبات_Wrapper_Desc(cellp)) أرجع;
            UASSERT_OBJ(lhsVscp->varp()->is_درون_داد(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cell_پورت_Aliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cell_پورت_فرزند_Aliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            جمع_کن_Wrapper_Cell(m_scopep, cellp);
        } وإلا إذا (فرزند_پورت_درون_Scope(rhsVscp, m_scopep, cellp)) {
            إذا (!fsm_ثبات_Wrapper_Desc(cellp)) أرجع;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cell_پورت_Aliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cell_پورت_فرزند_Aliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            جمع_کن_Wrapper_Cell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, ثابت string& پورت_Name) ثابت {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        ثابت Fsm_Cell_پورت_Map& ports = m_cell_پورت_Aliases.at(cellp);
        ثابت Fsm_Cell_پورت_Map::const_iterator پورت_It = ports.پیدا_کن(پورت_Name);
        أرجع پورت_It == ports.انتها() ? اشاره_تهی : پورت_It->دومی;
    }

    AstVarScope* فرزند_Role_Var_Scope(AstCell* cellp, ثابت string& پورت_Name) ثابت {
        ثابت Fsm_Cell_پورت_Map& ports = m_cell_پورت_فرزند_Aliases.at(cellp);
        ثابت Fsm_Cell_پورت_Map::const_iterator پورت_It = ports.پیدا_کن(پورت_Name);
        أرجع پورت_It == ports.انتها() ? اشاره_تهی : پورت_It->دومی;
    }

    صنف ثبات_Always_Analyzer final {
        AstScope* ثابت m_scopep;  // Scope containing the always block(s) being analyzed

    عام:
        صریح ثبات_Always_Analyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::وکتور<std::pair<AstCase*, Ast_گره_عبارت*>>
        one_بلوک_Candidates(AstAlways* alwaysp) ثابت {
            std::وکتور<std::pair<AstCase*, Ast_گره_عبارت*>> candidates;
            Ast_گره* ثابت stmtsp = alwaysp->stmtsp();
            إذا (AstIf* ثابت اولی_Ifp = VN_CAST(stmtsp, If)) {
                إذا (AstCase* ثابت casep = VN_CAST(اولی_Ifp->elsesp(), Case)) {
                    candidates.emplace_پشت(casep,
                                            FsmDetectVisitor::isSimpleResetCond(اولی_Ifp->condp())
                                                ? اولی_Ifp->condp()
                                                : اشاره_تهی);
                }
            }
            لكل (Ast_گره* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstCase* ثابت casep = VN_CAST(nodep, Case))
                    candidates.emplace_پشت(casep, اشاره_تهی);
            }
            أرجع candidates;
        }

        std::وکتور<std::pair<AstIf*, Ast_گره_عبارت*>>
        one_بلوک_If_Candidates(AstAlways* alwaysp) ثابت {
            std::وکتور<std::pair<AstIf*, Ast_گره_عبارت*>> candidates;
            Ast_گره* ثابت stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            إذا (AstIf* ثابت اولی_Ifp = VN_CAST(stmtsp, If)) {
                إذا (AstIf* ثابت chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(اولی_Ifp->elsesp()), If)) {
                    candidates.emplace_پشت(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(اولی_Ifp->condp())
                                                ? اولی_Ifp->condp()
                                                : اشاره_تهی);
                }
            }
            لكل (Ast_گره* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) candidates.emplace_پشت(ifp, اشاره_تهی);
            }
            أرجع candidates;
        }

        منطقي تطابق_ثبات_Candidate(AstAlways* alwaysp, Fsm_ثبات_Candidate& cand) ثابت {
            أرجع FsmDetectVisitor::تطابق_ثبات_Always(alwaysp, m_scopep, cand);
        }

        فراغ build_One_بلوک_Candidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_گره_عبارت* resetCondp,
                                    Fsm_ثبات_Candidate& reg) ثابت {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.بعدی_Vscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_درخت(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != اشاره_تهی);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* ثابت اولی_Ifp = VN_CAST(alwaysp->stmtsp(), If);
            إذا (اولی_Ifp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = اشاره_تهی;
                ثابت Reset_تخصیص_بده_وضعیت reset_وضعیت = FsmDetectVisitor::collectConstStateAssigns(
                    اولی_Ifp->thensp(), resetStateVscp, reg.resetArcs());
                إذا (reset_وضعیت == Reset_تخصیص_بده_وضعیت::خالی_3) {
                    reg.resetArcs().clear();
                } وإلا إذا (reset_وضعیت == Reset_تخصیص_بده_وضعیت::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_مقدار reset_مقدار;
                    Ast_گره* ثابت thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(اولی_Ifp->thensp());
                    UASSERT_OBJ(thenNodep, اولی_Ifp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_گره_تخصیص_بده* ثابت resetAssp = FsmDetectVisitor::مستقیم_Const_State_تخصیص_بده_گره(
                        thenNodep, resetStateVscp, reset_مقدار);
                    إذا (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_پشت(reset_مقدار, resetAssp);
                    }
                } وإلا إذا (reset_وضعیت == Reset_تخصیص_بده_وضعیت::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    منطقي تطابق_Fsm_Wrapper_Cell(AstScope* scopep, AstCell* cellp, Fsm_ثبات_Candidate& cand) ثابت {
        FsmWrapperRoles roles = rolesFromDesc(*fsm_ثبات_Wrapper_Desc(cellp));

        AstVarScope* ثابت بعدی_Vscp = roleVarScope(cellp, roles.d_پورت);
        AstVarScope* ثابت stateVscp = roleVarScope(cellp, roles.q_پورت);
        إذا (!بعدی_Vscp || !stateVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* ثابت clkVscp = roleVarScope(cellp, roles.clk_پورت);
        إذا (!clkVscp) {
            أرجع rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.یال_Type = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().افزودن_پشت(clkSense);

        AstVarScope* resetVscp = اشاره_تهی;
        إذا (!roles.rst_پورت.خالی()) resetVscp = roleVarScope(cellp, roles.rst_پورت);
        إذا (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            منطقي inferred_فعال_کم = خطأ;
            إذا (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rst_پورت, inferred_فعال_کم)) {
                roles.has_Rst_فعال_کم = درست;
                roles.rst_فعال_کم = inferred_فعال_کم;
            }
        }

        Ast_گره_عبارت* resetValuep = اشاره_تهی;
        Fsm_State_مقدار reset_مقدار;
        ثابت منطقي has_Reset_مقدار
            = !roles.rstValParam.خالی()
              && simple_Param_State_مقدار(cellp, roles.rstValParam, reset_مقدار, resetValuep);
        إذا (resetVscp && roles.has_Rst_فعال_کم && has_Reset_مقدار) {
            FsmSenDesc rstSense;
            rstSense.یال_Type = roles.rst_فعال_کم ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().افزودن_پشت(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().فعال_کم = roles.rst_فعال_کم;
            cand.hasResetCond(درست);
            cand.resetArcs().emplace_پشت(reset_مقدار, cellp, resetValuep);
        } وإلا إذا (!roles.rst_پورت.خالی() || !roles.rstValParam.خالی()) {
            string reason;
            إذا (roles.rst_پورت.خالی()) {
                reason = "reset port is not configured";
            } وإلا إذا (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } وإلا إذا (!roles.has_Rst_فعال_کم) {
                reason = "reset polarity could not be inferred from the wrapper";
            } وإلا إذا (roles.rstValParam.خالی()) {
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
        cand.alwaysp(اشاره_تهی);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(فرزند_Role_Var_Scope(cellp, roles.q_پورت));
        cand.بعدی_Vscp(بعدی_Vscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        أرجع درست;
    }

    صنف ComboAlwaysAnalyzer final {
    عام:
        بنية Combo_تطابق final {
            ثابت Fsm_ثبات_Candidate* matchedp = اشاره_تهی;
            Ast_گره* warnNodep = اشاره_تهی;
        };

    خاص:
        ثابت std::وکتور<Fsm_ثبات_Candidate>& m_ثبات_Candidates;

    عام:
        صریح ComboAlwaysAnalyzer(ثابت std::وکتور<Fsm_ثبات_Candidate>& ثبات_Candidates)
            : m_ثبات_Candidates{ثبات_Candidates} {}

        Combo_تطابق تطابق_Case(Ast_گره* stmtsp, AstCase* casep) ثابت {
            Combo_تطابق تطابق;
            AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع تطابق;
            لكل (ثابت Fsm_ثبات_Candidate& reg : m_ثبات_Candidates) {
                إذا (selp->varScopep() == reg.بعدی_Vscp()) {
                    إذا (!FsmDetectVisitor::has_Canonical_بعدی_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.بعدی_Vscp())) {
                        استمر;
                    }
                } وإلا إذا (selp->varScopep() != reg.stateVscp()) {
                    استمر;
                }
                Ast_گره* ثابت warnNodep = FsmDetectVisitor::case_Supported_Transition_گره(
                    casep, reg.بعدی_Vscp(), reg.inclCond());
                إذا (!warnNodep) استمر;
                تطابق.matchedp = &reg;
                تطابق.warnNodep = warnNodep;
            }
            أرجع تطابق;
        }

        Combo_تطابق تطابق_If_Chain(Ast_گره* stmtsp, ثابت FsmIfChainCandidate& chain) ثابت {
            Combo_تطابق تطابق;
            لكل (ثابت Fsm_ثبات_Candidate& reg : m_ثبات_Candidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                إذا (chain.compareVscp == reg.بعدی_Vscp()) {
                    إذا (!FsmDetectVisitor::has_Canonical_بعدی_State_Default_Before_Case(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.بعدی_Vscp())) {
                        استمر;
                    }
                } وإلا إذا (chain.compareVscp != reg.stateVscp()) {
                    استمر;
                }
                Ast_گره* ثابت warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_گره(chain, reg.بعدی_Vscp());
                إذا (!warnNodep) استمر;
                تطابق.matchedp = &reg;
                تطابق.warnNodep = warnNodep;
            }
            أرجع تطابق;
        }

        منطقي shouldWarnUnsupported(Ast_گره* stmtsp, AstCase* casep) ثابت {
            ثابت AstVarRef* ثابت selp = VN_CAST(casep->exprp(), VarRef);
            إذا (!selp) أرجع خطأ;

            أرجع std::any_از(
                m_ثبات_Candidates.cbegin(), m_ثبات_Candidates.cend(),
                [&](ثابت Fsm_ثبات_Candidate& reg) -> منطقي {
                    ثابت منطقي matches_بعدی = selp->varScopep() == reg.بعدی_Vscp();
                    ثابت منطقي matchesState = selp->varScopep() == reg.stateVscp();

                    إذا (!matches_بعدی && !matchesState) أرجع خطأ;
                    إذا (matches_بعدی
                        && !FsmDetectVisitor::has_Canonical_بعدی_State_Default_Before_Case(
                            stmtsp, casep, reg.stateVscp(), reg.بعدی_Vscp())) {
                        أرجع خطأ;
                    }
                    أرجع FsmDetectVisitor::case_Supported_Transition_گره(casep, reg.بعدی_Vscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    ساكن منطقي isSimpleResetCond(Ast_گره_عبارت* condp) { أرجع VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    ساكن FsmResetCondDesc describeResetCond(Ast_گره_عبارت* condp) {
        FsmResetCondDesc desc;
        إذا (AstVarRef* ثابت vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        أرجع desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    ساكن std::وکتور<FsmSenDesc> describe_Sen_درخت(Ast_Sen_درخت* sentreep) {
        std::وکتور<FsmSenDesc> senses;
        لكل (Ast_Sen_قلم* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), Sen_قلم)) {
            Ast_گره_Var_Ref* ثابت vrefp = itemp->varrefp();
            إذا (!vrefp) استمر;
            FsmSenDesc desc;
            desc.یال_Type = itemp->یال_Type().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.افزودن_پشت(desc);
        }
        أرجع senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    ساكن منطقي isIgnorableStmt(Ast_گره* nodep) { أرجع VN_IS(nodep, CoverInc); }

    ساكن Ast_گره* skipLeadingIgnorableStmt(Ast_گره* nodep) {
        طالما (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        أرجع nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    ساكن Ast_گره* singleMeaningfulStmt(Ast_گره* stmtp) {
        Ast_گره* resultp = اشاره_تهی;
        لكل (Ast_گره* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            إذا (isIgnorableStmt(nodep)) استمر;
            إذا (resultp) أرجع اشاره_تهی;
            resultp = nodep;
        }
        أرجع resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    ساكن Ast_گره* singleMeaningfulBranch(Ast_گره* branchp) {
        إذا (!branchp) أرجع اشاره_تهی;
        أرجع branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    ساكن منطقي isPlainComboSentree(ثابت Ast_Sen_درخت* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        لكل (ثابت Ast_Sen_قلم* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), Sen_قلم)) {
            إذا (senp->یال_Type() == VEdgeType::ET_تغییریافته) استمر;
            أرجع خطأ;
        }
        أرجع درست;
    }

    فراغ warnUnsupportedComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_ثبات_Candidates};
        Ast_گره* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        منطقي warned = خطأ;
        لكل (Ast_گره* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            إذا (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = درست;
            }
            إذا (warned) اكسر;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    ساكن Ast_گره_تخصیص_بده* مستقیم_State_تخصیص_بده(Ast_گره* stmtp, AstVarScope* stateVscp) {
        Ast_گره* ثابت nodep = singleMeaningfulBranch(stmtp);
        إذا (!nodep) أرجع اشاره_تهی;
        Ast_گره_تخصیص_بده* ثابت assp = VN_CAST(nodep, گره_تخصیص_بده);
        إذا (!assp) أرجع اشاره_تهی;
        AstVarRef* ثابت vrefp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!vrefp || vrefp->varScopep() != stateVscp) أرجع اشاره_تهی;
        أرجع assp;
    }

    ساكن Ast_گره_تخصیص_بده* گره_State_Var_تخصیص_بده(Ast_گره* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_گره_تخصیص_بده* ثابت assp = VN_CAST(nodep, گره_تخصیص_بده);
        إذا (!assp) أرجع اشاره_تهی;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->پایه_Fromp(درست), VarRef);
        AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
        إذا (!rhsp || !lhsp) أرجع اشاره_تهی;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        أرجع assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    ساكن Ast_گره_تخصیص_بده* مستقیم_Cond_State_Var_تخصیص_بده(Ast_گره* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_گره_عبارت*& condp,
                                                   Fsm_State_مقدار& reset_مقدار) {
        Ast_گره_تخصیص_بده* ثابت assp = VN_CAST(nodep, گره_تخصیص_بده);
        إذا (!assp) أرجع اشاره_تهی;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->پایه_Fromp(درست), VarRef);
        AstCond* ثابت rhsp = VN_CAST(assp->rhsp(), Cond);
        إذا (!rhsp || !lhsp) أرجع اشاره_تهی;
        AstVarRef* ثابت elsep = VN_CAST(rhsp->elsep(), VarRef);
        إذا (!elsep) أرجع اشاره_تهی;
        إذا (const_مقدار_وضعیت(rhsp->thenp(), reset_مقدار) != Const_مقدار_وضعیت::OK) أرجع اشاره_تهی;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        أرجع assp;
    }

    ساكن Ast_گره_تخصیص_بده* مستقیم_Const_State_تخصیص_بده_گره(Ast_گره* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_مقدار& مقدار) {
        Ast_گره_تخصیص_بده* ثابت assp = VN_CAST(nodep, گره_تخصیص_بده);
        إذا (!assp) أرجع اشاره_تهی;
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp()->پایه_Fromp(درست), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        إذا (const_مقدار_وضعیت(assp->rhsp(), مقدار) != Const_مقدار_وضعیت::OK) أرجع اشاره_تهی;
        stateVscp = lhsp->varScopep();
        أرجع assp;
    }

    تعداد صنف Reset_تخصیص_بده_وضعیت : uint8_t {
        خالی_3,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    ساكن Reset_تخصیص_بده_وضعیت collectConstStateAssigns(Ast_گره* stmtp, AstVarScope*& stateVscp,
                                                      std::وکتور<FsmResetArcDesc>& resetArcs) {
        Ast_گره* nodep = skipLeadingIgnorableStmt(stmtp);
        إذا (!nodep) أرجع Reset_تخصیص_بده_وضعیت::خالی_3;
        لكل (;; nodep = nodep->nextp()) {
            AstVarScope* تخصیص_بده_State_Vscp = اشاره_تهی;
            Fsm_State_مقدار مقدار;
            Ast_گره_تخصیص_بده* ثابت assp = مستقیم_Const_State_تخصیص_بده_گره(nodep, تخصیص_بده_State_Vscp, مقدار);
            إذا (!assp) أرجع Reset_تخصیص_بده_وضعیت::NONE;
            إذا (!stateVscp) stateVscp = تخصیص_بده_State_Vscp;
            إذا (تخصیص_بده_State_Vscp != stateVscp) أرجع Reset_تخصیص_بده_وضعیت::NONE;
            إذا (!resetArcs.خالی()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                أرجع Reset_تخصیص_بده_وضعیت::MULTI_SAME_STATE;
            }
            resetArcs.emplace_پشت(مقدار, assp);
            إذا (!nodep->nextp()) أرجع Reset_تخصیص_بده_وضعیت::SINGLE;
        }
    }

    ساكن منطقي has_Canonical_بعدی_State_Default_Before_Case(Ast_گره* stmtsp, Ast_گره* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* بعدی_Vscp) {
        Ast_گره* ثابت bodyp = skipLeadingIgnorableStmt(stmtsp);
        منطقي sawCanonicalDefault = خطأ;
        لكل (Ast_گره* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            إذا (nodep == targetp) أرجع sawCanonicalDefault;
            إذا (Ast_گره_تخصیص_بده* ثابت assp = VN_CAST(nodep, گره_تخصیص_بده)) {
                AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assp->rhsp(), VarRef);
                إذا (!lhsp || lhsp->varScopep() != بعدی_Vscp) استمر;
                إذا (sawCanonicalDefault) {
                    ثابت string بعدی_Name = بعدی_Vscp->varp()->prettyNameQ();
                    ثابت string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + بعدی_Name
                                     + ") when the canonical " + بعدی_Name + " = " + stateName
                                     + " default is overwritten before the case statement");
                    أرجع خطأ;
                }
                إذا (!rhsp || rhsp->varScopep() != stateVscp) أرجع خطأ;
                sawCanonicalDefault = درست;
            }
        }
    }

    ساكن منطقي if_State_Const_تخصیص_بده(Ast_گره* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_مقدار& then_مقدار, Fsm_State_مقدار& else_مقدار) {
        AstIf* ثابت ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        إذا (!ifp || !ifp->elsesp()) أرجع خطأ;
        AstVarScope* thenVscp = اشاره_تهی;
        AstVarScope* elseVscp = اشاره_تهی;
        Ast_گره* ثابت thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_گره* ثابت elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        إذا (!thenNodep || !elseNodep) أرجع خطأ;
        إذا (!مستقیم_Const_State_تخصیص_بده_گره(thenNodep, thenVscp, then_مقدار)) أرجع خطأ;
        إذا (!مستقیم_Const_State_تخصیص_بده_گره(elseNodep, elseVscp, else_مقدار)) أرجع خطأ;
        إذا (thenVscp == stateVscp && elseVscp == stateVscp) أرجع درست;
        إذا (thenVscp != elseVscp) أرجع خطأ;
        Ast_گره* ثابت followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = اشاره_تهی;
        AstVarScope* finalFromVscp = اشاره_تهی;
        Ast_گره* ثابت finalNodep = singleMeaningfulBranch(followp);
        إذا (!finalNodep) أرجع خطأ;
        إذا (!گره_State_Var_تخصیص_بده(finalNodep, finalStateVscp, finalFromVscp)) أرجع خطأ;
        إذا (finalStateVscp != stateVscp) أرجع خطأ;
        إذا (finalFromVscp != thenVscp) أرجع خطأ;
        أرجع درست;
    }

    ساكن منطقي مستقیم_State_Cond_Const_تخصیص_بده(Ast_گره* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_مقدار& then_مقدار, Fsm_State_مقدار& else_مقدار) {
        Ast_گره_تخصیص_بده* ثابت assp = مستقیم_State_تخصیص_بده(stmtp, stateVscp);
        إذا (!assp) أرجع خطأ;
        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
        إذا (!condp) أرجع خطأ;
        أرجع const_مقدار_وضعیت(condp->thenp(), then_مقدار) == Const_مقدار_وضعیت::OK
               && const_مقدار_وضعیت(condp->elsep(), else_مقدار) == Const_مقدار_وضعیت::OK;
    }

    ساكن Ast_گره* case_قلم_Supported_Arc_گره(Ast_Case_قلم* itemp, AstVarScope* stateVscp,
                                             منطقي inclCond) {
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع اشاره_تهی;
        }
        Ast_گره_تخصیص_بده* ثابت assp = مستقیم_State_تخصیص_بده(itemp->stmtsp(), stateVscp);
        إذا (assp) {
            Fsm_State_مقدار مقصد_مقدار;
            إذا (const_مقدار_وضعیت(assp->rhsp(), مقصد_مقدار) == Const_مقدار_وضعیت::OK) أرجع assp;
        }
        Fsm_State_مقدار then_مقدار;
        Fsm_State_مقدار else_مقدار;
        إذا (مستقیم_State_Cond_Const_تخصیص_بده(itemp->stmtsp(), stateVscp, then_مقدار, else_مقدار)) {
            أرجع assp;
        }
        إذا (if_State_Const_تخصیص_بده(itemp->stmtsp(), stateVscp, then_مقدار, else_مقدار)) {
            أرجع singleMeaningfulBranch(itemp->stmtsp());
        }
        أرجع اشاره_تهی;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    ساكن Ast_گره* case_Supported_Transition_گره(AstCase* casep, AstVarScope* stateVscp,
                                                منطقي inclCond) {
        لكل (Ast_Case_قلم* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_قلم)) {
            إذا (Ast_گره* ثابت nodep = case_قلم_Supported_Arc_گره(itemp, stateVscp, inclCond))
                أرجع nodep;
        }
        أرجع اشاره_تهی;
    }

    ساكن Ast_گره* case_قلم_Supported_Arc_گره_Like(Ast_گره* stmtsp, AstVarScope* stateVscp) {
        إذا (Ast_گره_تخصیص_بده* ثابت assp = مستقیم_State_تخصیص_بده(stmtsp, stateVscp)) {
            Fsm_State_مقدار مقصد_مقدار;
            إذا (const_مقدار_وضعیت(assp->rhsp(), مقصد_مقدار) == Const_مقدار_وضعیت::OK) أرجع assp;
            Fsm_State_مقدار then_مقدار;
            Fsm_State_مقدار else_مقدار;
            إذا (مستقیم_State_Cond_Const_تخصیص_بده(stmtsp, stateVscp, then_مقدار, else_مقدار)) أرجع assp;
        }
        أرجع اشاره_تهی;
    }

    ساكن Ast_گره* if_Chain_Supported_Transition_گره(ثابت FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        لكل (اندازه_t i = 0; i < chain.branches.اندازه(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            Ast_گره* ثابت nodep = case_قلم_Supported_Arc_گره_Like(branch.stmtsp, stateVscp);
            إذا (!nodep) أرجع اشاره_تهی;
        }
        أرجع chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    ساكن string label_For_مقدار(
        ثابت std::unordered_map<Fsm_State_مقدار, StateConstLabel, Fsm_State_مقدار_Hash>& labels,
        ثابت Fsm_State_مقدار& مقدار) {
        أرجع labels.at(مقدار).متن;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    تعداد صنف Const_مقدار_وضعیت : uint8_t { OK, NOT_CONST, XZ };

    ساكن Const_مقدار_وضعیت const_مقدار_وضعیت(Ast_گره_عبارت* exprp, Fsm_State_مقدار& مقدار) {
        ثابت AstConst* ثابت constp = VN_CAST(exprp, Const);
        إذا (!constp) أرجع Const_مقدار_وضعیت::NOT_CONST;
        ثابت V3_عدد& رقم = constp->رقم();
        إذا (رقم.isAnyXZ()) أرجع Const_مقدار_وضعیت::XZ;
        مقدار = Fsm_State_مقدار{رقم};
        أرجع Const_مقدار_وضعیت::OK;
    }

    ساكن منطقي خالص_State_Comparison_No_Alias(Ast_گره_عبارت* exprp, FsmStateComparison& cmp) {
        AstEq* ثابت eqp = VN_CAST(exprp, Eq);
        إذا (!eqp) أرجع خطأ;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_گره_عبارت* valuep = eqp->rhsp();
        إذا (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->پایه_Fromp(درست), VarRef);
            إذا (!vrefp) { أرجع خطأ; }
            valuep = eqp->lhsp();
        }

        Fsm_State_مقدار مقدار;
        إذا (const_مقدار_وضعیت(valuep, مقدار) != Const_مقدار_وضعیت::OK) أرجع خطأ;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.مقدار = مقدار;
        أرجع درست;
    }

    ساكن منطقي خالص_State_Comparison(Ast_گره_عبارت* exprp, ثابت FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        إذا (خالص_State_Comparison_No_Alias(exprp, cmp)) أرجع درست;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        إذا (AstVarRef* ثابت vrefp = VN_CAST(exprp, VarRef)) {
            ثابت FsmAliasMap::const_iterator it = aliases.پیدا_کن(vrefp->varScopep());
            إذا (it == aliases.انتها()) أرجع خطأ;
            cmp = it->دومی;
            أرجع درست;
        }
        أرجع خطأ;
    }

    ساكن منطقي supported_بالاترین_Level_Guard(Ast_گره_عبارت* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        إذا (VN_IS(exprp, Or)) أرجع خطأ;
        إذا (VN_IS(exprp, RedAnd)) أرجع خطأ;
        إذا (VN_IS(exprp, RedOr)) أرجع خطأ;
        إذا (VN_IS(exprp, RedXor)) أرجع خطأ;
        أرجع درست;
    }

    ساكن منطقي resolveIfPredicate(Ast_گره_عبارت* exprp, ثابت FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, منطقي& hasGuard) {
        std::وکتور<Ast_گره_عبارت*> terms;
        std::وکتور<Ast_گره_عبارت*> pending;
        pending.افزودن_پشت(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        طالما (!pending.خالی()) {
            Ast_گره_عبارت* ثابت nodep = pending.پشت();
            pending.برداشت_پشت();
            إذا (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
                pending.افزودن_پشت(andp->rhsp());
                pending.افزودن_پشت(andp->lhsp());
            } وإلا {
                terms.افزودن_پشت(nodep);
            }
        }

        منطقي sawComparison = خطأ;
        لكل (اندازه_t i = 0; i < terms.اندازه(); ++i) {
            Ast_گره_عبارت* ثابت termp = terms[i];
            FsmStateComparison جمله_Cmp;
            إذا (خالص_State_Comparison(termp, aliases, جمله_Cmp /*ref*/)) {
                إذا (sawComparison) أرجع خطأ;
                cmp = جمله_Cmp;
                sawComparison = درست;
                استمر;
            }
            إذا (!supported_بالاترین_Level_Guard(termp)) أرجع خطأ;
            hasGuard = درست;
        }
        أرجع sawComparison;
    }

    ساكن فراغ جمع_کن_Alias(FsmAliasMap& aliases, std::unordered_مجموعه<ثابت AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, ثابت FsmStateComparison& cmp) {
        إذا (ambiguous.پیدا_کن(aliasVscp) != ambiguous.انتها()) أرجع;
        ثابت FsmAliasMap::iterator it = aliases.پیدا_کن(aliasVscp);
        إذا (it == aliases.انتها()) {
            aliases.emplace(aliasVscp, cmp);
            أرجع;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        إذا (it->دومی.stateVscp == cmp.stateVscp && it->دومی.مقدار == cmp.مقدار) أرجع;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        أرجع;
    }

    ساكن فراغ collect_Alias_From_تخصیص_بده(Ast_گره_تخصیص_بده* assp, FsmAliasMap& aliases,
                                       std::unordered_مجموعه<ثابت AstVarScope*>& ambiguous) {
        AstVarRef* ثابت lhsp = VN_CAST(assp->lhsp(), VarRef);
        إذا (!lhsp) أرجع;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        إذا (!خالص_State_Comparison_No_Alias(assp->rhsp(), cmp /*ref*/)) أرجع;
        جمع_کن_Alias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_گره* stmtsp, Ast_گره* limitp) ثابت {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_مجموعه<ثابت AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        لكل (Ast_گره* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            إذا (Ast_گره_تخصیص_بده* ثابت assp = VN_CAST(nodep, گره_تخصیص_بده)) {
                collect_Alias_From_تخصیص_بده(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        لكل (ثابت AstVarScope* ثابت vscp : ambiguous) aliases.erase(vscp);
        أرجع aliases;
    }

    ساكن منطقي collectIfChain(AstIf* ifp, ثابت FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_مجموعه<string> دیده_مقادیر;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        طالما (درست) {
            FsmStateComparison cmp;
            منطقي hasGuard = خطأ;
            إذا (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) أرجع خطأ;
            إذا (chain.compareVscp && chain.compareVscp != cmp.stateVscp) أرجع خطأ;
            إذا (!دیده_مقادیر.درج(cmp.مقدار.کلید()).دومی) أرجع خطأ;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.افزودن_پشت(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.مقدار, hasGuard});

            Ast_گره* ثابت elseNodep
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
    ساكن منطقي validate_معلوم_State_مقدار(Ast_گره* nodep, ثابت FsmStateSpace& stateSpace,
                                        ثابت Fsm_State_مقدار& مقدار, ثابت string& role) {
        إذا (stateSpace.labels.پیدا_کن(مقدار) != stateSpace.labels.انتها()) أرجع درست;
        إذا (stateSpace.enumBacked) {
            ثابت string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + مقدار.warn_متن()
                                        + " is not present in the declared enum");
            أرجع خطأ;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + مقدار.warn_متن()
                                    + " is not present in the inferred state space");
        أرجع خطأ;
    }

    ساكن StateConstLabel stateLabelForConst(AstConst* constp) {
        ثابت string name = constp->origParamName();
        إذا (!name.خالی()) أرجع StateConstLabel{Ast_گره::prettyName(name), درست, 0};
        أرجع StateConstLabel{constp->name(), خطأ, 0};
    }

    ساكن فراغ updateStateLabel(FsmStateSpace& stateSpace, ثابت Fsm_State_مقدار& مقدار,
                                 ثابت StateConstLabel& label) {
        stateSpace.حالتها.at(stateSpace.labels.at(مقدار).state_شاخص).اولی = label.متن;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    ساكن منطقي تطابق_ثبات_Always(AstAlways* alwaysp, AstScope* scopep,
                                    Fsm_ثبات_Candidate& cand) {
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->has_یال()) أرجع خطأ;

        Ast_گره* ثابت stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_گره* ثابت nodep = singleMeaningfulStmt(stmtsp);
        إذا (!nodep) أرجع خطأ;

        AstVarScope* stateVscp = اشاره_تهی;
        AstVarScope* بعدی_Vscp = اشاره_تهی;
        إذا (AstIf* ثابت ifp = VN_CAST(nodep, If)) {
            إذا (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) أرجع خطأ;
            AstVarScope* resetStateVscp = اشاره_تهی;
            ثابت Reset_تخصیص_بده_وضعیت reset_وضعیت
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            ثابت منطقي خالی_Reset_Branch = reset_وضعیت == Reset_تخصیص_بده_وضعیت::خالی_3;
            إذا (خالی_Reset_Branch) {
                cand.resetArcs().clear();
            } وإلا إذا (reset_وضعیت == Reset_تخصیص_بده_وضعیت::NONE) {
                cand.resetArcs().clear();
                Fsm_State_مقدار reset_مقدار;
                Ast_گره* ثابت thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_گره_تخصیص_بده* ثابت resetAssp
                    = مستقیم_Const_State_تخصیص_بده_گره(thenNodep, resetStateVscp, reset_مقدار);
                إذا (!resetAssp) أرجع خطأ;
                cand.resetArcs().emplace_پشت(reset_مقدار, resetAssp);
            } وإلا إذا (reset_وضعیت == Reset_تخصیص_بده_وضعیت::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_گره* ثابت elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            إذا (!گره_State_Var_تخصیص_بده(elseNodep, stateVscp, بعدی_Vscp)) أرجع خطأ;
            إذا (!خالی_Reset_Branch && resetStateVscp != stateVscp) أرجع خطأ;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != اشاره_تهی);
        } وإلا {
            Ast_گره_عبارت* resetCondp = اشاره_تهی;
            Fsm_State_مقدار reset_مقدار;
            // Note the active low 'reset ? next : CONST' form is not accepted
            إذا (Ast_گره_تخصیص_بده* ثابت assp
                = مستقیم_Cond_State_Var_تخصیص_بده(nodep, stateVscp, بعدی_Vscp, resetCondp, reset_مقدار)) {
                cand.resetArcs().emplace_پشت(reset_مقدار, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != اشاره_تهی);
            } وإلا إذا (!گره_State_Var_تخصیص_بده(nodep, stateVscp, بعدی_Vscp)) {
                أرجع خطأ;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.بعدی_Vscp(بعدی_Vscp);
        cand.senses() = describe_Sen_درخت(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        أرجع درست;
    }

    ساكن منطقي جمع_کن_مقدار_مقصد_State_Space(Ast_گره* nodep, FsmStateSpace& stateSpace,
                                     ثابت Fsm_State_مقدار& مقدار, StateConstLabel label) {
        ثابت خودکار labelIt = stateSpace.labels.پیدا_کن(مقدار);
        إذا (labelIt != stateSpace.labels.انتها()) {
            StateConstLabel& existingLabel = labelIt->دومی;
            إذا (existingLabel.متن != label.متن && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + مقدار.warn_متن() + ": " + existingLabel.متن
                                            + " and " + label.متن);
                أرجع خطأ;
            }
            إذا (!existingLabel.fromParam && label.fromParam) {
                existingLabel.متن = label.متن;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, مقدار, label);
            }
        } وإلا {
            StateConstLabel storedLabel = label;
            storedLabel.state_شاخص = stateSpace.حالتها.اندازه();
            stateSpace.حالتها.emplace_پشت(label.متن, مقدار);
            stateSpace.labels.emplace(مقدار, storedLabel);
        }
        أرجع درست;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    ساكن منطقي جمع_کن_عبارت_مقصد_State_Space(Ast_گره_عبارت* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_مقدار مقدار;
        ثابت Const_مقدار_وضعیت وضعیت = const_مقدار_وضعیت(valuep, مقدار);
        إذا (وضعیت != Const_مقدار_وضعیت::OK) {
            إذا (وضعیت == Const_مقدار_وضعیت::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع جمع_کن_مقدار_مقصد_State_Space(valuep, stateSpace, مقدار, stateLabelForConst(constp));
    }

    ساكن منطقي جمع_کن_Optional_هدف_عبارت_مقصد_State_Space(Ast_گره_عبارت* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_مقدار مقدار;
        ثابت Const_مقدار_وضعیت وضعیت = const_مقدار_وضعیت(valuep, مقدار);
        إذا (وضعیت != Const_مقدار_وضعیت::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            أرجع خطأ;
        }
        AstConst* ثابت constp = VN_AS(valuep, Const);
        أرجع جمع_کن_مقدار_مقصد_State_Space(valuep, stateSpace, مقدار, stateLabelForConst(constp));
    }

    ساكن فراغ جمع_کن_Reset_Targets_مقصد_State_Space(ثابت std::وکتور<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.مقصد_مقدار().ascii(), خطأ, 0};
            إذا (AstConst* ثابت constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                جمع_کن_مقدار_مقصد_State_Space(resetArc.nodep(), stateSpace, resetArc.مقصد_مقدار(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    ساكن منطقي collectDeclaredStateSpace(Ast_گره* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, منطقي& needs_منبع_مقادیر) {
        AstVar* ثابت stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        إذا (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        ثابت منطقي forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        إذا (enump) {
            stateSpace.enumBacked = درست;
            لكل (Ast_Enum_قلم* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                ثابت Fsm_State_مقدار مقدار{constp->رقم()};
                ثابت اندازه_t state_شاخص = stateSpace.حالتها.اندازه();
                stateSpace.حالتها.emplace_پشت(itemp->name(), مقدار);
                stateSpace.labels.emplace(مقدار,
                                          StateConstLabel{itemp->name(), خطأ, state_شاخص});
            }
            أرجع stateSpace.حالتها.اندازه() >= 2;
        }

        إذا (forced) {
            needs_منبع_مقادیر = درست;
            أرجع درست;
        }

        needs_منبع_مقادیر = درست;
        أرجع درست;
    }

    قالب <نام_نوع T_ValuepVisitor>
    ساكن منطقي collect_State_Space_From_مقادیر(Ast_گره* warnNodep, AstVarScope* stateVscp,
                                            ثابت std::وکتور<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            ثابت T_ValuepVisitor& بازدید_Valueps) {
        منطقي needs_منبع_مقادیر = خطأ;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        إذا (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_منبع_مقادیر)) {
            أرجع خطأ;
        }
        إذا (!needs_منبع_مقادیر) أرجع درست;
        جمع_کن_Reset_Targets_مقصد_State_Space(resetArcs, stateSpace);
        إذا (!بازدید_Valueps(
                [&](Ast_گره_عبارت* valuep) { أرجع جمع_کن_عبارت_مقصد_State_Space(valuep, stateSpace); })) {
            أرجع خطأ;
        }
        أرجع stateSpace.حالتها.اندازه() >= 2;
    }

    ساكن منطقي collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* تخصیص_بده_Vscp,
                                  ثابت std::وکتور<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_مقادیر(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, تخصیص_بده_Vscp, &stateSpace](ثابت خودکار& بازدید_Valuep) {
                لكل (Ast_Case_قلم* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), Case_قلم)) {
                    إذا (!itemp->isDefault()) {
                        لكل (Ast_گره_عبارت* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), گره_عبارت)) {
                            إذا (!بازدید_Valuep(condp)) أرجع خطأ;
                        }
                    }
                    إذا (Ast_گره_تخصیص_بده* ثابت assp
                        = مستقیم_State_تخصیص_بده(itemp->stmtsp(), تخصیص_بده_Vscp)) {
                        Fsm_State_مقدار then_مقدار;
                        Fsm_State_مقدار else_مقدار;
                        AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                        إذا (condp
                            && مستقیم_State_Cond_Const_تخصیص_بده(itemp->stmtsp(), تخصیص_بده_Vscp, then_مقدار,
                                                          else_مقدار)) {
                            إذا (!بازدید_Valuep(condp->thenp())) أرجع خطأ;
                            إذا (!بازدید_Valuep(condp->elsep())) أرجع خطأ;
                        } وإلا إذا (!جمع_کن_Optional_هدف_عبارت_مقصد_State_Space(assp->rhsp(), stateSpace)) {
                            أرجع خطأ;
                        }
                    }
                }
                أرجع درست;
            });
    }

    ساكن منطقي collectStateSpace(ثابت FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* تخصیص_بده_Vscp,
                                  ثابت std::وکتور<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        أرجع collect_State_Space_From_مقادیر(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, تخصیص_بده_Vscp, &stateSpace](ثابت خودکار& بازدید_Valuep) {
                لكل (ثابت FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(بازدید_Valuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_گره_تخصیص_بده* ثابت assp = مستقیم_State_تخصیص_بده(branch.stmtsp, تخصیص_بده_Vscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_مقدار then_مقدار;
                    Fsm_State_مقدار else_مقدار;
                    AstCond* ثابت condp = VN_CAST(assp->rhsp(), Cond);
                    إذا (condp) {
                        UASSERT_OBJ(مستقیم_State_Cond_Const_تخصیص_بده(branch.stmtsp, تخصیص_بده_Vscp,
                                                               then_مقدار, else_مقدار),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        إذا (!بازدید_Valuep(condp->thenp())) أرجع خطأ;
                        إذا (!بازدید_Valuep(condp->elsep())) أرجع خطأ;
                    } وإلا إذا (!جمع_کن_Optional_هدف_عبارت_مقصد_State_Space(assp->rhsp(), stateSpace)) {
                        أرجع خطأ;
                    }
                }
                أرجع درست;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    ساكن منطقي emit_Case_قلم_Arcs(Fsm_گراف& گراف, Ast_Case_قلم* itemp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, منطقي inclCond) {
        std::وکتور<std::pair<string, Fsm_State_مقدار>> froms;
        إذا (itemp->isDefault()) {
            إذا (!inclCond) أرجع خطأ;
            froms.emplace_پشت("default", Fsm_State_مقدار{});
        } وإلا {
            لكل (Ast_گره_عبارت* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), گره_عبارت)) {
                Fsm_State_مقدار مقدار;
                إذا (const_مقدار_وضعیت(condp, مقدار) != Const_مقدار_وضعیت::OK) استمر;
                إذا (!validate_معلوم_State_مقدار(condp, stateSpace, مقدار, "source")) أرجع درست;
                froms.emplace_پشت(label_For_مقدار(stateSpace.labels, مقدار), مقدار);
            }
            إذا (froms.خالی()) أرجع خطأ;
        }

        إذا (Ast_گره_تخصیص_بده* ثابت assp = مستقیم_State_تخصیص_بده(itemp->stmtsp(), stateVscp)) {
            Fsm_State_مقدار مقصد_مقدار;
            ثابت Const_مقدار_وضعیت وضعیت = const_مقدار_وضعیت(assp->rhsp(), مقصد_مقدار);
            إذا (وضعیت == Const_مقدار_وضعیت::OK) {
                إذا (!validate_معلوم_State_مقدار(assp, stateSpace, مقصد_مقدار, "target")) أرجع درست;
                لكل (ثابت std::pair<string, Fsm_State_مقدار>& from : froms) {
                    گراف.جمع_کن_Arc(from.دومی, مقصد_مقدار, خطأ, خطأ, itemp->isDefault(),
                                 assp->fileline());
                }
                أرجع درست;
            }
        }

        Fsm_State_مقدار then_مقدار;
        Fsm_State_مقدار else_مقدار;
        إذا (مستقیم_State_Cond_Const_تخصیص_بده(itemp->stmtsp(), stateVscp, then_مقدار, else_مقدار)
            || if_State_Const_تخصیص_بده(itemp->stmtsp(), stateVscp, then_مقدار, else_مقدار)) {
            إذا (!validate_معلوم_State_مقدار(itemp->stmtsp(), stateSpace, then_مقدار, "target"))
                أرجع درست;
            إذا (!validate_معلوم_State_مقدار(itemp->stmtsp(), stateSpace, else_مقدار, "target"))
                أرجع درست;
            لكل (ثابت Fsm_State_مقدار& branch_مقدار : {then_مقدار, else_مقدار}) {
                لكل (ثابت std::pair<string, Fsm_State_مقدار>& from : froms) {
                    گراف.جمع_کن_Arc(from.دومی, branch_مقدار, خطأ, درست, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            أرجع درست;
        }

        أرجع خطأ;
    }

    ساكن فراغ emitStmtArcsFrom(Fsm_گراف& گراف, Ast_گره* stmtsp, AstVarScope* stateVscp,
                                 ثابت FsmStateSpace& stateSpace, Fsm_State_مقدار from_مقدار,
                                 منطقي isDefault, منطقي forceCond) {
        Ast_گره_تخصیص_بده* ثابت assp = مستقیم_State_تخصیص_بده(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_مقدار مقصد_مقدار;
        ثابت Const_مقدار_وضعیت وضعیت = const_مقدار_وضعیت(assp->rhsp(), مقصد_مقدار);
        إذا (وضعیت == Const_مقدار_وضعیت::OK) {
            إذا (!validate_معلوم_State_مقدار(assp, stateSpace, مقصد_مقدار, "target")) أرجع;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            گراف.جمع_کن_Arc(from_مقدار, مقصد_مقدار, خطأ, forceCond, isDefault, assp->fileline());
            أرجع;
        }

        Fsm_State_مقدار then_مقدار;
        Fsm_State_مقدار else_مقدار;
        ثابت منطقي cond_تخصیص_بده
            = مستقیم_State_Cond_Const_تخصیص_بده(stmtsp, stateVscp, then_مقدار, else_مقدار);
        UASSERT_OBJ(cond_تخصیص_بده, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        إذا (!validate_معلوم_State_مقدار(stmtsp, stateSpace, then_مقدار, "target")) أرجع;
        إذا (!validate_معلوم_State_مقدار(stmtsp, stateSpace, else_مقدار, "target")) أرجع;
        لكل (ثابت Fsm_State_مقدار& branch_مقدار : {then_مقدار, else_مقدار}) {
            گراف.جمع_کن_Arc(from_مقدار, branch_مقدار, خطأ, درست, isDefault, stmtsp->fileline());
        }
    }

    ساكن فراغ emitIfChainArcs(Fsm_گراف& گراف, ثابت FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, ثابت FsmStateSpace& stateSpace) {
        لكل (اندازه_t i = 0; i < chain.branches.اندازه(); ++i) {
            ثابت FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            إذا (!validate_معلوم_State_مقدار(branch.ifp, stateSpace, branch.from_مقدار, "source"))
                أرجع;
            emitStmtArcsFrom(گراف, branch.stmtsp, stateVscp, stateSpace, branch.from_مقدار, خطأ,
                             branch.has_بالاترین_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    ساكن فراغ جمع_کن_Reset_Arcs(Fsm_گراف& گراف, ثابت std::وکتور<FsmResetArcDesc>& resetArcs,
                             ثابت FsmStateSpace& stateSpace) {
        لكل (ثابت FsmResetArcDesc& resetArc : resetArcs) {
            إذا (!validate_معلوم_State_مقدار(resetArc.nodep(), stateSpace, resetArc.مقصد_مقدار(),
                                         "target"))
                استمر;
            گراف.جمع_کن_Arc(Fsm_State_مقدار{}, resetArc.مقصد_مقدار(), درست, خطأ, خطأ,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    فراغ processCase(AstCase* casep, AstVarScope* تخصیص_بده_Vscp, ثابت Fsm_ثبات_Candidate& reg) {
        UASSERT_OBJ(تخصیص_بده_Vscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        إذا (!collectStateSpace(casep, stateVscp, تخصیص_بده_Vscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& مدخل = m_state.fsmFor(stateVscp);
        إذا (!مدخل.graphp) {
            مدخل.graphp.reset(جديد Fsm_گراف{});
            مدخل.graphp->scopep(reg.scopep());
            مدخل.graphp->stateAlwaysp(reg.alwaysp());
            مدخل.graphp->stateVarName(stateVscp->prettyName());
            مدخل.graphp->state_Var_داخلی_Name(stateVscp->varp()->name());
            مدخل.graphp->stateVarScopep(stateVscp);
            مدخل.graphp->sampleVarScopep(reg.sampleVscp());
            مدخل.graphp->senses() = reg.senses();
            مدخل.graphp->resetCond() = reg.resetCond();
            مدخل.graphp->hasResetCond(reg.hasResetCond());
            مدخل.graphp->inclCond(reg.inclCond());
            مدخل.graphp->fileline(casep->fileline());
            لكل (ثابت std::pair<string, Fsm_State_مقدار>& state : stateSpace.حالتها) {
                مدخل.graphp->جمع_کن_State_رأس(state.اولی, state.دومی);
            }
            جمع_کن_Reset_Arcs(*مدخل.graphp, reg.resetArcs(), stateSpace);
        }
        لكل (Ast_Case_قلم* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_قلم)) {
            emit_Case_قلم_Arcs(*مدخل.graphp, itemp, تخصیص_بده_Vscp, stateSpace,
                             مدخل.graphp->inclCond());
        }
    }

    فراغ processIfChain(ثابت FsmIfChainCandidate& chain, AstVarScope* تخصیص_بده_Vscp,
                        ثابت Fsm_ثبات_Candidate& reg) {
        UASSERT_OBJ(تخصیص_بده_Vscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* ثابت stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        إذا (!collectStateSpace(chain, stateVscp, تخصیص_بده_Vscp, reg.resetArcs(), stateSpace)) أرجع;
        DetectedFsm& مدخل = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!مدخل.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        مدخل.graphp.reset(جديد Fsm_گراف{});
        مدخل.graphp->scopep(reg.scopep());
        مدخل.graphp->stateAlwaysp(reg.alwaysp());
        مدخل.graphp->stateVarName(stateVscp->prettyName());
        مدخل.graphp->state_Var_داخلی_Name(stateVscp->varp()->name());
        مدخل.graphp->stateVarScopep(stateVscp);
        مدخل.graphp->sampleVarScopep(reg.sampleVscp());
        مدخل.graphp->senses() = reg.senses();
        مدخل.graphp->resetCond() = reg.resetCond();
        مدخل.graphp->hasResetCond(reg.hasResetCond());
        مدخل.graphp->inclCond(reg.inclCond());
        مدخل.graphp->fileline(chain.ifp->fileline());
        لكل (ثابت std::pair<string, Fsm_State_مقدار>& state : stateSpace.حالتها) {
            مدخل.graphp->جمع_کن_State_رأس(state.اولی, state.دومی);
        }
        جمع_کن_Reset_Arcs(*مدخل.graphp, reg.resetArcs(), stateSpace);
        emitIfChainArcs(*مدخل.graphp, chain, تخصیص_بده_Vscp, stateSpace);
    }

    // Find the first supported FSM candidate in a clocked always block, warn on
    // additional candidates, and attach reset arcs when present. Candidate
    // filtering stays narrow on purpose: we prefer to skip ambiguous shapes now
    // and expand detection in a later PR rather than over-infer coverage from
    // forms we do not yet model confidently.
    فراغ process_One_بلوک_Always(ثابت FsmComboAlways& one_بلوک) {
        ثابت ثبات_Always_Analyzer analyzer{one_بلوک.scopep()};
        AstAlways* ثابت alwaysp = one_بلوک.alwaysp();
        إذا (!alwaysp->sentreep() || !alwaysp->sentreep()->has_یال()) أرجع;
        ثابت std::وکتور<std::pair<AstCase*, Ast_گره_عبارت*>> candidates
            = analyzer.one_بلوک_Candidates(alwaysp);

        FsmCaseCandidate اولی_Cand;
        لكل (ثابت std::pair<AstCase*, Ast_گره_عبارت*>& cand : candidates) {
            AstVarRef* ثابت selp = VN_CAST(cand.اولی->exprp(), VarRef);
            AstVarScope* ثابت vscp = selp ? selp->varScopep() : اشاره_تهی;
            إذا (!vscp) استمر;
            إذا (!اولی_Cand.stateVscp) {
                اولی_Cand.warnNodep = cand.اولی;
                اولی_Cand.stateVscp = vscp;
                Fsm_ثبات_Candidate reg;
                analyzer.build_One_بلوک_Candidate(alwaysp, vscp, cand.دومی, reg);
                processCase(cand.اولی, vscp, reg);
            } وإلا إذا (vscp != اولی_Cand.stateVscp) {
                cand.اولی->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.اولی, اولی_Cand));
            } وإلا {
                cand.اولی->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.اولی, اولی_Cand));
            }
        }

        إذا (اولی_Cand.stateVscp) أرجع;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        ثابت std::وکتور<std::pair<AstIf*, Ast_گره_عبارت*>> ifCandidates
            = analyzer.one_بلوک_If_Candidates(alwaysp);
        لكل (ثابت std::pair<AstIf*, Ast_گره_عبارت*>& cand : ifCandidates) {
            ثابت FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.اولی);
            FsmIfChainCandidate chain;
            إذا (!collectIfChain(cand.اولی, aliases, chain)) استمر;
            AstVarScope* ثابت vscp = chain.compareVscp;
            إذا (!if_Chain_Supported_Transition_گره(chain, vscp)) استمر;
            إذا (!اولی_Cand.stateVscp) {
                اولی_Cand.warnNodep = cand.اولی;
                اولی_Cand.stateVscp = vscp;
                Fsm_ثبات_Candidate reg;
                analyzer.build_One_بلوک_Candidate(alwaysp, vscp, cand.دومی, reg);
                processIfChain(chain, vscp, reg);
            } وإلا إذا (vscp != اولی_Cand.stateVscp) {
                cand.اولی->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.اولی, اولی_Cand));
            } وإلا {
                cand.اولی->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported "
                                   "transition candidates found in the same always block. Only "
                                   "the first candidate will be instrumented."
                                       << candidateConflictContext(cand.اولی, اولی_Cand));
            }
        }
    }

    // Phase 1 two-process pairing scans combinational always blocks only after
    // all strict register candidates have been collected, so source order does
    // not matter.
    ساكن فراغ warnComboSameAlways(Ast_گره* warnNodep, ثابت FsmCaseCandidate& اولی_Cand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, اولی_Cand));
    }

    فراغ processComboAlways(ثابت FsmComboAlways& combo) {
        ثابت ComboAlwaysAnalyzer analyzer{m_ثبات_Candidates};
        Ast_گره* ثابت stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate اولی_Cand;
        لكل (Ast_گره* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* ثابت casep = VN_CAST(nodep, Case);
            إذا (!casep) استمر;
            ثابت ComboAlwaysAnalyzer::Combo_تطابق تطابق = analyzer.تطابق_Case(stmtsp, casep);
            ثابت Fsm_ثبات_Candidate* ثابت matchedp = تطابق.matchedp;
            Ast_گره* ثابت matchedWarnNodep = تطابق.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!اولی_Cand.stateVscp) {
                ثابت خودکار درج_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!درج_Pair.دومی) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  درج_Pair.اولی->دومی));
                    استمر;
                }
                اولی_Cand.warnNodep = matchedWarnNodep;
                اولی_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->بعدی_Vscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != اولی_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, اولی_Cand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, اولی_Cand));
        }
        إذا (اولی_Cand.stateVscp) أرجع;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        لكل (Ast_گره* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* ثابت ifp = VN_CAST(nodep, If);
            إذا (!ifp) استمر;
            FsmIfChainCandidate chain;
            ثابت FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            إذا (!collectIfChain(ifp, aliases, chain)) استمر;
            ثابت ComboAlwaysAnalyzer::Combo_تطابق تطابق = analyzer.تطابق_If_Chain(stmtsp, chain);
            ثابت Fsm_ثبات_Candidate* ثابت matchedp = تطابق.matchedp;
            Ast_گره* ثابت matchedWarnNodep = تطابق.warnNodep;
            إذا (!matchedp) استمر;
            إذا (!اولی_Cand.stateVscp) {
                ثابت std::pair<std::unordered_map<ثابت AstVarScope*, FsmCaseCandidate>::iterator,
                                منطقي>
                    درج_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                إذا (!درج_Pair.دومی) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  درج_Pair.اولی->دومی));
                    استمر;
                }
                اولی_Cand.warnNodep = matchedWarnNodep;
                اولی_Cand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->بعدی_Vscp(), *matchedp);
                استمر;
            }
            إذا (matchedp->stateVscp() != اولی_Cand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, اولی_Cand);
                استمر;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported if-chain statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, اولی_Cand));
        }
    }

    // Track the current scope so each detected FSM records the module/scope
    // where instrumentation must later be inserted.
    فراغ بازدید(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    فراغ بازدید(AstAlways* nodep) override {
        إذا (nodep->keyword() == VAlwaysKwd::CONT_تخصیص_بده) {
            iterateChildren(nodep);
            أرجع;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_one_بلوک_Alwayss.emplace_پشت(m_scopep, nodep);
        ثابت ثبات_Always_Analyzer analyzer{m_scopep};
        Fsm_ثبات_Candidate reg;
        إذا (analyzer.تطابق_ثبات_Candidate(nodep, reg)) {
            AstVarScope* ثابت stateVscp = reg.stateVscp();
            ثابت منطقي یافت
                = std::any_از(m_ثبات_Candidates.cbegin(), m_ثبات_Candidates.cend(),
                              [stateVscp](ثابت Fsm_ثبات_Candidate& existing) {
                                  أرجع existing.stateVscp() == stateVscp;
                              });
            إذا (!یافت) { m_ثبات_Candidates.emplace_پشت(reg); }
        }
        إذا (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_پشت(m_scopep, nodep);
        } وإلا إذا (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            إذا (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_پشت(m_scopep, nodep);
            } وإلا {
                m_nonComboAlwayss.emplace_پشت(m_scopep, nodep);
            }
        }
    }

    فراغ بازدید(Ast_تخصیص_بده_W* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collect_Alias_From_تخصیص_بده(nodep, m_stateAliases, m_ambiguousStateAliases);
        collect_Cell_پورت_Alias(nodep);
        iterateChildren(nodep);
    }

    فراغ بازدید(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        إذا (m_scopep && fsm_ثبات_Wrapper_Desc(nodep)) جمع_کن_Wrapper_Cell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        لكل (ثابت std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            Fsm_ثبات_Candidate reg;
            إذا (تطابق_Fsm_Wrapper_Cell(wrapperCell.اولی, wrapperCell.دومی, reg)) {
                m_ثبات_Candidates.emplace_پشت(reg);
            }
        }
        لكل (ثابت FsmComboAlways& one_بلوک : m_one_بلوک_Alwayss) process_One_بلوک_Always(one_بلوک);
        لكل (ثابت FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        لكل (ثابت FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
صنف FsmCoverIncBuilder final {
    ثابت Fsm_گراف& m_گراف;  // Fsm graph
    std::وکتور<ثابت Fsm_State_رأس*> m_حالتها;  // States vertexes inside fsm graph
    std::unordered_مجموعه<uint64_t>
        m_undefined_حالتها;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    ثابت Fsm_رأس* m_defaultVertexp = اشاره_تهی;  // Default vertex
    ثابت Fsm_رأس* m_resetVertexp = اشاره_تهی;  // Reset vertex
    اندازه_t m_یالها_شمار = 0;  // Total count of edges
    AstVarScope* ثابت m_prevVscp;  // Previous state of fsm
    File_سطر* ثابت m_flp;  // Fileline pointer to use across the builder
    ثابت std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_گره_Stmt* m_coverIncp = اشاره_تهی;  // Increments proper CoverDecl after each change
    Ast_گره_Stmt* m_initialCoverIncp = اشاره_تهی;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = اشاره_تهی;  // List of CoverDecls
    منطقي m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(ثابت uint64_t مقدار) {
        V3_عدد رقم{static_cast<Ast_گره*>(اشاره_تهی), m_prevVscp->عرض()};
        رقم.مجموعه_Quad(مقدار);
        رقم.isSigned(m_prevVscp->dtypep()->isSigned());
        أرجع جديد AstConst{m_flp, رقم};
    }

    std::string quad_مقصد_بیت_Stirng_Label(ثابت uint64_t val) {
        صحيح عرض = m_prevVscp->عرض();
        UASSERT(عرض < 64, "We should not expand values that wide");
        std::string نتیجه = std::مقصد_string(عرض) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        طالما (--عرض >= 0) نتیجه.افزودن_پشت("01"[(val >> عرض) & 1]);
        أرجع نتیجه;
    }

    فراغ init_Existing_حالتها() {
        لكل (ثابت V3_گراف_رأس& vtx : m_گراف.vertices()) {
            ثابت Fsm_رأس* ثابت vertexp = vtx.as<Fsm_رأس>();
            m_یالها_شمار += vertexp->درون_یالها().اندازه();
            إذا (ثابت Fsm_State_رأس* ثابت statep = vertexp->cast<Fsm_State_رأس>()) {
                m_حالتها.افزودن_پشت(statep);
            } وإلا إذا (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_گراف.sampleVarScopep(),
                            "Two defaults?");
            } وإلا /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_گراف.sampleVarScopep(),
                            "Two resets?");
            }
        }
        ثابت صحيح عرض = m_گراف.sampleVarScopep()->عرض();
        m_isComplete = عرض < 64 && m_حالتها.اندازه() == (static_cast<اندازه_t>(1) << عرض);
    }

    فراغ init_Undefined_حالتها() {
        UASSERT_OBJ(m_گراف.sampleVarScopep()->عرض() < 64, m_گراف.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        ثابت uint64_t combinations_شمار = static_cast<اندازه_t>(1)
                                           << m_گراف.sampleVarScopep()->عرض();
        // Rolling trick allows to cut peak memory usage
        منطقي undefRolled = خطأ;
        خودکار rollUndefs = [هذا, combinations_شمار] {
            لكل (uint64_t i = 0; i < combinations_شمار; ++i) {
                ثابت خودکار درج = m_undefined_حالتها.درج(i);
                إذا (!درج.دومی) m_undefined_حالتها.erase(درج.اولی);
            }
        };
        لكل (ثابت Fsm_State_رأس* ثابت statep : m_حالتها) {
            ثابت V3_عدد& رقم = statep->مقدار().رقم();
            ثابت uint64_t مقدار = رقم.مقصد_UQuad();
            إذا (!m_isComplete) {
                إذا (undefRolled) {
                    UASSERT_OBJ(m_undefined_حالتها.erase(مقدار) == 1, m_گراف.sampleVarScopep(),
                                "Two states has the same value?");
                } وإلا {
                    UASSERT_OBJ(m_undefined_حالتها.درج(مقدار).دومی, m_گراف.sampleVarScopep(),
                                "Two states has the same value?");
                    إذا (m_undefined_حالتها.اندازه() * 2 >= combinations_شمار) {
                        undefRolled = درست;
                        rollUndefs();
                    }
                }
            }
        }
        إذا (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_گره_عبارت* buildResetCond() ثابت {
        Ast_گره_عبارت* ثابت refp
            = جديد AstVarRef{m_flp, m_گراف.resetCond().varScopep, VAccess::READ};
        أرجع m_گراف.resetCond().فعال_کم
                   ? static_cast<Ast_گره_عبارت*>(جديد AstLogNot{m_flp, refp})
                   : refp;
    }

    اندازه_t get_Arc_شمار_With_Expansion(ثابت VFsmExpandType expand) ثابت {
        اندازه_t arc_شمار_With_Expansion = ~0ULL;
        بدل (expand) {
        حالة VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_گراف.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            اكسر;
        }
        حالة VFsmExpandType::AUTO: {
            arc_شمار_With_Expansion = m_یالها_شمار;
            إذا (m_defaultVertexp) {
                UASSERT_OBJ(m_حالتها.اندازه() >= m_defaultVertexp->بیرون_یالها().اندازه(),
                            m_گراف.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_شمار_With_Expansion -= m_defaultVertexp->بیرون_یالها().اندازه();
                لكل (ثابت Fsm_State_رأس* ثابت statep : m_حالتها) {
                    arc_شمار_With_Expansion += static_cast<اندازه_t>(statep->بیرون_یالها().خالی())
                                             * m_defaultVertexp->بیرون_یالها().اندازه();
                }
            }
            إذا (m_resetVertexp) {
                UASSERT_OBJ(m_حالتها.اندازه() >= m_resetVertexp->بیرون_یالها().اندازه(),
                            m_گراف.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_شمار_With_Expansion += m_حالتها.اندازه() - m_resetVertexp->بیرون_یالها().اندازه();
            }
            اكسر;
        }
        حالة VFsmExpandType::AUTO_EXPAND: {
            arc_شمار_With_Expansion = m_حالتها.اندازه() * m_حالتها.اندازه() - m_حالتها.اندازه();
            اكسر;
        }
        حالة VFsmExpandType::پر: {
            ثابت صحيح عرض = m_گراف.sampleVarScopep()->عرض();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            إذا (عرض < 32) {
                ثابت uint64_t combinations_شمار = static_cast<اندازه_t>(1) << عرض;
                arc_شمار_With_Expansion = combinations_شمار * combinations_شمار - combinations_شمار;
            }
            اكسر;
        }
        }
        أرجع arc_شمار_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(ثابت std::string& label,
                                           ثابت std::string& fsm_برچسب = "") {
        AstCoverOtherDecl* ثابت declp
            = جديد AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_گراف.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_گراف.stateVarName(),
                                    "",
                                    label,
                                    fsm_برچسب};
        // We run after scoping, so this is declared for one instance only
        declp->در_هر_Instance(درست);
        m_declsp = Ast_گره::جمع_کن_بعدی(m_declsp, declp);
        أرجع declp;
    }

    Ast_گره_Stmt* buildCoverIncStmtp(ثابت std::string& fromStateLabel,
                                    ثابت std::string& مقصد_State_Label,
                                    ثابت std::string& پایه_Label = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* ثابت declp = جديد AstCoverOtherDecl{
            m_flp,
            "v_fsm_arc/" + m_modPrettyName,
            m_گراف.stateVarName() + "::" + fromStateLabel + "->" + مقصد_State_Label,
            "",
            0,
            m_گراف.stateVarName(),
            fromStateLabel,
            مقصد_State_Label,
            پایه_Label};
        // We run after scoping, so this is declared for one instance only
        declp->در_هر_Instance(درست);
        m_declsp = Ast_گره::جمع_کن_بعدی(m_declsp, declp);
        أرجع جديد AstCoverInc{m_flp, declp};
    }
    Ast_گره_Stmt* buildExpandedTransitionHandler(ثابت std::string& مقصد_State_Label,
                                                ثابت منطقي considerUndefs,
                                                ثابت منطقي expandOnlyWithTerminalVerticies,
                                                ثابت Fsm_State_رأس* ثابت مقصد_Statep = اشاره_تهی) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, اشاره_تهی};
        std::unordered_مجموعه<ثابت Fsm_State_رأس*> handledSoureces;
        إذا (مقصد_Statep) {
            لكل (ثابت V3_گراف_یال& یال : مقصد_Statep->درون_یالها()) {
                إذا (ثابت Fsm_State_رأس* ثابت fromp = یال.fromp()->cast<Fsm_State_رأس>()) {
                    إذا (!handledSoureces.emplace(fromp).دومی) استمر;
                    arcCasep->جمع_کن_Itemsp(جديد Ast_Case_قلم{
                        m_flp, makeConst(fromp->مقدار().رقم().مقصد_UQuad()),
                        (fromp != مقصد_Statep) ? buildCoverIncStmtp(fromp->label(), مقصد_State_Label)
                                            : اشاره_تهی});
                }
            }
        }
        لكل (ثابت Fsm_State_رأس* ثابت fromp : m_حالتها) {
            إذا (handledSoureces.پیدا_کن(fromp) != handledSoureces.انتها()) استمر;
            arcCasep->جمع_کن_Itemsp(جديد Ast_Case_قلم{
                m_flp, makeConst(fromp->مقدار().رقم().مقصد_UQuad()),
                (fromp != مقصد_Statep
                 && (!expandOnlyWithTerminalVerticies || fromp->بیرون_یالها().خالی()))
                    ? buildCoverIncStmtp(fromp->label(), مقصد_State_Label, "artificial")
                    : اشاره_تهی});
        }
        إذا (!m_isComplete) {
            إذا (considerUndefs) {
                لكل (ثابت uint64_t from : m_undefined_حالتها) {
                    ثابت std::string& label = quad_مقصد_بیت_Stirng_Label(from);
                    arcCasep->جمع_کن_Itemsp(
                        جديد Ast_Case_قلم{m_flp, makeConst(from),
                                        (label != مقصد_State_Label)
                                            ? buildCoverIncStmtp(label, مقصد_State_Label, "artificial")
                                            : اشاره_تهی});
                }
            } وإلا {
                arcCasep->جمع_کن_Itemsp(جديد Ast_Case_قلم{m_flp, اشاره_تهی, اشاره_تهی});
            }
        }
        أرجع arcCasep;
    }
    Ast_گره_Stmt* buildUnxpandedTransitionHandler(ثابت Fsm_State_رأس* ثابت مقصد_Statep) {
        AstCase* ثابت arcCasep = جديد AstCase{
            m_flp, VCaseType::CT_CASE, جديد AstVarRef{m_flp, m_prevVscp, VAccess::READ}, اشاره_تهی};
        لكل (ثابت V3_گراف_یال& یال : مقصد_Statep->درون_یالها()) {
            ثابت Fsm_Arc_یال* ثابت arcp = یال.as<Fsm_Arc_یال>();
            UASSERT_OBJ(!arcp->isReset(), m_گراف.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_گراف.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            ثابت Fsm_State_رأس* ثابت fromp = arcp->fromp()->as<Fsm_State_رأس>();
            إذا (fromp != مقصد_Statep) {
                arcCasep->جمع_کن_Itemsp(
                    جديد Ast_Case_قلم{m_flp, makeConst(fromp->مقدار().رقم().مقصد_UQuad()),
                                    buildCoverIncStmtp(fromp->label(), مقصد_Statep->label())});
            }
        }
        arcCasep->جمع_کن_Itemsp(جديد Ast_Case_قلم{m_flp, اشاره_تهی, اشاره_تهی});
        أرجع arcCasep;
    }

عام:
    FsmCoverIncBuilder(ثابت Fsm_گراف& گراف, AstVarScope* ثابت prevVscp)
        : m_گراف{گراف}
        , m_prevVscp{prevVscp}
        , m_flp{گراف.fileline()}
        , m_modPrettyName{گراف.scopep()->modp()->prettyName()} {
        init_Existing_حالتها();
        AstVarScope* ثابت sampleVscp = گراف.sampleVarScopep();
        ثابت VFsmExpandType expand
            = گراف.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? گراف.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            ثابت اندازه_t arc_شمار_With_Expansion = get_Arc_شمار_With_Expansion(expand);
            إذا (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_شمار_With_Expansion == 0ULL
                            || arc_شمار_With_Expansion
                                   > static_cast<اندازه_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_شمار_With_Expansion);
                أرجع;
            }
        }

        AstCase* ثابت arcCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, گراف.sampleVarScopep(), VAccess::READ}, اشاره_تهی};
        AstCase* ثابت initialStateCasep
            = جديد AstCase{m_flp, VCaseType::CT_CASE,
                          جديد AstVarRef{m_flp, گراف.sampleVarScopep(), VAccess::READ}, اشاره_تهی};

        ثابت منطقي expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_مجموعه<ثابت Fsm_State_رأس*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_مجموعه<ثابت Fsm_State_رأس*>
            resetTargets;  // States that have edge incoming from reset vertex
        إذا (!expandAll) {
            إذا (m_defaultVertexp) {
                لكل (ثابت V3_گراف_یال& یال : m_defaultVertexp->بیرون_یالها()) {
                    defaultTargets.emplace(یال.بالاترین()->as<Fsm_State_رأس>());
                }
            }
            إذا (m_resetVertexp) {
                لكل (ثابت V3_گراف_یال& یال : m_resetVertexp->بیرون_یالها()) {
                    resetTargets.emplace(یال.بالاترین()->as<Fsm_State_رأس>());
                }
            }
        }
        ثابت منطقي is_پر_Expansion = expand == VFsmExpandType::پر;
        إذا (is_پر_Expansion) init_Undefined_حالتها();
        لكل (ثابت Fsm_State_رأس* ثابت statep : m_حالتها) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(statep->label());
            initialStateCasep->جمع_کن_Itemsp(
                جديد Ast_Case_قلم{m_flp, makeConst(statep->مقدار().رقم().مقصد_UQuad()),
                                جديد AstCoverInc{m_flp, declp, درست}});
            ثابت منطقي expandAllOrReset
                = expandAll || (resetTargets.پیدا_کن(statep) != resetTargets.انتها());
            ثابت منطقي defaultExpantion
                = !expandAllOrReset && (defaultTargets.پیدا_کن(statep) != defaultTargets.انتها());
            arcCasep->جمع_کن_Itemsp(جديد Ast_Case_قلم{
                m_flp, makeConst(statep->مقدار().رقم().مقصد_UQuad()),
                Ast_گره::جمع_کن_بعدی<Ast_گره_Stmt>(
                    جديد AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), is_پر_Expansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        إذا (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            إذا (is_پر_Expansion) {
                لكل (ثابت uint64_t مقدار : m_undefined_حالتها) {
                    ثابت std::string& label = quad_مقصد_بیت_Stirng_Label(مقدار);
                    AstCoverOtherDecl* ثابت declp = buildStateCoverDecl(label);
                    initialStateCasep->جمع_کن_Itemsp(جديد Ast_Case_قلم{
                        m_flp, makeConst(مقدار), جديد AstCoverInc{m_flp, declp, درست}});
                    arcCasep->جمع_کن_Itemsp(
                        جديد Ast_Case_قلم{m_flp, makeConst(مقدار),
                                        Ast_گره::جمع_کن_بعدی<Ast_گره_Stmt>(
                                            جديد AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, درست, خطأ))});
                }
            } وإلا {
                // No expansion
                arcCasep->جمع_کن_Itemsp(جديد Ast_Case_قلم{m_flp, اشاره_تهی, اشاره_تهی});
                initialStateCasep->جمع_کن_Itemsp(جديد Ast_Case_قلم{m_flp, اشاره_تهی, اشاره_تهی});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = افتراضي;
    Ast_گره_Cover_Decl* getCoverDeclsp() ثابت { أرجع m_declsp; }
    Ast_گره_Stmt* getCoverIncp() ثابت { أرجع m_coverIncp; }
    Ast_گره_Stmt* getInitialCoverIncp() ثابت { أرجع m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
صنف Fsm_پایینی_Visitor final {
    // STATE - across all visitors
    ثابت FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    ساكن Ast_Sen_درخت* build_Sen_درخت(File_سطر* flp, ثابت std::وکتور<FsmSenDesc>& senses) {
        Ast_Sen_درخت* ثابت sentreep = جديد Ast_Sen_درخت{flp, اشاره_تهی};
        لكل (ثابت FsmSenDesc& sense : senses) {
            Ast_Sen_قلم* ثابت senItemp
                = جديد Ast_Sen_قلم{flp, VEdgeType{sense.یال_Type},
                                 جديد AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->جمع_کن_Sensesp(senItemp);
        }
        أرجع sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    فراغ buildOne(ثابت Fsm_گراف& گراف) {
        UINFO(1, "buildOne lowering FSM " << گراف.stateVarName()
                                          << " vertices=" << گراف.vertices().اندازه() << پایان_سطر);
        AstAlways* ثابت alwaysp = گراف.stateAlwaysp();
        AstScope* ثابت scopep = گراف.scopep();
        AstVarScope* ثابت stateVscp = گراف.stateVarScopep();
        AstVarScope* ثابت sampleVscp = گراف.sampleVarScopep();
        File_سطر* ثابت flp = گراف.fileline();
        Ast_گره_Module* ثابت modp = scopep->modp();
        AstVarScope* ثابت prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(درست);
        sampleVscp->optimizeLifePost(درست);
        prevVscp->optimizeLifePost(درست);

        Ast_فعال* ثابت initActivep
            = جديد Ast_فعال{flp, "fsm-coverage-init",
                            جديد Ast_Sen_درخت{flp, جديد Ast_Sen_قلم{flp, Ast_Sen_قلم::Initial{}}}};
        initActivep->sen_درخت_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->جمع_کن_Stmtsp(جديد AstInitialStatic{
            flp, جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                               جديد AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->جمع_کن_Blocksp(initActivep);

        AstAlwaysPost* ثابت covPostp = جديد AstAlwaysPost{flp};
        منطقي updatePrevAfterPost = خطأ;
        إذا (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        } وإلا {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = درست;
            prevVscp->varp()->مجموعه_Ignore_Post_Read();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            ثابت FsmCoverIncBuilder coverIncBuilder{گراف, prevVscp};
            modp->جمع_کن_Stmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->جمع_کن_Stmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->جمع_کن_Stmtsp(
                جديد AstIf{flp,
                          جديد AstNeq{flp, جديد AstVarRef{flp, prevVscp, VAccess::READ},
                                     جديد AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        إذا (updatePrevAfterPost) {
            covPostp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              جديد AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_درخت* ثابت sentreep = build_Sen_درخت(flp, گراف.senses());
        Ast_فعال* ثابت activep = جديد Ast_فعال{flp, "fsm-coverage", sentreep};
        activep->sen_درخت_Storep(sentreep);
        scopep->جمع_کن_Blocksp(activep);
        activep->جمع_کن_Stmtsp(covPostp);
    }

عام:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    صریح Fsm_پایینی_Visitor(ثابت FsmState& state)
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
        اندازه_t شاخص = 0;
        لكل (ثابت DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dump_برچسب(شاخص++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { Fsm_پایینی_Visitor پایینی{state}; }
    V3Global::dump_Check_Global_درخت("fsm-detect", 0, dump_درخت_Either_Level() >= 3);
}
