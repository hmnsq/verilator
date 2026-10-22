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
    ~GateEitherVertex() override = स्वभाव;

    // ACCESSORS
    तार्किक reducible() अचर { फर्काऊ m_reducible; }
    तार्किक dedupable() अचर { फर्काऊ m_dedupable; }
    तार्किक consumed() अचर { फर्काऊ m_consumed; }
    तार्किक staticInit() अचर { फर्काऊ m_staticInit; }
    शून्य setConsumed(अचर वर्ण* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = सत्य;
    }
    शून्य setStaticInit() { m_staticInit = सत्य; }
    शून्य clearReducible(अचर वर्ण* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = असत्य;
    }
    शून्य clearDedupable(अचर वर्ण* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = असत्य;
    }
    शून्य clearReducibleAndDedupable(अचर वर्ण* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() अचर override { फर्काऊ m_consumed ? "" : "dotted"; }
};

वर्ग GateVarVertex final : सार्वजनिक GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* अचर m_varScp;
    तार्किक m_is_शीर्ष = असत्य;
    तार्किक m_isClock = असत्य;
    Ast_गाँठो* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_गाँठो* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
सार्वजनिक:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = स्वभाव;

    // ACCESSORS
    AstVarScope* varScp() अचर VL_MT_STABLE { फर्काऊ m_varScp; }
    तार्किक is_शीर्ष() अचर { फर्काऊ m_is_शीर्ष; }
    शून्य set_Is_शीर्ष() { m_is_शीर्ष = सत्य; }
    तार्किक isClock() अचर { फर्काऊ m_isClock; }
    शून्य setIsClock() { m_isClock = सत्य; }
    Ast_गाँठो* rstSyncNodep() अचर { फर्काऊ m_rstSyncNodep; }
    शून्य rstSyncNodep(Ast_गाँठो* nodep) { m_rstSyncNodep = nodep; }
    Ast_गाँठो* rstAsyncNodep() अचर { फर्काऊ m_rstAsyncNodep; }
    शून्य rstAsyncNodep(Ast_गाँठो* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    शून्य propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        यदि (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() अचर override VL_MT_STABLE { फर्काऊ varScp()->name(); }
    std::string dotColor() अचर override { फर्काऊ "green"; }
};

वर्ग GateLogicVertex final : सार्वजनिक GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_गाँठो* अचर m_nodep;
    AstActive* अचर m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    अचर तार्किक m_slow;  // In slow block
सार्वजनिक:
    GateLogicVertex(V3Graph* graphp, Ast_गाँठो* nodep, AstActive* activep, तार्किक slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = स्वभाव;

    // ACCESSORS
    File_पङ्क्ति* fileline() अचर override { फर्काऊ nodep()->fileline(); }
    Ast_गाँठो* nodep() अचर { फर्काऊ m_nodep; }
    AstActive* activep() अचर { फर्काऊ m_activep; }
    तार्किक slow() अचर { फर्काऊ m_slow; }

    // DOT debug
    std::string name() अचर override { फर्काऊ m_nodep->fileline()->ascii(); }
    std::string dotColor() अचर override { फर्काऊ "red"; }
};

वर्ग GateEdge final : सार्वजनिक V3GraphEdge {
    std::string dotLabel() अचर override { फर्काऊ std::to_string(weight()); }

सार्वजनिक:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight)
        : V3GraphEdge{graphp, fromp, शीर्ष, weight} {}
};

वर्ग GateGraph final : सार्वजनिक V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    अचर VNUser1InUse m_inuser1;

