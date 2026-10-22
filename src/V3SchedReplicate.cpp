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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ನಾಮಾಕಾಶ {

// Driving region flags
ಗಣನಾ RegionFlags : uint8_t {
    NONE = 0x0,  //
    ಒಳಹರಿವು = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

ವರ್ಗ SchedReplicateVertex VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

ಸಾರ್ವಜನಿಕ:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_drivingRegions; }
    ಶೂನ್ಯ addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ಧ್ರುವ override {
        ವಿಕಲ್ಪ (static_cast<unsigned>(m_drivingRegions)) {
        ಪಕ್ಷ NONE: ಪ್ರತ್ಯಾಗಮ "black";
        ಪಕ್ಷ ಒಳಹರಿವು: ಪ್ರತ್ಯಾಗಮ "red";
        ಪಕ್ಷ ACTIVE: ಪ್ರತ್ಯಾಗಮ "green";
        ಪಕ್ಷ NBA: ಪ್ರತ್ಯಾಗಮ "blue";
        ಪಕ್ಷ ಒಳಹರಿವು | ACTIVE: ಪ್ರತ್ಯಾಗಮ "yellow";
        ಪಕ್ಷ ಒಳಹರಿವು | NBA: ಪ್ರತ್ಯಾಗಮ "magenta";
        ಪಕ್ಷ ACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "cyan";
        ಪಕ್ಷ ಒಳಹರಿವು | ACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "gray80";  // don't want white on white background

        ಪಕ್ಷ REACTIVE: ಪ್ರತ್ಯಾಗಮ "gray60";
        ಪಕ್ಷ REACTIVE | ಒಳಹರಿವು: ಪ್ರತ್ಯಾಗಮ "lightcoral";
        ಪಕ್ಷ REACTIVE | ACTIVE: ಪ್ರತ್ಯಾಗಮ "lightgreen";
        ಪಕ್ಷ REACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "lightblue";
        ಪಕ್ಷ REACTIVE | ಒಳಹರಿವು | ACTIVE: ಪ್ರತ್ಯಾಗಮ "lightyellow";
        ಪಕ್ಷ REACTIVE | ಒಳಹರಿವು | NBA: ಪ್ರತ್ಯಾಗಮ "lightpink";
        ಪಕ್ಷ REACTIVE | ACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "lightcyan";
        ಪಕ್ಷ REACTIVE | ಒಳಹರಿವು | ACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "gray90";

        ಪಕ್ಷ OBSERVED: ಪ್ರತ್ಯಾಗಮ "gray20";
        ಪಕ್ಷ OBSERVED | ಒಳಹರಿವು: ಪ್ರತ್ಯಾಗಮ "darkred";
        ಪಕ್ಷ OBSERVED | ACTIVE: ಪ್ರತ್ಯಾಗಮ "darkgreen";
        ಪಕ್ಷ OBSERVED | NBA: ಪ್ರತ್ಯಾಗಮ "darkblue";
        ಪಕ್ಷ OBSERVED | ಒಳಹರಿವು | ACTIVE: ಪ್ರತ್ಯಾಗಮ "gold";
        ಪಕ್ಷ OBSERVED | ಒಳಹರಿವು | NBA: ಪ್ರತ್ಯಾಗಮ "purple";
        ಪಕ್ಷ OBSERVED | ACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "darkcyan";
        ಪಕ್ಷ OBSERVED | ಒಳಹರಿವು | ACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "gray30";

        ಪಕ್ಷ REACTIVE | OBSERVED: ಪ್ರತ್ಯಾಗಮ "gray40";
        ಪಕ್ಷ REACTIVE | OBSERVED | ಒಳಹರಿವು: ಪ್ರತ್ಯಾಗಮ "indianred";
        ಪಕ್ಷ REACTIVE | OBSERVED | ACTIVE: ಪ್ರತ್ಯಾಗಮ "olive";
        ಪಕ್ಷ REACTIVE | OBSERVED | NBA: ಪ್ರತ್ಯಾಗಮ "dodgerBlue";
        ಪಕ್ಷ REACTIVE | OBSERVED | ಒಳಹರಿವು | ACTIVE: ಪ್ರತ್ಯಾಗಮ "khaki";
        ಪಕ್ಷ REACTIVE | OBSERVED | ಒಳಹರಿವು | NBA: ಪ್ರತ್ಯಾಗಮ "plum";
        ಪಕ್ಷ REACTIVE | OBSERVED | ACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "lightSeaGreen";
        ಪಕ್ಷ REACTIVE | OBSERVED | ಒಳಹರಿವು | ACTIVE | NBA: ಪ್ರತ್ಯಾಗಮ "gray50";
        ಸ್ವಭಾವ: v3fatalSrc("There are only 5 region bits"); ಪ್ರತ್ಯಾಗಮ "";
        }
    }
    // LCOV_EXCL_STOP
};

