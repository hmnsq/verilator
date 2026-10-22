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
    आगत = 0x1,  // Variable/logic is driven from top level input
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
    uint8_t drivingRegions() अचर { फर्काऊ m_drivingRegions; }
    शून्य addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() अचर override {
        विकल्प (static_cast<अचिन्हित>(m_drivingRegions)) {
        पक्ष NONE: फर्काऊ "black";
        पक्ष आगत: फर्काऊ "red";
        पक्ष ACTIVE: फर्काऊ "green";
        पक्ष NBA: फर्काऊ "blue";
        पक्ष आगत | ACTIVE: फर्काऊ "yellow";
        पक्ष आगत | NBA: फर्काऊ "magenta";
        पक्ष ACTIVE | NBA: फर्काऊ "cyan";
        पक्ष आगत | ACTIVE | NBA: फर्काऊ "gray80";  // don't want white on white background

        पक्ष REACTIVE: फर्काऊ "gray60";
        पक्ष REACTIVE | आगत: फर्काऊ "lightcoral";
        पक्ष REACTIVE | ACTIVE: फर्काऊ "lightgreen";
        पक्ष REACTIVE | NBA: फर्काऊ "lightblue";
        पक्ष REACTIVE | आगत | ACTIVE: फर्काऊ "lightyellow";
        पक्ष REACTIVE | आगत | NBA: फर्काऊ "lightpink";
        पक्ष REACTIVE | ACTIVE | NBA: फर्काऊ "lightcyan";
        पक्ष REACTIVE | आगत | ACTIVE | NBA: फर्काऊ "gray90";

        पक्ष OBSERVED: फर्काऊ "gray20";
        पक्ष OBSERVED | आगत: फर्काऊ "darkred";
        पक्ष OBSERVED | ACTIVE: फर्काऊ "darkgreen";
        पक्ष OBSERVED | NBA: फर्काऊ "darkblue";
        पक्ष OBSERVED | आगत | ACTIVE: फर्काऊ "gold";
        पक्ष OBSERVED | आगत | NBA: फर्काऊ "purple";
        पक्ष OBSERVED | ACTIVE | NBA: फर्काऊ "darkcyan";
        पक्ष OBSERVED | आगत | ACTIVE | NBA: फर्काऊ "gray30";

        पक्ष REACTIVE | OBSERVED: फर्काऊ "gray40";
        पक्ष REACTIVE | OBSERVED | आगत: फर्काऊ "indianred";
        पक्ष REACTIVE | OBSERVED | ACTIVE: फर्काऊ "olive";
        पक्ष REACTIVE | OBSERVED | NBA: फर्काऊ "dodgerBlue";
        पक्ष REACTIVE | OBSERVED | आगत | ACTIVE: फर्काऊ "khaki";
        पक्ष REACTIVE | OBSERVED | आगत | NBA: फर्काऊ "plum";
        पक्ष REACTIVE | OBSERVED | ACTIVE | NBA: फर्काऊ "lightSeaGreen";
        पक्ष REACTIVE | OBSERVED | आगत | ACTIVE | NBA: फर्काऊ "gray50";
        स्वभाव: v3fatalSrc("There are only 5 region bits"); फर्काऊ "";
        }
    }
    // LCOV_EXCL_STOP
};

वर्ग SchedReplicateLogicVertex final : सार्वजनिक SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* अचर m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_रूख* अचर m_senTreep;  // The sensitivity of the logic node
    Ast_गाँठो* अचर m_logicp;  // The logic node this vertex represents
    RegionFlags अचर m_assignedRegion;  // The region this logic is originally assigned to

सार्वजनिक:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_रूख* senTreep,
                              Ast_गाँठो* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() अचर { फर्काऊ m_scopep; }
    Ast_Sen_रूख* senTreep() अचर { फर्काऊ m_senTreep; }
    Ast_गाँठो* logicp() अचर { फर्काऊ m_logicp; }
    RegionFlags assignedRegion() अचर { फर्काऊ m_assignedRegion; }

    // For graph dumping
    string name() अचर override VL_MT_STABLE { फर्काऊ m_logicp->fileline()->ascii(); };
    string dotShape() अचर override { फर्काऊ "rectangle"; }
};

वर्ग SchedReplicateVarVertex final : सार्वजनिक SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* अचर m_vscp;  // The AstVarScope this vertex represents

सार्वजनिक:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        यदि (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(आगत);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        यदि (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() अचर { फर्काऊ m_vscp; }
    AstVar* varp() अचर { फर्काऊ m_vscp->varp(); }
    AstScope* scopep() अचर { फर्काऊ m_vscp->scopep(); }

    // For graph dumping
    string name() अचर override VL_MT_STABLE { फर्काऊ m_vscp->name(); }
    string dotShape() अचर override { फर्काऊ varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

वर्ग Graph final : सार्वजनिक V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(अचर LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{नूतन Graph};

    // AstVarScope::user1() -> VarVertx
    अचर VNUser1InUse user1InUse;
    अचर स्वतः getVarVertex = [&](AstVarScope* vscp) {
        यदि (!vscp->user1p()) vscp->user1p(नूतन SchedReplicateVarVertex{graphp.get(), vscp});
        फर्काऊ vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    अचर स्वतः addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* शीर्ष) {
        नूतन V3GraphEdge{graphp.get(), fromp, शीर्ष, 1};
    };

    अचर स्वतः addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_रूख* अचर senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<तार्किक(AstVarScope*)> readTriggersThisLogic;

        अचर VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        यदि (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { फर्काऊ असत्य; };
        } अन्यथा यदि (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { फर्काऊ सत्य; };
        } अन्यथा {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { फर्काऊ !vscp->user4(); };
            senTreep->foreach([](अचर AstVarRef* refp) {  //
                refp->varScopep()->user4(सत्य);
            });
        }

        लागि (Ast_गाँठो* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* अचर lvtxp
                = नूतन SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            अचर VNUser2InUse user2InUse;
            अचर VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* अचर vscp = refp->varScopep();
                SchedReplicateVarVertex* अचर vvtxp = getVarVertex(vscp);
                यदि (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.गन्ती(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                यदि (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    लागि (अचर स्वतः& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    लागि (अचर स्वतः& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    लागि (अचर स्वतः& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    लागि (अचर स्वतः& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    लागि (अचर स्वतः& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    फर्काऊ graphp;
}

शून्य propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    यदि (vtxp->user()) फर्काऊ;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    लागि (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* अचर srcp = edge.fromp()->as<SchedReplicateVertex>();
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
    लागि (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (SchedReplicateLogicVertex* अचर lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            अचर स्वतः replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_रूख(असत्य));
            };
            अचर uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            यदि (targetRegions & आगत) replicateTo(result.m_ico);
            यदि (targetRegions & ACTIVE) replicateTo(result.m_act);
            यदि (targetRegions & NBA) replicateTo(result.m_nba);
            यदि (targetRegions & OBSERVED) replicateTo(result.m_obs);
            यदि (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    फर्काऊ result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    अचर std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    लागि (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    फर्काऊ replicate(graphp.get());
}

}  // namespace V3Sched
