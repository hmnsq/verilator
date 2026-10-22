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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Ast.h"
#شامل "V3Dfg.h"
#شامل "V3DfgCache.h"
#شامل "V3DfgDataType.h"
#شامل "V3DfgPasses.h"
#شامل "V3DfgPeepholePatterns.h"

#شامل <algorithm>
#شامل <cctype>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
نامخلا {

سانچہ <قسم_نام T_Reduction>
ساخت Reduction_مقصد_Bitwise_Impl {};
سانچہ <> ساخت Reduction_مقصد_Bitwise_Impl<DfgRedAnd> { استعمال type = DfgAnd; };
سانچہ <> ساخت Reduction_مقصد_Bitwise_Impl<DfgRedOr>  { استعمال type = DfgOr;  };
سانچہ <> ساخت Reduction_مقصد_Bitwise_Impl<DfgRedXor> { استعمال type = DfgXor; };
سانچہ <قسم_نام T_Reduction>
استعمال Reduction_مقصد_Bitwise = قسم_نام Reduction_مقصد_Bitwise_Impl<T_Reduction>::type;

سانچہ <قسم_نام T_Bitwise>
ساخت Bitwise_مقصد_Reduction_Impl {};
سانچہ <> ساخت Bitwise_مقصد_Reduction_Impl<DfgAnd> { استعمال type = DfgRedAnd; };
سانچہ <> ساخت Bitwise_مقصد_Reduction_Impl<DfgOr>  { استعمال type = DfgRedOr;  };
سانچہ <> ساخت Bitwise_مقصد_Reduction_Impl<DfgXor> { استعمال type = DfgRedXor; };
سانچہ <قسم_نام T_Reduction>
استعمال Bitwise_مقصد_Reduction = قسم_نام Bitwise_مقصد_Reduction_Impl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
سانچہ<قسم_نام راس_2>
ساخت IsAssociative : عوامی std::false_type {};
سانچہ <> ساخت IsAssociative<DfgAnd> : عوامی std::true_type {};
سانچہ <> ساخت IsAssociative<DfgOr>  : عوامی std::true_type {};
سانچہ <> ساخت IsAssociative<DfgXor> : عوامی std::true_type {};
سانچہ <> ساخت IsAssociative<Dfg_جوڑو> : عوامی std::true_type {};
سانچہ <> ساخت IsAssociative<DfgConcat> : عوامی std::true_type {};
سانچہ <> ساخت IsAssociative<DfgMul> : عوامی std::true_type {};
سانچہ <> ساخت IsAssociative<DfgMulS> : عوامی std::true_type {};

// Commutative binary operators (a op b == b op a)
سانچہ<قسم_نام راس_2>
ساخت IsCommutative : عوامی std::false_type {};
سانچہ <> ساخت IsCommutative<DfgAnd> : عوامی std::true_type {};
سانچہ <> ساخت IsCommutative<DfgOr>  : عوامی std::true_type {};
سانچہ <> ساخت IsCommutative<DfgXor> : عوامی std::true_type {};
سانچہ <> ساخت IsCommutative<Dfg_جوڑو> : عوامی std::true_type {};
سانچہ <> ساخت IsCommutative<DfgEq> : عوامی std::true_type {};
سانچہ <> ساخت IsCommutative<DfgMul> : عوامی std::true_type {};
سانچہ <> ساخت IsCommutative<DfgMulS> : عوامی std::true_type {};
سانچہ <> ساخت IsCommutative<DfgNeq> : عوامی std::true_type {};

// Idempotent binary operators (a op a == a)
سانچہ<قسم_نام راس_2>
ساخت IsIdempotent : عوامی std::false_type {};
سانچہ <> ساخت IsIdempotent<DfgAnd> : عوامی std::true_type {};
سانچہ <> ساخت IsIdempotent<DfgOr>  : عوامی std::true_type {};

// Binary result dtype
سانچہ<قسم_نام راس_2>
ثابت Dfg_ڈیٹا_Type& نتیجہ_DType(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp);
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<Dfg_جوڑو>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgAnd>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgConcat>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(lhsp->چوڑائی() + rhsp->چوڑائی()); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgDiv>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgDivS>       (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgEq>         (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgGt>         (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgGtS>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgGte>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgGteS>       (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgLogAnd>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgLogEq>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgLogIf>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgLogOr>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgLt>         (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgLtS>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgLte>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgLteS>       (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgModDiv>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgModDivS>    (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgMul>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgMulS>       (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgNeq>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس Dfg_ڈیٹا_Type::packed(1); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgOr>         (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgPow>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgPowSS>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgPowSU>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgPowUS>      (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<Dfg_سرکاؤ_L>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<Dfg_سرکاؤ_R>     (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<Dfg_سرکاؤ_RS>    (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgSub>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }
سانچہ <> ثابت Dfg_ڈیٹا_Type& نتیجہ_DType<DfgXor>        (ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) { واپس lhsp->dtype(); }

// Unary constant folding
سانچہ<قسم_نام راس_2> خالی foldOp(V3_نمبر& برآمد, ثابت V3_نمبر& src);
سانچہ <> خالی foldOp<Dfg_گنتی_Ones>  (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.op_گنتی_Ones(src); }
سانچہ <> خالی foldOp<DfgExtend>     (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.op_تفویض(src); }
سانچہ <> خالی foldOp<DfgExtendS>    (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opExtendS(src, src.چوڑائی()); }
سانچہ <> خالی foldOp<DfgLogNot>     (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opLogNot(src); }
سانچہ <> خالی foldOp<DfgNegate>     (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opNegate(src); }
سانچہ <> خالی foldOp<DfgNot>        (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opNot(src); }
سانچہ <> خالی foldOp<DfgOneHot>     (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opOneHot(src); }
سانچہ <> خالی foldOp<DfgOneHot0>    (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opOneHot0(src); }
سانچہ <> خالی foldOp<DfgRedAnd>     (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opRedAnd(src); }
سانچہ <> خالی foldOp<DfgRedOr>      (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opRedOr(src); }
سانچہ <> خالی foldOp<DfgRedXor>     (V3_نمبر& برآمد, ثابت V3_نمبر& src) { برآمد.opRedXor(src); }

// Binary constant folding
سانچہ<قسم_نام راس_2> خالی foldOp(V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
سانچہ <> خالی foldOp<Dfg_جوڑو>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.op_جوڑو(lhs, rhs); }
سانچہ <> خالی foldOp<DfgAnd>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opAnd(lhs, rhs); }
سانچہ <> خالی foldOp<DfgConcat>     (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opConcat(lhs, rhs); }
سانچہ <> خالی foldOp<DfgDiv>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opDiv(lhs, rhs); }
سانچہ <> خالی foldOp<DfgDivS>       (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opDivS(lhs, rhs); }
سانچہ <> خالی foldOp<DfgEq>         (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opEq(lhs, rhs); }
سانچہ <> خالی foldOp<DfgGt>         (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opGt(lhs, rhs); }
سانچہ <> خالی foldOp<DfgGtS>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opGtS(lhs, rhs); }
سانچہ <> خالی foldOp<DfgGte>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opGte(lhs, rhs); }
سانچہ <> خالی foldOp<DfgGteS>       (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opGteS(lhs, rhs); }
سانچہ <> خالی foldOp<DfgLogAnd>     (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opLogAnd(lhs, rhs); }
سانچہ <> خالی foldOp<DfgLogEq>      (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opLogEq(lhs, rhs); }
سانچہ <> خالی foldOp<DfgLogIf>      (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opLogIf(lhs, rhs); }
سانچہ <> خالی foldOp<DfgLogOr>      (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opLogOr(lhs, rhs); }
سانچہ <> خالی foldOp<DfgLt>         (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opLt(lhs, rhs); }
سانچہ <> خالی foldOp<DfgLtS>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opLtS(lhs, rhs); }
سانچہ <> خالی foldOp<DfgLte>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opLte(lhs, rhs); }
سانچہ <> خالی foldOp<DfgLteS>       (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opLteS(lhs, rhs); }
سانچہ <> خالی foldOp<DfgModDiv>     (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opModDiv(lhs, rhs); }
سانچہ <> خالی foldOp<DfgModDivS>    (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opModDivS(lhs, rhs); }
سانچہ <> خالی foldOp<DfgMul>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opMul(lhs, rhs); }
سانچہ <> خالی foldOp<DfgMulS>       (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opMulS(lhs, rhs); }
سانچہ <> خالی foldOp<DfgNeq>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opNeq(lhs, rhs); }
سانچہ <> خالی foldOp<DfgOr>         (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opOr(lhs, rhs); }
سانچہ <> خالی foldOp<DfgPow>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opPow(lhs, rhs); }
سانچہ <> خالی foldOp<DfgPowSS>      (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opPowSS(lhs, rhs); }
سانچہ <> خالی foldOp<DfgPowSU>      (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opPowSU(lhs, rhs); }
سانچہ <> خالی foldOp<DfgPowUS>      (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opPowUS(lhs, rhs); }
سانچہ <> خالی foldOp<Dfg_سرکاؤ_L>     (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.op_سرکاؤ_L(lhs, rhs); }
سانچہ <> خالی foldOp<Dfg_سرکاؤ_R>     (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.op_سرکاؤ_R(lhs, rhs); }
سانچہ <> خالی foldOp<Dfg_سرکاؤ_RS>    (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.op_سرکاؤ_RS(lhs, rhs, lhs.چوڑائی()); }
سانچہ <> خالی foldOp<DfgSub>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opSub(lhs, rhs); }
سانچہ <> خالی foldOp<DfgXor>        (V3_نمبر& برآمد, ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { برآمد.opXor(lhs, rhs); }
}
// clang-format on

جماعت V3DfgPeephole final : عوامی DfgVisitor {
    // TYPES
    ساخت راس_Info final {
        سائز_t m_work_List_اشاریہ = 0;  // Position of this vertx m_workList (0 means not in list)
        سائز_t m_iter_List_اشاریہ = 0;  // Position of this vertx m_iterList (0 means not in list)
        سائز_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        سائز_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        منطقی m_is_Cached_راس = جھوٹ;  // This vertex is the vertex cached for its operation
    };

    // STATE
    Dfg_گراف& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ثابت Dfg_ڈیٹا_Type& m_ثنائی_ہندسہ_DType = Dfg_ڈیٹا_Type::packed(1);  // Common, so grab it up front
    std::سمتیہ<Dfg_راس*> m_workList;  // List of vertices processed in current interation
    std::سمتیہ<Dfg_راس*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<راس_Info> m_vInfo = m_dfg.makeUserMap<راس_Info>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    Dfg_راس* m_vtxp = خالی_اشاریہ;  // Currently considered vertex
    سائز_t m_currentGeneration = 0;  // Current generation number
    سائز_t m_آخری_Id = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ثابت m_tmpScopep = v3Global.rootp()->چوٹی_Scopep()->scopep();

    // STATIC STATE
    ساکن V3DebugBisect s_debugBisect;  // Debug aid

#تعریف_کرو APPLYING(id) اگر (checkApplying(VDfg_Peephole_نمونہ::id))

    // METHODS
    منطقی checkApplying(VDfg_Peephole_نمونہ id) {
        اگر (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) واپس جھوٹ;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_گنتی[id];
        واپس سچ;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    Dfg_راس* cache_راس(Dfg_راس* vtxp) {
        Dfg_راس* ثابت equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_is_Cached_راس || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_is_Cached_راس = !equivp;
        واپس equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    خالی invalidate_راس(Dfg_راس* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_is_Cached_راس = جھوٹ;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    سانچہ <قسم_نام راس_2, قسم_نام... Operands>
    راس_2* get_Or_Create_راس(File_سطر* flp, ثابت Dfg_ڈیٹا_Type& dtype, Operands... operands) {
        راس_2* ثابت vtxp = m_cache.getOrCreate<راس_2, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_is_Cached_راس = سچ;
        واپس vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    سانچہ <قسم_نام راس_2, قسم_نام... Operands>
    راس_2* get_راس(ثابت Dfg_ڈیٹا_Type& dtype, Operands... operands) {
        راس_2* ثابت vtxp = m_cache.get<راس_2>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_is_Cached_راس, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        واپس vtxp;
    }

    خالی incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    خالی جوڑو_مقصد_Work_List(Dfg_راس* vtxp) {
        راس_Info& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        اگر (vInfo.m_work_List_اشاریہ) واپس;
        // Add to work list
        vInfo.m_work_List_اشاریہ = m_workList.سائز();
        m_workList.ڈالو_عقب(vtxp);
    }

    خالی removeFromWorkList(Dfg_راس* vtxp) {
        راس_Info& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_work_List_اشاریہ] = خالی_اشاریہ;
        vInfo.m_work_List_اشاریہ = 0;
    }

    خالی جوڑو_مقصد_Iter_List(Dfg_راس* vtxp) {
        راس_Info& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        اگر (vInfo.m_iter_List_اشاریہ) واپس;
        // Add to iter list
        vInfo.m_iter_List_اشاریہ = m_iterList.سائز();
        m_iterList.ڈالو_عقب(vtxp);
    }

    خالی removeFromIterList(Dfg_راس* vtxp) {
        راس_Info& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iter_List_اشاریہ] = خالی_اشاریہ;
        vInfo.m_iter_List_اشاریہ = 0;
    }

    خالی delete_راس(Dfg_راس* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_work_List_اشاریہ, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ثابت Dfg_راس_Var* ثابت varp = vtxp->cast<Dfg_راس_Var>();
        UASSERT_OBJ(!varp || !varp->has_پچھلا(), vtxp, "Deleting variable consumed via DfgPrev");

        // Invalidate cache entry
        invalidate_راس(vtxp);

        // It might be in the iter list, remove it
        removeFromIterList(vtxp);

        // Gather source vertices - they might be duplicates, make unique using generation number
        incrementGeneration();
        std::سمتیہ<Dfg_راس*> srcps;
        srcps.reserve(vtxp->nInputs());
        vtxp->foreach_ماخذ([&](Dfg_راس& src) {
            // If it's a variable, add to work list to see if it became redundant
            اگر (src.is<Dfg_راس_Var>()) {
                جوڑو_مقصد_Work_List(&src);
                واپس جھوٹ;
            }
            // Gather unique sources
            راس_Info& vInfo = m_vInfo[src];
            اگر (vInfo.m_generation != m_currentGeneration) srcps.ڈالو_عقب(&src);
            vInfo.m_generation = m_currentGeneration;
            واپس جھوٹ;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        اگر (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ڈالو_عقب(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } ورنہ {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ثابت خودکار mid
            = std::stable_تقسیم(srcps.begin(), srcps.انتہا(), [](Dfg_راس* srcp) {  //
                  واپس srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        برائے (خودکار it = srcps.begin(); it != mid; ++it) جوڑو_مقصد_Iter_List(*it);

        // Recursively delete unused sources
        برائے (خودکار it = mid; it != srcps.انتہا(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            delete_راس(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    خالی replace(Dfg_راس* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_work_List_اشاریہ, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ثابت خودکار debugCallback = [&]() -> خالی {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.ماخذ_Cone({m_vtxp, resp});
            ثابت خودکار cone = m_dfg.ماخذ_Cone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ثابت Dfg_راس& v) {  //
                واپس cone->گنتی(&v);
            });
        };
        اگر (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) واپس;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreach_ماخذ([&](Dfg_راس& src) {
            جوڑو_مقصد_Iter_List(&src);
            واپس جھوٹ;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](Dfg_راس& dst) {
            invalidate_راس(&dst);
            واپس جھوٹ;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](Dfg_راس& dst) {
            cache_راس(&dst);
            واپس جھوٹ;
        });

        // Original vertex is now unused, so delete it
        delete_راس(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        جوڑو_مقصد_Iter_List(resp);
        // Add new vertex and sinks to work list as likely to match
        جوڑو_مقصد_Work_List(resp);
        resp->foreachSink([&](Dfg_راس& dst) {
            جوڑو_مقصد_Work_List(&dst);
            واپس جھوٹ;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_سطر* flp, uint32_t val) { واپس نیا DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_سطر* flp, uint32_t چوڑائی) {
        واپس نیا DfgConst{m_dfg, flp, چوڑائی, 0};
    }
    DfgConst* makeZero(File_سطر* flp, ثابت Dfg_ڈیٹا_Type& dtype) {
        واپس makeZero(flp, dtype.سائز());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_سطر* flp, uint32_t چوڑائی) {
        DfgConst* ثابت resp = makeZero(flp, چوڑائی);
        resp->عدد().سیٹ_All_Bits1();
        واپس resp;
    }

    // Create a new vertex of the given type
    سانچہ <قسم_نام راس_2, قسم_نام... Operands>
    راس_2* make(File_سطر* flp, ثابت Dfg_ڈیٹا_Type& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        راس_2* ثابت vtxp = get_Or_Create_راس<راس_2, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        اگر (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        راس_Info& vInfo = m_vInfo[vtxp];
        اگر (!vInfo.m_id) vInfo.m_id = ++m_آخری_Id;
        // Add to work list
        جوڑو_مقصد_Work_List(vtxp);
        // Return new node
        واپس vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    سانچہ <قسم_نام راس_2, قسم_نام... Operands>
    راس_2* make(ثابت Dfg_راس* examplep, Operands... operands) {
        واپس make<راس_2>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    Dfg_راس* replicate(Dfg_راس* vtxp, Dfg_راس* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_ثنائی_ہندسہ_DType, vtxp, "Expected 1-bit vertex");
        اگر (vtxp->dtype() == m_ثنائی_ہندسہ_DType) واپس bitp;
        واپس make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ساکن منطقی isSame(ثابت Dfg_راس* ap, ثابت Dfg_راس* bp) {
        اگر (ap == bp) واپس سچ;
        اگر (ap->dtype() != bp->dtype()) واپس جھوٹ;
        ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>();
        اگر (!aConstp) واپس جھوٹ;
        ثابت DfgConst* ثابت bConstp = bp->cast<DfgConst>();
        اگر (!bConstp) واپس جھوٹ;
        واپس aConstp->عدد().isCaseEq(bConstp->عدد());
    }

    ساکن منطقی isZero(ثابت Dfg_راس* vtxp) {
        اگر (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) واپس constp->isZero();
        واپس جھوٹ;
    }

    ساکن منطقی isOnes(ثابت Dfg_راس* vtxp) {
        اگر (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) واپس constp->isOnes();
        واپس جھوٹ;
    }

    ساکن منطقی isEqOne(ثابت Dfg_راس* vtxp) {
        اگر (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) واپس constp->عدد().isEqOne();
        واپس جھوٹ;
    }

    ساکن منطقی areAdjacent(uint32_t& lsb, ثابت DfgSel* lSelp, ثابت DfgSel* rSelp) {
        اگر (!isSame(lSelp->srcp(), rSelp->srcp())) واپس جھوٹ;
        اگر (lSelp->lsb() + lSelp->چوڑائی() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            واپس سچ;
        }
        اگر (lSelp->lsb() == rSelp->lsb() + rSelp->چوڑائی()) {
            lsb = rSelp->lsb();
            واپس سچ;
        }
        واپس جھوٹ;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    سانچہ <قسم_نام راس_2>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی foldUnary(راس_2* ثابت vtxp) {
        static_assert(std::is_بنیاد_کا<Dfg_راس_Unary, راس_2>::قدر, "Must invoke on unary");
        static_assert(std::is_final<راس_2>::قدر, "Must invoke on final class");
        اگر (DfgConst* ثابت srcp = vtxp->srcp()->سانچہ cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ثابت resultp = makeZero(vtxp->fileline(), vtxp->چوڑائی());
                foldOp<راس_2>(resultp->عدد(), srcp->عدد());
                replace(resultp);
                واپس سچ;
            }
        }
        واپس جھوٹ;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    سانچہ <قسم_نام راس_2>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی ثنائی(راس_2* ثابت vtxp) {
        static_assert(std::is_بنیاد_کا<Dfg_راس_ثنائی, راس_2>::قدر, "Must invoke on binary");
        static_assert(std::is_final<راس_2>::قدر, "Must invoke on final class");

        File_سطر* ثابت flp = vtxp->fileline();
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
        اگر (lConstp && rConstp) {
            APPLYING(FOLD_ثنائی) {
                DfgConst* ثابت resultp = makeZero(flp, نتیجہ_DType<راس_2>(lhsp, rhsp));
                foldOp<راس_2>(resultp->عدد(), lConstp->عدد(), rConstp->عدد());
                replace(resultp);
                واپس سچ;
            }
        }

        // Idempotent
        اگر VL_CONSTEXPR_CXX17 (IsIdempotent<راس_2>::قدر) {
            اگر (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ثنائی) {
                    replace(lhsp);
                    واپس سچ;
                }
            }
        }

        // Associative
        اگر VL_CONSTEXPR_CXX17 (IsAssociative<راس_2>::قدر) {
            // Also commutative and idempotent
            اگر VL_CONSTEXPR_CXX17 (IsCommutative<راس_2>::قدر && IsIdempotent<راس_2>::قدر) {
                اگر (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ثنائی_LHS) {
                        replace(lSamep);
                        واپس سچ;
                    }
                }
                اگر (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ثنائی_RHS) {
                        replace(rSamep);
                        واپس سچ;
                    }
                }
            }

            اگر (lConstp && rSamep) {
                اگر (DfgConst* ثابت rlConstp = rSamep->lhsp()->سانچہ cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ثنائی_LHS_کا_RHS) {
                        DfgConst* ثابت cp = makeZero(flp, نتیجہ_DType<راس_2>(lConstp, rlConstp));
                        foldOp<راس_2>(cp->عدد(), lConstp->عدد(), rlConstp->عدد());
                        replace(make<راس_2>(vtxp, cp, rSamep->rhsp()));
                        واپس سچ;
                    }
                }
            }

            اگر (lSamep && rConstp) {
                اگر (DfgConst* ثابت lrConstp = lSamep->rhsp()->سانچہ cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ثنائی_RHS_کا_LHS) {
                        DfgConst* ثابت cp = makeZero(flp, نتیجہ_DType<راس_2>(lrConstp, rConstp));
                        foldOp<راس_2>(cp->عدد(), lrConstp->عدد(), rConstp->عدد());
                        replace(make<راس_2>(vtxp, lSamep->lhsp(), cp));
                        واپس سچ;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            اگر (lSamep && !lSamep->has_مضرب_Sinks()) {
                Dfg_راس* ثابت ap = lSamep->lhsp();
                Dfg_راس* ثابت bp = lSamep->rhsp();
                Dfg_راس* ثابت cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                اگر (ap != cp && bp != cp) {
                    APPLYING(دایاں_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ثابت Dfg_ڈیٹا_Type& بچہ_DType = نتیجہ_DType<راس_2>(bp, cp);
                        راس_2* ثابت bcp = make<راس_2>(vtxp->fileline(), بچہ_DType, bp, cp);
                        replace(make<راس_2>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        واپس سچ;
                    }
                }
            }

            اگر (rSamep && !rSamep->has_مضرب_Sinks()) {
                Dfg_راس* ثابت rlVtxp = rSamep->lhsp();
                Dfg_راس* ثابت rrVtxp = rSamep->rhsp();

                اگر VL_CONSTEXPR_CXX17 (IsCommutative<راس_2>::قدر) {
                    اگر (!lhsp->has_مضرب_Sinks() && rlVtxp->has_مضرب_Sinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<راس_2>(vtxp, rlVtxp, make<راس_2>(vtxp, lhsp, rrVtxp)));
                            واپس سچ;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                اگر (راس_2* ثابت existingp
                    = get_راس<راس_2>(نتیجہ_DType<راس_2>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    اگر (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ثنائی_LHS_WITH_LHS_کا_RHS) {
                            replace(make<راس_2>(vtxp, existingp, rrVtxp));
                            واپس سچ;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                اگر VL_CONSTEXPR_CXX17 (IsCommutative<راس_2>::قدر) {
                    اگر (راس_2* ثابت existingp
                        = get_راس<راس_2>(نتیجہ_DType<راس_2>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        اگر (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ثنائی_LHS_WITH_RHS_کا_RHS) {
                                replace(make<راس_2>(vtxp, existingp, rlVtxp));
                                واپس سچ;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        اگر VL_CONSTEXPR_CXX17 (IsCommutative<راس_2>::قدر) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            اگر (lConstp) {
                اگر (DfgCond* ثابت rCondp = rhsp->cast<DfgCond>()) {
                    اگر (!rCondp->has_مضرب_Sinks()) {
                        APPLYING(ڈالو_COMMUTATIVE_ثنائی_THROUGH_COND) {
                            Dfg_راس* ثابت tp = make<راس_2>(vtxp, lConstp, rCondp->thenp());
                            Dfg_راس* ثابت ep = make<راس_2>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            واپس سچ;
                        }
                    }
                }
                واپس جھوٹ;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ثابت منطقی تبادلہ_Sides = [&]() {
            // Don't swap if not commutative, duh
            اگر VL_CONSTEXPR_CXX17 (!IsCommutative<راس_2>::قدر) واپس جھوٹ;
            // Const go on left
            اگر (lConstp) واپس جھوٹ;
            اگر (rConstp) واپس سچ;
            // Not go on left
            اگر (lhsp->is<DfgNot>()) واپس جھوٹ;
            اگر (rhsp->is<DfgNot>()) واپس سچ;
            // Same go on right
            اگر (rSamep) واپس جھوٹ;
            اگر (lSamep) واپس سچ;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ثابت راس_Info& lInfo = m_vInfo[lhsp];
            ثابت راس_Info& rInfo = m_vInfo[rhsp];
            واپس lInfo.m_id > rInfo.m_id;
        }();
        اگر (تبادلہ_Sides) {
            APPLYING(تبادلہ_SIDES_اندر_ثنائی) {
                replace(make<راس_2>(vtxp, rhsp, lhsp));
                واپس سچ;
            }
        }

        واپس جھوٹ;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    سانچہ <قسم_نام Other, قسم_نام راس_2>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی distributive_And_Associative_ثنائی(راس_2* vtxp) {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        اگر (!lhsp->has_مضرب_Sinks() && !rhsp->has_مضرب_Sinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            اگر (Other* ثابت lp = lhsp->cast<Other>()) {
                اگر (Other* ثابت rp = rhsp->cast<Other>()) {
                    Dfg_راس* ثابت llp = lp->lhsp();
                    Dfg_راس* ثابت lrp = lp->rhsp();
                    Dfg_راس* ثابت rlp = rp->lhsp();
                    Dfg_راس* ثابت rrp = rp->rhsp();
                    Dfg_راس* ap = خالی_اشاریہ;
                    Dfg_راس* bp = خالی_اشاریہ;
                    Dfg_راس* cp = خالی_اشاریہ;
                    اگر (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } ورنہ اگر (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } ورنہ اگر (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } ورنہ اگر (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    اگر (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_ثنائی) {
                            replace(make<Other>(vtxp, ap, make<راس_2>(lhsp, bp, cp)));
                            واپس سچ;
                        }
                    }
                }
            }
        }

        واپس جھوٹ;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    سانچہ <قسم_نام راس_2>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی
    try_ڈالو_Bitwise_Op_Through_Concat(راس_2* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_سطر* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ثابت catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ثابت catRConstp = concatp->rhsp()->cast<DfgConst>();
        اگر (catLConstp || catRConstp) {
            APPLYING(ڈالو_BITWISE_OP_THROUGH_CONCAT) {
                ثابت uint32_t چوڑائی = concatp->چوڑائی();
                ثابت Dfg_ڈیٹا_Type& lDtype = concatp->lhsp()->dtype();
                ثابت Dfg_ڈیٹا_Type& rDtype = concatp->rhsp()->dtype();
                ثابت uint32_t l_چوڑائی = lDtype.سائز();
                ثابت uint32_t r_چوڑائی = rDtype.سائز();

                // The new Lhs vertex
                Dfg_راس* ثابت newLhsp = [&]() -> Dfg_راس* {
                    DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), l_چوڑائی);
                    اگر (catLConstp) {
                        V3_نمبر عدد{constp->fileline(), static_cast<صحیح>(l_چوڑائی), 0u};
                        عدد.opSel(constp->عدد(), چوڑائی - 1, r_چوڑائی);
                        foldOp<راس_2>(newLhsConstp->عدد(), عدد, catLConstp->عدد());
                        واپس newLhsConstp;
                    }
                    newLhsConstp->عدد().opSel(constp->عدد(), چوڑائی - 1, r_چوڑائی);
                    واپس make<راس_2>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                Dfg_راس* ثابت newRhsp = [&]() -> Dfg_راس* {
                    DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), r_چوڑائی);
                    اگر (catRConstp) {
                        V3_نمبر عدد{constp->fileline(), static_cast<صحیح>(r_چوڑائی), 0u};
                        عدد.opSel(constp->عدد(), r_چوڑائی - 1, 0);
                        foldOp<راس_2>(newRhsConstp->عدد(), عدد, catRConstp->عدد());
                        واپس newRhsConstp;
                    }
                    newRhsConstp->عدد().opSel(constp->عدد(), r_چوڑائی - 1, 0);
                    واپس make<راس_2>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                واپس سچ;
            }
        }
        واپس جھوٹ;
    }

    سانچہ <قسم_نام راس_2>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی
    try_ڈالو_Compare_Op_Through_Concat(راس_2* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_سطر* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        اگر (!concatp->has_مضرب_Sinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ڈالو_COMPARE_OP_THROUGH_CONCAT) {
                ثابت uint32_t چوڑائی = concatp->چوڑائی();
                ثابت uint32_t l_چوڑائی = concatp->lhsp()->چوڑائی();
                ثابت uint32_t r_چوڑائی = concatp->rhsp()->چوڑائی();

                // The new Lhs vertex
                DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), l_چوڑائی);
                newLhsConstp->عدد().opSel(constp->عدد(), چوڑائی - 1, r_چوڑائی);
                راس_2* ثابت newLhsp
                    = make<راس_2>(flp, m_ثنائی_ہندسہ_DType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), r_چوڑائی);
                newRhsConstp->عدد().opSel(constp->عدد(), r_چوڑائی - 1, 0);
                راس_2* ثابت newRhsp
                    = make<راس_2>(flp, m_ثنائی_ہندسہ_DType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                اگر VL_CONSTEXPR_CXX17 (std::is_same<راس_2, DfgEq>::قدر) {
                    replace(make<DfgAnd>(concatp->fileline(), m_ثنائی_ہندسہ_DType, newLhsp, newRhsp));
                } ورنہ اگر VL_CONSTEXPR_CXX17 (std::is_same<راس_2, DfgNeq>::قدر) {
                    replace(make<DfgOr>(concatp->fileline(), m_ثنائی_ہندسہ_DType, newLhsp, newRhsp));
                } ورنہ {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                واپس سچ;
            }
        }
        واپس جھوٹ;
    }

    سانچہ <قسم_نام Bitwise>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی try_ڈالو_Bitwise_Op_Through_Reductions(Bitwise* ثابت vtxp) {
        استعمال Reduction = Bitwise_مقصد_Reduction<Bitwise>;

        اگر (Reduction* ثابت lRedp = vtxp->lhsp()->سانچہ cast<Reduction>()) {
            اگر (Reduction* ثابت rRedp = vtxp->rhsp()->سانچہ cast<Reduction>()) {
                Dfg_راس* ثابت lSrcp = lRedp->srcp();
                Dfg_راس* ثابت rSrcp = rRedp->srcp();
                اگر (lSrcp->dtype() == rSrcp->dtype() && lSrcp->چوڑائی() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->has_مضرب_Sinks() && !rSrcp->has_مضرب_Sinks()) {
                    APPLYING(ڈالو_BITWISE_THROUGH_REDUCTION) {
                        File_سطر* ثابت flp = vtxp->fileline();
                        Bitwise* ثابت bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_ثنائی_ہندسہ_DType, bwp));
                        واپس سچ;
                    }
                }
            }
        }

        واپس جھوٹ;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    ساکن std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(Dfg_راس* vtxp) {
        اگر (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            واپس {خالی_اشاریہ, خالی_اشاریہ};
        }
        اگر (vtxp->has_مضرب_Sinks()) واپس {خالی_اشاریہ, خالی_اشاریہ};
        Dfg_راس_ثنائی* ثابت bwp = vtxp->as<Dfg_راس_ثنائی>();
        DfgSel* ثابت lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ثابت rSelp = bwp->rhsp()->cast<DfgSel>();
        اگر (!lSelp || !rSelp) واپس {خالی_اشاریہ, خالی_اشاریہ};
        DfgSel* ثابت nSelp = lSelp->fromp()->چوڑائی() <= rSelp->fromp()->چوڑائی() ? lSelp : rSelp;
        DfgSel* ثابت wSelp = nSelp == lSelp ? rSelp : lSelp;
        ثابت uint32_t n_چوڑائی = nSelp->fromp()->چوڑائی();
        اگر (n_چوڑائی > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) واپس {خالی_اشاریہ, خالی_اشاریہ};
        اگر (wSelp->lsb() - nSelp->lsb() + n_چوڑائی > wSelp->fromp()->چوڑائی()) {
            واپس {خالی_اشاریہ, خالی_اشاریہ};
        }
        واپس {nSelp, wSelp};
    }

    سانچہ <قسم_نام Bitwise>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی try_ڈالو_Bitwise_Op_Throug_Sel(Bitwise* ثابت vtxp) {
        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        اگر (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = خالی_اشاریہ;
            Dfg_راس* extrap = خالی_اشاریہ;
            اگر (DfgSel* ثابت selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } ورنہ اگر (Bitwise* ثابت bitwisep = rhsp->cast<Bitwise>()) {
                اگر (DfgSel* ثابت rlSelp = bitwisep->lhsp()->سانچہ cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } ورنہ اگر (DfgSel* ثابت rrSelp = bitwisep->rhsp()->سانچہ cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            اگر (rSelp) {
                Dfg_راس* ثابت lFromp = lSelp->fromp();
                Dfg_راس* ثابت rFromp = rSelp->fromp();
                اگر (lFromp->dtype() == rFromp->dtype() && lFromp->چوڑائی() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ڈالو_BITWISE_THROUGH_SEL) {
                        Bitwise* ثابت bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        Dfg_راس* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        اگر (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        واپس سچ;
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
            اگر (rChainp && rChainp->has_مضرب_Sinks()) واپس جھوٹ;
            Dfg_راس* ثابت rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ثابت خودکار lSels = pushableUnalignedSels(lhsp);
            ثابت خودکار rSels = pushableUnalignedSels(rTermp);
            اگر (!lSels.پہلا && !rSels.پہلا) واپس جھوٹ;

            ثابت Dfg_راس* ثابت opp = lSels.پہلا ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            اگر (opp->is<Bitwise>()) واپس جھوٹ;
            ثابت خودکار& sels = lSels.پہلا ? lSels : rSels;
            Dfg_راس* ثابت nFromp = sels.پہلا->fromp();
            Dfg_راس* ثابت wFromp = sels.دوسرا->fromp();
            ثابت uint32_t انحراف = sels.دوسرا->lsb() - sels.پہلا->lsb();

            // Both terms must end up as selects from the same pushed operation
            ثابت خودکار matches = [&](ثابت Dfg_راس* termp, ثابت خودکار& tSels) {
                // Original form: 'n[i] OP w[j]'
                اگر (tSels.پہلا) {
                    واپس termp->type() == opp->type()  //
                           && tSels.پہلا->fromp() == nFromp  //
                           && tSels.دوسرا->fromp() == wFromp  //
                           && tSels.دوسرا->lsb() - tSels.پہلا->lsb() == انحراف;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                اگر (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                اگر (ثابت DfgSel* ثابت selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                اگر (termp->type() != opp->type()) واپس جھوٹ;
                // Operands might have been swapped since
                ثابت Dfg_راس_ثنائی* ثابت bwp = termp->as<Dfg_راس_ثنائی>();
                ثابت منطقی nIsLhs = bwp->lhsp() == nFromp;
                اگر (!nIsLhs && bwp->rhsp() != nFromp) واپس جھوٹ;
                ثابت DfgSel* ثابت slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                واپس slicep && slicep->fromp() == wFromp && slicep->lsb() == انحراف;
            };
            اگر (!matches(lhsp, lSels) || !matches(rTermp, rSels)) واپس جھوٹ;

            APPLYING(ڈالو_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_سطر* ثابت flp = vtxp->fileline();
                ثابت Dfg_ڈیٹا_Type& dtype = nFromp->dtype();
                DfgSel* ثابت slicep = make<DfgSel>(flp, dtype, wFromp, انحراف);
                Dfg_راس* bwp = خالی_اشاریہ;
                اگر (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } ورنہ اگر (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } ورنہ {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ثابت خودکار ڈالو = [&](Dfg_راس* termp, ثابت DfgSel* nSelp) -> Dfg_راس* {
                    واپس nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                Dfg_راس* ثابت newLhsp = ڈالو(lhsp, lSels.پہلا);
                Dfg_راس* ثابت newRTermp = ڈالو(rTermp, rSels.پہلا);
                اگر (rChainp) {
                    Dfg_راس* ثابت newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } ورنہ {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                واپس سچ;
            }
        }

        واپس جھوٹ;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ساکن منطقی isConcatFoldable(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) {
        // Concatenation of the same vertex is a replicate
        اگر (isSame(lhsp, rhsp)) واپس سچ;
        اگر (ثابت DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            اگر (isSame(lRepp->srcp(), rhsp)) واپس سچ;
        }
        اگر (ثابت DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
            اگر (isSame(lhsp, rRepp->srcp())) واپس سچ;
        }
        // Concatenation of adjoining selects is a select
        اگر (ثابت DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            اگر (ثابت DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                واپس isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->چوڑائی();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        اگر (ثابت DfgNot* ثابت lNotp = lhsp->cast<DfgNot>()) {
            اگر (ثابت DfgNot* ثابت rNotp = rhsp->cast<DfgNot>()) {
                واپس !lNotp->has_مضرب_Sinks()  //
                       && !rNotp->has_مضرب_Sinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        واپس جھوٹ;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ساکن منطقی isSameBitwise(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) {
        اگر (lhsp->type() != rhsp->type()) واپس جھوٹ;
        واپس lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ساکن منطقی isConcatSimplifiable(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp) {
        اگر (isConcatFoldable(lhsp, rhsp)) واپس سچ;
        اگر (!isSameBitwise(lhsp, rhsp)) واپس جھوٹ;
        اگر (lhsp->has_مضرب_Sinks() || rhsp->has_مضرب_Sinks()) واپس جھوٹ;
        ثابت Dfg_راس_ثنائی* ثابت lp = lhsp->as<Dfg_راس_ثنائی>();
        ثابت Dfg_راس_ثنائی* ثابت rp = rhsp->as<Dfg_راس_ثنائی>();
        اگر (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            واپس سچ;
        اگر (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            واپس سچ;
        واپس جھوٹ;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ساکن منطقی isConcatPushableThroughBitwise(ثابت Dfg_راس* lhsp, ثابت Dfg_راس* rhsp,
                                               منطقی& تبادلہ) {
        اگر (!isSameBitwise(lhsp, rhsp)) واپس جھوٹ;
        اگر (lhsp->has_مضرب_Sinks() || rhsp->has_مضرب_Sinks()) واپس جھوٹ;
        ثابت Dfg_راس_ثنائی* ثابت lp = lhsp->as<Dfg_راس_ثنائی>();
        ثابت Dfg_راس_ثنائی* ثابت rp = rhsp->as<Dfg_راس_ثنائی>();
        تبادلہ = جھوٹ;
        اگر (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) واپس سچ;
        اگر (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) واپس سچ;
        تبادلہ = سچ;
        اگر (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) واپس سچ;
        اگر (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) واپس سچ;
        واپس جھوٹ;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    Dfg_راس* makeConcatPushedThroughBitwise(File_سطر* flp, ثابت Dfg_راس* lhsp,
                                              ثابت Dfg_راس* rhsp, منطقی تبادلہ) {
        ثابت Dfg_راس_ثنائی* ثابت lp = lhsp->as<Dfg_راس_ثنائی>();
        ثابت Dfg_راس_ثنائی* ثابت rp = rhsp->as<Dfg_راس_ثنائی>();
        ثابت Dfg_ڈیٹا_Type& dtype = Dfg_ڈیٹا_Type::packed(lp->چوڑائی() + rp->چوڑائی());
        Dfg_راس* ثابت cp = تبادلہ ? rp->rhsp() : rp->lhsp();
        Dfg_راس* ثابت dp = تبادلہ ? rp->lhsp() : rp->rhsp();
        DfgConcat* ثابت newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ثابت newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        اگر (lp->is<DfgAnd>()) واپس make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        اگر (lp->is<DfgOr>()) واپس make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        واپس make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    سانچہ <قسم_نام Bitwise>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->چوڑائی() == 1, vtxp, "Width must be 1");
        استعمال Reduction = Bitwise_مقصد_Reduction<Bitwise>;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        اگر (DfgSel* ثابت lSelp = lhsp->سانچہ cast<DfgSel>()) {
            DfgSel* rSelp = خالی_اشاریہ;
            Dfg_راس* extrap = خالی_اشاریہ;
            اگر (DfgSel* ثابت selp = rhsp->سانچہ cast<DfgSel>()) {
                rSelp = selp;
            } ورنہ اگر (Bitwise* ثابت rBitwisep = rhsp->سانچہ cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->سانچہ cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } ورنہ اگر (Reduction* ثابت rRedp = rhsp->سانچہ cast<Reduction>()) {
                rSelp = rRedp->srcp()->سانچہ cast<DfgSel>();
            }
            اگر (rSelp) {
                uint32_t lsb = 0;
                اگر (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_کا_SELS_WITH_REDUCTION) {
                        ثابت Dfg_ڈیٹا_Type& dtype
                            = Dfg_ڈیٹا_Type::packed(lSelp->چوڑائی() + rSelp->چوڑائی());
                        DfgSel* ثابت newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        Dfg_راس* resp = make<Reduction>(vtxp, newSelp);
                        اگر (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        واپس سچ;
                    }
                }
            }
        }

        اگر (Reduction* ثابت lRedp = lhsp->سانچہ cast<Reduction>()) {
            Reduction* rRedp = rhsp->سانچہ cast<Reduction>();
            Dfg_راس* extrap = خالی_اشاریہ;
            اگر (!rRedp) {
                اگر (Bitwise* ثابت rBitwisep = rhsp->سانچہ cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->سانچہ cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            اگر (rRedp) {
                اگر (DfgSel* ثابت lSelp = lRedp->srcp()->سانچہ cast<DfgSel>()) {
                    اگر (DfgSel* ثابت rSelp = rRedp->srcp()->سانچہ cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        اگر (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_کا_REDUCTION_کا_SELS_WITH_REDUCTION) {
                                ثابت Dfg_ڈیٹا_Type& dtype
                                    = Dfg_ڈیٹا_Type::packed(lSelp->چوڑائی() + rSelp->چوڑائی());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                Dfg_راس* resp = make<Reduction>(vtxp, newSelp);
                                اگر (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                واپس سچ;
                            }
                        }
                    }
                }
            }
        }

        واپس جھوٹ;
    }

    سانچہ <قسم_نام Reduction>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی optimizeReduction(Reduction* ثابت vtxp) {
        استعمال Bitwise = Reduction_مقصد_Bitwise<Reduction>;

        اگر (foldUnary(vtxp)) واپس سچ;

        Dfg_راس* ثابت srcp = vtxp->srcp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Reduction of 1-bit value
        اگر (srcp->dtype() == m_ثنائی_ہندسہ_DType) {
            APPLYING(REMOVE_چوڑائی_ONE_REDUCTION) {
                replace(srcp);
                واپس سچ;
            }
        }

        اگر (DfgCond* ثابت condp = srcp->cast<DfgCond>()) {
            اگر (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ڈالو_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ثابت newThenp = make<Reduction>(flp, m_ثنائی_ہندسہ_DType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ثابت newElsep = make<Reduction>(flp, m_ثنائی_ہندسہ_DType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), m_ثنائی_ہندسہ_DType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    واپس سچ;
                }
            }
        }

        اگر (DfgConcat* ثابت concatp = srcp->cast<DfgConcat>()) {
            اگر (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_ثنائی_ہندسہ_DType
                || concatp->rhsp()->dtype() == m_ثنائی_ہندسہ_DType) {
                APPLYING(ڈالو_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ثابت lRedp
                        = make<Reduction>(concatp->fileline(), m_ثنائی_ہندسہ_DType, concatp->lhsp());
                    Reduction* ثابت rRedp
                        = make<Reduction>(concatp->fileline(), m_ثنائی_ہندسہ_DType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_ثنائی_ہندسہ_DType, lRedp, rRedp));
                    واپس سچ;
                }
            }
        }

        اگر (Bitwise* ثابت bitwisep = vtxp->srcp()->سانچہ cast<Bitwise>()) {
            اگر (!bitwisep->has_مضرب_Sinks()) {
                اگر (bitwisep->lhsp()->سانچہ is<DfgConcat>()
                    || bitwisep->rhsp()->سانچہ is<DfgConcat>()) {
                    APPLYING(ڈالو_REDUCTION_THROUGH_BITWISE_کا_CONCAT) {
                        Reduction* ثابت newLhsp
                            = make<Reduction>(flp, m_ثنائی_ہندسہ_DType, bitwisep->lhsp());
                        Reduction* ثابت newRhsp
                            = make<Reduction>(flp, m_ثنائی_ہندسہ_DType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_ثنائی_ہندسہ_DType, newLhsp, newRhsp));
                        واپس سچ;
                    }
                }

                اگر (DfgSel* ثابت lSelp = bitwisep->lhsp()->سانچہ cast<DfgSel>()) {
                    اگر (DfgSel* ثابت rSelp = bitwisep->rhsp()->سانچہ cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        اگر (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ڈالو_REDUCTION_THROUGH_BITWISE_کا_SELS) {
                                ثابت Dfg_ڈیٹا_Type& dtype
                                    = Dfg_ڈیٹا_Type::packed(lSelp->چوڑائی() + rSelp->چوڑائی());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                واپس سچ;
                            }
                        }
                    }
                }
            }
        }

        واپس جھوٹ;
    }

    سانچہ <قسم_نام سرکاؤ>
    VL_ATTR_WARN_غیر_مستعمل_نتیجہ منطقی optimize_سرکاؤ_RHS(سرکاؤ* ثابت vtxp) {
        static_assert(std::is_بنیاد_کا<Dfg_راس_ثنائی, سرکاؤ>::قدر, "Must invoke on binary");
        static_assert(std::is_final<سرکاؤ>::قدر, "Must invoke on final class");
        اگر (ثابت DfgConcat* ثابت concatp = vtxp->rhsp()->سانچہ cast<DfgConcat>()) {
            اگر (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_کا_سرکاؤ) {
                    replace(make<سرکاؤ>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    واپس سچ;
                }
            }
        }
        واپس جھوٹ;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<Dfg_راس*, uint32_t, uint32_t> جوڑو_مقصد_گنتی_Ones_رکن(Dfg_راس* vtxp) {
        اگر (DfgConcat* ثابت oCatp = vtxp->cast<DfgConcat>()) {
            اگر (isZero(oCatp->lhsp())) {
                اگر (Dfg_گنتی_Ones* ثابت گنتی_Onesp = oCatp->rhsp()->cast<Dfg_گنتی_Ones>()) {
                    // Zero extended count ones
                    اگر (DfgSel* ثابت selp = گنتی_Onesp->srcp()->cast<DfgSel>()) {
                        واپس {selp->fromp(), selp->lsb(), selp->چوڑائی()};
                    }
                } ورنہ اگر (DfgSel* ثابت selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    اگر (selp->dtype() == m_ثنائی_ہندسہ_DType) {  //
                        واپس {selp->fromp(), selp->lsb(), selp->چوڑائی()};
                    }
                }
            }
            واپس {خالی_اشاریہ, 0, 0};
        }
        اگر (Dfg_گنتی_Ones* ثابت گنتی_Onesp = vtxp->cast<Dfg_گنتی_Ones>()) {
            // Simple count ones
            اگر (DfgSel* ثابت selp = گنتی_Onesp->srcp()->cast<DfgSel>()) {
                واپس {selp->fromp(), selp->lsb(), selp->چوڑائی()};
            }
            واپس {خالی_اشاریہ, 0, 0};
        }
        اگر (DfgSel* ثابت oSelp = vtxp->cast<DfgSel>()) {
            اگر (oSelp->lsb() == 0) {
                // Truncated count ones
                اگر (Dfg_گنتی_Ones* ثابت گنتی_Onesp = oSelp->fromp()->cast<Dfg_گنتی_Ones>()) {
                    // Zero extended count ones
                    اگر (DfgSel* ثابت selp = گنتی_Onesp->srcp()->cast<DfgSel>()) {
                        واپس {selp->fromp(), selp->lsb(), selp->چوڑائی()};
                    }
                }
            }
            // Single bit select
            اگر (oSelp->dtype() == m_ثنائی_ہندسہ_DType) {  //
                واپس {oSelp->fromp(), oSelp->lsb(), 1};
            }
            واپس {خالی_اشاریہ, 0, 0};
        }
        // Altered form of extended MSB
        اگر (Dfg_سرکاؤ_R* ثابت shiftrp = vtxp->cast<Dfg_سرکاؤ_R>()) {
            اگر (DfgConst* ثابت rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                اگر (rConstp->مقصد_U32() == shiftrp->چوڑائی() - 1) {
                    واپس {shiftrp->lhsp(), shiftrp->چوڑائی() - 1, 1};
                }
            }
            واپس {خالی_اشاریہ, 0, 0};
        }
        // Not applicable
        واپس {خالی_اشاریہ, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<Dfg_راس*, uint32_t> unwindSel(Dfg_راس* fromp, uint32_t lsb,
                                              ثابت uint32_t چوڑائی) {
        جب (سچ) {
            ثابت uint32_t msb = lsb + چوڑائی - 1;

            // Sel from Concat
            اگر (DfgConcat* ثابت concatp = fromp->cast<DfgConcat>()) {
                Dfg_راس* ثابت lhsp = concatp->lhsp();
                Dfg_راس* ثابت rhsp = concatp->rhsp();

                اگر (msb < rhsp->چوڑائی()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_کا_CONCAT) {
                        fromp = rhsp;
                        جاری;
                    }
                } ورنہ اگر (lsb >= rhsp->چوڑائی()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_کا_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->چوڑائی();
                        جاری;
                    }
                }
            }

            اگر (DfgRep* ثابت repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ثابت uint32_t src_چوڑائی = repp->srcp()->چوڑائی();
                اگر (چوڑائی <= src_چوڑائی) {
                    ثابت uint32_t newLsb = lsb % src_چوڑائی;
                    ثابت uint32_t newMsb = newLsb + چوڑائی - 1;
                    اگر (newMsb < src_چوڑائی) {
                        APPLYING(ڈالو_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            جاری;
                        }
                    }
                }
            }

            // Sel from Sel
            اگر (DfgSel* ثابت selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    جاری;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            اگر (DfgVarPacked* ثابت varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ثابت خودکار pair = varp->driver_کا_Range(lsb, چوڑائی);
                Dfg_راس* ثابت driverp = pair.پہلا;
                ثابت uint32_t driverLsb = pair.دوسرا;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                اگر (driverp) {
                    اگر (driverp == varp->defaultp()) {
                        APPLYING(ڈالو_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            جاری;
                        }
                    } ورنہ {
                        APPLYING(ڈالو_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            جاری;
                        }
                    }
                }
            }

            // No patterns matched, stop
            توڑو;
        }
        واپس {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_راس*, uint32_t> commonLSBs(Dfg_راس* ap, Dfg_راس* bp) {
        اگر (ap == bp) واپس {ap, ap->چوڑائی()};

        // If both constants, check LSBs
        اگر (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            اگر (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت V3_نمبر& a_عدد = aConstp->عدد();
                ثابت V3_نمبر& b_عدد = bConstp->عدد();
                // Max match is the shorter constant
                ثابت uint32_t max_مطابقت = std::min(aConstp->چوڑائی(), bConstp->چوڑائی());
                // Check all bits
                uint32_t مطابقت_چوڑائی = 0;
                برائے (; مطابقت_چوڑائی < max_مطابقت; ++مطابقت_چوڑائی) {
                    اگر (a_عدد.ثنائی_ہندسہ_Is0(مطابقت_چوڑائی) != b_عدد.ثنائی_ہندسہ_Is0(مطابقت_چوڑائی)) توڑو;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aConstp->چوڑائی() < bConstp->چوڑائی() ? aConstp : bConstp;
                واپس {مطابقت_چوڑائی ? shorterp : خالی_اشاریہ, مطابقت_چوڑائی};
            }
        }

        // If Concat, check against the RHS
        اگر (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) واپس commonLSBs(catp->rhsp(), bp);
        اگر (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) واپس commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        اگر (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            اگر (selp->lsb() == 0) {
                Dfg_راس* ثابت fromp = selp->fromp();
                ثابت std::pair<Dfg_راس*, uint32_t> common = commonLSBs(fromp, bp);
                واپس {common.پہلا, std::min(common.دوسرا, selp->چوڑائی())};
            }
        }
        اگر (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            اگر (selp->lsb() == 0) {
                Dfg_راس* ثابت fromp = selp->fromp();
                ثابت std::pair<Dfg_راس*, uint32_t> common = commonLSBs(ap, fromp);
                واپس {common.پہلا, std::min(common.دوسرا, selp->چوڑائی())};
            }
        }

        // Otherwise no common LSBs
        واپس {خالی_اشاریہ, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_راس*, uint32_t> commonMSBs(Dfg_راس* ap, Dfg_راس* bp) {
        اگر (ap == bp) واپس {ap, ap->چوڑائی()};

        // If both constants, check MSBs
        اگر (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            اگر (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت uint32_t aMsb = aConstp->چوڑائی() - 1;
                ثابت uint32_t bMsb = bConstp->چوڑائی() - 1;
                ثابت V3_نمبر& a_عدد = aConstp->عدد();
                ثابت V3_نمبر& b_عدد = bConstp->عدد();
                // Max match is the shorter constant
                ثابت uint32_t max_مطابقت = std::min(aConstp->چوڑائی(), bConstp->چوڑائی());
                // Check all bits
                uint32_t مطابقت_چوڑائی = 0;
                برائے (; مطابقت_چوڑائی < max_مطابقت; ++مطابقت_چوڑائی) {
                    اگر (a_عدد.ثنائی_ہندسہ_Is0(aMsb - مطابقت_چوڑائی) != b_عدد.ثنائی_ہندسہ_Is0(bMsb - مطابقت_چوڑائی)) توڑو;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aMsb < bMsb ? aConstp : bConstp;
                واپس {مطابقت_چوڑائی ? shorterp : خالی_اشاریہ, مطابقت_چوڑائی};
            }
        }

        // If Concat, check against the LHS
        اگر (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) واپس commonMSBs(catp->lhsp(), bp);
        اگر (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) واپس commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        اگر (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            Dfg_راس* ثابت fromp = selp->fromp();
            اگر (selp->msb() == fromp->چوڑائی() - 1) {
                ثابت std::pair<Dfg_راس*, uint32_t> common = commonMSBs(fromp, bp);
                واپس {common.پہلا, std::min(common.دوسرا, selp->چوڑائی())};
            }
        }
        اگر (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            Dfg_راس* ثابت fromp = selp->fromp();
            اگر (selp->msb() == fromp->چوڑائی() - 1) {
                ثابت std::pair<Dfg_راس*, uint32_t> common = commonMSBs(ap, fromp);
                واپس {common.پہلا, std::min(common.دوسرا, selp->چوڑائی())};
            }
        }

        // Otherwise no common MSBs
        واپس {خالی_اشاریہ, 0};
    }

    // VISIT methods

    خالی دورہ(Dfg_راس*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    خالی دورہ(Dfg_گنتی_Ones* ثابت vtxp) override {
        اگر (foldUnary(vtxp)) واپس;
    }

    خالی دورہ(DfgExtend* ثابت vtxp) override {
        اگر (foldUnary(vtxp)) واپس;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            Dfg_راس* ثابت zerop
                = makeZero(vtxp->fileline(), vtxp->چوڑائی() - vtxp->srcp()->چوڑائی());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            واپس;
        }
    }

    خالی دورہ(DfgExtendS* ثابت vtxp) override {
        اگر (foldUnary(vtxp)) واپس;
    }

    خالی دورہ(DfgLogNot* ثابت vtxp) override {
        اگر (foldUnary(vtxp)) واپس;
    }

    خالی دورہ(DfgNegate* ثابت vtxp) override {
        اگر (foldUnary(vtxp)) واپس;
    }

    خالی دورہ(DfgNot* ثابت vtxp) override {
        اگر (foldUnary(vtxp)) واپس;

        // Not of Cond
        اگر (DfgCond* ثابت condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            اگر (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ڈالو_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ثابت newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ثابت newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    واپس;
                }
            }
        }

        // Not of Not
        اگر (DfgNot* ثابت notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                واپس;
            }
        }

        اگر (!vtxp->srcp()->has_مضرب_Sinks()) {
            // Not of Eq
            اگر (DfgEq* ثابت eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    واپس;
                }
            }

            // Not of Neq
            اگر (DfgNeq* ثابت neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    واپس;
                }
            }

            // Not of unsigned comparisons
            اگر (DfgLt* ثابت p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    واپس;
                }
            }
            اگر (DfgGte* ثابت p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    واپس;
                }
            }
            اگر (DfgGt* ثابت p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    واپس;
                }
            }
            اگر (DfgLte* ثابت p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    واپس;
                }
            }

            // Not of signed comparisons
            اگر (DfgLtS* ثابت p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    واپس;
                }
            }
            اگر (DfgGteS* ثابت p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    واپس;
                }
            }
            اگر (DfgGtS* ثابت p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    واپس;
                }
            }
            اگر (DfgLteS* ثابت p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    واپس;
                }
            }
        }
    }

    خالی دورہ(DfgOneHot* ثابت vtxp) override {
        اگر (foldUnary(vtxp)) واپس;
    }

    خالی دورہ(DfgOneHot0* ثابت vtxp) override {
        اگر (foldUnary(vtxp)) واپس;
    }

    خالی دورہ(DfgRedOr* ثابت vtxp) override {
        اگر (optimizeReduction(vtxp)) واپس;
    }

    خالی دورہ(DfgRedAnd* ثابت vtxp) override {
        اگر (optimizeReduction(vtxp)) واپس;
    }

    خالی دورہ(DfgRedXor* ثابت vtxp) override {
        اگر (optimizeReduction(vtxp)) واپس;
    }

    خالی دورہ(DfgSel* ثابت vtxp) override {
        Dfg_راس* ثابت fromp = vtxp->fromp();

        File_سطر* ثابت flp = vtxp->fileline();

        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t چوڑائی = vtxp->چوڑائی();
        ثابت uint32_t msb = lsb + چوڑائی - 1;

        اگر (DfgConst* ثابت constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ثابت resp = makeZero(flp, چوڑائی);
                resp->عدد().opSel(constp->عدد(), msb, lsb);
                replace(resp);
                واپس;
            }
        }

        // Full width select, replace with the source.
        اگر (fromp->چوڑائی() == چوڑائی) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_بھرا_چوڑائی_SEL) {
                replace(fromp);
                واپس;
            }
        }

        // Unwind through bit packing in one go
        {
            ثابت خودکار res = unwindSel(fromp, lsb, چوڑائی);
            اگر (res.پہلا != fromp || res.دوسرا != lsb) {
                replace(make<DfgSel>(vtxp, res.پہلا, res.دوسرا));
                واپس;
            }
        }

        // Sel from Not
        اگر (DfgNot* ثابت notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            اگر (!notp->has_مضرب_Sinks()) {
                APPLYING(ڈالو_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ثابت newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    واپس;
                }
            }
        }

        // Sel from Cond
        اگر (DfgCond* ثابت condp = fromp->cast<DfgCond>()) {
            اگر (!condp->has_مضرب_Sinks()) {
                APPLYING(ڈالو_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ثابت newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ثابت newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    واپس;
                }
            }
        }

        // Sel from ShiftL
        اگر (Dfg_سرکاؤ_L* ثابت سرکاؤ_Lp = fromp->cast<Dfg_سرکاؤ_L>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            اگر (lsb == 0) {
                UASSERT_OBJ(سرکاؤ_Lp->lhsp()->چوڑائی() >= چوڑائی, vtxp, "input of shift narrow");
                APPLYING(ڈالو_SEL_THROUGH_SHIFTL) {
                    DfgSel* ثابت newSelp = make<DfgSel>(vtxp, سرکاؤ_Lp->lhsp(), vtxp->lsb());
                    replace(make<Dfg_سرکاؤ_L>(vtxp, newSelp, سرکاؤ_Lp->rhsp()));
                    واپس;
                }
            }
        }
    }

    خالی دورہ(DfgMux* ثابت vtxp) override {
        Dfg_راس* ثابت fromp = vtxp->fromp();
        Dfg_راس* ثابت lsbp = vtxp->lsbp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (DfgConst* ثابت lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->عدد().مقصد_UInt()));
                واپس;
            }
        }

        اگر (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->چوڑائی()));
                واپس;
            }
        }

        اگر (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->چوڑائی()));
                واپس;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    خالی دورہ(DfgAnd* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        اگر (!lhsp->has_مضرب_Sinks() || !rhsp->has_مضرب_Sinks()) {
            اگر (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                اگر (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_کا_NOT_AND_NOT) {
                        DfgOr* ثابت orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        واپس;
                    }
                }
                اگر (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_کا_NOT_AND_NEQ) {
                        DfgEq* ثابت newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ثابت orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        واپس;
                    }
                }
            }
        }

        اگر (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            اگر (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    واپس;
                }
            }

            اگر (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    واپس;
                }
            }

            اگر (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                اگر (try_ڈالو_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) واپس;
            }
        }

        اگر (distributive_And_Associative_ثنائی<DfgOr, DfgAnd>(vtxp)) واپس;

        اگر (try_ڈالو_Bitwise_Op_Through_Reductions(vtxp)) واپس;

        اگر (try_ڈالو_Bitwise_Op_Throug_Sel(vtxp)) واپس;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            اگر ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->چوڑائی()));
                    واپس;
                }
            }

            اگر (DfgAnd* ثابت rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ثابت rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ثابت rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                اگر ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->چوڑائی()));
                        واپس;
                    }
                }
            }
        }

        اگر (vtxp->dtype() == m_ثنائی_ہندسہ_DType) {
            اگر (tryReplaceBitwiseWithReduction(vtxp)) واپس;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            اگر (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            اگر (repp && condp && repp->srcp()->سائز() == 1 && !condp->has_مضرب_Sinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ثابت newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    واپس;
                }
            }
        }
    }

    خالی دورہ(DfgOr* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        اگر (!lhsp->has_مضرب_Sinks() || !rhsp->has_مضرب_Sinks()) {
            اگر (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                اگر (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_کا_NOT_AND_NOT) {
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        واپس;
                    }
                }
                اگر (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_کا_NOT_AND_NEQ) {
                        DfgEq* ثابت newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        واپس;
                    }
                }
            }
        }

        اگر (DfgConcat* ثابت lhsConcatp = lhsp->cast<DfgConcat>()) {
            اگر (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                اگر (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    اگر (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_کا_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            واپس;
                        }
                    }
                    اگر (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_کا_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            واپس;
                        }
                    }
                }
            }
        }

        اگر (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            اگر (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    واپس;
                }
            }

            اگر (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    واپس;
                }
            }

            اگر (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                اگر (try_ڈالو_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) واپس;
            }
        }

        اگر (distributive_And_Associative_ثنائی<DfgAnd, DfgOr>(vtxp)) واپس;

        اگر (try_ڈالو_Bitwise_Op_Through_Reductions(vtxp)) واپس;

        اگر (try_ڈالو_Bitwise_Op_Throug_Sel(vtxp)) واپس;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            اگر ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->چوڑائی()));
                    واپس;
                }
            }

            اگر (DfgOr* ثابت rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ثابت rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ثابت rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                اگر ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->چوڑائی()));
                        واپس;
                    }
                }
            }
        }

        اگر (vtxp->dtype() == m_ثنائی_ہندسہ_DType) {
            اگر (tryReplaceBitwiseWithReduction(vtxp)) واپس;
        }
    }

    خالی دورہ(DfgXor* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->چوڑائی()));
                واپس;
            }
        }

        اگر (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            اگر (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    واپس;
                }
            }
            اگر (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    واپس;
                }
            }
            اگر (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                اگر (try_ڈالو_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) واپس;
            }
        }

        اگر (try_ڈالو_Bitwise_Op_Through_Reductions(vtxp)) واپس;

        اگر (try_ڈالو_Bitwise_Op_Throug_Sel(vtxp)) واپس;

        اگر (vtxp->dtype() == m_ثنائی_ہندسہ_DType) {
            اگر (tryReplaceBitwiseWithReduction(vtxp)) واپس;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    خالی دورہ(Dfg_جوڑو* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isZero(lhsp)) {
            APPLYING(REMOVE_جوڑو_ZERO) {
                replace(rhsp);
                واپس;
            }
        }

        ثابت std::tuple<Dfg_راس*, uint32_t, uint32_t> l_رکن = جوڑو_مقصد_گنتی_Ones_رکن(lhsp);
        اگر (Dfg_راس* ثابت lVtxp = std::get<0>(l_رکن)) {
            std::tuple<Dfg_راس*, uint32_t, uint32_t> r_رکن = جوڑو_مقصد_گنتی_Ones_رکن(rhsp);
            Dfg_راس* extrap = خالی_اشاریہ;
            اگر (!std::get<0>(r_رکن)) {
                اگر (Dfg_جوڑو* ثابت rAddp = rhsp->cast<Dfg_جوڑو>()) {
                    r_رکن = جوڑو_مقصد_گنتی_Ones_رکن(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            اگر (Dfg_راس* ثابت rVtxp = std::get<0>(r_رکن)) {
                اگر (isSame(lVtxp, rVtxp)) {
                    ثابت uint32_t lLsb = std::get<1>(l_رکن);
                    ثابت uint32_t rLsb = std::get<1>(r_رکن);
                    ثابت uint32_t l_چوڑائی = std::get<2>(l_رکن);
                    ثابت uint32_t r_چوڑائی = std::get<2>(r_رکن);
                    منطقی adjoined = سچ;
                    uint32_t lsb = 0;
                    اگر (lLsb + l_چوڑائی == rLsb) {
                        lsb = lLsb;
                    } ورنہ اگر (lLsb == rLsb + r_چوڑائی) {
                        lsb = rLsb;
                    } ورنہ {
                        adjoined = جھوٹ;
                    }
                    اگر (adjoined) {
                        APPLYING(REPLACE_جوڑو_WITH_گنتی_ONES) {
                            DfgSel* ثابت selp
                                = make<DfgSel>(vtxp->fileline(),
                                               Dfg_ڈیٹا_Type::packed(l_چوڑائی + r_چوڑائی), lVtxp, lsb);
                            Dfg_راس* resp
                                = make<Dfg_گنتی_Ones>(flp, Dfg_ڈیٹا_Type::packed(32), selp);
                            اگر (vtxp->چوڑائی() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->چوڑائی() - 32U),
                                                       resp);
                            } ورنہ اگر (vtxp->چوڑائی() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            اگر (extrap) resp = make<Dfg_جوڑو>(vtxp, resp, extrap);
                            replace(resp);
                            واپس;
                        }
                    }
                }
            }
        }
    }

    خالی دورہ(DfgArraySel* ثابت vtxp) override {
        DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>();
        اگر (!idxp) واپس;
        DfgVarArray* ثابت varp = vtxp->fromp()->cast<DfgVarArray>();
        اگر (!varp) واپس;
        AstVar* ثابت astVarp = varp->vscp()->varp();
        اگر (astVarp->isForced()) واپس;
        اگر (astVarp->isSigUserRWPublic()) واپس;
        Dfg_راس* ثابت srcp = varp->srcp();
        اگر (!srcp) {
            اگر (vtxp->isPacked()) {
                اگر (AstConsArray* ثابت iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    اگر (AstConst* ثابت valp = VN_CAST(iap->valuep(idxp->مقصد_سائز_T()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_جدول) {
                            replace(نیا DfgConst{m_dfg, valp->fileline(), valp->عدد()});
                            واپس;
                        }
                    }
                }
            }
            واپس;
        }

        اگر (DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            Dfg_راس* ثابت driverp = splicep->driverAt(idxp->مقصد_سائز_T());
            اگر (!driverp) واپس;
            DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>();
            اگر (!uap) واپس;
            اگر (uap->srcp()->is<Dfg_راس_Splice>()) واپس;
            // If driven by a variable that had a Driver in DFG, it is partial
            اگر (Dfg_راس_Var* ثابت dvarp = uap->srcp()->cast<Dfg_راس_Var>()) {
                اگر (dvarp->srcp()) واپس;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                واپس;
            }
        }

        اگر (DfgUnitArray* ثابت uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->مقصد_سائز_T() == 0, vtxp, "Array index out of range");
            اگر (uap->srcp()->is<DfgSplicePacked>()) واپس;
            // If driven by a variable that had a Driver in DFG, it is partial
            اگر (Dfg_راس_Var* ثابت dvarp = uap->srcp()->cast<Dfg_راس_Var>()) {
                اگر (dvarp->srcp()) واپس;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                واپس;
            }
        }
    }

    خالی دورہ(DfgConcat* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isZero(lhsp)) {
            DfgConst* ثابت lConstp = lhsp->as<DfgConst>();
            اگر (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                اگر (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->چوڑائی()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_چوٹی_WITH_SHIFTR) {
                        replace(
                            make<Dfg_سرکاؤ_R>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->چوڑائی())));
                        واپس;
                    }
                }
            }
        }

        اگر (isZero(rhsp)) {
            DfgConst* ثابت rConstp = rhsp->as<DfgConst>();
            اگر (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                اگر (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<Dfg_سرکاؤ_L>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->چوڑائی())));
                        واپس;
                    }
                }
            }
        }

        اگر (DfgNot* ثابت lNot = lhsp->cast<DfgNot>()) {
            اگر (DfgNot* ثابت rNot = rhsp->cast<DfgNot>()) {
                اگر (!lNot->has_مضرب_Sinks() && !rNot->has_مضرب_Sinks()) {
                    APPLYING(ڈالو_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ثابت newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        واپس;
                    }
                }
            }
        }

        {
            منطقی تبادلہ = جھوٹ;
            اگر (isConcatPushableThroughBitwise(lhsp, rhsp, تبادلہ)) {
                APPLYING(ڈالو_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, تبادلہ));
                    واپس;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            اگر (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                Dfg_راس* ثابت rlp = rConcatp->lhsp();
                اگر (!rConcatp->has_مضرب_Sinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, تبادلہ)) {
                    APPLYING(ڈالو_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        Dfg_راس* ثابت newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, تبادلہ);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        واپس;
                    }
                }
            }
        }

        اگر (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            اگر (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                اگر (isSame(lSelp->fromp(), rSelp->fromp())) {
                    اگر (lSelp->lsb() == rSelp->lsb() + rSelp->چوڑائی()) {
                        APPLYING(REMOVE_CONCAT_کا_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            واپس;
                        }
                    }
                }
            }

            اگر (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                اگر (DfgSel* ثابت rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    اگر (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        اگر (lSelp->lsb() == rlSelp->lsb() + rlSelp->چوڑائی()) {
                            APPLYING(REPLACE_NESTED_CONCAT_کا_ADJOINING_SELS_ON_LHS) {
                                ثابت uint32_t چوڑائی = lSelp->چوڑائی() + rlSelp->چوڑائی();
                                ثابت Dfg_ڈیٹا_Type& dtype = Dfg_ڈیٹا_Type::packed(چوڑائی);
                                DfgSel* ثابت selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                واپس;
                            }
                        }
                    }
                }
            }
        }

        اگر (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
            اگر (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                اگر (DfgSel* ثابت lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    اگر (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        اگر (lrSelp->lsb() == rSelp->lsb() + rSelp->چوڑائی()) {
                            APPLYING(REPLACE_NESTED_CONCAT_کا_ADJOINING_SELS_ON_RHS) {
                                ثابت uint32_t چوڑائی = lrSelp->چوڑائی() + rSelp->چوڑائی();
                                ثابت Dfg_ڈیٹا_Type& dtype = Dfg_ڈیٹا_Type::packed(چوڑائی);
                                DfgSel* ثابت selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                واپس;
                            }
                        }
                    }
                }
            }
        }

        اگر (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            اگر (DfgCond* ثابت rCondp = rhsp->cast<DfgCond>()) {
                Dfg_راس* ثابت rtVtxp = rCondp->thenp();
                Dfg_راس* ثابت reVtxp = rCondp->elsep();
                DfgConst* ثابت rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ثابت reConstp = reVtxp->cast<DfgConst>();
                اگر (!rCondp->has_مضرب_Sinks() && (rtConstp || reConstp)) {
                    APPLYING(ڈالو_CONCAT_THROUGH_COND_LHS) {
                        Dfg_راس* ثابت thenp = [&]() -> Dfg_راس* {
                            File_سطر* ثابت rtFlp = rtVtxp->fileline();
                            اگر (rtConstp) {
                                DfgConst* ثابت constp = makeZero(rtFlp, vtxp->چوڑائی());
                                constp->عدد().opConcat(lConstp->عدد(), rtConstp->عدد());
                                واپس constp;
                            }
                            واپس make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        Dfg_راس* ثابت elsep = [&]() -> Dfg_راس* {
                            File_سطر* ثابت reFlp = reVtxp->fileline();
                            اگر (reConstp) {
                                DfgConst* ثابت constp = makeZero(reFlp, vtxp->چوڑائی());
                                constp->عدد().opConcat(lConstp->عدد(), reConstp->عدد());
                                واپس constp;
                            }
                            واپس make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        واپس;
                    }
                }
            }
        }

        اگر (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            اگر (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                Dfg_راس* ثابت ltVtxp = lCondp->thenp();
                Dfg_راس* ثابت leVtxp = lCondp->elsep();
                DfgConst* ثابت ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ثابت leConstp = leVtxp->cast<DfgConst>();
                اگر (!lCondp->has_مضرب_Sinks() && (ltConstp || leConstp)) {
                    APPLYING(ڈالو_CONCAT_THROUGH_COND_RHS) {
                        Dfg_راس* ثابت thenp = [&]() -> Dfg_راس* {
                            File_سطر* ثابت ltFlp = ltVtxp->fileline();
                            اگر (ltConstp) {
                                DfgConst* ثابت constp = makeZero(ltFlp, vtxp->چوڑائی());
                                constp->عدد().opConcat(ltConstp->عدد(), rConstp->عدد());
                                واپس constp;
                            }
                            واپس make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        Dfg_راس* ثابت elsep = [&]() -> Dfg_راس* {
                            File_سطر* ثابت leFlp = leVtxp->fileline();
                            اگر (leConstp) {
                                DfgConst* ثابت constp = makeZero(leFlp, vtxp->چوڑائی());
                                constp->عدد().opConcat(leConstp->عدد(), rConstp->عدد());
                                واپس constp;
                            }
                            واپس make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        واپس;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ثابت uint32_t vMsb = vtxp->چوڑائی() - 1;  // MSB of the concatenation
            ثابت uint32_t lLsb = vtxp->rhsp()->چوڑائی();  // LSB of the LHS
            ثابت uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            منطقی hasCrossSink = جھوٹ;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](Dfg_راس& sink) {
                // Record bits used by DfgSel sinks
                اگر (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    ثابت uint32_t selLsb = selp->lsb();
                    ثابت uint32_t selMsb = selLsb + selp->چوڑائی() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    واپس جھوٹ;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                اگر (ثابت DfgVarPacked* ثابت varp = sink.cast<DfgVarPacked>()) {
                    اگر (!varp->hasSinks() && !varp->isObserved()) واپس جھوٹ;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                واپس سچ;
            });
            اگر (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    Dfg_راس* nRhsp = rhsp;
                    اگر (lsb != 0)
                        nRhsp = make<DfgSel>(flp, Dfg_ڈیٹا_Type::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    Dfg_راس* nLhsp = lhsp;
                    اگر (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, Dfg_ڈیٹا_Type::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    Dfg_راس* ثابت catp
                        = make<DfgConcat>(flp, Dfg_ڈیٹا_Type::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ثابت sp = نیا DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_آخری_Id;
                    sp->جوڑو_Driver(catp, lsb, flp);
                    Dfg_راس_Var* ثابت varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_آخری_Id;
                    varp->vscp()->varp()->is_اندرونی(سچ);
                    varp->srcp(sp);
                    replace(varp);
                    واپس;
                }
            }
        }

        // Convert various forms to REPLICATE
        اگر (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                واپس;
            }
        }
        اگر (DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            اگر (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    واپس;
                }
            }
        }
        اگر (DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
            اگر (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    واپس;
                }
            }
        }
        اگر (DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            اگر (DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
                اگر (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        واپس;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            اگر (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                اگر (DfgRep* ثابت rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    اگر (!rConcatp->has_مضرب_Sinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ثابت uint32_t چوڑائی = lRepp->چوڑائی() + rlRepp->چوڑائی();
                            ثابت Dfg_ڈیٹا_Type& dtype = Dfg_ڈیٹا_Type::packed(چوڑائی);
                            DfgRep* ثابت repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            واپس;
                        }
                    }
                }
            }
        }
    }

    خالی دورہ(DfgDiv* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        اگر (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            اگر (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    واپس;
                }
            }
        }
    }

    خالی دورہ(DfgDivS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        اگر (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            اگر (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    واپس;
                }
            }
        }
    }

    خالی دورہ(DfgGt* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                واپس;
            }
        }
    }

    خالی دورہ(DfgGtS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                واپس;
            }
        }
    }

    خالی دورہ(DfgGte* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                واپس;
            }
        }
    }

    خالی دورہ(DfgGteS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                واپس;
            }
        }
    }

    خالی دورہ(DfgLogAnd* ثابت vtxp) override {
        اگر (ثنائی(vtxp) || vtxp->rhsp()->unsafe()) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        اگر (lhsp->چوڑائی() == 1 && rhsp->چوڑائی() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                واپس;
            }
        }
    }

    خالی دورہ(DfgLogEq* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
    }

    خالی دورہ(DfgLogIf* ثابت vtxp) override {
        اگر (ثنائی(vtxp) || vtxp->rhsp()->unsafe()) واپس;
    }

    خالی دورہ(DfgLogOr* ثابت vtxp) override {
        اگر (ثنائی(vtxp) || vtxp->rhsp()->unsafe()) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        اگر (lhsp->چوڑائی() == 1 && rhsp->چوڑائی() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                واپس;
            }
        }
    }

    خالی دورہ(DfgLt* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                واپس;
            }
        }
    }

    خالی دورہ(DfgLtS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                واپس;
            }
        }
    }

    خالی دورہ(DfgLte* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                واپس;
            }
        }
    }

    خالی دورہ(DfgLteS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                واپس;
            }
        }
    }

    خالی دورہ(DfgModDiv* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
    }

    خالی دورہ(DfgModDivS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
    }

    خالی دورہ(DfgMul* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            اگر (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    واپس;
                }
            }
            اگر (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    واپس;
                }
            }
        }
    }

    خالی دورہ(DfgMulS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            اگر (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    واپس;
                }
            }
            اگر (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    واپس;
                }
            }
        }
    }

    خالی دورہ(DfgEq* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                واپس;
            }
        }

        اگر (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            اگر (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                اگر (try_ڈالو_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) واپس;
            }

            اگر (rhsp->dtype() == m_ثنائی_ہندسہ_DType) {
                اگر (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_ثنائی_ہندسہ_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        واپس;
                    }
                }

                اگر (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_ثنائی_ہندسہ_1) {
                        replace(rhsp);
                        واپس;
                    }
                }
            }
        }
    }

    خالی دورہ(DfgNeq* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                واپس;
            }
        }

        اگر (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            اگر (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                اگر (try_ڈالو_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) واپس;
            }

            اگر (rhsp->dtype() == m_ثنائی_ہندسہ_DType) {
                اگر (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_ثنائی_ہندسہ_0) {
                        replace(rhsp);
                        واپس;
                    }
                }

                اگر (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_ثنائی_ہندسہ_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        واپس;
                    }
                }
            }
        }
    }

    خالی دورہ(DfgPow* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
    }

    خالی دورہ(DfgPowSS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
    }

    خالی دورہ(DfgPowSU* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
    }

    خالی دورہ(DfgPowUS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
    }

    خالی دورہ(DfgRep* ثابت vtxp) override {
        Dfg_راس* ثابت srcp = vtxp->srcp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (DfgConst* ثابت sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ثابت resultp = makeZero(flp, vtxp->چوڑائی());
                resultp->عدد().opRepl(sConstp->عدد(), vtxp->گنتی());
                replace(resultp);
                واپس;
            }
        }

        اگر (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                واپس;
            }
        }

        اگر (DfgRep* ثابت sRepp = srcp->cast<DfgRep>()) {
            اگر (!sRepp->has_مضرب_Sinks() || vtxp->چوڑائی() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    واپس;
                }
            }
        }
    }

    خالی دورہ(Dfg_سرکاؤ_L* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
        اگر (optimize_سرکاؤ_RHS(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        اگر (Dfg_سرکاؤ_L* ثابت l_سرکاؤ_Lp = lhsp->cast<Dfg_سرکاؤ_L>()) {
            اگر (!l_سرکاؤ_Lp->has_مضرب_Sinks() && rhsp->dtype() == l_سرکاؤ_Lp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_سطر* ثابت flp = vtxp->fileline();
                    ثابت Dfg_ڈیٹا_Type& مجموع_DType = Dfg_ڈیٹا_Type::packed(rhsp->چوڑائی() + 1);
                    Dfg_راس* ثابت bp = make<DfgExtend>(flp, مجموع_DType, l_سرکاؤ_Lp->rhsp());
                    Dfg_راس* ثابت cp = make<DfgExtend>(flp, مجموع_DType, rhsp);
                    Dfg_جوڑو* ثابت addp = make<Dfg_جوڑو>(flp, مجموع_DType, bp, cp);
                    replace(make<Dfg_سرکاؤ_L>(vtxp, l_سرکاؤ_Lp->lhsp(), addp));
                    واپس;
                }
            }
        }

        اگر (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرکاؤ_Amount = rConstp->مقصد_U32();

            اگر (سرکاؤ_Amount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    واپس;
                }
            }

            اگر (سرکاؤ_Amount >= vtxp->چوڑائی()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->چوڑائی()));
                    واپس;
                }
            }

            اگر (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                اگر (!lConcatp->has_مضرب_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        Dfg_راس* ثابت lRhsp = lConcatp->rhsp();
                        Dfg_راس* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t rW = سرکاؤ_Amount;
                        ثابت uint32_t mW = std::min(lRhsp->چوڑائی(), vtxp->چوڑائی() - سرکاؤ_Amount);
                        ثابت uint32_t lW = vtxp->چوڑائی() - mW - rW;
                        // Construct the result
                        File_سطر* ثابت flp = lConcatp->fileline();
                        Dfg_راس* ثابت rp = makeZero(flp, سرکاؤ_Amount);
                        Dfg_راس* ثابت mp
                            = make<DfgSel>(flp, Dfg_ڈیٹا_Type::packed(mW), lRhsp, 0U);
                        Dfg_راس* np = make<DfgConcat>(flp, Dfg_ڈیٹا_Type::packed(mW + rW), mp, rp);
                        اگر (!lW) {
                            replace(np);
                            واپس;
                        }
                        Dfg_راس* ثابت lp
                            = make<DfgSel>(flp, Dfg_ڈیٹا_Type::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        واپس;
                    }
                }
            }

            اگر (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                اگر (!lSelp->has_مضرب_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ثابت uint32_t n_Sel_چوڑائی = lSelp->چوڑائی() - سرکاؤ_Amount;
                        Dfg_راس* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_ڈیٹا_Type::packed(n_Sel_چوڑائی),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), سرکاؤ_Amount)));
                        واپس;
                    }
                }
            }

            اگر (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                اگر (!lCondp->has_مضرب_Sinks()) {
                    APPLYING(ڈالو_SHIFTL_THROUGH_COND) {
                        Dfg_سرکاؤ_L* ثابت tp = make<Dfg_سرکاؤ_L>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_سرکاؤ_L* ثابت ep = make<Dfg_سرکاؤ_L>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        واپس;
                    }
                }
            }
        }
    }

    خالی دورہ(Dfg_سرکاؤ_R* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
        اگر (optimize_سرکاؤ_RHS(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();

        اگر (Dfg_سرکاؤ_R* ثابت l_سرکاؤ_Rp = lhsp->cast<Dfg_سرکاؤ_R>()) {
            اگر (!l_سرکاؤ_Rp->has_مضرب_Sinks() && rhsp->dtype() == l_سرکاؤ_Rp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_سطر* ثابت flp = vtxp->fileline();
                    ثابت Dfg_ڈیٹا_Type& مجموع_DType = Dfg_ڈیٹا_Type::packed(rhsp->چوڑائی() + 1);
                    Dfg_راس* ثابت bp = make<DfgExtend>(flp, مجموع_DType, l_سرکاؤ_Rp->rhsp());
                    Dfg_راس* ثابت cp = make<DfgExtend>(flp, مجموع_DType, rhsp);
                    Dfg_جوڑو* ثابت addp = make<Dfg_جوڑو>(flp, مجموع_DType, bp, cp);
                    replace(make<Dfg_سرکاؤ_R>(vtxp, l_سرکاؤ_Rp->lhsp(), addp));
                    واپس;
                }
            }
        }

        اگر (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t سرکاؤ_Amount = rConstp->مقصد_U32();

            اگر (سرکاؤ_Amount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    واپس;
                }
            }

            اگر (سرکاؤ_Amount >= vtxp->چوڑائی()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->چوڑائی()));
                    واپس;
                }
            }

            اگر (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                اگر (!lConcatp->has_مضرب_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        Dfg_راس* ثابت lRhsp = lConcatp->rhsp();
                        Dfg_راس* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t lW = سرکاؤ_Amount;
                        ثابت uint32_t mW = std::min(lLhsp->چوڑائی(), vtxp->چوڑائی() - سرکاؤ_Amount);
                        ثابت uint32_t rW = vtxp->چوڑائی() - mW - lW;
                        // Construct the result
                        File_سطر* ثابت flp = lConcatp->fileline();
                        Dfg_راس* ثابت lp = makeZero(flp, سرکاؤ_Amount);
                        Dfg_راس* ثابت mp = make<DfgSel>(flp, Dfg_ڈیٹا_Type::packed(mW), lLhsp,
                                                           lLhsp->چوڑائی() - mW);
                        Dfg_راس* np = make<DfgConcat>(flp, Dfg_ڈیٹا_Type::packed(lW + mW), lp, mp);
                        اگر (!rW) {
                            replace(np);
                            واپس;
                        }
                        Dfg_راس* ثابت rp = make<DfgSel>(flp, Dfg_ڈیٹا_Type::packed(rW), lRhsp,
                                                           lRhsp->چوڑائی() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        واپس;
                    }
                }
            }

            اگر (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                اگر (!lSelp->has_مضرب_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ثابت uint32_t n_Sel_چوڑائی = lSelp->چوڑائی() - سرکاؤ_Amount;
                        Dfg_راس* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_ڈیٹا_Type::packed(n_Sel_چوڑائی),
                                           lSelp->fromp(), lSelp->lsb() + سرکاؤ_Amount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), سرکاؤ_Amount), nSelp));
                        واپس;
                    }
                }
            }

            اگر (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                اگر (!lCondp->has_مضرب_Sinks()) {
                    APPLYING(ڈالو_SHIFTR_THROUGH_COND) {
                        Dfg_سرکاؤ_R* ثابت tp = make<Dfg_سرکاؤ_R>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_سرکاؤ_R* ثابت ep = make<Dfg_سرکاؤ_R>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        واپس;
                    }
                }
            }
        }
    }

    خالی دورہ(Dfg_سرکاؤ_RS* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;
        اگر (optimize_سرکاؤ_RHS(vtxp)) واپس;
    }

    خالی دورہ(DfgSub* ثابت vtxp) override {
        اگر (ثنائی(vtxp)) واپس;

        Dfg_راس* ثابت lhsp = vtxp->lhsp();
        Dfg_راس* ثابت rhsp = vtxp->rhsp();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                واپس;
            }
        }

        اگر (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            اگر (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    واپس;
                }
            }
            اگر (vtxp->dtype() == m_ثنائی_ہندسہ_DType && rConstp->has_قدر(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_ثنائی_ہندسہ_DType, lhsp));
                    واپس;
                }
            }
        }
    }

    خالی دورہ(Dfg_مطابقت_Masked* ثابت vtxp) override {
        اگر (DfgConst* ثابت constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ثابت مطابقت_Varp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), Ast_مطابقت_Masked::fold(constp->عدد(), مطابقت_Varp)));
                واپس;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    خالی دورہ(DfgCond* ثابت vtxp) override {
        Dfg_راس* ثابت condp = vtxp->condp();
        Dfg_راس* ثابت thenp = vtxp->thenp();
        Dfg_راس* ثابت elsep = vtxp->elsep();
        File_سطر* ثابت flp = vtxp->fileline();

        اگر (condp->dtype() != m_ثنائی_ہندسہ_DType) واپس;

        اگر (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                واپس;
            }
        }

        اگر (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                واپس;
            }
        }

        اگر (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                واپس;
            }
        }

        اگر (DfgNot* ثابت condNotp = condp->cast<DfgNot>()) {
            اگر (!condp->has_مضرب_Sinks() || condNotp->has_مضرب_Sinks()) {
                APPLYING(تبادلہ_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    واپس;
                }
            }
        }

        اگر (DfgNeq* ثابت condNeqp = condp->cast<DfgNeq>()) {
            اگر (!condp->has_مضرب_Sinks()) {
                APPLYING(تبادلہ_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ثابت newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    واپس;
                }
            }
        }

        اگر (DfgNot* ثابت thenNotp = thenp->cast<DfgNot>()) {
            اگر (DfgNot* ثابت elseNotp = elsep->cast<DfgNot>()) {
                اگر (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->has_مضرب_Sinks() && !elseNotp->has_مضرب_Sinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ثابت newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        واپس;
                    }
                }
            }
        }

        اگر (DfgOr* ثابت condOrp = condp->cast<DfgOr>()) {
            اگر (DfgCond* ثابت thenCondp = thenp->cast<DfgCond>()) {
                اگر (!thenCondp->has_مضرب_Sinks()) {
                    اگر (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ثابت ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            واپس;
                        }
                    }
                    اگر (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ثابت ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            واپس;
                        }
                    }
                }
            }
        }

        اگر (vtxp->چوڑائی() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            اگر (Dfg_جوڑو* ثابت thenAddp = thenp->cast<Dfg_جوڑو>()) {
                اگر (DfgConst* ثابت constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    اگر (constp->has_قدر(1)) {
                        اگر (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->چوڑائی() - 1), condp);
                                File_سطر* ثابت thenFlp = thenAddp->fileline();
                                replace(
                                    make<Dfg_جوڑو>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                واپس;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            اگر (DfgSub* ثابت thenSubp = thenp->cast<DfgSub>()) {
                اگر (DfgConst* ثابت constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    اگر (constp->has_قدر(1)) {
                        اگر (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->چوڑائی() - 1), condp);
                                File_سطر* ثابت thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                واپس;
                            }
                        }
                    }
                }
            }
        }

        اگر (vtxp->dtype() == m_ثنائی_ہندسہ_DType) {
            اگر (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    واپس;
                }
            }
            اگر (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    واپس;
                }
            }
        }

        اگر (vtxp->چوڑائی() <= VL_QUADSIZE) {
            اگر (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    Dfg_راس* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    واپس;
                }
            }
            اگر (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    Dfg_راس* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    واپس;
                }
            }
            اگر (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    Dfg_راس* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    واپس;
                }
            }
            اگر (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    Dfg_راس* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    واپس;
                }
            }

            اگر (DfgOr* ثابت tOrp = thenp->cast<DfgOr>()) {
                اگر (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        Dfg_راس* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        واپس;
                    }
                }
                اگر (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        Dfg_راس* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        واپس;
                    }
                }
            }
        }

        اگر (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ثابت std::pair<Dfg_راس*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            اگر (cLSBs.پہلا) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ثابت uint32_t r_چوڑائی = cLSBs.دوسرا;
                    Dfg_راس* rhsp = cLSBs.پہلا;
                    اگر (r_چوڑائی != rhsp->چوڑائی()) {
                        ثابت Dfg_ڈیٹا_Type& rDtype = Dfg_ڈیٹا_Type::packed(r_چوڑائی);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    اگر (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        واپس;
                    }
                    // Create new LHS
                    ثابت uint32_t l_چوڑائی = vtxp->چوڑائی() - r_چوڑائی;
                    ثابت Dfg_ڈیٹا_Type& lDtype = Dfg_ڈیٹا_Type::packed(l_چوڑائی);
                    Dfg_راس* ثابت lThenp = make<DfgSel>(flp, lDtype, thenp, r_چوڑائی);
                    Dfg_راس* ثابت lElsep = make<DfgSel>(flp, lDtype, elsep, r_چوڑائی);
                    Dfg_راس* ثابت lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    واپس;
                }
            }

            ثابت std::pair<Dfg_راس*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            اگر (cMSBs.پہلا) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ثابت uint32_t l_چوڑائی = cMSBs.دوسرا;
                    Dfg_راس* lhsp = cMSBs.پہلا;
                    اگر (l_چوڑائی != lhsp->چوڑائی()) {
                        ثابت Dfg_ڈیٹا_Type& lDtype = Dfg_ڈیٹا_Type::packed(l_چوڑائی);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->چوڑائی() - l_چوڑائی);
                    }
                    // If it's all the same, just replace
                    اگر (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        واپس;
                    }
                    // Create new RHS
                    ثابت uint32_t r_چوڑائی = vtxp->چوڑائی() - l_چوڑائی;
                    ثابت Dfg_ڈیٹا_Type& rDtype = Dfg_ڈیٹا_Type::packed(r_چوڑائی);
                    Dfg_راس* ثابت rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    Dfg_راس* ثابت rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    Dfg_راس* ثابت rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    واپس;
                }
            }
        }

        اگر (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                Dfg_راس* resp = condp;
                اگر (ثابت uint32_t extend = vtxp->چوڑائی() - 1) {
                    DfgConst* ثابت zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                واپس;
            }
        }

        اگر (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                Dfg_راس* resp = make<DfgNot>(condp, condp);
                اگر (ثابت uint32_t extend = vtxp->چوڑائی() - 1) {
                    DfgConst* ثابت zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                واپس;
            }
        }

        اگر (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                واپس;
            }
        }

        اگر (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                واپس;
            }
        }

        اگر (DfgCond* ثابت tCondp = thenp->cast<DfgCond>()) {
            اگر (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    واپس;
                }
            }
        }

        اگر (DfgCond* ثابت eCondp = elsep->cast<DfgCond>()) {
            اگر (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    واپس;
                }
            }
        }
    }

    خالی دورہ(Dfg_راس_Var* ثابت vtxp) override {
        اگر (vtxp->hasSinks()) واپس;
        اگر (vtxp->isObserved()) واپس;
        اگر (vtxp->defaultp()) واپس;

        // If undriven, or driven from another var, it is completely redundant.
        اگر (!vtxp->srcp() || vtxp->srcp()->is<Dfg_راس_Var>()) {
            APPLYING(REMOVE_VAR) {
                delete_راس(vtxp);
                واپس;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        منطقی ملا_One = جھوٹ;
        ثابت منطقی keep = vtxp->srcp()->foreachSink([&](Dfg_راس& sink) -> منطقی {
            // Ignore non-observable variable sinks. These can be eliminated.
            اگر (ثابت Dfg_راس_Var* ثابت varp = sink.cast<Dfg_راس_Var>()) {
                اگر (!varp->hasSinks() && !varp->isObserved()) واپس جھوٹ;
            }
            // Keep if found more than one sink
            اگر (ملا_One) واپس سچ;
            // Mark first sink found
            ملا_One = سچ;
            واپس جھوٹ;
        });
        اگر (!keep) {
            APPLYING(REMOVE_VAR) {
                delete_راس(vtxp);
                واپس;
            }
        }
    }

    V3DfgPeephole(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.for_Each_راس([&](Dfg_راس& vtx) { m_vInfo[vtx].m_id = ++m_آخری_Id; });

        // Add all operation vertices to the cache
        برائے (Dfg_راس& vtx : m_dfg.opVertices()) cache_راس(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.سائز() * 2);
        m_iterList.reserve(m_dfg.سائز() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ڈالو_عقب(خالی_اشاریہ);
        m_iterList.ڈالو_عقب(خالی_اشاریہ);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        برائے (Dfg_راس_Var& vtx : m_dfg.varVertices()) جوڑو_مقصد_Work_List(&vtx);

        // Add all operation vertices to the work list
        برائے (Dfg_راس& vtx : m_dfg.opVertices()) جوڑو_مقصد_Work_List(&vtx);

        // Process iteratively
        جب (سچ) {
            // Process the work list - keep the placeholder at index 0
            جب (m_workList.سائز() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.عقب();
                m_workList.نکالو_عقب();
                اگر (!m_vtxp) جاری;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_work_List_اشاریہ = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                اگر (Dfg_راس_Var* ثابت varp = m_vtxp->cast<Dfg_راس_Var>()) {
                    دورہ(varp);
                    جاری;
                }

                // Ast references are not considered
                اگر (m_vtxp->is<Dfg_راس_Ast>()) جاری;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                اگر (!m_vInfo[m_vtxp].m_is_Cached_راس) {
                    اگر (Dfg_راس* ثابت sampep = cache_راس(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            جاری;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            اگر (m_iterList.سائز() == 1) توڑو;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                سائز_t begin = 0;
                سائز_t انتہا = m_iterList.سائز();
                برائے (سائز_t hops = 1; hops <= 4; ++hops) {
                    برائے (سائز_t i = begin; i < انتہا; ++i) {
                        Dfg_راس* ثابت vtxp = m_iterList[i];
                        اگر (!vtxp) جاری;
                        vtxp->foreachSink([&](Dfg_راس& dst) {
                            جوڑو_مقصد_Iter_List(&dst);
                            واپس جھوٹ;
                        });
                        vtxp->foreach_ماخذ([&](Dfg_راس& src) {
                            جوڑو_مقصد_Iter_List(&src);
                            واپس جھوٹ;
                        });
                    }
                    begin = انتہا;
                    انتہا = m_iterList.سائز();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            برائے (Dfg_راس* ثابت vtxp : m_iterList) {
                اگر (!vtxp) جاری;
                removeFromIterList(vtxp);
                جوڑو_مقصد_Work_List(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#تعریف_منسوخ APPLYING

عوامی:
    ساکن خالی apply(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

خالی V3DfgPasses::peephole(Dfg_گراف& dfg, V3DfgPeepholeContext& ctx) {
    اگر (!v3Global.opt.fDfgPeephole()) واپس;
    V3DfgPeephole::apply(dfg, ctx);
}
