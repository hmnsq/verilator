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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ನಾಮಾಕಾಶ {

// Driving region flags
ಎಣಿಕೆ RegionFlags : uint8_t {
    NONE = 0x0,  //
    ಒಳಹರಿವು = 0x1,  // Variable/logic is driven from top level input
    ಸಕ್ರಿಯ_2 = 0x2,  // Variable/logic is driven from 'act' region logic
    NBA = 0x4,  // Variable/logic is driven from 'nba' region logic
    OBSERVED = 0x8,  // Variable/logic is driven from 'obs' region logic
    REACTIVE = 0x10,  // Variable/logic is driven from 're' region logic
};

//##############################################################################
// Data structures (graph types)

ವರ್ಗ Sched_Replicate_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Sched_Replicate_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    RegionFlags m_drivingRegions{RegionFlags::NONE};  // The regions driving this vertex

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ Sched_Replicate_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp} {}
    uint8_t drivingRegions() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_drivingRegions; }
    ಶೂನ್ಯ ಕೂಡಿಸು_Driving_Regions(uint8_t regions) {
        m_drivingRegions = static_cast<RegionFlags>(m_drivingRegions | regions);
    }

    // LCOV_EXCL_START // Debug code
    string dotColor() ಸ್ಥಿರಾಂಕ override {
        ಆಯ್ಕೆ (static_cast<ಚಿಹ್ನೆರಹಿತ>(m_drivingRegions)) {
        ಸ್ಥಿತಿ NONE: ಹಿಂತಿರುಗಿ "black";
        ಸ್ಥಿತಿ ಒಳಹರಿವು: ಹಿಂತಿರುಗಿ "red";
        ಸ್ಥಿತಿ ಸಕ್ರಿಯ_2: ಹಿಂತಿರುಗಿ "green";
        ಸ್ಥಿತಿ NBA: ಹಿಂತಿರುಗಿ "blue";
        ಸ್ಥಿತಿ ಒಳಹರಿವು | ಸಕ್ರಿಯ_2: ಹಿಂತಿರುಗಿ "yellow";
        ಸ್ಥಿತಿ ಒಳಹರಿವು | NBA: ಹಿಂತಿರುಗಿ "magenta";
        ಸ್ಥಿತಿ ಸಕ್ರಿಯ_2 | NBA: ಹಿಂತಿರುಗಿ "cyan";
        ಸ್ಥಿತಿ ಒಳಹರಿವು | ಸಕ್ರಿಯ_2 | NBA: ಹಿಂತಿರುಗಿ "gray80";  // don't want white on white background

        ಸ್ಥಿತಿ REACTIVE: ಹಿಂತಿರುಗಿ "gray60";
        ಸ್ಥಿತಿ REACTIVE | ಒಳಹರಿವು: ಹಿಂತಿರುಗಿ "lightcoral";
        ಸ್ಥಿತಿ REACTIVE | ಸಕ್ರಿಯ_2: ಹಿಂತಿರುಗಿ "lightgreen";
        ಸ್ಥಿತಿ REACTIVE | NBA: ಹಿಂತಿರುಗಿ "lightblue";
        ಸ್ಥಿತಿ REACTIVE | ಒಳಹರಿವು | ಸಕ್ರಿಯ_2: ಹಿಂತಿರುಗಿ "lightyellow";
        ಸ್ಥಿತಿ REACTIVE | ಒಳಹರಿವು | NBA: ಹಿಂತಿರುಗಿ "lightpink";
        ಸ್ಥಿತಿ REACTIVE | ಸಕ್ರಿಯ_2 | NBA: ಹಿಂತಿರುಗಿ "lightcyan";
        ಸ್ಥಿತಿ REACTIVE | ಒಳಹರಿವು | ಸಕ್ರಿಯ_2 | NBA: ಹಿಂತಿರುಗಿ "gray90";

        ಸ್ಥಿತಿ OBSERVED: ಹಿಂತಿರುಗಿ "gray20";
        ಸ್ಥಿತಿ OBSERVED | ಒಳಹರಿವು: ಹಿಂತಿರುಗಿ "darkred";
        ಸ್ಥಿತಿ OBSERVED | ಸಕ್ರಿಯ_2: ಹಿಂತಿರುಗಿ "darkgreen";
        ಸ್ಥಿತಿ OBSERVED | NBA: ಹಿಂತಿರುಗಿ "darkblue";
        ಸ್ಥಿತಿ OBSERVED | ಒಳಹರಿವು | ಸಕ್ರಿಯ_2: ಹಿಂತಿರುಗಿ "gold";
        ಸ್ಥಿತಿ OBSERVED | ಒಳಹರಿವು | NBA: ಹಿಂತಿರುಗಿ "purple";
        ಸ್ಥಿತಿ OBSERVED | ಸಕ್ರಿಯ_2 | NBA: ಹಿಂತಿರುಗಿ "darkcyan";
        ಸ್ಥಿತಿ OBSERVED | ಒಳಹರಿವು | ಸಕ್ರಿಯ_2 | NBA: ಹಿಂತಿರುಗಿ "gray30";

        ಸ್ಥಿತಿ REACTIVE | OBSERVED: ಹಿಂತಿರುಗಿ "gray40";
        ಸ್ಥಿತಿ REACTIVE | OBSERVED | ಒಳಹರಿವು: ಹಿಂತಿರುಗಿ "indianred";
        ಸ್ಥಿತಿ REACTIVE | OBSERVED | ಸಕ್ರಿಯ_2: ಹಿಂತಿರುಗಿ "olive";
        ಸ್ಥಿತಿ REACTIVE | OBSERVED | NBA: ಹಿಂತಿರುಗಿ "dodgerBlue";
        ಸ್ಥಿತಿ REACTIVE | OBSERVED | ಒಳಹರಿವು | ಸಕ್ರಿಯ_2: ಹಿಂತಿರುಗಿ "khaki";
        ಸ್ಥಿತಿ REACTIVE | OBSERVED | ಒಳಹರಿವು | NBA: ಹಿಂತಿರುಗಿ "plum";
        ಸ್ಥಿತಿ REACTIVE | OBSERVED | ಸಕ್ರಿಯ_2 | NBA: ಹಿಂತಿರುಗಿ "lightSeaGreen";
        ಸ್ಥಿತಿ REACTIVE | OBSERVED | ಒಳಹರಿವು | ಸಕ್ರಿಯ_2 | NBA: ಹಿಂತಿರುಗಿ "gray50";
        ಪೂರ್ವನಿಯೋಜಿತ: v3fatalSrc("There are only 5 region bits"); ಹಿಂತಿರುಗಿ "";
        }
    }
    // LCOV_EXCL_STOP
};

