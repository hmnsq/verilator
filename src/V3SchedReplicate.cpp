// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Scheduling - replicate combinational logic
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
//
// Combinational (including hybrid) logic driven from both the 'act' and 'nba'
// region needs to be re-evaluated even if only one of those regions updates
// an input variable. We achieve this by replicating such combinational logic
// in both the 'act' and 'nba' regions.
//
// Furthermore we also replicate all combinational logic driven from a top
// level input into a separate 'ico' (Input Combinational) region which is
// executed at the beginning of the time step. This allows us to change both
// data and clock signals during the same 'eval' call while maintaining the
// combinational invariant required by V3Order.
//
// The implementation is a simple graph algorithm, where we build a dependency
// graph of all logic in the design, and then propagate the driving region
// information through it. We then replicate any logic into its additional
// driving regions.
//
// For more details, please see the internals documentation.
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {

નામાકાશ {

// Driving region flags
ગણના RegionFlags : uint8_t {
    NONE = 0x0,  //
    આગત = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

વર્ગ SchedReplicateVertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

જાહેર:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() અચલ { પાછા m_drivingRegions; }
    શૂન્ય addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() અચલ override {
        પસંદગી (static_cast<unsigned>(m_drivingRegions)) {
        સ્થિતિ NONE: પાછા "black";
        સ્થિતિ આગત: પાછા "red";
        સ્થિતિ ACTIVE: પાછા "green";
        સ્થિતિ NBA: પાછા "blue";
        સ્થિતિ આગત | ACTIVE: પાછા "yellow";
        સ્થિતિ આગત | NBA: પાછા "magenta";
        સ્થિતિ ACTIVE | NBA: પાછા "cyan";
        સ્થિતિ આગત | ACTIVE | NBA: પાછા "gray80";  // don't want white on white background

        સ્થિતિ REACTIVE: પાછા "gray60";
        સ્થિતિ REACTIVE | આગત: પાછા "lightcoral";
        સ્થિતિ REACTIVE | ACTIVE: પાછા "lightgreen";
        સ્થિતિ REACTIVE | NBA: પાછા "lightblue";
        સ્થિતિ REACTIVE | આગત | ACTIVE: પાછા "lightyellow";
        સ્થિતિ REACTIVE | આગત | NBA: પાછા "lightpink";
        સ્થિતિ REACTIVE | ACTIVE | NBA: પાછા "lightcyan";
        સ્થિતિ REACTIVE | આગત | ACTIVE | NBA: પાછા "gray90";

        સ્થિતિ OBSERVED: પાછા "gray20";
        સ્થિતિ OBSERVED | આગત: પાછા "darkred";
        સ્થિતિ OBSERVED | ACTIVE: પાછા "darkgreen";
        સ્થિતિ OBSERVED | NBA: પાછા "darkblue";
        સ્થિતિ OBSERVED | આગત | ACTIVE: પાછા "gold";
        સ્થિતિ OBSERVED | આગત | NBA: પાછા "purple";
        સ્થિતિ OBSERVED | ACTIVE | NBA: પાછા "darkcyan";
        સ્થિતિ OBSERVED | આગત | ACTIVE | NBA: પાછા "gray30";

        સ્થિતિ REACTIVE | OBSERVED: પાછા "gray40";
        સ્થિતિ REACTIVE | OBSERVED | આગત: પાછા "indianred";
        સ્થિતિ REACTIVE | OBSERVED | ACTIVE: પાછા "olive";
        સ્થિતિ REACTIVE | OBSERVED | NBA: પાછા "dodgerBlue";
        સ્થિતિ REACTIVE | OBSERVED | આગત | ACTIVE: પાછા "khaki";
        સ્થિતિ REACTIVE | OBSERVED | આગત | NBA: પાછા "plum";
        સ્થિતિ REACTIVE | OBSERVED | ACTIVE | NBA: પાછા "lightSeaGreen";
        સ્થિતિ REACTIVE | OBSERVED | આગત | ACTIVE | NBA: પાછા "gray50";
        મૂળભૂત: v3fatalSrc("There are only 5 region bits"); પાછા "";
        }
    }
    // LCOV_EXCL_STOP
};

વર્ગ SchedReplicateLogicVertex final : જાહેર SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* અચલ m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_વૃક્ષ* અચલ m_senTreep;  // The sensitivity of the logic node
    Ast_ગાંઠ* અચલ m_logicp;  // The logic node this vertex represents
    RegionFlags અચલ m_assignedRegion;  // The region this logic is originally assigned to

જાહેર:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_વૃક્ષ* senTreep,
                              Ast_ગાંઠ* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() અચલ { પાછા m_scopep; }
    Ast_Sen_વૃક્ષ* senTreep() અચલ { પાછા m_senTreep; }
    Ast_ગાંઠ* logicp() અચલ { પાછા m_logicp; }
    RegionFlags assignedRegion() અચલ { પાછા m_assignedRegion; }

    // For graph dumping
    string name() અચલ override VL_MT_STABLE { પાછા m_logicp->fileline()->ascii(); };
    string dotShape() અચલ override { પાછા "rectangle"; }
};

વર્ગ SchedReplicateVarVertex final : જાહેર SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* અચલ m_vscp;  // The AstVarScope this vertex represents

જાહેર:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        જો (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(આગત);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        જો (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() અચલ { પાછા m_vscp; }
    AstVar* varp() અચલ { પાછા m_vscp->varp(); }
    AstScope* scopep() અચલ { પાછા m_vscp->scopep(); }

    // For graph dumping
    string name() અચલ override VL_MT_STABLE { પાછા m_vscp->name(); }
    string dotShape() અચલ override { પાછા varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

વર્ગ Graph final : જાહેર V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(અચલ LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{નવું Graph};

    // AstVarScope::user1() -> VarVertx
    અચલ VNUser1InUse user1InUse;
    અચલ auto getVarVertex = [&](AstVarScope* vscp) {
        જો (!vscp->user1p()) vscp->user1p(નવું SchedReplicateVarVertex{graphp.get(), vscp});
        પાછા vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    અચલ auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* ટોચ) {
        નવું V3GraphEdge{graphp.get(), fromp, ટોચ, 1};
    };

    અચલ auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_વૃક્ષ* અચલ senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<તાર્કિક(AstVarScope*)> readTriggersThisLogic;

        અચલ VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        જો (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { પાછા ખોટું; };
        } નહીંતર જો (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { પાછા સાચું; };
        } નહીંતર {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { પાછા !vscp->user4(); };
            senTreep->foreach([](અચલ AstVarRef* refp) {  //
                refp->varScopep()->user4(સાચું);
            });
        }

        માટે (Ast_ગાંઠ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* અચલ lvtxp
                = નવું SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            અચલ VNUser2InUse user2InUse;
            અચલ VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* અચલ vscp = refp->varScopep();
                SchedReplicateVarVertex* અચલ vvtxp = getVarVertex(vscp);
                જો (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.ગણતરી(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                જો (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    માટે (અચલ auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    માટે (અચલ auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    માટે (અચલ auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    માટે (અચલ auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    માટે (અચલ auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    પાછા graphp;
}

શૂન્ય propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    જો (vtxp->user()) પાછા;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    માટે (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* અચલ srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(સાચું);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    માટે (V3GraphVertex& vtx : graphp->vertices()) {
        જો (SchedReplicateLogicVertex* અચલ lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            અચલ auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_વૃક્ષ(ખોટું));
            };
            અચલ uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            જો (targetRegions & આગત) replicateTo(result.m_ico);
            જો (targetRegions & ACTIVE) replicateTo(result.m_act);
            જો (targetRegions & NBA) replicateTo(result.m_nba);
            જો (targetRegions & OBSERVED) replicateTo(result.m_obs);
            જો (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    પાછા result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    અચલ std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    માટે (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    પાછા replicate(graphp.get());
}

}  // namespace V3Sched
