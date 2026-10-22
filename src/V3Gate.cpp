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
    तार्किक m_reducible = सत्य;  // True if this node should be able to be eliminated
    तार्किक m_dedupable = सत्य;  // True if this node should be able to be deduped
    तार्किक m_consumed = असत्य;  // Output goes to something meaningful
    तार्किक m_staticInit = असत्य;  // True if this node is a static initializer
सार्वजनिक:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = पूर्वनिर्धारित;

    // ACCESSORS
    तार्किक reducible() स्थिरांक { परत m_reducible; }
    तार्किक dedupable() स्थिरांक { परत m_dedupable; }
    तार्किक consumed() स्थिरांक { परत m_consumed; }
    तार्किक staticInit() स्थिरांक { परत m_staticInit; }
    शून्य setConsumed(स्थिरांक अक्षर* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = सत्य;
    }
    शून्य setStaticInit() { m_staticInit = सत्य; }
    शून्य clearReducible(स्थिरांक अक्षर* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = असत्य;
    }
    शून्य clearDedupable(स्थिरांक अक्षर* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = असत्य;
    }
    शून्य clearReducibleAndDedupable(स्थिरांक अक्षर* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() स्थिरांक override { परत m_consumed ? "" : "dotted"; }
};

वर्ग GateVarVertex final : सार्वजनिक GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* स्थिरांक m_varScp;
    तार्किक m_is_शीर्ष = असत्य;
    तार्किक m_isClock = असत्य;
    Ast_गाठ* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_गाठ* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
सार्वजनिक:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = पूर्वनिर्धारित;

    // ACCESSORS
    AstVarScope* varScp() स्थिरांक VL_MT_STABLE { परत m_varScp; }
    तार्किक is_शीर्ष() स्थिरांक { परत m_is_शीर्ष; }
    शून्य set_Is_शीर्ष() { m_is_शीर्ष = सत्य; }
    तार्किक isClock() स्थिरांक { परत m_isClock; }
    शून्य setIsClock() { m_isClock = सत्य; }
    Ast_गाठ* rstSyncNodep() स्थिरांक { परत m_rstSyncNodep; }
    शून्य rstSyncNodep(Ast_गाठ* nodep) { m_rstSyncNodep = nodep; }
    Ast_गाठ* rstAsyncNodep() स्थिरांक { परत m_rstAsyncNodep; }
    शून्य rstAsyncNodep(Ast_गाठ* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    शून्य propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        जर (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() स्थिरांक override VL_MT_STABLE { परत varScp()->name(); }
    std::string dotColor() स्थिरांक override { परत "green"; }
};

वर्ग GateLogicVertex final : सार्वजनिक GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_गाठ* स्थिरांक m_nodep;
    AstActive* स्थिरांक m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    स्थिरांक तार्किक m_slow;  // In slow block
सार्वजनिक:
    GateLogicVertex(V3Graph* graphp, Ast_गाठ* nodep, AstActive* activep, तार्किक slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = पूर्वनिर्धारित;

    // ACCESSORS
    File_ओळ* fileline() स्थिरांक override { परत nodep()->fileline(); }
    Ast_गाठ* nodep() स्थिरांक { परत m_nodep; }
    AstActive* activep() स्थिरांक { परत m_activep; }
    तार्किक slow() स्थिरांक { परत m_slow; }

    // DOT debug
    std::string name() स्थिरांक override { परत m_nodep->fileline()->ascii(); }
    std::string dotColor() स्थिरांक override { परत "red"; }
};

वर्ग GateEdge final : सार्वजनिक V3GraphEdge {
    std::string dotLabel() स्थिरांक override { परत std::to_string(weight()); }

सार्वजनिक:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight)
        : V3GraphEdge{graphp, fromp, शीर्ष, weight} {}
};

वर्ग GateGraph final : सार्वजनिक V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    स्थिरांक VNUser1InUse m_inuser1;

