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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ V3_Dfg_ತಳ್ಳು_Down_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    ಸಂರಚನಾ State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        ತಾರ್ಕಿಕ visited = ಅಸತ್ಯ;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        ತಾರ್ಕಿಕ onWorklist = ಅಸತ್ಯ;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    DfgGraph& m_dfg;
    // Context for pass
    V3_Dfg_ತಳ್ಳು_Down_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_ರಾಶಿ;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    ಶೂನ್ಯ debugCheck() {
        ಯದಿ (VL_LIKELY(!v3Global.opt.debugCheck())) ಪ್ರತ್ಯಾಗಮ;
        m_dfg.forEachVertex([&](ಧ್ರುವ DfgVertex& src) {
            ಧ್ರುವ State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ಧ್ರುವ DfgVertex& dst) {
                ಧ್ರುವ State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    ಶೂನ್ಯ initializeOrdering() {
        // Start from all vertices with no inputs
        m_ರಾಶಿ.reserve(m_dfg.size());
        ಪ್ರತಿ (DfgVertexVar& vtx : m_dfg.varVertices()) {
            ಯದಿ (vtx.srcp() || vtx.defaultp()) ಅನುವರ್ತ;
            m_ರಾಶಿ.ತಳ್ಳು_back(&vtx);
        }
        ಪ್ರತಿ (DfgConst& vtx : m_dfg.constVertices()) m_ರಾಶಿ.ತಳ್ಳು_back(&vtx);
        ಪ್ರತಿ (DfgVertex& vtx : m_dfg.opVertices()) {
            ಯದಿ (!vtx.nInputs()) m_ರಾಶಿ.ತಳ್ಳು_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        ಯಾವತ್ (!m_ರಾಶಿ.empty()) {
            DfgVertex& vtx = *m_ರಾಶಿ.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            ಯದಿ (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_ರಾಶಿ.ತೆಗೆ_back();
                ಅನುವರ್ತ;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            ಯದಿ (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_ರಾಶಿ.ತೆಗೆ_back();
                ಅನುವರ್ತ;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = ಸತ್ಯ;
            vtx.foreachSink([&](DfgVertex& dst) {
                ಧ್ರುವ State& dstState = m_stateMap[dst];
                ಯದಿ (dstState.visited) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                m_ರಾಶಿ.ತಳ್ಳು_back(&dst);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        }

        // Should reach exact zero
        UASSERT(!rpoNext, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_stateMap[vtx].visited = ಅಸತ್ಯ; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    ತಾರ್ಕಿಕ addEdge(DfgVertex& src, DfgVertex& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ಧ್ರುವ State& srcState = m_stateMap[src];
        ಧ್ರುವ State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        ಯದಿ (dstState.ord > srcState.ord) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Pearce-Kelly dicovery step
        ಯದಿ (pkFwdDfs(src, dst)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    ತಾರ್ಕಿಕ pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        ಧ್ರುವ uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ರಾಶಿ.ತಳ್ಳು_back(&dst);
        ಯಾವತ್ (!m_ರಾಶಿ.empty()) {
            DfgVertex& vtx = *m_ರಾಶಿ.back();
            m_ರಾಶಿ.ತೆಗೆ_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            ಯದಿ (vtxState.visited) ಅನುವರ್ತ;

            // Save vertex, mark visited
            m_fwdVtxps.ತಳ್ಳು_back(&vtx);
            vtxState.visited = ಸತ್ಯ;

            // Enqueue unvisited sinks in affeced area
            ಧ್ರುವ ತಾರ್ಕಿಕ cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                ಧ್ರುವ State& sinkState = m_stateMap[sink];
                ಯದಿ (sinkState.ord == srcOrd) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Stop completely if cyclic
                ಯದಿ (sinkState.visited) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Stop search if already visited
                ಯದಿ (sinkState.ord > srcOrd) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Stop search if outside critical area
                m_ರಾಶಿ.ತಳ್ಳು_back(&sink);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });

            // If would be cyclic, reset state and return true
            ಯದಿ (cyclic) {
                ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = ಅಸತ್ಯ;
                m_fwdVtxps.clear();
                m_ರಾಶಿ.clear();
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        // Won't be cyclic, return false
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    ಶೂನ್ಯ pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        ಧ್ರುವ uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ರಾಶಿ.ತಳ್ಳು_back(&src);
        ಯಾವತ್ (!m_ರಾಶಿ.empty()) {
            DfgVertex& vtx = *m_ರಾಶಿ.back();
            m_ರಾಶಿ.ತೆಗೆ_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            ಯದಿ (vtxState.visited) ಅನುವರ್ತ;

            // Save vertex, mark visited
            m_bwdVtxps.ತಳ್ಳು_back(&vtx);
            vtxState.visited = ಸತ್ಯ;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                ಧ್ರುವ State& sourceState = m_stateMap[source];
                ಯದಿ (sourceState.visited) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Stop search if already visited
                ಯದಿ (sourceState.ord < dstOrd)
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Stop search if outside critical area
                m_ರಾಶಿ.ತಳ್ಳು_back(&source);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        }
    }

    // Pearce-Kelly reorder step
    ಶೂನ್ಯ pkReorder() {
        // Sort vertices found during forward and backward search
        ಧ್ರುವ auto cmp = [ಏತತ್](ಧ್ರುವ DfgVertex* ಧ್ರುವ ap, ಧ್ರುವ DfgVertex* ಧ್ರುವ bp) {
            ಪ್ರತ್ಯಾಗಮ m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ಧ್ರುವ size_t bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.ಸೇರಿಸು(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = ಅಸತ್ಯ;
            m_ords.ತಳ್ಳು_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        ಪ್ರತಿ (size_t i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ignoredSink(ಧ್ರುವ DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        ಯದಿ (ಧ್ರುವ DfgVarPacked* ಧ್ರುವ varp = sink.cast<DfgVarPacked>()) {
            ಯದಿ (!varp->hasSinks() && !varp->isObserved()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    ಶೂನ್ಯ findCandidatess() {
        ಪ್ರತಿ (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ಧ್ರುವ catp = vtx.cast<DfgConcat>();
            ಯದಿ (!catp) ಅನುವರ್ತ;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ಧ್ರುವ DfgVertex& sink) {
                ಯದಿ (sink.is<DfgSel>()) {
                    ++nSels;
                } ಅನ್ಯಥಾ ಯದಿ (sink.is<DfgConcat>()) {
                    ++nCats;
                } ಅನ್ಯಥಾ ಯದಿ (!ignoredSink(sink)) {
                    ++nOther;
                }
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });

            // Consider if optimizable
            ಯದಿ (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ತಳ್ಳು_back(catp);
                m_stateMap[catp].onWorklist = ಸತ್ಯ;
            }
        }
    }

    ಶೂನ್ಯ ತಳ್ಳು_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        ಯಾವತ್ (!m_catps.empty()) {
            DfgConcat* ಧ್ರುವ catp = m_catps.back();
            m_catps.ತೆಗೆ_back();
            m_stateMap[catp].onWorklist = ಅಸತ್ಯ;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            ಧ್ರುವ ತಾರ್ಕಿಕ multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                ಯದಿ (DfgSel* ಧ್ರುವ selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                // Skip ignored sinks
                ಯದಿ (ignoredSink(sink)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // If already found a non-DfgSel sink, return true
                ಯದಿ (sinkp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                // Save the non-DfgSel sink
                sinkp = &sink;
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            ಯದಿ (multipleNonSelSinks) ಅನುವರ್ತ;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            ಯದಿ (!sinkp) ಅನುವರ್ತ;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ಧ್ರುವ sinkCatp = sinkp->cast<DfgConcat>();
            ಯದಿ (!sinkCatp) ಅನುವರ್ತ;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ಧ್ರುವ uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            ಧ್ರುವ uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            ಪ್ರತಿ (DfgSel* ಧ್ರುವ selp : selps) {
                // Don't do it if it would create a cycle
                ಯದಿ (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    ಅನುವರ್ತ;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            ಯದಿ (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.ತಳ್ಳು_back(sinkCatp);
                sinkCatState.onWorklist = ಸತ್ಯ;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ತಳ್ಳು_Down_Sels(DfgGraph& dfg, V3_Dfg_ತಳ್ಳು_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        ಯದಿ (m_catps.empty()) ಪ್ರತ್ಯಾಗಮ;

        // Pre-allocate storage
        m_ರಾಶಿ.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [ಏತತ್](ಧ್ರುವ DfgConcat* ಧ್ರುವ ap, ಧ್ರುವ DfgConcat* ಧ್ರುವ bp) {
                      ಪ್ರತ್ಯಾಗಮ m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ತಳ್ಳು_Down_Sels();
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(DfgGraph& dfg, V3_Dfg_ತಳ್ಳು_Down_Sels_Context& ctx) {
        V3_Dfg_ತಳ್ಳು_Down_Sels{dfg, ctx};
    }
};

ಶೂನ್ಯ V3DfgPasses::ತಳ್ಳು_Down_Sels(DfgGraph& dfg, V3_Dfg_ತಳ್ಳು_Down_Sels_Context& ctx) {
    ಯದಿ (!v3Global.opt.f_Dfg_ತಳ್ಳು_Down_Sels()) ಪ್ರತ್ಯಾಗಮ;
    V3_Dfg_ತಳ್ಳು_Down_Sels::apply(dfg, ctx);
}
