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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3EmitV.h"
#ضمن "V3File.h"
#ضمن "V3Graph.h"
#ضمن "V3Sched.h"
#ضمن "V3SenTree.h"
#ضمن "V3Stats.h"

#ضمن <map>
#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <utility>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3Sched {

نطاق {

// ##############################################################################
//  Data structures (graph types)

صنف SchedAcyclicLogicVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    AstNode* ثابت m_logicp;  // The logic node this vertex represents
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node

عام:
    SchedAcyclicLogicVertex(V3Graph* graphp, AstNode* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) ثابت override {
        أرجع جديد SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    AstNode* logicp() ثابت { أرجع m_logicp; }
    AstScope* scopep() ثابت { أرجع m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { أرجع m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { أرجع "rectangle2"; }
    // LCOV_EXCL_STOP
};

صنف SchedAcyclicVarVertex final : عام V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عام:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ثابت { أرجع m_vscp; }
    AstVar* varp() ثابت { أرجع m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) ثابت override {
        أرجع جديد SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { أرجع m_vscp->name(); }
    string dotShape() ثابت override { أرجع "ellipse"; }
    string dotColor() ثابت override { أرجع "blue"; }
    // LCOV_EXCL_STOP
};

صنف Graph final : عام V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        𐡄𐡍 (SchedAcyclicLogicVertex* ثابت lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            AstNode* ثابت logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            𐡄𐡍 (ثابت AstAlways* ثابت alwaysp = VN_CAST(logicp, Always)) {
                𐡄𐡍 (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            أرجع logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } وإلا {
            SchedAcyclicVarVertex* ثابت vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* ثابت vscp = vvtxp->vscp();
            أرجع vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ثابت LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{جديد Graph};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1InUse user1InUse;
    ثابت auto getVarVertex = [&](AstVarScope* vscp) {
        𐡄𐡍 (!vscp->user1p()) vscp->user1p(جديد SchedAcyclicVarVertex{graphp.get(), vscp});
        أرجع vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    ثابت auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* top, صحيح weight, منطقي cuttable) {
        جديد V3GraphEdge{graphp.get(), fromp, top, weight, cuttable};
    };

    لكل (ثابت auto& entry : lbs) {
        AstScope* ثابت scopep = entry.m_scopep;
        AstActive* ثابت activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        لكل (AstNode* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            𐡄𐡍 (VN_IS(nodep, AlwaysPostponed)) استمر;

            SchedAcyclicLogicVertex* ثابت lvtxp
                = جديد SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            ثابت VNUser2InUse user2InUse;
            ثابت VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                SchedAcyclicVarVertex* ثابت vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                ثابت صحيح weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                𐡄𐡍 (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, 𐡒𐡔𐡈);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                𐡄𐡍 (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.count(vscp))
                    addEdge(vvtxp, lvtxp, weight, 𐡒𐡔𐡈);
            });
        }
    }

    أرجع graphp;
}

فراغ removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    ثابت auto enqueue = [&](V3GraphVertex* vtxp) {
        𐡄𐡍 (vtxp->user()) أرجع;  // Already in queue
        vtxp->user(1);
        queue.push_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        𐡄𐡍 (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    طالما (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* ثابت vtxp = queue.back();
        queue.pop_back();
        vtxp->user(0);  // No longer in queue

        𐡄𐡍 (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            لكل (V3GraphEdge* ثابت edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->top());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } وإلا 𐡄𐡍 (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            لكل (V3GraphEdge* ثابت edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
منطقي isCut(ثابت SchedAcyclicVarVertex* vtxp) {
    لكل (ثابت V3GraphEdge& edge : vtxp->inEdges()) {
        𐡄𐡍 (edge.weight() == 0) أرجع 𐡒𐡔𐡈;
    }
    لكل (ثابت V3GraphEdge& edge : vtxp->outEdges()) {
        𐡄𐡍 (edge.weight() == 0) أرجع 𐡒𐡔𐡈;
    }
    أرجع خطأ;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    ثابت VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        𐡄𐡍 (!vtx.color()) استمر;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        𐡄𐡍 (SchedAcyclicVarVertex* ثابت vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            𐡄𐡍 (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.push_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        𐡄𐡍 (v3Global.opt.stats()) sccs.insert(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    أرجع result;
}

فراغ resetEdgeWeights(ثابت std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    لكل (SchedAcyclicVarVertex* ثابت vvtxp : cutVertices) {
        لكل (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        لكل (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
فراغ gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    𐡄𐡍 (vtxp->user()) أرجع;  // Already done
    vtxp->user(𐡒𐡔𐡈);

    𐡄𐡍 (SchedAcyclicVarVertex* ثابت vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* ثابت varp = vvtxp->varp();
        ثابت string name = varp->prettyName();
        𐡄𐡍 (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ثابت unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    لكل (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* ثابت top = edge.top();
        𐡄𐡍 (top->color() == vtxp->color()) gatherSCCCandidates(top, candidates);
    }
    لكل (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* ثابت fromp = edge.fromp();
        𐡄𐡍 (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(FileLine* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
    std::ostringstream ss;
    // Vector of variables in UNOPTFLAT loop that are candidates for splitting.
    std::vector<Candidate> candidates;
    {
        // AstNode::user3 is used to mark if we have done a particular variable.
        // V3GraphVertex::user is used to mark if we have seen this vertex before.
        ثابت VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    𐡄𐡍 (candidates.empty()) أرجع "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    ثابت auto reportFirst10
        = [&](std::function<منطقي(ثابت Candidate&, ثابت Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        لكل (size_t i = 0; i < 10; i++) {
            𐡄𐡍 (i == candidates.size()) اكسر;
            ثابت Candidate& candidate = candidates[i];
            AstVar* ثابت varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            𐡄𐡍 (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        أرجع ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {
              أرجع a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {  //
              أرجع a.second > b.second;
          });

    𐡄𐡍 (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    أرجع ss.str();
}

فراغ reportCycles(Graph* graphp, ثابت std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    لكل (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* ثابت vscp = vvtxp->vscp();
        FileLine* ثابت flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        𐡄𐡍 (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } وإلا {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, 𐡒𐡔𐡈);
            // Create a subgraph for the UNOPTFLAT loop
            𐡄𐡍 (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

فراغ dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    لكل (V3GraphVertex& vtx : graphp->vertices()) {
        𐡄𐡍 (!vtx.color()) استمر;
        scc2Vtxps[vtx.color()].push_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    لكل (ثابت auto& pair : scc2Vtxps) {
        ثابت uint32_t color = pair.first;
        ثابت std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        ثابت std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        𐡄𐡍 (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        لكل (V3GraphVertex* vtxp : vtxps) {
            ثابت SchedAcyclicVarVertex* ثابت vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            𐡄𐡍 (!vvtxp) استمر;
            AstVarScope* ثابت vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debugVerilogForTree(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        لكل (V3GraphVertex* vtxp : vtxps) {
            ثابت SchedAcyclicLogicVertex* ثابت lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            𐡄𐡍 (!lvtxp) استمر;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debugVerilogForTree(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     ثابت std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        ثابت VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        لكل (SchedAcyclicVarVertex* ثابت vvtxp : cutVertices) {
            لكل (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* ثابت lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.top());
                𐡄𐡍 (!lvtxp->logicp()->user1SetOnce()) lvtxps.push_back(lvtxp);
                lvtx2Cuts[lvtxp].push_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    SenTreeFinder finder{netlistp};
    لكل (SchedAcyclicLogicVertex* ثابت lvtxp : lvtxps) {
        AstNode* ثابت logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        FileLine* ثابت flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        لكل (AstVarScope* ثابت vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ثابت refp = جديد AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ثابت nextp = جديد AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = AstNode::addNext(senItemsp, nextp);
        }
        AstSenTree* ثابت senTree = جديد AstSenTree{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.getSenTree(senTree), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(senTree->deleteTree(), senTree);
    }
    أرجع result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, ثابت LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    𐡄𐡍 (graphp->empty()) أرجع LogicByScope{};

    // Dump for debug
    𐡄𐡍 (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    ثابت std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    𐡄𐡍 (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    أرجع fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