सार्वजनिक:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        जर (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = नवीन GateVarVertex{हे, vscp};
            vscp->user1p(vVtxp);
            जर (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            जर (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            जर (vscp->varp()->isIO() && vscp->scopep()->is_शीर्ष()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_शीर्ष();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        परत vVtxp;
    }

    शून्य addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, पूर्णांक weight) {
        नवीन GateEdge{हे, srcp, dstp, weight};
    }

    शून्य addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, पूर्णांक weight) {
        नवीन GateEdge{हे, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

वर्ग GateBuildVisitor final : सार्वजनिक VNVisitorConst {
    // STATE
    GateGraph* m_graphp = नवीन GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    स्थिरांक Ast_गाठ_Module* m_modp = nullptr;  // Current module
    स्थिरांक AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    तार्किक m_inClockedActive = असत्य;  // Underneath clocked active
    तार्किक m_inEdgeActive = असत्य;  // Underneath edge active
    तार्किक m_inStaticActive = असत्य;  // Underneath static active
    तार्किक m_inInitialActive = असत्य;  // Underneath initial active
    तार्किक m_inSenItem = असत्य;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    शून्य check_गाठ(Ast_गाठ* nodep) {
        जर (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        जर (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    शून्य iterateLogic(Ast_गाठ* nodep, तार्किक slow = असत्य, स्थिरांक अक्षर* nonReducibleReason = nullptr,
                      स्थिरांक अक्षर* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = नवीन GateLogicVertex{m_graphp, nodep, m_activep, slow};
        जर (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } अन्यथा जर (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } अन्यथा जर (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        जर (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_गाठ(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Module* nodep) override {
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
        स्थिरांक VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    शून्य visit(Ast_गाठ_Procedure* nodep) override {
        स्थिरांक तार्किक slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    शून्य visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, असत्य, "CoverToggle", "CoverToggle");
    }
    शून्य visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = सत्य;
        जर (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } अन्यथा {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, असत्य, nullptr, "senItem");
        }
    }
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        जर (!m_logicVertexp) परत;

        AstVarScope* स्थिरांक vscp = nodep->varScopep();
        GateVarVertex* स्थिरांक vVtxp = m_graphp->makeVarVertex(vscp);

        जर (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(सत्य);
        } अन्यथा जर (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            जर (vscp->user2()) {
                जर (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } अन्यथा {
                जर (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        जर (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        जर (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    शून्य visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), गाठ_Assign)
                      && VN_AS(nodep->backp(), गाठ_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    शून्य visit(Ast_गाठ* nodep) override {
        जर (m_logicVertexp) check_गाठ(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्थिर std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        परत std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

स्थिर शून्य v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    स्थिरांक VNUser2InUse user2InUse;
    साठी (V3GraphVertex& vtx : graph.vertices()) {
        जर (स्थिरांक GateVarVertex* स्थिरांक vvertexp = vtx.cast<GateVarVertex>()) {
            स्थिरांक AstVarScope* स्थिरांक vscp = vvertexp->varScp();
            स्थिरांक Ast_गाठ* स्थिरांक sp = vvertexp->rstSyncNodep();
            स्थिरांक Ast_गाठ* स्थिरांक ap = vvertexp->rstAsyncNodep();
            जर (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                जर (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(सत्य);  // Warn only once per signal
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
    स्थिरांक AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    पूर्णांक m_offset = 0;  // Current offset of varscope
    पूर्णांक m_found_offset = 0;  // Found offset of varscope
    तार्किक m_found = असत्य;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        जर (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(सत्य);
            m_found_offset = m_offset;
            m_found = सत्य;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    शून्य visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // PUBLIC METHODS
    तार्किक concatOffset(AstConcat* concatp, AstVarScope* vscp, पूर्णांक& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = असत्य;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        परत m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

वर्ग GateOkVisitor final : सार्वजनिक VNVisitorConst {
    // RETURN STATE
    तार्किक m_isSimple = सत्य;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_गाठ_अभिव्यक्ती* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    स्थिरांक तार्किक m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    स्थिरांक तार्किक m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    स्थिरांक Ast_गाठ_Var_Ref* m_lhsVarRef = nullptr;
    पूर्णांक m_ops = 0;  // Operation count

    // METHODS
    शून्य clearSimple(स्थिरांक अक्षर* because) {
        जर (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = असत्य;
    }

    // VISITORS
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        जर (!m_isSimple) परत;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        जर (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        जर (nodep->access().isRW()) {
            clearSimple("R/W");
            परत;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* स्थिरांक vscp = nodep->varScopep();
        जर (nodep->access().isWriteOnly()) {
            जर (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } अन्यथा {
            // TODO: possible bug, should it be >= 1 as add is below?
            जर (m_readVscps.size() > 1) {
                जर (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ढकला_back(vscp);
        }
    }
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (!m_isSimple) परत;

        m_substitutionp = nodep->rhsp();
        जर (!VN_IS(nodep->lhsp(), गाठ_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } अन्यथा जर (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
        जर (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    शून्य visit(AstCReset* /*nodep*/) override {
        जर (!m_isSimple) परत;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    शून्य visit(AstMatchMasked* nodep) override {
        जर (!m_isSimple) परत;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    शून्य visit(Ast_गाठ* nodep) override {
        जर (!m_isSimple) परत;  // Fastpath

        जर (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            परत;
        }

        जर (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            परत;
        }

        iterateChildrenConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    GateOkVisitor(Ast_गाठ* nodep, तार्किक buffersOnly, तार्किक dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        जर (!m_substitutionp) {
            clearSimple("No assignment found\n");
            परत;
        }
        जर (m_isSimple && m_lhsVarRef) {
            साठी (स्थिरांक AstVarScope* स्थिरांक vscp : m_readVscps) {
                जर (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    परत;
                }
            }
        }
    }
    ~GateOkVisitor() override = पूर्वनिर्धारित;
    // PUBLIC METHODS
    तार्किक isSimple() स्थिरांक { परत m_isSimple; }
    Ast_गाठ_अभिव्यक्ती* substitutionp() स्थिरांक {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        परत m_substitutionp;
    }
    स्थिरांक std::vector<AstVarScope*>& readVscps() स्थिरांक { परत m_readVscps; }
    तार्किक varAssigned(स्थिरांक AstVarScope* scopep) स्थिरांक {
        परत m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

वर्ग GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_गाठ_अभिव्यक्ती*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    स्थिरांक VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_गाठ, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    माप_प्रकार m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_गाठ*, माप_प्रकार> m_hasPending;
    माप_प्रकार m_statInlined = 0;  // Statistic tracking - signals inlined
    माप_प्रकार m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    माप_प्रकार m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    स्थिर तार्किक isCheap(स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) {
        // Constant is cheap
        जर (VN_IS(exprp, Const)) परत सत्य;
        // Variable reference is cheap
        जर (VN_IS(exprp, गाठ_Var_Ref)) परत सत्य;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(exprp, Sel)) {
            जर (!isCheap(selp->fromp())) परत असत्य;
            जर (!selp->isWide()) परत सत्य;
            जर (!VN_IS(selp->lsbp(), Const)) परत असत्य;
            जर (selp->lsbConst() % VL_EDATASIZE != 0) परत असत्य;
            परत सत्य;
        }
        // AstArraySel is cheap if the fromp is cheap
        जर (स्थिरांक AstArraySel* स्थिरांक aselp = VN_CAST(exprp, ArraySel)) {
            परत isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        परत असत्य;
    }

    तार्किक shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, माप_प्रकार nReads,
                      Ast_गाठ_अभिव्यक्ती* substp, तार्किक allowMultiIn) {
        // Always inline constants
        जर (VN_IS(substp, Const)) परत सत्य;
        // Don't inline non-constant static initializers - these are scheduled differently
        जर (lVtxp->staticInit()) परत असत्य;
        // Inline simple variable references
        जर (VN_IS(substp, VarRef)) परत सत्य;
        // Only inline arrays if a simple variable or constant
        जर (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) परत असत्य;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        जर (nReads == 0) परत सत्य;
        // If it reads one variable, inline if not wide, or if cheap
        जर (nReads == 1 && (!substp->isWide() || isCheap(substp))) परत सत्य;
        // Don't inline on first round if reads more than one variable
        जर (nReads > 1 && !allowMultiIn) परत असत्य;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        पूर्णांक n = 0;
        साठी (V3GraphEdge& edge : vVtxp->outEdges()) {
            स्थिरांक GateLogicVertex* स्थिरांक dstVtxp = edge.शीर्ष()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            जर (dstVtxp->slow()) सुरूठेवा;
            जर (dstVtxp->outEmpty() && !dstVtxp->consumed()) सुरूठेवा;
            n += edge.weight();
            जर (n > 1) {
                ++m_statNotInlined;
                परत असत्य;
            }
        }
        परत सत्य;
    }

    शून्य recordSubstitution(AstVarScope* vscp, Ast_गाठ_अभिव्यक्ती* substp, Ast_गाठ* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        स्थिरांक स्वतः pair = m_substitutions(logicp).emplace(vscp, nullptr);
        जर (pair.second) pair.first->second = substp->clone_वृक्ष_Pure(असत्य);
    }

    शून्य commitSubstitutions(Ast_गाठ* logicp) {
        जर (!m_hasPending.erase(logicp)) परत;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<स्थिरांक AstVarScope*> replaced(substitutions.size() * 2);

        स्थिरांक std::function<शून्य(Ast_गाठ_Var_Ref*)> visit = [&](Ast_गाठ_Var_Ref* nodep) -> शून्य {
            // See if this variable has a substitution
            AstVarScope* स्थिरांक vscp = nodep->varScopep();
            स्थिरांक स्वतः& it = substitutions.find(vscp);
            जर (it == substitutions.end()) परत;

            // Do not substitute circular logic
            जर (!replaced.घाला(vscp).second) परत;

            // Substitute nodep with substp
            Ast_गाठ_अभिव्यक्ती* स्थिरांक substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, गाठ_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = substp->clone_वृक्ष_Pure(असत्य);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            जर (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            जर (Ast_गाठ_Var_Ref* स्थिरांक varrefp = VN_CAST(newp, गाठ_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_गाठ* स्थिरांक simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        साठी (स्थिरांक स्वतः& pair : substitutions) pair.second->delete_वृक्ष();
        substitutions.clear();
    }

    शून्य optimizeSignals(तार्किक allowMultiIn) {
        // Consider "inlining" variables
        स्वतः& vertices = m_graph.vertices();
        स्थिरांक स्वतः ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            पर्यंत (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            परत it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        पर्यंत (vIt != vertices.end()) {
            GateVarVertex* स्थिरांक vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            जर (!vVtxp->inSize1()) सुरूठेवा;

            // Can't inline if non-reducible, etc
            जर (!vVtxp->reducible()) सुरूठेवा;

            // Grab the driving logic
            GateLogicVertex* स्थिरांक lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            जर (!lVtxp->reducible()) सुरूठेवा;
            Ast_गाठ* स्थिरांक logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            स्थिरांक GateOkVisitor okVisitor{logicp, असत्य, असत्य};

            // Was it ok?
            जर (!okVisitor.isSimple()) सुरूठेवा;
            // If the varScope is already removed from logicp, no need to try substitution.
            जर (!okVisitor.varAssigned(vVtxp->varScp())) सुरूठेवा;

            // Expression we are considering to substitute with
            Ast_गाठ_अभिव्यक्ती* स्थिरांक substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            स्थिरांक माप_प्रकार nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            जर (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) सुरूठेवा;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* स्थिरांक vscp = vVtxp->varScp();

            जर (debug() >= 9) {
                vscp->dump_वृक्ष("substituting: ");
                substp->dump_वृक्ष("        with: ");
            }

            स्थिरांक स्वतः& readVscpsVec = okVisitor.readVscps();
            स्थिरांक std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            साठी (V3GraphEdge* स्थिरांक edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* स्थिरांक dstVtxp = edgep->शीर्ष()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                जर (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_गाठ* nodep = substp;
                    जर (AstNot* स्थिरांक notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    जर (!VN_IS(nodep, VarRef)) सुरूठेवा;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                तार्किक canInline = सत्य;
                साठी (स्थिरांक V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    स्थिरांक GateVarVertex* स्थिरांक consVVertexp = dedge.शीर्ष()->as<GateVarVertex>();
                    जर (readVscps.मोजणी(consVVertexp->varScp())) {
                        canInline = असत्य;
                        थांबा;
                    }
                }

                जर (!canInline) सुरूठेवा;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                जर (logicp == dstVtxp->nodep()) {
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
                    सुरूठेवा;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                साठी (AstVarScope* स्थिरांक newVscp : okVisitor.readVscps()) {
                    GateVarVertex* स्थिरांक varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            जर (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_वृक्ष(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(असत्य);
        // Then propagate more complicated equations
        optimizeSignals(सत्य);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_गाठ*, माप_प्रकार>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](स्थिरांक Pair& a, स्थिरांक Pair& b) {  //
            परत a.second < b.second;
        });
        साठी (स्थिरांक स्वतः& pair : pending) commitSubstitutions(pair.first);
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
    संरचना Aux_Ast_गाठ_अभिव्यक्ती final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_गाठ_अभिव्यक्ती* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_गाठ_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_गाठ_अभिव्यक्ती, Aux_Ast_गाठ_अभिव्यक्ती> m_aux_गाठ_अभिव्यक्ती;

    Ast_गाठ_अभिव्यक्ती* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_गाठ_अभिव्यक्ती m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    स्थिर तार्किक same(Ast_गाठ* node1p, Ast_गाठ* node2p) {
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
        परत node1p == node2p || (node1p && node1p->same_वृक्ष(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    तार्किक isSame(Ast_गाठ* node1p, Ast_गाठ* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        स्थिरांक स्वतः& aux2 = m_aux_गाठ_अभिव्यक्ती(VN_AS(node2p, गाठ_अभिव्यक्ती));
        परत m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

सार्वजनिक:
    GateDedupeHash() = पूर्वनिर्धारित;
    ~GateDedupeHash() = पूर्वनिर्धारित;

    स्थिरांक Ast_गाठ_Assign* hashAndFindDupe(Ast_गाठ_Assign* assignp, AstActive* activep,
                                         Ast_गाठ_अभिव्यक्ती* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        स्थिरांक स्वतः it = m_dupFinder.findDuplicate(m_currRhsp, हे);
        जर (it != m_dupFinder.end()) परत m_aux_गाठ_अभिव्यक्ती(VN_AS(it->second, गाठ_अभिव्यक्ती)).parentp;

        // Insert new node
        m_dupFinder.घाला(m_currRhsp);
        m_aux_गाठ_अभिव्यक्ती(m_currRhsp) = m_auxCurRhsp;
        परत nullptr;
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
    Ast_गाठ_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_गाठ_अभिव्यक्ती* m_ifCondp = nullptr;  // IF condition that assign is under
    तार्किक m_always = असत्य;  // Assign is under an always
    तार्किक m_dedupable = सत्य;  // Determined the assign to be dedupable

    // VISITORS
    शून्य visit(Ast_गाठ_Assign* nodep) override {
        जर (!m_dedupable) परत;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        जर (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_अभिव्यक्ती_Stmt*) { परत सत्य; });
            परत;
        }

        m_dedupable = असत्य;
    }

    शून्य visit(AstAlways* nodep) override {
        जर (!m_dedupable) परत;

        जर (!m_always) {
            m_always = सत्य;
            iterateAndNextConstNull(nodep->stmtsp());
            परत;
        }

        m_dedupable = असत्य;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    शून्य visit(Ast_गाठ_If* nodep) override {
        जर (!m_dedupable) परत;

        जर (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_अभिव्यक्ती_Stmt*) { परत सत्य; });
            iterateAndNextConstNull(nodep->thensp());
            परत;
        }

        m_dedupable = असत्य;
    }

    शून्य visit(AstComment*) override {}  // NOP
    //--------------------
    शून्य visit(Ast_गाठ*) override { m_dedupable = असत्य; }

सार्वजनिक:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = पूर्वनिर्धारित;
    ~GateDedupeVarVisitor() override = पूर्वनिर्धारित;
    // PUBLIC METHODS
    Ast_गाठ_Var_Ref* findDupe(Ast_गाठ* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = असत्य;
        m_dedupable = सत्य;
        iterateConst(logicp);
        जर (m_dedupable && m_assignp) {
            स्थिरांक Ast_गाठ* स्थिरांक lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक lRefp = VN_CAST(lhsp, गाठ_Var_Ref);
            जर (lRefp && lRefp->varScopep() == consumerVscp) {
                जर (स्थिरांक Ast_गाठ_Assign* स्थिरांक dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    परत static_cast<Ast_गाठ_Var_Ref*>(dup->lhsp());
                }
            }
        }
        परत nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

वर्ग GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    स्थिरांक VNUser2InUse m_inuser2;

    // STATE
    माप_प्रकार m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    शून्य visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        जर (m_depth > GATE_DEDUP_MAX_DEPTH) परत;

        // Check that we haven't been here before
        जर (vVtxp->varScp()->user2SetOnce()) परत;

        VL_RESTORER(m_depth);
        ++m_depth;
        जर (!vVtxp->inSize1()) परत;

        Ast_गाठ_Var_Ref* dupRefp = nullptr;
        साठी (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        जर (!dupRefp) परत;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* स्थिरांक lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        स्थिरांक GateOkVisitor okVisitor{lVtxp->nodep(), असत्य, सत्य};
        जर (!okVisitor.isSimple()) परत;

        ++m_statDedupLogic;
        GateVarVertex* स्थिरांक dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        साठी (V3GraphEdge* स्थिरांक edgep : vVtxp->outEdges().unlinkable()) {
            स्थिरांक GateLogicVertex* स्थिरांक consumerVtxp = edgep->शीर्ष()->as<GateLogicVertex>();
            Ast_गाठ* स्थिरांक consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            जर (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } अन्यथा {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_गाठ_Var_Ref* refp) {
                    जर (refp->varScopep() != vVtxp->varScp()) परत;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_गाठ_Var_Ref* स्थिरांक newp = dupRefp->clone_वृक्ष_Pure(असत्य);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        पर्यंत (V3GraphEdge* स्थिरांक edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_गाठ_Var_Ref* visit(GateLogicVertex* lVtxp, स्थिरांक GateVarVertex* consumerVtxp) {
        साठी (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        जर (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            परत m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        परत nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        साठी (V3GraphVertex& vtx : graph.vertices()) {
            जर (GateVarVertex* स्थिरांक vVtxp = vtx.cast<GateVarVertex>()) {
                जर (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        साठी (V3GraphVertex& vtx : graph.vertices()) {
            जर (GateVarVertex* स्थिरांक vVtxp = vtx.cast<GateVarVertex>()) {
                जर (vVtxp->is_शीर्ष() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
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
    माप_प्रकार m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        स्थिरांक AstVarRef* स्थिरांक pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* स्थिरांक cRefp = VN_CAST(currSelp->fromp(), VarRef);
        जर (!pRefp || !cRefp || !cRefp->same_गाठ(pRefp)) परत nullptr;  // not the same var

        स्थिरांक AstConst* स्थिरांक pstart = VN_CAST(prevSelp->lsbp(), Const);
        स्थिरांक AstConst* स्थिरांक cstart = VN_CAST(currSelp->lsbp(), Const);
        जर (!pstart || !cstart) परत nullptr;  // too complicated

        जर (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            परत नवीन AstSel{cRefp->fileline(), cRefp->clone_वृक्ष(असत्य), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } अन्यथा {
            परत nullptr;
        }
    }

    शून्य process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        साठी (V3GraphEdge* स्थिरांक edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* स्थिरांक lVtxp = edgep->fromp()->as<GateLogicVertex>();
            जर (!lVtxp->outSize1()) सुरूठेवा;

            AstAlways* स्थिरांक alwaysp = VN_CAST(lVtxp->nodep(), Always);
            जर (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) परत;
            AstAssignW* स्थिरांक assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            जर (!assignp) सुरूठेवा;

            जर (!VN_IS(assignp->lhsp(), Sel)) सुरूठेवा;

            // First assign with Sel-lhs, or not under the same active
            जर (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                सुरूठेवा;
            }

            AstSel* स्थिरांक prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* स्थिरांक currSelp = VN_AS(assignp->lhsp(), Sel);

            जर (AstSel* स्थिरांक newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_वृक्ष(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(नवीन AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                पर्यंत (V3GraphEdge* स्थिरांक iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* स्थिरांक fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } अन्यथा {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        साठी (V3GraphVertex& vtx : graph.vertices()) {
            जर (GateVarVertex* स्थिरांक vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        साठी (GateLogicVertex* स्थिरांक lVtxp : m_toRemove) {
            Ast_गाठ* स्थिरांक nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
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
        जर (vtxp->user()) परत;  // Already marked
        vtxp->user(सत्य);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        साठी (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* स्थिरांक fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    शून्य mark() {
        m_graph.userClearVertices();
        साठी (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            जर (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    स्थिर शून्य warnUnused(स्थिरांक Ast_गाठ* स्थिरांक nodep) {
        जर (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) परत;

        जर (स्थिरांक Ast_गाठ_Procedure* स्थिरांक procedurep = VN_CAST(nodep, गाठ_Procedure)) {
            जर (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](स्थिरांक AstLoop* स्थिरांक loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, सत्य);
                });
        }
    }

    // Remove unused logic
    शून्य हटाओ() {
        साठी (V3GraphVertex* स्थिरांक vtxp : m_graph.vertices().unlinkable()) {
            जर (GateLogicVertex* स्थिरांक lVtxp = vtxp->cast<GateLogicVertex>()) {
                जर (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_गाठ* स्थिरांक nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
                }
            }
        }
    }

    explicit GateUnused(GateGraph& graph)
        : m_graph{graph} {
        mark();  // Mark all used vertices
        हटाओ();  // Remove unused vertices
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
        जर (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        जर (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        जर (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        जर (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        जर (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_वृक्ष("gate", 0, dump_वृक्ष_Either_Level() >= 3);
}
