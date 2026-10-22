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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgCache.h"
#அந்தர்பாவ "V3DfgDataType.h"
#அந்தர்பாவ "V3DfgPasses.h"
#அந்தர்பாவ "V3DfgPeepholePatterns.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cctype>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
நாமாகாஶ {

template <typename T_Reduction>
கட்டமைப்பு ReductionToBitwiseImpl {};
template <> கட்டமைப்பு ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> கட்டமைப்பு ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> கட்டமைப்பு ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
கட்டமைப்பு BitwiseToReductionImpl {};
template <> கட்டமைப்பு BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> கட்டமைப்பு BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> கட்டமைப்பு BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
கட்டமைப்பு IsAssociative : பொது std::false_type {};
template <> கட்டமைப்பு IsAssociative<DfgAnd> : பொது std::true_type {};
template <> கட்டமைப்பு IsAssociative<DfgOr>  : பொது std::true_type {};
template <> கட்டமைப்பு IsAssociative<DfgXor> : பொது std::true_type {};
template <> கட்டமைப்பு IsAssociative<DfgAdd> : பொது std::true_type {};
template <> கட்டமைப்பு IsAssociative<DfgConcat> : பொது std::true_type {};
template <> கட்டமைப்பு IsAssociative<DfgMul> : பொது std::true_type {};
template <> கட்டமைப்பு IsAssociative<DfgMulS> : பொது std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
கட்டமைப்பு IsCommutative : பொது std::false_type {};
template <> கட்டமைப்பு IsCommutative<DfgAnd> : பொது std::true_type {};
template <> கட்டமைப்பு IsCommutative<DfgOr>  : பொது std::true_type {};
template <> கட்டமைப்பு IsCommutative<DfgXor> : பொது std::true_type {};
template <> கட்டமைப்பு IsCommutative<DfgAdd> : பொது std::true_type {};
template <> கட்டமைப்பு IsCommutative<DfgEq> : பொது std::true_type {};
template <> கட்டமைப்பு IsCommutative<DfgMul> : பொது std::true_type {};
template <> கட்டமைப்பு IsCommutative<DfgMulS> : பொது std::true_type {};
template <> கட்டமைப்பு IsCommutative<DfgNeq> : பொது std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
கட்டமைப்பு IsIdempotent : பொது std::false_type {};
template <> கட்டமைப்பு IsIdempotent<DfgAnd> : பொது std::true_type {};
template <> கட்டமைப்பு IsIdempotent<DfgOr>  : பொது std::true_type {};

// Binary result dtype
template<typename Vertex>
மாறிலி DfgDataType& resultDType(மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp);
template <> மாறிலி DfgDataType& resultDType<DfgAdd>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgAnd>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgConcat>     (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> மாறிலி DfgDataType& resultDType<DfgDiv>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgDivS>       (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgEq>         (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgGt>         (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgGtS>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgGte>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgGteS>       (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgLogAnd>     (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgLogEq>      (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgLogIf>      (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgLogOr>      (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgLt>         (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgLtS>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgLte>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgLteS>       (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgModDiv>     (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgModDivS>    (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgMul>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgMulS>       (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgNeq>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு DfgDataType::packed(1); }
template <> மாறிலி DfgDataType& resultDType<DfgOr>         (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgPow>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgPowSS>      (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgPowSU>      (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgPowUS>      (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgShiftL>     (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgShiftR>     (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgShiftRS>    (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgSub>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }
template <> மாறிலி DfgDataType& resultDType<DfgXor>        (மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) { திரும்பு lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> வெறுமை foldOp(V3_எண்& out, மாறிலி V3_எண்& src);
template <> வெறுமை foldOp<Dfg_மொத்தம்_Ones>  (V3_எண்& out, மாறிலி V3_எண்& src) { out.op_மொத்தம்_Ones(src); }
template <> வெறுமை foldOp<DfgExtend>     (V3_எண்& out, மாறிலி V3_எண்& src) { out.opAssign(src); }
template <> வெறுமை foldOp<DfgExtendS>    (V3_எண்& out, மாறிலி V3_எண்& src) { out.opExtendS(src, src.width()); }
template <> வெறுமை foldOp<DfgLogNot>     (V3_எண்& out, மாறிலி V3_எண்& src) { out.opLogNot(src); }
template <> வெறுமை foldOp<DfgNegate>     (V3_எண்& out, மாறிலி V3_எண்& src) { out.opNegate(src); }
template <> வெறுமை foldOp<DfgNot>        (V3_எண்& out, மாறிலி V3_எண்& src) { out.opNot(src); }
template <> வெறுமை foldOp<DfgOneHot>     (V3_எண்& out, மாறிலி V3_எண்& src) { out.opOneHot(src); }
template <> வெறுமை foldOp<DfgOneHot0>    (V3_எண்& out, மாறிலி V3_எண்& src) { out.opOneHot0(src); }
template <> வெறுமை foldOp<DfgRedAnd>     (V3_எண்& out, மாறிலி V3_எண்& src) { out.opRedAnd(src); }
template <> வெறுமை foldOp<DfgRedOr>      (V3_எண்& out, மாறிலி V3_எண்& src) { out.opRedOr(src); }
template <> வெறுமை foldOp<DfgRedXor>     (V3_எண்& out, மாறிலி V3_எண்& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> வெறுமை foldOp(V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
template <> வெறுமை foldOp<DfgAdd>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opAdd(lhs, rhs); }
template <> வெறுமை foldOp<DfgAnd>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opAnd(lhs, rhs); }
template <> வெறுமை foldOp<DfgConcat>     (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opConcat(lhs, rhs); }
template <> வெறுமை foldOp<DfgDiv>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opDiv(lhs, rhs); }
template <> வெறுமை foldOp<DfgDivS>       (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opDivS(lhs, rhs); }
template <> வெறுமை foldOp<DfgEq>         (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opEq(lhs, rhs); }
template <> வெறுமை foldOp<DfgGt>         (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opGt(lhs, rhs); }
template <> வெறுமை foldOp<DfgGtS>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opGtS(lhs, rhs); }
template <> வெறுமை foldOp<DfgGte>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opGte(lhs, rhs); }
template <> வெறுமை foldOp<DfgGteS>       (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opGteS(lhs, rhs); }
template <> வெறுமை foldOp<DfgLogAnd>     (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opLogAnd(lhs, rhs); }
template <> வெறுமை foldOp<DfgLogEq>      (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opLogEq(lhs, rhs); }
template <> வெறுமை foldOp<DfgLogIf>      (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opLogIf(lhs, rhs); }
template <> வெறுமை foldOp<DfgLogOr>      (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opLogOr(lhs, rhs); }
template <> வெறுமை foldOp<DfgLt>         (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opLt(lhs, rhs); }
template <> வெறுமை foldOp<DfgLtS>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opLtS(lhs, rhs); }
template <> வெறுமை foldOp<DfgLte>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opLte(lhs, rhs); }
template <> வெறுமை foldOp<DfgLteS>       (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opLteS(lhs, rhs); }
template <> வெறுமை foldOp<DfgModDiv>     (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opModDiv(lhs, rhs); }
template <> வெறுமை foldOp<DfgModDivS>    (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opModDivS(lhs, rhs); }
template <> வெறுமை foldOp<DfgMul>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opMul(lhs, rhs); }
template <> வெறுமை foldOp<DfgMulS>       (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opMulS(lhs, rhs); }
template <> வெறுமை foldOp<DfgNeq>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opNeq(lhs, rhs); }
template <> வெறுமை foldOp<DfgOr>         (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opOr(lhs, rhs); }
template <> வெறுமை foldOp<DfgPow>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opPow(lhs, rhs); }
template <> வெறுமை foldOp<DfgPowSS>      (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opPowSS(lhs, rhs); }
template <> வெறுமை foldOp<DfgPowSU>      (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opPowSU(lhs, rhs); }
template <> வெறுமை foldOp<DfgPowUS>      (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opPowUS(lhs, rhs); }
template <> வெறுமை foldOp<DfgShiftL>     (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opShiftL(lhs, rhs); }
template <> வெறுமை foldOp<DfgShiftR>     (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opShiftR(lhs, rhs); }
template <> வெறுமை foldOp<DfgShiftRS>    (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> வெறுமை foldOp<DfgSub>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opSub(lhs, rhs); }
template <> வெறுமை foldOp<DfgXor>        (V3_எண்& out, மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

வகுப்பு V3DfgPeephole final : பொது DfgVisitor {
    // TYPES
    கட்டமைப்பு VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        தருக்க m_isCachedVertex = பொய்;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    மாறிலி DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* மாறிலி m_tmpScopep = v3Global.rootp()->உச்சி_Scopep()->scopep();

    // STATIC STATE
    நிலையான V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) எனில் (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    தருக்க checkApplying(VDfgPeepholePattern id) {
        எனில் (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) திரும்பு பொய்;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_மொத்தம்[id];
        திரும்பு உண்மை;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* மாறிலி equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        திரும்பு equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    வெறுமை invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = பொய்;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_வரி* flp, மாறிலி DfgDataType& dtype, Operands... operands) {
        Vertex* மாறிலி vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = உண்மை;
        திரும்பு vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(மாறிலி DfgDataType& dtype, Operands... operands) {
        Vertex* மாறிலி vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        திரும்பு vtxp;
    }

    வெறுமை incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    வெறுமை addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        எனில் (vInfo.m_workListIndex) திரும்பு;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.தள்ளு_back(vtxp);
    }

    வெறுமை removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    வெறுமை addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        எனில் (vInfo.m_iterListIndex) திரும்பு;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.தள்ளு_back(vtxp);
    }

    வெறுமை removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    வெறுமை deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        மாறிலி DfgVertexVar* மாறிலி varp = vtxp->cast<DfgVertexVar>();
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
            எனில் (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                திரும்பு பொய்;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            எனில் (vInfo.m_generation != m_currentGeneration) srcps.தள்ளு_back(&src);
            vInfo.m_generation = m_currentGeneration;
            திரும்பு பொய்;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        எனில் (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.தள்ளு_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } இல்லையெனில் {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        மாறிலி auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  திரும்பு srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        க்கு (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        க்கு (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    வெறுமை replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        மாறிலி auto debugCallback = [&]() -> வெறுமை {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            மாறிலி auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](மாறிலி DfgVertex& v) {  //
                திரும்பு cone->மொத்தம்(&v);
            });
        };
        எனில் (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) திரும்பு;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            திரும்பு பொய்;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            திரும்பு பொய்;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            திரும்பு பொய்;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            திரும்பு பொய்;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_வரி* flp, uint32_t val) { திரும்பு புதிய DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_வரி* flp, uint32_t width) {
        திரும்பு புதிய DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_வரி* flp, மாறிலி DfgDataType& dtype) {
        திரும்பு makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_வரி* flp, uint32_t width) {
        DfgConst* மாறிலி resp = makeZero(flp, width);
        resp->இலக்கம்().setAllBits1();
        திரும்பு resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_வரி* flp, மாறிலி DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* மாறிலி vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        எனில் (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        எனில் (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        திரும்பு vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(மாறிலி DfgVertex* examplep, Operands... operands) {
        திரும்பு make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        எனில் (vtxp->dtype() == m_bitDType) திரும்பு bitp;
        திரும்பு make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    நிலையான தருக்க isSame(மாறிலி DfgVertex* ap, மாறிலி DfgVertex* bp) {
        எனில் (ap == bp) திரும்பு உண்மை;
        எனில் (ap->dtype() != bp->dtype()) திரும்பு பொய்;
        மாறிலி DfgConst* மாறிலி aConstp = ap->cast<DfgConst>();
        எனில் (!aConstp) திரும்பு பொய்;
        மாறிலி DfgConst* மாறிலி bConstp = bp->cast<DfgConst>();
        எனில் (!bConstp) திரும்பு பொய்;
        திரும்பு aConstp->இலக்கம்().isCaseEq(bConstp->இலக்கம்());
    }

    நிலையான தருக்க isZero(மாறிலி DfgVertex* vtxp) {
        எனில் (மாறிலி DfgConst* மாறிலி constp = vtxp->cast<DfgConst>()) திரும்பு constp->isZero();
        திரும்பு பொய்;
    }

    நிலையான தருக்க isOnes(மாறிலி DfgVertex* vtxp) {
        எனில் (மாறிலி DfgConst* மாறிலி constp = vtxp->cast<DfgConst>()) திரும்பு constp->isOnes();
        திரும்பு பொய்;
    }

    நிலையான தருக்க isEqOne(மாறிலி DfgVertex* vtxp) {
        எனில் (மாறிலி DfgConst* மாறிலி constp = vtxp->cast<DfgConst>()) திரும்பு constp->இலக்கம்().isEqOne();
        திரும்பு பொய்;
    }

    நிலையான தருக்க areAdjacent(uint32_t& lsb, மாறிலி DfgSel* lSelp, மாறிலி DfgSel* rSelp) {
        எனில் (!isSame(lSelp->srcp(), rSelp->srcp())) திரும்பு பொய்;
        எனில் (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            திரும்பு உண்மை;
        }
        எனில் (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க foldUnary(Vertex* மாறிலி vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::மதிப்பு, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::மதிப்பு, "Must invoke on final class");
        எனில் (DfgConst* மாறிலி srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* மாறிலி resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->இலக்கம்(), srcp->இலக்கம்());
                replace(resultp);
                திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க இருமம்(Vertex* மாறிலி vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_இருமம், Vertex>::மதிப்பு, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::மதிப்பு, "Must invoke on final class");

        File_வரி* மாறிலி flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* மாறிலி lConstp = lhsp->cast<DfgConst>();
        DfgConst* மாறிலி rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* மாறிலி lSamep = lhsp->cast<Vertex>();
        Vertex* மாறிலி rSamep = rhsp->cast<Vertex>();

        // Constant folding
        எனில் (lConstp && rConstp) {
            APPLYING(FOLD_இருமம்) {
                DfgConst* மாறிலி resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->இலக்கம்(), lConstp->இலக்கம்(), rConstp->இலக்கம்());
                replace(resultp);
                திரும்பு உண்மை;
            }
        }

        // Idempotent
        எனில் VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::மதிப்பு) {
            எனில் (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_இருமம்) {
                    replace(lhsp);
                    திரும்பு உண்மை;
                }
            }
        }

        // Associative
        எனில் VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::மதிப்பு) {
            // Also commutative and idempotent
            எனில் VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::மதிப்பு && IsIdempotent<Vertex>::மதிப்பு) {
                எனில் (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_இருமம்_LHS) {
                        replace(lSamep);
                        திரும்பு உண்மை;
                    }
                }
                எனில் (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_இருமம்_RHS) {
                        replace(rSamep);
                        திரும்பு உண்மை;
                    }
                }
            }

            எனில் (lConstp && rSamep) {
                எனில் (DfgConst* மாறிலி rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_இருமம்_LHS_OF_RHS) {
                        DfgConst* மாறிலி cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->இலக்கம்(), lConstp->இலக்கம்(), rlConstp->இலக்கம்());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        திரும்பு உண்மை;
                    }
                }
            }

            எனில் (lSamep && rConstp) {
                எனில் (DfgConst* மாறிலி lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_இருமம்_RHS_OF_LHS) {
                        DfgConst* மாறிலி cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->இலக்கம்(), lrConstp->இலக்கம்(), rConstp->இலக்கம்());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        திரும்பு உண்மை;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            எனில் (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* மாறிலி ap = lSamep->lhsp();
                DfgVertex* மாறிலி bp = lSamep->rhsp();
                DfgVertex* மாறிலி cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                எனில் (ap != cp && bp != cp) {
                    APPLYING(வலது_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        மாறிலி DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* மாறிலி bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        திரும்பு உண்மை;
                    }
                }
            }

            எனில் (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* மாறிலி rlVtxp = rSamep->lhsp();
                DfgVertex* மாறிலி rrVtxp = rSamep->rhsp();

                எனில் VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::மதிப்பு) {
                    எனில் (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            திரும்பு உண்மை;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                எனில் (Vertex* மாறிலி existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    எனில் (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_இருமம்_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            திரும்பு உண்மை;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                எனில் VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::மதிப்பு) {
                    எனில் (Vertex* மாறிலி existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        எனில் (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_இருமம்_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                திரும்பு உண்மை;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        எனில் VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::மதிப்பு) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            எனில் (lConstp) {
                எனில் (DfgCond* மாறிலி rCondp = rhsp->cast<DfgCond>()) {
                    எனில் (!rCondp->hasMultipleSinks()) {
                        APPLYING(தள்ளு_COMMUTATIVE_இருமம்_THROUGH_COND) {
                            DfgVertex* மாறிலி tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* மாறிலி ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            திரும்பு உண்மை;
                        }
                    }
                }
                திரும்பு பொய்;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        மாறிலி தருக்க swapSides = [&]() {
            // Don't swap if not commutative, duh
            எனில் VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::மதிப்பு) திரும்பு பொய்;
            // Const go on left
            எனில் (lConstp) திரும்பு பொய்;
            எனில் (rConstp) திரும்பு உண்மை;
            // Not go on left
            எனில் (lhsp->is<DfgNot>()) திரும்பு பொய்;
            எனில் (rhsp->is<DfgNot>()) திரும்பு உண்மை;
            // Same go on right
            எனில் (rSamep) திரும்பு பொய்;
            எனில் (lSamep) திரும்பு உண்மை;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            மாறிலி VertexInfo& lInfo = m_vInfo[lhsp];
            மாறிலி VertexInfo& rInfo = m_vInfo[rhsp];
            திரும்பு lInfo.m_id > rInfo.m_id;
        }();
        எனில் (swapSides) {
            APPLYING(SWAP_SIDES_IN_இருமம்) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                திரும்பு உண்மை;
            }
        }

        திரும்பு பொய்;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க distributive_And_Associative_இருமம்(Vertex* vtxp) {
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        எனில் (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            எனில் (Other* மாறிலி lp = lhsp->cast<Other>()) {
                எனில் (Other* மாறிலி rp = rhsp->cast<Other>()) {
                    DfgVertex* மாறிலி llp = lp->lhsp();
                    DfgVertex* மாறிலி lrp = lp->rhsp();
                    DfgVertex* மாறிலி rlp = rp->lhsp();
                    DfgVertex* மாறிலி rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    எனில் (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } இல்லையெனில் எனில் (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } இல்லையெனில் எனில் (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } இல்லையெனில் எனில் (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    எனில் (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_இருமம்) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            திரும்பு உண்மை;
                        }
                    }
                }
            }
        }

        திரும்பு பொய்;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க
    try_தள்ளு_Bitwise_Op_Through_Concat(Vertex* மாறிலி vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_வரி* மாறிலி flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* மாறிலி catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* மாறிலி catRConstp = concatp->rhsp()->cast<DfgConst>();
        எனில் (catLConstp || catRConstp) {
            APPLYING(தள்ளு_BITWISE_OP_THROUGH_CONCAT) {
                மாறிலி uint32_t width = concatp->width();
                மாறிலி DfgDataType& lDtype = concatp->lhsp()->dtype();
                மாறிலி DfgDataType& rDtype = concatp->rhsp()->dtype();
                மாறிலி uint32_t lWidth = lDtype.size();
                மாறிலி uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* மாறிலி newLhsp = [&]() -> DfgVertex* {
                    DfgConst* மாறிலி newLhsConstp = makeZero(constp->fileline(), lWidth);
                    எனில் (catLConstp) {
                        V3_எண் இலக்கம்{constp->fileline(), static_cast<முழு>(lWidth), 0u};
                        இலக்கம்.opSel(constp->இலக்கம்(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->இலக்கம்(), இலக்கம், catLConstp->இலக்கம்());
                        திரும்பு newLhsConstp;
                    }
                    newLhsConstp->இலக்கம்().opSel(constp->இலக்கம்(), width - 1, rWidth);
                    திரும்பு make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* மாறிலி newRhsp = [&]() -> DfgVertex* {
                    DfgConst* மாறிலி newRhsConstp = makeZero(constp->fileline(), rWidth);
                    எனில் (catRConstp) {
                        V3_எண் இலக்கம்{constp->fileline(), static_cast<முழு>(rWidth), 0u};
                        இலக்கம்.opSel(constp->இலக்கம்(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->இலக்கம்(), இலக்கம், catRConstp->இலக்கம்());
                        திரும்பு newRhsConstp;
                    }
                    newRhsConstp->இலக்கம்().opSel(constp->இலக்கம்(), rWidth - 1, 0);
                    திரும்பு make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க
    try_தள்ளு_Compare_Op_Through_Concat(Vertex* மாறிலி vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_வரி* மாறிலி flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        எனில் (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(தள்ளு_COMPARE_OP_THROUGH_CONCAT) {
                மாறிலி uint32_t width = concatp->width();
                மாறிலி uint32_t lWidth = concatp->lhsp()->width();
                மாறிலி uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* மாறிலி newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->இலக்கம்().opSel(constp->இலக்கம்(), width - 1, rWidth);
                Vertex* மாறிலி newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* மாறிலி newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->இலக்கம்().opSel(constp->இலக்கம்(), rWidth - 1, 0);
                Vertex* மாறிலி newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                எனில் VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::மதிப்பு) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } இல்லையெனில் எனில் VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::மதிப்பு) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } இல்லையெனில் {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க try_தள்ளு_Bitwise_Op_Through_Reductions(Bitwise* மாறிலி vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        எனில் (Reduction* மாறிலி lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            எனில் (Reduction* மாறிலி rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* மாறிலி lSrcp = lRedp->srcp();
                DfgVertex* மாறிலி rSrcp = rRedp->srcp();
                எனில் (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(தள்ளு_BITWISE_THROUGH_REDUCTION) {
                        File_வரி* மாறிலி flp = vtxp->fileline();
                        Bitwise* மாறிலி bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        திரும்பு உண்மை;
                    }
                }
            }
        }

        திரும்பு பொய்;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    நிலையான std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        எனில் (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            திரும்பு {nullptr, nullptr};
        }
        எனில் (vtxp->hasMultipleSinks()) திரும்பு {nullptr, nullptr};
        Dfg_Vertex_இருமம்* மாறிலி bwp = vtxp->as<Dfg_Vertex_இருமம்>();
        DfgSel* மாறிலி lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* மாறிலி rSelp = bwp->rhsp()->cast<DfgSel>();
        எனில் (!lSelp || !rSelp) திரும்பு {nullptr, nullptr};
        DfgSel* மாறிலி nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* மாறிலி wSelp = nSelp == lSelp ? rSelp : lSelp;
        மாறிலி uint32_t nWidth = nSelp->fromp()->width();
        எனில் (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) திரும்பு {nullptr, nullptr};
        எனில் (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            திரும்பு {nullptr, nullptr};
        }
        திரும்பு {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க try_தள்ளு_Bitwise_Op_Throug_Sel(Bitwise* மாறிலி vtxp) {
        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();

        எனில் (DfgSel* மாறிலி lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            எனில் (DfgSel* மாறிலி selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } இல்லையெனில் எனில் (Bitwise* மாறிலி bitwisep = rhsp->cast<Bitwise>()) {
                எனில் (DfgSel* மாறிலி rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } இல்லையெனில் எனில் (DfgSel* மாறிலி rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            எனில் (rSelp) {
                DfgVertex* மாறிலி lFromp = lSelp->fromp();
                DfgVertex* மாறிலி rFromp = rSelp->fromp();
                எனில் (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(தள்ளு_BITWISE_THROUGH_SEL) {
                        Bitwise* மாறிலி bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        எனில் (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        திரும்பு உண்மை;
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
            Bitwise* மாறிலி rChainp = rhsp->cast<Bitwise>();
            எனில் (rChainp && rChainp->hasMultipleSinks()) திரும்பு பொய்;
            DfgVertex* மாறிலி rTermp = rChainp ? rChainp->lhsp() : rhsp;

            மாறிலி auto lSels = pushableUnalignedSels(lhsp);
            மாறிலி auto rSels = pushableUnalignedSels(rTermp);
            எனில் (!lSels.first && !rSels.first) திரும்பு பொய்;

            மாறிலி DfgVertex* மாறிலி opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            எனில் (opp->is<Bitwise>()) திரும்பு பொய்;
            மாறிலி auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* மாறிலி nFromp = sels.first->fromp();
            DfgVertex* மாறிலி wFromp = sels.second->fromp();
            மாறிலி uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            மாறிலி auto matches = [&](மாறிலி DfgVertex* termp, மாறிலி auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                எனில் (tSels.first) {
                    திரும்பு termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                எனில் (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                எனில் (மாறிலி DfgSel* மாறிலி selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                எனில் (termp->type() != opp->type()) திரும்பு பொய்;
                // Operands might have been swapped since
                மாறிலி Dfg_Vertex_இருமம்* மாறிலி bwp = termp->as<Dfg_Vertex_இருமம்>();
                மாறிலி தருக்க nIsLhs = bwp->lhsp() == nFromp;
                எனில் (!nIsLhs && bwp->rhsp() != nFromp) திரும்பு பொய்;
                மாறிலி DfgSel* மாறிலி slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                திரும்பு slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            எனில் (!matches(lhsp, lSels) || !matches(rTermp, rSels)) திரும்பு பொய்;

            APPLYING(தள்ளு_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_வரி* மாறிலி flp = vtxp->fileline();
                மாறிலி DfgDataType& dtype = nFromp->dtype();
                DfgSel* மாறிலி slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                எனில் (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } இல்லையெனில் எனில் (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } இல்லையெனில் {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                மாறிலி auto தள்ளு = [&](DfgVertex* termp, மாறிலி DfgSel* nSelp) -> DfgVertex* {
                    திரும்பு nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* மாறிலி newLhsp = தள்ளு(lhsp, lSels.first);
                DfgVertex* மாறிலி newRTermp = தள்ளு(rTermp, rSels.first);
                எனில் (rChainp) {
                    DfgVertex* மாறிலி newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } இல்லையெனில் {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                திரும்பு உண்மை;
            }
        }

        திரும்பு பொய்;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    நிலையான தருக்க isConcatFoldable(மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        எனில் (isSame(lhsp, rhsp)) திரும்பு உண்மை;
        எனில் (மாறிலி DfgRep* மாறிலி lRepp = lhsp->cast<DfgRep>()) {
            எனில் (isSame(lRepp->srcp(), rhsp)) திரும்பு உண்மை;
        }
        எனில் (மாறிலி DfgRep* மாறிலி rRepp = rhsp->cast<DfgRep>()) {
            எனில் (isSame(lhsp, rRepp->srcp())) திரும்பு உண்மை;
        }
        // Concatenation of adjoining selects is a select
        எனில் (மாறிலி DfgSel* மாறிலி lSelp = lhsp->cast<DfgSel>()) {
            எனில் (மாறிலி DfgSel* மாறிலி rSelp = rhsp->cast<DfgSel>()) {
                திரும்பு isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        எனில் (மாறிலி DfgNot* மாறிலி lNotp = lhsp->cast<DfgNot>()) {
            எனில் (மாறிலி DfgNot* மாறிலி rNotp = rhsp->cast<DfgNot>()) {
                திரும்பு !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        திரும்பு பொய்;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    நிலையான தருக்க isSameBitwise(மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) {
        எனில் (lhsp->type() != rhsp->type()) திரும்பு பொய்;
        திரும்பு lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    நிலையான தருக்க isConcatSimplifiable(மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp) {
        எனில் (isConcatFoldable(lhsp, rhsp)) திரும்பு உண்மை;
        எனில் (!isSameBitwise(lhsp, rhsp)) திரும்பு பொய்;
        எனில் (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) திரும்பு பொய்;
        மாறிலி Dfg_Vertex_இருமம்* மாறிலி lp = lhsp->as<Dfg_Vertex_இருமம்>();
        மாறிலி Dfg_Vertex_இருமம்* மாறிலி rp = rhsp->as<Dfg_Vertex_இருமம்>();
        எனில் (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            திரும்பு உண்மை;
        எனில் (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            திரும்பு உண்மை;
        திரும்பு பொய்;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    நிலையான தருக்க isConcatPushableThroughBitwise(மாறிலி DfgVertex* lhsp, மாறிலி DfgVertex* rhsp,
                                               தருக்க& swap) {
        எனில் (!isSameBitwise(lhsp, rhsp)) திரும்பு பொய்;
        எனில் (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) திரும்பு பொய்;
        மாறிலி Dfg_Vertex_இருமம்* மாறிலி lp = lhsp->as<Dfg_Vertex_இருமம்>();
        மாறிலி Dfg_Vertex_இருமம்* மாறிலி rp = rhsp->as<Dfg_Vertex_இருமம்>();
        swap = பொய்;
        எனில் (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) திரும்பு உண்மை;
        எனில் (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) திரும்பு உண்மை;
        swap = உண்மை;
        எனில் (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) திரும்பு உண்மை;
        எனில் (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) திரும்பு உண்மை;
        திரும்பு பொய்;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_வரி* flp, மாறிலி DfgVertex* lhsp,
                                              மாறிலி DfgVertex* rhsp, தருக்க swap) {
        மாறிலி Dfg_Vertex_இருமம்* மாறிலி lp = lhsp->as<Dfg_Vertex_இருமம்>();
        மாறிலி Dfg_Vertex_இருமம்* மாறிலி rp = rhsp->as<Dfg_Vertex_இருமம்>();
        மாறிலி DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* மாறிலி cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* மாறிலி dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* மாறிலி newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* மாறிலி newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        எனில் (lp->is<DfgAnd>()) திரும்பு make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        எனில் (lp->is<DfgOr>()) திரும்பு make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        திரும்பு make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();

        எனில் (DfgSel* மாறிலி lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            எனில் (DfgSel* மாறிலி selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } இல்லையெனில் எனில் (Bitwise* மாறிலி rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } இல்லையெனில் எனில் (Reduction* மாறிலி rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            எனில் (rSelp) {
                uint32_t lsb = 0;
                எனில் (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        மாறிலி DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* மாறிலி newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        எனில் (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        திரும்பு உண்மை;
                    }
                }
            }
        }

        எனில் (Reduction* மாறிலி lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            எனில் (!rRedp) {
                எனில் (Bitwise* மாறிலி rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            எனில் (rRedp) {
                எனில் (DfgSel* மாறிலி lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    எனில் (DfgSel* மாறிலி rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        எனில் (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                மாறிலி DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* மாறிலி newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                எனில் (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                திரும்பு உண்மை;
                            }
                        }
                    }
                }
            }
        }

        திரும்பு பொய்;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க optimizeReduction(Reduction* மாறிலி vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        எனில் (foldUnary(vtxp)) திரும்பு உண்மை;

        DfgVertex* மாறிலி srcp = vtxp->srcp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        // Reduction of 1-bit value
        எனில் (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                திரும்பு உண்மை;
            }
        }

        எனில் (DfgCond* மாறிலி condp = srcp->cast<DfgCond>()) {
            எனில் (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(தள்ளு_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* மாறிலி newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* மாறிலி newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* மாறிலி newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    திரும்பு உண்மை;
                }
            }
        }

        எனில் (DfgConcat* மாறிலி concatp = srcp->cast<DfgConcat>()) {
            எனில் (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(தள்ளு_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* மாறிலி lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* மாறிலி rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    திரும்பு உண்மை;
                }
            }
        }

        எனில் (Bitwise* மாறிலி bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            எனில் (!bitwisep->hasMultipleSinks()) {
                எனில் (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(தள்ளு_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* மாறிலி newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* மாறிலி newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        திரும்பு உண்மை;
                    }
                }

                எனில் (DfgSel* மாறிலி lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    எனில் (DfgSel* மாறிலி rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        எனில் (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(தள்ளு_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                மாறிலி DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* மாறிலி newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                திரும்பு உண்மை;
                            }
                        }
                    }
                }
            }
        }

        திரும்பு பொய்;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT தருக்க optimizeShiftRHS(Shift* மாறிலி vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_இருமம், Shift>::மதிப்பு, "Must invoke on binary");
        static_assert(std::is_final<Shift>::மதிப்பு, "Must invoke on final class");
        எனில் (மாறிலி DfgConcat* மாறிலி concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            எனில் (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    திரும்பு உண்மை;
                }
            }
        }
        திரும்பு பொய்;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_மொத்தம்_Ones_Term(DfgVertex* vtxp) {
        எனில் (DfgConcat* மாறிலி oCatp = vtxp->cast<DfgConcat>()) {
            எனில் (isZero(oCatp->lhsp())) {
                எனில் (Dfg_மொத்தம்_Ones* மாறிலி மொத்தம்_Onesp = oCatp->rhsp()->cast<Dfg_மொத்தம்_Ones>()) {
                    // Zero extended count ones
                    எனில் (DfgSel* மாறிலி selp = மொத்தம்_Onesp->srcp()->cast<DfgSel>()) {
                        திரும்பு {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } இல்லையெனில் எனில் (DfgSel* மாறிலி selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    எனில் (selp->dtype() == m_bitDType) {  //
                        திரும்பு {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            திரும்பு {nullptr, 0, 0};
        }
        எனில் (Dfg_மொத்தம்_Ones* மாறிலி மொத்தம்_Onesp = vtxp->cast<Dfg_மொத்தம்_Ones>()) {
            // Simple count ones
            எனில் (DfgSel* மாறிலி selp = மொத்தம்_Onesp->srcp()->cast<DfgSel>()) {
                திரும்பு {selp->fromp(), selp->lsb(), selp->width()};
            }
            திரும்பு {nullptr, 0, 0};
        }
        எனில் (DfgSel* மாறிலி oSelp = vtxp->cast<DfgSel>()) {
            எனில் (oSelp->lsb() == 0) {
                // Truncated count ones
                எனில் (Dfg_மொத்தம்_Ones* மாறிலி மொத்தம்_Onesp = oSelp->fromp()->cast<Dfg_மொத்தம்_Ones>()) {
                    // Zero extended count ones
                    எனில் (DfgSel* மாறிலி selp = மொத்தம்_Onesp->srcp()->cast<DfgSel>()) {
                        திரும்பு {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            எனில் (oSelp->dtype() == m_bitDType) {  //
                திரும்பு {oSelp->fromp(), oSelp->lsb(), 1};
            }
            திரும்பு {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        எனில் (DfgShiftR* மாறிலி shiftrp = vtxp->cast<DfgShiftR>()) {
            எனில் (DfgConst* மாறிலி rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                எனில் (rConstp->toU32() == shiftrp->width() - 1) {
                    திரும்பு {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            திரும்பு {nullptr, 0, 0};
        }
        // Not applicable
        திரும்பு {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              மாறிலி uint32_t width) {
        போது (உண்மை) {
            மாறிலி uint32_t msb = lsb + width - 1;

            // Sel from Concat
            எனில் (DfgConcat* மாறிலி concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* மாறிலி lhsp = concatp->lhsp();
                DfgVertex* மாறிலி rhsp = concatp->rhsp();

                எனில் (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        தொடர்;
                    }
                } இல்லையெனில் எனில் (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        தொடர்;
                    }
                }
            }

            எனில் (DfgRep* மாறிலி repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                மாறிலி uint32_t srcWidth = repp->srcp()->width();
                எனில் (width <= srcWidth) {
                    மாறிலி uint32_t newLsb = lsb % srcWidth;
                    மாறிலி uint32_t newMsb = newLsb + width - 1;
                    எனில் (newMsb < srcWidth) {
                        APPLYING(தள்ளு_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            தொடர்;
                        }
                    }
                }
            }

            // Sel from Sel
            எனில் (DfgSel* மாறிலி selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    தொடர்;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            எனில் (DfgVarPacked* மாறிலி varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                மாறிலி auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* மாறிலி driverp = pair.first;
                மாறிலி uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                எனில் (driverp) {
                    எனில் (driverp == varp->defaultp()) {
                        APPLYING(தள்ளு_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            தொடர்;
                        }
                    } இல்லையெனில் {
                        APPLYING(தள்ளு_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            தொடர்;
                        }
                    }
                }
            }

            // No patterns matched, stop
            நிறுத்து;
        }
        திரும்பு {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        எனில் (ap == bp) திரும்பு {ap, ap->width()};

        // If both constants, check LSBs
        எனில் (DfgConst* மாறிலி aConstp = ap->cast<DfgConst>()) {
            எனில் (DfgConst* மாறிலி bConstp = bp->cast<DfgConst>()) {
                மாறிலி V3_எண்& a_இலக்கம் = aConstp->இலக்கம்();
                மாறிலி V3_எண்& b_இலக்கம் = bConstp->இலக்கம்();
                // Max match is the shorter constant
                மாறிலி uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                க்கு (; matchWidth < maxMatch; ++matchWidth) {
                    எனில் (a_இலக்கம்.bitIs0(matchWidth) != b_இலக்கம்.bitIs0(matchWidth)) நிறுத்து;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* மாறிலி shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                திரும்பு {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        எனில் (DfgConcat* மாறிலி catp = ap->cast<DfgConcat>()) திரும்பு commonLSBs(catp->rhsp(), bp);
        எனில் (DfgConcat* மாறிலி catp = bp->cast<DfgConcat>()) திரும்பு commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        எனில் (DfgSel* மாறிலி selp = ap->cast<DfgSel>()) {
            எனில் (selp->lsb() == 0) {
                DfgVertex* மாறிலி fromp = selp->fromp();
                மாறிலி std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                திரும்பு {common.first, std::min(common.second, selp->width())};
            }
        }
        எனில் (DfgSel* மாறிலி selp = bp->cast<DfgSel>()) {
            எனில் (selp->lsb() == 0) {
                DfgVertex* மாறிலி fromp = selp->fromp();
                மாறிலி std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                திரும்பு {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        திரும்பு {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        எனில் (ap == bp) திரும்பு {ap, ap->width()};

        // If both constants, check MSBs
        எனில் (DfgConst* மாறிலி aConstp = ap->cast<DfgConst>()) {
            எனில் (DfgConst* மாறிலி bConstp = bp->cast<DfgConst>()) {
                மாறிலி uint32_t aMsb = aConstp->width() - 1;
                மாறிலி uint32_t bMsb = bConstp->width() - 1;
                மாறிலி V3_எண்& a_இலக்கம் = aConstp->இலக்கம்();
                மாறிலி V3_எண்& b_இலக்கம் = bConstp->இலக்கம்();
                // Max match is the shorter constant
                மாறிலி uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                க்கு (; matchWidth < maxMatch; ++matchWidth) {
                    எனில் (a_இலக்கம்.bitIs0(aMsb - matchWidth) != b_இலக்கம்.bitIs0(bMsb - matchWidth)) நிறுத்து;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* மாறிலி shorterp = aMsb < bMsb ? aConstp : bConstp;
                திரும்பு {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        எனில் (DfgConcat* மாறிலி catp = ap->cast<DfgConcat>()) திரும்பு commonMSBs(catp->lhsp(), bp);
        எனில் (DfgConcat* மாறிலி catp = bp->cast<DfgConcat>()) திரும்பு commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        எனில் (DfgSel* மாறிலி selp = ap->cast<DfgSel>()) {
            DfgVertex* மாறிலி fromp = selp->fromp();
            எனில் (selp->msb() == fromp->width() - 1) {
                மாறிலி std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                திரும்பு {common.first, std::min(common.second, selp->width())};
            }
        }
        எனில் (DfgSel* மாறிலி selp = bp->cast<DfgSel>()) {
            DfgVertex* மாறிலி fromp = selp->fromp();
            எனில் (selp->msb() == fromp->width() - 1) {
                மாறிலி std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                திரும்பு {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        திரும்பு {nullptr, 0};
    }

    // VISIT methods

    வெறுமை visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    வெறுமை visit(Dfg_மொத்தம்_Ones* மாறிலி vtxp) override {
        எனில் (foldUnary(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgExtend* மாறிலி vtxp) override {
        எனில் (foldUnary(vtxp)) திரும்பு;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* மாறிலி zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            திரும்பு;
        }
    }

    வெறுமை visit(DfgExtendS* மாறிலி vtxp) override {
        எனில் (foldUnary(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgLogNot* மாறிலி vtxp) override {
        எனில் (foldUnary(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgNegate* மாறிலி vtxp) override {
        எனில் (foldUnary(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgNot* மாறிலி vtxp) override {
        எனில் (foldUnary(vtxp)) திரும்பு;

        // Not of Cond
        எனில் (DfgCond* மாறிலி condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            எனில் (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(தள்ளு_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* மாறிலி newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* மாறிலி newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* மாறிலி newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    திரும்பு;
                }
            }
        }

        // Not of Not
        எனில் (DfgNot* மாறிலி notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                திரும்பு;
            }
        }

        எனில் (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            எனில் (DfgEq* மாறிலி eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    திரும்பு;
                }
            }

            // Not of Neq
            எனில் (DfgNeq* மாறிலி neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    திரும்பு;
                }
            }

            // Not of unsigned comparisons
            எனில் (DfgLt* மாறிலி p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    திரும்பு;
                }
            }
            எனில் (DfgGte* மாறிலி p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    திரும்பு;
                }
            }
            எனில் (DfgGt* மாறிலி p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    திரும்பு;
                }
            }
            எனில் (DfgLte* மாறிலி p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    திரும்பு;
                }
            }

            // Not of signed comparisons
            எனில் (DfgLtS* மாறிலி p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    திரும்பு;
                }
            }
            எனில் (DfgGteS* மாறிலி p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    திரும்பு;
                }
            }
            எனில் (DfgGtS* மாறிலி p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    திரும்பு;
                }
            }
            எனில் (DfgLteS* மாறிலி p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgOneHot* மாறிலி vtxp) override {
        எனில் (foldUnary(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgOneHot0* மாறிலி vtxp) override {
        எனில் (foldUnary(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgRedOr* மாறிலி vtxp) override {
        எனில் (optimizeReduction(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgRedAnd* மாறிலி vtxp) override {
        எனில் (optimizeReduction(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgRedXor* மாறிலி vtxp) override {
        எனில் (optimizeReduction(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgSel* மாறிலி vtxp) override {
        DfgVertex* மாறிலி fromp = vtxp->fromp();

        File_வரி* மாறிலி flp = vtxp->fileline();

        மாறிலி uint32_t lsb = vtxp->lsb();
        மாறிலி uint32_t width = vtxp->width();
        மாறிலி uint32_t msb = lsb + width - 1;

        எனில் (DfgConst* மாறிலி constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* மாறிலி resp = makeZero(flp, width);
                resp->இலக்கம்().opSel(constp->இலக்கம்(), msb, lsb);
                replace(resp);
                திரும்பு;
            }
        }

        // Full width select, replace with the source.
        எனில் (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                திரும்பு;
            }
        }

        // Unwind through bit packing in one go
        {
            மாறிலி auto res = unwindSel(fromp, lsb, width);
            எனில் (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                திரும்பு;
            }
        }

        // Sel from Not
        எனில் (DfgNot* மாறிலி notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            எனில் (!notp->hasMultipleSinks()) {
                APPLYING(தள்ளு_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* மாறிலி newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    திரும்பு;
                }
            }
        }

        // Sel from Cond
        எனில் (DfgCond* மாறிலி condp = fromp->cast<DfgCond>()) {
            எனில் (!condp->hasMultipleSinks()) {
                APPLYING(தள்ளு_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* மாறிலி newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* மாறிலி newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* மாறிலி newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    திரும்பு;
                }
            }
        }

        // Sel from ShiftL
        எனில் (DfgShiftL* மாறிலி shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            எனில் (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(தள்ளு_SEL_THROUGH_SHIFTL) {
                    DfgSel* மாறிலி newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgMux* மாறிலி vtxp) override {
        DfgVertex* மாறிலி fromp = vtxp->fromp();
        DfgVertex* மாறிலி lsbp = vtxp->lsbp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (DfgConst* மாறிலி lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->இலக்கம்().toUInt()));
                திரும்பு;
            }
        }

        எனில் (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                திரும்பு;
            }
        }

        எனில் (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                திரும்பு;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    வெறுமை visit(DfgAnd* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        எனில் (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            எனில் (DfgNot* மாறிலி lhsNotp = lhsp->cast<DfgNot>()) {
                எனில் (DfgNot* மாறிலி rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* மாறிலி orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        திரும்பு;
                    }
                }
                எனில் (DfgNeq* மாறிலி rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* மாறிலி newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* மாறிலி orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (DfgConst* மாறிலி lConstp = lhsp->cast<DfgConst>()) {
            எனில் (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    திரும்பு;
                }
            }

            எனில் (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    திரும்பு;
                }
            }

            எனில் (DfgConcat* மாறிலி rhsConcatp = rhsp->cast<DfgConcat>()) {
                எனில் (try_தள்ளு_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) திரும்பு;
            }
        }

        எனில் (distributive_And_Associative_இருமம்<DfgOr, DfgAnd>(vtxp)) திரும்பு;

        எனில் (try_தள்ளு_Bitwise_Op_Through_Reductions(vtxp)) திரும்பு;

        எனில் (try_தள்ளு_Bitwise_Op_Throug_Sel(vtxp)) திரும்பு;

        {
            DfgNot* மாறிலி lNotp = lhsp->cast<DfgNot>();
            DfgNot* மாறிலி rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            எனில் ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    திரும்பு;
                }
            }

            எனில் (DfgAnd* மாறிலி rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* மாறிலி rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* மாறிலி rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                எனில் ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (vtxp->dtype() == m_bitDType) {
            எனில் (tryReplaceBitwiseWithReduction(vtxp)) திரும்பு;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            எனில் (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            எனில் (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* மாறிலி newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgOr* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        எனில் (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            எனில் (DfgNot* மாறிலி lhsNotp = lhsp->cast<DfgNot>()) {
                எனில் (DfgNot* மாறிலி rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* மாறிலி andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        திரும்பு;
                    }
                }
                எனில் (DfgNeq* மாறிலி rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* மாறிலி newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* மாறிலி andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (DfgConcat* மாறிலி lhsConcatp = lhsp->cast<DfgConcat>()) {
            எனில் (DfgConcat* மாறிலி rhsConcatp = rhsp->cast<DfgConcat>()) {
                எனில் (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    எனில் (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            திரும்பு;
                        }
                    }
                    எனில் (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            திரும்பு;
                        }
                    }
                }
            }
        }

        எனில் (DfgConst* மாறிலி lConstp = lhsp->cast<DfgConst>()) {
            எனில் (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    திரும்பு;
                }
            }

            எனில் (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    திரும்பு;
                }
            }

            எனில் (DfgConcat* மாறிலி rhsConcatp = rhsp->cast<DfgConcat>()) {
                எனில் (try_தள்ளு_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) திரும்பு;
            }
        }

        எனில் (distributive_And_Associative_இருமம்<DfgAnd, DfgOr>(vtxp)) திரும்பு;

        எனில் (try_தள்ளு_Bitwise_Op_Through_Reductions(vtxp)) திரும்பு;

        எனில் (try_தள்ளு_Bitwise_Op_Throug_Sel(vtxp)) திரும்பு;

        {
            DfgNot* மாறிலி lNotp = lhsp->cast<DfgNot>();
            DfgNot* மாறிலி rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            எனில் ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    திரும்பு;
                }
            }

            எனில் (DfgOr* மாறிலி rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* மாறிலி rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* மாறிலி rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                எனில் ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (vtxp->dtype() == m_bitDType) {
            எனில் (tryReplaceBitwiseWithReduction(vtxp)) திரும்பு;
        }
    }

    வெறுமை visit(DfgXor* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                திரும்பு;
            }
        }

        எனில் (DfgConst* மாறிலி lConstp = lhsp->cast<DfgConst>()) {
            எனில் (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    திரும்பு;
                }
            }
            எனில் (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    திரும்பு;
                }
            }
            எனில் (DfgConcat* மாறிலி rConcatp = rhsp->cast<DfgConcat>()) {
                எனில் (try_தள்ளு_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) திரும்பு;
            }
        }

        எனில் (try_தள்ளு_Bitwise_Op_Through_Reductions(vtxp)) திரும்பு;

        எனில் (try_தள்ளு_Bitwise_Op_Throug_Sel(vtxp)) திரும்பு;

        எனில் (vtxp->dtype() == m_bitDType) {
            எனில் (tryReplaceBitwiseWithReduction(vtxp)) திரும்பு;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    வெறுமை visit(DfgAdd* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                திரும்பு;
            }
        }

        மாறிலி std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_மொத்தம்_Ones_Term(lhsp);
        எனில் (DfgVertex* மாறிலி lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_மொத்தம்_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            எனில் (!std::get<0>(rTerm)) {
                எனில் (DfgAdd* மாறிலி rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_மொத்தம்_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            எனில் (DfgVertex* மாறிலி rVtxp = std::get<0>(rTerm)) {
                எனில் (isSame(lVtxp, rVtxp)) {
                    மாறிலி uint32_t lLsb = std::get<1>(lTerm);
                    மாறிலி uint32_t rLsb = std::get<1>(rTerm);
                    மாறிலி uint32_t lWidth = std::get<2>(lTerm);
                    மாறிலி uint32_t rWidth = std::get<2>(rTerm);
                    தருக்க adjoined = உண்மை;
                    uint32_t lsb = 0;
                    எனில் (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } இல்லையெனில் எனில் (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } இல்லையெனில் {
                        adjoined = பொய்;
                    }
                    எனில் (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_மொத்தம்_ONES) {
                            DfgSel* மாறிலி selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_மொத்தம்_Ones>(flp, DfgDataType::packed(32), selp);
                            எனில் (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } இல்லையெனில் எனில் (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            எனில் (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            திரும்பு;
                        }
                    }
                }
            }
        }
    }

    வெறுமை visit(DfgArraySel* மாறிலி vtxp) override {
        DfgConst* மாறிலி idxp = vtxp->bitp()->cast<DfgConst>();
        எனில் (!idxp) திரும்பு;
        DfgVarArray* மாறிலி varp = vtxp->fromp()->cast<DfgVarArray>();
        எனில் (!varp) திரும்பு;
        AstVar* மாறிலி astVarp = varp->vscp()->varp();
        எனில் (astVarp->isForced()) திரும்பு;
        எனில் (astVarp->isSigUserRWPublic()) திரும்பு;
        DfgVertex* மாறிலி srcp = varp->srcp();
        எனில் (!srcp) {
            எனில் (vtxp->isPacked()) {
                எனில் (AstConsArray* மாறிலி iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    எனில் (AstConst* மாறிலி valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(புதிய DfgConst{m_dfg, valp->fileline(), valp->இலக்கம்()});
                            திரும்பு;
                        }
                    }
                }
            }
            திரும்பு;
        }

        எனில் (DfgSpliceArray* மாறிலி splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* மாறிலி driverp = splicep->driverAt(idxp->toSizeT());
            எனில் (!driverp) திரும்பு;
            DfgUnitArray* மாறிலி uap = driverp->cast<DfgUnitArray>();
            எனில் (!uap) திரும்பு;
            எனில் (uap->srcp()->is<DfgVertexSplice>()) திரும்பு;
            // If driven by a variable that had a Driver in DFG, it is partial
            எனில் (DfgVertexVar* மாறிலி dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                எனில் (dvarp->srcp()) திரும்பு;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                திரும்பு;
            }
        }

        எனில் (DfgUnitArray* மாறிலி uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            எனில் (uap->srcp()->is<DfgSplicePacked>()) திரும்பு;
            // If driven by a variable that had a Driver in DFG, it is partial
            எனில் (DfgVertexVar* மாறிலி dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                எனில் (dvarp->srcp()) திரும்பு;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgConcat* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();

        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isZero(lhsp)) {
            DfgConst* மாறிலி lConstp = lhsp->as<DfgConst>();
            எனில் (DfgSel* மாறிலி rSelp = rhsp->cast<DfgSel>()) {
                எனில் (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_உச்சி_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (isZero(rhsp)) {
            DfgConst* மாறிலி rConstp = rhsp->as<DfgConst>();
            எனில் (DfgSel* மாறிலி lSelp = lhsp->cast<DfgSel>()) {
                எனில் (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (DfgNot* மாறிலி lNot = lhsp->cast<DfgNot>()) {
            எனில் (DfgNot* மாறிலி rNot = rhsp->cast<DfgNot>()) {
                எனில் (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(தள்ளு_CONCAT_THROUGH_NOTS) {
                        DfgConcat* மாறிலி newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        திரும்பு;
                    }
                }
            }
        }

        {
            தருக்க swap = பொய்;
            எனில் (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(தள்ளு_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    திரும்பு;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            எனில் (DfgConcat* மாறிலி rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* மாறிலி rlp = rConcatp->lhsp();
                எனில் (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(தள்ளு_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* மாறிலி newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (DfgSel* மாறிலி lSelp = lhsp->cast<DfgSel>()) {
            எனில் (DfgSel* மாறிலி rSelp = rhsp->cast<DfgSel>()) {
                எனில் (isSame(lSelp->fromp(), rSelp->fromp())) {
                    எனில் (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            திரும்பு;
                        }
                    }
                }
            }

            எனில் (DfgConcat* மாறிலி rConcatp = rhsp->cast<DfgConcat>()) {
                எனில் (DfgSel* மாறிலி rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    எனில் (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        எனில் (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                மாறிலி uint32_t width = lSelp->width() + rlSelp->width();
                                மாறிலி DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* மாறிலி selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                திரும்பு;
                            }
                        }
                    }
                }
            }
        }

        எனில் (DfgSel* மாறிலி rSelp = rhsp->cast<DfgSel>()) {
            எனில் (DfgConcat* மாறிலி lConcatp = lhsp->cast<DfgConcat>()) {
                எனில் (DfgSel* மாறிலி lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    எனில் (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        எனில் (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                மாறிலி uint32_t width = lrSelp->width() + rSelp->width();
                                மாறிலி DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* மாறிலி selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                திரும்பு;
                            }
                        }
                    }
                }
            }
        }

        எனில் (DfgConst* மாறிலி lConstp = lhsp->cast<DfgConst>()) {
            எனில் (DfgCond* மாறிலி rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* மாறிலி rtVtxp = rCondp->thenp();
                DfgVertex* மாறிலி reVtxp = rCondp->elsep();
                DfgConst* மாறிலி rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* மாறிலி reConstp = reVtxp->cast<DfgConst>();
                எனில் (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(தள்ளு_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* மாறிலி thenp = [&]() -> DfgVertex* {
                            File_வரி* மாறிலி rtFlp = rtVtxp->fileline();
                            எனில் (rtConstp) {
                                DfgConst* மாறிலி constp = makeZero(rtFlp, vtxp->width());
                                constp->இலக்கம்().opConcat(lConstp->இலக்கம்(), rtConstp->இலக்கம்());
                                திரும்பு constp;
                            }
                            திரும்பு make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* மாறிலி elsep = [&]() -> DfgVertex* {
                            File_வரி* மாறிலி reFlp = reVtxp->fileline();
                            எனில் (reConstp) {
                                DfgConst* மாறிலி constp = makeZero(reFlp, vtxp->width());
                                constp->இலக்கம்().opConcat(lConstp->இலக்கம்(), reConstp->இலக்கம்());
                                திரும்பு constp;
                            }
                            திரும்பு make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (DfgConst* மாறிலி rConstp = rhsp->cast<DfgConst>()) {
            எனில் (DfgCond* மாறிலி lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* மாறிலி ltVtxp = lCondp->thenp();
                DfgVertex* மாறிலி leVtxp = lCondp->elsep();
                DfgConst* மாறிலி ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* மாறிலி leConstp = leVtxp->cast<DfgConst>();
                எனில் (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(தள்ளு_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* மாறிலி thenp = [&]() -> DfgVertex* {
                            File_வரி* மாறிலி ltFlp = ltVtxp->fileline();
                            எனில் (ltConstp) {
                                DfgConst* மாறிலி constp = makeZero(ltFlp, vtxp->width());
                                constp->இலக்கம்().opConcat(ltConstp->இலக்கம்(), rConstp->இலக்கம்());
                                திரும்பு constp;
                            }
                            திரும்பு make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* மாறிலி elsep = [&]() -> DfgVertex* {
                            File_வரி* மாறிலி leFlp = leVtxp->fileline();
                            எனில் (leConstp) {
                                DfgConst* மாறிலி constp = makeZero(leFlp, vtxp->width());
                                constp->இலக்கம்().opConcat(leConstp->இலக்கம்(), rConstp->இலக்கம்());
                                திரும்பு constp;
                            }
                            திரும்பு make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        திரும்பு;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            மாறிலி uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            மாறிலி uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            மாறிலி uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            தருக்க hasCrossSink = பொய்;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                எனில் (மாறிலி DfgSel* மாறிலி selp = sink.cast<DfgSel>()) {
                    மாறிலி uint32_t selLsb = selp->lsb();
                    மாறிலி uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    திரும்பு பொய்;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                எனில் (மாறிலி DfgVarPacked* மாறிலி varp = sink.cast<DfgVarPacked>()) {
                    எனில் (!varp->hasSinks() && !varp->isObserved()) திரும்பு பொய்;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                திரும்பு உண்மை;
            });
            எனில் (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    எனில் (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    எனில் (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* மாறிலி catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* மாறிலி sp = புதிய DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* மாறிலி varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(உண்மை);
                    varp->srcp(sp);
                    replace(varp);
                    திரும்பு;
                }
            }
        }

        // Convert various forms to REPLICATE
        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                திரும்பு;
            }
        }
        எனில் (DfgRep* மாறிலி lRepp = lhsp->cast<DfgRep>()) {
            எனில் (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    திரும்பு;
                }
            }
        }
        எனில் (DfgRep* மாறிலி rRepp = rhsp->cast<DfgRep>()) {
            எனில் (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    திரும்பு;
                }
            }
        }
        எனில் (DfgRep* மாறிலி lRepp = lhsp->cast<DfgRep>()) {
            எனில் (DfgRep* மாறிலி rRepp = rhsp->cast<DfgRep>()) {
                எனில் (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        திரும்பு;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            எனில் (DfgConcat* மாறிலி rConcatp = rhsp->cast<DfgConcat>()) {
                எனில் (DfgRep* மாறிலி rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    எனில் (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            மாறிலி uint32_t width = lRepp->width() + rlRepp->width();
                            மாறிலி DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* மாறிலி repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            திரும்பு;
                        }
                    }
                }
            }
        }
    }

    வெறுமை visit(DfgDiv* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        எனில் (DfgConst* மாறிலி rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            எனில் (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgDivS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        எனில் (DfgConst* மாறிலி rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            எனில் (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgGt* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgGtS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgGte* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgGteS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgLogAnd* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp) || vtxp->rhsp()->unsafe()) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();

        எனில் (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgLogEq* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgLogIf* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp) || vtxp->rhsp()->unsafe()) திரும்பு;
    }

    வெறுமை visit(DfgLogOr* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp) || vtxp->rhsp()->unsafe()) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();

        எனில் (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgLt* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgLtS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgLte* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgLteS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                திரும்பு;
            }
        }
    }

    வெறுமை visit(DfgModDiv* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgModDivS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgMul* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (DfgConst* மாறிலி lConstp = lhsp->cast<DfgConst>()) {
            எனில் (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    திரும்பு;
                }
            }
            எனில் (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgMulS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (DfgConst* மாறிலி lConstp = lhsp->cast<DfgConst>()) {
            எனில் (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    திரும்பு;
                }
            }
            எனில் (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgEq* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                திரும்பு;
            }
        }

        எனில் (DfgConst* மாறிலி lhsConstp = lhsp->cast<DfgConst>()) {
            எனில் (DfgConcat* மாறிலி rhsConcatp = rhsp->cast<DfgConcat>()) {
                எனில் (try_தள்ளு_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) திரும்பு;
            }

            எனில் (rhsp->dtype() == m_bitDType) {
                எனில் (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        திரும்பு;
                    }
                }

                எனில் (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        திரும்பு;
                    }
                }
            }
        }
    }

    வெறுமை visit(DfgNeq* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                திரும்பு;
            }
        }

        எனில் (DfgConst* மாறிலி lhsConstp = lhsp->cast<DfgConst>()) {
            எனில் (DfgConcat* மாறிலி rhsConcatp = rhsp->cast<DfgConcat>()) {
                எனில் (try_தள்ளு_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) திரும்பு;
            }

            எனில் (rhsp->dtype() == m_bitDType) {
                எனில் (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        திரும்பு;
                    }
                }

                எனில் (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        திரும்பு;
                    }
                }
            }
        }
    }

    வெறுமை visit(DfgPow* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgPowSS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgPowSU* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgPowUS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgRep* மாறிலி vtxp) override {
        DfgVertex* மாறிலி srcp = vtxp->srcp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (DfgConst* மாறிலி sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* மாறிலி resultp = makeZero(flp, vtxp->width());
                resultp->இலக்கம்().opRepl(sConstp->இலக்கம்(), vtxp->மொத்தம்());
                replace(resultp);
                திரும்பு;
            }
        }

        எனில் (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                திரும்பு;
            }
        }

        எனில் (DfgRep* மாறிலி sRepp = srcp->cast<DfgRep>()) {
            எனில் (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgShiftL* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
        எனில் (optimizeShiftRHS(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();

        எனில் (DfgShiftL* மாறிலி lShiftLp = lhsp->cast<DfgShiftL>()) {
            எனில் (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_வரி* மாறிலி flp = vtxp->fileline();
                    மாறிலி DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* மாறிலி bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* மாறிலி cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* மாறிலி addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    திரும்பு;
                }
            }
        }

        எனில் (DfgConst* மாறிலி rConstp = rhsp->cast<DfgConst>()) {
            மாறிலி uint32_t shiftAmount = rConstp->toU32();

            எனில் (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    திரும்பு;
                }
            }

            எனில் (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    திரும்பு;
                }
            }

            எனில் (DfgConcat* மாறிலி lConcatp = lhsp->cast<DfgConcat>()) {
                எனில் (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* மாறிலி lRhsp = lConcatp->rhsp();
                        DfgVertex* மாறிலி lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        மாறிலி uint32_t rW = shiftAmount;
                        மாறிலி uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        மாறிலி uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_வரி* மாறிலி flp = lConcatp->fileline();
                        DfgVertex* மாறிலி rp = makeZero(flp, shiftAmount);
                        DfgVertex* மாறிலி mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        எனில் (!lW) {
                            replace(np);
                            திரும்பு;
                        }
                        DfgVertex* மாறிலி lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        திரும்பு;
                    }
                }
            }

            எனில் (DfgSel* மாறிலி lSelp = lhsp->cast<DfgSel>()) {
                எனில் (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        மாறிலி uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* மாறிலி nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        திரும்பு;
                    }
                }
            }

            எனில் (DfgCond* மாறிலி lCondp = lhsp->cast<DfgCond>()) {
                எனில் (!lCondp->hasMultipleSinks()) {
                    APPLYING(தள்ளு_SHIFTL_THROUGH_COND) {
                        DfgShiftL* மாறிலி tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* மாறிலி ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        திரும்பு;
                    }
                }
            }
        }
    }

    வெறுமை visit(DfgShiftR* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
        எனில் (optimizeShiftRHS(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();

        எனில் (DfgShiftR* மாறிலி lShiftRp = lhsp->cast<DfgShiftR>()) {
            எனில் (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_வரி* மாறிலி flp = vtxp->fileline();
                    மாறிலி DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* மாறிலி bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* மாறிலி cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* மாறிலி addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    திரும்பு;
                }
            }
        }

        எனில் (DfgConst* மாறிலி rConstp = rhsp->cast<DfgConst>()) {
            மாறிலி uint32_t shiftAmount = rConstp->toU32();

            எனில் (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    திரும்பு;
                }
            }

            எனில் (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    திரும்பு;
                }
            }

            எனில் (DfgConcat* மாறிலி lConcatp = lhsp->cast<DfgConcat>()) {
                எனில் (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* மாறிலி lRhsp = lConcatp->rhsp();
                        DfgVertex* மாறிலி lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        மாறிலி uint32_t lW = shiftAmount;
                        மாறிலி uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        மாறிலி uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_வரி* மாறிலி flp = lConcatp->fileline();
                        DfgVertex* மாறிலி lp = makeZero(flp, shiftAmount);
                        DfgVertex* மாறிலி mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        எனில் (!rW) {
                            replace(np);
                            திரும்பு;
                        }
                        DfgVertex* மாறிலி rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        திரும்பு;
                    }
                }
            }

            எனில் (DfgSel* மாறிலி lSelp = lhsp->cast<DfgSel>()) {
                எனில் (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        மாறிலி uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* மாறிலி nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        திரும்பு;
                    }
                }
            }

            எனில் (DfgCond* மாறிலி lCondp = lhsp->cast<DfgCond>()) {
                எனில் (!lCondp->hasMultipleSinks()) {
                    APPLYING(தள்ளு_SHIFTR_THROUGH_COND) {
                        DfgShiftR* மாறிலி tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* மாறிலி ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        திரும்பு;
                    }
                }
            }
        }
    }

    வெறுமை visit(DfgShiftRS* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;
        எனில் (optimizeShiftRHS(vtxp)) திரும்பு;
    }

    வெறுமை visit(DfgSub* மாறிலி vtxp) override {
        எனில் (இருமம்(vtxp)) திரும்பு;

        DfgVertex* மாறிலி lhsp = vtxp->lhsp();
        DfgVertex* மாறிலி rhsp = vtxp->rhsp();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                திரும்பு;
            }
        }

        எனில் (DfgConst* மாறிலி rConstp = rhsp->cast<DfgConst>()) {
            எனில் (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    திரும்பு;
                }
            }
            எனில் (vtxp->dtype() == m_bitDType && rConstp->has_மதிப்பு(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgMatchMasked* மாறிலி vtxp) override {
        எனில் (DfgConst* மாறிலி constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* மாறிலி matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->இலக்கம்(), matchVarp)));
                திரும்பு;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    வெறுமை visit(DfgCond* மாறிலி vtxp) override {
        DfgVertex* மாறிலி condp = vtxp->condp();
        DfgVertex* மாறிலி thenp = vtxp->thenp();
        DfgVertex* மாறிலி elsep = vtxp->elsep();
        File_வரி* மாறிலி flp = vtxp->fileline();

        எனில் (condp->dtype() != m_bitDType) திரும்பு;

        எனில் (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                திரும்பு;
            }
        }

        எனில் (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                திரும்பு;
            }
        }

        எனில் (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                திரும்பு;
            }
        }

        எனில் (DfgNot* மாறிலி condNotp = condp->cast<DfgNot>()) {
            எனில் (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    திரும்பு;
                }
            }
        }

        எனில் (DfgNeq* மாறிலி condNeqp = condp->cast<DfgNeq>()) {
            எனில் (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* மாறிலி newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    திரும்பு;
                }
            }
        }

        எனில் (DfgNot* மாறிலி thenNotp = thenp->cast<DfgNot>()) {
            எனில் (DfgNot* மாறிலி elseNotp = elsep->cast<DfgNot>()) {
                எனில் (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* மாறிலி newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (DfgOr* மாறிலி condOrp = condp->cast<DfgOr>()) {
            எனில் (DfgCond* மாறிலி thenCondp = thenp->cast<DfgCond>()) {
                எனில் (!thenCondp->hasMultipleSinks()) {
                    எனில் (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* மாறிலி ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            திரும்பு;
                        }
                    }
                    எனில் (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* மாறிலி ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            திரும்பு;
                        }
                    }
                }
            }
        }

        எனில் (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            எனில் (DfgAdd* மாறிலி thenAddp = thenp->cast<DfgAdd>()) {
                எனில் (DfgConst* மாறிலி constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    எனில் (constp->has_மதிப்பு(1)) {
                        எனில் (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* மாறிலி extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_வரி* மாறிலி thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                திரும்பு;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            எனில் (DfgSub* மாறிலி thenSubp = thenp->cast<DfgSub>()) {
                எனில் (DfgConst* மாறிலி constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    எனில் (constp->has_மதிப்பு(1)) {
                        எனில் (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* மாறிலி extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_வரி* மாறிலி thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                திரும்பு;
                            }
                        }
                    }
                }
            }
        }

        எனில் (vtxp->dtype() == m_bitDType) {
            எனில் (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    திரும்பு;
                }
            }
            எனில் (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    திரும்பு;
                }
            }
        }

        எனில் (vtxp->width() <= VL_QUADSIZE) {
            எனில் (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* மாறிலி maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    திரும்பு;
                }
            }
            எனில் (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* மாறிலி maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    திரும்பு;
                }
            }
            எனில் (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* மாறிலி maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    திரும்பு;
                }
            }
            எனில் (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* மாறிலி maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    திரும்பு;
                }
            }

            எனில் (DfgOr* மாறிலி tOrp = thenp->cast<DfgOr>()) {
                எனில் (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* மாறிலி maskp = replicate(vtxp, condp);
                        DfgAnd* மாறிலி andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        திரும்பு;
                    }
                }
                எனில் (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* மாறிலி maskp = replicate(vtxp, condp);
                        DfgAnd* மாறிலி andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        திரும்பு;
                    }
                }
            }
        }

        எனில் (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            மாறிலி std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            எனில் (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    மாறிலி uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    எனில் (rWidth != rhsp->width()) {
                        மாறிலி DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    எனில் (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        திரும்பு;
                    }
                    // Create new LHS
                    மாறிலி uint32_t lWidth = vtxp->width() - rWidth;
                    மாறிலி DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* மாறிலி lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* மாறிலி lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* மாறிலி lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    திரும்பு;
                }
            }

            மாறிலி std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            எனில் (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    மாறிலி uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    எனில் (lWidth != lhsp->width()) {
                        மாறிலி DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    எனில் (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        திரும்பு;
                    }
                    // Create new RHS
                    மாறிலி uint32_t rWidth = vtxp->width() - lWidth;
                    மாறிலி DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* மாறிலி rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* மாறிலி rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* மாறிலி rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    திரும்பு;
                }
            }
        }

        எனில் (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                எனில் (மாறிலி uint32_t extend = vtxp->width() - 1) {
                    DfgConst* மாறிலி zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                திரும்பு;
            }
        }

        எனில் (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                எனில் (மாறிலி uint32_t extend = vtxp->width() - 1) {
                    DfgConst* மாறிலி zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                திரும்பு;
            }
        }

        எனில் (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                திரும்பு;
            }
        }

        எனில் (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                திரும்பு;
            }
        }

        எனில் (DfgCond* மாறிலி tCondp = thenp->cast<DfgCond>()) {
            எனில் (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    திரும்பு;
                }
            }
        }

        எனில் (DfgCond* மாறிலி eCondp = elsep->cast<DfgCond>()) {
            எனில் (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    திரும்பு;
                }
            }
        }
    }

    வெறுமை visit(DfgVertexVar* மாறிலி vtxp) override {
        எனில் (vtxp->hasSinks()) திரும்பு;
        எனில் (vtxp->isObserved()) திரும்பு;
        எனில் (vtxp->defaultp()) திரும்பு;

        // If undriven, or driven from another var, it is completely redundant.
        எனில் (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                திரும்பு;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        தருக்க foundOne = பொய்;
        மாறிலி தருக்க keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> தருக்க {
            // Ignore non-observable variable sinks. These can be eliminated.
            எனில் (மாறிலி DfgVertexVar* மாறிலி varp = sink.cast<DfgVertexVar>()) {
                எனில் (!varp->hasSinks() && !varp->isObserved()) திரும்பு பொய்;
            }
            // Keep if found more than one sink
            எனில் (foundOne) திரும்பு உண்மை;
            // Mark first sink found
            foundOne = உண்மை;
            திரும்பு பொய்;
        });
        எனில் (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                திரும்பு;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        க்கு (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.தள்ளு_back(nullptr);
        m_iterList.தள்ளு_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        க்கு (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        க்கு (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        போது (உண்மை) {
            // Process the work list - keep the placeholder at index 0
            போது (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.எடு_back();
                எனில் (!m_vtxp) தொடர்;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                எனில் (DfgVertexVar* மாறிலி varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    தொடர்;
                }

                // Ast references are not considered
                எனில் (m_vtxp->is<DfgVertexAst>()) தொடர்;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                எனில் (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    எனில் (DfgVertex* மாறிலி sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            தொடர்;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            எனில் (m_iterList.size() == 1) நிறுத்து;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                க்கு (size_t hops = 1; hops <= 4; ++hops) {
                    க்கு (size_t i = begin; i < end; ++i) {
                        DfgVertex* மாறிலி vtxp = m_iterList[i];
                        எனில் (!vtxp) தொடர்;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            திரும்பு பொய்;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            திரும்பு பொய்;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            க்கு (DfgVertex* மாறிலி vtxp : m_iterList) {
                எனில் (!vtxp) தொடர்;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

பொது:
    நிலையான வெறுமை apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

வெறுமை V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    எனில் (!v3Global.opt.fDfgPeephole()) திரும்பு;
    V3DfgPeephole::apply(dfg, ctx);
}
