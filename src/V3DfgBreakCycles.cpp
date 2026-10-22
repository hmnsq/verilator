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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Hash.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <fstream>
#અન્તર્ભાવ <queue>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

નામાકાશ V3DfgBreakCycles {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
વર્ગ SccInfo final {
    DfgGraph& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<અચલ DfgVertex*, uint64_t> m_vtx2Scc;
    size_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

જાહેર:
    SccInfo(DfgGraph& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.forEachVertex([&](અચલ DfgVertex& vtx) {
            અચલ uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            જો (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    તાર્કિક isCyclic() અચલ { પાછા m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(અચલ DfgVertex& vtx) અચલ { પાછા m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    શૂન્ય add(અચલ DfgVertex& vtx, uint64_t sccIdx) {
        અચલ તાર્કિક newEntry = m_vtx2Scc.emplace(&vtx, sccIdx).second;
        UASSERT_OBJ(newEntry, &vtx, "Vertex already inserted");
        જો (sccIdx) ++m_nCyclicVertices;
    }

    તાર્કિક stillCyclicFwd(અચલ DfgVertex& vtx) અચલ {
        // If it only reads non-cyclic vertices, it has also become acyclic
        પાછા vtx.foreachSource([&](અચલ DfgVertex& src) {  //
            પાછા m_vtx2Scc.at(&src);
        });
    }

    તાર્કિક stillCyclicBwd(અચલ DfgVertex& vtx) અચલ {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        પાછા vtx.foreachSink([&](અચલ DfgVertex& dst) {  //
            પાછા m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    શૂન્ય updateAcyclic(અચલ DfgVertex& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](અચલ DfgVertex& dst) {
            // Sink was known to be acyclic, stop
            જો (!m_vtx2Scc.at(&dst)) પાછા ખોટું;
            જો (!stillCyclicFwd(dst)) updateAcyclic(dst);
            પાછા ખોટું;
        });
        // Propagate the update through the graph backward
        vtx.foreachSource([&](અચલ DfgVertex& src) {
            // Source was known to be acyclic, stop
            જો (!m_vtx2Scc.at(&src)) પાછા ખોટું;
            જો (!stillCyclicBwd(src)) updateAcyclic(src);
            પાછા ખોટું;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    શૂન્ય updateReplacement(અચલ DfgVertex& vtx, DfgVertex& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        જો (!get(replacement)) {
            replacement.foreachSink([&](DfgVertex& dst) {
                જો (!get(dst)) પાછા ખોટું;
                જો (!stillCyclicFwd(dst)) updateAcyclic(dst);
                પાછા ખોટું;
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
    શૂન્ય validate() અચલ {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.forEachVertex([&](અચલ DfgVertex& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

વર્ગ TraceDriver final : જાહેર DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    સંરચના Cache_ચાવી final {
        DfgVertex* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_ચાવી() = delete;
        Cache_ચાવી(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        સંરચના Hash final {
            size_t operator()(અચલ Cache_ચાવી& item) અચલ {
                // cppcheck-suppress unreadVariable
                V3Hash hash{item.m_vtxp};
                hash += item.m_lsb;
                hash += item.m_msb;
                પાછા hash.કિંમત();
            }
        };

        સંરચના Equal final {
            તાર્કિક operator()(અચલ Cache_ચાવી& a, અચલ Cache_ચાવી& b) અચલ {
                પાછા a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
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
    std::unordered_map<Cache_ચાવી, DfgVertex*, Cache_ચાવી::Hash, Cache_ચાવી::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_ચાવી, DfgVertex*, Cache_ચાવી::Hash, Cache_ચાવી::Equal> m_boundaryCache;

#ifdef VL_DEBUG
    std::ofstream m_લીટી_Coverage_File;  // Line coverage file, just for testing
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
    Vertex* make(અચલ DfgVertex* refp, uint32_t width) {
        static_assert(std::is_base_of<DfgVertex, Vertex>::કિંમત  //
                          && !std::is_base_of<DfgVertexVar, Vertex>::કિંમત,
                      "Should only make operation vertices and constants");

        જો VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, Vertex>::કિંમત) {
            DfgConst* અચલ vtxp = નવું DfgConst{m_dfg, refp->fileline(), width, 0};
            m_sccInfo.add(*vtxp, 0);
            પાછા reinterpret_cast<Vertex*>(vtxp);
        } નહીંતર {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            using Vtx = typename std::conditional<std::is_same<DfgConst, Vertex>::કિંમત, DfgSel,
                                                  Vertex>::type;
            Vtx* અચલ vtxp = નવું Vtx{m_dfg, refp->fileline(), DfgDataType::packed(width)};
            m_sccInfo.add(*vtxp, 0);
            પાછા reinterpret_cast<Vertex*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    DfgVertexVar* createTmp(અચલ અક્ષર* prefix, DfgVertex* vtxp) {
        File_લીટી* અચલ flp = vtxp->fileline();
        DfgVertex::ScopeCache scopeCache;
        AstScope* અચલ scopep = vtxp->scopep(scopeCache);
        DfgVertexVar* અચલ varp = m_dfg.makeNewVar(flp, prefix, vtxp->dtype(), scopep);
        varp->vscp()->varp()->isInternal(સાચું);
        varp->tmpForp(varp->vscp());
        m_sccInfo.add(*varp, 0);
        પાછા varp;
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
        DfgVertex* અચલ startVtxp = vtxp;
        અચલ uint32_t startMsb = msb;
        અચલ uint32_t startLsb = lsb;
        અચલ size_t pendingBegin = m_pendingps.size();
        DfgVertex* resp = nullptr;

        જ્યારે (સાચું) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->size() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            જો (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                DfgVertex*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(nullptr))
                          .first->second;
                જો (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    જો (DfgVertexSplice* અચલ splicep = respr->cast<DfgVertexSplice>()) {
                        DfgVertexVar* અચલ tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    જો (msb != respr->width() - 1 || lsb != 0) {
                        DfgSel* અચલ selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                તોડો;
            }

            // Normal cache for intra-component traces
            DfgVertex*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(nullptr))
                                    .first->second;

            // Trace the vertex
            જો (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } નહીંતર {
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
                જો (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ધકેલો_back(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    ચાલુ;
                }
                respr = m_resp;
            }
            resp = respr;
            તોડો;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, startVtxp, "Tracing driver failed for " << startVtxp->typeName());
        UASSERT_OBJ(resp->width() == (startMsb - startLsb + 1), startVtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        માટે (size_t i = pendingBegin; i < m_pendingps.size(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        પાછા resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    DfgVertex* traceSameIdx(DfgVertex* અચલ vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = nullptr;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->width() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        પાછા m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    DfgVertex* trace_ધકેલો_Idx(DfgVertex* અચલ vtxp, અચલ uint32_t idx) {
        અચલ size_t nIdxs = m_idxs.size();
        m_idxs.ધકેલો_back(idx);
        DfgVertex* અચલ resp = traceSameIdx(vtxp);
        m_idxs.કાઢો_back();
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        પાછા resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    DfgVertex* trace_કાઢો_Idx(DfgVertex* અચલ vtxp) {
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        અચલ size_t nIdxs = m_idxs.size();
        અચલ uint32_t idx = m_idxs.back();
        m_idxs.કાઢો_back();
        DfgVertex* અચલ resp = m_idxs.empty() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ધકેલો_back(idx);
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        પાછા resp;
    }

    template <typename Vertex>
    Vertex* trace_Bitwise_દ્વિઅંકી(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_દ્વિઅંકી, Vertex>::કિંમત,
                      "Should only call on DfgVertexBinary");
        Vertex* અચલ resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        પાછા resp;
    }

    template <typename Vertex>
    DfgVertex* traceAddSub(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_દ્વિઅંકી, Vertex>::કિંમત,
                      "Should only call on DfgVertexBinary");
        Vertex* અચલ opp = make<Vertex>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* અચલ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        પાછા selp;
    }

    template <typename Vertex>
    Vertex* traceReduction(Vertex* vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::કિંમત,
                      "Should only call on DfgVertexUnary");
        Vertex* અચલ resp = make<Vertex>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->width() - 1, 0));
        પાછા resp;
    }

    template <typename Vertex>
    Vertex* traceCmp(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_દ્વિઅંકી, Vertex>::કિંમત,
                      "Should only call on DfgVertexBinary");
        Vertex* અચલ resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        પાછા resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define TRACE_RESULT_COVER() \
    કરો { \
        જો (VL_UNLIKELY(m_લીટી_Coverage_File.is_open())) m_લીટી_Coverage_File << __LINE__ << '\n'; \
    } જ્યારે (ખોટું)
#નહીંતર
#define TRACE_RESULT_COVER() \
    કરો { \
    } જ્યારે (ખોટું)
#endif

#define RETURN_RESULT(vtxp) \
    કરો { \
        m_resp = (vtxp); \
        TRACE_RESULT_COVER(); \
        પાછા; \
    } જ્યારે (ખોટું)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#define RETURN_RESULT_TAIL(srcp, smsb, slsb) \
    કરો { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        DfgVertex* અચલ tailp = (srcp); \
        અચલ uint32_t tailMsb = (smsb); \
        અચલ uint32_t tailLsb = (slsb); \
        m_tailp = tailp; \
        m_msb = tailMsb; \
        m_lsb = tailLsb; \
        TRACE_RESULT_COVER(); \
        પાછા; \
    } જ્યારે (ખોટું)

    // VISITORS
    શૂન્ય visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    શૂન્ય visit(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        DfgVertex* defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        સંરચના Driver final {
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
        તાર્કિક tryWholeDefault = defaultp;
        DfgVertex* coverp = nullptr;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lsb) {
            અચલ uint32_t msb = lsb + src.width() - 1;
            drivers.emplace_back(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            જો (m_msb >= lsb && msb >= m_lsb) tryWholeDefault = ખોટું;
            // If it does not cover the whole searched bit range, move on
            જો (m_lsb < lsb || msb < m_msb) પાછા ખોટું;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            પાછા સાચું;
        });

        // Trace the driver covering the whole searched range, if there is one
        જો (coverp) RETURN_RESULT_TAIL(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        જો (tryWholeDefault) RETURN_RESULT_TAIL(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::sort(drivers.begin(), drivers.end(), [](અચલ Driver& a, અચલ Driver& b) {  //
            પાછા a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::vector<DfgVertex*> termps;
        uint32_t lsb = m_lsb;
        માટે (અચલ Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            જો (lsb > driver.m_msb) ચાલુ;
            // Driver is above the searched MSB, done
            જો (driver.m_lsb > m_msb) તોડો;
            // Gap below this driver, trace default to fill it
            જો (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_back(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            અચલ uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_back(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        જો (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_back(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.size() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        DfgVertex* resp = termps.front();
        માટે (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* અચલ termp = termps[i];
            DfgConcat* અચલ catp = make<DfgConcat>(termp, resp->width() + termp->width());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_RESULT(resp);
    }

    શૂન્ય visit(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        DfgVertex* અચલ defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        અચલ uint32_t idx = m_idxs.back();
        જો (DfgVertex* અચલ driverp = vtxp->driverAt(idx)) {
            DfgVertex* અચલ srcp = driverp->as<DfgUnitArray>()->srcp();
            જો (srcp->is<DfgVertexSplice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_RESULT(trace_કાઢો_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_RESULT(traceSameIdx(defaultp));
    }

    શૂન્ય visit(DfgVertexVar* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        DfgVertex* અચલ srcp = vtxp->srcp();
        DfgVertex* અચલ defaultp = vtxp->defaultp();
        DfgVertex* અચલ drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        જો (srcp && srcp->is<DfgVertexSplice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_RESULT(m_idxs.empty() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    શૂન્ય visit(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        જો (અચલ DfgConst* અચલ idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_RESULT(trace_ધકેલો_Idx(vtxp->fromp(), idxp->toU32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.empty(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* અચલ resp = make<DfgArraySel>(vtxp, vtxp->width());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->width() - 1, 0));
        DfgSel* અચલ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    શૂન્ય visit(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.back() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_RESULT(trace_કાઢો_Idx(vtxp->srcp()));
    }

    શૂન્ય visit(DfgConcat* vtxp) override {
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        અચલ uint32_t rWidth = rhsp->width();
        // If the traced bits are wholly in the RHS
        જો (rWidth > m_msb) RETURN_RESULT_TAIL(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        જો (m_lsb >= rWidth) RETURN_RESULT_TAIL(lhsp, m_msb - rWidth, m_lsb - rWidth);
        // The traced bit spans both sides, trace both
        DfgConcat* અચલ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, rWidth - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - rWidth, 0));
        RETURN_RESULT(resp);
    }

    શૂન્ય visit(DfgRep* vtxp) override {
        DfgVertex* અચલ srcp = vtxp->srcp();
        અચલ uint32_t sWidth = srcp->width();
        // If we need more bits than the source, then we need the whole source
        જો (m_msb - m_lsb + 1 > sWidth) {
            DfgRep* અચલ repp = make<DfgRep>(vtxp, vtxp->width());
            repp->srcp(trace(srcp, sWidth - 1, 0));
            DfgSel* અચલ resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_RESULT(resp);
        }
        // If the requested bits are within the same repliacted word
        જો (m_msb / sWidth == m_lsb / sWidth) {
            RETURN_RESULT_TAIL(srcp, m_msb % sWidth, m_lsb % sWidth);
        }
        // The requested bits span two replicated words
        DfgConcat* અચલ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, sWidth - 1, m_lsb % sWidth));
        resp->lhsp(trace(srcp, m_msb % sWidth, 0));
        RETURN_RESULT(resp);
    }

    શૂન્ય visit(DfgExtend* vtxp) override {
        DfgVertex* અચલ srcp = vtxp->srcp();
        અચલ uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        જો (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        જો (m_lsb >= sWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* અચલ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    શૂન્ય visit(DfgExtendS* vtxp) override {
        DfgVertex* અચલ srcp = vtxp->srcp();
        અચલ uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        જો (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        જો (m_lsb >= sWidth) {
            જો (m_msb == m_lsb) RETURN_RESULT_TAIL(srcp, sWidth - 1, sWidth - 1);
            DfgExtendS* અચલ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, sWidth - 1, sWidth - 1));
            RETURN_RESULT(resp);
        }
        // The traced bits span both sides
        DfgExtendS* અચલ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    શૂન્ય visit(DfgSel* vtxp) override {
        અચલ uint32_t lsb = vtxp->lsb();
        RETURN_RESULT_TAIL(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    શૂન્ય visit(DfgNot* vtxp) override {
        DfgNot* અચલ resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    શૂન્ય visit(DfgAnd* vtxp) override { RETURN_RESULT(trace_Bitwise_દ્વિઅંકી(vtxp)); }
    શૂન્ય visit(DfgOr* vtxp) override { RETURN_RESULT(trace_Bitwise_દ્વિઅંકી(vtxp)); }
    શૂન્ય visit(DfgXor* vtxp) override { RETURN_RESULT(trace_Bitwise_દ્વિઅંકી(vtxp)); }
    શૂન્ય visit(DfgAdd* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    શૂન્ય visit(DfgSub* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    શૂન્ય visit(DfgRedAnd* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    શૂન્ય visit(DfgRedOr* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    શૂન્ય visit(DfgRedXor* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    શૂન્ય visit(DfgEq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    શૂન્ય visit(DfgNeq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    શૂન્ય visit(DfgLt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    શૂન્ય visit(DfgLte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    શૂન્ય visit(DfgGt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    શૂન્ય visit(DfgGte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }

    શૂન્ય visit(DfgShiftRS* vtxp) override {
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        જો (અચલ DfgConst* અચલ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            અચલ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            અચલ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            જો (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            જો (m_lsb >= lowerWidth) {
                DfgExtendS* અચલ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->width() - 1, lhsp->width() - 1));
                RETURN_RESULT(resp);
            }
            // The traced bits span both sides
            DfgExtendS* અચલ resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftRS* અચલ shiftrsp = make<DfgShiftRS>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        જો (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrsp);
        DfgSel* અચલ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    શૂન્ય visit(DfgShiftR* vtxp) override {
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        જો (અચલ DfgConst* અચલ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            અચલ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            અચલ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            જો (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            જો (m_lsb >= lowerWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* અચલ resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftR* અચલ shiftrp = make<DfgShiftR>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        જો (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrp);
        DfgSel* અચલ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    શૂન્ય visit(DfgShiftL* vtxp) override {
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        જો (અચલ DfgConst* અચલ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            અચલ uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            અચલ uint32_t lowerWidth = shiftAmnt > vtxp->width() ? vtxp->width() : shiftAmnt;
            // If the traced bits are wholly in the input
            જો (m_lsb >= lowerWidth) {
                RETURN_RESULT_TAIL(lhsp, m_msb - shiftAmnt, m_lsb - shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            જો (lowerWidth > m_msb) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* અચલ resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->width() - (m_msb - lowerWidth + 1)));
            resp->lhsp(trace(lhsp, m_msb - shiftAmnt, lowerWidth - shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftL* અચલ shiftlp = make<DfgShiftL>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        જો (m_lsb == 0) RETURN_RESULT(shiftlp);
        DfgSel* અચલ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    શૂન્ય visit(DfgCond* vtxp) override {
        DfgCond* અચલ resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->width() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    શૂન્ય visit(DfgMatchMasked* vtxp) override {
        DfgMatchMasked* અચલ resp = make<DfgMatchMasked>(vtxp, vtxp->width());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->width() - 1, 0));
        DfgSel* અચલ selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

#undef RETURN_RESULT
#undef TRACE_RESULT_COVER
#undef RETURN_RESULT_TAIL

જાહેર:
    // CONSTRUCTOR
    TraceDriver(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#ifdef VL_DEBUG
        જો (v3Global.opt.debugCheck()) {
            m_લીટી_Coverage_File.open(  //
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
        પાછા trace(&vtx, lsb + width - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
વર્ગ BitMask final {
    // TYPES
    અચલ DfgDataType& m_dtype;
    સંઘ {
        V3_સંખ્યા m_અંક;  // The bits, if packed
        std::vector<BitMask> m_sub;  // The per element masks, if aggregate
    };

જાહેર:
    // CONSTRUCTOR
    BitMask(File_લીટી* flp, અચલ DfgDataType& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        જો (m_dtype.isPacked()) {
            નવું (&m_અંક) V3_સંખ્યા{flp, static_cast<પૂર્ણાંક>(m_dtype.size()), 0};
        } નહીંતર {
            નવું (&m_sub) std::vector<BitMask>{};
            m_sub.reserve(m_dtype.size());
            માટે (uint32_t i = 0; i < m_dtype.size(); ++i)
                m_sub.emplace_back(flp, m_dtype.elemDtype());
        }
    }
    BitMask(અચલ DfgVertex& vtx)
        : BitMask{vtx.fileline(), vtx.dtype()} {}
    BitMask(અચલ BitMask& other)
        : m_dtype{other.m_dtype} {
        જો (m_dtype.isPacked()) {
            નવું (&m_અંક) V3_સંખ્યા{other.m_અંક};
        } નહીંતર {
            નવું (&m_sub) std::vector<BitMask>{other.m_sub};
        }
    }
    BitMask& operator=(અચલ BitMask& other) {
        જો (આ != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            જો (other.m_dtype.isPacked()) {
                m_અંક = other.m_અંક;
            } નહીંતર {
                m_sub = other.m_sub;
            }
        }
        પાછા *આ;
    }
    ~BitMask() {
        જો (m_dtype.isPacked()) {
            m_અંક.~V3_સંખ્યા();
        } નહીંતર {
            m_sub.~vector<BitMask>();
        }
    }

    // METHODS
    V3_સંખ્યા& અંક() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        પાછા m_અંક;
    }
    અચલ V3_સંખ્યા& અંક() અચલ {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        પાછા m_અંક;
    }
    std::vector<BitMask>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        પાછા m_sub;
    }

    તાર્કિક isZero() અચલ {
        જો (m_dtype.isPacked()) પાછા m_અંક.isEqZero();

        માટે (અચલ BitMask& sub : m_sub) {
            જો (!sub.isZero()) પાછા ખોટું;
        }
        પાછા સાચું;
    }
    તાર્કિક isOnes() અચલ {
        જો (m_dtype.isPacked()) પાછા m_અંક.isEqAllOnes();

        માટે (અચલ BitMask& sub : m_sub) {
            જો (!sub.isOnes()) પાછા ખોટું;
        }
        પાછા સાચું;
    }
    શૂન્ય setZero() {
        જો (m_dtype.isPacked()) {
            m_અંક.setAllBits0();
        } નહીંતર {
            માટે (BitMask& sub : m_sub) sub.setZero();
        }
    }
    શૂન્ય setOnes() {
        જો (m_dtype.isPacked()) {
            m_અંક.setAllBits1();
        } નહીંતર {
            માટે (BitMask& sub : m_sub) sub.setOnes();
        }
    }

    તાર્કિક operator==(અચલ BitMask& other) અચલ {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        જો (m_dtype.isPacked()) પાછા m_અંક.isCaseEq(other.m_અંક);
        પાછા m_sub == other.m_sub;
    }
    તાર્કિક operator!=(અચલ BitMask& other) અચલ { પાછા !(*આ == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    તાર્કિક isContractionOf(અચલ BitMask& that) અચલ {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        જો (m_dtype.isPacked()) {
            અચલ size_t શબ્દો = VL_શબ્દો_I(m_dtype.size());
            માટે (size_t i = 0; i < શબ્દો; ++i) {
                જો (~m_અંક.edataWord(i) & that.m_અંક.edataWord(i)) પાછા સાચું;
            }
            પાછા ખોટું;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        માટે (size_t i = 0; i < m_sub.size(); ++i) {
            જો (m_sub[i].isContractionOf(that.m_sub[i])) પાછા સાચું;
        }
        પાછા ખોટું;
    }

    std::string toString() અચલ {
        જો (m_dtype.isPacked()) પાછા "0x" + m_અંક.displayed(m_અંક.fileline(), "%x");
        std::string result;
        result += "{";
        માટે (અચલ BitMask& sub : m_sub) {
            જો (&sub != &m_sub.front()) result += ", ";
            result += sub.toString();
        }
        result += "}";
        પાછા result;
    }
};

વર્ગ IndependentBits final : જાહેર DfgVisitor {
    // TYPES
    સંરચના VertexState final {
        size_t m_rpo_સંખ્યા = 0;  // Reverse Postorder Number of vertex
        તાર્કિક m_isOnWorkList = ખોટું;  // Whether the vertex is on the work list
    };

    // STATE
    અચલ SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<અચલ DfgVertex*, BitMask> m_vtxp2Mask;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_workQueue;
    std::unordered_map<અચલ DfgVertex*, VertexState> m_vtxp2State;  // State of each vertex

#ifdef VL_DEBUG
    std::ofstream m_લીટી_Coverage_File;  // Line coverage file, just for testing
#endif

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    BitMask& mask(અચલ DfgVertex& vtx) {
        // Look up (or create) mask for 'vtx'
        પાછા m_vtxp2Mask
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .first->second;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define MASK(vtxp) \
    ([આ](અચલ DfgVertex& vtx) -> BitMask& { \
        જો (VL_UNLIKELY(m_લીટી_Coverage_File.is_open())) m_લીટી_Coverage_File << __LINE__ << '\n'; \
        પાછા mask(vtx); \
    }(*vtxp))
#નહીંતર
#define MASK(vtxp) mask(*vtxp)
#endif

    // Clear all bits at or below the most significant clear bit
    શૂન્ય floodTowardsLsb(V3_સંખ્યા& અંક) {
        માટે (પૂર્ણાંક i = અંક.width() - 1; i >= 0; --i) {
            જો (અંક.bitIs1(i)) ચાલુ;
            અંક.opSetRange(0, i + 1, '0');
            તોડો;
        }
    }

    // Clear all bits at or above the least significant clear bit
    શૂન્ય floodTowardsMsb(V3_સંખ્યા& અંક) {
        માટે (પૂર્ણાંક i = 0; i < અંક.width(); ++i) {
            જો (અંક.bitIs1(i)) ચાલુ;
            અંક.opSetRange(i, અંક.width() - i, '0');
            તોડો;
        }
    }

    શૂન્ય propagateFromDriver(BitMask& m, અચલ DfgVertex* srcp) {
        // If there is no driver, we are done
        જો (!srcp) પાછા;
        // If it is driven by a splice, we need to combine the masks of the drivers
        જો (અચલ DfgSplicePacked* અચલ splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](અચલ DfgVertex& src, uint32_t lo) {
                m.અંક().opSelInto(MASK(&src).અંક(), lo, src.width());
                પાછા ખોટું;
            });
            પાછા;
        }
        જો (અચલ DfgSpliceArray* અચલ splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](અચલ DfgVertex& src, uint32_t lo) {
                જો (અચલ DfgUnitArray* અચલ uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = MASK(uap->srcp());
                } નહીંતર {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                પાછા ખોટું;
            });
            પાછા;
        }
        // Otherwise, we just use the mask of the single driver
        m = MASK(srcp);
    }

    // VISITORS
    શૂન્ય visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    શૂન્ય visit(DfgVertexVar* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        જો (vtxp->isVolatile()) પાછા;

        BitMask& m = MASK(vtxp);
        DfgVertex* અચલ srcp = vtxp->srcp();
        DfgVertex* અચલ defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        જો (defaultp) m = MASK(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    શૂન્ય visit(DfgVertexSplice* vtxp) override {
        propagateFromDriver(MASK(vtxp), vtxp);  // Needed to continue traversal
    }

    શૂન્ય visit(DfgUnitArray* vtxp) override { MASK(vtxp).sub().at(0) = MASK(vtxp->srcp()); }

    શૂન્ય visit(DfgArraySel* vtxp) override {
        DfgVertex* અચલ fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        જો (અચલ DfgConst* અચલ idxp = vtxp->bitp()->cast<DfgConst>()) {
            MASK(vtxp) = MASK(vtxp->fromp()).sub().at(idxp->toSizeT());
            પાછા;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        જો (MASK(vtxp->bitp()).isOnes() && MASK(fromp).isOnes()) MASK(vtxp).setOnes();
    }

    શૂન્ય visit(DfgConcat* vtxp) override {
        અચલ DfgVertex* અચલ rhsp = vtxp->rhsp();
        અચલ DfgVertex* અચલ lhsp = vtxp->lhsp();
        V3_સંખ્યા& m = MASK(vtxp).અંક();
        m.opSelInto(MASK(rhsp).અંક(), 0, rhsp->width());
        m.opSelInto(MASK(lhsp).અંક(), rhsp->width(), lhsp->width());
    }

    શૂન્ય visit(DfgRep* vtxp) override {
        અચલ uint32_t ગણતરી = vtxp->ગણતરી();
        અચલ DfgVertex* અચલ srcp = vtxp->srcp();
        અચલ uint32_t sWidth = srcp->width();
        V3_સંખ્યા& vMask = MASK(vtxp).અંક();
        V3_સંખ્યા& sMask = MASK(srcp).અંક();
        માટે (uint32_t i = 0; i < ગણતરી; ++i) vMask.opSelInto(sMask, i * sWidth, sWidth);
    }

    શૂન્ય visit(DfgSel* vtxp) override {
        અચલ uint32_t lsb = vtxp->lsb();
        અચલ uint32_t msb = lsb + vtxp->width() - 1;
        MASK(vtxp).અંક().opSel(MASK(vtxp->fromp()).અંક(), msb, lsb);
    }

    શૂન્ય visit(DfgExtend* vtxp) override {
        અચલ DfgVertex* અચલ srcp = vtxp->srcp();
        અચલ uint32_t sWidth = srcp->width();
        V3_સંખ્યા& s = MASK(srcp).અંક();
        V3_સંખ્યા& m = MASK(vtxp).અંક();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, '1');
    }
    શૂન્ય visit(DfgExtendS* vtxp) override {
        અચલ DfgVertex* અચલ srcp = vtxp->srcp();
        અચલ uint32_t sWidth = srcp->width();
        V3_સંખ્યા& s = MASK(srcp).અંક();
        V3_સંખ્યા& m = MASK(vtxp).અંક();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, s.bitIs0(sWidth - 1) ? '0' : '1');
    }

    શૂન્ય visit(DfgNot* vtxp) override {  //
        MASK(vtxp) = MASK(vtxp->srcp());
    }

    શૂન્ય visit(DfgAnd* vtxp) override {  //
        MASK(vtxp).અંક().opAnd(MASK(vtxp->lhsp()).અંક(), MASK(vtxp->rhsp()).અંક());
    }
    શૂન્ય visit(DfgOr* vtxp) override {  //
        MASK(vtxp).અંક().opAnd(MASK(vtxp->lhsp()).અંક(), MASK(vtxp->rhsp()).અંક());
    }
    શૂન્ય visit(DfgXor* vtxp) override {  //
        MASK(vtxp).અંક().opAnd(MASK(vtxp->lhsp()).અંક(), MASK(vtxp->rhsp()).અંક());
    }

    શૂન્ય visit(DfgAdd* vtxp) override {
        V3_સંખ્યા& m = MASK(vtxp).અંક();
        m.opAnd(MASK(vtxp->lhsp()).અંક(), MASK(vtxp->rhsp()).અંક());
        floodTowardsMsb(m);
    }
    શૂન્ય visit(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_સંખ્યા& m = MASK(vtxp).અંક();
        m.opAnd(MASK(vtxp->lhsp()).અંક(), MASK(vtxp->rhsp()).અંક());
        floodTowardsMsb(m);
    }

    શૂન્ય visit(DfgRedAnd* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }
    શૂન્ય visit(DfgRedOr* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }
    શૂન્ય visit(DfgRedXor* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }

    શૂન્ય visit(DfgEq* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }
    શૂન્ય visit(DfgNeq* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }
    શૂન્ય visit(DfgLt* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }
    શૂન્ય visit(DfgLte* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }
    શૂન્ય visit(DfgGt* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }
    શૂન્ય visit(DfgGte* vtxp) override {
        અચલ તાર્કિક independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        જો (independent) MASK(vtxp).setOnes();
    }

    શૂન્ય visit(DfgShiftRS* vtxp) override {
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        અચલ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        જો (DfgConst* અચલ rConstp = rhsp->cast<DfgConst>()) {
            અચલ uint32_t shiftAmount = rConstp->toU32();
            જો (shiftAmount >= width) {
                જો (MASK(lhsp).અંક().bitIs1(width - 1)) { MASK(vtxp).setOnes(); }
            } નહીંતર {
                V3_સંખ્યા& m = MASK(vtxp).અંક();
                m.opShiftRS(MASK(lhsp).અંક(), rConstp->અંક(), width);
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            પાછા;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        જો (MASK(rhsp).isOnes()) {
            V3_સંખ્યા& m = MASK(vtxp).અંક();
            m = MASK(lhsp).અંક();
            floodTowardsLsb(m);
        }
    }

    શૂન્ય visit(DfgShiftR* vtxp) override {
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        અચલ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        જો (DfgConst* અચલ rConstp = rhsp->cast<DfgConst>()) {
            અચલ uint32_t shiftAmount = rConstp->toU32();
            જો (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } નહીંતર {
                V3_સંખ્યા& m = MASK(vtxp).અંક();
                m.opShiftR(MASK(lhsp).અંક(), rConstp->અંક());
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            પાછા;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        જો (MASK(rhsp).isOnes()) {
            V3_સંખ્યા& m = MASK(vtxp).અંક();
            m = MASK(lhsp).અંક();
            floodTowardsLsb(m);
        }
    }

    શૂન્ય visit(DfgShiftL* vtxp) override {
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        અચલ uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        જો (DfgConst* અચલ rConstp = rhsp->cast<DfgConst>()) {
            અચલ uint32_t shiftAmount = rConstp->toU32();
            જો (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } નહીંતર {
                V3_સંખ્યા& m = MASK(vtxp).અંક();
                m.opShiftL(MASK(lhsp).અંક(), rConstp->અંક());
                m.opSetRange(0, shiftAmount, '1');
            }
            પાછા;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        જો (MASK(rhsp).isOnes()) {
            V3_સંખ્યા& m = MASK(vtxp).અંક();
            m = MASK(lhsp).અંક();
            floodTowardsMsb(m);
        }
    }

    શૂન્ય visit(DfgCond* vtxp) override {
        જો (MASK(vtxp->condp()).isOnes()) {
            MASK(vtxp).અંક().opAnd(MASK(vtxp->thenp()).અંક(), MASK(vtxp->elsep()).અંક());
        }
    }

    શૂન્ય visit(DfgMatchMasked* vtxp) override {
        જો (MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->matchp()).isOnes()) { MASK(vtxp).setOnes(); }
    }

#undef MASK

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    શૂન્ય enqueueSinks(DfgVertex& vtx) {
        vtx.foreachSink([&](DfgVertex& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            જો (!m_sccInfo.get(sink)) પાછા ખોટું;
            // Otherwise just enqueue it
            VertexState& state = m_vtxp2State.at(&sink);
            જો (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_workQueue.ધકેલો(state.m_rpo_સંખ્યા);
            }
            state.m_isOnWorkList = સાચું;
            પાછા ખોટું;
        });
    };

    શૂન્ય dfsPostOrder(std::unordered_set<DfgVertex*>& visited,  //
                      std::vector<DfgVertex*>& postOrderEnumeration,  //
                      DfgVertex& vtx) {
        // Mark visited, skip if already visited
        જો (!visited.ઉમેરો(&vtx).second) પાછા;
        // Visit un-visited successors
        vtx.foreachSink([&](DfgVertex& snk) {
            dfsPostOrder(visited, postOrderEnumeration, snk);
            પાછા ખોટું;
        });
        // Add to post order enumeration
        postOrderEnumeration.emplace_back(&vtx);
    };

    IndependentBits(DfgGraph& dfg, અચલ SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#ifdef VL_DEBUG
        જો (v3Global.opt.debugCheck()) {
            m_લીટી_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::app);
        }
#endif

        // Compute Reverse-Postorder enumeration of vertices
        std::vector<DfgVertex*> rpoEnumeration;
        std::unordered_set<DfgVertex*> visited;
        dfg.forEachVertex([&](DfgVertex& vtx) {
            જો (!vtx.nInputs()) dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        dfg.forEachVertex([&](DfgVertex& vtx) {  //
            dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        std::reverse(rpoEnumeration.begin(), rpoEnumeration.end());
        UASSERT(rpoEnumeration.size() == dfg.size(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        માટે (size_t i = 0; i < rpoEnumeration.size(); ++i) {
            VertexState& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_સંખ્યા = i;
            state.m_isOnWorkList = ખોટું;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        માટે (DfgVertex* અચલ vtxp : rpoEnumeration) {
            જો (m_sccInfo.get(*vtxp)) ચાલુ;
            mask(*vtxp).setOnes();
        }
        માટે (DfgVertex* અચલ vtxp : rpoEnumeration) {
            જો (!m_sccInfo.get(*vtxp)) ચાલુ;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << mask(*vtxp).toString());
            જો (!mask(*vtxp).isZero()) enqueueSinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        જ્યારે (!m_workQueue.empty()) {
            // Grab next item
            DfgVertex* અચલ currp = rpoEnumeration[m_workQueue.ટોચ()];
            m_workQueue.કાઢો();
            m_vtxp2State.at(currp).m_isOnWorkList = ખોટું;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            અચલ BitMask& currMask = mask(*currp);
            // Remember current mask so we can check if it changed
            અચલ BitMask prevMask = currMask;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            જો (prevMask != currMask) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prevMask.toString()
                             << "\n      to: " << currMask.toString());
                enqueueSinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                જો (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!currMask.isContractionOf(prevMask), currp,
                                "Mask should only expand");
                }
            }
        }
    }

જાહેર:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    સ્થિર std::unordered_map<અચલ DfgVertex*, BitMask> apply(DfgGraph& dfg,
                                                               અચલ SccInfo& sccInfo) {
        પાછા std::move(IndependentBits{dfg, sccInfo}.m_vtxp2Mask);
    }
};

વર્ગ FixUp final {
    DfgGraph& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    અચલ std::unordered_map<અચલ DfgVertex*, BitMask> m_independentBits
        = IndependentBits::apply(m_dfg, m_sccInfo);
    size_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    સ્થિર BitMask computeUsedBits(DfgVertex& vtx) {
        BitMask result{vtx};
        vtx.foreachSink([&result](DfgVertex& sink) {
            // If used via a Sel, mark the selected bits used
            જો (અચલ DfgSel* અચલ selp = sink.cast<DfgSel>()) {
                result.અંક().opSetRange(selp->lsb(), selp->width(), '1');
                પાછા ખોટું;
            }
            // Used without a Sel, so all bits are used
            result.setOnes();
            પાછા સાચું;
        });
        પાછા result;
    }

    સ્થિર std::string debugStr(અચલ DfgVertex& vtx) {  // LCOV_EXCL_START
        જો (અચલ DfgArraySel* અચલ aselp = vtx.cast<DfgArraySel>()) {
            અચલ size_t i = aselp->bitp()->as<DfgConst>()->toSizeT();
            પાછા debugStr(*aselp->fromp()) + "[" + std::to_string(i) + "]";
        }
        જો (અચલ DfgVertexVar* અચલ varp = vtx.cast<DfgVertexVar>()) {
            પાછા varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    શૂન્ય gatherTerms(std::vector<DfgVertex*>& termps, DfgVertex& vtx, અચલ V3_સંખ્યા& indpBits,
                     અચલ uint32_t hi, અચલ uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        તાર્કિક isIndependent = indpBits.bitIs1(lo);
        uint32_t lsb = lo;
        માટે (uint32_t msb = lo; msb <= hi; ++msb) {
            અચલ તાર્કિક endRange = msb == hi || isIndependent != indpBits.bitIs1(msb + 1);
            જો (!endRange) ચાલુ;
            અચલ uint32_t width = msb - lsb + 1;
            // Compute the term to use for this range
            જો (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_back(m_traceDriver.apply(vtx, lsb, width));
            } નહીંતર {
                // If dependent, fall back on using the part of the variable
                DfgSel* અચલ selp = નવું DfgSel{m_dfg, vtx.fileline(), DfgDataType::packed(width)};
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

    શૂન્ય fixUpPacked(DfgVertex& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        અચલ BitMask& indpBits = m_independentBits.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indpBits.toString());
        // Can't do anything if all bits are dependent
        જો (indpBits.isZero()) પાછા;

        // Figure out which bits of 'vtxp' are used
        અચલ BitMask usedBits = computeUsedBits(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << usedBits.toString());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_સંખ્યા usedAndIndependent{vtx.fileline(), static_cast<પૂર્ણાંક>(vtx.width()), 0};
        usedAndIndependent.opAnd(usedBits.અંક(), indpBits.અંક());
        જો (usedAndIndependent.isEqZero()) પાછા;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::vector<DfgVertex*> termps;

        // Iterate through consecutive used/unused ranges
        File_લીટી* અચલ flp = vtx.fileline();
        અચલ uint32_t width = vtx.width();
        તાર્કિક isUsed = usedBits.અંક().bitIs1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        માટે (uint32_t msb = 0; msb < width; ++msb) {
            અચલ તાર્કિક endRange = msb == width - 1 || isUsed != usedBits.અંક().bitIs1(msb + 1);
            જો (!endRange) ચાલુ;
            જો (isUsed) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indpBits.અંક(), msb, lsb);
            } નહીંતર {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* અચલ constp = નવું DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.add(*constp, 0);
                termps.emplace_back(constp);
            }
            // Next iteration
            isUsed = !isUsed;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        DfgVertex* replacementp = termps.front();
        અચલ uint64_t vComp = m_sccInfo.get(vtx);
        માટે (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* અચલ termp = termps[i];
            અચલ uint32_t catWidth = replacementp->width() + termp->width();
            DfgConcat* અચલ catp = નવું DfgConcat{m_dfg, flp, DfgDataType::packed(catWidth)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            અચલ uint64_t tComp = m_sccInfo.get(*termp);
            અચલ uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.add(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        માટે (DfgVertex* અચલ termp : termps) {
            જો (DfgSel* અચલ selp = termp->cast<DfgSel>()) {
                જો (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        જો (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    શૂન્ય મુખ્ય(DfgVertexVar& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        જો (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fixUpPacked(var);
        } નહીંતર જો (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            અચલ uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](DfgVertex& sink) {
                // Ignore if sink is not part of same cycle
                જો (m_sccInfo.get(sink) != component) પાછા ખોટું;
                // Only handle ArraySels with constant index
                DfgArraySel* અચલ aselp = sink.cast<DfgArraySel>();
                જો (!aselp) પાછા ખોટું;
                જો (!aselp->bitp()->is<DfgConst>()) પાછા ખોટું;
                // Fix up the word
                fixUpPacked(*aselp);
                પાછા ખોટું;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    FixUp(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

જાહેર:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    સ્થિર size_t apply(DfgGraph& dfg, SccInfo& sccInfo) {
        જો (!sccInfo.isCyclic()) પાછા 0;

        FixUp fixUp{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        માટે (DfgVertexVar& vtx : dfg.varVertices()) {
            જો (!sccInfo.get(vtx)) ચાલુ;
            fixUp.મુખ્ય(vtx);
            જો (!sccInfo.isCyclic()) તોડો;
        }
        // Return the number of improvements made
        પાછા fixUp.m_nImprovements;
    }
};

શૂન્ય breakCycles(DfgGraph& dfg, V3DfgBreakCyclesContext& ctx) {
    // Shorthand for dumping graph at given dump level
    અચલ auto dump = [&](પૂર્ણાંક level, અચલ DfgGraph& dfg, અચલ std::string& name) {
        જો (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    size_t nImprovements = 0;
    size_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    કરો {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        અચલ size_t nFixed = FixUp::apply(dfg, sccInfo);
        જો (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        જો (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        જો (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            પાછા;
        }
    } જ્યારે (nImprovements != prevNImprovements);

    // Debug dump
    જો (dumpDfgLevel() >= 9) {
        અચલ SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](અચલ DfgVertex& vtx) {
            પાછા sccInfo.get(vtx);  //
        });
    }

    // Accounting
    જો (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } નહીંતર {
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
    માટે (DfgVertexVar& vtx : dfg.varVertices()) {
        જો (!vtx.tmpForp()) varps.ધકેલો_back(&vtx);
    }
    std::stable_partition(varps.begin(), varps.end(), [](અચલ DfgVertexVar* varp) {
        પાછા varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    માટે (DfgVertexVar* અચલ varp : varps) {
        // Stop as soon as it becomes acyclic
        જો (!sccInfo.isCyclic()) તોડો;

        // Ignore if variable is not part of a cycle
        જો (!sccInfo.get(*varp)) ચાલુ;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* અચલ prevp = નવું DfgPrev{dfg, varp->vscp()};
        sccInfo.add(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    જો (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
શૂન્ય V3DfgPasses::breakCycles(DfgGraph& dfg, V3DfgContext& ctx) {
    V3DfgBreakCycles::breakCycles(dfg, ctx.m_breakCyclesContext);
    જો (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::removeUnused(dfg);
}