ವರ್ಗ Sched_Replicate_Logic_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Sched_Replicate_ಶೃಂಗ {
    VL_RTTI_IMPL(Sched_Replicate_Logic_ಶೃಂಗ, Sched_Replicate_ಶೃಂಗ)
    AstScope* ಸ್ಥಿರಾಂಕ m_scopep;  // The enclosing AstScope of the logic node
    Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ m_senTreep;  // The sensitivity of the logic node
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_logicp;  // The logic node this vertex represents
    RegionFlags ಸ್ಥಿರಾಂಕ m_assignedRegion;  // The region this logic is originally assigned to

ಸಾರ್ವಜನಿಕ:
    Sched_Replicate_Logic_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstScope* scopep, Ast_Sen_ವೃಕ್ಷ* senTreep,
                              Ast_ಗಂಟು* logicp, RegionFlags assignedRegion)
        : Sched_Replicate_ಶೃಂಗ{graphp}
        , m_scopep{scopep}
        , m_senTreep{senTreep}
        , m_logicp{logicp}
        , m_assignedRegion{assignedRegion} {
        ಕೂಡಿಸು_Driving_Regions(assignedRegion);
    }
    AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopep; }
    Ast_Sen_ವೃಕ್ಷ* senTreep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_senTreep; }
    Ast_ಗಂಟು* logicp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_logicp; }
    RegionFlags assignedRegion() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_assignedRegion; }

    // For graph dumping
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_logicp->fileline()->ascii(); };
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "rectangle"; }
};

