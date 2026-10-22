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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

नामाकाश {

// Driving region flags
गणना RegionFlags : uint8_t {
    NONE = 0x0,  //
    आदान = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

वर्ग SchedReplicateVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

सार्वजनिक:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() स्थिरांक { परत m_drivingRegions; }
    शून्य addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() स्थिरांक override {
        निवड (static_cast<अचिन्हित>(m_drivingRegions)) {
        स्थिती NONE: परत "black";
        स्थिती आदान: परत "red";
        स्थिती ACTIVE: परत "green";
        स्थिती NBA: परत "blue";
        स्थिती आदान | ACTIVE: परत "yellow";
        स्थिती आदान | NBA: परत "magenta";
        स्थिती ACTIVE | NBA: परत "cyan";
        स्थिती आदान | ACTIVE | NBA: परत "gray80";  // don't want white on white background

        स्थिती REACTIVE: परत "gray60";
        स्थिती REACTIVE | आदान: परत "lightcoral";
        स्थिती REACTIVE | ACTIVE: परत "lightgreen";
        स्थिती REACTIVE | NBA: परत "lightblue";
        स्थिती REACTIVE | आदान | ACTIVE: परत "lightyellow";
        स्थिती REACTIVE | आदान | NBA: परत "lightpink";
        स्थिती REACTIVE | ACTIVE | NBA: परत "lightcyan";
        स्थिती REACTIVE | आदान | ACTIVE | NBA: परत "gray90";

        स्थिती OBSERVED: परत "gray20";
        स्थिती OBSERVED | आदान: परत "darkred";
        स्थिती OBSERVED | ACTIVE: परत "darkgreen";
        स्थिती OBSERVED | NBA: परत "darkblue";
        स्थिती OBSERVED | आदान | ACTIVE: परत "gold";
        स्थिती OBSERVED | आदान | NBA: परत "purple";
        स्थिती OBSERVED | ACTIVE | NBA: परत "darkcyan";
        स्थिती OBSERVED | आदान | ACTIVE | NBA: परत "gray30";

        स्थिती REACTIVE | OBSERVED: परत "gray40";
        स्थिती REACTIVE | OBSERVED | आदान: परत "indianred";
        स्थिती REACTIVE | OBSERVED | ACTIVE: परत "olive";
        स्थिती REACTIVE | OBSERVED | NBA: परत "dodgerBlue";
        स्थिती REACTIVE | OBSERVED | आदान | ACTIVE: परत "khaki";
        स्थिती REACTIVE | OBSERVED | आदान | NBA: परत "plum";
        स्थिती REACTIVE | OBSERVED | ACTIVE | NBA: परत "lightSeaGreen";
        स्थिती REACTIVE | OBSERVED | आदान | ACTIVE | NBA: परत "gray50";
        पूर्वनिर्धारित: v3fatalSrc("There are only 5 region bits"); परत "";
        }
    }
    // LCOV_EXCL_STOP
};

वर्ग SchedReplicateLogicVertex final : सार्वजनिक SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* स्थिरांक m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_वृक्ष* स्थिरांक m_senTreep;  // The sensitivity of the logic node
    Ast_गाठ* स्थिरांक m_logicp;  // The logic node this vertex represents
    RegionFlags स्थिरांक m_assignedRegion;  // The region this logic is originally assigned to

सार्वजनिक:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_वृक्ष* senTreep,
                              Ast_गाठ* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() स्थिरांक { परत m_scopep; }
    Ast_Sen_वृक्ष* senTreep() स्थिरांक { परत m_senTreep; }
    Ast_गाठ* logicp() स्थिरांक { परत m_logicp; }
    RegionFlags assignedRegion() स्थिरांक { परत m_assignedRegion; }

    // For graph dumping
    string name() स्थिरांक override VL_MT_STABLE { परत m_logicp->fileline()->ascii(); };
    string dotShape() स्थिरांक override { परत "rectangle"; }
};

वर्ग SchedReplicateVarVertex final : सार्वजनिक SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* स्थिरांक m_vscp;  // The AstVarScope this vertex represents

सार्वजनिक:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        जर (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(आदान);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        जर (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() स्थिरांक { परत m_vscp; }
    AstVar* varp() स्थिरांक { परत m_vscp->varp(); }
    AstScope* scopep() स्थिरांक { परत m_vscp->scopep(); }

    // For graph dumping
    string name() स्थिरांक override VL_MT_STABLE { परत m_vscp->name(); }
    string dotShape() स्थिरांक override { परत varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

वर्ग Graph final : सार्वजनिक V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(स्थिरांक LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{नवीन Graph};

    // AstVarScope::user1() -> VarVertx
    स्थिरांक VNUser1InUse user1InUse;
    स्थिरांक स्वतः getVarVertex = [&](AstVarScope* vscp) {
        जर (!vscp->user1p()) vscp->user1p(नवीन SchedReplicateVarVertex{graphp.get(), vscp});
        परत vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    स्थिरांक स्वतः addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* शीर्ष) {
        नवीन V3GraphEdge{graphp.get(), fromp, शीर्ष, 1};
    };

    स्थिरांक स्वतः addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_वृक्ष* स्थिरांक senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<तार्किक(AstVarScope*)> readTriggersThisLogic;

        स्थिरांक VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        जर (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { परत असत्य; };
        } अन्यथा जर (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { परत सत्य; };
        } अन्यथा {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { परत !vscp->user4(); };
            senTreep->foreach([](स्थिरांक AstVarRef* refp) {  //
                refp->varScopep()->user4(सत्य);
            });
        }

        साठी (Ast_गाठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* स्थिरांक lvtxp
                = नवीन SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            स्थिरांक VNUser2InUse user2InUse;
            स्थिरांक VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* स्थिरांक vscp = refp->varScopep();
                SchedReplicateVarVertex* स्थिरांक vvtxp = getVarVertex(vscp);
                जर (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.मोजणी(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                जर (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    साठी (स्थिरांक स्वतः& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    साठी (स्थिरांक स्वतः& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    साठी (स्थिरांक स्वतः& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    साठी (स्थिरांक स्वतः& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    साठी (स्थिरांक स्वतः& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    परत graphp;
}

शून्य propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    जर (vtxp->user()) परत;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    साठी (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* स्थिरांक srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(सत्य);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    साठी (V3GraphVertex& vtx : graphp->vertices()) {
        जर (SchedReplicateLogicVertex* स्थिरांक lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            स्थिरांक स्वतः replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_वृक्ष(असत्य));
            };
            स्थिरांक uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            जर (targetRegions & आदान) replicateTo(result.m_ico);
            जर (targetRegions & ACTIVE) replicateTo(result.m_act);
            जर (targetRegions & NBA) replicateTo(result.m_nba);
            जर (targetRegions & OBSERVED) replicateTo(result.m_obs);
            जर (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    परत result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    स्थिरांक std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    साठी (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    परत replicate(graphp.get());
}

}  // namespace V3Sched
