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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3EmitV.h"
#समावेश "V3File.h"
#समावेश "V3Graph.h"
#समावेश "V3Sched.h"
#समावेश "V3SenTree.h"
#समावेश "V3Stats.h"

#समावेश <map>
#समावेश <unordered_map>
#समावेश <unordered_set>
#समावेश <utility>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

// ##############################################################################
//  Data structures (graph types)

श्रेणी SchedAcyclicLogicVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_गाँठ* स्थिर m_logicp;  // The logic node this vertex represents
    AstScope* स्थिर m_scopep;  // The enclosing AstScope of the logic node

खुला:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_गाँठ* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) स्थिर override {
        वापस नया SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_गाँठ* logicp() स्थिर { वापस m_logicp; }
    AstScope* scopep() स्थिर { वापस m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() स्थिर override VL_MT_STABLE { वापस m_logicp->fileline()->ascii(); };
    string dotShape() स्थिर override { वापस "rectangle2"; }
    // LCOV_EXCL_STOP
};

श्रेणी SchedAcyclicVarVertex final : खुला V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* स्थिर m_vscp;  // The AstVarScope this vertex represents

खुला:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() स्थिर { वापस m_vscp; }
    AstVar* varp() स्थिर { वापस m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) स्थिर override {
        वापस नया SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() स्थिर override VL_MT_STABLE { वापस m_vscp->name(); }
    string dotShape() स्थिर override { वापस "ellipse"; }
    string dotColor() स्थिर override { वापस "blue"; }
    // LCOV_EXCL_STOP
};

श्रेणी Graph final : खुला V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        अगर (SchedAcyclicLogicVertex* स्थिर lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_गाँठ* स्थिर logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            अगर (स्थिर AstAlways* स्थिर alwaysp = VN_CAST(logicp, Always)) {
                अगर (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            वापस logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } अन्यथा {
            SchedAcyclicVarVertex* स्थिर vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* स्थिर vscp = vvtxp->vscp();
            वापस vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(स्थिर LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{नया Graph};

    // AstVarScope::user1() -> VarVertx
    स्थिर VNUser1InUse user1InUse;
    स्थिर स्वतः getVarVertex = [&](AstVarScope* vscp) {
        अगर (!vscp->user1p()) vscp->user1p(नया SchedAcyclicVarVertex{graphp.लो(), vscp});
        वापस vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    स्थिर स्वतः addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णांक weight, bool cuttable) {
        नया V3GraphEdge{graphp.लो(), fromp, शीर्ष, weight, cuttable};
    };

    क्रम (स्थिर स्वतः& entry : lbs) {
        AstScope* स्थिर scopep = entry.m_scopep;
        AstActive* स्थिर activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        क्रम (Ast_गाँठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            अगर (VN_IS(nodep, AlwaysPostponed)) जारी;

            SchedAcyclicLogicVertex* स्थिर lvtxp
                = नया SchedAcyclicLogicVertex{graphp.लो(), nodep, scopep};
            स्थिर VNUser2InUse user2InUse;
            स्थिर VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* स्थिर vscp = refp->varScopep();
                SchedAcyclicVarVertex* स्थिर vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                स्थिर पूर्णांक weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                अगर (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, true);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                अगर (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.गिनती(vscp))
                    addEdge(vvtxp, lvtxp, weight, true);
            });
        }
    }

    वापस graphp;
}

व्योम removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    स्थिर स्वतः enqueue = [&](V3GraphVertex* vtxp) {
        अगर (vtxp->user()) वापस;  // Already in queue
        vtxp->user(1);
        queue.धकेलो_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    क्रम (V3GraphVertex& vtx : graphp->vertices()) {
        अगर (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    जबतक (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* स्थिर vtxp = queue.back();
        queue.निकालो_back();
        vtxp->user(0);  // No longer in queue

        अगर (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            क्रम (V3GraphEdge* स्थिर edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->शीर्ष());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } अन्यथा अगर (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            क्रम (V3GraphEdge* स्थिर edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
bool isCut(स्थिर SchedAcyclicVarVertex* vtxp) {
    क्रम (स्थिर V3GraphEdge& edge : vtxp->inEdges()) {
        अगर (edge.weight() == 0) वापस true;
    }
    क्रम (स्थिर V3GraphEdge& edge : vtxp->outEdges()) {
        अगर (edge.weight() == 0) वापस true;
    }
    वापस false;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> परिणाम;
    स्थिर VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    माप_प्रकार nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    माप_प्रकार nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    क्रम (V3GraphVertex& vtx : graphp->vertices()) {
        अगर (!vtx.color()) जारी;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        अगर (SchedAcyclicVarVertex* स्थिर vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            अगर (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) परिणाम.धकेलो_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        अगर (v3Global.opt.stats()) sccs.जोड़ो(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.आकार());
    V3Stats::addStat("Scheduling, Cycles, cut variables", परिणाम.आकार());

    वापस परिणाम;
}

व्योम resetEdgeWeights(स्थिर std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    क्रम (SchedAcyclicVarVertex* स्थिर vvtxp : cutVertices) {
        क्रम (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        क्रम (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, अचिन्हित>;

// Gather all splitting candidates that are in the same SCC as the given vertex
व्योम gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    अगर (vtxp->user()) वापस;  // Already done
    vtxp->user(true);

    अगर (SchedAcyclicVarVertex* स्थिर vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* स्थिर varp = vvtxp->varp();
        स्थिर string name = varp->prettyName();
        अगर (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            स्थिर अचिन्हित fanout = vtxp->outEdges().आकार();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    क्रम (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* स्थिर शीर्ष = edge.शीर्ष();
        अगर (शीर्ष->color() == vtxp->color()) gatherSCCCandidates(शीर्ष, candidates);
    }
    क्रम (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* स्थिर fromp = edge.fromp();
        अगर (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_पंक्ति* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        स्थिर VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    अगर (candidates.empty()) वापस "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    अचिन्हित splittable = 0;
    स्थिर स्वतः reportFirst10
        = [&](std::function<bool(स्थिर Candidate&, स्थिर Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        क्रम (माप_प्रकार i = 0; i < 10; i++) {
            अगर (i == candidates.आकार()) अवरोध;
            स्थिर Candidate& candidate = candidates[i];
            AstVar* स्थिर varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::डेसी << varp->width() << ", circular fanout "
                << candidate.second;
            अगर (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        वापस ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](स्थिर Candidate& a, स्थिर Candidate& b) {
              वापस a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](स्थिर Candidate& a, स्थिर Candidate& b) {  //
              वापस a.second > b.second;
          });

    अगर (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    वापस ss.str();
}

व्योम reportCycles(Graph* graphp, स्थिर std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    क्रम (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* स्थिर vscp = vvtxp->vscp();
        File_पंक्ति* स्थिर flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        अगर (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, true);
            // Create a subgraph for the UNOPTFLAT loop
            अगर (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

व्योम dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    क्रम (V3GraphVertex& vtx : graphp->vertices()) {
        अगर (!vtx.color()) जारी;
        scc2Vtxps[vtx.color()].धकेलो_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    क्रम (स्थिर स्वतः& pair : scc2Vtxps) {
        स्थिर uint32_t color = pair.first;
        स्थिर std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        स्थिर std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        स्थिर std::unique_ptr<std::बख_प्रवाह> ofp{V3File::new_ofstream(fname)};
        अगर (ofp->हार()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.आकार() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        क्रम (V3GraphVertex* vtxp : vtxps) {
            स्थिर SchedAcyclicVarVertex* स्थिर vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            अगर (!vvtxp) जारी;
            AstVarScope* स्थिर vscp = vvtxp->vscp();
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
        क्रम (V3GraphVertex* vtxp : vtxps) {
            स्थिर SchedAcyclicLogicVertex* स्थिर lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            अगर (!lvtxp) जारी;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_वृक्ष(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     स्थिर std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        स्थिर VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        क्रम (SchedAcyclicVarVertex* स्थिर vvtxp : cutVertices) {
            क्रम (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* स्थिर lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.शीर्ष());
                अगर (!lvtxp->logicp()->user1SetOnce()) lvtxps.धकेलो_back(lvtxp);
                lvtx2Cuts[lvtxp].धकेलो_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope परिणाम;
    Sen_वृक्ष_Finder finder{netlistp};
    क्रम (SchedAcyclicLogicVertex* स्थिर lvtxp : lvtxps) {
        Ast_गाँठ* स्थिर logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_पंक्ति* स्थिर flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        क्रम (AstVarScope* स्थिर vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* स्थिर refp = नया AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* स्थिर nextp = नया AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_गाँठ::add_अग्र(senItemsp, nextp);
        }
        Ast_Sen_वृक्ष* स्थिर sen_वृक्ष = नया Ast_Sen_वृक्ष{flp, senItemsp};
        // Add logic to result with new sensitivity
        परिणाम.add(lvtxp->scopep(), finder.get_Sen_वृक्ष(sen_वृक्ष), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_वृक्ष->delete_वृक्ष(), sen_वृक्ष);
    }
    वापस परिणाम;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, स्थिर LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    स्थिर std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.लो());

    // Nothing to do if no cycles, yay!
    अगर (graphp->empty()) वापस LogicByScope{};

    // Dump for debug
    अगर (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    स्थिर std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.लो());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.लो(), cutVertices);

    // Debug dump
    अगर (dumpLevel() >= 6) dumpSccs(graphp.लो());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    वापस fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
