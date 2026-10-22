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

صنف V3_Dfg_افزودن_پایین_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    بنية State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        منطقي دیدهشده = خطأ;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        منطقي onWorklist = خطأ;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    Dfg_گراف& m_dfg;
    // Context for pass
    V3_Dfg_افزودن_پایین_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::وکتور<Dfg_رأس*> m_پشته;  // DFS stack for various steps
    std::وکتور<Dfg_رأس*> m_fwdVtxps;  // Vertices found during forward DFS
    std::وکتور<Dfg_رأس*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::وکتور<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::وکتور<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    فراغ debugCheck() {
        إذا (VL_LIKELY(!v3Global.opt.debugCheck())) أرجع;
        m_dfg.for_Each_رأس([&](ثابت Dfg_رأس& src) {
            ثابت State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.دیدهشده, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ثابت Dfg_رأس& dst) {
                ثابت State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                أرجع خطأ;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    فراغ initializeOrdering() {
        // Start from all vertices with no inputs
        m_پشته.reserve(m_dfg.اندازه());
        لكل (Dfg_رأس_Var& vtx : m_dfg.varVertices()) {
            إذا (vtx.srcp() || vtx.defaultp()) استمر;
            m_پشته.افزودن_پشت(&vtx);
        }
        لكل (DfgConst& vtx : m_dfg.constVertices()) m_پشته.افزودن_پشت(&vtx);
        لكل (Dfg_رأس& vtx : m_dfg.opVertices()) {
            إذا (!vtx.nInputs()) m_پشته.افزودن_پشت(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpo_بعدی = m_dfg.اندازه();

        // DFS loop
        طالما (!m_پشته.خالی()) {
            Dfg_رأس& vtx = *m_پشته.پشت();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            إذا (vtxState.ord) {
                UASSERT_OBJ(vtxState.دیدهشده, &vtx, "Not visited, but ordering assigned");
                m_پشته.برداشت_پشت();
                استمر;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            إذا (vtxState.دیدهشده) {
                vtxState.ord = rpo_بعدی--;
                m_پشته.برداشت_پشت();
                استمر;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.دیدهشده = درست;
            vtx.foreachSink([&](Dfg_رأس& dst) {
                ثابت State& dstState = m_stateMap[dst];
                إذا (dstState.دیدهشده) أرجع خطأ;
                m_پشته.افزودن_پشت(&dst);
                أرجع خطأ;
            });
        }

        // Should reach exact zero
        UASSERT(!rpo_بعدی, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.for_Each_رأس([&](Dfg_رأس& vtx) { m_stateMap[vtx].دیدهشده = خطأ; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    منطقي جمع_کن_یال(Dfg_رأس& src, Dfg_رأس& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ثابت State& srcState = m_stateMap[src];
        ثابت State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        إذا (dstState.ord > srcState.ord) أرجع درست;
        // Pearce-Kelly dicovery step
        إذا (pkFwdDfs(src, dst)) أرجع خطأ;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        أرجع درست;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    منطقي pkFwdDfs(Dfg_رأس& src, Dfg_رأس& dst) {
        ثابت uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_پشته.افزودن_پشت(&dst);
        طالما (!m_پشته.خالی()) {
            Dfg_رأس& vtx = *m_پشته.پشت();
            m_پشته.برداشت_پشت();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            إذا (vtxState.دیدهشده) استمر;

            // Save vertex, mark visited
            m_fwdVtxps.افزودن_پشت(&vtx);
            vtxState.دیدهشده = درست;

            // Enqueue unvisited sinks in affeced area
            ثابت منطقي cyclic = vtx.foreachSink([&](Dfg_رأس& sink) {
                ثابت State& sinkState = m_stateMap[sink];
                إذا (sinkState.ord == srcOrd) أرجع درست;  // Stop completely if cyclic
                إذا (sinkState.دیدهشده) أرجع خطأ;  // Stop search if already visited
                إذا (sinkState.ord > srcOrd) أرجع خطأ;  // Stop search if outside critical area
                m_پشته.افزودن_پشت(&sink);
                أرجع خطأ;
            });

            // If would be cyclic, reset state and return true
            إذا (cyclic) {
                لكل (Dfg_رأس* ثابت vtxp : m_fwdVtxps) m_stateMap[vtxp].دیدهشده = خطأ;
                m_fwdVtxps.clear();
                m_پشته.clear();
                أرجع درست;
            }
        }
        // Won't be cyclic, return false
        أرجع خطأ;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    فراغ pkBwdDfs(Dfg_رأس& src, Dfg_رأس& dst) {
        ثابت uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_پشته.افزودن_پشت(&src);
        طالما (!m_پشته.خالی()) {
            Dfg_رأس& vtx = *m_پشته.پشت();
            m_پشته.برداشت_پشت();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            إذا (vtxState.دیدهشده) استمر;

            // Save vertex, mark visited
            m_bwdVtxps.افزودن_پشت(&vtx);
            vtxState.دیدهشده = درست;

            // Enqueue unvisited sources in affeced area
            vtx.foreach_منبع([&](Dfg_رأس& منبع) {
                ثابت State& منبع_State = m_stateMap[منبع];
                إذا (منبع_State.دیدهشده) أرجع خطأ;  // Stop search if already visited
                إذا (منبع_State.ord < dstOrd)
                    أرجع خطأ;  // Stop search if outside critical area
                m_پشته.افزودن_پشت(&منبع);
                أرجع خطأ;
            });
        }
    }

    // Pearce-Kelly reorder step
    فراغ pkReorder() {
        // Sort vertices found during forward and backward search
        ثابت خودکار cmp = [هذا](ثابت Dfg_رأس* ثابت ap, ثابت Dfg_رأس* ثابت bp) {
            أرجع m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::مرتب_سازی(m_bwdVtxps.begin(), m_bwdVtxps.انتها(), cmp);
        std::مرتب_سازی(m_fwdVtxps.begin(), m_fwdVtxps.انتها(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ثابت اندازه_t bwd_اندازه = m_bwdVtxps.اندازه();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.درج(m_bwdVtxps.انتها(), m_fwdVtxps.begin(), m_fwdVtxps.انتها());
        // Save the current ordering numbers, reset visitation marks
        لكل (Dfg_رأس* ثابت vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.دیدهشده = خطأ;
            m_ords.افزودن_پشت(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_ادغام(m_ords.begin(), m_ords.begin() + bwd_اندازه, m_ords.انتها());
        // Assign new ordering
        لكل (اندازه_t i = 0; i < m_ords.اندازه(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    ساكن منطقي ignoredSink(ثابت Dfg_رأس& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        إذا (ثابت DfgVarPacked* ثابت varp = sink.cast<DfgVarPacked>()) {
            إذا (!varp->hasSinks() && !varp->isObserved()) أرجع درست;
        }
        أرجع خطأ;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    فراغ پیدا_کن_Candidatess() {
        لكل (Dfg_رأس& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ثابت catp = vtx.cast<DfgConcat>();
            إذا (!catp) استمر;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ثابت Dfg_رأس& sink) {
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
                m_catps.افزودن_پشت(catp);
                m_stateMap[catp].onWorklist = درست;
            }
        }
    }

    فراغ افزودن_پایین_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::وکتور<DfgSel*> selps;
        selps.reserve(m_dfg.اندازه());
        // Consider each concatenation
        طالما (!m_catps.خالی()) {
            DfgConcat* ثابت catp = m_catps.پشت();
            m_catps.برداشت_پشت();
            m_stateMap[catp].onWorklist = خطأ;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            Dfg_رأس* sinkp = اشاره_تهی;  // The only non-DfgSel sink (ignoring some DfgVars)
            ثابت منطقي مضرب_Non_Sel_Sinks = catp->foreachSink([&](Dfg_رأس& sink) {
                // Collect selects
                إذا (DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    selps.emplace_پشت(selp);
                    أرجع خطأ;
                }
                // Skip ignored sinks
                إذا (ignoredSink(sink)) أرجع خطأ;
                // If already found a non-DfgSel sink, return true
                إذا (sinkp) أرجع درست;
                // Save the non-DfgSel sink
                sinkp = &sink;
                أرجع خطأ;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            إذا (مضرب_Non_Sel_Sinks) استمر;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.خالی(), catp, "Should have selects");
            // If no other sink, then nothing to do
            إذا (!sinkp) استمر;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ثابت sinkCatp = sinkp->cast<DfgConcat>();
            إذا (!sinkCatp) استمر;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ثابت uint32_t جابه_جایی = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->عرض();
            ثابت uint32_t pushed_پایین_Before = m_ctx.m_pushed_پایین;
            لكل (DfgSel* ثابت selp : selps) {
                // Don't do it if it would create a cycle
                إذا (!جمع_کن_یال(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    استمر;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushed_پایین;
                selp->lsb(selp->lsb() + جابه_جایی);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            إذا (pushed_پایین_Before != m_ctx.m_pushed_پایین && !sinkCatState.onWorklist) {
                m_catps.افزودن_پشت(sinkCatp);
                sinkCatState.onWorklist = درست;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_افزودن_پایین_Sels(Dfg_گراف& dfg, V3_Dfg_افزودن_پایین_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.اندازه());
        پیدا_کن_Candidatess();
        // Early exit if nothing to do
        إذا (m_catps.خالی()) أرجع;

        // Pre-allocate storage
        m_پشته.reserve(m_dfg.اندازه());
        m_fwdVtxps.reserve(m_dfg.اندازه());
        m_bwdVtxps.reserve(m_dfg.اندازه());
        m_ords.reserve(m_dfg.اندازه());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::مرتب_سازی(m_catps.begin(), m_catps.انتها(),
                  [هذا](ثابت DfgConcat* ثابت ap, ثابت DfgConcat* ثابت bp) {
                      أرجع m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        افزودن_پایین_Sels();
    }

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3_Dfg_افزودن_پایین_Sels_Context& ctx) {
        V3_Dfg_افزودن_پایین_Sels{dfg, ctx};
    }
};

فراغ V3DfgPasses::افزودن_پایین_Sels(Dfg_گراف& dfg, V3_Dfg_افزودن_پایین_Sels_Context& ctx) {
    إذا (!v3Global.opt.f_Dfg_افزودن_پایین_Sels()) أرجع;
    V3_Dfg_افزودن_پایین_Sels::apply(dfg, ctx);
}
