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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

നാമാകാശ V3Sched {

നാമാകാശ {

// Driving region flags
എണ്ണൽ RegionFlags : uint8_t {
    NONE = 0x0,  //
    നിവേശം = 0x1,  // Variable/logic is driven from top level input
    ACTIVE = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

വർഗ്ഗം SchedReplicateVertex VL_NOT_FINAL : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(SchedReplicateVertex, V3GraphVertex)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

പൊതു:
    explicit SchedReplicateVertex(V3Graph* graphp)
        : V3GraphVertex{graphp} {}
    uint8_t drivingRegions() സ്ഥിരം { തിരികെ m_drivingRegions; }
    ശൂന്യം addDrivingRegions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() സ്ഥിരം override {
        തിരഞ്ഞെടുപ്പ് (static_cast<unsigned>(m_drivingRegions)) {
        അവസ്ഥ NONE: തിരികെ "black";
        അവസ്ഥ നിവേശം: തിരികെ "red";
        അവസ്ഥ ACTIVE: തിരികെ "green";
        അവസ്ഥ NBA: തിരികെ "blue";
        അവസ്ഥ നിവേശം | ACTIVE: തിരികെ "yellow";
        അവസ്ഥ നിവേശം | NBA: തിരികെ "magenta";
        അവസ്ഥ ACTIVE | NBA: തിരികെ "cyan";
        അവസ്ഥ നിവേശം | ACTIVE | NBA: തിരികെ "gray80";  // don't want white on white background

        അവസ്ഥ REACTIVE: തിരികെ "gray60";
        അവസ്ഥ REACTIVE | നിവേശം: തിരികെ "lightcoral";
        അവസ്ഥ REACTIVE | ACTIVE: തിരികെ "lightgreen";
        അവസ്ഥ REACTIVE | NBA: തിരികെ "lightblue";
        അവസ്ഥ REACTIVE | നിവേശം | ACTIVE: തിരികെ "lightyellow";
        അവസ്ഥ REACTIVE | നിവേശം | NBA: തിരികെ "lightpink";
        അവസ്ഥ REACTIVE | ACTIVE | NBA: തിരികെ "lightcyan";
        അവസ്ഥ REACTIVE | നിവേശം | ACTIVE | NBA: തിരികെ "gray90";

        അവസ്ഥ OBSERVED: തിരികെ "gray20";
        അവസ്ഥ OBSERVED | നിവേശം: തിരികെ "darkred";
        അവസ്ഥ OBSERVED | ACTIVE: തിരികെ "darkgreen";
        അവസ്ഥ OBSERVED | NBA: തിരികെ "darkblue";
        അവസ്ഥ OBSERVED | നിവേശം | ACTIVE: തിരികെ "gold";
        അവസ്ഥ OBSERVED | നിവേശം | NBA: തിരികെ "purple";
        അവസ്ഥ OBSERVED | ACTIVE | NBA: തിരികെ "darkcyan";
        അവസ്ഥ OBSERVED | നിവേശം | ACTIVE | NBA: തിരികെ "gray30";

        അവസ്ഥ REACTIVE | OBSERVED: തിരികെ "gray40";
        അവസ്ഥ REACTIVE | OBSERVED | നിവേശം: തിരികെ "indianred";
        അവസ്ഥ REACTIVE | OBSERVED | ACTIVE: തിരികെ "olive";
        അവസ്ഥ REACTIVE | OBSERVED | NBA: തിരികെ "dodgerBlue";
        അവസ്ഥ REACTIVE | OBSERVED | നിവേശം | ACTIVE: തിരികെ "khaki";
        അവസ്ഥ REACTIVE | OBSERVED | നിവേശം | NBA: തിരികെ "plum";
        അവസ്ഥ REACTIVE | OBSERVED | ACTIVE | NBA: തിരികെ "lightSeaGreen";
        അവസ്ഥ REACTIVE | OBSERVED | നിവേശം | ACTIVE | NBA: തിരികെ "gray50";
        സ്ഥിരസ്ഥിതി: v3fatalSrc("There are only 5 region bits"); തിരികെ "";
        }
    }
    // LCOV_EXCL_STOP
};

വർഗ്ഗം SchedReplicateLogicVertex final : പൊതു SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateLogicVertex, SchedReplicateVertex)
    AstScope* സ്ഥിരം m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_വൃക്ഷം* സ്ഥിരം m_senTreep;  // The sensitivity of the logic node
    Ast_കണ്ണി* സ്ഥിരം m_logicp;  // The logic node this vertex represents
    RegionFlags സ്ഥിരം m_assignedRegion;  // The region this logic is originally assigned to

പൊതു:
    SchedReplicateLogicVertex(V3Graph* graphp, AstScope* scopep, Ast_Sen_വൃക്ഷം* senTreep,
                              Ast_കണ്ണി* logicp, RegionFlags assignedRegion)
        : SchedReplicateVertex{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        addDrivingRegions(assignedRegion);
    }
    AstScope* scopep() സ്ഥിരം { തിരികെ m_scopep; }
    Ast_Sen_വൃക്ഷം* senTreep() സ്ഥിരം { തിരികെ m_senTreep; }
    Ast_കണ്ണി* logicp() സ്ഥിരം { തിരികെ m_logicp; }
    RegionFlags assignedRegion() സ്ഥിരം { തിരികെ m_assignedRegion; }

    // For graph dumping
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_logicp->fileline()->ascii(); };
    string dotShape() സ്ഥിരം override { തിരികെ "rectangle"; }
};

