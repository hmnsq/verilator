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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

নামাকাশ {

// Driving region flags
গণনা RegionFlags : uint8_t {
    NONE = 0x0,  //
    প্ৰবেশ = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

শ্ৰেণী SchedReplicateVertex VL_NOT_FINAL : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

সার্ৱজনিক:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() ধ্রুৱ { ঘূৰাই m_drivingRegions; }
    শূন্য addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ধ্রুৱ override {
        ৱিকল্প (static_cast<unsigned>(m_drivingRegions)) {
        পক্ষ NONE: ঘূৰাই "black";
        পক্ষ প্ৰবেশ: ঘূৰাই "red";
        পক্ষ ACTIVE: ঘূৰাই "green";
        পক্ষ NBA: ঘূৰাই "blue";
        পক্ষ প্ৰবেশ | ACTIVE: ঘূৰাই "yellow";
        পক্ষ প্ৰবেশ | NBA: ঘূৰাই "magenta";
        পক্ষ ACTIVE | NBA: ঘূৰাই "cyan";
        পক্ষ প্ৰবেশ | ACTIVE | NBA: ঘূৰাই "gray80";  // don't want white on white background

        পক্ষ REACTIVE: ঘূৰাই "gray60";
        পক্ষ REACTIVE | প্ৰবেশ: ঘূৰাই "lightcoral";
        পক্ষ REACTIVE | ACTIVE: ঘূৰাই "lightgreen";
        পক্ষ REACTIVE | NBA: ঘূৰাই "lightblue";
        পক্ষ REACTIVE | প্ৰবেশ | ACTIVE: ঘূৰাই "lightyellow";
        পক্ষ REACTIVE | প্ৰবেশ | NBA: ঘূৰাই "lightpink";
        পক্ষ REACTIVE | ACTIVE | NBA: ঘূৰাই "lightcyan";
        পক্ষ REACTIVE | প্ৰবেশ | ACTIVE | NBA: ঘূৰাই "gray90";

        পক্ষ OBSERVED: ঘূৰাই "gray20";
        পক্ষ OBSERVED | প্ৰবেশ: ঘূৰাই "darkred";
        পক্ষ OBSERVED | ACTIVE: ঘূৰাই "darkgreen";
        পক্ষ OBSERVED | NBA: ঘূৰাই "darkblue";
        পক্ষ OBSERVED | প্ৰবেশ | ACTIVE: ঘূৰাই "gold";
        পক্ষ OBSERVED | প্ৰবেশ | NBA: ঘূৰাই "purple";
        পক্ষ OBSERVED | ACTIVE | NBA: ঘূৰাই "darkcyan";
        পক্ষ OBSERVED | প্ৰবেশ | ACTIVE | NBA: ঘূৰাই "gray30";

        পক্ষ REACTIVE | OBSERVED: ঘূৰাই "gray40";
        পক্ষ REACTIVE | OBSERVED | প্ৰবেশ: ঘূৰাই "indianred";
        পক্ষ REACTIVE | OBSERVED | ACTIVE: ঘূৰাই "olive";
        পক্ষ REACTIVE | OBSERVED | NBA: ঘূৰাই "dodgerBlue";
        পক্ষ REACTIVE | OBSERVED | প্ৰবেশ | ACTIVE: ঘূৰাই "khaki";
        পক্ষ REACTIVE | OBSERVED | প্ৰবেশ | NBA: ঘূৰাই "plum";
        পক্ষ REACTIVE | OBSERVED | ACTIVE | NBA: ঘূৰাই "lightSeaGreen";
        পক্ষ REACTIVE | OBSERVED | প্ৰবেশ | ACTIVE | NBA: ঘূৰাই "gray50";
        স্ৱভাৱ: v3fatalSrc("There are only 5 region bits"); ঘূৰাই "";
        }
    }
    // LCOV_EXCL_STOP
};

শ্ৰেণী SchedReplicateLogicVertex final : সার্ৱজনিক SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* ধ্রুৱ m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_বৃক্ষ* ধ্রুৱ m_senTreep;  // The sensitivity of the logic node
    Ast_গ্ৰন্থি* ধ্রুৱ m_logicp;  // The logic node this vertex represents
    RegionFlags ধ্রুৱ m_assignedRegion;  // The region this logic is originally assigned to

সার্ৱজনিক:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_বৃক্ষ* senTreep,
                              Ast_গ্ৰন্থি* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() ধ্রুৱ { ঘূৰাই m_scopep; }
    Ast_Sen_বৃক্ষ* senTreep() ধ্রুৱ { ঘূৰাই m_senTreep; }
    Ast_গ্ৰন্থি* logicp() ধ্রুৱ { ঘূৰাই m_logicp; }
    RegionFlags assignedRegion() ধ্রুৱ { ঘূৰাই m_assignedRegion; }

    // For graph dumping
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_logicp->fileline()->ascii(); };
    string dotShape() ধ্রুৱ override { ঘূৰাই "rectangle"; }
};

