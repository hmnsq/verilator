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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Gate.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3DupFinder.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <list>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr uint32_t GATE_DEDUP_MAX_DEPTH = 20;

//######################################################################
// Gate graph classes

വർഗ്ഗം GateEitherVertex VL_NOT_FINAL : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(GateEitherVertex, V3GraphVertex)
    തർക്കം m_reducible = സത്യം;  // True if this node should be able to be eliminated
    തർക്കം m_dedupable = സത്യം;  // True if this node should be able to be deduped
    തർക്കം m_consumed = അസത്യം;  // Output goes to something meaningful
    തർക്കം m_staticInit = അസത്യം;  // True if this node is a static initializer
പൊതു:
    explicit GateEitherVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    ~GateEitherVertex() override = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    തർക്കം reducible() സ്ഥിരം { തിരികെ m_reducible; }
    തർക്കം dedupable() സ്ഥിരം { തിരികെ m_dedupable; }
    തർക്കം consumed() സ്ഥിരം { തിരികെ m_consumed; }
    തർക്കം staticInit() സ്ഥിരം { തിരികെ m_staticInit; }
    ശൂന്യം setConsumed(സ്ഥിരം അക്ഷരം* /*consumedReason*/) {
        // if (!m_consumed) UINFO(0, "\t\tSetConsumed " << consumedReason << " " << this);
        m_consumed = സത്യം;
    }
    ശൂന്യം setStaticInit() { m_staticInit = സത്യം; }
    ശൂന്യം clearReducible(സ്ഥിരം അക്ഷരം* /*nonReducibleReason*/) {
        // UINFO(0, "     NR: " << nonReducibleReason << "  " << name());
        m_reducible = അസത്യം;
    }
    ശൂന്യം clearDedupable(സ്ഥിരം അക്ഷരം* /*nonDedupableReason*/) {
        // UINFO(0, "     ND: " << nonDedupableReason << "  " << name());
        m_dedupable = അസത്യം;
    }
    ശൂന്യം clearReducibleAndDedupable(സ്ഥിരം അക്ഷരം* nonReducibleReason) {
        clearReducible(nonReducibleReason);
        clearDedupable(nonReducibleReason);
    }

    // DOT debug
    std::string dotStyle() സ്ഥിരം override { തിരികെ m_consumed ? "" : "dotted"; }
};

വർഗ്ഗം GateVarVertex final : പൊതു GateEitherVertex {
    VL_RTTI_IMPL(GateVarVertex, GateEitherVertex)
    AstVarScope* സ്ഥിരം m_varScp;
    തർക്കം m_is_മുകൾ = അസത്യം;
    തർക്കം m_isClock = അസത്യം;
    Ast_കണ്ണി* m_rstSyncNodep = nullptr;  // Used as reset and not in SenItem, in clocked always
    Ast_കണ്ണി* m_rstAsyncNodep = nullptr;  // Used as reset and in SenItem, in clocked always
പൊതു:
    GateVarVertex(V3Graph* graphp, AstVarScope* varScp)
        : GateEitherVertex{graphp}
        , m_varScp{varScp} {}
    ~GateVarVertex() override = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    AstVarScope* varScp() സ്ഥിരം VL_MT_STABLE { തിരികെ m_varScp; }
    തർക്കം is_മുകൾ() സ്ഥിരം { തിരികെ m_is_മുകൾ; }
    ശൂന്യം set_Is_മുകൾ() { m_is_മുകൾ = സത്യം; }
    തർക്കം isClock() സ്ഥിരം { തിരികെ m_isClock; }
    ശൂന്യം setIsClock() { m_isClock = സത്യം; }
    Ast_കണ്ണി* rstSyncNodep() സ്ഥിരം { തിരികെ m_rstSyncNodep; }
    ശൂന്യം rstSyncNodep(Ast_കണ്ണി* nodep) { m_rstSyncNodep = nodep; }
    Ast_കണ്ണി* rstAsyncNodep() സ്ഥിരം { തിരികെ m_rstAsyncNodep; }
    ശൂന്യം rstAsyncNodep(Ast_കണ്ണി* nodep) { m_rstAsyncNodep = nodep; }

    // METHODS
    ശൂന്യം propagateAttrFrom(GateVarVertex* fromp) {
        varScp()->varp()->propagateAttrFrom(fromp->varScp()->varp());
        എങ്കിൽ (fromp->isClock()) setIsClock();
    }

    // DOT debug
    std::string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ varScp()->name(); }
    std::string dotColor() സ്ഥിരം override { തിരികെ "green"; }
};

വർഗ്ഗം GateLogicVertex final : പൊതു GateEitherVertex {
    VL_RTTI_IMPL(GateLogicVertex, GateEitherVertex)
    Ast_കണ്ണി* സ്ഥിരം m_nodep;
    AstActive* സ്ഥിരം m_activep;  // Under what active; nullptr is ok (under cfunc or such)
    സ്ഥിരം തർക്കം m_slow;  // In slow block
പൊതു:
    GateLogicVertex(V3Graph* graphp, Ast_കണ്ണി* nodep, AstActive* activep, തർക്കം slow)
        : GateEitherVertex{graphp}
        , m_nodep{nodep}
        , m_activep{activep}
        , m_slow{slow} {}
    ~GateLogicVertex() override = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    File_വരി* fileline() സ്ഥിരം override { തിരികെ nodep()->fileline(); }
    Ast_കണ്ണി* nodep() സ്ഥിരം { തിരികെ m_nodep; }
    AstActive* activep() സ്ഥിരം { തിരികെ m_activep; }
    തർക്കം slow() സ്ഥിരം { തിരികെ m_slow; }

    // DOT debug
    std::string name() സ്ഥിരം override { തിരികെ m_nodep->fileline()->ascii(); }
    std::string dotColor() സ്ഥിരം override { തിരികെ "red"; }
};

വർഗ്ഗം GateEdge final : പൊതു V3GraphEdge {
    std::string dotLabel() സ്ഥിരം override { തിരികെ std::to_string(weight()); }

പൊതു:
    GateEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* മുകൾ, പൂർണ്ണസംഖ്യ weight)
        : V3GraphEdge{graphp, fromp, മുകൾ, weight} {}
};

വർഗ്ഗം GateGraph final : പൊതു V3Graph {
    // NODE STATE
    // AstVarScope::user1p      -> GateVarVertex* for usage var, 0=not set yet
    സ്ഥിരം VNUser1InUse m_inuser1;

പൊതു:
    GateVarVertex* makeVarVertex(AstVarScope* vscp) {
        GateVarVertex* vVtxp = reinterpret_cast<GateVarVertex*>(vscp->user1p());
        എങ്കിൽ (!vVtxp) {
            UINFO(6, "New vertex " << vscp);
            vVtxp = പുതിയ GateVarVertex{ഇത്, vscp};
            vscp->user1p(vVtxp);
            എങ്കിൽ (vscp->varp()->sensIfacep() || vscp->varp()->isVirtIface()) {
                // Can be read/written to via the referenced actual interface
                vVtxp->clearReducibleAndDedupable("VirtIface");
                vVtxp->setConsumed("VirtIface");
            }
            എങ്കിൽ (vscp->varp()->isSigPublic()) {
                // Public signals shouldn't be changed, pli code might be messing with them
                vVtxp->clearReducibleAndDedupable("SigPublic");
                vVtxp->setConsumed("SigPublic");
            }
            എങ്കിൽ (vscp->varp()->isIO() && vscp->scopep()->is_മുകൾ()) {
                // We may need to convert to/from sysc/reg sigs
                vVtxp->set_Is_മുകൾ();
                vVtxp->clearReducibleAndDedupable("isTop");
                vVtxp->setConsumed("isTop");
            }
        }
        തിരികെ vVtxp;
    }

    ശൂന്യം addEdge(GateVarVertex* srcp, GateLogicVertex* dstp, പൂർണ്ണസംഖ്യ weight) {
        പുതിയ GateEdge{ഇത്, srcp, dstp, weight};
    }

    ശൂന്യം addEdge(GateLogicVertex* srcp, GateVarVertex* dstp, പൂർണ്ണസംഖ്യ weight) {
        പുതിയ GateEdge{ഇത്, srcp, dstp, weight};
    }
};