വർഗ്ഗം SchedReplicateVarVertex final : പൊതു SchedReplicateVertex {
    VL_RTTI_IMPL(SchedReplicateVarVertex, SchedReplicateVertex)
    AstVarScope* സ്ഥിരം m_vscp;  // The AstVarScope this vertex represents

പൊതു:
    SchedReplicateVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : SchedReplicateVertex{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        എങ്കിൽ (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            addDrivingRegions(നിവേശം);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        എങ്കിൽ (varp()->isWrittenBySuspendable()) addDrivingRegions(ACTIVE);
    }
    AstVarScope* vscp() സ്ഥിരം { തിരികെ m_vscp; }
    AstVar* varp() സ്ഥിരം { തിരികെ m_vscp->varp(); }
    AstScope* scopep() സ്ഥിരം { തിരികെ m_vscp->scopep(); }

    // For graph dumping
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_vscp->name(); }
    string dotShape() സ്ഥിരം override { തിരികെ varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

വർഗ്ഗം Graph final : പൊതു V3Graph {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(സ്ഥിരം LogicRegions& logicRegions) {
    std::unique_ptr<Graph> graphp{പുതിയ Graph};

    // AstVarScope::user1() -> VarVertx
    സ്ഥിരം VNUser1InUse user1InUse;
    സ്ഥിരം auto getVarVertex = [&](AstVarScope* vscp) {
        എങ്കിൽ (!vscp->user1p()) vscp->user1p(പുതിയ SchedReplicateVarVertex{graphp.get(), vscp});
        തിരികെ vscp->user1u().to<SchedReplicateVarVertex*>();
    };

    സ്ഥിരം auto addEdge = [&](SchedReplicateVertex* fromp, SchedReplicateVertex* മുകൾ) {
        പുതിയ V3GraphEdge{graphp.get(), fromp, മുകൾ, 1};
    };

    സ്ഥിരം auto addLogic = [&](RegionFlags region, AstScope* scopep, AstActive* activep) {
        Ast_Sen_വൃക്ഷം* സ്ഥിരം senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<തർക്കം(AstVarScope*)> readTriggersThisLogic;

        സ്ഥിരം VNUser4InUse user4InUse;  // bool: Explicit sensitivity of hybrid logic just below

        എങ്കിൽ (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { തിരികെ അസത്യം; };
        } അല്ലെങ്കിൽ എങ്കിൽ (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { തിരികെ സത്യം; };
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { തിരികെ !vscp->user4(); };
            senTreep->foreach([](സ്ഥിരം AstVarRef* refp) {  //
                refp->varScopep()->user4(സത്യം);
            });
        }

        വേണ്ടി (Ast_കണ്ണി* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            SchedReplicateLogicVertex* സ്ഥിരം lvtxp
                = പുതിയ SchedReplicateLogicVertex{graphp.get(), scopep, senTreep, nodep, region};
            സ്ഥിരം VNUser2InUse user2InUse;
            സ്ഥിരം VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* സ്ഥിരം vscp = refp->varScopep();
                SchedReplicateVarVertex* സ്ഥിരം vvtxp = getVarVertex(vscp);
                എങ്കിൽ (refp->access().isReadOrRW() && !vscp->user3SetOnce()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !forceReadEdgeIgnores.എണ്ണം(vscp)) {  //
                    addEdge(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                എങ്കിൽ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    addEdge(lvtxp, vvtxp);
                }
            });
        }
    };

    വേണ്ടി (സ്ഥിരം auto& entry : logicRegions.m_pre) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    വേണ്ടി (സ്ഥിരം auto& entry : logicRegions.m_act) addLogic(ACTIVE, entry.m_scopep, entry.m_activep);
    വേണ്ടി (സ്ഥിരം auto& entry : logicRegions.m_nba) addLogic(NBA, entry.m_scopep, entry.m_activep);
    വേണ്ടി (സ്ഥിരം auto& entry : logicRegions.m_obs)
        addLogic(OBSERVED, entry.m_scopep, entry.m_activep);
    വേണ്ടി (സ്ഥിരം auto& entry : logicRegions.m_react)
        addLogic(REACTIVE, entry.m_scopep, entry.m_activep);

    തിരികെ graphp;
}

ശൂന്യം propagateDrivingRegions(SchedReplicateVertex* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    എങ്കിൽ (vtxp->user()) തിരികെ;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    വേണ്ടി (V3GraphEdge& edge : vtxp->inEdges()) {
        SchedReplicateVertex* സ്ഥിരം srcp = edge.fromp()->as<SchedReplicateVertex>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->addDrivingRegions(drivingRegions);

    // Mark as visited
    vtxp->user(സത്യം);
}

LogicReplicas replicate(Graph* graphp) {
    LogicReplicas result;
    വേണ്ടി (V3GraphVertex& vtx : graphp->vertices()) {
        എങ്കിൽ (SchedReplicateLogicVertex* സ്ഥിരം lvtxp = vtx.cast<SchedReplicateLogicVertex>()) {
            സ്ഥിരം auto replicateTo = [&](LogicByScope& lbs) {
                lbs.add(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_വൃക്ഷം(അസത്യം));
            };
            സ്ഥിരം uint8_t targetRegions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || targetRegions == 0,
                    "replicating clocked logic");
            എങ്കിൽ (targetRegions & നിവേശം) replicateTo(result.m_ico);
            എങ്കിൽ (targetRegions & ACTIVE) replicateTo(result.m_act);
            എങ്കിൽ (targetRegions & NBA) replicateTo(result.m_nba);
            എങ്കിൽ (targetRegions & OBSERVED) replicateTo(result.m_obs);
            എങ്കിൽ (targetRegions & REACTIVE) replicateTo(result.m_react);
        }
    }
    തിരികെ result;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    സ്ഥിരം std::unique_ptr<Graph> graphp = buildGraph(logicRegionsRegions);
    // Dump for debug
    എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    വേണ്ടി (V3GraphVertex& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<SchedReplicateVertex>());
    }
    // Dump for debug
    എങ്കിൽ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    തിരികെ replicate(graphp.get());
}

}  // namespace V3Sched
