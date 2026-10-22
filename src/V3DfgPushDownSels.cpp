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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"
#অন্তর্ভাব "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ V3_Dfg_ঠেলো_Down_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    সংরচনা State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        তার্কিক visited = অসত্য;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        তার্কিক onWorklist = অসত্য;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    DfgGraph& m_dfg;
    // Context for pass
    V3_Dfg_ঠেলো_Down_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::vector<DfgVertex*> m_স্তূপ;  // DFS stack for various steps
    std::vector<DfgVertex*> m_fwdVtxps;  // Vertices found during forward DFS
    std::vector<DfgVertex*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::vector<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::vector<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    শূন্য debugCheck() {
        যদি (VL_LIKELY(!v3Global.opt.debugCheck())) প্রত্যাগম;
        m_dfg.forEachVertex([&](ধ্রুব DfgVertex& src) {
            ধ্রুব State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.visited, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ধ্রুব DfgVertex& dst) {
                ধ্রুব State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                প্রত্যাগম অসত্য;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    শূন্য initializeOrdering() {
        // Start from all vertices with no inputs
        m_স্তূপ.reserve(m_dfg.size());
        প্রতি (DfgVertexVar& vtx : m_dfg.varVertices()) {
            যদি (vtx.srcp() || vtx.defaultp()) অনুবর্ত;
            m_স্তূপ.ঠেলো_back(&vtx);
        }
        প্রতি (DfgConst& vtx : m_dfg.constVertices()) m_স্তূপ.ঠেলো_back(&vtx);
        প্রতি (DfgVertex& vtx : m_dfg.opVertices()) {
            যদি (!vtx.nInputs()) m_স্তূপ.ঠেলো_back(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpoNext = m_dfg.size();

        // DFS loop
        যাবত্ (!m_স্তূপ.empty()) {
            DfgVertex& vtx = *m_স্তূপ.back();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            যদি (vtxState.ord) {
                UASSERT_OBJ(vtxState.visited, &vtx, "Not visited, but ordering assigned");
                m_স্তূপ.বের_করো_back();
                অনুবর্ত;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            যদি (vtxState.visited) {
                vtxState.ord = rpoNext--;
                m_স্তূপ.বের_করো_back();
                অনুবর্ত;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.visited = সত্য;
            vtx.foreachSink([&](DfgVertex& dst) {
                ধ্রুব State& dstState = m_stateMap[dst];
                যদি (dstState.visited) প্রত্যাগম অসত্য;
                m_স্তূপ.ঠেলো_back(&dst);
                প্রত্যাগম অসত্য;
            });
        }

        // Should reach exact zero
        UASSERT(!rpoNext, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_stateMap[vtx].visited = অসত্য; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    তার্কিক addEdge(DfgVertex& src, DfgVertex& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ধ্রুব State& srcState = m_stateMap[src];
        ধ্রুব State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        যদি (dstState.ord > srcState.ord) প্রত্যাগম সত্য;
        // Pearce-Kelly dicovery step
        যদি (pkFwdDfs(src, dst)) প্রত্যাগম অসত্য;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        প্রত্যাগম সত্য;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    তার্কিক pkFwdDfs(DfgVertex& src, DfgVertex& dst) {
        ধ্রুব uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_স্তূপ.ঠেলো_back(&dst);
        যাবত্ (!m_স্তূপ.empty()) {
            DfgVertex& vtx = *m_স্তূপ.back();
            m_স্তূপ.বের_করো_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            যদি (vtxState.visited) অনুবর্ত;

            // Save vertex, mark visited
            m_fwdVtxps.ঠেলো_back(&vtx);
            vtxState.visited = সত্য;

            // Enqueue unvisited sinks in affeced area
            ধ্রুব তার্কিক cyclic = vtx.foreachSink([&](DfgVertex& sink) {
                ধ্রুব State& sinkState = m_stateMap[sink];
                যদি (sinkState.ord == srcOrd) প্রত্যাগম সত্য;  // Stop completely if cyclic
                যদি (sinkState.visited) প্রত্যাগম অসত্য;  // Stop search if already visited
                যদি (sinkState.ord > srcOrd) প্রত্যাগম অসত্য;  // Stop search if outside critical area
                m_স্তূপ.ঠেলো_back(&sink);
                প্রত্যাগম অসত্য;
            });

            // If would be cyclic, reset state and return true
            যদি (cyclic) {
                প্রতি (DfgVertex* ধ্রুব vtxp : m_fwdVtxps) m_stateMap[vtxp].visited = অসত্য;
                m_fwdVtxps.clear();
                m_স্তূপ.clear();
                প্রত্যাগম সত্য;
            }
        }
        // Won't be cyclic, return false
        প্রত্যাগম অসত্য;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    শূন্য pkBwdDfs(DfgVertex& src, DfgVertex& dst) {
        ধ্রুব uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_স্তূপ.ঠেলো_back(&src);
        যাবত্ (!m_স্তূপ.empty()) {
            DfgVertex& vtx = *m_স্তূপ.back();
            m_স্তূপ.বের_করো_back();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            যদি (vtxState.visited) অনুবর্ত;

            // Save vertex, mark visited
            m_bwdVtxps.ঠেলো_back(&vtx);
            vtxState.visited = সত্য;

            // Enqueue unvisited sources in affeced area
            vtx.foreachSource([&](DfgVertex& source) {
                ধ্রুব State& sourceState = m_stateMap[source];
                যদি (sourceState.visited) প্রত্যাগম অসত্য;  // Stop search if already visited
                যদি (sourceState.ord < dstOrd)
                    প্রত্যাগম অসত্য;  // Stop search if outside critical area
                m_স্তূপ.ঠেলো_back(&source);
                প্রত্যাগম অসত্য;
            });
        }
    }

    // Pearce-Kelly reorder step
    শূন্য pkReorder() {
        // Sort vertices found during forward and backward search
        ধ্রুব auto cmp = [এতত্](ধ্রুব DfgVertex* ধ্রুব ap, ধ্রুব DfgVertex* ধ্রুব bp) {
            প্রত্যাগম m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::sort(m_bwdVtxps.begin(), m_bwdVtxps.end(), cmp);
        std::sort(m_fwdVtxps.begin(), m_fwdVtxps.end(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ধ্রুব size_t bwdSize = m_bwdVtxps.size();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.ঢোকাও(m_bwdVtxps.end(), m_fwdVtxps.begin(), m_fwdVtxps.end());
        // Save the current ordering numbers, reset visitation marks
        প্রতি (DfgVertex* ধ্রুব vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.visited = অসত্য;
            m_ords.ঠেলো_back(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_merge(m_ords.begin(), m_ords.begin() + bwdSize, m_ords.end());
        // Assign new ordering
        প্রতি (size_t i = 0; i < m_ords.size(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    স্থির তার্কিক ignoredSink(ধ্রুব DfgVertex& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        যদি (ধ্রুব DfgVarPacked* ধ্রুব varp = sink.cast<DfgVarPacked>()) {
            যদি (!varp->hasSinks() && !varp->isObserved()) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    শূন্য findCandidatess() {
        প্রতি (DfgVertex& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ধ্রুব catp = vtx.cast<DfgConcat>();
            যদি (!catp) অনুবর্ত;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ধ্রুব DfgVertex& sink) {
                যদি (sink.is<DfgSel>()) {
                    ++nSels;
                } অন্যথা যদি (sink.is<DfgConcat>()) {
                    ++nCats;
                } অন্যথা যদি (!ignoredSink(sink)) {
                    ++nOther;
                }
                প্রত্যাগম অসত্য;
            });

            // Consider if optimizable
            যদি (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ঠেলো_back(catp);
                m_stateMap[catp].onWorklist = সত্য;
            }
        }
    }

    শূন্য ঠেলো_Down_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::vector<DfgSel*> selps;
        selps.reserve(m_dfg.size());
        // Consider each concatenation
        যাবত্ (!m_catps.empty()) {
            DfgConcat* ধ্রুব catp = m_catps.back();
            m_catps.বের_করো_back();
            m_stateMap[catp].onWorklist = অসত্য;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            DfgVertex* sinkp = nullptr;  // The only non-DfgSel sink (ignoring some DfgVars)
            ধ্রুব তার্কিক multipleNonSelSinks = catp->foreachSink([&](DfgVertex& sink) {
                // Collect selects
                যদি (DfgSel* ধ্রুব selp = sink.cast<DfgSel>()) {
                    selps.emplace_back(selp);
                    প্রত্যাগম অসত্য;
                }
                // Skip ignored sinks
                যদি (ignoredSink(sink)) প্রত্যাগম অসত্য;
                // If already found a non-DfgSel sink, return true
                যদি (sinkp) প্রত্যাগম সত্য;
                // Save the non-DfgSel sink
                sinkp = &sink;
                প্রত্যাগম অসত্য;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            যদি (multipleNonSelSinks) অনুবর্ত;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.empty(), catp, "Should have selects");
            // If no other sink, then nothing to do
            যদি (!sinkp) অনুবর্ত;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ধ্রুব sinkCatp = sinkp->cast<DfgConcat>();
            যদি (!sinkCatp) অনুবর্ত;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ধ্রুব uint32_t offset = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->width();
            ধ্রুব uint32_t pushedDownBefore = m_ctx.m_pushedDown;
            প্রতি (DfgSel* ধ্রুব selp : selps) {
                // Don't do it if it would create a cycle
                যদি (!addEdge(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    অনুবর্ত;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushedDown;
                selp->lsb(selp->lsb() + offset);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            যদি (pushedDownBefore != m_ctx.m_pushedDown && !sinkCatState.onWorklist) {
                m_catps.ঠেলো_back(sinkCatp);
                sinkCatState.onWorklist = সত্য;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ঠেলো_Down_Sels(DfgGraph& dfg, V3_Dfg_ঠেলো_Down_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.size());
        findCandidatess();
        // Early exit if nothing to do
        যদি (m_catps.empty()) প্রত্যাগম;

        // Pre-allocate storage
        m_স্তূপ.reserve(m_dfg.size());
        m_fwdVtxps.reserve(m_dfg.size());
        m_bwdVtxps.reserve(m_dfg.size());
        m_ords.reserve(m_dfg.size());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::sort(m_catps.begin(), m_catps.end(),
                  [এতত্](ধ্রুব DfgConcat* ধ্রুব ap, ধ্রুব DfgConcat* ধ্রুব bp) {
                      প্রত্যাগম m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ঠেলো_Down_Sels();
    }

সার্বজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, V3_Dfg_ঠেলো_Down_Sels_Context& ctx) {
        V3_Dfg_ঠেলো_Down_Sels{dfg, ctx};
    }
};

শূন্য V3DfgPasses::ঠেলো_Down_Sels(DfgGraph& dfg, V3_Dfg_ঠেলো_Down_Sels_Context& ctx) {
    যদি (!v3Global.opt.f_Dfg_ঠেলো_Down_Sels()) প্রত্যাগম;
    V3_Dfg_ঠেলো_Down_Sels::apply(dfg, ctx);
}
