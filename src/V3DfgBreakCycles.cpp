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

نطاق V3_Dfg_Break_چڪرن {

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
    سائيز_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

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
    فراغ جوڙيو(ثابت Dfg_راس& vtx, uint64_t sccIdx) {
        ثابت منطقي new_داخلا = m_vtx2Scc.emplace(&vtx, sccIdx).ٻيو;
        UASSERT_OBJ(new_داخلا, &vtx, "Vertex already inserted");
        إذا (sccIdx) ++m_nCyclicVertices;
    }

    منطقي stillCyclicFwd(ثابت Dfg_راس& vtx) ثابت {
        // If it only reads non-cyclic vertices, it has also become acyclic
        أرجع vtx.foreach_ذريعو([&](ثابت Dfg_راس& src) {  //
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
        vtx.foreach_ذريعو([&](ثابت Dfg_راس& src) {
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
    بنية Cache_ڪنجي final {
        Dfg_راس* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_ڪنجي() = ميٽيو;
        Cache_ڪنجي(Dfg_راس* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        بنية Hash final {
            سائيز_t عامل()(ثابت Cache_ڪنجي& شي_2) ثابت {
                // cppcheck-suppress unreadVariable
                V3Hash hash{شي_2.m_vtxp};
                hash += شي_2.m_lsb;
                hash += شي_2.m_msb;
                أرجع hash.قدر();
            }
        };

        بنية Equal final {
            منطقي عامل()(ثابت Cache_ڪنجي& a, ثابت Cache_ڪنجي& b) ثابت {
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
    std::ويڪٽر<uint32_t> m_idxs;  // Indices to extract from the currently visited Vertex
    std::ويڪٽر<Dfg_راس**> m_pendingps;  // Pending pass-through cache entries across trace calls
    // Result of tracing the currently visited Vertex. Use RETURN_RESULT below!
    Dfg_راس* m_resp = خالي_اشارو;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    Dfg_راس* m_tailp = خالي_اشارو;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    Dfg_راس* m_defaultp = خالي_اشارو;
    Dfg_راس* m_splicep = خالي_اشارو;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_ڪنجي, Dfg_راس*, Cache_ڪنجي::Hash, Cache_ڪنجي::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_ڪنجي, Dfg_راس*, Cache_ڪنجي::Hash, Cache_ڪنجي::Equal> m_boundaryCache;

#جيڪڏهن_وصف_ٿيل VL_DEBUG
    std::ofstream m_سٽ_Coverage_File;  // Line coverage file, just for testing
#جيڪڏهن_پورو

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    سانچو <قسم_نالو راس_2>
    راس_2* make(ثابت Dfg_راس* refp, uint32_t ويڪر) {
        static_assert(std::is_بنياد_جو<Dfg_راس, راس_2>::قدر  //
                          && !std::is_بنياد_جو<Dfg_راس_Var, راس_2>::قدر,
                      "Should only make operation vertices and constants");

        إذا VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, راس_2>::قدر) {
            DfgConst* ثابت vtxp = جديد DfgConst{m_dfg, refp->fileline(), ويڪر, 0};
            m_sccInfo.جوڙيو(*vtxp, 0);
            أرجع reinterpret_cast<راس_2*>(vtxp);
        } وإلا {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            استعمال Vtx = قسم_نالو std::conditional<std::is_same<DfgConst, راس_2>::قدر, DfgSel,
                                                  راس_2>::type;
            Vtx* ثابت vtxp = جديد Vtx{m_dfg, refp->fileline(), Dfg_معلومات_Type::packed(ويڪر)};
            m_sccInfo.جوڙيو(*vtxp, 0);
            أرجع reinterpret_cast<راس_2*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    Dfg_راس_Var* createTmp(ثابت حرف* اڳياڙي, Dfg_راس* vtxp) {
        File_سٽ* ثابت flp = vtxp->fileline();
        Dfg_راس::ScopeCache scopeCache;
        AstScope* ثابت scopep = vtxp->scopep(scopeCache);
        Dfg_راس_Var* ثابت varp = m_dfg.makeNewVar(flp, اڳياڙي, vtxp->dtype(), scopep);
        varp->vscp()->varp()->is_اندروني(سچ);
        varp->tmpForp(varp->vscp());
        m_sccInfo.جوڙيو(*varp, 0);
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
        Dfg_راس* ثابت شروعات_Vtxp = vtxp;
        ثابت uint32_t شروعات_Msb = msb;
        ثابت uint32_t شروعات_Lsb = lsb;
        ثابت سائيز_t pendingBegin = m_pendingps.سائيز();
        Dfg_راس* resp = خالي_اشارو;

        طالما (سچ) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->سائيز() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            إذا (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                Dfg_راس*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(خالي_اشارو))
                          .پهريون->ٻيو;
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
                    إذا (msb != respr->ويڪر() - 1 || lsb != 0) {
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
                                             std::forward_as_tuple(خالي_اشارو))
                                    .پهريون->ٻيو;

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
                m_resp = خالي_اشارو;
                m_tailp = خالي_اشارو;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != خالي_اشارو) != (m_tailp != خالي_اشارو), vtxp,
                            "Expected exactly one trace result");
                إذا (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ڌڪو_پٺ(&respr);
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
        UASSERT_OBJ(resp, شروعات_Vtxp, "Tracing driver failed for " << شروعات_Vtxp->typeName());
        UASSERT_OBJ(resp->ويڪر() == (شروعات_Msb - شروعات_Lsb + 1), شروعات_Vtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        لكل (سائيز_t i = pendingBegin; i < m_pendingps.سائيز(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        أرجع resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    Dfg_راس* traceSameIdx(Dfg_راس* ثابت vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.خالي_جايون(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = خالي_اشارو;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->ويڪر() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        أرجع m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    Dfg_راس* trace_ڌڪو_Idx(Dfg_راس* ثابت vtxp, ثابت uint32_t idx) {
        ثابت سائيز_t nIdxs = m_idxs.سائيز();
        m_idxs.ڌڪو_پٺ(idx);
        Dfg_راس* ثابت resp = traceSameIdx(vtxp);
        m_idxs.ڪڍو_پٺ();
        UASSERT_OBJ(m_idxs.سائيز() == nIdxs, vtxp, "Index stack size mismatch");
        أرجع resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    Dfg_راس* trace_ڪڍو_Idx(Dfg_راس* ثابت vtxp) {
        UASSERT_OBJ(!m_idxs.خالي_جايون(), vtxp, "Should have a pending index");
        ثابت سائيز_t nIdxs = m_idxs.سائيز();
        ثابت uint32_t idx = m_idxs.پٺ();
        m_idxs.ڪڍو_پٺ();
        Dfg_راس* ثابت resp = m_idxs.خالي_جايون() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ڌڪو_پٺ(idx);
        UASSERT_OBJ(m_idxs.سائيز() == nIdxs, vtxp, "Index stack size mismatch");
        أرجع resp;
    }

    سانچو <قسم_نالو راس_2>
    راس_2* trace_Bitwise_ٻٽو(راس_2* vtxp) {
        static_assert(std::is_بنياد_جو<Dfg_راس_ٻٽو, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت resp = make<راس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        أرجع resp;
    }

    سانچو <قسم_نالو راس_2>
    Dfg_راس* trace_جوڙيو_Sub(راس_2* vtxp) {
        static_assert(std::is_بنياد_جو<Dfg_راس_ٻٽو, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت opp = make<راس_2>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        أرجع selp;
    }

    سانچو <قسم_نالو راس_2>
    راس_2* traceReduction(راس_2* vtxp) {
        static_assert(std::is_بنياد_جو<Dfg_راس_Unary, راس_2>::قدر,
                      "Should only call on DfgVertexUnary");
        راس_2* ثابت resp = make<راس_2>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->ويڪر() - 1, 0));
        أرجع resp;
    }

    سانچو <قسم_نالو راس_2>
    راس_2* traceCmp(راس_2* vtxp) {
        static_assert(std::is_بنياد_جو<Dfg_راس_ٻٽو, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت resp = make<راس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ويڪر() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ويڪر() - 1, 0));
        أرجع resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#جيڪڏهن_وصف_ٿيل VL_DEBUG
#وصف_ڪريو TRACE_نتيجو_COVER() \
    افعل { \
        إذا (VL_UNLIKELY(m_سٽ_Coverage_File.is_open())) m_سٽ_Coverage_File << __LINE__ << '\n'; \
    } طالما (خطأ)
#وإلا
#وصف_ڪريو TRACE_نتيجو_COVER() \
    افعل { \
    } طالما (خطأ)
#جيڪڏهن_پورو

#وصف_ڪريو RETURN_نتيجو(vtxp) \
    افعل { \
        m_resp = (vtxp); \
        TRACE_نتيجو_COVER(); \
        أرجع; \
    } طالما (خطأ)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#وصف_ڪريو RETURN_نتيجو_پڇ(srcp, smsb, slsb) \
    افعل { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        Dfg_راس* ثابت tailp = (srcp); \
        ثابت uint32_t پڇ_Msb = (smsb); \
        ثابت uint32_t پڇ_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = پڇ_Msb; \
        m_lsb = پڇ_Lsb; \
        TRACE_نتيجو_COVER(); \
        أرجع; \
    } طالما (خطأ)

    // VISITORS
    فراغ گهمو(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    فراغ گهمو(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        Dfg_راس* defaultp = m_defaultp;
        m_defaultp = خالي_اشارو;
        m_splicep = خالي_اشارو;

        بنية Driver final {
            Dfg_راس* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = ميٽيو;
            Driver(Dfg_راس* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::ويڪٽر<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        منطقي try_سڄو_Default = defaultp;
        Dfg_راس* coverp = خالي_اشارو;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](Dfg_راس& src, uint32_t lsb) {
            ثابت uint32_t msb = lsb + src.ويڪر() - 1;
            drivers.emplace_پٺ(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            إذا (m_msb >= lsb && msb >= m_lsb) try_سڄو_Default = خطأ;
            // If it does not cover the whole searched bit range, move on
            إذا (m_lsb < lsb || msb < m_msb) أرجع خطأ;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            أرجع سچ;
        });

        // Trace the driver covering the whole searched range, if there is one
        إذا (coverp) RETURN_نتيجو_پڇ(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        إذا (try_سڄو_Default) RETURN_نتيجو_پڇ(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::ترتيب_ڏيو(drivers.begin(), drivers.پڇاڙي(), [](ثابت Driver& a, ثابت Driver& b) {  //
            أرجع a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::ويڪٽر<Dfg_راس*> termps;
        uint32_t lsb = m_lsb;
        لكل (ثابت Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            إذا (lsb > driver.m_msb) استمر;
            // Driver is above the searched MSB, done
            إذا (driver.m_lsb > m_msb) اكسر;
            // Gap below this driver, trace default to fill it
            إذا (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_پٺ(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ثابت uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_پٺ(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        إذا (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_پٺ(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.سائيز() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        Dfg_راس* resp = termps.front();
        لكل (سائيز_t i = 1; i < termps.سائيز(); ++i) {
            Dfg_راس* ثابت termp = termps[i];
            DfgConcat* ثابت catp = make<DfgConcat>(termp, resp->ويڪر() + termp->ويڪر());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_نتيجو(resp);
    }

    فراغ گهمو(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        Dfg_راس* ثابت defaultp = m_defaultp;
        m_defaultp = خالي_اشارو;
        m_splicep = خالي_اشارو;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ثابت uint32_t idx = m_idxs.پٺ();
        إذا (Dfg_راس* ثابت driverp = vtxp->driverAt(idx)) {
            Dfg_راس* ثابت srcp = driverp->as<DfgUnitArray>()->srcp();
            إذا (srcp->is<Dfg_راس_Splice>()) {
                // Partial-element propagation is rejected during synthesis.
                UASSERT_OBJ(!defaultp, vtxp, "Array default with partial element driver");
                m_splicep = srcp;
            }
            // Consume this index, then trace the element value
            RETURN_نتيجو(trace_ڪڍو_Idx(srcp));
        }
        // An element not driven explicitly comes from the default array at the same index.
        UASSERT_OBJ(defaultp, vtxp, "Independent array element should have a driver or default");
        RETURN_نتيجو(traceSameIdx(defaultp));
    }

    فراغ گهمو(Dfg_راس_Var* vtxp) override {
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
        RETURN_نتيجو(m_idxs.خالي_جايون() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    فراغ گهمو(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        إذا (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_نتيجو(trace_ڌڪو_Idx(vtxp->fromp(), idxp->منزل_U32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.خالي_جايون(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ثابت resp = make<DfgArraySel>(vtxp, vtxp->ويڪر());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->ويڪر() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_نتيجو(selp);
    }

    فراغ گهمو(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.پٺ() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_نتيجو(trace_ڪڍو_Idx(vtxp->srcp()));
    }

    فراغ گهمو(DfgConcat* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t r_ويڪر = rhsp->ويڪر();
        // If the traced bits are wholly in the RHS
        إذا (r_ويڪر > m_msb) RETURN_نتيجو_پڇ(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        إذا (m_lsb >= r_ويڪر) RETURN_نتيجو_پڇ(lhsp, m_msb - r_ويڪر, m_lsb - r_ويڪر);
        // The traced bit spans both sides, trace both
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, r_ويڪر - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - r_ويڪر, 0));
        RETURN_نتيجو(resp);
    }

    فراغ گهمو(DfgRep* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_ويڪر = srcp->ويڪر();
        // If we need more bits than the source, then we need the whole source
        إذا (m_msb - m_lsb + 1 > s_ويڪر) {
            DfgRep* ثابت repp = make<DfgRep>(vtxp, vtxp->ويڪر());
            repp->srcp(trace(srcp, s_ويڪر - 1, 0));
            DfgSel* ثابت resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_نتيجو(resp);
        }
        // If the requested bits are within the same repliacted word
        إذا (m_msb / s_ويڪر == m_lsb / s_ويڪر) {
            RETURN_نتيجو_پڇ(srcp, m_msb % s_ويڪر, m_lsb % s_ويڪر);
        }
        // The requested bits span two replicated words
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, s_ويڪر - 1, m_lsb % s_ويڪر));
        resp->lhsp(trace(srcp, m_msb % s_ويڪر, 0));
        RETURN_نتيجو(resp);
    }

    فراغ گهمو(DfgExtend* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_ويڪر = srcp->ويڪر();
        // If the traced bits are wholly in the input
        إذا (s_ويڪر > m_msb) RETURN_نتيجو_پڇ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        إذا (m_lsb >= s_ويڪر) RETURN_نتيجو(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_ويڪر - 1, m_lsb));
        RETURN_نتيجو(resp);
    }

    فراغ گهمو(DfgExtendS* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_ويڪر = srcp->ويڪر();
        // If the traced bits are wholly in the input
        إذا (s_ويڪر > m_msb) RETURN_نتيجو_پڇ(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        إذا (m_lsb >= s_ويڪر) {
            إذا (m_msb == m_lsb) RETURN_نتيجو_پڇ(srcp, s_ويڪر - 1, s_ويڪر - 1);
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, s_ويڪر - 1, s_ويڪر - 1));
            RETURN_نتيجو(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_ويڪر - 1, m_lsb));
        RETURN_نتيجو(resp);
    }

    فراغ گهمو(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        RETURN_نتيجو_پڇ(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    فراغ گهمو(DfgNot* vtxp) override {
        DfgNot* ثابت resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_نتيجو(resp);
    }

    فراغ گهمو(DfgAnd* vtxp) override { RETURN_نتيجو(trace_Bitwise_ٻٽو(vtxp)); }
    فراغ گهمو(DfgOr* vtxp) override { RETURN_نتيجو(trace_Bitwise_ٻٽو(vtxp)); }
    فراغ گهمو(DfgXor* vtxp) override { RETURN_نتيجو(trace_Bitwise_ٻٽو(vtxp)); }
    فراغ گهمو(Dfg_جوڙيو* vtxp) override { RETURN_نتيجو(trace_جوڙيو_Sub(vtxp)); }
    فراغ گهمو(DfgSub* vtxp) override { RETURN_نتيجو(trace_جوڙيو_Sub(vtxp)); }
    فراغ گهمو(DfgRedAnd* vtxp) override { RETURN_نتيجو(traceReduction(vtxp)); }
    فراغ گهمو(DfgRedOr* vtxp) override { RETURN_نتيجو(traceReduction(vtxp)); }
    فراغ گهمو(DfgRedXor* vtxp) override { RETURN_نتيجو(traceReduction(vtxp)); }
    فراغ گهمو(DfgEq* vtxp) override { RETURN_نتيجو(traceCmp(vtxp)); }
    فراغ گهمو(DfgNeq* vtxp) override { RETURN_نتيجو(traceCmp(vtxp)); }
    فراغ گهمو(DfgLt* vtxp) override { RETURN_نتيجو(traceCmp(vtxp)); }
    فراغ گهمو(DfgLte* vtxp) override { RETURN_نتيجو(traceCmp(vtxp)); }
    فراغ گهمو(DfgGt* vtxp) override { RETURN_نتيجو(traceCmp(vtxp)); }
    فراغ گهمو(DfgGte* vtxp) override { RETURN_نتيجو(traceCmp(vtxp)); }

    فراغ گهمو(Dfg_سرڪايو_RS* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t سرڪايو_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t هيٺاهون_ويڪر = سرڪايو_Amnt > vtxp->ويڪر() ? 0 : vtxp->ويڪر() - سرڪايو_Amnt;
            // If the traced bits are wholly in the input
            إذا (هيٺاهون_ويڪر > m_msb) {
                RETURN_نتيجو_پڇ(lhsp, m_msb + سرڪايو_Amnt, m_lsb + سرڪايو_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (m_lsb >= هيٺاهون_ويڪر) {
                DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->ويڪر() - 1, lhsp->ويڪر() - 1));
                RETURN_نتيجو(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, هيٺاهون_ويڪر - 1 + سرڪايو_Amnt, m_lsb + سرڪايو_Amnt));
            RETURN_نتيجو(resp);
        }

        Dfg_سرڪايو_RS* ثابت shiftrsp = make<Dfg_سرڪايو_RS>(vtxp, vtxp->lhsp()->ويڪر() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ويڪر() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ويڪر() - 1, m_lsb));
        إذا (m_msb == vtxp->lhsp()->ويڪر() - 1) RETURN_نتيجو(shiftrsp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_نتيجو(selp);
    }

    فراغ گهمو(Dfg_سرڪايو_R* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t سرڪايو_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t هيٺاهون_ويڪر = سرڪايو_Amnt > vtxp->ويڪر() ? 0 : vtxp->ويڪر() - سرڪايو_Amnt;
            // If the traced bits are wholly in the input
            إذا (هيٺاهون_ويڪر > m_msb) {
                RETURN_نتيجو_پڇ(lhsp, m_msb + سرڪايو_Amnt, m_lsb + سرڪايو_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (m_lsb >= هيٺاهون_ويڪر) RETURN_نتيجو(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, هيٺاهون_ويڪر - 1 + سرڪايو_Amnt, m_lsb + سرڪايو_Amnt));
            RETURN_نتيجو(resp);
        }

        Dfg_سرڪايو_R* ثابت shiftrp = make<Dfg_سرڪايو_R>(vtxp, vtxp->lhsp()->ويڪر() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ويڪر() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ويڪر() - 1, m_lsb));
        إذا (m_msb == vtxp->lhsp()->ويڪر() - 1) RETURN_نتيجو(shiftrp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_نتيجو(selp);
    }

    فراغ گهمو(Dfg_سرڪايو_L* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        إذا (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t سرڪايو_Amnt = rConstp->منزل_U32();
            // Width of lower half of result
            ثابت uint32_t هيٺاهون_ويڪر = سرڪايو_Amnt > vtxp->ويڪر() ? vtxp->ويڪر() : سرڪايو_Amnt;
            // If the traced bits are wholly in the input
            إذا (m_lsb >= هيٺاهون_ويڪر) {
                RETURN_نتيجو_پڇ(lhsp, m_msb - سرڪايو_Amnt, m_lsb - سرڪايو_Amnt);
            }
            // If the traced bits are wholly in the extension
            إذا (هيٺاهون_ويڪر > m_msb) RETURN_نتيجو(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->ويڪر() - (m_msb - هيٺاهون_ويڪر + 1)));
            resp->lhsp(trace(lhsp, m_msb - سرڪايو_Amnt, هيٺاهون_ويڪر - سرڪايو_Amnt));
            RETURN_نتيجو(resp);
        }

        Dfg_سرڪايو_L* ثابت shiftlp = make<Dfg_سرڪايو_L>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->ويڪر() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        إذا (m_lsb == 0) RETURN_نتيجو(shiftlp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_نتيجو(selp);
    }

    فراغ گهمو(DfgCond* vtxp) override {
        DfgCond* ثابت resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->ويڪر() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_نتيجو(resp);
    }

    فراغ گهمو(Dfg_ميلاپ_Masked* vtxp) override {
        Dfg_ميلاپ_Masked* ثابت resp = make<Dfg_ميلاپ_Masked>(vtxp, vtxp->ويڪر());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->ويڪر() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->ويڪر() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_نتيجو(selp);
    }

#وصف_رد RETURN_نتيجو
#وصف_رد TRACE_نتيجو_COVER
#وصف_رد RETURN_نتيجو_پڇ

عام:
    // CONSTRUCTOR
    TraceDriver(Dfg_گراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#جيڪڏهن_وصف_ٿيل VL_DEBUG
        إذا (v3Global.opt.debugCheck()) {
            m_سٽ_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.اڳياڙي()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_بنياد::ٻاهريون | std::ios_بنياد::app);
        }
#جيڪڏهن_پورو
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    Dfg_راس* apply(Dfg_راس& vtx, uint32_t lsb, uint32_t ويڪر) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        أرجع trace(&vtx, lsb + ويڪر - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
صنف ٻٽو_انگ_نقاب final {
    // TYPES
    ثابت Dfg_معلومات_Type& m_dtype;
    اتحاد {
        V3_عدد m_انگ;  // The bits, if packed
        std::ويڪٽر<ٻٽو_انگ_نقاب> m_sub;  // The per element masks, if aggregate
    };

عام:
    // CONSTRUCTOR
    ٻٽو_انگ_نقاب(File_سٽ* flp, ثابت Dfg_معلومات_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        إذا (m_dtype.isPacked()) {
            جديد (&m_انگ) V3_عدد{flp, static_cast<صحيح>(m_dtype.سائيز()), 0};
        } وإلا {
            جديد (&m_sub) std::ويڪٽر<ٻٽو_انگ_نقاب>{};
            m_sub.reserve(m_dtype.سائيز());
            لكل (uint32_t i = 0; i < m_dtype.سائيز(); ++i)
                m_sub.emplace_پٺ(flp, m_dtype.elemDtype());
        }
    }
    ٻٽو_انگ_نقاب(ثابت Dfg_راس& vtx)
        : ٻٽو_انگ_نقاب{vtx.fileline(), vtx.dtype()} {}
    ٻٽو_انگ_نقاب(ثابت ٻٽو_انگ_نقاب& other)
        : m_dtype{other.m_dtype} {
        إذا (m_dtype.isPacked()) {
            جديد (&m_انگ) V3_عدد{other.m_انگ};
        } وإلا {
            جديد (&m_sub) std::ويڪٽر<ٻٽو_انگ_نقاب>{other.m_sub};
        }
    }
    ٻٽو_انگ_نقاب& عامل=(ثابت ٻٽو_انگ_نقاب& other) {
        إذا (هذا != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            إذا (other.m_dtype.isPacked()) {
                m_انگ = other.m_انگ;
            } وإلا {
                m_sub = other.m_sub;
            }
        }
        أرجع *هذا;
    }
    ~ٻٽو_انگ_نقاب() {
        إذا (m_dtype.isPacked()) {
            m_انگ.~V3_عدد();
        } وإلا {
            m_sub.~ويڪٽر<ٻٽو_انگ_نقاب>();
        }
    }

    // METHODS
    V3_عدد& انگ() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        أرجع m_انگ;
    }
    ثابت V3_عدد& انگ() ثابت {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        أرجع m_انگ;
    }
    std::ويڪٽر<ٻٽو_انگ_نقاب>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        أرجع m_sub;
    }

    منطقي isZero() ثابت {
        إذا (m_dtype.isPacked()) أرجع m_انگ.isEqZero();

        لكل (ثابت ٻٽو_انگ_نقاب& sub : m_sub) {
            إذا (!sub.isZero()) أرجع خطأ;
        }
        أرجع سچ;
    }
    منطقي isOnes() ثابت {
        إذا (m_dtype.isPacked()) أرجع m_انگ.isEqAllOnes();

        لكل (ثابت ٻٽو_انگ_نقاب& sub : m_sub) {
            إذا (!sub.isOnes()) أرجع خطأ;
        }
        أرجع سچ;
    }
    فراغ سيٽ_Zero() {
        إذا (m_dtype.isPacked()) {
            m_انگ.سيٽ_All_Bits0();
        } وإلا {
            لكل (ٻٽو_انگ_نقاب& sub : m_sub) sub.سيٽ_Zero();
        }
    }
    فراغ سيٽ_Ones() {
        إذا (m_dtype.isPacked()) {
            m_انگ.سيٽ_All_Bits1();
        } وإلا {
            لكل (ٻٽو_انگ_نقاب& sub : m_sub) sub.سيٽ_Ones();
        }
    }

    منطقي عامل==(ثابت ٻٽو_انگ_نقاب& other) ثابت {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        إذا (m_dtype.isPacked()) أرجع m_انگ.isCaseEq(other.m_انگ);
        أرجع m_sub == other.m_sub;
    }
    منطقي عامل!=(ثابت ٻٽو_انگ_نقاب& other) ثابت { أرجع !(*هذا == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    منطقي is_Contraction_جو(ثابت ٻٽو_انگ_نقاب& that) ثابت {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        إذا (m_dtype.isPacked()) {
            ثابت سائيز_t لفظ_2 = VL_لفظ_I(m_dtype.سائيز());
            لكل (سائيز_t i = 0; i < لفظ_2; ++i) {
                إذا (~m_انگ.edata_لفظ(i) & that.m_انگ.edata_لفظ(i)) أرجع سچ;
            }
            أرجع خطأ;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        لكل (سائيز_t i = 0; i < m_sub.سائيز(); ++i) {
            إذا (m_sub[i].is_Contraction_جو(that.m_sub[i])) أرجع سچ;
        }
        أرجع خطأ;
    }

    std::string منزل_String() ثابت {
        إذا (m_dtype.isPacked()) أرجع "0x" + m_انگ.displayed(m_انگ.fileline(), "%x");
        std::string نتيجو;
        نتيجو += "{";
        لكل (ثابت ٻٽو_انگ_نقاب& sub : m_sub) {
            إذا (&sub != &m_sub.front()) نتيجو += ", ";
            نتيجو += sub.منزل_String();
        }
        نتيجو += "}";
        أرجع نتيجو;
    }
};

صنف Independent_بٽ final : عام DfgVisitor {
    // TYPES
    بنية راس_State final {
        سائيز_t m_rpo_عدد = 0;  // Reverse Postorder Number of vertex
        منطقي m_isOnWorkList = خطأ;  // Whether the vertex is on the work list
    };

    // STATE
    ثابت SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ثابت Dfg_راس*, ٻٽو_انگ_نقاب> m_vtxp2_نقاب;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::ترجيح_قطار<سائيز_t, std::ويڪٽر<سائيز_t>, std::greater<سائيز_t>> m_work_قطار;
    std::unordered_map<ثابت Dfg_راس*, راس_State> m_vtxp2State;  // State of each vertex

#جيڪڏهن_وصف_ٿيل VL_DEBUG
    std::ofstream m_سٽ_Coverage_File;  // Line coverage file, just for testing
#جيڪڏهن_پورو

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    ٻٽو_انگ_نقاب& نقاب(ثابت Dfg_راس& vtx) {
        // Look up (or create) mask for 'vtx'
        أرجع m_vtxp2_نقاب
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .پهريون->ٻيو;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#جيڪڏهن_وصف_ٿيل VL_DEBUG
#وصف_ڪريو نقاب_2(vtxp) \
    ([هذا](ثابت Dfg_راس& vtx) -> ٻٽو_انگ_نقاب& { \
        إذا (VL_UNLIKELY(m_سٽ_Coverage_File.is_open())) m_سٽ_Coverage_File << __LINE__ << '\n'; \
        أرجع نقاب(vtx); \
    }(*vtxp))
#وإلا
#وصف_ڪريو نقاب_2(vtxp) نقاب(*vtxp)
#جيڪڏهن_پورو

    // Clear all bits at or below the most significant clear bit
    فراغ floodTowardsLsb(V3_عدد& انگ) {
        لكل (صحيح i = انگ.ويڪر() - 1; i >= 0; --i) {
            إذا (انگ.ٻٽو_انگ_Is1(i)) استمر;
            انگ.op_سيٽ_Range(0, i + 1, '0');
            اكسر;
        }
    }

    // Clear all bits at or above the least significant clear bit
    فراغ floodTowardsMsb(V3_عدد& انگ) {
        لكل (صحيح i = 0; i < انگ.ويڪر(); ++i) {
            إذا (انگ.ٻٽو_انگ_Is1(i)) استمر;
            انگ.op_سيٽ_Range(i, انگ.ويڪر() - i, '0');
            اكسر;
        }
    }

    فراغ propagateFromDriver(ٻٽو_انگ_نقاب& m, ثابت Dfg_راس* srcp) {
        // If there is no driver, we are done
        إذا (!srcp) أرجع;
        // If it is driven by a splice, we need to combine the masks of the drivers
        إذا (ثابت DfgSplicePacked* ثابت splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
                m.انگ().opSelInto(نقاب_2(&src).انگ(), lo, src.ويڪر());
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
    فراغ گهمو(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    فراغ گهمو(Dfg_راس_Var* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        إذا (vtxp->isVolatile()) أرجع;

        ٻٽو_انگ_نقاب& m = نقاب_2(vtxp);
        Dfg_راس* ثابت srcp = vtxp->srcp();
        Dfg_راس* ثابت defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        إذا (defaultp) m = نقاب_2(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    فراغ گهمو(Dfg_راس_Splice* vtxp) override {
        propagateFromDriver(نقاب_2(vtxp), vtxp);  // Needed to continue traversal
    }

    فراغ گهمو(DfgUnitArray* vtxp) override { نقاب_2(vtxp).sub().at(0) = نقاب_2(vtxp->srcp()); }

    فراغ گهمو(DfgArraySel* vtxp) override {
        Dfg_راس* ثابت fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        إذا (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            نقاب_2(vtxp) = نقاب_2(vtxp->fromp()).sub().at(idxp->منزل_سائيز_T());
            أرجع;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        إذا (نقاب_2(vtxp->bitp()).isOnes() && نقاب_2(fromp).isOnes()) نقاب_2(vtxp).سيٽ_Ones();
    }

    فراغ گهمو(DfgConcat* vtxp) override {
        ثابت Dfg_راس* ثابت rhsp = vtxp->rhsp();
        ثابت Dfg_راس* ثابت lhsp = vtxp->lhsp();
        V3_عدد& m = نقاب_2(vtxp).انگ();
        m.opSelInto(نقاب_2(rhsp).انگ(), 0, rhsp->ويڪر());
        m.opSelInto(نقاب_2(lhsp).انگ(), rhsp->ويڪر(), lhsp->ويڪر());
    }

    فراغ گهمو(DfgRep* vtxp) override {
        ثابت uint32_t ڳڻپ = vtxp->ڳڻپ();
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_ويڪر = srcp->ويڪر();
        V3_عدد& v_نقاب = نقاب_2(vtxp).انگ();
        V3_عدد& s_نقاب = نقاب_2(srcp).انگ();
        لكل (uint32_t i = 0; i < ڳڻپ; ++i) v_نقاب.opSelInto(s_نقاب, i * s_ويڪر, s_ويڪر);
    }

    فراغ گهمو(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t msb = lsb + vtxp->ويڪر() - 1;
        نقاب_2(vtxp).انگ().opSel(نقاب_2(vtxp->fromp()).انگ(), msb, lsb);
    }

    فراغ گهمو(DfgExtend* vtxp) override {
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_ويڪر = srcp->ويڪر();
        V3_عدد& s = نقاب_2(srcp).انگ();
        V3_عدد& m = نقاب_2(vtxp).انگ();
        m.opSelInto(s, 0, s_ويڪر);
        m.op_سيٽ_Range(s_ويڪر, vtxp->ويڪر() - s_ويڪر, '1');
    }
    فراغ گهمو(DfgExtendS* vtxp) override {
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_ويڪر = srcp->ويڪر();
        V3_عدد& s = نقاب_2(srcp).انگ();
        V3_عدد& m = نقاب_2(vtxp).انگ();
        m.opSelInto(s, 0, s_ويڪر);
        m.op_سيٽ_Range(s_ويڪر, vtxp->ويڪر() - s_ويڪر, s.ٻٽو_انگ_Is0(s_ويڪر - 1) ? '0' : '1');
    }

    فراغ گهمو(DfgNot* vtxp) override {  //
        نقاب_2(vtxp) = نقاب_2(vtxp->srcp());
    }

    فراغ گهمو(DfgAnd* vtxp) override {  //
        نقاب_2(vtxp).انگ().opAnd(نقاب_2(vtxp->lhsp()).انگ(), نقاب_2(vtxp->rhsp()).انگ());
    }
    فراغ گهمو(DfgOr* vtxp) override {  //
        نقاب_2(vtxp).انگ().opAnd(نقاب_2(vtxp->lhsp()).انگ(), نقاب_2(vtxp->rhsp()).انگ());
    }
    فراغ گهمو(DfgXor* vtxp) override {  //
        نقاب_2(vtxp).انگ().opAnd(نقاب_2(vtxp->lhsp()).انگ(), نقاب_2(vtxp->rhsp()).انگ());
    }

    فراغ گهمو(Dfg_جوڙيو* vtxp) override {
        V3_عدد& m = نقاب_2(vtxp).انگ();
        m.opAnd(نقاب_2(vtxp->lhsp()).انگ(), نقاب_2(vtxp->rhsp()).انگ());
        floodTowardsMsb(m);
    }
    فراغ گهمو(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_عدد& m = نقاب_2(vtxp).انگ();
        m.opAnd(نقاب_2(vtxp->lhsp()).انگ(), نقاب_2(vtxp->rhsp()).انگ());
        floodTowardsMsb(m);
    }

    فراغ گهمو(DfgRedAnd* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }
    فراغ گهمو(DfgRedOr* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }
    فراغ گهمو(DfgRedXor* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }

    فراغ گهمو(DfgEq* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }
    فراغ گهمو(DfgNeq* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }
    فراغ گهمو(DfgLt* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }
    فراغ گهمو(DfgLte* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }
    فراغ گهمو(DfgGt* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }
    فراغ گهمو(DfgGte* vtxp) override {
        ثابت منطقي independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        إذا (independent) نقاب_2(vtxp).سيٽ_Ones();
    }

    فراغ گهمو(Dfg_سرڪايو_RS* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t ويڪر = vtxp->ويڪر();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرڪايو_Amount = rConstp->منزل_U32();
            إذا (سرڪايو_Amount >= ويڪر) {
                إذا (نقاب_2(lhsp).انگ().ٻٽو_انگ_Is1(ويڪر - 1)) { نقاب_2(vtxp).سيٽ_Ones(); }
            } وإلا {
                V3_عدد& m = نقاب_2(vtxp).انگ();
                m.op_سرڪايو_RS(نقاب_2(lhsp).انگ(), rConstp->انگ(), ويڪر);
                m.op_سيٽ_Range(ويڪر - سرڪايو_Amount, سرڪايو_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (نقاب_2(rhsp).isOnes()) {
            V3_عدد& m = نقاب_2(vtxp).انگ();
            m = نقاب_2(lhsp).انگ();
            floodTowardsLsb(m);
        }
    }

    فراغ گهمو(Dfg_سرڪايو_R* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t ويڪر = vtxp->ويڪر();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرڪايو_Amount = rConstp->منزل_U32();
            إذا (سرڪايو_Amount >= ويڪر) {
                نقاب_2(vtxp).سيٽ_Ones();
            } وإلا {
                V3_عدد& m = نقاب_2(vtxp).انگ();
                m.op_سرڪايو_R(نقاب_2(lhsp).انگ(), rConstp->انگ());
                m.op_سيٽ_Range(ويڪر - سرڪايو_Amount, سرڪايو_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (نقاب_2(rhsp).isOnes()) {
            V3_عدد& m = نقاب_2(vtxp).انگ();
            m = نقاب_2(lhsp).انگ();
            floodTowardsLsb(m);
        }
    }

    فراغ گهمو(Dfg_سرڪايو_L* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t ويڪر = vtxp->ويڪر();

        // Constant shift can be computed precisely
        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرڪايو_Amount = rConstp->منزل_U32();
            إذا (سرڪايو_Amount >= ويڪر) {
                نقاب_2(vtxp).سيٽ_Ones();
            } وإلا {
                V3_عدد& m = نقاب_2(vtxp).انگ();
                m.op_سرڪايو_L(نقاب_2(lhsp).انگ(), rConstp->انگ());
                m.op_سيٽ_Range(0, سرڪايو_Amount, '1');
            }
            أرجع;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        إذا (نقاب_2(rhsp).isOnes()) {
            V3_عدد& m = نقاب_2(vtxp).انگ();
            m = نقاب_2(lhsp).انگ();
            floodTowardsMsb(m);
        }
    }

    فراغ گهمو(DfgCond* vtxp) override {
        إذا (نقاب_2(vtxp->condp()).isOnes()) {
            نقاب_2(vtxp).انگ().opAnd(نقاب_2(vtxp->thenp()).انگ(), نقاب_2(vtxp->elsep()).انگ());
        }
    }

    فراغ گهمو(Dfg_ميلاپ_Masked* vtxp) override {
        إذا (نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->matchp()).isOnes()) { نقاب_2(vtxp).سيٽ_Ones(); }
    }

#وصف_رد نقاب_2

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    فراغ قطار_اندر_وجهو_Sinks(Dfg_راس& vtx) {
        vtx.foreachSink([&](Dfg_راس& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            إذا (!m_sccInfo.get(sink)) أرجع خطأ;
            // Otherwise just enqueue it
            راس_State& state = m_vtxp2State.at(&sink);
            إذا (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_work_قطار.ڌڪو(state.m_rpo_عدد);
            }
            state.m_isOnWorkList = سچ;
            أرجع خطأ;
        });
    };

    فراغ dfs_Post_ترتيب(std::unordered_سيٽ<Dfg_راس*>& گهميل,  //
                      std::ويڪٽر<Dfg_راس*>& post_ترتيب_Enumeration,  //
                      Dfg_راس& vtx) {
        // Mark visited, skip if already visited
        إذا (!گهميل.وجهو(&vtx).ٻيو) أرجع;
        // Visit un-visited successors
        vtx.foreachSink([&](Dfg_راس& snk) {
            dfs_Post_ترتيب(گهميل, post_ترتيب_Enumeration, snk);
            أرجع خطأ;
        });
        // Add to post order enumeration
        post_ترتيب_Enumeration.emplace_پٺ(&vtx);
    };

    Independent_بٽ(Dfg_گراف& dfg, ثابت SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#جيڪڏهن_وصف_ٿيل VL_DEBUG
        إذا (v3Global.opt.debugCheck()) {
            m_سٽ_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.اڳياڙي()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_بنياد::ٻاهريون | std::ios_بنياد::app);
        }
#جيڪڏهن_پورو

        // Compute Reverse-Postorder enumeration of vertices
        std::ويڪٽر<Dfg_راس*> rpoEnumeration;
        std::unordered_سيٽ<Dfg_راس*> گهميل;
        dfg.for_Each_راس([&](Dfg_راس& vtx) {
            إذا (!vtx.nInputs()) dfs_Post_ترتيب(گهميل, rpoEnumeration, vtx);
        });
        dfg.for_Each_راس([&](Dfg_راس& vtx) {  //
            dfs_Post_ترتيب(گهميل, rpoEnumeration, vtx);
        });
        std::ابتو(rpoEnumeration.begin(), rpoEnumeration.پڇاڙي());
        UASSERT(rpoEnumeration.سائيز() == dfg.سائيز(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        لكل (سائيز_t i = 0; i < rpoEnumeration.سائيز(); ++i) {
            راس_State& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_عدد = i;
            state.m_isOnWorkList = خطأ;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        لكل (Dfg_راس* ثابت vtxp : rpoEnumeration) {
            إذا (m_sccInfo.get(*vtxp)) استمر;
            نقاب(*vtxp).سيٽ_Ones();
        }
        لكل (Dfg_راس* ثابت vtxp : rpoEnumeration) {
            إذا (!m_sccInfo.get(*vtxp)) استمر;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << نقاب(*vtxp).منزل_String());
            إذا (!نقاب(*vtxp).isZero()) قطار_اندر_وجهو_Sinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        طالما (!m_work_قطار.خالي_جايون()) {
            // Grab next item
            Dfg_راس* ثابت currp = rpoEnumeration[m_work_قطار.چوٽي()];
            m_work_قطار.ڪڍو();
            m_vtxp2State.at(currp).m_isOnWorkList = خطأ;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ثابت ٻٽو_انگ_نقاب& curr_نقاب = نقاب(*currp);
            // Remember current mask so we can check if it changed
            ثابت ٻٽو_انگ_نقاب prev_نقاب = curr_نقاب;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            إذا (prev_نقاب != curr_نقاب) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << prev_نقاب.منزل_String()
                             << "\n      to: " << curr_نقاب.منزل_String());
                قطار_اندر_وجهو_Sinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!curr_نقاب.is_Contraction_جو(prev_نقاب), currp,
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
    ساكن std::unordered_map<ثابت Dfg_راس*, ٻٽو_انگ_نقاب> apply(Dfg_گراف& dfg,
                                                               ثابت SccInfo& sccInfo) {
        أرجع std::move(Independent_بٽ{dfg, sccInfo}.m_vtxp2_نقاب);
    }
};

صنف Fix_مٿي final {
    Dfg_گراف& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ثابت std::unordered_map<ثابت Dfg_راس*, ٻٽو_انگ_نقاب> m_independent_بٽ
        = Independent_بٽ::apply(m_dfg, m_sccInfo);
    سائيز_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ساكن ٻٽو_انگ_نقاب compute_استعمال_ٿيل_بٽ(Dfg_راس& vtx) {
        ٻٽو_انگ_نقاب نتيجو{vtx};
        vtx.foreachSink([&نتيجو](Dfg_راس& sink) {
            // If used via a Sel, mark the selected bits used
            إذا (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                نتيجو.انگ().op_سيٽ_Range(selp->lsb(), selp->ويڪر(), '1');
                أرجع خطأ;
            }
            // Used without a Sel, so all bits are used
            نتيجو.سيٽ_Ones();
            أرجع سچ;
        });
        أرجع نتيجو;
    }

    ساكن std::string debugStr(ثابت Dfg_راس& vtx) {  // LCOV_EXCL_START
        إذا (ثابت DfgArraySel* ثابت aselp = vtx.cast<DfgArraySel>()) {
            ثابت سائيز_t i = aselp->bitp()->as<DfgConst>()->منزل_سائيز_T();
            أرجع debugStr(*aselp->fromp()) + "[" + std::منزل_string(i) + "]";
        }
        إذا (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
            أرجع varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    فراغ gatherTerms(std::ويڪٽر<Dfg_راس*>& termps, Dfg_راس& vtx, ثابت V3_عدد& indp_بٽ,
                     ثابت uint32_t hi, ثابت uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        منطقي isIndependent = indp_بٽ.ٻٽو_انگ_Is1(lo);
        uint32_t lsb = lo;
        لكل (uint32_t msb = lo; msb <= hi; ++msb) {
            ثابت منطقي پڇاڙي_Range = msb == hi || isIndependent != indp_بٽ.ٻٽو_انگ_Is1(msb + 1);
            إذا (!پڇاڙي_Range) استمر;
            ثابت uint32_t ويڪر = msb - lsb + 1;
            // Compute the term to use for this range
            إذا (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_پٺ(m_traceDriver.apply(vtx, lsb, ويڪر));
            } وإلا {
                // If dependent, fall back on using the part of the variable
                DfgSel* ثابت selp = جديد DfgSel{m_dfg, vtx.fileline(), Dfg_معلومات_Type::packed(ويڪر)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.جوڙيو(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_پٺ(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    فراغ fix_مٿي_Packed(Dfg_راس& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ثابت ٻٽو_انگ_نقاب& indp_بٽ = m_independent_بٽ.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indp_بٽ.منزل_String());
        // Can't do anything if all bits are dependent
        إذا (indp_بٽ.isZero()) أرجع;

        // Figure out which bits of 'vtxp' are used
        ثابت ٻٽو_انگ_نقاب استعمال_ٿيل_بٽ = compute_استعمال_ٿيل_بٽ(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << استعمال_ٿيل_بٽ.منزل_String());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_عدد استعمال_ٿيل_And_Independent{vtx.fileline(), static_cast<صحيح>(vtx.ويڪر()), 0};
        استعمال_ٿيل_And_Independent.opAnd(استعمال_ٿيل_بٽ.انگ(), indp_بٽ.انگ());
        إذا (استعمال_ٿيل_And_Independent.isEqZero()) أرجع;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::ويڪٽر<Dfg_راس*> termps;

        // Iterate through consecutive used/unused ranges
        File_سٽ* ثابت flp = vtx.fileline();
        ثابت uint32_t ويڪر = vtx.ويڪر();
        منطقي is_استعمال_ٿيل = استعمال_ٿيل_بٽ.انگ().ٻٽو_انگ_Is1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        لكل (uint32_t msb = 0; msb < ويڪر; ++msb) {
            ثابت منطقي پڇاڙي_Range = msb == ويڪر - 1 || is_استعمال_ٿيل != استعمال_ٿيل_بٽ.انگ().ٻٽو_انگ_Is1(msb + 1);
            إذا (!پڇاڙي_Range) استمر;
            إذا (is_استعمال_ٿيل) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indp_بٽ.انگ(), msb, lsb);
            } وإلا {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ثابت constp = جديد DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.جوڙيو(*constp, 0);
                termps.emplace_پٺ(constp);
            }
            // Next iteration
            is_استعمال_ٿيل = !is_استعمال_ٿيل;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        Dfg_راس* replacementp = termps.front();
        ثابت uint64_t vComp = m_sccInfo.get(vtx);
        لكل (سائيز_t i = 1; i < termps.سائيز(); ++i) {
            Dfg_راس* ثابت termp = termps[i];
            ثابت uint32_t cat_ويڪر = replacementp->ويڪر() + termp->ويڪر();
            DfgConcat* ثابت catp = جديد DfgConcat{m_dfg, flp, Dfg_معلومات_Type::packed(cat_ويڪر)};
            catp->rhsp(replacementp);
            catp->lhsp(termp);
            // Need to figure out which component the replacement vertex
            // belongs to. If any of the terms are of the original
            // component, it's part of that component, otherwise its not
            // cyclic (all terms are from outside the original component,
            // and feed into the original component).
            ثابت uint64_t tComp = m_sccInfo.get(*termp);
            ثابت uint64_t rComp = m_sccInfo.get(*replacementp);
            m_sccInfo.جوڙيو(*catp, tComp == vComp || rComp == vComp ? vComp : 0);
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

    فراغ مکيه(Dfg_راس_Var& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        إذا (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fix_مٿي_Packed(var);
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
                fix_مٿي_Packed(*aselp);
                أرجع خطأ;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    Fix_مٿي(Dfg_گراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

عام:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ساكن سائيز_t apply(Dfg_گراف& dfg, SccInfo& sccInfo) {
        إذا (!sccInfo.isCyclic()) أرجع 0;

        Fix_مٿي fix_مٿي{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
            إذا (!sccInfo.get(vtx)) استمر;
            fix_مٿي.مکيه(vtx);
            إذا (!sccInfo.isCyclic()) اكسر;
        }
        // Return the number of improvements made
        أرجع fix_مٿي.m_nImprovements;
    }
};

فراغ break_چڪرن(Dfg_گراف& dfg, V3_Dfg_Break_چڪرن_Context& ctx) {
    // Shorthand for dumping graph at given dump level
    ثابت خودڪار dump = [&](صحيح level, ثابت Dfg_گراف& dfg, ثابت std::string& name) {
        إذا (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    سائيز_t nImprovements = 0;
    سائيز_t prevNImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    افعل {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        prevNImprovements = nImprovements;
        ثابت سائيز_t nFixed = Fix_مٿي::apply(dfg, sccInfo);
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
    std::ويڪٽر<Dfg_راس_Var*> varps;
    لكل (Dfg_راس_Var& vtx : dfg.varVertices()) {
        إذا (!vtx.tmpForp()) varps.ڌڪو_پٺ(&vtx);
    }
    std::stable_حصا_ڪريو(varps.begin(), varps.پڇاڙي(), [](ثابت Dfg_راس_Var* varp) {
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
        sccInfo.جوڙيو(*prevp, 0);
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
فراغ V3DfgPasses::break_چڪرن(Dfg_گراف& dfg, V3DfgContext& ctx) {
    V3_Dfg_Break_چڪرن::break_چڪرن(dfg, ctx.m_break_چڪرن_Context);
    إذا (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::remove_اڻ_استعمال_ٿيل(dfg);
}
