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

قالب <نام_نوع T_Reduction>
بنية Reduction_مقصد_Bitwise_Impl {};
قالب <> بنية Reduction_مقصد_Bitwise_Impl<DfgRedAnd> { استفاده type = DfgAnd; };
قالب <> بنية Reduction_مقصد_Bitwise_Impl<DfgRedOr>  { استفاده type = DfgOr;  };
قالب <> بنية Reduction_مقصد_Bitwise_Impl<DfgRedXor> { استفاده type = DfgXor; };
قالب <نام_نوع T_Reduction>
استفاده Reduction_مقصد_Bitwise = نام_نوع Reduction_مقصد_Bitwise_Impl<T_Reduction>::type;

قالب <نام_نوع T_Bitwise>
بنية Bitwise_مقصد_Reduction_Impl {};
قالب <> بنية Bitwise_مقصد_Reduction_Impl<DfgAnd> { استفاده type = DfgRedAnd; };
قالب <> بنية Bitwise_مقصد_Reduction_Impl<DfgOr>  { استفاده type = DfgRedOr;  };
قالب <> بنية Bitwise_مقصد_Reduction_Impl<DfgXor> { استفاده type = DfgRedXor; };
قالب <نام_نوع T_Reduction>
استفاده Bitwise_مقصد_Reduction = نام_نوع Bitwise_مقصد_Reduction_Impl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
قالب<نام_نوع رأس_2>
بنية IsAssociative : عام std::false_type {};
قالب <> بنية IsAssociative<DfgAnd> : عام std::true_type {};
قالب <> بنية IsAssociative<DfgOr>  : عام std::true_type {};
قالب <> بنية IsAssociative<DfgXor> : عام std::true_type {};
قالب <> بنية IsAssociative<Dfg_جمع_کن> : عام std::true_type {};
قالب <> بنية IsAssociative<DfgConcat> : عام std::true_type {};
قالب <> بنية IsAssociative<DfgMul> : عام std::true_type {};
قالب <> بنية IsAssociative<DfgMulS> : عام std::true_type {};

// Commutative binary operators (a op b == b op a)
قالب<نام_نوع رأس_2>
بنية IsCommutative : عام std::false_type {};
قالب <> بنية IsCommutative<DfgAnd> : عام std::true_type {};
قالب <> بنية IsCommutative<DfgOr>  : عام std::true_type {};
قالب <> بنية IsCommutative<DfgXor> : عام std::true_type {};
قالب <> بنية IsCommutative<Dfg_جمع_کن> : عام std::true_type {};
قالب <> بنية IsCommutative<DfgEq> : عام std::true_type {};
قالب <> بنية IsCommutative<DfgMul> : عام std::true_type {};
قالب <> بنية IsCommutative<DfgMulS> : عام std::true_type {};
قالب <> بنية IsCommutative<DfgNeq> : عام std::true_type {};

// Idempotent binary operators (a op a == a)
قالب<نام_نوع رأس_2>
بنية IsIdempotent : عام std::false_type {};
قالب <> بنية IsIdempotent<DfgAnd> : عام std::true_type {};
قالب <> بنية IsIdempotent<DfgOr>  : عام std::true_type {};

