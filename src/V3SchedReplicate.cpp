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
    ننوت = 0x1,  // Variable/logic is driven from top level input
    فعال_2 = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

صنف Sched_Replicate_راس VL_NOT_FINAL : عام V3_ګراف_راس {
    VL_RTTI_IMPL(Sched_Replicate_راس, V3_ګراف_راس)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

عام:
    څرګند Sched_Replicate_راس(V3_ګراف* graphp)
        : V3_ګراف_راس{graphp} {}
    uint8_t drivingRegions() ثابت { أرجع m_drivingRegions; }
    فراغ جمع_کړه_Driving_Regions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ثابت override {
        بدل (static_cast<بینښه>(m_drivingRegions)) {
        حالة NONE: أرجع "black";
        حالة ننوت: أرجع "red";
        حالة فعال_2: أرجع "green";
        حالة NBA: أرجع "blue";
        حالة ننوت | فعال_2: أرجع "yellow";
        حالة ننوت | NBA: أرجع "magenta";
        حالة فعال_2 | NBA: أرجع "cyan";
        حالة ننوت | فعال_2 | NBA: أرجع "gray80";  // don't want white on white background

        حالة REACTIVE: أرجع "gray60";
        حالة REACTIVE | ننوت: أرجع "lightcoral";
        حالة REACTIVE | فعال_2: أرجع "lightgreen";
        حالة REACTIVE | NBA: أرجع "lightblue";
        حالة REACTIVE | ننوت | فعال_2: أرجع "lightyellow";
        حالة REACTIVE | ننوت | NBA: أرجع "lightpink";
        حالة REACTIVE | فعال_2 | NBA: أرجع "lightcyan";
        حالة REACTIVE | ننوت | فعال_2 | NBA: أرجع "gray90";

        حالة OBSERVED: أرجع "gray20";
        حالة OBSERVED | ننوت: أرجع "darkred";
        حالة OBSERVED | فعال_2: أرجع "darkgreen";
        حالة OBSERVED | NBA: أرجع "darkblue";
        حالة OBSERVED | ننوت | فعال_2: أرجع "gold";
        حالة OBSERVED | ننوت | NBA: أرجع "purple";
        حالة OBSERVED | فعال_2 | NBA: أرجع "darkcyan";
        حالة OBSERVED | ننوت | فعال_2 | NBA: أرجع "gray30";

        حالة REACTIVE | OBSERVED: أرجع "gray40";
        حالة REACTIVE | OBSERVED | ننوت: أرجع "indianred";
        حالة REACTIVE | OBSERVED | فعال_2: أرجع "olive";
        حالة REACTIVE | OBSERVED | NBA: أرجع "dodgerBlue";
        حالة REACTIVE | OBSERVED | ننوت | فعال_2: أرجع "khaki";
        حالة REACTIVE | OBSERVED | ننوت | NBA: أرجع "plum";
        حالة REACTIVE | OBSERVED | فعال_2 | NBA: أرجع "lightSeaGreen";
        حالة REACTIVE | OBSERVED | ننوت | فعال_2 | NBA: أرجع "gray50";
        افتراضي: v3fatalSrc("There are only 5 region bits"); أرجع "";
        }
    }
    // LCOV_EXCL_STOP
};

