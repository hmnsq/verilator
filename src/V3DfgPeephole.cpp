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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3DfgCache.h"
#অন্তর্ভাৱ "V3DfgDataType.h"
#অন্তর্ভাৱ "V3DfgPasses.h"
#অন্তর্ভাৱ "V3DfgPeepholePatterns.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <cctype>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
নামাকাশ {

template <typename T_Reduction>
সংরচনা ReductionToBitwiseImpl {};
template <> সংরচনা ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> সংরচনা ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> সংরচনা ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
সংরচনা BitwiseToReductionImpl {};
template <> সংরচনা BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> সংরচনা BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> সংরচনা BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
সংরচনা IsAssociative : সার্ৱজনিক std::false_type {};
template <> সংরচনা IsAssociative<DfgAnd> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsAssociative<DfgOr>  : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsAssociative<DfgXor> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsAssociative<DfgAdd> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsAssociative<DfgConcat> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsAssociative<DfgMul> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsAssociative<DfgMulS> : সার্ৱজনিক std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
সংরচনা IsCommutative : সার্ৱজনিক std::false_type {};
template <> সংরচনা IsCommutative<DfgAnd> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsCommutative<DfgOr>  : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsCommutative<DfgXor> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsCommutative<DfgAdd> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsCommutative<DfgEq> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsCommutative<DfgMul> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsCommutative<DfgMulS> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsCommutative<DfgNeq> : সার্ৱজনিক std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
সংরচনা IsIdempotent : সার্ৱজনিক std::false_type {};
template <> সংরচনা IsIdempotent<DfgAnd> : সার্ৱজনিক std::true_type {};
template <> সংরচনা IsIdempotent<DfgOr>  : সার্ৱজনিক std::true_type {};

// Binary result dtype
template<typename Vertex>
ধ্রুৱ DfgDataType& resultDType(ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp);
template <> ধ্রুৱ DfgDataType& resultDType<DfgAdd>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgAnd>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgConcat>     (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgDiv>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgDivS>       (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgEq>         (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgGt>         (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgGtS>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgGte>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgGteS>       (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgLogAnd>     (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgLogEq>      (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgLogIf>      (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgLogOr>      (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgLt>         (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgLtS>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgLte>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgLteS>       (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgModDiv>     (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgModDivS>    (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgMul>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgMulS>       (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgNeq>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই DfgDataType::packed(1); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgOr>         (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgPow>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgPowSS>      (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgPowSU>      (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgPowUS>      (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgShiftL>     (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgShiftR>     (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgShiftRS>    (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgSub>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }
template <> ধ্রুৱ DfgDataType& resultDType<DfgXor>        (ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) { ঘূৰাই lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> শূন্য foldOp(V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src);
template <> শূন্য foldOp<Dfg_গুনতি_Ones>  (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.op_গুনতি_Ones(src); }
template <> শূন্য foldOp<DfgExtend>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opAssign(src); }
template <> শূন্য foldOp<DfgExtendS>    (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opExtendS(src, src.width()); }
template <> শূন্য foldOp<DfgLogNot>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opLogNot(src); }
template <> শূন্য foldOp<DfgNegate>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opNegate(src); }
template <> শূন্য foldOp<DfgNot>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opNot(src); }
template <> শূন্য foldOp<DfgOneHot>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opOneHot(src); }
template <> শূন্য foldOp<DfgOneHot0>    (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opOneHot0(src); }
template <> শূন্য foldOp<DfgRedAnd>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opRedAnd(src); }
template <> শূন্য foldOp<DfgRedOr>      (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opRedOr(src); }
template <> শূন্য foldOp<DfgRedXor>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> শূন্য foldOp(V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs);
template <> শূন্য foldOp<DfgAdd>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opAdd(lhs, rhs); }
template <> শূন্য foldOp<DfgAnd>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opAnd(lhs, rhs); }
template <> শূন্য foldOp<DfgConcat>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opConcat(lhs, rhs); }
template <> শূন্য foldOp<DfgDiv>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opDiv(lhs, rhs); }
template <> শূন্য foldOp<DfgDivS>       (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opDivS(lhs, rhs); }
template <> শূন্য foldOp<DfgEq>         (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opEq(lhs, rhs); }
template <> শূন্য foldOp<DfgGt>         (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opGt(lhs, rhs); }
template <> শূন্য foldOp<DfgGtS>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opGtS(lhs, rhs); }
template <> শূন্য foldOp<DfgGte>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opGte(lhs, rhs); }
template <> শূন্য foldOp<DfgGteS>       (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opGteS(lhs, rhs); }
template <> শূন্য foldOp<DfgLogAnd>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opLogAnd(lhs, rhs); }
template <> শূন্য foldOp<DfgLogEq>      (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opLogEq(lhs, rhs); }
template <> শূন্য foldOp<DfgLogIf>      (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opLogIf(lhs, rhs); }
template <> শূন্য foldOp<DfgLogOr>      (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opLogOr(lhs, rhs); }
template <> শূন্য foldOp<DfgLt>         (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opLt(lhs, rhs); }
template <> শূন্য foldOp<DfgLtS>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opLtS(lhs, rhs); }
template <> শূন্য foldOp<DfgLte>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opLte(lhs, rhs); }
template <> শূন্য foldOp<DfgLteS>       (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opLteS(lhs, rhs); }
template <> শূন্য foldOp<DfgModDiv>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opModDiv(lhs, rhs); }
template <> শূন্য foldOp<DfgModDivS>    (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opModDivS(lhs, rhs); }
template <> শূন্য foldOp<DfgMul>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opMul(lhs, rhs); }
template <> শূন্য foldOp<DfgMulS>       (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opMulS(lhs, rhs); }
template <> শূন্য foldOp<DfgNeq>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opNeq(lhs, rhs); }
template <> শূন্য foldOp<DfgOr>         (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opOr(lhs, rhs); }
template <> শূন্য foldOp<DfgPow>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opPow(lhs, rhs); }
template <> শূন্য foldOp<DfgPowSS>      (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opPowSS(lhs, rhs); }
template <> শূন্য foldOp<DfgPowSU>      (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opPowSU(lhs, rhs); }
template <> শূন্য foldOp<DfgPowUS>      (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opPowUS(lhs, rhs); }
template <> শূন্য foldOp<DfgShiftL>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opShiftL(lhs, rhs); }
template <> শূন্য foldOp<DfgShiftR>     (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opShiftR(lhs, rhs); }
template <> শূন্য foldOp<DfgShiftRS>    (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> শূন্য foldOp<DfgSub>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opSub(lhs, rhs); }
template <> শূন্য foldOp<DfgXor>        (V3_সংখ্যা& out, ধ্রুৱ V3_সংখ্যা& lhs, ধ্রুৱ V3_সংখ্যা& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

শ্ৰেণী V3DfgPeephole final : সার্ৱজনিক DfgVisitor {
    // TYPES
    সংরচনা VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        তার্কিক m_isCachedVertex = মিছা;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ধ্রুৱ DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ধ্রুৱ m_tmpScopep = v3Global.rootp()->শীৰ্ষ_Scopep()->scopep();

    // STATIC STATE
    স্থির V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) যদি (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    তার্কিক checkApplying(VDfgPeepholePattern id) {
        যদি (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ঘূৰাই মিছা;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_গুনতি[id];
        ঘূৰাই সঁচা;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* ধ্রুৱ equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        ঘূৰাই equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    শূন্য invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = মিছা;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_সাৰি* flp, ধ্রুৱ DfgDataType& dtype, Operands... operands) {
        Vertex* ধ্রুৱ vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = সঁচা;
        ঘূৰাই vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(ধ্রুৱ DfgDataType& dtype, Operands... operands) {
        Vertex* ধ্রুৱ vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ঘূৰাই vtxp;
    }

    শূন্য incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    শূন্য addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        যদি (vInfo.m_workListIndex) ঘূৰাই;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.ঠেলো_back(vtxp);
    }

    শূন্য removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    শূন্য addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        যদি (vInfo.m_iterListIndex) ঘূৰাই;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.ঠেলো_back(vtxp);
    }

    শূন্য removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    শূন্য deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = vtxp->cast<DfgVertexVar>();
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
            যদি (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                ঘূৰাই মিছা;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            যদি (vInfo.m_generation != m_currentGeneration) srcps.ঠেলো_back(&src);
            vInfo.m_generation = m_currentGeneration;
            ঘূৰাই মিছা;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        যদি (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ঠেলো_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } অন্যথা {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ধ্রুৱ auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  ঘূৰাই srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        বাবে (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        বাবে (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    শূন্য replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ধ্রুৱ auto debugCallback = [&]() -> শূন্য {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            ধ্রুৱ auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ধ্রুৱ DfgVertex& v) {  //
                ঘূৰাই cone->গুনতি(&v);
            });
        };
        যদি (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ঘূৰাই;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            ঘূৰাই মিছা;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            ঘূৰাই মিছা;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            ঘূৰাই মিছা;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            ঘূৰাই মিছা;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_সাৰি* flp, uint32_t val) { ঘূৰাই নূতন DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_সাৰি* flp, uint32_t width) {
        ঘূৰাই নূতন DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_সাৰি* flp, ধ্রুৱ DfgDataType& dtype) {
        ঘূৰাই makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_সাৰি* flp, uint32_t width) {
        DfgConst* ধ্রুৱ resp = makeZero(flp, width);
        resp->অঙ্ক().setAllBits1();
        ঘূৰাই resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_সাৰি* flp, ধ্রুৱ DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* ধ্রুৱ vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        যদি (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        যদি (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        ঘূৰাই vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(ধ্রুৱ DfgVertex* examplep, Operands... operands) {
        ঘূৰাই make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        যদি (vtxp->dtype() == m_bitDType) ঘূৰাই bitp;
        ঘূৰাই make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    স্থির তার্কিক isSame(ধ্রুৱ DfgVertex* ap, ধ্রুৱ DfgVertex* bp) {
        যদি (ap == bp) ঘূৰাই সঁচা;
        যদি (ap->dtype() != bp->dtype()) ঘূৰাই মিছা;
        ধ্রুৱ DfgConst* ধ্রুৱ aConstp = ap->cast<DfgConst>();
        যদি (!aConstp) ঘূৰাই মিছা;
        ধ্রুৱ DfgConst* ধ্রুৱ bConstp = bp->cast<DfgConst>();
        যদি (!bConstp) ঘূৰাই মিছা;
        ঘূৰাই aConstp->অঙ্ক().isCaseEq(bConstp->অঙ্ক());
    }

    স্থির তার্কিক isZero(ধ্রুৱ DfgVertex* vtxp) {
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ constp = vtxp->cast<DfgConst>()) ঘূৰাই constp->isZero();
        ঘূৰাই মিছা;
    }

    স্থির তার্কিক isOnes(ধ্রুৱ DfgVertex* vtxp) {
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ constp = vtxp->cast<DfgConst>()) ঘূৰাই constp->isOnes();
        ঘূৰাই মিছা;
    }

    স্থির তার্কিক isEqOne(ধ্রুৱ DfgVertex* vtxp) {
        যদি (ধ্রুৱ DfgConst* ধ্রুৱ constp = vtxp->cast<DfgConst>()) ঘূৰাই constp->অঙ্ক().isEqOne();
        ঘূৰাই মিছা;
    }

    স্থির তার্কিক areAdjacent(uint32_t& lsb, ধ্রুৱ DfgSel* lSelp, ধ্রুৱ DfgSel* rSelp) {
        যদি (!isSame(lSelp->srcp(), rSelp->srcp())) ঘূৰাই মিছা;
        যদি (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ঘূৰাই সঁচা;
        }
        যদি (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            ঘূৰাই সঁচা;
        }
        ঘূৰাই মিছা;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক foldUnary(Vertex* ধ্রুৱ vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::মান, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::মান, "Must invoke on final class");
        যদি (DfgConst* ধ্রুৱ srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ধ্রুৱ resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->অঙ্ক(), srcp->অঙ্ক());
                replace(resultp);
                ঘূৰাই সঁচা;
            }
        }
        ঘূৰাই মিছা;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক দ্বিমিক(Vertex* ধ্রুৱ vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_দ্বিমিক, Vertex>::মান, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::মান, "Must invoke on final class");

        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ধ্রুৱ lConstp = lhsp->cast<DfgConst>();
        DfgConst* ধ্রুৱ rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* ধ্রুৱ lSamep = lhsp->cast<Vertex>();
        Vertex* ধ্রুৱ rSamep = rhsp->cast<Vertex>();

        // Constant folding
        যদি (lConstp && rConstp) {
            APPLYING(FOLD_দ্বিমিক) {
                DfgConst* ধ্রুৱ resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->অঙ্ক(), lConstp->অঙ্ক(), rConstp->অঙ্ক());
                replace(resultp);
                ঘূৰাই সঁচা;
            }
        }

        // Idempotent
        যদি VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::মান) {
            যদি (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_দ্বিমিক) {
                    replace(lhsp);
                    ঘূৰাই সঁচা;
                }
            }
        }

        // Associative
        যদি VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::মান) {
            // Also commutative and idempotent
            যদি VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::মান && IsIdempotent<Vertex>::মান) {
                যদি (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_দ্বিমিক_LHS) {
                        replace(lSamep);
                        ঘূৰাই সঁচা;
                    }
                }
                যদি (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_দ্বিমিক_RHS) {
                        replace(rSamep);
                        ঘূৰাই সঁচা;
                    }
                }
            }

            যদি (lConstp && rSamep) {
                যদি (DfgConst* ধ্রুৱ rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_দ্বিমিক_LHS_OF_RHS) {
                        DfgConst* ধ্রুৱ cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->অঙ্ক(), lConstp->অঙ্ক(), rlConstp->অঙ্ক());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        ঘূৰাই সঁচা;
                    }
                }
            }

            যদি (lSamep && rConstp) {
                যদি (DfgConst* ধ্রুৱ lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_দ্বিমিক_RHS_OF_LHS) {
                        DfgConst* ধ্রুৱ cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->অঙ্ক(), lrConstp->অঙ্ক(), rConstp->অঙ্ক());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        ঘূৰাই সঁচা;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            যদি (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* ধ্রুৱ ap = lSamep->lhsp();
                DfgVertex* ধ্রুৱ bp = lSamep->rhsp();
                DfgVertex* ধ্রুৱ cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                যদি (ap != cp && bp != cp) {
                    APPLYING(ডান_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ধ্রুৱ DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* ধ্রুৱ bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ঘূৰাই সঁচা;
                    }
                }
            }

            যদি (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* ধ্রুৱ rlVtxp = rSamep->lhsp();
                DfgVertex* ধ্রুৱ rrVtxp = rSamep->rhsp();

                যদি VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::মান) {
                    যদি (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            ঘূৰাই সঁচা;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                যদি (Vertex* ধ্রুৱ existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    যদি (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_দ্বিমিক_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            ঘূৰাই সঁচা;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                যদি VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::মান) {
                    যদি (Vertex* ধ্রুৱ existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        যদি (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_দ্বিমিক_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                ঘূৰাই সঁচা;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        যদি VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::মান) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            যদি (lConstp) {
                যদি (DfgCond* ধ্রুৱ rCondp = rhsp->cast<DfgCond>()) {
                    যদি (!rCondp->hasMultipleSinks()) {
                        APPLYING(ঠেলো_COMMUTATIVE_দ্বিমিক_THROUGH_COND) {
                            DfgVertex* ধ্রুৱ tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* ধ্রুৱ ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ঘূৰাই সঁচা;
                        }
                    }
                }
                ঘূৰাই মিছা;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ধ্রুৱ তার্কিক swapSides = [&]() {
            // Don't swap if not commutative, duh
            যদি VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::মান) ঘূৰাই মিছা;
            // Const go on left
            যদি (lConstp) ঘূৰাই মিছা;
            যদি (rConstp) ঘূৰাই সঁচা;
            // Not go on left
            যদি (lhsp->is<DfgNot>()) ঘূৰাই মিছা;
            যদি (rhsp->is<DfgNot>()) ঘূৰাই সঁচা;
            // Same go on right
            যদি (rSamep) ঘূৰাই মিছা;
            যদি (lSamep) ঘূৰাই সঁচা;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ধ্রুৱ VertexInfo& lInfo = m_vInfo[lhsp];
            ধ্রুৱ VertexInfo& rInfo = m_vInfo[rhsp];
            ঘূৰাই lInfo.m_id > rInfo.m_id;
        }();
        যদি (swapSides) {
            APPLYING(SWAP_SIDES_IN_দ্বিমিক) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                ঘূৰাই সঁচা;
            }
        }

        ঘূৰাই মিছা;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক distributive_And_Associative_দ্বিমিক(Vertex* vtxp) {
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        যদি (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            যদি (Other* ধ্রুৱ lp = lhsp->cast<Other>()) {
                যদি (Other* ধ্রুৱ rp = rhsp->cast<Other>()) {
                    DfgVertex* ধ্রুৱ llp = lp->lhsp();
                    DfgVertex* ধ্রুৱ lrp = lp->rhsp();
                    DfgVertex* ধ্রুৱ rlp = rp->lhsp();
                    DfgVertex* ধ্রুৱ rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    যদি (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } অন্যথা যদি (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } অন্যথা যদি (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } অন্যথা যদি (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    যদি (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_দ্বিমিক) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            ঘূৰাই সঁচা;
                        }
                    }
                }
            }
        }

        ঘূৰাই মিছা;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক
    try_ঠেলো_Bitwise_Op_Through_Concat(Vertex* ধ্রুৱ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ধ্রুৱ catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ধ্রুৱ catRConstp = concatp->rhsp()->cast<DfgConst>();
        যদি (catLConstp || catRConstp) {
            APPLYING(ঠেলো_BITWISE_OP_THROUGH_CONCAT) {
                ধ্রুৱ uint32_t width = concatp->width();
                ধ্রুৱ DfgDataType& lDtype = concatp->lhsp()->dtype();
                ধ্রুৱ DfgDataType& rDtype = concatp->rhsp()->dtype();
                ধ্রুৱ uint32_t lWidth = lDtype.size();
                ধ্রুৱ uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* ধ্রুৱ newLhsp = [&]() -> DfgVertex* {
                    DfgConst* ধ্রুৱ newLhsConstp = makeZero(constp->fileline(), lWidth);
                    যদি (catLConstp) {
                        V3_সংখ্যা অঙ্ক{constp->fileline(), static_cast<পূর্ণাঙ্ক>(lWidth), 0u};
                        অঙ্ক.opSel(constp->অঙ্ক(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->অঙ্ক(), অঙ্ক, catLConstp->অঙ্ক());
                        ঘূৰাই newLhsConstp;
                    }
                    newLhsConstp->অঙ্ক().opSel(constp->অঙ্ক(), width - 1, rWidth);
                    ঘূৰাই make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* ধ্রুৱ newRhsp = [&]() -> DfgVertex* {
                    DfgConst* ধ্রুৱ newRhsConstp = makeZero(constp->fileline(), rWidth);
                    যদি (catRConstp) {
                        V3_সংখ্যা অঙ্ক{constp->fileline(), static_cast<পূর্ণাঙ্ক>(rWidth), 0u};
                        অঙ্ক.opSel(constp->অঙ্ক(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->অঙ্ক(), অঙ্ক, catRConstp->অঙ্ক());
                        ঘূৰাই newRhsConstp;
                    }
                    newRhsConstp->অঙ্ক().opSel(constp->অঙ্ক(), rWidth - 1, 0);
                    ঘূৰাই make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ঘূৰাই সঁচা;
            }
        }
        ঘূৰাই মিছা;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক
    try_ঠেলো_Compare_Op_Through_Concat(Vertex* ধ্রুৱ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        যদি (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ঠেলো_COMPARE_OP_THROUGH_CONCAT) {
                ধ্রুৱ uint32_t width = concatp->width();
                ধ্রুৱ uint32_t lWidth = concatp->lhsp()->width();
                ধ্রুৱ uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* ধ্রুৱ newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->অঙ্ক().opSel(constp->অঙ্ক(), width - 1, rWidth);
                Vertex* ধ্রুৱ newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ধ্রুৱ newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->অঙ্ক().opSel(constp->অঙ্ক(), rWidth - 1, 0);
                Vertex* ধ্রুৱ newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                যদি VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::মান) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } অন্যথা যদি VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::মান) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } অন্যথা {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ঘূৰাই সঁচা;
            }
        }
        ঘূৰাই মিছা;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক try_ঠেলো_Bitwise_Op_Through_Reductions(Bitwise* ধ্রুৱ vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        যদি (Reduction* ধ্রুৱ lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            যদি (Reduction* ধ্রুৱ rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* ধ্রুৱ lSrcp = lRedp->srcp();
                DfgVertex* ধ্রুৱ rSrcp = rRedp->srcp();
                যদি (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(ঠেলো_BITWISE_THROUGH_REDUCTION) {
                        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
                        Bitwise* ধ্রুৱ bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        ঘূৰাই সঁচা;
                    }
                }
            }
        }

        ঘূৰাই মিছা;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    স্থির std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        যদি (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ঘূৰাই {nullptr, nullptr};
        }
        যদি (vtxp->hasMultipleSinks()) ঘূৰাই {nullptr, nullptr};
        Dfg_Vertex_দ্বিমিক* ধ্রুৱ bwp = vtxp->as<Dfg_Vertex_দ্বিমিক>();
        DfgSel* ধ্রুৱ lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ধ্রুৱ rSelp = bwp->rhsp()->cast<DfgSel>();
        যদি (!lSelp || !rSelp) ঘূৰাই {nullptr, nullptr};
        DfgSel* ধ্রুৱ nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* ধ্রুৱ wSelp = nSelp == lSelp ? rSelp : lSelp;
        ধ্রুৱ uint32_t nWidth = nSelp->fromp()->width();
        যদি (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ঘূৰাই {nullptr, nullptr};
        যদি (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            ঘূৰাই {nullptr, nullptr};
        }
        ঘূৰাই {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক try_ঠেলো_Bitwise_Op_Throug_Sel(Bitwise* ধ্রুৱ vtxp) {
        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();

        যদি (DfgSel* ধ্রুৱ lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            যদি (DfgSel* ধ্রুৱ selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } অন্যথা যদি (Bitwise* ধ্রুৱ bitwisep = rhsp->cast<Bitwise>()) {
                যদি (DfgSel* ধ্রুৱ rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } অন্যথা যদি (DfgSel* ধ্রুৱ rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            যদি (rSelp) {
                DfgVertex* ধ্রুৱ lFromp = lSelp->fromp();
                DfgVertex* ধ্রুৱ rFromp = rSelp->fromp();
                যদি (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ঠেলো_BITWISE_THROUGH_SEL) {
                        Bitwise* ধ্রুৱ bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        যদি (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ঘূৰাই সঁচা;
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
            Bitwise* ধ্রুৱ rChainp = rhsp->cast<Bitwise>();
            যদি (rChainp && rChainp->hasMultipleSinks()) ঘূৰাই মিছা;
            DfgVertex* ধ্রুৱ rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ধ্রুৱ auto lSels = pushableUnalignedSels(lhsp);
            ধ্রুৱ auto rSels = pushableUnalignedSels(rTermp);
            যদি (!lSels.first && !rSels.first) ঘূৰাই মিছা;

            ধ্রুৱ DfgVertex* ধ্রুৱ opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            যদি (opp->is<Bitwise>()) ঘূৰাই মিছা;
            ধ্রুৱ auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* ধ্রুৱ nFromp = sels.first->fromp();
            DfgVertex* ধ্রুৱ wFromp = sels.second->fromp();
            ধ্রুৱ uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            ধ্রুৱ auto matches = [&](ধ্রুৱ DfgVertex* termp, ধ্রুৱ auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                যদি (tSels.first) {
                    ঘূৰাই termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                যদি (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                যদি (ধ্রুৱ DfgSel* ধ্রুৱ selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                যদি (termp->type() != opp->type()) ঘূৰাই মিছা;
                // Operands might have been swapped since
                ধ্রুৱ Dfg_Vertex_দ্বিমিক* ধ্রুৱ bwp = termp->as<Dfg_Vertex_দ্বিমিক>();
                ধ্রুৱ তার্কিক nIsLhs = bwp->lhsp() == nFromp;
                যদি (!nIsLhs && bwp->rhsp() != nFromp) ঘূৰাই মিছা;
                ধ্রুৱ DfgSel* ধ্রুৱ slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ঘূৰাই slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            যদি (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ঘূৰাই মিছা;

            APPLYING(ঠেলো_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
                ধ্রুৱ DfgDataType& dtype = nFromp->dtype();
                DfgSel* ধ্রুৱ slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                যদি (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } অন্যথা যদি (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } অন্যথা {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ধ্রুৱ auto ঠেলো = [&](DfgVertex* termp, ধ্রুৱ DfgSel* nSelp) -> DfgVertex* {
                    ঘূৰাই nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* ধ্রুৱ newLhsp = ঠেলো(lhsp, lSels.first);
                DfgVertex* ধ্রুৱ newRTermp = ঠেলো(rTermp, rSels.first);
                যদি (rChainp) {
                    DfgVertex* ধ্রুৱ newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } অন্যথা {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ঘূৰাই সঁচা;
            }
        }

        ঘূৰাই মিছা;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    স্থির তার্কিক isConcatFoldable(ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        যদি (isSame(lhsp, rhsp)) ঘূৰাই সঁচা;
        যদি (ধ্রুৱ DfgRep* ধ্রুৱ lRepp = lhsp->cast<DfgRep>()) {
            যদি (isSame(lRepp->srcp(), rhsp)) ঘূৰাই সঁচা;
        }
        যদি (ধ্রুৱ DfgRep* ধ্রুৱ rRepp = rhsp->cast<DfgRep>()) {
            যদি (isSame(lhsp, rRepp->srcp())) ঘূৰাই সঁচা;
        }
        // Concatenation of adjoining selects is a select
        যদি (ধ্রুৱ DfgSel* ধ্রুৱ lSelp = lhsp->cast<DfgSel>()) {
            যদি (ধ্রুৱ DfgSel* ধ্রুৱ rSelp = rhsp->cast<DfgSel>()) {
                ঘূৰাই isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        যদি (ধ্রুৱ DfgNot* ধ্রুৱ lNotp = lhsp->cast<DfgNot>()) {
            যদি (ধ্রুৱ DfgNot* ধ্রুৱ rNotp = rhsp->cast<DfgNot>()) {
                ঘূৰাই !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ঘূৰাই মিছা;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    স্থির তার্কিক isSameBitwise(ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) {
        যদি (lhsp->type() != rhsp->type()) ঘূৰাই মিছা;
        ঘূৰাই lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    স্থির তার্কিক isConcatSimplifiable(ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp) {
        যদি (isConcatFoldable(lhsp, rhsp)) ঘূৰাই সঁচা;
        যদি (!isSameBitwise(lhsp, rhsp)) ঘূৰাই মিছা;
        যদি (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ঘূৰাই মিছা;
        ধ্রুৱ Dfg_Vertex_দ্বিমিক* ধ্রুৱ lp = lhsp->as<Dfg_Vertex_দ্বিমিক>();
        ধ্রুৱ Dfg_Vertex_দ্বিমিক* ধ্রুৱ rp = rhsp->as<Dfg_Vertex_দ্বিমিক>();
        যদি (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ঘূৰাই সঁচা;
        যদি (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ঘূৰাই সঁচা;
        ঘূৰাই মিছা;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    স্থির তার্কিক isConcatPushableThroughBitwise(ধ্রুৱ DfgVertex* lhsp, ধ্রুৱ DfgVertex* rhsp,
                                               তার্কিক& swap) {
        যদি (!isSameBitwise(lhsp, rhsp)) ঘূৰাই মিছা;
        যদি (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ঘূৰাই মিছা;
        ধ্রুৱ Dfg_Vertex_দ্বিমিক* ধ্রুৱ lp = lhsp->as<Dfg_Vertex_দ্বিমিক>();
        ধ্রুৱ Dfg_Vertex_দ্বিমিক* ধ্রুৱ rp = rhsp->as<Dfg_Vertex_দ্বিমিক>();
        swap = মিছা;
        যদি (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ঘূৰাই সঁচা;
        যদি (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ঘূৰাই সঁচা;
        swap = সঁচা;
        যদি (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ঘূৰাই সঁচা;
        যদি (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ঘূৰাই সঁচা;
        ঘূৰাই মিছা;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_সাৰি* flp, ধ্রুৱ DfgVertex* lhsp,
                                              ধ্রুৱ DfgVertex* rhsp, তার্কিক swap) {
        ধ্রুৱ Dfg_Vertex_দ্বিমিক* ধ্রুৱ lp = lhsp->as<Dfg_Vertex_দ্বিমিক>();
        ধ্রুৱ Dfg_Vertex_দ্বিমিক* ধ্রুৱ rp = rhsp->as<Dfg_Vertex_দ্বিমিক>();
        ধ্রুৱ DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* ধ্রুৱ cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* ধ্রুৱ dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* ধ্রুৱ newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ধ্রুৱ newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        যদি (lp->is<DfgAnd>()) ঘূৰাই make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        যদি (lp->is<DfgOr>()) ঘূৰাই make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ঘূৰাই make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();

        যদি (DfgSel* ধ্রুৱ lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            যদি (DfgSel* ধ্রুৱ selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } অন্যথা যদি (Bitwise* ধ্রুৱ rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } অন্যথা যদি (Reduction* ধ্রুৱ rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            যদি (rSelp) {
                uint32_t lsb = 0;
                যদি (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        ধ্রুৱ DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* ধ্রুৱ newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        যদি (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ঘূৰাই সঁচা;
                    }
                }
            }
        }

        যদি (Reduction* ধ্রুৱ lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            যদি (!rRedp) {
                যদি (Bitwise* ধ্রুৱ rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            যদি (rRedp) {
                যদি (DfgSel* ধ্রুৱ lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    যদি (DfgSel* ধ্রুৱ rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        যদি (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                ধ্রুৱ DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ধ্রুৱ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                যদি (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ঘূৰাই সঁচা;
                            }
                        }
                    }
                }
            }
        }

        ঘূৰাই মিছা;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক optimizeReduction(Reduction* ধ্রুৱ vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        যদি (foldUnary(vtxp)) ঘূৰাই সঁচা;

        DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        // Reduction of 1-bit value
        যদি (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                ঘূৰাই সঁচা;
            }
        }

        যদি (DfgCond* ধ্রুৱ condp = srcp->cast<DfgCond>()) {
            যদি (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ঠেলো_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ধ্রুৱ newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ধ্রুৱ newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ধ্রুৱ newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ঘূৰাই সঁচা;
                }
            }
        }

        যদি (DfgConcat* ধ্রুৱ concatp = srcp->cast<DfgConcat>()) {
            যদি (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(ঠেলো_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ধ্রুৱ lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* ধ্রুৱ rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    ঘূৰাই সঁচা;
                }
            }
        }

        যদি (Bitwise* ধ্রুৱ bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            যদি (!bitwisep->hasMultipleSinks()) {
                যদি (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(ঠেলো_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* ধ্রুৱ newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* ধ্রুৱ newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        ঘূৰাই সঁচা;
                    }
                }

                যদি (DfgSel* ধ্রুৱ lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    যদি (DfgSel* ধ্রুৱ rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        যদি (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ঠেলো_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                ধ্রুৱ DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ধ্রুৱ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ঘূৰাই সঁচা;
                            }
                        }
                    }
                }
            }
        }

        ঘূৰাই মিছা;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT তার্কিক optimizeShiftRHS(Shift* ধ্রুৱ vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_দ্বিমিক, Shift>::মান, "Must invoke on binary");
        static_assert(std::is_final<Shift>::মান, "Must invoke on final class");
        যদি (ধ্রুৱ DfgConcat* ধ্রুৱ concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            যদি (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ঘূৰাই সঁচা;
                }
            }
        }
        ঘূৰাই মিছা;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_গুনতি_Ones_Term(DfgVertex* vtxp) {
        যদি (DfgConcat* ধ্রুৱ oCatp = vtxp->cast<DfgConcat>()) {
            যদি (isZero(oCatp->lhsp())) {
                যদি (Dfg_গুনতি_Ones* ধ্রুৱ গুনতি_Onesp = oCatp->rhsp()->cast<Dfg_গুনতি_Ones>()) {
                    // Zero extended count ones
                    যদি (DfgSel* ধ্রুৱ selp = গুনতি_Onesp->srcp()->cast<DfgSel>()) {
                        ঘূৰাই {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } অন্যথা যদি (DfgSel* ধ্রুৱ selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    যদি (selp->dtype() == m_bitDType) {  //
                        ঘূৰাই {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            ঘূৰাই {nullptr, 0, 0};
        }
        যদি (Dfg_গুনতি_Ones* ধ্রুৱ গুনতি_Onesp = vtxp->cast<Dfg_গুনতি_Ones>()) {
            // Simple count ones
            যদি (DfgSel* ধ্রুৱ selp = গুনতি_Onesp->srcp()->cast<DfgSel>()) {
                ঘূৰাই {selp->fromp(), selp->lsb(), selp->width()};
            }
            ঘূৰাই {nullptr, 0, 0};
        }
        যদি (DfgSel* ধ্রুৱ oSelp = vtxp->cast<DfgSel>()) {
            যদি (oSelp->lsb() == 0) {
                // Truncated count ones
                যদি (Dfg_গুনতি_Ones* ধ্রুৱ গুনতি_Onesp = oSelp->fromp()->cast<Dfg_গুনতি_Ones>()) {
                    // Zero extended count ones
                    যদি (DfgSel* ধ্রুৱ selp = গুনতি_Onesp->srcp()->cast<DfgSel>()) {
                        ঘূৰাই {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            যদি (oSelp->dtype() == m_bitDType) {  //
                ঘূৰাই {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ঘূৰাই {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        যদি (DfgShiftR* ধ্রুৱ shiftrp = vtxp->cast<DfgShiftR>()) {
            যদি (DfgConst* ধ্রুৱ rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                যদি (rConstp->toU32() == shiftrp->width() - 1) {
                    ঘূৰাই {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            ঘূৰাই {nullptr, 0, 0};
        }
        // Not applicable
        ঘূৰাই {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              ধ্রুৱ uint32_t width) {
        যেতিয়ালৈ (সঁচা) {
            ধ্রুৱ uint32_t msb = lsb + width - 1;

            // Sel from Concat
            যদি (DfgConcat* ধ্রুৱ concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* ধ্রুৱ lhsp = concatp->lhsp();
                DfgVertex* ধ্রুৱ rhsp = concatp->rhsp();

                যদি (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        অনুৱর্ত;
                    }
                } অন্যথা যদি (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        অনুৱর্ত;
                    }
                }
            }

            যদি (DfgRep* ধ্রুৱ repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ধ্রুৱ uint32_t srcWidth = repp->srcp()->width();
                যদি (width <= srcWidth) {
                    ধ্রুৱ uint32_t newLsb = lsb % srcWidth;
                    ধ্রুৱ uint32_t newMsb = newLsb + width - 1;
                    যদি (newMsb < srcWidth) {
                        APPLYING(ঠেলো_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            অনুৱর্ত;
                        }
                    }
                }
            }

            // Sel from Sel
            যদি (DfgSel* ধ্রুৱ selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    অনুৱর্ত;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            যদি (DfgVarPacked* ধ্রুৱ varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ধ্রুৱ auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* ধ্রুৱ driverp = pair.first;
                ধ্রুৱ uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                যদি (driverp) {
                    যদি (driverp == varp->defaultp()) {
                        APPLYING(ঠেলো_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            অনুৱর্ত;
                        }
                    } অন্যথা {
                        APPLYING(ঠেলো_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            অনুৱর্ত;
                        }
                    }
                }
            }

            // No patterns matched, stop
            ভঙ্গ;
        }
        ঘূৰাই {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        যদি (ap == bp) ঘূৰাই {ap, ap->width()};

        // If both constants, check LSBs
        যদি (DfgConst* ধ্রুৱ aConstp = ap->cast<DfgConst>()) {
            যদি (DfgConst* ধ্রুৱ bConstp = bp->cast<DfgConst>()) {
                ধ্রুৱ V3_সংখ্যা& a_অঙ্ক = aConstp->অঙ্ক();
                ধ্রুৱ V3_সংখ্যা& b_অঙ্ক = bConstp->অঙ্ক();
                // Max match is the shorter constant
                ধ্রুৱ uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                বাবে (; matchWidth < maxMatch; ++matchWidth) {
                    যদি (a_অঙ্ক.bitIs0(matchWidth) != b_অঙ্ক.bitIs0(matchWidth)) ভঙ্গ;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ধ্রুৱ shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                ঘূৰাই {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        যদি (DfgConcat* ধ্রুৱ catp = ap->cast<DfgConcat>()) ঘূৰাই commonLSBs(catp->rhsp(), bp);
        যদি (DfgConcat* ধ্রুৱ catp = bp->cast<DfgConcat>()) ঘূৰাই commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        যদি (DfgSel* ধ্রুৱ selp = ap->cast<DfgSel>()) {
            যদি (selp->lsb() == 0) {
                DfgVertex* ধ্রুৱ fromp = selp->fromp();
                ধ্রুৱ std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                ঘূৰাই {common.first, std::min(common.second, selp->width())};
            }
        }
        যদি (DfgSel* ধ্রুৱ selp = bp->cast<DfgSel>()) {
            যদি (selp->lsb() == 0) {
                DfgVertex* ধ্রুৱ fromp = selp->fromp();
                ধ্রুৱ std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                ঘূৰাই {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        ঘূৰাই {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        যদি (ap == bp) ঘূৰাই {ap, ap->width()};

        // If both constants, check MSBs
        যদি (DfgConst* ধ্রুৱ aConstp = ap->cast<DfgConst>()) {
            যদি (DfgConst* ধ্রুৱ bConstp = bp->cast<DfgConst>()) {
                ধ্রুৱ uint32_t aMsb = aConstp->width() - 1;
                ধ্রুৱ uint32_t bMsb = bConstp->width() - 1;
                ধ্রুৱ V3_সংখ্যা& a_অঙ্ক = aConstp->অঙ্ক();
                ধ্রুৱ V3_সংখ্যা& b_অঙ্ক = bConstp->অঙ্ক();
                // Max match is the shorter constant
                ধ্রুৱ uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                বাবে (; matchWidth < maxMatch; ++matchWidth) {
                    যদি (a_অঙ্ক.bitIs0(aMsb - matchWidth) != b_অঙ্ক.bitIs0(bMsb - matchWidth)) ভঙ্গ;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ধ্রুৱ shorterp = aMsb < bMsb ? aConstp : bConstp;
                ঘূৰাই {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        যদি (DfgConcat* ধ্রুৱ catp = ap->cast<DfgConcat>()) ঘূৰাই commonMSBs(catp->lhsp(), bp);
        যদি (DfgConcat* ধ্রুৱ catp = bp->cast<DfgConcat>()) ঘূৰাই commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        যদি (DfgSel* ধ্রুৱ selp = ap->cast<DfgSel>()) {
            DfgVertex* ধ্রুৱ fromp = selp->fromp();
            যদি (selp->msb() == fromp->width() - 1) {
                ধ্রুৱ std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                ঘূৰাই {common.first, std::min(common.second, selp->width())};
            }
        }
        যদি (DfgSel* ধ্রুৱ selp = bp->cast<DfgSel>()) {
            DfgVertex* ধ্রুৱ fromp = selp->fromp();
            যদি (selp->msb() == fromp->width() - 1) {
                ধ্রুৱ std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                ঘূৰাই {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        ঘূৰাই {nullptr, 0};
    }

    // VISIT methods

    শূন্য visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    শূন্য visit(Dfg_গুনতি_Ones* ধ্রুৱ vtxp) override {
        যদি (foldUnary(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgExtend* ধ্রুৱ vtxp) override {
        যদি (foldUnary(vtxp)) ঘূৰাই;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* ধ্রুৱ zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ঘূৰাই;
        }
    }

    শূন্য visit(DfgExtendS* ধ্রুৱ vtxp) override {
        যদি (foldUnary(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgLogNot* ধ্রুৱ vtxp) override {
        যদি (foldUnary(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgNegate* ধ্রুৱ vtxp) override {
        যদি (foldUnary(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgNot* ধ্রুৱ vtxp) override {
        যদি (foldUnary(vtxp)) ঘূৰাই;

        // Not of Cond
        যদি (DfgCond* ধ্রুৱ condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            যদি (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ঠেলো_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ধ্রুৱ newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ধ্রুৱ newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ধ্রুৱ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ঘূৰাই;
                }
            }
        }

        // Not of Not
        যদি (DfgNot* ধ্রুৱ notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ঘূৰাই;
            }
        }

        যদি (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            যদি (DfgEq* ধ্রুৱ eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ঘূৰাই;
                }
            }

            // Not of Neq
            যদি (DfgNeq* ধ্রুৱ neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ঘূৰাই;
                }
            }

            // Not of unsigned comparisons
            যদি (DfgLt* ধ্রুৱ p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ঘূৰাই;
                }
            }
            যদি (DfgGte* ধ্রুৱ p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ঘূৰাই;
                }
            }
            যদি (DfgGt* ধ্রুৱ p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ঘূৰাই;
                }
            }
            যদি (DfgLte* ধ্রুৱ p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ঘূৰাই;
                }
            }

            // Not of signed comparisons
            যদি (DfgLtS* ধ্রুৱ p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ঘূৰাই;
                }
            }
            যদি (DfgGteS* ধ্রুৱ p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ঘূৰাই;
                }
            }
            যদি (DfgGtS* ধ্রুৱ p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ঘূৰাই;
                }
            }
            যদি (DfgLteS* ধ্রুৱ p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgOneHot* ধ্রুৱ vtxp) override {
        যদি (foldUnary(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgOneHot0* ধ্রুৱ vtxp) override {
        যদি (foldUnary(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgRedOr* ধ্রুৱ vtxp) override {
        যদি (optimizeReduction(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgRedAnd* ধ্রুৱ vtxp) override {
        যদি (optimizeReduction(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgRedXor* ধ্রুৱ vtxp) override {
        যদি (optimizeReduction(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgSel* ধ্রুৱ vtxp) override {
        DfgVertex* ধ্রুৱ fromp = vtxp->fromp();

        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        ধ্রুৱ uint32_t lsb = vtxp->lsb();
        ধ্রুৱ uint32_t width = vtxp->width();
        ধ্রুৱ uint32_t msb = lsb + width - 1;

        যদি (DfgConst* ধ্রুৱ constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ধ্রুৱ resp = makeZero(flp, width);
                resp->অঙ্ক().opSel(constp->অঙ্ক(), msb, lsb);
                replace(resp);
                ঘূৰাই;
            }
        }

        // Full width select, replace with the source.
        যদি (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                ঘূৰাই;
            }
        }

        // Unwind through bit packing in one go
        {
            ধ্রুৱ auto res = unwindSel(fromp, lsb, width);
            যদি (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                ঘূৰাই;
            }
        }

        // Sel from Not
        যদি (DfgNot* ধ্রুৱ notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            যদি (!notp->hasMultipleSinks()) {
                APPLYING(ঠেলো_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ধ্রুৱ newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ঘূৰাই;
                }
            }
        }

        // Sel from Cond
        যদি (DfgCond* ধ্রুৱ condp = fromp->cast<DfgCond>()) {
            যদি (!condp->hasMultipleSinks()) {
                APPLYING(ঠেলো_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ধ্রুৱ newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ধ্রুৱ newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ধ্রুৱ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ঘূৰাই;
                }
            }
        }

        // Sel from ShiftL
        যদি (DfgShiftL* ধ্রুৱ shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            যদি (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(ঠেলো_SEL_THROUGH_SHIFTL) {
                    DfgSel* ধ্রুৱ newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgMux* ধ্রুৱ vtxp) override {
        DfgVertex* ধ্রুৱ fromp = vtxp->fromp();
        DfgVertex* ধ্রুৱ lsbp = vtxp->lsbp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (DfgConst* ধ্রুৱ lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->অঙ্ক().toUInt()));
                ঘূৰাই;
            }
        }

        যদি (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                ঘূৰাই;
            }
        }

        যদি (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                ঘূৰাই;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    শূন্য visit(DfgAnd* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        যদি (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            যদি (DfgNot* ধ্রুৱ lhsNotp = lhsp->cast<DfgNot>()) {
                যদি (DfgNot* ধ্রুৱ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* ধ্রুৱ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ঘূৰাই;
                    }
                }
                যদি (DfgNeq* ধ্রুৱ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* ধ্রুৱ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ধ্রুৱ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (DfgConst* ধ্রুৱ lConstp = lhsp->cast<DfgConst>()) {
            যদি (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ঘূৰাই;
                }
            }

            যদি (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ঘূৰাই;
                }
            }

            যদি (DfgConcat* ধ্রুৱ rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ঘূৰাই;
            }
        }

        যদি (distributive_And_Associative_দ্বিমিক<DfgOr, DfgAnd>(vtxp)) ঘূৰাই;

        যদি (try_ঠেলো_Bitwise_Op_Through_Reductions(vtxp)) ঘূৰাই;

        যদি (try_ঠেলো_Bitwise_Op_Throug_Sel(vtxp)) ঘূৰাই;

        {
            DfgNot* ধ্রুৱ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ধ্রুৱ rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            যদি ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    ঘূৰাই;
                }
            }

            যদি (DfgAnd* ধ্রুৱ rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ধ্রুৱ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ধ্রুৱ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                যদি ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (vtxp->dtype() == m_bitDType) {
            যদি (tryReplaceBitwiseWithReduction(vtxp)) ঘূৰাই;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            যদি (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            যদি (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ধ্রুৱ newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgOr* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        যদি (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            যদি (DfgNot* ধ্রুৱ lhsNotp = lhsp->cast<DfgNot>()) {
                যদি (DfgNot* ধ্রুৱ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* ধ্রুৱ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ঘূৰাই;
                    }
                }
                যদি (DfgNeq* ধ্রুৱ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* ধ্রুৱ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ধ্রুৱ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (DfgConcat* ধ্রুৱ lhsConcatp = lhsp->cast<DfgConcat>()) {
            যদি (DfgConcat* ধ্রুৱ rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    যদি (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ঘূৰাই;
                        }
                    }
                    যদি (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ঘূৰাই;
                        }
                    }
                }
            }
        }

        যদি (DfgConst* ধ্রুৱ lConstp = lhsp->cast<DfgConst>()) {
            যদি (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ঘূৰাই;
                }
            }

            যদি (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ঘূৰাই;
                }
            }

            যদি (DfgConcat* ধ্রুৱ rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ঘূৰাই;
            }
        }

        যদি (distributive_And_Associative_দ্বিমিক<DfgAnd, DfgOr>(vtxp)) ঘূৰাই;

        যদি (try_ঠেলো_Bitwise_Op_Through_Reductions(vtxp)) ঘূৰাই;

        যদি (try_ঠেলো_Bitwise_Op_Throug_Sel(vtxp)) ঘূৰাই;

        {
            DfgNot* ধ্রুৱ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ধ্রুৱ rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            যদি ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    ঘূৰাই;
                }
            }

            যদি (DfgOr* ধ্রুৱ rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ধ্রুৱ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ধ্রুৱ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                যদি ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (vtxp->dtype() == m_bitDType) {
            যদি (tryReplaceBitwiseWithReduction(vtxp)) ঘূৰাই;
        }
    }

    শূন্য visit(DfgXor* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                ঘূৰাই;
            }
        }

        যদি (DfgConst* ধ্রুৱ lConstp = lhsp->cast<DfgConst>()) {
            যদি (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ঘূৰাই;
                }
            }
            যদি (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ঘূৰাই;
                }
            }
            যদি (DfgConcat* ধ্রুৱ rConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) ঘূৰাই;
            }
        }

        যদি (try_ঠেলো_Bitwise_Op_Through_Reductions(vtxp)) ঘূৰাই;

        যদি (try_ঠেলো_Bitwise_Op_Throug_Sel(vtxp)) ঘূৰাই;

        যদি (vtxp->dtype() == m_bitDType) {
            যদি (tryReplaceBitwiseWithReduction(vtxp)) ঘূৰাই;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    শূন্য visit(DfgAdd* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                ঘূৰাই;
            }
        }

        ধ্রুৱ std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_গুনতি_Ones_Term(lhsp);
        যদি (DfgVertex* ধ্রুৱ lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_গুনতি_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            যদি (!std::get<0>(rTerm)) {
                যদি (DfgAdd* ধ্রুৱ rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_গুনতি_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            যদি (DfgVertex* ধ্রুৱ rVtxp = std::get<0>(rTerm)) {
                যদি (isSame(lVtxp, rVtxp)) {
                    ধ্রুৱ uint32_t lLsb = std::get<1>(lTerm);
                    ধ্রুৱ uint32_t rLsb = std::get<1>(rTerm);
                    ধ্রুৱ uint32_t lWidth = std::get<2>(lTerm);
                    ধ্রুৱ uint32_t rWidth = std::get<2>(rTerm);
                    তার্কিক adjoined = সঁচা;
                    uint32_t lsb = 0;
                    যদি (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } অন্যথা যদি (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } অন্যথা {
                        adjoined = মিছা;
                    }
                    যদি (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_গুনতি_ONES) {
                            DfgSel* ধ্রুৱ selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_গুনতি_Ones>(flp, DfgDataType::packed(32), selp);
                            যদি (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } অন্যথা যদি (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            যদি (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            ঘূৰাই;
                        }
                    }
                }
            }
        }
    }

    শূন্য visit(DfgArraySel* ধ্রুৱ vtxp) override {
        DfgConst* ধ্রুৱ idxp = vtxp->bitp()->cast<DfgConst>();
        যদি (!idxp) ঘূৰাই;
        DfgVarArray* ধ্রুৱ varp = vtxp->fromp()->cast<DfgVarArray>();
        যদি (!varp) ঘূৰাই;
        AstVar* ধ্রুৱ astVarp = varp->vscp()->varp();
        যদি (astVarp->isForced()) ঘূৰাই;
        যদি (astVarp->isSigUserRWPublic()) ঘূৰাই;
        DfgVertex* ধ্রুৱ srcp = varp->srcp();
        যদি (!srcp) {
            যদি (vtxp->isPacked()) {
                যদি (AstConsArray* ধ্রুৱ iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    যদি (AstConst* ধ্রুৱ valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(নূতন DfgConst{m_dfg, valp->fileline(), valp->অঙ্ক()});
                            ঘূৰাই;
                        }
                    }
                }
            }
            ঘূৰাই;
        }

        যদি (DfgSpliceArray* ধ্রুৱ splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* ধ্রুৱ driverp = splicep->driverAt(idxp->toSizeT());
            যদি (!driverp) ঘূৰাই;
            DfgUnitArray* ধ্রুৱ uap = driverp->cast<DfgUnitArray>();
            যদি (!uap) ঘূৰাই;
            যদি (uap->srcp()->is<DfgVertexSplice>()) ঘূৰাই;
            // If driven by a variable that had a Driver in DFG, it is partial
            যদি (DfgVertexVar* ধ্রুৱ dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                যদি (dvarp->srcp()) ঘূৰাই;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ঘূৰাই;
            }
        }

        যদি (DfgUnitArray* ধ্রুৱ uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            যদি (uap->srcp()->is<DfgSplicePacked>()) ঘূৰাই;
            // If driven by a variable that had a Driver in DFG, it is partial
            যদি (DfgVertexVar* ধ্রুৱ dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                যদি (dvarp->srcp()) ঘূৰাই;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgConcat* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();

        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isZero(lhsp)) {
            DfgConst* ধ্রুৱ lConstp = lhsp->as<DfgConst>();
            যদি (DfgSel* ধ্রুৱ rSelp = rhsp->cast<DfgSel>()) {
                যদি (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_শীৰ্ষ_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (isZero(rhsp)) {
            DfgConst* ধ্রুৱ rConstp = rhsp->as<DfgConst>();
            যদি (DfgSel* ধ্রুৱ lSelp = lhsp->cast<DfgSel>()) {
                যদি (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (DfgNot* ধ্রুৱ lNot = lhsp->cast<DfgNot>()) {
            যদি (DfgNot* ধ্রুৱ rNot = rhsp->cast<DfgNot>()) {
                যদি (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(ঠেলো_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ধ্রুৱ newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ঘূৰাই;
                    }
                }
            }
        }

        {
            তার্কিক swap = মিছা;
            যদি (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(ঠেলো_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    ঘূৰাই;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            যদি (DfgConcat* ধ্রুৱ rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* ধ্রুৱ rlp = rConcatp->lhsp();
                যদি (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(ঠেলো_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* ধ্রুৱ newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (DfgSel* ধ্রুৱ lSelp = lhsp->cast<DfgSel>()) {
            যদি (DfgSel* ধ্রুৱ rSelp = rhsp->cast<DfgSel>()) {
                যদি (isSame(lSelp->fromp(), rSelp->fromp())) {
                    যদি (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ঘূৰাই;
                        }
                    }
                }
            }

            যদি (DfgConcat* ধ্রুৱ rConcatp = rhsp->cast<DfgConcat>()) {
                যদি (DfgSel* ধ্রুৱ rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    যদি (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        যদি (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                ধ্রুৱ uint32_t width = lSelp->width() + rlSelp->width();
                                ধ্রুৱ DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ধ্রুৱ selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ঘূৰাই;
                            }
                        }
                    }
                }
            }
        }

        যদি (DfgSel* ধ্রুৱ rSelp = rhsp->cast<DfgSel>()) {
            যদি (DfgConcat* ধ্রুৱ lConcatp = lhsp->cast<DfgConcat>()) {
                যদি (DfgSel* ধ্রুৱ lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    যদি (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        যদি (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                ধ্রুৱ uint32_t width = lrSelp->width() + rSelp->width();
                                ধ্রুৱ DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ধ্রুৱ selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ঘূৰাই;
                            }
                        }
                    }
                }
            }
        }

        যদি (DfgConst* ধ্রুৱ lConstp = lhsp->cast<DfgConst>()) {
            যদি (DfgCond* ধ্রুৱ rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* ধ্রুৱ rtVtxp = rCondp->thenp();
                DfgVertex* ধ্রুৱ reVtxp = rCondp->elsep();
                DfgConst* ধ্রুৱ rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ধ্রুৱ reConstp = reVtxp->cast<DfgConst>();
                যদি (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(ঠেলো_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* ধ্রুৱ thenp = [&]() -> DfgVertex* {
                            File_সাৰি* ধ্রুৱ rtFlp = rtVtxp->fileline();
                            যদি (rtConstp) {
                                DfgConst* ধ্রুৱ constp = makeZero(rtFlp, vtxp->width());
                                constp->অঙ্ক().opConcat(lConstp->অঙ্ক(), rtConstp->অঙ্ক());
                                ঘূৰাই constp;
                            }
                            ঘূৰাই make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* ধ্রুৱ elsep = [&]() -> DfgVertex* {
                            File_সাৰি* ধ্রুৱ reFlp = reVtxp->fileline();
                            যদি (reConstp) {
                                DfgConst* ধ্রুৱ constp = makeZero(reFlp, vtxp->width());
                                constp->অঙ্ক().opConcat(lConstp->অঙ্ক(), reConstp->অঙ্ক());
                                ঘূৰাই constp;
                            }
                            ঘূৰাই make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (DfgConst* ধ্রুৱ rConstp = rhsp->cast<DfgConst>()) {
            যদি (DfgCond* ধ্রুৱ lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* ধ্রুৱ ltVtxp = lCondp->thenp();
                DfgVertex* ধ্রুৱ leVtxp = lCondp->elsep();
                DfgConst* ধ্রুৱ ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ধ্রুৱ leConstp = leVtxp->cast<DfgConst>();
                যদি (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(ঠেলো_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* ধ্রুৱ thenp = [&]() -> DfgVertex* {
                            File_সাৰি* ধ্রুৱ ltFlp = ltVtxp->fileline();
                            যদি (ltConstp) {
                                DfgConst* ধ্রুৱ constp = makeZero(ltFlp, vtxp->width());
                                constp->অঙ্ক().opConcat(ltConstp->অঙ্ক(), rConstp->অঙ্ক());
                                ঘূৰাই constp;
                            }
                            ঘূৰাই make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* ধ্রুৱ elsep = [&]() -> DfgVertex* {
                            File_সাৰি* ধ্রুৱ leFlp = leVtxp->fileline();
                            যদি (leConstp) {
                                DfgConst* ধ্রুৱ constp = makeZero(leFlp, vtxp->width());
                                constp->অঙ্ক().opConcat(leConstp->অঙ্ক(), rConstp->অঙ্ক());
                                ঘূৰাই constp;
                            }
                            ঘূৰাই make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ঘূৰাই;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ধ্রুৱ uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            ধ্রুৱ uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            ধ্রুৱ uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            তার্কিক hasCrossSink = মিছা;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                যদি (ধ্রুৱ DfgSel* ধ্রুৱ selp = sink.cast<DfgSel>()) {
                    ধ্রুৱ uint32_t selLsb = selp->lsb();
                    ধ্রুৱ uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ঘূৰাই মিছা;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                যদি (ধ্রুৱ DfgVarPacked* ধ্রুৱ varp = sink.cast<DfgVarPacked>()) {
                    যদি (!varp->hasSinks() && !varp->isObserved()) ঘূৰাই মিছা;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ঘূৰাই সঁচা;
            });
            যদি (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    যদি (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    যদি (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* ধ্রুৱ catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ধ্রুৱ sp = নূতন DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* ধ্রুৱ varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(সঁচা);
                    varp->srcp(sp);
                    replace(varp);
                    ঘূৰাই;
                }
            }
        }

        // Convert various forms to REPLICATE
        যদি (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ঘূৰাই;
            }
        }
        যদি (DfgRep* ধ্রুৱ lRepp = lhsp->cast<DfgRep>()) {
            যদি (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ঘূৰাই;
                }
            }
        }
        যদি (DfgRep* ধ্রুৱ rRepp = rhsp->cast<DfgRep>()) {
            যদি (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ঘূৰাই;
                }
            }
        }
        যদি (DfgRep* ধ্রুৱ lRepp = lhsp->cast<DfgRep>()) {
            যদি (DfgRep* ধ্রুৱ rRepp = rhsp->cast<DfgRep>()) {
                যদি (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ঘূৰাই;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            যদি (DfgConcat* ধ্রুৱ rConcatp = rhsp->cast<DfgConcat>()) {
                যদি (DfgRep* ধ্রুৱ rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    যদি (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ধ্রুৱ uint32_t width = lRepp->width() + rlRepp->width();
                            ধ্রুৱ DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* ধ্রুৱ repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ঘূৰাই;
                        }
                    }
                }
            }
        }
    }

    শূন্য visit(DfgDiv* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        যদি (DfgConst* ধ্রুৱ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            যদি (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgDivS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        যদি (DfgConst* ধ্রুৱ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            যদি (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgGt* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgGtS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgGte* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgGteS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgLogAnd* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp) || vtxp->rhsp()->unsafe()) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();

        যদি (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgLogEq* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgLogIf* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp) || vtxp->rhsp()->unsafe()) ঘূৰাই;
    }

    শূন্য visit(DfgLogOr* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp) || vtxp->rhsp()->unsafe()) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();

        যদি (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgLt* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgLtS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgLte* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgLteS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ঘূৰাই;
            }
        }
    }

    শূন্য visit(DfgModDiv* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgModDivS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgMul* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (DfgConst* ধ্রুৱ lConstp = lhsp->cast<DfgConst>()) {
            যদি (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ঘূৰাই;
                }
            }
            যদি (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgMulS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (DfgConst* ধ্রুৱ lConstp = lhsp->cast<DfgConst>()) {
            যদি (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ঘূৰাই;
                }
            }
            যদি (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgEq* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ঘূৰাই;
            }
        }

        যদি (DfgConst* ধ্রুৱ lhsConstp = lhsp->cast<DfgConst>()) {
            যদি (DfgConcat* ধ্রুৱ rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ঘূৰাই;
            }

            যদি (rhsp->dtype() == m_bitDType) {
                যদি (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ঘূৰাই;
                    }
                }

                যদি (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        ঘূৰাই;
                    }
                }
            }
        }
    }

    শূন্য visit(DfgNeq* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ঘূৰাই;
            }
        }

        যদি (DfgConst* ধ্রুৱ lhsConstp = lhsp->cast<DfgConst>()) {
            যদি (DfgConcat* ধ্রুৱ rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ঘূৰাই;
            }

            যদি (rhsp->dtype() == m_bitDType) {
                যদি (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        ঘূৰাই;
                    }
                }

                যদি (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ঘূৰাই;
                    }
                }
            }
        }
    }

    শূন্য visit(DfgPow* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgPowSS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgPowSU* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgPowUS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgRep* ধ্রুৱ vtxp) override {
        DfgVertex* ধ্রুৱ srcp = vtxp->srcp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (DfgConst* ধ্রুৱ sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ধ্রুৱ resultp = makeZero(flp, vtxp->width());
                resultp->অঙ্ক().opRepl(sConstp->অঙ্ক(), vtxp->গুনতি());
                replace(resultp);
                ঘূৰাই;
            }
        }

        যদি (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ঘূৰাই;
            }
        }

        যদি (DfgRep* ধ্রুৱ sRepp = srcp->cast<DfgRep>()) {
            যদি (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgShiftL* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
        যদি (optimizeShiftRHS(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();

        যদি (DfgShiftL* ধ্রুৱ lShiftLp = lhsp->cast<DfgShiftL>()) {
            যদি (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
                    ধ্রুৱ DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ধ্রুৱ bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* ধ্রুৱ cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ধ্রুৱ addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    ঘূৰাই;
                }
            }
        }

        যদি (DfgConst* ধ্রুৱ rConstp = rhsp->cast<DfgConst>()) {
            ধ্রুৱ uint32_t shiftAmount = rConstp->toU32();

            যদি (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ঘূৰাই;
                }
            }

            যদি (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ঘূৰাই;
                }
            }

            যদি (DfgConcat* ধ্রুৱ lConcatp = lhsp->cast<DfgConcat>()) {
                যদি (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* ধ্রুৱ lRhsp = lConcatp->rhsp();
                        DfgVertex* ধ্রুৱ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ধ্রুৱ uint32_t rW = shiftAmount;
                        ধ্রুৱ uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        ধ্রুৱ uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_সাৰি* ধ্রুৱ flp = lConcatp->fileline();
                        DfgVertex* ধ্রুৱ rp = makeZero(flp, shiftAmount);
                        DfgVertex* ধ্রুৱ mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        যদি (!lW) {
                            replace(np);
                            ঘূৰাই;
                        }
                        DfgVertex* ধ্রুৱ lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ঘূৰাই;
                    }
                }
            }

            যদি (DfgSel* ধ্রুৱ lSelp = lhsp->cast<DfgSel>()) {
                যদি (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ধ্রুৱ uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ধ্রুৱ nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        ঘূৰাই;
                    }
                }
            }

            যদি (DfgCond* ধ্রুৱ lCondp = lhsp->cast<DfgCond>()) {
                যদি (!lCondp->hasMultipleSinks()) {
                    APPLYING(ঠেলো_SHIFTL_THROUGH_COND) {
                        DfgShiftL* ধ্রুৱ tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* ধ্রুৱ ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ঘূৰাই;
                    }
                }
            }
        }
    }

    শূন্য visit(DfgShiftR* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
        যদি (optimizeShiftRHS(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();

        যদি (DfgShiftR* ধ্রুৱ lShiftRp = lhsp->cast<DfgShiftR>()) {
            যদি (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_সাৰি* ধ্রুৱ flp = vtxp->fileline();
                    ধ্রুৱ DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ধ্রুৱ bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* ধ্রুৱ cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ধ্রুৱ addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    ঘূৰাই;
                }
            }
        }

        যদি (DfgConst* ধ্রুৱ rConstp = rhsp->cast<DfgConst>()) {
            ধ্রুৱ uint32_t shiftAmount = rConstp->toU32();

            যদি (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ঘূৰাই;
                }
            }

            যদি (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ঘূৰাই;
                }
            }

            যদি (DfgConcat* ধ্রুৱ lConcatp = lhsp->cast<DfgConcat>()) {
                যদি (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* ধ্রুৱ lRhsp = lConcatp->rhsp();
                        DfgVertex* ধ্রুৱ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ধ্রুৱ uint32_t lW = shiftAmount;
                        ধ্রুৱ uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        ধ্রুৱ uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_সাৰি* ধ্রুৱ flp = lConcatp->fileline();
                        DfgVertex* ধ্রুৱ lp = makeZero(flp, shiftAmount);
                        DfgVertex* ধ্রুৱ mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        যদি (!rW) {
                            replace(np);
                            ঘূৰাই;
                        }
                        DfgVertex* ধ্রুৱ rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ঘূৰাই;
                    }
                }
            }

            যদি (DfgSel* ধ্রুৱ lSelp = lhsp->cast<DfgSel>()) {
                যদি (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ধ্রুৱ uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ধ্রুৱ nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        ঘূৰাই;
                    }
                }
            }

            যদি (DfgCond* ধ্রুৱ lCondp = lhsp->cast<DfgCond>()) {
                যদি (!lCondp->hasMultipleSinks()) {
                    APPLYING(ঠেলো_SHIFTR_THROUGH_COND) {
                        DfgShiftR* ধ্রুৱ tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* ধ্রুৱ ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ঘূৰাই;
                    }
                }
            }
        }
    }

    শূন্য visit(DfgShiftRS* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;
        যদি (optimizeShiftRHS(vtxp)) ঘূৰাই;
    }

    শূন্য visit(DfgSub* ধ্রুৱ vtxp) override {
        যদি (দ্বিমিক(vtxp)) ঘূৰাই;

        DfgVertex* ধ্রুৱ lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুৱ rhsp = vtxp->rhsp();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ঘূৰাই;
            }
        }

        যদি (DfgConst* ধ্রুৱ rConstp = rhsp->cast<DfgConst>()) {
            যদি (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ঘূৰাই;
                }
            }
            যদি (vtxp->dtype() == m_bitDType && rConstp->has_মান(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgMatchMasked* ধ্রুৱ vtxp) override {
        যদি (DfgConst* ধ্রুৱ constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ধ্রুৱ matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->অঙ্ক(), matchVarp)));
                ঘূৰাই;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    শূন্য visit(DfgCond* ধ্রুৱ vtxp) override {
        DfgVertex* ধ্রুৱ condp = vtxp->condp();
        DfgVertex* ধ্রুৱ thenp = vtxp->thenp();
        DfgVertex* ধ্রুৱ elsep = vtxp->elsep();
        File_সাৰি* ধ্রুৱ flp = vtxp->fileline();

        যদি (condp->dtype() != m_bitDType) ঘূৰাই;

        যদি (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ঘূৰাই;
            }
        }

        যদি (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ঘূৰাই;
            }
        }

        যদি (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ঘূৰাই;
            }
        }

        যদি (DfgNot* ধ্রুৱ condNotp = condp->cast<DfgNot>()) {
            যদি (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ঘূৰাই;
                }
            }
        }

        যদি (DfgNeq* ধ্রুৱ condNeqp = condp->cast<DfgNeq>()) {
            যদি (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ধ্রুৱ newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ঘূৰাই;
                }
            }
        }

        যদি (DfgNot* ধ্রুৱ thenNotp = thenp->cast<DfgNot>()) {
            যদি (DfgNot* ধ্রুৱ elseNotp = elsep->cast<DfgNot>()) {
                যদি (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ধ্রুৱ newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (DfgOr* ধ্রুৱ condOrp = condp->cast<DfgOr>()) {
            যদি (DfgCond* ধ্রুৱ thenCondp = thenp->cast<DfgCond>()) {
                যদি (!thenCondp->hasMultipleSinks()) {
                    যদি (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ধ্রুৱ ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ঘূৰাই;
                        }
                    }
                    যদি (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ধ্রুৱ ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ঘূৰাই;
                        }
                    }
                }
            }
        }

        যদি (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            যদি (DfgAdd* ধ্রুৱ thenAddp = thenp->cast<DfgAdd>()) {
                যদি (DfgConst* ধ্রুৱ constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    যদি (constp->has_মান(1)) {
                        যদি (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ধ্রুৱ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_সাৰি* ধ্রুৱ thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ঘূৰাই;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            যদি (DfgSub* ধ্রুৱ thenSubp = thenp->cast<DfgSub>()) {
                যদি (DfgConst* ধ্রুৱ constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    যদি (constp->has_মান(1)) {
                        যদি (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ধ্রুৱ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_সাৰি* ধ্রুৱ thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ঘূৰাই;
                            }
                        }
                    }
                }
            }
        }

        যদি (vtxp->dtype() == m_bitDType) {
            যদি (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    ঘূৰাই;
                }
            }
            যদি (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ঘূৰাই;
                }
            }
        }

        যদি (vtxp->width() <= VL_QUADSIZE) {
            যদি (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* ধ্রুৱ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ঘূৰাই;
                }
            }
            যদি (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* ধ্রুৱ maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ঘূৰাই;
                }
            }
            যদি (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* ধ্রুৱ maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ঘূৰাই;
                }
            }
            যদি (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* ধ্রুৱ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ঘূৰাই;
                }
            }

            যদি (DfgOr* ধ্রুৱ tOrp = thenp->cast<DfgOr>()) {
                যদি (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* ধ্রুৱ maskp = replicate(vtxp, condp);
                        DfgAnd* ধ্রুৱ andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ঘূৰাই;
                    }
                }
                যদি (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* ধ্রুৱ maskp = replicate(vtxp, condp);
                        DfgAnd* ধ্রুৱ andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ঘূৰাই;
                    }
                }
            }
        }

        যদি (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ধ্রুৱ std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            যদি (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ধ্রুৱ uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    যদি (rWidth != rhsp->width()) {
                        ধ্রুৱ DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    যদি (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ঘূৰাই;
                    }
                    // Create new LHS
                    ধ্রুৱ uint32_t lWidth = vtxp->width() - rWidth;
                    ধ্রুৱ DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* ধ্রুৱ lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* ধ্রুৱ lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* ধ্রুৱ lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ঘূৰাই;
                }
            }

            ধ্রুৱ std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            যদি (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ধ্রুৱ uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    যদি (lWidth != lhsp->width()) {
                        ধ্রুৱ DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    যদি (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ঘূৰাই;
                    }
                    // Create new RHS
                    ধ্রুৱ uint32_t rWidth = vtxp->width() - lWidth;
                    ধ্রুৱ DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* ধ্রুৱ rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* ধ্রুৱ rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* ধ্রুৱ rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ঘূৰাই;
                }
            }
        }

        যদি (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                যদি (ধ্রুৱ uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ধ্রুৱ zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ঘূৰাই;
            }
        }

        যদি (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                যদি (ধ্রুৱ uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ধ্রুৱ zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ঘূৰাই;
            }
        }

        যদি (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ঘূৰাই;
            }
        }

        যদি (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ঘূৰাই;
            }
        }

        যদি (DfgCond* ধ্রুৱ tCondp = thenp->cast<DfgCond>()) {
            যদি (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ঘূৰাই;
                }
            }
        }

        যদি (DfgCond* ধ্রুৱ eCondp = elsep->cast<DfgCond>()) {
            যদি (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ঘূৰাই;
                }
            }
        }
    }

    শূন্য visit(DfgVertexVar* ধ্রুৱ vtxp) override {
        যদি (vtxp->hasSinks()) ঘূৰাই;
        যদি (vtxp->isObserved()) ঘূৰাই;
        যদি (vtxp->defaultp()) ঘূৰাই;

        // If undriven, or driven from another var, it is completely redundant.
        যদি (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ঘূৰাই;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        তার্কিক foundOne = মিছা;
        ধ্রুৱ তার্কিক keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> তার্কিক {
            // Ignore non-observable variable sinks. These can be eliminated.
            যদি (ধ্রুৱ DfgVertexVar* ধ্রুৱ varp = sink.cast<DfgVertexVar>()) {
                যদি (!varp->hasSinks() && !varp->isObserved()) ঘূৰাই মিছা;
            }
            // Keep if found more than one sink
            যদি (foundOne) ঘূৰাই সঁচা;
            // Mark first sink found
            foundOne = সঁচা;
            ঘূৰাই মিছা;
        });
        যদি (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ঘূৰাই;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        বাবে (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ঠেলো_back(nullptr);
        m_iterList.ঠেলো_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        বাবে (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        বাবে (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        যেতিয়ালৈ (সঁচা) {
            // Process the work list - keep the placeholder at index 0
            যেতিয়ালৈ (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.বেৰ_কৰো_back();
                যদি (!m_vtxp) অনুৱর্ত;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                যদি (DfgVertexVar* ধ্রুৱ varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    অনুৱর্ত;
                }

                // Ast references are not considered
                যদি (m_vtxp->is<DfgVertexAst>()) অনুৱর্ত;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                যদি (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    যদি (DfgVertex* ধ্রুৱ sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            অনুৱর্ত;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            যদি (m_iterList.size() == 1) ভঙ্গ;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                বাবে (size_t hops = 1; hops <= 4; ++hops) {
                    বাবে (size_t i = begin; i < end; ++i) {
                        DfgVertex* ধ্রুৱ vtxp = m_iterList[i];
                        যদি (!vtxp) অনুৱর্ত;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            ঘূৰাই মিছা;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            ঘূৰাই মিছা;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            বাবে (DfgVertex* ধ্রুৱ vtxp : m_iterList) {
                যদি (!vtxp) অনুৱর্ত;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

সার্ৱজনিক:
    স্থির শূন্য apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

শূন্য V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    যদি (!v3Global.opt.fDfgPeephole()) ঘূৰাই;
    V3DfgPeephole::apply(dfg, ctx);
}
