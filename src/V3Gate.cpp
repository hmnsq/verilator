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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Gate.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3Const.h"
#ضمن "V3DupFinder.h"
#ضمن "V3Graph.h"
#ضمن "V3Stats.h"

#ضمن <list>
#ضمن <unordered_map>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

صنف GateEitherVertex VL_NOT_FINAL : عام V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    منطقي m_reducible = ܫܪܪܐ;  // True if this node should be able to be eliminated
    منطقي m_dedupable = ܫܪܪܐ;  // True if this node should be able to be deduped
    منطقي m_consumed = ܕܓܠܐ;  // Output goes to something meaningful
    منطقي m_staticInit = ܕܓܠܐ;  // True if this node is a static initializer
عام:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = افتراضي;

    // ACCESSORS
    منطقي reducible() ثابت { ܦܢܐ m_reducible; }
    منطقي dedupable() ثابت { ܦܢܐ m_dedupable; }
    منطقي consumed() ثابت { ܦܢܐ m_consumed; }
    منطقي staticInit() ثابت { ܦܢܐ m_staticInit; }
    فراغ setConsumed(ثابت حرف* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = ܫܪܪܐ;
    }
    فراغ setStaticInit() { m_staticInit = ܫܪܪܐ; }
    فراغ clearReducible(ثابت حرف* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = ܕܓܠܐ;
    }
    فراغ clearDedupable(ثابت حرف* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = ܕܓܠܐ;
    }
    فراغ clearReducibleAndDedupable(ثابت حرف* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ثابت override { ܦܢܐ m_consumed ? "" : "dotted"; }
};

صنف GateVarVertex final : عام GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* ثابت m_varScp;
    منطقي m_isTop = ܕܓܠܐ;
    منطقي m_isClock = ܕܓܠܐ;
    AstNode* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    AstNode* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
عام:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = افتراضي;

    // ACCESSORS
    AstVarScope* varScp() ثابت VL_MT_STABLE { ܦܢܐ m_varScp; }
    منطقي isTop() ثابت { ܦܢܐ m_isTop; }
    فراغ setIsTop() { m_isTop = ܫܪܪܐ; }
    منطقي isClock() ثابت { ܦܢܐ m_isClock; }
    فراغ setIsClock() { m_isClock = ܫܪܪܐ; }
    AstNode* rstSyncNodep() ثابت { ܦܢܐ m_rstSyncNodep; }
    فراغ rstSyncNodep(AstNode* nodep) { m_rstSyncNodep = nodep; }
    AstNode* rstAsyncNodep() ثابت { ܦܢܐ m_rstAsyncNodep; }
    فراغ rstAsyncNodep(AstNode* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    فراغ propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        ܐܢ (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() ثابت override VL_MT_STABLE { ܦܢܐ varScp()->name(); }
    std::string dotColor() ثابت override { ܦܢܐ "green"; }
};

صنف GateLogicVertex final : عام GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    AstNode* ثابت m_nodep;
    AstActive* ثابت m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ثابت منطقي m_slow;  // In slow block
عام:
    GateLogicVertex(V3Graph* graphp, AstNode* nodep, AstActive* activep, منطقي slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = افتراضي;

    // ACCESSORS
    FileLine* fileline() ثابت override { ܦܢܐ nodep()->fileline(); }
    AstNode* nodep() ثابت { ܦܢܐ m_nodep; }
    AstActive* activep() ثابت { ܦܢܐ m_activep; }
    منطقي slow() ثابت { ܦܢܐ m_slow; }

    // DOT debug
    std::string name() ثابت override { ܦܢܐ m_nodep->fileline()->ascii(); }
    std::string dotColor() ثابت override { ܦܢܐ "red"; }
};

صنف GateEdge final : عام V3GraphEdge {
    std::string dotLabel() ثابت override { ܦܢܐ std::to_string(weight()); }

عام:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* top, صحيح weight)
        : V3GraphEdge{graphp, fromp, top, weight} {}
};