//######################################################################
// GateGraph builder

വർഗ്ഗം GateBuildVisitor final : പൊതു VNVisitorConst {
    // STATE
    GateGraph* m_graphp = പുതിയ GateGraph{};  // The graph being built (var usages/dependencies)
    GateLogicVertex* m_logicVertexp = nullptr;  // Current statement being tracked, nullptr=ignored
    സ്ഥിരം Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    സ്ഥിരം AstScope* m_scopep = nullptr;  // Current scope being processed
    AstActive* m_activep = nullptr;  // Current active
    തർക്കം m_inClockedActive = അസത്യം;  // Underneath clocked active
    തർക്കം m_inEdgeActive = അസത്യം;  // Underneath edge active
    തർക്കം m_inStaticActive = അസത്യം;  // Underneath static active
    തർക്കം m_inInitialActive = അസത്യം;  // Underneath initial active
    തർക്കം m_inSenItem = അസത്യം;  // Underneath AstSenItem; any varrefs are clocks

    // METHODS
    ശൂന്യം check_കണ്ണി(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (nodep->isOutputter()) m_logicVertexp->setConsumed("outputter");
        എങ്കിൽ (nodep->isTimingControl()) {
            m_logicVertexp->clearReducibleAndDedupable("TimingControl");
            m_logicVertexp->setConsumed("TimingControl");
        }
    }

    ശൂന്യം iterateLogic(Ast_കണ്ണി* nodep, തർക്കം slow = അസത്യം, സ്ഥിരം അക്ഷരം* nonReducibleReason = nullptr,
                      സ്ഥിരം അക്ഷരം* consumeReason = nullptr) {
        UASSERT_OBJ(m_scopep, nodep, "Logic not under Scope");
        UASSERT_OBJ(!m_logicVertexp, nodep, "Logic blocks should not nest");
        VL_RESTORER(m_logicVertexp);

        // m_activep is null under AstCFunc's, that's ok.
        m_logicVertexp = പുതിയ GateLogicVertex{m_graphp, nodep, m_activep, slow};
        എങ്കിൽ (nonReducibleReason) {
            m_logicVertexp->clearReducibleAndDedupable(nonReducibleReason);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_inClockedActive) {
            m_logicVertexp->clearReducible("Clocked logic");  // but dedupable
        } അല്ലെങ്കിൽ എങ്കിൽ (m_inStaticActive || m_inInitialActive) {
            // Runs only once, so its output must not be treated as live combinational logic
            m_logicVertexp->setStaticInit();
        }
        എങ്കിൽ (consumeReason) m_logicVertexp->setConsumed(consumeReason);
        check_കണ്ണി(nodep);
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        UASSERT_OBJ(!m_modp, nodep, "Should not nest");
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstScope* nodep) override {
        UASSERT_OBJ(!m_scopep, nodep, "Should not nest");
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildrenConst(nodep);
    }
    ശൂന്യം visit(AstActive* nodep) override {
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
        സ്ഥിരം VNUser2InUse user2InUse;
        iterateChildrenConst(nodep);
    }

    ശൂന്യം visit(AstCFunc* nodep) override {  //
        iterateLogic(nodep, nodep->slow(), "C Function", "C Function");
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        സ്ഥിരം തർക്കം slow = VN_IS(nodep, Initial) || VN_IS(nodep, Final);
        iterateLogic(nodep, slow, nodep->isJustOneBodyStmt() ? nullptr : "Multiple Stmts");
    }
    ശൂന്യം visit(AstCoverToggle* nodep) override {
        iterateLogic(nodep, അസത്യം, "CoverToggle", "CoverToggle");
    }
    ശൂന്യം visit(AstSenItem* nodep) override {
        VL_RESTORER(m_inSenItem);
        m_inSenItem = സത്യം;
        എങ്കിൽ (m_logicVertexp) {  // Already under logic, e.g.: AstEventControl
            iterateChildrenConst(nodep);
        } അല്ലെങ്കിൽ {  // Standalone item, under a SenTree or an Active
            iterateLogic(nodep, അസത്യം, nullptr, "senItem");
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (!m_logicVertexp) തിരികെ;

        AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
        GateVarVertex* സ്ഥിരം vVtxp = m_graphp->makeVarVertex(vscp);

        എങ്കിൽ (m_inSenItem) {
            vVtxp->setIsClock();
            vscp->user2(സത്യം);
        } അല്ലെങ്കിൽ എങ്കിൽ (m_inEdgeActive && nodep->access().isReadOnly()) {
            // For SYNCASYNCNET
            എങ്കിൽ (vscp->user2()) {
                എങ്കിൽ (!vVtxp->rstAsyncNodep()) vVtxp->rstAsyncNodep(nodep);
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (!vVtxp->rstSyncNodep()) vVtxp->rstSyncNodep(nodep);
            }
        }

        // We use weight of one; if we ref the var more than once, when we simplify,
        // the weight will increase
        എങ്കിൽ (nodep->access().isWriteOrRW()) m_graphp->addEdge(m_logicVertexp, vVtxp, 1);
        എങ്കിൽ (nodep->access().isReadOrRW()) m_graphp->addEdge(vVtxp, m_logicVertexp, 1);
    }
    ശൂന്യം visit(AstConcat* nodep) override {
        UASSERT_OBJ(!(VN_IS(nodep->backp(), കണ്ണി_Assign)
                      && VN_AS(nodep->backp(), കണ്ണി_Assign)->lhsp() == nodep),
                    nodep, "Concat on LHS of assignment; V3Const should have deleted it");
        iterateChildrenConst(nodep);
    }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (m_logicVertexp) check_കണ്ണി(nodep);
        iterateChildrenConst(nodep);
    }

    // CONSTRUCTORS
    explicit GateBuildVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }

പൊതു:
    സ്ഥിതം std::unique_ptr<GateGraph> apply(AstNetlist* netlistp) {
        തിരികെ std::unique_ptr<GateGraph>{GateBuildVisitor{netlistp}.m_graphp};
    }
};

//######################################################################
// SYCNASYNC warning