सार्वजनिक:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        यदि (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = नूतन GateVarVertex{एतत्, vscp};
            vscp->user1p(vVtxp);
            यदि (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            यदि (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            यदि (vscp->varp()->isIO() && vscp->scopep()->is_शीर्ष()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_शीर्ष();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        फर्काऊ vVtxp;
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
    अचर Ast_गाँठो_Module* m_modp = nullptr;  // Current module
    अचर AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    तार्किक m_inClockedActive = असत्य;  // Underneath clocked active
    तार्किक m_inEdgeActive = असत्य;  // Underneath edge active
    तार्किक m_inStaticActive = असत्य;  // Underneath static active
    तार्किक m_inInitialActive = असत्य;  // Underneath initial active
    तार्किक m_inSenItem = असत्य;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    शून्य check_गाँठो(Ast_गाँठो* nodep) {
        यदि (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        यदि (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    शून्य iterateLogic(Ast_गाँठो* nodep, तार्किक slow = असत्य, अचर वर्ण* nonReducibleReason = nullptr,
                      अचर वर्ण* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = नूतन GateLogicVertex{m_graphp, nodep, m_activep, slow};
        यदि (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } अन्यथा यदि (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } अन्यथा यदि (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        यदि (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_गाँठो(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
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
        अचर VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    शून्य visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    शून्य visit(Ast_गाँठो_Procedure* nodep) override {
        अचर तार्किक slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    शून्य visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, असत्य, "CoverToggle", "CoverToggle");
    }
    शून्य visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = सत्य;
        यदि (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } अन्यथा {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, असत्य, nullptr, "senItem");
        }
    }
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        यदि (!m_logicVertexp) फर्काऊ;

        AstVarScope* अचर vscp = nodep->varScopep();
        GateVarVertex* अचर vVtxp = m_graphp->makeVarVertex(vscp);

        यदि (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(सत्य);
        } अन्यथा यदि (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            यदि (vscp->user2()) {
                यदि (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } अन्यथा {
                यदि (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        यदि (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        यदि (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    शून्य visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), गाँठो_Assign)
                      && VN_AS(nodep->backp(), गाँठो_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (m_logicVertexp) check_गाँठो(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

सार्वजनिक:
    स्थिर std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        फर्काऊ std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

स्थिर शून्य v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    अचर VNUser2InUse user2InUse;
    लागि (V3GraphVertex& vtx : graph.vertices()) {
        यदि (अचर GateVarVertex* अचर vvertexp = vtx.cast<GateVarVertex>()) {
            अचर AstVarScope* अचर vscp = vvertexp->varScp();
            अचर Ast_गाँठो* अचर sp = vvertexp->rstSyncNodep();
            अचर Ast_गाँठो* अचर ap = vvertexp->rstAsyncNodep();
            यदि (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                यदि (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
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
    अचर AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    पूर्णाङ्क m_offset = 0;  // Current offset of varscope
    पूर्णाङ्क m_found_offset = 0;  // Found offset of varscope
    तार्किक m_found = असत्य;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        यदि (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
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
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // PUBLIC METHODS
    तार्किक concatOffset(AstConcat* concatp, AstVarScope* vscp, पूर्णाङ्क& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = असत्य;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        फर्काऊ m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

वर्ग GateOkVisitor final : सार्वजनिक VNVisitorConst {
    // RETURN STATE
    तार्किक m_isSimple = सत्य;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_गाँठो_अभिव्यक्ति* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    अचर तार्किक m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    अचर तार्किक m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    अचर Ast_गाँठो_Var_Ref* m_lhsVarRef = nullptr;
    पूर्णाङ्क m_ops = 0;  // Operation count

    // METHODS
    शून्य clearSimple(अचर वर्ण* because) {
        यदि (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = असत्य;
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Var_Ref* nodep) override {
        यदि (!m_isSimple) फर्काऊ;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        यदि (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        यदि (nodep->access().isRW()) {
            clearSimple("R/W");
            फर्काऊ;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* अचर vscp = nodep->varScopep();
        यदि (nodep->access().isWriteOnly()) {
            यदि (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } अन्यथा {
            // TODO: possible bug, should it be >= 1 as add is below?
            यदि (m_readVscps.size() > 1) {
                यदि (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.थप_back(vscp);
        }
    }
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        यदि (!m_isSimple) फर्काऊ;

        m_substitutionp = nodep->rhsp();
        यदि (!VN_IS(nodep->lhsp(), गाँठो_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } अन्यथा यदि (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
        यदि (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    शून्य visit(AstCReset* /*nodep*/) override {
        यदि (!m_isSimple) फर्काऊ;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    शून्य visit(AstMatchMasked* nodep) override {
        यदि (!m_isSimple) फर्काऊ;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override {
        यदि (!m_isSimple) फर्काऊ;  // Fastpath

        यदि (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            फर्काऊ;
        }

        यदि (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            फर्काऊ;
        }

        iterateChildrenConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    GateOkVisitor(Ast_गाँठो* nodep, तार्किक buffersOnly, तार्किक dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        यदि (!m_substitutionp) {
            clearSimple("No assignment found\n");
            फर्काऊ;
        }
        यदि (m_isSimple && m_lhsVarRef) {
            लागि (अचर AstVarScope* अचर vscp : m_readVscps) {
                यदि (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    फर्काऊ;
                }
            }
        }
    }
    ~GateOkVisitor() override = स्वभाव;
    // PUBLIC METHODS
    तार्किक isSimple() अचर { फर्काऊ m_isSimple; }
    Ast_गाँठो_अभिव्यक्ति* substitutionp() अचर {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        फर्काऊ m_substitutionp;
    }
    अचर std::vector<AstVarScope*>& readVscps() अचर { फर्काऊ m_readVscps; }
    तार्किक varAssigned(अचर AstVarScope* scopep) अचर {
        फर्काऊ m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

वर्ग GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_गाँठो_अभिव्यक्ति*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    अचर VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_गाँठो, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    माप_प्रकार m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_गाँठो*, माप_प्रकार> m_hasPending;
    माप_प्रकार m_statInlined = 0;  // Statistic tracking - signals inlined
    माप_प्रकार m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    माप_प्रकार m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    स्थिर तार्किक isCheap(अचर Ast_गाँठो_अभिव्यक्ति* exprp) {
        // Constant is cheap
        यदि (VN_IS(exprp, Const)) फर्काऊ सत्य;
        // Variable reference is cheap
        यदि (VN_IS(exprp, गाँठो_Var_Ref)) फर्काऊ सत्य;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        यदि (अचर AstSel* अचर selp = VN_CAST(exprp, Sel)) {
            यदि (!isCheap(selp->fromp())) फर्काऊ असत्य;
            यदि (!selp->isWide()) फर्काऊ सत्य;
            यदि (!VN_IS(selp->lsbp(), Const)) फर्काऊ असत्य;
            यदि (selp->lsbConst() % VL_EDATASIZE != 0) फर्काऊ असत्य;
            फर्काऊ सत्य;
        }
        // AstArraySel is cheap if the fromp is cheap
        यदि (अचर AstArraySel* अचर aselp = VN_CAST(exprp, ArraySel)) {
            फर्काऊ isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        फर्काऊ असत्य;
    }

    तार्किक shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, माप_प्रकार nReads,
                      Ast_गाँठो_अभिव्यक्ति* substp, तार्किक allowMultiIn) {
        // Always inline constants
        यदि (VN_IS(substp, Const)) फर्काऊ सत्य;
        // Don't inline non-constant static initializers - these are scheduled differently
        यदि (lVtxp->staticInit()) फर्काऊ असत्य;
        // Inline simple variable references
        यदि (VN_IS(substp, VarRef)) फर्काऊ सत्य;
        // Only inline arrays if a simple variable or constant
        यदि (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) फर्काऊ असत्य;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        यदि (nReads == 0) फर्काऊ सत्य;
        // If it reads one variable, inline if not wide, or if cheap
        यदि (nReads == 1 && (!substp->isWide() || isCheap(substp))) फर्काऊ सत्य;
        // Don't inline on first round if reads more than one variable
        यदि (nReads > 1 && !allowMultiIn) फर्काऊ असत्य;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        पूर्णाङ्क n = 0;
        लागि (V3GraphEdge& edge : vVtxp->outEdges()) {
            अचर GateLogicVertex* अचर dstVtxp = edge.शीर्ष()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            यदि (dstVtxp->slow()) जारी;
            यदि (dstVtxp->outEmpty() && !dstVtxp->consumed()) जारी;
            n += edge.weight();
            यदि (n > 1) {
                ++m_statNotInlined;
                फर्काऊ असत्य;
            }
        }
        फर्काऊ सत्य;
    }

    शून्य recordSubstitution(AstVarScope* vscp, Ast_गाँठो_अभिव्यक्ति* substp, Ast_गाँठो* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        अचर स्वतः pair = m_substitutions(logicp).emplace(vscp, nullptr);
        यदि (pair.second) pair.first->second = substp->clone_रूख_Pure(असत्य);
    }

    शून्य commitSubstitutions(Ast_गाँठो* logicp) {
        यदि (!m_hasPending.erase(logicp)) फर्काऊ;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<अचर AstVarScope*> replaced(substitutions.size() * 2);

        अचर std::function<शून्य(Ast_गाँठो_Var_Ref*)> visit = [&](Ast_गाँठो_Var_Ref* nodep) -> शून्य {
            // See if this variable has a substitution
            AstVarScope* अचर vscp = nodep->varScopep();
            अचर स्वतः& it = substitutions.find(vscp);
            यदि (it == substitutions.end()) फर्काऊ;

            // Do not substitute circular logic
            यदि (!replaced.घुसाउ(vscp).second) फर्काऊ;

            // Substitute nodep with substp
            Ast_गाँठो_अभिव्यक्ति* अचर substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, गाँठो_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_गाँठो_अभिव्यक्ति* अचर newp = substp->clone_रूख_Pure(असत्य);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            यदि (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            यदि (Ast_गाँठो_Var_Ref* अचर varrefp = VN_CAST(newp, गाँठो_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_गाँठो* अचर simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        लागि (अचर स्वतः& pair : substitutions) pair.second->delete_रूख();
        substitutions.clear();
    }

    शून्य optimizeSignals(तार्किक allowMultiIn) {
        // Consider "inlining" variables
        स्वतः& vertices = m_graph.vertices();
        अचर स्वतः ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            जबसम्म (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            फर्काऊ it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        जबसम्म (vIt != vertices.end()) {
            GateVarVertex* अचर vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            यदि (!vVtxp->inSize1()) जारी;

            // Can't inline if non-reducible, etc
            यदि (!vVtxp->reducible()) जारी;

            // Grab the driving logic
            GateLogicVertex* अचर lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            यदि (!lVtxp->reducible()) जारी;
            Ast_गाँठो* अचर logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            अचर GateOkVisitor okVisitor{logicp, असत्य, असत्य};

            // Was it ok?
            यदि (!okVisitor.isSimple()) जारी;
            // If the varScope is already removed from logicp, no need to try substitution.
            यदि (!okVisitor.varAssigned(vVtxp->varScp())) जारी;

            // Expression we are considering to substitute with
            Ast_गाँठो_अभिव्यक्ति* अचर substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            अचर माप_प्रकार nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            यदि (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) जारी;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* अचर vscp = vVtxp->varScp();

            यदि (debug() >= 9) {
                vscp->dump_रूख("substituting: ");
                substp->dump_रूख("        with: ");
            }

            अचर स्वतः& readVscpsVec = okVisitor.readVscps();
            अचर std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            लागि (V3GraphEdge* अचर edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* अचर dstVtxp = edgep->शीर्ष()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                यदि (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_गाँठो* nodep = substp;
                    यदि (AstNot* अचर notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    यदि (!VN_IS(nodep, VarRef)) जारी;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                तार्किक canInline = सत्य;
                लागि (अचर V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    अचर GateVarVertex* अचर consVVertexp = dedge.शीर्ष()->as<GateVarVertex>();
                    यदि (readVscps.गन्ती(consVVertexp->varScp())) {
                        canInline = असत्य;
                        रोक;
                    }
                }

                यदि (!canInline) जारी;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                यदि (logicp == dstVtxp->nodep()) {
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
                    जारी;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                लागि (AstVarScope* अचर newVscp : okVisitor.readVscps()) {
                    GateVarVertex* अचर varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            यदि (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_रूख(), logicp);
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
        using Pair = std::pair<Ast_गाँठो*, माप_प्रकार>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](अचर Pair& a, अचर Pair& b) {  //
            फर्काऊ a.second < b.second;
        });
        लागि (अचर स्वतः& pair : pending) commitSubstitutions(pair.first);
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
    संरचना Aux_Ast_गाँठो_अभिव्यक्ति final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_गाँठो_अभिव्यक्ति* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_गाँठो_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_गाँठो_अभिव्यक्ति, Aux_Ast_गाँठो_अभिव्यक्ति> m_aux_गाँठो_अभिव्यक्ति;

    Ast_गाँठो_अभिव्यक्ति* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_गाँठो_अभिव्यक्ति m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    स्थिर तार्किक same(Ast_गाँठो* node1p, Ast_गाँठो* node2p) {
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
        फर्काऊ node1p == node2p || (node1p && node1p->same_रूख(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    तार्किक isSame(Ast_गाँठो* node1p, Ast_गाँठो* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        अचर स्वतः& aux2 = m_aux_गाँठो_अभिव्यक्ति(VN_AS(node2p, गाँठो_अभिव्यक्ति));
        फर्काऊ m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

सार्वजनिक:
    GateDedupeHash() = स्वभाव;
    ~GateDedupeHash() = स्वभाव;

    अचर Ast_गाँठो_Assign* hashAndFindDupe(Ast_गाँठो_Assign* assignp, AstActive* activep,
                                         Ast_गाँठो_अभिव्यक्ति* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        अचर स्वतः it = m_dupFinder.findDuplicate(m_currRhsp, एतत्);
        यदि (it != m_dupFinder.end()) फर्काऊ m_aux_गाँठो_अभिव्यक्ति(VN_AS(it->second, गाँठो_अभिव्यक्ति)).parentp;

        // Insert new node
        m_dupFinder.घुसाउ(m_currRhsp);
        m_aux_गाँठो_अभिव्यक्ति(m_currRhsp) = m_auxCurRhsp;
        फर्काऊ nullptr;
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
    Ast_गाँठो_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_गाँठो_अभिव्यक्ति* m_ifCondp = nullptr;  // IF condition that assign is under
    तार्किक m_always = असत्य;  // Assign is under an always
    तार्किक m_dedupable = सत्य;  // Determined the assign to be dedupable

    // VISITORS
    शून्य visit(Ast_गाँठो_Assign* nodep) override {
        यदि (!m_dedupable) फर्काऊ;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        यदि (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_अभिव्यक्ति_Stmt*) { फर्काऊ सत्य; });
            फर्काऊ;
        }

        m_dedupable = असत्य;
    }

    शून्य visit(AstAlways* nodep) override {
        यदि (!m_dedupable) फर्काऊ;

        यदि (!m_always) {
            m_always = सत्य;
            iterateAndNextConstNull(nodep->stmtsp());
            फर्काऊ;
        }

        m_dedupable = असत्य;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    शून्य visit(Ast_गाँठो_If* nodep) override {
        यदि (!m_dedupable) फर्काऊ;

        यदि (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_अभिव्यक्ति_Stmt*) { फर्काऊ सत्य; });
            iterateAndNextConstNull(nodep->thensp());
            फर्काऊ;
        }

        m_dedupable = असत्य;
    }

    शून्य visit(AstComment*) override {}  // NOP
    //--------------------
    शून्य visit(Ast_गाँठो*) override { m_dedupable = असत्य; }

सार्वजनिक:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = स्वभाव;
    ~GateDedupeVarVisitor() override = स्वभाव;
    // PUBLIC METHODS
    Ast_गाँठो_Var_Ref* findDupe(Ast_गाँठो* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = असत्य;
        m_dedupable = सत्य;
        iterateConst(logicp);
        यदि (m_dedupable && m_assignp) {
            अचर Ast_गाँठो* अचर lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            अचर Ast_गाँठो_Var_Ref* अचर lRefp = VN_CAST(lhsp, गाँठो_Var_Ref);
            यदि (lRefp && lRefp->varScopep() == consumerVscp) {
                यदि (अचर Ast_गाँठो_Assign* अचर dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    फर्काऊ static_cast<Ast_गाँठो_Var_Ref*>(dup->lhsp());
                }
            }
        }
        फर्काऊ nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

वर्ग GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    अचर VNUser2InUse m_inuser2;

    // STATE
    माप_प्रकार m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    शून्य visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        यदि (m_depth > GATE_DEDUP_MAX_DEPTH) फर्काऊ;

        // Check that we haven't been here before
        यदि (vVtxp->varScp()->user2SetOnce()) फर्काऊ;

        VL_RESTORER(m_depth);
        ++m_depth;
        यदि (!vVtxp->inSize1()) फर्काऊ;

        Ast_गाँठो_Var_Ref* dupRefp = nullptr;
        लागि (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        यदि (!dupRefp) फर्काऊ;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* अचर lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        अचर GateOkVisitor okVisitor{lVtxp->nodep(), असत्य, सत्य};
        यदि (!okVisitor.isSimple()) फर्काऊ;

        ++m_statDedupLogic;
        GateVarVertex* अचर dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        लागि (V3GraphEdge* अचर edgep : vVtxp->outEdges().unlinkable()) {
            अचर GateLogicVertex* अचर consumerVtxp = edgep->शीर्ष()->as<GateLogicVertex>();
            Ast_गाँठो* अचर consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            यदि (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } अन्यथा {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_गाँठो_Var_Ref* refp) {
                    यदि (refp->varScopep() != vVtxp->varScp()) फर्काऊ;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_गाँठो_Var_Ref* अचर newp = dupRefp->clone_रूख_Pure(असत्य);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_रूख(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        जबसम्म (V3GraphEdge* अचर edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_गाँठो_Var_Ref* visit(GateLogicVertex* lVtxp, अचर GateVarVertex* consumerVtxp) {
        लागि (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        यदि (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            फर्काऊ m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        फर्काऊ nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        लागि (V3GraphVertex& vtx : graph.vertices()) {
            यदि (GateVarVertex* अचर vVtxp = vtx.cast<GateVarVertex>()) {
                यदि (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        लागि (V3GraphVertex& vtx : graph.vertices()) {
            यदि (GateVarVertex* अचर vVtxp = vtx.cast<GateVarVertex>()) {
                यदि (vVtxp->is_शीर्ष() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
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
        अचर AstVarRef* अचर pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* अचर cRefp = VN_CAST(currSelp->fromp(), VarRef);
        यदि (!pRefp || !cRefp || !cRefp->same_गाँठो(pRefp)) फर्काऊ nullptr;  // not the same var

        अचर AstConst* अचर pstart = VN_CAST(prevSelp->lsbp(), Const);
        अचर AstConst* अचर cstart = VN_CAST(currSelp->lsbp(), Const);
        यदि (!pstart || !cstart) फर्काऊ nullptr;  // too complicated

        यदि (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            फर्काऊ नूतन AstSel{cRefp->fileline(), cRefp->clone_रूख(असत्य), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } अन्यथा {
            फर्काऊ nullptr;
        }
    }

    शून्य process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        लागि (V3GraphEdge* अचर edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* अचर lVtxp = edgep->fromp()->as<GateLogicVertex>();
            यदि (!lVtxp->outSize1()) जारी;

            AstAlways* अचर alwaysp = VN_CAST(lVtxp->nodep(), Always);
            यदि (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) फर्काऊ;
            AstAssignW* अचर assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            यदि (!assignp) जारी;

            यदि (!VN_IS(assignp->lhsp(), Sel)) जारी;

            // First assign with Sel-lhs, or not under the same active
            यदि (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                जारी;
            }

            AstSel* अचर prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* अचर currSelp = VN_AS(assignp->lhsp(), Sel);

            यदि (AstSel* अचर newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_रूख(), prevSelp);
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
                जबसम्म (V3GraphEdge* अचर iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* अचर fromVtxp = iedgep->fromp()->as<GateVarVertex>();
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
        लागि (V3GraphVertex& vtx : graph.vertices()) {
            यदि (GateVarVertex* अचर vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        लागि (GateLogicVertex* अचर lVtxp : m_toRemove) {
            Ast_गाँठो* अचर nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_रूख(), nodep);
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
        यदि (vtxp->user()) फर्काऊ;  // Already marked
        vtxp->user(सत्य);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        लागि (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* अचर fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    शून्य mark() {
        m_graph.userClearVertices();
        लागि (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            यदि (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    स्थिर शून्य warnUnused(अचर Ast_गाँठो* अचर nodep) {
        यदि (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) फर्काऊ;

        यदि (अचर Ast_गाँठो_Procedure* अचर procedurep = VN_CAST(nodep, गाँठो_Procedure)) {
            यदि (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](अचर AstLoop* अचर loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, सत्य);
                });
        }
    }

    // Remove unused logic
    शून्य हटाओ() {
        लागि (V3GraphVertex* अचर vtxp : m_graph.vertices().unlinkable()) {
            यदि (GateLogicVertex* अचर lVtxp = vtxp->cast<GateLogicVertex>()) {
                यदि (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_गाँठो* अचर nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_रूख(), nodep);
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
        यदि (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        यदि (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        यदि (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        यदि (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        यदि (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_रूख("gate", 0, dump_रूख_Either_Level() >= 3);
}
