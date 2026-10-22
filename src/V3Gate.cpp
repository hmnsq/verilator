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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Gate.h"

#અન્તર્ભાવ "V3AstUserAllocator.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3DupFinder.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <list>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

વર્ગ GateEitherVertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    તાર્કિક m_reducible = સાચું;  // True if this node should be able to be eliminated
    તાર્કિક m_dedupable = સાચું;  // True if this node should be able to be deduped
    તાર્કિક m_consumed = ખોટું;  // Output goes to something meaningful
    તાર્કિક m_staticInit = ખોટું;  // True if this node is a static initializer
જાહેર:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = મૂળભૂત;

    // ACCESSORS
    તાર્કિક reducible() અચલ { પાછા m_reducible; }
    તાર્કિક dedupable() અચલ { પાછા m_dedupable; }
    તાર્કિક consumed() અચલ { પાછા m_consumed; }
    તાર્કિક staticInit() અચલ { પાછા m_staticInit; }
    શૂન્ય setConsumed(અચલ અક્ષર* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = સાચું;
    }
    શૂન્ય setStaticInit() { m_staticInit = સાચું; }
    શૂન્ય clearReducible(અચલ અક્ષર* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = ખોટું;
    }
    શૂન્ય clearDedupable(અચલ અક્ષર* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = ખોટું;
    }
    શૂન્ય clearReducibleAndDedupable(અચલ અક્ષર* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() અચલ override { પાછા m_consumed ? "" : "dotted"; }
};

વર્ગ GateVarVertex final : જાહેર GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* અચલ m_varScp;
    તાર્કિક m_is_ટોચ = ખોટું;
    તાર્કિક m_isClock = ખોટું;
    Ast_ગાંઠ* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_ગાંઠ* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
જાહેર:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = મૂળભૂત;

    // ACCESSORS
    AstVarScope* varScp() અચલ VL_MT_STABLE { પાછા m_varScp; }
    તાર્કિક is_ટોચ() અચલ { પાછા m_is_ટોચ; }
    શૂન્ય set_Is_ટોચ() { m_is_ટોચ = સાચું; }
    તાર્કિક isClock() અચલ { પાછા m_isClock; }
    શૂન્ય setIsClock() { m_isClock = સાચું; }
    Ast_ગાંઠ* rstSyncNodep() અચલ { પાછા m_rstSyncNodep; }
    શૂન્ય rstSyncNodep(Ast_ગાંઠ* nodep) { m_rstSyncNodep = nodep; }
    Ast_ગાંઠ* rstAsyncNodep() અચલ { પાછા m_rstAsyncNodep; }
    શૂન્ય rstAsyncNodep(Ast_ગાંઠ* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    શૂન્ય propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        જો (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() અચલ override VL_MT_STABLE { પાછા varScp()->name(); }
    std::string dotColor() અચલ override { પાછા "green"; }
};

વર્ગ GateLogicVertex final : જાહેર GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_ગાંઠ* અચલ m_nodep;
    AstActive* અચલ m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    અચલ તાર્કિક m_slow;  // In slow block
જાહેર:
    GateLogicVertex(V3Graph* graphp, Ast_ગાંઠ* nodep, AstActive* activep, તાર્કિક slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = મૂળભૂત;

    // ACCESSORS
    File_લીટી* fileline() અચલ override { પાછા nodep()->fileline(); }
    Ast_ગાંઠ* nodep() અચલ { પાછા m_nodep; }
    AstActive* activep() અચલ { પાછા m_activep; }
    તાર્કિક slow() અચલ { પાછા m_slow; }

    // DOT debug
    std::string name() અચલ override { પાછા m_nodep->fileline()->ascii(); }
    std::string dotColor() અચલ override { પાછા "red"; }
};

વર્ગ GateEdge final : જાહેર V3GraphEdge {
    std::string dotLabel() અચલ override { પાછા std::to_string(weight()); }

જાહેર:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ટોચ, પૂર્ણાંક weight)
        : V3GraphEdge{graphp, fromp, ટોચ, weight} {}
};

વર્ગ GateGraph final : જાહેર V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    અચલ VNUser1InUse m_inuser1;

