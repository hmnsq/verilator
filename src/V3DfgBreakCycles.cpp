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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hash.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <fstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <queue>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਨਾਮਾਕਾਸ਼ V3_Dfg_Break_ਚੱਕਰਾਂ {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
ਵਰਗ SccInfo final {
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, uint64_t> m_vtx2Scc;
    ਮਾਪ_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

ਜਨਤਕ:
    SccInfo(Dfg_ਗ੍ਰਾਫ& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
            ਅਚਲ uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            ਜੇ (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    ਤਾਰਕਿਕ isCyclic() ਅਚਲ { ਵਾਪਸ m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) ਅਚਲ { ਵਾਪਸ m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    ਖਾਲੀ ਜੋੜੋ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx, uint64_t sccIdx) {
        ਅਚਲ ਤਾਰਕਿਕ new_ਇੰਦਰਾਜ = m_vtx2Scc.emplace(&vtx, sccIdx).ਦੂਜਾ;
        UASSERT_OBJ(new_ਇੰਦਰਾਜ, &vtx, "Vertex already inserted");
        ਜੇ (sccIdx) ++m_nCyclicVertices;
    }

    ਤਾਰਕਿਕ stillCyclicFwd(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) ਅਚਲ {
        // If it only reads non-cyclic vertices, it has also become acyclic
        ਵਾਪਸ vtx.foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {  //
            ਵਾਪਸ m_vtx2Scc.at(&src);
        });
    }

    ਤਾਰਕਿਕ stillCyclicBwd(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) ਅਚਲ {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        ਵਾਪਸ vtx.foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& dst) {  //
            ਵਾਪਸ m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    ਖਾਲੀ updateAcyclic(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& dst) {
            // Sink was known to be acyclic, stop
            ਜੇ (!m_vtx2Scc.at(&dst)) ਵਾਪਸ ਝੂਠ;
            ਜੇ (!stillCyclicFwd(dst)) updateAcyclic(dst);
            ਵਾਪਸ ਝੂਠ;
        });
        // Propagate the update through the graph backward
        vtx.foreach_ਸਰੋਤ([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src) {
            // Source was known to be acyclic, stop
            ਜੇ (!m_vtx2Scc.at(&src)) ਵਾਪਸ ਝੂਠ;
            ਜੇ (!stillCyclicBwd(src)) updateAcyclic(src);
            ਵਾਪਸ ਝੂਠ;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    ਖਾਲੀ updateReplacement(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx, Dfg_ਸ਼ੀਰਸ਼& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        ਜੇ (!get(replacement)) {
            replacement.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& dst) {
                ਜੇ (!get(dst)) ਵਾਪਸ ਝੂਠ;
                ਜੇ (!stillCyclicFwd(dst)) updateAcyclic(dst);
                ਵਾਪਸ ਝੂਠ;
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
    ਖਾਲੀ validate() ਅਚਲ {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

ਵਰਗ TraceDriver final : ਜਨਤਕ DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    ਬਣਤਰ Cache_ਕੁੰਜੀ final {
        Dfg_ਸ਼ੀਰਸ਼* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_ਕੁੰਜੀ() = ਮਿਟਾਓ;
        Cache_ਕੁੰਜੀ(Dfg_ਸ਼ੀਰਸ਼* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        ਬਣਤਰ Hash final {
            ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Cache_ਕੁੰਜੀ& ਵਸਤੂ_3) ਅਚਲ {
                // cppcheck-suppress unreadVariable
                V3Hash hash{ਵਸਤੂ_3.m_vtxp};
                hash += ਵਸਤੂ_3.m_lsb;
                hash += ਵਸਤੂ_3.m_msb;
                ਵਾਪਸ hash.ਮੁੱਲ();
            }
        };

        ਬਣਤਰ Equal final {
            ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ Cache_ਕੁੰਜੀ& a, ਅਚਲ Cache_ਕੁੰਜੀ& b) ਅਚਲ {
                ਵਾਪਸ a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
            }
        };
    };

    // STATE
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    // The strongly connected component we are currently trying to escape
    uint64_t m_component = 0;
    uint32_t m_lsb = 0;  // LSB to extract from the currently visited Vertex
    uint32_t m_msb = 0;  // MSB to extract from the currently visited Vertex
    std::ਸਦਿਸ਼<uint32_t> m_idxs;  // Indices to extract from the currently visited Vertex
    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼**> m_pendingps;  // Pending pass-through cache entries across trace calls
    // Result of tracing the currently visited Vertex. Use RETURN_RESULT below!
    Dfg_ਸ਼ੀਰਸ਼* m_resp = ਖਾਲੀ_ਸੂਚਕ;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    Dfg_ਸ਼ੀਰਸ਼* m_tailp = ਖਾਲੀ_ਸੂਚਕ;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    Dfg_ਸ਼ੀਰਸ਼* m_defaultp = ਖਾਲੀ_ਸੂਚਕ;
    Dfg_ਸ਼ੀਰਸ਼* m_splicep = ਖਾਲੀ_ਸੂਚਕ;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_ਕੁੰਜੀ, Dfg_ਸ਼ੀਰਸ਼*, Cache_ਕੁੰਜੀ::Hash, Cache_ਕੁੰਜੀ::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_ਕੁੰਜੀ, Dfg_ਸ਼ੀਰਸ਼*, Cache_ਕੁੰਜੀ::Hash, Cache_ਕੁੰਜੀ::Equal> m_boundaryCache;

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
    std::ofstream m_ਸਤਰ_Coverage_File;  // Line coverage file, just for testing
#ਜੇ_ਅੰਤ

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    ਸ਼ੀਰਸ਼_2* make(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* refp, uint32_t ਚੌੜਾਈ) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ  //
                          && !std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_Var, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ,
                      "Should only make operation vertices and constants");

        ਜੇ VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ) {
            DfgConst* ਅਚਲ vtxp = ਨਵਾਂ DfgConst{m_dfg, refp->fileline(), ਚੌੜਾਈ, 0};
            m_sccInfo.ਜੋੜੋ(*vtxp, 0);
            ਵਾਪਸ reinterpret_cast<ਸ਼ੀਰਸ਼_2*>(vtxp);
        } ਨਹੀਂਤਾਂ {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            ਵਰਤੋਂ Vtx = ਕਿਸਮ_ਨਾਂ std::conditional<std::is_same<DfgConst, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, DfgSel,
                                                  ਸ਼ੀਰਸ਼_2>::type;
            Vtx* ਅਚਲ vtxp = ਨਵਾਂ Vtx{m_dfg, refp->fileline(), Dfg_ਅੰਕੜੇ_Type::packed(ਚੌੜਾਈ)};
            m_sccInfo.ਜੋੜੋ(*vtxp, 0);
            ਵਾਪਸ reinterpret_cast<ਸ਼ੀਰਸ਼_2*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    Dfg_ਸ਼ੀਰਸ਼_Var* createTmp(ਅਚਲ ਅੱਖਰ* ਅਗੇਤਰ, Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
        Dfg_ਸ਼ੀਰਸ਼::ScopeCache scopeCache;
        AstScope* ਅਚਲ scopep = vtxp->scopep(scopeCache);
        Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = m_dfg.makeNewVar(flp, ਅਗੇਤਰ, vtxp->dtype(), scopep);
        varp->vscp()->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
        varp->tmpForp(varp->vscp());
        m_sccInfo.ਜੋੜੋ(*varp, 0);
        ਵਾਪਸ varp;
    }

    // Trace drivers of the given packed vertex, at the given bit range.
    Dfg_ਸ਼ੀਰਸ਼* trace(Dfg_ਸ਼ੀਰਸ਼* vtxp, uint32_t msb, uint32_t lsb) {
        // Many vertices just pass the traced bit range through to one of their sources
        // (see RETURN_RESULT_TAIL). Chains of such vertices can be tens of thousands of
        // vertices long in large designs (e.g.: a wide concatenation), so resolve them
        // in this loop instead of by recursion, which would overflow the C++ stack.
        // 'm_pendingps' holds the cache entries of the vertices on the current chain. They
        // all yield the same result, which is filled in when the chain is resolved.
        // Note: references into 'm_cache' are stable across the insertions below.
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ ਸ਼ੁਰੂ_Vtxp = vtxp;
        ਅਚਲ uint32_t ਸ਼ੁਰੂ_Msb = msb;
        ਅਚਲ uint32_t ਸ਼ੁਰੂ_Lsb = lsb;
        ਅਚਲ ਮਾਪ_t pendingBegin = m_pendingps.ਮਾਪ();
        Dfg_ਸ਼ੀਰਸ਼* resp = ਖਾਲੀ_ਸੂਚਕ;

        ਜਦੋਂ (ਸੱਚ) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->ਮਾਪ() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            ਜੇ (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                Dfg_ਸ਼ੀਰਸ਼*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(ਖਾਲੀ_ਸੂਚਕ))
                          .ਪਹਿਲਾ->ਦੂਜਾ;
                ਜੇ (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ splicep = respr->cast<Dfg_ਸ਼ੀਰਸ਼_Splice>()) {
                        Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    ਜੇ (msb != respr->ਚੌੜਾਈ() - 1 || lsb != 0) {
                        DfgSel* ਅਚਲ selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                ਰੋਕੋ;
            }

            // Normal cache for intra-component traces
            Dfg_ਸ਼ੀਰਸ਼*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(ਖਾਲੀ_ਸੂਚਕ))
                                    .ਪਹਿਲਾ->ਦੂਜਾ;

            // Trace the vertex
            ਜੇ (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } ਨਹੀਂਤਾਂ {
                // Otherwise visit the vertex to trace it
                VL_RESTORER(m_msb);
                VL_RESTORER(m_lsb);
                VL_RESTORER_CLEAR(m_idxs);
                VL_RESTORER(m_resp);
                VL_RESTORER(m_tailp);
                m_msb = msb;
                m_lsb = lsb;
                m_resp = ਖਾਲੀ_ਸੂਚਕ;
                m_tailp = ਖਾਲੀ_ਸੂਚਕ;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != ਖਾਲੀ_ਸੂਚਕ) != (m_tailp != ਖਾਲੀ_ਸੂਚਕ), vtxp,
                            "Expected exactly one trace result");
                ਜੇ (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ਧੱਕੋ_ਪਿੱਠ(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    ਜਾਰੀ;
                }
                respr = m_resp;
            }
            resp = respr;
            ਰੋਕੋ;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, ਸ਼ੁਰੂ_Vtxp, "Tracing driver failed for " << ਸ਼ੁਰੂ_Vtxp->typeName());
        UASSERT_OBJ(resp->ਚੌੜਾਈ() == (ਸ਼ੁਰੂ_Msb - ਸ਼ੁਰੂ_Lsb + 1), ਸ਼ੁਰੂ_Vtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        ਲਈ (ਮਾਪ_t i = pendingBegin; i < m_pendingps.ਮਾਪ(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        ਵਾਪਸ resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    Dfg_ਸ਼ੀਰਸ਼* traceSameIdx(Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.ਖਾਲੀ_ਥਾਵਾਂ(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = ਖਾਲੀ_ਸੂਚਕ;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->ਚੌੜਾਈ() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        ਵਾਪਸ m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    Dfg_ਸ਼ੀਰਸ਼* trace_ਧੱਕੋ_Idx(Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp, ਅਚਲ uint32_t idx) {
        ਅਚਲ ਮਾਪ_t nIdxs = m_idxs.ਮਾਪ();
        m_idxs.ਧੱਕੋ_ਪਿੱਠ(idx);
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ resp = traceSameIdx(vtxp);
        m_idxs.ਕੱਢੋ_ਪਿੱਠ();
        UASSERT_OBJ(m_idxs.ਮਾਪ() == nIdxs, vtxp, "Index stack size mismatch");
        ਵਾਪਸ resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    Dfg_ਸ਼ੀਰਸ਼* trace_ਕੱਢੋ_Idx(Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp) {
        UASSERT_OBJ(!m_idxs.ਖਾਲੀ_ਥਾਵਾਂ(), vtxp, "Should have a pending index");
        ਅਚਲ ਮਾਪ_t nIdxs = m_idxs.ਮਾਪ();
        ਅਚਲ uint32_t idx = m_idxs.ਪਿੱਠ();
        m_idxs.ਕੱਢੋ_ਪਿੱਠ();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ resp = m_idxs.ਖਾਲੀ_ਥਾਵਾਂ() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ਧੱਕੋ_ਪਿੱਠ(idx);
        UASSERT_OBJ(m_idxs.ਮਾਪ() == nIdxs, vtxp, "Index stack size mismatch");
        ਵਾਪਸ resp;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    ਸ਼ੀਰਸ਼_2* trace_Bitwise_ਦੁਆਧਾਰੀ(ਸ਼ੀਰਸ਼_2* vtxp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ,
                      "Should only call on DfgVertexBinary");
        ਸ਼ੀਰਸ਼_2* ਅਚਲ resp = make<ਸ਼ੀਰਸ਼_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        ਵਾਪਸ resp;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    Dfg_ਸ਼ੀਰਸ਼* trace_ਜੋੜੋ_Sub(ਸ਼ੀਰਸ਼_2* vtxp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ,
                      "Should only call on DfgVertexBinary");
        ਸ਼ੀਰਸ਼_2* ਅਚਲ opp = make<ਸ਼ੀਰਸ਼_2>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ਅਚਲ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        ਵਾਪਸ selp;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    ਸ਼ੀਰਸ਼_2* traceReduction(ਸ਼ੀਰਸ਼_2* vtxp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_Unary, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ,
                      "Should only call on DfgVertexUnary");
        ਸ਼ੀਰਸ਼_2* ਅਚਲ resp = make<ਸ਼ੀਰਸ਼_2>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->ਚੌੜਾਈ() - 1, 0));
        ਵਾਪਸ resp;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    ਸ਼ੀਰਸ਼_2* traceCmp(ਸ਼ੀਰਸ਼_2* vtxp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ,
                      "Should only call on DfgVertexBinary");
        ਸ਼ੀਰਸ਼_2* ਅਚਲ resp = make<ਸ਼ੀਰਸ਼_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ਚੌੜਾਈ() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ਚੌੜਾਈ() - 1, 0));
        ਵਾਪਸ resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ TRACE_ਨਤੀਜਾ_COVER() \
    ਕਰੋ { \
        ਜੇ (VL_UNLIKELY(m_ਸਤਰ_Coverage_File.is_open())) m_ਸਤਰ_Coverage_File << __LINE__ << '\n'; \
    } ਜਦੋਂ (ਝੂਠ)
#ਨਹੀਂਤਾਂ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ TRACE_ਨਤੀਜਾ_COVER() \
    ਕਰੋ { \
    } ਜਦੋਂ (ਝੂਠ)
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ RETURN_ਨਤੀਜਾ(vtxp) \
    ਕਰੋ { \
        m_resp = (vtxp); \
        TRACE_ਨਤੀਜਾ_COVER(); \
        ਵਾਪਸ; \
    } ਜਦੋਂ (ਝੂਠ)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ RETURN_ਨਤੀਜਾ_ਪੂਛ(srcp, smsb, slsb) \
    ਕਰੋ { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ tailp = (srcp); \
        ਅਚਲ uint32_t ਪੂਛ_Msb = (smsb); \
        ਅਚਲ uint32_t ਪੂਛ_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = ਪੂਛ_Msb; \
        m_lsb = ਪੂਛ_Lsb; \
        TRACE_ਨਤੀਜਾ_COVER(); \
        ਵਾਪਸ; \
    } ਜਦੋਂ (ਝੂਠ)

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Dfg_ਸ਼ੀਰਸ਼* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ਖਾਲੀ ਦੇਖੋ(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        Dfg_ਸ਼ੀਰਸ਼* defaultp = m_defaultp;
        m_defaultp = ਖਾਲੀ_ਸੂਚਕ;
        m_splicep = ਖਾਲੀ_ਸੂਚਕ;

        ਬਣਤਰ Driver final {
            Dfg_ਸ਼ੀਰਸ਼* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = ਮਿਟਾਓ;
            Driver(Dfg_ਸ਼ੀਰਸ਼* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::ਸਦਿਸ਼<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        ਤਾਰਕਿਕ try_ਸਾਰਾ_Default = defaultp;
        Dfg_ਸ਼ੀਰਸ਼* coverp = ਖਾਲੀ_ਸੂਚਕ;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](Dfg_ਸ਼ੀਰਸ਼& src, uint32_t lsb) {
            ਅਚਲ uint32_t msb = lsb + src.ਚੌੜਾਈ() - 1;
            drivers.emplace_ਪਿੱਠ(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            ਜੇ (m_msb >= lsb && msb >= m_lsb) try_ਸਾਰਾ_Default = ਝੂਠ;
            // If it does not cover the whole searched bit range, move on
            ਜੇ (m_lsb < lsb || msb < m_msb) ਵਾਪਸ ਝੂਠ;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            ਵਾਪਸ ਸੱਚ;
        });

        // Trace the driver covering the whole searched range, if there is one
        ਜੇ (coverp) RETURN_ਨਤੀਜਾ_ਪੂਛ(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        ਜੇ (try_ਸਾਰਾ_Default) RETURN_ਨਤੀਜਾ_ਪੂਛ(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::ਛਾਂਟੋ(drivers.begin(), drivers.ਸਿਰਾ(), [](ਅਚਲ Driver& a, ਅਚਲ Driver& b) {  //
            ਵਾਪਸ a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> termps;
        uint32_t lsb = m_lsb;
        ਲਈ (ਅਚਲ Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            ਜੇ (lsb > driver.m_msb) ਜਾਰੀ;
            // Driver is above the searched MSB, done
            ਜੇ (driver.m_lsb > m_msb) ਰੋਕੋ;
            // Gap below this driver, trace default to fill it
            ਜੇ (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_ਪਿੱਠ(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ਅਚਲ uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_ਪਿੱਠ(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        ਜੇ (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_ਪਿੱਠ(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.ਮਾਪ() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        Dfg_ਸ਼ੀਰਸ਼* resp = termps.front();
        ਲਈ (ਮਾਪ_t i = 1; i < termps.ਮਾਪ(); ++i) {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ termp = termps[i];
            DfgConcat* ਅਚਲ catp = make<DfgConcat>(termp, resp->ਚੌੜਾਈ() + termp->ਚੌੜਾਈ());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_ਨਤੀਜਾ(resp);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ defaultp = m_defaultp;
        m_defaultp = ਖਾਲੀ_ਸੂਚਕ;
        m_splicep = ਖਾਲੀ_ਸੂਚਕ;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ਅਚਲ uint32_t idx = m_idxs.ਪਿੱਠ();
        ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ driverp = vtxp->driverAt(idx)) {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = driverp->as<DfgUnitArray>()->srcp();
            ਜੇ (srcp->is<Dfg_ਸ਼ੀਰਸ਼_Splice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_ਨਤੀਜਾ(trace_ਕੱਢੋ_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_ਨਤੀਜਾ(traceSameIdx(defaultp));
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਸ਼ੀਰਸ਼_Var* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ defaultp = vtxp->defaultp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        ਜੇ (srcp && srcp->is<Dfg_ਸ਼ੀਰਸ਼_Splice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_ਨਤੀਜਾ(m_idxs.ਖਾਲੀ_ਥਾਵਾਂ() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    ਖਾਲੀ ਦੇਖੋ(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_ਨਤੀਜਾ(trace_ਧੱਕੋ_Idx(vtxp->fromp(), idxp->ਮੰਜ਼ਿਲ_U32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.ਖਾਲੀ_ਥਾਵਾਂ(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ਅਚਲ resp = make<DfgArraySel>(vtxp, vtxp->ਚੌੜਾਈ());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->ਚੌੜਾਈ() - 1, 0));
        DfgSel* ਅਚਲ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_ਨਤੀਜਾ(selp);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.ਪਿੱਠ() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_ਨਤੀਜਾ(trace_ਕੱਢੋ_Idx(vtxp->srcp()));
    }

    ਖਾਲੀ ਦੇਖੋ(DfgConcat* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        ਅਚਲ uint32_t r_ਚੌੜਾਈ = rhsp->ਚੌੜਾਈ();
        // If the traced bits are wholly in the RHS
        ਜੇ (r_ਚੌੜਾਈ > m_msb) RETURN_ਨਤੀਜਾ_ਪੂਛ(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        ਜੇ (m_lsb >= r_ਚੌੜਾਈ) RETURN_ਨਤੀਜਾ_ਪੂਛ(lhsp, m_msb - r_ਚੌੜਾਈ, m_lsb - r_ਚੌੜਾਈ);
        // The traced bit spans both sides, trace both
        DfgConcat* ਅਚਲ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, r_ਚੌੜਾਈ - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - r_ਚੌੜਾਈ, 0));
        RETURN_ਨਤੀਜਾ(resp);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgRep* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        ਅਚਲ uint32_t s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
        // If we need more bits than the source, then we need the whole source
        ਜੇ (m_msb - m_lsb + 1 > s_ਚੌੜਾਈ) {
            DfgRep* ਅਚਲ repp = make<DfgRep>(vtxp, vtxp->ਚੌੜਾਈ());
            repp->srcp(trace(srcp, s_ਚੌੜਾਈ - 1, 0));
            DfgSel* ਅਚਲ resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_ਨਤੀਜਾ(resp);
        }
        // If the requested bits are within the same repliacted word
        ਜੇ (m_msb / s_ਚੌੜਾਈ == m_lsb / s_ਚੌੜਾਈ) {
            RETURN_ਨਤੀਜਾ_ਪੂਛ(srcp, m_msb % s_ਚੌੜਾਈ, m_lsb % s_ਚੌੜਾਈ);
        }
        // The requested bits span two replicated words
        DfgConcat* ਅਚਲ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, s_ਚੌੜਾਈ - 1, m_lsb % s_ਚੌੜਾਈ));
        resp->lhsp(trace(srcp, m_msb % s_ਚੌੜਾਈ, 0));
        RETURN_ਨਤੀਜਾ(resp);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgExtend* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        ਅਚਲ uint32_t s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
        // If the traced bits are wholly in the input
        ਜੇ (s_ਚੌੜਾਈ > m_msb) RETURN_ਨਤੀਜਾ_ਪੂਛ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ਜੇ (m_lsb >= s_ਚੌੜਾਈ) RETURN_ਨਤੀਜਾ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ਅਚਲ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_ਚੌੜਾਈ - 1, m_lsb));
        RETURN_ਨਤੀਜਾ(resp);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgExtendS* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        ਅਚਲ uint32_t s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
        // If the traced bits are wholly in the input
        ਜੇ (s_ਚੌੜਾਈ > m_msb) RETURN_ਨਤੀਜਾ_ਪੂਛ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ਜੇ (m_lsb >= s_ਚੌੜਾਈ) {
            ਜੇ (m_msb == m_lsb) RETURN_ਨਤੀਜਾ_ਪੂਛ(srcp, s_ਚੌੜਾਈ - 1, s_ਚੌੜਾਈ - 1);
            DfgExtendS* ਅਚਲ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, s_ਚੌੜਾਈ - 1, s_ਚੌੜਾਈ - 1));
            RETURN_ਨਤੀਜਾ(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ਅਚਲ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_ਚੌੜਾਈ - 1, m_lsb));
        RETURN_ਨਤੀਜਾ(resp);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgSel* vtxp) override {
        ਅਚਲ uint32_t lsb = vtxp->lsb();
        RETURN_ਨਤੀਜਾ_ਪੂਛ(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgNot* vtxp) override {
        DfgNot* ਅਚਲ resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_ਨਤੀਜਾ(resp);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgAnd* vtxp) override { RETURN_ਨਤੀਜਾ(trace_Bitwise_ਦੁਆਧਾਰੀ(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgOr* vtxp) override { RETURN_ਨਤੀਜਾ(trace_Bitwise_ਦੁਆਧਾਰੀ(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgXor* vtxp) override { RETURN_ਨਤੀਜਾ(trace_Bitwise_ਦੁਆਧਾਰੀ(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(Dfg_ਜੋੜੋ* vtxp) override { RETURN_ਨਤੀਜਾ(trace_ਜੋੜੋ_Sub(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgSub* vtxp) override { RETURN_ਨਤੀਜਾ(trace_ਜੋੜੋ_Sub(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgRedAnd* vtxp) override { RETURN_ਨਤੀਜਾ(traceReduction(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgRedOr* vtxp) override { RETURN_ਨਤੀਜਾ(traceReduction(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgRedXor* vtxp) override { RETURN_ਨਤੀਜਾ(traceReduction(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgEq* vtxp) override { RETURN_ਨਤੀਜਾ(traceCmp(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgNeq* vtxp) override { RETURN_ਨਤੀਜਾ(traceCmp(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgLt* vtxp) override { RETURN_ਨਤੀਜਾ(traceCmp(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgLte* vtxp) override { RETURN_ਨਤੀਜਾ(traceCmp(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgGt* vtxp) override { RETURN_ਨਤੀਜਾ(traceCmp(vtxp)); }
    ਖਾਲੀ ਦੇਖੋ(DfgGte* vtxp) override { RETURN_ਨਤੀਜਾ(traceCmp(vtxp)); }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_RS* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amnt = rConstp->ਮੰਜ਼ਿਲ_U32();
            // Width of lower half of result
            ਅਚਲ uint32_t ਹੇਠਲਾ_ਚੌੜਾਈ = ਖਿਸਕਾਓ_Amnt > vtxp->ਚੌੜਾਈ() ? 0 : vtxp->ਚੌੜਾਈ() - ਖਿਸਕਾਓ_Amnt;
            // If the traced bits are wholly in the input
            ਜੇ (ਹੇਠਲਾ_ਚੌੜਾਈ > m_msb) {
                RETURN_ਨਤੀਜਾ_ਪੂਛ(lhsp, m_msb + ਖਿਸਕਾਓ_Amnt, m_lsb + ਖਿਸਕਾਓ_Amnt);
            }
            // If the traced bits are wholly in the extension
            ਜੇ (m_lsb >= ਹੇਠਲਾ_ਚੌੜਾਈ) {
                DfgExtendS* ਅਚਲ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->ਚੌੜਾਈ() - 1, lhsp->ਚੌੜਾਈ() - 1));
                RETURN_ਨਤੀਜਾ(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ਅਚਲ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, ਹੇਠਲਾ_ਚੌੜਾਈ - 1 + ਖਿਸਕਾਓ_Amnt, m_lsb + ਖਿਸਕਾਓ_Amnt));
            RETURN_ਨਤੀਜਾ(resp);
        }

        Dfg_ਖਿਸਕਾਓ_RS* ਅਚਲ shiftrsp = make<Dfg_ਖਿਸਕਾਓ_RS>(vtxp, vtxp->lhsp()->ਚੌੜਾਈ() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ਚੌੜਾਈ() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ਚੌੜਾਈ() - 1, m_lsb));
        ਜੇ (m_msb == vtxp->lhsp()->ਚੌੜਾਈ() - 1) RETURN_ਨਤੀਜਾ(shiftrsp);
        DfgSel* ਅਚਲ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_ਨਤੀਜਾ(selp);
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_R* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amnt = rConstp->ਮੰਜ਼ਿਲ_U32();
            // Width of lower half of result
            ਅਚਲ uint32_t ਹੇਠਲਾ_ਚੌੜਾਈ = ਖਿਸਕਾਓ_Amnt > vtxp->ਚੌੜਾਈ() ? 0 : vtxp->ਚੌੜਾਈ() - ਖਿਸਕਾਓ_Amnt;
            // If the traced bits are wholly in the input
            ਜੇ (ਹੇਠਲਾ_ਚੌੜਾਈ > m_msb) {
                RETURN_ਨਤੀਜਾ_ਪੂਛ(lhsp, m_msb + ਖਿਸਕਾਓ_Amnt, m_lsb + ਖਿਸਕਾਓ_Amnt);
            }
            // If the traced bits are wholly in the extension
            ਜੇ (m_lsb >= ਹੇਠਲਾ_ਚੌੜਾਈ) RETURN_ਨਤੀਜਾ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ਅਚਲ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, ਹੇਠਲਾ_ਚੌੜਾਈ - 1 + ਖਿਸਕਾਓ_Amnt, m_lsb + ਖਿਸਕਾਓ_Amnt));
            RETURN_ਨਤੀਜਾ(resp);
        }

        Dfg_ਖਿਸਕਾਓ_R* ਅਚਲ shiftrp = make<Dfg_ਖਿਸਕਾਓ_R>(vtxp, vtxp->lhsp()->ਚੌੜਾਈ() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ਚੌੜਾਈ() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ਚੌੜਾਈ() - 1, m_lsb));
        ਜੇ (m_msb == vtxp->lhsp()->ਚੌੜਾਈ() - 1) RETURN_ਨਤੀਜਾ(shiftrp);
        DfgSel* ਅਚਲ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_ਨਤੀਜਾ(selp);
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_L* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amnt = rConstp->ਮੰਜ਼ਿਲ_U32();
            // Width of lower half of result
            ਅਚਲ uint32_t ਹੇਠਲਾ_ਚੌੜਾਈ = ਖਿਸਕਾਓ_Amnt > vtxp->ਚੌੜਾਈ() ? vtxp->ਚੌੜਾਈ() : ਖਿਸਕਾਓ_Amnt;
            // If the traced bits are wholly in the input
            ਜੇ (m_lsb >= ਹੇਠਲਾ_ਚੌੜਾਈ) {
                RETURN_ਨਤੀਜਾ_ਪੂਛ(lhsp, m_msb - ਖਿਸਕਾਓ_Amnt, m_lsb - ਖਿਸਕਾਓ_Amnt);
            }
            // If the traced bits are wholly in the extension
            ਜੇ (ਹੇਠਲਾ_ਚੌੜਾਈ > m_msb) RETURN_ਨਤੀਜਾ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ਅਚਲ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->ਚੌੜਾਈ() - (m_msb - ਹੇਠਲਾ_ਚੌੜਾਈ + 1)));
            resp->lhsp(trace(lhsp, m_msb - ਖਿਸਕਾਓ_Amnt, ਹੇਠਲਾ_ਚੌੜਾਈ - ਖਿਸਕਾਓ_Amnt));
            RETURN_ਨਤੀਜਾ(resp);
        }

        Dfg_ਖਿਸਕਾਓ_L* ਅਚਲ shiftlp = make<Dfg_ਖਿਸਕਾਓ_L>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ਚੌੜਾਈ() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        ਜੇ (m_lsb == 0) RETURN_ਨਤੀਜਾ(shiftlp);
        DfgSel* ਅਚਲ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_ਨਤੀਜਾ(selp);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgCond* vtxp) override {
        DfgCond* ਅਚਲ resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->ਚੌੜਾਈ() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_ਨਤੀਜਾ(resp);
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਮੇਲ_Masked* vtxp) override {
        Dfg_ਮੇਲ_Masked* ਅਚਲ resp = make<Dfg_ਮੇਲ_Masked>(vtxp, vtxp->ਚੌੜਾਈ());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ਚੌੜਾਈ() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->ਚੌੜਾਈ() - 1, 0));
        DfgSel* ਅਚਲ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_ਨਤੀਜਾ(selp);
    }

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ RETURN_ਨਤੀਜਾ
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ TRACE_ਨਤੀਜਾ_COVER
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ RETURN_ਨਤੀਜਾ_ਪੂਛ

ਜਨਤਕ:
    // CONSTRUCTOR
    TraceDriver(Dfg_ਗ੍ਰਾਫ& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
        ਜੇ (v3Global.opt.debugCheck()) {
            m_ਸਤਰ_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.ਅਗੇਤਰ()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_ਅਧਾਰ::ਬਾਹਰਲਾ | std::ios_ਅਧਾਰ::app);
        }
#ਜੇ_ਅੰਤ
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    Dfg_ਸ਼ੀਰਸ਼* apply(Dfg_ਸ਼ੀਰਸ਼& vtx, uint32_t lsb, uint32_t ਚੌੜਾਈ) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        ਵਾਪਸ trace(&vtx, lsb + ਚੌੜਾਈ - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
ਵਰਗ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ final {
    // TYPES
    ਅਚਲ Dfg_ਅੰਕੜੇ_Type& m_dtype;
    ਸੰਘ {
        V3_ਸੰਖਿਆ m_ਅੰਕ;  // The bits, if packed
        std::ਸਦਿਸ਼<ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ> m_sub;  // The per element masks, if aggregate
    };

ਜਨਤਕ:
    // CONSTRUCTOR
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ(File_ਸਤਰ* flp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        ਜੇ (m_dtype.isPacked()) {
            ਨਵਾਂ (&m_ਅੰਕ) V3_ਸੰਖਿਆ{flp, static_cast<ਪੂਰਨ>(m_dtype.ਮਾਪ()), 0};
        } ਨਹੀਂਤਾਂ {
            ਨਵਾਂ (&m_sub) std::ਸਦਿਸ਼<ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ>{};
            m_sub.reserve(m_dtype.ਮਾਪ());
            ਲਈ (uint32_t i = 0; i < m_dtype.ਮਾਪ(); ++i)
                m_sub.emplace_ਪਿੱਠ(flp, m_dtype.elemDtype());
        }
    }
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx)
        : ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ{vtx.fileline(), vtx.dtype()} {}
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ(ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& other)
        : m_dtype{other.m_dtype} {
        ਜੇ (m_dtype.isPacked()) {
            ਨਵਾਂ (&m_ਅੰਕ) V3_ਸੰਖਿਆ{other.m_ਅੰਕ};
        } ਨਹੀਂਤਾਂ {
            ਨਵਾਂ (&m_sub) std::ਸਦਿਸ਼<ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ>{other.m_sub};
        }
    }
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& ਸੰਚਾਲਕ=(ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& other) {
        ਜੇ (ਇਹ != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            ਜੇ (other.m_dtype.isPacked()) {
                m_ਅੰਕ = other.m_ਅੰਕ;
            } ਨਹੀਂਤਾਂ {
                m_sub = other.m_sub;
            }
        }
        ਵਾਪਸ *ਇਹ;
    }
    ~ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ() {
        ਜੇ (m_dtype.isPacked()) {
            m_ਅੰਕ.~V3_ਸੰਖਿਆ();
        } ਨਹੀਂਤਾਂ {
            m_sub.~ਸਦਿਸ਼<ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ>();
        }
    }

    // METHODS
    V3_ਸੰਖਿਆ& ਅੰਕ() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ਵਾਪਸ m_ਅੰਕ;
    }
    ਅਚਲ V3_ਸੰਖਿਆ& ਅੰਕ() ਅਚਲ {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ਵਾਪਸ m_ਅੰਕ;
    }
    std::ਸਦਿਸ਼<ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        ਵਾਪਸ m_sub;
    }

    ਤਾਰਕਿਕ isZero() ਅਚਲ {
        ਜੇ (m_dtype.isPacked()) ਵਾਪਸ m_ਅੰਕ.isEqZero();

        ਲਈ (ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& sub : m_sub) {
            ਜੇ (!sub.isZero()) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਤਾਰਕਿਕ isOnes() ਅਚਲ {
        ਜੇ (m_dtype.isPacked()) ਵਾਪਸ m_ਅੰਕ.isEqAllOnes();

        ਲਈ (ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& sub : m_sub) {
            ਜੇ (!sub.isOnes()) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }
    ਖਾਲੀ ਸੈੱਟ_Zero() {
        ਜੇ (m_dtype.isPacked()) {
            m_ਅੰਕ.ਸੈੱਟ_All_Bits0();
        } ਨਹੀਂਤਾਂ {
            ਲਈ (ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& sub : m_sub) sub.ਸੈੱਟ_Zero();
        }
    }
    ਖਾਲੀ ਸੈੱਟ_Ones() {
        ਜੇ (m_dtype.isPacked()) {
            m_ਅੰਕ.ਸੈੱਟ_All_Bits1();
        } ਨਹੀਂਤਾਂ {
            ਲਈ (ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& sub : m_sub) sub.ਸੈੱਟ_Ones();
        }
    }

    ਤਾਰਕਿਕ ਸੰਚਾਲਕ==(ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& other) ਅਚਲ {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        ਜੇ (m_dtype.isPacked()) ਵਾਪਸ m_ਅੰਕ.isCaseEq(other.m_ਅੰਕ);
        ਵਾਪਸ m_sub == other.m_sub;
    }
    ਤਾਰਕਿਕ ਸੰਚਾਲਕ!=(ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& other) ਅਚਲ { ਵਾਪਸ !(*ਇਹ == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    ਤਾਰਕਿਕ is_Contraction_ਦਾ(ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& that) ਅਚਲ {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        ਜੇ (m_dtype.isPacked()) {
            ਅਚਲ ਮਾਪ_t ਸ਼ਬਦ_2 = VL_ਸ਼ਬਦ_I(m_dtype.ਮਾਪ());
            ਲਈ (ਮਾਪ_t i = 0; i < ਸ਼ਬਦ_2; ++i) {
                ਜੇ (~m_ਅੰਕ.edata_ਸ਼ਬਦ(i) & that.m_ਅੰਕ.edata_ਸ਼ਬਦ(i)) ਵਾਪਸ ਸੱਚ;
            }
            ਵਾਪਸ ਝੂਠ;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        ਲਈ (ਮਾਪ_t i = 0; i < m_sub.ਮਾਪ(); ++i) {
            ਜੇ (m_sub[i].is_Contraction_ਦਾ(that.m_sub[i])) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    std::string ਮੰਜ਼ਿਲ_String() ਅਚਲ {
        ਜੇ (m_dtype.isPacked()) ਵਾਪਸ "0x" + m_ਅੰਕ.displayed(m_ਅੰਕ.fileline(), "%x");
        std::string ਨਤੀਜਾ;
        ਨਤੀਜਾ += "{";
        ਲਈ (ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& sub : m_sub) {
            ਜੇ (&sub != &m_sub.front()) ਨਤੀਜਾ += ", ";
            ਨਤੀਜਾ += sub.ਮੰਜ਼ਿਲ_String();
        }
        ਨਤੀਜਾ += "}";
        ਵਾਪਸ ਨਤੀਜਾ;
    }
};

ਵਰਗ Independent_ਬਿੱਟ final : ਜਨਤਕ DfgVisitor {
    // TYPES
    ਬਣਤਰ ਸ਼ੀਰਸ਼_State final {
        ਮਾਪ_t m_rpo_ਸੰਖਿਆ = 0;  // Reverse Postorder Number of vertex
        ਤਾਰਕਿਕ m_isOnWorkList = ਝੂਠ;  // Whether the vertex is on the work list
    };

    // STATE
    ਅਚਲ SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ> m_vtxp2_ਨਕਾਬ;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::ਤਰਜੀਹ_ਕਤਾਰ<ਮਾਪ_t, std::ਸਦਿਸ਼<ਮਾਪ_t>, std::greater<ਮਾਪ_t>> m_work_ਕਤਾਰ;
    std::unordered_map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, ਸ਼ੀਰਸ਼_State> m_vtxp2State;  // State of each vertex

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
    std::ofstream m_ਸਤਰ_Coverage_File;  // Line coverage file, just for testing
#ਜੇ_ਅੰਤ

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& ਨਕਾਬ(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        // Look up (or create) mask for 'vtx'
        ਵਾਪਸ m_vtxp2_ਨਕਾਬ
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .ਪਹਿਲਾ->ਦੂਜਾ;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਨਕਾਬ_2(vtxp) \
    ([ਇਹ](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) -> ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& { \
        ਜੇ (VL_UNLIKELY(m_ਸਤਰ_Coverage_File.is_open())) m_ਸਤਰ_Coverage_File << __LINE__ << '\n'; \
        ਵਾਪਸ ਨਕਾਬ(vtx); \
    }(*vtxp))
#ਨਹੀਂਤਾਂ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਨਕਾਬ_2(vtxp) ਨਕਾਬ(*vtxp)
#ਜੇ_ਅੰਤ

    // Clear all bits at or below the most significant clear bit
    ਖਾਲੀ floodTowardsLsb(V3_ਸੰਖਿਆ& ਅੰਕ) {
        ਲਈ (ਪੂਰਨ i = ਅੰਕ.ਚੌੜਾਈ() - 1; i >= 0; --i) {
            ਜੇ (ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(i)) ਜਾਰੀ;
            ਅੰਕ.op_ਸੈੱਟ_Range(0, i + 1, '0');
            ਰੋਕੋ;
        }
    }

    // Clear all bits at or above the least significant clear bit
    ਖਾਲੀ floodTowardsMsb(V3_ਸੰਖਿਆ& ਅੰਕ) {
        ਲਈ (ਪੂਰਨ i = 0; i < ਅੰਕ.ਚੌੜਾਈ(); ++i) {
            ਜੇ (ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(i)) ਜਾਰੀ;
            ਅੰਕ.op_ਸੈੱਟ_Range(i, ਅੰਕ.ਚੌੜਾਈ() - i, '0');
            ਰੋਕੋ;
        }
    }

    ਖਾਲੀ propagateFromDriver(ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& m, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* srcp) {
        // If there is no driver, we are done
        ਜੇ (!srcp) ਵਾਪਸ;
        // If it is driven by a splice, we need to combine the masks of the drivers
        ਜੇ (ਅਚਲ DfgSplicePacked* ਅਚਲ splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src, uint32_t lo) {
                m.ਅੰਕ().opSelInto(ਨਕਾਬ_2(&src).ਅੰਕ(), lo, src.ਚੌੜਾਈ());
                ਵਾਪਸ ਝੂਠ;
            });
            ਵਾਪਸ;
        }
        ਜੇ (ਅਚਲ DfgSpliceArray* ਅਚਲ splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& src, uint32_t lo) {
                ਜੇ (ਅਚਲ DfgUnitArray* ਅਚਲ uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = ਨਕਾਬ_2(uap->srcp());
                } ਨਹੀਂਤਾਂ {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                ਵਾਪਸ ਝੂਠ;
            });
            ਵਾਪਸ;
        }
        // Otherwise, we just use the mask of the single driver
        m = ਨਕਾਬ_2(srcp);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Dfg_ਸ਼ੀਰਸ਼* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਸ਼ੀਰਸ਼_Var* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        ਜੇ (vtxp->isVolatile()) ਵਾਪਸ;

        ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& m = ਨਕਾਬ_2(vtxp);
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        ਜੇ (defaultp) m = ਨਕਾਬ_2(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਸ਼ੀਰਸ਼_Splice* vtxp) override {
        propagateFromDriver(ਨਕਾਬ_2(vtxp), vtxp);  // Needed to continue traversal
    }

    ਖਾਲੀ ਦੇਖੋ(DfgUnitArray* vtxp) override { ਨਕਾਬ_2(vtxp).sub().at(0) = ਨਕਾਬ_2(vtxp->srcp()); }

    ਖਾਲੀ ਦੇਖੋ(DfgArraySel* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ idxp = vtxp->bitp()->cast<DfgConst>()) {
            ਨਕਾਬ_2(vtxp) = ਨਕਾਬ_2(vtxp->fromp()).sub().at(idxp->ਮੰਜ਼ਿਲ_ਮਾਪ_T());
            ਵਾਪਸ;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        ਜੇ (ਨਕਾਬ_2(vtxp->bitp()).isOnes() && ਨਕਾਬ_2(fromp).isOnes()) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }

    ਖਾਲੀ ਦੇਖੋ(DfgConcat* vtxp) override {
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
        m.opSelInto(ਨਕਾਬ_2(rhsp).ਅੰਕ(), 0, rhsp->ਚੌੜਾਈ());
        m.opSelInto(ਨਕਾਬ_2(lhsp).ਅੰਕ(), rhsp->ਚੌੜਾਈ(), lhsp->ਚੌੜਾਈ());
    }

    ਖਾਲੀ ਦੇਖੋ(DfgRep* vtxp) override {
        ਅਚਲ uint32_t ਗਿਣਤੀ_2 = vtxp->ਗਿਣਤੀ_2();
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        ਅਚਲ uint32_t s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
        V3_ਸੰਖਿਆ& v_ਨਕਾਬ = ਨਕਾਬ_2(vtxp).ਅੰਕ();
        V3_ਸੰਖਿਆ& s_ਨਕਾਬ = ਨਕਾਬ_2(srcp).ਅੰਕ();
        ਲਈ (uint32_t i = 0; i < ਗਿਣਤੀ_2; ++i) v_ਨਕਾਬ.opSelInto(s_ਨਕਾਬ, i * s_ਚੌੜਾਈ, s_ਚੌੜਾਈ);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgSel* vtxp) override {
        ਅਚਲ uint32_t lsb = vtxp->lsb();
        ਅਚਲ uint32_t msb = lsb + vtxp->ਚੌੜਾਈ() - 1;
        ਨਕਾਬ_2(vtxp).ਅੰਕ().opSel(ਨਕਾਬ_2(vtxp->fromp()).ਅੰਕ(), msb, lsb);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgExtend* vtxp) override {
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        ਅਚਲ uint32_t s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
        V3_ਸੰਖਿਆ& s = ਨਕਾਬ_2(srcp).ਅੰਕ();
        V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
        m.opSelInto(s, 0, s_ਚੌੜਾਈ);
        m.op_ਸੈੱਟ_Range(s_ਚੌੜਾਈ, vtxp->ਚੌੜਾਈ() - s_ਚੌੜਾਈ, '1');
    }
    ਖਾਲੀ ਦੇਖੋ(DfgExtendS* vtxp) override {
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        ਅਚਲ uint32_t s_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
        V3_ਸੰਖਿਆ& s = ਨਕਾਬ_2(srcp).ਅੰਕ();
        V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
        m.opSelInto(s, 0, s_ਚੌੜਾਈ);
        m.op_ਸੈੱਟ_Range(s_ਚੌੜਾਈ, vtxp->ਚੌੜਾਈ() - s_ਚੌੜਾਈ, s.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(s_ਚੌੜਾਈ - 1) ? '0' : '1');
    }

    ਖਾਲੀ ਦੇਖੋ(DfgNot* vtxp) override {  //
        ਨਕਾਬ_2(vtxp) = ਨਕਾਬ_2(vtxp->srcp());
    }

    ਖਾਲੀ ਦੇਖੋ(DfgAnd* vtxp) override {  //
        ਨਕਾਬ_2(vtxp).ਅੰਕ().opAnd(ਨਕਾਬ_2(vtxp->lhsp()).ਅੰਕ(), ਨਕਾਬ_2(vtxp->rhsp()).ਅੰਕ());
    }
    ਖਾਲੀ ਦੇਖੋ(DfgOr* vtxp) override {  //
        ਨਕਾਬ_2(vtxp).ਅੰਕ().opAnd(ਨਕਾਬ_2(vtxp->lhsp()).ਅੰਕ(), ਨਕਾਬ_2(vtxp->rhsp()).ਅੰਕ());
    }
    ਖਾਲੀ ਦੇਖੋ(DfgXor* vtxp) override {  //
        ਨਕਾਬ_2(vtxp).ਅੰਕ().opAnd(ਨਕਾਬ_2(vtxp->lhsp()).ਅੰਕ(), ਨਕਾਬ_2(vtxp->rhsp()).ਅੰਕ());
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਜੋੜੋ* vtxp) override {
        V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
        m.opAnd(ਨਕਾਬ_2(vtxp->lhsp()).ਅੰਕ(), ਨਕਾਬ_2(vtxp->rhsp()).ਅੰਕ());
        floodTowardsMsb(m);
    }
    ਖਾਲੀ ਦੇਖੋ(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
        m.opAnd(ਨਕਾਬ_2(vtxp->lhsp()).ਅੰਕ(), ਨਕਾਬ_2(vtxp->rhsp()).ਅੰਕ());
        floodTowardsMsb(m);
    }

    ਖਾਲੀ ਦੇਖੋ(DfgRedAnd* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }
    ਖਾਲੀ ਦੇਖੋ(DfgRedOr* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }
    ਖਾਲੀ ਦੇਖੋ(DfgRedXor* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }

    ਖਾਲੀ ਦੇਖੋ(DfgEq* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes() && ਨਕਾਬ_2(vtxp->rhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }
    ਖਾਲੀ ਦੇਖੋ(DfgNeq* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes() && ਨਕਾਬ_2(vtxp->rhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }
    ਖਾਲੀ ਦੇਖੋ(DfgLt* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes() && ਨਕਾਬ_2(vtxp->rhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }
    ਖਾਲੀ ਦੇਖੋ(DfgLte* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes() && ਨਕਾਬ_2(vtxp->rhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }
    ਖਾਲੀ ਦੇਖੋ(DfgGt* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes() && ਨਕਾਬ_2(vtxp->rhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }
    ਖਾਲੀ ਦੇਖੋ(DfgGte* vtxp) override {
        ਅਚਲ ਤਾਰਕਿਕ independent = ਨਕਾਬ_2(vtxp->lhsp()).isOnes() && ਨਕਾਬ_2(vtxp->rhsp()).isOnes();
        ਜੇ (independent) ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_RS* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        ਅਚਲ uint32_t ਚੌੜਾਈ = vtxp->ਚੌੜਾਈ();

        // Constant shift can be computed precisely
        ਜੇ (DfgConst* ਅਚਲ rConstp = rhsp->cast<DfgConst>()) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amount = rConstp->ਮੰਜ਼ਿਲ_U32();
            ਜੇ (ਖਿਸਕਾਓ_Amount >= ਚੌੜਾਈ) {
                ਜੇ (ਨਕਾਬ_2(lhsp).ਅੰਕ().ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਚੌੜਾਈ - 1)) { ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones(); }
            } ਨਹੀਂਤਾਂ {
                V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
                m.op_ਖਿਸਕਾਓ_RS(ਨਕਾਬ_2(lhsp).ਅੰਕ(), rConstp->ਅੰਕ(), ਚੌੜਾਈ);
                m.op_ਸੈੱਟ_Range(ਚੌੜਾਈ - ਖਿਸਕਾਓ_Amount, ਖਿਸਕਾਓ_Amount, '1');
            }
            ਵਾਪਸ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ਜੇ (ਨਕਾਬ_2(rhsp).isOnes()) {
            V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
            m = ਨਕਾਬ_2(lhsp).ਅੰਕ();
            floodTowardsLsb(m);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_R* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        ਅਚਲ uint32_t ਚੌੜਾਈ = vtxp->ਚੌੜਾਈ();

        // Constant shift can be computed precisely
        ਜੇ (DfgConst* ਅਚਲ rConstp = rhsp->cast<DfgConst>()) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amount = rConstp->ਮੰਜ਼ਿਲ_U32();
            ਜੇ (ਖਿਸਕਾਓ_Amount >= ਚੌੜਾਈ) {
                ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
            } ਨਹੀਂਤਾਂ {
                V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
                m.op_ਖਿਸਕਾਓ_R(ਨਕਾਬ_2(lhsp).ਅੰਕ(), rConstp->ਅੰਕ());
                m.op_ਸੈੱਟ_Range(ਚੌੜਾਈ - ਖਿਸਕਾਓ_Amount, ਖਿਸਕਾਓ_Amount, '1');
            }
            ਵਾਪਸ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ਜੇ (ਨਕਾਬ_2(rhsp).isOnes()) {
            V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
            m = ਨਕਾਬ_2(lhsp).ਅੰਕ();
            floodTowardsLsb(m);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_L* vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        ਅਚਲ uint32_t ਚੌੜਾਈ = vtxp->ਚੌੜਾਈ();

        // Constant shift can be computed precisely
        ਜੇ (DfgConst* ਅਚਲ rConstp = rhsp->cast<DfgConst>()) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amount = rConstp->ਮੰਜ਼ਿਲ_U32();
            ਜੇ (ਖਿਸਕਾਓ_Amount >= ਚੌੜਾਈ) {
                ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones();
            } ਨਹੀਂਤਾਂ {
                V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
                m.op_ਖਿਸਕਾਓ_L(ਨਕਾਬ_2(lhsp).ਅੰਕ(), rConstp->ਅੰਕ());
                m.op_ਸੈੱਟ_Range(0, ਖਿਸਕਾਓ_Amount, '1');
            }
            ਵਾਪਸ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        ਜੇ (ਨਕਾਬ_2(rhsp).isOnes()) {
            V3_ਸੰਖਿਆ& m = ਨਕਾਬ_2(vtxp).ਅੰਕ();
            m = ਨਕਾਬ_2(lhsp).ਅੰਕ();
            floodTowardsMsb(m);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgCond* vtxp) override {
        ਜੇ (ਨਕਾਬ_2(vtxp->condp()).isOnes()) {
            ਨਕਾਬ_2(vtxp).ਅੰਕ().opAnd(ਨਕਾਬ_2(vtxp->thenp()).ਅੰਕ(), ਨਕਾਬ_2(vtxp->elsep()).ਅੰਕ());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਮੇਲ_Masked* vtxp) override {
        ਜੇ (ਨਕਾਬ_2(vtxp->lhsp()).isOnes() && ਨਕਾਬ_2(vtxp->matchp()).isOnes()) { ਨਕਾਬ_2(vtxp).ਸੈੱਟ_Ones(); }
    }

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ ਨਕਾਬ_2

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    ਖਾਲੀ ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ_Sinks(Dfg_ਸ਼ੀਰਸ਼& vtx) {
        vtx.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            ਜੇ (!m_sccInfo.get(sink)) ਵਾਪਸ ਝੂਠ;
            // Otherwise just enqueue it
            ਸ਼ੀਰਸ਼_State& state = m_vtxp2State.at(&sink);
            ਜੇ (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_work_ਕਤਾਰ.ਧੱਕੋ(state.m_rpo_ਸੰਖਿਆ);
            }
            state.m_isOnWorkList = ਸੱਚ;
            ਵਾਪਸ ਝੂਠ;
        });
    };

    ਖਾਲੀ dfs_Post_ਕ੍ਰਮ(std::unordered_ਸੈੱਟ<Dfg_ਸ਼ੀਰਸ਼*>& ਦੇਖਿਆ,  //
                      std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*>& post_ਕ੍ਰਮ_Enumeration,  //
                      Dfg_ਸ਼ੀਰਸ਼& vtx) {
        // Mark visited, skip if already visited
        ਜੇ (!ਦੇਖਿਆ.ਪਾਓ(&vtx).ਦੂਜਾ) ਵਾਪਸ;
        // Visit un-visited successors
        vtx.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& snk) {
            dfs_Post_ਕ੍ਰਮ(ਦੇਖਿਆ, post_ਕ੍ਰਮ_Enumeration, snk);
            ਵਾਪਸ ਝੂਠ;
        });
        // Add to post order enumeration
        post_ਕ੍ਰਮ_Enumeration.emplace_ਪਿੱਠ(&vtx);
    };

    Independent_ਬਿੱਟ(Dfg_ਗ੍ਰਾਫ& dfg, ਅਚਲ SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
        ਜੇ (v3Global.opt.debugCheck()) {
            m_ਸਤਰ_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.ਅਗੇਤਰ()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_ਅਧਾਰ::ਬਾਹਰਲਾ | std::ios_ਅਧਾਰ::app);
        }
#ਜੇ_ਅੰਤ

        // Compute Reverse-Postorder enumeration of vertices
        std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> rpoEnumeration;
        std::unordered_ਸੈੱਟ<Dfg_ਸ਼ੀਰਸ਼*> ਦੇਖਿਆ;
        dfg.for_Each_ਸ਼ੀਰਸ਼([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {
            ਜੇ (!vtx.nInputs()) dfs_Post_ਕ੍ਰਮ(ਦੇਖਿਆ, rpoEnumeration, vtx);
        });
        dfg.for_Each_ਸ਼ੀਰਸ਼([&](Dfg_ਸ਼ੀਰਸ਼& vtx) {  //
            dfs_Post_ਕ੍ਰਮ(ਦੇਖਿਆ, rpoEnumeration, vtx);
        });
        std::ਉਲਟ(rpoEnumeration.begin(), rpoEnumeration.ਸਿਰਾ());
        UASSERT(rpoEnumeration.ਮਾਪ() == dfg.ਮਾਪ(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        ਲਈ (ਮਾਪ_t i = 0; i < rpoEnumeration.ਮਾਪ(); ++i) {
            ਸ਼ੀਰਸ਼_State& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_ਸੰਖਿਆ = i;
            state.m_isOnWorkList = ਝੂਠ;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : rpoEnumeration) {
            ਜੇ (m_sccInfo.get(*vtxp)) ਜਾਰੀ;
            ਨਕਾਬ(*vtxp).ਸੈੱਟ_Ones();
        }
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : rpoEnumeration) {
            ਜੇ (!m_sccInfo.get(*vtxp)) ਜਾਰੀ;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << ਨਕਾਬ(*vtxp).ਮੰਜ਼ਿਲ_String());
            ਜੇ (!ਨਕਾਬ(*vtxp).isZero()) ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ_Sinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        ਜਦੋਂ (!m_work_ਕਤਾਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            // Grab next item
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ currp = rpoEnumeration[m_work_ਕਤਾਰ.ਸਿਖਰ()];
            m_work_ਕਤਾਰ.ਕੱਢੋ();
            m_vtxp2State.at(currp).m_isOnWorkList = ਝੂਠ;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& curr_ਨਕਾਬ = ਨਕਾਬ(*currp);
            // Remember current mask so we can check if it changed
            ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ prev_ਨਕਾਬ = curr_ਨਕਾਬ;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            ਜੇ (prev_ਨਕਾਬ != curr_ਨਕਾਬ) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prev_ਨਕਾਬ.ਮੰਜ਼ਿਲ_String()
                             << "\n      to: " << curr_ਨਕਾਬ.ਮੰਜ਼ਿਲ_String());
                ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ_Sinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                ਜੇ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!curr_ਨਕਾਬ.is_Contraction_ਦਾ(prev_ਨਕਾਬ), currp,
                                "Mask should only expand");
                }
            }
        }
    }

ਜਨਤਕ:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    ਸਥਿਰ std::unordered_map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ> apply(Dfg_ਗ੍ਰਾਫ& dfg,
                                                               ਅਚਲ SccInfo& sccInfo) {
        ਵਾਪਸ std::move(Independent_ਬਿੱਟ{dfg, sccInfo}.m_vtxp2_ਨਕਾਬ);
    }
};

ਵਰਗ Fix_ਉੱਪਰ final {
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ਅਚਲ std::unordered_map<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ> m_independent_ਬਿੱਟ
        = Independent_ਬਿੱਟ::apply(m_dfg, m_sccInfo);
    ਮਾਪ_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ਸਥਿਰ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ compute_ਵਰਤਿਆ_ਬਿੱਟ(Dfg_ਸ਼ੀਰਸ਼& vtx) {
        ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ ਨਤੀਜਾ{vtx};
        vtx.foreachSink([&ਨਤੀਜਾ](Dfg_ਸ਼ੀਰਸ਼& sink) {
            // If used via a Sel, mark the selected bits used
            ਜੇ (ਅਚਲ DfgSel* ਅਚਲ selp = sink.cast<DfgSel>()) {
                ਨਤੀਜਾ.ਅੰਕ().op_ਸੈੱਟ_Range(selp->lsb(), selp->ਚੌੜਾਈ(), '1');
                ਵਾਪਸ ਝੂਠ;
            }
            // Used without a Sel, so all bits are used
            ਨਤੀਜਾ.ਸੈੱਟ_Ones();
            ਵਾਪਸ ਸੱਚ;
        });
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    ਸਥਿਰ std::string debugStr(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {  // LCOV_EXCL_START
        ਜੇ (ਅਚਲ DfgArraySel* ਅਚਲ aselp = vtx.cast<DfgArraySel>()) {
            ਅਚਲ ਮਾਪ_t i = aselp->bitp()->as<DfgConst>()->ਮੰਜ਼ਿਲ_ਮਾਪ_T();
            ਵਾਪਸ debugStr(*aselp->fromp()) + "[" + std::ਮੰਜ਼ਿਲ_string(i) + "]";
        }
        ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtx.cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
            ਵਾਪਸ varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    ਖਾਲੀ gatherTerms(std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*>& termps, Dfg_ਸ਼ੀਰਸ਼& vtx, ਅਚਲ V3_ਸੰਖਿਆ& indp_ਬਿੱਟ,
                     ਅਚਲ uint32_t hi, ਅਚਲ uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        ਤਾਰਕਿਕ isIndependent = indp_ਬਿੱਟ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(lo);
        uint32_t lsb = lo;
        ਲਈ (uint32_t msb = lo; msb <= hi; ++msb) {
            ਅਚਲ ਤਾਰਕਿਕ ਸਿਰਾ_Range = msb == hi || isIndependent != indp_ਬਿੱਟ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(msb + 1);
            ਜੇ (!ਸਿਰਾ_Range) ਜਾਰੀ;
            ਅਚਲ uint32_t ਚੌੜਾਈ = msb - lsb + 1;
            // Compute the term to use for this range
            ਜੇ (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_ਪਿੱਠ(m_traceDriver.apply(vtx, lsb, ਚੌੜਾਈ));
            } ਨਹੀਂਤਾਂ {
                // If dependent, fall back on using the part of the variable
                DfgSel* ਅਚਲ selp = ਨਵਾਂ DfgSel{m_dfg, vtx.fileline(), Dfg_ਅੰਕੜੇ_Type::packed(ਚੌੜਾਈ)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.ਜੋੜੋ(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_ਪਿੱਠ(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    ਖਾਲੀ fix_ਉੱਪਰ_Packed(Dfg_ਸ਼ੀਰਸ਼& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ& indp_ਬਿੱਟ = m_independent_ਬਿੱਟ.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indp_ਬਿੱਟ.ਮੰਜ਼ਿਲ_String());
        // Can't do anything if all bits are dependent
        ਜੇ (indp_ਬਿੱਟ.isZero()) ਵਾਪਸ;

        // Figure out which bits of 'vtxp' are used
        ਅਚਲ ਦੋਆਧਾਰੀ_ਅੰਕ_ਨਕਾਬ ਵਰਤਿਆ_ਬਿੱਟ = compute_ਵਰਤਿਆ_ਬਿੱਟ(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << ਵਰਤਿਆ_ਬਿੱਟ.ਮੰਜ਼ਿਲ_String());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_ਸੰਖਿਆ ਵਰਤਿਆ_And_Independent{vtx.fileline(), static_cast<ਪੂਰਨ>(vtx.ਚੌੜਾਈ()), 0};
        ਵਰਤਿਆ_And_Independent.opAnd(ਵਰਤਿਆ_ਬਿੱਟ.ਅੰਕ(), indp_ਬਿੱਟ.ਅੰਕ());
        ਜੇ (ਵਰਤਿਆ_And_Independent.isEqZero()) ਵਾਪਸ;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> termps;

        // Iterate through consecutive used/unused ranges
        File_ਸਤਰ* ਅਚਲ flp = vtx.fileline();
        ਅਚਲ uint32_t ਚੌੜਾਈ = vtx.ਚੌੜਾਈ();
        ਤਾਰਕਿਕ is_ਵਰਤਿਆ = ਵਰਤਿਆ_ਬਿੱਟ.ਅੰਕ().ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        ਲਈ (uint32_t msb = 0; msb < ਚੌੜਾਈ; ++msb) {
            ਅਚਲ ਤਾਰਕਿਕ ਸਿਰਾ_Range = msb == ਚੌੜਾਈ - 1 || is_ਵਰਤਿਆ != ਵਰਤਿਆ_ਬਿੱਟ.ਅੰਕ().ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(msb + 1);
            ਜੇ (!ਸਿਰਾ_Range) ਜਾਰੀ;
            ਜੇ (is_ਵਰਤਿਆ) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indp_ਬਿੱਟ.ਅੰਕ(), msb, lsb);
            } ਨਹੀਂਤਾਂ {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ਅਚਲ constp = ਨਵਾਂ DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.ਜੋੜੋ(*constp, 0);
                termps.emplace_ਪਿੱਠ(constp);
            }
            // Next iteration
            is_ਵਰਤਿਆ = !is_ਵਰਤਿਆ;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        Dfg_ਸ਼ੀਰਸ਼* replacementp = termps.front();
        ਅਚਲ uint64_t vComp = m_sccInfo.get(vtx);
        ਲਈ (ਮਾਪ_t i = 1; i < termps.ਮਾਪ(); ++i) {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ termp = termps[i];
            ਅਚਲ uint32_t cat_ਚੌੜਾਈ = replacementp->ਚੌੜਾਈ() + termp->ਚੌੜਾਈ();
            DfgConcat* ਅਚਲ catp = ਨਵਾਂ DfgConcat{m_dfg, flp, Dfg_ਅੰਕੜੇ_Type::packed(cat_ਚੌੜਾਈ)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ਅਚਲ uint64_t tComp = m_sccInfo.get(*termp);
            ਅਚਲ uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.ਜੋੜੋ(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ termp : termps) {
            ਜੇ (DfgSel* ਅਚਲ selp = termp->cast<DfgSel>()) {
                ਜੇ (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        ਜੇ (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    ਖਾਲੀ ਮੁੱਖ(Dfg_ਸ਼ੀਰਸ਼_Var& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        ਜੇ (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fix_ਉੱਪਰ_Packed(var);
        } ਨਹੀਂਤਾਂ ਜੇ (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ਅਚਲ uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) {
                // Ignore if sink is not part of same cycle
                ਜੇ (m_sccInfo.get(sink) != component) ਵਾਪਸ ਝੂਠ;
                // Only handle ArraySels with constant index
                DfgArraySel* ਅਚਲ aselp = sink.cast<DfgArraySel>();
                ਜੇ (!aselp) ਵਾਪਸ ਝੂਠ;
                ਜੇ (!aselp->bitp()->is<DfgConst>()) ਵਾਪਸ ਝੂਠ;
                // Fix up the word
                fix_ਉੱਪਰ_Packed(*aselp);
                ਵਾਪਸ ਝੂਠ;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    Fix_ਉੱਪਰ(Dfg_ਗ੍ਰਾਫ& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

ਜਨਤਕ:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ਸਥਿਰ ਮਾਪ_t apply(Dfg_ਗ੍ਰਾਫ& dfg, SccInfo& sccInfo) {
        ਜੇ (!sccInfo.isCyclic()) ਵਾਪਸ 0;

        Fix_ਉੱਪਰ fix_ਉੱਪਰ{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : dfg.varVertices()) {
            ਜੇ (!sccInfo.get(vtx)) ਜਾਰੀ;
            fix_ਉੱਪਰ.ਮੁੱਖ(vtx);
            ਜੇ (!sccInfo.isCyclic()) ਰੋਕੋ;
        }
        // Return the number of improvements made
        ਵਾਪਸ fix_ਉੱਪਰ.m_nImprovements;
    }
};

ਖਾਲੀ break_ਚੱਕਰਾਂ(Dfg_ਗ੍ਰਾਫ& dfg, V3_Dfg_Break_ਚੱਕਰਾਂ_Context& ctx) {
    // Shorthand for dumping graph at given dump level
    ਅਚਲ ਸਵੈਚਾਲਿਤ dump = [&](ਪੂਰਨ level, ਅਚਲ Dfg_ਗ੍ਰਾਫ& dfg, ਅਚਲ std::string& name) {
        ਜੇ (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    ਮਾਪ_t nImprovements = 0;
    ਮਾਪ_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    ਕਰੋ {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ਅਚਲ ਮਾਪ_t nFixed = Fix_ਉੱਪਰ::apply(dfg, sccInfo);
        ਜੇ (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        ਜੇ (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        ਜੇ (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            ਵਾਪਸ;
        }
    } ਜਦੋਂ (nImprovements != prevNImprovements);

    // Debug dump
    ਜੇ (dumpDfgLevel() >= 9) {
        ਅਚਲ SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
            ਵਾਪਸ sccInfo.get(vtx);  //
        });
    }

    // Accounting
    ਜੇ (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } ਨਹੀਂਤਾਂ {
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
    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼_Var*> varps;
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : dfg.varVertices()) {
        ਜੇ (!vtx.tmpForp()) varps.ਧੱਕੋ_ਪਿੱਠ(&vtx);
    }
    std::stable_ਹਿੱਸੇ_ਕਰੋ(varps.begin(), varps.ਸਿਰਾ(), [](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* varp) {
        ਵਾਪਸ varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp : varps) {
        // Stop as soon as it becomes acyclic
        ਜੇ (!sccInfo.isCyclic()) ਰੋਕੋ;

        // Ignore if variable is not part of a cycle
        ਜੇ (!sccInfo.get(*varp)) ਜਾਰੀ;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* ਅਚਲ prevp = ਨਵਾਂ DfgPrev{dfg, varp->vscp()};
        sccInfo.ਜੋੜੋ(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    ਜੇ (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
ਖਾਲੀ V3DfgPasses::break_ਚੱਕਰਾਂ(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgContext& ctx) {
    V3_Dfg_Break_ਚੱਕਰਾਂ::break_ਚੱਕਰਾਂ(dfg, ctx.m_break_ਚੱਕਰਾਂ_Context);
    ਜੇ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::remove_ਅਣਵਰਤਿਆ(dfg);
}
