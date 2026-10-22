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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Gate.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3DupFinder.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <list>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

वर्ग GateEitherVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    तार्किक m_reducible = सच्च;  // True if this node should be able to be eliminated
    तार्किक m_dedupable = सच्च;  // True if this node should be able to be deduped
    तार्किक m_consumed = मिच्छ;  // Output goes to something meaningful
    तार्किक m_staticInit = मिच्छ;  // True if this node is a static initializer
सार्वजनिक:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = स्वभाव;

    // ACCESSORS
    तार्किक reducible() ध्रुव { प्रत्यागम m_reducible; }
    तार्किक dedupable() ध्रुव { प्रत्यागम m_dedupable; }
    तार्किक consumed() ध्रुव { प्रत्यागम m_consumed; }
    तार्किक staticInit() ध्रुव { प्रत्यागम m_staticInit; }
    शून्य setConsumed(ध्रुव वर्ण* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = सच्च;
    }
    शून्य setStaticInit() { m_staticInit = सच्च; }
    शून्य clearReducible(ध्रुव वर्ण* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = मिच्छ;
    }
    शून्य clearDedupable(ध्रुव वर्ण* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = मिच्छ;
    }
    शून्य clearReducibleAndDedupable(ध्रुव वर्ण* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ध्रुव override { प्रत्यागम m_consumed ? "" : "dotted"; }
};

वर्ग GateVarVertex final : सार्वजनिक GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* ध्रुव m_varScp;
    तार्किक m_is_शिखरम् = मिच्छ;
    तार्किक m_isClock = मिच्छ;
    Ast_ग्रन्थिः* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_ग्रन्थिः* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
सार्वजनिक:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = स्वभाव;

    // ACCESSORS
    AstVarScope* varScp() ध्रुव VL_MT_STABLE { प्रत्यागम m_varScp; }
    तार्किक is_शिखरम्() ध्रुव { प्रत्यागम m_is_शिखरम्; }
    शून्य set_Is_शिखरम्() { m_is_शिखरम् = सच्च; }
    तार्किक isClock() ध्रुव { प्रत्यागम m_isClock; }
    शून्य setIsClock() { m_isClock = सच्च; }
    Ast_ग्रन्थिः* rstSyncNodep() ध्रुव { प्रत्यागम m_rstSyncNodep; }
    शून्य rstSyncNodep(Ast_ग्रन्थिः* nodep) { m_rstSyncNodep = nodep; }
    Ast_ग्रन्थिः* rstAsyncNodep() ध्रुव { प्रत्यागम m_rstAsyncNodep; }
    शून्य rstAsyncNodep(Ast_ग्रन्थिः* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    शून्य propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        जइ (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() ध्रुव override VL_MT_STABLE { प्रत्यागम varScp()->name(); }
    std::string dotColor() ध्रुव override { प्रत्यागम "green"; }
};

वर्ग GateLogicVertex final : सार्वजनिक GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_ग्रन्थिः* ध्रुव m_nodep;
    AstActive* ध्रुव m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ध्रुव तार्किक m_slow;  // In slow block
सार्वजनिक:
    GateLogicVertex(V3Graph* graphp, Ast_ग्रन्थिः* nodep, AstActive* activep, तार्किक slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = स्वभाव;

    // ACCESSORS
    File_पङ्क्तिः* fileline() ध्रुव override { प्रत्यागम nodep()->fileline(); }
    Ast_ग्रन्थिः* nodep() ध्रुव { प्रत्यागम m_nodep; }
    AstActive* activep() ध्रुव { प्रत्यागम m_activep; }
    तार्किक slow() ध्रुव { प्रत्यागम m_slow; }

    // DOT debug
    std::string name() ध्रुव override { प्रत्यागम m_nodep->fileline()->ascii(); }
    std::string dotColor() ध्रुव override { प्रत्यागम "red"; }
};

वर्ग GateEdge final : सार्वजनिक V3GraphEdge {
    std::string dotLabel() ध्रुव override { प्रत्यागम std::to_string(weight()); }

सार्वजनिक:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शिखरम्, पूर्णाङ्क weight)
        : V3GraphEdge{graphp, fromp, शिखरम्, weight} {}
};

