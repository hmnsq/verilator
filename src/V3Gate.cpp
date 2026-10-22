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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Gate.h"

#समावेश "V3AstUserAllocator.h"
#समावेश "V3Const.h"
#समावेश "V3DupFinder.h"
#समावेश "V3Graph.h"
#समावेश "V3Stats.h"

#समावेश <list>
#समावेश <unordered_map>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

श्रेणी GateEitherVertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    bool m_reducible = true;  // True if this node should be able to be eliminated
    bool m_dedupable = true;  // True if this node should be able to be deduped
    bool m_consumed = false;  // Output goes to something meaningful
    bool m_staticInit = false;  // True if this node is a static initializer
खुला:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = शेष;

    // ACCESSORS
    bool reducible() स्थिर { वापस m_reducible; }
    bool dedupable() स्थिर { वापस m_dedupable; }
    bool consumed() स्थिर { वापस m_consumed; }
    bool staticInit() स्थिर { वापस m_staticInit; }
    व्योम setConsumed(स्थिर अक्षर* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = true;
    }
    व्योम setStaticInit() { m_staticInit = true; }
    व्योम clearReducible(स्थिर अक्षर* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = false;
    }
    व्योम clearDedupable(स्थिर अक्षर* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = false;
    }
    व्योम clearReducibleAndDedupable(स्थिर अक्षर* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() स्थिर override { वापस m_consumed ? "" : "dotted"; }
};

श्रेणी GateVarVertex final : खुला GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* स्थिर m_varScp;
    bool m_is_शीर्ष = false;
    bool m_isClock = false;
    Ast_गाँठ* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_गाँठ* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
खुला:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = शेष;

    // ACCESSORS
    AstVarScope* varScp() स्थिर VL_MT_STABLE { वापस m_varScp; }
    bool is_शीर्ष() स्थिर { वापस m_is_शीर्ष; }
    व्योम set_Is_शीर्ष() { m_is_शीर्ष = true; }
    bool isClock() स्थिर { वापस m_isClock; }
    व्योम setIsClock() { m_isClock = true; }
    Ast_गाँठ* rstSyncNodep() स्थिर { वापस m_rstSyncNodep; }
    व्योम rstSyncNodep(Ast_गाँठ* nodep) { m_rstSyncNodep = nodep; }
    Ast_गाँठ* rstAsyncNodep() स्थिर { वापस m_rstAsyncNodep; }
    व्योम rstAsyncNodep(Ast_गाँठ* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    व्योम propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        अगर (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() स्थिर override VL_MT_STABLE { वापस varScp()->name(); }
    std::string dotColor() स्थिर override { वापस "green"; }
};

श्रेणी GateLogicVertex final : खुला GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_गाँठ* स्थिर m_nodep;
    AstActive* स्थिर m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    स्थिर bool m_slow;  // In slow block
खुला:
    GateLogicVertex(V3Graph* graphp, Ast_गाँठ* nodep, AstActive* activep, bool slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = शेष;

    // ACCESSORS
    File_पंक्ति* fileline() स्थिर override { वापस nodep()->fileline(); }
    Ast_गाँठ* nodep() स्थिर { वापस m_nodep; }
    AstActive* activep() स्थिर { वापस m_activep; }
    bool slow() स्थिर { वापस m_slow; }

    // DOT debug
    std::string name() स्थिर override { वापस m_nodep->fileline()->ascii(); }
    std::string dotColor() स्थिर override { वापस "red"; }
};

श्रेणी GateEdge final : खुला V3GraphEdge {
    std::string dotLabel() स्थिर override { वापस std::to_string(weight()); }

खुला:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight)
        : V3GraphEdge{graphp, fromp, शीर्ष, weight} {}
};

श्रेणी GateGraph final : खुला V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    स्थिर VNUser1InUse m_inuser1;

