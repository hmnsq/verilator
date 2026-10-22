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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"
#अन्तर्भाव "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग V3_Dfg_ढकला_Down_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    संरचना State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        तार्किक visited = असत्य;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        तार्किक onWorklist = असत्य;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    DfgGraph& m_dfg;
    // Context for pass
    V3_Dfg_ढकला_Down_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_चळत;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    शून्य debugCheck() {
        जर (VL_LIKELY(!v3Global.opt.debugCheck())) परत;
        m_dfg.forEachVertex([&](स्थिरांक DfgVertex& src) {
            स्थिरांक State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](स्थिरांक DfgVertex& dst) {
                स्थिरांक State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                परत असत्य;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    शून्य initializeOrdering() {
        // Start from all vertices with no inputs
        m_चळत.reserve(m_dfg.size());
        साठी (DfgVertexVar& vtx : m_dfg.varVertices()) {
            जर (vtx.srcp() || vtx.defaultp()) सुरूठेवा;
            m_चळत.ढकला_back(&vtx);
        }
        साठी (DfgConst& vtx : m_dfg.constVertices()) m_चळत.ढकला_back(&vtx);
        साठी (DfgVertex& vtx : m_dfg.opVertices()) {
            जर (!vtx.nInputs()) m_चळत.ढकला_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        पर्यंत (!m_चळत.empty()) {
            DfgVertex& vtx = *m_चळत.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            जर (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_चळत.काढा_back();
                सुरूठेवा;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            जर (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_चळत.काढा_back();
                सुरूठेवा;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = सत्य;
            vtx.foreachSink([&](DfgVertex& dst) {
                स्थिरांक State& dstState = m_stateMap[dst];
                जर (dstState.visited) परत असत्य;
                m_चळत.ढकला_back(&dst);
                परत असत्य;
            });
        }

        // Should reach exact zero
        UASSERT(!rpoNext, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_stateMap[vtx].visited = असत्य; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    तार्किक addEdge(DfgVertex& src, DfgVertex& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        स्थिरांक State& srcState = m_stateMap[src];
        स्थिरांक State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        जर (dstState.ord > srcState.ord) परत सत्य;
        // Pearce-Kelly dicovery step
        जर (pkFwdDfs(src, dst)) परत असत्य;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        परत सत्य;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    तार्किक pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        स्थिरांक uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_चळत.ढकला_back(&dst);
        पर्यंत (!m_चळत.empty()) {
            DfgVertex& vtx = *m_चळत.back();
            m_चळत.काढा_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            जर (vtxState.visited) सुरूठेवा;

            // Save vertex, mark visited
            m_fwdVtxps.ढकला_back(&vtx);
            vtxState.visited = सत्य;

            // Enqueue unvisited sinks in affeced area
            स्थिरांक तार्किक cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                स्थिरांक State& sinkState = m_stateMap[sink];
                जर (sinkState.ord == srcOrd) परत सत्य;  // Stop completely if cyclic
                जर (sinkState.visited) परत असत्य;  // Stop search if already visited
                जर (sinkState.ord > srcOrd) परत असत्य;  // Stop search if outside critical area
                m_चळत.ढकला_back(&sink);
                परत असत्य;
            });

            // If would be cyclic, reset state and return true
            जर (cyclic) {
                साठी (DfgVertex* स्थिरांक vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = असत्य;
                m_fwdVtxps.clear();
                m_चळत.clear();
                परत सत्य;
            }
        }
        // Won't be cyclic, return false
        परत असत्य;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    शून्य pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        स्थिरांक uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_चळत.ढकला_back(&src);
        पर्यंत (!m_चळत.empty()) {
            DfgVertex& vtx = *m_चळत.back();
            m_चळत.काढा_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            जर (vtxState.visited) सुरूठेवा;

            // Save vertex, mark visited
            m_bwdVtxps.ढकला_back(&vtx);
            vtxState.visited = सत्य;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                स्थिरांक State& sourceState = m_stateMap[source];
                जर (sourceState.visited) परत असत्य;  // Stop search if already visited
                जर (sourceState.ord < dstOrd)
                    परत असत्य;  // Stop search if outside critical area
                m_चळत.ढकला_back(&source);
                परत असत्य;
            });
        }
    }

    // Pearce-Kelly reorder step
    शून्य pkReorder() {
        // Sort vertices found during forward and backward search
        स्थिरांक स्वतः cmp = [हे](स्थिरांक DfgVertex* स्थिरांक ap, स्थिरांक DfgVertex* स्थिरांक bp) {
            परत m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        स्थिरांक माप_प्रकार bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.घाला(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        साठी (DfgVertex* स्थिरांक vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = असत्य;
            m_ords.ढकला_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        साठी (माप_प्रकार i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    स्थिर तार्किक ignoredSink(स्थिरांक DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        जर (स्थिरांक DfgVarPacked* स्थिरांक varp = sink.cast<DfgVarPacked>()) {
            जर (!varp->hasSinks() && !varp->isObserved()) परत सत्य;
        }
        परत असत्य;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    शून्य findCandidatess() {
        साठी (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* स्थिरांक catp = vtx.cast<DfgConcat>();
            जर (!catp) सुरूठेवा;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](स्थिरांक DfgVertex& sink) {
                जर (sink.is<DfgSel>()) {
                    ++nSels;
                } अन्यथा जर (sink.is<DfgConcat>()) {
                    ++nCats;
                } अन्यथा जर (!ignoredSink(sink)) {
                    ++nOther;
                }
                परत असत्य;
            });

            // Consider if optimizable
            जर (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ढकला_back(catp);
                m_stateMap[catp].onWorklist = सत्य;
            }
        }
    }

    शून्य ढकला_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        पर्यंत (!m_catps.empty()) {
            DfgConcat* स्थिरांक catp = m_catps.back();
            m_catps.काढा_back();
            m_stateMap[catp].onWorklist = असत्य;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            स्थिरांक तार्किक multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                जर (DfgSel* स्थिरांक selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    परत असत्य;
                }
                // Skip ignored sinks
                जर (ignoredSink(sink)) परत असत्य;
                // If already found a non-DfgSel sink, return true
                जर (sinkp) परत सत्य;
                // Save the non-DfgSel sink
                sinkp = &sink;
                परत असत्य;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            जर (multipleNonSelSinks) सुरूठेवा;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            जर (!sinkp) सुरूठेवा;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* स्थिरांक sinkCatp = sinkp->cast<DfgConcat>();
            जर (!sinkCatp) सुरूठेवा;

            // Ok, we can try to push the selects down to the sink DfgConcat
            स्थिरांक uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            स्थिरांक uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            साठी (DfgSel* स्थिरांक selp : selps) {
                // Don't do it if it would create a cycle
                जर (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    सुरूठेवा;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            जर (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.ढकला_back(sinkCatp);
                sinkCatState.onWorklist = सत्य;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ढकला_Down_Sels(DfgGraph& dfg, V3_Dfg_ढकला_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        जर (m_catps.empty()) परत;

        // Pre-allocate storage
        m_चळत.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [हे](स्थिरांक DfgConcat* स्थिरांक ap, स्थिरांक DfgConcat* स्थिरांक bp) {
                      परत m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ढकला_Down_Sels();
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3_Dfg_ढकला_Down_Sels_Context& ctx) {
        V3_Dfg_ढकला_Down_Sels{dfg, ctx};
    }
};

शून्य V3DfgPasses::ढकला_Down_Sels(DfgGraph& dfg, V3_Dfg_ढकला_Down_Sels_Context& ctx) {
    जर (!v3Global.opt.f_Dfg_ढकला_Down_Sels()) परत;
    V3_Dfg_ढकला_Down_Sels::apply(dfg, ctx);
}