സ്ഥിതം ശൂന്യം v3GateWarnSyncAsync(GateGraph& graph) {
    // AstVar::user2            -> bool: Warned about SYNCASYNCNET
    സ്ഥിരം VNUser2InUse user2InUse;
    വേണ്ടി (V3GraphVertex& vtx : graph.vertices()) {
        എങ്കിൽ (സ്ഥിരം GateVarVertex* സ്ഥിരം vvertexp = vtx.cast<GateVarVertex>()) {
            സ്ഥിരം AstVarScope* സ്ഥിരം vscp = vvertexp->varScp();
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം sp = vvertexp->rstSyncNodep();
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം ap = vvertexp->rstAsyncNodep();
            എങ്കിൽ (ap && sp && !vscp->varp()->user2()) {
                // This is somewhat wrong, as marking one flop as ok for sync
                // may mean a different flop now fails.  However it's a pain to
                // then report a warning in a new place - we should report them all at once.
                // Instead, we will disable if any disabled
                എങ്കിൽ (!vscp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !ap->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)
                    && !sp->fileline()->warnIsOff(V3ErrorCode::SYNCASYNCNET)) {
                    vscp->varp()->user2(സത്യം);  // Warn only once per signal
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

വർഗ്ഗം GateConcatVisitor final : പൊതു VNVisitorConst {
    // STATE
    സ്ഥിരം AstVarScope* m_vscp = nullptr;  // Varscope we're trying to find
    പൂർണ്ണസംഖ്യ m_offset = 0;  // Current offset of varscope
    പൂർണ്ണസംഖ്യ m_found_offset = 0;  // Found offset of varscope
    തർക്കം m_found = അസത്യം;  // Offset found

    // VISITORS
    // TODO: This is broken, what if there is logic in between? {a, ~clk}
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (nodep->varScopep() == m_vscp && !nodep->user2() && !m_found) {
            // A concatenation may use the same var multiple times
            // But the graph will initially have an edge per instance
            nodep->user2(സത്യം);
            m_found_offset = m_offset;
            m_found = സത്യം;
            UINFO(9, "CLK DECOMP Concat found var (off = " << m_offset << ") - " << nodep);
        }
        m_offset += nodep->dtypep()->width();
    }
    ശൂന്യം visit(AstConcat* nodep) override {
        iterateConst(nodep->rhsp());
        iterateConst(nodep->lhsp());
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // PUBLIC METHODS
    തർക്കം concatOffset(AstConcat* concatp, AstVarScope* vscp, പൂർണ്ണസംഖ്യ& offsetr) {
        m_vscp = vscp;
        m_offset = 0;
        m_found = അസത്യം;
        // Iterate
        iterateConst(concatp);
        offsetr = m_found_offset;
        തിരികെ m_found;
    }
};

//######################################################################
// Is this a simple expression with a single input and single output?

വർഗ്ഗം GateOkVisitor final : പൊതു VNVisitorConst {
    // RETURN STATE
    തർക്കം m_isSimple = സത്യം;  // Set false when we know it isn't simple
    std::vector<AstVarScope*> m_readVscps;  // Variables read by logic
    Ast_കണ്ണി_വ്യഞ്ജകം* m_substitutionp = nullptr;  // What to replace the variable with

    // STATE
    സ്ഥിരം തർക്കം m_dedupe;  // Set when we use isGateDedupable instead of isGateOptimizable

    // Set when we only allow simple buffering, no equations (for clocks)
    സ്ഥിരം തർക്കം m_buffersOnly;
    // VarRef on lhs of assignment (what we're replacing)
    സ്ഥിരം Ast_കണ്ണി_Var_Ref* m_lhsVarRef = nullptr;
    പൂർണ്ണസംഖ്യ m_ops = 0;  // Operation count

    // METHODS
    ശൂന്യം clearSimple(സ്ഥിരം അക്ഷരം* because) {
        എങ്കിൽ (m_isSimple) UINFO(9, "Clear simple " << because);
        m_isSimple = അസത്യം;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (!m_isSimple) തിരികെ;

        ++m_ops;
        // Don't want to eliminate the VL_ASSIGN_S*
        എങ്കിൽ (nodep->varScopep()->varp()->isSc()) clearSimple("SystemC sig");

        എങ്കിൽ (nodep->access().isRW()) {
            clearSimple("R/W");
            തിരികെ;
        }

        // We only allow a LHS ref for the var being set, and a RHS ref for
        // something else being read.
        AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
        എങ്കിൽ (nodep->access().isWriteOnly()) {
            എങ്കിൽ (m_lhsVarRef) clearSimple(">1 write refs");
            m_lhsVarRef = nodep;
        } അല്ലെങ്കിൽ {
            // TODO: possible bug, should it be >= 1 as add is below?
            എങ്കിൽ (m_readVscps.size() > 1) {
                എങ്കിൽ (m_buffersOnly) clearSimple(">1 rhs varRefs");
            }
            m_readVscps.തള്ളുക_back(vscp);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (!m_isSimple) തിരികെ;

        m_substitutionp = nodep->rhsp();
        എങ്കിൽ (!VN_IS(nodep->lhsp(), കണ്ണി_Var_Ref)) {
            clearSimple("ASSIGN(non-VARREF)");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isTimingControl()) {
            clearSimple("Timing control");
        } അല്ലെങ്കിൽ {
            iterateChildrenConst(nodep);
        }
        എങ്കിൽ (m_buffersOnly && !VN_IS(nodep->rhsp(), VarRef)) {
            clearSimple("Not a buffer (goes to a clock)");
        }
    }
    ശൂന്യം visit(AstCReset* /*nodep*/) override {
        എങ്കിൽ (!m_isSimple) തിരികെ;
        // CReset is pure because we can optimize assignments, but if is
        // the only assignment to a variable we still need to initial
        // assign to get randomization etc
        clearSimple("CReset");
    }
    ശൂന്യം visit(AstMatchMasked* nodep) override {
        എങ്കിൽ (!m_isSimple) തിരികെ;
        // This node can be expensive
        clearSimple("MatchMasked");
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        എങ്കിൽ (!m_isSimple) തിരികെ;  // Fastpath

        എങ്കിൽ (++m_ops > v3Global.opt.gateStmts()) {
            clearSimple("--gate-stmts exceeded");
            തിരികെ;
        }

        എങ്കിൽ (!(m_dedupe ? nodep->isGateDedupable() : nodep->isGateOptimizable())  //
            || !nodep->isPure() || nodep->isBrancher()) {
            UINFO(5, "Non optimizable type: " << nodep);
            clearSimple("Non optimizable type");
            തിരികെ;
        }

        iterateChildrenConst(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    GateOkVisitor(Ast_കണ്ണി* nodep, തർക്കം buffersOnly, തർക്കം dedupe)
        : m_dedupe{dedupe}
        , m_buffersOnly{buffersOnly} {
        // Iterate
        iterateConst(nodep);
        // Check results
        എങ്കിൽ (!m_substitutionp) {
            clearSimple("No assignment found\n");
            തിരികെ;
        }
        എങ്കിൽ (m_isSimple && m_lhsVarRef) {
            വേണ്ടി (സ്ഥിരം AstVarScope* സ്ഥിരം vscp : m_readVscps) {
                എങ്കിൽ (m_lhsVarRef->varScopep() == vscp) {
                    clearSimple("Circular logic\n");
                    തിരികെ;
                }
            }
        }
    }
    ~GateOkVisitor() override = സ്ഥിരസ്ഥിതി;
    // PUBLIC METHODS
    തർക്കം isSimple() സ്ഥിരം { തിരികെ m_isSimple; }
    Ast_കണ്ണി_വ്യഞ്ജകം* substitutionp() സ്ഥിരം {
        UASSERT(m_isSimple, "Can't substitute non-simple");
        തിരികെ m_substitutionp;
    }
    സ്ഥിരം std::vector<AstVarScope*>& readVscps() സ്ഥിരം { തിരികെ m_readVscps; }
    തർക്കം varAssigned(സ്ഥിരം AstVarScope* scopep) സ്ഥിരം {
        തിരികെ m_lhsVarRef && (m_lhsVarRef->varScopep() == scopep);
    }
};

//######################################################################
// GateInline

വർഗ്ഗം GateInline final {
    using Substitutions = std::unordered_map<AstVarScope*, Ast_കണ്ണി_വ്യഞ്ജകം*>;

    // NODE STATE
    // {logic}Node::user2       -> map of substitutions, via m_substitutions
    സ്ഥിരം VNUser2InUse m_inuser2;

    // Variable substitutions to apply to a given logic block
    AstUser2Allocator<Ast_കണ്ണി, Substitutions> m_substitutions;

    // STATE
    GateGraph& m_graph;
    size_t m_ord = 0;  // Counter for sorting
    // Logic block with pending substitutions are stored in this map, together with their ordinal
    std::unordered_map<Ast_കണ്ണി*, size_t> m_hasPending;
    size_t m_statInlined = 0;  // Statistic tracking - signals inlined
    size_t m_statNotInlined = 0;  // Statistic tracking - signals not inlined due to cost
    size_t m_statRefs = 0;  // Statistic tracking - number of input variable references replaced

    // METHODS
    സ്ഥിതം തർക്കം isCheap(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        // Constant is cheap
        എങ്കിൽ (VN_IS(exprp, Const)) തിരികെ സത്യം;
        // Variable reference is cheap
        എങ്കിൽ (VN_IS(exprp, കണ്ണി_Var_Ref)) തിരികെ സത്യം;
        // AstSel is cheap if the fromp is cheap, and not a wide needing bit swizzling
        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(exprp, Sel)) {
            എങ്കിൽ (!isCheap(selp->fromp())) തിരികെ അസത്യം;
            എങ്കിൽ (!selp->isWide()) തിരികെ സത്യം;
            എങ്കിൽ (!VN_IS(selp->lsbp(), Const)) തിരികെ അസത്യം;
            എങ്കിൽ (selp->lsbConst() % VL_EDATASIZE != 0) തിരികെ അസത്യം;
            തിരികെ സത്യം;
        }
        // AstArraySel is cheap if the fromp is cheap
        എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം aselp = VN_CAST(exprp, ArraySel)) {
            തിരികെ isCheap(aselp->fromp());
        }
        // Otherwise it is not cheap
        തിരികെ അസത്യം;
    }

    തർക്കം shouldInline(GateVarVertex* vVtxp, GateLogicVertex* lVtxp, size_t nReads,
                      Ast_കണ്ണി_വ്യഞ്ജകം* substp, തർക്കം allowMultiIn) {
        // Always inline constants
        എങ്കിൽ (VN_IS(substp, Const)) തിരികെ സത്യം;
        // Don't inline non-constant static initializers - these are scheduled differently
        എങ്കിൽ (lVtxp->staticInit()) തിരികെ അസത്യം;
        // Inline simple variable references
        എങ്കിൽ (VN_IS(substp, VarRef)) തിരികെ സത്യം;
        // Only inline arrays if a simple variable or constant
        എങ്കിൽ (VN_IS(vVtxp->varScp()->dtypep()->skipRefp(), UnpackArrayDType)) തിരികെ അസത്യം;
        // Inline if reads no variables - unfolded constant expression, nullary builtin e.g.: $time
        എങ്കിൽ (nReads == 0) തിരികെ സത്യം;
        // If it reads one variable, inline if not wide, or if cheap
        എങ്കിൽ (nReads == 1 && (!substp->isWide() || isCheap(substp))) തിരികെ സത്യം;
        // Don't inline on first round if reads more than one variable
        എങ്കിൽ (nReads > 1 && !allowMultiIn) തിരികെ അസത്യം;
        // Reads multiple variables, or is expensive to compute.
        // Inline if used only once, ignoring slow code, or dead code that can be deleted.
        പൂർണ്ണസംഖ്യ n = 0;
        വേണ്ടി (V3GraphEdge& edge : vVtxp->outEdges()) {
            സ്ഥിരം GateLogicVertex* സ്ഥിരം dstVtxp = edge.മുകൾ()->as<GateLogicVertex>();
            // Ignore slow code, or if the destination is not used
            എങ്കിൽ (dstVtxp->slow()) തുടരുക;
            എങ്കിൽ (dstVtxp->outEmpty() && !dstVtxp->consumed()) തുടരുക;
            n += edge.weight();
            എങ്കിൽ (n > 1) {
                ++m_statNotInlined;
                തിരികെ അസത്യം;
            }
        }
        തിരികെ സത്യം;
    }

    ശൂന്യം recordSubstitution(AstVarScope* vscp, Ast_കണ്ണി_വ്യഞ്ജകം* substp, Ast_കണ്ണി* logicp) {
        m_hasPending.emplace(logicp, ++m_ord);  // It's OK if already present
        സ്ഥിരം auto pair = m_substitutions(logicp).emplace(vscp, nullptr);
        എങ്കിൽ (pair.second) pair.first->second = substp->clone_വൃക്ഷം_Pure(അസത്യം);
    }

    ശൂന്യം commitSubstitutions(Ast_കണ്ണി* logicp) {
        എങ്കിൽ (!m_hasPending.erase(logicp)) തിരികെ;  // Had no pending substitutions

        Substitutions& substitutions = m_substitutions(logicp);
        UASSERT_OBJ(!substitutions.empty(), logicp, "No pending substitutions");

        // Recursion filter holding already replaced variables
        std::unordered_set<സ്ഥിരം AstVarScope*> replaced(substitutions.size() * 2);

        സ്ഥിരം std::function<ശൂന്യം(Ast_കണ്ണി_Var_Ref*)> visit = [&](Ast_കണ്ണി_Var_Ref* nodep) -> ശൂന്യം {
            // See if this variable has a substitution
            AstVarScope* സ്ഥിരം vscp = nodep->varScopep();
            സ്ഥിരം auto& it = substitutions.find(vscp);
            എങ്കിൽ (it == substitutions.end()) തിരികെ;

            // Do not substitute circular logic
            എങ്കിൽ (!replaced.തിരുകുക(vscp).second) തിരികെ;

            // Substitute nodep with substp
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം substp = it->second;

            UASSERT_OBJ(nodep->access().isReadOnly(), nodep, "Can't replace write references");
            UASSERT_OBJ(!VN_IS(substp, കണ്ണി_Var_Ref) || !nodep->isSame(substp), substp,
                        "Replacing node with itself; perhaps circular logic?");
            // The replacement
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = substp->clone_വൃക്ഷം_Pure(അസത്യം);
            // Which fileline() to use? If replacing with logic, an error/warning is likely to want
            // to point to the logic IE what we're replacing with. However, a VARREF should point
            // to the original as it's otherwise confusing to throw warnings that point to a PIN
            // rather than where the pin us used.
            എങ്കിൽ (VN_IS(newp, VarRef)) newp->fileline(nodep->fileline());
            // Make the newp an rvalue like nodep.
            എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(newp, കണ്ണി_Var_Ref)) {
                varrefp->access(VAccess::READ);
            }
            // Replace the node
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            // Recursively substitute the new tree
            newp->foreach(visit);

            // Remove from recursion filter
            replaced.erase(vscp);
        };

        logicp->foreach(visit);

        Ast_കണ്ണി* സ്ഥിരം simplifiedp = V3Const::constifyEdit(logicp);
        UASSERT_OBJ(simplifiedp == logicp, simplifiedp, "Should not remove whole logic");
        വേണ്ടി (സ്ഥിരം auto& pair : substitutions) pair.second->delete_വൃക്ഷം();
        substitutions.clear();
    }

    ശൂന്യം optimizeSignals(തർക്കം allowMultiIn) {
        // Consider "inlining" variables
        auto& vertices = m_graph.vertices();
        സ്ഥിരം auto ffToVarVtx = [&](V3GraphVertex::List::iterator it) {
            ആകുമ്പോൾ (it != vertices.end() && !(*it).is<GateVarVertex>()) ++it;
            തിരികെ it;
        };
        V3GraphVertex::List::iterator vIt = ffToVarVtx(vertices.begin());
        ആകുമ്പോൾ (vIt != vertices.end()) {
            GateVarVertex* സ്ഥിരം vVtxp = (*vIt).as<GateVarVertex>();
            // vVtxp and it's driving logic might be deleted, so grab next up front
            vIt = ffToVarVtx(++vIt);

            // Nothing to inline if no driver, or multiple drivers ...
            എങ്കിൽ (!vVtxp->inSize1()) തുടരുക;

            // Can't inline if non-reducible, etc
            എങ്കിൽ (!vVtxp->reducible()) തുടരുക;

            // Grab the driving logic
            GateLogicVertex* സ്ഥിരം lVtxp
                = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
            എങ്കിൽ (!lVtxp->reducible()) തുടരുക;
            Ast_കണ്ണി* സ്ഥിരം logicp = lVtxp->nodep();

            // Commit pending optimizations to driving logic, as we will re-analyze
            commitSubstitutions(logicp);

            // Can we eliminate?
            സ്ഥിരം GateOkVisitor okVisitor{logicp, അസത്യം, അസത്യം};

            // Was it ok?
            എങ്കിൽ (!okVisitor.isSimple()) തുടരുക;
            // If the varScope is already removed from logicp, no need to try substitution.
            എങ്കിൽ (!okVisitor.varAssigned(vVtxp->varScp())) തുടരുക;

            // Expression we are considering to substitute with
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം substp = V3Const::constifyEdit(okVisitor.substitutionp());
            // Number of variables read by the substitution
            സ്ഥിരം size_t nReads = okVisitor.readVscps().size();

            // Check if it should be inlined
            എങ്കിൽ (!shouldInline(vVtxp, lVtxp, nReads, substp, allowMultiIn)) തുടരുക;

            // Process it
            ++m_statInlined;

            // Variable we are considering to substitute
            AstVarScope* സ്ഥിരം vscp = vVtxp->varScp();

            എങ്കിൽ (debug() >= 9) {
                vscp->dump_വൃക്ഷം("substituting: ");
                substp->dump_വൃക്ഷം("        with: ");
            }

            സ്ഥിരം auto& readVscpsVec = okVisitor.readVscps();
            സ്ഥിരം std::unordered_set<AstVarScope*> readVscps{readVscpsVec.begin(),
                                                             readVscpsVec.end()};

            വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : vVtxp->outEdges().unlinkable()) {
                GateLogicVertex* സ്ഥിരം dstVtxp = edgep->മുകൾ()->as<GateLogicVertex>();

                // Do not inline anything other than buffers and inverters into
                // sensitivity lists. If the signal becomes constant, we might
                // miss an initialization time edge.
                എങ്കിൽ (VN_IS(dstVtxp->nodep(), SenItem)) {
                    Ast_കണ്ണി* nodep = substp;
                    എങ്കിൽ (AstNot* സ്ഥിരം notp = VN_CAST(nodep, Not)) nodep = notp->lhsp();
                    എങ്കിൽ (!VN_IS(nodep, VarRef)) തുടരുക;
                }

                // If the consumer logic writes one of the variables that the substitution
                // is reading, then we would get a cycles, so we cannot do that.
                തർക്കം canInline = സത്യം;
                വേണ്ടി (സ്ഥിരം V3GraphEdge& dedge : dstVtxp->outEdges()) {
                    സ്ഥിരം GateVarVertex* സ്ഥിരം consVVertexp = dedge.മുകൾ()->as<GateVarVertex>();
                    എങ്കിൽ (readVscps.എണ്ണം(consVVertexp->varScp())) {
                        canInline = അസത്യം;
                        നിർത്തുക;
                    }
                }

                എങ്കിൽ (!canInline) തുടരുക;  // Cannot optimize this replacement

                UINFOTREE(9, dstVtxp->nodep(), "", "inside");

                എങ്കിൽ (logicp == dstVtxp->nodep()) {
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
                    തുടരുക;
                }

                recordSubstitution(vscp, substp, dstVtxp->nodep());

                // If the new replacement referred to a signal,
                // Correct the graph to point to this new generating variable
                വേണ്ടി (AstVarScope* സ്ഥിരം newVscp : okVisitor.readVscps()) {
                    GateVarVertex* സ്ഥിരം varvertexp = m_graph.makeVarVertex(newVscp);
                    m_graph.addEdge(varvertexp, dstVtxp, 1);
                    // Propagate clock attribute onto generating node
                    varvertexp->propagateAttrFrom(vVtxp);
                }

                // Remove the edge
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statRefs;
            }

            // If removed all usage
            എങ്കിൽ (vVtxp->outEmpty()) {
                // Remove Variable vertex
                VL_DO_DANGLING(vVtxp->unlinkDelete(&m_graph), vVtxp);
                // Remove driving logic and vertex
                VL_DO_DANGLING(logicp->unlinkFrBack()->delete_വൃക്ഷം(), logicp);
                VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
            }
        }
    }

    explicit GateInline(GateGraph& graph)
        : m_graph{graph} {
        // Find gate interconnect and optimize
        graph.userClearVertices();  // vertex->user(): bool. Indicates we've set it as consumed
        // Get rid of buffers first,
        optimizeSignals(അസത്യം);
        // Then propagate more complicated equations
        optimizeSignals(സത്യം);
        // Commit substitutions in insertion order for stability
        using Pair = std::pair<Ast_കണ്ണി*, size_t>;
        std::vector<Pair> pending{m_hasPending.begin(), m_hasPending.end()};
        std::sort(pending.begin(), pending.end(), [](സ്ഥിരം Pair& a, സ്ഥിരം Pair& b) {  //
            തിരികെ a.second < b.second;
        });
        വേണ്ടി (സ്ഥിരം auto& pair : pending) commitSubstitutions(pair.first);
    }

    ~GateInline() {
        V3Stats::addStat("Optimizations, Gate signals inlined", m_statInlined);
        V3Stats::addStat("Optimizations, Gate signals not inlined due to cost", m_statNotInlined);
        V3Stats::addStat("Optimizations, Gate reads replaced", m_statRefs);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(GateGraph& graph) { GateInline{graph}; }
};

//######################################################################
// Auxiliary hash class for GateDedupeVarVisitor

വർഗ്ഗം GateDedupeHash final : പൊതു V3DupFinderUserSame {
    // NODE STATE
    // VNUser2InUse    m_inuser2;      (Allocated in GateDedupe)
    ഘടന Aux_Ast_കണ്ണി_വ്യഞ്ജകം final {
        // AstActive* of assign, for isSame() in test for duplicate. Set to nullptr if this
        // assign's tree was later replaced
        AstActive* activep = nullptr;
        // AstNodeExpr* of assign if condition, for isSame() in test for duplicate. Set to nullptr
        // if this assign's tree was later replaced
        Ast_കണ്ണി_വ്യഞ്ജകം* condp = nullptr;
        // Parent AstNodeAssign* for this rhsp
        Ast_കണ്ണി_Assign* parentp = nullptr;
    };
    AstUser2Allocator<Ast_കണ്ണി_വ്യഞ്ജകം, Aux_Ast_കണ്ണി_വ്യഞ്ജകം> m_aux_കണ്ണി_വ്യഞ്ജകം;

    Ast_കണ്ണി_വ്യഞ്ജകം* m_currRhsp = nullptr;  // Current node we are searching for duplicates of
    Aux_Ast_കണ്ണി_വ്യഞ്ജകം m_auxCurRhsp;  // Aux of current node

    V3DupFinder m_dupFinder;  // Duplicate finder for rhs of assigns

    സ്ഥിതം തർക്കം same(Ast_കണ്ണി* node1p, Ast_കണ്ണി* node2p) {
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
        തിരികെ node1p == node2p || (node1p && node1p->same_വൃക്ഷം(node2p));
    }

    // Callback from V3DupFinder::findDuplicate
    തർക്കം isSame(Ast_കണ്ണി* node1p, Ast_കണ്ണി* node2p) override {
        UASSERT_OBJ(node1p == m_currRhsp, m_currRhsp, "Comparing to unexpected node");
        സ്ഥിരം auto& aux2 = m_aux_കണ്ണി_വ്യഞ്ജകം(VN_AS(node2p, കണ്ണി_വ്യഞ്ജകം));
        തിരികെ m_auxCurRhsp.parentp->type() == aux2.parentp->type()  //
               && same(m_auxCurRhsp.activep, aux2.activep)  //
               && same(m_auxCurRhsp.condp, aux2.condp);
    }

പൊതു:
    GateDedupeHash() = സ്ഥിരസ്ഥിതി;
    ~GateDedupeHash() = സ്ഥിരസ്ഥിതി;

    സ്ഥിരം Ast_കണ്ണി_Assign* hashAndFindDupe(Ast_കണ്ണി_Assign* assignp, AstActive* activep,
                                         Ast_കണ്ണി_വ്യഞ്ജകം* condp) {
        // Legal for activep to be nullptr, we'll compare with other assigns with also nullptr
        m_currRhsp = assignp->rhsp();
        m_auxCurRhsp.activep = activep;
        m_auxCurRhsp.condp = condp;
        m_auxCurRhsp.parentp = assignp;

        // Check for a duplicate, if found return its assignment
        സ്ഥിരം auto it = m_dupFinder.findDuplicate(m_currRhsp, ഇത്);
        എങ്കിൽ (it != m_dupFinder.end()) തിരികെ m_aux_കണ്ണി_വ്യഞ്ജകം(VN_AS(it->second, കണ്ണി_വ്യഞ്ജകം)).parentp;

        // Insert new node
        m_dupFinder.തിരുകുക(m_currRhsp);
        m_aux_കണ്ണി_വ്യഞ്ജകം(m_currRhsp) = m_auxCurRhsp;
        തിരികെ nullptr;
    }
};

//######################################################################
// Have we seen the rhs of this assign before?

വർഗ്ഗം GateDedupeVarVisitor final : പൊതു VNVisitorConst {
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
    Ast_കണ്ണി_Assign* m_assignp = nullptr;  // Assign found for dedupe
    Ast_കണ്ണി_വ്യഞ്ജകം* m_ifCondp = nullptr;  // IF condition that assign is under
    തർക്കം m_always = അസത്യം;  // Assign is under an always
    തർക്കം m_dedupable = സത്യം;  // Determined the assign to be dedupable

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
        എങ്കിൽ (!m_dedupable) തിരികെ;

        // I think we could safely dedupe an always block with multiple
        // non-blocking statements, but erring on side of caution here
        എങ്കിൽ (!m_assignp) {
            m_assignp = nodep;
            m_dedupable = !nodep->exists([](Ast_വ്യഞ്ജകം_Stmt*) { തിരികെ സത്യം; });
            തിരികെ;
        }

        m_dedupable = അസത്യം;
    }

    ശൂന്യം visit(AstAlways* nodep) override {
        എങ്കിൽ (!m_dedupable) തിരികെ;

        എങ്കിൽ (!m_always) {
            m_always = സത്യം;
            iterateAndNextConstNull(nodep->stmtsp());
            തിരികെ;
        }

        m_dedupable = അസത്യം;
    }

    // Ugly support for latches of the specific form -
    //  always @(...)
    //    if (...)
    //       foo = ...; // or foo <= ...;
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        എങ്കിൽ (!m_dedupable) തിരികെ;

        എങ്കിൽ (m_always && !m_ifCondp && !nodep->elsesp()) {
            // we're under an always, this is the first IF, and there's no else
            m_ifCondp = nodep->condp();
            m_dedupable = !m_ifCondp->exists([](Ast_വ്യഞ്ജകം_Stmt*) { തിരികെ സത്യം; });
            iterateAndNextConstNull(nodep->thensp());
            തിരികെ;
        }

        m_dedupable = അസത്യം;
    }

    ശൂന്യം visit(AstComment*) override {}  // NOP
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി*) override { m_dedupable = അസത്യം; }

പൊതു:
    // CONSTRUCTORS
    GateDedupeVarVisitor() = സ്ഥിരസ്ഥിതി;
    ~GateDedupeVarVisitor() override = സ്ഥിരസ്ഥിതി;
    // PUBLIC METHODS
    Ast_കണ്ണി_Var_Ref* findDupe(Ast_കണ്ണി* logicp, AstVarScope* consumerVscp, AstActive* activep) {
        m_assignp = nullptr;
        m_ifCondp = nullptr;
        m_always = അസത്യം;
        m_dedupable = സത്യം;
        iterateConst(logicp);
        എങ്കിൽ (m_dedupable && m_assignp) {
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം lhsp = m_assignp->lhsp();
            // Possible todo, handle more complex lhs expressions
            // A logic vertex may also contain a function argument assignment.
            സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം lRefp = VN_CAST(lhsp, കണ്ണി_Var_Ref);
            എങ്കിൽ (lRefp && lRefp->varScopep() == consumerVscp) {
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Assign* സ്ഥിരം dup
                    = m_ghash.hashAndFindDupe(m_assignp, activep, m_ifCondp)) {
                    തിരികെ static_cast<Ast_കണ്ണി_Var_Ref*>(dup->lhsp());
                }
            }
        }
        തിരികെ nullptr;
    }
};

