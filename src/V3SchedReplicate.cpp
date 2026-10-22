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
    إدخال = 0x1,  // Variable/logic is driven from top level input
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
    uint8_t drivingRegions() ثابت { إرجاع m_drivingRegions; }
    فراغ addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ثابت override {
        بدل (static_cast<unsigned>(m_drivingRegions)) {
        حالة NONE: إرجاع "black";
        حالة إدخال: إرجاع "red";
        حالة ACTIVE: إرجاع "green";
        حالة NBA: إرجاع "blue";
        حالة إدخال | ACTIVE: إرجاع "yellow";
        حالة إدخال | NBA: إرجاع "magenta";
        حالة ACTIVE | NBA: إرجاع "cyan";
        حالة إدخال | ACTIVE | NBA: إرجاع "gray80";  // don't want white on white background

        حالة REACTIVE: إرجاع "gray60";
        حالة REACTIVE | إدخال: إرجاع "lightcoral";
        حالة REACTIVE | ACTIVE: إرجاع "lightgreen";
        حالة REACTIVE | NBA: إرجاع "lightblue";
        حالة REACTIVE | إدخال | ACTIVE: إرجاع "lightyellow";
        حالة REACTIVE | إدخال | NBA: إرجاع "lightpink";
        حالة REACTIVE | ACTIVE | NBA: إرجاع "lightcyan";
        حالة REACTIVE | إدخال | ACTIVE | NBA: إرجاع "gray90";

        حالة OBSERVED: إرجاع "gray20";
        حالة OBSERVED | إدخال: إرجاع "darkred";
        حالة OBSERVED | ACTIVE: إرجاع "darkgreen";
        حالة OBSERVED | NBA: إرجاع "darkblue";
        حالة OBSERVED | إدخال | ACTIVE: إرجاع "gold";
        حالة OBSERVED | إدخال | NBA: إرجاع "purple";
        حالة OBSERVED | ACTIVE | NBA: إرجاع "darkcyan";
        حالة OBSERVED | إدخال | ACTIVE | NBA: إرجاع "gray30";

        حالة REACTIVE | OBSERVED: إرجاع "gray40";
        حالة REACTIVE | OBSERVED | إدخال: إرجاع "indianred";
        حالة REACTIVE | OBSERVED | ACTIVE: إرجاع "olive";
        حالة REACTIVE | OBSERVED | NBA: إرجاع "dodgerBlue";
        حالة REACTIVE | OBSERVED | إدخال | ACTIVE: إرجاع "khaki";
        حالة REACTIVE | OBSERVED | إدخال | NBA: إرجاع "plum";
        حالة REACTIVE | OBSERVED | ACTIVE | NBA: إرجاع "lightSeaGreen";
        حالة REACTIVE | OBSERVED | إدخال | ACTIVE | NBA: إرجاع "gray50";
        افتراضي: v3fatalSrc("There are only 5 region bits"); إرجاع "";
        }
    }
    // LCOV_EXCL_STOP
};

صنف SchedReplicateLogicVertex final : عام SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_شجرة* ثابت m_senTreep;  // The sensitivity of the logic node
    Ast_عقدة* ثابت m_logicp;  // The logic node this vertex represents
    RegionFlags ثابت m_assignedRegion;  // The region this logic is originally assigned to

عام:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_شجرة* senTreep,
                              Ast_عقدة* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() ثابت { إرجاع m_scopep; }
    Ast_Sen_شجرة* senTreep() ثابت { إرجاع m_senTreep; }
    Ast_عقدة* logicp() ثابت { إرجاع m_logicp; }
    RegionFlags assignedRegion() ثابت { إرجاع m_assignedRegion; }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { إرجاع m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { إرجاع "rectangle"; }
};

صنف SchedReplicateVarVertex final : عام SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عام:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        إذا (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(إدخال);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        إذا (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() ثابت { إرجاع m_vscp; }
    AstVar* varp() ثابت { إرجاع m_vscp->varp(); }
    AstScope* scopep() ثابت { إرجاع m_vscp->scopep(); }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { إرجاع m_vscp->name(); }
    string dotShape() ثابت override { إرجاع varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

صنف Graph final : عام V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ثابت LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{جديد Graph};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1InUse user1InUse;
    ثابت auto getVarVertex = [&](AstVarScope* vscp) {
        إذا (!vscp->user1p()) vscp->user1p(جديد SchedReplicateVarVertex{graphp.get(), vscp});
        إرجاع vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    ثابت auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* قمة) {
        جديد V3GraphEdge{graphp.get(), fromp, قمة, 1};
    };

    ثابت auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_شجرة* ثابت senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<منطقي(AstVarScope*)> readTriggersThisLogic;

        ثابت VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        إذا (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { إرجاع خطأ; };
        } وإلا إذا (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { إرجاع true; };
        } وإلا {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { إرجاع !vscp->user4(); };
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user4(true);
            });
        }

        لكل (Ast_عقدة* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* ثابت lvtxp
                = جديد SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            ثابت VNUser2InUse user2InUse;
            ثابت VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                SchedReplicateVarVertex* ثابت vvtxp = getVarVertex(vscp);
                إذا (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.تعداد_2(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                إذا (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
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

    إرجاع graphp;
}

فراغ propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    إذا (vtxp->user()) إرجاع;

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
    vtxp->user(true);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        إذا (SchedReplicateLogicVertex* ثابت lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            ثابت auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_شجرة(خطأ));
            };
            ثابت uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            إذا (targetRegions & إدخال) replicateTo(result.m_ico);
            إذا (targetRegions & ACTIVE) replicateTo(result.m_act);
            إذا (targetRegions & NBA) replicateTo(result.m_nba);
            إذا (targetRegions & OBSERVED) replicateTo(result.m_obs);
            إذا (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    إرجاع result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    إذا (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    إرجاع replicate(graphp.get());
}

}  // namespace V3Sched
