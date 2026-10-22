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

سانچو <قسم_نالو T_Reduction>
بنية Reduction_منزل_Bitwise_Impl {};
سانچو <> بنية Reduction_منزل_Bitwise_Impl<DfgRedAnd> { استعمال type = DfgAnd; };
سانچو <> بنية Reduction_منزل_Bitwise_Impl<DfgRedOr>  { استعمال type = DfgOr;  };
سانچو <> بنية Reduction_منزل_Bitwise_Impl<DfgRedXor> { استعمال type = DfgXor; };
سانچو <قسم_نالو T_Reduction>
استعمال Reduction_منزل_Bitwise = قسم_نالو Reduction_منزل_Bitwise_Impl<T_Reduction>::type;

سانچو <قسم_نالو T_Bitwise>
بنية Bitwise_منزل_Reduction_Impl {};
سانچو <> بنية Bitwise_منزل_Reduction_Impl<DfgAnd> { استعمال type = DfgRedAnd; };
سانچو <> بنية Bitwise_منزل_Reduction_Impl<DfgOr>  { استعمال type = DfgRedOr;  };
سانچو <> بنية Bitwise_منزل_Reduction_Impl<DfgXor> { استعمال type = DfgRedXor; };
سانچو <قسم_نالو T_Reduction>
استعمال Bitwise_منزل_Reduction = قسم_نالو Bitwise_منزل_Reduction_Impl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
سانچو<قسم_نالو راس_2>
بنية IsAssociative : عام std::false_type {};
سانچو <> بنية IsAssociative<DfgAnd> : عام std::true_type {};
سانچو <> بنية IsAssociative<DfgOr>  : عام std::true_type {};
سانچو <> بنية IsAssociative<DfgXor> : عام std::true_type {};
سانچو <> بنية IsAssociative<Dfg_جوڙيو> : عام std::true_type {};
سانچو <> بنية IsAssociative<DfgConcat> : عام std::true_type {};
سانچو <> بنية IsAssociative<DfgMul> : عام std::true_type {};
سانچو <> بنية IsAssociative<DfgMulS> : عام std::true_type {};

// Commutative binary operators (a op b == b op a)
سانچو<قسم_نالو راس_2>
بنية IsCommutative : عام std::false_type {};
سانچو <> بنية IsCommutative<DfgAnd> : عام std::true_type {};
سانچو <> بنية IsCommutative<DfgOr>  : عام std::true_type {};
سانچو <> بنية IsCommutative<DfgXor> : عام std::true_type {};
سانچو <> بنية IsCommutative<Dfg_جوڙيو> : عام std::true_type {};
سانچو <> بنية IsCommutative<DfgEq> : عام std::true_type {};
سانچو <> بنية IsCommutative<DfgMul> : عام std::true_type {};
سانچو <> بنية IsCommutative<DfgMulS> : عام std::true_type {};
سانچو <> بنية IsCommutative<DfgNeq> : عام std::true_type {};

// Idempotent binary operators (a op a == a)
سانچو<قسم_نالو راس_2>
بنية IsIdempotent : عام std::false_type {};
سانچو <> بنية IsIdempotent<DfgAnd> : عام std::true_type {};
سانچو <> بنية IsIdempotent<DfgOr>  : عام std::true_type {};

// Binary result dtype
سانچو<قسم_نالو راس_2>
ثابت Dfg_معلومات_Type& نتيجو_DType(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp);
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<Dfg_جوڙيو>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgAnd>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgConcat>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(lhsp->ويڪر() + rhsp->ويڪر()); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgDiv>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgDivS>       (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgEq>         (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgGt>         (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgGtS>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgGte>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgGteS>       (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgLogAnd>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgLogEq>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgLogIf>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgLogOr>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgLt>         (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgLtS>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgLte>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgLteS>       (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgModDiv>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgModDivS>    (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgMul>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgMulS>       (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgNeq>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع Dfg_معلومات_Type::packed(1); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgOr>         (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgPow>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgPowSS>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgPowSU>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgPowUS>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<Dfg_سرڪايو_L>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<Dfg_سرڪايو_R>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<Dfg_سرڪايو_RS>    (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgSub>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }
سانچو <> ثابت Dfg_معلومات_Type& نتيجو_DType<DfgXor>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { أرجع lhsp->dtype(); }

// Unary constant folding
سانچو<قسم_نالو راس_2> فراغ foldOp(V3_عدد& ٻاهريون, ثابت V3_عدد& src);
سانچو <> فراغ foldOp<Dfg_ڳڻپ_Ones>  (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.op_ڳڻپ_Ones(src); }
سانچو <> فراغ foldOp<DfgExtend>     (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.op_سونپيو(src); }
سانچو <> فراغ foldOp<DfgExtendS>    (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opExtendS(src, src.ويڪر()); }
سانچو <> فراغ foldOp<DfgLogNot>     (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opLogNot(src); }
سانچو <> فراغ foldOp<DfgNegate>     (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opNegate(src); }
سانچو <> فراغ foldOp<DfgNot>        (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opNot(src); }
سانچو <> فراغ foldOp<DfgOneHot>     (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opOneHot(src); }
سانچو <> فراغ foldOp<DfgOneHot0>    (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opOneHot0(src); }
سانچو <> فراغ foldOp<DfgRedAnd>     (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opRedAnd(src); }
سانچو <> فراغ foldOp<DfgRedOr>      (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opRedOr(src); }
سانچو <> فراغ foldOp<DfgRedXor>     (V3_عدد& ٻاهريون, ثابت V3_عدد& src) { ٻاهريون.opRedXor(src); }

// Binary constant folding
سانچو<قسم_نالو راس_2> فراغ foldOp(V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
سانچو <> فراغ foldOp<Dfg_جوڙيو>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.op_جوڙيو(lhs, rhs); }
سانچو <> فراغ foldOp<DfgAnd>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opAnd(lhs, rhs); }
سانچو <> فراغ foldOp<DfgConcat>     (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opConcat(lhs, rhs); }
سانچو <> فراغ foldOp<DfgDiv>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opDiv(lhs, rhs); }
سانچو <> فراغ foldOp<DfgDivS>       (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opDivS(lhs, rhs); }
سانچو <> فراغ foldOp<DfgEq>         (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opEq(lhs, rhs); }
سانچو <> فراغ foldOp<DfgGt>         (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opGt(lhs, rhs); }
سانچو <> فراغ foldOp<DfgGtS>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opGtS(lhs, rhs); }
سانچو <> فراغ foldOp<DfgGte>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opGte(lhs, rhs); }
سانچو <> فراغ foldOp<DfgGteS>       (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opGteS(lhs, rhs); }
سانچو <> فراغ foldOp<DfgLogAnd>     (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opLogAnd(lhs, rhs); }
سانچو <> فراغ foldOp<DfgLogEq>      (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opLogEq(lhs, rhs); }
سانچو <> فراغ foldOp<DfgLogIf>      (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opLogIf(lhs, rhs); }
سانچو <> فراغ foldOp<DfgLogOr>      (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opLogOr(lhs, rhs); }
سانچو <> فراغ foldOp<DfgLt>         (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opLt(lhs, rhs); }
سانچو <> فراغ foldOp<DfgLtS>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opLtS(lhs, rhs); }
سانچو <> فراغ foldOp<DfgLte>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opLte(lhs, rhs); }
سانچو <> فراغ foldOp<DfgLteS>       (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opLteS(lhs, rhs); }
سانچو <> فراغ foldOp<DfgModDiv>     (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opModDiv(lhs, rhs); }
سانچو <> فراغ foldOp<DfgModDivS>    (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opModDivS(lhs, rhs); }
سانچو <> فراغ foldOp<DfgMul>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opMul(lhs, rhs); }
سانچو <> فراغ foldOp<DfgMulS>       (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opMulS(lhs, rhs); }
سانچو <> فراغ foldOp<DfgNeq>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opNeq(lhs, rhs); }
سانچو <> فراغ foldOp<DfgOr>         (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opOr(lhs, rhs); }
سانچو <> فراغ foldOp<DfgPow>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opPow(lhs, rhs); }
سانچو <> فراغ foldOp<DfgPowSS>      (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opPowSS(lhs, rhs); }
سانچو <> فراغ foldOp<DfgPowSU>      (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opPowSU(lhs, rhs); }
سانچو <> فراغ foldOp<DfgPowUS>      (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opPowUS(lhs, rhs); }
سانچو <> فراغ foldOp<Dfg_سرڪايو_L>     (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.op_سرڪايو_L(lhs, rhs); }
سانچو <> فراغ foldOp<Dfg_سرڪايو_R>     (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.op_سرڪايو_R(lhs, rhs); }
سانچو <> فراغ foldOp<Dfg_سرڪايو_RS>    (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.op_سرڪايو_RS(lhs, rhs, lhs.ويڪر()); }
سانچو <> فراغ foldOp<DfgSub>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opSub(lhs, rhs); }
سانچو <> فراغ foldOp<DfgXor>        (V3_عدد& ٻاهريون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { ٻاهريون.opXor(lhs, rhs); }
}
// clang-format on

