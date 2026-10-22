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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

صنف V3_Dfg_ڌڪو_هيٺ_ڏانهن_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    بنية State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        منطقي گهميل = خطأ;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        منطقي onWorklist = خطأ;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    Dfg_گراف& m_dfg;
    // Context for pass
    V3_Dfg_ڌڪو_هيٺ_ڏانهن_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::ويڪٽر<Dfg_راس*> m_ڍير;  // DFS stack for various steps
    std::ويڪٽر<Dfg_راس*> m_fwdVtxps;  // Vertices found during forward DFS
    std::ويڪٽر<Dfg_راس*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::ويڪٽر<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::ويڪٽر<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    فراغ debugCheck() {
        إذا (VL_LIKELY(!v3Global.opt.debugCheck())) أرجع;
        m_dfg.for_Each_راس([&](ثابت Dfg_راس& src) {
            ثابت State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.گهميل, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ثابت Dfg_راس& dst) {
                ثابت State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                أرجع خطأ;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    فراغ initializeOrdering() {
        // Start from all vertices with no inputs
        m_ڍير.reserve(m_dfg.سائيز());
        لكل (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            إذا (vtx.srcp() || vtx.defaultp()) استمر;
            m_ڍير.ڌڪو_پٺ(&vtx);
        }
        لكل (DfgConst& vtx : m_dfg.constVertices()) m_ڍير.ڌڪو_پٺ(&vtx);
        لكل (Dfg_راس& vtx : m_dfg.opVertices()) {
            إذا (!vtx.nInputs()) m_ڍير.ڌڪو_پٺ(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpo_ايندڙ = m_dfg.سائيز();

        // DFS loop
        طالما (!m_ڍير.خالي_جايون()) {
            Dfg_راس& vtx = *m_ڍير.پٺ();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            إذا (vtxState.ord) {
                UASSERT_OBJ(vtxState.گهميل, &vtx, "Not visited, but ordering assigned");
                m_ڍير.ڪڍو_پٺ();
                استمر;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            إذا (vtxState.گهميل) {
                vtxState.ord = rpo_ايندڙ--;
                m_ڍير.ڪڍو_پٺ();
                استمر;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.گهميل = سچ;
            vtx.foreachSink([&](Dfg_راس& dst) {
                ثابت State& dstState = m_stateMap[dst];
                إذا (dstState.گهميل) أرجع خطأ;
                m_ڍير.ڌڪو_پٺ(&dst);
                أرجع خطأ;
            });
        }

        // Should reach exact zero
        UASSERT(!rpo_ايندڙ, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.for_Each_راس([&](Dfg_راس& vtx) { m_stateMap[vtx].گهميل = خطأ; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    منطقي جوڙيو_ڪنارو(Dfg_راس& src, Dfg_راس& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ثابت State& srcState = m_stateMap[src];
        ثابت State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        إذا (dstState.ord > srcState.ord) أرجع سچ;
        // Pearce-Kelly dicovery step
        إذا (pkFwdDfs(src, dst)) أرجع خطأ;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        أرجع سچ;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    منطقي pkFwdDfs(Dfg_راس& src, Dfg_راس& dst) {
        ثابت uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ڍير.ڌڪو_پٺ(&dst);
        طالما (!m_ڍير.خالي_جايون()) {
            Dfg_راس& vtx = *m_ڍير.پٺ();
            m_ڍير.ڪڍو_پٺ();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            إذا (vtxState.گهميل) استمر;

            // Save vertex, mark visited
            m_fwdVtxps.ڌڪو_پٺ(&vtx);
            vtxState.گهميل = سچ;

            // Enqueue unvisited sinks in affeced area
            ثابت منطقي cyclic = vtx.foreachSink([&](Dfg_راس& sink) {
                ثابت State& sinkState = m_stateMap[sink];
                إذا (sinkState.ord == srcOrd) أرجع سچ;  // Stop completely if cyclic
                إذا (sinkState.گهميل) أرجع خطأ;  // Stop search if already visited
                إذا (sinkState.ord > srcOrd) أرجع خطأ;  // Stop search if outside critical area
                m_ڍير.ڌڪو_پٺ(&sink);
                أرجع خطأ;
            });

            // If would be cyclic, reset state and return true
            إذا (cyclic) {
                لكل (Dfg_راس* ثابت vtxp : m_fwdVtxps) m_stateMap[vtxp].گهميل = خطأ;
                m_fwdVtxps.clear();
                m_ڍير.clear();
                أرجع سچ;
            }
        }
        // Won't be cyclic, return false
        أرجع خطأ;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    فراغ pkBwdDfs(Dfg_راس& src, Dfg_راس& dst) {
        ثابت uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ڍير.ڌڪو_پٺ(&src);
        طالما (!m_ڍير.خالي_جايون()) {
            Dfg_راس& vtx = *m_ڍير.پٺ();
            m_ڍير.ڪڍو_پٺ();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            إذا (vtxState.گهميل) استمر;

            // Save vertex, mark visited
            m_bwdVtxps.ڌڪو_پٺ(&vtx);
            vtxState.گهميل = سچ;

            // Enqueue unvisited sources in affeced area
            vtx.foreach_ذريعو([&](Dfg_راس& ذريعو) {
                ثابت State& ذريعو_State = m_stateMap[ذريعو];
                إذا (ذريعو_State.گهميل) أرجع خطأ;  // Stop search if already visited
                إذا (ذريعو_State.ord < dstOrd)
                    أرجع خطأ;  // Stop search if outside critical area
                m_ڍير.ڌڪو_پٺ(&ذريعو);
                أرجع خطأ;
            });
        }
    }

    // Pearce-Kelly reorder step
    فراغ pkReorder() {
        // Sort vertices found during forward and backward search
        ثابت خودڪار cmp = [هذا](ثابت Dfg_راس* ثابت ap, ثابت Dfg_راس* ثابت bp) {
            أرجع m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::ترتيب_ڏيو(m_bwdVtxps.begin(), m_bwdVtxps.پڇاڙي(), cmp);
        std::ترتيب_ڏيو(m_fwdVtxps.begin(), m_fwdVtxps.پڇاڙي(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ثابت سائيز_t bwd_سائيز = m_bwdVtxps.سائيز();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.وجهو(m_bwdVtxps.پڇاڙي(), m_fwdVtxps.begin(), m_fwdVtxps.پڇاڙي());
        // Save the current ordering numbers, reset visitation marks
        لكل (Dfg_راس* ثابت vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.گهميل = خطأ;
            m_ords.ڌڪو_پٺ(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_ملايو(m_ords.begin(), m_ords.begin() + bwd_سائيز, m_ords.پڇاڙي());
        // Assign new ordering
        لكل (سائيز_t i = 0; i < m_ords.سائيز(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    ساكن منطقي ignoredSink(ثابت Dfg_راس& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        إذا (ثابت DfgVarPacked* ثابت varp = sink.cast<DfgVarPacked>()) {
            إذا (!varp->hasSinks() && !varp->isObserved()) أرجع سچ;
        }
        أرجع خطأ;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    فراغ ڳوليو_Candidatess() {
        لكل (Dfg_راس& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ثابت catp = vtx.cast<DfgConcat>();
            إذا (!catp) استمر;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ثابت Dfg_راس& sink) {
                إذا (sink.is<DfgSel>()) {
                    ++nSels;
                } وإلا إذا (sink.is<DfgConcat>()) {
                    ++nCats;
                } وإلا إذا (!ignoredSink(sink)) {
                    ++nOther;
                }
                أرجع خطأ;
            });

            // Consider if optimizable
            إذا (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ڌڪو_پٺ(catp);
                m_stateMap[catp].onWorklist = سچ;
            }
        }
    }

    فراغ ڌڪو_هيٺ_ڏانهن_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::ويڪٽر<DfgSel*> selps;
        selps.reserve(m_dfg.سائيز());
        // Consider each concatenation
        طالما (!m_catps.خالي_جايون()) {
            DfgConcat* ثابت catp = m_catps.پٺ();
            m_catps.ڪڍو_پٺ();
            m_stateMap[catp].onWorklist = خطأ;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            Dfg_راس* sinkp = خالي_اشارو;  // The only non-DfgSel sink (ignoring some DfgVars)
            ثابت منطقي گڻو_Non_Sel_Sinks = catp->foreachSink([&](Dfg_راس& sink) {
                // Collect selects
                إذا (DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    selps.emplace_پٺ(selp);
                    أرجع خطأ;
                }
                // Skip ignored sinks
                إذا (ignoredSink(sink)) أرجع خطأ;
                // If already found a non-DfgSel sink, return true
                إذا (sinkp) أرجع سچ;
                // Save the non-DfgSel sink
                sinkp = &sink;
                أرجع خطأ;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            إذا (گڻو_Non_Sel_Sinks) استمر;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.خالي_جايون(), catp, "Should have selects");
            // If no other sink, then nothing to do
            إذا (!sinkp) استمر;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ثابت sinkCatp = sinkp->cast<DfgConcat>();
            إذا (!sinkCatp) استمر;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ثابت uint32_t وٿي = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->ويڪر();
            ثابت uint32_t pushed_هيٺ_ڏانهن_Before = m_ctx.m_pushed_هيٺ_ڏانهن;
            لكل (DfgSel* ثابت selp : selps) {
                // Don't do it if it would create a cycle
                إذا (!جوڙيو_ڪنارو(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    استمر;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushed_هيٺ_ڏانهن;
                selp->lsb(selp->lsb() + وٿي);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            إذا (pushed_هيٺ_ڏانهن_Before != m_ctx.m_pushed_هيٺ_ڏانهن && !sinkCatState.onWorklist) {
                m_catps.ڌڪو_پٺ(sinkCatp);
                sinkCatState.onWorklist = سچ;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ڌڪو_هيٺ_ڏانهن_Sels(Dfg_گراف& dfg, V3_Dfg_ڌڪو_هيٺ_ڏانهن_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.سائيز());
        ڳوليو_Candidatess();
        // Early exit if nothing to do
        إذا (m_catps.خالي_جايون()) أرجع;

        // Pre-allocate storage
        m_ڍير.reserve(m_dfg.سائيز());
        m_fwdVtxps.reserve(m_dfg.سائيز());
        m_bwdVtxps.reserve(m_dfg.سائيز());
        m_ords.reserve(m_dfg.سائيز());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::ترتيب_ڏيو(m_catps.begin(), m_catps.پڇاڙي(),
                  [هذا](ثابت DfgConcat* ثابت ap, ثابت DfgConcat* ثابت bp) {
                      أرجع m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ڌڪو_هيٺ_ڏانهن_Sels();
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3_Dfg_ڌڪو_هيٺ_ڏانهن_Sels_Context& ctx) {
        V3_Dfg_ڌڪو_هيٺ_ڏانهن_Sels{dfg, ctx};
    }
};

فراغ V3DfgPasses::ڌڪو_هيٺ_ڏانهن_Sels(Dfg_گراف& dfg, V3_Dfg_ڌڪو_هيٺ_ڏانهن_Sels_Context& ctx) {
    إذا (!v3Global.opt.f_Dfg_ڌڪو_هيٺ_ڏانهن_Sels()) أرجع;
    V3_Dfg_ڌڪو_هيٺ_ڏانهن_Sels::apply(dfg, ctx);
}
