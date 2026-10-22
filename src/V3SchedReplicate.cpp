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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Graph.h"
#شامل "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

نامخلا V3Sched {

نامخلا {

// Driving region flags
شمار RegionFlags : uint8_t {
    NONE = 0x0,  //
    داخلہ = 0x1,  // Variable/logic is driven from top level input
    فعال_2 = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

جماعت Sched_Replicate_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(Sched_Replicate_راس, V3_گراف_راس)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

عوامی:
    صریح Sched_Replicate_راس(V3_گراف* graphp)
        : V3_گراف_راس{graphp} {}
    uint8_t drivingRegions() ثابت { واپس m_drivingRegions; }
    خالی جوڑو_Driving_Regions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ثابت override {
        انتخاب (static_cast<غیرعلامتی>(m_drivingRegions)) {
        حالت NONE: واپس "black";
        حالت داخلہ: واپس "red";
        حالت فعال_2: واپس "green";
        حالت NBA: واپس "blue";
        حالت داخلہ | فعال_2: واپس "yellow";
        حالت داخلہ | NBA: واپس "magenta";
        حالت فعال_2 | NBA: واپس "cyan";
        حالت داخلہ | فعال_2 | NBA: واپس "gray80";  // don't want white on white background

        حالت REACTIVE: واپس "gray60";
        حالت REACTIVE | داخلہ: واپس "lightcoral";
        حالت REACTIVE | فعال_2: واپس "lightgreen";
        حالت REACTIVE | NBA: واپس "lightblue";
        حالت REACTIVE | داخلہ | فعال_2: واپس "lightyellow";
        حالت REACTIVE | داخلہ | NBA: واپس "lightpink";
        حالت REACTIVE | فعال_2 | NBA: واپس "lightcyan";
        حالت REACTIVE | داخلہ | فعال_2 | NBA: واپس "gray90";

        حالت OBSERVED: واپس "gray20";
        حالت OBSERVED | داخلہ: واپس "darkred";
        حالت OBSERVED | فعال_2: واپس "darkgreen";
        حالت OBSERVED | NBA: واپس "darkblue";
        حالت OBSERVED | داخلہ | فعال_2: واپس "gold";
        حالت OBSERVED | داخلہ | NBA: واپس "purple";
        حالت OBSERVED | فعال_2 | NBA: واپس "darkcyan";
        حالت OBSERVED | داخلہ | فعال_2 | NBA: واپس "gray30";

        حالت REACTIVE | OBSERVED: واپس "gray40";
        حالت REACTIVE | OBSERVED | داخلہ: واپس "indianred";
        حالت REACTIVE | OBSERVED | فعال_2: واپس "olive";
        حالت REACTIVE | OBSERVED | NBA: واپس "dodgerBlue";
        حالت REACTIVE | OBSERVED | داخلہ | فعال_2: واپس "khaki";
        حالت REACTIVE | OBSERVED | داخلہ | NBA: واپس "plum";
        حالت REACTIVE | OBSERVED | فعال_2 | NBA: واپس "lightSeaGreen";
        حالت REACTIVE | OBSERVED | داخلہ | فعال_2 | NBA: واپس "gray50";
        طےشدہ: v3fatalSrc("There are only 5 region bits"); واپس "";
        }
    }
    // LCOV_EXCL_STOP
};

جماعت Sched_Replicate_Logic_راس final : عوامی Sched_Replicate_راس {
    VL_RTTI_IMPL(Sched_Replicate_Logic_راس, Sched_Replicate_راس)
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_درخت* ثابت m_senTreep;  // The sensitivity of the logic node
    Ast_گرہ* ثابت m_logicp;  // The logic node this vertex represents
    RegionFlags ثابت m_assignedRegion;  // The region this logic is originally assigned to

عوامی:
    Sched_Replicate_Logic_راس(V3_گراف* graphp, AstScope* scopep, Ast_Sen_درخت* senTreep,
                              Ast_گرہ* logicp, RegionFlags assignedRegion)
        : Sched_Replicate_راس{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        جوڑو_Driving_Regions(assignedRegion);
    }
    AstScope* scopep() ثابت { واپس m_scopep; }
    Ast_Sen_درخت* senTreep() ثابت { واپس m_senTreep; }
    Ast_گرہ* logicp() ثابت { واپس m_logicp; }
    RegionFlags assignedRegion() ثابت { واپس m_assignedRegion; }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { واپس m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { واپس "rectangle"; }
};

جماعت Sched_Replicate_Var_راس final : عوامی Sched_Replicate_راس {
    VL_RTTI_IMPL(Sched_Replicate_Var_راس, Sched_Replicate_راس)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عوامی:
    Sched_Replicate_Var_راس(V3_گراف* graphp, AstVarScope* vscp)
        : Sched_Replicate_راس{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        اگر (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            جوڑو_Driving_Regions(داخلہ);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        اگر (varp()->isWrittenBySuspendable()) جوڑو_Driving_Regions(فعال_2);
    }
    AstVarScope* vscp() ثابت { واپس m_vscp; }
    AstVar* varp() ثابت { واپس m_vscp->varp(); }
    AstScope* scopep() ثابت { واپس m_vscp->scopep(); }

    // For graph dumping
    string name() ثابت override VL_MT_STABLE { واپس m_vscp->name(); }
    string dotShape() ثابت override { واپس varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

جماعت گراف_2 final : عوامی V3_گراف {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<گراف_2> build_گراف(ثابت LogicRegions& logicRegions) {
    std::unique_ptr<گراف_2> graphp{نیا گراف_2};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1_اندر_Use user1_اندر_Use;
    ثابت خودکار get_Var_راس = [&](AstVarScope* vscp) {
        اگر (!vscp->user1p()) vscp->user1p(نیا Sched_Replicate_Var_راس{graphp.get(), vscp});
        واپس vscp->user1u().مقصد<Sched_Replicate_Var_راس*>();
    };

    ثابت خودکار جوڑو_کنارہ = [&](Sched_Replicate_راس* fromp, Sched_Replicate_راس* چوٹی) {
        نیا V3_گراف_کنارہ{graphp.get(), fromp, چوٹی, 1};
    };

    ثابت خودکار جوڑو_Logic = [&](RegionFlags region, AstScope* scopep, Ast_فعال* activep) {
        Ast_Sen_درخت* ثابت senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<منطقی(AstVarScope*)> readTriggersThisLogic;

        ثابت VNUser4_اندر_Use user4_اندر_Use;  // bool: Explicit sensitivity of hybrid logic just below

        اگر (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { واپس جھوٹ; };
        } ورنہ اگر (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { واپس سچ; };
        } ورنہ {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { واپس !vscp->user4(); };
            senTreep->foreach([](ثابت AstVarRef* refp) {  //
                refp->varScopep()->user4(سچ);
            });
        }

        برائے (Ast_گرہ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            Sched_Replicate_Logic_راس* ثابت lvtxp
                = نیا Sched_Replicate_Logic_راس{graphp.get(), scopep, senTreep, nodep, region};
            ثابت VNUser2_اندر_Use user2_اندر_Use;
            ثابت VNUser3_اندر_Use user3_اندر_Use;

            V3Sched::util::Var_Scope_سیٹ force_Read_کنارہ_Ignores;
            V3Sched::util::collect_Force_Read_کنارہ_Ignores(nodep, force_Read_کنارہ_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                Sched_Replicate_Var_راس* ثابت vvtxp = get_Var_راس(vscp);
                اگر (refp->access().isReadOrRW() && !vscp->user3_سیٹ_Once()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !force_Read_کنارہ_Ignores.گنتی(vscp)) {  //
                    جوڑو_کنارہ(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                اگر (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_سیٹ_Once() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    جوڑو_کنارہ(lvtxp, vvtxp);
                }
            });
        }
    };

    برائے (ثابت خودکار& اندراج_2 : logicRegions.m_pre) جوڑو_Logic(فعال_2, اندراج_2.m_scopep, اندراج_2.m_activep);
    برائے (ثابت خودکار& اندراج_2 : logicRegions.m_act) جوڑو_Logic(فعال_2, اندراج_2.m_scopep, اندراج_2.m_activep);
    برائے (ثابت خودکار& اندراج_2 : logicRegions.m_nba) جوڑو_Logic(NBA, اندراج_2.m_scopep, اندراج_2.m_activep);
    برائے (ثابت خودکار& اندراج_2 : logicRegions.m_obs)
        جوڑو_Logic(OBSERVED, اندراج_2.m_scopep, اندراج_2.m_activep);
    برائے (ثابت خودکار& اندراج_2 : logicRegions.m_react)
        جوڑو_Logic(REACTIVE, اندراج_2.m_scopep, اندراج_2.m_activep);

    واپس graphp;
}

خالی propagateDrivingRegions(Sched_Replicate_راس* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    اگر (vtxp->user()) واپس;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    برائے (V3_گراف_کنارہ& کنارہ : vtxp->اندر_کنارے()) {
        Sched_Replicate_راس* ثابت srcp = کنارہ.fromp()->as<Sched_Replicate_راس>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->جوڑو_Driving_Regions(drivingRegions);

    // Mark as visited
    vtxp->user(سچ);
}

LogicReplicas replicate(گراف_2* graphp) {
    LogicReplicas نتیجہ;
    برائے (V3_گراف_راس& vtx : graphp->vertices()) {
        اگر (Sched_Replicate_Logic_راس* ثابت lvtxp = vtx.cast<Sched_Replicate_Logic_راس>()) {
            ثابت خودکار replicate_مقصد = [&](LogicByScope& lbs) {
                lbs.جوڑو(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_درخت(جھوٹ));
            };
            ثابت uint8_t ہدف_Regions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || ہدف_Regions == 0,
                    "replicating clocked logic");
            اگر (ہدف_Regions & داخلہ) replicate_مقصد(نتیجہ.m_ico);
            اگر (ہدف_Regions & فعال_2) replicate_مقصد(نتیجہ.m_act);
            اگر (ہدف_Regions & NBA) replicate_مقصد(نتیجہ.m_nba);
            اگر (ہدف_Regions & OBSERVED) replicate_مقصد(نتیجہ.m_obs);
            اگر (ہدف_Regions & REACTIVE) replicate_مقصد(نتیجہ.m_react);
        }
    }
    واپس نتیجہ;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<گراف_2> graphp = build_گراف(logicRegionsRegions);
    // Dump for debug
    اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    برائے (V3_گراف_راس& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<Sched_Replicate_راس>());
    }
    // Dump for debug
    اگر (dump_گراف_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    واپس replicate(graphp.get());
}

}  // namespace V3Sched
