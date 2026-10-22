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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3EmitV.h"
#include "V3File.h"
#include "V3Graph.h"
#include "V3Sched.h"
#include "V3SenTree.h"
#include "V3Stats.h"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

// ##############################################################################
//  Data structures (graph types)

class SchedAcyclicLogicVertex final : public V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicLogicVertex, V3GraphVertex)
    AstNode* const m_logicp;  // The logic node this vertex represents
    AstScope* const m_scopep;  // The enclosing AstScope of the logic node

public:
    SchedAcyclicLogicVertex(V3Graph* graphp, AstNode* logicp, AstScope* scopep)
        : V3GraphVertex{graphp}
        , m_logicp{logicp}
        , m_scopep{scopep} {}
    V3GraphVertex* clone(V3Graph* graphp) const override {
        ආපසු new SchedAcyclicLogicVertex{graphp, logicp(), scopep()};
    }

    AstNode* logicp() const { ආපසු m_logicp; }
    AstScope* scopep() const { ආපසු m_scopep; }

    // LCOV_EXCL_START // Debug code
    string name() const override VL_MT_STABLE { ආපසු m_logicp->fileline()->ascii(); };
    string dotShape() const override { ආපසු "rectangle2"; }
    // LCOV_EXCL_STOP
};

class SchedAcyclicVarVertex final : public V3GraphVertex {
    VL_RTTI_IMPL(SchedAcyclicVarVertex, V3GraphVertex)
    AstVarScope* const m_vscp;  // The AstVarScope this vertex represents

public:
    SchedAcyclicVarVertex(V3Graph* graphp, AstVarScope* vscp)
        : V3GraphVertex{graphp}
        , m_vscp{vscp} {}
    AstVarScope* vscp() const { ආපසු m_vscp; }
    AstVar* varp() const { ආපසු m_vscp->varp(); }
    V3GraphVertex* clone(V3Graph* graphp) const override {
        ආපසු new SchedAcyclicVarVertex{graphp, vscp()};
    }

    // LCOV_EXCL_START // Debug code
    string name() const override VL_MT_STABLE { ආපසු m_vscp->name(); }
    string dotShape() const override { ආපසු "ellipse"; }
    string dotColor() const override { ආපසු "blue"; }
    // LCOV_EXCL_STOP
};

class Graph final : public V3Graph {
    string loopsVertexCb(V3GraphVertex* vtxp) override {
        නම් (SchedAcyclicLogicVertex* const lvtxp = vtxp->cast<SchedAcyclicLogicVertex>()) {
            AstNode* const logicp = lvtxp->logicp();
            std::string logicName = logicp->prettyTypeName();
            නම් (const AstAlways* const alwaysp = VN_CAST(logicp, Always)) {
                නම් (alwaysp->keyword() == VAlwaysKwd::CONT_ASSIGN) {
                    logicName = "ASSIGNW";  // Keep using historiacl name until we have better
                }
            }
            ආපසු logicp->fileline()->warnOther() + "     Example path: " + logicName + "\n";
        } නැතහොත් {
            SchedAcyclicVarVertex* const vvtxp = vtxp->as<SchedAcyclicVarVertex>();
            AstVarScope* const vscp = vvtxp->vscp();
            ආපසු vscp->fileline()->warnOther() + "     Example path: " + vscp->prettyName()
                   + "\n";
        }
    }
};

//##############################################################################
// Algorithm implementation

