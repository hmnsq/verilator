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
    داخلا = 0x1,  // Variable/logic is driven from top level input
    سرگرم_2 = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

صنف Sched_Replicate_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Replicate_راس, V3_گراف_راس)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

عام:
    واضح Sched_Replicate_راس(V3_گراف* graphp)
        : V3_گراف_راس{graphp} {}
    uint8_t drivingRegions() ثابت { أرجع m_drivingRegions; }
    فراغ جوڙيو_Driving_Regions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ثابت override {
        بدل (static_cast<بنانشان>(m_drivingRegions)) {
        حالة NONE: أرجع "black";
        حالة داخلا: أرجع "red";
        حالة سرگرم_2: أرجع "green";
        حالة NBA: أرجع "blue";
        حالة داخلا | سرگرم_2: أرجع "yellow";
        حالة داخلا | NBA: أرجع "magenta";
        حالة سرگرم_2 | NBA: أرجع "cyan";
        حالة داخلا | سرگرم_2 | NBA: أرجع "gray80";  // don't want white on white background

        حالة REACTIVE: أرجع "gray60";
        حالة REACTIVE | داخلا: أرجع "lightcoral";
        حالة REACTIVE | سرگرم_2: أرجع "lightgreen";
        حالة REACTIVE | NBA: أرجع "lightblue";
        حالة REACTIVE | داخلا | سرگرم_2: أرجع "lightyellow";
        حالة REACTIVE | داخلا | NBA: أرجع "lightpink";
        حالة REACTIVE | سرگرم_2 | NBA: أرجع "lightcyan";
        حالة REACTIVE | داخلا | سرگرم_2 | NBA: أرجع "gray90";

        حالة OBSERVED: أرجع "gray20";
        حالة OBSERVED | داخلا: أرجع "darkred";
        حالة OBSERVED | سرگرم_2: أرجع "darkgreen";
        حالة OBSERVED | NBA: أرجع "darkblue";
        حالة OBSERVED | داخلا | سرگرم_2: أرجع "gold";
        حالة OBSERVED | داخلا | NBA: أرجع "purple";
        حالة OBSERVED | سرگرم_2 | NBA: أرجع "darkcyan";
        حالة OBSERVED | داخلا | سرگرم_2 | NBA: أرجع "gray30";

        حالة REACTIVE | OBSERVED: أرجع "gray40";
        حالة REACTIVE | OBSERVED | داخلا: أرجع "indianred";
        حالة REACTIVE | OBSERVED | سرگرم_2: أرجع "olive";
        حالة REACTIVE | OBSERVED | NBA: أرجع "dodgerBlue";
        حالة REACTIVE | OBSERVED | داخلا | سرگرم_2: أرجع "khaki";
        حالة REACTIVE | OBSERVED | داخلا | NBA: أرجع "plum";
        حالة REACTIVE | OBSERVED | سرگرم_2 | NBA: أرجع "lightSeaGreen";
        حالة REACTIVE | OBSERVED | داخلا | سرگرم_2 | NBA: أرجع "gray50";
        افتراضي: v3fatalSrc("There are only 5 region bits"); أرجع "";
        }
    }
    // LCOV_EXCL_STOP
};

