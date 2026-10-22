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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

جماعت V3_Dfg_ڈالو_نیچے_کو_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    ساخت State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        منطقی دیکھا = جھوٹ;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        منطقی onWorklist = جھوٹ;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    Dfg_گراف& m_dfg;
    // Context for pass
    V3_Dfg_ڈالو_نیچے_کو_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::سمتیہ<Dfg_راس*> m_ڈھیر;  // DFS stack for various steps
    std::سمتیہ<Dfg_راس*> m_fwdVtxps;  // Vertices found during forward DFS
    std::سمتیہ<Dfg_راس*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::سمتیہ<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::سمتیہ<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    خالی debugCheck() {
        اگر (VL_LIKELY(!v3Global.opt.debugCheck())) واپس;
        m_dfg.for_Each_راس([&](ثابت Dfg_راس& src) {
            ثابت State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.دیکھا, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ثابت Dfg_راس& dst) {
                ثابت State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                واپس جھوٹ;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    خالی initializeOrdering() {
        // Start from all vertices with no inputs
        m_ڈھیر.reserve(m_dfg.سائز());
        برائے (Dfg_راس_Var& vtx : m_dfg.varVertices()) {
            اگر (vtx.srcp() || vtx.defaultp()) جاری;
            m_ڈھیر.ڈالو_عقب(&vtx);
        }
        برائے (DfgConst& vtx : m_dfg.constVertices()) m_ڈھیر.ڈالو_عقب(&vtx);
        برائے (Dfg_راس& vtx : m_dfg.opVertices()) {
            اگر (!vtx.nInputs()) m_ڈھیر.ڈالو_عقب(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpo_اگلا = m_dfg.سائز();

        // DFS loop
        جب (!m_ڈھیر.خالی_خانے()) {
            Dfg_راس& vtx = *m_ڈھیر.عقب();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            اگر (vtxState.ord) {
                UASSERT_OBJ(vtxState.دیکھا, &vtx, "Not visited, but ordering assigned");
                m_ڈھیر.نکالو_عقب();
                جاری;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            اگر (vtxState.دیکھا) {
                vtxState.ord = rpo_اگلا--;
                m_ڈھیر.نکالو_عقب();
                جاری;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.دیکھا = سچ;
            vtx.foreachSink([&](Dfg_راس& dst) {
                ثابت State& dstState = m_stateMap[dst];
                اگر (dstState.دیکھا) واپس جھوٹ;
                m_ڈھیر.ڈالو_عقب(&dst);
                واپس جھوٹ;
            });
        }

        // Should reach exact zero
        UASSERT(!rpo_اگلا, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.for_Each_راس([&](Dfg_راس& vtx) { m_stateMap[vtx].دیکھا = جھوٹ; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    منطقی جوڑو_کنارہ(Dfg_راس& src, Dfg_راس& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ثابت State& srcState = m_stateMap[src];
        ثابت State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        اگر (dstState.ord > srcState.ord) واپس سچ;
        // Pearce-Kelly dicovery step
        اگر (pkFwdDfs(src, dst)) واپس جھوٹ;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        واپس سچ;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    منطقی pkFwdDfs(Dfg_راس& src, Dfg_راس& dst) {
        ثابت uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ڈھیر.ڈالو_عقب(&dst);
        جب (!m_ڈھیر.خالی_خانے()) {
            Dfg_راس& vtx = *m_ڈھیر.عقب();
            m_ڈھیر.نکالو_عقب();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            اگر (vtxState.دیکھا) جاری;

            // Save vertex, mark visited
            m_fwdVtxps.ڈالو_عقب(&vtx);
            vtxState.دیکھا = سچ;

            // Enqueue unvisited sinks in affeced area
            ثابت منطقی cyclic = vtx.foreachSink([&](Dfg_راس& sink) {
                ثابت State& sinkState = m_stateMap[sink];
                اگر (sinkState.ord == srcOrd) واپس سچ;  // Stop completely if cyclic
                اگر (sinkState.دیکھا) واپس جھوٹ;  // Stop search if already visited
                اگر (sinkState.ord > srcOrd) واپس جھوٹ;  // Stop search if outside critical area
                m_ڈھیر.ڈالو_عقب(&sink);
                واپس جھوٹ;
            });

            // If would be cyclic, reset state and return true
            اگر (cyclic) {
                برائے (Dfg_راس* ثابت vtxp : m_fwdVtxps) m_stateMap[vtxp].دیکھا = جھوٹ;
                m_fwdVtxps.clear();
                m_ڈھیر.clear();
                واپس سچ;
            }
        }
        // Won't be cyclic, return false
        واپس جھوٹ;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    خالی pkBwdDfs(Dfg_راس& src, Dfg_راس& dst) {
        ثابت uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ڈھیر.ڈالو_عقب(&src);
        جب (!m_ڈھیر.خالی_خانے()) {
            Dfg_راس& vtx = *m_ڈھیر.عقب();
            m_ڈھیر.نکالو_عقب();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            اگر (vtxState.دیکھا) جاری;

            // Save vertex, mark visited
            m_bwdVtxps.ڈالو_عقب(&vtx);
            vtxState.دیکھا = سچ;

            // Enqueue unvisited sources in affeced area
            vtx.foreach_ماخذ([&](Dfg_راس& ماخذ) {
                ثابت State& ماخذ_State = m_stateMap[ماخذ];
                اگر (ماخذ_State.دیکھا) واپس جھوٹ;  // Stop search if already visited
                اگر (ماخذ_State.ord < dstOrd)
                    واپس جھوٹ;  // Stop search if outside critical area
                m_ڈھیر.ڈالو_عقب(&ماخذ);
                واپس جھوٹ;
            });
        }
    }

    // Pearce-Kelly reorder step
    خالی pkReorder() {
        // Sort vertices found during forward and backward search
        ثابت خودکار cmp = [یہ](ثابت Dfg_راس* ثابت ap, ثابت Dfg_راس* ثابت bp) {
            واپس m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::ترتیب_دو(m_bwdVtxps.begin(), m_bwdVtxps.انتہا(), cmp);
        std::ترتیب_دو(m_fwdVtxps.begin(), m_fwdVtxps.انتہا(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ثابت سائز_t bwd_سائز = m_bwdVtxps.سائز();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.داخل_کرو(m_bwdVtxps.انتہا(), m_fwdVtxps.begin(), m_fwdVtxps.انتہا());
        // Save the current ordering numbers, reset visitation marks
        برائے (Dfg_راس* ثابت vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.دیکھا = جھوٹ;
            m_ords.ڈالو_عقب(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_ملاؤ(m_ords.begin(), m_ords.begin() + bwd_سائز, m_ords.انتہا());
        // Assign new ordering
        برائے (سائز_t i = 0; i < m_ords.سائز(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    ساکن منطقی ignoredSink(ثابت Dfg_راس& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        اگر (ثابت DfgVarPacked* ثابت varp = sink.cast<DfgVarPacked>()) {
            اگر (!varp->hasSinks() && !varp->isObserved()) واپس سچ;
        }
        واپس جھوٹ;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    خالی ڈھونڈو_Candidatess() {
        برائے (Dfg_راس& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ثابت catp = vtx.cast<DfgConcat>();
            اگر (!catp) جاری;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ثابت Dfg_راس& sink) {
                اگر (sink.is<DfgSel>()) {
                    ++nSels;
                } ورنہ اگر (sink.is<DfgConcat>()) {
                    ++nCats;
                } ورنہ اگر (!ignoredSink(sink)) {
                    ++nOther;
                }
                واپس جھوٹ;
            });

            // Consider if optimizable
            اگر (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ڈالو_عقب(catp);
                m_stateMap[catp].onWorklist = سچ;
            }
        }
    }

    خالی ڈالو_نیچے_کو_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::سمتیہ<DfgSel*> selps;
        selps.reserve(m_dfg.سائز());
        // Consider each concatenation
        جب (!m_catps.خالی_خانے()) {
            DfgConcat* ثابت catp = m_catps.عقب();
            m_catps.نکالو_عقب();
            m_stateMap[catp].onWorklist = جھوٹ;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            Dfg_راس* sinkp = خالی_اشاریہ;  // The only non-DfgSel sink (ignoring some DfgVars)
            ثابت منطقی مضرب_Non_Sel_Sinks = catp->foreachSink([&](Dfg_راس& sink) {
                // Collect selects
                اگر (DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    selps.emplace_عقب(selp);
                    واپس جھوٹ;
                }
                // Skip ignored sinks
                اگر (ignoredSink(sink)) واپس جھوٹ;
                // If already found a non-DfgSel sink, return true
                اگر (sinkp) واپس سچ;
                // Save the non-DfgSel sink
                sinkp = &sink;
                واپس جھوٹ;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            اگر (مضرب_Non_Sel_Sinks) جاری;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.خالی_خانے(), catp, "Should have selects");
            // If no other sink, then nothing to do
            اگر (!sinkp) جاری;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ثابت sinkCatp = sinkp->cast<DfgConcat>();
            اگر (!sinkCatp) جاری;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ثابت uint32_t انحراف = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->چوڑائی();
            ثابت uint32_t pushed_نیچے_کو_Before = m_ctx.m_pushed_نیچے_کو;
            برائے (DfgSel* ثابت selp : selps) {
                // Don't do it if it would create a cycle
                اگر (!جوڑو_کنارہ(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    جاری;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushed_نیچے_کو;
                selp->lsb(selp->lsb() + انحراف);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            اگر (pushed_نیچے_کو_Before != m_ctx.m_pushed_نیچے_کو && !sinkCatState.onWorklist) {
                m_catps.ڈالو_عقب(sinkCatp);
                sinkCatState.onWorklist = سچ;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ڈالو_نیچے_کو_Sels(Dfg_گراف& dfg, V3_Dfg_ڈالو_نیچے_کو_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.سائز());
        ڈھونڈو_Candidatess();
        // Early exit if nothing to do
        اگر (m_catps.خالی_خانے()) واپس;

        // Pre-allocate storage
        m_ڈھیر.reserve(m_dfg.سائز());
        m_fwdVtxps.reserve(m_dfg.سائز());
        m_bwdVtxps.reserve(m_dfg.سائز());
        m_ords.reserve(m_dfg.سائز());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::ترتیب_دو(m_catps.begin(), m_catps.انتہا(),
                  [یہ](ثابت DfgConcat* ثابت ap, ثابت DfgConcat* ثابت bp) {
                      واپس m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ڈالو_نیچے_کو_Sels();
    }

عوامی:
    ساکن خالی apply(Dfg_گراف& dfg, V3_Dfg_ڈالو_نیچے_کو_Sels_Context& ctx) {
        V3_Dfg_ڈالو_نیچے_کو_Sels{dfg, ctx};
    }
};

خالی V3DfgPasses::ڈالو_نیچے_کو_Sels(Dfg_گراف& dfg, V3_Dfg_ڈالو_نیچے_کو_Sels_Context& ctx) {
    اگر (!v3Global.opt.f_Dfg_ڈالو_نیچے_کو_Sels()) واپس;
    V3_Dfg_ڈالو_نیچے_کو_Sels::apply(dfg, ctx);
}
