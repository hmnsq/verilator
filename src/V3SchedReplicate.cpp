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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Graph.h"
#समावेश "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

// Driving region flags
क्रमागत RegionFlags : uint8_t {
    NONE = 0x0,  //
    निवेश = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

श्रेणी SchedReplicateVertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

खुला:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() स्थिर { वापस m_drivingRegions; }
    व्योम addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() स्थिर override {
        चयन (static_cast<अचिन्हित>(m_drivingRegions)) {
        हाल NONE: वापस "black";
        हाल निवेश: वापस "red";
        हाल ACTIVE: वापस "green";
        हाल NBA: वापस "blue";
        हाल निवेश | ACTIVE: वापस "yellow";
        हाल निवेश | NBA: वापस "magenta";
        हाल ACTIVE | NBA: वापस "cyan";
        हाल निवेश | ACTIVE | NBA: वापस "gray80";  // don't want white on white background

        हाल REACTIVE: वापस "gray60";
        हाल REACTIVE | निवेश: वापस "lightcoral";
        हाल REACTIVE | ACTIVE: वापस "lightgreen";
        हाल REACTIVE | NBA: वापस "lightblue";
        हाल REACTIVE | निवेश | ACTIVE: वापस "lightyellow";
        हाल REACTIVE | निवेश | NBA: वापस "lightpink";
        हाल REACTIVE | ACTIVE | NBA: वापस "lightcyan";
        हाल REACTIVE | निवेश | ACTIVE | NBA: वापस "gray90";

        हाल OBSERVED: वापस "gray20";
        हाल OBSERVED | निवेश: वापस "darkred";
        हाल OBSERVED | ACTIVE: वापस "darkgreen";
        हाल OBSERVED | NBA: वापस "darkblue";
        हाल OBSERVED | निवेश | ACTIVE: वापस "gold";
        हाल OBSERVED | निवेश | NBA: वापस "purple";
        हाल OBSERVED | ACTIVE | NBA: वापस "darkcyan";
        हाल OBSERVED | निवेश | ACTIVE | NBA: वापस "gray30";

        हाल REACTIVE | OBSERVED: वापस "gray40";
        हाल REACTIVE | OBSERVED | निवेश: वापस "indianred";
        हाल REACTIVE | OBSERVED | ACTIVE: वापस "olive";
        हाल REACTIVE | OBSERVED | NBA: वापस "dodgerBlue";
        हाल REACTIVE | OBSERVED | निवेश | ACTIVE: वापस "khaki";
        हाल REACTIVE | OBSERVED | निवेश | NBA: वापस "plum";
        हाल REACTIVE | OBSERVED | ACTIVE | NBA: वापस "lightSeaGreen";
        हाल REACTIVE | OBSERVED | निवेश | ACTIVE | NBA: वापस "gray50";
        शेष: v3fatalSrc("There are only 5 region bits"); वापस "";
        }
    }
    // LCOV_EXCL_STOP
};

श्रेणी SchedReplicateLogicVertex final : खुला SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* स्थिर m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_वृक्ष* स्थिर m_senTreep;  // The sensitivity of the logic node
    Ast_गाँठ* स्थिर m_logicp;  // The logic node this vertex represents
    RegionFlags स्थिर m_assignedRegion;  // The region this logic is originally assigned to

खुला:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_वृक्ष* senTreep,
                              Ast_गाँठ* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() स्थिर { वापस m_scopep; }
    Ast_Sen_वृक्ष* senTreep() स्थिर { वापस m_senTreep; }
    Ast_गाँठ* logicp() स्थिर { वापस m_logicp; }
    RegionFlags assignedRegion() स्थिर { वापस m_assignedRegion; }

    // For graph dumping
    string name() स्थिर override VL_MT_STABLE { वापस m_logicp->fileline()->ascii(); };
    string dotShape() स्थिर override { वापस "rectangle"; }
};

श्रेणी SchedReplicateVarVertex final : खुला SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* स्थिर m_vscp;  // The AstVarScope this vertex represents

खुला:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        अगर (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(निवेश);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        अगर (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() स्थिर { वापस m_vscp; }
    AstVar* varp() स्थिर { वापस m_vscp->varp(); }
    AstScope* scopep() स्थिर { वापस m_vscp->scopep(); }

    // For graph dumping
    string name() स्थिर override VL_MT_STABLE { वापस m_vscp->name(); }
    string dotShape() स्थिर override { वापस varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

श्रेणी Graph final : खुला V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(स्थिर LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{नया Graph};

    // AstVarScope::user1() -> VarVertx
    स्थिर VNUser1InUse user1InUse;
    स्थिर स्वतः getVarVertex = [&](AstVarScope* vscp) {
        अगर (!vscp->user1p()) vscp->user1p(नया SchedReplicateVarVertex{graphp.लो(), vscp});
        वापस vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    स्थिर स्वतः addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* शीर्ष) {
        नया V3GraphEdge{graphp.लो(), fromp, शीर्ष, 1};
    };

    स्थिर स्वतः addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_वृक्ष* स्थिर senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<bool(AstVarScope*)> readTriggersThisLogic;

        स्थिर VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        अगर (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { वापस false; };
        } अन्यथा अगर (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { वापस true; };
        } अन्यथा {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { वापस !vscp->user4(); };
            senTreep->foreach([](स्थिर AstVarRef* refp) {  //
                refp->varScopep()->user4(true);
            });
        }

        क्रम (Ast_गाँठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* स्थिर lvtxp
                = नया SchedReplicateLogicVertex{graphp.लो(), scopep, senTreep, nodep, region};
            स्थिर VNUser2InUse user2InUse;
            स्थिर VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* स्थिर vscp = refp->varScopep();
                SchedReplicateVarVertex* स्थिर vvtxp = getVarVertex(vscp);
                अगर (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.गिनती(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                अगर (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    क्रम (स्थिर स्वतः& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    क्रम (स्थिर स्वतः& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    क्रम (स्थिर स्वतः& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    क्रम (स्थिर स्वतः& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    क्रम (स्थिर स्वतः& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    वापस graphp;
}

व्योम propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    अगर (vtxp->user()) वापस;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    क्रम (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* स्थिर srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(true);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas परिणाम;
    क्रम (V3GraphVertex& vtx : graphp->vertices()) {
        अगर (SchedReplicateLogicVertex* स्थिर lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            स्थिर स्वतः replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_वृक्ष(false));
            };
            स्थिर uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            अगर (targetRegions & निवेश) replicateTo(परिणाम.m_ico);
            अगर (targetRegions & ACTIVE) replicateTo(परिणाम.m_act);
            अगर (targetRegions & NBA) replicateTo(परिणाम.m_nba);
            अगर (targetRegions & OBSERVED) replicateTo(परिणाम.m_obs);
            अगर (targetRegions & REACTIVE) replicateTo(परिणाम.m_react);
        }
    }
    वापस परिणाम;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    स्थिर std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    क्रम (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    वापस replicate(graphp.लो());
}

}  // namespace V3Sched
