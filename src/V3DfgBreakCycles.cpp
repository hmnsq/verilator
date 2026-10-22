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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Hash.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <fstream>
#ଅନ୍ତର୍ଭାଵ <queue>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ନାମାକାଶ V3DfgBreakCycles {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
ଶ୍ରେଣୀ SccInfo final {
    DfgGraph& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ଅଚଳ DfgVertex*, uint64_t> m_vtx2Scc;
    size_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

ସର୍ବସାଧାରଣ:
    SccInfo(DfgGraph& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.forEachVertex([&](ଅଚଳ DfgVertex& vtx) {
            ଅଚଳ uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            ଯଦି (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    ତାର୍କିକ isCyclic() ଅଚଳ { ଫେରନ୍ତୁ m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ଅଚଳ DfgVertex& vtx) ଅଚଳ { ଫେରନ୍ତୁ m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    ଶୂନ୍ୟ add(ଅଚଳ DfgVertex& vtx, uint64_t sccIdx) {
        ଅଚଳ ତାର୍କିକ newEntry = m_vtx2Scc.emplace(&vtx, sccIdx).second;
        UASSERT_OBJ(newEntry, &vtx, "Vertex already inserted");
        ଯଦି (sccIdx) ++m_nCyclicVertices;
    }

    ତାର୍କିକ stillCyclicFwd(ଅଚଳ DfgVertex& vtx) ଅଚଳ {
        // If it only reads non-cyclic vertices, it has also become acyclic
        ଫେରନ୍ତୁ vtx.foreachSource([&](ଅଚଳ DfgVertex& src) {  //
            ଫେରନ୍ତୁ m_vtx2Scc.at(&src);
        });
    }

    ତାର୍କିକ stillCyclicBwd(ଅଚଳ DfgVertex& vtx) ଅଚଳ {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        ଫେରନ୍ତୁ vtx.foreachSink([&](ଅଚଳ DfgVertex& dst) {  //
            ଫେରନ୍ତୁ m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    ଶୂନ୍ୟ updateAcyclic(ଅଚଳ DfgVertex& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ଅଚଳ DfgVertex& dst) {
            // Sink was known to be acyclic, stop
            ଯଦି (!m_vtx2Scc.at(&dst)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (!stillCyclicFwd(dst)) updateAcyclic(dst);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        // Propagate the update through the graph backward
        vtx.foreachSource([&](ଅଚଳ DfgVertex& src) {
            // Source was known to be acyclic, stop
            ଯଦି (!m_vtx2Scc.at(&src)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (!stillCyclicBwd(src)) updateAcyclic(src);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    ଶୂନ୍ୟ updateReplacement(ଅଚଳ DfgVertex& vtx, DfgVertex& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        ଯଦି (!get(replacement)) {
            replacement.foreachSink([&](DfgVertex& dst) {
                ଯଦି (!get(dst)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                ଯଦି (!stillCyclicFwd(dst)) updateAcyclic(dst);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
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
    ଶୂନ୍ୟ validate() ଅଚଳ {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.forEachVertex([&](ଅଚଳ DfgVertex& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

ଶ୍ରେଣୀ TraceDriver final : ସର୍ବସାଧାରଣ DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    ସଂରଚନା Cache_ଚାବି final {
        DfgVertex* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_ଚାବି() = delete;
        Cache_ଚାବି(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        ସଂରଚନା Hash final {
            size_t operator()(ଅଚଳ Cache_ଚାବି& item) ଅଚଳ {
                // cppcheck-suppress unreadVariable
                V3Hash hash{item.m_vtxp};
                hash += item.m_lsb;
                hash += item.m_msb;
                ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
            }
        };

        ସଂରଚନା Equal final {
            ତାର୍କିକ operator()(ଅଚଳ Cache_ଚାବି& a, ଅଚଳ Cache_ଚାବି& b) ଅଚଳ {
                ଫେରନ୍ତୁ a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
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
    std::unordered_map<Cache_ଚାବି, DfgVertex*, Cache_ଚାବି::Hash, Cache_ଚାବି::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_ଚାବି, DfgVertex*, Cache_ଚାବି::Hash, Cache_ଚାବି::Equal> m_boundaryCache;

#ifdef VL_DEBUG
    std::ofstream m_ଧାଡ଼ି_Coverage_File;  // Line coverage file, just for testing
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
    Vertex* make(ଅଚଳ DfgVertex* refp, uint32_t width) {
        static_assert(std::is_base_of<DfgVertex, Vertex>::ମୂଲ୍ୟ  //
                          && !std::is_base_of<DfgVertexVar, Vertex>::ମୂଲ୍ୟ,
                      "Should only make operation vertices and constants");

        ଯଦି VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, Vertex>::ମୂଲ୍ୟ) {
            DfgConst* ଅଚଳ vtxp = ନୂତନ DfgConst{m_dfg, refp->fileline(), width, 0};
            m_sccInfo.add(*vtxp, 0);
            ଫେରନ୍ତୁ reinterpret_cast<Vertex*>(vtxp);
        } ଅନ୍ୟଥା {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            using Vtx = typename std::conditional<std::is_same<DfgConst, Vertex>::ମୂଲ୍ୟ, DfgSel,
                                                  Vertex>::type;
            Vtx* ଅଚଳ vtxp = ନୂତନ Vtx{m_dfg, refp->fileline(), DfgDataType::packed(width)};
            m_sccInfo.add(*vtxp, 0);
            ଫେରନ୍ତୁ reinterpret_cast<Vertex*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    DfgVertexVar* createTmp(ଅଚଳ ଅକ୍ଷର* prefix, DfgVertex* vtxp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
        DfgVertex::ScopeCache scopeCache;
        AstScope* ଅଚଳ scopep = vtxp->scopep(scopeCache);
        DfgVertexVar* ଅଚଳ varp = m_dfg.makeNewVar(flp, prefix, vtxp->dtype(), scopep);
        varp->vscp()->varp()->isInternal(ସତ୍ୟ);
        varp->tmpForp(varp->vscp());
        m_sccInfo.add(*varp, 0);
        ଫେରନ୍ତୁ varp;
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
        DfgVertex* ଅଚଳ startVtxp = vtxp;
        ଅଚଳ uint32_t startMsb = msb;
        ଅଚଳ uint32_t startLsb = lsb;
        ଅଚଳ size_t pendingBegin = m_pendingps.size();
        DfgVertex* resp = nullptr;

        ଯେତେବେଳେ (ସତ୍ୟ) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->size() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            ଯଦି (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                DfgVertex*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(nullptr))
                          .first->second;
                ଯଦି (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    ଯଦି (DfgVertexSplice* ଅଚଳ splicep = respr->cast<DfgVertexSplice>()) {
                        DfgVertexVar* ଅଚଳ tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    ଯଦି (msb != respr->width() - 1 || lsb != 0) {
                        DfgSel* ଅଚଳ selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                ଭାଙ୍ଗ;
            }

            // Normal cache for intra-component traces
            DfgVertex*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(nullptr))
                                    .first->second;

            // Trace the vertex
            ଯଦି (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } ଅନ୍ୟଥା {
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
                ଯଦି (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ଠେଲ_back(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    ଜାରି;
                }
                respr = m_resp;
            }
            resp = respr;
            ଭାଙ୍ଗ;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, startVtxp, "Tracing driver failed for " << startVtxp->typeName());
        UASSERT_OBJ(resp->width() == (startMsb - startLsb + 1), startVtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        ପାଇଁ (size_t i = pendingBegin; i < m_pendingps.size(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        ଫେରନ୍ତୁ resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    DfgVertex* traceSameIdx(DfgVertex* ଅଚଳ vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = nullptr;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->width() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        ଫେରନ୍ତୁ m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    DfgVertex* trace_ଠେଲ_Idx(DfgVertex* ଅଚଳ vtxp, ଅଚଳ uint32_t idx) {
        ଅଚଳ size_t nIdxs = m_idxs.size();
        m_idxs.ଠେଲ_back(idx);
        DfgVertex* ଅଚଳ resp = traceSameIdx(vtxp);
        m_idxs.କାଢ଼_back();
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        ଫେରନ୍ତୁ resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    DfgVertex* trace_କାଢ଼_Idx(DfgVertex* ଅଚଳ vtxp) {
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        ଅଚଳ size_t nIdxs = m_idxs.size();
        ଅଚଳ uint32_t idx = m_idxs.back();
        m_idxs.କାଢ଼_back();
        DfgVertex* ଅଚଳ resp = m_idxs.empty() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ଠେଲ_back(idx);
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        ଫେରନ୍ତୁ resp;
    }

    template <typename Vertex>
    Vertex* trace_Bitwise_ଦ୍ୱିମିକ(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ଦ୍ୱିମିକ, Vertex>::ମୂଲ୍ୟ,
                      "Should only call on DfgVertexBinary");
        Vertex* ଅଚଳ resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        ଫେରନ୍ତୁ resp;
    }

    template <typename Vertex>
    DfgVertex* traceAddSub(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ଦ୍ୱିମିକ, Vertex>::ମୂଲ୍ୟ,
                      "Should only call on DfgVertexBinary");
        Vertex* ଅଚଳ opp = make<Vertex>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ଅଚଳ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        ଫେରନ୍ତୁ selp;
    }

    template <typename Vertex>
    Vertex* traceReduction(Vertex* vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::ମୂଲ୍ୟ,
                      "Should only call on DfgVertexUnary");
        Vertex* ଅଚଳ resp = make<Vertex>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->width() - 1, 0));
        ଫେରନ୍ତୁ resp;
    }

    template <typename Vertex>
    Vertex* traceCmp(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ଦ୍ୱିମିକ, Vertex>::ମୂଲ୍ୟ,
                      "Should only call on DfgVertexBinary");
        Vertex* ଅଚଳ resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        ଫେରନ୍ତୁ resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define TRACE_RESULT_COVER() \
    କର { \
        ଯଦି (VL_UNLIKELY(m_ଧାଡ଼ି_Coverage_File.is_open())) m_ଧାଡ଼ି_Coverage_File << __LINE__ << '\n'; \
    } ଯେତେବେଳେ (ମିଥ୍ୟା)
#ଅନ୍ୟଥା
#define TRACE_RESULT_COVER() \
    କର { \
    } ଯେତେବେଳେ (ମିଥ୍ୟା)
#endif

#define RETURN_RESULT(vtxp) \
    କର { \
        m_resp = (vtxp); \
        TRACE_RESULT_COVER(); \
        ଫେରନ୍ତୁ; \
    } ଯେତେବେଳେ (ମିଥ୍ୟା)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#define RETURN_RESULT_TAIL(srcp, smsb, slsb) \
    କର { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        DfgVertex* ଅଚଳ tailp = (srcp); \
        ଅଚଳ uint32_t tailMsb = (smsb); \
        ଅଚଳ uint32_t tailLsb = (slsb); \
        m_tailp = tailp; \
        m_msb = tailMsb; \
        m_lsb = tailLsb; \
        TRACE_RESULT_COVER(); \
        ଫେରନ୍ତୁ; \
    } ଯେତେବେଳେ (ମିଥ୍ୟା)

    // VISITORS
    ଶୂନ୍ୟ visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    ଶୂନ୍ୟ visit(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        DfgVertex* defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        ସଂରଚନା Driver final {
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
        ତାର୍କିକ tryWholeDefault = defaultp;
        DfgVertex* coverp = nullptr;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lsb) {
            ଅଚଳ uint32_t msb = lsb + src.width() - 1;
            drivers.emplace_back(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            ଯଦି (m_msb >= lsb && msb >= m_lsb) tryWholeDefault = ମିଥ୍ୟା;
            // If it does not cover the whole searched bit range, move on
            ଯଦି (m_lsb < lsb || msb < m_msb) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        });

        // Trace the driver covering the whole searched range, if there is one
        ଯଦି (coverp) RETURN_RESULT_TAIL(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        ଯଦି (tryWholeDefault) RETURN_RESULT_TAIL(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::sort(drivers.begin(), drivers.end(), [](ଅଚଳ Driver& a, ଅଚଳ Driver& b) {  //
            ଫେରନ୍ତୁ a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::vector<DfgVertex*> termps;
        uint32_t lsb = m_lsb;
        ପାଇଁ (ଅଚଳ Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            ଯଦି (lsb > driver.m_msb) ଜାରି;
            // Driver is above the searched MSB, done
            ଯଦି (driver.m_lsb > m_msb) ଭାଙ୍ଗ;
            // Gap below this driver, trace default to fill it
            ଯଦି (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_back(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ଅଚଳ uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_back(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        ଯଦି (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_back(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.size() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        DfgVertex* resp = termps.front();
        ପାଇଁ (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* ଅଚଳ termp = termps[i];
            DfgConcat* ଅଚଳ catp = make<DfgConcat>(termp, resp->width() + termp->width());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_RESULT(resp);
    }

    ଶୂନ୍ୟ visit(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        DfgVertex* ଅଚଳ defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ଅଚଳ uint32_t idx = m_idxs.back();
        ଯଦି (DfgVertex* ଅଚଳ driverp = vtxp->driverAt(idx)) {
            DfgVertex* ଅଚଳ srcp = driverp->as<DfgUnitArray>()->srcp();
            ଯଦି (srcp->is<DfgVertexSplice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_RESULT(trace_କାଢ଼_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_RESULT(traceSameIdx(defaultp));
    }

    ଶୂନ୍ୟ visit(DfgVertexVar* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        DfgVertex* ଅଚଳ defaultp = vtxp->defaultp();
        DfgVertex* ଅଚଳ drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        ଯଦି (srcp && srcp->is<DfgVertexSplice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_RESULT(m_idxs.empty() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    ଶୂନ୍ୟ visit(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_RESULT(trace_ଠେଲ_Idx(vtxp->fromp(), idxp->toU32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.empty(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ଅଚଳ resp = make<DfgArraySel>(vtxp, vtxp->width());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->width() - 1, 0));
        DfgSel* ଅଚଳ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    ଶୂନ୍ୟ visit(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.back() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_RESULT(trace_କାଢ଼_Idx(vtxp->srcp()));
    }

    ଶୂନ୍ୟ visit(DfgConcat* vtxp) override {
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        ଅଚଳ uint32_t rWidth = rhsp->width();
        // If the traced bits are wholly in the RHS
        ଯଦି (rWidth > m_msb) RETURN_RESULT_TAIL(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        ଯଦି (m_lsb >= rWidth) RETURN_RESULT_TAIL(lhsp, m_msb - rWidth, m_lsb - rWidth);
        // The traced bit spans both sides, trace both
        DfgConcat* ଅଚଳ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, rWidth - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - rWidth, 0));
        RETURN_RESULT(resp);
    }

    ଶୂନ୍ୟ visit(DfgRep* vtxp) override {
        DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        ଅଚଳ uint32_t sWidth = srcp->width();
        // If we need more bits than the source, then we need the whole source
        ଯଦି (m_msb - m_lsb + 1 > sWidth) {
            DfgRep* ଅଚଳ repp = make<DfgRep>(vtxp, vtxp->width());
            repp->srcp(trace(srcp, sWidth - 1, 0));
            DfgSel* ଅଚଳ resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_RESULT(resp);
        }
        // If the requested bits are within the same repliacted word
        ଯଦି (m_msb / sWidth == m_lsb / sWidth) {
            RETURN_RESULT_TAIL(srcp, m_msb % sWidth, m_lsb % sWidth);
        }
        // The requested bits span two replicated words
        DfgConcat* ଅଚଳ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, sWidth - 1, m_lsb % sWidth));
        resp->lhsp(trace(srcp, m_msb % sWidth, 0));
        RETURN_RESULT(resp);
    }

    ଶୂନ୍ୟ visit(DfgExtend* vtxp) override {
        DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        ଅଚଳ uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        ଯଦି (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ଯଦି (m_lsb >= sWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ଅଚଳ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    ଶୂନ୍ୟ visit(DfgExtendS* vtxp) override {
        DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        ଅଚଳ uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        ଯଦି (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        ଯଦି (m_lsb >= sWidth) {
            ଯଦି (m_msb == m_lsb) RETURN_RESULT_TAIL(srcp, sWidth - 1, sWidth - 1);
            DfgExtendS* ଅଚଳ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, sWidth - 1, sWidth - 1));
            RETURN_RESULT(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ଅଚଳ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    ଶୂନ୍ୟ visit(DfgSel* vtxp) override {
        ଅଚଳ uint32_t lsb = vtxp->lsb();
        RETURN_RESULT_TAIL(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    ଶୂନ୍ୟ visit(DfgNot* vtxp) override {
        DfgNot* ଅଚଳ resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    ଶୂନ୍ୟ visit(DfgAnd* vtxp) override { RETURN_RESULT(trace_Bitwise_ଦ୍ୱିମିକ(vtxp)); }
    ଶୂନ୍ୟ visit(DfgOr* vtxp) override { RETURN_RESULT(trace_Bitwise_ଦ୍ୱିମିକ(vtxp)); }
    ଶୂନ୍ୟ visit(DfgXor* vtxp) override { RETURN_RESULT(trace_Bitwise_ଦ୍ୱିମିକ(vtxp)); }
    ଶୂନ୍ୟ visit(DfgAdd* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    ଶୂନ୍ୟ visit(DfgSub* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    ଶୂନ୍ୟ visit(DfgRedAnd* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    ଶୂନ୍ୟ visit(DfgRedOr* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    ଶୂନ୍ୟ visit(DfgRedXor* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    ଶୂନ୍ୟ visit(DfgEq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ଶୂନ୍ୟ visit(DfgNeq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ଶୂନ୍ୟ visit(DfgLt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ଶୂନ୍ୟ visit(DfgLte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ଶୂନ୍ୟ visit(DfgGt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    ଶୂନ୍ୟ visit(DfgGte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }

    ଶୂନ୍ୟ visit(DfgShiftRS* vtxp) override {
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ଅଚଳ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ଅଚଳ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            ଯଦି (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            ଯଦି (m_lsb >= lowerWidth) {
                DfgExtendS* ଅଚଳ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->width() - 1, lhsp->width() - 1));
                RETURN_RESULT(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ଅଚଳ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftRS* ଅଚଳ shiftrsp = make<DfgShiftRS>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        ଯଦି (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrsp);
        DfgSel* ଅଚଳ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    ଶୂନ୍ୟ visit(DfgShiftR* vtxp) override {
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ଅଚଳ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ଅଚଳ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            ଯଦି (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            ଯଦି (m_lsb >= lowerWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ଅଚଳ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftR* ଅଚଳ shiftrp = make<DfgShiftR>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        ଯଦି (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrp);
        DfgSel* ଅଚଳ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    ଶୂନ୍ୟ visit(DfgShiftL* vtxp) override {
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ଅଚଳ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            ଅଚଳ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? vtxp->width() : shiftAmnt;
            // If the traced bits are wholly in the input
            ଯଦି (m_lsb >= lowerWidth) {
                RETURN_RESULT_TAIL(lhsp, m_msb - shiftAmnt, m_lsb - shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            ଯଦି (lowerWidth > m_msb) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ଅଚଳ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->width() - (m_msb - lowerWidth + 1)));
            resp->lhsp(trace(lhsp, m_msb - shiftAmnt, lowerWidth - shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftL* ଅଚଳ shiftlp = make<DfgShiftL>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        ଯଦି (m_lsb == 0) RETURN_RESULT(shiftlp);
        DfgSel* ଅଚଳ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    ଶୂନ୍ୟ visit(DfgCond* vtxp) override {
        DfgCond* ଅଚଳ resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->width() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    ଶୂନ୍ୟ visit(DfgMatchMasked* vtxp) override {
        DfgMatchMasked* ଅଚଳ resp = make<DfgMatchMasked>(vtxp, vtxp->width());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->width() - 1, 0));
        DfgSel* ଅଚଳ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

#undef RETURN_RESULT
#undef TRACE_RESULT_COVER
#undef RETURN_RESULT_TAIL

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    TraceDriver(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#ifdef VL_DEBUG
        ଯଦି (v3Global.opt.debugCheck()) {
            m_ଧାଡ଼ି_Coverage_File.open(  //
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
        ଫେରନ୍ତୁ trace(&vtx, lsb + width - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
ଶ୍ରେଣୀ BitMask final {
    // TYPES
    ଅଚଳ DfgDataType& m_dtype;
    ସଂଘ {
        V3_ସଂଖ୍ୟା m_ଅଙ୍କ;  // The bits, if packed
        std::vector<BitMask> m_sub;  // The per element masks, if aggregate
    };

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    BitMask(File_ଧାଡ଼ି* flp, ଅଚଳ DfgDataType& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        ଯଦି (m_dtype.isPacked()) {
            ନୂତନ (&m_ଅଙ୍କ) V3_ସଂଖ୍ୟା{flp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_dtype.size()), 0};
        } ଅନ୍ୟଥା {
            ନୂତନ (&m_sub) std::vector<BitMask>{};
            m_sub.reserve(m_dtype.size());
            ପାଇଁ (uint32_t i = 0; i < m_dtype.size(); ++i)
                m_sub.emplace_back(flp, m_dtype.elemDtype());
        }
    }
    BitMask(ଅଚଳ DfgVertex& vtx)
        : BitMask{vtx.fileline(), vtx.dtype()} {}
    BitMask(ଅଚଳ BitMask& other)
        : m_dtype{other.m_dtype} {
        ଯଦି (m_dtype.isPacked()) {
            ନୂତନ (&m_ଅଙ୍କ) V3_ସଂଖ୍ୟା{other.m_ଅଙ୍କ};
        } ଅନ୍ୟଥା {
            ନୂତନ (&m_sub) std::vector<BitMask>{other.m_sub};
        }
    }
    BitMask& operator=(ଅଚଳ BitMask& other) {
        ଯଦି (ଏହି != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            ଯଦି (other.m_dtype.isPacked()) {
                m_ଅଙ୍କ = other.m_ଅଙ୍କ;
            } ଅନ୍ୟଥା {
                m_sub = other.m_sub;
            }
        }
        ଫେରନ୍ତୁ *ଏହି;
    }
    ~BitMask() {
        ଯଦି (m_dtype.isPacked()) {
            m_ଅଙ୍କ.~V3_ସଂଖ୍ୟା();
        } ଅନ୍ୟଥା {
            m_sub.~vector<BitMask>();
        }
    }

    // METHODS
    V3_ସଂଖ୍ୟା& ଅଙ୍କ() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ଫେରନ୍ତୁ m_ଅଙ୍କ;
    }
    ଅଚଳ V3_ସଂଖ୍ୟା& ଅଙ୍କ() ଅଚଳ {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        ଫେରନ୍ତୁ m_ଅଙ୍କ;
    }
    std::vector<BitMask>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        ଫେରନ୍ତୁ m_sub;
    }

    ତାର୍କିକ isZero() ଅଚଳ {
        ଯଦି (m_dtype.isPacked()) ଫେରନ୍ତୁ m_ଅଙ୍କ.isEqZero();

        ପାଇଁ (ଅଚଳ BitMask& sub : m_sub) {
            ଯଦି (!sub.isZero()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ତାର୍କିକ isOnes() ଅଚଳ {
        ଯଦି (m_dtype.isPacked()) ଫେରନ୍ତୁ m_ଅଙ୍କ.isEqAllOnes();

        ପାଇଁ (ଅଚଳ BitMask& sub : m_sub) {
            ଯଦି (!sub.isOnes()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଶୂନ୍ୟ setZero() {
        ଯଦି (m_dtype.isPacked()) {
            m_ଅଙ୍କ.setAllBits0();
        } ଅନ୍ୟଥା {
            ପାଇଁ (BitMask& sub : m_sub) sub.setZero();
        }
    }
    ଶୂନ୍ୟ setOnes() {
        ଯଦି (m_dtype.isPacked()) {
            m_ଅଙ୍କ.setAllBits1();
        } ଅନ୍ୟଥା {
            ପାଇଁ (BitMask& sub : m_sub) sub.setOnes();
        }
    }

    ତାର୍କିକ operator==(ଅଚଳ BitMask& other) ଅଚଳ {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        ଯଦି (m_dtype.isPacked()) ଫେରନ୍ତୁ m_ଅଙ୍କ.isCaseEq(other.m_ଅଙ୍କ);
        ଫେରନ୍ତୁ m_sub == other.m_sub;
    }
    ତାର୍କିକ operator!=(ଅଚଳ BitMask& other) ଅଚଳ { ଫେରନ୍ତୁ !(*ଏହି == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    ତାର୍କିକ isContractionOf(ଅଚଳ BitMask& that) ଅଚଳ {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        ଯଦି (m_dtype.isPacked()) {
            ଅଚଳ size_t ଶବ୍ଦଗୁଡ଼ିକ = VL_ଶବ୍ଦଗୁଡ଼ିକ_I(m_dtype.size());
            ପାଇଁ (size_t i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ; ++i) {
                ଯଦି (~m_ଅଙ୍କ.edataWord(i) & that.m_ଅଙ୍କ.edataWord(i)) ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        ପାଇଁ (size_t i = 0; i < m_sub.size(); ++i) {
            ଯଦି (m_sub[i].isContractionOf(that.m_sub[i])) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    std::string toString() ଅଚଳ {
        ଯଦି (m_dtype.isPacked()) ଫେରନ୍ତୁ "0x" + m_ଅଙ୍କ.displayed(m_ଅଙ୍କ.fileline(), "%x");
        std::string result;
        result += "{";
        ପାଇଁ (ଅଚଳ BitMask& sub : m_sub) {
            ଯଦି (&sub != &m_sub.front()) result += ", ";
            result += sub.toString();
        }
        result += "}";
        ଫେରନ୍ତୁ result;
    }
};

ଶ୍ରେଣୀ IndependentBits final : ସର୍ବସାଧାରଣ DfgVisitor {
    // TYPES
    ସଂରଚନା VertexState final {
        size_t m_rpo_ସଂଖ୍ୟା = 0;  // Reverse Postorder Number of vertex
        ତାର୍କିକ m_isOnWorkList = ମିଥ୍ୟା;  // Whether the vertex is on the work list
    };

    // STATE
    ଅଚଳ SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ଅଚଳ DfgVertex*, BitMask> m_vtxp2Mask;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_workQueue;
    std::unordered_map<ଅଚଳ DfgVertex*, VertexState> m_vtxp2State;  // State of each vertex

#ifdef VL_DEBUG
    std::ofstream m_ଧାଡ଼ି_Coverage_File;  // Line coverage file, just for testing
#endif

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    BitMask& mask(ଅଚଳ DfgVertex& vtx) {
        // Look up (or create) mask for 'vtx'
        ଫେରନ୍ତୁ m_vtxp2Mask
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .first->second;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define MASK(vtxp) \
    ([ଏହି](ଅଚଳ DfgVertex& vtx) -> BitMask& { \
        ଯଦି (VL_UNLIKELY(m_ଧାଡ଼ି_Coverage_File.is_open())) m_ଧାଡ଼ି_Coverage_File << __LINE__ << '\n'; \
        ଫେରନ୍ତୁ mask(vtx); \
    }(*vtxp))
#ଅନ୍ୟଥା
#define MASK(vtxp) mask(*vtxp)
#endif

    // Clear all bits at or below the most significant clear bit
    ଶୂନ୍ୟ floodTowardsLsb(V3_ସଂଖ୍ୟା& ଅଙ୍କ) {
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = ଅଙ୍କ.width() - 1; i >= 0; --i) {
            ଯଦି (ଅଙ୍କ.bitIs1(i)) ଜାରି;
            ଅଙ୍କ.opSetRange(0, i + 1, '0');
            ଭାଙ୍ଗ;
        }
    }

    // Clear all bits at or above the least significant clear bit
    ଶୂନ୍ୟ floodTowardsMsb(V3_ସଂଖ୍ୟା& ଅଙ୍କ) {
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଅଙ୍କ.width(); ++i) {
            ଯଦି (ଅଙ୍କ.bitIs1(i)) ଜାରି;
            ଅଙ୍କ.opSetRange(i, ଅଙ୍କ.width() - i, '0');
            ଭାଙ୍ଗ;
        }
    }

    ଶୂନ୍ୟ propagateFromDriver(BitMask& m, ଅଚଳ DfgVertex* srcp) {
        // If there is no driver, we are done
        ଯଦି (!srcp) ଫେରନ୍ତୁ;
        // If it is driven by a splice, we need to combine the masks of the drivers
        ଯଦି (ଅଚଳ DfgSplicePacked* ଅଚଳ splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ଅଚଳ DfgVertex& src, uint32_t lo) {
                m.ଅଙ୍କ().opSelInto(MASK(&src).ଅଙ୍କ(), lo, src.width());
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            ଫେରନ୍ତୁ;
        }
        ଯଦି (ଅଚଳ DfgSpliceArray* ଅଚଳ splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ଅଚଳ DfgVertex& src, uint32_t lo) {
                ଯଦି (ଅଚଳ DfgUnitArray* ଅଚଳ uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = MASK(uap->srcp());
                } ଅନ୍ୟଥା {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            ଫେରନ୍ତୁ;
        }
        // Otherwise, we just use the mask of the single driver
        m = MASK(srcp);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    ଶୂନ୍ୟ visit(DfgVertexVar* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        ଯଦି (vtxp->isVolatile()) ଫେରନ୍ତୁ;

        BitMask& m = MASK(vtxp);
        DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        DfgVertex* ଅଚଳ defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        ଯଦି (defaultp) m = MASK(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    ଶୂନ୍ୟ visit(DfgVertexSplice* vtxp) override {
        propagateFromDriver(MASK(vtxp), vtxp);  // Needed to continue traversal
    }

    ଶୂନ୍ୟ visit(DfgUnitArray* vtxp) override { MASK(vtxp).sub().at(0) = MASK(vtxp->srcp()); }

    ଶୂନ୍ୟ visit(DfgArraySel* vtxp) override {
        DfgVertex* ଅଚଳ fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ idxp = vtxp->bitp()->cast<DfgConst>()) {
            MASK(vtxp) = MASK(vtxp->fromp()).sub().at(idxp->toSizeT());
            ଫେରନ୍ତୁ;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        ଯଦି (MASK(vtxp->bitp()).isOnes() && MASK(fromp).isOnes()) MASK(vtxp).setOnes();
    }

    ଶୂନ୍ୟ visit(DfgConcat* vtxp) override {
        ଅଚଳ DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        ଅଚଳ DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
        m.opSelInto(MASK(rhsp).ଅଙ୍କ(), 0, rhsp->width());
        m.opSelInto(MASK(lhsp).ଅଙ୍କ(), rhsp->width(), lhsp->width());
    }

    ଶୂନ୍ୟ visit(DfgRep* vtxp) override {
        ଅଚଳ uint32_t ଗଣନା_2 = vtxp->ଗଣନା_2();
        ଅଚଳ DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        ଅଚଳ uint32_t sWidth = srcp->width();
        V3_ସଂଖ୍ୟା& vMask = MASK(vtxp).ଅଙ୍କ();
        V3_ସଂଖ୍ୟା& sMask = MASK(srcp).ଅଙ୍କ();
        ପାଇଁ (uint32_t i = 0; i < ଗଣନା_2; ++i) vMask.opSelInto(sMask, i * sWidth, sWidth);
    }

    ଶୂନ୍ୟ visit(DfgSel* vtxp) override {
        ଅଚଳ uint32_t lsb = vtxp->lsb();
        ଅଚଳ uint32_t msb = lsb + vtxp->width() - 1;
        MASK(vtxp).ଅଙ୍କ().opSel(MASK(vtxp->fromp()).ଅଙ୍କ(), msb, lsb);
    }

    ଶୂନ୍ୟ visit(DfgExtend* vtxp) override {
        ଅଚଳ DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        ଅଚଳ uint32_t sWidth = srcp->width();
        V3_ସଂଖ୍ୟା& s = MASK(srcp).ଅଙ୍କ();
        V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, '1');
    }
    ଶୂନ୍ୟ visit(DfgExtendS* vtxp) override {
        ଅଚଳ DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        ଅଚଳ uint32_t sWidth = srcp->width();
        V3_ସଂଖ୍ୟା& s = MASK(srcp).ଅଙ୍କ();
        V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, s.bitIs0(sWidth - 1) ? '0' : '1');
    }

    ଶୂନ୍ୟ visit(DfgNot* vtxp) override {  //
        MASK(vtxp) = MASK(vtxp->srcp());
    }

    ଶୂନ୍ୟ visit(DfgAnd* vtxp) override {  //
        MASK(vtxp).ଅଙ୍କ().opAnd(MASK(vtxp->lhsp()).ଅଙ୍କ(), MASK(vtxp->rhsp()).ଅଙ୍କ());
    }
    ଶୂନ୍ୟ visit(DfgOr* vtxp) override {  //
        MASK(vtxp).ଅଙ୍କ().opAnd(MASK(vtxp->lhsp()).ଅଙ୍କ(), MASK(vtxp->rhsp()).ଅଙ୍କ());
    }
    ଶୂନ୍ୟ visit(DfgXor* vtxp) override {  //
        MASK(vtxp).ଅଙ୍କ().opAnd(MASK(vtxp->lhsp()).ଅଙ୍କ(), MASK(vtxp->rhsp()).ଅଙ୍କ());
    }

    ଶୂନ୍ୟ visit(DfgAdd* vtxp) override {
        V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
        m.opAnd(MASK(vtxp->lhsp()).ଅଙ୍କ(), MASK(vtxp->rhsp()).ଅଙ୍କ());
        floodTowardsMsb(m);
    }
    ଶୂନ୍ୟ visit(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
        m.opAnd(MASK(vtxp->lhsp()).ଅଙ୍କ(), MASK(vtxp->rhsp()).ଅଙ୍କ());
        floodTowardsMsb(m);
    }

    ଶୂନ୍ୟ visit(DfgRedAnd* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }
    ଶୂନ୍ୟ visit(DfgRedOr* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }
    ଶୂନ୍ୟ visit(DfgRedXor* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }

    ଶୂନ୍ୟ visit(DfgEq* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }
    ଶୂନ୍ୟ visit(DfgNeq* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }
    ଶୂନ୍ୟ visit(DfgLt* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }
    ଶୂନ୍ୟ visit(DfgLte* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }
    ଶୂନ୍ୟ visit(DfgGt* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }
    ଶୂନ୍ୟ visit(DfgGte* vtxp) override {
        ଅଚଳ ତାର୍କିକ independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        ଯଦି (independent) MASK(vtxp).setOnes();
    }

    ଶୂନ୍ୟ visit(DfgShiftRS* vtxp) override {
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        ଅଚଳ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        ଯଦି (DfgConst* ଅଚଳ rConstp = rhsp->cast<DfgConst>()) {
            ଅଚଳ uint32_t shiftAmount = rConstp->toU32();
            ଯଦି (shiftAmount >= width) {
                ଯଦି (MASK(lhsp).ଅଙ୍କ().bitIs1(width - 1)) { MASK(vtxp).setOnes(); }
            } ଅନ୍ୟଥା {
                V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
                m.opShiftRS(MASK(lhsp).ଅଙ୍କ(), rConstp->ଅଙ୍କ(), width);
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            ଫେରନ୍ତୁ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ଯଦି (MASK(rhsp).isOnes()) {
            V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
            m = MASK(lhsp).ଅଙ୍କ();
            floodTowardsLsb(m);
        }
    }

    ଶୂନ୍ୟ visit(DfgShiftR* vtxp) override {
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        ଅଚଳ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        ଯଦି (DfgConst* ଅଚଳ rConstp = rhsp->cast<DfgConst>()) {
            ଅଚଳ uint32_t shiftAmount = rConstp->toU32();
            ଯଦି (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } ଅନ୍ୟଥା {
                V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
                m.opShiftR(MASK(lhsp).ଅଙ୍କ(), rConstp->ଅଙ୍କ());
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            ଫେରନ୍ତୁ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        ଯଦି (MASK(rhsp).isOnes()) {
            V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
            m = MASK(lhsp).ଅଙ୍କ();
            floodTowardsLsb(m);
        }
    }

    ଶୂନ୍ୟ visit(DfgShiftL* vtxp) override {
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        ଅଚଳ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        ଯଦି (DfgConst* ଅଚଳ rConstp = rhsp->cast<DfgConst>()) {
            ଅଚଳ uint32_t shiftAmount = rConstp->toU32();
            ଯଦି (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } ଅନ୍ୟଥା {
                V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
                m.opShiftL(MASK(lhsp).ଅଙ୍କ(), rConstp->ଅଙ୍କ());
                m.opSetRange(0, shiftAmount, '1');
            }
            ଫେରନ୍ତୁ;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        ଯଦି (MASK(rhsp).isOnes()) {
            V3_ସଂଖ୍ୟା& m = MASK(vtxp).ଅଙ୍କ();
            m = MASK(lhsp).ଅଙ୍କ();
            floodTowardsMsb(m);
        }
    }

    ଶୂନ୍ୟ visit(DfgCond* vtxp) override {
        ଯଦି (MASK(vtxp->condp()).isOnes()) {
            MASK(vtxp).ଅଙ୍କ().opAnd(MASK(vtxp->thenp()).ଅଙ୍କ(), MASK(vtxp->elsep()).ଅଙ୍କ());
        }
    }

    ଶୂନ୍ୟ visit(DfgMatchMasked* vtxp) override {
        ଯଦି (MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->matchp()).isOnes()) { MASK(vtxp).setOnes(); }
    }

#undef MASK

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    ଶୂନ୍ୟ enqueueSinks(DfgVertex& vtx) {
        vtx.foreachSink([&](DfgVertex& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            ଯଦି (!m_sccInfo.get(sink)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Otherwise just enqueue it
            VertexState& state = m_vtxp2State.at(&sink);
            ଯଦି (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_workQueue.ଠେଲ(state.m_rpo_ସଂଖ୍ୟା);
            }
            state.m_isOnWorkList = ସତ୍ୟ;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
    };

    ଶୂନ୍ୟ dfsPostOrder(std::unordered_set<DfgVertex*>& visited,  //
                      std::vector<DfgVertex*>& postOrderEnumeration,  //
                      DfgVertex& vtx) {
        // Mark visited, skip if already visited
        ଯଦି (!visited.ଭର୍ତ୍ତି(&vtx).second) ଫେରନ୍ତୁ;
        // Visit un-visited successors
        vtx.foreachSink([&](DfgVertex& snk) {
            dfsPostOrder(visited, postOrderEnumeration, snk);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        // Add to post order enumeration
        postOrderEnumeration.emplace_back(&vtx);
    };

    IndependentBits(DfgGraph& dfg, ଅଚଳ SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#ifdef VL_DEBUG
        ଯଦି (v3Global.opt.debugCheck()) {
            m_ଧାଡ଼ି_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::app);
        }
#endif

        // Compute Reverse-Postorder enumeration of vertices
        std::vector<DfgVertex*> rpoEnumeration;
        std::unordered_set<DfgVertex*> visited;
        dfg.forEachVertex([&](DfgVertex& vtx) {
            ଯଦି (!vtx.nInputs()) dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        dfg.forEachVertex([&](DfgVertex& vtx) {  //
            dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        std::reverse(rpoEnumeration.begin(), rpoEnumeration.end());
        UASSERT(rpoEnumeration.size() == dfg.size(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        ପାଇଁ (size_t i = 0; i < rpoEnumeration.size(); ++i) {
            VertexState& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_ସଂଖ୍ୟା = i;
            state.m_isOnWorkList = ମିଥ୍ୟା;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        ପାଇଁ (DfgVertex* ଅଚଳ vtxp : rpoEnumeration) {
            ଯଦି (m_sccInfo.get(*vtxp)) ଜାରି;
            mask(*vtxp).setOnes();
        }
        ପାଇଁ (DfgVertex* ଅଚଳ vtxp : rpoEnumeration) {
            ଯଦି (!m_sccInfo.get(*vtxp)) ଜାରି;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << mask(*vtxp).toString());
            ଯଦି (!mask(*vtxp).isZero()) enqueueSinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        ଯେତେବେଳେ (!m_workQueue.empty()) {
            // Grab next item
            DfgVertex* ଅଚଳ currp = rpoEnumeration[m_workQueue.ଶୀର୍ଷ()];
            m_workQueue.କାଢ଼();
            m_vtxp2State.at(currp).m_isOnWorkList = ମିଥ୍ୟା;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ଅଚଳ BitMask& currMask = mask(*currp);
            // Remember current mask so we can check if it changed
            ଅଚଳ BitMask prevMask = currMask;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            ଯଦି (prevMask != currMask) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prevMask.toString()
                             << "\n      to: " << currMask.toString());
                enqueueSinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                ଯଦି (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!currMask.isContractionOf(prevMask), currp,
                                "Mask should only expand");
                }
            }
        }
    }

ସର୍ବସାଧାରଣ:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    ସ୍ଥିର std::unordered_map<ଅଚଳ DfgVertex*, BitMask> apply(DfgGraph& dfg,
                                                               ଅଚଳ SccInfo& sccInfo) {
        ଫେରନ୍ତୁ std::move(IndependentBits{dfg, sccInfo}.m_vtxp2Mask);
    }
};

ଶ୍ରେଣୀ FixUp final {
    DfgGraph& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ଅଚଳ std::unordered_map<ଅଚଳ DfgVertex*, BitMask> m_independentBits
        = IndependentBits::apply(m_dfg, m_sccInfo);
    size_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ସ୍ଥିର BitMask computeUsedBits(DfgVertex& vtx) {
        BitMask result{vtx};
        vtx.foreachSink([&result](DfgVertex& sink) {
            // If used via a Sel, mark the selected bits used
            ଯଦି (ଅଚଳ DfgSel* ଅଚଳ selp = sink.cast<DfgSel>()) {
                result.ଅଙ୍କ().opSetRange(selp->lsb(), selp->width(), '1');
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            // Used without a Sel, so all bits are used
            result.setOnes();
            ଫେରନ୍ତୁ ସତ୍ୟ;
        });
        ଫେରନ୍ତୁ result;
    }

    ସ୍ଥିର std::string debugStr(ଅଚଳ DfgVertex& vtx) {  // LCOV_EXCL_START
        ଯଦି (ଅଚଳ DfgArraySel* ଅଚଳ aselp = vtx.cast<DfgArraySel>()) {
            ଅଚଳ size_t i = aselp->bitp()->as<DfgConst>()->toSizeT();
            ଫେରନ୍ତୁ debugStr(*aselp->fromp()) + "[" + std::to_string(i) + "]";
        }
        ଯଦି (ଅଚଳ DfgVertexVar* ଅଚଳ varp = vtx.cast<DfgVertexVar>()) {
            ଫେରନ୍ତୁ varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    ଶୂନ୍ୟ gatherTerms(std::vector<DfgVertex*>& termps, DfgVertex& vtx, ଅଚଳ V3_ସଂଖ୍ୟା& indpBits,
                     ଅଚଳ uint32_t hi, ଅଚଳ uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        ତାର୍କିକ isIndependent = indpBits.bitIs1(lo);
        uint32_t lsb = lo;
        ପାଇଁ (uint32_t msb = lo; msb <= hi; ++msb) {
            ଅଚଳ ତାର୍କିକ endRange = msb == hi || isIndependent != indpBits.bitIs1(msb + 1);
            ଯଦି (!endRange) ଜାରି;
            ଅଚଳ uint32_t width = msb - lsb + 1;
            // Compute the term to use for this range
            ଯଦି (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_back(m_traceDriver.apply(vtx, lsb, width));
            } ଅନ୍ୟଥା {
                // If dependent, fall back on using the part of the variable
                DfgSel* ଅଚଳ selp = ନୂତନ DfgSel{m_dfg, vtx.fileline(), DfgDataType::packed(width)};
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

    ଶୂନ୍ୟ fixUpPacked(DfgVertex& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ଅଚଳ BitMask& indpBits = m_independentBits.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indpBits.toString());
        // Can't do anything if all bits are dependent
        ଯଦି (indpBits.isZero()) ଫେରନ୍ତୁ;

        // Figure out which bits of 'vtxp' are used
        ଅଚଳ BitMask usedBits = computeUsedBits(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << usedBits.toString());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_ସଂଖ୍ୟା usedAndIndependent{vtx.fileline(), static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(vtx.width()), 0};
        usedAndIndependent.opAnd(usedBits.ଅଙ୍କ(), indpBits.ଅଙ୍କ());
        ଯଦି (usedAndIndependent.isEqZero()) ଫେରନ୍ତୁ;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::vector<DfgVertex*> termps;

        // Iterate through consecutive used/unused ranges
        File_ଧାଡ଼ି* ଅଚଳ flp = vtx.fileline();
        ଅଚଳ uint32_t width = vtx.width();
        ତାର୍କିକ isUsed = usedBits.ଅଙ୍କ().bitIs1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        ପାଇଁ (uint32_t msb = 0; msb < width; ++msb) {
            ଅଚଳ ତାର୍କିକ endRange = msb == width - 1 || isUsed != usedBits.ଅଙ୍କ().bitIs1(msb + 1);
            ଯଦି (!endRange) ଜାରି;
            ଯଦି (isUsed) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indpBits.ଅଙ୍କ(), msb, lsb);
            } ଅନ୍ୟଥା {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ଅଚଳ constp = ନୂତନ DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.add(*constp, 0);
                termps.emplace_back(constp);
            }
            // Next iteration
            isUsed = !isUsed;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        DfgVertex* replacementp = termps.front();
        ଅଚଳ uint64_t vComp = m_sccInfo.get(vtx);
        ପାଇଁ (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* ଅଚଳ termp = termps[i];
            ଅଚଳ uint32_t catWidth = replacementp->width() + termp->width();
            DfgConcat* ଅଚଳ catp = ନୂତନ DfgConcat{m_dfg, flp, DfgDataType::packed(catWidth)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ଅଚଳ uint64_t tComp = m_sccInfo.get(*termp);
            ଅଚଳ uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.add(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        ପାଇଁ (DfgVertex* ଅଚଳ termp : termps) {
            ଯଦି (DfgSel* ଅଚଳ selp = termp->cast<DfgSel>()) {
                ଯଦି (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        ଯଦି (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    ଶୂନ୍ୟ ମୁଖ୍ୟ(DfgVertexVar& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        ଯଦି (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fixUpPacked(var);
        } ଅନ୍ୟଥା ଯଦି (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ଅଚଳ uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](DfgVertex& sink) {
                // Ignore if sink is not part of same cycle
                ଯଦି (m_sccInfo.get(sink) != component) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Only handle ArraySels with constant index
                DfgArraySel* ଅଚଳ aselp = sink.cast<DfgArraySel>();
                ଯଦି (!aselp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                ଯଦି (!aselp->bitp()->is<DfgConst>()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Fix up the word
                fixUpPacked(*aselp);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    FixUp(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

ସର୍ବସାଧାରଣ:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ସ୍ଥିର size_t apply(DfgGraph& dfg, SccInfo& sccInfo) {
        ଯଦି (!sccInfo.isCyclic()) ଫେରନ୍ତୁ 0;

        FixUp fixUp{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        ପାଇଁ (DfgVertexVar& vtx : dfg.varVertices()) {
            ଯଦି (!sccInfo.get(vtx)) ଜାରି;
            fixUp.ମୁଖ୍ୟ(vtx);
            ଯଦି (!sccInfo.isCyclic()) ଭାଙ୍ଗ;
        }
        // Return the number of improvements made
        ଫେରନ୍ତୁ fixUp.m_nImprovements;
    }
};

ଶୂନ୍ୟ breakCycles(DfgGraph& dfg, V3DfgBreakCyclesContext& ctx) {
    // Shorthand for dumping graph at given dump level
    ଅଚଳ auto dump = [&](ପୂର୍ଣ୍ଣସଂଖ୍ୟା level, ଅଚଳ DfgGraph& dfg, ଅଚଳ std::string& name) {
        ଯଦି (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    size_t nImprovements = 0;
    size_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    କର {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ଅଚଳ size_t nFixed = FixUp::apply(dfg, sccInfo);
        ଯଦି (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        ଯଦି (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        ଯଦି (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            ଫେରନ୍ତୁ;
        }
    } ଯେତେବେଳେ (nImprovements != prevNImprovements);

    // Debug dump
    ଯଦି (dumpDfgLevel() >= 9) {
        ଅଚଳ SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ଅଚଳ DfgVertex& vtx) {
            ଫେରନ୍ତୁ sccInfo.get(vtx);  //
        });
    }

    // Accounting
    ଯଦି (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } ଅନ୍ୟଥା {
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
    ପାଇଁ (DfgVertexVar& vtx : dfg.varVertices()) {
        ଯଦି (!vtx.tmpForp()) varps.ଠେଲ_back(&vtx);
    }
    std::stable_partition(varps.begin(), varps.end(), [](ଅଚଳ DfgVertexVar* varp) {
        ଫେରନ୍ତୁ varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    ପାଇଁ (DfgVertexVar* ଅଚଳ varp : varps) {
        // Stop as soon as it becomes acyclic
        ଯଦି (!sccInfo.isCyclic()) ଭାଙ୍ଗ;

        // Ignore if variable is not part of a cycle
        ଯଦି (!sccInfo.get(*varp)) ଜାରି;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* ଅଚଳ prevp = ନୂତନ DfgPrev{dfg, varp->vscp()};
        sccInfo.add(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    ଯଦି (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
ଶୂନ୍ୟ V3DfgPasses::breakCycles(DfgGraph& dfg, V3DfgContext& ctx) {
    V3DfgBreakCycles::breakCycles(dfg, ctx.m_breakCyclesContext);
    ଯଦି (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::removeUnused(dfg);
}