صنف V3DfgPeephole final : عام DfgVisitor {
    // TYPES
    بنية راس_Info final {
        سائيز_t m_work_List_اشاريو = 0;  // Position of this vertx m_workList (0 means not in list)
        سائيز_t m_iter_List_اشاريو = 0;  // Position of this vertx m_iterList (0 means not in list)
        سائيز_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        سائيز_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        منطقي m_is_Cached_راس = خطأ;  // This vertex is the vertex cached for its operation
    };

    // STATE
    Dfg_گراف& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ثابت Dfg_معلومات_Type& m_ٻٽو_انگ_DType = Dfg_معلومات_Type::packed(1);  // Common, so grab it up front
    std::ويڪٽر<Dfg_راس*> m_workList;  // List of vertices processed in current interation
    std::ويڪٽر<Dfg_راس*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<راس_Info> m_vInfo = m_dfg.makeUserMap<راس_Info>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    Dfg_راس* m_vtxp = خالي_اشارو;  // Currently considered vertex
    سائيز_t m_currentGeneration = 0;  // Current generation number
    سائيز_t m_آخري_Id = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ثابت m_tmpScopep = v3Global.rootp()->چوٽي_Scopep()->scopep();

    // STATIC STATE
    ساكن V3DebugBisect s_debugBisect;  // Debug aid

#وصف_ڪريو APPLYING(id) إذا (checkApplying(VDfg_Peephole_نمونو::id))

    // METHODS
    منطقي checkApplying(VDfg_Peephole_نمونو id) {
        إذا (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) أرجع خطأ;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_ڳڻپ[id];
        أرجع سچ;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    Dfg_راس* cache_راس(Dfg_راس* vtxp) {
        Dfg_راس* ثابت equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_is_Cached_راس || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_is_Cached_راس = !equivp;
        أرجع equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    فراغ invalidate_راس(Dfg_راس* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_is_Cached_راس = خطأ;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    سانچو <قسم_نالو راس_2, قسم_نالو... Operands>
    راس_2* get_Or_Create_راس(File_سٽ* flp, ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
        راس_2* ثابت vtxp = m_cache.getOrCreate<راس_2, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_is_Cached_راس = سچ;
        أرجع vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    سانچو <قسم_نالو راس_2, قسم_نالو... Operands>
    راس_2* get_راس(ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
        راس_2* ثابت vtxp = m_cache.get<راس_2>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_is_Cached_راس, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        أرجع vtxp;
    }

    فراغ incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    فراغ جوڙيو_منزل_Work_List(Dfg_راس* vtxp) {
        راس_Info& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        إذا (vInfo.m_work_List_اشاريو) أرجع;
        // Add to work list
        vInfo.m_work_List_اشاريو = m_workList.سائيز();
        m_workList.ڌڪو_پٺ(vtxp);
    }

    فراغ removeFromWorkList(Dfg_راس* vtxp) {
        راس_Info& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_work_List_اشاريو] = خالي_اشارو;
        vInfo.m_work_List_اشاريو = 0;
    }

    فراغ جوڙيو_منزل_Iter_List(Dfg_راس* vtxp) {
        راس_Info& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        إذا (vInfo.m_iter_List_اشاريو) أرجع;
        // Add to iter list
        vInfo.m_iter_List_اشاريو = m_iterList.سائيز();
        m_iterList.ڌڪو_پٺ(vtxp);
    }

    فراغ removeFromIterList(Dfg_راس* vtxp) {
        راس_Info& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iter_List_اشاريو] = خالي_اشارو;
        vInfo.m_iter_List_اشاريو = 0;
    }

    فراغ delete_راس(Dfg_راس* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_work_List_اشاريو, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ثابت Dfg_راس_Var* ثابت varp = vtxp->cast<Dfg_راس_Var>();
        UASSERT_OBJ(!varp || !varp->hasPrev(), vtxp, "Deleting variable consumed via DfgPrev");

        // Invalidate cache entry
        invalidate_راس(vtxp);

        // It might be in the iter list, remove it
        removeFromIterList(vtxp);

        // Gather source vertices - they might be duplicates, make unique using generation number
        incrementGeneration();
        std::ويڪٽر<Dfg_راس*> srcps;
        srcps.reserve(vtxp->nInputs());
        vtxp->foreach_ذريعو([&](Dfg_راس& src) {
            // If it's a variable, add to work list to see if it became redundant
            إذا (src.is<Dfg_راس_Var>()) {
                جوڙيو_منزل_Work_List(&src);
                أرجع خطأ;
            }
            // Gather unique sources
            راس_Info& vInfo = m_vInfo[src];
            إذا (vInfo.m_generation != m_currentGeneration) srcps.ڌڪو_پٺ(&src);
            vInfo.m_generation = m_currentGeneration;
            أرجع خطأ;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        إذا (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ڌڪو_پٺ(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } وإلا {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ثابت خودڪار mid
            = std::stable_حصا_ڪريو(srcps.begin(), srcps.پڇاڙي(), [](Dfg_راس* srcp) {  //
                  أرجع srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        لكل (خودڪار it = srcps.begin(); it != mid; ++it) جوڙيو_منزل_Iter_List(*it);

        // Recursively delete unused sources
        لكل (خودڪار it = mid; it != srcps.پڇاڙي(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            delete_راس(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    فراغ replace(Dfg_راس* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_work_List_اشاريو, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ثابت خودڪار debugCallback = [&]() -> فراغ {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.ذريعو_Cone({m_vtxp, resp});
            ثابت خودڪار cone = m_dfg.ذريعو_Cone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ثابت Dfg_راس& v) {  //
                أرجع cone->ڳڻپ(&v);
            });
        };
        إذا (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) أرجع;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreach_ذريعو([&](Dfg_راس& src) {
            جوڙيو_منزل_Iter_List(&src);
            أرجع خطأ;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](Dfg_راس& dst) {
            invalidate_راس(&dst);
            أرجع خطأ;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](Dfg_راس& dst) {
            cache_راس(&dst);
            أرجع خطأ;
        });

        // Original vertex is now unused, so delete it
        delete_راس(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        جوڙيو_منزل_Iter_List(resp);
        // Add new vertex and sinks to work list as likely to match
        جوڙيو_منزل_Work_List(resp);
        resp->foreachSink([&](Dfg_راس& dst) {
            جوڙيو_منزل_Work_List(&dst);
            أرجع خطأ;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_سٽ* flp, uint32_t val) { أرجع جديد DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_سٽ* flp, uint32_t ويڪر) {
        أرجع جديد DfgConst{m_dfg, flp, ويڪر, 0};
    }
    DfgConst* makeZero(File_سٽ* flp, ثابت Dfg_معلومات_Type& dtype) {
        أرجع makeZero(flp, dtype.سائيز());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_سٽ* flp, uint32_t ويڪر) {
        DfgConst* ثابت resp = makeZero(flp, ويڪر);
        resp->انگ().سيٽ_All_Bits1();
        أرجع resp;
    }

    // Create a new vertex of the given type
    سانچو <قسم_نالو راس_2, قسم_نالو... Operands>
    راس_2* make(File_سٽ* flp, ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        راس_2* ثابت vtxp = get_Or_Create_راس<راس_2, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        راس_Info& vInfo = m_vInfo[vtxp];
        إذا (!vInfo.m_id) vInfo.m_id = ++m_آخري_Id;
        // Add to work list
        جوڙيو_منزل_Work_List(vtxp);
        // Return new node
        أرجع vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    سانچو <قسم_نالو راس_2, قسم_نالو... Operands>
    راس_2* make(ثابت Dfg_راس* examplep, Operands... operands) {
        أرجع make<راس_2>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    Dfg_راس* replicate(Dfg_راس* vtxp, Dfg_راس* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_ٻٽو_انگ_DType, vtxp, "Expected 1-bit vertex");
        إذا (vtxp->dtype() == m_ٻٽو_انگ_DType) أرجع bitp;
        أرجع make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ساكن منطقي isSame(ثابت Dfg_راس* ap, ثابت Dfg_راس* bp) {
        إذا (ap == bp) أرجع سچ;
        إذا (ap->dtype() != bp->dtype()) أرجع خطأ;
        ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>();
        إذا (!aConstp) أرجع خطأ;
        ثابت DfgConst* ثابت bConstp = bp->cast<DfgConst>();
        إذا (!bConstp) أرجع خطأ;
        أرجع aConstp->انگ().isCaseEq(bConstp->انگ());
    }

    ساكن منطقي isZero(ثابت Dfg_راس* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->isZero();
        أرجع خطأ;
    }

    ساكن منطقي isOnes(ثابت Dfg_راس* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->isOnes();
        أرجع خطأ;
    }

    ساكن منطقي isEqOne(ثابت Dfg_راس* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->انگ().isEqOne();
        أرجع خطأ;
    }

    ساكن منطقي areAdjacent(uint32_t& lsb, ثابت DfgSel* lSelp, ثابت DfgSel* rSelp) {
        إذا (!isSame(lSelp->srcp(), rSelp->srcp())) أرجع خطأ;
        إذا (lSelp->lsb() + lSelp->ويڪر() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            أرجع سچ;
        }
        إذا (lSelp->lsb() == rSelp->lsb() + rSelp->ويڪر()) {
            lsb = rSelp->lsb();
            أرجع سچ;
        }
        أرجع خطأ;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    سانچو <قسم_نالو راس_2>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي foldUnary(راس_2* ثابت vtxp) {
        static_assert(std::is_بنياد_جو<Dfg_راس_Unary, راس_2>::قدر, "Must invoke on unary");
        static_assert(std::is_final<راس_2>::قدر, "Must invoke on final class");
        إذا (DfgConst* ثابت srcp = vtxp->srcp()->سانچو cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ثابت resultp = makeZero(vtxp->fileline(), vtxp->ويڪر());
                foldOp<راس_2>(resultp->انگ(), srcp->انگ());
                replace(resultp);
                أرجع سچ;
            }
        }
        أرجع خطأ;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    سانچو <قسم_نالو راس_2>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي ٻٽو(راس_2* ثابت vtxp) {
        static_assert(std::is_بنياد_جو<Dfg_راس_ٻٽو, راس_2>::قدر, "Must invoke on binary");
        static_assert(std::is_final<راس_2>::قدر, "Must invoke on final class");

        File_سٽ* ثابت flp = vtxp->fileline();
        // LHS/RHS of the vertex
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ثابت lConstp = lhsp->cast<DfgConst>();
        DfgConst* ثابت rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        راس_2* ثابت lSamep = lhsp->cast<راس_2>();
        راس_2* ثابت rSamep = rhsp->cast<راس_2>();

        // Constant folding
        إذا (lConstp && rConstp) {
            APPLYING(FOLD_ٻٽو) {
                DfgConst* ثابت resultp = makeZero(flp, نتيجو_DType<راس_2>(lhsp, rhsp));
                foldOp<راس_2>(resultp->انگ(), lConstp->انگ(), rConstp->انگ());
                replace(resultp);
                أرجع سچ;
            }
        }

        // Idempotent
        إذا VL_CONSTEXPR_CXX17 (IsIdempotent<راس_2>::قدر) {
            إذا (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ٻٽو) {
                    replace(lhsp);
                    أرجع سچ;
                }
            }
        }

        // Associative
        إذا VL_CONSTEXPR_CXX17 (IsAssociative<راس_2>::قدر) {
            // Also commutative and idempotent
            إذا VL_CONSTEXPR_CXX17 (IsCommutative<راس_2>::قدر && IsIdempotent<راس_2>::قدر) {
                إذا (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ٻٽو_LHS) {
                        replace(lSamep);
                        أرجع سچ;
                    }
                }
                إذا (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ٻٽو_RHS) {
                        replace(rSamep);
                        أرجع سچ;
                    }
                }
            }

            إذا (lConstp && rSamep) {
                إذا (DfgConst* ثابت rlConstp = rSamep->lhsp()->سانچو cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ٻٽو_LHS_جو_RHS) {
                        DfgConst* ثابت cp = makeZero(flp, نتيجو_DType<راس_2>(lConstp, rlConstp));
                        foldOp<راس_2>(cp->انگ(), lConstp->انگ(), rlConstp->انگ());
                        replace(make<راس_2>(vtxp, cp, rSamep->rhsp()));
                        أرجع سچ;
                    }
                }
            }

            إذا (lSamep && rConstp) {
                إذا (DfgConst* ثابت lrConstp = lSamep->rhsp()->سانچو cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ٻٽو_RHS_جو_LHS) {
                        DfgConst* ثابت cp = makeZero(flp, نتيجو_DType<راس_2>(lrConstp, rConstp));
                        foldOp<راس_2>(cp->انگ(), lrConstp->انگ(), rConstp->انگ());
                        replace(make<راس_2>(vtxp, lSamep->lhsp(), cp));
                        أرجع سچ;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            إذا (lSamep && !lSamep->has_گڻو_Sinks()) {
                Dfg_راس* ثابت ap = lSamep->lhsp();
                Dfg_راس* ثابت bp = lSamep->rhsp();
                Dfg_راس* ثابت cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                إذا (ap != cp && bp != cp) {
                    APPLYING(ساڄو_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ثابت Dfg_معلومات_Type& ٻار_DType = نتيجو_DType<راس_2>(bp, cp);
                        راس_2* ثابت bcp = make<راس_2>(vtxp->fileline(), ٻار_DType, bp, cp);
                        replace(make<راس_2>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        أرجع سچ;
                    }
                }
            }

            إذا (rSamep && !rSamep->has_گڻو_Sinks()) {
                Dfg_راس* ثابت rlVtxp = rSamep->lhsp();
                Dfg_راس* ثابت rrVtxp = rSamep->rhsp();

                إذا VL_CONSTEXPR_CXX17 (IsCommutative<راس_2>::قدر) {
                    إذا (!lhsp->has_گڻو_Sinks() && rlVtxp->has_گڻو_Sinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<راس_2>(vtxp, rlVtxp, make<راس_2>(vtxp, lhsp, rrVtxp)));
                            أرجع سچ;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                إذا (راس_2* ثابت existingp
                    = get_راس<راس_2>(نتيجو_DType<راس_2>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    إذا (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ٻٽو_LHS_WITH_LHS_جو_RHS) {
                            replace(make<راس_2>(vtxp, existingp, rrVtxp));
                            أرجع سچ;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                إذا VL_CONSTEXPR_CXX17 (IsCommutative<راس_2>::قدر) {
                    إذا (راس_2* ثابت existingp
                        = get_راس<راس_2>(نتيجو_DType<راس_2>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        إذا (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ٻٽو_LHS_WITH_RHS_جو_RHS) {
                                replace(make<راس_2>(vtxp, existingp, rlVtxp));
                                أرجع سچ;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        إذا VL_CONSTEXPR_CXX17 (IsCommutative<راس_2>::قدر) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            إذا (lConstp) {
                إذا (DfgCond* ثابت rCondp = rhsp->cast<DfgCond>()) {
                    إذا (!rCondp->has_گڻو_Sinks()) {
                        APPLYING(ڌڪو_COMMUTATIVE_ٻٽو_THROUGH_COND) {
                            Dfg_راس* ثابت tp = make<راس_2>(vtxp, lConstp, rCondp->thenp());
                            Dfg_راس* ثابت ep = make<راس_2>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            أرجع سچ;
                        }
                    }
                }
                أرجع خطأ;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ثابت منطقي مٽا_سٽا_Sides = [&]() {
            // Don't swap if not commutative, duh
            إذا VL_CONSTEXPR_CXX17 (!IsCommutative<راس_2>::قدر) أرجع خطأ;
            // Const go on left
            إذا (lConstp) أرجع خطأ;
            إذا (rConstp) أرجع سچ;
            // Not go on left
            إذا (lhsp->is<DfgNot>()) أرجع خطأ;
            إذا (rhsp->is<DfgNot>()) أرجع سچ;
            // Same go on right
            إذا (rSamep) أرجع خطأ;
            إذا (lSamep) أرجع سچ;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ثابت راس_Info& lInfo = m_vInfo[lhsp];
            ثابت راس_Info& rInfo = m_vInfo[rhsp];
            أرجع lInfo.m_id > rInfo.m_id;
        }();
        إذا (مٽا_سٽا_Sides) {
            APPLYING(مٽا_سٽا_SIDES_اندريون_ٻٽو) {
                replace(make<راس_2>(vtxp, rhsp, lhsp));
                أرجع سچ;
            }
        }

        أرجع خطأ;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    سانچو <قسم_نالو Other, قسم_نالو راس_2>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي distributive_And_Associative_ٻٽو(راس_2* vtxp) {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        إذا (!lhsp->has_گڻو_Sinks() && !rhsp->has_گڻو_Sinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            إذا (Other* ثابت lp = lhsp->cast<Other>()) {
                إذا (Other* ثابت rp = rhsp->cast<Other>()) {
                    Dfg_راس* ثابت llp = lp->lhsp();
                    Dfg_راس* ثابت lrp = lp->rhsp();
                    Dfg_راس* ثابت rlp = rp->lhsp();
                    Dfg_راس* ثابت rrp = rp->rhsp();
                    Dfg_راس* ap = خالي_اشارو;
                    Dfg_راس* bp = خالي_اشارو;
                    Dfg_راس* cp = خالي_اشارو;
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
                        APPLYING(REPLACE_DISTRIBUTIVE_ٻٽو) {
                            replace(make<Other>(vtxp, ap, make<راس_2>(lhsp, bp, cp)));
                            أرجع سچ;
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    سانچو <قسم_نالو راس_2>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي
    try_ڌڪو_Bitwise_Op_Through_Concat(راس_2* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_سٽ* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ثابت catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ثابت catRConstp = concatp->rhsp()->cast<DfgConst>();
        إذا (catLConstp || catRConstp) {
            APPLYING(ڌڪو_BITWISE_OP_THROUGH_CONCAT) {
                ثابت uint32_t ويڪر = concatp->ويڪر();
                ثابت Dfg_معلومات_Type& lDtype = concatp->lhsp()->dtype();
                ثابت Dfg_معلومات_Type& rDtype = concatp->rhsp()->dtype();
                ثابت uint32_t l_ويڪر = lDtype.سائيز();
                ثابت uint32_t r_ويڪر = rDtype.سائيز();

                // The new Lhs vertex
                Dfg_راس* ثابت newLhsp = [&]() -> Dfg_راس* {
                    DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), l_ويڪر);
                    إذا (catLConstp) {
                        V3_عدد انگ{constp->fileline(), static_cast<صحيح>(l_ويڪر), 0u};
                        انگ.opSel(constp->انگ(), ويڪر - 1, r_ويڪر);
                        foldOp<راس_2>(newLhsConstp->انگ(), انگ, catLConstp->انگ());
                        أرجع newLhsConstp;
                    }
                    newLhsConstp->انگ().opSel(constp->انگ(), ويڪر - 1, r_ويڪر);
                    أرجع make<راس_2>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                Dfg_راس* ثابت newRhsp = [&]() -> Dfg_راس* {
                    DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), r_ويڪر);
                    إذا (catRConstp) {
                        V3_عدد انگ{constp->fileline(), static_cast<صحيح>(r_ويڪر), 0u};
                        انگ.opSel(constp->انگ(), r_ويڪر - 1, 0);
                        foldOp<راس_2>(newRhsConstp->انگ(), انگ, catRConstp->انگ());
                        أرجع newRhsConstp;
                    }
                    newRhsConstp->انگ().opSel(constp->انگ(), r_ويڪر - 1, 0);
                    أرجع make<راس_2>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                أرجع سچ;
            }
        }
        أرجع خطأ;
    }

    سانچو <قسم_نالو راس_2>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي
    try_ڌڪو_Compare_Op_Through_Concat(راس_2* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_سٽ* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        إذا (!concatp->has_گڻو_Sinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ڌڪو_COMPARE_OP_THROUGH_CONCAT) {
                ثابت uint32_t ويڪر = concatp->ويڪر();
                ثابت uint32_t l_ويڪر = concatp->lhsp()->ويڪر();
                ثابت uint32_t r_ويڪر = concatp->rhsp()->ويڪر();

                // The new Lhs vertex
                DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), l_ويڪر);
                newLhsConstp->انگ().opSel(constp->انگ(), ويڪر - 1, r_ويڪر);
                راس_2* ثابت newLhsp
                    = make<راس_2>(flp, m_ٻٽو_انگ_DType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), r_ويڪر);
                newRhsConstp->انگ().opSel(constp->انگ(), r_ويڪر - 1, 0);
                راس_2* ثابت newRhsp
                    = make<راس_2>(flp, m_ٻٽو_انگ_DType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                إذا VL_CONSTEXPR_CXX17 (std::is_same<راس_2, DfgEq>::قدر) {
                    replace(make<DfgAnd>(concatp->fileline(), m_ٻٽو_انگ_DType, newLhsp, newRhsp));
                } وإلا إذا VL_CONSTEXPR_CXX17 (std::is_same<راس_2, DfgNeq>::قدر) {
                    replace(make<DfgOr>(concatp->fileline(), m_ٻٽو_انگ_DType, newLhsp, newRhsp));
                } وإلا {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                أرجع سچ;
            }
        }
        أرجع خطأ;
    }

    سانچو <قسم_نالو Bitwise>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي try_ڌڪو_Bitwise_Op_Through_Reductions(Bitwise* ثابت vtxp) {
        استعمال Reduction = Bitwise_منزل_Reduction<Bitwise>;

        إذا (Reduction* ثابت lRedp = vtxp->lhsp()->سانچو cast<Reduction>()) {
            إذا (Reduction* ثابت rRedp = vtxp->rhsp()->سانچو cast<Reduction>()) {
                Dfg_راس* ثابت lSrcp = lRedp->srcp();
                Dfg_راس* ثابت rSrcp = rRedp->srcp();
                إذا (lSrcp->dtype() == rSrcp->dtype() && lSrcp->ويڪر() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->has_گڻو_Sinks() && !rSrcp->has_گڻو_Sinks()) {
                    APPLYING(ڌڪو_BITWISE_THROUGH_REDUCTION) {
                        File_سٽ* ثابت flp = vtxp->fileline();
                        Bitwise* ثابت bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_ٻٽو_انگ_DType, bwp));
                        أرجع سچ;
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
    ساكن std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(Dfg_راس* vtxp) {
        إذا (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            أرجع {خالي_اشارو, خالي_اشارو};
        }
        إذا (vtxp->has_گڻو_Sinks()) أرجع {خالي_اشارو, خالي_اشارو};
        Dfg_راس_ٻٽو* ثابت bwp = vtxp->as<Dfg_راس_ٻٽو>();
        DfgSel* ثابت lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ثابت rSelp = bwp->rhsp()->cast<DfgSel>();
        إذا (!lSelp || !rSelp) أرجع {خالي_اشارو, خالي_اشارو};
        DfgSel* ثابت nSelp = lSelp->fromp()->ويڪر() <= rSelp->fromp()->ويڪر() ? lSelp : rSelp;
        DfgSel* ثابت wSelp = nSelp == lSelp ? rSelp : lSelp;
        ثابت uint32_t n_ويڪر = nSelp->fromp()->ويڪر();
        إذا (n_ويڪر > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) أرجع {خالي_اشارو, خالي_اشارو};
        إذا (wSelp->lsb() - nSelp->lsb() + n_ويڪر > wSelp->fromp()->ويڪر()) {
            أرجع {خالي_اشارو, خالي_اشارو};
        }
        أرجع {nSelp, wSelp};
    }

    سانچو <قسم_نالو Bitwise>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي try_ڌڪو_Bitwise_Op_Throug_Sel(Bitwise* ثابت vtxp) {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = خالي_اشارو;
            Dfg_راس* extrap = خالي_اشارو;
            إذا (DfgSel* ثابت selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } وإلا إذا (Bitwise* ثابت bitwisep = rhsp->cast<Bitwise>()) {
                إذا (DfgSel* ثابت rlSelp = bitwisep->lhsp()->سانچو cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } وإلا إذا (DfgSel* ثابت rrSelp = bitwisep->rhsp()->سانچو cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            إذا (rSelp) {
                Dfg_راس* ثابت lFromp = lSelp->fromp();
                Dfg_راس* ثابت rFromp = rSelp->fromp();
                إذا (lFromp->dtype() == rFromp->dtype() && lFromp->ويڪر() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ڌڪو_BITWISE_THROUGH_SEL) {
                        Bitwise* ثابت bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        Dfg_راس* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        أرجع سچ;
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
            إذا (rChainp && rChainp->has_گڻو_Sinks()) أرجع خطأ;
            Dfg_راس* ثابت rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ثابت خودڪار lSels = pushableUnalignedSels(lhsp);
            ثابت خودڪار rSels = pushableUnalignedSels(rTermp);
            إذا (!lSels.پهريون && !rSels.پهريون) أرجع خطأ;

            ثابت Dfg_راس* ثابت opp = lSels.پهريون ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            إذا (opp->is<Bitwise>()) أرجع خطأ;
            ثابت خودڪار& sels = lSels.پهريون ? lSels : rSels;
            Dfg_راس* ثابت nFromp = sels.پهريون->fromp();
            Dfg_راس* ثابت wFromp = sels.ٻيو->fromp();
            ثابت uint32_t وٿي = sels.ٻيو->lsb() - sels.پهريون->lsb();

            // Both terms must end up as selects from the same pushed operation
            ثابت خودڪار matches = [&](ثابت Dfg_راس* termp, ثابت خودڪار& tSels) {
                // Original form: 'n[i] OP w[j]'
                إذا (tSels.پهريون) {
                    أرجع termp->type() == opp->type()  //
                           && tSels.پهريون->fromp() == nFromp  //
                           && tSels.ٻيو->fromp() == wFromp  //
                           && tSels.ٻيو->lsb() - tSels.پهريون->lsb() == وٿي;
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
                ثابت Dfg_راس_ٻٽو* ثابت bwp = termp->as<Dfg_راس_ٻٽو>();
                ثابت منطقي nIsLhs = bwp->lhsp() == nFromp;
                إذا (!nIsLhs && bwp->rhsp() != nFromp) أرجع خطأ;
                ثابت DfgSel* ثابت slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                أرجع slicep && slicep->fromp() == wFromp && slicep->lsb() == وٿي;
            };
            إذا (!matches(lhsp, lSels) || !matches(rTermp, rSels)) أرجع خطأ;

            APPLYING(ڌڪو_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_سٽ* ثابت flp = vtxp->fileline();
                ثابت Dfg_معلومات_Type& dtype = nFromp->dtype();
                DfgSel* ثابت slicep = make<DfgSel>(flp, dtype, wFromp, وٿي);
                Dfg_راس* bwp = خالي_اشارو;
                إذا (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } وإلا إذا (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } وإلا {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ثابت خودڪار ڌڪو = [&](Dfg_راس* termp, ثابت DfgSel* nSelp) -> Dfg_راس* {
                    أرجع nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                Dfg_راس* ثابت newLhsp = ڌڪو(lhsp, lSels.پهريون);
                Dfg_راس* ثابت newRTermp = ڌڪو(rTermp, rSels.پهريون);
                إذا (rChainp) {
                    Dfg_راس* ثابت newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } وإلا {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                أرجع سچ;
            }
        }

        أرجع خطأ;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ساكن منطقي isConcatFoldable(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) {
        // Concatenation of the same vertex is a replicate
        إذا (isSame(lhsp, rhsp)) أرجع سچ;
        إذا (ثابت DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            إذا (isSame(lRepp->srcp(), rhsp)) أرجع سچ;
        }
        إذا (ثابت DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
            إذا (isSame(lhsp, rRepp->srcp())) أرجع سچ;
        }
        // Concatenation of adjoining selects is a select
        إذا (ثابت DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            إذا (ثابت DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                أرجع isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->ويڪر();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        إذا (ثابت DfgNot* ثابت lNotp = lhsp->cast<DfgNot>()) {
            إذا (ثابت DfgNot* ثابت rNotp = rhsp->cast<DfgNot>()) {
                أرجع !lNotp->has_گڻو_Sinks()  //
                       && !rNotp->has_گڻو_Sinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        أرجع خطأ;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ساكن منطقي isSameBitwise(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) {
        إذا (lhsp->type() != rhsp->type()) أرجع خطأ;
        أرجع lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ساكن منطقي isConcatSimplifiable(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) {
        إذا (isConcatFoldable(lhsp, rhsp)) أرجع سچ;
        إذا (!isSameBitwise(lhsp, rhsp)) أرجع خطأ;
        إذا (lhsp->has_گڻو_Sinks() || rhsp->has_گڻو_Sinks()) أرجع خطأ;
        ثابت Dfg_راس_ٻٽو* ثابت lp = lhsp->as<Dfg_راس_ٻٽو>();
        ثابت Dfg_راس_ٻٽو* ثابت rp = rhsp->as<Dfg_راس_ٻٽو>();
        إذا (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            أرجع سچ;
        إذا (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            أرجع سچ;
        أرجع خطأ;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ساكن منطقي isConcatPushableThroughBitwise(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp,
                                               منطقي& مٽا_سٽا) {
        إذا (!isSameBitwise(lhsp, rhsp)) أرجع خطأ;
        إذا (lhsp->has_گڻو_Sinks() || rhsp->has_گڻو_Sinks()) أرجع خطأ;
        ثابت Dfg_راس_ٻٽو* ثابت lp = lhsp->as<Dfg_راس_ٻٽو>();
        ثابت Dfg_راس_ٻٽو* ثابت rp = rhsp->as<Dfg_راس_ٻٽو>();
        مٽا_سٽا = خطأ;
        إذا (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) أرجع سچ;
        إذا (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) أرجع سچ;
        مٽا_سٽا = سچ;
        إذا (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) أرجع سچ;
        إذا (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) أرجع سچ;
        أرجع خطأ;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    Dfg_راس* makeConcatPushedThroughBitwise(File_سٽ* flp, ثابت Dfg_راس* lhsp,
                                              ثابت Dfg_راس* rhsp, منطقي مٽا_سٽا) {
        ثابت Dfg_راس_ٻٽو* ثابت lp = lhsp->as<Dfg_راس_ٻٽو>();
        ثابت Dfg_راس_ٻٽو* ثابت rp = rhsp->as<Dfg_راس_ٻٽو>();
        ثابت Dfg_معلومات_Type& dtype = Dfg_معلومات_Type::packed(lp->ويڪر() + rp->ويڪر());
        Dfg_راس* ثابت cp = مٽا_سٽا ? rp->rhsp() : rp->lhsp();
        Dfg_راس* ثابت dp = مٽا_سٽا ? rp->lhsp() : rp->rhsp();
        DfgConcat* ثابت newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ثابت newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        إذا (lp->is<DfgAnd>()) أرجع make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        إذا (lp->is<DfgOr>()) أرجع make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        أرجع make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    سانچو <قسم_نالو Bitwise>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->ويڪر() == 1, vtxp, "Width must be 1");
        استعمال Reduction = Bitwise_منزل_Reduction<Bitwise>;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        إذا (DfgSel* ثابت lSelp = lhsp->سانچو cast<DfgSel>()) {
            DfgSel* rSelp = خالي_اشارو;
            Dfg_راس* extrap = خالي_اشارو;
            إذا (DfgSel* ثابت selp = rhsp->سانچو cast<DfgSel>()) {
                rSelp = selp;
            } وإلا إذا (Bitwise* ثابت rBitwisep = rhsp->سانچو cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->سانچو cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } وإلا إذا (Reduction* ثابت rRedp = rhsp->سانچو cast<Reduction>()) {
                rSelp = rRedp->srcp()->سانچو cast<DfgSel>();
            }
            إذا (rSelp) {
                uint32_t lsb = 0;
                إذا (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_جو_SELS_WITH_REDUCTION) {
                        ثابت Dfg_معلومات_Type& dtype
                            = Dfg_معلومات_Type::packed(lSelp->ويڪر() + rSelp->ويڪر());
                        DfgSel* ثابت newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        Dfg_راس* resp = make<Reduction>(vtxp, newSelp);
                        إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        أرجع سچ;
                    }
                }
            }
        }

        إذا (Reduction* ثابت lRedp = lhsp->سانچو cast<Reduction>()) {
            Reduction* rRedp = rhsp->سانچو cast<Reduction>();
            Dfg_راس* extrap = خالي_اشارو;
            إذا (!rRedp) {
                إذا (Bitwise* ثابت rBitwisep = rhsp->سانچو cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->سانچو cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            إذا (rRedp) {
                إذا (DfgSel* ثابت lSelp = lRedp->srcp()->سانچو cast<DfgSel>()) {
                    إذا (DfgSel* ثابت rSelp = rRedp->srcp()->سانچو cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        إذا (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_جو_REDUCTION_جو_SELS_WITH_REDUCTION) {
                                ثابت Dfg_معلومات_Type& dtype
                                    = Dfg_معلومات_Type::packed(lSelp->ويڪر() + rSelp->ويڪر());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                Dfg_راس* resp = make<Reduction>(vtxp, newSelp);
                                إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                أرجع سچ;
                            }
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    سانچو <قسم_نالو Reduction>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي optimizeReduction(Reduction* ثابت vtxp) {
        استعمال Bitwise = Reduction_منزل_Bitwise<Reduction>;

        إذا (foldUnary(vtxp)) أرجع سچ;

        Dfg_راس* ثابت srcp = vtxp->srcp();
        File_سٽ* ثابت flp = vtxp->fileline();

        // Reduction of 1-bit value
        إذا (srcp->dtype() == m_ٻٽو_انگ_DType) {
            APPLYING(REMOVE_ويڪر_ONE_REDUCTION) {
                replace(srcp);
                أرجع سچ;
            }
        }

        إذا (DfgCond* ثابت condp = srcp->cast<DfgCond>()) {
            إذا (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ڌڪو_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ثابت newThenp = make<Reduction>(flp, m_ٻٽو_انگ_DType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ثابت newElsep = make<Reduction>(flp, m_ٻٽو_انگ_DType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), m_ٻٽو_انگ_DType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    أرجع سچ;
                }
            }
        }

        إذا (DfgConcat* ثابت concatp = srcp->cast<DfgConcat>()) {
            إذا (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_ٻٽو_انگ_DType
                || concatp->rhsp()->dtype() == m_ٻٽو_انگ_DType) {
                APPLYING(ڌڪو_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ثابت lRedp
                        = make<Reduction>(concatp->fileline(), m_ٻٽو_انگ_DType, concatp->lhsp());
                    Reduction* ثابت rRedp
                        = make<Reduction>(concatp->fileline(), m_ٻٽو_انگ_DType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_ٻٽو_انگ_DType, lRedp, rRedp));
                    أرجع سچ;
                }
            }
        }

        إذا (Bitwise* ثابت bitwisep = vtxp->srcp()->سانچو cast<Bitwise>()) {
            إذا (!bitwisep->has_گڻو_Sinks()) {
                إذا (bitwisep->lhsp()->سانچو is<DfgConcat>()
                    || bitwisep->rhsp()->سانچو is<DfgConcat>()) {
                    APPLYING(ڌڪو_REDUCTION_THROUGH_BITWISE_جو_CONCAT) {
                        Reduction* ثابت newLhsp
                            = make<Reduction>(flp, m_ٻٽو_انگ_DType, bitwisep->lhsp());
                        Reduction* ثابت newRhsp
                            = make<Reduction>(flp, m_ٻٽو_انگ_DType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_ٻٽو_انگ_DType, newLhsp, newRhsp));
                        أرجع سچ;
                    }
                }

                إذا (DfgSel* ثابت lSelp = bitwisep->lhsp()->سانچو cast<DfgSel>()) {
                    إذا (DfgSel* ثابت rSelp = bitwisep->rhsp()->سانچو cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        إذا (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ڌڪو_REDUCTION_THROUGH_BITWISE_جو_SELS) {
                                ثابت Dfg_معلومات_Type& dtype
                                    = Dfg_معلومات_Type::packed(lSelp->ويڪر() + rSelp->ويڪر());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                أرجع سچ;
                            }
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    سانچو <قسم_نالو سرڪايو>
    VL_ATTR_WARN_اڻ_استعمال_ٿيل_نتيجو منطقي optimize_سرڪايو_RHS(سرڪايو* ثابت vtxp) {
        static_assert(std::is_بنياد_جو<Dfg_راس_ٻٽو, سرڪايو>::قدر, "Must invoke on binary");
        static_assert(std::is_final<سرڪايو>::قدر, "Must invoke on final class");
        إذا (ثابت DfgConcat* ثابت concatp = vtxp->rhsp()->سانچو cast<DfgConcat>()) {
            إذا (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_جو_سرڪايو) {
                    replace(make<سرڪايو>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    أرجع سچ;
                }
            }
        }
        أرجع خطأ;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<Dfg_راس*, uint32_t, uint32_t> جوڙيو_منزل_ڳڻپ_Ones_رڪن(Dfg_راس* vtxp) {
        إذا (DfgConcat* ثابت oCatp = vtxp->cast<DfgConcat>()) {
            إذا (isZero(oCatp->lhsp())) {
                إذا (Dfg_ڳڻپ_Ones* ثابت ڳڻپ_Onesp = oCatp->rhsp()->cast<Dfg_ڳڻپ_Ones>()) {
                    // Zero extended count ones
                    إذا (DfgSel* ثابت selp = ڳڻپ_Onesp->srcp()->cast<DfgSel>()) {
                        أرجع {selp->fromp(), selp->lsb(), selp->ويڪر()};
                    }
                } وإلا إذا (DfgSel* ثابت selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    إذا (selp->dtype() == m_ٻٽو_انگ_DType) {  //
                        أرجع {selp->fromp(), selp->lsb(), selp->ويڪر()};
                    }
                }
            }
            أرجع {خالي_اشارو, 0, 0};
        }
        إذا (Dfg_ڳڻپ_Ones* ثابت ڳڻپ_Onesp = vtxp->cast<Dfg_ڳڻپ_Ones>()) {
            // Simple count ones
            إذا (DfgSel* ثابت selp = ڳڻپ_Onesp->srcp()->cast<DfgSel>()) {
                أرجع {selp->fromp(), selp->lsb(), selp->ويڪر()};
            }
            أرجع {خالي_اشارو, 0, 0};
        }
        إذا (DfgSel* ثابت oSelp = vtxp->cast<DfgSel>()) {
            إذا (oSelp->lsb() == 0) {
                // Truncated count ones
                إذا (Dfg_ڳڻپ_Ones* ثابت ڳڻپ_Onesp = oSelp->fromp()->cast<Dfg_ڳڻپ_Ones>()) {
                    // Zero extended count ones
                    إذا (DfgSel* ثابت selp = ڳڻپ_Onesp->srcp()->cast<DfgSel>()) {
                        أرجع {selp->fromp(), selp->lsb(), selp->ويڪر()};
                    }
                }
            }
            // Single bit select
            إذا (oSelp->dtype() == m_ٻٽو_انگ_DType) {  //
                أرجع {oSelp->fromp(), oSelp->lsb(), 1};
            }
            أرجع {خالي_اشارو, 0, 0};
        }
        // Altered form of extended MSB
        إذا (Dfg_سرڪايو_R* ثابت shiftrp = vtxp->cast<Dfg_سرڪايو_R>()) {
            إذا (DfgConst* ثابت rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                إذا (rConstp->منزل_U32() == shiftrp->ويڪر() - 1) {
                    أرجع {shiftrp->lhsp(), shiftrp->ويڪر() - 1, 1};
                }
            }
            أرجع {خالي_اشارو, 0, 0};
        }
        // Not applicable
        أرجع {خالي_اشارو, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<Dfg_راس*, uint32_t> unwindSel(Dfg_راس* fromp, uint32_t lsb,
                                              ثابت uint32_t ويڪر) {
        طالما (سچ) {
            ثابت uint32_t msb = lsb + ويڪر - 1;

            // Sel from Concat
            إذا (DfgConcat* ثابت concatp = fromp->cast<DfgConcat>()) {
                Dfg_راس* ثابت lhsp = concatp->lhsp();
                Dfg_راس* ثابت rhsp = concatp->rhsp();

                إذا (msb < rhsp->ويڪر()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_جو_CONCAT) {
                        fromp = rhsp;
                        استمر;
                    }
                } وإلا إذا (lsb >= rhsp->ويڪر()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_جو_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->ويڪر();
                        استمر;
                    }
                }
            }

            إذا (DfgRep* ثابت repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ثابت uint32_t src_ويڪر = repp->srcp()->ويڪر();
                إذا (ويڪر <= src_ويڪر) {
                    ثابت uint32_t newLsb = lsb % src_ويڪر;
                    ثابت uint32_t newMsb = newLsb + ويڪر - 1;
                    إذا (newMsb < src_ويڪر) {
                        APPLYING(ڌڪو_SEL_THROUGH_REP) {
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
                ثابت خودڪار pair = varp->driver_جو_Range(lsb, ويڪر);
                Dfg_راس* ثابت driverp = pair.پهريون;
                ثابت uint32_t driverLsb = pair.ٻيو;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                إذا (driverp) {
                    إذا (driverp == varp->defaultp()) {
                        APPLYING(ڌڪو_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            استمر;
                        }
                    } وإلا {
                        APPLYING(ڌڪو_SEL_THROUGH_SPLICE) {
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
    std::pair<Dfg_راس*, uint32_t> commonLSBs(Dfg_راس* ap, Dfg_راس* bp) {
        إذا (ap == bp) أرجع {ap, ap->ويڪر()};

        // If both constants, check LSBs
        إذا (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            إذا (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت V3_عدد& a_انگ = aConstp->انگ();
                ثابت V3_عدد& b_انگ = bConstp->انگ();
                // Max match is the shorter constant
                ثابت uint32_t max_ميلاپ = std::min(aConstp->ويڪر(), bConstp->ويڪر());
                // Check all bits
                uint32_t ميلاپ_ويڪر = 0;
                لكل (; ميلاپ_ويڪر < max_ميلاپ; ++ميلاپ_ويڪر) {
                    إذا (a_انگ.ٻٽو_انگ_Is0(ميلاپ_ويڪر) != b_انگ.ٻٽو_انگ_Is0(ميلاپ_ويڪر)) اكسر;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aConstp->ويڪر() < bConstp->ويڪر() ? aConstp : bConstp;
                أرجع {ميلاپ_ويڪر ? shorterp : خالي_اشارو, ميلاپ_ويڪر};
            }
        }

        // If Concat, check against the RHS
        إذا (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) أرجع commonLSBs(catp->rhsp(), bp);
        إذا (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) أرجع commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        إذا (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            إذا (selp->lsb() == 0) {
                Dfg_راس* ثابت fromp = selp->fromp();
                ثابت std::pair<Dfg_راس*, uint32_t> common = commonLSBs(fromp, bp);
                أرجع {common.پهريون, std::min(common.ٻيو, selp->ويڪر())};
            }
        }
        إذا (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            إذا (selp->lsb() == 0) {
                Dfg_راس* ثابت fromp = selp->fromp();
                ثابت std::pair<Dfg_راس*, uint32_t> common = commonLSBs(ap, fromp);
                أرجع {common.پهريون, std::min(common.ٻيو, selp->ويڪر())};
            }
        }

        // Otherwise no common LSBs
        أرجع {خالي_اشارو, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_راس*, uint32_t> commonMSBs(Dfg_راس* ap, Dfg_راس* bp) {
        إذا (ap == bp) أرجع {ap, ap->ويڪر()};

        // If both constants, check MSBs
        إذا (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            إذا (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت uint32_t aMsb = aConstp->ويڪر() - 1;
                ثابت uint32_t bMsb = bConstp->ويڪر() - 1;
                ثابت V3_عدد& a_انگ = aConstp->انگ();
                ثابت V3_عدد& b_انگ = bConstp->انگ();
                // Max match is the shorter constant
                ثابت uint32_t max_ميلاپ = std::min(aConstp->ويڪر(), bConstp->ويڪر());
                // Check all bits
                uint32_t ميلاپ_ويڪر = 0;
                لكل (; ميلاپ_ويڪر < max_ميلاپ; ++ميلاپ_ويڪر) {
                    إذا (a_انگ.ٻٽو_انگ_Is0(aMsb - ميلاپ_ويڪر) != b_انگ.ٻٽو_انگ_Is0(bMsb - ميلاپ_ويڪر)) اكسر;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aMsb < bMsb ? aConstp : bConstp;
                أرجع {ميلاپ_ويڪر ? shorterp : خالي_اشارو, ميلاپ_ويڪر};
            }
        }

        // If Concat, check against the LHS
        إذا (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) أرجع commonMSBs(catp->lhsp(), bp);
        إذا (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) أرجع commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        إذا (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            Dfg_راس* ثابت fromp = selp->fromp();
            إذا (selp->msb() == fromp->ويڪر() - 1) {
                ثابت std::pair<Dfg_راس*, uint32_t> common = commonMSBs(fromp, bp);
                أرجع {common.پهريون, std::min(common.ٻيو, selp->ويڪر())};
            }
        }
        إذا (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            Dfg_راس* ثابت fromp = selp->fromp();
            إذا (selp->msb() == fromp->ويڪر() - 1) {
                ثابت std::pair<Dfg_راس*, uint32_t> common = commonMSBs(ap, fromp);
                أرجع {common.پهريون, std::min(common.ٻيو, selp->ويڪر())};
            }
        }

        // Otherwise no common MSBs
        أرجع {خالي_اشارو, 0};
    }

    // VISIT methods

    فراغ گهمو(Dfg_راس*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    فراغ گهمو(Dfg_ڳڻپ_Ones* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ گهمو(DfgExtend* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            Dfg_راس* ثابت zerop
                = makeZero(vtxp->fileline(), vtxp->ويڪر() - vtxp->srcp()->ويڪر());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            أرجع;
        }
    }

    فراغ گهمو(DfgExtendS* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ گهمو(DfgLogNot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ گهمو(DfgNegate* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ گهمو(DfgNot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;

        // Not of Cond
        إذا (DfgCond* ثابت condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            إذا (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ڌڪو_NOT_THROUGH_COND) {
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

        إذا (!vtxp->srcp()->has_گڻو_Sinks()) {
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

    فراغ گهمو(DfgOneHot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ گهمو(DfgOneHot0* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ گهمو(DfgRedOr* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ گهمو(DfgRedAnd* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ گهمو(DfgRedXor* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ گهمو(DfgSel* ثابت vtxp) override {
        Dfg_راس* ثابت fromp = vtxp->fromp();

        File_سٽ* ثابت flp = vtxp->fileline();

        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t ويڪر = vtxp->ويڪر();
        ثابت uint32_t msb = lsb + ويڪر - 1;

        إذا (DfgConst* ثابت constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ثابت resp = makeZero(flp, ويڪر);
                resp->انگ().opSel(constp->انگ(), msb, lsb);
                replace(resp);
                أرجع;
            }
        }

        // Full width select, replace with the source.
        إذا (fromp->ويڪر() == ويڪر) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_ڀريل_ويڪر_SEL) {
                replace(fromp);
                أرجع;
            }
        }

        // Unwind through bit packing in one go
        {
            ثابت خودڪار res = unwindSel(fromp, lsb, ويڪر);
            إذا (res.پهريون != fromp || res.ٻيو != lsb) {
                replace(make<DfgSel>(vtxp, res.پهريون, res.ٻيو));
                أرجع;
            }
        }

        // Sel from Not
        إذا (DfgNot* ثابت notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            إذا (!notp->has_گڻو_Sinks()) {
                APPLYING(ڌڪو_SEL_THROUGH_NOT) {
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
            إذا (!condp->has_گڻو_Sinks()) {
                APPLYING(ڌڪو_SEL_THROUGH_COND) {
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
        إذا (Dfg_سرڪايو_L* ثابت سرڪايو_Lp = fromp->cast<Dfg_سرڪايو_L>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            إذا (lsb == 0) {
                UASSERT_OBJ(سرڪايو_Lp->lhsp()->ويڪر() >= ويڪر, vtxp, "input of shift narrow");
                APPLYING(ڌڪو_SEL_THROUGH_SHIFTL) {
                    DfgSel* ثابت newSelp = make<DfgSel>(vtxp, سرڪايو_Lp->lhsp(), vtxp->lsb());
                    replace(make<Dfg_سرڪايو_L>(vtxp, newSelp, سرڪايو_Lp->rhsp()));
                    أرجع;
                }
            }
        }
    }

    فراغ گهمو(DfgMux* ثابت vtxp) override {
        Dfg_راس* ثابت fromp = vtxp->fromp();
        Dfg_راس* ثابت lsbp = vtxp->lsbp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (DfgConst* ثابت lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->انگ().منزل_UInt()));
                أرجع;
            }
        }

        إذا (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->ويڪر()));
                أرجع;
            }
        }

        إذا (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->ويڪر()));
                أرجع;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    فراغ گهمو(DfgAnd* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        إذا (!lhsp->has_گڻو_Sinks() || !rhsp->has_گڻو_Sinks()) {
            إذا (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                إذا (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_جو_NOT_AND_NOT) {
                        DfgOr* ثابت orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        أرجع;
                    }
                }
                إذا (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_جو_NOT_AND_NEQ) {
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
                إذا (try_ڌڪو_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) أرجع;
            }
        }

        إذا (distributive_And_Associative_ٻٽو<DfgOr, DfgAnd>(vtxp)) أرجع;

        إذا (try_ڌڪو_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_ڌڪو_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            إذا ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->ويڪر()));
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
                        replace(makeZero(flp, vtxp->ويڪر()));
                        أرجع;
                    }
                }
            }
        }

        إذا (vtxp->dtype() == m_ٻٽو_انگ_DType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            إذا (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            إذا (repp && condp && repp->srcp()->سائيز() == 1 && !condp->has_گڻو_Sinks()
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

    فراغ گهمو(DfgOr* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        إذا (!lhsp->has_گڻو_Sinks() || !rhsp->has_گڻو_Sinks()) {
            إذا (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                إذا (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_جو_NOT_AND_NOT) {
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        أرجع;
                    }
                }
                إذا (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_جو_NOT_AND_NEQ) {
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
                        APPLYING(REPLACE_OR_جو_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            أرجع;
                        }
                    }
                    إذا (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_جو_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
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
                إذا (try_ڌڪو_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) أرجع;
            }
        }

        إذا (distributive_And_Associative_ٻٽو<DfgAnd, DfgOr>(vtxp)) أرجع;

        إذا (try_ڌڪو_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_ڌڪو_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            إذا ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->ويڪر()));
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
                        replace(makeOnes(flp, vtxp->ويڪر()));
                        أرجع;
                    }
                }
            }
        }

        إذا (vtxp->dtype() == m_ٻٽو_انگ_DType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }
    }

    فراغ گهمو(DfgXor* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->ويڪر()));
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
                إذا (try_ڌڪو_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) أرجع;
            }
        }

        إذا (try_ڌڪو_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_ڌڪو_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        إذا (vtxp->dtype() == m_ٻٽو_انگ_DType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    فراغ گهمو(Dfg_جوڙيو* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isZero(lhsp)) {
            APPLYING(REMOVE_جوڙيو_ZERO) {
                replace(rhsp);
                أرجع;
            }
        }

        ثابت std::tuple<Dfg_راس*, uint32_t, uint32_t> l_رڪن = جوڙيو_منزل_ڳڻپ_Ones_رڪن(lhsp);
        إذا (Dfg_راس* ثابت lVtxp = std::get<0>(l_رڪن)) {
            std::tuple<Dfg_راس*, uint32_t, uint32_t> r_رڪن = جوڙيو_منزل_ڳڻپ_Ones_رڪن(rhsp);
            Dfg_راس* extrap = خالي_اشارو;
            إذا (!std::get<0>(r_رڪن)) {
                إذا (Dfg_جوڙيو* ثابت rAddp = rhsp->cast<Dfg_جوڙيو>()) {
                    r_رڪن = جوڙيو_منزل_ڳڻپ_Ones_رڪن(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            إذا (Dfg_راس* ثابت rVtxp = std::get<0>(r_رڪن)) {
                إذا (isSame(lVtxp, rVtxp)) {
                    ثابت uint32_t lLsb = std::get<1>(l_رڪن);
                    ثابت uint32_t rLsb = std::get<1>(r_رڪن);
                    ثابت uint32_t l_ويڪر = std::get<2>(l_رڪن);
                    ثابت uint32_t r_ويڪر = std::get<2>(r_رڪن);
                    منطقي adjoined = سچ;
                    uint32_t lsb = 0;
                    إذا (lLsb + l_ويڪر == rLsb) {
                        lsb = lLsb;
                    } وإلا إذا (lLsb == rLsb + r_ويڪر) {
                        lsb = rLsb;
                    } وإلا {
                        adjoined = خطأ;
                    }
                    إذا (adjoined) {
                        APPLYING(REPLACE_جوڙيو_WITH_ڳڻپ_ONES) {
                            DfgSel* ثابت selp
                                = make<DfgSel>(vtxp->fileline(),
                                               Dfg_معلومات_Type::packed(l_ويڪر + r_ويڪر), lVtxp, lsb);
                            Dfg_راس* resp
                                = make<Dfg_ڳڻپ_Ones>(flp, Dfg_معلومات_Type::packed(32), selp);
                            إذا (vtxp->ويڪر() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->ويڪر() - 32U),
                                                       resp);
                            } وإلا إذا (vtxp->ويڪر() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            إذا (extrap) resp = make<Dfg_جوڙيو>(vtxp, resp, extrap);
                            replace(resp);
                            أرجع;
                        }
                    }
                }
            }
        }
    }

    فراغ گهمو(DfgArraySel* ثابت vtxp) override {
        DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>();
        إذا (!idxp) أرجع;
        DfgVarArray* ثابت varp = vtxp->fromp()->cast<DfgVarArray>();
        إذا (!varp) أرجع;
        AstVar* ثابت astVarp = varp->vscp()->varp();
        إذا (astVarp->isForced()) أرجع;
        إذا (astVarp->isSigUserRWPublic()) أرجع;
        Dfg_راس* ثابت srcp = varp->srcp();
        إذا (!srcp) {
            إذا (vtxp->isPacked()) {
                إذا (AstConsArray* ثابت iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    إذا (AstConst* ثابت valp = VN_CAST(iap->valuep(idxp->منزل_سائيز_T()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_جدول) {
                            replace(جديد DfgConst{m_dfg, valp->fileline(), valp->انگ()});
                            أرجع;
                        }
                    }
                }
            }
            أرجع;
        }

        إذا (DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            Dfg_راس* ثابت driverp = splicep->driverAt(idxp->منزل_سائيز_T());
            إذا (!driverp) أرجع;
            DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>();
            إذا (!uap) أرجع;
            إذا (uap->srcp()->is<Dfg_راس_Splice>()) أرجع;
            // If driven by a variable that had a Driver in DFG, it is partial
            إذا (Dfg_راس_Var* ثابت dvarp = uap->srcp()->cast<Dfg_راس_Var>()) {
                إذا (dvarp->srcp()) أرجع;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                أرجع;
            }
        }

        إذا (DfgUnitArray* ثابت uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->منزل_سائيز_T() == 0, vtxp, "Array index out of range");
            إذا (uap->srcp()->is<DfgSplicePacked>()) أرجع;
            // If driven by a variable that had a Driver in DFG, it is partial
            إذا (Dfg_راس_Var* ثابت dvarp = uap->srcp()->cast<Dfg_راس_Var>()) {
                إذا (dvarp->srcp()) أرجع;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgConcat* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isZero(lhsp)) {
            DfgConst* ثابت lConstp = lhsp->as<DfgConst>();
            إذا (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                إذا (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->ويڪر()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_چوٽي_WITH_SHIFTR) {
                        replace(
                            make<Dfg_سرڪايو_R>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->ويڪر())));
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
                            make<Dfg_سرڪايو_L>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->ويڪر())));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgNot* ثابت lNot = lhsp->cast<DfgNot>()) {
            إذا (DfgNot* ثابت rNot = rhsp->cast<DfgNot>()) {
                إذا (!lNot->has_گڻو_Sinks() && !rNot->has_گڻو_Sinks()) {
                    APPLYING(ڌڪو_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ثابت newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        أرجع;
                    }
                }
            }
        }

        {
            منطقي مٽا_سٽا = خطأ;
            إذا (isConcatPushableThroughBitwise(lhsp, rhsp, مٽا_سٽا)) {
                APPLYING(ڌڪو_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, مٽا_سٽا));
                    أرجع;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            إذا (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                Dfg_راس* ثابت rlp = rConcatp->lhsp();
                إذا (!rConcatp->has_گڻو_Sinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, مٽا_سٽا)) {
                    APPLYING(ڌڪو_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        Dfg_راس* ثابت newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, مٽا_سٽا);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            إذا (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                إذا (isSame(lSelp->fromp(), rSelp->fromp())) {
                    إذا (lSelp->lsb() == rSelp->lsb() + rSelp->ويڪر()) {
                        APPLYING(REMOVE_CONCAT_جو_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            أرجع;
                        }
                    }
                }
            }

            إذا (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                إذا (DfgSel* ثابت rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    إذا (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        إذا (lSelp->lsb() == rlSelp->lsb() + rlSelp->ويڪر()) {
                            APPLYING(REPLACE_NESTED_CONCAT_جو_ADJOINING_SELS_ON_LHS) {
                                ثابت uint32_t ويڪر = lSelp->ويڪر() + rlSelp->ويڪر();
                                ثابت Dfg_معلومات_Type& dtype = Dfg_معلومات_Type::packed(ويڪر);
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
                        إذا (lrSelp->lsb() == rSelp->lsb() + rSelp->ويڪر()) {
                            APPLYING(REPLACE_NESTED_CONCAT_جو_ADJOINING_SELS_ON_RHS) {
                                ثابت uint32_t ويڪر = lrSelp->ويڪر() + rSelp->ويڪر();
                                ثابت Dfg_معلومات_Type& dtype = Dfg_معلومات_Type::packed(ويڪر);
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
                Dfg_راس* ثابت rtVtxp = rCondp->thenp();
                Dfg_راس* ثابت reVtxp = rCondp->elsep();
                DfgConst* ثابت rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ثابت reConstp = reVtxp->cast<DfgConst>();
                إذا (!rCondp->has_گڻو_Sinks() && (rtConstp || reConstp)) {
                    APPLYING(ڌڪو_CONCAT_THROUGH_COND_LHS) {
                        Dfg_راس* ثابت thenp = [&]() -> Dfg_راس* {
                            File_سٽ* ثابت rtFlp = rtVtxp->fileline();
                            إذا (rtConstp) {
                                DfgConst* ثابت constp = makeZero(rtFlp, vtxp->ويڪر());
                                constp->انگ().opConcat(lConstp->انگ(), rtConstp->انگ());
                                أرجع constp;
                            }
                            أرجع make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        Dfg_راس* ثابت elsep = [&]() -> Dfg_راس* {
                            File_سٽ* ثابت reFlp = reVtxp->fileline();
                            إذا (reConstp) {
                                DfgConst* ثابت constp = makeZero(reFlp, vtxp->ويڪر());
                                constp->انگ().opConcat(lConstp->انگ(), reConstp->انگ());
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
                Dfg_راس* ثابت ltVtxp = lCondp->thenp();
                Dfg_راس* ثابت leVtxp = lCondp->elsep();
                DfgConst* ثابت ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ثابت leConstp = leVtxp->cast<DfgConst>();
                إذا (!lCondp->has_گڻو_Sinks() && (ltConstp || leConstp)) {
                    APPLYING(ڌڪو_CONCAT_THROUGH_COND_RHS) {
                        Dfg_راس* ثابت thenp = [&]() -> Dfg_راس* {
                            File_سٽ* ثابت ltFlp = ltVtxp->fileline();
                            إذا (ltConstp) {
                                DfgConst* ثابت constp = makeZero(ltFlp, vtxp->ويڪر());
                                constp->انگ().opConcat(ltConstp->انگ(), rConstp->انگ());
                                أرجع constp;
                            }
                            أرجع make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        Dfg_راس* ثابت elsep = [&]() -> Dfg_راس* {
                            File_سٽ* ثابت leFlp = leVtxp->fileline();
                            إذا (leConstp) {
                                DfgConst* ثابت constp = makeZero(leFlp, vtxp->ويڪر());
                                constp->انگ().opConcat(leConstp->انگ(), rConstp->انگ());
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
            ثابت uint32_t vMsb = vtxp->ويڪر() - 1;  // MSB of the concatenation
            ثابت uint32_t lLsb = vtxp->rhsp()->ويڪر();  // LSB of the LHS
            ثابت uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            منطقي hasCrossSink = خطأ;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](Dfg_راس& sink) {
                // Record bits used by DfgSel sinks
                إذا (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    ثابت uint32_t selLsb = selp->lsb();
                    ثابت uint32_t selMsb = selLsb + selp->ويڪر() - 1;
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
                أرجع سچ;
            });
            إذا (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    Dfg_راس* nRhsp = rhsp;
                    إذا (lsb != 0)
                        nRhsp = make<DfgSel>(flp, Dfg_معلومات_Type::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    Dfg_راس* nLhsp = lhsp;
                    إذا (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, Dfg_معلومات_Type::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    Dfg_راس* ثابت catp
                        = make<DfgConcat>(flp, Dfg_معلومات_Type::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ثابت sp = جديد DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_آخري_Id;
                    sp->جوڙيو_Driver(catp, lsb, flp);
                    Dfg_راس_Var* ثابت varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_آخري_Id;
                    varp->vscp()->varp()->is_اندروني(سچ);
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
                    إذا (!rConcatp->has_گڻو_Sinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ثابت uint32_t ويڪر = lRepp->ويڪر() + rlRepp->ويڪر();
                            ثابت Dfg_معلومات_Type& dtype = Dfg_معلومات_Type::packed(ويڪر);
                            DfgRep* ثابت repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            أرجع;
                        }
                    }
                }
            }
        }
    }

    فراغ گهمو(DfgDiv* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        إذا (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            إذا (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    أرجع;
                }
            }
        }
    }

    فراغ گهمو(DfgDivS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        إذا (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            إذا (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    أرجع;
                }
            }
        }
    }

    فراغ گهمو(DfgGt* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgGtS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgGte* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgGteS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgLogAnd* ثابت vtxp) override {
        إذا (ٻٽو(vtxp) || vtxp->rhsp()->unsafe()) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        إذا (lhsp->ويڪر() == 1 && rhsp->ويڪر() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgLogEq* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
    }

    فراغ گهمو(DfgLogIf* ثابت vtxp) override {
        إذا (ٻٽو(vtxp) || vtxp->rhsp()->unsafe()) أرجع;
    }

    فراغ گهمو(DfgLogOr* ثابت vtxp) override {
        إذا (ٻٽو(vtxp) || vtxp->rhsp()->unsafe()) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        إذا (lhsp->ويڪر() == 1 && rhsp->ويڪر() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgLt* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgLtS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgLte* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgLteS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ گهمو(DfgModDiv* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
    }

    فراغ گهمو(DfgModDivS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
    }

    فراغ گهمو(DfgMul* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

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

    فراغ گهمو(DfgMulS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

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

    فراغ گهمو(DfgEq* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }

        إذا (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_ڌڪو_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) أرجع;
            }

            إذا (rhsp->dtype() == m_ٻٽو_انگ_DType) {
                إذا (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_ٻٽو_انگ_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        أرجع;
                    }
                }

                إذا (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_ٻٽو_انگ_1) {
                        replace(rhsp);
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ گهمو(DfgNeq* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }

        إذا (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_ڌڪو_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) أرجع;
            }

            إذا (rhsp->dtype() == m_ٻٽو_انگ_DType) {
                إذا (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_ٻٽو_انگ_0) {
                        replace(rhsp);
                        أرجع;
                    }
                }

                إذا (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_ٻٽو_انگ_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ گهمو(DfgPow* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
    }

    فراغ گهمو(DfgPowSS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
    }

    فراغ گهمو(DfgPowSU* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
    }

    فراغ گهمو(DfgPowUS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
    }

    فراغ گهمو(DfgRep* ثابت vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (DfgConst* ثابت sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ثابت resultp = makeZero(flp, vtxp->ويڪر());
                resultp->انگ().opRepl(sConstp->انگ(), vtxp->ڳڻپ());
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
            إذا (!sRepp->has_گڻو_Sinks() || vtxp->ويڪر() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    أرجع;
                }
            }
        }
    }

    فراغ گهمو(Dfg_سرڪايو_L* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
        إذا (optimize_سرڪايو_RHS(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        إذا (Dfg_سرڪايو_L* ثابت l_سرڪايو_Lp = lhsp->cast<Dfg_سرڪايو_L>()) {
            إذا (!l_سرڪايو_Lp->has_گڻو_Sinks() && rhsp->dtype() == l_سرڪايو_Lp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_سٽ* ثابت flp = vtxp->fileline();
                    ثابت Dfg_معلومات_Type& جوڙ_DType = Dfg_معلومات_Type::packed(rhsp->ويڪر() + 1);
                    Dfg_راس* ثابت bp = make<DfgExtend>(flp, جوڙ_DType, l_سرڪايو_Lp->rhsp());
                    Dfg_راس* ثابت cp = make<DfgExtend>(flp, جوڙ_DType, rhsp);
                    Dfg_جوڙيو* ثابت addp = make<Dfg_جوڙيو>(flp, جوڙ_DType, bp, cp);
                    replace(make<Dfg_سرڪايو_L>(vtxp, l_سرڪايو_Lp->lhsp(), addp));
                    أرجع;
                }
            }
        }

        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرڪايو_Amount = rConstp->منزل_U32();

            إذا (سرڪايو_Amount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    أرجع;
                }
            }

            إذا (سرڪايو_Amount >= vtxp->ويڪر()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->ويڪر()));
                    أرجع;
                }
            }

            إذا (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                إذا (!lConcatp->has_گڻو_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        Dfg_راس* ثابت lRhsp = lConcatp->rhsp();
                        Dfg_راس* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t rW = سرڪايو_Amount;
                        ثابت uint32_t mW = std::min(lRhsp->ويڪر(), vtxp->ويڪر() - سرڪايو_Amount);
                        ثابت uint32_t lW = vtxp->ويڪر() - mW - rW;
                        // Construct the result
                        File_سٽ* ثابت flp = lConcatp->fileline();
                        Dfg_راس* ثابت rp = makeZero(flp, سرڪايو_Amount);
                        Dfg_راس* ثابت mp
                            = make<DfgSel>(flp, Dfg_معلومات_Type::packed(mW), lRhsp, 0U);
                        Dfg_راس* np = make<DfgConcat>(flp, Dfg_معلومات_Type::packed(mW + rW), mp, rp);
                        إذا (!lW) {
                            replace(np);
                            أرجع;
                        }
                        Dfg_راس* ثابت lp
                            = make<DfgSel>(flp, Dfg_معلومات_Type::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        أرجع;
                    }
                }
            }

            إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                إذا (!lSelp->has_گڻو_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ثابت uint32_t n_Sel_ويڪر = lSelp->ويڪر() - سرڪايو_Amount;
                        Dfg_راس* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_معلومات_Type::packed(n_Sel_ويڪر),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), سرڪايو_Amount)));
                        أرجع;
                    }
                }
            }

            إذا (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                إذا (!lCondp->has_گڻو_Sinks()) {
                    APPLYING(ڌڪو_SHIFTL_THROUGH_COND) {
                        Dfg_سرڪايو_L* ثابت tp = make<Dfg_سرڪايو_L>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_سرڪايو_L* ثابت ep = make<Dfg_سرڪايو_L>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ گهمو(Dfg_سرڪايو_R* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
        إذا (optimize_سرڪايو_RHS(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        إذا (Dfg_سرڪايو_R* ثابت l_سرڪايو_Rp = lhsp->cast<Dfg_سرڪايو_R>()) {
            إذا (!l_سرڪايو_Rp->has_گڻو_Sinks() && rhsp->dtype() == l_سرڪايو_Rp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_سٽ* ثابت flp = vtxp->fileline();
                    ثابت Dfg_معلومات_Type& جوڙ_DType = Dfg_معلومات_Type::packed(rhsp->ويڪر() + 1);
                    Dfg_راس* ثابت bp = make<DfgExtend>(flp, جوڙ_DType, l_سرڪايو_Rp->rhsp());
                    Dfg_راس* ثابت cp = make<DfgExtend>(flp, جوڙ_DType, rhsp);
                    Dfg_جوڙيو* ثابت addp = make<Dfg_جوڙيو>(flp, جوڙ_DType, bp, cp);
                    replace(make<Dfg_سرڪايو_R>(vtxp, l_سرڪايو_Rp->lhsp(), addp));
                    أرجع;
                }
            }
        }

        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرڪايو_Amount = rConstp->منزل_U32();

            إذا (سرڪايو_Amount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    أرجع;
                }
            }

            إذا (سرڪايو_Amount >= vtxp->ويڪر()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->ويڪر()));
                    أرجع;
                }
            }

            إذا (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                إذا (!lConcatp->has_گڻو_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        Dfg_راس* ثابت lRhsp = lConcatp->rhsp();
                        Dfg_راس* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t lW = سرڪايو_Amount;
                        ثابت uint32_t mW = std::min(lLhsp->ويڪر(), vtxp->ويڪر() - سرڪايو_Amount);
                        ثابت uint32_t rW = vtxp->ويڪر() - mW - lW;
                        // Construct the result
                        File_سٽ* ثابت flp = lConcatp->fileline();
                        Dfg_راس* ثابت lp = makeZero(flp, سرڪايو_Amount);
                        Dfg_راس* ثابت mp = make<DfgSel>(flp, Dfg_معلومات_Type::packed(mW), lLhsp,
                                                           lLhsp->ويڪر() - mW);
                        Dfg_راس* np = make<DfgConcat>(flp, Dfg_معلومات_Type::packed(lW + mW), lp, mp);
                        إذا (!rW) {
                            replace(np);
                            أرجع;
                        }
                        Dfg_راس* ثابت rp = make<DfgSel>(flp, Dfg_معلومات_Type::packed(rW), lRhsp,
                                                           lRhsp->ويڪر() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        أرجع;
                    }
                }
            }

            إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                إذا (!lSelp->has_گڻو_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ثابت uint32_t n_Sel_ويڪر = lSelp->ويڪر() - سرڪايو_Amount;
                        Dfg_راس* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_معلومات_Type::packed(n_Sel_ويڪر),
                                           lSelp->fromp(), lSelp->lsb() + سرڪايو_Amount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), سرڪايو_Amount), nSelp));
                        أرجع;
                    }
                }
            }

            إذا (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                إذا (!lCondp->has_گڻو_Sinks()) {
                    APPLYING(ڌڪو_SHIFTR_THROUGH_COND) {
                        Dfg_سرڪايو_R* ثابت tp = make<Dfg_سرڪايو_R>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_سرڪايو_R* ثابت ep = make<Dfg_سرڪايو_R>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ گهمو(Dfg_سرڪايو_RS* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;
        إذا (optimize_سرڪايو_RHS(vtxp)) أرجع;
    }

    فراغ گهمو(DfgSub* ثابت vtxp) override {
        إذا (ٻٽو(vtxp)) أرجع;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سٽ* ثابت flp = vtxp->fileline();

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
            إذا (vtxp->dtype() == m_ٻٽو_انگ_DType && rConstp->has_قدر(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_ٻٽو_انگ_DType, lhsp));
                    أرجع;
                }
            }
        }
    }

    فراغ گهمو(Dfg_ميلاپ_Masked* ثابت vtxp) override {
        إذا (DfgConst* ثابت constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ثابت ميلاپ_Varp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), Ast_ميلاپ_Masked::fold(constp->انگ(), ميلاپ_Varp)));
                أرجع;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    فراغ گهمو(DfgCond* ثابت vtxp) override {
        Dfg_راس* ثابت condp = vtxp->condp();
        Dfg_راس* ثابت thenp = vtxp->thenp();
        Dfg_راس* ثابت elsep = vtxp->elsep();
        File_سٽ* ثابت flp = vtxp->fileline();

        إذا (condp->dtype() != m_ٻٽو_انگ_DType) أرجع;

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
            إذا (!condp->has_گڻو_Sinks() || condNotp->has_گڻو_Sinks()) {
                APPLYING(مٽا_سٽا_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    أرجع;
                }
            }
        }

        إذا (DfgNeq* ثابت condNeqp = condp->cast<DfgNeq>()) {
            إذا (!condp->has_گڻو_Sinks()) {
                APPLYING(مٽا_سٽا_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ثابت newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    أرجع;
                }
            }
        }

        إذا (DfgNot* ثابت thenNotp = thenp->cast<DfgNot>()) {
            إذا (DfgNot* ثابت elseNotp = elsep->cast<DfgNot>()) {
                إذا (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->has_گڻو_Sinks() && !elseNotp->has_گڻو_Sinks()) {
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
                إذا (!thenCondp->has_گڻو_Sinks()) {
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

        إذا (vtxp->ويڪر() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            إذا (Dfg_جوڙيو* ثابت thenAddp = thenp->cast<Dfg_جوڙيو>()) {
                إذا (DfgConst* ثابت constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    إذا (constp->has_قدر(1)) {
                        إذا (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->ويڪر() - 1), condp);
                                File_سٽ* ثابت thenFlp = thenAddp->fileline();
                                replace(
                                    make<Dfg_جوڙيو>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                أرجع;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            إذا (DfgSub* ثابت thenSubp = thenp->cast<DfgSub>()) {
                إذا (DfgConst* ثابت constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    إذا (constp->has_قدر(1)) {
                        إذا (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->ويڪر() - 1), condp);
                                File_سٽ* ثابت thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                أرجع;
                            }
                        }
                    }
                }
            }
        }

        إذا (vtxp->dtype() == m_ٻٽو_انگ_DType) {
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

        إذا (vtxp->ويڪر() <= VL_QUADSIZE) {
            إذا (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    Dfg_راس* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    أرجع;
                }
            }
            إذا (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    Dfg_راس* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    أرجع;
                }
            }
            إذا (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    Dfg_راس* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    أرجع;
                }
            }
            إذا (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    Dfg_راس* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    أرجع;
                }
            }

            إذا (DfgOr* ثابت tOrp = thenp->cast<DfgOr>()) {
                إذا (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        Dfg_راس* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        أرجع;
                    }
                }
                إذا (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        Dfg_راس* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        أرجع;
                    }
                }
            }
        }

        إذا (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ثابت std::pair<Dfg_راس*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            إذا (cLSBs.پهريون) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ثابت uint32_t r_ويڪر = cLSBs.ٻيو;
                    Dfg_راس* rhsp = cLSBs.پهريون;
                    إذا (r_ويڪر != rhsp->ويڪر()) {
                        ثابت Dfg_معلومات_Type& rDtype = Dfg_معلومات_Type::packed(r_ويڪر);
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
                    ثابت uint32_t l_ويڪر = vtxp->ويڪر() - r_ويڪر;
                    ثابت Dfg_معلومات_Type& lDtype = Dfg_معلومات_Type::packed(l_ويڪر);
                    Dfg_راس* ثابت lThenp = make<DfgSel>(flp, lDtype, thenp, r_ويڪر);
                    Dfg_راس* ثابت lElsep = make<DfgSel>(flp, lDtype, elsep, r_ويڪر);
                    Dfg_راس* ثابت lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    أرجع;
                }
            }

            ثابت std::pair<Dfg_راس*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            إذا (cMSBs.پهريون) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ثابت uint32_t l_ويڪر = cMSBs.ٻيو;
                    Dfg_راس* lhsp = cMSBs.پهريون;
                    إذا (l_ويڪر != lhsp->ويڪر()) {
                        ثابت Dfg_معلومات_Type& lDtype = Dfg_معلومات_Type::packed(l_ويڪر);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->ويڪر() - l_ويڪر);
                    }
                    // If it's all the same, just replace
                    إذا (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        أرجع;
                    }
                    // Create new RHS
                    ثابت uint32_t r_ويڪر = vtxp->ويڪر() - l_ويڪر;
                    ثابت Dfg_معلومات_Type& rDtype = Dfg_معلومات_Type::packed(r_ويڪر);
                    Dfg_راس* ثابت rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    Dfg_راس* ثابت rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    Dfg_راس* ثابت rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    أرجع;
                }
            }
        }

        إذا (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                Dfg_راس* resp = condp;
                إذا (ثابت uint32_t extend = vtxp->ويڪر() - 1) {
                    DfgConst* ثابت zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                أرجع;
            }
        }

        إذا (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                Dfg_راس* resp = make<DfgNot>(condp, condp);
                إذا (ثابت uint32_t extend = vtxp->ويڪر() - 1) {
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

    فراغ گهمو(Dfg_راس_Var* ثابت vtxp) override {
        إذا (vtxp->hasSinks()) أرجع;
        إذا (vtxp->isObserved()) أرجع;
        إذا (vtxp->defaultp()) أرجع;

        // If undriven, or driven from another var, it is completely redundant.
        إذا (!vtxp->srcp() || vtxp->srcp()->is<Dfg_راس_Var>()) {
            APPLYING(REMOVE_VAR) {
                delete_راس(vtxp);
                أرجع;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        منطقي مليو_One = خطأ;
        ثابت منطقي keep = vtxp->srcp()->foreachSink([&](Dfg_راس& sink) -> منطقي {
            // Ignore non-observable variable sinks. These can be eliminated.
            إذا (ثابت Dfg_راس_Var* ثابت varp = sink.cast<Dfg_راس_Var>()) {
                إذا (!varp->hasSinks() && !varp->isObserved()) أرجع خطأ;
            }
            // Keep if found more than one sink
            إذا (مليو_One) أرجع سچ;
            // Mark first sink found
            مليو_One = سچ;
            أرجع خطأ;
        });
        إذا (!keep) {
            APPLYING(REMOVE_VAR) {
                delete_راس(vtxp);
                أرجع;
            }
        }
    }

    V3DfgPeephole(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.for_Each_راس([&](Dfg_راس& vtx) { m_vInfo[vtx].m_id = ++m_آخري_Id; });

        // Add all operation vertices to the cache
        لكل (Dfg_راس& vtx : m_dfg.opVertices()) cache_راس(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.سائيز() * 2);
        m_iterList.reserve(m_dfg.سائيز() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ڌڪو_پٺ(خالي_اشارو);
        m_iterList.ڌڪو_پٺ(خالي_اشارو);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        لكل (Dfg_راس_Var& vtx : m_dfg.varVertices()) جوڙيو_منزل_Work_List(&vtx);

        // Add all operation vertices to the work list
        لكل (Dfg_راس& vtx : m_dfg.opVertices()) جوڙيو_منزل_Work_List(&vtx);

        // Process iteratively
        طالما (سچ) {
            // Process the work list - keep the placeholder at index 0
            طالما (m_workList.سائيز() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.پٺ();
                m_workList.ڪڍو_پٺ();
                إذا (!m_vtxp) استمر;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_work_List_اشاريو = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                إذا (Dfg_راس_Var* ثابت varp = m_vtxp->cast<Dfg_راس_Var>()) {
                    گهمو(varp);
                    استمر;
                }

                // Ast references are not considered
                إذا (m_vtxp->is<Dfg_راس_Ast>()) استمر;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                إذا (!m_vInfo[m_vtxp].m_is_Cached_راس) {
                    إذا (Dfg_راس* ثابت sampep = cache_راس(m_vtxp)) {
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
            إذا (m_iterList.سائيز() == 1) اكسر;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                سائيز_t begin = 0;
                سائيز_t پڇاڙي = m_iterList.سائيز();
                لكل (سائيز_t hops = 1; hops <= 4; ++hops) {
                    لكل (سائيز_t i = begin; i < پڇاڙي; ++i) {
                        Dfg_راس* ثابت vtxp = m_iterList[i];
                        إذا (!vtxp) استمر;
                        vtxp->foreachSink([&](Dfg_راس& dst) {
                            جوڙيو_منزل_Iter_List(&dst);
                            أرجع خطأ;
                        });
                        vtxp->foreach_ذريعو([&](Dfg_راس& src) {
                            جوڙيو_منزل_Iter_List(&src);
                            أرجع خطأ;
                        });
                    }
                    begin = پڇاڙي;
                    پڇاڙي = m_iterList.سائيز();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            لكل (Dfg_راس* ثابت vtxp : m_iterList) {
                إذا (!vtxp) استمر;
                removeFromIterList(vtxp);
                جوڙيو_منزل_Work_List(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#وصف_رد APPLYING

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

فراغ V3DfgPasses::peephole(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx) {
    إذا (!v3Global.opt.fDfgPeephole()) أرجع;
    V3DfgPeephole::apply(dfg, ctx);
}
