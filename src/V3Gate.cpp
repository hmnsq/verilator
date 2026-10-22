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
    منطقي m_reducible = true;  // True if this node should be able to be eliminated
    منطقي m_dedupable = true;  // True if this node should be able to be deduped
    منطقي m_consumed = خطأ;  // Output goes to something meaningful
    منطقي m_staticInit = خطأ;  // True if this node is a static initializer
عام:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = افتراضي;

    // ACCESSORS
    منطقي reducible() ثابت { إرجاع m_reducible; }
    منطقي dedupable() ثابت { إرجاع m_dedupable; }
    منطقي consumed() ثابت { إرجاع m_consumed; }
    منطقي staticInit() ثابت { إرجاع m_staticInit; }
    فراغ setConsumed(ثابت حرف* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = true;
    }
    فراغ setStaticInit() { m_staticInit = true; }
    فراغ clearReducible(ثابت حرف* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = خطأ;
    }
    فراغ clearDedupable(ثابت حرف* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = خطأ;
    }
    فراغ clearReducibleAndDedupable(ثابت حرف* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ثابت override { إرجاع m_consumed ? "" : "dotted"; }
};

صنف GateVarVertex final : عام GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* ثابت m_varScp;
    منطقي m_is_قمة = خطأ;
    منطقي m_isClock = خطأ;
    Ast_عقدة* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_عقدة* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
عام:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = افتراضي;

    // ACCESSORS
    AstVarScope* varScp() ثابت VL_MT_STABLE { إرجاع m_varScp; }
    منطقي is_قمة() ثابت { إرجاع m_is_قمة; }
    فراغ set_Is_قمة() { m_is_قمة = true; }
    منطقي isClock() ثابت { إرجاع m_isClock; }
    فراغ setIsClock() { m_isClock = true; }
    Ast_عقدة* rstSyncNodep() ثابت { إرجاع m_rstSyncNodep; }
    فراغ rstSyncNodep(Ast_عقدة* nodep) { m_rstSyncNodep = nodep; }
    Ast_عقدة* rstAsyncNodep() ثابت { إرجاع m_rstAsyncNodep; }
    فراغ rstAsyncNodep(Ast_عقدة* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    فراغ propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        إذا (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() ثابت override VL_MT_STABLE { إرجاع varScp()->name(); }
    std::string dotColor() ثابت override { إرجاع "green"; }
};

صنف GateLogicVertex final : عام GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_عقدة* ثابت m_nodep;
    AstActive* ثابت m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ثابت منطقي m_slow;  // In slow block
عام:
    GateLogicVertex(V3Graph* graphp, Ast_عقدة* nodep, AstActive* activep, منطقي slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = افتراضي;

    // ACCESSORS
    File_سطر* fileline() ثابت override { إرجاع nodep()->fileline(); }
    Ast_عقدة* nodep() ثابت { إرجاع m_nodep; }
    AstActive* activep() ثابت { إرجاع m_activep; }
    منطقي slow() ثابت { إرجاع m_slow; }

    // DOT debug
    std::string name() ثابت override { إرجاع m_nodep->fileline()->ascii(); }
    std::string dotColor() ثابت override { إرجاع "red"; }
};

صنف GateEdge final : عام V3GraphEdge {
    std::string dotLabel() ثابت override { إرجاع std::to_string(weight()); }

عام:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* قمة, صحيح weight)
        : V3GraphEdge{graphp, fromp, قمة, weight} {}
};

صنف GateGraph final : عام V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ثابت VNUser1InUse m_inuser1;

