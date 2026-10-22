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

#include "V3PchAstNoMT.h"

#include "V3FsmDetect.h"

#include "V3Ast.h"
#include "V3Control.h"
#include "V3Graph.h"
#include "V3SharedTmps.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {

// Width-preserving FSM state identity. FSM detection needs a stable key for
// graph vertices and lookup tables, but lowering still needs the original
// folded Verilog value so emitted comparisons keep the correct width and bits.
class FsmStateValue final {
    // Hash/equality key only. It deliberately ignores signedness because
    // signed and unsigned constants with the same width and bits denote the
    // same encoded FSM state.
    string m_key;  // Canonical "width:value" identity, independent of signedness

    // Semantic value. This is what diagnostics and lowering use when printing
    // values or rebuilding AstConst nodes for instrumentation.
    V3Number m_num;  // Original folded value, preserving width for lowered comparisons

    static string makeKey(const V3Number& num) {
        V3Number keyNum = num;
        // Signedness does not change FSM state identity: same width and bits
        // should address the same graph vertex.
        keyNum.isSigned(false);
        반환 cvtToStr(keyNum.width()) + ":" + keyNum.ascii(true, true);
    }

public:
    // Default value is used only for synthetic pseudo-states such as ANY and
    // default, which never use m_num as a real Verilog state encoding.
    FsmStateValue()
        : m_key{"1:1'h0"}
        , m_num{static_cast<AstNode*>(nullptr), 1, 0} {}
    explicit FsmStateValue(const V3Number& num)
        : m_key{makeKey(num)}
        , m_num{num} {}

    const string& key() const { 반환 m_key; }
    const V3Number& num() const { 반환 m_num; }
    string ascii() const { 반환 m_num.ascii(true, true); }
    string warnText() const {
        // Preserve legacy diagnostics for old <=32-bit FSMs, but print wide
        // values without truncation.
        만약 (m_num.width() <= 32) 반환 cvtToStr(m_num.toUInt());
        반환 ascii();
    }

    bool operator==(const FsmStateValue& rhs) const { 반환 m_key == rhs.m_key; }
};

// unordered_map needs an explicit hash for this custom key type. Keep the
// hash definition paired with operator== by hashing the same canonical key.
struct FsmStateValueHash final {
    size_t operator()(const FsmStateValue& value) const {
        반환 std::hash<string>{}(value.key());
    }
};

// Captures one sensitivity-list entry so the lowering phase can later rebuild
// an active block with the same triggering event control.
struct FsmSenDesc final {
    // Encoded edge kind copied from AstSenItem::edgeType() so lowering can
    // rebuild the same trigger semantics on the synthesized coverage block.
    VEdgeType::en edgeType = static_cast<VEdgeType::en>(0);
    // Triggering signal in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
};

// Captures the simple reset predicate shape that survives to this pass after
// earlier normalization so reset arcs can be reconstructed during lowering.
struct FsmResetCondDesc final {
    // Reset signal used by the FSM in the saved scoped AST.
    AstVarScope* varScopep = nullptr;
    bool activeLow = false;
};

class FsmResetArcDesc final {
    FsmStateValue m_toValue;  // Encoded reset target state.
    AstNode* m_nodep = nullptr;  // Source node for warnings and emitted metadata.
    AstNodeExpr* m_valuep = nullptr;  // Expression that provided the reset value.

public:
    FsmResetArcDesc() = default;
    FsmResetArcDesc(FsmStateValue toValue, AstNodeAssign* nodep)
        : m_toValue{toValue}
        , m_nodep{nodep}
        , m_valuep{nodep->rhsp()} {}
    FsmResetArcDesc(FsmStateValue toValue, AstNode* nodep, AstNodeExpr* valuep)
        : m_toValue{toValue}
        , m_nodep{nodep}
        , m_valuep{valuep} {}

    FsmStateValue toValue() const { 반환 m_toValue; }
    AstNode* nodep() const { 반환 m_nodep; }
    AstNodeExpr* valuep() const { 반환 m_valuep; }
};

struct FsmWrapperRoles final {
    string dPort;
    string qPort;
    string clkPort;
    string rstPort;
    string rstValParam;
    bool hasRstActiveLow = false;
    bool rstActiveLow = false;
};

static bool fsmWrapperResetPolarityFromWrapperAst(AstCell* cellp, const string& portName,
                                                  bool& activeLow) {
    bool matched = false;
    cellp->modp()->foreach([&](AstSenItem* itemp) {
        AstNodeVarRef* const vrefp = itemp->varrefp();
        만약 (!vrefp) 반환;
        만약 (vrefp->varp()->name() != portName) 반환;
        activeLow = itemp->edgeType() == VEdgeType::ET_NEGEDGE;
        matched = true;
    });
    반환 matched;
}

static const V3Control::FsmRegisterWrapper* fsmRegisterWrapperDesc(AstCell* cellp) {
    AstNodeModule* const modp = cellp->modp();
    const string origName = modp->origName();
    만약 (const V3Control::FsmRegisterWrapper* const descp
        = V3Control::getFsmRegisterWrapper(origName)) {
        반환 descp;
    }
    반환 V3Control::getFsmRegisterWrapper(modp->prettyDehashOrigOrName());
}

static FsmWrapperRoles rolesFromDesc(const V3Control::FsmRegisterWrapper& desc) {
    FsmWrapperRoles roles;
    roles.dPort = desc.d;
    roles.qPort = desc.q;
    roles.clkPort = desc.clock;
    roles.rstPort = desc.reset;
    roles.rstValParam = desc.resetValue;
    반환 roles;
}

class FsmRegisterCandidate final {
    AstScope* m_scopep = nullptr;  // Owning scope for the paired FSM.
    AstAlways* m_alwaysp = nullptr;  // Register process that commits the state.
    AstVarScope* m_stateVscp = nullptr;  // Registered FSM state variable.
    AstVarScope* m_sampleVscp = nullptr;  // Variable sampled by lowered coverage logic.
    AstVarScope* m_nextVscp = nullptr;  // Next-state variable or same state var for 1-block FSMs.
    std::vector<FsmSenDesc> m_senses;  // Event controls for recreated coverage blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate, if any.
    std::vector<FsmResetArcDesc> m_resetArcs;  // Reset target arcs recovered during detect.
    bool m_hasResetCond = false;  // Whether the FSM had a modeled reset predicate.
    bool m_inclCond = false;  // Whether conditional/default arcs are kept explicitly.
    FileLine* m_flp = nullptr;  // Representative source location.

public:
    AstScope* scopep() const { 반환 m_scopep; }
    void scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* alwaysp() const { 반환 m_alwaysp; }
    void alwaysp(AstAlways* alwaysp) { m_alwaysp = alwaysp; }
    AstVarScope* stateVscp() const { 반환 m_stateVscp; }
    void stateVscp(AstVarScope* vscp) { m_stateVscp = vscp; }
    AstVarScope* sampleVscp() const { 반환 m_sampleVscp ? m_sampleVscp : m_stateVscp; }
    void sampleVscp(AstVarScope* vscp) { m_sampleVscp = vscp; }
    AstVarScope* nextVscp() const { 반환 m_nextVscp; }
    void nextVscp(AstVarScope* vscp) { m_nextVscp = vscp; }
    const std::vector<FsmSenDesc>& senses() const { 반환 m_senses; }
    std::vector<FsmSenDesc>& senses() { 반환 m_senses; }
    const FsmResetCondDesc& resetCond() const { 반환 m_resetCond; }
    FsmResetCondDesc& resetCond() { 반환 m_resetCond; }
    const std::vector<FsmResetArcDesc>& resetArcs() const { 반환 m_resetArcs; }
    std::vector<FsmResetArcDesc>& resetArcs() { 반환 m_resetArcs; }
    bool hasResetCond() const { 반환 m_hasResetCond; }
    void hasResetCond(bool flag) { m_hasResetCond = flag; }
    bool inclCond() const { 반환 m_inclCond; }
    void inclCond(bool flag) { m_inclCond = flag; }
    FileLine* fileline() const { 반환 m_flp; }
    void fileline(FileLine* flp) { m_flp = flp; }
};

class FsmComboAlways final {
    AstScope* const m_scopep = nullptr;  // Owning scope for the combinational process.
    AstAlways* const m_alwaysp = nullptr;  // Candidate transition process.

public:
    FsmComboAlways() = default;
    FsmComboAlways(AstScope* scopep, AstAlways* alwaysp)
        : m_scopep{scopep}
        , m_alwaysp{alwaysp} {}

    AstScope* scopep() const { 반환 m_scopep; }
    AstAlways* alwaysp() const { 반환 m_alwaysp; }
};

class FsmGraph;

class FsmVertex VL_NOT_FINAL : public V3GraphVertex {
    VL_RTTI_IMPL(FsmVertex, V3GraphVertex)

public:
    enum class Kind : uint8_t { STATE, RESET_ANY, DEFAULT_ANY };

private:
    Kind m_kind;  // State vs synthetic ANY/default vertex role.
    string m_label;  // User-facing state or pseudo-state label.
    FsmStateValue m_value;  // Encoded state value for real state vertices.

protected:
    FsmVertex(V3Graph* graphp, Kind kind, string label, FsmStateValue value) VL_MT_DISABLED
        : V3GraphVertex{graphp},
          m_kind{kind},
          m_label{label},
          m_value{value} {}
    ~FsmVertex() override = default;

public:
    Kind kind() const { 반환 m_kind; }
    bool isState() const { 반환 m_kind == Kind::STATE; }
    bool isResetAny() const { 반환 m_kind == Kind::RESET_ANY; }
    bool isDefaultAny() const { 반환 m_kind == Kind::DEFAULT_ANY; }
    const string& label() const { 반환 m_label; }
    const FsmStateValue& value() const { 반환 m_value; }

    string name() const override VL_MT_SAFE { 반환 m_label + "=" + m_value.ascii(); }
};

class FsmStateVertex final : public FsmVertex {
    VL_RTTI_IMPL(FsmStateVertex, FsmVertex)

public:
    FsmStateVertex(V3Graph* graphp, string label, FsmStateValue value) VL_MT_DISABLED
        : FsmVertex{graphp, Kind::STATE, label, value} {}
    ~FsmStateVertex() override = default;

    string dotColor() const override { 반환 "lightblue"; }
    string dotShape() const override { 반환 "ellipse"; }
};

class FsmPseudoVertex final : public FsmVertex {
    VL_RTTI_IMPL(FsmPseudoVertex, FsmVertex)

public:
    FsmPseudoVertex(V3Graph* graphp, Kind kind, string label) VL_MT_DISABLED
        : FsmVertex{graphp, kind, label, FsmStateValue{}} {}
    ~FsmPseudoVertex() override = default;

    string name() const override VL_MT_SAFE { 반환 label(); }
    string dotColor() const override { 반환 isResetAny() ? "darkgreen" : "orange"; }
    string dotShape() const override { 반환 "diamond"; }
};

class FsmArcEdge final : public V3GraphEdge {
    VL_RTTI_IMPL(FsmArcEdge, V3GraphEdge)
    bool m_isReset = false;  // Arc originates from the synthetic reset source.
    bool m_isCond = false;  // Arc came from a conditional next-state split.
    bool m_isDefault = false;  // Arc represents a case default source.
    FileLine* m_flp = nullptr;  // Source location for emitted coverage metadata.

public:
    FsmArcEdge(V3Graph* graphp, FsmVertex* fromp, FsmStateVertex* top, bool isReset, bool isCond,
               bool isDefault, FileLine* flp) VL_MT_DISABLED : V3GraphEdge{graphp, fromp, top, 1},
                                                               m_isReset{isReset},
                                                               m_isCond{isCond},
                                                               m_isDefault{isDefault},
                                                               m_flp{flp} {}
    ~FsmArcEdge() override = default;

    bool isReset() const { 반환 m_isReset; }
    bool isCond() const { 반환 m_isCond; }
    bool isDefault() const { 반환 m_isDefault; }
    FileLine* fileline() const { 반환 m_flp; }

    string dotLabel() const override {
        만약 (m_isReset) 반환 "reset";
        만약 (m_isDefault) 반환 "default";
        만약 (m_isCond) 반환 "cond";
        반환 "";
    }
    string dotColor() const override {
        만약 (m_isReset) 반환 "darkgreen";
        만약 (m_isDefault) 반환 "orange";
        만약 (m_isCond) 반환 "blue";
        반환 "black";
    }
};

// One graph per detected FSM. Graph-level metadata captures the non-graph
// context needed to lower states/arcs back into the AST after detection.
class FsmGraph final : public V3Graph {
    AstScope* m_scopep = nullptr;  // Owning scoped block for the detected FSM.
    AstAlways* m_stateAlwaysp = nullptr;  // Register always block being instrumented.
    string m_stateVarName;  // Pretty state variable name for user-visible output.
    string m_stateVarInternalName;  // Internal state symbol name for dump tags.
    AstVarScope* m_stateVarScopep = nullptr;  // Scoped state variable being tracked.
    AstVarScope* m_sampleVarScopep = nullptr;  // Scoped variable sampled by coverage logic.
    std::vector<FsmSenDesc> m_senses;  // Saved event controls for recreated active blocks.
    FsmResetCondDesc m_resetCond;  // Saved reset predicate shape, if one exists.
    bool m_hasResetCond = false;  // Whether the detected FSM had a reset branch.
    bool m_inclCond = false;  // Whether conditional arcs should be kept explicitly.
    FileLine* m_flp = nullptr;  // Representative source location for declarations/arcs.
    std::unordered_map<FsmStateValue, FsmStateVertex*, FsmStateValueHash>
        m_stateVertices;  // Value to state map.
    FsmPseudoVertex* m_resetVertexp = nullptr;  // Synthetic ANY source for reset arcs.
    FsmPseudoVertex* m_defaultVertexp = nullptr;  // Synthetic default source for case defaults.

public:
    FsmGraph() VL_MT_DISABLED
        : m_resetVertexp{new FsmPseudoVertex{this, FsmVertex::Kind::RESET_ANY, "ANY"}},
          m_defaultVertexp{new FsmPseudoVertex{this, FsmVertex::Kind::DEFAULT_ANY, "default"}} {}