صنف Sched_Replicate_Logic_راس final : عام Sched_Replicate_راس {
    VL_RTTI_IMPL(Sched_Replicate_Logic_راس, Sched_Replicate_راس)
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_وڻ* ثابت m_senTreep;  // The sensitivity of the logic node
    Ast_ڳنڍ* ثابت m_logicp;  // The logic node this vertex represents
    RegionFlags ثابت m_assignedRegion;  // The region this logic is originally assigned to

عام:
    Sched_Replicate_Logic_راس(V3_گراف* graphp, AstScope* scopep, Ast_Sen_وڻ* senTreep,
                              Ast_ڳنڍ* logicp, RegionFlags assignedRegion)
        : Sched_Replicate_راس{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        جوڙيو_Driving_Regions(assignedRegion);
    }
    AstScope* scopep() ثابت { أرجع m_scopep; }
    Ast_Sen_وڻ* senTreep() ثابت { أرجع m_senTreep; }
    Ast_ڳنڍ* logicp() ثابت { أرجع m_logicp; }
    RegionFlags assignedRegion() ثابت { أرجع m_assignedRegion; }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { أرجع m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { أرجع "rectangle"; }
};

صنف Sched_Replicate_Var_راس final : عام Sched_Replicate_راس {
    VL_RTTI_IMPL(Sched_Replicate_Var_راس, Sched_Replicate_راس)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عام:
    Sched_Replicate_Var_راس(V3_گراف* graphp, AstVarScope* vscp)
        : Sched_Replicate_راس{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        إذا (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            جوڙيو_Driving_Regions(داخلا);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        إذا (varp()->isWrittenBySuspendable()) جوڙيو_Driving_Regions(سرگرم_2);
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
    ثابت VNUser1_اندريون_Use user1_اندريون_Use;
    ثابت خودڪار get_Var_راس = [&](AstVarScope* vscp) {
        إذا (!vscp->user1p()) vscp->user1p(جديد Sched_Replicate_Var_راس{graphp.get(), vscp});
        أرجع vscp->user1u().منزل<Sched_Replicate_Var_راس*>();
    };

    ثابت خودڪار جوڙيو_ڪنارو = [&](Sched_Replicate_راس* fromp, Sched_Replicate_راس* چوٽي) {
        جديد V3_گراف_ڪنارو{graphp.get(), fromp, چوٽي, 1};
    };

    ثابت خودڪار جوڙيو_Logic = [&](RegionFlags region, AstScope* scopep, Ast_سرگرم* activep) {
        Ast_Sen_وڻ* ثابت senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<منطقي(AstVarScope*)> readTriggersThisLogic;

        ثابت VNUser4_اندريون_Use user4_اندريون_Use;  // bool: Explicit sensitivity of hybrid logic just below

        إذا (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { أرجع خطأ; };
        } وإلا إذا (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { أرجع سچ; };
        } وإلا {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { أرجع !vscp->user4(); };
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user4(سچ);
            });
        }

        لكل (Ast_ڳنڍ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            Sched_Replicate_Logic_راس* ثابت lvtxp
                = جديد Sched_Replicate_Logic_راس{graphp.get(), scopep, senTreep, nodep, region};
            ثابت VNUser2_اندريون_Use user2_اندريون_Use;
            ثابت VNUser3_اندريون_Use user3_اندريون_Use;

            V3Sched::util::Var_Scope_سيٽ force_Read_ڪنارو_Ignores;
            V3Sched::util::collect_Force_Read_ڪنارو_Ignores(nodep, force_Read_ڪنارو_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                Sched_Replicate_Var_راس* ثابت vvtxp = get_Var_راس(vscp);
                إذا (refp->access().isReadOrRW() && !vscp->user3_سيٽ_Once()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !force_Read_ڪنارو_Ignores.ڳڻپ(vscp)) {  //
                    جوڙيو_ڪنارو(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                إذا (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_سيٽ_Once() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    جوڙيو_ڪنارو(lvtxp, vvtxp);
                }
            });
        }
    };

    لكل (ثابت خودڪار& داخلا_2 : logicRegions.m_pre) جوڙيو_Logic(سرگرم_2, داخلا_2.m_scopep, داخلا_2.m_activep);
    لكل (ثابت خودڪار& داخلا_2 : logicRegions.m_act) جوڙيو_Logic(سرگرم_2, داخلا_2.m_scopep, داخلا_2.m_activep);
    لكل (ثابت خودڪار& داخلا_2 : logicRegions.m_nba) جوڙيو_Logic(NBA, داخلا_2.m_scopep, داخلا_2.m_activep);
    لكل (ثابت خودڪار& داخلا_2 : logicRegions.m_obs)
        جوڙيو_Logic(OBSERVED, داخلا_2.m_scopep, داخلا_2.m_activep);
    لكل (ثابت خودڪار& داخلا_2 : logicRegions.m_react)
        جوڙيو_Logic(REACTIVE, داخلا_2.m_scopep, داخلا_2.m_activep);

    أرجع graphp;
}

فراغ propagateDrivingRegions(Sched_Replicate_راس* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    إذا (vtxp->user()) أرجع;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->اندريون_ڪنارا()) {
        Sched_Replicate_راس* ثابت srcp = ڪنارو.fromp()->as<Sched_Replicate_راس>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->جوڙيو_Driving_Regions(drivingRegions);

    // Mark as visited
    vtxp->user(سچ);
}

LogicReplicas replicate(گراف_2* graphp) {
    LogicReplicas نتيجو;
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        إذا (Sched_Replicate_Logic_راس* ثابت lvtxp = vtx.cast<Sched_Replicate_Logic_راس>()) {
            ثابت خودڪار replicate_منزل = [&](LogicByScope& lbs) {
                lbs.جوڙيو(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_وڻ(خطأ));
            };
            ثابت uint8_t هدف_Regions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || هدف_Regions == 0,
                    "replicating clocked logic");
            إذا (هدف_Regions & داخلا) replicate_منزل(نتيجو.m_ico);
            إذا (هدف_Regions & سرگرم_2) replicate_منزل(نتيجو.m_act);
            إذا (هدف_Regions & NBA) replicate_منزل(نتيجو.m_nba);
            إذا (هدف_Regions & OBSERVED) replicate_منزل(نتيجو.m_obs);
            إذا (هدف_Regions & REACTIVE) replicate_منزل(نتيجو.m_react);
        }
    }
    أرجع نتيجو;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<گراف_2> graphp = build_گراف(logicRegionsRegions);
    // Dump for debug
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    لكل (V3_گراف_راس& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<Sched_Replicate_راس>());
    }
    // Dump for debug
    إذا (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    أرجع replicate(graphp.get());
}

}  // namespace V3Sched