std::unique_ptr<Graph> buildGraph(const LogicByScope& lbs) {
    std::unique_ptr<Graph> graphp{new Graph};

    // AstVarScope::user1() -> VarVertx
    const VNUser1InUse user1InUse;
    const auto getVarVertex = [&](AstVarScope* vscp) {
        නම් (!vscp->user1p()) vscp->user1p(new SchedAcyclicVarVertex{graphp.get(), vscp});
        ආපසු vscp->user1u().to<SchedAcyclicVarVertex*>();
    };

    const auto addEdge = [&](V3GraphVertex* fromp, V3GraphVertex* top, පූර්ණ_සංඛ්‍යා weight, bool cuttable) {
        new V3GraphEdge{graphp.get(), fromp, top, weight, cuttable};
    };

    සඳහා (const auto& entry : lbs) {
        AstScope* const scopep = entry.m_scopep;
        AstActive* const activep = entry.m_activep;
        UASSERT_OBJ(activep->hasCombo(), activep, "Not combinational logic");
        සඳහා (AstNode* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
            // Can safely ignore Postponed as we generate them all
            නම් (VN_IS(nodep, AlwaysPostponed)) continue;

            SchedAcyclicLogicVertex* const lvtxp
                = new SchedAcyclicLogicVertex{graphp.get(), nodep, scopep};
            const VNUser2InUse user2InUse;
            const VNUser3InUse user3InUse;

            V3Sched::util::VarScopeSet forceReadEdgeIgnores;
            V3Sched::util::collectForceReadEdgeIgnores(nodep, forceReadEdgeIgnores);

            nodep->foreach([&](AstVarRef* refp) {
                AstVarScope* const vscp = refp->varScopep();
                SchedAcyclicVarVertex* const vvtxp = getVarVertex(vscp);
                // We want to cut the narrowest signals
                const පූර්ණ_සංඛ්‍යා weight = vscp->width() / 8 + 1;
                // If written, add logic -> var edge
                නම් (refp->access().isWriteOrRW() && !refp->varp()->ignoreSchedWrite()
                    && !vscp->user2SetOnce())
                    addEdge(lvtxp, vvtxp, weight, true);
                // If read, add var -> logic edge
                // Note: Use same heuristic as ordering does to ignore written variables
                // TODO: Use live variable analysis.
                නම් (refp->access().isReadOrRW() && !vscp->user3SetOnce() && !vscp->user2()
                    && !forceReadEdgeIgnores.count(vscp))
                    addEdge(vvtxp, lvtxp, weight, true);
            });
        }
    }

    ආපසු graphp;
}

