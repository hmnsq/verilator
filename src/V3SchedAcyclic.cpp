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

वग्ग SchedAcyclicLogicVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_ग्रन्थिः* ध्रुव m_logicp;  // The logic node this vertex represents
    AstScope* ध्रुव m_scopep;  // The enclosing AstScope of the logic node

सार्वजनिक:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_ग्रन्थिः* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) ध्रुव override {
        पटिनिवत्त नूतन SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_ग्रन्थिः* logicp() ध्रुव { पटिनिवत्त m_logicp; }
    AstScope* scopep() ध्रुव { पटिनिवत्त m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_logicp->fileline()->ascii(); };
    string dotShape() ध्रुव override { पटिनिवत्त "rectangle2"; }
    // LCOV_EXCL_STOP
};

वग्ग SchedAcyclicVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* ध्रुव m_vscp;  // The AstVarScope this vertex represents

सार्वजनिक:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ध्रुव { पटिनिवत्त m_vscp; }
    AstVar* varp() ध्रुव { पटिनिवत्त m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) ध्रुव override {
        पटिनिवत्त नूतन SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_vscp->name(); }
    string dotShape() ध्रुव override { पटिनिवत्त "ellipse"; }
    string dotColor() ध्रुव override { पटिनिवत्त "blue"; }
    // LCOV_EXCL_STOP
};

वग्ग Graph final : सार्वजनिक V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        सचे (SchedAcyclicLogicVertex* ध्रुव lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_ग्रन्थिः* ध्रुव logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            सचे (ध्रुव AstAlways* ध्रुव alwaysp = VN_CAST(logicp, Always)) {
                सचे (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            पटिनिवत्त logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } अञ्ञथा {
            SchedAcyclicVarVertex* ध्रुव vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* ध्रुव vscp = vvtxp->vscp();
            पटिनिवत्त vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ध्रुव LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{नूतन Graph};

    // AstVarScope::user1() -> VarVertx
    ध्रुव VNUser1InUse user1InUse;
    ध्रुव auto getVarVertex = [&](AstVarScope* vscp) {
        सचे (!vscp->user1p()) vscp->user1p(नूतन SchedAcyclicVarVertex{graphp.get(), vscp});
        पटिनिवत्त vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    ध्रुव auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* शिखरम्, पूर्णाङ्क weight, तार्किक cuttable) {
        नूतन V3GraphEdge{graphp.get(), fromp, शिखरम्, weight, cuttable};
    };

    अत्थाय (ध्रुव auto& entry : lbs) {
        AstScope* ध्रुव scopep = entry.m_scopep;
        AstActive* ध्रुव activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        अत्थाय (Ast_ग्रन्थिः* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            सचे (VN_IS(nodep, AlwaysPostponed)) अनुवर्त;

            SchedAcyclicLogicVertex* ध्रुव lvtxp
                = नूतन SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            ध्रुव VNUser2InUse user2InUse;
            ध्रुव VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ध्रुव vscp = refp->varScopep();
                SchedAcyclicVarVertex* ध्रुव vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                ध्रुव पूर्णाङ्क weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                सचे (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, सच्च);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                सचे (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.गणनम्(vscp))
                    addEdge(vvtxp, lvtxp, weight, सच्च);
            });
        }
    }

    पटिनिवत्त graphp;
}

