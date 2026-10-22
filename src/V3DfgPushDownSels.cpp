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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    ರಚನೆ State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        ತಾರ್ಕಿಕ ಭೇಟಿಯಾದ = ಸುಳ್ಳು;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        ತಾರ್ಕಿಕ onWorklist = ಸುಳ್ಳು;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    Dfg_ನಕ್ಷೆ& m_dfg;
    // Context for pass
    V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::ಸದಿಶ<Dfg_ಶೃಂಗ*> m_ರಾಶಿ;  // DFS stack for various steps
    std::ಸದಿಶ<Dfg_ಶೃಂಗ*> m_fwdVtxps;  // Vertices found during forward DFS
    std::ಸದಿಶ<Dfg_ಶೃಂಗ*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::ಸದಿಶ<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::ಸದಿಶ<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    ಶೂನ್ಯ debugCheck() {
        ಆದರೆ (VL_LIKELY(!v3Global.opt.debugCheck())) ಹಿಂತಿರುಗಿ;
        m_dfg.for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {
            ಸ್ಥಿರಾಂಕ State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.ಭೇಟಿಯಾದ, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& dst) {
                ಸ್ಥಿರಾಂಕ State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    ಶೂನ್ಯ initializeOrdering() {
        // Start from all vertices with no inputs
        m_ರಾಶಿ.reserve(m_dfg.ಗಾತ್ರ_2());
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : m_dfg.varVertices()) {
            ಆದರೆ (vtx.srcp() || vtx.defaultp()) ಮುಂದುವರಿ;
            m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
        }
        ಗಾಗಿ (DfgConst& vtx : m_dfg.constVertices()) m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
            ಆದರೆ (!vtx.nInputs()) m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpo_ಮುಂದಿನ = m_dfg.ಗಾತ್ರ_2();

        // DFS loop
        ಆಗ (!m_ರಾಶಿ.ಖಾಲಿಗಳು()) {
            Dfg_ಶೃಂಗ& vtx = *m_ರಾಶಿ.ಹಿಂಭಾಗ();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            ಆದರೆ (vtxState.ord) {
                UASSERT_OBJ(vtxState.ಭೇಟಿಯಾದ, &vtx, "Not visited, but ordering assigned");
                m_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
                ಮುಂದುವರಿ;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            ಆದರೆ (vtxState.ಭೇಟಿಯಾದ) {
                vtxState.ord = rpo_ಮುಂದಿನ--;
                m_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
                ಮುಂದುವರಿ;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.ಭೇಟಿಯಾದ = ನಿಜ;
            vtx.foreachSink([&](Dfg_ಶೃಂಗ& dst) {
                ಸ್ಥಿರಾಂಕ State& dstState = m_stateMap[dst];
                ಆದರೆ (dstState.ಭೇಟಿಯಾದ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&dst);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        }

        // Should reach exact zero
        UASSERT(!rpo_ಮುಂದಿನ, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.for_Each_ಶೃಂಗ([&](Dfg_ಶೃಂಗ& vtx) { m_stateMap[vtx].ಭೇಟಿಯಾದ = ಸುಳ್ಳು; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    ತಾರ್ಕಿಕ ಕೂಡಿಸು_ಅಂಚು(Dfg_ಶೃಂಗ& src, Dfg_ಶೃಂಗ& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        ಸ್ಥಿರಾಂಕ State& srcState = m_stateMap[src];
        ಸ್ಥಿರಾಂಕ State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        ಆದರೆ (dstState.ord > srcState.ord) ಹಿಂತಿರುಗಿ ನಿಜ;
        // Pearce-Kelly dicovery step
        ಆದರೆ (pkFwdDfs(src, dst)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    ತಾರ್ಕಿಕ pkFwdDfs(Dfg_ಶೃಂಗ& src, Dfg_ಶೃಂಗ& dst) {
        ಸ್ಥಿರಾಂಕ uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&dst);
        ಆಗ (!m_ರಾಶಿ.ಖಾಲಿಗಳು()) {
            Dfg_ಶೃಂಗ& vtx = *m_ರಾಶಿ.ಹಿಂಭಾಗ();
            m_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            ಆದರೆ (vtxState.ಭೇಟಿಯಾದ) ಮುಂದುವರಿ;

            // Save vertex, mark visited
            m_fwdVtxps.ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
            vtxState.ಭೇಟಿಯಾದ = ನಿಜ;

            // Enqueue unvisited sinks in affeced area
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ cyclic = vtx.foreachSink([&](Dfg_ಶೃಂಗ& sink) {
                ಸ್ಥಿರಾಂಕ State& sinkState = m_stateMap[sink];
                ಆದರೆ (sinkState.ord == srcOrd) ಹಿಂತಿರುಗಿ ನಿಜ;  // Stop completely if cyclic
                ಆದರೆ (sinkState.ಭೇಟಿಯಾದ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Stop search if already visited
                ಆದರೆ (sinkState.ord > srcOrd) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Stop search if outside critical area
                m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&sink);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });

            // If would be cyclic, reset state and return true
            ಆದರೆ (cyclic) {
                ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_fwdVtxps) m_stateMap[vtxp].ಭೇಟಿಯಾದ = ಸುಳ್ಳು;
                m_fwdVtxps.clear();
                m_ರಾಶಿ.clear();
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        // Won't be cyclic, return false
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    ಶೂನ್ಯ pkBwdDfs(Dfg_ಶೃಂಗ& src, Dfg_ಶೃಂಗ& dst) {
        ಸ್ಥಿರಾಂಕ uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&src);
        ಆಗ (!m_ರಾಶಿ.ಖಾಲಿಗಳು()) {
            Dfg_ಶೃಂಗ& vtx = *m_ರಾಶಿ.ಹಿಂಭಾಗ();
            m_ರಾಶಿ.ತೆಗೆ_ಹಿಂಭಾಗ();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            ಆದರೆ (vtxState.ಭೇಟಿಯಾದ) ಮುಂದುವರಿ;

            // Save vertex, mark visited
            m_bwdVtxps.ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
            vtxState.ಭೇಟಿಯಾದ = ನಿಜ;

            // Enqueue unvisited sources in affeced area
            vtx.foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& ಮೂಲ) {
                ಸ್ಥಿರಾಂಕ State& ಮೂಲ_State = m_stateMap[ಮೂಲ];
                ಆದರೆ (ಮೂಲ_State.ಭೇಟಿಯಾದ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Stop search if already visited
                ಆದರೆ (ಮೂಲ_State.ord < dstOrd)
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Stop search if outside critical area
                m_ರಾಶಿ.ತಳ್ಳು_ಹಿಂಭಾಗ(&ಮೂಲ);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        }
    }

    // Pearce-Kelly reorder step
    ಶೂನ್ಯ pkReorder() {
        // Sort vertices found during forward and backward search
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ cmp = [ಇದು](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ap, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ bp) {
            ಹಿಂತಿರುಗಿ m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::ವಿಂಗಡಿಸು(m_bwdVtxps.begin(), m_bwdVtxps.ಕೊನೆ(), cmp);
        std::ವಿಂಗಡಿಸು(m_fwdVtxps.begin(), m_fwdVtxps.ಕೊನೆ(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t bwd_ಗಾತ್ರ = m_bwdVtxps.ಗಾತ್ರ_2();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.ಸೇರಿಸು(m_bwdVtxps.ಕೊನೆ(), m_fwdVtxps.begin(), m_fwdVtxps.ಕೊನೆ());
        // Save the current ordering numbers, reset visitation marks
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.ಭೇಟಿಯಾದ = ಸುಳ್ಳು;
            m_ords.ತಳ್ಳು_ಹಿಂಭಾಗ(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_ವಿಲೀನ(m_ords.begin(), m_ords.begin() + bwd_ಗಾತ್ರ, m_ords.ಕೊನೆ());
        // Assign new ordering
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < m_ords.ಗಾತ್ರ_2(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    ಸ್ಥಿರ ತಾರ್ಕಿಕ ignoredSink(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgVarPacked* ಸ್ಥಿರಾಂಕ varp = sink.cast<DfgVarPacked>()) {
            ಆದರೆ (!varp->hasSinks() && !varp->isObserved()) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    ಶೂನ್ಯ ಕಂಡುಹಿಡಿ_Candidatess() {
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* ಸ್ಥಿರಾಂಕ catp = vtx.cast<DfgConcat>();
            ಆದರೆ (!catp) ಮುಂದುವರಿ;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& sink) {
                ಆದರೆ (sink.is<DfgSel>()) {
                    ++nSels;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (sink.is<DfgConcat>()) {
                    ++nCats;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ignoredSink(sink)) {
                    ++nOther;
                }
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });

            // Consider if optimizable
            ಆದರೆ (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.ತಳ್ಳು_ಹಿಂಭಾಗ(catp);
                m_stateMap[catp].onWorklist = ನಿಜ;
            }
        }
    }

    ಶೂನ್ಯ ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::ಸದಿಶ<DfgSel*> selps;
        selps.reserve(m_dfg.ಗಾತ್ರ_2());
        // Consider each concatenation
        ಆಗ (!m_catps.ಖಾಲಿಗಳು()) {
            DfgConcat* ಸ್ಥಿರಾಂಕ catp = m_catps.ಹಿಂಭಾಗ();
            m_catps.ತೆಗೆ_ಹಿಂಭಾಗ();
            m_stateMap[catp].onWorklist = ಸುಳ್ಳು;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            Dfg_ಶೃಂಗ* sinkp = ಶೂನ್ಯಸೂಚಕ;  // The only non-DfgSel sink (ignoring some DfgVars)
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಅಪವರ್ತ್ಯ_Non_Sel_Sinks = catp->foreachSink([&](Dfg_ಶೃಂಗ& sink) {
                // Collect selects
                ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = sink.cast<DfgSel>()) {
                    selps.emplace_ಹಿಂಭಾಗ(selp);
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                // Skip ignored sinks
                ಆದರೆ (ignoredSink(sink)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // If already found a non-DfgSel sink, return true
                ಆದರೆ (sinkp) ಹಿಂತಿರುಗಿ ನಿಜ;
                // Save the non-DfgSel sink
                sinkp = &sink;
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            ಆದರೆ (ಅಪವರ್ತ್ಯ_Non_Sel_Sinks) ಮುಂದುವರಿ;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.ಖಾಲಿಗಳು(), catp, "Should have selects");
            // If no other sink, then nothing to do
            ಆದರೆ (!sinkp) ಮುಂದುವರಿ;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* ಸ್ಥಿರಾಂಕ sinkCatp = sinkp->cast<DfgConcat>();
            ಆದರೆ (!sinkCatp) ಮುಂದುವರಿ;

            // Ok, we can try to push the selects down to the sink DfgConcat
            ಸ್ಥಿರಾಂಕ uint32_t ಸ್ಥಾನಾಂತರ = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ uint32_t pushed_ಕೆಳಕ್ಕೆ_Before = m_ctx.m_pushed_ಕೆಳಕ್ಕೆ;
            ಗಾಗಿ (DfgSel* ಸ್ಥಿರಾಂಕ selp : selps) {
                // Don't do it if it would create a cycle
                ಆದರೆ (!ಕೂಡಿಸು_ಅಂಚು(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    ಮುಂದುವರಿ;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushed_ಕೆಳಕ್ಕೆ;
                selp->lsb(selp->lsb() + ಸ್ಥಾನಾಂತರ);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            ಆದರೆ (pushed_ಕೆಳಕ್ಕೆ_Before != m_ctx.m_pushed_ಕೆಳಕ್ಕೆ && !sinkCatState.onWorklist) {
                m_catps.ತಳ್ಳು_ಹಿಂಭಾಗ(sinkCatp);
                sinkCatState.onWorklist = ನಿಜ;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels(Dfg_ನಕ್ಷೆ& dfg, V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.ಗಾತ್ರ_2());
        ಕಂಡುಹಿಡಿ_Candidatess();
        // Early exit if nothing to do
        ಆದರೆ (m_catps.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

        // Pre-allocate storage
        m_ರಾಶಿ.reserve(m_dfg.ಗಾತ್ರ_2());
        m_fwdVtxps.reserve(m_dfg.ಗಾತ್ರ_2());
        m_bwdVtxps.reserve(m_dfg.ಗಾತ್ರ_2());
        m_ords.reserve(m_dfg.ಗಾತ್ರ_2());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::ವಿಂಗಡಿಸು(m_catps.begin(), m_catps.ಕೊನೆ(),
                  [ಇದು](ಸ್ಥಿರಾಂಕ DfgConcat* ಸ್ಥಿರಾಂಕ ap, ಸ್ಥಿರಾಂಕ DfgConcat* ಸ್ಥಿರಾಂಕ bp) {
                      ಹಿಂತಿರುಗಿ m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels();
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Dfg_ನಕ್ಷೆ& dfg, V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context& ctx) {
        V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels{dfg, ctx};
    }
};

ಶೂನ್ಯ V3DfgPasses::ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels(Dfg_ನಕ್ಷೆ& dfg, V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context& ctx) {
    ಆದರೆ (!v3Global.opt.f_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels()) ಹಿಂತಿರುಗಿ;
    V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels::apply(dfg, ctx);
}
