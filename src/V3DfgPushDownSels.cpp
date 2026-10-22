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

वर्ग V3_Dfg_धकेलो_Down_Sels final {
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
    शून्य debugCheck() {
        यदि (VL_LIKELY(!v3Global.opt.debugCheck())) प्रत्यागम;
        m_dfg.forEachVertex([&](ध्रुव DfgVertex& src) {
            ध्रुव State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ध्रुव DfgVertex& dst) {
                ध्रुव State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                प्रत्यागम असत्य;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    शून्य initializeOrdering() {
        // Start from all vertices with no inputs
        m_ढेर.reserve(m_dfg.size());
        प्रति (DfgVertexVar& vtx : m_dfg.varVertices()) {
            यदि (vtx.srcp() || vtx.defaultp()) अनुवर्त;
            m_ढेर.धकेलो_back(&vtx);
        }
        प्रति (DfgConst& vtx : m_dfg.constVertices()) m_ढेर.धकेलो_back(&vtx);
        प्रति (DfgVertex& vtx : m_dfg.opVertices()) {
            यदि (!vtx.nInputs()) m_ढेर.धकेलो_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        यावत् (!m_ढेर.empty()) {
            DfgVertex& vtx = *m_ढेर.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            यदि (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_ढेर.निकालो_back();
                अनुवर्त;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            यदि (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_ढेर.निकालो_back();
                अनुवर्त;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = सत्य;
            vtx.foreachSink([&](DfgVertex& dst) {
                ध्रुव State& dstState = m_stateMap[dst];
                यदि (dstState.visited) प्रत्यागम असत्य;
                m_ढेर.धकेलो_back(&dst);
                प्रत्यागम असत्य;
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
        ध्रुव State& srcState = m_stateMap[src];
        ध्रुव State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        यदि (dstState.ord > srcState.ord) प्रत्यागम सत्य;
        // Pearce-Kelly dicovery step
        यदि (pkFwdDfs(src, dst)) प्रत्यागम असत्य;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        प्रत्यागम सत्य;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    तार्किक pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        ध्रुव uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ढेर.धकेलो_back(&dst);
        यावत् (!m_ढेर.empty()) {
            DfgVertex& vtx = *m_ढेर.back();
            m_ढेर.निकालो_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            यदि (vtxState.visited) अनुवर्त;

            // Save vertex, mark visited
            m_fwdVtxps.धकेलो_back(&vtx);
            vtxState.visited = सत्य;

            // Enqueue unvisited sinks in affeced area
            ध्रुव तार्किक cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                ध्रुव State& sinkState = m_stateMap[sink];
                यदि (sinkState.ord == srcOrd) प्रत्यागम सत्य;  // Stop completely if cyclic
                यदि (sinkState.visited) प्रत्यागम असत्य;  // Stop search if already visited
                यदि (sinkState.ord > srcOrd) प्रत्यागम असत्य;  // Stop search if outside critical area
                m_ढेर.धकेलो_back(&sink);
                प्रत्यागम असत्य;
            });

            // If would be cyclic, reset state and return true
            यदि (cyclic) {
                प्रति (DfgVertex* ध्रुव vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = असत्य;
                m_fwdVtxps.clear();
                m_ढेर.clear();
                प्रत्यागम सत्य;
            }
        }
        // Won't be cyclic, return false
        प्रत्यागम असत्य;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    शून्य pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        ध्रुव uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ढेर.धकेलो_back(&src);
        यावत् (!m_ढेर.empty()) {
            DfgVertex& vtx = *m_ढेर.back();
            m_ढेर.निकालो_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            यदि (vtxState.visited) अनुवर्त;

            // Save vertex, mark visited
            m_bwdVtxps.धकेलो_back(&vtx);
            vtxState.visited = सत्य;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                ध्रुव State& sourceState = m_stateMap[source];
                यदि (sourceState.visited) प्रत्यागम असत्य;  // Stop search if already visited
                यदि (sourceState.ord < dstOrd)
                    प्रत्यागम असत्य;  // Stop search if outside critical area
                m_ढेर.धकेलो_back(&source);
                प्रत्यागम असत्य;
            });
        }
    }

    // Pearce-Kelly reorder step
    शून्य pkReorder() {
        // Sort vertices found during forward and backward search
        ध्रुव स्वतः cmp = [एतत्](ध्रुव DfgVertex* ध्रुव ap, ध्रुव DfgVertex* ध्रुव bp) {
            प्रत्यागम m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ध्रुव माप_प्रकार bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.जोड़ो(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        प्रति (DfgVertex* ध्रुव vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = असत्य;
            m_ords.धकेलो_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        प्रति (माप_प्रकार i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    स्थिर तार्किक ignoredSink(ध्रुव DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        यदि (ध्रुव DfgVarPacked* ध्रुव varp = sink.cast<DfgVarPacked>()) {
            यदि (!varp->hasSinks() && !varp->isObserved()) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    शून्य findCandidatess() {
        प्रति (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ध्रुव catp = vtx.cast<DfgConcat>();
            यदि (!catp) अनुवर्त;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ध्रुव DfgVertex& sink) {
                यदि (sink.is<DfgSel>()) {
                    ++nSels;
                } अन्यथा यदि (sink.is<DfgConcat>()) {
                    ++nCats;
                } अन्यथा यदि (!ignoredSink(sink)) {
                    ++nOther;
                }
                प्रत्यागम असत्य;
            });

            // Consider if optimizable
            यदि (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.धकेलो_back(catp);
                m_stateMap[catp].onWorklist = सत्य;
            }
        }
    }

    शून्य धकेलो_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        यावत् (!m_catps.empty()) {
            DfgConcat* ध्रुव catp = m_catps.back();
            m_catps.निकालो_back();
            m_stateMap[catp].onWorklist = असत्य;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            ध्रुव तार्किक multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                यदि (DfgSel* ध्रुव selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    प्रत्यागम असत्य;
                }
                // Skip ignored sinks
                यदि (ignoredSink(sink)) प्रत्यागम असत्य;
                // If already found a non-DfgSel sink, return true
                यदि (sinkp) प्रत्यागम सत्य;
                // Save the non-DfgSel sink
                sinkp = &sink;
                प्रत्यागम असत्य;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            यदि (multipleNonSelSinks) अनुवर्त;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            यदि (!sinkp) अनुवर्त;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ध्रुव sinkCatp = sinkp->cast<DfgConcat>();
            यदि (!sinkCatp) अनुवर्त;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ध्रुव uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            ध्रुव uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            प्रति (DfgSel* ध्रुव selp : selps) {
                // Don't do it if it would create a cycle
                यदि (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    अनुवर्त;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            यदि (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.धकेलो_back(sinkCatp);
                sinkCatState.onWorklist = सत्य;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_धकेलो_Down_Sels(DfgGraph& dfg, V3_Dfg_धकेलो_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        यदि (m_catps.empty()) प्रत्यागम;

        // Pre-allocate storage
        m_ढेर.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [एतत्](ध्रुव DfgConcat* ध्रुव ap, ध्रुव DfgConcat* ध्रुव bp) {
                      प्रत्यागम m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        धकेलो_Down_Sels();
    }

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3_Dfg_धकेलो_Down_Sels_Context& ctx) {
        V3_Dfg_धकेलो_Down_Sels{dfg, ctx};
    }
};

शून्य V3DfgPasses::धकेलो_Down_Sels(DfgGraph& dfg, V3_Dfg_धकेलो_Down_Sels_Context& ctx) {
    यदि (!v3Global.opt.f_Dfg_धकेलो_Down_Sels()) प्रत्यागम;
    V3_Dfg_धकेलो_Down_Sels::apply(dfg, ctx);
}
