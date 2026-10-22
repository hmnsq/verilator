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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3EmitV.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Graph.h"
#অন্তর্ভাৱ "V3Sched.h"
#অন্তর্ভাৱ "V3SenTree.h"
#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <unordered_set>
#অন্তর্ভাৱ <utility>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3Sched {

নামাকাশ {

// ##############################################################################
//  Data structures (graph types)

শ্ৰেণী SchedAcyclicLogicVertex final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_গ্ৰন্থি* ধ্রুৱ m_logicp;  // The logic node this vertex represents
    AstScope* ধ্রুৱ m_scopep;  // The enclosing AstScope of the logic node

সার্ৱজনিক:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_গ্ৰন্থি* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) ধ্রুৱ override {
        ঘূৰাই নূতন SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_গ্ৰন্থি* logicp() ধ্রুৱ { ঘূৰাই m_logicp; }
    AstScope* scopep() ধ্রুৱ { ঘূৰাই m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_logicp->fileline()->ascii(); };
    string dotShape() ধ্রুৱ override { ঘূৰাই "rectangle2"; }
    // LCOV_EXCL_STOP
};

শ্ৰেণী SchedAcyclicVarVertex final : সার্ৱজনিক V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* ধ্রুৱ m_vscp;  // The AstVarScope this vertex represents

সার্ৱজনিক:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ধ্রুৱ { ঘূৰাই m_vscp; }
    AstVar* varp() ধ্রুৱ { ঘূৰাই m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) ধ্রুৱ override {
        ঘূৰাই নূতন SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ধ্রুৱ override VL_MT_STABLE { ঘূৰাই m_vscp->name(); }
    string dotShape() ধ্রুৱ override { ঘূৰাই "ellipse"; }
    string dotColor() ধ্রুৱ override { ঘূৰাই "blue"; }
    // LCOV_EXCL_STOP
};

শ্ৰেণী Graph final : সার্ৱজনিক V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        যদি (SchedAcyclicLogicVertex* ধ্রুৱ lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_গ্ৰন্থি* ধ্রুৱ logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            যদি (ধ্রুৱ AstAlways* ধ্রুৱ alwaysp = VN_CAST(logicp, Always)) {
                যদি (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            ঘূৰাই logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } অন্যথা {
            SchedAcyclicVarVertex* ধ্রুৱ vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* ধ্রুৱ vscp = vvtxp->vscp();
            ঘূৰাই vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ধ্রুৱ LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{নূতন Graph};

    // AstVarScope::user1() -> VarVertx
    ধ্রুৱ VNUser1InUse user1InUse;
    ধ্রুৱ auto getVarVertex = [&](AstVarScope* vscp) {
        যদি (!vscp->user1p()) vscp->user1p(নূতন SchedAcyclicVarVertex{graphp.get(), vscp});
        ঘূৰাই vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    ধ্রুৱ auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* শীৰ্ষ, পূর্ণাঙ্ক weight, তার্কিক cuttable) {
        নূতন V3GraphEdge{graphp.get(), fromp, শীৰ্ষ, weight, cuttable};
    };

    বাবে (ধ্রুৱ auto& entry : lbs) {
        AstScope* ধ্রুৱ scopep = entry.m_scopep;
        AstActive* ধ্রুৱ activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        বাবে (Ast_গ্ৰন্থি* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            যদি (VN_IS(nodep, AlwaysPostponed)) অনুৱর্ত;

            SchedAcyclicLogicVertex* ধ্রুৱ lvtxp
                = নূতন SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            ধ্রুৱ VNUser2InUse user2InUse;
            ধ্রুৱ VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ধ্রুৱ vscp = refp->varScopep();
                SchedAcyclicVarVertex* ধ্রুৱ vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                ধ্রুৱ পূর্ণাঙ্ক weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                যদি (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, সঁচা);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                যদি (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.গুনতি(vscp))
                    addEdge(vvtxp, lvtxp, weight, সঁচা);
            });
        }
    }

    ঘূৰাই graphp;
}

