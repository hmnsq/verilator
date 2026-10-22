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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

নামাকাশ {

// Driving region flags
গণনা RegionFlags : uint8_t {
    NONE = 0x0,  //
    প্রবেশ = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

বর্গ SchedReplicateVertex VL_NOT_FINAL : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

সার্বজনিক:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() ধ্রুব { প্রত্যাগম m_drivingRegions; }
    শূন্য addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ধ্রুব override {
        বিকল্প (static_cast<unsigned>(m_drivingRegions)) {
        পক্ষ NONE: প্রত্যাগম "black";
        পক্ষ প্রবেশ: প্রত্যাগম "red";
        পক্ষ ACTIVE: প্রত্যাগম "green";
        পক্ষ NBA: প্রত্যাগম "blue";
        পক্ষ প্রবেশ | ACTIVE: প্রত্যাগম "yellow";
        পক্ষ প্রবেশ | NBA: প্রত্যাগম "magenta";
        পক্ষ ACTIVE | NBA: প্রত্যাগম "cyan";
        পক্ষ প্রবেশ | ACTIVE | NBA: প্রত্যাগম "gray80";  // don't want white on white background

        পক্ষ REACTIVE: প্রত্যাগম "gray60";
        পক্ষ REACTIVE | প্রবেশ: প্রত্যাগম "lightcoral";
        পক্ষ REACTIVE | ACTIVE: প্রত্যাগম "lightgreen";
        পক্ষ REACTIVE | NBA: প্রত্যাগম "lightblue";
        পক্ষ REACTIVE | প্রবেশ | ACTIVE: প্রত্যাগম "lightyellow";
        পক্ষ REACTIVE | প্রবেশ | NBA: প্রত্যাগম "lightpink";
        পক্ষ REACTIVE | ACTIVE | NBA: প্রত্যাগম "lightcyan";
        পক্ষ REACTIVE | প্রবেশ | ACTIVE | NBA: প্রত্যাগম "gray90";

        পক্ষ OBSERVED: প্রত্যাগম "gray20";
        পক্ষ OBSERVED | প্রবেশ: প্রত্যাগম "darkred";
        পক্ষ OBSERVED | ACTIVE: প্রত্যাগম "darkgreen";
        পক্ষ OBSERVED | NBA: প্রত্যাগম "darkblue";
        পক্ষ OBSERVED | প্রবেশ | ACTIVE: প্রত্যাগম "gold";
        পক্ষ OBSERVED | প্রবেশ | NBA: প্রত্যাগম "purple";
        পক্ষ OBSERVED | ACTIVE | NBA: প্রত্যাগম "darkcyan";
        পক্ষ OBSERVED | প্রবেশ | ACTIVE | NBA: প্রত্যাগম "gray30";

        পক্ষ REACTIVE | OBSERVED: প্রত্যাগম "gray40";
        পক্ষ REACTIVE | OBSERVED | প্রবেশ: প্রত্যাগম "indianred";
        পক্ষ REACTIVE | OBSERVED | ACTIVE: প্রত্যাগম "olive";
        পক্ষ REACTIVE | OBSERVED | NBA: প্রত্যাগম "dodgerBlue";
        পক্ষ REACTIVE | OBSERVED | প্রবেশ | ACTIVE: প্রত্যাগম "khaki";
        পক্ষ REACTIVE | OBSERVED | প্রবেশ | NBA: প্রত্যাগম "plum";
        পক্ষ REACTIVE | OBSERVED | ACTIVE | NBA: প্রত্যাগম "lightSeaGreen";
        পক্ষ REACTIVE | OBSERVED | প্রবেশ | ACTIVE | NBA: প্রত্যাগম "gray50";
        স্বভাব: v3fatalSrc("There are only 5 region bits"); প্রত্যাগম "";
        }
    }
    // LCOV_EXCL_STOP
};

বর্গ SchedReplicateLogicVertex final : সার্বজনিক SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* ধ্রুব m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_বৃক্ষ* ধ্রুব m_senTreep;  // The sensitivity of the logic node
    Ast_গ্রন্থি* ধ্রুব m_logicp;  // The logic node this vertex represents
    RegionFlags ধ্রুব m_assignedRegion;  // The region this logic is originally assigned to

সার্বজনিক:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_বৃক্ষ* senTreep,
                              Ast_গ্রন্থি* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() ধ্রুব { প্রত্যাগম m_scopep; }
    Ast_Sen_বৃক্ষ* senTreep() ধ্রুব { প্রত্যাগম m_senTreep; }
    Ast_গ্রন্থি* logicp() ধ্রুব { প্রত্যাগম m_logicp; }
    RegionFlags assignedRegion() ধ্রুব { প্রত্যাগম m_assignedRegion; }

    // For graph dumping
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_logicp->fileline()->ascii(); };
    string dotShape() ধ্রুব override { প্রত্যাগম "rectangle"; }
};

