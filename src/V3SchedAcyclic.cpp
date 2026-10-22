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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3EmitV.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Sched.h"
#అన్తర్భావ "V3SenTree.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <map>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3Sched {

నామాకాశ {

// ##############################################################################
//  Data structures (graph types)

తరగతి Sched_Acyclic_Logic_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Sched_Acyclic_Logic_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    Ast_కణుపు* స్థిరరాశి m_logicp;  // The logic node this vertex represents
    AstScope* స్థిరరాశి m_scopep;  // The enclosing AstScope of the logic node

బహిరంగ:
    Sched_Acyclic_Logic_శీర్షం(V3_లేఖాచిత్రం* graphp, Ast_కణుపు* logicp, AstScope* scopep)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3_లేఖాచిత్రం_శీర్షం* clone(V3_లేఖాచిత్రం* graphp) స్థిరరాశి override {
        తిరిగి కొత్త Sched_Acyclic_Logic_శీర్షం{graphp, logicp(), scopep()};
    }

    Ast_కణుపు* logicp() స్థిరరాశి { తిరిగి m_logicp; }
    AstScope* scopep() స్థిరరాశి { తిరిగి m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_logicp->fileline()->ascii(); };
    string dotShape() స్థిరరాశి override { తిరిగి "rectangle2"; }
    // LCOV_EXCL_STOP
};

