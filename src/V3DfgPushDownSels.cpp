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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Dfg.h"
#include "V3DfgPasses.h"
#include "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

class V3DfgPushDownSels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    struct State final {
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
    V3DfgPushDownSelsContext& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_stack;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    void debugCheck() {
        만약 (VL_LIKELY(!v3Global.opt.debugCheck())) 반환;
        m_dfg.forEachVertex([&](const DfgVertex& src) {
            const State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](const DfgVertex& dst) {
                const State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                반환 false;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    void initializeOrdering() {
        // Start from all vertices with no inputs
        m_stack.reserve(m_dfg.size());
        위해 (DfgVertexVar& vtx : m_dfg.varVertices()) {
            만약 (vtx.srcp() || vtx.defaultp()) continue;
            m_stack.push_back(&vtx);
        }
        위해 (DfgConst& vtx : m_dfg.constVertices()) m_stack.push_back(&vtx);
        위해 (DfgVertex& vtx : m_dfg.opVertices()) {
            만약 (!vtx.nInputs()) m_stack.push_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        동안 (!m_stack.empty()) {
            DfgVertex& vtx = *m_stack.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            만약 (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_stack.pop_back();
                continue;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            만약 (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_stack.pop_back();
                continue;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = true;
            vtx.foreachSink([&](DfgVertex& dst) {
                const State& dstState = m_stateMap[dst];
                만약 (dstState.visited) 반환 false;
                m_stack.push_back(&dst);
                반환 false;
            });
        }

        // Should reach exact zero
        UASSERT(!rpoNext, "All vertics should have been visited exactly once");

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
        const State& srcState = m_stateMap[src];
        const State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        만약 (dstState.ord > srcState.ord) 반환 true;
        // Pearce-Kelly dicovery step
        만약 (pkFwdDfs(src, dst)) 반환 false;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        반환 true;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    bool pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        const uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_stack.push_back(&dst);
        동안 (!m_stack.empty()) {
            DfgVertex& vtx = *m_stack.back();
            m_stack.pop_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            만약 (vtxState.visited) continue;

            // Save vertex, mark visited
            m_fwdVtxps.push_back(&vtx);
            vtxState.visited = true;

            // Enqueue unvisited sinks in affeced area
            const bool cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                const State& sinkState = m_stateMap[sink];
                만약 (sinkState.ord == srcOrd) 반환 true;  // Stop completely if cyclic
                만약 (sinkState.visited) 반환 false;  // Stop search if already visited
                만약 (sinkState.ord > srcOrd) 반환 false;  // Stop search if outside critical area
                m_stack.push_back(&sink);
                반환 false;
            });

            // If would be cyclic, reset state and return true
            만약 (cyclic) {
                위해 (DfgVertex* const vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = false;
                m_fwdVtxps.clear();
                m_stack.clear();
                반환 true;
            }
        }
        // Won't be cyclic, return false
        반환 false;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    void pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        const uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_stack.push_back(&src);
        동안 (!m_stack.empty()) {
            DfgVertex& vtx = *m_stack.back();
            m_stack.pop_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            만약 (vtxState.visited) continue;

            // Save vertex, mark visited
            m_bwdVtxps.push_back(&vtx);
            vtxState.visited = true;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                const State& sourceState = m_stateMap[source];
                만약 (sourceState.visited) 반환 false;  // Stop search if already visited
                만약 (sourceState.ord < dstOrd)
                    반환 false;  // Stop search if outside critical area
                m_stack.push_back(&source);
                반환 false;
            });
        }
    }

    // Pearce-Kelly reorder step
    void pkReorder() {
        // Sort vertices found during forward and backward search
        const auto cmp = [this](const DfgVertex* const ap, const DfgVertex* const bp) {
            반환 m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        const size_t bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.insert(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        위해 (DfgVertex* const vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = false;
            m_ords.push_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        위해 (size_t i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    static bool ignoredSink(const DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        만약 (const DfgVarPacked* const varp = sink.cast<DfgVarPacked>()) {
            만약 (!varp->hasSinks() && !varp->isObserved()) 반환 true;
        }
        반환 false;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    void findCandidatess() {
        위해 (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* const catp = vtx.cast<DfgConcat>();
            만약 (!catp) continue;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](const DfgVertex& sink) {
                만약 (sink.is<DfgSel>()) {
                    ++nSels;
                } 아니면 만약 (sink.is<DfgConcat>()) {
                    ++nCats;
                } 아니면 만약 (!ignoredSink(sink)) {
                    ++nOther;
                }
                반환 false;
            });

            // Consider if optimizable
            만약 (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.push_back(catp);
                m_stateMap[catp].onWorklist = true;
            }
        }
    }

    void pushDownSels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        동안 (!m_catps.empty()) {
            DfgConcat* const catp = m_catps.back();
            m_catps.pop_back();
            m_stateMap[catp].onWorklist = false;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            const bool multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                만약 (DfgSel* const selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    반환 false;
                }
                // Skip ignored sinks
                만약 (ignoredSink(sink)) 반환 false;
                // If already found a non-DfgSel sink, return true
                만약 (sinkp) 반환 true;
                // Save the non-DfgSel sink
                sinkp = &sink;
                반환 false;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            만약 (multipleNonSelSinks) continue;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            만약 (!sinkp) continue;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* const sinkCatp = sinkp->cast<DfgConcat>();
            만약 (!sinkCatp) continue;

            // Ok, we can try to push the selects down to the sink DfgConcat
            const uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            const uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            위해 (DfgSel* const selp : selps) {
                // Don't do it if it would create a cycle
                만약 (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    continue;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            만약 (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.push_back(sinkCatp);
                sinkCatState.onWorklist = true;
            }
        }
    }

    // CONSTRUCTOR
    V3DfgPushDownSels(DfgGraph& dfg, V3DfgPushDownSelsContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        만약 (m_catps.empty()) 반환;

        // Pre-allocate storage
        m_stack.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [this](const DfgConcat* const ap, const DfgConcat* const bp) {
                      반환 m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        pushDownSels();
    }

public:
    static void apply(DfgGraph& dfg, V3DfgPushDownSelsContext& ctx) {
        V3DfgPushDownSels{dfg, ctx};
    }
};

void V3DfgPasses::pushDownSels(DfgGraph& dfg, V3DfgPushDownSelsContext& ctx) {
    만약 (!v3Global.opt.fDfgPushDownSels()) 반환;
    V3DfgPushDownSels::apply(dfg, ctx);
}
