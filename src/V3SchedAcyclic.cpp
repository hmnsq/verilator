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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3EmitV.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3Sched.h"
#ଅନ୍ତର୍ଭାଵ "V3SenTree.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3Sched {

ନାମାକାଶ {

// ##############################################################################
//  Data structures (graph types)

ଶ୍ରେଣୀ SchedAcyclicLogicVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_ଗଣ୍ଠି* ଅଚଳ m_logicp;  // The logic node this vertex represents
    AstScope* ଅଚଳ m_scopep;  // The enclosing AstScope of the logic node

ସର୍ବସାଧାରଣ:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_ଗଣ୍ଠି* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) ଅଚଳ override {
        ଫେରନ୍ତୁ ନୂତନ SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_ଗଣ୍ଠି* logicp() ଅଚଳ { ଫେରନ୍ତୁ m_logicp; }
    AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_logicp->fileline()->ascii(); };
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "rectangle2"; }
    // LCOV_EXCL_STOP
};

ଶ୍ରେଣୀ SchedAcyclicVarVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* ଅଚଳ m_vscp;  // The AstVarScope this vertex represents

ସର୍ବସାଧାରଣ:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ଅଚଳ { ଫେରନ୍ତୁ m_vscp; }
    AstVar* varp() ଅଚଳ { ଫେରନ୍ତୁ m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) ଅଚଳ override {
        ଫେରନ୍ତୁ ନୂତନ SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_vscp->name(); }
    string dotShape() ଅଚଳ override { ଫେରନ୍ତୁ "ellipse"; }
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "blue"; }
    // LCOV_EXCL_STOP
};

ଶ୍ରେଣୀ Graph final : ସର୍ବସାଧାରଣ V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        ଯଦି (SchedAcyclicLogicVertex* ଅଚଳ lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_ଗଣ୍ଠି* ଅଚଳ logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            ଯଦି (ଅଚଳ AstAlways* ଅଚଳ alwaysp = VN_CAST(logicp, Always)) {
                ଯଦି (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            ଫେରନ୍ତୁ logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } ଅନ୍ୟଥା {
            SchedAcyclicVarVertex* ଅଚଳ vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* ଅଚଳ vscp = vvtxp->vscp();
            ଫେରନ୍ତୁ vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ଅଚଳ LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{ନୂତନ Graph};

    // AstVarScope::user1() -> VarVertx
    ଅଚଳ VNUser1InUse user1InUse;
    ଅଚଳ auto getVarVertex = [&](AstVarScope* vscp) {
        ଯଦି (!vscp->user1p()) vscp->user1p(ନୂତନ SchedAcyclicVarVertex{graphp.get(), vscp});
        ଫେରନ୍ତୁ vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    ଅଚଳ auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight, ତାର୍କିକ cuttable) {
        ନୂତନ V3GraphEdge{graphp.get(), fromp, ଶୀର୍ଷ, weight, cuttable};
    };

    ପାଇଁ (ଅଚଳ auto& entry : lbs) {
        AstScope* ଅଚଳ scopep = entry.m_scopep;
        AstActive* ଅଚଳ activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            ଯଦି (VN_IS(nodep, AlwaysPostponed)) ଜାରି;

            SchedAcyclicLogicVertex* ଅଚଳ lvtxp
                = ନୂତନ SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            ଅଚଳ VNUser2InUse user2InUse;
            ଅଚଳ VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ଅଚଳ vscp = refp->varScopep();
                SchedAcyclicVarVertex* ଅଚଳ vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                ଯଦି (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, ସତ୍ୟ);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                ଯଦି (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.ଗଣନା_2(vscp))
                    addEdge(vvtxp, lvtxp, weight, ସତ୍ୟ);
            });
        }
    }

    ଫେରନ୍ତୁ graphp;
}

