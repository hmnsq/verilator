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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"
#അന്തര്ഭാവ "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

വർഗ്ഗം V3_Dfg_തള്ളുക_Down_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    ഘടന State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        തർക്കം visited = അസത്യം;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        തർക്കം onWorklist = അസത്യം;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    DfgGraph& m_dfg;
    // Context for pass
    V3_Dfg_തള്ളുക_Down_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_അട്ടി;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    ശൂന്യം debugCheck() {
        എങ്കിൽ (VL_LIKELY(!v3Global.opt.debugCheck())) തിരികെ;
        m_dfg.forEachVertex([&](സ്ഥിരം DfgVertex& src) {
            സ്ഥിരം State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](സ്ഥിരം DfgVertex& dst) {
                സ്ഥിരം State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                തിരികെ അസത്യം;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    ശൂന്യം initializeOrdering() {
        // Start from all vertices with no inputs
        m_അട്ടി.reserve(m_dfg.size());
        വേണ്ടി (DfgVertexVar& vtx : m_dfg.varVertices()) {
            എങ്കിൽ (vtx.srcp() || vtx.defaultp()) തുടരുക;
            m_അട്ടി.തള്ളുക_back(&vtx);
        }
        വേണ്ടി (DfgConst& vtx : m_dfg.constVertices()) m_അട്ടി.തള്ളുക_back(&vtx);
        വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) {
            എങ്കിൽ (!vtx.nInputs()) m_അട്ടി.തള്ളുക_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        ആകുമ്പോൾ (!m_അട്ടി.empty()) {
            DfgVertex& vtx = *m_അട്ടി.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            എങ്കിൽ (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_അട്ടി.എടുക്കുക_back();
                തുടരുക;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            എങ്കിൽ (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_അട്ടി.എടുക്കുക_back();
                തുടരുക;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = സത്യം;
            vtx.foreachSink([&](DfgVertex& dst) {
                സ്ഥിരം State& dstState = m_stateMap[dst];
                എങ്കിൽ (dstState.visited) തിരികെ അസത്യം;
                m_അട്ടി.തള്ളുക_back(&dst);
                തിരികെ അസത്യം;
            });
        }

        // Should reach exact zero
        UASSERT(!rpoNext, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_stateMap[vtx].visited = അസത്യം; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    തർക്കം addEdge(DfgVertex& src, DfgVertex& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        സ്ഥിരം State& srcState = m_stateMap[src];
        സ്ഥിരം State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        എങ്കിൽ (dstState.ord > srcState.ord) തിരികെ സത്യം;
        // Pearce-Kelly dicovery step
        എങ്കിൽ (pkFwdDfs(src, dst)) തിരികെ അസത്യം;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        തിരികെ സത്യം;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    തർക്കം pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        സ്ഥിരം uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_അട്ടി.തള്ളുക_back(&dst);
        ആകുമ്പോൾ (!m_അട്ടി.empty()) {
            DfgVertex& vtx = *m_അട്ടി.back();
            m_അട്ടി.എടുക്കുക_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            എങ്കിൽ (vtxState.visited) തുടരുക;

            // Save vertex, mark visited
            m_fwdVtxps.തള്ളുക_back(&vtx);
            vtxState.visited = സത്യം;

            // Enqueue unvisited sinks in affeced area
            സ്ഥിരം തർക്കം cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                സ്ഥിരം State& sinkState = m_stateMap[sink];
                എങ്കിൽ (sinkState.ord == srcOrd) തിരികെ സത്യം;  // Stop completely if cyclic
                എങ്കിൽ (sinkState.visited) തിരികെ അസത്യം;  // Stop search if already visited
                എങ്കിൽ (sinkState.ord > srcOrd) തിരികെ അസത്യം;  // Stop search if outside critical area
                m_അട്ടി.തള്ളുക_back(&sink);
                തിരികെ അസത്യം;
            });

            // If would be cyclic, reset state and return true
            എങ്കിൽ (cyclic) {
                വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = അസത്യം;
                m_fwdVtxps.clear();
                m_അട്ടി.clear();
                തിരികെ സത്യം;
            }
        }
        // Won't be cyclic, return false
        തിരികെ അസത്യം;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    ശൂന്യം pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        സ്ഥിരം uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_അട്ടി.തള്ളുക_back(&src);
        ആകുമ്പോൾ (!m_അട്ടി.empty()) {
            DfgVertex& vtx = *m_അട്ടി.back();
            m_അട്ടി.എടുക്കുക_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            എങ്കിൽ (vtxState.visited) തുടരുക;

            // Save vertex, mark visited
            m_bwdVtxps.തള്ളുക_back(&vtx);
            vtxState.visited = സത്യം;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                സ്ഥിരം State& sourceState = m_stateMap[source];
                എങ്കിൽ (sourceState.visited) തിരികെ അസത്യം;  // Stop search if already visited
                എങ്കിൽ (sourceState.ord < dstOrd)
                    തിരികെ അസത്യം;  // Stop search if outside critical area
                m_അട്ടി.തള്ളുക_back(&source);
                തിരികെ അസത്യം;
            });
        }
    }

    // Pearce-Kelly reorder step
    ശൂന്യം pkReorder() {
        // Sort vertices found during forward and backward search
        സ്ഥിരം auto cmp = [ഇത്](സ്ഥിരം DfgVertex* സ്ഥിരം ap, സ്ഥിരം DfgVertex* സ്ഥിരം bp) {
            തിരികെ m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        സ്ഥിരം size_t bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.തിരുകുക(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = അസത്യം;
            m_ords.തള്ളുക_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        വേണ്ടി (size_t i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    സ്ഥിതം തർക്കം ignoredSink(സ്ഥിരം DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        എങ്കിൽ (സ്ഥിരം DfgVarPacked* സ്ഥിരം varp = sink.cast<DfgVarPacked>()) {
            എങ്കിൽ (!varp->hasSinks() && !varp->isObserved()) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    ശൂന്യം findCandidatess() {
        വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* സ്ഥിരം catp = vtx.cast<DfgConcat>();
            എങ്കിൽ (!catp) തുടരുക;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](സ്ഥിരം DfgVertex& sink) {
                എങ്കിൽ (sink.is<DfgSel>()) {
                    ++nSels;
                } അല്ലെങ്കിൽ എങ്കിൽ (sink.is<DfgConcat>()) {
                    ++nCats;
                } അല്ലെങ്കിൽ എങ്കിൽ (!ignoredSink(sink)) {
                    ++nOther;
                }
                തിരികെ അസത്യം;
            });

            // Consider if optimizable
            എങ്കിൽ (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.തള്ളുക_back(catp);
                m_stateMap[catp].onWorklist = സത്യം;
            }
        }
    }

    ശൂന്യം തള്ളുക_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        ആകുമ്പോൾ (!m_catps.empty()) {
            DfgConcat* സ്ഥിരം catp = m_catps.back();
            m_catps.എടുക്കുക_back();
            m_stateMap[catp].onWorklist = അസത്യം;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            സ്ഥിരം തർക്കം multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                എങ്കിൽ (DfgSel* സ്ഥിരം selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    തിരികെ അസത്യം;
                }
                // Skip ignored sinks
                എങ്കിൽ (ignoredSink(sink)) തിരികെ അസത്യം;
                // If already found a non-DfgSel sink, return true
                എങ്കിൽ (sinkp) തിരികെ സത്യം;
                // Save the non-DfgSel sink
                sinkp = &sink;
                തിരികെ അസത്യം;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            എങ്കിൽ (multipleNonSelSinks) തുടരുക;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            എങ്കിൽ (!sinkp) തുടരുക;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* സ്ഥിരം sinkCatp = sinkp->cast<DfgConcat>();
            എങ്കിൽ (!sinkCatp) തുടരുക;

            // Ok, we can try to push the selects down to the sink DfgConcat
            സ്ഥിരം uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            സ്ഥിരം uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            വേണ്ടി (DfgSel* സ്ഥിരം selp : selps) {
                // Don't do it if it would create a cycle
                എങ്കിൽ (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    തുടരുക;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            എങ്കിൽ (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.തള്ളുക_back(sinkCatp);
                sinkCatState.onWorklist = സത്യം;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_തള്ളുക_Down_Sels(DfgGraph& dfg, V3_Dfg_തള്ളുക_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        എങ്കിൽ (m_catps.empty()) തിരികെ;

        // Pre-allocate storage
        m_അട്ടി.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [ഇത്](സ്ഥിരം DfgConcat* സ്ഥിരം ap, സ്ഥിരം DfgConcat* സ്ഥിരം bp) {
                      തിരികെ m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        തള്ളുക_Down_Sels();
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(DfgGraph& dfg, V3_Dfg_തള്ളുക_Down_Sels_Context& ctx) {
        V3_Dfg_തള്ളുക_Down_Sels{dfg, ctx};
    }
};

ശൂന്യം V3DfgPasses::തള്ളുക_Down_Sels(DfgGraph& dfg, V3_Dfg_തള്ളുക_Down_Sels_Context& ctx) {
    എങ്കിൽ (!v3Global.opt.f_Dfg_തള്ളുക_Down_Sels()) തിരികെ;
    V3_Dfg_തള്ളുക_Down_Sels::apply(dfg, ctx);
}
