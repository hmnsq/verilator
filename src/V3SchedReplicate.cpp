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
    درون_داد = 0x1,  // Variable/logic is driven from top level input
    فعال_2 = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

صنف Sched_Replicate_رأس VL_NOT_FINAL : عام V3_گراف_رأس {
    VL_RTTI_IMPL(Sched_Replicate_رأس, V3_گراف_رأس)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

عام:
    صریح Sched_Replicate_رأس(V3_گراف* graphp)
        : V3_گراف_رأس{graphp} {}
    uint8_t drivingRegions() ثابت { أرجع m_drivingRegions; }
    فراغ جمع_کن_Driving_Regions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ثابت override {
        بدل (static_cast<بیعلامت>(m_drivingRegions)) {
        حالة NONE: أرجع "black";
        حالة درون_داد: أرجع "red";
        حالة فعال_2: أرجع "green";
        حالة NBA: أرجع "blue";
        حالة درون_داد | فعال_2: أرجع "yellow";
        حالة درون_داد | NBA: أرجع "magenta";
        حالة فعال_2 | NBA: أرجع "cyan";
        حالة درون_داد | فعال_2 | NBA: أرجع "gray80";  // don't want white on white background

        حالة REACTIVE: أرجع "gray60";
        حالة REACTIVE | درون_داد: أرجع "lightcoral";
        حالة REACTIVE | فعال_2: أرجع "lightgreen";
        حالة REACTIVE | NBA: أرجع "lightblue";
        حالة REACTIVE | درون_داد | فعال_2: أرجع "lightyellow";
        حالة REACTIVE | درون_داد | NBA: أرجع "lightpink";
        حالة REACTIVE | فعال_2 | NBA: أرجع "lightcyan";
        حالة REACTIVE | درون_داد | فعال_2 | NBA: أرجع "gray90";

        حالة OBSERVED: أرجع "gray20";
        حالة OBSERVED | درون_داد: أرجع "darkred";
        حالة OBSERVED | فعال_2: أرجع "darkgreen";
        حالة OBSERVED | NBA: أرجع "darkblue";
        حالة OBSERVED | درون_داد | فعال_2: أرجع "gold";
        حالة OBSERVED | درون_داد | NBA: أرجع "purple";
        حالة OBSERVED | فعال_2 | NBA: أرجع "darkcyan";
        حالة OBSERVED | درون_داد | فعال_2 | NBA: أرجع "gray30";

        حالة REACTIVE | OBSERVED: أرجع "gray40";
        حالة REACTIVE | OBSERVED | درون_داد: أرجع "indianred";
        حالة REACTIVE | OBSERVED | فعال_2: أرجع "olive";
        حالة REACTIVE | OBSERVED | NBA: أرجع "dodgerBlue";
        حالة REACTIVE | OBSERVED | درون_داد | فعال_2: أرجع "khaki";
        حالة REACTIVE | OBSERVED | درون_داد | NBA: أرجع "plum";
        حالة REACTIVE | OBSERVED | فعال_2 | NBA: أرجع "lightSeaGreen";
        حالة REACTIVE | OBSERVED | درون_داد | فعال_2 | NBA: أرجع "gray50";
        افتراضي: v3fatalSrc("There are only 5 region bits"); أرجع "";
        }
    }
    // LCOV_EXCL_STOP
};

