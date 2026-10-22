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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    ਬਣਤਰ State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        ਤਾਰਕਿਕ ਦੇਖਿਆ = ਝੂਠ;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        ਤਾਰਕਿਕ onWorklist = ਝੂਠ;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    Dfg_ਗ੍ਰਾਫ& m_dfg;
    // Context for pass
    V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> m_ਢੇਰ;  // DFS stack for various steps
    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> m_fwdVtxps;  // Vertices found during forward DFS
    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::ਸਦਿਸ਼<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::ਸਦਿਸ਼<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    ਖਾਲੀ debugCheck() {
        ਜੇ (VL_LIKELY(!v3Global.opt.debugCheck())) ਵਾਪਸ;
        m_dfg.for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {
            ਅਚਲ State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.ਦੇਖਿਆ, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& dst) {
                ਅਚਲ State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                ਵਾਪਸ ਝੂਠ;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    ਖਾਲੀ initializeOrdering() {
        // Start from all vertices with no inputs
        m_ਢੇਰ.reserve(m_dfg.ਮਾਪ());
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : m_dfg.varVertices()) {
            ਜੇ (vtx.srcp() || vtx.defaultp()) ਜਾਰੀ;
            m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&vtx);
        }
        ਲਈ (DfgConst& vtx : m_dfg.constVertices()) m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&vtx);
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
            ਜੇ (!vtx.nInputs()) m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpo_ਅਗਲਾ = m_dfg.ਮਾਪ();

        // DFS loop
        ਜਦੋਂ (!m_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            Dfg_ਸ਼ੀਰਸ਼& vtx = *m_ਢੇਰ.ਪਿੱਠ();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            ਜੇ (vtxState.ord) {
                UASSERT_OBJ(vtxState.ਦੇਖਿਆ, &vtx, "Not visited, but ordering assigned");
                m_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
                ਜਾਰੀ;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            ਜੇ (vtxState.ਦੇਖਿਆ) {
                vtxState.ord = rpo_ਅਗਲਾ--;
                m_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
                ਜਾਰੀ;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.ਦੇਖਿਆ = ਸੱਚ;
            vtx.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& dst) {
                ਅਚਲ State& dstState = m_stateMap[dst];
                ਜੇ (dstState.ਦੇਖਿਆ) ਵਾਪਸ ਝੂਠ;
                m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&dst);
                ਵਾਪਸ ਝੂਠ;
            });
        }

        // Should reach exact zero
        UASSERT(!rpo_ਅਗਲਾ, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.for_Each_ਸ਼ੀਰਸ਼([&](Dfg_ਸ਼ੀਰਸ਼& vtx) { m_stateMap[vtx].ਦੇਖਿਆ = ਝੂਠ; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    ਤਾਰਕਿਕ ਜੋੜੋ_ਕਿਨਾਰਾ(Dfg_ਸ਼ੀਰਸ਼& src, Dfg_ਸ਼ੀਰਸ਼& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ਅਚਲ State& srcState = m_stateMap[src];
        ਅਚਲ State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        ਜੇ (dstState.ord > srcState.ord) ਵਾਪਸ ਸੱਚ;
        // Pearce-Kelly dicovery step
        ਜੇ (pkFwdDfs(src, dst)) ਵਾਪਸ ਝੂਠ;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        ਵਾਪਸ ਸੱਚ;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    ਤਾਰਕਿਕ pkFwdDfs(Dfg_ਸ਼ੀਰਸ਼& src, Dfg_ਸ਼ੀਰਸ਼& dst) {
        ਅਚਲ uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&dst);
        ਜਦੋਂ (!m_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            Dfg_ਸ਼ੀਰਸ਼& vtx = *m_ਢੇਰ.ਪਿੱਠ();
            m_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            ਜੇ (vtxState.ਦੇਖਿਆ) ਜਾਰੀ;

            // Save vertex, mark visited
            m_fwdVtxps.ਧੱਕੋ_ਪਿੱਠ(&vtx);
            vtxState.ਦੇਖਿਆ = ਸੱਚ;

            // Enqueue unvisited sinks in affeced area
            ਅਚਲ ਤਾਰਕਿਕ cyclic = vtx.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) {
                ਅਚਲ State& sinkState = m_stateMap[sink];
                ਜੇ (sinkState.ord == srcOrd) ਵਾਪਸ ਸੱਚ;  // Stop completely if cyclic
                ਜੇ (sinkState.ਦੇਖਿਆ) ਵਾਪਸ ਝੂਠ;  // Stop search if already visited
                ਜੇ (sinkState.ord > srcOrd) ਵਾਪਸ ਝੂਠ;  // Stop search if outside critical area
                m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&sink);
                ਵਾਪਸ ਝੂਠ;
            });

            // If would be cyclic, reset state and return true
            ਜੇ (cyclic) {
                ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_fwdVtxps) m_stateMap[vtxp].ਦੇਖਿਆ = ਝੂਠ;
                m_fwdVtxps.clear();
                m_ਢੇਰ.clear();
                ਵਾਪਸ ਸੱਚ;
            }
        }
        // Won't be cyclic, return false
        ਵਾਪਸ ਝੂਠ;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    ਖਾਲੀ pkBwdDfs(Dfg_ਸ਼ੀਰਸ਼& src, Dfg_ਸ਼ੀਰਸ਼& dst) {
        ਅਚਲ uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&src);
        ਜਦੋਂ (!m_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            Dfg_ਸ਼ੀਰਸ਼& vtx = *m_ਢੇਰ.ਪਿੱਠ();
            m_ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            ਜੇ (vtxState.ਦੇਖਿਆ) ਜਾਰੀ;

            // Save vertex, mark visited
            m_bwdVtxps.ਧੱਕੋ_ਪਿੱਠ(&vtx);
            vtxState.ਦੇਖਿਆ = ਸੱਚ;

            // Enqueue unvisited sources in affeced area
            vtx.foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& ਸਰੋਤ) {
                ਅਚਲ State& ਸਰੋਤ_State = m_stateMap[ਸਰੋਤ];
                ਜੇ (ਸਰੋਤ_State.ਦੇਖਿਆ) ਵਾਪਸ ਝੂਠ;  // Stop search if already visited
                ਜੇ (ਸਰੋਤ_State.ord < dstOrd)
                    ਵਾਪਸ ਝੂਠ;  // Stop search if outside critical area
                m_ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(&ਸਰੋਤ);
                ਵਾਪਸ ਝੂਠ;
            });
        }
    }

    // Pearce-Kelly reorder step
    ਖਾਲੀ pkReorder() {
        // Sort vertices found during forward and backward search
        ਅਚਲ ਸਵੈਚਾਲਿਤ cmp = [ਇਹ](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ ap, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ bp) {
            ਵਾਪਸ m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::ਛਾਂਟੋ(m_bwdVtxps.begin(), m_bwdVtxps.ਸਿਰਾ(), cmp);
        std::ਛਾਂਟੋ(m_fwdVtxps.begin(), m_fwdVtxps.ਸਿਰਾ(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ਅਚਲ ਮਾਪ_t bwd_ਮਾਪ = m_bwdVtxps.ਮਾਪ();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.ਪਾਓ(m_bwdVtxps.ਸਿਰਾ(), m_fwdVtxps.begin(), m_fwdVtxps.ਸਿਰਾ());
        // Save the current ordering numbers, reset visitation marks
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.ਦੇਖਿਆ = ਝੂਠ;
            m_ords.ਧੱਕੋ_ਪਿੱਠ(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_ਮਿਲਾਓ(m_ords.begin(), m_ords.begin() + bwd_ਮਾਪ, m_ords.ਸਿਰਾ());
        // Assign new ordering
        ਲਈ (ਮਾਪ_t i = 0; i < m_ords.ਮਾਪ(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    ਸਥਿਰ ਤਾਰਕਿਕ ignoredSink(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        ਜੇ (ਅਚਲ DfgVarPacked* ਅਚਲ varp = sink.cast<DfgVarPacked>()) {
            ਜੇ (!varp->hasSinks() && !varp->isObserved()) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    ਖਾਲੀ ਲੱਭੋ_Candidatess() {
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ਅਚਲ catp = vtx.cast<DfgConcat>();
            ਜੇ (!catp) ਜਾਰੀ;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& sink) {
                ਜੇ (sink.is<DfgSel>()) {
                    ++nSels;
                } ਨਹੀਂਤਾਂ ਜੇ (sink.is<DfgConcat>()) {
                    ++nCats;
                } ਨਹੀਂਤਾਂ ਜੇ (!ignoredSink(sink)) {
                    ++nOther;
                }
                ਵਾਪਸ ਝੂਠ;
            });

            // Consider if optimizable
            ਜੇ (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ਧੱਕੋ_ਪਿੱਠ(catp);
                m_stateMap[catp].onWorklist = ਸੱਚ;
            }
        }
    }

    ਖਾਲੀ ਧੱਕੋ_ਥੱਲੇ_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::ਸਦਿਸ਼<DfgSel*> selps;
        selps.reserve(m_dfg.ਮਾਪ());
        // Consider each concatenation
        ਜਦੋਂ (!m_catps.ਖਾਲੀ_ਥਾਵਾਂ()) {
            DfgConcat* ਅਚਲ catp = m_catps.ਪਿੱਠ();
            m_catps.ਕੱਢੋ_ਪਿੱਠ();
            m_stateMap[catp].onWorklist = ਝੂਠ;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            Dfg_ਸ਼ੀਰਸ਼* sinkp = ਖਾਲੀ_ਸੂਚਕ;  // The only non-DfgSel sink (ignoring some DfgVars)
            ਅਚਲ ਤਾਰਕਿਕ ਗੁਣਜ_Non_Sel_Sinks = catp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) {
                // Collect selects
                ਜੇ (DfgSel* ਅਚਲ selp = sink.cast<DfgSel>()) {
                    selps.emplace_ਪਿੱਠ(selp);
                    ਵਾਪਸ ਝੂਠ;
                }
                // Skip ignored sinks
                ਜੇ (ignoredSink(sink)) ਵਾਪਸ ਝੂਠ;
                // If already found a non-DfgSel sink, return true
                ਜੇ (sinkp) ਵਾਪਸ ਸੱਚ;
                // Save the non-DfgSel sink
                sinkp = &sink;
                ਵਾਪਸ ਝੂਠ;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            ਜੇ (ਗੁਣਜ_Non_Sel_Sinks) ਜਾਰੀ;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.ਖਾਲੀ_ਥਾਵਾਂ(), catp, "Should have selects");
            // If no other sink, then nothing to do
            ਜੇ (!sinkp) ਜਾਰੀ;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ਅਚਲ sinkCatp = sinkp->cast<DfgConcat>();
            ਜੇ (!sinkCatp) ਜਾਰੀ;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ਅਚਲ uint32_t ਵਿੱਥ = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->ਚੌੜਾਈ();
            ਅਚਲ uint32_t pushed_ਥੱਲੇ_Before = m_ctx.m_pushed_ਥੱਲੇ;
            ਲਈ (DfgSel* ਅਚਲ selp : selps) {
                // Don't do it if it would create a cycle
                ਜੇ (!ਜੋੜੋ_ਕਿਨਾਰਾ(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    ਜਾਰੀ;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushed_ਥੱਲੇ;
                selp->lsb(selp->lsb() + ਵਿੱਥ);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            ਜੇ (pushed_ਥੱਲੇ_Before != m_ctx.m_pushed_ਥੱਲੇ && !sinkCatState.onWorklist) {
                m_catps.ਧੱਕੋ_ਪਿੱਠ(sinkCatp);
                sinkCatState.onWorklist = ਸੱਚ;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels(Dfg_ਗ੍ਰਾਫ& dfg, V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.ਮਾਪ());
        ਲੱਭੋ_Candidatess();
        // Early exit if nothing to do
        ਜੇ (m_catps.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

        // Pre-allocate storage
        m_ਢੇਰ.reserve(m_dfg.ਮਾਪ());
        m_fwdVtxps.reserve(m_dfg.ਮਾਪ());
        m_bwdVtxps.reserve(m_dfg.ਮਾਪ());
        m_ords.reserve(m_dfg.ਮਾਪ());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::ਛਾਂਟੋ(m_catps.begin(), m_catps.ਸਿਰਾ(),
                  [ਇਹ](ਅਚਲ DfgConcat* ਅਚਲ ap, ਅਚਲ DfgConcat* ਅਚਲ bp) {
                      ਵਾਪਸ m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ਧੱਕੋ_ਥੱਲੇ_Sels();
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Dfg_ਗ੍ਰਾਫ& dfg, V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context& ctx) {
        V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels{dfg, ctx};
    }
};

ਖਾਲੀ V3DfgPasses::ਧੱਕੋ_ਥੱਲੇ_Sels(Dfg_ਗ੍ਰਾਫ& dfg, V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context& ctx) {
    ਜੇ (!v3Global.opt.f_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels()) ਵਾਪਸ;
    V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels::apply(dfg, ctx);
}
