// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph acyclic algorithm
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

#define VL_MT_DISABLED_CODE_UNIT 1

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Algorithms - acyclic
//      Break the minimal number of backward edges to make the graph acyclic

ଶ୍ରେଣୀ GraphAcycVertex final : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(GraphAcycVertex, V3GraphVertex)
    // user() is used for various sub-algorithm pieces
    V3GraphVertex* ଅଚଳ m_origVertexp;  // Pointer to first vertex this represents
ସୁରକ୍ଷିତ:
    friend ଶ୍ରେଣୀ GraphAcyc;
    V3ListLinks<GraphAcycVertex> m_links;  // List links to store instances of this class
    uint32_t m_storedRank = 0;  // Rank held until commit to edge placement
    ତାର୍କିକ m_onWorkList = ମିଥ୍ୟା;  // True if already on list of work to do
    ତାର୍କିକ m_deleted = ମିଥ୍ୟା;  // True if deleted

ବ୍ୟକ୍ତିଗତ:
    V3ListLinks<GraphAcycVertex>& links() { ଫେରନ୍ତୁ m_links; }

ସର୍ବସାଧାରଣ:
    // List type to store instances of this class
    using List = V3List<GraphAcycVertex, &GraphAcycVertex::links>;

    GraphAcycVertex(V3Graph* graphp, V3GraphVertex* origVertexp)
        : V3GraphVertex{graphp}
        , m_origVertexp{origVertexp} {}
    ~GraphAcycVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    V3GraphVertex* origVertexp() ଅଚଳ { ଫେରନ୍ତୁ m_origVertexp; }
    ଶୂନ୍ୟ setDelete() { m_deleted = ସତ୍ୟ; }
    ତାର୍କିକ isDelete() ଅଚଳ { ଫେରନ୍ତୁ m_deleted; }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string name() ଅଚଳ override { ଫେରନ୍ତୁ m_origVertexp->name(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ m_origVertexp->dotColor(); }
    // cppcheck-suppress uselessOverride // cppcheck is wrong ...
    File_ଧାଡ଼ି* fileline() ଅଚଳ override { ଫେରନ୍ତୁ m_origVertexp->fileline(); }
};

//--------------------------------------------------------------------

ଶ୍ରେଣୀ GraphAcycEdge final : ସର୍ବସାଧାରଣ V3GraphEdge {
    VL_RTTI_IMPL(GraphAcycEdge, V3GraphEdge)
    // userp() is always used to point to the head original graph edge
ବ୍ୟକ୍ତିଗତ:
    using OrigEdgeList = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcyc's decl
    V3GraphEdge* origEdgep() ଅଚଳ {
        ଅଚଳ OrigEdgeList* ଅଚଳ oEListp = static_cast<OrigEdgeList*>(userp());
        UASSERT(oEListp, "No original edge associated with acyc edge " << ଏହି);
        ଫେରନ୍ତୁ (oEListp->front());
    }

ସର୍ବସାଧାରଣ:
    GraphAcycEdge(V3Graph* graphp, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା weight,
                  ତାର୍କିକ cutable = ମିଥ୍ୟା)
        : V3GraphEdge{graphp, fromp, ଶୀର୍ଷ, weight, cutable} {}
    ~GraphAcycEdge() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // yellow=we might still cut it, else oldEdge: yellowGreen=made uncutable, red=uncutable
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ (cutable() ? "yellow" : origEdgep()->dotColor()); }
};

//--------------------------------------------------------------------

