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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Gate.h"

#অন্তর্ভাৱ "V3AstUserAllocator.h"
#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3DupFinder.h"
#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <list>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

শ্ৰেণী GateEitherVertex VL_NOT_FINAL : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    তার্কিক m_reducible = সঁচা;  // True if this node should be able to be eliminated
    তার্কিক m_dedupable = সঁচা;  // True if this node should be able to be deduped
    তার্কিক m_consumed = মিছা;  // Output goes to something meaningful
    তার্কিক m_staticInit = মিছা;  // True if this node is a static initializer
সার্ৱজনিক:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = স্ৱভাৱ;

    // ACCESSORS
    তার্কিক reducible() ধ্রুৱ { ঘূৰাই m_reducible; }
    তার্কিক dedupable() ধ্রুৱ { ঘূৰাই m_dedupable; }
    তার্কিক consumed() ধ্রুৱ { ঘূৰাই m_consumed; }
    তার্কিক staticInit() ধ্রুৱ { ঘূৰাই m_staticInit; }
    শূন্য setConsumed(ধ্রুৱ ৱর্ণ* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = সঁচা;
    }
    শূন্য setStaticInit() { m_staticInit = সঁচা; }
    শূন্য clearReducible(ধ্রুৱ ৱর্ণ* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = মিছা;
    }
    শূন্য clearDedupable(ধ্রুৱ ৱর্ণ* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = মিছা;
    }
    শূন্য clearReducibleAndDedupable(ধ্রুৱ ৱর্ণ* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ধ্রুৱ override { ঘূৰাই m_consumed ? "" : "dotted"; }
};

শ্ৰেণী GateVarVertex final : সার্ৱজনিক GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* ধ্রুৱ m_varScp;
    তার্কিক m_is_শীৰ্ষ = মিছা;
    তার্কিক m_isClock = মিছা;
    Ast_গ্ৰন্থি* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_গ্ৰন্থি* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
সার্ৱজনিক:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = স্ৱভাৱ;

    // ACCESSORS
    AstVarScope* varScp() ধ্রুৱ VL_MT_STABLE { ঘূৰাই m_varScp; }
    তার্কিক is_শীৰ্ষ() ধ্রুৱ { ঘূৰাই m_is_শীৰ্ষ; }
    শূন্য set_Is_শীৰ্ষ() { m_is_শীৰ্ষ = সঁচা; }
    তার্কিক isClock() ধ্রুৱ { ঘূৰাই m_isClock; }
    শূন্য setIsClock() { m_isClock = সঁচা; }
    Ast_গ্ৰন্থি* rstSyncNodep() ধ্রুৱ { ঘূৰাই m_rstSyncNodep; }
    শূন্য rstSyncNodep(Ast_গ্ৰন্থি* nodep) { m_rstSyncNodep = nodep; }
    Ast_গ্ৰন্থি* rstAsyncNodep() ধ্রুৱ { ঘূৰাই m_rstAsyncNodep; }
    শূন্য rstAsyncNodep(Ast_গ্ৰন্থি* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    শূন্য propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        যদি (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই varScp()->name(); }
    std::string dotColor() ধ্রুৱ override { ঘূৰাই "green"; }
};

শ্ৰেণী GateLogicVertex final : সার্ৱজনিক GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_গ্ৰন্থি* ধ্রুৱ m_nodep;
    AstActive* ধ্রুৱ m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ধ্রুৱ তার্কিক m_slow;  // In slow block
সার্ৱজনিক:
    GateLogicVertex(V3Graph* graphp, Ast_গ্ৰন্থি* nodep, AstActive* activep, তার্কিক slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = স্ৱভাৱ;

    // ACCESSORS
    File_সাৰি* fileline() ধ্রুৱ override { ঘূৰাই nodep()->fileline(); }
    Ast_গ্ৰন্থি* nodep() ধ্রুৱ { ঘূৰাই m_nodep; }
    AstActive* activep() ধ্রুৱ { ঘূৰাই m_activep; }
    তার্কিক slow() ধ্রুৱ { ঘূৰাই m_slow; }

    // DOT debug
    std::string name() ধ্রুৱ override { ঘূৰাই m_nodep->fileline()->ascii(); }
    std::string dotColor() ধ্রুৱ override { ঘূৰাই "red"; }
};

