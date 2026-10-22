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
    Ast_गाँठो* अचर m_logicp;  // The logic node this vertex represents
    AstScope* अचर m_scopep;  // The enclosing AstScope of the logic node

सार्वजनिक:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_गाँठो* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) अचर override {
        फर्काऊ नूतन SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_गाँठो* logicp() अचर { फर्काऊ m_logicp; }
    AstScope* scopep() अचर { फर्काऊ m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() अचर override VL_MT_STABLE { फर्काऊ m_logicp->fileline()->ascii(); };
    string dotShape() अचर override { फर्काऊ "rectangle2"; }
    // LCOV_EXCL_STOP
};

वर्ग SchedAcyclicVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* अचर m_vscp;  // The AstVarScope this vertex represents

सार्वजनिक:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() अचर { फर्काऊ m_vscp; }
    AstVar* varp() अचर { फर्काऊ m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) अचर override {
        फर्काऊ नूतन SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() अचर override VL_MT_STABLE { फर्काऊ m_vscp->name(); }
    string dotShape() अचर override { फर्काऊ "ellipse"; }
    string dotColor() अचर override { फर्काऊ "blue"; }
    // LCOV_EXCL_STOP
};

वर्ग Graph final : सार्वजनिक V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        यदि (SchedAcyclicLogicVertex* अचर lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_गाँठो* अचर logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            यदि (अचर AstAlways* अचर alwaysp = VN_CAST(logicp, Always)) {
                यदि (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            फर्काऊ logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } अन्यथा {
            SchedAcyclicVarVertex* अचर vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* अचर vscp = vvtxp->vscp();
            फर्काऊ vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(अचर LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{नूतन Graph};

    // AstVarScope::user1() -> VarVertx
    अचर VNUser1InUse user1InUse;
    अचर स्वतः getVarVertex = [&](AstVarScope* vscp) {
        यदि (!vscp->user1p()) vscp->user1p(नूतन SchedAcyclicVarVertex{graphp.get(), vscp});
        फर्काऊ vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    अचर स्वतः addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* शीर्ष, पूर्णाङ्क weight, तार्किक cuttable) {
        नूतन V3GraphEdge{graphp.get(), fromp, शीर्ष, weight, cuttable};
    };

    लागि (अचर स्वतः& entry : lbs) {
        AstScope* अचर scopep = entry.m_scopep;
        AstActive* अचर activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        लागि (Ast_गाँठो* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            यदि (VN_IS(nodep, AlwaysPostponed)) जारी;

            SchedAcyclicLogicVertex* अचर lvtxp
                = नूतन SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            अचर VNUser2InUse user2InUse;
            अचर VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* अचर vscp = refp->varScopep();
                SchedAcyclicVarVertex* अचर vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                अचर पूर्णाङ्क weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                यदि (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, सत्य);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                यदि (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.गन्ती(vscp))
                    addEdge(vvtxp, lvtxp, weight, सत्य);
            });
        }
    }

    फर्काऊ graphp;
}

शून्य removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    अचर स्वतः enqueue = [&](V3GraphVertex* vtxp) {
        यदि (vtxp->user()) फर्काऊ;  // Already in queue
        vtxp->user(1);
        queue.थप_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    लागि (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    जबसम्म (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* अचर vtxp = queue.back();
        queue.झिक_back();
        vtxp->user(0);  // No longer in queue

        यदि (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            लागि (V3GraphEdge* अचर edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->शीर्ष());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } अन्यथा यदि (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            लागि (V3GraphEdge* अचर edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
तार्किक isCut(अचर SchedAcyclicVarVertex* vtxp) {
    लागि (अचर V3GraphEdge& edge : vtxp->inEdges()) {
        यदि (edge.weight() == 0) फर्काऊ सत्य;
    }
    लागि (अचर V3GraphEdge& edge : vtxp->outEdges()) {
        यदि (edge.weight() == 0) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    अचर VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    माप_प्रकार nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    माप_प्रकार nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    लागि (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (!vtx.color()) जारी;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        यदि (SchedAcyclicVarVertex* अचर vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            यदि (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.थप_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        यदि (v3Global.opt.stats()) sccs.घुसाउ(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    फर्काऊ result;
}

शून्य resetEdgeWeights(अचर std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    लागि (SchedAcyclicVarVertex* अचर vvtxp : cutVertices) {
        लागि (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        लागि (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, अचिन्हित>;

// Gather all splitting candidates that are in the same SCC as the given vertex
शून्य gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    यदि (vtxp->user()) फर्काऊ;  // Already done
    vtxp->user(सत्य);

    यदि (SchedAcyclicVarVertex* अचर vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* अचर varp = vvtxp->varp();
        अचर string name = varp->prettyName();
        यदि (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            अचर अचिन्हित fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    लागि (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* अचर शीर्ष = edge.शीर्ष();
        यदि (शीर्ष->color() == vtxp->color()) gatherSCCCandidates(शीर्ष, candidates);
    }
    लागि (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* अचर fromp = edge.fromp();
        यदि (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_पङ्क्ति* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        अचर VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    यदि (candidates.empty()) फर्काऊ "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    अचिन्हित splittable = 0;
    अचर स्वतः reportFirst10
        = [&](std::function<तार्किक(अचर Candidate&, अचर Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        लागि (माप_प्रकार i = 0; i < 10; i++) {
            यदि (i == candidates.size()) रोक;
            अचर Candidate& candidate = candidates[i];
            AstVar* अचर varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            यदि (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        फर्काऊ ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](अचर Candidate& a, अचर Candidate& b) {
              फर्काऊ a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](अचर Candidate& a, अचर Candidate& b) {  //
              फर्काऊ a.second > b.second;
          });

    यदि (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    फर्काऊ ss.str();
}

शून्य reportCycles(Graph* graphp, अचर std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    लागि (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* अचर vscp = vvtxp->vscp();
        File_पङ्क्ति* अचर flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        यदि (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
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
            यदि (v3Global.opt.reportUnoptflat()) {
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
    लागि (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (!vtx.color()) जारी;
        scc2Vtxps[vtx.color()].थप_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    लागि (अचर स्वतः& pair : scc2Vtxps) {
        अचर uint32_t color = pair.first;
        अचर std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        अचर std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        अचर std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        यदि (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        लागि (V3GraphVertex* vtxp : vtxps) {
            अचर SchedAcyclicVarVertex* अचर vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            यदि (!vvtxp) जारी;
            AstVarScope* अचर vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_रूख(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        लागि (V3GraphVertex* vtxp : vtxps) {
            अचर SchedAcyclicLogicVertex* अचर lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            यदि (!lvtxp) जारी;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_रूख(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     अचर std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        अचर VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        लागि (SchedAcyclicVarVertex* अचर vvtxp : cutVertices) {
            लागि (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* अचर lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.शीर्ष());
                यदि (!lvtxp->logicp()->user1SetOnce()) lvtxps.थप_back(lvtxp);
                lvtx2Cuts[lvtxp].थप_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_रूख_Finder finder{netlistp};
    लागि (SchedAcyclicLogicVertex* अचर lvtxp : lvtxps) {
        Ast_गाँठो* अचर logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_पङ्क्ति* अचर flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        लागि (AstVarScope* अचर vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* अचर refp = नूतन AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* अचर nextp = नूतन AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_गाँठो::addNext(senItemsp, nextp);
        }
        Ast_Sen_रूख* अचर sen_रूख = नूतन Ast_Sen_रूख{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_रूख(sen_रूख), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_रूख->delete_रूख(), sen_रूख);
    }
    फर्काऊ result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, अचर LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    अचर std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    यदि (graphp->empty()) फर्काऊ LogicByScope{};

    // Dump for debug
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    अचर std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    यदि (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    फर्काऊ fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
