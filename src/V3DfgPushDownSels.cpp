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

صنف V3_Dfg_ټېل_ښکته_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    بنية State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        منطقي لیدل_شوی = خطأ;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        منطقي onWorklist = خطأ;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    Dfg_ګراف& m_dfg;
    // Context for pass
    V3_Dfg_ټېل_ښکته_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::ویکټور<Dfg_راس*> m_ډېری;  // DFS stack for various steps
    std::ویکټور<Dfg_راس*> m_fwdVtxps;  // Vertices found during forward DFS
    std::ویکټور<Dfg_راس*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::ویکټور<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::ویکټور<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    فراغ debugCheck() {
        إذا (VL_LIKELY(!v3Global.opt.debugCheck())) أرجع;
        m_dfg.for_Each_راس([&](ثابت Dfg_راس& src) {
            ثابت State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.لیدل_شوی, &src, "Visit marker not reset");
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
        m_ډېری.reserve(m_dfg.اندازه());
        لكل (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            إذا (vtx.srcp() || vtx.defaultp()) استمر;
            m_ډېری.ټېل_شا(&vtx);
        }
        لكل (DfgConst& vtx : m_dfg.constVertices()) m_ډېری.ټېل_شا(&vtx);
        لكل (Dfg_راس& vtx : m_dfg.opVertices()) {
            إذا (!vtx.nInputs()) m_ډېری.ټېل_شا(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpo_بل = m_dfg.اندازه();

        // DFS loop
        طالما (!m_ډېری.تشه()) {
            Dfg_راس& vtx = *m_ډېری.شا();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            إذا (vtxState.ord) {
                UASSERT_OBJ(vtxState.لیدل_شوی, &vtx, "Not visited, but ordering assigned");
                m_ډېری.راوباسه_شا();
                استمر;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            إذا (vtxState.لیدل_شوی) {
                vtxState.ord = rpo_بل--;
                m_ډېری.راوباسه_شا();
                استمر;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.لیدل_شوی = ریښتیا;
            vtx.foreachSink([&](Dfg_راس& dst) {
                ثابت State& dstState = m_stateMap[dst];
                إذا (dstState.لیدل_شوی) أرجع خطأ;
                m_ډېری.ټېل_شا(&dst);
                أرجع خطأ;
            });
        }

        // Should reach exact zero
        UASSERT(!rpo_بل, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.for_Each_راس([&](Dfg_راس& vtx) { m_stateMap[vtx].لیدل_شوی = خطأ; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    منطقي جمع_کړه_څنډه(Dfg_راس& src, Dfg_راس& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ثابت State& srcState = m_stateMap[src];
        ثابت State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        إذا (dstState.ord > srcState.ord) أرجع ریښتیا;
        // Pearce-Kelly dicovery step
        إذا (pkFwdDfs(src, dst)) أرجع خطأ;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        أرجع ریښتیا;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    منطقي pkFwdDfs(Dfg_راس& src, Dfg_راس& dst) {
        ثابت uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ډېری.ټېل_شا(&dst);
        طالما (!m_ډېری.تشه()) {
            Dfg_راس& vtx = *m_ډېری.شا();
            m_ډېری.راوباسه_شا();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            إذا (vtxState.لیدل_شوی) استمر;

            // Save vertex, mark visited
            m_fwdVtxps.ټېل_شا(&vtx);
            vtxState.لیدل_شوی = ریښتیا;

            // Enqueue unvisited sinks in affeced area
            ثابت منطقي cyclic = vtx.foreachSink([&](Dfg_راس& sink) {
                ثابت State& sinkState = m_stateMap[sink];
                إذا (sinkState.ord == srcOrd) أرجع ریښتیا;  // Stop completely if cyclic
                إذا (sinkState.لیدل_شوی) أرجع خطأ;  // Stop search if already visited
                إذا (sinkState.ord > srcOrd) أرجع خطأ;  // Stop search if outside critical area
                m_ډېری.ټېل_شا(&sink);
                أرجع خطأ;
            });

            // If would be cyclic, reset state and return true
            إذا (cyclic) {
                لكل (Dfg_راس* ثابت vtxp : m_fwdVtxps) m_stateMap[vtxp].لیدل_شوی = خطأ;
                m_fwdVtxps.clear();
                m_ډېری.clear();
                أرجع ریښتیا;
            }
        }
        // Won't be cyclic, return false
        أرجع خطأ;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    فراغ pkBwdDfs(Dfg_راس& src, Dfg_راس& dst) {
        ثابت uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ډېری.ټېل_شا(&src);
        طالما (!m_ډېری.تشه()) {
            Dfg_راس& vtx = *m_ډېری.شا();
            m_ډېری.راوباسه_شا();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            إذا (vtxState.لیدل_شوی) استمر;

            // Save vertex, mark visited
            m_bwdVtxps.ټېل_شا(&vtx);
            vtxState.لیدل_شوی = ریښتیا;

            // Enqueue unvisited sources in affeced area
            vtx.foreach_سرچینه([&](Dfg_راس& سرچینه) {
                ثابت State& سرچینه_State = m_stateMap[سرچینه];
                إذا (سرچینه_State.لیدل_شوی) أرجع خطأ;  // Stop search if already visited
                إذا (سرچینه_State.ord < dstOrd)
                    أرجع خطأ;  // Stop search if outside critical area
                m_ډېری.ټېل_شا(&سرچینه);
                أرجع خطأ;
            });
        }
    }

    // Pearce-Kelly reorder step
    فراغ pkReorder() {
        // Sort vertices found during forward and backward search
        ثابت خپلکاری cmp = [هذا](ثابت Dfg_راس* ثابت ap, ثابت Dfg_راس* ثابت bp) {
            أرجع m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::ترتیبول(m_bwdVtxps.begin(), m_bwdVtxps.پای_ټکی(), cmp);
        std::ترتیبول(m_fwdVtxps.begin(), m_fwdVtxps.پای_ټکی(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ثابت اندازه_t bwd_اندازه = m_bwdVtxps.اندازه();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.ورننباسه(m_bwdVtxps.پای_ټکی(), m_fwdVtxps.begin(), m_fwdVtxps.پای_ټکی());
        // Save the current ordering numbers, reset visitation marks
        لكل (Dfg_راس* ثابت vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.لیدل_شوی = خطأ;
            m_ords.ټېل_شا(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_یوځای(m_ords.begin(), m_ords.begin() + bwd_اندازه, m_ords.پای_ټکی());
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

    ساكن منطقي ignoredSink(ثابت Dfg_راس& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        إذا (ثابت DfgVarPacked* ثابت varp = sink.cast<DfgVarPacked>()) {
            إذا (!varp->hasSinks() && !varp->isObserved()) أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    فراغ پیدا_کړه_Candidatess() {
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
                m_catps.ټېل_شا(catp);
                m_stateMap[catp].onWorklist = ریښتیا;
            }
        }
    }

    فراغ ټېل_ښکته_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::ویکټور<DfgSel*> selps;
        selps.reserve(m_dfg.اندازه());
        // Consider each concatenation
        طالما (!m_catps.تشه()) {
            DfgConcat* ثابت catp = m_catps.شا();
            m_catps.راوباسه_شا();
            m_stateMap[catp].onWorklist = خطأ;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            Dfg_راس* sinkp = تش_اشاره;  // The only non-DfgSel sink (ignoring some DfgVars)
            ثابت منطقي مضرب_Non_Sel_Sinks = catp->foreachSink([&](Dfg_راس& sink) {
                // Collect selects
                إذا (DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    selps.emplace_شا(selp);
                    أرجع خطأ;
                }
                // Skip ignored sinks
                إذا (ignoredSink(sink)) أرجع خطأ;
                // If already found a non-DfgSel sink, return true
                إذا (sinkp) أرجع ریښتیا;
                // Save the non-DfgSel sink
                sinkp = &sink;
                أرجع خطأ;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            إذا (مضرب_Non_Sel_Sinks) استمر;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.تشه(), catp, "Should have selects");
            // If no other sink, then nothing to do
            إذا (!sinkp) استمر;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ثابت sinkCatp = sinkp->cast<DfgConcat>();
            إذا (!sinkCatp) استمر;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ثابت uint32_t دننه_واټن = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->پلنوالی();
            ثابت uint32_t pushed_ښکته_Before = m_ctx.m_pushed_ښکته;
            لكل (DfgSel* ثابت selp : selps) {
                // Don't do it if it would create a cycle
                إذا (!جمع_کړه_څنډه(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    استمر;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushed_ښکته;
                selp->lsb(selp->lsb() + دننه_واټن);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            إذا (pushed_ښکته_Before != m_ctx.m_pushed_ښکته && !sinkCatState.onWorklist) {
                m_catps.ټېل_شا(sinkCatp);
                sinkCatState.onWorklist = ریښتیا;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ټېل_ښکته_Sels(Dfg_ګراف& dfg, V3_Dfg_ټېل_ښکته_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.اندازه());
        پیدا_کړه_Candidatess();
        // Early exit if nothing to do
        إذا (m_catps.تشه()) أرجع;

        // Pre-allocate storage
        m_ډېری.reserve(m_dfg.اندازه());
        m_fwdVtxps.reserve(m_dfg.اندازه());
        m_bwdVtxps.reserve(m_dfg.اندازه());
        m_ords.reserve(m_dfg.اندازه());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::ترتیبول(m_catps.begin(), m_catps.پای_ټکی(),
                  [هذا](ثابت DfgConcat* ثابت ap, ثابت DfgConcat* ثابت bp) {
                      أرجع m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ټېل_ښکته_Sels();
    }

عام:
    ساكن فراغ apply(Dfg_ګراف& dfg, V3_Dfg_ټېل_ښکته_Sels_Context& ctx) {
        V3_Dfg_ټېل_ښکته_Sels{dfg, ctx};
    }
};

فراغ V3DfgPasses::ټېل_ښکته_Sels(Dfg_ګراف& dfg, V3_Dfg_ټېل_ښکته_Sels_Context& ctx) {
    إذا (!v3Global.opt.f_Dfg_ټېل_ښکته_Sels()) أرجع;
    V3_Dfg_ټېل_ښکته_Sels::apply(dfg, ctx);
}