    AstScope* scopep() const { 반환 m_scopep; }
    void scopep(AstScope* scopep) { m_scopep = scopep; }
    AstAlways* stateAlwaysp() const { 반환 m_stateAlwaysp; }
    void stateAlwaysp(AstAlways* alwaysp) { m_stateAlwaysp = alwaysp; }
    const string& stateVarName() const { 반환 m_stateVarName; }
    void stateVarName(const string& name) { m_stateVarName = name; }
    const string& stateVarInternalName() const { 반환 m_stateVarInternalName; }
    void stateVarInternalName(const string& name) { m_stateVarInternalName = name; }
    AstVarScope* stateVarScopep() const { 반환 m_stateVarScopep; }
    void stateVarScopep(AstVarScope* vscp) { m_stateVarScopep = vscp; }
    AstVarScope* sampleVarScopep() const {
        반환 m_sampleVarScopep ? m_sampleVarScopep : m_stateVarScopep;
    }
    void sampleVarScopep(AstVarScope* vscp) { m_sampleVarScopep = vscp; }
    const std::vector<FsmSenDesc>& senses() const { 반환 m_senses; }
    std::vector<FsmSenDesc>& senses() { 반환 m_senses; }
    const FsmResetCondDesc& resetCond() const { 반환 m_resetCond; }
    FsmResetCondDesc& resetCond() { 반환 m_resetCond; }
    bool hasResetCond() const { 반환 m_hasResetCond; }
    void hasResetCond(bool flag) { m_hasResetCond = flag; }
    bool inclCond() const { 반환 m_inclCond; }
    void inclCond(bool flag) { m_inclCond = flag; }
    FileLine* fileline() const { 반환 m_flp; }
    void fileline(FileLine* flp) { m_flp = flp; }

    FsmStateVertex* addStateVertex(string label, FsmStateValue value) VL_MT_DISABLED {
        FsmStateVertex* const vertexp = new FsmStateVertex{this, label, value};
        m_stateVertices.emplace(value, vertexp);
        반환 vertexp;
    }
    FsmPseudoVertex* resetAnyVertex() VL_MT_DISABLED { 반환 m_resetVertexp; }
    FsmPseudoVertex* defaultAnyVertex() VL_MT_DISABLED { 반환 m_defaultVertexp; }
    FsmArcEdge* addArc(FsmStateValue fromValue, FsmStateValue toValue, bool isReset, bool isCond,
                       bool isDefault, FileLine* flp) VL_MT_DISABLED {
        FsmStateVertex* const top = m_stateVertices.at(toValue);
        FsmVertex* fromp = nullptr;
        만약 (isReset) {
            fromp = resetAnyVertex();
        } 아니면 만약 (isDefault) {
            fromp = defaultAnyVertex();
        } 아니면 {
            fromp = m_stateVertices.at(fromValue);
        }
        반환 new FsmArcEdge{this, fromp, top, isReset, isCond, isDefault, flp};
    }

    string name() const VL_MT_SAFE {
        반환 "FSM "
               + (m_stateVarName.empty() ? (m_stateVarScopep ? m_stateVarScopep->name() : "")
                                         : m_stateVarName);
    }
    string dumpTag(size_t index) const {
        string tag = stateVarInternalName();
        위해 (char& ch : tag) {
            만약 (!std::isalnum(static_cast<unsigned char>(ch))) ch = '_';
        }
        반환 "fsm_" + cvtToStr(index) + "_" + tag;
    }
};

struct DetectedFsm final {
    std::unique_ptr<FsmGraph> graphp;  // Extracted graph for one detected FSM candidate.
};

struct FsmCaseCandidate final {
    AstNode* warnNodep = nullptr;  // Transition node that made the candidate supported.
    AstVarScope* stateVscp = nullptr;  // FSM state variable associated with that candidate.
};

// Keep the source expression with the encoded value so inferred literal FSMs can
// reuse the same state-space policy as case-item dispatch.
struct FsmStateComparison final {
    AstVarScope* stateVscp = nullptr;  // Compared state variable
    AstNodeExpr* valuep = nullptr;  // Compared constant value expression
    FsmStateValue value;  // Encoded compared state value
};

// A branch is usable only after its predicate has exactly one state comparison;
// any extra predicate term is treated as an arc guard.
struct FsmIfBranch final {
    AstIf* ifp = nullptr;  // Source if/else-if node
    AstNode* stmtsp = nullptr;  // Branch body
    AstNodeExpr* valuep = nullptr;  // Source state value expression
    FsmStateValue fromValue;  // Encoded source state value
    bool hasTopGuard = false;  // Branch condition had extra guard terms
};

// If-chains are kept separate from cases until graph construction so the
// existing case path remains the preferred candidate when both forms appear.
struct FsmIfChainCandidate final {
    AstIf* ifp = nullptr;  // Top-level if-chain node
    AstVarScope* compareVscp = nullptr;  // Variable used by every state comparison
    std::vector<FsmIfBranch> branches;  // Recognized state-dispatch branches
    AstNode* defaultStmtsp = nullptr;  // Optional final else body
};

// Aliases are accepted only when they are equivalent to spelling the state
// comparison inline; this avoids inferring FSM semantics from arbitrary logic.
using FsmAliasMap = std::unordered_map<const AstVarScope*, FsmStateComparison>;
using FsmCellPortMap = std::unordered_map<string, AstVarScope*>;
using FsmCellPortAliasMap = std::unordered_map<const AstCell*, FsmCellPortMap>;

struct StateConstLabel final {
    string text;
    bool fromParam = false;
    size_t stateIndex = 0;
};

struct FsmStateSpace final {
    std::vector<std::pair<string, FsmStateValue>> states;  // User label and encoded value
    std::unordered_map<FsmStateValue, StateConstLabel, FsmStateValueHash>
        labels;  // Encoded value to label
    AstVar* stateVarp = nullptr;  // Tracked FSM state variable
    bool enumBacked = false;  // Whether states came from an enum declaration
};

// Local shared state between the two adjacent FSM coverage phases. Detection
// fills this with recovered FSM graphs; lowering consumes the completed graphs
// immediately afterward without needing any AST serialization bridge.
class FsmState final {
    // All detected FSMs in discovery order. This is the only bridge between
    // the adjacent detect and lower phases, so the second phase never needs to
    // rediscover or serialize the extracted machine.
    std::vector<DetectedFsm> m_fsms;
    std::map<const AstVarScope*, size_t> m_fsmIndex;

public:
    DetectedFsm& fsmFor(AstVarScope* stateVscp) {
        const std::map<const AstVarScope*, size_t>::const_iterator it = m_fsmIndex.find(stateVscp);
        만약 (it != m_fsmIndex.end()) 반환 m_fsms.at(it->second);
        const size_t index = m_fsms.size();
        m_fsmIndex.emplace(stateVscp, index);
        m_fsms.emplace_back();
        반환 m_fsms.back();
    }
    const std::vector<DetectedFsm>& fsms() const { 반환 m_fsms; }
};

// Detection runs while the original clocked/case structure is still intact and
// populates graph-backed FSM models without mutating the tree mid-traversal.
// This pass is intentionally conservative: for this PR we only lock down the
// small set of transition/selector forms that are already stable in the
// normalized AST we see here. The remaining reject branches are therefore
// mostly future-feature boundaries, not accidental dead code.
class FsmDetectVisitor final : public VNVisitor {
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
    std::unordered_map<const AstVarScope*, FsmCaseCandidate> m_comboPaired;
    // Continuous aliases are order-independent, while procedural aliases must
    // remain source-order scoped to avoid using assignments not yet executed.
    FsmAliasMap m_stateAliases;
    std::unordered_set<const AstVarScope*> m_ambiguousStateAliases;
    // A surviving wrapper's semantic d/q relationship is split across the
    // parent scope and the child module scope. This table is the narrow bridge
    // between those scopes: only transparent port aliases are recorded, so the
    // detector does not become a general cross-module dataflow engine.
    FsmCellPortAliasMap m_cellPortAliases;
    FsmCellPortAliasMap m_cellPortChildAliases;

    // METHODS
    // Enum-backed FSMs may be wrapped in refs/typedefs; normalize to the
    // underlying enum type before deciding whether a case is a candidate.
    static AstNodeDType* unwrapEnumCandidate(AstNodeDType* dtypep) {
        반환 dtypep->skipRefToEnump();
    }

    static string candidateConflictContext(AstNode* laterNodep,
                                           const FsmCaseCandidate& firstCand) {
        반환 '\n' + laterNodep->warnContextPrimary() + firstCand.warnNodep->warnOther()
               + "... Location of first supported candidate for "
               + firstCand.stateVscp->prettyNameQ() + '\n'
               + firstCand.warnNodep->warnContextSecondary();
    }

    static bool rejectFsmWrapperCell(AstCell* cellp, const string& reason) {
        cellp->v3warn(COVERIGN, "Ignoring unsupported: " + reason);
        반환 false;
    }

    static bool simpleParamStateValue(AstCell* cellp, const string& name, FsmStateValue& value,
                                      AstNodeExpr*& valuepr) {
        // Cell-path reset recovery must behave like the inlined path when the
        // instance relies on a parameter default. Looking into the linked module
        // default preserves that equivalence while keeping the cell detector's
        // contract narrow: only static, known reset encodings become reset arcs.
        valuepr = nullptr;
        위해 (AstNode* stmtp = cellp->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* const varp = VN_CAST(stmtp, Var);
            만약 (!varp || !varp->isParam() || varp->name() != name) continue;
            valuepr = VN_AS(varp->valuep(), NodeExpr);
            반환 constValueStatus(valuepr, value) == ConstValueStatus::OK;
        }
        반환 false;
    }

    static bool childPortInScope(AstVarScope* vscp, AstScope* parentScopep, AstCell*& cellpr) {
        만약 (!vscp->varp()->isIO()) 반환 false;
        AstScope* const scopep = vscp->scopep();
        UASSERT_OBJ(scopep, vscp, "VarScope without scope");
        만약 (scopep->aboveScopep() != parentScopep) 반환 false;
        UASSERT_OBJ(scopep->aboveCellp(), vscp,
                    "Child port scope should retain the instance that created it");
        cellpr = scopep->aboveCellp();
        반환 true;
    }

    static AstVarScope* simpleAssignVarScope(AstNodeExpr* exprp) {
        AstVarRef* const vrefp = VN_CAST(exprp, VarRef);
        반환 vrefp ? vrefp->varScopep() : nullptr;
    }

    void addWrapperCell(AstScope* scopep, AstCell* cellp) {
        m_cellPortAliases.emplace(cellp, FsmCellPortMap{});
        m_cellPortChildAliases.emplace(cellp, FsmCellPortMap{});
        const std::pair<AstScope*, AstCell*> item{scopep, cellp};
        만약 (std::find(m_wrapperCells.cbegin(), m_wrapperCells.cend(), item)
            != m_wrapperCells.cend()) {
            반환;
        }
        m_wrapperCells.emplace_back(item);
    }