ସଂରଚନା GraphAcycEdgeCmp final {
    ତାର୍କିକ operator()(ଅଚଳ V3GraphEdge* lhsp, ଅଚଳ V3GraphEdge* rhsp) ଅଚଳ {
        ଯଦି (lhsp->weight() > rhsp->weight()) ଫେରନ୍ତୁ ସତ୍ୟ;  // LHS goes first
        ଯଦି (lhsp->weight() < rhsp->weight()) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // RHS goes first
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
};

//--------------------------------------------------------------------

// CLASSES
ଶ୍ରେଣୀ GraphAcyc final {
    using OrigEdgeList
        = std::list<V3GraphEdge*>;  // List of orig edges, see also GraphAcycEdge's decl
    // GRAPH USERS
    //  origGraph
    //    GraphVertex::user()   GraphAycVerted* New graph node
    //  m_breakGraph
    //    GraphEdge::user()     OrigEdgeList*   Old graph edges
    //    GraphVertex::user     bool            Detection of loops in simplifyDupIterate
    // MEMBERS
    V3Graph* ଅଚଳ m_origGraphp;  // Original graph
    V3Graph m_breakGraph;  // Graph with only breakable edges represented
    GraphAcycVertex::List m_work;  // List of vertices with optimization work left
    std::vector<OrigEdgeList*> m_origEdgeDelp;  // List of deletions to do when done
    ଅଚଳ V3EdgeFuncP
        m_origEdgeFuncp;  // Function that says we follow this edge (in original graph)
    uint32_t m_placeStep = 0;  // Number that user() must be equal to to indicate processing

    // METHODS
    ଶୂନ୍ୟ buildGraph(V3Graph* origGraphp);
    ଶୂନ୍ୟ buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp);
    ଶୂନ୍ୟ simplify(ତାର୍କିକ allowCut);
    ଶୂନ୍ୟ simplifyNone(GraphAcycVertex* avertexp);
    ଶୂନ୍ୟ simplifyOne(GraphAcycVertex* avertexp);
    ଶୂନ୍ୟ simplifyOut(GraphAcycVertex* avertexp);
    ଶୂନ୍ୟ simplifyDup(GraphAcycVertex* avertexp);
    ଶୂନ୍ୟ cutBasic(GraphAcycVertex* avertexp);
    ଶୂନ୍ୟ cutBackward(GraphAcycVertex* avertexp);
    ଶୂନ୍ୟ deleteMarked();
    ଶୂନ୍ୟ place();
    ଶୂନ୍ୟ placeTryEdge(V3GraphEdge* edgep);
    ତାର୍କିକ placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank);

    ତାର୍କିକ origFollowEdge(V3GraphEdge* edgep) {
        ଫେରନ୍ତୁ (edgep->weight() && (m_origEdgeFuncp)(edgep));
    }
    ଶୂନ୍ୟ edgeFromEdge(V3GraphEdge* oldedgep, V3GraphVertex* fromp, V3GraphVertex* ଶୀର୍ଷ) {
        // Make new breakGraph edge, with old edge as a template
        GraphAcycEdge* ଅଚଳ newEdgep = ନୂତନ GraphAcycEdge{&m_breakGraph, fromp, ଶୀର୍ଷ,
                                                          oldedgep->weight(), oldedgep->cutable()};
        newEdgep->userp(oldedgep->userp());  // Keep pointer to OrigEdgeList
    }
    ଶୂନ୍ୟ addOrigEdgep(V3GraphEdge* toEdgep, V3GraphEdge* addEdgep) {
        // Add addEdge (or it's list) to list of edges that break edge represents
        // Note addEdge may already have a bunch of similar linked edge representations.  Yuk.
        UASSERT(addEdgep, "Adding nullptr");
        ଯଦି (!toEdgep->userp()) {
            OrigEdgeList* ଅଚଳ oep = ନୂତନ OrigEdgeList;
            m_origEdgeDelp.ଠେଲ_back(oep);
            toEdgep->userp(oep);
        }
        OrigEdgeList* ଅଚଳ oEListp = static_cast<OrigEdgeList*>(toEdgep->userp());
        ଯଦି (OrigEdgeList* ଅଚଳ addListp = static_cast<OrigEdgeList*>(addEdgep->userp())) {
            ପାଇଁ (ଅଚଳ auto& itr : *addListp) oEListp->ଠେଲ_back(itr);
            addListp->clear();  // Done with it
        } ଅନ୍ୟଥା {
            oEListp->ଠେଲ_back(addEdgep);
        }
    }
    ଶୂନ୍ୟ cutOrigEdge(V3GraphEdge* breakEdgep, ଅଚଳ ଅକ୍ଷର* why) {
        // From the break edge, cut edges in original graph it represents
        UINFO(8, why << " CUT " << breakEdgep->fromp());
        breakEdgep->cut();
        ଅଚଳ OrigEdgeList* ଅଚଳ oEListp = static_cast<OrigEdgeList*>(breakEdgep->userp());
        ଯଦି (!oEListp) {
            v3fatalSrc("No original edge associated with cutting edge " << breakEdgep);
        }
        // The breakGraph edge may represent multiple real edges; cut them all
        ପାଇଁ (ଅଚଳ auto& origEdgep : *oEListp) {
            origEdgep->cut();
            UINFO(8, "  " << why << "   " << origEdgep->fromp() << " ->" << origEdgep->ଶୀର୍ଷ());
        }
    }
    // Work Queue
    ଶୂନ୍ୟ work_ଠେଲ(V3GraphVertex* vertexp) {
        GraphAcycVertex* ଅଚଳ avertexp = static_cast<GraphAcycVertex*>(vertexp);
        // Add vertex to list of nodes needing further optimization trials
        ଯଦି (!avertexp->m_onWorkList) {
            avertexp->m_onWorkList = ସତ୍ୟ;
            m_work.linkBack(avertexp);
        }
    }
    GraphAcycVertex* workBeginp() { ଫେରନ୍ତୁ m_work.frontp(); }
    ଶୂନ୍ୟ work_କାଢ଼() {
        GraphAcycVertex* ଅଚଳ avertexp = workBeginp();
        avertexp->m_onWorkList = ମିଥ୍ୟା;
        m_work.unlink(avertexp);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    GraphAcyc(V3Graph* origGraphp, V3EdgeFuncP edgeFuncp)
        : m_origGraphp{origGraphp}
        , m_origEdgeFuncp{edgeFuncp} {}
    ~GraphAcyc() {
        ପାଇଁ (OrigEdgeList* ip : m_origEdgeDelp) delete ip;
        m_origEdgeDelp.clear();
    }
    ଶୂନ୍ୟ ମୁଖ୍ୟ();
};

