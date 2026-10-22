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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {

ନାମାକାଶ {

// Driving region flags
ଗଣନା RegionFlags : uint8_t {
    NONE = 0x0,  //
    ନିବେଶ = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

ଶ୍ରେଣୀ SchedReplicateVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

ସର୍ବସାଧାରଣ:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() ଅଚଳ { ଫେରନ୍ତୁ m_drivingRegions; }
    ଶୂନ୍ୟ addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ଅଚଳ override {
        ବାଛ (static_cast<unsigned>(m_drivingRegions)) {
        ସ୍ଥିତି NONE: ଫେରନ୍ତୁ "black";
        ସ୍ଥିତି ନିବେଶ: ଫେରନ୍ତୁ "red";
        ସ୍ଥିତି ACTIVE: ଫେରନ୍ତୁ "green";
        ସ୍ଥିତି NBA: ଫେରନ୍ତୁ "blue";
        ସ୍ଥିତି ନିବେଶ | ACTIVE: ଫେରନ୍ତୁ "yellow";
        ସ୍ଥିତି ନିବେଶ | NBA: ଫେରନ୍ତୁ "magenta";
        ସ୍ଥିତି ACTIVE | NBA: ଫେରନ୍ତୁ "cyan";
        ସ୍ଥିତି ନିବେଶ | ACTIVE | NBA: ଫେରନ୍ତୁ "gray80";  // don't want white on white background

        ସ୍ଥିତି REACTIVE: ଫେରନ୍ତୁ "gray60";
        ସ୍ଥିତି REACTIVE | ନିବେଶ: ଫେରନ୍ତୁ "lightcoral";
        ସ୍ଥିତି REACTIVE | ACTIVE: ଫେରନ୍ତୁ "lightgreen";
        ସ୍ଥିତି REACTIVE | NBA: ଫେରନ୍ତୁ "lightblue";
        ସ୍ଥିତି REACTIVE | ନିବେଶ | ACTIVE: ଫେରନ୍ତୁ "lightyellow";
        ସ୍ଥିତି REACTIVE | ନିବେଶ | NBA: ଫେରନ୍ତୁ "lightpink";
        ସ୍ଥିତି REACTIVE | ACTIVE | NBA: ଫେରନ୍ତୁ "lightcyan";
        ସ୍ଥିତି REACTIVE | ନିବେଶ | ACTIVE | NBA: ଫେରନ୍ତୁ "gray90";

        ସ୍ଥିତି OBSERVED: ଫେରନ୍ତୁ "gray20";
        ସ୍ଥିତି OBSERVED | ନିବେଶ: ଫେରନ୍ତୁ "darkred";
        ସ୍ଥିତି OBSERVED | ACTIVE: ଫେରନ୍ତୁ "darkgreen";
        ସ୍ଥିତି OBSERVED | NBA: ଫେରନ୍ତୁ "darkblue";
        ସ୍ଥିତି OBSERVED | ନିବେଶ | ACTIVE: ଫେରନ୍ତୁ "gold";
        ସ୍ଥିତି OBSERVED | ନିବେଶ | NBA: ଫେରନ୍ତୁ "purple";
        ସ୍ଥିତି OBSERVED | ACTIVE | NBA: ଫେରନ୍ତୁ "darkcyan";
        ସ୍ଥିତି OBSERVED | ନିବେଶ | ACTIVE | NBA: ଫେରନ୍ତୁ "gray30";

        ସ୍ଥିତି REACTIVE | OBSERVED: ଫେରନ୍ତୁ "gray40";
        ସ୍ଥିତି REACTIVE | OBSERVED | ନିବେଶ: ଫେରନ୍ତୁ "indianred";
        ସ୍ଥିତି REACTIVE | OBSERVED | ACTIVE: ଫେରନ୍ତୁ "olive";
        ସ୍ଥିତି REACTIVE | OBSERVED | NBA: ଫେରନ୍ତୁ "dodgerBlue";
        ସ୍ଥିତି REACTIVE | OBSERVED | ନିବେଶ | ACTIVE: ଫେରନ୍ତୁ "khaki";
        ସ୍ଥିତି REACTIVE | OBSERVED | ନିବେଶ | NBA: ଫେରନ୍ତୁ "plum";
        ସ୍ଥିତି REACTIVE | OBSERVED | ACTIVE | NBA: ଫେରନ୍ତୁ "lightSeaGreen";
        ସ୍ଥିତି REACTIVE | OBSERVED | ନିବେଶ | ACTIVE | NBA: ଫେରନ୍ତୁ "gray50";
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: v3fatalSrc("There are only 5 region bits"); ଫେରନ୍ତୁ "";
        }
    }
    // LCOV_EXCL_STOP
};

ଶ୍ରେଣୀ SchedReplicateLogicVertex final : ସର୍ବସାଧାରଣ SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* ଅଚଳ m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_ବୃକ୍ଷ* ଅଚଳ m_senTreep;  // The sensitivity of the logic node
    Ast_ଗଣ୍ଠି* ଅଚଳ m_logicp;  // The logic node this vertex represents
    RegionFlags ଅଚଳ m_assignedRegion;  // The region this logic is originally assigned to

ସର୍ବସାଧାରଣ:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_ବୃକ୍ଷ* senTreep,
                              Ast_ଗଣ୍ଠି* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_scopep; }
    Ast_Sen_ବୃକ୍ଷ* senTreep() ଅଚଳ { ଫେରନ୍ତୁ m_senTreep; }
    Ast_ଗଣ୍ଠି* logicp() ଅଚଳ { ଫେରନ୍ତୁ m_logicp; }
    RegionFlags assignedRegion() ଅଚଳ { ଫେରନ୍ତୁ m_assignedRegion; }

    // For graph dumping
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_logicp->fileline()->ascii(); };
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "rectangle"; }
};

