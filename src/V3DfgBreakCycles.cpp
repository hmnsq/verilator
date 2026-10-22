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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Hash.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <fstream>
#ಅನ್ತರ್ಭಾವ <queue>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ನಾಮಾಕಾಶ V3_Dfg_Break_ಚಕ್ರಗಳು {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
ವರ್ಗ SccInfo final {
    Dfg_ನಕ್ಷೆ& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, uint64_t> m_vtx2Scc;
    ಗಾತ್ರ_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

ಸಾರ್ವಜನಿಕ:
    SccInfo(Dfg_ನಕ್ಷೆ& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
            ಸ್ಥಿರಾಂಕ uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            ಆದರೆ (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    ತಾರ್ಕಿಕ isCyclic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    ಶೂನ್ಯ ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx, uint64_t sccIdx) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ new_ನಮೂದು = m_vtx2Scc.emplace(&vtx, sccIdx).ಎರಡನೇ;
        UASSERT_OBJ(new_ನಮೂದು, &vtx, "Vertex already inserted");
        ಆದರೆ (sccIdx) ++m_nCyclicVertices;
    }

    ತಾರ್ಕಿಕ stillCyclicFwd(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) ಸ್ಥಿರಾಂಕ {
        // If it only reads non-cyclic vertices, it has also become acyclic
        ಹಿಂತಿರುಗಿ vtx.foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {  //
            ಹಿಂತಿರುಗಿ m_vtx2Scc.at(&src);
        });
    }

    ತಾರ್ಕಿಕ stillCyclicBwd(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) ಸ್ಥಿರಾಂಕ {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        ಹಿಂತಿರುಗಿ vtx.foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& dst) {  //
            ಹಿಂತಿರುಗಿ m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    ಶೂನ್ಯ updateAcyclic(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& dst) {
            // Sink was known to be acyclic, stop
            ಆದರೆ (!m_vtx2Scc.at(&dst)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (!stillCyclicFwd(dst)) updateAcyclic(dst);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        // Propagate the update through the graph backward
        vtx.foreach_ಮೂಲ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src) {
            // Source was known to be acyclic, stop
            ಆದರೆ (!m_vtx2Scc.at(&src)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (!stillCyclicBwd(src)) updateAcyclic(src);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    ಶೂನ್ಯ updateReplacement(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx, Dfg_ಶೃಂಗ& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        ಆದರೆ (!get(replacement)) {
            replacement.foreachSink([&](Dfg_ಶೃಂಗ& dst) {
                ಆದರೆ (!get(dst)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                ಆದರೆ (!stillCyclicFwd(dst)) updateAcyclic(dst);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
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
    ಶೂನ್ಯ validate() ಸ್ಥಿರಾಂಕ {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

ವರ್ಗ TraceDriver final : ಸಾರ್ವಜನಿಕ DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    ರಚನೆ Cache_ಕೀಲಿ final {
        Dfg_ಶೃಂಗ* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_ಕೀಲಿ() = ಅಳಿಸು;
        Cache_ಕೀಲಿ(Dfg_ಶೃಂಗ* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        ರಚನೆ Hash final {
            ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Cache_ಕೀಲಿ& ಅಂಶ_2) ಸ್ಥಿರಾಂಕ {
                // cppcheck-suppress unreadVariable
                V3Hash hash{ಅಂಶ_2.m_vtxp};
                hash += ಅಂಶ_2.m_lsb;
                hash += ಅಂಶ_2.m_msb;
                ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
            }
        };

        ರಚನೆ Equal final {
            ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Cache_ಕೀಲಿ& a, ಸ್ಥಿರಾಂಕ Cache_ಕೀಲಿ& b) ಸ್ಥಿರಾಂಕ {
                ಹಿಂತಿರುಗಿ a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
            }
        };
    };

    // STATE
    Dfg_ನಕ್ಷೆ& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    // The strongly connected component we are currently trying to escape
    uint64_t m_component = 0;
    uint32_t m_lsb = 0;  // LSB to extract from the currently visited Vertex
    uint32_t m_msb = 0;  // MSB to extract from the currently visited Vertex
    std::ಸದಿಶ<uint32_t> m_idxs;  // Indices to extract from the currently visited Vertex
    std::ಸದಿಶ<Dfg_ಶೃಂಗ**> m_pendingps;  // Pending pass-through cache entries across trace calls
    // Result of tracing the currently visited Vertex. Use RETURN_RESULT below!
    Dfg_ಶೃಂಗ* m_resp = ಶೂನ್ಯಸೂಚಕ;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    Dfg_ಶೃಂಗ* m_tailp = ಶೂನ್ಯಸೂಚಕ;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    Dfg_ಶೃಂಗ* m_defaultp = ಶೂನ್ಯಸೂಚಕ;
    Dfg_ಶೃಂಗ* m_splicep = ಶೂನ್ಯಸೂಚಕ;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_ಕೀಲಿ, Dfg_ಶೃಂಗ*, Cache_ಕೀಲಿ::Hash, Cache_ಕೀಲಿ::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_ಕೀಲಿ, Dfg_ಶೃಂಗ*, Cache_ಕೀಲಿ::Hash, Cache_ಕೀಲಿ::Equal> m_boundaryCache;

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
    std::ofstream m_ಸಾಲು_Coverage_File;  // Line coverage file, just for testing
#ಆದರೆ_ಅಂತ್ಯ

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    ಶೃಂಗ_2* make(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* refp, uint32_t ಅಗಲ) {
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ, ಶೃಂಗ_2>::ಮೌಲ್ಯ  //
                          && !std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_Var, ಶೃಂಗ_2>::ಮೌಲ್ಯ,
                      "Should only make operation vertices and constants");

        ಆದರೆ VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, ಶೃಂಗ_2>::ಮೌಲ್ಯ) {
            DfgConst* ಸ್ಥಿರಾಂಕ vtxp = ಹೊಸ DfgConst{m_dfg, refp->fileline(), ಅಗಲ, 0};
            m_sccInfo.ಕೂಡಿಸು(*vtxp, 0);
            ಹಿಂತಿರುಗಿ reinterpret_cast<ಶೃಂಗ_2*>(vtxp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            ಬಳಸಿ Vtx = ಬಗೆ_ಹೆಸರು std::conditional<std::is_same<DfgConst, ಶೃಂಗ_2>::ಮೌಲ್ಯ, DfgSel,
                                                  ಶೃಂಗ_2>::type;
            Vtx* ಸ್ಥಿರಾಂಕ vtxp = ಹೊಸ Vtx{m_dfg, refp->fileline(), Dfg_ದತ್ತಾಂಶ_Type::packed(ಅಗಲ)};
            m_sccInfo.ಕೂಡಿಸು(*vtxp, 0);
            ಹಿಂತಿರುಗಿ reinterpret_cast<ಶೃಂಗ_2*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    Dfg_ಶೃಂಗ_Var* createTmp(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪೂರ್ವಪ್ರತ್ಯಯ, Dfg_ಶೃಂಗ* vtxp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
        Dfg_ಶೃಂಗ::ScopeCache scopeCache;
        AstScope* ಸ್ಥಿರಾಂಕ scopep = vtxp->scopep(scopeCache);
        Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = m_dfg.makeNewVar(flp, ಪೂರ್ವಪ್ರತ್ಯಯ, vtxp->dtype(), scopep);
        varp->vscp()->varp()->is_ಆಂತರಿಕ(ನಿಜ);
        varp->tmpForp(varp->vscp());
        m_sccInfo.ಕೂಡಿಸು(*varp, 0);
        ಹಿಂತಿರುಗಿ varp;
    }

    // Trace drivers of the given packed vertex, at the given bit range.
    Dfg_ಶೃಂಗ* trace(Dfg_ಶೃಂಗ* vtxp, uint32_t msb, uint32_t lsb) {
        // Many vertices just pass the traced bit range through to one of their sources
        // (see RETURN_RESULT_TAIL). Chains of such vertices can be tens of thousands of
        // vertices long in large designs (e.g.: a wide concatenation), so resolve them
        // in this loop instead of by recursion, which would overflow the C++ stack.
        // 'm_pendingps' holds the cache entries of the vertices on the current chain. They
        // all yield the same result, which is filled in when the chain is resolved.
        // Note: references into 'm_cache' are stable across the insertions below.
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಆರಂಭ_Vtxp = vtxp;
        ಸ್ಥಿರಾಂಕ uint32_t ಆರಂಭ_Msb = msb;
        ಸ್ಥಿರಾಂಕ uint32_t ಆರಂಭ_Lsb = lsb;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t pendingBegin = m_pendingps.ಗಾತ್ರ_2();
        Dfg_ಶೃಂಗ* resp = ಶೂನ್ಯಸೂಚಕ;

        ಆಗ (ನಿಜ) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->ಗಾತ್ರ_2() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            ಆದರೆ (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                Dfg_ಶೃಂಗ*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(ಶೂನ್ಯಸೂಚಕ))
                          .ಮೊದಲ->ಎರಡನೇ;
                ಆದರೆ (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    ಆದರೆ (Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ splicep = respr->cast<Dfg_ಶೃಂಗ_Splice>()) {
                        Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    ಆದರೆ (msb != respr->ಅಗಲ() - 1 || lsb != 0) {
                        DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                ನಿಲ್ಲಿಸು;
            }

            // Normal cache for intra-component traces
            Dfg_ಶೃಂಗ*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(ಶೂನ್ಯಸೂಚಕ))
                                    .ಮೊದಲ->ಎರಡನೇ;

            // Trace the vertex
            ಆದರೆ (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Otherwise visit the vertex to trace it
                VL_RESTORER(m_msb);
                VL_RESTORER(m_lsb);
                VL_RESTORER_CLEAR(m_idxs);
                VL_RESTORER(m_resp);
                VL_RESTORER(m_tailp);
                m_msb = msb;
                m_lsb = lsb;
                m_resp = ಶೂನ್ಯಸೂಚಕ;
                m_tailp = ಶೂನ್ಯಸೂಚಕ;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != ಶೂನ್ಯಸೂಚಕ) != (m_tailp != ಶೂನ್ಯಸೂಚಕ), vtxp,
                            "Expected exactly one trace result");
                ಆದರೆ (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ತಳ್ಳು_ಹಿಂಭಾಗ(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    ಮುಂದುವರಿ;
                }
                respr = m_resp;
            }
            resp = respr;
            ನಿಲ್ಲಿಸು;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, ಆರಂಭ_Vtxp, "Tracing driver failed for " << ಆರಂಭ_Vtxp->typeName());
        UASSERT_OBJ(resp->ಅಗಲ() == (ಆರಂಭ_Msb - ಆರಂಭ_Lsb + 1), ಆರಂಭ_Vtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        ಗಾಗಿ (ಗಾತ್ರ_t i = pendingBegin; i < m_pendingps.ಗಾತ್ರ_2(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        ಹಿಂತಿರುಗಿ resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    Dfg_ಶೃಂಗ* traceSameIdx(Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.ಖಾಲಿಗಳು(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = ಶೂನ್ಯಸೂಚಕ;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->ಅಗಲ() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        ಹಿಂತಿರುಗಿ m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    Dfg_ಶೃಂಗ* trace_ತಳ್ಳು_Idx(Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp, ಸ್ಥಿರಾಂಕ uint32_t idx) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nIdxs = m_idxs.ಗಾತ್ರ_2();
        m_idxs.ತಳ್ಳು_ಹಿಂಭಾಗ(idx);
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ resp = traceSameIdx(vtxp);
        m_idxs.ತೆಗೆ_ಹಿಂಭಾಗ();
        UASSERT_OBJ(m_idxs.ಗಾತ್ರ_2() == nIdxs, vtxp, "Index stack size mismatch");
        ಹಿಂತಿರುಗಿ resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    Dfg_ಶೃಂಗ* trace_ತೆಗೆ_Idx(Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp) {
        UASSERT_OBJ(!m_idxs.ಖಾಲಿಗಳು(), vtxp, "Should have a pending index");
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nIdxs = m_idxs.ಗಾತ್ರ_2();
        ಸ್ಥಿರಾಂಕ uint32_t idx = m_idxs.ಹಿಂಭಾಗ();
        m_idxs.ತೆಗೆ_ಹಿಂಭಾಗ();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ resp = m_idxs.ಖಾಲಿಗಳು() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ತಳ್ಳು_ಹಿಂಭಾಗ(idx);
        UASSERT_OBJ(m_idxs.ಗಾತ್ರ_2() == nIdxs, vtxp, "Index stack size mismatch");
        ಹಿಂತಿರುಗಿ resp;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    ಶೃಂಗ_2* trace_Bitwise_ದ್ವಿಮಾನ(ಶೃಂಗ_2* vtxp) {
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ, ಶೃಂಗ_2>::ಮೌಲ್ಯ,
                      "Should only call on DfgVertexBinary");
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ resp = make<ಶೃಂಗ_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        ಹಿಂತಿರುಗಿ resp;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    Dfg_ಶೃಂಗ* trace_ಕೂಡಿಸು_Sub(ಶೃಂಗ_2* vtxp) {
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ, ಶೃಂಗ_2>::ಮೌಲ್ಯ,
                      "Should only call on DfgVertexBinary");
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ opp = make<ಶೃಂಗ_2>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        ಹಿಂತಿರುಗಿ selp;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    ಶೃಂಗ_2* traceReduction(ಶೃಂಗ_2* vtxp) {
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_Unary, ಶೃಂಗ_2>::ಮೌಲ್ಯ,
                      "Should only call on DfgVertexUnary");
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ resp = make<ಶೃಂಗ_2>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->ಅಗಲ() - 1, 0));
        ಹಿಂತಿರುಗಿ resp;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    ಶೃಂಗ_2* traceCmp(ಶೃಂಗ_2* vtxp) {
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ, ಶೃಂಗ_2>::ಮೌಲ್ಯ,
                      "Should only call on DfgVertexBinary");
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ resp = make<ಶೃಂಗ_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ಅಗಲ() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ಅಗಲ() - 1, 0));
        ಹಿಂತಿರುಗಿ resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
#ವ್ಯಾಖ್ಯಾನಿಸು TRACE_ಫಲಿತಾಂಶ_COVER() \
    ಮಾಡು { \
        ಆದರೆ (VL_UNLIKELY(m_ಸಾಲು_Coverage_File.is_open())) m_ಸಾಲು_Coverage_File << __LINE__ << '\n'; \
    } ಆಗ (ಸುಳ್ಳು)
#ಇಲ್ಲದಿದ್ದರೆ
#ವ್ಯಾಖ್ಯಾನಿಸು TRACE_ಫಲಿತಾಂಶ_COVER() \
    ಮಾಡು { \
    } ಆಗ (ಸುಳ್ಳು)
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು RETURN_ಫಲಿತಾಂಶ(vtxp) \
    ಮಾಡು { \
        m_resp = (vtxp); \
        TRACE_ಫಲಿತಾಂಶ_COVER(); \
        ಹಿಂತಿರುಗಿ; \
    } ಆಗ (ಸುಳ್ಳು)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#ವ್ಯಾಖ್ಯಾನಿಸು RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(srcp, smsb, slsb) \
    ಮಾಡು { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ tailp = (srcp); \
        ಸ್ಥಿರಾಂಕ uint32_t ಬಾಲ_Msb = (smsb); \
        ಸ್ಥಿರಾಂಕ uint32_t ಬಾಲ_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = ಬಾಲ_Msb; \
        m_lsb = ಬಾಲ_Lsb; \
        TRACE_ಫಲಿತಾಂಶ_COVER(); \
        ಹಿಂತಿರುಗಿ; \
    } ಆಗ (ಸುಳ್ಳು)

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಶೃಂಗ* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ಶೂನ್ಯ ಭೇಟಿ(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        Dfg_ಶೃಂಗ* defaultp = m_defaultp;
        m_defaultp = ಶೂನ್ಯಸೂಚಕ;
        m_splicep = ಶೂನ್ಯಸೂಚಕ;

        ರಚನೆ Driver final {
            Dfg_ಶೃಂಗ* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = ಅಳಿಸು;
            Driver(Dfg_ಶೃಂಗ* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::ಸದಿಶ<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        ತಾರ್ಕಿಕ try_ಸಂಪೂರ್ಣ_Default = defaultp;
        Dfg_ಶೃಂಗ* coverp = ಶೂನ್ಯಸೂಚಕ;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](Dfg_ಶೃಂಗ& src, uint32_t lsb) {
            ಸ್ಥಿರಾಂಕ uint32_t msb = lsb + src.ಅಗಲ() - 1;
            drivers.emplace_ಹಿಂಭಾಗ(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            ಆದರೆ (m_msb >= lsb && msb >= m_lsb) try_ಸಂಪೂರ್ಣ_Default = ಸುಳ್ಳು;
            // If it does not cover the whole searched bit range, move on
            ಆದರೆ (m_lsb < lsb || msb < m_msb) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        });

        // Trace the driver covering the whole searched range, if there is one
        ಆದರೆ (coverp) RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        ಆದರೆ (try_ಸಂಪೂರ್ಣ_Default) RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::ವಿಂಗಡಿಸು(drivers.begin(), drivers.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ Driver& a, ಸ್ಥಿರಾಂಕ Driver& b) {  //
            ಹಿಂತಿರುಗಿ a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::ಸದಿಶ<Dfg_ಶೃಂಗ*> termps;
        uint32_t lsb = m_lsb;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            ಆದರೆ (lsb > driver.m_msb) ಮುಂದುವರಿ;
            // Driver is above the searched MSB, done
            ಆದರೆ (driver.m_lsb > m_msb) ನಿಲ್ಲಿಸು;
            // Gap below this driver, trace default to fill it
            ಆದರೆ (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_ಹಿಂಭಾಗ(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ಸ್ಥಿರಾಂಕ uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_ಹಿಂಭಾಗ(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        ಆದರೆ (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_ಹಿಂಭಾಗ(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.ಗಾತ್ರ_2() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        Dfg_ಶೃಂಗ* resp = termps.front();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < termps.ಗಾತ್ರ_2(); ++i) {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ termp = termps[i];
            DfgConcat* ಸ್ಥಿರಾಂಕ catp = make<DfgConcat>(termp, resp->ಅಗಲ() + termp->ಅಗಲ());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_ಫಲಿತಾಂಶ(resp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ defaultp = m_defaultp;
        m_defaultp = ಶೂನ್ಯಸೂಚಕ;
        m_splicep = ಶೂನ್ಯಸೂಚಕ;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ಸ್ಥಿರಾಂಕ uint32_t idx = m_idxs.ಹಿಂಭಾಗ();
        ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ driverp = vtxp->driverAt(idx)) {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = driverp->as<DfgUnitArray>()->srcp();
            ಆದರೆ (srcp->is<Dfg_ಶೃಂಗ_Splice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_ಫಲಿತಾಂಶ(trace_ತೆಗೆ_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_ಫಲಿತಾಂಶ(traceSameIdx(defaultp));
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಶೃಂಗ_Var* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ defaultp = vtxp->defaultp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        ಆದರೆ (srcp && srcp->is<Dfg_ಶೃಂಗ_Splice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_ಫಲಿತಾಂಶ(m_idxs.ಖಾಲಿಗಳು() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_ಫಲಿತಾಂಶ(trace_ತಳ್ಳು_Idx(vtxp->fromp(), idxp->ಗಮ್ಯ_U32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.ಖಾಲಿಗಳು(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ಸ್ಥಿರಾಂಕ resp = make<DfgArraySel>(vtxp, vtxp->ಅಗಲ());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->ಅಗಲ() - 1, 0));
        DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_ಫಲಿತಾಂಶ(selp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.ಹಿಂಭಾಗ() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_ಫಲಿತಾಂಶ(trace_ತೆಗೆ_Idx(vtxp->srcp()));
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgConcat* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        ಸ್ಥಿರಾಂಕ uint32_t r_ಅಗಲ = rhsp->ಅಗಲ();
        // If the traced bits are wholly in the RHS
        ಆದರೆ (r_ಅಗಲ > m_msb) RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        ಆದರೆ (m_lsb >= r_ಅಗಲ) RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(lhsp, m_msb - r_ಅಗಲ, m_lsb - r_ಅಗಲ);
        // The traced bit spans both sides, trace both
        DfgConcat* ಸ್ಥಿರಾಂಕ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, r_ಅಗಲ - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - r_ಅಗಲ, 0));
        RETURN_ಫಲಿತಾಂಶ(resp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgRep* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        ಸ್ಥಿರಾಂಕ uint32_t s_ಅಗಲ = srcp->ಅಗಲ();
        // If we need more bits than the source, then we need the whole source
        ಆದರೆ (m_msb - m_lsb + 1 > s_ಅಗಲ) {
            DfgRep* ಸ್ಥಿರಾಂಕ repp = make<DfgRep>(vtxp, vtxp->ಅಗಲ());
            repp->srcp(trace(srcp, s_ಅಗಲ - 1, 0));
            DfgSel* ಸ್ಥಿರಾಂಕ resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_ಫಲಿತಾಂಶ(resp);
        }
        // If the requested bits are within the same repliacted word
        ಆದರೆ (m_msb / s_ಅಗಲ == m_lsb / s_ಅಗಲ) {
            RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(srcp, m_msb % s_ಅಗಲ, m_lsb % s_ಅಗಲ);
        }
        // The requested bits span two replicated words
        DfgConcat* ಸ್ಥಿರಾಂಕ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, s_ಅಗಲ - 1, m_lsb % s_ಅಗಲ));
        resp->lhsp(trace(srcp, m_msb % s_ಅಗಲ, 0));
        RETURN_ಫಲಿತಾಂಶ(resp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgExtend* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        ಸ್ಥಿರಾಂಕ uint32_t s_ಅಗಲ = srcp->ಅಗಲ();
        // If the traced bits are wholly in the input
        ಆದರೆ (s_ಅಗಲ > m_msb) RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ಆದರೆ (m_lsb >= s_ಅಗಲ) RETURN_ಫಲಿತಾಂಶ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ಸ್ಥಿರಾಂಕ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_ಅಗಲ - 1, m_lsb));
        RETURN_ಫಲಿತಾಂಶ(resp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgExtendS* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        ಸ್ಥಿರಾಂಕ uint32_t s_ಅಗಲ = srcp->ಅಗಲ();
        // If the traced bits are wholly in the input
        ಆದರೆ (s_ಅಗಲ > m_msb) RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ಆದರೆ (m_lsb >= s_ಅಗಲ) {
            ಆದರೆ (m_msb == m_lsb) RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(srcp, s_ಅಗಲ - 1, s_ಅಗಲ - 1);
            DfgExtendS* ಸ್ಥಿರಾಂಕ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, s_ಅಗಲ - 1, s_ಅಗಲ - 1));
            RETURN_ಫಲಿತಾಂಶ(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ಸ್ಥಿರಾಂಕ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_ಅಗಲ - 1, m_lsb));
        RETURN_ಫಲಿತಾಂಶ(resp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgSel* vtxp) override {
        ಸ್ಥಿರಾಂಕ uint32_t lsb = vtxp->lsb();
        RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgNot* vtxp) override {
        DfgNot* ಸ್ಥಿರಾಂಕ resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_ಫಲಿತಾಂಶ(resp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgAnd* vtxp) override { RETURN_ಫಲಿತಾಂಶ(trace_Bitwise_ದ್ವಿಮಾನ(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgOr* vtxp) override { RETURN_ಫಲಿತಾಂಶ(trace_Bitwise_ದ್ವಿಮಾನ(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgXor* vtxp) override { RETURN_ಫಲಿತಾಂಶ(trace_Bitwise_ದ್ವಿಮಾನ(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಕೂಡಿಸು* vtxp) override { RETURN_ಫಲಿತಾಂಶ(trace_ಕೂಡಿಸು_Sub(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgSub* vtxp) override { RETURN_ಫಲಿತಾಂಶ(trace_ಕೂಡಿಸು_Sub(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgRedAnd* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceReduction(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgRedOr* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceReduction(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgRedXor* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceReduction(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgEq* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceCmp(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgNeq* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceCmp(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgLt* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceCmp(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgLte* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceCmp(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgGt* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceCmp(vtxp)); }
    ಶೂನ್ಯ ಭೇಟಿ(DfgGte* vtxp) override { RETURN_ಫಲಿತಾಂಶ(traceCmp(vtxp)); }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_RS* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amnt = rConstp->ಗಮ್ಯ_U32();
            // Width of lower half of result
            ಸ್ಥಿರಾಂಕ uint32_t ಅಧೋ_ಅಗಲ = ಸರಿಸು_Amnt > vtxp->ಅಗಲ() ? 0 : vtxp->ಅಗಲ() - ಸರಿಸು_Amnt;
            // If the traced bits are wholly in the input
            ಆದರೆ (ಅಧೋ_ಅಗಲ > m_msb) {
                RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(lhsp, m_msb + ಸರಿಸು_Amnt, m_lsb + ಸರಿಸು_Amnt);
            }
            // If the traced bits are wholly in the extension
            ಆದರೆ (m_lsb >= ಅಧೋ_ಅಗಲ) {
                DfgExtendS* ಸ್ಥಿರಾಂಕ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->ಅಗಲ() - 1, lhsp->ಅಗಲ() - 1));
                RETURN_ಫಲಿತಾಂಶ(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ಸ್ಥಿರಾಂಕ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, ಅಧೋ_ಅಗಲ - 1 + ಸರಿಸು_Amnt, m_lsb + ಸರಿಸು_Amnt));
            RETURN_ಫಲಿತಾಂಶ(resp);
        }

        Dfg_ಸರಿಸು_RS* ಸ್ಥಿರಾಂಕ shiftrsp = make<Dfg_ಸರಿಸು_RS>(vtxp, vtxp->lhsp()->ಅಗಲ() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ಅಗಲ() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ಅಗಲ() - 1, m_lsb));
        ಆದರೆ (m_msb == vtxp->lhsp()->ಅಗಲ() - 1) RETURN_ಫಲಿತಾಂಶ(shiftrsp);
        DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_ಫಲಿತಾಂಶ(selp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_R* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amnt = rConstp->ಗಮ್ಯ_U32();
            // Width of lower half of result
            ಸ್ಥಿರಾಂಕ uint32_t ಅಧೋ_ಅಗಲ = ಸರಿಸು_Amnt > vtxp->ಅಗಲ() ? 0 : vtxp->ಅಗಲ() - ಸರಿಸು_Amnt;
            // If the traced bits are wholly in the input
            ಆದರೆ (ಅಧೋ_ಅಗಲ > m_msb) {
                RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(lhsp, m_msb + ಸರಿಸು_Amnt, m_lsb + ಸರಿಸು_Amnt);
            }
            // If the traced bits are wholly in the extension
            ಆದರೆ (m_lsb >= ಅಧೋ_ಅಗಲ) RETURN_ಫಲಿತಾಂಶ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ಸ್ಥಿರಾಂಕ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, ಅಧೋ_ಅಗಲ - 1 + ಸರಿಸು_Amnt, m_lsb + ಸರಿಸು_Amnt));
            RETURN_ಫಲಿತಾಂಶ(resp);
        }

        Dfg_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ shiftrp = make<Dfg_ಸರಿಸು_R>(vtxp, vtxp->lhsp()->ಅಗಲ() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ಅಗಲ() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ಅಗಲ() - 1, m_lsb));
        ಆದರೆ (m_msb == vtxp->lhsp()->ಅಗಲ() - 1) RETURN_ಫಲಿತಾಂಶ(shiftrp);
        DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_ಫಲಿತಾಂಶ(selp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_L* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amnt = rConstp->ಗಮ್ಯ_U32();
            // Width of lower half of result
            ಸ್ಥಿರಾಂಕ uint32_t ಅಧೋ_ಅಗಲ = ಸರಿಸು_Amnt > vtxp->ಅಗಲ() ? vtxp->ಅಗಲ() : ಸರಿಸು_Amnt;
            // If the traced bits are wholly in the input
            ಆದರೆ (m_lsb >= ಅಧೋ_ಅಗಲ) {
                RETURN_ಫಲಿತಾಂಶ_ಬಾಲ(lhsp, m_msb - ಸರಿಸು_Amnt, m_lsb - ಸರಿಸು_Amnt);
            }
            // If the traced bits are wholly in the extension
            ಆದರೆ (ಅಧೋ_ಅಗಲ > m_msb) RETURN_ಫಲಿತಾಂಶ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ಸ್ಥಿರಾಂಕ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->ಅಗಲ() - (m_msb - ಅಧೋ_ಅಗಲ + 1)));
            resp->lhsp(trace(lhsp, m_msb - ಸರಿಸು_Amnt, ಅಧೋ_ಅಗಲ - ಸರಿಸು_Amnt));
            RETURN_ಫಲಿತಾಂಶ(resp);
        }

        Dfg_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ shiftlp = make<Dfg_ಸರಿಸು_L>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ಅಗಲ() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        ಆದರೆ (m_lsb == 0) RETURN_ಫಲಿತಾಂಶ(shiftlp);
        DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_ಫಲಿತಾಂಶ(selp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgCond* vtxp) override {
        DfgCond* ಸ್ಥಿರಾಂಕ resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->ಅಗಲ() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_ಫಲಿತಾಂಶ(resp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಹೊಂದಿಕೆ_Masked* vtxp) override {
        Dfg_ಹೊಂದಿಕೆ_Masked* ಸ್ಥಿರಾಂಕ resp = make<Dfg_ಹೊಂದಿಕೆ_Masked>(vtxp, vtxp->ಅಗಲ());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ಅಗಲ() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->ಅಗಲ() - 1, 0));
        DfgSel* ಸ್ಥಿರಾಂಕ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_ಫಲಿತಾಂಶ(selp);
    }

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು RETURN_ಫಲಿತಾಂಶ
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು TRACE_ಫಲಿತಾಂಶ_COVER
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು RETURN_ಫಲಿತಾಂಶ_ಬಾಲ

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    TraceDriver(Dfg_ನಕ್ಷೆ& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
        ಆದರೆ (v3Global.opt.debugCheck()) {
            m_ಸಾಲು_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_ತಳ::ಹೊರಹೋಗು | std::ios_ತಳ::app);
        }
#ಆದರೆ_ಅಂತ್ಯ
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    Dfg_ಶೃಂಗ* apply(Dfg_ಶೃಂಗ& vtx, uint32_t lsb, uint32_t ಅಗಲ) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        ಹಿಂತಿರುಗಿ trace(&vtx, lsb + ಅಗಲ - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
ವರ್ಗ ಬಿಟ್_ಮುಖವಾಡ final {
    // TYPES
    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& m_dtype;
    ಒಕ್ಕೂಟ {
        V3_ಸಂಖ್ಯೆ m_ಅಂಕೆ;  // The bits, if packed
        std::ಸದಿಶ<ಬಿಟ್_ಮುಖವಾಡ> m_sub;  // The per element masks, if aggregate
    };

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಬಿಟ್_ಮುಖವಾಡ(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        ಆದರೆ (m_dtype.isPacked()) {
            ಹೊಸ (&m_ಅಂಕೆ) V3_ಸಂಖ್ಯೆ{flp, static_cast<ಪೂರ್ಣಾಂಕ>(m_dtype.ಗಾತ್ರ_2()), 0};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹೊಸ (&m_sub) std::ಸದಿಶ<ಬಿಟ್_ಮುಖವಾಡ>{};
            m_sub.reserve(m_dtype.ಗಾತ್ರ_2());
            ಗಾಗಿ (uint32_t i = 0; i < m_dtype.ಗಾತ್ರ_2(); ++i)
                m_sub.emplace_ಹಿಂಭಾಗ(flp, m_dtype.elemDtype());
        }
    }
    ಬಿಟ್_ಮುಖವಾಡ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx)
        : ಬಿಟ್_ಮುಖವಾಡ{vtx.fileline(), vtx.dtype()} {}
    ಬಿಟ್_ಮುಖವಾಡ(ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& other)
        : m_dtype{other.m_dtype} {
        ಆದರೆ (m_dtype.isPacked()) {
            ಹೊಸ (&m_ಅಂಕೆ) V3_ಸಂಖ್ಯೆ{other.m_ಅಂಕೆ};
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹೊಸ (&m_sub) std::ಸದಿಶ<ಬಿಟ್_ಮುಖವಾಡ>{other.m_sub};
        }
    }
    ಬಿಟ್_ಮುಖವಾಡ& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& other) {
        ಆದರೆ (ಇದು != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            ಆದರೆ (other.m_dtype.isPacked()) {
                m_ಅಂಕೆ = other.m_ಅಂಕೆ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_sub = other.m_sub;
            }
        }
        ಹಿಂತಿರುಗಿ *ಇದು;
    }
    ~ಬಿಟ್_ಮುಖವಾಡ() {
        ಆದರೆ (m_dtype.isPacked()) {
            m_ಅಂಕೆ.~V3_ಸಂಖ್ಯೆ();
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_sub.~ಸದಿಶ<ಬಿಟ್_ಮುಖವಾಡ>();
        }
    }

    // METHODS
    V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ಹಿಂತಿರುಗಿ m_ಅಂಕೆ;
    }
    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() ಸ್ಥಿರಾಂಕ {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ಹಿಂತಿರುಗಿ m_ಅಂಕೆ;
    }
    std::ಸದಿಶ<ಬಿಟ್_ಮುಖವಾಡ>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        ಹಿಂತಿರುಗಿ m_sub;
    }

    ತಾರ್ಕಿಕ isZero() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_dtype.isPacked()) ಹಿಂತಿರುಗಿ m_ಅಂಕೆ.isEqZero();

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& sub : m_sub) {
            ಆದರೆ (!sub.isZero()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ತಾರ್ಕಿಕ isOnes() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_dtype.isPacked()) ಹಿಂತಿರುಗಿ m_ಅಂಕೆ.isEqAllOnes();

        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& sub : m_sub) {
            ಆದರೆ (!sub.isOnes()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಶೂನ್ಯ ಗಣ_Zero() {
        ಆದರೆ (m_dtype.isPacked()) {
            m_ಅಂಕೆ.ಗಣ_All_Bits0();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (ಬಿಟ್_ಮುಖವಾಡ& sub : m_sub) sub.ಗಣ_Zero();
        }
    }
    ಶೂನ್ಯ ಗಣ_Ones() {
        ಆದರೆ (m_dtype.isPacked()) {
            m_ಅಂಕೆ.ಗಣ_All_Bits1();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (ಬಿಟ್_ಮುಖವಾಡ& sub : m_sub) sub.ಗಣ_Ones();
        }
    }

    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& other) ಸ್ಥಿರಾಂಕ {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        ಆದರೆ (m_dtype.isPacked()) ಹಿಂತಿರುಗಿ m_ಅಂಕೆ.isCaseEq(other.m_ಅಂಕೆ);
        ಹಿಂತಿರುಗಿ m_sub == other.m_sub;
    }
    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ!=(ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !(*ಇದು == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    ತಾರ್ಕಿಕ is_Contraction_ದ(ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& that) ಸ್ಥಿರಾಂಕ {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        ಆದರೆ (m_dtype.isPacked()) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಪದಗಳು = VL_ಪದಗಳು_I(m_dtype.ಗಾತ್ರ_2());
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಪದಗಳು; ++i) {
                ಆದರೆ (~m_ಅಂಕೆ.edata_ಶಬ್ದ(i) & that.m_ಅಂಕೆ.edata_ಶಬ್ದ(i)) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < m_sub.ಗಾತ್ರ_2(); ++i) {
            ಆದರೆ (m_sub[i].is_Contraction_ದ(that.m_sub[i])) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    std::string ಗಮ್ಯ_String() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_dtype.isPacked()) ಹಿಂತಿರುಗಿ "0x" + m_ಅಂಕೆ.displayed(m_ಅಂಕೆ.fileline(), "%x");
        std::string ಫಲಿತಾಂಶ;
        ಫಲಿತಾಂಶ += "{";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& sub : m_sub) {
            ಆದರೆ (&sub != &m_sub.front()) ಫಲಿತಾಂಶ += ", ";
            ಫಲಿತಾಂಶ += sub.ಗಮ್ಯ_String();
        }
        ಫಲಿತಾಂಶ += "}";
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
};

ವರ್ಗ Independent_ಬಿಟ್ಟುಗಳು final : ಸಾರ್ವಜನಿಕ DfgVisitor {
    // TYPES
    ರಚನೆ ಶೃಂಗ_State final {
        ಗಾತ್ರ_t m_rpo_ಸಂಖ್ಯೆ = 0;  // Reverse Postorder Number of vertex
        ತಾರ್ಕಿಕ m_isOnWorkList = ಸುಳ್ಳು;  // Whether the vertex is on the work list
    };

    // STATE
    ಸ್ಥಿರಾಂಕ SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, ಬಿಟ್_ಮುಖವಾಡ> m_vtxp2_ಮುಖವಾಡ;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::ಆದ್ಯತೆ_ಸರತಿ<ಗಾತ್ರ_t, std::ಸದಿಶ<ಗಾತ್ರ_t>, std::greater<ಗಾತ್ರ_t>> m_work_ಸರತಿ;
    std::unordered_map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, ಶೃಂಗ_State> m_vtxp2State;  // State of each vertex

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
    std::ofstream m_ಸಾಲು_Coverage_File;  // Line coverage file, just for testing
#ಆದರೆ_ಅಂತ್ಯ

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    ಬಿಟ್_ಮುಖವಾಡ& ಮುಖವಾಡ(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        // Look up (or create) mask for 'vtx'
        ಹಿಂತಿರುಗಿ m_vtxp2_ಮುಖವಾಡ
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .ಮೊದಲ->ಎರಡನೇ;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
#ವ್ಯಾಖ್ಯಾನಿಸು ಮುಖವಾಡ_2(vtxp) \
    ([ಇದು](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) -> ಬಿಟ್_ಮುಖವಾಡ& { \
        ಆದರೆ (VL_UNLIKELY(m_ಸಾಲು_Coverage_File.is_open())) m_ಸಾಲು_Coverage_File << __LINE__ << '\n'; \
        ಹಿಂತಿರುಗಿ ಮುಖವಾಡ(vtx); \
    }(*vtxp))
#ಇಲ್ಲದಿದ್ದರೆ
#ವ್ಯಾಖ್ಯಾನಿಸು ಮುಖವಾಡ_2(vtxp) ಮುಖವಾಡ(*vtxp)
#ಆದರೆ_ಅಂತ್ಯ

    // Clear all bits at or below the most significant clear bit
    ಶೂನ್ಯ floodTowardsLsb(V3_ಸಂಖ್ಯೆ& ಅಂಕೆ) {
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = ಅಂಕೆ.ಅಗಲ() - 1; i >= 0; --i) {
            ಆದರೆ (ಅಂಕೆ.ಬಿಟ್_Is1(i)) ಮುಂದುವರಿ;
            ಅಂಕೆ.op_ಗಣ_Range(0, i + 1, '0');
            ನಿಲ್ಲಿಸು;
        }
    }

    // Clear all bits at or above the least significant clear bit
    ಶೂನ್ಯ floodTowardsMsb(V3_ಸಂಖ್ಯೆ& ಅಂಕೆ) {
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಅಂಕೆ.ಅಗಲ(); ++i) {
            ಆದರೆ (ಅಂಕೆ.ಬಿಟ್_Is1(i)) ಮುಂದುವರಿ;
            ಅಂಕೆ.op_ಗಣ_Range(i, ಅಂಕೆ.ಅಗಲ() - i, '0');
            ನಿಲ್ಲಿಸು;
        }
    }

    ಶೂನ್ಯ propagateFromDriver(ಬಿಟ್_ಮುಖವಾಡ& m, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* srcp) {
        // If there is no driver, we are done
        ಆದರೆ (!srcp) ಹಿಂತಿರುಗಿ;
        // If it is driven by a splice, we need to combine the masks of the drivers
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSplicePacked* ಸ್ಥಿರಾಂಕ splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src, uint32_t lo) {
                m.ಅಂಕೆ().opSelInto(ಮುಖವಾಡ_2(&src).ಅಂಕೆ(), lo, src.ಅಗಲ());
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSpliceArray* ಸ್ಥಿರಾಂಕ splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& src, uint32_t lo) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = ಮುಖವಾಡ_2(uap->srcp());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಹಿಂತಿರುಗಿ;
        }
        // Otherwise, we just use the mask of the single driver
        m = ಮುಖವಾಡ_2(srcp);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಶೃಂಗ* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಶೃಂಗ_Var* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        ಆದರೆ (vtxp->isVolatile()) ಹಿಂತಿರುಗಿ;

        ಬಿಟ್_ಮುಖವಾಡ& m = ಮುಖವಾಡ_2(vtxp);
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        ಆದರೆ (defaultp) m = ಮುಖವಾಡ_2(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಶೃಂಗ_Splice* vtxp) override {
        propagateFromDriver(ಮುಖವಾಡ_2(vtxp), vtxp);  // Needed to continue traversal
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgUnitArray* vtxp) override { ಮುಖವಾಡ_2(vtxp).sub().at(0) = ಮುಖವಾಡ_2(vtxp->srcp()); }

    ಶೂನ್ಯ ಭೇಟಿ(DfgArraySel* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ idxp = vtxp->bitp()->cast<DfgConst>()) {
            ಮುಖವಾಡ_2(vtxp) = ಮುಖವಾಡ_2(vtxp->fromp()).sub().at(idxp->ಗಮ್ಯ_ಗಾತ್ರ_T());
            ಹಿಂತಿರುಗಿ;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        ಆದರೆ (ಮುಖವಾಡ_2(vtxp->bitp()).isOnes() && ಮುಖವಾಡ_2(fromp).isOnes()) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgConcat* vtxp) override {
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
        m.opSelInto(ಮುಖವಾಡ_2(rhsp).ಅಂಕೆ(), 0, rhsp->ಅಗಲ());
        m.opSelInto(ಮುಖವಾಡ_2(lhsp).ಅಂಕೆ(), rhsp->ಅಗಲ(), lhsp->ಅಗಲ());
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgRep* vtxp) override {
        ಸ್ಥಿರಾಂಕ uint32_t ಎಣಿಕೆ_2 = vtxp->ಎಣಿಕೆ_2();
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        ಸ್ಥಿರಾಂಕ uint32_t s_ಅಗಲ = srcp->ಅಗಲ();
        V3_ಸಂಖ್ಯೆ& v_ಮುಖವಾಡ = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
        V3_ಸಂಖ್ಯೆ& s_ಮುಖವಾಡ = ಮುಖವಾಡ_2(srcp).ಅಂಕೆ();
        ಗಾಗಿ (uint32_t i = 0; i < ಎಣಿಕೆ_2; ++i) v_ಮುಖವಾಡ.opSelInto(s_ಮುಖವಾಡ, i * s_ಅಗಲ, s_ಅಗಲ);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgSel* vtxp) override {
        ಸ್ಥಿರಾಂಕ uint32_t lsb = vtxp->lsb();
        ಸ್ಥಿರಾಂಕ uint32_t msb = lsb + vtxp->ಅಗಲ() - 1;
        ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ().opSel(ಮುಖವಾಡ_2(vtxp->fromp()).ಅಂಕೆ(), msb, lsb);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgExtend* vtxp) override {
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        ಸ್ಥಿರಾಂಕ uint32_t s_ಅಗಲ = srcp->ಅಗಲ();
        V3_ಸಂಖ್ಯೆ& s = ಮುಖವಾಡ_2(srcp).ಅಂಕೆ();
        V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
        m.opSelInto(s, 0, s_ಅಗಲ);
        m.op_ಗಣ_Range(s_ಅಗಲ, vtxp->ಅಗಲ() - s_ಅಗಲ, '1');
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgExtendS* vtxp) override {
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        ಸ್ಥಿರಾಂಕ uint32_t s_ಅಗಲ = srcp->ಅಗಲ();
        V3_ಸಂಖ್ಯೆ& s = ಮುಖವಾಡ_2(srcp).ಅಂಕೆ();
        V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
        m.opSelInto(s, 0, s_ಅಗಲ);
        m.op_ಗಣ_Range(s_ಅಗಲ, vtxp->ಅಗಲ() - s_ಅಗಲ, s.ಬಿಟ್_Is0(s_ಅಗಲ - 1) ? '0' : '1');
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgNot* vtxp) override {  //
        ಮುಖವಾಡ_2(vtxp) = ಮುಖವಾಡ_2(vtxp->srcp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgAnd* vtxp) override {  //
        ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ().opAnd(ಮುಖವಾಡ_2(vtxp->lhsp()).ಅಂಕೆ(), ಮುಖವಾಡ_2(vtxp->rhsp()).ಅಂಕೆ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgOr* vtxp) override {  //
        ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ().opAnd(ಮುಖವಾಡ_2(vtxp->lhsp()).ಅಂಕೆ(), ಮುಖವಾಡ_2(vtxp->rhsp()).ಅಂಕೆ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgXor* vtxp) override {  //
        ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ().opAnd(ಮುಖವಾಡ_2(vtxp->lhsp()).ಅಂಕೆ(), ಮುಖವಾಡ_2(vtxp->rhsp()).ಅಂಕೆ());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಕೂಡಿಸು* vtxp) override {
        V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
        m.opAnd(ಮುಖವಾಡ_2(vtxp->lhsp()).ಅಂಕೆ(), ಮುಖವಾಡ_2(vtxp->rhsp()).ಅಂಕೆ());
        floodTowardsMsb(m);
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
        m.opAnd(ಮುಖವಾಡ_2(vtxp->lhsp()).ಅಂಕೆ(), ಮುಖವಾಡ_2(vtxp->rhsp()).ಅಂಕೆ());
        floodTowardsMsb(m);
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgRedAnd* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgRedOr* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgRedXor* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgEq* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes() && ಮುಖವಾಡ_2(vtxp->rhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgNeq* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes() && ಮುಖವಾಡ_2(vtxp->rhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgLt* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes() && ಮುಖವಾಡ_2(vtxp->rhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgLte* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes() && ಮುಖವಾಡ_2(vtxp->rhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgGt* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes() && ಮುಖವಾಡ_2(vtxp->rhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }
    ಶೂನ್ಯ ಭೇಟಿ(DfgGte* vtxp) override {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ independent = ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes() && ಮುಖವಾಡ_2(vtxp->rhsp()).isOnes();
        ಆದರೆ (independent) ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_RS* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = vtxp->ಅಗಲ();

        // Constant shift can be computed precisely
        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->cast<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amount = rConstp->ಗಮ್ಯ_U32();
            ಆದರೆ (ಸರಿಸು_Amount >= ಅಗಲ) {
                ಆದರೆ (ಮುಖವಾಡ_2(lhsp).ಅಂಕೆ().ಬಿಟ್_Is1(ಅಗಲ - 1)) { ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones(); }
            } ಇಲ್ಲದಿದ್ದರೆ {
                V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
                m.op_ಸರಿಸು_RS(ಮುಖವಾಡ_2(lhsp).ಅಂಕೆ(), rConstp->ಅಂಕೆ(), ಅಗಲ);
                m.op_ಗಣ_Range(ಅಗಲ - ಸರಿಸು_Amount, ಸರಿಸು_Amount, '1');
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ಆದರೆ (ಮುಖವಾಡ_2(rhsp).isOnes()) {
            V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
            m = ಮುಖವಾಡ_2(lhsp).ಅಂಕೆ();
            floodTowardsLsb(m);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_R* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = vtxp->ಅಗಲ();

        // Constant shift can be computed precisely
        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->cast<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amount = rConstp->ಗಮ್ಯ_U32();
            ಆದರೆ (ಸರಿಸು_Amount >= ಅಗಲ) {
                ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
            } ಇಲ್ಲದಿದ್ದರೆ {
                V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
                m.op_ಸರಿಸು_R(ಮುಖವಾಡ_2(lhsp).ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                m.op_ಗಣ_Range(ಅಗಲ - ಸರಿಸು_Amount, ಸರಿಸು_Amount, '1');
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ಆದರೆ (ಮುಖವಾಡ_2(rhsp).isOnes()) {
            V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
            m = ಮುಖವಾಡ_2(lhsp).ಅಂಕೆ();
            floodTowardsLsb(m);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_L* vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = vtxp->ಅಗಲ();

        // Constant shift can be computed precisely
        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->cast<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amount = rConstp->ಗಮ್ಯ_U32();
            ಆದರೆ (ಸರಿಸು_Amount >= ಅಗಲ) {
                ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones();
            } ಇಲ್ಲದಿದ್ದರೆ {
                V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
                m.op_ಸರಿಸು_L(ಮುಖವಾಡ_2(lhsp).ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                m.op_ಗಣ_Range(0, ಸರಿಸು_Amount, '1');
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        ಆದರೆ (ಮುಖವಾಡ_2(rhsp).isOnes()) {
            V3_ಸಂಖ್ಯೆ& m = ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ();
            m = ಮುಖವಾಡ_2(lhsp).ಅಂಕೆ();
            floodTowardsMsb(m);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgCond* vtxp) override {
        ಆದರೆ (ಮುಖವಾಡ_2(vtxp->condp()).isOnes()) {
            ಮುಖವಾಡ_2(vtxp).ಅಂಕೆ().opAnd(ಮುಖವಾಡ_2(vtxp->thenp()).ಅಂಕೆ(), ಮುಖವಾಡ_2(vtxp->elsep()).ಅಂಕೆ());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಹೊಂದಿಕೆ_Masked* vtxp) override {
        ಆದರೆ (ಮುಖವಾಡ_2(vtxp->lhsp()).isOnes() && ಮುಖವಾಡ_2(vtxp->matchp()).isOnes()) { ಮುಖವಾಡ_2(vtxp).ಗಣ_Ones(); }
    }

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು ಮುಖವಾಡ_2

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    ಶೂನ್ಯ ಸರತಿಗೆ_ಸೇರಿಸು_Sinks(Dfg_ಶೃಂಗ& vtx) {
        vtx.foreachSink([&](Dfg_ಶೃಂಗ& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            ಆದರೆ (!m_sccInfo.get(sink)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Otherwise just enqueue it
            ಶೃಂಗ_State& state = m_vtxp2State.at(&sink);
            ಆದರೆ (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_work_ಸರತಿ.ತಳ್ಳು(state.m_rpo_ಸಂಖ್ಯೆ);
            }
            state.m_isOnWorkList = ನಿಜ;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    };

    ಶೂನ್ಯ dfs_Post_ಕ್ರಮ(std::unordered_ಗಣ<Dfg_ಶೃಂಗ*>& ಭೇಟಿಯಾದ,  //
                      std::ಸದಿಶ<Dfg_ಶೃಂಗ*>& post_ಕ್ರಮ_Enumeration,  //
                      Dfg_ಶೃಂಗ& vtx) {
        // Mark visited, skip if already visited
        ಆದರೆ (!ಭೇಟಿಯಾದ.ಸೇರಿಸು(&vtx).ಎರಡನೇ) ಹಿಂತಿರುಗಿ;
        // Visit un-visited successors
        vtx.foreachSink([&](Dfg_ಶೃಂಗ& snk) {
            dfs_Post_ಕ್ರಮ(ಭೇಟಿಯಾದ, post_ಕ್ರಮ_Enumeration, snk);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        // Add to post order enumeration
        post_ಕ್ರಮ_Enumeration.emplace_ಹಿಂಭಾಗ(&vtx);
    };

    Independent_ಬಿಟ್ಟುಗಳು(Dfg_ನಕ್ಷೆ& dfg, ಸ್ಥಿರಾಂಕ SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
        ಆದರೆ (v3Global.opt.debugCheck()) {
            m_ಸಾಲು_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_ತಳ::ಹೊರಹೋಗು | std::ios_ತಳ::app);
        }
#ಆದರೆ_ಅಂತ್ಯ

        // Compute Reverse-Postorder enumeration of vertices
        std::ಸದಿಶ<Dfg_ಶೃಂಗ*> rpoEnumeration;
        std::unordered_ಗಣ<Dfg_ಶೃಂಗ*> ಭೇಟಿಯಾದ;
        dfg.for_Each_ಶೃಂಗ([&](Dfg_ಶೃಂಗ& vtx) {
            ಆದರೆ (!vtx.nInputs()) dfs_Post_ಕ್ರಮ(ಭೇಟಿಯಾದ, rpoEnumeration, vtx);
        });
        dfg.for_Each_ಶೃಂಗ([&](Dfg_ಶೃಂಗ& vtx) {  //
            dfs_Post_ಕ್ರಮ(ಭೇಟಿಯಾದ, rpoEnumeration, vtx);
        });
        std::ಹಿಮ್ಮುಖ(rpoEnumeration.begin(), rpoEnumeration.ಕೊನೆ());
        UASSERT(rpoEnumeration.ಗಾತ್ರ_2() == dfg.ಗಾತ್ರ_2(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < rpoEnumeration.ಗಾತ್ರ_2(); ++i) {
            ಶೃಂಗ_State& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_ಸಂಖ್ಯೆ = i;
            state.m_isOnWorkList = ಸುಳ್ಳು;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : rpoEnumeration) {
            ಆದರೆ (m_sccInfo.get(*vtxp)) ಮುಂದುವರಿ;
            ಮುಖವಾಡ(*vtxp).ಗಣ_Ones();
        }
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : rpoEnumeration) {
            ಆದರೆ (!m_sccInfo.get(*vtxp)) ಮುಂದುವರಿ;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << ಮುಖವಾಡ(*vtxp).ಗಮ್ಯ_String());
            ಆದರೆ (!ಮುಖವಾಡ(*vtxp).isZero()) ಸರತಿಗೆ_ಸೇರಿಸು_Sinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        ಆಗ (!m_work_ಸರತಿ.ಖಾಲಿಗಳು()) {
            // Grab next item
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ currp = rpoEnumeration[m_work_ಸರತಿ.ತುದಿ()];
            m_work_ಸರತಿ.ತೆಗೆ_2();
            m_vtxp2State.at(currp).m_isOnWorkList = ಸುಳ್ಳು;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& curr_ಮುಖವಾಡ = ಮುಖವಾಡ(*currp);
            // Remember current mask so we can check if it changed
            ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ prev_ಮುಖವಾಡ = curr_ಮುಖವಾಡ;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            ಆದರೆ (prev_ಮುಖವಾಡ != curr_ಮುಖವಾಡ) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prev_ಮುಖವಾಡ.ಗಮ್ಯ_String()
                             << "\n      to: " << curr_ಮುಖವಾಡ.ಗಮ್ಯ_String());
                ಸರತಿಗೆ_ಸೇರಿಸು_Sinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                ಆದರೆ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!curr_ಮುಖವಾಡ.is_Contraction_ದ(prev_ಮುಖವಾಡ), currp,
                                "Mask should only expand");
                }
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    ಸ್ಥಿರ std::unordered_map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, ಬಿಟ್_ಮುಖವಾಡ> apply(Dfg_ನಕ್ಷೆ& dfg,
                                                               ಸ್ಥಿರಾಂಕ SccInfo& sccInfo) {
        ಹಿಂತಿರುಗಿ std::move(Independent_ಬಿಟ್ಟುಗಳು{dfg, sccInfo}.m_vtxp2_ಮುಖವಾಡ);
    }
};

ವರ್ಗ Fix_ಮೇಲೆ final {
    Dfg_ನಕ್ಷೆ& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ಸ್ಥಿರಾಂಕ std::unordered_map<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, ಬಿಟ್_ಮುಖವಾಡ> m_independent_ಬಿಟ್ಟುಗಳು
        = Independent_ಬಿಟ್ಟುಗಳು::apply(m_dfg, m_sccInfo);
    ಗಾತ್ರ_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ಸ್ಥಿರ ಬಿಟ್_ಮುಖವಾಡ compute_ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು(Dfg_ಶೃಂಗ& vtx) {
        ಬಿಟ್_ಮುಖವಾಡ ಫಲಿತಾಂಶ{vtx};
        vtx.foreachSink([&ಫಲಿತಾಂಶ](Dfg_ಶೃಂಗ& sink) {
            // If used via a Sel, mark the selected bits used
            ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSel* ಸ್ಥಿರಾಂಕ selp = sink.cast<DfgSel>()) {
                ಫಲಿತಾಂಶ.ಅಂಕೆ().op_ಗಣ_Range(selp->lsb(), selp->ಅಗಲ(), '1');
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            // Used without a Sel, so all bits are used
            ಫಲಿತಾಂಶ.ಗಣ_Ones();
            ಹಿಂತಿರುಗಿ ನಿಜ;
        });
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    ಸ್ಥಿರ std::string debugStr(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {  // LCOV_EXCL_START
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgArraySel* ಸ್ಥಿರಾಂಕ aselp = vtx.cast<DfgArraySel>()) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t i = aselp->bitp()->as<DfgConst>()->ಗಮ್ಯ_ಗಾತ್ರ_T();
            ಹಿಂತಿರುಗಿ debugStr(*aselp->fromp()) + "[" + std::ಗಮ್ಯ_string(i) + "]";
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtx.cast<Dfg_ಶೃಂಗ_Var>()) {
            ಹಿಂತಿರುಗಿ varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    ಶೂನ್ಯ gatherTerms(std::ಸದಿಶ<Dfg_ಶೃಂಗ*>& termps, Dfg_ಶೃಂಗ& vtx, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& indp_ಬಿಟ್ಟುಗಳು,
                     ಸ್ಥಿರಾಂಕ uint32_t hi, ಸ್ಥಿರಾಂಕ uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        ತಾರ್ಕಿಕ isIndependent = indp_ಬಿಟ್ಟುಗಳು.ಬಿಟ್_Is1(lo);
        uint32_t lsb = lo;
        ಗಾಗಿ (uint32_t msb = lo; msb <= hi; ++msb) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಕೊನೆ_Range = msb == hi || isIndependent != indp_ಬಿಟ್ಟುಗಳು.ಬಿಟ್_Is1(msb + 1);
            ಆದರೆ (!ಕೊನೆ_Range) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = msb - lsb + 1;
            // Compute the term to use for this range
            ಆದರೆ (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_ಹಿಂಭಾಗ(m_traceDriver.apply(vtx, lsb, ಅಗಲ));
            } ಇಲ್ಲದಿದ್ದರೆ {
                // If dependent, fall back on using the part of the variable
                DfgSel* ಸ್ಥಿರಾಂಕ selp = ಹೊಸ DfgSel{m_dfg, vtx.fileline(), Dfg_ದತ್ತಾಂಶ_Type::packed(ಅಗಲ)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.ಕೂಡಿಸು(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_ಹಿಂಭಾಗ(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    ಶೂನ್ಯ fix_ಮೇಲೆ_Packed(Dfg_ಶೃಂಗ& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ& indp_ಬಿಟ್ಟುಗಳು = m_independent_ಬಿಟ್ಟುಗಳು.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indp_ಬಿಟ್ಟುಗಳು.ಗಮ್ಯ_String());
        // Can't do anything if all bits are dependent
        ಆದರೆ (indp_ಬಿಟ್ಟುಗಳು.isZero()) ಹಿಂತಿರುಗಿ;

        // Figure out which bits of 'vtxp' are used
        ಸ್ಥಿರಾಂಕ ಬಿಟ್_ಮುಖವಾಡ ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು = compute_ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು.ಗಮ್ಯ_String());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_ಸಂಖ್ಯೆ ಬಳಸಿದ_And_Independent{vtx.fileline(), static_cast<ಪೂರ್ಣಾಂಕ>(vtx.ಅಗಲ()), 0};
        ಬಳಸಿದ_And_Independent.opAnd(ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು.ಅಂಕೆ(), indp_ಬಿಟ್ಟುಗಳು.ಅಂಕೆ());
        ಆದರೆ (ಬಳಸಿದ_And_Independent.isEqZero()) ಹಿಂತಿರುಗಿ;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::ಸದಿಶ<Dfg_ಶೃಂಗ*> termps;

        // Iterate through consecutive used/unused ranges
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtx.fileline();
        ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = vtx.ಅಗಲ();
        ತಾರ್ಕಿಕ is_ಬಳಸಿದ = ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು.ಅಂಕೆ().ಬಿಟ್_Is1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        ಗಾಗಿ (uint32_t msb = 0; msb < ಅಗಲ; ++msb) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಕೊನೆ_Range = msb == ಅಗಲ - 1 || is_ಬಳಸಿದ != ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು.ಅಂಕೆ().ಬಿಟ್_Is1(msb + 1);
            ಆದರೆ (!ಕೊನೆ_Range) ಮುಂದುವರಿ;
            ಆದರೆ (is_ಬಳಸಿದ) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indp_ಬಿಟ್ಟುಗಳು.ಅಂಕೆ(), msb, lsb);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ಸ್ಥಿರಾಂಕ constp = ಹೊಸ DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.ಕೂಡಿಸು(*constp, 0);
                termps.emplace_ಹಿಂಭಾಗ(constp);
            }
            // Next iteration
            is_ಬಳಸಿದ = !is_ಬಳಸಿದ;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        Dfg_ಶೃಂಗ* replacementp = termps.front();
        ಸ್ಥಿರಾಂಕ uint64_t vComp = m_sccInfo.get(vtx);
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < termps.ಗಾತ್ರ_2(); ++i) {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ termp = termps[i];
            ಸ್ಥಿರಾಂಕ uint32_t cat_ಅಗಲ = replacementp->ಅಗಲ() + termp->ಅಗಲ();
            DfgConcat* ಸ್ಥಿರಾಂಕ catp = ಹೊಸ DfgConcat{m_dfg, flp, Dfg_ದತ್ತಾಂಶ_Type::packed(cat_ಅಗಲ)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ಸ್ಥಿರಾಂಕ uint64_t tComp = m_sccInfo.get(*termp);
            ಸ್ಥಿರಾಂಕ uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.ಕೂಡಿಸು(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ termp : termps) {
            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = termp->cast<DfgSel>()) {
                ಆದರೆ (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        ಆದರೆ (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    ಶೂನ್ಯ ಮುಖ್ಯ(Dfg_ಶೃಂಗ_Var& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        ಆದರೆ (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fix_ಮೇಲೆ_Packed(var);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ಸ್ಥಿರಾಂಕ uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](Dfg_ಶೃಂಗ& sink) {
                // Ignore if sink is not part of same cycle
                ಆದರೆ (m_sccInfo.get(sink) != component) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Only handle ArraySels with constant index
                DfgArraySel* ಸ್ಥಿರಾಂಕ aselp = sink.cast<DfgArraySel>();
                ಆದರೆ (!aselp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                ಆದರೆ (!aselp->bitp()->is<DfgConst>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Fix up the word
                fix_ಮೇಲೆ_Packed(*aselp);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    Fix_ಮೇಲೆ(Dfg_ನಕ್ಷೆ& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

ಸಾರ್ವಜನಿಕ:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ಸ್ಥಿರ ಗಾತ್ರ_t apply(Dfg_ನಕ್ಷೆ& dfg, SccInfo& sccInfo) {
        ಆದರೆ (!sccInfo.isCyclic()) ಹಿಂತಿರುಗಿ 0;

        Fix_ಮೇಲೆ fix_ಮೇಲೆ{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : dfg.varVertices()) {
            ಆದರೆ (!sccInfo.get(vtx)) ಮುಂದುವರಿ;
            fix_ಮೇಲೆ.ಮುಖ್ಯ(vtx);
            ಆದರೆ (!sccInfo.isCyclic()) ನಿಲ್ಲಿಸು;
        }
        // Return the number of improvements made
        ಹಿಂತಿರುಗಿ fix_ಮೇಲೆ.m_nImprovements;
    }
};

ಶೂನ್ಯ break_ಚಕ್ರಗಳು(Dfg_ನಕ್ಷೆ& dfg, V3_Dfg_Break_ಚಕ್ರಗಳು_Context& ctx) {
    // Shorthand for dumping graph at given dump level
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ dump = [&](ಪೂರ್ಣಾಂಕ level, ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ& dfg, ಸ್ಥಿರಾಂಕ std::string& name) {
        ಆದರೆ (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    ಗಾತ್ರ_t nImprovements = 0;
    ಗಾತ್ರ_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    ಮಾಡು {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t nFixed = Fix_ಮೇಲೆ::apply(dfg, sccInfo);
        ಆದರೆ (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        ಆದರೆ (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        ಆದರೆ (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            ಹಿಂತಿರುಗಿ;
        }
    } ಆಗ (nImprovements != prevNImprovements);

    // Debug dump
    ಆದರೆ (dumpDfgLevel() >= 9) {
        ಸ್ಥಿರಾಂಕ SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
            ಹಿಂತಿರುಗಿ sccInfo.get(vtx);  //
        });
    }

    // Accounting
    ಆದರೆ (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } ಇಲ್ಲದಿದ್ದರೆ {
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
    std::ಸದಿಶ<Dfg_ಶೃಂಗ_Var*> varps;
    ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : dfg.varVertices()) {
        ಆದರೆ (!vtx.tmpForp()) varps.ತಳ್ಳು_ಹಿಂಭಾಗ(&vtx);
    }
    std::stable_ವಿಭಜನೆ(varps.begin(), varps.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* varp) {
        ಹಿಂತಿರುಗಿ varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    ಗಾಗಿ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp : varps) {
        // Stop as soon as it becomes acyclic
        ಆದರೆ (!sccInfo.isCyclic()) ನಿಲ್ಲಿಸು;

        // Ignore if variable is not part of a cycle
        ಆದರೆ (!sccInfo.get(*varp)) ಮುಂದುವರಿ;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* ಸ್ಥಿರಾಂಕ prevp = ಹೊಸ DfgPrev{dfg, varp->vscp()};
        sccInfo.ಕೂಡಿಸು(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    ಆದರೆ (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
ಶೂನ್ಯ V3DfgPasses::break_ಚಕ್ರಗಳು(Dfg_ನಕ್ಷೆ& dfg, V3DfgContext& ctx) {
    V3_Dfg_Break_ಚಕ್ರಗಳು::break_ಚಕ್ರಗಳು(dfg, ctx.m_break_ಚಕ್ರಗಳು_Context);
    ಆದರೆ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::remove_ಬಳಸದ(dfg);
}
