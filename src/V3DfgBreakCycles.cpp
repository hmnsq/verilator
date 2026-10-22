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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3Error.h"
#شامل "V3Hash.h"

#شامل <algorithm>
#شامل <fstream>
#شامل <queue>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

نامخلا V3_Dfg_Break_ادوار {

// Mutable map from vertices to the SCC index they belong to. Initialized
// from a proper analysis, then updated as we break cycles. After updates
// the information is consertvative, meaning a vertex that is marked as
// cyclic might not actually be if an SCC split into multple SCCs after a
// fixup. However it should always be true that if a vertex is thought to
// be non-cyclic based on this datastructure, then it indeed is not cyclic.
جماعت SccInfo final {
    Dfg_گراف& m_dfg;  // The annotated graph
    // The map from vertices to their SCC index (0 if trivial SCC - non cyclic)
    std::unordered_map<ثابت Dfg_راس*, uint64_t> m_vtx2Scc;
    سائز_t m_nCyclicVertices = 0;  // Number of vertices in non-trivial SCCs

عوامی:
    SccInfo(Dfg_گراف& dfg)
        : m_dfg{dfg} {
        // Initialize m_vtx2Scc for the graph from a proper analysis
        DfgUserMap<uint64_t> vtx2Scc = dfg.makeUserMap<uint64_t>();
        V3DfgPasses::colorStronglyConnectedComponents(dfg, vtx2Scc);
        m_dfg.for_Each_راس([&](ثابت Dfg_راس& vtx) {
            ثابت uint64_t sccIdx = vtx2Scc[vtx];
            m_vtx2Scc[&vtx] = sccIdx;
            اگر (sccIdx) ++m_nCyclicVertices;
        });
    }

    // Is the graph cyclic still?
    منطقی isCyclic() ثابت { واپس m_nCyclicVertices; }

    // Returns the index of the SCC the given vertex is a part of
    // This is 0 iff the vertex is in a trivial SCC (no cycles through vertex)
    uint64_t get(ثابت Dfg_راس& vtx) ثابت { واپس m_vtx2Scc.at(&vtx); }

    // Add a new vertex to graph that is part of the given SCC
    خالی جوڑو(ثابت Dfg_راس& vtx, uint64_t sccIdx) {
        ثابت منطقی new_اندراج = m_vtx2Scc.emplace(&vtx, sccIdx).دوسرا;
        UASSERT_OBJ(new_اندراج, &vtx, "Vertex already inserted");
        اگر (sccIdx) ++m_nCyclicVertices;
    }

    منطقی stillCyclicFwd(ثابت Dfg_راس& vtx) ثابت {
        // If it only reads non-cyclic vertices, it has also become acyclic
        واپس vtx.foreach_ماخذ([&](ثابت Dfg_راس& src) {  //
            واپس m_vtx2Scc.at(&src);
        });
    }

    منطقی stillCyclicBwd(ثابت Dfg_راس& vtx) ثابت {
        // If itonly drives non-cyclic vertices, it has also become acyclic
        واپس vtx.foreachSink([&](ثابت Dfg_راس& dst) {  //
            واپس m_vtx2Scc.at(&dst);
        });
    }

    // The given vertex is known to have become acyclic after some edits,
    // propagate through graph to mark connected vertices as well.
    خالی updateAcyclic(ثابت Dfg_راس& vtx) {
        // Mark as acyclic
        uint64_t& sccIdxr = m_vtx2Scc.at(&vtx);
        UASSERT_OBJ(sccIdxr, &vtx, "Vertex should be cyclic");
        sccIdxr = 0;
        --m_nCyclicVertices;
        // Propagate the update through the graph forward
        vtx.foreachSink([&](ثابت Dfg_راس& dst) {
            // Sink was known to be acyclic, stop
            اگر (!m_vtx2Scc.at(&dst)) واپس جھوٹ;
            اگر (!stillCyclicFwd(dst)) updateAcyclic(dst);
            واپس جھوٹ;
        });
        // Propagate the update through the graph backward
        vtx.foreach_ماخذ([&](ثابت Dfg_راس& src) {
            // Source was known to be acyclic, stop
            اگر (!m_vtx2Scc.at(&src)) واپس جھوٹ;
            اگر (!stillCyclicBwd(src)) updateAcyclic(src);
            واپس جھوٹ;
        });
    }

    // Update after replacing 'vtxp' with 'replacement'
    خالی updateReplacement(ثابت Dfg_راس& vtx, Dfg_راس& replacement) {
        UASSERT_OBJ(!vtx.hasSinks(), &vtx, "Replaced vertex should have no sinks");
        updateAcyclic(vtx);
        اگر (!get(replacement)) {
            replacement.foreachSink([&](Dfg_راس& dst) {
                اگر (!get(dst)) واپس جھوٹ;
                اگر (!stillCyclicFwd(dst)) updateAcyclic(dst);
                واپس جھوٹ;
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
    خالی validate() ثابت {
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

جماعت TraceDriver final : عوامی DfgVisitor {
    // TYPES
    // Key for caching the result of a trace
    ساخت Cache_کلید final {
        Dfg_راس* m_vtxp;  // Vertex being traced
        uint32_t m_lsb;  // LSB of the range within m_vtxp being traced
        uint32_t m_msb;  // MSB of the range within m_vtxp being traced

        Cache_کلید() = حذف;
        Cache_کلید(Dfg_راس* vtxp, uint32_t lsb, uint32_t msb)
            : m_vtxp{vtxp}
            , m_lsb{lsb}
            , m_msb{msb} {}

        ساخت Hash final {
            سائز_t عامل()(ثابت Cache_کلید& شے_3) ثابت {
                // cppcheck-suppress unreadVariable
                V3Hash hash{شے_3.m_vtxp};
                hash += شے_3.m_lsb;
                hash += شے_3.m_msb;
                واپس hash.قدر();
            }
        };

        ساخت Equal final {
            منطقی عامل()(ثابت Cache_کلید& a, ثابت Cache_کلید& b) ثابت {
                واپس a.m_vtxp == b.m_vtxp && a.m_lsb == b.m_lsb && a.m_msb == b.m_msb;
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
    Dfg_راس* m_resp = خالی_اشاریہ;
    // Set instead of 'm_resp' when the result of tracing the currently visited Vertex is
    // exactly the result of tracing 'm_tailp' at the updated [m_msb:m_lsb]. 'trace'
    // resolves these iteratively. Use RETURN_RESULT_TAIL below!
    Dfg_راس* m_tailp = خالی_اشاریہ;
    // When tracing a splice, this is the corresponding 'defaultp' of the variable it drives
    Dfg_راس* m_defaultp = خالی_اشاریہ;
    Dfg_راس* m_splicep = خالی_اشاریہ;  // The splice vertex being traced, just for assertions
    // Result cache for reusing already traced vertices
    std::unordered_map<Cache_کلید, Dfg_راس*, Cache_کلید::Hash, Cache_کلید::Equal> m_cache;
    // Cache for boundary vertices (where m_sccInfo.get(*vtxp) != m_component)
    std::unordered_map<Cache_کلید, Dfg_راس*, Cache_کلید::Hash, Cache_کلید::Equal> m_boundaryCache;

#اگر_تعریف_شدہ VL_DEBUG
    std::ofstream m_سطر_Coverage_File;  // Line coverage file, just for testing
#اگر_ختم

    // METHODS

    // Create and return a new Vertex. Always use this to create new vertices.
    // Fileline is taken from 'refp', but 'refp' is otherwise not used.
    // Also adds the vertex to m_sccInfo with scc index 0, indicating the new
    // vertex is not part of a strongly connected component. This should always
    // be true, as all the vertices we create here are driven from outside the
    // component we are trying to escape, and will sink into that component.
    // Given those are separate SCCs, these new vertices must be acyclic.
    سانچہ <قسم_نام راس_2>
    راس_2* make(ثابت Dfg_راس* refp, uint32_t چوڑائی) {
        static_assert(std::is_بنیاد_کا<Dfg_راس, راس_2>::قدر  //
                          && !std::is_بنیاد_کا<Dfg_راس_Var, راس_2>::قدر,
                      "Should only make operation vertices and constants");

        اگر VL_CONSTEXPR_CXX17 (std::is_same<DfgConst, راس_2>::قدر) {
            DfgConst* ثابت vtxp = نیا DfgConst{m_dfg, refp->fileline(), چوڑائی, 0};
            m_sccInfo.جوڑو(*vtxp, 0);
            واپس reinterpret_cast<راس_2*>(vtxp);
        } ورنہ {
            // TODO: this is a gross hack around lack of C++17 'if constexpr' Vtx is always Vertex
            // when this code is actually executed, but needs a fudged type to type check when
            // Vertex is DfgConst, in which case this code is unreachable ...
            استعمال Vtx = قسم_نام std::conditional<std::is_same<DfgConst, راس_2>::قدر, DfgSel,
                                                  راس_2>::type;
            Vtx* ثابت vtxp = نیا Vtx{m_dfg, refp->fileline(), Dfg_ڈیٹا_Type::packed(چوڑائی)};
            m_sccInfo.جوڑو(*vtxp, 0);
            واپس reinterpret_cast<راس_2*>(vtxp);
        }
    }

    // Create temporary capable of holding the result of 'vtxp'
    Dfg_راس_Var* createTmp(ثابت حرف* سابقہ, Dfg_راس* vtxp) {
        File_سطر* ثابت flp = vtxp->fileline();
        Dfg_راس::ScopeCache scopeCache;
        AstScope* ثابت scopep = vtxp->scopep(scopeCache);
        Dfg_راس_Var* ثابت varp = m_dfg.makeNewVar(flp, سابقہ, vtxp->dtype(), scopep);
        varp->vscp()->varp()->is_اندرونی(سچ);
        varp->tmpForp(varp->vscp());
        m_sccInfo.جوڑو(*varp, 0);
        واپس varp;
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
        Dfg_راس* ثابت آغاز_Vtxp = vtxp;
        ثابت uint32_t آغاز_Msb = msb;
        ثابت uint32_t آغاز_Lsb = lsb;
        ثابت سائز_t pendingBegin = m_pendingps.سائز();
        Dfg_راس* resp = خالی_اشاریہ;

        جب (سچ) {
            UASSERT_OBJ(vtxp->isPacked(), vtxp, "Can only trace packed type vertices");
            UASSERT_OBJ(vtxp->سائز() > msb, vtxp, "Traced Vertex too narrow");
            UASSERT_OBJ(!m_defaultp || vtxp == m_splicep, vtxp, "Tracing wrong vertex");

            // If the currently traced vertex is in a different component,
            // then we found what we were looking for. But if it's a splice
            // with a corresponding default, we need to keep going as the
            // splice does not fully define the value we are seeking.
            اگر (m_sccInfo.get(*vtxp) != m_component && !m_defaultp) {
                Dfg_راس*& respr
                    = m_boundaryCache
                          .emplace(std::piecewise_construct, std::forward_as_tuple(vtxp, msb, lsb),
                                   std::forward_as_tuple(خالی_اشاریہ))
                          .پہلا->دوسرا;
                اگر (!respr) {
                    respr = vtxp;
                    // If the result is a splice, we need to insert a temporary for it
                    // as a splice cannot be fed into arbitray logic
                    اگر (Dfg_راس_Splice* ثابت splicep = respr->cast<Dfg_راس_Splice>()) {
                        Dfg_راس_Var* ثابت tmpp = createTmp("TraceDriver", splicep);
                        // Note: we can't do 'splicep->replaceWith(tmpp)', as other
                        // variable sinks of the splice might have a defaultp driver.
                        tmpp->srcp(splicep);
                        respr = tmpp;
                    }
                    // Apply a Sel to extract the relevant bits if only a part is needed
                    اگر (msb != respr->چوڑائی() - 1 || lsb != 0) {
                        DfgSel* ثابت selp = make<DfgSel>(respr, msb - lsb + 1);
                        selp->fromp(respr);
                        selp->lsb(lsb);
                        respr = selp;
                    }
                }
                resp = respr;
                توڑو;
            }

            // Normal cache for intra-component traces
            Dfg_راس*& respr = m_cache
                                    .emplace(std::piecewise_construct,  //
                                             std::forward_as_tuple(vtxp, msb, lsb),  //
                                             std::forward_as_tuple(خالی_اشاریہ))
                                    .پہلا->دوسرا;

            // Trace the vertex
            اگر (respr) {
                // If already traced this vtxp/msb/lsb, just use the result.
                // This is important to avoid combinatorial explosion when the
                // same sub-expression is needed multiple times.
            } ورنہ {
                // Otherwise visit the vertex to trace it
                VL_RESTORER(m_msb);
                VL_RESTORER(m_lsb);
                VL_RESTORER_CLEAR(m_idxs);
                VL_RESTORER(m_resp);
                VL_RESTORER(m_tailp);
                m_msb = msb;
                m_lsb = lsb;
                m_resp = خالی_اشاریہ;
                m_tailp = خالی_اشاریہ;
                iterate(vtxp);
                UASSERT_OBJ((m_resp != خالی_اشاریہ) != (m_tailp != خالی_اشاریہ), vtxp,
                            "Expected exactly one trace result");
                اگر (m_tailp) {
                    // This vertex just passes the range through, so continue with its
                    // source, at the updated range. Its cache entry is filled in when
                    // the chain is resolved.
                    m_pendingps.ڈالو_عقب(&respr);
                    vtxp = m_tailp;
                    msb = m_msb;
                    lsb = m_lsb;
                    جاری;
                }
                respr = m_resp;
            }
            resp = respr;
            توڑو;
        }

        // We only ever trace drivers of bits that are known to be independent
        // of the cycles, so we should always be able to find an acyclic driver.
        UASSERT_OBJ(resp, آغاز_Vtxp, "Tracing driver failed for " << آغاز_Vtxp->typeName());
        UASSERT_OBJ(resp->چوڑائی() == (آغاز_Msb - آغاز_Lsb + 1), آغاز_Vtxp, "Wrong result width");
        // Every vertex on the pass-through chain resolves to the same driver
        برائے (سائز_t i = pendingBegin; i < m_pendingps.سائز(); ++i) *m_pendingps[i] = resp;
        m_pendingps.resize(pendingBegin);
        واپس resp;
    }

    // Trace drivers of the given array vertex, at the current m_idxs, m_msb, m_lsb.
    Dfg_راس* traceSameIdx(Dfg_راس* ثابت vtxp) {
        UASSERT_OBJ(vtxp->isArray(), vtxp, "Should be an array vertex");
        UASSERT_OBJ(!m_idxs.خالی_خانے(), vtxp, "Should have a pending index");
        VL_RESTORER(m_resp);
        m_resp = خالی_اشاریہ;
        iterate(vtxp);  // Resolve the element, the array visitors set 'm_resp'
        UASSERT_OBJ(m_resp, vtxp, "Tracing driver failed for " << vtxp->typeName());
        UASSERT_OBJ(m_resp->چوڑائی() == (m_msb - m_lsb + 1), vtxp, "Wrong result width");
        واپس m_resp;
    }

    // Trace drivers of the given array vertex, pushing the given index, at current m_msb, m_lsb.
    Dfg_راس* trace_ڈالو_Idx(Dfg_راس* ثابت vtxp, ثابت uint32_t idx) {
        ثابت سائز_t nIdxs = m_idxs.سائز();
        m_idxs.ڈالو_عقب(idx);
        Dfg_راس* ثابت resp = traceSameIdx(vtxp);
        m_idxs.نکالو_عقب();
        UASSERT_OBJ(m_idxs.سائز() == nIdxs, vtxp, "Index stack size mismatch");
        واپس resp;
    }

    // Trace drivers of the given vertex, popping the innermost index, at current m_msb, m_lsb.
    Dfg_راس* trace_نکالو_Idx(Dfg_راس* ثابت vtxp) {
        UASSERT_OBJ(!m_idxs.خالی_خانے(), vtxp, "Should have a pending index");
        ثابت سائز_t nIdxs = m_idxs.سائز();
        ثابت uint32_t idx = m_idxs.عقب();
        m_idxs.نکالو_عقب();
        Dfg_راس* ثابت resp = m_idxs.خالی_خانے() ? trace(vtxp, m_msb, m_lsb) : traceSameIdx(vtxp);
        m_idxs.ڈالو_عقب(idx);
        UASSERT_OBJ(m_idxs.سائز() == nIdxs, vtxp, "Index stack size mismatch");
        واپس resp;
    }

    سانچہ <قسم_نام راس_2>
    راس_2* trace_Bitwise_ثنائی(راس_2* vtxp) {
        static_assert(std::is_بنیاد_کا<Dfg_راس_ثنائی, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت resp = make<راس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), m_msb, m_lsb));
        resp->lhsp(trace(vtxp->lhsp(), m_msb, m_lsb));
        واپس resp;
    }

    سانچہ <قسم_نام راس_2>
    Dfg_راس* trace_جوڑو_Sub(راس_2* vtxp) {
        static_assert(std::is_بنیاد_کا<Dfg_راس_ثنائی, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت opp = make<راس_2>(vtxp, m_msb + 1);
        opp->rhsp(trace(vtxp->rhsp(), m_msb, 0));
        opp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(opp);
        selp->lsb(m_lsb);
        واپس selp;
    }

    سانچہ <قسم_نام راس_2>
    راس_2* traceReduction(راس_2* vtxp) {
        static_assert(std::is_بنیاد_کا<Dfg_راس_Unary, راس_2>::قدر,
                      "Should only call on DfgVertexUnary");
        راس_2* ثابت resp = make<راس_2>(vtxp, 1);
        resp->srcp(trace(vtxp->srcp(), vtxp->srcp()->چوڑائی() - 1, 0));
        واپس resp;
    }

    سانچہ <قسم_نام راس_2>
    راس_2* traceCmp(راس_2* vtxp) {
        static_assert(std::is_بنیاد_کا<Dfg_راس_ثنائی, راس_2>::قدر,
                      "Should only call on DfgVertexBinary");
        راس_2* ثابت resp = make<راس_2>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->چوڑائی() - 1, 0));
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->چوڑائی() - 1, 0));
        واپس resp;
    }

    // Use this macro to return the result from 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#اگر_تعریف_شدہ VL_DEBUG
#تعریف_کرو TRACE_نتیجہ_COVER() \
    کرو { \
        اگر (VL_UNLIKELY(m_سطر_Coverage_File.is_open())) m_سطر_Coverage_File << __LINE__ << '\n'; \
    } جب (جھوٹ)
#ورنہ
#تعریف_کرو TRACE_نتیجہ_COVER() \
    کرو { \
    } جب (جھوٹ)
#اگر_ختم

#تعریف_کرو RETURN_نتیجہ(vtxp) \
    کرو { \
        m_resp = (vtxp); \
        TRACE_نتیجہ_COVER(); \
        واپس; \
    } جب (جھوٹ)

    // Use this macro in 'visit' methods when the result is exactly the result of tracing
    // 'srcp' at [smsb:slsb]. 'trace' then continues iteratively instead of recursing, so
    // arbitrarily long chains of such pass-through vertices do not consume C++ stack.
    // Note this overwrites 'm_msb'/'m_lsb', which is fine as the visitor returns
    // immediately. Only use this when no state set up by the visitor (e.g.: 'm_defaultp')
    // has to stay live while 'srcp' is traced, as that happens after the visitor returned.
#تعریف_کرو RETURN_نتیجہ_دم(srcp, smsb, slsb) \
    کرو { \
        /* Evaluate all arguments before assigning, as they usually read 'm_msb'/'m_lsb' */ \
        Dfg_راس* ثابت tailp = (srcp); \
        ثابت uint32_t دم_Msb = (smsb); \
        ثابت uint32_t دم_Lsb = (slsb); \
        m_tailp = tailp; \
        m_msb = دم_Msb; \
        m_lsb = دم_Lsb; \
        TRACE_نتیجہ_COVER(); \
        واپس; \
    } جب (جھوٹ)

    // VISITORS
    خالی دورہ(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        vtxp->v3fatalSrc("TraceDriver - Unhandled vertex type: " << vtxp->typeName());
    }  // LCOV_EXCL_STOP

    خالی دورہ(DfgSplicePacked* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSplicePacked");
        Dfg_راس* defaultp = m_defaultp;
        m_defaultp = خالی_اشاریہ;
        m_splicep = خالی_اشاریہ;

        ساخت Driver final {
            Dfg_راس* m_vtxp;  // Vertex driving this range
            uint32_t m_lsb;  // LSB of driven range (internal, not Verilog)
            uint32_t m_msb;  // MSB of driven range (internal, not Verilog)
            Driver() = حذف;
            Driver(Dfg_راس* vtxp, uint32_t lsb, uint32_t msb)
                : m_vtxp{vtxp}
                , m_lsb{lsb}
                , m_msb{msb} {}
        };
        std::سمتیہ<Driver> drivers;

        // Look at all the drivers, one might cover the whole range, but also gather all drivers
        منطقی try_پورا_Default = defaultp;
        Dfg_راس* coverp = خالی_اشاریہ;  // Driver covering the whole searched range, if any
        uint32_t coverLsb = 0;  // LSB of the range driven by 'coverp'
        vtxp->foreachDriver([&](Dfg_راس& src, uint32_t lsb) {
            ثابت uint32_t msb = lsb + src.چوڑائی() - 1;
            drivers.emplace_عقب(&src, lsb, msb);
            // Check if this driver covers any of the bits, then we can't use whole default
            اگر (m_msb >= lsb && msb >= m_lsb) try_پورا_Default = جھوٹ;
            // If it does not cover the whole searched bit range, move on
            اگر (m_lsb < lsb || msb < m_msb) واپس جھوٹ;
            // Driver covers whole search range, we are done gathering
            coverp = &src;
            coverLsb = lsb;
            واپس سچ;
        });

        // Trace the driver covering the whole searched range, if there is one
        اگر (coverp) RETURN_نتیجہ_دم(coverp, m_msb - coverLsb, m_lsb - coverLsb);

        // Trace the default driver if no other drivers cover the searched range
        اگر (try_پورا_Default) RETURN_نتیجہ_دم(defaultp, m_msb, m_lsb);

        // Hard case: We need to combine multiple drivers to produce the searched bit range

        // Sort drivers (they are non-overlapping)
        std::ترتیب_دو(drivers.begin(), drivers.انتہا(), [](ثابت Driver& a, ثابت Driver& b) {  //
            واپس a.m_lsb < b.m_lsb;
        });

        // Gather terms
        std::سمتیہ<Dfg_راس*> termps;
        uint32_t lsb = m_lsb;
        برائے (ثابت Driver& driver : drivers) {
            // Driver is below the searched LSB, move on
            اگر (lsb > driver.m_msb) جاری;
            // Driver is above the searched MSB, done
            اگر (driver.m_lsb > m_msb) توڑو;
            // Gap below this driver, trace default to fill it
            اگر (driver.m_lsb > lsb) {
                UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
                termps.emplace_عقب(trace(defaultp, driver.m_lsb - 1, lsb));
                lsb = driver.m_lsb;
            }
            // Driver covers searched range, pick the needed/available bits
            ثابت uint32_t lim = std::min(m_msb, driver.m_msb);
            termps.emplace_عقب(trace(driver.m_vtxp, lim - driver.m_lsb, lsb - driver.m_lsb));
            lsb = lim + 1;
        }
        اگر (m_msb >= lsb) {
            UASSERT_OBJ(defaultp, vtxp, "Should have a default driver if needs tracing");
            termps.emplace_عقب(trace(defaultp, m_msb, lsb));
        }

        // The earlier cheks cover the case when either a whole driver or the default covers
        // the whole range, so there should be at least 2 terms required here.
        UASSERT_OBJ(termps.سائز() >= 2, vtxp, "Should have returned in special cases");

        // Concatenate all terms and set result
        Dfg_راس* resp = termps.front();
        برائے (سائز_t i = 1; i < termps.سائز(); ++i) {
            Dfg_راس* ثابت termp = termps[i];
            DfgConcat* ثابت catp = make<DfgConcat>(termp, resp->چوڑائی() + termp->چوڑائی());
            catp->rhsp(resp);
            catp->lhsp(termp);
            resp = catp;
        }
        RETURN_نتیجہ(resp);
    }

    خالی دورہ(DfgSpliceArray* vtxp) override {
        UASSERT_OBJ(m_splicep == vtxp, vtxp, "Unexpected trace of DfgSpliceArray");
        Dfg_راس* ثابت defaultp = m_defaultp;
        m_defaultp = خالی_اشاریہ;
        m_splicep = خالی_اشاریہ;

        // Explicit per-element driver (a UnitArray wrapping the element value)
        ثابت uint32_t idx = m_idxs.عقب();
        اگر (Dfg_راس* ثابت driverp = vtxp->driverAt(idx)) {
            Dfg_راس* ثابت srcp = driverp->as<DfgUnitArray>()->srcp();
            اگر (srcp->is<Dfg_راس_Splice>()) {
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

    خالی دورہ(Dfg_راس_Var* vtxp) override {
        UASSERT_OBJ(!vtxp->isVolatile(), vtxp, "Should not trace through volatile variable");
        Dfg_راس* ثابت srcp = vtxp->srcp();
        Dfg_راس* ثابت defaultp = vtxp->defaultp();
        Dfg_راس* ثابت drvp = srcp ? srcp : defaultp;
        // If we are about to trace a splice, set the defaultp to the corresponding default
        اگر (srcp && srcp->is<Dfg_راس_Splice>()) {
            m_defaultp = defaultp;
            m_splicep = srcp;
        }
        UASSERT_OBJ(drvp, vtxp, "Should not have to trace undriven variable");
        // Packed variable: trace the driver. Array variable: continue navigating it at
        // the pending element (both at the same bit range).
        RETURN_نتیجہ(m_idxs.خالی_خانے() ? trace(drvp, m_msb, m_lsb) : traceSameIdx(drvp));
    }

    خالی دورہ(DfgArraySel* vtxp) override {
        // If constant index, push it and trace the selected element through the array
        // structure of 'fromp'. This handles arbitrarily nested (multi-dimensional)
        // arrays, as each nested ArraySel pushes a further index.
        اگر (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            RETURN_نتیجہ(trace_ڈالو_Idx(vtxp->fromp(), idxp->مقصد_U32()));
        }

        // If index is not constant, independence was proven only if the 'fromp' is
        // independent, so no need to trace that, just reference it with the traced
        // index. This only happens at the packed ArraySel leaf.
        UASSERT_OBJ(m_idxs.خالی_خانے(), vtxp, "Non-constant index below an outer array index");
        DfgArraySel* ثابت resp = make<DfgArraySel>(vtxp, vtxp->چوڑائی());
        resp->fromp(vtxp->fromp());
        resp->bitp(trace(vtxp->bitp(), vtxp->bitp()->چوڑائی() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_نتیجہ(selp);
    }

    خالی دورہ(DfgUnitArray* vtxp) override {
        // Single-element array adapter, the pending index must be 0, unwrap the element
        UASSERT_OBJ(m_idxs.عقب() == 0, vtxp, "UnitArray element index should be 0");
        RETURN_نتیجہ(trace_نکالو_Idx(vtxp->srcp()));
    }

    خالی دورہ(DfgConcat* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t r_چوڑائی = rhsp->چوڑائی();
        // If the traced bits are wholly in the RHS
        اگر (r_چوڑائی > m_msb) RETURN_نتیجہ_دم(rhsp, m_msb, m_lsb);
        // If the traced bits are wholly in the LHS
        اگر (m_lsb >= r_چوڑائی) RETURN_نتیجہ_دم(lhsp, m_msb - r_چوڑائی, m_lsb - r_چوڑائی);
        // The traced bit spans both sides, trace both
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(rhsp, r_چوڑائی - 1, m_lsb));
        resp->lhsp(trace(lhsp, m_msb - r_چوڑائی, 0));
        RETURN_نتیجہ(resp);
    }

    خالی دورہ(DfgRep* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        // If we need more bits than the source, then we need the whole source
        اگر (m_msb - m_lsb + 1 > s_چوڑائی) {
            DfgRep* ثابت repp = make<DfgRep>(vtxp, vtxp->چوڑائی());
            repp->srcp(trace(srcp, s_چوڑائی - 1, 0));
            DfgSel* ثابت resp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
            resp->fromp(repp);
            resp->lsb(m_lsb);
            RETURN_نتیجہ(resp);
        }
        // If the requested bits are within the same repliacted word
        اگر (m_msb / s_چوڑائی == m_lsb / s_چوڑائی) {
            RETURN_نتیجہ_دم(srcp, m_msb % s_چوڑائی, m_lsb % s_چوڑائی);
        }
        // The requested bits span two replicated words
        DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
        resp->rhsp(trace(srcp, s_چوڑائی - 1, m_lsb % s_چوڑائی));
        resp->lhsp(trace(srcp, m_msb % s_چوڑائی, 0));
        RETURN_نتیجہ(resp);
    }

    خالی دورہ(DfgExtend* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        // If the traced bits are wholly in the input
        اگر (s_چوڑائی > m_msb) RETURN_نتیجہ_دم(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        اگر (m_lsb >= s_چوڑائی) RETURN_نتیجہ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
        // The traced bits span both sides
        DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_چوڑائی - 1, m_lsb));
        RETURN_نتیجہ(resp);
    }

    خالی دورہ(DfgExtendS* vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        // If the traced bits are wholly in the input
        اگر (s_چوڑائی > m_msb) RETURN_نتیجہ_دم(srcp, m_msb, m_lsb);
        // If the traced bits are wholly in the extension
        اگر (m_lsb >= s_چوڑائی) {
            اگر (m_msb == m_lsb) RETURN_نتیجہ_دم(srcp, s_چوڑائی - 1, s_چوڑائی - 1);
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(srcp, s_چوڑائی - 1, s_چوڑائی - 1));
            RETURN_نتیجہ(resp);
        }
        // The traced bits span both sides
        DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(srcp, s_چوڑائی - 1, m_lsb));
        RETURN_نتیجہ(resp);
    }

    خالی دورہ(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        RETURN_نتیجہ_دم(vtxp->srcp(), m_msb + lsb, m_lsb + lsb);
    }

    خالی دورہ(DfgNot* vtxp) override {
        DfgNot* ثابت resp = make<DfgNot>(vtxp, m_msb - m_lsb + 1);
        resp->srcp(trace(vtxp->srcp(), m_msb, m_lsb));
        RETURN_نتیجہ(resp);
    }

    خالی دورہ(DfgAnd* vtxp) override { RETURN_نتیجہ(trace_Bitwise_ثنائی(vtxp)); }
    خالی دورہ(DfgOr* vtxp) override { RETURN_نتیجہ(trace_Bitwise_ثنائی(vtxp)); }
    خالی دورہ(DfgXor* vtxp) override { RETURN_نتیجہ(trace_Bitwise_ثنائی(vtxp)); }
    خالی دورہ(Dfg_جوڑو* vtxp) override { RETURN_نتیجہ(trace_جوڑو_Sub(vtxp)); }
    خالی دورہ(DfgSub* vtxp) override { RETURN_نتیجہ(trace_جوڑو_Sub(vtxp)); }
    خالی دورہ(DfgRedAnd* vtxp) override { RETURN_نتیجہ(traceReduction(vtxp)); }
    خالی دورہ(DfgRedOr* vtxp) override { RETURN_نتیجہ(traceReduction(vtxp)); }
    خالی دورہ(DfgRedXor* vtxp) override { RETURN_نتیجہ(traceReduction(vtxp)); }
    خالی دورہ(DfgEq* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    خالی دورہ(DfgNeq* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    خالی دورہ(DfgLt* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    خالی دورہ(DfgLte* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    خالی دورہ(DfgGt* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }
    خالی دورہ(DfgGte* vtxp) override { RETURN_نتیجہ(traceCmp(vtxp)); }

    خالی دورہ(Dfg_سرکاؤ_RS* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        اگر (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t سرکاؤ_Amnt = rConstp->مقصد_U32();
            // Width of lower half of result
            ثابت uint32_t زیریں_چوڑائی = سرکاؤ_Amnt > vtxp->چوڑائی() ? 0 : vtxp->چوڑائی() - سرکاؤ_Amnt;
            // If the traced bits are wholly in the input
            اگر (زیریں_چوڑائی > m_msb) {
                RETURN_نتیجہ_دم(lhsp, m_msb + سرکاؤ_Amnt, m_lsb + سرکاؤ_Amnt);
            }
            // If the traced bits are wholly in the extension
            اگر (m_lsb >= زیریں_چوڑائی) {
                DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
                resp->srcp(trace(lhsp, lhsp->چوڑائی() - 1, lhsp->چوڑائی() - 1));
                RETURN_نتیجہ(resp);
            }
            // The traced bits span both sides
            DfgExtendS* ثابت resp = make<DfgExtendS>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, زیریں_چوڑائی - 1 + سرکاؤ_Amnt, m_lsb + سرکاؤ_Amnt));
            RETURN_نتیجہ(resp);
        }

        Dfg_سرکاؤ_RS* ثابت shiftrsp = make<Dfg_سرکاؤ_RS>(vtxp, vtxp->lhsp()->چوڑائی() - m_lsb);
        shiftrsp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->چوڑائی() - 1, 0));
        shiftrsp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->چوڑائی() - 1, m_lsb));
        اگر (m_msb == vtxp->lhsp()->چوڑائی() - 1) RETURN_نتیجہ(shiftrsp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrsp);
        selp->lsb(0);
        RETURN_نتیجہ(selp);
    }

    خالی دورہ(Dfg_سرکاؤ_R* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        اگر (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t سرکاؤ_Amnt = rConstp->مقصد_U32();
            // Width of lower half of result
            ثابت uint32_t زیریں_چوڑائی = سرکاؤ_Amnt > vtxp->چوڑائی() ? 0 : vtxp->چوڑائی() - سرکاؤ_Amnt;
            // If the traced bits are wholly in the input
            اگر (زیریں_چوڑائی > m_msb) {
                RETURN_نتیجہ_دم(lhsp, m_msb + سرکاؤ_Amnt, m_lsb + سرکاؤ_Amnt);
            }
            // If the traced bits are wholly in the extension
            اگر (m_lsb >= زیریں_چوڑائی) RETURN_نتیجہ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgExtend* ثابت resp = make<DfgExtend>(vtxp, m_msb - m_lsb + 1);
            resp->srcp(trace(lhsp, زیریں_چوڑائی - 1 + سرکاؤ_Amnt, m_lsb + سرکاؤ_Amnt));
            RETURN_نتیجہ(resp);
        }

        Dfg_سرکاؤ_R* ثابت shiftrp = make<Dfg_سرکاؤ_R>(vtxp, vtxp->lhsp()->چوڑائی() - m_lsb);
        shiftrp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->چوڑائی() - 1, 0));
        shiftrp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->چوڑائی() - 1, m_lsb));
        اگر (m_msb == vtxp->lhsp()->چوڑائی() - 1) RETURN_نتیجہ(shiftrp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftrp);
        selp->lsb(0);
        RETURN_نتیجہ(selp);
    }

    خالی دورہ(Dfg_سرکاؤ_L* vtxp) override {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        اگر (ثابت DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ثابت uint32_t سرکاؤ_Amnt = rConstp->مقصد_U32();
            // Width of lower half of result
            ثابت uint32_t زیریں_چوڑائی = سرکاؤ_Amnt > vtxp->چوڑائی() ? vtxp->چوڑائی() : سرکاؤ_Amnt;
            // If the traced bits are wholly in the input
            اگر (m_lsb >= زیریں_چوڑائی) {
                RETURN_نتیجہ_دم(lhsp, m_msb - سرکاؤ_Amnt, m_lsb - سرکاؤ_Amnt);
            }
            // If the traced bits are wholly in the extension
            اگر (زیریں_چوڑائی > m_msb) RETURN_نتیجہ(make<DfgConst>(vtxp, m_msb - m_lsb + 1));
            // The traced bits span both sides
            DfgConcat* ثابت resp = make<DfgConcat>(vtxp, m_msb - m_lsb + 1);
            resp->rhsp(make<DfgConst>(vtxp, resp->چوڑائی() - (m_msb - زیریں_چوڑائی + 1)));
            resp->lhsp(trace(lhsp, m_msb - سرکاؤ_Amnt, زیریں_چوڑائی - سرکاؤ_Amnt));
            RETURN_نتیجہ(resp);
        }

        Dfg_سرکاؤ_L* ثابت shiftlp = make<Dfg_سرکاؤ_L>(vtxp, m_msb + 1);
        shiftlp->rhsp(trace(vtxp->rhsp(), vtxp->rhsp()->چوڑائی() - 1, 0));
        shiftlp->lhsp(trace(vtxp->lhsp(), m_msb, 0));
        اگر (m_lsb == 0) RETURN_نتیجہ(shiftlp);
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(shiftlp);
        selp->lsb(m_lsb);
        RETURN_نتیجہ(selp);
    }

    خالی دورہ(DfgCond* vtxp) override {
        DfgCond* ثابت resp = make<DfgCond>(vtxp, m_msb - m_lsb + 1);
        resp->condp(trace(vtxp->condp(), vtxp->condp()->چوڑائی() - 1, 0));
        resp->thenp(trace(vtxp->thenp(), m_msb, m_lsb));
        resp->elsep(trace(vtxp->elsep(), m_msb, m_lsb));
        RETURN_نتیجہ(resp);
    }

    خالی دورہ(Dfg_مطابقت_Masked* vtxp) override {
        Dfg_مطابقت_Masked* ثابت resp = make<Dfg_مطابقت_Masked>(vtxp, vtxp->چوڑائی());
        resp->lhsp(trace(vtxp->lhsp(), vtxp->lhsp()->چوڑائی() - 1, 0));
        resp->matchp(trace(vtxp->matchp(), vtxp->matchp()->چوڑائی() - 1, 0));
        DfgSel* ثابت selp = make<DfgSel>(vtxp, m_msb - m_lsb + 1);
        selp->fromp(resp);
        selp->lsb(m_lsb);
        RETURN_نتیجہ(selp);
    }

#تعریف_منسوخ RETURN_نتیجہ
#تعریف_منسوخ TRACE_نتیجہ_COVER
#تعریف_منسوخ RETURN_نتیجہ_دم

عوامی:
    // CONSTRUCTOR
    TraceDriver(Dfg_گراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {
#اگر_تعریف_شدہ VL_DEBUG
        اگر (v3Global.opt.debugCheck()) {
            m_سطر_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.سابقہ()
                    + "__V3DfgBreakCycles-TraceDriver-line-coverage.txt",  //
                std::ios_بنیاد::برآمد | std::ios_بنیاد::app);
        }
#اگر_ختم
    }

    // Given a Vertex that is part of an SCC, return a vertex that is equivalent
    // to 'vtxp[lsb +: width]', but is not part of the same SCC. This should only
    // be called if the bit range is known to be independent of the SCC, so the
    // trace can always succeed.
    Dfg_راس* apply(Dfg_راس& vtx, uint32_t lsb, uint32_t چوڑائی) {
        VL_RESTORER(m_component);
        m_component = m_sccInfo.get(vtx);
        واپس trace(&vtx, lsb + چوڑائی - 1, lsb);
    }
};