صنف GateGraph final : عام V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ثابت VNUser1InUse m_inuser1;

عام:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        ܐܢ (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = جديد GateVarVertex{هذا, vscp};
            vscp->user1p(vVtxp);
            ܐܢ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            ܐܢ (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            ܐܢ (vscp->varp()->isIO() && vscp->scopep()->isTop()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->setIsTop();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        ܦܢܐ vVtxp;
    }

    فراغ addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, صحيح weight) {
        جديد GateEdge{هذا, srcp, dstp, weight};
    }

    فراغ addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, صحيح weight) {
        جديد GateEdge{هذا, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

صنف GateBuildVisitor final : عام VNVisitorConst {
    // STATE
    GateGraph* m_graphp = جديد GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    ثابت AstNodeModule* m_modp = nullptr;  // Current module
    ثابت AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    منطقي m_inClockedActive = ܕܓܠܐ;  // Underneath clocked active
    منطقي m_inEdgeActive = ܕܓܠܐ;  // Underneath edge active
    منطقي m_inStaticActive = ܕܓܠܐ;  // Underneath static active
    منطقي m_inInitialActive = ܕܓܠܐ;  // Underneath initial active
    منطقي m_inSenItem = ܕܓܠܐ;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    فراغ checkNode(AstNode* nodep) {
        ܐܢ (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        ܐܢ (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    فراغ iterateLogic(AstNode* nodep, منطقي slow = ܕܓܠܐ, ثابت حرف* nonReducibleReason = nullptr,
                      ثابت حرف* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = جديد GateLogicVertex{m_graphp, nodep, m_activep, slow};
        ܐܢ (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } ܐܚܪܢܐ ܐܢ (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } ܐܚܪܢܐ ܐܢ (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        ܐܢ (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        checkNode(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ visit(AstActive* nodep) override {
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
        ثابت VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    فراغ visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    فراغ visit(AstNodeProcedure* nodep) override {
        ثابت منطقي slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    فراغ visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, ܕܓܠܐ, "CoverToggle", "CoverToggle");
    }
    فراغ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = ܫܪܪܐ;
        ܐܢ (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } ܐܚܪܢܐ {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, ܕܓܠܐ, nullptr, "senItem");
        }
    }
    فراغ visit(AstNodeVarRef* nodep) override {
        ܐܢ (!m_logicVertexp) ܦܢܐ;

        AstVarScope* ثابت vscp = nodep->varScopep();
        GateVarVertex* ثابت vVtxp = m_graphp->makeVarVertex(vscp);

        ܐܢ (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(ܫܪܪܐ);
        } ܐܚܪܢܐ ܐܢ (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            ܐܢ (vscp->user2()) {
                ܐܢ (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } ܐܚܪܢܐ {
                ܐܢ (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        ܐܢ (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        ܐܢ (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    فراغ visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), NodeAssign)
                      && VN_AS(nodep->backp(), NodeAssign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ visit(AstNode* nodep) override {
        ܐܢ (m_logicVertexp) checkNode(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

عام:
    ساكن std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        ܦܢܐ std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ساكن فراغ v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ثابت VNUser2InUse user2InUse;
    لكل (V3GraphVertex& vtx : graph.vertices()) {
        ܐܢ (ثابت GateVarVertex* ثابت vvertexp = vtx.cast<GateVarVertex>()) {
            ثابت AstVarScope* ثابت vscp = vvertexp->varScp();
            ثابت AstNode* ثابت sp = vvertexp->rstSyncNodep();
            ثابت AstNode* ثابت ap = vvertexp->rstAsyncNodep();
            ܐܢ (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                ܐܢ (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(ܫܪܪܐ);  // Warn only once per signal
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

صنف GateConcatVisitor final : عام VNVisitorConst {
    // STATE
    ثابت AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    صحيح m_offset = 0;  // Current offset of varscope
    صحيح m_found_offset = 0;  // Found offset of varscope
    منطقي m_found = ܕܓܠܐ;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    فراغ visit(AstNodeVarRef* nodep) override {
        ܐܢ (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(ܫܪܪܐ);
            m_found_offset = m_offset;
            m_found = ܫܪܪܐ;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    فراغ visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

عام:
    // PUBLIC METHODS
    منطقي concatOffset(AstConcat* concatp, AstVarScope* vscp, صحيح& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = ܕܓܠܐ;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        ܦܢܐ m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

صنف GateOkVisitor final : عام VNVisitorConst {
    // RETURN STATE
    منطقي m_isSimple = ܫܪܪܐ;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    AstNodeExpr* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    ثابت منطقي m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ثابت منطقي m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ثابت AstNodeVarRef* m_lhsVarRef = nullptr;
    صحيح m_ops = 0;  // Operation count

    // METHODS
    فراغ clearSimple(ثابت حرف* because) {
        ܐܢ (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = ܕܓܠܐ;
    }

    // VISITORS
    فراغ visit(AstNodeVarRef* nodep) override {
        ܐܢ (!m_isSimple) ܦܢܐ;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        ܐܢ (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        ܐܢ (nodep->access().isRW()) {
            clearSimple("R/W");
            ܦܢܐ;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ثابت vscp = nodep->varScopep();
        ܐܢ (nodep->access().isWriteOnly()) {
            ܐܢ (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } ܐܚܪܢܐ {
            // TODO: possible bug, should it be >= 1 as add is below?
            ܐܢ (m_readVscps.size() > 1) {
                ܐܢ (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.push_back(vscp);
        }
    }
    فراغ visit(AstNodeAssign* nodep) override {
        ܐܢ (!m_isSimple) ܦܢܐ;

        m_substitutionp = nodep->rhsp();
        ܐܢ (!VN_IS(nodep->lhsp(), NodeVarRef)) {
            clearSimple("ASSIGN(non-VARREF)");
        } ܐܚܪܢܐ ܐܢ (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } ܐܚܪܢܐ {
            iterateChildrenConst(nodep);
        }
        ܐܢ (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    فراغ visit(AstCReset* /*nodep*/) override {
        ܐܢ (!m_isSimple) ܦܢܐ;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    فراغ visit(AstMatchMasked* nodep) override {
        ܐܢ (!m_isSimple) ܦܢܐ;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    فراغ visit(AstNode* nodep) override {
        ܐܢ (!m_isSimple) ܦܢܐ;  // Fastpath

        ܐܢ (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            ܦܢܐ;
        }

        ܐܢ (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            ܦܢܐ;
        }

        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    GateOkVisitor(AstNode* nodep, منطقي buffersOnly, منطقي dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        ܐܢ (!m_substitutionp) {
            clearSimple("No assignment found\n");
            ܦܢܐ;
        }
        ܐܢ (m_isSimple && m_lhsVarRef) {
            لكل (ثابت AstVarScope* ثابت vscp : m_readVscps) {
                ܐܢ (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    ܦܢܐ;
                }
            }
        }
    }
    ~GateOkVisitor() override = افتراضي;
    // PUBLIC METHODS
    منطقي isSimple() ثابت { ܦܢܐ m_isSimple; }
    AstNodeExpr* substitutionp() ثابت {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        ܦܢܐ m_substitutionp;
    }
    ثابت std::vector<AstVarScope*>& readVscps() ثابت { ܦܢܐ m_readVscps; }
    منطقي varAssigned(ثابت AstVarScope* scopep) ثابت {
        ܦܢܐ m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

صنف GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, AstNodeExpr*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ثابت VNUser2InUse m_inuser2;

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
    ساكن منطقي isCheap(ثابت AstNodeExpr* exprp) {
        // Constant is cheap
        ܐܢ (VN_IS(exprp, Const)) ܦܢܐ ܫܪܪܐ;
        // Variable reference is cheap
        ܐܢ (VN_IS(exprp, NodeVarRef)) ܦܢܐ ܫܪܪܐ;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        ܐܢ (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            ܐܢ (!isCheap(selp->fromp())) ܦܢܐ ܕܓܠܐ;
            ܐܢ (!selp->isWide()) ܦܢܐ ܫܪܪܐ;
            ܐܢ (!VN_IS(selp->lsbp(), Const)) ܦܢܐ ܕܓܠܐ;
            ܐܢ (selp->lsbConst() % VL_EDATASIZE != 0) ܦܢܐ ܕܓܠܐ;
            ܦܢܐ ܫܪܪܐ;
        }
        // AstArraySel is cheap if the fromp is cheap
        ܐܢ (ثابت AstArraySel* ثابت aselp = VN_CAST(exprp, ArraySel)) {
            ܦܢܐ isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        ܦܢܐ ܕܓܠܐ;
    }

    منطقي shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      AstNodeExpr* substp, منطقي allowMultiIn) {
        // Always inline constants
        ܐܢ (VN_IS(substp, Const)) ܦܢܐ ܫܪܪܐ;
        // Don't inline non-constant static initializers - these are scheduled differently
        ܐܢ (lVtxp->staticInit()) ܦܢܐ ܕܓܠܐ;
        // Inline simple variable references
        ܐܢ (VN_IS(substp, VarRef)) ܦܢܐ ܫܪܪܐ;
        // Only inline arrays if a simple variable or constant
        ܐܢ (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) ܦܢܐ ܕܓܠܐ;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        ܐܢ (nReads == 0) ܦܢܐ ܫܪܪܐ;
        // If it reads one variable, inline if not wide, or if cheap
        ܐܢ (nReads == 1 && (!substp->isWide() || isCheap(substp))) ܦܢܐ ܫܪܪܐ;
        // Don't inline on first round if reads more than one variable
        ܐܢ (nReads > 1 && !allowMultiIn) ܦܢܐ ܕܓܠܐ;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        صحيح n = 0;
        لكل (V3GraphEdge& edge : vVtxp->outEdges()) {
            ثابت GateLogicVertex* ثابت dstVtxp = edge.top()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            ܐܢ (dstVtxp->slow()) استمر;
            ܐܢ (dstVtxp->outEmpty() && !dstVtxp->consumed()) استمر;
            n += edge.weight();
            ܐܢ (n > 1) {
                ++m_statNotInlined;
                ܦܢܐ ܕܓܠܐ;
            }
        }
        ܦܢܐ ܫܪܪܐ;
    }

    فراغ recordSubstitution(AstVarScope* vscp, AstNodeExpr* substp, AstNode* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ثابت auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        ܐܢ (pair.second) pair.first->second = substp->cloneTreePure(ܕܓܠܐ);
    }

    فراغ commitSubstitutions(AstNode* logicp) {
        ܐܢ (!m_hasPending.erase(logicp)) ܦܢܐ;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<ثابت AstVarScope*> replaced(substitutions.size() * 2);

        ثابت std::function<فراغ(AstNodeVarRef*)> visit = [&](AstNodeVarRef* nodep) -> فراغ {
            // See if this variable has a substitution
            AstVarScope* ثابت vscp = nodep->varScopep();
            ثابت auto& it = substitutions.find(vscp);
            ܐܢ (it == substitutions.end()) ܦܢܐ;

            // Do not substitute circular logic
            ܐܢ (!replaced.insert(vscp).second) ܦܢܐ;

            // Substitute nodep with substp
            AstNodeExpr* ثابت substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, NodeVarRef) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            AstNodeExpr* ثابت newp = substp->cloneTreePure(ܕܓܠܐ);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            ܐܢ (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            ܐܢ (AstNodeVarRef* ثابت varrefp = VN_CAST(newp, NodeVarRef)) {
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

        AstNode* ثابت simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        لكل (ثابت auto& pair : substitutions) pair.second->deleteTree();
        substitutions.clear();
    }

    فراغ optimizeSignals(منطقي allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        ثابت auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            ܟܡܐ (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            ܦܢܐ it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        ܟܡܐ (vIt != vertices.end()) {
            GateVarVertex* ثابت vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            ܐܢ (!vVtxp->inSize1()) استمر;

            // Can't inline if non-reducible, etc
            ܐܢ (!vVtxp->reducible()) استمر;

            // Grab the driving logic
            GateLogicVertex* ثابت lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            ܐܢ (!lVtxp->reducible()) استمر;
            AstNode* ثابت logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ثابت GateOkVisitor okVisitor{logicp, ܕܓܠܐ, ܕܓܠܐ};

            // Was it ok?
            ܐܢ (!okVisitor.isSimple()) استمر;
            // If the varScope is already removed from logicp, no need to try substitution.
            ܐܢ (!okVisitor.varAssigned(vVtxp->varScp())) استمر;

            // Expression we are considering to substitute with
            AstNodeExpr* ثابت substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ثابت size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            ܐܢ (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) استمر;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ثابت vscp = vVtxp->varScp();

            ܐܢ (debug() >= 9) {
                vscp->dumpTree("substituting: ");
                substp->dumpTree("        with: ");
            }

            ثابت auto& readVscpsVec = okVisitor.readVscps();
            ثابت std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            لكل (V3GraphEdge* ثابت edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* ثابت dstVtxp = edgep->top()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                ܐܢ (VN_IS(dstVtxp->nodep(), SenItem)) {
                    AstNode* nodep = substp;
                    ܐܢ (AstNot* ثابت notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    ܐܢ (!VN_IS(nodep, VarRef)) استمر;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                منطقي canInline = ܫܪܪܐ;
                لكل (ثابت V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    ثابت GateVarVertex* ثابت consVVertexp = dedge.top()->as<GateVarVertex>();
                    ܐܢ (readVscps.count(consVVertexp->varScp())) {
                        canInline = ܕܓܠܐ;
                        اكسر;
                    }
                }

                ܐܢ (!canInline) استمر;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                ܐܢ (logicp == dstVtxp->nodep()) {
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
                    استمر;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                لكل (AstVarScope* ثابت newVscp : okVisitor.readVscps()) {
                    GateVarVertex* ثابت varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            ܐܢ (vVtxp->outEmpty()) {
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
        optimizeSignals(ܕܓܠܐ);
        // Then propagate more complicated equations
        optimizeSignals(ܫܪܪܐ);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<AstNode*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](ثابت Pair& a, ثابت Pair& b) {  //
            ܦܢܐ a.second < b.second;
        });
        لكل (ثابت auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

عام:
    ساكن فراغ apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

صنف GateDedupeHash final : عام V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    بنية AuxAstNodeExpr final {
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

    ساكن منطقي same(AstNode* node1p, AstNode* node2p) {
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
        ܦܢܐ node1p == node2p || (node1p && node1p->sameTree(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    منطقي isSame(AstNode* node1p, AstNode* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ثابت auto& aux2 = m_auxNodeExpr(VN_AS(node2p, NodeExpr));
        ܦܢܐ m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

عام:
    GateDedupeHash() = افتراضي;
    ~GateDedupeHash() = افتراضي;

    ثابت AstNodeAssign* hashAndFindDupe(AstNodeAssign* assignp, AstActive* activep,
                                         AstNodeExpr* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ثابت auto it = m_dupFinder.findDuplicate(m_currRhsp, هذا);
        ܐܢ (it != m_dupFinder.end()) ܦܢܐ m_auxNodeExpr(VN_AS(it->second, NodeExpr)).parentp;

        // Insert new node
        m_dupFinder.insert(m_currRhsp);
        m_auxNodeExpr(m_currRhsp) = m_auxCurRhsp;
        ܦܢܐ nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

صنف GateDedupeVarVisitor final : عام VNVisitorConst {
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
    منطقي m_always = ܕܓܠܐ;  // Assign is under an always
    منطقي m_dedupable = ܫܪܪܐ;  // Determined the assign to be dedupable

    // VISITORS
    فراغ visit(AstNodeAssign* nodep) override {
        ܐܢ (!m_dedupable) ܦܢܐ;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        ܐܢ (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](AstExprStmt*) { ܦܢܐ ܫܪܪܐ; });
            ܦܢܐ;
        }

        m_dedupable = ܕܓܠܐ;
    }

    فراغ visit(AstAlways* nodep) override {
        ܐܢ (!m_dedupable) ܦܢܐ;

        ܐܢ (!m_always) {
            m_always = ܫܪܪܐ;
            iterateAndNextConstNull(nodep->stmtsp());
            ܦܢܐ;
        }

        m_dedupable = ܕܓܠܐ;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    فراغ visit(AstNodeIf* nodep) override {
        ܐܢ (!m_dedupable) ܦܢܐ;

        ܐܢ (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](AstExprStmt*) { ܦܢܐ ܫܪܪܐ; });
            iterateAndNextConstNull(nodep->thensp());
            ܦܢܐ;
        }

        m_dedupable = ܕܓܠܐ;
    }

    فراغ visit(AstComment*) override {}  // NOP
    //--------------------
    فراغ visit(AstNode*) override { m_dedupable = ܕܓܠܐ; }

عام:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = افتراضي;
    ~GateDedupeVarVisitor() override = افتراضي;
    // PUBLIC METHODS
    AstNodeVarRef* findDupe(AstNode* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = ܕܓܠܐ;
        m_dedupable = ܫܪܪܐ;
        iterateConst(logicp);
        ܐܢ (m_dedupable && m_assignp) {
            ثابت AstNode* ثابت lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ثابت AstNodeVarRef* ثابت lRefp = VN_CAST(lhsp, NodeVarRef);
            ܐܢ (lRefp && lRefp->varScopep() == consumerVscp) {
                ܐܢ (ثابت AstNodeAssign* ثابت dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    ܦܢܐ static_cast<AstNodeVarRef*>(dup->lhsp());
                }
            }
        }
        ܦܢܐ nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

صنف GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ثابت VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    فراغ visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        ܐܢ (m_depth > GATE_DEDUP_MAX_DEPTH) ܦܢܐ;

        // Check that we haven't been here before
        ܐܢ (vVtxp->varScp()->user2SetOnce()) ܦܢܐ;

        VL_RESTORER(m_depth);
        ++m_depth;
        ܐܢ (!vVtxp->inSize1()) ܦܢܐ;

        AstNodeVarRef* dupRefp = nullptr;
        لكل (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        ܐܢ (!dupRefp) ܦܢܐ;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* ثابت lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        ثابت GateOkVisitor okVisitor{lVtxp->nodep(), ܕܓܠܐ, ܫܪܪܐ};
        ܐܢ (!okVisitor.isSimple()) ܦܢܐ;

        ++m_statDedupLogic;
        GateVarVertex* ثابت dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        لكل (V3GraphEdge* ثابت edgep : vVtxp->outEdges().unlinkable()) {
            ثابت GateLogicVertex* ثابت consumerVtxp = edgep->top()->as<GateLogicVertex>();
            AstNode* ثابت consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            ܐܢ (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } ܐܚܪܢܐ {
                // Substitute consumer logic
                consumerp->foreach([&](AstNodeVarRef* refp) {
                    ܐܢ (refp->varScopep() != vVtxp->varScp()) ܦܢܐ;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    AstNodeVarRef* ثابت newp = dupRefp->cloneTreePure(ܕܓܠܐ);
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
        ܟܡܐ (V3GraphEdge* ثابت edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    AstNodeVarRef* visit(GateLogicVertex* lVtxp, ثابت GateVarVertex* consumerVtxp) {
        لكل (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        ܐܢ (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            ܦܢܐ m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        ܦܢܐ nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        لكل (V3GraphVertex& vtx : graph.vertices()) {
            ܐܢ (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) {
                ܐܢ (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        لكل (V3GraphVertex& vtx : graph.vertices()) {
            ܐܢ (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) {
                ܐܢ (vVtxp->isTop() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

عام:
    ساكن فراغ apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

صنف GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        ثابت AstVarRef* ثابت pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ثابت cRefp = VN_CAST(currSelp->fromp(), VarRef);
        ܐܢ (!pRefp || !cRefp || !cRefp->sameNode(pRefp)) ܦܢܐ nullptr;  // not the same var

        ثابت AstConst* ثابت pstart = VN_CAST(prevSelp->lsbp(), Const);
        ثابت AstConst* ثابت cstart = VN_CAST(currSelp->lsbp(), Const);
        ܐܢ (!pstart || !cstart) ܦܢܐ nullptr;  // too complicated

        ܐܢ (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            ܦܢܐ جديد AstSel{cRefp->fileline(), cRefp->cloneTree(ܕܓܠܐ), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } ܐܚܪܢܐ {
            ܦܢܐ nullptr;
        }
    }

    فراغ process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        لكل (V3GraphEdge* ثابت edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* ثابت lVtxp = edgep->fromp()->as<GateLogicVertex>();
            ܐܢ (!lVtxp->outSize1()) استمر;

            AstAlways* ثابت alwaysp = VN_CAST(lVtxp->nodep(), Always);
            ܐܢ (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) ܦܢܐ;
            AstAssignW* ثابت assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            ܐܢ (!assignp) استمر;

            ܐܢ (!VN_IS(assignp->lhsp(), Sel)) استمر;

            // First assign with Sel-lhs, or not under the same active
            ܐܢ (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                استمر;
            }

            AstSel* ثابت prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ثابت currSelp = VN_AS(assignp->lhsp(), Sel);

            ܐܢ (AstSel* ثابت newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->deleteTree(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(جديد AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                ܟܡܐ (V3GraphEdge* ثابت iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* ثابت fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } ܐܚܪܢܐ {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        لكل (V3GraphVertex& vtx : graph.vertices()) {
            ܐܢ (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        لكل (GateLogicVertex* ثابت lVtxp : m_toRemove) {
            AstNode* ثابت nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

عام:
    ساكن فراغ apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

صنف GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    فراغ markRecurse(GateEitherVertex* vtxp) {
        ܐܢ (vtxp->user()) ܦܢܐ;  // Already marked
        vtxp->user(ܫܪܪܐ);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        لكل (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* ثابت fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    فراغ mark() {
        m_graph.userClearVertices();
        لكل (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            ܐܢ (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ساكن فراغ warnUnused(ثابت AstNode* ثابت nodep) {
        ܐܢ (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) ܦܢܐ;

        ܐܢ (ثابت AstNodeProcedure* ثابت procedurep = VN_CAST(nodep, NodeProcedure)) {
            ܐܢ (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ثابت AstLoop* ثابت loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, ܫܪܪܐ);
                });
        }
    }

    // Remove unused logic
    فراغ remove() {
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            ܐܢ (GateLogicVertex* ثابت lVtxp = vtxp->cast<GateLogicVertex>()) {
                ܐܢ (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    AstNode* ثابت nodep = lVtxp->nodep();
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

عام:
    ساكن فراغ apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

فراغ V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        ܐܢ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        ܐܢ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        ܐܢ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        ܐܢ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        ܐܢ (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            ܐܢ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        ܐܢ (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            ܐܢ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        ܐܢ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dumpCheckGlobalTree("gate", 0, dumpTreeEitherLevel() >= 3);
}
