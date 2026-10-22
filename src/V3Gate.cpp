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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Gate.h"

#அந்தர்பாவ "V3AstUserAllocator.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3DupFinder.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <list>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

வகுப்பு GateEitherVertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    தருக்க m_reducible = உண்மை;  // True if this node should be able to be eliminated
    தருக்க m_dedupable = உண்மை;  // True if this node should be able to be deduped
    தருக்க m_consumed = பொய்;  // Output goes to something meaningful
    தருக்க m_staticInit = பொய்;  // True if this node is a static initializer
பொது:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = இயல்பு;

    // ACCESSORS
    தருக்க reducible() மாறிலி { திரும்பு m_reducible; }
    தருக்க dedupable() மாறிலி { திரும்பு m_dedupable; }
    தருக்க consumed() மாறிலி { திரும்பு m_consumed; }
    தருக்க staticInit() மாறிலி { திரும்பு m_staticInit; }
    வெறுமை setConsumed(மாறிலி எழுத்து* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = உண்மை;
    }
    வெறுமை setStaticInit() { m_staticInit = உண்மை; }
    வெறுமை clearReducible(மாறிலி எழுத்து* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = பொய்;
    }
    வெறுமை clearDedupable(மாறிலி எழுத்து* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = பொய்;
    }
    வெறுமை clearReducibleAndDedupable(மாறிலி எழுத்து* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() மாறிலி override { திரும்பு m_consumed ? "" : "dotted"; }
};

வகுப்பு GateVarVertex final : பொது GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* மாறிலி m_varScp;
    தருக்க m_is_உச்சி = பொய்;
    தருக்க m_isClock = பொய்;
    Ast_முனை* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_முனை* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
பொது:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = இயல்பு;

    // ACCESSORS
    AstVarScope* varScp() மாறிலி VL_MT_STABLE { திரும்பு m_varScp; }
    தருக்க is_உச்சி() மாறிலி { திரும்பு m_is_உச்சி; }
    வெறுமை set_Is_உச்சி() { m_is_உச்சி = உண்மை; }
    தருக்க isClock() மாறிலி { திரும்பு m_isClock; }
    வெறுமை setIsClock() { m_isClock = உண்மை; }
    Ast_முனை* rstSyncNodep() மாறிலி { திரும்பு m_rstSyncNodep; }
    வெறுமை rstSyncNodep(Ast_முனை* nodep) { m_rstSyncNodep = nodep; }
    Ast_முனை* rstAsyncNodep() மாறிலி { திரும்பு m_rstAsyncNodep; }
    வெறுமை rstAsyncNodep(Ast_முனை* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    வெறுமை propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        எனில் (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() மாறிலி override VL_MT_STABLE { திரும்பு varScp()->name(); }
    std::string dotColor() மாறிலி override { திரும்பு "green"; }
};

வகுப்பு GateLogicVertex final : பொது GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_முனை* மாறிலி m_nodep;
    AstActive* மாறிலி m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    மாறிலி தருக்க m_slow;  // In slow block
பொது:
    GateLogicVertex(V3Graph* graphp, Ast_முனை* nodep, AstActive* activep, தருக்க slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = இயல்பு;

    // ACCESSORS
    File_வரி* fileline() மாறிலி override { திரும்பு nodep()->fileline(); }
    Ast_முனை* nodep() மாறிலி { திரும்பு m_nodep; }
    AstActive* activep() மாறிலி { திரும்பு m_activep; }
    தருக்க slow() மாறிலி { திரும்பு m_slow; }

    // DOT debug
    std::string name() மாறிலி override { திரும்பு m_nodep->fileline()->ascii(); }
    std::string dotColor() மாறிலி override { திரும்பு "red"; }
};

வகுப்பு GateEdge final : பொது V3GraphEdge {
    std::string dotLabel() மாறிலி override { திரும்பு std::to_string(weight()); }

பொது:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight)
        : V3GraphEdge{graphp, fromp, உச்சி, weight} {}
};

