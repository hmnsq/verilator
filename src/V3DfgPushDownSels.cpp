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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"
#અન્તર્ભાવ "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

વર્ગ V3_Dfg_ધકેલો_Down_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    સંરચના State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        તાર્કિક visited = ખોટું;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        તાર્કિક onWorklist = ખોટું;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    DfgGraph& m_dfg;
    // Context for pass
    V3_Dfg_ધકેલો_Down_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_ઢગલો;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    શૂન્ય debugCheck() {
        જો (VL_LIKELY(!v3Global.opt.debugCheck())) પાછા;
        m_dfg.forEachVertex([&](અચલ DfgVertex& src) {
            અચલ State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](અચલ DfgVertex& dst) {
                અચલ State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                પાછા ખોટું;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    શૂન્ય initializeOrdering() {
        // Start from all vertices with no inputs
        m_ઢગલો.reserve(m_dfg.size());
        માટે (DfgVertexVar& vtx : m_dfg.varVertices()) {
            જો (vtx.srcp() || vtx.defaultp()) ચાલુ;
            m_ઢગલો.ધકેલો_back(&vtx);
        }
        માટે (DfgConst& vtx : m_dfg.constVertices()) m_ઢગલો.ધકેલો_back(&vtx);
        માટે (DfgVertex& vtx : m_dfg.opVertices()) {
            જો (!vtx.nInputs()) m_ઢગલો.ધકેલો_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        જ્યારે (!m_ઢગલો.empty()) {
            DfgVertex& vtx = *m_ઢગલો.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            જો (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_ઢગલો.કાઢો_back();
                ચાલુ;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            જો (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_ઢગલો.કાઢો_back();
                ચાલુ;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = સાચું;
            vtx.foreachSink([&](DfgVertex& dst) {
                અચલ State& dstState = m_stateMap[dst];
                જો (dstState.visited) પાછા ખોટું;
                m_ઢગલો.ધકેલો_back(&dst);
                પાછા ખોટું;
            });
        }

        // Should reach exact zero
        UASSERT(!rpoNext, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_stateMap[vtx].visited = ખોટું; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    તાર્કિક addEdge(DfgVertex& src, DfgVertex& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        અચલ State& srcState = m_stateMap[src];
        અચલ State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        જો (dstState.ord > srcState.ord) પાછા સાચું;
        // Pearce-Kelly dicovery step
        જો (pkFwdDfs(src, dst)) પાછા ખોટું;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        પાછા સાચું;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    તાર્કિક pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        અચલ uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ઢગલો.ધકેલો_back(&dst);
        જ્યારે (!m_ઢગલો.empty()) {
            DfgVertex& vtx = *m_ઢગલો.back();
            m_ઢગલો.કાઢો_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            જો (vtxState.visited) ચાલુ;

            // Save vertex, mark visited
            m_fwdVtxps.ધકેલો_back(&vtx);
            vtxState.visited = સાચું;

            // Enqueue unvisited sinks in affeced area
            અચલ તાર્કિક cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                અચલ State& sinkState = m_stateMap[sink];
                જો (sinkState.ord == srcOrd) પાછા સાચું;  // Stop completely if cyclic
                જો (sinkState.visited) પાછા ખોટું;  // Stop search if already visited
                જો (sinkState.ord > srcOrd) પાછા ખોટું;  // Stop search if outside critical area
                m_ઢગલો.ધકેલો_back(&sink);
                પાછા ખોટું;
            });

            // If would be cyclic, reset state and return true
            જો (cyclic) {
                માટે (DfgVertex* અચલ vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = ખોટું;
                m_fwdVtxps.clear();
                m_ઢગલો.clear();
                પાછા સાચું;
            }
        }
        // Won't be cyclic, return false
        પાછા ખોટું;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    શૂન્ય pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        અચલ uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ઢગલો.ધકેલો_back(&src);
        જ્યારે (!m_ઢગલો.empty()) {
            DfgVertex& vtx = *m_ઢગલો.back();
            m_ઢગલો.કાઢો_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            જો (vtxState.visited) ચાલુ;

            // Save vertex, mark visited
            m_bwdVtxps.ધકેલો_back(&vtx);
            vtxState.visited = સાચું;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                અચલ State& sourceState = m_stateMap[source];
                જો (sourceState.visited) પાછા ખોટું;  // Stop search if already visited
                જો (sourceState.ord < dstOrd)
                    પાછા ખોટું;  // Stop search if outside critical area
                m_ઢગલો.ધકેલો_back(&source);
                પાછા ખોટું;
            });
        }
    }

    // Pearce-Kelly reorder step
    શૂન્ય pkReorder() {
        // Sort vertices found during forward and backward search
        અચલ auto cmp = [આ](અચલ DfgVertex* અચલ ap, અચલ DfgVertex* અચલ bp) {
            પાછા m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        અચલ size_t bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.ઉમેરો(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        માટે (DfgVertex* અચલ vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = ખોટું;
            m_ords.ધકેલો_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        માટે (size_t i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    સ્થિર તાર્કિક ignoredSink(અચલ DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        જો (અચલ DfgVarPacked* અચલ varp = sink.cast<DfgVarPacked>()) {
            જો (!varp->hasSinks() && !varp->isObserved()) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    શૂન્ય findCandidatess() {
        માટે (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* અચલ catp = vtx.cast<DfgConcat>();
            જો (!catp) ચાલુ;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](અચલ DfgVertex& sink) {
                જો (sink.is<DfgSel>()) {
                    ++nSels;
                } નહીંતર જો (sink.is<DfgConcat>()) {
                    ++nCats;
                } નહીંતર જો (!ignoredSink(sink)) {
                    ++nOther;
                }
                પાછા ખોટું;
            });

            // Consider if optimizable
            જો (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ધકેલો_back(catp);
                m_stateMap[catp].onWorklist = સાચું;
            }
        }
    }

    શૂન્ય ધકેલો_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        જ્યારે (!m_catps.empty()) {
            DfgConcat* અચલ catp = m_catps.back();
            m_catps.કાઢો_back();
            m_stateMap[catp].onWorklist = ખોટું;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            અચલ તાર્કિક multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                જો (DfgSel* અચલ selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    પાછા ખોટું;
                }
                // Skip ignored sinks
                જો (ignoredSink(sink)) પાછા ખોટું;
                // If already found a non-DfgSel sink, return true
                જો (sinkp) પાછા સાચું;
                // Save the non-DfgSel sink
                sinkp = &sink;
                પાછા ખોટું;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            જો (multipleNonSelSinks) ચાલુ;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            જો (!sinkp) ચાલુ;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* અચલ sinkCatp = sinkp->cast<DfgConcat>();
            જો (!sinkCatp) ચાલુ;

            // Ok, we can try to push the selects down to the sink DfgConcat
            અચલ uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            અચલ uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            માટે (DfgSel* અચલ selp : selps) {
                // Don't do it if it would create a cycle
                જો (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    ચાલુ;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            જો (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.ધકેલો_back(sinkCatp);
                sinkCatState.onWorklist = સાચું;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ધકેલો_Down_Sels(DfgGraph& dfg, V3_Dfg_ધકેલો_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        જો (m_catps.empty()) પાછા;

        // Pre-allocate storage
        m_ઢગલો.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [આ](અચલ DfgConcat* અચલ ap, અચલ DfgConcat* અચલ bp) {
                      પાછા m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ધકેલો_Down_Sels();
    }

જાહેર:
    સ્થિર શૂન્ય apply(DfgGraph& dfg, V3_Dfg_ધકેલો_Down_Sels_Context& ctx) {
        V3_Dfg_ધકેલો_Down_Sels{dfg, ctx};
    }
};

શૂન્ય V3DfgPasses::ધકેલો_Down_Sels(DfgGraph& dfg, V3_Dfg_ધકેલો_Down_Sels_Context& ctx) {
    જો (!v3Global.opt.f_Dfg_ધકેલો_Down_Sels()) પાછા;
    V3_Dfg_ધકેલો_Down_Sels::apply(dfg, ctx);
}