// A bit mask for each bit in a value, which can be either packed or aggregate type
جماعت ثنائی_ہندسہ_نقاب final {
    // TYPES
    ثابت Dfg_ڈیٹا_Type& m_dtype;
    اتحاد {
        V3_نمبر m_عدد;  // The bits, if packed
        std::سمتیہ<ثنائی_ہندسہ_نقاب> m_sub;  // The per element masks, if aggregate
    };

عوامی:
    // CONSTRUCTOR
    ثنائی_ہندسہ_نقاب(File_سطر* flp, ثابت Dfg_ڈیٹا_Type& dtype)
        : m_dtype{dtype} {
        UASSERT_OBJ(!m_dtype.isNull(), flp, "Expected non-null data type");
        اگر (m_dtype.isPacked()) {
            نیا (&m_عدد) V3_نمبر{flp, static_cast<صحیح>(m_dtype.سائز()), 0};
        } ورنہ {
            نیا (&m_sub) std::سمتیہ<ثنائی_ہندسہ_نقاب>{};
            m_sub.reserve(m_dtype.سائز());
            برائے (uint32_t i = 0; i < m_dtype.سائز(); ++i)
                m_sub.emplace_عقب(flp, m_dtype.elemDtype());
        }
    }
    ثنائی_ہندسہ_نقاب(ثابت Dfg_راس& vtx)
        : ثنائی_ہندسہ_نقاب{vtx.fileline(), vtx.dtype()} {}
    ثنائی_ہندسہ_نقاب(ثابت ثنائی_ہندسہ_نقاب& other)
        : m_dtype{other.m_dtype} {
        اگر (m_dtype.isPacked()) {
            نیا (&m_عدد) V3_نمبر{other.m_عدد};
        } ورنہ {
            نیا (&m_sub) std::سمتیہ<ثنائی_ہندسہ_نقاب>{other.m_sub};
        }
    }
    ثنائی_ہندسہ_نقاب& عامل=(ثابت ثنائی_ہندسہ_نقاب& other) {
        اگر (یہ != &other) {
            UASSERT(m_dtype == other.m_dtype, "Expected same data type");
            اگر (other.m_dtype.isPacked()) {
                m_عدد = other.m_عدد;
            } ورنہ {
                m_sub = other.m_sub;
            }
        }
        واپس *یہ;
    }
    ~ثنائی_ہندسہ_نقاب() {
        اگر (m_dtype.isPacked()) {
            m_عدد.~V3_نمبر();
        } ورنہ {
            m_sub.~سمتیہ<ثنائی_ہندسہ_نقاب>();
        }
    }

    // METHODS
    V3_نمبر& عدد() {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        واپس m_عدد;
    }
    ثابت V3_نمبر& عدد() ثابت {
        UASSERT(m_dtype.isPacked(), "Expected packed data type");
        واپس m_عدد;
    }
    std::سمتیہ<ثنائی_ہندسہ_نقاب>& sub() {
        UASSERT(m_dtype.isArray(), "Expected array data type");
        واپس m_sub;
    }

    منطقی isZero() ثابت {
        اگر (m_dtype.isPacked()) واپس m_عدد.isEqZero();

        برائے (ثابت ثنائی_ہندسہ_نقاب& sub : m_sub) {
            اگر (!sub.isZero()) واپس جھوٹ;
        }
        واپس سچ;
    }
    منطقی isOnes() ثابت {
        اگر (m_dtype.isPacked()) واپس m_عدد.isEqAllOnes();

        برائے (ثابت ثنائی_ہندسہ_نقاب& sub : m_sub) {
            اگر (!sub.isOnes()) واپس جھوٹ;
        }
        واپس سچ;
    }
    خالی سیٹ_Zero() {
        اگر (m_dtype.isPacked()) {
            m_عدد.سیٹ_All_Bits0();
        } ورنہ {
            برائے (ثنائی_ہندسہ_نقاب& sub : m_sub) sub.سیٹ_Zero();
        }
    }
    خالی سیٹ_Ones() {
        اگر (m_dtype.isPacked()) {
            m_عدد.سیٹ_All_Bits1();
        } ورنہ {
            برائے (ثنائی_ہندسہ_نقاب& sub : m_sub) sub.سیٹ_Ones();
        }
    }

    منطقی عامل==(ثابت ثنائی_ہندسہ_نقاب& other) ثابت {
        UASSERT(m_dtype == other.m_dtype, "Expected same data type");
        اگر (m_dtype.isPacked()) واپس m_عدد.isCaseEq(other.m_عدد);
        واپس m_sub == other.m_sub;
    }
    منطقی عامل!=(ثابت ثنائی_ہندسہ_نقاب& other) ثابت { واپس !(*یہ == other); }

    // 'this' has a clear bit where 'that' has a set bit, that is,
    // 'this' has a proper subset of bits set compared to 'that'.
    منطقی is_Contraction_کا(ثابت ثنائی_ہندسہ_نقاب& that) ثابت {
        UASSERT(m_dtype == that.m_dtype, "Expected same data type");
        اگر (m_dtype.isPacked()) {
            ثابت سائز_t الفاظ = VL_الفاظ_I(m_dtype.سائز());
            برائے (سائز_t i = 0; i < الفاظ; ++i) {
                اگر (~m_عدد.edata_لفظ(i) & that.m_عدد.edata_لفظ(i)) واپس سچ;
            }
            واپس جھوٹ;
        }

        UASSERT(m_dtype.isArray(), "Expected array data type");
        برائے (سائز_t i = 0; i < m_sub.سائز(); ++i) {
            اگر (m_sub[i].is_Contraction_کا(that.m_sub[i])) واپس سچ;
        }
        واپس جھوٹ;
    }

    std::string مقصد_String() ثابت {
        اگر (m_dtype.isPacked()) واپس "0x" + m_عدد.displayed(m_عدد.fileline(), "%x");
        std::string نتیجہ;
        نتیجہ += "{";
        برائے (ثابت ثنائی_ہندسہ_نقاب& sub : m_sub) {
            اگر (&sub != &m_sub.front()) نتیجہ += ", ";
            نتیجہ += sub.مقصد_String();
        }
        نتیجہ += "}";
        واپس نتیجہ;
    }
};