ವರ್ಗ SchedReplicateLogicVertex final : ಸಾರ್ವಜನಿಕ SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* ಧ್ರುವ m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ m_senTreep;  // The sensitivity of the logic node
    Ast_ಗಂಟು* ಧ್ರುವ m_logicp;  // The logic node this vertex represents
    RegionFlags ಧ್ರುವ m_assignedRegion;  // The region this logic is originally assigned to

ಸಾರ್ವಜನಿಕ:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_ವೃಕ್ಷ* senTreep,
                              Ast_ಗಂಟು* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopep; }
    Ast_Sen_ವೃಕ್ಷ* senTreep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_senTreep; }
    Ast_ಗಂಟು* logicp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_logicp; }
    RegionFlags assignedRegion() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_assignedRegion; }

    // For graph dumping
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_logicp->fileline()->ascii(); };
    string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "rectangle"; }
};

ವರ್ಗ SchedReplicateVarVertex final : ಸಾರ್ವಜನಿಕ SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* ಧ್ರುವ m_vscp;  // The AstVarScope this vertex represents

ಸಾರ್ವಜನಿಕ:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        ಯದಿ (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(ಒಳಹರಿವು);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        ಯದಿ (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp; }
    AstVar* varp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp->varp(); }
    AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp->scopep(); }

    // For graph dumping
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_vscp->name(); }
    string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

ವರ್ಗ Graph final : ಸಾರ್ವಜನಿಕ V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ಧ್ರುವ LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{ನೂತನ Graph};

    // AstVarScope::user1() -> VarVertx
    ಧ್ರುವ VNUser1InUse user1InUse;
    ಧ್ರುವ auto getVarVertex = [&](AstVarScope* vscp) {
        ಯದಿ (!vscp->user1p()) vscp->user1p(ನೂತನ SchedReplicateVarVertex{graphp.get(), vscp});
        ಪ್ರತ್ಯಾಗಮ vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    ಧ್ರುವ auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* ತುದಿ) {
        ನೂತನ V3GraphEdge{graphp.get(), fromp, ತುದಿ, 1};
    };

    ಧ್ರುವ auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<ತಾರ್ಕಿಕ(AstVarScope*)> readTriggersThisLogic;

        ಧ್ರುವ VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        ಯದಿ (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ; };
        } ಅನ್ಯಥಾ ಯದಿ (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; };
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { ಪ್ರತ್ಯಾಗಮ !vscp->user4(); };
            senTreep->foreach([](ಧ್ರುವ AstVarRef* refp) {  //
                refp->varScopep()->user4(ಸತ್ಯ);
            });
        }

        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* ಧ್ರುವ lvtxp
                = ನೂತನ SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            ಧ್ರುವ VNUser2InUse user2InUse;
            ಧ್ರುವ VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
                SchedReplicateVarVertex* ಧ್ರುವ vvtxp = getVarVertex(vscp);
                ಯದಿ (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.ಎಣಿಕೆ(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                ಯದಿ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    ಪ್ರತಿ (ಧ್ರುವ auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    ಪ್ರತಿ (ಧ್ರುವ auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    ಪ್ರತ್ಯಾಗಮ graphp;
}

ಶೂನ್ಯ propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    ಯದಿ (vtxp->user()) ಪ್ರತ್ಯಾಗಮ;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    ಪ್ರತಿ (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* ಧ್ರುವ srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(ಸತ್ಯ);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    ಪ್ರತಿ (V3GraphVertex& vtx : graphp->vertices()) {
        ಯದಿ (SchedReplicateLogicVertex* ಧ್ರುವ lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            ಧ್ರುವ auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            };
            ಧ್ರುವ uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            ಯದಿ (targetRegions & ಒಳಹರಿವು) replicateTo(result.m_ico);
            ಯದಿ (targetRegions & ACTIVE) replicateTo(result.m_act);
            ಯದಿ (targetRegions & NBA) replicateTo(result.m_nba);
            ಯದಿ (targetRegions & OBSERVED) replicateTo(result.m_obs);
            ಯದಿ (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ಧ್ರುವ std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    ಪ್ರತಿ (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    ಪ್ರತ್ಯಾಗಮ replicate(graphp.get());
}

}  // namespace V3Sched
