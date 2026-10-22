// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Push DfgSels through DfgConcat to avoid temporaries
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
// If a DfgConcat drives both a DfgSel and a DfgConcat, and would othersiwe
// not need a temporary, then push the DfgSel down to the lower DfgConcat.
// This avoids having to insert a temporary for many intermediate results.
//
// We need to be careful not to create a cycle by pushing down a DfgSel
// that in turn feeds the concat it is being redirected to. To handle this,
// we use the Pierce-Kelly algorithm to check if a cycle would be created by
// adding a new edge. See: "A Dynamic Topological Sort Algorithm for
// Directed Acyclic Graphs", David J. Pearce, Paul H.J. Kelly, 2007
//
//*************************************************************************

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"
#समावेश "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी V3_Dfg_धकेलो_Down_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    काष्ठा State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        bool visited = false;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        bool onWorklist = false;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    DfgGraph& m_dfg;
    // Context for pass
    V3_Dfg_धकेलो_Down_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_ढेर;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    व्योम debugCheck() {
        अगर (VL_LIKELY(!v3Global.opt.debugCheck())) वापस;
        m_dfg.forEachVertex([&](स्थिर DfgVertex& src) {
            स्थिर State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](स्थिर DfgVertex& dst) {
                स्थिर State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                वापस false;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    व्योम initializeOrdering() {
        // Start from all vertices with no inputs
        m_ढेर.reserve(m_dfg.आकार());
        क्रम (DfgVertexVar& vtx : m_dfg.varVertices()) {
            अगर (vtx.srcp() || vtx.defaultp()) जारी;
            m_ढेर.धकेलो_back(&vtx);
        }
        क्रम (DfgConst& vtx : m_dfg.constVertices()) m_ढेर.धकेलो_back(&vtx);
        क्रम (DfgVertex& vtx : m_dfg.opVertices()) {
            अगर (!vtx.nInputs()) m_ढेर.धकेलो_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpo_अग्र = m_dfg.आकार();

        // DFS loop
        जबतक (!m_ढेर.empty()) {
            DfgVertex& vtx = *m_ढेर.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            अगर (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_ढेर.निकालो_back();
                जारी;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            अगर (vtxState.visited) {
                vtxState.ord = rpo_अग्र--;
                m_ढेर.निकालो_back();
                जारी;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = true;
            vtx.foreachSink([&](DfgVertex& dst) {
                स्थिर State& dstState = m_stateMap[dst];
                अगर (dstState.visited) वापस false;
                m_ढेर.धकेलो_back(&dst);
                वापस false;
            });
        }

        // Should reach exact zero
        UASSERT(!rpo_अग्र, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_stateMap[vtx].visited = false; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    bool addEdge(DfgVertex& src, DfgVertex& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        स्थिर State& srcState = m_stateMap[src];
        स्थिर State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        अगर (dstState.ord > srcState.ord) वापस true;
        // Pearce-Kelly dicovery step
        अगर (pkFwdDfs(src, dst)) वापस false;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        वापस true;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    bool pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        स्थिर uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ढेर.धकेलो_back(&dst);
        जबतक (!m_ढेर.empty()) {
            DfgVertex& vtx = *m_ढेर.back();
            m_ढेर.निकालो_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            अगर (vtxState.visited) जारी;

            // Save vertex, mark visited
            m_fwdVtxps.धकेलो_back(&vtx);
            vtxState.visited = true;

            // Enqueue unvisited sinks in affeced area
            स्थिर bool cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                स्थिर State& sinkState = m_stateMap[sink];
                अगर (sinkState.ord == srcOrd) वापस true;  // Stop completely if cyclic
                अगर (sinkState.visited) वापस false;  // Stop search if already visited
                अगर (sinkState.ord > srcOrd) वापस false;  // Stop search if outside critical area
                m_ढेर.धकेलो_back(&sink);
                वापस false;
            });

            // If would be cyclic, reset state and return true
            अगर (cyclic) {
                क्रम (DfgVertex* स्थिर vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = false;
                m_fwdVtxps.सफा();
                m_ढेर.सफा();
                वापस true;
            }
        }
        // Won't be cyclic, return false
        वापस false;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    व्योम pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        स्थिर uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ढेर.धकेलो_back(&src);
        जबतक (!m_ढेर.empty()) {
            DfgVertex& vtx = *m_ढेर.back();
            m_ढेर.निकालो_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            अगर (vtxState.visited) जारी;

            // Save vertex, mark visited
            m_bwdVtxps.धकेलो_back(&vtx);
            vtxState.visited = true;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                स्थिर State& sourceState = m_stateMap[source];
                अगर (sourceState.visited) वापस false;  // Stop search if already visited
                अगर (sourceState.ord < dstOrd)
                    वापस false;  // Stop search if outside critical area
                m_ढेर.धकेलो_back(&source);
                वापस false;
            });
        }
    }

    // Pearce-Kelly reorder step
    व्योम pkReorder() {
        // Sort vertices found during forward and backward search
        स्थिर स्वतः cmp = [यह](स्थिर DfgVertex* स्थिर ap, स्थिर DfgVertex* स्थिर bp) {
            वापस m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        स्थिर माप_प्रकार bwd_आकार = m_bwdVtxps.आकार();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.जोड़ो(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        क्रम (DfgVertex* स्थिर vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = false;
            m_ords.धकेलो_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwd_आकार, m_ords.end());
        // Assign new ordering
        क्रम (माप_प्रकार i = 0; i < m_ords.आकार(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.सफा();
        m_bwdVtxps.सफा();
        m_ords.सफा();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    अटल bool ignoredSink(स्थिर DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        अगर (स्थिर DfgVarPacked* स्थिर varp = sink.cast<DfgVarPacked>()) {
            अगर (!varp->hasSinks() && !varp->isObserved()) वापस true;
        }
        वापस false;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    व्योम findCandidatess() {
        क्रम (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* स्थिर catp = vtx.cast<DfgConcat>();
            अगर (!catp) जारी;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](स्थिर DfgVertex& sink) {
                अगर (sink.is<DfgSel>()) {
                    ++nSels;
                } अन्यथा अगर (sink.is<DfgConcat>()) {
                    ++nCats;
                } अन्यथा अगर (!ignoredSink(sink)) {
                    ++nOther;
                }
                वापस false;
            });

            // Consider if optimizable
            अगर (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.धकेलो_back(catp);
                m_stateMap[catp].onWorklist = true;
            }
        }
    }

    व्योम धकेलो_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.आकार());
        // Consider each concatenation
        जबतक (!m_catps.empty()) {
            DfgConcat* स्थिर catp = m_catps.back();
            m_catps.निकालो_back();
            m_stateMap[catp].onWorklist = false;

            // Iterate sinks, collect selects, check if should be optimized
            selps.सफा();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            स्थिर bool multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                अगर (DfgSel* स्थिर selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    वापस false;
                }
                // Skip ignored sinks
                अगर (ignoredSink(sink)) वापस false;
                // If already found a non-DfgSel sink, return true
                अगर (sinkp) वापस true;
                // Save the non-DfgSel sink
                sinkp = &sink;
                वापस false;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            अगर (multipleNonSelSinks) जारी;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            अगर (!sinkp) जारी;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* स्थिर sinkCatp = sinkp->cast<DfgConcat>();
            अगर (!sinkCatp) जारी;

            // Ok, we can try to push the selects down to the sink DfgConcat
            स्थिर uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            स्थिर uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            क्रम (DfgSel* स्थिर selp : selps) {
                // Don't do it if it would create a cycle
                अगर (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    जारी;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            अगर (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.धकेलो_back(sinkCatp);
                sinkCatState.onWorklist = true;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_धकेलो_Down_Sels(DfgGraph& dfg, V3_Dfg_धकेलो_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.आकार());
        findCandidatess();
        // Early exit if nothing to do
        अगर (m_catps.empty()) वापस;

        // Pre-allocate storage
        m_ढेर.reserve(m_dfg.आकार());
        m_fwdVtxps.reserve(m_dfg.आकार());
        m_bwdVtxps.reserve(m_dfg.आकार());
        m_ords.reserve(m_dfg.आकार());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [यह](स्थिर DfgConcat* स्थिर ap, स्थिर DfgConcat* स्थिर bp) {
                      वापस m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        धकेलो_Down_Sels();
    }

खुला:
    अटल व्योम apply(DfgGraph& dfg, V3_Dfg_धकेलो_Down_Sels_Context& ctx) {
        V3_Dfg_धकेलो_Down_Sels{dfg, ctx};
    }
};

व्योम V3DfgPasses::धकेलो_Down_Sels(DfgGraph& dfg, V3_Dfg_धकेलो_Down_Sels_Context& ctx) {
    अगर (!v3Global.opt.f_Dfg_धकेलो_Down_Sels()) वापस;
    V3_Dfg_धकेलो_Down_Sels::apply(dfg, ctx);
}
