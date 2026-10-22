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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3EmitV.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3Sched.h"
#અન્તર્ભાવ "V3SenTree.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <utility>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3Sched {

નામાકાશ {

// ##############################################################################
//  Data structures (graph types)

વર્ગ SchedAcyclicLogicVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_ગાંઠ* અચલ m_logicp;  // The logic node this vertex represents
    AstScope* અચલ m_scopep;  // The enclosing AstScope of the logic node

જાહેર:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_ગાંઠ* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) અચલ override {
        પાછા નવું SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_ગાંઠ* logicp() અચલ { પાછા m_logicp; }
    AstScope* scopep() અચલ { પાછા m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() અચલ override VL_MT_STABLE { પાછા m_logicp->fileline()->ascii(); };
    string dotShape() અચલ override { પાછા "rectangle2"; }
    // LCOV_EXCL_STOP
};

વર્ગ SchedAcyclicVarVertex final : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* અચલ m_vscp;  // The AstVarScope this vertex represents

જાહેર:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() અચલ { પાછા m_vscp; }
    AstVar* varp() અચલ { પાછા m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) અચલ override {
        પાછા નવું SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() અચલ override VL_MT_STABLE { પાછા m_vscp->name(); }
    string dotShape() અચલ override { પાછા "ellipse"; }
    string dotColor() અચલ override { પાછા "blue"; }
    // LCOV_EXCL_STOP
};

વર્ગ Graph final : જાહેર V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        જો (SchedAcyclicLogicVertex* અચલ lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_ગાંઠ* અચલ logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            જો (અચલ AstAlways* અચલ alwaysp = VN_CAST(logicp, Always)) {
                જો (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            પાછા logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } નહીંતર {
            SchedAcyclicVarVertex* અચલ vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* અચલ vscp = vvtxp->vscp();
            પાછા vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(અચલ LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{નવું Graph};

    // AstVarScope::user1() -> VarVertx
    અચલ VNUser1InUse user1InUse;
    અચલ auto getVarVertex = [&](AstVarScope* vscp) {
        જો (!vscp->user1p()) vscp->user1p(નવું SchedAcyclicVarVertex{graphp.get(), vscp});
        પાછા vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    અચલ auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* ટોચ, પૂર્ણાંક weight, તાર્કિક cuttable) {
        નવું V3GraphEdge{graphp.get(), fromp, ટોચ, weight, cuttable};
    };

    માટે (અચલ auto& entry : lbs) {
        AstScope* અચલ scopep = entry.m_scopep;
        AstActive* અચલ activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        માટે (Ast_ગાંઠ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            જો (VN_IS(nodep, AlwaysPostponed)) ચાલુ;

            SchedAcyclicLogicVertex* અચલ lvtxp
                = નવું SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            અચલ VNUser2InUse user2InUse;
            અચલ VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* અચલ vscp = refp->varScopep();
                SchedAcyclicVarVertex* અચલ vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                અચલ પૂર્ણાંક weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                જો (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, સાચું);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                જો (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.ગણતરી(vscp))
                    addEdge(vvtxp, lvtxp, weight, સાચું);
            });
        }
    }

    પાછા graphp;
}

શૂન્ય removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    અચલ auto enqueue = [&](V3GraphVertex* vtxp) {
        જો (vtxp->user()) પાછા;  // Already in queue
        vtxp->user(1);
        queue.ધકેલો_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    માટે (V3GraphVertex& vtx : graphp->vertices()) {
        જો (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    જ્યારે (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* અચલ vtxp = queue.back();
        queue.કાઢો_back();
        vtxp->user(0);  // No longer in queue

        જો (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            માટે (V3GraphEdge* અચલ edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->ટોચ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } નહીંતર જો (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            માટે (V3GraphEdge* અચલ edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
તાર્કિક isCut(અચલ SchedAcyclicVarVertex* vtxp) {
    માટે (અચલ V3GraphEdge& edge : vtxp->inEdges()) {
        જો (edge.weight() == 0) પાછા સાચું;
    }
    માટે (અચલ V3GraphEdge& edge : vtxp->outEdges()) {
        જો (edge.weight() == 0) પાછા સાચું;
    }
    પાછા ખોટું;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    અચલ VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    માટે (V3GraphVertex& vtx : graphp->vertices()) {
        જો (!vtx.color()) ચાલુ;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        જો (SchedAcyclicVarVertex* અચલ vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            જો (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.ધકેલો_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        જો (v3Global.opt.stats()) sccs.ઉમેરો(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    પાછા result;
}

શૂન્ય resetEdgeWeights(અચલ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    માટે (SchedAcyclicVarVertex* અચલ vvtxp : cutVertices) {
        માટે (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        માટે (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
શૂન્ય gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    જો (vtxp->user()) પાછા;  // Already done
    vtxp->user(સાચું);

    જો (SchedAcyclicVarVertex* અચલ vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* અચલ varp = vvtxp->varp();
        અચલ string name = varp->prettyName();
        જો (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            અચલ unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    માટે (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* અચલ ટોચ = edge.ટોચ();
        જો (ટોચ->color() == vtxp->color()) gatherSCCCandidates(ટોચ, candidates);
    }
    માટે (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* અચલ fromp = edge.fromp();
        જો (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_લીટી* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        અચલ VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    જો (candidates.empty()) પાછા "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    અચલ auto reportFirst10
        = [&](std::function<તાર્કિક(અચલ Candidate&, અચલ Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        માટે (size_t i = 0; i < 10; i++) {
            જો (i == candidates.size()) તોડો;
            અચલ Candidate& candidate = candidates[i];
            AstVar* અચલ varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            જો (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        પાછા ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](અચલ Candidate& a, અચલ Candidate& b) {
              પાછા a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](અચલ Candidate& a, અચલ Candidate& b) {  //
              પાછા a.second > b.second;
          });

    જો (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    પાછા ss.str();
}

શૂન્ય reportCycles(Graph* graphp, અચલ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    માટે (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* અચલ vscp = vvtxp->vscp();
        File_લીટી* અચલ flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        જો (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } નહીંતર {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, સાચું);
            // Create a subgraph for the UNOPTFLAT loop
            જો (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

શૂન્ય dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    માટે (V3GraphVertex& vtx : graphp->vertices()) {
        જો (!vtx.color()) ચાલુ;
        scc2Vtxps[vtx.color()].ધકેલો_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    માટે (અચલ auto& pair : scc2Vtxps) {
        અચલ uint32_t color = pair.first;
        અચલ std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        અચલ std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        અચલ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        જો (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        માટે (V3GraphVertex* vtxp : vtxps) {
            અચલ SchedAcyclicVarVertex* અચલ vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            જો (!vvtxp) ચાલુ;
            AstVarScope* અચલ vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_વૃક્ષ(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        માટે (V3GraphVertex* vtxp : vtxps) {
            અચલ SchedAcyclicLogicVertex* અચલ lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            જો (!lvtxp) ચાલુ;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_વૃક્ષ(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     અચલ std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        અચલ VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        માટે (SchedAcyclicVarVertex* અચલ vvtxp : cutVertices) {
            માટે (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* અચલ lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.ટોચ());
                જો (!lvtxp->logicp()->user1SetOnce()) lvtxps.ધકેલો_back(lvtxp);
                lvtx2Cuts[lvtxp].ધકેલો_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_વૃક્ષ_Finder finder{netlistp};
    માટે (SchedAcyclicLogicVertex* અચલ lvtxp : lvtxps) {
        Ast_ગાંઠ* અચલ logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_લીટી* અચલ flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        માટે (AstVarScope* અચલ vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* અચલ refp = નવું AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* અચલ nextp = નવું AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_ગાંઠ::addNext(senItemsp, nextp);
        }
        Ast_Sen_વૃક્ષ* અચલ sen_વૃક્ષ = નવું Ast_Sen_વૃક્ષ{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_વૃક્ષ(sen_વૃક્ષ), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_વૃક્ષ->delete_વૃક્ષ(), sen_વૃક્ષ);
    }
    પાછા result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, અચલ LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    અચલ std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    જો (graphp->empty()) પાછા LogicByScope{};

    // Dump for debug
    જો (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    અચલ std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    જો (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    પાછા fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