ವರ್ಗ Sched_Replicate_Var_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ Sched_Replicate_ಶೃಂಗ {
    VL_RTTI_IMPL(Sched_Replicate_Var_ಶೃಂಗ, Sched_Replicate_ಶೃಂಗ)
    AstVarScope* ಸ್ಥಿರಾಂಕ m_vscp;  // The AstVarScope this vertex represents

ಸಾರ್ವಜನಿಕ:
    Sched_Replicate_Var_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* vscp)
        : Sched_Replicate_ಶೃಂಗ{graphp}
        , m_vscp{vscp} {
        // Top level inputs are
        ಆದರೆ (varp()->isPrimaryInish() || varp()->isSigUserRWPublic() || varp()->sampled()
            || varp()->isWrittenByDpi() || varp()->sensIfacep() || varp()->isVirtIface()) {
            ಕೂಡಿಸು_Driving_Regions(ಒಳಹರಿವು);
        }
        // Currently we always execute suspendable processes at the beginning of
        // the act region, which means combinational logic driven from a suspendable
        // processes must be present in the 'act' region
        ಆದರೆ (varp()->isWrittenBySuspendable()) ಕೂಡಿಸು_Driving_Regions(ಸಕ್ರಿಯ_2);
    }
    AstVarScope* vscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp; }
    AstVar* varp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp->varp(); }
    AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp->scopep(); }

    // For graph dumping
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_vscp->name(); }
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ varp()->isPrimaryInish() ? "invhouse" : "ellipse"; }
};

ವರ್ಗ ನಕ್ಷೆ_2 final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {};

//##############################################################################
// Algorithm implementation

std::unique_ptr<ನಕ್ಷೆ_2> build_ನಕ್ಷೆ(ಸ್ಥಿರಾಂಕ LogicRegions& logicRegions) {
    std::unique_ptr<ನಕ್ಷೆ_2> graphp{ಹೊಸ ನಕ್ಷೆ_2};

    // AstVarScope::user1() -> VarVertx
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ get_Var_ಶೃಂಗ = [&](AstVarScope* vscp) {
        ಆದರೆ (!vscp->user1p()) vscp->user1p(ಹೊಸ Sched_Replicate_Var_ಶೃಂಗ{graphp.get(), vscp});
        ಹಿಂತಿರುಗಿ vscp->user1u().ಗಮ್ಯ<Sched_Replicate_Var_ಶೃಂಗ*>();
    };

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_ಅಂಚು = [&](Sched_Replicate_ಶೃಂಗ* fromp, Sched_Replicate_ಶೃಂಗ* ತುದಿ) {
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{graphp.get(), fromp, ತುದಿ, 1};
    };

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Logic = [&](RegionFlags region, AstScope* scopep, Ast_ಸಕ್ರಿಯ* activep) {
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = activep->sentreep();

        // Predicate for whether a read of the given variable triggers this block
        std::function<ತಾರ್ಕಿಕ(AstVarScope*)> readTriggersThisLogic;

        ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use user4_ಒಳ_Use;  // bool: Explicit sensitivity of hybrid logic just below

        ಆದರೆ (senTreep->hasClocked()) {
            // Clocked logic is never triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ಹಿಂತಿರುಗಿ ಸುಳ್ಳು; };
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (senTreep->hasCombo()) {
            // Combinational logic is always triggered by reads
            readTriggersThisLogic = [](AstVarScope*) { ಹಿಂತಿರುಗಿ ನಿಜ; };
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(senTreep->hasHybrid(), activep, "unexpected");
            // Hybrid logic is triggered by all reads, except for reads of the explicit
            // sensitivities
            readTriggersThisLogic = [](AstVarScope* vscp) { ಹಿಂತಿರುಗಿ !vscp->user4(); };
            senTreep->foreach([](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {  //
                refp->varScopep()->user4(ನಿಜ);
            });
        }

        ಗಾಗಿ (Ast_ಗಂಟು* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            Sched_Replicate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp
                = ಹೊಸ Sched_Replicate_Logic_ಶೃಂಗ{graphp.get(), scopep, senTreep, nodep, region};
            ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
            ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;

            V3Sched::util::Var_Scope_ಗಣ force_Read_ಅಂಚು_Ignores;
            V3Sched::util::collect_Force_Read_ಅಂಚು_Ignores(nodep, force_Read_ಅಂಚು_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
                Sched_Replicate_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp = get_Var_ಶೃಂಗ(vscp);
                ಆದರೆ (refp->access().isReadOrRW() && !vscp->user3_ಗಣ_Once()
                    && readTriggersThisLogic(vscp) && !vscp->user2()
                    && !force_Read_ಅಂಚು_Ignores.ಎಣಿಕೆ_2(vscp)) {  //
                    ಕೂಡಿಸು_ಅಂಚು(vvtxp, lvtxp);
                }
                // If written, add logic -> var edge
                // Note: See V3Order for why AlwaysPostponed is safe to be ignored. We ignore it
                // as otherwise we would end up with a false cycle.
                ಆದರೆ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_ಗಣ_Once() && !VN_IS(nodep, AlwaysPostponed)) {  //
                    ಕೂಡಿಸು_ಅಂಚು(lvtxp, vvtxp);
                }
            });
        }
    };

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : logicRegions.m_pre) ಕೂಡಿಸು_Logic(ಸಕ್ರಿಯ_2, ನಮೂದು.m_scopep, ನಮೂದು.m_activep);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : logicRegions.m_act) ಕೂಡಿಸು_Logic(ಸಕ್ರಿಯ_2, ನಮೂದು.m_scopep, ನಮೂದು.m_activep);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : logicRegions.m_nba) ಕೂಡಿಸು_Logic(NBA, ನಮೂದು.m_scopep, ನಮೂದು.m_activep);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : logicRegions.m_obs)
        ಕೂಡಿಸು_Logic(OBSERVED, ನಮೂದು.m_scopep, ನಮೂದು.m_activep);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : logicRegions.m_react)
        ಕೂಡಿಸು_Logic(REACTIVE, ನಮೂದು.m_scopep, ನಮೂದು.m_activep);

    ಹಿಂತಿರುಗಿ graphp;
}

