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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgCache.h"
#অন্তর্ভাব "V3DfgDataType.h"
#অন্তর্ভাব "V3DfgPasses.h"
#অন্তর্ভাব "V3DfgPeepholePatterns.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cctype>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
নামাকাশ {

template <typename T_Reduction>
কাঠামো ReductionToBitwiseImpl {};
template <> কাঠামো ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> কাঠামো ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> কাঠামো ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
কাঠামো BitwiseToReductionImpl {};
template <> কাঠামো BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> কাঠামো BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> কাঠামো BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
কাঠামো IsAssociative : সর্বজনীন std::false_type {};
template <> কাঠামো IsAssociative<DfgAnd> : সর্বজনীন std::true_type {};
template <> কাঠামো IsAssociative<DfgOr>  : সর্বজনীন std::true_type {};
template <> কাঠামো IsAssociative<DfgXor> : সর্বজনীন std::true_type {};
template <> কাঠামো IsAssociative<DfgAdd> : সর্বজনীন std::true_type {};
template <> কাঠামো IsAssociative<DfgConcat> : সর্বজনীন std::true_type {};
template <> কাঠামো IsAssociative<DfgMul> : সর্বজনীন std::true_type {};
template <> কাঠামো IsAssociative<DfgMulS> : সর্বজনীন std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
কাঠামো IsCommutative : সর্বজনীন std::false_type {};
template <> কাঠামো IsCommutative<DfgAnd> : সর্বজনীন std::true_type {};
template <> কাঠামো IsCommutative<DfgOr>  : সর্বজনীন std::true_type {};
template <> কাঠামো IsCommutative<DfgXor> : সর্বজনীন std::true_type {};
template <> কাঠামো IsCommutative<DfgAdd> : সর্বজনীন std::true_type {};
template <> কাঠামো IsCommutative<DfgEq> : সর্বজনীন std::true_type {};
template <> কাঠামো IsCommutative<DfgMul> : সর্বজনীন std::true_type {};
template <> কাঠামো IsCommutative<DfgMulS> : সর্বজনীন std::true_type {};
template <> কাঠামো IsCommutative<DfgNeq> : সর্বজনীন std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
কাঠামো IsIdempotent : সর্বজনীন std::false_type {};
template <> কাঠামো IsIdempotent<DfgAnd> : সর্বজনীন std::true_type {};
template <> কাঠামো IsIdempotent<DfgOr>  : সর্বজনীন std::true_type {};

// Binary result dtype
template<typename Vertex>
ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType(ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp);
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgAdd>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgAnd>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgConcat>     (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(lhsp->width() + rhsp->width()); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgDiv>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgDivS>       (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgEq>         (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgGt>         (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgGtS>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgGte>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgGteS>       (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgLogAnd>     (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgLogEq>      (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgLogIf>      (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgLogOr>      (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgLt>         (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgLtS>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgLte>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgLteS>       (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgModDiv>     (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgModDivS>    (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgMul>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgMulS>       (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgNeq>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত Dfg_তথ্য_Type::packed(1); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgOr>         (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgPow>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgPowSS>      (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgPowSU>      (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgPowUS>      (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgShiftL>     (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgShiftR>     (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgShiftRS>    (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgSub>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }
template <> ধ্রুবক Dfg_তথ্য_Type& ফলাফল_DType<DfgXor>        (ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) { ফেরত lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> শূন্য foldOp(V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src);
template <> শূন্য foldOp<Dfg_গুনতি_Ones>  (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.op_গুনতি_Ones(src); }
template <> শূন্য foldOp<DfgExtend>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opAssign(src); }
template <> শূন্য foldOp<DfgExtendS>    (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opExtendS(src, src.width()); }
template <> শূন্য foldOp<DfgLogNot>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opLogNot(src); }
template <> শূন্য foldOp<DfgNegate>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opNegate(src); }
template <> শূন্য foldOp<DfgNot>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opNot(src); }
template <> শূন্য foldOp<DfgOneHot>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opOneHot(src); }
template <> শূন্য foldOp<DfgOneHot0>    (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opOneHot0(src); }
template <> শূন্য foldOp<DfgRedAnd>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opRedAnd(src); }
template <> শূন্য foldOp<DfgRedOr>      (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opRedOr(src); }
template <> শূন্য foldOp<DfgRedXor>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> শূন্য foldOp(V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
template <> শূন্য foldOp<DfgAdd>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opAdd(lhs, rhs); }
template <> শূন্য foldOp<DfgAnd>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opAnd(lhs, rhs); }
template <> শূন্য foldOp<DfgConcat>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opConcat(lhs, rhs); }
template <> শূন্য foldOp<DfgDiv>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opDiv(lhs, rhs); }
template <> শূন্য foldOp<DfgDivS>       (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opDivS(lhs, rhs); }
template <> শূন্য foldOp<DfgEq>         (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opEq(lhs, rhs); }
template <> শূন্য foldOp<DfgGt>         (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opGt(lhs, rhs); }
template <> শূন্য foldOp<DfgGtS>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opGtS(lhs, rhs); }
template <> শূন্য foldOp<DfgGte>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opGte(lhs, rhs); }
template <> শূন্য foldOp<DfgGteS>       (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opGteS(lhs, rhs); }
template <> শূন্য foldOp<DfgLogAnd>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opLogAnd(lhs, rhs); }
template <> শূন্য foldOp<DfgLogEq>      (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opLogEq(lhs, rhs); }
template <> শূন্য foldOp<DfgLogIf>      (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opLogIf(lhs, rhs); }
template <> শূন্য foldOp<DfgLogOr>      (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opLogOr(lhs, rhs); }
template <> শূন্য foldOp<DfgLt>         (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opLt(lhs, rhs); }
template <> শূন্য foldOp<DfgLtS>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opLtS(lhs, rhs); }
template <> শূন্য foldOp<DfgLte>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opLte(lhs, rhs); }
template <> শূন্য foldOp<DfgLteS>       (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opLteS(lhs, rhs); }
template <> শূন্য foldOp<DfgModDiv>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opModDiv(lhs, rhs); }
template <> শূন্য foldOp<DfgModDivS>    (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opModDivS(lhs, rhs); }
template <> শূন্য foldOp<DfgMul>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opMul(lhs, rhs); }
template <> শূন্য foldOp<DfgMulS>       (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opMulS(lhs, rhs); }
template <> শূন্য foldOp<DfgNeq>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opNeq(lhs, rhs); }
template <> শূন্য foldOp<DfgOr>         (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opOr(lhs, rhs); }
template <> শূন্য foldOp<DfgPow>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opPow(lhs, rhs); }
template <> শূন্য foldOp<DfgPowSS>      (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opPowSS(lhs, rhs); }
template <> শূন্য foldOp<DfgPowSU>      (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opPowSU(lhs, rhs); }
template <> শূন্য foldOp<DfgPowUS>      (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opPowUS(lhs, rhs); }
template <> শূন্য foldOp<DfgShiftL>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opShiftL(lhs, rhs); }
template <> শূন্য foldOp<DfgShiftR>     (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opShiftR(lhs, rhs); }
template <> শূন্য foldOp<DfgShiftRS>    (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> শূন্য foldOp<DfgSub>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opSub(lhs, rhs); }
template <> শূন্য foldOp<DfgXor>        (V3_সংখ্যা& out, ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

শ্রেণী V3DfgPeephole final : সর্বজনীন DfgVisitor {
    // TYPES
    কাঠামো VertexInfo final {
        আকার_t m_work_List_সূচক = 0;  // Position of this vertx m_workList (0 means not in list)
        আকার_t m_iter_List_সূচক = 0;  // Position of this vertx m_iterList (0 means not in list)
        আকার_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        আকার_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        যৌক্তিক m_isCachedVertex = মিথ্যা;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ধ্রুবক Dfg_তথ্য_Type& m_bitDType = Dfg_তথ্য_Type::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    আকার_t m_currentGeneration = 0;  // Current generation number
    আকার_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ধ্রুবক m_tmpScopep = v3Global.rootp()->শীর্ষ_Scopep()->scopep();

    // STATIC STATE
    স্থির V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) যদি (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    যৌক্তিক checkApplying(VDfgPeepholePattern id) {
        যদি (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ফেরত মিথ্যা;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_গুনতি[id];
        ফেরত সত্য;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* ধ্রুবক equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        ফেরত equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    শূন্য invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = মিথ্যা;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_সারি* flp, ধ্রুবক Dfg_তথ্য_Type& dtype, Operands... operands) {
        Vertex* ধ্রুবক vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = সত্য;
        ফেরত vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(ধ্রুবক Dfg_তথ্য_Type& dtype, Operands... operands) {
        Vertex* ধ্রুবক vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ফেরত vtxp;
    }

    শূন্য incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    শূন্য addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        যদি (vInfo.m_work_List_সূচক) ফেরত;
        // Add to work list
        vInfo.m_work_List_সূচক = m_workList.আকার_2();
        m_workList.ঠেলো_back(vtxp);
    }

    শূন্য removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_work_List_সূচক] = nullptr;
        vInfo.m_work_List_সূচক = 0;
    }

    শূন্য addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        যদি (vInfo.m_iter_List_সূচক) ফেরত;
        // Add to iter list
        vInfo.m_iter_List_সূচক = m_iterList.আকার_2();
        m_iterList.ঠেলো_back(vtxp);
    }

    শূন্য removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iter_List_সূচক] = nullptr;
        vInfo.m_iter_List_সূচক = 0;
    }

    শূন্য deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_work_List_সূচক, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ধ্রুবক DfgVertexVar* ধ্রুবক varp = vtxp->cast<DfgVertexVar>();
        UASSERT_OBJ(!varp || !varp->has_পূর্ব(), vtxp, "Deleting variable consumed via DfgPrev");

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
                ফেরত মিথ্যা;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            যদি (vInfo.m_generation != m_currentGeneration) srcps.ঠেলো_back(&src);
            vInfo.m_generation = m_currentGeneration;
            ফেরত মিথ্যা;
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
        ধ্রুবক auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  ফেরত srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        জন্য (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        জন্য (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    শূন্য replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_work_List_সূচক, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ধ্রুবক auto debugCallback = [&]() -> শূন্য {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            ধ্রুবক auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ধ্রুবক DfgVertex& v) {  //
                ফেরত cone->গুনতি(&v);
            });
        };
        যদি (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ফেরত;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            ফেরত মিথ্যা;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            ফেরত মিথ্যা;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            ফেরত মিথ্যা;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            ফেরত মিথ্যা;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_সারি* flp, uint32_t val) { ফেরত নতুন DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_সারি* flp, uint32_t width) {
        ফেরত নতুন DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_সারি* flp, ধ্রুবক Dfg_তথ্য_Type& dtype) {
        ফেরত makeZero(flp, dtype.আকার_2());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_সারি* flp, uint32_t width) {
        DfgConst* ধ্রুবক resp = makeZero(flp, width);
        resp->অঙ্ক().setAllBits1();
        ফেরত resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_সারি* flp, ধ্রুবক Dfg_তথ্য_Type& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* ধ্রুবক vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        যদি (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        যদি (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        ফেরত vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(ধ্রুবক DfgVertex* examplep, Operands... operands) {
        ফেরত make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        যদি (vtxp->dtype() == m_bitDType) ফেরত bitp;
        ফেরত make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    স্থির যৌক্তিক isSame(ধ্রুবক DfgVertex* ap, ধ্রুবক DfgVertex* bp) {
        যদি (ap == bp) ফেরত সত্য;
        যদি (ap->dtype() != bp->dtype()) ফেরত মিথ্যা;
        ধ্রুবক DfgConst* ধ্রুবক aConstp = ap->cast<DfgConst>();
        যদি (!aConstp) ফেরত মিথ্যা;
        ধ্রুবক DfgConst* ধ্রুবক bConstp = bp->cast<DfgConst>();
        যদি (!bConstp) ফেরত মিথ্যা;
        ফেরত aConstp->অঙ্ক().isCaseEq(bConstp->অঙ্ক());
    }

    স্থির যৌক্তিক isZero(ধ্রুবক DfgVertex* vtxp) {
        যদি (ধ্রুবক DfgConst* ধ্রুবক constp = vtxp->cast<DfgConst>()) ফেরত constp->isZero();
        ফেরত মিথ্যা;
    }

    স্থির যৌক্তিক isOnes(ধ্রুবক DfgVertex* vtxp) {
        যদি (ধ্রুবক DfgConst* ধ্রুবক constp = vtxp->cast<DfgConst>()) ফেরত constp->isOnes();
        ফেরত মিথ্যা;
    }

    স্থির যৌক্তিক isEqOne(ধ্রুবক DfgVertex* vtxp) {
        যদি (ধ্রুবক DfgConst* ধ্রুবক constp = vtxp->cast<DfgConst>()) ফেরত constp->অঙ্ক().isEqOne();
        ফেরত মিথ্যা;
    }

    স্থির যৌক্তিক areAdjacent(uint32_t& lsb, ধ্রুবক DfgSel* lSelp, ধ্রুবক DfgSel* rSelp) {
        যদি (!isSame(lSelp->srcp(), rSelp->srcp())) ফেরত মিথ্যা;
        যদি (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ফেরত সত্য;
        }
        যদি (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            ফেরত সত্য;
        }
        ফেরত মিথ্যা;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক foldUnary(Vertex* ধ্রুবক vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::মান, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::মান, "Must invoke on final class");
        যদি (DfgConst* ধ্রুবক srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ধ্রুবক resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->অঙ্ক(), srcp->অঙ্ক());
                replace(resultp);
                ফেরত সত্য;
            }
        }
        ফেরত মিথ্যা;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক দ্বিমিক(Vertex* ধ্রুবক vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_দ্বিমিক, Vertex>::মান, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::মান, "Must invoke on final class");

        File_সারি* ধ্রুবক flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ধ্রুবক lConstp = lhsp->cast<DfgConst>();
        DfgConst* ধ্রুবক rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* ধ্রুবক lSamep = lhsp->cast<Vertex>();
        Vertex* ধ্রুবক rSamep = rhsp->cast<Vertex>();

        // Constant folding
        যদি (lConstp && rConstp) {
            APPLYING(FOLD_দ্বিমিক) {
                DfgConst* ধ্রুবক resultp = makeZero(flp, ফলাফল_DType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->অঙ্ক(), lConstp->অঙ্ক(), rConstp->অঙ্ক());
                replace(resultp);
                ফেরত সত্য;
            }
        }

        // Idempotent
        যদি VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::মান) {
            যদি (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_দ্বিমিক) {
                    replace(lhsp);
                    ফেরত সত্য;
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
                        ফেরত সত্য;
                    }
                }
                যদি (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_দ্বিমিক_RHS) {
                        replace(rSamep);
                        ফেরত সত্য;
                    }
                }
            }

            যদি (lConstp && rSamep) {
                যদি (DfgConst* ধ্রুবক rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_দ্বিমিক_LHS_OF_RHS) {
                        DfgConst* ধ্রুবক cp = makeZero(flp, ফলাফল_DType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->অঙ্ক(), lConstp->অঙ্ক(), rlConstp->অঙ্ক());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        ফেরত সত্য;
                    }
                }
            }

            যদি (lSamep && rConstp) {
                যদি (DfgConst* ধ্রুবক lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_দ্বিমিক_RHS_OF_LHS) {
                        DfgConst* ধ্রুবক cp = makeZero(flp, ফলাফল_DType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->অঙ্ক(), lrConstp->অঙ্ক(), rConstp->অঙ্ক());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        ফেরত সত্য;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            যদি (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* ধ্রুবক ap = lSamep->lhsp();
                DfgVertex* ধ্রুবক bp = lSamep->rhsp();
                DfgVertex* ধ্রুবক cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                যদি (ap != cp && bp != cp) {
                    APPLYING(ডান_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ধ্রুবক Dfg_তথ্য_Type& childDType = ফলাফল_DType<Vertex>(bp, cp);
                        Vertex* ধ্রুবক bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ফেরত সত্য;
                    }
                }
            }

            যদি (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* ধ্রুবক rlVtxp = rSamep->lhsp();
                DfgVertex* ধ্রুবক rrVtxp = rSamep->rhsp();

                যদি VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::মান) {
                    যদি (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            ফেরত সত্য;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                যদি (Vertex* ধ্রুবক existingp
                    = getVertex<Vertex>(ফলাফল_DType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    যদি (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_দ্বিমিক_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            ফেরত সত্য;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                যদি VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::মান) {
                    যদি (Vertex* ধ্রুবক existingp
                        = getVertex<Vertex>(ফলাফল_DType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        যদি (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_দ্বিমিক_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                ফেরত সত্য;
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
                যদি (DfgCond* ধ্রুবক rCondp = rhsp->cast<DfgCond>()) {
                    যদি (!rCondp->hasMultipleSinks()) {
                        APPLYING(ঠেলো_COMMUTATIVE_দ্বিমিক_THROUGH_COND) {
                            DfgVertex* ধ্রুবক tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* ধ্রুবক ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ফেরত সত্য;
                        }
                    }
                }
                ফেরত মিথ্যা;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ধ্রুবক যৌক্তিক swapSides = [&]() {
            // Don't swap if not commutative, duh
            যদি VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::মান) ফেরত মিথ্যা;
            // Const go on left
            যদি (lConstp) ফেরত মিথ্যা;
            যদি (rConstp) ফেরত সত্য;
            // Not go on left
            যদি (lhsp->is<DfgNot>()) ফেরত মিথ্যা;
            যদি (rhsp->is<DfgNot>()) ফেরত সত্য;
            // Same go on right
            যদি (rSamep) ফেরত মিথ্যা;
            যদি (lSamep) ফেরত সত্য;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ধ্রুবক VertexInfo& lInfo = m_vInfo[lhsp];
            ধ্রুবক VertexInfo& rInfo = m_vInfo[rhsp];
            ফেরত lInfo.m_id > rInfo.m_id;
        }();
        যদি (swapSides) {
            APPLYING(SWAP_SIDES_IN_দ্বিমিক) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                ফেরত সত্য;
            }
        }

        ফেরত মিথ্যা;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক distributive_And_Associative_দ্বিমিক(Vertex* vtxp) {
        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        যদি (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            যদি (Other* ধ্রুবক lp = lhsp->cast<Other>()) {
                যদি (Other* ধ্রুবক rp = rhsp->cast<Other>()) {
                    DfgVertex* ধ্রুবক llp = lp->lhsp();
                    DfgVertex* ধ্রুবক lrp = lp->rhsp();
                    DfgVertex* ধ্রুবক rlp = rp->lhsp();
                    DfgVertex* ধ্রুবক rrp = rp->rhsp();
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
                            ফেরত সত্য;
                        }
                    }
                }
            }
        }

        ফেরত মিথ্যা;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক
    try_ঠেলো_Bitwise_Op_Through_Concat(Vertex* ধ্রুবক vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ধ্রুবক catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ধ্রুবক catRConstp = concatp->rhsp()->cast<DfgConst>();
        যদি (catLConstp || catRConstp) {
            APPLYING(ঠেলো_BITWISE_OP_THROUGH_CONCAT) {
                ধ্রুবক uint32_t width = concatp->width();
                ধ্রুবক Dfg_তথ্য_Type& lDtype = concatp->lhsp()->dtype();
                ধ্রুবক Dfg_তথ্য_Type& rDtype = concatp->rhsp()->dtype();
                ধ্রুবক uint32_t lWidth = lDtype.আকার_2();
                ধ্রুবক uint32_t rWidth = rDtype.আকার_2();

                // The new Lhs vertex
                DfgVertex* ধ্রুবক newLhsp = [&]() -> DfgVertex* {
                    DfgConst* ধ্রুবক newLhsConstp = makeZero(constp->fileline(), lWidth);
                    যদি (catLConstp) {
                        V3_সংখ্যা অঙ্ক{constp->fileline(), static_cast<পূর্ণসংখ্যা>(lWidth), 0u};
                        অঙ্ক.opSel(constp->অঙ্ক(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->অঙ্ক(), অঙ্ক, catLConstp->অঙ্ক());
                        ফেরত newLhsConstp;
                    }
                    newLhsConstp->অঙ্ক().opSel(constp->অঙ্ক(), width - 1, rWidth);
                    ফেরত make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* ধ্রুবক newRhsp = [&]() -> DfgVertex* {
                    DfgConst* ধ্রুবক newRhsConstp = makeZero(constp->fileline(), rWidth);
                    যদি (catRConstp) {
                        V3_সংখ্যা অঙ্ক{constp->fileline(), static_cast<পূর্ণসংখ্যা>(rWidth), 0u};
                        অঙ্ক.opSel(constp->অঙ্ক(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->অঙ্ক(), অঙ্ক, catRConstp->অঙ্ক());
                        ফেরত newRhsConstp;
                    }
                    newRhsConstp->অঙ্ক().opSel(constp->অঙ্ক(), rWidth - 1, 0);
                    ফেরত make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ফেরত সত্য;
            }
        }
        ফেরত মিথ্যা;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক
    try_ঠেলো_Compare_Op_Through_Concat(Vertex* ধ্রুবক vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        যদি (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ঠেলো_COMPARE_OP_THROUGH_CONCAT) {
                ধ্রুবক uint32_t width = concatp->width();
                ধ্রুবক uint32_t lWidth = concatp->lhsp()->width();
                ধ্রুবক uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* ধ্রুবক newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->অঙ্ক().opSel(constp->অঙ্ক(), width - 1, rWidth);
                Vertex* ধ্রুবক newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ধ্রুবক newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->অঙ্ক().opSel(constp->অঙ্ক(), rWidth - 1, 0);
                Vertex* ধ্রুবক newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                যদি VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::মান) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } অন্যথা যদি VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::মান) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } অন্যথা {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ফেরত সত্য;
            }
        }
        ফেরত মিথ্যা;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক try_ঠেলো_Bitwise_Op_Through_Reductions(Bitwise* ধ্রুবক vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        যদি (Reduction* ধ্রুবক lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            যদি (Reduction* ধ্রুবক rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* ধ্রুবক lSrcp = lRedp->srcp();
                DfgVertex* ধ্রুবক rSrcp = rRedp->srcp();
                যদি (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(ঠেলো_BITWISE_THROUGH_REDUCTION) {
                        File_সারি* ধ্রুবক flp = vtxp->fileline();
                        Bitwise* ধ্রুবক bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        ফেরত সত্য;
                    }
                }
            }
        }

        ফেরত মিথ্যা;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    স্থির std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        যদি (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ফেরত {nullptr, nullptr};
        }
        যদি (vtxp->hasMultipleSinks()) ফেরত {nullptr, nullptr};
        Dfg_Vertex_দ্বিমিক* ধ্রুবক bwp = vtxp->as<Dfg_Vertex_দ্বিমিক>();
        DfgSel* ধ্রুবক lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ধ্রুবক rSelp = bwp->rhsp()->cast<DfgSel>();
        যদি (!lSelp || !rSelp) ফেরত {nullptr, nullptr};
        DfgSel* ধ্রুবক nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* ধ্রুবক wSelp = nSelp == lSelp ? rSelp : lSelp;
        ধ্রুবক uint32_t nWidth = nSelp->fromp()->width();
        যদি (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ফেরত {nullptr, nullptr};
        যদি (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            ফেরত {nullptr, nullptr};
        }
        ফেরত {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক try_ঠেলো_Bitwise_Op_Throug_Sel(Bitwise* ধ্রুবক vtxp) {
        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();

        যদি (DfgSel* ধ্রুবক lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            যদি (DfgSel* ধ্রুবক selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } অন্যথা যদি (Bitwise* ধ্রুবক bitwisep = rhsp->cast<Bitwise>()) {
                যদি (DfgSel* ধ্রুবক rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } অন্যথা যদি (DfgSel* ধ্রুবক rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            যদি (rSelp) {
                DfgVertex* ধ্রুবক lFromp = lSelp->fromp();
                DfgVertex* ধ্রুবক rFromp = rSelp->fromp();
                যদি (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ঠেলো_BITWISE_THROUGH_SEL) {
                        Bitwise* ধ্রুবক bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        যদি (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ফেরত সত্য;
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
            Bitwise* ধ্রুবক rChainp = rhsp->cast<Bitwise>();
            যদি (rChainp && rChainp->hasMultipleSinks()) ফেরত মিথ্যা;
            DfgVertex* ধ্রুবক rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ধ্রুবক auto lSels = pushableUnalignedSels(lhsp);
            ধ্রুবক auto rSels = pushableUnalignedSels(rTermp);
            যদি (!lSels.first && !rSels.first) ফেরত মিথ্যা;

            ধ্রুবক DfgVertex* ধ্রুবক opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            যদি (opp->is<Bitwise>()) ফেরত মিথ্যা;
            ধ্রুবক auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* ধ্রুবক nFromp = sels.first->fromp();
            DfgVertex* ধ্রুবক wFromp = sels.second->fromp();
            ধ্রুবক uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            ধ্রুবক auto matches = [&](ধ্রুবক DfgVertex* termp, ধ্রুবক auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                যদি (tSels.first) {
                    ফেরত termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                যদি (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                যদি (ধ্রুবক DfgSel* ধ্রুবক selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                যদি (termp->type() != opp->type()) ফেরত মিথ্যা;
                // Operands might have been swapped since
                ধ্রুবক Dfg_Vertex_দ্বিমিক* ধ্রুবক bwp = termp->as<Dfg_Vertex_দ্বিমিক>();
                ধ্রুবক যৌক্তিক nIsLhs = bwp->lhsp() == nFromp;
                যদি (!nIsLhs && bwp->rhsp() != nFromp) ফেরত মিথ্যা;
                ধ্রুবক DfgSel* ধ্রুবক slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ফেরত slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            যদি (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ফেরত মিথ্যা;

            APPLYING(ঠেলো_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_সারি* ধ্রুবক flp = vtxp->fileline();
                ধ্রুবক Dfg_তথ্য_Type& dtype = nFromp->dtype();
                DfgSel* ধ্রুবক slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                যদি (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } অন্যথা যদি (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } অন্যথা {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ধ্রুবক auto ঠেলো = [&](DfgVertex* termp, ধ্রুবক DfgSel* nSelp) -> DfgVertex* {
                    ফেরত nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* ধ্রুবক newLhsp = ঠেলো(lhsp, lSels.first);
                DfgVertex* ধ্রুবক newRTermp = ঠেলো(rTermp, rSels.first);
                যদি (rChainp) {
                    DfgVertex* ধ্রুবক newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } অন্যথা {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ফেরত সত্য;
            }
        }

        ফেরত মিথ্যা;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    স্থির যৌক্তিক isConcatFoldable(ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        যদি (isSame(lhsp, rhsp)) ফেরত সত্য;
        যদি (ধ্রুবক DfgRep* ধ্রুবক lRepp = lhsp->cast<DfgRep>()) {
            যদি (isSame(lRepp->srcp(), rhsp)) ফেরত সত্য;
        }
        যদি (ধ্রুবক DfgRep* ধ্রুবক rRepp = rhsp->cast<DfgRep>()) {
            যদি (isSame(lhsp, rRepp->srcp())) ফেরত সত্য;
        }
        // Concatenation of adjoining selects is a select
        যদি (ধ্রুবক DfgSel* ধ্রুবক lSelp = lhsp->cast<DfgSel>()) {
            যদি (ধ্রুবক DfgSel* ধ্রুবক rSelp = rhsp->cast<DfgSel>()) {
                ফেরত isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        যদি (ধ্রুবক DfgNot* ধ্রুবক lNotp = lhsp->cast<DfgNot>()) {
            যদি (ধ্রুবক DfgNot* ধ্রুবক rNotp = rhsp->cast<DfgNot>()) {
                ফেরত !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ফেরত মিথ্যা;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    স্থির যৌক্তিক isSameBitwise(ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) {
        যদি (lhsp->type() != rhsp->type()) ফেরত মিথ্যা;
        ফেরত lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    স্থির যৌক্তিক isConcatSimplifiable(ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp) {
        যদি (isConcatFoldable(lhsp, rhsp)) ফেরত সত্য;
        যদি (!isSameBitwise(lhsp, rhsp)) ফেরত মিথ্যা;
        যদি (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ফেরত মিথ্যা;
        ধ্রুবক Dfg_Vertex_দ্বিমিক* ধ্রুবক lp = lhsp->as<Dfg_Vertex_দ্বিমিক>();
        ধ্রুবক Dfg_Vertex_দ্বিমিক* ধ্রুবক rp = rhsp->as<Dfg_Vertex_দ্বিমিক>();
        যদি (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ফেরত সত্য;
        যদি (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ফেরত সত্য;
        ফেরত মিথ্যা;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    স্থির যৌক্তিক isConcatPushableThroughBitwise(ধ্রুবক DfgVertex* lhsp, ধ্রুবক DfgVertex* rhsp,
                                               যৌক্তিক& swap) {
        যদি (!isSameBitwise(lhsp, rhsp)) ফেরত মিথ্যা;
        যদি (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ফেরত মিথ্যা;
        ধ্রুবক Dfg_Vertex_দ্বিমিক* ধ্রুবক lp = lhsp->as<Dfg_Vertex_দ্বিমিক>();
        ধ্রুবক Dfg_Vertex_দ্বিমিক* ধ্রুবক rp = rhsp->as<Dfg_Vertex_দ্বিমিক>();
        swap = মিথ্যা;
        যদি (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ফেরত সত্য;
        যদি (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ফেরত সত্য;
        swap = সত্য;
        যদি (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ফেরত সত্য;
        যদি (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ফেরত সত্য;
        ফেরত মিথ্যা;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_সারি* flp, ধ্রুবক DfgVertex* lhsp,
                                              ধ্রুবক DfgVertex* rhsp, যৌক্তিক swap) {
        ধ্রুবক Dfg_Vertex_দ্বিমিক* ধ্রুবক lp = lhsp->as<Dfg_Vertex_দ্বিমিক>();
        ধ্রুবক Dfg_Vertex_দ্বিমিক* ধ্রুবক rp = rhsp->as<Dfg_Vertex_দ্বিমিক>();
        ধ্রুবক Dfg_তথ্য_Type& dtype = Dfg_তথ্য_Type::packed(lp->width() + rp->width());
        DfgVertex* ধ্রুবক cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* ধ্রুবক dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* ধ্রুবক newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ধ্রুবক newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        যদি (lp->is<DfgAnd>()) ফেরত make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        যদি (lp->is<DfgOr>()) ফেরত make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ফেরত make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();

        যদি (DfgSel* ধ্রুবক lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            যদি (DfgSel* ধ্রুবক selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } অন্যথা যদি (Bitwise* ধ্রুবক rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } অন্যথা যদি (Reduction* ধ্রুবক rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            যদি (rSelp) {
                uint32_t lsb = 0;
                যদি (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        ধ্রুবক Dfg_তথ্য_Type& dtype
                            = Dfg_তথ্য_Type::packed(lSelp->width() + rSelp->width());
                        DfgSel* ধ্রুবক newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        যদি (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ফেরত সত্য;
                    }
                }
            }
        }

        যদি (Reduction* ধ্রুবক lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            যদি (!rRedp) {
                যদি (Bitwise* ধ্রুবক rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            যদি (rRedp) {
                যদি (DfgSel* ধ্রুবক lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    যদি (DfgSel* ধ্রুবক rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        যদি (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                ধ্রুবক Dfg_তথ্য_Type& dtype
                                    = Dfg_তথ্য_Type::packed(lSelp->width() + rSelp->width());
                                DfgSel* ধ্রুবক newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                যদি (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ফেরত সত্য;
                            }
                        }
                    }
                }
            }
        }

        ফেরত মিথ্যা;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক optimizeReduction(Reduction* ধ্রুবক vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        যদি (foldUnary(vtxp)) ফেরত সত্য;

        DfgVertex* ধ্রুবক srcp = vtxp->srcp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        // Reduction of 1-bit value
        যদি (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                ফেরত সত্য;
            }
        }

        যদি (DfgCond* ধ্রুবক condp = srcp->cast<DfgCond>()) {
            যদি (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ঠেলো_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ধ্রুবক newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ধ্রুবক newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ধ্রুবক newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ফেরত সত্য;
                }
            }
        }

        যদি (DfgConcat* ধ্রুবক concatp = srcp->cast<DfgConcat>()) {
            যদি (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(ঠেলো_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ধ্রুবক lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* ধ্রুবক rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    ফেরত সত্য;
                }
            }
        }

        যদি (Bitwise* ধ্রুবক bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            যদি (!bitwisep->hasMultipleSinks()) {
                যদি (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(ঠেলো_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* ধ্রুবক newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* ধ্রুবক newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        ফেরত সত্য;
                    }
                }

                যদি (DfgSel* ধ্রুবক lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    যদি (DfgSel* ধ্রুবক rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        যদি (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ঠেলো_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                ধ্রুবক Dfg_তথ্য_Type& dtype
                                    = Dfg_তথ্য_Type::packed(lSelp->width() + rSelp->width());
                                DfgSel* ধ্রুবক newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ফেরত সত্য;
                            }
                        }
                    }
                }
            }
        }

        ফেরত মিথ্যা;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_ফলাফল যৌক্তিক optimizeShiftRHS(Shift* ধ্রুবক vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_দ্বিমিক, Shift>::মান, "Must invoke on binary");
        static_assert(std::is_final<Shift>::মান, "Must invoke on final class");
        যদি (ধ্রুবক DfgConcat* ধ্রুবক concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            যদি (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ফেরত সত্য;
                }
            }
        }
        ফেরত মিথ্যা;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_গুনতি_Ones_Term(DfgVertex* vtxp) {
        যদি (DfgConcat* ধ্রুবক oCatp = vtxp->cast<DfgConcat>()) {
            যদি (isZero(oCatp->lhsp())) {
                যদি (Dfg_গুনতি_Ones* ধ্রুবক গুনতি_Onesp = oCatp->rhsp()->cast<Dfg_গুনতি_Ones>()) {
                    // Zero extended count ones
                    যদি (DfgSel* ধ্রুবক selp = গুনতি_Onesp->srcp()->cast<DfgSel>()) {
                        ফেরত {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } অন্যথা যদি (DfgSel* ধ্রুবক selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    যদি (selp->dtype() == m_bitDType) {  //
                        ফেরত {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            ফেরত {nullptr, 0, 0};
        }
        যদি (Dfg_গুনতি_Ones* ধ্রুবক গুনতি_Onesp = vtxp->cast<Dfg_গুনতি_Ones>()) {
            // Simple count ones
            যদি (DfgSel* ধ্রুবক selp = গুনতি_Onesp->srcp()->cast<DfgSel>()) {
                ফেরত {selp->fromp(), selp->lsb(), selp->width()};
            }
            ফেরত {nullptr, 0, 0};
        }
        যদি (DfgSel* ধ্রুবক oSelp = vtxp->cast<DfgSel>()) {
            যদি (oSelp->lsb() == 0) {
                // Truncated count ones
                যদি (Dfg_গুনতি_Ones* ধ্রুবক গুনতি_Onesp = oSelp->fromp()->cast<Dfg_গুনতি_Ones>()) {
                    // Zero extended count ones
                    যদি (DfgSel* ধ্রুবক selp = গুনতি_Onesp->srcp()->cast<DfgSel>()) {
                        ফেরত {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            যদি (oSelp->dtype() == m_bitDType) {  //
                ফেরত {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ফেরত {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        যদি (DfgShiftR* ধ্রুবক shiftrp = vtxp->cast<DfgShiftR>()) {
            যদি (DfgConst* ধ্রুবক rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                যদি (rConstp->toU32() == shiftrp->width() - 1) {
                    ফেরত {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            ফেরত {nullptr, 0, 0};
        }
        // Not applicable
        ফেরত {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              ধ্রুবক uint32_t width) {
        যখন (সত্য) {
            ধ্রুবক uint32_t msb = lsb + width - 1;

            // Sel from Concat
            যদি (DfgConcat* ধ্রুবক concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* ধ্রুবক lhsp = concatp->lhsp();
                DfgVertex* ধ্রুবক rhsp = concatp->rhsp();

                যদি (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        চালু;
                    }
                } অন্যথা যদি (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        চালু;
                    }
                }
            }

            যদি (DfgRep* ধ্রুবক repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ধ্রুবক uint32_t srcWidth = repp->srcp()->width();
                যদি (width <= srcWidth) {
                    ধ্রুবক uint32_t newLsb = lsb % srcWidth;
                    ধ্রুবক uint32_t newMsb = newLsb + width - 1;
                    যদি (newMsb < srcWidth) {
                        APPLYING(ঠেলো_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            চালু;
                        }
                    }
                }
            }

            // Sel from Sel
            যদি (DfgSel* ধ্রুবক selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    চালু;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            যদি (DfgVarPacked* ধ্রুবক varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ধ্রুবক auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* ধ্রুবক driverp = pair.first;
                ধ্রুবক uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                যদি (driverp) {
                    যদি (driverp == varp->defaultp()) {
                        APPLYING(ঠেলো_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            চালু;
                        }
                    } অন্যথা {
                        APPLYING(ঠেলো_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            চালু;
                        }
                    }
                }
            }

            // No patterns matched, stop
            বিরতি;
        }
        ফেরত {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        যদি (ap == bp) ফেরত {ap, ap->width()};

        // If both constants, check LSBs
        যদি (DfgConst* ধ্রুবক aConstp = ap->cast<DfgConst>()) {
            যদি (DfgConst* ধ্রুবক bConstp = bp->cast<DfgConst>()) {
                ধ্রুবক V3_সংখ্যা& a_অঙ্ক = aConstp->অঙ্ক();
                ধ্রুবক V3_সংখ্যা& b_অঙ্ক = bConstp->অঙ্ক();
                // Max match is the shorter constant
                ধ্রুবক uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                জন্য (; matchWidth < maxMatch; ++matchWidth) {
                    যদি (a_অঙ্ক.bitIs0(matchWidth) != b_অঙ্ক.bitIs0(matchWidth)) বিরতি;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ধ্রুবক shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                ফেরত {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        যদি (DfgConcat* ধ্রুবক catp = ap->cast<DfgConcat>()) ফেরত commonLSBs(catp->rhsp(), bp);
        যদি (DfgConcat* ধ্রুবক catp = bp->cast<DfgConcat>()) ফেরত commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        যদি (DfgSel* ধ্রুবক selp = ap->cast<DfgSel>()) {
            যদি (selp->lsb() == 0) {
                DfgVertex* ধ্রুবক fromp = selp->fromp();
                ধ্রুবক std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                ফেরত {common.first, std::min(common.second, selp->width())};
            }
        }
        যদি (DfgSel* ধ্রুবক selp = bp->cast<DfgSel>()) {
            যদি (selp->lsb() == 0) {
                DfgVertex* ধ্রুবক fromp = selp->fromp();
                ধ্রুবক std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                ফেরত {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        ফেরত {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        যদি (ap == bp) ফেরত {ap, ap->width()};

        // If both constants, check MSBs
        যদি (DfgConst* ধ্রুবক aConstp = ap->cast<DfgConst>()) {
            যদি (DfgConst* ধ্রুবক bConstp = bp->cast<DfgConst>()) {
                ধ্রুবক uint32_t aMsb = aConstp->width() - 1;
                ধ্রুবক uint32_t bMsb = bConstp->width() - 1;
                ধ্রুবক V3_সংখ্যা& a_অঙ্ক = aConstp->অঙ্ক();
                ধ্রুবক V3_সংখ্যা& b_অঙ্ক = bConstp->অঙ্ক();
                // Max match is the shorter constant
                ধ্রুবক uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                জন্য (; matchWidth < maxMatch; ++matchWidth) {
                    যদি (a_অঙ্ক.bitIs0(aMsb - matchWidth) != b_অঙ্ক.bitIs0(bMsb - matchWidth)) বিরতি;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ধ্রুবক shorterp = aMsb < bMsb ? aConstp : bConstp;
                ফেরত {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        যদি (DfgConcat* ধ্রুবক catp = ap->cast<DfgConcat>()) ফেরত commonMSBs(catp->lhsp(), bp);
        যদি (DfgConcat* ধ্রুবক catp = bp->cast<DfgConcat>()) ফেরত commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        যদি (DfgSel* ধ্রুবক selp = ap->cast<DfgSel>()) {
            DfgVertex* ধ্রুবক fromp = selp->fromp();
            যদি (selp->msb() == fromp->width() - 1) {
                ধ্রুবক std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                ফেরত {common.first, std::min(common.second, selp->width())};
            }
        }
        যদি (DfgSel* ধ্রুবক selp = bp->cast<DfgSel>()) {
            DfgVertex* ধ্রুবক fromp = selp->fromp();
            যদি (selp->msb() == fromp->width() - 1) {
                ধ্রুবক std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                ফেরত {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        ফেরত {nullptr, 0};
    }

    // VISIT methods

    শূন্য visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    শূন্য visit(Dfg_গুনতি_Ones* ধ্রুবক vtxp) override {
        যদি (foldUnary(vtxp)) ফেরত;
    }

    শূন্য visit(DfgExtend* ধ্রুবক vtxp) override {
        যদি (foldUnary(vtxp)) ফেরত;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* ধ্রুবক zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ফেরত;
        }
    }

    শূন্য visit(DfgExtendS* ধ্রুবক vtxp) override {
        যদি (foldUnary(vtxp)) ফেরত;
    }

    শূন্য visit(DfgLogNot* ধ্রুবক vtxp) override {
        যদি (foldUnary(vtxp)) ফেরত;
    }

    শূন্য visit(DfgNegate* ধ্রুবক vtxp) override {
        যদি (foldUnary(vtxp)) ফেরত;
    }

    শূন্য visit(DfgNot* ধ্রুবক vtxp) override {
        যদি (foldUnary(vtxp)) ফেরত;

        // Not of Cond
        যদি (DfgCond* ধ্রুবক condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            যদি (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ঠেলো_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ধ্রুবক newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ধ্রুবক newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ধ্রুবক newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ফেরত;
                }
            }
        }

        // Not of Not
        যদি (DfgNot* ধ্রুবক notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ফেরত;
            }
        }

        যদি (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            যদি (DfgEq* ধ্রুবক eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ফেরত;
                }
            }

            // Not of Neq
            যদি (DfgNeq* ধ্রুবক neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ফেরত;
                }
            }

            // Not of unsigned comparisons
            যদি (DfgLt* ধ্রুবক p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ফেরত;
                }
            }
            যদি (DfgGte* ধ্রুবক p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ফেরত;
                }
            }
            যদি (DfgGt* ধ্রুবক p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ফেরত;
                }
            }
            যদি (DfgLte* ধ্রুবক p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ফেরত;
                }
            }

            // Not of signed comparisons
            যদি (DfgLtS* ধ্রুবক p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ফেরত;
                }
            }
            যদি (DfgGteS* ধ্রুবক p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ফেরত;
                }
            }
            যদি (DfgGtS* ধ্রুবক p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ফেরত;
                }
            }
            যদি (DfgLteS* ধ্রুবক p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgOneHot* ধ্রুবক vtxp) override {
        যদি (foldUnary(vtxp)) ফেরত;
    }

    শূন্য visit(DfgOneHot0* ধ্রুবক vtxp) override {
        যদি (foldUnary(vtxp)) ফেরত;
    }

    শূন্য visit(DfgRedOr* ধ্রুবক vtxp) override {
        যদি (optimizeReduction(vtxp)) ফেরত;
    }

    শূন্য visit(DfgRedAnd* ধ্রুবক vtxp) override {
        যদি (optimizeReduction(vtxp)) ফেরত;
    }

    শূন্য visit(DfgRedXor* ধ্রুবক vtxp) override {
        যদি (optimizeReduction(vtxp)) ফেরত;
    }

    শূন্য visit(DfgSel* ধ্রুবক vtxp) override {
        DfgVertex* ধ্রুবক fromp = vtxp->fromp();

        File_সারি* ধ্রুবক flp = vtxp->fileline();

        ধ্রুবক uint32_t lsb = vtxp->lsb();
        ধ্রুবক uint32_t width = vtxp->width();
        ধ্রুবক uint32_t msb = lsb + width - 1;

        যদি (DfgConst* ধ্রুবক constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ধ্রুবক resp = makeZero(flp, width);
                resp->অঙ্ক().opSel(constp->অঙ্ক(), msb, lsb);
                replace(resp);
                ফেরত;
            }
        }

        // Full width select, replace with the source.
        যদি (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                ফেরত;
            }
        }

        // Unwind through bit packing in one go
        {
            ধ্রুবক auto res = unwindSel(fromp, lsb, width);
            যদি (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                ফেরত;
            }
        }

        // Sel from Not
        যদি (DfgNot* ধ্রুবক notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            যদি (!notp->hasMultipleSinks()) {
                APPLYING(ঠেলো_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ধ্রুবক newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ফেরত;
                }
            }
        }

        // Sel from Cond
        যদি (DfgCond* ধ্রুবক condp = fromp->cast<DfgCond>()) {
            যদি (!condp->hasMultipleSinks()) {
                APPLYING(ঠেলো_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ধ্রুবক newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ধ্রুবক newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ধ্রুবক newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ফেরত;
                }
            }
        }

        // Sel from ShiftL
        যদি (DfgShiftL* ধ্রুবক shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            যদি (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(ঠেলো_SEL_THROUGH_SHIFTL) {
                    DfgSel* ধ্রুবক newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgMux* ধ্রুবক vtxp) override {
        DfgVertex* ধ্রুবক fromp = vtxp->fromp();
        DfgVertex* ধ্রুবক lsbp = vtxp->lsbp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (DfgConst* ধ্রুবক lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->অঙ্ক().toUInt()));
                ফেরত;
            }
        }

        যদি (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                ফেরত;
            }
        }

        যদি (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                ফেরত;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    শূন্য visit(DfgAnd* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        যদি (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            যদি (DfgNot* ধ্রুবক lhsNotp = lhsp->cast<DfgNot>()) {
                যদি (DfgNot* ধ্রুবক rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* ধ্রুবক orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ফেরত;
                    }
                }
                যদি (DfgNeq* ধ্রুবক rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* ধ্রুবক newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ধ্রুবক orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ফেরত;
                    }
                }
            }
        }

        যদি (DfgConst* ধ্রুবক lConstp = lhsp->cast<DfgConst>()) {
            যদি (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ফেরত;
                }
            }

            যদি (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ফেরত;
                }
            }

            যদি (DfgConcat* ধ্রুবক rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ফেরত;
            }
        }

        যদি (distributive_And_Associative_দ্বিমিক<DfgOr, DfgAnd>(vtxp)) ফেরত;

        যদি (try_ঠেলো_Bitwise_Op_Through_Reductions(vtxp)) ফেরত;

        যদি (try_ঠেলো_Bitwise_Op_Throug_Sel(vtxp)) ফেরত;

        {
            DfgNot* ধ্রুবক lNotp = lhsp->cast<DfgNot>();
            DfgNot* ধ্রুবক rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            যদি ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    ফেরত;
                }
            }

            যদি (DfgAnd* ধ্রুবক rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ধ্রুবক rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ধ্রুবক rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                যদি ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        ফেরত;
                    }
                }
            }
        }

        যদি (vtxp->dtype() == m_bitDType) {
            যদি (tryReplaceBitwiseWithReduction(vtxp)) ফেরত;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            যদি (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            যদি (repp && condp && repp->srcp()->আকার_2() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ধ্রুবক newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgOr* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        যদি (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            যদি (DfgNot* ধ্রুবক lhsNotp = lhsp->cast<DfgNot>()) {
                যদি (DfgNot* ধ্রুবক rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* ধ্রুবক andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ফেরত;
                    }
                }
                যদি (DfgNeq* ধ্রুবক rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* ধ্রুবক newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ধ্রুবক andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ফেরত;
                    }
                }
            }
        }

        যদি (DfgConcat* ধ্রুবক lhsConcatp = lhsp->cast<DfgConcat>()) {
            যদি (DfgConcat* ধ্রুবক rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    যদি (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ফেরত;
                        }
                    }
                    যদি (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ফেরত;
                        }
                    }
                }
            }
        }

        যদি (DfgConst* ধ্রুবক lConstp = lhsp->cast<DfgConst>()) {
            যদি (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ফেরত;
                }
            }

            যদি (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ফেরত;
                }
            }

            যদি (DfgConcat* ধ্রুবক rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ফেরত;
            }
        }

        যদি (distributive_And_Associative_দ্বিমিক<DfgAnd, DfgOr>(vtxp)) ফেরত;

        যদি (try_ঠেলো_Bitwise_Op_Through_Reductions(vtxp)) ফেরত;

        যদি (try_ঠেলো_Bitwise_Op_Throug_Sel(vtxp)) ফেরত;

        {
            DfgNot* ধ্রুবক lNotp = lhsp->cast<DfgNot>();
            DfgNot* ধ্রুবক rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            যদি ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    ফেরত;
                }
            }

            যদি (DfgOr* ধ্রুবক rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ধ্রুবক rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ধ্রুবক rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                যদি ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        ফেরত;
                    }
                }
            }
        }

        যদি (vtxp->dtype() == m_bitDType) {
            যদি (tryReplaceBitwiseWithReduction(vtxp)) ফেরত;
        }
    }

    শূন্য visit(DfgXor* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                ফেরত;
            }
        }

        যদি (DfgConst* ধ্রুবক lConstp = lhsp->cast<DfgConst>()) {
            যদি (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ফেরত;
                }
            }
            যদি (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ফেরত;
                }
            }
            যদি (DfgConcat* ধ্রুবক rConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) ফেরত;
            }
        }

        যদি (try_ঠেলো_Bitwise_Op_Through_Reductions(vtxp)) ফেরত;

        যদি (try_ঠেলো_Bitwise_Op_Throug_Sel(vtxp)) ফেরত;

        যদি (vtxp->dtype() == m_bitDType) {
            যদি (tryReplaceBitwiseWithReduction(vtxp)) ফেরত;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    শূন্য visit(DfgAdd* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                ফেরত;
            }
        }

        ধ্রুবক std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_গুনতি_Ones_Term(lhsp);
        যদি (DfgVertex* ধ্রুবক lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_গুনতি_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            যদি (!std::get<0>(rTerm)) {
                যদি (DfgAdd* ধ্রুবক rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_গুনতি_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            যদি (DfgVertex* ধ্রুবক rVtxp = std::get<0>(rTerm)) {
                যদি (isSame(lVtxp, rVtxp)) {
                    ধ্রুবক uint32_t lLsb = std::get<1>(lTerm);
                    ধ্রুবক uint32_t rLsb = std::get<1>(rTerm);
                    ধ্রুবক uint32_t lWidth = std::get<2>(lTerm);
                    ধ্রুবক uint32_t rWidth = std::get<2>(rTerm);
                    যৌক্তিক adjoined = সত্য;
                    uint32_t lsb = 0;
                    যদি (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } অন্যথা যদি (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } অন্যথা {
                        adjoined = মিথ্যা;
                    }
                    যদি (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_গুনতি_ONES) {
                            DfgSel* ধ্রুবক selp
                                = make<DfgSel>(vtxp->fileline(),
                                               Dfg_তথ্য_Type::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_গুনতি_Ones>(flp, Dfg_তথ্য_Type::packed(32), selp);
                            যদি (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } অন্যথা যদি (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            যদি (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            ফেরত;
                        }
                    }
                }
            }
        }
    }

    শূন্য visit(DfgArraySel* ধ্রুবক vtxp) override {
        DfgConst* ধ্রুবক idxp = vtxp->bitp()->cast<DfgConst>();
        যদি (!idxp) ফেরত;
        DfgVarArray* ধ্রুবক varp = vtxp->fromp()->cast<DfgVarArray>();
        যদি (!varp) ফেরত;
        AstVar* ধ্রুবক astVarp = varp->vscp()->varp();
        যদি (astVarp->isForced()) ফেরত;
        যদি (astVarp->isSigUserRWPublic()) ফেরত;
        DfgVertex* ধ্রুবক srcp = varp->srcp();
        যদি (!srcp) {
            যদি (vtxp->isPacked()) {
                যদি (AstConsArray* ধ্রুবক iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    যদি (AstConst* ধ্রুবক valp = VN_CAST(iap->valuep(idxp->to_আকার_T()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(নতুন DfgConst{m_dfg, valp->fileline(), valp->অঙ্ক()});
                            ফেরত;
                        }
                    }
                }
            }
            ফেরত;
        }

        যদি (DfgSpliceArray* ধ্রুবক splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* ধ্রুবক driverp = splicep->driverAt(idxp->to_আকার_T());
            যদি (!driverp) ফেরত;
            DfgUnitArray* ধ্রুবক uap = driverp->cast<DfgUnitArray>();
            যদি (!uap) ফেরত;
            যদি (uap->srcp()->is<DfgVertexSplice>()) ফেরত;
            // If driven by a variable that had a Driver in DFG, it is partial
            যদি (DfgVertexVar* ধ্রুবক dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                যদি (dvarp->srcp()) ফেরত;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ফেরত;
            }
        }

        যদি (DfgUnitArray* ধ্রুবক uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->to_আকার_T() == 0, vtxp, "Array index out of range");
            যদি (uap->srcp()->is<DfgSplicePacked>()) ফেরত;
            // If driven by a variable that had a Driver in DFG, it is partial
            যদি (DfgVertexVar* ধ্রুবক dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                যদি (dvarp->srcp()) ফেরত;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgConcat* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();

        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isZero(lhsp)) {
            DfgConst* ধ্রুবক lConstp = lhsp->as<DfgConst>();
            যদি (DfgSel* ধ্রুবক rSelp = rhsp->cast<DfgSel>()) {
                যদি (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_শীর্ষ_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        ফেরত;
                    }
                }
            }
        }

        যদি (isZero(rhsp)) {
            DfgConst* ধ্রুবক rConstp = rhsp->as<DfgConst>();
            যদি (DfgSel* ধ্রুবক lSelp = lhsp->cast<DfgSel>()) {
                যদি (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        ফেরত;
                    }
                }
            }
        }

        যদি (DfgNot* ধ্রুবক lNot = lhsp->cast<DfgNot>()) {
            যদি (DfgNot* ধ্রুবক rNot = rhsp->cast<DfgNot>()) {
                যদি (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(ঠেলো_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ধ্রুবক newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ফেরত;
                    }
                }
            }
        }

        {
            যৌক্তিক swap = মিথ্যা;
            যদি (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(ঠেলো_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    ফেরত;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            যদি (DfgConcat* ধ্রুবক rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* ধ্রুবক rlp = rConcatp->lhsp();
                যদি (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(ঠেলো_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* ধ্রুবক newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ফেরত;
                    }
                }
            }
        }

        যদি (DfgSel* ধ্রুবক lSelp = lhsp->cast<DfgSel>()) {
            যদি (DfgSel* ধ্রুবক rSelp = rhsp->cast<DfgSel>()) {
                যদি (isSame(lSelp->fromp(), rSelp->fromp())) {
                    যদি (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ফেরত;
                        }
                    }
                }
            }

            যদি (DfgConcat* ধ্রুবক rConcatp = rhsp->cast<DfgConcat>()) {
                যদি (DfgSel* ধ্রুবক rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    যদি (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        যদি (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                ধ্রুবক uint32_t width = lSelp->width() + rlSelp->width();
                                ধ্রুবক Dfg_তথ্য_Type& dtype = Dfg_তথ্য_Type::packed(width);
                                DfgSel* ধ্রুবক selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ফেরত;
                            }
                        }
                    }
                }
            }
        }

        যদি (DfgSel* ধ্রুবক rSelp = rhsp->cast<DfgSel>()) {
            যদি (DfgConcat* ধ্রুবক lConcatp = lhsp->cast<DfgConcat>()) {
                যদি (DfgSel* ধ্রুবক lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    যদি (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        যদি (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                ধ্রুবক uint32_t width = lrSelp->width() + rSelp->width();
                                ধ্রুবক Dfg_তথ্য_Type& dtype = Dfg_তথ্য_Type::packed(width);
                                DfgSel* ধ্রুবক selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ফেরত;
                            }
                        }
                    }
                }
            }
        }

        যদি (DfgConst* ধ্রুবক lConstp = lhsp->cast<DfgConst>()) {
            যদি (DfgCond* ধ্রুবক rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* ধ্রুবক rtVtxp = rCondp->thenp();
                DfgVertex* ধ্রুবক reVtxp = rCondp->elsep();
                DfgConst* ধ্রুবক rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ধ্রুবক reConstp = reVtxp->cast<DfgConst>();
                যদি (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(ঠেলো_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* ধ্রুবক thenp = [&]() -> DfgVertex* {
                            File_সারি* ধ্রুবক rtFlp = rtVtxp->fileline();
                            যদি (rtConstp) {
                                DfgConst* ধ্রুবক constp = makeZero(rtFlp, vtxp->width());
                                constp->অঙ্ক().opConcat(lConstp->অঙ্ক(), rtConstp->অঙ্ক());
                                ফেরত constp;
                            }
                            ফেরত make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* ধ্রুবক elsep = [&]() -> DfgVertex* {
                            File_সারি* ধ্রুবক reFlp = reVtxp->fileline();
                            যদি (reConstp) {
                                DfgConst* ধ্রুবক constp = makeZero(reFlp, vtxp->width());
                                constp->অঙ্ক().opConcat(lConstp->অঙ্ক(), reConstp->অঙ্ক());
                                ফেরত constp;
                            }
                            ফেরত make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ফেরত;
                    }
                }
            }
        }

        যদি (DfgConst* ধ্রুবক rConstp = rhsp->cast<DfgConst>()) {
            যদি (DfgCond* ধ্রুবক lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* ধ্রুবক ltVtxp = lCondp->thenp();
                DfgVertex* ধ্রুবক leVtxp = lCondp->elsep();
                DfgConst* ধ্রুবক ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ধ্রুবক leConstp = leVtxp->cast<DfgConst>();
                যদি (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(ঠেলো_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* ধ্রুবক thenp = [&]() -> DfgVertex* {
                            File_সারি* ধ্রুবক ltFlp = ltVtxp->fileline();
                            যদি (ltConstp) {
                                DfgConst* ধ্রুবক constp = makeZero(ltFlp, vtxp->width());
                                constp->অঙ্ক().opConcat(ltConstp->অঙ্ক(), rConstp->অঙ্ক());
                                ফেরত constp;
                            }
                            ফেরত make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* ধ্রুবক elsep = [&]() -> DfgVertex* {
                            File_সারি* ধ্রুবক leFlp = leVtxp->fileline();
                            যদি (leConstp) {
                                DfgConst* ধ্রুবক constp = makeZero(leFlp, vtxp->width());
                                constp->অঙ্ক().opConcat(leConstp->অঙ্ক(), rConstp->অঙ্ক());
                                ফেরত constp;
                            }
                            ফেরত make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ফেরত;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ধ্রুবক uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            ধ্রুবক uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            ধ্রুবক uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            যৌক্তিক hasCrossSink = মিথ্যা;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                যদি (ধ্রুবক DfgSel* ধ্রুবক selp = sink.cast<DfgSel>()) {
                    ধ্রুবক uint32_t selLsb = selp->lsb();
                    ধ্রুবক uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ফেরত মিথ্যা;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                যদি (ধ্রুবক DfgVarPacked* ধ্রুবক varp = sink.cast<DfgVarPacked>()) {
                    যদি (!varp->hasSinks() && !varp->isObserved()) ফেরত মিথ্যা;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ফেরত সত্য;
            });
            যদি (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    যদি (lsb != 0)
                        nRhsp = make<DfgSel>(flp, Dfg_তথ্য_Type::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    যদি (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, Dfg_তথ্য_Type::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* ধ্রুবক catp
                        = make<DfgConcat>(flp, Dfg_তথ্য_Type::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ধ্রুবক sp = নতুন DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* ধ্রুবক varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(সত্য);
                    varp->srcp(sp);
                    replace(varp);
                    ফেরত;
                }
            }
        }

        // Convert various forms to REPLICATE
        যদি (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ফেরত;
            }
        }
        যদি (DfgRep* ধ্রুবক lRepp = lhsp->cast<DfgRep>()) {
            যদি (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ফেরত;
                }
            }
        }
        যদি (DfgRep* ধ্রুবক rRepp = rhsp->cast<DfgRep>()) {
            যদি (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ফেরত;
                }
            }
        }
        যদি (DfgRep* ধ্রুবক lRepp = lhsp->cast<DfgRep>()) {
            যদি (DfgRep* ধ্রুবক rRepp = rhsp->cast<DfgRep>()) {
                যদি (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ফেরত;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            যদি (DfgConcat* ধ্রুবক rConcatp = rhsp->cast<DfgConcat>()) {
                যদি (DfgRep* ধ্রুবক rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    যদি (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ধ্রুবক uint32_t width = lRepp->width() + rlRepp->width();
                            ধ্রুবক Dfg_তথ্য_Type& dtype = Dfg_তথ্য_Type::packed(width);
                            DfgRep* ধ্রুবক repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ফেরত;
                        }
                    }
                }
            }
        }
    }

    শূন্য visit(DfgDiv* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        যদি (DfgConst* ধ্রুবক rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            যদি (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgDivS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        যদি (DfgConst* ধ্রুবক rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            যদি (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgGt* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgGtS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgGte* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgGteS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgLogAnd* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp) || vtxp->rhsp()->unsafe()) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();

        যদি (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgLogEq* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
    }

    শূন্য visit(DfgLogIf* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp) || vtxp->rhsp()->unsafe()) ফেরত;
    }

    শূন্য visit(DfgLogOr* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp) || vtxp->rhsp()->unsafe()) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();

        যদি (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgLt* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgLtS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgLte* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgLteS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ফেরত;
            }
        }
    }

    শূন্য visit(DfgModDiv* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
    }

    শূন্য visit(DfgModDivS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
    }

    শূন্য visit(DfgMul* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (DfgConst* ধ্রুবক lConstp = lhsp->cast<DfgConst>()) {
            যদি (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ফেরত;
                }
            }
            যদি (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgMulS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (DfgConst* ধ্রুবক lConstp = lhsp->cast<DfgConst>()) {
            যদি (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ফেরত;
                }
            }
            যদি (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgEq* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ফেরত;
            }
        }

        যদি (DfgConst* ধ্রুবক lhsConstp = lhsp->cast<DfgConst>()) {
            যদি (DfgConcat* ধ্রুবক rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ফেরত;
            }

            যদি (rhsp->dtype() == m_bitDType) {
                যদি (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ফেরত;
                    }
                }

                যদি (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        ফেরত;
                    }
                }
            }
        }
    }

    শূন্য visit(DfgNeq* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ফেরত;
            }
        }

        যদি (DfgConst* ধ্রুবক lhsConstp = lhsp->cast<DfgConst>()) {
            যদি (DfgConcat* ধ্রুবক rhsConcatp = rhsp->cast<DfgConcat>()) {
                যদি (try_ঠেলো_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ফেরত;
            }

            যদি (rhsp->dtype() == m_bitDType) {
                যদি (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        ফেরত;
                    }
                }

                যদি (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ফেরত;
                    }
                }
            }
        }
    }

    শূন্য visit(DfgPow* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
    }

    শূন্য visit(DfgPowSS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
    }

    শূন্য visit(DfgPowSU* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
    }

    শূন্য visit(DfgPowUS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
    }

    শূন্য visit(DfgRep* ধ্রুবক vtxp) override {
        DfgVertex* ধ্রুবক srcp = vtxp->srcp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (DfgConst* ধ্রুবক sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ধ্রুবক resultp = makeZero(flp, vtxp->width());
                resultp->অঙ্ক().opRepl(sConstp->অঙ্ক(), vtxp->গুনতি());
                replace(resultp);
                ফেরত;
            }
        }

        যদি (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ফেরত;
            }
        }

        যদি (DfgRep* ধ্রুবক sRepp = srcp->cast<DfgRep>()) {
            যদি (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgShiftL* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
        যদি (optimizeShiftRHS(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();

        যদি (DfgShiftL* ধ্রুবক lShiftLp = lhsp->cast<DfgShiftL>()) {
            যদি (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_সারি* ধ্রুবক flp = vtxp->fileline();
                    ধ্রুবক Dfg_তথ্য_Type& যোগ_DType = Dfg_তথ্য_Type::packed(rhsp->width() + 1);
                    DfgVertex* ধ্রুবক bp = make<DfgExtend>(flp, যোগ_DType, lShiftLp->rhsp());
                    DfgVertex* ধ্রুবক cp = make<DfgExtend>(flp, যোগ_DType, rhsp);
                    DfgAdd* ধ্রুবক addp = make<DfgAdd>(flp, যোগ_DType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    ফেরত;
                }
            }
        }

        যদি (DfgConst* ধ্রুবক rConstp = rhsp->cast<DfgConst>()) {
            ধ্রুবক uint32_t shiftAmount = rConstp->toU32();

            যদি (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ফেরত;
                }
            }

            যদি (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ফেরত;
                }
            }

            যদি (DfgConcat* ধ্রুবক lConcatp = lhsp->cast<DfgConcat>()) {
                যদি (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* ধ্রুবক lRhsp = lConcatp->rhsp();
                        DfgVertex* ধ্রুবক lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ধ্রুবক uint32_t rW = shiftAmount;
                        ধ্রুবক uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        ধ্রুবক uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_সারি* ধ্রুবক flp = lConcatp->fileline();
                        DfgVertex* ধ্রুবক rp = makeZero(flp, shiftAmount);
                        DfgVertex* ধ্রুবক mp
                            = make<DfgSel>(flp, Dfg_তথ্য_Type::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, Dfg_তথ্য_Type::packed(mW + rW), mp, rp);
                        যদি (!lW) {
                            replace(np);
                            ফেরত;
                        }
                        DfgVertex* ধ্রুবক lp
                            = make<DfgSel>(flp, Dfg_তথ্য_Type::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ফেরত;
                    }
                }
            }

            যদি (DfgSel* ধ্রুবক lSelp = lhsp->cast<DfgSel>()) {
                যদি (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ধ্রুবক uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ধ্রুবক nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_তথ্য_Type::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        ফেরত;
                    }
                }
            }

            যদি (DfgCond* ধ্রুবক lCondp = lhsp->cast<DfgCond>()) {
                যদি (!lCondp->hasMultipleSinks()) {
                    APPLYING(ঠেলো_SHIFTL_THROUGH_COND) {
                        DfgShiftL* ধ্রুবক tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* ধ্রুবক ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ফেরত;
                    }
                }
            }
        }
    }

    শূন্য visit(DfgShiftR* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
        যদি (optimizeShiftRHS(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();

        যদি (DfgShiftR* ধ্রুবক lShiftRp = lhsp->cast<DfgShiftR>()) {
            যদি (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_সারি* ধ্রুবক flp = vtxp->fileline();
                    ধ্রুবক Dfg_তথ্য_Type& যোগ_DType = Dfg_তথ্য_Type::packed(rhsp->width() + 1);
                    DfgVertex* ধ্রুবক bp = make<DfgExtend>(flp, যোগ_DType, lShiftRp->rhsp());
                    DfgVertex* ধ্রুবক cp = make<DfgExtend>(flp, যোগ_DType, rhsp);
                    DfgAdd* ধ্রুবক addp = make<DfgAdd>(flp, যোগ_DType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    ফেরত;
                }
            }
        }

        যদি (DfgConst* ধ্রুবক rConstp = rhsp->cast<DfgConst>()) {
            ধ্রুবক uint32_t shiftAmount = rConstp->toU32();

            যদি (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ফেরত;
                }
            }

            যদি (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ফেরত;
                }
            }

            যদি (DfgConcat* ধ্রুবক lConcatp = lhsp->cast<DfgConcat>()) {
                যদি (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* ধ্রুবক lRhsp = lConcatp->rhsp();
                        DfgVertex* ধ্রুবক lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ধ্রুবক uint32_t lW = shiftAmount;
                        ধ্রুবক uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        ধ্রুবক uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_সারি* ধ্রুবক flp = lConcatp->fileline();
                        DfgVertex* ধ্রুবক lp = makeZero(flp, shiftAmount);
                        DfgVertex* ধ্রুবক mp = make<DfgSel>(flp, Dfg_তথ্য_Type::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, Dfg_তথ্য_Type::packed(lW + mW), lp, mp);
                        যদি (!rW) {
                            replace(np);
                            ফেরত;
                        }
                        DfgVertex* ধ্রুবক rp = make<DfgSel>(flp, Dfg_তথ্য_Type::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ফেরত;
                    }
                }
            }

            যদি (DfgSel* ধ্রুবক lSelp = lhsp->cast<DfgSel>()) {
                যদি (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ধ্রুবক uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ধ্রুবক nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_তথ্য_Type::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        ফেরত;
                    }
                }
            }

            যদি (DfgCond* ধ্রুবক lCondp = lhsp->cast<DfgCond>()) {
                যদি (!lCondp->hasMultipleSinks()) {
                    APPLYING(ঠেলো_SHIFTR_THROUGH_COND) {
                        DfgShiftR* ধ্রুবক tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* ধ্রুবক ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ফেরত;
                    }
                }
            }
        }
    }

    শূন্য visit(DfgShiftRS* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;
        যদি (optimizeShiftRHS(vtxp)) ফেরত;
    }

    শূন্য visit(DfgSub* ধ্রুবক vtxp) override {
        যদি (দ্বিমিক(vtxp)) ফেরত;

        DfgVertex* ধ্রুবক lhsp = vtxp->lhsp();
        DfgVertex* ধ্রুবক rhsp = vtxp->rhsp();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ফেরত;
            }
        }

        যদি (DfgConst* ধ্রুবক rConstp = rhsp->cast<DfgConst>()) {
            যদি (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ফেরত;
                }
            }
            যদি (vtxp->dtype() == m_bitDType && rConstp->has_মান(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgMatchMasked* ধ্রুবক vtxp) override {
        যদি (DfgConst* ধ্রুবক constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ধ্রুবক matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->অঙ্ক(), matchVarp)));
                ফেরত;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    শূন্য visit(DfgCond* ধ্রুবক vtxp) override {
        DfgVertex* ধ্রুবক condp = vtxp->condp();
        DfgVertex* ধ্রুবক thenp = vtxp->thenp();
        DfgVertex* ধ্রুবক elsep = vtxp->elsep();
        File_সারি* ধ্রুবক flp = vtxp->fileline();

        যদি (condp->dtype() != m_bitDType) ফেরত;

        যদি (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ফেরত;
            }
        }

        যদি (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ফেরত;
            }
        }

        যদি (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ফেরত;
            }
        }

        যদি (DfgNot* ধ্রুবক condNotp = condp->cast<DfgNot>()) {
            যদি (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ফেরত;
                }
            }
        }

        যদি (DfgNeq* ধ্রুবক condNeqp = condp->cast<DfgNeq>()) {
            যদি (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ধ্রুবক newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ফেরত;
                }
            }
        }

        যদি (DfgNot* ধ্রুবক thenNotp = thenp->cast<DfgNot>()) {
            যদি (DfgNot* ধ্রুবক elseNotp = elsep->cast<DfgNot>()) {
                যদি (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ধ্রুবক newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ফেরত;
                    }
                }
            }
        }

        যদি (DfgOr* ধ্রুবক condOrp = condp->cast<DfgOr>()) {
            যদি (DfgCond* ধ্রুবক thenCondp = thenp->cast<DfgCond>()) {
                যদি (!thenCondp->hasMultipleSinks()) {
                    যদি (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ধ্রুবক ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ফেরত;
                        }
                    }
                    যদি (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ধ্রুবক ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ফেরত;
                        }
                    }
                }
            }
        }

        যদি (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            যদি (DfgAdd* ধ্রুবক thenAddp = thenp->cast<DfgAdd>()) {
                যদি (DfgConst* ধ্রুবক constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    যদি (constp->has_মান(1)) {
                        যদি (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ধ্রুবক extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_সারি* ধ্রুবক thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ফেরত;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            যদি (DfgSub* ধ্রুবক thenSubp = thenp->cast<DfgSub>()) {
                যদি (DfgConst* ধ্রুবক constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    যদি (constp->has_মান(1)) {
                        যদি (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ধ্রুবক extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_সারি* ধ্রুবক thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ফেরত;
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
                    ফেরত;
                }
            }
            যদি (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ফেরত;
                }
            }
        }

        যদি (vtxp->width() <= VL_QUADSIZE) {
            যদি (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* ধ্রুবক maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ফেরত;
                }
            }
            যদি (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* ধ্রুবক maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ফেরত;
                }
            }
            যদি (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* ধ্রুবক maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ফেরত;
                }
            }
            যদি (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* ধ্রুবক maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ফেরত;
                }
            }

            যদি (DfgOr* ধ্রুবক tOrp = thenp->cast<DfgOr>()) {
                যদি (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* ধ্রুবক maskp = replicate(vtxp, condp);
                        DfgAnd* ধ্রুবক andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ফেরত;
                    }
                }
                যদি (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* ধ্রুবক maskp = replicate(vtxp, condp);
                        DfgAnd* ধ্রুবক andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ফেরত;
                    }
                }
            }
        }

        যদি (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ধ্রুবক std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            যদি (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ধ্রুবক uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    যদি (rWidth != rhsp->width()) {
                        ধ্রুবক Dfg_তথ্য_Type& rDtype = Dfg_তথ্য_Type::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    যদি (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ফেরত;
                    }
                    // Create new LHS
                    ধ্রুবক uint32_t lWidth = vtxp->width() - rWidth;
                    ধ্রুবক Dfg_তথ্য_Type& lDtype = Dfg_তথ্য_Type::packed(lWidth);
                    DfgVertex* ধ্রুবক lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* ধ্রুবক lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* ধ্রুবক lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ফেরত;
                }
            }

            ধ্রুবক std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            যদি (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ধ্রুবক uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    যদি (lWidth != lhsp->width()) {
                        ধ্রুবক Dfg_তথ্য_Type& lDtype = Dfg_তথ্য_Type::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    যদি (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ফেরত;
                    }
                    // Create new RHS
                    ধ্রুবক uint32_t rWidth = vtxp->width() - lWidth;
                    ধ্রুবক Dfg_তথ্য_Type& rDtype = Dfg_তথ্য_Type::packed(rWidth);
                    DfgVertex* ধ্রুবক rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* ধ্রুবক rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* ধ্রুবক rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ফেরত;
                }
            }
        }

        যদি (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                যদি (ধ্রুবক uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ধ্রুবক zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ফেরত;
            }
        }

        যদি (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                যদি (ধ্রুবক uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ধ্রুবক zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ফেরত;
            }
        }

        যদি (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ফেরত;
            }
        }

        যদি (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ফেরত;
            }
        }

        যদি (DfgCond* ধ্রুবক tCondp = thenp->cast<DfgCond>()) {
            যদি (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ফেরত;
                }
            }
        }

        যদি (DfgCond* ধ্রুবক eCondp = elsep->cast<DfgCond>()) {
            যদি (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ফেরত;
                }
            }
        }
    }

    শূন্য visit(DfgVertexVar* ধ্রুবক vtxp) override {
        যদি (vtxp->hasSinks()) ফেরত;
        যদি (vtxp->isObserved()) ফেরত;
        যদি (vtxp->defaultp()) ফেরত;

        // If undriven, or driven from another var, it is completely redundant.
        যদি (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ফেরত;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        যৌক্তিক foundOne = মিথ্যা;
        ধ্রুবক যৌক্তিক keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> যৌক্তিক {
            // Ignore non-observable variable sinks. These can be eliminated.
            যদি (ধ্রুবক DfgVertexVar* ধ্রুবক varp = sink.cast<DfgVertexVar>()) {
                যদি (!varp->hasSinks() && !varp->isObserved()) ফেরত মিথ্যা;
            }
            // Keep if found more than one sink
            যদি (foundOne) ফেরত সত্য;
            // Mark first sink found
            foundOne = সত্য;
            ফেরত মিথ্যা;
        });
        যদি (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ফেরত;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        জন্য (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.আকার_2() * 2);
        m_iterList.reserve(m_dfg.আকার_2() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ঠেলো_back(nullptr);
        m_iterList.ঠেলো_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        জন্য (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        জন্য (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        যখন (সত্য) {
            // Process the work list - keep the placeholder at index 0
            যখন (m_workList.আকার_2() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.বের_করো_back();
                যদি (!m_vtxp) চালু;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_work_List_সূচক = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                যদি (DfgVertexVar* ধ্রুবক varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    চালু;
                }

                // Ast references are not considered
                যদি (m_vtxp->is<DfgVertexAst>()) চালু;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                যদি (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    যদি (DfgVertex* ধ্রুবক sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            চালু;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            যদি (m_iterList.আকার_2() == 1) বিরতি;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                আকার_t begin = 0;
                আকার_t end = m_iterList.আকার_2();
                জন্য (আকার_t hops = 1; hops <= 4; ++hops) {
                    জন্য (আকার_t i = begin; i < end; ++i) {
                        DfgVertex* ধ্রুবক vtxp = m_iterList[i];
                        যদি (!vtxp) চালু;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            ফেরত মিথ্যা;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            ফেরত মিথ্যা;
                        });
                    }
                    begin = end;
                    end = m_iterList.আকার_2();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            জন্য (DfgVertex* ধ্রুবক vtxp : m_iterList) {
                যদি (!vtxp) চালু;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

সর্বজনীন:
    স্থির শূন্য apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

শূন্য V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    যদি (!v3Global.opt.fDfgPeephole()) ফেরত;
    V3DfgPeephole::apply(dfg, ctx);
}
