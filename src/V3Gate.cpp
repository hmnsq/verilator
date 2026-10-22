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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Gate.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3Const.h"
#شامل "V3DupFinder.h"
#شامل "V3Graph.h"
#شامل "V3Stats.h"

#شامل <list>
#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

رده GateEitherVertex VL_NOT_FINAL : عمومی V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    منطقی m_reducible = درست;  // True if this node should be able to be eliminated
    منطقی m_dedupable = درست;  // True if this node should be able to be deduped
    منطقی m_consumed = نادرست;  // Output goes to something meaningful
    منطقی m_staticInit = نادرست;  // True if this node is a static initializer
عمومی:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = پیشفرض;

    // ACCESSORS
    منطقی reducible() ثابت { بازگشت m_reducible; }
    منطقی dedupable() ثابت { بازگشت m_dedupable; }
    منطقی consumed() ثابت { بازگشت m_consumed; }
    منطقی staticInit() ثابت { بازگشت m_staticInit; }
    تهی setConsumed(ثابت نویسه* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = درست;
    }
    تهی setStaticInit() { m_staticInit = درست; }
    تهی clearReducible(ثابت نویسه* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = نادرست;
    }
    تهی clearDedupable(ثابت نویسه* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = نادرست;
    }
    تهی clearReducibleAndDedupable(ثابت نویسه* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ثابت override { بازگشت m_consumed ? "" : "dotted"; }
};

رده GateVarVertex final : عمومی GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* ثابت m_varScp;
    منطقی m_is_بالاترین = نادرست;
    منطقی m_isClock = نادرست;
    Ast_گره* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_گره* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
عمومی:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = پیشفرض;

    // ACCESSORS
    AstVarScope* varScp() ثابت VL_MT_STABLE { بازگشت m_varScp; }
    منطقی is_بالاترین() ثابت { بازگشت m_is_بالاترین; }
    تهی set_Is_بالاترین() { m_is_بالاترین = درست; }
    منطقی isClock() ثابت { بازگشت m_isClock; }
    تهی setIsClock() { m_isClock = درست; }
    Ast_گره* rstSyncNodep() ثابت { بازگشت m_rstSyncNodep; }
    تهی rstSyncNodep(Ast_گره* nodep) { m_rstSyncNodep = nodep; }
    Ast_گره* rstAsyncNodep() ثابت { بازگشت m_rstAsyncNodep; }
    تهی rstAsyncNodep(Ast_گره* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    تهی propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        اگر (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() ثابت override VL_MT_STABLE { بازگشت varScp()->name(); }
    std::string dotColor() ثابت override { بازگشت "green"; }
};

رده GateLogicVertex final : عمومی GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_گره* ثابت m_nodep;
    AstActive* ثابت m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ثابت منطقی m_slow;  // In slow block
عمومی:
    GateLogicVertex(V3Graph* graphp, Ast_گره* nodep, AstActive* activep, منطقی slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = پیشفرض;

    // ACCESSORS
    File_سطر* fileline() ثابت override { بازگشت nodep()->fileline(); }
    Ast_گره* nodep() ثابت { بازگشت m_nodep; }
    AstActive* activep() ثابت { بازگشت m_activep; }
    منطقی slow() ثابت { بازگشت m_slow; }

    // DOT debug
    std::string name() ثابت override { بازگشت m_nodep->fileline()->ascii(); }
    std::string dotColor() ثابت override { بازگشت "red"; }
};

رده GateEdge final : عمومی V3GraphEdge {
    std::string dotLabel() ثابت override { بازگشت std::to_string(weight()); }

عمومی:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight)
        : V3GraphEdge{graphp, fromp, بالاترین, weight} {}
};

رده GateGraph final : عمومی V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ثابت VNUser1InUse m_inuser1;

