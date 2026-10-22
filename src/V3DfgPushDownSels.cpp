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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ଶ୍ରେଣୀ V3_Dfg_ଠେଲ_Down_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    ସଂରଚନା State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        ତାର୍କିକ visited = ମିଥ୍ୟା;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        ତାର୍କିକ onWorklist = ମିଥ୍ୟା;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    DfgGraph& m_dfg;
    // Context for pass
    V3_Dfg_ଠେଲ_Down_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_ଗଦା;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    ଶୂନ୍ୟ debugCheck() {
        ଯଦି (VL_LIKELY(!v3Global.opt.debugCheck())) ଫେରନ୍ତୁ;
        m_dfg.forEachVertex([&](ଅଚଳ DfgVertex& src) {
            ଅଚଳ State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ଅଚଳ DfgVertex& dst) {
                ଅଚଳ State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    ଶୂନ୍ୟ initializeOrdering() {
        // Start from all vertices with no inputs
        m_ଗଦା.reserve(m_dfg.size());
        ପାଇଁ (DfgVertexVar& vtx : m_dfg.varVertices()) {
            ଯଦି (vtx.srcp() || vtx.defaultp()) ଜାରି;
            m_ଗଦା.ଠେଲ_back(&vtx);
        }
        ପାଇଁ (DfgConst& vtx : m_dfg.constVertices()) m_ଗଦା.ଠେଲ_back(&vtx);
        ପାଇଁ (DfgVertex& vtx : m_dfg.opVertices()) {
            ଯଦି (!vtx.nInputs()) m_ଗଦା.ଠେଲ_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        ଯେତେବେଳେ (!m_ଗଦା.empty()) {
            DfgVertex& vtx = *m_ଗଦା.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            ଯଦି (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_ଗଦା.କାଢ଼_back();
                ଜାରି;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            ଯଦି (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_ଗଦା.କାଢ଼_back();
                ଜାରି;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = ସତ୍ୟ;
            vtx.foreachSink([&](DfgVertex& dst) {
                ଅଚଳ State& dstState = m_stateMap[dst];
                ଯଦି (dstState.visited) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                m_ଗଦା.ଠେଲ_back(&dst);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
        }

        // Should reach exact zero
        UASSERT(!rpoNext, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_stateMap[vtx].visited = ମିଥ୍ୟା; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    ତାର୍କିକ addEdge(DfgVertex& src, DfgVertex& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ଅଚଳ State& srcState = m_stateMap[src];
        ଅଚଳ State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        ଯଦି (dstState.ord > srcState.ord) ଫେରନ୍ତୁ ସତ୍ୟ;
        // Pearce-Kelly dicovery step
        ଯଦି (pkFwdDfs(src, dst)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    ତାର୍କିକ pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        ଅଚଳ uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ଗଦା.ଠେଲ_back(&dst);
        ଯେତେବେଳେ (!m_ଗଦା.empty()) {
            DfgVertex& vtx = *m_ଗଦା.back();
            m_ଗଦା.କାଢ଼_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            ଯଦି (vtxState.visited) ଜାରି;

            // Save vertex, mark visited
            m_fwdVtxps.ଠେଲ_back(&vtx);
            vtxState.visited = ସତ୍ୟ;

            // Enqueue unvisited sinks in affeced area
            ଅଚଳ ତାର୍କିକ cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                ଅଚଳ State& sinkState = m_stateMap[sink];
                ଯଦି (sinkState.ord == srcOrd) ଫେରନ୍ତୁ ସତ୍ୟ;  // Stop completely if cyclic
                ଯଦି (sinkState.visited) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Stop search if already visited
                ଯଦି (sinkState.ord > srcOrd) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Stop search if outside critical area
                m_ଗଦା.ଠେଲ_back(&sink);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });

            // If would be cyclic, reset state and return true
            ଯଦି (cyclic) {
                ପାଇଁ (DfgVertex* ଅଚଳ vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = ମିଥ୍ୟା;
                m_fwdVtxps.clear();
                m_ଗଦା.clear();
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        // Won't be cyclic, return false
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    ଶୂନ୍ୟ pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        ଅଚଳ uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ଗଦା.ଠେଲ_back(&src);
        ଯେତେବେଳେ (!m_ଗଦା.empty()) {
            DfgVertex& vtx = *m_ଗଦା.back();
            m_ଗଦା.କାଢ଼_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            ଯଦି (vtxState.visited) ଜାରି;

            // Save vertex, mark visited
            m_bwdVtxps.ଠେଲ_back(&vtx);
            vtxState.visited = ସତ୍ୟ;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                ଅଚଳ State& sourceState = m_stateMap[source];
                ଯଦି (sourceState.visited) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Stop search if already visited
                ଯଦି (sourceState.ord < dstOrd)
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Stop search if outside critical area
                m_ଗଦା.ଠେଲ_back(&source);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
        }
    }

    // Pearce-Kelly reorder step
    ଶୂନ୍ୟ pkReorder() {
        // Sort vertices found during forward and backward search
        ଅଚଳ auto cmp = [ଏହି](ଅଚଳ DfgVertex* ଅଚଳ ap, ଅଚଳ DfgVertex* ଅଚଳ bp) {
            ଫେରନ୍ତୁ m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ଅଚଳ size_t bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.ଭର୍ତ୍ତି(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        ପାଇଁ (DfgVertex* ଅଚଳ vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = ମିଥ୍ୟା;
            m_ords.ଠେଲ_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        ପାଇଁ (size_t i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    ସ୍ଥିର ତାର୍କିକ ignoredSink(ଅଚଳ DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        ଯଦି (ଅଚଳ DfgVarPacked* ଅଚଳ varp = sink.cast<DfgVarPacked>()) {
            ଯଦି (!varp->hasSinks() && !varp->isObserved()) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    ଶୂନ୍ୟ findCandidatess() {
        ପାଇଁ (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ଅଚଳ catp = vtx.cast<DfgConcat>();
            ଯଦି (!catp) ଜାରି;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ଅଚଳ DfgVertex& sink) {
                ଯଦି (sink.is<DfgSel>()) {
                    ++nSels;
                } ଅନ୍ୟଥା ଯଦି (sink.is<DfgConcat>()) {
                    ++nCats;
                } ଅନ୍ୟଥା ଯଦି (!ignoredSink(sink)) {
                    ++nOther;
                }
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });

            // Consider if optimizable
            ଯଦି (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ଠେଲ_back(catp);
                m_stateMap[catp].onWorklist = ସତ୍ୟ;
            }
        }
    }

    ଶୂନ୍ୟ ଠେଲ_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        ଯେତେବେଳେ (!m_catps.empty()) {
            DfgConcat* ଅଚଳ catp = m_catps.back();
            m_catps.କାଢ଼_back();
            m_stateMap[catp].onWorklist = ମିଥ୍ୟା;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            ଅଚଳ ତାର୍କିକ multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                ଯଦି (DfgSel* ଅଚଳ selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                // Skip ignored sinks
                ଯଦି (ignoredSink(sink)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // If already found a non-DfgSel sink, return true
                ଯଦି (sinkp) ଫେରନ୍ତୁ ସତ୍ୟ;
                // Save the non-DfgSel sink
                sinkp = &sink;
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            ଯଦି (multipleNonSelSinks) ଜାରି;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            ଯଦି (!sinkp) ଜାରି;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ଅଚଳ sinkCatp = sinkp->cast<DfgConcat>();
            ଯଦି (!sinkCatp) ଜାରି;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ଅଚଳ uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            ଅଚଳ uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            ପାଇଁ (DfgSel* ଅଚଳ selp : selps) {
                // Don't do it if it would create a cycle
                ଯଦି (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    ଜାରି;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            ଯଦି (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.ଠେଲ_back(sinkCatp);
                sinkCatState.onWorklist = ସତ୍ୟ;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ଠେଲ_Down_Sels(DfgGraph& dfg, V3_Dfg_ଠେଲ_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        ଯଦି (m_catps.empty()) ଫେରନ୍ତୁ;

        // Pre-allocate storage
        m_ଗଦା.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [ଏହି](ଅଚଳ DfgConcat* ଅଚଳ ap, ଅଚଳ DfgConcat* ଅଚଳ bp) {
                      ଫେରନ୍ତୁ m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ଠେଲ_Down_Sels();
    }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(DfgGraph& dfg, V3_Dfg_ଠେଲ_Down_Sels_Context& ctx) {
        V3_Dfg_ଠେଲ_Down_Sels{dfg, ctx};
    }
};

ଶୂନ୍ୟ V3DfgPasses::ଠେଲ_Down_Sels(DfgGraph& dfg, V3_Dfg_ଠେଲ_Down_Sels_Context& ctx) {
    ଯଦି (!v3Global.opt.f_Dfg_ଠେଲ_Down_Sels()) ଫେରନ୍ତୁ;
    V3_Dfg_ଠେଲ_Down_Sels::apply(dfg, ctx);
}
