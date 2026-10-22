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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgCache.h"
#ಅನ್ತರ್ಭಾವ "V3DfgDataType.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPeepholePatterns.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cctype>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
ನಾಮಾಕಾಶ {

template <typename T_Reduction>
ಸಂರಚನಾ ReductionToBitwiseImpl {};
template <> ಸಂರಚನಾ ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> ಸಂರಚನಾ ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> ಸಂರಚನಾ ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
ಸಂರಚನಾ BitwiseToReductionImpl {};
template <> ಸಂರಚನಾ BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> ಸಂರಚನಾ BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> ಸಂರಚನಾ BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
ಸಂರಚನಾ IsAssociative : ಸಾರ್ವಜನಿಕ std::false_type {};
template <> ಸಂರಚನಾ IsAssociative<DfgAnd> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsAssociative<DfgOr>  : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsAssociative<DfgXor> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsAssociative<DfgAdd> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsAssociative<DfgConcat> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsAssociative<DfgMul> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsAssociative<DfgMulS> : ಸಾರ್ವಜನಿಕ std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
ಸಂರಚನಾ IsCommutative : ಸಾರ್ವಜನಿಕ std::false_type {};
template <> ಸಂರಚನಾ IsCommutative<DfgAnd> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsCommutative<DfgOr>  : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsCommutative<DfgXor> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsCommutative<DfgAdd> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsCommutative<DfgEq> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsCommutative<DfgMul> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsCommutative<DfgMulS> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsCommutative<DfgNeq> : ಸಾರ್ವಜನಿಕ std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
ಸಂರಚನಾ IsIdempotent : ಸಾರ್ವಜನಿಕ std::false_type {};
template <> ಸಂರಚನಾ IsIdempotent<DfgAnd> : ಸಾರ್ವಜನಿಕ std::true_type {};
template <> ಸಂರಚನಾ IsIdempotent<DfgOr>  : ಸಾರ್ವಜನಿಕ std::true_type {};

// Binary result dtype
template<typename Vertex>
ಧ್ರುವ DfgDataType& resultDType(ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp);
template <> ಧ್ರುವ DfgDataType& resultDType<DfgAdd>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgAnd>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgConcat>     (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgDiv>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgDivS>       (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgEq>         (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgGt>         (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgGtS>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgGte>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgGteS>       (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgLogAnd>     (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgLogEq>      (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgLogIf>      (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgLogOr>      (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgLt>         (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgLtS>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgLte>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgLteS>       (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgModDiv>     (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgModDivS>    (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgMul>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgMulS>       (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgNeq>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ DfgDataType::packed(1); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgOr>         (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgPow>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgPowSS>      (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgPowSU>      (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgPowUS>      (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgShiftL>     (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgShiftR>     (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgShiftRS>    (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgSub>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }
template <> ಧ್ರುವ DfgDataType& resultDType<DfgXor>        (ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) { ಪ್ರತ್ಯಾಗಮ lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> ಶೂನ್ಯ foldOp(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src);
template <> ಶೂನ್ಯ foldOp<Dfg_ಎಣಿಕೆ_Ones>  (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.op_ಎಣಿಕೆ_Ones(src); }
template <> ಶೂನ್ಯ foldOp<DfgExtend>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opAssign(src); }
template <> ಶೂನ್ಯ foldOp<DfgExtendS>    (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opExtendS(src, src.width()); }
template <> ಶೂನ್ಯ foldOp<DfgLogNot>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opLogNot(src); }
template <> ಶೂನ್ಯ foldOp<DfgNegate>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opNegate(src); }
template <> ಶೂನ್ಯ foldOp<DfgNot>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opNot(src); }
template <> ಶೂನ್ಯ foldOp<DfgOneHot>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opOneHot(src); }
template <> ಶೂನ್ಯ foldOp<DfgOneHot0>    (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opOneHot0(src); }
template <> ಶೂನ್ಯ foldOp<DfgRedAnd>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opRedAnd(src); }
template <> ಶೂನ್ಯ foldOp<DfgRedOr>      (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opRedOr(src); }
template <> ಶೂನ್ಯ foldOp<DfgRedXor>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> ಶೂನ್ಯ foldOp(V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
template <> ಶೂನ್ಯ foldOp<DfgAdd>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opAdd(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgAnd>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opAnd(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgConcat>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opConcat(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgDiv>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opDiv(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgDivS>       (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opDivS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgEq>         (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opEq(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgGt>         (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opGt(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgGtS>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opGtS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgGte>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opGte(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgGteS>       (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opGteS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgLogAnd>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opLogAnd(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgLogEq>      (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opLogEq(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgLogIf>      (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opLogIf(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgLogOr>      (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opLogOr(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgLt>         (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opLt(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgLtS>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opLtS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgLte>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opLte(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgLteS>       (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opLteS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgModDiv>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opModDiv(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgModDivS>    (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opModDivS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgMul>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opMul(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgMulS>       (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opMulS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgNeq>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opNeq(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgOr>         (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opOr(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgPow>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opPow(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgPowSS>      (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opPowSS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgPowSU>      (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opPowSU(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgPowUS>      (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opPowUS(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgShiftL>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opShiftL(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgShiftR>     (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opShiftR(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgShiftRS>    (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> ಶೂನ್ಯ foldOp<DfgSub>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opSub(lhs, rhs); }
template <> ಶೂನ್ಯ foldOp<DfgXor>        (V3_ಸಂಖ್ಯೆ& out, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

ವರ್ಗ V3DfgPeephole final : ಸಾರ್ವಜನಿಕ DfgVisitor {
    // TYPES
    ಸಂರಚನಾ VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        ತಾರ್ಕಿಕ m_isCachedVertex = ಅಸತ್ಯ;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ಧ್ರುವ DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ಧ್ರುವ m_tmpScopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();

    // STATIC STATE
    ಸ್ಥಿರ V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) ಯದಿ (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    ತಾರ್ಕಿಕ checkApplying(VDfgPeepholePattern id) {
        ಯದಿ (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_ಎಣಿಕೆ[id];
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* ಧ್ರುವ equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        ಪ್ರತ್ಯಾಗಮ equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    ಶೂನ್ಯ invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = ಅಸತ್ಯ;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_ಸಾಲು* flp, ಧ್ರುವ DfgDataType& dtype, Operands... operands) {
        Vertex* ಧ್ರುವ vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(ಧ್ರುವ DfgDataType& dtype, Operands... operands) {
        Vertex* ಧ್ರುವ vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ಪ್ರತ್ಯಾಗಮ vtxp;
    }

    ಶೂನ್ಯ incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    ಶೂನ್ಯ addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        ಯದಿ (vInfo.m_workListIndex) ಪ್ರತ್ಯಾಗಮ;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.ತಳ್ಳು_back(vtxp);
    }

    ಶೂನ್ಯ removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    ಶೂನ್ಯ addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        ಯದಿ (vInfo.m_iterListIndex) ಪ್ರತ್ಯಾಗಮ;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.ತಳ್ಳು_back(vtxp);
    }

    ಶೂನ್ಯ removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    ಶೂನ್ಯ deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = vtxp->cast<DfgVertexVar>();
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
            ಯದಿ (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            ಯದಿ (vInfo.m_generation != m_currentGeneration) srcps.ತಳ್ಳು_back(&src);
            vInfo.m_generation = m_currentGeneration;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        ಯದಿ (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ತಳ್ಳು_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } ಅನ್ಯಥಾ {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ಧ್ರುವ auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  ಪ್ರತ್ಯಾಗಮ srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        ಪ್ರತಿ (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        ಪ್ರತಿ (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    ಶೂನ್ಯ replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ಧ್ರುವ auto debugCallback = [&]() -> ಶೂನ್ಯ {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            ಧ್ರುವ auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ಧ್ರುವ DfgVertex& v) {  //
                ಪ್ರತ್ಯಾಗಮ cone->ಎಣಿಕೆ(&v);
            });
        };
        ಯದಿ (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ಪ್ರತ್ಯಾಗಮ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_ಸಾಲು* flp, uint32_t val) { ಪ್ರತ್ಯಾಗಮ ನೂತನ DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_ಸಾಲು* flp, uint32_t width) {
        ಪ್ರತ್ಯಾಗಮ ನೂತನ DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_ಸಾಲು* flp, ಧ್ರುವ DfgDataType& dtype) {
        ಪ್ರತ್ಯಾಗಮ makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_ಸಾಲು* flp, uint32_t width) {
        DfgConst* ಧ್ರುವ resp = makeZero(flp, width);
        resp->ಅಂಕೆ().setAllBits1();
        ಪ್ರತ್ಯಾಗಮ resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_ಸಾಲು* flp, ಧ್ರುವ DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* ಧ್ರುವ vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        ಯದಿ (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        ಯದಿ (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        ಪ್ರತ್ಯಾಗಮ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(ಧ್ರುವ DfgVertex* examplep, Operands... operands) {
        ಪ್ರತ್ಯಾಗಮ make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        ಯದಿ (vtxp->dtype() == m_bitDType) ಪ್ರತ್ಯಾಗಮ bitp;
        ಪ್ರತ್ಯಾಗಮ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSame(ಧ್ರುವ DfgVertex* ap, ಧ್ರುವ DfgVertex* bp) {
        ಯದಿ (ap == bp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (ap->dtype() != bp->dtype()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ DfgConst* ಧ್ರುವ aConstp = ap->cast<DfgConst>();
        ಯದಿ (!aConstp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ DfgConst* ಧ್ರುವ bConstp = bp->cast<DfgConst>();
        ಯದಿ (!bConstp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ aConstp->ಅಂಕೆ().isCaseEq(bConstp->ಅಂಕೆ());
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isZero(ಧ್ರುವ DfgVertex* vtxp) {
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ constp = vtxp->cast<DfgConst>()) ಪ್ರತ್ಯಾಗಮ constp->isZero();
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isOnes(ಧ್ರುವ DfgVertex* vtxp) {
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ constp = vtxp->cast<DfgConst>()) ಪ್ರತ್ಯಾಗಮ constp->isOnes();
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEqOne(ಧ್ರುವ DfgVertex* vtxp) {
        ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ constp = vtxp->cast<DfgConst>()) ಪ್ರತ್ಯಾಗಮ constp->ಅಂಕೆ().isEqOne();
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ areAdjacent(uint32_t& lsb, ಧ್ರುವ DfgSel* lSelp, ಧ್ರುವ DfgSel* rSelp) {
        ಯದಿ (!isSame(lSelp->srcp(), rSelp->srcp())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ foldUnary(Vertex* ಧ್ರುವ vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::ಮೌಲ್ಯ, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::ಮೌಲ್ಯ, "Must invoke on final class");
        ಯದಿ (DfgConst* ಧ್ರುವ srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ಧ್ರುವ resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->ಅಂಕೆ(), srcp->ಅಂಕೆ());
                replace(resultp);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ ದ್ವಿಮಾನ(Vertex* ಧ್ರುವ vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ದ್ವಿಮಾನ, Vertex>::ಮೌಲ್ಯ, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::ಮೌಲ್ಯ, "Must invoke on final class");

        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ಧ್ರುವ lConstp = lhsp->cast<DfgConst>();
        DfgConst* ಧ್ರುವ rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* ಧ್ರುವ lSamep = lhsp->cast<Vertex>();
        Vertex* ಧ್ರುವ rSamep = rhsp->cast<Vertex>();

        // Constant folding
        ಯದಿ (lConstp && rConstp) {
            APPLYING(FOLD_ದ್ವಿಮಾನ) {
                DfgConst* ಧ್ರುವ resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->ಅಂಕೆ(), lConstp->ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                replace(resultp);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }

        // Idempotent
        ಯದಿ VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::ಮೌಲ್ಯ) {
            ಯದಿ (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ದ್ವಿಮಾನ) {
                    replace(lhsp);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
        }

        // Associative
        ಯದಿ VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::ಮೌಲ್ಯ) {
            // Also commutative and idempotent
            ಯದಿ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::ಮೌಲ್ಯ && IsIdempotent<Vertex>::ಮೌಲ್ಯ) {
                ಯದಿ (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ದ್ವಿಮಾನ_LHS) {
                        replace(lSamep);
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
                ಯದಿ (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ದ್ವಿಮಾನ_RHS) {
                        replace(rSamep);
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
            }

            ಯದಿ (lConstp && rSamep) {
                ಯದಿ (DfgConst* ಧ್ರುವ rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ದ್ವಿಮಾನ_LHS_OF_RHS) {
                        DfgConst* ಧ್ರುವ cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->ಅಂಕೆ(), lConstp->ಅಂಕೆ(), rlConstp->ಅಂಕೆ());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
            }

            ಯದಿ (lSamep && rConstp) {
                ಯದಿ (DfgConst* ಧ್ರುವ lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ದ್ವಿಮಾನ_RHS_OF_LHS) {
                        DfgConst* ಧ್ರುವ cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->ಅಂಕೆ(), lrConstp->ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            ಯದಿ (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* ಧ್ರುವ ap = lSamep->lhsp();
                DfgVertex* ಧ್ರುವ bp = lSamep->rhsp();
                DfgVertex* ಧ್ರುವ cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                ಯದಿ (ap != cp && bp != cp) {
                    APPLYING(ಬಲ_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ಧ್ರುವ DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* ಧ್ರುವ bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
            }

            ಯದಿ (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* ಧ್ರುವ rlVtxp = rSamep->lhsp();
                DfgVertex* ಧ್ರುವ rrVtxp = rSamep->rhsp();

                ಯದಿ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::ಮೌಲ್ಯ) {
                    ಯದಿ (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                ಯದಿ (Vertex* ಧ್ರುವ existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    ಯದಿ (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ದ್ವಿಮಾನ_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                ಯದಿ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::ಮೌಲ್ಯ) {
                    ಯದಿ (Vertex* ಧ್ರುವ existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        ಯದಿ (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ದ್ವಿಮಾನ_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        ಯದಿ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::ಮೌಲ್ಯ) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            ಯದಿ (lConstp) {
                ಯದಿ (DfgCond* ಧ್ರುವ rCondp = rhsp->cast<DfgCond>()) {
                    ಯದಿ (!rCondp->hasMultipleSinks()) {
                        APPLYING(ತಳ್ಳು_COMMUTATIVE_ದ್ವಿಮಾನ_THROUGH_COND) {
                            DfgVertex* ಧ್ರುವ tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* ಧ್ರುವ ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                        }
                    }
                }
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ಧ್ರುವ ತಾರ್ಕಿಕ swapSides = [&]() {
            // Don't swap if not commutative, duh
            ಯದಿ VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::ಮೌಲ್ಯ) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Const go on left
            ಯದಿ (lConstp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (rConstp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // Not go on left
            ಯದಿ (lhsp->is<DfgNot>()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (rhsp->is<DfgNot>()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // Same go on right
            ಯದಿ (rSamep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (lSamep) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ಧ್ರುವ VertexInfo& lInfo = m_vInfo[lhsp];
            ಧ್ರುವ VertexInfo& rInfo = m_vInfo[rhsp];
            ಪ್ರತ್ಯಾಗಮ lInfo.m_id > rInfo.m_id;
        }();
        ಯದಿ (swapSides) {
            APPLYING(SWAP_SIDES_IN_ದ್ವಿಮಾನ) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ distributive_And_Associative_ದ್ವಿಮಾನ(Vertex* vtxp) {
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        ಯದಿ (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            ಯದಿ (Other* ಧ್ರುವ lp = lhsp->cast<Other>()) {
                ಯದಿ (Other* ಧ್ರುವ rp = rhsp->cast<Other>()) {
                    DfgVertex* ಧ್ರುವ llp = lp->lhsp();
                    DfgVertex* ಧ್ರುವ lrp = lp->rhsp();
                    DfgVertex* ಧ್ರುವ rlp = rp->lhsp();
                    DfgVertex* ಧ್ರುವ rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    ಯದಿ (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } ಅನ್ಯಥಾ ಯದಿ (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } ಅನ್ಯಥಾ ಯದಿ (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } ಅನ್ಯಥಾ ಯದಿ (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    ಯದಿ (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_ದ್ವಿಮಾನ) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                        }
                    }
                }
            }
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ
    try_ತಳ್ಳು_Bitwise_Op_Through_Concat(Vertex* ಧ್ರುವ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ಧ್ರುವ catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ಧ್ರುವ catRConstp = concatp->rhsp()->cast<DfgConst>();
        ಯದಿ (catLConstp || catRConstp) {
            APPLYING(ತಳ್ಳು_BITWISE_OP_THROUGH_CONCAT) {
                ಧ್ರುವ uint32_t width = concatp->width();
                ಧ್ರುವ DfgDataType& lDtype = concatp->lhsp()->dtype();
                ಧ್ರುವ DfgDataType& rDtype = concatp->rhsp()->dtype();
                ಧ್ರುವ uint32_t lWidth = lDtype.size();
                ಧ್ರುವ uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* ಧ್ರುವ newLhsp = [&]() -> DfgVertex* {
                    DfgConst* ಧ್ರುವ newLhsConstp = makeZero(constp->fileline(), lWidth);
                    ಯದಿ (catLConstp) {
                        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{constp->fileline(), static_cast<ಪೂರ್ಣಾಙ್ಕ>(lWidth), 0u};
                        ಅಂಕೆ.opSel(constp->ಅಂಕೆ(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->ಅಂಕೆ(), ಅಂಕೆ, catLConstp->ಅಂಕೆ());
                        ಪ್ರತ್ಯಾಗಮ newLhsConstp;
                    }
                    newLhsConstp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), width - 1, rWidth);
                    ಪ್ರತ್ಯಾಗಮ make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* ಧ್ರುವ newRhsp = [&]() -> DfgVertex* {
                    DfgConst* ಧ್ರುವ newRhsConstp = makeZero(constp->fileline(), rWidth);
                    ಯದಿ (catRConstp) {
                        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{constp->fileline(), static_cast<ಪೂರ್ಣಾಙ್ಕ>(rWidth), 0u};
                        ಅಂಕೆ.opSel(constp->ಅಂಕೆ(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->ಅಂಕೆ(), ಅಂಕೆ, catRConstp->ಅಂಕೆ());
                        ಪ್ರತ್ಯಾಗಮ newRhsConstp;
                    }
                    newRhsConstp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), rWidth - 1, 0);
                    ಪ್ರತ್ಯಾಗಮ make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ
    try_ತಳ್ಳು_Compare_Op_Through_Concat(Vertex* ಧ್ರುವ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        ಯದಿ (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ತಳ್ಳು_COMPARE_OP_THROUGH_CONCAT) {
                ಧ್ರುವ uint32_t width = concatp->width();
                ಧ್ರುವ uint32_t lWidth = concatp->lhsp()->width();
                ಧ್ರುವ uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* ಧ್ರುವ newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), width - 1, rWidth);
                Vertex* ಧ್ರುವ newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ಧ್ರುವ newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), rWidth - 1, 0);
                Vertex* ಧ್ರುವ newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                ಯದಿ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::ಮೌಲ್ಯ) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ಅನ್ಯಥಾ ಯದಿ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::ಮೌಲ್ಯ) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ಅನ್ಯಥಾ {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ try_ತಳ್ಳು_Bitwise_Op_Through_Reductions(Bitwise* ಧ್ರುವ vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        ಯದಿ (Reduction* ಧ್ರುವ lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            ಯದಿ (Reduction* ಧ್ರುವ rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* ಧ್ರುವ lSrcp = lRedp->srcp();
                DfgVertex* ಧ್ರುವ rSrcp = rRedp->srcp();
                ಯದಿ (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(ತಳ್ಳು_BITWISE_THROUGH_REDUCTION) {
                        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
                        Bitwise* ಧ್ರುವ bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
            }
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    ಸ್ಥಿರ std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        ಯದಿ (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ಪ್ರತ್ಯಾಗಮ {nullptr, nullptr};
        }
        ಯದಿ (vtxp->hasMultipleSinks()) ಪ್ರತ್ಯಾಗಮ {nullptr, nullptr};
        Dfg_Vertex_ದ್ವಿಮಾನ* ಧ್ರುವ bwp = vtxp->as<Dfg_Vertex_ದ್ವಿಮಾನ>();
        DfgSel* ಧ್ರುವ lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ಧ್ರುವ rSelp = bwp->rhsp()->cast<DfgSel>();
        ಯದಿ (!lSelp || !rSelp) ಪ್ರತ್ಯಾಗಮ {nullptr, nullptr};
        DfgSel* ಧ್ರುವ nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* ಧ್ರುವ wSelp = nSelp == lSelp ? rSelp : lSelp;
        ಧ್ರುವ uint32_t nWidth = nSelp->fromp()->width();
        ಯದಿ (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ಪ್ರತ್ಯಾಗಮ {nullptr, nullptr};
        ಯದಿ (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            ಪ್ರತ್ಯಾಗಮ {nullptr, nullptr};
        }
        ಪ್ರತ್ಯಾಗಮ {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ try_ತಳ್ಳು_Bitwise_Op_Throug_Sel(Bitwise* ಧ್ರುವ vtxp) {
        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();

        ಯದಿ (DfgSel* ಧ್ರುವ lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ಯದಿ (DfgSel* ಧ್ರುವ selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } ಅನ್ಯಥಾ ಯದಿ (Bitwise* ಧ್ರುವ bitwisep = rhsp->cast<Bitwise>()) {
                ಯದಿ (DfgSel* ಧ್ರುವ rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } ಅನ್ಯಥಾ ಯದಿ (DfgSel* ಧ್ರುವ rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            ಯದಿ (rSelp) {
                DfgVertex* ಧ್ರುವ lFromp = lSelp->fromp();
                DfgVertex* ಧ್ರುವ rFromp = rSelp->fromp();
                ಯದಿ (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ತಳ್ಳು_BITWISE_THROUGH_SEL) {
                        Bitwise* ಧ್ರುವ bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        ಯದಿ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
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
            Bitwise* ಧ್ರುವ rChainp = rhsp->cast<Bitwise>();
            ಯದಿ (rChainp && rChainp->hasMultipleSinks()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            DfgVertex* ಧ್ರುವ rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ಧ್ರುವ auto lSels = pushableUnalignedSels(lhsp);
            ಧ್ರುವ auto rSels = pushableUnalignedSels(rTermp);
            ಯದಿ (!lSels.first && !rSels.first) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

            ಧ್ರುವ DfgVertex* ಧ್ರುವ opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            ಯದಿ (opp->is<Bitwise>()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಧ್ರುವ auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* ಧ್ರುವ nFromp = sels.first->fromp();
            DfgVertex* ಧ್ರುವ wFromp = sels.second->fromp();
            ಧ್ರುವ uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            ಧ್ರುವ auto matches = [&](ಧ್ರುವ DfgVertex* termp, ಧ್ರುವ auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                ಯದಿ (tSels.first) {
                    ಪ್ರತ್ಯಾಗಮ termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                ಯದಿ (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                ಯದಿ (ಧ್ರುವ DfgSel* ಧ್ರುವ selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                ಯದಿ (termp->type() != opp->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                // Operands might have been swapped since
                ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* ಧ್ರುವ bwp = termp->as<Dfg_Vertex_ದ್ವಿಮಾನ>();
                ಧ್ರುವ ತಾರ್ಕಿಕ nIsLhs = bwp->lhsp() == nFromp;
                ಯದಿ (!nIsLhs && bwp->rhsp() != nFromp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                ಧ್ರುವ DfgSel* ಧ್ರುವ slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ಪ್ರತ್ಯಾಗಮ slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            ಯದಿ (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

            APPLYING(ತಳ್ಳು_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
                ಧ್ರುವ DfgDataType& dtype = nFromp->dtype();
                DfgSel* ಧ್ರುವ slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                ಯದಿ (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } ಅನ್ಯಥಾ ಯದಿ (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } ಅನ್ಯಥಾ {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ಧ್ರುವ auto ತಳ್ಳು = [&](DfgVertex* termp, ಧ್ರುವ DfgSel* nSelp) -> DfgVertex* {
                    ಪ್ರತ್ಯಾಗಮ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* ಧ್ರುವ newLhsp = ತಳ್ಳು(lhsp, lSels.first);
                DfgVertex* ಧ್ರುವ newRTermp = ತಳ್ಳು(rTermp, rSels.first);
                ಯದಿ (rChainp) {
                    DfgVertex* ಧ್ರುವ newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } ಅನ್ಯಥಾ {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isConcatFoldable(ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        ಯದಿ (isSame(lhsp, rhsp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ DfgRep* ಧ್ರುವ lRepp = lhsp->cast<DfgRep>()) {
            ಯದಿ (isSame(lRepp->srcp(), rhsp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (ಧ್ರುವ DfgRep* ಧ್ರುವ rRepp = rhsp->cast<DfgRep>()) {
            ಯದಿ (isSame(lhsp, rRepp->srcp())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // Concatenation of adjoining selects is a select
        ಯದಿ (ಧ್ರುವ DfgSel* ಧ್ರುವ lSelp = lhsp->cast<DfgSel>()) {
            ಯದಿ (ಧ್ರುವ DfgSel* ಧ್ರುವ rSelp = rhsp->cast<DfgSel>()) {
                ಪ್ರತ್ಯಾಗಮ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        ಯದಿ (ಧ್ರುವ DfgNot* ಧ್ರುವ lNotp = lhsp->cast<DfgNot>()) {
            ಯದಿ (ಧ್ರುವ DfgNot* ಧ್ರುವ rNotp = rhsp->cast<DfgNot>()) {
                ಪ್ರತ್ಯಾಗಮ !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSameBitwise(ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) {
        ಯದಿ (lhsp->type() != rhsp->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isConcatSimplifiable(ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp) {
        ಯದಿ (isConcatFoldable(lhsp, rhsp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (!isSameBitwise(lhsp, rhsp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* ಧ್ರುವ lp = lhsp->as<Dfg_Vertex_ದ್ವಿಮಾನ>();
        ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* ಧ್ರುವ rp = rhsp->as<Dfg_Vertex_ದ್ವಿಮಾನ>();
        ಯದಿ (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isConcatPushableThroughBitwise(ಧ್ರುವ DfgVertex* lhsp, ಧ್ರುವ DfgVertex* rhsp,
                                               ತಾರ್ಕಿಕ& swap) {
        ಯದಿ (!isSameBitwise(lhsp, rhsp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* ಧ್ರುವ lp = lhsp->as<Dfg_Vertex_ದ್ವಿಮಾನ>();
        ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* ಧ್ರುವ rp = rhsp->as<Dfg_Vertex_ದ್ವಿಮಾನ>();
        swap = ಅಸತ್ಯ;
        ಯದಿ (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        swap = ಸತ್ಯ;
        ಯದಿ (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_ಸಾಲು* flp, ಧ್ರುವ DfgVertex* lhsp,
                                              ಧ್ರುವ DfgVertex* rhsp, ತಾರ್ಕಿಕ swap) {
        ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* ಧ್ರುವ lp = lhsp->as<Dfg_Vertex_ದ್ವಿಮಾನ>();
        ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* ಧ್ರುವ rp = rhsp->as<Dfg_Vertex_ದ್ವಿಮಾನ>();
        ಧ್ರುವ DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* ಧ್ರುವ cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* ಧ್ರುವ dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* ಧ್ರುವ newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ಧ್ರುವ newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        ಯದಿ (lp->is<DfgAnd>()) ಪ್ರತ್ಯಾಗಮ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        ಯದಿ (lp->is<DfgOr>()) ಪ್ರತ್ಯಾಗಮ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ಪ್ರತ್ಯಾಗಮ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();

        ಯದಿ (DfgSel* ಧ್ರುವ lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ಯದಿ (DfgSel* ಧ್ರುವ selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } ಅನ್ಯಥಾ ಯದಿ (Bitwise* ಧ್ರುವ rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } ಅನ್ಯಥಾ ಯದಿ (Reduction* ಧ್ರುವ rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            ಯದಿ (rSelp) {
                uint32_t lsb = 0;
                ಯದಿ (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        ಧ್ರುವ DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* ಧ್ರುವ newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        ಯದಿ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }
            }
        }

        ಯದಿ (Reduction* ಧ್ರುವ lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            ಯದಿ (!rRedp) {
                ಯದಿ (Bitwise* ಧ್ರುವ rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            ಯದಿ (rRedp) {
                ಯದಿ (DfgSel* ಧ್ರುವ lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    ಯದಿ (DfgSel* ಧ್ರುವ rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ಯದಿ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                ಧ್ರುವ DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ಧ್ರುವ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                ಯದಿ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                            }
                        }
                    }
                }
            }
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ optimizeReduction(Reduction* ಧ್ರುವ vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

        DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        // Reduction of 1-bit value
        ಯದಿ (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }

        ಯದಿ (DfgCond* ಧ್ರುವ condp = srcp->cast<DfgCond>()) {
            ಯದಿ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ತಳ್ಳು_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ಧ್ರುವ newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ಧ್ರುವ newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ಧ್ರುವ newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
        }

        ಯದಿ (DfgConcat* ಧ್ರುವ concatp = srcp->cast<DfgConcat>()) {
            ಯದಿ (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(ತಳ್ಳು_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ಧ್ರುವ lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* ಧ್ರುವ rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
        }

        ಯದಿ (Bitwise* ಧ್ರುವ bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            ಯದಿ (!bitwisep->hasMultipleSinks()) {
                ಯದಿ (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(ತಳ್ಳು_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* ಧ್ರುವ newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* ಧ್ರುವ newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                }

                ಯದಿ (DfgSel* ಧ್ರುವ lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    ಯದಿ (DfgSel* ಧ್ರುವ rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ಯದಿ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ತಳ್ಳು_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                ಧ್ರುವ DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ಧ್ರುವ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                            }
                        }
                    }
                }
            }
        }

        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT ತಾರ್ಕಿಕ optimizeShiftRHS(Shift* ಧ್ರುವ vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ದ್ವಿಮಾನ, Shift>::ಮೌಲ್ಯ, "Must invoke on binary");
        static_assert(std::is_final<Shift>::ಮೌಲ್ಯ, "Must invoke on final class");
        ಯದಿ (ಧ್ರುವ DfgConcat* ಧ್ರುವ concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            ಯದಿ (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_ಎಣಿಕೆ_Ones_Term(DfgVertex* vtxp) {
        ಯದಿ (DfgConcat* ಧ್ರುವ oCatp = vtxp->cast<DfgConcat>()) {
            ಯದಿ (isZero(oCatp->lhsp())) {
                ಯದಿ (Dfg_ಎಣಿಕೆ_Ones* ಧ್ರುವ ಎಣಿಕೆ_Onesp = oCatp->rhsp()->cast<Dfg_ಎಣಿಕೆ_Ones>()) {
                    // Zero extended count ones
                    ಯದಿ (DfgSel* ಧ್ರುವ selp = ಎಣಿಕೆ_Onesp->srcp()->cast<DfgSel>()) {
                        ಪ್ರತ್ಯಾಗಮ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } ಅನ್ಯಥಾ ಯದಿ (DfgSel* ಧ್ರುವ selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    ಯದಿ (selp->dtype() == m_bitDType) {  //
                        ಪ್ರತ್ಯಾಗಮ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            ಪ್ರತ್ಯಾಗಮ {nullptr, 0, 0};
        }
        ಯದಿ (Dfg_ಎಣಿಕೆ_Ones* ಧ್ರುವ ಎಣಿಕೆ_Onesp = vtxp->cast<Dfg_ಎಣಿಕೆ_Ones>()) {
            // Simple count ones
            ಯದಿ (DfgSel* ಧ್ರುವ selp = ಎಣಿಕೆ_Onesp->srcp()->cast<DfgSel>()) {
                ಪ್ರತ್ಯಾಗಮ {selp->fromp(), selp->lsb(), selp->width()};
            }
            ಪ್ರತ್ಯಾಗಮ {nullptr, 0, 0};
        }
        ಯದಿ (DfgSel* ಧ್ರುವ oSelp = vtxp->cast<DfgSel>()) {
            ಯದಿ (oSelp->lsb() == 0) {
                // Truncated count ones
                ಯದಿ (Dfg_ಎಣಿಕೆ_Ones* ಧ್ರುವ ಎಣಿಕೆ_Onesp = oSelp->fromp()->cast<Dfg_ಎಣಿಕೆ_Ones>()) {
                    // Zero extended count ones
                    ಯದಿ (DfgSel* ಧ್ರುವ selp = ಎಣಿಕೆ_Onesp->srcp()->cast<DfgSel>()) {
                        ಪ್ರತ್ಯಾಗಮ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            ಯದಿ (oSelp->dtype() == m_bitDType) {  //
                ಪ್ರತ್ಯಾಗಮ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ಪ್ರತ್ಯಾಗಮ {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        ಯದಿ (DfgShiftR* ಧ್ರುವ shiftrp = vtxp->cast<DfgShiftR>()) {
            ಯದಿ (DfgConst* ಧ್ರುವ rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                ಯದಿ (rConstp->toU32() == shiftrp->width() - 1) {
                    ಪ್ರತ್ಯಾಗಮ {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            ಪ್ರತ್ಯಾಗಮ {nullptr, 0, 0};
        }
        // Not applicable
        ಪ್ರತ್ಯಾಗಮ {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              ಧ್ರುವ uint32_t width) {
        ಯಾವತ್ (ಸತ್ಯ) {
            ಧ್ರುವ uint32_t msb = lsb + width - 1;

            // Sel from Concat
            ಯದಿ (DfgConcat* ಧ್ರುವ concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* ಧ್ರುವ lhsp = concatp->lhsp();
                DfgVertex* ಧ್ರುವ rhsp = concatp->rhsp();

                ಯದಿ (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        ಅನುವರ್ತ;
                    }
                } ಅನ್ಯಥಾ ಯದಿ (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        ಅನುವರ್ತ;
                    }
                }
            }

            ಯದಿ (DfgRep* ಧ್ರುವ repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ಧ್ರುವ uint32_t srcWidth = repp->srcp()->width();
                ಯದಿ (width <= srcWidth) {
                    ಧ್ರುವ uint32_t newLsb = lsb % srcWidth;
                    ಧ್ರುವ uint32_t newMsb = newLsb + width - 1;
                    ಯದಿ (newMsb < srcWidth) {
                        APPLYING(ತಳ್ಳು_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            ಅನುವರ್ತ;
                        }
                    }
                }
            }

            // Sel from Sel
            ಯದಿ (DfgSel* ಧ್ರುವ selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    ಅನುವರ್ತ;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            ಯದಿ (DfgVarPacked* ಧ್ರುವ varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ಧ್ರುವ auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* ಧ್ರುವ driverp = pair.first;
                ಧ್ರುವ uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                ಯದಿ (driverp) {
                    ಯದಿ (driverp == varp->defaultp()) {
                        APPLYING(ತಳ್ಳು_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ಅನುವರ್ತ;
                        }
                    } ಅನ್ಯಥಾ {
                        APPLYING(ತಳ್ಳು_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ಅನುವರ್ತ;
                        }
                    }
                }
            }

            // No patterns matched, stop
            ಭಙ್ಗ;
        }
        ಪ್ರತ್ಯಾಗಮ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        ಯದಿ (ap == bp) ಪ್ರತ್ಯಾಗಮ {ap, ap->width()};

        // If both constants, check LSBs
        ಯದಿ (DfgConst* ಧ್ರುವ aConstp = ap->cast<DfgConst>()) {
            ಯದಿ (DfgConst* ಧ್ರುವ bConstp = bp->cast<DfgConst>()) {
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& a_ಅಂಕೆ = aConstp->ಅಂಕೆ();
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& b_ಅಂಕೆ = bConstp->ಅಂಕೆ();
                // Max match is the shorter constant
                ಧ್ರುವ uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                ಪ್ರತಿ (; matchWidth < maxMatch; ++matchWidth) {
                    ಯದಿ (a_ಅಂಕೆ.bitIs0(matchWidth) != b_ಅಂಕೆ.bitIs0(matchWidth)) ಭಙ್ಗ;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ಧ್ರುವ shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                ಪ್ರತ್ಯಾಗಮ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        ಯದಿ (DfgConcat* ಧ್ರುವ catp = ap->cast<DfgConcat>()) ಪ್ರತ್ಯಾಗಮ commonLSBs(catp->rhsp(), bp);
        ಯದಿ (DfgConcat* ಧ್ರುವ catp = bp->cast<DfgConcat>()) ಪ್ರತ್ಯಾಗಮ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        ಯದಿ (DfgSel* ಧ್ರುವ selp = ap->cast<DfgSel>()) {
            ಯದಿ (selp->lsb() == 0) {
                DfgVertex* ಧ್ರುವ fromp = selp->fromp();
                ಧ್ರುವ std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                ಪ್ರತ್ಯಾಗಮ {common.first, std::min(common.second, selp->width())};
            }
        }
        ಯದಿ (DfgSel* ಧ್ರುವ selp = bp->cast<DfgSel>()) {
            ಯದಿ (selp->lsb() == 0) {
                DfgVertex* ಧ್ರುವ fromp = selp->fromp();
                ಧ್ರುವ std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                ಪ್ರತ್ಯಾಗಮ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        ಪ್ರತ್ಯಾಗಮ {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        ಯದಿ (ap == bp) ಪ್ರತ್ಯಾಗಮ {ap, ap->width()};

        // If both constants, check MSBs
        ಯದಿ (DfgConst* ಧ್ರುವ aConstp = ap->cast<DfgConst>()) {
            ಯದಿ (DfgConst* ಧ್ರುವ bConstp = bp->cast<DfgConst>()) {
                ಧ್ರುವ uint32_t aMsb = aConstp->width() - 1;
                ಧ್ರುವ uint32_t bMsb = bConstp->width() - 1;
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& a_ಅಂಕೆ = aConstp->ಅಂಕೆ();
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& b_ಅಂಕೆ = bConstp->ಅಂಕೆ();
                // Max match is the shorter constant
                ಧ್ರುವ uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                ಪ್ರತಿ (; matchWidth < maxMatch; ++matchWidth) {
                    ಯದಿ (a_ಅಂಕೆ.bitIs0(aMsb - matchWidth) != b_ಅಂಕೆ.bitIs0(bMsb - matchWidth)) ಭಙ್ಗ;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ಧ್ರುವ shorterp = aMsb < bMsb ? aConstp : bConstp;
                ಪ್ರತ್ಯಾಗಮ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        ಯದಿ (DfgConcat* ಧ್ರುವ catp = ap->cast<DfgConcat>()) ಪ್ರತ್ಯಾಗಮ commonMSBs(catp->lhsp(), bp);
        ಯದಿ (DfgConcat* ಧ್ರುವ catp = bp->cast<DfgConcat>()) ಪ್ರತ್ಯಾಗಮ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        ಯದಿ (DfgSel* ಧ್ರುವ selp = ap->cast<DfgSel>()) {
            DfgVertex* ಧ್ರುವ fromp = selp->fromp();
            ಯದಿ (selp->msb() == fromp->width() - 1) {
                ಧ್ರುವ std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                ಪ್ರತ್ಯಾಗಮ {common.first, std::min(common.second, selp->width())};
            }
        }
        ಯದಿ (DfgSel* ಧ್ರುವ selp = bp->cast<DfgSel>()) {
            DfgVertex* ಧ್ರುವ fromp = selp->fromp();
            ಯದಿ (selp->msb() == fromp->width() - 1) {
                ಧ್ರುವ std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                ಪ್ರತ್ಯಾಗಮ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        ಪ್ರತ್ಯಾಗಮ {nullptr, 0};
    }

    // VISIT methods

    ಶೂನ್ಯ visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    ಶೂನ್ಯ visit(Dfg_ಎಣಿಕೆ_Ones* ಧ್ರುವ vtxp) override {
        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgExtend* ಧ್ರುವ vtxp) override {
        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* ಧ್ರುವ zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ಪ್ರತ್ಯಾಗಮ;
        }
    }

    ಶೂನ್ಯ visit(DfgExtendS* ಧ್ರುವ vtxp) override {
        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgLogNot* ಧ್ರುವ vtxp) override {
        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgNegate* ಧ್ರುವ vtxp) override {
        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgNot* ಧ್ರುವ vtxp) override {
        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        // Not of Cond
        ಯದಿ (DfgCond* ಧ್ರುವ condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            ಯದಿ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ತಳ್ಳು_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ಧ್ರುವ newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ಧ್ರುವ newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ಧ್ರುವ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        // Not of Not
        ಯದಿ (DfgNot* ಧ್ರುವ notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            ಯದಿ (DfgEq* ಧ್ರುವ eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            // Not of Neq
            ಯದಿ (DfgNeq* ಧ್ರುವ neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            // Not of unsigned comparisons
            ಯದಿ (DfgLt* ಧ್ರುವ p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (DfgGte* ಧ್ರುವ p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (DfgGt* ಧ್ರುವ p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (DfgLte* ಧ್ರುವ p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            // Not of signed comparisons
            ಯದಿ (DfgLtS* ಧ್ರುವ p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (DfgGteS* ಧ್ರುವ p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (DfgGtS* ಧ್ರುವ p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (DfgLteS* ಧ್ರುವ p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgOneHot* ಧ್ರುವ vtxp) override {
        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgOneHot0* ಧ್ರುವ vtxp) override {
        ಯದಿ (foldUnary(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgRedOr* ಧ್ರುವ vtxp) override {
        ಯದಿ (optimizeReduction(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgRedAnd* ಧ್ರುವ vtxp) override {
        ಯದಿ (optimizeReduction(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgRedXor* ಧ್ರುವ vtxp) override {
        ಯದಿ (optimizeReduction(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgSel* ಧ್ರುವ vtxp) override {
        DfgVertex* ಧ್ರುವ fromp = vtxp->fromp();

        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಧ್ರುವ uint32_t lsb = vtxp->lsb();
        ಧ್ರುವ uint32_t width = vtxp->width();
        ಧ್ರುವ uint32_t msb = lsb + width - 1;

        ಯದಿ (DfgConst* ಧ್ರುವ constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ಧ್ರುವ resp = makeZero(flp, width);
                resp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), msb, lsb);
                replace(resp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // Full width select, replace with the source.
        ಯದಿ (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // Unwind through bit packing in one go
        {
            ಧ್ರುವ auto res = unwindSel(fromp, lsb, width);
            ಯದಿ (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // Sel from Not
        ಯದಿ (DfgNot* ಧ್ರುವ notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            ಯದಿ (!notp->hasMultipleSinks()) {
                APPLYING(ತಳ್ಳು_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ಧ್ರುವ newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        // Sel from Cond
        ಯದಿ (DfgCond* ಧ್ರುವ condp = fromp->cast<DfgCond>()) {
            ಯದಿ (!condp->hasMultipleSinks()) {
                APPLYING(ತಳ್ಳು_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ಧ್ರುವ newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ಧ್ರುವ newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ಧ್ರುವ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        // Sel from ShiftL
        ಯದಿ (DfgShiftL* ಧ್ರುವ shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            ಯದಿ (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(ತಳ್ಳು_SEL_THROUGH_SHIFTL) {
                    DfgSel* ಧ್ರುವ newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgMux* ಧ್ರುವ vtxp) override {
        DfgVertex* ಧ್ರುವ fromp = vtxp->fromp();
        DfgVertex* ಧ್ರುವ lsbp = vtxp->lsbp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (DfgConst* ಧ್ರುವ lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->ಅಂಕೆ().toUInt()));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    ಶೂನ್ಯ visit(DfgAnd* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ಯದಿ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ಯದಿ (DfgNot* ಧ್ರುವ lhsNotp = lhsp->cast<DfgNot>()) {
                ಯದಿ (DfgNot* ಧ್ರುವ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* ಧ್ರುವ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
                ಯದಿ (DfgNeq* ಧ್ರುವ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* ಧ್ರುವ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ಧ್ರುವ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ lConstp = lhsp->cast<DfgConst>()) {
            ಯದಿ (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (DfgConcat* ಧ್ರುವ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (distributive_And_Associative_ದ್ವಿಮಾನ<DfgOr, DfgAnd>(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Through_Reductions(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Throug_Sel(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        {
            DfgNot* ಧ್ರುವ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ಧ್ರುವ rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            ಯದಿ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (DfgAnd* ಧ್ರುವ rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ಧ್ರುವ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ಧ್ರುವ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                ಯದಿ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (vtxp->dtype() == m_bitDType) {
            ಯದಿ (tryReplaceBitwiseWithReduction(vtxp)) ಪ್ರತ್ಯಾಗಮ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            ಯದಿ (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            ಯದಿ (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ಧ್ರುವ newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgOr* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ಯದಿ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ಯದಿ (DfgNot* ಧ್ರುವ lhsNotp = lhsp->cast<DfgNot>()) {
                ಯದಿ (DfgNot* ಧ್ರುವ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* ಧ್ರುವ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
                ಯದಿ (DfgNeq* ಧ್ರುವ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* ಧ್ರುವ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ಧ್ರುವ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (DfgConcat* ಧ್ರುವ lhsConcatp = lhsp->cast<DfgConcat>()) {
            ಯದಿ (DfgConcat* ಧ್ರುವ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಯದಿ (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    ಯದಿ (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                    ಯದಿ (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                }
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ lConstp = lhsp->cast<DfgConst>()) {
            ಯದಿ (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (DfgConcat* ಧ್ರುವ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (distributive_And_Associative_ದ್ವಿಮಾನ<DfgAnd, DfgOr>(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Through_Reductions(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Throug_Sel(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        {
            DfgNot* ಧ್ರುವ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ಧ್ರುವ rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            ಯದಿ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (DfgOr* ಧ್ರುವ rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ಧ್ರುವ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ಧ್ರುವ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                ಯದಿ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (vtxp->dtype() == m_bitDType) {
            ಯದಿ (tryReplaceBitwiseWithReduction(vtxp)) ಪ್ರತ್ಯಾಗಮ;
        }
    }

    ಶೂನ್ಯ visit(DfgXor* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ lConstp = lhsp->cast<DfgConst>()) {
            ಯದಿ (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (DfgConcat* ಧ್ರುವ rConcatp = rhsp->cast<DfgConcat>()) {
                ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Through_Reductions(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (try_ತಳ್ಳು_Bitwise_Op_Throug_Sel(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (vtxp->dtype() == m_bitDType) {
            ಯದಿ (tryReplaceBitwiseWithReduction(vtxp)) ಪ್ರತ್ಯಾಗಮ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    ಶೂನ್ಯ visit(DfgAdd* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಧ್ರುವ std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_ಎಣಿಕೆ_Ones_Term(lhsp);
        ಯದಿ (DfgVertex* ಧ್ರುವ lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_ಎಣಿಕೆ_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            ಯದಿ (!std::get<0>(rTerm)) {
                ಯದಿ (DfgAdd* ಧ್ರುವ rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_ಎಣಿಕೆ_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            ಯದಿ (DfgVertex* ಧ್ರುವ rVtxp = std::get<0>(rTerm)) {
                ಯದಿ (isSame(lVtxp, rVtxp)) {
                    ಧ್ರುವ uint32_t lLsb = std::get<1>(lTerm);
                    ಧ್ರುವ uint32_t rLsb = std::get<1>(rTerm);
                    ಧ್ರುವ uint32_t lWidth = std::get<2>(lTerm);
                    ಧ್ರುವ uint32_t rWidth = std::get<2>(rTerm);
                    ತಾರ್ಕಿಕ adjoined = ಸತ್ಯ;
                    uint32_t lsb = 0;
                    ಯದಿ (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } ಅನ್ಯಥಾ ಯದಿ (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } ಅನ್ಯಥಾ {
                        adjoined = ಅಸತ್ಯ;
                    }
                    ಯದಿ (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_ಎಣಿಕೆ_ONES) {
                            DfgSel* ಧ್ರುವ selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_ಎಣಿಕೆ_Ones>(flp, DfgDataType::packed(32), selp);
                            ಯದಿ (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } ಅನ್ಯಥಾ ಯದಿ (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            ಯದಿ (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgArraySel* ಧ್ರುವ vtxp) override {
        DfgConst* ಧ್ರುವ idxp = vtxp->bitp()->cast<DfgConst>();
        ಯದಿ (!idxp) ಪ್ರತ್ಯಾಗಮ;
        DfgVarArray* ಧ್ರುವ varp = vtxp->fromp()->cast<DfgVarArray>();
        ಯದಿ (!varp) ಪ್ರತ್ಯಾಗಮ;
        AstVar* ಧ್ರುವ astVarp = varp->vscp()->varp();
        ಯದಿ (astVarp->isForced()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (astVarp->isSigUserRWPublic()) ಪ್ರತ್ಯಾಗಮ;
        DfgVertex* ಧ್ರುವ srcp = varp->srcp();
        ಯದಿ (!srcp) {
            ಯದಿ (vtxp->isPacked()) {
                ಯದಿ (AstConsArray* ಧ್ರುವ iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    ಯದಿ (AstConst* ಧ್ರುವ valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(ನೂತನ DfgConst{m_dfg, valp->fileline(), valp->ಅಂಕೆ()});
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಯದಿ (DfgSpliceArray* ಧ್ರುವ splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* ಧ್ರುವ driverp = splicep->driverAt(idxp->toSizeT());
            ಯದಿ (!driverp) ಪ್ರತ್ಯಾಗಮ;
            DfgUnitArray* ಧ್ರುವ uap = driverp->cast<DfgUnitArray>();
            ಯದಿ (!uap) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (uap->srcp()->is<DfgVertexSplice>()) ಪ್ರತ್ಯಾಗಮ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ಯದಿ (DfgVertexVar* ಧ್ರುವ dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ಯದಿ (dvarp->srcp()) ಪ್ರತ್ಯಾಗಮ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (DfgUnitArray* ಧ್ರುವ uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            ಯದಿ (uap->srcp()->is<DfgSplicePacked>()) ಪ್ರತ್ಯಾಗಮ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ಯದಿ (DfgVertexVar* ಧ್ರುವ dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ಯದಿ (dvarp->srcp()) ಪ್ರತ್ಯಾಗಮ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgConcat* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();

        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isZero(lhsp)) {
            DfgConst* ಧ್ರುವ lConstp = lhsp->as<DfgConst>();
            ಯದಿ (DfgSel* ಧ್ರುವ rSelp = rhsp->cast<DfgSel>()) {
                ಯದಿ (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_ತುದಿ_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (isZero(rhsp)) {
            DfgConst* ಧ್ರುವ rConstp = rhsp->as<DfgConst>();
            ಯದಿ (DfgSel* ಧ್ರುವ lSelp = lhsp->cast<DfgSel>()) {
                ಯದಿ (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (DfgNot* ಧ್ರುವ lNot = lhsp->cast<DfgNot>()) {
            ಯದಿ (DfgNot* ಧ್ರುವ rNot = rhsp->cast<DfgNot>()) {
                ಯದಿ (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(ತಳ್ಳು_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ಧ್ರುವ newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        {
            ತಾರ್ಕಿಕ swap = ಅಸತ್ಯ;
            ಯದಿ (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(ತಳ್ಳು_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ಯದಿ (DfgConcat* ಧ್ರುವ rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* ಧ್ರುವ rlp = rConcatp->lhsp();
                ಯದಿ (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(ತಳ್ಳು_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* ಧ್ರುವ newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (DfgSel* ಧ್ರುವ lSelp = lhsp->cast<DfgSel>()) {
            ಯದಿ (DfgSel* ಧ್ರುವ rSelp = rhsp->cast<DfgSel>()) {
                ಯದಿ (isSame(lSelp->fromp(), rSelp->fromp())) {
                    ಯದಿ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                }
            }

            ಯದಿ (DfgConcat* ಧ್ರುವ rConcatp = rhsp->cast<DfgConcat>()) {
                ಯದಿ (DfgSel* ಧ್ರುವ rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    ಯದಿ (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        ಯದಿ (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                ಧ್ರುವ uint32_t width = lSelp->width() + rlSelp->width();
                                ಧ್ರುವ DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ಧ್ರುವ selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ಪ್ರತ್ಯಾಗಮ;
                            }
                        }
                    }
                }
            }
        }

        ಯದಿ (DfgSel* ಧ್ರುವ rSelp = rhsp->cast<DfgSel>()) {
            ಯದಿ (DfgConcat* ಧ್ರುವ lConcatp = lhsp->cast<DfgConcat>()) {
                ಯದಿ (DfgSel* ಧ್ರುವ lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    ಯದಿ (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        ಯದಿ (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                ಧ್ರುವ uint32_t width = lrSelp->width() + rSelp->width();
                                ಧ್ರುವ DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ಧ್ರುವ selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ಪ್ರತ್ಯಾಗಮ;
                            }
                        }
                    }
                }
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ lConstp = lhsp->cast<DfgConst>()) {
            ಯದಿ (DfgCond* ಧ್ರುವ rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* ಧ್ರುವ rtVtxp = rCondp->thenp();
                DfgVertex* ಧ್ರುವ reVtxp = rCondp->elsep();
                DfgConst* ಧ್ರುವ rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ಧ್ರುವ reConstp = reVtxp->cast<DfgConst>();
                ಯದಿ (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(ತಳ್ಳು_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* ಧ್ರುವ thenp = [&]() -> DfgVertex* {
                            File_ಸಾಲು* ಧ್ರುವ rtFlp = rtVtxp->fileline();
                            ಯದಿ (rtConstp) {
                                DfgConst* ಧ್ರುವ constp = makeZero(rtFlp, vtxp->width());
                                constp->ಅಂಕೆ().opConcat(lConstp->ಅಂಕೆ(), rtConstp->ಅಂಕೆ());
                                ಪ್ರತ್ಯಾಗಮ constp;
                            }
                            ಪ್ರತ್ಯಾಗಮ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* ಧ್ರುವ elsep = [&]() -> DfgVertex* {
                            File_ಸಾಲು* ಧ್ರುವ reFlp = reVtxp->fileline();
                            ಯದಿ (reConstp) {
                                DfgConst* ಧ್ರುವ constp = makeZero(reFlp, vtxp->width());
                                constp->ಅಂಕೆ().opConcat(lConstp->ಅಂಕೆ(), reConstp->ಅಂಕೆ());
                                ಪ್ರತ್ಯಾಗಮ constp;
                            }
                            ಪ್ರತ್ಯಾಗಮ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = rhsp->cast<DfgConst>()) {
            ಯದಿ (DfgCond* ಧ್ರುವ lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* ಧ್ರುವ ltVtxp = lCondp->thenp();
                DfgVertex* ಧ್ರುವ leVtxp = lCondp->elsep();
                DfgConst* ಧ್ರುವ ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ಧ್ರುವ leConstp = leVtxp->cast<DfgConst>();
                ಯದಿ (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(ತಳ್ಳು_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* ಧ್ರುವ thenp = [&]() -> DfgVertex* {
                            File_ಸಾಲು* ಧ್ರುವ ltFlp = ltVtxp->fileline();
                            ಯದಿ (ltConstp) {
                                DfgConst* ಧ್ರುವ constp = makeZero(ltFlp, vtxp->width());
                                constp->ಅಂಕೆ().opConcat(ltConstp->ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                                ಪ್ರತ್ಯಾಗಮ constp;
                            }
                            ಪ್ರತ್ಯಾಗಮ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* ಧ್ರುವ elsep = [&]() -> DfgVertex* {
                            File_ಸಾಲು* ಧ್ರುವ leFlp = leVtxp->fileline();
                            ಯದಿ (leConstp) {
                                DfgConst* ಧ್ರುವ constp = makeZero(leFlp, vtxp->width());
                                constp->ಅಂಕೆ().opConcat(leConstp->ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                                ಪ್ರತ್ಯಾಗಮ constp;
                            }
                            ಪ್ರತ್ಯಾಗಮ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ಧ್ರುವ uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            ಧ್ರುವ uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            ಧ್ರುವ uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            ತಾರ್ಕಿಕ hasCrossSink = ಅಸತ್ಯ;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                ಯದಿ (ಧ್ರುವ DfgSel* ಧ್ರುವ selp = sink.cast<DfgSel>()) {
                    ಧ್ರುವ uint32_t selLsb = selp->lsb();
                    ಧ್ರುವ uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                ಯದಿ (ಧ್ರುವ DfgVarPacked* ಧ್ರುವ varp = sink.cast<DfgVarPacked>()) {
                    ಯದಿ (!varp->hasSinks() && !varp->isObserved()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            });
            ಯದಿ (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    ಯದಿ (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    ಯದಿ (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* ಧ್ರುವ catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ಧ್ರುವ sp = ನೂತನ DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* ಧ್ರುವ varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(ಸತ್ಯ);
                    varp->srcp(sp);
                    replace(varp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        // Convert various forms to REPLICATE
        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        ಯದಿ (DfgRep* ಧ್ರುವ lRepp = lhsp->cast<DfgRep>()) {
            ಯದಿ (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
        ಯದಿ (DfgRep* ಧ್ರುವ rRepp = rhsp->cast<DfgRep>()) {
            ಯದಿ (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
        ಯದಿ (DfgRep* ಧ್ರುವ lRepp = lhsp->cast<DfgRep>()) {
            ಯದಿ (DfgRep* ಧ್ರುವ rRepp = rhsp->cast<DfgRep>()) {
                ಯದಿ (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ಯದಿ (DfgConcat* ಧ್ರುವ rConcatp = rhsp->cast<DfgConcat>()) {
                ಯದಿ (DfgRep* ಧ್ರುವ rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    ಯದಿ (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ಧ್ರುವ uint32_t width = lRepp->width() + rlRepp->width();
                            ಧ್ರುವ DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* ಧ್ರುವ repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgDiv* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಯದಿ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgDivS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಯದಿ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgGt* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgGtS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgGte* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgGteS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgLogAnd* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp) || vtxp->rhsp()->unsafe()) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();

        ಯದಿ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgLogEq* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgLogIf* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp) || vtxp->rhsp()->unsafe()) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgLogOr* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp) || vtxp->rhsp()->unsafe()) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();

        ಯದಿ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgLt* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgLtS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgLte* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgLteS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    ಶೂನ್ಯ visit(DfgModDiv* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgModDivS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgMul* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (DfgConst* ಧ್ರುವ lConstp = lhsp->cast<DfgConst>()) {
            ಯದಿ (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgMulS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (DfgConst* ಧ್ರುವ lConstp = lhsp->cast<DfgConst>()) {
            ಯದಿ (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgEq* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ lhsConstp = lhsp->cast<DfgConst>()) {
            ಯದಿ (DfgConcat* ಧ್ರುವ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಯದಿ (try_ತಳ್ಳು_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ಪ್ರತ್ಯಾಗಮ;
            }

            ಯದಿ (rhsp->dtype() == m_bitDType) {
                ಯದಿ (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }

                ಯದಿ (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgNeq* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ lhsConstp = lhsp->cast<DfgConst>()) {
            ಯದಿ (DfgConcat* ಧ್ರುವ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಯದಿ (try_ತಳ್ಳು_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ಪ್ರತ್ಯಾಗಮ;
            }

            ಯದಿ (rhsp->dtype() == m_bitDType) {
                ಯದಿ (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }

                ಯದಿ (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgPow* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgPowSS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgPowSU* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgPowUS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgRep* ಧ್ರುವ vtxp) override {
        DfgVertex* ಧ್ರುವ srcp = vtxp->srcp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (DfgConst* ಧ್ರುವ sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ಧ್ರುವ resultp = makeZero(flp, vtxp->width());
                resultp->ಅಂಕೆ().opRepl(sConstp->ಅಂಕೆ(), vtxp->ಎಣಿಕೆ());
                replace(resultp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (DfgRep* ಧ್ರುವ sRepp = srcp->cast<DfgRep>()) {
            ಯದಿ (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgShiftL* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (optimizeShiftRHS(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();

        ಯದಿ (DfgShiftL* ಧ್ರುವ lShiftLp = lhsp->cast<DfgShiftL>()) {
            ಯದಿ (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
                    ಧ್ರುವ DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ಧ್ರುವ bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* ಧ್ರುವ cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ಧ್ರುವ addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = rhsp->cast<DfgConst>()) {
            ಧ್ರುವ uint32_t shiftAmount = rConstp->toU32();

            ಯದಿ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (DfgConcat* ಧ್ರುವ lConcatp = lhsp->cast<DfgConcat>()) {
                ಯದಿ (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* ಧ್ರುವ lRhsp = lConcatp->rhsp();
                        DfgVertex* ಧ್ರುವ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ಧ್ರುವ uint32_t rW = shiftAmount;
                        ಧ್ರುವ uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        ಧ್ರುವ uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_ಸಾಲು* ಧ್ರುವ flp = lConcatp->fileline();
                        DfgVertex* ಧ್ರುವ rp = makeZero(flp, shiftAmount);
                        DfgVertex* ಧ್ರುವ mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        ಯದಿ (!lW) {
                            replace(np);
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                        DfgVertex* ಧ್ರುವ lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }

            ಯದಿ (DfgSel* ಧ್ರುವ lSelp = lhsp->cast<DfgSel>()) {
                ಯದಿ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ಧ್ರುವ uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ಧ್ರುವ nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }

            ಯದಿ (DfgCond* ಧ್ರುವ lCondp = lhsp->cast<DfgCond>()) {
                ಯದಿ (!lCondp->hasMultipleSinks()) {
                    APPLYING(ತಳ್ಳು_SHIFTL_THROUGH_COND) {
                        DfgShiftL* ಧ್ರುವ tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* ಧ್ರುವ ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgShiftR* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (optimizeShiftRHS(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();

        ಯದಿ (DfgShiftR* ಧ್ರುವ lShiftRp = lhsp->cast<DfgShiftR>()) {
            ಯದಿ (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();
                    ಧ್ರುವ DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ಧ್ರುವ bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* ಧ್ರುವ cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ಧ್ರುವ addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = rhsp->cast<DfgConst>()) {
            ಧ್ರುವ uint32_t shiftAmount = rConstp->toU32();

            ಯದಿ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (DfgConcat* ಧ್ರುವ lConcatp = lhsp->cast<DfgConcat>()) {
                ಯದಿ (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* ಧ್ರುವ lRhsp = lConcatp->rhsp();
                        DfgVertex* ಧ್ರುವ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ಧ್ರುವ uint32_t lW = shiftAmount;
                        ಧ್ರುವ uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        ಧ್ರುವ uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_ಸಾಲು* ಧ್ರುವ flp = lConcatp->fileline();
                        DfgVertex* ಧ್ರುವ lp = makeZero(flp, shiftAmount);
                        DfgVertex* ಧ್ರುವ mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        ಯದಿ (!rW) {
                            replace(np);
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                        DfgVertex* ಧ್ರುವ rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }

            ಯದಿ (DfgSel* ಧ್ರುವ lSelp = lhsp->cast<DfgSel>()) {
                ಯದಿ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ಧ್ರುವ uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ಧ್ರುವ nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }

            ಯದಿ (DfgCond* ಧ್ರುವ lCondp = lhsp->cast<DfgCond>()) {
                ಯದಿ (!lCondp->hasMultipleSinks()) {
                    APPLYING(ತಳ್ಳು_SHIFTR_THROUGH_COND) {
                        DfgShiftR* ಧ್ರುವ tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* ಧ್ರುವ ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgShiftRS* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (optimizeShiftRHS(vtxp)) ಪ್ರತ್ಯಾಗಮ;
    }

    ಶೂನ್ಯ visit(DfgSub* ಧ್ರುವ vtxp) override {
        ಯದಿ (ದ್ವಿಮಾನ(vtxp)) ಪ್ರತ್ಯಾಗಮ;

        DfgVertex* ಧ್ರುವ lhsp = vtxp->lhsp();
        DfgVertex* ಧ್ರುವ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (DfgConst* ಧ್ರುವ rConstp = rhsp->cast<DfgConst>()) {
            ಯದಿ (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (vtxp->dtype() == m_bitDType && rConstp->has_ಮೌಲ್ಯ(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgMatchMasked* ಧ್ರುವ vtxp) override {
        ಯದಿ (DfgConst* ಧ್ರುವ constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ಧ್ರುವ matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->ಅಂಕೆ(), matchVarp)));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    ಶೂನ್ಯ visit(DfgCond* ಧ್ರುವ vtxp) override {
        DfgVertex* ಧ್ರುವ condp = vtxp->condp();
        DfgVertex* ಧ್ರುವ thenp = vtxp->thenp();
        DfgVertex* ಧ್ರುವ elsep = vtxp->elsep();
        File_ಸಾಲು* ಧ್ರುವ flp = vtxp->fileline();

        ಯದಿ (condp->dtype() != m_bitDType) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (DfgNot* ಧ್ರುವ condNotp = condp->cast<DfgNot>()) {
            ಯದಿ (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        ಯದಿ (DfgNeq* ಧ್ರುವ condNeqp = condp->cast<DfgNeq>()) {
            ಯದಿ (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ಧ್ರುವ newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        ಯದಿ (DfgNot* ಧ್ರುವ thenNotp = thenp->cast<DfgNot>()) {
            ಯದಿ (DfgNot* ಧ್ರುವ elseNotp = elsep->cast<DfgNot>()) {
                ಯದಿ (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ಧ್ರುವ newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (DfgOr* ಧ್ರುವ condOrp = condp->cast<DfgOr>()) {
            ಯದಿ (DfgCond* ಧ್ರುವ thenCondp = thenp->cast<DfgCond>()) {
                ಯದಿ (!thenCondp->hasMultipleSinks()) {
                    ಯದಿ (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ಧ್ರುವ ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                    ಯದಿ (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ಧ್ರುವ ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ಪ್ರತ್ಯಾಗಮ;
                        }
                    }
                }
            }
        }

        ಯದಿ (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            ಯದಿ (DfgAdd* ಧ್ರುವ thenAddp = thenp->cast<DfgAdd>()) {
                ಯದಿ (DfgConst* ಧ್ರುವ constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    ಯದಿ (constp->has_ಮೌಲ್ಯ(1)) {
                        ಯದಿ (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ಧ್ರುವ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_ಸಾಲು* ಧ್ರುವ thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ಪ್ರತ್ಯಾಗಮ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            ಯದಿ (DfgSub* ಧ್ರುವ thenSubp = thenp->cast<DfgSub>()) {
                ಯದಿ (DfgConst* ಧ್ರುವ constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    ಯದಿ (constp->has_ಮೌಲ್ಯ(1)) {
                        ಯದಿ (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ಧ್ರುವ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_ಸಾಲು* ಧ್ರುವ thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ಪ್ರತ್ಯಾಗಮ;
                            }
                        }
                    }
                }
            }
        }

        ಯದಿ (vtxp->dtype() == m_bitDType) {
            ಯದಿ (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        ಯದಿ (vtxp->width() <= VL_QUADSIZE) {
            ಯದಿ (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* ಧ್ರುವ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* ಧ್ರುವ maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* ಧ್ರುವ maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* ಧ್ರುವ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (DfgOr* ಧ್ರುವ tOrp = thenp->cast<DfgOr>()) {
                ಯದಿ (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* ಧ್ರುವ maskp = replicate(vtxp, condp);
                        DfgAnd* ಧ್ರುವ andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
                ಯದಿ (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* ಧ್ರುವ maskp = replicate(vtxp, condp);
                        DfgAnd* ಧ್ರುವ andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }
        }

        ಯದಿ (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ಧ್ರುವ std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            ಯದಿ (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ಧ್ರುವ uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    ಯದಿ (rWidth != rhsp->width()) {
                        ಧ್ರುವ DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    ಯದಿ (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                    // Create new LHS
                    ಧ್ರುವ uint32_t lWidth = vtxp->width() - rWidth;
                    ಧ್ರುವ DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* ಧ್ರುವ lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* ಧ್ರುವ lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* ಧ್ರುವ lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಧ್ರುವ std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            ಯದಿ (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ಧ್ರುವ uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    ಯದಿ (lWidth != lhsp->width()) {
                        ಧ್ರುವ DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    ಯದಿ (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                    // Create new RHS
                    ಧ್ರುವ uint32_t rWidth = vtxp->width() - lWidth;
                    ಧ್ರುವ DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* ಧ್ರುವ rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* ಧ್ರುವ rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* ಧ್ರುವ rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        ಯದಿ (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                ಯದಿ (ಧ್ರುವ uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ಧ್ರುವ zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                ಯದಿ (ಧ್ರುವ uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ಧ್ರುವ zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (DfgCond* ಧ್ರುವ tCondp = thenp->cast<DfgCond>()) {
            ಯದಿ (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }

        ಯದಿ (DfgCond* ಧ್ರುವ eCondp = elsep->cast<DfgCond>()) {
            ಯದಿ (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
    }

    ಶೂನ್ಯ visit(DfgVertexVar* ಧ್ರುವ vtxp) override {
        ಯದಿ (vtxp->hasSinks()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (vtxp->isObserved()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (vtxp->defaultp()) ಪ್ರತ್ಯಾಗಮ;

        // If undriven, or driven from another var, it is completely redundant.
        ಯದಿ (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        ತಾರ್ಕಿಕ foundOne = ಅಸತ್ಯ;
        ಧ್ರುವ ತಾರ್ಕಿಕ keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> ತಾರ್ಕಿಕ {
            // Ignore non-observable variable sinks. These can be eliminated.
            ಯದಿ (ಧ್ರುವ DfgVertexVar* ಧ್ರುವ varp = sink.cast<DfgVertexVar>()) {
                ಯದಿ (!varp->hasSinks() && !varp->isObserved()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            // Keep if found more than one sink
            ಯದಿ (foundOne) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // Mark first sink found
            foundOne = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        });
        ಯದಿ (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        ಪ್ರತಿ (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ತಳ್ಳು_back(nullptr);
        m_iterList.ತಳ್ಳು_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        ಪ್ರತಿ (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        ಪ್ರತಿ (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        ಯಾವತ್ (ಸತ್ಯ) {
            // Process the work list - keep the placeholder at index 0
            ಯಾವತ್ (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.ತೆಗೆ_back();
                ಯದಿ (!m_vtxp) ಅನುವರ್ತ;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                ಯದಿ (DfgVertexVar* ಧ್ರುವ varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    ಅನುವರ್ತ;
                }

                // Ast references are not considered
                ಯದಿ (m_vtxp->is<DfgVertexAst>()) ಅನುವರ್ತ;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                ಯದಿ (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    ಯದಿ (DfgVertex* ಧ್ರುವ sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            ಅನುವರ್ತ;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            ಯದಿ (m_iterList.size() == 1) ಭಙ್ಗ;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                ಪ್ರತಿ (size_t hops = 1; hops <= 4; ++hops) {
                    ಪ್ರತಿ (size_t i = begin; i < end; ++i) {
                        DfgVertex* ಧ್ರುವ vtxp = m_iterList[i];
                        ಯದಿ (!vtxp) ಅನುವರ್ತ;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : m_iterList) {
                ಯದಿ (!vtxp) ಅನುವರ್ತ;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

ಶೂನ್ಯ V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    ಯದಿ (!v3Global.opt.fDfgPeephole()) ಪ್ರತ್ಯಾಗಮ;
    V3DfgPeephole::apply(dfg, ctx);
}