void removeNonCyclic(Graph* graphp) {
    // Work queue
    std::vector<V3GraphVertex*> queue;

    const auto enqueue = [&](V3GraphVertex* vtxp) {
        නම් (vtxp->user()) ආපසු;  // Already in queue
        vtxp->user(1);
        queue.push_back(vtxp);
    };

    // Start with vertices with no inputs or outputs
    සඳහා (V3GraphVertex& vtx : graphp->vertices()) {
        නම් (vtx.inEmpty() || vtx.outEmpty()) enqueue(&vtx);
    }

    // Iterate while we still have candidates
    අතරතුර (!queue.empty()) {
        // Pop next candidate
        V3GraphVertex* const vtxp = queue.back();
        queue.pop_back();
        vtxp->user(0);  // No longer in queue

        නම් (vtxp->inEmpty()) {
            // Enqueue children for consideration, remove out edges, and delete this vertex
            සඳහා (V3GraphEdge* const edgep : vtxp->outEdges().unlinkable()) {
                enqueue(edgep->top());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        } නැතහොත් නම් (vtxp->outEmpty()) {
            // Enqueue parents for consideration, remove in edges, and delete this vertex
            සඳහා (V3GraphEdge* const edgep : vtxp->inEdges().unlinkable()) {
                enqueue(edgep->fromp());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            VL_DO_DANGLING(vtxp->unlinkDelete(graphp), vtxp);
        }
    }
}

// Has this VarVertex been cut? (any edges in or out has been cut)
bool isCut(const SchedAcyclicVarVertex* vtxp) {
    සඳහා (const V3GraphEdge& edge : vtxp->inEdges()) {
        නම් (edge.weight() == 0) ආපසු true;
    }
    සඳහා (const V3GraphEdge& edge : vtxp->outEdges()) {
        නම් (edge.weight() == 0) ආපසු true;
    }
    ආපසු false;
}

std::vector<SchedAcyclicVarVertex*> findCutVertices(Graph* graphp) {
    // List of cut vertices being computed here
    std::vector<SchedAcyclicVarVertex*> result;
    const VNUser1InUse user1InUse;  // bool: already added to result

    // Statistics
    size_t nCyclicVtxs = 0;  // Number of vertices that are part of an SCC (cycle)
    size_t nCyclicVars = 0;  // Number of variables that are part of an SCC (cycle)
    std::unordered_set<uint32_t> sccs;  // Unique SCC colors

    සඳහා (V3GraphVertex& vtx : graphp->vertices()) {
        නම් (!vtx.color()) continue;  // Not part of an SCC (cycle), can ignore
        ++nCyclicVtxs;
        නම් (SchedAcyclicVarVertex* const vvtxp = vtx.cast<SchedAcyclicVarVertex>()) {
            ++nCyclicVars;
            නම් (!vvtxp->vscp()->user1SetOnce() && isCut(vvtxp)) result.push_back(vvtxp);
        }
        // Don't bother counting if not dumping statistics
        නම් (v3Global.opt.stats()) sccs.insert(vtx.color());
    }

    V3Stats::addStat("Scheduling, Cycles, cyclic variables", nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, cyclic logic blocks", nCyclicVtxs - nCyclicVars);
    V3Stats::addStat("Scheduling, Cycles, unique SCCs", sccs.size());
    V3Stats::addStat("Scheduling, Cycles, cut variables", result.size());

    ආපසු result;
}

void resetEdgeWeights(const std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    සඳහා (SchedAcyclicVarVertex* const vvtxp : cutVertices) {
        සඳහා (V3GraphEdge& e : vvtxp->inEdges()) e.weight(1);
        සඳහා (V3GraphEdge& e : vvtxp->outEdges()) e.weight(1);
    }
}

// A VarVertex together with its fanout
using Candidate = std::pair<SchedAcyclicVarVertex*, unsigned>;

// Gather all splitting candidates that are in the same SCC as the given vertex
void gatherSCCCandidates(V3GraphVertex* vtxp, std::vector<Candidate>& candidates) {
    නම් (vtxp->user()) ආපසු;  // Already done
    vtxp->user(true);

    නම් (SchedAcyclicVarVertex* const vvtxp = vtxp->cast<SchedAcyclicVarVertex>()) {
        AstVar* const varp = vvtxp->varp();
        const string name = varp->prettyName();
        නම් (!varp->user3SetOnce()  // Only consider each AstVar once
            && varp->width() != 1  // Ignore 1-bit signals (they cannot be split further)
            && name.find("__Vdly") == string::npos  // Ignore internal signals
            && name.find("__Vcell") == string::npos) {
            // Also compute the fanout of this vertex
            const unsigned fanout = vtxp->outEdges().size();
            candidates.emplace_back(vvtxp, fanout);
        }
    }

    // Iterate through all the vertices within the same strongly connected component (same color)
    සඳහා (V3GraphEdge& edge : vtxp->outEdges()) {
        V3GraphVertex* const top = edge.top();
        නම් (top->color() == vtxp->color()) gatherSCCCandidates(top, candidates);
    }
    සඳහා (V3GraphEdge& edge : vtxp->inEdges()) {
        V3GraphVertex* const fromp = edge.fromp();
        නම් (fromp->color() == vtxp->color()) gatherSCCCandidates(fromp, candidates);
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
        const VNUser3InUse user3InUse;
        graphp->userClearVertices();
        gatherSCCCandidates(vvtxp, candidates);
        graphp->userClearVertices();
    }

    // Possible we only have candidates the user cannot do anything about, so don't bother them.
    නම් (candidates.empty()) ආපසු "";

    // There may be a very large number of candidates, so only report up to 10 of the "most
    // important" signals.
    unsigned splittable = 0;
    const auto reportFirst10
        = [&](std::function<bool(const Candidate&, const Candidate&)> less) -> string {
        std::stable_sort(candidates.begin(), candidates.end(), less);
        std::ostringstream ss2;
        සඳහා (size_t i = 0; i < 10; i++) {
            නම් (i == candidates.size()) bha_nga;
            const Candidate& candidate = candidates[i];
            AstVar* const varp = candidate.first->varp();

            ss2 << V3Error::warnMore() << "    " << varp->fileline() << ' ' << varp->prettyName()
                << ", width " << std::dec << varp->width() << ", circular fanout "
                << candidate.second;
            නම් (!varp->cannotSplitKindReason()) {
                ss2 << ", can split_var";
                ++splittable;
            }
            ss2 << '\n';
        }
        ආපසු ss2.str();
    };

    // Widest variables
    ss << V3Error::warnMore() << "... Widest variables candidate to splitting:\n"
       << reportFirst10([](const Candidate& a, const Candidate& b) {
              ආපසු a.first->varp()->width() > b.first->varp()->width();
          });

    // Highest fanout
    ss << V3Error::warnMore() << "... Candidates with the highest fanout:\n"
       << reportFirst10([](const Candidate& a, const Candidate& b) {  //
              ආපසු a.second > b.second;
          });

    නම් (splittable) {
        ss << V3Error::warnMore()
           << "... Suggest add /*verilator split_var*/ to appropriate variables above.\n";
    }
    V3Stats::addStat("Scheduling, split_var, candidates", splittable);
    ආපසු ss.str();
}

void reportCycles(Graph* graphp, const std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    සඳහා (SchedAcyclicVarVertex* vvtxp : cutVertices) {
        AstVarScope* const vscp = vvtxp->vscp();
        FileLine* const flp = vscp->fileline();

        // First v3warn not inside warnIsOff so we can see the suppressions with --debug
        නම් (flp->warnIsOff(V3ErrorCode::UNOPTFLAT)) {
            // First v3warn not inside warnIsOff so we can see the suppressions with --debug
            vscp->v3warn(UNOPTFLAT, "Signal unoptimizable: Circular combinational logic: "
                                        << vscp->prettyNameQ());
        } නැතහොත් {
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
            නම් (v3Global.opt.reportUnoptflat()) {
                V3Graph loopGraph;
                graphp->subtreeLoops(&V3GraphEdge::followAlwaysTrue, vvtxp, &loopGraph);
                loopGraph.dumpDotFilePrefixedAlways("unoptflat");
            }
        }
    }
}

void dumpSccs(V3Graph* graphp) {
    // Map from SCC color to vertices in that SCC
    std::map<uint32_t, std::vector<V3GraphVertex*>> scc2Vtxps;

    // Gather all vertices in each SCC
    සඳහා (V3GraphVertex& vtx : graphp->vertices()) {
        නම් (!vtx.color()) continue;
        scc2Vtxps[vtx.color()].push_back(&vtx);
    }

    // Dump Verilog for each SCC into separate files
    සඳහා (const auto& pair : scc2Vtxps) {
        const uint32_t color = pair.first;
        const std::vector<V3GraphVertex*>& vtxps = pair.second;

        // Open dump file
        const std::string fname
            = v3Global.debugFilename("sched_scc_" + std::to_string(color) + ".v");
        const std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(fname)};
        නම් (ofp->fail()) v3fatal("Can't write file: " << fname);

        // Write header
        *ofp << "// SCC " << color << ", size: " << vtxps.size() << "\n\n";

        // Dump variables
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "// Variables\n";
        *ofp << "//////////////////////////////////////////////////////////////////////\n";
        *ofp << "\n";
        සඳහා (V3GraphVertex* vtxp : vtxps) {
            const SchedAcyclicVarVertex* const vvtxp = vtxp->cast<SchedAcyclicVarVertex>();
            නම් (!vvtxp) continue;
            AstVarScope* const vscp = vvtxp->vscp();
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
        සඳහා (V3GraphVertex* vtxp : vtxps) {
            const SchedAcyclicLogicVertex* const lvtxp = vtxp->cast<SchedAcyclicLogicVertex>();
            නම් (!lvtxp) continue;
            *ofp << "// " << lvtxp->logicp()->fileline()->ascii() << "\n";
            V3EmitV::debugVerilogForTree(lvtxp->logicp(), *ofp);
            *ofp << "\n";
        }
    }
}

LogicByScope fixCuts(AstNetlist* netlistp,
                     const std::vector<SchedAcyclicVarVertex*>& cutVertices) {
    // For all logic that reads a cut vertex, build a map from logic -> list of cut AstVarScope
    // they read. Also build a vector of the involved logic for deterministic results.
    std::unordered_map<SchedAcyclicLogicVertex*, std::vector<AstVarScope*>> lvtx2Cuts;
    std::vector<SchedAcyclicLogicVertex*> lvtxps;
    {
        const VNUser1InUse user1InUse;  // bool: already added to 'lvtxps'
        සඳහා (SchedAcyclicVarVertex* const vvtxp : cutVertices) {
            සඳහා (V3GraphEdge& edge : vvtxp->outEdges()) {
                SchedAcyclicLogicVertex* const lvtxp
                    = static_cast<SchedAcyclicLogicVertex*>(edge.top());
                නම් (!lvtxp->logicp()->user1SetOnce()) lvtxps.push_back(lvtxp);
                lvtx2Cuts[lvtxp].push_back(vvtxp->vscp());
            }
        }
    }

    // Make the logic reading cut vertices use a hybrid sensitivity (combinational, but with some
    // explicit additional triggers on the cut variables)
    LogicByScope result;
    SenTreeFinder finder{netlistp};
    සඳහා (SchedAcyclicLogicVertex* const lvtxp : lvtxps) {
        AstNode* const logicp = lvtxp->logicp();
        logicp->unlinkFrBack();
        FileLine* const flp = logicp->fileline();
        // Build the hybrid sensitivity list
        AstSenItem* senItemsp = nullptr;
        සඳහා (AstVarScope* const vscp : lvtx2Cuts[lvtxp]) {
            AstVarRef* const refp = new AstVarRef{flp, vscp, VAccess::READ};
            AstSenItem* const nextp = new AstSenItem{flp, VEdgeType::ET_HYBRID, refp};
            senItemsp = AstNode::addNext(senItemsp, nextp);
        }
        AstSenTree* const senTree = new AstSenTree{flp, senItemsp};
        // Add logic to result with new sensitivity
        result.add(lvtxp->scopep(), finder.getSenTree(senTree), logicp);
        // SenTreeFinder::getSenTree clones, so clean up
        VL_DO_DANGLING(senTree->deleteTree(), senTree);
    }
    ආපසු result;
}

}  // namespace

LogicByScope breakCycles(AstNetlist* netlistp, const LogicByScope& combinationalLogic) {
    // Build the dataflow (dependency) graph
    const std::unique_ptr<Graph> graphp = buildGraph(combinationalLogic);

    // Remove nodes that don't form part of a cycle
    removeNonCyclic(graphp.get());

    // Nothing to do if no cycles, yay!
    නම් (graphp->empty()) ආපසු LogicByScope{};

    // Dump for debug
    නම් (dumpGraphLevel() >= 6) graphp->dumpDotFilePrefixed("sched-comb-cycles");

    // Make graph acyclic by cutting some edges. Note: This also colors strongly connected
    // components which reportCycles uses to print each SCCs separately.
    // TODO: A more optimal algorithm that cuts by removing/marking VarVertex vertices is possible
    //       Search for "Feedback vertex set" (current algorithm is "Feedback arc set")
    graphp->acyclic(&V3GraphEdge::followAlwaysTrue);

    // Find all cut vertices
    const std::vector<SchedAcyclicVarVertex*> cutVertices = findCutVertices(graphp.get());

    // Reset edge weights for reporting
    resetEdgeWeights(cutVertices);

    // Report warnings/diagnostics
    reportCycles(graphp.get(), cutVertices);

    // Debug dump
    නම් (dumpLevel() >= 6) dumpSccs(graphp.get());

    // Fix cuts by converting dependent logic to use hybrid sensitivities
    ආපසු fixCuts(netlistp, cutVertices);
}

}  // namespace V3Sched
