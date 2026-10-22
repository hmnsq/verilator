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
वर्ग Fsm_State_मूल्य final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_किल्ली;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3_संख्या m_अंक;  // Original folded value, preserving width for lowered comparisons

    स्थिर string make_किल्ली(स्थिरांक V3_संख्या& अंक) {
        V3_संख्या किल्ली_अंक = अंक;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        किल्ली_अंक.isSigned(असत्य);
        परत cvtToStr(किल्ली_अंक.width()) + ":" + किल्ली_अंक.ascii(सत्य, सत्य);
    }

सार्वजनिक:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    Fsm_State_मूल्य()
        : m_किल्ली{"1:1'h0"}
        , m_अंक{static_cast<Ast_गाठ*>(nullptr), 1, 0} {}
    explicit Fsm_State_मूल्य(स्थिरांक V3_संख्या& अंक)
        : m_किल्ली{make_किल्ली(अंक)}
        , m_अंक{अंक} {}

    स्थिरांक string& किल्ली() स्थिरांक { परत m_किल्ली; }
    स्थिरांक V3_संख्या& अंक() स्थिरांक { परत m_अंक; }
    string ascii() स्थिरांक { परत m_अंक.ascii(सत्य, सत्य); }
    string warnText() स्थिरांक {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        जर (m_अंक.width() <= 32) परत cvtToStr(m_अंक.toUInt());
        परत ascii();
    }

    तार्किक चालक==(स्थिरांक Fsm_State_मूल्य& rhs) स्थिरांक { परत m_किल्ली == rhs.m_किल्ली; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
संरचना Fsm_State_मूल्य_Hash final {
    माप_प्रकार चालक()(स्थिरांक Fsm_State_मूल्य& मूल्य) स्थिरांक {
        परत std::hash<string>{}(मूल्य.किल्ली());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
संरचना FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
संरचना FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    तार्किक active_खालचा = असत्य;
};

वर्ग FsmResetArcDesc final {
    Fsm_State_मूल्य m_to_मूल्य;  // Encoded reset target state.
    Ast_गाठ* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    Ast_गाठ_अभिव्यक्ती* m_valuep = nullptr;  // Expression that provided the reset value.

सार्वजनिक:
    FsmResetArcDesc() = पूर्वनिर्धारित;
    FsmResetArcDesc(Fsm_State_मूल्य to_मूल्य, Ast_गाठ_Assign* nodep)
        : m_to_मूल्य{to_मूल्य}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(Fsm_State_मूल्य to_मूल्य, Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* valuep)
        : m_to_मूल्य{to_मूल्य}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    Fsm_State_मूल्य to_मूल्य() स्थिरांक { परत m_to_मूल्य; }
    Ast_गाठ* nodep() स्थिरांक { परत m_nodep; }
    Ast_गाठ_अभिव्यक्ती* valuep() स्थिरांक { परत m_valuep; }
};

संरचना FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    तार्किक has_Rst_Active_खालचा = असत्य;
    तार्किक rst_Active_खालचा = असत्य;
};

स्थिर तार्किक fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, स्थिरांक string& portName,
                                                  तार्किक& active_खालचा) {
    तार्किक matched = असत्य;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        Ast_गाठ_Var_Ref* स्थिरांक vrefp = itemp->varrefp();
        जर (!vrefp) परत;
        जर (vrefp->varp()->name() != portName) परत;
        active_खालचा = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = सत्य;
    });
    परत matched;
}

स्थिर स्थिरांक V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    Ast_गाठ_Module* स्थिरांक modp = cellp->modp();
    स्थिरांक string origName = modp->origName();
    जर (स्थिरांक V3Control::FsmRegisterWrapper* स्थिरांक descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        परत descp;
    }
    परत V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

स्थिर FsmWrapperRoles rolesFromDesc(स्थिरांक V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.घड़ी;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.reset_मूल्य;
    परत roles;
}

वर्ग FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    तार्किक m_hasResetCond = असत्य;  // Whether the FSM had a modeled reset predicate.
    तार्किक m_inclCond = असत्य;  // Whether conditional/default arcs are kept explicitly.
    File_ओळ* m_flp = nullptr;  // Representative source location.

सार्वजनिक:
    AstScope* scopep() स्थिरांक { परत m_scopep; }
    शून्य scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() स्थिरांक { परत m_alwaysp; }
    शून्य alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() स्थिरांक { परत m_stateVscp; }
    शून्य stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() स्थिरांक { परत m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    शून्य sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() स्थिरांक { परत m_nextVscp; }
    शून्य nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    स्थिरांक std::vector<FsmSenDesc>& senses() स्थिरांक { परत m_senses; }
    std::vector<FsmSenDesc>& senses() { परत m_senses; }
    स्थिरांक FsmResetCondDesc& resetCond() स्थिरांक { परत m_resetCond; }
    FsmResetCondDesc& resetCond() { परत m_resetCond; }
    स्थिरांक std::vector<FsmResetArcDesc>& resetArcs() स्थिरांक { परत m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { परत m_resetArcs; }
    तार्किक hasResetCond() स्थिरांक { परत m_hasResetCond; }
    शून्य hasResetCond(तार्किक flag) { m_hasResetCond = flag; }
    तार्किक inclCond() स्थिरांक { परत m_inclCond; }
    शून्य inclCond(तार्किक flag) { m_inclCond = flag; }
    File_ओळ* fileline() स्थिरांक { परत m_flp; }
    शून्य fileline(File_ओळ* flp) { m_flp = flp; }
};

वर्ग FsmComboAlways final {
    AstScope* स्थिरांक m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* स्थिरांक m_alwaysp = nullptr;  // Candidate transition process.

सार्वजनिक:
    FsmComboAlways() = पूर्वनिर्धारित;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() स्थिरांक { परत m_scopep; }
    AstAlways* alwaysp() स्थिरांक { परत m_alwaysp; }
};

वर्ग FsmGraph;

वर्ग FsmVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

सार्वजनिक:
    गणना वर्ग Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

खाजगी:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    Fsm_State_मूल्य m_मूल्य;  // Encoded state value for real state vertices.

संरक्षित:
    FsmVertex(V3Graph* graphp, Kind kind, string label, Fsm_State_मूल्य मूल्य) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_मूल्य{मूल्य} {}
    ~FsmVertex() override = पूर्वनिर्धारित;

सार्वजनिक:
    Kind kind() स्थिरांक { परत m_kind; }
    तार्किक isState() स्थिरांक { परत m_kind == Kind::STATE; }
    तार्किक isResetAny() स्थिरांक { परत m_kind == Kind::RESET_ANY; }
    तार्किक isDefaultAny() स्थिरांक { परत m_kind == Kind::DEFAULT_ANY; }
    स्थिरांक string& label() स्थिरांक { परत m_label; }
    स्थिरांक Fsm_State_मूल्य& मूल्य() स्थिरांक { परत m_मूल्य; }

    string name() स्थिरांक override VL_MT_SAFE { परत m_label + "=" + m_मूल्य.ascii(); }
};

वर्ग FsmStateVertex final : सार्वजनिक FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

सार्वजनिक:
    FsmStateVertex(V3Graph* graphp, string label, Fsm_State_मूल्य मूल्य) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, मूल्य} {}
    ~FsmStateVertex() override = पूर्वनिर्धारित;

    string dotColor() स्थिरांक override { परत "lightblue"; }
    string dotShape() स्थिरांक override { परत "ellipse"; }
};

वर्ग FsmPseudoVertex final : सार्वजनिक FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

सार्वजनिक:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, Fsm_State_मूल्य{}} {}
    ~FsmPseudoVertex() override = पूर्वनिर्धारित;

    string name() स्थिरांक override VL_MT_SAFE { परत label(); }
    string dotColor() स्थिरांक override { परत isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() स्थिरांक override { परत "diamond"; }
};

वर्ग FsmArcEdge final : सार्वजनिक V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    तार्किक m_isReset = असत्य;  // Arc originates from the synthetic reset source.
    तार्किक m_isCond = असत्य;  // Arc came from a conditional next-state split.
    तार्किक m_isDefault = असत्य;  // Arc represents a case default source.
    File_ओळ* m_flp = nullptr;  // Source location for emitted coverage metadata.

