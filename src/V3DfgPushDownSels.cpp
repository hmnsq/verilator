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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"
#அந்தர்பாவ "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

வகுப்பு V3_Dfg_தள்ளு_Down_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    கட்டமைப்பு State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        தருக்க visited = பொய்;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        தருக்க onWorklist = பொய்;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    DfgGraph& m_dfg;
    // Context for pass
    V3_Dfg_தள்ளு_Down_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_அடுக்கு;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    வெறுமை debugCheck() {
        எனில் (VL_LIKELY(!v3Global.opt.debugCheck())) திரும்பு;
        m_dfg.forEachVertex([&](மாறிலி DfgVertex& src) {
            மாறிலி State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](மாறிலி DfgVertex& dst) {
                மாறிலி State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                திரும்பு பொய்;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    வெறுமை initializeOrdering() {
        // Start from all vertices with no inputs
        m_அடுக்கு.reserve(m_dfg.size());
        க்கு (DfgVertexVar& vtx : m_dfg.varVertices()) {
            எனில் (vtx.srcp() || vtx.defaultp()) தொடர்;
            m_அடுக்கு.தள்ளு_back(&vtx);
        }
        க்கு (DfgConst& vtx : m_dfg.constVertices()) m_அடுக்கு.தள்ளு_back(&vtx);
        க்கு (DfgVertex& vtx : m_dfg.opVertices()) {
            எனில் (!vtx.nInputs()) m_அடுக்கு.தள்ளு_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        போது (!m_அடுக்கு.empty()) {
            DfgVertex& vtx = *m_அடுக்கு.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            எனில் (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_அடுக்கு.எடு_back();
                தொடர்;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            எனில் (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_அடுக்கு.எடு_back();
                தொடர்;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = உண்மை;
            vtx.foreachSink([&](DfgVertex& dst) {
                மாறிலி State& dstState = m_stateMap[dst];
                எனில் (dstState.visited) திரும்பு பொய்;
                m_அடுக்கு.தள்ளு_back(&dst);
                திரும்பு பொய்;
            });
        }

        // Should reach exact zero
        UASSERT(!rpoNext, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_stateMap[vtx].visited = பொய்; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    தருக்க addEdge(DfgVertex& src, DfgVertex& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        மாறிலி State& srcState = m_stateMap[src];
        மாறிலி State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        எனில் (dstState.ord > srcState.ord) திரும்பு உண்மை;
        // Pearce-Kelly dicovery step
        எனில் (pkFwdDfs(src, dst)) திரும்பு பொய்;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        திரும்பு உண்மை;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    தருக்க pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        மாறிலி uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_அடுக்கு.தள்ளு_back(&dst);
        போது (!m_அடுக்கு.empty()) {
            DfgVertex& vtx = *m_அடுக்கு.back();
            m_அடுக்கு.எடு_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            எனில் (vtxState.visited) தொடர்;

            // Save vertex, mark visited
            m_fwdVtxps.தள்ளு_back(&vtx);
            vtxState.visited = உண்மை;

            // Enqueue unvisited sinks in affeced area
            மாறிலி தருக்க cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                மாறிலி State& sinkState = m_stateMap[sink];
                எனில் (sinkState.ord == srcOrd) திரும்பு உண்மை;  // Stop completely if cyclic
                எனில் (sinkState.visited) திரும்பு பொய்;  // Stop search if already visited
                எனில் (sinkState.ord > srcOrd) திரும்பு பொய்;  // Stop search if outside critical area
                m_அடுக்கு.தள்ளு_back(&sink);
                திரும்பு பொய்;
            });

            // If would be cyclic, reset state and return true
            எனில் (cyclic) {
                க்கு (DfgVertex* மாறிலி vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = பொய்;
                m_fwdVtxps.clear();
                m_அடுக்கு.clear();
                திரும்பு உண்மை;
            }
        }
        // Won't be cyclic, return false
        திரும்பு பொய்;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    வெறுமை pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        மாறிலி uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_அடுக்கு.தள்ளு_back(&src);
        போது (!m_அடுக்கு.empty()) {
            DfgVertex& vtx = *m_அடுக்கு.back();
            m_அடுக்கு.எடு_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            எனில் (vtxState.visited) தொடர்;

            // Save vertex, mark visited
            m_bwdVtxps.தள்ளு_back(&vtx);
            vtxState.visited = உண்மை;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                மாறிலி State& sourceState = m_stateMap[source];
                எனில் (sourceState.visited) திரும்பு பொய்;  // Stop search if already visited
                எனில் (sourceState.ord < dstOrd)
                    திரும்பு பொய்;  // Stop search if outside critical area
                m_அடுக்கு.தள்ளு_back(&source);
                திரும்பு பொய்;
            });
        }
    }

    // Pearce-Kelly reorder step
    வெறுமை pkReorder() {
        // Sort vertices found during forward and backward search
        மாறிலி auto cmp = [இது](மாறிலி DfgVertex* மாறிலி ap, மாறிலி DfgVertex* மாறிலி bp) {
            திரும்பு m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        மாறிலி size_t bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.செருகு(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        க்கு (DfgVertex* மாறிலி vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = பொய்;
            m_ords.தள்ளு_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        க்கு (size_t i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    நிலையான தருக்க ignoredSink(மாறிலி DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        எனில் (மாறிலி DfgVarPacked* மாறிலி varp = sink.cast<DfgVarPacked>()) {
            எனில் (!varp->hasSinks() && !varp->isObserved()) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    வெறுமை findCandidatess() {
        க்கு (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* மாறிலி catp = vtx.cast<DfgConcat>();
            எனில் (!catp) தொடர்;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](மாறிலி DfgVertex& sink) {
                எனில் (sink.is<DfgSel>()) {
                    ++nSels;
                } இல்லையெனில் எனில் (sink.is<DfgConcat>()) {
                    ++nCats;
                } இல்லையெனில் எனில் (!ignoredSink(sink)) {
                    ++nOther;
                }
                திரும்பு பொய்;
            });

            // Consider if optimizable
            எனில் (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.தள்ளு_back(catp);
                m_stateMap[catp].onWorklist = உண்மை;
            }
        }
    }

    வெறுமை தள்ளு_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        போது (!m_catps.empty()) {
            DfgConcat* மாறிலி catp = m_catps.back();
            m_catps.எடு_back();
            m_stateMap[catp].onWorklist = பொய்;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            மாறிலி தருக்க multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                எனில் (DfgSel* மாறிலி selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    திரும்பு பொய்;
                }
                // Skip ignored sinks
                எனில் (ignoredSink(sink)) திரும்பு பொய்;
                // If already found a non-DfgSel sink, return true
                எனில் (sinkp) திரும்பு உண்மை;
                // Save the non-DfgSel sink
                sinkp = &sink;
                திரும்பு பொய்;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            எனில் (multipleNonSelSinks) தொடர்;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            எனில் (!sinkp) தொடர்;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* மாறிலி sinkCatp = sinkp->cast<DfgConcat>();
            எனில் (!sinkCatp) தொடர்;

            // Ok, we can try to push the selects down to the sink DfgConcat
            மாறிலி uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            மாறிலி uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            க்கு (DfgSel* மாறிலி selp : selps) {
                // Don't do it if it would create a cycle
                எனில் (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    தொடர்;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            எனில் (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.தள்ளு_back(sinkCatp);
                sinkCatState.onWorklist = உண்மை;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_தள்ளு_Down_Sels(DfgGraph& dfg, V3_Dfg_தள்ளு_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        எனில் (m_catps.empty()) திரும்பு;

        // Pre-allocate storage
        m_அடுக்கு.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [இது](மாறிலி DfgConcat* மாறிலி ap, மாறிலி DfgConcat* மாறிலி bp) {
                      திரும்பு m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        தள்ளு_Down_Sels();
    }

பொது:
    நிலையான வெறுமை apply(DfgGraph& dfg, V3_Dfg_தள்ளு_Down_Sels_Context& ctx) {
        V3_Dfg_தள்ளு_Down_Sels{dfg, ctx};
    }
};

வெறுமை V3DfgPasses::தள்ளு_Down_Sels(DfgGraph& dfg, V3_Dfg_தள்ளு_Down_Sels_Context& ctx) {
    எனில் (!v3Global.opt.f_Dfg_தள்ளு_Down_Sels()) திரும்பு;
    V3_Dfg_தள்ளு_Down_Sels::apply(dfg, ctx);
}