// Binary result dtype
قالب<نام_نوع رأس_2>
ثابت Dfg_داده_Type& نتیجه_DType(ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp);
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<Dfg_جمع_کن>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgAnd>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgConcat>     (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(lhsp->عرض() + rhsp->عرض()); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgDiv>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgDivS>       (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgEq>         (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgGt>         (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgGtS>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgGte>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgGteS>       (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgLogAnd>     (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgLogEq>      (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgLogIf>      (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgLogOr>      (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgLt>         (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgLtS>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgLte>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgLteS>       (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgModDiv>     (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgModDivS>    (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgMul>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgMulS>       (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgNeq>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع Dfg_داده_Type::packed(1); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgOr>         (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgPow>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgPowSS>      (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgPowSU>      (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgPowUS>      (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<Dfg_لغزش_L>     (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<Dfg_لغزش_R>     (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<Dfg_لغزش_RS>    (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgSub>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }
قالب <> ثابت Dfg_داده_Type& نتیجه_DType<DfgXor>        (ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) { أرجع lhsp->dtype(); }

// Unary constant folding
قالب<نام_نوع رأس_2> فراغ foldOp(V3_عدد& بیرون, ثابت V3_عدد& src);
قالب <> فراغ foldOp<Dfg_شمار_Ones>  (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.op_شمار_Ones(src); }
قالب <> فراغ foldOp<DfgExtend>     (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.op_تخصیص_بده(src); }
قالب <> فراغ foldOp<DfgExtendS>    (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opExtendS(src, src.عرض()); }
قالب <> فراغ foldOp<DfgLogNot>     (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opLogNot(src); }
قالب <> فراغ foldOp<DfgNegate>     (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opNegate(src); }
قالب <> فراغ foldOp<DfgNot>        (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opNot(src); }
قالب <> فراغ foldOp<DfgOneHot>     (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opOneHot(src); }
قالب <> فراغ foldOp<DfgOneHot0>    (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opOneHot0(src); }
قالب <> فراغ foldOp<DfgRedAnd>     (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opRedAnd(src); }
قالب <> فراغ foldOp<DfgRedOr>      (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opRedOr(src); }
قالب <> فراغ foldOp<DfgRedXor>     (V3_عدد& بیرون, ثابت V3_عدد& src) { بیرون.opRedXor(src); }

// Binary constant folding
قالب<نام_نوع رأس_2> فراغ foldOp(V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
قالب <> فراغ foldOp<Dfg_جمع_کن>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.op_جمع_کن(lhs, rhs); }
قالب <> فراغ foldOp<DfgAnd>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opAnd(lhs, rhs); }
قالب <> فراغ foldOp<DfgConcat>     (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opConcat(lhs, rhs); }
قالب <> فراغ foldOp<DfgDiv>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opDiv(lhs, rhs); }
قالب <> فراغ foldOp<DfgDivS>       (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opDivS(lhs, rhs); }
قالب <> فراغ foldOp<DfgEq>         (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opEq(lhs, rhs); }
قالب <> فراغ foldOp<DfgGt>         (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opGt(lhs, rhs); }
قالب <> فراغ foldOp<DfgGtS>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opGtS(lhs, rhs); }
قالب <> فراغ foldOp<DfgGte>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opGte(lhs, rhs); }
قالب <> فراغ foldOp<DfgGteS>       (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opGteS(lhs, rhs); }
قالب <> فراغ foldOp<DfgLogAnd>     (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opLogAnd(lhs, rhs); }
قالب <> فراغ foldOp<DfgLogEq>      (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opLogEq(lhs, rhs); }
قالب <> فراغ foldOp<DfgLogIf>      (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opLogIf(lhs, rhs); }
قالب <> فراغ foldOp<DfgLogOr>      (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opLogOr(lhs, rhs); }
قالب <> فراغ foldOp<DfgLt>         (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opLt(lhs, rhs); }
قالب <> فراغ foldOp<DfgLtS>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opLtS(lhs, rhs); }
قالب <> فراغ foldOp<DfgLte>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opLte(lhs, rhs); }
قالب <> فراغ foldOp<DfgLteS>       (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opLteS(lhs, rhs); }
قالب <> فراغ foldOp<DfgModDiv>     (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opModDiv(lhs, rhs); }
قالب <> فراغ foldOp<DfgModDivS>    (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opModDivS(lhs, rhs); }
قالب <> فراغ foldOp<DfgMul>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opMul(lhs, rhs); }
قالب <> فراغ foldOp<DfgMulS>       (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opMulS(lhs, rhs); }
قالب <> فراغ foldOp<DfgNeq>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opNeq(lhs, rhs); }
قالب <> فراغ foldOp<DfgOr>         (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opOr(lhs, rhs); }
قالب <> فراغ foldOp<DfgPow>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opPow(lhs, rhs); }
قالب <> فراغ foldOp<DfgPowSS>      (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opPowSS(lhs, rhs); }
قالب <> فراغ foldOp<DfgPowSU>      (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opPowSU(lhs, rhs); }
قالب <> فراغ foldOp<DfgPowUS>      (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opPowUS(lhs, rhs); }
قالب <> فراغ foldOp<Dfg_لغزش_L>     (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.op_لغزش_L(lhs, rhs); }
قالب <> فراغ foldOp<Dfg_لغزش_R>     (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.op_لغزش_R(lhs, rhs); }
قالب <> فراغ foldOp<Dfg_لغزش_RS>    (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.op_لغزش_RS(lhs, rhs, lhs.عرض()); }
قالب <> فراغ foldOp<DfgSub>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opSub(lhs, rhs); }
قالب <> فراغ foldOp<DfgXor>        (V3_عدد& بیرون, ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { بیرون.opXor(lhs, rhs); }
}
// clang-format on

صنف V3DfgPeephole final : عام DfgVisitor {
    // TYPES
    بنية رأس_Info final {
        اندازه_t m_work_List_شاخص = 0;  // Position of this vertx m_workList (0 means not in list)
        اندازه_t m_iter_List_شاخص = 0;  // Position of this vertx m_iterList (0 means not in list)
        اندازه_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        اندازه_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        منطقي m_is_Cached_رأس = خطأ;  // This vertex is the vertex cached for its operation
    };

    // STATE
    Dfg_گراف& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ثابت Dfg_داده_Type& m_بیت_DType = Dfg_داده_Type::packed(1);  // Common, so grab it up front
    std::وکتور<Dfg_رأس*> m_workList;  // List of vertices processed in current interation
    std::وکتور<Dfg_رأس*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<رأس_Info> m_vInfo = m_dfg.makeUserMap<رأس_Info>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    Dfg_رأس* m_vtxp = اشاره_تهی;  // Currently considered vertex
    اندازه_t m_currentGeneration = 0;  // Current generation number
    اندازه_t m_آخرین_Id = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ثابت m_tmpScopep = v3Global.rootp()->بالاترین_Scopep()->scopep();

    // STATIC STATE
    ساكن V3DebugBisect s_debugBisect;  // Debug aid

#تعریف_کن APPLYING(id) إذا (checkApplying(VDfg_Peephole_الگو::id))

    // METHODS
    منطقي checkApplying(VDfg_Peephole_الگو id) {
        إذا (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) أرجع خطأ;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_شمار[id];
        أرجع درست;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    Dfg_رأس* cache_رأس(Dfg_رأس* vtxp) {
        Dfg_رأس* ثابت equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_is_Cached_رأس || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_is_Cached_رأس = !equivp;
        أرجع equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    فراغ invalidate_رأس(Dfg_رأس* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_is_Cached_رأس = خطأ;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    قالب <نام_نوع رأس_2, نام_نوع... Operands>
    رأس_2* get_Or_Create_رأس(File_سطر* flp, ثابت Dfg_داده_Type& dtype, Operands... operands) {
        رأس_2* ثابت vtxp = m_cache.getOrCreate<رأس_2, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_is_Cached_رأس = درست;
        أرجع vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    قالب <نام_نوع رأس_2, نام_نوع... Operands>
    رأس_2* get_رأس(ثابت Dfg_داده_Type& dtype, Operands... operands) {
        رأس_2* ثابت vtxp = m_cache.get<رأس_2>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_is_Cached_رأس, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        أرجع vtxp;
    }

    فراغ incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    فراغ جمع_کن_مقصد_Work_List(Dfg_رأس* vtxp) {
        رأس_Info& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        إذا (vInfo.m_work_List_شاخص) أرجع;
        // Add to work list
        vInfo.m_work_List_شاخص = m_workList.اندازه();
        m_workList.افزودن_پشت(vtxp);
    }

    فراغ removeFromWorkList(Dfg_رأس* vtxp) {
        رأس_Info& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_work_List_شاخص] = اشاره_تهی;
        vInfo.m_work_List_شاخص = 0;
    }

    فراغ جمع_کن_مقصد_Iter_List(Dfg_رأس* vtxp) {
        رأس_Info& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        إذا (vInfo.m_iter_List_شاخص) أرجع;
        // Add to iter list
        vInfo.m_iter_List_شاخص = m_iterList.اندازه();
        m_iterList.افزودن_پشت(vtxp);
    }

    فراغ removeFromIterList(Dfg_رأس* vtxp) {
        رأس_Info& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iter_List_شاخص] = اشاره_تهی;
        vInfo.m_iter_List_شاخص = 0;
    }

    فراغ delete_رأس(Dfg_رأس* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_work_List_شاخص, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ثابت Dfg_رأس_Var* ثابت varp = vtxp->cast<Dfg_رأس_Var>();
        UASSERT_OBJ(!varp || !varp->hasPrev(), vtxp, "Deleting variable consumed via DfgPrev");

        // Invalidate cache entry
        invalidate_رأس(vtxp);

        // It might be in the iter list, remove it
        removeFromIterList(vtxp);

        // Gather source vertices - they might be duplicates, make unique using generation number
        incrementGeneration();
        std::وکتور<Dfg_رأس*> srcps;
        srcps.reserve(vtxp->nInputs());
        vtxp->foreach_منبع([&](Dfg_رأس& src) {
            // If it's a variable, add to work list to see if it became redundant
            إذا (src.is<Dfg_رأس_Var>()) {
                جمع_کن_مقصد_Work_List(&src);
                أرجع خطأ;
            }
            // Gather unique sources
            رأس_Info& vInfo = m_vInfo[src];
            إذا (vInfo.m_generation != m_currentGeneration) srcps.افزودن_پشت(&src);
            vInfo.m_generation = m_currentGeneration;
            أرجع خطأ;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        إذا (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.افزودن_پشت(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } وإلا {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ثابت خودکار mid
            = std::stable_افراز(srcps.begin(), srcps.انتها(), [](Dfg_رأس* srcp) {  //
                  أرجع srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        لكل (خودکار it = srcps.begin(); it != mid; ++it) جمع_کن_مقصد_Iter_List(*it);

        // Recursively delete unused sources
        لكل (خودکار it = mid; it != srcps.انتها(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            delete_رأس(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    فراغ replace(Dfg_رأس* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_work_List_شاخص, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ثابت خودکار debugCallback = [&]() -> فراغ {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.منبع_Cone({m_vtxp, resp});
            ثابت خودکار cone = m_dfg.منبع_Cone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ثابت Dfg_رأس& v) {  //
                أرجع cone->شمار(&v);
            });
        };
        إذا (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) أرجع;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreach_منبع([&](Dfg_رأس& src) {
            جمع_کن_مقصد_Iter_List(&src);
            أرجع خطأ;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](Dfg_رأس& dst) {
            invalidate_رأس(&dst);
            أرجع خطأ;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](Dfg_رأس& dst) {
            cache_رأس(&dst);
            أرجع خطأ;
        });

        // Original vertex is now unused, so delete it
        delete_رأس(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        جمع_کن_مقصد_Iter_List(resp);
        // Add new vertex and sinks to work list as likely to match
        جمع_کن_مقصد_Work_List(resp);
        resp->foreachSink([&](Dfg_رأس& dst) {
            جمع_کن_مقصد_Work_List(&dst);
            أرجع خطأ;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_سطر* flp, uint32_t val) { أرجع جديد DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_سطر* flp, uint32_t عرض) {
        أرجع جديد DfgConst{m_dfg, flp, عرض, 0};
    }
    DfgConst* makeZero(File_سطر* flp, ثابت Dfg_داده_Type& dtype) {
        أرجع makeZero(flp, dtype.اندازه());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_سطر* flp, uint32_t عرض) {
        DfgConst* ثابت resp = makeZero(flp, عرض);
        resp->رقم().مجموعه_All_Bits1();
        أرجع resp;
    }

    // Create a new vertex of the given type
    قالب <نام_نوع رأس_2, نام_نوع... Operands>
    رأس_2* make(File_سطر* flp, ثابت Dfg_داده_Type& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        رأس_2* ثابت vtxp = get_Or_Create_رأس<رأس_2, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        رأس_Info& vInfo = m_vInfo[vtxp];
        إذا (!vInfo.m_id) vInfo.m_id = ++m_آخرین_Id;
        // Add to work list
        جمع_کن_مقصد_Work_List(vtxp);
        // Return new node
        أرجع vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    قالب <نام_نوع رأس_2, نام_نوع... Operands>
    رأس_2* make(ثابت Dfg_رأس* examplep, Operands... operands) {
        أرجع make<رأس_2>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    Dfg_رأس* replicate(Dfg_رأس* vtxp, Dfg_رأس* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_بیت_DType, vtxp, "Expected 1-bit vertex");
        إذا (vtxp->dtype() == m_بیت_DType) أرجع bitp;
        أرجع make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ساكن منطقي isSame(ثابت Dfg_رأس* ap, ثابت Dfg_رأس* bp) {
        إذا (ap == bp) أرجع درست;
        إذا (ap->dtype() != bp->dtype()) أرجع خطأ;
        ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>();
        إذا (!aConstp) أرجع خطأ;
        ثابت DfgConst* ثابت bConstp = bp->cast<DfgConst>();
        إذا (!bConstp) أرجع خطأ;
        أرجع aConstp->رقم().isCaseEq(bConstp->رقم());
    }

    ساكن منطقي isZero(ثابت Dfg_رأس* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->isZero();
        أرجع خطأ;
    }

    ساكن منطقي isOnes(ثابت Dfg_رأس* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->isOnes();
        أرجع خطأ;
    }

    ساكن منطقي isEqOne(ثابت Dfg_رأس* vtxp) {
        إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->رقم().isEqOne();
        أرجع خطأ;
    }

    ساكن منطقي areAdjacent(uint32_t& lsb, ثابت DfgSel* lSelp, ثابت DfgSel* rSelp) {
        إذا (!isSame(lSelp->srcp(), rSelp->srcp())) أرجع خطأ;
        إذا (lSelp->lsb() + lSelp->عرض() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            أرجع درست;
        }
        إذا (lSelp->lsb() == rSelp->lsb() + rSelp->عرض()) {
            lsb = rSelp->lsb();
            أرجع درست;
        }
        أرجع خطأ;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    قالب <نام_نوع رأس_2>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي foldUnary(رأس_2* ثابت vtxp) {
        static_assert(std::is_پایه_از<Dfg_رأس_Unary, رأس_2>::مقدار, "Must invoke on unary");
        static_assert(std::is_final<رأس_2>::مقدار, "Must invoke on final class");
        إذا (DfgConst* ثابت srcp = vtxp->srcp()->قالب cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ثابت resultp = makeZero(vtxp->fileline(), vtxp->عرض());
                foldOp<رأس_2>(resultp->رقم(), srcp->رقم());
                replace(resultp);
                أرجع درست;
            }
        }
        أرجع خطأ;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    قالب <نام_نوع رأس_2>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي دودویی(رأس_2* ثابت vtxp) {
        static_assert(std::is_پایه_از<Dfg_رأس_دودویی, رأس_2>::مقدار, "Must invoke on binary");
        static_assert(std::is_final<رأس_2>::مقدار, "Must invoke on final class");

        File_سطر* ثابت flp = vtxp->fileline();
        // LHS/RHS of the vertex
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ثابت lConstp = lhsp->cast<DfgConst>();
        DfgConst* ثابت rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        رأس_2* ثابت lSamep = lhsp->cast<رأس_2>();
        رأس_2* ثابت rSamep = rhsp->cast<رأس_2>();

        // Constant folding
        إذا (lConstp && rConstp) {
            APPLYING(FOLD_دودویی) {
                DfgConst* ثابت resultp = makeZero(flp, نتیجه_DType<رأس_2>(lhsp, rhsp));
                foldOp<رأس_2>(resultp->رقم(), lConstp->رقم(), rConstp->رقم());
                replace(resultp);
                أرجع درست;
            }
        }

        // Idempotent
        إذا VL_CONSTEXPR_CXX17 (IsIdempotent<رأس_2>::مقدار) {
            إذا (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_دودویی) {
                    replace(lhsp);
                    أرجع درست;
                }
            }
        }

        // Associative
        إذا VL_CONSTEXPR_CXX17 (IsAssociative<رأس_2>::مقدار) {
            // Also commutative and idempotent
            إذا VL_CONSTEXPR_CXX17 (IsCommutative<رأس_2>::مقدار && IsIdempotent<رأس_2>::مقدار) {
                إذا (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_دودویی_LHS) {
                        replace(lSamep);
                        أرجع درست;
                    }
                }
                إذا (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_دودویی_RHS) {
                        replace(rSamep);
                        أرجع درست;
                    }
                }
            }

            إذا (lConstp && rSamep) {
                إذا (DfgConst* ثابت rlConstp = rSamep->lhsp()->قالب cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_دودویی_LHS_از_RHS) {
                        DfgConst* ثابت cp = makeZero(flp, نتیجه_DType<رأس_2>(lConstp, rlConstp));
                        foldOp<رأس_2>(cp->رقم(), lConstp->رقم(), rlConstp->رقم());
                        replace(make<رأس_2>(vtxp, cp, rSamep->rhsp()));
                        أرجع درست;
                    }
                }
            }

            إذا (lSamep && rConstp) {
                إذا (DfgConst* ثابت lrConstp = lSamep->rhsp()->قالب cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_دودویی_RHS_از_LHS) {
                        DfgConst* ثابت cp = makeZero(flp, نتیجه_DType<رأس_2>(lrConstp, rConstp));
                        foldOp<رأس_2>(cp->رقم(), lrConstp->رقم(), rConstp->رقم());
                        replace(make<رأس_2>(vtxp, lSamep->lhsp(), cp));
                        أرجع درست;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            إذا (lSamep && !lSamep->has_مضرب_Sinks()) {
                Dfg_رأس* ثابت ap = lSamep->lhsp();
                Dfg_رأس* ثابت bp = lSamep->rhsp();
                Dfg_رأس* ثابت cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                إذا (ap != cp && bp != cp) {
                    APPLYING(راست_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ثابت Dfg_داده_Type& فرزند_DType = نتیجه_DType<رأس_2>(bp, cp);
                        رأس_2* ثابت bcp = make<رأس_2>(vtxp->fileline(), فرزند_DType, bp, cp);
                        replace(make<رأس_2>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        أرجع درست;
                    }
                }
            }

            إذا (rSamep && !rSamep->has_مضرب_Sinks()) {
                Dfg_رأس* ثابت rlVtxp = rSamep->lhsp();
                Dfg_رأس* ثابت rrVtxp = rSamep->rhsp();

                إذا VL_CONSTEXPR_CXX17 (IsCommutative<رأس_2>::مقدار) {
                    إذا (!lhsp->has_مضرب_Sinks() && rlVtxp->has_مضرب_Sinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<رأس_2>(vtxp, rlVtxp, make<رأس_2>(vtxp, lhsp, rrVtxp)));
                            أرجع درست;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                إذا (رأس_2* ثابت existingp
                    = get_رأس<رأس_2>(نتیجه_DType<رأس_2>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    إذا (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_دودویی_LHS_WITH_LHS_از_RHS) {
                            replace(make<رأس_2>(vtxp, existingp, rrVtxp));
                            أرجع درست;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                إذا VL_CONSTEXPR_CXX17 (IsCommutative<رأس_2>::مقدار) {
                    إذا (رأس_2* ثابت existingp
                        = get_رأس<رأس_2>(نتیجه_DType<رأس_2>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        إذا (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_دودویی_LHS_WITH_RHS_از_RHS) {
                                replace(make<رأس_2>(vtxp, existingp, rlVtxp));
                                أرجع درست;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        إذا VL_CONSTEXPR_CXX17 (IsCommutative<رأس_2>::مقدار) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            إذا (lConstp) {
                إذا (DfgCond* ثابت rCondp = rhsp->cast<DfgCond>()) {
                    إذا (!rCondp->has_مضرب_Sinks()) {
                        APPLYING(افزودن_COMMUTATIVE_دودویی_THROUGH_COND) {
                            Dfg_رأس* ثابت tp = make<رأس_2>(vtxp, lConstp, rCondp->thenp());
                            Dfg_رأس* ثابت ep = make<رأس_2>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            أرجع درست;
                        }
                    }
                }
                أرجع خطأ;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ثابت منطقي جابجا_Sides = [&]() {
            // Don't swap if not commutative, duh
            إذا VL_CONSTEXPR_CXX17 (!IsCommutative<رأس_2>::مقدار) أرجع خطأ;
            // Const go on left
            إذا (lConstp) أرجع خطأ;
            إذا (rConstp) أرجع درست;
            // Not go on left
            إذا (lhsp->is<DfgNot>()) أرجع خطأ;
            إذا (rhsp->is<DfgNot>()) أرجع درست;
            // Same go on right
            إذا (rSamep) أرجع خطأ;
            إذا (lSamep) أرجع درست;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ثابت رأس_Info& lInfo = m_vInfo[lhsp];
            ثابت رأس_Info& rInfo = m_vInfo[rhsp];
            أرجع lInfo.m_id > rInfo.m_id;
        }();
        إذا (جابجا_Sides) {
            APPLYING(جابجا_SIDES_درون_دودویی) {
                replace(make<رأس_2>(vtxp, rhsp, lhsp));
                أرجع درست;
            }
        }

        أرجع خطأ;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    قالب <نام_نوع Other, نام_نوع رأس_2>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي distributive_And_Associative_دودویی(رأس_2* vtxp) {
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        إذا (!lhsp->has_مضرب_Sinks() && !rhsp->has_مضرب_Sinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            إذا (Other* ثابت lp = lhsp->cast<Other>()) {
                إذا (Other* ثابت rp = rhsp->cast<Other>()) {
                    Dfg_رأس* ثابت llp = lp->lhsp();
                    Dfg_رأس* ثابت lrp = lp->rhsp();
                    Dfg_رأس* ثابت rlp = rp->lhsp();
                    Dfg_رأس* ثابت rrp = rp->rhsp();
                    Dfg_رأس* ap = اشاره_تهی;
                    Dfg_رأس* bp = اشاره_تهی;
                    Dfg_رأس* cp = اشاره_تهی;
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
                        APPLYING(REPLACE_DISTRIBUTIVE_دودویی) {
                            replace(make<Other>(vtxp, ap, make<رأس_2>(lhsp, bp, cp)));
                            أرجع درست;
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    قالب <نام_نوع رأس_2>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي
    try_افزودن_Bitwise_Op_Through_Concat(رأس_2* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_سطر* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ثابت catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ثابت catRConstp = concatp->rhsp()->cast<DfgConst>();
        إذا (catLConstp || catRConstp) {
            APPLYING(افزودن_BITWISE_OP_THROUGH_CONCAT) {
                ثابت uint32_t عرض = concatp->عرض();
                ثابت Dfg_داده_Type& lDtype = concatp->lhsp()->dtype();
                ثابت Dfg_داده_Type& rDtype = concatp->rhsp()->dtype();
                ثابت uint32_t l_عرض = lDtype.اندازه();
                ثابت uint32_t r_عرض = rDtype.اندازه();

                // The new Lhs vertex
                Dfg_رأس* ثابت newLhsp = [&]() -> Dfg_رأس* {
                    DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), l_عرض);
                    إذا (catLConstp) {
                        V3_عدد رقم{constp->fileline(), static_cast<صحيح>(l_عرض), 0u};
                        رقم.opSel(constp->رقم(), عرض - 1, r_عرض);
                        foldOp<رأس_2>(newLhsConstp->رقم(), رقم, catLConstp->رقم());
                        أرجع newLhsConstp;
                    }
                    newLhsConstp->رقم().opSel(constp->رقم(), عرض - 1, r_عرض);
                    أرجع make<رأس_2>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                Dfg_رأس* ثابت newRhsp = [&]() -> Dfg_رأس* {
                    DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), r_عرض);
                    إذا (catRConstp) {
                        V3_عدد رقم{constp->fileline(), static_cast<صحيح>(r_عرض), 0u};
                        رقم.opSel(constp->رقم(), r_عرض - 1, 0);
                        foldOp<رأس_2>(newRhsConstp->رقم(), رقم, catRConstp->رقم());
                        أرجع newRhsConstp;
                    }
                    newRhsConstp->رقم().opSel(constp->رقم(), r_عرض - 1, 0);
                    أرجع make<رأس_2>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                أرجع درست;
            }
        }
        أرجع خطأ;
    }

    قالب <نام_نوع رأس_2>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي
    try_افزودن_Compare_Op_Through_Concat(رأس_2* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_سطر* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        إذا (!concatp->has_مضرب_Sinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(افزودن_COMPARE_OP_THROUGH_CONCAT) {
                ثابت uint32_t عرض = concatp->عرض();
                ثابت uint32_t l_عرض = concatp->lhsp()->عرض();
                ثابت uint32_t r_عرض = concatp->rhsp()->عرض();

                // The new Lhs vertex
                DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), l_عرض);
                newLhsConstp->رقم().opSel(constp->رقم(), عرض - 1, r_عرض);
                رأس_2* ثابت newLhsp
                    = make<رأس_2>(flp, m_بیت_DType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), r_عرض);
                newRhsConstp->رقم().opSel(constp->رقم(), r_عرض - 1, 0);
                رأس_2* ثابت newRhsp
                    = make<رأس_2>(flp, m_بیت_DType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                إذا VL_CONSTEXPR_CXX17 (std::is_same<رأس_2, DfgEq>::مقدار) {
                    replace(make<DfgAnd>(concatp->fileline(), m_بیت_DType, newLhsp, newRhsp));
                } وإلا إذا VL_CONSTEXPR_CXX17 (std::is_same<رأس_2, DfgNeq>::مقدار) {
                    replace(make<DfgOr>(concatp->fileline(), m_بیت_DType, newLhsp, newRhsp));
                } وإلا {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                أرجع درست;
            }
        }
        أرجع خطأ;
    }

    قالب <نام_نوع Bitwise>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي try_افزودن_Bitwise_Op_Through_Reductions(Bitwise* ثابت vtxp) {
        استفاده Reduction = Bitwise_مقصد_Reduction<Bitwise>;

        إذا (Reduction* ثابت lRedp = vtxp->lhsp()->قالب cast<Reduction>()) {
            إذا (Reduction* ثابت rRedp = vtxp->rhsp()->قالب cast<Reduction>()) {
                Dfg_رأس* ثابت lSrcp = lRedp->srcp();
                Dfg_رأس* ثابت rSrcp = rRedp->srcp();
                إذا (lSrcp->dtype() == rSrcp->dtype() && lSrcp->عرض() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->has_مضرب_Sinks() && !rSrcp->has_مضرب_Sinks()) {
                    APPLYING(افزودن_BITWISE_THROUGH_REDUCTION) {
                        File_سطر* ثابت flp = vtxp->fileline();
                        Bitwise* ثابت bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_بیت_DType, bwp));
                        أرجع درست;
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
    ساكن std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(Dfg_رأس* vtxp) {
        إذا (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            أرجع {اشاره_تهی, اشاره_تهی};
        }
        إذا (vtxp->has_مضرب_Sinks()) أرجع {اشاره_تهی, اشاره_تهی};
        Dfg_رأس_دودویی* ثابت bwp = vtxp->as<Dfg_رأس_دودویی>();
        DfgSel* ثابت lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ثابت rSelp = bwp->rhsp()->cast<DfgSel>();
        إذا (!lSelp || !rSelp) أرجع {اشاره_تهی, اشاره_تهی};
        DfgSel* ثابت nSelp = lSelp->fromp()->عرض() <= rSelp->fromp()->عرض() ? lSelp : rSelp;
        DfgSel* ثابت wSelp = nSelp == lSelp ? rSelp : lSelp;
        ثابت uint32_t n_عرض = nSelp->fromp()->عرض();
        إذا (n_عرض > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) أرجع {اشاره_تهی, اشاره_تهی};
        إذا (wSelp->lsb() - nSelp->lsb() + n_عرض > wSelp->fromp()->عرض()) {
            أرجع {اشاره_تهی, اشاره_تهی};
        }
        أرجع {nSelp, wSelp};
    }

    قالب <نام_نوع Bitwise>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي try_افزودن_Bitwise_Op_Throug_Sel(Bitwise* ثابت vtxp) {
        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();

        إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = اشاره_تهی;
            Dfg_رأس* extrap = اشاره_تهی;
            إذا (DfgSel* ثابت selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } وإلا إذا (Bitwise* ثابت bitwisep = rhsp->cast<Bitwise>()) {
                إذا (DfgSel* ثابت rlSelp = bitwisep->lhsp()->قالب cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } وإلا إذا (DfgSel* ثابت rrSelp = bitwisep->rhsp()->قالب cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            إذا (rSelp) {
                Dfg_رأس* ثابت lFromp = lSelp->fromp();
                Dfg_رأس* ثابت rFromp = rSelp->fromp();
                إذا (lFromp->dtype() == rFromp->dtype() && lFromp->عرض() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(افزودن_BITWISE_THROUGH_SEL) {
                        Bitwise* ثابت bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        Dfg_رأس* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        أرجع درست;
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
            إذا (rChainp && rChainp->has_مضرب_Sinks()) أرجع خطأ;
            Dfg_رأس* ثابت rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ثابت خودکار lSels = pushableUnalignedSels(lhsp);
            ثابت خودکار rSels = pushableUnalignedSels(rTermp);
            إذا (!lSels.اولی && !rSels.اولی) أرجع خطأ;

            ثابت Dfg_رأس* ثابت opp = lSels.اولی ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            إذا (opp->is<Bitwise>()) أرجع خطأ;
            ثابت خودکار& sels = lSels.اولی ? lSels : rSels;
            Dfg_رأس* ثابت nFromp = sels.اولی->fromp();
            Dfg_رأس* ثابت wFromp = sels.دومی->fromp();
            ثابت uint32_t جابه_جایی = sels.دومی->lsb() - sels.اولی->lsb();

            // Both terms must end up as selects from the same pushed operation
            ثابت خودکار matches = [&](ثابت Dfg_رأس* termp, ثابت خودکار& tSels) {
                // Original form: 'n[i] OP w[j]'
                إذا (tSels.اولی) {
                    أرجع termp->type() == opp->type()  //
                           && tSels.اولی->fromp() == nFromp  //
                           && tSels.دومی->fromp() == wFromp  //
                           && tSels.دومی->lsb() - tSels.اولی->lsb() == جابه_جایی;
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
                ثابت Dfg_رأس_دودویی* ثابت bwp = termp->as<Dfg_رأس_دودویی>();
                ثابت منطقي nIsLhs = bwp->lhsp() == nFromp;
                إذا (!nIsLhs && bwp->rhsp() != nFromp) أرجع خطأ;
                ثابت DfgSel* ثابت slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                أرجع slicep && slicep->fromp() == wFromp && slicep->lsb() == جابه_جایی;
            };
            إذا (!matches(lhsp, lSels) || !matches(rTermp, rSels)) أرجع خطأ;

            APPLYING(افزودن_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_سطر* ثابت flp = vtxp->fileline();
                ثابت Dfg_داده_Type& dtype = nFromp->dtype();
                DfgSel* ثابت slicep = make<DfgSel>(flp, dtype, wFromp, جابه_جایی);
                Dfg_رأس* bwp = اشاره_تهی;
                إذا (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } وإلا إذا (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } وإلا {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ثابت خودکار افزودن = [&](Dfg_رأس* termp, ثابت DfgSel* nSelp) -> Dfg_رأس* {
                    أرجع nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                Dfg_رأس* ثابت newLhsp = افزودن(lhsp, lSels.اولی);
                Dfg_رأس* ثابت newRTermp = افزودن(rTermp, rSels.اولی);
                إذا (rChainp) {
                    Dfg_رأس* ثابت newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } وإلا {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                أرجع درست;
            }
        }

        أرجع خطأ;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ساكن منطقي isConcatFoldable(ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) {
        // Concatenation of the same vertex is a replicate
        إذا (isSame(lhsp, rhsp)) أرجع درست;
        إذا (ثابت DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            إذا (isSame(lRepp->srcp(), rhsp)) أرجع درست;
        }
        إذا (ثابت DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
            إذا (isSame(lhsp, rRepp->srcp())) أرجع درست;
        }
        // Concatenation of adjoining selects is a select
        إذا (ثابت DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            إذا (ثابت DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                أرجع isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->عرض();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        إذا (ثابت DfgNot* ثابت lNotp = lhsp->cast<DfgNot>()) {
            إذا (ثابت DfgNot* ثابت rNotp = rhsp->cast<DfgNot>()) {
                أرجع !lNotp->has_مضرب_Sinks()  //
                       && !rNotp->has_مضرب_Sinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        أرجع خطأ;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ساكن منطقي isSameBitwise(ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) {
        إذا (lhsp->type() != rhsp->type()) أرجع خطأ;
        أرجع lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ساكن منطقي isConcatSimplifiable(ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp) {
        إذا (isConcatFoldable(lhsp, rhsp)) أرجع درست;
        إذا (!isSameBitwise(lhsp, rhsp)) أرجع خطأ;
        إذا (lhsp->has_مضرب_Sinks() || rhsp->has_مضرب_Sinks()) أرجع خطأ;
        ثابت Dfg_رأس_دودویی* ثابت lp = lhsp->as<Dfg_رأس_دودویی>();
        ثابت Dfg_رأس_دودویی* ثابت rp = rhsp->as<Dfg_رأس_دودویی>();
        إذا (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            أرجع درست;
        إذا (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            أرجع درست;
        أرجع خطأ;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ساكن منطقي isConcatPushableThroughBitwise(ثابت Dfg_رأس* lhsp, ثابت Dfg_رأس* rhsp,
                                               منطقي& جابجا) {
        إذا (!isSameBitwise(lhsp, rhsp)) أرجع خطأ;
        إذا (lhsp->has_مضرب_Sinks() || rhsp->has_مضرب_Sinks()) أرجع خطأ;
        ثابت Dfg_رأس_دودویی* ثابت lp = lhsp->as<Dfg_رأس_دودویی>();
        ثابت Dfg_رأس_دودویی* ثابت rp = rhsp->as<Dfg_رأس_دودویی>();
        جابجا = خطأ;
        إذا (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) أرجع درست;
        إذا (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) أرجع درست;
        جابجا = درست;
        إذا (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) أرجع درست;
        إذا (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) أرجع درست;
        أرجع خطأ;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    Dfg_رأس* makeConcatPushedThroughBitwise(File_سطر* flp, ثابت Dfg_رأس* lhsp,
                                              ثابت Dfg_رأس* rhsp, منطقي جابجا) {
        ثابت Dfg_رأس_دودویی* ثابت lp = lhsp->as<Dfg_رأس_دودویی>();
        ثابت Dfg_رأس_دودویی* ثابت rp = rhsp->as<Dfg_رأس_دودویی>();
        ثابت Dfg_داده_Type& dtype = Dfg_داده_Type::packed(lp->عرض() + rp->عرض());
        Dfg_رأس* ثابت cp = جابجا ? rp->rhsp() : rp->lhsp();
        Dfg_رأس* ثابت dp = جابجا ? rp->lhsp() : rp->rhsp();
        DfgConcat* ثابت newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ثابت newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        إذا (lp->is<DfgAnd>()) أرجع make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        إذا (lp->is<DfgOr>()) أرجع make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        أرجع make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    قالب <نام_نوع Bitwise>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->عرض() == 1, vtxp, "Width must be 1");
        استفاده Reduction = Bitwise_مقصد_Reduction<Bitwise>;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();

        إذا (DfgSel* ثابت lSelp = lhsp->قالب cast<DfgSel>()) {
            DfgSel* rSelp = اشاره_تهی;
            Dfg_رأس* extrap = اشاره_تهی;
            إذا (DfgSel* ثابت selp = rhsp->قالب cast<DfgSel>()) {
                rSelp = selp;
            } وإلا إذا (Bitwise* ثابت rBitwisep = rhsp->قالب cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->قالب cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } وإلا إذا (Reduction* ثابت rRedp = rhsp->قالب cast<Reduction>()) {
                rSelp = rRedp->srcp()->قالب cast<DfgSel>();
            }
            إذا (rSelp) {
                uint32_t lsb = 0;
                إذا (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_از_SELS_WITH_REDUCTION) {
                        ثابت Dfg_داده_Type& dtype
                            = Dfg_داده_Type::packed(lSelp->عرض() + rSelp->عرض());
                        DfgSel* ثابت newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        Dfg_رأس* resp = make<Reduction>(vtxp, newSelp);
                        إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        أرجع درست;
                    }
                }
            }
        }

        إذا (Reduction* ثابت lRedp = lhsp->قالب cast<Reduction>()) {
            Reduction* rRedp = rhsp->قالب cast<Reduction>();
            Dfg_رأس* extrap = اشاره_تهی;
            إذا (!rRedp) {
                إذا (Bitwise* ثابت rBitwisep = rhsp->قالب cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->قالب cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            إذا (rRedp) {
                إذا (DfgSel* ثابت lSelp = lRedp->srcp()->قالب cast<DfgSel>()) {
                    إذا (DfgSel* ثابت rSelp = rRedp->srcp()->قالب cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        إذا (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_از_REDUCTION_از_SELS_WITH_REDUCTION) {
                                ثابت Dfg_داده_Type& dtype
                                    = Dfg_داده_Type::packed(lSelp->عرض() + rSelp->عرض());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                Dfg_رأس* resp = make<Reduction>(vtxp, newSelp);
                                إذا (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                أرجع درست;
                            }
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    قالب <نام_نوع Reduction>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي optimizeReduction(Reduction* ثابت vtxp) {
        استفاده Bitwise = Reduction_مقصد_Bitwise<Reduction>;

        إذا (foldUnary(vtxp)) أرجع درست;

        Dfg_رأس* ثابت srcp = vtxp->srcp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Reduction of 1-bit value
        إذا (srcp->dtype() == m_بیت_DType) {
            APPLYING(REMOVE_عرض_ONE_REDUCTION) {
                replace(srcp);
                أرجع درست;
            }
        }

        إذا (DfgCond* ثابت condp = srcp->cast<DfgCond>()) {
            إذا (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(افزودن_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ثابت newThenp = make<Reduction>(flp, m_بیت_DType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ثابت newElsep = make<Reduction>(flp, m_بیت_DType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), m_بیت_DType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    أرجع درست;
                }
            }
        }

        إذا (DfgConcat* ثابت concatp = srcp->cast<DfgConcat>()) {
            إذا (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_بیت_DType
                || concatp->rhsp()->dtype() == m_بیت_DType) {
                APPLYING(افزودن_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ثابت lRedp
                        = make<Reduction>(concatp->fileline(), m_بیت_DType, concatp->lhsp());
                    Reduction* ثابت rRedp
                        = make<Reduction>(concatp->fileline(), m_بیت_DType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_بیت_DType, lRedp, rRedp));
                    أرجع درست;
                }
            }
        }

        إذا (Bitwise* ثابت bitwisep = vtxp->srcp()->قالب cast<Bitwise>()) {
            إذا (!bitwisep->has_مضرب_Sinks()) {
                إذا (bitwisep->lhsp()->قالب is<DfgConcat>()
                    || bitwisep->rhsp()->قالب is<DfgConcat>()) {
                    APPLYING(افزودن_REDUCTION_THROUGH_BITWISE_از_CONCAT) {
                        Reduction* ثابت newLhsp
                            = make<Reduction>(flp, m_بیت_DType, bitwisep->lhsp());
                        Reduction* ثابت newRhsp
                            = make<Reduction>(flp, m_بیت_DType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_بیت_DType, newLhsp, newRhsp));
                        أرجع درست;
                    }
                }

                إذا (DfgSel* ثابت lSelp = bitwisep->lhsp()->قالب cast<DfgSel>()) {
                    إذا (DfgSel* ثابت rSelp = bitwisep->rhsp()->قالب cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        إذا (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(افزودن_REDUCTION_THROUGH_BITWISE_از_SELS) {
                                ثابت Dfg_داده_Type& dtype
                                    = Dfg_داده_Type::packed(lSelp->عرض() + rSelp->عرض());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                أرجع درست;
                            }
                        }
                    }
                }
            }
        }

        أرجع خطأ;
    }

    قالب <نام_نوع لغزش>
    VL_ATTR_WARN_استفادهنشده_نتیجه منطقي optimize_لغزش_RHS(لغزش* ثابت vtxp) {
        static_assert(std::is_پایه_از<Dfg_رأس_دودویی, لغزش>::مقدار, "Must invoke on binary");
        static_assert(std::is_final<لغزش>::مقدار, "Must invoke on final class");
        إذا (ثابت DfgConcat* ثابت concatp = vtxp->rhsp()->قالب cast<DfgConcat>()) {
            إذا (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_از_لغزش) {
                    replace(make<لغزش>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    أرجع درست;
                }
            }
        }
        أرجع خطأ;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<Dfg_رأس*, uint32_t, uint32_t> جمع_کن_مقصد_شمار_Ones_جمله(Dfg_رأس* vtxp) {
        إذا (DfgConcat* ثابت oCatp = vtxp->cast<DfgConcat>()) {
            إذا (isZero(oCatp->lhsp())) {
                إذا (Dfg_شمار_Ones* ثابت شمار_Onesp = oCatp->rhsp()->cast<Dfg_شمار_Ones>()) {
                    // Zero extended count ones
                    إذا (DfgSel* ثابت selp = شمار_Onesp->srcp()->cast<DfgSel>()) {
                        أرجع {selp->fromp(), selp->lsb(), selp->عرض()};
                    }
                } وإلا إذا (DfgSel* ثابت selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    إذا (selp->dtype() == m_بیت_DType) {  //
                        أرجع {selp->fromp(), selp->lsb(), selp->عرض()};
                    }
                }
            }
            أرجع {اشاره_تهی, 0, 0};
        }
        إذا (Dfg_شمار_Ones* ثابت شمار_Onesp = vtxp->cast<Dfg_شمار_Ones>()) {
            // Simple count ones
            إذا (DfgSel* ثابت selp = شمار_Onesp->srcp()->cast<DfgSel>()) {
                أرجع {selp->fromp(), selp->lsb(), selp->عرض()};
            }
            أرجع {اشاره_تهی, 0, 0};
        }
        إذا (DfgSel* ثابت oSelp = vtxp->cast<DfgSel>()) {
            إذا (oSelp->lsb() == 0) {
                // Truncated count ones
                إذا (Dfg_شمار_Ones* ثابت شمار_Onesp = oSelp->fromp()->cast<Dfg_شمار_Ones>()) {
                    // Zero extended count ones
                    إذا (DfgSel* ثابت selp = شمار_Onesp->srcp()->cast<DfgSel>()) {
                        أرجع {selp->fromp(), selp->lsb(), selp->عرض()};
                    }
                }
            }
            // Single bit select
            إذا (oSelp->dtype() == m_بیت_DType) {  //
                أرجع {oSelp->fromp(), oSelp->lsb(), 1};
            }
            أرجع {اشاره_تهی, 0, 0};
        }
        // Altered form of extended MSB
        إذا (Dfg_لغزش_R* ثابت shiftrp = vtxp->cast<Dfg_لغزش_R>()) {
            إذا (DfgConst* ثابت rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                إذا (rConstp->مقصد_U32() == shiftrp->عرض() - 1) {
                    أرجع {shiftrp->lhsp(), shiftrp->عرض() - 1, 1};
                }
            }
            أرجع {اشاره_تهی, 0, 0};
        }
        // Not applicable
        أرجع {اشاره_تهی, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<Dfg_رأس*, uint32_t> unwindSel(Dfg_رأس* fromp, uint32_t lsb,
                                              ثابت uint32_t عرض) {
        طالما (درست) {
            ثابت uint32_t msb = lsb + عرض - 1;

            // Sel from Concat
            إذا (DfgConcat* ثابت concatp = fromp->cast<DfgConcat>()) {
                Dfg_رأس* ثابت lhsp = concatp->lhsp();
                Dfg_رأس* ثابت rhsp = concatp->rhsp();

                إذا (msb < rhsp->عرض()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_از_CONCAT) {
                        fromp = rhsp;
                        استمر;
                    }
                } وإلا إذا (lsb >= rhsp->عرض()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_از_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->عرض();
                        استمر;
                    }
                }
            }

            إذا (DfgRep* ثابت repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ثابت uint32_t src_عرض = repp->srcp()->عرض();
                إذا (عرض <= src_عرض) {
                    ثابت uint32_t newLsb = lsb % src_عرض;
                    ثابت uint32_t newMsb = newLsb + عرض - 1;
                    إذا (newMsb < src_عرض) {
                        APPLYING(افزودن_SEL_THROUGH_REP) {
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
                ثابت خودکار pair = varp->driver_از_Range(lsb, عرض);
                Dfg_رأس* ثابت driverp = pair.اولی;
                ثابت uint32_t driverLsb = pair.دومی;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                إذا (driverp) {
                    إذا (driverp == varp->defaultp()) {
                        APPLYING(افزودن_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            استمر;
                        }
                    } وإلا {
                        APPLYING(افزودن_SEL_THROUGH_SPLICE) {
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
    std::pair<Dfg_رأس*, uint32_t> commonLSBs(Dfg_رأس* ap, Dfg_رأس* bp) {
        إذا (ap == bp) أرجع {ap, ap->عرض()};

        // If both constants, check LSBs
        إذا (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            إذا (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت V3_عدد& a_رقم = aConstp->رقم();
                ثابت V3_عدد& b_رقم = bConstp->رقم();
                // Max match is the shorter constant
                ثابت uint32_t max_تطابق = std::min(aConstp->عرض(), bConstp->عرض());
                // Check all bits
                uint32_t تطابق_عرض = 0;
                لكل (; تطابق_عرض < max_تطابق; ++تطابق_عرض) {
                    إذا (a_رقم.بیت_Is0(تطابق_عرض) != b_رقم.بیت_Is0(تطابق_عرض)) اكسر;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aConstp->عرض() < bConstp->عرض() ? aConstp : bConstp;
                أرجع {تطابق_عرض ? shorterp : اشاره_تهی, تطابق_عرض};
            }
        }

        // If Concat, check against the RHS
        إذا (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) أرجع commonLSBs(catp->rhsp(), bp);
        إذا (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) أرجع commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        إذا (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            إذا (selp->lsb() == 0) {
                Dfg_رأس* ثابت fromp = selp->fromp();
                ثابت std::pair<Dfg_رأس*, uint32_t> common = commonLSBs(fromp, bp);
                أرجع {common.اولی, std::min(common.دومی, selp->عرض())};
            }
        }
        إذا (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            إذا (selp->lsb() == 0) {
                Dfg_رأس* ثابت fromp = selp->fromp();
                ثابت std::pair<Dfg_رأس*, uint32_t> common = commonLSBs(ap, fromp);
                أرجع {common.اولی, std::min(common.دومی, selp->عرض())};
            }
        }

        // Otherwise no common LSBs
        أرجع {اشاره_تهی, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_رأس*, uint32_t> commonMSBs(Dfg_رأس* ap, Dfg_رأس* bp) {
        إذا (ap == bp) أرجع {ap, ap->عرض()};

        // If both constants, check MSBs
        إذا (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            إذا (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت uint32_t aMsb = aConstp->عرض() - 1;
                ثابت uint32_t bMsb = bConstp->عرض() - 1;
                ثابت V3_عدد& a_رقم = aConstp->رقم();
                ثابت V3_عدد& b_رقم = bConstp->رقم();
                // Max match is the shorter constant
                ثابت uint32_t max_تطابق = std::min(aConstp->عرض(), bConstp->عرض());
                // Check all bits
                uint32_t تطابق_عرض = 0;
                لكل (; تطابق_عرض < max_تطابق; ++تطابق_عرض) {
                    إذا (a_رقم.بیت_Is0(aMsb - تطابق_عرض) != b_رقم.بیت_Is0(bMsb - تطابق_عرض)) اكسر;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aMsb < bMsb ? aConstp : bConstp;
                أرجع {تطابق_عرض ? shorterp : اشاره_تهی, تطابق_عرض};
            }
        }

        // If Concat, check against the LHS
        إذا (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) أرجع commonMSBs(catp->lhsp(), bp);
        إذا (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) أرجع commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        إذا (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            Dfg_رأس* ثابت fromp = selp->fromp();
            إذا (selp->msb() == fromp->عرض() - 1) {
                ثابت std::pair<Dfg_رأس*, uint32_t> common = commonMSBs(fromp, bp);
                أرجع {common.اولی, std::min(common.دومی, selp->عرض())};
            }
        }
        إذا (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            Dfg_رأس* ثابت fromp = selp->fromp();
            إذا (selp->msb() == fromp->عرض() - 1) {
                ثابت std::pair<Dfg_رأس*, uint32_t> common = commonMSBs(ap, fromp);
                أرجع {common.اولی, std::min(common.دومی, selp->عرض())};
            }
        }

        // Otherwise no common MSBs
        أرجع {اشاره_تهی, 0};
    }

    // VISIT methods

    فراغ بازدید(Dfg_رأس*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    فراغ بازدید(Dfg_شمار_Ones* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ بازدید(DfgExtend* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            Dfg_رأس* ثابت zerop
                = makeZero(vtxp->fileline(), vtxp->عرض() - vtxp->srcp()->عرض());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            أرجع;
        }
    }

    فراغ بازدید(DfgExtendS* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ بازدید(DfgLogNot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ بازدید(DfgNegate* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ بازدید(DfgNot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;

        // Not of Cond
        إذا (DfgCond* ثابت condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            إذا (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(افزودن_NOT_THROUGH_COND) {
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

        إذا (!vtxp->srcp()->has_مضرب_Sinks()) {
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

    فراغ بازدید(DfgOneHot* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ بازدید(DfgOneHot0* ثابت vtxp) override {
        إذا (foldUnary(vtxp)) أرجع;
    }

    فراغ بازدید(DfgRedOr* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ بازدید(DfgRedAnd* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ بازدید(DfgRedXor* ثابت vtxp) override {
        إذا (optimizeReduction(vtxp)) أرجع;
    }

    فراغ بازدید(DfgSel* ثابت vtxp) override {
        Dfg_رأس* ثابت fromp = vtxp->fromp();

        File_سطر* ثابت flp = vtxp->fileline();

        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t عرض = vtxp->عرض();
        ثابت uint32_t msb = lsb + عرض - 1;

        إذا (DfgConst* ثابت constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ثابت resp = makeZero(flp, عرض);
                resp->رقم().opSel(constp->رقم(), msb, lsb);
                replace(resp);
                أرجع;
            }
        }

        // Full width select, replace with the source.
        إذا (fromp->عرض() == عرض) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_پر_عرض_SEL) {
                replace(fromp);
                أرجع;
            }
        }

        // Unwind through bit packing in one go
        {
            ثابت خودکار res = unwindSel(fromp, lsb, عرض);
            إذا (res.اولی != fromp || res.دومی != lsb) {
                replace(make<DfgSel>(vtxp, res.اولی, res.دومی));
                أرجع;
            }
        }

        // Sel from Not
        إذا (DfgNot* ثابت notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            إذا (!notp->has_مضرب_Sinks()) {
                APPLYING(افزودن_SEL_THROUGH_NOT) {
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
            إذا (!condp->has_مضرب_Sinks()) {
                APPLYING(افزودن_SEL_THROUGH_COND) {
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
        إذا (Dfg_لغزش_L* ثابت لغزش_Lp = fromp->cast<Dfg_لغزش_L>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            إذا (lsb == 0) {
                UASSERT_OBJ(لغزش_Lp->lhsp()->عرض() >= عرض, vtxp, "input of shift narrow");
                APPLYING(افزودن_SEL_THROUGH_SHIFTL) {
                    DfgSel* ثابت newSelp = make<DfgSel>(vtxp, لغزش_Lp->lhsp(), vtxp->lsb());
                    replace(make<Dfg_لغزش_L>(vtxp, newSelp, لغزش_Lp->rhsp()));
                    أرجع;
                }
            }
        }
    }

    فراغ بازدید(DfgMux* ثابت vtxp) override {
        Dfg_رأس* ثابت fromp = vtxp->fromp();
        Dfg_رأس* ثابت lsbp = vtxp->lsbp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (DfgConst* ثابت lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->رقم().مقصد_UInt()));
                أرجع;
            }
        }

        إذا (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->عرض()));
                أرجع;
            }
        }

        إذا (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->عرض()));
                أرجع;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    فراغ بازدید(DfgAnd* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        إذا (!lhsp->has_مضرب_Sinks() || !rhsp->has_مضرب_Sinks()) {
            إذا (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                إذا (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_از_NOT_AND_NOT) {
                        DfgOr* ثابت orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        أرجع;
                    }
                }
                إذا (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_از_NOT_AND_NEQ) {
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
                إذا (try_افزودن_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) أرجع;
            }
        }

        إذا (distributive_And_Associative_دودویی<DfgOr, DfgAnd>(vtxp)) أرجع;

        إذا (try_افزودن_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_افزودن_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            إذا ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->عرض()));
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
                        replace(makeZero(flp, vtxp->عرض()));
                        أرجع;
                    }
                }
            }
        }

        إذا (vtxp->dtype() == m_بیت_DType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            إذا (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            إذا (repp && condp && repp->srcp()->اندازه() == 1 && !condp->has_مضرب_Sinks()
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

    فراغ بازدید(DfgOr* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        إذا (!lhsp->has_مضرب_Sinks() || !rhsp->has_مضرب_Sinks()) {
            إذا (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                إذا (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_از_NOT_AND_NOT) {
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        أرجع;
                    }
                }
                إذا (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_از_NOT_AND_NEQ) {
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
                        APPLYING(REPLACE_OR_از_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            أرجع;
                        }
                    }
                    إذا (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_از_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
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
                إذا (try_افزودن_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) أرجع;
            }
        }

        إذا (distributive_And_Associative_دودویی<DfgAnd, DfgOr>(vtxp)) أرجع;

        إذا (try_افزودن_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_افزودن_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            إذا ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->عرض()));
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
                        replace(makeOnes(flp, vtxp->عرض()));
                        أرجع;
                    }
                }
            }
        }

        إذا (vtxp->dtype() == m_بیت_DType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }
    }

    فراغ بازدید(DfgXor* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->عرض()));
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
                إذا (try_افزودن_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) أرجع;
            }
        }

        إذا (try_افزودن_Bitwise_Op_Through_Reductions(vtxp)) أرجع;

        إذا (try_افزودن_Bitwise_Op_Throug_Sel(vtxp)) أرجع;

        إذا (vtxp->dtype() == m_بیت_DType) {
            إذا (tryReplaceBitwiseWithReduction(vtxp)) أرجع;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    فراغ بازدید(Dfg_جمع_کن* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isZero(lhsp)) {
            APPLYING(REMOVE_جمع_کن_ZERO) {
                replace(rhsp);
                أرجع;
            }
        }

        ثابت std::tuple<Dfg_رأس*, uint32_t, uint32_t> l_جمله = جمع_کن_مقصد_شمار_Ones_جمله(lhsp);
        إذا (Dfg_رأس* ثابت lVtxp = std::get<0>(l_جمله)) {
            std::tuple<Dfg_رأس*, uint32_t, uint32_t> r_جمله = جمع_کن_مقصد_شمار_Ones_جمله(rhsp);
            Dfg_رأس* extrap = اشاره_تهی;
            إذا (!std::get<0>(r_جمله)) {
                إذا (Dfg_جمع_کن* ثابت rAddp = rhsp->cast<Dfg_جمع_کن>()) {
                    r_جمله = جمع_کن_مقصد_شمار_Ones_جمله(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            إذا (Dfg_رأس* ثابت rVtxp = std::get<0>(r_جمله)) {
                إذا (isSame(lVtxp, rVtxp)) {
                    ثابت uint32_t lLsb = std::get<1>(l_جمله);
                    ثابت uint32_t rLsb = std::get<1>(r_جمله);
                    ثابت uint32_t l_عرض = std::get<2>(l_جمله);
                    ثابت uint32_t r_عرض = std::get<2>(r_جمله);
                    منطقي adjoined = درست;
                    uint32_t lsb = 0;
                    إذا (lLsb + l_عرض == rLsb) {
                        lsb = lLsb;
                    } وإلا إذا (lLsb == rLsb + r_عرض) {
                        lsb = rLsb;
                    } وإلا {
                        adjoined = خطأ;
                    }
                    إذا (adjoined) {
                        APPLYING(REPLACE_جمع_کن_WITH_شمار_ONES) {
                            DfgSel* ثابت selp
                                = make<DfgSel>(vtxp->fileline(),
                                               Dfg_داده_Type::packed(l_عرض + r_عرض), lVtxp, lsb);
                            Dfg_رأس* resp
                                = make<Dfg_شمار_Ones>(flp, Dfg_داده_Type::packed(32), selp);
                            إذا (vtxp->عرض() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->عرض() - 32U),
                                                       resp);
                            } وإلا إذا (vtxp->عرض() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            إذا (extrap) resp = make<Dfg_جمع_کن>(vtxp, resp, extrap);
                            replace(resp);
                            أرجع;
                        }
                    }
                }
            }
        }
    }

    فراغ بازدید(DfgArraySel* ثابت vtxp) override {
        DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>();
        إذا (!idxp) أرجع;
        DfgVarArray* ثابت varp = vtxp->fromp()->cast<DfgVarArray>();
        إذا (!varp) أرجع;
        AstVar* ثابت astVarp = varp->vscp()->varp();
        إذا (astVarp->isForced()) أرجع;
        إذا (astVarp->isSigUserRWPublic()) أرجع;
        Dfg_رأس* ثابت srcp = varp->srcp();
        إذا (!srcp) {
            إذا (vtxp->isPacked()) {
                إذا (AstConsArray* ثابت iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    إذا (AstConst* ثابت valp = VN_CAST(iap->valuep(idxp->مقصد_اندازه_T()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_جدول) {
                            replace(جديد DfgConst{m_dfg, valp->fileline(), valp->رقم()});
                            أرجع;
                        }
                    }
                }
            }
            أرجع;
        }

        إذا (DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            Dfg_رأس* ثابت driverp = splicep->driverAt(idxp->مقصد_اندازه_T());
            إذا (!driverp) أرجع;
            DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>();
            إذا (!uap) أرجع;
            إذا (uap->srcp()->is<Dfg_رأس_Splice>()) أرجع;
            // If driven by a variable that had a Driver in DFG, it is partial
            إذا (Dfg_رأس_Var* ثابت dvarp = uap->srcp()->cast<Dfg_رأس_Var>()) {
                إذا (dvarp->srcp()) أرجع;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                أرجع;
            }
        }

        إذا (DfgUnitArray* ثابت uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->مقصد_اندازه_T() == 0, vtxp, "Array index out of range");
            إذا (uap->srcp()->is<DfgSplicePacked>()) أرجع;
            // If driven by a variable that had a Driver in DFG, it is partial
            إذا (Dfg_رأس_Var* ثابت dvarp = uap->srcp()->cast<Dfg_رأس_Var>()) {
                إذا (dvarp->srcp()) أرجع;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgConcat* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();

        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isZero(lhsp)) {
            DfgConst* ثابت lConstp = lhsp->as<DfgConst>();
            إذا (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                إذا (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->عرض()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_بالاترین_WITH_SHIFTR) {
                        replace(
                            make<Dfg_لغزش_R>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->عرض())));
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
                            make<Dfg_لغزش_L>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->عرض())));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgNot* ثابت lNot = lhsp->cast<DfgNot>()) {
            إذا (DfgNot* ثابت rNot = rhsp->cast<DfgNot>()) {
                إذا (!lNot->has_مضرب_Sinks() && !rNot->has_مضرب_Sinks()) {
                    APPLYING(افزودن_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ثابت newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        أرجع;
                    }
                }
            }
        }

        {
            منطقي جابجا = خطأ;
            إذا (isConcatPushableThroughBitwise(lhsp, rhsp, جابجا)) {
                APPLYING(افزودن_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, جابجا));
                    أرجع;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            إذا (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                Dfg_رأس* ثابت rlp = rConcatp->lhsp();
                إذا (!rConcatp->has_مضرب_Sinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, جابجا)) {
                    APPLYING(افزودن_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        Dfg_رأس* ثابت newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, جابجا);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        أرجع;
                    }
                }
            }
        }

        إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            إذا (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                إذا (isSame(lSelp->fromp(), rSelp->fromp())) {
                    إذا (lSelp->lsb() == rSelp->lsb() + rSelp->عرض()) {
                        APPLYING(REMOVE_CONCAT_از_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            أرجع;
                        }
                    }
                }
            }

            إذا (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                إذا (DfgSel* ثابت rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    إذا (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        إذا (lSelp->lsb() == rlSelp->lsb() + rlSelp->عرض()) {
                            APPLYING(REPLACE_NESTED_CONCAT_از_ADJOINING_SELS_ON_LHS) {
                                ثابت uint32_t عرض = lSelp->عرض() + rlSelp->عرض();
                                ثابت Dfg_داده_Type& dtype = Dfg_داده_Type::packed(عرض);
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
                        إذا (lrSelp->lsb() == rSelp->lsb() + rSelp->عرض()) {
                            APPLYING(REPLACE_NESTED_CONCAT_از_ADJOINING_SELS_ON_RHS) {
                                ثابت uint32_t عرض = lrSelp->عرض() + rSelp->عرض();
                                ثابت Dfg_داده_Type& dtype = Dfg_داده_Type::packed(عرض);
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
                Dfg_رأس* ثابت rtVtxp = rCondp->thenp();
                Dfg_رأس* ثابت reVtxp = rCondp->elsep();
                DfgConst* ثابت rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ثابت reConstp = reVtxp->cast<DfgConst>();
                إذا (!rCondp->has_مضرب_Sinks() && (rtConstp || reConstp)) {
                    APPLYING(افزودن_CONCAT_THROUGH_COND_LHS) {
                        Dfg_رأس* ثابت thenp = [&]() -> Dfg_رأس* {
                            File_سطر* ثابت rtFlp = rtVtxp->fileline();
                            إذا (rtConstp) {
                                DfgConst* ثابت constp = makeZero(rtFlp, vtxp->عرض());
                                constp->رقم().opConcat(lConstp->رقم(), rtConstp->رقم());
                                أرجع constp;
                            }
                            أرجع make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        Dfg_رأس* ثابت elsep = [&]() -> Dfg_رأس* {
                            File_سطر* ثابت reFlp = reVtxp->fileline();
                            إذا (reConstp) {
                                DfgConst* ثابت constp = makeZero(reFlp, vtxp->عرض());
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
                Dfg_رأس* ثابت ltVtxp = lCondp->thenp();
                Dfg_رأس* ثابت leVtxp = lCondp->elsep();
                DfgConst* ثابت ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ثابت leConstp = leVtxp->cast<DfgConst>();
                إذا (!lCondp->has_مضرب_Sinks() && (ltConstp || leConstp)) {
                    APPLYING(افزودن_CONCAT_THROUGH_COND_RHS) {
                        Dfg_رأس* ثابت thenp = [&]() -> Dfg_رأس* {
                            File_سطر* ثابت ltFlp = ltVtxp->fileline();
                            إذا (ltConstp) {
                                DfgConst* ثابت constp = makeZero(ltFlp, vtxp->عرض());
                                constp->رقم().opConcat(ltConstp->رقم(), rConstp->رقم());
                                أرجع constp;
                            }
                            أرجع make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        Dfg_رأس* ثابت elsep = [&]() -> Dfg_رأس* {
                            File_سطر* ثابت leFlp = leVtxp->fileline();
                            إذا (leConstp) {
                                DfgConst* ثابت constp = makeZero(leFlp, vtxp->عرض());
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
            ثابت uint32_t vMsb = vtxp->عرض() - 1;  // MSB of the concatenation
            ثابت uint32_t lLsb = vtxp->rhsp()->عرض();  // LSB of the LHS
            ثابت uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            منطقي hasCrossSink = خطأ;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](Dfg_رأس& sink) {
                // Record bits used by DfgSel sinks
                إذا (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    ثابت uint32_t selLsb = selp->lsb();
                    ثابت uint32_t selMsb = selLsb + selp->عرض() - 1;
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
                أرجع درست;
            });
            إذا (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    Dfg_رأس* nRhsp = rhsp;
                    إذا (lsb != 0)
                        nRhsp = make<DfgSel>(flp, Dfg_داده_Type::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    Dfg_رأس* nLhsp = lhsp;
                    إذا (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, Dfg_داده_Type::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    Dfg_رأس* ثابت catp
                        = make<DfgConcat>(flp, Dfg_داده_Type::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ثابت sp = جديد DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_آخرین_Id;
                    sp->جمع_کن_Driver(catp, lsb, flp);
                    Dfg_رأس_Var* ثابت varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_آخرین_Id;
                    varp->vscp()->varp()->is_داخلی(درست);
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
                    إذا (!rConcatp->has_مضرب_Sinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ثابت uint32_t عرض = lRepp->عرض() + rlRepp->عرض();
                            ثابت Dfg_داده_Type& dtype = Dfg_داده_Type::packed(عرض);
                            DfgRep* ثابت repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            أرجع;
                        }
                    }
                }
            }
        }
    }

    فراغ بازدید(DfgDiv* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        إذا (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            إذا (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    أرجع;
                }
            }
        }
    }

    فراغ بازدید(DfgDivS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        إذا (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            إذا (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    أرجع;
                }
            }
        }
    }

    فراغ بازدید(DfgGt* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgGtS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgGte* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgGteS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgLogAnd* ثابت vtxp) override {
        إذا (دودویی(vtxp) || vtxp->rhsp()->unsafe()) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();

        إذا (lhsp->عرض() == 1 && rhsp->عرض() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgLogEq* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
    }

    فراغ بازدید(DfgLogIf* ثابت vtxp) override {
        إذا (دودویی(vtxp) || vtxp->rhsp()->unsafe()) أرجع;
    }

    فراغ بازدید(DfgLogOr* ثابت vtxp) override {
        إذا (دودویی(vtxp) || vtxp->rhsp()->unsafe()) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();

        إذا (lhsp->عرض() == 1 && rhsp->عرض() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgLt* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgLtS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgLte* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgLteS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }
    }

    فراغ بازدید(DfgModDiv* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
    }

    فراغ بازدید(DfgModDivS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
    }

    فراغ بازدید(DfgMul* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
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

    فراغ بازدید(DfgMulS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
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

    فراغ بازدید(DfgEq* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                أرجع;
            }
        }

        إذا (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_افزودن_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) أرجع;
            }

            إذا (rhsp->dtype() == m_بیت_DType) {
                إذا (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_بیت_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        أرجع;
                    }
                }

                إذا (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_بیت_1) {
                        replace(rhsp);
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ بازدید(DfgNeq* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                أرجع;
            }
        }

        إذا (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            إذا (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                إذا (try_افزودن_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) أرجع;
            }

            إذا (rhsp->dtype() == m_بیت_DType) {
                إذا (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_بیت_0) {
                        replace(rhsp);
                        أرجع;
                    }
                }

                إذا (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_بیت_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ بازدید(DfgPow* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
    }

    فراغ بازدید(DfgPowSS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
    }

    فراغ بازدید(DfgPowSU* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
    }

    فراغ بازدید(DfgPowUS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
    }

    فراغ بازدید(DfgRep* ثابت vtxp) override {
        Dfg_رأس* ثابت srcp = vtxp->srcp();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (DfgConst* ثابت sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ثابت resultp = makeZero(flp, vtxp->عرض());
                resultp->رقم().opRepl(sConstp->رقم(), vtxp->شمار());
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
            إذا (!sRepp->has_مضرب_Sinks() || vtxp->عرض() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    أرجع;
                }
            }
        }
    }

    فراغ بازدید(Dfg_لغزش_L* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
        إذا (optimize_لغزش_RHS(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();

        إذا (Dfg_لغزش_L* ثابت l_لغزش_Lp = lhsp->cast<Dfg_لغزش_L>()) {
            إذا (!l_لغزش_Lp->has_مضرب_Sinks() && rhsp->dtype() == l_لغزش_Lp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_سطر* ثابت flp = vtxp->fileline();
                    ثابت Dfg_داده_Type& جمع_DType = Dfg_داده_Type::packed(rhsp->عرض() + 1);
                    Dfg_رأس* ثابت bp = make<DfgExtend>(flp, جمع_DType, l_لغزش_Lp->rhsp());
                    Dfg_رأس* ثابت cp = make<DfgExtend>(flp, جمع_DType, rhsp);
                    Dfg_جمع_کن* ثابت addp = make<Dfg_جمع_کن>(flp, جمع_DType, bp, cp);
                    replace(make<Dfg_لغزش_L>(vtxp, l_لغزش_Lp->lhsp(), addp));
                    أرجع;
                }
            }
        }

        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t لغزش_Amount = rConstp->مقصد_U32();

            إذا (لغزش_Amount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    أرجع;
                }
            }

            إذا (لغزش_Amount >= vtxp->عرض()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->عرض()));
                    أرجع;
                }
            }

            إذا (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                إذا (!lConcatp->has_مضرب_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        Dfg_رأس* ثابت lRhsp = lConcatp->rhsp();
                        Dfg_رأس* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t rW = لغزش_Amount;
                        ثابت uint32_t mW = std::min(lRhsp->عرض(), vtxp->عرض() - لغزش_Amount);
                        ثابت uint32_t lW = vtxp->عرض() - mW - rW;
                        // Construct the result
                        File_سطر* ثابت flp = lConcatp->fileline();
                        Dfg_رأس* ثابت rp = makeZero(flp, لغزش_Amount);
                        Dfg_رأس* ثابت mp
                            = make<DfgSel>(flp, Dfg_داده_Type::packed(mW), lRhsp, 0U);
                        Dfg_رأس* np = make<DfgConcat>(flp, Dfg_داده_Type::packed(mW + rW), mp, rp);
                        إذا (!lW) {
                            replace(np);
                            أرجع;
                        }
                        Dfg_رأس* ثابت lp
                            = make<DfgSel>(flp, Dfg_داده_Type::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        أرجع;
                    }
                }
            }

            إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                إذا (!lSelp->has_مضرب_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ثابت uint32_t n_Sel_عرض = lSelp->عرض() - لغزش_Amount;
                        Dfg_رأس* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_داده_Type::packed(n_Sel_عرض),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), لغزش_Amount)));
                        أرجع;
                    }
                }
            }

            إذا (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                إذا (!lCondp->has_مضرب_Sinks()) {
                    APPLYING(افزودن_SHIFTL_THROUGH_COND) {
                        Dfg_لغزش_L* ثابت tp = make<Dfg_لغزش_L>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_لغزش_L* ثابت ep = make<Dfg_لغزش_L>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ بازدید(Dfg_لغزش_R* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
        إذا (optimize_لغزش_RHS(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();

        إذا (Dfg_لغزش_R* ثابت l_لغزش_Rp = lhsp->cast<Dfg_لغزش_R>()) {
            إذا (!l_لغزش_Rp->has_مضرب_Sinks() && rhsp->dtype() == l_لغزش_Rp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_سطر* ثابت flp = vtxp->fileline();
                    ثابت Dfg_داده_Type& جمع_DType = Dfg_داده_Type::packed(rhsp->عرض() + 1);
                    Dfg_رأس* ثابت bp = make<DfgExtend>(flp, جمع_DType, l_لغزش_Rp->rhsp());
                    Dfg_رأس* ثابت cp = make<DfgExtend>(flp, جمع_DType, rhsp);
                    Dfg_جمع_کن* ثابت addp = make<Dfg_جمع_کن>(flp, جمع_DType, bp, cp);
                    replace(make<Dfg_لغزش_R>(vtxp, l_لغزش_Rp->lhsp(), addp));
                    أرجع;
                }
            }
        }

        إذا (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t لغزش_Amount = rConstp->مقصد_U32();

            إذا (لغزش_Amount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    أرجع;
                }
            }

            إذا (لغزش_Amount >= vtxp->عرض()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->عرض()));
                    أرجع;
                }
            }

            إذا (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                إذا (!lConcatp->has_مضرب_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        Dfg_رأس* ثابت lRhsp = lConcatp->rhsp();
                        Dfg_رأس* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t lW = لغزش_Amount;
                        ثابت uint32_t mW = std::min(lLhsp->عرض(), vtxp->عرض() - لغزش_Amount);
                        ثابت uint32_t rW = vtxp->عرض() - mW - lW;
                        // Construct the result
                        File_سطر* ثابت flp = lConcatp->fileline();
                        Dfg_رأس* ثابت lp = makeZero(flp, لغزش_Amount);
                        Dfg_رأس* ثابت mp = make<DfgSel>(flp, Dfg_داده_Type::packed(mW), lLhsp,
                                                           lLhsp->عرض() - mW);
                        Dfg_رأس* np = make<DfgConcat>(flp, Dfg_داده_Type::packed(lW + mW), lp, mp);
                        إذا (!rW) {
                            replace(np);
                            أرجع;
                        }
                        Dfg_رأس* ثابت rp = make<DfgSel>(flp, Dfg_داده_Type::packed(rW), lRhsp,
                                                           lRhsp->عرض() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        أرجع;
                    }
                }
            }

            إذا (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                إذا (!lSelp->has_مضرب_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ثابت uint32_t n_Sel_عرض = lSelp->عرض() - لغزش_Amount;
                        Dfg_رأس* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_داده_Type::packed(n_Sel_عرض),
                                           lSelp->fromp(), lSelp->lsb() + لغزش_Amount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), لغزش_Amount), nSelp));
                        أرجع;
                    }
                }
            }

            إذا (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                إذا (!lCondp->has_مضرب_Sinks()) {
                    APPLYING(افزودن_SHIFTR_THROUGH_COND) {
                        Dfg_لغزش_R* ثابت tp = make<Dfg_لغزش_R>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_لغزش_R* ثابت ep = make<Dfg_لغزش_R>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        أرجع;
                    }
                }
            }
        }
    }

    فراغ بازدید(Dfg_لغزش_RS* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;
        إذا (optimize_لغزش_RHS(vtxp)) أرجع;
    }

    فراغ بازدید(DfgSub* ثابت vtxp) override {
        إذا (دودویی(vtxp)) أرجع;

        Dfg_رأس* ثابت lhsp = vtxp->lhsp();
        Dfg_رأس* ثابت rhsp = vtxp->rhsp();
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
            إذا (vtxp->dtype() == m_بیت_DType && rConstp->has_مقدار(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_بیت_DType, lhsp));
                    أرجع;
                }
            }
        }
    }

    فراغ بازدید(Dfg_تطابق_Masked* ثابت vtxp) override {
        إذا (DfgConst* ثابت constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ثابت تطابق_Varp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), Ast_تطابق_Masked::fold(constp->رقم(), تطابق_Varp)));
                أرجع;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    فراغ بازدید(DfgCond* ثابت vtxp) override {
        Dfg_رأس* ثابت condp = vtxp->condp();
        Dfg_رأس* ثابت thenp = vtxp->thenp();
        Dfg_رأس* ثابت elsep = vtxp->elsep();
        File_سطر* ثابت flp = vtxp->fileline();

        إذا (condp->dtype() != m_بیت_DType) أرجع;

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
            إذا (!condp->has_مضرب_Sinks() || condNotp->has_مضرب_Sinks()) {
                APPLYING(جابجا_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    أرجع;
                }
            }
        }

        إذا (DfgNeq* ثابت condNeqp = condp->cast<DfgNeq>()) {
            إذا (!condp->has_مضرب_Sinks()) {
                APPLYING(جابجا_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ثابت newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    أرجع;
                }
            }
        }

        إذا (DfgNot* ثابت thenNotp = thenp->cast<DfgNot>()) {
            إذا (DfgNot* ثابت elseNotp = elsep->cast<DfgNot>()) {
                إذا (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->has_مضرب_Sinks() && !elseNotp->has_مضرب_Sinks()) {
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
                إذا (!thenCondp->has_مضرب_Sinks()) {
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

        إذا (vtxp->عرض() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            إذا (Dfg_جمع_کن* ثابت thenAddp = thenp->cast<Dfg_جمع_کن>()) {
                إذا (DfgConst* ثابت constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    إذا (constp->has_مقدار(1)) {
                        إذا (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->عرض() - 1), condp);
                                File_سطر* ثابت thenFlp = thenAddp->fileline();
                                replace(
                                    make<Dfg_جمع_کن>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                أرجع;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            إذا (DfgSub* ثابت thenSubp = thenp->cast<DfgSub>()) {
                إذا (DfgConst* ثابت constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    إذا (constp->has_مقدار(1)) {
                        إذا (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->عرض() - 1), condp);
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

        إذا (vtxp->dtype() == m_بیت_DType) {
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

        إذا (vtxp->عرض() <= VL_QUADSIZE) {
            إذا (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    Dfg_رأس* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    أرجع;
                }
            }
            إذا (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    Dfg_رأس* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    أرجع;
                }
            }
            إذا (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    Dfg_رأس* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    أرجع;
                }
            }
            إذا (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    Dfg_رأس* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    أرجع;
                }
            }

            إذا (DfgOr* ثابت tOrp = thenp->cast<DfgOr>()) {
                إذا (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        Dfg_رأس* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        أرجع;
                    }
                }
                إذا (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        Dfg_رأس* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        أرجع;
                    }
                }
            }
        }

        إذا (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ثابت std::pair<Dfg_رأس*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            إذا (cLSBs.اولی) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ثابت uint32_t r_عرض = cLSBs.دومی;
                    Dfg_رأس* rhsp = cLSBs.اولی;
                    إذا (r_عرض != rhsp->عرض()) {
                        ثابت Dfg_داده_Type& rDtype = Dfg_داده_Type::packed(r_عرض);
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
                    ثابت uint32_t l_عرض = vtxp->عرض() - r_عرض;
                    ثابت Dfg_داده_Type& lDtype = Dfg_داده_Type::packed(l_عرض);
                    Dfg_رأس* ثابت lThenp = make<DfgSel>(flp, lDtype, thenp, r_عرض);
                    Dfg_رأس* ثابت lElsep = make<DfgSel>(flp, lDtype, elsep, r_عرض);
                    Dfg_رأس* ثابت lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    أرجع;
                }
            }

            ثابت std::pair<Dfg_رأس*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            إذا (cMSBs.اولی) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ثابت uint32_t l_عرض = cMSBs.دومی;
                    Dfg_رأس* lhsp = cMSBs.اولی;
                    إذا (l_عرض != lhsp->عرض()) {
                        ثابت Dfg_داده_Type& lDtype = Dfg_داده_Type::packed(l_عرض);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->عرض() - l_عرض);
                    }
                    // If it's all the same, just replace
                    إذا (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        أرجع;
                    }
                    // Create new RHS
                    ثابت uint32_t r_عرض = vtxp->عرض() - l_عرض;
                    ثابت Dfg_داده_Type& rDtype = Dfg_داده_Type::packed(r_عرض);
                    Dfg_رأس* ثابت rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    Dfg_رأس* ثابت rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    Dfg_رأس* ثابت rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    أرجع;
                }
            }
        }

        إذا (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                Dfg_رأس* resp = condp;
                إذا (ثابت uint32_t extend = vtxp->عرض() - 1) {
                    DfgConst* ثابت zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                أرجع;
            }
        }

        إذا (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                Dfg_رأس* resp = make<DfgNot>(condp, condp);
                إذا (ثابت uint32_t extend = vtxp->عرض() - 1) {
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

    فراغ بازدید(Dfg_رأس_Var* ثابت vtxp) override {
        إذا (vtxp->hasSinks()) أرجع;
        إذا (vtxp->isObserved()) أرجع;
        إذا (vtxp->defaultp()) أرجع;

        // If undriven, or driven from another var, it is completely redundant.
        إذا (!vtxp->srcp() || vtxp->srcp()->is<Dfg_رأس_Var>()) {
            APPLYING(REMOVE_VAR) {
                delete_رأس(vtxp);
                أرجع;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        منطقي یافت_One = خطأ;
        ثابت منطقي keep = vtxp->srcp()->foreachSink([&](Dfg_رأس& sink) -> منطقي {
            // Ignore non-observable variable sinks. These can be eliminated.
            إذا (ثابت Dfg_رأس_Var* ثابت varp = sink.cast<Dfg_رأس_Var>()) {
                إذا (!varp->hasSinks() && !varp->isObserved()) أرجع خطأ;
            }
            // Keep if found more than one sink
            إذا (یافت_One) أرجع درست;
            // Mark first sink found
            یافت_One = درست;
            أرجع خطأ;
        });
        إذا (!keep) {
            APPLYING(REMOVE_VAR) {
                delete_رأس(vtxp);
                أرجع;
            }
        }
    }

    V3DfgPeephole(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.for_Each_رأس([&](Dfg_رأس& vtx) { m_vInfo[vtx].m_id = ++m_آخرین_Id; });

        // Add all operation vertices to the cache
        لكل (Dfg_رأس& vtx : m_dfg.opVertices()) cache_رأس(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.اندازه() * 2);
        m_iterList.reserve(m_dfg.اندازه() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.افزودن_پشت(اشاره_تهی);
        m_iterList.افزودن_پشت(اشاره_تهی);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        لكل (Dfg_رأس_Var& vtx : m_dfg.varVertices()) جمع_کن_مقصد_Work_List(&vtx);

        // Add all operation vertices to the work list
        لكل (Dfg_رأس& vtx : m_dfg.opVertices()) جمع_کن_مقصد_Work_List(&vtx);

        // Process iteratively
        طالما (درست) {
            // Process the work list - keep the placeholder at index 0
            طالما (m_workList.اندازه() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.پشت();
                m_workList.برداشت_پشت();
                إذا (!m_vtxp) استمر;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_work_List_شاخص = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                إذا (Dfg_رأس_Var* ثابت varp = m_vtxp->cast<Dfg_رأس_Var>()) {
                    بازدید(varp);
                    استمر;
                }

                // Ast references are not considered
                إذا (m_vtxp->is<Dfg_رأس_Ast>()) استمر;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                إذا (!m_vInfo[m_vtxp].m_is_Cached_رأس) {
                    إذا (Dfg_رأس* ثابت sampep = cache_رأس(m_vtxp)) {
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
            إذا (m_iterList.اندازه() == 1) اكسر;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                اندازه_t begin = 0;
                اندازه_t انتها = m_iterList.اندازه();
                لكل (اندازه_t hops = 1; hops <= 4; ++hops) {
                    لكل (اندازه_t i = begin; i < انتها; ++i) {
                        Dfg_رأس* ثابت vtxp = m_iterList[i];
                        إذا (!vtxp) استمر;
                        vtxp->foreachSink([&](Dfg_رأس& dst) {
                            جمع_کن_مقصد_Iter_List(&dst);
                            أرجع خطأ;
                        });
                        vtxp->foreach_منبع([&](Dfg_رأس& src) {
                            جمع_کن_مقصد_Iter_List(&src);
                            أرجع خطأ;
                        });
                    }
                    begin = انتها;
                    انتها = m_iterList.اندازه();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            لكل (Dfg_رأس* ثابت vtxp : m_iterList) {
                إذا (!vtxp) استمر;
                removeFromIterList(vtxp);
                جمع_کن_مقصد_Work_List(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#لغو_تعریف APPLYING

عام:
    ساكن فراغ apply(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

فراغ V3DfgPasses::peephole(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx) {
    إذا (!v3Global.opt.fDfgPeephole()) أرجع;
    V3DfgPeephole::apply(dfg, ctx);
}
