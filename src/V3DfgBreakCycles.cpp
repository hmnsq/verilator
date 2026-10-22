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

نطاق V3_Dfg_Break_چکراں {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
صنف SccInfo final {
    Dfg_گراف& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ثابت Dfg_راس*, uint64_t> m_vtx2Scc;
    ناپ_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

عام:
    SccInfo(Dfg_گراف& dfg)
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
    فراغ جوڑو(ثابت Dfg_راس& vtx, uint64_t sccIdx) {
        ثابت منطقي new_اندراج = m_vtx2Scc.emplace(&vtx, sccIdx).دوجا;
        UASSERT_OBJ(new_اندراج, &vtx, "Vertex already inserted");
        إذا (sccIdx) ++m_nCyclicVertices;
    }

    منطقي stillCyclicFwd(ثابت Dfg_راس& vtx) ثابت {
        // If it only reads non-cyclic vertices, it has also become acyclic
        أرجع vtx.foreach_سروت([&](ثابت Dfg_راس& src) {  //
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
        vtx.foreach_سروت([&](ثابت Dfg_راس& src) {
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
    بنية Cache_کلید final {
        Dfg_راس* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_کلید() = مٹاؤ;
        Cache_کلید(Dfg_راس* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        بنية Hash final {
            ناپ_t عامل()(ثابت Cache_کلید& شے_2) ثابت {
                // cppcheck-suppress unreadVariable
                V3Hash hash{شے_2.m_vtxp};
                hash += شے_2.m_lsb;
                hash += شے_2.m_msb;
                أرجع hash.قدر();
            }
        };

        بنية Equal final {
            منطقي عامل()(ثابت Cache_کلید& a, ثابت Cache_کلید& b) ثابت {
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
    std::سمتیہ<uint32_t> m_idxs;  // Indices to extract from the currently visited Vertex
    std::سمتیہ<Dfg_راس**> m_pendingps;  // Pending pass-through cache entries across trace calls
    // Result of tracing the currently visited Vertex. Use RETURN_RESULT below!
    Dfg_راس* m_resp = خالی_سوچک;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    Dfg_راس* m_tailp = خالی_سوچک;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    Dfg_راس* m_defaultp = خالی_سوچک;
    Dfg_راس* m_splicep = خالی_سوچک;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_کلید, Dfg_راس*, Cache_کلید::Hash, Cache_کلید::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_کلید, Dfg_راس*, Cache_کلید::Hash, Cache_کلید::Equal> m_boundaryCache;

#جے_تعریف_شدہ VL_DEBUG
    std::ofstream m_سطر_Coverage_File;  // Line coverage file, just for testing
#جے_انت

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    سانچا <قسم_ناں راس_2>
    راس_2* make(ثابت Dfg_راس* refp, uint32_t چوڑائی) {
        static_assert(std::is_ادھار_دا<Dfg_راس, راس_2>::قدر  //
                          && !std::is_ادھار_دا<Dfg_راس_Var, راس_2>::قدر,
                      "Should only make operation vertices and constants");

        إذا VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, راس_2>::قدر) {
            DfgConst* ثابت vtxp = جديد DfgConst{m_dfg, refp->fileline(), چوڑائی, 0};
            m_sccInfo.جوڑو(*vtxp, 0);
            أرجع reinterpret_cast<راس_2*>(vtxp);
        } وإلا {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            ورتوں Vtx = قسم_ناں std::conditional<std::is_same<DfgConst, راس_2>::قدر, DfgSel,
                                                  راس_2>::type;
            Vtx* ثابت vtxp = جديد Vtx{m_dfg, refp->fileline(), Dfg_انکڑے_Type::packed(چوڑائی)};
            m_sccInfo.جوڑو(*vtxp, 0);
            أرجع reinterpret_cast<راس_2*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    Dfg_راس_Var* createTmp(ثابت حرف* اگیتر, Dfg_راس* vtxp) {
        File_سطر* ثابت flp = vtxp->fileline();
        Dfg_راس::ScopeCache scopeCache;
        AstScope* ثابت scopep = vtxp->scopep(scopeCache);
        Dfg_راس_Var* ثابت varp = m_dfg.makeNewVar(flp, اگیتر, vtxp->dtype(), scopep);
        varp->vscp()->varp()->is_اندرونی(سچ);
        varp->tmpForp(varp->vscp());
        m_sccInfo.جوڑو(*varp, 0);
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
        Dfg_راس* ثابت شروع_Vtxp = vtxp;
        ثابت uint32_t شروع_Msb = msb;
        ثابت uint32_t شروع_Lsb = lsb;
        ثابت ناپ_t pendingBegin = m_pendingps.ناپ();
        Dfg_راس* resp = خالی_سوچک;

        طالما (سچ) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->ناپ() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            إذا (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                Dfg_راس*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(خالی_سوچک))
                          .پہلا->دوجا;
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
                    إذا (msb != respr->چوڑائی() - 1 || lsb != 0) {
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
                                             std::forward_as_tuple(خالی_سوچک))
                                    .پہلا->دوجا;

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
                m_resp = خالی_سوچک;
                m_tailp = خالی_سوچک;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != خالی_سوچک) != (m_tailp != خالی_سوچک), vtxp,
                            "Expected exactly one trace result");
                إذا (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ڈالو_پٹھ(&respr);
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
        UASSERT_OBJ(resp->چوڑائی() == (شروع_Msb - شروع_Lsb + 1), شروع_Vtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        لكل (ناپ_t i = pendingBegin; i < m_pendingps.ناپ(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        أرجع resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    Dfg_راس* traceSameIdx(Dfg_راس* ثابت vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.خالی_تھاواں(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = خالی_سوچک;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->چوڑائی() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        أرجع m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    Dfg_راس* trace_ڈالو_Idx(Dfg_راس* ثابت vtxp, ثابت uint32_t idx) {
        ثابت ناپ_t nIdxs = m_idxs.ناپ();
        m_idxs.ڈالو_پٹھ(idx);
        Dfg_راس* ثابت resp = traceSameIdx(vtxp);
        m_idxs.نکالو_پٹھ();
        UASSERT_OBJ(m_idxs.ناپ() == nIdxs, vtxp, "Index stack size mismatch");
        أرجع resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    Dfg_راس* trace_نکالو_Idx(Dfg_راس* ثابت vtxp) {
        UASSERT_OBJ(!m_idxs.خالی_تھاواں(), vtxp, "Should have a pending index");
        ثابت ناپ_t nIdxs = m_idxs.ناپ();
        ثابت uint32_t idx = m_idxs.پٹھ();
        m_idxs.نکالو_پٹھ();
        Dfg_راس* ثابت resp = m_idxs.خالی_تھاواں() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ڈالو_پٹھ(idx);
        UASSERT_OBJ(m_idxs.ناپ() == nIdxs, vtxp, "Index stack size mismatch");
        أرجع resp;
    }

    سانچا <قسم_ناں راس_2>
    راس_2* trace_Bitwise_ثنائی(راس_2* vtxp) {
        static_assert(std::is_ادھار_دا<Dfg_راس_ثنائی, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت resp = make<راس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        أرجع resp;
    }

    سانچا <قسم_ناں راس_2>
    Dfg_راس* trace_جوڑو_Sub(راس_2* vtxp) {
        static_assert(std::is_ادھار_دا<Dfg_راس_ثنائی, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت opp = make<راس_2>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        أرجع selp;
    }

    سانچا <قسم_ناں راس_2>
    راس_2* traceReduction(راس_2* vtxp) {
        static_assert(std::is_ادھار_دا<Dfg_راس_Unary, راس_2>::قدر,
                      "Should only call on DfgVertexUnary");
        راس_2* ثابت resp = make<راس_2>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->چوڑائی() - 1, 0));
        أرجع resp;
    }

    سانچا <قسم_ناں راس_2>
    راس_2* traceCmp(راس_2* vtxp) {
        static_assert(std::is_ادھار_دا<Dfg_راس_ثنائی, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت resp = make<راس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->چوڑائی() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->چوڑائی() - 1, 0));
        أرجع resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#جے_تعریف_شدہ VL_DEBUG
#تعریف_کرو TRACE_نتیجہ_COVER() \
    افعل { \
        إذا (VL_UNLIKELY(m_سطر_Coverage_File.is_open())) m_سطر_Coverage_File << __LINE__ << '\n'; \
    } طالما (خطأ)
#وإلا
#تعریف_کرو TRACE_نتیجہ_COVER() \
    افعل { \
    } طالما (خطأ)
#جے_انت

#تعریف_کرو RETURN_نتیجہ(vtxp) \
    افعل { \
        m_resp = (vtxp); \
        TRACE_نتیجہ_COVER(); \
        أرجع; \
    } طالما (خطأ)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#تعریف_کرو RETURN_نتیجہ_پوچھ(srcp, smsb, slsb) \
    افعل { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        Dfg_راس* ثابت tailp = (srcp); \
        ثابت uint32_t پوچھ_Msb = (smsb); \
        ثابت uint32_t پوچھ_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = پوچھ_Msb; \
        m_lsb = پوچھ_Lsb; \
        TRACE_نتیجہ_COVER(); \
        أرجع; \
    } طالما (خطأ)

    // VISITORS
    فراغ ویکھو(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ ویکھو(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        Dfg_راس* defaultp = m_defaultp;
        m_defaultp = خالی_سوچک;
        m_splicep = خالی_سوچک;

        بنية Driver final {
            Dfg_راس* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = مٹاؤ;
            Driver(Dfg_راس* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::سمتیہ<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        منطقي try_سارا_Default = defaultp;
        Dfg_راس* coverp = خالی_سوچک;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](Dfg_راس& src, uint32_t lsb) {
            ثابت uint32_t msb = lsb + src.چوڑائی() - 1;
            drivers.emplace_پٹھ(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            إذا (m_msb >= lsb && msb >= m_lsb) try_سارا_Default = خطأ;
            // If it does not cover the whole searched bit range, move on
            إذا (m_lsb < lsb || msb < m_msb) أرجع خطأ;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            أرجع سچ;
        });

        // Trace the driver covering the whole searched range, if there is one
        إذا (coverp) RETURN_نتیجہ_پوچھ(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        إذا (try_سارا_Default) RETURN_نتیجہ_پوچھ(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::ترتیب_دیو(drivers.begin(), drivers.سرا(), [](ثابت Driver& a, ثابت Driver& b) {  //
            أرجع a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::سمتیہ<Dfg_راس*> termps;
        uint32_t lsb = m_lsb;
        لكل (ثابت Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            إذا (lsb > driver.m_msb) استمر;
            // Driver is above the searched MSB, done
            إذا (driver.m_lsb > m_msb) اكسر;
            // Gap below this driver, trace default to fill it
            إذا (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_پٹھ(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ثابت uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_پٹھ(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        إذا (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_پٹھ(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.ناپ() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        Dfg_راس* resp = termps.front();
        لكل (ناپ_t i = 1; i < termps.ناپ(); ++i) {
            Dfg_راس* ثابت termp = termps[i];
            DfgConcat* ثابت catp = make<DfgConcat>(termp, resp->چوڑائی() + termp->چوڑائی());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_نتیجہ(resp);
    }

    فراغ ویکھو(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        Dfg_راس* ثابت defaultp = m_defaultp;
        m_defaultp = خالی_سوچک;
        m_splicep = خالی_سوچک;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ثابت uint32_t idx = m_idxs.پٹھ();
        إذا (Dfg_راس* ثابت driverp = vtxp->driverAt(idx)) {
            Dfg_راس* ثابت srcp = driverp->as<DfgUnitArray>()->srcp();
            إذا (srcp->is<Dfg_راس_Splice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_نتیجہ(trace_نکالو_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_نتیجہ(traceSameIdx(defaultp));
    }

    فراغ ویکھو(Dfg_راس_Var* vtxp) override {
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
        RETURN_نتیجہ(m_idxs.خالی_تھاواں() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    فراغ ویکھو(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        إذا (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_نتیجہ(trace_ڈالو_Idx(vtxp->fromp(), idxp->منزل_U32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.خالی_تھاواں(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ثابت resp = make<DfgArraySel>(vtxp, vtxp->چوڑائی());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->چوڑائی() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_نتیجہ(selp);
    }

    فراغ ویکھو(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.پٹھ() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_نتیجہ(trace_نکالو_Idx(vtxp->srcp()));
    }

    فراغ ویکھو(DfgConcat* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t r_چوڑائی = rhsp->چوڑائی();
        // If the traced bits are wholly in the RHS
        إذا (r_چوڑائی > m_msb) RETURN_نتیجہ_پوچھ(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        إذا (m_lsb >= r_چوڑائی) RETURN_نتیجہ_پوچھ(lhsp, m_msb - r_چوڑائی, m_lsb - r_چوڑائی);
        // The traced bit spans both sides, trace both
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, r_چوڑائی - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - r_چوڑائی, 0));
        RETURN_نتیجہ(resp);
    }

    فراغ ویکھو(DfgRep* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        // If we need more bits than the source, then we need the whole source
        إذا (m_msb - m_lsb + 1 > s_چوڑائی) {
            DfgRep* ثابت repp = make<DfgRep>(vtxp, vtxp->چوڑائی());
            repp->srcp(trace(srcp, s_چوڑائی - 1, 0));
            DfgSel* ثابت resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_نتیجہ(resp);
        }
        // If the requested bits are within the same repliacted word
        إذا (m_msb / s_چوڑائی == m_lsb / s_چوڑائی) {
            RETURN_نتیجہ_پوچھ(srcp, m_msb % s_چوڑائی, m_lsb % s_چوڑائی);
        }
        // The requested bits span two replicated words
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, s_چوڑائی - 1, m_lsb % s_چوڑائی));
        resp->lhsp(trace(srcp, m_msb % s_چوڑائی, 0));
        RETURN_نتیجہ(resp);
    }

    فراغ ویکھو(DfgExtend* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        // If the traced bits are wholly in the input
        إذا (s_چوڑائی > m_msb) RETURN_نتیجہ_پوچھ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        إذا (m_lsb >= s_چوڑائی) RETURN_نتیجہ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_چوڑائی - 1, m_lsb));
        RETURN_نتیجہ(resp);
    }

    فراغ ویکھو(DfgExtendS* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        // If the traced bits are wholly in the input
        إذا (s_چوڑائی > m_msb) RETURN_نتیجہ_پوچھ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        إذا (m_lsb >= s_چوڑائی) {
            إذا (m_msb == m_lsb) RETURN_نتیجہ_پوچھ(srcp, s_چوڑائی - 1, s_چوڑائی - 1);
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, s_چوڑائی - 1, s_چوڑائی - 1));
            RETURN_نتیجہ(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_چوڑائی - 1, m_lsb));
        RETURN_نتیجہ(resp);
    }

    فراغ ویکھو(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        RETURN_نتیجہ_پوچھ(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    فراغ ویکھو(DfgNot* vtxp) override {
        DfgNot* ثابت resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_نتیجہ(resp);
    }

    فراغ ویکھو(DfgAnd* vtxp) override { RETURN_نتیجہ(trace_Bitwise_ثنائی(vtxp)); }
    فراغ ویکھو(DfgOr* vtxp) override { RETURN_نتیجہ(trace_Bitwise_ثنائی(vtxp)); }
    فراغ ویکھو(DfgXor* vtxp) override { RETURN_نتیجہ(trace_Bitwise_ثنائی(vtxp)); }
    فراغ ویکھو(Dfg_جوڑو* vtxp) override { RETURN_نتیجہ(trace_جوڑو_Sub(vtxp)); }
    فراغ ویکھو(DfgSub* vtxp) override { RETURN_نتیجہ(trace_جوڑو_Sub(vtxp)); }
    فراغ ویکھو(DfgRedAnd* vtxp) override { RETURN_نتیجہ(traceReduction(vtxp)); }
    فراغ ویکھو(DfgRedOr* vtxp) override { RETURN_نتیجہ(traceReduction(vtxp)); }
    فراغ ویکھو(DfgRedXor* vtxp) override { RETURN_نتیجہ(traceReduction(vtxp)); }
    فراغ ویکھو(DfgEq* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    فراغ ویکھو(DfgNeq* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    فراغ ویکھو(DfgLt* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    فراغ ویکھو(DfgLte* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    فراغ ویکھو(DfgGt* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    فراغ ویکھو(DfgGte* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }

    فراغ ویکھو(Dfg_کھسکاؤ_RS* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t کھسکاؤ_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t ہیٹھلا_چوڑائی = کھسکاؤ_Amnt > vtxp->چوڑائی() ? 0 : vtxp->چوڑائی() - کھسکاؤ_Amnt;
            // If the traced bits are wholly in the input
            إذا (ہیٹھلا_چوڑائی > m_msb) {
                RETURN_نتیجہ_پوچھ(lhsp, m_msb + کھسکاؤ_Amnt, m_lsb + کھسکاؤ_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (m_lsb >= ہیٹھلا_چوڑائی) {
                DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->چوڑائی() - 1, lhsp->چوڑائی() - 1));
                RETURN_نتیجہ(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, ہیٹھلا_چوڑائی - 1 + کھسکاؤ_Amnt, m_lsb + کھسکاؤ_Amnt));
            RETURN_نتیجہ(resp);
        }

        Dfg_کھسکاؤ_RS* ثابت shiftrsp = make<Dfg_کھسکاؤ_RS>(vtxp, vtxp->lhsp()->چوڑائی() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->چوڑائی() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->چوڑائی() - 1, m_lsb));
        إذا (m_msb == vtxp->lhsp()->چوڑائی() - 1) RETURN_نتیجہ(shiftrsp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_نتیجہ(selp);
    }

    فراغ ویکھو(Dfg_کھسکاؤ_R* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t کھسکاؤ_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t ہیٹھلا_چوڑائی = کھسکاؤ_Amnt > vtxp->چوڑائی() ? 0 : vtxp->چوڑائی() - کھسکاؤ_Amnt;
            // If the traced bits are wholly in the input
            إذا (ہیٹھلا_چوڑائی > m_msb) {
                RETURN_نتیجہ_پوچھ(lhsp, m_msb + کھسکاؤ_Amnt, m_lsb + کھسکاؤ_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (m_lsb >= ہیٹھلا_چوڑائی) RETURN_نتیجہ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, ہیٹھلا_چوڑائی - 1 + کھسکاؤ_Amnt, m_lsb + کھسکاؤ_Amnt));
            RETURN_نتیجہ(resp);
        }

        Dfg_کھسکاؤ_R* ثابت shiftrp = make<Dfg_کھسکاؤ_R>(vtxp, vtxp->lhsp()->چوڑائی() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->چوڑائی() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->چوڑائی() - 1, m_lsb));
        إذا (m_msb == vtxp->lhsp()->چوڑائی() - 1) RETURN_نتیجہ(shiftrp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_نتیجہ(selp);
    }

    فراغ ویکھو(Dfg_کھسکاؤ_L* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t کھسکاؤ_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t ہیٹھلا_چوڑائی = کھسکاؤ_Amnt > vtxp->چوڑائی() ? vtxp->چوڑائی() : کھسکاؤ_Amnt;
            // If the traced bits are wholly in the input
            إذا (m_lsb >= ہیٹھلا_چوڑائی) {
                RETURN_نتیجہ_پوچھ(lhsp, m_msb - کھسکاؤ_Amnt, m_lsb - کھسکاؤ_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (ہیٹھلا_چوڑائی > m_msb) RETURN_نتیجہ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->چوڑائی() - (m_msb - ہیٹھلا_چوڑائی + 1)));
            resp->lhsp(trace(lhsp, m_msb - کھسکاؤ_Amnt, ہیٹھلا_چوڑائی - کھسکاؤ_Amnt));
            RETURN_نتیجہ(resp);
        }

        Dfg_کھسکاؤ_L* ثابت shiftlp = make<Dfg_کھسکاؤ_L>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->چوڑائی() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        إذا (m_lsb == 0) RETURN_نتیجہ(shiftlp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_نتیجہ(selp);
    }

    فراغ ویکھو(DfgCond* vtxp) override {
        DfgCond* ثابت resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->چوڑائی() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_نتیجہ(resp);
    }

    فراغ ویکھو(Dfg_میل_Masked* vtxp) override {
        Dfg_میل_Masked* ثابت resp = make<Dfg_میل_Masked>(vtxp, vtxp->چوڑائی());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->چوڑائی() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->چوڑائی() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_نتیجہ(selp);
    }

#تعریف_رد RETURN_نتیجہ
#تعریف_رد TRACE_نتیجہ_COVER
#تعریف_رد RETURN_نتیجہ_پوچھ

عام:
    // CONSTRUCTOR
    TraceDriver(Dfg_گراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#جے_تعریف_شدہ VL_DEBUG
        إذا (v3Global.opt.debugCheck()) {
            m_سطر_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.اگیتر()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_ادھار::باہرلا | std::ios_ادھار::app);
        }
#جے_انت
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    Dfg_راس* apply(Dfg_راس& vtx, uint32_t lsb, uint32_t چوڑائی) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        أرجع trace(&vtx, lsb + چوڑائی - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
صنف ثنائی_ہندسہ_نقاب final {
    // TYPES
    ثابت Dfg_انکڑے_Type& m_dtype;
    اتحاد {
        V3_نمبر m_عدد;  // The bits, if packed
        std::سمتیہ<ثنائی_ہندسہ_نقاب> m_sub;  // The per element masks, if aggregate
    };

عام:
    // CONSTRUCTOR
    ثنائی_ہندسہ_نقاب(File_سطر* flp, ثابت Dfg_انکڑے_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        إذا (m_dtype.isPacked()) {
            جديد (&m_عدد) V3_نمبر{flp, static_cast<صحيح>(m_dtype.ناپ()), 0};
        } وإلا {
            جديد (&m_sub) std::سمتیہ<ثنائی_ہندسہ_نقاب>{};
            m_sub.reserve(m_dtype.ناپ());
            لكل (uint32_t i = 0; i < m_dtype.ناپ(); ++i)
                m_sub.emplace_پٹھ(flp, m_dtype.elemDtype());
        }
    }
    ثنائی_ہندسہ_نقاب(ثابت Dfg_راس& vtx)
        : ثنائی_ہندسہ_نقاب{vtx.fileline(), vtx.dtype()} {}
    ثنائی_ہندسہ_نقاب(ثابت ثنائی_ہندسہ_نقاب& other)
        : m_dtype{other.m_dtype} {
        إذا (m_dtype.isPacked()) {
            جديد (&m_عدد) V3_نمبر{other.m_عدد};
        } وإلا {
            جديد (&m_sub) std::سمتیہ<ثنائی_ہندسہ_نقاب>{other.m_sub};
        }
    }
    ثنائی_ہندسہ_نقاب& عامل=(ثابت ثنائی_ہندسہ_نقاب& other) {
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
    ~ثنائی_ہندسہ_نقاب() {
        إذا (m_dtype.isPacked()) {
            m_عدد.~V3_نمبر();
        } وإلا {
            m_sub.~سمتیہ<ثنائی_ہندسہ_نقاب>();
        }
    }

    // METHODS
    V3_نمبر& عدد() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        أرجع m_عدد;
    }
    ثابت V3_نمبر& عدد() ثابت {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        أرجع m_عدد;
    }
    std::سمتیہ<ثنائی_ہندسہ_نقاب>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        أرجع m_sub;
    }

    منطقي isZero() ثابت {
        إذا (m_dtype.isPacked()) أرجع m_عدد.isEqZero();

        لكل (ثابت ثنائی_ہندسہ_نقاب& sub : m_sub) {
            إذا (!sub.isZero()) أرجع خطأ;
        }
        أرجع سچ;
    }
    منطقي isOnes() ثابت {
        إذا (m_dtype.isPacked()) أرجع m_عدد.isEqAllOnes();

        لكل (ثابت ثنائی_ہندسہ_نقاب& sub : m_sub) {
            إذا (!sub.isOnes()) أرجع خطأ;
        }
        أرجع سچ;
    }
    فراغ سیٹ_Zero() {
        إذا (m_dtype.isPacked()) {
            m_عدد.سیٹ_All_Bits0();
        } وإلا {
            لكل (ثنائی_ہندسہ_نقاب& sub : m_sub) sub.سیٹ_Zero();
        }
    }
    فراغ سیٹ_Ones() {
        إذا (m_dtype.isPacked()) {
            m_عدد.سیٹ_All_Bits1();
        } وإلا {
            لكل (ثنائی_ہندسہ_نقاب& sub : m_sub) sub.سیٹ_Ones();
        }
    }

    منطقي عامل==(ثابت ثنائی_ہندسہ_نقاب& other) ثابت {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        إذا (m_dtype.isPacked()) أرجع m_عدد.isCaseEq(other.m_عدد);
        أرجع m_sub == other.m_sub;
    }
    منطقي عامل!=(ثابت ثنائی_ہندسہ_نقاب& other) ثابت { أرجع !(*هذا == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    منطقي is_Contraction_دا(ثابت ثنائی_ہندسہ_نقاب& that) ثابت {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        إذا (m_dtype.isPacked()) {
            ثابت ناپ_t الفاظ = VL_الفاظ_I(m_dtype.ناپ());
            لكل (ناپ_t i = 0; i < الفاظ; ++i) {
                إذا (~m_عدد.edata_لفظ(i) & that.m_عدد.edata_لفظ(i)) أرجع سچ;
            }
            أرجع خطأ;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        لكل (ناپ_t i = 0; i < m_sub.ناپ(); ++i) {
            إذا (m_sub[i].is_Contraction_دا(that.m_sub[i])) أرجع سچ;
        }
        أرجع خطأ;
    }

    std::string منزل_String() ثابت {
        إذا (m_dtype.isPacked()) أرجع "0x" + m_عدد.displayed(m_عدد.fileline(), "%x");
        std::string نتیجہ;
        نتیجہ += "{";
        لكل (ثابت ثنائی_ہندسہ_نقاب& sub : m_sub) {
            إذا (&sub != &m_sub.front()) نتیجہ += ", ";
            نتیجہ += sub.منزل_String();
        }
        نتیجہ += "}";
        أرجع نتیجہ;
    }
};

صنف Independent_بٹ final : عام DfgVisitor {
    // TYPES
    بنية راس_State final {
        ناپ_t m_rpo_نمبر = 0;  // Reverse Postorder Number of vertex
        منطقي m_isOnWorkList = خطأ;  // Whether the vertex is on the work list
    };

    // STATE
    ثابت SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ثابت Dfg_راس*, ثنائی_ہندسہ_نقاب> m_vtxp2_نقاب;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::ترجیح_قطار<ناپ_t, std::سمتیہ<ناپ_t>, std::greater<ناپ_t>> m_work_قطار;
    std::unordered_map<ثابت Dfg_راس*, راس_State> m_vtxp2State;  // State of each vertex

#جے_تعریف_شدہ VL_DEBUG
    std::ofstream m_سطر_Coverage_File;  // Line coverage file, just for testing
#جے_انت

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    ثنائی_ہندسہ_نقاب& نقاب(ثابت Dfg_راس& vtx) {
        // Look up (or create) mask for 'vtx'
        أرجع m_vtxp2_نقاب
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .پہلا->دوجا;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#جے_تعریف_شدہ VL_DEBUG
#تعریف_کرو نقاب_2(vtxp) \
    ([هذا](ثابت Dfg_راس& vtx) -> ثنائی_ہندسہ_نقاب& { \
        إذا (VL_UNLIKELY(m_سطر_Coverage_File.is_open())) m_سطر_Coverage_File << __LINE__ << '\n'; \
        أرجع نقاب(vtx); \
    }(*vtxp))
#وإلا
#تعریف_کرو نقاب_2(vtxp) نقاب(*vtxp)
#جے_انت

    // Clear all bits at or below the most significant clear bit
    فراغ floodTowardsLsb(V3_نمبر& عدد) {
        لكل (صحيح i = عدد.چوڑائی() - 1; i >= 0; --i) {
            إذا (عدد.ثنائی_ہندسہ_Is1(i)) استمر;
            عدد.op_سیٹ_Range(0, i + 1, '0');
            اكسر;
        }
    }

    // Clear all bits at or above the least significant clear bit
    فراغ floodTowardsMsb(V3_نمبر& عدد) {
        لكل (صحيح i = 0; i < عدد.چوڑائی(); ++i) {
            إذا (عدد.ثنائی_ہندسہ_Is1(i)) استمر;
            عدد.op_سیٹ_Range(i, عدد.چوڑائی() - i, '0');
            اكسر;
        }
    }

    فراغ propagateFromDriver(ثنائی_ہندسہ_نقاب& m, ثابت Dfg_راس* srcp) {
        // If there is no driver, we are done
        إذا (!srcp) أرجع;
        // If it is driven by a splice, we need to combine the masks of the drivers
        إذا (ثابت DfgSplicePacked* ثابت splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
                m.عدد().opSelInto(نقاب_2(&src).عدد(), lo, src.چوڑائی());
                أرجع خطأ;
            });
            أرجع;
        }
        إذا (ثابت DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
                إذا (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = نقاب_2(uap->srcp());
                } وإلا {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                أرجع خطأ;
            });
            أرجع;
        }
        // Otherwise, we just use the mask of the single driver
        m = نقاب_2(srcp);
    }

    // VISITORS
    فراغ ویکھو(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    فراغ ویکھو(Dfg_راس_Var* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        إذا (vtxp->isVolatile()) أرجع;

        ثنائی_ہندسہ_نقاب& m = نقاب_2(vtxp);
        Dfg_راس* ثابت srcp = vtxp->srcp();
        Dfg_راس* ثابت defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        إذا (defaultp) m = نقاب_2(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    فراغ ویکھو(Dfg_راس_Splice* vtxp) override {
        propagateFromDriver(نقاب_2(vtxp), vtxp);  // Needed to continue traversal
    }

    فراغ ویکھو(DfgUnitArray* vtxp) override { نقاب_2(vtxp).sub().at(0) = نقاب_2(vtxp->srcp()); }

    فراغ ویکھو(DfgArraySel* vtxp) override {
        Dfg_راس* ثابت fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        إذا (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            نقاب_2(vtxp) = نقاب_2(vtxp->fromp()).sub().at(idxp->منزل_ناپ_T());
            أرجع;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        إذا (نقاب_2(vtxp->bitp()).isOnes() && نقاب_2(fromp).isOnes()) نقاب_2(vtxp).سیٹ_Ones();
    }

    فراغ ویکھو(DfgConcat* vtxp) override {
        ثابت Dfg_راس* ثابت rhsp = vtxp->rhsp();
        ثابت Dfg_راس* ثابت lhsp = vtxp->lhsp();
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opSelInto(نقاب_2(rhsp).عدد(), 0, rhsp->چوڑائی());
        m.opSelInto(نقاب_2(lhsp).عدد(), rhsp->چوڑائی(), lhsp->چوڑائی());
    }

    فراغ ویکھو(DfgRep* vtxp) override {
        ثابت uint32_t گنتی = vtxp->گنتی();
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        V3_نمبر& v_نقاب = نقاب_2(vtxp).عدد();
        V3_نمبر& s_نقاب = نقاب_2(srcp).عدد();
        لكل (uint32_t i = 0; i < گنتی; ++i) v_نقاب.opSelInto(s_نقاب, i * s_چوڑائی, s_چوڑائی);
    }

    فراغ ویکھو(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t msb = lsb + vtxp->چوڑائی() - 1;
        نقاب_2(vtxp).عدد().opSel(نقاب_2(vtxp->fromp()).عدد(), msb, lsb);
    }

    فراغ ویکھو(DfgExtend* vtxp) override {
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        V3_نمبر& s = نقاب_2(srcp).عدد();
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opSelInto(s, 0, s_چوڑائی);
        m.op_سیٹ_Range(s_چوڑائی, vtxp->چوڑائی() - s_چوڑائی, '1');
    }
    فراغ ویکھو(DfgExtendS* vtxp) override {
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        V3_نمبر& s = نقاب_2(srcp).عدد();
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opSelInto(s, 0, s_چوڑائی);
        m.op_سیٹ_Range(s_چوڑائی, vtxp->چوڑائی() - s_چوڑائی, s.ثنائی_ہندسہ_Is0(s_چوڑائی - 1) ? '0' : '1');
    }

    فراغ ویکھو(DfgNot* vtxp) override {  //
        نقاب_2(vtxp) = نقاب_2(vtxp->srcp());
    }

    فراغ ویکھو(DfgAnd* vtxp) override {  //
        نقاب_2(vtxp).عدد().opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
    }
    فراغ ویکھو(DfgOr* vtxp) override {  //
        نقاب_2(vtxp).عدد().opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
    }
    فراغ ویکھو(DfgXor* vtxp) override {  //
        نقاب_2(vtxp).عدد().opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
    }

    فراغ ویکھو(Dfg_جوڑو* vtxp) override {
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
        floodTowardsMsb(m);
    }
    فراغ ویکھو(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
        floodTowardsMsb(m);
    }

    فراغ ویکھو(DfgRedAnd* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    فراغ ویکھو(DfgRedOr* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    فراغ ویکھو(DfgRedXor* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }

    فراغ ویکھو(DfgEq* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    فراغ ویکھو(DfgNeq* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    فراغ ویکھو(DfgLt* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    فراغ ویکھو(DfgLte* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    فراغ ویکھو(DfgGt* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    فراغ ویکھو(DfgGte* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سیٹ_Ones();
    }

    فراغ ویکھو(Dfg_کھسکاؤ_RS* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t کھسکاؤ_Amount = rConstp->منزل_U32();
            إذا (کھسکاؤ_Amount >= چوڑائی) {
                إذا (نقاب_2(lhsp).عدد().ثنائی_ہندسہ_Is1(چوڑائی - 1)) { نقاب_2(vtxp).سیٹ_Ones(); }
            } وإلا {
                V3_نمبر& m = نقاب_2(vtxp).عدد();
                m.op_کھسکاؤ_RS(نقاب_2(lhsp).عدد(), rConstp->عدد(), چوڑائی);
                m.op_سیٹ_Range(چوڑائی - کھسکاؤ_Amount, کھسکاؤ_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (نقاب_2(rhsp).isOnes()) {
            V3_نمبر& m = نقاب_2(vtxp).عدد();
            m = نقاب_2(lhsp).عدد();
            floodTowardsLsb(m);
        }
    }

    فراغ ویکھو(Dfg_کھسکاؤ_R* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t کھسکاؤ_Amount = rConstp->منزل_U32();
            إذا (کھسکاؤ_Amount >= چوڑائی) {
                نقاب_2(vtxp).سیٹ_Ones();
            } وإلا {
                V3_نمبر& m = نقاب_2(vtxp).عدد();
                m.op_کھسکاؤ_R(نقاب_2(lhsp).عدد(), rConstp->عدد());
                m.op_سیٹ_Range(چوڑائی - کھسکاؤ_Amount, کھسکاؤ_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (نقاب_2(rhsp).isOnes()) {
            V3_نمبر& m = نقاب_2(vtxp).عدد();
            m = نقاب_2(lhsp).عدد();
            floodTowardsLsb(m);
        }
    }

    فراغ ویکھو(Dfg_کھسکاؤ_L* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t کھسکاؤ_Amount = rConstp->منزل_U32();
            إذا (کھسکاؤ_Amount >= چوڑائی) {
                نقاب_2(vtxp).سیٹ_Ones();
            } وإلا {
                V3_نمبر& m = نقاب_2(vtxp).عدد();
                m.op_کھسکاؤ_L(نقاب_2(lhsp).عدد(), rConstp->عدد());
                m.op_سیٹ_Range(0, کھسکاؤ_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (نقاب_2(rhsp).isOnes()) {
            V3_نمبر& m = نقاب_2(vtxp).عدد();
            m = نقاب_2(lhsp).عدد();
            floodTowardsMsb(m);
        }
    }

    فراغ ویکھو(DfgCond* vtxp) override {
        إذا (نقاب_2(vtxp->condp()).isOnes()) {
            نقاب_2(vtxp).عدد().opAnd(نقاب_2(vtxp->thenp()).عدد(), نقاب_2(vtxp->elsep()).عدد());
        }
    }

    فراغ ویکھو(Dfg_میل_Masked* vtxp) override {
        إذا (نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->matchp()).isOnes()) { نقاب_2(vtxp).سیٹ_Ones(); }
    }

#تعریف_رد نقاب_2

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    فراغ قطار_وچ_پاؤ_Sinks(Dfg_راس& vtx) {
        vtx.foreachSink([&](Dfg_راس& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            إذا (!m_sccInfo.get(sink)) أرجع خطأ;
            // Otherwise just enqueue it
            راس_State& state = m_vtxp2State.at(&sink);
            إذا (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_work_قطار.ڈالو(state.m_rpo_نمبر);
            }
            state.m_isOnWorkList = سچ;
            أرجع خطأ;
        });
    };

    فراغ dfs_Post_ترتیب(std::unordered_سیٹ<Dfg_راس*>& ویکھیا,  //
                      std::سمتیہ<Dfg_راس*>& post_ترتیب_Enumeration,  //
                      Dfg_راس& vtx) {
        // Mark visited, skip if already visited
        إذا (!ویکھیا.داخل_کرو(&vtx).دوجا) أرجع;
        // Visit un-visited successors
        vtx.foreachSink([&](Dfg_راس& snk) {
            dfs_Post_ترتیب(ویکھیا, post_ترتیب_Enumeration, snk);
            أرجع خطأ;
        });
        // Add to post order enumeration
        post_ترتیب_Enumeration.emplace_پٹھ(&vtx);
    };

    Independent_بٹ(Dfg_گراف& dfg, ثابت SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#جے_تعریف_شدہ VL_DEBUG
        إذا (v3Global.opt.debugCheck()) {
            m_سطر_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.اگیتر()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_ادھار::باہرلا | std::ios_ادھار::app);
        }
#جے_انت

        // Compute Reverse-Postorder enumeration of vertices
        std::سمتیہ<Dfg_راس*> rpoEnumeration;
        std::unordered_سیٹ<Dfg_راس*> ویکھیا;
        dfg.for_Each_راس([&](Dfg_راس& vtx) {
            إذا (!vtx.nInputs()) dfs_Post_ترتیب(ویکھیا, rpoEnumeration, vtx);
        });
        dfg.for_Each_راس([&](Dfg_راس& vtx) {  //
            dfs_Post_ترتیب(ویکھیا, rpoEnumeration, vtx);
        });
        std::الٹ(rpoEnumeration.begin(), rpoEnumeration.سرا());
        UASSERT(rpoEnumeration.ناپ() == dfg.ناپ(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        لكل (ناپ_t i = 0; i < rpoEnumeration.ناپ(); ++i) {
            راس_State& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_نمبر = i;
            state.m_isOnWorkList = خطأ;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        لكل (Dfg_راس* ثابت vtxp : rpoEnumeration) {
            إذا (m_sccInfo.get(*vtxp)) استمر;
            نقاب(*vtxp).سیٹ_Ones();
        }
        لكل (Dfg_راس* ثابت vtxp : rpoEnumeration) {
            إذا (!m_sccInfo.get(*vtxp)) استمر;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << نقاب(*vtxp).منزل_String());
            إذا (!نقاب(*vtxp).isZero()) قطار_وچ_پاؤ_Sinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        طالما (!m_work_قطار.خالی_تھاواں()) {
            // Grab next item
            Dfg_راس* ثابت currp = rpoEnumeration[m_work_قطار.چوٹی()];
            m_work_قطار.نکالو();
            m_vtxp2State.at(currp).m_isOnWorkList = خطأ;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ثابت ثنائی_ہندسہ_نقاب& curr_نقاب = نقاب(*currp);
            // Remember current mask so we can check if it changed
            ثابت ثنائی_ہندسہ_نقاب prev_نقاب = curr_نقاب;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            إذا (prev_نقاب != curr_نقاب) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prev_نقاب.منزل_String()
                             << "\n      to: " << curr_نقاب.منزل_String());
                قطار_وچ_پاؤ_Sinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!curr_نقاب.is_Contraction_دا(prev_نقاب), currp,
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
    ساكن std::unordered_map<ثابت Dfg_راس*, ثنائی_ہندسہ_نقاب> apply(Dfg_گراف& dfg,
                                                               ثابت SccInfo& sccInfo) {
        أرجع std::move(Independent_بٹ{dfg, sccInfo}.m_vtxp2_نقاب);
    }
};

صنف Fix_اتے final {
    Dfg_گراف& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ثابت std::unordered_map<ثابت Dfg_راس*, ثنائی_ہندسہ_نقاب> m_independent_بٹ
        = Independent_بٹ::apply(m_dfg, m_sccInfo);
    ناپ_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ساكن ثنائی_ہندسہ_نقاب compute_ورتیا_بٹ(Dfg_راس& vtx) {
        ثنائی_ہندسہ_نقاب نتیجہ{vtx};
        vtx.foreachSink([&نتیجہ](Dfg_راس& sink) {
            // If used via a Sel, mark the selected bits used
            إذا (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                نتیجہ.عدد().op_سیٹ_Range(selp->lsb(), selp->چوڑائی(), '1');
                أرجع خطأ;
            }
            // Used without a Sel, so all bits are used
            نتیجہ.سیٹ_Ones();
            أرجع سچ;
        });
        أرجع نتیجہ;
    }

    ساكن std::string debugStr(ثابت Dfg_راس& vtx) {  // LCOV_EXCL_START
        إذا (ثابت DfgArraySel* ثابت aselp = vtx.cast<DfgArraySel>()) {
            ثابت ناپ_t i = aselp->bitp()->as<DfgConst>()->منزل_ناپ_T();
            أرجع debugStr(*aselp->fromp()) + "[" + std::منزل_string(i) + "]";
        }
        إذا (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
            أرجع varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    فراغ gatherTerms(std::سمتیہ<Dfg_راس*>& termps, Dfg_راس& vtx, ثابت V3_نمبر& indp_بٹ,
                     ثابت uint32_t hi, ثابت uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        منطقي isIndependent = indp_بٹ.ثنائی_ہندسہ_Is1(lo);
        uint32_t lsb = lo;
        لكل (uint32_t msb = lo; msb <= hi; ++msb) {
            ثابت منطقي سرا_Range = msb == hi || isIndependent != indp_بٹ.ثنائی_ہندسہ_Is1(msb + 1);
            إذا (!سرا_Range) استمر;
            ثابت uint32_t چوڑائی = msb - lsb + 1;
            // Compute the term to use for this range
            إذا (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_پٹھ(m_traceDriver.apply(vtx, lsb, چوڑائی));
            } وإلا {
                // If dependent, fall back on using the part of the variable
                DfgSel* ثابت selp = جديد DfgSel{m_dfg, vtx.fileline(), Dfg_انکڑے_Type::packed(چوڑائی)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.جوڑو(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_پٹھ(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    فراغ fix_اتے_Packed(Dfg_راس& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ثابت ثنائی_ہندسہ_نقاب& indp_بٹ = m_independent_بٹ.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indp_بٹ.منزل_String());
        // Can't do anything if all bits are dependent
        إذا (indp_بٹ.isZero()) أرجع;

        // Figure out which bits of 'vtxp' are used
        ثابت ثنائی_ہندسہ_نقاب ورتیا_بٹ = compute_ورتیا_بٹ(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << ورتیا_بٹ.منزل_String());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_نمبر ورتیا_And_Independent{vtx.fileline(), static_cast<صحيح>(vtx.چوڑائی()), 0};
        ورتیا_And_Independent.opAnd(ورتیا_بٹ.عدد(), indp_بٹ.عدد());
        إذا (ورتیا_And_Independent.isEqZero()) أرجع;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::سمتیہ<Dfg_راس*> termps;

        // Iterate through consecutive used/unused ranges
        File_سطر* ثابت flp = vtx.fileline();
        ثابت uint32_t چوڑائی = vtx.چوڑائی();
        منطقي is_ورتیا = ورتیا_بٹ.عدد().ثنائی_ہندسہ_Is1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        لكل (uint32_t msb = 0; msb < چوڑائی; ++msb) {
            ثابت منطقي سرا_Range = msb == چوڑائی - 1 || is_ورتیا != ورتیا_بٹ.عدد().ثنائی_ہندسہ_Is1(msb + 1);
            إذا (!سرا_Range) استمر;
            إذا (is_ورتیا) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indp_بٹ.عدد(), msb, lsb);
            } وإلا {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ثابت constp = جديد DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.جوڑو(*constp, 0);
                termps.emplace_پٹھ(constp);
            }
            // Next iteration
            is_ورتیا = !is_ورتیا;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        Dfg_راس* replacementp = termps.front();
        ثابت uint64_t vComp = m_sccInfo.get(vtx);
        لكل (ناپ_t i = 1; i < termps.ناپ(); ++i) {
            Dfg_راس* ثابت termp = termps[i];
            ثابت uint32_t cat_چوڑائی = replacementp->چوڑائی() + termp->چوڑائی();
            DfgConcat* ثابت catp = جديد DfgConcat{m_dfg, flp, Dfg_انکڑے_Type::packed(cat_چوڑائی)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ثابت uint64_t tComp = m_sccInfo.get(*termp);
            ثابت uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.جوڑو(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
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

    فراغ مکھ(Dfg_راس_Var& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        إذا (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fix_اتے_Packed(var);
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
                fix_اتے_Packed(*aselp);
                أرجع خطأ;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    Fix_اتے(Dfg_گراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

عام:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ساكن ناپ_t apply(Dfg_گراف& dfg, SccInfo& sccInfo) {
        إذا (!sccInfo.isCyclic()) أرجع 0;

        Fix_اتے fix_اتے{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
            إذا (!sccInfo.get(vtx)) استمر;
            fix_اتے.مکھ(vtx);
            إذا (!sccInfo.isCyclic()) اكسر;
        }
        // Return the number of improvements made
        أرجع fix_اتے.m_nImprovements;
    }
};

فراغ break_چکراں(Dfg_گراف& dfg, V3_Dfg_Break_چکراں_Context& ctx) {
    // Shorthand for dumping graph at given dump level
    ثابت خودکار dump = [&](صحيح level, ثابت Dfg_گراف& dfg, ثابت std::string& name) {
        إذا (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    ناپ_t nImprovements = 0;
    ناپ_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    افعل {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ثابت ناپ_t nFixed = Fix_اتے::apply(dfg, sccInfo);
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
    std::سمتیہ<Dfg_راس_Var*> varps;
    لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
        إذا (!vtx.tmpForp()) varps.ڈالو_پٹھ(&vtx);
    }
    std::stable_حصے_کرو(varps.begin(), varps.سرا(), [](ثابت Dfg_راس_Var* varp) {
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
        sccInfo.جوڑو(*prevp, 0);
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
فراغ V3DfgPasses::break_چکراں(Dfg_گراف& dfg, V3DfgContext& ctx) {
    V3_Dfg_Break_چکراں::break_چکراں(dfg, ctx.m_break_چکراں_Context);
    إذا (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::remove_بن_ورتیا(dfg);
}
