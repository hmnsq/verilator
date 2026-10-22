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
    uint8_t drivingRegions() ध्रुव { प्रत्यागम m_drivingRegions; }
    शून्य addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ध्रुव override {
        विकल्प (static_cast<unsigned>(m_drivingRegions)) {
        पक्ष NONE: प्रत्यागम "black";
        पक्ष आदान: प्रत्यागम "red";
        पक्ष ACTIVE: प्रत्यागम "green";
        पक्ष NBA: प्रत्यागम "blue";
        पक्ष आदान | ACTIVE: प्रत्यागम "yellow";
        पक्ष आदान | NBA: प्रत्यागम "magenta";
        पक्ष ACTIVE | NBA: प्रत्यागम "cyan";
        पक्ष आदान | ACTIVE | NBA: प्रत्यागम "gray80";  // don't want white on white background

        पक्ष REACTIVE: प्रत्यागम "gray60";
        पक्ष REACTIVE | आदान: प्रत्यागम "lightcoral";
        पक्ष REACTIVE | ACTIVE: प्रत्यागम "lightgreen";
        पक्ष REACTIVE | NBA: प्रत्यागम "lightblue";
        पक्ष REACTIVE | आदान | ACTIVE: प्रत्यागम "lightyellow";
        पक्ष REACTIVE | आदान | NBA: प्रत्यागम "lightpink";
        पक्ष REACTIVE | ACTIVE | NBA: प्रत्यागम "lightcyan";
        पक्ष REACTIVE | आदान | ACTIVE | NBA: प्रत्यागम "gray90";

        पक्ष OBSERVED: प्रत्यागम "gray20";
        पक्ष OBSERVED | आदान: प्रत्यागम "darkred";
        पक्ष OBSERVED | ACTIVE: प्रत्यागम "darkgreen";
        पक्ष OBSERVED | NBA: प्रत्यागम "darkblue";
        पक्ष OBSERVED | आदान | ACTIVE: प्रत्यागम "gold";
        पक्ष OBSERVED | आदान | NBA: प्रत्यागम "purple";
        पक्ष OBSERVED | ACTIVE | NBA: प्रत्यागम "darkcyan";
        पक्ष OBSERVED | आदान | ACTIVE | NBA: प्रत्यागम "gray30";

        पक्ष REACTIVE | OBSERVED: प्रत्यागम "gray40";
        पक्ष REACTIVE | OBSERVED | आदान: प्रत्यागम "indianred";
        पक्ष REACTIVE | OBSERVED | ACTIVE: प्रत्यागम "olive";
        पक्ष REACTIVE | OBSERVED | NBA: प्रत्यागम "dodgerBlue";
        पक्ष REACTIVE | OBSERVED | आदान | ACTIVE: प्रत्यागम "khaki";
        पक्ष REACTIVE | OBSERVED | आदान | NBA: प्रत्यागम "plum";
        पक्ष REACTIVE | OBSERVED | ACTIVE | NBA: प्रत्यागम "lightSeaGreen";
        पक्ष REACTIVE | OBSERVED | आदान | ACTIVE | NBA: प्रत्यागम "gray50";
        स्वभाव: v3fatalSrc("There are only 5 region bits"); प्रत्यागम "";
        }
    }
    // LCOV_EXCL_STOP
};

वर्ग SchedReplicateLogicVertex final : सार्वजनिक SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* ध्रुव m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_वृक्ष* ध्रुव m_senTreep;  // The sensitivity of the logic node
    Ast_गाठ* ध्रुव m_logicp;  // The logic node this vertex represents
    RegionFlags ध्रुव m_assignedRegion;  // The region this logic is originally assigned to

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
    AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }
    Ast_Sen_वृक्ष* senTreep() ध्रुव { प्रत्यागम m_senTreep; }
    Ast_गाठ* logicp() ध्रुव { प्रत्यागम m_logicp; }
    RegionFlags assignedRegion() ध्रुव { प्रत्यागम m_assignedRegion; }

    // For graph dumping
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_logicp->fileline()->ascii(); };
    string dotShape() ध्रुव override { प्रत्यागम "rectangle"; }
};

वर्ग SchedReplicateVarVertex final : सार्वजनिक SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* ध्रुव m_vscp;  // The AstVarScope this vertex represents

सार्वजनिक:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        यदि (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(आदान);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        यदि (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() ध्रुव { प्रत्यागम m_vscp; }
    AstVar* varp() ध्रुव { प्रत्यागम m_vscp->varp(); }
    AstScope* scopep() ध्रुव { प्रत्यागम m_vscp->scopep(); }

    // For graph dumping
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_vscp->name(); }
    string dotShape() ध्रुव override { प्रत्यागम varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

वर्ग Graph final : सार्वजनिक V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ध्रुव LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{नूतन Graph};

    // AstVarScope::user1() -> VarVertx
    ध्रुव VNUser1InUse user1InUse;
    ध्रुव auto getVarVertex = [&](AstVarScope* vscp) {
        यदि (!vscp->user1p()) vscp->user1p(नूतन SchedReplicateVarVertex{graphp.get(), vscp});
        प्रत्यागम vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    ध्रुव auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* शीर्ष) {
        नूतन V3GraphEdge{graphp.get(), fromp, शीर्ष, 1};
    };

    ध्रुव auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_वृक्ष* ध्रुव senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<तार्किक(AstVarScope*)> readTriggersThisLogic;

        ध्रुव VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        यदि (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { प्रत्यागम असत्य; };
        } अन्यथा यदि (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { प्रत्यागम सत्य; };
        } अन्यथा {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { प्रत्यागम !vscp->user4(); };
            senTreep->foreach([](ध्रुव AstVarRef* refp) {  //
                refp->varScopep()->user4(सत्य);
            });
        }

        प्रति (Ast_गाठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* ध्रुव lvtxp
                = नूतन SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            ध्रुव VNUser2InUse user2InUse;
            ध्रुव VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ध्रुव vscp = refp->varScopep();
                SchedReplicateVarVertex* ध्रुव vvtxp = getVarVertex(vscp);
                यदि (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.मोजणी(vscp)) {  //
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

    प्रति (ध्रुव auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    प्रति (ध्रुव auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    प्रति (ध्रुव auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    प्रति (ध्रुव auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    प्रति (ध्रुव auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    प्रत्यागम graphp;
}

शून्य propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    यदि (vtxp->user()) प्रत्यागम;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    प्रति (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* ध्रुव srcp = edge.fromp()->as<SchedReplicateVertex>();
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
    प्रति (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (SchedReplicateLogicVertex* ध्रुव lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            ध्रुव auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_वृक्ष(असत्य));
            };
            ध्रुव uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            यदि (targetRegions & आदान) replicateTo(result.m_ico);
            यदि (targetRegions & ACTIVE) replicateTo(result.m_act);
            यदि (targetRegions & NBA) replicateTo(result.m_nba);
            यदि (targetRegions & OBSERVED) replicateTo(result.m_obs);
            यदि (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    प्रत्यागम result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ध्रुव std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    प्रति (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    प्रत्यागम replicate(graphp.get());
}

}  // namespace V3Sched
