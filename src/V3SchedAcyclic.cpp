// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Scheduling - break combinational cycles
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
// Combinational loops are broken by the introduction of instances of the
// 'hybrid' logic. Hybrid logic is like combinational logic, but also has
// explicit sensitivities. Any explicit sensitivity of hybrid logic suppresses
// the implicit sensitivity of the logic on the same variable. This enables us
// to cut combinational logic loops and perform ordering as if the logic is
// acyclic.  See the internals documentation for more details.
//
// To achieve this we build a dependency graph of all combinational logic in
// the design, and then breaks all combinational cycles by converting all
// combinational logic that consumes a variable driven via a 'back-edge' into
// hybrid logic. Here back-edge' just means a graph edge that points from a
// higher rank vertex to a lower rank vertex in some consistent ranking of
// the directed graph. Variables driven via a back-edge in the dependency
// graph are marked, and all combinational logic that depends on such
// variables is converted into hybrid logic, with the back-edge driven
// variables listed as explicit 'changed' sensitivities.
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3EmitV.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"
#ಅನ್ತರ್ಭಾವ "V3SenTree.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3Sched {

ನಾಮಾಕಾಶ {

// ##############################################################################
//  Data structures (graph types)

ವರ್ಗ Sched_Acyclic_Logic_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Sched_Acyclic_Logic_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_logicp;  // The logic node this vertex represents
    AstScope* ಸ್ಥಿರಾಂಕ m_scopep;  // The enclosing AstScope of the logic node

ಸಾರ್ವಜನಿಕ:
    Sched_Acyclic_Logic_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* logicp, AstScope* scopep)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3_ನಕ್ಷೆ_ಶೃಂಗ* clone(V3_ನಕ್ಷೆ* graphp) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಹೊಸ Sched_Acyclic_Logic_ಶೃಂಗ{graphp, logicp(), scopep()};
    }

    Ast_ಗಂಟು* logicp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_logicp; }
    AstScope* scopep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_logicp->fileline()->ascii(); };
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "rectangle2"; }
    // LCOV_EXCL_STOP
};

ವರ್ಗ Sched_Acyclic_Var_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Sched_Acyclic_Var_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    AstVarScope* ಸ್ಥಿರಾಂಕ m_vscp;  // The AstVarScope this vertex represents

ಸಾರ್ವಜನಿಕ:
    Sched_Acyclic_Var_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, AstVarScope* vscp)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp; }
    AstVar* varp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp->varp(); }
    V3_ನಕ್ಷೆ_ಶೃಂಗ* clone(V3_ನಕ್ಷೆ* graphp) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ ಹೊಸ Sched_Acyclic_Var_ಶೃಂಗ{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_vscp->name(); }
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "ellipse"; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "blue"; }
    // LCOV_EXCL_STOP
};

