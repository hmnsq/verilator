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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {

ਨਾਮਾਕਾਸ਼ {

// Driving region flags
ਗਿਣਤੀ RegionFlags : uint8_t {
    NONE = 0x0,  //
    ਨਿਵੇਸ਼ = 0x1,  // Variable/logic is driven from top level input
    ਸਰਗਰਮ_2 = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

ਵਰਗ Sched_Replicate_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sched_Replicate_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

ਜਨਤਕ:
    ਸਪਸ਼ਟ Sched_Replicate_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp} {}
    uint8_t drivingRegions() ਅਚਲ { ਵਾਪਸ m_drivingRegions; }
    ਖਾਲੀ ਜੋੜੋ_Driving_Regions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ਅਚਲ override {
        ਚੋਣ (static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ>(m_drivingRegions)) {
        ਹਾਲਤ NONE: ਵਾਪਸ "black";
        ਹਾਲਤ ਨਿਵੇਸ਼: ਵਾਪਸ "red";
        ਹਾਲਤ ਸਰਗਰਮ_2: ਵਾਪਸ "green";
        ਹਾਲਤ NBA: ਵਾਪਸ "blue";
        ਹਾਲਤ ਨਿਵੇਸ਼ | ਸਰਗਰਮ_2: ਵਾਪਸ "yellow";
        ਹਾਲਤ ਨਿਵੇਸ਼ | NBA: ਵਾਪਸ "magenta";
        ਹਾਲਤ ਸਰਗਰਮ_2 | NBA: ਵਾਪਸ "cyan";
        ਹਾਲਤ ਨਿਵੇਸ਼ | ਸਰਗਰਮ_2 | NBA: ਵਾਪਸ "gray80";  // don't want white on white background

        ਹਾਲਤ REACTIVE: ਵਾਪਸ "gray60";
        ਹਾਲਤ REACTIVE | ਨਿਵੇਸ਼: ਵਾਪਸ "lightcoral";
        ਹਾਲਤ REACTIVE | ਸਰਗਰਮ_2: ਵਾਪਸ "lightgreen";
        ਹਾਲਤ REACTIVE | NBA: ਵਾਪਸ "lightblue";
        ਹਾਲਤ REACTIVE | ਨਿਵੇਸ਼ | ਸਰਗਰਮ_2: ਵਾਪਸ "lightyellow";
        ਹਾਲਤ REACTIVE | ਨਿਵੇਸ਼ | NBA: ਵਾਪਸ "lightpink";
        ਹਾਲਤ REACTIVE | ਸਰਗਰਮ_2 | NBA: ਵਾਪਸ "lightcyan";
        ਹਾਲਤ REACTIVE | ਨਿਵੇਸ਼ | ਸਰਗਰਮ_2 | NBA: ਵਾਪਸ "gray90";

        ਹਾਲਤ OBSERVED: ਵਾਪਸ "gray20";
        ਹਾਲਤ OBSERVED | ਨਿਵੇਸ਼: ਵਾਪਸ "darkred";
        ਹਾਲਤ OBSERVED | ਸਰਗਰਮ_2: ਵਾਪਸ "darkgreen";
        ਹਾਲਤ OBSERVED | NBA: ਵਾਪਸ "darkblue";
        ਹਾਲਤ OBSERVED | ਨਿਵੇਸ਼ | ਸਰਗਰਮ_2: ਵਾਪਸ "gold";
        ਹਾਲਤ OBSERVED | ਨਿਵੇਸ਼ | NBA: ਵਾਪਸ "purple";
        ਹਾਲਤ OBSERVED | ਸਰਗਰਮ_2 | NBA: ਵਾਪਸ "darkcyan";
        ਹਾਲਤ OBSERVED | ਨਿਵੇਸ਼ | ਸਰਗਰਮ_2 | NBA: ਵਾਪਸ "gray30";

        ਹਾਲਤ REACTIVE | OBSERVED: ਵਾਪਸ "gray40";
        ਹਾਲਤ REACTIVE | OBSERVED | ਨਿਵੇਸ਼: ਵਾਪਸ "indianred";
        ਹਾਲਤ REACTIVE | OBSERVED | ਸਰਗਰਮ_2: ਵਾਪਸ "olive";
        ਹਾਲਤ REACTIVE | OBSERVED | NBA: ਵਾਪਸ "dodgerBlue";
        ਹਾਲਤ REACTIVE | OBSERVED | ਨਿਵੇਸ਼ | ਸਰਗਰਮ_2: ਵਾਪਸ "khaki";
        ਹਾਲਤ REACTIVE | OBSERVED | ਨਿਵੇਸ਼ | NBA: ਵਾਪਸ "plum";
        ਹਾਲਤ REACTIVE | OBSERVED | ਸਰਗਰਮ_2 | NBA: ਵਾਪਸ "lightSeaGreen";
        ਹਾਲਤ REACTIVE | OBSERVED | ਨਿਵੇਸ਼ | ਸਰਗਰਮ_2 | NBA: ਵਾਪਸ "gray50";
        ਮੂਲ: v3fatalSrc("There are only 5 region bits"); ਵਾਪਸ "";
        }
    }
    // LCOV_EXCL_STOP
};

ਵਰਗ Sched_Replicate_Logic_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Sched_Replicate_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sched_Replicate_Logic_ਸ਼ੀਰਸ਼, Sched_Replicate_ਸ਼ੀਰਸ਼)
    AstScope* ਅਚਲ m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_ਰੁੱਖ* ਅਚਲ m_senTreep;  // The sensitivity of the logic node
    Ast_ਗੰਢ* ਅਚਲ m_logicp;  // The logic node this vertex represents
    RegionFlags ਅਚਲ m_assignedRegion;  // The region this logic is originally assigned to

