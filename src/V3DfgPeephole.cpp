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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Ast.h"
#समावेश "V3Dfg.h"
#समावेश "V3DfgCache.h"
#समावेश "V3DfgDataType.h"
#समावेश "V3DfgPasses.h"
#समावेश "V3DfgPeepholePatterns.h"

#समावेश <algorithm>
#समावेश <cctype>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
namespace {

ढाँचा <typename T_Reduction>
काष्ठा ReductionToBitwiseImpl {};
ढाँचा <> काष्ठा ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
ढाँचा <> काष्ठा ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
ढाँचा <> काष्ठा ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
ढाँचा <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

ढाँचा <typename T_Bitwise>
काष्ठा BitwiseToReductionImpl {};
ढाँचा <> काष्ठा BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
ढाँचा <> काष्ठा BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
ढाँचा <> काष्ठा BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
ढाँचा <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
ढाँचा<typename Vertex>
काष्ठा IsAssociative : खुला std::false_type {};
ढाँचा <> काष्ठा IsAssociative<DfgAnd> : खुला std::true_type {};
ढाँचा <> काष्ठा IsAssociative<DfgOr>  : खुला std::true_type {};
ढाँचा <> काष्ठा IsAssociative<DfgXor> : खुला std::true_type {};
ढाँचा <> काष्ठा IsAssociative<DfgAdd> : खुला std::true_type {};
ढाँचा <> काष्ठा IsAssociative<DfgConcat> : खुला std::true_type {};
ढाँचा <> काष्ठा IsAssociative<DfgMul> : खुला std::true_type {};
ढाँचा <> काष्ठा IsAssociative<DfgMulS> : खुला std::true_type {};

// Commutative binary operators (a op b == b op a)
ढाँचा<typename Vertex>
काष्ठा IsCommutative : खुला std::false_type {};
ढाँचा <> काष्ठा IsCommutative<DfgAnd> : खुला std::true_type {};
ढाँचा <> काष्ठा IsCommutative<DfgOr>  : खुला std::true_type {};
ढाँचा <> काष्ठा IsCommutative<DfgXor> : खुला std::true_type {};
ढाँचा <> काष्ठा IsCommutative<DfgAdd> : खुला std::true_type {};
ढाँचा <> काष्ठा IsCommutative<DfgEq> : खुला std::true_type {};
ढाँचा <> काष्ठा IsCommutative<DfgMul> : खुला std::true_type {};
ढाँचा <> काष्ठा IsCommutative<DfgMulS> : खुला std::true_type {};
ढाँचा <> काष्ठा IsCommutative<DfgNeq> : खुला std::true_type {};

// Idempotent binary operators (a op a == a)
ढाँचा<typename Vertex>
काष्ठा IsIdempotent : खुला std::false_type {};
ढाँचा <> काष्ठा IsIdempotent<DfgAnd> : खुला std::true_type {};
ढाँचा <> काष्ठा IsIdempotent<DfgOr>  : खुला std::true_type {};

// Binary result dtype
ढाँचा<typename Vertex>
स्थिर Dfg_तथ्य_Type& परिणाम_DType(स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp);
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgAdd>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgAnd>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgConcat>     (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(lhsp->width() + rhsp->width()); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgDiv>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgDivS>       (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgEq>         (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgGt>         (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgGtS>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgGte>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgGteS>       (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgLogAnd>     (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgLogEq>      (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgLogIf>      (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgLogOr>      (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgLt>         (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgLtS>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgLte>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgLteS>       (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgModDiv>     (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgModDivS>    (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgMul>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgMulS>       (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgNeq>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस Dfg_तथ्य_Type::packed(1); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgOr>         (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgPow>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgPowSS>      (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgPowSU>      (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgPowUS>      (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgShiftL>     (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgShiftR>     (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgShiftRS>    (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgSub>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }
ढाँचा <> स्थिर Dfg_तथ्य_Type& परिणाम_DType<DfgXor>        (स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) { वापस lhsp->dtype(); }

// Unary constant folding
ढाँचा<typename Vertex> व्योम foldOp(V3_संख्या& out, स्थिर V3_संख्या& src);
ढाँचा <> व्योम foldOp<Dfg_गिनती_Ones>  (V3_संख्या& out, स्थिर V3_संख्या& src) { out.op_गिनती_Ones(src); }
ढाँचा <> व्योम foldOp<DfgExtend>     (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opAssign(src); }
ढाँचा <> व्योम foldOp<DfgExtendS>    (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opExtendS(src, src.width()); }
ढाँचा <> व्योम foldOp<DfgLogNot>     (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opLogNot(src); }
ढाँचा <> व्योम foldOp<DfgNegate>     (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opNegate(src); }
ढाँचा <> व्योम foldOp<DfgNot>        (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opNot(src); }
ढाँचा <> व्योम foldOp<DfgOneHot>     (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opOneHot(src); }
ढाँचा <> व्योम foldOp<DfgOneHot0>    (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opOneHot0(src); }
ढाँचा <> व्योम foldOp<DfgRedAnd>     (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opRedAnd(src); }
ढाँचा <> व्योम foldOp<DfgRedOr>      (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opRedOr(src); }
ढाँचा <> व्योम foldOp<DfgRedXor>     (V3_संख्या& out, स्थिर V3_संख्या& src) { out.opRedXor(src); }

// Binary constant folding
ढाँचा<typename Vertex> व्योम foldOp(V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
ढाँचा <> व्योम foldOp<DfgAdd>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opAdd(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgAnd>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opAnd(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgConcat>     (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opConcat(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgDiv>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opDiv(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgDivS>       (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opDivS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgEq>         (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opEq(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgGt>         (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opGt(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgGtS>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opGtS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgGte>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opGte(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgGteS>       (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opGteS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgLogAnd>     (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opLogAnd(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgLogEq>      (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opLogEq(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgLogIf>      (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opLogIf(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgLogOr>      (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opLogOr(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgLt>         (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opLt(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgLtS>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opLtS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgLte>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opLte(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgLteS>       (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opLteS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgModDiv>     (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opModDiv(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgModDivS>    (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opModDivS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgMul>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opMul(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgMulS>       (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opMulS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgNeq>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opNeq(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgOr>         (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opOr(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgPow>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opPow(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgPowSS>      (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opPowSS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgPowSU>      (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opPowSU(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgPowUS>      (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opPowUS(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgShiftL>     (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opShiftL(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgShiftR>     (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opShiftR(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgShiftRS>    (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
ढाँचा <> व्योम foldOp<DfgSub>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opSub(lhs, rhs); }
ढाँचा <> व्योम foldOp<DfgXor>        (V3_संख्या& out, स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

श्रेणी V3DfgPeephole final : खुला DfgVisitor {
    // TYPES
    काष्ठा VertexInfo final {
        माप_प्रकार m_work_List_सूचकांक = 0;  // Position of this vertx m_workList (0 means not in list)
        माप_प्रकार m_iter_List_सूचकांक = 0;  // Position of this vertx m_iterList (0 means not in list)
        माप_प्रकार m_generation = 0;  // Generation number of this vertex - for uniqueness check
        माप_प्रकार m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        bool m_isCachedVertex = false;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    स्थिर Dfg_तथ्य_Type& m_bitDType = Dfg_तथ्य_Type::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    माप_प्रकार m_currentGeneration = 0;  // Current generation number
    माप_प्रकार m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* स्थिर m_tmpScopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();

    // STATIC STATE
    अटल V3DebugBisect s_debugBisect;  // Debug aid

#घोषणा APPLYING(id) अगर (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    bool checkApplying(VDfgPeepholePattern id) {
        अगर (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) वापस false;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_गिनती[id];
        वापस true;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* स्थिर equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        वापस equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    व्योम invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = false;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_पंक्ति* flp, स्थिर Dfg_तथ्य_Type& dtype, Operands... operands) {
        Vertex* स्थिर vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = true;
        वापस vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* getVertex(स्थिर Dfg_तथ्य_Type& dtype, Operands... operands) {
        Vertex* स्थिर vtxp = m_cache.लो<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        वापस vtxp;
    }

    व्योम incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    व्योम addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        अगर (vInfo.m_work_List_सूचकांक) वापस;
        // Add to work list
        vInfo.m_work_List_सूचकांक = m_workList.आकार();
        m_workList.धकेलो_back(vtxp);
    }

    व्योम removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_work_List_सूचकांक] = nullptr;
        vInfo.m_work_List_सूचकांक = 0;
    }

    व्योम addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        अगर (vInfo.m_iter_List_सूचकांक) वापस;
        // Add to iter list
        vInfo.m_iter_List_सूचकांक = m_iterList.आकार();
        m_iterList.धकेलो_back(vtxp);
    }

    व्योम removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iter_List_सूचकांक] = nullptr;
        vInfo.m_iter_List_सूचकांक = 0;
    }

    व्योम deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_work_List_सूचकांक, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        स्थिर DfgVertexVar* स्थिर varp = vtxp->cast<DfgVertexVar>();
        UASSERT_OBJ(!varp || !varp->has_पूर्व(), vtxp, "Deleting variable consumed via DfgPrev");

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
            अगर (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                वापस false;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            अगर (vInfo.m_generation != m_currentGeneration) srcps.धकेलो_back(&src);
            vInfo.m_generation = m_currentGeneration;
            वापस false;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        अगर (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.धकेलो_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } अन्यथा {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        स्थिर स्वतः mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  वापस srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        क्रम (स्वतः it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        क्रम (स्वतः it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    व्योम replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_work_List_सूचकांक, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        स्थिर स्वतः debugCallback = [&]() -> व्योम {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            स्थिर स्वतः cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](स्थिर DfgVertex& v) {  //
                वापस cone->गिनती(&v);
            });
        };
        अगर (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) वापस;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            वापस false;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            वापस false;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            वापस false;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            वापस false;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_पंक्ति* flp, uint32_t val) { वापस नया DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_पंक्ति* flp, uint32_t width) {
        वापस नया DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_पंक्ति* flp, स्थिर Dfg_तथ्य_Type& dtype) {
        वापस makeZero(flp, dtype.आकार());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_पंक्ति* flp, uint32_t width) {
        DfgConst* स्थिर resp = makeZero(flp, width);
        resp->अंक().setAllBits1();
        वापस resp;
    }

    // Create a new vertex of the given type
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* make(File_पंक्ति* flp, स्थिर Dfg_तथ्य_Type& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* स्थिर vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        अगर (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        अगर (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        वापस vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* make(स्थिर DfgVertex* examplep, Operands... operands) {
        वापस make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        अगर (vtxp->dtype() == m_bitDType) वापस bitp;
        वापस make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    अटल bool isSame(स्थिर DfgVertex* ap, स्थिर DfgVertex* bp) {
        अगर (ap == bp) वापस true;
        अगर (ap->dtype() != bp->dtype()) वापस false;
        स्थिर DfgConst* स्थिर aConstp = ap->cast<DfgConst>();
        अगर (!aConstp) वापस false;
        स्थिर DfgConst* स्थिर bConstp = bp->cast<DfgConst>();
        अगर (!bConstp) वापस false;
        वापस aConstp->अंक().isCaseEq(bConstp->अंक());
    }

    अटल bool isZero(स्थिर DfgVertex* vtxp) {
        अगर (स्थिर DfgConst* स्थिर constp = vtxp->cast<DfgConst>()) वापस constp->isZero();
        वापस false;
    }

    अटल bool isOnes(स्थिर DfgVertex* vtxp) {
        अगर (स्थिर DfgConst* स्थिर constp = vtxp->cast<DfgConst>()) वापस constp->isOnes();
        वापस false;
    }

    अटल bool isEqOne(स्थिर DfgVertex* vtxp) {
        अगर (स्थिर DfgConst* स्थिर constp = vtxp->cast<DfgConst>()) वापस constp->अंक().isEqOne();
        वापस false;
    }

    अटल bool areAdjacent(uint32_t& lsb, स्थिर DfgSel* lSelp, स्थिर DfgSel* rSelp) {
        अगर (!isSame(lSelp->srcp(), rSelp->srcp())) वापस false;
        अगर (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            वापस true;
        }
        अगर (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            वापस true;
        }
        वापस false;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    ढाँचा <typename Vertex>
    VL_ATTR_WARN_UNUSED_परिणाम bool foldUnary(Vertex* स्थिर vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::मूल्य, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::मूल्य, "Must invoke on final class");
        अगर (DfgConst* स्थिर srcp = vtxp->srcp()->ढाँचा cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* स्थिर resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->अंक(), srcp->अंक());
                replace(resultp);
                वापस true;
            }
        }
        वापस false;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    ढाँचा <typename Vertex>
    VL_ATTR_WARN_UNUSED_परिणाम bool द्विग(Vertex* स्थिर vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विआधारी, Vertex>::मूल्य, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::मूल्य, "Must invoke on final class");

        File_पंक्ति* स्थिर flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* स्थिर lConstp = lhsp->cast<DfgConst>();
        DfgConst* स्थिर rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* स्थिर lSamep = lhsp->cast<Vertex>();
        Vertex* स्थिर rSamep = rhsp->cast<Vertex>();

        // Constant folding
        अगर (lConstp && rConstp) {
            APPLYING(FOLD_द्विआधारी) {
                DfgConst* स्थिर resultp = makeZero(flp, परिणाम_DType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->अंक(), lConstp->अंक(), rConstp->अंक());
                replace(resultp);
                वापस true;
            }
        }

        // Idempotent
        अगर VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::मूल्य) {
            अगर (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_द्विआधारी) {
                    replace(lhsp);
                    वापस true;
                }
            }
        }

        // Associative
        अगर VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::मूल्य) {
            // Also commutative and idempotent
            अगर VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मूल्य && IsIdempotent<Vertex>::मूल्य) {
                अगर (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_द्विआधारी_LHS) {
                        replace(lSamep);
                        वापस true;
                    }
                }
                अगर (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_द्विआधारी_RHS) {
                        replace(rSamep);
                        वापस true;
                    }
                }
            }

            अगर (lConstp && rSamep) {
                अगर (DfgConst* स्थिर rlConstp = rSamep->lhsp()->ढाँचा cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_द्विआधारी_LHS_OF_RHS) {
                        DfgConst* स्थिर cp = makeZero(flp, परिणाम_DType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->अंक(), lConstp->अंक(), rlConstp->अंक());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        वापस true;
                    }
                }
            }

            अगर (lSamep && rConstp) {
                अगर (DfgConst* स्थिर lrConstp = lSamep->rhsp()->ढाँचा cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_द्विआधारी_RHS_OF_LHS) {
                        DfgConst* स्थिर cp = makeZero(flp, परिणाम_DType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->अंक(), lrConstp->अंक(), rConstp->अंक());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        वापस true;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            अगर (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* स्थिर ap = lSamep->lhsp();
                DfgVertex* स्थिर bp = lSamep->rhsp();
                DfgVertex* स्थिर cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                अगर (ap != cp && bp != cp) {
                    APPLYING(दायाँ_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        स्थिर Dfg_तथ्य_Type& childDType = परिणाम_DType<Vertex>(bp, cp);
                        Vertex* स्थिर bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        वापस true;
                    }
                }
            }

            अगर (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* स्थिर rlVtxp = rSamep->lhsp();
                DfgVertex* स्थिर rrVtxp = rSamep->rhsp();

                अगर VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मूल्य) {
                    अगर (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            वापस true;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                अगर (Vertex* स्थिर existingp
                    = getVertex<Vertex>(परिणाम_DType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    अगर (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_द्विआधारी_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            वापस true;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                अगर VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मूल्य) {
                    अगर (Vertex* स्थिर existingp
                        = getVertex<Vertex>(परिणाम_DType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        अगर (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_द्विआधारी_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                वापस true;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        अगर VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मूल्य) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            अगर (lConstp) {
                अगर (DfgCond* स्थिर rCondp = rhsp->cast<DfgCond>()) {
                    अगर (!rCondp->hasMultipleSinks()) {
                        APPLYING(धकेलो_COMMUTATIVE_द्विआधारी_THROUGH_COND) {
                            DfgVertex* स्थिर tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* स्थिर ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            वापस true;
                        }
                    }
                }
                वापस false;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        स्थिर bool swapSides = [&]() {
            // Don't swap if not commutative, duh
            अगर VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::मूल्य) वापस false;
            // Const go on left
            अगर (lConstp) वापस false;
            अगर (rConstp) वापस true;
            // Not go on left
            अगर (lhsp->is<DfgNot>()) वापस false;
            अगर (rhsp->is<DfgNot>()) वापस true;
            // Same go on right
            अगर (rSamep) वापस false;
            अगर (lSamep) वापस true;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            स्थिर VertexInfo& lInfo = m_vInfo[lhsp];
            स्थिर VertexInfo& rInfo = m_vInfo[rhsp];
            वापस lInfo.m_id > rInfo.m_id;
        }();
        अगर (swapSides) {
            APPLYING(SWAP_SIDES_IN_द्विआधारी) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                वापस true;
            }
        }

        वापस false;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    ढाँचा <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_परिणाम bool distributive_And_Associative_द्विआधारी(Vertex* vtxp) {
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        अगर (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            अगर (Other* स्थिर lp = lhsp->cast<Other>()) {
                अगर (Other* स्थिर rp = rhsp->cast<Other>()) {
                    DfgVertex* स्थिर llp = lp->lhsp();
                    DfgVertex* स्थिर lrp = lp->rhsp();
                    DfgVertex* स्थिर rlp = rp->lhsp();
                    DfgVertex* स्थिर rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    अगर (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } अन्यथा अगर (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } अन्यथा अगर (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } अन्यथा अगर (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    अगर (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_द्विआधारी) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            वापस true;
                        }
                    }
                }
            }
        }

        वापस false;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    ढाँचा <typename Vertex>
    VL_ATTR_WARN_UNUSED_परिणाम bool
    try_धकेलो_Bitwise_Op_Through_Concat(Vertex* स्थिर vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* स्थिर catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* स्थिर catRConstp = concatp->rhsp()->cast<DfgConst>();
        अगर (catLConstp || catRConstp) {
            APPLYING(धकेलो_BITWISE_OP_THROUGH_CONCAT) {
                स्थिर uint32_t width = concatp->width();
                स्थिर Dfg_तथ्य_Type& lDtype = concatp->lhsp()->dtype();
                स्थिर Dfg_तथ्य_Type& rDtype = concatp->rhsp()->dtype();
                स्थिर uint32_t lWidth = lDtype.आकार();
                स्थिर uint32_t rWidth = rDtype.आकार();

                // The new Lhs vertex
                DfgVertex* स्थिर newLhsp = [&]() -> DfgVertex* {
                    DfgConst* स्थिर newLhsConstp = makeZero(constp->fileline(), lWidth);
                    अगर (catLConstp) {
                        V3_संख्या अंक{constp->fileline(), static_cast<पूर्णांक>(lWidth), 0u};
                        अंक.opSel(constp->अंक(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->अंक(), अंक, catLConstp->अंक());
                        वापस newLhsConstp;
                    }
                    newLhsConstp->अंक().opSel(constp->अंक(), width - 1, rWidth);
                    वापस make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* स्थिर newRhsp = [&]() -> DfgVertex* {
                    DfgConst* स्थिर newRhsConstp = makeZero(constp->fileline(), rWidth);
                    अगर (catRConstp) {
                        V3_संख्या अंक{constp->fileline(), static_cast<पूर्णांक>(rWidth), 0u};
                        अंक.opSel(constp->अंक(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->अंक(), अंक, catRConstp->अंक());
                        वापस newRhsConstp;
                    }
                    newRhsConstp->अंक().opSel(constp->अंक(), rWidth - 1, 0);
                    वापस make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                वापस true;
            }
        }
        वापस false;
    }

    ढाँचा <typename Vertex>
    VL_ATTR_WARN_UNUSED_परिणाम bool
    try_धकेलो_Compare_Op_Through_Concat(Vertex* स्थिर vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        अगर (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(धकेलो_COMPARE_OP_THROUGH_CONCAT) {
                स्थिर uint32_t width = concatp->width();
                स्थिर uint32_t lWidth = concatp->lhsp()->width();
                स्थिर uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* स्थिर newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->अंक().opSel(constp->अंक(), width - 1, rWidth);
                Vertex* स्थिर newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* स्थिर newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->अंक().opSel(constp->अंक(), rWidth - 1, 0);
                Vertex* स्थिर newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                अगर VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::मूल्य) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } अन्यथा अगर VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::मूल्य) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } अन्यथा {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                वापस true;
            }
        }
        वापस false;
    }

    ढाँचा <typename Bitwise>
    VL_ATTR_WARN_UNUSED_परिणाम bool try_धकेलो_Bitwise_Op_Through_Reductions(Bitwise* स्थिर vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        अगर (Reduction* स्थिर lRedp = vtxp->lhsp()->ढाँचा cast<Reduction>()) {
            अगर (Reduction* स्थिर rRedp = vtxp->rhsp()->ढाँचा cast<Reduction>()) {
                DfgVertex* स्थिर lSrcp = lRedp->srcp();
                DfgVertex* स्थिर rSrcp = rRedp->srcp();
                अगर (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(धकेलो_BITWISE_THROUGH_REDUCTION) {
                        File_पंक्ति* स्थिर flp = vtxp->fileline();
                        Bitwise* स्थिर bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        वापस true;
                    }
                }
            }
        }

        वापस false;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    अटल std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        अगर (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            वापस {nullptr, nullptr};
        }
        अगर (vtxp->hasMultipleSinks()) वापस {nullptr, nullptr};
        Dfg_Vertex_द्विआधारी* स्थिर bwp = vtxp->as<Dfg_Vertex_द्विआधारी>();
        DfgSel* स्थिर lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* स्थिर rSelp = bwp->rhsp()->cast<DfgSel>();
        अगर (!lSelp || !rSelp) वापस {nullptr, nullptr};
        DfgSel* स्थिर nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* स्थिर wSelp = nSelp == lSelp ? rSelp : lSelp;
        स्थिर uint32_t nWidth = nSelp->fromp()->width();
        अगर (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) वापस {nullptr, nullptr};
        अगर (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            वापस {nullptr, nullptr};
        }
        वापस {nSelp, wSelp};
    }

    ढाँचा <typename Bitwise>
    VL_ATTR_WARN_UNUSED_परिणाम bool try_धकेलो_Bitwise_Op_Throug_Sel(Bitwise* स्थिर vtxp) {
        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();

        अगर (DfgSel* स्थिर lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            अगर (DfgSel* स्थिर selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } अन्यथा अगर (Bitwise* स्थिर bitwisep = rhsp->cast<Bitwise>()) {
                अगर (DfgSel* स्थिर rlSelp = bitwisep->lhsp()->ढाँचा cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } अन्यथा अगर (DfgSel* स्थिर rrSelp = bitwisep->rhsp()->ढाँचा cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            अगर (rSelp) {
                DfgVertex* स्थिर lFromp = lSelp->fromp();
                DfgVertex* स्थिर rFromp = rSelp->fromp();
                अगर (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(धकेलो_BITWISE_THROUGH_SEL) {
                        Bitwise* स्थिर bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        अगर (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        वापस true;
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
            Bitwise* स्थिर rChainp = rhsp->cast<Bitwise>();
            अगर (rChainp && rChainp->hasMultipleSinks()) वापस false;
            DfgVertex* स्थिर rTermp = rChainp ? rChainp->lhsp() : rhsp;

            स्थिर स्वतः lSels = pushableUnalignedSels(lhsp);
            स्थिर स्वतः rSels = pushableUnalignedSels(rTermp);
            अगर (!lSels.first && !rSels.first) वापस false;

            स्थिर DfgVertex* स्थिर opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            अगर (opp->is<Bitwise>()) वापस false;
            स्थिर स्वतः& sels = lSels.first ? lSels : rSels;
            DfgVertex* स्थिर nFromp = sels.first->fromp();
            DfgVertex* स्थिर wFromp = sels.second->fromp();
            स्थिर uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            स्थिर स्वतः matches = [&](स्थिर DfgVertex* termp, स्थिर स्वतः& tSels) {
                // Original form: 'n[i] OP w[j]'
                अगर (tSels.first) {
                    वापस termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                अगर (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                अगर (स्थिर DfgSel* स्थिर selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                अगर (termp->type() != opp->type()) वापस false;
                // Operands might have been swapped since
                स्थिर Dfg_Vertex_द्विआधारी* स्थिर bwp = termp->as<Dfg_Vertex_द्विआधारी>();
                स्थिर bool nIsLhs = bwp->lhsp() == nFromp;
                अगर (!nIsLhs && bwp->rhsp() != nFromp) वापस false;
                स्थिर DfgSel* स्थिर slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                वापस slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            अगर (!matches(lhsp, lSels) || !matches(rTermp, rSels)) वापस false;

            APPLYING(धकेलो_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_पंक्ति* स्थिर flp = vtxp->fileline();
                स्थिर Dfg_तथ्य_Type& dtype = nFromp->dtype();
                DfgSel* स्थिर slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                अगर (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } अन्यथा अगर (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } अन्यथा {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                स्थिर स्वतः धकेलो = [&](DfgVertex* termp, स्थिर DfgSel* nSelp) -> DfgVertex* {
                    वापस nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* स्थिर newLhsp = धकेलो(lhsp, lSels.first);
                DfgVertex* स्थिर newRTermp = धकेलो(rTermp, rSels.first);
                अगर (rChainp) {
                    DfgVertex* स्थिर newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } अन्यथा {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                वापस true;
            }
        }

        वापस false;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    अटल bool isConcatFoldable(स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        अगर (isSame(lhsp, rhsp)) वापस true;
        अगर (स्थिर DfgRep* स्थिर lRepp = lhsp->cast<DfgRep>()) {
            अगर (isSame(lRepp->srcp(), rhsp)) वापस true;
        }
        अगर (स्थिर DfgRep* स्थिर rRepp = rhsp->cast<DfgRep>()) {
            अगर (isSame(lhsp, rRepp->srcp())) वापस true;
        }
        // Concatenation of adjoining selects is a select
        अगर (स्थिर DfgSel* स्थिर lSelp = lhsp->cast<DfgSel>()) {
            अगर (स्थिर DfgSel* स्थिर rSelp = rhsp->cast<DfgSel>()) {
                वापस isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        अगर (स्थिर DfgNot* स्थिर lNotp = lhsp->cast<DfgNot>()) {
            अगर (स्थिर DfgNot* स्थिर rNotp = rhsp->cast<DfgNot>()) {
                वापस !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        वापस false;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    अटल bool isSameBitwise(स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) {
        अगर (lhsp->type() != rhsp->type()) वापस false;
        वापस lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    अटल bool isConcatSimplifiable(स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp) {
        अगर (isConcatFoldable(lhsp, rhsp)) वापस true;
        अगर (!isSameBitwise(lhsp, rhsp)) वापस false;
        अगर (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) वापस false;
        स्थिर Dfg_Vertex_द्विआधारी* स्थिर lp = lhsp->as<Dfg_Vertex_द्विआधारी>();
        स्थिर Dfg_Vertex_द्विआधारी* स्थिर rp = rhsp->as<Dfg_Vertex_द्विआधारी>();
        अगर (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            वापस true;
        अगर (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            वापस true;
        वापस false;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    अटल bool isConcatPushableThroughBitwise(स्थिर DfgVertex* lhsp, स्थिर DfgVertex* rhsp,
                                               bool& swap) {
        अगर (!isSameBitwise(lhsp, rhsp)) वापस false;
        अगर (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) वापस false;
        स्थिर Dfg_Vertex_द्विआधारी* स्थिर lp = lhsp->as<Dfg_Vertex_द्विआधारी>();
        स्थिर Dfg_Vertex_द्विआधारी* स्थिर rp = rhsp->as<Dfg_Vertex_द्विआधारी>();
        swap = false;
        अगर (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) वापस true;
        अगर (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) वापस true;
        swap = true;
        अगर (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) वापस true;
        अगर (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) वापस true;
        वापस false;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_पंक्ति* flp, स्थिर DfgVertex* lhsp,
                                              स्थिर DfgVertex* rhsp, bool swap) {
        स्थिर Dfg_Vertex_द्विआधारी* स्थिर lp = lhsp->as<Dfg_Vertex_द्विआधारी>();
        स्थिर Dfg_Vertex_द्विआधारी* स्थिर rp = rhsp->as<Dfg_Vertex_द्विआधारी>();
        स्थिर Dfg_तथ्य_Type& dtype = Dfg_तथ्य_Type::packed(lp->width() + rp->width());
        DfgVertex* स्थिर cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* स्थिर dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* स्थिर newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* स्थिर newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        अगर (lp->is<DfgAnd>()) वापस make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        अगर (lp->is<DfgOr>()) वापस make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        वापस make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    ढाँचा <typename Bitwise>
    VL_ATTR_WARN_UNUSED_परिणाम bool tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();

        अगर (DfgSel* स्थिर lSelp = lhsp->ढाँचा cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            अगर (DfgSel* स्थिर selp = rhsp->ढाँचा cast<DfgSel>()) {
                rSelp = selp;
            } अन्यथा अगर (Bitwise* स्थिर rBitwisep = rhsp->ढाँचा cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->ढाँचा cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } अन्यथा अगर (Reduction* स्थिर rRedp = rhsp->ढाँचा cast<Reduction>()) {
                rSelp = rRedp->srcp()->ढाँचा cast<DfgSel>();
            }
            अगर (rSelp) {
                uint32_t lsb = 0;
                अगर (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        स्थिर Dfg_तथ्य_Type& dtype
                            = Dfg_तथ्य_Type::packed(lSelp->width() + rSelp->width());
                        DfgSel* स्थिर newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        अगर (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        वापस true;
                    }
                }
            }
        }

        अगर (Reduction* स्थिर lRedp = lhsp->ढाँचा cast<Reduction>()) {
            Reduction* rRedp = rhsp->ढाँचा cast<Reduction>();
            DfgVertex* extrap = nullptr;
            अगर (!rRedp) {
                अगर (Bitwise* स्थिर rBitwisep = rhsp->ढाँचा cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->ढाँचा cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            अगर (rRedp) {
                अगर (DfgSel* स्थिर lSelp = lRedp->srcp()->ढाँचा cast<DfgSel>()) {
                    अगर (DfgSel* स्थिर rSelp = rRedp->srcp()->ढाँचा cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        अगर (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                स्थिर Dfg_तथ्य_Type& dtype
                                    = Dfg_तथ्य_Type::packed(lSelp->width() + rSelp->width());
                                DfgSel* स्थिर newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                अगर (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                वापस true;
                            }
                        }
                    }
                }
            }
        }

        वापस false;
    }

    ढाँचा <typename Reduction>
    VL_ATTR_WARN_UNUSED_परिणाम bool optimizeReduction(Reduction* स्थिर vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        अगर (foldUnary(vtxp)) वापस true;

        DfgVertex* स्थिर srcp = vtxp->srcp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        // Reduction of 1-bit value
        अगर (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                वापस true;
            }
        }

        अगर (DfgCond* स्थिर condp = srcp->cast<DfgCond>()) {
            अगर (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(धकेलो_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* स्थिर newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* स्थिर newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* स्थिर newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    वापस true;
                }
            }
        }

        अगर (DfgConcat* स्थिर concatp = srcp->cast<DfgConcat>()) {
            अगर (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(धकेलो_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* स्थिर lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* स्थिर rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    वापस true;
                }
            }
        }

        अगर (Bitwise* स्थिर bitwisep = vtxp->srcp()->ढाँचा cast<Bitwise>()) {
            अगर (!bitwisep->hasMultipleSinks()) {
                अगर (bitwisep->lhsp()->ढाँचा is<DfgConcat>()
                    || bitwisep->rhsp()->ढाँचा is<DfgConcat>()) {
                    APPLYING(धकेलो_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* स्थिर newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* स्थिर newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        वापस true;
                    }
                }

                अगर (DfgSel* स्थिर lSelp = bitwisep->lhsp()->ढाँचा cast<DfgSel>()) {
                    अगर (DfgSel* स्थिर rSelp = bitwisep->rhsp()->ढाँचा cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        अगर (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(धकेलो_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                स्थिर Dfg_तथ्य_Type& dtype
                                    = Dfg_तथ्य_Type::packed(lSelp->width() + rSelp->width());
                                DfgSel* स्थिर newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                वापस true;
                            }
                        }
                    }
                }
            }
        }

        वापस false;
    }

    ढाँचा <typename Shift>
    VL_ATTR_WARN_UNUSED_परिणाम bool optimizeShiftRHS(Shift* स्थिर vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विआधारी, Shift>::मूल्य, "Must invoke on binary");
        static_assert(std::is_final<Shift>::मूल्य, "Must invoke on final class");
        अगर (स्थिर DfgConcat* स्थिर concatp = vtxp->rhsp()->ढाँचा cast<DfgConcat>()) {
            अगर (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    वापस true;
                }
            }
        }
        वापस false;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_गिनती_Ones_Term(DfgVertex* vtxp) {
        अगर (DfgConcat* स्थिर oCatp = vtxp->cast<DfgConcat>()) {
            अगर (isZero(oCatp->lhsp())) {
                अगर (Dfg_गिनती_Ones* स्थिर गिनती_Onesp = oCatp->rhsp()->cast<Dfg_गिनती_Ones>()) {
                    // Zero extended count ones
                    अगर (DfgSel* स्थिर selp = गिनती_Onesp->srcp()->cast<DfgSel>()) {
                        वापस {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } अन्यथा अगर (DfgSel* स्थिर selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    अगर (selp->dtype() == m_bitDType) {  //
                        वापस {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            वापस {nullptr, 0, 0};
        }
        अगर (Dfg_गिनती_Ones* स्थिर गिनती_Onesp = vtxp->cast<Dfg_गिनती_Ones>()) {
            // Simple count ones
            अगर (DfgSel* स्थिर selp = गिनती_Onesp->srcp()->cast<DfgSel>()) {
                वापस {selp->fromp(), selp->lsb(), selp->width()};
            }
            वापस {nullptr, 0, 0};
        }
        अगर (DfgSel* स्थिर oSelp = vtxp->cast<DfgSel>()) {
            अगर (oSelp->lsb() == 0) {
                // Truncated count ones
                अगर (Dfg_गिनती_Ones* स्थिर गिनती_Onesp = oSelp->fromp()->cast<Dfg_गिनती_Ones>()) {
                    // Zero extended count ones
                    अगर (DfgSel* स्थिर selp = गिनती_Onesp->srcp()->cast<DfgSel>()) {
                        वापस {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            अगर (oSelp->dtype() == m_bitDType) {  //
                वापस {oSelp->fromp(), oSelp->lsb(), 1};
            }
            वापस {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        अगर (DfgShiftR* स्थिर shiftrp = vtxp->cast<DfgShiftR>()) {
            अगर (DfgConst* स्थिर rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                अगर (rConstp->toU32() == shiftrp->width() - 1) {
                    वापस {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            वापस {nullptr, 0, 0};
        }
        // Not applicable
        वापस {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              स्थिर uint32_t width) {
        जबतक (true) {
            स्थिर uint32_t msb = lsb + width - 1;

            // Sel from Concat
            अगर (DfgConcat* स्थिर concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* स्थिर lhsp = concatp->lhsp();
                DfgVertex* स्थिर rhsp = concatp->rhsp();

                अगर (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        जारी;
                    }
                } अन्यथा अगर (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        जारी;
                    }
                }
            }

            अगर (DfgRep* स्थिर repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                स्थिर uint32_t srcWidth = repp->srcp()->width();
                अगर (width <= srcWidth) {
                    स्थिर uint32_t newLsb = lsb % srcWidth;
                    स्थिर uint32_t newMsb = newLsb + width - 1;
                    अगर (newMsb < srcWidth) {
                        APPLYING(धकेलो_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            जारी;
                        }
                    }
                }
            }

            // Sel from Sel
            अगर (DfgSel* स्थिर selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    जारी;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            अगर (DfgVarPacked* स्थिर varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                स्थिर स्वतः pair = varp->driverOfRange(lsb, width);
                DfgVertex* स्थिर driverp = pair.first;
                स्थिर uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                अगर (driverp) {
                    अगर (driverp == varp->defaultp()) {
                        APPLYING(धकेलो_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            जारी;
                        }
                    } अन्यथा {
                        APPLYING(धकेलो_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            जारी;
                        }
                    }
                }
            }

            // No patterns matched, stop
            अवरोध;
        }
        वापस {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        अगर (ap == bp) वापस {ap, ap->width()};

        // If both constants, check LSBs
        अगर (DfgConst* स्थिर aConstp = ap->cast<DfgConst>()) {
            अगर (DfgConst* स्थिर bConstp = bp->cast<DfgConst>()) {
                स्थिर V3_संख्या& a_अंक = aConstp->अंक();
                स्थिर V3_संख्या& b_अंक = bConstp->अंक();
                // Max match is the shorter constant
                स्थिर uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                क्रम (; matchWidth < maxMatch; ++matchWidth) {
                    अगर (a_अंक.bitIs0(matchWidth) != b_अंक.bitIs0(matchWidth)) अवरोध;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* स्थिर shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                वापस {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        अगर (DfgConcat* स्थिर catp = ap->cast<DfgConcat>()) वापस commonLSBs(catp->rhsp(), bp);
        अगर (DfgConcat* स्थिर catp = bp->cast<DfgConcat>()) वापस commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        अगर (DfgSel* स्थिर selp = ap->cast<DfgSel>()) {
            अगर (selp->lsb() == 0) {
                DfgVertex* स्थिर fromp = selp->fromp();
                स्थिर std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                वापस {common.first, std::min(common.second, selp->width())};
            }
        }
        अगर (DfgSel* स्थिर selp = bp->cast<DfgSel>()) {
            अगर (selp->lsb() == 0) {
                DfgVertex* स्थिर fromp = selp->fromp();
                स्थिर std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                वापस {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        वापस {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        अगर (ap == bp) वापस {ap, ap->width()};

        // If both constants, check MSBs
        अगर (DfgConst* स्थिर aConstp = ap->cast<DfgConst>()) {
            अगर (DfgConst* स्थिर bConstp = bp->cast<DfgConst>()) {
                स्थिर uint32_t aMsb = aConstp->width() - 1;
                स्थिर uint32_t bMsb = bConstp->width() - 1;
                स्थिर V3_संख्या& a_अंक = aConstp->अंक();
                स्थिर V3_संख्या& b_अंक = bConstp->अंक();
                // Max match is the shorter constant
                स्थिर uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                क्रम (; matchWidth < maxMatch; ++matchWidth) {
                    अगर (a_अंक.bitIs0(aMsb - matchWidth) != b_अंक.bitIs0(bMsb - matchWidth)) अवरोध;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* स्थिर shorterp = aMsb < bMsb ? aConstp : bConstp;
                वापस {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        अगर (DfgConcat* स्थिर catp = ap->cast<DfgConcat>()) वापस commonMSBs(catp->lhsp(), bp);
        अगर (DfgConcat* स्थिर catp = bp->cast<DfgConcat>()) वापस commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        अगर (DfgSel* स्थिर selp = ap->cast<DfgSel>()) {
            DfgVertex* स्थिर fromp = selp->fromp();
            अगर (selp->msb() == fromp->width() - 1) {
                स्थिर std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                वापस {common.first, std::min(common.second, selp->width())};
            }
        }
        अगर (DfgSel* स्थिर selp = bp->cast<DfgSel>()) {
            DfgVertex* स्थिर fromp = selp->fromp();
            अगर (selp->msb() == fromp->width() - 1) {
                स्थिर std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                वापस {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        वापस {nullptr, 0};
    }

    // VISIT methods

    व्योम visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    व्योम visit(Dfg_गिनती_Ones* स्थिर vtxp) override {
        अगर (foldUnary(vtxp)) वापस;
    }

    व्योम visit(DfgExtend* स्थिर vtxp) override {
        अगर (foldUnary(vtxp)) वापस;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* स्थिर zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            वापस;
        }
    }

    व्योम visit(DfgExtendS* स्थिर vtxp) override {
        अगर (foldUnary(vtxp)) वापस;
    }

    व्योम visit(DfgLogNot* स्थिर vtxp) override {
        अगर (foldUnary(vtxp)) वापस;
    }

    व्योम visit(DfgNegate* स्थिर vtxp) override {
        अगर (foldUnary(vtxp)) वापस;
    }

    व्योम visit(DfgNot* स्थिर vtxp) override {
        अगर (foldUnary(vtxp)) वापस;

        // Not of Cond
        अगर (DfgCond* स्थिर condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            अगर (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(धकेलो_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* स्थिर newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* स्थिर newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* स्थिर newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    वापस;
                }
            }
        }

        // Not of Not
        अगर (DfgNot* स्थिर notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                वापस;
            }
        }

        अगर (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            अगर (DfgEq* स्थिर eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    वापस;
                }
            }

            // Not of Neq
            अगर (DfgNeq* स्थिर neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    वापस;
                }
            }

            // Not of unsigned comparisons
            अगर (DfgLt* स्थिर p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    वापस;
                }
            }
            अगर (DfgGte* स्थिर p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    वापस;
                }
            }
            अगर (DfgGt* स्थिर p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    वापस;
                }
            }
            अगर (DfgLte* स्थिर p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    वापस;
                }
            }

            // Not of signed comparisons
            अगर (DfgLtS* स्थिर p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    वापस;
                }
            }
            अगर (DfgGteS* स्थिर p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    वापस;
                }
            }
            अगर (DfgGtS* स्थिर p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    वापस;
                }
            }
            अगर (DfgLteS* स्थिर p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgOneHot* स्थिर vtxp) override {
        अगर (foldUnary(vtxp)) वापस;
    }

    व्योम visit(DfgOneHot0* स्थिर vtxp) override {
        अगर (foldUnary(vtxp)) वापस;
    }

    व्योम visit(DfgRedOr* स्थिर vtxp) override {
        अगर (optimizeReduction(vtxp)) वापस;
    }

    व्योम visit(DfgRedAnd* स्थिर vtxp) override {
        अगर (optimizeReduction(vtxp)) वापस;
    }

    व्योम visit(DfgRedXor* स्थिर vtxp) override {
        अगर (optimizeReduction(vtxp)) वापस;
    }

    व्योम visit(DfgSel* स्थिर vtxp) override {
        DfgVertex* स्थिर fromp = vtxp->fromp();

        File_पंक्ति* स्थिर flp = vtxp->fileline();

        स्थिर uint32_t lsb = vtxp->lsb();
        स्थिर uint32_t width = vtxp->width();
        स्थिर uint32_t msb = lsb + width - 1;

        अगर (DfgConst* स्थिर constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* स्थिर resp = makeZero(flp, width);
                resp->अंक().opSel(constp->अंक(), msb, lsb);
                replace(resp);
                वापस;
            }
        }

        // Full width select, replace with the source.
        अगर (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                वापस;
            }
        }

        // Unwind through bit packing in one go
        {
            स्थिर स्वतः res = unwindSel(fromp, lsb, width);
            अगर (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                वापस;
            }
        }

        // Sel from Not
        अगर (DfgNot* स्थिर notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            अगर (!notp->hasMultipleSinks()) {
                APPLYING(धकेलो_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* स्थिर newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    वापस;
                }
            }
        }

        // Sel from Cond
        अगर (DfgCond* स्थिर condp = fromp->cast<DfgCond>()) {
            अगर (!condp->hasMultipleSinks()) {
                APPLYING(धकेलो_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* स्थिर newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* स्थिर newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* स्थिर newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    वापस;
                }
            }
        }

        // Sel from ShiftL
        अगर (DfgShiftL* स्थिर shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            अगर (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(धकेलो_SEL_THROUGH_SHIFTL) {
                    DfgSel* स्थिर newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgMux* स्थिर vtxp) override {
        DfgVertex* स्थिर fromp = vtxp->fromp();
        DfgVertex* स्थिर lsbp = vtxp->lsbp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (DfgConst* स्थिर lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->अंक().toUInt()));
                वापस;
            }
        }

        अगर (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                वापस;
            }
        }

        अगर (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                वापस;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    व्योम visit(DfgAnd* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        अगर (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            अगर (DfgNot* स्थिर lhsNotp = lhsp->cast<DfgNot>()) {
                अगर (DfgNot* स्थिर rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* स्थिर orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        वापस;
                    }
                }
                अगर (DfgNeq* स्थिर rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* स्थिर newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* स्थिर orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        वापस;
                    }
                }
            }
        }

        अगर (DfgConst* स्थिर lConstp = lhsp->cast<DfgConst>()) {
            अगर (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    वापस;
                }
            }

            अगर (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    वापस;
                }
            }

            अगर (DfgConcat* स्थिर rhsConcatp = rhsp->cast<DfgConcat>()) {
                अगर (try_धकेलो_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) वापस;
            }
        }

        अगर (distributive_And_Associative_द्विआधारी<DfgOr, DfgAnd>(vtxp)) वापस;

        अगर (try_धकेलो_Bitwise_Op_Through_Reductions(vtxp)) वापस;

        अगर (try_धकेलो_Bitwise_Op_Throug_Sel(vtxp)) वापस;

        {
            DfgNot* स्थिर lNotp = lhsp->cast<DfgNot>();
            DfgNot* स्थिर rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            अगर ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    वापस;
                }
            }

            अगर (DfgAnd* स्थिर rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* स्थिर rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* स्थिर rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                अगर ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        वापस;
                    }
                }
            }
        }

        अगर (vtxp->dtype() == m_bitDType) {
            अगर (tryReplaceBitwiseWithReduction(vtxp)) वापस;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            अगर (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            अगर (repp && condp && repp->srcp()->आकार() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* स्थिर newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgOr* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        अगर (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            अगर (DfgNot* स्थिर lhsNotp = lhsp->cast<DfgNot>()) {
                अगर (DfgNot* स्थिर rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* स्थिर andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        वापस;
                    }
                }
                अगर (DfgNeq* स्थिर rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* स्थिर newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* स्थिर andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        वापस;
                    }
                }
            }
        }

        अगर (DfgConcat* स्थिर lhsConcatp = lhsp->cast<DfgConcat>()) {
            अगर (DfgConcat* स्थिर rhsConcatp = rhsp->cast<DfgConcat>()) {
                अगर (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    अगर (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            वापस;
                        }
                    }
                    अगर (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            वापस;
                        }
                    }
                }
            }
        }

        अगर (DfgConst* स्थिर lConstp = lhsp->cast<DfgConst>()) {
            अगर (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    वापस;
                }
            }

            अगर (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    वापस;
                }
            }

            अगर (DfgConcat* स्थिर rhsConcatp = rhsp->cast<DfgConcat>()) {
                अगर (try_धकेलो_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) वापस;
            }
        }

        अगर (distributive_And_Associative_द्विआधारी<DfgAnd, DfgOr>(vtxp)) वापस;

        अगर (try_धकेलो_Bitwise_Op_Through_Reductions(vtxp)) वापस;

        अगर (try_धकेलो_Bitwise_Op_Throug_Sel(vtxp)) वापस;

        {
            DfgNot* स्थिर lNotp = lhsp->cast<DfgNot>();
            DfgNot* स्थिर rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            अगर ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    वापस;
                }
            }

            अगर (DfgOr* स्थिर rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* स्थिर rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* स्थिर rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                अगर ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        वापस;
                    }
                }
            }
        }

        अगर (vtxp->dtype() == m_bitDType) {
            अगर (tryReplaceBitwiseWithReduction(vtxp)) वापस;
        }
    }

    व्योम visit(DfgXor* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                वापस;
            }
        }

        अगर (DfgConst* स्थिर lConstp = lhsp->cast<DfgConst>()) {
            अगर (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    वापस;
                }
            }
            अगर (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    वापस;
                }
            }
            अगर (DfgConcat* स्थिर rConcatp = rhsp->cast<DfgConcat>()) {
                अगर (try_धकेलो_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) वापस;
            }
        }

        अगर (try_धकेलो_Bitwise_Op_Through_Reductions(vtxp)) वापस;

        अगर (try_धकेलो_Bitwise_Op_Throug_Sel(vtxp)) वापस;

        अगर (vtxp->dtype() == m_bitDType) {
            अगर (tryReplaceBitwiseWithReduction(vtxp)) वापस;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    व्योम visit(DfgAdd* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                वापस;
            }
        }

        स्थिर std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_गिनती_Ones_Term(lhsp);
        अगर (DfgVertex* स्थिर lVtxp = std::लो<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_गिनती_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            अगर (!std::लो<0>(rTerm)) {
                अगर (DfgAdd* स्थिर rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_गिनती_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            अगर (DfgVertex* स्थिर rVtxp = std::लो<0>(rTerm)) {
                अगर (isSame(lVtxp, rVtxp)) {
                    स्थिर uint32_t lLsb = std::लो<1>(lTerm);
                    स्थिर uint32_t rLsb = std::लो<1>(rTerm);
                    स्थिर uint32_t lWidth = std::लो<2>(lTerm);
                    स्थिर uint32_t rWidth = std::लो<2>(rTerm);
                    bool adjoined = true;
                    uint32_t lsb = 0;
                    अगर (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } अन्यथा अगर (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } अन्यथा {
                        adjoined = false;
                    }
                    अगर (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_गिनती_ONES) {
                            DfgSel* स्थिर selp
                                = make<DfgSel>(vtxp->fileline(),
                                               Dfg_तथ्य_Type::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_गिनती_Ones>(flp, Dfg_तथ्य_Type::packed(32), selp);
                            अगर (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } अन्यथा अगर (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            अगर (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            वापस;
                        }
                    }
                }
            }
        }
    }

    व्योम visit(DfgArraySel* स्थिर vtxp) override {
        DfgConst* स्थिर idxp = vtxp->bitp()->cast<DfgConst>();
        अगर (!idxp) वापस;
        DfgVarArray* स्थिर varp = vtxp->fromp()->cast<DfgVarArray>();
        अगर (!varp) वापस;
        AstVar* स्थिर astVarp = varp->vscp()->varp();
        अगर (astVarp->isForced()) वापस;
        अगर (astVarp->isSigUserRWPublic()) वापस;
        DfgVertex* स्थिर srcp = varp->srcp();
        अगर (!srcp) {
            अगर (vtxp->isPacked()) {
                अगर (AstConsArray* स्थिर iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    अगर (AstConst* स्थिर valp = VN_CAST(iap->valuep(idxp->to_आकार_T()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(नया DfgConst{m_dfg, valp->fileline(), valp->अंक()});
                            वापस;
                        }
                    }
                }
            }
            वापस;
        }

        अगर (DfgSpliceArray* स्थिर splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* स्थिर driverp = splicep->driverAt(idxp->to_आकार_T());
            अगर (!driverp) वापस;
            DfgUnitArray* स्थिर uap = driverp->cast<DfgUnitArray>();
            अगर (!uap) वापस;
            अगर (uap->srcp()->is<DfgVertexSplice>()) वापस;
            // If driven by a variable that had a Driver in DFG, it is partial
            अगर (DfgVertexVar* स्थिर dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                अगर (dvarp->srcp()) वापस;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                वापस;
            }
        }

        अगर (DfgUnitArray* स्थिर uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->to_आकार_T() == 0, vtxp, "Array index out of range");
            अगर (uap->srcp()->is<DfgSplicePacked>()) वापस;
            // If driven by a variable that had a Driver in DFG, it is partial
            अगर (DfgVertexVar* स्थिर dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                अगर (dvarp->srcp()) वापस;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                वापस;
            }
        }
    }

    व्योम visit(DfgConcat* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();

        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isZero(lhsp)) {
            DfgConst* स्थिर lConstp = lhsp->as<DfgConst>();
            अगर (DfgSel* स्थिर rSelp = rhsp->cast<DfgSel>()) {
                अगर (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_शीर्ष_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        वापस;
                    }
                }
            }
        }

        अगर (isZero(rhsp)) {
            DfgConst* स्थिर rConstp = rhsp->as<DfgConst>();
            अगर (DfgSel* स्थिर lSelp = lhsp->cast<DfgSel>()) {
                अगर (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        वापस;
                    }
                }
            }
        }

        अगर (DfgNot* स्थिर lNot = lhsp->cast<DfgNot>()) {
            अगर (DfgNot* स्थिर rNot = rhsp->cast<DfgNot>()) {
                अगर (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(धकेलो_CONCAT_THROUGH_NOTS) {
                        DfgConcat* स्थिर newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        वापस;
                    }
                }
            }
        }

        {
            bool swap = false;
            अगर (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(धकेलो_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    वापस;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            अगर (DfgConcat* स्थिर rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* स्थिर rlp = rConcatp->lhsp();
                अगर (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(धकेलो_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* स्थिर newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        वापस;
                    }
                }
            }
        }

        अगर (DfgSel* स्थिर lSelp = lhsp->cast<DfgSel>()) {
            अगर (DfgSel* स्थिर rSelp = rhsp->cast<DfgSel>()) {
                अगर (isSame(lSelp->fromp(), rSelp->fromp())) {
                    अगर (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            वापस;
                        }
                    }
                }
            }

            अगर (DfgConcat* स्थिर rConcatp = rhsp->cast<DfgConcat>()) {
                अगर (DfgSel* स्थिर rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    अगर (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        अगर (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                स्थिर uint32_t width = lSelp->width() + rlSelp->width();
                                स्थिर Dfg_तथ्य_Type& dtype = Dfg_तथ्य_Type::packed(width);
                                DfgSel* स्थिर selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                वापस;
                            }
                        }
                    }
                }
            }
        }

        अगर (DfgSel* स्थिर rSelp = rhsp->cast<DfgSel>()) {
            अगर (DfgConcat* स्थिर lConcatp = lhsp->cast<DfgConcat>()) {
                अगर (DfgSel* स्थिर lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    अगर (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        अगर (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                स्थिर uint32_t width = lrSelp->width() + rSelp->width();
                                स्थिर Dfg_तथ्य_Type& dtype = Dfg_तथ्य_Type::packed(width);
                                DfgSel* स्थिर selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                वापस;
                            }
                        }
                    }
                }
            }
        }

        अगर (DfgConst* स्थिर lConstp = lhsp->cast<DfgConst>()) {
            अगर (DfgCond* स्थिर rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* स्थिर rtVtxp = rCondp->thenp();
                DfgVertex* स्थिर reVtxp = rCondp->elsep();
                DfgConst* स्थिर rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* स्थिर reConstp = reVtxp->cast<DfgConst>();
                अगर (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(धकेलो_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* स्थिर thenp = [&]() -> DfgVertex* {
                            File_पंक्ति* स्थिर rtFlp = rtVtxp->fileline();
                            अगर (rtConstp) {
                                DfgConst* स्थिर constp = makeZero(rtFlp, vtxp->width());
                                constp->अंक().opConcat(lConstp->अंक(), rtConstp->अंक());
                                वापस constp;
                            }
                            वापस make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* स्थिर elsep = [&]() -> DfgVertex* {
                            File_पंक्ति* स्थिर reFlp = reVtxp->fileline();
                            अगर (reConstp) {
                                DfgConst* स्थिर constp = makeZero(reFlp, vtxp->width());
                                constp->अंक().opConcat(lConstp->अंक(), reConstp->अंक());
                                वापस constp;
                            }
                            वापस make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        वापस;
                    }
                }
            }
        }

        अगर (DfgConst* स्थिर rConstp = rhsp->cast<DfgConst>()) {
            अगर (DfgCond* स्थिर lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* स्थिर ltVtxp = lCondp->thenp();
                DfgVertex* स्थिर leVtxp = lCondp->elsep();
                DfgConst* स्थिर ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* स्थिर leConstp = leVtxp->cast<DfgConst>();
                अगर (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(धकेलो_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* स्थिर thenp = [&]() -> DfgVertex* {
                            File_पंक्ति* स्थिर ltFlp = ltVtxp->fileline();
                            अगर (ltConstp) {
                                DfgConst* स्थिर constp = makeZero(ltFlp, vtxp->width());
                                constp->अंक().opConcat(ltConstp->अंक(), rConstp->अंक());
                                वापस constp;
                            }
                            वापस make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* स्थिर elsep = [&]() -> DfgVertex* {
                            File_पंक्ति* स्थिर leFlp = leVtxp->fileline();
                            अगर (leConstp) {
                                DfgConst* स्थिर constp = makeZero(leFlp, vtxp->width());
                                constp->अंक().opConcat(leConstp->अंक(), rConstp->अंक());
                                वापस constp;
                            }
                            वापस make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        वापस;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            स्थिर uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            स्थिर uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            स्थिर uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            bool hasCrossSink = false;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                अगर (स्थिर DfgSel* स्थिर selp = sink.cast<DfgSel>()) {
                    स्थिर uint32_t selLsb = selp->lsb();
                    स्थिर uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    वापस false;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                अगर (स्थिर DfgVarPacked* स्थिर varp = sink.cast<DfgVarPacked>()) {
                    अगर (!varp->hasSinks() && !varp->isObserved()) वापस false;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                वापस true;
            });
            अगर (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    अगर (lsb != 0)
                        nRhsp = make<DfgSel>(flp, Dfg_तथ्य_Type::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    अगर (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, Dfg_तथ्य_Type::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* स्थिर catp
                        = make<DfgConcat>(flp, Dfg_तथ्य_Type::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* स्थिर sp = नया DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* स्थिर varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(true);
                    varp->srcp(sp);
                    replace(varp);
                    वापस;
                }
            }
        }

        // Convert various forms to REPLICATE
        अगर (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                वापस;
            }
        }
        अगर (DfgRep* स्थिर lRepp = lhsp->cast<DfgRep>()) {
            अगर (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    वापस;
                }
            }
        }
        अगर (DfgRep* स्थिर rRepp = rhsp->cast<DfgRep>()) {
            अगर (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    वापस;
                }
            }
        }
        अगर (DfgRep* स्थिर lRepp = lhsp->cast<DfgRep>()) {
            अगर (DfgRep* स्थिर rRepp = rhsp->cast<DfgRep>()) {
                अगर (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        वापस;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            अगर (DfgConcat* स्थिर rConcatp = rhsp->cast<DfgConcat>()) {
                अगर (DfgRep* स्थिर rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    अगर (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            स्थिर uint32_t width = lRepp->width() + rlRepp->width();
                            स्थिर Dfg_तथ्य_Type& dtype = Dfg_तथ्य_Type::packed(width);
                            DfgRep* स्थिर repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            वापस;
                        }
                    }
                }
            }
        }
    }

    व्योम visit(DfgDiv* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        अगर (DfgConst* स्थिर rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            अगर (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgDivS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        अगर (DfgConst* स्थिर rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            अगर (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgGt* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                वापस;
            }
        }
    }

    व्योम visit(DfgGtS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                वापस;
            }
        }
    }

    व्योम visit(DfgGte* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                वापस;
            }
        }
    }

    व्योम visit(DfgGteS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                वापस;
            }
        }
    }

    व्योम visit(DfgLogAnd* स्थिर vtxp) override {
        अगर (द्विग(vtxp) || vtxp->rhsp()->unsafe()) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();

        अगर (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                वापस;
            }
        }
    }

    व्योम visit(DfgLogEq* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
    }

    व्योम visit(DfgLogIf* स्थिर vtxp) override {
        अगर (द्विग(vtxp) || vtxp->rhsp()->unsafe()) वापस;
    }

    व्योम visit(DfgLogOr* स्थिर vtxp) override {
        अगर (द्विग(vtxp) || vtxp->rhsp()->unsafe()) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();

        अगर (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                वापस;
            }
        }
    }

    व्योम visit(DfgLt* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                वापस;
            }
        }
    }

    व्योम visit(DfgLtS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                वापस;
            }
        }
    }

    व्योम visit(DfgLte* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                वापस;
            }
        }
    }

    व्योम visit(DfgLteS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                वापस;
            }
        }
    }

    व्योम visit(DfgModDiv* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
    }

    व्योम visit(DfgModDivS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
    }

    व्योम visit(DfgMul* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (DfgConst* स्थिर lConstp = lhsp->cast<DfgConst>()) {
            अगर (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    वापस;
                }
            }
            अगर (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgMulS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (DfgConst* स्थिर lConstp = lhsp->cast<DfgConst>()) {
            अगर (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    वापस;
                }
            }
            अगर (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgEq* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                वापस;
            }
        }

        अगर (DfgConst* स्थिर lhsConstp = lhsp->cast<DfgConst>()) {
            अगर (DfgConcat* स्थिर rhsConcatp = rhsp->cast<DfgConcat>()) {
                अगर (try_धकेलो_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) वापस;
            }

            अगर (rhsp->dtype() == m_bitDType) {
                अगर (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        वापस;
                    }
                }

                अगर (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        वापस;
                    }
                }
            }
        }
    }

    व्योम visit(DfgNeq* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                वापस;
            }
        }

        अगर (DfgConst* स्थिर lhsConstp = lhsp->cast<DfgConst>()) {
            अगर (DfgConcat* स्थिर rhsConcatp = rhsp->cast<DfgConcat>()) {
                अगर (try_धकेलो_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) वापस;
            }

            अगर (rhsp->dtype() == m_bitDType) {
                अगर (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        वापस;
                    }
                }

                अगर (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        वापस;
                    }
                }
            }
        }
    }

    व्योम visit(DfgPow* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
    }

    व्योम visit(DfgPowSS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
    }

    व्योम visit(DfgPowSU* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
    }

    व्योम visit(DfgPowUS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
    }

    व्योम visit(DfgRep* स्थिर vtxp) override {
        DfgVertex* स्थिर srcp = vtxp->srcp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (DfgConst* स्थिर sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* स्थिर resultp = makeZero(flp, vtxp->width());
                resultp->अंक().opRepl(sConstp->अंक(), vtxp->गिनती());
                replace(resultp);
                वापस;
            }
        }

        अगर (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                वापस;
            }
        }

        अगर (DfgRep* स्थिर sRepp = srcp->cast<DfgRep>()) {
            अगर (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgShiftL* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
        अगर (optimizeShiftRHS(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();

        अगर (DfgShiftL* स्थिर lShiftLp = lhsp->cast<DfgShiftL>()) {
            अगर (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_पंक्ति* स्थिर flp = vtxp->fileline();
                    स्थिर Dfg_तथ्य_Type& योग_DType = Dfg_तथ्य_Type::packed(rhsp->width() + 1);
                    DfgVertex* स्थिर bp = make<DfgExtend>(flp, योग_DType, lShiftLp->rhsp());
                    DfgVertex* स्थिर cp = make<DfgExtend>(flp, योग_DType, rhsp);
                    DfgAdd* स्थिर addp = make<DfgAdd>(flp, योग_DType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    वापस;
                }
            }
        }

        अगर (DfgConst* स्थिर rConstp = rhsp->cast<DfgConst>()) {
            स्थिर uint32_t shiftAmount = rConstp->toU32();

            अगर (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    वापस;
                }
            }

            अगर (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    वापस;
                }
            }

            अगर (DfgConcat* स्थिर lConcatp = lhsp->cast<DfgConcat>()) {
                अगर (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* स्थिर lRhsp = lConcatp->rhsp();
                        DfgVertex* स्थिर lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        स्थिर uint32_t rW = shiftAmount;
                        स्थिर uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        स्थिर uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_पंक्ति* स्थिर flp = lConcatp->fileline();
                        DfgVertex* स्थिर rp = makeZero(flp, shiftAmount);
                        DfgVertex* स्थिर mp
                            = make<DfgSel>(flp, Dfg_तथ्य_Type::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, Dfg_तथ्य_Type::packed(mW + rW), mp, rp);
                        अगर (!lW) {
                            replace(np);
                            वापस;
                        }
                        DfgVertex* स्थिर lp
                            = make<DfgSel>(flp, Dfg_तथ्य_Type::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        वापस;
                    }
                }
            }

            अगर (DfgSel* स्थिर lSelp = lhsp->cast<DfgSel>()) {
                अगर (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        स्थिर uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* स्थिर nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_तथ्य_Type::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        वापस;
                    }
                }
            }

            अगर (DfgCond* स्थिर lCondp = lhsp->cast<DfgCond>()) {
                अगर (!lCondp->hasMultipleSinks()) {
                    APPLYING(धकेलो_SHIFTL_THROUGH_COND) {
                        DfgShiftL* स्थिर tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* स्थिर ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        वापस;
                    }
                }
            }
        }
    }

    व्योम visit(DfgShiftR* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
        अगर (optimizeShiftRHS(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();

        अगर (DfgShiftR* स्थिर lShiftRp = lhsp->cast<DfgShiftR>()) {
            अगर (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_पंक्ति* स्थिर flp = vtxp->fileline();
                    स्थिर Dfg_तथ्य_Type& योग_DType = Dfg_तथ्य_Type::packed(rhsp->width() + 1);
                    DfgVertex* स्थिर bp = make<DfgExtend>(flp, योग_DType, lShiftRp->rhsp());
                    DfgVertex* स्थिर cp = make<DfgExtend>(flp, योग_DType, rhsp);
                    DfgAdd* स्थिर addp = make<DfgAdd>(flp, योग_DType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    वापस;
                }
            }
        }

        अगर (DfgConst* स्थिर rConstp = rhsp->cast<DfgConst>()) {
            स्थिर uint32_t shiftAmount = rConstp->toU32();

            अगर (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    वापस;
                }
            }

            अगर (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    वापस;
                }
            }

            अगर (DfgConcat* स्थिर lConcatp = lhsp->cast<DfgConcat>()) {
                अगर (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* स्थिर lRhsp = lConcatp->rhsp();
                        DfgVertex* स्थिर lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        स्थिर uint32_t lW = shiftAmount;
                        स्थिर uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        स्थिर uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_पंक्ति* स्थिर flp = lConcatp->fileline();
                        DfgVertex* स्थिर lp = makeZero(flp, shiftAmount);
                        DfgVertex* स्थिर mp = make<DfgSel>(flp, Dfg_तथ्य_Type::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, Dfg_तथ्य_Type::packed(lW + mW), lp, mp);
                        अगर (!rW) {
                            replace(np);
                            वापस;
                        }
                        DfgVertex* स्थिर rp = make<DfgSel>(flp, Dfg_तथ्य_Type::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        वापस;
                    }
                }
            }

            अगर (DfgSel* स्थिर lSelp = lhsp->cast<DfgSel>()) {
                अगर (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        स्थिर uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* स्थिर nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_तथ्य_Type::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        वापस;
                    }
                }
            }

            अगर (DfgCond* स्थिर lCondp = lhsp->cast<DfgCond>()) {
                अगर (!lCondp->hasMultipleSinks()) {
                    APPLYING(धकेलो_SHIFTR_THROUGH_COND) {
                        DfgShiftR* स्थिर tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* स्थिर ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        वापस;
                    }
                }
            }
        }
    }

    व्योम visit(DfgShiftRS* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;
        अगर (optimizeShiftRHS(vtxp)) वापस;
    }

    व्योम visit(DfgSub* स्थिर vtxp) override {
        अगर (द्विग(vtxp)) वापस;

        DfgVertex* स्थिर lhsp = vtxp->lhsp();
        DfgVertex* स्थिर rhsp = vtxp->rhsp();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                वापस;
            }
        }

        अगर (DfgConst* स्थिर rConstp = rhsp->cast<DfgConst>()) {
            अगर (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    वापस;
                }
            }
            अगर (vtxp->dtype() == m_bitDType && rConstp->has_मूल्य(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgMatchMasked* स्थिर vtxp) override {
        अगर (DfgConst* स्थिर constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* स्थिर matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->अंक(), matchVarp)));
                वापस;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    व्योम visit(DfgCond* स्थिर vtxp) override {
        DfgVertex* स्थिर condp = vtxp->condp();
        DfgVertex* स्थिर thenp = vtxp->thenp();
        DfgVertex* स्थिर elsep = vtxp->elsep();
        File_पंक्ति* स्थिर flp = vtxp->fileline();

        अगर (condp->dtype() != m_bitDType) वापस;

        अगर (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                वापस;
            }
        }

        अगर (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                वापस;
            }
        }

        अगर (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                वापस;
            }
        }

        अगर (DfgNot* स्थिर condNotp = condp->cast<DfgNot>()) {
            अगर (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    वापस;
                }
            }
        }

        अगर (DfgNeq* स्थिर condNeqp = condp->cast<DfgNeq>()) {
            अगर (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* स्थिर newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    वापस;
                }
            }
        }

        अगर (DfgNot* स्थिर thenNotp = thenp->cast<DfgNot>()) {
            अगर (DfgNot* स्थिर elseNotp = elsep->cast<DfgNot>()) {
                अगर (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* स्थिर newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        वापस;
                    }
                }
            }
        }

        अगर (DfgOr* स्थिर condOrp = condp->cast<DfgOr>()) {
            अगर (DfgCond* स्थिर thenCondp = thenp->cast<DfgCond>()) {
                अगर (!thenCondp->hasMultipleSinks()) {
                    अगर (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* स्थिर ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            वापस;
                        }
                    }
                    अगर (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* स्थिर ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            वापस;
                        }
                    }
                }
            }
        }

        अगर (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            अगर (DfgAdd* स्थिर thenAddp = thenp->cast<DfgAdd>()) {
                अगर (DfgConst* स्थिर constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    अगर (constp->has_मूल्य(1)) {
                        अगर (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* स्थिर extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_पंक्ति* स्थिर thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                वापस;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            अगर (DfgSub* स्थिर thenSubp = thenp->cast<DfgSub>()) {
                अगर (DfgConst* स्थिर constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    अगर (constp->has_मूल्य(1)) {
                        अगर (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* स्थिर extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_पंक्ति* स्थिर thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                वापस;
                            }
                        }
                    }
                }
            }
        }

        अगर (vtxp->dtype() == m_bitDType) {
            अगर (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    वापस;
                }
            }
            अगर (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    वापस;
                }
            }
        }

        अगर (vtxp->width() <= VL_QUADSIZE) {
            अगर (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* स्थिर maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    वापस;
                }
            }
            अगर (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* स्थिर maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    वापस;
                }
            }
            अगर (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* स्थिर maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    वापस;
                }
            }
            अगर (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* स्थिर maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    वापस;
                }
            }

            अगर (DfgOr* स्थिर tOrp = thenp->cast<DfgOr>()) {
                अगर (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* स्थिर maskp = replicate(vtxp, condp);
                        DfgAnd* स्थिर andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        वापस;
                    }
                }
                अगर (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* स्थिर maskp = replicate(vtxp, condp);
                        DfgAnd* स्थिर andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        वापस;
                    }
                }
            }
        }

        अगर (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            स्थिर std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            अगर (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    स्थिर uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    अगर (rWidth != rhsp->width()) {
                        स्थिर Dfg_तथ्य_Type& rDtype = Dfg_तथ्य_Type::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    अगर (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        वापस;
                    }
                    // Create new LHS
                    स्थिर uint32_t lWidth = vtxp->width() - rWidth;
                    स्थिर Dfg_तथ्य_Type& lDtype = Dfg_तथ्य_Type::packed(lWidth);
                    DfgVertex* स्थिर lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* स्थिर lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* स्थिर lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    वापस;
                }
            }

            स्थिर std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            अगर (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    स्थिर uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    अगर (lWidth != lhsp->width()) {
                        स्थिर Dfg_तथ्य_Type& lDtype = Dfg_तथ्य_Type::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    अगर (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        वापस;
                    }
                    // Create new RHS
                    स्थिर uint32_t rWidth = vtxp->width() - lWidth;
                    स्थिर Dfg_तथ्य_Type& rDtype = Dfg_तथ्य_Type::packed(rWidth);
                    DfgVertex* स्थिर rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* स्थिर rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* स्थिर rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    वापस;
                }
            }
        }

        अगर (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                अगर (स्थिर uint32_t extend = vtxp->width() - 1) {
                    DfgConst* स्थिर zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                वापस;
            }
        }

        अगर (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                अगर (स्थिर uint32_t extend = vtxp->width() - 1) {
                    DfgConst* स्थिर zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                वापस;
            }
        }

        अगर (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                वापस;
            }
        }

        अगर (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                वापस;
            }
        }

        अगर (DfgCond* स्थिर tCondp = thenp->cast<DfgCond>()) {
            अगर (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    वापस;
                }
            }
        }

        अगर (DfgCond* स्थिर eCondp = elsep->cast<DfgCond>()) {
            अगर (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    वापस;
                }
            }
        }
    }

    व्योम visit(DfgVertexVar* स्थिर vtxp) override {
        अगर (vtxp->hasSinks()) वापस;
        अगर (vtxp->isObserved()) वापस;
        अगर (vtxp->defaultp()) वापस;

        // If undriven, or driven from another var, it is completely redundant.
        अगर (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                वापस;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        bool foundOne = false;
        स्थिर bool keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> bool {
            // Ignore non-observable variable sinks. These can be eliminated.
            अगर (स्थिर DfgVertexVar* स्थिर varp = sink.cast<DfgVertexVar>()) {
                अगर (!varp->hasSinks() && !varp->isObserved()) वापस false;
            }
            // Keep if found more than one sink
            अगर (foundOne) वापस true;
            // Mark first sink found
            foundOne = true;
            वापस false;
        });
        अगर (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                वापस;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        क्रम (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.आकार() * 2);
        m_iterList.reserve(m_dfg.आकार() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.धकेलो_back(nullptr);
        m_iterList.धकेलो_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        क्रम (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        क्रम (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        जबतक (true) {
            // Process the work list - keep the placeholder at index 0
            जबतक (m_workList.आकार() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.निकालो_back();
                अगर (!m_vtxp) जारी;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_work_List_सूचकांक = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                अगर (DfgVertexVar* स्थिर varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    जारी;
                }

                // Ast references are not considered
                अगर (m_vtxp->is<DfgVertexAst>()) जारी;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                अगर (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    अगर (DfgVertex* स्थिर sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            जारी;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            अगर (m_iterList.आकार() == 1) अवरोध;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                माप_प्रकार begin = 0;
                माप_प्रकार end = m_iterList.आकार();
                क्रम (माप_प्रकार hops = 1; hops <= 4; ++hops) {
                    क्रम (माप_प्रकार i = begin; i < end; ++i) {
                        DfgVertex* स्थिर vtxp = m_iterList[i];
                        अगर (!vtxp) जारी;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            वापस false;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            वापस false;
                        });
                    }
                    begin = end;
                    end = m_iterList.आकार();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            क्रम (DfgVertex* स्थिर vtxp : m_iterList) {
                अगर (!vtxp) जारी;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#अघोषित APPLYING

खुला:
    अटल व्योम apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

व्योम V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    अगर (!v3Global.opt.fDfgPeephole()) वापस;
    V3DfgPeephole::apply(dfg, ctx);
}