खुला:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        अगर (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = नया GateVarVertex{यह, vscp};
            vscp->user1p(vVtxp);
            अगर (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            अगर (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            अगर (vscp->varp()->isIO() && vscp->scopep()->is_शीर्ष()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_शीर्ष();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        वापस vVtxp;
    }

    व्योम addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, पूर्णांक weight) {
        नया GateEdge{यह, srcp, dstp, weight};
    }

    व्योम addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, पूर्णांक weight) {
        नया GateEdge{यह, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

श्रेणी GateBuildVisitor final : खुला VNVisitorConst {
    // STATE
    GateGraph* m_graphp = नया GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    स्थिर Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    स्थिर AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    bool m_inClockedActive = false;  // Underneath clocked active
    bool m_inEdgeActive = false;  // Underneath edge active
    bool m_inStaticActive = false;  // Underneath static active
    bool m_inInitialActive = false;  // Underneath initial active
    bool m_inSenItem = false;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    व्योम check_गाँठ(Ast_गाँठ* nodep) {
        अगर (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        अगर (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    व्योम iterateLogic(Ast_गाँठ* nodep, bool slow = false, स्थिर अक्षर* nonReducibleReason = nullptr,
                      स्थिर अक्षर* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = नया GateLogicVertex{m_graphp, nodep, m_activep, slow};
        अगर (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } अन्यथा अगर (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } अन्यथा अगर (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        अगर (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_गाँठ(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    व्योम visit(AstActive* nodep) override {
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
        स्थिर VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    व्योम visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {
        स्थिर bool slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    व्योम visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, false, "CoverToggle", "CoverToggle");
    }
    व्योम visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = true;
        अगर (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } अन्यथा {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, false, nullptr, "senItem");
        }
    }
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (!m_logicVertexp) वापस;

        AstVarScope* स्थिर vscp = nodep->varScopep();
        GateVarVertex* स्थिर vVtxp = m_graphp->makeVarVertex(vscp);

        अगर (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(true);
        } अन्यथा अगर (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            अगर (vscp->user2()) {
                अगर (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } अन्यथा {
                अगर (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        अगर (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        अगर (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    व्योम visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), गाँठ_Assign)
                      && VN_AS(nodep->backp(), गाँठ_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (m_logicVertexp) check_गाँठ(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

खुला:
    अटल std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        वापस std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

अटल व्योम v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    स्थिर VNUser2InUse user2InUse;
    क्रम (V3GraphVertex& vtx : graph.vertices()) {
        अगर (स्थिर GateVarVertex* स्थिर vvertexp = vtx.cast<GateVarVertex>()) {
            स्थिर AstVarScope* स्थिर vscp = vvertexp->varScp();
            स्थिर Ast_गाँठ* स्थिर sp = vvertexp->rstSyncNodep();
            स्थिर Ast_गाँठ* स्थिर ap = vvertexp->rstAsyncNodep();
            अगर (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                अगर (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
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

श्रेणी GateConcatVisitor final : खुला VNVisitorConst {
    // STATE
    स्थिर AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    पूर्णांक m_offset = 0;  // Current offset of varscope
    पूर्णांक m_found_offset = 0;  // Found offset of varscope
    bool m_found = false;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(true);
            m_found_offset = m_offset;
            m_found = true;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    व्योम visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // PUBLIC METHODS
    bool concatOffset(AstConcat* concatp, AstVarScope* vscp, पूर्णांक& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = false;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        वापस m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

श्रेणी GateOkVisitor final : खुला VNVisitorConst {
    // RETURN STATE
    bool m_isSimple = true;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_गाँठ_व्यंजक* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    स्थिर bool m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    स्थिर bool m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    स्थिर Ast_गाँठ_Var_Ref* m_lhsVarRef = nullptr;
    पूर्णांक m_ops = 0;  // Operation count

    // METHODS
    व्योम clearSimple(स्थिर अक्षर* because) {
        अगर (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = false;
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Var_Ref* nodep) override {
        अगर (!m_isSimple) वापस;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        अगर (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        अगर (nodep->access().isRW()) {
            clearSimple("R/W");
            वापस;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* स्थिर vscp = nodep->varScopep();
        अगर (nodep->access().isWriteOnly()) {
            अगर (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } अन्यथा {
            // TODO: possible bug, should it be >= 1 as add is below?
            अगर (m_readVscps.आकार() > 1) {
                अगर (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.धकेलो_back(vscp);
        }
    }
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (!m_isSimple) वापस;

        m_substitutionp = nodep->rhsp();
        अगर (!VN_IS(nodep->lhsp(), गाँठ_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } अन्यथा अगर (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } अन्यथा {
            iterateChildrenConst(nodep);
        }
        अगर (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    व्योम visit(AstCReset* /*nodep*/) override {
        अगर (!m_isSimple) वापस;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    व्योम visit(AstMatchMasked* nodep) override {
        अगर (!m_isSimple) वापस;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (!m_isSimple) वापस;  // Fastpath

        अगर (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            वापस;
        }

        अगर (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            वापस;
        }

        iterateChildrenConst(nodep);
    }

खुला:
    // CONSTRUCTORS
    GateOkVisitor(Ast_गाँठ* nodep, bool buffersOnly, bool dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        अगर (!m_substitutionp) {
            clearSimple("No assignment found\n");
            वापस;
        }
        अगर (m_isSimple && m_lhsVarRef) {
            क्रम (स्थिर AstVarScope* स्थिर vscp : m_readVscps) {
                अगर (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    वापस;
                }
            }
        }
    }
    ~GateOkVisitor() override = शेष;
    // PUBLIC METHODS
    bool isSimple() स्थिर { वापस m_isSimple; }
    Ast_गाँठ_व्यंजक* substitutionp() स्थिर {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        वापस m_substitutionp;
    }
    स्थिर std::vector<AstVarScope*>& readVscps() स्थिर { वापस m_readVscps; }
    bool varAssigned(स्थिर AstVarScope* scopep) स्थिर {
        वापस m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

श्रेणी GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_गाँठ_व्यंजक*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    स्थिर VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_गाँठ, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    माप_प्रकार m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_गाँठ*, माप_प्रकार> m_hasPending;
    माप_प्रकार m_statInlined = 0;  // Statistic tracking - signals inlined
    माप_प्रकार m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    माप_प्रकार m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    अटल bool isCheap(स्थिर Ast_गाँठ_व्यंजक* exprp) {
        // Constant is cheap
        अगर (VN_IS(exprp, Const)) वापस true;
        // Variable reference is cheap
        अगर (VN_IS(exprp, गाँठ_Var_Ref)) वापस true;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        अगर (स्थिर AstSel* स्थिर selp = VN_CAST(exprp, Sel)) {
            अगर (!isCheap(selp->fromp())) वापस false;
            अगर (!selp->isWide()) वापस true;
            अगर (!VN_IS(selp->lsbp(), Const)) वापस false;
            अगर (selp->lsbConst() % VL_EDATASIZE != 0) वापस false;
            वापस true;
        }
        // AstArraySel is cheap if the fromp is cheap
        अगर (स्थिर AstArraySel* स्थिर aselp = VN_CAST(exprp, ArraySel)) {
            वापस isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        वापस false;
    }

    bool shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, माप_प्रकार nReads,
                      Ast_गाँठ_व्यंजक* substp, bool allowMultiIn) {
        // Always inline constants
        अगर (VN_IS(substp, Const)) वापस true;
        // Don't inline non-constant static initializers - these are scheduled differently
        अगर (lVtxp->staticInit()) वापस false;
        // Inline simple variable references
        अगर (VN_IS(substp, VarRef)) वापस true;
        // Only inline arrays if a simple variable or constant
        अगर (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) वापस false;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        अगर (nReads == 0) वापस true;
        // If it reads one variable, inline if not wide, or if cheap
        अगर (nReads == 1 && (!substp->isWide() || isCheap(substp))) वापस true;
        // Don't inline on first round if reads more than one variable
        अगर (nReads > 1 && !allowMultiIn) वापस false;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        पूर्णांक n = 0;
        क्रम (V3GraphEdge& edge : vVtxp->outEdges()) {
            स्थिर GateLogicVertex* स्थिर dstVtxp = edge.शीर्ष()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            अगर (dstVtxp->slow()) जारी;
            अगर (dstVtxp->outEmpty() && !dstVtxp->consumed()) जारी;
            n += edge.weight();
            अगर (n > 1) {
                ++m_statNotInlined;
                वापस false;
            }
        }
        वापस true;
    }

    व्योम recordSubstitution(AstVarScope* vscp, Ast_गाँठ_व्यंजक* substp, Ast_गाँठ* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        स्थिर स्वतः pair = m_substitutions(logicp).emplace(vscp, nullptr);
        अगर (pair.second) pair.first->second = substp->clone_वृक्ष_Pure(false);
    }

    व्योम commitSubstitutions(Ast_गाँठ* logicp) {
        अगर (!m_hasPending.erase(logicp)) वापस;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<स्थिर AstVarScope*> replaced(substitutions.आकार() * 2);

        स्थिर std::function<व्योम(Ast_गाँठ_Var_Ref*)> visit = [&](Ast_गाँठ_Var_Ref* nodep) -> व्योम {
            // See if this variable has a substitution
            AstVarScope* स्थिर vscp = nodep->varScopep();
            स्थिर स्वतः& it = substitutions.find(vscp);
            अगर (it == substitutions.end()) वापस;

            // Do not substitute circular logic
            अगर (!replaced.जोड़ो(vscp).second) वापस;

            // Substitute nodep with substp
            Ast_गाँठ_व्यंजक* स्थिर substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, गाँठ_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_गाँठ_व्यंजक* स्थिर newp = substp->clone_वृक्ष_Pure(false);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            अगर (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            अगर (Ast_गाँठ_Var_Ref* स्थिर varrefp = VN_CAST(newp, गाँठ_Var_Ref)) {
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

        Ast_गाँठ* स्थिर simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        क्रम (स्थिर स्वतः& pair : substitutions) pair.second->delete_वृक्ष();
        substitutions.सफा();
    }

    व्योम optimizeSignals(bool allowMultiIn) {
        // Consider "inlining" variables
        स्वतः& vertices = m_graph.vertices();
        स्थिर स्वतः ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            जबतक (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            वापस it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        जबतक (vIt != vertices.end()) {
            GateVarVertex* स्थिर vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            अगर (!vVtxp->inSize1()) जारी;

            // Can't inline if non-reducible, etc
            अगर (!vVtxp->reducible()) जारी;

            // Grab the driving logic
            GateLogicVertex* स्थिर lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            अगर (!lVtxp->reducible()) जारी;
            Ast_गाँठ* स्थिर logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            स्थिर GateOkVisitor okVisitor{logicp, false, false};

            // Was it ok?
            अगर (!okVisitor.isSimple()) जारी;
            // If the varScope is already removed from logicp, no need to try substitution.
            अगर (!okVisitor.varAssigned(vVtxp->varScp())) जारी;

            // Expression we are considering to substitute with
            Ast_गाँठ_व्यंजक* स्थिर substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            स्थिर माप_प्रकार nReads = okVisitor.readVscps().आकार();

            // Check if it should be inlined
            अगर (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) जारी;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* स्थिर vscp = vVtxp->varScp();

            अगर (debug() >= 9) {
                vscp->dump_वृक्ष("substituting: ");
                substp->dump_वृक्ष("        with: ");
            }

            स्थिर स्वतः& readVscpsVec = okVisitor.readVscps();
            स्थिर std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            क्रम (V3GraphEdge* स्थिर edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* स्थिर dstVtxp = edgep->शीर्ष()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                अगर (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_गाँठ* nodep = substp;
                    अगर (AstNot* स्थिर notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    अगर (!VN_IS(nodep, VarRef)) जारी;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                bool canInline = true;
                क्रम (स्थिर V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    स्थिर GateVarVertex* स्थिर consVVertexp = dedge.शीर्ष()->as<GateVarVertex>();
                    अगर (readVscps.गिनती(consVVertexp->varScp())) {
                        canInline = false;
                        अवरोध;
                    }
                }

                अगर (!canInline) जारी;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                अगर (logicp == dstVtxp->nodep()) {
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
                क्रम (AstVarScope* स्थिर newVscp : okVisitor.readVscps()) {
                    GateVarVertex* स्थिर varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            अगर (vVtxp->outEmpty()) {
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
        optimizeSignals(false);
        // Then propagate more complicated equations
        optimizeSignals(true);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_गाँठ*, माप_प्रकार>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](स्थिर Pair& a, स्थिर Pair& b) {  //
            वापस a.second < b.second;
        });
        क्रम (स्थिर स्वतः& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

खुला:
    अटल व्योम apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

श्रेणी GateDedupeHash final : खुला V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    काष्ठा Aux_Ast_गाँठ_व्यंजक final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_गाँठ_व्यंजक* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_गाँठ_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_गाँठ_व्यंजक, Aux_Ast_गाँठ_व्यंजक> m_aux_गाँठ_व्यंजक;

    Ast_गाँठ_व्यंजक* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_गाँठ_व्यंजक m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    अटल bool same(Ast_गाँठ* node1p, Ast_गाँठ* node2p) {
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
        वापस node1p == node2p || (node1p && node1p->same_वृक्ष(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    bool isSame(Ast_गाँठ* node1p, Ast_गाँठ* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        स्थिर स्वतः& aux2 = m_aux_गाँठ_व्यंजक(VN_AS(node2p, गाँठ_व्यंजक));
        वापस m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

खुला:
    GateDedupeHash() = शेष;
    ~GateDedupeHash() = शेष;

    स्थिर Ast_गाँठ_Assign* hashAndFindDupe(Ast_गाँठ_Assign* assignp, AstActive* activep,
                                         Ast_गाँठ_व्यंजक* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        स्थिर स्वतः it = m_dupFinder.findDuplicate(m_currRhsp, यह);
        अगर (it != m_dupFinder.end()) वापस m_aux_गाँठ_व्यंजक(VN_AS(it->second, गाँठ_व्यंजक)).parentp;

        // Insert new node
        m_dupFinder.जोड़ो(m_currRhsp);
        m_aux_गाँठ_व्यंजक(m_currRhsp) = m_auxCurRhsp;
        वापस nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

श्रेणी GateDedupeVarVisitor final : खुला VNVisitorConst {
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
    Ast_गाँठ_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_गाँठ_व्यंजक* m_ifCondp = nullptr;  // IF condition that assign is under
    bool m_always = false;  // Assign is under an always
    bool m_dedupable = true;  // Determined the assign to be dedupable

    // VISITORS
    व्योम visit(Ast_गाँठ_Assign* nodep) override {
        अगर (!m_dedupable) वापस;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        अगर (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_व्यंजक_Stmt*) { वापस true; });
            वापस;
        }

        m_dedupable = false;
    }

    व्योम visit(AstAlways* nodep) override {
        अगर (!m_dedupable) वापस;

        अगर (!m_always) {
            m_always = true;
            iterate_And_अग्र_Const_Null(nodep->stmtsp());
            वापस;
        }

        m_dedupable = false;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    व्योम visit(Ast_गाँठ_If* nodep) override {
        अगर (!m_dedupable) वापस;

        अगर (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_व्यंजक_Stmt*) { वापस true; });
            iterate_And_अग्र_Const_Null(nodep->thensp());
            वापस;
        }

        m_dedupable = false;
    }

    व्योम visit(AstComment*) override {}  // NOP
    //--------------------
    व्योम visit(Ast_गाँठ*) override { m_dedupable = false; }

खुला:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = शेष;
    ~GateDedupeVarVisitor() override = शेष;
    // PUBLIC METHODS
    Ast_गाँठ_Var_Ref* findDupe(Ast_गाँठ* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = false;
        m_dedupable = true;
        iterateConst(logicp);
        अगर (m_dedupable && m_assignp) {
            स्थिर Ast_गाँठ* स्थिर lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            स्थिर Ast_गाँठ_Var_Ref* स्थिर lRefp = VN_CAST(lhsp, गाँठ_Var_Ref);
            अगर (lRefp && lRefp->varScopep() == consumerVscp) {
                अगर (स्थिर Ast_गाँठ_Assign* स्थिर dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    वापस static_cast<Ast_गाँठ_Var_Ref*>(dup->lhsp());
                }
            }
        }
        वापस nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

श्रेणी GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    स्थिर VNUser2InUse m_inuser2;

    // STATE
    माप_प्रकार m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    व्योम visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        अगर (m_depth > GATE_DEDUP_MAX_DEPTH) वापस;

        // Check that we haven't been here before
        अगर (vVtxp->varScp()->user2SetOnce()) वापस;

        VL_RESTORER(m_depth);
        ++m_depth;
        अगर (!vVtxp->inSize1()) वापस;

        Ast_गाँठ_Var_Ref* dupRefp = nullptr;
        क्रम (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        अगर (!dupRefp) वापस;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* स्थिर lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        स्थिर GateOkVisitor okVisitor{lVtxp->nodep(), false, true};
        अगर (!okVisitor.isSimple()) वापस;

        ++m_statDedupLogic;
        GateVarVertex* स्थिर dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        क्रम (V3GraphEdge* स्थिर edgep : vVtxp->outEdges().unlinkable()) {
            स्थिर GateLogicVertex* स्थिर consumerVtxp = edgep->शीर्ष()->as<GateLogicVertex>();
            Ast_गाँठ* स्थिर consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            अगर (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } अन्यथा {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_गाँठ_Var_Ref* refp) {
                    अगर (refp->varScopep() != vVtxp->varScp()) वापस;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_गाँठ_Var_Ref* स्थिर newp = dupRefp->clone_वृक्ष_Pure(false);
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
        जबतक (V3GraphEdge* स्थिर edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_गाँठ_Var_Ref* visit(GateLogicVertex* lVtxp, स्थिर GateVarVertex* consumerVtxp) {
        क्रम (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        अगर (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            वापस m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        वापस nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        क्रम (V3GraphVertex& vtx : graph.vertices()) {
            अगर (GateVarVertex* स्थिर vVtxp = vtx.cast<GateVarVertex>()) {
                अगर (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        क्रम (V3GraphVertex& vtx : graph.vertices()) {
            अगर (GateVarVertex* स्थिर vVtxp = vtx.cast<GateVarVertex>()) {
                अगर (vVtxp->is_शीर्ष() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

खुला:
    अटल व्योम apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

श्रेणी GateMergeAssignments final {
    GateGraph& m_graph;
    माप_प्रकार m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* पूर्व_Selp, AstSel* currSelp) {
        स्थिर AstVarRef* स्थिर pRefp = VN_CAST(पूर्व_Selp->fromp(), VarRef);
        AstVarRef* स्थिर cRefp = VN_CAST(currSelp->fromp(), VarRef);
        अगर (!pRefp || !cRefp || !cRefp->same_गाँठ(pRefp)) वापस nullptr;  // not the same var

        स्थिर AstConst* स्थिर pstart = VN_CAST(पूर्व_Selp->lsbp(), Const);
        स्थिर AstConst* स्थिर cstart = VN_CAST(currSelp->lsbp(), Const);
        अगर (!pstart || !cstart) वापस nullptr;  // too complicated

        अगर (currSelp->msbConst() + 1 == पूर्व_Selp->lsbConst()) {
            वापस नया AstSel{cRefp->fileline(), cRefp->clone_वृक्ष(false), currSelp->lsbConst(),
                              पूर्व_Selp->widthConst() + currSelp->widthConst()};
        } अन्यथा {
            वापस nullptr;
        }
    }

    व्योम process(GateVarVertex* vVtxp) {
        GateLogicVertex* पूर्व_LVtxp = nullptr;
        AstAssignW* पूर्व_Assignp = nullptr;

        क्रम (V3GraphEdge* स्थिर edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* स्थिर lVtxp = edgep->fromp()->as<GateLogicVertex>();
            अगर (!lVtxp->outSize1()) जारी;

            AstAlways* स्थिर alwaysp = VN_CAST(lVtxp->nodep(), Always);
            अगर (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) वापस;
            AstAssignW* स्थिर assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            अगर (!assignp) जारी;

            अगर (!VN_IS(assignp->lhsp(), Sel)) जारी;

            // First assign with Sel-lhs, or not under the same active
            अगर (!पूर्व_LVtxp || पूर्व_LVtxp->activep() != lVtxp->activep()) {
                पूर्व_LVtxp = lVtxp;
                पूर्व_Assignp = assignp;
                जारी;
            }

            AstSel* स्थिर पूर्व_Selp = VN_AS(पूर्व_Assignp->lhsp(), Sel);
            AstSel* स्थिर currSelp = VN_AS(assignp->lhsp(), Sel);

            अगर (AstSel* स्थिर newSelp = merge(पूर्व_Selp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                पूर्व_Selp->replaceWith(newSelp);
                VL_DO_DANGLING(पूर्व_Selp->delete_वृक्ष(), पूर्व_Selp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                पूर्व_Assignp->rhsp(नया AstConcat{पूर्व_Assignp->rhsp()->fileline(),
                                                पूर्व_Assignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                पूर्व_Assignp->dtypeChgWidthSigned(पूर्व_Assignp->width() + assignp->width(),
                                                 पूर्व_Assignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                जबतक (V3GraphEdge* स्थिर iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* स्थिर fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, पूर्व_LVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } अन्यथा {
                पूर्व_LVtxp = lVtxp;
                पूर्व_Assignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        क्रम (V3GraphVertex& vtx : graph.vertices()) {
            अगर (GateVarVertex* स्थिर vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        क्रम (GateLogicVertex* स्थिर lVtxp : m_toRemove) {
            Ast_गाँठ* स्थिर nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

खुला:
    अटल व्योम apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

श्रेणी GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    व्योम markRecurse(GateEitherVertex* vtxp) {
        अगर (vtxp->user()) वापस;  // Already marked
        vtxp->user(true);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        क्रम (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* स्थिर fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    व्योम mark() {
        m_graph.userClearVertices();
        क्रम (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            अगर (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    अटल व्योम warnUnused(स्थिर Ast_गाँठ* स्थिर nodep) {
        अगर (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) वापस;

        अगर (स्थिर Ast_गाँठ_Procedure* स्थिर procedurep = VN_CAST(nodep, गाँठ_Procedure)) {
            अगर (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](स्थिर AstLoop* स्थिर loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, true);
                });
        }
    }

    // Remove unused logic
    व्योम हटाओ() {
        क्रम (V3GraphVertex* स्थिर vtxp : m_graph.vertices().unlinkable()) {
            अगर (GateLogicVertex* स्थिर lVtxp = vtxp->cast<GateLogicVertex>()) {
                अगर (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_गाँठ* स्थिर nodep = lVtxp->nodep();
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

खुला:
    अटल व्योम apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

व्योम V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        अगर (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->remove_Redundant_Edges_योग(&V3GraphEdge::followAlwaysTrue);
        अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        अगर (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        अगर (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        अगर (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        अगर (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_वृक्ष("gate", 0, dump_वृक्ष_Either_Level() >= 3);
}