শ্ৰেণী GateEdge final : সার্ৱজনিক V3GraphEdge {
    std::string dotLabel() ধ্রুৱ override { ঘূৰাই std::to_string(weight()); }

সার্ৱজনিক:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ, পূর্ণাঙ্ক weight)
        : V3GraphEdge{graphp, fromp, শীৰ্ষ, weight} {}
};

শ্ৰেণী GateGraph final : সার্ৱজনিক V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ধ্রুৱ VNUser1InUse m_inuser1;

সার্ৱজনিক:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        যদি (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = নূতন GateVarVertex{এতত্, vscp};
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
            যদি (vscp->varp()->isIO() && vscp->scopep()->is_শীৰ্ষ()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_শীৰ্ষ();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        ঘূৰাই vVtxp;
    }

    শূন্য addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, পূর্ণাঙ্ক weight) {
        নূতন GateEdge{এতত্, srcp, dstp, weight};
    }

    শূন্য addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, পূর্ণাঙ্ক weight) {
        নূতন GateEdge{এতত্, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

শ্ৰেণী GateBuildVisitor final : সার্ৱজনিক VNVisitorConst {
    // STATE
    GateGraph* m_graphp = নূতন GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    ধ্রুৱ Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    ধ্রুৱ AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    তার্কিক m_inClockedActive = মিছা;  // Underneath clocked active
    তার্কিক m_inEdgeActive = মিছা;  // Underneath edge active
    তার্কিক m_inStaticActive = মিছা;  // Underneath static active
    তার্কিক m_inInitialActive = মিছা;  // Underneath initial active
    তার্কিক m_inSenItem = মিছা;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    শূন্য check_গ্ৰন্থি(Ast_গ্ৰন্থি* nodep) {
        যদি (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        যদি (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    শূন্য iterateLogic(Ast_গ্ৰন্থি* nodep, তার্কিক slow = মিছা, ধ্রুৱ ৱর্ণ* nonReducibleReason = nullptr,
                      ধ্রুৱ ৱর্ণ* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = নূতন GateLogicVertex{m_graphp, nodep, m_activep, slow};
        যদি (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } অন্যথা যদি (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } অন্যথা যদি (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        যদি (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_গ্ৰন্থি(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
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
        ধ্রুৱ VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    শূন্য visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    শূন্য visit(Ast_গ্ৰন্থি_Procedure* nodep) override {
        ধ্রুৱ তার্কিক slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    শূন্য visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, মিছা, "CoverToggle", "CoverToggle");
    }
    শূন্য visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = সঁচা;
        যদি (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } অন্যথা {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, মিছা, nullptr, "senItem");
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        যদি (!m_logicVertexp) ঘূৰাই;

        AstVarScope* ধ্রুৱ vscp = nodep->varScopep();
        GateVarVertex* ধ্রুৱ vVtxp = m_graphp->makeVarVertex(vscp);

        যদি (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(সঁচা);
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
        UASSERT_OBJ(!(VN_IS(nodep->backp(), গ্ৰন্থি_Assign)
                      && VN_AS(nodep->backp(), গ্ৰন্থি_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        যদি (m_logicVertexp) check_গ্ৰন্থি(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

সার্ৱজনিক:
    স্থির std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        ঘূৰাই std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

স্থির শূন্য v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ধ্রুৱ VNUser2InUse user2InUse;
    বাবে (V3GraphVertex& vtx : graph.vertices()) {
        যদি (ধ্রুৱ GateVarVertex* ধ্রুৱ vvertexp = vtx.cast<GateVarVertex>()) {
            ধ্রুৱ AstVarScope* ধ্রুৱ vscp = vvertexp->varScp();
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ sp = vvertexp->rstSyncNodep();
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ ap = vvertexp->rstAsyncNodep();
            যদি (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                যদি (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(সঁচা);  // Warn only once per signal
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

শ্ৰেণী GateConcatVisitor final : সার্ৱজনিক VNVisitorConst {
    // STATE
    ধ্রুৱ AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    পূর্ণাঙ্ক m_offset = 0;  // Current offset of varscope
    পূর্ণাঙ্ক m_found_offset = 0;  // Found offset of varscope
    তার্কিক m_found = মিছা;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        যদি (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(সঁচা);
            m_found_offset = m_offset;
            m_found = সঁচা;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    শূন্য visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildrenConst(nodep); }

সার্ৱজনিক:
    // PUBLIC METHODS
    তার্কিক concatOffset(AstConcat* concatp, AstVarScope* vscp, পূর্ণাঙ্ক& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = মিছা;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        ঘূৰাই m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

শ্ৰেণী GateOkVisitor final : সার্ৱজনিক VNVisitorConst {
    // RETURN STATE
    তার্কিক m_isSimple = সঁচা;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_গ্ৰন্থি_ৰাশিমালা* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    ধ্রুৱ তার্কিক m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ধ্রুৱ তার্কিক m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* m_lhsVarRef = nullptr;
    পূর্ণাঙ্ক m_ops = 0;  // Operation count

    // METHODS
    শূন্য clearSimple(ধ্রুৱ ৱর্ণ* because) {
        যদি (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = মিছা;
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Var_Ref* nodep) override {
        যদি (!m_isSimple) ঘূৰাই;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        যদি (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        যদি (nodep->access().isRW()) {
            clearSimple("R/W");
            ঘূৰাই;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ধ্রুৱ vscp = nodep->varScopep();
        যদি (nodep->access().isWriteOnly()) {
            যদি (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } অন্যথা {
            // TODO: possible bug, should it be >= 1 as add is below?
            যদি (m_readVscps.size() > 1) {
                যদি (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ঠেলো_back(vscp);
        }
    }
    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        যদি (!m_isSimple) ঘূৰাই;

        m_substitutionp = nodep->rhsp();
        যদি (!VN_IS(nodep->lhsp(), গ্ৰন্থি_Var_Ref)) {
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
        যদি (!m_isSimple) ঘূৰাই;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    শূন্য visit(AstMatchMasked* nodep) override {
        যদি (!m_isSimple) ঘূৰাই;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override {
        যদি (!m_isSimple) ঘূৰাই;  // Fastpath

        যদি (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            ঘূৰাই;
        }

        যদি (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            ঘূৰাই;
        }

        iterateChildrenConst(nodep);
    }

সার্ৱজনিক:
    // CONSTRUCTORS
    GateOkVisitor(Ast_গ্ৰন্থি* nodep, তার্কিক buffersOnly, তার্কিক dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        যদি (!m_substitutionp) {
            clearSimple("No assignment found\n");
            ঘূৰাই;
        }
        যদি (m_isSimple && m_lhsVarRef) {
            বাবে (ধ্রুৱ AstVarScope* ধ্রুৱ vscp : m_readVscps) {
                যদি (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    ঘূৰাই;
                }
            }
        }
    }
    ~GateOkVisitor() override = স্ৱভাৱ;
    // PUBLIC METHODS
    তার্কিক isSimple() ধ্রুৱ { ঘূৰাই m_isSimple; }
    Ast_গ্ৰন্থি_ৰাশিমালা* substitutionp() ধ্রুৱ {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        ঘূৰাই m_substitutionp;
    }
    ধ্রুৱ std::vector<AstVarScope*>& readVscps() ধ্রুৱ { ঘূৰাই m_readVscps; }
    তার্কিক varAssigned(ধ্রুৱ AstVarScope* scopep) ধ্রুৱ {
        ঘূৰাই m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

শ্ৰেণী GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_গ্ৰন্থি_ৰাশিমালা*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ধ্রুৱ VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_গ্ৰন্থি, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_গ্ৰন্থি*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    স্থির তার্কিক isCheap(ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        // Constant is cheap
        যদি (VN_IS(exprp, Const)) ঘূৰাই সঁচা;
        // Variable reference is cheap
        যদি (VN_IS(exprp, গ্ৰন্থি_Var_Ref)) ঘূৰাই সঁচা;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        যদি (ধ্রুৱ AstSel* ধ্রুৱ selp = VN_CAST(exprp, Sel)) {
            যদি (!isCheap(selp->fromp())) ঘূৰাই মিছা;
            যদি (!selp->isWide()) ঘূৰাই সঁচা;
            যদি (!VN_IS(selp->lsbp(), Const)) ঘূৰাই মিছা;
            যদি (selp->lsbConst() % VL_EDATASIZE != 0) ঘূৰাই মিছা;
            ঘূৰাই সঁচা;
        }
        // AstArraySel is cheap if the fromp is cheap
        যদি (ধ্রুৱ AstArraySel* ধ্রুৱ aselp = VN_CAST(exprp, ArraySel)) {
            ঘূৰাই isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        ঘূৰাই মিছা;
    }

    তার্কিক shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_গ্ৰন্থি_ৰাশিমালা* substp, তার্কিক allowMultiIn) {
        // Always inline constants
        যদি (VN_IS(substp, Const)) ঘূৰাই সঁচা;
        // Don't inline non-constant static initializers - these are scheduled differently
        যদি (lVtxp->staticInit()) ঘূৰাই মিছা;
        // Inline simple variable references
        যদি (VN_IS(substp, VarRef)) ঘূৰাই সঁচা;
        // Only inline arrays if a simple variable or constant
        যদি (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) ঘূৰাই মিছা;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        যদি (nReads == 0) ঘূৰাই সঁচা;
        // If it reads one variable, inline if not wide, or if cheap
        যদি (nReads == 1 && (!substp->isWide() || isCheap(substp))) ঘূৰাই সঁচা;
        // Don't inline on first round if reads more than one variable
        যদি (nReads > 1 && !allowMultiIn) ঘূৰাই মিছা;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        পূর্ণাঙ্ক n = 0;
        বাবে (V3GraphEdge& edge : vVtxp->outEdges()) {
            ধ্রুৱ GateLogicVertex* ধ্রুৱ dstVtxp = edge.শীৰ্ষ()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            যদি (dstVtxp->slow()) অনুৱর্ত;
            যদি (dstVtxp->outEmpty() && !dstVtxp->consumed()) অনুৱর্ত;
            n += edge.weight();
            যদি (n > 1) {
                ++m_statNotInlined;
                ঘূৰাই মিছা;
            }
        }
        ঘূৰাই সঁচা;
    }

    শূন্য recordSubstitution(AstVarScope* vscp, Ast_গ্ৰন্থি_ৰাশিমালা* substp, Ast_গ্ৰন্থি* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ধ্রুৱ auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        যদি (pair.second) pair.first->second = substp->clone_বৃক্ষ_Pure(মিছা);
    }

    শূন্য commitSubstitutions(Ast_গ্ৰন্থি* logicp) {
        যদি (!m_hasPending.erase(logicp)) ঘূৰাই;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<ধ্রুৱ AstVarScope*> replaced(substitutions.size() * 2);

        ধ্রুৱ std::function<শূন্য(Ast_গ্ৰন্থি_Var_Ref*)> visit = [&](Ast_গ্ৰন্থি_Var_Ref* nodep) -> শূন্য {
            // See if this variable has a substitution
            AstVarScope* ধ্রুৱ vscp = nodep->varScopep();
            ধ্রুৱ auto& it = substitutions.find(vscp);
            যদি (it == substitutions.end()) ঘূৰাই;

            // Do not substitute circular logic
            যদি (!replaced.ঢোকাও(vscp).second) ঘূৰাই;

            // Substitute nodep with substp
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, গ্ৰন্থি_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ newp = substp->clone_বৃক্ষ_Pure(মিছা);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            যদি (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            যদি (Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ varrefp = VN_CAST(newp, গ্ৰন্থি_Var_Ref)) {
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

        Ast_গ্ৰন্থি* ধ্রুৱ simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        বাবে (ধ্রুৱ auto& pair : substitutions) pair.second->delete_বৃক্ষ();
        substitutions.clear();
    }

    শূন্য optimizeSignals(তার্কিক allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        ধ্রুৱ auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            যেতিয়ালৈ (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            ঘূৰাই it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        যেতিয়ালৈ (vIt != vertices.end()) {
            GateVarVertex* ধ্রুৱ vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            যদি (!vVtxp->inSize1()) অনুৱর্ত;

            // Can't inline if non-reducible, etc
            যদি (!vVtxp->reducible()) অনুৱর্ত;

            // Grab the driving logic
            GateLogicVertex* ধ্রুৱ lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            যদি (!lVtxp->reducible()) অনুৱর্ত;
            Ast_গ্ৰন্থি* ধ্রুৱ logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ধ্রুৱ GateOkVisitor okVisitor{logicp, মিছা, মিছা};

            // Was it ok?
            যদি (!okVisitor.isSimple()) অনুৱর্ত;
            // If the varScope is already removed from logicp, no need to try substitution.
            যদি (!okVisitor.varAssigned(vVtxp->varScp())) অনুৱর্ত;

            // Expression we are considering to substitute with
            Ast_গ্ৰন্থি_ৰাশিমালা* ধ্রুৱ substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ধ্রুৱ size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            যদি (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) অনুৱর্ত;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ধ্রুৱ vscp = vVtxp->varScp();

            যদি (debug() >= 9) {
                vscp->dump_বৃক্ষ("substituting: ");
                substp->dump_বৃক্ষ("        with: ");
            }

            ধ্রুৱ auto& readVscpsVec = okVisitor.readVscps();
            ধ্রুৱ std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            বাবে (V3GraphEdge* ধ্রুৱ edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* ধ্রুৱ dstVtxp = edgep->শীৰ্ষ()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                যদি (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_গ্ৰন্থি* nodep = substp;
                    যদি (AstNot* ধ্রুৱ notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    যদি (!VN_IS(nodep, VarRef)) অনুৱর্ত;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                তার্কিক canInline = সঁচা;
                বাবে (ধ্রুৱ V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    ধ্রুৱ GateVarVertex* ধ্রুৱ consVVertexp = dedge.শীৰ্ষ()->as<GateVarVertex>();
                    যদি (readVscps.গুনতি(consVVertexp->varScp())) {
                        canInline = মিছা;
                        ভঙ্গ;
                    }
                }

                যদি (!canInline) অনুৱর্ত;  // Cannot optimize this replacement

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
                    অনুৱর্ত;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                বাবে (AstVarScope* ধ্রুৱ newVscp : okVisitor.readVscps()) {
                    GateVarVertex* ধ্রুৱ varvertexp = m_graph.makeVarVertex(newVscp);
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
        optimizeSignals(মিছা);
        // Then propagate more complicated equations
        optimizeSignals(সঁচা);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_গ্ৰন্থি*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](ধ্রুৱ Pair& a, ধ্রুৱ Pair& b) {  //
            ঘূৰাই a.second < b.second;
        });
        বাবে (ধ্রুৱ auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

সার্ৱজনিক:
    স্থির শূন্য apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

শ্ৰেণী GateDedupeHash final : সার্ৱজনিক V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    সংরচনা Aux_Ast_গ্ৰন্থি_ৰাশিমালা final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_গ্ৰন্থি_ৰাশিমালা* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_গ্ৰন্থি_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_গ্ৰন্থি_ৰাশিমালা, Aux_Ast_গ্ৰন্থি_ৰাশিমালা> m_aux_গ্ৰন্থি_ৰাশিমালা;

    Ast_গ্ৰন্থি_ৰাশিমালা* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_গ্ৰন্থি_ৰাশিমালা m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    স্থির তার্কিক same(Ast_গ্ৰন্থি* node1p, Ast_গ্ৰন্থি* node2p) {
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
        ঘূৰাই node1p == node2p || (node1p && node1p->same_বৃক্ষ(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    তার্কিক isSame(Ast_গ্ৰন্থি* node1p, Ast_গ্ৰন্থি* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ধ্রুৱ auto& aux2 = m_aux_গ্ৰন্থি_ৰাশিমালা(VN_AS(node2p, গ্ৰন্থি_ৰাশিমালা));
        ঘূৰাই m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

সার্ৱজনিক:
    GateDedupeHash() = স্ৱভাৱ;
    ~GateDedupeHash() = স্ৱভাৱ;

    ধ্রুৱ Ast_গ্ৰন্থি_Assign* hashAndFindDupe(Ast_গ্ৰন্থি_Assign* assignp, AstActive* activep,
                                         Ast_গ্ৰন্থি_ৰাশিমালা* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ধ্রুৱ auto it = m_dupFinder.findDuplicate(m_currRhsp, এতত্);
        যদি (it != m_dupFinder.end()) ঘূৰাই m_aux_গ্ৰন্থি_ৰাশিমালা(VN_AS(it->second, গ্ৰন্থি_ৰাশিমালা)).parentp;

        // Insert new node
        m_dupFinder.ঢোকাও(m_currRhsp);
        m_aux_গ্ৰন্থি_ৰাশিমালা(m_currRhsp) = m_auxCurRhsp;
        ঘূৰাই nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

শ্ৰেণী GateDedupeVarVisitor final : সার্ৱজনিক VNVisitorConst {
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
    Ast_গ্ৰন্থি_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_গ্ৰন্থি_ৰাশিমালা* m_ifCondp = nullptr;  // IF condition that assign is under
    তার্কিক m_always = মিছা;  // Assign is under an always
    তার্কিক m_dedupable = সঁচা;  // Determined the assign to be dedupable

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Assign* nodep) override {
        যদি (!m_dedupable) ঘূৰাই;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        যদি (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_ৰাশিমালা_Stmt*) { ঘূৰাই সঁচা; });
            ঘূৰাই;
        }

        m_dedupable = মিছা;
    }

    শূন্য visit(AstAlways* nodep) override {
        যদি (!m_dedupable) ঘূৰাই;

        যদি (!m_always) {
            m_always = সঁচা;
            iterateAndNextConstNull(nodep->stmtsp());
            ঘূৰাই;
        }

        m_dedupable = মিছা;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        যদি (!m_dedupable) ঘূৰাই;

        যদি (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_ৰাশিমালা_Stmt*) { ঘূৰাই সঁচা; });
            iterateAndNextConstNull(nodep->thensp());
            ঘূৰাই;
        }

        m_dedupable = মিছা;
    }

    শূন্য visit(AstComment*) override {}  // NOP
    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি*) override { m_dedupable = মিছা; }

সার্ৱজনিক:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = স্ৱভাৱ;
    ~GateDedupeVarVisitor() override = স্ৱভাৱ;
    // PUBLIC METHODS
    Ast_গ্ৰন্থি_Var_Ref* findDupe(Ast_গ্ৰন্থি* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = মিছা;
        m_dedupable = সঁচা;
        iterateConst(logicp);
        যদি (m_dedupable && m_assignp) {
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ধ্রুৱ Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ lRefp = VN_CAST(lhsp, গ্ৰন্থি_Var_Ref);
            যদি (lRefp && lRefp->varScopep() == consumerVscp) {
                যদি (ধ্রুৱ Ast_গ্ৰন্থি_Assign* ধ্রুৱ dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    ঘূৰাই static_cast<Ast_গ্ৰন্থি_Var_Ref*>(dup->lhsp());
                }
            }
        }
        ঘূৰাই nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

শ্ৰেণী GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ধ্রুৱ VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    শূন্য visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        যদি (m_depth > GATE_DEDUP_MAX_DEPTH) ঘূৰাই;

        // Check that we haven't been here before
        যদি (vVtxp->varScp()->user2SetOnce()) ঘূৰাই;

        VL_RESTORER(m_depth);
        ++m_depth;
        যদি (!vVtxp->inSize1()) ঘূৰাই;

        Ast_গ্ৰন্থি_Var_Ref* dupRefp = nullptr;
        বাবে (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        যদি (!dupRefp) ঘূৰাই;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* ধ্রুৱ lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        ধ্রুৱ GateOkVisitor okVisitor{lVtxp->nodep(), মিছা, সঁচা};
        যদি (!okVisitor.isSimple()) ঘূৰাই;

        ++m_statDedupLogic;
        GateVarVertex* ধ্রুৱ dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        বাবে (V3GraphEdge* ধ্রুৱ edgep : vVtxp->outEdges().unlinkable()) {
            ধ্রুৱ GateLogicVertex* ধ্রুৱ consumerVtxp = edgep->শীৰ্ষ()->as<GateLogicVertex>();
            Ast_গ্ৰন্থি* ধ্রুৱ consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            যদি (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } অন্যথা {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_গ্ৰন্থি_Var_Ref* refp) {
                    যদি (refp->varScopep() != vVtxp->varScp()) ঘূৰাই;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_গ্ৰন্থি_Var_Ref* ধ্রুৱ newp = dupRefp->clone_বৃক্ষ_Pure(মিছা);
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
        যেতিয়ালৈ (V3GraphEdge* ধ্রুৱ edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_গ্ৰন্থি_Var_Ref* visit(GateLogicVertex* lVtxp, ধ্রুৱ GateVarVertex* consumerVtxp) {
        বাবে (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        যদি (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            ঘূৰাই m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        ঘূৰাই nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        বাবে (V3GraphVertex& vtx : graph.vertices()) {
            যদি (GateVarVertex* ধ্রুৱ vVtxp = vtx.cast<GateVarVertex>()) {
                যদি (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        বাবে (V3GraphVertex& vtx : graph.vertices()) {
            যদি (GateVarVertex* ধ্রুৱ vVtxp = vtx.cast<GateVarVertex>()) {
                যদি (vVtxp->is_শীৰ্ষ() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

সার্ৱজনিক:
    স্থির শূন্য apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

শ্ৰেণী GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        ধ্রুৱ AstVarRef* ধ্রুৱ pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ধ্রুৱ cRefp = VN_CAST(currSelp->fromp(), VarRef);
        যদি (!pRefp || !cRefp || !cRefp->same_গ্ৰন্থি(pRefp)) ঘূৰাই nullptr;  // not the same var

        ধ্রুৱ AstConst* ধ্রুৱ pstart = VN_CAST(prevSelp->lsbp(), Const);
        ধ্রুৱ AstConst* ধ্রুৱ cstart = VN_CAST(currSelp->lsbp(), Const);
        যদি (!pstart || !cstart) ঘূৰাই nullptr;  // too complicated

        যদি (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            ঘূৰাই নূতন AstSel{cRefp->fileline(), cRefp->clone_বৃক্ষ(মিছা), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } অন্যথা {
            ঘূৰাই nullptr;
        }
    }

    শূন্য process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        বাবে (V3GraphEdge* ধ্রুৱ edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* ধ্রুৱ lVtxp = edgep->fromp()->as<GateLogicVertex>();
            যদি (!lVtxp->outSize1()) অনুৱর্ত;

            AstAlways* ধ্রুৱ alwaysp = VN_CAST(lVtxp->nodep(), Always);
            যদি (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) ঘূৰাই;
            AstAssignW* ধ্রুৱ assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            যদি (!assignp) অনুৱর্ত;

            যদি (!VN_IS(assignp->lhsp(), Sel)) অনুৱর্ত;

            // First assign with Sel-lhs, or not under the same active
            যদি (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                অনুৱর্ত;
            }

            AstSel* ধ্রুৱ prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ধ্রুৱ currSelp = VN_AS(assignp->lhsp(), Sel);

            যদি (AstSel* ধ্রুৱ newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_বৃক্ষ(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(নূতন AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                যেতিয়ালৈ (V3GraphEdge* ধ্রুৱ iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* ধ্রুৱ fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } অন্যথা {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        বাবে (V3GraphVertex& vtx : graph.vertices()) {
            যদি (GateVarVertex* ধ্রুৱ vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        বাবে (GateLogicVertex* ধ্রুৱ lVtxp : m_toRemove) {
            Ast_গ্ৰন্থি* ধ্রুৱ nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

সার্ৱজনিক:
    স্থির শূন্য apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

শ্ৰেণী GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    শূন্য markRecurse(GateEitherVertex* vtxp) {
        যদি (vtxp->user()) ঘূৰাই;  // Already marked
        vtxp->user(সঁচা);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        বাবে (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* ধ্রুৱ fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    শূন্য mark() {
        m_graph.userClearVertices();
        বাবে (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            যদি (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    স্থির শূন্য warnUnused(ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ nodep) {
        যদি (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) ঘূৰাই;

        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Procedure* ধ্রুৱ procedurep = VN_CAST(nodep, গ্ৰন্থি_Procedure)) {
            যদি (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ধ্রুৱ AstLoop* ধ্রুৱ loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, সঁচা);
                });
        }
    }

    // Remove unused logic
    শূন্য remove() {
        বাবে (V3GraphVertex* ধ্রুৱ vtxp : m_graph.vertices().unlinkable()) {
            যদি (GateLogicVertex* ধ্রুৱ lVtxp = vtxp->cast<GateLogicVertex>()) {
                যদি (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_গ্ৰন্থি* ধ্রুৱ nodep = lVtxp->nodep();
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

সার্ৱজনিক:
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
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
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
