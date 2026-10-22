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
ga_nanaa RegionFlags : uint8_t {
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
    uint8_t drivingRegions() const { ꦮꦭꦶ m_drivingRegions; }
    void addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() const override {
        switch (static_cast<unsigned>(m_drivingRegions)) {
        pak_sa NONE: ꦮꦭꦶ "black";
        pak_sa INPUT: ꦮꦭꦶ "red";
        pak_sa ACTIVE: ꦮꦭꦶ "green";
        pak_sa NBA: ꦮꦭꦶ "blue";
        pak_sa INPUT | ACTIVE: ꦮꦭꦶ "yellow";
        pak_sa INPUT | NBA: ꦮꦭꦶ "magenta";
        pak_sa ACTIVE | NBA: ꦮꦭꦶ "cyan";
        pak_sa INPUT | ACTIVE | NBA: ꦮꦭꦶ "gray80";  // don't want white on white background

        pak_sa REACTIVE: ꦮꦭꦶ "gray60";
        pak_sa REACTIVE | INPUT: ꦮꦭꦶ "lightcoral";
        pak_sa REACTIVE | ACTIVE: ꦮꦭꦶ "lightgreen";
        pak_sa REACTIVE | NBA: ꦮꦭꦶ "lightblue";
        pak_sa REACTIVE | INPUT | ACTIVE: ꦮꦭꦶ "lightyellow";
        pak_sa REACTIVE | INPUT | NBA: ꦮꦭꦶ "lightpink";
        pak_sa REACTIVE | ACTIVE | NBA: ꦮꦭꦶ "lightcyan";
        pak_sa REACTIVE | INPUT | ACTIVE | NBA: ꦮꦭꦶ "gray90";

        pak_sa OBSERVED: ꦮꦭꦶ "gray20";
        pak_sa OBSERVED | INPUT: ꦮꦭꦶ "darkred";
        pak_sa OBSERVED | ACTIVE: ꦮꦭꦶ "darkgreen";
        pak_sa OBSERVED | NBA: ꦮꦭꦶ "darkblue";
        pak_sa OBSERVED | INPUT | ACTIVE: ꦮꦭꦶ "gold";
        pak_sa OBSERVED | INPUT | NBA: ꦮꦭꦶ "purple";
        pak_sa OBSERVED | ACTIVE | NBA: ꦮꦭꦶ "darkcyan";
        pak_sa OBSERVED | INPUT | ACTIVE | NBA: ꦮꦭꦶ "gray30";

        pak_sa REACTIVE | OBSERVED: ꦮꦭꦶ "gray40";
        pak_sa REACTIVE | OBSERVED | INPUT: ꦮꦭꦶ "indianred";
        pak_sa REACTIVE | OBSERVED | ACTIVE: ꦮꦭꦶ "olive";
        pak_sa REACTIVE | OBSERVED | NBA: ꦮꦭꦶ "dodgerBlue";
        pak_sa REACTIVE | OBSERVED | INPUT | ACTIVE: ꦮꦭꦶ "khaki";
        pak_sa REACTIVE | OBSERVED | INPUT | NBA: ꦮꦭꦶ "plum";
        pak_sa REACTIVE | OBSERVED | ACTIVE | NBA: ꦮꦭꦶ "lightSeaGreen";
        pak_sa REACTIVE | OBSERVED | INPUT | ACTIVE | NBA: ꦮꦭꦶ "gray50";
        default: v3fatalSrc("There are only 5 region bits"); ꦮꦭꦶ "";
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
    AstScope* scopep() const { ꦮꦭꦶ m_scopep; }
    AstSenTree* senTreep() const { ꦮꦭꦶ m_senTreep; }
    AstNode* logicp() const { ꦮꦭꦶ m_logicp; }
    RegionFlags assignedRegion() const { ꦮꦭꦶ m_assignedRegion; }

    // For graph dumping
    string name() const override VL_MT_STABLE { ꦮꦭꦶ m_logicp->fileline()->ascii(); };
    string dotShape() const override { ꦮꦭꦶ "rectangle"; }
};

class SchedReplicateVarVertex final : public SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* const m_vscp;  // The AstVarScope this vertex represents

public:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        ꦪꦼꦤ꧀ (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(INPUT);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        ꦪꦼꦤ꧀ (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() const { ꦮꦭꦶ m_vscp; }
    AstVar* varp() const { ꦮꦭꦶ m_vscp->varp(); }
    AstScope* scopep() const { ꦮꦭꦶ m_vscp->scopep(); }

    // For graph dumping
    string name() const override VL_MT_STABLE { ꦮꦭꦶ m_vscp->name(); }
    string dotShape() const override { ꦮꦭꦶ varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

class Graph final : public V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(const LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{new Graph};

    // AstVarScope::user1() -> VarVertx
    const VNUser1InUse user1InUse;
    const auto getVarVertex = [&](AstVarScope* vscp) {
        ꦪꦼꦤ꧀ (!vscp->user1p()) vscp->user1p(new SchedReplicateVarVertex{graphp.get(), vscp});
        ꦮꦭꦶ vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    const auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* top) {
        new V3GraphEdge{graphp.get(), fromp, top, 1};
    };

    const auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        AstSenTree* const senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<bool(AstVarScope*)> readTriggersThisLogic;

        const VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        ꦪꦼꦤ꧀ (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ꦮꦭꦶ false; };
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ꦮꦭꦶ true; };
        } ꦭꦶꦪ {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { ꦮꦭꦶ !vscp->user4(); };
            senTreep->foreach([](const AstVarRef* refp) {  //
                refp->varScopep()->user4(true);
            });
        }

        ꦏꦁꦒꦺꦴ (AstNode* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* const lvtxp
                = new SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            const VNUser2InUse user2InUse;
            const VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* const vscp = refp->varScopep();
                SchedReplicateVarVertex* const vvtxp = getVarVertex(vscp);
                ꦪꦼꦤ꧀ (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.count(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                ꦪꦼꦤ꧀ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    ꦏꦁꦒꦺꦴ (const auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    ꦏꦁꦒꦺꦴ (const auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    ꦏꦁꦒꦺꦴ (const auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    ꦏꦁꦒꦺꦴ (const auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    ꦏꦁꦒꦺꦴ (const auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    ꦮꦭꦶ graphp;
}

void propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    ꦪꦼꦤ꧀ (vtxp->user()) ꦮꦭꦶ;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    ꦏꦁꦒꦺꦴ (V3GraphEdge& edge : vtxp->inEdges()) {
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
    ꦏꦁꦒꦺꦴ (V3GraphVertex& vtx : graphp->vertices()) {
        ꦪꦼꦤ꧀ (SchedReplicateLogicVertex* const lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            const auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->cloneTree(false));
            };
            const uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            ꦪꦼꦤ꧀ (targetRegions & INPUT) replicateTo(result.m_ico);
            ꦪꦼꦤ꧀ (targetRegions & ACTIVE) replicateTo(result.m_act);
            ꦪꦼꦤ꧀ (targetRegions & NBA) replicateTo(result.m_nba);
            ꦪꦼꦤ꧀ (targetRegions & OBSERVED) replicateTo(result.m_obs);
            ꦪꦼꦤ꧀ (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    ꦮꦭꦶ result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    const std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    ꦪꦼꦤ꧀ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    ꦏꦁꦒꦺꦴ (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    ꦪꦼꦤ꧀ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    ꦮꦭꦶ replicate(graphp.get());
}

}  // namespace V3Sched
