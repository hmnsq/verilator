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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3EmitV.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3Sched.h"
#அந்தர்பாவ "V3SenTree.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3Sched {

நாமாகாஶ {

// ##############################################################################
//  Data structures (graph types)

வகுப்பு SchedAcyclicLogicVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_முனை* மாறிலி m_logicp;  // The logic node this vertex represents
    AstScope* மாறிலி m_scopep;  // The enclosing AstScope of the logic node

பொது:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_முனை* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) மாறிலி override {
        திரும்பு புதிய SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_முனை* logicp() மாறிலி { திரும்பு m_logicp; }
    AstScope* scopep() மாறிலி { திரும்பு m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_logicp->fileline()->ascii(); };
    string dotShape() மாறிலி override { திரும்பு "rectangle2"; }
    // LCOV_EXCL_STOP
};

வகுப்பு SchedAcyclicVarVertex final : பொது V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* மாறிலி m_vscp;  // The AstVarScope this vertex represents

பொது:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() மாறிலி { திரும்பு m_vscp; }
    AstVar* varp() மாறிலி { திரும்பு m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) மாறிலி override {
        திரும்பு புதிய SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() மாறிலி override VL_MT_STABLE { திரும்பு m_vscp->name(); }
    string dotShape() மாறிலி override { திரும்பு "ellipse"; }
    string dotColor() மாறிலி override { திரும்பு "blue"; }
    // LCOV_EXCL_STOP
};

