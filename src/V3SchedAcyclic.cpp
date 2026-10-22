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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitV.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SenTree.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3Sched {

ਨਾਮਾਕਾਸ਼ {

// ##############################################################################
//  Data structures (graph types)

ਵਰਗ Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    Ast_ਗੰਢ* ਅਚਲ m_logicp;  // The logic node this vertex represents
    AstScope* ਅਚਲ m_scopep;  // The enclosing AstScope of the logic node

ਜਨਤਕ:
    Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* logicp, AstScope* scopep)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* clone(V3_ਗ੍ਰਾਫ* graphp) ਅਚਲ override {
        ਵਾਪਸ ਨਵਾਂ Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼{graphp, logicp(), scopep()};
    }

    Ast_ਗੰਢ* logicp() ਅਚਲ { ਵਾਪਸ m_logicp; }
    AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_logicp->fileline()->ascii(); };
    string dotShape() ਅਚਲ override { ਵਾਪਸ "rectangle2"; }
    // LCOV_EXCL_STOP
};

ਵਰਗ Sched_Acyclic_Var_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Sched_Acyclic_Var_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    AstVarScope* ਅਚਲ m_vscp;  // The AstVarScope this vertex represents

ਜਨਤਕ:
    Sched_Acyclic_Var_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ਅਚਲ { ਵਾਪਸ m_vscp; }
    AstVar* varp() ਅਚਲ { ਵਾਪਸ m_vscp->varp(); }
    V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* clone(V3_ਗ੍ਰਾਫ* graphp) ਅਚਲ override {
        ਵਾਪਸ ਨਵਾਂ Sched_Acyclic_Var_ਸ਼ੀਰਸ਼{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_vscp->name(); }
    string dotShape() ਅਚਲ override { ਵਾਪਸ "ellipse"; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "blue"; }
    // LCOV_EXCL_STOP
};

ਵਰਗ ਗ੍ਰਾਫ_2 final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    string loops_ਸ਼ੀਰਸ਼_Cb(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vtxp) override {
        ਜੇ (Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp = vtxp->cast<Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼>()) {
            Ast_ਗੰਢ* ਅਚਲ logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            ਜੇ (ਅਚਲ AstAlways* ਅਚਲ alwaysp = VN_CAST(logicp, Always)) {
                ਜੇ (alwaysp->keyword() == VAlwaysKwd::CONT_ਸੌਂਪੋ) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            ਵਾਪਸ logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } ਨਹੀਂਤਾਂ {
            Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp = vtxp->as<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼>();
            AstVarScope* ਅਚਲ vscp = vvtxp->vscp();
            ਵਾਪਸ vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<ਗ੍ਰਾਫ_2> build_ਗ੍ਰਾਫ(ਅਚਲ LogicByScope& lbs) {
    std::unique_ptr<ਗ੍ਰਾਫ_2> graphp{ਨਵਾਂ ਗ੍ਰਾਫ_2};

    // AstVarScope::user1() -> VarVertx
    ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
    ਅਚਲ ਸਵੈਚਾਲਿਤ get_Var_ਸ਼ੀਰਸ਼ = [&](AstVarScope* vscp) {
        ਜੇ (!vscp->user1p()) vscp->user1p(ਨਵਾਂ Sched_Acyclic_Var_ਸ਼ੀਰਸ਼{graphp.get(), vscp});
        ਵਾਪਸ vscp->user1u().ਮੰਜ਼ਿਲ<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼*>();
    };

    ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_ਕਿਨਾਰਾ = [&](V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* fromp, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2, ਤਾਰਕਿਕ cuttable) {
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp.get(), fromp, ਸਿਖਰ, ਭਾਰ_2, cuttable};
    };

    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : lbs) {
        AstScope* ਅਚਲ scopep = ਇੰਦਰਾਜ.m_scopep;
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        ਲਈ (Ast_ਗੰਢ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            ਜੇ (VN_IS(nodep, AlwaysPostponed)) ਜਾਰੀ;

            Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp
                = ਨਵਾਂ Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼{graphp.get(), nodep, scopep};
            ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
            ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;

            V3Sched::util::Var_Scope_ਸੈੱਟ force_Read_ਕਿਨਾਰਾ_Ignores;
            V3Sched::util::collect_Force_Read_ਕਿਨਾਰਾ_Ignores(nodep, force_Read_ਕਿਨਾਰਾ_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ਅਚਲ vscp = refp->varScopep();
                Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp = get_Var_ਸ਼ੀਰਸ਼(vscp);
                // We want to cut the narrowest signals
                ਅਚਲ ਪੂਰਨ ਭਾਰ_2 = vscp->ਚੌੜਾਈ() / 8 + 1;
                // If written, add logic -> var edge
                ਜੇ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_ਸੈੱਟ_Once())
                    ਜੋੜੋ_ਕਿਨਾਰਾ(lvtxp, vvtxp, ਭਾਰ_2, ਸੱਚ);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                ਜੇ (refp->access().isReadOrRW() && !vscp->user3_ਸੈੱਟ_Once() && !vscp->user2()
                    && !force_Read_ਕਿਨਾਰਾ_Ignores.ਗਿਣਤੀ_2(vscp))
                    ਜੋੜੋ_ਕਿਨਾਰਾ(vvtxp, lvtxp, ਭਾਰ_2, ਸੱਚ);
            });
        }
    }

    ਵਾਪਸ graphp;
}