ವರ್ಗ ನಕ್ಷೆ_2 final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    string loops_ಶೃಂಗ_Cb(V3_ನಕ್ಷೆ_ಶೃಂಗ* vtxp) override {
        ಆದರೆ (Sched_Acyclic_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp = vtxp->cast<Sched_Acyclic_Logic_ಶೃಂಗ>()) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstAlways* ಸ್ಥಿರಾಂಕ alwaysp = VN_CAST(logicp, Always)) {
                ಆದರೆ (alwaysp->keyword() == VAlwaysKwd::CONT_ನಿಯೋಜಿಸು) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            ಹಿಂತಿರುಗಿ logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } ಇಲ್ಲದಿದ್ದರೆ {
            Sched_Acyclic_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp = vtxp->as<Sched_Acyclic_Var_ಶೃಂಗ>();
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vvtxp->vscp();
            ಹಿಂತಿರುಗಿ vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<ನಕ್ಷೆ_2> build_ನಕ್ಷೆ(ಸ್ಥಿರಾಂಕ LogicByScope& lbs) {
    std::unique_ptr<ನಕ್ಷೆ_2> graphp{ಹೊಸ ನಕ್ಷೆ_2};

    // AstVarScope::user1() -> VarVertx
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ get_Var_ಶೃಂಗ = [&](AstVarScope* vscp) {
        ಆದರೆ (!vscp->user1p()) vscp->user1p(ಹೊಸ Sched_Acyclic_Var_ಶೃಂಗ{graphp.get(), vscp});
        ಹಿಂತಿರುಗಿ vscp->user1u().ಗಮ್ಯ<Sched_Acyclic_Var_ಶೃಂಗ*>();
    };

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_ಅಂಚು = [&](V3_ನಕ್ಷೆ_ಶೃಂಗ* fromp, V3_ನಕ್ಷೆ_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2, ತಾರ್ಕಿಕ cuttable) {
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{graphp.get(), fromp, ತುದಿ, ತೂಕ_2, cuttable};
    };

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : lbs) {
        AstScope* ಸ್ಥಿರಾಂಕ scopep = ನಮೂದು.m_scopep;
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            ಆದರೆ (VN_IS(nodep, AlwaysPostponed)) ಮುಂದುವರಿ;

            Sched_Acyclic_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp
                = ಹೊಸ Sched_Acyclic_Logic_ಶೃಂಗ{graphp.get(), nodep, scopep};
            ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
            ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;

            V3Sched::util::Var_Scope_ಗಣ force_Read_ಅಂಚು_Ignores;
            V3Sched::util::collect_Force_Read_ಅಂಚು_Ignores(nodep, force_Read_ಅಂಚು_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
                Sched_Acyclic_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp = get_Var_ಶೃಂಗ(vscp);
                // We want to cut the narrowest signals
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ತೂಕ_2 = vscp->ಅಗಲ() / 8 + 1;
                // If written, add logic -> var edge
                ಆದರೆ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_ಗಣ_Once())
                    ಕೂಡಿಸು_ಅಂಚು(lvtxp, vvtxp, ತೂಕ_2, ನಿಜ);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                ಆದರೆ (refp->access().isReadOrRW() && !vscp->user3_ಗಣ_Once() && !vscp->user2()
                    && !force_Read_ಅಂಚು_Ignores.ಎಣಿಕೆ_2(vscp))
                    ಕೂಡಿಸು_ಅಂಚು(vvtxp, lvtxp, ತೂಕ_2, ನಿಜ);
            });
        }
    }

    ಹಿಂತಿರುಗಿ graphp;
}