வகுப்பு Graph final : பொது V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        எனில் (SchedAcyclicLogicVertex* மாறிலி lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_முனை* மாறிலி logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            எனில் (மாறிலி AstAlways* மாறிலி alwaysp = VN_CAST(logicp, Always)) {
                எனில் (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            திரும்பு logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } இல்லையெனில் {
            SchedAcyclicVarVertex* மாறிலி vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* மாறிலி vscp = vvtxp->vscp();
            திரும்பு vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(மாறிலி LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{புதிய Graph};

    // AstVarScope::user1() -> VarVertx
    மாறிலி VNUser1InUse user1InUse;
    மாறிலி auto getVarVertex = [&](AstVarScope* vscp) {
        எனில் (!vscp->user1p()) vscp->user1p(புதிய SchedAcyclicVarVertex{graphp.get(), vscp});
        திரும்பு vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    மாறிலி auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* உச்சி, முழு weight, தருக்க cuttable) {
        புதிய V3GraphEdge{graphp.get(), fromp, உச்சி, weight, cuttable};
    };

    க்கு (மாறிலி auto& entry : lbs) {
        AstScope* மாறிலி scopep = entry.m_scopep;
        AstActive* மாறிலி activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        க்கு (Ast_முனை* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            எனில் (VN_IS(nodep, AlwaysPostponed)) தொடர்;

            SchedAcyclicLogicVertex* மாறிலி lvtxp
                = புதிய SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            மாறிலி VNUser2InUse user2InUse;
            மாறிலி VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* மாறிலி vscp = refp->varScopep();
                SchedAcyclicVarVertex* மாறிலி vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                மாறிலி முழு weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                எனில் (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, உண்மை);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                எனில் (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.மொத்தம்(vscp))
                    addEdge(vvtxp, lvtxp, weight, உண்மை);
            });
        }
    }

    திரும்பு graphp;
}

வெறுமை removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    மாறிலி auto enqueue = [&](V3GraphVertex* vtxp) {
        எனில் (vtxp->user()) திரும்பு;  // Already in queue
        vtxp->user(1);
        queue.தள்ளு_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    க்கு (V3GraphVertex& vtx : graphp->vertices()) {
        எனில் (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    போது (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* மாறிலி vtxp = queue.back();
        queue.எடு_back();
        vtxp->user(0);  // No longer in queue

        எனில் (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            க்கு (V3GraphEdge* மாறிலி edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->உச்சி());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } இல்லையெனில் எனில் (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            க்கு (V3GraphEdge* மாறிலி edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
தருக்க isCut(மாறிலி SchedAcyclicVarVertex* vtxp) {
    க்கு (மாறிலி V3GraphEdge& edge : vtxp->inEdges()) {
        எனில் (edge.weight() == 0) திரும்பு உண்மை;
    }
    க்கு (மாறிலி V3GraphEdge& edge : vtxp->outEdges()) {
        எனில் (edge.weight() == 0) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    மாறிலி VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    க்கு (V3GraphVertex& vtx : graphp->vertices()) {
        எனில் (!vtx.color()) தொடர்;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        எனில் (SchedAcyclicVarVertex* மாறிலி vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            எனில் (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.தள்ளு_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        எனில் (v3Global.opt.stats()) sccs.செருகு(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    திரும்பு result;
}

வெறுமை resetEdgeWeights(மாறிலி std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    க்கு (SchedAcyclicVarVertex* மாறிலி vvtxp : cutVertices) {
        க்கு (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        க்கு (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
வெறுமை gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    எனில் (vtxp->user()) திரும்பு;  // Already done
    vtxp->user(உண்மை);

    எனில் (SchedAcyclicVarVertex* மாறிலி vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* மாறிலி varp = vvtxp->varp();
        மாறிலி string name = varp->prettyName();
        எனில் (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            மாறிலி unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    க்கு (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* மாறிலி உச்சி = edge.உச்சி();
        எனில் (உச்சி->color() == vtxp->color()) gatherSCCCandidates(உச்சி, candidates);
    }
    க்கு (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* மாறிலி fromp = edge.fromp();
        எனில் (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_வரி* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        மாறிலி VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    எனில் (candidates.empty()) திரும்பு "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    மாறிலி auto reportFirst10
        = [&](std::function<தருக்க(மாறிலி Candidate&, மாறிலி Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        க்கு (size_t i = 0; i < 10; i++) {
            எனில் (i == candidates.size()) நிறுத்து;
            மாறிலி Candidate& candidate = candidates[i];
            AstVar* மாறிலி varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            எனில் (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        திரும்பு ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](மாறிலி Candidate& a, மாறிலி Candidate& b) {
              திரும்பு a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](மாறிலி Candidate& a, மாறிலி Candidate& b) {  //
              திரும்பு a.second > b.second;
          });

    எனில் (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    திரும்பு ss.str();
}

வெறுமை reportCycles(Graph* graphp, மாறிலி std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    க்கு (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* மாறிலி vscp = vvtxp->vscp();
        File_வரி* மாறிலி flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        எனில் (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } இல்லையெனில் {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, உண்மை);
            // Create a subgraph for the UNOPTFLAT loop
            எனில் (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

வெறுமை dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    க்கு (V3GraphVertex& vtx : graphp->vertices()) {
        எனில் (!vtx.color()) தொடர்;
        scc2Vtxps[vtx.color()].தள்ளு_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    க்கு (மாறிலி auto& pair : scc2Vtxps) {
        மாறிலி uint32_t color = pair.first;
        மாறிலி std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        மாறிலி std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        மாறிலி std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        எனில் (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        க்கு (V3GraphVertex* vtxp : vtxps) {
            மாறிலி SchedAcyclicVarVertex* மாறிலி vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            எனில் (!vvtxp) தொடர்;
            AstVarScope* மாறிலி vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_மரம்(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        க்கு (V3GraphVertex* vtxp : vtxps) {
            மாறிலி SchedAcyclicLogicVertex* மாறிலி lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            எனில் (!lvtxp) தொடர்;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_மரம்(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     மாறிலி std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        மாறிலி VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        க்கு (SchedAcyclicVarVertex* மாறிலி vvtxp : cutVertices) {
            க்கு (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* மாறிலி lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.உச்சி());
                எனில் (!lvtxp->logicp()->user1SetOnce()) lvtxps.தள்ளு_back(lvtxp);
                lvtx2Cuts[lvtxp].தள்ளு_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_மரம்_Finder finder{netlistp};
    க்கு (SchedAcyclicLogicVertex* மாறிலி lvtxp : lvtxps) {
        Ast_முனை* மாறிலி logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_வரி* மாறிலி flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        க்கு (AstVarScope* மாறிலி vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* மாறிலி refp = புதிய AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* மாறிலி nextp = புதிய AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_முனை::addNext(senItemsp, nextp);
        }
        Ast_Sen_மரம்* மாறிலி sen_மரம் = புதிய Ast_Sen_மரம்{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_மரம்(sen_மரம்), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_மரம்->delete_மரம்(), sen_மரம்);
    }
    திரும்பு result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, மாறிலி LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    மாறிலி std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    எனில் (graphp->empty()) திரும்பு LogicByScope{};

    // Dump for debug
    எனில் (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    மாறிலி std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    எனில் (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    திரும்பு fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
