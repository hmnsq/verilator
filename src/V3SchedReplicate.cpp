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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Graph.h"
#include "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

// Driving region flags
enum RegionFlags : uint8_t {
    NONE = 0x0,  //
    INPUT = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

class SchedReplicateVertex VL_NOT_FINAL : public V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

public:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() const { 返回 m_drivingRegions; }
    void addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() const override {
        switch (static_cast<unsigned>(m_drivingRegions)) {
        case NONE: 返回 "black";
        case INPUT: 返回 "red";
        case ACTIVE: 返回 "green";
        case NBA: 返回 "blue";
        case INPUT | ACTIVE: 返回 "yellow";
        case INPUT | NBA: 返回 "magenta";
        case ACTIVE | NBA: 返回 "cyan";
        case INPUT | ACTIVE | NBA: 返回 "gray80";  // don't want white on white background

        case REACTIVE: 返回 "gray60";
        case REACTIVE | INPUT: 返回 "lightcoral";
        case REACTIVE | ACTIVE: 返回 "lightgreen";
        case REACTIVE | NBA: 返回 "lightblue";
        case REACTIVE | INPUT | ACTIVE: 返回 "lightyellow";
        case REACTIVE | INPUT | NBA: 返回 "lightpink";
        case REACTIVE | ACTIVE | NBA: 返回 "lightcyan";
        case REACTIVE | INPUT | ACTIVE | NBA: 返回 "gray90";

        case OBSERVED: 返回 "gray20";
        case OBSERVED | INPUT: 返回 "darkred";
        case OBSERVED | ACTIVE: 返回 "darkgreen";
        case OBSERVED | NBA: 返回 "darkblue";
        case OBSERVED | INPUT | ACTIVE: 返回 "gold";
        case OBSERVED | INPUT | NBA: 返回 "purple";
        case OBSERVED | ACTIVE | NBA: 返回 "darkcyan";
        case OBSERVED | INPUT | ACTIVE | NBA: 返回 "gray30";

        case REACTIVE | OBSERVED: 返回 "gray40";
        case REACTIVE | OBSERVED | INPUT: 返回 "indianred";
        case REACTIVE | OBSERVED | ACTIVE: 返回 "olive";
        case REACTIVE | OBSERVED | NBA: 返回 "dodgerBlue";
        case REACTIVE | OBSERVED | INPUT | ACTIVE: 返回 "khaki";
        case REACTIVE | OBSERVED | INPUT | NBA: 返回 "plum";
        case REACTIVE | OBSERVED | ACTIVE | NBA: 返回 "lightSeaGreen";
        case REACTIVE | OBSERVED | INPUT | ACTIVE | NBA: 返回 "gray50";
        default: v3fatalSrc("There are only 5 region bits"); 返回 "";
        }
    }
    // LCOV_EXCL_STOP
};

class SchedReplicateLogicVertex final : public SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* const m_scopep;  // The enclosing AstScope of the logic node
    AstSenTree* const m_senTreep;  // The sensitivity of the logic node
    AstNode* const m_logicp;  // The logic node this vertex represents
    RegionFlags const m_assignedRegion;  // The region this logic is originally assigned to

public:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, AstSenTree* senTreep,
                              AstNode* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() const { 返回 m_scopep; }
    AstSenTree* senTreep() const { 返回 m_senTreep; }
    AstNode* logicp() const { 返回 m_logicp; }
    RegionFlags assignedRegion() const { 返回 m_assignedRegion; }

    // For graph dumping
    string name() const override VL_MT_STABLE { 返回 m_logicp->fileline()->ascii(); };
    string dotShape() const override { 返回 "rectangle"; }
};

class SchedReplicateVarVertex final : public SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* const m_vscp;  // The AstVarScope this vertex represents

public:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        如果 (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(INPUT);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        如果 (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() const { 返回 m_vscp; }
    AstVar* varp() const { 返回 m_vscp->varp(); }
    AstScope* scopep() const { 返回 m_vscp->scopep(); }

    // For graph dumping
    string name() const override VL_MT_STABLE { 返回 m_vscp->name(); }
    string dotShape() const override { 返回 varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

class Graph final : public V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(const LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{new Graph};

    // AstVarScope::user1() -> VarVertx
    const VNUser1InUse user1InUse;
    const auto getVarVertex = [&](AstVarScope* vscp) {
        如果 (!vscp->user1p()) vscp->user1p(new SchedReplicateVarVertex{graphp.get(), vscp});
        返回 vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    const auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* top) {
        new V3GraphEdge{graphp.get(), fromp, top, 1};
    };

    const auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        AstSenTree* const senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<bool(AstVarScope*)> readTriggersThisLogic;

        const VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        如果 (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { 返回 false; };
        } 否则 如果 (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { 返回 true; };
        } 否则 {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { 返回 !vscp->user4(); };
            senTreep->foreach([](const AstVarRef* refp) {  //
                refp->varScopep()->user4(true);
            });
        }

        为 (AstNode* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* const lvtxp
                = new SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            const VNUser2InUse user2InUse;
            const VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* const vscp = refp->varScopep();
                SchedReplicateVarVertex* const vvtxp = getVarVertex(vscp);
                如果 (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.count(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                如果 (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    为 (const auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    为 (const auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    为 (const auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    为 (const auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    为 (const auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    返回 graphp;
}

void propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    如果 (vtxp->user()) 返回;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    为 (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* const srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(true);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    为 (V3GraphVertex& vtx : graphp->vertices()) {
        如果 (SchedReplicateLogicVertex* const lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            const auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->cloneTree(false));
            };
            const uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            如果 (targetRegions & INPUT) replicateTo(result.m_ico);
            如果 (targetRegions & ACTIVE) replicateTo(result.m_act);
            如果 (targetRegions & NBA) replicateTo(result.m_nba);
            如果 (targetRegions & OBSERVED) replicateTo(result.m_obs);
            如果 (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    返回 result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    const std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    如果 (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    为 (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    如果 (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    返回 replicate(graphp.get());
}

}  // namespace V3Sched