শূন্য removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    ধ্রুৱ auto enqueue = [&](V3GraphVertex* vtxp) {
        যদি (vtxp->user()) ঘূৰাই;  // Already in queue
        vtxp->user(1);
        queue.ঠেলো_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    বাবে (V3GraphVertex& vtx : graphp->vertices()) {
        যদি (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    যেতিয়ালৈ (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* ধ্রুৱ vtxp = queue.back();
        queue.বেৰ_কৰো_back();
        vtxp->user(0);  // No longer in queue

        যদি (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            বাবে (V3GraphEdge* ধ্রুৱ edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->শীৰ্ষ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } অন্যথা যদি (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            বাবে (V3GraphEdge* ধ্রুৱ edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
তার্কিক isCut(ধ্রুৱ SchedAcyclicVarVertex* vtxp) {
    বাবে (ধ্রুৱ V3GraphEdge& edge : vtxp->inEdges()) {
        যদি (edge.weight() == 0) ঘূৰাই সঁচা;
    }
    বাবে (ধ্রুৱ V3GraphEdge& edge : vtxp->outEdges()) {
        যদি (edge.weight() == 0) ঘূৰাই সঁচা;
    }
    ঘূৰাই মিছা;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    ধ্রুৱ VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    বাবে (V3GraphVertex& vtx : graphp->vertices()) {
        যদি (!vtx.color()) অনুৱর্ত;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        যদি (SchedAcyclicVarVertex* ধ্রুৱ vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            যদি (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.ঠেলো_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        যদি (v3Global.opt.stats()) sccs.ঢোকাও(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    ঘূৰাই result;
}

শূন্য resetEdgeWeights(ধ্রুৱ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    বাবে (SchedAcyclicVarVertex* ধ্রুৱ vvtxp : cutVertices) {
        বাবে (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        বাবে (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
শূন্য gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    যদি (vtxp->user()) ঘূৰাই;  // Already done
    vtxp->user(সঁচা);

    যদি (SchedAcyclicVarVertex* ধ্রুৱ vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* ধ্রুৱ varp = vvtxp->varp();
        ধ্রুৱ string name = varp->prettyName();
        যদি (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ধ্রুৱ unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    বাবে (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* ধ্রুৱ শীৰ্ষ = edge.শীৰ্ষ();
        যদি (শীৰ্ষ->color() == vtxp->color()) gatherSCCCandidates(শীৰ্ষ, candidates);
    }
    বাবে (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* ধ্রুৱ fromp = edge.fromp();
        যদি (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_সাৰি* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ধ্রুৱ VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    যদি (candidates.empty()) ঘূৰাই "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    ধ্রুৱ auto reportFirst10
        = [&](std::function<তার্কিক(ধ্রুৱ Candidate&, ধ্রুৱ Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        বাবে (size_t i = 0; i < 10; i++) {
            যদি (i == candidates.size()) ভঙ্গ;
            ধ্রুৱ Candidate& candidate = candidates[i];
            AstVar* ধ্রুৱ varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            যদি (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        ঘূৰাই ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ধ্রুৱ Candidate& a, ধ্রুৱ Candidate& b) {
              ঘূৰাই a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ধ্রুৱ Candidate& a, ধ্রুৱ Candidate& b) {  //
              ঘূৰাই a.second > b.second;
          });

    যদি (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    ঘূৰাই ss.str();
}

শূন্য reportCycles(Graph* graphp, ধ্রুৱ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    বাবে (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* ধ্রুৱ vscp = vvtxp->vscp();
        File_সাৰি* ধ্রুৱ flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        যদি (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } অন্যথা {
            vscp->v3warn(UNOPTFLAT,
                         "Signal unoptimizable: Circular combinational logic: "
                             << vscp->prettyNameQ() << '\n'
                             << vscp->warnContextPrimary()
                             << V3Error::warnAdditionalInfo()
                             // Calls Graph::loopsVertexCb
                             << graphp->reportLoops(&V3GraphEdge::followAlwaysTrue, vvtxp)
                             // Report candidate variables for splitting
                             << (v3Global.opt.reportUnoptflat()
                                     ? reportLoopVars(vscp->fileline(), graphp, vvtxp)
                                     : ""));
            // Complain just once
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, সঁচা);
            // Create a subgraph for the UNOPTFLAT loop
            যদি (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

শূন্য dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    বাবে (V3GraphVertex& vtx : graphp->vertices()) {
        যদি (!vtx.color()) অনুৱর্ত;
        scc2Vtxps[vtx.color()].ঠেলো_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    বাবে (ধ্রুৱ auto& pair : scc2Vtxps) {
        ধ্রুৱ uint32_t color = pair.first;
        ধ্রুৱ std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        ধ্রুৱ std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        ধ্রুৱ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        যদি (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        বাবে (V3GraphVertex* vtxp : vtxps) {
            ধ্রুৱ SchedAcyclicVarVertex* ধ্রুৱ vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            যদি (!vvtxp) অনুৱর্ত;
            AstVarScope* ধ্রুৱ vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_বৃক্ষ(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        বাবে (V3GraphVertex* vtxp : vtxps) {
            ধ্রুৱ SchedAcyclicLogicVertex* ধ্রুৱ lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            যদি (!lvtxp) অনুৱর্ত;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_বৃক্ষ(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ধ্রুৱ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        ধ্রুৱ VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        বাবে (SchedAcyclicVarVertex* ধ্রুৱ vvtxp : cutVertices) {
            বাবে (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* ধ্রুৱ lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.শীৰ্ষ());
                যদি (!lvtxp->logicp()->user1SetOnce()) lvtxps.ঠেলো_back(lvtxp);
                lvtx2Cuts[lvtxp].ঠেলো_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_বৃক্ষ_Finder finder{netlistp};
    বাবে (SchedAcyclicLogicVertex* ধ্রুৱ lvtxp : lvtxps) {
        Ast_গ্ৰন্থি* ধ্রুৱ logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_সাৰি* ধ্রুৱ flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        বাবে (AstVarScope* ধ্রুৱ vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ধ্রুৱ refp = নূতন AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ধ্রুৱ nextp = নূতন AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_গ্ৰন্থি::addNext(senItemsp, nextp);
        }
        Ast_Sen_বৃক্ষ* ধ্রুৱ sen_বৃক্ষ = নূতন Ast_Sen_বৃক্ষ{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_বৃক্ষ(sen_বৃক্ষ), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_বৃক্ষ->delete_বৃক্ষ(), sen_বৃক্ষ);
    }
    ঘূৰাই result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, ধ্রুৱ LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ধ্রুৱ std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    যদি (graphp->empty()) ঘূৰাই LogicByScope{};

    // Dump for debug
    যদি (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    ধ্রুৱ std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    যদি (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    ঘূৰাই fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