//--------------------------------------------------------------------

ଶୂନ୍ୟ GraphAcyc::buildGraph(V3Graph* origGraphp) {
    // Presumes the graph has been strongly ordered,
    // and thus there's a unique color if there are loops in this subgraph.

    // For each old node, make a new graph node for optimization
    origGraphp->userClearVertices();
    origGraphp->userClearEdges();
    ପାଇଁ (V3GraphVertex& overtex : origGraphp->vertices()) {
        ଯଦି (overtex.color()) {
            GraphAcycVertex* ଅଚଳ avertexp = ନୂତନ GraphAcycVertex{&m_breakGraph, &overtex};
            overtex.userp(avertexp);  // Stash so can look up later
        }
    }

    // Build edges between logic vertices
    ପାଇଁ (V3GraphVertex& overtex : origGraphp->vertices()) {
        ଯଦି (overtex.color()) {
            GraphAcycVertex* ଅଚଳ avertexp = static_cast<GraphAcycVertex*>(overtex.userp());
            buildGraphIterate(&overtex, avertexp);
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::buildGraphIterate(V3GraphVertex* overtexp, GraphAcycVertex* avertexp) {
    // Make new edges
    ପାଇଁ (V3GraphEdge& edge : overtexp->outEdges()) {
        ଯଦି (origFollowEdge(&edge)) {  // not cut
            ଅଚଳ V3GraphVertex* toVertexp = edge.ଶୀର୍ଷ();
            ଯଦି (toVertexp->color()) {
                GraphAcycVertex* ଅଚଳ toAVertexp
                    = static_cast<GraphAcycVertex*>(toVertexp->userp());
                // Replicate the old edge into the new graph
                // There may be multiple edges between same pairs of vertices
                V3GraphEdge* breakEdgep = ନୂତନ GraphAcycEdge{&m_breakGraph, avertexp, toAVertexp,
                                                            edge.weight(), edge.cutable()};
                addOrigEdgep(breakEdgep, &edge);  // So can find original edge
            }
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::simplify(ତାର୍କିକ allowCut) {
    // Add all nodes to list of work to do
    ପାଇଁ (V3GraphVertex& vertex : m_breakGraph.vertices()) work_ଠେଲ(&vertex);
    // Optimize till everything finished
    ଯେତେବେଳେ (GraphAcycVertex* vertexp = workBeginp()) {
        work_କାଢ଼();
        simplifyNone(vertexp);
        simplifyOne(vertexp);
        simplifyOut(vertexp);
        simplifyDup(vertexp);
        ଯଦି (allowCut) {
            // The main algorithm works without these, though slower
            // So if changing the main algorithm, comment these out for a test run
            ଯଦି (v3Global.opt.fAcycSimp()) {
                cutBasic(vertexp);
                cutBackward(vertexp);
            }
        }
    }
    deleteMarked();
}

ଶୂନ୍ୟ GraphAcyc::deleteMarked() {
    // Delete nodes marked for removal
    ପାଇଁ (V3GraphVertex* ଅଚଳ vtxp : m_breakGraph.vertices().unlinkable()) {
        GraphAcycVertex* ଅଚଳ avertexp = static_cast<GraphAcycVertex*>(vtxp);
        ଯଦି (avertexp->isDelete()) {
            VL_DO_DANGLING(avertexp->unlinkDelete(&m_breakGraph), avertexp);
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::simplifyNone(GraphAcycVertex* avertexp) {
    // Don't need any vertices with no inputs, There's no way they can have a loop.
    // Likewise, vertices with no outputs
    ଯଦି (avertexp->isDelete()) ଫେରନ୍ତୁ;
    ଯଦି (avertexp->inEmpty() || avertexp->outEmpty()) {
        UINFO(9, "  SimplifyNoneRemove " << avertexp);
        avertexp->setDelete();  // Mark so we won't delete it twice
        // Remove edges
        ଯେତେବେଳେ (V3GraphEdge* ଅଚଳ edgep = avertexp->outEdges().frontp()) {
            work_ଠେଲ(edgep->ଶୀର୍ଷ());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
        ଯେତେବେଳେ (V3GraphEdge* ଅଚଳ edgep = avertexp->inEdges().frontp()) {
            work_ଠେଲ(edgep->fromp());
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::simplifyOne(GraphAcycVertex* avertexp) {
    // If a node has one input and one output, we can remove it and change the edges
    ଯଦି (avertexp->isDelete()) ଫେରନ୍ତୁ;
    ଯଦି (avertexp->inSize1() && avertexp->outSize1()) {
        V3GraphEdge* ଅଚଳ inEdgep = avertexp->inEdges().frontp();
        V3GraphEdge* ଅଚଳ outEdgep = avertexp->outEdges().frontp();
        V3GraphVertex* ଅଚଳ inVertexp = inEdgep->fromp();
        V3GraphVertex* ଅଚଳ outVertexp = outEdgep->ଶୀର୍ଷ();
        // The in and out may be the same node; we'll make a loop
        // The in OR out may be THIS node; we can't delete it then.
        ଯଦି (inVertexp != avertexp && outVertexp != avertexp) {
            UINFO(9, "  SimplifyOneRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            // Make a new edge connecting the two vertices directly
            // If both are breakable, we pick the one with less weight, else it's arbitrary
            // We can forget about the origEdge list for the "non-selected" set of edges,
            // as we need to break only one set or the other set of edges, not both.
            // (This is why we must give preference to the cutable set.)
            V3GraphEdge* ଅଚଳ templateEdgep
                = ((inEdgep->cutable()
                    && (!outEdgep->cutable() || inEdgep->weight() < outEdgep->weight()))
                       ? inEdgep
                       : outEdgep);
            // cppcheck-suppress leakReturnValNotUsed
            edgeFromEdge(templateEdgep, inVertexp, outVertexp);
            // Remove old edge
            VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            VL_DANGLING(templateEdgep);
            work_ଠେଲ(inVertexp);
            work_ଠେଲ(outVertexp);
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::simplifyOut(GraphAcycVertex* avertexp) {
    // If a node has one output that's not cutable, all its inputs can be reassigned
    // to the next node in the list
    ଯଦି (avertexp->isDelete()) ଫେରନ୍ତୁ;
    ଯଦି (avertexp->outSize1()) {
        V3GraphEdge* ଅଚଳ outEdgep = avertexp->outEdges().frontp();
        ଯଦି (!outEdgep->cutable()) {
            V3GraphVertex* outVertexp = outEdgep->ଶୀର୍ଷ();
            UINFO(9, "  SimplifyOutRemove " << avertexp);
            avertexp->setDelete();  // Mark so we won't delete it twice
            ପାଇଁ (V3GraphEdge* ଅଚଳ inEdgep : avertexp->inEdges().unlinkable()) {
                V3GraphVertex* inVertexp = inEdgep->fromp();
                ଯଦି (inVertexp == avertexp) {
                    ଯଦି (debug()) v3error("Non-cutable vertex=" << avertexp);  // LCOV_EXCL_LINE
                    v3error("Circular logic when ordering code (non-cutable edge loop)\n"
                            << m_origGraphp->reportLoops(  // calls OrderGraph::loopsVertexCb
                                   &V3GraphEdge::followNotCutable, avertexp->origVertexp()));
                    // Things are unlikely to end well at this point,
                    // but we'll try something to get to further errors...
                    inEdgep->cutable(ସତ୍ୟ);
                    ଫେରନ୍ତୁ;
                }
                // Make a new edge connecting the two vertices directly
                // cppcheck-suppress leakReturnValNotUsed
                edgeFromEdge(inEdgep, inVertexp, outVertexp);
                // Remove old edge
                VL_DO_DANGLING(inEdgep->unlinkDelete(), inEdgep);
                work_ଠେଲ(inVertexp);
            }
            VL_DO_DANGLING(outEdgep->unlinkDelete(), outEdgep);
            work_ଠେଲ(outVertexp);
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::simplifyDup(GraphAcycVertex* avertexp) {
    // Remove redundant edges
    ଯଦି (avertexp->isDelete()) ଫେରନ୍ତୁ;
    // Clear marks
    ପାଇଁ (V3GraphEdge& edge : avertexp->outEdges()) edge.ଶୀର୍ଷ()->userp(nullptr);
    // Mark edges and detect duplications
    ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : avertexp->outEdges().unlinkable()) {
        V3GraphVertex* outVertexp = edgep->ଶୀର୍ଷ();
        V3GraphEdge* prevEdgep = static_cast<V3GraphEdge*>(outVertexp->userp());
        ଯଦି (prevEdgep) {
            ଯଦି (!prevEdgep->cutable()) {
                // !cutable duplicates prev !cutable: we can ignore it, redundant
                //  cutable duplicates prev !cutable: know it's not a relevant loop, ignore it
                UINFO(8, "    DelDupEdge " << avertexp << " -> " << edgep->ଶୀର୍ଷ());
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            } ଅନ୍ୟଥା ଯଦି (!edgep->cutable()) {
                // !cutable duplicates prev  cutable: delete the earlier cutable
                UINFO(8, "    DelDupPrev " << avertexp << " -> " << prevEdgep->ଶୀର୍ଷ());
                VL_DO_DANGLING(prevEdgep->unlinkDelete(), prevEdgep);
                outVertexp->userp(edgep);
            } ଅନ୍ୟଥା {
                //  cutable duplicates prev  cutable: combine weights
                UINFO(8, "    DelDupComb " << avertexp << " -> " << edgep->ଶୀର୍ଷ());
                prevEdgep->weight(prevEdgep->weight() + edgep->weight());
                addOrigEdgep(prevEdgep, edgep);
                VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            }
            work_ଠେଲ(outVertexp);
            work_ଠେଲ(avertexp);
        } ଅନ୍ୟଥା {
            // No previous assignment
            outVertexp->userp(edgep);
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::cutBasic(GraphAcycVertex* avertexp) {
    // Detect and cleanup any loops from node to itself
    ଯଦି (avertexp->isDelete()) ଫେରନ୍ତୁ;
    ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : avertexp->outEdges().unlinkable()) {
        ଯଦି (edgep->cutable() && edgep->ଶୀର୍ଷ() == avertexp) {
            cutOrigEdge(edgep, "  Cut Basic");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ଠେଲ(avertexp);
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::cutBackward(GraphAcycVertex* avertexp) {
    // If a cutable edge is from A->B, and there's a non-cutable edge B->A, then must cut!
    ଯଦି (avertexp->isDelete()) ଫେରନ୍ତୁ;
    // Clear marks
    ପାଇଁ (V3GraphEdge& edge : avertexp->outEdges()) edge.ଶୀର୍ଷ()->user(ମିଥ୍ୟା);
    ପାଇଁ (V3GraphEdge& edge : avertexp->inEdges()) {
        ଯଦି (!edge.cutable()) edge.fromp()->user(ସତ୍ୟ);
    }
    // Detect duplications
    ପାଇଁ (V3GraphEdge* ଅଚଳ edgep : avertexp->outEdges().unlinkable()) {
        ଯଦି (edgep->cutable() && edgep->ଶୀର୍ଷ()->user()) {
            cutOrigEdge(edgep, "  Cut A->B->A");
            VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
            work_ଠେଲ(avertexp);
        }
    }
}

ଶୂନ୍ୟ GraphAcyc::place() {
    // Input is m_breakGraph with ranks already assigned on non-breakable edges

    // Make a list of all cutable edges in the graph
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_Edges = 0;
    ପାଇଁ (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        ପାଇଁ (V3GraphEdge& edge : vertex.outEdges()) {
            ଯଦି (edge.weight() && edge.cutable()) ++ଅଙ୍କ_Edges;
        }
    }
    UINFO(4, "    Cutable edges = " << ଅଙ୍କ_Edges);

    std::vector<V3GraphEdge*> edges;  // List of all edges to be processed
    // Make the vector properly sized right off the bat -- faster than reallocating
    edges.reserve(ଅଙ୍କ_Edges + 1);
    ପାଇଁ (V3GraphVertex& vertex : m_breakGraph.vertices()) {
        vertex.user(0);  // Clear in prep of next step
        ପାଇଁ (V3GraphEdge& edge : vertex.outEdges()) {
            ଯଦି (edge.weight() && edge.cutable()) edges.ଠେଲ_back(&edge);
        }
    }

    // Sort by weight, then by vertex (so that we completely process one vertex, when possible)
    stable_sort(edges.begin(), edges.end(), GraphAcycEdgeCmp());

    // Process each edge in weighted order
    m_placeStep = 10;
    ପାଇଁ (V3GraphEdge* edgep : edges) placeTryEdge(edgep);
}

ଶୂନ୍ୟ GraphAcyc::placeTryEdge(V3GraphEdge* edgep) {
    // Try to make this edge uncutable
    m_placeStep++;
    UINFO(8, "    PlaceEdge s" << m_placeStep << " w" << edgep->weight() << " " << edgep->fromp());
    // Make the edge uncutable so we detect it in placement
    edgep->cutable(ମିଥ୍ୟା);
    // Vertex::m_user begin: number indicates this edge was completed
    // Try to assign ranks, presuming this edge is in place
    // If we come across user()==placestep, we've detected a loop and must back out
    ଅଚଳ ତାର୍କିକ loop
        = placeIterate(static_cast<GraphAcycVertex*>(edgep->ଶୀର୍ଷ()), edgep->fromp()->rank() + 1);
    ଯଦି (!loop) {
        // No loop, we can keep it as uncutable
        // Commit the new ranks we calculated
        // Just cleanup the list.  If this is slow, we can add another set of
        // user counters to avoid cleaning up the list.
        ଯେତେବେଳେ (workBeginp()) work_କାଢ଼();
    } ଅନ୍ୟଥା {
        // Adding this edge would cause a loop, kill it
        edgep->cutable(ସତ୍ୟ);  // So graph still looks pretty
        cutOrigEdge(edgep, "  Cut loop");
        VL_DO_DANGLING(edgep->unlinkDelete(), edgep);
        // Back out the ranks we calculated
        ଯେତେବେଳେ (GraphAcycVertex* vertexp = workBeginp()) {
            work_କାଢ଼();
            vertexp->rank(vertexp->m_storedRank);
        }
    }
}

ତାର୍କିକ GraphAcyc::placeIterate(GraphAcycVertex* vertexp, uint32_t currentRank) {
    // Assign rank to each unvisited node
    //   rank() is the "committed rank" of the graph known without loops
    // If larger rank is found, assign it and loop back through
    // If we hit a back node make a list of all loops
    ଯଦି (vertexp->rank() >= currentRank) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Already processed it
    ଯଦି (vertexp->user() == m_placeStep) ଫେରନ୍ତୁ ସତ୍ୟ;  // Loop detected
    vertexp->user(m_placeStep);
    // Remember we're changing the rank of this node; might need to back out
    ଯଦି (!vertexp->m_onWorkList) {
        vertexp->m_storedRank = vertexp->rank();
        work_ଠେଲ(vertexp);
    }
    vertexp->rank(currentRank);
    // Follow all edges and increase their ranks
    ପାଇଁ (V3GraphEdge& edge : vertexp->outEdges()) {
        ଯଦି (edge.weight() && !edge.cutable()) {
            ଯଦି (placeIterate(static_cast<GraphAcycVertex*>(edge.ଶୀର୍ଷ()), currentRank + 1)) {
                // We don't need to reset user(); we'll use a different placeStep for the next edge
                ଫେରନ୍ତୁ ସତ୍ୟ;  // Loop detected
            }
        }
    }
    vertexp->user(0);
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

//----- Main algorithm entry point

ଶୂନ୍ୟ GraphAcyc::ମୁଖ୍ୟ() {
    m_breakGraph.userClearEdges();

    // Color based on possible loops
    m_origGraphp->stronglyConnected(m_origEdgeFuncp);

    // Make a new graph with vertices that have only a single vertex
    // for each group of old vertices that are interconnected with unbreakable
    // edges (and thus can't represent loops - if we did the unbreakable
    // marking right, anyways)
    buildGraph(m_origGraphp);
    ଯଦି (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_pre");

    // Perform simple optimizations before any cuttings
    simplify(ମିଥ୍ୟା);
    ଯଦି (dumpGraphLevel() >= 5) m_breakGraph.dumpDotFilePrefixed("acyc_simp");

    UINFO(4, " Cutting trivial loops");
    simplify(ସତ୍ୟ);
    ଯଦି (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_mid");

    UINFO(4, " Ranking");
    m_breakGraph.rank(&V3GraphEdge::followNotCutable);
    ଯଦି (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_rank");

    UINFO(4, " Placement");
    place();
    ଯଦି (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_place");

    UINFO(4, " Final Ranking");
    // Only needed to assert there are no loops in completed graph
    m_breakGraph.rank(&V3GraphEdge::followAlwaysTrue);
    ଯଦି (dumpGraphLevel() >= 6) m_breakGraph.dumpDotFilePrefixed("acyc_done");
}

ଶୂନ୍ୟ V3Graph::acyclic(V3EdgeFuncP edgeFuncp) {
    UINFO(4, "Acyclic");
    GraphAcyc acyc{ଏହି, edgeFuncp};
    acyc.ମୁଖ୍ୟ();
    UINFO(4, "Acyclic done");
}