ਜਨਤਕ:
    Sched_Replicate_Logic_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstScope* scopep, Ast_Sen_ਰੁੱਖ* senTreep,
                              Ast_ਗੰਢ* logicp, RegionFlags assignedRegion)
        : Sched_Replicate_ਸ਼ੀਰਸ਼{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        ਜੋੜੋ_Driving_Regions(assignedRegion);
    }
    AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_scopep; }
    Ast_Sen_ਰੁੱਖ* senTreep() ਅਚਲ { ਵਾਪਸ m_senTreep; }
    Ast_ਗੰਢ* logicp() ਅਚਲ { ਵਾਪਸ m_logicp; }
    RegionFlags assignedRegion() ਅਚਲ { ਵਾਪਸ m_assignedRegion; }

    // For graph dumping
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_logicp->fileline()->ascii(); };
    string dotShape() ਅਚਲ override { ਵਾਪਸ "rectangle"; }
};

ਵਰਗ Sched_Replicate_Var_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ Sched_Replicate_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sched_Replicate_Var_ਸ਼ੀਰਸ਼, Sched_Replicate_ਸ਼ੀਰਸ਼)
    AstVarScope* ਅਚਲ m_vscp;  // The AstVarScope this vertex represents

ਜਨਤਕ:
    Sched_Replicate_Var_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp)
        : Sched_Replicate_ਸ਼ੀਰਸ਼{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        ਜੇ (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            ਜੋੜੋ_Driving_Regions(ਨਿਵੇਸ਼);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        ਜੇ (varp()->isWrittenBySuspendable()) ਜੋੜੋ_Driving_Regions(ਸਰਗਰਮ_2);
    }
    AstVarScope* vscp() ਅਚਲ { ਵਾਪਸ m_vscp; }
    AstVar* varp() ਅਚਲ { ਵਾਪਸ m_vscp->varp(); }
    AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_vscp->scopep(); }

    // For graph dumping
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_vscp->name(); }
    string dotShape() ਅਚਲ override { ਵਾਪਸ varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

ਵਰਗ ਗ੍ਰਾਫ_2 final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<ਗ੍ਰਾਫ_2> build_ਗ੍ਰਾਫ(ਅਚਲ LogicRegions& logicRegions) {
    std::unique_ptr<ਗ੍ਰਾਫ_2> graphp{ਨਵਾਂ ਗ੍ਰਾਫ_2};

    // AstVarScope::user1() -> VarVertx
    ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
    ਅਚਲ ਸਵੈਚਾਲਿਤ get_Var_ਸ਼ੀਰਸ਼ = [&](AstVarScope* vscp) {
        ਜੇ (!vscp->user1p()) vscp->user1p(ਨਵਾਂ Sched_Replicate_Var_ਸ਼ੀਰਸ਼{graphp.get(), vscp});
        ਵਾਪਸ vscp->user1u().ਮੰਜ਼ਿਲ<Sched_Replicate_Var_ਸ਼ੀਰਸ਼*>();
    };

    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_ਕਿਨਾਰਾ = [&](Sched_Replicate_ਸ਼ੀਰਸ਼* fromp, Sched_Replicate_ਸ਼ੀਰਸ਼* ਸਿਖਰ) {
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp.get(), fromp, ਸਿਖਰ, 1};
    };

    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Logic = [&](RegionFlags region, AstScope* scopep, Ast_ਸਰਗਰਮ* activep) {
        Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<ਤਾਰਕਿਕ(AstVarScope*)> readTriggersThisLogic;

        ਅਚਲ VNUser4_ਵਿੱਚ_Use user4_ਵਿੱਚ_Use;  // bool: Explicit sensitivity of hybrid logic just below

        ਜੇ (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ਵਾਪਸ ਝੂਠ; };
        } ਨਹੀਂਤਾਂ ਜੇ (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ਵਾਪਸ ਸੱਚ; };
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { ਵਾਪਸ !vscp->user4(); };
            senTreep->foreach([](ਅਚਲ AstVarRef* refp) {  //
                refp->varScopep()->user4(ਸੱਚ);
            });
        }

        ਲਈ (Ast_ਗੰਢ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            Sched_Replicate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp
                = ਨਵਾਂ Sched_Replicate_Logic_ਸ਼ੀਰਸ਼{graphp.get(), scopep, senTreep, nodep, region};
            ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
            ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;

            V3Sched::util::Var_Scope_ਸੈੱਟ force_Read_ਕਿਨਾਰਾ_Ignores;
            V3Sched::util::collect_Force_Read_ਕਿਨਾਰਾ_Ignores(nodep, force_Read_ਕਿਨਾਰਾ_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ਅਚਲ vscp = refp->varScopep();
                Sched_Replicate_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp = get_Var_ਸ਼ੀਰਸ਼(vscp);
                ਜੇ (refp->access().isReadOrRW() && !vscp->user3_ਸੈੱਟ_Once()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !force_Read_ਕਿਨਾਰਾ_Ignores.ਗਿਣਤੀ_2(vscp)) {  //
                    ਜੋੜੋ_ਕਿਨਾਰਾ(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                ਜੇ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_ਸੈੱਟ_Once() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    ਜੋੜੋ_ਕਿਨਾਰਾ(lvtxp, vvtxp);
                }
            });
        }
    };

    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : logicRegions.m_pre) ਜੋੜੋ_Logic(ਸਰਗਰਮ_2, ਇੰਦਰਾਜ.m_scopep, ਇੰਦਰਾਜ.m_activep);
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : logicRegions.m_act) ਜੋੜੋ_Logic(ਸਰਗਰਮ_2, ਇੰਦਰਾਜ.m_scopep, ਇੰਦਰਾਜ.m_activep);
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : logicRegions.m_nba) ਜੋੜੋ_Logic(NBA, ਇੰਦਰਾਜ.m_scopep, ਇੰਦਰਾਜ.m_activep);
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : logicRegions.m_obs)
        ਜੋੜੋ_Logic(OBSERVED, ਇੰਦਰਾਜ.m_scopep, ਇੰਦਰਾਜ.m_activep);
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : logicRegions.m_react)
        ਜੋੜੋ_Logic(REACTIVE, ਇੰਦਰਾਜ.m_scopep, ਇੰਦਰਾਜ.m_activep);

    ਵਾਪਸ graphp;
}