सार्वजनिक:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* शीर्ष, तार्किक isReset, तार्किक isCond,
               तार्किक isDefault, File_ओळ* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, शीर्ष, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = पूर्वनिर्धारित;

    तार्किक isReset() स्थिरांक { परत m_isReset; }
    तार्किक isCond() स्थिरांक { परत m_isCond; }
    तार्किक isDefault() स्थिरांक { परत m_isDefault; }
    File_ओळ* fileline() स्थिरांक { परत m_flp; }

    string dotLabel() स्थिरांक override {
        जर (m_isReset) परत "reset";
        जर (m_isDefault) परत "default";
        जर (m_isCond) परत "cond";
        परत "";
    }
    string dotColor() स्थिरांक override {
        जर (m_isReset) परत "darkgreen";
        जर (m_isDefault) परत "orange";
        जर (m_isCond) परत "blue";
        परत "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
वर्ग FsmGraph final : सार्वजनिक V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    तार्किक m_hasResetCond = असत्य;  // Whether the detected FSM had a reset branch.
    तार्किक m_inclCond = असत्य;  // Whether conditional arcs should be kept explicitly.
    File_ओळ* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<Fsm_State_मूल्य, FsmStateVertex*, Fsm_State_मूल्य_Hash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

सार्वजनिक:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{नवीन FsmPseudoVertex{हे, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{नवीन FsmPseudoVertex{हे, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() स्थिरांक { परत m_scopep; }
    शून्य scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() स्थिरांक { परत m_stateAlwaysp; }
    शून्य stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    स्थिरांक string& stateVarName() स्थिरांक { परत m_stateVarName; }
    शून्य stateVarName(स्थिरांक string& name) { m_stateVarName = name; }
    स्थिरांक string& stateVarInternalName() स्थिरांक { परत m_stateVarInternalName; }
    शून्य stateVarInternalName(स्थिरांक string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() स्थिरांक { परत m_stateVarScopep; }
    शून्य stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() स्थिरांक {
        परत m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    शून्य sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    स्थिरांक std::vector<FsmSenDesc>& senses() स्थिरांक { परत m_senses; }
    std::vector<FsmSenDesc>& senses() { परत m_senses; }
    स्थिरांक FsmResetCondDesc& resetCond() स्थिरांक { परत m_resetCond; }
    FsmResetCondDesc& resetCond() { परत m_resetCond; }
    तार्किक hasResetCond() स्थिरांक { परत m_hasResetCond; }
    शून्य hasResetCond(तार्किक flag) { m_hasResetCond = flag; }
    तार्किक inclCond() स्थिरांक { परत m_inclCond; }
    शून्य inclCond(तार्किक flag) { m_inclCond = flag; }
    File_ओळ* fileline() स्थिरांक { परत m_flp; }
    शून्य fileline(File_ओळ* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, Fsm_State_मूल्य मूल्य) VL_MT_DISABLED {
        FsmStateVertex* स्थिरांक vertexp = नवीन FsmStateVertex{हे, label, मूल्य};
        m_stateVertices.emplace(मूल्य, vertexp);
        परत vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { परत m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { परत m_defaultVertexp; }
    FsmArcEdge* addArc(Fsm_State_मूल्य from_मूल्य, Fsm_State_मूल्य to_मूल्य, तार्किक isReset, तार्किक isCond,
                       तार्किक isDefault, File_ओळ* flp) VL_MT_DISABLED {
        FsmStateVertex* स्थिरांक शीर्ष = m_stateVertices.at(to_मूल्य);
        FsmVertex* fromp = nullptr;
        जर (isReset) {
            fromp = resetAnyVertex();
        } अन्यथा जर (isDefault) {
            fromp = defaultAnyVertex();
        } अन्यथा {
            fromp = m_stateVertices.at(from_मूल्य);
        }
        परत नवीन FsmArcEdge{हे, fromp, शीर्ष, isReset, isCond, isDefault, flp};
    }

    string name() स्थिरांक VL_MT_SAFE {
        परत "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(माप_प्रकार index) स्थिरांक {
        string tag = stateVarInternalName();
        साठी (अक्षर& ch : tag) {
            जर (!std::है_अक्षर_अंक(static_cast<अचिन्हित अक्षर>(ch))) ch = '_';
        }
        परत "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

संरचना DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

संरचना FsmCaseCandidate final {
    Ast_गाठ* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
संरचना FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    Ast_गाठ_अभिव्यक्ती* valuep = nullptr;  // Compared constant value expression
    Fsm_State_मूल्य मूल्य;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
संरचना FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    Ast_गाठ* stmtsp = nullptr;  // Branch body
    Ast_गाठ_अभिव्यक्ती* valuep = nullptr;  // Source state value expression
    Fsm_State_मूल्य from_मूल्य;  // Encoded source state value
    तार्किक has_शीर्ष_Guard = असत्य;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
संरचना FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    Ast_गाठ* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<स्थिरांक AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<स्थिरांक AstCell*, FsmCellPortMap>;

संरचना StateConstLabel final {
    string text;
    तार्किक fromParam = असत्य;
    माप_प्रकार stateIndex = 0;
};

संरचना FsmStateSpace final {
    std::vector<std::pair<string, Fsm_State_मूल्य>> states;  // User label and encoded value
    std::unordered_map<Fsm_State_मूल्य, StateConstLabel, Fsm_State_मूल्य_Hash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    तार्किक enumBacked = असत्य;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
वर्ग FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<स्थिरांक AstVarScope*, माप_प्रकार> m_fsmIndex;

सार्वजनिक:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        स्थिरांक std::map<स्थिरांक AstVarScope*, माप_प्रकार>::const_iterator it = m_fsmIndex.find(stateVscp);
        जर (it != m_fsmIndex.end()) परत m_fsms.at(it->second);
        स्थिरांक माप_प्रकार index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        परत m_fsms.back();
    }
    स्थिरांक std::vector<DetectedFsm>& fsms() स्थिरांक { परत m_fsms; }
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
    std::unordered_map<स्थिरांक AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<स्थिरांक AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    स्थिर Ast_गाठ_DType* unwrapEnumCandidate(Ast_गाठ_DType* dtypep) {
        परत dtypep->skipRefToEnump();
    }

    स्थिर string candidateConflictContext(Ast_गाठ* laterNodep,
                                           स्थिरांक FsmCaseCandidate& firstCand) {
        परत '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    स्थिर तार्किक rejectFsmWrapperCell(AstCell* cellp, स्थिरांक string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        परत असत्य;
    }

    स्थिर तार्किक simple_Param_State_मूल्य(AstCell* cellp, स्थिरांक string& name, Fsm_State_मूल्य& मूल्य,
                                      Ast_गाठ_अभिव्यक्ती*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        साठी (Ast_गाठ* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* स्थिरांक varp = VN_CAST(stmtp, Var);
            जर (!varp || !varp->isParam() || varp->name() != name) सुरूठेवा;
            valuepr = VN_AS(varp->valuep(), गाठ_अभिव्यक्ती);
            परत const_मूल्य_Status(valuepr, मूल्य) == Const_मूल्य_Status::OK;
        }
        परत असत्य;
    }

    स्थिर तार्किक childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        जर (!vscp->varp()->isIO()) परत असत्य;
        AstScope* स्थिरांक scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        जर (scopep->aboveScopep() != parentScopep) परत असत्य;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        परत सत्य;
    }

    स्थिर AstVarScope* simpleAssignVarScope(Ast_गाठ_अभिव्यक्ती* exprp) {
        AstVarRef* स्थिरांक vrefp = VN_CAST(exprp, VarRef);
        परत vrefp ? vrefp->varScopep() : nullptr;
    }

    शून्य addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        स्थिरांक std::pair<AstScope*, AstCell*> item{scopep, cellp};
        जर (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            परत;
        }
        m_wrapperCells.emplace_back(item);
    }

    शून्य collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* स्थिरांक lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* स्थिरांक rhsVscp = simpleAssignVarScope(nodep->rhsp());
        जर (!lhsVscp || !rhsVscp) परत;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        जर (childPortInScope(lhsVscp, m_scopep, cellp)) {
            जर (!fsmRegisterWrapperDesc(cellp)) परत;
            UASSERT_OBJ(lhsVscp->varp()->is_आदान(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } अन्यथा जर (childPortInScope(rhsVscp, m_scopep, cellp)) {
            जर (!fsmRegisterWrapperDesc(cellp)) परत;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, स्थिरांक string& portName) स्थिरांक {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        स्थिरांक FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        स्थिरांक FsmCellPortMap::const_iterator portIt = ports.find(portName);
        परत portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, स्थिरांक string& portName) स्थिरांक {
        स्थिरांक FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        स्थिरांक FsmCellPortMap::const_iterator portIt = ports.find(portName);
        परत portIt == ports.end() ? nullptr : portIt->second;
    }

    वर्ग RegisterAlwaysAnalyzer final {
        AstScope* स्थिरांक m_scopep;  // Scope containing the always block(s) being analyzed

    सार्वजनिक:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, Ast_गाठ_अभिव्यक्ती*>>
        oneBlockCandidates(AstAlways* alwaysp) स्थिरांक {
            std::vector<std::pair<AstCase*, Ast_गाठ_अभिव्यक्ती*>> candidates;
            Ast_गाठ* स्थिरांक stmtsp = alwaysp->stmtsp();
            जर (AstIf* स्थिरांक firstIfp = VN_CAST(stmtsp, If)) {
                जर (AstCase* स्थिरांक casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            साठी (Ast_गाठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                जर (AstCase* स्थिरांक casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            परत candidates;
        }

        std::vector<std::pair<AstIf*, Ast_गाठ_अभिव्यक्ती*>>
        oneBlockIfCandidates(AstAlways* alwaysp) स्थिरांक {
            std::vector<std::pair<AstIf*, Ast_गाठ_अभिव्यक्ती*>> candidates;
            Ast_गाठ* स्थिरांक stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            जर (AstIf* स्थिरांक firstIfp = VN_CAST(stmtsp, If)) {
                जर (AstIf* स्थिरांक chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            साठी (Ast_गाठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                जर (AstIf* स्थिरांक ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            परत candidates;
        }

        तार्किक matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) स्थिरांक {
            परत FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        शून्य buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, Ast_गाठ_अभिव्यक्ती* resetCondp,
                                    FsmRegisterCandidate& reg) स्थिरांक {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describe_Sen_वृक्ष(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* स्थिरांक firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            जर (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                स्थिरांक ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                जर (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } अन्यथा जर (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    Fsm_State_मूल्य reset_मूल्य;
                    Ast_गाठ* स्थिरांक thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    Ast_गाठ_Assign* स्थिरांक resetAssp = FsmDetectVisitor::direct_Const_State_Assign_गाठ(
                        thenNodep, resetStateVscp, reset_मूल्य);
                    जर (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(reset_मूल्य, resetAssp);
                    }
                } अन्यथा जर (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    तार्किक matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) स्थिरांक {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* स्थिरांक nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* स्थिरांक stateVscp = roleVarScope(cellp, roles.qPort);
        जर (!nextVscp || !stateVscp) {
            परत rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* स्थिरांक clkVscp = roleVarScope(cellp, roles.clkPort);
        जर (!clkVscp) {
            परत rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().ढकला_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        जर (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        जर (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            तार्किक inferred_Active_खालचा = असत्य;
            जर (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferred_Active_खालचा)) {
                roles.has_Rst_Active_खालचा = सत्य;
                roles.rst_Active_खालचा = inferred_Active_खालचा;
            }
        }

        Ast_गाठ_अभिव्यक्ती* resetValuep = nullptr;
        Fsm_State_मूल्य reset_मूल्य;
        स्थिरांक तार्किक has_Reset_मूल्य
            = !roles.rstValParam.empty()
              && simple_Param_State_मूल्य(cellp, roles.rstValParam, reset_मूल्य, resetValuep);
        जर (resetVscp && roles.has_Rst_Active_खालचा && has_Reset_मूल्य) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rst_Active_खालचा ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().ढकला_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().active_खालचा = roles.rst_Active_खालचा;
            cand.hasResetCond(सत्य);
            cand.resetArcs().emplace_back(reset_मूल्य, cellp, resetValuep);
        } अन्यथा जर (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            जर (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } अन्यथा जर (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } अन्यथा जर (!roles.has_Rst_Active_खालचा) {
                reason = "reset polarity could not be inferred from the wrapper";
            } अन्यथा जर (roles.rstValParam.empty()) {
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
        cand.alwaysp(nullptr);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(childRoleVarScope(cellp, roles.qPort));
        cand.nextVscp(nextVscp);
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(cellp->fileline());
        परत सत्य;
    }

    वर्ग ComboAlwaysAnalyzer final {
    सार्वजनिक:
        संरचना ComboMatch final {
            स्थिरांक FsmRegisterCandidate* matchedp = nullptr;
            Ast_गाठ* warnNodep = nullptr;
        };

    खाजगी:
        स्थिरांक std::vector<FsmRegisterCandidate>& m_registerCandidates;

    सार्वजनिक:
        explicit ComboAlwaysAnalyzer(स्थिरांक std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(Ast_गाठ* stmtsp, AstCase* casep) स्थिरांक {
            ComboMatch match;
            AstVarRef* स्थिरांक selp = VN_CAST(casep->exprp(), VarRef);
            जर (!selp) परत match;
            साठी (स्थिरांक FsmRegisterCandidate& reg : m_registerCandidates) {
                जर (selp->varScopep() == reg.nextVscp()) {
                    जर (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        सुरूठेवा;
                    }
                } अन्यथा जर (selp->varScopep() != reg.stateVscp()) {
                    सुरूठेवा;
                }
                Ast_गाठ* स्थिरांक warnNodep = FsmDetectVisitor::case_Supported_Transition_गाठ(
                    casep, reg.nextVscp(), reg.inclCond());
                जर (!warnNodep) सुरूठेवा;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            परत match;
        }

        ComboMatch matchIfChain(Ast_गाठ* stmtsp, स्थिरांक FsmIfChainCandidate& chain) स्थिरांक {
            ComboMatch match;
            साठी (स्थिरांक FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                जर (chain.compareVscp == reg.nextVscp()) {
                    जर (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        सुरूठेवा;
                    }
                } अन्यथा जर (chain.compareVscp != reg.stateVscp()) {
                    सुरूठेवा;
                }
                Ast_गाठ* स्थिरांक warnNodep
                    = FsmDetectVisitor::if_Chain_Supported_Transition_गाठ(chain, reg.nextVscp());
                जर (!warnNodep) सुरूठेवा;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            परत match;
        }

        तार्किक shouldWarnUnsupported(Ast_गाठ* stmtsp, AstCase* casep) स्थिरांक {
            स्थिरांक AstVarRef* स्थिरांक selp = VN_CAST(casep->exprp(), VarRef);
            जर (!selp) परत असत्य;

            परत std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](स्थिरांक FsmRegisterCandidate& reg) -> तार्किक {
                    स्थिरांक तार्किक matchesNext = selp->varScopep() == reg.nextVscp();
                    स्थिरांक तार्किक matchesState = selp->varScopep() == reg.stateVscp();

                    जर (!matchesNext && !matchesState) परत असत्य;
                    जर (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        परत असत्य;
                    }
                    परत FsmDetectVisitor::case_Supported_Transition_गाठ(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    स्थिर तार्किक isSimpleResetCond(Ast_गाठ_अभिव्यक्ती* condp) { परत VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    स्थिर FsmResetCondDesc describeResetCond(Ast_गाठ_अभिव्यक्ती* condp) {
        FsmResetCondDesc desc;
        जर (AstVarRef* स्थिरांक vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        परत desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    स्थिर std::vector<FsmSenDesc> describe_Sen_वृक्ष(Ast_Sen_वृक्ष* sentreep) {
        std::vector<FsmSenDesc> senses;
        साठी (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            Ast_गाठ_Var_Ref* स्थिरांक vrefp = itemp->varrefp();
            जर (!vrefp) सुरूठेवा;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.ढकला_back(desc);
        }
        परत senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    स्थिर तार्किक isIgnorableStmt(Ast_गाठ* nodep) { परत VN_IS(nodep, CoverInc); }

    स्थिर Ast_गाठ* skipLeadingIgnorableStmt(Ast_गाठ* nodep) {
        पर्यंत (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        परत nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    स्थिर Ast_गाठ* singleMeaningfulStmt(Ast_गाठ* stmtp) {
        Ast_गाठ* resultp = nullptr;
        साठी (Ast_गाठ* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            जर (isIgnorableStmt(nodep)) सुरूठेवा;
            जर (resultp) परत nullptr;
            resultp = nodep;
        }
        परत resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    स्थिर Ast_गाठ* singleMeaningfulBranch(Ast_गाठ* branchp) {
        जर (!branchp) परत nullptr;
        परत branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    स्थिर तार्किक isPlainComboSentree(स्थिरांक Ast_Sen_वृक्ष* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        साठी (स्थिरांक AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            जर (senp->edgeType() == VEdgeType::ET_CHANGED) सुरूठेवा;
            परत असत्य;
        }
        परत सत्य;
    }

    शून्य warnUnsupportedComboAlways(स्थिरांक FsmComboAlways& combo) {
        स्थिरांक ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_गाठ* स्थिरांक stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        तार्किक warned = असत्य;
        साठी (Ast_गाठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* स्थिरांक casep = VN_CAST(nodep, Case);
            जर (!casep) सुरूठेवा;
            जर (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = सत्य;
            }
            जर (warned) थांबा;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    स्थिर Ast_गाठ_Assign* directStateAssign(Ast_गाठ* stmtp, AstVarScope* stateVscp) {
        Ast_गाठ* स्थिरांक nodep = singleMeaningfulBranch(stmtp);
        जर (!nodep) परत nullptr;
        Ast_गाठ_Assign* स्थिरांक assp = VN_CAST(nodep, गाठ_Assign);
        जर (!assp) परत nullptr;
        AstVarRef* स्थिरांक vrefp = VN_CAST(assp->lhsp(), VarRef);
        जर (!vrefp || vrefp->varScopep() != stateVscp) परत nullptr;
        परत assp;
    }

    स्थिर Ast_गाठ_Assign* गाठ_State_Var_Assign(Ast_गाठ* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        Ast_गाठ_Assign* स्थिरांक assp = VN_CAST(nodep, गाठ_Assign);
        जर (!assp) परत nullptr;
        AstVarRef* स्थिरांक lhsp = VN_CAST(assp->lhsp()->baseFromp(सत्य), VarRef);
        AstVarRef* स्थिरांक rhsp = VN_CAST(assp->rhsp(), VarRef);
        जर (!rhsp || !lhsp) परत nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        परत assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    स्थिर Ast_गाठ_Assign* directCondStateVarAssign(Ast_गाठ* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, Ast_गाठ_अभिव्यक्ती*& condp,
                                                   Fsm_State_मूल्य& reset_मूल्य) {
        Ast_गाठ_Assign* स्थिरांक assp = VN_CAST(nodep, गाठ_Assign);
        जर (!assp) परत nullptr;
        AstVarRef* स्थिरांक lhsp = VN_CAST(assp->lhsp()->baseFromp(सत्य), VarRef);
        AstCond* स्थिरांक rhsp = VN_CAST(assp->rhsp(), Cond);
        जर (!rhsp || !lhsp) परत nullptr;
        AstVarRef* स्थिरांक elsep = VN_CAST(rhsp->elsep(), VarRef);
        जर (!elsep) परत nullptr;
        जर (const_मूल्य_Status(rhsp->thenp(), reset_मूल्य) != Const_मूल्य_Status::OK) परत nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        परत assp;
    }

    स्थिर Ast_गाठ_Assign* direct_Const_State_Assign_गाठ(Ast_गाठ* nodep, AstVarScope*& stateVscp,
                                                     Fsm_State_मूल्य& मूल्य) {
        Ast_गाठ_Assign* स्थिरांक assp = VN_CAST(nodep, गाठ_Assign);
        जर (!assp) परत nullptr;
        AstVarRef* स्थिरांक lhsp = VN_CAST(assp->lhsp()->baseFromp(सत्य), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        जर (const_मूल्य_Status(assp->rhsp(), मूल्य) != Const_मूल्य_Status::OK) परत nullptr;
        stateVscp = lhsp->varScopep();
        परत assp;
    }

    गणना वर्ग ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    स्थिर ResetAssignStatus collectConstStateAssigns(Ast_गाठ* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        Ast_गाठ* nodep = skipLeadingIgnorableStmt(stmtp);
        जर (!nodep) परत ResetAssignStatus::EMPTY;
        साठी (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            Fsm_State_मूल्य मूल्य;
            Ast_गाठ_Assign* स्थिरांक assp = direct_Const_State_Assign_गाठ(nodep, assignStateVscp, मूल्य);
            जर (!assp) परत ResetAssignStatus::NONE;
            जर (!stateVscp) stateVscp = assignStateVscp;
            जर (assignStateVscp != stateVscp) परत ResetAssignStatus::NONE;
            जर (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                परत ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(मूल्य, assp);
            जर (!nodep->nextp()) परत ResetAssignStatus::SINGLE;
        }
    }

    स्थिर तार्किक hasCanonicalNextStateDefaultBeforeCase(Ast_गाठ* stmtsp, Ast_गाठ* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        Ast_गाठ* स्थिरांक bodyp = skipLeadingIgnorableStmt(stmtsp);
        तार्किक sawCanonicalDefault = असत्य;
        साठी (Ast_गाठ* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            जर (nodep == targetp) परत sawCanonicalDefault;
            जर (Ast_गाठ_Assign* स्थिरांक assp = VN_CAST(nodep, गाठ_Assign)) {
                AstVarRef* स्थिरांक lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* स्थिरांक rhsp = VN_CAST(assp->rhsp(), VarRef);
                जर (!lhsp || lhsp->varScopep() != nextVscp) सुरूठेवा;
                जर (sawCanonicalDefault) {
                    स्थिरांक string nextName = nextVscp->varp()->prettyNameQ();
                    स्थिरांक string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    परत असत्य;
                }
                जर (!rhsp || rhsp->varScopep() != stateVscp) परत असत्य;
                sawCanonicalDefault = सत्य;
            }
        }
    }

    स्थिर तार्किक ifStateConstAssign(Ast_गाठ* stmtp, AstVarScope* stateVscp,
                                   Fsm_State_मूल्य& then_मूल्य, Fsm_State_मूल्य& else_मूल्य) {
        AstIf* स्थिरांक ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        जर (!ifp || !ifp->elsesp()) परत असत्य;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        Ast_गाठ* स्थिरांक thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        Ast_गाठ* स्थिरांक elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        जर (!thenNodep || !elseNodep) परत असत्य;
        जर (!direct_Const_State_Assign_गाठ(thenNodep, thenVscp, then_मूल्य)) परत असत्य;
        जर (!direct_Const_State_Assign_गाठ(elseNodep, elseVscp, else_मूल्य)) परत असत्य;
        जर (thenVscp == stateVscp && elseVscp == stateVscp) परत सत्य;
        जर (thenVscp != elseVscp) परत असत्य;
        Ast_गाठ* स्थिरांक followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        Ast_गाठ* स्थिरांक finalNodep = singleMeaningfulBranch(followp);
        जर (!finalNodep) परत असत्य;
        जर (!गाठ_State_Var_Assign(finalNodep, finalStateVscp, finalFromVscp)) परत असत्य;
        जर (finalStateVscp != stateVscp) परत असत्य;
        जर (finalFromVscp != thenVscp) परत असत्य;
        परत सत्य;
    }

    स्थिर तार्किक directStateCondConstAssign(Ast_गाठ* stmtp, AstVarScope* stateVscp,
                                           Fsm_State_मूल्य& then_मूल्य, Fsm_State_मूल्य& else_मूल्य) {
        Ast_गाठ_Assign* स्थिरांक assp = directStateAssign(stmtp, stateVscp);
        जर (!assp) परत असत्य;
        AstCond* स्थिरांक condp = VN_CAST(assp->rhsp(), Cond);
        जर (!condp) परत असत्य;
        परत const_मूल्य_Status(condp->thenp(), then_मूल्य) == Const_मूल्य_Status::OK
               && const_मूल्य_Status(condp->elsep(), else_मूल्य) == Const_मूल्य_Status::OK;
    }

    स्थिर Ast_गाठ* case_Item_Supported_Arc_गाठ(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             तार्किक inclCond) {
        जर (itemp->isDefault()) {
            जर (!inclCond) परत nullptr;
        }
        Ast_गाठ_Assign* स्थिरांक assp = directStateAssign(itemp->stmtsp(), stateVscp);
        जर (assp) {
            Fsm_State_मूल्य to_मूल्य;
            जर (const_मूल्य_Status(assp->rhsp(), to_मूल्य) == Const_मूल्य_Status::OK) परत assp;
        }
        Fsm_State_मूल्य then_मूल्य;
        Fsm_State_मूल्य else_मूल्य;
        जर (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_मूल्य, else_मूल्य)) {
            परत assp;
        }
        जर (ifStateConstAssign(itemp->stmtsp(), stateVscp, then_मूल्य, else_मूल्य)) {
            परत singleMeaningfulBranch(itemp->stmtsp());
        }
        परत nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    स्थिर Ast_गाठ* case_Supported_Transition_गाठ(AstCase* casep, AstVarScope* stateVscp,
                                                तार्किक inclCond) {
        साठी (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            जर (Ast_गाठ* स्थिरांक nodep = case_Item_Supported_Arc_गाठ(itemp, stateVscp, inclCond))
                परत nodep;
        }
        परत nullptr;
    }

    स्थिर Ast_गाठ* case_Item_Supported_Arc_गाठ_Like(Ast_गाठ* stmtsp, AstVarScope* stateVscp) {
        जर (Ast_गाठ_Assign* स्थिरांक assp = directStateAssign(stmtsp, stateVscp)) {
            Fsm_State_मूल्य to_मूल्य;
            जर (const_मूल्य_Status(assp->rhsp(), to_मूल्य) == Const_मूल्य_Status::OK) परत assp;
            Fsm_State_मूल्य then_मूल्य;
            Fsm_State_मूल्य else_मूल्य;
            जर (directStateCondConstAssign(stmtsp, stateVscp, then_मूल्य, else_मूल्य)) परत assp;
        }
        परत nullptr;
    }

    स्थिर Ast_गाठ* if_Chain_Supported_Transition_गाठ(स्थिरांक FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        साठी (माप_प्रकार i = 0; i < chain.branches.size(); ++i) {
            स्थिरांक FsmIfBranch& branch = chain.branches[i];
            Ast_गाठ* स्थिरांक nodep = case_Item_Supported_Arc_गाठ_Like(branch.stmtsp, stateVscp);
            जर (!nodep) परत nullptr;
        }
        परत chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    स्थिर string label_For_मूल्य(
        स्थिरांक std::unordered_map<Fsm_State_मूल्य, StateConstLabel, Fsm_State_मूल्य_Hash>& labels,
        स्थिरांक Fsm_State_मूल्य& मूल्य) {
        परत labels.at(मूल्य).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    गणना वर्ग Const_मूल्य_Status : uint8_t { OK, NOT_CONST, XZ };

    स्थिर Const_मूल्य_Status const_मूल्य_Status(Ast_गाठ_अभिव्यक्ती* exprp, Fsm_State_मूल्य& मूल्य) {
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(exprp, Const);
        जर (!constp) परत Const_मूल्य_Status::NOT_CONST;
        स्थिरांक V3_संख्या& अंक = constp->अंक();
        जर (अंक.isAnyXZ()) परत Const_मूल्य_Status::XZ;
        मूल्य = Fsm_State_मूल्य{अंक};
        परत Const_मूल्य_Status::OK;
    }

    स्थिर तार्किक pureStateComparisonNoAlias(Ast_गाठ_अभिव्यक्ती* exprp, FsmStateComparison& cmp) {
        AstEq* स्थिरांक eqp = VN_CAST(exprp, Eq);
        जर (!eqp) परत असत्य;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        Ast_गाठ_अभिव्यक्ती* valuep = eqp->rhsp();
        जर (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(सत्य), VarRef);
            जर (!vrefp) { परत असत्य; }
            valuep = eqp->lhsp();
        }

        Fsm_State_मूल्य मूल्य;
        जर (const_मूल्य_Status(valuep, मूल्य) != Const_मूल्य_Status::OK) परत असत्य;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.मूल्य = मूल्य;
        परत सत्य;
    }

    स्थिर तार्किक pureStateComparison(Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        जर (pureStateComparisonNoAlias(exprp, cmp)) परत सत्य;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        जर (AstVarRef* स्थिरांक vrefp = VN_CAST(exprp, VarRef)) {
            स्थिरांक FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            जर (it == aliases.end()) परत असत्य;
            cmp = it->second;
            परत सत्य;
        }
        परत असत्य;
    }

    स्थिर तार्किक supported_शीर्ष_Level_Guard(Ast_गाठ_अभिव्यक्ती* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        जर (VN_IS(exprp, Or)) परत असत्य;
        जर (VN_IS(exprp, RedAnd)) परत असत्य;
        जर (VN_IS(exprp, RedOr)) परत असत्य;
        जर (VN_IS(exprp, RedXor)) परत असत्य;
        परत सत्य;
    }

    स्थिर तार्किक resolveIfPredicate(Ast_गाठ_अभिव्यक्ती* exprp, स्थिरांक FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, तार्किक& hasGuard) {
        std::vector<Ast_गाठ_अभिव्यक्ती*> terms;
        std::vector<Ast_गाठ_अभिव्यक्ती*> pending;
        pending.ढकला_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        पर्यंत (!pending.empty()) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक nodep = pending.back();
            pending.काढा_back();
            जर (AstAnd* स्थिरांक andp = VN_CAST(nodep, And)) {
                pending.ढकला_back(andp->rhsp());
                pending.ढकला_back(andp->lhsp());
            } अन्यथा {
                terms.ढकला_back(nodep);
            }
        }

        तार्किक sawComparison = असत्य;
        साठी (माप_प्रकार i = 0; i < terms.size(); ++i) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक termp = terms[i];
            FsmStateComparison termCmp;
            जर (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                जर (sawComparison) परत असत्य;
                cmp = termCmp;
                sawComparison = सत्य;
                सुरूठेवा;
            }
            जर (!supported_शीर्ष_Level_Guard(termp)) परत असत्य;
            hasGuard = सत्य;
        }
        परत sawComparison;
    }

    स्थिर शून्य addAlias(FsmAliasMap& aliases, std::unordered_set<स्थिरांक AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, स्थिरांक FsmStateComparison& cmp) {
        जर (ambiguous.find(aliasVscp) != ambiguous.end()) परत;
        स्थिरांक FsmAliasMap::iterator it = aliases.find(aliasVscp);
        जर (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            परत;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        जर (it->second.stateVscp == cmp.stateVscp && it->second.मूल्य == cmp.मूल्य) परत;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        परत;
    }

    स्थिर शून्य collectAliasFromAssign(Ast_गाठ_Assign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<स्थिरांक AstVarScope*>& ambiguous) {
        AstVarRef* स्थिरांक lhsp = VN_CAST(assp->lhsp(), VarRef);
        जर (!lhsp) परत;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        जर (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) परत;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(Ast_गाठ* stmtsp, Ast_गाठ* limitp) स्थिरांक {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<स्थिरांक AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        साठी (Ast_गाठ* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            जर (Ast_गाठ_Assign* स्थिरांक assp = VN_CAST(nodep, गाठ_Assign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        साठी (स्थिरांक AstVarScope* स्थिरांक vscp : ambiguous) aliases.erase(vscp);
        परत aliases;
    }

    स्थिर तार्किक collectIfChain(AstIf* ifp, स्थिरांक FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seen_मूल्ये;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        पर्यंत (सत्य) {
            FsmStateComparison cmp;
            तार्किक hasGuard = असत्य;
            जर (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) परत असत्य;
            जर (chain.compareVscp && chain.compareVscp != cmp.stateVscp) परत असत्य;
            जर (!seen_मूल्ये.घाला(cmp.मूल्य.किल्ली()).second) परत असत्य;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.ढकला_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.मूल्य, hasGuard});

            Ast_गाठ* स्थिरांक elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            जर (!elseNodep) थांबा;
            जर (AstIf* स्थिरांक elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                सुरूठेवा;
            }
            chain.defaultStmtsp = elseNodep;
            थांबा;
        }
        परत chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    स्थिर तार्किक validate_Known_State_मूल्य(Ast_गाठ* nodep, स्थिरांक FsmStateSpace& stateSpace,
                                        स्थिरांक Fsm_State_मूल्य& मूल्य, स्थिरांक string& role) {
        जर (stateSpace.labels.find(मूल्य) != stateSpace.labels.end()) परत सत्य;
        जर (stateSpace.enumBacked) {
            स्थिरांक string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + मूल्य.warnText()
                                        + " is not present in the declared enum");
            परत असत्य;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + मूल्य.warnText()
                                    + " is not present in the inferred state space");
        परत असत्य;
    }

    स्थिर StateConstLabel stateLabelForConst(AstConst* constp) {
        स्थिरांक string name = constp->origParamName();
        जर (!name.empty()) परत StateConstLabel{Ast_गाठ::prettyName(name), सत्य, 0};
        परत StateConstLabel{constp->name(), असत्य, 0};
    }

    स्थिर शून्य updateStateLabel(FsmStateSpace& stateSpace, स्थिरांक Fsm_State_मूल्य& मूल्य,
                                 स्थिरांक StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(मूल्य).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    स्थिर तार्किक matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        जर (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) परत असत्य;

        Ast_गाठ* स्थिरांक stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        Ast_गाठ* स्थिरांक nodep = singleMeaningfulStmt(stmtsp);
        जर (!nodep) परत असत्य;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        जर (AstIf* स्थिरांक ifp = VN_CAST(nodep, If)) {
            जर (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) परत असत्य;
            AstVarScope* resetStateVscp = nullptr;
            स्थिरांक ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            स्थिरांक तार्किक emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            जर (emptyResetBranch) {
                cand.resetArcs().clear();
            } अन्यथा जर (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                Fsm_State_मूल्य reset_मूल्य;
                Ast_गाठ* स्थिरांक thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                Ast_गाठ_Assign* स्थिरांक resetAssp
                    = direct_Const_State_Assign_गाठ(thenNodep, resetStateVscp, reset_मूल्य);
                जर (!resetAssp) परत असत्य;
                cand.resetArcs().emplace_back(reset_मूल्य, resetAssp);
            } अन्यथा जर (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            Ast_गाठ* स्थिरांक elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            जर (!गाठ_State_Var_Assign(elseNodep, stateVscp, nextVscp)) परत असत्य;
            जर (!emptyResetBranch && resetStateVscp != stateVscp) परत असत्य;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } अन्यथा {
            Ast_गाठ_अभिव्यक्ती* resetCondp = nullptr;
            Fsm_State_मूल्य reset_मूल्य;
            // Note the active low 'reset ? next : CONST' form is not accepted
            जर (Ast_गाठ_Assign* स्थिरांक assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, reset_मूल्य)) {
                cand.resetArcs().emplace_back(reset_मूल्य, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } अन्यथा जर (!गाठ_State_Var_Assign(nodep, stateVscp, nextVscp)) {
                परत असत्य;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describe_Sen_वृक्ष(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        परत सत्य;
    }

    स्थिर तार्किक add_मूल्य_To_State_Space(Ast_गाठ* nodep, FsmStateSpace& stateSpace,
                                     स्थिरांक Fsm_State_मूल्य& मूल्य, StateConstLabel label) {
        स्थिरांक स्वतः labelIt = stateSpace.labels.find(मूल्य);
        जर (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            जर (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + मूल्य.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                परत असत्य;
            }
            जर (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, मूल्य, label);
            }
        } अन्यथा {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, मूल्य);
            stateSpace.labels.emplace(मूल्य, storedLabel);
        }
        परत सत्य;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    स्थिर तार्किक add_अभिव्यक्ती_To_State_Space(Ast_गाठ_अभिव्यक्ती* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_मूल्य मूल्य;
        स्थिरांक Const_मूल्य_Status status = const_मूल्य_Status(valuep, मूल्य);
        जर (status != Const_मूल्य_Status::OK) {
            जर (status == Const_मूल्य_Status::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            परत असत्य;
        }
        AstConst* स्थिरांक constp = VN_AS(valuep, Const);
        परत add_मूल्य_To_State_Space(valuep, stateSpace, मूल्य, stateLabelForConst(constp));
    }

    स्थिर तार्किक add_Optional_Target_अभिव्यक्ती_To_State_Space(Ast_गाठ_अभिव्यक्ती* valuep, FsmStateSpace& stateSpace) {
        Fsm_State_मूल्य मूल्य;
        स्थिरांक Const_मूल्य_Status status = const_मूल्य_Status(valuep, मूल्य);
        जर (status != Const_मूल्य_Status::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            परत असत्य;
        }
        AstConst* स्थिरांक constp = VN_AS(valuep, Const);
        परत add_मूल्य_To_State_Space(valuep, stateSpace, मूल्य, stateLabelForConst(constp));
    }

    स्थिर शून्य addResetTargetsToStateSpace(स्थिरांक std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        साठी (स्थिरांक FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.to_मूल्य().ascii(), असत्य, 0};
            जर (AstConst* स्थिरांक constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                add_मूल्य_To_State_Space(resetArc.nodep(), stateSpace, resetArc.to_मूल्य(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    स्थिर तार्किक collectDeclaredStateSpace(Ast_गाठ* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, तार्किक& needs_Source_मूल्ये) {
        AstVar* स्थिरांक stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        जर (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        स्थिरांक तार्किक forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        जर (enump) {
            stateSpace.enumBacked = सत्य;
            साठी (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                स्थिरांक AstConst* स्थिरांक constp = VN_AS(itemp->valuep(), Const);
                स्थिरांक Fsm_State_मूल्य मूल्य{constp->अंक()};
                स्थिरांक माप_प्रकार stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), मूल्य);
                stateSpace.labels.emplace(मूल्य,
                                          StateConstLabel{itemp->name(), असत्य, stateIndex});
            }
            परत stateSpace.states.size() >= 2;
        }

        जर (forced) {
            needs_Source_मूल्ये = सत्य;
            परत सत्य;
        }

        needs_Source_मूल्ये = सत्य;
        परत सत्य;
    }

    ढाँचा <typename T_ValuepVisitor>
    स्थिर तार्किक collect_State_Space_From_मूल्ये(Ast_गाठ* warnNodep, AstVarScope* stateVscp,
                                            स्थिरांक std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            स्थिरांक T_ValuepVisitor& visitValueps) {
        तार्किक needs_Source_मूल्ये = असत्य;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        जर (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needs_Source_मूल्ये)) {
            परत असत्य;
        }
        जर (!needs_Source_मूल्ये) परत सत्य;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        जर (!visitValueps(
                [&](Ast_गाठ_अभिव्यक्ती* valuep) { परत add_अभिव्यक्ती_To_State_Space(valuep, stateSpace); })) {
            परत असत्य;
        }
        परत stateSpace.states.size() >= 2;
    }

    स्थिर तार्किक collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  स्थिरांक std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        परत collect_State_Space_From_मूल्ये(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](स्थिरांक स्वतः& visitValuep) {
                साठी (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    जर (!itemp->isDefault()) {
                        साठी (Ast_गाठ_अभिव्यक्ती* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), गाठ_अभिव्यक्ती)) {
                            जर (!visitValuep(condp)) परत असत्य;
                        }
                    }
                    जर (Ast_गाठ_Assign* स्थिरांक assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        Fsm_State_मूल्य then_मूल्य;
                        Fsm_State_मूल्य else_मूल्य;
                        AstCond* स्थिरांक condp = VN_CAST(assp->rhsp(), Cond);
                        जर (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, then_मूल्य,
                                                          else_मूल्य)) {
                            जर (!visitValuep(condp->thenp())) परत असत्य;
                            जर (!visitValuep(condp->elsep())) परत असत्य;
                        } अन्यथा जर (!add_Optional_Target_अभिव्यक्ती_To_State_Space(assp->rhsp(), stateSpace)) {
                            परत असत्य;
                        }
                    }
                }
                परत सत्य;
            });
    }

    स्थिर तार्किक collectStateSpace(स्थिरांक FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  स्थिरांक std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        परत collect_State_Space_From_मूल्ये(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](स्थिरांक स्वतः& visitValuep) {
                साठी (स्थिरांक FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    Ast_गाठ_Assign* स्थिरांक assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    Fsm_State_मूल्य then_मूल्य;
                    Fsm_State_मूल्य else_मूल्य;
                    AstCond* स्थिरांक condp = VN_CAST(assp->rhsp(), Cond);
                    जर (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               then_मूल्य, else_मूल्य),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        जर (!visitValuep(condp->thenp())) परत असत्य;
                        जर (!visitValuep(condp->elsep())) परत असत्य;
                    } अन्यथा जर (!add_Optional_Target_अभिव्यक्ती_To_State_Space(assp->rhsp(), stateSpace)) {
                        परत असत्य;
                    }
                }
                परत सत्य;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    स्थिर तार्किक emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 स्थिरांक FsmStateSpace& stateSpace, तार्किक inclCond) {
        std::vector<std::pair<string, Fsm_State_मूल्य>> froms;
        जर (itemp->isDefault()) {
            जर (!inclCond) परत असत्य;
            froms.emplace_back("default", Fsm_State_मूल्य{});
        } अन्यथा {
            साठी (Ast_गाठ_अभिव्यक्ती* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), गाठ_अभिव्यक्ती)) {
                Fsm_State_मूल्य मूल्य;
                जर (const_मूल्य_Status(condp, मूल्य) != Const_मूल्य_Status::OK) सुरूठेवा;
                जर (!validate_Known_State_मूल्य(condp, stateSpace, मूल्य, "source")) परत सत्य;
                froms.emplace_back(label_For_मूल्य(stateSpace.labels, मूल्य), मूल्य);
            }
            जर (froms.empty()) परत असत्य;
        }

        जर (Ast_गाठ_Assign* स्थिरांक assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            Fsm_State_मूल्य to_मूल्य;
            स्थिरांक Const_मूल्य_Status status = const_मूल्य_Status(assp->rhsp(), to_मूल्य);
            जर (status == Const_मूल्य_Status::OK) {
                जर (!validate_Known_State_मूल्य(assp, stateSpace, to_मूल्य, "target")) परत सत्य;
                साठी (स्थिरांक std::pair<string, Fsm_State_मूल्य>& from : froms) {
                    graph.addArc(from.second, to_मूल्य, असत्य, असत्य, itemp->isDefault(),
                                 assp->fileline());
                }
                परत सत्य;
            }
        }

        Fsm_State_मूल्य then_मूल्य;
        Fsm_State_मूल्य else_मूल्य;
        जर (directStateCondConstAssign(itemp->stmtsp(), stateVscp, then_मूल्य, else_मूल्य)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, then_मूल्य, else_मूल्य)) {
            जर (!validate_Known_State_मूल्य(itemp->stmtsp(), stateSpace, then_मूल्य, "target"))
                परत सत्य;
            जर (!validate_Known_State_मूल्य(itemp->stmtsp(), stateSpace, else_मूल्य, "target"))
                परत सत्य;
            साठी (स्थिरांक Fsm_State_मूल्य& branch_मूल्य : {then_मूल्य, else_मूल्य}) {
                साठी (स्थिरांक std::pair<string, Fsm_State_मूल्य>& from : froms) {
                    graph.addArc(from.second, branch_मूल्य, असत्य, सत्य, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            परत सत्य;
        }

        परत असत्य;
    }

    स्थिर शून्य emitStmtArcsFrom(FsmGraph& graph, Ast_गाठ* stmtsp, AstVarScope* stateVscp,
                                 स्थिरांक FsmStateSpace& stateSpace, Fsm_State_मूल्य from_मूल्य,
                                 तार्किक isDefault, तार्किक forceCond) {
        Ast_गाठ_Assign* स्थिरांक assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        Fsm_State_मूल्य to_मूल्य;
        स्थिरांक Const_मूल्य_Status status = const_मूल्य_Status(assp->rhsp(), to_मूल्य);
        जर (status == Const_मूल्य_Status::OK) {
            जर (!validate_Known_State_मूल्य(assp, stateSpace, to_मूल्य, "target")) परत;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(from_मूल्य, to_मूल्य, असत्य, forceCond, isDefault, assp->fileline());
            परत;
        }

        Fsm_State_मूल्य then_मूल्य;
        Fsm_State_मूल्य else_मूल्य;
        स्थिरांक तार्किक condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, then_मूल्य, else_मूल्य);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        जर (!validate_Known_State_मूल्य(stmtsp, stateSpace, then_मूल्य, "target")) परत;
        जर (!validate_Known_State_मूल्य(stmtsp, stateSpace, else_मूल्य, "target")) परत;
        साठी (स्थिरांक Fsm_State_मूल्य& branch_मूल्य : {then_मूल्य, else_मूल्य}) {
            graph.addArc(from_मूल्य, branch_मूल्य, असत्य, सत्य, isDefault, stmtsp->fileline());
        }
    }

    स्थिर शून्य emitIfChainArcs(FsmGraph& graph, स्थिरांक FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, स्थिरांक FsmStateSpace& stateSpace) {
        साठी (माप_प्रकार i = 0; i < chain.branches.size(); ++i) {
            स्थिरांक FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            जर (!validate_Known_State_मूल्य(branch.ifp, stateSpace, branch.from_मूल्य, "source"))
                परत;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.from_मूल्य, असत्य,
                             branch.has_शीर्ष_Guard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    स्थिर शून्य addResetArcs(FsmGraph& graph, स्थिरांक std::vector<FsmResetArcDesc>& resetArcs,
                             स्थिरांक FsmStateSpace& stateSpace) {
        साठी (स्थिरांक FsmResetArcDesc& resetArc : resetArcs) {
            जर (!validate_Known_State_मूल्य(resetArc.nodep(), stateSpace, resetArc.to_मूल्य(),
                                         "target"))
                सुरूठेवा;
            graph.addArc(Fsm_State_मूल्य{}, resetArc.to_मूल्य(), सत्य, असत्य, असत्य,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    शून्य processCase(AstCase* casep, AstVarScope* assignVscp, स्थिरांक FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* स्थिरांक stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        जर (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) परत;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        जर (!entry.graphp) {
            entry.graphp.reset(नवीन FsmGraph{});
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
            साठी (स्थिरांक std::pair<string, Fsm_State_मूल्य>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        साठी (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    शून्य processIfChain(स्थिरांक FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        स्थिरांक FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* स्थिरांक stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        जर (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) परत;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(नवीन FsmGraph{});
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
        साठी (स्थिरांक std::pair<string, Fsm_State_मूल्य>& state : stateSpace.states) {
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
    शून्य processOneBlockAlways(स्थिरांक FsmComboAlways& oneBlock) {
        स्थिरांक RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* स्थिरांक alwaysp = oneBlock.alwaysp();
        जर (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) परत;
        स्थिरांक std::vector<std::pair<AstCase*, Ast_गाठ_अभिव्यक्ती*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        साठी (स्थिरांक std::pair<AstCase*, Ast_गाठ_अभिव्यक्ती*>& cand : candidates) {
            AstVarRef* स्थिरांक selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* स्थिरांक vscp = selp ? selp->varScopep() : nullptr;
            जर (!vscp) सुरूठेवा;
            जर (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } अन्यथा जर (vscp != firstCand.stateVscp) {
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

        जर (firstCand.stateVscp) परत;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        स्थिरांक std::vector<std::pair<AstIf*, Ast_गाठ_अभिव्यक्ती*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        साठी (स्थिरांक std::pair<AstIf*, Ast_गाठ_अभिव्यक्ती*>& cand : ifCandidates) {
            स्थिरांक FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            जर (!collectIfChain(cand.first, aliases, chain)) सुरूठेवा;
            AstVarScope* स्थिरांक vscp = chain.compareVscp;
            जर (!if_Chain_Supported_Transition_गाठ(chain, vscp)) सुरूठेवा;
            जर (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } अन्यथा जर (vscp != firstCand.stateVscp) {
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
    स्थिर शून्य warnComboSameAlways(Ast_गाठ* warnNodep, स्थिरांक FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    शून्य processComboAlways(स्थिरांक FsmComboAlways& combo) {
        स्थिरांक ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        Ast_गाठ* स्थिरांक stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        साठी (Ast_गाठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* स्थिरांक casep = VN_CAST(nodep, Case);
            जर (!casep) सुरूठेवा;
            स्थिरांक ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            स्थिरांक FsmRegisterCandidate* स्थिरांक matchedp = match.matchedp;
            Ast_गाठ* स्थिरांक matchedWarnNodep = match.warnNodep;
            जर (!matchedp) सुरूठेवा;
            जर (!firstCand.stateVscp) {
                स्थिरांक स्वतः घाला_Pair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                जर (!घाला_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  घाला_Pair.first->second));
                    सुरूठेवा;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                सुरूठेवा;
            }
            जर (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                सुरूठेवा;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        जर (firstCand.stateVscp) परत;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        साठी (Ast_गाठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* स्थिरांक ifp = VN_CAST(nodep, If);
            जर (!ifp) सुरूठेवा;
            FsmIfChainCandidate chain;
            स्थिरांक FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            जर (!collectIfChain(ifp, aliases, chain)) सुरूठेवा;
            स्थिरांक ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            स्थिरांक FsmRegisterCandidate* स्थिरांक matchedp = match.matchedp;
            Ast_गाठ* स्थिरांक matchedWarnNodep = match.warnNodep;
            जर (!matchedp) सुरूठेवा;
            जर (!firstCand.stateVscp) {
                स्थिरांक std::pair<std::unordered_map<स्थिरांक AstVarScope*, FsmCaseCandidate>::iterator,
                                तार्किक>
                    घाला_Pair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                जर (!घाला_Pair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  घाला_Pair.first->second));
                    सुरूठेवा;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                सुरूठेवा;
            }
            जर (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                सुरूठेवा;
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
    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    शून्य visit(AstAlways* nodep) override {
        जर (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            परत;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        स्थिरांक RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        जर (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* स्थिरांक stateVscp = reg.stateVscp();
            स्थिरांक तार्किक found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](स्थिरांक FsmRegisterCandidate& existing) {
                                  परत existing.stateVscp() == stateVscp;
                              });
            जर (!found) { m_registerCandidates.emplace_back(reg); }
        }
        जर (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } अन्यथा जर (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            जर (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } अन्यथा {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    शून्य visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    शून्य visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        जर (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        साठी (स्थिरांक std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            जर (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        साठी (स्थिरांक FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        साठी (स्थिरांक FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        साठी (स्थिरांक FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
वर्ग FsmCoverIncBuilder final {
    स्थिरांक FsmGraph& m_graph;  // Fsm graph
    std::vector<स्थिरांक FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    स्थिरांक FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    स्थिरांक FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    माप_प्रकार m_edges_मोजणी = 0;  // Total count of edges
    AstVarScope* स्थिरांक m_prevVscp;  // Previous state of fsm
    File_ओळ* स्थिरांक m_flp;  // Fileline pointer to use across the builder
    स्थिरांक std::string m_modPrettyName;  // Pretty fsm mode name
    Ast_गाठ_Stmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    Ast_गाठ_Stmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    तार्किक m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(स्थिरांक uint64_t मूल्य) {
        V3_संख्या अंक{static_cast<Ast_गाठ*>(nullptr), m_prevVscp->width()};
        अंक.setQuad(मूल्य);
        अंक.isSigned(m_prevVscp->dtypep()->isSigned());
        परत नवीन AstConst{m_flp, अंक};
    }

    std::string quadToBitStirngLabel(स्थिरांक uint64_t val) {
        पूर्णांक width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        पर्यंत (--width >= 0) result.ढकला_back("01"[(val >> width) & 1]);
        परत result;
    }

    शून्य initExistingStates() {
        साठी (स्थिरांक V3GraphVertex& vtx : m_graph.vertices()) {
            स्थिरांक FsmVertex* स्थिरांक vertexp = vtx.as<FsmVertex>();
            m_edges_मोजणी += vertexp->inEdges().size();
            जर (स्थिरांक FsmStateVertex* स्थिरांक statep = vertexp->cast<FsmStateVertex>()) {
                m_states.ढकला_back(statep);
            } अन्यथा जर (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } अन्यथा /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        स्थिरांक पूर्णांक width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<माप_प्रकार>(1) << width);
    }

    शून्य initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        स्थिरांक uint64_t combinations_मोजणी = static_cast<माप_प्रकार>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        तार्किक undefRolled = असत्य;
        स्वतः rollUndefs = [हे, combinations_मोजणी] {
            साठी (uint64_t i = 0; i < combinations_मोजणी; ++i) {
                स्थिरांक स्वतः घाला = m_undefinedStates.घाला(i);
                जर (!घाला.second) m_undefinedStates.erase(घाला.first);
            }
        };
        साठी (स्थिरांक FsmStateVertex* स्थिरांक statep : m_states) {
            स्थिरांक V3_संख्या& अंक = statep->मूल्य().अंक();
            स्थिरांक uint64_t मूल्य = अंक.toUQuad();
            जर (!m_isComplete) {
                जर (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(मूल्य) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } अन्यथा {
                    UASSERT_OBJ(m_undefinedStates.घाला(मूल्य).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    जर (m_undefinedStates.size() * 2 >= combinations_मोजणी) {
                        undefRolled = सत्य;
                        rollUndefs();
                    }
                }
            }
        }
        जर (!(m_isComplete || undefRolled)) rollUndefs();
    }

    Ast_गाठ_अभिव्यक्ती* buildResetCond() स्थिरांक {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक refp
            = नवीन AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        परत m_graph.resetCond().active_खालचा
                   ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstLogNot{m_flp, refp})
                   : refp;
    }

    माप_प्रकार get_Arc_मोजणी_With_Expansion(स्थिरांक VFsmExpandType expand) स्थिरांक {
        माप_प्रकार arc_मोजणी_With_Expansion = ~0ULL;
        निवड (expand) {
        स्थिती VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            थांबा;
        }
        स्थिती VFsmExpandType::AUTO: {
            arc_मोजणी_With_Expansion = m_edges_मोजणी;
            जर (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arc_मोजणी_With_Expansion -= m_defaultVertexp->outEdges().size();
                साठी (स्थिरांक FsmStateVertex* स्थिरांक statep : m_states) {
                    arc_मोजणी_With_Expansion += static_cast<माप_प्रकार>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            जर (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arc_मोजणी_With_Expansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            थांबा;
        }
        स्थिती VFsmExpandType::AUTO_EXPAND: {
            arc_मोजणी_With_Expansion = m_states.size() * m_states.size() - m_states.size();
            थांबा;
        }
        स्थिती VFsmExpandType::FULL: {
            स्थिरांक पूर्णांक width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            जर (width < 32) {
                स्थिरांक uint64_t combinations_मोजणी = static_cast<माप_प्रकार>(1) << width;
                arc_मोजणी_With_Expansion = combinations_मोजणी * combinations_मोजणी - combinations_मोजणी;
            }
            थांबा;
        }
        }
        परत arc_मोजणी_With_Expansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(स्थिरांक std::string& label,
                                           स्थिरांक std::string& fsmTag = "") {
        AstCoverOtherDecl* स्थिरांक declp
            = नवीन AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(सत्य);
        m_declsp = Ast_गाठ::addNext(m_declsp, declp);
        परत declp;
    }

    Ast_गाठ_Stmt* buildCoverIncStmtp(स्थिरांक std::string& fromStateLabel,
                                    स्थिरांक std::string& toStateLabel,
                                    स्थिरांक std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* स्थिरांक declp = नवीन AstCoverOtherDecl{
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
        declp->perInstance(सत्य);
        m_declsp = Ast_गाठ::addNext(m_declsp, declp);
        परत नवीन AstCoverInc{m_flp, declp};
    }
    Ast_गाठ_Stmt* buildExpandedTransitionHandler(स्थिरांक std::string& toStateLabel,
                                                स्थिरांक तार्किक considerUndefs,
                                                स्थिरांक तार्किक expandOnlyWithTerminalVerticies,
                                                स्थिरांक FsmStateVertex* स्थिरांक toStatep = nullptr) {
        AstCase* स्थिरांक arcCasep = नवीन AstCase{
            m_flp, VCaseType::CT_CASE, नवीन AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<स्थिरांक FsmStateVertex*> handledSoureces;
        जर (toStatep) {
            साठी (स्थिरांक V3GraphEdge& edge : toStatep->inEdges()) {
                जर (स्थिरांक FsmStateVertex* स्थिरांक fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    जर (!handledSoureces.emplace(fromp).second) सुरूठेवा;
                    arcCasep->addItemsp(नवीन AstCaseItem{
                        m_flp, makeConst(fromp->मूल्य().अंक().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        साठी (स्थिरांक FsmStateVertex* स्थिरांक fromp : m_states) {
            जर (handledSoureces.find(fromp) != handledSoureces.end()) सुरूठेवा;
            arcCasep->addItemsp(नवीन AstCaseItem{
                m_flp, makeConst(fromp->मूल्य().अंक().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        जर (!m_isComplete) {
            जर (considerUndefs) {
                साठी (स्थिरांक uint64_t from : m_undefinedStates) {
                    स्थिरांक std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        नवीन AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } अन्यथा {
                arcCasep->addItemsp(नवीन AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        परत arcCasep;
    }
    Ast_गाठ_Stmt* buildUnxpandedTransitionHandler(स्थिरांक FsmStateVertex* स्थिरांक toStatep) {
        AstCase* स्थिरांक arcCasep = नवीन AstCase{
            m_flp, VCaseType::CT_CASE, नवीन AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        साठी (स्थिरांक V3GraphEdge& edge : toStatep->inEdges()) {
            स्थिरांक FsmArcEdge* स्थिरांक arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            स्थिरांक FsmStateVertex* स्थिरांक fromp = arcp->fromp()->as<FsmStateVertex>();
            जर (fromp != toStatep) {
                arcCasep->addItemsp(
                    नवीन AstCaseItem{m_flp, makeConst(fromp->मूल्य().अंक().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(नवीन AstCaseItem{m_flp, nullptr, nullptr});
        परत arcCasep;
    }

सार्वजनिक:
    FsmCoverIncBuilder(स्थिरांक FsmGraph& graph, AstVarScope* स्थिरांक prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* स्थिरांक sampleVscp = graph.sampleVarScopep();
        स्थिरांक VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            स्थिरांक माप_प्रकार arc_मोजणी_With_Expansion = get_Arc_मोजणी_With_Expansion(expand);
            जर (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arc_मोजणी_With_Expansion == 0ULL
                            || arc_मोजणी_With_Expansion
                                   > static_cast<माप_प्रकार>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arc_मोजणी_With_Expansion);
                परत;
            }
        }

        AstCase* स्थिरांक arcCasep
            = नवीन AstCase{m_flp, VCaseType::CT_CASE,
                          नवीन AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* स्थिरांक initialStateCasep
            = नवीन AstCase{m_flp, VCaseType::CT_CASE,
                          नवीन AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        स्थिरांक तार्किक expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<स्थिरांक FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<स्थिरांक FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        जर (!expandAll) {
            जर (m_defaultVertexp) {
                साठी (स्थिरांक V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.शीर्ष()->as<FsmStateVertex>());
                }
            }
            जर (m_resetVertexp) {
                साठी (स्थिरांक V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.शीर्ष()->as<FsmStateVertex>());
                }
            }
        }
        स्थिरांक तार्किक isFullExpansion = expand == VFsmExpandType::FULL;
        जर (isFullExpansion) initUndefinedStates();
        साठी (स्थिरांक FsmStateVertex* स्थिरांक statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* स्थिरांक declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                नवीन AstCaseItem{m_flp, makeConst(statep->मूल्य().अंक().toUQuad()),
                                नवीन AstCoverInc{m_flp, declp, सत्य}});
            स्थिरांक तार्किक expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            स्थिरांक तार्किक defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(नवीन AstCaseItem{
                m_flp, makeConst(statep->मूल्य().अंक().toUQuad()),
                Ast_गाठ::addNext<Ast_गाठ_Stmt>(
                    नवीन AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        जर (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            जर (isFullExpansion) {
                साठी (स्थिरांक uint64_t मूल्य : m_undefinedStates) {
                    स्थिरांक std::string& label = quadToBitStirngLabel(मूल्य);
                    AstCoverOtherDecl* स्थिरांक declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(नवीन AstCaseItem{
                        m_flp, makeConst(मूल्य), नवीन AstCoverInc{m_flp, declp, सत्य}});
                    arcCasep->addItemsp(
                        नवीन AstCaseItem{m_flp, makeConst(मूल्य),
                                        Ast_गाठ::addNext<Ast_गाठ_Stmt>(
                                            नवीन AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, सत्य, असत्य))});
                }
            } अन्यथा {
                // No expansion
                arcCasep->addItemsp(नवीन AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(नवीन AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = पूर्वनिर्धारित;
    Ast_गाठ_Cover_Decl* getCoverDeclsp() स्थिरांक { परत m_declsp; }
    Ast_गाठ_Stmt* getCoverIncp() स्थिरांक { परत m_coverIncp; }
    Ast_गाठ_Stmt* getInitialCoverIncp() स्थिरांक { परत m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
वर्ग FsmLowerVisitor final {
    // STATE - across all visitors
    स्थिरांक FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    स्थिर Ast_Sen_वृक्ष* build_Sen_वृक्ष(File_ओळ* flp, स्थिरांक std::vector<FsmSenDesc>& senses) {
        Ast_Sen_वृक्ष* स्थिरांक sentreep = नवीन Ast_Sen_वृक्ष{flp, nullptr};
        साठी (स्थिरांक FsmSenDesc& sense : senses) {
            AstSenItem* स्थिरांक senItemp
                = नवीन AstSenItem{flp, VEdgeType{sense.edgeType},
                                 नवीन AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        परत sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    शून्य buildOne(स्थिरांक FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* स्थिरांक alwaysp = graph.stateAlwaysp();
        AstScope* स्थिरांक scopep = graph.scopep();
        AstVarScope* स्थिरांक stateVscp = graph.stateVarScopep();
        AstVarScope* स्थिरांक sampleVscp = graph.sampleVarScopep();
        File_ओळ* स्थिरांक flp = graph.fileline();
        Ast_गाठ_Module* स्थिरांक modp = scopep->modp();
        AstVarScope* स्थिरांक prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(सत्य);
        sampleVscp->optimizeLifePost(सत्य);
        prevVscp->optimizeLifePost(सत्य);

        AstActive* स्थिरांक initActivep
            = नवीन AstActive{flp, "fsm-coverage-init",
                            नवीन Ast_Sen_वृक्ष{flp, नवीन AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->sen_वृक्ष_Storep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(नवीन AstInitialStatic{
            flp, नवीन AstAssign{flp, नवीन AstVarRef{flp, prevVscp, VAccess::WRITE},
                               नवीन AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* स्थिरांक covPostp = नवीन AstAlwaysPost{flp};
        तार्किक updatePrevAfterPost = असत्य;
        जर (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 नवीन AstVarRef{flp, sampleVscp, VAccess::READ}});
        } अन्यथा {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = सत्य;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            स्थिरांक FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                नवीन AstIf{flp,
                          नवीन AstNeq{flp, नवीन AstVarRef{flp, prevVscp, VAccess::READ},
                                     नवीन AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        जर (updatePrevAfterPost) {
            covPostp->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              नवीन AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        Ast_Sen_वृक्ष* स्थिरांक sentreep = build_Sen_वृक्ष(flp, graph.senses());
        AstActive* स्थिरांक activep = नवीन AstActive{flp, "fsm-coverage", sentreep};
        activep->sen_वृक्ष_Storep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

सार्वजनिक:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(स्थिरांक FsmState& state)
        : m_state{state} {
        साठी (स्थिरांक DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

शून्य V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    जर (dumpGraphLevel() >= 6) {
        माप_प्रकार index = 0;
        साठी (स्थिरांक DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dump_Check_Global_वृक्ष("fsm-detect", 0, dump_वृक्ष_Either_Level() >= 3);
}
