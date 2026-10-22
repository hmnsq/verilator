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
    Ast_ग्रन्थिः* ध्रुव m_logicp;  // The logic node this vertex represents
    AstScope* ध्रुव m_scopep;  // The enclosing AstScope of the logic node

सार्वजनिक:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_ग्रन्थिः* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) ध्रुव override {
        प्रत्यागम नूतन SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_ग्रन्थिः* logicp() ध्रुव { प्रत्यागम m_logicp; }
    AstScope* scopep() ध्रुव { प्रत्यागम m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_logicp->fileline()->ascii(); };
    string dotShape() ध्रुव override { प्रत्यागम "rectangle2"; }
    // LCOV_EXCL_STOP
};

वर्ग SchedAcyclicVarVertex final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* ध्रुव m_vscp;  // The AstVarScope this vertex represents

सार्वजनिक:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ध्रुव { प्रत्यागम m_vscp; }
    AstVar* varp() ध्रुव { प्रत्यागम m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) ध्रुव override {
        प्रत्यागम नूतन SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ध्रुव override VL_MT_STABLE { प्रत्यागम m_vscp->name(); }
    string dotShape() ध्रुव override { प्रत्यागम "ellipse"; }
    string dotColor() ध्रुव override { प्रत्यागम "blue"; }
    // LCOV_EXCL_STOP
};

वर्ग Graph final : सार्वजनिक V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        यदि (SchedAcyclicLogicVertex* ध्रुव lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_ग्रन्थिः* ध्रुव logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            यदि (ध्रुव AstAlways* ध्रुव alwaysp = VN_CAST(logicp, Always)) {
                यदि (alwaysp->keyword() == VAlwaysKwd::CONT_नियोजय) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            प्रत्यागम logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } अन्यथा {
            SchedAcyclicVarVertex* ध्रुव vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* ध्रुव vscp = vvtxp->vscp();
            प्रत्यागम vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ध्रुव LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{नूतन Graph};

    // AstVarScope::user1() -> VarVertx
    ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;
    ध्रुव स्वचालित getVarVertex = [&](AstVarScope* vscp) {
        यदि (!vscp->user1p()) vscp->user1p(नूतन SchedAcyclicVarVertex{graphp.get(), vscp});
        प्रत्यागम vscp->user1u().गन्तव्यम्<SchedAcyclicVarVertex*>();
    };

    ध्रुव स्वचालित सङ्कलय_Edge = [&](V3GraphVertex* fromp, V3GraphVertex* शिखरम्, पूर्णाङ्क weight, तार्किक cuttable) {
        नूतन V3GraphEdge{graphp.get(), fromp, शिखरम्, weight, cuttable};
    };

    प्रति (ध्रुव स्वचालित& प्रविष्टिः : lbs) {
        AstScope* ध्रुव scopep = प्रविष्टिः.m_scopep;
        Ast_सक्रिय* ध्रुव activep = प्रविष्टिः.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        प्रति (Ast_ग्रन्थिः* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            यदि (VN_IS(nodep, AlwaysPostponed)) अनुवर्त;

            SchedAcyclicLogicVertex* ध्रुव lvtxp
                = नूतन SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
            ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;

            V3Sched::util::Var_Scope_समुच्चयः forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ध्रुव vscp = refp->varScopep();
                SchedAcyclicVarVertex* ध्रुव vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                ध्रुव पूर्णाङ्क weight = vscp->विस्तारः() / 8 + 1;
                // If written, add logic -> var edge
                यदि (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2_समुच्चयः_Once())
                    सङ्कलय_Edge(lvtxp, vvtxp, weight, सत्य);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                यदि (refp->access().isReadOrRW() && !vscp->user3_समुच्चयः_Once() && !vscp->user2()
                    && !forceReadEdgeIgnores.गणनम्(vscp))
                    सङ्कलय_Edge(vvtxp, lvtxp, weight, सत्य);
            });
        }
    }

    प्रत्यागम graphp;
}