જાહેર:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        જો (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = નવું GateVarVertex{આ, vscp};
            vscp->user1p(vVtxp);
            જો (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            જો (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            જો (vscp->varp()->isIO() && vscp->scopep()->is_ટોચ()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_ટોચ();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        પાછા vVtxp;
    }

    શૂન્ય addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, પૂર્ણાંક weight) {
        નવું GateEdge{આ, srcp, dstp, weight};
    }

    શૂન્ય addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, પૂર્ણાંક weight) {
        નવું GateEdge{આ, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

વર્ગ GateBuildVisitor final : જાહેર VNVisitorConst {
    // STATE
    GateGraph* m_graphp = નવું GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    અચલ Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    અચલ AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    તાર્કિક m_inClockedActive = ખોટું;  // Underneath clocked active
    તાર્કિક m_inEdgeActive = ખોટું;  // Underneath edge active
    તાર્કિક m_inStaticActive = ખોટું;  // Underneath static active
    તાર્કિક m_inInitialActive = ખોટું;  // Underneath initial active
    તાર્કિક m_inSenItem = ખોટું;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    શૂન્ય check_ગાંઠ(Ast_ગાંઠ* nodep) {
        જો (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        જો (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    શૂન્ય iterateLogic(Ast_ગાંઠ* nodep, તાર્કિક slow = ખોટું, અચલ અક્ષર* nonReducibleReason = nullptr,
                      અચલ અક્ષર* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = નવું GateLogicVertex{m_graphp, nodep, m_activep, slow};
        જો (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } નહીંતર જો (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } નહીંતર જો (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        જો (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_ગાંઠ(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    શૂન્ય visit(AstActive* nodep) override {
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
        અચલ VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    શૂન્ય visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        અચલ તાર્કિક slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    શૂન્ય visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, ખોટું, "CoverToggle", "CoverToggle");
    }
    શૂન્ય visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = સાચું;
        જો (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } નહીંતર {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, ખોટું, nullptr, "senItem");
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (!m_logicVertexp) પાછા;

        AstVarScope* અચલ vscp = nodep->varScopep();
        GateVarVertex* અચલ vVtxp = m_graphp->makeVarVertex(vscp);

        જો (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(સાચું);
        } નહીંતર જો (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            જો (vscp->user2()) {
                જો (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } નહીંતર {
                જો (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        જો (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        જો (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    શૂન્ય visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), ગાંઠ_Assign)
                      && VN_AS(nodep->backp(), ગાંઠ_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (m_logicVertexp) check_ગાંઠ(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

જાહેર:
    સ્થિર std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        પાછા std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

સ્થિર શૂન્ય v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    અચલ VNUser2InUse user2InUse;
    માટે (V3GraphVertex& vtx : graph.vertices()) {
        જો (અચલ GateVarVertex* અચલ vvertexp = vtx.cast<GateVarVertex>()) {
            અચલ AstVarScope* અચલ vscp = vvertexp->varScp();
            અચલ Ast_ગાંઠ* અચલ sp = vvertexp->rstSyncNodep();
            અચલ Ast_ગાંઠ* અચલ ap = vvertexp->rstAsyncNodep();
            જો (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                જો (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(સાચું);  // Warn only once per signal
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

વર્ગ GateConcatVisitor final : જાહેર VNVisitorConst {
    // STATE
    અચલ AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    પૂર્ણાંક m_offset = 0;  // Current offset of varscope
    પૂર્ણાંક m_found_offset = 0;  // Found offset of varscope
    તાર્કિક m_found = ખોટું;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(સાચું);
            m_found_offset = m_offset;
            m_found = સાચું;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    શૂન્ય visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // PUBLIC METHODS
    તાર્કિક concatOffset(AstConcat* concatp, AstVarScope* vscp, પૂર્ણાંક& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = ખોટું;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        પાછા m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

વર્ગ GateOkVisitor final : જાહેર VNVisitorConst {
    // RETURN STATE
    તાર્કિક m_isSimple = સાચું;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_ગાંઠ_પદાવલિ* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    અચલ તાર્કિક m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    અચલ તાર્કિક m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    અચલ Ast_ગાંઠ_Var_Ref* m_lhsVarRef = nullptr;
    પૂર્ણાંક m_ops = 0;  // Operation count

    // METHODS
    શૂન્ય clearSimple(અચલ અક્ષર* because) {
        જો (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = ખોટું;
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Var_Ref* nodep) override {
        જો (!m_isSimple) પાછા;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        જો (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        જો (nodep->access().isRW()) {
            clearSimple("R/W");
            પાછા;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* અચલ vscp = nodep->varScopep();
        જો (nodep->access().isWriteOnly()) {
            જો (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } નહીંતર {
            // TODO: possible bug, should it be >= 1 as add is below?
            જો (m_readVscps.size() > 1) {
                જો (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ધકેલો_back(vscp);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (!m_isSimple) પાછા;

        m_substitutionp = nodep->rhsp();
        જો (!VN_IS(nodep->lhsp(), ગાંઠ_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } નહીંતર જો (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } નહીંતર {
            iterateChildrenConst(nodep);
        }
        જો (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    શૂન્ય visit(AstCReset* /*nodep*/) override {
        જો (!m_isSimple) પાછા;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    શૂન્ય visit(AstMatchMasked* nodep) override {
        જો (!m_isSimple) પાછા;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (!m_isSimple) પાછા;  // Fastpath

        જો (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            પાછા;
        }

        જો (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            પાછા;
        }

        iterateChildrenConst(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    GateOkVisitor(Ast_ગાંઠ* nodep, તાર્કિક buffersOnly, તાર્કિક dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        જો (!m_substitutionp) {
            clearSimple("No assignment found\n");
            પાછા;
        }
        જો (m_isSimple && m_lhsVarRef) {
            માટે (અચલ AstVarScope* અચલ vscp : m_readVscps) {
                જો (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    પાછા;
                }
            }
        }
    }
    ~GateOkVisitor() override = મૂળભૂત;
    // PUBLIC METHODS
    તાર્કિક isSimple() અચલ { પાછા m_isSimple; }
    Ast_ગાંઠ_પદાવલિ* substitutionp() અચલ {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        પાછા m_substitutionp;
    }
    અચલ std::vector<AstVarScope*>& readVscps() અચલ { પાછા m_readVscps; }
    તાર્કિક varAssigned(અચલ AstVarScope* scopep) અચલ {
        પાછા m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

વર્ગ GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_ગાંઠ_પદાવલિ*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    અચલ VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_ગાંઠ, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_ગાંઠ*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    સ્થિર તાર્કિક isCheap(અચલ Ast_ગાંઠ_પદાવલિ* exprp) {
        // Constant is cheap
        જો (VN_IS(exprp, Const)) પાછા સાચું;
        // Variable reference is cheap
        જો (VN_IS(exprp, ગાંઠ_Var_Ref)) પાછા સાચું;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        જો (અચલ AstSel* અચલ selp = VN_CAST(exprp, Sel)) {
            જો (!isCheap(selp->fromp())) પાછા ખોટું;
            જો (!selp->isWide()) પાછા સાચું;
            જો (!VN_IS(selp->lsbp(), Const)) પાછા ખોટું;
            જો (selp->lsbConst() % VL_EDATASIZE != 0) પાછા ખોટું;
            પાછા સાચું;
        }
        // AstArraySel is cheap if the fromp is cheap
        જો (અચલ AstArraySel* અચલ aselp = VN_CAST(exprp, ArraySel)) {
            પાછા isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        પાછા ખોટું;
    }

    તાર્કિક shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_ગાંઠ_પદાવલિ* substp, તાર્કિક allowMultiIn) {
        // Always inline constants
        જો (VN_IS(substp, Const)) પાછા સાચું;
        // Don't inline non-constant static initializers - these are scheduled differently
        જો (lVtxp->staticInit()) પાછા ખોટું;
        // Inline simple variable references
        જો (VN_IS(substp, VarRef)) પાછા સાચું;
        // Only inline arrays if a simple variable or constant
        જો (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) પાછા ખોટું;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        જો (nReads == 0) પાછા સાચું;
        // If it reads one variable, inline if not wide, or if cheap
        જો (nReads == 1 && (!substp->isWide() || isCheap(substp))) પાછા સાચું;
        // Don't inline on first round if reads more than one variable
        જો (nReads > 1 && !allowMultiIn) પાછા ખોટું;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        પૂર્ણાંક n = 0;
        માટે (V3GraphEdge& edge : vVtxp->outEdges()) {
            અચલ GateLogicVertex* અચલ dstVtxp = edge.ટોચ()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            જો (dstVtxp->slow()) ચાલુ;
            જો (dstVtxp->outEmpty() && !dstVtxp->consumed()) ચાલુ;
            n += edge.weight();
            જો (n > 1) {
                ++m_statNotInlined;
                પાછા ખોટું;
            }
        }
        પાછા સાચું;
    }

    શૂન્ય recordSubstitution(AstVarScope* vscp, Ast_ગાંઠ_પદાવલિ* substp, Ast_ગાંઠ* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        અચલ auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        જો (pair.second) pair.first->second = substp->clone_વૃક્ષ_Pure(ખોટું);
    }

    શૂન્ય commitSubstitutions(Ast_ગાંઠ* logicp) {
        જો (!m_hasPending.erase(logicp)) પાછા;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<અચલ AstVarScope*> replaced(substitutions.size() * 2);

        અચલ std::function<શૂન્ય(Ast_ગાંઠ_Var_Ref*)> visit = [&](Ast_ગાંઠ_Var_Ref* nodep) -> શૂન્ય {
            // See if this variable has a substitution
            AstVarScope* અચલ vscp = nodep->varScopep();
            અચલ auto& it = substitutions.find(vscp);
            જો (it == substitutions.end()) પાછા;

            // Do not substitute circular logic
            જો (!replaced.ઉમેરો(vscp).second) પાછા;

            // Substitute nodep with substp
            Ast_ગાંઠ_પદાવલિ* અચલ substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, ગાંઠ_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_ગાંઠ_પદાવલિ* અચલ newp = substp->clone_વૃક્ષ_Pure(ખોટું);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            જો (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            જો (Ast_ગાંઠ_Var_Ref* અચલ varrefp = VN_CAST(newp, ગાંઠ_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_ગાંઠ* અચલ simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        માટે (અચલ auto& pair : substitutions) pair.second->delete_વૃક્ષ();
        substitutions.clear();
    }

    શૂન્ય optimizeSignals(તાર્કિક allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        અચલ auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            જ્યારે (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            પાછા it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        જ્યારે (vIt != vertices.end()) {
            GateVarVertex* અચલ vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            જો (!vVtxp->inSize1()) ચાલુ;

            // Can't inline if non-reducible, etc
            જો (!vVtxp->reducible()) ચાલુ;

            // Grab the driving logic
            GateLogicVertex* અચલ lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            જો (!lVtxp->reducible()) ચાલુ;
            Ast_ગાંઠ* અચલ logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            અચલ GateOkVisitor okVisitor{logicp, ખોટું, ખોટું};

            // Was it ok?
            જો (!okVisitor.isSimple()) ચાલુ;
            // If the varScope is already removed from logicp, no need to try substitution.
            જો (!okVisitor.varAssigned(vVtxp->varScp())) ચાલુ;

            // Expression we are considering to substitute with
            Ast_ગાંઠ_પદાવલિ* અચલ substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            અચલ size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            જો (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) ચાલુ;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* અચલ vscp = vVtxp->varScp();

            જો (debug() >= 9) {
                vscp->dump_વૃક્ષ("substituting: ");
                substp->dump_વૃક્ષ("        with: ");
            }

            અચલ auto& readVscpsVec = okVisitor.readVscps();
            અચલ std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            માટે (V3GraphEdge* અચલ edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* અચલ dstVtxp = edgep->ટોચ()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                જો (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_ગાંઠ* nodep = substp;
                    જો (AstNot* અચલ notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    જો (!VN_IS(nodep, VarRef)) ચાલુ;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                તાર્કિક canInline = સાચું;
                માટે (અચલ V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    અચલ GateVarVertex* અચલ consVVertexp = dedge.ટોચ()->as<GateVarVertex>();
                    જો (readVscps.ગણતરી(consVVertexp->varScp())) {
                        canInline = ખોટું;
                        તોડો;
                    }
                }

                જો (!canInline) ચાલુ;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                જો (logicp == dstVtxp->nodep()) {
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
                    ચાલુ;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                માટે (AstVarScope* અચલ newVscp : okVisitor.readVscps()) {
                    GateVarVertex* અચલ varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            જો (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_વૃક્ષ(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(ખોટું);
        // Then propagate more complicated equations
        optimizeSignals(સાચું);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_ગાંઠ*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](અચલ Pair& a, અચલ Pair& b) {  //
            પાછા a.second < b.second;
        });
        માટે (અચલ auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

જાહેર:
    સ્થિર શૂન્ય apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

વર્ગ GateDedupeHash final : જાહેર V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    સંરચના Aux_Ast_ગાંઠ_પદાવલિ final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_ગાંઠ_પદાવલિ* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_ગાંઠ_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_ગાંઠ_પદાવલિ, Aux_Ast_ગાંઠ_પદાવલિ> m_aux_ગાંઠ_પદાવલિ;

    Ast_ગાંઠ_પદાવલિ* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_ગાંઠ_પદાવલિ m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    સ્થિર તાર્કિક same(Ast_ગાંઠ* node1p, Ast_ગાંઠ* node2p) {
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
        પાછા node1p == node2p || (node1p && node1p->same_વૃક્ષ(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    તાર્કિક isSame(Ast_ગાંઠ* node1p, Ast_ગાંઠ* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        અચલ auto& aux2 = m_aux_ગાંઠ_પદાવલિ(VN_AS(node2p, ગાંઠ_પદાવલિ));
        પાછા m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

જાહેર:
    GateDedupeHash() = મૂળભૂત;
    ~GateDedupeHash() = મૂળભૂત;

    અચલ Ast_ગાંઠ_Assign* hashAndFindDupe(Ast_ગાંઠ_Assign* assignp, AstActive* activep,
                                         Ast_ગાંઠ_પદાવલિ* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        અચલ auto it = m_dupFinder.findDuplicate(m_currRhsp, આ);
        જો (it != m_dupFinder.end()) પાછા m_aux_ગાંઠ_પદાવલિ(VN_AS(it->second, ગાંઠ_પદાવલિ)).parentp;

        // Insert new node
        m_dupFinder.ઉમેરો(m_currRhsp);
        m_aux_ગાંઠ_પદાવલિ(m_currRhsp) = m_auxCurRhsp;
        પાછા nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

વર્ગ GateDedupeVarVisitor final : જાહેર VNVisitorConst {
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
    Ast_ગાંઠ_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_ગાંઠ_પદાવલિ* m_ifCondp = nullptr;  // IF condition that assign is under
    તાર્કિક m_always = ખોટું;  // Assign is under an always
    તાર્કિક m_dedupable = સાચું;  // Determined the assign to be dedupable

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Assign* nodep) override {
        જો (!m_dedupable) પાછા;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        જો (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_પદાવલિ_Stmt*) { પાછા સાચું; });
            પાછા;
        }

        m_dedupable = ખોટું;
    }

    શૂન્ય visit(AstAlways* nodep) override {
        જો (!m_dedupable) પાછા;

        જો (!m_always) {
            m_always = સાચું;
            iterateAndNextConstNull(nodep->stmtsp());
            પાછા;
        }

        m_dedupable = ખોટું;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        જો (!m_dedupable) પાછા;

        જો (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_પદાવલિ_Stmt*) { પાછા સાચું; });
            iterateAndNextConstNull(nodep->thensp());
            પાછા;
        }

        m_dedupable = ખોટું;
    }

    શૂન્ય visit(AstComment*) override {}  // NOP
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ*) override { m_dedupable = ખોટું; }

જાહેર:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = મૂળભૂત;
    ~GateDedupeVarVisitor() override = મૂળભૂત;
    // PUBLIC METHODS
    Ast_ગાંઠ_Var_Ref* findDupe(Ast_ગાંઠ* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = ખોટું;
        m_dedupable = સાચું;
        iterateConst(logicp);
        જો (m_dedupable && m_assignp) {
            અચલ Ast_ગાંઠ* અચલ lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            અચલ Ast_ગાંઠ_Var_Ref* અચલ lRefp = VN_CAST(lhsp, ગાંઠ_Var_Ref);
            જો (lRefp && lRefp->varScopep() == consumerVscp) {
                જો (અચલ Ast_ગાંઠ_Assign* અચલ dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    પાછા static_cast<Ast_ગાંઠ_Var_Ref*>(dup->lhsp());
                }
            }
        }
        પાછા nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

વર્ગ GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    અચલ VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    શૂન્ય visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        જો (m_depth > GATE_DEDUP_MAX_DEPTH) પાછા;

        // Check that we haven't been here before
        જો (vVtxp->varScp()->user2SetOnce()) પાછા;

        VL_RESTORER(m_depth);
        ++m_depth;
        જો (!vVtxp->inSize1()) પાછા;

        Ast_ગાંઠ_Var_Ref* dupRefp = nullptr;
        માટે (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        જો (!dupRefp) પાછા;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* અચલ lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        અચલ GateOkVisitor okVisitor{lVtxp->nodep(), ખોટું, સાચું};
        જો (!okVisitor.isSimple()) પાછા;

        ++m_statDedupLogic;
        GateVarVertex* અચલ dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        માટે (V3GraphEdge* અચલ edgep : vVtxp->outEdges().unlinkable()) {
            અચલ GateLogicVertex* અચલ consumerVtxp = edgep->ટોચ()->as<GateLogicVertex>();
            Ast_ગાંઠ* અચલ consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            જો (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } નહીંતર {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_ગાંઠ_Var_Ref* refp) {
                    જો (refp->varScopep() != vVtxp->varScp()) પાછા;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_ગાંઠ_Var_Ref* અચલ newp = dupRefp->clone_વૃક્ષ_Pure(ખોટું);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_વૃક્ષ(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        જ્યારે (V3GraphEdge* અચલ edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_ગાંઠ_Var_Ref* visit(GateLogicVertex* lVtxp, અચલ GateVarVertex* consumerVtxp) {
        માટે (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        જો (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            પાછા m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        પાછા nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        માટે (V3GraphVertex& vtx : graph.vertices()) {
            જો (GateVarVertex* અચલ vVtxp = vtx.cast<GateVarVertex>()) {
                જો (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        માટે (V3GraphVertex& vtx : graph.vertices()) {
            જો (GateVarVertex* અચલ vVtxp = vtx.cast<GateVarVertex>()) {
                જો (vVtxp->is_ટોચ() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

જાહેર:
    સ્થિર શૂન્ય apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

વર્ગ GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        અચલ AstVarRef* અચલ pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* અચલ cRefp = VN_CAST(currSelp->fromp(), VarRef);
        જો (!pRefp || !cRefp || !cRefp->same_ગાંઠ(pRefp)) પાછા nullptr;  // not the same var

        અચલ AstConst* અચલ pstart = VN_CAST(prevSelp->lsbp(), Const);
        અચલ AstConst* અચલ cstart = VN_CAST(currSelp->lsbp(), Const);
        જો (!pstart || !cstart) પાછા nullptr;  // too complicated

        જો (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            પાછા નવું AstSel{cRefp->fileline(), cRefp->clone_વૃક્ષ(ખોટું), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } નહીંતર {
            પાછા nullptr;
        }
    }

    શૂન્ય process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        માટે (V3GraphEdge* અચલ edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* અચલ lVtxp = edgep->fromp()->as<GateLogicVertex>();
            જો (!lVtxp->outSize1()) ચાલુ;

            AstAlways* અચલ alwaysp = VN_CAST(lVtxp->nodep(), Always);
            જો (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) પાછા;
            AstAssignW* અચલ assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            જો (!assignp) ચાલુ;

            જો (!VN_IS(assignp->lhsp(), Sel)) ચાલુ;

            // First assign with Sel-lhs, or not under the same active
            જો (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                ચાલુ;
            }

            AstSel* અચલ prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* અચલ currSelp = VN_AS(assignp->lhsp(), Sel);

            જો (AstSel* અચલ newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_વૃક્ષ(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(નવું AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                જ્યારે (V3GraphEdge* અચલ iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* અચલ fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } નહીંતર {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        માટે (V3GraphVertex& vtx : graph.vertices()) {
            જો (GateVarVertex* અચલ vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        માટે (GateLogicVertex* અચલ lVtxp : m_toRemove) {
            Ast_ગાંઠ* અચલ nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

જાહેર:
    સ્થિર શૂન્ય apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

વર્ગ GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    શૂન્ય markRecurse(GateEitherVertex* vtxp) {
        જો (vtxp->user()) પાછા;  // Already marked
        vtxp->user(સાચું);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        માટે (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* અચલ fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    શૂન્ય mark() {
        m_graph.userClearVertices();
        માટે (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            જો (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    સ્થિર શૂન્ય warnUnused(અચલ Ast_ગાંઠ* અચલ nodep) {
        જો (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) પાછા;

        જો (અચલ Ast_ગાંઠ_Procedure* અચલ procedurep = VN_CAST(nodep, ગાંઠ_Procedure)) {
            જો (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](અચલ AstLoop* અચલ loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, સાચું);
                });
        }
    }

    // Remove unused logic
    શૂન્ય remove() {
        માટે (V3GraphVertex* અચલ vtxp : m_graph.vertices().unlinkable()) {
            જો (GateLogicVertex* અચલ lVtxp = vtxp->cast<GateLogicVertex>()) {
                જો (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_ગાંઠ* અચલ nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
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

જાહેર:
    સ્થિર શૂન્ય apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

શૂન્ય V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        જો (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        જો (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        જો (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        જો (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        જો (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_વૃક્ષ("gate", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