शून्य removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    ध्रुव auto enqueue = [&](V3GraphVertex* vtxp) {
        सचे (vtxp->user()) पटिनिवत्त;  // Already in queue
        vtxp->user(1);
        queue.प्रक्षिप_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    अत्थाय (V3GraphVertex& vtx : graphp->vertices()) {
        सचे (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    याव (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* ध्रुव vtxp = queue.back();
        queue.निष्कासय_back();
        vtxp->user(0);  // No longer in queue

        सचे (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            अत्थाय (V3GraphEdge* ध्रुव edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->शिखरम्());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } अञ्ञथा सचे (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            अत्थाय (V3GraphEdge* ध्रुव edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
तार्किक isCut(ध्रुव SchedAcyclicVarVertex* vtxp) {
    अत्थाय (ध्रुव V3GraphEdge& edge : vtxp->inEdges()) {
        सचे (edge.weight() == 0) पटिनिवत्त सच्च;
    }
    अत्थाय (ध्रुव V3GraphEdge& edge : vtxp->outEdges()) {
        सचे (edge.weight() == 0) पटिनिवत्त सच्च;
    }
    पटिनिवत्त मिच्छा;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    ध्रुव VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    अत्थाय (V3GraphVertex& vtx : graphp->vertices()) {
        सचे (!vtx.color()) अनुवर्त;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        सचे (SchedAcyclicVarVertex* ध्रुव vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            सचे (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.प्रक्षिप_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        सचे (v3Global.opt.stats()) sccs.निवेशय(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    पटिनिवत्त result;
}

शून्य resetEdgeWeights(ध्रुव std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    अत्थाय (SchedAcyclicVarVertex* ध्रुव vvtxp : cutVertices) {
        अत्थाय (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        अत्थाय (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
शून्य gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    सचे (vtxp->user()) पटिनिवत्त;  // Already done
    vtxp->user(सच्च);

    सचे (SchedAcyclicVarVertex* ध्रुव vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* ध्रुव varp = vvtxp->varp();
        ध्रुव string name = varp->prettyName();
        सचे (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ध्रुव unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    अत्थाय (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* ध्रुव शिखरम् = edge.शिखरम्();
        सचे (शिखरम्->color() == vtxp->color()) gatherSCCCandidates(शिखरम्, candidates);
    }
    अत्थाय (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* ध्रुव fromp = edge.fromp();
        सचे (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_पङ्क्तिः* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ध्रुव VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    सचे (candidates.empty()) पटिनिवत्त "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    ध्रुव auto reportFirst10
        = [&](std::function<तार्किक(ध्रुव Candidate&, ध्रुव Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        अत्थाय (size_t i = 0; i < 10; i++) {
            सचे (i == candidates.size()) भङ्ग;
            ध्रुव Candidate& candidate = candidates[i];
            AstVar* ध्रुव varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            सचे (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        पटिनिवत्त ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ध्रुव Candidate& a, ध्रुव Candidate& b) {
              पटिनिवत्त a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ध्रुव Candidate& a, ध्रुव Candidate& b) {  //
              पटिनिवत्त a.second > b.second;
          });

    सचे (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    पटिनिवत्त ss.str();
}

शून्य reportCycles(Graph* graphp, ध्रुव std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    अत्थाय (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* ध्रुव vscp = vvtxp->vscp();
        File_पङ्क्तिः* ध्रुव flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        सचे (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } अञ्ञथा {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, सच्च);
            // Create a subgraph for the UNOPTFLAT loop
            सचे (v3Global.opt.reportUnoptflat()) {
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
    अत्थाय (V3GraphVertex& vtx : graphp->vertices()) {
        सचे (!vtx.color()) अनुवर्त;
        scc2Vtxps[vtx.color()].प्रक्षिप_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    अत्थाय (ध्रुव auto& pair : scc2Vtxps) {
        ध्रुव uint32_t color = pair.first;
        ध्रुव std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        ध्रुव std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        ध्रुव std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        सचे (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        अत्थाय (V3GraphVertex* vtxp : vtxps) {
            ध्रुव SchedAcyclicVarVertex* ध्रुव vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            सचे (!vvtxp) अनुवर्त;
            AstVarScope* ध्रुव vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_वृक्षः(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        अत्थाय (V3GraphVertex* vtxp : vtxps) {
            ध्रुव SchedAcyclicLogicVertex* ध्रुव lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            सचे (!lvtxp) अनुवर्त;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_वृक्षः(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ध्रुव std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        ध्रुव VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        अत्थाय (SchedAcyclicVarVertex* ध्रुव vvtxp : cutVertices) {
            अत्थाय (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* ध्रुव lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.शिखरम्());
                सचे (!lvtxp->logicp()->user1SetOnce()) lvtxps.प्रक्षिप_back(lvtxp);
                lvtx2Cuts[lvtxp].प्रक्षिप_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_वृक्षः_Finder finder{netlistp};
    अत्थाय (SchedAcyclicLogicVertex* ध्रुव lvtxp : lvtxps) {
        Ast_ग्रन्थिः* ध्रुव logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_पङ्क्तिः* ध्रुव flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        अत्थाय (AstVarScope* ध्रुव vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ध्रुव nextp = नूतन AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_ग्रन्थिः::addNext(senItemsp, nextp);
        }
        Ast_Sen_वृक्षः* ध्रुव sen_वृक्षः = नूतन Ast_Sen_वृक्षः{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_वृक्षः(sen_वृक्षः), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_वृक्षः->delete_वृक्षः(), sen_वृक्षः);
    }
    पटिनिवत्त result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, ध्रुव LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ध्रुव std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    सचे (graphp->empty()) पटिनिवत्त LogicByScope{};

    // Dump for debug
    सचे (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    ध्रुव std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    सचे (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    पटिनिवत्त fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
