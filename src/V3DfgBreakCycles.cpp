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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3Error.h"
#ضمن "V3Hash.h"

#ضمن <algorithm>
#ضمن <fstream>
#ضمن <queue>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

نطاق V3_Dfg_Break_دورې {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
صنف SccInfo final {
    Dfg_ګراف& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ثابت Dfg_راس*, uint64_t> m_vtx2Scc;
    اندازه_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

عام:
    SccInfo(Dfg_ګراف& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) {
            ثابت uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            إذا (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    منطقي isCyclic() ثابت { أرجع m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ثابت Dfg_راس& vtx) ثابت { أرجع m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    فراغ جمع_کړه(ثابت Dfg_راس& vtx, uint64_t sccIdx) {
        ثابت منطقي new_ثبت = m_vtx2Scc.emplace(&vtx, sccIdx).دوهم;
        UASSERT_OBJ(new_ثبت, &vtx, "Vertex already inserted");
        إذا (sccIdx) ++m_nCyclicVertices;
    }

    منطقي stillCyclicFwd(ثابت Dfg_راس& vtx) ثابت {
        // If it only reads non-cyclic vertices, it has also become acyclic
        أرجع vtx.foreach_سرچینه([&](ثابت Dfg_راس& src) {  //
            أرجع m_vtx2Scc.at(&src);
        });
    }

    منطقي stillCyclicBwd(ثابت Dfg_راس& vtx) ثابت {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        أرجع vtx.foreachSink([&](ثابت Dfg_راس& dst) {  //
            أرجع m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    فراغ updateAcyclic(ثابت Dfg_راس& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ثابت Dfg_راس& dst) {
            // Sink was known to be acyclic, stop
            إذا (!m_vtx2Scc.at(&dst)) أرجع خطأ;
            إذا (!stillCyclicFwd(dst)) updateAcyclic(dst);
            أرجع خطأ;
        });
        // Propagate the update through the graph backward
        vtx.foreach_سرچینه([&](ثابت Dfg_راس& src) {
            // Source was known to be acyclic, stop
            إذا (!m_vtx2Scc.at(&src)) أرجع خطأ;
            إذا (!stillCyclicBwd(src)) updateAcyclic(src);
            أرجع خطأ;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    فراغ updateReplacement(ثابت Dfg_راس& vtx, Dfg_راس& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        إذا (!get(replacement)) {
            replacement.foreachSink([&](Dfg_راس& dst) {
                إذا (!get(dst)) أرجع خطأ;
                إذا (!stillCyclicFwd(dst)) updateAcyclic(dst);
                أرجع خطأ;
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
    فراغ validate() ثابت {
        // Re-compute the actual SCCs
        DfgUserMap<uint64_t> actual = m_dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(m_dfg, actual);
        // Assert that if we think a vertex is not cyclic, it indeed is not
        m_dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

صنف TraceDriver final : عام DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    بنية Cache_کلی final {
        Dfg_راس* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_کلی() = ړنګول;
        Cache_کلی(Dfg_راس* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        بنية Hash final {
            اندازه_t چلوونکی()(ثابت Cache_کلی& توکی_2) ثابت {
                // cppcheck-suppress unreadVariable
                V3Hash hash{توکی_2.m_vtxp};
                hash += توکی_2.m_lsb;
                hash += توکی_2.m_msb;
                أرجع hash.ارزښت();
            }
        };

        بنية Equal final {
            منطقي چلوونکی()(ثابت Cache_کلی& a, ثابت Cache_کلی& b) ثابت {
                أرجع a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
            }
        };
    };

    // STATE
    Dfg_ګراف& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    // The strongly connected component we are currently trying to escape
    uint64_t m_component = 0;
    uint32_t m_lsb = 0;  // LSB to extract from the currently visited Vertex
    uint32_t m_msb = 0;  // MSB to extract from the currently visited Vertex
    std::ویکټور<uint32_t> m_idxs;  // Indices to extract from the currently visited Vertex
    std::ویکټور<Dfg_راس**> m_pendingps;  // Pending pass-through cache entries across trace calls
    // Result of tracing the currently visited Vertex. Use RETURN_RESULT below!
    Dfg_راس* m_resp = تش_اشاره;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    Dfg_راس* m_tailp = تش_اشاره;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    Dfg_راس* m_defaultp = تش_اشاره;
    Dfg_راس* m_splicep = تش_اشاره;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_کلی, Dfg_راس*, Cache_کلی::Hash, Cache_کلی::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_کلی, Dfg_راس*, Cache_کلی::Hash, Cache_کلی::Equal> m_boundaryCache;

#که_تعریف_شوی VL_DEBUG
    std::ofstream m_کرښه_Coverage_File;  // Line coverage file, just for testing
#د_که_پای

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    کينډۍ <ډول_نوم راس_2>
    راس_2* make(ثابت Dfg_راس* refp, uint32_t پلنوالی) {
        static_assert(std::is_بنسټ_د<Dfg_راس, راس_2>::ارزښت  //
                          && !std::is_بنسټ_د<Dfg_راس_Var, راس_2>::ارزښت,
                      "Should only make operation vertices and constants");

        إذا VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, راس_2>::ارزښت) {
            DfgConst* ثابت vtxp = جديد DfgConst{m_dfg, refp->fileline(), پلنوالی, 0};
            m_sccInfo.جمع_کړه(*vtxp, 0);
            أرجع reinterpret_cast<راس_2*>(vtxp);
        } وإلا {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            کارول Vtx = ډول_نوم std::conditional<std::is_same<DfgConst, راس_2>::ارزښت, DfgSel,
                                                  راس_2>::type;
            Vtx* ثابت vtxp = جديد Vtx{m_dfg, refp->fileline(), Dfg_معلومات_Type::packed(پلنوالی)};
            m_sccInfo.جمع_کړه(*vtxp, 0);
            أرجع reinterpret_cast<راس_2*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    Dfg_راس_Var* createTmp(ثابت حرف* مختاړی, Dfg_راس* vtxp) {
        File_کرښه* ثابت flp = vtxp->fileline();
        Dfg_راس::ScopeCache scopeCache;
        AstScope* ثابت scopep = vtxp->scopep(scopeCache);
        Dfg_راس_Var* ثابت varp = m_dfg.makeNewVar(flp, مختاړی, vtxp->dtype(), scopep);
        varp->vscp()->varp()->is_دننی(ریښتیا);
        varp->tmpForp(varp->vscp());
        m_sccInfo.جمع_کړه(*varp, 0);
        أرجع varp;
    }

    // Trace drivers of the given packed vertex, at the given bit range.
    Dfg_راس* trace(Dfg_راس* vtxp, uint32_t msb, uint32_t lsb) {
        // Many vertices just pass the traced bit range through to one of their sources
        // (see RETURN_RESULT_TAIL). Chains of such vertices can be tens of thousands of
        // vertices long in large designs (e.g.: a wide concatenation), so resolve them
        // in this loop instead of by recursion, which would overflow the C++ stack.
        // 'm_pendingps' holds the cache entries of the vertices on the current chain. They
        // all yield the same result, which is filled in when the chain is resolved.
        // Note: references into 'm_cache' are stable across the insertions below.
        Dfg_راس* ثابت پیل_Vtxp = vtxp;
        ثابت uint32_t پیل_Msb = msb;
        ثابت uint32_t پیل_Lsb = lsb;
        ثابت اندازه_t pendingBegin = m_pendingps.اندازه();
        Dfg_راس* resp = تش_اشاره;

        طالما (ریښتیا) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->اندازه() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            إذا (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                Dfg_راس*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(تش_اشاره))
                          .لومړی->دوهم;
                إذا (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    إذا (Dfg_راس_Splice* ثابت splicep = respr->cast<Dfg_راس_Splice>()) {
                        Dfg_راس_Var* ثابت tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    إذا (msb != respr->پلنوالی() - 1 || lsb != 0) {
                        DfgSel* ثابت selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                اكسر;
            }

            // Normal cache for intra-component traces
            Dfg_راس*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(تش_اشاره))
                                    .لومړی->دوهم;

            // Trace the vertex
            إذا (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } وإلا {
                // Otherwise visit the vertex to trace it
                VL_RESTORER(m_msb);
                VL_RESTORER(m_lsb);
                VL_RESTORER_CLEAR(m_idxs);
                VL_RESTORER(m_resp);
                VL_RESTORER(m_tailp);
                m_msb = msb;
                m_lsb = lsb;
                m_resp = تش_اشاره;
                m_tailp = تش_اشاره;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != تش_اشاره) != (m_tailp != تش_اشاره), vtxp,
                            "Expected exactly one trace result");
                إذا (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ټېل_شا(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    استمر;
                }
                respr = m_resp;
            }
            resp = respr;
            اكسر;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, پیل_Vtxp, "Tracing driver failed for " << پیل_Vtxp->typeName());
        UASSERT_OBJ(resp->پلنوالی() == (پیل_Msb - پیل_Lsb + 1), پیل_Vtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        لكل (اندازه_t i = pendingBegin; i < m_pendingps.اندازه(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        أرجع resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    Dfg_راس* traceSameIdx(Dfg_راس* ثابت vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.تشه(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = تش_اشاره;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->پلنوالی() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        أرجع m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    Dfg_راس* trace_ټېل_Idx(Dfg_راس* ثابت vtxp, ثابت uint32_t idx) {
        ثابت اندازه_t nIdxs = m_idxs.اندازه();
        m_idxs.ټېل_شا(idx);
        Dfg_راس* ثابت resp = traceSameIdx(vtxp);
        m_idxs.راوباسه_شا();
        UASSERT_OBJ(m_idxs.اندازه() == nIdxs, vtxp, "Index stack size mismatch");
        أرجع resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    Dfg_راس* trace_راوباسه_Idx(Dfg_راس* ثابت vtxp) {
        UASSERT_OBJ(!m_idxs.تشه(), vtxp, "Should have a pending index");
        ثابت اندازه_t nIdxs = m_idxs.اندازه();
        ثابت uint32_t idx = m_idxs.شا();
        m_idxs.راوباسه_شا();
        Dfg_راس* ثابت resp = m_idxs.تشه() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ټېل_شا(idx);
        UASSERT_OBJ(m_idxs.اندازه() == nIdxs, vtxp, "Index stack size mismatch");
        أرجع resp;
    }

    کينډۍ <ډول_نوم راس_2>
    راس_2* trace_Bitwise_دوګونی(راس_2* vtxp) {
        static_assert(std::is_بنسټ_د<Dfg_راس_دوګونی, راس_2>::ارزښت,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت resp = make<راس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        أرجع resp;
    }

    کينډۍ <ډول_نوم راس_2>
    Dfg_راس* trace_جمع_کړه_Sub(راس_2* vtxp) {
        static_assert(std::is_بنسټ_د<Dfg_راس_دوګونی, راس_2>::ارزښت,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت opp = make<راس_2>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        أرجع selp;
    }

    کينډۍ <ډول_نوم راس_2>
    راس_2* traceReduction(راس_2* vtxp) {
        static_assert(std::is_بنسټ_د<Dfg_راس_Unary, راس_2>::ارزښت,
                      "Should only call on DfgVertexUnary");
        راس_2* ثابت resp = make<راس_2>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->پلنوالی() - 1, 0));
        أرجع resp;
    }

    کينډۍ <ډول_نوم راس_2>
    راس_2* traceCmp(راس_2* vtxp) {
        static_assert(std::is_بنسټ_د<Dfg_راس_دوګونی, راس_2>::ارزښت,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت resp = make<راس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->پلنوالی() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->پلنوالی() - 1, 0));
        أرجع resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#که_تعریف_شوی VL_DEBUG
#تعریف_کړه TRACE_پایله_COVER() \
    افعل { \
        إذا (VL_UNLIKELY(m_کرښه_Coverage_File.is_open())) m_کرښه_Coverage_File << __LINE__ << '\n'; \
    } طالما (خطأ)
#وإلا
#تعریف_کړه TRACE_پایله_COVER() \
    افعل { \
    } طالما (خطأ)
#د_که_پای

#تعریف_کړه RETURN_پایله(vtxp) \
    افعل { \
        m_resp = (vtxp); \
        TRACE_پایله_COVER(); \
        أرجع; \
    } طالما (خطأ)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#تعریف_کړه RETURN_پایله_لکۍ(srcp, smsb, slsb) \
    افعل { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        Dfg_راس* ثابت tailp = (srcp); \
        ثابت uint32_t لکۍ_Msb = (smsb); \
        ثابت uint32_t لکۍ_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = لکۍ_Msb; \
        m_lsb = لکۍ_Lsb; \
        TRACE_پایله_COVER(); \
        أرجع; \
    } طالما (خطأ)

    // VISITORS
    فراغ لیدنه(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ لیدنه(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        Dfg_راس* defaultp = m_defaultp;
        m_defaultp = تش_اشاره;
        m_splicep = تش_اشاره;

        بنية Driver final {
            Dfg_راس* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = ړنګول;
            Driver(Dfg_راس* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::ویکټور<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        منطقي try_ټولیز_Default = defaultp;
        Dfg_راس* coverp = تش_اشاره;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](Dfg_راس& src, uint32_t lsb) {
            ثابت uint32_t msb = lsb + src.پلنوالی() - 1;
            drivers.emplace_شا(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            إذا (m_msb >= lsb && msb >= m_lsb) try_ټولیز_Default = خطأ;
            // If it does not cover the whole searched bit range, move on
            إذا (m_lsb < lsb || msb < m_msb) أرجع خطأ;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            أرجع ریښتیا;
        });

        // Trace the driver covering the whole searched range, if there is one
        إذا (coverp) RETURN_پایله_لکۍ(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        إذا (try_ټولیز_Default) RETURN_پایله_لکۍ(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::ترتیبول(drivers.begin(), drivers.پای_ټکی(), [](ثابت Driver& a, ثابت Driver& b) {  //
            أرجع a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::ویکټور<Dfg_راس*> termps;
        uint32_t lsb = m_lsb;
        لكل (ثابت Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            إذا (lsb > driver.m_msb) استمر;
            // Driver is above the searched MSB, done
            إذا (driver.m_lsb > m_msb) اكسر;
            // Gap below this driver, trace default to fill it
            إذا (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_شا(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ثابت uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_شا(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        إذا (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_شا(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.اندازه() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        Dfg_راس* resp = termps.front();
        لكل (اندازه_t i = 1; i < termps.اندازه(); ++i) {
            Dfg_راس* ثابت termp = termps[i];
            DfgConcat* ثابت catp = make<DfgConcat>(termp, resp->پلنوالی() + termp->پلنوالی());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_پایله(resp);
    }

    فراغ لیدنه(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        Dfg_راس* ثابت defaultp = m_defaultp;
        m_defaultp = تش_اشاره;
        m_splicep = تش_اشاره;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ثابت uint32_t idx = m_idxs.شا();
        إذا (Dfg_راس* ثابت driverp = vtxp->driverAt(idx)) {
            Dfg_راس* ثابت srcp = driverp->as<DfgUnitArray>()->srcp();
            إذا (srcp->is<Dfg_راس_Splice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_پایله(trace_راوباسه_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_پایله(traceSameIdx(defaultp));
    }

    فراغ لیدنه(Dfg_راس_Var* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        Dfg_راس* ثابت srcp = vtxp->srcp();
        Dfg_راس* ثابت defaultp = vtxp->defaultp();
        Dfg_راس* ثابت drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        إذا (srcp && srcp->is<Dfg_راس_Splice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_پایله(m_idxs.تشه() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    فراغ لیدنه(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        إذا (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_پایله(trace_ټېل_Idx(vtxp->fromp(), idxp->منزل_U32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.تشه(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ثابت resp = make<DfgArraySel>(vtxp, vtxp->پلنوالی());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->پلنوالی() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_پایله(selp);
    }

    فراغ لیدنه(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.شا() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_پایله(trace_راوباسه_Idx(vtxp->srcp()));
    }

    فراغ لیدنه(DfgConcat* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t r_پلنوالی = rhsp->پلنوالی();
        // If the traced bits are wholly in the RHS
        إذا (r_پلنوالی > m_msb) RETURN_پایله_لکۍ(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        إذا (m_lsb >= r_پلنوالی) RETURN_پایله_لکۍ(lhsp, m_msb - r_پلنوالی, m_lsb - r_پلنوالی);
        // The traced bit spans both sides, trace both
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, r_پلنوالی - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - r_پلنوالی, 0));
        RETURN_پایله(resp);
    }

    فراغ لیدنه(DfgRep* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_پلنوالی = srcp->پلنوالی();
        // If we need more bits than the source, then we need the whole source
        إذا (m_msb - m_lsb + 1 > s_پلنوالی) {
            DfgRep* ثابت repp = make<DfgRep>(vtxp, vtxp->پلنوالی());
            repp->srcp(trace(srcp, s_پلنوالی - 1, 0));
            DfgSel* ثابت resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_پایله(resp);
        }
        // If the requested bits are within the same repliacted word
        إذا (m_msb / s_پلنوالی == m_lsb / s_پلنوالی) {
            RETURN_پایله_لکۍ(srcp, m_msb % s_پلنوالی, m_lsb % s_پلنوالی);
        }
        // The requested bits span two replicated words
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, s_پلنوالی - 1, m_lsb % s_پلنوالی));
        resp->lhsp(trace(srcp, m_msb % s_پلنوالی, 0));
        RETURN_پایله(resp);
    }

    فراغ لیدنه(DfgExtend* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_پلنوالی = srcp->پلنوالی();
        // If the traced bits are wholly in the input
        إذا (s_پلنوالی > m_msb) RETURN_پایله_لکۍ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        إذا (m_lsb >= s_پلنوالی) RETURN_پایله(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_پلنوالی - 1, m_lsb));
        RETURN_پایله(resp);
    }

    فراغ لیدنه(DfgExtendS* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_پلنوالی = srcp->پلنوالی();
        // If the traced bits are wholly in the input
        إذا (s_پلنوالی > m_msb) RETURN_پایله_لکۍ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        إذا (m_lsb >= s_پلنوالی) {
            إذا (m_msb == m_lsb) RETURN_پایله_لکۍ(srcp, s_پلنوالی - 1, s_پلنوالی - 1);
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, s_پلنوالی - 1, s_پلنوالی - 1));
            RETURN_پایله(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_پلنوالی - 1, m_lsb));
        RETURN_پایله(resp);
    }

    فراغ لیدنه(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        RETURN_پایله_لکۍ(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    فراغ لیدنه(DfgNot* vtxp) override {
        DfgNot* ثابت resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_پایله(resp);
    }

    فراغ لیدنه(DfgAnd* vtxp) override { RETURN_پایله(trace_Bitwise_دوګونی(vtxp)); }
    فراغ لیدنه(DfgOr* vtxp) override { RETURN_پایله(trace_Bitwise_دوګونی(vtxp)); }
    فراغ لیدنه(DfgXor* vtxp) override { RETURN_پایله(trace_Bitwise_دوګونی(vtxp)); }
    فراغ لیدنه(Dfg_جمع_کړه* vtxp) override { RETURN_پایله(trace_جمع_کړه_Sub(vtxp)); }
    فراغ لیدنه(DfgSub* vtxp) override { RETURN_پایله(trace_جمع_کړه_Sub(vtxp)); }
    فراغ لیدنه(DfgRedAnd* vtxp) override { RETURN_پایله(traceReduction(vtxp)); }
    فراغ لیدنه(DfgRedOr* vtxp) override { RETURN_پایله(traceReduction(vtxp)); }
    فراغ لیدنه(DfgRedXor* vtxp) override { RETURN_پایله(traceReduction(vtxp)); }
    فراغ لیدنه(DfgEq* vtxp) override { RETURN_پایله(traceCmp(vtxp)); }
    فراغ لیدنه(DfgNeq* vtxp) override { RETURN_پایله(traceCmp(vtxp)); }
    فراغ لیدنه(DfgLt* vtxp) override { RETURN_پایله(traceCmp(vtxp)); }
    فراغ لیدنه(DfgLte* vtxp) override { RETURN_پایله(traceCmp(vtxp)); }
    فراغ لیدنه(DfgGt* vtxp) override { RETURN_پایله(traceCmp(vtxp)); }
    فراغ لیدنه(DfgGte* vtxp) override { RETURN_پایله(traceCmp(vtxp)); }

    فراغ لیدنه(Dfg_ښويدنه_RS* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t ښويدنه_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t ښکتنی_پلنوالی = ښويدنه_Amnt > vtxp->پلنوالی() ? 0 : vtxp->پلنوالی() - ښويدنه_Amnt;
            // If the traced bits are wholly in the input
            إذا (ښکتنی_پلنوالی > m_msb) {
                RETURN_پایله_لکۍ(lhsp, m_msb + ښويدنه_Amnt, m_lsb + ښويدنه_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (m_lsb >= ښکتنی_پلنوالی) {
                DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->پلنوالی() - 1, lhsp->پلنوالی() - 1));
                RETURN_پایله(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, ښکتنی_پلنوالی - 1 + ښويدنه_Amnt, m_lsb + ښويدنه_Amnt));
            RETURN_پایله(resp);
        }

        Dfg_ښويدنه_RS* ثابت shiftrsp = make<Dfg_ښويدنه_RS>(vtxp, vtxp->lhsp()->پلنوالی() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->پلنوالی() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->پلنوالی() - 1, m_lsb));
        إذا (m_msb == vtxp->lhsp()->پلنوالی() - 1) RETURN_پایله(shiftrsp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_پایله(selp);
    }

    فراغ لیدنه(Dfg_ښويدنه_R* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t ښويدنه_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t ښکتنی_پلنوالی = ښويدنه_Amnt > vtxp->پلنوالی() ? 0 : vtxp->پلنوالی() - ښويدنه_Amnt;
            // If the traced bits are wholly in the input
            إذا (ښکتنی_پلنوالی > m_msb) {
                RETURN_پایله_لکۍ(lhsp, m_msb + ښويدنه_Amnt, m_lsb + ښويدنه_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (m_lsb >= ښکتنی_پلنوالی) RETURN_پایله(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, ښکتنی_پلنوالی - 1 + ښويدنه_Amnt, m_lsb + ښويدنه_Amnt));
            RETURN_پایله(resp);
        }

        Dfg_ښويدنه_R* ثابت shiftrp = make<Dfg_ښويدنه_R>(vtxp, vtxp->lhsp()->پلنوالی() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->پلنوالی() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->پلنوالی() - 1, m_lsb));
        إذا (m_msb == vtxp->lhsp()->پلنوالی() - 1) RETURN_پایله(shiftrp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_پایله(selp);
    }

    فراغ لیدنه(Dfg_ښويدنه_L* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t ښويدنه_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t ښکتنی_پلنوالی = ښويدنه_Amnt > vtxp->پلنوالی() ? vtxp->پلنوالی() : ښويدنه_Amnt;
            // If the traced bits are wholly in the input
            إذا (m_lsb >= ښکتنی_پلنوالی) {
                RETURN_پایله_لکۍ(lhsp, m_msb - ښويدنه_Amnt, m_lsb - ښويدنه_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (ښکتنی_پلنوالی > m_msb) RETURN_پایله(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->پلنوالی() - (m_msb - ښکتنی_پلنوالی + 1)));
            resp->lhsp(trace(lhsp, m_msb - ښويدنه_Amnt, ښکتنی_پلنوالی - ښويدنه_Amnt));
            RETURN_پایله(resp);
        }

        Dfg_ښويدنه_L* ثابت shiftlp = make<Dfg_ښويدنه_L>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->پلنوالی() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        إذا (m_lsb == 0) RETURN_پایله(shiftlp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_پایله(selp);
    }

    فراغ لیدنه(DfgCond* vtxp) override {
        DfgCond* ثابت resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->پلنوالی() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_پایله(resp);
    }

    فراغ لیدنه(Dfg_سمون_Masked* vtxp) override {
        Dfg_سمون_Masked* ثابت resp = make<Dfg_سمون_Masked>(vtxp, vtxp->پلنوالی());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->پلنوالی() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->پلنوالی() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_پایله(selp);
    }

#تعریف_لغوه RETURN_پایله
#تعریف_لغوه TRACE_پایله_COVER
#تعریف_لغوه RETURN_پایله_لکۍ

عام:
    // CONSTRUCTOR
    TraceDriver(Dfg_ګراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#که_تعریف_شوی VL_DEBUG
        إذا (v3Global.opt.debugCheck()) {
            m_کرښه_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.مختاړی()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_بنسټ::بهر | std::ios_بنسټ::app);
        }
#د_که_پای
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    Dfg_راس* apply(Dfg_راس& vtx, uint32_t lsb, uint32_t پلنوالی) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        أرجع trace(&vtx, lsb + پلنوالی - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
صنف بټ_ماسک final {
    // TYPES
    ثابت Dfg_معلومات_Type& m_dtype;
    اتحاد {
        V3_شمېره m_عدد;  // The bits, if packed
        std::ویکټور<بټ_ماسک> m_sub;  // The per element masks, if aggregate
    };

عام:
    // CONSTRUCTOR
    بټ_ماسک(File_کرښه* flp, ثابت Dfg_معلومات_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        إذا (m_dtype.isPacked()) {
            جديد (&m_عدد) V3_شمېره{flp, static_cast<صحيح>(m_dtype.اندازه()), 0};
        } وإلا {
            جديد (&m_sub) std::ویکټور<بټ_ماسک>{};
            m_sub.reserve(m_dtype.اندازه());
            لكل (uint32_t i = 0; i < m_dtype.اندازه(); ++i)
                m_sub.emplace_شا(flp, m_dtype.elemDtype());
        }
    }
    بټ_ماسک(ثابت Dfg_راس& vtx)
        : بټ_ماسک{vtx.fileline(), vtx.dtype()} {}
    بټ_ماسک(ثابت بټ_ماسک& other)
        : m_dtype{other.m_dtype} {
        إذا (m_dtype.isPacked()) {
            جديد (&m_عدد) V3_شمېره{other.m_عدد};
        } وإلا {
            جديد (&m_sub) std::ویکټور<بټ_ماسک>{other.m_sub};
        }
    }
    بټ_ماسک& چلوونکی=(ثابت بټ_ماسک& other) {
        إذا (هذا != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            إذا (other.m_dtype.isPacked()) {
                m_عدد = other.m_عدد;
            } وإلا {
                m_sub = other.m_sub;
            }
        }
        أرجع *هذا;
    }
    ~بټ_ماسک() {
        إذا (m_dtype.isPacked()) {
            m_عدد.~V3_شمېره();
        } وإلا {
            m_sub.~ویکټور<بټ_ماسک>();
        }
    }

    // METHODS
    V3_شمېره& عدد() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        أرجع m_عدد;
    }
    ثابت V3_شمېره& عدد() ثابت {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        أرجع m_عدد;
    }
    std::ویکټور<بټ_ماسک>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        أرجع m_sub;
    }

    منطقي isZero() ثابت {
        إذا (m_dtype.isPacked()) أرجع m_عدد.isEqZero();

        لكل (ثابت بټ_ماسک& sub : m_sub) {
            إذا (!sub.isZero()) أرجع خطأ;
        }
        أرجع ریښتیا;
    }
    منطقي isOnes() ثابت {
        إذا (m_dtype.isPacked()) أرجع m_عدد.isEqAllOnes();

        لكل (ثابت بټ_ماسک& sub : m_sub) {
            إذا (!sub.isOnes()) أرجع خطأ;
        }
        أرجع ریښتیا;
    }
    فراغ سېټ_Zero() {
        إذا (m_dtype.isPacked()) {
            m_عدد.سېټ_All_Bits0();
        } وإلا {
            لكل (بټ_ماسک& sub : m_sub) sub.سېټ_Zero();
        }
    }
    فراغ سېټ_Ones() {
        إذا (m_dtype.isPacked()) {
            m_عدد.سېټ_All_Bits1();
        } وإلا {
            لكل (بټ_ماسک& sub : m_sub) sub.سېټ_Ones();
        }
    }

    منطقي چلوونکی==(ثابت بټ_ماسک& other) ثابت {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        إذا (m_dtype.isPacked()) أرجع m_عدد.isCaseEq(other.m_عدد);
        أرجع m_sub == other.m_sub;
    }
    منطقي چلوونکی!=(ثابت بټ_ماسک& other) ثابت { أرجع !(*هذا == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    منطقي is_Contraction_د(ثابت بټ_ماسک& that) ثابت {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        إذا (m_dtype.isPacked()) {
            ثابت اندازه_t ټکي = VL_ټکي_I(m_dtype.اندازه());
            لكل (اندازه_t i = 0; i < ټکي; ++i) {
                إذا (~m_عدد.edata_کلمه(i) & that.m_عدد.edata_کلمه(i)) أرجع ریښتیا;
            }
            أرجع خطأ;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        لكل (اندازه_t i = 0; i < m_sub.اندازه(); ++i) {
            إذا (m_sub[i].is_Contraction_د(that.m_sub[i])) أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    std::string منزل_String() ثابت {
        إذا (m_dtype.isPacked()) أرجع "0x" + m_عدد.displayed(m_عدد.fileline(), "%x");
        std::string پایله;
        پایله += "{";
        لكل (ثابت بټ_ماسک& sub : m_sub) {
            إذا (&sub != &m_sub.front()) پایله += ", ";
            پایله += sub.منزل_String();
        }
        پایله += "}";
        أرجع پایله;
    }
};

صنف Independent_بټونه final : عام DfgVisitor {
    // TYPES
    بنية راس_State final {
        اندازه_t m_rpo_شمېره = 0;  // Reverse Postorder Number of vertex
        منطقي m_isOnWorkList = خطأ;  // Whether the vertex is on the work list
    };

    // STATE
    ثابت SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ثابت Dfg_راس*, بټ_ماسک> m_vtxp2_ماسک;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::لومړیتوب_کتار<اندازه_t, std::ویکټور<اندازه_t>, std::greater<اندازه_t>> m_work_کتار;
    std::unordered_map<ثابت Dfg_راس*, راس_State> m_vtxp2State;  // State of each vertex

#که_تعریف_شوی VL_DEBUG
    std::ofstream m_کرښه_Coverage_File;  // Line coverage file, just for testing
#د_که_پای

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    بټ_ماسک& ماسک(ثابت Dfg_راس& vtx) {
        // Look up (or create) mask for 'vtx'
        أرجع m_vtxp2_ماسک
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .لومړی->دوهم;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#که_تعریف_شوی VL_DEBUG
#تعریف_کړه ماسک_2(vtxp) \
    ([هذا](ثابت Dfg_راس& vtx) -> بټ_ماسک& { \
        إذا (VL_UNLIKELY(m_کرښه_Coverage_File.is_open())) m_کرښه_Coverage_File << __LINE__ << '\n'; \
        أرجع ماسک(vtx); \
    }(*vtxp))
#وإلا
#تعریف_کړه ماسک_2(vtxp) ماسک(*vtxp)
#د_که_پای

    // Clear all bits at or below the most significant clear bit
    فراغ floodTowardsLsb(V3_شمېره& عدد) {
        لكل (صحيح i = عدد.پلنوالی() - 1; i >= 0; --i) {
            إذا (عدد.بټ_Is1(i)) استمر;
            عدد.op_سېټ_Range(0, i + 1, '0');
            اكسر;
        }
    }

    // Clear all bits at or above the least significant clear bit
    فراغ floodTowardsMsb(V3_شمېره& عدد) {
        لكل (صحيح i = 0; i < عدد.پلنوالی(); ++i) {
            إذا (عدد.بټ_Is1(i)) استمر;
            عدد.op_سېټ_Range(i, عدد.پلنوالی() - i, '0');
            اكسر;
        }
    }

    فراغ propagateFromDriver(بټ_ماسک& m, ثابت Dfg_راس* srcp) {
        // If there is no driver, we are done
        إذا (!srcp) أرجع;
        // If it is driven by a splice, we need to combine the masks of the drivers
        إذا (ثابت DfgSplicePacked* ثابت splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
                m.عدد().opSelInto(ماسک_2(&src).عدد(), lo, src.پلنوالی());
                أرجع خطأ;
            });
            أرجع;
        }
        إذا (ثابت DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
                إذا (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = ماسک_2(uap->srcp());
                } وإلا {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                أرجع خطأ;
            });
            أرجع;
        }
        // Otherwise, we just use the mask of the single driver
        m = ماسک_2(srcp);
    }

    // VISITORS
    فراغ لیدنه(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    فراغ لیدنه(Dfg_راس_Var* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        إذا (vtxp->isVolatile()) أرجع;

        بټ_ماسک& m = ماسک_2(vtxp);
        Dfg_راس* ثابت srcp = vtxp->srcp();
        Dfg_راس* ثابت defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        إذا (defaultp) m = ماسک_2(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    فراغ لیدنه(Dfg_راس_Splice* vtxp) override {
        propagateFromDriver(ماسک_2(vtxp), vtxp);  // Needed to continue traversal
    }

    فراغ لیدنه(DfgUnitArray* vtxp) override { ماسک_2(vtxp).sub().at(0) = ماسک_2(vtxp->srcp()); }

    فراغ لیدنه(DfgArraySel* vtxp) override {
        Dfg_راس* ثابت fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        إذا (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            ماسک_2(vtxp) = ماسک_2(vtxp->fromp()).sub().at(idxp->منزل_اندازه_T());
            أرجع;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        إذا (ماسک_2(vtxp->bitp()).isOnes() && ماسک_2(fromp).isOnes()) ماسک_2(vtxp).سېټ_Ones();
    }

    فراغ لیدنه(DfgConcat* vtxp) override {
        ثابت Dfg_راس* ثابت rhsp = vtxp->rhsp();
        ثابت Dfg_راس* ثابت lhsp = vtxp->lhsp();
        V3_شمېره& m = ماسک_2(vtxp).عدد();
        m.opSelInto(ماسک_2(rhsp).عدد(), 0, rhsp->پلنوالی());
        m.opSelInto(ماسک_2(lhsp).عدد(), rhsp->پلنوالی(), lhsp->پلنوالی());
    }

    فراغ لیدنه(DfgRep* vtxp) override {
        ثابت uint32_t شمېر = vtxp->شمېر();
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_پلنوالی = srcp->پلنوالی();
        V3_شمېره& v_ماسک = ماسک_2(vtxp).عدد();
        V3_شمېره& s_ماسک = ماسک_2(srcp).عدد();
        لكل (uint32_t i = 0; i < شمېر; ++i) v_ماسک.opSelInto(s_ماسک, i * s_پلنوالی, s_پلنوالی);
    }

    فراغ لیدنه(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t msb = lsb + vtxp->پلنوالی() - 1;
        ماسک_2(vtxp).عدد().opSel(ماسک_2(vtxp->fromp()).عدد(), msb, lsb);
    }

    فراغ لیدنه(DfgExtend* vtxp) override {
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_پلنوالی = srcp->پلنوالی();
        V3_شمېره& s = ماسک_2(srcp).عدد();
        V3_شمېره& m = ماسک_2(vtxp).عدد();
        m.opSelInto(s, 0, s_پلنوالی);
        m.op_سېټ_Range(s_پلنوالی, vtxp->پلنوالی() - s_پلنوالی, '1');
    }
    فراغ لیدنه(DfgExtendS* vtxp) override {
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_پلنوالی = srcp->پلنوالی();
        V3_شمېره& s = ماسک_2(srcp).عدد();
        V3_شمېره& m = ماسک_2(vtxp).عدد();
        m.opSelInto(s, 0, s_پلنوالی);
        m.op_سېټ_Range(s_پلنوالی, vtxp->پلنوالی() - s_پلنوالی, s.بټ_Is0(s_پلنوالی - 1) ? '0' : '1');
    }

    فراغ لیدنه(DfgNot* vtxp) override {  //
        ماسک_2(vtxp) = ماسک_2(vtxp->srcp());
    }

    فراغ لیدنه(DfgAnd* vtxp) override {  //
        ماسک_2(vtxp).عدد().opAnd(ماسک_2(vtxp->lhsp()).عدد(), ماسک_2(vtxp->rhsp()).عدد());
    }
    فراغ لیدنه(DfgOr* vtxp) override {  //
        ماسک_2(vtxp).عدد().opAnd(ماسک_2(vtxp->lhsp()).عدد(), ماسک_2(vtxp->rhsp()).عدد());
    }
    فراغ لیدنه(DfgXor* vtxp) override {  //
        ماسک_2(vtxp).عدد().opAnd(ماسک_2(vtxp->lhsp()).عدد(), ماسک_2(vtxp->rhsp()).عدد());
    }

    فراغ لیدنه(Dfg_جمع_کړه* vtxp) override {
        V3_شمېره& m = ماسک_2(vtxp).عدد();
        m.opAnd(ماسک_2(vtxp->lhsp()).عدد(), ماسک_2(vtxp->rhsp()).عدد());
        floodTowardsMsb(m);
    }
    فراغ لیدنه(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_شمېره& m = ماسک_2(vtxp).عدد();
        m.opAnd(ماسک_2(vtxp->lhsp()).عدد(), ماسک_2(vtxp->rhsp()).عدد());
        floodTowardsMsb(m);
    }

    فراغ لیدنه(DfgRedAnd* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }
    فراغ لیدنه(DfgRedOr* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }
    فراغ لیدنه(DfgRedXor* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }

    فراغ لیدنه(DfgEq* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }
    فراغ لیدنه(DfgNeq* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }
    فراغ لیدنه(DfgLt* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }
    فراغ لیدنه(DfgLte* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }
    فراغ لیدنه(DfgGt* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }
    فراغ لیدنه(DfgGte* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).سېټ_Ones();
    }

    فراغ لیدنه(Dfg_ښويدنه_RS* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t پلنوالی = vtxp->پلنوالی();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t ښويدنه_Amount = rConstp->منزل_U32();
            إذا (ښويدنه_Amount >= پلنوالی) {
                إذا (ماسک_2(lhsp).عدد().بټ_Is1(پلنوالی - 1)) { ماسک_2(vtxp).سېټ_Ones(); }
            } وإلا {
                V3_شمېره& m = ماسک_2(vtxp).عدد();
                m.op_ښويدنه_RS(ماسک_2(lhsp).عدد(), rConstp->عدد(), پلنوالی);
                m.op_سېټ_Range(پلنوالی - ښويدنه_Amount, ښويدنه_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (ماسک_2(rhsp).isOnes()) {
            V3_شمېره& m = ماسک_2(vtxp).عدد();
            m = ماسک_2(lhsp).عدد();
            floodTowardsLsb(m);
        }
    }

    فراغ لیدنه(Dfg_ښويدنه_R* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t پلنوالی = vtxp->پلنوالی();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t ښويدنه_Amount = rConstp->منزل_U32();
            إذا (ښويدنه_Amount >= پلنوالی) {
                ماسک_2(vtxp).سېټ_Ones();
            } وإلا {
                V3_شمېره& m = ماسک_2(vtxp).عدد();
                m.op_ښويدنه_R(ماسک_2(lhsp).عدد(), rConstp->عدد());
                m.op_سېټ_Range(پلنوالی - ښويدنه_Amount, ښويدنه_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (ماسک_2(rhsp).isOnes()) {
            V3_شمېره& m = ماسک_2(vtxp).عدد();
            m = ماسک_2(lhsp).عدد();
            floodTowardsLsb(m);
        }
    }

    فراغ لیدنه(Dfg_ښويدنه_L* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t پلنوالی = vtxp->پلنوالی();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t ښويدنه_Amount = rConstp->منزل_U32();
            إذا (ښويدنه_Amount >= پلنوالی) {
                ماسک_2(vtxp).سېټ_Ones();
            } وإلا {
                V3_شمېره& m = ماسک_2(vtxp).عدد();
                m.op_ښويدنه_L(ماسک_2(lhsp).عدد(), rConstp->عدد());
                m.op_سېټ_Range(0, ښويدنه_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (ماسک_2(rhsp).isOnes()) {
            V3_شمېره& m = ماسک_2(vtxp).عدد();
            m = ماسک_2(lhsp).عدد();
            floodTowardsMsb(m);
        }
    }

    فراغ لیدنه(DfgCond* vtxp) override {
        إذا (ماسک_2(vtxp->condp()).isOnes()) {
            ماسک_2(vtxp).عدد().opAnd(ماسک_2(vtxp->thenp()).عدد(), ماسک_2(vtxp->elsep()).عدد());
        }
    }

    فراغ لیدنه(Dfg_سمون_Masked* vtxp) override {
        إذا (ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->matchp()).isOnes()) { ماسک_2(vtxp).سېټ_Ones(); }
    }

#تعریف_لغوه ماسک_2

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    فراغ کتار_ته_ورزیاتول_Sinks(Dfg_راس& vtx) {
        vtx.foreachSink([&](Dfg_راس& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            إذا (!m_sccInfo.get(sink)) أرجع خطأ;
            // Otherwise just enqueue it
            راس_State& state = m_vtxp2State.at(&sink);
            إذا (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_work_کتار.ټېل(state.m_rpo_شمېره);
            }
            state.m_isOnWorkList = ریښتیا;
            أرجع خطأ;
        });
    };

    فراغ dfs_Post_ترتیب(std::unordered_سېټ<Dfg_راس*>& لیدل_شوی,  //
                      std::ویکټور<Dfg_راس*>& post_ترتیب_Enumeration,  //
                      Dfg_راس& vtx) {
        // Mark visited, skip if already visited
        إذا (!لیدل_شوی.ورننباسه(&vtx).دوهم) أرجع;
        // Visit un-visited successors
        vtx.foreachSink([&](Dfg_راس& snk) {
            dfs_Post_ترتیب(لیدل_شوی, post_ترتیب_Enumeration, snk);
            أرجع خطأ;
        });
        // Add to post order enumeration
        post_ترتیب_Enumeration.emplace_شا(&vtx);
    };

    Independent_بټونه(Dfg_ګراف& dfg, ثابت SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#که_تعریف_شوی VL_DEBUG
        إذا (v3Global.opt.debugCheck()) {
            m_کرښه_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.مختاړی()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_بنسټ::بهر | std::ios_بنسټ::app);
        }
#د_که_پای

        // Compute Reverse-Postorder enumeration of vertices
        std::ویکټور<Dfg_راس*> rpoEnumeration;
        std::unordered_سېټ<Dfg_راس*> لیدل_شوی;
        dfg.for_Each_راس([&](Dfg_راس& vtx) {
            إذا (!vtx.nInputs()) dfs_Post_ترتیب(لیدل_شوی, rpoEnumeration, vtx);
        });
        dfg.for_Each_راس([&](Dfg_راس& vtx) {  //
            dfs_Post_ترتیب(لیدل_شوی, rpoEnumeration, vtx);
        });
        std::سرچپه(rpoEnumeration.begin(), rpoEnumeration.پای_ټکی());
        UASSERT(rpoEnumeration.اندازه() == dfg.اندازه(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        لكل (اندازه_t i = 0; i < rpoEnumeration.اندازه(); ++i) {
            راس_State& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_شمېره = i;
            state.m_isOnWorkList = خطأ;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        لكل (Dfg_راس* ثابت vtxp : rpoEnumeration) {
            إذا (m_sccInfo.get(*vtxp)) استمر;
            ماسک(*vtxp).سېټ_Ones();
        }
        لكل (Dfg_راس* ثابت vtxp : rpoEnumeration) {
            إذا (!m_sccInfo.get(*vtxp)) استمر;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << ماسک(*vtxp).منزل_String());
            إذا (!ماسک(*vtxp).isZero()) کتار_ته_ورزیاتول_Sinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        طالما (!m_work_کتار.تشه()) {
            // Grab next item
            Dfg_راس* ثابت currp = rpoEnumeration[m_work_کتار.سر()];
            m_work_کتار.راوباسه();
            m_vtxp2State.at(currp).m_isOnWorkList = خطأ;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ثابت بټ_ماسک& curr_ماسک = ماسک(*currp);
            // Remember current mask so we can check if it changed
            ثابت بټ_ماسک prev_ماسک = curr_ماسک;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            إذا (prev_ماسک != curr_ماسک) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prev_ماسک.منزل_String()
                             << "\n      to: " << curr_ماسک.منزل_String());
                کتار_ته_ورزیاتول_Sinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!curr_ماسک.is_Contraction_د(prev_ماسک), currp,
                                "Mask should only expand");
                }
            }
        }
    }

عام:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    ساكن std::unordered_map<ثابت Dfg_راس*, بټ_ماسک> apply(Dfg_ګراف& dfg,
                                                               ثابت SccInfo& sccInfo) {
        أرجع std::move(Independent_بټونه{dfg, sccInfo}.m_vtxp2_ماسک);
    }
};

صنف Fix_پورته final {
    Dfg_ګراف& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ثابت std::unordered_map<ثابت Dfg_راس*, بټ_ماسک> m_independent_بټونه
        = Independent_بټونه::apply(m_dfg, m_sccInfo);
    اندازه_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ساكن بټ_ماسک compute_کارول_شوی_بټونه(Dfg_راس& vtx) {
        بټ_ماسک پایله{vtx};
        vtx.foreachSink([&پایله](Dfg_راس& sink) {
            // If used via a Sel, mark the selected bits used
            إذا (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                پایله.عدد().op_سېټ_Range(selp->lsb(), selp->پلنوالی(), '1');
                أرجع خطأ;
            }
            // Used without a Sel, so all bits are used
            پایله.سېټ_Ones();
            أرجع ریښتیا;
        });
        أرجع پایله;
    }

    ساكن std::string debugStr(ثابت Dfg_راس& vtx) {  // LCOV_EXCL_START
        إذا (ثابت DfgArraySel* ثابت aselp = vtx.cast<DfgArraySel>()) {
            ثابت اندازه_t i = aselp->bitp()->as<DfgConst>()->منزل_اندازه_T();
            أرجع debugStr(*aselp->fromp()) + "[" + std::منزل_string(i) + "]";
        }
        إذا (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
            أرجع varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    فراغ gatherTerms(std::ویکټور<Dfg_راس*>& termps, Dfg_راس& vtx, ثابت V3_شمېره& indp_بټونه,
                     ثابت uint32_t hi, ثابت uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        منطقي isIndependent = indp_بټونه.بټ_Is1(lo);
        uint32_t lsb = lo;
        لكل (uint32_t msb = lo; msb <= hi; ++msb) {
            ثابت منطقي پای_ټکی_Range = msb == hi || isIndependent != indp_بټونه.بټ_Is1(msb + 1);
            إذا (!پای_ټکی_Range) استمر;
            ثابت uint32_t پلنوالی = msb - lsb + 1;
            // Compute the term to use for this range
            إذا (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_شا(m_traceDriver.apply(vtx, lsb, پلنوالی));
            } وإلا {
                // If dependent, fall back on using the part of the variable
                DfgSel* ثابت selp = جديد DfgSel{m_dfg, vtx.fileline(), Dfg_معلومات_Type::packed(پلنوالی)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.جمع_کړه(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_شا(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    فراغ fix_پورته_Packed(Dfg_راس& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ثابت بټ_ماسک& indp_بټونه = m_independent_بټونه.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indp_بټونه.منزل_String());
        // Can't do anything if all bits are dependent
        إذا (indp_بټونه.isZero()) أرجع;

        // Figure out which bits of 'vtxp' are used
        ثابت بټ_ماسک کارول_شوی_بټونه = compute_کارول_شوی_بټونه(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << کارول_شوی_بټونه.منزل_String());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_شمېره کارول_شوی_And_Independent{vtx.fileline(), static_cast<صحيح>(vtx.پلنوالی()), 0};
        کارول_شوی_And_Independent.opAnd(کارول_شوی_بټونه.عدد(), indp_بټونه.عدد());
        إذا (کارول_شوی_And_Independent.isEqZero()) أرجع;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::ویکټور<Dfg_راس*> termps;

        // Iterate through consecutive used/unused ranges
        File_کرښه* ثابت flp = vtx.fileline();
        ثابت uint32_t پلنوالی = vtx.پلنوالی();
        منطقي is_کارول_شوی = کارول_شوی_بټونه.عدد().بټ_Is1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        لكل (uint32_t msb = 0; msb < پلنوالی; ++msb) {
            ثابت منطقي پای_ټکی_Range = msb == پلنوالی - 1 || is_کارول_شوی != کارول_شوی_بټونه.عدد().بټ_Is1(msb + 1);
            إذا (!پای_ټکی_Range) استمر;
            إذا (is_کارول_شوی) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indp_بټونه.عدد(), msb, lsb);
            } وإلا {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ثابت constp = جديد DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.جمع_کړه(*constp, 0);
                termps.emplace_شا(constp);
            }
            // Next iteration
            is_کارول_شوی = !is_کارول_شوی;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        Dfg_راس* replacementp = termps.front();
        ثابت uint64_t vComp = m_sccInfo.get(vtx);
        لكل (اندازه_t i = 1; i < termps.اندازه(); ++i) {
            Dfg_راس* ثابت termp = termps[i];
            ثابت uint32_t cat_پلنوالی = replacementp->پلنوالی() + termp->پلنوالی();
            DfgConcat* ثابت catp = جديد DfgConcat{m_dfg, flp, Dfg_معلومات_Type::packed(cat_پلنوالی)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ثابت uint64_t tComp = m_sccInfo.get(*termp);
            ثابت uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.جمع_کړه(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        لكل (Dfg_راس* ثابت termp : termps) {
            إذا (DfgSel* ثابت selp = termp->cast<DfgSel>()) {
                إذا (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        إذا (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    فراغ اصلي(Dfg_راس_Var& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        إذا (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fix_پورته_Packed(var);
        } وإلا إذا (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ثابت uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](Dfg_راس& sink) {
                // Ignore if sink is not part of same cycle
                إذا (m_sccInfo.get(sink) != component) أرجع خطأ;
                // Only handle ArraySels with constant index
                DfgArraySel* ثابت aselp = sink.cast<DfgArraySel>();
                إذا (!aselp) أرجع خطأ;
                إذا (!aselp->bitp()->is<DfgConst>()) أرجع خطأ;
                // Fix up the word
                fix_پورته_Packed(*aselp);
                أرجع خطأ;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    Fix_پورته(Dfg_ګراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

عام:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ساكن اندازه_t apply(Dfg_ګراف& dfg, SccInfo& sccInfo) {
        إذا (!sccInfo.isCyclic()) أرجع 0;

        Fix_پورته fix_پورته{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
            إذا (!sccInfo.get(vtx)) استمر;
            fix_پورته.اصلي(vtx);
            إذا (!sccInfo.isCyclic()) اكسر;
        }
        // Return the number of improvements made
        أرجع fix_پورته.m_nImprovements;
    }
};

فراغ break_دورې(Dfg_ګراف& dfg, V3_Dfg_Break_دورې_Context& ctx) {
    // Shorthand for dumping graph at given dump level
    ثابت خپلکاری dump = [&](صحيح level, ثابت Dfg_ګراف& dfg, ثابت std::string& name) {
        إذا (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    اندازه_t nImprovements = 0;
    اندازه_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    افعل {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ثابت اندازه_t nFixed = Fix_پورته::apply(dfg, sccInfo);
        إذا (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        إذا (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        إذا (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            أرجع;
        }
    } طالما (nImprovements != prevNImprovements);

    // Debug dump
    إذا (dumpDfgLevel() >= 9) {
        ثابت SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ثابت Dfg_راس& vtx) {
            أرجع sccInfo.get(vtx);  //
        });
    }

    // Accounting
    إذا (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } وإلا {
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
    std::ویکټور<Dfg_راس_Var*> varps;
    لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
        إذا (!vtx.tmpForp()) varps.ټېل_شا(&vtx);
    }
    std::stable_برخه_کول(varps.begin(), varps.پای_ټکی(), [](ثابت Dfg_راس_Var* varp) {
        أرجع varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    لكل (Dfg_راس_Var* ثابت varp : varps) {
        // Stop as soon as it becomes acyclic
        إذا (!sccInfo.isCyclic()) اكسر;

        // Ignore if variable is not part of a cycle
        إذا (!sccInfo.get(*varp)) استمر;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* ثابت prevp = جديد DfgPrev{dfg, varp->vscp()};
        sccInfo.جمع_کړه(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_nPrevInserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    إذا (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
فراغ V3DfgPasses::break_دورې(Dfg_ګراف& dfg, V3DfgContext& ctx) {
    V3_Dfg_Break_دورې::break_دورې(dfg, ctx.m_break_دورې_Context);
    إذا (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::remove_ناکارول_شوی(dfg);
}
