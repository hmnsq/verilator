// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Gate optimizations, such as wire elimination
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Gate's Transformations:
//
// Extract a graph of the *entire* netlist with cells expanded
// Perform constant optimization across the graph
// Create VARSCOPEs for any variables we can rip out
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Gate.h"

#include "V3AstUserAllocator.h"
#include "V3Const.h"
#include "V3DupFinder.h"
#include "V3Graph.h"
#include "V3Stats.h"

#include <list>
#include <unordered_map>
#include <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

class GateEitherVertex VL_NOT_FINAL : public V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    bool m_reducible = true;  // True if this node should be able to be eliminated
    bool m_dedupable = true;  // True if this node should be able to be deduped
    bool m_consumed = false;  // Output goes to something meaningful
    bool m_staticInit = false;  // True if this node is a static initializer
public:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = default;

    // ACCESSORS
    bool reducible() const { คืน m_reducible; }
    bool dedupable() const { คืน m_dedupable; }
    bool consumed() const { คืน m_consumed; }
    bool staticInit() const { คืน m_staticInit; }
    void setConsumed(const var_na* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = true;
    }
    void setStaticInit() { m_staticInit = true; }
    void clearReducible(const var_na* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = false;
    }
    void clearDedupable(const var_na* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = false;
    }
    void clearReducibleAndDedupable(const var_na* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() const override { คืน m_consumed ? "" : "dotted"; }
};

class GateVarVertex final : public GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* const m_varScp;
    bool m_isTop = false;
    bool m_isClock = false;
    AstNode* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    AstNode* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
public:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = default;

    // ACCESSORS
    AstVarScope* varScp() const VL_MT_STABLE { คืน m_varScp; }
    bool isTop() const { คืน m_isTop; }
    void setIsTop() { m_isTop = true; }
    bool isClock() const { คืน m_isClock; }
    void setIsClock() { m_isClock = true; }
    AstNode* rstSyncNodep() const { คืน m_rstSyncNodep; }
    void rstSyncNodep(AstNode* nodep) { m_rstSyncNodep = nodep; }
    AstNode* rstAsyncNodep() const { คืน m_rstAsyncNodep; }
    void rstAsyncNodep(AstNode* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    void propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        ถ้า (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() const override VL_MT_STABLE { คืน varScp()->name(); }
    std::string dotColor() const override { คืน "green"; }
};

class GateLogicVertex final : public GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    AstNode* const m_nodep;
    AstActive* const m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    const bool m_slow;  // In slow block
public:
    GateLogicVertex(V3Graph* graphp, AstNode* nodep, AstActive* activep, bool slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = default;

    // ACCESSORS
    FileLine* fileline() const override { คืน nodep()->fileline(); }
    AstNode* nodep() const { คืน m_nodep; }
    AstActive* activep() const { คืน m_activep; }
    bool slow() const { คืน m_slow; }

    // DOT debug
    std::string name() const override { คืน m_nodep->fileline()->ascii(); }
    std::string dotColor() const override { คืน "red"; }
};

class GateEdge final : public V3GraphEdge {
    std::string dotLabel() const override { คืน std::to_string(weight()); }

public:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* top, จำนวนเต็ม weight)
        : V3GraphEdge{graphp, fromp, top, weight} {}
};

class GateGraph final : public V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    const VNUser1InUse m_inuser1;

public:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        ถ้า (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = new GateVarVertex{this, vscp};
            vscp->user1p(vVtxp);
            ถ้า (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            ถ้า (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            ถ้า (vscp->varp()->isIO() && vscp->scopep()->isTop()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->setIsTop();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        คืน vVtxp;
    }

    void addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, จำนวนเต็ม weight) {
        new GateEdge{this, srcp, dstp, weight};
    }

    void addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, จำนวนเต็ม weight) {
        new GateEdge{this, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

class GateBuildVisitor final : public VNVisitorConst {
    // STATE
    GateGraph* m_graphp = new GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    const AstNodeModule* m_modp = nullptr;  // Current module
    const AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    bool m_inClockedActive = false;  // Underneath clocked active
    bool m_inEdgeActive = false;  // Underneath edge active
    bool m_inStaticActive = false;  // Underneath static active
    bool m_inInitialActive = false;  // Underneath initial active
    bool m_inSenItem = false;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    void checkNode(AstNode* nodep) {
        ถ้า (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        ถ้า (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    void iterateLogic(AstNode* nodep, bool slow = false, const var_na* nonReducibleReason = nullptr,
                      const var_na* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = new GateLogicVertex{m_graphp, nodep, m_activep, slow};
        ถ้า (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } มิฉะนั้น ถ้า (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } มิฉะนั้น ถ้า (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        ถ้า (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        checkNode(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    void visit(AstNodeModule* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    void visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    void visit(AstActive* nodep) override {
        UASSERT_OBJ(!m_activep, nodep, "Should not nest");
        VL_RESTORER(m_activep);
        VL_RESTORER(m_inClockedActive);
        VL_RESTORER(m_inEdgeActive);
        VL_RESTORER(m_inStaticActive);
        VL_RESTORER(m_inInitialActive);
        m_activep = nodep;
        m_inClockedActive = nodep->hasClocked();
        m_inEdgeActive = nodep->sentreep() && nodep->sentreep()->hasEdge();
        m_inStaticActive = nodep->hasStatic();
        m_inInitialActive = nodep->hasInitial();

        // AstVarScope::user2 -> bool: Signal used in SenItem in *this* active block
        const VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    void visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    void visit(AstNodeProcedure* nodep) override {
        const bool slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    void visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, false, "CoverToggle", "CoverToggle");
    }
    void visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = true;
        ถ้า (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } มิฉะนั้น {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, false, nullptr, "senItem");
        }
    }
    void visit(AstNodeVarRef* nodep) override {
        ถ้า (!m_logicVertexp) คืน;

        AstVarScope* const vscp = nodep->varScopep();
        GateVarVertex* const vVtxp = m_graphp->makeVarVertex(vscp);

        ถ้า (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(true);
        } มิฉะนั้น ถ้า (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            ถ้า (vscp->user2()) {
                ถ้า (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } มิฉะนั้น {
                ถ้า (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        ถ้า (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        ถ้า (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    void visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), NodeAssign)
                      && VN_AS(nodep->backp(), NodeAssign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    void visit(AstNode* nodep) override {
        ถ้า (m_logicVertexp) checkNode(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

public:
    static std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        คืน std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

static void v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    const VNUser2InUse user2InUse;
    สำหรับ (V3GraphVertex& vtx : graph.vertices()) {
        ถ้า (const GateVarVertex* const vvertexp = vtx.cast<GateVarVertex>()) {
            const AstVarScope* const vscp = vvertexp->varScp();
            const AstNode* const sp = vvertexp->rstSyncNodep();
            const AstNode* const ap = vvertexp->rstAsyncNodep();
            ถ้า (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                ถ้า (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(true);  // Warn only once per signal
                    vscp->v3warn(SYNCASYNCNET,
                                 "Signal flopped as both synchronous and async: "
                                     << vscp->prettyNameQ() << '\n'
                                     << ap->warnOther() << "... Location of async usage\n"
                                     << ap->warnContextPrimary() << '\n'
                                     << sp->warnOther() << "... Location of sync usage\n"
                                     << sp->warnContextSecondary());
                }
            }
        }
    }
}

//######################################################################
// Find a var's offset in a concatenation (only used by GateClkDecomp)

class GateConcatVisitor final : public VNVisitorConst {
    // STATE
    const AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    จำนวนเต็ม m_offset = 0;  // Current offset of varscope
    จำนวนเต็ม m_found_offset = 0;  // Found offset of varscope
    bool m_found = false;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    void visit(AstNodeVarRef* nodep) override {
        ถ้า (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(true);
            m_found_offset = m_offset;
            m_found = true;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    void visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

public:
    // PUBLIC METHODS
    bool concatOffset(AstConcat* concatp, AstVarScope* vscp, จำนวนเต็ม& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = false;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        คืน m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

class GateOkVisitor final : public VNVisitorConst {
    // RETURN STATE
    bool m_isSimple = true;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    AstNodeExpr* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    const bool m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    const bool m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    const AstNodeVarRef* m_lhsVarRef = nullptr;
    จำนวนเต็ม m_ops = 0;  // Operation count

    // METHODS
    void clearSimple(const var_na* because) {
        ถ้า (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = false;
    }

    // VISITORS
    void visit(AstNodeVarRef* nodep) override {
        ถ้า (!m_isSimple) คืน;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        ถ้า (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        ถ้า (nodep->access().isRW()) {
            clearSimple("R/W");
            คืน;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* const vscp = nodep->varScopep();
        ถ้า (nodep->access().isWriteOnly()) {
            ถ้า (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } มิฉะนั้น {
            // TODO: possible bug, should it be >= 1 as add is below?
            ถ้า (m_readVscps.size() > 1) {
                ถ้า (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.push_back(vscp);
        }
    }
    void visit(AstNodeAssign* nodep) override {
        ถ้า (!m_isSimple) คืน;

        m_substitutionp = nodep->rhsp();
        ถ้า (!VN_IS(nodep->lhsp(), NodeVarRef)) {
            clearSimple("ASSIGN(non-VARREF)");
        } มิฉะนั้น ถ้า (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } มิฉะนั้น {
            iterateChildrenConst(nodep);
        }
        ถ้า (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    void visit(AstCReset* /*nodep*/) override {
        ถ้า (!m_isSimple) คืน;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    void visit(AstMatchMasked* nodep) override {
        ถ้า (!m_isSimple) คืน;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    void visit(AstNode* nodep) override {
        ถ้า (!m_isSimple) คืน;  // Fastpath

        ถ้า (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            คืน;
        }

        ถ้า (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            คืน;
        }

        iterateChildrenConst(nodep);
    }

public:
    // CONSTRUCTORS
    GateOkVisitor(AstNode* nodep, bool buffersOnly, bool dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        ถ้า (!m_substitutionp) {
            clearSimple("No assignment found\n");
            คืน;
        }
        ถ้า (m_isSimple && m_lhsVarRef) {
            สำหรับ (const AstVarScope* const vscp : m_readVscps) {
                ถ้า (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    คืน;
                }
            }
        }
    }
    ~GateOkVisitor() override = default;
    // PUBLIC METHODS
    bool isSimple() const { คืน m_isSimple; }
    AstNodeExpr* substitutionp() const {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        คืน m_substitutionp;
    }
    const std::vector<AstVarScope*>& readVscps() const { คืน m_readVscps; }
    bool varAssigned(const AstVarScope* scopep) const {
        คืน m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

class GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, AstNodeExpr*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    const VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<AstNode, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<AstNode*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    static bool isCheap(const AstNodeExpr* exprp) {
        // Constant is cheap
        ถ้า (VN_IS(exprp, Const)) คืน true;
        // Variable reference is cheap
        ถ้า (VN_IS(exprp, NodeVarRef)) คืน true;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        ถ้า (const AstSel* const selp = VN_CAST(exprp, Sel)) {
            ถ้า (!isCheap(selp->fromp())) คืน false;
            ถ้า (!selp->isWide()) คืน true;
            ถ้า (!VN_IS(selp->lsbp(), Const)) คืน false;
            ถ้า (selp->lsbConst() % VL_EDATASIZE != 0) คืน false;
            คืน true;
        }
        // AstArraySel is cheap if the fromp is cheap
        ถ้า (const AstArraySel* const aselp = VN_CAST(exprp, ArraySel)) {
            คืน isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        คืน false;
    }

    bool shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      AstNodeExpr* substp, bool allowMultiIn) {
        // Always inline constants
        ถ้า (VN_IS(substp, Const)) คืน true;
        // Don't inline non-constant static initializers - these are scheduled differently
        ถ้า (lVtxp->staticInit()) คืน false;
        // Inline simple variable references
        ถ้า (VN_IS(substp, VarRef)) คืน true;
        // Only inline arrays if a simple variable or constant
        ถ้า (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) คืน false;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        ถ้า (nReads == 0) คืน true;
        // If it reads one variable, inline if not wide, or if cheap
        ถ้า (nReads == 1 && (!substp->isWide() || isCheap(substp))) คืน true;
        // Don't inline on first round if reads more than one variable
        ถ้า (nReads > 1 && !allowMultiIn) คืน false;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        จำนวนเต็ม n = 0;
        สำหรับ (V3GraphEdge& edge : vVtxp->outEdges()) {
            const GateLogicVertex* const dstVtxp = edge.top()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            ถ้า (dstVtxp->slow()) continue;
            ถ้า (dstVtxp->outEmpty() && !dstVtxp->consumed()) continue;
            n += edge.weight();
            ถ้า (n > 1) {
                ++m_statNotInlined;
                คืน false;
            }
        }
        คืน true;
    }

    void recordSubstitution(AstVarScope* vscp, AstNodeExpr* substp, AstNode* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        const auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        ถ้า (pair.second) pair.first->second = substp->cloneTreePure(false);
    }

    void commitSubstitutions(AstNode* logicp) {
        ถ้า (!m_hasPending.erase(logicp)) คืน;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<const AstVarScope*> replaced(substitutions.size() * 2);

        const std::function<void(AstNodeVarRef*)> visit = [&](AstNodeVarRef* nodep) -> void {
            // See if this variable has a substitution
            AstVarScope* const vscp = nodep->varScopep();
            const auto& it = substitutions.find(vscp);
            ถ้า (it == substitutions.end()) คืน;

            // Do not substitute circular logic
            ถ้า (!replaced.insert(vscp).second) คืน;

            // Substitute nodep with substp
            AstNodeExpr* const substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, NodeVarRef) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            AstNodeExpr* const newp = substp->cloneTreePure(false);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            ถ้า (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            ถ้า (AstNodeVarRef* const varrefp = VN_CAST(newp, NodeVarRef)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        AstNode* const simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        สำหรับ (const auto& pair : substitutions) pair.second->deleteTree();
        substitutions.clear();
    }

    void optimizeSignals(bool allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        const auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            ขณะ (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            คืน it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        ขณะ (vIt != vertices.end()) {
            GateVarVertex* const vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            ถ้า (!vVtxp->inSize1()) continue;

            // Can't inline if non-reducible, etc
            ถ้า (!vVtxp->reducible()) continue;

            // Grab the driving logic
            GateLogicVertex* const lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            ถ้า (!lVtxp->reducible()) continue;
            AstNode* const logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            const GateOkVisitor okVisitor{logicp, false, false};

            // Was it ok?
            ถ้า (!okVisitor.isSimple()) continue;
            // If the varScope is already removed from logicp, no need to try substitution.
            ถ้า (!okVisitor.varAssigned(vVtxp->varScp())) continue;

            // Expression we are considering to substitute with
            AstNodeExpr* const substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            const size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            ถ้า (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) continue;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* const vscp = vVtxp->varScp();

            ถ้า (debug() >= 9) {
                vscp->dumpTree("substituting: ");
                substp->dumpTree("        with: ");
            }

            const auto& readVscpsVec = okVisitor.readVscps();
            const std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            สำหรับ (V3GraphEdge* const edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* const dstVtxp = edgep->top()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                ถ้า (VN_IS(dstVtxp->nodep(), SenItem)) {
                    AstNode* nodep = substp;
                    ถ้า (AstNot* const notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    ถ้า (!VN_IS(nodep, VarRef)) continue;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                bool canInline = true;
                สำหรับ (const V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    const GateVarVertex* const consVVertexp = dedge.top()->as<GateVarVertex>();
                    ถ้า (readVscps.count(consVVertexp->varScp())) {
                        canInline = false;
                        bha_nga;
                    }
                }

                ถ้า (!canInline) continue;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                ถ้า (logicp == dstVtxp->nodep()) {
                    // This is a bit involved. The graph tells us that the logic is circular
                    // (driver is same as sink), however, okVisitor rejects a circular driver
                    // and we would not reach here if the driver logic was actually circular.
                    // The reason we end up here is because during graph building, the driver
                    // was circular, however, after committing some substitutions to it, it
                    // has become non-circular due to V3Const being applied inside
                    // 'commitSubstitutions'. We will trust GateOkVisitor telling the truth
                    // that the logic is not actually circular, meaning this edge is not
                    // actually needed, can just delete it and move on.
                    VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                    continue;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                สำหรับ (AstVarScope* const newVscp : okVisitor.readVscps()) {
                    GateVarVertex* const varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            ถ้า (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->deleteTree(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(false);
        // Then propagate more complicated equations
        optimizeSignals(true);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<AstNode*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](const Pair& a, const Pair& b) {  //
            คืน a.second < b.second;
        });
        สำหรับ (const auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

public:
    static void apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

class GateDedupeHash final : public V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    sa_nracanaa AuxAstNodeExpr final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        AstNodeExpr* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        AstNodeAssign* parentp = nullptr;
    };
    AstUser2Allocator<AstNodeExpr, AuxAstNodeExpr> m_auxNodeExpr;

    AstNodeExpr* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    AuxAstNodeExpr m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    static bool same(AstNode* node1p, AstNode* node2p) {
        // Regarding the complexity of this function 'same':
        // Applying this comparison function to a a set of n trees pairwise is O(n^2) in the
        // number of comparisons (number of pairs). AstNode::sameTree itself, is O(sizeOfTree) in
        // the worst case, which happens if the operands of sameTree are indeed identical copies,
        // which means this line is O(n^2*sizeOfTree), iff you are comparing identical copies of
        // the same tree. In practice the identity comparison over the pointers, and the short
        // circuiting in sameTree means that for comparing the same tree instance to itself, or
        // trees of different types/shapes is a lot closer to O(1), so this 'same' function is
        // Omega(n^2) and O(n^2*sizeOfTree), and in practice as we are mostly comparing the same
        // instance to itself or different trees, the complexity should be closer to the lower
        // bound.
        //
        // Also if you see where this 'same' function is used within isSame, it's only ever
        // comparing AstActive nodes, which are very likely not to compare equals (and for the
        // purposes of V3Gate, we probably only care about them either being identical instances,
        // or having the same sensitivities anyway, so if this becomes a problem, it can be
        // improved which should also speed things up), and AstNodeExpr for if conditions, which
        // are hopefully small.
        คืน node1p == node2p || (node1p && node1p->sameTree(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    bool isSame(AstNode* node1p, AstNode* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        const auto& aux2 = m_auxNodeExpr(VN_AS(node2p, NodeExpr));
        คืน m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

public:
    GateDedupeHash() = default;
    ~GateDedupeHash() = default;

    const AstNodeAssign* hashAndFindDupe(AstNodeAssign* assignp, AstActive* activep,
                                         AstNodeExpr* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        const auto it = m_dupFinder.findDuplicate(m_currRhsp, this);
        ถ้า (it != m_dupFinder.end()) คืน m_auxNodeExpr(VN_AS(it->second, NodeExpr)).parentp;

        // Insert new node
        m_dupFinder.insert(m_currRhsp);
        m_auxNodeExpr(m_currRhsp) = m_auxCurRhsp;
        คืน nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

class GateDedupeVarVisitor final : public VNVisitorConst {
    // Given a node, it is visited to try to find the AstNodeAssign under
    // it that can used for dedupe.
    // Right now, only the following node trees are supported for dedupe.
    // 1. AstNodeAssign
    // 2. AstAlways -> AstNodeAssign
    //   (Note, the assign must also be the only node under the always)
    // 3. AstAlways -> AstNodeIf -> AstNodeAssign
    //   (Note, the IF must be the only node under the always,
    //    and the assign must be the only node under the if, other than the ifcond)
    // Any other ordering or node type, except for an AstComment, makes it not dedupable
    // AstExprStmt in the subtree of a node also makes the node not dedupable.

    // STATE
    GateDedupeHash m_ghash;  // Hash used to find dupes of rhs of assign
    AstNodeAssign* m_assignp = nullptr;  // Assign found for dedupe
    AstNodeExpr* m_ifCondp = nullptr;  // IF condition that assign is under
    bool m_always = false;  // Assign is under an always
    bool m_dedupable = true;  // Determined the assign to be dedupable

    // VISITORS
    void visit(AstNodeAssign* nodep) override {
        ถ้า (!m_dedupable) คืน;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        ถ้า (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](AstExprStmt*) { คืน true; });
            คืน;
        }

        m_dedupable = false;
    }

    void visit(AstAlways* nodep) override {
        ถ้า (!m_dedupable) คืน;

        ถ้า (!m_always) {
            m_always = true;
            iterateAndNextConstNull(nodep->stmtsp());
            คืน;
        }

        m_dedupable = false;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    void visit(AstNodeIf* nodep) override {
        ถ้า (!m_dedupable) คืน;

        ถ้า (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](AstExprStmt*) { คืน true; });
            iterateAndNextConstNull(nodep->thensp());
            คืน;
        }

        m_dedupable = false;
    }

    void visit(AstComment*) override {}  // NOP
    //--------------------
    void visit(AstNode*) override { m_dedupable = false; }

public:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = default;
    ~GateDedupeVarVisitor() override = default;
    // PUBLIC METHODS
    AstNodeVarRef* findDupe(AstNode* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = false;
        m_dedupable = true;
        iterateConst(logicp);
        ถ้า (m_dedupable && m_assignp) {
            const AstNode* const lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            const AstNodeVarRef* const lRefp = VN_CAST(lhsp, NodeVarRef);
            ถ้า (lRefp && lRefp->varScopep() == consumerVscp) {
                ถ้า (const AstNodeAssign* const dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    คืน static_cast<AstNodeVarRef*>(dup->lhsp());
                }
            }
        }
        คืน nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

class GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    const VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    void visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        ถ้า (m_depth > GATE_DEDUP_MAX_DEPTH) คืน;

        // Check that we haven't been here before
        ถ้า (vVtxp->varScp()->user2SetOnce()) คืน;

        VL_RESTORER(m_depth);
        ++m_depth;
        ถ้า (!vVtxp->inSize1()) คืน;

        AstNodeVarRef* dupRefp = nullptr;
        สำหรับ (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        ถ้า (!dupRefp) คืน;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* const lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        const GateOkVisitor okVisitor{lVtxp->nodep(), false, true};
        ถ้า (!okVisitor.isSimple()) คืน;

        ++m_statDedupLogic;
        GateVarVertex* const dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        สำหรับ (V3GraphEdge* const edgep : vVtxp->outEdges().unlinkable()) {
            const GateLogicVertex* const consumerVtxp = edgep->top()->as<GateLogicVertex>();
            AstNode* const consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            ถ้า (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } มิฉะนั้น {
                // Substitute consumer logic
                consumerp->foreach([&](AstNodeVarRef* refp) {
                    ถ้า (refp->varScopep() != vVtxp->varScp()) คืน;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    AstNodeVarRef* const newp = dupRefp->cloneTreePure(false);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->deleteTree(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        ขณะ (V3GraphEdge* const edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    AstNodeVarRef* visit(GateLogicVertex* lVtxp, const GateVarVertex* consumerVtxp) {
        สำหรับ (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        ถ้า (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            คืน m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        คืน nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        สำหรับ (V3GraphVertex& vtx : graph.vertices()) {
            ถ้า (GateVarVertex* const vVtxp = vtx.cast<GateVarVertex>()) {
                ถ้า (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        สำหรับ (V3GraphVertex& vtx : graph.vertices()) {
            ถ้า (GateVarVertex* const vVtxp = vtx.cast<GateVarVertex>()) {
                ถ้า (vVtxp->isTop() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

public:
    static void apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

class GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        const AstVarRef* const pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* const cRefp = VN_CAST(currSelp->fromp(), VarRef);
        ถ้า (!pRefp || !cRefp || !cRefp->sameNode(pRefp)) คืน nullptr;  // not the same var

        const AstConst* const pstart = VN_CAST(prevSelp->lsbp(), Const);
        const AstConst* const cstart = VN_CAST(currSelp->lsbp(), Const);
        ถ้า (!pstart || !cstart) คืน nullptr;  // too complicated

        ถ้า (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            คืน new AstSel{cRefp->fileline(), cRefp->cloneTree(false), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } มิฉะนั้น {
            คืน nullptr;
        }
    }

    void process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        สำหรับ (V3GraphEdge* const edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* const lVtxp = edgep->fromp()->as<GateLogicVertex>();
            ถ้า (!lVtxp->outSize1()) continue;

            AstAlways* const alwaysp = VN_CAST(lVtxp->nodep(), Always);
            ถ้า (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) คืน;
            AstAssignW* const assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            ถ้า (!assignp) continue;

            ถ้า (!VN_IS(assignp->lhsp(), Sel)) continue;

            // First assign with Sel-lhs, or not under the same active
            ถ้า (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                continue;
            }

            AstSel* const prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* const currSelp = VN_AS(assignp->lhsp(), Sel);

            ถ้า (AstSel* const newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->deleteTree(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(new AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                ขณะ (V3GraphEdge* const iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* const fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } มิฉะนั้น {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        สำหรับ (V3GraphVertex& vtx : graph.vertices()) {
            ถ้า (GateVarVertex* const vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        สำหรับ (GateLogicVertex* const lVtxp : m_toRemove) {
            AstNode* const nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

public:
    static void apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

class GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    void markRecurse(GateEitherVertex* vtxp) {
        ถ้า (vtxp->user()) คืน;  // Already marked
        vtxp->user(true);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        สำหรับ (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* const fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    void mark() {
        m_graph.userClearVertices();
        สำหรับ (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            ถ้า (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    static void warnUnused(const AstNode* const nodep) {
        ถ้า (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) คืน;

        ถ้า (const AstNodeProcedure* const procedurep = VN_CAST(nodep, NodeProcedure)) {
            ถ้า (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](const AstLoop* const loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, true);
                });
        }
    }

    // Remove unused logic
    void remove() {
        สำหรับ (V3GraphVertex* const vtxp : m_graph.vertices().unlinkable()) {
            ถ้า (GateLogicVertex* const lVtxp = vtxp->cast<GateLogicVertex>()) {
                ถ้า (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    AstNode* const nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->deleteTree(), nodep);
                    VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
                }
            }
        }
    }

    explicit GateUnused(GateGraph& graph)
        : m_graph{graph} {
        mark();  // Mark all used vertices
        remove();  // Remove unused vertices
    }

public:
    static void apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

void V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        ถ้า (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        ถ้า (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        ถ้า (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        ถ้า (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        ถ้า (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            ถ้า (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        ถ้า (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            ถ้า (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        ถ้า (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dumpCheckGlobalTree("gate", 0, dumpTreeEitherLevel() >= 3);
}