শ্ৰেণী SchedReplicateVarVertex final : সার্ৱজনিক SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* ধ্রুৱ m_vscp;  // The AstVarScope this vertex represents

সার্ৱজনিক:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        যদি (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(প্ৰবেশ);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        যদি (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() ধ্রুৱ { ঘূৰাই m_vscp; }
    AstVar* varp() ধ্রুৱ { ঘূৰাই m_vscp->varp(); }
    AstScope* scopep() ধ্রুৱ { ঘূৰাই m_vscp->scopep(); }

    // For graph dumping
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_vscp->name(); }
    string dotShape() ধ্রুৱ override { ঘূৰাই varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

শ্ৰেণী Graph final : সার্ৱজনিক V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ধ্রুৱ LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{নূতন Graph};

    // AstVarScope::user1() -> VarVertx
    ধ্রুৱ VNUser1InUse user1InUse;
    ধ্রুৱ auto getVarVertex = [&](AstVarScope* vscp) {
        যদি (!vscp->user1p()) vscp->user1p(নূতন SchedReplicateVarVertex{graphp.get(), vscp});
        ঘূৰাই vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    ধ্রুৱ auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* শীৰ্ষ) {
        নূতন V3GraphEdge{graphp.get(), fromp, শীৰ্ষ, 1};
    };

    ধ্রুৱ auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_বৃক্ষ* ধ্রুৱ senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<তার্কিক(AstVarScope*)> readTriggersThisLogic;

        ধ্রুৱ VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        যদি (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ঘূৰাই মিছা; };
        } অন্যথা যদি (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ঘূৰাই সঁচা; };
        } অন্যথা {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { ঘূৰাই !vscp->user4(); };
            senTreep->foreach([](ধ্রুৱ AstVarRef* refp) {  //
                refp->varScopep()->user4(সঁচা);
            });
        }

        বাবে (Ast_গ্ৰন্থি* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* ধ্রুৱ lvtxp
                = নূতন SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            ধ্রুৱ VNUser2InUse user2InUse;
            ধ্রুৱ VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ধ্রুৱ vscp = refp->varScopep();
                SchedReplicateVarVertex* ধ্রুৱ vvtxp = getVarVertex(vscp);
                যদি (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.গুনতি(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                যদি (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    বাবে (ধ্রুৱ auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    বাবে (ধ্রুৱ auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    বাবে (ধ্রুৱ auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    বাবে (ধ্রুৱ auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    বাবে (ধ্রুৱ auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    ঘূৰাই graphp;
}

শূন্য propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    যদি (vtxp->user()) ঘূৰাই;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    বাবে (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* ধ্রুৱ srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(সঁচা);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    বাবে (V3GraphVertex& vtx : graphp->vertices()) {
        যদি (SchedReplicateLogicVertex* ধ্রুৱ lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            ধ্রুৱ auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_বৃক্ষ(মিছা));
            };
            ধ্রুৱ uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            যদি (targetRegions & প্ৰবেশ) replicateTo(result.m_ico);
            যদি (targetRegions & ACTIVE) replicateTo(result.m_act);
            যদি (targetRegions & NBA) replicateTo(result.m_nba);
            যদি (targetRegions & OBSERVED) replicateTo(result.m_obs);
            যদি (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    ঘূৰাই result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ধ্রুৱ std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    বাবে (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    ঘূৰাই replicate(graphp.get());
}

}  // namespace V3Sched