বর্গ SchedReplicateVarVertex final : সার্বজনিক SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* ধ্রুব m_vscp;  // The AstVarScope this vertex represents

সার্বজনিক:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        যদি (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(প্রবেশ);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        যদি (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() ধ্রুব { প্রত্যাগম m_vscp; }
    AstVar* varp() ধ্রুব { প্রত্যাগম m_vscp->varp(); }
    AstScope* scopep() ধ্রুব { প্রত্যাগম m_vscp->scopep(); }

    // For graph dumping
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_vscp->name(); }
    string dotShape() ধ্রুব override { প্রত্যাগম varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

বর্গ Graph final : সার্বজনিক V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ধ্রুব LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{নূতন Graph};

    // AstVarScope::user1() -> VarVertx
    ধ্রুব VNUser1InUse user1InUse;
    ধ্রুব auto getVarVertex = [&](AstVarScope* vscp) {
        যদি (!vscp->user1p()) vscp->user1p(নূতন SchedReplicateVarVertex{graphp.get(), vscp});
        প্রত্যাগম vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    ধ্রুব auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* শীর্ষ) {
        নূতন V3GraphEdge{graphp.get(), fromp, শীর্ষ, 1};
    };

    ধ্রুব auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_বৃক্ষ* ধ্রুব senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<তার্কিক(AstVarScope*)> readTriggersThisLogic;

        ধ্রুব VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        যদি (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { প্রত্যাগম অসত্য; };
        } অন্যথা যদি (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { প্রত্যাগম সত্য; };
        } অন্যথা {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { প্রত্যাগম !vscp->user4(); };
            senTreep->foreach([](ধ্রুব AstVarRef* refp) {  //
                refp->varScopep()->user4(সত্য);
            });
        }

        প্রতি (Ast_গ্রন্থি* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* ধ্রুব lvtxp
                = নূতন SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            ধ্রুব VNUser2InUse user2InUse;
            ধ্রুব VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ধ্রুব vscp = refp->varScopep();
                SchedReplicateVarVertex* ধ্রুব vvtxp = getVarVertex(vscp);
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

    প্রতি (ধ্রুব auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    প্রতি (ধ্রুব auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    প্রতি (ধ্রুব auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    প্রতি (ধ্রুব auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    প্রতি (ধ্রুব auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    প্রত্যাগম graphp;
}

শূন্য propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    যদি (vtxp->user()) প্রত্যাগম;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    প্রতি (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* ধ্রুব srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(সত্য);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    প্রতি (V3GraphVertex& vtx : graphp->vertices()) {
        যদি (SchedReplicateLogicVertex* ধ্রুব lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            ধ্রুব auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_বৃক্ষ(অসত্য));
            };
            ধ্রুব uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            যদি (targetRegions & প্রবেশ) replicateTo(result.m_ico);
            যদি (targetRegions & ACTIVE) replicateTo(result.m_act);
            যদি (targetRegions & NBA) replicateTo(result.m_nba);
            যদি (targetRegions & OBSERVED) replicateTo(result.m_obs);
            যদি (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    প্রত্যাগম result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ধ্রুব std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    প্রতি (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    প্রত্যাগম replicate(graphp.get());
}

}  // namespace V3Sched