ଶ୍ରେଣୀ SchedReplicateVarVertex final : ସର୍ବସାଧାରଣ SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* ଅଚଳ m_vscp;  // The AstVarScope this vertex represents

ସର୍ବସାଧାରଣ:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        ଯଦି (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(ନିବେଶ);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        ଯଦି (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() ଅଚଳ { ଫେରନ୍ତୁ m_vscp; }
    AstVar* varp() ଅଚଳ { ଫେରନ୍ତୁ m_vscp->varp(); }
    AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_vscp->scopep(); }

    // For graph dumping
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_vscp->name(); }
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

ଶ୍ରେଣୀ Graph final : ସର୍ବସାଧାରଣ V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ଅଚଳ LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{ନୂତନ Graph};

    // AstVarScope::user1() -> VarVertx
    ଅଚଳ VNUser1InUse user1InUse;
    ଅଚଳ auto getVarVertex = [&](AstVarScope* vscp) {
        ଯଦି (!vscp->user1p()) vscp->user1p(ନୂତନ SchedReplicateVarVertex{graphp.get(), vscp});
        ଫେରନ୍ତୁ vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    ଅଚଳ auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* ଶୀର୍ଷ) {
        ନୂତନ V3GraphEdge{graphp.get(), fromp, ଶୀର୍ଷ, 1};
    };

    ଅଚଳ auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<ତାର୍କିକ(AstVarScope*)> readTriggersThisLogic;

        ଅଚଳ VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        ଯଦି (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ଫେରନ୍ତୁ ମିଥ୍ୟା; };
        } ଅନ୍ୟଥା ଯଦି (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ଫେରନ୍ତୁ ସତ୍ୟ; };
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { ଫେରନ୍ତୁ !vscp->user4(); };
            senTreep->foreach([](ଅଚଳ AstVarRef* refp) {  //
                refp->varScopep()->user4(ସତ୍ୟ);
            });
        }

        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* ଅଚଳ lvtxp
                = ନୂତନ SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            ଅଚଳ VNUser2InUse user2InUse;
            ଅଚଳ VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ଅଚଳ vscp = refp->varScopep();
                SchedReplicateVarVertex* ଅଚଳ vvtxp = getVarVertex(vscp);
                ଯଦି (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.ଗଣନା_2(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                ଯଦି (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    ପାଇଁ (ଅଚଳ auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    ପାଇଁ (ଅଚଳ auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    ପାଇଁ (ଅଚଳ auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    ପାଇଁ (ଅଚଳ auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    ପାଇଁ (ଅଚଳ auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    ଫେରନ୍ତୁ graphp;
}

ଶୂନ୍ୟ propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    ଯଦି (vtxp->user()) ଫେରନ୍ତୁ;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    ପାଇଁ (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* ଅଚଳ srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(ସତ୍ୟ);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    ପାଇଁ (V3GraphVertex& vtx : graphp->vertices()) {
        ଯଦି (SchedReplicateLogicVertex* ଅଚଳ lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            ଅଚଳ auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            };
            ଅଚଳ uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            ଯଦି (targetRegions & ନିବେଶ) replicateTo(result.m_ico);
            ଯଦି (targetRegions & ACTIVE) replicateTo(result.m_act);
            ଯଦି (targetRegions & NBA) replicateTo(result.m_nba);
            ଯଦି (targetRegions & OBSERVED) replicateTo(result.m_obs);
            ଯଦି (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    ଫେରନ୍ତୁ result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ଅଚଳ std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    ପାଇଁ (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    ଫେରନ୍ତୁ replicate(graphp.get());
}

}  // namespace V3Sched