ਖਾਲੀ removeNonCyclic(ਗ੍ਰਾਫ_2* graphp) {
    // Work queue
    std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*> ਕਤਾਰ;

    ਅਚਲ ਸਵੈਚਾਲਿਤ ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ = [&](V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vtxp) {
        ਜੇ (vtxp->user()) ਵਾਪਸ;  // Already in queue
        vtxp->user(1);
        ਕਤਾਰ.ਧੱਕੋ_ਪਿੱਠ(vtxp);
    };

    // Start with vertices with no inputs or outputs
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
        ਜੇ (vtx.ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ() || vtx.ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(&vtx);
    }

    // Iterate while we still have candidates
    ਜਦੋਂ (!ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        // Pop next candidate
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = ਕਤਾਰ.ਪਿੱਠ();
        ਕਤਾਰ.ਕੱਢੋ_ਪਿੱਠ();
        vtxp->user(0);  // No longer in queue

        ਜੇ (vtxp->ਵਿੱਚ_ਖਾਲੀ_ਥਾਵਾਂ()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().unlinkable()) {
                ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(edgep->ਸਿਖਰ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } ਨਹੀਂਤਾਂ ਜੇ (vtxp->ਬਾਹਰਲਾ_ਖਾਲੀ_ਥਾਵਾਂ()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ* ਅਚਲ edgep : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ().unlinkable()) {
                ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
ਤਾਰਕਿਕ isCut(ਅਚਲ Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* vtxp) {
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
        ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2() == 0) ਵਾਪਸ ਸੱਚ;
    }
    ਲਈ (ਅਚਲ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        ਜੇ (ਕਿਨਾਰਾ.ਭਾਰ_2() == 0) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}

std::ਸਦਿਸ਼<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼*> ਲੱਭੋ_Cut_Vertices(ਗ੍ਰਾਫ_2* graphp) {
    // List of cut vertices being computed here
    std::ਸਦਿਸ਼<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼*> ਨਤੀਜਾ;
    ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;  // bool: already added to result

    // Statistics
    ਮਾਪ_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    ਮਾਪ_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_ਸੈੱਟ<uint32_t> sccs;  // Unique SCC colors

    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
        ਜੇ (!vtx.color()) ਜਾਰੀ;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        ਜੇ (Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp = vtx.cast<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼>()) {
            ++nCyclicVars;
            ਜੇ (!vvtxp->vscp()->user1_ਸੈੱਟ_Once() && isCut(vvtxp)) ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        ਜੇ (v3Global.opt.stats()) sccs.ਪਾਓ(vtx.color());
    }

    V3Stats::ਜੋੜੋ_Stat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::ਜੋੜੋ_Stat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::ਜੋੜੋ_Stat("Scheduling, Cycles, unique SCCs", sccs.ਮਾਪ());
    V3Stats::ਜੋੜੋ_Stat("Scheduling, Cycles, cut variables", ਨਤੀਜਾ.ਮਾਪ());

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਖਾਲੀ reset_ਕਿਨਾਰਾ_Weights(ਅਚਲ std::ਸਦਿਸ਼<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼*>& cutVertices) {
    ਲਈ (Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp : cutVertices) {
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e : vvtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) e.ਭਾਰ_2(1);
        ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& e : vvtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) e.ਭਾਰ_2(1);
    }
}

// A VarVertex together with its fanout
ਵਰਤੋਂ Candidate = std::pair<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼*, ਬਿਨਾਨਿਸ਼ਾਨੀ>;

// Gather all splitting candidates that are in the same SCC as the given vertex
ਖਾਲੀ gatherSCCCandidates(V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vtxp, std::ਸਦਿਸ਼<Candidate>& candidates) {
    ਜੇ (vtxp->user()) ਵਾਪਸ;  // Already done
    vtxp->user(ਸੱਚ);

    ਜੇ (Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp = vtxp->cast<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼>()) {
        AstVar* ਅਚਲ varp = vvtxp->varp();
        ਅਚਲ string name = varp->prettyName();
        ਜੇ (!varp->user3_ਸੈੱਟ_Once()  // Only consider each AstVar once
            && varp->ਚੌੜਾਈ() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.ਲੱਭੋ("__Vdly") == string::npos  // Ignore internal signals
            && name.ਲੱਭੋ("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ fanout = vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().ਮਾਪ();
            candidates.emplace_ਪਿੱਠ(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ ਸਿਖਰ = ਕਿਨਾਰਾ.ਸਿਖਰ();
        ਜੇ (ਸਿਖਰ->color() == vtxp->color()) gatherSCCCandidates(ਸਿਖਰ, candidates);
    }
    ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
        V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = ਕਿਨਾਰਾ.fromp();
        ਜੇ (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_ਸਤਰ* /*warnFl*/, ਗ੍ਰਾਫ_2* graphp, Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::ਸਦਿਸ਼<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    ਜੇ (candidates.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    ਬਿਨਾਨਿਸ਼ਾਨੀ splittable = 0;
    ਅਚਲ ਸਵੈਚਾਲਿਤ reportFirst10
        = [&](std::function<ਤਾਰਕਿਕ(ਅਚਲ Candidate&, ਅਚਲ Candidate&)> less) -> string {
        std::stable_ਛਾਂਟੋ(candidates.begin(), candidates.ਸਿਰਾ(), less);
        std::ostringstream ss2;
        ਲਈ (ਮਾਪ_t i = 0; i < 10; i++) {
            ਜੇ (i == candidates.ਮਾਪ()) ਰੋਕੋ;
            ਅਚਲ Candidate& candidate = candidates[i];
            AstVar* ਅਚਲ varp = candidate.ਪਹਿਲਾ->varp();

            ss2 << V3Error::warn_ਹੋਰ() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->ਚੌੜਾਈ() << ", circular fanout "
                << candidate.ਦੂਜਾ;
            ਜੇ (!varp->cannot_ਵੰਡੋ_ਕਿਸਮ_Reason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        ਵਾਪਸ ss2.str();
    };

    // Widest variables
    ss << V3Error::warn_ਹੋਰ() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ਅਚਲ Candidate& a, ਅਚਲ Candidate& b) {
              ਵਾਪਸ a.ਪਹਿਲਾ->varp()->ਚੌੜਾਈ() > b.ਪਹਿਲਾ->varp()->ਚੌੜਾਈ();
          });

    // Highest fanout
    ss << V3Error::warn_ਹੋਰ() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ਅਚਲ Candidate& a, ਅਚਲ Candidate& b) {  //
              ਵਾਪਸ a.ਦੂਜਾ > b.ਦੂਜਾ;
          });

    ਜੇ (splittable) {
        ss << V3Error::warn_ਹੋਰ()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::ਜੋੜੋ_Stat("Scheduling, split_var, candidates", splittable);
    ਵਾਪਸ ss.str();
}

ਖਾਲੀ report_ਚੱਕਰਾਂ(ਗ੍ਰਾਫ_2* graphp, ਅਚਲ std::ਸਦਿਸ਼<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼*>& cutVertices) {
    ਲਈ (Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* vvtxp : cutVertices) {
        AstVarScope* ਅਚਲ vscp = vvtxp->vscp();
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        ਜੇ (flp->warnIsOff(V3_Error_ਕੋਡ::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } ਨਹੀਂਤਾਂ {
            vscp->v3warn(UNOPTFLAT,
                         "Signal unoptimizable: Circular combinational logic: "
                             << vscp->prettyNameQ() << '\n'
                             << vscp->warnContextPrimary()
                             << V3Error::warnAdditionalInfo()
                             // Calls Graph::loopsVertexCb
                             << graphp->reportLoops(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue, vvtxp)
                             // Report candidate variables for splitting
                             << (v3Global.opt.reportUnoptflat()
                                     ? reportLoopVars(vscp->fileline(), graphp, vvtxp)
                                     : ""));
            // Complain just once
            flp->modifyWarnOff(V3_Error_ਕੋਡ::UNOPTFLAT, ਸੱਚ);
            // Create a subgraph for the UNOPTFLAT loop
            ਜੇ (v3Global.opt.reportUnoptflat()) {
                V3_ਗ੍ਰਾਫ loop_ਗ੍ਰਾਫ;
                graphp->subtreeLoops(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue, vvtxp, &loop_ਗ੍ਰਾਫ);
                loop_ਗ੍ਰਾਫ.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

ਖਾਲੀ dumpSccs(V3_ਗ੍ਰਾਫ* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*>> scc2Vtxps;

    // Gather all vertices in each SCC
    ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& vtx : graphp->vertices()) {
        ਜੇ (!vtx.color()) ਜਾਰੀ;
        scc2Vtxps[vtx.color()].ਧੱਕੋ_ਪਿੱਠ(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : scc2Vtxps) {
        ਅਚਲ uint32_t color = pair.ਪਹਿਲਾ;
        ਅਚਲ std::ਸਦਿਸ਼<V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*>& vtxps = pair.ਦੂਜਾ;

        // Open dump file
        ਅਚਲ std::string fname
            = v3Global.debugFilename("sched_scc_" + std::ਮੰਜ਼ਿਲ_string(color) + ".v");
        ਅਚਲ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        ਜੇ (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.ਮਾਪ() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vtxp : vtxps) {
            ਅਚਲ Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp = vtxp->cast<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼>();
            ਜੇ (!vvtxp) ਜਾਰੀ;
            AstVarScope* ਅਚਲ vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_ਰੁੱਖ(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vtxp : vtxps) {
            ਅਚਲ Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp = vtxp->cast<Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼>();
            ਜੇ (!lvtxp) ਜਾਰੀ;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_ਰੁੱਖ(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ਅਚਲ std::ਸਦਿਸ਼<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼*, std::ਸਦਿਸ਼<AstVarScope*>> lvtx2Cuts;
    std::ਸਦਿਸ਼<Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼*> lvtxps;
    {
        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;  // bool: already added to 'lvtxps'
        ਲਈ (Sched_Acyclic_Var_ਸ਼ੀਰਸ਼* ਅਚਲ vvtxp : cutVertices) {
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vvtxp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp
                    = static_cast<Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼*>(ਕਿਨਾਰਾ.ਸਿਖਰ());
                ਜੇ (!lvtxp->logicp()->user1_ਸੈੱਟ_Once()) lvtxps.ਧੱਕੋ_ਪਿੱਠ(lvtxp);
                lvtx2Cuts[lvtxp].ਧੱਕੋ_ਪਿੱਠ(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope ਨਤੀਜਾ;
    Sen_ਰੁੱਖ_Finder finder{netlistp};
    ਲਈ (Sched_Acyclic_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp : lvtxps) {
        Ast_ਗੰਢ* ਅਚਲ logicp = lvtxp->logicp();
        logicp->unlink_Fr_ਪਿੱਠ();
        File_ਸਤਰ* ਅਚਲ flp = logicp->fileline();
        // Build the hybrid sensitivity list
        Ast_Sen_ਵਸਤੂ* senItemsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (AstVarScope* ਅਚਲ vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_ਵਸਤੂ* ਅਚਲ nextp = ਨਵਾਂ Ast_Sen_ਵਸਤੂ{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(senItemsp, nextp);
        }
        Ast_Sen_ਰੁੱਖ* ਅਚਲ sen_ਰੁੱਖ = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{flp, senItemsp};
        // Add logic to result with new sensitivity
        ਨਤੀਜਾ.ਜੋੜੋ(lvtxp->scopep(), finder.get_Sen_ਰੁੱਖ(sen_ਰੁੱਖ), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_ਰੁੱਖ->delete_ਰੁੱਖ(), sen_ਰੁੱਖ);
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

}  // namespace

LogicByScope break_ਚੱਕਰਾਂ(AstNetlist* netlistp, ਅਚਲ LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ਅਚਲ std::unique_ptr<ਗ੍ਰਾਫ_2> graphp = build_ਗ੍ਰਾਫ(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    ਜੇ (graphp->ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ LogicByScope{};

    // Dump for debug
    ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);

    // Find all cut vertices
    ਅਚਲ std::ਸਦਿਸ਼<Sched_Acyclic_Var_ਸ਼ੀਰਸ਼*> cutVertices = ਲੱਭੋ_Cut_Vertices(graphp.get());

    // Reset edge weights for reporting
    reset_ਕਿਨਾਰਾ_Weights(cutVertices);

    // Report warnings/diagnostics
    report_ਚੱਕਰਾਂ(graphp.get(), cutVertices);

    // Debug dump
    ਜੇ (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    ਵਾਪਸ fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