عام:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        إذا (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = جديد GateVarVertex{هذا, vscp};
            vscp->user1p(vVtxp);
            إذا (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            إذا (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            إذا (vscp->varp()->isIO() && vscp->scopep()->is_قمة()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_قمة();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        إرجاع vVtxp;
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
    ثابت Ast_عقدة_Module* m_modp = nullptr;  // Current module
    ثابت AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    منطقي m_inClockedActive = خطأ;  // Underneath clocked active
    منطقي m_inEdgeActive = خطأ;  // Underneath edge active
    منطقي m_inStaticActive = خطأ;  // Underneath static active
    منطقي m_inInitialActive = خطأ;  // Underneath initial active
    منطقي m_inSenItem = خطأ;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    فراغ check_عقدة(Ast_عقدة* nodep) {
        إذا (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        إذا (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    فراغ iterateLogic(Ast_عقدة* nodep, منطقي slow = خطأ, ثابت حرف* nonReducibleReason = nullptr,
                      ثابت حرف* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = جديد GateLogicVertex{m_graphp, nodep, m_activep, slow};
        إذا (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } وإلا إذا (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } وإلا إذا (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        إذا (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_عقدة(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Module* nodep) override {
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
    فراغ visit(Ast_عقدة_Procedure* nodep) override {
        ثابت منطقي slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    فراغ visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, خطأ, "CoverToggle", "CoverToggle");
    }
    فراغ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = true;
        إذا (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } وإلا {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, خطأ, nullptr, "senItem");
        }
    }
    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        إذا (!m_logicVertexp) إرجاع;

        AstVarScope* ثابت vscp = nodep->varScopep();
        GateVarVertex* ثابت vVtxp = m_graphp->makeVarVertex(vscp);

        إذا (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(true);
        } وإلا إذا (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            إذا (vscp->user2()) {
                إذا (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } وإلا {
                إذا (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        إذا (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        إذا (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    فراغ visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), عقدة_Assign)
                      && VN_AS(nodep->backp(), عقدة_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    فراغ visit(Ast_عقدة* nodep) override {
        إذا (m_logicVertexp) check_عقدة(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

عام:
    ساكن std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        إرجاع std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ساكن فراغ v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ثابت VNUser2InUse user2InUse;
    لكل (V3GraphVertex& vtx : graph.vertices()) {
        إذا (ثابت GateVarVertex* ثابت vvertexp = vtx.cast<GateVarVertex>()) {
            ثابت AstVarScope* ثابت vscp = vvertexp->varScp();
            ثابت Ast_عقدة* ثابت sp = vvertexp->rstSyncNodep();
            ثابت Ast_عقدة* ثابت ap = vvertexp->rstAsyncNodep();
            إذا (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                إذا (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
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

صنف GateConcatVisitor final : عام VNVisitorConst {
    // STATE
    ثابت AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    صحيح m_offset = 0;  // Current offset of varscope
    صحيح m_found_offset = 0;  // Found offset of varscope
    منطقي m_found = خطأ;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        إذا (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(true);
            m_found_offset = m_offset;
            m_found = true;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    فراغ visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    فراغ visit(Ast_عقدة* nodep) override { iterateChildrenConst(nodep); }

عام:
    // PUBLIC METHODS
    منطقي concatOffset(AstConcat* concatp, AstVarScope* vscp, صحيح& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = خطأ;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        إرجاع m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

صنف GateOkVisitor final : عام VNVisitorConst {
    // RETURN STATE
    منطقي m_isSimple = true;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_عقدة_تعبير* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    ثابت منطقي m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ثابت منطقي m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ثابت Ast_عقدة_Var_Ref* m_lhsVarRef = nullptr;
    صحيح m_ops = 0;  // Operation count

    // METHODS
    فراغ clearSimple(ثابت حرف* because) {
        إذا (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = خطأ;
    }

    // VISITORS
    فراغ visit(Ast_عقدة_Var_Ref* nodep) override {
        إذا (!m_isSimple) إرجاع;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        إذا (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        إذا (nodep->access().isRW()) {
            clearSimple("R/W");
            إرجاع;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ثابت vscp = nodep->varScopep();
        إذا (nodep->access().isWriteOnly()) {
            إذا (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } وإلا {
            // TODO: possible bug, should it be >= 1 as add is below?
            إذا (m_readVscps.size() > 1) {
                إذا (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ادفع_back(vscp);
        }
    }
    فراغ visit(Ast_عقدة_Assign* nodep) override {
        إذا (!m_isSimple) إرجاع;

        m_substitutionp = nodep->rhsp();
        إذا (!VN_IS(nodep->lhsp(), عقدة_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } وإلا إذا (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } وإلا {
            iterateChildrenConst(nodep);
        }
        إذا (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    فراغ visit(AstCReset* /*nodep*/) override {
        إذا (!m_isSimple) إرجاع;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    فراغ visit(AstMatchMasked* nodep) override {
        إذا (!m_isSimple) إرجاع;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    فراغ visit(Ast_عقدة* nodep) override {
        إذا (!m_isSimple) إرجاع;  // Fastpath

        إذا (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            إرجاع;
        }

        إذا (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            إرجاع;
        }

        iterateChildrenConst(nodep);
    }

عام:
    // CONSTRUCTORS
    GateOkVisitor(Ast_عقدة* nodep, منطقي buffersOnly, منطقي dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        إذا (!m_substitutionp) {
            clearSimple("No assignment found\n");
            إرجاع;
        }
        إذا (m_isSimple && m_lhsVarRef) {
            لكل (ثابت AstVarScope* ثابت vscp : m_readVscps) {
                إذا (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    إرجاع;
                }
            }
        }
    }
    ~GateOkVisitor() override = افتراضي;
    // PUBLIC METHODS
    منطقي isSimple() ثابت { إرجاع m_isSimple; }
    Ast_عقدة_تعبير* substitutionp() ثابت {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        إرجاع m_substitutionp;
    }
    ثابت std::vector<AstVarScope*>& readVscps() ثابت { إرجاع m_readVscps; }
    منطقي varAssigned(ثابت AstVarScope* scopep) ثابت {
        إرجاع m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

صنف GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_عقدة_تعبير*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ثابت VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_عقدة, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_عقدة*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    ساكن منطقي isCheap(ثابت Ast_عقدة_تعبير* exprp) {
        // Constant is cheap
        إذا (VN_IS(exprp, Const)) إرجاع true;
        // Variable reference is cheap
        إذا (VN_IS(exprp, عقدة_Var_Ref)) إرجاع true;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        إذا (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            إذا (!isCheap(selp->fromp())) إرجاع خطأ;
            إذا (!selp->isWide()) إرجاع true;
            إذا (!VN_IS(selp->lsbp(), Const)) إرجاع خطأ;
            إذا (selp->lsbConst() % VL_EDATASIZE != 0) إرجاع خطأ;
            إرجاع true;
        }
        // AstArraySel is cheap if the fromp is cheap
        إذا (ثابت AstArraySel* ثابت aselp = VN_CAST(exprp, ArraySel)) {
            إرجاع isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        إرجاع خطأ;
    }

    منطقي shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_عقدة_تعبير* substp, منطقي allowMultiIn) {
        // Always inline constants
        إذا (VN_IS(substp, Const)) إرجاع true;
        // Don't inline non-constant static initializers - these are scheduled differently
        إذا (lVtxp->staticInit()) إرجاع خطأ;
        // Inline simple variable references
        إذا (VN_IS(substp, VarRef)) إرجاع true;
        // Only inline arrays if a simple variable or constant
        إذا (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) إرجاع خطأ;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        إذا (nReads == 0) إرجاع true;
        // If it reads one variable, inline if not wide, or if cheap
        إذا (nReads == 1 && (!substp->isWide() || isCheap(substp))) إرجاع true;
        // Don't inline on first round if reads more than one variable
        إذا (nReads > 1 && !allowMultiIn) إرجاع خطأ;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        صحيح n = 0;
        لكل (V3GraphEdge& edge : vVtxp->outEdges()) {
            ثابت GateLogicVertex* ثابت dstVtxp = edge.قمة()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            إذا (dstVtxp->slow()) استمر;
            إذا (dstVtxp->outEmpty() && !dstVtxp->consumed()) استمر;
            n += edge.weight();
            إذا (n > 1) {
                ++m_statNotInlined;
                إرجاع خطأ;
            }
        }
        إرجاع true;
    }

    فراغ recordSubstitution(AstVarScope* vscp, Ast_عقدة_تعبير* substp, Ast_عقدة* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ثابت auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        إذا (pair.second) pair.first->second = substp->clone_شجرة_Pure(خطأ);
    }

    فراغ commitSubstitutions(Ast_عقدة* logicp) {
        إذا (!m_hasPending.erase(logicp)) إرجاع;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<ثابت AstVarScope*> replaced(substitutions.size() * 2);

        ثابت std::function<فراغ(Ast_عقدة_Var_Ref*)> visit = [&](Ast_عقدة_Var_Ref* nodep) -> فراغ {
            // See if this variable has a substitution
            AstVarScope* ثابت vscp = nodep->varScopep();
            ثابت auto& it = substitutions.find(vscp);
            إذا (it == substitutions.end()) إرجاع;

            // Do not substitute circular logic
            إذا (!replaced.أدرج(vscp).second) إرجاع;

            // Substitute nodep with substp
            Ast_عقدة_تعبير* ثابت substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, عقدة_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_عقدة_تعبير* ثابت newp = substp->clone_شجرة_Pure(خطأ);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            إذا (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            إذا (Ast_عقدة_Var_Ref* ثابت varrefp = VN_CAST(newp, عقدة_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_عقدة* ثابت simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        لكل (ثابت auto& pair : substitutions) pair.second->delete_شجرة();
        substitutions.clear();
    }

    فراغ optimizeSignals(منطقي allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        ثابت auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            بينما (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            إرجاع it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        بينما (vIt != vertices.end()) {
            GateVarVertex* ثابت vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            إذا (!vVtxp->inSize1()) استمر;

            // Can't inline if non-reducible, etc
            إذا (!vVtxp->reducible()) استمر;

            // Grab the driving logic
            GateLogicVertex* ثابت lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            إذا (!lVtxp->reducible()) استمر;
            Ast_عقدة* ثابت logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ثابت GateOkVisitor okVisitor{logicp, خطأ, خطأ};

            // Was it ok?
            إذا (!okVisitor.isSimple()) استمر;
            // If the varScope is already removed from logicp, no need to try substitution.
            إذا (!okVisitor.varAssigned(vVtxp->varScp())) استمر;

            // Expression we are considering to substitute with
            Ast_عقدة_تعبير* ثابت substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ثابت size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            إذا (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) استمر;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ثابت vscp = vVtxp->varScp();

            إذا (debug() >= 9) {
                vscp->dump_شجرة("substituting: ");
                substp->dump_شجرة("        with: ");
            }

            ثابت auto& readVscpsVec = okVisitor.readVscps();
            ثابت std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            لكل (V3GraphEdge* ثابت edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* ثابت dstVtxp = edgep->قمة()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                إذا (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_عقدة* nodep = substp;
                    إذا (AstNot* ثابت notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    إذا (!VN_IS(nodep, VarRef)) استمر;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                منطقي canInline = true;
                لكل (ثابت V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    ثابت GateVarVertex* ثابت consVVertexp = dedge.قمة()->as<GateVarVertex>();
                    إذا (readVscps.تعداد_2(consVVertexp->varScp())) {
                        canInline = خطأ;
                        اكسر;
                    }
                }

                إذا (!canInline) استمر;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                إذا (logicp == dstVtxp->nodep()) {
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
            إذا (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_شجرة(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(خطأ);
        // Then propagate more complicated equations
        optimizeSignals(true);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_عقدة*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](ثابت Pair& a, ثابت Pair& b) {  //
            إرجاع a.second < b.second;
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
    بنية Aux_Ast_عقدة_تعبير final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_عقدة_تعبير* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_عقدة_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_عقدة_تعبير, Aux_Ast_عقدة_تعبير> m_aux_عقدة_تعبير;

    Ast_عقدة_تعبير* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_عقدة_تعبير m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    ساكن منطقي same(Ast_عقدة* node1p, Ast_عقدة* node2p) {
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
        إرجاع node1p == node2p || (node1p && node1p->same_شجرة(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    منطقي isSame(Ast_عقدة* node1p, Ast_عقدة* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ثابت auto& aux2 = m_aux_عقدة_تعبير(VN_AS(node2p, عقدة_تعبير));
        إرجاع m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

عام:
    GateDedupeHash() = افتراضي;
    ~GateDedupeHash() = افتراضي;

    ثابت Ast_عقدة_Assign* hashAndFindDupe(Ast_عقدة_Assign* assignp, AstActive* activep,
                                         Ast_عقدة_تعبير* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ثابت auto it = m_dupFinder.findDuplicate(m_currRhsp, هذا);
        إذا (it != m_dupFinder.end()) إرجاع m_aux_عقدة_تعبير(VN_AS(it->second, عقدة_تعبير)).parentp;

        // Insert new node
        m_dupFinder.أدرج(m_currRhsp);
        m_aux_عقدة_تعبير(m_currRhsp) = m_auxCurRhsp;
        إرجاع nullptr;
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
    Ast_عقدة_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_عقدة_تعبير* m_ifCondp = nullptr;  // IF condition that assign is under
    منطقي m_always = خطأ;  // Assign is under an always
    منطقي m_dedupable = true;  // Determined the assign to be dedupable

    // VISITORS
    فراغ visit(Ast_عقدة_Assign* nodep) override {
        إذا (!m_dedupable) إرجاع;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        إذا (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_تعبير_Stmt*) { إرجاع true; });
            إرجاع;
        }

        m_dedupable = خطأ;
    }

    فراغ visit(AstAlways* nodep) override {
        إذا (!m_dedupable) إرجاع;

        إذا (!m_always) {
            m_always = true;
            iterateAndNextConstNull(nodep->stmtsp());
            إرجاع;
        }

        m_dedupable = خطأ;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    فراغ visit(Ast_عقدة_If* nodep) override {
        إذا (!m_dedupable) إرجاع;

        إذا (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_تعبير_Stmt*) { إرجاع true; });
            iterateAndNextConstNull(nodep->thensp());
            إرجاع;
        }

        m_dedupable = خطأ;
    }

    فراغ visit(AstComment*) override {}  // NOP
    //--------------------
    فراغ visit(Ast_عقدة*) override { m_dedupable = خطأ; }

عام:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = افتراضي;
    ~GateDedupeVarVisitor() override = افتراضي;
    // PUBLIC METHODS
    Ast_عقدة_Var_Ref* findDupe(Ast_عقدة* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = خطأ;
        m_dedupable = true;
        iterateConst(logicp);
        إذا (m_dedupable && m_assignp) {
            ثابت Ast_عقدة* ثابت lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ثابت Ast_عقدة_Var_Ref* ثابت lRefp = VN_CAST(lhsp, عقدة_Var_Ref);
            إذا (lRefp && lRefp->varScopep() == consumerVscp) {
                إذا (ثابت Ast_عقدة_Assign* ثابت dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    إرجاع static_cast<Ast_عقدة_Var_Ref*>(dup->lhsp());
                }
            }
        }
        إرجاع nullptr;
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
        إذا (m_depth > GATE_DEDUP_MAX_DEPTH) إرجاع;

        // Check that we haven't been here before
        إذا (vVtxp->varScp()->user2SetOnce()) إرجاع;

        VL_RESTORER(m_depth);
        ++m_depth;
        إذا (!vVtxp->inSize1()) إرجاع;

        Ast_عقدة_Var_Ref* dupRefp = nullptr;
        لكل (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        إذا (!dupRefp) إرجاع;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* ثابت lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        ثابت GateOkVisitor okVisitor{lVtxp->nodep(), خطأ, true};
        إذا (!okVisitor.isSimple()) إرجاع;

        ++m_statDedupLogic;
        GateVarVertex* ثابت dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        لكل (V3GraphEdge* ثابت edgep : vVtxp->outEdges().unlinkable()) {
            ثابت GateLogicVertex* ثابت consumerVtxp = edgep->قمة()->as<GateLogicVertex>();
            Ast_عقدة* ثابت consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            إذا (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } وإلا {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_عقدة_Var_Ref* refp) {
                    إذا (refp->varScopep() != vVtxp->varScp()) إرجاع;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_عقدة_Var_Ref* ثابت newp = dupRefp->clone_شجرة_Pure(خطأ);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_شجرة(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        بينما (V3GraphEdge* ثابت edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_عقدة_Var_Ref* visit(GateLogicVertex* lVtxp, ثابت GateVarVertex* consumerVtxp) {
        لكل (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        إذا (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            إرجاع m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        إرجاع nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        لكل (V3GraphVertex& vtx : graph.vertices()) {
            إذا (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) {
                إذا (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        لكل (V3GraphVertex& vtx : graph.vertices()) {
            إذا (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) {
                إذا (vVtxp->is_قمة() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
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
        إذا (!pRefp || !cRefp || !cRefp->same_عقدة(pRefp)) إرجاع nullptr;  // not the same var

        ثابت AstConst* ثابت pstart = VN_CAST(prevSelp->lsbp(), Const);
        ثابت AstConst* ثابت cstart = VN_CAST(currSelp->lsbp(), Const);
        إذا (!pstart || !cstart) إرجاع nullptr;  // too complicated

        إذا (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            إرجاع جديد AstSel{cRefp->fileline(), cRefp->clone_شجرة(خطأ), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } وإلا {
            إرجاع nullptr;
        }
    }

    فراغ process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        لكل (V3GraphEdge* ثابت edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* ثابت lVtxp = edgep->fromp()->as<GateLogicVertex>();
            إذا (!lVtxp->outSize1()) استمر;

            AstAlways* ثابت alwaysp = VN_CAST(lVtxp->nodep(), Always);
            إذا (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) إرجاع;
            AstAssignW* ثابت assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            إذا (!assignp) استمر;

            إذا (!VN_IS(assignp->lhsp(), Sel)) استمر;

            // First assign with Sel-lhs, or not under the same active
            إذا (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                استمر;
            }

            AstSel* ثابت prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ثابت currSelp = VN_AS(assignp->lhsp(), Sel);

            إذا (AstSel* ثابت newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_شجرة(), prevSelp);
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
                بينما (V3GraphEdge* ثابت iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* ثابت fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } وإلا {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        لكل (V3GraphVertex& vtx : graph.vertices()) {
            إذا (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        لكل (GateLogicVertex* ثابت lVtxp : m_toRemove) {
            Ast_عقدة* ثابت nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_شجرة(), nodep);
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
        إذا (vtxp->user()) إرجاع;  // Already marked
        vtxp->user(true);
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
            إذا (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ساكن فراغ warnUnused(ثابت Ast_عقدة* ثابت nodep) {
        إذا (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) إرجاع;

        إذا (ثابت Ast_عقدة_Procedure* ثابت procedurep = VN_CAST(nodep, عقدة_Procedure)) {
            إذا (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ثابت AstLoop* ثابت loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, true);
                });
        }
    }

    // Remove unused logic
    فراغ remove() {
        لكل (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            إذا (GateLogicVertex* ثابت lVtxp = vtxp->cast<GateLogicVertex>()) {
                إذا (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_عقدة* ثابت nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
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
        إذا (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        إذا (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        إذا (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        إذا (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        إذا (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_شجرة("gate", 0, dump_شجرة_Either_Level() >= 3);
}
