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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3DfgPasses.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3Hash.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <fstream>
#অন্তর্ভাৱ <queue>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

নামাকাশ V3DfgBreakCycles {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
শ্ৰেণী SccInfo final {
    DfgGraph& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ধ্রুৱ DfgVertex*, uint64_t> m_vtx2Scc;
    size_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

সার্ৱজনিক:
    SccInfo(DfgGraph& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.forEachVertex([&](ধ্রুৱ DfgVertex& vtx) {
            ধ্রুৱ uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            যদি (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    তার্কিক isCyclic() ধ্রুৱ { ঘূৰাই m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ধ্রুৱ DfgVertex& vtx) ধ্রুৱ { ঘূৰাই m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    শূন্য add(ধ্রুৱ DfgVertex& vtx, uint64_t sccIdx) {
        ধ্রুৱ তার্কিক newEntry = m_vtx2Scc.emplace(&vtx, sccIdx).second;
        UASSERT_OBJ(newEntry, &vtx, "Vertex already inserted");
        যদি (sccIdx) ++m_nCyclicVertices;
    }

    তার্কিক stillCyclicFwd(ধ্রুৱ DfgVertex& vtx) ধ্রুৱ {
        // If it only reads non-cyclic vertices, it has also become acyclic
        ঘূৰাই vtx.foreachSource([&](ধ্রুৱ DfgVertex& src) {  //
            ঘূৰাই m_vtx2Scc.at(&src);
        });
    }

    তার্কিক stillCyclicBwd(ধ্রুৱ DfgVertex& vtx) ধ্রুৱ {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        ঘূৰাই vtx.foreachSink([&](ধ্রুৱ DfgVertex& dst) {  //
            ঘূৰাই m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    শূন্য updateAcyclic(ধ্রুৱ DfgVertex& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ধ্রুৱ DfgVertex& dst) {
            // Sink was known to be acyclic, stop
            যদি (!m_vtx2Scc.at(&dst)) ঘূৰাই মিছা;
            যদি (!stillCyclicFwd(dst)) updateAcyclic(dst);
            ঘূৰাই মিছা;
        });
        // Propagate the update through the graph backward
        vtx.foreachSource([&](ধ্রুৱ DfgVertex& src) {
            // Source was known to be acyclic, stop
            যদি (!m_vtx2Scc.at(&src)) ঘূৰাই মিছা;
            যদি (!stillCyclicBwd(src)) updateAcyclic(src);
            ঘূৰাই মিছা;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    শূন্য updateReplacement(ধ্রুৱ DfgVertex& vtx, DfgVertex& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        যদি (!get(replacement)) {
            replacement.foreachSink([&](DfgVertex& dst) {
                যদি (!get(dst)) ঘূৰাই মিছা;
                যদি (!stillCyclicFwd(dst)) updateAcyclic(dst);
                ঘূৰাই মিছা;
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
    শূন্য validate() ধ্রুৱ {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.forEachVertex([&](ধ্রুৱ DfgVertex& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

শ্ৰেণী TraceDriver final : সার্ৱজনিক DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    সংরচনা Cache_চাবি final {
        DfgVertex* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_চাবি() = delete;
        Cache_চাবি(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        সংরচনা Hash final {
            size_t operator()(ধ্রুৱ Cache_চাবি& item) ধ্রুৱ {
                // cppcheck-suppress unreadVariable
                V3Hash hash{item.m_vtxp};
                hash += item.m_lsb;
                hash += item.m_msb;
                ঘূৰাই hash.মান();
            }
        };

        সংরচনা Equal final {
            তার্কিক operator()(ধ্রুৱ Cache_চাবি& a, ধ্রুৱ Cache_চাবি& b) ধ্রুৱ {
                ঘূৰাই a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
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
    std::unordered_map<Cache_চাবি, DfgVertex*, Cache_চাবি::Hash, Cache_চাবি::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_চাবি, DfgVertex*, Cache_চাবি::Hash, Cache_চাবি::Equal> m_boundaryCache;

#ifdef VL_DEBUG
    std::ofstream m_সাৰি_Coverage_File;  // Line coverage file, just for testing
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
    Vertex* make(ধ্রুৱ DfgVertex* refp, uint32_t width) {
        static_assert(std::is_base_of<DfgVertex, Vertex>::মান  //
                          && !std::is_base_of<DfgVertexVar, Vertex>::মান,
                      "Should only make operation vertices and constants");

        যদি VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, Vertex>::মান) {
            DfgConst* ধ্রুৱ vtxp = নূতন DfgConst{m_dfg, refp->fileline(), width, 0};
            m_sccInfo.add(*vtxp, 0);
            ঘূৰাই reinterpret_cast<Vertex*>(vtxp);
        } অন্যথা {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            using Vtx = typename std::conditional<std::is_same<DfgConst, Vertex>::মান, DfgSel,
                                                  Vertex>::type;
            Vtx* ধ্রুৱ vtxp = নূতন Vtx{m_dfg, refp->fileline(), DfgDataType::packed(width)};
            m_sccInfo.add(*vtxp, 0);
            ঘূৰাই reinterpret_cast<Vertex*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    DfgVertexVar* createTmp(ধ্রুৱ ৱর্ণ* prefix, DfgVertex* vtxp) {
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
        DfgVertex::ScopeCache scopeCache;
        AstScope* ধ্রুৱ scopep = vtxp->scopep(scopeCache);
        DfgVertexVar* ধ্রুৱ varp = m_dfg.makeNewVar(flp, prefix, vtxp->dtype(), scopep);
        varp->vscp()->varp()->isInternal(সঁচা);
        varp->tmpForp(varp->vscp());
        m_sccInfo.add(*varp, 0);
        ঘূৰাই varp;
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
        DfgVertex* ধ্রুৱ startVtxp = vtxp;
        ধ্রুৱ uint32_t startMsb = msb;
        ধ্রুৱ uint32_t startLsb = lsb;
        ধ্রুৱ size_t pendingBegin = m_pendingps.size();
        DfgVertex* resp = nullptr;

        যেতিয়ালৈ (সঁচা) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->size() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            যদি (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                DfgVertex*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(nullptr))
                          .first->second;
                যদি (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    যদি (DfgVertexSplice* ধ্রুৱ splicep = respr->cast<DfgVertexSplice>()) {
                        DfgVertexVar* ধ্রুৱ tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    যদি (msb != respr->width() - 1 || lsb != 0) {
                        DfgSel* ধ্রুৱ selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                ভঙ্গ;
            }

            // Normal cache for intra-component traces
            DfgVertex*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(nullptr))
                                    .first->second;

            // Trace the vertex
            যদি (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } অন্যথা {
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
                যদি (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ঠেলো_back(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    অনুৱর্ত;
                }
                respr = m_resp;
            }
            resp = respr;
            ভঙ্গ;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, startVtxp, "Tracing driver failed for " << startVtxp->typeName());
        UASSERT_OBJ(resp->width() == (startMsb - startLsb + 1), startVtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        বাবে (size_t i = pendingBegin; i < m_pendingps.size(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        ঘূৰাই resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    DfgVertex* traceSameIdx(DfgVertex* ধ্রুৱ vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = nullptr;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->width() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        ঘূৰাই m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    DfgVertex* trace_ঠেলো_Idx(DfgVertex* ধ্রুৱ vtxp, ধ্রুৱ uint32_t idx) {
        ধ্রুৱ size_t nIdxs = m_idxs.size();
        m_idxs.ঠেলো_back(idx);
        DfgVertex* ধ্রুৱ resp = traceSameIdx(vtxp);
        m_idxs.বেৰ_কৰো_back();
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        ঘূৰাই resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    DfgVertex* trace_বেৰ_কৰো_Idx(DfgVertex* ধ্রুৱ vtxp) {
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        ধ্রুৱ size_t nIdxs = m_idxs.size();
        ধ্রুৱ uint32_t idx = m_idxs.back();
        m_idxs.বেৰ_কৰো_back();
        DfgVertex* ধ্রুৱ resp = m_idxs.empty() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ঠেলো_back(idx);
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        ঘূৰাই resp;
    }

    template <typename Vertex>
    Vertex* trace_Bitwise_দ্বিমিক(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_দ্বিমিক, Vertex>::মান,
                      "Should only call on DfgVertexBinary");
        Vertex* ধ্রুৱ resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        ঘূৰাই resp;
    }

    template <typename Vertex>
    DfgVertex* traceAddSub(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_দ্বিমিক, Vertex>::মান,
                      "Should only call on DfgVertexBinary");
        Vertex* ধ্রুৱ opp = make<Vertex>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ধ্রুৱ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        ঘূৰাই selp;
    }

    template <typename Vertex>
    Vertex* traceReduction(Vertex* vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::মান,
                      "Should only call on DfgVertexUnary");
        Vertex* ধ্রুৱ resp = make<Vertex>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->width() - 1, 0));
        ঘূৰাই resp;
    }

    template <typename Vertex>
    Vertex* traceCmp(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_দ্বিমিক, Vertex>::মান,
                      "Should only call on DfgVertexBinary");
        Vertex* ধ্রুৱ resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        ঘূৰাই resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define TRACE_RESULT_COVER() \
    কৰা { \
        যদি (VL_UNLIKELY(m_সাৰি_Coverage_File.is_open())) m_সাৰি_Coverage_File << __LINE__ << '\n'; \
    } যেতিয়ালৈ (মিছা)
#অন্যথা
#define TRACE_RESULT_COVER() \
    কৰা { \
    } যেতিয়ালৈ (মিছা)
#endif

#define RETURN_RESULT(vtxp) \
    কৰা { \
        m_resp = (vtxp); \
        TRACE_RESULT_COVER(); \
        ঘূৰাই; \
    } যেতিয়ালৈ (মিছা)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#define RETURN_RESULT_TAIL(srcp, smsb, slsb) \
    কৰা { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        DfgVertex* ধ্রুৱ tailp = (srcp); \
        ধ্রুৱ uint32_t tailMsb = (smsb); \
        ধ্রুৱ uint32_t tailLsb = (slsb); \
        m_tailp = tailp; \
        m_msb = tailMsb; \
        m_lsb = tailLsb; \
        TRACE_RESULT_COVER(); \
        ঘূৰাই; \
    } যেতিয়ালৈ (মিছা)

    // VISITORS
    শূন্য visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    শূন্য visit(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        DfgVertex* defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        সংরচনা Driver final {
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
        তার্কিক tryWholeDefault = defaultp;
        DfgVertex* coverp = nullptr;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lsb) {
            ধ্রুৱ uint32_t msb = lsb + src.width() - 1;
            drivers.emplace_back(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            যদি (m_msb >= lsb && msb >= m_lsb) tryWholeDefault = মিছা;
            // If it does not cover the whole searched bit range, move on
            যদি (m_lsb < lsb || msb < m_msb) ঘূৰাই মিছা;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            ঘূৰাই সঁচা;
        });

        // Trace the driver covering the whole searched range, if there is one
        যদি (coverp) RETURN_RESULT_TAIL(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        যদি (tryWholeDefault) RETURN_RESULT_TAIL(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::sort(drivers.begin(), drivers.end(), [](ধ্রুৱ Driver& a, ধ্রুৱ Driver& b) {  //
            ঘূৰাই a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::vector<DfgVertex*> termps;
        uint32_t lsb = m_lsb;
        বাবে (ধ্রুৱ Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            যদি (lsb > driver.m_msb) অনুৱর্ত;
            // Driver is above the searched MSB, done
            যদি (driver.m_lsb > m_msb) ভঙ্গ;
            // Gap below this driver, trace default to fill it
            যদি (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_back(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ধ্রুৱ uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_back(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        যদি (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_back(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.size() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        DfgVertex* resp = termps.front();
        বাবে (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* ধ্রুৱ termp = termps[i];
            DfgConcat* ধ্রুৱ catp = make<DfgConcat>(termp, resp->width() + termp->width());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_RESULT(resp);
    }

    শূন্য visit(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        DfgVertex* ধ্রুৱ defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ধ্রুৱ uint32_t idx = m_idxs.back();
        যদি (DfgVertex* ধ্রুৱ driverp = vtxp->driverAt(idx)) {
            DfgVertex* ধ্রুৱ srcp = driverp->as<DfgUnitArray>()->srcp();
            যদি (srcp->is<DfgVertexSplice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_RESULT(trace_বেৰ_কৰো_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_RESULT(traceSameIdx(defaultp));
    }

    শূন্য visit(DfgVertexVar* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        DfgVertex* ধ্রুৱ defaultp = vtxp->defaultp();
        DfgVertex* ধ্রুৱ drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        যদি (srcp && srcp->is<DfgVertexSplice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_RESULT(m_idxs.empty() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    শূন্য visit(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_RESULT(trace_ঠেলো_Idx(vtxp->fromp(), idxp->toU32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.empty(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ধ্রুৱ resp = make<DfgArraySel>(vtxp, vtxp->width());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->width() - 1, 0));
        DfgSel* ধ্রুৱ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    শূন্য visit(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.back() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_RESULT(trace_বেৰ_কৰো_Idx(vtxp->srcp()));
    }

    শূন্য visit(DfgConcat* vtxp) override {
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        ধ্রুৱ uint32_t rWidth = rhsp->width();
        // If the traced bits are wholly in the RHS
        যদি (rWidth > m_msb) RETURN_RESULT_TAIL(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        যদি (m_lsb >= rWidth) RETURN_RESULT_TAIL(lhsp, m_msb - rWidth, m_lsb - rWidth);
        // The traced bit spans both sides, trace both
        DfgConcat* ধ্রুৱ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, rWidth - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - rWidth, 0));
        RETURN_RESULT(resp);
    }

    শূন্য visit(DfgRep* vtxp) override {
        DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        ধ্রুৱ uint32_t sWidth = srcp->width();
        // If we need more bits than the source, then we need the whole source
        যদি (m_msb - m_lsb + 1 > sWidth) {
            DfgRep* ধ্রুৱ repp = make<DfgRep>(vtxp, vtxp->width());
            repp->srcp(trace(srcp, sWidth - 1, 0));
            DfgSel* ধ্রুৱ resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_RESULT(resp);
        }
        // If the requested bits are within the same repliacted word
        যদি (m_msb / sWidth == m_lsb / sWidth) {
            RETURN_RESULT_TAIL(srcp, m_msb % sWidth, m_lsb % sWidth);
        }
        // The requested bits span two replicated words
        DfgConcat* ধ্রুৱ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, sWidth - 1, m_lsb % sWidth));
        resp->lhsp(trace(srcp, m_msb % sWidth, 0));
        RETURN_RESULT(resp);
    }

    শূন্য visit(DfgExtend* vtxp) override {
        DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        ধ্রুৱ uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        যদি (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        যদি (m_lsb >= sWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ধ্রুৱ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    শূন্য visit(DfgExtendS* vtxp) override {
        DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        ধ্রুৱ uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        যদি (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        যদি (m_lsb >= sWidth) {
            যদি (m_msb == m_lsb) RETURN_RESULT_TAIL(srcp, sWidth - 1, sWidth - 1);
            DfgExtendS* ধ্রুৱ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, sWidth - 1, sWidth - 1));
            RETURN_RESULT(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ধ্রুৱ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    শূন্য visit(DfgSel* vtxp) override {
        ধ্রুৱ uint32_t lsb = vtxp->lsb();
        RETURN_RESULT_TAIL(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    শূন্য visit(DfgNot* vtxp) override {
        DfgNot* ধ্রুৱ resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    শূন্য visit(DfgAnd* vtxp) override { RETURN_RESULT(trace_Bitwise_দ্বিমিক(vtxp)); }
    শূন্য visit(DfgOr* vtxp) override { RETURN_RESULT(trace_Bitwise_দ্বিমিক(vtxp)); }
    শূন্য visit(DfgXor* vtxp) override { RETURN_RESULT(trace_Bitwise_দ্বিমিক(vtxp)); }
    শূন্য visit(DfgAdd* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    শূন্য visit(DfgSub* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    শূন্য visit(DfgRedAnd* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    শূন্য visit(DfgRedOr* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    শূন্য visit(DfgRedXor* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    শূন্য visit(DfgEq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    শূন্য visit(DfgNeq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    শূন্য visit(DfgLt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    শূন্য visit(DfgLte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    শূন্য visit(DfgGt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    শূন্য visit(DfgGte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }

    শূন্য visit(DfgShiftRS* vtxp) override {
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ধ্রুৱ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ধ্রুৱ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            যদি (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            যদি (m_lsb >= lowerWidth) {
                DfgExtendS* ধ্রুৱ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->width() - 1, lhsp->width() - 1));
                RETURN_RESULT(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ধ্রুৱ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftRS* ধ্রুৱ shiftrsp = make<DfgShiftRS>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        যদি (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrsp);
        DfgSel* ধ্রুৱ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    শূন্য visit(DfgShiftR* vtxp) override {
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ধ্রুৱ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ধ্রুৱ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            যদি (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            যদি (m_lsb >= lowerWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ধ্রুৱ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftR* ধ্রুৱ shiftrp = make<DfgShiftR>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        যদি (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrp);
        DfgSel* ধ্রুৱ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    শূন্য visit(DfgShiftL* vtxp) override {
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ধ্রুৱ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ধ্রুৱ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? vtxp->width() : shiftAmnt;
            // If the traced bits are wholly in the input
            যদি (m_lsb >= lowerWidth) {
                RETURN_RESULT_TAIL(lhsp, m_msb - shiftAmnt, m_lsb - shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            যদি (lowerWidth > m_msb) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ধ্রুৱ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->width() - (m_msb - lowerWidth + 1)));
            resp->lhsp(trace(lhsp, m_msb - shiftAmnt, lowerWidth - shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftL* ধ্রুৱ shiftlp = make<DfgShiftL>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        যদি (m_lsb == 0) RETURN_RESULT(shiftlp);
        DfgSel* ধ্রুৱ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    শূন্য visit(DfgCond* vtxp) override {
        DfgCond* ধ্রুৱ resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->width() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    শূন্য visit(DfgMatchMasked* vtxp) override {
        DfgMatchMasked* ধ্রুৱ resp = make<DfgMatchMasked>(vtxp, vtxp->width());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->width() - 1, 0));
        DfgSel* ধ্রুৱ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

#undef RETURN_RESULT
#undef TRACE_RESULT_COVER
#undef RETURN_RESULT_TAIL

সার্ৱজনিক:
    // CONSTRUCTOR
    TraceDriver(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#ifdef VL_DEBUG
        যদি (v3Global.opt.debugCheck()) {
            m_সাৰি_Coverage_File.open(  //
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
        ঘূৰাই trace(&vtx, lsb + width - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
শ্ৰেণী BitMask final {
    // TYPES
    ধ্রুৱ DfgDataType& m_dtype;
    সঙ্ঘ {
        V3_সংখ্যা m_অঙ্ক;  // The bits, if packed
        std::vector<BitMask> m_sub;  // The per element masks, if aggregate
    };

সার্ৱজনিক:
    // CONSTRUCTOR
    BitMask(File_সাৰি* flp, ধ্রুৱ DfgDataType& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        যদি (m_dtype.isPacked()) {
            নূতন (&m_অঙ্ক) V3_সংখ্যা{flp, static_cast<পূর্ণাঙ্ক>(m_dtype.size()), 0};
        } অন্যথা {
            নূতন (&m_sub) std::vector<BitMask>{};
            m_sub.reserve(m_dtype.size());
            বাবে (uint32_t i = 0; i < m_dtype.size(); ++i)
                m_sub.emplace_back(flp, m_dtype.elemDtype());
        }
    }
    BitMask(ধ্রুৱ DfgVertex& vtx)
        : BitMask{vtx.fileline(), vtx.dtype()} {}
    BitMask(ধ্রুৱ BitMask& other)
        : m_dtype{other.m_dtype} {
        যদি (m_dtype.isPacked()) {
            নূতন (&m_অঙ্ক) V3_সংখ্যা{other.m_অঙ্ক};
        } অন্যথা {
            নূতন (&m_sub) std::vector<BitMask>{other.m_sub};
        }
    }
    BitMask& operator=(ধ্রুৱ BitMask& other) {
        যদি (এতত্ != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            যদি (other.m_dtype.isPacked()) {
                m_অঙ্ক = other.m_অঙ্ক;
            } অন্যথা {
                m_sub = other.m_sub;
            }
        }
        ঘূৰাই *এতত্;
    }
    ~BitMask() {
        যদি (m_dtype.isPacked()) {
            m_অঙ্ক.~V3_সংখ্যা();
        } অন্যথা {
            m_sub.~vector<BitMask>();
        }
    }

    // METHODS
    V3_সংখ্যা& অঙ্ক() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ঘূৰাই m_অঙ্ক;
    }
    ধ্রুৱ V3_সংখ্যা& অঙ্ক() ধ্রুৱ {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ঘূৰাই m_অঙ্ক;
    }
    std::vector<BitMask>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        ঘূৰাই m_sub;
    }

    তার্কিক isZero() ধ্রুৱ {
        যদি (m_dtype.isPacked()) ঘূৰাই m_অঙ্ক.isEqZero();

        বাবে (ধ্রুৱ BitMask& sub : m_sub) {
            যদি (!sub.isZero()) ঘূৰাই মিছা;
        }
        ঘূৰাই সঁচা;
    }
    তার্কিক isOnes() ধ্রুৱ {
        যদি (m_dtype.isPacked()) ঘূৰাই m_অঙ্ক.isEqAllOnes();

        বাবে (ধ্রুৱ BitMask& sub : m_sub) {
            যদি (!sub.isOnes()) ঘূৰাই মিছা;
        }
        ঘূৰাই সঁচা;
    }
    শূন্য setZero() {
        যদি (m_dtype.isPacked()) {
            m_অঙ্ক.setAllBits0();
        } অন্যথা {
            বাবে (BitMask& sub : m_sub) sub.setZero();
        }
    }
    শূন্য setOnes() {
        যদি (m_dtype.isPacked()) {
            m_অঙ্ক.setAllBits1();
        } অন্যথা {
            বাবে (BitMask& sub : m_sub) sub.setOnes();
        }
    }

    তার্কিক operator==(ধ্রুৱ BitMask& other) ধ্রুৱ {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        যদি (m_dtype.isPacked()) ঘূৰাই m_অঙ্ক.isCaseEq(other.m_অঙ্ক);
        ঘূৰাই m_sub == other.m_sub;
    }
    তার্কিক operator!=(ধ্রুৱ BitMask& other) ধ্রুৱ { ঘূৰাই !(*এতত্ == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    তার্কিক isContractionOf(ধ্রুৱ BitMask& that) ধ্রুৱ {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        যদি (m_dtype.isPacked()) {
            ধ্রুৱ size_t শব্দগুলি = VL_শব্দগুলি_I(m_dtype.size());
            বাবে (size_t i = 0; i < শব্দগুলি; ++i) {
                যদি (~m_অঙ্ক.edataWord(i) & that.m_অঙ্ক.edataWord(i)) ঘূৰাই সঁচা;
            }
            ঘূৰাই মিছা;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        বাবে (size_t i = 0; i < m_sub.size(); ++i) {
            যদি (m_sub[i].isContractionOf(that.m_sub[i])) ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }

    std::string toString() ধ্রুৱ {
        যদি (m_dtype.isPacked()) ঘূৰাই "0x" + m_অঙ্ক.displayed(m_অঙ্ক.fileline(), "%x");
        std::string result;
        result += "{";
        বাবে (ধ্রুৱ BitMask& sub : m_sub) {
            যদি (&sub != &m_sub.front()) result += ", ";
            result += sub.toString();
        }
        result += "}";
        ঘূৰাই result;
    }
};

শ্ৰেণী IndependentBits final : সার্ৱজনিক DfgVisitor {
    // TYPES
    সংরচনা VertexState final {
        size_t m_rpo_সংখ্যা = 0;  // Reverse Postorder Number of vertex
        তার্কিক m_isOnWorkList = মিছা;  // Whether the vertex is on the work list
    };

    // STATE
    ধ্রুৱ SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ধ্রুৱ DfgVertex*, BitMask> m_vtxp2Mask;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_workQueue;
    std::unordered_map<ধ্রুৱ DfgVertex*, VertexState> m_vtxp2State;  // State of each vertex

#ifdef VL_DEBUG
    std::ofstream m_সাৰি_Coverage_File;  // Line coverage file, just for testing
#endif

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    BitMask& mask(ধ্রুৱ DfgVertex& vtx) {
        // Look up (or create) mask for 'vtx'
        ঘূৰাই m_vtxp2Mask
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .first->second;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define MASK(vtxp) \
    ([এতত্](ধ্রুৱ DfgVertex& vtx) -> BitMask& { \
        যদি (VL_UNLIKELY(m_সাৰি_Coverage_File.is_open())) m_সাৰি_Coverage_File << __LINE__ << '\n'; \
        ঘূৰাই mask(vtx); \
    }(*vtxp))
#অন্যথা
#define MASK(vtxp) mask(*vtxp)
#endif

    // Clear all bits at or below the most significant clear bit
    শূন্য floodTowardsLsb(V3_সংখ্যা& অঙ্ক) {
        বাবে (পূর্ণাঙ্ক i = অঙ্ক.width() - 1; i >= 0; --i) {
            যদি (অঙ্ক.bitIs1(i)) অনুৱর্ত;
            অঙ্ক.opSetRange(0, i + 1, '0');
            ভঙ্গ;
        }
    }

    // Clear all bits at or above the least significant clear bit
    শূন্য floodTowardsMsb(V3_সংখ্যা& অঙ্ক) {
        বাবে (পূর্ণাঙ্ক i = 0; i < অঙ্ক.width(); ++i) {
            যদি (অঙ্ক.bitIs1(i)) অনুৱর্ত;
            অঙ্ক.opSetRange(i, অঙ্ক.width() - i, '0');
            ভঙ্গ;
        }
    }

    শূন্য propagateFromDriver(BitMask& m, ধ্রুৱ DfgVertex* srcp) {
        // If there is no driver, we are done
        যদি (!srcp) ঘূৰাই;
        // If it is driven by a splice, we need to combine the masks of the drivers
        যদি (ধ্রুৱ DfgSplicePacked* ধ্রুৱ splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ধ্রুৱ DfgVertex& src, uint32_t lo) {
                m.অঙ্ক().opSelInto(MASK(&src).অঙ্ক(), lo, src.width());
                ঘূৰাই মিছা;
            });
            ঘূৰাই;
        }
        যদি (ধ্রুৱ DfgSpliceArray* ধ্রুৱ splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ধ্রুৱ DfgVertex& src, uint32_t lo) {
                যদি (ধ্রুৱ DfgUnitArray* ধ্রুৱ uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = MASK(uap->srcp());
                } অন্যথা {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                ঘূৰাই মিছা;
            });
            ঘূৰাই;
        }
        // Otherwise, we just use the mask of the single driver
        m = MASK(srcp);
    }

    // VISITORS
    শূন্য visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    শূন্য visit(DfgVertexVar* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        যদি (vtxp->isVolatile()) ঘূৰাই;

        BitMask& m = MASK(vtxp);
        DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        DfgVertex* ধ্রুৱ defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        যদি (defaultp) m = MASK(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    শূন্য visit(DfgVertexSplice* vtxp) override {
        propagateFromDriver(MASK(vtxp), vtxp);  // Needed to continue traversal
    }

    শূন্য visit(DfgUnitArray* vtxp) override { MASK(vtxp).sub().at(0) = MASK(vtxp->srcp()); }

    শূন্য visit(DfgArraySel* vtxp) override {
        DfgVertex* ধ্রুৱ fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ idxp = vtxp->bitp()->cast<DfgConst>()) {
            MASK(vtxp) = MASK(vtxp->fromp()).sub().at(idxp->toSizeT());
            ঘূৰাই;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        যদি (MASK(vtxp->bitp()).isOnes() && MASK(fromp).isOnes()) MASK(vtxp).setOnes();
    }

    শূন্য visit(DfgConcat* vtxp) override {
        ধ্রুৱ DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        ধ্রুৱ DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
        m.opSelInto(MASK(rhsp).অঙ্ক(), 0, rhsp->width());
        m.opSelInto(MASK(lhsp).অঙ্ক(), rhsp->width(), lhsp->width());
    }

    শূন্য visit(DfgRep* vtxp) override {
        ধ্রুৱ uint32_t গুনতি = vtxp->গুনতি();
        ধ্রুৱ DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        ধ্রুৱ uint32_t sWidth = srcp->width();
        V3_সংখ্যা& vMask = MASK(vtxp).অঙ্ক();
        V3_সংখ্যা& sMask = MASK(srcp).অঙ্ক();
        বাবে (uint32_t i = 0; i < গুনতি; ++i) vMask.opSelInto(sMask, i * sWidth, sWidth);
    }

    শূন্য visit(DfgSel* vtxp) override {
        ধ্রুৱ uint32_t lsb = vtxp->lsb();
        ধ্রুৱ uint32_t msb = lsb + vtxp->width() - 1;
        MASK(vtxp).অঙ্ক().opSel(MASK(vtxp->fromp()).অঙ্ক(), msb, lsb);
    }

    শূন্য visit(DfgExtend* vtxp) override {
        ধ্রুৱ DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        ধ্রুৱ uint32_t sWidth = srcp->width();
        V3_সংখ্যা& s = MASK(srcp).অঙ্ক();
        V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, '1');
    }
    শূন্য visit(DfgExtendS* vtxp) override {
        ধ্রুৱ DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        ধ্রুৱ uint32_t sWidth = srcp->width();
        V3_সংখ্যা& s = MASK(srcp).অঙ্ক();
        V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, s.bitIs0(sWidth - 1) ? '0' : '1');
    }

    শূন্য visit(DfgNot* vtxp) override {  //
        MASK(vtxp) = MASK(vtxp->srcp());
    }

    শূন্য visit(DfgAnd* vtxp) override {  //
        MASK(vtxp).অঙ্ক().opAnd(MASK(vtxp->lhsp()).অঙ্ক(), MASK(vtxp->rhsp()).অঙ্ক());
    }
    শূন্য visit(DfgOr* vtxp) override {  //
        MASK(vtxp).অঙ্ক().opAnd(MASK(vtxp->lhsp()).অঙ্ক(), MASK(vtxp->rhsp()).অঙ্ক());
    }
    শূন্য visit(DfgXor* vtxp) override {  //
        MASK(vtxp).অঙ্ক().opAnd(MASK(vtxp->lhsp()).অঙ্ক(), MASK(vtxp->rhsp()).অঙ্ক());
    }

    শূন্য visit(DfgAdd* vtxp) override {
        V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
        m.opAnd(MASK(vtxp->lhsp()).অঙ্ক(), MASK(vtxp->rhsp()).অঙ্ক());
        floodTowardsMsb(m);
    }
    শূন্য visit(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
        m.opAnd(MASK(vtxp->lhsp()).অঙ্ক(), MASK(vtxp->rhsp()).অঙ্ক());
        floodTowardsMsb(m);
    }

    শূন্য visit(DfgRedAnd* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }
    শূন্য visit(DfgRedOr* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }
    শূন্য visit(DfgRedXor* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }

    শূন্য visit(DfgEq* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }
    শূন্য visit(DfgNeq* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }
    শূন্য visit(DfgLt* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }
    শূন্য visit(DfgLte* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }
    শূন্য visit(DfgGt* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }
    শূন্য visit(DfgGte* vtxp) override {
        ধ্রুৱ তার্কিক independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        যদি (independent) MASK(vtxp).setOnes();
    }

    শূন্য visit(DfgShiftRS* vtxp) override {
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        ধ্রুৱ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        যদি (DfgConst* ধ্রুৱ rConstp = rhsp->cast<DfgConst>()) {
            ধ্রুৱ uint32_t shiftAmount = rConstp->toU32();
            যদি (shiftAmount >= width) {
                যদি (MASK(lhsp).অঙ্ক().bitIs1(width - 1)) { MASK(vtxp).setOnes(); }
            } অন্যথা {
                V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
                m.opShiftRS(MASK(lhsp).অঙ্ক(), rConstp->অঙ্ক(), width);
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            ঘূৰাই;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        যদি (MASK(rhsp).isOnes()) {
            V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
            m = MASK(lhsp).অঙ্ক();
            floodTowardsLsb(m);
        }
    }

    শূন্য visit(DfgShiftR* vtxp) override {
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        ধ্রুৱ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        যদি (DfgConst* ধ্রুৱ rConstp = rhsp->cast<DfgConst>()) {
            ধ্রুৱ uint32_t shiftAmount = rConstp->toU32();
            যদি (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } অন্যথা {
                V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
                m.opShiftR(MASK(lhsp).অঙ্ক(), rConstp->অঙ্ক());
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            ঘূৰাই;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        যদি (MASK(rhsp).isOnes()) {
            V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
            m = MASK(lhsp).অঙ্ক();
            floodTowardsLsb(m);
        }
    }

    শূন্য visit(DfgShiftL* vtxp) override {
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        ধ্রুৱ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        যদি (DfgConst* ধ্রুৱ rConstp = rhsp->cast<DfgConst>()) {
            ধ্রুৱ uint32_t shiftAmount = rConstp->toU32();
            যদি (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } অন্যথা {
                V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
                m.opShiftL(MASK(lhsp).অঙ্ক(), rConstp->অঙ্ক());
                m.opSetRange(0, shiftAmount, '1');
            }
            ঘূৰাই;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        যদি (MASK(rhsp).isOnes()) {
            V3_সংখ্যা& m = MASK(vtxp).অঙ্ক();
            m = MASK(lhsp).অঙ্ক();
            floodTowardsMsb(m);
        }
    }

    শূন্য visit(DfgCond* vtxp) override {
        যদি (MASK(vtxp->condp()).isOnes()) {
            MASK(vtxp).অঙ্ক().opAnd(MASK(vtxp->thenp()).অঙ্ক(), MASK(vtxp->elsep()).অঙ্ক());
        }
    }

    শূন্য visit(DfgMatchMasked* vtxp) override {
        যদি (MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->matchp()).isOnes()) { MASK(vtxp).setOnes(); }
    }

#undef MASK

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    শূন্য enqueueSinks(DfgVertex& vtx) {
        vtx.foreachSink([&](DfgVertex& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            যদি (!m_sccInfo.get(sink)) ঘূৰাই মিছা;
            // Otherwise just enqueue it
            VertexState& state = m_vtxp2State.at(&sink);
            যদি (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_workQueue.ঠেলো(state.m_rpo_সংখ্যা);
            }
            state.m_isOnWorkList = সঁচা;
            ঘূৰাই মিছা;
        });
    };

    শূন্য dfsPostOrder(std::unordered_set<DfgVertex*>& visited,  //
                      std::vector<DfgVertex*>& postOrderEnumeration,  //
                      DfgVertex& vtx) {
        // Mark visited, skip if already visited
        যদি (!visited.ঢোকাও(&vtx).second) ঘূৰাই;
        // Visit un-visited successors
        vtx.foreachSink([&](DfgVertex& snk) {
            dfsPostOrder(visited, postOrderEnumeration, snk);
            ঘূৰাই মিছা;
        });
        // Add to post order enumeration
        postOrderEnumeration.emplace_back(&vtx);
    };

    IndependentBits(DfgGraph& dfg, ধ্রুৱ SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#ifdef VL_DEBUG
        যদি (v3Global.opt.debugCheck()) {
            m_সাৰি_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::app);
        }
#endif

        // Compute Reverse-Postorder enumeration of vertices
        std::vector<DfgVertex*> rpoEnumeration;
        std::unordered_set<DfgVertex*> visited;
        dfg.forEachVertex([&](DfgVertex& vtx) {
            যদি (!vtx.nInputs()) dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        dfg.forEachVertex([&](DfgVertex& vtx) {  //
            dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        std::reverse(rpoEnumeration.begin(), rpoEnumeration.end());
        UASSERT(rpoEnumeration.size() == dfg.size(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        বাবে (size_t i = 0; i < rpoEnumeration.size(); ++i) {
            VertexState& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_সংখ্যা = i;
            state.m_isOnWorkList = মিছা;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        বাবে (DfgVertex* ধ্রুৱ vtxp : rpoEnumeration) {
            যদি (m_sccInfo.get(*vtxp)) অনুৱর্ত;
            mask(*vtxp).setOnes();
        }
        বাবে (DfgVertex* ধ্রুৱ vtxp : rpoEnumeration) {
            যদি (!m_sccInfo.get(*vtxp)) অনুৱর্ত;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << mask(*vtxp).toString());
            যদি (!mask(*vtxp).isZero()) enqueueSinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        যেতিয়ালৈ (!m_workQueue.empty()) {
            // Grab next item
            DfgVertex* ধ্রুৱ currp = rpoEnumeration[m_workQueue.শীৰ্ষ()];
            m_workQueue.বেৰ_কৰো();
            m_vtxp2State.at(currp).m_isOnWorkList = মিছা;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ধ্রুৱ BitMask& currMask = mask(*currp);
            // Remember current mask so we can check if it changed
            ধ্রুৱ BitMask prevMask = currMask;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            যদি (prevMask != currMask) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prevMask.toString()
                             << "\n      to: " << currMask.toString());
                enqueueSinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                যদি (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!currMask.isContractionOf(prevMask), currp,
                                "Mask should only expand");
                }
            }
        }
    }

সার্ৱজনিক:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    স্থির std::unordered_map<ধ্রুৱ DfgVertex*, BitMask> apply(DfgGraph& dfg,
                                                               ধ্রুৱ SccInfo& sccInfo) {
        ঘূৰাই std::move(IndependentBits{dfg, sccInfo}.m_vtxp2Mask);
    }
};

শ্ৰেণী FixUp final {
    DfgGraph& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ধ্রুৱ std::unordered_map<ধ্রুৱ DfgVertex*, BitMask> m_independentBits
        = IndependentBits::apply(m_dfg, m_sccInfo);
    size_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    স্থির BitMask computeUsedBits(DfgVertex& vtx) {
        BitMask result{vtx};
        vtx.foreachSink([&result](DfgVertex& sink) {
            // If used via a Sel, mark the selected bits used
            যদি (ধ্রুৱ DfgSel* ধ্রুৱ selp = sink.cast<DfgSel>()) {
                result.অঙ্ক().opSetRange(selp->lsb(), selp->width(), '1');
                ঘূৰাই মিছা;
            }
            // Used without a Sel, so all bits are used
            result.setOnes();
            ঘূৰাই সঁচা;
        });
        ঘূৰাই result;
    }

    স্থির std::string debugStr(ধ্রুৱ DfgVertex& vtx) {  // LCOV_EXCL_START
        যদি (ধ্রুৱ DfgArraySel* ধ্রুৱ aselp = vtx.cast<DfgArraySel>()) {
            ধ্রুৱ size_t i = aselp->bitp()->as<DfgConst>()->toSizeT();
            ঘূৰাই debugStr(*aselp->fromp()) + "[" + std::to_string(i) + "]";
        }
        যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = vtx.cast<DfgVertexVar>()) {
            ঘূৰাই varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    শূন্য gatherTerms(std::vector<DfgVertex*>& termps, DfgVertex& vtx, ধ্রুৱ V3_সংখ্যা& indpBits,
                     ধ্রুৱ uint32_t hi, ধ্রুৱ uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        তার্কিক isIndependent = indpBits.bitIs1(lo);
        uint32_t lsb = lo;
        বাবে (uint32_t msb = lo; msb <= hi; ++msb) {
            ধ্রুৱ তার্কিক endRange = msb == hi || isIndependent != indpBits.bitIs1(msb + 1);
            যদি (!endRange) অনুৱর্ত;
            ধ্রুৱ uint32_t width = msb - lsb + 1;
            // Compute the term to use for this range
            যদি (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_back(m_traceDriver.apply(vtx, lsb, width));
            } অন্যথা {
                // If dependent, fall back on using the part of the variable
                DfgSel* ধ্রুৱ selp = নূতন DfgSel{m_dfg, vtx.fileline(), DfgDataType::packed(width)};
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

    শূন্য fixUpPacked(DfgVertex& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ধ্রুৱ BitMask& indpBits = m_independentBits.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indpBits.toString());
        // Can't do anything if all bits are dependent
        যদি (indpBits.isZero()) ঘূৰাই;

        // Figure out which bits of 'vtxp' are used
        ধ্রুৱ BitMask usedBits = computeUsedBits(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << usedBits.toString());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_সংখ্যা usedAndIndependent{vtx.fileline(), static_cast<পূর্ণাঙ্ক>(vtx.width()), 0};
        usedAndIndependent.opAnd(usedBits.অঙ্ক(), indpBits.অঙ্ক());
        যদি (usedAndIndependent.isEqZero()) ঘূৰাই;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::vector<DfgVertex*> termps;

        // Iterate through consecutive used/unused ranges
        File_সাৰি* ধ্রুৱ flp = vtx.fileline();
        ধ্রুৱ uint32_t width = vtx.width();
        তার্কিক isUsed = usedBits.অঙ্ক().bitIs1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        বাবে (uint32_t msb = 0; msb < width; ++msb) {
            ধ্রুৱ তার্কিক endRange = msb == width - 1 || isUsed != usedBits.অঙ্ক().bitIs1(msb + 1);
            যদি (!endRange) অনুৱর্ত;
            যদি (isUsed) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indpBits.অঙ্ক(), msb, lsb);
            } অন্যথা {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ধ্রুৱ constp = নূতন DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.add(*constp, 0);
                termps.emplace_back(constp);
            }
            // Next iteration
            isUsed = !isUsed;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        DfgVertex* replacementp = termps.front();
        ধ্রুৱ uint64_t vComp = m_sccInfo.get(vtx);
        বাবে (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* ধ্রুৱ termp = termps[i];
            ধ্রুৱ uint32_t catWidth = replacementp->width() + termp->width();
            DfgConcat* ধ্রুৱ catp = নূতন DfgConcat{m_dfg, flp, DfgDataType::packed(catWidth)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ধ্রুৱ uint64_t tComp = m_sccInfo.get(*termp);
            ধ্রুৱ uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.add(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        বাবে (DfgVertex* ধ্রুৱ termp : termps) {
            যদি (DfgSel* ধ্রুৱ selp = termp->cast<DfgSel>()) {
                যদি (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        যদি (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    শূন্য main(DfgVertexVar& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        যদি (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fixUpPacked(var);
        } অন্যথা যদি (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ধ্রুৱ uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](DfgVertex& sink) {
                // Ignore if sink is not part of same cycle
                যদি (m_sccInfo.get(sink) != component) ঘূৰাই মিছা;
                // Only handle ArraySels with constant index
                DfgArraySel* ধ্রুৱ aselp = sink.cast<DfgArraySel>();
                যদি (!aselp) ঘূৰাই মিছা;
                যদি (!aselp->bitp()->is<DfgConst>()) ঘূৰাই মিছা;
                // Fix up the word
                fixUpPacked(*aselp);
                ঘূৰাই মিছা;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    FixUp(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

সার্ৱজনিক:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    স্থির size_t apply(DfgGraph& dfg, SccInfo& sccInfo) {
        যদি (!sccInfo.isCyclic()) ঘূৰাই 0;

        FixUp fixUp{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        বাবে (DfgVertexVar& vtx : dfg.varVertices()) {
            যদি (!sccInfo.get(vtx)) অনুৱর্ত;
            fixUp.main(vtx);
            যদি (!sccInfo.isCyclic()) ভঙ্গ;
        }
        // Return the number of improvements made
        ঘূৰাই fixUp.m_nImprovements;
    }
};

শূন্য breakCycles(DfgGraph& dfg, V3DfgBreakCyclesContext& ctx) {
    // Shorthand for dumping graph at given dump level
    ধ্রুৱ auto dump = [&](পূর্ণাঙ্ক level, ধ্রুৱ DfgGraph& dfg, ধ্রুৱ std::string& name) {
        যদি (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    size_t nImprovements = 0;
    size_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    কৰা {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ধ্রুৱ size_t nFixed = FixUp::apply(dfg, sccInfo);
        যদি (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        যদি (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        যদি (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            ঘূৰাই;
        }
    } যেতিয়ালৈ (nImprovements != prevNImprovements);

    // Debug dump
    যদি (dumpDfgLevel() >= 9) {
        ধ্রুৱ SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ধ্রুৱ DfgVertex& vtx) {
            ঘূৰাই sccInfo.get(vtx);  //
        });
    }

    // Accounting
    যদি (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } অন্যথা {
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
    বাবে (DfgVertexVar& vtx : dfg.varVertices()) {
        যদি (!vtx.tmpForp()) varps.ঠেলো_back(&vtx);
    }
    std::stable_partition(varps.begin(), varps.end(), [](ধ্রুৱ DfgVertexVar* varp) {
        ঘূৰাই varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    বাবে (DfgVertexVar* ধ্রুৱ varp : varps) {
        // Stop as soon as it becomes acyclic
        যদি (!sccInfo.isCyclic()) ভঙ্গ;

        // Ignore if variable is not part of a cycle
        যদি (!sccInfo.get(*varp)) অনুৱর্ত;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* ধ্রুৱ prevp = নূতন DfgPrev{dfg, varp->vscp()};
        sccInfo.add(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    যদি (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
শূন্য V3DfgPasses::breakCycles(DfgGraph& dfg, V3DfgContext& ctx) {
    V3DfgBreakCycles::breakCycles(dfg, ctx.m_breakCyclesContext);
    যদি (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::removeUnused(dfg);
}