वर्ग GateGraph final : सार्वजनिक V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ध्रुव VNUser1InUse m_inuser1;

सार्वजनिक:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        जइ (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = नूतन GateVarVertex{एतत्, vscp};
            vscp->user1p(vVtxp);
            जइ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            जइ (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            जइ (vscp->varp()->isIO() && vscp->scopep()->is_शिखरम्()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_शिखरम्();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        प्रत्यागम vVtxp;
    }

    शून्य addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, पूर्णाङ्क weight) {
        नूतन GateEdge{एतत्, srcp, dstp, weight};
    }

    शून्य addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, पूर्णाङ्क weight) {
        नूतन GateEdge{एतत्, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

वर्ग GateBuildVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    GateGraph* m_graphp = नूतन GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    ध्रुव Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module
    ध्रुव AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    तार्किक m_inClockedActive = मिच्छ;  // Underneath clocked active
    तार्किक m_inEdgeActive = मिच्छ;  // Underneath edge active
    तार्किक m_inStaticActive = मिच्छ;  // Underneath static active
    तार्किक m_inInitialActive = मिच्छ;  // Underneath initial active
    तार्किक m_inSenItem = मिच्छ;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    शून्य check_ग्रन्थिः(Ast_ग्रन्थिः* nodep) {
        जइ (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        जइ (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    शून्य iterateLogic(Ast_ग्रन्थिः* nodep, तार्किक slow = मिच्छ, ध्रुव वर्ण* nonReducibleReason = nullptr,
                      ध्रुव वर्ण* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = नूतन GateLogicVertex{m_graphp, nodep, m_activep, slow};
        जइ (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } अण्णहा जइ (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } अण्णहा जइ (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        जइ (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_ग्रन्थिः(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    शून्य visit(AstActive* nodep) override {
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
        ध्रुव VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override {
        ध्रुव तार्किक slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    शून्य visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, मिच्छ, "CoverToggle", "CoverToggle");
    }
    शून्य visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = सच्च;
        जइ (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } अण्णहा {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, मिच्छ, nullptr, "senItem");
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        जइ (!m_logicVertexp) प्रत्यागम;

        AstVarScope* ध्रुव vscp = nodep->varScopep();
        GateVarVertex* ध्रुव vVtxp = m_graphp->makeVarVertex(vscp);

        जइ (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(सच्च);
        } अण्णहा जइ (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            जइ (vscp->user2()) {
                जइ (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } अण्णहा {
                जइ (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        जइ (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        जइ (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    शून्य visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), ग्रन्थिः_Assign)
                      && VN_AS(nodep->backp(), ग्रन्थिः_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        जइ (m_logicVertexp) check_ग्रन्थिः(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्थिर std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        प्रत्यागम std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

स्थिर शून्य v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ध्रुव VNUser2InUse user2InUse;
    प्रति (V3GraphVertex& vtx : graph.vertices()) {
        जइ (ध्रुव GateVarVertex* ध्रुव vvertexp = vtx.cast<GateVarVertex>()) {
            ध्रुव AstVarScope* ध्रुव vscp = vvertexp->varScp();
            ध्रुव Ast_ग्रन्थिः* ध्रुव sp = vvertexp->rstSyncNodep();
            ध्रुव Ast_ग्रन्थिः* ध्रुव ap = vvertexp->rstAsyncNodep();
            जइ (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                जइ (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(सच्च);  // Warn only once per signal
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

वर्ग GateConcatVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    ध्रुव AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    पूर्णाङ्क m_offset = 0;  // Current offset of varscope
    पूर्णाङ्क m_found_offset = 0;  // Found offset of varscope
    तार्किक m_found = मिच्छ;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        जइ (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(सच्च);
            m_found_offset = m_offset;
            m_found = सच्च;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    शून्य visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // PUBLIC METHODS
    तार्किक concatOffset(AstConcat* concatp, AstVarScope* vscp, पूर्णाङ्क& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = मिच्छ;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        प्रत्यागम m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

वर्ग GateOkVisitor final : सार्वजनिक VNVisitorConst {
    // RETURN STATE
    तार्किक m_isSimple = सच्च;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_ग्रन्थिः_व्यञ्जकम्* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    ध्रुव तार्किक m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ध्रुव तार्किक m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ध्रुव Ast_ग्रन्थिः_Var_Ref* m_lhsVarRef = nullptr;
    पूर्णाङ्क m_ops = 0;  // Operation count

    // METHODS
    शून्य clearSimple(ध्रुव वर्ण* because) {
        जइ (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = मिच्छ;
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        जइ (!m_isSimple) प्रत्यागम;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        जइ (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        जइ (nodep->access().isRW()) {
            clearSimple("R/W");
            प्रत्यागम;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ध्रुव vscp = nodep->varScopep();
        जइ (nodep->access().isWriteOnly()) {
            जइ (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } अण्णहा {
            // TODO: possible bug, should it be >= 1 as add is below?
            जइ (m_readVscps.size() > 1) {
                जइ (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.प्रक्षिप_back(vscp);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        जइ (!m_isSimple) प्रत्यागम;

        m_substitutionp = nodep->rhsp();
        जइ (!VN_IS(nodep->lhsp(), ग्रन्थिः_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } अण्णहा जइ (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } अण्णहा {
            iterateChildrenConst(nodep);
        }
        जइ (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    शून्य visit(AstCReset* /*nodep*/) override {
        जइ (!m_isSimple) प्रत्यागम;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    शून्य visit(AstMatchMasked* nodep) override {
        जइ (!m_isSimple) प्रत्यागम;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    शून्य visit(Ast_ग्रन्थिः* nodep) override {
        जइ (!m_isSimple) प्रत्यागम;  // Fastpath

        जइ (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            प्रत्यागम;
        }

        जइ (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            प्रत्यागम;
        }

        iterateChildrenConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    GateOkVisitor(Ast_ग्रन्थिः* nodep, तार्किक buffersOnly, तार्किक dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        जइ (!m_substitutionp) {
            clearSimple("No assignment found\n");
            प्रत्यागम;
        }
        जइ (m_isSimple && m_lhsVarRef) {
            प्रति (ध्रुव AstVarScope* ध्रुव vscp : m_readVscps) {
                जइ (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    प्रत्यागम;
                }
            }
        }
    }
    ~GateOkVisitor() override = स्वभाव;
    // PUBLIC METHODS
    तार्किक isSimple() ध्रुव { प्रत्यागम m_isSimple; }
    Ast_ग्रन्थिः_व्यञ्जकम्* substitutionp() ध्रुव {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        प्रत्यागम m_substitutionp;
    }
    ध्रुव std::vector<AstVarScope*>& readVscps() ध्रुव { प्रत्यागम m_readVscps; }
    तार्किक varAssigned(ध्रुव AstVarScope* scopep) ध्रुव {
        प्रत्यागम m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

वर्ग GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_ग्रन्थिः_व्यञ्जकम्*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ध्रुव VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_ग्रन्थिः, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_ग्रन्थिः*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    स्थिर तार्किक isCheap(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // Constant is cheap
        जइ (VN_IS(exprp, Const)) प्रत्यागम सच्च;
        // Variable reference is cheap
        जइ (VN_IS(exprp, ग्रन्थिः_Var_Ref)) प्रत्यागम सच्च;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        जइ (ध्रुव AstSel* ध्रुव selp = VN_CAST(exprp, Sel)) {
            जइ (!isCheap(selp->fromp())) प्रत्यागम मिच्छ;
            जइ (!selp->isWide()) प्रत्यागम सच्च;
            जइ (!VN_IS(selp->lsbp(), Const)) प्रत्यागम मिच्छ;
            जइ (selp->lsbConst() % VL_EDATASIZE != 0) प्रत्यागम मिच्छ;
            प्रत्यागम सच्च;
        }
        // AstArraySel is cheap if the fromp is cheap
        जइ (ध्रुव AstArraySel* ध्रुव aselp = VN_CAST(exprp, ArraySel)) {
            प्रत्यागम isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        प्रत्यागम मिच्छ;
    }

    तार्किक shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_ग्रन्थिः_व्यञ्जकम्* substp, तार्किक allowMultiIn) {
        // Always inline constants
        जइ (VN_IS(substp, Const)) प्रत्यागम सच्च;
        // Don't inline non-constant static initializers - these are scheduled differently
        जइ (lVtxp->staticInit()) प्रत्यागम मिच्छ;
        // Inline simple variable references
        जइ (VN_IS(substp, VarRef)) प्रत्यागम सच्च;
        // Only inline arrays if a simple variable or constant
        जइ (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) प्रत्यागम मिच्छ;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        जइ (nReads == 0) प्रत्यागम सच्च;
        // If it reads one variable, inline if not wide, or if cheap
        जइ (nReads == 1 && (!substp->isWide() || isCheap(substp))) प्रत्यागम सच्च;
        // Don't inline on first round if reads more than one variable
        जइ (nReads > 1 && !allowMultiIn) प्रत्यागम मिच्छ;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        पूर्णाङ्क n = 0;
        प्रति (V3GraphEdge& edge : vVtxp->outEdges()) {
            ध्रुव GateLogicVertex* ध्रुव dstVtxp = edge.शिखरम्()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            जइ (dstVtxp->slow()) अनुवर्त;
            जइ (dstVtxp->outEmpty() && !dstVtxp->consumed()) अनुवर्त;
            n += edge.weight();
            जइ (n > 1) {
                ++m_statNotInlined;
                प्रत्यागम मिच्छ;
            }
        }
        प्रत्यागम सच्च;
    }

    शून्य recordSubstitution(AstVarScope* vscp, Ast_ग्रन्थिः_व्यञ्जकम्* substp, Ast_ग्रन्थिः* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ध्रुव auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        जइ (pair.second) pair.first->second = substp->clone_वृक्षः_Pure(मिच्छ);
    }

    शून्य commitSubstitutions(Ast_ग्रन्थिः* logicp) {
        जइ (!m_hasPending.erase(logicp)) प्रत्यागम;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<ध्रुव AstVarScope*> replaced(substitutions.size() * 2);

        ध्रुव std::function<शून्य(Ast_ग्रन्थिः_Var_Ref*)> visit = [&](Ast_ग्रन्थिः_Var_Ref* nodep) -> शून्य {
            // See if this variable has a substitution
            AstVarScope* ध्रुव vscp = nodep->varScopep();
            ध्रुव auto& it = substitutions.find(vscp);
            जइ (it == substitutions.end()) प्रत्यागम;

            // Do not substitute circular logic
            जइ (!replaced.निवेशय(vscp).second) प्रत्यागम;

            // Substitute nodep with substp
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, ग्रन्थिः_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = substp->clone_वृक्षः_Pure(मिच्छ);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            जइ (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            जइ (Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(newp, ग्रन्थिः_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_ग्रन्थिः* ध्रुव simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        प्रति (ध्रुव auto& pair : substitutions) pair.second->delete_वृक्षः();
        substitutions.clear();
    }

    शून्य optimizeSignals(तार्किक allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        ध्रुव auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            जाव (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            प्रत्यागम it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        जाव (vIt != vertices.end()) {
            GateVarVertex* ध्रुव vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            जइ (!vVtxp->inSize1()) अनुवर्त;

            // Can't inline if non-reducible, etc
            जइ (!vVtxp->reducible()) अनुवर्त;

            // Grab the driving logic
            GateLogicVertex* ध्रुव lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            जइ (!lVtxp->reducible()) अनुवर्त;
            Ast_ग्रन्थिः* ध्रुव logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ध्रुव GateOkVisitor okVisitor{logicp, मिच्छ, मिच्छ};

            // Was it ok?
            जइ (!okVisitor.isSimple()) अनुवर्त;
            // If the varScope is already removed from logicp, no need to try substitution.
            जइ (!okVisitor.varAssigned(vVtxp->varScp())) अनुवर्त;

            // Expression we are considering to substitute with
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ध्रुव size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            जइ (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) अनुवर्त;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ध्रुव vscp = vVtxp->varScp();

            जइ (debug() >= 9) {
                vscp->dump_वृक्षः("substituting: ");
                substp->dump_वृक्षः("        with: ");
            }

            ध्रुव auto& readVscpsVec = okVisitor.readVscps();
            ध्रुव std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            प्रति (V3GraphEdge* ध्रुव edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* ध्रुव dstVtxp = edgep->शिखरम्()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                जइ (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_ग्रन्थिः* nodep = substp;
                    जइ (AstNot* ध्रुव notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    जइ (!VN_IS(nodep, VarRef)) अनुवर्त;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                तार्किक canInline = सच्च;
                प्रति (ध्रुव V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    ध्रुव GateVarVertex* ध्रुव consVVertexp = dedge.शिखरम्()->as<GateVarVertex>();
                    जइ (readVscps.गणनम्(consVVertexp->varScp())) {
                        canInline = मिच्छ;
                        भङ्ग;
                    }
                }

                जइ (!canInline) अनुवर्त;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                जइ (logicp == dstVtxp->nodep()) {
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
                    अनुवर्त;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                प्रति (AstVarScope* ध्रुव newVscp : okVisitor.readVscps()) {
                    GateVarVertex* ध्रुव varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            जइ (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_वृक्षः(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(मिच्छ);
        // Then propagate more complicated equations
        optimizeSignals(सच्च);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_ग्रन्थिः*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](ध्रुव Pair& a, ध्रुव Pair& b) {  //
            प्रत्यागम a.second < b.second;
        });
        प्रति (ध्रुव auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

सार्वजनिक:
    स्थिर शून्य apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

वर्ग GateDedupeHash final : सार्वजनिक V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    संरचना Aux_Ast_ग्रन्थिः_व्यञ्जकम् final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_ग्रन्थिः_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_ग्रन्थिः_व्यञ्जकम्, Aux_Ast_ग्रन्थिः_व्यञ्जकम्> m_aux_ग्रन्थिः_व्यञ्जकम्;

    Ast_ग्रन्थिः_व्यञ्जकम्* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_ग्रन्थिः_व्यञ्जकम् m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    स्थिर तार्किक same(Ast_ग्रन्थिः* node1p, Ast_ग्रन्थिः* node2p) {
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
        प्रत्यागम node1p == node2p || (node1p && node1p->same_वृक्षः(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    तार्किक isSame(Ast_ग्रन्थिः* node1p, Ast_ग्रन्थिः* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ध्रुव auto& aux2 = m_aux_ग्रन्थिः_व्यञ्जकम्(VN_AS(node2p, ग्रन्थिः_व्यञ्जकम्));
        प्रत्यागम m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

सार्वजनिक:
    GateDedupeHash() = स्वभाव;
    ~GateDedupeHash() = स्वभाव;

    ध्रुव Ast_ग्रन्थिः_Assign* hashAndFindDupe(Ast_ग्रन्थिः_Assign* assignp, AstActive* activep,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ध्रुव auto it = m_dupFinder.findDuplicate(m_currRhsp, एतत्);
        जइ (it != m_dupFinder.end()) प्रत्यागम m_aux_ग्रन्थिः_व्यञ्जकम्(VN_AS(it->second, ग्रन्थिः_व्यञ्जकम्)).parentp;

        // Insert new node
        m_dupFinder.निवेशय(m_currRhsp);
        m_aux_ग्रन्थिः_व्यञ्जकम्(m_currRhsp) = m_auxCurRhsp;
        प्रत्यागम nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

वर्ग GateDedupeVarVisitor final : सार्वजनिक VNVisitorConst {
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
    Ast_ग्रन्थिः_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_ग्रन्थिः_व्यञ्जकम्* m_ifCondp = nullptr;  // IF condition that assign is under
    तार्किक m_always = मिच्छ;  // Assign is under an always
    तार्किक m_dedupable = सच्च;  // Determined the assign to be dedupable

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Assign* nodep) override {
        जइ (!m_dedupable) प्रत्यागम;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        जइ (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_व्यञ्जकम्_Stmt*) { प्रत्यागम सच्च; });
            प्रत्यागम;
        }

        m_dedupable = मिच्छ;
    }

    शून्य visit(AstAlways* nodep) override {
        जइ (!m_dedupable) प्रत्यागम;

        जइ (!m_always) {
            m_always = सच्च;
            iterateAndNextConstNull(nodep->stmtsp());
            प्रत्यागम;
        }

        m_dedupable = मिच्छ;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    शून्य visit(Ast_ग्रन्थिः_If* nodep) override {
        जइ (!m_dedupable) प्रत्यागम;

        जइ (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_व्यञ्जकम्_Stmt*) { प्रत्यागम सच्च; });
            iterateAndNextConstNull(nodep->thensp());
            प्रत्यागम;
        }

        m_dedupable = मिच्छ;
    }

    शून्य visit(AstComment*) override {}  // NOP
    //--------------------
    शून्य visit(Ast_ग्रन्थिः*) override { m_dedupable = मिच्छ; }

सार्वजनिक:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = स्वभाव;
    ~GateDedupeVarVisitor() override = स्वभाव;
    // PUBLIC METHODS
    Ast_ग्रन्थिः_Var_Ref* findDupe(Ast_ग्रन्थिः* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = मिच्छ;
        m_dedupable = सच्च;
        iterateConst(logicp);
        जइ (m_dedupable && m_assignp) {
            ध्रुव Ast_ग्रन्थिः* ध्रुव lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव lRefp = VN_CAST(lhsp, ग्रन्थिः_Var_Ref);
            जइ (lRefp && lRefp->varScopep() == consumerVscp) {
                जइ (ध्रुव Ast_ग्रन्थिः_Assign* ध्रुव dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    प्रत्यागम static_cast<Ast_ग्रन्थिः_Var_Ref*>(dup->lhsp());
                }
            }
        }
        प्रत्यागम nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

वर्ग GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ध्रुव VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    शून्य visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        जइ (m_depth > GATE_DEDUP_MAX_DEPTH) प्रत्यागम;

        // Check that we haven't been here before
        जइ (vVtxp->varScp()->user2SetOnce()) प्रत्यागम;

        VL_RESTORER(m_depth);
        ++m_depth;
        जइ (!vVtxp->inSize1()) प्रत्यागम;

        Ast_ग्रन्थिः_Var_Ref* dupRefp = nullptr;
        प्रति (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        जइ (!dupRefp) प्रत्यागम;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* ध्रुव lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        ध्रुव GateOkVisitor okVisitor{lVtxp->nodep(), मिच्छ, सच्च};
        जइ (!okVisitor.isSimple()) प्रत्यागम;

        ++m_statDedupLogic;
        GateVarVertex* ध्रुव dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        प्रति (V3GraphEdge* ध्रुव edgep : vVtxp->outEdges().unlinkable()) {
            ध्रुव GateLogicVertex* ध्रुव consumerVtxp = edgep->शिखरम्()->as<GateLogicVertex>();
            Ast_ग्रन्थिः* ध्रुव consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            जइ (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } अण्णहा {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_ग्रन्थिः_Var_Ref* refp) {
                    जइ (refp->varScopep() != vVtxp->varScp()) प्रत्यागम;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_ग्रन्थिः_Var_Ref* ध्रुव newp = dupRefp->clone_वृक्षः_Pure(मिच्छ);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        जाव (V3GraphEdge* ध्रुव edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_ग्रन्थिः_Var_Ref* visit(GateLogicVertex* lVtxp, ध्रुव GateVarVertex* consumerVtxp) {
        प्रति (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        जइ (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            प्रत्यागम m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        प्रत्यागम nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        प्रति (V3GraphVertex& vtx : graph.vertices()) {
            जइ (GateVarVertex* ध्रुव vVtxp = vtx.cast<GateVarVertex>()) {
                जइ (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        प्रति (V3GraphVertex& vtx : graph.vertices()) {
            जइ (GateVarVertex* ध्रुव vVtxp = vtx.cast<GateVarVertex>()) {
                जइ (vVtxp->is_शिखरम्() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

सार्वजनिक:
    स्थिर शून्य apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

वर्ग GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        ध्रुव AstVarRef* ध्रुव pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ध्रुव cRefp = VN_CAST(currSelp->fromp(), VarRef);
        जइ (!pRefp || !cRefp || !cRefp->same_ग्रन्थिः(pRefp)) प्रत्यागम nullptr;  // not the same var

        ध्रुव AstConst* ध्रुव pstart = VN_CAST(prevSelp->lsbp(), Const);
        ध्रुव AstConst* ध्रुव cstart = VN_CAST(currSelp->lsbp(), Const);
        जइ (!pstart || !cstart) प्रत्यागम nullptr;  // too complicated

        जइ (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            प्रत्यागम नूतन AstSel{cRefp->fileline(), cRefp->clone_वृक्षः(मिच्छ), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } अण्णहा {
            प्रत्यागम nullptr;
        }
    }

    शून्य process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        प्रति (V3GraphEdge* ध्रुव edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* ध्रुव lVtxp = edgep->fromp()->as<GateLogicVertex>();
            जइ (!lVtxp->outSize1()) अनुवर्त;

            AstAlways* ध्रुव alwaysp = VN_CAST(lVtxp->nodep(), Always);
            जइ (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) प्रत्यागम;
            AstAssignW* ध्रुव assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            जइ (!assignp) अनुवर्त;

            जइ (!VN_IS(assignp->lhsp(), Sel)) अनुवर्त;

            // First assign with Sel-lhs, or not under the same active
            जइ (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                अनुवर्त;
            }

            AstSel* ध्रुव prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ध्रुव currSelp = VN_AS(assignp->lhsp(), Sel);

            जइ (AstSel* ध्रुव newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_वृक्षः(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(नूतन AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                जाव (V3GraphEdge* ध्रुव iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* ध्रुव fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } अण्णहा {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        प्रति (V3GraphVertex& vtx : graph.vertices()) {
            जइ (GateVarVertex* ध्रुव vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        प्रति (GateLogicVertex* ध्रुव lVtxp : m_toRemove) {
            Ast_ग्रन्थिः* ध्रुव nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

सार्वजनिक:
    स्थिर शून्य apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

वर्ग GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    शून्य markRecurse(GateEitherVertex* vtxp) {
        जइ (vtxp->user()) प्रत्यागम;  // Already marked
        vtxp->user(सच्च);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        प्रति (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* ध्रुव fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    शून्य mark() {
        m_graph.userClearVertices();
        प्रति (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            जइ (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    स्थिर शून्य warnUnused(ध्रुव Ast_ग्रन्थिः* ध्रुव nodep) {
        जइ (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) प्रत्यागम;

        जइ (ध्रुव Ast_ग्रन्थिः_Procedure* ध्रुव procedurep = VN_CAST(nodep, ग्रन्थिः_Procedure)) {
            जइ (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ध्रुव AstLoop* ध्रुव loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, सच्च);
                });
        }
    }

    // Remove unused logic
    शून्य remove() {
        प्रति (V3GraphVertex* ध्रुव vtxp : m_graph.vertices().unlinkable()) {
            जइ (GateLogicVertex* ध्रुव lVtxp = vtxp->cast<GateLogicVertex>()) {
                जइ (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_ग्रन्थिः* ध्रुव nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
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

सार्वजनिक:
    स्थिर शून्य apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

शून्य V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        जइ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        जइ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        जइ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        जइ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        जइ (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            जइ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        जइ (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            जइ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        जइ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_वृक्षः("gate", 0, dump_वृक्षः_Either_Level() >= 3);
}
