// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Converting cyclic DFGs into acyclic DFGs
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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Hash.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <fstream>
#అన్తర్భావ <queue>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

నామాకాశ V3_Dfg_Break_చక్రాలు {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
తరగతి SccInfo final {
    Dfg_లేఖాచిత్రం& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<స్థిరరాశి Dfg_శీర్షం*, uint64_t> m_vtx2Scc;
    కొలత_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

బహిరంగ:
    SccInfo(Dfg_లేఖాచిత్రం& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) {
            స్థిరరాశి uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            ఒకవేళ (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    తార్కిక isCyclic() స్థిరరాశి { తిరిగి m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(స్థిరరాశి Dfg_శీర్షం& vtx) స్థిరరాశి { తిరిగి m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    శూన్య కూడు(స్థిరరాశి Dfg_శీర్షం& vtx, uint64_t sccIdx) {
        స్థిరరాశి తార్కిక new_నమోదు = m_vtx2Scc.emplace(&vtx, sccIdx).రెండవ;
        UASSERT_OBJ(new_నమోదు, &vtx, "Vertex already inserted");
        ఒకవేళ (sccIdx) ++m_nCyclicVertices;
    }

    తార్కిక stillCyclicFwd(స్థిరరాశి Dfg_శీర్షం& vtx) స్థిరరాశి {
        // If it only reads non-cyclic vertices, it has also become acyclic
        తిరిగి vtx.foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం& src) {  //
            తిరిగి m_vtx2Scc.at(&src);
        });
    }

    తార్కిక stillCyclicBwd(స్థిరరాశి Dfg_శీర్షం& vtx) స్థిరరాశి {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        తిరిగి vtx.foreachSink([&](స్థిరరాశి Dfg_శీర్షం& dst) {  //
            తిరిగి m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    శూన్య updateAcyclic(స్థిరరాశి Dfg_శీర్షం& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](స్థిరరాశి Dfg_శీర్షం& dst) {
            // Sink was known to be acyclic, stop
            ఒకవేళ (!m_vtx2Scc.at(&dst)) తిరిగి అబద్ధం;
            ఒకవేళ (!stillCyclicFwd(dst)) updateAcyclic(dst);
            తిరిగి అబద్ధం;
        });
        // Propagate the update through the graph backward
        vtx.foreach_ఆధారం([&](స్థిరరాశి Dfg_శీర్షం& src) {
            // Source was known to be acyclic, stop
            ఒకవేళ (!m_vtx2Scc.at(&src)) తిరిగి అబద్ధం;
            ఒకవేళ (!stillCyclicBwd(src)) updateAcyclic(src);
            తిరిగి అబద్ధం;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    శూన్య updateReplacement(స్థిరరాశి Dfg_శీర్షం& vtx, Dfg_శీర్షం& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        ఒకవేళ (!get(replacement)) {
            replacement.foreachSink([&](Dfg_శీర్షం& dst) {
                ఒకవేళ (!get(dst)) తిరిగి అబద్ధం;
                ఒకవేళ (!stillCyclicFwd(dst)) updateAcyclic(dst);
                తిరిగి అబద్ధం;
            });
        }
    }

    /*
       Check stored information is consistent with actual SCCs. Note we
       can't detect during updates if an SCC has split into multiple SCCs.
       Consider:
          A -- E -- G
         / \       / \
        B   C     H   I
         \ /       \ /
          D -- F -- J
       If we fixed up E, the original SCC would split into two, (A,B,C,D)
       and (G,H,I,J), but also F would no longer be cyclic. For this reason,
       we can only assert that anything that we think is not cyclic based
       on the SccInfo must indeed be not cyclic, but might still think a
       vertex is cyclic based on the SccInfo but it actually isn't.
    */
    శూన్య validate() స్థిరరాశి {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

తరగతి TraceDriver final : బహిరంగ DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    నిర్మాణం Cache_కీలకం final {
        Dfg_శీర్షం* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_కీలకం() = తొలగించు;
        Cache_కీలకం(Dfg_శీర్షం* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        నిర్మాణం Hash final {
            కొలత_t క్రియాకారకం()(స్థిరరాశి Cache_కీలకం& అంశం_2) స్థిరరాశి {
                // cppcheck-suppress unreadVariable
                V3Hash hash{అంశం_2.m_vtxp};
                hash += అంశం_2.m_lsb;
                hash += అంశం_2.m_msb;
                తిరిగి hash.విలువ();
            }
        };

        నిర్మాణం Equal final {
            తార్కిక క్రియాకారకం()(స్థిరరాశి Cache_కీలకం& a, స్థిరరాశి Cache_కీలకం& b) స్థిరరాశి {
                తిరిగి a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
            }
        };
    };

    // STATE
    Dfg_లేఖాచిత్రం& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    // The strongly connected component we are currently trying to escape
    uint64_t m_component = 0;
    uint32_t m_lsb = 0;  // LSB to extract from the currently visited Vertex
    uint32_t m_msb = 0;  // MSB to extract from the currently visited Vertex
    std::సదిశ<uint32_t> m_idxs;  // Indices to extract from the currently visited Vertex
    std::సదిశ<Dfg_శీర్షం**> m_pendingps;  // Pending pass-through cache entries across trace calls
    // Result of tracing the currently visited Vertex. Use RETURN_RESULT below!
    Dfg_శీర్షం* m_resp = శూన్యసూచిక;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    Dfg_శీర్షం* m_tailp = శూన్యసూచిక;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    Dfg_శీర్షం* m_defaultp = శూన్యసూచిక;
    Dfg_శీర్షం* m_splicep = శూన్యసూచిక;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_కీలకం, Dfg_శీర్షం*, Cache_కీలకం::Hash, Cache_కీలకం::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_కీలకం, Dfg_శీర్షం*, Cache_కీలకం::Hash, Cache_కీలకం::Equal> m_boundaryCache;

#నిర్వచితమైతే VL_DEBUG
    std::ofstream m_పంక్తి_Coverage_File;  // Line coverage file, just for testing
#అయితే_ముగింపు

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    మూస <రకం_పేరు శీర్షం_2>
    శీర్షం_2* make(స్థిరరాశి Dfg_శీర్షం* refp, uint32_t వెడల్పు) {
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం, శీర్షం_2>::విలువ  //
                          && !std::is_పునాది_యొక్క<Dfg_శీర్షం_Var, శీర్షం_2>::విలువ,
                      "Should only make operation vertices and constants");

        ఒకవేళ VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, శీర్షం_2>::విలువ) {
            DfgConst* స్థిరరాశి vtxp = కొత్త DfgConst{m_dfg, refp->fileline(), వెడల్పు, 0};
            m_sccInfo.కూడు(*vtxp, 0);
            తిరిగి reinterpret_cast<శీర్షం_2*>(vtxp);
        } లేకపోతే {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            ఉపయోగించి Vtx = రకం_పేరు std::conditional<std::is_same<DfgConst, శీర్షం_2>::విలువ, DfgSel,
                                                  శీర్షం_2>::type;
            Vtx* స్థిరరాశి vtxp = కొత్త Vtx{m_dfg, refp->fileline(), Dfg_దత్తాంశం_Type::packed(వెడల్పు)};
            m_sccInfo.కూడు(*vtxp, 0);
            తిరిగి reinterpret_cast<శీర్షం_2*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    Dfg_శీర్షం_Var* createTmp(స్థిరరాశి అక్షరం* ఉపసర్గ, Dfg_శీర్షం* vtxp) {
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
        Dfg_శీర్షం::ScopeCache scopeCache;
        AstScope* స్థిరరాశి scopep = vtxp->scopep(scopeCache);
        Dfg_శీర్షం_Var* స్థిరరాశి varp = m_dfg.makeNewVar(flp, ఉపసర్గ, vtxp->dtype(), scopep);
        varp->vscp()->varp()->is_అంతర్గత(నిజం);
        varp->tmpForp(varp->vscp());
        m_sccInfo.కూడు(*varp, 0);
        తిరిగి varp;
    }

    // Trace drivers of the given packed vertex, at the given bit range.
    Dfg_శీర్షం* trace(Dfg_శీర్షం* vtxp, uint32_t msb, uint32_t lsb) {
        // Many vertices just pass the traced bit range through to one of their sources
        // (see RETURN_RESULT_TAIL). Chains of such vertices can be tens of thousands of
        // vertices long in large designs (e.g.: a wide concatenation), so resolve them
        // in this loop instead of by recursion, which would overflow the C++ stack.
        // 'm_pendingps' holds the cache entries of the vertices on the current chain. They
        // all yield the same result, which is filled in when the chain is resolved.
        // Note: references into 'm_cache' are stable across the insertions below.
        Dfg_శీర్షం* స్థిరరాశి ప్రారంభం_Vtxp = vtxp;
        స్థిరరాశి uint32_t ప్రారంభం_Msb = msb;
        స్థిరరాశి uint32_t ప్రారంభం_Lsb = lsb;
        స్థిరరాశి కొలత_t pendingBegin = m_pendingps.కొలత();
        Dfg_శీర్షం* resp = శూన్యసూచిక;

        అయితే (నిజం) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->కొలత() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            ఒకవేళ (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                Dfg_శీర్షం*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(శూన్యసూచిక))
                          .మొదటి->రెండవ;
                ఒకవేళ (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    ఒకవేళ (Dfg_శీర్షం_Splice* స్థిరరాశి splicep = respr->cast<Dfg_శీర్షం_Splice>()) {
                        Dfg_శీర్షం_Var* స్థిరరాశి tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    ఒకవేళ (msb != respr->వెడల్పు() - 1 || lsb != 0) {
                        DfgSel* స్థిరరాశి selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                ఆపు;
            }

            // Normal cache for intra-component traces
            Dfg_శీర్షం*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(శూన్యసూచిక))
                                    .మొదటి->రెండవ;

            // Trace the vertex
            ఒకవేళ (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } లేకపోతే {
                // Otherwise visit the vertex to trace it
                VL_RESTORER(m_msb);
                VL_RESTORER(m_lsb);
                VL_RESTORER_CLEAR(m_idxs);
                VL_RESTORER(m_resp);
                VL_RESTORER(m_tailp);
                m_msb = msb;
                m_lsb = lsb;
                m_resp = శూన్యసూచిక;
                m_tailp = శూన్యసూచిక;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != శూన్యసూచిక) != (m_tailp != శూన్యసూచిక), vtxp,
                            "Expected exactly one trace result");
                ఒకవేళ (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.నెట్టు_వెనుక(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    కొనసాగు;
                }
                respr = m_resp;
            }
            resp = respr;
            ఆపు;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, ప్రారంభం_Vtxp, "Tracing driver failed for " << ప్రారంభం_Vtxp->typeName());
        UASSERT_OBJ(resp->వెడల్పు() == (ప్రారంభం_Msb - ప్రారంభం_Lsb + 1), ప్రారంభం_Vtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        కొరకు (కొలత_t i = pendingBegin; i < m_pendingps.కొలత(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        తిరిగి resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    Dfg_శీర్షం* traceSameIdx(Dfg_శీర్షం* స్థిరరాశి vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.ఖాళీలు(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = శూన్యసూచిక;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->వెడల్పు() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        తిరిగి m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    Dfg_శీర్షం* trace_నెట్టు_Idx(Dfg_శీర్షం* స్థిరరాశి vtxp, స్థిరరాశి uint32_t idx) {
        స్థిరరాశి కొలత_t nIdxs = m_idxs.కొలత();
        m_idxs.నెట్టు_వెనుక(idx);
        Dfg_శీర్షం* స్థిరరాశి resp = traceSameIdx(vtxp);
        m_idxs.తీయి_వెనుక();
        UASSERT_OBJ(m_idxs.కొలత() == nIdxs, vtxp, "Index stack size mismatch");
        తిరిగి resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    Dfg_శీర్షం* trace_తీయి_Idx(Dfg_శీర్షం* స్థిరరాశి vtxp) {
        UASSERT_OBJ(!m_idxs.ఖాళీలు(), vtxp, "Should have a pending index");
        స్థిరరాశి కొలత_t nIdxs = m_idxs.కొలత();
        స్థిరరాశి uint32_t idx = m_idxs.వెనుక();
        m_idxs.తీయి_వెనుక();
        Dfg_శీర్షం* స్థిరరాశి resp = m_idxs.ఖాళీలు() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.నెట్టు_వెనుక(idx);
        UASSERT_OBJ(m_idxs.కొలత() == nIdxs, vtxp, "Index stack size mismatch");
        తిరిగి resp;
    }

    మూస <రకం_పేరు శీర్షం_2>
    శీర్షం_2* trace_Bitwise_ద్విమాన(శీర్షం_2* vtxp) {
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం_ద్విమాన, శీర్షం_2>::విలువ,
                      "Should only call on DfgVertexBinary");
        శీర్షం_2* స్థిరరాశి resp = make<శీర్షం_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        తిరిగి resp;
    }

    మూస <రకం_పేరు శీర్షం_2>
    Dfg_శీర్షం* trace_కూడు_Sub(శీర్షం_2* vtxp) {
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం_ద్విమాన, శీర్షం_2>::విలువ,
                      "Should only call on DfgVertexBinary");
        శీర్షం_2* స్థిరరాశి opp = make<శీర్షం_2>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* స్థిరరాశి selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        తిరిగి selp;
    }

    మూస <రకం_పేరు శీర్షం_2>
    శీర్షం_2* traceReduction(శీర్షం_2* vtxp) {
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం_Unary, శీర్షం_2>::విలువ,
                      "Should only call on DfgVertexUnary");
        శీర్షం_2* స్థిరరాశి resp = make<శీర్షం_2>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->వెడల్పు() - 1, 0));
        తిరిగి resp;
    }

    మూస <రకం_పేరు శీర్షం_2>
    శీర్షం_2* traceCmp(శీర్షం_2* vtxp) {
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం_ద్విమాన, శీర్షం_2>::విలువ,
                      "Should only call on DfgVertexBinary");
        శీర్షం_2* స్థిరరాశి resp = make<శీర్షం_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->వెడల్పు() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->వెడల్పు() - 1, 0));
        తిరిగి resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#నిర్వచితమైతే VL_DEBUG
#నిర్వచించు TRACE_ఫలితం_COVER() \
    చేయి { \
        ఒకవేళ (VL_UNLIKELY(m_పంక్తి_Coverage_File.is_open())) m_పంక్తి_Coverage_File << __LINE__ << '\n'; \
    } అయితే (అబద్ధం)
#లేకపోతే
#నిర్వచించు TRACE_ఫలితం_COVER() \
    చేయి { \
    } అయితే (అబద్ధం)
#అయితే_ముగింపు

#నిర్వచించు RETURN_ఫలితం(vtxp) \
    చేయి { \
        m_resp = (vtxp); \
        TRACE_ఫలితం_COVER(); \
        తిరిగి; \
    } అయితే (అబద్ధం)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#నిర్వచించు RETURN_ఫలితం_తోక(srcp, smsb, slsb) \
    చేయి { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        Dfg_శీర్షం* స్థిరరాశి tailp = (srcp); \
        స్థిరరాశి uint32_t తోక_Msb = (smsb); \
        స్థిరరాశి uint32_t తోక_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = తోక_Msb; \
        m_lsb = తోక_Lsb; \
        TRACE_ఫలితం_COVER(); \
        తిరిగి; \
    } అయితే (అబద్ధం)

    // VISITORS
    శూన్య సందర్శించు(Dfg_శీర్షం* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    శూన్య సందర్శించు(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        Dfg_శీర్షం* defaultp = m_defaultp;
        m_defaultp = శూన్యసూచిక;
        m_splicep = శూన్యసూచిక;

        నిర్మాణం Driver final {
            Dfg_శీర్షం* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = తొలగించు;
            Driver(Dfg_శీర్షం* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::సదిశ<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        తార్కిక try_సంపూర్ణ_Default = defaultp;
        Dfg_శీర్షం* coverp = శూన్యసూచిక;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](Dfg_శీర్షం& src, uint32_t lsb) {
            స్థిరరాశి uint32_t msb = lsb + src.వెడల్పు() - 1;
            drivers.emplace_వెనుక(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            ఒకవేళ (m_msb >= lsb && msb >= m_lsb) try_సంపూర్ణ_Default = అబద్ధం;
            // If it does not cover the whole searched bit range, move on
            ఒకవేళ (m_lsb < lsb || msb < m_msb) తిరిగి అబద్ధం;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            తిరిగి నిజం;
        });

        // Trace the driver covering the whole searched range, if there is one
        ఒకవేళ (coverp) RETURN_ఫలితం_తోక(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        ఒకవేళ (try_సంపూర్ణ_Default) RETURN_ఫలితం_తోక(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::క్రమీకరించు(drivers.begin(), drivers.చివర(), [](స్థిరరాశి Driver& a, స్థిరరాశి Driver& b) {  //
            తిరిగి a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::సదిశ<Dfg_శీర్షం*> termps;
        uint32_t lsb = m_lsb;
        కొరకు (స్థిరరాశి Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            ఒకవేళ (lsb > driver.m_msb) కొనసాగు;
            // Driver is above the searched MSB, done
            ఒకవేళ (driver.m_lsb > m_msb) ఆపు;
            // Gap below this driver, trace default to fill it
            ఒకవేళ (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_వెనుక(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            స్థిరరాశి uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_వెనుక(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        ఒకవేళ (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_వెనుక(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.కొలత() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        Dfg_శీర్షం* resp = termps.front();
        కొరకు (కొలత_t i = 1; i < termps.కొలత(); ++i) {
            Dfg_శీర్షం* స్థిరరాశి termp = termps[i];
            DfgConcat* స్థిరరాశి catp = make<DfgConcat>(termp, resp->వెడల్పు() + termp->వెడల్పు());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_ఫలితం(resp);
    }

    శూన్య సందర్శించు(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        Dfg_శీర్షం* స్థిరరాశి defaultp = m_defaultp;
        m_defaultp = శూన్యసూచిక;
        m_splicep = శూన్యసూచిక;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        స్థిరరాశి uint32_t idx = m_idxs.వెనుక();
        ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి driverp = vtxp->driverAt(idx)) {
            Dfg_శీర్షం* స్థిరరాశి srcp = driverp->as<DfgUnitArray>()->srcp();
            ఒకవేళ (srcp->is<Dfg_శీర్షం_Splice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_ఫలితం(trace_తీయి_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_ఫలితం(traceSameIdx(defaultp));
    }

    శూన్య సందర్శించు(Dfg_శీర్షం_Var* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        Dfg_శీర్షం* స్థిరరాశి defaultp = vtxp->defaultp();
        Dfg_శీర్షం* స్థిరరాశి drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        ఒకవేళ (srcp && srcp->is<Dfg_శీర్షం_Splice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_ఫలితం(m_idxs.ఖాళీలు() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    శూన్య సందర్శించు(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_ఫలితం(trace_నెట్టు_Idx(vtxp->fromp(), idxp->గమ్యం_U32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.ఖాళీలు(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* స్థిరరాశి resp = make<DfgArraySel>(vtxp, vtxp->వెడల్పు());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->వెడల్పు() - 1, 0));
        DfgSel* స్థిరరాశి selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_ఫలితం(selp);
    }

    శూన్య సందర్శించు(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.వెనుక() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_ఫలితం(trace_తీయి_Idx(vtxp->srcp()));
    }

    శూన్య సందర్శించు(DfgConcat* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        స్థిరరాశి uint32_t r_వెడల్పు = rhsp->వెడల్పు();
        // If the traced bits are wholly in the RHS
        ఒకవేళ (r_వెడల్పు > m_msb) RETURN_ఫలితం_తోక(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        ఒకవేళ (m_lsb >= r_వెడల్పు) RETURN_ఫలితం_తోక(lhsp, m_msb - r_వెడల్పు, m_lsb - r_వెడల్పు);
        // The traced bit spans both sides, trace both
        DfgConcat* స్థిరరాశి resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, r_వెడల్పు - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - r_వెడల్పు, 0));
        RETURN_ఫలితం(resp);
    }

    శూన్య సందర్శించు(DfgRep* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        స్థిరరాశి uint32_t s_వెడల్పు = srcp->వెడల్పు();
        // If we need more bits than the source, then we need the whole source
        ఒకవేళ (m_msb - m_lsb + 1 > s_వెడల్పు) {
            DfgRep* స్థిరరాశి repp = make<DfgRep>(vtxp, vtxp->వెడల్పు());
            repp->srcp(trace(srcp, s_వెడల్పు - 1, 0));
            DfgSel* స్థిరరాశి resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_ఫలితం(resp);
        }
        // If the requested bits are within the same repliacted word
        ఒకవేళ (m_msb / s_వెడల్పు == m_lsb / s_వెడల్పు) {
            RETURN_ఫలితం_తోక(srcp, m_msb % s_వెడల్పు, m_lsb % s_వెడల్పు);
        }
        // The requested bits span two replicated words
        DfgConcat* స్థిరరాశి resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, s_వెడల్పు - 1, m_lsb % s_వెడల్పు));
        resp->lhsp(trace(srcp, m_msb % s_వెడల్పు, 0));
        RETURN_ఫలితం(resp);
    }

    శూన్య సందర్శించు(DfgExtend* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        స్థిరరాశి uint32_t s_వెడల్పు = srcp->వెడల్పు();
        // If the traced bits are wholly in the input
        ఒకవేళ (s_వెడల్పు > m_msb) RETURN_ఫలితం_తోక(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ఒకవేళ (m_lsb >= s_వెడల్పు) RETURN_ఫలితం(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* స్థిరరాశి resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_వెడల్పు - 1, m_lsb));
        RETURN_ఫలితం(resp);
    }

    శూన్య సందర్శించు(DfgExtendS* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        స్థిరరాశి uint32_t s_వెడల్పు = srcp->వెడల్పు();
        // If the traced bits are wholly in the input
        ఒకవేళ (s_వెడల్పు > m_msb) RETURN_ఫలితం_తోక(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ఒకవేళ (m_lsb >= s_వెడల్పు) {
            ఒకవేళ (m_msb == m_lsb) RETURN_ఫలితం_తోక(srcp, s_వెడల్పు - 1, s_వెడల్పు - 1);
            DfgExtendS* స్థిరరాశి resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, s_వెడల్పు - 1, s_వెడల్పు - 1));
            RETURN_ఫలితం(resp);
        }
        // The traced bits span both sides
        DfgExtendS* స్థిరరాశి resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_వెడల్పు - 1, m_lsb));
        RETURN_ఫలితం(resp);
    }

    శూన్య సందర్శించు(DfgSel* vtxp) override {
        స్థిరరాశి uint32_t lsb = vtxp->lsb();
        RETURN_ఫలితం_తోక(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    శూన్య సందర్శించు(DfgNot* vtxp) override {
        DfgNot* స్థిరరాశి resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_ఫలితం(resp);
    }

    శూన్య సందర్శించు(DfgAnd* vtxp) override { RETURN_ఫలితం(trace_Bitwise_ద్విమాన(vtxp)); }
    శూన్య సందర్శించు(DfgOr* vtxp) override { RETURN_ఫలితం(trace_Bitwise_ద్విమాన(vtxp)); }
    శూన్య సందర్శించు(DfgXor* vtxp) override { RETURN_ఫలితం(trace_Bitwise_ద్విమాన(vtxp)); }
    శూన్య సందర్శించు(Dfg_కూడు* vtxp) override { RETURN_ఫలితం(trace_కూడు_Sub(vtxp)); }
    శూన్య సందర్శించు(DfgSub* vtxp) override { RETURN_ఫలితం(trace_కూడు_Sub(vtxp)); }
    శూన్య సందర్శించు(DfgRedAnd* vtxp) override { RETURN_ఫలితం(traceReduction(vtxp)); }
    శూన్య సందర్శించు(DfgRedOr* vtxp) override { RETURN_ఫలితం(traceReduction(vtxp)); }
    శూన్య సందర్శించు(DfgRedXor* vtxp) override { RETURN_ఫలితం(traceReduction(vtxp)); }
    శూన్య సందర్శించు(DfgEq* vtxp) override { RETURN_ఫలితం(traceCmp(vtxp)); }
    శూన్య సందర్శించు(DfgNeq* vtxp) override { RETURN_ఫలితం(traceCmp(vtxp)); }
    శూన్య సందర్శించు(DfgLt* vtxp) override { RETURN_ఫలితం(traceCmp(vtxp)); }
    శూన్య సందర్శించు(DfgLte* vtxp) override { RETURN_ఫలితం(traceCmp(vtxp)); }
    శూన్య సందర్శించు(DfgGt* vtxp) override { RETURN_ఫలితం(traceCmp(vtxp)); }
    శూన్య సందర్శించు(DfgGte* vtxp) override { RETURN_ఫలితం(traceCmp(vtxp)); }

    శూన్య సందర్శించు(Dfg_జరుపు_RS* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            స్థిరరాశి uint32_t జరుపు_Amnt = rConstp->గమ్యం_U32();
            // Width of lower half of result
            స్థిరరాశి uint32_t క్రింది_వెడల్పు = జరుపు_Amnt > vtxp->వెడల్పు() ? 0 : vtxp->వెడల్పు() - జరుపు_Amnt;
            // If the traced bits are wholly in the input
            ఒకవేళ (క్రింది_వెడల్పు > m_msb) {
                RETURN_ఫలితం_తోక(lhsp, m_msb + జరుపు_Amnt, m_lsb + జరుపు_Amnt);
            }
            // If the traced bits are wholly in the extension
            ఒకవేళ (m_lsb >= క్రింది_వెడల్పు) {
                DfgExtendS* స్థిరరాశి resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->వెడల్పు() - 1, lhsp->వెడల్పు() - 1));
                RETURN_ఫలితం(resp);
            }
            // The traced bits span both sides
            DfgExtendS* స్థిరరాశి resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, క్రింది_వెడల్పు - 1 + జరుపు_Amnt, m_lsb + జరుపు_Amnt));
            RETURN_ఫలితం(resp);
        }

        Dfg_జరుపు_RS* స్థిరరాశి shiftrsp = make<Dfg_జరుపు_RS>(vtxp, vtxp->lhsp()->వెడల్పు() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->వెడల్పు() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->వెడల్పు() - 1, m_lsb));
        ఒకవేళ (m_msb == vtxp->lhsp()->వెడల్పు() - 1) RETURN_ఫలితం(shiftrsp);
        DfgSel* స్థిరరాశి selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_ఫలితం(selp);
    }

    శూన్య సందర్శించు(Dfg_జరుపు_R* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            స్థిరరాశి uint32_t జరుపు_Amnt = rConstp->గమ్యం_U32();
            // Width of lower half of result
            స్థిరరాశి uint32_t క్రింది_వెడల్పు = జరుపు_Amnt > vtxp->వెడల్పు() ? 0 : vtxp->వెడల్పు() - జరుపు_Amnt;
            // If the traced bits are wholly in the input
            ఒకవేళ (క్రింది_వెడల్పు > m_msb) {
                RETURN_ఫలితం_తోక(lhsp, m_msb + జరుపు_Amnt, m_lsb + జరుపు_Amnt);
            }
            // If the traced bits are wholly in the extension
            ఒకవేళ (m_lsb >= క్రింది_వెడల్పు) RETURN_ఫలితం(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* స్థిరరాశి resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, క్రింది_వెడల్పు - 1 + జరుపు_Amnt, m_lsb + జరుపు_Amnt));
            RETURN_ఫలితం(resp);
        }

        Dfg_జరుపు_R* స్థిరరాశి shiftrp = make<Dfg_జరుపు_R>(vtxp, vtxp->lhsp()->వెడల్పు() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->వెడల్పు() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->వెడల్పు() - 1, m_lsb));
        ఒకవేళ (m_msb == vtxp->lhsp()->వెడల్పు() - 1) RETURN_ఫలితం(shiftrp);
        DfgSel* స్థిరరాశి selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_ఫలితం(selp);
    }

    శూన్య సందర్శించు(Dfg_జరుపు_L* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            స్థిరరాశి uint32_t జరుపు_Amnt = rConstp->గమ్యం_U32();
            // Width of lower half of result
            స్థిరరాశి uint32_t క్రింది_వెడల్పు = జరుపు_Amnt > vtxp->వెడల్పు() ? vtxp->వెడల్పు() : జరుపు_Amnt;
            // If the traced bits are wholly in the input
            ఒకవేళ (m_lsb >= క్రింది_వెడల్పు) {
                RETURN_ఫలితం_తోక(lhsp, m_msb - జరుపు_Amnt, m_lsb - జరుపు_Amnt);
            }
            // If the traced bits are wholly in the extension
            ఒకవేళ (క్రింది_వెడల్పు > m_msb) RETURN_ఫలితం(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* స్థిరరాశి resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->వెడల్పు() - (m_msb - క్రింది_వెడల్పు + 1)));
            resp->lhsp(trace(lhsp, m_msb - జరుపు_Amnt, క్రింది_వెడల్పు - జరుపు_Amnt));
            RETURN_ఫలితం(resp);
        }

        Dfg_జరుపు_L* స్థిరరాశి shiftlp = make<Dfg_జరుపు_L>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->వెడల్పు() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        ఒకవేళ (m_lsb == 0) RETURN_ఫలితం(shiftlp);
        DfgSel* స్థిరరాశి selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_ఫలితం(selp);
    }

    శూన్య సందర్శించు(DfgCond* vtxp) override {
        DfgCond* స్థిరరాశి resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->వెడల్పు() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_ఫలితం(resp);
    }

    శూన్య సందర్శించు(Dfg_సరిపోలిక_Masked* vtxp) override {
        Dfg_సరిపోలిక_Masked* స్థిరరాశి resp = make<Dfg_సరిపోలిక_Masked>(vtxp, vtxp->వెడల్పు());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->వెడల్పు() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->వెడల్పు() - 1, 0));
        DfgSel* స్థిరరాశి selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_ఫలితం(selp);
    }

#నిర్వచనం_రద్దు RETURN_ఫలితం
#నిర్వచనం_రద్దు TRACE_ఫలితం_COVER
#నిర్వచనం_రద్దు RETURN_ఫలితం_తోక

బహిరంగ:
    // CONSTRUCTOR
    TraceDriver(Dfg_లేఖాచిత్రం& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#నిర్వచితమైతే VL_DEBUG
        ఒకవేళ (v3Global.opt.debugCheck()) {
            m_పంక్తి_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.ఉపసర్గ()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_పునాది::బహిర్గతం | std::ios_పునాది::app);
        }
#అయితే_ముగింపు
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    Dfg_శీర్షం* apply(Dfg_శీర్షం& vtx, uint32_t lsb, uint32_t వెడల్పు) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        తిరిగి trace(&vtx, lsb + వెడల్పు - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
తరగతి బిట్టు_ముసుగు final {
    // TYPES
    స్థిరరాశి Dfg_దత్తాంశం_Type& m_dtype;
    సంఘం {
        V3_సంఖ్య m_అంకె;  // The bits, if packed
        std::సదిశ<బిట్టు_ముసుగు> m_sub;  // The per element masks, if aggregate
    };

బహిరంగ:
    // CONSTRUCTOR
    బిట్టు_ముసుగు(File_పంక్తి* flp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        ఒకవేళ (m_dtype.isPacked()) {
            కొత్త (&m_అంకె) V3_సంఖ్య{flp, static_cast<పూర్ణాంకం>(m_dtype.కొలత()), 0};
        } లేకపోతే {
            కొత్త (&m_sub) std::సదిశ<బిట్టు_ముసుగు>{};
            m_sub.reserve(m_dtype.కొలత());
            కొరకు (uint32_t i = 0; i < m_dtype.కొలత(); ++i)
                m_sub.emplace_వెనుక(flp, m_dtype.elemDtype());
        }
    }
    బిట్టు_ముసుగు(స్థిరరాశి Dfg_శీర్షం& vtx)
        : బిట్టు_ముసుగు{vtx.fileline(), vtx.dtype()} {}
    బిట్టు_ముసుగు(స్థిరరాశి బిట్టు_ముసుగు& other)
        : m_dtype{other.m_dtype} {
        ఒకవేళ (m_dtype.isPacked()) {
            కొత్త (&m_అంకె) V3_సంఖ్య{other.m_అంకె};
        } లేకపోతే {
            కొత్త (&m_sub) std::సదిశ<బిట్టు_ముసుగు>{other.m_sub};
        }
    }
    బిట్టు_ముసుగు& క్రియాకారకం=(స్థిరరాశి బిట్టు_ముసుగు& other) {
        ఒకవేళ (ఇది != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            ఒకవేళ (other.m_dtype.isPacked()) {
                m_అంకె = other.m_అంకె;
            } లేకపోతే {
                m_sub = other.m_sub;
            }
        }
        తిరిగి *ఇది;
    }
    ~బిట్టు_ముసుగు() {
        ఒకవేళ (m_dtype.isPacked()) {
            m_అంకె.~V3_సంఖ్య();
        } లేకపోతే {
            m_sub.~సదిశ<బిట్టు_ముసుగు>();
        }
    }

    // METHODS
    V3_సంఖ్య& అంకె() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        తిరిగి m_అంకె;
    }
    స్థిరరాశి V3_సంఖ్య& అంకె() స్థిరరాశి {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        తిరిగి m_అంకె;
    }
    std::సదిశ<బిట్టు_ముసుగు>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        తిరిగి m_sub;
    }

    తార్కిక isZero() స్థిరరాశి {
        ఒకవేళ (m_dtype.isPacked()) తిరిగి m_అంకె.isEqZero();

        కొరకు (స్థిరరాశి బిట్టు_ముసుగు& sub : m_sub) {
            ఒకవేళ (!sub.isZero()) తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }
    తార్కిక isOnes() స్థిరరాశి {
        ఒకవేళ (m_dtype.isPacked()) తిరిగి m_అంకె.isEqAllOnes();

        కొరకు (స్థిరరాశి బిట్టు_ముసుగు& sub : m_sub) {
            ఒకవేళ (!sub.isOnes()) తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }
    శూన్య సమితి_Zero() {
        ఒకవేళ (m_dtype.isPacked()) {
            m_అంకె.సమితి_All_Bits0();
        } లేకపోతే {
            కొరకు (బిట్టు_ముసుగు& sub : m_sub) sub.సమితి_Zero();
        }
    }
    శూన్య సమితి_Ones() {
        ఒకవేళ (m_dtype.isPacked()) {
            m_అంకె.సమితి_All_Bits1();
        } లేకపోతే {
            కొరకు (బిట్టు_ముసుగు& sub : m_sub) sub.సమితి_Ones();
        }
    }

    తార్కిక క్రియాకారకం==(స్థిరరాశి బిట్టు_ముసుగు& other) స్థిరరాశి {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        ఒకవేళ (m_dtype.isPacked()) తిరిగి m_అంకె.isCaseEq(other.m_అంకె);
        తిరిగి m_sub == other.m_sub;
    }
    తార్కిక క్రియాకారకం!=(స్థిరరాశి బిట్టు_ముసుగు& other) స్థిరరాశి { తిరిగి !(*ఇది == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    తార్కిక is_Contraction_యొక్క(స్థిరరాశి బిట్టు_ముసుగు& that) స్థిరరాశి {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        ఒకవేళ (m_dtype.isPacked()) {
            స్థిరరాశి కొలత_t పదాలు = VL_పదాలు_I(m_dtype.కొలత());
            కొరకు (కొలత_t i = 0; i < పదాలు; ++i) {
                ఒకవేళ (~m_అంకె.edata_మాట(i) & that.m_అంకె.edata_మాట(i)) తిరిగి నిజం;
            }
            తిరిగి అబద్ధం;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        కొరకు (కొలత_t i = 0; i < m_sub.కొలత(); ++i) {
            ఒకవేళ (m_sub[i].is_Contraction_యొక్క(that.m_sub[i])) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    std::string గమ్యం_String() స్థిరరాశి {
        ఒకవేళ (m_dtype.isPacked()) తిరిగి "0x" + m_అంకె.displayed(m_అంకె.fileline(), "%x");
        std::string ఫలితం;
        ఫలితం += "{";
        కొరకు (స్థిరరాశి బిట్టు_ముసుగు& sub : m_sub) {
            ఒకవేళ (&sub != &m_sub.front()) ఫలితం += ", ";
            ఫలితం += sub.గమ్యం_String();
        }
        ఫలితం += "}";
        తిరిగి ఫలితం;
    }
};

తరగతి Independent_బిట్లు final : బహిరంగ DfgVisitor {
    // TYPES
    నిర్మాణం శీర్షం_State final {
        కొలత_t m_rpo_సంఖ్య = 0;  // Reverse Postorder Number of vertex
        తార్కిక m_isOnWorkList = అబద్ధం;  // Whether the vertex is on the work list
    };

    // STATE
    స్థిరరాశి SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<స్థిరరాశి Dfg_శీర్షం*, బిట్టు_ముసుగు> m_vtxp2_ముసుగు;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::ప్రాధాన్యత_వరుస<కొలత_t, std::సదిశ<కొలత_t>, std::greater<కొలత_t>> m_work_వరుస;
    std::unordered_map<స్థిరరాశి Dfg_శీర్షం*, శీర్షం_State> m_vtxp2State;  // State of each vertex

#నిర్వచితమైతే VL_DEBUG
    std::ofstream m_పంక్తి_Coverage_File;  // Line coverage file, just for testing
#అయితే_ముగింపు

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    బిట్టు_ముసుగు& ముసుగు(స్థిరరాశి Dfg_శీర్షం& vtx) {
        // Look up (or create) mask for 'vtx'
        తిరిగి m_vtxp2_ముసుగు
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .మొదటి->రెండవ;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#నిర్వచితమైతే VL_DEBUG
#నిర్వచించు ముసుగు_2(vtxp) \
    ([ఇది](స్థిరరాశి Dfg_శీర్షం& vtx) -> బిట్టు_ముసుగు& { \
        ఒకవేళ (VL_UNLIKELY(m_పంక్తి_Coverage_File.is_open())) m_పంక్తి_Coverage_File << __LINE__ << '\n'; \
        తిరిగి ముసుగు(vtx); \
    }(*vtxp))
#లేకపోతే
#నిర్వచించు ముసుగు_2(vtxp) ముసుగు(*vtxp)
#అయితే_ముగింపు

    // Clear all bits at or below the most significant clear bit
    శూన్య floodTowardsLsb(V3_సంఖ్య& అంకె) {
        కొరకు (పూర్ణాంకం i = అంకె.వెడల్పు() - 1; i >= 0; --i) {
            ఒకవేళ (అంకె.బిట్టు_Is1(i)) కొనసాగు;
            అంకె.op_సమితి_Range(0, i + 1, '0');
            ఆపు;
        }
    }

    // Clear all bits at or above the least significant clear bit
    శూన్య floodTowardsMsb(V3_సంఖ్య& అంకె) {
        కొరకు (పూర్ణాంకం i = 0; i < అంకె.వెడల్పు(); ++i) {
            ఒకవేళ (అంకె.బిట్టు_Is1(i)) కొనసాగు;
            అంకె.op_సమితి_Range(i, అంకె.వెడల్పు() - i, '0');
            ఆపు;
        }
    }

    శూన్య propagateFromDriver(బిట్టు_ముసుగు& m, స్థిరరాశి Dfg_శీర్షం* srcp) {
        // If there is no driver, we are done
        ఒకవేళ (!srcp) తిరిగి;
        // If it is driven by a splice, we need to combine the masks of the drivers
        ఒకవేళ (స్థిరరాశి DfgSplicePacked* స్థిరరాశి splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](స్థిరరాశి Dfg_శీర్షం& src, uint32_t lo) {
                m.అంకె().opSelInto(ముసుగు_2(&src).అంకె(), lo, src.వెడల్పు());
                తిరిగి అబద్ధం;
            });
            తిరిగి;
        }
        ఒకవేళ (స్థిరరాశి DfgSpliceArray* స్థిరరాశి splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](స్థిరరాశి Dfg_శీర్షం& src, uint32_t lo) {
                ఒకవేళ (స్థిరరాశి DfgUnitArray* స్థిరరాశి uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = ముసుగు_2(uap->srcp());
                } లేకపోతే {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                తిరిగి అబద్ధం;
            });
            తిరిగి;
        }
        // Otherwise, we just use the mask of the single driver
        m = ముసుగు_2(srcp);
    }

    // VISITORS
    శూన్య సందర్శించు(Dfg_శీర్షం* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    శూన్య సందర్శించు(Dfg_శీర్షం_Var* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        ఒకవేళ (vtxp->isVolatile()) తిరిగి;

        బిట్టు_ముసుగు& m = ముసుగు_2(vtxp);
        Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        Dfg_శీర్షం* స్థిరరాశి defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        ఒకవేళ (defaultp) m = ముసుగు_2(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    శూన్య సందర్శించు(Dfg_శీర్షం_Splice* vtxp) override {
        propagateFromDriver(ముసుగు_2(vtxp), vtxp);  // Needed to continue traversal
    }

    శూన్య సందర్శించు(DfgUnitArray* vtxp) override { ముసుగు_2(vtxp).sub().at(0) = ముసుగు_2(vtxp->srcp()); }

    శూన్య సందర్శించు(DfgArraySel* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి idxp = vtxp->bitp()->cast<DfgConst>()) {
            ముసుగు_2(vtxp) = ముసుగు_2(vtxp->fromp()).sub().at(idxp->గమ్యం_కొలత_T());
            తిరిగి;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        ఒకవేళ (ముసుగు_2(vtxp->bitp()).isOnes() && ముసుగు_2(fromp).isOnes()) ముసుగు_2(vtxp).సమితి_Ones();
    }

    శూన్య సందర్శించు(DfgConcat* vtxp) override {
        స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
        m.opSelInto(ముసుగు_2(rhsp).అంకె(), 0, rhsp->వెడల్పు());
        m.opSelInto(ముసుగు_2(lhsp).అంకె(), rhsp->వెడల్పు(), lhsp->వెడల్పు());
    }

    శూన్య సందర్శించు(DfgRep* vtxp) override {
        స్థిరరాశి uint32_t లెక్క = vtxp->లెక్క();
        స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        స్థిరరాశి uint32_t s_వెడల్పు = srcp->వెడల్పు();
        V3_సంఖ్య& v_ముసుగు = ముసుగు_2(vtxp).అంకె();
        V3_సంఖ్య& s_ముసుగు = ముసుగు_2(srcp).అంకె();
        కొరకు (uint32_t i = 0; i < లెక్క; ++i) v_ముసుగు.opSelInto(s_ముసుగు, i * s_వెడల్పు, s_వెడల్పు);
    }

    శూన్య సందర్శించు(DfgSel* vtxp) override {
        స్థిరరాశి uint32_t lsb = vtxp->lsb();
        స్థిరరాశి uint32_t msb = lsb + vtxp->వెడల్పు() - 1;
        ముసుగు_2(vtxp).అంకె().opSel(ముసుగు_2(vtxp->fromp()).అంకె(), msb, lsb);
    }

    శూన్య సందర్శించు(DfgExtend* vtxp) override {
        స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        స్థిరరాశి uint32_t s_వెడల్పు = srcp->వెడల్పు();
        V3_సంఖ్య& s = ముసుగు_2(srcp).అంకె();
        V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
        m.opSelInto(s, 0, s_వెడల్పు);
        m.op_సమితి_Range(s_వెడల్పు, vtxp->వెడల్పు() - s_వెడల్పు, '1');
    }
    శూన్య సందర్శించు(DfgExtendS* vtxp) override {
        స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        స్థిరరాశి uint32_t s_వెడల్పు = srcp->వెడల్పు();
        V3_సంఖ్య& s = ముసుగు_2(srcp).అంకె();
        V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
        m.opSelInto(s, 0, s_వెడల్పు);
        m.op_సమితి_Range(s_వెడల్పు, vtxp->వెడల్పు() - s_వెడల్పు, s.బిట్టు_Is0(s_వెడల్పు - 1) ? '0' : '1');
    }

    శూన్య సందర్శించు(DfgNot* vtxp) override {  //
        ముసుగు_2(vtxp) = ముసుగు_2(vtxp->srcp());
    }

    శూన్య సందర్శించు(DfgAnd* vtxp) override {  //
        ముసుగు_2(vtxp).అంకె().opAnd(ముసుగు_2(vtxp->lhsp()).అంకె(), ముసుగు_2(vtxp->rhsp()).అంకె());
    }
    శూన్య సందర్శించు(DfgOr* vtxp) override {  //
        ముసుగు_2(vtxp).అంకె().opAnd(ముసుగు_2(vtxp->lhsp()).అంకె(), ముసుగు_2(vtxp->rhsp()).అంకె());
    }
    శూన్య సందర్శించు(DfgXor* vtxp) override {  //
        ముసుగు_2(vtxp).అంకె().opAnd(ముసుగు_2(vtxp->lhsp()).అంకె(), ముసుగు_2(vtxp->rhsp()).అంకె());
    }

    శూన్య సందర్శించు(Dfg_కూడు* vtxp) override {
        V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
        m.opAnd(ముసుగు_2(vtxp->lhsp()).అంకె(), ముసుగు_2(vtxp->rhsp()).అంకె());
        floodTowardsMsb(m);
    }
    శూన్య సందర్శించు(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
        m.opAnd(ముసుగు_2(vtxp->lhsp()).అంకె(), ముసుగు_2(vtxp->rhsp()).అంకె());
        floodTowardsMsb(m);
    }

    శూన్య సందర్శించు(DfgRedAnd* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }
    శూన్య సందర్శించు(DfgRedOr* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }
    శూన్య సందర్శించు(DfgRedXor* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }

    శూన్య సందర్శించు(DfgEq* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes() && ముసుగు_2(vtxp->rhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }
    శూన్య సందర్శించు(DfgNeq* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes() && ముసుగు_2(vtxp->rhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }
    శూన్య సందర్శించు(DfgLt* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes() && ముసుగు_2(vtxp->rhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }
    శూన్య సందర్శించు(DfgLte* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes() && ముసుగు_2(vtxp->rhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }
    శూన్య సందర్శించు(DfgGt* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes() && ముసుగు_2(vtxp->rhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }
    శూన్య సందర్శించు(DfgGte* vtxp) override {
        స్థిరరాశి తార్కిక independent = ముసుగు_2(vtxp->lhsp()).isOnes() && ముసుగు_2(vtxp->rhsp()).isOnes();
        ఒకవేళ (independent) ముసుగు_2(vtxp).సమితి_Ones();
    }

    శూన్య సందర్శించు(Dfg_జరుపు_RS* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        స్థిరరాశి uint32_t వెడల్పు = vtxp->వెడల్పు();

        // Constant shift can be computed precisely
        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = rhsp->cast<DfgConst>()) {
            స్థిరరాశి uint32_t జరుపు_Amount = rConstp->గమ్యం_U32();
            ఒకవేళ (జరుపు_Amount >= వెడల్పు) {
                ఒకవేళ (ముసుగు_2(lhsp).అంకె().బిట్టు_Is1(వెడల్పు - 1)) { ముసుగు_2(vtxp).సమితి_Ones(); }
            } లేకపోతే {
                V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
                m.op_జరుపు_RS(ముసుగు_2(lhsp).అంకె(), rConstp->అంకె(), వెడల్పు);
                m.op_సమితి_Range(వెడల్పు - జరుపు_Amount, జరుపు_Amount, '1');
            }
            తిరిగి;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ఒకవేళ (ముసుగు_2(rhsp).isOnes()) {
            V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
            m = ముసుగు_2(lhsp).అంకె();
            floodTowardsLsb(m);
        }
    }

    శూన్య సందర్శించు(Dfg_జరుపు_R* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        స్థిరరాశి uint32_t వెడల్పు = vtxp->వెడల్పు();

        // Constant shift can be computed precisely
        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = rhsp->cast<DfgConst>()) {
            స్థిరరాశి uint32_t జరుపు_Amount = rConstp->గమ్యం_U32();
            ఒకవేళ (జరుపు_Amount >= వెడల్పు) {
                ముసుగు_2(vtxp).సమితి_Ones();
            } లేకపోతే {
                V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
                m.op_జరుపు_R(ముసుగు_2(lhsp).అంకె(), rConstp->అంకె());
                m.op_సమితి_Range(వెడల్పు - జరుపు_Amount, జరుపు_Amount, '1');
            }
            తిరిగి;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ఒకవేళ (ముసుగు_2(rhsp).isOnes()) {
            V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
            m = ముసుగు_2(lhsp).అంకె();
            floodTowardsLsb(m);
        }
    }

    శూన్య సందర్శించు(Dfg_జరుపు_L* vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        స్థిరరాశి uint32_t వెడల్పు = vtxp->వెడల్పు();

        // Constant shift can be computed precisely
        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = rhsp->cast<DfgConst>()) {
            స్థిరరాశి uint32_t జరుపు_Amount = rConstp->గమ్యం_U32();
            ఒకవేళ (జరుపు_Amount >= వెడల్పు) {
                ముసుగు_2(vtxp).సమితి_Ones();
            } లేకపోతే {
                V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
                m.op_జరుపు_L(ముసుగు_2(lhsp).అంకె(), rConstp->అంకె());
                m.op_సమితి_Range(0, జరుపు_Amount, '1');
            }
            తిరిగి;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        ఒకవేళ (ముసుగు_2(rhsp).isOnes()) {
            V3_సంఖ్య& m = ముసుగు_2(vtxp).అంకె();
            m = ముసుగు_2(lhsp).అంకె();
            floodTowardsMsb(m);
        }
    }

    శూన్య సందర్శించు(DfgCond* vtxp) override {
        ఒకవేళ (ముసుగు_2(vtxp->condp()).isOnes()) {
            ముసుగు_2(vtxp).అంకె().opAnd(ముసుగు_2(vtxp->thenp()).అంకె(), ముసుగు_2(vtxp->elsep()).అంకె());
        }
    }

    శూన్య సందర్శించు(Dfg_సరిపోలిక_Masked* vtxp) override {
        ఒకవేళ (ముసుగు_2(vtxp->lhsp()).isOnes() && ముసుగు_2(vtxp->matchp()).isOnes()) { ముసుగు_2(vtxp).సమితి_Ones(); }
    }

#నిర్వచనం_రద్దు ముసుగు_2

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    శూన్య వరుసలో_చేర్చు_Sinks(Dfg_శీర్షం& vtx) {
        vtx.foreachSink([&](Dfg_శీర్షం& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            ఒకవేళ (!m_sccInfo.get(sink)) తిరిగి అబద్ధం;
            // Otherwise just enqueue it
            శీర్షం_State& state = m_vtxp2State.at(&sink);
            ఒకవేళ (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_work_వరుస.నెట్టు(state.m_rpo_సంఖ్య);
            }
            state.m_isOnWorkList = నిజం;
            తిరిగి అబద్ధం;
        });
    };

    శూన్య dfs_Post_క్రమం(std::unordered_సమితి<Dfg_శీర్షం*>& సందర్శించిన,  //
                      std::సదిశ<Dfg_శీర్షం*>& post_క్రమం_Enumeration,  //
                      Dfg_శీర్షం& vtx) {
        // Mark visited, skip if already visited
        ఒకవేళ (!సందర్శించిన.చొప్పించు(&vtx).రెండవ) తిరిగి;
        // Visit un-visited successors
        vtx.foreachSink([&](Dfg_శీర్షం& snk) {
            dfs_Post_క్రమం(సందర్శించిన, post_క్రమం_Enumeration, snk);
            తిరిగి అబద్ధం;
        });
        // Add to post order enumeration
        post_క్రమం_Enumeration.emplace_వెనుక(&vtx);
    };

    Independent_బిట్లు(Dfg_లేఖాచిత్రం& dfg, స్థిరరాశి SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#నిర్వచితమైతే VL_DEBUG
        ఒకవేళ (v3Global.opt.debugCheck()) {
            m_పంక్తి_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.ఉపసర్గ()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_పునాది::బహిర్గతం | std::ios_పునాది::app);
        }
#అయితే_ముగింపు

        // Compute Reverse-Postorder enumeration of vertices
        std::సదిశ<Dfg_శీర్షం*> rpoEnumeration;
        std::unordered_సమితి<Dfg_శీర్షం*> సందర్శించిన;
        dfg.for_Each_శీర్షం([&](Dfg_శీర్షం& vtx) {
            ఒకవేళ (!vtx.nInputs()) dfs_Post_క్రమం(సందర్శించిన, rpoEnumeration, vtx);
        });
        dfg.for_Each_శీర్షం([&](Dfg_శీర్షం& vtx) {  //
            dfs_Post_క్రమం(సందర్శించిన, rpoEnumeration, vtx);
        });
        std::తిరగేయి(rpoEnumeration.begin(), rpoEnumeration.చివర());
        UASSERT(rpoEnumeration.కొలత() == dfg.కొలత(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        కొరకు (కొలత_t i = 0; i < rpoEnumeration.కొలత(); ++i) {
            శీర్షం_State& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_సంఖ్య = i;
            state.m_isOnWorkList = అబద్ధం;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : rpoEnumeration) {
            ఒకవేళ (m_sccInfo.get(*vtxp)) కొనసాగు;
            ముసుగు(*vtxp).సమితి_Ones();
        }
        కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : rpoEnumeration) {
            ఒకవేళ (!m_sccInfo.get(*vtxp)) కొనసాగు;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << ముసుగు(*vtxp).గమ్యం_String());
            ఒకవేళ (!ముసుగు(*vtxp).isZero()) వరుసలో_చేర్చు_Sinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        అయితే (!m_work_వరుస.ఖాళీలు()) {
            // Grab next item
            Dfg_శీర్షం* స్థిరరాశి currp = rpoEnumeration[m_work_వరుస.శిఖరం()];
            m_work_వరుస.తీయి();
            m_vtxp2State.at(currp).m_isOnWorkList = అబద్ధం;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            స్థిరరాశి బిట్టు_ముసుగు& curr_ముసుగు = ముసుగు(*currp);
            // Remember current mask so we can check if it changed
            స్థిరరాశి బిట్టు_ముసుగు prev_ముసుగు = curr_ముసుగు;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            ఒకవేళ (prev_ముసుగు != curr_ముసుగు) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prev_ముసుగు.గమ్యం_String()
                             << "\n      to: " << curr_ముసుగు.గమ్యం_String());
                వరుసలో_చేర్చు_Sinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                ఒకవేళ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!curr_ముసుగు.is_Contraction_యొక్క(prev_ముసుగు), currp,
                                "Mask should only expand");
                }
            }
        }
    }

బహిరంగ:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    స్థిర std::unordered_map<స్థిరరాశి Dfg_శీర్షం*, బిట్టు_ముసుగు> apply(Dfg_లేఖాచిత్రం& dfg,
                                                               స్థిరరాశి SccInfo& sccInfo) {
        తిరిగి std::move(Independent_బిట్లు{dfg, sccInfo}.m_vtxp2_ముసుగు);
    }
};

తరగతి Fix_పైకి final {
    Dfg_లేఖాచిత్రం& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    స్థిరరాశి std::unordered_map<స్థిరరాశి Dfg_శీర్షం*, బిట్టు_ముసుగు> m_independent_బిట్లు
        = Independent_బిట్లు::apply(m_dfg, m_sccInfo);
    కొలత_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    స్థిర బిట్టు_ముసుగు compute_వాడిన_బిట్లు(Dfg_శీర్షం& vtx) {
        బిట్టు_ముసుగు ఫలితం{vtx};
        vtx.foreachSink([&ఫలితం](Dfg_శీర్షం& sink) {
            // If used via a Sel, mark the selected bits used
            ఒకవేళ (స్థిరరాశి DfgSel* స్థిరరాశి selp = sink.cast<DfgSel>()) {
                ఫలితం.అంకె().op_సమితి_Range(selp->lsb(), selp->వెడల్పు(), '1');
                తిరిగి అబద్ధం;
            }
            // Used without a Sel, so all bits are used
            ఫలితం.సమితి_Ones();
            తిరిగి నిజం;
        });
        తిరిగి ఫలితం;
    }

    స్థిర std::string debugStr(స్థిరరాశి Dfg_శీర్షం& vtx) {  // LCOV_EXCL_START
        ఒకవేళ (స్థిరరాశి DfgArraySel* స్థిరరాశి aselp = vtx.cast<DfgArraySel>()) {
            స్థిరరాశి కొలత_t i = aselp->bitp()->as<DfgConst>()->గమ్యం_కొలత_T();
            తిరిగి debugStr(*aselp->fromp()) + "[" + std::గమ్యం_string(i) + "]";
        }
        ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = vtx.cast<Dfg_శీర్షం_Var>()) {
            తిరిగి varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    శూన్య gatherTerms(std::సదిశ<Dfg_శీర్షం*>& termps, Dfg_శీర్షం& vtx, స్థిరరాశి V3_సంఖ్య& indp_బిట్లు,
                     స్థిరరాశి uint32_t hi, స్థిరరాశి uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        తార్కిక isIndependent = indp_బిట్లు.బిట్టు_Is1(lo);
        uint32_t lsb = lo;
        కొరకు (uint32_t msb = lo; msb <= hi; ++msb) {
            స్థిరరాశి తార్కిక చివర_Range = msb == hi || isIndependent != indp_బిట్లు.బిట్టు_Is1(msb + 1);
            ఒకవేళ (!చివర_Range) కొనసాగు;
            స్థిరరాశి uint32_t వెడల్పు = msb - lsb + 1;
            // Compute the term to use for this range
            ఒకవేళ (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_వెనుక(m_traceDriver.apply(vtx, lsb, వెడల్పు));
            } లేకపోతే {
                // If dependent, fall back on using the part of the variable
                DfgSel* స్థిరరాశి selp = కొత్త DfgSel{m_dfg, vtx.fileline(), Dfg_దత్తాంశం_Type::packed(వెడల్పు)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.కూడు(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_వెనుక(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    శూన్య fix_పైకి_Packed(Dfg_శీర్షం& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        స్థిరరాశి బిట్టు_ముసుగు& indp_బిట్లు = m_independent_బిట్లు.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indp_బిట్లు.గమ్యం_String());
        // Can't do anything if all bits are dependent
        ఒకవేళ (indp_బిట్లు.isZero()) తిరిగి;

        // Figure out which bits of 'vtxp' are used
        స్థిరరాశి బిట్టు_ముసుగు వాడిన_బిట్లు = compute_వాడిన_బిట్లు(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << వాడిన_బిట్లు.గమ్యం_String());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_సంఖ్య వాడిన_And_Independent{vtx.fileline(), static_cast<పూర్ణాంకం>(vtx.వెడల్పు()), 0};
        వాడిన_And_Independent.opAnd(వాడిన_బిట్లు.అంకె(), indp_బిట్లు.అంకె());
        ఒకవేళ (వాడిన_And_Independent.isEqZero()) తిరిగి;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::సదిశ<Dfg_శీర్షం*> termps;

        // Iterate through consecutive used/unused ranges
        File_పంక్తి* స్థిరరాశి flp = vtx.fileline();
        స్థిరరాశి uint32_t వెడల్పు = vtx.వెడల్పు();
        తార్కిక is_వాడిన = వాడిన_బిట్లు.అంకె().బిట్టు_Is1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        కొరకు (uint32_t msb = 0; msb < వెడల్పు; ++msb) {
            స్థిరరాశి తార్కిక చివర_Range = msb == వెడల్పు - 1 || is_వాడిన != వాడిన_బిట్లు.అంకె().బిట్టు_Is1(msb + 1);
            ఒకవేళ (!చివర_Range) కొనసాగు;
            ఒకవేళ (is_వాడిన) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indp_బిట్లు.అంకె(), msb, lsb);
            } లేకపోతే {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* స్థిరరాశి constp = కొత్త DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.కూడు(*constp, 0);
                termps.emplace_వెనుక(constp);
            }
            // Next iteration
            is_వాడిన = !is_వాడిన;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        Dfg_శీర్షం* replacementp = termps.front();
        స్థిరరాశి uint64_t vComp = m_sccInfo.get(vtx);
        కొరకు (కొలత_t i = 1; i < termps.కొలత(); ++i) {
            Dfg_శీర్షం* స్థిరరాశి termp = termps[i];
            స్థిరరాశి uint32_t cat_వెడల్పు = replacementp->వెడల్పు() + termp->వెడల్పు();
            DfgConcat* స్థిరరాశి catp = కొత్త DfgConcat{m_dfg, flp, Dfg_దత్తాంశం_Type::packed(cat_వెడల్పు)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            స్థిరరాశి uint64_t tComp = m_sccInfo.get(*termp);
            స్థిరరాశి uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.కూడు(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        కొరకు (Dfg_శీర్షం* స్థిరరాశి termp : termps) {
            ఒకవేళ (DfgSel* స్థిరరాశి selp = termp->cast<DfgSel>()) {
                ఒకవేళ (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        ఒకవేళ (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    శూన్య ప్రధాన(Dfg_శీర్షం_Var& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        ఒకవేళ (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fix_పైకి_Packed(var);
        } లేకపోతే ఒకవేళ (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            స్థిరరాశి uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](Dfg_శీర్షం& sink) {
                // Ignore if sink is not part of same cycle
                ఒకవేళ (m_sccInfo.get(sink) != component) తిరిగి అబద్ధం;
                // Only handle ArraySels with constant index
                DfgArraySel* స్థిరరాశి aselp = sink.cast<DfgArraySel>();
                ఒకవేళ (!aselp) తిరిగి అబద్ధం;
                ఒకవేళ (!aselp->bitp()->is<DfgConst>()) తిరిగి అబద్ధం;
                // Fix up the word
                fix_పైకి_Packed(*aselp);
                తిరిగి అబద్ధం;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    Fix_పైకి(Dfg_లేఖాచిత్రం& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

బహిరంగ:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    స్థిర కొలత_t apply(Dfg_లేఖాచిత్రం& dfg, SccInfo& sccInfo) {
        ఒకవేళ (!sccInfo.isCyclic()) తిరిగి 0;

        Fix_పైకి fix_పైకి{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        కొరకు (Dfg_శీర్షం_Var& vtx : dfg.varVertices()) {
            ఒకవేళ (!sccInfo.get(vtx)) కొనసాగు;
            fix_పైకి.ప్రధాన(vtx);
            ఒకవేళ (!sccInfo.isCyclic()) ఆపు;
        }
        // Return the number of improvements made
        తిరిగి fix_పైకి.m_nImprovements;
    }
};

శూన్య break_చక్రాలు(Dfg_లేఖాచిత్రం& dfg, V3_Dfg_Break_చక్రాలు_Context& ctx) {
    // Shorthand for dumping graph at given dump level
    స్థిరరాశి స్వయంచాలక dump = [&](పూర్ణాంకం level, స్థిరరాశి Dfg_లేఖాచిత్రం& dfg, స్థిరరాశి std::string& name) {
        ఒకవేళ (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    కొలత_t nImprovements = 0;
    కొలత_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    చేయి {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        స్థిరరాశి కొలత_t nFixed = Fix_పైకి::apply(dfg, sccInfo);
        ఒకవేళ (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        ఒకవేళ (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        ఒకవేళ (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            తిరిగి;
        }
    } అయితే (nImprovements != prevNImprovements);

    // Debug dump
    ఒకవేళ (dumpDfgLevel() >= 9) {
        స్థిరరాశి SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](స్థిరరాశి Dfg_శీర్షం& vtx) {
            తిరిగి sccInfo.get(vtx);  //
        });
    }

    // Accounting
    ఒకవేళ (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } లేకపోతే {
        UINFO(7, "Graph NOT improved");
        ++ctx.m_nUnchanged;
    }

    // Break any remaining cycles by inserting a DfgPrev for variables still in a cycle.
    // Doing this unconditionally is both safe and sufficient:
    //  - Safe: a DfgPrev makes the variable's readers read its stored value instead
    //    of its in-graph driver, while the variable itself is still assigned. This
    //    merely reconstructs the cyclic variable-level dataflow, which the scheduler
    //    will resolve via its settle loop, so the computation is equivalent.
    //  - Sufficient: every cycle must pass through a variable, and in particular
    //    through a non-temporary one (synthesis doesn't create cycles within a single
    //    logic block).
    // Also note that synthesized logic that depends on an in-graph update will read
    // the synthesis temporary, not the real variable (which is what we are cutting),
    // so the graph still holds all updates in a form visible to later optimizers.

    // Gather all non-temporary variables as candidate cut points, prefer those that
    // are kept regardless. Breaking a cycle there is essentially free, as they must be
    // materialized anyway, whereas cutting an internal variable forces it to survive
    // via its DfgPrev when it could otherwise be inlined away later.
    std::సదిశ<Dfg_శీర్షం_Var*> varps;
    కొరకు (Dfg_శీర్షం_Var& vtx : dfg.varVertices()) {
        ఒకవేళ (!vtx.tmpForp()) varps.నెట్టు_వెనుక(&vtx);
    }
    std::stable_విభజన(varps.begin(), varps.చివర(), [](స్థిరరాశి Dfg_శీర్షం_Var* varp) {
        తిరిగి varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    కొరకు (Dfg_శీర్షం_Var* స్థిరరాశి varp : varps) {
        // Stop as soon as it becomes acyclic
        ఒకవేళ (!sccInfo.isCyclic()) ఆపు;

        // Ignore if variable is not part of a cycle
        ఒకవేళ (!sccInfo.get(*varp)) కొనసాగు;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* స్థిరరాశి prevp = కొత్త DfgPrev{dfg, varp->vscp()};
        sccInfo.కూడు(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    ఒకవేళ (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
శూన్య V3DfgPasses::break_చక్రాలు(Dfg_లేఖాచిత్రం& dfg, V3DfgContext& ctx) {
    V3_Dfg_Break_చక్రాలు::break_చక్రాలు(dfg, ctx.m_break_చక్రాలు_Context);
    ఒకవేళ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::remove_వాడని(dfg);
}
