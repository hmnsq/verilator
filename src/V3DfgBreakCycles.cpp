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

ನಾಮಾಕಾಶ V3DfgBreakCycles {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
ವರ್ಗ SccInfo final {
    DfgGraph& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ಧ್ರುವ DfgVertex*, uint64_t> m_vtx2Scc;
    size_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

ಸಾರ್ವಜನಿಕ:
    SccInfo(DfgGraph& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) {
            ಧ್ರುವ uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            ಯದಿ (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    ತಾರ್ಕಿಕ isCyclic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ಧ್ರುವ DfgVertex& vtx) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    ಶೂನ್ಯ add(ಧ್ರುವ DfgVertex& vtx, uint64_t sccIdx) {
        ಧ್ರುವ ತಾರ್ಕಿಕ newEntry = m_vtx2Scc.emplace(&vtx, sccIdx).second;
        UASSERT_OBJ(newEntry, &vtx, "Vertex already inserted");
        ಯದಿ (sccIdx) ++m_nCyclicVertices;
    }

    ತಾರ್ಕಿಕ stillCyclicFwd(ಧ್ರುವ DfgVertex& vtx) ಧ್ರುವ {
        // If it only reads non-cyclic vertices, it has also become acyclic
        ಪ್ರತ್ಯಾಗಮ vtx.foreachSource([&](ಧ್ರುವ DfgVertex& src) {  //
            ಪ್ರತ್ಯಾಗಮ m_vtx2Scc.at(&src);
        });
    }

    ತಾರ್ಕಿಕ stillCyclicBwd(ಧ್ರುವ DfgVertex& vtx) ಧ್ರುವ {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        ಪ್ರತ್ಯಾಗಮ vtx.foreachSink([&](ಧ್ರುವ DfgVertex& dst) {  //
            ಪ್ರತ್ಯಾಗಮ m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    ಶೂನ್ಯ updateAcyclic(ಧ್ರುವ DfgVertex& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ಧ್ರುವ DfgVertex& dst) {
            // Sink was known to be acyclic, stop
            ಯದಿ (!m_vtx2Scc.at(&dst)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (!stillCyclicFwd(dst)) updateAcyclic(dst);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        // Propagate the update through the graph backward
        vtx.foreachSource([&](ಧ್ರುವ DfgVertex& src) {
            // Source was known to be acyclic, stop
            ಯದಿ (!m_vtx2Scc.at(&src)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (!stillCyclicBwd(src)) updateAcyclic(src);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    ಶೂನ್ಯ updateReplacement(ಧ್ರುವ DfgVertex& vtx, DfgVertex& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        ಯದಿ (!get(replacement)) {
            replacement.foreachSink([&](DfgVertex& dst) {
                ಯದಿ (!get(dst)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                ಯದಿ (!stillCyclicFwd(dst)) updateAcyclic(dst);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
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
    ಶೂನ್ಯ validate() ಧ್ರುವ {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

ವರ್ಗ TraceDriver final : ಸಾರ್ವಜನಿಕ DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    ಸಂರಚನಾ Cache_ಕೀಲಿ final {
        DfgVertex* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_ಕೀಲಿ() = delete;
        Cache_ಕೀಲಿ(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        ಸಂರಚನಾ Hash final {
            size_t operator()(ಧ್ರುವ Cache_ಕೀಲಿ& item) ಧ್ರುವ {
                // cppcheck-suppress unreadVariable
                V3Hash hash{item.m_vtxp};
                hash += item.m_lsb;
                hash += item.m_msb;
                ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
            }
        };

        ಸಂರಚನಾ Equal final {
            ತಾರ್ಕಿಕ operator()(ಧ್ರುವ Cache_ಕೀಲಿ& a, ಧ್ರುವ Cache_ಕೀಲಿ& b) ಧ್ರುವ {
                ಪ್ರತ್ಯಾಗಮ a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
            }
        };
    };

    // STATE
    DfgGraph& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    // The strongly connected component we are currently trying to escape
    uint64_t m_component = 0;
    uint32_t m_lsb = 0;  // LSB to extract from the currently visited Vertex
    uint32_t m_msb = 0;  // MSB to extract from the currently visited Vertex
    std::vector<uint32_t> m_idxs;  // Indices to extract from the currently visited Vertex
    std::vector<DfgVertex**> m_pendingps;  // Pending pass-through cache entries across trace calls
    // Result of tracing the currently visited Vertex. Use RETURN_RESULT below!
    DfgVertex* m_resp = nullptr;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    DfgVertex* m_tailp = nullptr;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    DfgVertex* m_defaultp = nullptr;
    DfgVertex* m_splicep = nullptr;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_ಕೀಲಿ, DfgVertex*, Cache_ಕೀಲಿ::Hash, Cache_ಕೀಲಿ::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_ಕೀಲಿ, DfgVertex*, Cache_ಕೀಲಿ::Hash, Cache_ಕೀಲಿ::Equal> m_boundaryCache;

#ifdef VL_DEBUG
    std::ofstream m_ಸಾಲು_Coverage_File;  // Line coverage file, just for testing
#endif

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    template <typename Vertex>
    Vertex* make(ಧ್ರುವ DfgVertex* refp, uint32_t width) {
        static_assert(std::is_base_of<DfgVertex, Vertex>::ಮೌಲ್ಯ  //
                          && !std::is_base_of<DfgVertexVar, Vertex>::ಮೌಲ್ಯ,
                      "Should only make operation vertices and constants");

        ಯದಿ VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, Vertex>::ಮೌಲ್ಯ) {
            DfgConst* ಧ್ರುವ vtxp = ನೂತನ DfgConst{m_dfg, refp->fileline(), width, 0};
            m_sccInfo.add(*vtxp, 0);
            ಪ್ರತ್ಯಾಗಮ reinterpret_cast<Vertex*>(vtxp);
        } ಅನ್ಯಥಾ {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            using Vtx = typename std::conditional<std::is_same<DfgConst, Vertex>::ಮೌಲ್ಯ, DfgSel,
                                                  Vertex>::type;
            Vtx* ಧ್ರುವ vtxp = ನೂತನ Vtx{m_dfg, refp->fileline(), DfgDataType::packed(width)};
            m_sccInfo.add(*vtxp, 0);
            ಪ್ರತ್ಯಾಗಮ reinterpret_cast<Vertex*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    DfgVertexVar* createTmp(ಧ್ರುವ ವರ್ಣ* prefix, DfgVertex* vtxp) {
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
        DfgVertex::ScopeCache scopeCache;
        AstScope* ಧ್ರುವ scopep = vtxp->scopep(scopeCache);
        DfgVertexVar* ಧ್ರುವ varp = m_dfg.makeNewVar(flp, prefix, vtxp->dtype(), scopep);
        varp->vscp()->varp()->isInternal(ಸತ್ಯ);
        varp->tmpForp(varp->vscp());
        m_sccInfo.add(*varp, 0);
        ಪ್ರತ್ಯಾಗಮ varp;
    }

    // Trace drivers of the given packed vertex, at the given bit range.
    DfgVertex* trace(DfgVertex* vtxp, uint32_t msb, uint32_t lsb) {
        // Many vertices just pass the traced bit range through to one of their sources
        // (see RETURN_RESULT_TAIL). Chains of such vertices can be tens of thousands of
        // vertices long in large designs (e.g.: a wide concatenation), so resolve them
        // in this loop instead of by recursion, which would overflow the C++ stack.
        // 'm_pendingps' holds the cache entries of the vertices on the current chain. They
        // all yield the same result, which is filled in when the chain is resolved.
        // Note: references into 'm_cache' are stable across the insertions below.
        DfgVertex* ಧ್ರುವ startVtxp = vtxp;
        ಧ್ರುವ uint32_t startMsb = msb;
        ಧ್ರುವ uint32_t startLsb = lsb;
        ಧ್ರುವ size_t pendingBegin = m_pendingps.size();
        DfgVertex* resp = nullptr;

        ಯಾವತ್ (ಸತ್ಯ) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->size() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            ಯದಿ (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                DfgVertex*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(nullptr))
                          .first->second;
                ಯದಿ (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    ಯದಿ (DfgVertexSplice* ಧ್ರುವ splicep = respr->cast<DfgVertexSplice>()) {
                        DfgVertexVar* ಧ್ರುವ tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    ಯದಿ (msb != respr->width() - 1 || lsb != 0) {
                        DfgSel* ಧ್ರುವ selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                ಭಙ್ಗ;
            }

            // Normal cache for intra-component traces
            DfgVertex*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(nullptr))
                                    .first->second;

            // Trace the vertex
            ಯದಿ (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } ಅನ್ಯಥಾ {
                // Otherwise visit the vertex to trace it
                VL_RESTORER(m_msb);
                VL_RESTORER(m_lsb);
                VL_RESTORER_CLEAR(m_idxs);
                VL_RESTORER(m_resp);
                VL_RESTORER(m_tailp);
                m_msb = msb;
                m_lsb = lsb;
                m_resp = nullptr;
                m_tailp = nullptr;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != nullptr) != (m_tailp != nullptr), vtxp,
                            "Expected exactly one trace result");
                ಯದಿ (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ತಳ್ಳು_back(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    ಅನುವರ್ತ;
                }
                respr = m_resp;
            }
            resp = respr;
            ಭಙ್ಗ;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, startVtxp, "Tracing driver failed for " << startVtxp->typeName());
        UASSERT_OBJ(resp->width() == (startMsb - startLsb + 1), startVtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        ಪ್ರತಿ (size_t i = pendingBegin; i < m_pendingps.size(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    DfgVertex* traceSameIdx(DfgVertex* ಧ್ರುವ vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = nullptr;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->width() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        ಪ್ರತ್ಯಾಗಮ m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    DfgVertex* trace_ತಳ್ಳು_Idx(DfgVertex* ಧ್ರುವ vtxp, ಧ್ರುವ uint32_t idx) {
        ಧ್ರುವ size_t nIdxs = m_idxs.size();
        m_idxs.ತಳ್ಳು_back(idx);
        DfgVertex* ಧ್ರುವ resp = traceSameIdx(vtxp);
        m_idxs.ತೆಗೆ_back();
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    DfgVertex* trace_ತೆಗೆ_Idx(DfgVertex* ಧ್ರುವ vtxp) {
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        ಧ್ರುವ size_t nIdxs = m_idxs.size();
        ಧ್ರುವ uint32_t idx = m_idxs.back();
        m_idxs.ತೆಗೆ_back();
        DfgVertex* ಧ್ರುವ resp = m_idxs.empty() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ತಳ್ಳು_back(idx);
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    template <typename Vertex>
    Vertex* trace_Bitwise_ದ್ವಿಮಾನ(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ದ್ವಿಮಾನ, Vertex>::ಮೌಲ್ಯ,
                      "Should only call on DfgVertexBinary");
        Vertex* ಧ್ರುವ resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    template <typename Vertex>
    DfgVertex* traceAddSub(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ದ್ವಿಮಾನ, Vertex>::ಮೌಲ್ಯ,
                      "Should only call on DfgVertexBinary");
        Vertex* ಧ್ರುವ opp = make<Vertex>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ಧ್ರುವ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        ಪ್ರತ್ಯಾಗಮ selp;
    }

    template <typename Vertex>
    Vertex* traceReduction(Vertex* vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::ಮೌಲ್ಯ,
                      "Should only call on DfgVertexUnary");
        Vertex* ಧ್ರುವ resp = make<Vertex>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->width() - 1, 0));
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    template <typename Vertex>
    Vertex* traceCmp(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ದ್ವಿಮಾನ, Vertex>::ಮೌಲ್ಯ,
                      "Should only call on DfgVertexBinary");
        Vertex* ಧ್ರುವ resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define TRACE_RESULT_COVER() \
    ಕುರು { \
        ಯದಿ (VL_UNLIKELY(m_ಸಾಲು_Coverage_File.is_open())) m_ಸಾಲು_Coverage_File << __LINE__ << '\n'; \
    } ಯಾವತ್ (ಅಸತ್ಯ)
#ಅನ್ಯಥಾ
#define TRACE_RESULT_COVER() \
    ಕುರು { \
    } ಯಾವತ್ (ಅಸತ್ಯ)
#endif

#define RETURN_RESULT(vtxp) \
    ಕುರು { \
        m_resp = (vtxp); \
        TRACE_RESULT_COVER(); \
        ಪ್ರತ್ಯಾಗಮ; \
    } ಯಾವತ್ (ಅಸತ್ಯ)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#define RETURN_RESULT_TAIL(srcp, smsb, slsb) \
    ಕುರು { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        DfgVertex* ಧ್ರುವ tailp = (srcp); \
        ಧ್ರುವ uint32_t tailMsb = (smsb); \
        ಧ್ರುವ uint32_t tailLsb = (slsb); \
        m_tailp = tailp; \
        m_msb = tailMsb; \
        m_lsb = tailLsb; \
        TRACE_RESULT_COVER(); \
        ಪ್ರತ್ಯಾಗಮ; \
    } ಯಾವತ್ (ಅಸತ್ಯ)

    // VISITORS
    ಶೂನ್ಯ visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ಶೂನ್ಯ visit(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        DfgVertex* defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        ಸಂರಚನಾ Driver final {
            DfgVertex* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = delete;
            Driver(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::vector<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        ತಾರ್ಕಿಕ tryWholeDefault = defaultp;
        DfgVertex* coverp = nullptr;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lsb) {
            ಧ್ರುವ uint32_t msb = lsb + src.width() - 1;
            drivers.emplace_back(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            ಯದಿ (m_msb >= lsb && msb >= m_lsb) tryWholeDefault = ಅಸತ್ಯ;
            // If it does not cover the whole searched bit range, move on
            ಯದಿ (m_lsb < lsb || msb < m_msb) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        });

        // Trace the driver covering the whole searched range, if there is one
        ಯದಿ (coverp) RETURN_RESULT_TAIL(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        ಯದಿ (tryWholeDefault) RETURN_RESULT_TAIL(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::sort(drivers.begin(), drivers.end(), [](ಧ್ರುವ Driver& a, ಧ್ರುವ Driver& b) {  //
            ಪ್ರತ್ಯಾಗಮ a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::vector<DfgVertex*> termps;
        uint32_t lsb = m_lsb;
        ಪ್ರತಿ (ಧ್ರುವ Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            ಯದಿ (lsb > driver.m_msb) ಅನುವರ್ತ;
            // Driver is above the searched MSB, done
            ಯದಿ (driver.m_lsb > m_msb) ಭಙ್ಗ;
            // Gap below this driver, trace default to fill it
            ಯದಿ (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_back(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ಧ್ರುವ uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_back(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        ಯದಿ (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_back(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.size() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        DfgVertex* resp = termps.front();
        ಪ್ರತಿ (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* ಧ್ರುವ termp = termps[i];
            DfgConcat* ಧ್ರುವ catp = make<DfgConcat>(termp, resp->width() + termp->width());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_RESULT(resp);
    }

    ಶೂನ್ಯ visit(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        DfgVertex* ಧ್ರುವ defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ಧ್ರುವ uint32_t idx = m_idxs.back();
        ಯದಿ (DfgVertex* ಧ್ರುವ driverp = vtxp->driverAt(idx)) {
            DfgVertex* ಧ್ರುವ srcp = driverp->as<DfgUnitArray>()->srcp();
            ಯದಿ (srcp->is<DfgVertexSplice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_RESULT(trace_ತೆಗೆ_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_RESULT(traceSameIdx(defaultp));
    }

    ಶೂನ್ಯ visit(DfgVertexVar* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        DfgVertex* ಧ್ರುವ defaultp = vtxp->defaultp();
        DfgVertex* ಧ್ರುವ drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        ಯದಿ (srcp && srcp->is<DfgVertexSplice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_RESULT(m_idxs.empty() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    ಶೂನ್ಯ visit(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_RESULT(trace_ತಳ್ಳು_Idx(vtxp->fromp(), idxp->toU32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.empty(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ಧ್ರುವ resp = make<DfgArraySel>(vtxp, vtxp->width());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->width() - 1, 0));
        DfgSel* ಧ್ರುವ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    ಶೂನ್ಯ visit(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.back() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_RESULT(trace_ತೆಗೆ_Idx(vtxp->srcp()));
    }

    ಶೂನ್ಯ visit(DfgConcat* vtxp) override {
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        ಧ್ರುವ uint32_t rWidth = rhsp->width();
        // If the traced bits are wholly in the RHS
        ಯದಿ (rWidth > m_msb) RETURN_RESULT_TAIL(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        ಯದಿ (m_lsb >= rWidth) RETURN_RESULT_TAIL(lhsp, m_msb - rWidth, m_lsb - rWidth);
        // The traced bit spans both sides, trace both
        DfgConcat* ಧ್ರುವ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, rWidth - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - rWidth, 0));
        RETURN_RESULT(resp);
    }

    ಶೂನ್ಯ visit(DfgRep* vtxp) override {
        DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        ಧ್ರುವ uint32_t sWidth = srcp->width();
        // If we need more bits than the source, then we need the whole source
        ಯದಿ (m_msb - m_lsb + 1 > sWidth) {
            DfgRep* ಧ್ರುವ repp = make<DfgRep>(vtxp, vtxp->width());
            repp->srcp(trace(srcp, sWidth - 1, 0));
            DfgSel* ಧ್ರುವ resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_RESULT(resp);
        }
        // If the requested bits are within the same repliacted word
        ಯದಿ (m_msb / sWidth == m_lsb / sWidth) {
            RETURN_RESULT_TAIL(srcp, m_msb % sWidth, m_lsb % sWidth);
        }
        // The requested bits span two replicated words
        DfgConcat* ಧ್ರುವ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, sWidth - 1, m_lsb % sWidth));
        resp->lhsp(trace(srcp, m_msb % sWidth, 0));
        RETURN_RESULT(resp);
    }

    ಶೂನ್ಯ visit(DfgExtend* vtxp) override {
        DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        ಧ್ರುವ uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        ಯದಿ (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ಯದಿ (m_lsb >= sWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ಧ್ರುವ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    ಶೂನ್ಯ visit(DfgExtendS* vtxp) override {
        DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        ಧ್ರುವ uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        ಯದಿ (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ಯದಿ (m_lsb >= sWidth) {
            ಯದಿ (m_msb == m_lsb) RETURN_RESULT_TAIL(srcp, sWidth - 1, sWidth - 1);
            DfgExtendS* ಧ್ರುವ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, sWidth - 1, sWidth - 1));
            RETURN_RESULT(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ಧ್ರುವ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    ಶೂನ್ಯ visit(DfgSel* vtxp) override {
        ಧ್ರುವ uint32_t lsb = vtxp->lsb();
        RETURN_RESULT_TAIL(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    ಶೂನ್ಯ visit(DfgNot* vtxp) override {
        DfgNot* ಧ್ರುವ resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    ಶೂನ್ಯ visit(DfgAnd* vtxp) override { RETURN_RESULT(trace_Bitwise_ದ್ವಿಮಾನ(vtxp)); }
    ಶೂನ್ಯ visit(DfgOr* vtxp) override { RETURN_RESULT(trace_Bitwise_ದ್ವಿಮಾನ(vtxp)); }
    ಶೂನ್ಯ visit(DfgXor* vtxp) override { RETURN_RESULT(trace_Bitwise_ದ್ವಿಮಾನ(vtxp)); }
    ಶೂನ್ಯ visit(DfgAdd* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    ಶೂನ್ಯ visit(DfgSub* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    ಶೂನ್ಯ visit(DfgRedAnd* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    ಶೂನ್ಯ visit(DfgRedOr* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    ಶೂನ್ಯ visit(DfgRedXor* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    ಶೂನ್ಯ visit(DfgEq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ಶೂನ್ಯ visit(DfgNeq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ಶೂನ್ಯ visit(DfgLt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ಶೂನ್ಯ visit(DfgLte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ಶೂನ್ಯ visit(DfgGt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ಶೂನ್ಯ visit(DfgGte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }

    ಶೂನ್ಯ visit(DfgShiftRS* vtxp) override {
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಧ್ರುವ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ಧ್ರುವ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            ಯದಿ (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            ಯದಿ (m_lsb >= lowerWidth) {
                DfgExtendS* ಧ್ರುವ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->width() - 1, lhsp->width() - 1));
                RETURN_RESULT(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ಧ್ರುವ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftRS* ಧ್ರುವ shiftrsp = make<DfgShiftRS>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        ಯದಿ (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrsp);
        DfgSel* ಧ್ರುವ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    ಶೂನ್ಯ visit(DfgShiftR* vtxp) override {
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಧ್ರುವ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ಧ್ರುವ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            ಯದಿ (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            ಯದಿ (m_lsb >= lowerWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ಧ್ರುವ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftR* ಧ್ರುವ shiftrp = make<DfgShiftR>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        ಯದಿ (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrp);
        DfgSel* ಧ್ರುವ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    ಶೂನ್ಯ visit(DfgShiftL* vtxp) override {
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಧ್ರುವ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ಧ್ರುವ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? vtxp->width() : shiftAmnt;
            // If the traced bits are wholly in the input
            ಯದಿ (m_lsb >= lowerWidth) {
                RETURN_RESULT_TAIL(lhsp, m_msb - shiftAmnt, m_lsb - shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            ಯದಿ (lowerWidth > m_msb) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ಧ್ರುವ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->width() - (m_msb - lowerWidth + 1)));
            resp->lhsp(trace(lhsp, m_msb - shiftAmnt, lowerWidth - shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftL* ಧ್ರುವ shiftlp = make<DfgShiftL>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        ಯದಿ (m_lsb == 0) RETURN_RESULT(shiftlp);
        DfgSel* ಧ್ರುವ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    ಶೂನ್ಯ visit(DfgCond* vtxp) override {
        DfgCond* ಧ್ರುವ resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->width() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    ಶೂನ್ಯ visit(DfgMatchMasked* vtxp) override {
        DfgMatchMasked* ಧ್ರುವ resp = make<DfgMatchMasked>(vtxp, vtxp->width());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->width() - 1, 0));
        DfgSel* ಧ್ರುವ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

#undef RETURN_RESULT
#undef TRACE_RESULT_COVER
#undef RETURN_RESULT_TAIL

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    TraceDriver(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#ifdef VL_DEBUG
        ಯದಿ (v3Global.opt.debugCheck()) {
            m_ಸಾಲು_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::app);
        }
#endif
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    DfgVertex* apply(DfgVertex& vtx, uint32_t lsb, uint32_t width) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        ಪ್ರತ್ಯಾಗಮ trace(&vtx, lsb + width - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
ವರ್ಗ BitMask final {
    // TYPES
    ಧ್ರುವ DfgDataType& m_dtype;
    ಸಙ್ಘ {
        V3_ಸಂಖ್ಯೆ m_ಅಂಕೆ;  // The bits, if packed
        std::vector<BitMask> m_sub;  // The per element masks, if aggregate
    };

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    BitMask(File_ಸಾಲು* flp, ಧ್ರುವ DfgDataType& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        ಯದಿ (m_dtype.isPacked()) {
            ನೂತನ (&m_ಅಂಕೆ) V3_ಸಂಖ್ಯೆ{flp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_dtype.size()), 0};
        } ಅನ್ಯಥಾ {
            ನೂತನ (&m_sub) std::vector<BitMask>{};
            m_sub.reserve(m_dtype.size());
            ಪ್ರತಿ (uint32_t i = 0; i < m_dtype.size(); ++i)
                m_sub.emplace_back(flp, m_dtype.elemDtype());
        }
    }
    BitMask(ಧ್ರುವ DfgVertex& vtx)
        : BitMask{vtx.fileline(), vtx.dtype()} {}
    BitMask(ಧ್ರುವ BitMask& other)
        : m_dtype{other.m_dtype} {
        ಯದಿ (m_dtype.isPacked()) {
            ನೂತನ (&m_ಅಂಕೆ) V3_ಸಂಖ್ಯೆ{other.m_ಅಂಕೆ};
        } ಅನ್ಯಥಾ {
            ನೂತನ (&m_sub) std::vector<BitMask>{other.m_sub};
        }
    }
    BitMask& operator=(ಧ್ರುವ BitMask& other) {
        ಯದಿ (ಏತತ್ != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            ಯದಿ (other.m_dtype.isPacked()) {
                m_ಅಂಕೆ = other.m_ಅಂಕೆ;
            } ಅನ್ಯಥಾ {
                m_sub = other.m_sub;
            }
        }
        ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    }
    ~BitMask() {
        ಯದಿ (m_dtype.isPacked()) {
            m_ಅಂಕೆ.~V3_ಸಂಖ್ಯೆ();
        } ಅನ್ಯಥಾ {
            m_sub.~vector<BitMask>();
        }
    }

    // METHODS
    V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ;
    }
    ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಂಕೆ() ಧ್ರುವ {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ;
    }
    std::vector<BitMask>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        ಪ್ರತ್ಯಾಗಮ m_sub;
    }

    ತಾರ್ಕಿಕ isZero() ಧ್ರುವ {
        ಯದಿ (m_dtype.isPacked()) ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ.isEqZero();

        ಪ್ರತಿ (ಧ್ರುವ BitMask& sub : m_sub) {
            ಯದಿ (!sub.isZero()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ isOnes() ಧ್ರುವ {
        ಯದಿ (m_dtype.isPacked()) ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ.isEqAllOnes();

        ಪ್ರತಿ (ಧ್ರುವ BitMask& sub : m_sub) {
            ಯದಿ (!sub.isOnes()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಶೂನ್ಯ setZero() {
        ಯದಿ (m_dtype.isPacked()) {
            m_ಅಂಕೆ.setAllBits0();
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (BitMask& sub : m_sub) sub.setZero();
        }
    }
    ಶೂನ್ಯ setOnes() {
        ಯದಿ (m_dtype.isPacked()) {
            m_ಅಂಕೆ.setAllBits1();
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (BitMask& sub : m_sub) sub.setOnes();
        }
    }

    ತಾರ್ಕಿಕ operator==(ಧ್ರುವ BitMask& other) ಧ್ರುವ {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        ಯದಿ (m_dtype.isPacked()) ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ.isCaseEq(other.m_ಅಂಕೆ);
        ಪ್ರತ್ಯಾಗಮ m_sub == other.m_sub;
    }
    ತಾರ್ಕಿಕ operator!=(ಧ್ರುವ BitMask& other) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !(*ಏತತ್ == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    ತಾರ್ಕಿಕ isContractionOf(ಧ್ರುವ BitMask& that) ಧ್ರುವ {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        ಯದಿ (m_dtype.isPacked()) {
            ಧ್ರುವ size_t ಪದಗಳು = VL_ಪದಗಳು_I(m_dtype.size());
            ಪ್ರತಿ (size_t i = 0; i < ಪದಗಳು; ++i) {
                ಯದಿ (~m_ಅಂಕೆ.edataWord(i) & that.m_ಅಂಕೆ.edataWord(i)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        ಪ್ರತಿ (size_t i = 0; i < m_sub.size(); ++i) {
            ಯದಿ (m_sub[i].isContractionOf(that.m_sub[i])) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    std::string toString() ಧ್ರುವ {
        ಯದಿ (m_dtype.isPacked()) ಪ್ರತ್ಯಾಗಮ "0x" + m_ಅಂಕೆ.displayed(m_ಅಂಕೆ.fileline(), "%x");
        std::string result;
        result += "{";
        ಪ್ರತಿ (ಧ್ರುವ BitMask& sub : m_sub) {
            ಯದಿ (&sub != &m_sub.front()) result += ", ";
            result += sub.toString();
        }
        result += "}";
        ಪ್ರತ್ಯಾಗಮ result;
    }
};

ವರ್ಗ IndependentBits final : ಸಾರ್ವಜನಿಕ DfgVisitor {
    // TYPES
    ಸಂರಚನಾ VertexState final {
        size_t m_rpo_ಸಂಖ್ಯೆ = 0;  // Reverse Postorder Number of vertex
        ತಾರ್ಕಿಕ m_isOnWorkList = ಅಸತ್ಯ;  // Whether the vertex is on the work list
    };

    // STATE
    ಧ್ರುವ SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ಧ್ರುವ DfgVertex*, BitMask> m_vtxp2Mask;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_workQueue;
    std::unordered_map<ಧ್ರುವ DfgVertex*, VertexState> m_vtxp2State;  // State of each vertex

#ifdef VL_DEBUG
    std::ofstream m_ಸಾಲು_Coverage_File;  // Line coverage file, just for testing
#endif

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    BitMask& mask(ಧ್ರುವ DfgVertex& vtx) {
        // Look up (or create) mask for 'vtx'
        ಪ್ರತ್ಯಾಗಮ m_vtxp2Mask
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .first->second;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define MASK(vtxp) \
    ([ಏತತ್](ಧ್ರುವ DfgVertex& vtx) -> BitMask& { \
        ಯದಿ (VL_UNLIKELY(m_ಸಾಲು_Coverage_File.is_open())) m_ಸಾಲು_Coverage_File << __LINE__ << '\n'; \
        ಪ್ರತ್ಯಾಗಮ mask(vtx); \
    }(*vtxp))
#ಅನ್ಯಥಾ
#define MASK(vtxp) mask(*vtxp)
#endif

    // Clear all bits at or below the most significant clear bit
    ಶೂನ್ಯ floodTowardsLsb(V3_ಸಂಖ್ಯೆ& ಅಂಕೆ) {
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = ಅಂಕೆ.width() - 1; i >= 0; --i) {
            ಯದಿ (ಅಂಕೆ.bitIs1(i)) ಅನುವರ್ತ;
            ಅಂಕೆ.opSetRange(0, i + 1, '0');
            ಭಙ್ಗ;
        }
    }

    // Clear all bits at or above the least significant clear bit
    ಶೂನ್ಯ floodTowardsMsb(V3_ಸಂಖ್ಯೆ& ಅಂಕೆ) {
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಅಂಕೆ.width(); ++i) {
            ಯದಿ (ಅಂಕೆ.bitIs1(i)) ಅನುವರ್ತ;
            ಅಂಕೆ.opSetRange(i, ಅಂಕೆ.width() - i, '0');
            ಭಙ್ಗ;
        }
    }

    ಶೂನ್ಯ propagateFromDriver(BitMask& m, ಧ್ರುವ DfgVertex* srcp) {
        // If there is no driver, we are done
        ಯದಿ (!srcp) ಪ್ರತ್ಯಾಗಮ;
        // If it is driven by a splice, we need to combine the masks of the drivers
        ಯದಿ (ಧ್ರುವ DfgSplicePacked* ಧ್ರುವ splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ಧ್ರುವ DfgVertex& src, uint32_t lo) {
                m.ಅಂಕೆ().opSelInto(MASK(&src).ಅಂಕೆ(), lo, src.width());
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಧ್ರುವ DfgSpliceArray* ಧ್ರುವ splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ಧ್ರುವ DfgVertex& src, uint32_t lo) {
                ಯದಿ (ಧ್ರುವ DfgUnitArray* ಧ್ರುವ uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = MASK(uap->srcp());
                } ಅನ್ಯಥಾ {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Otherwise, we just use the mask of the single driver
        m = MASK(srcp);
    }

    // VISITORS
    ಶೂನ್ಯ visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    ಶೂನ್ಯ visit(DfgVertexVar* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        ಯದಿ (vtxp->isVolatile()) ಪ್ರತ್ಯಾಗಮ;

        BitMask& m = MASK(vtxp);
        DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        DfgVertex* ಧ್ರುವ defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        ಯದಿ (defaultp) m = MASK(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    ಶೂನ್ಯ visit(DfgVertexSplice* vtxp) override {
        propagateFromDriver(MASK(vtxp), vtxp);  // Needed to continue traversal
    }

    ಶೂನ್ಯ visit(DfgUnitArray* vtxp) override { MASK(vtxp).sub().at(0) = MASK(vtxp->srcp()); }

    ಶೂನ್ಯ visit(DfgArraySel* vtxp) override {
        DfgVertex* ಧ್ರುವ fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ idxp = vtxp->bitp()->cast<DfgConst>()) {
            MASK(vtxp) = MASK(vtxp->fromp()).sub().at(idxp->toSizeT());
            ಪ್ರತ್ಯಾಗಮ;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        ಯದಿ (MASK(vtxp->bitp()).isOnes() && MASK(fromp).isOnes()) MASK(vtxp).setOnes();
    }

    ಶೂನ್ಯ visit(DfgConcat* vtxp) override {
        ಧ್ರುವ DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        ಧ್ರುವ DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
        m.opSelInto(MASK(rhsp).ಅಂಕೆ(), 0, rhsp->width());
        m.opSelInto(MASK(lhsp).ಅಂಕೆ(), rhsp->width(), lhsp->width());
    }

    ಶೂನ್ಯ visit(DfgRep* vtxp) override {
        ಧ್ರುವ uint32_t ಎಣಿಕೆ = vtxp->ಎಣಿಕೆ();
        ಧ್ರುವ DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        ಧ್ರುವ uint32_t sWidth = srcp->width();
        V3_ಸಂಖ್ಯೆ& vMask = MASK(vtxp).ಅಂಕೆ();
        V3_ಸಂಖ್ಯೆ& sMask = MASK(srcp).ಅಂಕೆ();
        ಪ್ರತಿ (uint32_t i = 0; i < ಎಣಿಕೆ; ++i) vMask.opSelInto(sMask, i * sWidth, sWidth);
    }

    ಶೂನ್ಯ visit(DfgSel* vtxp) override {
        ಧ್ರುವ uint32_t lsb = vtxp->lsb();
        ಧ್ರುವ uint32_t msb = lsb + vtxp->width() - 1;
        MASK(vtxp).ಅಂಕೆ().opSel(MASK(vtxp->fromp()).ಅಂಕೆ(), msb, lsb);
    }

    ಶೂನ್ಯ visit(DfgExtend* vtxp) override {
        ಧ್ರುವ DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        ಧ್ರುವ uint32_t sWidth = srcp->width();
        V3_ಸಂಖ್ಯೆ& s = MASK(srcp).ಅಂಕೆ();
        V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, '1');
    }
    ಶೂನ್ಯ visit(DfgExtendS* vtxp) override {
        ಧ್ರುವ DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        ಧ್ರುವ uint32_t sWidth = srcp->width();
        V3_ಸಂಖ್ಯೆ& s = MASK(srcp).ಅಂಕೆ();
        V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, s.bitIs0(sWidth - 1) ? '0' : '1');
    }

    ಶೂನ್ಯ visit(DfgNot* vtxp) override {  //
        MASK(vtxp) = MASK(vtxp->srcp());
    }

    ಶೂನ್ಯ visit(DfgAnd* vtxp) override {  //
        MASK(vtxp).ಅಂಕೆ().opAnd(MASK(vtxp->lhsp()).ಅಂಕೆ(), MASK(vtxp->rhsp()).ಅಂಕೆ());
    }
    ಶೂನ್ಯ visit(DfgOr* vtxp) override {  //
        MASK(vtxp).ಅಂಕೆ().opAnd(MASK(vtxp->lhsp()).ಅಂಕೆ(), MASK(vtxp->rhsp()).ಅಂಕೆ());
    }
    ಶೂನ್ಯ visit(DfgXor* vtxp) override {  //
        MASK(vtxp).ಅಂಕೆ().opAnd(MASK(vtxp->lhsp()).ಅಂಕೆ(), MASK(vtxp->rhsp()).ಅಂಕೆ());
    }

    ಶೂನ್ಯ visit(DfgAdd* vtxp) override {
        V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
        m.opAnd(MASK(vtxp->lhsp()).ಅಂಕೆ(), MASK(vtxp->rhsp()).ಅಂಕೆ());
        floodTowardsMsb(m);
    }
    ಶೂನ್ಯ visit(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
        m.opAnd(MASK(vtxp->lhsp()).ಅಂಕೆ(), MASK(vtxp->rhsp()).ಅಂಕೆ());
        floodTowardsMsb(m);
    }

    ಶೂನ್ಯ visit(DfgRedAnd* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }
    ಶೂನ್ಯ visit(DfgRedOr* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }
    ಶೂನ್ಯ visit(DfgRedXor* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }

    ಶೂನ್ಯ visit(DfgEq* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }
    ಶೂನ್ಯ visit(DfgNeq* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }
    ಶೂನ್ಯ visit(DfgLt* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }
    ಶೂನ್ಯ visit(DfgLte* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }
    ಶೂನ್ಯ visit(DfgGt* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }
    ಶೂನ್ಯ visit(DfgGte* vtxp) override {
        ಧ್ರುವ ತಾರ್ಕಿಕ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ಯದಿ (independent) MASK(vtxp).setOnes();
    }

    ಶೂನ್ಯ visit(DfgShiftRS* vtxp) override {
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        ಧ್ರುವ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = rhsp->cast<DfgConst>()) {
            ಧ್ರುವ uint32_t shiftAmount = rConstp->toU32();
            ಯದಿ (shiftAmount >= width) {
                ಯದಿ (MASK(lhsp).ಅಂಕೆ().bitIs1(width - 1)) { MASK(vtxp).setOnes(); }
            } ಅನ್ಯಥಾ {
                V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
                m.opShiftRS(MASK(lhsp).ಅಂಕೆ(), rConstp->ಅಂಕೆ(), width);
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ಯದಿ (MASK(rhsp).isOnes()) {
            V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
            m = MASK(lhsp).ಅಂಕೆ();
            floodTowardsLsb(m);
        }
    }

    ಶೂನ್ಯ visit(DfgShiftR* vtxp) override {
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        ಧ್ರುವ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = rhsp->cast<DfgConst>()) {
            ಧ್ರುವ uint32_t shiftAmount = rConstp->toU32();
            ಯದಿ (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } ಅನ್ಯಥಾ {
                V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
                m.opShiftR(MASK(lhsp).ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ಯದಿ (MASK(rhsp).isOnes()) {
            V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
            m = MASK(lhsp).ಅಂಕೆ();
            floodTowardsLsb(m);
        }
    }

    ಶೂನ್ಯ visit(DfgShiftL* vtxp) override {
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        ಧ್ರುವ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = rhsp->cast<DfgConst>()) {
            ಧ್ರುವ uint32_t shiftAmount = rConstp->toU32();
            ಯದಿ (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } ಅನ್ಯಥಾ {
                V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
                m.opShiftL(MASK(lhsp).ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                m.opSetRange(0, shiftAmount, '1');
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        ಯದಿ (MASK(rhsp).isOnes()) {
            V3_ಸಂಖ್ಯೆ& m = MASK(vtxp).ಅಂಕೆ();
            m = MASK(lhsp).ಅಂಕೆ();
            floodTowardsMsb(m);
        }
    }

    ಶೂನ್ಯ visit(DfgCond* vtxp) override {
        ಯದಿ (MASK(vtxp->condp()).isOnes()) {
            MASK(vtxp).ಅಂಕೆ().opAnd(MASK(vtxp->thenp()).ಅಂಕೆ(), MASK(vtxp->elsep()).ಅಂಕೆ());
        }
    }

    ಶೂನ್ಯ visit(DfgMatchMasked* vtxp) override {
        ಯದಿ (MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->matchp()).isOnes()) { MASK(vtxp).setOnes(); }
    }

#undef MASK

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    ಶೂನ್ಯ enqueueSinks(DfgVertex& vtx) {
        vtx.foreachSink([&](DfgVertex& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            ಯದಿ (!m_sccInfo.get(sink)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Otherwise just enqueue it
            VertexState& state = m_vtxp2State.at(&sink);
            ಯದಿ (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_workQueue.ತಳ್ಳು(state.m_rpo_ಸಂಖ್ಯೆ);
            }
            state.m_isOnWorkList = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
    };

    ಶೂನ್ಯ dfsPostOrder(std::unordered_set<DfgVertex*>& visited,  //
                      std::vector<DfgVertex*>& postOrderEnumeration,  //
                      DfgVertex& vtx) {
        // Mark visited, skip if already visited
        ಯದಿ (!visited.ಸೇರಿಸು(&vtx).second) ಪ್ರತ್ಯಾಗಮ;
        // Visit un-visited successors
        vtx.foreachSink([&](DfgVertex& snk) {
            dfsPostOrder(visited, postOrderEnumeration, snk);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        // Add to post order enumeration
        postOrderEnumeration.emplace_back(&vtx);
    };

    IndependentBits(DfgGraph& dfg, ಧ್ರುವ SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#ifdef VL_DEBUG
        ಯದಿ (v3Global.opt.debugCheck()) {
            m_ಸಾಲು_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::app);
        }
#endif

        // Compute Reverse-Postorder enumeration of vertices
        std::vector<DfgVertex*> rpoEnumeration;
        std::unordered_set<DfgVertex*> visited;
        dfg.forEachVertex([&](DfgVertex& vtx) {
            ಯದಿ (!vtx.nInputs()) dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        dfg.forEachVertex([&](DfgVertex& vtx) {  //
            dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        std::reverse(rpoEnumeration.begin(), rpoEnumeration.end());
        UASSERT(rpoEnumeration.size() == dfg.size(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        ಪ್ರತಿ (size_t i = 0; i < rpoEnumeration.size(); ++i) {
            VertexState& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_ಸಂಖ್ಯೆ = i;
            state.m_isOnWorkList = ಅಸತ್ಯ;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : rpoEnumeration) {
            ಯದಿ (m_sccInfo.get(*vtxp)) ಅನುವರ್ತ;
            mask(*vtxp).setOnes();
        }
        ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : rpoEnumeration) {
            ಯದಿ (!m_sccInfo.get(*vtxp)) ಅನುವರ್ತ;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << mask(*vtxp).toString());
            ಯದಿ (!mask(*vtxp).isZero()) enqueueSinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        ಯಾವತ್ (!m_workQueue.empty()) {
            // Grab next item
            DfgVertex* ಧ್ರುವ currp = rpoEnumeration[m_workQueue.ತುದಿ()];
            m_workQueue.ತೆಗೆ();
            m_vtxp2State.at(currp).m_isOnWorkList = ಅಸತ್ಯ;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ಧ್ರುವ BitMask& currMask = mask(*currp);
            // Remember current mask so we can check if it changed
            ಧ್ರುವ BitMask prevMask = currMask;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            ಯದಿ (prevMask != currMask) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prevMask.toString()
                             << "\n      to: " << currMask.toString());
                enqueueSinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                ಯದಿ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!currMask.isContractionOf(prevMask), currp,
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
    ಸ್ಥಿರ std::unordered_map<ಧ್ರುವ DfgVertex*, BitMask> apply(DfgGraph& dfg,
                                                               ಧ್ರುವ SccInfo& sccInfo) {
        ಪ್ರತ್ಯಾಗಮ std::move(IndependentBits{dfg, sccInfo}.m_vtxp2Mask);
    }
};

ವರ್ಗ FixUp final {
    DfgGraph& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ಧ್ರುವ std::unordered_map<ಧ್ರುವ DfgVertex*, BitMask> m_independentBits
        = IndependentBits::apply(m_dfg, m_sccInfo);
    size_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ಸ್ಥಿರ BitMask computeUsedBits(DfgVertex& vtx) {
        BitMask result{vtx};
        vtx.foreachSink([&result](DfgVertex& sink) {
            // If used via a Sel, mark the selected bits used
            ಯದಿ (ಧ್ರುವ DfgSel* ಧ್ರುವ selp = sink.cast<DfgSel>()) {
                result.ಅಂಕೆ().opSetRange(selp->lsb(), selp->width(), '1');
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            // Used without a Sel, so all bits are used
            result.setOnes();
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        });
        ಪ್ರತ್ಯಾಗಮ result;
    }

    ಸ್ಥಿರ std::string debugStr(ಧ್ರುವ DfgVertex& vtx) {  // LCOV_EXCL_START
        ಯದಿ (ಧ್ರುವ DfgArraySel* ಧ್ರುವ aselp = vtx.cast<DfgArraySel>()) {
            ಧ್ರುವ size_t i = aselp->bitp()->as<DfgConst>()->toSizeT();
            ಪ್ರತ್ಯಾಗಮ debugStr(*aselp->fromp()) + "[" + std::to_string(i) + "]";
        }
        ಯದಿ (ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = vtx.cast<DfgVertexVar>()) {
            ಪ್ರತ್ಯಾಗಮ varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    ಶೂನ್ಯ gatherTerms(std::vector<DfgVertex*>& termps, DfgVertex& vtx, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& indpBits,
                     ಧ್ರುವ uint32_t hi, ಧ್ರುವ uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        ತಾರ್ಕಿಕ isIndependent = indpBits.bitIs1(lo);
        uint32_t lsb = lo;
        ಪ್ರತಿ (uint32_t msb = lo; msb <= hi; ++msb) {
            ಧ್ರುವ ತಾರ್ಕಿಕ endRange = msb == hi || isIndependent != indpBits.bitIs1(msb + 1);
            ಯದಿ (!endRange) ಅನುವರ್ತ;
            ಧ್ರುವ uint32_t width = msb - lsb + 1;
            // Compute the term to use for this range
            ಯದಿ (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_back(m_traceDriver.apply(vtx, lsb, width));
            } ಅನ್ಯಥಾ {
                // If dependent, fall back on using the part of the variable
                DfgSel* ಧ್ರುವ selp = ನೂತನ DfgSel{m_dfg, vtx.fileline(), DfgDataType::packed(width)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.add(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_back(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    ಶೂನ್ಯ fixUpPacked(DfgVertex& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ಧ್ರುವ BitMask& indpBits = m_independentBits.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indpBits.toString());
        // Can't do anything if all bits are dependent
        ಯದಿ (indpBits.isZero()) ಪ್ರತ್ಯಾಗಮ;

        // Figure out which bits of 'vtxp' are used
        ಧ್ರುವ BitMask usedBits = computeUsedBits(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << usedBits.toString());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_ಸಂಖ್ಯೆ usedAndIndependent{vtx.fileline(), static_cast<ಪೂರ್ಣಾಙ್ಕ>(vtx.width()), 0};
        usedAndIndependent.opAnd(usedBits.ಅಂಕೆ(), indpBits.ಅಂಕೆ());
        ಯದಿ (usedAndIndependent.isEqZero()) ಪ್ರತ್ಯಾಗಮ;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::vector<DfgVertex*> termps;

        // Iterate through consecutive used/unused ranges
        File_ಸಾಲು* ಧ್ರುವ flp = vtx.fileline();
        ಧ್ರುವ uint32_t width = vtx.width();
        ತಾರ್ಕಿಕ isUsed = usedBits.ಅಂಕೆ().bitIs1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        ಪ್ರತಿ (uint32_t msb = 0; msb < width; ++msb) {
            ಧ್ರುವ ತಾರ್ಕಿಕ endRange = msb == width - 1 || isUsed != usedBits.ಅಂಕೆ().bitIs1(msb + 1);
            ಯದಿ (!endRange) ಅನುವರ್ತ;
            ಯದಿ (isUsed) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indpBits.ಅಂಕೆ(), msb, lsb);
            } ಅನ್ಯಥಾ {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ಧ್ರುವ constp = ನೂತನ DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.add(*constp, 0);
                termps.emplace_back(constp);
            }
            // Next iteration
            isUsed = !isUsed;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        DfgVertex* replacementp = termps.front();
        ಧ್ರುವ uint64_t vComp = m_sccInfo.get(vtx);
        ಪ್ರತಿ (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* ಧ್ರುವ termp = termps[i];
            ಧ್ರುವ uint32_t catWidth = replacementp->width() + termp->width();
            DfgConcat* ಧ್ರುವ catp = ನೂತನ DfgConcat{m_dfg, flp, DfgDataType::packed(catWidth)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ಧ್ರುವ uint64_t tComp = m_sccInfo.get(*termp);
            ಧ್ರುವ uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.add(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        ಪ್ರತಿ (DfgVertex* ಧ್ರುವ termp : termps) {
            ಯದಿ (DfgSel* ಧ್ರುವ selp = termp->cast<DfgSel>()) {
                ಯದಿ (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        ಯದಿ (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    ಶೂನ್ಯ main(DfgVertexVar& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        ಯದಿ (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fixUpPacked(var);
        } ಅನ್ಯಥಾ ಯದಿ (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ಧ್ರುವ uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](DfgVertex& sink) {
                // Ignore if sink is not part of same cycle
                ಯದಿ (m_sccInfo.get(sink) != component) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Only handle ArraySels with constant index
                DfgArraySel* ಧ್ರುವ aselp = sink.cast<DfgArraySel>();
                ಯದಿ (!aselp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                ಯದಿ (!aselp->bitp()->is<DfgConst>()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Fix up the word
                fixUpPacked(*aselp);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    FixUp(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

ಸಾರ್ವಜನಿಕ:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ಸ್ಥಿರ size_t apply(DfgGraph& dfg, SccInfo& sccInfo) {
        ಯದಿ (!sccInfo.isCyclic()) ಪ್ರತ್ಯಾಗಮ 0;

        FixUp fixUp{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        ಪ್ರತಿ (DfgVertexVar& vtx : dfg.varVertices()) {
            ಯದಿ (!sccInfo.get(vtx)) ಅನುವರ್ತ;
            fixUp.main(vtx);
            ಯದಿ (!sccInfo.isCyclic()) ಭಙ್ಗ;
        }
        // Return the number of improvements made
        ಪ್ರತ್ಯಾಗಮ fixUp.m_nImprovements;
    }
};

ಶೂನ್ಯ breakCycles(DfgGraph& dfg, V3DfgBreakCyclesContext& ctx) {
    // Shorthand for dumping graph at given dump level
    ಧ್ರುವ auto dump = [&](ಪೂರ್ಣಾಙ್ಕ level, ಧ್ರುವ DfgGraph& dfg, ಧ್ರುವ std::string& name) {
        ಯದಿ (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    size_t nImprovements = 0;
    size_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    ಕುರು {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ಧ್ರುವ size_t nFixed = FixUp::apply(dfg, sccInfo);
        ಯದಿ (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        ಯದಿ (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        ಯದಿ (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            ಪ್ರತ್ಯಾಗಮ;
        }
    } ಯಾವತ್ (nImprovements != prevNImprovements);

    // Debug dump
    ಯದಿ (dumpDfgLevel() >= 9) {
        ಧ್ರುವ SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ಧ್ರುವ DfgVertex& vtx) {
            ಪ್ರತ್ಯಾಗಮ sccInfo.get(vtx);  //
        });
    }

    // Accounting
    ಯದಿ (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } ಅನ್ಯಥಾ {
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
    std::vector<DfgVertexVar*> varps;
    ಪ್ರತಿ (DfgVertexVar& vtx : dfg.varVertices()) {
        ಯದಿ (!vtx.tmpForp()) varps.ತಳ್ಳು_back(&vtx);
    }
    std::stable_partition(varps.begin(), varps.end(), [](ಧ್ರುವ DfgVertexVar* varp) {
        ಪ್ರತ್ಯಾಗಮ varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    ಪ್ರತಿ (DfgVertexVar* ಧ್ರುವ varp : varps) {
        // Stop as soon as it becomes acyclic
        ಯದಿ (!sccInfo.isCyclic()) ಭಙ್ಗ;

        // Ignore if variable is not part of a cycle
        ಯದಿ (!sccInfo.get(*varp)) ಅನುವರ್ತ;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* ಧ್ರುವ prevp = ನೂತನ DfgPrev{dfg, varp->vscp()};
        sccInfo.add(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    ಯದಿ (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
ಶೂನ್ಯ V3DfgPasses::breakCycles(DfgGraph& dfg, V3DfgContext& ctx) {
    V3DfgBreakCycles::breakCycles(dfg, ctx.m_breakCyclesContext);
    ಯದಿ (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::removeUnused(dfg);
}
