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

ವರ್ಗ SchedAcyclicLogicVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_ಗಂಟು* ಧ್ರುವ m_logicp;  // The logic node this vertex represents
    AstScope* ಧ್ರುವ m_scopep;  // The enclosing AstScope of the logic node

ಸಾರ್ವಜನಿಕ:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_ಗಂಟು* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_ಗಂಟು* logicp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_logicp; }
    AstScope* scopep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_logicp->fileline()->ascii(); };
    string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "rectangle2"; }
    // LCOV_EXCL_STOP
};

ವರ್ಗ SchedAcyclicVarVertex final : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* ಧ್ರುವ m_vscp;  // The AstVarScope this vertex represents

ಸಾರ್ವಜನಿಕ:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp; }
    AstVar* varp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_vscp->name(); }
    string dotShape() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "ellipse"; }
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "blue"; }
    // LCOV_EXCL_STOP
};

ವರ್ಗ Graph final : ಸಾರ್ವಜನಿಕ V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        ಯದಿ (SchedAcyclicLogicVertex* ಧ್ರುವ lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_ಗಂಟು* ಧ್ರುವ logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            ಯದಿ (ಧ್ರುವ AstAlways* ಧ್ರುವ alwaysp = VN_CAST(logicp, Always)) {
                ಯದಿ (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            ಪ್ರತ್ಯಾಗಮ logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } ಅನ್ಯಥಾ {
            SchedAcyclicVarVertex* ಧ್ರುವ vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* ಧ್ರುವ vscp = vvtxp->vscp();
            ಪ್ರತ್ಯಾಗಮ vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ಧ್ರುವ LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{ನೂತನ Graph};

    // AstVarScope::user1() -> VarVertx
    ಧ್ರುವ VNUser1InUse user1InUse;
    ಧ್ರುವ auto getVarVertex = [&](AstVarScope* vscp) {
        ಯದಿ (!vscp->user1p()) vscp->user1p(ನೂತನ SchedAcyclicVarVertex{graphp.get(), vscp});
        ಪ್ರತ್ಯಾಗಮ vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    ಧ್ರುವ auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* ತುದಿ, ಪೂರ್ಣಾಙ್ಕ weight, ತಾರ್ಕಿಕ cuttable) {
        ನೂತನ V3GraphEdge{graphp.get(), fromp, ತುದಿ, weight, cuttable};
    };

    ಪ್ರತಿ (ಧ್ರುವ auto& entry : lbs) {
        AstScope* ಧ್ರುವ scopep = entry.m_scopep;
        AstActive* ಧ್ರುವ activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        ಪ್ರತಿ (Ast_ಗಂಟು* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            ಯದಿ (VN_IS(nodep, AlwaysPostponed)) ಅನುವರ್ತ;

            SchedAcyclicLogicVertex* ಧ್ರುವ lvtxp
                = ನೂತನ SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            ಧ್ರುವ VNUser2InUse user2InUse;
            ಧ್ರುವ VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
                SchedAcyclicVarVertex* ಧ್ರುವ vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                ಯದಿ (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, ಸತ್ಯ);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                ಯದಿ (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.ಎಣಿಕೆ(vscp))
                    addEdge(vvtxp, lvtxp, weight, ಸತ್ಯ);
            });
        }
    }

    ಪ್ರತ್ಯಾಗಮ graphp;
}

