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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Hash.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <fstream>
#अन्तर्भाव <queue>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

नामाकाश V3DfgBreakCycles {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
वर्ग SccInfo final {
    DfgGraph& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ध्रुव DfgVertex*, uint64_t> m_vtx2Scc;
    size_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

सार्वजनिक:
    SccInfo(DfgGraph& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.forEachVertex([&](ध्रुव DfgVertex& vtx) {
            ध्रुव uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            यदि (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    तार्किक isCyclic() ध्रुव { प्रत्यागम m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ध्रुव DfgVertex& vtx) ध्रुव { प्रत्यागम m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    शून्य add(ध्रुव DfgVertex& vtx, uint64_t sccIdx) {
        ध्रुव तार्किक newEntry = m_vtx2Scc.emplace(&vtx, sccIdx).second;
        UASSERT_OBJ(newEntry, &vtx, "Vertex already inserted");
        यदि (sccIdx) ++m_nCyclicVertices;
    }

    तार्किक stillCyclicFwd(ध्रुव DfgVertex& vtx) ध्रुव {
        // If it only reads non-cyclic vertices, it has also become acyclic
        प्रत्यागम vtx.foreachSource([&](ध्रुव DfgVertex& src) {  //
            प्रत्यागम m_vtx2Scc.at(&src);
        });
    }

    तार्किक stillCyclicBwd(ध्रुव DfgVertex& vtx) ध्रुव {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        प्रत्यागम vtx.foreachSink([&](ध्रुव DfgVertex& dst) {  //
            प्रत्यागम m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    शून्य updateAcyclic(ध्रुव DfgVertex& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ध्रुव DfgVertex& dst) {
            // Sink was known to be acyclic, stop
            यदि (!m_vtx2Scc.at(&dst)) प्रत्यागम असत्य;
            यदि (!stillCyclicFwd(dst)) updateAcyclic(dst);
            प्रत्यागम असत्य;
        });
        // Propagate the update through the graph backward
        vtx.foreachSource([&](ध्रुव DfgVertex& src) {
            // Source was known to be acyclic, stop
            यदि (!m_vtx2Scc.at(&src)) प्रत्यागम असत्य;
            यदि (!stillCyclicBwd(src)) updateAcyclic(src);
            प्रत्यागम असत्य;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    शून्य updateReplacement(ध्रुव DfgVertex& vtx, DfgVertex& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        यदि (!get(replacement)) {
            replacement.foreachSink([&](DfgVertex& dst) {
                यदि (!get(dst)) प्रत्यागम असत्य;
                यदि (!stillCyclicFwd(dst)) updateAcyclic(dst);
                प्रत्यागम असत्य;
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
    शून्य validate() ध्रुव {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.forEachVertex([&](ध्रुव DfgVertex& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

वर्ग TraceDriver final : सार्वजनिक DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    संरचना Cache_किल्ली final {
        DfgVertex* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_किल्ली() = delete;
        Cache_किल्ली(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        संरचना Hash final {
            size_t operator()(ध्रुव Cache_किल्ली& item) ध्रुव {
                // cppcheck-suppress unreadVariable
                V3Hash hash{item.m_vtxp};
                hash += item.m_lsb;
                hash += item.m_msb;
                प्रत्यागम hash.मूल्य();
            }
        };

        संरचना Equal final {
            तार्किक operator()(ध्रुव Cache_किल्ली& a, ध्रुव Cache_किल्ली& b) ध्रुव {
                प्रत्यागम a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
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
    std::unordered_map<Cache_किल्ली, DfgVertex*, Cache_किल्ली::Hash, Cache_किल्ली::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_किल्ली, DfgVertex*, Cache_किल्ली::Hash, Cache_किल्ली::Equal> m_boundaryCache;

#ifdef VL_DEBUG
    std::ofstream m_ओळ_Coverage_File;  // Line coverage file, just for testing
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
    Vertex* make(ध्रुव DfgVertex* refp, uint32_t width) {
        static_assert(std::is_base_of<DfgVertex, Vertex>::मूल्य  //
                          && !std::is_base_of<DfgVertexVar, Vertex>::मूल्य,
                      "Should only make operation vertices and constants");

        यदि VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, Vertex>::मूल्य) {
            DfgConst* ध्रुव vtxp = नूतन DfgConst{m_dfg, refp->fileline(), width, 0};
            m_sccInfo.add(*vtxp, 0);
            प्रत्यागम reinterpret_cast<Vertex*>(vtxp);
        } अन्यथा {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            using Vtx = typename std::conditional<std::is_same<DfgConst, Vertex>::मूल्य, DfgSel,
                                                  Vertex>::type;
            Vtx* ध्रुव vtxp = नूतन Vtx{m_dfg, refp->fileline(), DfgDataType::packed(width)};
            m_sccInfo.add(*vtxp, 0);
            प्रत्यागम reinterpret_cast<Vertex*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    DfgVertexVar* createTmp(ध्रुव वर्ण* prefix, DfgVertex* vtxp) {
        File_ओळ* ध्रुव flp = vtxp->fileline();
        DfgVertex::ScopeCache scopeCache;
        AstScope* ध्रुव scopep = vtxp->scopep(scopeCache);
        DfgVertexVar* ध्रुव varp = m_dfg.makeNewVar(flp, prefix, vtxp->dtype(), scopep);
        varp->vscp()->varp()->isInternal(सत्य);
        varp->tmpForp(varp->vscp());
        m_sccInfo.add(*varp, 0);
        प्रत्यागम varp;
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
        DfgVertex* ध्रुव startVtxp = vtxp;
        ध्रुव uint32_t startMsb = msb;
        ध्रुव uint32_t startLsb = lsb;
        ध्रुव size_t pendingBegin = m_pendingps.size();
        DfgVertex* resp = nullptr;

        यावत् (सत्य) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->size() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            यदि (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                DfgVertex*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(nullptr))
                          .first->second;
                यदि (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    यदि (DfgVertexSplice* ध्रुव splicep = respr->cast<DfgVertexSplice>()) {
                        DfgVertexVar* ध्रुव tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    यदि (msb != respr->width() - 1 || lsb != 0) {
                        DfgSel* ध्रुव selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                भङ्ग;
            }

            // Normal cache for intra-component traces
            DfgVertex*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(nullptr))
                                    .first->second;

            // Trace the vertex
            यदि (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } अन्यथा {
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
                यदि (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ढकला_back(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    अनुवर्त;
                }
                respr = m_resp;
            }
            resp = respr;
            भङ्ग;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, startVtxp, "Tracing driver failed for " << startVtxp->typeName());
        UASSERT_OBJ(resp->width() == (startMsb - startLsb + 1), startVtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        प्रति (size_t i = pendingBegin; i < m_pendingps.size(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        प्रत्यागम resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    DfgVertex* traceSameIdx(DfgVertex* ध्रुव vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = nullptr;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->width() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        प्रत्यागम m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    DfgVertex* trace_ढकला_Idx(DfgVertex* ध्रुव vtxp, ध्रुव uint32_t idx) {
        ध्रुव size_t nIdxs = m_idxs.size();
        m_idxs.ढकला_back(idx);
        DfgVertex* ध्रुव resp = traceSameIdx(vtxp);
        m_idxs.काढा_back();
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        प्रत्यागम resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    DfgVertex* trace_काढा_Idx(DfgVertex* ध्रुव vtxp) {
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        ध्रुव size_t nIdxs = m_idxs.size();
        ध्रुव uint32_t idx = m_idxs.back();
        m_idxs.काढा_back();
        DfgVertex* ध्रुव resp = m_idxs.empty() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ढकला_back(idx);
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        प्रत्यागम resp;
    }

    template <typename Vertex>
    Vertex* trace_Bitwise_द्विमान(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विमान, Vertex>::मूल्य,
                      "Should only call on DfgVertexBinary");
        Vertex* ध्रुव resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        प्रत्यागम resp;
    }

    template <typename Vertex>
    DfgVertex* traceAddSub(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विमान, Vertex>::मूल्य,
                      "Should only call on DfgVertexBinary");
        Vertex* ध्रुव opp = make<Vertex>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ध्रुव selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        प्रत्यागम selp;
    }

    template <typename Vertex>
    Vertex* traceReduction(Vertex* vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::मूल्य,
                      "Should only call on DfgVertexUnary");
        Vertex* ध्रुव resp = make<Vertex>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->width() - 1, 0));
        प्रत्यागम resp;
    }

    template <typename Vertex>
    Vertex* traceCmp(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विमान, Vertex>::मूल्य,
                      "Should only call on DfgVertexBinary");
        Vertex* ध्रुव resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        प्रत्यागम resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define TRACE_RESULT_COVER() \
    कुरु { \
        यदि (VL_UNLIKELY(m_ओळ_Coverage_File.is_open())) m_ओळ_Coverage_File << __LINE__ << '\n'; \
    } यावत् (असत्य)
#अन्यथा
#define TRACE_RESULT_COVER() \
    कुरु { \
    } यावत् (असत्य)
#endif

#define RETURN_RESULT(vtxp) \
    कुरु { \
        m_resp = (vtxp); \
        TRACE_RESULT_COVER(); \
        प्रत्यागम; \
    } यावत् (असत्य)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#define RETURN_RESULT_TAIL(srcp, smsb, slsb) \
    कुरु { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        DfgVertex* ध्रुव tailp = (srcp); \
        ध्रुव uint32_t tailMsb = (smsb); \
        ध्रुव uint32_t tailLsb = (slsb); \
        m_tailp = tailp; \
        m_msb = tailMsb; \
        m_lsb = tailLsb; \
        TRACE_RESULT_COVER(); \
        प्रत्यागम; \
    } यावत् (असत्य)

    // VISITORS
    शून्य visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    शून्य visit(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        DfgVertex* defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        संरचना Driver final {
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
        तार्किक tryWholeDefault = defaultp;
        DfgVertex* coverp = nullptr;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lsb) {
            ध्रुव uint32_t msb = lsb + src.width() - 1;
            drivers.emplace_back(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            यदि (m_msb >= lsb && msb >= m_lsb) tryWholeDefault = असत्य;
            // If it does not cover the whole searched bit range, move on
            यदि (m_lsb < lsb || msb < m_msb) प्रत्यागम असत्य;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            प्रत्यागम सत्य;
        });

        // Trace the driver covering the whole searched range, if there is one
        यदि (coverp) RETURN_RESULT_TAIL(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        यदि (tryWholeDefault) RETURN_RESULT_TAIL(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::sort(drivers.begin(), drivers.end(), [](ध्रुव Driver& a, ध्रुव Driver& b) {  //
            प्रत्यागम a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::vector<DfgVertex*> termps;
        uint32_t lsb = m_lsb;
        प्रति (ध्रुव Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            यदि (lsb > driver.m_msb) अनुवर्त;
            // Driver is above the searched MSB, done
            यदि (driver.m_lsb > m_msb) भङ्ग;
            // Gap below this driver, trace default to fill it
            यदि (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_back(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ध्रुव uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_back(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        यदि (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_back(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.size() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        DfgVertex* resp = termps.front();
        प्रति (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* ध्रुव termp = termps[i];
            DfgConcat* ध्रुव catp = make<DfgConcat>(termp, resp->width() + termp->width());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_RESULT(resp);
    }

    शून्य visit(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        DfgVertex* ध्रुव defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ध्रुव uint32_t idx = m_idxs.back();
        यदि (DfgVertex* ध्रुव driverp = vtxp->driverAt(idx)) {
            DfgVertex* ध्रुव srcp = driverp->as<DfgUnitArray>()->srcp();
            यदि (srcp->is<DfgVertexSplice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_RESULT(trace_काढा_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_RESULT(traceSameIdx(defaultp));
    }

    शून्य visit(DfgVertexVar* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        DfgVertex* ध्रुव srcp = vtxp->srcp();
        DfgVertex* ध्रुव defaultp = vtxp->defaultp();
        DfgVertex* ध्रुव drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        यदि (srcp && srcp->is<DfgVertexSplice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_RESULT(m_idxs.empty() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    शून्य visit(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        यदि (ध्रुव DfgConst* ध्रुव idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_RESULT(trace_ढकला_Idx(vtxp->fromp(), idxp->toU32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.empty(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ध्रुव resp = make<DfgArraySel>(vtxp, vtxp->width());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->width() - 1, 0));
        DfgSel* ध्रुव selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    शून्य visit(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.back() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_RESULT(trace_काढा_Idx(vtxp->srcp()));
    }

    शून्य visit(DfgConcat* vtxp) override {
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        ध्रुव uint32_t rWidth = rhsp->width();
        // If the traced bits are wholly in the RHS
        यदि (rWidth > m_msb) RETURN_RESULT_TAIL(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        यदि (m_lsb >= rWidth) RETURN_RESULT_TAIL(lhsp, m_msb - rWidth, m_lsb - rWidth);
        // The traced bit spans both sides, trace both
        DfgConcat* ध्रुव resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, rWidth - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - rWidth, 0));
        RETURN_RESULT(resp);
    }

    शून्य visit(DfgRep* vtxp) override {
        DfgVertex* ध्रुव srcp = vtxp->srcp();
        ध्रुव uint32_t sWidth = srcp->width();
        // If we need more bits than the source, then we need the whole source
        यदि (m_msb - m_lsb + 1 > sWidth) {
            DfgRep* ध्रुव repp = make<DfgRep>(vtxp, vtxp->width());
            repp->srcp(trace(srcp, sWidth - 1, 0));
            DfgSel* ध्रुव resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_RESULT(resp);
        }
        // If the requested bits are within the same repliacted word
        यदि (m_msb / sWidth == m_lsb / sWidth) {
            RETURN_RESULT_TAIL(srcp, m_msb % sWidth, m_lsb % sWidth);
        }
        // The requested bits span two replicated words
        DfgConcat* ध्रुव resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, sWidth - 1, m_lsb % sWidth));
        resp->lhsp(trace(srcp, m_msb % sWidth, 0));
        RETURN_RESULT(resp);
    }

    शून्य visit(DfgExtend* vtxp) override {
        DfgVertex* ध्रुव srcp = vtxp->srcp();
        ध्रुव uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        यदि (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        यदि (m_lsb >= sWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ध्रुव resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    शून्य visit(DfgExtendS* vtxp) override {
        DfgVertex* ध्रुव srcp = vtxp->srcp();
        ध्रुव uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        यदि (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        यदि (m_lsb >= sWidth) {
            यदि (m_msb == m_lsb) RETURN_RESULT_TAIL(srcp, sWidth - 1, sWidth - 1);
            DfgExtendS* ध्रुव resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, sWidth - 1, sWidth - 1));
            RETURN_RESULT(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ध्रुव resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    शून्य visit(DfgSel* vtxp) override {
        ध्रुव uint32_t lsb = vtxp->lsb();
        RETURN_RESULT_TAIL(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    शून्य visit(DfgNot* vtxp) override {
        DfgNot* ध्रुव resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    शून्य visit(DfgAnd* vtxp) override { RETURN_RESULT(trace_Bitwise_द्विमान(vtxp)); }
    शून्य visit(DfgOr* vtxp) override { RETURN_RESULT(trace_Bitwise_द्विमान(vtxp)); }
    शून्य visit(DfgXor* vtxp) override { RETURN_RESULT(trace_Bitwise_द्विमान(vtxp)); }
    शून्य visit(DfgAdd* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    शून्य visit(DfgSub* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    शून्य visit(DfgRedAnd* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    शून्य visit(DfgRedOr* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    शून्य visit(DfgRedXor* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    शून्य visit(DfgEq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    शून्य visit(DfgNeq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    शून्य visit(DfgLt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    शून्य visit(DfgLte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    शून्य visit(DfgGt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    शून्य visit(DfgGte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }

    शून्य visit(DfgShiftRS* vtxp) override {
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        यदि (ध्रुव DfgConst* ध्रुव rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ध्रुव uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ध्रुव uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            यदि (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            यदि (m_lsb >= lowerWidth) {
                DfgExtendS* ध्रुव resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->width() - 1, lhsp->width() - 1));
                RETURN_RESULT(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ध्रुव resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftRS* ध्रुव shiftrsp = make<DfgShiftRS>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        यदि (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrsp);
        DfgSel* ध्रुव selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    शून्य visit(DfgShiftR* vtxp) override {
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        यदि (ध्रुव DfgConst* ध्रुव rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ध्रुव uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ध्रुव uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            यदि (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            यदि (m_lsb >= lowerWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ध्रुव resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftR* ध्रुव shiftrp = make<DfgShiftR>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        यदि (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrp);
        DfgSel* ध्रुव selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    शून्य visit(DfgShiftL* vtxp) override {
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        यदि (ध्रुव DfgConst* ध्रुव rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ध्रुव uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ध्रुव uint32_t lowerWidth = shiftAmnt > vtxp->width() ? vtxp->width() : shiftAmnt;
            // If the traced bits are wholly in the input
            यदि (m_lsb >= lowerWidth) {
                RETURN_RESULT_TAIL(lhsp, m_msb - shiftAmnt, m_lsb - shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            यदि (lowerWidth > m_msb) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ध्रुव resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->width() - (m_msb - lowerWidth + 1)));
            resp->lhsp(trace(lhsp, m_msb - shiftAmnt, lowerWidth - shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftL* ध्रुव shiftlp = make<DfgShiftL>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        यदि (m_lsb == 0) RETURN_RESULT(shiftlp);
        DfgSel* ध्रुव selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    शून्य visit(DfgCond* vtxp) override {
        DfgCond* ध्रुव resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->width() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    शून्य visit(DfgMatchMasked* vtxp) override {
        DfgMatchMasked* ध्रुव resp = make<DfgMatchMasked>(vtxp, vtxp->width());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->width() - 1, 0));
        DfgSel* ध्रुव selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

#undef RETURN_RESULT
#undef TRACE_RESULT_COVER
#undef RETURN_RESULT_TAIL

सार्वजनिक:
    // CONSTRUCTOR
    TraceDriver(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#ifdef VL_DEBUG
        यदि (v3Global.opt.debugCheck()) {
            m_ओळ_Coverage_File.open(  //
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
        प्रत्यागम trace(&vtx, lsb + width - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
वर्ग BitMask final {
    // TYPES
    ध्रुव DfgDataType& m_dtype;
    सङ्घ {
        V3_संख्या m_अंक;  // The bits, if packed
        std::vector<BitMask> m_sub;  // The per element masks, if aggregate
    };

सार्वजनिक:
    // CONSTRUCTOR
    BitMask(File_ओळ* flp, ध्रुव DfgDataType& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        यदि (m_dtype.isPacked()) {
            नूतन (&m_अंक) V3_संख्या{flp, static_cast<पूर्णाङ्क>(m_dtype.size()), 0};
        } अन्यथा {
            नूतन (&m_sub) std::vector<BitMask>{};
            m_sub.reserve(m_dtype.size());
            प्रति (uint32_t i = 0; i < m_dtype.size(); ++i)
                m_sub.emplace_back(flp, m_dtype.elemDtype());
        }
    }
    BitMask(ध्रुव DfgVertex& vtx)
        : BitMask{vtx.fileline(), vtx.dtype()} {}
    BitMask(ध्रुव BitMask& other)
        : m_dtype{other.m_dtype} {
        यदि (m_dtype.isPacked()) {
            नूतन (&m_अंक) V3_संख्या{other.m_अंक};
        } अन्यथा {
            नूतन (&m_sub) std::vector<BitMask>{other.m_sub};
        }
    }
    BitMask& operator=(ध्रुव BitMask& other) {
        यदि (एतत् != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            यदि (other.m_dtype.isPacked()) {
                m_अंक = other.m_अंक;
            } अन्यथा {
                m_sub = other.m_sub;
            }
        }
        प्रत्यागम *एतत्;
    }
    ~BitMask() {
        यदि (m_dtype.isPacked()) {
            m_अंक.~V3_संख्या();
        } अन्यथा {
            m_sub.~vector<BitMask>();
        }
    }

    // METHODS
    V3_संख्या& अंक() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        प्रत्यागम m_अंक;
    }
    ध्रुव V3_संख्या& अंक() ध्रुव {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        प्रत्यागम m_अंक;
    }
    std::vector<BitMask>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        प्रत्यागम m_sub;
    }

    तार्किक isZero() ध्रुव {
        यदि (m_dtype.isPacked()) प्रत्यागम m_अंक.isEqZero();

        प्रति (ध्रुव BitMask& sub : m_sub) {
            यदि (!sub.isZero()) प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }
    तार्किक isOnes() ध्रुव {
        यदि (m_dtype.isPacked()) प्रत्यागम m_अंक.isEqAllOnes();

        प्रति (ध्रुव BitMask& sub : m_sub) {
            यदि (!sub.isOnes()) प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }
    शून्य setZero() {
        यदि (m_dtype.isPacked()) {
            m_अंक.setAllBits0();
        } अन्यथा {
            प्रति (BitMask& sub : m_sub) sub.setZero();
        }
    }
    शून्य setOnes() {
        यदि (m_dtype.isPacked()) {
            m_अंक.setAllBits1();
        } अन्यथा {
            प्रति (BitMask& sub : m_sub) sub.setOnes();
        }
    }

    तार्किक operator==(ध्रुव BitMask& other) ध्रुव {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        यदि (m_dtype.isPacked()) प्रत्यागम m_अंक.isCaseEq(other.m_अंक);
        प्रत्यागम m_sub == other.m_sub;
    }
    तार्किक operator!=(ध्रुव BitMask& other) ध्रुव { प्रत्यागम !(*एतत् == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    तार्किक isContractionOf(ध्रुव BitMask& that) ध्रुव {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        यदि (m_dtype.isPacked()) {
            ध्रुव size_t शब्द = VL_शब्द_I(m_dtype.size());
            प्रति (size_t i = 0; i < शब्द; ++i) {
                यदि (~m_अंक.edataWord(i) & that.m_अंक.edataWord(i)) प्रत्यागम सत्य;
            }
            प्रत्यागम असत्य;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        प्रति (size_t i = 0; i < m_sub.size(); ++i) {
            यदि (m_sub[i].isContractionOf(that.m_sub[i])) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    std::string toString() ध्रुव {
        यदि (m_dtype.isPacked()) प्रत्यागम "0x" + m_अंक.displayed(m_अंक.fileline(), "%x");
        std::string result;
        result += "{";
        प्रति (ध्रुव BitMask& sub : m_sub) {
            यदि (&sub != &m_sub.front()) result += ", ";
            result += sub.toString();
        }
        result += "}";
        प्रत्यागम result;
    }
};

वर्ग IndependentBits final : सार्वजनिक DfgVisitor {
    // TYPES
    संरचना VertexState final {
        size_t m_rpo_संख्या = 0;  // Reverse Postorder Number of vertex
        तार्किक m_isOnWorkList = असत्य;  // Whether the vertex is on the work list
    };

    // STATE
    ध्रुव SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ध्रुव DfgVertex*, BitMask> m_vtxp2Mask;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_workQueue;
    std::unordered_map<ध्रुव DfgVertex*, VertexState> m_vtxp2State;  // State of each vertex

#ifdef VL_DEBUG
    std::ofstream m_ओळ_Coverage_File;  // Line coverage file, just for testing
#endif

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    BitMask& mask(ध्रुव DfgVertex& vtx) {
        // Look up (or create) mask for 'vtx'
        प्रत्यागम m_vtxp2Mask
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .first->second;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define MASK(vtxp) \
    ([एतत्](ध्रुव DfgVertex& vtx) -> BitMask& { \
        यदि (VL_UNLIKELY(m_ओळ_Coverage_File.is_open())) m_ओळ_Coverage_File << __LINE__ << '\n'; \
        प्रत्यागम mask(vtx); \
    }(*vtxp))
#अन्यथा
#define MASK(vtxp) mask(*vtxp)
#endif

    // Clear all bits at or below the most significant clear bit
    शून्य floodTowardsLsb(V3_संख्या& अंक) {
        प्रति (पूर्णाङ्क i = अंक.width() - 1; i >= 0; --i) {
            यदि (अंक.bitIs1(i)) अनुवर्त;
            अंक.opSetRange(0, i + 1, '0');
            भङ्ग;
        }
    }

    // Clear all bits at or above the least significant clear bit
    शून्य floodTowardsMsb(V3_संख्या& अंक) {
        प्रति (पूर्णाङ्क i = 0; i < अंक.width(); ++i) {
            यदि (अंक.bitIs1(i)) अनुवर्त;
            अंक.opSetRange(i, अंक.width() - i, '0');
            भङ्ग;
        }
    }

    शून्य propagateFromDriver(BitMask& m, ध्रुव DfgVertex* srcp) {
        // If there is no driver, we are done
        यदि (!srcp) प्रत्यागम;
        // If it is driven by a splice, we need to combine the masks of the drivers
        यदि (ध्रुव DfgSplicePacked* ध्रुव splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ध्रुव DfgVertex& src, uint32_t lo) {
                m.अंक().opSelInto(MASK(&src).अंक(), lo, src.width());
                प्रत्यागम असत्य;
            });
            प्रत्यागम;
        }
        यदि (ध्रुव DfgSpliceArray* ध्रुव splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ध्रुव DfgVertex& src, uint32_t lo) {
                यदि (ध्रुव DfgUnitArray* ध्रुव uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = MASK(uap->srcp());
                } अन्यथा {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                प्रत्यागम असत्य;
            });
            प्रत्यागम;
        }
        // Otherwise, we just use the mask of the single driver
        m = MASK(srcp);
    }

    // VISITORS
    शून्य visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    शून्य visit(DfgVertexVar* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        यदि (vtxp->isVolatile()) प्रत्यागम;

        BitMask& m = MASK(vtxp);
        DfgVertex* ध्रुव srcp = vtxp->srcp();
        DfgVertex* ध्रुव defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        यदि (defaultp) m = MASK(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    शून्य visit(DfgVertexSplice* vtxp) override {
        propagateFromDriver(MASK(vtxp), vtxp);  // Needed to continue traversal
    }

    शून्य visit(DfgUnitArray* vtxp) override { MASK(vtxp).sub().at(0) = MASK(vtxp->srcp()); }

    शून्य visit(DfgArraySel* vtxp) override {
        DfgVertex* ध्रुव fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        यदि (ध्रुव DfgConst* ध्रुव idxp = vtxp->bitp()->cast<DfgConst>()) {
            MASK(vtxp) = MASK(vtxp->fromp()).sub().at(idxp->toSizeT());
            प्रत्यागम;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        यदि (MASK(vtxp->bitp()).isOnes() && MASK(fromp).isOnes()) MASK(vtxp).setOnes();
    }

    शून्य visit(DfgConcat* vtxp) override {
        ध्रुव DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        ध्रुव DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opSelInto(MASK(rhsp).अंक(), 0, rhsp->width());
        m.opSelInto(MASK(lhsp).अंक(), rhsp->width(), lhsp->width());
    }

    शून्य visit(DfgRep* vtxp) override {
        ध्रुव uint32_t मोजणी = vtxp->मोजणी();
        ध्रुव DfgVertex* ध्रुव srcp = vtxp->srcp();
        ध्रुव uint32_t sWidth = srcp->width();
        V3_संख्या& vMask = MASK(vtxp).अंक();
        V3_संख्या& sMask = MASK(srcp).अंक();
        प्रति (uint32_t i = 0; i < मोजणी; ++i) vMask.opSelInto(sMask, i * sWidth, sWidth);
    }

    शून्य visit(DfgSel* vtxp) override {
        ध्रुव uint32_t lsb = vtxp->lsb();
        ध्रुव uint32_t msb = lsb + vtxp->width() - 1;
        MASK(vtxp).अंक().opSel(MASK(vtxp->fromp()).अंक(), msb, lsb);
    }

    शून्य visit(DfgExtend* vtxp) override {
        ध्रुव DfgVertex* ध्रुव srcp = vtxp->srcp();
        ध्रुव uint32_t sWidth = srcp->width();
        V3_संख्या& s = MASK(srcp).अंक();
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, '1');
    }
    शून्य visit(DfgExtendS* vtxp) override {
        ध्रुव DfgVertex* ध्रुव srcp = vtxp->srcp();
        ध्रुव uint32_t sWidth = srcp->width();
        V3_संख्या& s = MASK(srcp).अंक();
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, s.bitIs0(sWidth - 1) ? '0' : '1');
    }

    शून्य visit(DfgNot* vtxp) override {  //
        MASK(vtxp) = MASK(vtxp->srcp());
    }

    शून्य visit(DfgAnd* vtxp) override {  //
        MASK(vtxp).अंक().opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
    }
    शून्य visit(DfgOr* vtxp) override {  //
        MASK(vtxp).अंक().opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
    }
    शून्य visit(DfgXor* vtxp) override {  //
        MASK(vtxp).अंक().opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
    }

    शून्य visit(DfgAdd* vtxp) override {
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
        floodTowardsMsb(m);
    }
    शून्य visit(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
        floodTowardsMsb(m);
    }

    शून्य visit(DfgRedAnd* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }
    शून्य visit(DfgRedOr* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }
    शून्य visit(DfgRedXor* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }

    शून्य visit(DfgEq* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }
    शून्य visit(DfgNeq* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }
    शून्य visit(DfgLt* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }
    शून्य visit(DfgLte* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }
    शून्य visit(DfgGt* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }
    शून्य visit(DfgGte* vtxp) override {
        ध्रुव तार्किक independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        यदि (independent) MASK(vtxp).setOnes();
    }

    शून्य visit(DfgShiftRS* vtxp) override {
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        ध्रुव uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        यदि (DfgConst* ध्रुव rConstp = rhsp->cast<DfgConst>()) {
            ध्रुव uint32_t shiftAmount = rConstp->toU32();
            यदि (shiftAmount >= width) {
                यदि (MASK(lhsp).अंक().bitIs1(width - 1)) { MASK(vtxp).setOnes(); }
            } अन्यथा {
                V3_संख्या& m = MASK(vtxp).अंक();
                m.opShiftRS(MASK(lhsp).अंक(), rConstp->अंक(), width);
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            प्रत्यागम;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        यदि (MASK(rhsp).isOnes()) {
            V3_संख्या& m = MASK(vtxp).अंक();
            m = MASK(lhsp).अंक();
            floodTowardsLsb(m);
        }
    }

    शून्य visit(DfgShiftR* vtxp) override {
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        ध्रुव uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        यदि (DfgConst* ध्रुव rConstp = rhsp->cast<DfgConst>()) {
            ध्रुव uint32_t shiftAmount = rConstp->toU32();
            यदि (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } अन्यथा {
                V3_संख्या& m = MASK(vtxp).अंक();
                m.opShiftR(MASK(lhsp).अंक(), rConstp->अंक());
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            प्रत्यागम;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        यदि (MASK(rhsp).isOnes()) {
            V3_संख्या& m = MASK(vtxp).अंक();
            m = MASK(lhsp).अंक();
            floodTowardsLsb(m);
        }
    }

    शून्य visit(DfgShiftL* vtxp) override {
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        ध्रुव uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        यदि (DfgConst* ध्रुव rConstp = rhsp->cast<DfgConst>()) {
            ध्रुव uint32_t shiftAmount = rConstp->toU32();
            यदि (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } अन्यथा {
                V3_संख्या& m = MASK(vtxp).अंक();
                m.opShiftL(MASK(lhsp).अंक(), rConstp->अंक());
                m.opSetRange(0, shiftAmount, '1');
            }
            प्रत्यागम;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        यदि (MASK(rhsp).isOnes()) {
            V3_संख्या& m = MASK(vtxp).अंक();
            m = MASK(lhsp).अंक();
            floodTowardsMsb(m);
        }
    }

    शून्य visit(DfgCond* vtxp) override {
        यदि (MASK(vtxp->condp()).isOnes()) {
            MASK(vtxp).अंक().opAnd(MASK(vtxp->thenp()).अंक(), MASK(vtxp->elsep()).अंक());
        }
    }

    शून्य visit(DfgMatchMasked* vtxp) override {
        यदि (MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->matchp()).isOnes()) { MASK(vtxp).setOnes(); }
    }

#undef MASK

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    शून्य enqueueSinks(DfgVertex& vtx) {
        vtx.foreachSink([&](DfgVertex& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            यदि (!m_sccInfo.get(sink)) प्रत्यागम असत्य;
            // Otherwise just enqueue it
            VertexState& state = m_vtxp2State.at(&sink);
            यदि (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_workQueue.ढकला(state.m_rpo_संख्या);
            }
            state.m_isOnWorkList = सत्य;
            प्रत्यागम असत्य;
        });
    };

    शून्य dfsPostOrder(std::unordered_set<DfgVertex*>& visited,  //
                      std::vector<DfgVertex*>& postOrderEnumeration,  //
                      DfgVertex& vtx) {
        // Mark visited, skip if already visited
        यदि (!visited.घाला(&vtx).second) प्रत्यागम;
        // Visit un-visited successors
        vtx.foreachSink([&](DfgVertex& snk) {
            dfsPostOrder(visited, postOrderEnumeration, snk);
            प्रत्यागम असत्य;
        });
        // Add to post order enumeration
        postOrderEnumeration.emplace_back(&vtx);
    };

    IndependentBits(DfgGraph& dfg, ध्रुव SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#ifdef VL_DEBUG
        यदि (v3Global.opt.debugCheck()) {
            m_ओळ_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::app);
        }
#endif

        // Compute Reverse-Postorder enumeration of vertices
        std::vector<DfgVertex*> rpoEnumeration;
        std::unordered_set<DfgVertex*> visited;
        dfg.forEachVertex([&](DfgVertex& vtx) {
            यदि (!vtx.nInputs()) dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        dfg.forEachVertex([&](DfgVertex& vtx) {  //
            dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        std::reverse(rpoEnumeration.begin(), rpoEnumeration.end());
        UASSERT(rpoEnumeration.size() == dfg.size(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        प्रति (size_t i = 0; i < rpoEnumeration.size(); ++i) {
            VertexState& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_संख्या = i;
            state.m_isOnWorkList = असत्य;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        प्रति (DfgVertex* ध्रुव vtxp : rpoEnumeration) {
            यदि (m_sccInfo.get(*vtxp)) अनुवर्त;
            mask(*vtxp).setOnes();
        }
        प्रति (DfgVertex* ध्रुव vtxp : rpoEnumeration) {
            यदि (!m_sccInfo.get(*vtxp)) अनुवर्त;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << mask(*vtxp).toString());
            यदि (!mask(*vtxp).isZero()) enqueueSinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        यावत् (!m_workQueue.empty()) {
            // Grab next item
            DfgVertex* ध्रुव currp = rpoEnumeration[m_workQueue.शीर्ष()];
            m_workQueue.काढा();
            m_vtxp2State.at(currp).m_isOnWorkList = असत्य;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ध्रुव BitMask& currMask = mask(*currp);
            // Remember current mask so we can check if it changed
            ध्रुव BitMask prevMask = currMask;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            यदि (prevMask != currMask) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prevMask.toString()
                             << "\n      to: " << currMask.toString());
                enqueueSinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                यदि (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!currMask.isContractionOf(prevMask), currp,
                                "Mask should only expand");
                }
            }
        }
    }

सार्वजनिक:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    स्थिर std::unordered_map<ध्रुव DfgVertex*, BitMask> apply(DfgGraph& dfg,
                                                               ध्रुव SccInfo& sccInfo) {
        प्रत्यागम std::move(IndependentBits{dfg, sccInfo}.m_vtxp2Mask);
    }
};

वर्ग FixUp final {
    DfgGraph& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ध्रुव std::unordered_map<ध्रुव DfgVertex*, BitMask> m_independentBits
        = IndependentBits::apply(m_dfg, m_sccInfo);
    size_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    स्थिर BitMask computeUsedBits(DfgVertex& vtx) {
        BitMask result{vtx};
        vtx.foreachSink([&result](DfgVertex& sink) {
            // If used via a Sel, mark the selected bits used
            यदि (ध्रुव DfgSel* ध्रुव selp = sink.cast<DfgSel>()) {
                result.अंक().opSetRange(selp->lsb(), selp->width(), '1');
                प्रत्यागम असत्य;
            }
            // Used without a Sel, so all bits are used
            result.setOnes();
            प्रत्यागम सत्य;
        });
        प्रत्यागम result;
    }

    स्थिर std::string debugStr(ध्रुव DfgVertex& vtx) {  // LCOV_EXCL_START
        यदि (ध्रुव DfgArraySel* ध्रुव aselp = vtx.cast<DfgArraySel>()) {
            ध्रुव size_t i = aselp->bitp()->as<DfgConst>()->toSizeT();
            प्रत्यागम debugStr(*aselp->fromp()) + "[" + std::to_string(i) + "]";
        }
        यदि (ध्रुव DfgVertexVar* ध्रुव varp = vtx.cast<DfgVertexVar>()) {
            प्रत्यागम varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    शून्य gatherTerms(std::vector<DfgVertex*>& termps, DfgVertex& vtx, ध्रुव V3_संख्या& indpBits,
                     ध्रुव uint32_t hi, ध्रुव uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        तार्किक isIndependent = indpBits.bitIs1(lo);
        uint32_t lsb = lo;
        प्रति (uint32_t msb = lo; msb <= hi; ++msb) {
            ध्रुव तार्किक endRange = msb == hi || isIndependent != indpBits.bitIs1(msb + 1);
            यदि (!endRange) अनुवर्त;
            ध्रुव uint32_t width = msb - lsb + 1;
            // Compute the term to use for this range
            यदि (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_back(m_traceDriver.apply(vtx, lsb, width));
            } अन्यथा {
                // If dependent, fall back on using the part of the variable
                DfgSel* ध्रुव selp = नूतन DfgSel{m_dfg, vtx.fileline(), DfgDataType::packed(width)};
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

    शून्य fixUpPacked(DfgVertex& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ध्रुव BitMask& indpBits = m_independentBits.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indpBits.toString());
        // Can't do anything if all bits are dependent
        यदि (indpBits.isZero()) प्रत्यागम;

        // Figure out which bits of 'vtxp' are used
        ध्रुव BitMask usedBits = computeUsedBits(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << usedBits.toString());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_संख्या usedAndIndependent{vtx.fileline(), static_cast<पूर्णाङ्क>(vtx.width()), 0};
        usedAndIndependent.opAnd(usedBits.अंक(), indpBits.अंक());
        यदि (usedAndIndependent.isEqZero()) प्रत्यागम;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::vector<DfgVertex*> termps;

        // Iterate through consecutive used/unused ranges
        File_ओळ* ध्रुव flp = vtx.fileline();
        ध्रुव uint32_t width = vtx.width();
        तार्किक isUsed = usedBits.अंक().bitIs1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        प्रति (uint32_t msb = 0; msb < width; ++msb) {
            ध्रुव तार्किक endRange = msb == width - 1 || isUsed != usedBits.अंक().bitIs1(msb + 1);
            यदि (!endRange) अनुवर्त;
            यदि (isUsed) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indpBits.अंक(), msb, lsb);
            } अन्यथा {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ध्रुव constp = नूतन DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.add(*constp, 0);
                termps.emplace_back(constp);
            }
            // Next iteration
            isUsed = !isUsed;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        DfgVertex* replacementp = termps.front();
        ध्रुव uint64_t vComp = m_sccInfo.get(vtx);
        प्रति (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* ध्रुव termp = termps[i];
            ध्रुव uint32_t catWidth = replacementp->width() + termp->width();
            DfgConcat* ध्रुव catp = नूतन DfgConcat{m_dfg, flp, DfgDataType::packed(catWidth)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ध्रुव uint64_t tComp = m_sccInfo.get(*termp);
            ध्रुव uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.add(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        प्रति (DfgVertex* ध्रुव termp : termps) {
            यदि (DfgSel* ध्रुव selp = termp->cast<DfgSel>()) {
                यदि (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        यदि (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    शून्य main(DfgVertexVar& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        यदि (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fixUpPacked(var);
        } अन्यथा यदि (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ध्रुव uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](DfgVertex& sink) {
                // Ignore if sink is not part of same cycle
                यदि (m_sccInfo.get(sink) != component) प्रत्यागम असत्य;
                // Only handle ArraySels with constant index
                DfgArraySel* ध्रुव aselp = sink.cast<DfgArraySel>();
                यदि (!aselp) प्रत्यागम असत्य;
                यदि (!aselp->bitp()->is<DfgConst>()) प्रत्यागम असत्य;
                // Fix up the word
                fixUpPacked(*aselp);
                प्रत्यागम असत्य;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    FixUp(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

सार्वजनिक:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    स्थिर size_t apply(DfgGraph& dfg, SccInfo& sccInfo) {
        यदि (!sccInfo.isCyclic()) प्रत्यागम 0;

        FixUp fixUp{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        प्रति (DfgVertexVar& vtx : dfg.varVertices()) {
            यदि (!sccInfo.get(vtx)) अनुवर्त;
            fixUp.main(vtx);
            यदि (!sccInfo.isCyclic()) भङ्ग;
        }
        // Return the number of improvements made
        प्रत्यागम fixUp.m_nImprovements;
    }
};

शून्य breakCycles(DfgGraph& dfg, V3DfgBreakCyclesContext& ctx) {
    // Shorthand for dumping graph at given dump level
    ध्रुव auto dump = [&](पूर्णाङ्क level, ध्रुव DfgGraph& dfg, ध्रुव std::string& name) {
        यदि (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    size_t nImprovements = 0;
    size_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    कुरु {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ध्रुव size_t nFixed = FixUp::apply(dfg, sccInfo);
        यदि (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        यदि (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        यदि (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            प्रत्यागम;
        }
    } यावत् (nImprovements != prevNImprovements);

    // Debug dump
    यदि (dumpDfgLevel() >= 9) {
        ध्रुव SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ध्रुव DfgVertex& vtx) {
            प्रत्यागम sccInfo.get(vtx);  //
        });
    }

    // Accounting
    यदि (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } अन्यथा {
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
    प्रति (DfgVertexVar& vtx : dfg.varVertices()) {
        यदि (!vtx.tmpForp()) varps.ढकला_back(&vtx);
    }
    std::stable_partition(varps.begin(), varps.end(), [](ध्रुव DfgVertexVar* varp) {
        प्रत्यागम varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    प्रति (DfgVertexVar* ध्रुव varp : varps) {
        // Stop as soon as it becomes acyclic
        यदि (!sccInfo.isCyclic()) भङ्ग;

        // Ignore if variable is not part of a cycle
        यदि (!sccInfo.get(*varp)) अनुवर्त;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* ध्रुव prevp = नूतन DfgPrev{dfg, varp->vscp()};
        sccInfo.add(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    यदि (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
शून्य V3DfgPasses::breakCycles(DfgGraph& dfg, V3DfgContext& ctx) {
    V3DfgBreakCycles::breakCycles(dfg, ctx.m_breakCyclesContext);
    यदि (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::removeUnused(dfg);
}