    void collectCellPortAlias(AstAssignW* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "Cell port alias collection requires a scoped assignment");
        AstVarScope* const lhsVscp = simpleAssignVarScope(nodep->lhsp());
        AstVarScope* const rhsVscp = simpleAssignVarScope(nodep->rhsp());
        만약 (!lhsVscp || !rhsVscp) 반환;
        AstCell* cellp = nullptr;
        // The cell path is intentionally a transparent-wrapper recognizer. A
        // direct parent<->child variable assignment preserves the register's
        // identity across the hierarchy boundary; any expression, slice, or
        // transform is outside this phase's contract and therefore not recorded.
        만약 (childPortInScope(lhsVscp, m_scopep, cellp)) {
            만약 (!fsmRegisterWrapperDesc(cellp)) 반환;
            UASSERT_OBJ(lhsVscp->varp()->isInput(), nodep,
                        "Child-side port alias lhs should be an input");
            // Note the connected variable can live in a scope further up, as V3Inst
            // aliases a port connection, merging the port variable of an instance into
            // whatever it is connected to
            m_cellPortAliases[cellp][lhsVscp->varp()->name()] = rhsVscp;
            m_cellPortChildAliases[cellp][lhsVscp->varp()->name()] = lhsVscp;
            addWrapperCell(m_scopep, cellp);
        } 아니면 만약 (childPortInScope(rhsVscp, m_scopep, cellp)) {
            만약 (!fsmRegisterWrapperDesc(cellp)) 반환;
            UASSERT_OBJ(rhsVscp->varp()->isWritable(), nodep,
                        "Child-side port alias rhs should be writable");
            m_cellPortAliases[cellp][rhsVscp->varp()->name()] = lhsVscp;
            m_cellPortChildAliases[cellp][rhsVscp->varp()->name()] = rhsVscp;
            addWrapperCell(m_scopep, cellp);
        }
    }

    AstVarScope* roleVarScope(AstCell* cellp, const string& portName) const {
        // At this point explicit AstPin expressions have been lowered away, so
        // role resolution crosses the wrapper boundary only through the
        // transparent alias table above. This keeps wrapper support aligned with
        // direct-register detection instead of growing into interprocedural FSM
        // inference.
        const FsmCellPortMap& ports = m_cellPortAliases.at(cellp);
        const FsmCellPortMap::const_iterator portIt = ports.find(portName);
        반환 portIt == ports.end() ? nullptr : portIt->second;
    }

    AstVarScope* childRoleVarScope(AstCell* cellp, const string& portName) const {
        const FsmCellPortMap& ports = m_cellPortChildAliases.at(cellp);
        const FsmCellPortMap::const_iterator portIt = ports.find(portName);
        반환 portIt == ports.end() ? nullptr : portIt->second;
    }

    class RegisterAlwaysAnalyzer final {
        AstScope* const m_scopep;  // Scope containing the always block(s) being analyzed

    public:
        explicit RegisterAlwaysAnalyzer(AstScope* scopep)
            : m_scopep{scopep} {}

        std::vector<std::pair<AstCase*, AstNodeExpr*>>
        oneBlockCandidates(AstAlways* alwaysp) const {
            std::vector<std::pair<AstCase*, AstNodeExpr*>> candidates;
            AstNode* const stmtsp = alwaysp->stmtsp();
            만약 (AstIf* const firstIfp = VN_CAST(stmtsp, If)) {
                만약 (AstCase* const casep = VN_CAST(firstIfp->elsesp(), Case)) {
                    candidates.emplace_back(casep,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            위해 (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                만약 (AstCase* const casep = VN_CAST(nodep, Case))
                    candidates.emplace_back(casep, nullptr);
            }
            반환 candidates;
        }

        std::vector<std::pair<AstIf*, AstNodeExpr*>>
        oneBlockIfCandidates(AstAlways* alwaysp) const {
            std::vector<std::pair<AstIf*, AstNodeExpr*>> candidates;
            AstNode* const stmtsp = alwaysp->stmtsp();
            // Reset-else FSMs should behave like the existing case path: reset
            // information is metadata, not part of steady-state dispatch.
            만약 (AstIf* const firstIfp = VN_CAST(stmtsp, If)) {
                만약 (AstIf* const chainp
                    = VN_CAST(FsmDetectVisitor::singleMeaningfulBranch(firstIfp->elsesp()), If)) {
                    candidates.emplace_back(chainp,
                                            FsmDetectVisitor::isSimpleResetCond(firstIfp->condp())
                                                ? firstIfp->condp()
                                                : nullptr);
                }
            }
            위해 (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
                만약 (AstIf* const ifp = VN_CAST(nodep, If)) candidates.emplace_back(ifp, nullptr);
            }
            반환 candidates;
        }

        bool matchRegisterCandidate(AstAlways* alwaysp, FsmRegisterCandidate& cand) const {
            반환 FsmDetectVisitor::matchRegisterAlways(alwaysp, m_scopep, cand);
        }

        void buildOneBlockCandidate(AstAlways* alwaysp, AstVarScope* vscp, AstNodeExpr* resetCondp,
                                    FsmRegisterCandidate& reg) const {
            reg.scopep(m_scopep);
            reg.alwaysp(alwaysp);
            reg.stateVscp(vscp);
            reg.sampleVscp(vscp);
            reg.nextVscp(vscp);
            reg.senses() = FsmDetectVisitor::describeSenTree(alwaysp->sentreep());
            reg.resetCond() = FsmDetectVisitor::describeResetCond(resetCondp);
            reg.hasResetCond(reg.resetCond().varScopep != nullptr);
            reg.inclCond(vscp->varp()->attrFsmArcInclCond());
            AstIf* const firstIfp = VN_CAST(alwaysp->stmtsp(), If);
            만약 (firstIfp && reg.hasResetCond()) {
                AstVarScope* resetStateVscp = nullptr;
                const ResetAssignStatus resetStatus = FsmDetectVisitor::collectConstStateAssigns(
                    firstIfp->thensp(), resetStateVscp, reg.resetArcs());
                만약 (resetStatus == ResetAssignStatus::EMPTY) {
                    reg.resetArcs().clear();
                } 아니면 만약 (resetStatus == ResetAssignStatus::NONE || resetStateVscp != vscp) {
                    reg.resetArcs().clear();
                    FsmStateValue resetValue;
                    AstNode* const thenNodep
                        = FsmDetectVisitor::singleMeaningfulBranch(firstIfp->thensp());
                    UASSERT_OBJ(thenNodep, firstIfp,
                                "one-block reset fallback requires a non-empty reset branch");
                    AstNodeAssign* const resetAssp = FsmDetectVisitor::directConstStateAssignNode(
                        thenNodep, resetStateVscp, resetValue);
                    만약 (resetAssp && resetStateVscp == vscp) {
                        reg.resetArcs().emplace_back(resetValue, resetAssp);
                    }
                } 아니면 만약 (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                    reg.resetArcs().clear();
                }
            }
        }
    };

    bool matchFsmWrapperCell(AstScope* scopep, AstCell* cellp, FsmRegisterCandidate& cand) const {
        FsmWrapperRoles roles = rolesFromDesc(*fsmRegisterWrapperDesc(cellp));

        AstVarScope* const nextVscp = roleVarScope(cellp, roles.dPort);
        AstVarScope* const stateVscp = roleVarScope(cellp, roles.qPort);
        만약 (!nextVscp || !stateVscp) {
            반환 rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper d and q connections must be simple variables");
        }
        AstVarScope* const clkVscp = roleVarScope(cellp, roles.clkPort);
        만약 (!clkVscp) {
            반환 rejectFsmWrapperCell(
                cellp, "fsm_register_wrapper instance requires a simple clock connection");
        }

        FsmSenDesc clkSense;
        clkSense.edgeType = VEdgeType::ET_POSEDGE;
        clkSense.varScopep = clkVscp;
        cand.senses().push_back(clkSense);

        AstVarScope* resetVscp = nullptr;
        만약 (!roles.rstPort.empty()) resetVscp = roleVarScope(cellp, roles.rstPort);
        만약 (resetVscp) {
            // The descriptor identifies the reset port but not its polarity. Use
            // the wrapper's own event control AST as the contract for sampling
            // the connected parent signal.
            bool inferredActiveLow = false;
            만약 (fsmWrapperResetPolarityFromWrapperAst(cellp, roles.rstPort, inferredActiveLow)) {
                roles.hasRstActiveLow = true;
                roles.rstActiveLow = inferredActiveLow;
            }
        }

        AstNodeExpr* resetValuep = nullptr;
        FsmStateValue resetValue;
        const bool hasResetValue
            = !roles.rstValParam.empty()
              && simpleParamStateValue(cellp, roles.rstValParam, resetValue, resetValuep);
        만약 (resetVscp && roles.hasRstActiveLow && hasResetValue) {
            FsmSenDesc rstSense;
            rstSense.edgeType = roles.rstActiveLow ? VEdgeType::ET_NEGEDGE : VEdgeType::ET_POSEDGE;
            rstSense.varScopep = resetVscp;
            cand.senses().push_back(rstSense);
            cand.resetCond().varScopep = resetVscp;
            cand.resetCond().activeLow = roles.rstActiveLow;
            cand.hasResetCond(true);
            cand.resetArcs().emplace_back(resetValue, cellp, resetValuep);
        } 아니면 만약 (!roles.rstPort.empty() || !roles.rstValParam.empty()) {
            string reason;
            만약 (roles.rstPort.empty()) {
                reason = "reset port is not configured";
            } 아니면 만약 (!resetVscp) {
                reason = "reset connection is missing or not a simple variable";
            } 아니면 만약 (!roles.hasRstActiveLow) {
                reason = "reset polarity could not be inferred from the wrapper";
            } 아니면 만약 (roles.rstValParam.empty()) {
                reason = "reset_value parameter is not configured";
            } 아니면 {
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
        반환 true;
    }

    class ComboAlwaysAnalyzer final {
    public:
        struct ComboMatch final {
            const FsmRegisterCandidate* matchedp = nullptr;
            AstNode* warnNodep = nullptr;
        };

    private:
        const std::vector<FsmRegisterCandidate>& m_registerCandidates;

    public:
        explicit ComboAlwaysAnalyzer(const std::vector<FsmRegisterCandidate>& registerCandidates)
            : m_registerCandidates{registerCandidates} {}

        ComboMatch matchCase(AstNode* stmtsp, AstCase* casep) const {
            ComboMatch match;
            AstVarRef* const selp = VN_CAST(casep->exprp(), VarRef);
            만약 (!selp) 반환 match;
            위해 (const FsmRegisterCandidate& reg : m_registerCandidates) {
                만약 (selp->varScopep() == reg.nextVscp()) {
                    만약 (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        continue;
                    }
                } 아니면 만약 (selp->varScopep() != reg.stateVscp()) {
                    continue;
                }
                AstNode* const warnNodep = FsmDetectVisitor::caseSupportedTransitionNode(
                    casep, reg.nextVscp(), reg.inclCond());
                만약 (!warnNodep) continue;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            반환 match;
        }

        ComboMatch matchIfChain(AstNode* stmtsp, const FsmIfChainCandidate& chain) const {
            ComboMatch match;
            위해 (const FsmRegisterCandidate& reg : m_registerCandidates) {
                // Comparing state_d is safe only with the canonical default;
                // otherwise the chain may be dispatching on already-mutated data.
                만약 (chain.compareVscp == reg.nextVscp()) {
                    만약 (!FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, chain.ifp, reg.stateVscp(), reg.nextVscp())) {
                        continue;
                    }
                } 아니면 만약 (chain.compareVscp != reg.stateVscp()) {
                    continue;
                }
                AstNode* const warnNodep
                    = FsmDetectVisitor::ifChainSupportedTransitionNode(chain, reg.nextVscp());
                만약 (!warnNodep) continue;
                match.matchedp = &reg;
                match.warnNodep = warnNodep;
            }
            반환 match;
        }

        bool shouldWarnUnsupported(AstNode* stmtsp, AstCase* casep) const {
            const AstVarRef* const selp = VN_CAST(casep->exprp(), VarRef);
            만약 (!selp) 반환 false;

            반환 std::any_of(
                m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                [&](const FsmRegisterCandidate& reg) -> bool {
                    const bool matchesNext = selp->varScopep() == reg.nextVscp();
                    const bool matchesState = selp->varScopep() == reg.stateVscp();

                    만약 (!matchesNext && !matchesState) 반환 false;
                    만약 (matchesNext
                        && !FsmDetectVisitor::hasCanonicalNextStateDefaultBeforeCase(
                            stmtsp, casep, reg.stateVscp(), reg.nextVscp())) {
                        반환 false;
                    }
                    반환 FsmDetectVisitor::caseSupportedTransitionNode(casep, reg.nextVscp(),
                                                                         reg.inclCond());
                });
        }
    };

    // Reset arcs are only modeled for the simple signal form that survives to
    // this pass after earlier normalization.
    static bool isSimpleResetCond(AstNodeExpr* condp) { 반환 VN_IS(condp, VarRef); }

    // Normalize the reset condition into a compact description so the lowering
    // phase can regenerate the same predicate after detection. By the time
    // this pass runs, active-low source forms such as "!rst_n" have already
    // been canonicalized to a positive-condition if/else shape, so only a
    // plain VarRef survives here.
    static FsmResetCondDesc describeResetCond(AstNodeExpr* condp) {
        FsmResetCondDesc desc;
        만약 (AstVarRef* const vrefp = VN_CAST(condp, VarRef)) {
            desc.varScopep = vrefp->varScopep();
        }
        반환 desc;
    }

    // Snapshot the original event control so the lowering phase can rebuild an
    // active block with the same edge semantics.
    static std::vector<FsmSenDesc> describeSenTree(AstSenTree* sentreep) {
        std::vector<FsmSenDesc> senses;
        위해 (AstSenItem* itemp = sentreep->sensesp(); itemp;
             itemp = VN_AS(itemp->nextp(), SenItem)) {
            AstNodeVarRef* const vrefp = itemp->varrefp();
            만약 (!vrefp) continue;
            FsmSenDesc desc;
            desc.edgeType = itemp->edgeType().m_e;
            desc.varScopep = vrefp->varScopep();
            senses.push_back(desc);
        }
        반환 senses;
    }

    // Ignore existing coverage increments so FSM detection sees the user logic
    // rather than other instrumentation already attached to the block.
    static bool isIgnorableStmt(AstNode* nodep) { 반환 VN_IS(nodep, CoverInc); }

    static AstNode* skipLeadingIgnorableStmt(AstNode* nodep) {
        동안 (nodep && isIgnorableStmt(nodep)) nodep = nodep->nextp();
        반환 nodep;
    }

    // Conservative extractor for statement lists: only treat a list as simple
    // when exactly one non-coverage statement remains after unwrapping.
    // Richer multi-statement or control-flow forms are intentionally left for
    // follow-on FSM-detection work instead of being partially inferred here.
    static AstNode* singleMeaningfulStmt(AstNode* stmtp) {
        AstNode* resultp = nullptr;
        위해 (AstNode* nodep = stmtp; nodep; nodep = nodep->nextp()) {
            만약 (isIgnorableStmt(nodep)) continue;
            만약 (resultp) 반환 nullptr;
            resultp = nodep;
        }
        반환 resultp;
    }

    // If/else branches are a single subtree, not a statement list, so do not
    // walk nextp() here or we may accidentally consume the sibling else-arm.
    static AstNode* singleMeaningfulBranch(AstNode* branchp) {
        만약 (!branchp) 반환 nullptr;
        반환 branchp;
    }

    // By fsm-detect time, non-clocked always @* blocks are already admitted through
    // a missing sentree. This helper therefore only needs to recognize
    // explicit changed-sensitivity lists such as always @(a or b); clocked and
    // event-driven forms remain out of scope.
    static bool isPlainComboSentree(const AstSenTree* sentreep) {
        UASSERT(sentreep, "plain combo sensitivity check requires a sensitivity tree");
        위해 (const AstSenItem* senp = sentreep->sensesp(); senp;
             senp = VN_AS(senp->nextp(), SenItem)) {
            만약 (senp->edgeType() == VEdgeType::ET_CHANGED) continue;
            반환 false;
        }
        반환 true;
    }

    void warnUnsupportedComboAlways(const FsmComboAlways& combo) {
        const ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        AstNode* const stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        bool warned = false;
        위해 (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* const casep = VN_CAST(nodep, Case);
            만약 (!casep) continue;
            만약 (analyzer.shouldWarnUnsupported(stmtsp, casep)) {
                casep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-clocked always "
                                        "blocks requires a combinational sensitivity list or "
                                        "always_comb");
                warned = true;
            }
            만약 (warned) break;
        }
    }

    // Case-item bodies are single subtrees like if/else arms, not statement
    // lists, so unwrap only local begin/end wrappers here rather than walking
    // sibling case items via nextp().
    static AstNodeAssign* directStateAssign(AstNode* stmtp, AstVarScope* stateVscp) {
        AstNode* const nodep = singleMeaningfulBranch(stmtp);
        만약 (!nodep) 반환 nullptr;
        AstNodeAssign* const assp = VN_CAST(nodep, NodeAssign);
        만약 (!assp) 반환 nullptr;
        AstVarRef* const vrefp = VN_CAST(assp->lhsp(), VarRef);
        만약 (!vrefp || vrefp->varScopep() != stateVscp) 반환 nullptr;
        반환 assp;
    }

    static AstNodeAssign* nodeStateVarAssign(AstNode* nodep, AstVarScope*& stateVscp,
                                             AstVarScope*& fromVscp) {
        AstNodeAssign* const assp = VN_CAST(nodep, NodeAssign);
        만약 (!assp) 반환 nullptr;
        AstVarRef* const lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        AstVarRef* const rhsp = VN_CAST(assp->rhsp(), VarRef);
        만약 (!rhsp || !lhsp) 반환 nullptr;
        stateVscp = lhsp->varScopep();
        fromVscp = rhsp->varScopep();
        반환 assp;
    }

    // Match 'state = reset ? CONST : next', an active high reset folded into the commit
    static AstNodeAssign* directCondStateVarAssign(AstNode* nodep, AstVarScope*& stateVscp,
                                                   AstVarScope*& fromVscp, AstNodeExpr*& condp,
                                                   FsmStateValue& resetValue) {
        AstNodeAssign* const assp = VN_CAST(nodep, NodeAssign);
        만약 (!assp) 반환 nullptr;
        AstVarRef* const lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        AstCond* const rhsp = VN_CAST(assp->rhsp(), Cond);
        만약 (!rhsp || !lhsp) 반환 nullptr;
        AstVarRef* const elsep = VN_CAST(rhsp->elsep(), VarRef);
        만약 (!elsep) 반환 nullptr;
        만약 (constValueStatus(rhsp->thenp(), resetValue) != ConstValueStatus::OK) 반환 nullptr;
        fromVscp = elsep->varScopep();
        stateVscp = lhsp->varScopep();
        condp = rhsp->condp();
        반환 assp;
    }

    static AstNodeAssign* directConstStateAssignNode(AstNode* nodep, AstVarScope*& stateVscp,
                                                     FsmStateValue& value) {
        AstNodeAssign* const assp = VN_CAST(nodep, NodeAssign);
        만약 (!assp) 반환 nullptr;
        AstVarRef* const lhsp = VN_CAST(assp->lhsp()->baseFromp(true), VarRef);
        UASSERT_OBJ(lhsp, assp,
                    "direct constant state assignment lhs should be normalized to a VarRef");
        만약 (constValueStatus(assp->rhsp(), value) != ConstValueStatus::OK) 반환 nullptr;
        stateVscp = lhsp->varScopep();
        반환 assp;
    }

    enum class ResetAssignStatus : uint8_t {
        EMPTY,  // Reset branch had no non-coverage statements.
        NONE,  // Reset branch was not the supported direct-constant shape.
        SINGLE,  // Exactly one supported reset assignment was collected.
        MULTI_SAME_STATE  // Multiple assignments to the same FSM state var; warn and ignore.
    };

    // Reset arcs are only extracted from the single direct-constant form. If
    // user RTL assigns the same state register multiple times in the reset
    // branch, warn and skip reset-arc modeling rather than inventing multiple
    // reset transitions for an odd but legal coding style.
    static ResetAssignStatus collectConstStateAssigns(AstNode* stmtp, AstVarScope*& stateVscp,
                                                      std::vector<FsmResetArcDesc>& resetArcs) {
        AstNode* nodep = skipLeadingIgnorableStmt(stmtp);
        만약 (!nodep) 반환 ResetAssignStatus::EMPTY;
        위해 (;; nodep = nodep->nextp()) {
            AstVarScope* assignStateVscp = nullptr;
            FsmStateValue value;
            AstNodeAssign* const assp = directConstStateAssignNode(nodep, assignStateVscp, value);
            만약 (!assp) 반환 ResetAssignStatus::NONE;
            만약 (!stateVscp) stateVscp = assignStateVscp;
            만약 (assignStateVscp != stateVscp) 반환 ResetAssignStatus::NONE;
            만약 (!resetArcs.empty()) {
                assp->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on reset branches with "
                                       "multiple assignments to the state variable");
                resetArcs.clear();
                반환 ResetAssignStatus::MULTI_SAME_STATE;
            }
            resetArcs.emplace_back(value, assp);
            만약 (!nodep->nextp()) 반환 ResetAssignStatus::SINGLE;
        }
    }

    static bool hasCanonicalNextStateDefaultBeforeCase(AstNode* stmtsp, AstNode* targetp,
                                                       AstVarScope* stateVscp,
                                                       AstVarScope* nextVscp) {
        AstNode* const bodyp = skipLeadingIgnorableStmt(stmtsp);
        bool sawCanonicalDefault = false;
        위해 (AstNode* nodep = bodyp;; nodep = nodep->nextp()) {
            UASSERT_OBJ(nodep, targetp,
                        "next-state candidate not found in scanned statement list");
            만약 (nodep == targetp) 반환 sawCanonicalDefault;
            만약 (AstNodeAssign* const assp = VN_CAST(nodep, NodeAssign)) {
                AstVarRef* const lhsp = VN_CAST(assp->lhsp(), VarRef);
                AstVarRef* const rhsp = VN_CAST(assp->rhsp(), VarRef);
                만약 (!lhsp || lhsp->varScopep() != nextVscp) continue;
                만약 (sawCanonicalDefault) {
                    const string nextName = nextVscp->varp()->prettyNameQ();
                    const string stateName = stateVscp->varp()->prettyNameQ();
                    assp->v3warn(COVERIGN,
                                 "Ignoring unsupported: FSM coverage on case(" + nextName
                                     + ") when the canonical " + nextName + " = " + stateName
                                     + " default is overwritten before the case statement");
                    반환 false;
                }
                만약 (!rhsp || rhsp->varScopep() != stateVscp) 반환 false;
                sawCanonicalDefault = true;
            }
        }
    }

    static bool ifStateConstAssign(AstNode* stmtp, AstVarScope* stateVscp,
                                   FsmStateValue& thenValue, FsmStateValue& elseValue) {
        AstIf* const ifp = VN_CAST(singleMeaningfulBranch(stmtp), If);
        만약 (!ifp || !ifp->elsesp()) 반환 false;
        AstVarScope* thenVscp = nullptr;
        AstVarScope* elseVscp = nullptr;
        AstNode* const thenNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->thensp()));
        AstNode* const elseNodep = singleMeaningfulBranch(skipLeadingIgnorableStmt(ifp->elsesp()));
        만약 (!thenNodep || !elseNodep) 반환 false;
        만약 (!directConstStateAssignNode(thenNodep, thenVscp, thenValue)) 반환 false;
        만약 (!directConstStateAssignNode(elseNodep, elseVscp, elseValue)) 반환 false;
        만약 (thenVscp == stateVscp && elseVscp == stateVscp) 반환 true;
        만약 (thenVscp != elseVscp) 반환 false;
        AstNode* const followp = skipLeadingIgnorableStmt(ifp->nextp());
        AstVarScope* finalStateVscp = nullptr;
        AstVarScope* finalFromVscp = nullptr;
        AstNode* const finalNodep = singleMeaningfulBranch(followp);
        만약 (!finalNodep) 반환 false;
        만약 (!nodeStateVarAssign(finalNodep, finalStateVscp, finalFromVscp)) 반환 false;
        만약 (finalStateVscp != stateVscp) 반환 false;
        만약 (finalFromVscp != thenVscp) 반환 false;
        반환 true;
    }

    static bool directStateCondConstAssign(AstNode* stmtp, AstVarScope* stateVscp,
                                           FsmStateValue& thenValue, FsmStateValue& elseValue) {
        AstNodeAssign* const assp = directStateAssign(stmtp, stateVscp);
        만약 (!assp) 반환 false;
        AstCond* const condp = VN_CAST(assp->rhsp(), Cond);
        만약 (!condp) 반환 false;
        반환 constValueStatus(condp->thenp(), thenValue) == ConstValueStatus::OK
               && constValueStatus(condp->elsep(), elseValue) == ConstValueStatus::OK;
    }

    static AstNode* caseItemSupportedArcNode(AstCaseItem* itemp, AstVarScope* stateVscp,
                                             bool inclCond) {
        만약 (itemp->isDefault()) {
            만약 (!inclCond) 반환 nullptr;
        }
        AstNodeAssign* const assp = directStateAssign(itemp->stmtsp(), stateVscp);
        만약 (assp) {
            FsmStateValue toValue;
            만약 (constValueStatus(assp->rhsp(), toValue) == ConstValueStatus::OK) 반환 assp;
        }
        FsmStateValue thenValue;
        FsmStateValue elseValue;
        만약 (directStateCondConstAssign(itemp->stmtsp(), stateVscp, thenValue, elseValue)) {
            반환 assp;
        }
        만약 (ifStateConstAssign(itemp->stmtsp(), stateVscp, thenValue, elseValue)) {
            반환 singleMeaningfulBranch(itemp->stmtsp());
        }
        반환 nullptr;
    }

    // Combinational transition blocks are paired only through supported case
    // items that assign to the recorded next-state variable.
    static AstNode* caseSupportedTransitionNode(AstCase* casep, AstVarScope* stateVscp,
                                                bool inclCond) {
        위해 (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            만약 (AstNode* const nodep = caseItemSupportedArcNode(itemp, stateVscp, inclCond))
                반환 nodep;
        }
        반환 nullptr;
    }

    static AstNode* caseItemSupportedArcNodeLike(AstNode* stmtsp, AstVarScope* stateVscp) {
        만약 (AstNodeAssign* const assp = directStateAssign(stmtsp, stateVscp)) {
            FsmStateValue toValue;
            만약 (constValueStatus(assp->rhsp(), toValue) == ConstValueStatus::OK) 반환 assp;
            FsmStateValue thenValue;
            FsmStateValue elseValue;
            만약 (directStateCondConstAssign(stmtsp, stateVscp, thenValue, elseValue)) 반환 assp;
        }
        반환 nullptr;
    }

    static AstNode* ifChainSupportedTransitionNode(const FsmIfChainCandidate& chain,
                                                   AstVarScope* stateVscp) {
        위해 (size_t i = 0; i < chain.branches.size(); ++i) {
            const FsmIfBranch& branch = chain.branches[i];
            AstNode* const nodep = caseItemSupportedArcNodeLike(branch.stmtsp, stateVscp);
            만약 (!nodep) 반환 nullptr;
        }
        반환 chain.branches.front().ifp;
    }

    // Prefer user labels in reports. Forced non-enum FSMs prepopulate synthetic
    // labels, so all emitted arcs should already have a known label here.
    static string labelForValue(
        const std::unordered_map<FsmStateValue, StateConstLabel, FsmStateValueHash>& labels,
        const FsmStateValue& value) {
        반환 labels.at(value).text;
    }

    // The extractor only models constant-valued state transitions, and by the
    // time detect runs those values have already been constant-folded.
    enum class ConstValueStatus : uint8_t { OK, NOT_CONST, XZ };

    static ConstValueStatus constValueStatus(AstNodeExpr* exprp, FsmStateValue& value) {
        const AstConst* const constp = VN_CAST(exprp, Const);
        만약 (!constp) 반환 ConstValueStatus::NOT_CONST;
        const V3Number& num = constp->num();
        만약 (num.isAnyXZ()) 반환 ConstValueStatus::XZ;
        value = FsmStateValue{num};
        반환 ConstValueStatus::OK;
    }

    static bool pureStateComparisonNoAlias(AstNodeExpr* exprp, FsmStateComparison& cmp) {
        AstEq* const eqp = VN_CAST(exprp, Eq);
        만약 (!eqp) 반환 false;

        // Operand order is not semantically meaningful for state dispatch, so
        // both normalized forms should classify identically.
        AstVarRef* vrefp = VN_CAST(eqp->lhsp(), VarRef);
        AstNodeExpr* valuep = eqp->rhsp();
        만약 (!vrefp) {
            vrefp = VN_CAST(eqp->rhsp()->baseFromp(true), VarRef);
            만약 (!vrefp) { 반환 false; }
            valuep = eqp->lhsp();
        }

        FsmStateValue value;
        만약 (constValueStatus(valuep, value) != ConstValueStatus::OK) 반환 false;
        cmp.stateVscp = vrefp->varScopep();
        cmp.valuep = valuep;
        cmp.value = value;
        반환 true;
    }

    static bool pureStateComparison(AstNodeExpr* exprp, const FsmAliasMap& aliases,
                                    FsmStateComparison& cmp) {
        만약 (pureStateComparisonNoAlias(exprp, cmp)) 반환 true;
        // Bare predicates are too broad for FSM inference unless a prior alias
        // proves they are exactly a state comparison.
        만약 (AstVarRef* const vrefp = VN_CAST(exprp, VarRef)) {
            const FsmAliasMap::const_iterator it = aliases.find(vrefp->varScopep());
            만약 (it == aliases.end()) 반환 false;
            cmp = it->second;
            반환 true;
        }
        반환 false;
    }

    static bool supportedTopLevelGuard(AstNodeExpr* exprp) {
        // These terms can combine multiple dispatch choices into one branch, so
        // treating them as ordinary guards would over-infer the FSM shape.
        만약 (VN_IS(exprp, Or)) 반환 false;
        만약 (VN_IS(exprp, RedAnd)) 반환 false;
        만약 (VN_IS(exprp, RedOr)) 반환 false;
        만약 (VN_IS(exprp, RedXor)) 반환 false;
        반환 true;
    }

    static bool resolveIfPredicate(AstNodeExpr* exprp, const FsmAliasMap& aliases,
                                   FsmStateComparison& cmp, bool& hasGuard) {
        std::vector<AstNodeExpr*> terms;
        std::vector<AstNodeExpr*> pending;
        pending.push_back(exprp);
        // Top-level conjunction is the only decomposition we can map cleanly to
        // one source state plus optional transition guards.
        동안 (!pending.empty()) {
            AstNodeExpr* const nodep = pending.back();
            pending.pop_back();
            만약 (AstAnd* const andp = VN_CAST(nodep, And)) {
                pending.push_back(andp->rhsp());
                pending.push_back(andp->lhsp());
            } 아니면 {
                terms.push_back(nodep);
            }
        }

        bool sawComparison = false;
        위해 (size_t i = 0; i < terms.size(); ++i) {
            AstNodeExpr* const termp = terms[i];
            FsmStateComparison termCmp;
            만약 (pureStateComparison(termp, aliases, termCmp /*ref*/)) {
                만약 (sawComparison) 반환 false;
                cmp = termCmp;
                sawComparison = true;
                continue;
            }
            만약 (!supportedTopLevelGuard(termp)) 반환 false;
            hasGuard = true;
        }
        반환 sawComparison;
    }

    static void addAlias(FsmAliasMap& aliases, std::unordered_set<const AstVarScope*>& ambiguous,
                         AstVarScope* aliasVscp, const FsmStateComparison& cmp) {
        만약 (ambiguous.find(aliasVscp) != ambiguous.end()) 반환;
        const FsmAliasMap::iterator it = aliases.find(aliasVscp);
        만약 (it == aliases.end()) {
            aliases.emplace(aliasVscp, cmp);
            반환;
        }
        // Conflicting alias definitions make the predicate ambiguous, and
        // ambiguous aliases are worse than missing an optional FSM.
        만약 (it->second.stateVscp == cmp.stateVscp && it->second.value == cmp.value) 반환;
        aliases.erase(aliasVscp);
        ambiguous.emplace(aliasVscp);
        반환;
    }

    static void collectAliasFromAssign(AstNodeAssign* assp, FsmAliasMap& aliases,
                                       std::unordered_set<const AstVarScope*>& ambiguous) {
        AstVarRef* const lhsp = VN_CAST(assp->lhsp(), VarRef);
        만약 (!lhsp) 반환;
        FsmStateComparison cmp;
        // Guarded aliases blur dispatch and transition conditions, so require a
        // pure comparison and let guards live at the use site.
        만약 (!pureStateComparisonNoAlias(assp->rhsp(), cmp /*ref*/)) 반환;
        addAlias(aliases /*ref*/, ambiguous /*ref*/, lhsp->varScopep(), cmp);
    }

    FsmAliasMap localAliasesBefore(AstNode* stmtsp, AstNode* limitp) const {
        FsmAliasMap aliases = m_stateAliases;
        std::unordered_set<const AstVarScope*> ambiguous = m_ambiguousStateAliases;
        // Procedural aliases cannot be applied before their assignment without
        // changing the meaning of the surrounding always block.
        위해 (AstNode* nodep = skipLeadingIgnorableStmt(stmtsp); nodep && nodep != limitp;
             nodep = nodep->nextp()) {
            만약 (AstNodeAssign* const assp = VN_CAST(nodep, NodeAssign)) {
                collectAliasFromAssign(assp, aliases /*ref*/, ambiguous /*ref*/);
            }
        }
        위해 (const AstVarScope* const vscp : ambiguous) aliases.erase(vscp);
        반환 aliases;
    }

    static bool collectIfChain(AstIf* ifp, const FsmAliasMap& aliases,
                               FsmIfChainCandidate& chain) {
        chain.ifp = ifp;
        std::unordered_set<string> seenValues;
        AstIf* curp = ifp;
        // Only the top-level spine represents dispatch; treating nested branch
        // logic as additional source states would invent transitions.
        동안 (true) {
            FsmStateComparison cmp;
            bool hasGuard = false;
            만약 (!resolveIfPredicate(curp->condp(), aliases, cmp, hasGuard)) 반환 false;
            만약 (chain.compareVscp && chain.compareVscp != cmp.stateVscp) 반환 false;
            만약 (!seenValues.insert(cmp.value.key()).second) 반환 false;
            chain.compareVscp = cmp.stateVscp;
            chain.branches.push_back(
                FsmIfBranch{curp, curp->thensp(), cmp.valuep, cmp.value, hasGuard});

            AstNode* const elseNodep
                = singleMeaningfulBranch(skipLeadingIgnorableStmt(curp->elsesp()));
            만약 (!elseNodep) break;
            만약 (AstIf* const elseIfp = VN_CAST(elseNodep, If)) {
                curp = elseIfp;
                continue;
            }
            chain.defaultStmtsp = elseNodep;
            break;
        }
        반환 chain.branches.size() >= 2;
    }

    // Enum-backed FSMs should only use values that were interned as known states.
    // If a constant transition references some other encoding, warn and skip FSM
    // instrumentation for that edge rather than silently dropping it or turning
    // optional coverage into a hard compile failure.
    static bool validateKnownStateValue(AstNode* nodep, const FsmStateSpace& stateSpace,
                                        const FsmStateValue& value, const string& role) {
        만약 (stateSpace.labels.find(value) != stateSpace.labels.end()) 반환 true;
        만약 (stateSpace.enumBacked) {
            const string enumRole = role == "source" ? "case item value" : "assigned value";
            nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on enum state variable "
                                        + stateSpace.stateVarp->prettyNameQ() + ": " + enumRole
                                        + " " + value.warnText()
                                        + " is not present in the declared enum");
            반환 false;
        }
        nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum state variable "
                                    + stateSpace.stateVarp->prettyNameQ() + ": " + role + " value "
                                    + value.warnText()
                                    + " is not present in the inferred state space");
        반환 false;
    }

    static StateConstLabel stateLabelForConst(AstConst* constp) {
        const string name = constp->origParamName();
        만약 (!name.empty()) 반환 StateConstLabel{AstNode::prettyName(name), true, 0};
        반환 StateConstLabel{constp->name(), false, 0};
    }

    static void updateStateLabel(FsmStateSpace& stateSpace, const FsmStateValue& value,
                                 const StateConstLabel& label) {
        stateSpace.states.at(stateSpace.labels.at(value).stateIndex).first = label.text;
    }

    // Strict Phase 1 matcher for register processes: either a bare state
    // commit, or a top-level reset guard whose else path is that commit.
    static bool matchRegisterAlways(AstAlways* alwaysp, AstScope* scopep,
                                    FsmRegisterCandidate& cand) {
        만약 (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) 반환 false;

        AstNode* const stmtsp = skipLeadingIgnorableStmt(alwaysp->stmtsp());
        AstNode* const nodep = singleMeaningfulStmt(stmtsp);
        만약 (!nodep) 반환 false;

        AstVarScope* stateVscp = nullptr;
        AstVarScope* nextVscp = nullptr;
        만약 (AstIf* const ifp = VN_CAST(nodep, If)) {
            만약 (!ifp->elsesp() || !isSimpleResetCond(ifp->condp())) 반환 false;
            AstVarScope* resetStateVscp = nullptr;
            const ResetAssignStatus resetStatus
                = collectConstStateAssigns(ifp->thensp(), resetStateVscp, cand.resetArcs());
            const bool emptyResetBranch = resetStatus == ResetAssignStatus::EMPTY;
            만약 (emptyResetBranch) {
                cand.resetArcs().clear();
            } 아니면 만약 (resetStatus == ResetAssignStatus::NONE) {
                cand.resetArcs().clear();
                FsmStateValue resetValue;
                AstNode* const thenNodep = singleMeaningfulBranch(ifp->thensp());
                UASSERT_OBJ(thenNodep, ifp, "reset fallback requires a non-empty reset branch");
                AstNodeAssign* const resetAssp
                    = directConstStateAssignNode(thenNodep, resetStateVscp, resetValue);
                만약 (!resetAssp) 반환 false;
                cand.resetArcs().emplace_back(resetValue, resetAssp);
            } 아니면 만약 (resetStatus == ResetAssignStatus::MULTI_SAME_STATE) {
                cand.resetArcs().clear();
            }
            AstNode* const elseNodep = singleMeaningfulBranch(ifp->elsesp());
            UASSERT_OBJ(elseNodep, ifp, "register reset match requires a non-empty commit branch");
            만약 (!nodeStateVarAssign(elseNodep, stateVscp, nextVscp)) 반환 false;
            만약 (!emptyResetBranch && resetStateVscp != stateVscp) 반환 false;
            cand.resetCond() = describeResetCond(ifp->condp());
            cand.hasResetCond(cand.resetCond().varScopep != nullptr);
        } 아니면 {
            AstNodeExpr* resetCondp = nullptr;
            FsmStateValue resetValue;
            // Note the active low 'reset ? next : CONST' form is not accepted
            만약 (AstNodeAssign* const assp
                = directCondStateVarAssign(nodep, stateVscp, nextVscp, resetCondp, resetValue)) {
                cand.resetArcs().emplace_back(resetValue, assp);
                cand.resetCond() = describeResetCond(resetCondp);
                cand.hasResetCond(cand.resetCond().varScopep != nullptr);
            } 아니면 만약 (!nodeStateVarAssign(nodep, stateVscp, nextVscp)) {
                반환 false;
            }
        }
        cand.scopep(scopep);
        cand.alwaysp(alwaysp);
        cand.stateVscp(stateVscp);
        cand.sampleVscp(stateVscp);
        cand.nextVscp(nextVscp);
        cand.senses() = describeSenTree(alwaysp->sentreep());
        cand.inclCond(stateVscp->varp()->attrFsmArcInclCond());
        cand.fileline(alwaysp->fileline());
        반환 true;
    }

    static bool addValueToStateSpace(AstNode* nodep, FsmStateSpace& stateSpace,
                                     const FsmStateValue& value, StateConstLabel label) {
        const auto labelIt = stateSpace.labels.find(value);
        만약 (labelIt != stateSpace.labels.end()) {
            StateConstLabel& existingLabel = labelIt->second;
            만약 (existingLabel.text != label.text && existingLabel.fromParam && label.fromParam) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                        "state variable "
                                            + stateSpace.stateVarp->prettyNameQ()
                                            + " with multiple labels for the same value "
                                            + value.warnText() + ": " + existingLabel.text
                                            + " and " + label.text);
                반환 false;
            }
            만약 (!existingLabel.fromParam && label.fromParam) {
                existingLabel.text = label.text;
                existingLabel.fromParam = label.fromParam;
                updateStateLabel(stateSpace, value, label);
            }
        } 아니면 {
            StateConstLabel storedLabel = label;
            storedLabel.stateIndex = stateSpace.states.size();
            stateSpace.states.emplace_back(label.text, value);
            stateSpace.labels.emplace(value, storedLabel);
        }
        반환 true;
    }

    // Helper: process a single observed state expression and add it to the state space
    // Returns true on success, false if the state space is invalid
    static bool addExprToStateSpace(AstNodeExpr* valuep, FsmStateSpace& stateSpace) {
        FsmStateValue value;
        const ConstValueStatus status = constValueStatus(valuep, value);
        만약 (status != ConstValueStatus::OK) {
            만약 (status == ConstValueStatus::XZ) {
                valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                         "state variable "
                                             + stateSpace.stateVarp->prettyNameQ()
                                             + " with X/Z state encoding values");
            }
            반환 false;
        }
        AstConst* const constp = VN_AS(valuep, Const);
        반환 addValueToStateSpace(valuep, stateSpace, value, stateLabelForConst(constp));
    }

    static bool addOptionalTargetExprToStateSpace(AstNodeExpr* valuep, FsmStateSpace& stateSpace) {
        FsmStateValue value;
        const ConstValueStatus status = constValueStatus(valuep, value);
        만약 (status != ConstValueStatus::OK) {
            valuep->v3warn(COVERIGN, "Ignoring unsupported: FSM coverage on non-enum "
                                     "state variable "
                                         + stateSpace.stateVarp->prettyNameQ()
                                         + " with non-constant target state values");
            반환 false;
        }
        AstConst* const constp = VN_AS(valuep, Const);
        반환 addValueToStateSpace(valuep, stateSpace, value, stateLabelForConst(constp));
    }

    static void addResetTargetsToStateSpace(const std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace) {
        위해 (const FsmResetArcDesc& resetArc : resetArcs) {
            StateConstLabel label{resetArc.toValue().ascii(), false, 0};
            만약 (AstConst* const constp = VN_CAST(resetArc.valuep(), Const)) {
                label = stateLabelForConst(constp);
            }
            UASSERT_OBJ(
                addValueToStateSpace(resetArc.nodep(), stateSpace, resetArc.toValue(), label),
                resetArc.nodep(), "reset target labels should be unambiguous");
        }
    }

    // Build the Phase 1 state space from the tracked registered state
    // variable, not from whichever signal the transition statement happened to use.
    static bool collectDeclaredStateSpace(AstNode* warnNodep, AstVarScope* stateVscp,
                                          FsmStateSpace& stateSpace, bool& needsSourceValues) {
        AstVar* const stateVarp = stateVscp->varp();
        AstEnumDType* enump = VN_CAST(unwrapEnumCandidate(stateVscp->dtypep()), EnumDType);
        만약 (!enump) enump = VN_CAST(unwrapEnumCandidate(stateVarp->dtypep()), EnumDType);
        const bool forced = stateVarp->attrFsmState();
        stateSpace.stateVarp = stateVarp;

        만약 (enump) {
            stateSpace.enumBacked = true;
            위해 (AstEnumItem* itemp = enump->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                const AstConst* const constp = VN_AS(itemp->valuep(), Const);
                const FsmStateValue value{constp->num()};
                const size_t stateIndex = stateSpace.states.size();
                stateSpace.states.emplace_back(itemp->name(), value);
                stateSpace.labels.emplace(value,
                                          StateConstLabel{itemp->name(), false, stateIndex});
            }
            반환 stateSpace.states.size() >= 2;
        }

        만약 (forced) {
            needsSourceValues = true;
            반환 true;
        }

        needsSourceValues = true;
        반환 true;
    }

    template <typename T_ValuepVisitor>
    static bool collectStateSpaceFromValues(AstNode* warnNodep, AstVarScope* stateVscp,
                                            const std::vector<FsmResetArcDesc>& resetArcs,
                                            FsmStateSpace& stateSpace,
                                            const T_ValuepVisitor& visitValueps) {
        bool needsSourceValues = false;
        // Cases and if-chains should share the same state-space policy; only
        // the source of inferred literal values differs between the forms.
        만약 (!collectDeclaredStateSpace(warnNodep, stateVscp, stateSpace, needsSourceValues)) {
            반환 false;
        }
        만약 (!needsSourceValues) 반환 true;
        addResetTargetsToStateSpace(resetArcs, stateSpace);
        만약 (!visitValueps(
                [&](AstNodeExpr* valuep) { 반환 addExprToStateSpace(valuep, stateSpace); })) {
            반환 false;
        }
        반환 stateSpace.states.size() >= 2;
    }

    static bool collectStateSpace(AstCase* casep, AstVarScope* stateVscp, AstVarScope* assignVscp,
                                  const std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        반환 collectStateSpaceFromValues(
            casep, stateVscp, resetArcs, stateSpace,
            [casep, assignVscp, &stateSpace](const auto& visitValuep) {
                위해 (AstCaseItem* itemp = casep->itemsp(); itemp;
                     itemp = VN_AS(itemp->nextp(), CaseItem)) {
                    만약 (!itemp->isDefault()) {
                        위해 (AstNodeExpr* condp = itemp->condsp(); condp;
                             condp = VN_AS(condp->nextp(), NodeExpr)) {
                            만약 (!visitValuep(condp)) 반환 false;
                        }
                    }
                    만약 (AstNodeAssign* const assp
                        = directStateAssign(itemp->stmtsp(), assignVscp)) {
                        FsmStateValue thenValue;
                        FsmStateValue elseValue;
                        AstCond* const condp = VN_CAST(assp->rhsp(), Cond);
                        만약 (condp
                            && directStateCondConstAssign(itemp->stmtsp(), assignVscp, thenValue,
                                                          elseValue)) {
                            만약 (!visitValuep(condp->thenp())) 반환 false;
                            만약 (!visitValuep(condp->elsep())) 반환 false;
                        } 아니면 만약 (!addOptionalTargetExprToStateSpace(assp->rhsp(), stateSpace)) {
                            반환 false;
                        }
                    }
                }
                반환 true;
            });
    }

    static bool collectStateSpace(const FsmIfChainCandidate& chain, AstVarScope* stateVscp,
                                  AstVarScope* assignVscp,
                                  const std::vector<FsmResetArcDesc>& resetArcs,
                                  FsmStateSpace& stateSpace) {
        반환 collectStateSpaceFromValues(
            chain.ifp, stateVscp, resetArcs, stateSpace,
            [&chain, assignVscp, &stateSpace](const auto& visitValuep) {
                위해 (const FsmIfBranch& branch : chain.branches) {
                    // Reaching this point with an unresolvable source value
                    // would mean the if-chain classifier and emitter disagree.
                    UASSERT_OBJ(visitValuep(branch.valuep), branch.valuep,
                                "FSM if-chain source values should be prevalidated");
                    AstNodeAssign* const assp = directStateAssign(branch.stmtsp, assignVscp);
                    UASSERT_OBJ(assp, branch.stmtsp,
                                "FSM if-chain target values should be prevalidated");
                    FsmStateValue thenValue;
                    FsmStateValue elseValue;
                    AstCond* const condp = VN_CAST(assp->rhsp(), Cond);
                    만약 (condp) {
                        UASSERT_OBJ(directStateCondConstAssign(branch.stmtsp, assignVscp,
                                                               thenValue, elseValue),
                                    condp, "FSM if-chain ternary targets should be prevalidated");
                        만약 (!visitValuep(condp->thenp())) 반환 false;
                        만약 (!visitValuep(condp->elsep())) 반환 false;
                    } 아니면 만약 (!addOptionalTargetExprToStateSpace(assp->rhsp(), stateSpace)) {
                        반환 false;
                    }
                }
                반환 true;
            });
    }

    // Extract supported case-item transitions in one place so the conservative
    // policy for direct and ternary forms stays consistent. The false exits in
    // this helper are deliberate subset boundaries: they document shapes we do
    // not yet model in this PR and that future FSM-detection work may widen.
    static bool emitCaseItemArcs(FsmGraph& graph, AstCaseItem* itemp, AstVarScope* stateVscp,
                                 const FsmStateSpace& stateSpace, bool inclCond) {
        std::vector<std::pair<string, FsmStateValue>> froms;
        만약 (itemp->isDefault()) {
            만약 (!inclCond) 반환 false;
            froms.emplace_back("default", FsmStateValue{});
        } 아니면 {
            위해 (AstNodeExpr* condp = itemp->condsp(); condp;
                 condp = VN_AS(condp->nextp(), NodeExpr)) {
                FsmStateValue value;
                만약 (constValueStatus(condp, value) != ConstValueStatus::OK) continue;
                만약 (!validateKnownStateValue(condp, stateSpace, value, "source")) 반환 true;
                froms.emplace_back(labelForValue(stateSpace.labels, value), value);
            }
            만약 (froms.empty()) 반환 false;
        }

        만약 (AstNodeAssign* const assp = directStateAssign(itemp->stmtsp(), stateVscp)) {
            FsmStateValue toValue;
            const ConstValueStatus status = constValueStatus(assp->rhsp(), toValue);
            만약 (status == ConstValueStatus::OK) {
                만약 (!validateKnownStateValue(assp, stateSpace, toValue, "target")) 반환 true;
                위해 (const std::pair<string, FsmStateValue>& from : froms) {
                    graph.addArc(from.second, toValue, false, false, itemp->isDefault(),
                                 assp->fileline());
                }
                반환 true;
            }
        }

        FsmStateValue thenValue;
        FsmStateValue elseValue;
        만약 (directStateCondConstAssign(itemp->stmtsp(), stateVscp, thenValue, elseValue)
            || ifStateConstAssign(itemp->stmtsp(), stateVscp, thenValue, elseValue)) {
            만약 (!validateKnownStateValue(itemp->stmtsp(), stateSpace, thenValue, "target"))
                반환 true;
            만약 (!validateKnownStateValue(itemp->stmtsp(), stateSpace, elseValue, "target"))
                반환 true;
            위해 (const FsmStateValue& branchValue : {thenValue, elseValue}) {
                위해 (const std::pair<string, FsmStateValue>& from : froms) {
                    graph.addArc(from.second, branchValue, false, true, itemp->isDefault(),
                                 itemp->stmtsp()->fileline());
                }
            }
            반환 true;
        }

        반환 false;
    }

    static void emitStmtArcsFrom(FsmGraph& graph, AstNode* stmtsp, AstVarScope* stateVscp,
                                 const FsmStateSpace& stateSpace, FsmStateValue fromValue,
                                 bool isDefault, bool forceCond) {
        AstNodeAssign* const assp = directStateAssign(stmtsp, stateVscp);
        UASSERT_OBJ(assp, stmtsp, "FSM if-chain branch should have been prevalidated");
        FsmStateValue toValue;
        const ConstValueStatus status = constValueStatus(assp->rhsp(), toValue);
        만약 (status == ConstValueStatus::OK) {
            만약 (!validateKnownStateValue(assp, stateSpace, toValue, "target")) 반환;
            // Preserve the user's guard in coverage by marking this arc
            // conditional even when the branch body is a direct assignment.
            graph.addArc(fromValue, toValue, false, forceCond, isDefault, assp->fileline());
            반환;
        }

        FsmStateValue thenValue;
        FsmStateValue elseValue;
        const bool condAssign
            = directStateCondConstAssign(stmtsp, stateVscp, thenValue, elseValue);
        UASSERT_OBJ(condAssign, stmtsp,
                    "FSM if-chain branch should be a direct constant transition");
        만약 (!validateKnownStateValue(stmtsp, stateSpace, thenValue, "target")) 반환;
        만약 (!validateKnownStateValue(stmtsp, stateSpace, elseValue, "target")) 반환;
        위해 (const FsmStateValue& branchValue : {thenValue, elseValue}) {
            graph.addArc(fromValue, branchValue, false, true, isDefault, stmtsp->fileline());
        }
    }

    static void emitIfChainArcs(FsmGraph& graph, const FsmIfChainCandidate& chain,
                                AstVarScope* stateVscp, const FsmStateSpace& stateSpace) {
        위해 (size_t i = 0; i < chain.branches.size(); ++i) {
            const FsmIfBranch& branch = chain.branches[i];
            // Invalid source labels mean the extracted graph would no longer
            // match the resolved state space, so abandon the candidate.
            만약 (!validateKnownStateValue(branch.ifp, stateSpace, branch.fromValue, "source"))
                반환;
            emitStmtArcsFrom(graph, branch.stmtsp, stateVscp, stateSpace, branch.fromValue, false,
                             branch.hasTopGuard);
        }
    }

    // Reset transitions are described separately because they live in the reset
    // branch outside the steady-state case statement.
    static void addResetArcs(FsmGraph& graph, const std::vector<FsmResetArcDesc>& resetArcs,
                             const FsmStateSpace& stateSpace) {
        위해 (const FsmResetArcDesc& resetArc : resetArcs) {
            만약 (!validateKnownStateValue(resetArc.nodep(), stateSpace, resetArc.toValue(),
                                         "target"))
                continue;
            graph.addArc(FsmStateValue{}, resetArc.toValue(), true, false, false,
                         resetArc.nodep()->fileline());
        }
    }

    // Turn one candidate case statement into the graph representation that the
    // later lowering phase will consume directly, while reviewers can still
    // inspect the extracted machine via DOT dumps.
    void processCase(AstCase* casep, AstVarScope* assignVscp, const FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, casep, "FSM case processing requires a non-null assignment var");
        AstVarScope* const stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        만약 (!collectStateSpace(casep, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) 반환;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        만약 (!entry.graphp) {
            entry.graphp.reset(new FsmGraph{});
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
            위해 (const std::pair<string, FsmStateValue>& state : stateSpace.states) {
                entry.graphp->addStateVertex(state.first, state.second);
            }
            addResetArcs(*entry.graphp, reg.resetArcs(), stateSpace);
        }
        위해 (AstCaseItem* itemp = casep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {
            emitCaseItemArcs(*entry.graphp, itemp, assignVscp, stateSpace,
                             entry.graphp->inclCond());
        }
    }

    void processIfChain(const FsmIfChainCandidate& chain, AstVarScope* assignVscp,
                        const FsmRegisterCandidate& reg) {
        UASSERT_OBJ(assignVscp, chain.ifp,
                    "FSM if-chain processing requires a non-null assignment var");
        AstVarScope* const stateVscp = reg.stateVscp();
        FsmStateSpace stateSpace;
        만약 (!collectStateSpace(chain, stateVscp, assignVscp, reg.resetArcs(), stateSpace)) 반환;
        DetectedFsm& entry = m_state.fsmFor(stateVscp);
        // Case candidates keep ownership of existing graphs; reaching this path
        // means the if-chain is the only supported dispatch for this FSM.
        UASSERT_OBJ(!entry.graphp, chain.ifp, "FSM if-chain graph should not already exist");
        entry.graphp.reset(new FsmGraph{});
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
        위해 (const std::pair<string, FsmStateValue>& state : stateSpace.states) {
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
    void processOneBlockAlways(const FsmComboAlways& oneBlock) {
        const RegisterAlwaysAnalyzer analyzer{oneBlock.scopep()};
        AstAlways* const alwaysp = oneBlock.alwaysp();
        만약 (!alwaysp->sentreep() || !alwaysp->sentreep()->hasEdge()) 반환;
        const std::vector<std::pair<AstCase*, AstNodeExpr*>> candidates
            = analyzer.oneBlockCandidates(alwaysp);

        FsmCaseCandidate firstCand;
        위해 (const std::pair<AstCase*, AstNodeExpr*>& cand : candidates) {
            AstVarRef* const selp = VN_CAST(cand.first->exprp(), VarRef);
            AstVarScope* const vscp = selp ? selp->varScopep() : nullptr;
            만약 (!vscp) continue;
            만약 (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processCase(cand.first, vscp, reg);
            } 아니면 만약 (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed case statements found in "
                                   "the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } 아니면 {
                cand.first->v3warn(COVERIGN,
                                   "Ignoring unsupported: FSM coverage on multiple supported case "
                                   "statements found in the same always block. Only the first "
                                   "candidate will be instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            }
        }

        만약 (firstCand.stateVscp) 반환;

        // Case dispatch is more explicit and pre-existing behavior depends on
        // it winning when both shapes are present.
        const std::vector<std::pair<AstIf*, AstNodeExpr*>> ifCandidates
            = analyzer.oneBlockIfCandidates(alwaysp);
        위해 (const std::pair<AstIf*, AstNodeExpr*>& cand : ifCandidates) {
            const FsmAliasMap aliases = localAliasesBefore(alwaysp->stmtsp(), cand.first);
            FsmIfChainCandidate chain;
            만약 (!collectIfChain(cand.first, aliases, chain)) continue;
            AstVarScope* const vscp = chain.compareVscp;
            만약 (!ifChainSupportedTransitionNode(chain, vscp)) continue;
            만약 (!firstCand.stateVscp) {
                firstCand.warnNodep = cand.first;
                firstCand.stateVscp = vscp;
                FsmRegisterCandidate reg;
                analyzer.buildOneBlockCandidate(alwaysp, vscp, cand.second, reg);
                processIfChain(chain, vscp, reg);
            } 아니면 만약 (vscp != firstCand.stateVscp) {
                cand.first->v3warn(FSMMULTI,
                                   "FSM coverage: multiple enum-typed transition candidates found "
                                   "in the same always block. Only the first candidate will be "
                                   "instrumented."
                                       << candidateConflictContext(cand.first, firstCand));
            } 아니면 {
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
    static void warnComboSameAlways(AstNode* warnNodep, const FsmCaseCandidate& firstCand) {
        warnNodep->v3warn(FSMMULTI,
                          "FSM coverage: multiple supported transition candidates found in "
                          "the same combinational always block. Only the first candidate "
                          "will be instrumented."
                              << candidateConflictContext(warnNodep, firstCand));
    }

    void processComboAlways(const FsmComboAlways& combo) {
        const ComboAlwaysAnalyzer analyzer{m_registerCandidates};
        AstNode* const stmtsp = skipLeadingIgnorableStmt(combo.alwaysp()->stmtsp());
        FsmCaseCandidate firstCand;
        위해 (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstCase* const casep = VN_CAST(nodep, Case);
            만약 (!casep) continue;
            const ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchCase(stmtsp, casep);
            const FsmRegisterCandidate* const matchedp = match.matchedp;
            AstNode* const matchedWarnNodep = match.warnNodep;
            만약 (!matchedp) continue;
            만약 (!firstCand.stateVscp) {
                const auto insertPair = m_comboPaired.emplace(
                    matchedp->stateVscp(),
                    FsmCaseCandidate{matchedWarnNodep,
                                     const_cast<AstVarScope*>(matchedp->stateVscp())});
                만약 (!insertPair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  insertPair.first->second));
                    continue;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processCase(casep, matchedp->nextVscp(), *matchedp);
                continue;
            }
            만약 (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                continue;
            }
            matchedWarnNodep->v3warn(COVERIGN,
                                     "Ignoring unsupported: FSM coverage on multiple "
                                     "supported case statements found in the same "
                                     "combinational always block. Only the first "
                                     "candidate will be instrumented."
                                         << candidateConflictContext(matchedWarnNodep, firstCand));
        }
        만약 (firstCand.stateVscp) 반환;

        // Keep the same priority in paired combinational logic: if-chain
        // support must not change which existing case FSM is instrumented.
        위해 (AstNode* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            AstIf* const ifp = VN_CAST(nodep, If);
            만약 (!ifp) continue;
            FsmIfChainCandidate chain;
            const FsmAliasMap aliases = localAliasesBefore(stmtsp, nodep);
            만약 (!collectIfChain(ifp, aliases, chain)) continue;
            const ComboAlwaysAnalyzer::ComboMatch match = analyzer.matchIfChain(stmtsp, chain);
            const FsmRegisterCandidate* const matchedp = match.matchedp;
            AstNode* const matchedWarnNodep = match.warnNodep;
            만약 (!matchedp) continue;
            만약 (!firstCand.stateVscp) {
                const std::pair<std::unordered_map<const AstVarScope*, FsmCaseCandidate>::iterator,
                                bool>
                    insertPair = m_comboPaired.emplace(
                        matchedp->stateVscp(),
                        FsmCaseCandidate{matchedWarnNodep,
                                         const_cast<AstVarScope*>(matchedp->stateVscp())});
                만약 (!insertPair.second) {
                    matchedWarnNodep->v3warn(
                        FSMMULTI, "FSM coverage: multiple supported transition candidates found "
                                  "for the same FSM in combinational always blocks. Only the "
                                  "first candidate will be instrumented."
                                      << candidateConflictContext(matchedWarnNodep,
                                                                  insertPair.first->second));
                    continue;
                }
                firstCand.warnNodep = matchedWarnNodep;
                firstCand.stateVscp = const_cast<AstVarScope*>(matchedp->stateVscp());
                processIfChain(chain, matchedp->nextVscp(), *matchedp);
                continue;
            }
            만약 (matchedp->stateVscp() != firstCand.stateVscp) {
                warnComboSameAlways(matchedWarnNodep, firstCand);
                continue;
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
    void visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    // Collect processes first, then analyze FSM candidates once all alias and
    // register information is available.
    void visit(AstAlways* nodep) override {
        만약 (nodep->keyword() == VAlwaysKwd::CONT_ASSIGN) {
            iterateChildren(nodep);
            반환;
        }
        // This avoids making one-block if-chain detection sensitive to whether
        // a continuous alias appears before or after the always block.
        m_oneBlockAlwayss.emplace_back(m_scopep, nodep);
        const RegisterAlwaysAnalyzer analyzer{m_scopep};
        FsmRegisterCandidate reg;
        만약 (analyzer.matchRegisterCandidate(nodep, reg)) {
            AstVarScope* const stateVscp = reg.stateVscp();
            const bool found
                = std::any_of(m_registerCandidates.cbegin(), m_registerCandidates.cend(),
                              [stateVscp](const FsmRegisterCandidate& existing) {
                                  반환 existing.stateVscp() == stateVscp;
                              });
            만약 (!found) { m_registerCandidates.emplace_back(reg); }
        }
        만약 (nodep->keyword() == VAlwaysKwd::ALWAYS_COMB) {
            m_comboAlwayss.emplace_back(m_scopep, nodep);
        } 아니면 만약 (nodep->keyword() == VAlwaysKwd::ALWAYS) {
            만약 (!nodep->sentreep() || isPlainComboSentree(nodep->sentreep())) {
                m_comboAlwayss.emplace_back(m_scopep, nodep);
            } 아니면 {
                m_nonComboAlwayss.emplace_back(m_scopep, nodep);
            }
        }
    }

    void visit(AstAssignW* nodep) override {
        // Continuous aliases are unordered hardware connections, so source
        // order should not affect whether an if-chain FSM is recognized.
        collectAliasFromAssign(nodep, m_stateAliases, m_ambiguousStateAliases);
        collectCellPortAlias(nodep);
        iterateChildren(nodep);
    }

    void visit(AstCell* nodep) override {
        // Cells are matched after the full traversal because linkdot lowers
        // uninlined port connections into sibling continuous assignments.
        만약 (m_scopep && fsmRegisterWrapperDesc(nodep)) addWrapperCell(m_scopep, nodep);
        iterateChildren(nodep);
    }

    // Continue the walk through the rest of the design hierarchy.
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    // Collect all FSM graphs into the shared local state before the lowering
    // phase starts mutating the AST with coverage machinery.
    FsmDetectVisitor(FsmState& state, AstNetlist* rootp)
        : m_state{state} {
        iterate(rootp);
        위해 (const std::pair<AstScope*, AstCell*>& wrapperCell : m_wrapperCells) {
            FsmRegisterCandidate reg;
            만약 (matchFsmWrapperCell(wrapperCell.first, wrapperCell.second, reg)) {
                m_registerCandidates.emplace_back(reg);
            }
        }
        위해 (const FsmComboAlways& oneBlock : m_oneBlockAlwayss) processOneBlockAlways(oneBlock);
        위해 (const FsmComboAlways& combo : m_comboAlwayss) processComboAlways(combo);
        위해 (const FsmComboAlways& combo : m_nonComboAlwayss) warnUnsupportedComboAlways(combo);
    }
};

// Builder of statements that will handle incrementation of proper AstNodeCoverDecls - which will
// be also created here
class FsmCoverIncBuilder final {
    const FsmGraph& m_graph;  // Fsm graph
    std::vector<const FsmStateVertex*> m_states;  // States vertexes inside fsm graph
    std::unordered_set<uint64_t>
        m_undefinedStates;  // States that have no enum value but are possible to represent with a
                            // variable of given width.
                            // This variable keeps uint64_t instead of V3Numebr because it is safe
                            // to assume that nobody has over 16M TB of RAM
    const FsmVertex* m_defaultVertexp = nullptr;  // Default vertex
    const FsmVertex* m_resetVertexp = nullptr;  // Reset vertex
    size_t m_edgesCount = 0;  // Total count of edges
    AstVarScope* const m_prevVscp;  // Previous state of fsm
    FileLine* const m_flp;  // Fileline pointer to use across the builder
    const std::string m_modPrettyName;  // Pretty fsm mode name
    AstNodeStmt* m_coverIncp = nullptr;  // Increments proper CoverDecl after each change
    AstNodeStmt* m_initialCoverIncp = nullptr;  // Increments CoverDecl at the beggining of the sim
    AstCoverOtherDecl* m_declsp = nullptr;  // List of CoverDecls
    bool m_isComplete;  // Whether all possible values of state variable are covered by some state
    // (e.g.: when state variable has two bits this will be true if exactly
    // four states exist)

    AstConst* makeConst(const uint64_t value) {
        V3Number num{static_cast<AstNode*>(nullptr), m_prevVscp->width()};
        num.setQuad(value);
        num.isSigned(m_prevVscp->dtypep()->isSigned());
        반환 new AstConst{m_flp, num};
    }

    std::string quadToBitStirngLabel(const uint64_t val) {
        정수 width = m_prevVscp->width();
        UASSERT(width < 64, "We should not expand values that wide");
        std::string result = std::to_string(width) + "'b";
        // Binary not hex because in future we want to represent
        // four-state values accuratelly as well
        동안 (--width >= 0) result.push_back("01"[(val >> width) & 1]);
        반환 result;
    }

    void initExistingStates() {
        위해 (const V3GraphVertex& vtx : m_graph.vertices()) {
            const FsmVertex* const vertexp = vtx.as<FsmVertex>();
            m_edgesCount += vertexp->inEdges().size();
            만약 (const FsmStateVertex* const statep = vertexp->cast<FsmStateVertex>()) {
                m_states.push_back(statep);
            } 아니면 만약 (vertexp->isDefaultAny()) {
                UASSERT_OBJ(!std::exchange(m_defaultVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two defaults?");
            } 아니면 /* if (vertexp->isResetAny()) */ {
                UASSERT_OBJ(!std::exchange(m_resetVertexp, vertexp), m_graph.sampleVarScopep(),
                            "Two resets?");
            }
        }
        const 정수 width = m_graph.sampleVarScopep()->width();
        m_isComplete = width < 64 && m_states.size() == (static_cast<size_t>(1) << width);
    }

    void initUndefinedStates() {
        UASSERT_OBJ(m_graph.sampleVarScopep()->width() < 64, m_graph.sampleVarScopep(),
                    "This FSM is too wide to expand - this should be caught earlier");
        const uint64_t combinationsCount = static_cast<size_t>(1)
                                           << m_graph.sampleVarScopep()->width();
        // Rolling trick allows to cut peak memory usage
        bool undefRolled = false;
        auto rollUndefs = [this, combinationsCount] {
            위해 (uint64_t i = 0; i < combinationsCount; ++i) {
                const auto insert = m_undefinedStates.insert(i);
                만약 (!insert.second) m_undefinedStates.erase(insert.first);
            }
        };
        위해 (const FsmStateVertex* const statep : m_states) {
            const V3Number& num = statep->value().num();
            const uint64_t value = num.toUQuad();
            만약 (!m_isComplete) {
                만약 (undefRolled) {
                    UASSERT_OBJ(m_undefinedStates.erase(value) == 1, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                } 아니면 {
                    UASSERT_OBJ(m_undefinedStates.insert(value).second, m_graph.sampleVarScopep(),
                                "Two states has the same value?");
                    만약 (m_undefinedStates.size() * 2 >= combinationsCount) {
                        undefRolled = true;
                        rollUndefs();
                    }
                }
            }
        }
        만약 (!(m_isComplete || undefRolled)) rollUndefs();
    }

    AstNodeExpr* buildResetCond() const {
        AstNodeExpr* const refp
            = new AstVarRef{m_flp, m_graph.resetCond().varScopep, VAccess::READ};
        반환 m_graph.resetCond().activeLow
                   ? static_cast<AstNodeExpr*>(new AstLogNot{m_flp, refp})
                   : refp;
    }

    size_t getArcCountWithExpansion(const VFsmExpandType expand) const {
        size_t arcCountWithExpansion = ~0ULL;
        switch (expand) {
        case VFsmExpandType::DEFAULT: {  // LCOV_EXCL_LINE
            m_graph.sampleVarScopep()->v3fatalSrc(
                "At this point default should be substituted with a value it defaults to");
            break;
        }
        case VFsmExpandType::AUTO: {
            arcCountWithExpansion = m_edgesCount;
            만약 (m_defaultVertexp) {
                UASSERT_OBJ(m_states.size() >= m_defaultVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Default has more out edges than possible states?");
                arcCountWithExpansion -= m_defaultVertexp->outEdges().size();
                위해 (const FsmStateVertex* const statep : m_states) {
                    arcCountWithExpansion += static_cast<size_t>(statep->outEdges().empty())
                                             * m_defaultVertexp->outEdges().size();
                }
            }
            만약 (m_resetVertexp) {
                UASSERT_OBJ(m_states.size() >= m_resetVertexp->outEdges().size(),
                            m_graph.sampleVarScopep(),
                            "Reset has more out edges than possible states?");
                arcCountWithExpansion += m_states.size() - m_resetVertexp->outEdges().size();
            }
            break;
        }
        case VFsmExpandType::AUTO_EXPAND: {
            arcCountWithExpansion = m_states.size() * m_states.size() - m_states.size();
            break;
        }
        case VFsmExpandType::FULL: {
            const 정수 width = m_graph.sampleVarScopep()->width();
            // 32 is derived from:
            // tranitionsCount <= std::numeric_limits<uint64_t>::max()
            // (2^width)^2 - (2^width) <= 2^64 - 1
            // (2^width)^2 - (2^width) < 2^64 since we operate on integral values we can trade 1
            // log_2((2^width)^2 - (2^width)) < 64
            // log_2((2^width)^2) < 64 - `(2^width)` is negigible so, it may be dropped
            // 2width < 64
            만약 (width < 32) {
                const uint64_t combinationsCount = static_cast<size_t>(1) << width;
                arcCountWithExpansion = combinationsCount * combinationsCount - combinationsCount;
            }
            break;
        }
        }
        반환 arcCountWithExpansion;
    }

    AstCoverOtherDecl* buildStateCoverDecl(const std::string& label,
                                           const std::string& fsmTag = "") {
        AstCoverOtherDecl* const declp
            = new AstCoverOtherDecl{m_flp,
                                    "v_fsm_state/" + m_modPrettyName,
                                    m_graph.stateVarName() + "::" + label,
                                    "",
                                    0,
                                    m_graph.stateVarName(),
                                    "",
                                    label,
                                    fsmTag};
        // We run after scoping, so this is declared for one instance only
        declp->perInstance(true);
        m_declsp = AstNode::addNext(m_declsp, declp);
        반환 declp;
    }

    AstNodeStmt* buildCoverIncStmtp(const std::string& fromStateLabel,
                                    const std::string& toStateLabel,
                                    const std::string& baseLabel = "") {
        // Whole builder is designed in such a way that this function won't be called twice with
        // the same arguments - but if it will ever change in future, `AstCoverOtherDecl` shall be
        // cached for particular args set
        AstCoverOtherDecl* const declp = new AstCoverOtherDecl{
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
        declp->perInstance(true);
        m_declsp = AstNode::addNext(m_declsp, declp);
        반환 new AstCoverInc{m_flp, declp};
    }
    AstNodeStmt* buildExpandedTransitionHandler(const std::string& toStateLabel,
                                                const bool considerUndefs,
                                                const bool expandOnlyWithTerminalVerticies,
                                                const FsmStateVertex* const toStatep = nullptr) {
        AstCase* const arcCasep = new AstCase{
            m_flp, VCaseType::CT_CASE, new AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        std::unordered_set<const FsmStateVertex*> handledSoureces;
        만약 (toStatep) {
            위해 (const V3GraphEdge& edge : toStatep->inEdges()) {
                만약 (const FsmStateVertex* const fromp = edge.fromp()->cast<FsmStateVertex>()) {
                    만약 (!handledSoureces.emplace(fromp).second) continue;
                    arcCasep->addItemsp(new AstCaseItem{
                        m_flp, makeConst(fromp->value().num().toUQuad()),
                        (fromp != toStatep) ? buildCoverIncStmtp(fromp->label(), toStateLabel)
                                            : nullptr});
                }
            }
        }
        위해 (const FsmStateVertex* const fromp : m_states) {
            만약 (handledSoureces.find(fromp) != handledSoureces.end()) continue;
            arcCasep->addItemsp(new AstCaseItem{
                m_flp, makeConst(fromp->value().num().toUQuad()),
                (fromp != toStatep
                 && (!expandOnlyWithTerminalVerticies || fromp->outEdges().empty()))
                    ? buildCoverIncStmtp(fromp->label(), toStateLabel, "artificial")
                    : nullptr});
        }
        만약 (!m_isComplete) {
            만약 (considerUndefs) {
                위해 (const uint64_t from : m_undefinedStates) {
                    const std::string& label = quadToBitStirngLabel(from);
                    arcCasep->addItemsp(
                        new AstCaseItem{m_flp, makeConst(from),
                                        (label != toStateLabel)
                                            ? buildCoverIncStmtp(label, toStateLabel, "artificial")
                                            : nullptr});
                }
            } 아니면 {
                arcCasep->addItemsp(new AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        반환 arcCasep;
    }
    AstNodeStmt* buildUnxpandedTransitionHandler(const FsmStateVertex* const toStatep) {
        AstCase* const arcCasep = new AstCase{
            m_flp, VCaseType::CT_CASE, new AstVarRef{m_flp, m_prevVscp, VAccess::READ}, nullptr};
        위해 (const V3GraphEdge& edge : toStatep->inEdges()) {
            const FsmArcEdge* const arcp = edge.as<FsmArcEdge>();
            UASSERT_OBJ(!arcp->isReset(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a reset arc");
            UASSERT_OBJ(!arcp->isDefault(), m_graph.stateVarScopep(),
                        "This function should never be called on a target of a default arc");
            const FsmStateVertex* const fromp = arcp->fromp()->as<FsmStateVertex>();
            만약 (fromp != toStatep) {
                arcCasep->addItemsp(
                    new AstCaseItem{m_flp, makeConst(fromp->value().num().toUQuad()),
                                    buildCoverIncStmtp(fromp->label(), toStatep->label())});
            }
        }
        arcCasep->addItemsp(new AstCaseItem{m_flp, nullptr, nullptr});
        반환 arcCasep;
    }

public:
    FsmCoverIncBuilder(const FsmGraph& graph, AstVarScope* const prevVscp)
        : m_graph{graph}
        , m_prevVscp{prevVscp}
        , m_flp{graph.fileline()}
        , m_modPrettyName{graph.scopep()->modp()->prettyName()} {
        initExistingStates();
        AstVarScope* const sampleVscp = graph.sampleVarScopep();
        const VFsmExpandType expand
            = graph.stateVarScopep()->varp()->attrFsmStateExpand() != VFsmExpandType::DEFAULT
                  ? graph.stateVarScopep()->varp()->attrFsmStateExpand()
                  : v3Global.opt.coverageFsmExpand();

        {
            const size_t arcCountWithExpansion = getArcCountWithExpansion(expand);
            만약 (VL_UNLIKELY(v3Global.opt.coverageFsmMaxArcs() < 0 || ~arcCountWithExpansion == 0ULL
                            || arcCountWithExpansion
                                   > static_cast<size_t>(v3Global.opt.coverageFsmMaxArcs()))) {
                sampleVscp->v3error("Exceeded size of --coverage-fsm-max-arcs "
                                    << v3Global.opt.coverageFsmMaxArcs()
                                    << " with: " << arcCountWithExpansion);
                반환;
            }
        }

        AstCase* const arcCasep
            = new AstCase{m_flp, VCaseType::CT_CASE,
                          new AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};
        AstCase* const initialStateCasep
            = new AstCase{m_flp, VCaseType::CT_CASE,
                          new AstVarRef{m_flp, graph.sampleVarScopep(), VAccess::READ}, nullptr};

        const bool expandAll = expand != VFsmExpandType::AUTO;
        std::unordered_set<const FsmStateVertex*>
            defaultTargets;  // States that have edge incoming from default vertex
        std::unordered_set<const FsmStateVertex*>
            resetTargets;  // States that have edge incoming from reset vertex
        만약 (!expandAll) {
            만약 (m_defaultVertexp) {
                위해 (const V3GraphEdge& edge : m_defaultVertexp->outEdges()) {
                    defaultTargets.emplace(edge.top()->as<FsmStateVertex>());
                }
            }
            만약 (m_resetVertexp) {
                위해 (const V3GraphEdge& edge : m_resetVertexp->outEdges()) {
                    resetTargets.emplace(edge.top()->as<FsmStateVertex>());
                }
            }
        }
        const bool isFullExpansion = expand == VFsmExpandType::FULL;
        만약 (isFullExpansion) initUndefinedStates();
        위해 (const FsmStateVertex* const statep : m_states) {
            // State coverage fires when the FSM enters a state from any other
            // value, so repeated self-holds do not count as new entries.
            AstCoverOtherDecl* const declp = buildStateCoverDecl(statep->label());
            initialStateCasep->addItemsp(
                new AstCaseItem{m_flp, makeConst(statep->value().num().toUQuad()),
                                new AstCoverInc{m_flp, declp, true}});
            const bool expandAllOrReset
                = expandAll || (resetTargets.find(statep) != resetTargets.end());
            const bool defaultExpantion
                = !expandAllOrReset && (defaultTargets.find(statep) != defaultTargets.end());
            arcCasep->addItemsp(new AstCaseItem{
                m_flp, makeConst(statep->value().num().toUQuad()),
                AstNode::addNext<AstNodeStmt>(
                    new AstCoverInc{m_flp, declp},
                    expandAllOrReset || defaultExpantion
                        ? buildExpandedTransitionHandler(statep->label(), isFullExpansion,
                                                         defaultExpantion, statep)
                        : buildUnxpandedTransitionHandler(statep))});
        }
        만약 (!m_isComplete) {
            // Case is incomplete default CaseItem is needed
            만약 (isFullExpansion) {
                위해 (const uint64_t value : m_undefinedStates) {
                    const std::string& label = quadToBitStirngLabel(value);
                    AstCoverOtherDecl* const declp = buildStateCoverDecl(label);
                    initialStateCasep->addItemsp(new AstCaseItem{
                        m_flp, makeConst(value), new AstCoverInc{m_flp, declp, true}});
                    arcCasep->addItemsp(
                        new AstCaseItem{m_flp, makeConst(value),
                                        AstNode::addNext<AstNodeStmt>(
                                            new AstCoverInc{m_flp, declp},
                                            buildExpandedTransitionHandler(label, true, false))});
                }
            } 아니면 {
                // No expansion
                arcCasep->addItemsp(new AstCaseItem{m_flp, nullptr, nullptr});
                initialStateCasep->addItemsp(new AstCaseItem{m_flp, nullptr, nullptr});
            }
        }
        m_coverIncp = arcCasep;
        m_initialCoverIncp = initialStateCasep;
    }
    VL_UNCOPYABLE(FsmCoverIncBuilder);
    ~FsmCoverIncBuilder() = default;
    AstNodeCoverDecl* getCoverDeclsp() const { 반환 m_declsp; }
    AstNodeStmt* getCoverIncp() const { 반환 m_coverIncp; }
    AstNodeStmt* getInitialCoverIncp() const { 반환 m_initialCoverIncp; }
};

// Lower the completed FSM graphs into the concrete coverage declarations,
// previous-state tracking, and pre/post-triggered instrumentation that the
// runtime uses to record state and transition coverage.
class FsmLowerVisitor final {
    // STATE - across all visitors
    const FsmState& m_state;
    V3SharedTmps m_prevTmps{"__VfsmCovPrev", VVarType::MODULETEMP};  // Previous states

    // METHODS
    // Rebuild the original event control from the saved sense description so
    // post-state coverage sampling runs on the same triggering edges.
    static AstSenTree* buildSenTree(FileLine* flp, const std::vector<FsmSenDesc>& senses) {
        AstSenTree* const sentreep = new AstSenTree{flp, nullptr};
        위해 (const FsmSenDesc& sense : senses) {
            AstSenItem* const senItemp
                = new AstSenItem{flp, VEdgeType{sense.edgeType},
                                 new AstVarRef{flp, sense.varScopep, VAccess::READ}};
            sentreep->addSensesp(senItemp);
        }
        반환 sentreep;
    }

    // Lower one fully detected FSM graph into the concrete coverage machinery
    // used by generated models: declarations, previous-state tracking, and the
    // pre/post-triggered increment logic for states and arcs.
    void buildOne(const FsmGraph& graph) {
        UINFO(1, "buildOne lowering FSM " << graph.stateVarName()
                                          << " vertices=" << graph.vertices().size() << endl);
        AstAlways* const alwaysp = graph.stateAlwaysp();
        AstScope* const scopep = graph.scopep();
        AstVarScope* const stateVscp = graph.stateVarScopep();
        AstVarScope* const sampleVscp = graph.sampleVarScopep();
        FileLine* const flp = graph.fileline();
        AstNodeModule* const modp = scopep->modp();
        AstVarScope* const prevVscp
            = m_prevTmps.make(flp, scopep, sampleVscp->dtypep(), stateVscp->varp()->shortName());
        // The saved previous-state temp crosses the scheduler's pre/post split
        // in the same way as Verilator's built-in NBA shadow variables, so keep
        // both vars marked as post-life participants for stable MT ordering.
        stateVscp->optimizeLifePost(true);
        sampleVscp->optimizeLifePost(true);
        prevVscp->optimizeLifePost(true);

        AstActive* const initActivep
            = new AstActive{flp, "fsm-coverage-init",
                            new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Initial{}}}};
        initActivep->senTreeStorep(initActivep->sentreep());
        // Seed the previous-state temp during initialization so the first
        // clock edge compares against a defined state value.
        initActivep->addStmtsp(new AstInitialStatic{
            flp, new AstAssign{flp, new AstVarRef{flp, prevVscp, VAccess::WRITE},
                               new AstVarRef{flp, sampleVscp, VAccess::READ}}});
        scopep->addBlocksp(initActivep);

        AstAlwaysPost* const covPostp = new AstAlwaysPost{flp};
        bool updatePrevAfterPost = false;
        만약 (alwaysp) {
            // Save the previous state as plain sequential logic at the front of
            // the original always_ff body, then evaluate coverage in post logic
            // after the delayed state update commits. This avoids a scheduler
            // race between a separate AstAlwaysPre task and the real state
            // commit for direct parent-level registers.
            alwaysp->prependStmtsp(new AstAssign{flp, new AstVarRef{flp, prevVscp, VAccess::WRITE},
                                                 new AstVarRef{flp, sampleVscp, VAccess::READ}});
        } 아니면 {
            // Wrapper-derived register candidates do not have a parent
            // always_ff body to splice into. Sample coverage first, then save
            // the current state for the next clock tick; this survives cell
            // boundary scheduling where the real flop update lives elsewhere.
            updatePrevAfterPost = true;
            prevVscp->varp()->setIgnorePostRead();
        }

        {
            // Create and increment proper AstNodeCoverDecls
            const FsmCoverIncBuilder coverIncBuilder{graph, prevVscp};
            modp->addStmtsp(coverIncBuilder.getCoverDeclsp());
            initActivep->addStmtsp(coverIncBuilder.getInitialCoverIncp());
            covPostp->addStmtsp(
                new AstIf{flp,
                          new AstNeq{flp, new AstVarRef{flp, prevVscp, VAccess::READ},
                                     new AstVarRef{flp, sampleVscp, VAccess::READ}},
                          coverIncBuilder.getCoverIncp()});
        }

        만약 (updatePrevAfterPost) {
            covPostp->addStmtsp(new AstAssign{flp, new AstVarRef{flp, prevVscp, VAccess::WRITE},
                                              new AstVarRef{flp, sampleVscp, VAccess::READ}});
        }

        AstSenTree* const sentreep = buildSenTree(flp, graph.senses());
        AstActive* const activep = new AstActive{flp, "fsm-coverage", sentreep};
        activep->senTreeStorep(sentreep);
        scopep->addBlocksp(activep);
        activep->addStmtsp(covPostp);
    }

public:
    // CONSTRUCTORS
    // Lower every detected FSM graph from the shared local state into
    // concrete coverage instrumentation while the saved scoped pointers are
    // still valid in the same pass.
    explicit FsmLowerVisitor(const FsmState& state)
        : m_state{state} {
        위해 (const DetectedFsm& fsm : m_state.fsms()) { buildOne(*fsm.graphp); }
    }
};

}  // namespace

void V3FsmDetect::detect(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    FsmState state;
    // Phase 1: recover each supported FSM into a complete graph while the
    // original clocked/case structure is still easy to recognize.
    FsmDetectVisitor detect{state, rootp};
    만약 (dumpGraphLevel() >= 6) {
        size_t index = 0;
        위해 (const DetectedFsm& fsm : state.fsms()) {
            fsm.graphp->dumpDotFilePrefixed(fsm.graphp->dumpTag(index++));
        }
    }
    // Phase 2: lower the completed in-memory graph state immediately, without
    // crossing into another pass owner or serializing through AST placeholders.
    { FsmLowerVisitor lower{state}; }
    V3Global::dumpCheckGlobalTree("fsm-detect", 0, dumpTreeEitherLevel() >= 3);
}
