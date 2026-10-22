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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"
#అన్తర్భావ "V3Error.h"

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి V3_Dfg_నెట్టు_కిందికి_Sels final {
    // TYPES

    // Each vertex has an associated State via DfgUserMap
    నిర్మాణం State final {
        // -- For Pearce-Kelly algorithm only
        // Topological ordering index. For all pair of vertices (a, b),
        // ord(a) < ord(b) iff there is no path from b to a in the graph.
        uint32_t ord = 0;
        తార్కిక సందర్శించిన = అబద్ధం;  // Whether the vertex has been visited during DFS
        // -- For the actial optimization only management
        తార్కిక onWorklist = అబద్ధం;  // Whether the vertex is in m_catps
    };

    // STATE
    // The graph being processed - must be acyclic (DAG)
    Dfg_లేఖాచిత్రం& m_dfg;
    // Context for pass
    V3_Dfg_నెట్టు_కిందికి_Sels_Context& m_ctx;
    // Map from DfgVertex to State
    DfgUserMap<State> m_stateMap = m_dfg.makeUserMap<State>();

    // STATE - Temporaries for Pearce-Kelly algorithm - as members to avoid reallocations
    std::సదిశ<Dfg_శీర్షం*> m_దొంతర;  // DFS stack for various steps
    std::సదిశ<Dfg_శీర్షం*> m_fwdVtxps;  // Vertices found during forward DFS
    std::సదిశ<Dfg_శీర్షం*> m_bwdVtxps;  // Vertices found during backward DFS - also work buffer
    std::సదిశ<uint32_t> m_ords;  // Ordering numbers reassigned in current ordering update

    // STATE - For vertex movement
    std::సదిశ<DfgConcat*> m_catps;  // DfgConcat vertices that may be optimizable

    // METHODS - Pearce-Kelly algorithm
    శూన్య debugCheck() {
        ఒకవేళ (VL_LIKELY(!v3Global.opt.debugCheck())) తిరిగి;
        m_dfg.for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& src) {
            స్థిరరాశి State& srcState = m_stateMap[src];
            UASSERT_OBJ(!srcState.సందర్శించిన, &src, "Visit marker not reset");
            UASSERT_OBJ(srcState.ord > 0, &src, "No ordering assigned");
            src.foreachSink([&](స్థిరరాశి Dfg_శీర్షం& dst) {
                స్థిరరాశి State& dstState = m_stateMap[dst];
                UASSERT_OBJ(srcState.ord < dstState.ord, &src, "Invalid ordering");
                తిరిగి అబద్ధం;
            });
        });
    }

    // Find initial topological ordering using reverse post order numbering via DFS
    శూన్య initializeOrdering() {
        // Start from all vertices with no inputs
        m_దొంతర.reserve(m_dfg.కొలత());
        కొరకు (Dfg_శీర్షం_Var& vtx : m_dfg.varVertices()) {
            ఒకవేళ (vtx.srcp() || vtx.defaultp()) కొనసాగు;
            m_దొంతర.నెట్టు_వెనుక(&vtx);
        }
        కొరకు (DfgConst& vtx : m_dfg.constVertices()) m_దొంతర.నెట్టు_వెనుక(&vtx);
        కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
            ఒకవేళ (!vtx.nInputs()) m_దొంతర.నెట్టు_వెనుక(&vtx);
        }

        // Reverse post order number to assign to next vertex
        uint32_t rpo_తదుపరి = m_dfg.కొలత();

        // DFS loop
        అయితే (!m_దొంతర.ఖాళీలు()) {
            Dfg_శీర్షం& vtx = *m_దొంతర.వెనుక();
            State& vtxState = m_stateMap[vtx];
            // If the ordering already assigned, just pop. It was visited
            // through another path through a different child.
            ఒకవేళ (vtxState.ord) {
                UASSERT_OBJ(vtxState.సందర్శించిన, &vtx, "Not visited, but ordering assigned");
                m_దొంతర.తీయి_వెనుక();
                కొనసాగు;
            }
            // When exiting a vertex, assign the reverse post order number as ordering
            ఒకవేళ (vtxState.సందర్శించిన) {
                vtxState.ord = rpo_తదుపరి--;
                m_దొంతర.తీయి_వెనుక();
                కొనసాగు;
            }
            // Entering vertex. Enqueue all unvisited children.
            vtxState.సందర్శించిన = నిజం;
            vtx.foreachSink([&](Dfg_శీర్షం& dst) {
                స్థిరరాశి State& dstState = m_stateMap[dst];
                ఒకవేళ (dstState.సందర్శించిన) తిరిగి అబద్ధం;
                m_దొంతర.నెట్టు_వెనుక(&dst);
                తిరిగి అబద్ధం;
            });
        }

        // Should reach exact zero
        UASSERT(!rpo_తదుపరి, "All vertics should have been visited exactly once");

        // Reset marks
        m_dfg.for_Each_శీర్షం([&](Dfg_శీర్షం& vtx) { m_stateMap[vtx].సందర్శించిన = అబద్ధం; });

        // Make sure it's valid
        debugCheck();
    }

    // Attempt to add an edge to the graph. Returns false if this would create
    // a cycle, and in that case, no state is modified, so it is safe to then
    // not add the actual edge. Otherwise returns true and updates state as
    // if the edge was indeed added, so caller must add the actual edge.
    తార్కిక కూడు_అంచు(Dfg_శీర్షం& src, Dfg_శీర్షం& dst) {
        UASSERT_OBJ(&src != &dst, &src, "Should be different");
        స్థిరరాశి State& srcState = m_stateMap[src];
        స్థిరరాశి State& dstState = m_stateMap[dst];
        // If 'dst' is after 'src' in the topological ordering,
        // then ok to add edge and no need to update the ordering.
        ఒకవేళ (dstState.ord > srcState.ord) తిరిగి నిజం;
        // Pearce-Kelly dicovery step
        ఒకవేళ (pkFwdDfs(src, dst)) తిరిగి అబద్ధం;
        pkBwdDfs(src, dst);
        // Pearce-Kelly update step
        pkReorder();
        తిరిగి నిజం;
    }

    // Pearce-Kelly forward DFS discovery step. Record visited vertices.
    // Returns true if a cycle would be created by adding the edge (src, dst).
    తార్కిక pkFwdDfs(Dfg_శీర్షం& src, Dfg_శీర్షం& dst) {
        స్థిరరాశి uint32_t srcOrd = m_stateMap[src].ord;
        // DFS forward from dst
        m_దొంతర.నెట్టు_వెనుక(&dst);
        అయితే (!m_దొంతర.ఖాళీలు()) {
            Dfg_శీర్షం& vtx = *m_దొంతర.వెనుక();
            m_దొంతర.తీయి_వెనుక();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different sink
            ఒకవేళ (vtxState.సందర్శించిన) కొనసాగు;

            // Save vertex, mark visited
            m_fwdVtxps.నెట్టు_వెనుక(&vtx);
            vtxState.సందర్శించిన = నిజం;

            // Enqueue unvisited sinks in affeced area
            స్థిరరాశి తార్కిక cyclic = vtx.foreachSink([&](Dfg_శీర్షం& sink) {
                స్థిరరాశి State& sinkState = m_stateMap[sink];
                ఒకవేళ (sinkState.ord == srcOrd) తిరిగి నిజం;  // Stop completely if cyclic
                ఒకవేళ (sinkState.సందర్శించిన) తిరిగి అబద్ధం;  // Stop search if already visited
                ఒకవేళ (sinkState.ord > srcOrd) తిరిగి అబద్ధం;  // Stop search if outside critical area
                m_దొంతర.నెట్టు_వెనుక(&sink);
                తిరిగి అబద్ధం;
            });

            // If would be cyclic, reset state and return true
            ఒకవేళ (cyclic) {
                కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : m_fwdVtxps) m_stateMap[vtxp].సందర్శించిన = అబద్ధం;
                m_fwdVtxps.clear();
                m_దొంతర.clear();
                తిరిగి నిజం;
            }
        }
        // Won't be cyclic, return false
        తిరిగి అబద్ధం;
    }

    // Pearce-Kelly backward DFS discovery step. Record visited vertices.
    శూన్య pkBwdDfs(Dfg_శీర్షం& src, Dfg_శీర్షం& dst) {
        స్థిరరాశి uint32_t dstOrd = m_stateMap[dst].ord;
        // DFS backward from src
        m_దొంతర.నెట్టు_వెనుక(&src);
        అయితే (!m_దొంతర.ఖాళీలు()) {
            Dfg_శీర్షం& vtx = *m_దొంతర.వెనుక();
            m_దొంతర.తీయి_వెనుక();
            State& vtxState = m_stateMap[vtx];

            // Ignore if already visited through another path through different source
            ఒకవేళ (vtxState.సందర్శించిన) కొనసాగు;

            // Save vertex, mark visited
            m_bwdVtxps.నెట్టు_వెనుక(&vtx);
            vtxState.సందర్శించిన = నిజం;

            // Enqueue unvisited sources in affeced area
            vtx.foreach_ఆధారం([&](Dfg_శీర్షం& ఆధారం) {
                స్థిరరాశి State& ఆధారం_State = m_stateMap[ఆధారం];
                ఒకవేళ (ఆధారం_State.సందర్శించిన) తిరిగి అబద్ధం;  // Stop search if already visited
                ఒకవేళ (ఆధారం_State.ord < dstOrd)
                    తిరిగి అబద్ధం;  // Stop search if outside critical area
                m_దొంతర.నెట్టు_వెనుక(&ఆధారం);
                తిరిగి అబద్ధం;
            });
        }
    }

    // Pearce-Kelly reorder step
    శూన్య pkReorder() {
        // Sort vertices found during forward and backward search
        స్థిరరాశి స్వయంచాలక cmp = [ఇది](స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి ap, స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి bp) {
            తిరిగి m_stateMap[ap].ord < m_stateMap[bp].ord;
        };
        std::క్రమీకరించు(m_bwdVtxps.begin(), m_bwdVtxps.చివర(), cmp);
        std::క్రమీకరించు(m_fwdVtxps.begin(), m_fwdVtxps.చివర(), cmp);
        // Will use m_bwdVtxps for processing to avoid copying. Save the size.
        స్థిరరాశి కొలత_t bwd_కొలత = m_bwdVtxps.కొలత();
        // Append forward vertices to the backward list for processing
        m_bwdVtxps.చొప్పించు(m_bwdVtxps.చివర(), m_fwdVtxps.begin(), m_fwdVtxps.చివర());
        // Save the current ordering numbers, reset visitation marks
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : m_bwdVtxps) {
            State& state = m_stateMap[vtxp];
            state.సందర్శించిన = అబద్ధం;
            m_ords.నెట్టు_వెనుక(state.ord);
        }
        // The current ordering numbers are sorted in the two sub lists, merge them
        std::inplace_విలీనం(m_ords.begin(), m_ords.begin() + bwd_కొలత, m_ords.చివర());
        // Assign new ordering
        కొరకు (కొలత_t i = 0; i < m_ords.కొలత(); ++i) m_stateMap[m_bwdVtxps[i]].ord = m_ords[i];
        // Reset sate
        m_fwdVtxps.clear();
        m_bwdVtxps.clear();
        m_ords.clear();
        // Make sure it's valid
        debugCheck();
    }

    // METHODS - Vertex processing

    స్థిర తార్కిక ignoredSink(స్థిరరాశి Dfg_శీర్షం& sink) {
        // Ignore non-observable variable sinks. These will be eliminated.
        ఒకవేళ (స్థిరరాశి DfgVarPacked* స్థిరరాశి varp = sink.cast<DfgVarPacked>()) {
            ఒకవేళ (!varp->hasSinks() && !varp->isObserved()) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // Find all concatenations that feed another concatenation and may be
    // optimizable. These are the ones that feed a DfgSel, and no other
    // observable sinks. (If there were other observable sinks, a temporary
    // would be required anyway.)
    శూన్య కనుగొను_Candidatess() {
        కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) {
            // Consider only concatenations ...
            DfgConcat* స్థిరరాశి catp = vtx.cast<DfgConcat>();
            ఒకవేళ (!catp) కొనసాగు;

            // Count the various types of sinks
            uint32_t nSels = 0;
            uint32_t nCats = 0;
            uint32_t nOther = 0;
            vtx.foreachSink([&](స్థిరరాశి Dfg_శీర్షం& sink) {
                ఒకవేళ (sink.is<DfgSel>()) {
                    ++nSels;
                } లేకపోతే ఒకవేళ (sink.is<DfgConcat>()) {
                    ++nCats;
                } లేకపోతే ఒకవేళ (!ignoredSink(sink)) {
                    ++nOther;
                }
                తిరిగి అబద్ధం;
            });

            // Consider if optimizable
            ఒకవేళ (nSels > 0 && nCats == 1 && nOther == 0) {
                m_catps.నెట్టు_వెనుక(catp);
                m_stateMap[catp].onWorklist = నిజం;
            }
        }
    }

    శూన్య నెట్టు_కిందికి_Sels() {
        // Selects driven by the current vertex. Outside loop to avoid reallocation.
        std::సదిశ<DfgSel*> selps;
        selps.reserve(m_dfg.కొలత());
        // Consider each concatenation
        అయితే (!m_catps.ఖాళీలు()) {
            DfgConcat* స్థిరరాశి catp = m_catps.వెనుక();
            m_catps.తీయి_వెనుక();
            m_stateMap[catp].onWorklist = అబద్ధం;

            // Iterate sinks, collect selects, check if should be optimized
            selps.clear();
            Dfg_శీర్షం* sinkp = శూన్యసూచిక;  // The only non-DfgSel sink (ignoring some DfgVars)
            స్థిరరాశి తార్కిక గుణిజం_Non_Sel_Sinks = catp->foreachSink([&](Dfg_శీర్షం& sink) {
                // Collect selects
                ఒకవేళ (DfgSel* స్థిరరాశి selp = sink.cast<DfgSel>()) {
                    selps.emplace_వెనుక(selp);
                    తిరిగి అబద్ధం;
                }
                // Skip ignored sinks
                ఒకవేళ (ignoredSink(sink)) తిరిగి అబద్ధం;
                // If already found a non-DfgSel sink, return true
                ఒకవేళ (sinkp) తిరిగి నిజం;
                // Save the non-DfgSel sink
                sinkp = &sink;
                తిరిగి అబద్ధం;
            });

            // It it has multiple non-DfgSel sinks, it will need a temporary, so don't bother
            ఒకవేళ (గుణిజం_Non_Sel_Sinks) కొనసాగు;
            // We only add DfgConcats to the work list that drive a select.
            UASSERT_OBJ(!selps.ఖాళీలు(), catp, "Should have selects");
            // If no other sink, then nothing to do
            ఒకవేళ (!sinkp) కొనసాగు;
            // If the only other sink is not a concatenation, then nothing to do
            DfgConcat* స్థిరరాశి sinkCatp = sinkp->cast<DfgConcat>();
            ఒకవేళ (!sinkCatp) కొనసాగు;

            // Ok, we can try to push the selects down to the sink DfgConcat
            స్థిరరాశి uint32_t స్థానభ్రంశం = sinkCatp->rhsp() == catp ? 0 : sinkCatp->rhsp()->వెడల్పు();
            స్థిరరాశి uint32_t pushed_కిందికి_Before = m_ctx.m_pushed_కిందికి;
            కొరకు (DfgSel* స్థిరరాశి selp : selps) {
                // Don't do it if it would create a cycle
                ఒకవేళ (!కూడు_అంచు(*sinkCatp, *selp)) {
                    ++m_ctx.m_wouldBeCyclic;
                    కొనసాగు;
                }
                // Otherwise redirect the select
                ++m_ctx.m_pushed_కిందికి;
                selp->lsb(selp->lsb() + స్థానభ్రంశం);
                selp->fromp(sinkCatp);
            }
            // If we pushed down any selects, then we need to consider the sink concatenation
            // again
            State& sinkCatState = m_stateMap[sinkCatp];
            ఒకవేళ (pushed_కిందికి_Before != m_ctx.m_pushed_కిందికి && !sinkCatState.onWorklist) {
                m_catps.నెట్టు_వెనుక(sinkCatp);
                sinkCatState.onWorklist = నిజం;
            }
        }
    }

    // CONSTRUCTOR
    V3_Dfg_నెట్టు_కిందికి_Sels(Dfg_లేఖాచిత్రం& dfg, V3_Dfg_నెట్టు_కిందికి_Sels_Context& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Find optimization candidates
        m_catps.reserve(m_dfg.కొలత());
        కనుగొను_Candidatess();
        // Early exit if nothing to do
        ఒకవేళ (m_catps.ఖాళీలు()) తిరిగి;

        // Pre-allocate storage
        m_దొంతర.reserve(m_dfg.కొలత());
        m_fwdVtxps.reserve(m_dfg.కొలత());
        m_bwdVtxps.reserve(m_dfg.కొలత());
        m_ords.reserve(m_dfg.కొలత());

        // Initialize topologicel ordering
        initializeOrdering();

        // Sort candidates in topological order so we process them the least amount
        std::క్రమీకరించు(m_catps.begin(), m_catps.చివర(),
                  [ఇది](స్థిరరాశి DfgConcat* స్థిరరాశి ap, స్థిరరాశి DfgConcat* స్థిరరాశి bp) {
                      తిరిగి m_stateMap[ap].ord < m_stateMap[bp].ord;
                  });

        // Push selects down to the lowest concatenation
        నెట్టు_కిందికి_Sels();
    }

బహిరంగ:
    స్థిర శూన్య apply(Dfg_లేఖాచిత్రం& dfg, V3_Dfg_నెట్టు_కిందికి_Sels_Context& ctx) {
        V3_Dfg_నెట్టు_కిందికి_Sels{dfg, ctx};
    }
};

శూన్య V3DfgPasses::నెట్టు_కిందికి_Sels(Dfg_లేఖాచిత్రం& dfg, V3_Dfg_నెట్టు_కిందికి_Sels_Context& ctx) {
    ఒకవేళ (!v3Global.opt.f_Dfg_నెట్టు_కిందికి_Sels()) తిరిగి;
    V3_Dfg_నెట్టు_కిందికి_Sels::apply(dfg, ctx);
}
