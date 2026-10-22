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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"
#समावेश "V3Error.h"
#समावेश "V3Hash.h"

#समावेश <algorithm>
#समावेश <fstream>
#समावेश <queue>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3DfgBreakCycles {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
श्रेणी SccInfo final {
    DfgGraph& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<स्थिर DfgVertex*, uint64_t> m_vtx2Scc;
    माप_प्रकार m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

खुला:
    SccInfo(DfgGraph& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.forEachVertex([&](स्थिर DfgVertex& vtx) {
            स्थिर uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            अगर (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    bool isCyclic() स्थिर { वापस m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t लो(स्थिर DfgVertex& vtx) स्थिर { वापस m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    व्योम add(स्थिर DfgVertex& vtx, uint64_t sccIdx) {
        स्थिर bool newEntry = m_vtx2Scc.emplace(&vtx, sccIdx).second;
        UASSERT_OBJ(newEntry, &vtx, "Vertex already inserted");
        अगर (sccIdx) ++m_nCyclicVertices;
    }

    bool stillCyclicFwd(स्थिर DfgVertex& vtx) स्थिर {
        // If it only reads non-cyclic vertices, it has also become acyclic
        वापस vtx.foreachSource([&](स्थिर DfgVertex& src) {  //
            वापस m_vtx2Scc.at(&src);
        });
    }

    bool stillCyclicBwd(स्थिर DfgVertex& vtx) स्थिर {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        वापस vtx.foreachSink([&](स्थिर DfgVertex& dst) {  //
            वापस m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    व्योम updateAcyclic(स्थिर DfgVertex& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](स्थिर DfgVertex& dst) {
            // Sink was known to be acyclic, stop
            अगर (!m_vtx2Scc.at(&dst)) वापस false;
            अगर (!stillCyclicFwd(dst)) updateAcyclic(dst);
            वापस false;
        });
        // Propagate the update through the graph backward
        vtx.foreachSource([&](स्थिर DfgVertex& src) {
            // Source was known to be acyclic, stop
            अगर (!m_vtx2Scc.at(&src)) वापस false;
            अगर (!stillCyclicBwd(src)) updateAcyclic(src);
            वापस false;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    व्योम updateReplacement(स्थिर DfgVertex& vtx, DfgVertex& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        अगर (!लो(replacement)) {
            replacement.foreachSink([&](DfgVertex& dst) {
                अगर (!लो(dst)) वापस false;
                अगर (!stillCyclicFwd(dst)) updateAcyclic(dst);
                वापस false;
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
    व्योम validate() स्थिर {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.forEachVertex([&](स्थिर DfgVertex& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

श्रेणी TraceDriver final : खुला DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    काष्ठा Cache_कुंजी final {
        DfgVertex* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_कुंजी() = मिटाओ;
        Cache_कुंजी(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        काष्ठा Hash final {
            माप_प्रकार चालक()(स्थिर Cache_कुंजी& item) स्थिर {
                // cppcheck-suppress unreadVariable
                V3Hash hash{item.m_vtxp};
                hash += item.m_lsb;
                hash += item.m_msb;
                वापस hash.मूल्य();
            }
        };

        काष्ठा Equal final {
            bool चालक()(स्थिर Cache_कुंजी& a, स्थिर Cache_कुंजी& b) स्थिर {
                वापस a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
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
    std::unordered_map<Cache_कुंजी, DfgVertex*, Cache_कुंजी::Hash, Cache_कुंजी::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_कुंजी, DfgVertex*, Cache_कुंजी::Hash, Cache_कुंजी::Equal> m_boundaryCache;

#अगर_घोषित VL_DEBUG
    std::बख_प्रवाह m_पंक्ति_Coverage_File;  // Line coverage file, just for testing
#पूर्ण_अगर

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    ढाँचा <typename Vertex>
    Vertex* make(स्थिर DfgVertex* refp, uint32_t width) {
        static_assert(std::is_base_of<DfgVertex, Vertex>::मूल्य  //
                          && !std::is_base_of<DfgVertexVar, Vertex>::मूल्य,
                      "Should only make operation vertices and constants");

        अगर VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, Vertex>::मूल्य) {
            DfgConst* स्थिर vtxp = नया DfgConst{m_dfg, refp->fileline(), width, 0};
            m_sccInfo.add(*vtxp, 0);
            वापस reinterpret_cast<Vertex*>(vtxp);
        } अन्यथा {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            using Vtx = typename std::conditional<std::is_same<DfgConst, Vertex>::मूल्य, DfgSel,
                                                  Vertex>::type;
            Vtx* स्थिर vtxp = नया Vtx{m_dfg, refp->fileline(), Dfg_तथ्य_Type::packed(width)};
            m_sccInfo.add(*vtxp, 0);
            वापस reinterpret_cast<Vertex*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    DfgVertexVar* createTmp(स्थिर अक्षर* prefix, DfgVertex* vtxp) {
        File_पंक्ति* स्थिर flp = vtxp->fileline();
        DfgVertex::ScopeCache scopeCache;
        AstScope* स्थिर scopep = vtxp->scopep(scopeCache);
        DfgVertexVar* स्थिर varp = m_dfg.makeNewVar(flp, prefix, vtxp->dtype(), scopep);
        varp->vscp()->varp()->isInternal(true);
        varp->tmpForp(varp->vscp());
        m_sccInfo.add(*varp, 0);
        वापस varp;
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
        DfgVertex* स्थिर startVtxp = vtxp;
        स्थिर uint32_t startMsb = msb;
        स्थिर uint32_t startLsb = lsb;
        स्थिर माप_प्रकार pendingBegin = m_pendingps.आकार();
        DfgVertex* resp = nullptr;

        जबतक (true) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->आकार() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            अगर (m_sccInfo.लो(*vtxp) != m_component && !m_defaultp) {
                DfgVertex*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(nullptr))
                          .first->second;
                अगर (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    अगर (DfgVertexSplice* स्थिर splicep = respr->cast<DfgVertexSplice>()) {
                        DfgVertexVar* स्थिर tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    अगर (msb != respr->width() - 1 || lsb != 0) {
                        DfgSel* स्थिर selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                अवरोध;
            }

            // Normal cache for intra-component traces
            DfgVertex*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(nullptr))
                                    .first->second;

            // Trace the vertex
            अगर (respr) {
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
                अगर (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.धकेलो_back(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    जारी;
                }
                respr = m_resp;
            }
            resp = respr;
            अवरोध;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, startVtxp, "Tracing driver failed for " << startVtxp->typeName());
        UASSERT_OBJ(resp->width() == (startMsb - startLsb + 1), startVtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        क्रम (माप_प्रकार i = pendingBegin; i < m_pendingps.आकार(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        वापस resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    DfgVertex* traceSameIdx(DfgVertex* स्थिर vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = nullptr;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->width() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        वापस m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    DfgVertex* trace_धकेलो_Idx(DfgVertex* स्थिर vtxp, स्थिर uint32_t idx) {
        स्थिर माप_प्रकार nIdxs = m_idxs.आकार();
        m_idxs.धकेलो_back(idx);
        DfgVertex* स्थिर resp = traceSameIdx(vtxp);
        m_idxs.निकालो_back();
        UASSERT_OBJ(m_idxs.आकार() == nIdxs, vtxp, "Index stack size mismatch");
        वापस resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    DfgVertex* trace_निकालो_Idx(DfgVertex* स्थिर vtxp) {
        UASSERT_OBJ(!m_idxs.empty(), vtxp, "Should have a pending index");
        स्थिर माप_प्रकार nIdxs = m_idxs.आकार();
        स्थिर uint32_t idx = m_idxs.back();
        m_idxs.निकालो_back();
        DfgVertex* स्थिर resp = m_idxs.empty() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.धकेलो_back(idx);
        UASSERT_OBJ(m_idxs.आकार() == nIdxs, vtxp, "Index stack size mismatch");
        वापस resp;
    }

    ढाँचा <typename Vertex>
    Vertex* trace_Bitwise_द्विआधारी(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विआधारी, Vertex>::मूल्य,
                      "Should only call on DfgVertexBinary");
        Vertex* स्थिर resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        वापस resp;
    }

    ढाँचा <typename Vertex>
    DfgVertex* traceAddSub(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विआधारी, Vertex>::मूल्य,
                      "Should only call on DfgVertexBinary");
        Vertex* स्थिर opp = make<Vertex>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* स्थिर selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        वापस selp;
    }

    ढाँचा <typename Vertex>
    Vertex* traceReduction(Vertex* vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::मूल्य,
                      "Should only call on DfgVertexUnary");
        Vertex* स्थिर resp = make<Vertex>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->width() - 1, 0));
        वापस resp;
    }

    ढाँचा <typename Vertex>
    Vertex* traceCmp(Vertex* vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विआधारी, Vertex>::मूल्य,
                      "Should only call on DfgVertexBinary");
        Vertex* स्थिर resp = make<Vertex>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        वापस resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#अगर_घोषित VL_DEBUG
#घोषणा TRACE_परिणाम_COVER() \
    करो { \
        अगर (VL_UNLIKELY(m_पंक्ति_Coverage_File.is_open())) m_पंक्ति_Coverage_File << __LINE__ << '\n'; \
    } जबतक (false)
#अन्यथा
#घोषणा TRACE_परिणाम_COVER() \
    करो { \
    } जबतक (false)
#पूर्ण_अगर

#घोषणा RETURN_परिणाम(vtxp) \
    करो { \
        m_resp = (vtxp); \
        TRACE_परिणाम_COVER(); \
        वापस; \
    } जबतक (false)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#घोषणा RETURN_परिणाम_पुच्छ(srcp, smsb, slsb) \
    करो { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        DfgVertex* स्थिर tailp = (srcp); \
        स्थिर uint32_t पुच्छ_Msb = (smsb); \
        स्थिर uint32_t पुच्छ_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = पुच्छ_Msb; \
        m_lsb = पुच्छ_Lsb; \
        TRACE_परिणाम_COVER(); \
        वापस; \
    } जबतक (false)

    // VISITORS
    व्योम visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    व्योम visit(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        DfgVertex* defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        काष्ठा Driver final {
            DfgVertex* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = मिटाओ;
            Driver(DfgVertex* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::vector<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        bool tryWholeDefault = defaultp;
        DfgVertex* coverp = nullptr;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](DfgVertex& src, uint32_t lsb) {
            स्थिर uint32_t msb = lsb + src.width() - 1;
            drivers.emplace_back(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            अगर (m_msb >= lsb && msb >= m_lsb) tryWholeDefault = false;
            // If it does not cover the whole searched bit range, move on
            अगर (m_lsb < lsb || msb < m_msb) वापस false;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            वापस true;
        });

        // Trace the driver covering the whole searched range, if there is one
        अगर (coverp) RETURN_परिणाम_पुच्छ(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        अगर (tryWholeDefault) RETURN_परिणाम_पुच्छ(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::sort(drivers.begin(), drivers.end(), [](स्थिर Driver& a, स्थिर Driver& b) {  //
            वापस a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::vector<DfgVertex*> termps;
        uint32_t lsb = m_lsb;
        क्रम (स्थिर Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            अगर (lsb > driver.m_msb) जारी;
            // Driver is above the searched MSB, done
            अगर (driver.m_lsb > m_msb) अवरोध;
            // Gap below this driver, trace default to fill it
            अगर (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_back(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            स्थिर uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_back(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        अगर (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_back(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.आकार() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        DfgVertex* resp = termps.front();
        क्रम (माप_प्रकार i = 1; i < termps.आकार(); ++i) {
            DfgVertex* स्थिर termp = termps[i];
            DfgConcat* स्थिर catp = make<DfgConcat>(termp, resp->width() + termp->width());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_परिणाम(resp);
    }

    व्योम visit(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        DfgVertex* स्थिर defaultp = m_defaultp;
        m_defaultp = nullptr;
        m_splicep = nullptr;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        स्थिर uint32_t idx = m_idxs.back();
        अगर (DfgVertex* स्थिर driverp = vtxp->driverAt(idx)) {
            DfgVertex* स्थिर srcp = driverp->as<DfgUnitArray>()->srcp();
            अगर (srcp->is<DfgVertexSplice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_परिणाम(trace_निकालो_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_परिणाम(traceSameIdx(defaultp));
    }

    व्योम visit(DfgVertexVar* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        DfgVertex* स्थिर srcp = vtxp->srcp();
        DfgVertex* स्थिर defaultp = vtxp->defaultp();
        DfgVertex* स्थिर drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        अगर (srcp && srcp->is<DfgVertexSplice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_परिणाम(m_idxs.empty() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    व्योम visit(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        अगर (स्थिर DfgConst* स्थिर idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_परिणाम(trace_धकेलो_Idx(vtxp->fromp(), idxp->toU32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.empty(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* स्थिर resp = make<DfgArraySel>(vtxp, vtxp->width());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->width() - 1, 0));
        DfgSel* स्थिर selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_परिणाम(selp);
    }

    व्योम visit(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.back() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_परिणाम(trace_निकालो_Idx(vtxp->srcp()));
    }

    व्योम visit(DfgConcat* vtxp) override {
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        स्थिर uint32_t rWidth = rhsp->width();
        // If the traced bits are wholly in the RHS
        अगर (rWidth > m_msb) RETURN_परिणाम_पुच्छ(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        अगर (m_lsb >= rWidth) RETURN_परिणाम_पुच्छ(lhsp, m_msb - rWidth, m_lsb - rWidth);
        // The traced bit spans both sides, trace both
        DfgConcat* स्थिर resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, rWidth - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - rWidth, 0));
        RETURN_परिणाम(resp);
    }

    व्योम visit(DfgRep* vtxp) override {
        DfgVertex* स्थिर srcp = vtxp->srcp();
        स्थिर uint32_t sWidth = srcp->width();
        // If we need more bits than the source, then we need the whole source
        अगर (m_msb - m_lsb + 1 > sWidth) {
            DfgRep* स्थिर repp = make<DfgRep>(vtxp, vtxp->width());
            repp->srcp(trace(srcp, sWidth - 1, 0));
            DfgSel* स्थिर resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_परिणाम(resp);
        }
        // If the requested bits are within the same repliacted word
        अगर (m_msb / sWidth == m_lsb / sWidth) {
            RETURN_परिणाम_पुच्छ(srcp, m_msb % sWidth, m_lsb % sWidth);
        }
        // The requested bits span two replicated words
        DfgConcat* स्थिर resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, sWidth - 1, m_lsb % sWidth));
        resp->lhsp(trace(srcp, m_msb % sWidth, 0));
        RETURN_परिणाम(resp);
    }

    व्योम visit(DfgExtend* vtxp) override {
        DfgVertex* स्थिर srcp = vtxp->srcp();
        स्थिर uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        अगर (sWidth > m_msb) RETURN_परिणाम_पुच्छ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        अगर (m_lsb >= sWidth) RETURN_परिणाम(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* स्थिर resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_परिणाम(resp);
    }

    व्योम visit(DfgExtendS* vtxp) override {
        DfgVertex* स्थिर srcp = vtxp->srcp();
        स्थिर uint32_t sWidth = srcp->width();
        // If the traced bits are wholly in the input
        अगर (sWidth > m_msb) RETURN_परिणाम_पुच्छ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        अगर (m_lsb >= sWidth) {
            अगर (m_msb == m_lsb) RETURN_परिणाम_पुच्छ(srcp, sWidth - 1, sWidth - 1);
            DfgExtendS* स्थिर resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, sWidth - 1, sWidth - 1));
            RETURN_परिणाम(resp);
        }
        // The traced bits span both sides
        DfgExtendS* स्थिर resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, sWidth - 1, m_lsb));
        RETURN_परिणाम(resp);
    }

    व्योम visit(DfgSel* vtxp) override {
        स्थिर uint32_t lsb = vtxp->lsb();
        RETURN_परिणाम_पुच्छ(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    व्योम visit(DfgNot* vtxp) override {
        DfgNot* स्थिर resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_परिणाम(resp);
    }

    व्योम visit(DfgAnd* vtxp) override { RETURN_परिणाम(trace_Bitwise_द्विआधारी(vtxp)); }
    व्योम visit(DfgOr* vtxp) override { RETURN_परिणाम(trace_Bitwise_द्विआधारी(vtxp)); }
    व्योम visit(DfgXor* vtxp) override { RETURN_परिणाम(trace_Bitwise_द्विआधारी(vtxp)); }
    व्योम visit(DfgAdd* vtxp) override { RETURN_परिणाम(traceAddSub(vtxp)); }
    व्योम visit(DfgSub* vtxp) override { RETURN_परिणाम(traceAddSub(vtxp)); }
    व्योम visit(DfgRedAnd* vtxp) override { RETURN_परिणाम(traceReduction(vtxp)); }
    व्योम visit(DfgRedOr* vtxp) override { RETURN_परिणाम(traceReduction(vtxp)); }
    व्योम visit(DfgRedXor* vtxp) override { RETURN_परिणाम(traceReduction(vtxp)); }
    व्योम visit(DfgEq* vtxp) override { RETURN_परिणाम(traceCmp(vtxp)); }
    व्योम visit(DfgNeq* vtxp) override { RETURN_परिणाम(traceCmp(vtxp)); }
    व्योम visit(DfgLt* vtxp) override { RETURN_परिणाम(traceCmp(vtxp)); }
    व्योम visit(DfgLte* vtxp) override { RETURN_परिणाम(traceCmp(vtxp)); }
    व्योम visit(DfgGt* vtxp) override { RETURN_परिणाम(traceCmp(vtxp)); }
    व्योम visit(DfgGte* vtxp) override { RETURN_परिणाम(traceCmp(vtxp)); }

    व्योम visit(DfgShiftRS* vtxp) override {
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        अगर (स्थिर DfgConst* स्थिर rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            स्थिर uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            स्थिर uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            अगर (lowerWidth > m_msb) {
                RETURN_परिणाम_पुच्छ(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            अगर (m_lsb >= lowerWidth) {
                DfgExtendS* स्थिर resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->width() - 1, lhsp->width() - 1));
                RETURN_परिणाम(resp);
            }
            // The traced bits span both sides
            DfgExtendS* स्थिर resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_परिणाम(resp);
        }

        DfgShiftRS* स्थिर shiftrsp = make<DfgShiftRS>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        अगर (m_msb == vtxp->lhsp()->width() - 1) RETURN_परिणाम(shiftrsp);
        DfgSel* स्थिर selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_परिणाम(selp);
    }

    व्योम visit(DfgShiftR* vtxp) override {
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        अगर (स्थिर DfgConst* स्थिर rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            स्थिर uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            स्थिर uint32_t lowerWidth = shiftAmnt > vtxp->width() ? 0 : vtxp->width() - shiftAmnt;
            // If the traced bits are wholly in the input
            अगर (lowerWidth > m_msb) {
                RETURN_परिणाम_पुच्छ(lhsp, m_msb + shiftAmnt, m_lsb + shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            अगर (m_lsb >= lowerWidth) RETURN_परिणाम(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* स्थिर resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, lowerWidth - 1 + shiftAmnt, m_lsb + shiftAmnt));
            RETURN_परिणाम(resp);
        }

        DfgShiftR* स्थिर shiftrp = make<DfgShiftR>(vtxp, vtxp->lhsp()->width() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, m_lsb));
        अगर (m_msb == vtxp->lhsp()->width() - 1) RETURN_परिणाम(shiftrp);
        DfgSel* स्थिर selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_परिणाम(selp);
    }

    व्योम visit(DfgShiftL* vtxp) override {
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        अगर (स्थिर DfgConst* स्थिर rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            स्थिर uint32_t shiftAmnt = rConstp->toU32();
            // Width of lower half of result
            स्थिर uint32_t lowerWidth = shiftAmnt > vtxp->width() ? vtxp->width() : shiftAmnt;
            // If the traced bits are wholly in the input
            अगर (m_lsb >= lowerWidth) {
                RETURN_परिणाम_पुच्छ(lhsp, m_msb - shiftAmnt, m_lsb - shiftAmnt);
            }
            // If the traced bits are wholly in the extension
            अगर (lowerWidth > m_msb) RETURN_परिणाम(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* स्थिर resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->width() - (m_msb - lowerWidth + 1)));
            resp->lhsp(trace(lhsp, m_msb - shiftAmnt, lowerWidth - shiftAmnt));
            RETURN_परिणाम(resp);
        }

        DfgShiftL* स्थिर shiftlp = make<DfgShiftL>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->width() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        अगर (m_lsb == 0) RETURN_परिणाम(shiftlp);
        DfgSel* स्थिर selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_परिणाम(selp);
    }

    व्योम visit(DfgCond* vtxp) override {
        DfgCond* स्थिर resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->width() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_परिणाम(resp);
    }

    व्योम visit(DfgMatchMasked* vtxp) override {
        DfgMatchMasked* स्थिर resp = make<DfgMatchMasked>(vtxp, vtxp->width());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->width() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->width() - 1, 0));
        DfgSel* स्थिर selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_परिणाम(selp);
    }

#अघोषित RETURN_परिणाम
#अघोषित TRACE_परिणाम_COVER
#अघोषित RETURN_परिणाम_पुच्छ

खुला:
    // CONSTRUCTOR
    TraceDriver(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#अगर_घोषित VL_DEBUG
        अगर (v3Global.opt.debugCheck()) {
            m_पंक्ति_Coverage_File.खोलो(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::नत्थी);
        }
#पूर्ण_अगर
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    DfgVertex* apply(DfgVertex& vtx, uint32_t lsb, uint32_t width) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.लो(vtx);
        वापस trace(&vtx, lsb + width - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
श्रेणी BitMask final {
    // TYPES
    स्थिर Dfg_तथ्य_Type& m_dtype;
    जोड़ {
        V3_संख्या m_अंक;  // The bits, if packed
        std::vector<BitMask> m_sub;  // The per element masks, if aggregate
    };

खुला:
    // CONSTRUCTOR
    BitMask(File_पंक्ति* flp, स्थिर Dfg_तथ्य_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        अगर (m_dtype.isPacked()) {
            नया (&m_अंक) V3_संख्या{flp, static_cast<पूर्णांक>(m_dtype.आकार()), 0};
        } अन्यथा {
            नया (&m_sub) std::vector<BitMask>{};
            m_sub.reserve(m_dtype.आकार());
            क्रम (uint32_t i = 0; i < m_dtype.आकार(); ++i)
                m_sub.emplace_back(flp, m_dtype.elemDtype());
        }
    }
    BitMask(स्थिर DfgVertex& vtx)
        : BitMask{vtx.fileline(), vtx.dtype()} {}
    BitMask(स्थिर BitMask& other)
        : m_dtype{other.m_dtype} {
        अगर (m_dtype.isPacked()) {
            नया (&m_अंक) V3_संख्या{other.m_अंक};
        } अन्यथा {
            नया (&m_sub) std::vector<BitMask>{other.m_sub};
        }
    }
    BitMask& चालक=(स्थिर BitMask& other) {
        अगर (यह != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            अगर (other.m_dtype.isPacked()) {
                m_अंक = other.m_अंक;
            } अन्यथा {
                m_sub = other.m_sub;
            }
        }
        वापस *यह;
    }
    ~BitMask() {
        अगर (m_dtype.isPacked()) {
            m_अंक.~V3_संख्या();
        } अन्यथा {
            m_sub.~vector<BitMask>();
        }
    }

    // METHODS
    V3_संख्या& अंक() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        वापस m_अंक;
    }
    स्थिर V3_संख्या& अंक() स्थिर {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        वापस m_अंक;
    }
    std::vector<BitMask>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        वापस m_sub;
    }

    bool isZero() स्थिर {
        अगर (m_dtype.isPacked()) वापस m_अंक.isEqZero();

        क्रम (स्थिर BitMask& sub : m_sub) {
            अगर (!sub.isZero()) वापस false;
        }
        वापस true;
    }
    bool isOnes() स्थिर {
        अगर (m_dtype.isPacked()) वापस m_अंक.isEqAllOnes();

        क्रम (स्थिर BitMask& sub : m_sub) {
            अगर (!sub.isOnes()) वापस false;
        }
        वापस true;
    }
    व्योम setZero() {
        अगर (m_dtype.isPacked()) {
            m_अंक.setAllBits0();
        } अन्यथा {
            क्रम (BitMask& sub : m_sub) sub.setZero();
        }
    }
    व्योम setOnes() {
        अगर (m_dtype.isPacked()) {
            m_अंक.setAllBits1();
        } अन्यथा {
            क्रम (BitMask& sub : m_sub) sub.setOnes();
        }
    }

    bool चालक==(स्थिर BitMask& other) स्थिर {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        अगर (m_dtype.isPacked()) वापस m_अंक.isCaseEq(other.m_अंक);
        वापस m_sub == other.m_sub;
    }
    bool चालक!=(स्थिर BitMask& other) स्थिर { वापस !(*यह == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    bool isContractionOf(स्थिर BitMask& that) स्थिर {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        अगर (m_dtype.isPacked()) {
            स्थिर माप_प्रकार शब्दगण = VL_शब्दगण_I(m_dtype.आकार());
            क्रम (माप_प्रकार i = 0; i < शब्दगण; ++i) {
                अगर (~m_अंक.edataWord(i) & that.m_अंक.edataWord(i)) वापस true;
            }
            वापस false;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        क्रम (माप_प्रकार i = 0; i < m_sub.आकार(); ++i) {
            अगर (m_sub[i].isContractionOf(that.m_sub[i])) वापस true;
        }
        वापस false;
    }

    std::string toString() स्थिर {
        अगर (m_dtype.isPacked()) वापस "0x" + m_अंक.displayed(m_अंक.fileline(), "%x");
        std::string परिणाम;
        परिणाम += "{";
        क्रम (स्थिर BitMask& sub : m_sub) {
            अगर (&sub != &m_sub.front()) परिणाम += ", ";
            परिणाम += sub.toString();
        }
        परिणाम += "}";
        वापस परिणाम;
    }
};

श्रेणी IndependentBits final : खुला DfgVisitor {
    // TYPES
    काष्ठा VertexState final {
        माप_प्रकार m_rpo_संख्या = 0;  // Reverse Postorder Number of vertex
        bool m_isOnWorkList = false;  // Whether the vertex is on the work list
    };

    // STATE
    स्थिर SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<स्थिर DfgVertex*, BitMask> m_vtxp2Mask;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::priority_queue<माप_प्रकार, std::vector<माप_प्रकार>, std::greater<माप_प्रकार>> m_workQueue;
    std::unordered_map<स्थिर DfgVertex*, VertexState> m_vtxp2State;  // State of each vertex

#अगर_घोषित VL_DEBUG
    std::बख_प्रवाह m_पंक्ति_Coverage_File;  // Line coverage file, just for testing
#पूर्ण_अगर

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    BitMask& mask(स्थिर DfgVertex& vtx) {
        // Look up (or create) mask for 'vtx'
        वापस m_vtxp2Mask
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .first->second;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#अगर_घोषित VL_DEBUG
#घोषणा MASK(vtxp) \
    ([यह](स्थिर DfgVertex& vtx) -> BitMask& { \
        अगर (VL_UNLIKELY(m_पंक्ति_Coverage_File.is_open())) m_पंक्ति_Coverage_File << __LINE__ << '\n'; \
        वापस mask(vtx); \
    }(*vtxp))
#अन्यथा
#घोषणा MASK(vtxp) mask(*vtxp)
#पूर्ण_अगर

    // Clear all bits at or below the most significant clear bit
    व्योम floodTowardsLsb(V3_संख्या& अंक) {
        क्रम (पूर्णांक i = अंक.width() - 1; i >= 0; --i) {
            अगर (अंक.bitIs1(i)) जारी;
            अंक.opSetRange(0, i + 1, '0');
            अवरोध;
        }
    }

    // Clear all bits at or above the least significant clear bit
    व्योम floodTowardsMsb(V3_संख्या& अंक) {
        क्रम (पूर्णांक i = 0; i < अंक.width(); ++i) {
            अगर (अंक.bitIs1(i)) जारी;
            अंक.opSetRange(i, अंक.width() - i, '0');
            अवरोध;
        }
    }

    व्योम propagateFromDriver(BitMask& m, स्थिर DfgVertex* srcp) {
        // If there is no driver, we are done
        अगर (!srcp) वापस;
        // If it is driven by a splice, we need to combine the masks of the drivers
        अगर (स्थिर DfgSplicePacked* स्थिर splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](स्थिर DfgVertex& src, uint32_t lo) {
                m.अंक().opSelInto(MASK(&src).अंक(), lo, src.width());
                वापस false;
            });
            वापस;
        }
        अगर (स्थिर DfgSpliceArray* स्थिर splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](स्थिर DfgVertex& src, uint32_t lo) {
                अगर (स्थिर DfgUnitArray* स्थिर uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = MASK(uap->srcp());
                } अन्यथा {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                वापस false;
            });
            वापस;
        }
        // Otherwise, we just use the mask of the single driver
        m = MASK(srcp);
    }

    // VISITORS
    व्योम visit(DfgVertex* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    व्योम visit(DfgVertexVar* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        अगर (vtxp->isVolatile()) वापस;

        BitMask& m = MASK(vtxp);
        DfgVertex* स्थिर srcp = vtxp->srcp();
        DfgVertex* स्थिर defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        अगर (defaultp) m = MASK(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    व्योम visit(DfgVertexSplice* vtxp) override {
        propagateFromDriver(MASK(vtxp), vtxp);  // Needed to continue traversal
    }

    व्योम visit(DfgUnitArray* vtxp) override { MASK(vtxp).sub().at(0) = MASK(vtxp->srcp()); }

    व्योम visit(DfgArraySel* vtxp) override {
        DfgVertex* स्थिर fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        अगर (स्थिर DfgConst* स्थिर idxp = vtxp->bitp()->cast<DfgConst>()) {
            MASK(vtxp) = MASK(vtxp->fromp()).sub().at(idxp->to_आकार_T());
            वापस;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        अगर (MASK(vtxp->bitp()).isOnes() && MASK(fromp).isOnes()) MASK(vtxp).setOnes();
    }

    व्योम visit(DfgConcat* vtxp) override {
        स्थिर DfgVertex* स्थिर rhsp = vtxp->rhsp();
        स्थिर DfgVertex* स्थिर lhsp = vtxp->lhsp();
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opSelInto(MASK(rhsp).अंक(), 0, rhsp->width());
        m.opSelInto(MASK(lhsp).अंक(), rhsp->width(), lhsp->width());
    }

    व्योम visit(DfgRep* vtxp) override {
        स्थिर uint32_t गिनती = vtxp->गिनती();
        स्थिर DfgVertex* स्थिर srcp = vtxp->srcp();
        स्थिर uint32_t sWidth = srcp->width();
        V3_संख्या& vMask = MASK(vtxp).अंक();
        V3_संख्या& sMask = MASK(srcp).अंक();
        क्रम (uint32_t i = 0; i < गिनती; ++i) vMask.opSelInto(sMask, i * sWidth, sWidth);
    }

    व्योम visit(DfgSel* vtxp) override {
        स्थिर uint32_t lsb = vtxp->lsb();
        स्थिर uint32_t msb = lsb + vtxp->width() - 1;
        MASK(vtxp).अंक().opSel(MASK(vtxp->fromp()).अंक(), msb, lsb);
    }

    व्योम visit(DfgExtend* vtxp) override {
        स्थिर DfgVertex* स्थिर srcp = vtxp->srcp();
        स्थिर uint32_t sWidth = srcp->width();
        V3_संख्या& s = MASK(srcp).अंक();
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, '1');
    }
    व्योम visit(DfgExtendS* vtxp) override {
        स्थिर DfgVertex* स्थिर srcp = vtxp->srcp();
        स्थिर uint32_t sWidth = srcp->width();
        V3_संख्या& s = MASK(srcp).अंक();
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opSelInto(s, 0, sWidth);
        m.opSetRange(sWidth, vtxp->width() - sWidth, s.bitIs0(sWidth - 1) ? '0' : '1');
    }

    व्योम visit(DfgNot* vtxp) override {  //
        MASK(vtxp) = MASK(vtxp->srcp());
    }

    व्योम visit(DfgAnd* vtxp) override {  //
        MASK(vtxp).अंक().opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
    }
    व्योम visit(DfgOr* vtxp) override {  //
        MASK(vtxp).अंक().opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
    }
    व्योम visit(DfgXor* vtxp) override {  //
        MASK(vtxp).अंक().opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
    }

    व्योम visit(DfgAdd* vtxp) override {
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
        floodTowardsMsb(m);
    }
    व्योम visit(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_संख्या& m = MASK(vtxp).अंक();
        m.opAnd(MASK(vtxp->lhsp()).अंक(), MASK(vtxp->rhsp()).अंक());
        floodTowardsMsb(m);
    }

    व्योम visit(DfgRedAnd* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }
    व्योम visit(DfgRedOr* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }
    व्योम visit(DfgRedXor* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }

    व्योम visit(DfgEq* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }
    व्योम visit(DfgNeq* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }
    व्योम visit(DfgLt* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }
    व्योम visit(DfgLte* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }
    व्योम visit(DfgGt* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }
    व्योम visit(DfgGte* vtxp) override {
        स्थिर bool independent = MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->rhsp()).isOnes();
        अगर (independent) MASK(vtxp).setOnes();
    }

    व्योम visit(DfgShiftRS* vtxp) override {
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        स्थिर uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        अगर (DfgConst* स्थिर rConstp = rhsp->cast<DfgConst>()) {
            स्थिर uint32_t shiftAmount = rConstp->toU32();
            अगर (shiftAmount >= width) {
                अगर (MASK(lhsp).अंक().bitIs1(width - 1)) { MASK(vtxp).setOnes(); }
            } अन्यथा {
                V3_संख्या& m = MASK(vtxp).अंक();
                m.opShiftRS(MASK(lhsp).अंक(), rConstp->अंक(), width);
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            वापस;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        अगर (MASK(rhsp).isOnes()) {
            V3_संख्या& m = MASK(vtxp).अंक();
            m = MASK(lhsp).अंक();
            floodTowardsLsb(m);
        }
    }

    व्योम visit(DfgShiftR* vtxp) override {
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        स्थिर uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        अगर (DfgConst* स्थिर rConstp = rhsp->cast<DfgConst>()) {
            स्थिर uint32_t shiftAmount = rConstp->toU32();
            अगर (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } अन्यथा {
                V3_संख्या& m = MASK(vtxp).अंक();
                m.opShiftR(MASK(lhsp).अंक(), rConstp->अंक());
                m.opSetRange(width - shiftAmount, shiftAmount, '1');
            }
            वापस;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        अगर (MASK(rhsp).isOnes()) {
            V3_संख्या& m = MASK(vtxp).अंक();
            m = MASK(lhsp).अंक();
            floodTowardsLsb(m);
        }
    }

    व्योम visit(DfgShiftL* vtxp) override {
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        स्थिर uint32_t width = vtxp->width();

        // Constant shift can be computed precisely
        अगर (DfgConst* स्थिर rConstp = rhsp->cast<DfgConst>()) {
            स्थिर uint32_t shiftAmount = rConstp->toU32();
            अगर (shiftAmount >= width) {
                MASK(vtxp).setOnes();
            } अन्यथा {
                V3_संख्या& m = MASK(vtxp).अंक();
                m.opShiftL(MASK(lhsp).अंक(), rConstp->अंक());
                m.opSetRange(0, shiftAmount, '1');
            }
            वापस;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        अगर (MASK(rhsp).isOnes()) {
            V3_संख्या& m = MASK(vtxp).अंक();
            m = MASK(lhsp).अंक();
            floodTowardsMsb(m);
        }
    }

    व्योम visit(DfgCond* vtxp) override {
        अगर (MASK(vtxp->condp()).isOnes()) {
            MASK(vtxp).अंक().opAnd(MASK(vtxp->thenp()).अंक(), MASK(vtxp->elsep()).अंक());
        }
    }

    व्योम visit(DfgMatchMasked* vtxp) override {
        अगर (MASK(vtxp->lhsp()).isOnes() && MASK(vtxp->matchp()).isOnes()) { MASK(vtxp).setOnes(); }
    }

#अघोषित MASK

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    व्योम enqueueSinks(DfgVertex& vtx) {
        vtx.foreachSink([&](DfgVertex& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            अगर (!m_sccInfo.लो(sink)) वापस false;
            // Otherwise just enqueue it
            VertexState& state = m_vtxp2State.at(&sink);
            अगर (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_workQueue.धकेलो(state.m_rpo_संख्या);
            }
            state.m_isOnWorkList = true;
            वापस false;
        });
    };

    व्योम dfsPostOrder(std::unordered_set<DfgVertex*>& visited,  //
                      std::vector<DfgVertex*>& postOrderEnumeration,  //
                      DfgVertex& vtx) {
        // Mark visited, skip if already visited
        अगर (!visited.जोड़ो(&vtx).second) वापस;
        // Visit un-visited successors
        vtx.foreachSink([&](DfgVertex& snk) {
            dfsPostOrder(visited, postOrderEnumeration, snk);
            वापस false;
        });
        // Add to post order enumeration
        postOrderEnumeration.emplace_back(&vtx);
    };

    IndependentBits(DfgGraph& dfg, स्थिर SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#अगर_घोषित VL_DEBUG
        अगर (v3Global.opt.debugCheck()) {
            m_पंक्ति_Coverage_File.खोलो(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.prefix()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_base::out | std::ios_base::नत्थी);
        }
#पूर्ण_अगर

        // Compute Reverse-Postorder enumeration of vertices
        std::vector<DfgVertex*> rpoEnumeration;
        std::unordered_set<DfgVertex*> visited;
        dfg.forEachVertex([&](DfgVertex& vtx) {
            अगर (!vtx.nInputs()) dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        dfg.forEachVertex([&](DfgVertex& vtx) {  //
            dfsPostOrder(visited, rpoEnumeration, vtx);
        });
        std::reverse(rpoEnumeration.begin(), rpoEnumeration.end());
        UASSERT(rpoEnumeration.आकार() == dfg.आकार(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        क्रम (माप_प्रकार i = 0; i < rpoEnumeration.आकार(); ++i) {
            VertexState& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_संख्या = i;
            state.m_isOnWorkList = false;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        क्रम (DfgVertex* स्थिर vtxp : rpoEnumeration) {
            अगर (m_sccInfo.लो(*vtxp)) जारी;
            mask(*vtxp).setOnes();
        }
        क्रम (DfgVertex* स्थिर vtxp : rpoEnumeration) {
            अगर (!m_sccInfo.लो(*vtxp)) जारी;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << mask(*vtxp).toString());
            अगर (!mask(*vtxp).isZero()) enqueueSinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        जबतक (!m_workQueue.empty()) {
            // Grab next item
            DfgVertex* स्थिर currp = rpoEnumeration[m_workQueue.शीर्ष()];
            m_workQueue.निकालो();
            m_vtxp2State.at(currp).m_isOnWorkList = false;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.लो(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            स्थिर BitMask& currMask = mask(*currp);
            // Remember current mask so we can check if it changed
            स्थिर BitMask पूर्व_Mask = currMask;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            अगर (पूर्व_Mask != currMask) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << पूर्व_Mask.toString()
                             << "\n      to: " << currMask.toString());
                enqueueSinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                अगर (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!currMask.isContractionOf(पूर्व_Mask), currp,
                                "Mask should only expand");
                }
            }
        }
    }

खुला:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    अटल std::unordered_map<स्थिर DfgVertex*, BitMask> apply(DfgGraph& dfg,
                                                               स्थिर SccInfo& sccInfo) {
        वापस std::move(IndependentBits{dfg, sccInfo}.m_vtxp2Mask);
    }
};

श्रेणी FixUp final {
    DfgGraph& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    स्थिर std::unordered_map<स्थिर DfgVertex*, BitMask> m_independentBits
        = IndependentBits::apply(m_dfg, m_sccInfo);
    माप_प्रकार m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    अटल BitMask computeUsedBits(DfgVertex& vtx) {
        BitMask परिणाम{vtx};
        vtx.foreachSink([&परिणाम](DfgVertex& sink) {
            // If used via a Sel, mark the selected bits used
            अगर (स्थिर DfgSel* स्थिर selp = sink.cast<DfgSel>()) {
                परिणाम.अंक().opSetRange(selp->lsb(), selp->width(), '1');
                वापस false;
            }
            // Used without a Sel, so all bits are used
            परिणाम.setOnes();
            वापस true;
        });
        वापस परिणाम;
    }

    अटल std::string debugStr(स्थिर DfgVertex& vtx) {  // LCOV_EXCL_START
        अगर (स्थिर DfgArraySel* स्थिर aselp = vtx.cast<DfgArraySel>()) {
            स्थिर माप_प्रकार i = aselp->bitp()->as<DfgConst>()->to_आकार_T();
            वापस debugStr(*aselp->fromp()) + "[" + std::to_string(i) + "]";
        }
        अगर (स्थिर DfgVertexVar* स्थिर varp = vtx.cast<DfgVertexVar>()) {
            वापस varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    व्योम gatherTerms(std::vector<DfgVertex*>& termps, DfgVertex& vtx, स्थिर V3_संख्या& indpBits,
                     स्थिर uint32_t hi, स्थिर uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        bool isIndependent = indpBits.bitIs1(lo);
        uint32_t lsb = lo;
        क्रम (uint32_t msb = lo; msb <= hi; ++msb) {
            स्थिर bool endRange = msb == hi || isIndependent != indpBits.bitIs1(msb + 1);
            अगर (!endRange) जारी;
            स्थिर uint32_t width = msb - lsb + 1;
            // Compute the term to use for this range
            अगर (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_back(m_traceDriver.apply(vtx, lsb, width));
            } अन्यथा {
                // If dependent, fall back on using the part of the variable
                DfgSel* स्थिर selp = नया DfgSel{m_dfg, vtx.fileline(), Dfg_तथ्य_Type::packed(width)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.add(*selp, m_sccInfo.लो(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_back(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    व्योम fixUpPacked(DfgVertex& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        स्थिर BitMask& indpBits = m_independentBits.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indpBits.toString());
        // Can't do anything if all bits are dependent
        अगर (indpBits.isZero()) वापस;

        // Figure out which bits of 'vtxp' are used
        स्थिर BitMask usedBits = computeUsedBits(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << usedBits.toString());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_संख्या usedAndIndependent{vtx.fileline(), static_cast<पूर्णांक>(vtx.width()), 0};
        usedAndIndependent.opAnd(usedBits.अंक(), indpBits.अंक());
        अगर (usedAndIndependent.isEqZero()) वापस;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::vector<DfgVertex*> termps;

        // Iterate through consecutive used/unused ranges
        File_पंक्ति* स्थिर flp = vtx.fileline();
        स्थिर uint32_t width = vtx.width();
        bool isUsed = usedBits.अंक().bitIs1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        क्रम (uint32_t msb = 0; msb < width; ++msb) {
            स्थिर bool endRange = msb == width - 1 || isUsed != usedBits.अंक().bitIs1(msb + 1);
            अगर (!endRange) जारी;
            अगर (isUsed) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indpBits.अंक(), msb, lsb);
            } अन्यथा {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* स्थिर constp = नया DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.add(*constp, 0);
                termps.emplace_back(constp);
            }
            // Next iteration
            isUsed = !isUsed;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        DfgVertex* replacementp = termps.front();
        स्थिर uint64_t vComp = m_sccInfo.लो(vtx);
        क्रम (माप_प्रकार i = 1; i < termps.आकार(); ++i) {
            DfgVertex* स्थिर termp = termps[i];
            स्थिर uint32_t catWidth = replacementp->width() + termp->width();
            DfgConcat* स्थिर catp = नया DfgConcat{m_dfg, flp, Dfg_तथ्य_Type::packed(catWidth)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            स्थिर uint64_t tComp = m_sccInfo.लो(*termp);
            स्थिर uint64_t rComp = m_sccInfo.लो(*replacementp);
            m_sccInfo.add(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        क्रम (DfgVertex* स्थिर termp : termps) {
            अगर (DfgSel* स्थिर selp = termp->cast<DfgSel>()) {
                अगर (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.लो(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        अगर (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    व्योम मुख्य(DfgVertexVar& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        अगर (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fixUpPacked(var);
        } अन्यथा अगर (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            स्थिर uint64_t component = m_sccInfo.लो(var);
            var.foreachSink([&](DfgVertex& sink) {
                // Ignore if sink is not part of same cycle
                अगर (m_sccInfo.लो(sink) != component) वापस false;
                // Only handle ArraySels with constant index
                DfgArraySel* स्थिर aselp = sink.cast<DfgArraySel>();
                अगर (!aselp) वापस false;
                अगर (!aselp->bitp()->is<DfgConst>()) वापस false;
                // Fix up the word
                fixUpPacked(*aselp);
                वापस false;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    FixUp(DfgGraph& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

खुला:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    अटल माप_प्रकार apply(DfgGraph& dfg, SccInfo& sccInfo) {
        अगर (!sccInfo.isCyclic()) वापस 0;

        FixUp fixUp{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        क्रम (DfgVertexVar& vtx : dfg.varVertices()) {
            अगर (!sccInfo.लो(vtx)) जारी;
            fixUp.मुख्य(vtx);
            अगर (!sccInfo.isCyclic()) अवरोध;
        }
        // Return the number of improvements made
        वापस fixUp.m_nImprovements;
    }
};

व्योम breakCycles(DfgGraph& dfg, V3DfgBreakCyclesContext& ctx) {
    // Shorthand for dumping graph at given dump level
    स्थिर स्वतः dump = [&](पूर्णांक level, स्थिर DfgGraph& dfg, स्थिर std::string& name) {
        अगर (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    माप_प्रकार nImprovements = 0;
    माप_प्रकार पूर्व_NImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    करो {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        पूर्व_NImprovements = nImprovements;
        स्थिर माप_प्रकार nFixed = FixUp::apply(dfg, sccInfo);
        अगर (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        अगर (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        अगर (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            वापस;
        }
    } जबतक (nImprovements != पूर्व_NImprovements);

    // Debug dump
    अगर (dumpDfgLevel() >= 9) {
        स्थिर SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](स्थिर DfgVertex& vtx) {
            वापस sccInfo.लो(vtx);  //
        });
    }

    // Accounting
    अगर (nImprovements) {
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
    क्रम (DfgVertexVar& vtx : dfg.varVertices()) {
        अगर (!vtx.tmpForp()) varps.धकेलो_back(&vtx);
    }
    std::stable_partition(varps.begin(), varps.end(), [](स्थिर DfgVertexVar* varp) {
        वापस varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    क्रम (DfgVertexVar* स्थिर varp : varps) {
        // Stop as soon as it becomes acyclic
        अगर (!sccInfo.isCyclic()) अवरोध;

        // Ignore if variable is not part of a cycle
        अगर (!sccInfo.लो(*varp)) जारी;

        // Insert a DfgPrev vertex for the variable
        Dfg_पूर्व* स्थिर prevp = नया Dfg_पूर्व{dfg, varp->vscp()};
        sccInfo.add(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_n_पूर्व_Inserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    अगर (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
व्योम V3DfgPasses::breakCycles(DfgGraph& dfg, V3DfgContext& ctx) {
    V3DfgBreakCycles::breakCycles(dfg, ctx.m_breakCyclesContext);
    अगर (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::removeUnused(dfg);
}