جماعت Independent_بٹ final : عوامی DfgVisitor {
    // TYPES
    ساخت راس_State final {
        سائز_t m_rpo_نمبر = 0;  // Reverse Postorder Number of vertex
        منطقی m_isOnWorkList = جھوٹ;  // Whether the vertex is on the work list
    };

    // STATE
    ثابت SccInfo& m_sccInfo;  // The SccInfo instance
    // Vertex to current bit mask map. The mask is set for the bits that are independent of the SCC
    std::unordered_map<ثابت Dfg_راس*, ثنائی_ہندسہ_نقاب> m_vtxp2_نقاب;
    // Work list for the traversal (min-queue of vertex RPO numbers)
    std::ترجیح_قطار<سائز_t, std::سمتیہ<سائز_t>, std::greater<سائز_t>> m_work_قطار;
    std::unordered_map<ثابت Dfg_راس*, راس_State> m_vtxp2State;  // State of each vertex

#اگر_تعریف_شدہ VL_DEBUG
    std::ofstream m_سطر_Coverage_File;  // Line coverage file, just for testing
#اگر_ختم

    // METHODS

    // Retrieve the mask for the given vertex (create it with value 0 if needed)
    ثنائی_ہندسہ_نقاب& نقاب(ثابت Dfg_راس& vtx) {
        // Look up (or create) mask for 'vtx'
        واپس m_vtxp2_نقاب
            .emplace(std::piecewise_construct,  //
                     std::forward_as_tuple(&vtx), std::forward_as_tuple(vtx))
            .پہلا->دوسرا;
    }

    // Use this macro to call 'mask' in 'visit' methods. This also emits
    // a line to m_lineCoverageFile for testing.
    // TODO: Use C++20 std::source_location instead of a macro
#اگر_تعریف_شدہ VL_DEBUG
#تعریف_کرو نقاب_2(vtxp) \
    ([یہ](ثابت Dfg_راس& vtx) -> ثنائی_ہندسہ_نقاب& { \
        اگر (VL_UNLIKELY(m_سطر_Coverage_File.is_open())) m_سطر_Coverage_File << __LINE__ << '\n'; \
        واپس نقاب(vtx); \
    }(*vtxp))
#ورنہ
#تعریف_کرو نقاب_2(vtxp) نقاب(*vtxp)
#اگر_ختم

    // Clear all bits at or below the most significant clear bit
    خالی floodTowardsLsb(V3_نمبر& عدد) {
        برائے (صحیح i = عدد.چوڑائی() - 1; i >= 0; --i) {
            اگر (عدد.ثنائی_ہندسہ_Is1(i)) جاری;
            عدد.op_سیٹ_Range(0, i + 1, '0');
            توڑو;
        }
    }

    // Clear all bits at or above the least significant clear bit
    خالی floodTowardsMsb(V3_نمبر& عدد) {
        برائے (صحیح i = 0; i < عدد.چوڑائی(); ++i) {
            اگر (عدد.ثنائی_ہندسہ_Is1(i)) جاری;
            عدد.op_سیٹ_Range(i, عدد.چوڑائی() - i, '0');
            توڑو;
        }
    }

    خالی propagateFromDriver(ثنائی_ہندسہ_نقاب& m, ثابت Dfg_راس* srcp) {
        // If there is no driver, we are done
        اگر (!srcp) واپس;
        // If it is driven by a splice, we need to combine the masks of the drivers
        اگر (ثابت DfgSplicePacked* ثابت splicep = srcp->cast<DfgSplicePacked>()) {
            splicep->foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
                m.عدد().opSelInto(نقاب_2(&src).عدد(), lo, src.چوڑائی());
                واپس جھوٹ;
            });
            واپس;
        }
        اگر (ثابت DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            splicep->foreachDriver([&](ثابت Dfg_راس& src, uint32_t lo) {
                اگر (ثابت DfgUnitArray* ثابت uap = src.cast<DfgUnitArray>()) {
                    m.sub().at(lo) = نقاب_2(uap->srcp());
                } ورنہ {
                    // m.sub().at(lo) = Can't happen MASK(&src);
                }
                واپس جھوٹ;
            });
            واپس;
        }
        // Otherwise, we just use the mask of the single driver
        m = نقاب_2(srcp);
    }

    // VISITORS
    خالی دورہ(Dfg_راس* vtxp) override {  // LCOV_EXCL_START
        UINFO(9, "IndependentBits - Unhandled vertex type: " << vtxp->typeName());
        // Conservative assumption about all bits being dependent prevails
    }  // LCOV_EXCL_STOP

    خالی دورہ(Dfg_راس_Var* vtxp) override {
        // We cannot trace through a volatile variable, so pretend all bits are dependent
        اگر (vtxp->isVolatile()) واپس;

        ثنائی_ہندسہ_نقاب& m = نقاب_2(vtxp);
        Dfg_راس* ثابت srcp = vtxp->srcp();
        Dfg_راس* ثابت defaultp = vtxp->defaultp();
        // If there is a default driver, we start from that
        اگر (defaultp) m = نقاب_2(defaultp);
        // Then propagate mask from the driver
        propagateFromDriver(m, srcp);
    }

    خالی دورہ(Dfg_راس_Splice* vtxp) override {
        propagateFromDriver(نقاب_2(vtxp), vtxp);  // Needed to continue traversal
    }

    خالی دورہ(DfgUnitArray* vtxp) override { نقاب_2(vtxp).sub().at(0) = نقاب_2(vtxp->srcp()); }

    خالی دورہ(DfgArraySel* vtxp) override {
        Dfg_راس* ثابت fromp = vtxp->fromp();

        // If constant index, copy mask of relevant element
        اگر (ثابت DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>()) {
            نقاب_2(vtxp) = نقاب_2(vtxp->fromp()).sub().at(idxp->مقصد_سائز_T());
            واپس;
        }

        // If index is not constant, independent only if the index is indenpendent, and the array
        // is independent. TODO: could relax by '&' reducing, not sure if worth it.
        اگر (نقاب_2(vtxp->bitp()).isOnes() && نقاب_2(fromp).isOnes()) نقاب_2(vtxp).سیٹ_Ones();
    }

    خالی دورہ(DfgConcat* vtxp) override {
        ثابت Dfg_راس* ثابت rhsp = vtxp->rhsp();
        ثابت Dfg_راس* ثابت lhsp = vtxp->lhsp();
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opSelInto(نقاب_2(rhsp).عدد(), 0, rhsp->چوڑائی());
        m.opSelInto(نقاب_2(lhsp).عدد(), rhsp->چوڑائی(), lhsp->چوڑائی());
    }

    خالی دورہ(DfgRep* vtxp) override {
        ثابت uint32_t گنتی = vtxp->گنتی();
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        V3_نمبر& v_نقاب = نقاب_2(vtxp).عدد();
        V3_نمبر& s_نقاب = نقاب_2(srcp).عدد();
        برائے (uint32_t i = 0; i < گنتی; ++i) v_نقاب.opSelInto(s_نقاب, i * s_چوڑائی, s_چوڑائی);
    }

    خالی دورہ(DfgSel* vtxp) override {
        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t msb = lsb + vtxp->چوڑائی() - 1;
        نقاب_2(vtxp).عدد().opSel(نقاب_2(vtxp->fromp()).عدد(), msb, lsb);
    }

    خالی دورہ(DfgExtend* vtxp) override {
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        V3_نمبر& s = نقاب_2(srcp).عدد();
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opSelInto(s, 0, s_چوڑائی);
        m.op_سیٹ_Range(s_چوڑائی, vtxp->چوڑائی() - s_چوڑائی, '1');
    }
    خالی دورہ(DfgExtendS* vtxp) override {
        ثابت Dfg_راس* ثابت srcp = vtxp->srcp();
        ثابت uint32_t s_چوڑائی = srcp->چوڑائی();
        V3_نمبر& s = نقاب_2(srcp).عدد();
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opSelInto(s, 0, s_چوڑائی);
        m.op_سیٹ_Range(s_چوڑائی, vtxp->چوڑائی() - s_چوڑائی, s.ثنائی_ہندسہ_Is0(s_چوڑائی - 1) ? '0' : '1');
    }

    خالی دورہ(DfgNot* vtxp) override {  //
        نقاب_2(vtxp) = نقاب_2(vtxp->srcp());
    }

    خالی دورہ(DfgAnd* vtxp) override {  //
        نقاب_2(vtxp).عدد().opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
    }
    خالی دورہ(DfgOr* vtxp) override {  //
        نقاب_2(vtxp).عدد().opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
    }
    خالی دورہ(DfgXor* vtxp) override {  //
        نقاب_2(vtxp).عدد().opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
    }

    خالی دورہ(Dfg_جوڑو* vtxp) override {
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
        floodTowardsMsb(m);
    }
    خالی دورہ(DfgSub* vtxp) override {  // Same as Add: 2's complement (a - b) == (a + ~b + 1)
        V3_نمبر& m = نقاب_2(vtxp).عدد();
        m.opAnd(نقاب_2(vtxp->lhsp()).عدد(), نقاب_2(vtxp->rhsp()).عدد());
        floodTowardsMsb(m);
    }

    خالی دورہ(DfgRedAnd* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    خالی دورہ(DfgRedOr* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    خالی دورہ(DfgRedXor* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }

    خالی دورہ(DfgEq* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    خالی دورہ(DfgNeq* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    خالی دورہ(DfgLt* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    خالی دورہ(DfgLte* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    خالی دورہ(DfgGt* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }
    خالی دورہ(DfgGte* vtxp) override {
        ثابت منطقی independent = نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->rhsp()).isOnes();
        اگر (independent) نقاب_2(vtxp).سیٹ_Ones();
    }

    خالی دورہ(Dfg_سرکاؤ_RS* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();

        // Constant shift can be computed precisely
        اگر (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرکاؤ_Amount = rConstp->مقصد_U32();
            اگر (سرکاؤ_Amount >= چوڑائی) {
                اگر (نقاب_2(lhsp).عدد().ثنائی_ہندسہ_Is1(چوڑائی - 1)) { نقاب_2(vtxp).سیٹ_Ones(); }
            } ورنہ {
                V3_نمبر& m = نقاب_2(vtxp).عدد();
                m.op_سرکاؤ_RS(نقاب_2(lhsp).عدد(), rConstp->عدد(), چوڑائی);
                m.op_سیٹ_Range(چوڑائی - سرکاؤ_Amount, سرکاؤ_Amount, '1');
            }
            واپس;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        اگر (نقاب_2(rhsp).isOnes()) {
            V3_نمبر& m = نقاب_2(vtxp).عدد();
            m = نقاب_2(lhsp).عدد();
            floodTowardsLsb(m);
        }
    }

    خالی دورہ(Dfg_سرکاؤ_R* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();

        // Constant shift can be computed precisely
        اگر (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرکاؤ_Amount = rConstp->مقصد_U32();
            اگر (سرکاؤ_Amount >= چوڑائی) {
                نقاب_2(vtxp).سیٹ_Ones();
            } ورنہ {
                V3_نمبر& m = نقاب_2(vtxp).عدد();
                m.op_سرکاؤ_R(نقاب_2(lhsp).عدد(), rConstp->عدد());
                m.op_سیٹ_Range(چوڑائی - سرکاؤ_Amount, سرکاؤ_Amount, '1');
            }
            واپس;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive top bits in the lhs yield an independent result
        // if the shift amount is independent.
        اگر (نقاب_2(rhsp).isOnes()) {
            V3_نمبر& m = نقاب_2(vtxp).عدد();
            m = نقاب_2(lhsp).عدد();
            floodTowardsLsb(m);
        }
    }

    خالی دورہ(Dfg_سرکاؤ_L* vtxp) override {
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();

        // Constant shift can be computed precisely
        اگر (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرکاؤ_Amount = rConstp->مقصد_U32();
            اگر (سرکاؤ_Amount >= چوڑائی) {
                نقاب_2(vtxp).سیٹ_Ones();
            } ورنہ {
                V3_نمبر& m = نقاب_2(vtxp).عدد();
                m.op_سرکاؤ_L(نقاب_2(lhsp).عدد(), rConstp->عدد());
                m.op_سیٹ_Range(0, سرکاؤ_Amount, '1');
            }
            واپس;
        }

        // Otherwise, as the shift amount is non-negative, all independent
        // and consecutive bottom bits in the lhs yield an independent result
        // if the shift amount is independent.
        اگر (نقاب_2(rhsp).isOnes()) {
            V3_نمبر& m = نقاب_2(vtxp).عدد();
            m = نقاب_2(lhsp).عدد();
            floodTowardsMsb(m);
        }
    }

    خالی دورہ(DfgCond* vtxp) override {
        اگر (نقاب_2(vtxp->condp()).isOnes()) {
            نقاب_2(vtxp).عدد().opAnd(نقاب_2(vtxp->thenp()).عدد(), نقاب_2(vtxp->elsep()).عدد());
        }
    }

    خالی دورہ(Dfg_مطابقت_Masked* vtxp) override {
        اگر (نقاب_2(vtxp->lhsp()).isOnes() && نقاب_2(vtxp->matchp()).isOnes()) { نقاب_2(vtxp).سیٹ_Ones(); }
    }

#تعریف_منسوخ نقاب_2

    // Enqueue sinks of vertex to work list for traversal - only called from constructor
    خالی قطار_میں_ڈالو_Sinks(Dfg_راس& vtx) {
        vtx.foreachSink([&](Dfg_راس& sink) {
            // Ignore if sink is not part of an SCC, already has all bits marked independent
            اگر (!m_sccInfo.get(sink)) واپس جھوٹ;
            // Otherwise just enqueue it
            راس_State& state = m_vtxp2State.at(&sink);
            اگر (!state.m_isOnWorkList) {
                UINFO(9, "Enqueuing vertex " << &sink << " " << sink.typeName());
                m_work_قطار.ڈالو(state.m_rpo_نمبر);
            }
            state.m_isOnWorkList = سچ;
            واپس جھوٹ;
        });
    };

    خالی dfs_Post_ترتیب(std::unordered_سیٹ<Dfg_راس*>& دیکھا,  //
                      std::سمتیہ<Dfg_راس*>& post_ترتیب_Enumeration,  //
                      Dfg_راس& vtx) {
        // Mark visited, skip if already visited
        اگر (!دیکھا.داخل_کرو(&vtx).دوسرا) واپس;
        // Visit un-visited successors
        vtx.foreachSink([&](Dfg_راس& snk) {
            dfs_Post_ترتیب(دیکھا, post_ترتیب_Enumeration, snk);
            واپس جھوٹ;
        });
        // Add to post order enumeration
        post_ترتیب_Enumeration.emplace_عقب(&vtx);
    };

    Independent_بٹ(Dfg_گراف& dfg, ثابت SccInfo& sccInfo)
        : m_sccInfo{sccInfo} {

#اگر_تعریف_شدہ VL_DEBUG
        اگر (v3Global.opt.debugCheck()) {
            m_سطر_Coverage_File.open(  //
                v3Global.opt.makeDir() + "/" + v3Global.opt.سابقہ()
                    + "__V3DfgBreakCycles-IndependentBits-line-coverage.txt",  //
                std::ios_بنیاد::برآمد | std::ios_بنیاد::app);
        }
#اگر_ختم

        // Compute Reverse-Postorder enumeration of vertices
        std::سمتیہ<Dfg_راس*> rpoEnumeration;
        std::unordered_سیٹ<Dfg_راس*> دیکھا;
        dfg.for_Each_راس([&](Dfg_راس& vtx) {
            اگر (!vtx.nInputs()) dfs_Post_ترتیب(دیکھا, rpoEnumeration, vtx);
        });
        dfg.for_Each_راس([&](Dfg_راس& vtx) {  //
            dfs_Post_ترتیب(دیکھا, rpoEnumeration, vtx);
        });
        std::الٹ(rpoEnumeration.begin(), rpoEnumeration.انتہا());
        UASSERT(rpoEnumeration.سائز() == dfg.سائز(), "Inconsistent vertex count");

        // Initialzie VertexState for each vertex
        برائے (سائز_t i = 0; i < rpoEnumeration.سائز(); ++i) {
            راس_State& state = m_vtxp2State[rpoEnumeration[i]];
            state.m_rpo_نمبر = i;
            state.m_isOnWorkList = جھوٹ;
        }

        // Set up the initial conditions:
        // - For vertices not in an SCC, mark all bits as independent
        // - For vertices in an SCC, dispatch to analyse at least once, as some vertices
        //   always assign constants bits, which are always independent (eg Extend/Shift)
        // Enqueue sinks of all SCC vertices that have at least one independent bit
        برائے (Dfg_راس* ثابت vtxp : rpoEnumeration) {
            اگر (m_sccInfo.get(*vtxp)) جاری;
            نقاب(*vtxp).سیٹ_Ones();
        }
        برائے (Dfg_راس* ثابت vtxp : rpoEnumeration) {
            اگر (!m_sccInfo.get(*vtxp)) جاری;
            iterate(vtxp);
            UINFO(9, "Initial independent bits of " << vtxp << " " << vtxp->typeName()
                                                    << " are: " << نقاب(*vtxp).مقصد_String());
            اگر (!نقاب(*vtxp).isZero()) قطار_میں_ڈالو_Sinks(*vtxp);
        }

        // Propagate independent bits until no more changes are made
        جب (!m_work_قطار.خالی_خانے()) {
            // Grab next item
            Dfg_راس* ثابت currp = rpoEnumeration[m_work_قطار.چوٹی()];
            m_work_قطار.نکالو();
            m_vtxp2State.at(currp).m_isOnWorkList = جھوٹ;
            // Should not enqueue vertices that are not in an SCC
            UASSERT_OBJ(m_sccInfo.get(*currp), currp, "Vertex should be in an SCC");

            // Grab current mask of item
            ثابت ثنائی_ہندسہ_نقاب& curr_نقاب = نقاب(*currp);
            // Remember current mask so we can check if it changed
            ثابت ثنائی_ہندسہ_نقاب پچھلا_نقاب = curr_نقاب;

            UINFO(9, "Analyzing independent bits of " << currp << " " << currp->typeName());

            // Dispatch it to update the mask in the visit methods
            iterate(currp);

            // If mask changed, enqueue sinks
            اگر (پچھلا_نقاب != curr_نقاب) {
                UINFO(9, "Independent bits of "  //
                             << currp << " " << currp->typeName() << " changed"  //
                             << "\n    form: " << پچھلا_نقاب.مقصد_String()
                             << "\n      to: " << curr_نقاب.مقصد_String());
                قطار_میں_ڈالو_Sinks(*currp);
                // Check the mask only ever expands (no bit goes 1 -> 0)
                اگر (VL_UNLIKELY(v3Global.opt.debugCheck())) {
                    UASSERT_OBJ(!curr_نقاب.is_Contraction_کا(پچھلا_نقاب), currp,
                                "Mask should only expand");
                }
            }
        }
    }

عوامی:
    // Given a DfgGraph, and a map from vertices to the SCCs they reside in,
    // returns a map from vertices to a bit mask, where a bit in the mask is
    // set if the corresponding bit in that vertex is known to be independent
    // of the values of vertices in the same SCC as the vertex resides in.
    ساکن std::unordered_map<ثابت Dfg_راس*, ثنائی_ہندسہ_نقاب> apply(Dfg_گراف& dfg,
                                                               ثابت SccInfo& sccInfo) {
        واپس std::move(Independent_بٹ{dfg, sccInfo}.m_vtxp2_نقاب);
    }
};

جماعت Fix_اوپر final {
    Dfg_گراف& m_dfg;  // The graph being processed
    SccInfo& m_sccInfo;  // The SccInfo instance
    TraceDriver m_traceDriver{m_dfg, m_sccInfo};
    // The independent bits map
    ثابت std::unordered_map<ثابت Dfg_راس*, ثنائی_ہندسہ_نقاب> m_independent_بٹ
        = Independent_بٹ::apply(m_dfg, m_sccInfo);
    سائز_t m_nImprovements = 0;  // Number of improvements made

    // Returns a bitmask set if that bit of 'vtx' is used (has a sink)
    ساکن ثنائی_ہندسہ_نقاب compute_مستعمل_بٹ(Dfg_راس& vtx) {
        ثنائی_ہندسہ_نقاب نتیجہ{vtx};
        vtx.foreachSink([&نتیجہ](Dfg_راس& sink) {
            // If used via a Sel, mark the selected bits used
            اگر (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                نتیجہ.عدد().op_سیٹ_Range(selp->lsb(), selp->چوڑائی(), '1');
                واپس جھوٹ;
            }
            // Used without a Sel, so all bits are used
            نتیجہ.سیٹ_Ones();
            واپس سچ;
        });
        واپس نتیجہ;
    }

    ساکن std::string debugStr(ثابت Dfg_راس& vtx) {  // LCOV_EXCL_START
        اگر (ثابت DfgArraySel* ثابت aselp = vtx.cast<DfgArraySel>()) {
            ثابت سائز_t i = aselp->bitp()->as<DfgConst>()->مقصد_سائز_T();
            واپس debugStr(*aselp->fromp()) + "[" + std::مقصد_string(i) + "]";
        }
        اگر (ثابت Dfg_راس_Var* ثابت varp = vtx.cast<Dfg_راس_Var>()) {
            واپس varp->vscp()->name();
        }
        vtx.v3fatalSrc("Unhandled node type");
    }  // LCOV_EXCL_STOP

    // Trace drivers of independent bits of 'vtxp' in the range '[hi:lo]'
    // append replacement terms to 'termps'.
    خالی gatherTerms(std::سمتیہ<Dfg_راس*>& termps, Dfg_راس& vtx, ثابت V3_نمبر& indp_بٹ,
                     ثابت uint32_t hi, ثابت uint32_t lo) {
        // Iterate through consecutive dependent/non-dependet ranges within [hi:lo]
        منطقی isIndependent = indp_بٹ.ثنائی_ہندسہ_Is1(lo);
        uint32_t lsb = lo;
        برائے (uint32_t msb = lo; msb <= hi; ++msb) {
            ثابت منطقی انتہا_Range = msb == hi || isIndependent != indp_بٹ.ثنائی_ہندسہ_Is1(msb + 1);
            اگر (!انتہا_Range) جاری;
            ثابت uint32_t چوڑائی = msb - lsb + 1;
            // Compute the term to use for this range
            اگر (isIndependent) {
                // If the range is independent of the cycles, trace its driver
                ++m_nImprovements;
                termps.emplace_عقب(m_traceDriver.apply(vtx, lsb, چوڑائی));
            } ورنہ {
                // If dependent, fall back on using the part of the variable
                DfgSel* ثابت selp = نیا DfgSel{m_dfg, vtx.fileline(), Dfg_ڈیٹا_Type::packed(چوڑائی)};
                // Same component as 'vtxp', as reads 'vtxp' and will replace 'vtxp'
                m_sccInfo.جوڑو(*selp, m_sccInfo.get(vtx));
                // Do not connect selp->fromp yet, need to do afer replacing 'vtxp'
                selp->lsb(lsb);
                termps.emplace_عقب(selp);
            }
            // Next iteration
            isIndependent = !isIndependent;
            lsb = msb + 1;
        }
    }

    خالی fix_اوپر_Packed(Dfg_راس& vtx) {
        UASSERT_OBJ(vtx.isPacked(), &vtx, "Should be a packed type");
        UASSERT_OBJ(!vtx.is<DfgSplicePacked>(), &vtx, "Should not be a splice");

        // Get which bits of 'vtxp' are independent of the SCCs
        ثابت ثنائی_ہندسہ_نقاب& indp_بٹ = m_independent_بٹ.at(&vtx);
        UINFO(9, "Independent bits of '" << debugStr(vtx) << "' are " << indp_بٹ.مقصد_String());
        // Can't do anything if all bits are dependent
        اگر (indp_بٹ.isZero()) واپس;

        // Figure out which bits of 'vtxp' are used
        ثابت ثنائی_ہندسہ_نقاب مستعمل_بٹ = compute_مستعمل_بٹ(vtx);
        UINFO(9, "Used        bits of '" << debugStr(vtx) << "' are " << مستعمل_بٹ.مقصد_String());

        // Nothing to do if no used bits are independen (all used bits are dependent)
        V3_نمبر مستعمل_And_Independent{vtx.fileline(), static_cast<صحیح>(vtx.چوڑائی()), 0};
        مستعمل_And_Independent.opAnd(مستعمل_بٹ.عدد(), indp_بٹ.عدد());
        اگر (مستعمل_And_Independent.isEqZero()) واپس;

        // We are computing the terms to concatenate and replace 'vtxp' with
        std::سمتیہ<Dfg_راس*> termps;

        // Iterate through consecutive used/unused ranges
        File_سطر* ثابت flp = vtx.fileline();
        ثابت uint32_t چوڑائی = vtx.چوڑائی();
        منطقی is_مستعمل = مستعمل_بٹ.عدد().ثنائی_ہندسہ_Is1(0);  // Is current range used
        uint32_t lsb = 0;  // LSB of current range
        برائے (uint32_t msb = 0; msb < چوڑائی; ++msb) {
            ثابت منطقی انتہا_Range = msb == چوڑائی - 1 || is_مستعمل != مستعمل_بٹ.عدد().ثنائی_ہندسہ_Is1(msb + 1);
            اگر (!انتہا_Range) جاری;
            اگر (is_مستعمل) {
                // The range is used, compute the replacement terms
                gatherTerms(termps, vtx, indp_بٹ.عدد(), msb, lsb);
            } ورنہ {
                // The range is not used, just use constant 0 as a placeholder
                DfgConst* ثابت constp = نیا DfgConst{m_dfg, flp, msb - lsb + 1, 0};
                m_sccInfo.جوڑو(*constp, 0);
                termps.emplace_عقب(constp);
            }
            // Next iteration
            is_مستعمل = !is_مستعمل;
            lsb = msb + 1;
        }

        // Concatenate all the terms to create the replacement
        Dfg_راس* replacementp = termps.front();
        ثابت uint64_t vComp = m_sccInfo.get(vtx);
        برائے (سائز_t i = 1; i < termps.سائز(); ++i) {
            Dfg_راس* ثابت termp = termps[i];
            ثابت uint32_t cat_چوڑائی = replacementp->چوڑائی() + termp->چوڑائی();
            DfgConcat* ثابت catp = نیا DfgConcat{m_dfg, flp, Dfg_ڈیٹا_Type::packed(cat_چوڑائی)};
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
        برائے (Dfg_راس* ثابت termp : termps) {
            اگر (DfgSel* ثابت selp = termp->cast<DfgSel>()) {
                اگر (!selp->fromp()) selp->fromp(&vtx);
            }
        }

        // If the vertex still has sinks, then the replacement is still cyclic, and vice versa
        UASSERT_OBJ(!vtx.hasSinks() == !m_sccInfo.get(*replacementp), &vtx,
                    "Replacement vertex SCC inconsistent");

        // If we broke the cycle through this vertex, update the SccInfo
        اگر (!vtx.hasSinks()) m_sccInfo.updateReplacement(vtx, *replacementp);
    }

    خالی مین(Dfg_راس_Var& var) {
        UINFO(9, "FixUp of " << var.vscp()->name());

        اگر (var.is<DfgVarPacked>()) {
            // For Packed variables, fix up as whole
            fix_اوپر_Packed(var);
        } ورنہ اگر (var.is<DfgVarArray>()) {
            // For array variables, fix up element-wise
            ثابت uint64_t component = m_sccInfo.get(var);
            var.foreachSink([&](Dfg_راس& sink) {
                // Ignore if sink is not part of same cycle
                اگر (m_sccInfo.get(sink) != component) واپس جھوٹ;
                // Only handle ArraySels with constant index
                DfgArraySel* ثابت aselp = sink.cast<DfgArraySel>();
                اگر (!aselp) واپس جھوٹ;
                اگر (!aselp->bitp()->is<DfgConst>()) واپس جھوٹ;
                // Fix up the word
                fix_اوپر_Packed(*aselp);
                واپس جھوٹ;
            });
        }

        UINFO(9, "FixUp made " << m_nImprovements << " improvements");
    }

    Fix_اوپر(Dfg_گراف& dfg, SccInfo& sccInfo)
        : m_dfg{dfg}
        , m_sccInfo{sccInfo} {}

عوامی:
    // Compute which bits of vertices are independent of the SCC they reside
    // in, and replace rferences to used independent bits with an equivalent
    // vertex that is not part of the same SCC.
    ساکن سائز_t apply(Dfg_گراف& dfg, SccInfo& sccInfo) {
        اگر (!sccInfo.isCyclic()) واپس 0;

        Fix_اوپر fix_اوپر{dfg, sccInfo};

        // TODO: Compute minimal feedback vertex set and cut only those.
        //       Sadly that is a computationally hard problem.

        // Fix up cyclic variables, stop as soon as the graph becomes acyclic
        برائے (Dfg_راس_Var& vtx : dfg.varVertices()) {
            اگر (!sccInfo.get(vtx)) جاری;
            fix_اوپر.مین(vtx);
            اگر (!sccInfo.isCyclic()) توڑو;
        }
        // Return the number of improvements made
        واپس fix_اوپر.m_nImprovements;
    }
};

خالی break_ادوار(Dfg_گراف& dfg, V3_Dfg_Break_ادوار_Context& ctx) {
    // Shorthand for dumping graph at given dump level
    ثابت خودکار dump = [&](صحیح level, ثابت Dfg_گراف& dfg, ثابت std::string& name) {
        اگر (dumpDfgLevel() >= level) dfg.dumpDotFilePrefixed("breakCycles-" + name);
    };

    // Show input for debugging
    dump(7, dfg, "input");

    // How many improvements have we made
    سائز_t nImprovements = 0;
    سائز_t پچھلا_NImprovements;

    // Iterate while an improvement can be made and the graph is still cyclic
    کرو {
        // Compute SCCs
        SccInfo sccInfo{dfg};

        // Fix up independent ranges in vertices
        UINFO(9, "New iteration after " << nImprovements << " improvements");
        پچھلا_NImprovements = nImprovements;
        ثابت سائز_t nFixed = Fix_اوپر::apply(dfg, sccInfo);
        اگر (nFixed) {
            nImprovements += nFixed;
            ctx.m_nImprovements += nFixed;
            dump(9, dfg, "FixUp");
        }

        // Validate SccInfo if in debug mode
        اگر (v3Global.opt.debugCheck()) sccInfo.validate();

        // Congrats if it has become acyclic
        اگر (!sccInfo.isCyclic()) {
            UINFO(7, "Graph became acyclic after " << nImprovements << " improvements");
            dump(7, dfg, "result-acyclic");
            ++ctx.m_nFixed;
            واپس;
        }
    } جب (nImprovements != پچھلا_NImprovements);

    // Debug dump
    اگر (dumpDfgLevel() >= 9) {
        ثابت SccInfo sccInfo{dfg};
        dfg.dumpDotFilePrefixed("breakCycles-remaining", [&](ثابت Dfg_راس& vtx) {
            واپس sccInfo.get(vtx);  //
        });
    }

    // Accounting
    اگر (nImprovements) {
        UINFO(7, "Graph was improved " << nImprovements << " times");
        dump(7, dfg, "result-improved");
        ++ctx.m_nImproved;
    } ورنہ {
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
    برائے (Dfg_راس_Var& vtx : dfg.varVertices()) {
        اگر (!vtx.tmpForp()) varps.ڈالو_عقب(&vtx);
    }
    std::stable_تقسیم(varps.begin(), varps.انتہا(), [](ثابت Dfg_راس_Var* varp) {
        واپس varp->hasExtRefs() || varp->isVolatile();
    });

    // Insert DfgPrev vertices until the graph becomes acyclic
    SccInfo sccInfo{dfg};
    برائے (Dfg_راس_Var* ثابت varp : varps) {
        // Stop as soon as it becomes acyclic
        اگر (!sccInfo.isCyclic()) توڑو;

        // Ignore if variable is not part of a cycle
        اگر (!sccInfo.get(*varp)) جاری;

        // Insert a DfgPrev vertex for the variable
        Dfg_پچھلا* ثابت prevp = نیا Dfg_پچھلا{dfg, varp->vscp()};
        sccInfo.جوڑو(*prevp, 0);
        varp->replaceWith(prevp);
        ++ctx.m_n_پچھلا_Inserted;

        // Update SCC info
        sccInfo.updateReplacement(*varp, *prevp);
    }
    // Validate SccInfo if in debug mode
    اگر (v3Global.opt.debugCheck()) sccInfo.validate();
    UASSERT(!sccInfo.isCyclic(), "Graph should become acyclic");

    dump(7, dfg, "result-withprev");
}

}  //namespace V3DfgBreakCycles
خالی V3DfgPasses::break_ادوار(Dfg_گراف& dfg, V3DfgContext& ctx) {
    V3_Dfg_Break_ادوار::break_ادوار(dfg, ctx.m_break_ادوار_Context);
    اگر (v3Global.opt.debugCheck()) V3DfgPasses::typeCheck(dfg);
    V3DfgPasses::remove_غیر_مستعمل(dfg);
}
