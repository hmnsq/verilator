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

نطاق V3_Dfg_Break_چرخهها {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
صنف SccInfo final {
    Dfg_گراف& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ثابت Dfg_رأس*, uint64_t> m_vtx2Scc;
    اندازه_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

عام:
    SccInfo(Dfg_گراف& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.for_Each_رأس([&](ثابت Dfg_رأس& vtx) {
            ثابت uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            إذا (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    منطقي isCyclic() ثابت { أرجع m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ثابت Dfg_رأس& vtx) ثابت { أرجع m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    فراغ جمع_کن(ثابت Dfg_رأس& vtx, uint64_t sccIdx) {
        ثابت منطقي new_مدخل = m_vtx2Scc.emplace(&vtx, sccIdx).دومی;
        UASSERT_OBJ(new_مدخل, &vtx, "Vertex already inserted");
        إذا (sccIdx) ++m_nCyclicVertices;
    }

    منطقي stillCyclicFwd(ثابت Dfg_رأس& vtx) ثابت {
        // If it only reads non-cyclic vertices, it has also become acyclic
        أرجع vtx.foreach_منبع([&](ثابت Dfg_رأس& src) {  //
            أرجع m_vtx2Scc.at(&src);
        });
    }

    منطقي stillCyclicBwd(ثابت Dfg_رأس& vtx) ثابت {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        أرجع vtx.foreachSink([&](ثابت Dfg_رأس& dst) {  //
            أرجع m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    فراغ updateAcyclic(ثابت Dfg_رأس& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ثابت Dfg_رأس& dst) {
            // Sink was known to be acyclic, stop
            إذا (!m_vtx2Scc.at(&dst)) أرجع خطأ;
            إذا (!stillCyclicFwd(dst)) updateAcyclic(dst);
            أرجع خطأ;
        });
        // Propagate the update through the graph backward
        vtx.foreach_منبع([&](ثابت Dfg_رأس& src) {
            // Source was known to be acyclic, stop
            إذا (!m_vtx2Scc.at(&src)) أرجع خطأ;
            إذا (!stillCyclicBwd(src)) updateAcyclic(src);
            أرجع خطأ;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    فراغ updateReplacement(ثابت Dfg_رأس& vtx, Dfg_رأس& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        إذا (!get(replacement)) {
            replacement.foreachSink([&](Dfg_رأس& dst) {
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
        m_dfg.for_Each_رأس([&](ثابت Dfg_رأس& vtx) {
            // 'think not cyclic' implies 'actually not cyclic'
            UASSERT_OBJ(m_vtx2Scc.at(&vtx) || !actual.at(vtx), &vtx, "Inconsisten SccInfo");
        });
    }
};

صنف TraceDriver final : عام DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    بنية Cache_کلید final {
        Dfg_رأس* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_کلید() = حذف;
        Cache_کلید(Dfg_رأس* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        بنية Hash final {
            اندازه_t عملگر()(ثابت Cache_کلید& قلم_2) ثابت {
                // cppcheck-suppress unreadVariable
                V3Hash hash{قلم_2.m_vtxp};
                hash += قلم_2.m_lsb;
                hash += قلم_2.m_msb;
                أرجع hash.مقدار();
            }
        };

        بنية Equal final {
            منطقي عملگر()(ثابت Cache_کلید& a, ثابت Cache_کلید& b) ثابت {
                أرجع a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
            }
        };
    };

    // STATE
    Dfg_گراف& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    // The strongly connected component we are currently trying to escape
    uint64_t m_component = 0;
    uint32_t m_lsb = 0;  // LSB to extract from the currently visited Vertex
    uint32_t m_msb = 0;  // MSB to extract from the currently visited Vertex
    std::وکتور<uint32_t> m_idxs;  // Indices to extract from the currently visited Vertex
    std::وکتور<Dfg_رأس**> m_pendingps;  // Pending pass-through cache entries across trace calls
    // Result of tracing the currently visited Vertex. Use RETURN_RESULT below!
    Dfg_رأس* m_resp = اشاره_تهی;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    Dfg_رأس* m_tailp = اشاره_تهی;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    Dfg_رأس* m_defaultp = اشاره_تهی;
    Dfg_رأس* m_splicep = اشاره_تهی;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_کلید, Dfg_رأس*, Cache_کلید::Hash, Cache_کلید::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_کلید, Dfg_رأس*, Cache_کلید::Hash, Cache_کلید::Equal> m_boundaryCache;

#اگر_تعریف_شده VL_DEBUG
    std::ofstream m_سطر_Coverage_File;  // Line coverage file, just for testing
#پایان_اگر

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    قالب <نام_نوع رأس_2>
    رأس_2* make(ثابت Dfg_رأس* refp, uint32_t عرض) {
        static_assert(std::is_پایه_از<Dfg_رأس, رأس_2>::مقدار  //
                          && !std::is_پایه_از<Dfg_رأس_Var, رأس_2>::مقدار,
                      "Should only make operation vertices and constants");

        إذا VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, رأس_2>::مقدار) {
            DfgConst* ثابت vtxp = جديد DfgConst{m_dfg, refp->fileline(), عرض, 0};
            m_sccInfo.جمع_کن(*vtxp, 0);
            أرجع reinterpret_cast<رأس_2*>(vtxp);
        } وإلا {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            استفاده Vtx = نام_نوع std::conditional<std::is_same<DfgConst, رأس_2>::مقدار, DfgSel,
                                                  رأس_2>::type;
            Vtx* ثابت vtxp = جديد Vtx{m_dfg, refp->fileline(), Dfg_داده_Type::packed(عرض)};
            m_sccInfo.جمع_کن(*vtxp, 0);
            أرجع reinterpret_cast<رأس_2*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    Dfg_رأس_Var* createTmp(ثابت حرف* پیشوند, Dfg_رأس* vtxp) {
        File_سطر* ثابت flp = vtxp->fileline();
        Dfg_رأس::ScopeCache scopeCache;
        AstScope* ثابت scopep = vtxp->scopep(scopeCache);
        Dfg_رأس_Var* ثابت varp = m_dfg.makeNewVar(flp, پیشوند, vtxp->dtype(), scopep);
        varp->vscp()->varp()->is_داخلی(درست);
        varp->tmpForp(varp->vscp());
        m_sccInfo.جمع_کن(*varp, 0);
        أرجع varp;
    }

    // Trace drivers of the given packed vertex, at the given bit range.
    Dfg_رأس* trace(Dfg_رأس* vtxp, uint32_t msb, uint32_t lsb) {
        // Many vertices just pass the traced bit range through to one of their sources
        // (see RETURN_RESULT_TAIL). Chains of such vertices can be tens of thousands of
        // vertices long in large designs (e.g.: a wide concatenation), so resolve them
        // in this loop instead of by recursion, which would overflow the C++ stack.
        // 'm_pendingps' holds the cache entries of the vertices on the current chain. They
        // all yield the same result, which is filled in when the chain is resolved.
        // Note: references into 'm_cache' are stable across the insertions below.
        Dfg_رأس* ثابت شروع_Vtxp = vtxp;
        ثابت uint32_t شروع_Msb = msb;
        ثابت uint32_t شروع_Lsb = lsb;
        ثابت اندازه_t pendingBegin = m_pendingps.اندازه();
        Dfg_رأس* resp = اشاره_تهی;

        طالما (درست) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->اندازه() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            إذا (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                Dfg_رأس*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(اشاره_تهی))
                          .اولی->دومی;
                إذا (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    إذا (Dfg_رأس_Splice* ثابت splicep = respr->cast<Dfg_رأس_Splice>()) {
                        Dfg_رأس_Var* ثابت tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    إذا (msb != respr->عرض() - 1 || lsb != 0) {
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
            Dfg_رأس*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(اشاره_تهی))
                                    .اولی->دومی;

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
                m_resp = اشاره_تهی;
                m_tailp = اشاره_تهی;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != اشاره_تهی) != (m_tailp != اشاره_تهی), vtxp,
                            "Expected exactly one trace result");
                إذا (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.افزودن_پشت(&respr);
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
        UASSERT_OBJ(resp, شروع_Vtxp, "Tracing driver failed for " << شروع_Vtxp->typeName());
        UASSERT_OBJ(resp->عرض() == (شروع_Msb - شروع_Lsb + 1), شروع_Vtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        لكل (اندازه_t i = pendingBegin; i < m_pendingps.اندازه(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        أرجع resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    Dfg_رأس* traceSameIdx(Dfg_رأس* ثابت vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.خالی(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = اشاره_تهی;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->عرض() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        أرجع m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    Dfg_رأس* trace_افزودن_Idx(Dfg_رأس* ثابت vtxp, ثابت uint32_t idx) {
        ثابت اندازه_t nIdxs = m_idxs.اندازه();
        m_idxs.افزودن_پشت(idx);
        Dfg_رأس* ثابت resp = traceSameIdx(vtxp);
        m_idxs.برداشت_پشت();
        UASSERT_OBJ(m_idxs.اندازه() == nIdxs, vtxp, "Index stack size mismatch");
        أرجع resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    Dfg_رأس* trace_برداشت_Idx(Dfg_رأس* ثابت vtxp) {
        UASSERT_OBJ(!m_idxs.خالی(), vtxp, "Should have a pending index");
        ثابت اندازه_t nIdxs = m_idxs.اندازه();
        ثابت uint32_t idx = m_idxs.پشت();
        m_idxs.برداشت_پشت();
        Dfg_رأس* ثابت resp = m_idxs.خالی() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.افزودن_پشت(idx);
        UASSERT_OBJ(m_idxs.اندازه() == nIdxs, vtxp, "Index stack size mismatch");
        أرجع resp;
    }

    قالب <نام_نوع رأس_2>
    رأس_2* trace_Bitwise_دودویی(رأس_2* vtxp) {
        static_assert(std::is_پایه_از<Dfg_رأس_دودویی, رأس_2>::مقدار,
                      "Should only call on DfgVertexBinary");
        رأس_2* ثابت resp = make<رأس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        أرجع resp;
    }

    قالب <نام_نوع رأس_2>
    Dfg_رأس* trace_جمع_کن_Sub(رأس_2* vtxp) {
        static_assert(std::is_پایه_از<Dfg_رأس_دودویی, رأس_2>::مقدار,
                      "Should only call on DfgVertexBinary");
        رأس_2* ثابت opp = make<رأس_2>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        أرجع selp;
    }

    قالب <نام_نوع رأس_2>
    رأس_2* traceReduction(رأس_2* vtxp) {
        static_assert(std::is_پایه_از<Dfg_رأس_Unary, رأس_2>::مقدار,
                      "Should only call on DfgVertexUnary");
        رأس_2* ثابت resp = make<رأس_2>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->عرض() - 1, 0));
        أرجع resp;
    }

    قالب <نام_نوع رأس_2>
    رأس_2* traceCmp(رأس_2* vtxp) {
        static_assert(std::is_پایه_از<Dfg_رأس_دودویی, رأس_2>::مقدار,
                      "Should only call on DfgVertexBinary");
        رأس_2* ثابت resp = make<رأس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->عرض() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->عرض() - 1, 0));
        أرجع resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#اگر_تعریف_شده VL_DEBUG
#تعریف_کن TRACE_نتیجه_COVER() \
    افعل { \
        إذا (VL_UNLIKELY(m_سطر_Coverage_File.is_open())) m_سطر_Coverage_File << __LINE__ << '\n'; \
    } طالما (خطأ)
#وإلا
#تعریف_کن TRACE_نتیجه_COVER() \
    افعل { \
    } طالما (خطأ)
#پایان_اگر

#تعریف_کن RETURN_نتیجه(vtxp) \
    افعل { \
        m_resp = (vtxp); \
        TRACE_نتیجه_COVER(); \
        أرجع; \
    } طالما (خطأ)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#تعریف_کن RETURN_نتیجه_دم(srcp, smsb, slsb) \
    افعل { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        Dfg_رأس* ثابت tailp = (srcp); \
        ثابت uint32_t دم_Msb = (smsb); \
        ثابت uint32_t دم_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = دم_Msb; \
        m_lsb = دم_Lsb; \
        TRACE_نتیجه_COVER(); \
        أرجع; \
    } طالما (خطأ)

    // VISITORS
    فراغ بازدید(Dfg_رأس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ بازدید(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        Dfg_رأس* defaultp = m_defaultp;
        m_defaultp = اشاره_تهی;
        m_splicep = اشاره_تهی;

        بنية Driver final {
            Dfg_رأس* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = حذف;
            Driver(Dfg_رأس* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::وکتور<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        منطقي try_کامل_Default = defaultp;
        Dfg_رأس* coverp = اشاره_تهی;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](Dfg_رأس& src, uint32_t lsb) {
            ثابت uint32_t msb = lsb + src.عرض() - 1;
            drivers.emplace_پشت(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            إذا (m_msb >= lsb && msb >= m_lsb) try_کامل_Default = خطأ;
            // If it does not cover the whole searched bit range, move on
            إذا (m_lsb < lsb || msb < m_msb) أرجع خطأ;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            أرجع درست;
        });

        // Trace the driver covering the whole searched range, if there is one
        إذا (coverp) RETURN_نتیجه_دم(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        إذا (try_کامل_Default) RETURN_نتیجه_دم(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::مرتب_سازی(drivers.begin(), drivers.انتها(), [](ثابت Driver& a, ثابت Driver& b) {  //
            أرجع a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::وکتور<Dfg_رأس*> termps;
        uint32_t lsb = m_lsb;
        لكل (ثابت Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            إذا (lsb > driver.m_msb) استمر;
            // Driver is above the searched MSB, done
            إذا (driver.m_lsb > m_msb) اكسر;
            // Gap below this driver, trace default to fill it
            إذا (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_پشت(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ثابت uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_پشت(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        إذا (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_پشت(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.اندازه() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        Dfg_رأس* resp = termps.front();
        لكل (اندازه_t i = 1; i < termps.اندازه(); ++i) {
            Dfg_رأس* ثابت termp = termps[i];
            DfgConcat* ثابت catp = make<DfgConcat>(termp, resp->عرض() + termp->عرض());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_نتیجه(resp);
    }

    فراغ بازدید(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        Dfg_رأس* ثابت defaultp = m_defaultp;
        m_defaultp = اشاره_تهی;
        m_splicep = اشاره_تهی;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ثابت uint32_t idx = m_idxs.پشت();
        إذا (Dfg_رأس* ثابت driverp = vtxp->driverAt(idx)) {
            Dfg_رأس* ثابت srcp = driverp->as<DfgUnitArray>()->srcp();
            إذا (srcp->is<Dfg_رأس_Splice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_نتیجه(trace_برداشت_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_نتیجه(traceSameIdx(defaultp));
    }

    فراغ بازدید(Dfg_رأس_Var* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        Dfg_رأس* ثابت srcp = vtxp->srcp();
        Dfg_رأس* ثابت defaultp = vtxp->defaultp();
        Dfg_رأس* ثابت drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        إذا (srcp && srcp->is<Dfg_رأس_Splice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_نتیجه(m_idxs.خالی() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    فراغ بازدید(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        إذا (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_نتیجه(trace_افزودن_Idx(vtxp->fromp(), idxp->مقصد_U32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.خالی(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ثابت resp = make<DfgArraySel>(vtxp, vtxp->عرض());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->عرض() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_نتیجه(selp);
    }

    فراغ بازدید(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.پشت() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_نتیجه(trace_برداشت_Idx(vtxp->srcp()));
    }

    فراغ بازدید(DfgConcat* vtxp) override {
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t r_عرض = rhsp->عرض();
        // If the traced bits are wholly in the RHS
        إذا (r_عرض > m_msb) RETURN_نتیجه_دم(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        إذا (m_lsb >= r_عرض) RETURN_نتیجه_دم(lhsp, m_msb - r_عرض, m_lsb - r_عرض);
        // The traced bit spans both sides, trace both
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, r_عرض - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - r_عرض, 0));
        RETURN_نتیجه(resp);
    }

    فراغ بازدید(DfgRep* vtxp) override {
        Dfg_رأس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_عرض = srcp->عرض();
        // If we need more bits than the source, then we need the whole source
        إذا (m_msb - m_lsb + 1 > s_عرض) {
            DfgRep* ثابت repp = make<DfgRep>(vtxp, vtxp->عرض());
            repp->srcp(trace(srcp, s_عرض - 1, 0));
            DfgSel* ثابت resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_نتیجه(resp);
        }
        // If the requested bits are within the same repliacted word
        إذا (m_msb / s_عرض == m_lsb / s_عرض) {
            RETURN_نتیجه_دم(srcp, m_msb % s_عرض, m_lsb % s_عرض);
        }
        // The requested bits span two replicated words
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, s_عرض - 1, m_lsb % s_عرض));
        resp->lhsp(trace(srcp, m_msb % s_عرض, 0));
        RETURN_نتیجه(resp);
    }

    فراغ بازدید(DfgExtend* vtxp) override {
        Dfg_رأس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_عرض = srcp->عرض();
        // If the traced bits are wholly in the input
        إذا (s_عرض > m_msb) RETURN_نتیجه_دم(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        إذا (m_lsb >= s_عرض) RETURN_نتیجه(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_عرض - 1, m_lsb));
        RETURN_نتیجه(resp);
    }

    فراغ بازدید(DfgExtendS* vtxp) override {
        Dfg_رأس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_عرض = srcp->عرض();
        // If the traced bits are wholly in the input
        إذا (s_عرض > m_msb) RETURN_نتیجه_دم(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        إذا (m_lsb >= s_عرض) {
            إذا (m_msb == m_lsb) RETURN_نتیجه_دم(srcp, s_عرض - 1, s_عرض - 1);
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, s_عرض - 1, s_عرض - 1));
            RETURN_نتیجه(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_عرض - 1, m_lsb));
        RETURN_نتیجه(resp);
    }

    فراغ بازدید(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        RETURN_نتیجه_دم(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    فراغ بازدید(DfgNot* vtxp) override {
        DfgNot* ثابت resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_نتیجه(resp);
    }

    فراغ بازدید(DfgAnd* vtxp) override { RETURN_نتیجه(trace_Bitwise_دودویی(vtxp)); }
    فراغ بازدید(DfgOr* vtxp) override { RETURN_نتیجه(trace_Bitwise_دودویی(vtxp)); }
    فراغ بازدید(DfgXor* vtxp) override { RETURN_نتیجه(trace_Bitwise_دودویی(vtxp)); }
    فراغ بازدید(Dfg_جمع_کن* vtxp) override { RETURN_نتیجه(trace_جمع_کن_Sub(vtxp)); }
    فراغ بازدید(DfgSub* vtxp) override { RETURN_نتیجه(trace_جمع_کن_Sub(vtxp)); }
    فراغ بازدید(DfgRedAnd* vtxp) override { RETURN_نتیجه(traceReduction(vtxp)); }
    فراغ بازدید(DfgRedOr* vtxp) override { RETURN_نتیجه(traceReduction(vtxp)); }
    فراغ بازدید(DfgRedXor* vtxp) override { RETURN_نتیجه(traceReduction(vtxp)); }
    فراغ بازدید(DfgEq* vtxp) override { RETURN_نتیجه(traceCmp(vtxp)); }
    فراغ بازدید(DfgNeq* vtxp) override { RETURN_نتیجه(traceCmp(vtxp)); }
    فراغ بازدید(DfgLt* vtxp) override { RETURN_نتیجه(traceCmp(vtxp)); }
    فراغ بازدید(DfgLte* vtxp) override { RETURN_نتیجه(traceCmp(vtxp)); }
    فراغ بازدید(DfgGt* vtxp) override { RETURN_نتیجه(traceCmp(vtxp)); }
    فراغ بازدید(DfgGte* vtxp) override { RETURN_نتیجه(traceCmp(vtxp)); }

    فراغ بازدید(Dfg_لغزش_RS* vtxp) override {
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t لغزش_Amnt = rConstp->مقصد_U32();
            // Width of lower half of result
            ثابت uint32_t پایینی_عرض = لغزش_Amnt > vtxp->عرض() ? 0 : vtxp->عرض() - لغزش_Amnt;
            // If the traced bits are wholly in the input
            إذا (پایینی_عرض > m_msb) {
                RETURN_نتیجه_دم(lhsp, m_msb + لغزش_Amnt, m_lsb + لغزش_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (m_lsb >= پایینی_عرض) {
                DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->عرض() - 1, lhsp->عرض() - 1));
                RETURN_نتیجه(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, پایینی_عرض - 1 + لغزش_Amnt, m_lsb + لغزش_Amnt));
            RETURN_نتیجه(resp);
        }

        Dfg_لغزش_RS* ثابت shiftrsp = make<Dfg_لغزش_RS>(vtxp, vtxp->lhsp()->عرض() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->عرض() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->عرض() - 1, m_lsb));
        إذا (m_msb == vtxp->lhsp()->عرض() - 1) RETURN_نتیجه(shiftrsp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_نتیجه(selp);
    }

    فراغ بازدید(Dfg_لغزش_R* vtxp) override {
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t لغزش_Amnt = rConstp->مقصد_U32();
            // Width of lower half of result
            ثابت uint32_t پایینی_عرض = لغزش_Amnt > vtxp->عرض() ? 0 : vtxp->عرض() - لغزش_Amnt;
            // If the traced bits are wholly in the input
            إذا (پایینی_عرض > m_msb) {
                RETURN_نتیجه_دم(lhsp, m_msb + لغزش_Amnt, m_lsb + لغزش_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (m_lsb >= پایینی_عرض) RETURN_نتیجه(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, پایینی_عرض - 1 + لغزش_Amnt, m_lsb + لغزش_Amnt));
            RETURN_نتیجه(resp);
        }

        Dfg_لغزش_R* ثابت shiftrp = make<Dfg_لغزش_R>(vtxp, vtxp->lhsp()->عرض() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->عرض() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->عرض() - 1, m_lsb));
        إذا (m_msb == vtxp->lhsp()->عرض() - 1) RETURN_نتیجه(shiftrp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_نتیجه(selp);
    }

    فراغ بازدید(Dfg_لغزش_L* vtxp) override {
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t لغزش_Amnt = rConstp->مقصد_U32();
            // Width of lower half of result
            ثابت uint32_t پایینی_عرض = لغزش_Amnt > vtxp->عرض() ? vtxp->عرض() : لغزش_Amnt;
            // If the traced bits are wholly in the input
            إذا (m_lsb >= پایینی_عرض) {
                RETURN_نتیجه_دم(lhsp, m_msb - لغزش_Amnt, m_lsb - لغزش_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (پایینی_عرض > m_msb) RETURN_نتیجه(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->عرض() - (m_msb - پایینی_عرض + 1)));
            resp->lhsp(trace(lhsp, m_msb - لغزش_Amnt, پایینی_عرض - لغزش_Amnt));
            RETURN_نتیجه(resp);
        }

        Dfg_لغزش_L* ثابت shiftlp = make<Dfg_لغزش_L>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->عرض() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        إذا (m_lsb == 0) RETURN_نتیجه(shiftlp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_نتیجه(selp);
    }

    فراغ بازدید(DfgCond* vtxp) override {
        DfgCond* ثابت resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->عرض() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_نتیجه(resp);
    }

    فراغ بازدید(Dfg_تطابق_Masked* vtxp) override {
        Dfg_تطابق_Masked* ثابت resp = make<Dfg_تطابق_Masked>(vtxp, vtxp->عرض());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->عرض() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->عرض() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_نتیجه(selp);
    }

#لغو_تعریف RETURN_نتیجه
#لغو_تعریف TRACE_نتیجه_COVER
#لغو_تعریف RETURN_نتیجه_دم

عام:
    // CONSTRUCTOR
    TraceDriver(Dfg_گراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#اگر_تعریف_شده VL_DEBUG
        إذا (v3Global.opt.debugCheck()) {
            m_سطر_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.پیشوند()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_پایه::بیرون | std::ios_پایه::app);
        }
#پایان_اگر
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    Dfg_رأس* apply(Dfg_رأس& vtx, uint32_t lsb, uint32_t عرض) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        أرجع trace(&vtx, lsb + عرض - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
صنف بیت_ماسک final {
    // TYPES
    ثابت Dfg_داده_Type& m_dtype;
    اتحاد {
        V3_عدد m_رقم;  // The bits, if packed
        std::وکتور<بیت_ماسک> m_sub;  // The per element masks, if aggregate
    };

عام:
    // CONSTRUCTOR
    بیت_ماسک(File_سطر* flp, ثابت Dfg_داده_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        إذا (m_dtype.isPacked()) {
            جديد (&m_رقم) V3_عدد{flp, static_cast<صحيح>(m_dtype.اندازه()), 0};
        } وإلا {
            جديد (&m_sub) std::وکتور<بیت_ماسک>{};
            m_sub.reserve(m_dtype.اندازه());
            لكل (uint32_t i = 0; i < m_dtype.اندازه(); ++i)
                m_sub.emplace_پشت(flp, m_dtype.elemDtype());
        }
    }
    بیت_ماسک(ثابت Dfg_رأس& vtx)
        : بیت_ماسک{vtx.fileline(), vtx.dtype()} {}
    بیت_ماسک(ثابت بیت_ماسک& other)
        : m_dtype{other.m_dtype} {
        إذا (m_dtype.isPacked()) {
            جديد (&m_رقم) V3_عدد{other.m_رقم};
        } وإلا {
            جديد (&m_sub) std::وکتور<بیت_ماسک>{other.m_sub};
        }
    }
    بیت_ماسک& عملگر=(ثابت بیت_ماسک& other) {
        إذا (هذا != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            إذا (other.m_dtype.isPacked()) {
                m_رقم = other.m_رقم;
            } وإلا {
                m_sub = other.m_sub;
            }
        }
        أرجع *هذا;
    }
    ~بیت_ماسک() {
        إذا (m_dtype.isPacked()) {
            m_رقم.~V3_عدد();
        } وإلا {
            m_sub.~وکتور<بیت_ماسک>();
        }
    }

    // METHODS
    V3_عدد& رقم() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        أرجع m_رقم;
    }
    ثابت V3_عدد& رقم() ثابت {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        أرجع m_رقم;
    }
    std::وکتور<بیت_ماسک>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        أرجع m_sub;
    }

    منطقي isZero() ثابت {
        إذا (m_dtype.isPacked()) أرجع m_رقم.isEqZero();

        لكل (ثابت بیت_ماسک& sub : m_sub) {
            إذا (!sub.isZero()) أرجع خطأ;
        }
        أرجع درست;
    }
    منطقي isOnes() ثابت {
        إذا (m_dtype.isPacked()) أرجع m_رقم.isEqAllOnes();

        لكل (ثابت بیت_ماسک& sub : m_sub) {
            إذا (!sub.isOnes()) أرجع خطأ;
        }
        أرجع درست;
    }
    فراغ مجموعه_Zero() {
        إذا (m_dtype.isPacked()) {
            m_رقم.مجموعه_All_Bits0();
        } وإلا {
            لكل (بیت_ماسک& sub : m_sub) sub.مجموعه_Zero();
        }
    }
    فراغ مجموعه_Ones() {
        إذا (m_dtype.isPacked()) {
            m_رقم.مجموعه_All_Bits1();
        } وإلا {
            لكل (بیت_ماسک& sub : m_sub) sub.مجموعه_Ones();
        }
    }

    منطقي عملگر==(ثابت بیت_ماسک& other) ثابت {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        إذا (m_dtype.isPacked()) أرجع m_رقم.isCaseEq(other.m_رقم);
        أرجع m_sub == other.m_sub;
    }
    منطقي عملگر!=(ثابت بیت_ماسک& other) ثابت { أرجع !(*هذا == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    منطقي is_Contraction_از(ثابت بیت_ماسک& that) ثابت {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        إذا (m_dtype.isPacked()) {
            ثابت اندازه_t واژگان = VL_واژگان_I(m_dtype.اندازه());
            لكل (اندازه_t i = 0; i < واژگان; ++i) {
                إذا (~m_رقم.edata_کلمه(i) & that.m_رقم.edata_کلمه(i)) أرجع درست;
            }
            أرجع خطأ;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        لكل (اندازه_t i = 0; i < m_sub.اندازه(); ++i) {
            إذا (m_sub[i].is_Contraction_از(that.m_sub[i])) أرجع درست;
        }
        أرجع خطأ;
    }

    std::string مقصد_String() ثابت {
        إذا (m_dtype.isPacked()) أرجع "0x" + m_رقم.displayed(m_رقم.fileline(), "%x");
        std::string نتیجه;
        نتیجه += "{";
        لكل (ثابت بیت_ماسک& sub : m_sub) {
            إذا (&sub != &m_sub.front()) نتیجه += ", ";
            نتیجه += sub.مقصد_String();
        }
        نتیجه += "}";
        أرجع نتیجه;
    }
};

صنف Independent_بیتها final : عام DfgVisitor {
    // TYPES
    بنية رأس_State final {
        اندازه_t m_rpo_عدد = 0;  // Reverse Postorder Number of vertex
        منطقي m_isOnWorkList = خطأ;  // Whether the vertex is on the work list
    };

    // STATE
    ثابت SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ثابت Dfg_رأس*, بیت_ماسک> m_vtxp2_ماسک;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::اولویت_صف<اندازه_t, std::وکتور<اندازه_t>, std::greater<اندازه_t>> m_work_صف;
    std::unordered_map<ثابت Dfg_رأس*, رأس_State> m_vtxp2State;  // State of each vertex

#اگر_تعریف_شده VL_DEBUG
    std::ofstream m_سطر_Coverage_File;  // Line coverage file, just for testing
#پایان_اگر

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    بیت_ماسک& ماسک(ثابت Dfg_رأس& vtx) {
        // Look up (or create) mask for 'vtx'
        أرجع m_vtxp2_ماسک
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .اولی->دومی;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#اگر_تعریف_شده VL_DEBUG
#تعریف_کن ماسک_2(vtxp) \
    ([هذا](ثابت Dfg_رأس& vtx) -> بیت_ماسک& { \
        إذا (VL_UNLIKELY(m_سطر_Coverage_File.is_open())) m_سطر_Coverage_File << __LINE__ << '\n'; \
        أرجع ماسک(vtx); \
    }(*vtxp))
#وإلا
#تعریف_کن ماسک_2(vtxp) ماسک(*vtxp)
#پایان_اگر

    // Clear all bits at or below the most significant clear bit
    فراغ floodTowardsLsb(V3_عدد& رقم) {
        لكل (صحيح i = رقم.عرض() - 1; i >= 0; --i) {
            إذا (رقم.بیت_Is1(i)) استمر;
            رقم.op_مجموعه_Range(0, i + 1, '0');
            اكسر;
        }
    }

    // Clear all bits at or above the least significant clear bit
    فراغ floodTowardsMsb(V3_عدد& رقم) {
        لكل (صحيح i = 0; i < رقم.عرض(); ++i) {
            إذا (رقم.بیت_Is1(i)) استمر;
            رقم.op_مجموعه_Range(i, رقم.عرض() - i, '0');
            اكسر;
        }
    }

    فراغ propagateFromDriver(بیت_ماسک& m, ثابت Dfg_رأس* srcp) {
        // If there is no driver, we are done
        إذا (!srcp) أرجع;
        // If it is driven by a splice, we need to combine the masks of the drivers
        إذا (ثابت DfgSplicePacked* ثابت splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ثابت Dfg_رأس& src, uint32_t lo) {
                m.رقم().opSelInto(ماسک_2(&src).رقم(), lo, src.عرض());
                أرجع خطأ;
            });
            أرجع;
        }
        إذا (ثابت DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ثابت Dfg_رأس& src, uint32_t lo) {
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
    فراغ بازدید(Dfg_رأس* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    فراغ بازدید(Dfg_رأس_Var* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        إذا (vtxp->isVolatile()) أرجع;

        بیت_ماسک& m = ماسک_2(vtxp);
        Dfg_رأس* ثابت srcp = vtxp->srcp();
        Dfg_رأس* ثابت defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        إذا (defaultp) m = ماسک_2(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    فراغ بازدید(Dfg_رأس_Splice* vtxp) override {
        propagateFromDriver(ماسک_2(vtxp), vtxp);  // Needed to continue traversal
    }

    فراغ بازدید(DfgUnitArray* vtxp) override { ماسک_2(vtxp).sub().at(0) = ماسک_2(vtxp->srcp()); }

    فراغ بازدید(DfgArraySel* vtxp) override {
        Dfg_رأس* ثابت fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        إذا (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            ماسک_2(vtxp) = ماسک_2(vtxp->fromp()).sub().at(idxp->مقصد_اندازه_T());
            أرجع;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        إذا (ماسک_2(vtxp->bitp()).isOnes() && ماسک_2(fromp).isOnes()) ماسک_2(vtxp).مجموعه_Ones();
    }

    فراغ بازدید(DfgConcat* vtxp) override {
        ثابت Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        ثابت Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        V3_عدد& m = ماسک_2(vtxp).رقم();
        m.opSelInto(ماسک_2(rhsp).رقم(), 0, rhsp->عرض());
        m.opSelInto(ماسک_2(lhsp).رقم(), rhsp->عرض(), lhsp->عرض());
    }

    فراغ بازدید(DfgRep* vtxp) override {
        ثابت uint32_t شمار = vtxp->شمار();
        ثابت Dfg_رأس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_عرض = srcp->عرض();
        V3_عدد& v_ماسک = ماسک_2(vtxp).رقم();
        V3_عدد& s_ماسک = ماسک_2(srcp).رقم();
        لكل (uint32_t i = 0; i < شمار; ++i) v_ماسک.opSelInto(s_ماسک, i * s_عرض, s_عرض);
    }

    فراغ بازدید(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t msb = lsb + vtxp->عرض() - 1;
        ماسک_2(vtxp).رقم().opSel(ماسک_2(vtxp->fromp()).رقم(), msb, lsb);
    }

    فراغ بازدید(DfgExtend* vtxp) override {
        ثابت Dfg_رأس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_عرض = srcp->عرض();
        V3_عدد& s = ماسک_2(srcp).رقم();
        V3_عدد& m = ماسک_2(vtxp).رقم();
        m.opSelInto(s, 0, s_عرض);
        m.op_مجموعه_Range(s_عرض, vtxp->عرض() - s_عرض, '1');
    }
    فراغ بازدید(DfgExtendS* vtxp) override {
        ثابت Dfg_رأس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_عرض = srcp->عرض();
        V3_عدد& s = ماسک_2(srcp).رقم();
        V3_عدد& m = ماسک_2(vtxp).رقم();
        m.opSelInto(s, 0, s_عرض);
        m.op_مجموعه_Range(s_عرض, vtxp->عرض() - s_عرض, s.بیت_Is0(s_عرض - 1) ? '0' : '1');
    }

    فراغ بازدید(DfgNot* vtxp) override {  //
        ماسک_2(vtxp) = ماسک_2(vtxp->srcp());
    }

    فراغ بازدید(DfgAnd* vtxp) override {  //
        ماسک_2(vtxp).رقم().opAnd(ماسک_2(vtxp->lhsp()).رقم(), ماسک_2(vtxp->rhsp()).رقم());
    }
    فراغ بازدید(DfgOr* vtxp) override {  //
        ماسک_2(vtxp).رقم().opAnd(ماسک_2(vtxp->lhsp()).رقم(), ماسک_2(vtxp->rhsp()).رقم());
    }
    فراغ بازدید(DfgXor* vtxp) override {  //
        ماسک_2(vtxp).رقم().opAnd(ماسک_2(vtxp->lhsp()).رقم(), ماسک_2(vtxp->rhsp()).رقم());
    }

    فراغ بازدید(Dfg_جمع_کن* vtxp) override {
        V3_عدد& m = ماسک_2(vtxp).رقم();
        m.opAnd(ماسک_2(vtxp->lhsp()).رقم(), ماسک_2(vtxp->rhsp()).رقم());
        floodTowardsMsb(m);
    }
    فراغ بازدید(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_عدد& m = ماسک_2(vtxp).رقم();
        m.opAnd(ماسک_2(vtxp->lhsp()).رقم(), ماسک_2(vtxp->rhsp()).رقم());
        floodTowardsMsb(m);
    }

    فراغ بازدید(DfgRedAnd* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }
    فراغ بازدید(DfgRedOr* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }
    فراغ بازدید(DfgRedXor* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }

    فراغ بازدید(DfgEq* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }
    فراغ بازدید(DfgNeq* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }
    فراغ بازدید(DfgLt* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }
    فراغ بازدید(DfgLte* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }
    فراغ بازدید(DfgGt* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }
    فراغ بازدید(DfgGte* vtxp) override {
        ثابت منطقي independent = ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->rhsp()).isOnes();
        إذا (independent) ماسک_2(vtxp).مجموعه_Ones();
    }

    فراغ بازدید(Dfg_لغزش_RS* vtxp) override {
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t عرض = vtxp->عرض();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t لغزش_Amount = rConstp->مقصد_U32();
            إذا (لغزش_Amount >= عرض) {
                إذا (ماسک_2(lhsp).رقم().بیت_Is1(عرض - 1)) { ماسک_2(vtxp).مجموعه_Ones(); }
            } وإلا {
                V3_عدد& m = ماسک_2(vtxp).رقم();
                m.op_لغزش_RS(ماسک_2(lhsp).رقم(), rConstp->رقم(), عرض);
                m.op_مجموعه_Range(عرض - لغزش_Amount, لغزش_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (ماسک_2(rhsp).isOnes()) {
            V3_عدد& m = ماسک_2(vtxp).رقم();
            m = ماسک_2(lhsp).رقم();
            floodTowardsLsb(m);
        }
    }

    فراغ بازدید(Dfg_لغزش_R* vtxp) override {
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t عرض = vtxp->عرض();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t لغزش_Amount = rConstp->مقصد_U32();
            إذا (لغزش_Amount >= عرض) {
                ماسک_2(vtxp).مجموعه_Ones();
            } وإلا {
                V3_عدد& m = ماسک_2(vtxp).رقم();
                m.op_لغزش_R(ماسک_2(lhsp).رقم(), rConstp->رقم());
                m.op_مجموعه_Range(عرض - لغزش_Amount, لغزش_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (ماسک_2(rhsp).isOnes()) {
            V3_عدد& m = ماسک_2(vtxp).رقم();
            m = ماسک_2(lhsp).رقم();
            floodTowardsLsb(m);
        }
    }

    فراغ بازدید(Dfg_لغزش_L* vtxp) override {
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t عرض = vtxp->عرض();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t لغزش_Amount = rConstp->مقصد_U32();
            إذا (لغزش_Amount >= عرض) {
                ماسک_2(vtxp).مجموعه_Ones();
            } وإلا {
                V3_عدد& m = ماسک_2(vtxp).رقم();
                m.op_لغزش_L(ماسک_2(lhsp).رقم(), rConstp->رقم());
                m.op_مجموعه_Range(0, لغزش_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (ماسک_2(rhsp).isOnes()) {
            V3_عدد& m = ماسک_2(vtxp).رقم();
            m = ماسک_2(lhsp).رقم();
            floodTowardsMsb(m);
        }
    }

    فراغ بازدید(DfgCond* vtxp) override {
        إذا (ماسک_2(vtxp->condp()).isOnes()) {
            ماسک_2(vtxp).رقم().opAnd(ماسک_2(vtxp->thenp()).رقم(), ماسک_2(vtxp->elsep()).رقم());
        }
    }

    فراغ بازدید(Dfg_تطابق_Masked* vtxp) override {
        إذا (ماسک_2(vtxp->lhsp()).isOnes() && ماسک_2(vtxp->matchp()).isOnes()) { ماسک_2(vtxp).مجموعه_Ones(); }
    }

#لغو_تعریف ماسک_2

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    فراغ افزودن_به_صف_Sinks(Dfg_رأس& vtx) {
        vtx.foreachSink([&](Dfg_رأس& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            إذا (!m_sccInfo.get(sink)) أرجع خطأ;
            // Otherwise just enqueue it
            رأس_State& state = m_vtxp2State.at(&sink);
            إذا (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_work_صف.افزودن(state.m_rpo_عدد);
            }
            state.m_isOnWorkList = درست;
            أرجع خطأ;
        });
    };

    فراغ dfs_Post_ترتیب(std::unordered_مجموعه<Dfg_رأس*>& دیدهشده,  //
                      std::وکتور<Dfg_رأس*>& post_ترتیب_Enumeration,  //
                      Dfg_رأس& vtx) {
        // Mark visited, skip if already visited
        إذا (!دیدهشده.درج(&vtx).دومی) أرجع;
        // Visit un-visited successors
        vtx.foreachSink([&](Dfg_رأس& snk) {
            dfs_Post_ترتیب(دیدهشده, post_ترتیب_Enumeration, snk);
            أرجع خطأ;
        });
        // Add to post order enumeration
        post_ترتیب_Enumeration.emplace_پشت(&vtx);
    };

    Independent_بیتها(Dfg_گراف& dfg, ثابت SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#اگر_تعریف_شده VL_DEBUG
        إذا (v3Global.opt.debugCheck()) {
            m_سطر_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.پیشوند()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_پایه::بیرون | std::ios_پایه::app);
        }
#پایان_اگر

        // Compute Reverse-Postorder enumeration of vertices
        std::وکتور<Dfg_رأس*> rpoEnumeration;
        std::unordered_مجموعه<Dfg_رأس*> دیدهشده;
        dfg.for_Each_رأس([&](Dfg_رأس& vtx) {
            إذا (!vtx.nInputs()) dfs_Post_ترتیب(دیدهشده, rpoEnumeration, vtx);
        });
        dfg.for_Each_رأس([&](Dfg_رأس& vtx) {  //
            dfs_Post_ترتیب(دیدهشده, rpoEnumeration, vtx);
        });
        std::معکوس(rpoEnumeration.begin(), rpoEnumeration.انتها());
        UASSERT(rpoEnumeration.اندازه() == dfg.اندازه(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        لكل (اندازه_t i = 0; i < rpoEnumeration.اندازه(); ++i) {
            رأس_State& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_عدد = i;
            state.m_isOnWorkList = خطأ;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        لكل (Dfg_رأس* ثابت vtxp : rpoEnumeration) {
            إذا (m_sccInfo.get(*vtxp)) استمر;
            ماسک(*vtxp).مجموعه_Ones();
        }
        لكل (Dfg_رأس* ثابت vtxp : rpoEnumeration) {
            إذا (!m_sccInfo.get(*vtxp)) استمر;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << ماسک(*vtxp).مقصد_String());
            إذا (!ماسک(*vtxp).isZero()) افزودن_به_صف_Sinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        طالما (!m_work_صف.خالی()) {
            // Grab next item
            Dfg_رأس* ثابت currp = rpoEnumeration[m_work_صف.بالاترین()];
            m_work_صف.برداشت();
            m_vtxp2State.at(currp).m_isOnWorkList = خطأ;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ثابت بیت_ماسک& curr_ماسک = ماسک(*currp);
            // Remember current mask so we can check if it changed
            ثابت بیت_ماسک prev_ماسک = curr_ماسک;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            إذا (prev_ماسک != curr_ماسک) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prev_ماسک.مقصد_String()
                             << "\n      to: " << curr_ماسک.مقصد_String());
                افزودن_به_صف_Sinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!curr_ماسک.is_Contraction_از(prev_ماسک), currp,
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
    ساكن std::unordered_map<ثابت Dfg_رأس*, بیت_ماسک> apply(Dfg_گراف& dfg,
                                                               ثابت SccInfo& sccInfo) {
        أرجع std::move(Independent_بیتها{dfg, sccInfo}.m_vtxp2_ماسک);
    }
};

صنف Fix_به_بالا final {
    Dfg_گراف& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ثابت std::unordered_map<ثابت Dfg_رأس*, بیت_ماسک> m_independent_بیتها
        = Independent_بیتها::apply(m_dfg, m_sccInfo);
    اندازه_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ساكن بیت_ماسک compute_استفاده_شده_بیتها(Dfg_رأس& vtx) {
        بیت_ماسک نتیجه{vtx};
        vtx.foreachSink([&نتیجه](Dfg_رأس& sink) {
            // If used via a Sel, mark the selected bits used
            إذا (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                نتیجه.رقم().op_مجموعه_Range(selp->lsb(), selp->عرض(), '1');
                أرجع خطأ;
            }
            // Used without a Sel, so all bits are used
            نتیجه.مجموعه_Ones();
            أرجع درست;
        });
        أرجع نتیجه;
    }

    ساكن std::string debugStr(ثابت Dfg_رأس& vtx) {  // LCOV_EXCL_START
        إذا (ثابت DfgArraySel* ثابت aselp = vtx.cast<DfgArraySel>()) {
            ثابت اندازه_t i = aselp->bitp()->as<DfgConst>()->مقصد_اندازه_T();
            أرجع debugStr(*aselp->fromp()) + "[" + std::مقصد_string(i) + "]";
        }
        إذا (ثابت Dfg_رأس_Var* ثابت varp = vtx.cast<Dfg_رأس_Var>()) {
            أرجع varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    فراغ gatherTerms(std::وکتور<Dfg_رأس*>& termps, Dfg_رأس& vtx, ثابت V3_عدد& indp_بیتها,
                     ثابت uint32_t hi, ثابت uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        منطقي isIndependent = indp_بیتها.بیت_Is1(lo);
        uint32_t lsb = lo;
        لكل (uint32_t msb = lo; msb <= hi; ++msb) {
            ثابت منطقي انتها_Range = msb == hi || isIndependent != indp_بیتها.بیت_Is1(msb + 1);
            إذا (!انتها_Range) استمر;
            ثابت uint32_t عرض = msb - lsb + 1;
            // Compute the term to use for this range
            إذا (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_پشت(m_traceDriver.apply(vtx, lsb, عرض));
            } وإلا {
                // If dependent, fall back on using the part of the variable
                DfgSel* ثابت selp = جديد DfgSel{m_dfg, vtx.fileline(), Dfg_داده_Type::packed(عرض)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.جمع_کن(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_پشت(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    فراغ fix_به_بالا_Packed(Dfg_رأس& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ثابت بیت_ماسک& indp_بیتها = m_independent_بیتها.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indp_بیتها.مقصد_String());
        // Can't do anything if all bits are dependent
        إذا (indp_بیتها.isZero()) أرجع;

        // Figure out which bits of 'vtxp' are used
        ثابت بیت_ماسک استفاده_شده_بیتها = compute_استفاده_شده_بیتها(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << استفاده_شده_بیتها.مقصد_String());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_عدد استفاده_شده_And_Independent{vtx.fileline(), static_cast<صحيح>(vtx.عرض()), 0};
        استفاده_شده_And_Independent.opAnd(استفاده_شده_بیتها.رقم(), indp_بیتها.رقم());
        إذا (استفاده_شده_And_Independent.isEqZero()) أرجع;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::وکتور<Dfg_رأس*> termps;

        // Iterate through consecutive used/unused ranges
        File_سطر* ثابت flp = vtx.fileline();
        ثابت uint32_t عرض = vtx.عرض();
        منطقي is_استفاده_شده = استفاده_شده_بیتها.رقم().بیت_Is1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        لكل (uint32_t msb = 0; msb < عرض; ++msb) {
            ثابت منطقي انتها_Range = msb == عرض - 1 || is_استفاده_شده != استفاده_شده_بیتها.رقم().بیت_Is1(msb + 1);
            إذا (!انتها_Range) استمر;
            إذا (is_استفاده_شده) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indp_بیتها.رقم(), msb, lsb);
            } وإلا {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ثابت constp = جديد DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.جمع_کن(*constp, 0);
                termps.emplace_پشت(constp);
            }
            // Next iteration
            is_استفاده_شده = !is_استفاده_شده;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        Dfg_رأس* replacementp = termps.front();
        ثابت uint64_t vComp = m_sccInfo.get(vtx);
        لكل (اندازه_t i = 1; i < termps.اندازه(); ++i) {
            Dfg_رأس* ثابت termp = termps[i];
            ثابت uint32_t cat_عرض = replacementp->عرض() + termp->عرض();
            DfgConcat* ثابت catp = جديد DfgConcat{m_dfg, flp, Dfg_داده_Type::packed(cat_عرض)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ثابت uint64_t tComp = m_sccInfo.get(*termp);
            ثابت uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.جمع_کن(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
            replacementp = catp;
        }

        // Replace the vertex
        vtx.replaceWith(replacementp);
        // Connect the Sel nodes in the replacement
        لكل (Dfg_رأس* ثابت termp : termps) {
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

    فراغ اصلی(Dfg_رأس_Var& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        إذا (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fix_به_بالا_Packed(var);
        } وإلا إذا (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ثابت uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](Dfg_رأس& sink) {
                // Ignore if sink is not part of same cycle
                إذا (m_sccInfo.get(sink) != component) أرجع خطأ;
                // Only handle ArraySels with constant index
                DfgArraySel* ثابت aselp = sink.cast<DfgArraySel>();
                إذا (!aselp) أرجع خطأ;
                إذا (!aselp->bitp()->is<DfgConst>()) أرجع خطأ;
                // Fix up the word
                fix_به_بالا_Packed(*aselp);
                أرجع خطأ;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    Fix_به_بالا(Dfg_گراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

عام:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ساكن اندازه_t apply(Dfg_گراف& dfg, SccInfo& sccInfo) {
        إذا (!sccInfo.isCyclic()) أرجع 0;

        Fix_به_بالا fix_به_بالا{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        لكل (Dfg_رأس_Var& vtx : dfg.varVertices()) {
            إذا (!sccInfo.get(vtx)) استمر;
            fix_به_بالا.اصلی(vtx);
            إذا (!sccInfo.isCyclic()) اكسر;
        }
        // Return the number of improvements made
        أرجع fix_به_بالا.m_nImprovements;
    }
};

فراغ break_چرخهها(Dfg_گراف& dfg, V3_Dfg_Break_چرخهها_Context& ctx) {
    // Shorthand for dumping graph at given dump level
    ثابت خودکار dump = [&](صحيح level, ثابت Dfg_گراف& dfg, ثابت std::string& name) {
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
        ثابت اندازه_t nFixed = Fix_به_بالا::apply(dfg, sccInfo);
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
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ثابت Dfg_رأس& vtx) {
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
    std::وکتور<Dfg_رأس_Var*> varps;
    لكل (Dfg_رأس_Var& vtx : dfg.varVertices()) {
        إذا (!vtx.tmpForp()) varps.افزودن_پشت(&vtx);
    }
    std::stable_افراز(varps.begin(), varps.انتها(), [](ثابت Dfg_رأس_Var* varp) {
        أرجع varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    لكل (Dfg_رأس_Var* ثابت varp : varps) {
        // Stop as soon as it becomes acyclic
        إذا (!sccInfo.isCyclic()) اكسر;

        // Ignore if variable is not part of a cycle
        إذا (!sccInfo.get(*varp)) استمر;

        // Insert a DfgPrev vertex for the variable
        DfgPrev* ثابت prevp = جديد DfgPrev{dfg, varp->vscp()};
        sccInfo.جمع_کن(*prevp, 0);
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
فراغ V3DfgPasses::break_چرخهها(Dfg_گراف& dfg, V3DfgContext& ctx) {
    V3_Dfg_Break_چرخهها::break_چرخهها(dfg, ctx.m_break_چرخهها_Context);
    إذا (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::remove_استفادهنشده(dfg);
}