صنف Sched_Replicate_Logic_رأس final : عام Sched_Replicate_رأس {
    VL_RTTI_IMPL(Sched_Replicate_Logic_رأس, Sched_Replicate_رأس)
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_درخت* ثابت m_senTreep;  // The sensitivity of the logic node
    Ast_گره* ثابت m_logicp;  // The logic node this vertex represents
    RegionFlags ثابت m_assignedRegion;  // The region this logic is originally assigned to

عام:
    Sched_Replicate_Logic_رأس(V3_گراف* graphp, AstScope* scopep, Ast_Sen_درخت* senTreep,
                              Ast_گره* logicp, RegionFlags assignedRegion)
        : Sched_Replicate_رأس{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        جمع_کن_Driving_Regions(assignedRegion);
    }
    AstScope* scopep() ثابت { أرجع m_scopep; }
    Ast_Sen_درخت* senTreep() ثابت { أرجع m_senTreep; }
    Ast_گره* logicp() ثابت { أرجع m_logicp; }
    RegionFlags assignedRegion() ثابت { أرجع m_assignedRegion; }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { أرجع m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { أرجع "rectangle"; }
};

صنف Sched_Replicate_Var_رأس final : عام Sched_Replicate_رأس {
    VL_RTTI_IMPL(Sched_Replicate_Var_رأس, Sched_Replicate_رأس)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عام:
    Sched_Replicate_Var_رأس(V3_گراف* graphp, AstVarScope* vscp)
        : Sched_Replicate_رأس{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        إذا (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            جمع_کن_Driving_Regions(درون_داد);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        إذا (varp()->isWrittenBySuspendable()) جمع_کن_Driving_Regions(فعال_2);
    }
    AstVarScope* vscp() ثابت { أرجع m_vscp; }
    AstVar* varp() ثابت { أرجع m_vscp->varp(); }
    AstScope* scopep() ثابت { أرجع m_vscp->scopep(); }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { أرجع m_vscp->name(); }
    string dotShape() ثابت override { أرجع varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

صنف گراف_2 final : عام V3_گراف {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<گراف_2> build_گراف(ثابت LogicRegions& logicRegions) {
    std::unique_ptr<گراف_2> graphp{جديد گراف_2};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1_درون_Use user1_درون_Use;
    ثابت خودکار get_Var_رأس = [&](AstVarScope* vscp) {
        إذا (!vscp->user1p()) vscp->user1p(جديد Sched_Replicate_Var_رأس{graphp.get(), vscp});
        أرجع vscp->user1u().مقصد<Sched_Replicate_Var_رأس*>();
    };

    ثابت خودکار جمع_کن_یال = [&](Sched_Replicate_رأس* fromp, Sched_Replicate_رأس* بالاترین) {
        جديد V3_گراف_یال{graphp.get(), fromp, بالاترین, 1};
    };

    ثابت خودکار جمع_کن_Logic = [&](RegionFlags region, AstScope* scopep, Ast_فعال* activep) {
        Ast_Sen_درخت* ثابت senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<منطقي(AstVarScope*)> readTriggersThisLogic;

        ثابت VNUser4_درون_Use user4_درون_Use;  // bool: Explicit sensitivity of hybrid logic just below

        إذا (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { أرجع خطأ; };
        } وإلا إذا (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { أرجع درست; };
        } وإلا {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { أرجع !vscp->user4(); };
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user4(درست);
            });
        }

        لكل (Ast_گره* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            Sched_Replicate_Logic_رأس* ثابت lvtxp
                = جديد Sched_Replicate_Logic_رأس{graphp.get(), scopep, senTreep, nodep, region};
            ثابت VNUser2_درون_Use user2_درون_Use;
            ثابت VNUser3_درون_Use user3_درون_Use;

            V3Sched::util::Var_Scope_مجموعه force_Read_یال_Ignores;
            V3Sched::util::collect_Force_Read_یال_Ignores(nodep, force_Read_یال_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                Sched_Replicate_Var_رأس* ثابت vvtxp = get_Var_رأس(vscp);
                إذا (refp->access().isReadOrRW() && !vscp->user3_مجموعه_Once()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !force_Read_یال_Ignores.شمار(vscp)) {  //
                    جمع_کن_یال(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                إذا (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_مجموعه_Once() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    جمع_کن_یال(lvtxp, vvtxp);
                }
            });
        }
    };

    لكل (ثابت خودکار& مدخل : logicRegions.m_pre) جمع_کن_Logic(فعال_2, مدخل.m_scopep, مدخل.m_activep);
    لكل (ثابت خودکار& مدخل : logicRegions.m_act) جمع_کن_Logic(فعال_2, مدخل.m_scopep, مدخل.m_activep);
    لكل (ثابت خودکار& مدخل : logicRegions.m_nba) جمع_کن_Logic(NBA, مدخل.m_scopep, مدخل.m_activep);
    لكل (ثابت خودکار& مدخل : logicRegions.m_obs)
        جمع_کن_Logic(OBSERVED, مدخل.m_scopep, مدخل.m_activep);
    لكل (ثابت خودکار& مدخل : logicRegions.m_react)
        جمع_کن_Logic(REACTIVE, مدخل.m_scopep, مدخل.m_activep);

    أرجع graphp;
}

فراغ propagateDrivingRegions(Sched_Replicate_رأس* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    إذا (vtxp->user()) أرجع;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    لكل (V3_گراف_یال& یال : vtxp->درون_یالها()) {
        Sched_Replicate_رأس* ثابت srcp = یال.fromp()->as<Sched_Replicate_رأس>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->جمع_کن_Driving_Regions(drivingRegions);

    // Mark as visited
    vtxp->user(درست);
}

LogicReplicas replicate(گراف_2* graphp) {
    LogicReplicas نتیجه;
    لكل (V3_گراف_رأس& vtx : graphp->vertices()) {
        إذا (Sched_Replicate_Logic_رأس* ثابت lvtxp = vtx.cast<Sched_Replicate_Logic_رأس>()) {
            ثابت خودکار replicate_مقصد = [&](LogicByScope& lbs) {
                lbs.جمع_کن(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_درخت(خطأ));
            };
            ثابت uint8_t هدف_Regions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || هدف_Regions == 0,
                    "replicating clocked logic");
            إذا (هدف_Regions & درون_داد) replicate_مقصد(نتیجه.m_ico);
            إذا (هدف_Regions & فعال_2) replicate_مقصد(نتیجه.m_act);
            إذا (هدف_Regions & NBA) replicate_مقصد(نتیجه.m_nba);
            إذا (هدف_Regions & OBSERVED) replicate_مقصد(نتیجه.m_obs);
            إذا (هدف_Regions & REACTIVE) replicate_مقصد(نتیجه.m_react);
        }
    }
    أرجع نتیجه;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<گراف_2> graphp = build_گراف(logicRegionsRegions);
    // Dump for debug
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    لكل (V3_گراف_رأس& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<Sched_Replicate_رأس>());
    }
    // Dump for debug
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    أرجع replicate(graphp.get());
}

}  // namespace V3Sched
