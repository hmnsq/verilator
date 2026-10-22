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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgCache.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgDataType.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPeepholePatterns.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cctype>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
ନାମାକାଶ {

template <typename T_Reduction>
ସଂରଚନା ReductionToBitwiseImpl {};
template <> ସଂରଚନା ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> ସଂରଚନା ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> ସଂରଚନା ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
ସଂରଚନା BitwiseToReductionImpl {};
template <> ସଂରଚନା BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> ସଂରଚନା BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> ସଂରଚନା BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
ସଂରଚନା IsAssociative : ସର୍ବସାଧାରଣ std::false_type {};
template <> ସଂରଚନା IsAssociative<DfgAnd> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsAssociative<DfgOr>  : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsAssociative<DfgXor> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsAssociative<DfgAdd> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsAssociative<DfgConcat> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsAssociative<DfgMul> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsAssociative<DfgMulS> : ସର୍ବସାଧାରଣ std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
ସଂରଚନା IsCommutative : ସର୍ବସାଧାରଣ std::false_type {};
template <> ସଂରଚନା IsCommutative<DfgAnd> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsCommutative<DfgOr>  : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsCommutative<DfgXor> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsCommutative<DfgAdd> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsCommutative<DfgEq> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsCommutative<DfgMul> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsCommutative<DfgMulS> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsCommutative<DfgNeq> : ସର୍ବସାଧାରଣ std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
ସଂରଚନା IsIdempotent : ସର୍ବସାଧାରଣ std::false_type {};
template <> ସଂରଚନା IsIdempotent<DfgAnd> : ସର୍ବସାଧାରଣ std::true_type {};
template <> ସଂରଚନା IsIdempotent<DfgOr>  : ସର୍ବସାଧାରଣ std::true_type {};

// Binary result dtype
template<typename Vertex>
ଅଚଳ DfgDataType& resultDType(ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp);
template <> ଅଚଳ DfgDataType& resultDType<DfgAdd>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgAnd>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgConcat>     (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> ଅଚଳ DfgDataType& resultDType<DfgDiv>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgDivS>       (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgEq>         (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgGt>         (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgGtS>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgGte>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgGteS>       (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgLogAnd>     (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgLogEq>      (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgLogIf>      (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgLogOr>      (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgLt>         (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgLtS>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgLte>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgLteS>       (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgModDiv>     (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgModDivS>    (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgMul>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgMulS>       (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgNeq>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ DfgDataType::packed(1); }
template <> ଅଚଳ DfgDataType& resultDType<DfgOr>         (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgPow>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgPowSS>      (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgPowSU>      (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgPowUS>      (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgShiftL>     (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgShiftR>     (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgShiftRS>    (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgSub>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }
template <> ଅଚଳ DfgDataType& resultDType<DfgXor>        (ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) { ଫେରନ୍ତୁ lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> ଶୂନ୍ୟ foldOp(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src);
template <> ଶୂନ୍ୟ foldOp<Dfg_ଗଣନା_Ones>  (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.op_ଗଣନା_Ones(src); }
template <> ଶୂନ୍ୟ foldOp<DfgExtend>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opAssign(src); }
template <> ଶୂନ୍ୟ foldOp<DfgExtendS>    (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opExtendS(src, src.width()); }
template <> ଶୂନ୍ୟ foldOp<DfgLogNot>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opLogNot(src); }
template <> ଶୂନ୍ୟ foldOp<DfgNegate>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opNegate(src); }
template <> ଶୂନ୍ୟ foldOp<DfgNot>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opNot(src); }
template <> ଶୂନ୍ୟ foldOp<DfgOneHot>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opOneHot(src); }
template <> ଶୂନ୍ୟ foldOp<DfgOneHot0>    (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opOneHot0(src); }
template <> ଶୂନ୍ୟ foldOp<DfgRedAnd>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opRedAnd(src); }
template <> ଶୂନ୍ୟ foldOp<DfgRedOr>      (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opRedOr(src); }
template <> ଶୂନ୍ୟ foldOp<DfgRedXor>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> ଶୂନ୍ୟ foldOp(V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
template <> ଶୂନ୍ୟ foldOp<DfgAdd>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opAdd(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgAnd>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opAnd(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgConcat>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opConcat(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgDiv>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opDiv(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgDivS>       (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opDivS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgEq>         (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opEq(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgGt>         (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opGt(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgGtS>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opGtS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgGte>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opGte(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgGteS>       (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opGteS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgLogAnd>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opLogAnd(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgLogEq>      (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opLogEq(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgLogIf>      (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opLogIf(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgLogOr>      (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opLogOr(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgLt>         (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opLt(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgLtS>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opLtS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgLte>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opLte(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgLteS>       (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opLteS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgModDiv>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opModDiv(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgModDivS>    (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opModDivS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgMul>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opMul(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgMulS>       (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opMulS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgNeq>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opNeq(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgOr>         (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opOr(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgPow>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opPow(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgPowSS>      (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opPowSS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgPowSU>      (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opPowSU(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgPowUS>      (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opPowUS(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgShiftL>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opShiftL(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgShiftR>     (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opShiftR(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgShiftRS>    (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> ଶୂନ୍ୟ foldOp<DfgSub>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opSub(lhs, rhs); }
template <> ଶୂନ୍ୟ foldOp<DfgXor>        (V3_ସଂଖ୍ୟା& out, ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

ଶ୍ରେଣୀ V3DfgPeephole final : ସର୍ବସାଧାରଣ DfgVisitor {
    // TYPES
    ସଂରଚନା VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        ତାର୍କିକ m_isCachedVertex = ମିଥ୍ୟା;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ଅଚଳ DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ଅଚଳ m_tmpScopep = v3Global.rootp()->ଶୀର୍ଷ_Scopep()->scopep();

    // STATIC STATE
    ସ୍ଥିର V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) ଯଦି (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    ତାର୍କିକ checkApplying(VDfgPeepholePattern id) {
        ଯଦି (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_ଗଣନା[id];
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* ଅଚଳ equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        ଫେରନ୍ତୁ equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    ଶୂନ୍ୟ invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = ମିଥ୍ୟା;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_ଧାଡ଼ି* flp, ଅଚଳ DfgDataType& dtype, Operands... operands) {
        Vertex* ଅଚଳ vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = ସତ୍ୟ;
        ଫେରନ୍ତୁ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(ଅଚଳ DfgDataType& dtype, Operands... operands) {
        Vertex* ଅଚଳ vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ଫେରନ୍ତୁ vtxp;
    }

    ଶୂନ୍ୟ incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    ଶୂନ୍ୟ addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        ଯଦି (vInfo.m_workListIndex) ଫେରନ୍ତୁ;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.ଠେଲ_back(vtxp);
    }

    ଶୂନ୍ୟ removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    ଶୂନ୍ୟ addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        ଯଦି (vInfo.m_iterListIndex) ଫେରନ୍ତୁ;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.ଠେଲ_back(vtxp);
    }

    ଶୂନ୍ୟ removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    ଶୂନ୍ୟ deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ଅଚଳ DfgVertexVar* ଅଚଳ varp = vtxp->cast<DfgVertexVar>();
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
            ଯଦି (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            ଯଦି (vInfo.m_generation != m_currentGeneration) srcps.ଠେଲ_back(&src);
            vInfo.m_generation = m_currentGeneration;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        ଯଦି (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ଠେଲ_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } ଅନ୍ୟଥା {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ଅଚଳ auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  ଫେରନ୍ତୁ srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        ପାଇଁ (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        ପାଇଁ (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    ଶୂନ୍ୟ replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ଅଚଳ auto debugCallback = [&]() -> ଶୂନ୍ୟ {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            ଅଚଳ auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ଅଚଳ DfgVertex& v) {  //
                ଫେରନ୍ତୁ cone->ଗଣନା_2(&v);
            });
        };
        ଯଦି (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ଫେରନ୍ତୁ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_ଧାଡ଼ି* flp, uint32_t val) { ଫେରନ୍ତୁ ନୂତନ DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_ଧାଡ଼ି* flp, uint32_t width) {
        ଫେରନ୍ତୁ ନୂତନ DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_ଧାଡ଼ି* flp, ଅଚଳ DfgDataType& dtype) {
        ଫେରନ୍ତୁ makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_ଧାଡ଼ି* flp, uint32_t width) {
        DfgConst* ଅଚଳ resp = makeZero(flp, width);
        resp->ଅଙ୍କ().setAllBits1();
        ଫେରନ୍ତୁ resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_ଧାଡ଼ି* flp, ଅଚଳ DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* ଅଚଳ vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        ଯଦି (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        ଯଦି (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        ଫେରନ୍ତୁ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(ଅଚଳ DfgVertex* examplep, Operands... operands) {
        ଫେରନ୍ତୁ make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        ଯଦି (vtxp->dtype() == m_bitDType) ଫେରନ୍ତୁ bitp;
        ଫେରନ୍ତୁ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ସ୍ଥିର ତାର୍କିକ isSame(ଅଚଳ DfgVertex* ap, ଅଚଳ DfgVertex* bp) {
        ଯଦି (ap == bp) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (ap->dtype() != bp->dtype()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ DfgConst* ଅଚଳ aConstp = ap->cast<DfgConst>();
        ଯଦି (!aConstp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ DfgConst* ଅଚଳ bConstp = bp->cast<DfgConst>();
        ଯଦି (!bConstp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ aConstp->ଅଙ୍କ().isCaseEq(bConstp->ଅଙ୍କ());
    }

    ସ୍ଥିର ତାର୍କିକ isZero(ଅଚଳ DfgVertex* vtxp) {
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ constp = vtxp->cast<DfgConst>()) ଫେରନ୍ତୁ constp->isZero();
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ isOnes(ଅଚଳ DfgVertex* vtxp) {
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ constp = vtxp->cast<DfgConst>()) ଫେରନ୍ତୁ constp->isOnes();
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ isEqOne(ଅଚଳ DfgVertex* vtxp) {
        ଯଦି (ଅଚଳ DfgConst* ଅଚଳ constp = vtxp->cast<DfgConst>()) ଫେରନ୍ତୁ constp->ଅଙ୍କ().isEqOne();
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ସ୍ଥିର ତାର୍କିକ areAdjacent(uint32_t& lsb, ଅଚଳ DfgSel* lSelp, ଅଚଳ DfgSel* rSelp) {
        ଯଦି (!isSame(lSelp->srcp(), rSelp->srcp())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ foldUnary(Vertex* ଅଚଳ vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::ମୂଲ୍ୟ, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::ମୂଲ୍ୟ, "Must invoke on final class");
        ଯଦି (DfgConst* ଅଚଳ srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ଅଚଳ resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->ଅଙ୍କ(), srcp->ଅଙ୍କ());
                replace(resultp);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ ଦ୍ୱିମିକ(Vertex* ଅଚଳ vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ଦ୍ୱିମିକ, Vertex>::ମୂଲ୍ୟ, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::ମୂଲ୍ୟ, "Must invoke on final class");

        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ଅଚଳ lConstp = lhsp->cast<DfgConst>();
        DfgConst* ଅଚଳ rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* ଅଚଳ lSamep = lhsp->cast<Vertex>();
        Vertex* ଅଚଳ rSamep = rhsp->cast<Vertex>();

        // Constant folding
        ଯଦି (lConstp && rConstp) {
            APPLYING(FOLD_ଦ୍ୱିମିକ) {
                DfgConst* ଅଚଳ resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->ଅଙ୍କ(), lConstp->ଅଙ୍କ(), rConstp->ଅଙ୍କ());
                replace(resultp);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }

        // Idempotent
        ଯଦି VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::ମୂଲ୍ୟ) {
            ଯଦି (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ଦ୍ୱିମିକ) {
                    replace(lhsp);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
        }

        // Associative
        ଯଦି VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::ମୂଲ୍ୟ) {
            // Also commutative and idempotent
            ଯଦି VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::ମୂଲ୍ୟ && IsIdempotent<Vertex>::ମୂଲ୍ୟ) {
                ଯଦି (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ଦ୍ୱିମିକ_LHS) {
                        replace(lSamep);
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
                ଯଦି (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ଦ୍ୱିମିକ_RHS) {
                        replace(rSamep);
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
            }

            ଯଦି (lConstp && rSamep) {
                ଯଦି (DfgConst* ଅଚଳ rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ଦ୍ୱିମିକ_LHS_OF_RHS) {
                        DfgConst* ଅଚଳ cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->ଅଙ୍କ(), lConstp->ଅଙ୍କ(), rlConstp->ଅଙ୍କ());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
            }

            ଯଦି (lSamep && rConstp) {
                ଯଦି (DfgConst* ଅଚଳ lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ଦ୍ୱିମିକ_RHS_OF_LHS) {
                        DfgConst* ଅଚଳ cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->ଅଙ୍କ(), lrConstp->ଅଙ୍କ(), rConstp->ଅଙ୍କ());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            ଯଦି (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* ଅଚଳ ap = lSamep->lhsp();
                DfgVertex* ଅଚଳ bp = lSamep->rhsp();
                DfgVertex* ଅଚଳ cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                ଯଦି (ap != cp && bp != cp) {
                    APPLYING(ଡାହାଣ_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ଅଚଳ DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* ଅଚଳ bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
            }

            ଯଦି (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* ଅଚଳ rlVtxp = rSamep->lhsp();
                DfgVertex* ଅଚଳ rrVtxp = rSamep->rhsp();

                ଯଦି VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::ମୂଲ୍ୟ) {
                    ଯଦି (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            ଫେରନ୍ତୁ ସତ୍ୟ;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                ଯଦି (Vertex* ଅଚଳ existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    ଯଦି (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ଦ୍ୱିମିକ_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            ଫେରନ୍ତୁ ସତ୍ୟ;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                ଯଦି VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::ମୂଲ୍ୟ) {
                    ଯଦି (Vertex* ଅଚଳ existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        ଯଦି (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ଦ୍ୱିମିକ_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                ଫେରନ୍ତୁ ସତ୍ୟ;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        ଯଦି VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::ମୂଲ୍ୟ) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            ଯଦି (lConstp) {
                ଯଦି (DfgCond* ଅଚଳ rCondp = rhsp->cast<DfgCond>()) {
                    ଯଦି (!rCondp->hasMultipleSinks()) {
                        APPLYING(ଠେଲ_COMMUTATIVE_ଦ୍ୱିମିକ_THROUGH_COND) {
                            DfgVertex* ଅଚଳ tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* ଅଚଳ ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ଫେରନ୍ତୁ ସତ୍ୟ;
                        }
                    }
                }
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ଅଚଳ ତାର୍କିକ swapSides = [&]() {
            // Don't swap if not commutative, duh
            ଯଦି VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::ମୂଲ୍ୟ) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Const go on left
            ଯଦି (lConstp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (rConstp) ଫେରନ୍ତୁ ସତ୍ୟ;
            // Not go on left
            ଯଦି (lhsp->is<DfgNot>()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (rhsp->is<DfgNot>()) ଫେରନ୍ତୁ ସତ୍ୟ;
            // Same go on right
            ଯଦି (rSamep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (lSamep) ଫେରନ୍ତୁ ସତ୍ୟ;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ଅଚଳ VertexInfo& lInfo = m_vInfo[lhsp];
            ଅଚଳ VertexInfo& rInfo = m_vInfo[rhsp];
            ଫେରନ୍ତୁ lInfo.m_id > rInfo.m_id;
        }();
        ଯଦି (swapSides) {
            APPLYING(SWAP_SIDES_IN_ଦ୍ୱିମିକ) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ distributive_And_Associative_ଦ୍ୱିମିକ(Vertex* vtxp) {
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        ଯଦି (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            ଯଦି (Other* ଅଚଳ lp = lhsp->cast<Other>()) {
                ଯଦି (Other* ଅଚଳ rp = rhsp->cast<Other>()) {
                    DfgVertex* ଅଚଳ llp = lp->lhsp();
                    DfgVertex* ଅଚଳ lrp = lp->rhsp();
                    DfgVertex* ଅଚଳ rlp = rp->lhsp();
                    DfgVertex* ଅଚଳ rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    ଯଦି (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } ଅନ୍ୟଥା ଯଦି (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } ଅନ୍ୟଥା ଯଦି (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } ଅନ୍ୟଥା ଯଦି (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    ଯଦି (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_ଦ୍ୱିମିକ) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            ଫେରନ୍ତୁ ସତ୍ୟ;
                        }
                    }
                }
            }
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ
    try_ଠେଲ_Bitwise_Op_Through_Concat(Vertex* ଅଚଳ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ଅଚଳ catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ଅଚଳ catRConstp = concatp->rhsp()->cast<DfgConst>();
        ଯଦି (catLConstp || catRConstp) {
            APPLYING(ଠେଲ_BITWISE_OP_THROUGH_CONCAT) {
                ଅଚଳ uint32_t width = concatp->width();
                ଅଚଳ DfgDataType& lDtype = concatp->lhsp()->dtype();
                ଅଚଳ DfgDataType& rDtype = concatp->rhsp()->dtype();
                ଅଚଳ uint32_t lWidth = lDtype.size();
                ଅଚଳ uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* ଅଚଳ newLhsp = [&]() -> DfgVertex* {
                    DfgConst* ଅଚଳ newLhsConstp = makeZero(constp->fileline(), lWidth);
                    ଯଦି (catLConstp) {
                        V3_ସଂଖ୍ୟା ଅଙ୍କ{constp->fileline(), static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(lWidth), 0u};
                        ଅଙ୍କ.opSel(constp->ଅଙ୍କ(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->ଅଙ୍କ(), ଅଙ୍କ, catLConstp->ଅଙ୍କ());
                        ଫେରନ୍ତୁ newLhsConstp;
                    }
                    newLhsConstp->ଅଙ୍କ().opSel(constp->ଅଙ୍କ(), width - 1, rWidth);
                    ଫେରନ୍ତୁ make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* ଅଚଳ newRhsp = [&]() -> DfgVertex* {
                    DfgConst* ଅଚଳ newRhsConstp = makeZero(constp->fileline(), rWidth);
                    ଯଦି (catRConstp) {
                        V3_ସଂଖ୍ୟା ଅଙ୍କ{constp->fileline(), static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(rWidth), 0u};
                        ଅଙ୍କ.opSel(constp->ଅଙ୍କ(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->ଅଙ୍କ(), ଅଙ୍କ, catRConstp->ଅଙ୍କ());
                        ଫେରନ୍ତୁ newRhsConstp;
                    }
                    newRhsConstp->ଅଙ୍କ().opSel(constp->ଅଙ୍କ(), rWidth - 1, 0);
                    ଫେରନ୍ତୁ make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ
    try_ଠେଲ_Compare_Op_Through_Concat(Vertex* ଅଚଳ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        ଯଦି (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ଠେଲ_COMPARE_OP_THROUGH_CONCAT) {
                ଅଚଳ uint32_t width = concatp->width();
                ଅଚଳ uint32_t lWidth = concatp->lhsp()->width();
                ଅଚଳ uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* ଅଚଳ newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->ଅଙ୍କ().opSel(constp->ଅଙ୍କ(), width - 1, rWidth);
                Vertex* ଅଚଳ newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ଅଚଳ newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->ଅଙ୍କ().opSel(constp->ଅଙ୍କ(), rWidth - 1, 0);
                Vertex* ଅଚଳ newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                ଯଦି VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::ମୂଲ୍ୟ) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ଅନ୍ୟଥା ଯଦି VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::ମୂଲ୍ୟ) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ଅନ୍ୟଥା {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ try_ଠେଲ_Bitwise_Op_Through_Reductions(Bitwise* ଅଚଳ vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        ଯଦି (Reduction* ଅଚଳ lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            ଯଦି (Reduction* ଅଚଳ rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* ଅଚଳ lSrcp = lRedp->srcp();
                DfgVertex* ଅଚଳ rSrcp = rRedp->srcp();
                ଯଦି (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(ଠେଲ_BITWISE_THROUGH_REDUCTION) {
                        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
                        Bitwise* ଅଚଳ bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
            }
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    ସ୍ଥିର std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        ଯଦି (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ଫେରନ୍ତୁ {nullptr, nullptr};
        }
        ଯଦି (vtxp->hasMultipleSinks()) ଫେରନ୍ତୁ {nullptr, nullptr};
        Dfg_Vertex_ଦ୍ୱିମିକ* ଅଚଳ bwp = vtxp->as<Dfg_Vertex_ଦ୍ୱିମିକ>();
        DfgSel* ଅଚଳ lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ଅଚଳ rSelp = bwp->rhsp()->cast<DfgSel>();
        ଯଦି (!lSelp || !rSelp) ଫେରନ୍ତୁ {nullptr, nullptr};
        DfgSel* ଅଚଳ nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* ଅଚଳ wSelp = nSelp == lSelp ? rSelp : lSelp;
        ଅଚଳ uint32_t nWidth = nSelp->fromp()->width();
        ଯଦି (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ଫେରନ୍ତୁ {nullptr, nullptr};
        ଯଦି (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            ଫେରନ୍ତୁ {nullptr, nullptr};
        }
        ଫେରନ୍ତୁ {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ try_ଠେଲ_Bitwise_Op_Throug_Sel(Bitwise* ଅଚଳ vtxp) {
        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();

        ଯଦି (DfgSel* ଅଚଳ lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ଯଦି (DfgSel* ଅଚଳ selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } ଅନ୍ୟଥା ଯଦି (Bitwise* ଅଚଳ bitwisep = rhsp->cast<Bitwise>()) {
                ଯଦି (DfgSel* ଅଚଳ rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } ଅନ୍ୟଥା ଯଦି (DfgSel* ଅଚଳ rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            ଯଦି (rSelp) {
                DfgVertex* ଅଚଳ lFromp = lSelp->fromp();
                DfgVertex* ଅଚଳ rFromp = rSelp->fromp();
                ଯଦି (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ଠେଲ_BITWISE_THROUGH_SEL) {
                        Bitwise* ଅଚଳ bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        ଯଦି (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ଫେରନ୍ତୁ ସତ୍ୟ;
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
            Bitwise* ଅଚଳ rChainp = rhsp->cast<Bitwise>();
            ଯଦି (rChainp && rChainp->hasMultipleSinks()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            DfgVertex* ଅଚଳ rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ଅଚଳ auto lSels = pushableUnalignedSels(lhsp);
            ଅଚଳ auto rSels = pushableUnalignedSels(rTermp);
            ଯଦି (!lSels.first && !rSels.first) ଫେରନ୍ତୁ ମିଥ୍ୟା;

            ଅଚଳ DfgVertex* ଅଚଳ opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            ଯଦି (opp->is<Bitwise>()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଅଚଳ auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* ଅଚଳ nFromp = sels.first->fromp();
            DfgVertex* ଅଚଳ wFromp = sels.second->fromp();
            ଅଚଳ uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            ଅଚଳ auto matches = [&](ଅଚଳ DfgVertex* termp, ଅଚଳ auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                ଯଦି (tSels.first) {
                    ଫେରନ୍ତୁ termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                ଯଦି (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                ଯଦି (ଅଚଳ DfgSel* ଅଚଳ selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                ଯଦି (termp->type() != opp->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                // Operands might have been swapped since
                ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* ଅଚଳ bwp = termp->as<Dfg_Vertex_ଦ୍ୱିମିକ>();
                ଅଚଳ ତାର୍କିକ nIsLhs = bwp->lhsp() == nFromp;
                ଯଦି (!nIsLhs && bwp->rhsp() != nFromp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                ଅଚଳ DfgSel* ଅଚଳ slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ଫେରନ୍ତୁ slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            ଯଦି (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

            APPLYING(ଠେଲ_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
                ଅଚଳ DfgDataType& dtype = nFromp->dtype();
                DfgSel* ଅଚଳ slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                ଯଦି (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } ଅନ୍ୟଥା ଯଦି (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } ଅନ୍ୟଥା {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ଅଚଳ auto ଠେଲ = [&](DfgVertex* termp, ଅଚଳ DfgSel* nSelp) -> DfgVertex* {
                    ଫେରନ୍ତୁ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* ଅଚଳ newLhsp = ଠେଲ(lhsp, lSels.first);
                DfgVertex* ଅଚଳ newRTermp = ଠେଲ(rTermp, rSels.first);
                ଯଦି (rChainp) {
                    DfgVertex* ଅଚଳ newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } ଅନ୍ୟଥା {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ସ୍ଥିର ତାର୍କିକ isConcatFoldable(ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        ଯଦି (isSame(lhsp, rhsp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (ଅଚଳ DfgRep* ଅଚଳ lRepp = lhsp->cast<DfgRep>()) {
            ଯଦି (isSame(lRepp->srcp(), rhsp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଯଦି (ଅଚଳ DfgRep* ଅଚଳ rRepp = rhsp->cast<DfgRep>()) {
            ଯଦି (isSame(lhsp, rRepp->srcp())) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        // Concatenation of adjoining selects is a select
        ଯଦି (ଅଚଳ DfgSel* ଅଚଳ lSelp = lhsp->cast<DfgSel>()) {
            ଯଦି (ଅଚଳ DfgSel* ଅଚଳ rSelp = rhsp->cast<DfgSel>()) {
                ଫେରନ୍ତୁ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        ଯଦି (ଅଚଳ DfgNot* ଅଚଳ lNotp = lhsp->cast<DfgNot>()) {
            ଯଦି (ଅଚଳ DfgNot* ଅଚଳ rNotp = rhsp->cast<DfgNot>()) {
                ଫେରନ୍ତୁ !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ସ୍ଥିର ତାର୍କିକ isSameBitwise(ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) {
        ଯଦି (lhsp->type() != rhsp->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ସ୍ଥିର ତାର୍କିକ isConcatSimplifiable(ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp) {
        ଯଦି (isConcatFoldable(lhsp, rhsp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (!isSameBitwise(lhsp, rhsp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* ଅଚଳ lp = lhsp->as<Dfg_Vertex_ଦ୍ୱିମିକ>();
        ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* ଅଚଳ rp = rhsp->as<Dfg_Vertex_ଦ୍ୱିମିକ>();
        ଯଦି (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ସ୍ଥିର ତାର୍କିକ isConcatPushableThroughBitwise(ଅଚଳ DfgVertex* lhsp, ଅଚଳ DfgVertex* rhsp,
                                               ତାର୍କିକ& swap) {
        ଯଦି (!isSameBitwise(lhsp, rhsp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* ଅଚଳ lp = lhsp->as<Dfg_Vertex_ଦ୍ୱିମିକ>();
        ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* ଅଚଳ rp = rhsp->as<Dfg_Vertex_ଦ୍ୱିମିକ>();
        swap = ମିଥ୍ୟା;
        ଯଦି (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ଫେରନ୍ତୁ ସତ୍ୟ;
        swap = ସତ୍ୟ;
        ଯଦି (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_ଧାଡ଼ି* flp, ଅଚଳ DfgVertex* lhsp,
                                              ଅଚଳ DfgVertex* rhsp, ତାର୍କିକ swap) {
        ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* ଅଚଳ lp = lhsp->as<Dfg_Vertex_ଦ୍ୱିମିକ>();
        ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* ଅଚଳ rp = rhsp->as<Dfg_Vertex_ଦ୍ୱିମିକ>();
        ଅଚଳ DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* ଅଚଳ cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* ଅଚଳ dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* ଅଚଳ newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ଅଚଳ newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        ଯଦି (lp->is<DfgAnd>()) ଫେରନ୍ତୁ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        ଯଦି (lp->is<DfgOr>()) ଫେରନ୍ତୁ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ଫେରନ୍ତୁ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();

        ଯଦି (DfgSel* ଅଚଳ lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ଯଦି (DfgSel* ଅଚଳ selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } ଅନ୍ୟଥା ଯଦି (Bitwise* ଅଚଳ rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } ଅନ୍ୟଥା ଯଦି (Reduction* ଅଚଳ rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            ଯଦି (rSelp) {
                uint32_t lsb = 0;
                ଯଦି (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        ଅଚଳ DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* ଅଚଳ newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        ଯଦି (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }
            }
        }

        ଯଦି (Reduction* ଅଚଳ lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            ଯଦି (!rRedp) {
                ଯଦି (Bitwise* ଅଚଳ rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            ଯଦି (rRedp) {
                ଯଦି (DfgSel* ଅଚଳ lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    ଯଦି (DfgSel* ଅଚଳ rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ଯଦି (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                ଅଚଳ DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ଅଚଳ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                ଯଦି (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ଫେରନ୍ତୁ ସତ୍ୟ;
                            }
                        }
                    }
                }
            }
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ optimizeReduction(Reduction* ଅଚଳ vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ ସତ୍ୟ;

        DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        // Reduction of 1-bit value
        ଯଦି (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }

        ଯଦି (DfgCond* ଅଚଳ condp = srcp->cast<DfgCond>()) {
            ଯଦି (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ଠେଲ_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ଅଚଳ newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ଅଚଳ newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ଅଚଳ newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
        }

        ଯଦି (DfgConcat* ଅଚଳ concatp = srcp->cast<DfgConcat>()) {
            ଯଦି (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(ଠେଲ_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ଅଚଳ lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* ଅଚଳ rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
        }

        ଯଦି (Bitwise* ଅଚଳ bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            ଯଦି (!bitwisep->hasMultipleSinks()) {
                ଯଦି (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(ଠେଲ_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* ଅଚଳ newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* ଅଚଳ newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        ଫେରନ୍ତୁ ସତ୍ୟ;
                    }
                }

                ଯଦି (DfgSel* ଅଚଳ lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    ଯଦି (DfgSel* ଅଚଳ rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ଯଦି (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ଠେଲ_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                ଅଚଳ DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ଅଚଳ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ଫେରନ୍ତୁ ସତ୍ୟ;
                            }
                        }
                    }
                }
            }
        }

        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT ତାର୍କିକ optimizeShiftRHS(Shift* ଅଚଳ vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ଦ୍ୱିମିକ, Shift>::ମୂଲ୍ୟ, "Must invoke on binary");
        static_assert(std::is_final<Shift>::ମୂଲ୍ୟ, "Must invoke on final class");
        ଯଦି (ଅଚଳ DfgConcat* ଅଚଳ concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            ଯଦି (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_ଗଣନା_Ones_Term(DfgVertex* vtxp) {
        ଯଦି (DfgConcat* ଅଚଳ oCatp = vtxp->cast<DfgConcat>()) {
            ଯଦି (isZero(oCatp->lhsp())) {
                ଯଦି (Dfg_ଗଣନା_Ones* ଅଚଳ ଗଣନା_Onesp = oCatp->rhsp()->cast<Dfg_ଗଣନା_Ones>()) {
                    // Zero extended count ones
                    ଯଦି (DfgSel* ଅଚଳ selp = ଗଣନା_Onesp->srcp()->cast<DfgSel>()) {
                        ଫେରନ୍ତୁ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } ଅନ୍ୟଥା ଯଦି (DfgSel* ଅଚଳ selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    ଯଦି (selp->dtype() == m_bitDType) {  //
                        ଫେରନ୍ତୁ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            ଫେରନ୍ତୁ {nullptr, 0, 0};
        }
        ଯଦି (Dfg_ଗଣନା_Ones* ଅଚଳ ଗଣନା_Onesp = vtxp->cast<Dfg_ଗଣନା_Ones>()) {
            // Simple count ones
            ଯଦି (DfgSel* ଅଚଳ selp = ଗଣନା_Onesp->srcp()->cast<DfgSel>()) {
                ଫେରନ୍ତୁ {selp->fromp(), selp->lsb(), selp->width()};
            }
            ଫେରନ୍ତୁ {nullptr, 0, 0};
        }
        ଯଦି (DfgSel* ଅଚଳ oSelp = vtxp->cast<DfgSel>()) {
            ଯଦି (oSelp->lsb() == 0) {
                // Truncated count ones
                ଯଦି (Dfg_ଗଣନା_Ones* ଅଚଳ ଗଣନା_Onesp = oSelp->fromp()->cast<Dfg_ଗଣନା_Ones>()) {
                    // Zero extended count ones
                    ଯଦି (DfgSel* ଅଚଳ selp = ଗଣନା_Onesp->srcp()->cast<DfgSel>()) {
                        ଫେରନ୍ତୁ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            ଯଦି (oSelp->dtype() == m_bitDType) {  //
                ଫେରନ୍ତୁ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ଫେରନ୍ତୁ {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        ଯଦି (DfgShiftR* ଅଚଳ shiftrp = vtxp->cast<DfgShiftR>()) {
            ଯଦି (DfgConst* ଅଚଳ rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                ଯଦି (rConstp->toU32() == shiftrp->width() - 1) {
                    ଫେରନ୍ତୁ {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            ଫେରନ୍ତୁ {nullptr, 0, 0};
        }
        // Not applicable
        ଫେରନ୍ତୁ {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              ଅଚଳ uint32_t width) {
        ଯେତେବେଳେ (ସତ୍ୟ) {
            ଅଚଳ uint32_t msb = lsb + width - 1;

            // Sel from Concat
            ଯଦି (DfgConcat* ଅଚଳ concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* ଅଚଳ lhsp = concatp->lhsp();
                DfgVertex* ଅଚଳ rhsp = concatp->rhsp();

                ଯଦି (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        ଜାରି;
                    }
                } ଅନ୍ୟଥା ଯଦି (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        ଜାରି;
                    }
                }
            }

            ଯଦି (DfgRep* ଅଚଳ repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ଅଚଳ uint32_t srcWidth = repp->srcp()->width();
                ଯଦି (width <= srcWidth) {
                    ଅଚଳ uint32_t newLsb = lsb % srcWidth;
                    ଅଚଳ uint32_t newMsb = newLsb + width - 1;
                    ଯଦି (newMsb < srcWidth) {
                        APPLYING(ଠେଲ_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            ଜାରି;
                        }
                    }
                }
            }

            // Sel from Sel
            ଯଦି (DfgSel* ଅଚଳ selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    ଜାରି;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            ଯଦି (DfgVarPacked* ଅଚଳ varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ଅଚଳ auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* ଅଚଳ driverp = pair.first;
                ଅଚଳ uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                ଯଦି (driverp) {
                    ଯଦି (driverp == varp->defaultp()) {
                        APPLYING(ଠେଲ_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ଜାରି;
                        }
                    } ଅନ୍ୟଥା {
                        APPLYING(ଠେଲ_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ଜାରି;
                        }
                    }
                }
            }

            // No patterns matched, stop
            ଭାଙ୍ଗ;
        }
        ଫେରନ୍ତୁ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        ଯଦି (ap == bp) ଫେରନ୍ତୁ {ap, ap->width()};

        // If both constants, check LSBs
        ଯଦି (DfgConst* ଅଚଳ aConstp = ap->cast<DfgConst>()) {
            ଯଦି (DfgConst* ଅଚଳ bConstp = bp->cast<DfgConst>()) {
                ଅଚଳ V3_ସଂଖ୍ୟା& a_ଅଙ୍କ = aConstp->ଅଙ୍କ();
                ଅଚଳ V3_ସଂଖ୍ୟା& b_ଅଙ୍କ = bConstp->ଅଙ୍କ();
                // Max match is the shorter constant
                ଅଚଳ uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                ପାଇଁ (; matchWidth < maxMatch; ++matchWidth) {
                    ଯଦି (a_ଅଙ୍କ.bitIs0(matchWidth) != b_ଅଙ୍କ.bitIs0(matchWidth)) ଭାଙ୍ଗ;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ଅଚଳ shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                ଫେରନ୍ତୁ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        ଯଦି (DfgConcat* ଅଚଳ catp = ap->cast<DfgConcat>()) ଫେରନ୍ତୁ commonLSBs(catp->rhsp(), bp);
        ଯଦି (DfgConcat* ଅଚଳ catp = bp->cast<DfgConcat>()) ଫେରନ୍ତୁ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        ଯଦି (DfgSel* ଅଚଳ selp = ap->cast<DfgSel>()) {
            ଯଦି (selp->lsb() == 0) {
                DfgVertex* ଅଚଳ fromp = selp->fromp();
                ଅଚଳ std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                ଫେରନ୍ତୁ {common.first, std::min(common.second, selp->width())};
            }
        }
        ଯଦି (DfgSel* ଅଚଳ selp = bp->cast<DfgSel>()) {
            ଯଦି (selp->lsb() == 0) {
                DfgVertex* ଅଚଳ fromp = selp->fromp();
                ଅଚଳ std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                ଫେରନ୍ତୁ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        ଫେରନ୍ତୁ {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        ଯଦି (ap == bp) ଫେରନ୍ତୁ {ap, ap->width()};

        // If both constants, check MSBs
        ଯଦି (DfgConst* ଅଚଳ aConstp = ap->cast<DfgConst>()) {
            ଯଦି (DfgConst* ଅଚଳ bConstp = bp->cast<DfgConst>()) {
                ଅଚଳ uint32_t aMsb = aConstp->width() - 1;
                ଅଚଳ uint32_t bMsb = bConstp->width() - 1;
                ଅଚଳ V3_ସଂଖ୍ୟା& a_ଅଙ୍କ = aConstp->ଅଙ୍କ();
                ଅଚଳ V3_ସଂଖ୍ୟା& b_ଅଙ୍କ = bConstp->ଅଙ୍କ();
                // Max match is the shorter constant
                ଅଚଳ uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                ପାଇଁ (; matchWidth < maxMatch; ++matchWidth) {
                    ଯଦି (a_ଅଙ୍କ.bitIs0(aMsb - matchWidth) != b_ଅଙ୍କ.bitIs0(bMsb - matchWidth)) ଭାଙ୍ଗ;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ଅଚଳ shorterp = aMsb < bMsb ? aConstp : bConstp;
                ଫେରନ୍ତୁ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        ଯଦି (DfgConcat* ଅଚଳ catp = ap->cast<DfgConcat>()) ଫେରନ୍ତୁ commonMSBs(catp->lhsp(), bp);
        ଯଦି (DfgConcat* ଅଚଳ catp = bp->cast<DfgConcat>()) ଫେରନ୍ତୁ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        ଯଦି (DfgSel* ଅଚଳ selp = ap->cast<DfgSel>()) {
            DfgVertex* ଅଚଳ fromp = selp->fromp();
            ଯଦି (selp->msb() == fromp->width() - 1) {
                ଅଚଳ std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                ଫେରନ୍ତୁ {common.first, std::min(common.second, selp->width())};
            }
        }
        ଯଦି (DfgSel* ଅଚଳ selp = bp->cast<DfgSel>()) {
            DfgVertex* ଅଚଳ fromp = selp->fromp();
            ଯଦି (selp->msb() == fromp->width() - 1) {
                ଅଚଳ std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                ଫେରନ୍ତୁ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        ଫେରନ୍ତୁ {nullptr, 0};
    }

    // VISIT methods

    ଶୂନ୍ୟ visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    ଶୂନ୍ୟ visit(Dfg_ଗଣନା_Ones* ଅଚଳ vtxp) override {
        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgExtend* ଅଚଳ vtxp) override {
        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* ଅଚଳ zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ଫେରନ୍ତୁ;
        }
    }

    ଶୂନ୍ୟ visit(DfgExtendS* ଅଚଳ vtxp) override {
        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgLogNot* ଅଚଳ vtxp) override {
        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgNegate* ଅଚଳ vtxp) override {
        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgNot* ଅଚଳ vtxp) override {
        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ;

        // Not of Cond
        ଯଦି (DfgCond* ଅଚଳ condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            ଯଦି (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ଠେଲ_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ଅଚଳ newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ଅଚଳ newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ଅଚଳ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ଫେରନ୍ତୁ;
                }
            }
        }

        // Not of Not
        ଯଦି (DfgNot* ଅଚଳ notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            ଯଦି (DfgEq* ଅଚଳ eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }

            // Not of Neq
            ଯଦି (DfgNeq* ଅଚଳ neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }

            // Not of unsigned comparisons
            ଯଦି (DfgLt* ଅଚଳ p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (DfgGte* ଅଚଳ p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (DfgGt* ଅଚଳ p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (DfgLte* ଅଚଳ p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }

            // Not of signed comparisons
            ଯଦି (DfgLtS* ଅଚଳ p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (DfgGteS* ଅଚଳ p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (DfgGtS* ଅଚଳ p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (DfgLteS* ଅଚଳ p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgOneHot* ଅଚଳ vtxp) override {
        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgOneHot0* ଅଚଳ vtxp) override {
        ଯଦି (foldUnary(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgRedOr* ଅଚଳ vtxp) override {
        ଯଦି (optimizeReduction(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgRedAnd* ଅଚଳ vtxp) override {
        ଯଦି (optimizeReduction(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgRedXor* ଅଚଳ vtxp) override {
        ଯଦି (optimizeReduction(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgSel* ଅଚଳ vtxp) override {
        DfgVertex* ଅଚଳ fromp = vtxp->fromp();

        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଅଚଳ uint32_t lsb = vtxp->lsb();
        ଅଚଳ uint32_t width = vtxp->width();
        ଅଚଳ uint32_t msb = lsb + width - 1;

        ଯଦି (DfgConst* ଅଚଳ constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ଅଚଳ resp = makeZero(flp, width);
                resp->ଅଙ୍କ().opSel(constp->ଅଙ୍କ(), msb, lsb);
                replace(resp);
                ଫେରନ୍ତୁ;
            }
        }

        // Full width select, replace with the source.
        ଯଦି (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                ଫେରନ୍ତୁ;
            }
        }

        // Unwind through bit packing in one go
        {
            ଅଚଳ auto res = unwindSel(fromp, lsb, width);
            ଯଦି (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                ଫେରନ୍ତୁ;
            }
        }

        // Sel from Not
        ଯଦି (DfgNot* ଅଚଳ notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            ଯଦି (!notp->hasMultipleSinks()) {
                APPLYING(ଠେଲ_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ଅଚଳ newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ଫେରନ୍ତୁ;
                }
            }
        }

        // Sel from Cond
        ଯଦି (DfgCond* ଅଚଳ condp = fromp->cast<DfgCond>()) {
            ଯଦି (!condp->hasMultipleSinks()) {
                APPLYING(ଠେଲ_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ଅଚଳ newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ଅଚଳ newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ଅଚଳ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ଫେରନ୍ତୁ;
                }
            }
        }

        // Sel from ShiftL
        ଯଦି (DfgShiftL* ଅଚଳ shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            ଯଦି (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(ଠେଲ_SEL_THROUGH_SHIFTL) {
                    DfgSel* ଅଚଳ newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgMux* ଅଚଳ vtxp) override {
        DfgVertex* ଅଚଳ fromp = vtxp->fromp();
        DfgVertex* ଅଚଳ lsbp = vtxp->lsbp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (DfgConst* ଅଚଳ lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->ଅଙ୍କ().toUInt()));
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                ଫେରନ୍ତୁ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    ଶୂନ୍ୟ visit(DfgAnd* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ଯଦି (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ଯଦି (DfgNot* ଅଚଳ lhsNotp = lhsp->cast<DfgNot>()) {
                ଯଦି (DfgNot* ଅଚଳ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* ଅଚଳ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ଫେରନ୍ତୁ;
                    }
                }
                ଯଦି (DfgNeq* ଅଚଳ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* ଅଚଳ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ଅଚଳ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (DfgConst* ଅଚଳ lConstp = lhsp->cast<DfgConst>()) {
            ଯଦି (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (DfgConcat* ଅଚଳ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ଯଦି (try_ଠେଲ_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (distributive_And_Associative_ଦ୍ୱିମିକ<DfgOr, DfgAnd>(vtxp)) ଫେରନ୍ତୁ;

        ଯଦି (try_ଠେଲ_Bitwise_Op_Through_Reductions(vtxp)) ଫେରନ୍ତୁ;

        ଯଦି (try_ଠେଲ_Bitwise_Op_Throug_Sel(vtxp)) ଫେରନ୍ତୁ;

        {
            DfgNot* ଅଚଳ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ଅଚଳ rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            ଯଦି ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (DfgAnd* ଅଚଳ rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ଅଚଳ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ଅଚଳ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                ଯଦି ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (vtxp->dtype() == m_bitDType) {
            ଯଦି (tryReplaceBitwiseWithReduction(vtxp)) ଫେରନ୍ତୁ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            ଯଦି (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            ଯଦି (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ଅଚଳ newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgOr* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ଯଦି (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ଯଦି (DfgNot* ଅଚଳ lhsNotp = lhsp->cast<DfgNot>()) {
                ଯଦି (DfgNot* ଅଚଳ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* ଅଚଳ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ଫେରନ୍ତୁ;
                    }
                }
                ଯଦି (DfgNeq* ଅଚଳ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* ଅଚଳ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ଅଚଳ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (DfgConcat* ଅଚଳ lhsConcatp = lhsp->cast<DfgConcat>()) {
            ଯଦି (DfgConcat* ଅଚଳ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ଯଦି (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    ଯଦି (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ଫେରନ୍ତୁ;
                        }
                    }
                    ଯଦି (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ଫେରନ୍ତୁ;
                        }
                    }
                }
            }
        }

        ଯଦି (DfgConst* ଅଚଳ lConstp = lhsp->cast<DfgConst>()) {
            ଯଦି (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (DfgConcat* ଅଚଳ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ଯଦି (try_ଠେଲ_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (distributive_And_Associative_ଦ୍ୱିମିକ<DfgAnd, DfgOr>(vtxp)) ଫେରନ୍ତୁ;

        ଯଦି (try_ଠେଲ_Bitwise_Op_Through_Reductions(vtxp)) ଫେରନ୍ତୁ;

        ଯଦି (try_ଠେଲ_Bitwise_Op_Throug_Sel(vtxp)) ଫେରନ୍ତୁ;

        {
            DfgNot* ଅଚଳ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ଅଚଳ rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            ଯଦି ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (DfgOr* ଅଚଳ rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ଅଚଳ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ଅଚଳ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                ଯଦି ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (vtxp->dtype() == m_bitDType) {
            ଯଦି (tryReplaceBitwiseWithReduction(vtxp)) ଫେରନ୍ତୁ;
        }
    }

    ଶୂନ୍ୟ visit(DfgXor* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (DfgConst* ଅଚଳ lConstp = lhsp->cast<DfgConst>()) {
            ଯଦି (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (DfgConcat* ଅଚଳ rConcatp = rhsp->cast<DfgConcat>()) {
                ଯଦି (try_ଠେଲ_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (try_ଠେଲ_Bitwise_Op_Through_Reductions(vtxp)) ଫେରନ୍ତୁ;

        ଯଦି (try_ଠେଲ_Bitwise_Op_Throug_Sel(vtxp)) ଫେରନ୍ତୁ;

        ଯଦି (vtxp->dtype() == m_bitDType) {
            ଯଦି (tryReplaceBitwiseWithReduction(vtxp)) ଫେରନ୍ତୁ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    ଶୂନ୍ୟ visit(DfgAdd* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                ଫେରନ୍ତୁ;
            }
        }

        ଅଚଳ std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_ଗଣନା_Ones_Term(lhsp);
        ଯଦି (DfgVertex* ଅଚଳ lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_ଗଣନା_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            ଯଦି (!std::get<0>(rTerm)) {
                ଯଦି (DfgAdd* ଅଚଳ rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_ଗଣନା_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            ଯଦି (DfgVertex* ଅଚଳ rVtxp = std::get<0>(rTerm)) {
                ଯଦି (isSame(lVtxp, rVtxp)) {
                    ଅଚଳ uint32_t lLsb = std::get<1>(lTerm);
                    ଅଚଳ uint32_t rLsb = std::get<1>(rTerm);
                    ଅଚଳ uint32_t lWidth = std::get<2>(lTerm);
                    ଅଚଳ uint32_t rWidth = std::get<2>(rTerm);
                    ତାର୍କିକ adjoined = ସତ୍ୟ;
                    uint32_t lsb = 0;
                    ଯଦି (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } ଅନ୍ୟଥା ଯଦି (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } ଅନ୍ୟଥା {
                        adjoined = ମିଥ୍ୟା;
                    }
                    ଯଦି (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_ଗଣନା_ONES) {
                            DfgSel* ଅଚଳ selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_ଗଣନା_Ones>(flp, DfgDataType::packed(32), selp);
                            ଯଦି (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } ଅନ୍ୟଥା ଯଦି (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            ଯଦି (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            ଫେରନ୍ତୁ;
                        }
                    }
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgArraySel* ଅଚଳ vtxp) override {
        DfgConst* ଅଚଳ idxp = vtxp->bitp()->cast<DfgConst>();
        ଯଦି (!idxp) ଫେରନ୍ତୁ;
        DfgVarArray* ଅଚଳ varp = vtxp->fromp()->cast<DfgVarArray>();
        ଯଦି (!varp) ଫେରନ୍ତୁ;
        AstVar* ଅଚଳ astVarp = varp->vscp()->varp();
        ଯଦି (astVarp->isForced()) ଫେରନ୍ତୁ;
        ଯଦି (astVarp->isSigUserRWPublic()) ଫେରନ୍ତୁ;
        DfgVertex* ଅଚଳ srcp = varp->srcp();
        ଯଦି (!srcp) {
            ଯଦି (vtxp->isPacked()) {
                ଯଦି (AstConsArray* ଅଚଳ iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    ଯଦି (AstConst* ଅଚଳ valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(ନୂତନ DfgConst{m_dfg, valp->fileline(), valp->ଅଙ୍କ()});
                            ଫେରନ୍ତୁ;
                        }
                    }
                }
            }
            ଫେରନ୍ତୁ;
        }

        ଯଦି (DfgSpliceArray* ଅଚଳ splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* ଅଚଳ driverp = splicep->driverAt(idxp->toSizeT());
            ଯଦି (!driverp) ଫେରନ୍ତୁ;
            DfgUnitArray* ଅଚଳ uap = driverp->cast<DfgUnitArray>();
            ଯଦି (!uap) ଫେରନ୍ତୁ;
            ଯଦି (uap->srcp()->is<DfgVertexSplice>()) ଫେରନ୍ତୁ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ଯଦି (DfgVertexVar* ଅଚଳ dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ଯଦି (dvarp->srcp()) ଫେରନ୍ତୁ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (DfgUnitArray* ଅଚଳ uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            ଯଦି (uap->srcp()->is<DfgSplicePacked>()) ଫେରନ୍ତୁ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ଯଦି (DfgVertexVar* ଅଚଳ dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ଯଦି (dvarp->srcp()) ଫେରନ୍ତୁ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgConcat* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();

        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isZero(lhsp)) {
            DfgConst* ଅଚଳ lConstp = lhsp->as<DfgConst>();
            ଯଦି (DfgSel* ଅଚଳ rSelp = rhsp->cast<DfgSel>()) {
                ଯଦି (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_ଶୀର୍ଷ_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (isZero(rhsp)) {
            DfgConst* ଅଚଳ rConstp = rhsp->as<DfgConst>();
            ଯଦି (DfgSel* ଅଚଳ lSelp = lhsp->cast<DfgSel>()) {
                ଯଦି (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (DfgNot* ଅଚଳ lNot = lhsp->cast<DfgNot>()) {
            ଯଦି (DfgNot* ଅଚଳ rNot = rhsp->cast<DfgNot>()) {
                ଯଦି (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(ଠେଲ_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ଅଚଳ newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        {
            ତାର୍କିକ swap = ମିଥ୍ୟା;
            ଯଦି (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(ଠେଲ_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    ଫେରନ୍ତୁ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ଯଦି (DfgConcat* ଅଚଳ rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* ଅଚଳ rlp = rConcatp->lhsp();
                ଯଦି (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(ଠେଲ_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* ଅଚଳ newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (DfgSel* ଅଚଳ lSelp = lhsp->cast<DfgSel>()) {
            ଯଦି (DfgSel* ଅଚଳ rSelp = rhsp->cast<DfgSel>()) {
                ଯଦି (isSame(lSelp->fromp(), rSelp->fromp())) {
                    ଯଦି (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ଫେରନ୍ତୁ;
                        }
                    }
                }
            }

            ଯଦି (DfgConcat* ଅଚଳ rConcatp = rhsp->cast<DfgConcat>()) {
                ଯଦି (DfgSel* ଅଚଳ rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    ଯଦି (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        ଯଦି (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                ଅଚଳ uint32_t width = lSelp->width() + rlSelp->width();
                                ଅଚଳ DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ଅଚଳ selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ଫେରନ୍ତୁ;
                            }
                        }
                    }
                }
            }
        }

        ଯଦି (DfgSel* ଅଚଳ rSelp = rhsp->cast<DfgSel>()) {
            ଯଦି (DfgConcat* ଅଚଳ lConcatp = lhsp->cast<DfgConcat>()) {
                ଯଦି (DfgSel* ଅଚଳ lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    ଯଦି (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        ଯଦି (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                ଅଚଳ uint32_t width = lrSelp->width() + rSelp->width();
                                ଅଚଳ DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ଅଚଳ selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ଫେରନ୍ତୁ;
                            }
                        }
                    }
                }
            }
        }

        ଯଦି (DfgConst* ଅଚଳ lConstp = lhsp->cast<DfgConst>()) {
            ଯଦି (DfgCond* ଅଚଳ rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* ଅଚଳ rtVtxp = rCondp->thenp();
                DfgVertex* ଅଚଳ reVtxp = rCondp->elsep();
                DfgConst* ଅଚଳ rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ଅଚଳ reConstp = reVtxp->cast<DfgConst>();
                ଯଦି (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(ଠେଲ_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* ଅଚଳ thenp = [&]() -> DfgVertex* {
                            File_ଧାଡ଼ି* ଅଚଳ rtFlp = rtVtxp->fileline();
                            ଯଦି (rtConstp) {
                                DfgConst* ଅଚଳ constp = makeZero(rtFlp, vtxp->width());
                                constp->ଅଙ୍କ().opConcat(lConstp->ଅଙ୍କ(), rtConstp->ଅଙ୍କ());
                                ଫେରନ୍ତୁ constp;
                            }
                            ଫେରନ୍ତୁ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* ଅଚଳ elsep = [&]() -> DfgVertex* {
                            File_ଧାଡ଼ି* ଅଚଳ reFlp = reVtxp->fileline();
                            ଯଦି (reConstp) {
                                DfgConst* ଅଚଳ constp = makeZero(reFlp, vtxp->width());
                                constp->ଅଙ୍କ().opConcat(lConstp->ଅଙ୍କ(), reConstp->ଅଙ୍କ());
                                ଫେରନ୍ତୁ constp;
                            }
                            ଫେରନ୍ତୁ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (DfgConst* ଅଚଳ rConstp = rhsp->cast<DfgConst>()) {
            ଯଦି (DfgCond* ଅଚଳ lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* ଅଚଳ ltVtxp = lCondp->thenp();
                DfgVertex* ଅଚଳ leVtxp = lCondp->elsep();
                DfgConst* ଅଚଳ ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ଅଚଳ leConstp = leVtxp->cast<DfgConst>();
                ଯଦି (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(ଠେଲ_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* ଅଚଳ thenp = [&]() -> DfgVertex* {
                            File_ଧାଡ଼ି* ଅଚଳ ltFlp = ltVtxp->fileline();
                            ଯଦି (ltConstp) {
                                DfgConst* ଅଚଳ constp = makeZero(ltFlp, vtxp->width());
                                constp->ଅଙ୍କ().opConcat(ltConstp->ଅଙ୍କ(), rConstp->ଅଙ୍କ());
                                ଫେରନ୍ତୁ constp;
                            }
                            ଫେରନ୍ତୁ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* ଅଚଳ elsep = [&]() -> DfgVertex* {
                            File_ଧାଡ଼ି* ଅଚଳ leFlp = leVtxp->fileline();
                            ଯଦି (leConstp) {
                                DfgConst* ଅଚଳ constp = makeZero(leFlp, vtxp->width());
                                constp->ଅଙ୍କ().opConcat(leConstp->ଅଙ୍କ(), rConstp->ଅଙ୍କ());
                                ଫେରନ୍ତୁ constp;
                            }
                            ଫେରନ୍ତୁ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ଅଚଳ uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            ଅଚଳ uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            ଅଚଳ uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            ତାର୍କିକ hasCrossSink = ମିଥ୍ୟା;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                ଯଦି (ଅଚଳ DfgSel* ଅଚଳ selp = sink.cast<DfgSel>()) {
                    ଅଚଳ uint32_t selLsb = selp->lsb();
                    ଅଚଳ uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                ଯଦି (ଅଚଳ DfgVarPacked* ଅଚଳ varp = sink.cast<DfgVarPacked>()) {
                    ଯଦି (!varp->hasSinks() && !varp->isObserved()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ଫେରନ୍ତୁ ସତ୍ୟ;
            });
            ଯଦି (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    ଯଦି (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    ଯଦି (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* ଅଚଳ catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ଅଚଳ sp = ନୂତନ DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* ଅଚଳ varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(ସତ୍ୟ);
                    varp->srcp(sp);
                    replace(varp);
                    ଫେରନ୍ତୁ;
                }
            }
        }

        // Convert various forms to REPLICATE
        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ଫେରନ୍ତୁ;
            }
        }
        ଯଦି (DfgRep* ଅଚଳ lRepp = lhsp->cast<DfgRep>()) {
            ଯଦି (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ଫେରନ୍ତୁ;
                }
            }
        }
        ଯଦି (DfgRep* ଅଚଳ rRepp = rhsp->cast<DfgRep>()) {
            ଯଦି (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ଫେରନ୍ତୁ;
                }
            }
        }
        ଯଦି (DfgRep* ଅଚଳ lRepp = lhsp->cast<DfgRep>()) {
            ଯଦି (DfgRep* ଅଚଳ rRepp = rhsp->cast<DfgRep>()) {
                ଯଦି (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ଯଦି (DfgConcat* ଅଚଳ rConcatp = rhsp->cast<DfgConcat>()) {
                ଯଦି (DfgRep* ଅଚଳ rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    ଯଦି (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ଅଚଳ uint32_t width = lRepp->width() + rlRepp->width();
                            ଅଚଳ DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* ଅଚଳ repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ଫେରନ୍ତୁ;
                        }
                    }
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgDiv* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        ଯଦି (DfgConst* ଅଚଳ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ଯଦି (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgDivS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        ଯଦି (DfgConst* ଅଚଳ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ଯଦି (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgGt* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgGtS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgGte* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgGteS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgLogAnd* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp) || vtxp->rhsp()->unsafe()) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();

        ଯଦି (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgLogEq* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgLogIf* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp) || vtxp->rhsp()->unsafe()) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgLogOr* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp) || vtxp->rhsp()->unsafe()) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();

        ଯଦି (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgLt* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgLtS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgLte* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgLteS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgModDiv* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgModDivS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgMul* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (DfgConst* ଅଚଳ lConstp = lhsp->cast<DfgConst>()) {
            ଯଦି (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgMulS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (DfgConst* ଅଚଳ lConstp = lhsp->cast<DfgConst>()) {
            ଯଦି (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgEq* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (DfgConst* ଅଚଳ lhsConstp = lhsp->cast<DfgConst>()) {
            ଯଦି (DfgConcat* ଅଚଳ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ଯଦି (try_ଠେଲ_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ଫେରନ୍ତୁ;
            }

            ଯଦି (rhsp->dtype() == m_bitDType) {
                ଯଦି (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ଫେରନ୍ତୁ;
                    }
                }

                ଯଦି (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgNeq* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (DfgConst* ଅଚଳ lhsConstp = lhsp->cast<DfgConst>()) {
            ଯଦି (DfgConcat* ଅଚଳ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ଯଦି (try_ଠେଲ_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ଫେରନ୍ତୁ;
            }

            ଯଦି (rhsp->dtype() == m_bitDType) {
                ଯଦି (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        ଫେରନ୍ତୁ;
                    }
                }

                ଯଦି (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgPow* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgPowSS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgPowSU* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgPowUS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgRep* ଅଚଳ vtxp) override {
        DfgVertex* ଅଚଳ srcp = vtxp->srcp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (DfgConst* ଅଚଳ sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ଅଚଳ resultp = makeZero(flp, vtxp->width());
                resultp->ଅଙ୍କ().opRepl(sConstp->ଅଙ୍କ(), vtxp->ଗଣନା_2());
                replace(resultp);
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (DfgRep* ଅଚଳ sRepp = srcp->cast<DfgRep>()) {
            ଯଦି (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgShiftL* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
        ଯଦି (optimizeShiftRHS(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();

        ଯଦି (DfgShiftL* ଅଚଳ lShiftLp = lhsp->cast<DfgShiftL>()) {
            ଯଦି (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
                    ଅଚଳ DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ଅଚଳ bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* ଅଚଳ cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ଅଚଳ addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    ଫେରନ୍ତୁ;
                }
            }
        }

        ଯଦି (DfgConst* ଅଚଳ rConstp = rhsp->cast<DfgConst>()) {
            ଅଚଳ uint32_t shiftAmount = rConstp->toU32();

            ଯଦି (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (DfgConcat* ଅଚଳ lConcatp = lhsp->cast<DfgConcat>()) {
                ଯଦି (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* ଅଚଳ lRhsp = lConcatp->rhsp();
                        DfgVertex* ଅଚଳ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ଅଚଳ uint32_t rW = shiftAmount;
                        ଅଚଳ uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        ଅଚଳ uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_ଧାଡ଼ି* ଅଚଳ flp = lConcatp->fileline();
                        DfgVertex* ଅଚଳ rp = makeZero(flp, shiftAmount);
                        DfgVertex* ଅଚଳ mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        ଯଦି (!lW) {
                            replace(np);
                            ଫେରନ୍ତୁ;
                        }
                        DfgVertex* ଅଚଳ lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ଫେରନ୍ତୁ;
                    }
                }
            }

            ଯଦି (DfgSel* ଅଚଳ lSelp = lhsp->cast<DfgSel>()) {
                ଯଦି (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ଅଚଳ uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ଅଚଳ nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        ଫେରନ୍ତୁ;
                    }
                }
            }

            ଯଦି (DfgCond* ଅଚଳ lCondp = lhsp->cast<DfgCond>()) {
                ଯଦି (!lCondp->hasMultipleSinks()) {
                    APPLYING(ଠେଲ_SHIFTL_THROUGH_COND) {
                        DfgShiftL* ଅଚଳ tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* ଅଚଳ ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgShiftR* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
        ଯଦି (optimizeShiftRHS(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();

        ଯଦି (DfgShiftR* ଅଚଳ lShiftRp = lhsp->cast<DfgShiftR>()) {
            ଯଦି (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();
                    ଅଚଳ DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ଅଚଳ bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* ଅଚଳ cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ଅଚଳ addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    ଫେରନ୍ତୁ;
                }
            }
        }

        ଯଦି (DfgConst* ଅଚଳ rConstp = rhsp->cast<DfgConst>()) {
            ଅଚଳ uint32_t shiftAmount = rConstp->toU32();

            ଯଦି (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (DfgConcat* ଅଚଳ lConcatp = lhsp->cast<DfgConcat>()) {
                ଯଦି (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* ଅଚଳ lRhsp = lConcatp->rhsp();
                        DfgVertex* ଅଚଳ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ଅଚଳ uint32_t lW = shiftAmount;
                        ଅଚଳ uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        ଅଚଳ uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_ଧାଡ଼ି* ଅଚଳ flp = lConcatp->fileline();
                        DfgVertex* ଅଚଳ lp = makeZero(flp, shiftAmount);
                        DfgVertex* ଅଚଳ mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        ଯଦି (!rW) {
                            replace(np);
                            ଫେରନ୍ତୁ;
                        }
                        DfgVertex* ଅଚଳ rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ଫେରନ୍ତୁ;
                    }
                }
            }

            ଯଦି (DfgSel* ଅଚଳ lSelp = lhsp->cast<DfgSel>()) {
                ଯଦି (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ଅଚଳ uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ଅଚଳ nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        ଫେରନ୍ତୁ;
                    }
                }
            }

            ଯଦି (DfgCond* ଅଚଳ lCondp = lhsp->cast<DfgCond>()) {
                ଯଦି (!lCondp->hasMultipleSinks()) {
                    APPLYING(ଠେଲ_SHIFTR_THROUGH_COND) {
                        DfgShiftR* ଅଚଳ tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* ଅଚଳ ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgShiftRS* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;
        ଯଦି (optimizeShiftRHS(vtxp)) ଫେରନ୍ତୁ;
    }

    ଶୂନ୍ୟ visit(DfgSub* ଅଚଳ vtxp) override {
        ଯଦି (ଦ୍ୱିମିକ(vtxp)) ଫେରନ୍ତୁ;

        DfgVertex* ଅଚଳ lhsp = vtxp->lhsp();
        DfgVertex* ଅଚଳ rhsp = vtxp->rhsp();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (DfgConst* ଅଚଳ rConstp = rhsp->cast<DfgConst>()) {
            ଯଦି (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (vtxp->dtype() == m_bitDType && rConstp->has_ମୂଲ୍ୟ(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgMatchMasked* ଅଚଳ vtxp) override {
        ଯଦି (DfgConst* ଅଚଳ constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ଅଚଳ matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->ଅଙ୍କ(), matchVarp)));
                ଫେରନ୍ତୁ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    ଶୂନ୍ୟ visit(DfgCond* ଅଚଳ vtxp) override {
        DfgVertex* ଅଚଳ condp = vtxp->condp();
        DfgVertex* ଅଚଳ thenp = vtxp->thenp();
        DfgVertex* ଅଚଳ elsep = vtxp->elsep();
        File_ଧାଡ଼ି* ଅଚଳ flp = vtxp->fileline();

        ଯଦି (condp->dtype() != m_bitDType) ଫେରନ୍ତୁ;

        ଯଦି (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (DfgNot* ଅଚଳ condNotp = condp->cast<DfgNot>()) {
            ଯଦି (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ଫେରନ୍ତୁ;
                }
            }
        }

        ଯଦି (DfgNeq* ଅଚଳ condNeqp = condp->cast<DfgNeq>()) {
            ଯଦି (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ଅଚଳ newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ଫେରନ୍ତୁ;
                }
            }
        }

        ଯଦି (DfgNot* ଅଚଳ thenNotp = thenp->cast<DfgNot>()) {
            ଯଦି (DfgNot* ଅଚଳ elseNotp = elsep->cast<DfgNot>()) {
                ଯଦି (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ଅଚଳ newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (DfgOr* ଅଚଳ condOrp = condp->cast<DfgOr>()) {
            ଯଦି (DfgCond* ଅଚଳ thenCondp = thenp->cast<DfgCond>()) {
                ଯଦି (!thenCondp->hasMultipleSinks()) {
                    ଯଦି (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ଅଚଳ ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ଫେରନ୍ତୁ;
                        }
                    }
                    ଯଦି (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ଅଚଳ ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ଫେରନ୍ତୁ;
                        }
                    }
                }
            }
        }

        ଯଦି (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            ଯଦି (DfgAdd* ଅଚଳ thenAddp = thenp->cast<DfgAdd>()) {
                ଯଦି (DfgConst* ଅଚଳ constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    ଯଦି (constp->has_ମୂଲ୍ୟ(1)) {
                        ଯଦି (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ଅଚଳ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_ଧାଡ଼ି* ଅଚଳ thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ଫେରନ୍ତୁ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            ଯଦି (DfgSub* ଅଚଳ thenSubp = thenp->cast<DfgSub>()) {
                ଯଦି (DfgConst* ଅଚଳ constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    ଯଦି (constp->has_ମୂଲ୍ୟ(1)) {
                        ଯଦି (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ଅଚଳ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_ଧାଡ଼ି* ଅଚଳ thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ଫେରନ୍ତୁ;
                            }
                        }
                    }
                }
            }
        }

        ଯଦି (vtxp->dtype() == m_bitDType) {
            ଯଦି (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ଫେରନ୍ତୁ;
                }
            }
        }

        ଯଦି (vtxp->width() <= VL_QUADSIZE) {
            ଯଦି (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* ଅଚଳ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* ଅଚଳ maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* ଅଚଳ maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* ଅଚଳ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ଫେରନ୍ତୁ;
                }
            }

            ଯଦି (DfgOr* ଅଚଳ tOrp = thenp->cast<DfgOr>()) {
                ଯଦି (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* ଅଚଳ maskp = replicate(vtxp, condp);
                        DfgAnd* ଅଚଳ andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ଫେରନ୍ତୁ;
                    }
                }
                ଯଦି (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* ଅଚଳ maskp = replicate(vtxp, condp);
                        DfgAnd* ଅଚଳ andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ଫେରନ୍ତୁ;
                    }
                }
            }
        }

        ଯଦି (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ଅଚଳ std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            ଯଦି (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ଅଚଳ uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    ଯଦି (rWidth != rhsp->width()) {
                        ଅଚଳ DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    ଯଦି (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ଫେରନ୍ତୁ;
                    }
                    // Create new LHS
                    ଅଚଳ uint32_t lWidth = vtxp->width() - rWidth;
                    ଅଚଳ DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* ଅଚଳ lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* ଅଚଳ lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* ଅଚଳ lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ଫେରନ୍ତୁ;
                }
            }

            ଅଚଳ std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            ଯଦି (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ଅଚଳ uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    ଯଦି (lWidth != lhsp->width()) {
                        ଅଚଳ DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    ଯଦି (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ଫେରନ୍ତୁ;
                    }
                    // Create new RHS
                    ଅଚଳ uint32_t rWidth = vtxp->width() - lWidth;
                    ଅଚଳ DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* ଅଚଳ rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* ଅଚଳ rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* ଅଚଳ rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ଫେରନ୍ତୁ;
                }
            }
        }

        ଯଦି (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                ଯଦି (ଅଚଳ uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ଅଚଳ zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                ଯଦି (ଅଚଳ uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ଅଚଳ zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ଫେରନ୍ତୁ;
            }
        }

        ଯଦି (DfgCond* ଅଚଳ tCondp = thenp->cast<DfgCond>()) {
            ଯଦି (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ଫେରନ୍ତୁ;
                }
            }
        }

        ଯଦି (DfgCond* ଅଚଳ eCondp = elsep->cast<DfgCond>()) {
            ଯଦି (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ଫେରନ୍ତୁ;
                }
            }
        }
    }

    ଶୂନ୍ୟ visit(DfgVertexVar* ଅଚଳ vtxp) override {
        ଯଦି (vtxp->hasSinks()) ଫେରନ୍ତୁ;
        ଯଦି (vtxp->isObserved()) ଫେରନ୍ତୁ;
        ଯଦି (vtxp->defaultp()) ଫେରନ୍ତୁ;

        // If undriven, or driven from another var, it is completely redundant.
        ଯଦି (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ଫେରନ୍ତୁ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        ତାର୍କିକ foundOne = ମିଥ୍ୟା;
        ଅଚଳ ତାର୍କିକ keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> ତାର୍କିକ {
            // Ignore non-observable variable sinks. These can be eliminated.
            ଯଦି (ଅଚଳ DfgVertexVar* ଅଚଳ varp = sink.cast<DfgVertexVar>()) {
                ଯଦି (!varp->hasSinks() && !varp->isObserved()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            // Keep if found more than one sink
            ଯଦି (foundOne) ଫେରନ୍ତୁ ସତ୍ୟ;
            // Mark first sink found
            foundOne = ସତ୍ୟ;
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        });
        ଯଦି (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ଫେରନ୍ତୁ;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        ପାଇଁ (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ଠେଲ_back(nullptr);
        m_iterList.ଠେଲ_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        ପାଇଁ (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        ପାଇଁ (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        ଯେତେବେଳେ (ସତ୍ୟ) {
            // Process the work list - keep the placeholder at index 0
            ଯେତେବେଳେ (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.କାଢ଼_back();
                ଯଦି (!m_vtxp) ଜାରି;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                ଯଦି (DfgVertexVar* ଅଚଳ varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    ଜାରି;
                }

                // Ast references are not considered
                ଯଦି (m_vtxp->is<DfgVertexAst>()) ଜାରି;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                ଯଦି (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    ଯଦି (DfgVertex* ଅଚଳ sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            ଜାରି;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            ଯଦି (m_iterList.size() == 1) ଭାଙ୍ଗ;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                ପାଇଁ (size_t hops = 1; hops <= 4; ++hops) {
                    ପାଇଁ (size_t i = begin; i < end; ++i) {
                        DfgVertex* ଅଚଳ vtxp = m_iterList[i];
                        ଯଦି (!vtxp) ଜାରି;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            ଫେରନ୍ତୁ ମିଥ୍ୟା;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            ଫେରନ୍ତୁ ମିଥ୍ୟା;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            ପାଇଁ (DfgVertex* ଅଚଳ vtxp : m_iterList) {
                ଯଦି (!vtxp) ଜାରି;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

ଶୂନ୍ୟ V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    ଯଦି (!v3Global.opt.fDfgPeephole()) ଫେରନ୍ତୁ;
    V3DfgPeephole::apply(dfg, ctx);
}