//######################################################################
// Recurse through the graph, looking for duplicate expressions on the rhs of an assign

വർഗ്ഗം GateDedupe final {
    // NODE STATE
    // AstVarScope::user2p      -> bool: already visited
    സ്ഥിരം VNUser2InUse m_inuser2;

    // STATE
    size_t m_statDedupLogic = 0;  // Statistic tracking
    GateDedupeVarVisitor m_varVisitor;  // Looks for a dupe of the logic
    uint32_t m_depth = 0;  // Iteration depth

    ശൂന്യം visit(GateVarVertex* vVtxp) {
        // Break loops; before user2 set so hit this vertex later
        എങ്കിൽ (m_depth > GATE_DEDUP_MAX_DEPTH) തിരികെ;

        // Check that we haven't been here before
        എങ്കിൽ (vVtxp->varScp()->user2SetOnce()) തിരികെ;

        VL_RESTORER(m_depth);
        ++m_depth;
        എങ്കിൽ (!vVtxp->inSize1()) തിരികെ;

        Ast_കണ്ണി_Var_Ref* dupRefp = nullptr;
        വേണ്ടി (V3GraphEdge& edge : vVtxp->inEdges()) {
            dupRefp = visit(edge.fromp()->as<GateLogicVertex>(), vVtxp);
        }
        എങ്കിൽ (!dupRefp) തിരികെ;

        UASSERT_OBJ(vVtxp->dedupable(), vVtxp->varScp(),
                    "GateLogicVertex* visit should have returned nullptr "
                    "if consumer var vertex is not dedupable.");

        GateLogicVertex* സ്ഥിരം lVtxp = vVtxp->inEdges().frontp()->fromp()->as<GateLogicVertex>();
        സ്ഥിരം GateOkVisitor okVisitor{lVtxp->nodep(), അസത്യം, സത്യം};
        എങ്കിൽ (!okVisitor.isSimple()) തിരികെ;

        ++m_statDedupLogic;
        GateVarVertex* സ്ഥിരം dupVVtxp = dupRefp->varScopep()->user1u().to<GateVarVertex*>();
        UINFO(4, "replacing " << vVtxp << " with " << dupVVtxp);

        // Replace all of this varvertex's consumers with dupRefp
        വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : vVtxp->outEdges().unlinkable()) {
            സ്ഥിരം GateLogicVertex* സ്ഥിരം consumerVtxp = edgep->മുകൾ()->as<GateLogicVertex>();
            Ast_കണ്ണി* സ്ഥിരം consumerp = consumerVtxp->nodep();
            UINFO(9, "elim src vtx" << lVtxp << " node " << lVtxp->nodep());
            UINFO(9, "elim cons vtx" << consumerVtxp << " node " << consumerp);
            UINFO(9, "elim var vtx " << vVtxp << " node " << vVtxp->varScp());
            UINFO(9, "replace with " << dupRefp);
            എങ്കിൽ (lVtxp == consumerVtxp) {
                UINFO(9, "skipping as self-recirculates");
            } അല്ലെങ്കിൽ {
                // Substitute consumer logic
                consumerp->foreach([&](Ast_കണ്ണി_Var_Ref* refp) {
                    എങ്കിൽ (refp->varScopep() != vVtxp->varScp()) തിരികെ;

                    UASSERT_OBJ(refp->access().isReadOnly(), refp, "Can't replace a write ref");

                    // The replacement
                    Ast_കണ്ണി_Var_Ref* സ്ഥിരം newp = dupRefp->clone_വൃക്ഷം_Pure(അസത്യം);
                    // A VARREF should point to the original as it's otherwise confusing to throw
                    // warnings that point to a PIN rather than where the pin is used.
                    newp->fileline(refp->fileline());
                    newp->access(VAccess::READ);

                    // Replace the node
                    refp->replaceWith(newp);
                    VL_DO_DANGLING(refp->delete_വൃക്ഷം(), refp);
                });
            }
            edgep->relinkFromp(dupVVtxp);
        }

        // Remove inputs links
        ആകുമ്പോൾ (V3GraphEdge* സ്ഥിരം edgep = vVtxp->inEdges().frontp()) {
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }

        // Propagate attributes
        dupVVtxp->propagateAttrFrom(vVtxp);
    }

    // Given iterated logic, starting at consumerVtxp, returns a varref that
    // has the same logic input, or nullptr if none
    Ast_കണ്ണി_Var_Ref* visit(GateLogicVertex* lVtxp, സ്ഥിരം GateVarVertex* consumerVtxp) {
        വേണ്ടി (V3GraphEdge& edge : lVtxp->inEdges()) visit(edge.fromp()->as<GateVarVertex>());

        എങ്കിൽ (lVtxp->dedupable() && consumerVtxp->dedupable()) {
            // TODO: Doing a simple pointer comparison of activep won't work
            // optimally for statements under generated clocks. Statements under
            // different generated clocks will never compare as equal, even if the
            // generated clocks are deduped into one clock.
            തിരികെ m_varVisitor.findDupe(lVtxp->nodep(), consumerVtxp->varScp(), lVtxp->activep());
        }
        തിരികെ nullptr;
    }

    explicit GateDedupe(GateGraph& graph) {
        // Traverse starting from each of the clocks
        UINFO(9, "Gate dedupe() clocks:");
        വേണ്ടി (V3GraphVertex& vtx : graph.vertices()) {
            എങ്കിൽ (GateVarVertex* സ്ഥിരം vVtxp = vtx.cast<GateVarVertex>()) {
                എങ്കിൽ (vVtxp->isClock()) visit(vVtxp);
            }
        }
        // Traverse starting from each of the outputs
        UINFO(9, "Gate dedupe() outputs:");
        വേണ്ടി (V3GraphVertex& vtx : graph.vertices()) {
            എങ്കിൽ (GateVarVertex* സ്ഥിരം vVtxp = vtx.cast<GateVarVertex>()) {
                എങ്കിൽ (vVtxp->is_മുകൾ() && vVtxp->varScp()->varp()->isWritable()) visit(vVtxp);
            }
        }
    }

    ~GateDedupe() { V3Stats::addStat("Optimizations, Gate sigs deduped", m_statDedupLogic); }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(GateGraph& graph) { GateDedupe{graph}; }
};