తరగతి Sched_Acyclic_Var_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(Sched_Acyclic_Var_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    AstVarScope* స్థిరరాశి m_vscp;  // The AstVarScope this vertex represents

బహిరంగ:
    Sched_Acyclic_Var_శీర్షం(V3_లేఖాచిత్రం* graphp, AstVarScope* vscp)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() స్థిరరాశి { తిరిగి m_vscp; }
    AstVar* varp() స్థిరరాశి { తిరిగి m_vscp->varp(); }
    V3_లేఖాచిత్రం_శీర్షం* clone(V3_లేఖాచిత్రం* graphp) స్థిరరాశి override {
        తిరిగి కొత్త Sched_Acyclic_Var_శీర్షం{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_vscp->name(); }
    string dotShape() స్థిరరాశి override { తిరిగి "ellipse"; }
    string dotColor() స్థిరరాశి override { తిరిగి "blue"; }
    // LCOV_EXCL_STOP
};

తరగతి లేఖాచిత్రం_2 final : బహిరంగ V3_లేఖాచిత్రం {
    string loops_శీర్షం_Cb(V3_లేఖాచిత్రం_శీర్షం* vtxp) override {
        ఒకవేళ (Sched_Acyclic_Logic_శీర్షం* స్థిరరాశి lvtxp = vtxp->cast<Sched_Acyclic_Logic_శీర్షం>()) {
            Ast_కణుపు* స్థిరరాశి logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            ఒకవేళ (స్థిరరాశి AstAlways* స్థిరరాశి alwaysp = VN_CAST(logicp, Always)) {
                ఒకవేళ (alwaysp->keyword() == VAlwaysKwd::CONT_కేటాయించు) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            తిరిగి logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } లేకపోతే {
            Sched_Acyclic_Var_శీర్షం* స్థిరరాశి vvtxp = vtxp->as<Sched_Acyclic_Var_శీర్షం>();
            AstVarScope* స్థిరరాశి vscp = vvtxp->vscp();
            తిరిగి vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<లేఖాచిత్రం_2> build_లేఖాచిత్రం(స్థిరరాశి LogicByScope& lbs) {
    std::unique_ptr<లేఖాచిత్రం_2> graphp{కొత్త లేఖాచిత్రం_2};

    // AstVarScope::user1() -> VarVertx
    స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
    స్థిరరాశి స్వయంచాలక get_Var_శీర్షం = [&](AstVarScope* vscp) {
        ఒకవేళ (!vscp->user1p()) vscp->user1p(కొత్త Sched_Acyclic_Var_శీర్షం{graphp.get(), vscp});
        తిరిగి vscp->user1u().గమ్యం<Sched_Acyclic_Var_శీర్షం*>();
    };

    స్థిరరాశి స్వయంచాలక కూడు_అంచు = [&](V3_లేఖాచిత్రం_శీర్షం* fromp, V3_లేఖాచిత్రం_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2, తార్కిక cuttable) {
        కొత్త V3_లేఖాచిత్రం_అంచు{graphp.get(), fromp, శిఖరం, బరువు_2, cuttable};
    };

    కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : lbs) {
        AstScope* స్థిరరాశి scopep = నమోదు_2.m_scopep;
        Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        కొరకు (Ast_కణుపు* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            ఒకవేళ (VN_IS(nodep, AlwaysPostponed)) కొనసాగు;

            Sched_Acyclic_Logic_శీర్షం* స్థిరరాశి lvtxp
                = కొత్త Sched_Acyclic_Logic_శీర్షం{graphp.get(), nodep, scopep};
            స్థిరరాశి VNUser2_లోపల_Use user2_లోపల_Use;
            స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;

            V3Sched::util::Var_Scope_సమితి force_Read_అంచు_Ignores;
            V3Sched::util::collect_Force_Read_అంచు_Ignores(nodep, force_Read_అంచు_Ignores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* స్థిరరాశి vscp = refp->varScopep();
                Sched_Acyclic_Var_శీర్షం* స్థిరరాశి vvtxp = get_Var_శీర్షం(vscp);
                // We want to cut the narrowest signals
                స్థిరరాశి పూర్ణాంకం బరువు_2 = vscp->వెడల్పు() / 8 + 1;
                // If written, add logic -> var edge
                ఒకవేళ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_సమితి_Once())
                    కూడు_అంచు(lvtxp, vvtxp, బరువు_2, నిజం);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                ఒకవేళ (refp->access().isReadOrRW() && !vscp->user3_సమితి_Once() && !vscp->user2()
                    && !force_Read_అంచు_Ignores.లెక్క(vscp))
                    కూడు_అంచు(vvtxp, lvtxp, బరువు_2, నిజం);
            });
        }
    }

    తిరిగి graphp;
}

శూన్య removeNonCyclic(లేఖాచిత్రం_2* graphp) {
    // Work queue
    std::సదిశ<V3_లేఖాచిత్రం_శీర్షం*> వరుస;

    స్థిరరాశి స్వయంచాలక వరుసలో_చేర్చు = [&](V3_లేఖాచిత్రం_శీర్షం* vtxp) {
        ఒకవేళ (vtxp->user()) తిరిగి;  // Already in queue
        vtxp->user(1);
        వరుస.నెట్టు_వెనుక(vtxp);
    };

    // Start with vertices with no inputs or outputs
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
        ఒకవేళ (vtx.లోపల_ఖాళీలు() || vtx.బహిర్గతం_ఖాళీలు()) వరుసలో_చేర్చు(&vtx);
    }

    // Iterate while we still have candidates
    అయితే (!వరుస.ఖాళీలు()) {
        // Pop next candidate
        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి vtxp = వరుస.వెనుక();
        వరుస.తీయి_వెనుక();
        vtxp->user(0);  // No longer in queue

        ఒకవేళ (vtxp->లోపల_ఖాళీలు()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : vtxp->బహిర్గతం_అంచులు().unlinkable()) {
                వరుసలో_చేర్చు(edgep->శిఖరం());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } లేకపోతే ఒకవేళ (vtxp->బహిర్గతం_ఖాళీలు()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            కొరకు (V3_లేఖాచిత్రం_అంచు* స్థిరరాశి edgep : vtxp->లోపల_అంచులు().unlinkable()) {
                వరుసలో_చేర్చు(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
తార్కిక isCut(స్థిరరాశి Sched_Acyclic_Var_శీర్షం* vtxp) {
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->లోపల_అంచులు()) {
        ఒకవేళ (అంచు.బరువు_2() == 0) తిరిగి నిజం;
    }
    కొరకు (స్థిరరాశి V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->బహిర్గతం_అంచులు()) {
        ఒకవేళ (అంచు.బరువు_2() == 0) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}

std::సదిశ<Sched_Acyclic_Var_శీర్షం*> కనుగొను_Cut_Vertices(లేఖాచిత్రం_2* graphp) {
    // List of cut vertices being computed here
    std::సదిశ<Sched_Acyclic_Var_శీర్షం*> ఫలితం;
    స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;  // bool: already added to result

    // Statistics
    కొలత_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    కొలత_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_సమితి<uint32_t> sccs;  // Unique SCC colors

    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
        ఒకవేళ (!vtx.color()) కొనసాగు;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        ఒకవేళ (Sched_Acyclic_Var_శీర్షం* స్థిరరాశి vvtxp = vtx.cast<Sched_Acyclic_Var_శీర్షం>()) {
            ++nCyclicVars;
            ఒకవేళ (!vvtxp->vscp()->user1_సమితి_Once() && isCut(vvtxp)) ఫలితం.నెట్టు_వెనుక(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        ఒకవేళ (v3Global.opt.stats()) sccs.చొప్పించు(vtx.color());
    }

    V3Stats::కూడు_Stat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::కూడు_Stat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::కూడు_Stat("Scheduling, Cycles, unique SCCs", sccs.కొలత());
    V3Stats::కూడు_Stat("Scheduling, Cycles, cut variables", ఫలితం.కొలత());

    తిరిగి ఫలితం;
}

శూన్య reset_అంచు_Weights(స్థిరరాశి std::సదిశ<Sched_Acyclic_Var_శీర్షం*>& cutVertices) {
    కొరకు (Sched_Acyclic_Var_శీర్షం* స్థిరరాశి vvtxp : cutVertices) {
        కొరకు (V3_లేఖాచిత్రం_అంచు& e : vvtxp->లోపల_అంచులు()) e.బరువు_2(1);
        కొరకు (V3_లేఖాచిత్రం_అంచు& e : vvtxp->బహిర్గతం_అంచులు()) e.బరువు_2(1);
    }
}

// A VarVertex together with its fanout
ఉపయోగించి Candidate = std::pair<Sched_Acyclic_Var_శీర్షం*, గుర్తులేని>;

// Gather all splitting candidates that are in the same SCC as the given vertex
శూన్య gatherSCCCandidates(V3_లేఖాచిత్రం_శీర్షం* vtxp, std::సదిశ<Candidate>& candidates) {
    ఒకవేళ (vtxp->user()) తిరిగి;  // Already done
    vtxp->user(నిజం);

    ఒకవేళ (Sched_Acyclic_Var_శీర్షం* స్థిరరాశి vvtxp = vtxp->cast<Sched_Acyclic_Var_శీర్షం>()) {
        AstVar* స్థిరరాశి varp = vvtxp->varp();
        స్థిరరాశి string name = varp->prettyName();
        ఒకవేళ (!varp->user3_సమితి_Once()  // Only consider each AstVar once
            && varp->వెడల్పు() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.కనుగొను("__Vdly") == string::npos  // Ignore internal signals
            && name.కనుగొను("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            స్థిరరాశి గుర్తులేని fanout = vtxp->బహిర్గతం_అంచులు().కొలత();
            candidates.emplace_వెనుక(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->బహిర్గతం_అంచులు()) {
        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి శిఖరం = అంచు.శిఖరం();
        ఒకవేళ (శిఖరం->color() == vtxp->color()) gatherSCCCandidates(శిఖరం, candidates);
    }
    కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->లోపల_అంచులు()) {
        V3_లేఖాచిత్రం_శీర్షం* స్థిరరాశి fromp = అంచు.fromp();
        ఒకవేళ (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_పంక్తి* /*warnFl*/, లేఖాచిత్రం_2* graphp, Sched_Acyclic_Var_శీర్షం* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::సదిశ<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    ఒకవేళ (candidates.ఖాళీలు()) తిరిగి "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    గుర్తులేని splittable = 0;
    స్థిరరాశి స్వయంచాలక reportFirst10
        = [&](std::function<తార్కిక(స్థిరరాశి Candidate&, స్థిరరాశి Candidate&)> less) -> string {
        std::stable_క్రమీకరించు(candidates.begin(), candidates.చివర(), less);
        std::ostringstream ss2;
        కొరకు (కొలత_t i = 0; i < 10; i++) {
            ఒకవేళ (i == candidates.కొలత()) ఆపు;
            స్థిరరాశి Candidate& candidate = candidates[i];
            AstVar* స్థిరరాశి varp = candidate.మొదటి->varp();

            ss2 << V3Error::warn_మరిన్ని() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->వెడల్పు() << ", circular fanout "
                << candidate.రెండవ;
            ఒకవేళ (!varp->cannot_విభజించు_రకం_Reason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        తిరిగి ss2.str();
    };

    // Widest variables
    ss << V3Error::warn_మరిన్ని() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](స్థిరరాశి Candidate& a, స్థిరరాశి Candidate& b) {
              తిరిగి a.మొదటి->varp()->వెడల్పు() > b.మొదటి->varp()->వెడల్పు();
          });

    // Highest fanout
    ss << V3Error::warn_మరిన్ని() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](స్థిరరాశి Candidate& a, స్థిరరాశి Candidate& b) {  //
              తిరిగి a.రెండవ > b.రెండవ;
          });

    ఒకవేళ (splittable) {
        ss << V3Error::warn_మరిన్ని()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::కూడు_Stat("Scheduling, split_var, candidates", splittable);
    తిరిగి ss.str();
}

శూన్య report_చక్రాలు(లేఖాచిత్రం_2* graphp, స్థిరరాశి std::సదిశ<Sched_Acyclic_Var_శీర్షం*>& cutVertices) {
    కొరకు (Sched_Acyclic_Var_శీర్షం* vvtxp : cutVertices) {
        AstVarScope* స్థిరరాశి vscp = vvtxp->vscp();
        File_పంక్తి* స్థిరరాశి flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        ఒకవేళ (flp->warnIsOff(V3_Error_సంకేతం::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } లేకపోతే {
            vscp->v3warn(UNOPTFLAT,
                         "Signal unoptimizable: Circular combinational logic: "
                             << vscp->prettyNameQ() << '\n'
                             << vscp->warnContextPrimary()
                             << V3Error::warnAdditionalInfo()
                             // Calls Graph::loopsVertexCb
                             << graphp->reportLoops(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue, vvtxp)
                             // Report candidate variables for splitting
                             << (v3Global.opt.reportUnoptflat()
                                     ? reportLoopVars(vscp->fileline(), graphp, vvtxp)
                                     : ""));
            // Complain just once
            flp->modifyWarnOff(V3_Error_సంకేతం::UNOPTFLAT, నిజం);
            // Create a subgraph for the UNOPTFLAT loop
            ఒకవేళ (v3Global.opt.reportUnoptflat()) {
                V3_లేఖాచిత్రం loop_లేఖాచిత్రం;
                graphp->subtreeLoops(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue, vvtxp, &loop_లేఖాచిత్రం);
                loop_లేఖాచిత్రం.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

శూన్య dumpSccs(V3_లేఖాచిత్రం* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::సదిశ<V3_లేఖాచిత్రం_శీర్షం*>> scc2Vtxps;

    // Gather all vertices in each SCC
    కొరకు (V3_లేఖాచిత్రం_శీర్షం& vtx : graphp->vertices()) {
        ఒకవేళ (!vtx.color()) కొనసాగు;
        scc2Vtxps[vtx.color()].నెట్టు_వెనుక(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    కొరకు (స్థిరరాశి స్వయంచాలక& pair : scc2Vtxps) {
        స్థిరరాశి uint32_t color = pair.మొదటి;
        స్థిరరాశి std::సదిశ<V3_లేఖాచిత్రం_శీర్షం*>& vtxps = pair.రెండవ;

        // Open dump file
        స్థిరరాశి std::string fname
            = v3Global.debugFilename("sched_scc_" + std::గమ్యం_string(color) + ".v");
        స్థిరరాశి std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        ఒకవేళ (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.కొలత() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* vtxp : vtxps) {
            స్థిరరాశి Sched_Acyclic_Var_శీర్షం* స్థిరరాశి vvtxp = vtxp->cast<Sched_Acyclic_Var_శీర్షం>();
            ఒకవేళ (!vvtxp) కొనసాగు;
            AstVarScope* స్థిరరాశి vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_వృక్షం(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        కొరకు (V3_లేఖాచిత్రం_శీర్షం* vtxp : vtxps) {
            స్థిరరాశి Sched_Acyclic_Logic_శీర్షం* స్థిరరాశి lvtxp = vtxp->cast<Sched_Acyclic_Logic_శీర్షం>();
            ఒకవేళ (!lvtxp) కొనసాగు;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_వృక్షం(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     స్థిరరాశి std::సదిశ<Sched_Acyclic_Var_శీర్షం*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<Sched_Acyclic_Logic_శీర్షం*, std::సదిశ<AstVarScope*>> lvtx2Cuts;
    std::సదిశ<Sched_Acyclic_Logic_శీర్షం*> lvtxps;
    {
        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;  // bool: already added to 'lvtxps'
        కొరకు (Sched_Acyclic_Var_శీర్షం* స్థిరరాశి vvtxp : cutVertices) {
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vvtxp->బహిర్గతం_అంచులు()) {
                Sched_Acyclic_Logic_శీర్షం* స్థిరరాశి lvtxp
                    = static_cast<Sched_Acyclic_Logic_శీర్షం*>(అంచు.శిఖరం());
                ఒకవేళ (!lvtxp->logicp()->user1_సమితి_Once()) lvtxps.నెట్టు_వెనుక(lvtxp);
                lvtx2Cuts[lvtxp].నెట్టు_వెనుక(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope ఫలితం;
    Sen_వృక్షం_Finder finder{netlistp};
    కొరకు (Sched_Acyclic_Logic_శీర్షం* స్థిరరాశి lvtxp : lvtxps) {
        Ast_కణుపు* స్థిరరాశి logicp = lvtxp->logicp();
        logicp->unlink_Fr_వెనుక();
        File_పంక్తి* స్థిరరాశి flp = logicp->fileline();
        // Build the hybrid sensitivity list
        Ast_Sen_అంశం* senItemsp = శూన్యసూచిక;
        కొరకు (AstVarScope* స్థిరరాశి vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{flp, vscp, VAccess::READ};
            Ast_Sen_అంశం* స్థిరరాశి nextp = కొత్త Ast_Sen_అంశం{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_కణుపు::కూడు_తదుపరి(senItemsp, nextp);
        }
        Ast_Sen_వృక్షం* స్థిరరాశి sen_వృక్షం = కొత్త Ast_Sen_వృక్షం{flp, senItemsp};
        // Add logic to result with new sensitivity
        ఫలితం.కూడు(lvtxp->scopep(), finder.get_Sen_వృక్షం(sen_వృక్షం), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_వృక్షం->delete_వృక్షం(), sen_వృక్షం);
    }
    తిరిగి ఫలితం;
}

}  // namespace

LogicByScope break_చక్రాలు(AstNetlist* netlistp, స్థిరరాశి LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    స్థిరరాశి std::unique_ptr<లేఖాచిత్రం_2> graphp = build_లేఖాచిత్రం(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    ఒకవేళ (graphp->ఖాళీలు()) తిరిగి LogicByScope{};

    // Dump for debug
    ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);

    // Find all cut vertices
    స్థిరరాశి std::సదిశ<Sched_Acyclic_Var_శీర్షం*> cutVertices = కనుగొను_Cut_Vertices(graphp.get());

    // Reset edge weights for reporting
    reset_అంచు_Weights(cutVertices);

    // Report warnings/diagnostics
    report_చక్రాలు(graphp.get(), cutVertices);

    // Debug dump
    ఒకవేళ (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    తిరిగి fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