ਖਾਲੀ propagateDrivingRegions(Sched_Replicate_ਸ਼ੀਰਸ਼* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    ਜੇ (vtxp->user()) ਵਾਪਸ;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
        Sched_Replicate_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = ਕਿਨਾਰਾ.fromp()->as<Sched_Replicate_ਸ਼ੀਰਸ਼>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->ਜੋੜੋ_Driving_Regions(drivingRegions);

    // Mark as visited
    vtxp->user(ਸੱਚ);
}

LogicReplicas replicate(ਗ੍ਰਾਫ_2* graphp) {
    LogicReplicas ਨਤੀਜਾ;
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
        ਜੇ (Sched_Replicate_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp = vtx.cast<Sched_Replicate_Logic_ਸ਼ੀਰਸ਼>()) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ replicate_ਮੰਜ਼ਿਲ = [&](LogicByScope& lbs) {
                lbs.ਜੋੜੋ(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_ਰੁੱਖ(ਝੂਠ));
            };
            ਅਚਲ uint8_t ਨਿਸ਼ਾਨਾ_Regions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || ਨਿਸ਼ਾਨਾ_Regions == 0,
                    "replicating clocked logic");
            ਜੇ (ਨਿਸ਼ਾਨਾ_Regions & ਨਿਵੇਸ਼) replicate_ਮੰਜ਼ਿਲ(ਨਤੀਜਾ.m_ico);
            ਜੇ (ਨਿਸ਼ਾਨਾ_Regions & ਸਰਗਰਮ_2) replicate_ਮੰਜ਼ਿਲ(ਨਤੀਜਾ.m_act);
            ਜੇ (ਨਿਸ਼ਾਨਾ_Regions & NBA) replicate_ਮੰਜ਼ਿਲ(ਨਤੀਜਾ.m_nba);
            ਜੇ (ਨਿਸ਼ਾਨਾ_Regions & OBSERVED) replicate_ਮੰਜ਼ਿਲ(ਨਤੀਜਾ.m_obs);
            ਜੇ (ਨਿਸ਼ਾਨਾ_Regions & REACTIVE) replicate_ਮੰਜ਼ਿਲ(ਨਤੀਜਾ.m_react);
        }
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ਅਚਲ std::unique_ptr<ਗ੍ਰਾਫ_2> graphp = build_ਗ੍ਰਾਫ(logicRegionsRegions);
    // Dump for debug
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<Sched_Replicate_ਸ਼ੀਰਸ਼>());
    }
    // Dump for debug
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    ਵਾਪਸ replicate(graphp.get());
}

}  // namespace V3Sched