//######################################################################
// Recurse through the graph, try to merge assigns

വർഗ്ഗം GateMergeAssignments final {
    GateGraph& m_graph;
    size_t m_statAssignMerged = 0;  // Statistic tracking
    std::vector<GateLogicVertex*> m_toRemove;  // Logic vertices to delete

    // assemble two Sel into one if possible
    AstSel* merge(AstSel* prevSelp, AstSel* currSelp) {
        സ്ഥിരം AstVarRef* സ്ഥിരം pRefp = VN_CAST(prevSelp->fromp(), VarRef);
        AstVarRef* സ്ഥിരം cRefp = VN_CAST(currSelp->fromp(), VarRef);
        എങ്കിൽ (!pRefp || !cRefp || !cRefp->same_കണ്ണി(pRefp)) തിരികെ nullptr;  // not the same var

        സ്ഥിരം AstConst* സ്ഥിരം pstart = VN_CAST(prevSelp->lsbp(), Const);
        സ്ഥിരം AstConst* സ്ഥിരം cstart = VN_CAST(currSelp->lsbp(), Const);
        എങ്കിൽ (!pstart || !cstart) തിരികെ nullptr;  // too complicated

        എങ്കിൽ (currSelp->msbConst() + 1 == prevSelp->lsbConst()) {
            തിരികെ പുതിയ AstSel{cRefp->fileline(), cRefp->clone_വൃക്ഷം(അസത്യം), currSelp->lsbConst(),
                              prevSelp->widthConst() + currSelp->widthConst()};
        } അല്ലെങ്കിൽ {
            തിരികെ nullptr;
        }
    }

    ശൂന്യം process(GateVarVertex* vVtxp) {
        GateLogicVertex* prevLVtxp = nullptr;
        AstAssignW* prevAssignp = nullptr;

        വേണ്ടി (V3GraphEdge* സ്ഥിരം edgep : vVtxp->inEdges().unlinkable()) {
            GateLogicVertex* സ്ഥിരം lVtxp = edgep->fromp()->as<GateLogicVertex>();
            എങ്കിൽ (!lVtxp->outSize1()) തുടരുക;

            AstAlways* സ്ഥിരം alwaysp = VN_CAST(lVtxp->nodep(), Always);
            എങ്കിൽ (!alwaysp || !alwaysp->stmtsp() || alwaysp->stmtsp()->nextp()) തിരികെ;
            AstAssignW* സ്ഥിരം assignp = VN_CAST(alwaysp->stmtsp(), AssignW);
            എങ്കിൽ (!assignp) തുടരുക;

            എങ്കിൽ (!VN_IS(assignp->lhsp(), Sel)) തുടരുക;

            // First assign with Sel-lhs, or not under the same active
            എങ്കിൽ (!prevLVtxp || prevLVtxp->activep() != lVtxp->activep()) {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
                തുടരുക;
            }

            AstSel* സ്ഥിരം prevSelp = VN_AS(prevAssignp->lhsp(), Sel);
            AstSel* സ്ഥിരം currSelp = VN_AS(assignp->lhsp(), Sel);

            എങ്കിൽ (AstSel* സ്ഥിരം newSelp = merge(prevSelp, currSelp)) {
                UINFO(5, "assemble to new sel: " << newSelp);
                // replace preSel with newSel
                prevSelp->replaceWith(newSelp);
                VL_DO_DANGLING(prevSelp->delete_വൃക്ഷം(), prevSelp);
                // Update RHS of the prev assignment, reusing existing parts (might be impure).
                prevAssignp->rhsp(പുതിയ AstConcat{prevAssignp->rhsp()->fileline(),
                                                prevAssignp->rhsp()->unlinkFrBack(),
                                                assignp->rhsp()->unlinkFrBack()});
                // Why do we care about the type of an assignment?
                prevAssignp->dtypeChgWidthSigned(prevAssignp->width() + assignp->width(),
                                                 prevAssignp->width() + assignp->width(),
                                                 VSigning::SIGNED);
                // We will delete the current assignment
                m_toRemove.emplace_back(lVtxp);

                // Update the graph
                ആകുമ്പോൾ (V3GraphEdge* സ്ഥിരം iedgep = lVtxp->inEdges().frontp()) {
                    GateVarVertex* സ്ഥിരം fromVtxp = iedgep->fromp()->as<GateVarVertex>();
                    m_graph.addEdge(fromVtxp, prevLVtxp, 1);
                    VL_DO_DANGLING(iedgep->unlinkDelete(), iedgep);
                }

                // Delete the out-edges of lVtxp (there is only one, we checked earlier)
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
                ++m_statAssignMerged;
            } അല്ലെങ്കിൽ {
                prevLVtxp = lVtxp;
                prevAssignp = assignp;
            }
        }
    }

    explicit GateMergeAssignments(GateGraph& graph)
        : m_graph{graph} {
        UINFO(6, "mergeAssigns");
        വേണ്ടി (V3GraphVertex& vtx : graph.vertices()) {
            എങ്കിൽ (GateVarVertex* സ്ഥിരം vVtxp = vtx.cast<GateVarVertex>()) process(vVtxp);
        }
        // Delete merged assignments
        വേണ്ടി (GateLogicVertex* സ്ഥിരം lVtxp : m_toRemove) {
            Ast_കണ്ണി* സ്ഥിരം nodep = lVtxp->nodep();
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            VL_DO_DANGLING(lVtxp->unlinkDelete(&m_graph), lVtxp);
        }
    }

    ~GateMergeAssignments() {
        V3Stats::addStat("Optimizations, Gate assign merged", m_statAssignMerged);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(GateGraph& graph) { GateMergeAssignments{graph}; }
};

//######################################################################
// GateUnused

വർഗ്ഗം GateUnused final {
    // STATE
    GateGraph& m_graph;

    // METHODS

    ശൂന്യം markRecurse(GateEitherVertex* vtxp) {
        എങ്കിൽ (vtxp->user()) തിരികെ;  // Already marked
        vtxp->user(സത്യം);
        vtxp->setConsumed("propagated");
        // Walk sources and mark them too
        വേണ്ടി (V3GraphEdge& edge : vtxp->inEdges()) {
            GateEitherVertex* സ്ഥിരം fromVtxp = static_cast<GateEitherVertex*>(edge.fromp());
            markRecurse(fromVtxp);
        }
    }

    // Mark all vertices that feed a consumed vertex
    ശൂന്യം mark() {
        m_graph.userClearVertices();
        വേണ്ടി (V3GraphVertex& vtx : m_graph.vertices()) {
            GateEitherVertex& eVtx = static_cast<GateEitherVertex&>(vtx);
            എങ്കിൽ (eVtx.consumed()) markRecurse(&eVtx);
        }
    }

    സ്ഥിതം ശൂന്യം warnUnused(സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep) {
        എങ്കിൽ (nodep->fileline()->warnIsOff(V3ErrorCode::UNUSEDLOOP)) തിരികെ;

        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Procedure* സ്ഥിരം procedurep = VN_CAST(nodep, കണ്ണി_Procedure)) {
            എങ്കിൽ (procedurep->stmtsp())
                procedurep->stmtsp()->foreach([](സ്ഥിരം AstLoop* സ്ഥിരം loopp) {  //
                    loopp->v3warn(UNUSEDLOOP, "Loop is not used and will be optimized out");
                    loopp->fileline()->modifyWarnOff(V3ErrorCode::UNUSEDLOOP, സത്യം);
                });
        }
    }

    // Remove unused logic
    ശൂന്യം remove() {
        വേണ്ടി (V3GraphVertex* സ്ഥിരം vtxp : m_graph.vertices().unlinkable()) {
            എങ്കിൽ (GateLogicVertex* സ്ഥിരം lVtxp = vtxp->cast<GateLogicVertex>()) {
                എങ്കിൽ (!lVtxp->consumed() && lVtxp->activep()) {  // activep is nullptr under cfunc
                    Ast_കണ്ണി* സ്ഥിരം nodep = lVtxp->nodep();
                    warnUnused(nodep);

                    UINFO(8, "    Remove unconsumed " << nodep);
                    nodep->unlinkFrBack();
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
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

പൊതു:
    സ്ഥിതം ശൂന്യം apply(GateGraph& graph) { GateUnused{graph}; }
};

//######################################################################
// Pass entry point

ശൂന്യം V3Gate::gateAll(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");

    {
        // Build the graph
        std::unique_ptr<GateGraph> graphp = GateBuildVisitor::apply(netlistp);
        എങ്കിൽ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_graph");

        // Warn, before loss of sync/async pointers
        v3GateWarnSyncAsync(*graphp);

        // Remove redundant edges. Edge weighs are added, so a variable read twice by
        // the same logic block will have and edge to the logic block with weight 2
        graphp->removeRedundantEdgesSum(&V3GraphEdge::followAlwaysTrue);
        എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_simp");

        // Remove unused logic
        GateUnused::apply(*graphp);
        എങ്കിൽ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_unused");

        // Inline variables
        GateInline::apply(*graphp);
        എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_inline");

        // Remove redundant logic
        എങ്കിൽ (v3Global.opt.fDedupe()) {
            GateDedupe::apply(*graphp);
            എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_dedup");
        }

        // Merge assignments
        എങ്കിൽ (v3Global.opt.fAssemble()) {
            GateMergeAssignments::apply(*graphp);
            എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("gate_merge");
        }

        // Remove unused logic
        GateUnused::apply(*graphp);
        എങ്കിൽ (dumpGraphLevel() >= 3) graphp->dumpDotFilePrefixed("gate_final");
    }

    V3Global::dump_Check_Global_വൃക്ഷം("gate", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
