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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Gate.h"

#অন্তর্ভাব "V3AstUserAllocator.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3DupFinder.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <list>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

শ্রেণী GateEitherVertex VL_NOT_FINAL : সর্বজনীন V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    যৌক্তিক m_reducible = সত্য;  // True if this node should be able to be eliminated
    যৌক্তিক m_dedupable = সত্য;  // True if this node should be able to be deduped
    যৌক্তিক m_consumed = মিথ্যা;  // Output goes to something meaningful
    যৌক্তিক m_staticInit = মিথ্যা;  // True if this node is a static initializer
সর্বজনীন:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = পূর্বনির্ধারিত;

    // ACCESSORS
    যৌক্তিক reducible() ধ্রুবক { ফেরত m_reducible; }
    যৌক্তিক dedupable() ধ্রুবক { ফেরত m_dedupable; }
    যৌক্তিক consumed() ধ্রুবক { ফেরত m_consumed; }
    যৌক্তিক staticInit() ধ্রুবক { ফেরত m_staticInit; }
    শূন্য setConsumed(ধ্রুবক অক্ষর* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = সত্য;
    }
    শূন্য setStaticInit() { m_staticInit = সত্য; }
    শূন্য clearReducible(ধ্রুবক অক্ষর* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = মিথ্যা;
    }
    শূন্য clearDedupable(ধ্রুবক অক্ষর* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = মিথ্যা;
    }
    শূন্য clearReducibleAndDedupable(ধ্রুবক অক্ষর* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ধ্রুবক override { ফেরত m_consumed ? "" : "dotted"; }
};

শ্রেণী GateVarVertex final : সর্বজনীন GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* ধ্রুবক m_varScp;
    যৌক্তিক m_is_শীর্ষ = মিথ্যা;
    যৌক্তিক m_isClock = মিথ্যা;
    Ast_গ্রন্থি* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_গ্রন্থি* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
সর্বজনীন:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = পূর্বনির্ধারিত;

    // ACCESSORS
    AstVarScope* varScp() ধ্রুবক VL_MT_STABLE { ফেরত m_varScp; }
    যৌক্তিক is_শীর্ষ() ধ্রুবক { ফেরত m_is_শীর্ষ; }
    শূন্য set_Is_শীর্ষ() { m_is_শীর্ষ = সত্য; }
    যৌক্তিক isClock() ধ্রুবক { ফেরত m_isClock; }
    শূন্য setIsClock() { m_isClock = সত্য; }
    Ast_গ্রন্থি* rstSyncNodep() ধ্রুবক { ফেরত m_rstSyncNodep; }
    শূন্য rstSyncNodep(Ast_গ্রন্থি* nodep) { m_rstSyncNodep = nodep; }
    Ast_গ্রন্থি* rstAsyncNodep() ধ্রুবক { ফেরত m_rstAsyncNodep; }
    শূন্য rstAsyncNodep(Ast_গ্রন্থি* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    শূন্য propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        যদি (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() ধ্রুবক override VL_MT_STABLE { ফেরত varScp()->name(); }
    std::string dotColor() ধ্রুবক override { ফেরত "green"; }
};

শ্রেণী GateLogicVertex final : সর্বজনীন GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_গ্রন্থি* ধ্রুবক m_nodep;
    AstActive* ধ্রুবক m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ধ্রুবক যৌক্তিক m_slow;  // In slow block
সর্বজনীন:
    GateLogicVertex(V3Graph* graphp, Ast_গ্রন্থি* nodep, AstActive* activep, যৌক্তিক slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = পূর্বনির্ধারিত;

    // ACCESSORS
    File_সারি* fileline() ধ্রুবক override { ফেরত nodep()->fileline(); }
    Ast_গ্রন্থি* nodep() ধ্রুবক { ফেরত m_nodep; }
    AstActive* activep() ধ্রুবক { ফেরত m_activep; }
    যৌক্তিক slow() ধ্রুবক { ফেরত m_slow; }

    // DOT debug
    std::string name() ধ্রুবক override { ফেরত m_nodep->fileline()->ascii(); }
    std::string dotColor() ধ্রুবক override { ফেরত "red"; }
};

শ্রেণী GateEdge final : সর্বজনীন V3GraphEdge {
    std::string dotLabel() ধ্রুবক override { ফেরত std::to_string(weight()); }

সর্বজনীন:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীর্ষ, পূর্ণসংখ্যা weight)
        : V3GraphEdge{graphp, fromp, শীর্ষ, weight} {}
};