வகுப்பு GateGraph final : பொது V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    மாறிலி VNUser1InUse m_inuser1;

பொது:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        எனில் (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = புதிய GateVarVertex{இது, vscp};
            vscp->user1p(vVtxp);
            எனில் (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            எனில் (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            எனில் (vscp->varp()->isIO() && vscp->scopep()->is_உச்சி()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_உச்சி();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        திரும்பு vVtxp;
    }

    வெறுமை addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, முழு weight) {
        புதிய GateEdge{இது, srcp, dstp, weight};
    }

    வெறுமை addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, முழு weight) {
        புதிய GateEdge{இது, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

வகுப்பு GateBuildVisitor final : பொது VNVisitorConst {
    // STATE
    GateGraph* m_graphp = புதிய GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    மாறிலி Ast_முனை_Module* m_modp = nullptr;  // Current module
    மாறிலி AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    தருக்க m_inClockedActive = பொய்;  // Underneath clocked active
    தருக்க m_inEdgeActive = பொய்;  // Underneath edge active
    தருக்க m_inStaticActive = பொய்;  // Underneath static active
    தருக்க m_inInitialActive = பொய்;  // Underneath initial active
    தருக்க m_inSenItem = பொய்;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    வெறுமை check_முனை(Ast_முனை* nodep) {
        எனில் (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        எனில் (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    வெறுமை iterateLogic(Ast_முனை* nodep, தருக்க slow = பொய், மாறிலி எழுத்து* nonReducibleReason = nullptr,
                      மாறிலி எழுத்து* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = புதிய GateLogicVertex{m_graphp, nodep, m_activep, slow};
        எனில் (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } இல்லையெனில் எனில் (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } இல்லையெனில் எனில் (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        எனில் (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_முனை(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    வெறுமை visit(AstActive* nodep) override {
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
        மாறிலி VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    வெறுமை visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {
        மாறிலி தருக்க slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    வெறுமை visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, பொய், "CoverToggle", "CoverToggle");
    }
    வெறுமை visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = உண்மை;
        எனில் (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } இல்லையெனில் {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, பொய், nullptr, "senItem");
        }
    }
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (!m_logicVertexp) திரும்பு;

        AstVarScope* மாறிலி vscp = nodep->varScopep();
        GateVarVertex* மாறிலி vVtxp = m_graphp->makeVarVertex(vscp);

        எனில் (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(உண்மை);
        } இல்லையெனில் எனில் (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            எனில் (vscp->user2()) {
                எனில் (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } இல்லையெனில் {
                எனில் (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        எனில் (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        எனில் (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    வெறுமை visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), முனை_Assign)
                      && VN_AS(nodep->backp(), முனை_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (m_logicVertexp) check_முனை(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

பொது:
    நிலையான std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        திரும்பு std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

நிலையான வெறுமை v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    மாறிலி VNUser2InUse user2InUse;
    க்கு (V3GraphVertex& vtx : graph.vertices()) {
        எனில் (மாறிலி GateVarVertex* மாறிலி vvertexp = vtx.cast<GateVarVertex>()) {
            மாறிலி AstVarScope* மாறிலி vscp = vvertexp->varScp();
            மாறிலி Ast_முனை* மாறிலி sp = vvertexp->rstSyncNodep();
            மாறிலி Ast_முனை* மாறிலி ap = vvertexp->rstAsyncNodep();
            எனில் (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                எனில் (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(உண்மை);  // Warn only once per signal
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

வகுப்பு GateConcatVisitor final : பொது VNVisitorConst {
    // STATE
    மாறிலி AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    முழு m_offset = 0;  // Current offset of varscope
    முழு m_found_offset = 0;  // Found offset of varscope
    தருக்க m_found = பொய்;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(உண்மை);
            m_found_offset = m_offset;
            m_found = உண்மை;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    வெறுமை visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // PUBLIC METHODS
    தருக்க concatOffset(AstConcat* concatp, AstVarScope* vscp, முழு& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = பொய்;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        திரும்பு m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

வகுப்பு GateOkVisitor final : பொது VNVisitorConst {
    // RETURN STATE
    தருக்க m_isSimple = உண்மை;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_முனை_கோவை* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    மாறிலி தருக்க m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    மாறிலி தருக்க m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    மாறிலி Ast_முனை_Var_Ref* m_lhsVarRef = nullptr;
    முழு m_ops = 0;  // Operation count

    // METHODS
    வெறுமை clearSimple(மாறிலி எழுத்து* because) {
        எனில் (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = பொய்;
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Var_Ref* nodep) override {
        எனில் (!m_isSimple) திரும்பு;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        எனில் (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        எனில் (nodep->access().isRW()) {
            clearSimple("R/W");
            திரும்பு;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* மாறிலி vscp = nodep->varScopep();
        எனில் (nodep->access().isWriteOnly()) {
            எனில் (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } இல்லையெனில் {
            // TODO: possible bug, should it be >= 1 as add is below?
            எனில் (m_readVscps.size() > 1) {
                எனில் (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.தள்ளு_back(vscp);
        }
    }
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (!m_isSimple) திரும்பு;

        m_substitutionp = nodep->rhsp();
        எனில் (!VN_IS(nodep->lhsp(), முனை_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } இல்லையெனில் எனில் (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } இல்லையெனில் {
            iterateChildrenConst(nodep);
        }
        எனில் (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    வெறுமை visit(AstCReset* /*nodep*/) override {
        எனில் (!m_isSimple) திரும்பு;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    வெறுமை visit(AstMatchMasked* nodep) override {
        எனில் (!m_isSimple) திரும்பு;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    வெறுமை visit(Ast_முனை* nodep) override {
        எனில் (!m_isSimple) திரும்பு;  // Fastpath

        எனில் (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            திரும்பு;
        }

        எனில் (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            திரும்பு;
        }

        iterateChildrenConst(nodep);
    }

பொது:
    // CONSTRUCTORS
    GateOkVisitor(Ast_முனை* nodep, தருக்க buffersOnly, தருக்க dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        எனில் (!m_substitutionp) {
            clearSimple("No assignment found\n");
            திரும்பு;
        }
        எனில் (m_isSimple && m_lhsVarRef) {
            க்கு (மாறிலி AstVarScope* மாறிலி vscp : m_readVscps) {
                எனில் (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    திரும்பு;
                }
            }
        }
    }
    ~GateOkVisitor() override = இயல்பு;
    // PUBLIC METHODS
    தருக்க isSimple() மாறிலி { திரும்பு m_isSimple; }
    Ast_முனை_கோவை* substitutionp() மாறிலி {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        திரும்பு m_substitutionp;
    }
    மாறிலி std::vector<AstVarScope*>& readVscps() மாறிலி { திரும்பு m_readVscps; }
    தருக்க varAssigned(மாறிலி AstVarScope* scopep) மாறிலி {
        திரும்பு m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

வகுப்பு GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_முனை_கோவை*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    மாறிலி VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_முனை, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_முனை*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    நிலையான தருக்க isCheap(மாறிலி Ast_முனை_கோவை* exprp) {
        // Constant is cheap
        எனில் (VN_IS(exprp, Const)) திரும்பு உண்மை;
        // Variable reference is cheap
        எனில் (VN_IS(exprp, முனை_Var_Ref)) திரும்பு உண்மை;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        எனில் (மாறிலி AstSel* மாறிலி selp = VN_CAST(exprp, Sel)) {
            எனில் (!isCheap(selp->fromp())) திரும்பு பொய்;
            எனில் (!selp->isWide()) திரும்பு உண்மை;
            எனில் (!VN_IS(selp->lsbp(), Const)) திரும்பு பொய்;
            எனில் (selp->lsbConst() % VL_EDATASIZE != 0) திரும்பு பொய்;
            திரும்பு உண்மை;
        }
        // AstArraySel is cheap if the fromp is cheap
        எனில் (மாறிலி AstArraySel* மாறிலி aselp = VN_CAST(exprp, ArraySel)) {
            திரும்பு isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        திரும்பு பொய்;
    }

    தருக்க shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_முனை_கோவை* substp, தருக்க allowMultiIn) {
        // Always inline constants
        எனில் (VN_IS(substp, Const)) திரும்பு உண்மை;
        // Don't inline non-constant static initializers - these are scheduled differently
        எனில் (lVtxp->staticInit()) திரும்பு பொய்;
        // Inline simple variable references
        எனில் (VN_IS(substp, VarRef)) திரும்பு உண்மை;
        // Only inline arrays if a simple variable or constant
        எனில் (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) திரும்பு பொய்;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        எனில் (nReads == 0) திரும்பு உண்மை;
        // If it reads one variable, inline if not wide, or if cheap
        எனில் (nReads == 1 && (!substp->isWide() || isCheap(substp))) திரும்பு உண்மை;
        // Don't inline on first round if reads more than one variable
        எனில் (nReads > 1 && !allowMultiIn) திரும்பு பொய்;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        முழு n = 0;
        க்கு (V3GraphEdge& edge : vVtxp->outEdges()) {
            மாறிலி GateLogicVertex* மாறிலி dstVtxp = edge.உச்சி()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            எனில் (dstVtxp->slow()) தொடர்;
            எனில் (dstVtxp->outEmpty() && !dstVtxp->consumed()) தொடர்;
            n += edge.weight();
            எனில் (n > 1) {
                ++m_statNotInlined;
                திரும்பு பொய்;
            }
        }
        திரும்பு உண்மை;
    }

    வெறுமை recordSubstitution(AstVarScope* vscp, Ast_முனை_கோவை* substp, Ast_முனை* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        மாறிலி auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        எனில் (pair.second) pair.first->second = substp->clone_மரம்_Pure(பொய்);
    }

    வெறுமை commitSubstitutions(Ast_முனை* logicp) {
        எனில் (!m_hasPending.erase(logicp)) திரும்பு;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<மாறிலி AstVarScope*> replaced(substitutions.size() * 2);

        மாறிலி std::function<வெறுமை(Ast_முனை_Var_Ref*)> visit = [&](Ast_முனை_Var_Ref* nodep) -> வெறுமை {
            // See if this variable has a substitution
            AstVarScope* மாறிலி vscp = nodep->varScopep();
            மாறிலி auto& it = substitutions.find(vscp);
            எனில் (it == substitutions.end()) திரும்பு;

            // Do not substitute circular logic
            எனில் (!replaced.செருகு(vscp).second) திரும்பு;

            // Substitute nodep with substp
            Ast_முனை_கோவை* மாறிலி substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, முனை_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_முனை_கோவை* மாறிலி newp = substp->clone_மரம்_Pure(பொய்);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            எனில் (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            எனில் (Ast_முனை_Var_Ref* மாறிலி varrefp = VN_CAST(newp, முனை_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_முனை* மாறிலி simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        க்கு (மாறிலி auto& pair : substitutions) pair.second->delete_மரம்();
        substitutions.clear();
    }

    வெறுமை optimizeSignals(தருக்க allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        மாறிலி auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            போது (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            திரும்பு it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        போது (vIt != vertices.end()) {
            GateVarVertex* மாறிலி vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            எனில் (!vVtxp->inSize1()) தொடர்;

            // Can't inline if non-reducible, etc
            எனில் (!vVtxp->reducible()) தொடர்;

            // Grab the driving logic
            GateLogicVertex* மாறிலி lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            எனில் (!lVtxp->reducible()) தொடர்;
            Ast_முனை* மாறிலி logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            மாறிலி GateOkVisitor okVisitor{logicp, பொய், பொய்};

            // Was it ok?
            எனில் (!okVisitor.isSimple()) தொடர்;
            // If the varScope is already removed from logicp, no need to try substitution.
            எனில் (!okVisitor.varAssigned(vVtxp->varScp())) தொடர்;

            // Expression we are considering to substitute with
            Ast_முனை_கோவை* மாறிலி substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            மாறிலி size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            எனில் (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) தொடர்;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* மாறிலி vscp = vVtxp->varScp();

            எனில் (debug() >= 9) {
                vscp->dump_மரம்("substituting: ");
                substp->dump_மரம்("        with: ");
            }

            மாறிலி auto& readVscpsVec = okVisitor.readVscps();
            மாறிலி std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            க்கு (V3GraphEdge* மாறிலி edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* மாறிலி dstVtxp = edgep->உச்சி()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                எனில் (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_முனை* nodep = substp;
                    எனில் (AstNot* மாறிலி notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    எனில் (!VN_IS(nodep, VarRef)) தொடர்;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                தருக்க canInline = உண்மை;
                க்கு (மாறிலி V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    மாறிலி GateVarVertex* மாறிலி consVVertexp = dedge.உச்சி()->as<GateVarVertex>();
                    எனில் (readVscps.மொத்தம்(consVVertexp->varScp())) {
                        canInline = பொய்;
                        நிறுத்து;
                    }
                }

                எனில் (!canInline) தொடர்;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                எனில் (logicp == dstVtxp->nodep()) {
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
                    தொடர்;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                க்கு (AstVarScope* மாறிலி newVscp : okVisitor.readVscps()) {
                    GateVarVertex* மாறிலி varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            எனில் (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_மரம்(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(பொய்);
        // Then propagate more complicated equations
        optimizeSignals(உண்மை);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_முனை*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](மாறிலி Pair& a, மாறிலி Pair& b) {  //
            திரும்பு a.second < b.second;
        });
        க்கு (மாறிலி auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

பொது:
    நிலையான வெறுமை apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

வகுப்பு GateDedupeHash final : பொது V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    கட்டமைப்பு Aux_Ast_முனை_கோவை final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_முனை_கோவை* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_முனை_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_முனை_கோவை, Aux_Ast_முனை_கோவை> m_aux_முனை_கோவை;

    Ast_முனை_கோவை* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_முனை_கோவை m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    நிலையான தருக்க same(Ast_முனை* node1p, Ast_முனை* node2p) {
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
        திரும்பு node1p == node2p || (node1p && node1p->same_மரம்(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    தருக்க isSame(Ast_முனை* node1p, Ast_முனை* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        மாறிலி auto& aux2 = m_aux_முனை_கோவை(VN_AS(node2p, முனை_கோவை));
        திரும்பு m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

பொது:
    GateDedupeHash() = இயல்பு;
    ~GateDedupeHash() = இயல்பு;

    மாறிலி Ast_முனை_Assign* hashAndFindDupe(Ast_முனை_Assign* assignp, AstActive* activep,
                                         Ast_முனை_கோவை* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        மாறிலி auto it = m_dupFinder.findDuplicate(m_currRhsp, இது);
        எனில் (it != m_dupFinder.end()) திரும்பு m_aux_முனை_கோவை(VN_AS(it->second, முனை_கோவை)).parentp;

        // Insert new node
        m_dupFinder.செருகு(m_currRhsp);
        m_aux_முனை_கோவை(m_currRhsp) = m_auxCurRhsp;
        திரும்பு nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

வகுப்பு GateDedupeVarVisitor final : பொது VNVisitorConst {
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
    Ast_முனை_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_முனை_கோவை* m_ifCondp = nullptr;  // IF condition that assign is under
    தருக்க m_always = பொய்;  // Assign is under an always
    தருக்க m_dedupable = உண்மை;  // Determined the assign to be dedupable

    // VISITORS
    வெறுமை visit(Ast_முனை_Assign* nodep) override {
        எனில் (!m_dedupable) திரும்பு;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        எனில் (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_கோவை_Stmt*) { திரும்பு உண்மை; });
            திரும்பு;
        }

        m_dedupable = பொய்;
    }

    வெறுமை visit(AstAlways* nodep) override {
        எனில் (!m_dedupable) திரும்பு;

        எனில் (!m_always) {
            m_always = உண்மை;
            iterateAndNextConstNull(nodep->stmtsp());
            திரும்பு;
        }

        m_dedupable = பொய்;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    வெறுமை visit(Ast_முனை_If* nodep) override {
        எனில் (!m_dedupable) திரும்பு;

        எனில் (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_கோவை_Stmt*) { திரும்பு உண்மை; });
            iterateAndNextConstNull(nodep->thensp());
            திரும்பு;
        }

        m_dedupable = பொய்;
    }

    வெறுமை visit(AstComment*) override {}  // NOP
    //--------------------
    வெறுமை visit(Ast_முனை*) override { m_dedupable = பொய்; }

பொது:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = இயல்பு;
    ~GateDedupeVarVisitor() override = இயல்பு;
    // PUBLIC METHODS
    Ast_முனை_Var_Ref* findDupe(Ast_முனை* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = பொய்;
        m_dedupable = உண்மை;
        iterateConst(logicp);
        எனில் (m_dedupable && m_assignp) {
            மாறிலி Ast_முனை* மாறிலி lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            மாறிலி Ast_முனை_Var_Ref* மாறிலி lRefp = VN_CAST(lhsp, முனை_Var_Ref);
            எனில் (lRefp && lRefp->varScopep() == consumerVscp) {
                எனில் (மாறிலி Ast_முனை_Assign* மாறிலி dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    திரும்பு static_cast<Ast_முனை_Var_Ref*>(dup->lhsp());
                }
            }
        }
        திரும்பு nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

வகுப்பு GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    மாறிலி VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    வெறுமை visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        எனில் (m_depth > GATE_DEDUP_MAX_DEPTH) திரும்பு;

        // Check that we haven't been here before
        எனில் (vVtxp->varScp()->user2SetOnce()) திரும்பு;

        VL_RESTORER(m_depth);
        ++m_depth;
        எனில் (!vVtxp->inSize1()) திரும்பு;

        Ast_முனை_Var_Ref* dupRefp = nullptr;
        க்கு (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        எனில் (!dupRefp) திரும்பு;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* மாறிலி lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        மாறிலி GateOkVisitor okVisitor{lVtxp->nodep(), பொய், உண்மை};
        எனில் (!okVisitor.isSimple()) திரும்பு;

        ++m_statDedupLogic;
        GateVarVertex* மாறிலி dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        க்கு (V3GraphEdge* மாறிலி edgep : vVtxp->outEdges().unlinkable()) {
            மாறிலி GateLogicVertex* மாறிலி consumerVtxp = edgep->உச்சி()->as<GateLogicVertex>();
            Ast_முனை* மாறிலி consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            எனில் (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } இல்லையெனில் {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_முனை_Var_Ref* refp) {
                    எனில் (refp->varScopep() != vVtxp->varScp()) திரும்பு;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_முனை_Var_Ref* மாறிலி newp = dupRefp->clone_மரம்_Pure(பொய்);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_மரம்(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        போது (V3GraphEdge* மாறிலி edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_முனை_Var_Ref* visit(GateLogicVertex* lVtxp, மாறிலி GateVarVertex* consumerVtxp) {
        க்கு (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        எனில் (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            திரும்பு m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        திரும்பு nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        க்கு (V3GraphVertex& vtx : graph.vertices()) {
            எனில் (GateVarVertex* மாறிலி vVtxp = vtx.cast<GateVarVertex>()) {
                எனில் (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        க்கு (V3GraphVertex& vtx : graph.vertices()) {
            எனில் (GateVarVertex* மாறிலி vVtxp = vtx.cast<GateVarVertex>()) {
                எனில் (vVtxp->is_உச்சி() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

பொது:
    நிலையான வெறுமை apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

வகுப்பு GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        மாறிலி AstVarRef* மாறிலி pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* மாறிலி cRefp = VN_CAST(currSelp->fromp(), VarRef);
        எனில் (!pRefp || !cRefp || !cRefp->same_முனை(pRefp)) திரும்பு nullptr;  // not the same var

        மாறிலி AstConst* மாறிலி pstart = VN_CAST(prevSelp->lsbp(), Const);
        மாறிலி AstConst* மாறிலி cstart = VN_CAST(currSelp->lsbp(), Const);
        எனில் (!pstart || !cstart) திரும்பு nullptr;  // too complicated

        எனில் (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            திரும்பு புதிய AstSel{cRefp->fileline(), cRefp->clone_மரம்(பொய்), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } இல்லையெனில் {
            திரும்பு nullptr;
        }
    }

    வெறுமை process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        க்கு (V3GraphEdge* மாறிலி edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* மாறிலி lVtxp = edgep->fromp()->as<GateLogicVertex>();
            எனில் (!lVtxp->outSize1()) தொடர்;

            AstAlways* மாறிலி alwaysp = VN_CAST(lVtxp->nodep(), Always);
            எனில் (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) திரும்பு;
            AstAssignW* மாறிலி assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            எனில் (!assignp) தொடர்;

            எனில் (!VN_IS(assignp->lhsp(), Sel)) தொடர்;

            // First assign with Sel-lhs, or not under the same active
            எனில் (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                தொடர்;
            }

            AstSel* மாறிலி prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* மாறிலி currSelp = VN_AS(assignp->lhsp(), Sel);

            எனில் (AstSel* மாறிலி newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_மரம்(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(புதிய AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                போது (V3GraphEdge* மாறிலி iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* மாறிலி fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } இல்லையெனில் {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        க்கு (V3GraphVertex& vtx : graph.vertices()) {
            எனில் (GateVarVertex* மாறிலி vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        க்கு (GateLogicVertex* மாறிலி lVtxp : m_toRemove) {
            Ast_முனை* மாறிலி nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

பொது:
    நிலையான வெறுமை apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

வகுப்பு GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    வெறுமை markRecurse(GateEitherVertex* vtxp) {
        எனில் (vtxp->user()) திரும்பு;  // Already marked
        vtxp->user(உண்மை);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        க்கு (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* மாறிலி fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    வெறுமை mark() {
        m_graph.userClearVertices();
        க்கு (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            எனில் (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    நிலையான வெறுமை warnUnused(மாறிலி Ast_முனை* மாறிலி nodep) {
        எனில் (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) திரும்பு;

        எனில் (மாறிலி Ast_முனை_Procedure* மாறிலி procedurep = VN_CAST(nodep, முனை_Procedure)) {
            எனில் (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](மாறிலி AstLoop* மாறிலி loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, உண்மை);
                });
        }
    }

    // Remove unused logic
    வெறுமை remove() {
        க்கு (V3GraphVertex* மாறிலி vtxp : m_graph.vertices().unlinkable()) {
            எனில் (GateLogicVertex* மாறிலி lVtxp = vtxp->cast<GateLogicVertex>()) {
                எனில் (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_முனை* மாறிலி nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
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

பொது:
    நிலையான வெறுமை apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

வெறுமை V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        எனில் (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        எனில் (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        எனில் (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        எனில் (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        எனில் (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_மரம்("gate", 0, dump_மரம்_Either_Level() >= 3);
}
