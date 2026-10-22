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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Gate.h"

#ଅନ୍ତର୍ଭାଵ "V3AstUserAllocator.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3DupFinder.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

ଶ୍ରେଣୀ GateEitherVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    ତାର୍କିକ m_reducible = ସତ୍ୟ;  // True if this node should be able to be eliminated
    ତାର୍କିକ m_dedupable = ସତ୍ୟ;  // True if this node should be able to be deduped
    ତାର୍କିକ m_consumed = ମିଥ୍ୟା;  // Output goes to something meaningful
    ତାର୍କିକ m_staticInit = ମିଥ୍ୟା;  // True if this node is a static initializer
ସର୍ବସାଧାରଣ:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    ତାର୍କିକ reducible() ଅଚଳ { ଫେରନ୍ତୁ m_reducible; }
    ତାର୍କିକ dedupable() ଅଚଳ { ଫେରନ୍ତୁ m_dedupable; }
    ତାର୍କିକ consumed() ଅଚଳ { ଫେରନ୍ତୁ m_consumed; }
    ତାର୍କିକ staticInit() ଅଚଳ { ଫେରନ୍ତୁ m_staticInit; }
    ଶୂନ୍ୟ setConsumed(ଅଚଳ ଅକ୍ଷର* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = ସତ୍ୟ;
    }
    ଶୂନ୍ୟ setStaticInit() { m_staticInit = ସତ୍ୟ; }
    ଶୂନ୍ୟ clearReducible(ଅଚଳ ଅକ୍ଷର* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ clearDedupable(ଅଚଳ ଅକ୍ଷର* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ clearReducibleAndDedupable(ଅଚଳ ଅକ୍ଷର* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() ଅଚଳ override { ଫେରନ୍ତୁ m_consumed ? "" : "dotted"; }
};

ଶ୍ରେଣୀ GateVarVertex final : ସର୍ବସାଧାରଣ GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* ଅଚଳ m_varScp;
    ତାର୍କିକ m_is_ଶୀର୍ଷ = ମିଥ୍ୟା;
    ତାର୍କିକ m_isClock = ମିଥ୍ୟା;
    Ast_ଗଣ୍ଠି* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_ଗଣ୍ଠି* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
ସର୍ବସାଧାରଣ:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    AstVarScope* varScp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_varScp; }
    ତାର୍କିକ is_ଶୀର୍ଷ() ଅଚଳ { ଫେରନ୍ତୁ m_is_ଶୀର୍ଷ; }
    ଶୂନ୍ୟ set_Is_ଶୀର୍ଷ() { m_is_ଶୀର୍ଷ = ସତ୍ୟ; }
    ତାର୍କିକ isClock() ଅଚଳ { ଫେରନ୍ତୁ m_isClock; }
    ଶୂନ୍ୟ setIsClock() { m_isClock = ସତ୍ୟ; }
    Ast_ଗଣ୍ଠି* rstSyncNodep() ଅଚଳ { ଫେରନ୍ତୁ m_rstSyncNodep; }
    ଶୂନ୍ୟ rstSyncNodep(Ast_ଗଣ୍ଠି* nodep) { m_rstSyncNodep = nodep; }
    Ast_ଗଣ୍ଠି* rstAsyncNodep() ଅଚଳ { ଫେରନ୍ତୁ m_rstAsyncNodep; }
    ଶୂନ୍ୟ rstAsyncNodep(Ast_ଗଣ୍ଠି* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    ଶୂନ୍ୟ propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        ଯଦି (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ varScp()->name(); }
    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "green"; }
};

ଶ୍ରେଣୀ GateLogicVertex final : ସର୍ବସାଧାରଣ GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_ଗଣ୍ଠି* ଅଚଳ m_nodep;
    AstActive* ଅଚଳ m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    ଅଚଳ ତାର୍କିକ m_slow;  // In slow block
ସର୍ବସାଧାରଣ:
    GateLogicVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* nodep, AstActive* activep, ତାର୍କିକ slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ nodep()->fileline(); }
    Ast_ଗଣ୍ଠି* nodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
    AstActive* activep() ଅଚଳ { ଫେରନ୍ତୁ m_activep; }
    ତାର୍କିକ slow() ଅଚଳ { ଫେରନ୍ତୁ m_slow; }

    // DOT debug
    std::string name() ଅଚଳ override { ଫେରନ୍ତୁ m_nodep->fileline()->ascii(); }
    std::string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "red"; }
};

