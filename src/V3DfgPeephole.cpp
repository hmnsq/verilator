// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Peephole optimizations over DfgGraph
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
//
// A pattern-matching based optimizer for DfgGraph. This is in some aspects
// similar to V3Const, but more powerful in that it does not care about
// ordering combinational statement. This is also less broadly applicable
// than V3Const, as it does not apply to procedural statements with
// sequential execution semantics.
//
// Each pattern can look at a certain number of source vertices to see
// if a simplified form can be introduced. Some patterns also look at the
// immediate sinks of some vertices. Ideally the algorithm should run to
// fixed point (until nothing else changes). To do this efficiently, two
// lists of vertices are maintained:
// - the 'work' list contains vertices to be considered on the current
//   iteration
// - the 'iter' list contains vertices whose whole neighborhood could be
//   considered on the next iteration
// The 'work' list ensures simple cascading pattern applications are
// handled in a single pass. The 'iter' list ensures the algorithm runs
// to fixed point if no pattern looks deeper across the graph than the
// neighborhood considered on the next iteration.
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Ast.h"
#ضمن "V3Dfg.h"
#ضمن "V3DfgCache.h"
#ضمن "V3DfgDataType.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3DfgPeepholePatterns.h"

#ضمن <algorithm>
#ضمن <cctype>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
نطاق {

template <typename T_Reduction>
بنية ReductionToBitwiseImpl {};
template <> بنية ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> بنية ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> بنية ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
بنية BitwiseToReductionImpl {};
template <> بنية BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> بنية BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> بنية BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
بنية IsAssociative : عام std::false_type {};
template <> بنية IsAssociative<DfgAnd> : عام std::true_type {};
template <> بنية IsAssociative<DfgOr>  : عام std::true_type {};
template <> بنية IsAssociative<DfgXor> : عام std::true_type {};
template <> بنية IsAssociative<DfgAdd> : عام std::true_type {};
template <> بنية IsAssociative<DfgConcat> : عام std::true_type {};
template <> بنية IsAssociative<DfgMul> : عام std::true_type {};
template <> بنية IsAssociative<DfgMulS> : عام std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
بنية IsCommutative : عام std::false_type {};
template <> بنية IsCommutative<DfgAnd> : عام std::true_type {};
template <> بنية IsCommutative<DfgOr>  : عام std::true_type {};
template <> بنية IsCommutative<DfgXor> : عام std::true_type {};
template <> بنية IsCommutative<DfgAdd> : عام std::true_type {};
template <> بنية IsCommutative<DfgEq> : عام std::true_type {};
template <> بنية IsCommutative<DfgMul> : عام std::true_type {};
template <> بنية IsCommutative<DfgMulS> : عام std::true_type {};
template <> بنية IsCommutative<DfgNeq> : عام std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
بنية IsIdempotent : عام std::false_type {};
template <> بنية IsIdempotent<DfgAnd> : عام std::true_type {};
template <> بنية IsIdempotent<DfgOr>  : عام std::true_type {};

// Binary result dtype
template<typename Vertex>
ثابت DfgDataType& resultDType(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp);
template <> ثابت DfgDataType& resultDType<DfgAdd>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgAnd>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgConcat>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> ثابت DfgDataType& resultDType<DfgDiv>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgDivS>       (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgEq>         (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgGt>         (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgGtS>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgGte>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgGteS>       (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLogAnd>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLogEq>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLogIf>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLogOr>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLt>         (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLtS>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLte>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLteS>       (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgModDiv>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgModDivS>    (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgMul>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgMulS>       (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgNeq>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgOr>         (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgPow>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgPowSS>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgPowSU>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgPowUS>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgShiftL>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgShiftR>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgShiftRS>    (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgSub>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgXor>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { أرجع lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> فراغ foldOp(V3_رقم& out, ثابت V3_رقم& src);
template <> فراغ foldOp<Dfg_عدد_Ones>  (V3_رقم& out, ثابت V3_رقم& src) { out.op_عدد_Ones(src); }
template <> فراغ foldOp<DfgExtend>     (V3_رقم& out, ثابت V3_رقم& src) { out.opAssign(src); }
template <> فراغ foldOp<DfgExtendS>    (V3_رقم& out, ثابت V3_رقم& src) { out.opExtendS(src, src.width()); }
template <> فراغ foldOp<DfgLogNot>     (V3_رقم& out, ثابت V3_رقم& src) { out.opLogNot(src); }
template <> فراغ foldOp<DfgNegate>     (V3_رقم& out, ثابت V3_رقم& src) { out.opNegate(src); }
template <> فراغ foldOp<DfgNot>        (V3_رقم& out, ثابت V3_رقم& src) { out.opNot(src); }
template <> فراغ foldOp<DfgOneHot>     (V3_رقم& out, ثابت V3_رقم& src) { out.opOneHot(src); }
template <> فراغ foldOp<DfgOneHot0>    (V3_رقم& out, ثابت V3_رقم& src) { out.opOneHot0(src); }
template <> فراغ foldOp<DfgRedAnd>     (V3_رقم& out, ثابت V3_رقم& src) { out.opRedAnd(src); }
template <> فراغ foldOp<DfgRedOr>      (V3_رقم& out, ثابت V3_رقم& src) { out.opRedOr(src); }
template <> فراغ foldOp<DfgRedXor>     (V3_رقم& out, ثابت V3_رقم& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> فراغ foldOp(V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
template <> فراغ foldOp<DfgAdd>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opAdd(lhs, rhs); }
template <> فراغ foldOp<DfgAnd>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opAnd(lhs, rhs); }
template <> فراغ foldOp<DfgConcat>     (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opConcat(lhs, rhs); }
template <> فراغ foldOp<DfgDiv>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opDiv(lhs, rhs); }
template <> فراغ foldOp<DfgDivS>       (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opDivS(lhs, rhs); }
template <> فراغ foldOp<DfgEq>         (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opEq(lhs, rhs); }
template <> فراغ foldOp<DfgGt>         (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opGt(lhs, rhs); }
template <> فراغ foldOp<DfgGtS>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opGtS(lhs, rhs); }
template <> فراغ foldOp<DfgGte>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opGte(lhs, rhs); }
template <> فراغ foldOp<DfgGteS>       (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opGteS(lhs, rhs); }
template <> فراغ foldOp<DfgLogAnd>     (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opLogAnd(lhs, rhs); }
template <> فراغ foldOp<DfgLogEq>      (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opLogEq(lhs, rhs); }
template <> فراغ foldOp<DfgLogIf>      (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opLogIf(lhs, rhs); }
template <> فراغ foldOp<DfgLogOr>      (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opLogOr(lhs, rhs); }
template <> فراغ foldOp<DfgLt>         (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opLt(lhs, rhs); }
template <> فراغ foldOp<DfgLtS>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opLtS(lhs, rhs); }
template <> فراغ foldOp<DfgLte>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opLte(lhs, rhs); }
template <> فراغ foldOp<DfgLteS>       (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opLteS(lhs, rhs); }
template <> فراغ foldOp<DfgModDiv>     (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opModDiv(lhs, rhs); }
template <> فراغ foldOp<DfgModDivS>    (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opModDivS(lhs, rhs); }
template <> فراغ foldOp<DfgMul>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opMul(lhs, rhs); }
template <> فراغ foldOp<DfgMulS>       (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opMulS(lhs, rhs); }
template <> فراغ foldOp<DfgNeq>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opNeq(lhs, rhs); }
template <> فراغ foldOp<DfgOr>         (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opOr(lhs, rhs); }
template <> فراغ foldOp<DfgPow>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opPow(lhs, rhs); }
template <> فراغ foldOp<DfgPowSS>      (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opPowSS(lhs, rhs); }
template <> فراغ foldOp<DfgPowSU>      (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opPowSU(lhs, rhs); }
template <> فراغ foldOp<DfgPowUS>      (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opPowUS(lhs, rhs); }
template <> فراغ foldOp<DfgShiftL>     (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opShiftL(lhs, rhs); }
template <> فراغ foldOp<DfgShiftR>     (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opShiftR(lhs, rhs); }
template <> فراغ foldOp<DfgShiftRS>    (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> فراغ foldOp<DfgSub>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opSub(lhs, rhs); }
template <> فراغ foldOp<DfgXor>        (V3_رقم& out, ثابت V3_رقم& lhs, ثابت V3_رقم& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

صنف V3DfgPeephole final : عام DfgVisitor {
    // TYPES
    بنية VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        منطقي m_isCachedVertex = خطأ;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ثابت DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ثابت m_tmpScopep = v3Global.rootp()->قمة_Scopep()->scopep();

    // STATIC STATE
    ساكن V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) إذا (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    منطقي checkApplying(VDfgPeepholePattern id) {
        إذا (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) أرجع خطأ;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_عدد[id];
        أرجع true;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* ثابت equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        أرجع equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    فراغ invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = خطأ;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_سطر* flp, ثابت DfgDataType& dtype, Operands... operands) {
        Vertex* ثابت vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = true;
        أرجع vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(ثابت DfgDataType& dtype, Operands... operands) {
        Vertex* ثابت vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        أرجع vtxp;
    }

    فراغ incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    فراغ addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        إذا (vInfo.m_workListIndex) أرجع;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.ادفع_back(vtxp);
    }

    فراغ removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    فراغ addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        إذا (vInfo.m_iterListIndex) أرجع;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.ادفع_back(vtxp);
    }

    فراغ removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    فراغ deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ثابت DfgVertexVar* ثابت varp = vtxp->cast<DfgVertexVar>();
        UASSERT_OBJ(!varp || !varp->hasPrev(), vtxp, "Deleting variable consumed via DfgPrev");

        // Invalidate cache entry
        invalidateVertex(vtxp);

        // It might be in the iter list, remove it
        removeFromIterList(vtxp);

        // Gather source vertices - they might be duplicates, make unique using generation number
        incrementGeneration();
        std::vector<DfgVertex*> srcps;
        srcps.reserve(vtxp->nInputs());
        vtxp->foreachSource([&](DfgVertex& src) {
            // If it's a variable, add to work list to see if it became redundant
            إذا (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                أرجع خطأ;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            إذا (vInfo.m_generation != m_currentGeneration) srcps.ادفع_back(&src);
            vInfo.m_generation = m_currentGeneration;
            أرجع خطأ;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        إذا (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ادفع_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } وإلا {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ثابت auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  أرجع srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        لكل (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        لكل (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    فراغ replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ثابت auto debugCallback = [&]() -> فراغ {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            ثابت auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ثابت DfgVertex& v) {  //
                أرجع cone->عدد(&v);
            });
        };
        إذا (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) أرجع;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            أرجع خطأ;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            أرجع خطأ;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            أرجع خطأ;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            أرجع خطأ;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_سطر* flp, uint32_t val) { أرجع جديد DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_سطر* flp, uint32_t width) {
        أرجع جديد DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_سطر* flp, ثابت DfgDataType& dtype) {
        أرجع makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_سطر* flp, uint32_t width) {
        DfgConst* ثابت resp = makeZero(flp, width);
        resp->رقم().setAllBits1();
        أرجع resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_سطر* flp, ثابت DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* ثابت vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        إذا (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        أرجع vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(ثابت DfgVertex* examplep, Operands... operands) {
        أرجع make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        إذا (vtxp->dtype() == m_bitDType) أرجع bitp;
        أرجع make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ساكن منطقي isSame(ثابت DfgVertex* ap, ثابت DfgVertex* bp) {
        إذا (ap == bp) أرجع true;
        إذا (ap->dtype() != bp->dtype()) أرجع خطأ;
        ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>();
        إذا (!aConstp) أرجع خطأ;
        ثابت DfgConst* ثابت bConstp = bp->cast<DfgConst>();
        إذا (!bConstp) أرجع خطأ;
        أرجع aConstp->رقم().isCaseEq(bConstp->رقم());
    }

    ساكن منطقي isZero(ثابت DfgVertex* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->isZero();
        أرجع خطأ;
    }

    ساكن منطقي isOnes(ثابت DfgVertex* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->isOnes();
        أرجع خطأ;
    }

    ساكن منطقي isEqOne(ثابت DfgVertex* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->رقم().isEqOne();
        أرجع خطأ;
    }

    ساكن منطقي areAdjacent(uint32_t& lsb, ثابت DfgSel* lSelp, ثابت DfgSel* rSelp) {
        إذا (!isSame(lSelp->srcp(), rSelp->srcp())) أرجع خطأ;
        إذا (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            أرجع true;
        }
        إذا (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            أرجع true;
        }
        أرجع خطأ;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي foldUnary(Vertex* ثابت vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::قيمة, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::قيمة, "Must invoke on final class");
        إذا (DfgConst* ثابت srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ثابت resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->رقم(), srcp->رقم());
                replace(resultp);
                أرجع true;
            }
        }
        أرجع خطأ;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي ثنائي(Vertex* ثابت vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ثنائي, Vertex>::قيمة, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::قيمة, "Must invoke on final class");

        File_سطر* ثابت flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ثابت lConstp = lhsp->cast<DfgConst>();
        DfgConst* ثابت rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* ثابت lSamep = lhsp->cast<Vertex>();
        Vertex* ثابت rSamep = rhsp->cast<Vertex>();

        // Constant folding
        إذا (lConstp && rConstp) {
            APPLYING(FOLD_ثنائي) {
                DfgConst* ثابت resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->رقم(), lConstp->رقم(), rConstp->رقم());
                replace(resultp);
                أرجع true;
            }
        }

        // Idempotent
        إذا VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::قيمة) {
            إذا (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ثنائي) {
                    replace(lhsp);
                    أرجع true;
                }
            }
        }

        // Associative
        إذا VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::قيمة) {
            // Also commutative and idempotent
            إذا VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::قيمة && IsIdempotent<Vertex>::قيمة) {
                إذا (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ثنائي_LHS) {
                        replace(lSamep);
                        أرجع true;
                    }
                }
                إذا (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ثنائي_RHS) {
                        replace(rSamep);
                        أرجع true;
                    }
                }
            }

            إذا (lConstp && rSamep) {
                إذا (DfgConst* ثابت rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ثنائي_LHS_OF_RHS) {
                        DfgConst* ثابت cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->رقم(), lConstp->رقم(), rlConstp->رقم());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        أرجع true;
                    }
                }
            }

            إذا (lSamep && rConstp) {
                إذا (DfgConst* ثابت lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ثنائي_RHS_OF_LHS) {
                        DfgConst* ثابت cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->رقم(), lrConstp->رقم(), rConstp->رقم());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        أرجع true;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            إذا (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* ثابت ap = lSamep->lhsp();
                DfgVertex* ثابت bp = lSamep->rhsp();
                DfgVertex* ثابت cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                إذا (ap != cp && bp != cp) {
                    APPLYING(يمين_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ثابت DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* ثابت bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        أرجع true;
                    }
                }
            }

            إذا (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* ثابت rlVtxp = rSamep->lhsp();
                DfgVertex* ثابت rrVtxp = rSamep->rhsp();

                إذا VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::قيمة) {
                    إذا (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            أرجع true;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                إذا (Vertex* ثابت existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    إذا (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ثنائي_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            أرجع true;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                إذا VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::قيمة) {
                    إذا (Vertex* ثابت existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        إذا (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ثنائي_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                أرجع true;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        إذا VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::قيمة) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            إذا (lConstp) {
                إذا (DfgCond* ثابت rCondp = rhsp->cast<DfgCond>()) {
                    إذا (!rCondp->hasMultipleSinks()) {
                        APPLYING(ادفع_COMMUTATIVE_ثنائي_THROUGH_COND) {
                            DfgVertex* ثابت tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* ثابت ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            أرجع true;
                        }
                    }
                }
                أرجع خطأ;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ثابت منطقي swapSides = [&]() {
            // Don't swap if not commutative, duh
            إذا VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::قيمة) أرجع خطأ;
            // Const go on left
            إذا (lConstp) أرجع خطأ;
            إذا (rConstp) أرجع true;
            // Not go on left
            إذا (lhsp->is<DfgNot>()) أرجع خطأ;
            إذا (rhsp->is<DfgNot>()) أرجع true;
            // Same go on right
            إذا (rSamep) أرجع خطأ;
            إذا (lSamep) أرجع true;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ثابت VertexInfo& lInfo = m_vInfo[lhsp];
            ثابت VertexInfo& rInfo = m_vInfo[rhsp];
            أرجع lInfo.m_id > rInfo.m_id;
        }();
        إذا (swapSides) {
            APPLYING(SWAP_SIDES_IN_ثنائي) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                أرجع true;
            }
        }

        أرجع خطأ;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي distributive_And_Associative_ثنائي(Vertex* vtxp) {
        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        إذا (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            إذا (Other* ثابت lp = lhsp->cast<Other>()) {
                إذا (Other* ثابت rp = rhsp->cast<Other>()) {
                    DfgVertex* ثابت llp = lp->lhsp();
                    DfgVertex* ثابت lrp = lp->rhsp();
                    DfgVertex* ثابت rlp = rp->lhsp();
                    DfgVertex* ثابت rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    إذا (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } وإلا إذا (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } وإلا إذا (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } وإلا إذا (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    إذا (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_ثنائي) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            أرجع true;
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي
    try_ادفع_Bitwise_Op_Through_Concat(Vertex* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_سطر* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ثابت catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ثابت catRConstp = concatp->rhsp()->cast<DfgConst>();
        إذا (catLConstp || catRConstp) {
            APPLYING(ادفع_BITWISE_OP_THROUGH_CONCAT) {
                ثابت uint32_t width = concatp->width();
                ثابت DfgDataType& lDtype = concatp->lhsp()->dtype();
                ثابت DfgDataType& rDtype = concatp->rhsp()->dtype();
                ثابت uint32_t lWidth = lDtype.size();
                ثابت uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* ثابت newLhsp = [&]() -> DfgVertex* {
                    DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), lWidth);
                    إذا (catLConstp) {
                        V3_رقم رقم{constp->fileline(), static_cast<صحيح>(lWidth), 0u};
                        رقم.opSel(constp->رقم(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->رقم(), رقم, catLConstp->رقم());
                        أرجع newLhsConstp;
                    }
                    newLhsConstp->رقم().opSel(constp->رقم(), width - 1, rWidth);
                    أرجع make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* ثابت newRhsp = [&]() -> DfgVertex* {
                    DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), rWidth);
                    إذا (catRConstp) {
                        V3_رقم رقم{constp->fileline(), static_cast<صحيح>(rWidth), 0u};
                        رقم.opSel(constp->رقم(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->رقم(), رقم, catRConstp->رقم());
                        أرجع newRhsConstp;
                    }
                    newRhsConstp->رقم().opSel(constp->رقم(), rWidth - 1, 0);
                    أرجع make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                أرجع true;
            }
        }
        أرجع خطأ;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي
    try_ادفع_Compare_Op_Through_Concat(Vertex* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_سطر* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        إذا (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ادفع_COMPARE_OP_THROUGH_CONCAT) {
                ثابت uint32_t width = concatp->width();
                ثابت uint32_t lWidth = concatp->lhsp()->width();
                ثابت uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->رقم().opSel(constp->رقم(), width - 1, rWidth);
                Vertex* ثابت newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->رقم().opSel(constp->رقم(), rWidth - 1, 0);
                Vertex* ثابت newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                إذا VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::قيمة) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } وإلا إذا VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::قيمة) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } وإلا {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                أرجع true;
            }
        }
        أرجع خطأ;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT منطقي try_ادفع_Bitwise_Op_Through_Reductions(Bitwise* ثابت vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        إذا (Reduction* ثابت lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            إذا (Reduction* ثابت rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* ثابت lSrcp = lRedp->srcp();
                DfgVertex* ثابت rSrcp = rRedp->srcp();
                إذا (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(ادفع_BITWISE_THROUGH_REDUCTION) {
                        File_سطر* ثابت flp = vtxp->fileline();
                        Bitwise* ثابت bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        أرجع true;
                    }
                }
            }
        }

        أرجع خطأ;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    ساكن std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        إذا (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            أرجع {nullptr, nullptr};
        }
        إذا (vtxp->hasMultipleSinks()) أرجع {nullptr, nullptr};
        Dfg_Vertex_ثنائي* ثابت bwp = vtxp->as<Dfg_Vertex_ثنائي>();
        DfgSel* ثابت lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ثابت rSelp = bwp->rhsp()->cast<DfgSel>();
        إذا (!lSelp || !rSelp) أرجع {nullptr, nullptr};
        DfgSel* ثابت nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* ثابت wSelp = nSelp == lSelp ? rSelp : lSelp;
        ثابت uint32_t nWidth = nSelp->fromp()->width();
        إذا (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) أرجع {nullptr, nullptr};
        إذا (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            أرجع {nullptr, nullptr};
        }
        أرجع {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT منطقي try_ادفع_Bitwise_Op_Throug_Sel(Bitwise* ثابت vtxp) {
        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            إذا (DfgSel* ثابت selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } وإلا إذا (Bitwise* ثابت bitwisep = rhsp->cast<Bitwise>()) {
                إذا (DfgSel* ثابت rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } وإلا إذا (DfgSel* ثابت rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            إذا (rSelp) {
                DfgVertex* ثابت lFromp = lSelp->fromp();
                DfgVertex* ثابت rFromp = rSelp->fromp();
                إذا (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ادفع_BITWISE_THROUGH_SEL) {
                        Bitwise* ثابت bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        أرجع true;
                    }
                }
            }
        }

        // Push a different bitwise operation through unaligned selects (see
        // 'pushableUnalignedSels'), but only in adjacent terms of a chain of 'vtxp' operations
        // that then select from the same vertex, so they reduce. One of the terms might already
        // have been pushed. E.g. with 'a' 8 bits and 'b' 64 bits: 'a[3] & b[20] | a[4] & b[21]'
        // -> '(a & b[24:17])[3] | (a & b[24:17])[4]' -> '|(a & b[24:17])[4:3]'. Applied to a
        // lone term, the push only widens the operation and creates a temporary.
        {
            // The adjacent terms are the LHS, and the RHS, or in a right-leaning chain the LHS
            // of the RHS
            Bitwise* ثابت rChainp = rhsp->cast<Bitwise>();
            إذا (rChainp && rChainp->hasMultipleSinks()) أرجع خطأ;
            DfgVertex* ثابت rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ثابت auto lSels = pushableUnalignedSels(lhsp);
            ثابت auto rSels = pushableUnalignedSels(rTermp);
            إذا (!lSels.first && !rSels.first) أرجع خطأ;

            ثابت DfgVertex* ثابت opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            إذا (opp->is<Bitwise>()) أرجع خطأ;
            ثابت auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* ثابت nFromp = sels.first->fromp();
            DfgVertex* ثابت wFromp = sels.second->fromp();
            ثابت uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            ثابت auto matches = [&](ثابت DfgVertex* termp, ثابت auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                إذا (tSels.first) {
                    أرجع termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                إذا (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                إذا (ثابت DfgSel* ثابت selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                إذا (termp->type() != opp->type()) أرجع خطأ;
                // Operands might have been swapped since
                ثابت Dfg_Vertex_ثنائي* ثابت bwp = termp->as<Dfg_Vertex_ثنائي>();
                ثابت منطقي nIsLhs = bwp->lhsp() == nFromp;
                إذا (!nIsLhs && bwp->rhsp() != nFromp) أرجع خطأ;
                ثابت DfgSel* ثابت slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                أرجع slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            إذا (!matches(lhsp, lSels) || !matches(rTermp, rSels)) أرجع خطأ;

            APPLYING(ادفع_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_سطر* ثابت flp = vtxp->fileline();
                ثابت DfgDataType& dtype = nFromp->dtype();
                DfgSel* ثابت slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                إذا (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } وإلا إذا (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } وإلا {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ثابت auto ادفع = [&](DfgVertex* termp, ثابت DfgSel* nSelp) -> DfgVertex* {
                    أرجع nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* ثابت newLhsp = ادفع(lhsp, lSels.first);
                DfgVertex* ثابت newRTermp = ادفع(rTermp, rSels.first);
                إذا (rChainp) {
                    DfgVertex* ثابت newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } وإلا {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                أرجع true;
            }
        }

        أرجع خطأ;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ساكن منطقي isConcatFoldable(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        إذا (isSame(lhsp, rhsp)) أرجع true;
        إذا (ثابت DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            إذا (isSame(lRepp->srcp(), rhsp)) أرجع true;
        }
        إذا (ثابت DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
            إذا (isSame(lhsp, rRepp->srcp())) أرجع true;
        }
        // Concatenation of adjoining selects is a select
        إذا (ثابت DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            إذا (ثابت DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                أرجع isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        إذا (ثابت DfgNot* ثابت lNotp = lhsp->cast<DfgNot>()) {
            إذا (ثابت DfgNot* ثابت rNotp = rhsp->cast<DfgNot>()) {
                أرجع !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        أرجع خطأ;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ساكن منطقي isSameBitwise(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) {
        إذا (lhsp->type() != rhsp->type()) أرجع خطأ;
        أرجع lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ساكن منطقي isConcatSimplifiable(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) {
        إذا (isConcatFoldable(lhsp, rhsp)) أرجع true;
        إذا (!isSameBitwise(lhsp, rhsp)) أرجع خطأ;
        إذا (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) أرجع خطأ;
        ثابت Dfg_Vertex_ثنائي* ثابت lp = lhsp->as<Dfg_Vertex_ثنائي>();
        ثابت Dfg_Vertex_ثنائي* ثابت rp = rhsp->as<Dfg_Vertex_ثنائي>();
        إذا (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            أرجع true;
        إذا (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            أرجع true;
        أرجع خطأ;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ساكن منطقي isConcatPushableThroughBitwise(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp,
                                               منطقي& swap) {
        إذا (!isSameBitwise(lhsp, rhsp)) أرجع خطأ;
        إذا (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) أرجع خطأ;
        ثابت Dfg_Vertex_ثنائي* ثابت lp = lhsp->as<Dfg_Vertex_ثنائي>();
        ثابت Dfg_Vertex_ثنائي* ثابت rp = rhsp->as<Dfg_Vertex_ثنائي>();
        swap = خطأ;
        إذا (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) أرجع true;
        إذا (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) أرجع true;
        swap = true;
        إذا (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) أرجع true;
        إذا (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) أرجع true;
        أرجع خطأ;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_سطر* flp, ثابت DfgVertex* lhsp,
                                              ثابت DfgVertex* rhsp, منطقي swap) {
        ثابت Dfg_Vertex_ثنائي* ثابت lp = lhsp->as<Dfg_Vertex_ثنائي>();
        ثابت Dfg_Vertex_ثنائي* ثابت rp = rhsp->as<Dfg_Vertex_ثنائي>();
        ثابت DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* ثابت cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* ثابت dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* ثابت newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ثابت newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        إذا (lp->is<DfgAnd>()) أرجع make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        إذا (lp->is<DfgOr>()) أرجع make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        أرجع make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT منطقي tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        إذا (DfgSel* ثابت lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            إذا (DfgSel* ثابت selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } وإلا إذا (Bitwise* ثابت rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } وإلا إذا (Reduction* ثابت rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            إذا (rSelp) {
                uint32_t lsb = 0;
                إذا (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        ثابت DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* ثابت newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        أرجع true;
                    }
                }
            }
        }

        إذا (Reduction* ثابت lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            إذا (!rRedp) {
                إذا (Bitwise* ثابت rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            إذا (rRedp) {
                إذا (DfgSel* ثابت lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    إذا (DfgSel* ثابت rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        إذا (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                ثابت DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                أرجع true;
                            }
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT منطقي optimizeReduction(Reduction* ثابت vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        إذا (foldUnary(vtxp)) أرجع true;

        DfgVertex* ثابت srcp = vtxp->srcp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Reduction of 1-bit value
        إذا (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                أرجع true;
            }
        }

        إذا (DfgCond* ثابت condp = srcp->cast<DfgCond>()) {
            إذا (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ادفع_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ثابت newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ثابت newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    أرجع true;
                }
            }
        }

        إذا (DfgConcat* ثابت concatp = srcp->cast<DfgConcat>()) {
            إذا (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(ادفع_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ثابت lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* ثابت rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    أرجع true;
                }
            }
        }

        إذا (Bitwise* ثابت bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            إذا (!bitwisep->hasMultipleSinks()) {
                إذا (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(ادفع_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* ثابت newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* ثابت newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        أرجع true;
                    }
                }

                إذا (DfgSel* ثابت lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    إذا (DfgSel* ثابت rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        إذا (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ادفع_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                ثابت DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                أرجع true;
                            }
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT منطقي optimizeShiftRHS(Shift* ثابت vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ثنائي, Shift>::قيمة, "Must invoke on binary");
        static_assert(std::is_final<Shift>::قيمة, "Must invoke on final class");
        إذا (ثابت DfgConcat* ثابت concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            إذا (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    أرجع true;
                }
            }
        }
        أرجع خطأ;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_عدد_Ones_Term(DfgVertex* vtxp) {
        إذا (DfgConcat* ثابت oCatp = vtxp->cast<DfgConcat>()) {
            إذا (isZero(oCatp->lhsp())) {
                إذا (Dfg_عدد_Ones* ثابت عدد_Onesp = oCatp->rhsp()->cast<Dfg_عدد_Ones>()) {
                    // Zero extended count ones
                    إذا (DfgSel* ثابت selp = عدد_Onesp->srcp()->cast<DfgSel>()) {
                        أرجع {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } وإلا إذا (DfgSel* ثابت selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    إذا (selp->dtype() == m_bitDType) {  //
                        أرجع {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            أرجع {nullptr, 0, 0};
        }
        إذا (Dfg_عدد_Ones* ثابت عدد_Onesp = vtxp->cast<Dfg_عدد_Ones>()) {
            // Simple count ones
            إذا (DfgSel* ثابت selp = عدد_Onesp->srcp()->cast<DfgSel>()) {
                أرجع {selp->fromp(), selp->lsb(), selp->width()};
            }
            أرجع {nullptr, 0, 0};
        }
        إذا (DfgSel* ثابت oSelp = vtxp->cast<DfgSel>()) {
            إذا (oSelp->lsb() == 0) {
                // Truncated count ones
                إذا (Dfg_عدد_Ones* ثابت عدد_Onesp = oSelp->fromp()->cast<Dfg_عدد_Ones>()) {
                    // Zero extended count ones
                    إذا (DfgSel* ثابت selp = عدد_Onesp->srcp()->cast<DfgSel>()) {
                        أرجع {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            إذا (oSelp->dtype() == m_bitDType) {  //
                أرجع {oSelp->fromp(), oSelp->lsb(), 1};
            }
            أرجع {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        إذا (DfgShiftR* ثابت shiftrp = vtxp->cast<DfgShiftR>()) {
            إذا (DfgConst* ثابت rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                إذا (rConstp->toU32() == shiftrp->width() - 1) {
                    أرجع {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            أرجع {nullptr, 0, 0};
        }
        // Not applicable
        أرجع {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              ثابت uint32_t width) {
        طالما (true) {
            ثابت uint32_t msb = lsb + width - 1;

            // Sel from Concat
            إذا (DfgConcat* ثابت concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* ثابت lhsp = concatp->lhsp();
                DfgVertex* ثابت rhsp = concatp->rhsp();

                إذا (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        استمر;
                    }
                } وإلا إذا (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        استمر;
                    }
                }
            }

            إذا (DfgRep* ثابت repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ثابت uint32_t srcWidth = repp->srcp()->width();
                إذا (width <= srcWidth) {
                    ثابت uint32_t newLsb = lsb % srcWidth;
                    ثابت uint32_t newMsb = newLsb + width - 1;
                    إذا (newMsb < srcWidth) {
                        APPLYING(ادفع_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            استمر;
                        }
                    }
                }
            }

            // Sel from Sel
            إذا (DfgSel* ثابت selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    استمر;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            إذا (DfgVarPacked* ثابت varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ثابت auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* ثابت driverp = pair.first;
                ثابت uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                إذا (driverp) {
                    إذا (driverp == varp->defaultp()) {
                        APPLYING(ادفع_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            استمر;
                        }
                    } وإلا {
                        APPLYING(ادفع_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            استمر;
                        }
                    }
                }
            }

            // No patterns matched, stop
            اكسر;
        }
        أرجع {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        إذا (ap == bp) أرجع {ap, ap->width()};

        // If both constants, check LSBs
        إذا (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            إذا (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت V3_رقم& a_رقم = aConstp->رقم();
                ثابت V3_رقم& b_رقم = bConstp->رقم();
                // Max match is the shorter constant
                ثابت uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                لكل (; matchWidth < maxMatch; ++matchWidth) {
                    إذا (a_رقم.bitIs0(matchWidth) != b_رقم.bitIs0(matchWidth)) اكسر;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                أرجع {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        إذا (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) أرجع commonLSBs(catp->rhsp(), bp);
        إذا (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) أرجع commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        إذا (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            إذا (selp->lsb() == 0) {
                DfgVertex* ثابت fromp = selp->fromp();
                ثابت std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                أرجع {common.first, std::min(common.second, selp->width())};
            }
        }
        إذا (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            إذا (selp->lsb() == 0) {
                DfgVertex* ثابت fromp = selp->fromp();
                ثابت std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                أرجع {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        أرجع {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        إذا (ap == bp) أرجع {ap, ap->width()};

        // If both constants, check MSBs
        إذا (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            إذا (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت uint32_t aMsb = aConstp->width() - 1;
                ثابت uint32_t bMsb = bConstp->width() - 1;
                ثابت V3_رقم& a_رقم = aConstp->رقم();
                ثابت V3_رقم& b_رقم = bConstp->رقم();
                // Max match is the shorter constant
                ثابت uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                لكل (; matchWidth < maxMatch; ++matchWidth) {
                    إذا (a_رقم.bitIs0(aMsb - matchWidth) != b_رقم.bitIs0(bMsb - matchWidth)) اكسر;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aMsb < bMsb ? aConstp : bConstp;
                أرجع {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        إذا (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) أرجع commonMSBs(catp->lhsp(), bp);
        إذا (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) أرجع commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        إذا (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            DfgVertex* ثابت fromp = selp->fromp();
            إذا (selp->msb() == fromp->width() - 1) {
                ثابت std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                أرجع {common.first, std::min(common.second, selp->width())};
            }
        }
        إذا (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            DfgVertex* ثابت fromp = selp->fromp();
            إذا (selp->msb() == fromp->width() - 1) {
                ثابت std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                أرجع {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        أرجع {nullptr, 0};
    }

    // VISIT methods

    فراغ visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    فراغ visit(Dfg_عدد_Ones* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ visit(DfgExtend* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* ثابت zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            أرجع;
        }
    }

    فراغ visit(DfgExtendS* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ visit(DfgLogNot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ visit(DfgNegate* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ visit(DfgNot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;

        // Not of Cond
        إذا (DfgCond* ثابت condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            إذا (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ادفع_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ثابت newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ثابت newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    أرجع;
                }
            }
        }

        // Not of Not
        إذا (DfgNot* ثابت notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                أرجع;
            }
        }

        إذا (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            إذا (DfgEq* ثابت eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    أرجع;
                }
            }

            // Not of Neq
            إذا (DfgNeq* ثابت neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    أرجع;
                }
            }

            // Not of unsigned comparisons
            إذا (DfgLt* ثابت p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    أرجع;
                }
            }
            إذا (DfgGte* ثابت p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    أرجع;
                }
            }
            إذا (DfgGt* ثابت p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    أرجع;
                }
            }
            إذا (DfgLte* ثابت p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    أرجع;
                }
            }

            // Not of signed comparisons
            إذا (DfgLtS* ثابت p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    أرجع;
                }
            }
            إذا (DfgGteS* ثابت p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    أرجع;
                }
            }
            إذا (DfgGtS* ثابت p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    أرجع;
                }
            }
            إذا (DfgLteS* ثابت p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgOneHot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ visit(DfgOneHot0* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ visit(DfgRedOr* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ visit(DfgRedAnd* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ visit(DfgRedXor* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ visit(DfgSel* ثابت vtxp) override {
        DfgVertex* ثابت fromp = vtxp->fromp();

        File_سطر* ثابت flp = vtxp->fileline();

        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t width = vtxp->width();
        ثابت uint32_t msb = lsb + width - 1;

        إذا (DfgConst* ثابت constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ثابت resp = makeZero(flp, width);
                resp->رقم().opSel(constp->رقم(), msb, lsb);
                replace(resp);
                أرجع;
            }
        }

        // Full width select, replace with the source.
        إذا (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                أرجع;
            }
        }

        // Unwind through bit packing in one go
        {
            ثابت auto res = unwindSel(fromp, lsb, width);
            إذا (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                أرجع;
            }
        }

        // Sel from Not
        إذا (DfgNot* ثابت notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            إذا (!notp->hasMultipleSinks()) {
                APPLYING(ادفع_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ثابت newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    أرجع;
                }
            }
        }

        // Sel from Cond
        إذا (DfgCond* ثابت condp = fromp->cast<DfgCond>()) {
            إذا (!condp->hasMultipleSinks()) {
                APPLYING(ادفع_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ثابت newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ثابت newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    أرجع;
                }
            }
        }

        // Sel from ShiftL
        إذا (DfgShiftL* ثابت shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            إذا (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(ادفع_SEL_THROUGH_SHIFTL) {
                    DfgSel* ثابت newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgMux* ثابت vtxp) override {
        DfgVertex* ثابت fromp = vtxp->fromp();
        DfgVertex* ثابت lsbp = vtxp->lsbp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (DfgConst* ثابت lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->رقم().toUInt()));
                أرجع;
            }
        }

        إذا (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                أرجع;
            }
        }

        إذا (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                أرجع;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    فراغ visit(DfgAnd* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        إذا (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            إذا (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                إذا (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* ثابت orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        أرجع;
                    }
                }
                إذا (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* ثابت newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ثابت orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            إذا (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    أرجع;
                }
            }

            إذا (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    أرجع;
                }
            }

            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_ادفع_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) أرجع;
            }
        }

        إذا (distributive_And_Associative_ثنائي<DfgOr, DfgAnd>(vtxp)) أرجع;

        إذا (try_ادفع_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_ادفع_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            إذا ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    أرجع;
                }
            }

            إذا (DfgAnd* ثابت rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ثابت rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ثابت rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                إذا ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        أرجع;
                    }
                }
            }
        }

        إذا (vtxp->dtype() == m_bitDType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            إذا (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            إذا (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ثابت newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgOr* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        إذا (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            إذا (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                إذا (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        أرجع;
                    }
                }
                إذا (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* ثابت newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgConcat* ثابت lhsConcatp = lhsp->cast<DfgConcat>()) {
            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    إذا (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            أرجع;
                        }
                    }
                    إذا (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            أرجع;
                        }
                    }
                }
            }
        }

        إذا (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            إذا (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    أرجع;
                }
            }

            إذا (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    أرجع;
                }
            }

            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_ادفع_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) أرجع;
            }
        }

        إذا (distributive_And_Associative_ثنائي<DfgAnd, DfgOr>(vtxp)) أرجع;

        إذا (try_ادفع_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_ادفع_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            إذا ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    أرجع;
                }
            }

            إذا (DfgOr* ثابت rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ثابت rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ثابت rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                إذا ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        أرجع;
                    }
                }
            }
        }

        إذا (vtxp->dtype() == m_bitDType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }
    }

    فراغ visit(DfgXor* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                أرجع;
            }
        }

        إذا (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            إذا (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    أرجع;
                }
            }
            إذا (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    أرجع;
                }
            }
            إذا (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_ادفع_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) أرجع;
            }
        }

        إذا (try_ادفع_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_ادفع_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        إذا (vtxp->dtype() == m_bitDType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    فراغ visit(DfgAdd* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                أرجع;
            }
        }

        ثابت std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_عدد_Ones_Term(lhsp);
        إذا (DfgVertex* ثابت lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_عدد_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            إذا (!std::get<0>(rTerm)) {
                إذا (DfgAdd* ثابت rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_عدد_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            إذا (DfgVertex* ثابت rVtxp = std::get<0>(rTerm)) {
                إذا (isSame(lVtxp, rVtxp)) {
                    ثابت uint32_t lLsb = std::get<1>(lTerm);
                    ثابت uint32_t rLsb = std::get<1>(rTerm);
                    ثابت uint32_t lWidth = std::get<2>(lTerm);
                    ثابت uint32_t rWidth = std::get<2>(rTerm);
                    منطقي adjoined = true;
                    uint32_t lsb = 0;
                    إذا (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } وإلا إذا (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } وإلا {
                        adjoined = خطأ;
                    }
                    إذا (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_عدد_ONES) {
                            DfgSel* ثابت selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_عدد_Ones>(flp, DfgDataType::packed(32), selp);
                            إذا (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } وإلا إذا (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            إذا (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            أرجع;
                        }
                    }
                }
            }
        }
    }

    فراغ visit(DfgArraySel* ثابت vtxp) override {
        DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>();
        إذا (!idxp) أرجع;
        DfgVarArray* ثابت varp = vtxp->fromp()->cast<DfgVarArray>();
        إذا (!varp) أرجع;
        AstVar* ثابت astVarp = varp->vscp()->varp();
        إذا (astVarp->isForced()) أرجع;
        إذا (astVarp->isSigUserRWPublic()) أرجع;
        DfgVertex* ثابت srcp = varp->srcp();
        إذا (!srcp) {
            إذا (vtxp->isPacked()) {
                إذا (AstConsArray* ثابت iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    إذا (AstConst* ثابت valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(جديد DfgConst{m_dfg, valp->fileline(), valp->رقم()});
                            أرجع;
                        }
                    }
                }
            }
            أرجع;
        }

        إذا (DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* ثابت driverp = splicep->driverAt(idxp->toSizeT());
            إذا (!driverp) أرجع;
            DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>();
            إذا (!uap) أرجع;
            إذا (uap->srcp()->is<DfgVertexSplice>()) أرجع;
            // If driven by a variable that had a Driver in DFG, it is partial
            إذا (DfgVertexVar* ثابت dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                إذا (dvarp->srcp()) أرجع;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                أرجع;
            }
        }

        إذا (DfgUnitArray* ثابت uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            إذا (uap->srcp()->is<DfgSplicePacked>()) أرجع;
            // If driven by a variable that had a Driver in DFG, it is partial
            إذا (DfgVertexVar* ثابت dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                إذا (dvarp->srcp()) أرجع;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                أرجع;
            }
        }
    }

    فراغ visit(DfgConcat* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isZero(lhsp)) {
            DfgConst* ثابت lConstp = lhsp->as<DfgConst>();
            إذا (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                إذا (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_قمة_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        أرجع;
                    }
                }
            }
        }

        إذا (isZero(rhsp)) {
            DfgConst* ثابت rConstp = rhsp->as<DfgConst>();
            إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                إذا (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgNot* ثابت lNot = lhsp->cast<DfgNot>()) {
            إذا (DfgNot* ثابت rNot = rhsp->cast<DfgNot>()) {
                إذا (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(ادفع_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ثابت newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        أرجع;
                    }
                }
            }
        }

        {
            منطقي swap = خطأ;
            إذا (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(ادفع_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    أرجع;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            إذا (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* ثابت rlp = rConcatp->lhsp();
                إذا (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(ادفع_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* ثابت newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            إذا (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                إذا (isSame(lSelp->fromp(), rSelp->fromp())) {
                    إذا (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            أرجع;
                        }
                    }
                }
            }

            إذا (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                إذا (DfgSel* ثابت rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    إذا (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        إذا (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                ثابت uint32_t width = lSelp->width() + rlSelp->width();
                                ثابت DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ثابت selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                أرجع;
                            }
                        }
                    }
                }
            }
        }

        إذا (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
            إذا (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                إذا (DfgSel* ثابت lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    إذا (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        إذا (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                ثابت uint32_t width = lrSelp->width() + rSelp->width();
                                ثابت DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ثابت selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                أرجع;
                            }
                        }
                    }
                }
            }
        }

        إذا (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            إذا (DfgCond* ثابت rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* ثابت rtVtxp = rCondp->thenp();
                DfgVertex* ثابت reVtxp = rCondp->elsep();
                DfgConst* ثابت rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ثابت reConstp = reVtxp->cast<DfgConst>();
                إذا (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(ادفع_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* ثابت thenp = [&]() -> DfgVertex* {
                            File_سطر* ثابت rtFlp = rtVtxp->fileline();
                            إذا (rtConstp) {
                                DfgConst* ثابت constp = makeZero(rtFlp, vtxp->width());
                                constp->رقم().opConcat(lConstp->رقم(), rtConstp->رقم());
                                أرجع constp;
                            }
                            أرجع make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* ثابت elsep = [&]() -> DfgVertex* {
                            File_سطر* ثابت reFlp = reVtxp->fileline();
                            إذا (reConstp) {
                                DfgConst* ثابت constp = makeZero(reFlp, vtxp->width());
                                constp->رقم().opConcat(lConstp->رقم(), reConstp->رقم());
                                أرجع constp;
                            }
                            أرجع make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            إذا (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* ثابت ltVtxp = lCondp->thenp();
                DfgVertex* ثابت leVtxp = lCondp->elsep();
                DfgConst* ثابت ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ثابت leConstp = leVtxp->cast<DfgConst>();
                إذا (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(ادفع_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* ثابت thenp = [&]() -> DfgVertex* {
                            File_سطر* ثابت ltFlp = ltVtxp->fileline();
                            إذا (ltConstp) {
                                DfgConst* ثابت constp = makeZero(ltFlp, vtxp->width());
                                constp->رقم().opConcat(ltConstp->رقم(), rConstp->رقم());
                                أرجع constp;
                            }
                            أرجع make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* ثابت elsep = [&]() -> DfgVertex* {
                            File_سطر* ثابت leFlp = leVtxp->fileline();
                            إذا (leConstp) {
                                DfgConst* ثابت constp = makeZero(leFlp, vtxp->width());
                                constp->رقم().opConcat(leConstp->رقم(), rConstp->رقم());
                                أرجع constp;
                            }
                            أرجع make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        أرجع;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ثابت uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            ثابت uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            ثابت uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            منطقي hasCrossSink = خطأ;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                إذا (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    ثابت uint32_t selLsb = selp->lsb();
                    ثابت uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    أرجع خطأ;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                إذا (ثابت DfgVarPacked* ثابت varp = sink.cast<DfgVarPacked>()) {
                    إذا (!varp->hasSinks() && !varp->isObserved()) أرجع خطأ;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                أرجع true;
            });
            إذا (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    إذا (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    إذا (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* ثابت catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ثابت sp = جديد DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* ثابت varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(true);
                    varp->srcp(sp);
                    replace(varp);
                    أرجع;
                }
            }
        }

        // Convert various forms to REPLICATE
        إذا (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                أرجع;
            }
        }
        إذا (DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            إذا (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    أرجع;
                }
            }
        }
        إذا (DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
            إذا (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    أرجع;
                }
            }
        }
        إذا (DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            إذا (DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
                إذا (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        أرجع;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            إذا (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                إذا (DfgRep* ثابت rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    إذا (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ثابت uint32_t width = lRepp->width() + rlRepp->width();
                            ثابت DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* ثابت repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            أرجع;
                        }
                    }
                }
            }
        }
    }

    فراغ visit(DfgDiv* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        إذا (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            إذا (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgDivS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        إذا (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            إذا (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgGt* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ visit(DfgGtS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ visit(DfgGte* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ visit(DfgGteS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ visit(DfgLogAnd* ثابت vtxp) override {
        إذا (ثنائي(vtxp) || vtxp->rhsp()->unsafe()) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        إذا (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                أرجع;
            }
        }
    }

    فراغ visit(DfgLogEq* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
    }

    فراغ visit(DfgLogIf* ثابت vtxp) override {
        إذا (ثنائي(vtxp) || vtxp->rhsp()->unsafe()) أرجع;
    }

    فراغ visit(DfgLogOr* ثابت vtxp) override {
        إذا (ثنائي(vtxp) || vtxp->rhsp()->unsafe()) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        إذا (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                أرجع;
            }
        }
    }

    فراغ visit(DfgLt* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ visit(DfgLtS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ visit(DfgLte* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ visit(DfgLteS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ visit(DfgModDiv* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
    }

    فراغ visit(DfgModDivS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
    }

    فراغ visit(DfgMul* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            إذا (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    أرجع;
                }
            }
            إذا (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgMulS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            إذا (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    أرجع;
                }
            }
            إذا (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgEq* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }

        إذا (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_ادفع_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) أرجع;
            }

            إذا (rhsp->dtype() == m_bitDType) {
                إذا (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        أرجع;
                    }
                }

                إذا (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ visit(DfgNeq* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }

        إذا (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_ادفع_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) أرجع;
            }

            إذا (rhsp->dtype() == m_bitDType) {
                إذا (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        أرجع;
                    }
                }

                إذا (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ visit(DfgPow* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
    }

    فراغ visit(DfgPowSS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
    }

    فراغ visit(DfgPowSU* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
    }

    فراغ visit(DfgPowUS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
    }

    فراغ visit(DfgRep* ثابت vtxp) override {
        DfgVertex* ثابت srcp = vtxp->srcp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (DfgConst* ثابت sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ثابت resultp = makeZero(flp, vtxp->width());
                resultp->رقم().opRepl(sConstp->رقم(), vtxp->عدد());
                replace(resultp);
                أرجع;
            }
        }

        إذا (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                أرجع;
            }
        }

        إذا (DfgRep* ثابت sRepp = srcp->cast<DfgRep>()) {
            إذا (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgShiftL* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
        إذا (optimizeShiftRHS(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        إذا (DfgShiftL* ثابت lShiftLp = lhsp->cast<DfgShiftL>()) {
            إذا (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_سطر* ثابت flp = vtxp->fileline();
                    ثابت DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ثابت bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* ثابت cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ثابت addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    أرجع;
                }
            }
        }

        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t shiftAmount = rConstp->toU32();

            إذا (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    أرجع;
                }
            }

            إذا (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    أرجع;
                }
            }

            إذا (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                إذا (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* ثابت lRhsp = lConcatp->rhsp();
                        DfgVertex* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t rW = shiftAmount;
                        ثابت uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        ثابت uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_سطر* ثابت flp = lConcatp->fileline();
                        DfgVertex* ثابت rp = makeZero(flp, shiftAmount);
                        DfgVertex* ثابت mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        إذا (!lW) {
                            replace(np);
                            أرجع;
                        }
                        DfgVertex* ثابت lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        أرجع;
                    }
                }
            }

            إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                إذا (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ثابت uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        أرجع;
                    }
                }
            }

            إذا (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                إذا (!lCondp->hasMultipleSinks()) {
                    APPLYING(ادفع_SHIFTL_THROUGH_COND) {
                        DfgShiftL* ثابت tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* ثابت ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ visit(DfgShiftR* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
        إذا (optimizeShiftRHS(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        إذا (DfgShiftR* ثابت lShiftRp = lhsp->cast<DfgShiftR>()) {
            إذا (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_سطر* ثابت flp = vtxp->fileline();
                    ثابت DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ثابت bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* ثابت cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ثابت addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    أرجع;
                }
            }
        }

        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t shiftAmount = rConstp->toU32();

            إذا (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    أرجع;
                }
            }

            إذا (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    أرجع;
                }
            }

            إذا (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                إذا (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* ثابت lRhsp = lConcatp->rhsp();
                        DfgVertex* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t lW = shiftAmount;
                        ثابت uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        ثابت uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_سطر* ثابت flp = lConcatp->fileline();
                        DfgVertex* ثابت lp = makeZero(flp, shiftAmount);
                        DfgVertex* ثابت mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        إذا (!rW) {
                            replace(np);
                            أرجع;
                        }
                        DfgVertex* ثابت rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        أرجع;
                    }
                }
            }

            إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                إذا (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ثابت uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        أرجع;
                    }
                }
            }

            إذا (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                إذا (!lCondp->hasMultipleSinks()) {
                    APPLYING(ادفع_SHIFTR_THROUGH_COND) {
                        DfgShiftR* ثابت tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* ثابت ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ visit(DfgShiftRS* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;
        إذا (optimizeShiftRHS(vtxp)) أرجع;
    }

    فراغ visit(DfgSub* ثابت vtxp) override {
        إذا (ثنائي(vtxp)) أرجع;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                أرجع;
            }
        }

        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            إذا (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    أرجع;
                }
            }
            إذا (vtxp->dtype() == m_bitDType && rConstp->has_قيمة(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgMatchMasked* ثابت vtxp) override {
        إذا (DfgConst* ثابت constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ثابت matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->رقم(), matchVarp)));
                أرجع;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    فراغ visit(DfgCond* ثابت vtxp) override {
        DfgVertex* ثابت condp = vtxp->condp();
        DfgVertex* ثابت thenp = vtxp->thenp();
        DfgVertex* ثابت elsep = vtxp->elsep();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (condp->dtype() != m_bitDType) أرجع;

        إذا (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                أرجع;
            }
        }

        إذا (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                أرجع;
            }
        }

        إذا (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                أرجع;
            }
        }

        إذا (DfgNot* ثابت condNotp = condp->cast<DfgNot>()) {
            إذا (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    أرجع;
                }
            }
        }

        إذا (DfgNeq* ثابت condNeqp = condp->cast<DfgNeq>()) {
            إذا (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ثابت newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    أرجع;
                }
            }
        }

        إذا (DfgNot* ثابت thenNotp = thenp->cast<DfgNot>()) {
            إذا (DfgNot* ثابت elseNotp = elsep->cast<DfgNot>()) {
                إذا (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ثابت newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgOr* ثابت condOrp = condp->cast<DfgOr>()) {
            إذا (DfgCond* ثابت thenCondp = thenp->cast<DfgCond>()) {
                إذا (!thenCondp->hasMultipleSinks()) {
                    إذا (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ثابت ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            أرجع;
                        }
                    }
                    إذا (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ثابت ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            أرجع;
                        }
                    }
                }
            }
        }

        إذا (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            إذا (DfgAdd* ثابت thenAddp = thenp->cast<DfgAdd>()) {
                إذا (DfgConst* ثابت constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    إذا (constp->has_قيمة(1)) {
                        إذا (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_سطر* ثابت thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                أرجع;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            إذا (DfgSub* ثابت thenSubp = thenp->cast<DfgSub>()) {
                إذا (DfgConst* ثابت constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    إذا (constp->has_قيمة(1)) {
                        إذا (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_سطر* ثابت thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                أرجع;
                            }
                        }
                    }
                }
            }
        }

        إذا (vtxp->dtype() == m_bitDType) {
            إذا (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    أرجع;
                }
            }
            إذا (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    أرجع;
                }
            }
        }

        إذا (vtxp->width() <= VL_QUADSIZE) {
            إذا (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    أرجع;
                }
            }
            إذا (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    أرجع;
                }
            }
            إذا (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    أرجع;
                }
            }
            إذا (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    أرجع;
                }
            }

            إذا (DfgOr* ثابت tOrp = thenp->cast<DfgOr>()) {
                إذا (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        أرجع;
                    }
                }
                إذا (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        أرجع;
                    }
                }
            }
        }

        إذا (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ثابت std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            إذا (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ثابت uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    إذا (rWidth != rhsp->width()) {
                        ثابت DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    إذا (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        أرجع;
                    }
                    // Create new LHS
                    ثابت uint32_t lWidth = vtxp->width() - rWidth;
                    ثابت DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* ثابت lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* ثابت lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* ثابت lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    أرجع;
                }
            }

            ثابت std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            إذا (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ثابت uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    إذا (lWidth != lhsp->width()) {
                        ثابت DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    إذا (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        أرجع;
                    }
                    // Create new RHS
                    ثابت uint32_t rWidth = vtxp->width() - lWidth;
                    ثابت DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* ثابت rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* ثابت rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* ثابت rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    أرجع;
                }
            }
        }

        إذا (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                إذا (ثابت uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ثابت zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                أرجع;
            }
        }

        إذا (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                إذا (ثابت uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ثابت zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                أرجع;
            }
        }

        إذا (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                أرجع;
            }
        }

        إذا (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                أرجع;
            }
        }

        إذا (DfgCond* ثابت tCondp = thenp->cast<DfgCond>()) {
            إذا (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    أرجع;
                }
            }
        }

        إذا (DfgCond* ثابت eCondp = elsep->cast<DfgCond>()) {
            إذا (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    أرجع;
                }
            }
        }
    }

    فراغ visit(DfgVertexVar* ثابت vtxp) override {
        إذا (vtxp->hasSinks()) أرجع;
        إذا (vtxp->isObserved()) أرجع;
        إذا (vtxp->defaultp()) أرجع;

        // If undriven, or driven from another var, it is completely redundant.
        إذا (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                أرجع;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        منطقي foundOne = خطأ;
        ثابت منطقي keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> منطقي {
            // Ignore non-observable variable sinks. These can be eliminated.
            إذا (ثابت DfgVertexVar* ثابت varp = sink.cast<DfgVertexVar>()) {
                إذا (!varp->hasSinks() && !varp->isObserved()) أرجع خطأ;
            }
            // Keep if found more than one sink
            إذا (foundOne) أرجع true;
            // Mark first sink found
            foundOne = true;
            أرجع خطأ;
        });
        إذا (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                أرجع;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        لكل (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ادفع_back(nullptr);
        m_iterList.ادفع_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        لكل (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        لكل (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        طالما (true) {
            // Process the work list - keep the placeholder at index 0
            طالما (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.اسحب_back();
                إذا (!m_vtxp) استمر;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                إذا (DfgVertexVar* ثابت varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    استمر;
                }

                // Ast references are not considered
                إذا (m_vtxp->is<DfgVertexAst>()) استمر;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                إذا (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    إذا (DfgVertex* ثابت sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            استمر;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            إذا (m_iterList.size() == 1) اكسر;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                لكل (size_t hops = 1; hops <= 4; ++hops) {
                    لكل (size_t i = begin; i < end; ++i) {
                        DfgVertex* ثابت vtxp = m_iterList[i];
                        إذا (!vtxp) استمر;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            أرجع خطأ;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            أرجع خطأ;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            لكل (DfgVertex* ثابت vtxp : m_iterList) {
                إذا (!vtxp) استمر;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

عام:
    ساكن فراغ apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

فراغ V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    إذا (!v3Global.opt.fDfgPeephole()) أرجع;
    V3DfgPeephole::apply(dfg, ctx);
}
