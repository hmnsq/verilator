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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {

நாமாகாஶ {

// Driving region flags
எண்ணிக்கை RegionFlags : uint8_t {
    NONE = 0x0,  //
    உள்ளீடு = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

வகுப்பு SchedReplicateVertex VL_NOT_FINAL : பொது V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

பொது:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() மாறிலி { திரும்பு m_drivingRegions; }
    வெறுமை addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() மாறிலி override {
        தேர்வு (static_cast<unsigned>(m_drivingRegions)) {
        நிலை NONE: திரும்பு "black";
        நிலை உள்ளீடு: திரும்பு "red";
        நிலை ACTIVE: திரும்பு "green";
        நிலை NBA: திரும்பு "blue";
        நிலை உள்ளீடு | ACTIVE: திரும்பு "yellow";
        நிலை உள்ளீடு | NBA: திரும்பு "magenta";
        நிலை ACTIVE | NBA: திரும்பு "cyan";
        நிலை உள்ளீடு | ACTIVE | NBA: திரும்பு "gray80";  // don't want white on white background

        நிலை REACTIVE: திரும்பு "gray60";
        நிலை REACTIVE | உள்ளீடு: திரும்பு "lightcoral";
        நிலை REACTIVE | ACTIVE: திரும்பு "lightgreen";
        நிலை REACTIVE | NBA: திரும்பு "lightblue";
        நிலை REACTIVE | உள்ளீடு | ACTIVE: திரும்பு "lightyellow";
        நிலை REACTIVE | உள்ளீடு | NBA: திரும்பு "lightpink";
        நிலை REACTIVE | ACTIVE | NBA: திரும்பு "lightcyan";
        நிலை REACTIVE | உள்ளீடு | ACTIVE | NBA: திரும்பு "gray90";

        நிலை OBSERVED: திரும்பு "gray20";
        நிலை OBSERVED | உள்ளீடு: திரும்பு "darkred";
        நிலை OBSERVED | ACTIVE: திரும்பு "darkgreen";
        நிலை OBSERVED | NBA: திரும்பு "darkblue";
        நிலை OBSERVED | உள்ளீடு | ACTIVE: திரும்பு "gold";
        நிலை OBSERVED | உள்ளீடு | NBA: திரும்பு "purple";
        நிலை OBSERVED | ACTIVE | NBA: திரும்பு "darkcyan";
        நிலை OBSERVED | உள்ளீடு | ACTIVE | NBA: திரும்பு "gray30";

        நிலை REACTIVE | OBSERVED: திரும்பு "gray40";
        நிலை REACTIVE | OBSERVED | உள்ளீடு: திரும்பு "indianred";
        நிலை REACTIVE | OBSERVED | ACTIVE: திரும்பு "olive";
        நிலை REACTIVE | OBSERVED | NBA: திரும்பு "dodgerBlue";
        நிலை REACTIVE | OBSERVED | உள்ளீடு | ACTIVE: திரும்பு "khaki";
        நிலை REACTIVE | OBSERVED | உள்ளீடு | NBA: திரும்பு "plum";
        நிலை REACTIVE | OBSERVED | ACTIVE | NBA: திரும்பு "lightSeaGreen";
        நிலை REACTIVE | OBSERVED | உள்ளீடு | ACTIVE | NBA: திரும்பு "gray50";
        இயல்பு: v3fatalSrc("There are only 5 region bits"); திரும்பு "";
        }
    }
    // LCOV_EXCL_STOP
};

வகுப்பு SchedReplicateLogicVertex final : பொது SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* மாறிலி m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_மரம்* மாறிலி m_senTreep;  // The sensitivity of the logic node
    Ast_முனை* மாறிலி m_logicp;  // The logic node this vertex represents
    RegionFlags மாறிலி m_assignedRegion;  // The region this logic is originally assigned to

பொது:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_மரம்* senTreep,
                              Ast_முனை* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() மாறிலி { திரும்பு m_scopep; }
    Ast_Sen_மரம்* senTreep() மாறிலி { திரும்பு m_senTreep; }
    Ast_முனை* logicp() மாறிலி { திரும்பு m_logicp; }
    RegionFlags assignedRegion() மாறிலி { திரும்பு m_assignedRegion; }

    // For graph dumping
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_logicp->fileline()->ascii(); };
    string dotShape() மாறிலி override { திரும்பு "rectangle"; }
};

வகுப்பு SchedReplicateVarVertex final : பொது SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* மாறிலி m_vscp;  // The AstVarScope this vertex represents

பொது:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        எனில் (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(உள்ளீடு);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        எனில் (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() மாறிலி { திரும்பு m_vscp; }
    AstVar* varp() மாறிலி { திரும்பு m_vscp->varp(); }
    AstScope* scopep() மாறிலி { திரும்பு m_vscp->scopep(); }

    // For graph dumping
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_vscp->name(); }
    string dotShape() மாறிலி override { திரும்பு varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

வகுப்பு Graph final : பொது V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(மாறிலி LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{புதிய Graph};

    // AstVarScope::user1() -> VarVertx
    மாறிலி VNUser1InUse user1InUse;
    மாறிலி auto getVarVertex = [&](AstVarScope* vscp) {
        எனில் (!vscp->user1p()) vscp->user1p(புதிய SchedReplicateVarVertex{graphp.get(), vscp});
        திரும்பு vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    மாறிலி auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* உச்சி) {
        புதிய V3GraphEdge{graphp.get(), fromp, உச்சி, 1};
    };

    மாறிலி auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_மரம்* மாறிலி senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<தருக்க(AstVarScope*)> readTriggersThisLogic;

        மாறிலி VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        எனில் (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { திரும்பு பொய்; };
        } இல்லையெனில் எனில் (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { திரும்பு உண்மை; };
        } இல்லையெனில் {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { திரும்பு !vscp->user4(); };
            senTreep->foreach([](மாறிலி AstVarRef* refp) {  //
                refp->varScopep()->user4(உண்மை);
            });
        }

        க்கு (Ast_முனை* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* மாறிலி lvtxp
                = புதிய SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            மாறிலி VNUser2InUse user2InUse;
            மாறிலி VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* மாறிலி vscp = refp->varScopep();
                SchedReplicateVarVertex* மாறிலி vvtxp = getVarVertex(vscp);
                எனில் (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.மொத்தம்(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                எனில் (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    க்கு (மாறிலி auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    க்கு (மாறிலி auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    க்கு (மாறிலி auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    க்கு (மாறிலி auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    க்கு (மாறிலி auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    திரும்பு graphp;
}

வெறுமை propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    எனில் (vtxp->user()) திரும்பு;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    க்கு (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* மாறிலி srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(உண்மை);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    க்கு (V3GraphVertex& vtx : graphp->vertices()) {
        எனில் (SchedReplicateLogicVertex* மாறிலி lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            மாறிலி auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_மரம்(பொய்));
            };
            மாறிலி uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            எனில் (targetRegions & உள்ளீடு) replicateTo(result.m_ico);
            எனில் (targetRegions & ACTIVE) replicateTo(result.m_act);
            எனில் (targetRegions & NBA) replicateTo(result.m_nba);
            எனில் (targetRegions & OBSERVED) replicateTo(result.m_obs);
            எனில் (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    திரும்பு result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    மாறிலி std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    க்கு (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    திரும்பு replicate(graphp.get());
}

}  // namespace V3Sched