शून्य removeNonCyclic(Graph* graphp) {
    // Work queue
    std::सदिशम्<V3GraphVertex*> queue;

    ध्रुव स्वचालित पङ्क्तौ_निक्षिप = [&](V3GraphVertex* vtxp) {
        यदि (vtxp->user()) प्रत्यागम;  // Already in queue
        vtxp->user(1);
        queue.प्रक्षिप_पृष्ठभागः(vtxp);
    };

    // Start with vertices with no inputs or outputs
    प्रति (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (vtx.अन्तर्_रिक्तानि() || vtx.बहिर्गमनम्_रिक्तानि()) पङ्क्तौ_निक्षिप(&vtx);
    }

    // Iterate while we still have candidates
    यावत् (!queue.रिक्तानि()) {
        // Pop next candidate
        V3GraphVertex* ध्रुव vtxp = queue.पृष्ठभागः();
        queue.निष्कासय_पृष्ठभागः();
        vtxp->user(0);  // No longer in queue

        यदि (vtxp->अन्तर्_रिक्तानि()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            प्रति (V3GraphEdge* ध्रुव edgep : vtxp->बहिर्गमनम्_धाराः().unlinkable()) {
                पङ्क्तौ_निक्षिप(edgep->शिखरम्());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } अन्यथा यदि (vtxp->बहिर्गमनम्_रिक्तानि()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            प्रति (V3GraphEdge* ध्रुव edgep : vtxp->अन्तर्_धाराः().unlinkable()) {
                पङ्क्तौ_निक्षिप(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
तार्किक isCut(ध्रुव SchedAcyclicVarVertex* vtxp) {
    प्रति (ध्रुव V3GraphEdge& edge : vtxp->अन्तर्_धाराः()) {
        यदि (edge.weight() == 0) प्रत्यागम सत्य;
    }
    प्रति (ध्रुव V3GraphEdge& edge : vtxp->बहिर्गमनम्_धाराः()) {
        यदि (edge.weight() == 0) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}

std::सदिशम्<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::सदिशम्<SchedAcyclicVarVertex*> परिणाम;
    ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;  // bool: already added to result

    // Statistics
    आकार_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    आकार_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_समुच्चयः<uint32_t> sccs;  // Unique SCC colors

    प्रति (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (!vtx.color()) अनुवर्त;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        यदि (SchedAcyclicVarVertex* ध्रुव vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            यदि (!vvtxp->vscp()->user1_समुच्चयः_Once() && isCut(vvtxp)) परिणाम.प्रक्षिप_पृष्ठभागः(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        यदि (v3Global.opt.stats()) sccs.निवेशय(vtx.color());
    }

    V3Stats::सङ्कलय_Stat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::सङ्कलय_Stat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::सङ्कलय_Stat("Scheduling, Cycles, unique SCCs", sccs.आकार());
    V3Stats::सङ्कलय_Stat("Scheduling, Cycles, cut variables", परिणाम.आकार());

    प्रत्यागम परिणाम;
}

शून्य resetEdgeWeights(ध्रुव std::सदिशम्<SchedAcyclicVarVertex*>& cutVertices) {
    प्रति (SchedAcyclicVarVertex* ध्रुव vvtxp : cutVertices) {
        प्रति (V3GraphEdge& e : vvtxp->अन्तर्_धाराः()) e.weight(1);
        प्रति (V3GraphEdge& e : vvtxp->बहिर्गमनम्_धाराः()) e.weight(1);
    }
}

// A VarVertex together with its fanout
उपयुज्य Candidate = std::pair<SchedAcyclicVarVertex*, अचिह्नित>;

// Gather all splitting candidates that are in the same SCC as the given vertex
शून्य gatherSCCCandidates(V3GraphVertex* vtxp, std::सदिशम्<Candidate>& candidates) {
    यदि (vtxp->user()) प्रत्यागम;  // Already done
    vtxp->user(सत्य);

    यदि (SchedAcyclicVarVertex* ध्रुव vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* ध्रुव varp = vvtxp->varp();
        ध्रुव string name = varp->prettyName();
        यदि (!varp->user3_समुच्चयः_Once()  // Only consider each AstVar once
            && varp->विस्तारः() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ध्रुव अचिह्नित fanout = vtxp->बहिर्गमनम्_धाराः().आकार();
            candidates.emplace_पृष्ठभागः(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    प्रति (V3GraphEdge& edge : vtxp->बहिर्गमनम्_धाराः()) {
        V3GraphVertex* ध्रुव शिखरम् = edge.शिखरम्();
        यदि (शिखरम्->color() == vtxp->color()) gatherSCCCandidates(शिखरम्, candidates);
    }
    प्रति (V3GraphEdge& edge : vtxp->अन्तर्_धाराः()) {
        V3GraphVertex* ध्रुव fromp = edge.fromp();
        यदि (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_पङ्क्तिः* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::सदिशम्<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ध्रुव VNUser3_अन्तर्_Use user3_अन्तर्_Use;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    यदि (candidates.रिक्तानि()) प्रत्यागम "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    अचिह्नित splittable = 0;
    ध्रुव स्वचालित reportFirst10
        = [&](std::function<तार्किक(ध्रुव Candidate&, ध्रुव Candidate&)> less) -> string {
        std::stable_क्रमय(candidates.begin(), candidates.अन्तः_2(), less);
        std::ostringstream ss2;
        प्रति (आकार_t i = 0; i < 10; i++) {
            यदि (i == candidates.आकार()) भङ्ग;
            ध्रुव Candidate& candidate = candidates[i];
            AstVar* ध्रुव varp = candidate.first->varp();

            ss2 << V3Error::warn_अधिकम्() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->विस्तारः() << ", circular fanout "
                << candidate.second;
            यदि (!varp->cannot_विभज_विधा_Reason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        प्रत्यागम ss2.str();
    };

    // Widest variables
    ss << V3Error::warn_अधिकम्() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ध्रुव Candidate& a, ध्रुव Candidate& b) {
              प्रत्यागम a.first->varp()->विस्तारः() > b.first->varp()->विस्तारः();
          });

    // Highest fanout
    ss << V3Error::warn_अधिकम्() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ध्रुव Candidate& a, ध्रुव Candidate& b) {  //
              प्रत्यागम a.second > b.second;
          });

    यदि (splittable) {
        ss << V3Error::warn_अधिकम्()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::सङ्कलय_Stat("Scheduling, split_var, candidates", splittable);
    प्रत्यागम ss.str();
}

शून्य report_आवृत्तयः(Graph* graphp, ध्रुव std::सदिशम्<SchedAcyclicVarVertex*>& cutVertices) {
    प्रति (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* ध्रुव vscp = vvtxp->vscp();
        File_पङ्क्तिः* ध्रुव flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        यदि (flp->warnIsOff(V3_Error_संहिता::UNOPTFLAT)) {
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
            flp->modifyWarnOff(V3_Error_संहिता::UNOPTFLAT, सत्य);
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
    std::map<uint32_t, std::सदिशम्<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    प्रति (V3GraphVertex& vtx : graphp->vertices()) {
        यदि (!vtx.color()) अनुवर्त;
        scc2Vtxps[vtx.color()].प्रक्षिप_पृष्ठभागः(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    प्रति (ध्रुव स्वचालित& pair : scc2Vtxps) {
        ध्रुव uint32_t color = pair.first;
        ध्रुव std::सदिशम्<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        ध्रुव std::string fname
            = v3Global.debugFilename("sched_scc_" + std::गन्तव्यम्_string(color) + ".v");
        ध्रुव std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        यदि (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.आकार() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        प्रति (V3GraphVertex* vtxp : vtxps) {
            ध्रुव SchedAcyclicVarVertex* ध्रुव vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            यदि (!vvtxp) अनुवर्त;
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
        प्रति (V3GraphVertex* vtxp : vtxps) {
            ध्रुव SchedAcyclicLogicVertex* ध्रुव lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            यदि (!lvtxp) अनुवर्त;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_वृक्षः(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ध्रुव std::सदिशम्<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::सदिशम्<AstVarScope*>> lvtx2Cuts;
    std::सदिशम्<SchedAcyclicLogicVertex*> lvtxps;
    {
        ध्रुव VNUser1_अन्तर्_Use user1_अन्तर्_Use;  // bool: already added to 'lvtxps'
        प्रति (SchedAcyclicVarVertex* ध्रुव vvtxp : cutVertices) {
            प्रति (V3GraphEdge& edge : vvtxp->बहिर्गमनम्_धाराः()) {
                SchedAcyclicLogicVertex* ध्रुव lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.शिखरम्());
                यदि (!lvtxp->logicp()->user1_समुच्चयः_Once()) lvtxps.प्रक्षिप_पृष्ठभागः(lvtxp);
                lvtx2Cuts[lvtxp].प्रक्षिप_पृष्ठभागः(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope परिणाम;
    Sen_वृक्षः_Finder finder{netlistp};
    प्रति (SchedAcyclicLogicVertex* ध्रुव lvtxp : lvtxps) {
        Ast_ग्रन्थिः* ध्रुव logicp = lvtxp->logicp();
        logicp->unlink_Fr_पृष्ठभागः();
        File_पङ्क्तिः* ध्रुव flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = शून्यसूचकः;
        प्रति (AstVarScope* ध्रुव vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ध्रुव refp = नूतन AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ध्रुव nextp = नूतन AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_ग्रन्थिः::सङ्कलय_अग्र(senItemsp, nextp);
        }
        Ast_Sen_वृक्षः* ध्रुव sen_वृक्षः = नूतन Ast_Sen_वृक्षः{flp, senItemsp};
        // Add logic to result with new sensitivity
        परिणाम.सङ्कलय(lvtxp->scopep(), finder.get_Sen_वृक्षः(sen_वृक्षः), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_वृक्षः->delete_वृक्षः(), sen_वृक्षः);
    }
    प्रत्यागम परिणाम;
}

}  // namespace

LogicByScope break_आवृत्तयः(AstNetlist* netlistp, ध्रुव LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ध्रुव std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    यदि (graphp->रिक्तानि()) प्रत्यागम LogicByScope{};

    // Dump for debug
    यदि (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    ध्रुव std::सदिशम्<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    report_आवृत्तयः(graphp.get(), cutVertices);

    // Debug dump
    यदि (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    प्रत्यागम fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