ಶೂನ್ಯ removeNonCyclic(ನಕ್ಷೆ_2* graphp) {
    // Work queue
    std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಶೃಂಗ*> ಸರತಿ;

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಸರತಿಗೆ_ಸೇರಿಸು = [&](V3_ನಕ್ಷೆ_ಶೃಂಗ* vtxp) {
        ಆದರೆ (vtxp->user()) ಹಿಂತಿರುಗಿ;  // Already in queue
        vtxp->user(1);
        ಸರತಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(vtxp);
    };

    // Start with vertices with no inputs or outputs
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
        ಆದರೆ (vtx.ಒಳ_ಖಾಲಿಗಳು() || vtx.ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) ಸರತಿಗೆ_ಸೇರಿಸು(&vtx);
    }

    // Iterate while we still have candidates
    ಆಗ (!ಸರತಿ.ಖಾಲಿಗಳು()) {
        // Pop next candidate
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = ಸರತಿ.ಹಿಂಭಾಗ();
        ಸರತಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
        vtxp->user(0);  // No longer in queue

        ಆದರೆ (vtxp->ಒಳ_ಖಾಲಿಗಳು()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().unlinkable()) {
                ಸರತಿಗೆ_ಸೇರಿಸು(edgep->ತುದಿ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vtxp->ಹೊರಹೋಗು_ಖಾಲಿಗಳು()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು* ಸ್ಥಿರಾಂಕ edgep : vtxp->ಒಳ_ಅಂಚುಗಳು().unlinkable()) {
                ಸರತಿಗೆ_ಸೇರಿಸು(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
ತಾರ್ಕಿಕ isCut(ಸ್ಥಿರಾಂಕ Sched_Acyclic_Var_ಶೃಂಗ* vtxp) {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
        ಆದರೆ (ಅಂಚು.ತೂಕ_2() == 0) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        ಆದರೆ (ಅಂಚು.ತೂಕ_2() == 0) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

std::ಸದಿಶ<Sched_Acyclic_Var_ಶೃಂಗ*> ಕಂಡುಹಿಡಿ_Cut_Vertices(ನಕ್ಷೆ_2* graphp) {
    // List of cut vertices being computed here
    std::ಸದಿಶ<Sched_Acyclic_Var_ಶೃಂಗ*> ಫಲಿತಾಂಶ;
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;  // bool: already added to result

    // Statistics
    ಗಾತ್ರ_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    ಗಾತ್ರ_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_ಗಣ<uint32_t> sccs;  // Unique SCC colors

    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
        ಆದರೆ (!vtx.color()) ಮುಂದುವರಿ;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        ಆದರೆ (Sched_Acyclic_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp = vtx.cast<Sched_Acyclic_Var_ಶೃಂಗ>()) {
            ++nCyclicVars;
            ಆದರೆ (!vvtxp->vscp()->user1_ಗಣ_Once() && isCut(vvtxp)) ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        ಆದರೆ (v3Global.opt.stats()) sccs.ಸೇರಿಸು(vtx.color());
    }

    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, Cycles, unique SCCs", sccs.ಗಾತ್ರ_2());
    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, Cycles, cut variables", ಫಲಿತಾಂಶ.ಗಾತ್ರ_2());

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಶೂನ್ಯ reset_ಅಂಚು_Weights(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Sched_Acyclic_Var_ಶೃಂಗ*>& cutVertices) {
    ಗಾಗಿ (Sched_Acyclic_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp : cutVertices) {
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& e : vvtxp->ಒಳ_ಅಂಚುಗಳು()) e.ತೂಕ_2(1);
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& e : vvtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) e.ತೂಕ_2(1);
    }
}

// A VarVertex together with its fanout
ಬಳಸಿ Candidate = std::pair<Sched_Acyclic_Var_ಶೃಂಗ*, ಚಿಹ್ನೆರಹಿತ>;

// Gather all splitting candidates that are in the same SCC as the given vertex
ಶೂನ್ಯ gatherSCCCandidates(V3_ನಕ್ಷೆ_ಶೃಂಗ* vtxp, std::ಸದಿಶ<Candidate>& candidates) {
    ಆದರೆ (vtxp->user()) ಹಿಂತಿರುಗಿ;  // Already done
    vtxp->user(ನಿಜ);

    ಆದರೆ (Sched_Acyclic_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp = vtxp->cast<Sched_Acyclic_Var_ಶೃಂಗ>()) {
        AstVar* ಸ್ಥಿರಾಂಕ varp = vvtxp->varp();
        ಸ್ಥಿರಾಂಕ string name = varp->prettyName();
        ಆದರೆ (!varp->user3_ಗಣ_Once()  // Only consider each AstVar once
            && varp->ಅಗಲ() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.ಕಂಡುಹಿಡಿ("__Vdly") == string::npos  // Ignore internal signals
            && name.ಕಂಡುಹಿಡಿ("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ fanout = vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().ಗಾತ್ರ_2();
            candidates.emplace_ಹಿಂಭಾಗ(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ತುದಿ = ಅಂಚು.ತುದಿ();
        ಆದರೆ (ತುದಿ->color() == vtxp->color()) gatherSCCCandidates(ತುದಿ, candidates);
    }
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
        V3_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = ಅಂಚು.fromp();
        ಆದರೆ (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_ಸಾಲು* /*warnFl*/, ನಕ್ಷೆ_2* graphp, Sched_Acyclic_Var_ಶೃಂಗ* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::ಸದಿಶ<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    ಆದರೆ (candidates.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    ಚಿಹ್ನೆರಹಿತ splittable = 0;
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ reportFirst10
        = [&](std::function<ತಾರ್ಕಿಕ(ಸ್ಥಿರಾಂಕ Candidate&, ಸ್ಥಿರಾಂಕ Candidate&)> less) -> string {
        std::stable_ವಿಂಗಡಿಸು(candidates.begin(), candidates.ಕೊನೆ(), less);
        std::ostringstream ss2;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < 10; i++) {
            ಆದರೆ (i == candidates.ಗಾತ್ರ_2()) ನಿಲ್ಲಿಸು;
            ಸ್ಥಿರಾಂಕ Candidate& candidate = candidates[i];
            AstVar* ಸ್ಥಿರಾಂಕ varp = candidate.ಮೊದಲ->varp();

            ss2 << V3Error::warn_ಇನ್ನಷ್ಟು() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->ಅಗಲ() << ", circular fanout "
                << candidate.ಎರಡನೇ;
            ಆದರೆ (!varp->cannot_ವಿಭಜಿಸು_ಬಗೆ_Reason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        ಹಿಂತಿರುಗಿ ss2.str();
    };

    // Widest variables
    ss << V3Error::warn_ಇನ್ನಷ್ಟು() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ಸ್ಥಿರಾಂಕ Candidate& a, ಸ್ಥಿರಾಂಕ Candidate& b) {
              ಹಿಂತಿರುಗಿ a.ಮೊದಲ->varp()->ಅಗಲ() > b.ಮೊದಲ->varp()->ಅಗಲ();
          });

    // Highest fanout
    ss << V3Error::warn_ಇನ್ನಷ್ಟು() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ಸ್ಥಿರಾಂಕ Candidate& a, ಸ್ಥಿರಾಂಕ Candidate& b) {  //
              ಹಿಂತಿರುಗಿ a.ಎರಡನೇ > b.ಎರಡನೇ;
          });

    ಆದರೆ (splittable) {
        ss << V3Error::warn_ಇನ್ನಷ್ಟು()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::ಕೂಡಿಸು_Stat("Scheduling, split_var, candidates", splittable);
    ಹಿಂತಿರುಗಿ ss.str();
}

ಶೂನ್ಯ report_ಚಕ್ರಗಳು(ನಕ್ಷೆ_2* graphp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Sched_Acyclic_Var_ಶೃಂಗ*>& cutVertices) {
    ಗಾಗಿ (Sched_Acyclic_Var_ಶೃಂಗ* vvtxp : cutVertices) {
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vvtxp->vscp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        ಆದರೆ (flp->warnIsOff(V3_Error_ಸಂಕೇತ::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            vscp->v3warn(UNOPTFLAT,
                         "Signal unoptimizable: Circular combinational logic: "
                             << vscp->prettyNameQ() << '\n'
                             << vscp->warnContextPrimary()
                             << V3Error::warnAdditionalInfo()
                             // Calls Graph::loopsVertexCb
                             << graphp->reportLoops(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue, vvtxp)
                             // Report candidate variables for splitting
                             << (v3Global.opt.reportUnoptflat()
                                     ? reportLoopVars(vscp->fileline(), graphp, vvtxp)
                                     : ""));
            // Complain just once
            flp->modifyWarnOff(V3_Error_ಸಂಕೇತ::UNOPTFLAT, ನಿಜ);
            // Create a subgraph for the UNOPTFLAT loop
            ಆದರೆ (v3Global.opt.reportUnoptflat()) {
                V3_ನಕ್ಷೆ loop_ನಕ್ಷೆ;
                graphp->subtreeLoops(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue, vvtxp, &loop_ನಕ್ಷೆ);
                loop_ನಕ್ಷೆ.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

ಶೂನ್ಯ dumpSccs(V3_ನಕ್ಷೆ* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಶೃಂಗ*>> scc2Vtxps;

    // Gather all vertices in each SCC
    ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& vtx : graphp->vertices()) {
        ಆದರೆ (!vtx.color()) ಮುಂದುವರಿ;
        scc2Vtxps[vtx.color()].ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : scc2Vtxps) {
        ಸ್ಥಿರಾಂಕ uint32_t color = pair.ಮೊದಲ;
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<V3_ನಕ್ಷೆ_ಶೃಂಗ*>& vtxps = pair.ಎರಡನೇ;

        // Open dump file
        ಸ್ಥಿರಾಂಕ std::string fname
            = v3Global.debugFilename("sched_scc_" + std::ಗಮ್ಯ_string(color) + ".v");
        ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        ಆದರೆ (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.ಗಾತ್ರ_2() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* vtxp : vtxps) {
            ಸ್ಥಿರಾಂಕ Sched_Acyclic_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp = vtxp->cast<Sched_Acyclic_Var_ಶೃಂಗ>();
            ಆದರೆ (!vvtxp) ಮುಂದುವರಿ;
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ* vtxp : vtxps) {
            ಸ್ಥಿರಾಂಕ Sched_Acyclic_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp = vtxp->cast<Sched_Acyclic_Logic_ಶೃಂಗ>();
            ಆದರೆ (!lvtxp) ಮುಂದುವರಿ;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Sched_Acyclic_Var_ಶೃಂಗ*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<Sched_Acyclic_Logic_ಶೃಂಗ*, std::ಸದಿಶ<AstVarScope*>> lvtx2Cuts;
    std::ಸದಿಶ<Sched_Acyclic_Logic_ಶೃಂಗ*> lvtxps;
    {
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;  // bool: already added to 'lvtxps'
        ಗಾಗಿ (Sched_Acyclic_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vvtxp : cutVertices) {
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vvtxp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                Sched_Acyclic_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp
                    = static_cast<Sched_Acyclic_Logic_ಶೃಂಗ*>(ಅಂಚು.ತುದಿ());
                ಆದರೆ (!lvtxp->logicp()->user1_ಗಣ_Once()) lvtxps.ತಳ್ಳು_ಹಿಂಭಾಗ(lvtxp);
                lvtx2Cuts[lvtxp].ತಳ್ಳು_ಹಿಂಭಾಗ(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope ಫಲಿತಾಂಶ;
    Sen_ವೃಕ್ಷ_Finder finder{netlistp};
    ಗಾಗಿ (Sched_Acyclic_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp : lvtxps) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ logicp = lvtxp->logicp();
        logicp->unlink_Fr_ಹಿಂಭಾಗ();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = logicp->fileline();
        // Build the hybrid sensitivity list
        Ast_Sen_ಅಂಶ* senItemsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_ಅಂಶ* ಸ್ಥಿರಾಂಕ nextp = ಹೊಸ Ast_Sen_ಅಂಶ{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(senItemsp, nextp);
        }
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sen_ವೃಕ್ಷ = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{flp, senItemsp};
        // Add logic to result with new sensitivity
        ಫಲಿತಾಂಶ.ಕೂಡಿಸು(lvtxp->scopep(), finder.get_Sen_ವೃಕ್ಷ(sen_ವೃಕ್ಷ), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_ವೃಕ್ಷ->delete_ವೃಕ್ಷ(), sen_ವೃಕ್ಷ);
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

}  // namespace

LogicByScope break_ಚಕ್ರಗಳು(AstNetlist* netlistp, ಸ್ಥಿರಾಂಕ LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ಸ್ಥಿರಾಂಕ std::unique_ptr<ನಕ್ಷೆ_2> graphp = build_ನಕ್ಷೆ(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    ಆದರೆ (graphp->ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ LogicByScope{};

    // Dump for debug
    ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);

    // Find all cut vertices
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<Sched_Acyclic_Var_ಶೃಂಗ*> cutVertices = ಕಂಡುಹಿಡಿ_Cut_Vertices(graphp.get());

    // Reset edge weights for reporting
    reset_ಅಂಚು_Weights(cutVertices);

    // Report warnings/diagnostics
    report_ಚಕ್ರಗಳು(graphp.get(), cutVertices);

    // Debug dump
    ಆದರೆ (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    ಹಿಂತಿರುಗಿ fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
