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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Graph.h"
#ضمن "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {

نطاق {

// Driving region flags
تعداد RegionFlags : uint8_t {
    NONE = 0x0,  //
    INPUT = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

صنف SchedReplicateVertex VL_NOT_FINAL : عام V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

عام:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() ثابت { ܦܢܐ m_drivingRegions; }
    فراغ addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ثابت override {
        بدل (static_cast<unsigned>(m_drivingRegions)) {
        حالة NONE: ܦܢܐ "black";
        حالة INPUT: ܦܢܐ "red";
        حالة ACTIVE: ܦܢܐ "green";
        حالة NBA: ܦܢܐ "blue";
        حالة INPUT | ACTIVE: ܦܢܐ "yellow";
        حالة INPUT | NBA: ܦܢܐ "magenta";
        حالة ACTIVE | NBA: ܦܢܐ "cyan";
        حالة INPUT | ACTIVE | NBA: ܦܢܐ "gray80";  // don't want white on white background

        حالة REACTIVE: ܦܢܐ "gray60";
        حالة REACTIVE | INPUT: ܦܢܐ "lightcoral";
        حالة REACTIVE | ACTIVE: ܦܢܐ "lightgreen";
        حالة REACTIVE | NBA: ܦܢܐ "lightblue";
        حالة REACTIVE | INPUT | ACTIVE: ܦܢܐ "lightyellow";
        حالة REACTIVE | INPUT | NBA: ܦܢܐ "lightpink";
        حالة REACTIVE | ACTIVE | NBA: ܦܢܐ "lightcyan";
        حالة REACTIVE | INPUT | ACTIVE | NBA: ܦܢܐ "gray90";

        حالة OBSERVED: ܦܢܐ "gray20";
        حالة OBSERVED | INPUT: ܦܢܐ "darkred";
        حالة OBSERVED | ACTIVE: ܦܢܐ "darkgreen";
        حالة OBSERVED | NBA: ܦܢܐ "darkblue";
        حالة OBSERVED | INPUT | ACTIVE: ܦܢܐ "gold";
        حالة OBSERVED | INPUT | NBA: ܦܢܐ "purple";
        حالة OBSERVED | ACTIVE | NBA: ܦܢܐ "darkcyan";
        حالة OBSERVED | INPUT | ACTIVE | NBA: ܦܢܐ "gray30";

        حالة REACTIVE | OBSERVED: ܦܢܐ "gray40";
        حالة REACTIVE | OBSERVED | INPUT: ܦܢܐ "indianred";
        حالة REACTIVE | OBSERVED | ACTIVE: ܦܢܐ "olive";
        حالة REACTIVE | OBSERVED | NBA: ܦܢܐ "dodgerBlue";
        حالة REACTIVE | OBSERVED | INPUT | ACTIVE: ܦܢܐ "khaki";
        حالة REACTIVE | OBSERVED | INPUT | NBA: ܦܢܐ "plum";
        حالة REACTIVE | OBSERVED | ACTIVE | NBA: ܦܢܐ "lightSeaGreen";
        حالة REACTIVE | OBSERVED | INPUT | ACTIVE | NBA: ܦܢܐ "gray50";
        افتراضي: v3fatalSrc("There are only 5 region bits"); ܦܢܐ "";
        }
    }
    // LCOV_EXCL_STOP
};

صنف SchedReplicateLogicVertex final : عام SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node
    AstSenTree* ثابت m_senTreep;  // The sensitivity of the logic node
    AstNode* ثابت m_logicp;  // The logic node this vertex represents
    RegionFlags ثابت m_assignedRegion;  // The region this logic is originally assigned to

عام:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, AstSenTree* senTreep,
                              AstNode* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() ثابت { ܦܢܐ m_scopep; }
    AstSenTree* senTreep() ثابت { ܦܢܐ m_senTreep; }
    AstNode* logicp() ثابت { ܦܢܐ m_logicp; }
    RegionFlags assignedRegion() ثابت { ܦܢܐ m_assignedRegion; }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { ܦܢܐ m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { ܦܢܐ "rectangle"; }
};

صنف SchedReplicateVarVertex final : عام SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عام:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        ܐܢ (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(INPUT);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        ܐܢ (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() ثابت { ܦܢܐ m_vscp; }
    AstVar* varp() ثابت { ܦܢܐ m_vscp->varp(); }
    AstScope* scopep() ثابت { ܦܢܐ m_vscp->scopep(); }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { ܦܢܐ m_vscp->name(); }
    string dotShape() ثابت override { ܦܢܐ varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

صنف Graph final : عام V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ثابت LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{جديد Graph};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1InUse user1InUse;
    ثابت auto getVarVertex = [&](AstVarScope* vscp) {
        ܐܢ (!vscp->user1p()) vscp->user1p(جديد SchedReplicateVarVertex{graphp.get(), vscp});
        ܦܢܐ vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    ثابت auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* top) {
        جديد V3GraphEdge{graphp.get(), fromp, top, 1};
    };

    ثابت auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        AstSenTree* ثابت senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<منطقي(AstVarScope*)> readTriggersThisLogic;

        ثابت VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        ܐܢ (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ܦܢܐ ܕܓܠܐ; };
        } ܐܚܪܢܐ ܐܢ (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ܦܢܐ ܫܪܪܐ; };
        } ܐܚܪܢܐ {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { ܦܢܐ !vscp->user4(); };
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user4(ܫܪܪܐ);
            });
        }

        لكل (AstNode* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* ثابت lvtxp
                = جديد SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            ثابت VNUser2InUse user2InUse;
            ثابت VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                SchedReplicateVarVertex* ثابت vvtxp = getVarVertex(vscp);
                ܐܢ (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.count(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                ܐܢ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    لكل (ثابت auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    لكل (ثابت auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    لكل (ثابت auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    لكل (ثابت auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    لكل (ثابت auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    ܦܢܐ graphp;
}

فراغ propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    ܐܢ (vtxp->user()) ܦܢܐ;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    لكل (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* ثابت srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(ܫܪܪܐ);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        ܐܢ (SchedReplicateLogicVertex* ثابت lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            ثابت auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->cloneTree(ܕܓܠܐ));
            };
            ثابت uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            ܐܢ (targetRegions & INPUT) replicateTo(result.m_ico);
            ܐܢ (targetRegions & ACTIVE) replicateTo(result.m_act);
            ܐܢ (targetRegions & NBA) replicateTo(result.m_nba);
            ܐܢ (targetRegions & OBSERVED) replicateTo(result.m_obs);
            ܐܢ (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    ܦܢܐ result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    ܐܢ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    ܐܢ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    ܦܢܐ replicate(graphp.get());
}

}  // namespace V3Sched
