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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgCache.h"
#અન્તર્ભાવ "V3DfgDataType.h"
#અન્તર્ભાવ "V3DfgPasses.h"
#અન્તર્ભાવ "V3DfgPeepholePatterns.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cctype>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
નામાકાશ {

template <typename T_Reduction>
સંરચના ReductionToBitwiseImpl {};
template <> સંરચના ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> સંરચના ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> સંરચના ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
સંરચના BitwiseToReductionImpl {};
template <> સંરચના BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> સંરચના BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> સંરચના BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
સંરચના IsAssociative : જાહેર std::false_type {};
template <> સંરચના IsAssociative<DfgAnd> : જાહેર std::true_type {};
template <> સંરચના IsAssociative<DfgOr>  : જાહેર std::true_type {};
template <> સંરચના IsAssociative<DfgXor> : જાહેર std::true_type {};
template <> સંરચના IsAssociative<DfgAdd> : જાહેર std::true_type {};
template <> સંરચના IsAssociative<DfgConcat> : જાહેર std::true_type {};
template <> સંરચના IsAssociative<DfgMul> : જાહેર std::true_type {};
template <> સંરચના IsAssociative<DfgMulS> : જાહેર std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
સંરચના IsCommutative : જાહેર std::false_type {};
template <> સંરચના IsCommutative<DfgAnd> : જાહેર std::true_type {};
template <> સંરચના IsCommutative<DfgOr>  : જાહેર std::true_type {};
template <> સંરચના IsCommutative<DfgXor> : જાહેર std::true_type {};
template <> સંરચના IsCommutative<DfgAdd> : જાહેર std::true_type {};
template <> સંરચના IsCommutative<DfgEq> : જાહેર std::true_type {};
template <> સંરચના IsCommutative<DfgMul> : જાહેર std::true_type {};
template <> સંરચના IsCommutative<DfgMulS> : જાહેર std::true_type {};
template <> સંરચના IsCommutative<DfgNeq> : જાહેર std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
સંરચના IsIdempotent : જાહેર std::false_type {};
template <> સંરચના IsIdempotent<DfgAnd> : જાહેર std::true_type {};
template <> સંરચના IsIdempotent<DfgOr>  : જાહેર std::true_type {};

// Binary result dtype
template<typename Vertex>
અચલ DfgDataType& resultDType(અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp);
template <> અચલ DfgDataType& resultDType<DfgAdd>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgAnd>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgConcat>     (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> અચલ DfgDataType& resultDType<DfgDiv>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgDivS>       (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgEq>         (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgGt>         (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgGtS>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgGte>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgGteS>       (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgLogAnd>     (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgLogEq>      (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgLogIf>      (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgLogOr>      (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgLt>         (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgLtS>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgLte>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgLteS>       (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgModDiv>     (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgModDivS>    (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgMul>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgMulS>       (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgNeq>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા DfgDataType::packed(1); }
template <> અચલ DfgDataType& resultDType<DfgOr>         (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgPow>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgPowSS>      (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgPowSU>      (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgPowUS>      (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgShiftL>     (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgShiftR>     (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgShiftRS>    (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgSub>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }
template <> અચલ DfgDataType& resultDType<DfgXor>        (અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) { પાછા lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> શૂન્ય foldOp(V3_સંખ્યા& out, અચલ V3_સંખ્યા& src);
template <> શૂન્ય foldOp<Dfg_ગણતરી_Ones>  (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.op_ગણતરી_Ones(src); }
template <> શૂન્ય foldOp<DfgExtend>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opAssign(src); }
template <> શૂન્ય foldOp<DfgExtendS>    (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opExtendS(src, src.width()); }
template <> શૂન્ય foldOp<DfgLogNot>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opLogNot(src); }
template <> શૂન્ય foldOp<DfgNegate>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opNegate(src); }
template <> શૂન્ય foldOp<DfgNot>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opNot(src); }
template <> શૂન્ય foldOp<DfgOneHot>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opOneHot(src); }
template <> શૂન્ય foldOp<DfgOneHot0>    (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opOneHot0(src); }
template <> શૂન્ય foldOp<DfgRedAnd>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opRedAnd(src); }
template <> શૂન્ય foldOp<DfgRedOr>      (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opRedOr(src); }
template <> શૂન્ય foldOp<DfgRedXor>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> શૂન્ય foldOp(V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
template <> શૂન્ય foldOp<DfgAdd>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opAdd(lhs, rhs); }
template <> શૂન્ય foldOp<DfgAnd>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opAnd(lhs, rhs); }
template <> શૂન્ય foldOp<DfgConcat>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opConcat(lhs, rhs); }
template <> શૂન્ય foldOp<DfgDiv>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opDiv(lhs, rhs); }
template <> શૂન્ય foldOp<DfgDivS>       (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opDivS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgEq>         (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opEq(lhs, rhs); }
template <> શૂન્ય foldOp<DfgGt>         (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opGt(lhs, rhs); }
template <> શૂન્ય foldOp<DfgGtS>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opGtS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgGte>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opGte(lhs, rhs); }
template <> શૂન્ય foldOp<DfgGteS>       (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opGteS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgLogAnd>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opLogAnd(lhs, rhs); }
template <> શૂન્ય foldOp<DfgLogEq>      (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opLogEq(lhs, rhs); }
template <> શૂન્ય foldOp<DfgLogIf>      (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opLogIf(lhs, rhs); }
template <> શૂન્ય foldOp<DfgLogOr>      (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opLogOr(lhs, rhs); }
template <> શૂન્ય foldOp<DfgLt>         (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opLt(lhs, rhs); }
template <> શૂન્ય foldOp<DfgLtS>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opLtS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgLte>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opLte(lhs, rhs); }
template <> શૂન્ય foldOp<DfgLteS>       (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opLteS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgModDiv>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opModDiv(lhs, rhs); }
template <> શૂન્ય foldOp<DfgModDivS>    (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opModDivS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgMul>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opMul(lhs, rhs); }
template <> શૂન્ય foldOp<DfgMulS>       (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opMulS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgNeq>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opNeq(lhs, rhs); }
template <> શૂન્ય foldOp<DfgOr>         (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opOr(lhs, rhs); }
template <> શૂન્ય foldOp<DfgPow>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opPow(lhs, rhs); }
template <> શૂન્ય foldOp<DfgPowSS>      (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opPowSS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgPowSU>      (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opPowSU(lhs, rhs); }
template <> શૂન્ય foldOp<DfgPowUS>      (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opPowUS(lhs, rhs); }
template <> શૂન્ય foldOp<DfgShiftL>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opShiftL(lhs, rhs); }
template <> શૂન્ય foldOp<DfgShiftR>     (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opShiftR(lhs, rhs); }
template <> શૂન્ય foldOp<DfgShiftRS>    (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> શૂન્ય foldOp<DfgSub>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opSub(lhs, rhs); }
template <> શૂન્ય foldOp<DfgXor>        (V3_સંખ્યા& out, અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

વર્ગ V3DfgPeephole final : જાહેર DfgVisitor {
    // TYPES
    સંરચના VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        તાર્કિક m_isCachedVertex = ખોટું;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    અચલ DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* અચલ m_tmpScopep = v3Global.rootp()->ટોચ_Scopep()->scopep();

    // STATIC STATE
    સ્થિર V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) જો (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    તાર્કિક checkApplying(VDfgPeepholePattern id) {
        જો (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) પાછા ખોટું;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_ગણતરી[id];
        પાછા સાચું;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* અચલ equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        પાછા equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    શૂન્ય invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = ખોટું;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_લીટી* flp, અચલ DfgDataType& dtype, Operands... operands) {
        Vertex* અચલ vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = સાચું;
        પાછા vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(અચલ DfgDataType& dtype, Operands... operands) {
        Vertex* અચલ vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        પાછા vtxp;
    }

    શૂન્ય incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    શૂન્ય addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        જો (vInfo.m_workListIndex) પાછા;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.ધકેલો_back(vtxp);
    }

    શૂન્ય removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    શૂન્ય addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        જો (vInfo.m_iterListIndex) પાછા;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.ધકેલો_back(vtxp);
    }

    શૂન્ય removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    શૂન્ય deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        અચલ DfgVertexVar* અચલ varp = vtxp->cast<DfgVertexVar>();
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
            જો (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                પાછા ખોટું;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            જો (vInfo.m_generation != m_currentGeneration) srcps.ધકેલો_back(&src);
            vInfo.m_generation = m_currentGeneration;
            પાછા ખોટું;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        જો (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ધકેલો_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } નહીંતર {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        અચલ auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  પાછા srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        માટે (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        માટે (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    શૂન્ય replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        અચલ auto debugCallback = [&]() -> શૂન્ય {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            અચલ auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](અચલ DfgVertex& v) {  //
                પાછા cone->ગણતરી(&v);
            });
        };
        જો (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) પાછા;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            પાછા ખોટું;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            પાછા ખોટું;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            પાછા ખોટું;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            પાછા ખોટું;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_લીટી* flp, uint32_t val) { પાછા નવું DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_લીટી* flp, uint32_t width) {
        પાછા નવું DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_લીટી* flp, અચલ DfgDataType& dtype) {
        પાછા makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_લીટી* flp, uint32_t width) {
        DfgConst* અચલ resp = makeZero(flp, width);
        resp->અંક().setAllBits1();
        પાછા resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_લીટી* flp, અચલ DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* અચલ vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        જો (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        જો (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        પાછા vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(અચલ DfgVertex* examplep, Operands... operands) {
        પાછા make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        જો (vtxp->dtype() == m_bitDType) પાછા bitp;
        પાછા make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    સ્થિર તાર્કિક isSame(અચલ DfgVertex* ap, અચલ DfgVertex* bp) {
        જો (ap == bp) પાછા સાચું;
        જો (ap->dtype() != bp->dtype()) પાછા ખોટું;
        અચલ DfgConst* અચલ aConstp = ap->cast<DfgConst>();
        જો (!aConstp) પાછા ખોટું;
        અચલ DfgConst* અચલ bConstp = bp->cast<DfgConst>();
        જો (!bConstp) પાછા ખોટું;
        પાછા aConstp->અંક().isCaseEq(bConstp->અંક());
    }

    સ્થિર તાર્કિક isZero(અચલ DfgVertex* vtxp) {
        જો (અચલ DfgConst* અચલ constp = vtxp->cast<DfgConst>()) પાછા constp->isZero();
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક isOnes(અચલ DfgVertex* vtxp) {
        જો (અચલ DfgConst* અચલ constp = vtxp->cast<DfgConst>()) પાછા constp->isOnes();
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક isEqOne(અચલ DfgVertex* vtxp) {
        જો (અચલ DfgConst* અચલ constp = vtxp->cast<DfgConst>()) પાછા constp->અંક().isEqOne();
        પાછા ખોટું;
    }

    સ્થિર તાર્કિક areAdjacent(uint32_t& lsb, અચલ DfgSel* lSelp, અચલ DfgSel* rSelp) {
        જો (!isSame(lSelp->srcp(), rSelp->srcp())) પાછા ખોટું;
        જો (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            પાછા સાચું;
        }
        જો (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            પાછા સાચું;
        }
        પાછા ખોટું;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક foldUnary(Vertex* અચલ vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::કિંમત, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::કિંમત, "Must invoke on final class");
        જો (DfgConst* અચલ srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* અચલ resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->અંક(), srcp->અંક());
                replace(resultp);
                પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક દ્વિઅંકી(Vertex* અચલ vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_દ્વિઅંકી, Vertex>::કિંમત, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::કિંમત, "Must invoke on final class");

        File_લીટી* અચલ flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* અચલ lConstp = lhsp->cast<DfgConst>();
        DfgConst* અચલ rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* અચલ lSamep = lhsp->cast<Vertex>();
        Vertex* અચલ rSamep = rhsp->cast<Vertex>();

        // Constant folding
        જો (lConstp && rConstp) {
            APPLYING(FOLD_દ્વિઅંકી) {
                DfgConst* અચલ resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->અંક(), lConstp->અંક(), rConstp->અંક());
                replace(resultp);
                પાછા સાચું;
            }
        }

        // Idempotent
        જો VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::કિંમત) {
            જો (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_દ્વિઅંકી) {
                    replace(lhsp);
                    પાછા સાચું;
                }
            }
        }

        // Associative
        જો VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::કિંમત) {
            // Also commutative and idempotent
            જો VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::કિંમત && IsIdempotent<Vertex>::કિંમત) {
                જો (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_દ્વિઅંકી_LHS) {
                        replace(lSamep);
                        પાછા સાચું;
                    }
                }
                જો (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_દ્વિઅંકી_RHS) {
                        replace(rSamep);
                        પાછા સાચું;
                    }
                }
            }

            જો (lConstp && rSamep) {
                જો (DfgConst* અચલ rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_દ્વિઅંકી_LHS_OF_RHS) {
                        DfgConst* અચલ cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->અંક(), lConstp->અંક(), rlConstp->અંક());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        પાછા સાચું;
                    }
                }
            }

            જો (lSamep && rConstp) {
                જો (DfgConst* અચલ lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_દ્વિઅંકી_RHS_OF_LHS) {
                        DfgConst* અચલ cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->અંક(), lrConstp->અંક(), rConstp->અંક());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        પાછા સાચું;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            જો (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* અચલ ap = lSamep->lhsp();
                DfgVertex* અચલ bp = lSamep->rhsp();
                DfgVertex* અચલ cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                જો (ap != cp && bp != cp) {
                    APPLYING(જમણું_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        અચલ DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* અચલ bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        પાછા સાચું;
                    }
                }
            }

            જો (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* અચલ rlVtxp = rSamep->lhsp();
                DfgVertex* અચલ rrVtxp = rSamep->rhsp();

                જો VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::કિંમત) {
                    જો (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            પાછા સાચું;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                જો (Vertex* અચલ existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    જો (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_દ્વિઅંકી_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            પાછા સાચું;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                જો VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::કિંમત) {
                    જો (Vertex* અચલ existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        જો (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_દ્વિઅંકી_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                પાછા સાચું;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        જો VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::કિંમત) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            જો (lConstp) {
                જો (DfgCond* અચલ rCondp = rhsp->cast<DfgCond>()) {
                    જો (!rCondp->hasMultipleSinks()) {
                        APPLYING(ધકેલો_COMMUTATIVE_દ્વિઅંકી_THROUGH_COND) {
                            DfgVertex* અચલ tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* અચલ ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            પાછા સાચું;
                        }
                    }
                }
                પાછા ખોટું;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        અચલ તાર્કિક swapSides = [&]() {
            // Don't swap if not commutative, duh
            જો VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::કિંમત) પાછા ખોટું;
            // Const go on left
            જો (lConstp) પાછા ખોટું;
            જો (rConstp) પાછા સાચું;
            // Not go on left
            જો (lhsp->is<DfgNot>()) પાછા ખોટું;
            જો (rhsp->is<DfgNot>()) પાછા સાચું;
            // Same go on right
            જો (rSamep) પાછા ખોટું;
            જો (lSamep) પાછા સાચું;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            અચલ VertexInfo& lInfo = m_vInfo[lhsp];
            અચલ VertexInfo& rInfo = m_vInfo[rhsp];
            પાછા lInfo.m_id > rInfo.m_id;
        }();
        જો (swapSides) {
            APPLYING(SWAP_SIDES_IN_દ્વિઅંકી) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                પાછા સાચું;
            }
        }

        પાછા ખોટું;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક distributive_And_Associative_દ્વિઅંકી(Vertex* vtxp) {
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        જો (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            જો (Other* અચલ lp = lhsp->cast<Other>()) {
                જો (Other* અચલ rp = rhsp->cast<Other>()) {
                    DfgVertex* અચલ llp = lp->lhsp();
                    DfgVertex* અચલ lrp = lp->rhsp();
                    DfgVertex* અચલ rlp = rp->lhsp();
                    DfgVertex* અચલ rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    જો (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } નહીંતર જો (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } નહીંતર જો (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } નહીંતર જો (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    જો (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_દ્વિઅંકી) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            પાછા સાચું;
                        }
                    }
                }
            }
        }

        પાછા ખોટું;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક
    try_ધકેલો_Bitwise_Op_Through_Concat(Vertex* અચલ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_લીટી* અચલ flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* અચલ catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* અચલ catRConstp = concatp->rhsp()->cast<DfgConst>();
        જો (catLConstp || catRConstp) {
            APPLYING(ધકેલો_BITWISE_OP_THROUGH_CONCAT) {
                અચલ uint32_t width = concatp->width();
                અચલ DfgDataType& lDtype = concatp->lhsp()->dtype();
                અચલ DfgDataType& rDtype = concatp->rhsp()->dtype();
                અચલ uint32_t lWidth = lDtype.size();
                અચલ uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* અચલ newLhsp = [&]() -> DfgVertex* {
                    DfgConst* અચલ newLhsConstp = makeZero(constp->fileline(), lWidth);
                    જો (catLConstp) {
                        V3_સંખ્યા અંક{constp->fileline(), static_cast<પૂર્ણાંક>(lWidth), 0u};
                        અંક.opSel(constp->અંક(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->અંક(), અંક, catLConstp->અંક());
                        પાછા newLhsConstp;
                    }
                    newLhsConstp->અંક().opSel(constp->અંક(), width - 1, rWidth);
                    પાછા make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* અચલ newRhsp = [&]() -> DfgVertex* {
                    DfgConst* અચલ newRhsConstp = makeZero(constp->fileline(), rWidth);
                    જો (catRConstp) {
                        V3_સંખ્યા અંક{constp->fileline(), static_cast<પૂર્ણાંક>(rWidth), 0u};
                        અંક.opSel(constp->અંક(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->અંક(), અંક, catRConstp->અંક());
                        પાછા newRhsConstp;
                    }
                    newRhsConstp->અંક().opSel(constp->અંક(), rWidth - 1, 0);
                    પાછા make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક
    try_ધકેલો_Compare_Op_Through_Concat(Vertex* અચલ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_લીટી* અચલ flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        જો (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ધકેલો_COMPARE_OP_THROUGH_CONCAT) {
                અચલ uint32_t width = concatp->width();
                અચલ uint32_t lWidth = concatp->lhsp()->width();
                અચલ uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* અચલ newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->અંક().opSel(constp->અંક(), width - 1, rWidth);
                Vertex* અચલ newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* અચલ newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->અંક().opSel(constp->અંક(), rWidth - 1, 0);
                Vertex* અચલ newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                જો VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::કિંમત) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } નહીંતર જો VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::કિંમત) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } નહીંતર {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક try_ધકેલો_Bitwise_Op_Through_Reductions(Bitwise* અચલ vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        જો (Reduction* અચલ lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            જો (Reduction* અચલ rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* અચલ lSrcp = lRedp->srcp();
                DfgVertex* અચલ rSrcp = rRedp->srcp();
                જો (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(ધકેલો_BITWISE_THROUGH_REDUCTION) {
                        File_લીટી* અચલ flp = vtxp->fileline();
                        Bitwise* અચલ bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        પાછા સાચું;
                    }
                }
            }
        }

        પાછા ખોટું;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    સ્થિર std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        જો (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            પાછા {nullptr, nullptr};
        }
        જો (vtxp->hasMultipleSinks()) પાછા {nullptr, nullptr};
        Dfg_Vertex_દ્વિઅંકી* અચલ bwp = vtxp->as<Dfg_Vertex_દ્વિઅંકી>();
        DfgSel* અચલ lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* અચલ rSelp = bwp->rhsp()->cast<DfgSel>();
        જો (!lSelp || !rSelp) પાછા {nullptr, nullptr};
        DfgSel* અચલ nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* અચલ wSelp = nSelp == lSelp ? rSelp : lSelp;
        અચલ uint32_t nWidth = nSelp->fromp()->width();
        જો (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) પાછા {nullptr, nullptr};
        જો (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            પાછા {nullptr, nullptr};
        }
        પાછા {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક try_ધકેલો_Bitwise_Op_Throug_Sel(Bitwise* અચલ vtxp) {
        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();

        જો (DfgSel* અચલ lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            જો (DfgSel* અચલ selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } નહીંતર જો (Bitwise* અચલ bitwisep = rhsp->cast<Bitwise>()) {
                જો (DfgSel* અચલ rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } નહીંતર જો (DfgSel* અચલ rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            જો (rSelp) {
                DfgVertex* અચલ lFromp = lSelp->fromp();
                DfgVertex* અચલ rFromp = rSelp->fromp();
                જો (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ધકેલો_BITWISE_THROUGH_SEL) {
                        Bitwise* અચલ bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        જો (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        પાછા સાચું;
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
            Bitwise* અચલ rChainp = rhsp->cast<Bitwise>();
            જો (rChainp && rChainp->hasMultipleSinks()) પાછા ખોટું;
            DfgVertex* અચલ rTermp = rChainp ? rChainp->lhsp() : rhsp;

            અચલ auto lSels = pushableUnalignedSels(lhsp);
            અચલ auto rSels = pushableUnalignedSels(rTermp);
            જો (!lSels.first && !rSels.first) પાછા ખોટું;

            અચલ DfgVertex* અચલ opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            જો (opp->is<Bitwise>()) પાછા ખોટું;
            અચલ auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* અચલ nFromp = sels.first->fromp();
            DfgVertex* અચલ wFromp = sels.second->fromp();
            અચલ uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            અચલ auto matches = [&](અચલ DfgVertex* termp, અચલ auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                જો (tSels.first) {
                    પાછા termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                જો (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                જો (અચલ DfgSel* અચલ selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                જો (termp->type() != opp->type()) પાછા ખોટું;
                // Operands might have been swapped since
                અચલ Dfg_Vertex_દ્વિઅંકી* અચલ bwp = termp->as<Dfg_Vertex_દ્વિઅંકી>();
                અચલ તાર્કિક nIsLhs = bwp->lhsp() == nFromp;
                જો (!nIsLhs && bwp->rhsp() != nFromp) પાછા ખોટું;
                અચલ DfgSel* અચલ slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                પાછા slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            જો (!matches(lhsp, lSels) || !matches(rTermp, rSels)) પાછા ખોટું;

            APPLYING(ધકેલો_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_લીટી* અચલ flp = vtxp->fileline();
                અચલ DfgDataType& dtype = nFromp->dtype();
                DfgSel* અચલ slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                જો (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } નહીંતર જો (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } નહીંતર {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                અચલ auto ધકેલો = [&](DfgVertex* termp, અચલ DfgSel* nSelp) -> DfgVertex* {
                    પાછા nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* અચલ newLhsp = ધકેલો(lhsp, lSels.first);
                DfgVertex* અચલ newRTermp = ધકેલો(rTermp, rSels.first);
                જો (rChainp) {
                    DfgVertex* અચલ newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } નહીંતર {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                પાછા સાચું;
            }
        }

        પાછા ખોટું;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    સ્થિર તાર્કિક isConcatFoldable(અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        જો (isSame(lhsp, rhsp)) પાછા સાચું;
        જો (અચલ DfgRep* અચલ lRepp = lhsp->cast<DfgRep>()) {
            જો (isSame(lRepp->srcp(), rhsp)) પાછા સાચું;
        }
        જો (અચલ DfgRep* અચલ rRepp = rhsp->cast<DfgRep>()) {
            જો (isSame(lhsp, rRepp->srcp())) પાછા સાચું;
        }
        // Concatenation of adjoining selects is a select
        જો (અચલ DfgSel* અચલ lSelp = lhsp->cast<DfgSel>()) {
            જો (અચલ DfgSel* અચલ rSelp = rhsp->cast<DfgSel>()) {
                પાછા isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        જો (અચલ DfgNot* અચલ lNotp = lhsp->cast<DfgNot>()) {
            જો (અચલ DfgNot* અચલ rNotp = rhsp->cast<DfgNot>()) {
                પાછા !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        પાછા ખોટું;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    સ્થિર તાર્કિક isSameBitwise(અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) {
        જો (lhsp->type() != rhsp->type()) પાછા ખોટું;
        પાછા lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    સ્થિર તાર્કિક isConcatSimplifiable(અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp) {
        જો (isConcatFoldable(lhsp, rhsp)) પાછા સાચું;
        જો (!isSameBitwise(lhsp, rhsp)) પાછા ખોટું;
        જો (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) પાછા ખોટું;
        અચલ Dfg_Vertex_દ્વિઅંકી* અચલ lp = lhsp->as<Dfg_Vertex_દ્વિઅંકી>();
        અચલ Dfg_Vertex_દ્વિઅંકી* અચલ rp = rhsp->as<Dfg_Vertex_દ્વિઅંકી>();
        જો (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            પાછા સાચું;
        જો (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            પાછા સાચું;
        પાછા ખોટું;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    સ્થિર તાર્કિક isConcatPushableThroughBitwise(અચલ DfgVertex* lhsp, અચલ DfgVertex* rhsp,
                                               તાર્કિક& swap) {
        જો (!isSameBitwise(lhsp, rhsp)) પાછા ખોટું;
        જો (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) પાછા ખોટું;
        અચલ Dfg_Vertex_દ્વિઅંકી* અચલ lp = lhsp->as<Dfg_Vertex_દ્વિઅંકી>();
        અચલ Dfg_Vertex_દ્વિઅંકી* અચલ rp = rhsp->as<Dfg_Vertex_દ્વિઅંકી>();
        swap = ખોટું;
        જો (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) પાછા સાચું;
        જો (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) પાછા સાચું;
        swap = સાચું;
        જો (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) પાછા સાચું;
        જો (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) પાછા સાચું;
        પાછા ખોટું;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_લીટી* flp, અચલ DfgVertex* lhsp,
                                              અચલ DfgVertex* rhsp, તાર્કિક swap) {
        અચલ Dfg_Vertex_દ્વિઅંકી* અચલ lp = lhsp->as<Dfg_Vertex_દ્વિઅંકી>();
        અચલ Dfg_Vertex_દ્વિઅંકી* અચલ rp = rhsp->as<Dfg_Vertex_દ્વિઅંકી>();
        અચલ DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* અચલ cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* અચલ dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* અચલ newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* અચલ newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        જો (lp->is<DfgAnd>()) પાછા make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        જો (lp->is<DfgOr>()) પાછા make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        પાછા make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();

        જો (DfgSel* અચલ lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            જો (DfgSel* અચલ selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } નહીંતર જો (Bitwise* અચલ rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } નહીંતર જો (Reduction* અચલ rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            જો (rSelp) {
                uint32_t lsb = 0;
                જો (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        અચલ DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* અચલ newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        જો (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        પાછા સાચું;
                    }
                }
            }
        }

        જો (Reduction* અચલ lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            જો (!rRedp) {
                જો (Bitwise* અચલ rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            જો (rRedp) {
                જો (DfgSel* અચલ lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    જો (DfgSel* અચલ rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        જો (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                અચલ DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* અચલ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                જો (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                પાછા સાચું;
                            }
                        }
                    }
                }
            }
        }

        પાછા ખોટું;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક optimizeReduction(Reduction* અચલ vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        જો (foldUnary(vtxp)) પાછા સાચું;

        DfgVertex* અચલ srcp = vtxp->srcp();
        File_લીટી* અચલ flp = vtxp->fileline();

        // Reduction of 1-bit value
        જો (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                પાછા સાચું;
            }
        }

        જો (DfgCond* અચલ condp = srcp->cast<DfgCond>()) {
            જો (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ધકેલો_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* અચલ newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* અચલ newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* અચલ newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    પાછા સાચું;
                }
            }
        }

        જો (DfgConcat* અચલ concatp = srcp->cast<DfgConcat>()) {
            જો (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(ધકેલો_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* અચલ lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* અચલ rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    પાછા સાચું;
                }
            }
        }

        જો (Bitwise* અચલ bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            જો (!bitwisep->hasMultipleSinks()) {
                જો (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(ધકેલો_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* અચલ newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* અચલ newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        પાછા સાચું;
                    }
                }

                જો (DfgSel* અચલ lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    જો (DfgSel* અચલ rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        જો (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ધકેલો_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                અચલ DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* અચલ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                પાછા સાચું;
                            }
                        }
                    }
                }
            }
        }

        પાછા ખોટું;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT તાર્કિક optimizeShiftRHS(Shift* અચલ vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_દ્વિઅંકી, Shift>::કિંમત, "Must invoke on binary");
        static_assert(std::is_final<Shift>::કિંમત, "Must invoke on final class");
        જો (અચલ DfgConcat* અચલ concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            જો (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    પાછા સાચું;
                }
            }
        }
        પાછા ખોટું;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_ગણતરી_Ones_Term(DfgVertex* vtxp) {
        જો (DfgConcat* અચલ oCatp = vtxp->cast<DfgConcat>()) {
            જો (isZero(oCatp->lhsp())) {
                જો (Dfg_ગણતરી_Ones* અચલ ગણતરી_Onesp = oCatp->rhsp()->cast<Dfg_ગણતરી_Ones>()) {
                    // Zero extended count ones
                    જો (DfgSel* અચલ selp = ગણતરી_Onesp->srcp()->cast<DfgSel>()) {
                        પાછા {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } નહીંતર જો (DfgSel* અચલ selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    જો (selp->dtype() == m_bitDType) {  //
                        પાછા {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            પાછા {nullptr, 0, 0};
        }
        જો (Dfg_ગણતરી_Ones* અચલ ગણતરી_Onesp = vtxp->cast<Dfg_ગણતરી_Ones>()) {
            // Simple count ones
            જો (DfgSel* અચલ selp = ગણતરી_Onesp->srcp()->cast<DfgSel>()) {
                પાછા {selp->fromp(), selp->lsb(), selp->width()};
            }
            પાછા {nullptr, 0, 0};
        }
        જો (DfgSel* અચલ oSelp = vtxp->cast<DfgSel>()) {
            જો (oSelp->lsb() == 0) {
                // Truncated count ones
                જો (Dfg_ગણતરી_Ones* અચલ ગણતરી_Onesp = oSelp->fromp()->cast<Dfg_ગણતરી_Ones>()) {
                    // Zero extended count ones
                    જો (DfgSel* અચલ selp = ગણતરી_Onesp->srcp()->cast<DfgSel>()) {
                        પાછા {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            જો (oSelp->dtype() == m_bitDType) {  //
                પાછા {oSelp->fromp(), oSelp->lsb(), 1};
            }
            પાછા {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        જો (DfgShiftR* અચલ shiftrp = vtxp->cast<DfgShiftR>()) {
            જો (DfgConst* અચલ rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                જો (rConstp->toU32() == shiftrp->width() - 1) {
                    પાછા {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            પાછા {nullptr, 0, 0};
        }
        // Not applicable
        પાછા {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              અચલ uint32_t width) {
        જ્યારે (સાચું) {
            અચલ uint32_t msb = lsb + width - 1;

            // Sel from Concat
            જો (DfgConcat* અચલ concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* અચલ lhsp = concatp->lhsp();
                DfgVertex* અચલ rhsp = concatp->rhsp();

                જો (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        ચાલુ;
                    }
                } નહીંતર જો (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        ચાલુ;
                    }
                }
            }

            જો (DfgRep* અચલ repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                અચલ uint32_t srcWidth = repp->srcp()->width();
                જો (width <= srcWidth) {
                    અચલ uint32_t newLsb = lsb % srcWidth;
                    અચલ uint32_t newMsb = newLsb + width - 1;
                    જો (newMsb < srcWidth) {
                        APPLYING(ધકેલો_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            ચાલુ;
                        }
                    }
                }
            }

            // Sel from Sel
            જો (DfgSel* અચલ selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    ચાલુ;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            જો (DfgVarPacked* અચલ varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                અચલ auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* અચલ driverp = pair.first;
                અચલ uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                જો (driverp) {
                    જો (driverp == varp->defaultp()) {
                        APPLYING(ધકેલો_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ચાલુ;
                        }
                    } નહીંતર {
                        APPLYING(ધકેલો_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ચાલુ;
                        }
                    }
                }
            }

            // No patterns matched, stop
            તોડો;
        }
        પાછા {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        જો (ap == bp) પાછા {ap, ap->width()};

        // If both constants, check LSBs
        જો (DfgConst* અચલ aConstp = ap->cast<DfgConst>()) {
            જો (DfgConst* અચલ bConstp = bp->cast<DfgConst>()) {
                અચલ V3_સંખ્યા& a_અંક = aConstp->અંક();
                અચલ V3_સંખ્યા& b_અંક = bConstp->અંક();
                // Max match is the shorter constant
                અચલ uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                માટે (; matchWidth < maxMatch; ++matchWidth) {
                    જો (a_અંક.bitIs0(matchWidth) != b_અંક.bitIs0(matchWidth)) તોડો;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* અચલ shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                પાછા {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        જો (DfgConcat* અચલ catp = ap->cast<DfgConcat>()) પાછા commonLSBs(catp->rhsp(), bp);
        જો (DfgConcat* અચલ catp = bp->cast<DfgConcat>()) પાછા commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        જો (DfgSel* અચલ selp = ap->cast<DfgSel>()) {
            જો (selp->lsb() == 0) {
                DfgVertex* અચલ fromp = selp->fromp();
                અચલ std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                પાછા {common.first, std::min(common.second, selp->width())};
            }
        }
        જો (DfgSel* અચલ selp = bp->cast<DfgSel>()) {
            જો (selp->lsb() == 0) {
                DfgVertex* અચલ fromp = selp->fromp();
                અચલ std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                પાછા {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        પાછા {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        જો (ap == bp) પાછા {ap, ap->width()};

        // If both constants, check MSBs
        જો (DfgConst* અચલ aConstp = ap->cast<DfgConst>()) {
            જો (DfgConst* અચલ bConstp = bp->cast<DfgConst>()) {
                અચલ uint32_t aMsb = aConstp->width() - 1;
                અચલ uint32_t bMsb = bConstp->width() - 1;
                અચલ V3_સંખ્યા& a_અંક = aConstp->અંક();
                અચલ V3_સંખ્યા& b_અંક = bConstp->અંક();
                // Max match is the shorter constant
                અચલ uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                માટે (; matchWidth < maxMatch; ++matchWidth) {
                    જો (a_અંક.bitIs0(aMsb - matchWidth) != b_અંક.bitIs0(bMsb - matchWidth)) તોડો;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* અચલ shorterp = aMsb < bMsb ? aConstp : bConstp;
                પાછા {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        જો (DfgConcat* અચલ catp = ap->cast<DfgConcat>()) પાછા commonMSBs(catp->lhsp(), bp);
        જો (DfgConcat* અચલ catp = bp->cast<DfgConcat>()) પાછા commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        જો (DfgSel* અચલ selp = ap->cast<DfgSel>()) {
            DfgVertex* અચલ fromp = selp->fromp();
            જો (selp->msb() == fromp->width() - 1) {
                અચલ std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                પાછા {common.first, std::min(common.second, selp->width())};
            }
        }
        જો (DfgSel* અચલ selp = bp->cast<DfgSel>()) {
            DfgVertex* અચલ fromp = selp->fromp();
            જો (selp->msb() == fromp->width() - 1) {
                અચલ std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                પાછા {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        પાછા {nullptr, 0};
    }

    // VISIT methods

    શૂન્ય visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    શૂન્ય visit(Dfg_ગણતરી_Ones* અચલ vtxp) override {
        જો (foldUnary(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgExtend* અચલ vtxp) override {
        જો (foldUnary(vtxp)) પાછા;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* અચલ zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            પાછા;
        }
    }

    શૂન્ય visit(DfgExtendS* અચલ vtxp) override {
        જો (foldUnary(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgLogNot* અચલ vtxp) override {
        જો (foldUnary(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgNegate* અચલ vtxp) override {
        જો (foldUnary(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgNot* અચલ vtxp) override {
        જો (foldUnary(vtxp)) પાછા;

        // Not of Cond
        જો (DfgCond* અચલ condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            જો (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ધકેલો_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* અચલ newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* અચલ newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* અચલ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    પાછા;
                }
            }
        }

        // Not of Not
        જો (DfgNot* અચલ notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                પાછા;
            }
        }

        જો (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            જો (DfgEq* અચલ eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    પાછા;
                }
            }

            // Not of Neq
            જો (DfgNeq* અચલ neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    પાછા;
                }
            }

            // Not of unsigned comparisons
            જો (DfgLt* અચલ p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    પાછા;
                }
            }
            જો (DfgGte* અચલ p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    પાછા;
                }
            }
            જો (DfgGt* અચલ p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    પાછા;
                }
            }
            જો (DfgLte* અચલ p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    પાછા;
                }
            }

            // Not of signed comparisons
            જો (DfgLtS* અચલ p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    પાછા;
                }
            }
            જો (DfgGteS* અચલ p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    પાછા;
                }
            }
            જો (DfgGtS* અચલ p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    પાછા;
                }
            }
            જો (DfgLteS* અચલ p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgOneHot* અચલ vtxp) override {
        જો (foldUnary(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgOneHot0* અચલ vtxp) override {
        જો (foldUnary(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgRedOr* અચલ vtxp) override {
        જો (optimizeReduction(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgRedAnd* અચલ vtxp) override {
        જો (optimizeReduction(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgRedXor* અચલ vtxp) override {
        જો (optimizeReduction(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgSel* અચલ vtxp) override {
        DfgVertex* અચલ fromp = vtxp->fromp();

        File_લીટી* અચલ flp = vtxp->fileline();

        અચલ uint32_t lsb = vtxp->lsb();
        અચલ uint32_t width = vtxp->width();
        અચલ uint32_t msb = lsb + width - 1;

        જો (DfgConst* અચલ constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* અચલ resp = makeZero(flp, width);
                resp->અંક().opSel(constp->અંક(), msb, lsb);
                replace(resp);
                પાછા;
            }
        }

        // Full width select, replace with the source.
        જો (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                પાછા;
            }
        }

        // Unwind through bit packing in one go
        {
            અચલ auto res = unwindSel(fromp, lsb, width);
            જો (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                પાછા;
            }
        }

        // Sel from Not
        જો (DfgNot* અચલ notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            જો (!notp->hasMultipleSinks()) {
                APPLYING(ધકેલો_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* અચલ newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    પાછા;
                }
            }
        }

        // Sel from Cond
        જો (DfgCond* અચલ condp = fromp->cast<DfgCond>()) {
            જો (!condp->hasMultipleSinks()) {
                APPLYING(ધકેલો_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* અચલ newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* અચલ newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* અચલ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    પાછા;
                }
            }
        }

        // Sel from ShiftL
        જો (DfgShiftL* અચલ shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            જો (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(ધકેલો_SEL_THROUGH_SHIFTL) {
                    DfgSel* અચલ newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgMux* અચલ vtxp) override {
        DfgVertex* અચલ fromp = vtxp->fromp();
        DfgVertex* અચલ lsbp = vtxp->lsbp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (DfgConst* અચલ lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->અંક().toUInt()));
                પાછા;
            }
        }

        જો (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                પાછા;
            }
        }

        જો (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                પાછા;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    શૂન્ય visit(DfgAnd* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        જો (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            જો (DfgNot* અચલ lhsNotp = lhsp->cast<DfgNot>()) {
                જો (DfgNot* અચલ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* અચલ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        પાછા;
                    }
                }
                જો (DfgNeq* અચલ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* અચલ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* અચલ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        પાછા;
                    }
                }
            }
        }

        જો (DfgConst* અચલ lConstp = lhsp->cast<DfgConst>()) {
            જો (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    પાછા;
                }
            }

            જો (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    પાછા;
                }
            }

            જો (DfgConcat* અચલ rhsConcatp = rhsp->cast<DfgConcat>()) {
                જો (try_ધકેલો_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) પાછા;
            }
        }

        જો (distributive_And_Associative_દ્વિઅંકી<DfgOr, DfgAnd>(vtxp)) પાછા;

        જો (try_ધકેલો_Bitwise_Op_Through_Reductions(vtxp)) પાછા;

        જો (try_ધકેલો_Bitwise_Op_Throug_Sel(vtxp)) પાછા;

        {
            DfgNot* અચલ lNotp = lhsp->cast<DfgNot>();
            DfgNot* અચલ rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            જો ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    પાછા;
                }
            }

            જો (DfgAnd* અચલ rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* અચલ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* અચલ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                જો ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        પાછા;
                    }
                }
            }
        }

        જો (vtxp->dtype() == m_bitDType) {
            જો (tryReplaceBitwiseWithReduction(vtxp)) પાછા;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            જો (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            જો (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* અચલ newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgOr* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        જો (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            જો (DfgNot* અચલ lhsNotp = lhsp->cast<DfgNot>()) {
                જો (DfgNot* અચલ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* અચલ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        પાછા;
                    }
                }
                જો (DfgNeq* અચલ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* અચલ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* અચલ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        પાછા;
                    }
                }
            }
        }

        જો (DfgConcat* અચલ lhsConcatp = lhsp->cast<DfgConcat>()) {
            જો (DfgConcat* અચલ rhsConcatp = rhsp->cast<DfgConcat>()) {
                જો (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    જો (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            પાછા;
                        }
                    }
                    જો (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            પાછા;
                        }
                    }
                }
            }
        }

        જો (DfgConst* અચલ lConstp = lhsp->cast<DfgConst>()) {
            જો (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    પાછા;
                }
            }

            જો (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    પાછા;
                }
            }

            જો (DfgConcat* અચલ rhsConcatp = rhsp->cast<DfgConcat>()) {
                જો (try_ધકેલો_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) પાછા;
            }
        }

        જો (distributive_And_Associative_દ્વિઅંકી<DfgAnd, DfgOr>(vtxp)) પાછા;

        જો (try_ધકેલો_Bitwise_Op_Through_Reductions(vtxp)) પાછા;

        જો (try_ધકેલો_Bitwise_Op_Throug_Sel(vtxp)) પાછા;

        {
            DfgNot* અચલ lNotp = lhsp->cast<DfgNot>();
            DfgNot* અચલ rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            જો ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    પાછા;
                }
            }

            જો (DfgOr* અચલ rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* અચલ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* અચલ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                જો ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        પાછા;
                    }
                }
            }
        }

        જો (vtxp->dtype() == m_bitDType) {
            જો (tryReplaceBitwiseWithReduction(vtxp)) પાછા;
        }
    }

    શૂન્ય visit(DfgXor* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                પાછા;
            }
        }

        જો (DfgConst* અચલ lConstp = lhsp->cast<DfgConst>()) {
            જો (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    પાછા;
                }
            }
            જો (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    પાછા;
                }
            }
            જો (DfgConcat* અચલ rConcatp = rhsp->cast<DfgConcat>()) {
                જો (try_ધકેલો_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) પાછા;
            }
        }

        જો (try_ધકેલો_Bitwise_Op_Through_Reductions(vtxp)) પાછા;

        જો (try_ધકેલો_Bitwise_Op_Throug_Sel(vtxp)) પાછા;

        જો (vtxp->dtype() == m_bitDType) {
            જો (tryReplaceBitwiseWithReduction(vtxp)) પાછા;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    શૂન્ય visit(DfgAdd* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                પાછા;
            }
        }

        અચલ std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_ગણતરી_Ones_Term(lhsp);
        જો (DfgVertex* અચલ lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_ગણતરી_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            જો (!std::get<0>(rTerm)) {
                જો (DfgAdd* અચલ rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_ગણતરી_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            જો (DfgVertex* અચલ rVtxp = std::get<0>(rTerm)) {
                જો (isSame(lVtxp, rVtxp)) {
                    અચલ uint32_t lLsb = std::get<1>(lTerm);
                    અચલ uint32_t rLsb = std::get<1>(rTerm);
                    અચલ uint32_t lWidth = std::get<2>(lTerm);
                    અચલ uint32_t rWidth = std::get<2>(rTerm);
                    તાર્કિક adjoined = સાચું;
                    uint32_t lsb = 0;
                    જો (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } નહીંતર જો (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } નહીંતર {
                        adjoined = ખોટું;
                    }
                    જો (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_ગણતરી_ONES) {
                            DfgSel* અચલ selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_ગણતરી_Ones>(flp, DfgDataType::packed(32), selp);
                            જો (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } નહીંતર જો (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            જો (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            પાછા;
                        }
                    }
                }
            }
        }
    }

    શૂન્ય visit(DfgArraySel* અચલ vtxp) override {
        DfgConst* અચલ idxp = vtxp->bitp()->cast<DfgConst>();
        જો (!idxp) પાછા;
        DfgVarArray* અચલ varp = vtxp->fromp()->cast<DfgVarArray>();
        જો (!varp) પાછા;
        AstVar* અચલ astVarp = varp->vscp()->varp();
        જો (astVarp->isForced()) પાછા;
        જો (astVarp->isSigUserRWPublic()) પાછા;
        DfgVertex* અચલ srcp = varp->srcp();
        જો (!srcp) {
            જો (vtxp->isPacked()) {
                જો (AstConsArray* અચલ iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    જો (AstConst* અચલ valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(નવું DfgConst{m_dfg, valp->fileline(), valp->અંક()});
                            પાછા;
                        }
                    }
                }
            }
            પાછા;
        }

        જો (DfgSpliceArray* અચલ splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* અચલ driverp = splicep->driverAt(idxp->toSizeT());
            જો (!driverp) પાછા;
            DfgUnitArray* અચલ uap = driverp->cast<DfgUnitArray>();
            જો (!uap) પાછા;
            જો (uap->srcp()->is<DfgVertexSplice>()) પાછા;
            // If driven by a variable that had a Driver in DFG, it is partial
            જો (DfgVertexVar* અચલ dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                જો (dvarp->srcp()) પાછા;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                પાછા;
            }
        }

        જો (DfgUnitArray* અચલ uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            જો (uap->srcp()->is<DfgSplicePacked>()) પાછા;
            // If driven by a variable that had a Driver in DFG, it is partial
            જો (DfgVertexVar* અચલ dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                જો (dvarp->srcp()) પાછા;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgConcat* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();

        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isZero(lhsp)) {
            DfgConst* અચલ lConstp = lhsp->as<DfgConst>();
            જો (DfgSel* અચલ rSelp = rhsp->cast<DfgSel>()) {
                જો (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_ટોચ_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        પાછા;
                    }
                }
            }
        }

        જો (isZero(rhsp)) {
            DfgConst* અચલ rConstp = rhsp->as<DfgConst>();
            જો (DfgSel* અચલ lSelp = lhsp->cast<DfgSel>()) {
                જો (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        પાછા;
                    }
                }
            }
        }

        જો (DfgNot* અચલ lNot = lhsp->cast<DfgNot>()) {
            જો (DfgNot* અચલ rNot = rhsp->cast<DfgNot>()) {
                જો (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(ધકેલો_CONCAT_THROUGH_NOTS) {
                        DfgConcat* અચલ newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        પાછા;
                    }
                }
            }
        }

        {
            તાર્કિક swap = ખોટું;
            જો (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(ધકેલો_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    પાછા;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            જો (DfgConcat* અચલ rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* અચલ rlp = rConcatp->lhsp();
                જો (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(ધકેલો_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* અચલ newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        પાછા;
                    }
                }
            }
        }

        જો (DfgSel* અચલ lSelp = lhsp->cast<DfgSel>()) {
            જો (DfgSel* અચલ rSelp = rhsp->cast<DfgSel>()) {
                જો (isSame(lSelp->fromp(), rSelp->fromp())) {
                    જો (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            પાછા;
                        }
                    }
                }
            }

            જો (DfgConcat* અચલ rConcatp = rhsp->cast<DfgConcat>()) {
                જો (DfgSel* અચલ rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    જો (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        જો (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                અચલ uint32_t width = lSelp->width() + rlSelp->width();
                                અચલ DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* અચલ selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                પાછા;
                            }
                        }
                    }
                }
            }
        }

        જો (DfgSel* અચલ rSelp = rhsp->cast<DfgSel>()) {
            જો (DfgConcat* અચલ lConcatp = lhsp->cast<DfgConcat>()) {
                જો (DfgSel* અચલ lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    જો (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        જો (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                અચલ uint32_t width = lrSelp->width() + rSelp->width();
                                અચલ DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* અચલ selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                પાછા;
                            }
                        }
                    }
                }
            }
        }

        જો (DfgConst* અચલ lConstp = lhsp->cast<DfgConst>()) {
            જો (DfgCond* અચલ rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* અચલ rtVtxp = rCondp->thenp();
                DfgVertex* અચલ reVtxp = rCondp->elsep();
                DfgConst* અચલ rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* અચલ reConstp = reVtxp->cast<DfgConst>();
                જો (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(ધકેલો_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* અચલ thenp = [&]() -> DfgVertex* {
                            File_લીટી* અચલ rtFlp = rtVtxp->fileline();
                            જો (rtConstp) {
                                DfgConst* અચલ constp = makeZero(rtFlp, vtxp->width());
                                constp->અંક().opConcat(lConstp->અંક(), rtConstp->અંક());
                                પાછા constp;
                            }
                            પાછા make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* અચલ elsep = [&]() -> DfgVertex* {
                            File_લીટી* અચલ reFlp = reVtxp->fileline();
                            જો (reConstp) {
                                DfgConst* અચલ constp = makeZero(reFlp, vtxp->width());
                                constp->અંક().opConcat(lConstp->અંક(), reConstp->અંક());
                                પાછા constp;
                            }
                            પાછા make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        પાછા;
                    }
                }
            }
        }

        જો (DfgConst* અચલ rConstp = rhsp->cast<DfgConst>()) {
            જો (DfgCond* અચલ lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* અચલ ltVtxp = lCondp->thenp();
                DfgVertex* અચલ leVtxp = lCondp->elsep();
                DfgConst* અચલ ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* અચલ leConstp = leVtxp->cast<DfgConst>();
                જો (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(ધકેલો_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* અચલ thenp = [&]() -> DfgVertex* {
                            File_લીટી* અચલ ltFlp = ltVtxp->fileline();
                            જો (ltConstp) {
                                DfgConst* અચલ constp = makeZero(ltFlp, vtxp->width());
                                constp->અંક().opConcat(ltConstp->અંક(), rConstp->અંક());
                                પાછા constp;
                            }
                            પાછા make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* અચલ elsep = [&]() -> DfgVertex* {
                            File_લીટી* અચલ leFlp = leVtxp->fileline();
                            જો (leConstp) {
                                DfgConst* અચલ constp = makeZero(leFlp, vtxp->width());
                                constp->અંક().opConcat(leConstp->અંક(), rConstp->અંક());
                                પાછા constp;
                            }
                            પાછા make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        પાછા;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            અચલ uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            અચલ uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            અચલ uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            તાર્કિક hasCrossSink = ખોટું;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                જો (અચલ DfgSel* અચલ selp = sink.cast<DfgSel>()) {
                    અચલ uint32_t selLsb = selp->lsb();
                    અચલ uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    પાછા ખોટું;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                જો (અચલ DfgVarPacked* અચલ varp = sink.cast<DfgVarPacked>()) {
                    જો (!varp->hasSinks() && !varp->isObserved()) પાછા ખોટું;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                પાછા સાચું;
            });
            જો (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    જો (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    જો (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* અચલ catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* અચલ sp = નવું DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* અચલ varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(સાચું);
                    varp->srcp(sp);
                    replace(varp);
                    પાછા;
                }
            }
        }

        // Convert various forms to REPLICATE
        જો (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                પાછા;
            }
        }
        જો (DfgRep* અચલ lRepp = lhsp->cast<DfgRep>()) {
            જો (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    પાછા;
                }
            }
        }
        જો (DfgRep* અચલ rRepp = rhsp->cast<DfgRep>()) {
            જો (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    પાછા;
                }
            }
        }
        જો (DfgRep* અચલ lRepp = lhsp->cast<DfgRep>()) {
            જો (DfgRep* અચલ rRepp = rhsp->cast<DfgRep>()) {
                જો (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        પાછા;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            જો (DfgConcat* અચલ rConcatp = rhsp->cast<DfgConcat>()) {
                જો (DfgRep* અચલ rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    જો (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            અચલ uint32_t width = lRepp->width() + rlRepp->width();
                            અચલ DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* અચલ repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            પાછા;
                        }
                    }
                }
            }
        }
    }

    શૂન્ય visit(DfgDiv* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        જો (DfgConst* અચલ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            જો (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgDivS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        જો (DfgConst* અચલ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            જો (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgGt* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgGtS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgGte* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgGteS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgLogAnd* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp) || vtxp->rhsp()->unsafe()) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();

        જો (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgLogEq* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgLogIf* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp) || vtxp->rhsp()->unsafe()) પાછા;
    }

    શૂન્ય visit(DfgLogOr* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp) || vtxp->rhsp()->unsafe()) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();

        જો (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgLt* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgLtS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgLte* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgLteS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                પાછા;
            }
        }
    }

    શૂન્ય visit(DfgModDiv* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgModDivS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgMul* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (DfgConst* અચલ lConstp = lhsp->cast<DfgConst>()) {
            જો (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    પાછા;
                }
            }
            જો (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgMulS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (DfgConst* અચલ lConstp = lhsp->cast<DfgConst>()) {
            જો (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    પાછા;
                }
            }
            જો (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgEq* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                પાછા;
            }
        }

        જો (DfgConst* અચલ lhsConstp = lhsp->cast<DfgConst>()) {
            જો (DfgConcat* અચલ rhsConcatp = rhsp->cast<DfgConcat>()) {
                જો (try_ધકેલો_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) પાછા;
            }

            જો (rhsp->dtype() == m_bitDType) {
                જો (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        પાછા;
                    }
                }

                જો (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        પાછા;
                    }
                }
            }
        }
    }

    શૂન્ય visit(DfgNeq* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                પાછા;
            }
        }

        જો (DfgConst* અચલ lhsConstp = lhsp->cast<DfgConst>()) {
            જો (DfgConcat* અચલ rhsConcatp = rhsp->cast<DfgConcat>()) {
                જો (try_ધકેલો_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) પાછા;
            }

            જો (rhsp->dtype() == m_bitDType) {
                જો (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        પાછા;
                    }
                }

                જો (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        પાછા;
                    }
                }
            }
        }
    }

    શૂન્ય visit(DfgPow* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgPowSS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgPowSU* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgPowUS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgRep* અચલ vtxp) override {
        DfgVertex* અચલ srcp = vtxp->srcp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (DfgConst* અચલ sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* અચલ resultp = makeZero(flp, vtxp->width());
                resultp->અંક().opRepl(sConstp->અંક(), vtxp->ગણતરી());
                replace(resultp);
                પાછા;
            }
        }

        જો (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                પાછા;
            }
        }

        જો (DfgRep* અચલ sRepp = srcp->cast<DfgRep>()) {
            જો (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgShiftL* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
        જો (optimizeShiftRHS(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();

        જો (DfgShiftL* અચલ lShiftLp = lhsp->cast<DfgShiftL>()) {
            જો (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_લીટી* અચલ flp = vtxp->fileline();
                    અચલ DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* અચલ bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* અચલ cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* અચલ addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    પાછા;
                }
            }
        }

        જો (DfgConst* અચલ rConstp = rhsp->cast<DfgConst>()) {
            અચલ uint32_t shiftAmount = rConstp->toU32();

            જો (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    પાછા;
                }
            }

            જો (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    પાછા;
                }
            }

            જો (DfgConcat* અચલ lConcatp = lhsp->cast<DfgConcat>()) {
                જો (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* અચલ lRhsp = lConcatp->rhsp();
                        DfgVertex* અચલ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        અચલ uint32_t rW = shiftAmount;
                        અચલ uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        અચલ uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_લીટી* અચલ flp = lConcatp->fileline();
                        DfgVertex* અચલ rp = makeZero(flp, shiftAmount);
                        DfgVertex* અચલ mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        જો (!lW) {
                            replace(np);
                            પાછા;
                        }
                        DfgVertex* અચલ lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        પાછા;
                    }
                }
            }

            જો (DfgSel* અચલ lSelp = lhsp->cast<DfgSel>()) {
                જો (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        અચલ uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* અચલ nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        પાછા;
                    }
                }
            }

            જો (DfgCond* અચલ lCondp = lhsp->cast<DfgCond>()) {
                જો (!lCondp->hasMultipleSinks()) {
                    APPLYING(ધકેલો_SHIFTL_THROUGH_COND) {
                        DfgShiftL* અચલ tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* અચલ ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        પાછા;
                    }
                }
            }
        }
    }

    શૂન્ય visit(DfgShiftR* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
        જો (optimizeShiftRHS(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();

        જો (DfgShiftR* અચલ lShiftRp = lhsp->cast<DfgShiftR>()) {
            જો (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_લીટી* અચલ flp = vtxp->fileline();
                    અચલ DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* અચલ bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* અચલ cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* અચલ addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    પાછા;
                }
            }
        }

        જો (DfgConst* અચલ rConstp = rhsp->cast<DfgConst>()) {
            અચલ uint32_t shiftAmount = rConstp->toU32();

            જો (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    પાછા;
                }
            }

            જો (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    પાછા;
                }
            }

            જો (DfgConcat* અચલ lConcatp = lhsp->cast<DfgConcat>()) {
                જો (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* અચલ lRhsp = lConcatp->rhsp();
                        DfgVertex* અચલ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        અચલ uint32_t lW = shiftAmount;
                        અચલ uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        અચલ uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_લીટી* અચલ flp = lConcatp->fileline();
                        DfgVertex* અચલ lp = makeZero(flp, shiftAmount);
                        DfgVertex* અચલ mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        જો (!rW) {
                            replace(np);
                            પાછા;
                        }
                        DfgVertex* અચલ rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        પાછા;
                    }
                }
            }

            જો (DfgSel* અચલ lSelp = lhsp->cast<DfgSel>()) {
                જો (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        અચલ uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* અચલ nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        પાછા;
                    }
                }
            }

            જો (DfgCond* અચલ lCondp = lhsp->cast<DfgCond>()) {
                જો (!lCondp->hasMultipleSinks()) {
                    APPLYING(ધકેલો_SHIFTR_THROUGH_COND) {
                        DfgShiftR* અચલ tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* અચલ ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        પાછા;
                    }
                }
            }
        }
    }

    શૂન્ય visit(DfgShiftRS* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;
        જો (optimizeShiftRHS(vtxp)) પાછા;
    }

    શૂન્ય visit(DfgSub* અચલ vtxp) override {
        જો (દ્વિઅંકી(vtxp)) પાછા;

        DfgVertex* અચલ lhsp = vtxp->lhsp();
        DfgVertex* અચલ rhsp = vtxp->rhsp();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                પાછા;
            }
        }

        જો (DfgConst* અચલ rConstp = rhsp->cast<DfgConst>()) {
            જો (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    પાછા;
                }
            }
            જો (vtxp->dtype() == m_bitDType && rConstp->has_કિંમત(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgMatchMasked* અચલ vtxp) override {
        જો (DfgConst* અચલ constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* અચલ matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->અંક(), matchVarp)));
                પાછા;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    શૂન્ય visit(DfgCond* અચલ vtxp) override {
        DfgVertex* અચલ condp = vtxp->condp();
        DfgVertex* અચલ thenp = vtxp->thenp();
        DfgVertex* અચલ elsep = vtxp->elsep();
        File_લીટી* અચલ flp = vtxp->fileline();

        જો (condp->dtype() != m_bitDType) પાછા;

        જો (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                પાછા;
            }
        }

        જો (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                પાછા;
            }
        }

        જો (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                પાછા;
            }
        }

        જો (DfgNot* અચલ condNotp = condp->cast<DfgNot>()) {
            જો (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    પાછા;
                }
            }
        }

        જો (DfgNeq* અચલ condNeqp = condp->cast<DfgNeq>()) {
            જો (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* અચલ newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    પાછા;
                }
            }
        }

        જો (DfgNot* અચલ thenNotp = thenp->cast<DfgNot>()) {
            જો (DfgNot* અચલ elseNotp = elsep->cast<DfgNot>()) {
                જો (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* અચલ newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        પાછા;
                    }
                }
            }
        }

        જો (DfgOr* અચલ condOrp = condp->cast<DfgOr>()) {
            જો (DfgCond* અચલ thenCondp = thenp->cast<DfgCond>()) {
                જો (!thenCondp->hasMultipleSinks()) {
                    જો (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* અચલ ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            પાછા;
                        }
                    }
                    જો (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* અચલ ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            પાછા;
                        }
                    }
                }
            }
        }

        જો (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            જો (DfgAdd* અચલ thenAddp = thenp->cast<DfgAdd>()) {
                જો (DfgConst* અચલ constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    જો (constp->has_કિંમત(1)) {
                        જો (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* અચલ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_લીટી* અચલ thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                પાછા;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            જો (DfgSub* અચલ thenSubp = thenp->cast<DfgSub>()) {
                જો (DfgConst* અચલ constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    જો (constp->has_કિંમત(1)) {
                        જો (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* અચલ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_લીટી* અચલ thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                પાછા;
                            }
                        }
                    }
                }
            }
        }

        જો (vtxp->dtype() == m_bitDType) {
            જો (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    પાછા;
                }
            }
            જો (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    પાછા;
                }
            }
        }

        જો (vtxp->width() <= VL_QUADSIZE) {
            જો (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* અચલ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    પાછા;
                }
            }
            જો (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* અચલ maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    પાછા;
                }
            }
            જો (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* અચલ maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    પાછા;
                }
            }
            જો (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* અચલ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    પાછા;
                }
            }

            જો (DfgOr* અચલ tOrp = thenp->cast<DfgOr>()) {
                જો (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* અચલ maskp = replicate(vtxp, condp);
                        DfgAnd* અચલ andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        પાછા;
                    }
                }
                જો (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* અચલ maskp = replicate(vtxp, condp);
                        DfgAnd* અચલ andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        પાછા;
                    }
                }
            }
        }

        જો (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            અચલ std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            જો (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    અચલ uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    જો (rWidth != rhsp->width()) {
                        અચલ DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    જો (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        પાછા;
                    }
                    // Create new LHS
                    અચલ uint32_t lWidth = vtxp->width() - rWidth;
                    અચલ DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* અચલ lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* અચલ lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* અચલ lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    પાછા;
                }
            }

            અચલ std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            જો (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    અચલ uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    જો (lWidth != lhsp->width()) {
                        અચલ DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    જો (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        પાછા;
                    }
                    // Create new RHS
                    અચલ uint32_t rWidth = vtxp->width() - lWidth;
                    અચલ DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* અચલ rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* અચલ rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* અચલ rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    પાછા;
                }
            }
        }

        જો (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                જો (અચલ uint32_t extend = vtxp->width() - 1) {
                    DfgConst* અચલ zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                પાછા;
            }
        }

        જો (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                જો (અચલ uint32_t extend = vtxp->width() - 1) {
                    DfgConst* અચલ zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                પાછા;
            }
        }

        જો (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                પાછા;
            }
        }

        જો (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                પાછા;
            }
        }

        જો (DfgCond* અચલ tCondp = thenp->cast<DfgCond>()) {
            જો (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    પાછા;
                }
            }
        }

        જો (DfgCond* અચલ eCondp = elsep->cast<DfgCond>()) {
            જો (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    પાછા;
                }
            }
        }
    }

    શૂન્ય visit(DfgVertexVar* અચલ vtxp) override {
        જો (vtxp->hasSinks()) પાછા;
        જો (vtxp->isObserved()) પાછા;
        જો (vtxp->defaultp()) પાછા;

        // If undriven, or driven from another var, it is completely redundant.
        જો (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                પાછા;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        તાર્કિક foundOne = ખોટું;
        અચલ તાર્કિક keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> તાર્કિક {
            // Ignore non-observable variable sinks. These can be eliminated.
            જો (અચલ DfgVertexVar* અચલ varp = sink.cast<DfgVertexVar>()) {
                જો (!varp->hasSinks() && !varp->isObserved()) પાછા ખોટું;
            }
            // Keep if found more than one sink
            જો (foundOne) પાછા સાચું;
            // Mark first sink found
            foundOne = સાચું;
            પાછા ખોટું;
        });
        જો (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                પાછા;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        માટે (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ધકેલો_back(nullptr);
        m_iterList.ધકેલો_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        માટે (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        માટે (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        જ્યારે (સાચું) {
            // Process the work list - keep the placeholder at index 0
            જ્યારે (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.કાઢો_back();
                જો (!m_vtxp) ચાલુ;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                જો (DfgVertexVar* અચલ varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    ચાલુ;
                }

                // Ast references are not considered
                જો (m_vtxp->is<DfgVertexAst>()) ચાલુ;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                જો (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    જો (DfgVertex* અચલ sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            ચાલુ;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            જો (m_iterList.size() == 1) તોડો;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                માટે (size_t hops = 1; hops <= 4; ++hops) {
                    માટે (size_t i = begin; i < end; ++i) {
                        DfgVertex* અચલ vtxp = m_iterList[i];
                        જો (!vtxp) ચાલુ;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            પાછા ખોટું;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            પાછા ખોટું;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            માટે (DfgVertex* અચલ vtxp : m_iterList) {
                જો (!vtxp) ચાલુ;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

જાહેર:
    સ્થિર શૂન્ય apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

શૂન્ય V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    જો (!v3Global.opt.fDfgPeephole()) પાછા;
    V3DfgPeephole::apply(dfg, ctx);
}