শ্রেণী GateGraph final : সর্বজনীন V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ধ্রুবক VNUser1InUse m_inuser1;

সর্বজনীন:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        যদি (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = নতুন GateVarVertex{এই, vscp};
            vscp->user1p(vVtxp);
            যদি (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            যদি (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            যদি (vscp->varp()->isIO() && vscp->scopep()->is_শীর্ষ()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_শীর্ষ();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        ফেরত vVtxp;
    }

    শূন্য addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, পূর্ণসংখ্যা weight) {
        নতুন GateEdge{এই, srcp, dstp, weight};
    }

    শূন্য addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, পূর্ণসংখ্যা weight) {
        নতুন GateEdge{এই, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

শ্রেণী GateBuildVisitor final : সর্বজনীন VNVisitorConst {
    // STATE
    GateGraph* m_graphp = নতুন GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    ধ্রুবক Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    ধ্রুবক AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    যৌক্তিক m_inClockedActive = মিথ্যা;  // Underneath clocked active
    যৌক্তিক m_inEdgeActive = মিথ্যা;  // Underneath edge active
    যৌক্তিক m_inStaticActive = মিথ্যা;  // Underneath static active
    যৌক্তিক m_inInitialActive = মিথ্যা;  // Underneath initial active
    যৌক্তিক m_inSenItem = মিথ্যা;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    শূন্য check_গ্রন্থি(Ast_গ্রন্থি* nodep) {
        যদি (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        যদি (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    শূন্য iterateLogic(Ast_গ্রন্থি* nodep, যৌক্তিক slow = মিথ্যা, ধ্রুবক অক্ষর* nonReducibleReason = nullptr,
                      ধ্রুবক অক্ষর* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = নতুন GateLogicVertex{m_graphp, nodep, m_activep, slow};
        যদি (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } অন্যথা যদি (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } অন্যথা যদি (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        যদি (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_গ্রন্থি(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    শূন্য visit(AstActive* nodep) override {
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
        ধ্রুবক VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {
        ধ্রুবক যৌক্তিক slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    শূন্য visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, মিথ্যা, "CoverToggle", "CoverToggle");
    }
    শূন্য visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = সত্য;
        যদি (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } অন্যথা {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, মিথ্যা, nullptr, "senItem");
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (!m_logicVertexp) ফেরত;

        AstVarScope* ধ্রুবক vscp = nodep->varScopep();
        GateVarVertex* ধ্রুবক vVtxp = m_graphp->makeVarVertex(vscp);

        যদি (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(সত্য);
        } অন্যথা যদি (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            যদি (vscp->user2()) {
                যদি (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } অন্যথা {
                যদি (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        যদি (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        যদি (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    শূন্য visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), গ্রন্থি_Assign)
                      && VN_AS(nodep->backp(), গ্রন্থি_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (m_logicVertexp) check_গ্রন্থি(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

সর্বজনীন:
    স্থির std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        ফেরত std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

স্থির শূন্য v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ধ্রুবক VNUser2InUse user2InUse;
    জন্য (V3GraphVertex& vtx : graph.vertices()) {
        যদি (ধ্রুবক GateVarVertex* ধ্রুবক vvertexp = vtx.cast<GateVarVertex>()) {
            ধ্রুবক AstVarScope* ধ্রুবক vscp = vvertexp->varScp();
            ধ্রুবক Ast_গ্রন্থি* ধ্রুবক sp = vvertexp->rstSyncNodep();
            ধ্রুবক Ast_গ্রন্থি* ধ্রুবক ap = vvertexp->rstAsyncNodep();
            যদি (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                যদি (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(সত্য);  // Warn only once per signal
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

শ্রেণী GateConcatVisitor final : সর্বজনীন VNVisitorConst {
    // STATE
    ধ্রুবক AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    পূর্ণসংখ্যা m_offset = 0;  // Current offset of varscope
    পূর্ণসংখ্যা m_found_offset = 0;  // Found offset of varscope
    যৌক্তিক m_found = মিথ্যা;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(সত্য);
            m_found_offset = m_offset;
            m_found = সত্য;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    শূন্য visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সর্বজনীন:
    // PUBLIC METHODS
    যৌক্তিক concatOffset(AstConcat* concatp, AstVarScope* vscp, পূর্ণসংখ্যা& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = মিথ্যা;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        ফেরত m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

শ্রেণী GateOkVisitor final : সর্বজনীন VNVisitorConst {
    // RETURN STATE
    যৌক্তিক m_isSimple = সত্য;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_গ্রন্থি_রাশিমালা* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    ধ্রুবক যৌক্তিক m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ধ্রুবক যৌক্তিক m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ধ্রুবক Ast_গ্রন্থি_Var_Ref* m_lhsVarRef = nullptr;
    পূর্ণসংখ্যা m_ops = 0;  // Operation count

    // METHODS
    শূন্য clearSimple(ধ্রুবক অক্ষর* because) {
        যদি (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = মিথ্যা;
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Var_Ref* nodep) override {
        যদি (!m_isSimple) ফেরত;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        যদি (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        যদি (nodep->access().isRW()) {
            clearSimple("R/W");
            ফেরত;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ধ্রুবক vscp = nodep->varScopep();
        যদি (nodep->access().isWriteOnly()) {
            যদি (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } অন্যথা {
            // TODO: possible bug, should it be >= 1 as add is below?
            যদি (m_readVscps.আকার_2() > 1) {
                যদি (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ঠেলো_back(vscp);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (!m_isSimple) ফেরত;

        m_substitutionp = nodep->rhsp();
        যদি (!VN_IS(nodep->lhsp(), গ্রন্থি_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } অন্যথা যদি (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } অন্যথা {
            iterateChildrenConst(nodep);
        }
        যদি (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    শূন্য visit(AstCReset* /*nodep*/) override {
        যদি (!m_isSimple) ফেরত;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    শূন্য visit(AstMatchMasked* nodep) override {
        যদি (!m_isSimple) ফেরত;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        যদি (!m_isSimple) ফেরত;  // Fastpath

        যদি (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            ফেরত;
        }

        যদি (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            ফেরত;
        }

        iterateChildrenConst(nodep);
    }

সর্বজনীন:
    // CONSTRUCTORS
    GateOkVisitor(Ast_গ্রন্থি* nodep, যৌক্তিক buffersOnly, যৌক্তিক dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        যদি (!m_substitutionp) {
            clearSimple("No assignment found\n");
            ফেরত;
        }
        যদি (m_isSimple && m_lhsVarRef) {
            জন্য (ধ্রুবক AstVarScope* ধ্রুবক vscp : m_readVscps) {
                যদি (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    ফেরত;
                }
            }
        }
    }
    ~GateOkVisitor() override = পূর্বনির্ধারিত;
    // PUBLIC METHODS
    যৌক্তিক isSimple() ধ্রুবক { ফেরত m_isSimple; }
    Ast_গ্রন্থি_রাশিমালা* substitutionp() ধ্রুবক {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        ফেরত m_substitutionp;
    }
    ধ্রুবক std::vector<AstVarScope*>& readVscps() ধ্রুবক { ফেরত m_readVscps; }
    যৌক্তিক varAssigned(ধ্রুবক AstVarScope* scopep) ধ্রুবক {
        ফেরত m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

শ্রেণী GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_গ্রন্থি_রাশিমালা*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ধ্রুবক VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_গ্রন্থি, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    আকার_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_গ্রন্থি*, আকার_t> m_hasPending;
    আকার_t m_statInlined = 0;  // Statistic tracking - signals inlined
    আকার_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    আকার_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    স্থির যৌক্তিক isCheap(ধ্রুবক Ast_গ্রন্থি_রাশিমালা* exprp) {
        // Constant is cheap
        যদি (VN_IS(exprp, Const)) ফেরত সত্য;
        // Variable reference is cheap
        যদি (VN_IS(exprp, গ্রন্থি_Var_Ref)) ফেরত সত্য;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        যদি (ধ্রুবক AstSel* ধ্রুবক selp = VN_CAST(exprp, Sel)) {
            যদি (!isCheap(selp->fromp())) ফেরত মিথ্যা;
            যদি (!selp->isWide()) ফেরত সত্য;
            যদি (!VN_IS(selp->lsbp(), Const)) ফেরত মিথ্যা;
            যদি (selp->lsbConst() % VL_EDATASIZE != 0) ফেরত মিথ্যা;
            ফেরত সত্য;
        }
        // AstArraySel is cheap if the fromp is cheap
        যদি (ধ্রুবক AstArraySel* ধ্রুবক aselp = VN_CAST(exprp, ArraySel)) {
            ফেরত isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        ফেরত মিথ্যা;
    }

    যৌক্তিক shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, আকার_t nReads,
                      Ast_গ্রন্থি_রাশিমালা* substp, যৌক্তিক allowMultiIn) {
        // Always inline constants
        যদি (VN_IS(substp, Const)) ফেরত সত্য;
        // Don't inline non-constant static initializers - these are scheduled differently
        যদি (lVtxp->staticInit()) ফেরত মিথ্যা;
        // Inline simple variable references
        যদি (VN_IS(substp, VarRef)) ফেরত সত্য;
        // Only inline arrays if a simple variable or constant
        যদি (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) ফেরত মিথ্যা;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        যদি (nReads == 0) ফেরত সত্য;
        // If it reads one variable, inline if not wide, or if cheap
        যদি (nReads == 1 && (!substp->isWide() || isCheap(substp))) ফেরত সত্য;
        // Don't inline on first round if reads more than one variable
        যদি (nReads > 1 && !allowMultiIn) ফেরত মিথ্যা;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        পূর্ণসংখ্যা n = 0;
        জন্য (V3GraphEdge& edge : vVtxp->outEdges()) {
            ধ্রুবক GateLogicVertex* ধ্রুবক dstVtxp = edge.শীর্ষ()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            যদি (dstVtxp->slow()) চালু;
            যদি (dstVtxp->outEmpty() && !dstVtxp->consumed()) চালু;
            n += edge.weight();
            যদি (n > 1) {
                ++m_statNotInlined;
                ফেরত মিথ্যা;
            }
        }
        ফেরত সত্য;
    }

    শূন্য recordSubstitution(AstVarScope* vscp, Ast_গ্রন্থি_রাশিমালা* substp, Ast_গ্রন্থি* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ধ্রুবক auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        যদি (pair.second) pair.first->second = substp->clone_বৃক্ষ_Pure(মিথ্যা);
    }

    শূন্য commitSubstitutions(Ast_গ্রন্থি* logicp) {
        যদি (!m_hasPending.erase(logicp)) ফেরত;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<ধ্রুবক AstVarScope*> replaced(substitutions.আকার_2() * 2);

        ধ্রুবক std::function<শূন্য(Ast_গ্রন্থি_Var_Ref*)> visit = [&](Ast_গ্রন্থি_Var_Ref* nodep) -> শূন্য {
            // See if this variable has a substitution
            AstVarScope* ধ্রুবক vscp = nodep->varScopep();
            ধ্রুবক auto& it = substitutions.find(vscp);
            যদি (it == substitutions.end()) ফেরত;

            // Do not substitute circular logic
            যদি (!replaced.ঢোকাও(vscp).second) ফেরত;

            // Substitute nodep with substp
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, গ্রন্থি_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক newp = substp->clone_বৃক্ষ_Pure(মিথ্যা);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            যদি (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            যদি (Ast_গ্রন্থি_Var_Ref* ধ্রুবক varrefp = VN_CAST(newp, গ্রন্থি_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_গ্রন্থি* ধ্রুবক simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        জন্য (ধ্রুবক auto& pair : substitutions) pair.second->delete_বৃক্ষ();
        substitutions.clear();
    }

    শূন্য optimizeSignals(যৌক্তিক allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        ধ্রুবক auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            যখন (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            ফেরত it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        যখন (vIt != vertices.end()) {
            GateVarVertex* ধ্রুবক vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            যদি (!vVtxp->inSize1()) চালু;

            // Can't inline if non-reducible, etc
            যদি (!vVtxp->reducible()) চালু;

            // Grab the driving logic
            GateLogicVertex* ধ্রুবক lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            যদি (!lVtxp->reducible()) চালু;
            Ast_গ্রন্থি* ধ্রুবক logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ধ্রুবক GateOkVisitor okVisitor{logicp, মিথ্যা, মিথ্যা};

            // Was it ok?
            যদি (!okVisitor.isSimple()) চালু;
            // If the varScope is already removed from logicp, no need to try substitution.
            যদি (!okVisitor.varAssigned(vVtxp->varScp())) চালু;

            // Expression we are considering to substitute with
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ধ্রুবক আকার_t nReads = okVisitor.readVscps().আকার_2();

            // Check if it should be inlined
            যদি (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) চালু;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ধ্রুবক vscp = vVtxp->varScp();

            যদি (debug() >= 9) {
                vscp->dump_বৃক্ষ("substituting: ");
                substp->dump_বৃক্ষ("        with: ");
            }

            ধ্রুবক auto& readVscpsVec = okVisitor.readVscps();
            ধ্রুবক std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            জন্য (V3GraphEdge* ধ্রুবক edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* ধ্রুবক dstVtxp = edgep->শীর্ষ()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                যদি (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_গ্রন্থি* nodep = substp;
                    যদি (AstNot* ধ্রুবক notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    যদি (!VN_IS(nodep, VarRef)) চালু;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                যৌক্তিক canInline = সত্য;
                জন্য (ধ্রুবক V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    ধ্রুবক GateVarVertex* ধ্রুবক consVVertexp = dedge.শীর্ষ()->as<GateVarVertex>();
                    যদি (readVscps.গুনতি(consVVertexp->varScp())) {
                        canInline = মিথ্যা;
                        বিরতি;
                    }
                }

                যদি (!canInline) চালু;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                যদি (logicp == dstVtxp->nodep()) {
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
                    চালু;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                জন্য (AstVarScope* ধ্রুবক newVscp : okVisitor.readVscps()) {
                    GateVarVertex* ধ্রুবক varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            যদি (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_বৃক্ষ(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(মিথ্যা);
        // Then propagate more complicated equations
        optimizeSignals(সত্য);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_গ্রন্থি*, আকার_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](ধ্রুবক Pair& a, ধ্রুবক Pair& b) {  //
            ফেরত a.second < b.second;
        });
        জন্য (ধ্রুবক auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

সর্বজনীন:
    স্থির শূন্য apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

শ্রেণী GateDedupeHash final : সর্বজনীন V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    কাঠামো Aux_Ast_গ্রন্থি_রাশিমালা final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_গ্রন্থি_রাশিমালা* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_গ্রন্থি_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_গ্রন্থি_রাশিমালা, Aux_Ast_গ্রন্থি_রাশিমালা> m_aux_গ্রন্থি_রাশিমালা;

    Ast_গ্রন্থি_রাশিমালা* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_গ্রন্থি_রাশিমালা m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    স্থির যৌক্তিক same(Ast_গ্রন্থি* node1p, Ast_গ্রন্থি* node2p) {
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
        ফেরত node1p == node2p || (node1p && node1p->same_বৃক্ষ(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    যৌক্তিক isSame(Ast_গ্রন্থি* node1p, Ast_গ্রন্থি* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ধ্রুবক auto& aux2 = m_aux_গ্রন্থি_রাশিমালা(VN_AS(node2p, গ্রন্থি_রাশিমালা));
        ফেরত m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

সর্বজনীন:
    GateDedupeHash() = পূর্বনির্ধারিত;
    ~GateDedupeHash() = পূর্বনির্ধারিত;

    ধ্রুবক Ast_গ্রন্থি_Assign* hashAndFindDupe(Ast_গ্রন্থি_Assign* assignp, AstActive* activep,
                                         Ast_গ্রন্থি_রাশিমালা* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ধ্রুবক auto it = m_dupFinder.findDuplicate(m_currRhsp, এই);
        যদি (it != m_dupFinder.end()) ফেরত m_aux_গ্রন্থি_রাশিমালা(VN_AS(it->second, গ্রন্থি_রাশিমালা)).parentp;

        // Insert new node
        m_dupFinder.ঢোকাও(m_currRhsp);
        m_aux_গ্রন্থি_রাশিমালা(m_currRhsp) = m_auxCurRhsp;
        ফেরত nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

শ্রেণী GateDedupeVarVisitor final : সর্বজনীন VNVisitorConst {
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
    Ast_গ্রন্থি_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_গ্রন্থি_রাশিমালা* m_ifCondp = nullptr;  // IF condition that assign is under
    যৌক্তিক m_always = মিথ্যা;  // Assign is under an always
    যৌক্তিক m_dedupable = সত্য;  // Determined the assign to be dedupable

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Assign* nodep) override {
        যদি (!m_dedupable) ফেরত;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        যদি (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_রাশিমালা_Stmt*) { ফেরত সত্য; });
            ফেরত;
        }

        m_dedupable = মিথ্যা;
    }

    শূন্য visit(AstAlways* nodep) override {
        যদি (!m_dedupable) ফেরত;

        যদি (!m_always) {
            m_always = সত্য;
            iterate_And_পরবর্তী_Const_Null(nodep->stmtsp());
            ফেরত;
        }

        m_dedupable = মিথ্যা;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    শূন্য visit(Ast_গ্রন্থি_If* nodep) override {
        যদি (!m_dedupable) ফেরত;

        যদি (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_রাশিমালা_Stmt*) { ফেরত সত্য; });
            iterate_And_পরবর্তী_Const_Null(nodep->thensp());
            ফেরত;
        }

        m_dedupable = মিথ্যা;
    }

    শূন্য visit(AstComment*) override {}  // NOP
    //--------------------
    শূন্য visit(Ast_গ্রন্থি*) override { m_dedupable = মিথ্যা; }

সর্বজনীন:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = পূর্বনির্ধারিত;
    ~GateDedupeVarVisitor() override = পূর্বনির্ধারিত;
    // PUBLIC METHODS
    Ast_গ্রন্থি_Var_Ref* findDupe(Ast_গ্রন্থি* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = মিথ্যা;
        m_dedupable = সত্য;
        iterateConst(logicp);
        যদি (m_dedupable && m_assignp) {
            ধ্রুবক Ast_গ্রন্থি* ধ্রুবক lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ধ্রুবক Ast_গ্রন্থি_Var_Ref* ধ্রুবক lRefp = VN_CAST(lhsp, গ্রন্থি_Var_Ref);
            যদি (lRefp && lRefp->varScopep() == consumerVscp) {
                যদি (ধ্রুবক Ast_গ্রন্থি_Assign* ধ্রুবক dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    ফেরত static_cast<Ast_গ্রন্থি_Var_Ref*>(dup->lhsp());
                }
            }
        }
        ফেরত nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

শ্রেণী GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ধ্রুবক VNUser2InUse m_inuser2;

    // STATE
    আকার_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    শূন্য visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        যদি (m_depth > GATE_DEDUP_MAX_DEPTH) ফেরত;

        // Check that we haven't been here before
        যদি (vVtxp->varScp()->user2SetOnce()) ফেরত;

        VL_RESTORER(m_depth);
        ++m_depth;
        যদি (!vVtxp->inSize1()) ফেরত;

        Ast_গ্রন্থি_Var_Ref* dupRefp = nullptr;
        জন্য (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        যদি (!dupRefp) ফেরত;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* ধ্রুবক lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        ধ্রুবক GateOkVisitor okVisitor{lVtxp->nodep(), মিথ্যা, সত্য};
        যদি (!okVisitor.isSimple()) ফেরত;

        ++m_statDedupLogic;
        GateVarVertex* ধ্রুবক dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        জন্য (V3GraphEdge* ধ্রুবক edgep : vVtxp->outEdges().unlinkable()) {
            ধ্রুবক GateLogicVertex* ধ্রুবক consumerVtxp = edgep->শীর্ষ()->as<GateLogicVertex>();
            Ast_গ্রন্থি* ধ্রুবক consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            যদি (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } অন্যথা {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_গ্রন্থি_Var_Ref* refp) {
                    যদি (refp->varScopep() != vVtxp->varScp()) ফেরত;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_গ্রন্থি_Var_Ref* ধ্রুবক newp = dupRefp->clone_বৃক্ষ_Pure(মিথ্যা);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        যখন (V3GraphEdge* ধ্রুবক edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_গ্রন্থি_Var_Ref* visit(GateLogicVertex* lVtxp, ধ্রুবক GateVarVertex* consumerVtxp) {
        জন্য (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        যদি (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            ফেরত m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        ফেরত nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        জন্য (V3GraphVertex& vtx : graph.vertices()) {
            যদি (GateVarVertex* ধ্রুবক vVtxp = vtx.cast<GateVarVertex>()) {
                যদি (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        জন্য (V3GraphVertex& vtx : graph.vertices()) {
            যদি (GateVarVertex* ধ্রুবক vVtxp = vtx.cast<GateVarVertex>()) {
                যদি (vVtxp->is_শীর্ষ() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

সর্বজনীন:
    স্থির শূন্য apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

শ্রেণী GateMergeAssignments final {
    GateGraph& m_graph;
    আকার_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* পূর্ব_Selp, AstSel* currSelp) {
        ধ্রুবক AstVarRef* ধ্রুবক pRefp = VN_CAST(পূর্ব_Selp->fromp(), VarRef);
        AstVarRef* ধ্রুবক cRefp = VN_CAST(currSelp->fromp(), VarRef);
        যদি (!pRefp || !cRefp || !cRefp->same_গ্রন্থি(pRefp)) ফেরত nullptr;  // not the same var

        ধ্রুবক AstConst* ধ্রুবক pstart = VN_CAST(পূর্ব_Selp->lsbp(), Const);
        ধ্রুবক AstConst* ধ্রুবক cstart = VN_CAST(currSelp->lsbp(), Const);
        যদি (!pstart || !cstart) ফেরত nullptr;  // too complicated

        যদি (currSelp->msbConst() + 1 == পূর্ব_Selp->lsbConst()) {
            ফেরত নতুন AstSel{cRefp->fileline(), cRefp->clone_বৃক্ষ(মিথ্যা), currSelp->lsbConst(),
                              পূর্ব_Selp->widthConst() + currSelp->widthConst()};
        } অন্যথা {
            ফেরত nullptr;
        }
    }

    শূন্য process(GateVarVertex* vVtxp) {
        GateLogicVertex* পূর্ব_LVtxp = nullptr;
        AstAssignW* পূর্ব_Assignp = nullptr;

        জন্য (V3GraphEdge* ধ্রুবক edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* ধ্রুবক lVtxp = edgep->fromp()->as<GateLogicVertex>();
            যদি (!lVtxp->outSize1()) চালু;

            AstAlways* ধ্রুবক alwaysp = VN_CAST(lVtxp->nodep(), Always);
            যদি (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) ফেরত;
            AstAssignW* ধ্রুবক assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            যদি (!assignp) চালু;

            যদি (!VN_IS(assignp->lhsp(), Sel)) চালু;

            // First assign with Sel-lhs, or not under the same active
            যদি (!পূর্ব_LVtxp || পূর্ব_LVtxp->activep() != lVtxp->activep()) {
                পূর্ব_LVtxp = lVtxp;
                পূর্ব_Assignp = assignp;
                চালু;
            }

            AstSel* ধ্রুবক পূর্ব_Selp = VN_AS(পূর্ব_Assignp->lhsp(), Sel);
            AstSel* ধ্রুবক currSelp = VN_AS(assignp->lhsp(), Sel);

            যদি (AstSel* ধ্রুবক newSelp = merge(পূর্ব_Selp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                পূর্ব_Selp->replaceWith(newSelp);
                VL_DO_DANGLING(পূর্ব_Selp->delete_বৃক্ষ(), পূর্ব_Selp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                পূর্ব_Assignp->rhsp(নতুন AstConcat{পূর্ব_Assignp->rhsp()->fileline(),
                                                পূর্ব_Assignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                পূর্ব_Assignp->dtypeChgWidthSigned(পূর্ব_Assignp->width() + assignp->width(),
                                                 পূর্ব_Assignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                যখন (V3GraphEdge* ধ্রুবক iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* ধ্রুবক fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, পূর্ব_LVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } অন্যথা {
                পূর্ব_LVtxp = lVtxp;
                পূর্ব_Assignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        জন্য (V3GraphVertex& vtx : graph.vertices()) {
            যদি (GateVarVertex* ধ্রুবক vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        জন্য (GateLogicVertex* ধ্রুবক lVtxp : m_toRemove) {
            Ast_গ্রন্থি* ধ্রুবক nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

সর্বজনীন:
    স্থির শূন্য apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

শ্রেণী GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    শূন্য markRecurse(GateEitherVertex* vtxp) {
        যদি (vtxp->user()) ফেরত;  // Already marked
        vtxp->user(সত্য);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        জন্য (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* ধ্রুবক fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    শূন্য mark() {
        m_graph.userClearVertices();
        জন্য (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            যদি (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    স্থির শূন্য warnUnused(ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nodep) {
        যদি (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) ফেরত;

        যদি (ধ্রুবক Ast_গ্রন্থি_Procedure* ধ্রুবক procedurep = VN_CAST(nodep, গ্রন্থি_Procedure)) {
            যদি (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ধ্রুবক AstLoop* ধ্রুবক loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, সত্য);
                });
        }
    }

    // Remove unused logic
    শূন্য remove() {
        জন্য (V3GraphVertex* ধ্রুবক vtxp : m_graph.vertices().unlinkable()) {
            যদি (GateLogicVertex* ধ্রুবক lVtxp = vtxp->cast<GateLogicVertex>()) {
                যদি (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_গ্রন্থি* ধ্রুবক nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
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

সর্বজনীন:
    স্থির শূন্য apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

শূন্য V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        যদি (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->remove_Redundant_Edges_যোগ(&V3GraphEdge::followAlwaysTrue);
        যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        যদি (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        যদি (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        যদি (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        যদি (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_বৃক্ষ("gate", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
