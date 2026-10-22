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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Hash.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <fstream>
#அந்தர்பாவ <queue>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

நாமாகாஶ V3DfgBreakCycles {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
வகுப்பு SccInfo final {
    DfgGraph& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<மாறிலி DfgVertex*, uint64_t> m_vtx2Scc;
    size_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

பொது:
    SccInfo(DfgGraph& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.forEachVertex([&](மாறிலி DfgVertex& vtx) {
            மாறிலி uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            எனில் (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    தருக்க isCyclic() மாறிலி { திரும்பு m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(மாறிலி DfgVertex& vtx) மாறிலி { திரும்பு m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    வெறுமை add(மாறிலி DfgVertex& vtx, uint64_t sccIdx) {
        மாறிலி தருக்க newEntry = m_vtx2Scc.emplace(&vtx, sccIdx).second;
        UASSERT_OBJ(newEntry, &vtx, "Vertex already inserted");
        எனில் (sccIdx) ++m_nCyclicVertices;
    }

    தருக்க stillCyclicFwd(மாறிலி DfgVertex& vtx) மாறிலி {
        // If it only reads non-cyclic vertices, it has also become acyclic
        திரும்பு vtx.foreachSource([&](மாறிலி DfgVertex& src) {  //
            திரும்பு m_vtx2Scc.at(&src);
        });
    }

    தருக்க stillCyclicBwd(மாறிலி DfgVertex& vtx) மாறிலி {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        திரும்பு vtx.foreachSink([&](மாறிலி DfgVertex& dst) {  //
            திரும்பு m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    வெறுமை updateAcyclic(மாறிலி DfgVertex& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](மாறிலி DfgVertex& dst) {
            // Sink was known to be acyclic, stop
            எனில் (!m_vtx2Scc.at(&dst)) திரும்பு பொய்;
            எனில் (!stillCyclicFwd(dst)) updateAcyclic(dst);
            திரும்பு பொய்;
        });
        // Propagate the update through the graph backward
        vtx.foreachSource([&](மாறிலி DfgVertex& src) {
            // Source was known to be acyclic, stop
            எனில் (!m_vtx2Scc.at(&src)) திரும்பு பொய்;
            எனில் (!stillCyclicBwd(src)) updateAcyclic(src);
            திரும்பு பொய்;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    வெறுமை updateReplacement(மாறிலி DfgVertex& vtx, DfgVertex& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        எனில் (!get(replacement)) {
            replacement.foreachSink([&](DfgVertex& dst) {
                எனில் (!get(dst)) திரும்பு பொய்;
                எனில் (!stillCyclicFwd(dst)) updateAcyclic(dst);
                திரும்பு பொய்;
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
    வெறுமை validate() மாறிலி {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.forEachVertex([&](மாறிலி DfgVertex& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

வகுப்பு TraceDriver final : பொது DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    கட்டமைப்பு Cache_சாவி final {
        DfgVertex* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_சாவி() = delete;
        Cache_சாவி(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        கட்டமைப்பு Hash final {
            size_t operator()(மாறிலி Cache_சாவி& item) மாறிலி {
                // cppcheck-suppress unreadVariable
                V3Hash hash{item.m_vtxp};
                hash += item.m_lsb;
                hash += item.m_msb;
                திரும்பு hash.மதிப்பு();
            }
        };

        கட்டமைப்பு Equal final {
            தருக்க operator()(மாறிலி Cache_சாவி& a, மாறிலி Cache_சாவி& b) மாறிலி {
                திரும்பு a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
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
    std::unordered_map<Cache_சாவி, DfgVertex*, Cache_சாவி::Hash, Cache_சாவி::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_சாவி, DfgVertex*, Cache_சாவி::Hash, Cache_சாவி::Equal> m_boundaryCache;

#ifdef VL_DEBUG
    std::ofstream m_வரி_Coverage_File;  // Line coverage file, just for testing
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
    Vertex* make(மாறிலி DfgVertex* refp, uint32_t width) {
        static_assert(std::is_base_of<DfgVertex, Vertex>::மதிப்பு  //
                          && !std::is_base_of<DfgVertexVar, Vertex>::மதிப்பு,
                      "Should only make operation vertices and constants");

        எனில் VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, Vertex>::மதிப்பு) {
            DfgConst* மாறிலி vtxp = புதிய DfgConst{m_dfg, refp->fileline(), width, 0};
            m_sccInfo.add(*vtxp, 0);
            திரும்பு reinterpret_cast<Vertex*>(vtxp);
        } இல்லையெனில் {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            using Vtx = typename std::conditional<std::is_same<DfgConst, Vertex>::மதிப்பு, DfgSel,
                                                  Vertex>::type;
            Vtx* மாறிலி vtxp = புதிய Vtx{m_dfg, refp->fileline(), DfgDataType::packed(width)};
            m_sccInfo.add(*vtxp, 0);
            திரும்பு reinterpret_cast<Vertex*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    DfgVertexVar* createTmp(மாறிலி எழுத்து* prefix, DfgVertex* vtxp) {
        File_வரி* மாறிலி flp = vtxp->fileline();
        DfgVertex::ScopeCache scopeCache;
        AstScope* மாறிலி scopep = vtxp->scopep(scopeCache);
        DfgVertexVar* மாறிலி varp = m_dfg.makeNewVar(flp, prefix, vtxp->dtype(), scopep);
        varp->vscp()->varp()->isInternal(உண்மை);
        varp->tmpForp(varp->vscp());
        m_sccInfo.add(*varp, 0);
        திரும்பு varp;
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
        DfgVertex* மாறிலி startVtxp = vtxp;
        மாறிலி uint32_t startMsb = msb;
        மாறிலி uint32_t startLsb = lsb;
        மாறிலி size_t pendingBegin = m_pendingps.size();
        DfgVertex* resp = nullptr;

        போது (உண்மை) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->size() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            எனில் (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                DfgVertex*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(nullptr))
                          .first->second;
                எனில் (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    எனில் (DfgVertexSplice* மாறிலி splicep = respr->cast<DfgVertexSplice>()) {
                        DfgVertexVar* மாறிலி tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    எனில் (msb != respr->width() - 1 || lsb != 0) {
                        DfgSel* மாறிலி selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                நிறுத்து;
            }

            // Normal cache for intra-component traces
            DfgVertex*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(nullptr))
                                    .first->second;

            // Trace the vertex
            எனில் (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } இல்லையெனில் {
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
                எனில் (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.தள்ளு_back(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    தொடர்;
                }
                respr = m_resp;
            }
            resp = respr;
            நிறுத்து;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, startVtxp, "Tracing driver failed for " << startVtxp->typeName());
        UASSERT_OBJ(resp->width() == (startMsb - startLsb + 1), startVtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        க்கு (size_t i = pendingBegin; i < m_pendingps.size(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        திரும்பு resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    DfgVertex* traceSameIdx(DfgVertex* மாறிலி vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = nullptr;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->width() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        திரும்பு m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    DfgVertex* trace_தள்ளு_Idx(DfgVertex* மாறிலி vtxp, மாறிலி uint32_t idx) {
        மாறிலி size_t nIdxs = m_idxs.size();
        m_idxs.தள்ளு_back(idx);
        DfgVertex* மாறிலி resp = traceSameIdx(vtxp);
        m_idxs.எடு_back();
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        திரும்பு resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    DfgVertex* trace_எடு_Idx(DfgVertex* மாறிலி vtxp) {
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        மாறிலி size_t nIdxs = m_idxs.size();
        மாறிலி uint32_t idx = m_idxs.back();
        m_idxs.எடு_back();
        DfgVertex* மாறிலி resp = m_idxs.empty() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.தள்ளு_back(idx);
        UASSERT_OBJ(m_idxs.size() == nIdxs, vtxp, "Index stack size mismatch");
        திரும்பு resp;
    }

    template <typename Vertex>
    Vertex* trace_Bitwise_இருமம்(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_இருமம், Vertex>::மதிப்பு,
                      "Should only call on DfgVertexBinary");
        Vertex* மாறிலி resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        திரும்பு resp;
    }

    template <typename Vertex>
    DfgVertex* traceAddSub(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_இருமம், Vertex>::மதிப்பு,
                      "Should only call on DfgVertexBinary");
        Vertex* மாறிலி opp = make<Vertex>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* மாறிலி selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        திரும்பு selp;
    }

    template <typename Vertex>
    Vertex* traceReduction(Vertex* vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::மதிப்பு,
                      "Should only call on DfgVertexUnary");
        Vertex* மாறிலி resp = make<Vertex>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->width() - 1, 0));
        திரும்பு resp;
    }

    template <typename Vertex>
    Vertex* traceCmp(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_இருமம், Vertex>::மதிப்பு,
                      "Should only call on DfgVertexBinary");
        Vertex* மாறிலி resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        திரும்பு resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define TRACE_RESULT_COVER() \
    செய் { \
        எனில் (VL_UNLIKELY(m_வரி_Coverage_File.is_open())) m_வரி_Coverage_File << __LINE__ << '\n'; \
    } போது (பொய்)
#இல்லையெனில்
#define TRACE_RESULT_COVER() \
    செய் { \
    } போது (பொய்)
#endif

#define RETURN_RESULT(vtxp) \
    செய் { \
        m_resp = (vtxp); \
        TRACE_RESULT_COVER(); \
        திரும்பு; \
    } போது (பொய்)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#define RETURN_RESULT_TAIL(srcp, smsb, slsb) \
    செய் { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        DfgVertex* மாறிலி tailp = (srcp); \
        மாறிலி uint32_t tailMsb = (smsb); \
        மாறிலி uint32_t tailLsb = (slsb); \
        m_tailp = tailp; \
        m_msb = tailMsb; \
        m_lsb = tailLsb; \
        TRACE_RESULT_COVER(); \
        திரும்பு; \
    } போது (பொய்)

    // VISITORS
    வெறுமை visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    வெறுமை visit(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        DfgVertex* defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        கட்டமைப்பு Driver final {
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
        தருக்க tryWholeDefault = defaultp;
        DfgVertex* coverp = nullptr;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lsb) {
            மாறிலி uint32_t msb = lsb + src.width() - 1;
            drivers.emplace_back(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            எனில் (m_msb >= lsb && msb >= m_lsb) tryWholeDefault = பொய்;
            // If it does not cover the whole searched bit range, move on
            எனில் (m_lsb < lsb || msb < m_msb) திரும்பு பொய்;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            திரும்பு உண்மை;
        });

        // Trace the driver covering the whole searched range, if there is one
        எனில் (coverp) RETURN_RESULT_TAIL(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        எனில் (tryWholeDefault) RETURN_RESULT_TAIL(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::sort(drivers.begin(), drivers.end(), [](மாறிலி Driver& a, மாறிலி Driver& b) {  //
            திரும்பு a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::vector<DfgVertex*> termps;
        uint32_t lsb = m_lsb;
        க்கு (மாறிலி Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            எனில் (lsb > driver.m_msb) தொடர்;
            // Driver is above the searched MSB, done
            எனில் (driver.m_lsb > m_msb) நிறுத்து;
            // Gap below this driver, trace default to fill it
            எனில் (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_back(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            மாறிலி uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_back(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        எனில் (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_back(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.size() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        DfgVertex* resp = termps.front();
        க்கு (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* மாறிலி termp = termps[i];
            DfgConcat* மாறிலி catp = make<DfgConcat>(termp, resp->width() + termp->width());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_RESULT(resp);
    }

    வெறுமை visit(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        DfgVertex* மாறிலி defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        மாறிலி uint32_t idx = m_idxs.back();
        எனில் (DfgVertex* மாறிலி driverp = vtxp->driverAt(idx)) {
            DfgVertex* மாறிலி srcp = driverp->as<DfgUnitArray>()->srcp();
            எனில் (srcp->is<DfgVertexSplice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_RESULT(trace_எடு_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_RESULT(traceSameIdx(defaultp));
    }

    வெறுமை visit(DfgVertexVar* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        DfgVertex* மாறிலி srcp = vtxp->srcp();
        DfgVertex* மாறிலி defaultp = vtxp->defaultp();
        DfgVertex* மாறிலி drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        எனில் (srcp && srcp->is<DfgVertexSplice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_RESULT(m_idxs.empty() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    வெறுமை visit(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        எனில் (மாறிலி DfgConst* மாறிலி idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_RESULT(trace_தள்ளு_Idx(vtxp->fromp(), idxp->toU32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.empty(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* மாறிலி resp = make<DfgArraySel>(vtxp, vtxp->width());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->width() - 1, 0));
        DfgSel* மாறிலி selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    வெறுமை visit(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.back() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_RESULT(trace_எடு_Idx(vtxp->srcp()));
    }

    வெறுமை visit(DfgConcat* vtxp) override {
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        மாறிலி uint32_t rWidth = rhsp->width();
        // If the traced bits are wholly in the RHS
        எனில் (rWidth > m_msb) RETURN_RESULT_TAIL(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        எனில் (m_lsb >= rWidth) RETURN_RESULT_TAIL(lhsp, m_msb - rWidth, m_lsb - rWidth);
        // The traced bit spans both sides, trace both
        DfgConcat* மாறிலி resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, rWidth - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - rWidth, 0));
        RETURN_RESULT(resp);
    }

    வெறுமை visit(DfgRep* vtxp) override {
        DfgVertex* மாறிலி srcp = vtxp->srcp();
        மாறிலி uint32_t sWidth = srcp->width();
        // If we need more bits than the source, then we need the whole source
        எனில் (m_msb - m_lsb + 1 > sWidth) {
            DfgRep* மாறிலி repp = make<DfgRep>(vtxp, vtxp->width());
            repp->srcp(trace(srcp, sWidth - 1, 0));
            DfgSel* மாறிலி resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_RESULT(resp);
        }
        // If the requested bits are within the same repliacted word
        எனில் (m_msb / sWidth == m_lsb / sWidth) {
            RETURN_RESULT_TAIL(srcp, m_msb % sWidth, m_lsb % sWidth);
        }
        // The requested bits span two replicated words
        DfgConcat* மாறிலி resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, sWidth - 1, m_lsb % sWidth));
        resp->lhsp(trace(srcp, m_msb % sWidth, 0));
        RETURN_RESULT(resp);
    }

    வெறுமை visit(DfgExtend* vtxp) override {
        DfgVertex* மாறிலி srcp = vtxp->srcp();
        மாறிலி uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        எனில் (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        எனில் (m_lsb >= sWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* மாறிலி resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    வெறுமை visit(DfgExtendS* vtxp) override {
        DfgVertex* மாறிலி srcp = vtxp->srcp();
        மாறிலி uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        எனில் (sWidth > m_msb) RETURN_RESULT_TAIL(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        எனில் (m_lsb >= sWidth) {
            எனில் (m_msb == m_lsb) RETURN_RESULT_TAIL(srcp, sWidth - 1, sWidth - 1);
            DfgExtendS* மாறிலி resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, sWidth - 1, sWidth - 1));
            RETURN_RESULT(resp);
        }
        // The traced bits span both sides
        DfgExtendS* மாறிலி resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_RESULT(resp);
    }

    வெறுமை visit(DfgSel* vtxp) override {
        மாறிலி uint32_t lsb = vtxp->lsb();
        RETURN_RESULT_TAIL(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    வெறுமை visit(DfgNot* vtxp) override {
        DfgNot* மாறிலி resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    வெறுமை visit(DfgAnd* vtxp) override { RETURN_RESULT(trace_Bitwise_இருமம்(vtxp)); }
    வெறுமை visit(DfgOr* vtxp) override { RETURN_RESULT(trace_Bitwise_இருமம்(vtxp)); }
    வெறுமை visit(DfgXor* vtxp) override { RETURN_RESULT(trace_Bitwise_இருமம்(vtxp)); }
    வெறுமை visit(DfgAdd* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    வெறுமை visit(DfgSub* vtxp) override { RETURN_RESULT(traceAddSub(vtxp)); }
    வெறுமை visit(DfgRedAnd* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    வெறுமை visit(DfgRedOr* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    வெறுமை visit(DfgRedXor* vtxp) override { RETURN_RESULT(traceReduction(vtxp)); }
    வெறுமை visit(DfgEq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    வெறுமை visit(DfgNeq* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    வெறுமை visit(DfgLt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    வெறுமை visit(DfgLte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    வெறுமை visit(DfgGt* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }
    வெறுமை visit(DfgGte* vtxp) override { RETURN_RESULT(traceCmp(vtxp)); }

    வெறுமை visit(DfgShiftRS* vtxp) override {
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        எனில் (மாறிலி DfgConst* மாறிலி rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            மாறிலி uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            மாறிலி uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            எனில் (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            எனில் (m_lsb >= lowerWidth) {
                DfgExtendS* மாறிலி resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->width() - 1, lhsp->width() - 1));
                RETURN_RESULT(resp);
            }
            // The traced bits span both sides
            DfgExtendS* மாறிலி resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftRS* மாறிலி shiftrsp = make<DfgShiftRS>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        எனில் (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrsp);
        DfgSel* மாறிலி selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    வெறுமை visit(DfgShiftR* vtxp) override {
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        எனில் (மாறிலி DfgConst* மாறிலி rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            மாறிலி uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            மாறிலி uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            எனில் (lowerWidth > m_msb) {
                RETURN_RESULT_TAIL(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            எனில் (m_lsb >= lowerWidth) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* மாறிலி resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftR* மாறிலி shiftrp = make<DfgShiftR>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        எனில் (m_msb == vtxp->lhsp()->width() - 1) RETURN_RESULT(shiftrp);
        DfgSel* மாறிலி selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_RESULT(selp);
    }

    வெறுமை visit(DfgShiftL* vtxp) override {
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        எனில் (மாறிலி DfgConst* மாறிலி rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            மாறிலி uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            மாறிலி uint32_t lowerWidth = shiftAmnt > vtxp->width() ? vtxp->width() : shiftAmnt;
            // If the traced bits are wholly in the input
            எனில் (m_lsb >= lowerWidth) {
                RETURN_RESULT_TAIL(lhsp, m_msb - shiftAmnt, m_lsb - shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            எனில் (lowerWidth > m_msb) RETURN_RESULT(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* மாறிலி resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->width() - (m_msb - lowerWidth + 1)));
            resp->lhsp(trace(lhsp, m_msb - shiftAmnt, lowerWidth - shiftAmnt));
            RETURN_RESULT(resp);
        }

        DfgShiftL* மாறிலி shiftlp = make<DfgShiftL>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        எனில் (m_lsb == 0) RETURN_RESULT(shiftlp);
        DfgSel* மாறிலி selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

    வெறுமை visit(DfgCond* vtxp) override {
        DfgCond* மாறிலி resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->width() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_RESULT(resp);
    }

    வெறுமை visit(DfgMatchMasked* vtxp) override {
        DfgMatchMasked* மாறிலி resp = make<DfgMatchMasked>(vtxp, vtxp->width());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->width() - 1, 0));
        DfgSel* மாறிலி selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_RESULT(selp);
    }

#undef RETURN_RESULT
#undef TRACE_RESULT_COVER
#undef RETURN_RESULT_TAIL

பொது:
    // CONSTRUCTOR
    TraceDriver(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#ifdef VL_DEBUG
        எனில் (v3Global.opt.debugCheck()) {
            m_வரி_Coverage_File.open(  //
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
        திரும்பு trace(&vtx, lsb + width - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
வகுப்பு BitMask final {
    // TYPES
    மாறிலி DfgDataType& m_dtype;
    ஒன்றிப்பு {
        V3_எண் m_இலக்கம்;  // The bits, if packed
        std::vector<BitMask> m_sub;  // The per element masks, if aggregate
    };

பொது:
    // CONSTRUCTOR
    BitMask(File_வரி* flp, மாறிலி DfgDataType& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        எனில் (m_dtype.isPacked()) {
            புதிய (&m_இலக்கம்) V3_எண்{flp, static_cast<முழு>(m_dtype.size()), 0};
        } இல்லையெனில் {
            புதிய (&m_sub) std::vector<BitMask>{};
            m_sub.reserve(m_dtype.size());
            க்கு (uint32_t i = 0; i < m_dtype.size(); ++i)
                m_sub.emplace_back(flp, m_dtype.elemDtype());
        }
    }
    BitMask(மாறிலி DfgVertex& vtx)
        : BitMask{vtx.fileline(), vtx.dtype()} {}
    BitMask(மாறிலி BitMask& other)
        : m_dtype{other.m_dtype} {
        எனில் (m_dtype.isPacked()) {
            புதிய (&m_இலக்கம்) V3_எண்{other.m_இலக்கம்};
        } இல்லையெனில் {
            புதிய (&m_sub) std::vector<BitMask>{other.m_sub};
        }
    }
    BitMask& operator=(மாறிலி BitMask& other) {
        எனில் (இது != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            எனில் (other.m_dtype.isPacked()) {
                m_இலக்கம் = other.m_இலக்கம்;
            } இல்லையெனில் {
                m_sub = other.m_sub;
            }
        }
        திரும்பு *இது;
    }
    ~BitMask() {
        எனில் (m_dtype.isPacked()) {
            m_இலக்கம்.~V3_எண்();
        } இல்லையெனில் {
            m_sub.~vector<BitMask>();
        }
    }

    // METHODS
    V3_எண்& இலக்கம்() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        திரும்பு m_இலக்கம்;
    }
    மாறிலி V3_எண்& இலக்கம்() மாறிலி {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        திரும்பு m_இலக்கம்;
    }
    std::vector<BitMask>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        திரும்பு m_sub;
    }

    தருக்க isZero() மாறிலி {
        எனில் (m_dtype.isPacked()) திரும்பு m_இலக்கம்.isEqZero();

        க்கு (மாறிலி BitMask& sub : m_sub) {
            எனில் (!sub.isZero()) திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }
    தருக்க isOnes() மாறிலி {
        எனில் (m_dtype.isPacked()) திரும்பு m_இலக்கம்.isEqAllOnes();

        க்கு (மாறிலி BitMask& sub : m_sub) {
            எனில் (!sub.isOnes()) திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }
    வெறுமை setZero() {
        எனில் (m_dtype.isPacked()) {
            m_இலக்கம்.setAllBits0();
        } இல்லையெனில் {
            க்கு (BitMask& sub : m_sub) sub.setZero();
        }
    }
    வெறுமை setOnes() {
        எனில் (m_dtype.isPacked()) {
            m_இலக்கம்.setAllBits1();
        } இல்லையெனில் {
            க்கு (BitMask& sub : m_sub) sub.setOnes();
        }
    }

    தருக்க operator==(மாறிலி BitMask& other) மாறிலி {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        எனில் (m_dtype.isPacked()) திரும்பு m_இலக்கம்.isCaseEq(other.m_இலக்கம்);
        திரும்பு m_sub == other.m_sub;
    }
    தருக்க operator!=(மாறிலி BitMask& other) மாறிலி { திரும்பு !(*இது == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    தருக்க isContractionOf(மாறிலி BitMask& that) மாறிலி {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        எனில் (m_dtype.isPacked()) {
            மாறிலி size_t சொற்கள் = VL_சொற்கள்_I(m_dtype.size());
            க்கு (size_t i = 0; i < சொற்கள்; ++i) {
                எனில் (~m_இலக்கம்.edataWord(i) & that.m_இலக்கம்.edataWord(i)) திரும்பு உண்மை;
            }
            திரும்பு பொய்;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        க்கு (size_t i = 0; i < m_sub.size(); ++i) {
            எனில் (m_sub[i].isContractionOf(that.m_sub[i])) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    std::string toString() மாறிலி {
        எனில் (m_dtype.isPacked()) திரும்பு "0x" + m_இலக்கம்.displayed(m_இலக்கம்.fileline(), "%x");
        std::string result;
        result += "{";
        க்கு (மாறிலி BitMask& sub : m_sub) {
            எனில் (&sub != &m_sub.front()) result += ", ";
            result += sub.toString();
        }
        result += "}";
        திரும்பு result;
    }
};

வகுப்பு IndependentBits final : பொது DfgVisitor {
    // TYPES
    கட்டமைப்பு VertexState final {
        size_t m_rpo_எண் = 0;  // Reverse Postorder Number of vertex
        தருக்க m_isOnWorkList = பொய்;  // Whether the vertex is on the work list
    };

    // STATE
    மாறிலி SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<மாறிலி DfgVertex*, BitMask> m_vtxp2Mask;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> m_workQueue;
    std::unordered_map<மாறிலி DfgVertex*, VertexState> m_vtxp2State;  // State of each vertex

#ifdef VL_DEBUG
    std::ofstream m_வரி_Coverage_File;  // Line coverage file, just for testing
#endif

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    BitMask& mask(மாறிலி DfgVertex& vtx) {
        // Look up (or create) mask for 'vtx'
        திரும்பு m_vtxp2Mask
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .first->second;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#ifdef VL_DEBUG
#define MASK(vtxp) \
    ([இது](மாறிலி DfgVertex& vtx) -> BitMask& { \
        எனில் (VL_UNLIKELY(m_வரி_Coverage_File.is_open())) m_வரி_Coverage_File << __LINE__ << '\n'; \
        திரும்பு mask(vtx); \
    }(*vtxp))
#இல்லையெனில்
#define MASK(vtxp) mask(*vtxp)
#endif

    // Clear all bits at or below the most significant clear bit
    வெறுமை floodTowardsLsb(V3_எண்& இலக்கம்) {
        க்கு (முழு i = இலக்கம்.width() - 1; i >= 0; --i) {
            எனில் (இலக்கம்.bitIs1(i)) தொடர்;
            இலக்கம்.opSetRange(0, i + 1, '0');
            நிறுத்து;
        }
    }

    // Clear all bits at or above the least significant clear bit
    வெறுமை floodTowardsMsb(V3_எண்& இலக்கம்) {
        க்கு (முழு i = 0; i < இலக்கம்.width(); ++i) {
            எனில் (இலக்கம்.bitIs1(i)) தொடர்;
            இலக்கம்.opSetRange(i, இலக்கம்.width() - i, '0');
            நிறுத்து;
        }
    }

    வெறுமை propagateFromDriver(BitMask& m, மாறிலி DfgVertex* srcp) {
        // If there is no driver, we are done
        எனில் (!srcp) திரும்பு;
        // If it is driven by a splice, we need to combine the masks of the drivers
        எனில் (மாறிலி DfgSplicePacked* மாறிலி splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](மாறிலி DfgVertex& src, uint32_t lo) {
                m.இலக்கம்().opSelInto(MASK(&src).இலக்கம்(), lo, src.width());
                திரும்பு பொய்;
            });
            திரும்பு;
        }
        எனில் (மாறிலி DfgSpliceArray* மாறிலி splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](மாறிலி DfgVertex& src, uint32_t lo) {
                எனில் (மாறிலி DfgUnitArray* மாறிலி uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = MASK(uap->srcp());
                } இல்லையெனில் {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                திரும்பு பொய்;
            });
            திரும்பு;
        }
        // Otherwise, we just use the mask of the single driver
        m = MASK(srcp);
    }

    // VISITORS
    வெறுமை visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    வெறுமை visit(DfgVertexVar* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        எனில் (vtxp->isVolatile()) திரும்பு;

        BitMask& m = MASK(vtxp);
        DfgVertex* மாறிலி srcp = vtxp->srcp();
        DfgVertex* மாறிலி defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        எனில் (defaultp) m = MASK(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    வெறுமை visit(DfgVertexSplice* vtxp) override {
        propagateFromDriver(MASK(vtxp), vtxp);  // Needed to continue traversal
    }

    வெறுமை visit(DfgUnitArray* vtxp) override { MASK(vtxp).sub().at(0) = MASK(vtxp->srcp()); }

    வெறுமை visit(DfgArraySel* vtxp) override {
        DfgVertex* மாறிலி fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        எனில் (மாறிலி DfgConst* மாறிலி idxp = vtxp->bitp()->cast<DfgConst>()) {
            MASK(vtxp) = MASK(vtxp->fromp()).sub().at(idxp->toSizeT());
            திரும்பு;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        எனில் (MASK(vtxp->bitp()).isOnes() && MASK(fromp).isOnes()) MASK(vtxp).setOnes();
    }

    வெறுமை visit(DfgConcat* vtxp) override {
        மாறிலி DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        மாறிலி DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        V3_எண்& m = MASK(vtxp).இலக்கம்();
        m.opSelInto(MASK(rhsp).இலக்கம்(), 0, rhsp->width());
        m.opSelInto(MASK(lhsp).இலக்கம்(), rhsp->width(), lhsp->width());
    }

    வெறுமை visit(DfgRep* vtxp) override {
        மாறிலி uint32_t மொத்தம் = vtxp->மொத்தம்();
        மாறிலி DfgVertex* மாறிலி srcp = vtxp->srcp();
        மாறிலி uint32_t sWidth = srcp->width();
        V3_எண்& vMask = MASK(vtxp).இலக்கம்();
        V3_எண்& sMask = MASK(srcp).இலக்கம்();
        க்கு (uint32_t i = 0; i < மொத்தம்; ++i) vMask.opSelInto(sMask, i * sWidth, sWidth);
    }

    வெறுமை visit(DfgSel* vtxp) override {
        மாறிலி uint32_t lsb = vtxp->lsb();
        மாறிலி uint32_t msb = lsb + vtxp->width() - 1;
        MASK(vtxp).இலக்கம்().opSel(MASK(vtxp->fromp()).இலக்கம்(), msb, lsb);
    }

    வெறுமை visit(DfgExtend* vtxp) override {
        மாறிலி DfgVertex* மாறிலி srcp = vtxp->srcp();
        மாறிலி uint32_t sWidth = srcp->width();
        V3_எண்& s = MASK(srcp).இலக்கம்();
        V3_எண்& m = MASK(vtxp).இலக்கம்();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, '1');
    }
    வெறுமை visit(DfgExtendS* vtxp) override {
        மாறிலி DfgVertex* மாறிலி srcp = vtxp->srcp();
        மாறிலி uint32_t sWidth = srcp->width();
        V3_எண்& s = MASK(srcp).இலக்கம்();
        V3_எண்& m = MASK(vtxp).இலக்கம்();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, s.bitIs0(sWidth - 1) ? '0' : '1');
    }

    வெறுமை visit(DfgNot* vtxp) override {  //
        MASK(vtxp) = MASK(vtxp->srcp());
    }

    வெறுமை visit(DfgAnd* vtxp) override {  //
        MASK(vtxp).இலக்கம்().opAnd(MASK(vtxp->lhsp()).இலக்கம்(), MASK(vtxp->rhsp()).இலக்கம்());
    }
    வெறுமை visit(DfgOr* vtxp) override {  //
        MASK(vtxp).இலக்கம்().opAnd(MASK(vtxp->lhsp()).இலக்கம்(), MASK(vtxp->rhsp()).இலக்கம்());
    }
    வெறுமை visit(DfgXor* vtxp) override {  //
        MASK(vtxp).இலக்கம்().opAnd(MASK(vtxp->lhsp()).இலக்கம்(), MASK(vtxp->rhsp()).இலக்கம்());
    }

    வெறுமை visit(DfgAdd* vtxp) override {
        V3_எண்& m = MASK(vtxp).இலக்கம்();
        m.opAnd(MASK(vtxp->lhsp()).இலக்கம்(), MASK(vtxp->rhsp()).இலக்கம்());
        floodTowardsMsb(m);
    }
    வெறுமை visit(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_எண்& m = MASK(vtxp).இலக்கம்();
        m.opAnd(MASK(vtxp->lhsp()).இலக்கம்(), MASK(vtxp->rhsp()).இலக்கம்());
        floodTowardsMsb(m);
    }

    வெறுமை visit(DfgRedAnd* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }
    வெறுமை visit(DfgRedOr* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }
    வெறுமை visit(DfgRedXor* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }

    வெறுமை visit(DfgEq* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }
    வெறுமை visit(DfgNeq* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }
    வெறுமை visit(DfgLt* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }
    வெறுமை visit(DfgLte* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }
    வெறுமை visit(DfgGt* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }
    வெறுமை visit(DfgGte* vtxp) override {
        மாறிலி தருக்க independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        எனில் (independent) MASK(vtxp).setOnes();
    }

    வெறுமை visit(DfgShiftRS* vtxp) override {
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        மாறிலி uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        எனில் (DfgConst* மாறிலி rConstp = rhsp->cast<DfgConst>()) {
            மாறிலி uint32_t shiftAmount = rConstp->toU32();
            எனில் (shiftAmount >= width) {
                எனில் (MASK(lhsp).இலக்கம்().bitIs1(width - 1)) { MASK(vtxp).setOnes(); }
            } இல்லையெனில் {
                V3_எண்& m = MASK(vtxp).இலக்கம்();
                m.opShiftRS(MASK(lhsp).இலக்கம்(), rConstp->இலக்கம்(), width);
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            திரும்பு;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        எனில் (MASK(rhsp).isOnes()) {
            V3_எண்& m = MASK(vtxp).இலக்கம்();
            m = MASK(lhsp).இலக்கம்();
            floodTowardsLsb(m);
        }
    }

    வெறுமை visit(DfgShiftR* vtxp) override {
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        மாறிலி uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        எனில் (DfgConst* மாறிலி rConstp = rhsp->cast<DfgConst>()) {
            மாறிலி uint32_t shiftAmount = rConstp->toU32();
            எனில் (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } இல்லையெனில் {
                V3_எண்& m = MASK(vtxp).இலக்கம்();
                m.opShiftR(MASK(lhsp).இலக்கம்(), rConstp->இலக்கம்());
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            திரும்பு;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        எனில் (MASK(rhsp).isOnes()) {
            V3_எண்& m = MASK(vtxp).இலக்கம்();
            m = MASK(lhsp).இலக்கம்();
            floodTowardsLsb(m);
        }
    }

    வெறுமை visit(DfgShiftL* vtxp) override {
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        மாறிலி uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        எனில் (DfgConst* மாறிலி rConstp = rhsp->cast<DfgConst>()) {
            மாறிலி uint32_t shiftAmount = rConstp->toU32();
            எனில் (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } இல்லையெனில் {
                V3_எண்& m = MASK(vtxp).இலக்கம்();
                m.opShiftL(MASK(lhsp).இலக்கம்(), rConstp->இலக்கம்());
                m.opSetRange(0, shiftAmount, '1');
            }
            திரும்பு;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        எனில் (MASK(rhsp).isOnes()) {
            V3_எண்& m = MASK(vtxp).இலக்கம்();
            m = MASK(lhsp).இலக்கம்();
            floodTowardsMsb(m);
        }
    }

    வெறுமை visit(DfgCond* vtxp) override {
        எனில் (MASK(vtxp->condp()).isOnes()) {
            MASK(vtxp).இலக்கம்().opAnd(MASK(vtxp->thenp()).இலக்கம்(), MASK(vtxp->elsep()).இலக்கம்());
        }
    }

    வெறுமை visit(DfgMatchMasked* vtxp) override {
        எனில் (MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->matchp()).isOnes()) { MASK(vtxp).setOnes(); }
    }

#undef MASK

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    வெறுமை enqueueSinks(DfgVertex& vtx) {
        vtx.foreachSink([&](DfgVertex& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            எனில் (!m_sccInfo.get(sink)) திரும்பு பொய்;
            // Otherwise just enqueue it
            VertexState& state = m_vtxp2State.at(&sink);
            எனில் (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_workQueue.தள்ளு(state.m_rpo_எண்);
            }
            state.m_isOnWorkList = உண்மை;
            திரும்பு பொய்;
        });
    };

    வெறுமை dfsPostOrder(std::unordered_set<DfgVertex*>& visited,  //
                      std::vector<DfgVertex*>& postOrderEnumeration,  //
                      DfgVertex& vtx) {
        // Mark visited, skip if already visited
        எனில் (!visited.செருகு(&vtx).second) திரும்பு;
        // Visit un-visited successors
        vtx.foreachSink([&](DfgVertex& snk) {
            dfsPostOrder(visited, postOrderEnumeration, snk);
            திரும்பு பொய்;
        });
        // Add to post order enumeration
        postOrderEnumeration.emplace_back(&vtx);
    };

    IndependentBits(DfgGraph& dfg, மாறிலி SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#ifdef VL_DEBUG
        எனில் (v3Global.opt.debugCheck()) {
            m_வரி_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::app);
        }
#endif

        // Compute Reverse-Postorder enumeration of vertices
        std::vector<DfgVertex*> rpoEnumeration;
        std::unordered_set<DfgVertex*> visited;
        dfg.forEachVertex([&](DfgVertex& vtx) {
            எனில் (!vtx.nInputs()) dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        dfg.forEachVertex([&](DfgVertex& vtx) {  //
            dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        std::reverse(rpoEnumeration.begin(), rpoEnumeration.end());
        UASSERT(rpoEnumeration.size() == dfg.size(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        க்கு (size_t i = 0; i < rpoEnumeration.size(); ++i) {
            VertexState& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_எண் = i;
            state.m_isOnWorkList = பொய்;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        க்கு (DfgVertex* மாறிலி vtxp : rpoEnumeration) {
            எனில் (m_sccInfo.get(*vtxp)) தொடர்;
            mask(*vtxp).setOnes();
        }
        க்கு (DfgVertex* மாறிலி vtxp : rpoEnumeration) {
            எனில் (!m_sccInfo.get(*vtxp)) தொடர்;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << mask(*vtxp).toString());
            எனில் (!mask(*vtxp).isZero()) enqueueSinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        போது (!m_workQueue.empty()) {
            // Grab next item
            DfgVertex* மாறிலி currp = rpoEnumeration[m_workQueue.உச்சி()];
            m_workQueue.எடு();
            m_vtxp2State.at(currp).m_isOnWorkList = பொய்;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            மாறிலி BitMask& currMask = mask(*currp);
            // Remember current mask so we can check if it changed
            மாறிலி BitMask prevMask = currMask;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            எனில் (prevMask != currMask) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prevMask.toString()
                             << "\n      to: " << currMask.toString());
                enqueueSinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                எனில் (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!currMask.isContractionOf(prevMask), currp,
                                "Mask should only expand");
                }
            }
        }
    }

பொது:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    நிலையான std::unordered_map<மாறிலி DfgVertex*, BitMask> apply(DfgGraph& dfg,
                                                               மாறிலி SccInfo& sccInfo) {
        திரும்பு std::move(IndependentBits{dfg, sccInfo}.m_vtxp2Mask);
    }
};

வகுப்பு FixUp final {
    DfgGraph& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    மாறிலி std::unordered_map<மாறிலி DfgVertex*, BitMask> m_independentBits
        = IndependentBits::apply(m_dfg, m_sccInfo);
    size_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    நிலையான BitMask computeUsedBits(DfgVertex& vtx) {
        BitMask result{vtx};
        vtx.foreachSink([&result](DfgVertex& sink) {
            // If used via a Sel, mark the selected bits used
            எனில் (மாறிலி DfgSel* மாறிலி selp = sink.cast<DfgSel>()) {
                result.இலக்கம்().opSetRange(selp->lsb(), selp->width(), '1');
                திரும்பு பொய்;
            }
            // Used without a Sel, so all bits are used
            result.setOnes();
            திரும்பு உண்மை;
        });
        திரும்பு result;
    }

    நிலையான std::string debugStr(மாறிலி DfgVertex& vtx) {  // LCOV_EXCL_START
        எனில் (மாறிலி DfgArraySel* மாறிலி aselp = vtx.cast<DfgArraySel>()) {
            மாறிலி size_t i = aselp->bitp()->as<DfgConst>()->toSizeT();
            திரும்பு debugStr(*aselp->fromp()) + "[" + std::to_string(i) + "]";
        }
        எனில் (மாறிலி DfgVertexVar* மாறிலி varp = vtx.cast<DfgVertexVar>()) {
            திரும்பு varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    வெறுமை gatherTerms(std::vector<DfgVertex*>& termps, DfgVertex& vtx, மாறிலி V3_எண்& indpBits,
                     மாறிலி uint32_t hi, மாறிலி uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        தருக்க isIndependent = indpBits.bitIs1(lo);
        uint32_t lsb = lo;
        க்கு (uint32_t msb = lo; msb <= hi; ++msb) {
            மாறிலி தருக்க endRange = msb == hi || isIndependent != indpBits.bitIs1(msb + 1);
            எனில் (!endRange) தொடர்;
            மாறிலி uint32_t width = msb - lsb + 1;
            // Compute the term to use for this range
            எனில் (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_back(m_traceDriver.apply(vtx, lsb, width));
            } இல்லையெனில் {
                // If dependent, fall back on using the part of the variable
                DfgSel* மாறிலி selp = புதிய DfgSel{m_dfg, vtx.fileline(), DfgDataType::packed(width)};
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

    வெறுமை fixUpPacked(DfgVertex& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        மாறிலி BitMask& indpBits = m_independentBits.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indpBits.toString());
        // Can't do anything if all bits are dependent
        எனில் (indpBits.isZero()) திரும்பு;

        // Figure out which bits of 'vtxp' are used
        மாறிலி BitMask usedBits = computeUsedBits(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << usedBits.toString());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_எண் usedAndIndependent{vtx.fileline(), static_cast<முழு>(vtx.width()), 0};
        usedAndIndependent.opAnd(usedBits.இலக்கம்(), indpBits.இலக்கம்());
        எனில் (usedAndIndependent.isEqZero()) திரும்பு;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::vector<DfgVertex*> termps;

        // Iterate through consecutive used/unused ranges
        File_வரி* மாறிலி flp = vtx.fileline();
        மாறிலி uint32_t width = vtx.width();
        தருக்க isUsed = usedBits.இலக்கம்().bitIs1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        க்கு (uint32_t msb = 0; msb < width; ++msb) {
            மாறிலி தருக்க endRange = msb == width - 1 || isUsed != usedBits.இலக்கம்().bitIs1(msb + 1);
            எனில் (!endRange) தொடர்;
            எனில் (isUsed) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indpBits.இலக்கம்(), msb, lsb);
            } இல்லையெனில் {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* மாறிலி constp = புதிய DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.add(*constp, 0);
                termps.emplace_back(constp);
            }
            // Next iteration
            isUsed = !isUsed;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        DfgVertex* replacementp = termps.front();
        மாறிலி uint64_t vComp = m_sccInfo.get(vtx);
        க்கு (size_t i = 1; i < termps.size(); ++i) {
            DfgVertex* மாறிலி termp = termps[i];
            மாறிலி uint32_t catWidth = replacementp->width() + termp->width();
            DfgConcat* மாறிலி catp = புதிய DfgConcat{m_dfg, flp, DfgDataType::packed(catWidth)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            மாறிலி uint64_t tComp = m_sccInfo.get(*termp);
            மாறிலி uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.add(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        க்கு (DfgVertex* மாறிலி termp : termps) {
            எனில் (DfgSel* மாறிலி selp = termp->cast<DfgSel>()) {
                எனில் (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        எனில் (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    வெறுமை முதன்மை(DfgVertexVar& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        எனில் (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fixUpPacked(var);
        } இல்லையெனில் எனில் (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            மாறிலி uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](DfgVertex& sink) {
                // Ignore if sink is not part of same cycle
                எனில் (m_sccInfo.get(sink) != component) திரும்பு பொய்;
                // Only handle ArraySels with constant index
                DfgArraySel* மாறிலி aselp = sink.cast<DfgArraySel>();
                எனில் (!aselp) திரும்பு பொய்;
                எனில் (!aselp->bitp()->is<DfgConst>()) திரும்பு பொய்;
                // Fix up the word
                fixUpPacked(*aselp);
                திரும்பு பொய்;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    FixUp(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

பொது:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    நிலையான size_t apply(DfgGraph& dfg, SccInfo& sccInfo) {
        எனில் (!sccInfo.isCyclic()) திரும்பு 0;

        FixUp fixUp{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        க்கு (DfgVertexVar& vtx : dfg.varVertices()) {
            எனில் (!sccInfo.get(vtx)) தொடர்;
            fixUp.முதன்மை(vtx);
            எனில் (!sccInfo.isCyclic()) நிறுத்து;
        }
        // Return the number of improvements made
        திரும்பு fixUp.m_nImprovements;
    }
};

வெறுமை breakCycles(DfgGraph& dfg, V3DfgBreakCyclesContext& ctx) {
    // Shorthand for dumping graph at given dump level
    மாறிலி auto dump = [&](முழு level, மாறிலி DfgGraph& dfg, மாறிலி std::string& name) {
        எனில் (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    size_t nImprovements = 0;
    size_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    செய் {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        மாறிலி size_t nFixed = FixUp::apply(dfg, sccInfo);
        எனில் (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        எனில் (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        எனில் (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            திரும்பு;
        }
    } போது (nImprovements != prevNImprovements);

    // Debug dump
    எனில் (dumpDfgLevel() >= 9) {
        மாறிலி SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](மாறிலி DfgVertex& vtx) {
            திரும்பு sccInfo.get(vtx);  //
        });
    }

    // Accounting
    எனில் (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } இல்லையெனில் {
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
    க்கு (DfgVertexVar& vtx : dfg.varVertices()) {
        எனில் (!vtx.tmpForp()) varps.தள்ளு_back(&vtx);
    }
    std::stable_partition(varps.begin(), varps.end(), [](மாறிலி DfgVertexVar* varp) {
        திரும்பு varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    க்கு (DfgVertexVar* மாறிலி varp : varps) {
        // Stop as soon as it becomes acyclic
        எனில் (!sccInfo.isCyclic()) நிறுத்து;

        // Ignore if variable is not part of a cycle
        எனில் (!sccInfo.get(*varp)) தொடர்;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* மாறிலி prevp = புதிய DfgPrev{dfg, varp->vscp()};
        sccInfo.add(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    எனில் (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
வெறுமை V3DfgPasses::breakCycles(DfgGraph& dfg, V3DfgContext& ctx) {
    V3DfgBreakCycles::breakCycles(dfg, ctx.m_breakCyclesContext);
    எனில் (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::removeUnused(dfg);
}