ಶೂನ್ಯ removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    ಧ್ರುವ auto enqueue = [&](V3GraphVertex* vtxp) {
        ಯದಿ (vtxp->user()) ಪ್ರತ್ಯಾಗಮ;  // Already in queue
        vtxp->user(1);
        queue.ತಳ್ಳು_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    ಪ್ರತಿ (V3GraphVertex& vtx : graphp->vertices()) {
        ಯದಿ (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    ಯಾವತ್ (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* ಧ್ರುವ vtxp = queue.back();
        queue.ತೆಗೆ_back();
        vtxp->user(0);  // No longer in queue

        ಯದಿ (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->ತುದಿ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } ಅನ್ಯಥಾ ಯದಿ (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            ಪ್ರತಿ (V3GraphEdge* ಧ್ರುವ edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
ತಾರ್ಕಿಕ isCut(ಧ್ರುವ SchedAcyclicVarVertex* vtxp) {
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vtxp->inEdges()) {
        ಯದಿ (edge.weight() == 0) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತಿ (ಧ್ರುವ V3GraphEdge& edge : vtxp->outEdges()) {
        ಯದಿ (edge.weight() == 0) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    ಧ್ರುವ VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    ಪ್ರತಿ (V3GraphVertex& vtx : graphp->vertices()) {
        ಯದಿ (!vtx.color()) ಅನುವರ್ತ;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        ಯದಿ (SchedAcyclicVarVertex* ಧ್ರುವ vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            ಯದಿ (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.ತಳ್ಳು_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        ಯದಿ (v3Global.opt.stats()) sccs.ಸೇರಿಸು(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    ಪ್ರತ್ಯಾಗಮ result;
}

ಶೂನ್ಯ resetEdgeWeights(ಧ್ರುವ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    ಪ್ರತಿ (SchedAcyclicVarVertex* ಧ್ರುವ vvtxp : cutVertices) {
        ಪ್ರತಿ (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        ಪ್ರತಿ (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
ಶೂನ್ಯ gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    ಯದಿ (vtxp->user()) ಪ್ರತ್ಯಾಗಮ;  // Already done
    vtxp->user(ಸತ್ಯ);

    ಯದಿ (SchedAcyclicVarVertex* ಧ್ರುವ vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* ಧ್ರುವ varp = vvtxp->varp();
        ಧ್ರುವ string name = varp->prettyName();
        ಯದಿ (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ಧ್ರುವ unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    ಪ್ರತಿ (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* ಧ್ರುವ ತುದಿ = edge.ತುದಿ();
        ಯದಿ (ತುದಿ->color() == vtxp->color()) gatherSCCCandidates(ತುದಿ, candidates);
    }
    ಪ್ರತಿ (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* ಧ್ರುವ fromp = edge.fromp();
        ಯದಿ (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_ಸಾಲು* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ಧ್ರುವ VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    ಯದಿ (candidates.empty()) ಪ್ರತ್ಯಾಗಮ "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    ಧ್ರುವ auto reportFirst10
        = [&](std::function<ತಾರ್ಕಿಕ(ಧ್ರುವ Candidate&, ಧ್ರುವ Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        ಪ್ರತಿ (size_t i = 0; i < 10; i++) {
            ಯದಿ (i == candidates.size()) ಭಙ್ಗ;
            ಧ್ರುವ Candidate& candidate = candidates[i];
            AstVar* ಧ್ರುವ varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            ಯದಿ (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        ಪ್ರತ್ಯಾಗಮ ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ಧ್ರುವ Candidate& a, ಧ್ರುವ Candidate& b) {
              ಪ್ರತ್ಯಾಗಮ a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ಧ್ರುವ Candidate& a, ಧ್ರುವ Candidate& b) {  //
              ಪ್ರತ್ಯಾಗಮ a.second > b.second;
          });

    ಯದಿ (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    ಪ್ರತ್ಯಾಗಮ ss.str();
}

ಶೂನ್ಯ reportCycles(Graph* graphp, ಧ್ರುವ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    ಪ್ರತಿ (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* ಧ್ರುವ vscp = vvtxp->vscp();
        File_ಸಾಲು* ಧ್ರುವ flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        ಯದಿ (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } ಅನ್ಯಥಾ {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, ಸತ್ಯ);
            // Create a subgraph for the UNOPTFLAT loop
            ಯದಿ (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

ಶೂನ್ಯ dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    ಪ್ರತಿ (V3GraphVertex& vtx : graphp->vertices()) {
        ಯದಿ (!vtx.color()) ಅನುವರ್ತ;
        scc2Vtxps[vtx.color()].ತಳ್ಳು_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    ಪ್ರತಿ (ಧ್ರುವ auto& pair : scc2Vtxps) {
        ಧ್ರುವ uint32_t color = pair.first;
        ಧ್ರುವ std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        ಧ್ರುವ std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        ಧ್ರುವ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        ಯದಿ (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        ಪ್ರತಿ (V3GraphVertex* vtxp : vtxps) {
            ಧ್ರುವ SchedAcyclicVarVertex* ಧ್ರುವ vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            ಯದಿ (!vvtxp) ಅನುವರ್ತ;
            AstVarScope* ಧ್ರುವ vscp = vvtxp->vscp();
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
        ಪ್ರತಿ (V3GraphVertex* vtxp : vtxps) {
            ಧ್ರುವ SchedAcyclicLogicVertex* ಧ್ರುವ lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            ಯದಿ (!lvtxp) ಅನುವರ್ತ;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_ವೃಕ್ಷ(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ಧ್ರುವ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        ಧ್ರುವ VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        ಪ್ರತಿ (SchedAcyclicVarVertex* ಧ್ರುವ vvtxp : cutVertices) {
            ಪ್ರತಿ (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* ಧ್ರುವ lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.ತುದಿ());
                ಯದಿ (!lvtxp->logicp()->user1SetOnce()) lvtxps.ತಳ್ಳು_back(lvtxp);
                lvtx2Cuts[lvtxp].ತಳ್ಳು_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_ವೃಕ್ಷ_Finder finder{netlistp};
    ಪ್ರತಿ (SchedAcyclicLogicVertex* ಧ್ರುವ lvtxp : lvtxps) {
        Ast_ಗಂಟು* ಧ್ರುವ logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_ಸಾಲು* ಧ್ರುವ flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        ಪ್ರತಿ (AstVarScope* ಧ್ರುವ vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ಧ್ರುವ nextp = ನೂತನ AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_ಗಂಟು::addNext(senItemsp, nextp);
        }
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ sen_ವೃಕ್ಷ = ನೂತನ Ast_Sen_ವೃಕ್ಷ{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_ವೃಕ್ಷ(sen_ವೃಕ್ಷ), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_ವೃಕ್ಷ->delete_ವೃಕ್ಷ(), sen_ವೃಕ್ಷ);
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, ಧ್ರುವ LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ಧ್ರುವ std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    ಯದಿ (graphp->empty()) ಪ್ರತ್ಯಾಗಮ LogicByScope{};

    // Dump for debug
    ಯದಿ (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    ಧ್ರುವ std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    ಯದಿ (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    ಪ್ರತ್ಯಾಗಮ fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