ଶୂନ୍ୟ removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    ଅଚଳ auto enqueue = [&](V3GraphVertex* vtxp) {
        ଯଦି (vtxp->user()) ଫେରନ୍ତୁ;  // Already in queue
        vtxp->user(1);
        queue.ଠେଲ_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    ପାଇଁ (V3GraphVertex& vtx : graphp->vertices()) {
        ଯଦି (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    ଯେତେବେଳେ (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* ଅଚଳ vtxp = queue.back();
        queue.କାଢ଼_back();
        vtxp->user(0);  // No longer in queue

        ଯଦି (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->ଶୀର୍ଷ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } ଅନ୍ୟଥା ଯଦି (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
ତାର୍କିକ isCut(ଅଚଳ SchedAcyclicVarVertex* vtxp) {
    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vtxp->inEdges()) {
        ଯଦି (edge.weight() == 0) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ପାଇଁ (ଅଚଳ V3GraphEdge& edge : vtxp->outEdges()) {
        ଯଦି (edge.weight() == 0) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    ଅଚଳ VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    ପାଇଁ (V3GraphVertex& vtx : graphp->vertices()) {
        ଯଦି (!vtx.color()) ଜାରି;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        ଯଦି (SchedAcyclicVarVertex* ଅଚଳ vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            ଯଦି (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.ଠେଲ_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        ଯଦି (v3Global.opt.stats()) sccs.ଭର୍ତ୍ତି(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    ଫେରନ୍ତୁ result;
}

ଶୂନ୍ୟ resetEdgeWeights(ଅଚଳ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    ପାଇଁ (SchedAcyclicVarVertex* ଅଚଳ vvtxp : cutVertices) {
        ପାଇଁ (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        ପାଇଁ (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
ଶୂନ୍ୟ gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    ଯଦି (vtxp->user()) ଫେରନ୍ତୁ;  // Already done
    vtxp->user(ସତ୍ୟ);

    ଯଦି (SchedAcyclicVarVertex* ଅଚଳ vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* ଅଚଳ varp = vvtxp->varp();
        ଅଚଳ string name = varp->prettyName();
        ଯଦି (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ଅଚଳ unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    ପାଇଁ (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* ଅଚଳ ଶୀର୍ଷ = edge.ଶୀର୍ଷ();
        ଯଦି (ଶୀର୍ଷ->color() == vtxp->color()) gatherSCCCandidates(ଶୀର୍ଷ, candidates);
    }
    ପାଇଁ (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* ଅଚଳ fromp = edge.fromp();
        ଯଦି (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_ଧାଡ଼ି* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ଅଚଳ VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    ଯଦି (candidates.empty()) ଫେରନ୍ତୁ "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    ଅଚଳ auto reportFirst10
        = [&](std::function<ତାର୍କିକ(ଅଚଳ Candidate&, ଅଚଳ Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        ପାଇଁ (size_t i = 0; i < 10; i++) {
            ଯଦି (i == candidates.size()) ଭାଙ୍ଗ;
            ଅଚଳ Candidate& candidate = candidates[i];
            AstVar* ଅଚଳ varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            ଯଦି (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        ଫେରନ୍ତୁ ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ଅଚଳ Candidate& a, ଅଚଳ Candidate& b) {
              ଫେରନ୍ତୁ a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ଅଚଳ Candidate& a, ଅଚଳ Candidate& b) {  //
              ଫେରନ୍ତୁ a.second > b.second;
          });

    ଯଦି (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    ଫେରନ୍ତୁ ss.str();
}

ଶୂନ୍ୟ reportCycles(Graph* graphp, ଅଚଳ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    ପାଇଁ (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* ଅଚଳ vscp = vvtxp->vscp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        ଯଦି (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } ଅନ୍ୟଥା {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, ସତ୍ୟ);
            // Create a subgraph for the UNOPTFLAT loop
            ଯଦି (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

ଶୂନ୍ୟ dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    ପାଇଁ (V3GraphVertex& vtx : graphp->vertices()) {
        ଯଦି (!vtx.color()) ଜାରି;
        scc2Vtxps[vtx.color()].ଠେଲ_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    ପାଇଁ (ଅଚଳ auto& pair : scc2Vtxps) {
        ଅଚଳ uint32_t color = pair.first;
        ଅଚଳ std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        ଅଚଳ std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        ଅଚଳ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        ଯଦି (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        ପାଇଁ (V3GraphVertex* vtxp : vtxps) {
            ଅଚଳ SchedAcyclicVarVertex* ଅଚଳ vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            ଯଦି (!vvtxp) ଜାରି;
            AstVarScope* ଅଚଳ vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        ପାଇଁ (V3GraphVertex* vtxp : vtxps) {
            ଅଚଳ SchedAcyclicLogicVertex* ଅଚଳ lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            ଯଦି (!lvtxp) ଜାରି;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_ବୃକ୍ଷ(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ଅଚଳ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        ଅଚଳ VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        ପାଇଁ (SchedAcyclicVarVertex* ଅଚଳ vvtxp : cutVertices) {
            ପାଇଁ (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* ଅଚଳ lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.ଶୀର୍ଷ());
                ଯଦି (!lvtxp->logicp()->user1SetOnce()) lvtxps.ଠେଲ_back(lvtxp);
                lvtx2Cuts[lvtxp].ଠେଲ_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_ବୃକ୍ଷ_Finder finder{netlistp};
    ପାଇଁ (SchedAcyclicLogicVertex* ଅଚଳ lvtxp : lvtxps) {
        Ast_ଗଣ୍ଠି* ଅଚଳ logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_ଧାଡ଼ି* ଅଚଳ flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        ପାଇଁ (AstVarScope* ଅଚଳ vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ଅଚଳ nextp = ନୂତନ AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_ଗଣ୍ଠି::addNext(senItemsp, nextp);
        }
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ sen_ବୃକ୍ଷ = ନୂତନ Ast_Sen_ବୃକ୍ଷ{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_ବୃକ୍ଷ(sen_ବୃକ୍ଷ), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_ବୃକ୍ଷ->delete_ବୃକ୍ଷ(), sen_ବୃକ୍ଷ);
    }
    ଫେରନ୍ତୁ result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, ଅଚଳ LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ଅଚଳ std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    ଯଦି (graphp->empty()) ଫେରନ୍ତୁ LogicByScope{};

    // Dump for debug
    ଯଦି (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    ଅଚଳ std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    ଯଦି (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    ଫେରନ୍ତୁ fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