صنف Sched_Replicate_Logic_راس final : عام Sched_Replicate_راس {
    VL_RTTI_IMPL(Sched_Replicate_Logic_راس, Sched_Replicate_راس)
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_ونه* ثابت m_senTreep;  // The sensitivity of the logic node
    Ast_غوټه* ثابت m_logicp;  // The logic node this vertex represents
    RegionFlags ثابت m_assignedRegion;  // The region this logic is originally assigned to

عام:
    Sched_Replicate_Logic_راس(V3_ګراف* graphp, AstScope* scopep, Ast_Sen_ونه* senTreep,
                              Ast_غوټه* logicp, RegionFlags assignedRegion)
        : Sched_Replicate_راس{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        جمع_کړه_Driving_Regions(assignedRegion);
    }
    AstScope* scopep() ثابت { أرجع m_scopep; }
    Ast_Sen_ونه* senTreep() ثابت { أرجع m_senTreep; }
    Ast_غوټه* logicp() ثابت { أرجع m_logicp; }
    RegionFlags assignedRegion() ثابت { أرجع m_assignedRegion; }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { أرجع m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { أرجع "rectangle"; }
};

صنف Sched_Replicate_Var_راس final : عام Sched_Replicate_راس {
    VL_RTTI_IMPL(Sched_Replicate_Var_راس, Sched_Replicate_راس)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عام:
    Sched_Replicate_Var_راس(V3_ګراف* graphp, AstVarScope* vscp)
        : Sched_Replicate_راس{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        إذا (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            جمع_کړه_Driving_Regions(ننوت);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        إذا (varp()->isWrittenBySuspendable()) جمع_کړه_Driving_Regions(فعال_2);
    }
    AstVarScope* vscp() ثابت { أرجع m_vscp; }
    AstVar* varp() ثابت { أرجع m_vscp->varp(); }
    AstScope* scopep() ثابت { أرجع m_vscp->scopep(); }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { أرجع m_vscp->name(); }
    string dotShape() ثابت override { أرجع varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

صنف ګراف_2 final : عام V3_ګراف {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<ګراف_2> build_ګراف(ثابت LogicRegions& logicRegions) {
    std::unique_ptr<ګراف_2> graphp{جديد ګراف_2};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1_دننه_Use user1_دننه_Use;
    ثابت خپلکاری get_Var_راس = [&](AstVarScope* vscp) {
        إذا (!vscp->user1p()) vscp->user1p(جديد Sched_Replicate_Var_راس{graphp.get(), vscp});
        أرجع vscp->user1u().منزل<Sched_Replicate_Var_راس*>();
    };

    ثابت خپلکاری جمع_کړه_څنډه = [&](Sched_Replicate_راس* fromp, Sched_Replicate_راس* سر) {
        جديد V3_ګراف_څنډه{graphp.get(), fromp, سر, 1};
    };

    ثابت خپلکاری جمع_کړه_Logic = [&](RegionFlags region, AstScope* scopep, Ast_فعال* activep) {
        Ast_Sen_ونه* ثابت senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<منطقي(AstVarScope*)> readTriggersThisLogic;

        ثابت VNUser4_دننه_Use user4_دننه_Use;  // bool: Explicit sensitivity of hybrid logic just below

        إذا (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { أرجع خطأ; };
        } وإلا إذا (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { أرجع ریښتیا; };
        } وإلا {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { أرجع !vscp->user4(); };
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user4(ریښتیا);
            });
        }

        لكل (Ast_غوټه* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            Sched_Replicate_Logic_راس* ثابت lvtxp
                = جديد Sched_Replicate_Logic_راس{graphp.get(), scopep, senTreep, nodep, region};
            ثابت VNUser2_دننه_Use user2_دننه_Use;
            ثابت VNUser3_دننه_Use user3_دننه_Use;

            V3Sched::util::Var_Scope_سېټ force_Read_څنډه_Ignores;
            V3Sched::util::collect_Force_Read_څنډه_Ignores(nodep, force_Read_څنډه_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                Sched_Replicate_Var_راس* ثابت vvtxp = get_Var_راس(vscp);
                إذا (refp->access().isReadOrRW() && !vscp->user3_سېټ_Once()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !force_Read_څنډه_Ignores.شمېر(vscp)) {  //
                    جمع_کړه_څنډه(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                إذا (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_سېټ_Once() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    جمع_کړه_څنډه(lvtxp, vvtxp);
                }
            });
        }
    };

    لكل (ثابت خپلکاری& ثبت : logicRegions.m_pre) جمع_کړه_Logic(فعال_2, ثبت.m_scopep, ثبت.m_activep);
    لكل (ثابت خپلکاری& ثبت : logicRegions.m_act) جمع_کړه_Logic(فعال_2, ثبت.m_scopep, ثبت.m_activep);
    لكل (ثابت خپلکاری& ثبت : logicRegions.m_nba) جمع_کړه_Logic(NBA, ثبت.m_scopep, ثبت.m_activep);
    لكل (ثابت خپلکاری& ثبت : logicRegions.m_obs)
        جمع_کړه_Logic(OBSERVED, ثبت.m_scopep, ثبت.m_activep);
    لكل (ثابت خپلکاری& ثبت : logicRegions.m_react)
        جمع_کړه_Logic(REACTIVE, ثبت.m_scopep, ثبت.m_activep);

    أرجع graphp;
}

فراغ propagateDrivingRegions(Sched_Replicate_راس* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    إذا (vtxp->user()) أرجع;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    لكل (V3_ګراف_څنډه& څنډه : vtxp->دننه_څنډې()) {
        Sched_Replicate_راس* ثابت srcp = څنډه.fromp()->as<Sched_Replicate_راس>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->جمع_کړه_Driving_Regions(drivingRegions);

    // Mark as visited
    vtxp->user(ریښتیا);
}

LogicReplicas replicate(ګراف_2* graphp) {
    LogicReplicas پایله;
    لكل (V3_ګراف_راس& vtx : graphp->vertices()) {
        إذا (Sched_Replicate_Logic_راس* ثابت lvtxp = vtx.cast<Sched_Replicate_Logic_راس>()) {
            ثابت خپلکاری replicate_منزل = [&](LogicByScope& lbs) {
                lbs.جمع_کړه(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_ونه(خطأ));
            };
            ثابت uint8_t موخه_Regions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || موخه_Regions == 0,
                    "replicating clocked logic");
            إذا (موخه_Regions & ننوت) replicate_منزل(پایله.m_ico);
            إذا (موخه_Regions & فعال_2) replicate_منزل(پایله.m_act);
            إذا (موخه_Regions & NBA) replicate_منزل(پایله.m_nba);
            إذا (موخه_Regions & OBSERVED) replicate_منزل(پایله.m_obs);
            إذا (موخه_Regions & REACTIVE) replicate_منزل(پایله.m_react);
        }
    }
    أرجع پایله;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<ګراف_2> graphp = build_ګراف(logicRegionsRegions);
    // Dump for debug
    إذا (dump_ګراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    لكل (V3_ګراف_راس& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<Sched_Replicate_راس>());
    }
    // Dump for debug
    إذا (dump_ګراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    أرجع replicate(graphp.get());
}

}  // namespace V3Sched
