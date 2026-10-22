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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {

నామాకాశ {

// Driving region flags
గణన RegionFlags : uint8_t {
    NONE = 0x0,  //
    ఆదానం = 0x1,  // Variable/logic is driven from top level input
    చురుకైన_2 = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

తరగతి Sched_Replicate_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Sched_Replicate_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

బహిరంగ:
    స్పష్టమైన Sched_Replicate_శీర్షం(V3_లేఖాచిత్రం* graphp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp} {}
    uint8_t drivingRegions() స్థిరరాశి { తిరిగి m_drivingRegions; }
    శూన్య కూడు_Driving_Regions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() స్థిరరాశి override {
        ఎంపిక (static_cast<గుర్తులేని>(m_drivingRegions)) {
        స్థితి NONE: తిరిగి "black";
        స్థితి ఆదానం: తిరిగి "red";
        స్థితి చురుకైన_2: తిరిగి "green";
        స్థితి NBA: తిరిగి "blue";
        స్థితి ఆదానం | చురుకైన_2: తిరిగి "yellow";
        స్థితి ఆదానం | NBA: తిరిగి "magenta";
        స్థితి చురుకైన_2 | NBA: తిరిగి "cyan";
        స్థితి ఆదానం | చురుకైన_2 | NBA: తిరిగి "gray80";  // don't want white on white background

        స్థితి REACTIVE: తిరిగి "gray60";
        స్థితి REACTIVE | ఆదానం: తిరిగి "lightcoral";
        స్థితి REACTIVE | చురుకైన_2: తిరిగి "lightgreen";
        స్థితి REACTIVE | NBA: తిరిగి "lightblue";
        స్థితి REACTIVE | ఆదానం | చురుకైన_2: తిరిగి "lightyellow";
        స్థితి REACTIVE | ఆదానం | NBA: తిరిగి "lightpink";
        స్థితి REACTIVE | చురుకైన_2 | NBA: తిరిగి "lightcyan";
        స్థితి REACTIVE | ఆదానం | చురుకైన_2 | NBA: తిరిగి "gray90";

        స్థితి OBSERVED: తిరిగి "gray20";
        స్థితి OBSERVED | ఆదానం: తిరిగి "darkred";
        స్థితి OBSERVED | చురుకైన_2: తిరిగి "darkgreen";
        స్థితి OBSERVED | NBA: తిరిగి "darkblue";
        స్థితి OBSERVED | ఆదానం | చురుకైన_2: తిరిగి "gold";
        స్థితి OBSERVED | ఆదానం | NBA: తిరిగి "purple";
        స్థితి OBSERVED | చురుకైన_2 | NBA: తిరిగి "darkcyan";
        స్థితి OBSERVED | ఆదానం | చురుకైన_2 | NBA: తిరిగి "gray30";

        స్థితి REACTIVE | OBSERVED: తిరిగి "gray40";
        స్థితి REACTIVE | OBSERVED | ఆదానం: తిరిగి "indianred";
        స్థితి REACTIVE | OBSERVED | చురుకైన_2: తిరిగి "olive";
        స్థితి REACTIVE | OBSERVED | NBA: తిరిగి "dodgerBlue";
        స్థితి REACTIVE | OBSERVED | ఆదానం | చురుకైన_2: తిరిగి "khaki";
        స్థితి REACTIVE | OBSERVED | ఆదానం | NBA: తిరిగి "plum";
        స్థితి REACTIVE | OBSERVED | చురుకైన_2 | NBA: తిరిగి "lightSeaGreen";
        స్థితి REACTIVE | OBSERVED | ఆదానం | చురుకైన_2 | NBA: తిరిగి "gray50";
        అప్రమేయ: v3fatalSrc("There are only 5 region bits"); తిరిగి "";
        }
    }
    // LCOV_EXCL_STOP
};

తరగతి Sched_Replicate_Logic_శీర్షం final : బహిరంగ Sched_Replicate_శీర్షం {
    VL_RTTI_IMPL(Sched_Replicate_Logic_శీర్షం, Sched_Replicate_శీర్షం)
    AstScope* స్థిరరాశి m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_వృక్షం* స్థిరరాశి m_senTreep;  // The sensitivity of the logic node
    Ast_కణుపు* స్థిరరాశి m_logicp;  // The logic node this vertex represents
    RegionFlags స్థిరరాశి m_assignedRegion;  // The region this logic is originally assigned to

బహిరంగ:
    Sched_Replicate_Logic_శీర్షం(V3_లేఖాచిత్రం* graphp, AstScope* scopep, Ast_Sen_వృక్షం* senTreep,
                              Ast_కణుపు* logicp, RegionFlags assignedRegion)
        : Sched_Replicate_శీర్షం{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        కూడు_Driving_Regions(assignedRegion);
    }
    AstScope* scopep() స్థిరరాశి { తిరిగి m_scopep; }
    Ast_Sen_వృక్షం* senTreep() స్థిరరాశి { తిరిగి m_senTreep; }
    Ast_కణుపు* logicp() స్థిరరాశి { తిరిగి m_logicp; }
    RegionFlags assignedRegion() స్థిరరాశి { తిరిగి m_assignedRegion; }

    // For graph dumping
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_logicp->fileline()->ascii(); };
    string dotShape() స్థిరరాశి override { తిరిగి "rectangle"; }
};

తరగతి Sched_Replicate_Var_శీర్షం final : బహిరంగ Sched_Replicate_శీర్షం {
    VL_RTTI_IMPL(Sched_Replicate_Var_శీర్షం, Sched_Replicate_శీర్షం)
    AstVarScope* స్థిరరాశి m_vscp;  // The AstVarScope this vertex represents

బహిరంగ:
    Sched_Replicate_Var_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* vscp)
        : Sched_Replicate_శీర్షం{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        ఒకవేళ (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            కూడు_Driving_Regions(ఆదానం);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        ఒకవేళ (varp()->isWrittenBySuspendable()) కూడు_Driving_Regions(చురుకైన_2);
    }
    AstVarScope* vscp() స్థిరరాశి { తిరిగి m_vscp; }
    AstVar* varp() స్థిరరాశి { తిరిగి m_vscp->varp(); }
    AstScope* scopep() స్థిరరాశి { తిరిగి m_vscp->scopep(); }

    // For graph dumping
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_vscp->name(); }
    string dotShape() స్థిరరాశి override { తిరిగి varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

తరగతి లేఖాచిత్రం_2 final : బహిరంగ V3_లేఖాచిత్రం {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<లేఖాచిత్రం_2> build_లేఖాచిత్రం(స్థిరరాశి LogicRegions& logicRegions) {
    std::unique_ptr<లేఖాచిత్రం_2> graphp{కొత్త లేఖాచిత్రం_2};

    // AstVarScope::user1() -> VarVertx
    స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
    స్థిరరాశి స్వయంచాలక get_Var_శీర్షం = [&](AstVarScope* vscp) {
        ఒకవేళ (!vscp->user1p()) vscp->user1p(కొత్త Sched_Replicate_Var_శీర్షం{graphp.get(), vscp});
        తిరిగి vscp->user1u().గమ్యం<Sched_Replicate_Var_శీర్షం*>();
    };

    స్థిరరాశి స్వయంచాలక కూడు_అంచు = [&](Sched_Replicate_శీర్షం* fromp, Sched_Replicate_శీర్షం* శిఖరం) {
        కొత్త V3_లేఖాచిత్రం_అంచు{graphp.get(), fromp, శిఖరం, 1};
    };

    స్థిరరాశి స్వయంచాలక కూడు_Logic = [&](RegionFlags region, AstScope* scopep, Ast_చురుకైన* activep) {
        Ast_Sen_వృక్షం* స్థిరరాశి senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<తార్కిక(AstVarScope*)> readTriggersThisLogic;

        స్థిరరాశి VNUser4_లోపల_Use user4_లోపల_Use;  // bool: Explicit sensitivity of hybrid logic just below

        ఒకవేళ (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { తిరిగి అబద్ధం; };
        } లేకపోతే ఒకవేళ (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { తిరిగి నిజం; };
        } లేకపోతే {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { తిరిగి !vscp->user4(); };
            senTreep->foreach([](స్థిరరాశి AstVarRef* refp) {  //
                refp->varScopep()->user4(నిజం);
            });
        }

        కొరకు (Ast_కణుపు* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            Sched_Replicate_Logic_శీర్షం* స్థిరరాశి lvtxp
                = కొత్త Sched_Replicate_Logic_శీర్షం{graphp.get(), scopep, senTreep, nodep, region};
            స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
            స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;

            V3Sched::util::Var_Scope_సమితి force_Read_అంచు_Ignores;
            V3Sched::util::collect_Force_Read_అంచు_Ignores(nodep, force_Read_అంచు_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* స్థిరరాశి vscp = refp->varScopep();
                Sched_Replicate_Var_శీర్షం* స్థిరరాశి vvtxp = get_Var_శీర్షం(vscp);
                ఒకవేళ (refp->access().isReadOrRW() && !vscp->user3_సమితి_Once()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !force_Read_అంచు_Ignores.లెక్క(vscp)) {  //
                    కూడు_అంచు(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                ఒకవేళ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_సమితి_Once() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    కూడు_అంచు(lvtxp, vvtxp);
                }
            });
        }
    };

    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : logicRegions.m_pre) కూడు_Logic(చురుకైన_2, నమోదు_2.m_scopep, నమోదు_2.m_activep);
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : logicRegions.m_act) కూడు_Logic(చురుకైన_2, నమోదు_2.m_scopep, నమోదు_2.m_activep);
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : logicRegions.m_nba) కూడు_Logic(NBA, నమోదు_2.m_scopep, నమోదు_2.m_activep);
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : logicRegions.m_obs)
        కూడు_Logic(OBSERVED, నమోదు_2.m_scopep, నమోదు_2.m_activep);
    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : logicRegions.m_react)
        కూడు_Logic(REACTIVE, నమోదు_2.m_scopep, నమోదు_2.m_activep);

    తిరిగి graphp;
}

