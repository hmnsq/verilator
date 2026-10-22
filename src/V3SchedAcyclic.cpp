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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3EmitV.h"
#شامل "V3File.h"
#شامل "V3Graph.h"
#شامل "V3Sched.h"
#شامل "V3SenTree.h"
#شامل "V3Stats.h"

#شامل <map>
#شامل <unordered_map>
#شامل <unordered_set>
#شامل <utility>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

فضاینام V3Sched {

فضاینام {

// ##############################################################################
//  Data structures (graph types)

رده SchedAcyclicLogicVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    Ast_گره* ثابت m_logicp;  // The logic node this vertex represents
    AstScope* ثابت m_scopep;  // The enclosing AstScope of the logic node

عمومی:
    SchedAcyclicLogicVertex(V3Graph* graphp, Ast_گره* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) ثابت override {
        بازگشت جدید SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    Ast_گره* logicp() ثابت { بازگشت m_logicp; }
    AstScope* scopep() ثابت { بازگشت m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { بازگشت m_logicp->fileline()->ascii(); };
    string dotShape() ثابت override { بازگشت "rectangle2"; }
    // LCOV_EXCL_STOP
};

رده SchedAcyclicVarVertex final : عمومی V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* ثابت m_vscp;  // The AstVarScope this vertex represents

عمومی:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() ثابت { بازگشت m_vscp; }
    AstVar* varp() ثابت { بازگشت m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) ثابت override {
        بازگشت جدید SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE { بازگشت m_vscp->name(); }
    string dotShape() ثابت override { بازگشت "ellipse"; }
    string dotColor() ثابت override { بازگشت "blue"; }
    // LCOV_EXCL_STOP
};

رده Graph final : عمومی V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        اگر (SchedAcyclicLogicVertex* ثابت lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            Ast_گره* ثابت logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            اگر (ثابت AstAlways* ثابت alwaysp = VN_CAST(logicp, Always)) {
                اگر (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            بازگشت logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } وگرنه {
            SchedAcyclicVarVertex* ثابت vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* ثابت vscp = vvtxp->vscp();
            بازگشت vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(ثابت LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{جدید Graph};

    // AstVarScope::user1() -> VarVertx
    ثابت VNUser1InUse user1InUse;
    ثابت auto getVarVertex = [&](AstVarScope* vscp) {
        اگر (!vscp->user1p()) vscp->user1p(جدید SchedAcyclicVarVertex{graphp.get(), vscp});
        بازگشت vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    ثابت auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* بالاترین, صحیح weight, منطقی cuttable) {
        جدید V3GraphEdge{graphp.get(), fromp, بالاترین, weight, cuttable};
    };

    برای (ثابت auto& entry : lbs) {
        AstScope* ثابت scopep = entry.m_scopep;
        AstActive* ثابت activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        برای (Ast_گره* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            اگر (VN_IS(nodep, AlwaysPostponed)) ادامه;

            SchedAcyclicLogicVertex* ثابت lvtxp
                = جدید SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            ثابت VNUser2InUse user2InUse;
            ثابت VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                SchedAcyclicVarVertex* ثابت vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                ثابت صحیح weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                اگر (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, درست);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                اگر (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.شمار(vscp))
                    addEdge(vvtxp, lvtxp, weight, درست);
            });
        }
    }

    بازگشت graphp;
}

تهی removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    ثابت auto enqueue = [&](V3GraphVertex* vtxp) {
        اگر (vtxp->user()) بازگشت;  // Already in queue
        vtxp->user(1);
        queue.افزودن_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    برای (V3GraphVertex& vtx : graphp->vertices()) {
        اگر (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    وقتی (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* ثابت vtxp = queue.back();
        queue.برداشت_back();
        vtxp->user(0);  // No longer in queue

        اگر (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            برای (V3GraphEdge* ثابت edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->بالاترین());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } وگرنه اگر (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            برای (V3GraphEdge* ثابت edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
منطقی isCut(ثابت SchedAcyclicVarVertex* vtxp) {
    برای (ثابت V3GraphEdge& edge : vtxp->inEdges()) {
        اگر (edge.weight() == 0) بازگشت درست;
    }
    برای (ثابت V3GraphEdge& edge : vtxp->outEdges()) {
        اگر (edge.weight() == 0) بازگشت درست;
    }
    بازگشت نادرست;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    ثابت VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    برای (V3GraphVertex& vtx : graphp->vertices()) {
        اگر (!vtx.color()) ادامه;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        اگر (SchedAcyclicVarVertex* ثابت vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            اگر (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.افزودن_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        اگر (v3Global.opt.stats()) sccs.درج(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    بازگشت result;
}

تهی resetEdgeWeights(ثابت std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    برای (SchedAcyclicVarVertex* ثابت vvtxp : cutVertices) {
        برای (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        برای (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
تهی gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    اگر (vtxp->user()) بازگشت;  // Already done
    vtxp->user(درست);

    اگر (SchedAcyclicVarVertex* ثابت vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* ثابت varp = vvtxp->varp();
        ثابت string name = varp->prettyName();
        اگر (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            ثابت unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    برای (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* ثابت بالاترین = edge.بالاترین();
        اگر (بالاترین->color() == vtxp->color()) gatherSCCCandidates(بالاترین, candidates);
    }
    برای (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* ثابت fromp = edge.fromp();
        اگر (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
    }
}

// Find all variables in a loop (SCC) that are candidates for splitting to break loops.
std::string reportLoopVars(File_سطر* /*warnFl*/, Graph* graphp, SchedAcyclicVarVertex* vvtxp) {
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
    اگر (candidates.empty()) بازگشت "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    ثابت auto reportFirst10
        = [&](std::function<منطقی(ثابت Candidate&, ثابت Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        برای (size_t i = 0; i < 10; i++) {
            اگر (i == candidates.size()) بشکن;
            ثابت Candidate& candidate = candidates[i];
            AstVar* ثابت varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            اگر (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        بازگشت ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {
              بازگشت a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](ثابت Candidate& a, ثابت Candidate& b) {  //
              بازگشت a.second > b.second;
          });

    اگر (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    بازگشت ss.str();
}

تهی reportCycles(Graph* graphp, ثابت std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    برای (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* ثابت vscp = vvtxp->vscp();
        File_سطر* ثابت flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        اگر (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } وگرنه {
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
            flp->modifyWarnOff(V3ErrorCode::UNOPTFLAT, درست);
            // Create a subgraph for the UNOPTFLAT loop
            اگر (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

تهی dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    برای (V3GraphVertex& vtx : graphp->vertices()) {
        اگر (!vtx.color()) ادامه;
        scc2Vtxps[vtx.color()].افزودن_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    برای (ثابت auto& pair : scc2Vtxps) {
        ثابت uint32_t color = pair.first;
        ثابت std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        ثابت std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        اگر (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        برای (V3GraphVertex* vtxp : vtxps) {
            ثابت SchedAcyclicVarVertex* ثابت vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            اگر (!vvtxp) ادامه;
            AstVarScope* ثابت vscp = vvtxp->vscp();
            *ofp << "// " << vscp->fileline()->ascii() << "\n";
            *ofp << "// " << vscp->prettyName() << "\n";
            V3EmitV::debug_Verilog_For_درخت(vscp->varp(), *ofp);
            *ofp << "\n";
        }

        // Dump logic
        *ofp << "\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Logic\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        برای (V3GraphVertex* vtxp : vtxps) {
            ثابت SchedAcyclicLogicVertex* ثابت lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            اگر (!lvtxp) ادامه;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debug_Verilog_For_درخت(lvtxp->logicp(), *ofp);
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
        برای (SchedAcyclicVarVertex* ثابت vvtxp : cutVertices) {
            برای (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* ثابت lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.بالاترین());
                اگر (!lvtxp->logicp()->user1SetOnce()) lvtxps.افزودن_back(lvtxp);
                lvtx2Cuts[lvtxp].افزودن_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    Sen_درخت_Finder finder{netlistp};
    برای (SchedAcyclicLogicVertex* ثابت lvtxp : lvtxps) {
        Ast_گره* ثابت logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        File_سطر* ثابت flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        برای (AstVarScope* ثابت vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* ثابت refp = جدید AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* ثابت nextp = جدید AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = Ast_گره::addNext(senItemsp, nextp);
        }
        Ast_Sen_درخت* ثابت sen_درخت = جدید Ast_Sen_درخت{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.get_Sen_درخت(sen_درخت), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(sen_درخت->delete_درخت(), sen_درخت);
    }
    بازگشت result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, ثابت LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    ثابت std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    اگر (graphp->empty()) بازگشت LogicByScope{};

    // Dump for debug
    اگر (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

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
    اگر (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    بازگشت fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