عمومی:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        اگر (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = جدید GateVarVertex{این, vscp};
            vscp->user1p(vVtxp);
            اگر (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            اگر (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            اگر (vscp->varp()->isIO() && vscp->scopep()->is_بالاترین()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_بالاترین();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        بازگشت vVtxp;
    }

    تهی addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, صحیح weight) {
        جدید GateEdge{این, srcp, dstp, weight};
    }

    تهی addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, صحیح weight) {
        جدید GateEdge{این, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

رده GateBuildVisitor final : عمومی VNVisitorConst {
    // STATE
    GateGraph* m_graphp = جدید GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    ثابت Ast_گره_Module* m_modp = nullptr;  // Current module
    ثابت AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    منطقی m_inClockedActive = نادرست;  // Underneath clocked active
    منطقی m_inEdgeActive = نادرست;  // Underneath edge active
    منطقی m_inStaticActive = نادرست;  // Underneath static active
    منطقی m_inInitialActive = نادرست;  // Underneath initial active
    منطقی m_inSenItem = نادرست;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    تهی check_گره(Ast_گره* nodep) {
        اگر (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        اگر (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    تهی iterateLogic(Ast_گره* nodep, منطقی slow = نادرست, ثابت نویسه* nonReducibleReason = nullptr,
                      ثابت نویسه* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = جدید GateLogicVertex{m_graphp, nodep, m_activep, slow};
        اگر (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } وگرنه اگر (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } وگرنه اگر (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        اگر (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_گره(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    تهی visit(AstActive* nodep) override {
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

    تهی visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    تهی visit(Ast_گره_Procedure* nodep) override {
        ثابت منطقی slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    تهی visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, نادرست, "CoverToggle", "CoverToggle");
    }
    تهی visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = درست;
        اگر (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } وگرنه {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, نادرست, nullptr, "senItem");
        }
    }
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (!m_logicVertexp) بازگشت;

        AstVarScope* ثابت vscp = nodep->varScopep();
        GateVarVertex* ثابت vVtxp = m_graphp->makeVarVertex(vscp);

        اگر (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(درست);
        } وگرنه اگر (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            اگر (vscp->user2()) {
                اگر (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } وگرنه {
                اگر (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        اگر (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        اگر (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    تهی visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), گره_Assign)
                      && VN_AS(nodep->backp(), گره_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    تهی visit(Ast_گره* nodep) override {
        اگر (m_logicVertexp) check_گره(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

عمومی:
    ایستا std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        بازگشت std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ایستا تهی v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ثابت VNUser2InUse user2InUse;
    برای (V3GraphVertex& vtx : graph.vertices()) {
        اگر (ثابت GateVarVertex* ثابت vvertexp = vtx.cast<GateVarVertex>()) {
            ثابت AstVarScope* ثابت vscp = vvertexp->varScp();
            ثابت Ast_گره* ثابت sp = vvertexp->rstSyncNodep();
            ثابت Ast_گره* ثابت ap = vvertexp->rstAsyncNodep();
            اگر (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                اگر (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(درست);  // Warn only once per signal
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

رده GateConcatVisitor final : عمومی VNVisitorConst {
    // STATE
    ثابت AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    صحیح m_offset = 0;  // Current offset of varscope
    صحیح m_found_offset = 0;  // Found offset of varscope
    منطقی m_found = نادرست;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(درست);
            m_found_offset = m_offset;
            m_found = درست;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    تهی visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // PUBLIC METHODS
    منطقی concatOffset(AstConcat* concatp, AstVarScope* vscp, صحیح& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = نادرست;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        بازگشت m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

رده GateOkVisitor final : عمومی VNVisitorConst {
    // RETURN STATE
    منطقی m_isSimple = درست;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_گره_عبارت* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    ثابت منطقی m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ثابت منطقی m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ثابت Ast_گره_Var_Ref* m_lhsVarRef = nullptr;
    صحیح m_ops = 0;  // Operation count

    // METHODS
    تهی clearSimple(ثابت نویسه* because) {
        اگر (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = نادرست;
    }

    // VISITORS
    تهی visit(Ast_گره_Var_Ref* nodep) override {
        اگر (!m_isSimple) بازگشت;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        اگر (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        اگر (nodep->access().isRW()) {
            clearSimple("R/W");
            بازگشت;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ثابت vscp = nodep->varScopep();
        اگر (nodep->access().isWriteOnly()) {
            اگر (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } وگرنه {
            // TODO: possible bug, should it be >= 1 as add is below?
            اگر (m_readVscps.size() > 1) {
                اگر (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.افزودن_back(vscp);
        }
    }
    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (!m_isSimple) بازگشت;

        m_substitutionp = nodep->rhsp();
        اگر (!VN_IS(nodep->lhsp(), گره_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } وگرنه اگر (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } وگرنه {
            iterateChildrenConst(nodep);
        }
        اگر (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    تهی visit(AstCReset* /*nodep*/) override {
        اگر (!m_isSimple) بازگشت;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    تهی visit(AstMatchMasked* nodep) override {
        اگر (!m_isSimple) بازگشت;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    تهی visit(Ast_گره* nodep) override {
        اگر (!m_isSimple) بازگشت;  // Fastpath

        اگر (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            بازگشت;
        }

        اگر (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            بازگشت;
        }

        iterateChildrenConst(nodep);
    }

عمومی:
    // CONSTRUCTORS
    GateOkVisitor(Ast_گره* nodep, منطقی buffersOnly, منطقی dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        اگر (!m_substitutionp) {
            clearSimple("No assignment found\n");
            بازگشت;
        }
        اگر (m_isSimple && m_lhsVarRef) {
            برای (ثابت AstVarScope* ثابت vscp : m_readVscps) {
                اگر (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    بازگشت;
                }
            }
        }
    }
    ~GateOkVisitor() override = پیشفرض;
    // PUBLIC METHODS
    منطقی isSimple() ثابت { بازگشت m_isSimple; }
    Ast_گره_عبارت* substitutionp() ثابت {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        بازگشت m_substitutionp;
    }
    ثابت std::vector<AstVarScope*>& readVscps() ثابت { بازگشت m_readVscps; }
    منطقی varAssigned(ثابت AstVarScope* scopep) ثابت {
        بازگشت m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

رده GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_گره_عبارت*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ثابت VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_گره, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_گره*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    ایستا منطقی isCheap(ثابت Ast_گره_عبارت* exprp) {
        // Constant is cheap
        اگر (VN_IS(exprp, Const)) بازگشت درست;
        // Variable reference is cheap
        اگر (VN_IS(exprp, گره_Var_Ref)) بازگشت درست;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        اگر (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            اگر (!isCheap(selp->fromp())) بازگشت نادرست;
            اگر (!selp->isWide()) بازگشت درست;
            اگر (!VN_IS(selp->lsbp(), Const)) بازگشت نادرست;
            اگر (selp->lsbConst() % VL_EDATASIZE != 0) بازگشت نادرست;
            بازگشت درست;
        }
        // AstArraySel is cheap if the fromp is cheap
        اگر (ثابت AstArraySel* ثابت aselp = VN_CAST(exprp, ArraySel)) {
            بازگشت isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        بازگشت نادرست;
    }

    منطقی shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_گره_عبارت* substp, منطقی allowMultiIn) {
        // Always inline constants
        اگر (VN_IS(substp, Const)) بازگشت درست;
        // Don't inline non-constant static initializers - these are scheduled differently
        اگر (lVtxp->staticInit()) بازگشت نادرست;
        // Inline simple variable references
        اگر (VN_IS(substp, VarRef)) بازگشت درست;
        // Only inline arrays if a simple variable or constant
        اگر (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) بازگشت نادرست;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        اگر (nReads == 0) بازگشت درست;
        // If it reads one variable, inline if not wide, or if cheap
        اگر (nReads == 1 && (!substp->isWide() || isCheap(substp))) بازگشت درست;
        // Don't inline on first round if reads more than one variable
        اگر (nReads > 1 && !allowMultiIn) بازگشت نادرست;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        صحیح n = 0;
        برای (V3GraphEdge& edge : vVtxp->outEdges()) {
            ثابت GateLogicVertex* ثابت dstVtxp = edge.بالاترین()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            اگر (dstVtxp->slow()) ادامه;
            اگر (dstVtxp->outEmpty() && !dstVtxp->consumed()) ادامه;
            n += edge.weight();
            اگر (n > 1) {
                ++m_statNotInlined;
                بازگشت نادرست;
            }
        }
        بازگشت درست;
    }

    تهی recordSubstitution(AstVarScope* vscp, Ast_گره_عبارت* substp, Ast_گره* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ثابت auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        اگر (pair.second) pair.first->second = substp->clone_درخت_Pure(نادرست);
    }

    تهی commitSubstitutions(Ast_گره* logicp) {
        اگر (!m_hasPending.erase(logicp)) بازگشت;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<ثابت AstVarScope*> replaced(substitutions.size() * 2);

        ثابت std::function<تهی(Ast_گره_Var_Ref*)> visit = [&](Ast_گره_Var_Ref* nodep) -> تهی {
            // See if this variable has a substitution
            AstVarScope* ثابت vscp = nodep->varScopep();
            ثابت auto& it = substitutions.find(vscp);
            اگر (it == substitutions.end()) بازگشت;

            // Do not substitute circular logic
            اگر (!replaced.درج(vscp).second) بازگشت;

            // Substitute nodep with substp
            Ast_گره_عبارت* ثابت substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, گره_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_گره_عبارت* ثابت newp = substp->clone_درخت_Pure(نادرست);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            اگر (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            اگر (Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(newp, گره_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_گره* ثابت simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        برای (ثابت auto& pair : substitutions) pair.second->delete_درخت();
        substitutions.clear();
    }

    تهی optimizeSignals(منطقی allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        ثابت auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            وقتی (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            بازگشت it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        وقتی (vIt != vertices.end()) {
            GateVarVertex* ثابت vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            اگر (!vVtxp->inSize1()) ادامه;

            // Can't inline if non-reducible, etc
            اگر (!vVtxp->reducible()) ادامه;

            // Grab the driving logic
            GateLogicVertex* ثابت lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            اگر (!lVtxp->reducible()) ادامه;
            Ast_گره* ثابت logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ثابت GateOkVisitor okVisitor{logicp, نادرست, نادرست};

            // Was it ok?
            اگر (!okVisitor.isSimple()) ادامه;
            // If the varScope is already removed from logicp, no need to try substitution.
            اگر (!okVisitor.varAssigned(vVtxp->varScp())) ادامه;

            // Expression we are considering to substitute with
            Ast_گره_عبارت* ثابت substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ثابت size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            اگر (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) ادامه;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ثابت vscp = vVtxp->varScp();

            اگر (debug() >= 9) {
                vscp->dump_درخت("substituting: ");
                substp->dump_درخت("        with: ");
            }

            ثابت auto& readVscpsVec = okVisitor.readVscps();
            ثابت std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            برای (V3GraphEdge* ثابت edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* ثابت dstVtxp = edgep->بالاترین()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                اگر (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_گره* nodep = substp;
                    اگر (AstNot* ثابت notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    اگر (!VN_IS(nodep, VarRef)) ادامه;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                منطقی canInline = درست;
                برای (ثابت V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    ثابت GateVarVertex* ثابت consVVertexp = dedge.بالاترین()->as<GateVarVertex>();
                    اگر (readVscps.شمار(consVVertexp->varScp())) {
                        canInline = نادرست;
                        بشکن;
                    }
                }

                اگر (!canInline) ادامه;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                اگر (logicp == dstVtxp->nodep()) {
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
                    ادامه;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                برای (AstVarScope* ثابت newVscp : okVisitor.readVscps()) {
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
            اگر (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_درخت(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(نادرست);
        // Then propagate more complicated equations
        optimizeSignals(درست);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_گره*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](ثابت Pair& a, ثابت Pair& b) {  //
            بازگشت a.second < b.second;
        });
        برای (ثابت auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

عمومی:
    ایستا تهی apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

رده GateDedupeHash final : عمومی V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    ساختار Aux_Ast_گره_عبارت final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_گره_عبارت* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_گره_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_گره_عبارت, Aux_Ast_گره_عبارت> m_aux_گره_عبارت;

    Ast_گره_عبارت* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_گره_عبارت m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    ایستا منطقی same(Ast_گره* node1p, Ast_گره* node2p) {
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
        بازگشت node1p == node2p || (node1p && node1p->same_درخت(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    منطقی isSame(Ast_گره* node1p, Ast_گره* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ثابت auto& aux2 = m_aux_گره_عبارت(VN_AS(node2p, گره_عبارت));
        بازگشت m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

عمومی:
    GateDedupeHash() = پیشفرض;
    ~GateDedupeHash() = پیشفرض;

    ثابت Ast_گره_Assign* hashAndFindDupe(Ast_گره_Assign* assignp, AstActive* activep,
                                         Ast_گره_عبارت* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ثابت auto it = m_dupFinder.findDuplicate(m_currRhsp, این);
        اگر (it != m_dupFinder.end()) بازگشت m_aux_گره_عبارت(VN_AS(it->second, گره_عبارت)).parentp;

        // Insert new node
        m_dupFinder.درج(m_currRhsp);
        m_aux_گره_عبارت(m_currRhsp) = m_auxCurRhsp;
        بازگشت nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

رده GateDedupeVarVisitor final : عمومی VNVisitorConst {
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
    Ast_گره_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_گره_عبارت* m_ifCondp = nullptr;  // IF condition that assign is under
    منطقی m_always = نادرست;  // Assign is under an always
    منطقی m_dedupable = درست;  // Determined the assign to be dedupable

    // VISITORS
    تهی visit(Ast_گره_Assign* nodep) override {
        اگر (!m_dedupable) بازگشت;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        اگر (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_عبارت_Stmt*) { بازگشت درست; });
            بازگشت;
        }

        m_dedupable = نادرست;
    }

    تهی visit(AstAlways* nodep) override {
        اگر (!m_dedupable) بازگشت;

        اگر (!m_always) {
            m_always = درست;
            iterateAndNextConstNull(nodep->stmtsp());
            بازگشت;
        }

        m_dedupable = نادرست;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    تهی visit(Ast_گره_If* nodep) override {
        اگر (!m_dedupable) بازگشت;

        اگر (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_عبارت_Stmt*) { بازگشت درست; });
            iterateAndNextConstNull(nodep->thensp());
            بازگشت;
        }

        m_dedupable = نادرست;
    }

    تهی visit(AstComment*) override {}  // NOP
    //--------------------
    تهی visit(Ast_گره*) override { m_dedupable = نادرست; }

عمومی:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = پیشفرض;
    ~GateDedupeVarVisitor() override = پیشفرض;
    // PUBLIC METHODS
    Ast_گره_Var_Ref* findDupe(Ast_گره* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = نادرست;
        m_dedupable = درست;
        iterateConst(logicp);
        اگر (m_dedupable && m_assignp) {
            ثابت Ast_گره* ثابت lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ثابت Ast_گره_Var_Ref* ثابت lRefp = VN_CAST(lhsp, گره_Var_Ref);
            اگر (lRefp && lRefp->varScopep() == consumerVscp) {
                اگر (ثابت Ast_گره_Assign* ثابت dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    بازگشت static_cast<Ast_گره_Var_Ref*>(dup->lhsp());
                }
            }
        }
        بازگشت nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

رده GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ثابت VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    تهی visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        اگر (m_depth > GATE_DEDUP_MAX_DEPTH) بازگشت;

        // Check that we haven't been here before
        اگر (vVtxp->varScp()->user2SetOnce()) بازگشت;

        VL_RESTORER(m_depth);
        ++m_depth;
        اگر (!vVtxp->inSize1()) بازگشت;

        Ast_گره_Var_Ref* dupRefp = nullptr;
        برای (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        اگر (!dupRefp) بازگشت;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* ثابت lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        ثابت GateOkVisitor okVisitor{lVtxp->nodep(), نادرست, درست};
        اگر (!okVisitor.isSimple()) بازگشت;

        ++m_statDedupLogic;
        GateVarVertex* ثابت dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        برای (V3GraphEdge* ثابت edgep : vVtxp->outEdges().unlinkable()) {
            ثابت GateLogicVertex* ثابت consumerVtxp = edgep->بالاترین()->as<GateLogicVertex>();
            Ast_گره* ثابت consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            اگر (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } وگرنه {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_گره_Var_Ref* refp) {
                    اگر (refp->varScopep() != vVtxp->varScp()) بازگشت;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_گره_Var_Ref* ثابت newp = dupRefp->clone_درخت_Pure(نادرست);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_درخت(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        وقتی (V3GraphEdge* ثابت edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_گره_Var_Ref* visit(GateLogicVertex* lVtxp, ثابت GateVarVertex* consumerVtxp) {
        برای (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        اگر (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            بازگشت m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        بازگشت nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        برای (V3GraphVertex& vtx : graph.vertices()) {
            اگر (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) {
                اگر (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        برای (V3GraphVertex& vtx : graph.vertices()) {
            اگر (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) {
                اگر (vVtxp->is_بالاترین() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

عمومی:
    ایستا تهی apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

رده GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        ثابت AstVarRef* ثابت pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ثابت cRefp = VN_CAST(currSelp->fromp(), VarRef);
        اگر (!pRefp || !cRefp || !cRefp->same_گره(pRefp)) بازگشت nullptr;  // not the same var

        ثابت AstConst* ثابت pstart = VN_CAST(prevSelp->lsbp(), Const);
        ثابت AstConst* ثابت cstart = VN_CAST(currSelp->lsbp(), Const);
        اگر (!pstart || !cstart) بازگشت nullptr;  // too complicated

        اگر (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            بازگشت جدید AstSel{cRefp->fileline(), cRefp->clone_درخت(نادرست), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } وگرنه {
            بازگشت nullptr;
        }
    }

    تهی process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        برای (V3GraphEdge* ثابت edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* ثابت lVtxp = edgep->fromp()->as<GateLogicVertex>();
            اگر (!lVtxp->outSize1()) ادامه;

            AstAlways* ثابت alwaysp = VN_CAST(lVtxp->nodep(), Always);
            اگر (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) بازگشت;
            AstAssignW* ثابت assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            اگر (!assignp) ادامه;

            اگر (!VN_IS(assignp->lhsp(), Sel)) ادامه;

            // First assign with Sel-lhs, or not under the same active
            اگر (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                ادامه;
            }

            AstSel* ثابت prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ثابت currSelp = VN_AS(assignp->lhsp(), Sel);

            اگر (AstSel* ثابت newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_درخت(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(جدید AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                وقتی (V3GraphEdge* ثابت iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* ثابت fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } وگرنه {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        برای (V3GraphVertex& vtx : graph.vertices()) {
            اگر (GateVarVertex* ثابت vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        برای (GateLogicVertex* ثابت lVtxp : m_toRemove) {
            Ast_گره* ثابت nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

عمومی:
    ایستا تهی apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

رده GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    تهی markRecurse(GateEitherVertex* vtxp) {
        اگر (vtxp->user()) بازگشت;  // Already marked
        vtxp->user(درست);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        برای (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* ثابت fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    تهی mark() {
        m_graph.userClearVertices();
        برای (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            اگر (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ایستا تهی warnUnused(ثابت Ast_گره* ثابت nodep) {
        اگر (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) بازگشت;

        اگر (ثابت Ast_گره_Procedure* ثابت procedurep = VN_CAST(nodep, گره_Procedure)) {
            اگر (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ثابت AstLoop* ثابت loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, درست);
                });
        }
    }

    // Remove unused logic
    تهی remove() {
        برای (V3GraphVertex* ثابت vtxp : m_graph.vertices().unlinkable()) {
            اگر (GateLogicVertex* ثابت lVtxp = vtxp->cast<GateLogicVertex>()) {
                اگر (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_گره* ثابت nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
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

عمومی:
    ایستا تهی apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

تهی V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        اگر (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        اگر (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        اگر (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        اگر (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        اگر (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_درخت("gate", 0, dump_درخت_Either_Level() >= 3);
}