శూన్య propagateDrivingRegions(Sched_Replicate_శీర్షం* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    ఒకవేళ (vtxp->user()) తిరిగి;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->లోపల_అంచులు()) {
        Sched_Replicate_శీర్షం* స్థిరరాశి srcp = అంచు.fromp()->as<Sched_Replicate_శీర్షం>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->కూడు_Driving_Regions(drivingRegions);

    // Mark as visited
    vtxp->user(నిజం);
}

LogicReplicas replicate(లేఖాచిత్రం_2* graphp) {
    LogicReplicas ఫలితం;
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
        ఒకవేళ (Sched_Replicate_Logic_శీర్షం* స్థిరరాశి lvtxp = vtx.cast<Sched_Replicate_Logic_శీర్షం>()) {
            స్థిరరాశి స్వయంచాలక replicate_గమ్యం = [&](LogicByScope& lbs) {
                lbs.కూడు(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_వృక్షం(అబద్ధం));
            };
            స్థిరరాశి uint8_t లక్ష్యం_Regions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || లక్ష్యం_Regions == 0,
                    "replicating clocked logic");
            ఒకవేళ (లక్ష్యం_Regions & ఆదానం) replicate_గమ్యం(ఫలితం.m_ico);
            ఒకవేళ (లక్ష్యం_Regions & చురుకైన_2) replicate_గమ్యం(ఫలితం.m_act);
            ఒకవేళ (లక్ష్యం_Regions & NBA) replicate_గమ్యం(ఫలితం.m_nba);
            ఒకవేళ (లక్ష్యం_Regions & OBSERVED) replicate_గమ్యం(ఫలితం.m_obs);
            ఒకవేళ (లక్ష్యం_Regions & REACTIVE) replicate_గమ్యం(ఫలితం.m_react);
        }
    }
    తిరిగి ఫలితం;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    స్థిరరాశి std::unique_ptr<లేఖాచిత్రం_2> graphp = build_లేఖాచిత్రం(logicRegionsRegions);
    // Dump for debug
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<Sched_Replicate_శీర్షం>());
    }
    // Dump for debug
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    తిరిగి replicate(graphp.get());
}

}  // namespace V3Sched