ಶೂನ್ಯ propagateDrivingRegions(Sched_Replicate_ಶೃಂಗ* vtxp) {
    // Note: The graph is always acyclic, so the recursion will terminate

    // Nothing to do if already visited
    ಆದರೆ (vtxp->user()) ಹಿಂತಿರುಗಿ;

    // Compute union of driving regions of all inputs
    uint8_t drivingRegions = 0;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
        Sched_Replicate_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = ಅಂಚು.fromp()->as<Sched_Replicate_ಶೃಂಗ>();
        propagateDrivingRegions(srcp);
        drivingRegions |= srcp->drivingRegions();
    }

    // Add any new driving regions
    vtxp->ಕೂಡಿಸು_Driving_Regions(drivingRegions);

    // Mark as visited
    vtxp->user(ನಿಜ);
}

LogicReplicas replicate(ನಕ್ಷೆ_2* graphp) {
    LogicReplicas ಫಲಿತಾಂಶ;
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
        ಆದರೆ (Sched_Replicate_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp = vtx.cast<Sched_Replicate_Logic_ಶೃಂಗ>()) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ replicate_ಗಮ್ಯ = [&](LogicByScope& lbs) {
                lbs.ಕೂಡಿಸು(lvtxp->scopep(), lvtxp->senTreep(), lvtxp->logicp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            };
            ಸ್ಥಿರಾಂಕ uint8_t ಗುರಿ_Regions = lvtxp->drivingRegions() & ~lvtxp->assignedRegion();
            UASSERT(!lvtxp->senTreep()->hasClocked() || ಗುರಿ_Regions == 0,
                    "replicating clocked logic");
            ಆದರೆ (ಗುರಿ_Regions & ಒಳಹರಿವು) replicate_ಗಮ್ಯ(ಫಲಿತಾಂಶ.m_ico);
            ಆದರೆ (ಗುರಿ_Regions & ಸಕ್ರಿಯ_2) replicate_ಗಮ್ಯ(ಫಲಿತಾಂಶ.m_act);
            ಆದರೆ (ಗುರಿ_Regions & NBA) replicate_ಗಮ್ಯ(ಫಲಿತಾಂಶ.m_nba);
            ಆದರೆ (ಗುರಿ_Regions & OBSERVED) replicate_ಗಮ್ಯ(ಫಲಿತಾಂಶ.m_obs);
            ಆದರೆ (ಗುರಿ_Regions & REACTIVE) replicate_ಗಮ್ಯ(ಫಲಿತಾಂಶ.m_react);
        }
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

}  // namespace

LogicReplicas replicateLogic(LogicRegions& logicRegionsRegions) {
    // Build the dataflow (dependency) graph
    ಸ್ಥಿರಾಂಕ std::unique_ptr<ನಕ್ಷೆ_2> graphp = build_ನಕ್ಷೆ(logicRegionsRegions);
    // Dump for debug
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate");
    // Propagate driving region flags
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
        propagateDrivingRegions(vtx.as<Sched_Replicate_ಶೃಂಗ>());
    }
    // Dump for debug
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("sched-replicate-propagated");
    // Replicate the necessary logic
    ಹಿಂತಿರುಗಿ replicate(graphp.get());
}

}  // namespace V3Sched
