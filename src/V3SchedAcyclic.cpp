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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3Sched.h"
#अन्तर्भाव "V3SenTree.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <map>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3Sched {

नामाकाश {

// ##############################################################################
//  Data structures (graph types)

वर्ग SchedAcyclicLogicVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_गाठ* स्थिरांक m_logicp;  // The logic node this vertex represents
    AstScope* स्थिरांक m_scopep;  // The enclosing AstScope of the logic node

सार्वजनिक:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_गाठ* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) स्थिरांक override {
        परत नवीन SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_गाठ* logicp() स्थिरांक { परत m_logicp; }
    AstScope* scopep() स्थिरांक { परत m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() स्थिरांक override VL_MT_STABLE { परत m_logicp->fileline()->ascii(); };
    string dotShape() स्थिरांक override { परत "rectangle2"; }
    // LCOV_EXCL_STOP
};

वर्ग SchedAcyclicVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* स्थिरांक m_vscp;  // The AstVarScope this vertex represents

सार्वजनिक:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() स्थिरांक { परत m_vscp; }
    AstVar* varp() स्थिरांक { परत m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) स्थिरांक override {
        परत नवीन SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() स्थिरांक override VL_MT_STABLE { परत m_vscp->name(); }
    string dotShape() स्थिरांक override { परत "ellipse"; }
    string dotColor() स्थिरांक override { परत "blue"; }
    // LCOV_EXCL_STOP
};

वर्ग Graph final : सार्वजनिक V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        जर (SchedAcyclicLogicVertex* स्थिरांक lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_गाठ* स्थिरांक logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            जर (स्थिरांक AstAlways* स्थिरांक alwaysp = VN_CAST(logicp, Always)) {
                जर (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            परत logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } अन्यथा {
            SchedAcyclicVarVertex* स्थिरांक vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* स्थिरांक vscp = vvtxp->vscp();
            परत vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(स्थिरांक LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{नवीन Graph};

    // AstVarScope::user1() -> VarVertx
    स्थिरांक VNUser1InUse user1InUse;
    स्थिरांक स्वतः getVarVertex = [&](AstVarScope* vscp) {
        जर (!vscp->user1p()) vscp->user1p(नवीन SchedAcyclicVarVertex{graphp.get(), vscp});
        परत vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    स्थिरांक स्वतः addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight, तार्किक cuttable) {
        नवीन V3GraphEdge{graphp.get(), fromp, शीर्ष, weight, cuttable};
    };

    साठी (स्थिरांक स्वतः& entry : lbs) {
        AstScope* स्थिरांक scopep = entry.m_scopep;
        AstActive* स्थिरांक activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        साठी (Ast_गाठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            जर (VN_IS(nodep, AlwaysPostponed)) सुरूठेवा;

            SchedAcyclicLogicVertex* स्थिरांक lvtxp
                = नवीन SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            स्थिरांक VNUser2InUse user2InUse;
            स्थिरांक VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* स्थिरांक vscp = refp->varScopep();
                SchedAcyclicVarVertex* स्थिरांक vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                स्थिरांक पूर्णांक weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                जर (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, सत्य);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                जर (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.मोजणी(vscp))
                    addEdge(vvtxp, lvtxp, weight, सत्य);
            });
        }
    }

    परत graphp;
}

शून्य removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    स्थिरांक स्वतः enqueue = [&](V3GraphVertex* vtxp) {
        जर (vtxp->user()) परत;  // Already in queue
        vtxp->user(1);
        queue.ढकला_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    साठी (V3GraphVertex& vtx : graphp->vertices()) {
        जर (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    पर्यंत (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* स्थिरांक vtxp = queue.back();
        queue.काढा_back();
        vtxp->user(0);  // No longer in queue

        जर (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            साठी (V3GraphEdge* स्थिरांक edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->शीर्ष());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } अन्यथा जर (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            साठी (V3GraphEdge* स्थिरांक edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
तार्किक isCut(स्थिरांक SchedAcyclicVarVertex* vtxp) {
    साठी (स्थिरांक V3GraphEdge& edge : vtxp->inEdges()) {
        जर (edge.weight() == 0) परत सत्य;
    }
    साठी (स्थिरांक V3GraphEdge& edge : vtxp->outEdges()) {
        जर (edge.weight() == 0) परत सत्य;
    }
    परत असत्य;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    स्थिरांक VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    माप_प्रकार nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    माप_प्रकार nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    साठी (V3GraphVertex& vtx : graphp->vertices()) {
        जर (!vtx.color()) सुरूठेवा;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        जर (SchedAcyclicVarVertex* स्थिरांक vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            जर (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.ढकला_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        जर (v3Global.opt.stats()) sccs.घाला(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    परत result;
}

शून्य resetEdgeWeights(स्थिरांक std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    साठी (SchedAcyclicVarVertex* स्थिरांक vvtxp : cutVertices) {
        साठी (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        साठी (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, अचिन्हित>;

// Gather all splitting candidates that are in the same SCC as the given vertex
शून्य gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    जर (vtxp->user()) परत;  // Already done
    vtxp->user(सत्य);

    जर (SchedAcyclicVarVertex* स्थिरांक vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* स्थिरांक varp = vvtxp->varp();
        स्थिरांक string name = varp->prettyName();
        जर (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            स्थिरांक अचिन्हित fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    साठी (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* स्थिरांक शीर्ष = edge.शीर्ष();
        जर (शीर्ष->color() == vtxp->color()) gatherSCCCandidates(शीर्ष, candidates);
    }
    साठी (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* स्थिरांक fromp = edge.fromp();
        जर (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_ओळ* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        स्थिरांक VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    जर (candidates.empty()) परत "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    अचिन्हित splittable = 0;
    स्थिरांक स्वतः reportFirst10
        = [&](std::function<तार्किक(स्थिरांक Candidate&, स्थिरांक Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        साठी (माप_प्रकार i = 0; i < 10; i++) {
            जर (i == candidates.size()) थांबा;
            स्थिरांक Candidate& candidate = candidates[i];
            AstVar* स्थिरांक varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            जर (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        परत ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](स्थिरांक Candidate& a, स्थिरांक Candidate& b) {
              परत a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](स्थिरांक Candidate& a, स्थिरांक Candidate& b) {  //
              परत a.second > b.second;
          });

    जर (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    परत ss.str();
}

शून्य reportCycles(Graph* graphp, स्थिरांक std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    साठी (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* स्थिरांक vscp = vvtxp->vscp();
        File_ओळ* स्थिरांक flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        जर (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } अन्यथा {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, सत्य);
            // Create a subgraph for the UNOPTFLAT loop
            जर (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

शून्य dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    साठी (V3GraphVertex& vtx : graphp->vertices()) {
        जर (!vtx.color()) सुरूठेवा;
        scc2Vtxps[vtx.color()].ढकला_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    साठी (स्थिरांक स्वतः& pair : scc2Vtxps) {
        स्थिरांक uint32_t color = pair.first;
        स्थिरांक std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        स्थिरांक std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        स्थिरांक std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        जर (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        साठी (V3GraphVertex* vtxp : vtxps) {
            स्थिरांक SchedAcyclicVarVertex* स्थिरांक vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            जर (!vvtxp) सुरूठेवा;
            AstVarScope* स्थिरांक vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_वृक्ष(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        साठी (V3GraphVertex* vtxp : vtxps) {
            स्थिरांक SchedAcyclicLogicVertex* स्थिरांक lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            जर (!lvtxp) सुरूठेवा;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_वृक्ष(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     स्थिरांक std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        स्थिरांक VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        साठी (SchedAcyclicVarVertex* स्थिरांक vvtxp : cutVertices) {
            साठी (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* स्थिरांक lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.शीर्ष());
                जर (!lvtxp->logicp()->user1SetOnce()) lvtxps.ढकला_back(lvtxp);
                lvtx2Cuts[lvtxp].ढकला_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_वृक्ष_Finder finder{netlistp};
    साठी (SchedAcyclicLogicVertex* स्थिरांक lvtxp : lvtxps) {
        Ast_गाठ* स्थिरांक logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_ओळ* स्थिरांक flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        साठी (AstVarScope* स्थिरांक vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* स्थिरांक refp = नवीन AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* स्थिरांक nextp = नवीन AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_गाठ::addNext(senItemsp, nextp);
        }
        Ast_Sen_वृक्ष* स्थिरांक sen_वृक्ष = नवीन Ast_Sen_वृक्ष{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_वृक्ष(sen_वृक्ष), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_वृक्ष->delete_वृक्ष(), sen_वृक्ष);
    }
    परत result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, स्थिरांक LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    स्थिरांक std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    जर (graphp->empty()) परत LogicByScope{};

    // Dump for debug
    जर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    स्थिरांक std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    जर (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    परत fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