ଶ୍ରେଣୀ GateEdge final : ସର୍ବସାଧାରଣ V3GraphEdge {
    std::string dotLabel() ଅଚଳ override { ଫେରନ୍ତୁ std::to_string(weight()); }

ସର୍ବସାଧାରଣ:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight)
        : V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, weight} {}
};

ଶ୍ରେଣୀ GateGraph final : ସର୍ବସାଧାରଣ V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    ଅଚଳ VNUser1InUse m_inuser1;

ସର୍ବସାଧାରଣ:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        ଯଦି (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = ନୂତନ GateVarVertex{ଏହି, vscp};
            vscp->user1p(vVtxp);
            ଯଦି (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            ଯଦି (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            ଯଦି (vscp->varp()->isIO() && vscp->scopep()->is_ଶୀର୍ଷ()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_ଶୀର୍ଷ();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        ଫେରନ୍ତୁ vVtxp;
    }

    ଶୂନ୍ୟ addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) {
        ନୂତନ GateEdge{ଏହି, srcp, dstp, weight};
    }

    ଶୂନ୍ୟ addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight) {
        ନୂତନ GateEdge{ଏହି, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

ଶ୍ରେଣୀ GateBuildVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE
    GateGraph* m_graphp = ନୂତନ GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    ଅଚଳ AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    ତାର୍କିକ m_inClockedActive = ମିଥ୍ୟା;  // Underneath clocked active
    ତାର୍କିକ m_inEdgeActive = ମିଥ୍ୟା;  // Underneath edge active
    ତାର୍କିକ m_inStaticActive = ମିଥ୍ୟା;  // Underneath static active
    ତାର୍କିକ m_inInitialActive = ମିଥ୍ୟା;  // Underneath initial active
    ତାର୍କିକ m_inSenItem = ମିଥ୍ୟା;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    ଶୂନ୍ୟ check_ଗଣ୍ଠି(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        ଯଦି (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    ଶୂନ୍ୟ iterateLogic(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ slow = ମିଥ୍ୟା, ଅଚଳ ଅକ୍ଷର* nonReducibleReason = nullptr,
                      ଅଚଳ ଅକ୍ଷର* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = ନୂତନ GateLogicVertex{m_graphp, nodep, m_activep, slow};
        ଯଦି (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } ଅନ୍ୟଥା ଯଦି (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } ଅନ୍ୟଥା ଯଦି (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        ଯଦି (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_ଗଣ୍ଠି(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
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
        ଅଚଳ VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {
        ଅଚଳ ତାର୍କିକ slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    ଶୂନ୍ୟ visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, ମିଥ୍ୟା, "CoverToggle", "CoverToggle");
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = ସତ୍ୟ;
        ଯଦି (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } ଅନ୍ୟଥା {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, ମିଥ୍ୟା, nullptr, "senItem");
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (!m_logicVertexp) ଫେରନ୍ତୁ;

        AstVarScope* ଅଚଳ vscp = nodep->varScopep();
        GateVarVertex* ଅଚଳ vVtxp = m_graphp->makeVarVertex(vscp);

        ଯଦି (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(ସତ୍ୟ);
        } ଅନ୍ୟଥା ଯଦି (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            ଯଦି (vscp->user2()) {
                ଯଦି (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } ଅନ୍ୟଥା {
                ଯଦି (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        ଯଦି (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        ଯଦି (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    ଶୂନ୍ୟ visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), ଗଣ୍ଠି_Assign)
                      && VN_AS(nodep->backp(), ଗଣ୍ଠି_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (m_logicVertexp) check_ଗଣ୍ଠି(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        ଫେରନ୍ତୁ std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

ସ୍ଥିର ଶୂନ୍ୟ v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    ଅଚଳ VNUser2InUse user2InUse;
    ପାଇଁ (V3GraphVertex& vtx : graph.vertices()) {
        ଯଦି (ଅଚଳ GateVarVertex* ଅଚଳ vvertexp = vtx.cast<GateVarVertex>()) {
            ଅଚଳ AstVarScope* ଅଚଳ vscp = vvertexp->varScp();
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ sp = vvertexp->rstSyncNodep();
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ ap = vvertexp->rstAsyncNodep();
            ଯଦି (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                ଯଦି (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(ସତ୍ୟ);  // Warn only once per signal
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

ଶ୍ରେଣୀ GateConcatVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // STATE
    ଅଚଳ AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_offset = 0;  // Current offset of varscope
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_found_offset = 0;  // Found offset of varscope
    ତାର୍କିକ m_found = ମିଥ୍ୟା;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(ସତ୍ୟ);
            m_found_offset = m_offset;
            m_found = ସତ୍ୟ;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    ଶୂନ୍ୟ visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // PUBLIC METHODS
    ତାର୍କିକ concatOffset(AstConcat* concatp, AstVarScope* vscp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = ମିଥ୍ୟା;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        ଫେରନ୍ତୁ m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

ଶ୍ରେଣୀ GateOkVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // RETURN STATE
    ତାର୍କିକ m_isSimple = ସତ୍ୟ;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    ଅଚଳ ତାର୍କିକ m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    ଅଚଳ ତାର୍କିକ m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* m_lhsVarRef = nullptr;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ops = 0;  // Operation count

    // METHODS
    ଶୂନ୍ୟ clearSimple(ଅଚଳ ଅକ୍ଷର* because) {
        ଯଦି (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = ମିଥ୍ୟା;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Var_Ref* nodep) override {
        ଯଦି (!m_isSimple) ଫେରନ୍ତୁ;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        ଯଦି (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        ଯଦି (nodep->access().isRW()) {
            clearSimple("R/W");
            ଫେରନ୍ତୁ;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* ଅଚଳ vscp = nodep->varScopep();
        ଯଦି (nodep->access().isWriteOnly()) {
            ଯଦି (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } ଅନ୍ୟଥା {
            // TODO: possible bug, should it be >= 1 as add is below?
            ଯଦି (m_readVscps.size() > 1) {
                ଯଦି (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.ଠେଲ_back(vscp);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (!m_isSimple) ଫେରନ୍ତୁ;

        m_substitutionp = nodep->rhsp();
        ଯଦି (!VN_IS(nodep->lhsp(), ଗଣ୍ଠି_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } ଅନ୍ୟଥା ଯଦି (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } ଅନ୍ୟଥା {
            iterateChildrenConst(nodep);
        }
        ଯଦି (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    ଶୂନ୍ୟ visit(AstCReset* /*nodep*/) override {
        ଯଦି (!m_isSimple) ଫେରନ୍ତୁ;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    ଶୂନ୍ୟ visit(AstMatchMasked* nodep) override {
        ଯଦି (!m_isSimple) ଫେରନ୍ତୁ;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        ଯଦି (!m_isSimple) ଫେରନ୍ତୁ;  // Fastpath

        ଯଦି (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            ଫେରନ୍ତୁ;
        }

        ଯଦି (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            ଫେରନ୍ତୁ;
        }

        iterateChildrenConst(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    GateOkVisitor(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ buffersOnly, ତାର୍କିକ dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        ଯଦି (!m_substitutionp) {
            clearSimple("No assignment found\n");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_isSimple && m_lhsVarRef) {
            ପାଇଁ (ଅଚଳ AstVarScope* ଅଚଳ vscp : m_readVscps) {
                ଯଦି (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }
    ~GateOkVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // PUBLIC METHODS
    ତାର୍କିକ isSimple() ଅଚଳ { ଫେରନ୍ତୁ m_isSimple; }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* substitutionp() ଅଚଳ {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        ଫେରନ୍ତୁ m_substitutionp;
    }
    ଅଚଳ std::vector<AstVarScope*>& readVscps() ଅଚଳ { ଫେରନ୍ତୁ m_readVscps; }
    ତାର୍କିକ varAssigned(ଅଚଳ AstVarScope* scopep) ଅଚଳ {
        ଫେରନ୍ତୁ m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

ଶ୍ରେଣୀ GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    ଅଚଳ VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_ଗଣ୍ଠି, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_ଗଣ୍ଠି*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    ସ୍ଥିର ତାର୍କିକ isCheap(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        // Constant is cheap
        ଯଦି (VN_IS(exprp, Const)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Variable reference is cheap
        ଯଦି (VN_IS(exprp, ଗଣ୍ଠି_Var_Ref)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        ଯଦି (ଅଚଳ AstSel* ଅଚଳ selp = VN_CAST(exprp, Sel)) {
            ଯଦି (!isCheap(selp->fromp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (!selp->isWide()) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଯଦି (!VN_IS(selp->lsbp(), Const)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (selp->lsbConst() % VL_EDATASIZE != 0) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // AstArraySel is cheap if the fromp is cheap
        ଯଦି (ଅଚଳ AstArraySel* ଅଚଳ aselp = VN_CAST(exprp, ArraySel)) {
            ଫେରନ୍ତୁ isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ତାର୍କିକ shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* substp, ତାର୍କିକ allowMultiIn) {
        // Always inline constants
        ଯଦି (VN_IS(substp, Const)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Don't inline non-constant static initializers - these are scheduled differently
        ଯଦି (lVtxp->staticInit()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Inline simple variable references
        ଯଦି (VN_IS(substp, VarRef)) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Only inline arrays if a simple variable or constant
        ଯଦି (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        ଯଦି (nReads == 0) ଫେରନ୍ତୁ ସତ୍ୟ;
        // If it reads one variable, inline if not wide, or if cheap
        ଯଦି (nReads == 1 && (!substp->isWide() || isCheap(substp))) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Don't inline on first round if reads more than one variable
        ଯଦି (nReads > 1 && !allowMultiIn) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0;
        ପାଇଁ (V3GraphEdge& edge : vVtxp->outEdges()) {
            ଅଚଳ GateLogicVertex* ଅଚଳ dstVtxp = edge.ଶୀର୍ଷ()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            ଯଦି (dstVtxp->slow()) ଜାରି;
            ଯଦି (dstVtxp->outEmpty() && !dstVtxp->consumed()) ଜାରି;
            n += edge.weight();
            ଯଦି (n > 1) {
                ++m_statNotInlined;
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ recordSubstitution(AstVarScope* vscp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* substp, Ast_ଗଣ୍ଠି* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        ଅଚଳ auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        ଯଦି (pair.second) pair.first->second = substp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
    }

    ଶୂନ୍ୟ commitSubstitutions(Ast_ଗଣ୍ଠି* logicp) {
        ଯଦି (!m_hasPending.erase(logicp)) ଫେରନ୍ତୁ;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<ଅଚଳ AstVarScope*> replaced(substitutions.size() * 2);

        ଅଚଳ std::function<ଶୂନ୍ୟ(Ast_ଗଣ୍ଠି_Var_Ref*)> visit = [&](Ast_ଗଣ୍ଠି_Var_Ref* nodep) -> ଶୂନ୍ୟ {
            // See if this variable has a substitution
            AstVarScope* ଅଚଳ vscp = nodep->varScopep();
            ଅଚଳ auto& it = substitutions.find(vscp);
            ଯଦି (it == substitutions.end()) ଫେରନ୍ତୁ;

            // Do not substitute circular logic
            ଯଦି (!replaced.ଭର୍ତ୍ତି(vscp).second) ଫେରନ୍ତୁ;

            // Substitute nodep with substp
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, ଗଣ୍ଠି_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = substp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            ଯଦି (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            ଯଦି (Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varrefp = VN_CAST(newp, ଗଣ୍ଠି_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_ଗଣ୍ଠି* ଅଚଳ simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        ପାଇଁ (ଅଚଳ auto& pair : substitutions) pair.second->delete_ବୃକ୍ଷ();
        substitutions.clear();
    }

    ଶୂନ୍ୟ optimizeSignals(ତାର୍କିକ allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        ଅଚଳ auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            ଯେତେବେଳେ (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            ଫେରନ୍ତୁ it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        ଯେତେବେଳେ (vIt != vertices.end()) {
            GateVarVertex* ଅଚଳ vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            ଯଦି (!vVtxp->inSize1()) ଜାରି;

            // Can't inline if non-reducible, etc
            ଯଦି (!vVtxp->reducible()) ଜାରି;

            // Grab the driving logic
            GateLogicVertex* ଅଚଳ lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            ଯଦି (!lVtxp->reducible()) ଜାରି;
            Ast_ଗଣ୍ଠି* ଅଚଳ logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            ଅଚଳ GateOkVisitor okVisitor{logicp, ମିଥ୍ୟା, ମିଥ୍ୟା};

            // Was it ok?
            ଯଦି (!okVisitor.isSimple()) ଜାରି;
            // If the varScope is already removed from logicp, no need to try substitution.
            ଯଦି (!okVisitor.varAssigned(vVtxp->varScp())) ଜାରି;

            // Expression we are considering to substitute with
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            ଅଚଳ size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            ଯଦି (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) ଜାରି;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* ଅଚଳ vscp = vVtxp->varScp();

            ଯଦି (debug() >= 9) {
                vscp->dump_ବୃକ୍ଷ("substituting: ");
                substp->dump_ବୃକ୍ଷ("        with: ");
            }

            ଅଚଳ auto& readVscpsVec = okVisitor.readVscps();
            ଅଚଳ std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* ଅଚଳ dstVtxp = edgep->ଶୀର୍ଷ()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                ଯଦି (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_ଗଣ୍ଠି* nodep = substp;
                    ଯଦି (AstNot* ଅଚଳ notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    ଯଦି (!VN_IS(nodep, VarRef)) ଜାରି;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                ତାର୍କିକ canInline = ସତ୍ୟ;
                ପାଇଁ (ଅଚଳ V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    ଅଚଳ GateVarVertex* ଅଚଳ consVVertexp = dedge.ଶୀର୍ଷ()->as<GateVarVertex>();
                    ଯଦି (readVscps.ଗଣନା_2(consVVertexp->varScp())) {
                        canInline = ମିଥ୍ୟା;
                        ଭାଙ୍ଗ;
                    }
                }

                ଯଦି (!canInline) ଜାରି;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                ଯଦି (logicp == dstVtxp->nodep()) {
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
                    ଜାରି;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                ପାଇଁ (AstVarScope* ଅଚଳ newVscp : okVisitor.readVscps()) {
                    GateVarVertex* ଅଚଳ varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            ଯଦି (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_ବୃକ୍ଷ(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(ମିଥ୍ୟା);
        // Then propagate more complicated equations
        optimizeSignals(ସତ୍ୟ);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_ଗଣ୍ଠି*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](ଅଚଳ Pair& a, ଅଚଳ Pair& b) {  //
            ଫେରନ୍ତୁ a.second < b.second;
        });
        ପାଇଁ (ଅଚଳ auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

ଶ୍ରେଣୀ GateDedupeHash final : ସର୍ବସାଧାରଣ V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    ସଂରଚନା Aux_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_ଗଣ୍ଠି_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି, Aux_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି> m_aux_ଗଣ୍ଠି_ପଦସମଷ୍ଟି;

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    ସ୍ଥିର ତାର୍କିକ same(Ast_ଗଣ୍ଠି* node1p, Ast_ଗଣ୍ଠି* node2p) {
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
        ଫେରନ୍ତୁ node1p == node2p || (node1p && node1p->same_ବୃକ୍ଷ(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    ତାର୍କିକ isSame(Ast_ଗଣ୍ଠି* node1p, Ast_ଗଣ୍ଠି* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        ଅଚଳ auto& aux2 = m_aux_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(VN_AS(node2p, ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
        ଫେରନ୍ତୁ m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

ସର୍ବସାଧାରଣ:
    GateDedupeHash() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~GateDedupeHash() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଅଚଳ Ast_ଗଣ୍ଠି_Assign* hashAndFindDupe(Ast_ଗଣ୍ଠି_Assign* assignp, AstActive* activep,
                                         Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        ଅଚଳ auto it = m_dupFinder.findDuplicate(m_currRhsp, ଏହି);
        ଯଦି (it != m_dupFinder.end()) ଫେରନ୍ତୁ m_aux_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(VN_AS(it->second, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)).parentp;

        // Insert new node
        m_dupFinder.ଭର୍ତ୍ତି(m_currRhsp);
        m_aux_ଗଣ୍ଠି_ପଦସମଷ୍ଟି(m_currRhsp) = m_auxCurRhsp;
        ଫେରନ୍ତୁ nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

ଶ୍ରେଣୀ GateDedupeVarVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
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
    Ast_ଗଣ୍ଠି_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_ifCondp = nullptr;  // IF condition that assign is under
    ତାର୍କିକ m_always = ମିଥ୍ୟା;  // Assign is under an always
    ତାର୍କିକ m_dedupable = ସତ୍ୟ;  // Determined the assign to be dedupable

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Assign* nodep) override {
        ଯଦି (!m_dedupable) ଫେରନ୍ତୁ;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        ଯଦି (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_ପଦସମଷ୍ଟି_Stmt*) { ଫେରନ୍ତୁ ସତ୍ୟ; });
            ଫେରନ୍ତୁ;
        }

        m_dedupable = ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        ଯଦି (!m_dedupable) ଫେରନ୍ତୁ;

        ଯଦି (!m_always) {
            m_always = ସତ୍ୟ;
            iterateAndNextConstNull(nodep->stmtsp());
            ଫେରନ୍ତୁ;
        }

        m_dedupable = ମିଥ୍ୟା;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        ଯଦି (!m_dedupable) ଫେରନ୍ତୁ;

        ଯଦି (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_ପଦସମଷ୍ଟି_Stmt*) { ଫେରନ୍ତୁ ସତ୍ୟ; });
            iterateAndNextConstNull(nodep->thensp());
            ଫେରନ୍ତୁ;
        }

        m_dedupable = ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ visit(AstComment*) override {}  // NOP
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି*) override { m_dedupable = ମିଥ୍ୟା; }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~GateDedupeVarVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // PUBLIC METHODS
    Ast_ଗଣ୍ଠି_Var_Ref* findDupe(Ast_ଗଣ୍ଠି* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = ମିଥ୍ୟା;
        m_dedupable = ସତ୍ୟ;
        iterateConst(logicp);
        ଯଦି (m_dedupable && m_assignp) {
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ lRefp = VN_CAST(lhsp, ଗଣ୍ଠି_Var_Ref);
            ଯଦି (lRefp && lRefp->varScopep() == consumerVscp) {
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Assign* ଅଚଳ dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    ଫେରନ୍ତୁ static_cast<Ast_ଗଣ୍ଠି_Var_Ref*>(dup->lhsp());
                }
            }
        }
        ଫେରନ୍ତୁ nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

ଶ୍ରେଣୀ GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    ଅଚଳ VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    ଶୂନ୍ୟ visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        ଯଦି (m_depth > GATE_DEDUP_MAX_DEPTH) ଫେରନ୍ତୁ;

        // Check that we haven't been here before
        ଯଦି (vVtxp->varScp()->user2SetOnce()) ଫେରନ୍ତୁ;

        VL_RESTORER(m_depth);
        ++m_depth;
        ଯଦି (!vVtxp->inSize1()) ଫେରନ୍ତୁ;

        Ast_ଗଣ୍ଠି_Var_Ref* dupRefp = nullptr;
        ପାଇଁ (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        ଯଦି (!dupRefp) ଫେରନ୍ତୁ;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* ଅଚଳ lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        ଅଚଳ GateOkVisitor okVisitor{lVtxp->nodep(), ମିଥ୍ୟା, ସତ୍ୟ};
        ଯଦି (!okVisitor.isSimple()) ଫେରନ୍ତୁ;

        ++m_statDedupLogic;
        GateVarVertex* ଅଚଳ dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : vVtxp->outEdges().unlinkable()) {
            ଅଚଳ GateLogicVertex* ଅଚଳ consumerVtxp = edgep->ଶୀର୍ଷ()->as<GateLogicVertex>();
            Ast_ଗଣ୍ଠି* ଅଚଳ consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            ଯଦି (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } ଅନ୍ୟଥା {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_ଗଣ୍ଠି_Var_Ref* refp) {
                    ଯଦି (refp->varScopep() != vVtxp->varScp()) ଫେରନ୍ତୁ;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ newp = dupRefp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_ବୃକ୍ଷ(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        ଯେତେବେଳେ (V3GraphEdge* ଅଚଳ edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_ଗଣ୍ଠି_Var_Ref* visit(GateLogicVertex* lVtxp, ଅଚଳ GateVarVertex* consumerVtxp) {
        ପାଇଁ (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        ଯଦି (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            ଫେରନ୍ତୁ m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        ଫେରନ୍ତୁ nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        ପାଇଁ (V3GraphVertex& vtx : graph.vertices()) {
            ଯଦି (GateVarVertex* ଅଚଳ vVtxp = vtx.cast<GateVarVertex>()) {
                ଯଦି (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        ପାଇଁ (V3GraphVertex& vtx : graph.vertices()) {
            ଯଦି (GateVarVertex* ଅଚଳ vVtxp = vtx.cast<GateVarVertex>()) {
                ଯଦି (vVtxp->is_ଶୀର୍ଷ() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

ଶ୍ରେଣୀ GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        ଅଚଳ AstVarRef* ଅଚଳ pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* ଅଚଳ cRefp = VN_CAST(currSelp->fromp(), VarRef);
        ଯଦି (!pRefp || !cRefp || !cRefp->same_ଗଣ୍ଠି(pRefp)) ଫେରନ୍ତୁ nullptr;  // not the same var

        ଅଚଳ AstConst* ଅଚଳ pstart = VN_CAST(prevSelp->lsbp(), Const);
        ଅଚଳ AstConst* ଅଚଳ cstart = VN_CAST(currSelp->lsbp(), Const);
        ଯଦି (!pstart || !cstart) ଫେରନ୍ତୁ nullptr;  // too complicated

        ଯଦି (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            ଫେରନ୍ତୁ ନୂତନ AstSel{cRefp->fileline(), cRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ nullptr;
        }
    }

    ଶୂନ୍ୟ process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* ଅଚଳ lVtxp = edgep->fromp()->as<GateLogicVertex>();
            ଯଦି (!lVtxp->outSize1()) ଜାରି;

            AstAlways* ଅଚଳ alwaysp = VN_CAST(lVtxp->nodep(), Always);
            ଯଦି (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) ଫେରନ୍ତୁ;
            AstAssignW* ଅଚଳ assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            ଯଦି (!assignp) ଜାରି;

            ଯଦି (!VN_IS(assignp->lhsp(), Sel)) ଜାରି;

            // First assign with Sel-lhs, or not under the same active
            ଯଦି (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                ଜାରି;
            }

            AstSel* ଅଚଳ prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* ଅଚଳ currSelp = VN_AS(assignp->lhsp(), Sel);

            ଯଦି (AstSel* ଅଚଳ newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_ବୃକ୍ଷ(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(ନୂତନ AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                ଯେତେବେଳେ (V3GraphEdge* ଅଚଳ iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* ଅଚଳ fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } ଅନ୍ୟଥା {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        ପାଇଁ (V3GraphVertex& vtx : graph.vertices()) {
            ଯଦି (GateVarVertex* ଅଚଳ vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        ପାଇଁ (GateLogicVertex* ଅଚଳ lVtxp : m_toRemove) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

ଶ୍ରେଣୀ GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    ଶୂନ୍ୟ markRecurse(GateEitherVertex* vtxp) {
        ଯଦି (vtxp->user()) ଫେରନ୍ତୁ;  // Already marked
        vtxp->user(ସତ୍ୟ);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        ପାଇଁ (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* ଅଚଳ fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    ଶୂନ୍ୟ mark() {
        m_graph.userClearVertices();
        ପାଇଁ (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            ଯଦି (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    ସ୍ଥିର ଶୂନ୍ୟ warnUnused(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep) {
        ଯଦି (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) ଫେରନ୍ତୁ;

        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ procedurep = VN_CAST(nodep, ଗଣ୍ଠି_Procedure)) {
            ଯଦି (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](ଅଚଳ AstLoop* ଅଚଳ loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, ସତ୍ୟ);
                });
        }
    }

    // Remove unused logic
    ଶୂନ୍ୟ remove() {
        ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_graph.vertices().unlinkable()) {
            ଯଦି (GateLogicVertex* ଅଚଳ lVtxp = vtxp->cast<GateLogicVertex>()) {
                ଯଦି (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_ଗଣ୍ଠି* ଅଚଳ nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
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

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

ଶୂନ୍ୟ V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        ଯଦି (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        ଯଦି (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        ଯଦି (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        ଯଦି (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        ଯଦି (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_ବୃକ୍ଷ("gate", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
