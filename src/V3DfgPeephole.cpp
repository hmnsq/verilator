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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgCache.h"
#അന്തര്ഭാവ "V3DfgDataType.h"
#അന്തര്ഭാവ "V3DfgPasses.h"
#അന്തര്ഭാവ "V3DfgPeepholePatterns.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cctype>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
നാമാകാശ {

template <typename T_Reduction>
ഘടന ReductionToBitwiseImpl {};
template <> ഘടന ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> ഘടന ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> ഘടന ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
ഘടന BitwiseToReductionImpl {};
template <> ഘടന BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> ഘടന BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> ഘടന BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
ഘടന IsAssociative : പൊതു std::false_type {};
template <> ഘടന IsAssociative<DfgAnd> : പൊതു std::true_type {};
template <> ഘടന IsAssociative<DfgOr>  : പൊതു std::true_type {};
template <> ഘടന IsAssociative<DfgXor> : പൊതു std::true_type {};
template <> ഘടന IsAssociative<DfgAdd> : പൊതു std::true_type {};
template <> ഘടന IsAssociative<DfgConcat> : പൊതു std::true_type {};
template <> ഘടന IsAssociative<DfgMul> : പൊതു std::true_type {};
template <> ഘടന IsAssociative<DfgMulS> : പൊതു std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
ഘടന IsCommutative : പൊതു std::false_type {};
template <> ഘടന IsCommutative<DfgAnd> : പൊതു std::true_type {};
template <> ഘടന IsCommutative<DfgOr>  : പൊതു std::true_type {};
template <> ഘടന IsCommutative<DfgXor> : പൊതു std::true_type {};
template <> ഘടന IsCommutative<DfgAdd> : പൊതു std::true_type {};
template <> ഘടന IsCommutative<DfgEq> : പൊതു std::true_type {};
template <> ഘടന IsCommutative<DfgMul> : പൊതു std::true_type {};
template <> ഘടന IsCommutative<DfgMulS> : പൊതു std::true_type {};
template <> ഘടന IsCommutative<DfgNeq> : പൊതു std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
ഘടന IsIdempotent : പൊതു std::false_type {};
template <> ഘടന IsIdempotent<DfgAnd> : പൊതു std::true_type {};
template <> ഘടന IsIdempotent<DfgOr>  : പൊതു std::true_type {};

// Binary result dtype
template<typename Vertex>
സ്ഥിരം DfgDataType& resultDType(സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp);
template <> സ്ഥിരം DfgDataType& resultDType<DfgAdd>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgAnd>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgConcat>     (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgDiv>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgDivS>       (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgEq>         (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgGt>         (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgGtS>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgGte>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgGteS>       (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgLogAnd>     (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgLogEq>      (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgLogIf>      (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgLogOr>      (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgLt>         (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgLtS>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgLte>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgLteS>       (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgModDiv>     (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgModDivS>    (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgMul>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgMulS>       (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgNeq>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ DfgDataType::packed(1); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgOr>         (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgPow>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgPowSS>      (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgPowSU>      (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgPowUS>      (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgShiftL>     (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgShiftR>     (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgShiftRS>    (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgSub>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }
template <> സ്ഥിരം DfgDataType& resultDType<DfgXor>        (സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) { തിരികെ lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> ശൂന്യം foldOp(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src);
template <> ശൂന്യം foldOp<Dfg_എണ്ണം_Ones>  (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.op_എണ്ണം_Ones(src); }
template <> ശൂന്യം foldOp<DfgExtend>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opAssign(src); }
template <> ശൂന്യം foldOp<DfgExtendS>    (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opExtendS(src, src.width()); }
template <> ശൂന്യം foldOp<DfgLogNot>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opLogNot(src); }
template <> ശൂന്യം foldOp<DfgNegate>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opNegate(src); }
template <> ശൂന്യം foldOp<DfgNot>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opNot(src); }
template <> ശൂന്യം foldOp<DfgOneHot>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opOneHot(src); }
template <> ശൂന്യം foldOp<DfgOneHot0>    (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opOneHot0(src); }
template <> ശൂന്യം foldOp<DfgRedAnd>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opRedAnd(src); }
template <> ശൂന്യം foldOp<DfgRedOr>      (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opRedOr(src); }
template <> ശൂന്യം foldOp<DfgRedXor>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> ശൂന്യം foldOp(V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
template <> ശൂന്യം foldOp<DfgAdd>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opAdd(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgAnd>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opAnd(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgConcat>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opConcat(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgDiv>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opDiv(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgDivS>       (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opDivS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgEq>         (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opEq(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgGt>         (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opGt(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgGtS>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opGtS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgGte>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opGte(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgGteS>       (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opGteS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgLogAnd>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opLogAnd(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgLogEq>      (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opLogEq(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgLogIf>      (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opLogIf(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgLogOr>      (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opLogOr(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgLt>         (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opLt(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgLtS>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opLtS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgLte>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opLte(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgLteS>       (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opLteS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgModDiv>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opModDiv(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgModDivS>    (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opModDivS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgMul>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opMul(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgMulS>       (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opMulS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgNeq>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opNeq(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgOr>         (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opOr(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgPow>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opPow(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgPowSS>      (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opPowSS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgPowSU>      (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opPowSU(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgPowUS>      (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opPowUS(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgShiftL>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opShiftL(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgShiftR>     (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opShiftR(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgShiftRS>    (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> ശൂന്യം foldOp<DfgSub>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opSub(lhs, rhs); }
template <> ശൂന്യം foldOp<DfgXor>        (V3_സംഖ്യ& out, സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

വർഗ്ഗം V3DfgPeephole final : പൊതു DfgVisitor {
    // TYPES
    ഘടന VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        തർക്കം m_isCachedVertex = അസത്യം;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    സ്ഥിരം DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* സ്ഥിരം m_tmpScopep = v3Global.rootp()->മുകൾ_Scopep()->scopep();

    // STATIC STATE
    സ്ഥിതം V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) എങ്കിൽ (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    തർക്കം checkApplying(VDfgPeepholePattern id) {
        എങ്കിൽ (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) തിരികെ അസത്യം;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_എണ്ണം[id];
        തിരികെ സത്യം;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* സ്ഥിരം equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        തിരികെ equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    ശൂന്യം invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = അസത്യം;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_വരി* flp, സ്ഥിരം DfgDataType& dtype, Operands... operands) {
        Vertex* സ്ഥിരം vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = സത്യം;
        തിരികെ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(സ്ഥിരം DfgDataType& dtype, Operands... operands) {
        Vertex* സ്ഥിരം vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        തിരികെ vtxp;
    }

    ശൂന്യം incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    ശൂന്യം addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        എങ്കിൽ (vInfo.m_workListIndex) തിരികെ;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.തള്ളുക_back(vtxp);
    }

    ശൂന്യം removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    ശൂന്യം addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        എങ്കിൽ (vInfo.m_iterListIndex) തിരികെ;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.തള്ളുക_back(vtxp);
    }

    ശൂന്യം removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    ശൂന്യം deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = vtxp->cast<DfgVertexVar>();
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
            എങ്കിൽ (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                തിരികെ അസത്യം;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            എങ്കിൽ (vInfo.m_generation != m_currentGeneration) srcps.തള്ളുക_back(&src);
            vInfo.m_generation = m_currentGeneration;
            തിരികെ അസത്യം;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        എങ്കിൽ (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.തള്ളുക_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } അല്ലെങ്കിൽ {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        സ്ഥിരം auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  തിരികെ srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        വേണ്ടി (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        വേണ്ടി (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    ശൂന്യം replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        സ്ഥിരം auto debugCallback = [&]() -> ശൂന്യം {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            സ്ഥിരം auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](സ്ഥിരം DfgVertex& v) {  //
                തിരികെ cone->എണ്ണം(&v);
            });
        };
        എങ്കിൽ (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) തിരികെ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            തിരികെ അസത്യം;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            തിരികെ അസത്യം;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            തിരികെ അസത്യം;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            തിരികെ അസത്യം;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_വരി* flp, uint32_t val) { തിരികെ പുതിയ DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_വരി* flp, uint32_t width) {
        തിരികെ പുതിയ DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_വരി* flp, സ്ഥിരം DfgDataType& dtype) {
        തിരികെ makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_വരി* flp, uint32_t width) {
        DfgConst* സ്ഥിരം resp = makeZero(flp, width);
        resp->അക്കം().setAllBits1();
        തിരികെ resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_വരി* flp, സ്ഥിരം DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* സ്ഥിരം vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        എങ്കിൽ (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        എങ്കിൽ (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        തിരികെ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(സ്ഥിരം DfgVertex* examplep, Operands... operands) {
        തിരികെ make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        എങ്കിൽ (vtxp->dtype() == m_bitDType) തിരികെ bitp;
        തിരികെ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    സ്ഥിതം തർക്കം isSame(സ്ഥിരം DfgVertex* ap, സ്ഥിരം DfgVertex* bp) {
        എങ്കിൽ (ap == bp) തിരികെ സത്യം;
        എങ്കിൽ (ap->dtype() != bp->dtype()) തിരികെ അസത്യം;
        സ്ഥിരം DfgConst* സ്ഥിരം aConstp = ap->cast<DfgConst>();
        എങ്കിൽ (!aConstp) തിരികെ അസത്യം;
        സ്ഥിരം DfgConst* സ്ഥിരം bConstp = bp->cast<DfgConst>();
        എങ്കിൽ (!bConstp) തിരികെ അസത്യം;
        തിരികെ aConstp->അക്കം().isCaseEq(bConstp->അക്കം());
    }

    സ്ഥിതം തർക്കം isZero(സ്ഥിരം DfgVertex* vtxp) {
        എങ്കിൽ (സ്ഥിരം DfgConst* സ്ഥിരം constp = vtxp->cast<DfgConst>()) തിരികെ constp->isZero();
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം isOnes(സ്ഥിരം DfgVertex* vtxp) {
        എങ്കിൽ (സ്ഥിരം DfgConst* സ്ഥിരം constp = vtxp->cast<DfgConst>()) തിരികെ constp->isOnes();
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം isEqOne(സ്ഥിരം DfgVertex* vtxp) {
        എങ്കിൽ (സ്ഥിരം DfgConst* സ്ഥിരം constp = vtxp->cast<DfgConst>()) തിരികെ constp->അക്കം().isEqOne();
        തിരികെ അസത്യം;
    }

    സ്ഥിതം തർക്കം areAdjacent(uint32_t& lsb, സ്ഥിരം DfgSel* lSelp, സ്ഥിരം DfgSel* rSelp) {
        എങ്കിൽ (!isSame(lSelp->srcp(), rSelp->srcp())) തിരികെ അസത്യം;
        എങ്കിൽ (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            തിരികെ സത്യം;
        }
        എങ്കിൽ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം foldUnary(Vertex* സ്ഥിരം vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::മൂല്യം, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::മൂല്യം, "Must invoke on final class");
        എങ്കിൽ (DfgConst* സ്ഥിരം srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* സ്ഥിരം resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->അക്കം(), srcp->അക്കം());
                replace(resultp);
                തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം ദ്വിമാന(Vertex* സ്ഥിരം vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ദ്വിമാന, Vertex>::മൂല്യം, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::മൂല്യം, "Must invoke on final class");

        File_വരി* സ്ഥിരം flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* സ്ഥിരം lConstp = lhsp->cast<DfgConst>();
        DfgConst* സ്ഥിരം rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* സ്ഥിരം lSamep = lhsp->cast<Vertex>();
        Vertex* സ്ഥിരം rSamep = rhsp->cast<Vertex>();

        // Constant folding
        എങ്കിൽ (lConstp && rConstp) {
            APPLYING(FOLD_ദ്വിമാന) {
                DfgConst* സ്ഥിരം resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->അക്കം(), lConstp->അക്കം(), rConstp->അക്കം());
                replace(resultp);
                തിരികെ സത്യം;
            }
        }

        // Idempotent
        എങ്കിൽ VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::മൂല്യം) {
            എങ്കിൽ (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ദ്വിമാന) {
                    replace(lhsp);
                    തിരികെ സത്യം;
                }
            }
        }

        // Associative
        എങ്കിൽ VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::മൂല്യം) {
            // Also commutative and idempotent
            എങ്കിൽ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::മൂല്യം && IsIdempotent<Vertex>::മൂല്യം) {
                എങ്കിൽ (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ദ്വിമാന_LHS) {
                        replace(lSamep);
                        തിരികെ സത്യം;
                    }
                }
                എങ്കിൽ (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ദ്വിമാന_RHS) {
                        replace(rSamep);
                        തിരികെ സത്യം;
                    }
                }
            }

            എങ്കിൽ (lConstp && rSamep) {
                എങ്കിൽ (DfgConst* സ്ഥിരം rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ദ്വിമാന_LHS_OF_RHS) {
                        DfgConst* സ്ഥിരം cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->അക്കം(), lConstp->അക്കം(), rlConstp->അക്കം());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        തിരികെ സത്യം;
                    }
                }
            }

            എങ്കിൽ (lSamep && rConstp) {
                എങ്കിൽ (DfgConst* സ്ഥിരം lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ദ്വിമാന_RHS_OF_LHS) {
                        DfgConst* സ്ഥിരം cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->അക്കം(), lrConstp->അക്കം(), rConstp->അക്കം());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        തിരികെ സത്യം;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            എങ്കിൽ (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* സ്ഥിരം ap = lSamep->lhsp();
                DfgVertex* സ്ഥിരം bp = lSamep->rhsp();
                DfgVertex* സ്ഥിരം cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                എങ്കിൽ (ap != cp && bp != cp) {
                    APPLYING(വലത്_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        സ്ഥിരം DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* സ്ഥിരം bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        തിരികെ സത്യം;
                    }
                }
            }

            എങ്കിൽ (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* സ്ഥിരം rlVtxp = rSamep->lhsp();
                DfgVertex* സ്ഥിരം rrVtxp = rSamep->rhsp();

                എങ്കിൽ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::മൂല്യം) {
                    എങ്കിൽ (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            തിരികെ സത്യം;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                എങ്കിൽ (Vertex* സ്ഥിരം existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    എങ്കിൽ (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ദ്വിമാന_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            തിരികെ സത്യം;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                എങ്കിൽ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::മൂല്യം) {
                    എങ്കിൽ (Vertex* സ്ഥിരം existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        എങ്കിൽ (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ദ്വിമാന_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                തിരികെ സത്യം;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        എങ്കിൽ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::മൂല്യം) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            എങ്കിൽ (lConstp) {
                എങ്കിൽ (DfgCond* സ്ഥിരം rCondp = rhsp->cast<DfgCond>()) {
                    എങ്കിൽ (!rCondp->hasMultipleSinks()) {
                        APPLYING(തള്ളുക_COMMUTATIVE_ദ്വിമാന_THROUGH_COND) {
                            DfgVertex* സ്ഥിരം tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* സ്ഥിരം ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            തിരികെ സത്യം;
                        }
                    }
                }
                തിരികെ അസത്യം;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        സ്ഥിരം തർക്കം swapSides = [&]() {
            // Don't swap if not commutative, duh
            എങ്കിൽ VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::മൂല്യം) തിരികെ അസത്യം;
            // Const go on left
            എങ്കിൽ (lConstp) തിരികെ അസത്യം;
            എങ്കിൽ (rConstp) തിരികെ സത്യം;
            // Not go on left
            എങ്കിൽ (lhsp->is<DfgNot>()) തിരികെ അസത്യം;
            എങ്കിൽ (rhsp->is<DfgNot>()) തിരികെ സത്യം;
            // Same go on right
            എങ്കിൽ (rSamep) തിരികെ അസത്യം;
            എങ്കിൽ (lSamep) തിരികെ സത്യം;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            സ്ഥിരം VertexInfo& lInfo = m_vInfo[lhsp];
            സ്ഥിരം VertexInfo& rInfo = m_vInfo[rhsp];
            തിരികെ lInfo.m_id > rInfo.m_id;
        }();
        എങ്കിൽ (swapSides) {
            APPLYING(SWAP_SIDES_IN_ദ്വിമാന) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                തിരികെ സത്യം;
            }
        }

        തിരികെ അസത്യം;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം distributive_And_Associative_ദ്വിമാന(Vertex* vtxp) {
        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        എങ്കിൽ (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            എങ്കിൽ (Other* സ്ഥിരം lp = lhsp->cast<Other>()) {
                എങ്കിൽ (Other* സ്ഥിരം rp = rhsp->cast<Other>()) {
                    DfgVertex* സ്ഥിരം llp = lp->lhsp();
                    DfgVertex* സ്ഥിരം lrp = lp->rhsp();
                    DfgVertex* സ്ഥിരം rlp = rp->lhsp();
                    DfgVertex* സ്ഥിരം rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    എങ്കിൽ (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } അല്ലെങ്കിൽ എങ്കിൽ (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } അല്ലെങ്കിൽ എങ്കിൽ (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } അല്ലെങ്കിൽ എങ്കിൽ (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    എങ്കിൽ (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_ദ്വിമാന) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            തിരികെ സത്യം;
                        }
                    }
                }
            }
        }

        തിരികെ അസത്യം;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം
    try_തള്ളുക_Bitwise_Op_Through_Concat(Vertex* സ്ഥിരം vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* സ്ഥിരം catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* സ്ഥിരം catRConstp = concatp->rhsp()->cast<DfgConst>();
        എങ്കിൽ (catLConstp || catRConstp) {
            APPLYING(തള്ളുക_BITWISE_OP_THROUGH_CONCAT) {
                സ്ഥിരം uint32_t width = concatp->width();
                സ്ഥിരം DfgDataType& lDtype = concatp->lhsp()->dtype();
                സ്ഥിരം DfgDataType& rDtype = concatp->rhsp()->dtype();
                സ്ഥിരം uint32_t lWidth = lDtype.size();
                സ്ഥിരം uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* സ്ഥിരം newLhsp = [&]() -> DfgVertex* {
                    DfgConst* സ്ഥിരം newLhsConstp = makeZero(constp->fileline(), lWidth);
                    എങ്കിൽ (catLConstp) {
                        V3_സംഖ്യ അക്കം{constp->fileline(), static_cast<പൂർണ്ണസംഖ്യ>(lWidth), 0u};
                        അക്കം.opSel(constp->അക്കം(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->അക്കം(), അക്കം, catLConstp->അക്കം());
                        തിരികെ newLhsConstp;
                    }
                    newLhsConstp->അക്കം().opSel(constp->അക്കം(), width - 1, rWidth);
                    തിരികെ make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* സ്ഥിരം newRhsp = [&]() -> DfgVertex* {
                    DfgConst* സ്ഥിരം newRhsConstp = makeZero(constp->fileline(), rWidth);
                    എങ്കിൽ (catRConstp) {
                        V3_സംഖ്യ അക്കം{constp->fileline(), static_cast<പൂർണ്ണസംഖ്യ>(rWidth), 0u};
                        അക്കം.opSel(constp->അക്കം(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->അക്കം(), അക്കം, catRConstp->അക്കം());
                        തിരികെ newRhsConstp;
                    }
                    newRhsConstp->അക്കം().opSel(constp->അക്കം(), rWidth - 1, 0);
                    തിരികെ make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം
    try_തള്ളുക_Compare_Op_Through_Concat(Vertex* സ്ഥിരം vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        എങ്കിൽ (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(തള്ളുക_COMPARE_OP_THROUGH_CONCAT) {
                സ്ഥിരം uint32_t width = concatp->width();
                സ്ഥിരം uint32_t lWidth = concatp->lhsp()->width();
                സ്ഥിരം uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* സ്ഥിരം newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->അക്കം().opSel(constp->അക്കം(), width - 1, rWidth);
                Vertex* സ്ഥിരം newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* സ്ഥിരം newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->അക്കം().opSel(constp->അക്കം(), rWidth - 1, 0);
                Vertex* സ്ഥിരം newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                എങ്കിൽ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::മൂല്യം) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } അല്ലെങ്കിൽ എങ്കിൽ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::മൂല്യം) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } അല്ലെങ്കിൽ {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം try_തള്ളുക_Bitwise_Op_Through_Reductions(Bitwise* സ്ഥിരം vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        എങ്കിൽ (Reduction* സ്ഥിരം lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            എങ്കിൽ (Reduction* സ്ഥിരം rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* സ്ഥിരം lSrcp = lRedp->srcp();
                DfgVertex* സ്ഥിരം rSrcp = rRedp->srcp();
                എങ്കിൽ (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(തള്ളുക_BITWISE_THROUGH_REDUCTION) {
                        File_വരി* സ്ഥിരം flp = vtxp->fileline();
                        Bitwise* സ്ഥിരം bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        തിരികെ സത്യം;
                    }
                }
            }
        }

        തിരികെ അസത്യം;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    സ്ഥിതം std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        എങ്കിൽ (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            തിരികെ {nullptr, nullptr};
        }
        എങ്കിൽ (vtxp->hasMultipleSinks()) തിരികെ {nullptr, nullptr};
        Dfg_Vertex_ദ്വിമാന* സ്ഥിരം bwp = vtxp->as<Dfg_Vertex_ദ്വിമാന>();
        DfgSel* സ്ഥിരം lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* സ്ഥിരം rSelp = bwp->rhsp()->cast<DfgSel>();
        എങ്കിൽ (!lSelp || !rSelp) തിരികെ {nullptr, nullptr};
        DfgSel* സ്ഥിരം nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* സ്ഥിരം wSelp = nSelp == lSelp ? rSelp : lSelp;
        സ്ഥിരം uint32_t nWidth = nSelp->fromp()->width();
        എങ്കിൽ (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) തിരികെ {nullptr, nullptr};
        എങ്കിൽ (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            തിരികെ {nullptr, nullptr};
        }
        തിരികെ {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം try_തള്ളുക_Bitwise_Op_Throug_Sel(Bitwise* സ്ഥിരം vtxp) {
        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();

        എങ്കിൽ (DfgSel* സ്ഥിരം lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            എങ്കിൽ (DfgSel* സ്ഥിരം selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } അല്ലെങ്കിൽ എങ്കിൽ (Bitwise* സ്ഥിരം bitwisep = rhsp->cast<Bitwise>()) {
                എങ്കിൽ (DfgSel* സ്ഥിരം rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } അല്ലെങ്കിൽ എങ്കിൽ (DfgSel* സ്ഥിരം rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            എങ്കിൽ (rSelp) {
                DfgVertex* സ്ഥിരം lFromp = lSelp->fromp();
                DfgVertex* സ്ഥിരം rFromp = rSelp->fromp();
                എങ്കിൽ (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(തള്ളുക_BITWISE_THROUGH_SEL) {
                        Bitwise* സ്ഥിരം bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        എങ്കിൽ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        തിരികെ സത്യം;
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
            Bitwise* സ്ഥിരം rChainp = rhsp->cast<Bitwise>();
            എങ്കിൽ (rChainp && rChainp->hasMultipleSinks()) തിരികെ അസത്യം;
            DfgVertex* സ്ഥിരം rTermp = rChainp ? rChainp->lhsp() : rhsp;

            സ്ഥിരം auto lSels = pushableUnalignedSels(lhsp);
            സ്ഥിരം auto rSels = pushableUnalignedSels(rTermp);
            എങ്കിൽ (!lSels.first && !rSels.first) തിരികെ അസത്യം;

            സ്ഥിരം DfgVertex* സ്ഥിരം opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            എങ്കിൽ (opp->is<Bitwise>()) തിരികെ അസത്യം;
            സ്ഥിരം auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* സ്ഥിരം nFromp = sels.first->fromp();
            DfgVertex* സ്ഥിരം wFromp = sels.second->fromp();
            സ്ഥിരം uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            സ്ഥിരം auto matches = [&](സ്ഥിരം DfgVertex* termp, സ്ഥിരം auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                എങ്കിൽ (tSels.first) {
                    തിരികെ termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                എങ്കിൽ (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                എങ്കിൽ (സ്ഥിരം DfgSel* സ്ഥിരം selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                എങ്കിൽ (termp->type() != opp->type()) തിരികെ അസത്യം;
                // Operands might have been swapped since
                സ്ഥിരം Dfg_Vertex_ദ്വിമാന* സ്ഥിരം bwp = termp->as<Dfg_Vertex_ദ്വിമാന>();
                സ്ഥിരം തർക്കം nIsLhs = bwp->lhsp() == nFromp;
                എങ്കിൽ (!nIsLhs && bwp->rhsp() != nFromp) തിരികെ അസത്യം;
                സ്ഥിരം DfgSel* സ്ഥിരം slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                തിരികെ slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            എങ്കിൽ (!matches(lhsp, lSels) || !matches(rTermp, rSels)) തിരികെ അസത്യം;

            APPLYING(തള്ളുക_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_വരി* സ്ഥിരം flp = vtxp->fileline();
                സ്ഥിരം DfgDataType& dtype = nFromp->dtype();
                DfgSel* സ്ഥിരം slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                എങ്കിൽ (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } അല്ലെങ്കിൽ എങ്കിൽ (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } അല്ലെങ്കിൽ {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                സ്ഥിരം auto തള്ളുക = [&](DfgVertex* termp, സ്ഥിരം DfgSel* nSelp) -> DfgVertex* {
                    തിരികെ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* സ്ഥിരം newLhsp = തള്ളുക(lhsp, lSels.first);
                DfgVertex* സ്ഥിരം newRTermp = തള്ളുക(rTermp, rSels.first);
                എങ്കിൽ (rChainp) {
                    DfgVertex* സ്ഥിരം newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } അല്ലെങ്കിൽ {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                തിരികെ സത്യം;
            }
        }

        തിരികെ അസത്യം;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    സ്ഥിതം തർക്കം isConcatFoldable(സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        എങ്കിൽ (isSame(lhsp, rhsp)) തിരികെ സത്യം;
        എങ്കിൽ (സ്ഥിരം DfgRep* സ്ഥിരം lRepp = lhsp->cast<DfgRep>()) {
            എങ്കിൽ (isSame(lRepp->srcp(), rhsp)) തിരികെ സത്യം;
        }
        എങ്കിൽ (സ്ഥിരം DfgRep* സ്ഥിരം rRepp = rhsp->cast<DfgRep>()) {
            എങ്കിൽ (isSame(lhsp, rRepp->srcp())) തിരികെ സത്യം;
        }
        // Concatenation of adjoining selects is a select
        എങ്കിൽ (സ്ഥിരം DfgSel* സ്ഥിരം lSelp = lhsp->cast<DfgSel>()) {
            എങ്കിൽ (സ്ഥിരം DfgSel* സ്ഥിരം rSelp = rhsp->cast<DfgSel>()) {
                തിരികെ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        എങ്കിൽ (സ്ഥിരം DfgNot* സ്ഥിരം lNotp = lhsp->cast<DfgNot>()) {
            എങ്കിൽ (സ്ഥിരം DfgNot* സ്ഥിരം rNotp = rhsp->cast<DfgNot>()) {
                തിരികെ !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        തിരികെ അസത്യം;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    സ്ഥിതം തർക്കം isSameBitwise(സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) {
        എങ്കിൽ (lhsp->type() != rhsp->type()) തിരികെ അസത്യം;
        തിരികെ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    സ്ഥിതം തർക്കം isConcatSimplifiable(സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp) {
        എങ്കിൽ (isConcatFoldable(lhsp, rhsp)) തിരികെ സത്യം;
        എങ്കിൽ (!isSameBitwise(lhsp, rhsp)) തിരികെ അസത്യം;
        എങ്കിൽ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) തിരികെ അസത്യം;
        സ്ഥിരം Dfg_Vertex_ദ്വിമാന* സ്ഥിരം lp = lhsp->as<Dfg_Vertex_ദ്വിമാന>();
        സ്ഥിരം Dfg_Vertex_ദ്വിമാന* സ്ഥിരം rp = rhsp->as<Dfg_Vertex_ദ്വിമാന>();
        എങ്കിൽ (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            തിരികെ സത്യം;
        എങ്കിൽ (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            തിരികെ സത്യം;
        തിരികെ അസത്യം;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    സ്ഥിതം തർക്കം isConcatPushableThroughBitwise(സ്ഥിരം DfgVertex* lhsp, സ്ഥിരം DfgVertex* rhsp,
                                               തർക്കം& swap) {
        എങ്കിൽ (!isSameBitwise(lhsp, rhsp)) തിരികെ അസത്യം;
        എങ്കിൽ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) തിരികെ അസത്യം;
        സ്ഥിരം Dfg_Vertex_ദ്വിമാന* സ്ഥിരം lp = lhsp->as<Dfg_Vertex_ദ്വിമാന>();
        സ്ഥിരം Dfg_Vertex_ദ്വിമാന* സ്ഥിരം rp = rhsp->as<Dfg_Vertex_ദ്വിമാന>();
        swap = അസത്യം;
        എങ്കിൽ (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) തിരികെ സത്യം;
        എങ്കിൽ (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) തിരികെ സത്യം;
        swap = സത്യം;
        എങ്കിൽ (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) തിരികെ സത്യം;
        എങ്കിൽ (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) തിരികെ സത്യം;
        തിരികെ അസത്യം;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_വരി* flp, സ്ഥിരം DfgVertex* lhsp,
                                              സ്ഥിരം DfgVertex* rhsp, തർക്കം swap) {
        സ്ഥിരം Dfg_Vertex_ദ്വിമാന* സ്ഥിരം lp = lhsp->as<Dfg_Vertex_ദ്വിമാന>();
        സ്ഥിരം Dfg_Vertex_ദ്വിമാന* സ്ഥിരം rp = rhsp->as<Dfg_Vertex_ദ്വിമാന>();
        സ്ഥിരം DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* സ്ഥിരം cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* സ്ഥിരം dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* സ്ഥിരം newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* സ്ഥിരം newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        എങ്കിൽ (lp->is<DfgAnd>()) തിരികെ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        എങ്കിൽ (lp->is<DfgOr>()) തിരികെ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        തിരികെ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();

        എങ്കിൽ (DfgSel* സ്ഥിരം lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            എങ്കിൽ (DfgSel* സ്ഥിരം selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } അല്ലെങ്കിൽ എങ്കിൽ (Bitwise* സ്ഥിരം rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } അല്ലെങ്കിൽ എങ്കിൽ (Reduction* സ്ഥിരം rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            എങ്കിൽ (rSelp) {
                uint32_t lsb = 0;
                എങ്കിൽ (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        സ്ഥിരം DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* സ്ഥിരം newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        എങ്കിൽ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        തിരികെ സത്യം;
                    }
                }
            }
        }

        എങ്കിൽ (Reduction* സ്ഥിരം lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            എങ്കിൽ (!rRedp) {
                എങ്കിൽ (Bitwise* സ്ഥിരം rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            എങ്കിൽ (rRedp) {
                എങ്കിൽ (DfgSel* സ്ഥിരം lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    എങ്കിൽ (DfgSel* സ്ഥിരം rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        എങ്കിൽ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                സ്ഥിരം DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* സ്ഥിരം newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                എങ്കിൽ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                തിരികെ സത്യം;
                            }
                        }
                    }
                }
            }
        }

        തിരികെ അസത്യം;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം optimizeReduction(Reduction* സ്ഥിരം vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        എങ്കിൽ (foldUnary(vtxp)) തിരികെ സത്യം;

        DfgVertex* സ്ഥിരം srcp = vtxp->srcp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        // Reduction of 1-bit value
        എങ്കിൽ (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                തിരികെ സത്യം;
            }
        }

        എങ്കിൽ (DfgCond* സ്ഥിരം condp = srcp->cast<DfgCond>()) {
            എങ്കിൽ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(തള്ളുക_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* സ്ഥിരം newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* സ്ഥിരം newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* സ്ഥിരം newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    തിരികെ സത്യം;
                }
            }
        }

        എങ്കിൽ (DfgConcat* സ്ഥിരം concatp = srcp->cast<DfgConcat>()) {
            എങ്കിൽ (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(തള്ളുക_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* സ്ഥിരം lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* സ്ഥിരം rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    തിരികെ സത്യം;
                }
            }
        }

        എങ്കിൽ (Bitwise* സ്ഥിരം bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            എങ്കിൽ (!bitwisep->hasMultipleSinks()) {
                എങ്കിൽ (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(തള്ളുക_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* സ്ഥിരം newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* സ്ഥിരം newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        തിരികെ സത്യം;
                    }
                }

                എങ്കിൽ (DfgSel* സ്ഥിരം lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    എങ്കിൽ (DfgSel* സ്ഥിരം rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        എങ്കിൽ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(തള്ളുക_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                സ്ഥിരം DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* സ്ഥിരം newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                തിരികെ സത്യം;
                            }
                        }
                    }
                }
            }
        }

        തിരികെ അസത്യം;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT തർക്കം optimizeShiftRHS(Shift* സ്ഥിരം vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_ദ്വിമാന, Shift>::മൂല്യം, "Must invoke on binary");
        static_assert(std::is_final<Shift>::മൂല്യം, "Must invoke on final class");
        എങ്കിൽ (സ്ഥിരം DfgConcat* സ്ഥിരം concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            എങ്കിൽ (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    തിരികെ സത്യം;
                }
            }
        }
        തിരികെ അസത്യം;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_എണ്ണം_Ones_Term(DfgVertex* vtxp) {
        എങ്കിൽ (DfgConcat* സ്ഥിരം oCatp = vtxp->cast<DfgConcat>()) {
            എങ്കിൽ (isZero(oCatp->lhsp())) {
                എങ്കിൽ (Dfg_എണ്ണം_Ones* സ്ഥിരം എണ്ണം_Onesp = oCatp->rhsp()->cast<Dfg_എണ്ണം_Ones>()) {
                    // Zero extended count ones
                    എങ്കിൽ (DfgSel* സ്ഥിരം selp = എണ്ണം_Onesp->srcp()->cast<DfgSel>()) {
                        തിരികെ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (DfgSel* സ്ഥിരം selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    എങ്കിൽ (selp->dtype() == m_bitDType) {  //
                        തിരികെ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            തിരികെ {nullptr, 0, 0};
        }
        എങ്കിൽ (Dfg_എണ്ണം_Ones* സ്ഥിരം എണ്ണം_Onesp = vtxp->cast<Dfg_എണ്ണം_Ones>()) {
            // Simple count ones
            എങ്കിൽ (DfgSel* സ്ഥിരം selp = എണ്ണം_Onesp->srcp()->cast<DfgSel>()) {
                തിരികെ {selp->fromp(), selp->lsb(), selp->width()};
            }
            തിരികെ {nullptr, 0, 0};
        }
        എങ്കിൽ (DfgSel* സ്ഥിരം oSelp = vtxp->cast<DfgSel>()) {
            എങ്കിൽ (oSelp->lsb() == 0) {
                // Truncated count ones
                എങ്കിൽ (Dfg_എണ്ണം_Ones* സ്ഥിരം എണ്ണം_Onesp = oSelp->fromp()->cast<Dfg_എണ്ണം_Ones>()) {
                    // Zero extended count ones
                    എങ്കിൽ (DfgSel* സ്ഥിരം selp = എണ്ണം_Onesp->srcp()->cast<DfgSel>()) {
                        തിരികെ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            എങ്കിൽ (oSelp->dtype() == m_bitDType) {  //
                തിരികെ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            തിരികെ {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        എങ്കിൽ (DfgShiftR* സ്ഥിരം shiftrp = vtxp->cast<DfgShiftR>()) {
            എങ്കിൽ (DfgConst* സ്ഥിരം rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                എങ്കിൽ (rConstp->toU32() == shiftrp->width() - 1) {
                    തിരികെ {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            തിരികെ {nullptr, 0, 0};
        }
        // Not applicable
        തിരികെ {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              സ്ഥിരം uint32_t width) {
        ആകുമ്പോൾ (സത്യം) {
            സ്ഥിരം uint32_t msb = lsb + width - 1;

            // Sel from Concat
            എങ്കിൽ (DfgConcat* സ്ഥിരം concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* സ്ഥിരം lhsp = concatp->lhsp();
                DfgVertex* സ്ഥിരം rhsp = concatp->rhsp();

                എങ്കിൽ (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        തുടരുക;
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        തുടരുക;
                    }
                }
            }

            എങ്കിൽ (DfgRep* സ്ഥിരം repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                സ്ഥിരം uint32_t srcWidth = repp->srcp()->width();
                എങ്കിൽ (width <= srcWidth) {
                    സ്ഥിരം uint32_t newLsb = lsb % srcWidth;
                    സ്ഥിരം uint32_t newMsb = newLsb + width - 1;
                    എങ്കിൽ (newMsb < srcWidth) {
                        APPLYING(തള്ളുക_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            തുടരുക;
                        }
                    }
                }
            }

            // Sel from Sel
            എങ്കിൽ (DfgSel* സ്ഥിരം selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    തുടരുക;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            എങ്കിൽ (DfgVarPacked* സ്ഥിരം varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                സ്ഥിരം auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* സ്ഥിരം driverp = pair.first;
                സ്ഥിരം uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                എങ്കിൽ (driverp) {
                    എങ്കിൽ (driverp == varp->defaultp()) {
                        APPLYING(തള്ളുക_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            തുടരുക;
                        }
                    } അല്ലെങ്കിൽ {
                        APPLYING(തള്ളുക_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            തുടരുക;
                        }
                    }
                }
            }

            // No patterns matched, stop
            നിർത്തുക;
        }
        തിരികെ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        എങ്കിൽ (ap == bp) തിരികെ {ap, ap->width()};

        // If both constants, check LSBs
        എങ്കിൽ (DfgConst* സ്ഥിരം aConstp = ap->cast<DfgConst>()) {
            എങ്കിൽ (DfgConst* സ്ഥിരം bConstp = bp->cast<DfgConst>()) {
                സ്ഥിരം V3_സംഖ്യ& a_അക്കം = aConstp->അക്കം();
                സ്ഥിരം V3_സംഖ്യ& b_അക്കം = bConstp->അക്കം();
                // Max match is the shorter constant
                സ്ഥിരം uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                വേണ്ടി (; matchWidth < maxMatch; ++matchWidth) {
                    എങ്കിൽ (a_അക്കം.bitIs0(matchWidth) != b_അക്കം.bitIs0(matchWidth)) നിർത്തുക;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* സ്ഥിരം shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                തിരികെ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        എങ്കിൽ (DfgConcat* സ്ഥിരം catp = ap->cast<DfgConcat>()) തിരികെ commonLSBs(catp->rhsp(), bp);
        എങ്കിൽ (DfgConcat* സ്ഥിരം catp = bp->cast<DfgConcat>()) തിരികെ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        എങ്കിൽ (DfgSel* സ്ഥിരം selp = ap->cast<DfgSel>()) {
            എങ്കിൽ (selp->lsb() == 0) {
                DfgVertex* സ്ഥിരം fromp = selp->fromp();
                സ്ഥിരം std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                തിരികെ {common.first, std::min(common.second, selp->width())};
            }
        }
        എങ്കിൽ (DfgSel* സ്ഥിരം selp = bp->cast<DfgSel>()) {
            എങ്കിൽ (selp->lsb() == 0) {
                DfgVertex* സ്ഥിരം fromp = selp->fromp();
                സ്ഥിരം std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                തിരികെ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        തിരികെ {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        എങ്കിൽ (ap == bp) തിരികെ {ap, ap->width()};

        // If both constants, check MSBs
        എങ്കിൽ (DfgConst* സ്ഥിരം aConstp = ap->cast<DfgConst>()) {
            എങ്കിൽ (DfgConst* സ്ഥിരം bConstp = bp->cast<DfgConst>()) {
                സ്ഥിരം uint32_t aMsb = aConstp->width() - 1;
                സ്ഥിരം uint32_t bMsb = bConstp->width() - 1;
                സ്ഥിരം V3_സംഖ്യ& a_അക്കം = aConstp->അക്കം();
                സ്ഥിരം V3_സംഖ്യ& b_അക്കം = bConstp->അക്കം();
                // Max match is the shorter constant
                സ്ഥിരം uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                വേണ്ടി (; matchWidth < maxMatch; ++matchWidth) {
                    എങ്കിൽ (a_അക്കം.bitIs0(aMsb - matchWidth) != b_അക്കം.bitIs0(bMsb - matchWidth)) നിർത്തുക;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* സ്ഥിരം shorterp = aMsb < bMsb ? aConstp : bConstp;
                തിരികെ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        എങ്കിൽ (DfgConcat* സ്ഥിരം catp = ap->cast<DfgConcat>()) തിരികെ commonMSBs(catp->lhsp(), bp);
        എങ്കിൽ (DfgConcat* സ്ഥിരം catp = bp->cast<DfgConcat>()) തിരികെ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        എങ്കിൽ (DfgSel* സ്ഥിരം selp = ap->cast<DfgSel>()) {
            DfgVertex* സ്ഥിരം fromp = selp->fromp();
            എങ്കിൽ (selp->msb() == fromp->width() - 1) {
                സ്ഥിരം std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                തിരികെ {common.first, std::min(common.second, selp->width())};
            }
        }
        എങ്കിൽ (DfgSel* സ്ഥിരം selp = bp->cast<DfgSel>()) {
            DfgVertex* സ്ഥിരം fromp = selp->fromp();
            എങ്കിൽ (selp->msb() == fromp->width() - 1) {
                സ്ഥിരം std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                തിരികെ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        തിരികെ {nullptr, 0};
    }

    // VISIT methods

    ശൂന്യം visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    ശൂന്യം visit(Dfg_എണ്ണം_Ones* സ്ഥിരം vtxp) override {
        എങ്കിൽ (foldUnary(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgExtend* സ്ഥിരം vtxp) override {
        എങ്കിൽ (foldUnary(vtxp)) തിരികെ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* സ്ഥിരം zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            തിരികെ;
        }
    }

    ശൂന്യം visit(DfgExtendS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (foldUnary(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgLogNot* സ്ഥിരം vtxp) override {
        എങ്കിൽ (foldUnary(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgNegate* സ്ഥിരം vtxp) override {
        എങ്കിൽ (foldUnary(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgNot* സ്ഥിരം vtxp) override {
        എങ്കിൽ (foldUnary(vtxp)) തിരികെ;

        // Not of Cond
        എങ്കിൽ (DfgCond* സ്ഥിരം condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            എങ്കിൽ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(തള്ളുക_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* സ്ഥിരം newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* സ്ഥിരം newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* സ്ഥിരം newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    തിരികെ;
                }
            }
        }

        // Not of Not
        എങ്കിൽ (DfgNot* സ്ഥിരം notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                തിരികെ;
            }
        }

        എങ്കിൽ (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            എങ്കിൽ (DfgEq* സ്ഥിരം eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    തിരികെ;
                }
            }

            // Not of Neq
            എങ്കിൽ (DfgNeq* സ്ഥിരം neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    തിരികെ;
                }
            }

            // Not of unsigned comparisons
            എങ്കിൽ (DfgLt* സ്ഥിരം p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    തിരികെ;
                }
            }
            എങ്കിൽ (DfgGte* സ്ഥിരം p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    തിരികെ;
                }
            }
            എങ്കിൽ (DfgGt* സ്ഥിരം p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    തിരികെ;
                }
            }
            എങ്കിൽ (DfgLte* സ്ഥിരം p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    തിരികെ;
                }
            }

            // Not of signed comparisons
            എങ്കിൽ (DfgLtS* സ്ഥിരം p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    തിരികെ;
                }
            }
            എങ്കിൽ (DfgGteS* സ്ഥിരം p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    തിരികെ;
                }
            }
            എങ്കിൽ (DfgGtS* സ്ഥിരം p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    തിരികെ;
                }
            }
            എങ്കിൽ (DfgLteS* സ്ഥിരം p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgOneHot* സ്ഥിരം vtxp) override {
        എങ്കിൽ (foldUnary(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgOneHot0* സ്ഥിരം vtxp) override {
        എങ്കിൽ (foldUnary(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgRedOr* സ്ഥിരം vtxp) override {
        എങ്കിൽ (optimizeReduction(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgRedAnd* സ്ഥിരം vtxp) override {
        എങ്കിൽ (optimizeReduction(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgRedXor* സ്ഥിരം vtxp) override {
        എങ്കിൽ (optimizeReduction(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgSel* സ്ഥിരം vtxp) override {
        DfgVertex* സ്ഥിരം fromp = vtxp->fromp();

        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        സ്ഥിരം uint32_t lsb = vtxp->lsb();
        സ്ഥിരം uint32_t width = vtxp->width();
        സ്ഥിരം uint32_t msb = lsb + width - 1;

        എങ്കിൽ (DfgConst* സ്ഥിരം constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* സ്ഥിരം resp = makeZero(flp, width);
                resp->അക്കം().opSel(constp->അക്കം(), msb, lsb);
                replace(resp);
                തിരികെ;
            }
        }

        // Full width select, replace with the source.
        എങ്കിൽ (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                തിരികെ;
            }
        }

        // Unwind through bit packing in one go
        {
            സ്ഥിരം auto res = unwindSel(fromp, lsb, width);
            എങ്കിൽ (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                തിരികെ;
            }
        }

        // Sel from Not
        എങ്കിൽ (DfgNot* സ്ഥിരം notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            എങ്കിൽ (!notp->hasMultipleSinks()) {
                APPLYING(തള്ളുക_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* സ്ഥിരം newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    തിരികെ;
                }
            }
        }

        // Sel from Cond
        എങ്കിൽ (DfgCond* സ്ഥിരം condp = fromp->cast<DfgCond>()) {
            എങ്കിൽ (!condp->hasMultipleSinks()) {
                APPLYING(തള്ളുക_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* സ്ഥിരം newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* സ്ഥിരം newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* സ്ഥിരം newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    തിരികെ;
                }
            }
        }

        // Sel from ShiftL
        എങ്കിൽ (DfgShiftL* സ്ഥിരം shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            എങ്കിൽ (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(തള്ളുക_SEL_THROUGH_SHIFTL) {
                    DfgSel* സ്ഥിരം newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgMux* സ്ഥിരം vtxp) override {
        DfgVertex* സ്ഥിരം fromp = vtxp->fromp();
        DfgVertex* സ്ഥിരം lsbp = vtxp->lsbp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (DfgConst* സ്ഥിരം lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->അക്കം().toUInt()));
                തിരികെ;
            }
        }

        എങ്കിൽ (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                തിരികെ;
            }
        }

        എങ്കിൽ (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                തിരികെ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    ശൂന്യം visit(DfgAnd* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        എങ്കിൽ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            എങ്കിൽ (DfgNot* സ്ഥിരം lhsNotp = lhsp->cast<DfgNot>()) {
                എങ്കിൽ (DfgNot* സ്ഥിരം rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* സ്ഥിരം orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        തിരികെ;
                    }
                }
                എങ്കിൽ (DfgNeq* സ്ഥിരം rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* സ്ഥിരം newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* സ്ഥിരം orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം lConstp = lhsp->cast<DfgConst>()) {
            എങ്കിൽ (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    തിരികെ;
                }
            }

            എങ്കിൽ (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    തിരികെ;
                }
            }

            എങ്കിൽ (DfgConcat* സ്ഥിരം rhsConcatp = rhsp->cast<DfgConcat>()) {
                എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) തിരികെ;
            }
        }

        എങ്കിൽ (distributive_And_Associative_ദ്വിമാന<DfgOr, DfgAnd>(vtxp)) തിരികെ;

        എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Through_Reductions(vtxp)) തിരികെ;

        എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Throug_Sel(vtxp)) തിരികെ;

        {
            DfgNot* സ്ഥിരം lNotp = lhsp->cast<DfgNot>();
            DfgNot* സ്ഥിരം rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            എങ്കിൽ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    തിരികെ;
                }
            }

            എങ്കിൽ (DfgAnd* സ്ഥിരം rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* സ്ഥിരം rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* സ്ഥിരം rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                എങ്കിൽ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (vtxp->dtype() == m_bitDType) {
            എങ്കിൽ (tryReplaceBitwiseWithReduction(vtxp)) തിരികെ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            എങ്കിൽ (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            എങ്കിൽ (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* സ്ഥിരം newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgOr* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        എങ്കിൽ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            എങ്കിൽ (DfgNot* സ്ഥിരം lhsNotp = lhsp->cast<DfgNot>()) {
                എങ്കിൽ (DfgNot* സ്ഥിരം rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* സ്ഥിരം andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        തിരികെ;
                    }
                }
                എങ്കിൽ (DfgNeq* സ്ഥിരം rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* സ്ഥിരം newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* സ്ഥിരം andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (DfgConcat* സ്ഥിരം lhsConcatp = lhsp->cast<DfgConcat>()) {
            എങ്കിൽ (DfgConcat* സ്ഥിരം rhsConcatp = rhsp->cast<DfgConcat>()) {
                എങ്കിൽ (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    എങ്കിൽ (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            തിരികെ;
                        }
                    }
                    എങ്കിൽ (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            തിരികെ;
                        }
                    }
                }
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം lConstp = lhsp->cast<DfgConst>()) {
            എങ്കിൽ (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    തിരികെ;
                }
            }

            എങ്കിൽ (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    തിരികെ;
                }
            }

            എങ്കിൽ (DfgConcat* സ്ഥിരം rhsConcatp = rhsp->cast<DfgConcat>()) {
                എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) തിരികെ;
            }
        }

        എങ്കിൽ (distributive_And_Associative_ദ്വിമാന<DfgAnd, DfgOr>(vtxp)) തിരികെ;

        എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Through_Reductions(vtxp)) തിരികെ;

        എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Throug_Sel(vtxp)) തിരികെ;

        {
            DfgNot* സ്ഥിരം lNotp = lhsp->cast<DfgNot>();
            DfgNot* സ്ഥിരം rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            എങ്കിൽ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    തിരികെ;
                }
            }

            എങ്കിൽ (DfgOr* സ്ഥിരം rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* സ്ഥിരം rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* സ്ഥിരം rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                എങ്കിൽ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (vtxp->dtype() == m_bitDType) {
            എങ്കിൽ (tryReplaceBitwiseWithReduction(vtxp)) തിരികെ;
        }
    }

    ശൂന്യം visit(DfgXor* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                തിരികെ;
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം lConstp = lhsp->cast<DfgConst>()) {
            എങ്കിൽ (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    തിരികെ;
                }
            }
            എങ്കിൽ (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    തിരികെ;
                }
            }
            എങ്കിൽ (DfgConcat* സ്ഥിരം rConcatp = rhsp->cast<DfgConcat>()) {
                എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) തിരികെ;
            }
        }

        എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Through_Reductions(vtxp)) തിരികെ;

        എങ്കിൽ (try_തള്ളുക_Bitwise_Op_Throug_Sel(vtxp)) തിരികെ;

        എങ്കിൽ (vtxp->dtype() == m_bitDType) {
            എങ്കിൽ (tryReplaceBitwiseWithReduction(vtxp)) തിരികെ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    ശൂന്യം visit(DfgAdd* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                തിരികെ;
            }
        }

        സ്ഥിരം std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_എണ്ണം_Ones_Term(lhsp);
        എങ്കിൽ (DfgVertex* സ്ഥിരം lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_എണ്ണം_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            എങ്കിൽ (!std::get<0>(rTerm)) {
                എങ്കിൽ (DfgAdd* സ്ഥിരം rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_എണ്ണം_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            എങ്കിൽ (DfgVertex* സ്ഥിരം rVtxp = std::get<0>(rTerm)) {
                എങ്കിൽ (isSame(lVtxp, rVtxp)) {
                    സ്ഥിരം uint32_t lLsb = std::get<1>(lTerm);
                    സ്ഥിരം uint32_t rLsb = std::get<1>(rTerm);
                    സ്ഥിരം uint32_t lWidth = std::get<2>(lTerm);
                    സ്ഥിരം uint32_t rWidth = std::get<2>(rTerm);
                    തർക്കം adjoined = സത്യം;
                    uint32_t lsb = 0;
                    എങ്കിൽ (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } അല്ലെങ്കിൽ എങ്കിൽ (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } അല്ലെങ്കിൽ {
                        adjoined = അസത്യം;
                    }
                    എങ്കിൽ (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_എണ്ണം_ONES) {
                            DfgSel* സ്ഥിരം selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_എണ്ണം_Ones>(flp, DfgDataType::packed(32), selp);
                            എങ്കിൽ (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } അല്ലെങ്കിൽ എങ്കിൽ (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            എങ്കിൽ (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            തിരികെ;
                        }
                    }
                }
            }
        }
    }

    ശൂന്യം visit(DfgArraySel* സ്ഥിരം vtxp) override {
        DfgConst* സ്ഥിരം idxp = vtxp->bitp()->cast<DfgConst>();
        എങ്കിൽ (!idxp) തിരികെ;
        DfgVarArray* സ്ഥിരം varp = vtxp->fromp()->cast<DfgVarArray>();
        എങ്കിൽ (!varp) തിരികെ;
        AstVar* സ്ഥിരം astVarp = varp->vscp()->varp();
        എങ്കിൽ (astVarp->isForced()) തിരികെ;
        എങ്കിൽ (astVarp->isSigUserRWPublic()) തിരികെ;
        DfgVertex* സ്ഥിരം srcp = varp->srcp();
        എങ്കിൽ (!srcp) {
            എങ്കിൽ (vtxp->isPacked()) {
                എങ്കിൽ (AstConsArray* സ്ഥിരം iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    എങ്കിൽ (AstConst* സ്ഥിരം valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(പുതിയ DfgConst{m_dfg, valp->fileline(), valp->അക്കം()});
                            തിരികെ;
                        }
                    }
                }
            }
            തിരികെ;
        }

        എങ്കിൽ (DfgSpliceArray* സ്ഥിരം splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* സ്ഥിരം driverp = splicep->driverAt(idxp->toSizeT());
            എങ്കിൽ (!driverp) തിരികെ;
            DfgUnitArray* സ്ഥിരം uap = driverp->cast<DfgUnitArray>();
            എങ്കിൽ (!uap) തിരികെ;
            എങ്കിൽ (uap->srcp()->is<DfgVertexSplice>()) തിരികെ;
            // If driven by a variable that had a Driver in DFG, it is partial
            എങ്കിൽ (DfgVertexVar* സ്ഥിരം dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                എങ്കിൽ (dvarp->srcp()) തിരികെ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                തിരികെ;
            }
        }

        എങ്കിൽ (DfgUnitArray* സ്ഥിരം uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            എങ്കിൽ (uap->srcp()->is<DfgSplicePacked>()) തിരികെ;
            // If driven by a variable that had a Driver in DFG, it is partial
            എങ്കിൽ (DfgVertexVar* സ്ഥിരം dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                എങ്കിൽ (dvarp->srcp()) തിരികെ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgConcat* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();

        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isZero(lhsp)) {
            DfgConst* സ്ഥിരം lConstp = lhsp->as<DfgConst>();
            എങ്കിൽ (DfgSel* സ്ഥിരം rSelp = rhsp->cast<DfgSel>()) {
                എങ്കിൽ (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_മുകൾ_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (isZero(rhsp)) {
            DfgConst* സ്ഥിരം rConstp = rhsp->as<DfgConst>();
            എങ്കിൽ (DfgSel* സ്ഥിരം lSelp = lhsp->cast<DfgSel>()) {
                എങ്കിൽ (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (DfgNot* സ്ഥിരം lNot = lhsp->cast<DfgNot>()) {
            എങ്കിൽ (DfgNot* സ്ഥിരം rNot = rhsp->cast<DfgNot>()) {
                എങ്കിൽ (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(തള്ളുക_CONCAT_THROUGH_NOTS) {
                        DfgConcat* സ്ഥിരം newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        തിരികെ;
                    }
                }
            }
        }

        {
            തർക്കം swap = അസത്യം;
            എങ്കിൽ (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(തള്ളുക_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    തിരികെ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            എങ്കിൽ (DfgConcat* സ്ഥിരം rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* സ്ഥിരം rlp = rConcatp->lhsp();
                എങ്കിൽ (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(തള്ളുക_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* സ്ഥിരം newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (DfgSel* സ്ഥിരം lSelp = lhsp->cast<DfgSel>()) {
            എങ്കിൽ (DfgSel* സ്ഥിരം rSelp = rhsp->cast<DfgSel>()) {
                എങ്കിൽ (isSame(lSelp->fromp(), rSelp->fromp())) {
                    എങ്കിൽ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            തിരികെ;
                        }
                    }
                }
            }

            എങ്കിൽ (DfgConcat* സ്ഥിരം rConcatp = rhsp->cast<DfgConcat>()) {
                എങ്കിൽ (DfgSel* സ്ഥിരം rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    എങ്കിൽ (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        എങ്കിൽ (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                സ്ഥിരം uint32_t width = lSelp->width() + rlSelp->width();
                                സ്ഥിരം DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* സ്ഥിരം selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                തിരികെ;
                            }
                        }
                    }
                }
            }
        }

        എങ്കിൽ (DfgSel* സ്ഥിരം rSelp = rhsp->cast<DfgSel>()) {
            എങ്കിൽ (DfgConcat* സ്ഥിരം lConcatp = lhsp->cast<DfgConcat>()) {
                എങ്കിൽ (DfgSel* സ്ഥിരം lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    എങ്കിൽ (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        എങ്കിൽ (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                സ്ഥിരം uint32_t width = lrSelp->width() + rSelp->width();
                                സ്ഥിരം DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* സ്ഥിരം selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                തിരികെ;
                            }
                        }
                    }
                }
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം lConstp = lhsp->cast<DfgConst>()) {
            എങ്കിൽ (DfgCond* സ്ഥിരം rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* സ്ഥിരം rtVtxp = rCondp->thenp();
                DfgVertex* സ്ഥിരം reVtxp = rCondp->elsep();
                DfgConst* സ്ഥിരം rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* സ്ഥിരം reConstp = reVtxp->cast<DfgConst>();
                എങ്കിൽ (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(തള്ളുക_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* സ്ഥിരം thenp = [&]() -> DfgVertex* {
                            File_വരി* സ്ഥിരം rtFlp = rtVtxp->fileline();
                            എങ്കിൽ (rtConstp) {
                                DfgConst* സ്ഥിരം constp = makeZero(rtFlp, vtxp->width());
                                constp->അക്കം().opConcat(lConstp->അക്കം(), rtConstp->അക്കം());
                                തിരികെ constp;
                            }
                            തിരികെ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* സ്ഥിരം elsep = [&]() -> DfgVertex* {
                            File_വരി* സ്ഥിരം reFlp = reVtxp->fileline();
                            എങ്കിൽ (reConstp) {
                                DfgConst* സ്ഥിരം constp = makeZero(reFlp, vtxp->width());
                                constp->അക്കം().opConcat(lConstp->അക്കം(), reConstp->അക്കം());
                                തിരികെ constp;
                            }
                            തിരികെ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം rConstp = rhsp->cast<DfgConst>()) {
            എങ്കിൽ (DfgCond* സ്ഥിരം lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* സ്ഥിരം ltVtxp = lCondp->thenp();
                DfgVertex* സ്ഥിരം leVtxp = lCondp->elsep();
                DfgConst* സ്ഥിരം ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* സ്ഥിരം leConstp = leVtxp->cast<DfgConst>();
                എങ്കിൽ (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(തള്ളുക_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* സ്ഥിരം thenp = [&]() -> DfgVertex* {
                            File_വരി* സ്ഥിരം ltFlp = ltVtxp->fileline();
                            എങ്കിൽ (ltConstp) {
                                DfgConst* സ്ഥിരം constp = makeZero(ltFlp, vtxp->width());
                                constp->അക്കം().opConcat(ltConstp->അക്കം(), rConstp->അക്കം());
                                തിരികെ constp;
                            }
                            തിരികെ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* സ്ഥിരം elsep = [&]() -> DfgVertex* {
                            File_വരി* സ്ഥിരം leFlp = leVtxp->fileline();
                            എങ്കിൽ (leConstp) {
                                DfgConst* സ്ഥിരം constp = makeZero(leFlp, vtxp->width());
                                constp->അക്കം().opConcat(leConstp->അക്കം(), rConstp->അക്കം());
                                തിരികെ constp;
                            }
                            തിരികെ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        തിരികെ;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            സ്ഥിരം uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            സ്ഥിരം uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            സ്ഥിരം uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            തർക്കം hasCrossSink = അസത്യം;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                എങ്കിൽ (സ്ഥിരം DfgSel* സ്ഥിരം selp = sink.cast<DfgSel>()) {
                    സ്ഥിരം uint32_t selLsb = selp->lsb();
                    സ്ഥിരം uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    തിരികെ അസത്യം;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                എങ്കിൽ (സ്ഥിരം DfgVarPacked* സ്ഥിരം varp = sink.cast<DfgVarPacked>()) {
                    എങ്കിൽ (!varp->hasSinks() && !varp->isObserved()) തിരികെ അസത്യം;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                തിരികെ സത്യം;
            });
            എങ്കിൽ (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    എങ്കിൽ (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    എങ്കിൽ (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* സ്ഥിരം catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* സ്ഥിരം sp = പുതിയ DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* സ്ഥിരം varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(സത്യം);
                    varp->srcp(sp);
                    replace(varp);
                    തിരികെ;
                }
            }
        }

        // Convert various forms to REPLICATE
        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                തിരികെ;
            }
        }
        എങ്കിൽ (DfgRep* സ്ഥിരം lRepp = lhsp->cast<DfgRep>()) {
            എങ്കിൽ (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    തിരികെ;
                }
            }
        }
        എങ്കിൽ (DfgRep* സ്ഥിരം rRepp = rhsp->cast<DfgRep>()) {
            എങ്കിൽ (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    തിരികെ;
                }
            }
        }
        എങ്കിൽ (DfgRep* സ്ഥിരം lRepp = lhsp->cast<DfgRep>()) {
            എങ്കിൽ (DfgRep* സ്ഥിരം rRepp = rhsp->cast<DfgRep>()) {
                എങ്കിൽ (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        തിരികെ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            എങ്കിൽ (DfgConcat* സ്ഥിരം rConcatp = rhsp->cast<DfgConcat>()) {
                എങ്കിൽ (DfgRep* സ്ഥിരം rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    എങ്കിൽ (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            സ്ഥിരം uint32_t width = lRepp->width() + rlRepp->width();
                            സ്ഥിരം DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* സ്ഥിരം repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            തിരികെ;
                        }
                    }
                }
            }
        }
    }

    ശൂന്യം visit(DfgDiv* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        എങ്കിൽ (DfgConst* സ്ഥിരം rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            എങ്കിൽ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgDivS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        എങ്കിൽ (DfgConst* സ്ഥിരം rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            എങ്കിൽ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgGt* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgGtS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgGte* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgGteS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgLogAnd* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp) || vtxp->rhsp()->unsafe()) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();

        എങ്കിൽ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgLogEq* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgLogIf* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp) || vtxp->rhsp()->unsafe()) തിരികെ;
    }

    ശൂന്യം visit(DfgLogOr* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp) || vtxp->rhsp()->unsafe()) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();

        എങ്കിൽ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgLt* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgLtS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgLte* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgLteS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                തിരികെ;
            }
        }
    }

    ശൂന്യം visit(DfgModDiv* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgModDivS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgMul* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (DfgConst* സ്ഥിരം lConstp = lhsp->cast<DfgConst>()) {
            എങ്കിൽ (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    തിരികെ;
                }
            }
            എങ്കിൽ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgMulS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (DfgConst* സ്ഥിരം lConstp = lhsp->cast<DfgConst>()) {
            എങ്കിൽ (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    തിരികെ;
                }
            }
            എങ്കിൽ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgEq* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                തിരികെ;
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം lhsConstp = lhsp->cast<DfgConst>()) {
            എങ്കിൽ (DfgConcat* സ്ഥിരം rhsConcatp = rhsp->cast<DfgConcat>()) {
                എങ്കിൽ (try_തള്ളുക_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) തിരികെ;
            }

            എങ്കിൽ (rhsp->dtype() == m_bitDType) {
                എങ്കിൽ (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        തിരികെ;
                    }
                }

                എങ്കിൽ (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        തിരികെ;
                    }
                }
            }
        }
    }

    ശൂന്യം visit(DfgNeq* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                തിരികെ;
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം lhsConstp = lhsp->cast<DfgConst>()) {
            എങ്കിൽ (DfgConcat* സ്ഥിരം rhsConcatp = rhsp->cast<DfgConcat>()) {
                എങ്കിൽ (try_തള്ളുക_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) തിരികെ;
            }

            എങ്കിൽ (rhsp->dtype() == m_bitDType) {
                എങ്കിൽ (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        തിരികെ;
                    }
                }

                എങ്കിൽ (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        തിരികെ;
                    }
                }
            }
        }
    }

    ശൂന്യം visit(DfgPow* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgPowSS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgPowSU* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgPowUS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgRep* സ്ഥിരം vtxp) override {
        DfgVertex* സ്ഥിരം srcp = vtxp->srcp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (DfgConst* സ്ഥിരം sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* സ്ഥിരം resultp = makeZero(flp, vtxp->width());
                resultp->അക്കം().opRepl(sConstp->അക്കം(), vtxp->എണ്ണം());
                replace(resultp);
                തിരികെ;
            }
        }

        എങ്കിൽ (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                തിരികെ;
            }
        }

        എങ്കിൽ (DfgRep* സ്ഥിരം sRepp = srcp->cast<DfgRep>()) {
            എങ്കിൽ (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgShiftL* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
        എങ്കിൽ (optimizeShiftRHS(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();

        എങ്കിൽ (DfgShiftL* സ്ഥിരം lShiftLp = lhsp->cast<DfgShiftL>()) {
            എങ്കിൽ (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_വരി* സ്ഥിരം flp = vtxp->fileline();
                    സ്ഥിരം DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* സ്ഥിരം bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* സ്ഥിരം cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* സ്ഥിരം addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    തിരികെ;
                }
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം rConstp = rhsp->cast<DfgConst>()) {
            സ്ഥിരം uint32_t shiftAmount = rConstp->toU32();

            എങ്കിൽ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    തിരികെ;
                }
            }

            എങ്കിൽ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    തിരികെ;
                }
            }

            എങ്കിൽ (DfgConcat* സ്ഥിരം lConcatp = lhsp->cast<DfgConcat>()) {
                എങ്കിൽ (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* സ്ഥിരം lRhsp = lConcatp->rhsp();
                        DfgVertex* സ്ഥിരം lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        സ്ഥിരം uint32_t rW = shiftAmount;
                        സ്ഥിരം uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        സ്ഥിരം uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_വരി* സ്ഥിരം flp = lConcatp->fileline();
                        DfgVertex* സ്ഥിരം rp = makeZero(flp, shiftAmount);
                        DfgVertex* സ്ഥിരം mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        എങ്കിൽ (!lW) {
                            replace(np);
                            തിരികെ;
                        }
                        DfgVertex* സ്ഥിരം lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        തിരികെ;
                    }
                }
            }

            എങ്കിൽ (DfgSel* സ്ഥിരം lSelp = lhsp->cast<DfgSel>()) {
                എങ്കിൽ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        സ്ഥിരം uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* സ്ഥിരം nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        തിരികെ;
                    }
                }
            }

            എങ്കിൽ (DfgCond* സ്ഥിരം lCondp = lhsp->cast<DfgCond>()) {
                എങ്കിൽ (!lCondp->hasMultipleSinks()) {
                    APPLYING(തള്ളുക_SHIFTL_THROUGH_COND) {
                        DfgShiftL* സ്ഥിരം tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* സ്ഥിരം ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        തിരികെ;
                    }
                }
            }
        }
    }

    ശൂന്യം visit(DfgShiftR* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
        എങ്കിൽ (optimizeShiftRHS(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();

        എങ്കിൽ (DfgShiftR* സ്ഥിരം lShiftRp = lhsp->cast<DfgShiftR>()) {
            എങ്കിൽ (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_വരി* സ്ഥിരം flp = vtxp->fileline();
                    സ്ഥിരം DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* സ്ഥിരം bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* സ്ഥിരം cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* സ്ഥിരം addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    തിരികെ;
                }
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം rConstp = rhsp->cast<DfgConst>()) {
            സ്ഥിരം uint32_t shiftAmount = rConstp->toU32();

            എങ്കിൽ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    തിരികെ;
                }
            }

            എങ്കിൽ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    തിരികെ;
                }
            }

            എങ്കിൽ (DfgConcat* സ്ഥിരം lConcatp = lhsp->cast<DfgConcat>()) {
                എങ്കിൽ (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* സ്ഥിരം lRhsp = lConcatp->rhsp();
                        DfgVertex* സ്ഥിരം lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        സ്ഥിരം uint32_t lW = shiftAmount;
                        സ്ഥിരം uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        സ്ഥിരം uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_വരി* സ്ഥിരം flp = lConcatp->fileline();
                        DfgVertex* സ്ഥിരം lp = makeZero(flp, shiftAmount);
                        DfgVertex* സ്ഥിരം mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        എങ്കിൽ (!rW) {
                            replace(np);
                            തിരികെ;
                        }
                        DfgVertex* സ്ഥിരം rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        തിരികെ;
                    }
                }
            }

            എങ്കിൽ (DfgSel* സ്ഥിരം lSelp = lhsp->cast<DfgSel>()) {
                എങ്കിൽ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        സ്ഥിരം uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* സ്ഥിരം nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        തിരികെ;
                    }
                }
            }

            എങ്കിൽ (DfgCond* സ്ഥിരം lCondp = lhsp->cast<DfgCond>()) {
                എങ്കിൽ (!lCondp->hasMultipleSinks()) {
                    APPLYING(തള്ളുക_SHIFTR_THROUGH_COND) {
                        DfgShiftR* സ്ഥിരം tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* സ്ഥിരം ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        തിരികെ;
                    }
                }
            }
        }
    }

    ശൂന്യം visit(DfgShiftRS* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;
        എങ്കിൽ (optimizeShiftRHS(vtxp)) തിരികെ;
    }

    ശൂന്യം visit(DfgSub* സ്ഥിരം vtxp) override {
        എങ്കിൽ (ദ്വിമാന(vtxp)) തിരികെ;

        DfgVertex* സ്ഥിരം lhsp = vtxp->lhsp();
        DfgVertex* സ്ഥിരം rhsp = vtxp->rhsp();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                തിരികെ;
            }
        }

        എങ്കിൽ (DfgConst* സ്ഥിരം rConstp = rhsp->cast<DfgConst>()) {
            എങ്കിൽ (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    തിരികെ;
                }
            }
            എങ്കിൽ (vtxp->dtype() == m_bitDType && rConstp->has_മൂല്യം(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgMatchMasked* സ്ഥിരം vtxp) override {
        എങ്കിൽ (DfgConst* സ്ഥിരം constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* സ്ഥിരം matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->അക്കം(), matchVarp)));
                തിരികെ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    ശൂന്യം visit(DfgCond* സ്ഥിരം vtxp) override {
        DfgVertex* സ്ഥിരം condp = vtxp->condp();
        DfgVertex* സ്ഥിരം thenp = vtxp->thenp();
        DfgVertex* സ്ഥിരം elsep = vtxp->elsep();
        File_വരി* സ്ഥിരം flp = vtxp->fileline();

        എങ്കിൽ (condp->dtype() != m_bitDType) തിരികെ;

        എങ്കിൽ (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                തിരികെ;
            }
        }

        എങ്കിൽ (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                തിരികെ;
            }
        }

        എങ്കിൽ (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                തിരികെ;
            }
        }

        എങ്കിൽ (DfgNot* സ്ഥിരം condNotp = condp->cast<DfgNot>()) {
            എങ്കിൽ (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    തിരികെ;
                }
            }
        }

        എങ്കിൽ (DfgNeq* സ്ഥിരം condNeqp = condp->cast<DfgNeq>()) {
            എങ്കിൽ (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* സ്ഥിരം newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    തിരികെ;
                }
            }
        }

        എങ്കിൽ (DfgNot* സ്ഥിരം thenNotp = thenp->cast<DfgNot>()) {
            എങ്കിൽ (DfgNot* സ്ഥിരം elseNotp = elsep->cast<DfgNot>()) {
                എങ്കിൽ (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* സ്ഥിരം newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (DfgOr* സ്ഥിരം condOrp = condp->cast<DfgOr>()) {
            എങ്കിൽ (DfgCond* സ്ഥിരം thenCondp = thenp->cast<DfgCond>()) {
                എങ്കിൽ (!thenCondp->hasMultipleSinks()) {
                    എങ്കിൽ (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* സ്ഥിരം ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            തിരികെ;
                        }
                    }
                    എങ്കിൽ (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* സ്ഥിരം ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            തിരികെ;
                        }
                    }
                }
            }
        }

        എങ്കിൽ (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            എങ്കിൽ (DfgAdd* സ്ഥിരം thenAddp = thenp->cast<DfgAdd>()) {
                എങ്കിൽ (DfgConst* സ്ഥിരം constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    എങ്കിൽ (constp->has_മൂല്യം(1)) {
                        എങ്കിൽ (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* സ്ഥിരം extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_വരി* സ്ഥിരം thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                തിരികെ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            എങ്കിൽ (DfgSub* സ്ഥിരം thenSubp = thenp->cast<DfgSub>()) {
                എങ്കിൽ (DfgConst* സ്ഥിരം constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    എങ്കിൽ (constp->has_മൂല്യം(1)) {
                        എങ്കിൽ (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* സ്ഥിരം extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_വരി* സ്ഥിരം thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                തിരികെ;
                            }
                        }
                    }
                }
            }
        }

        എങ്കിൽ (vtxp->dtype() == m_bitDType) {
            എങ്കിൽ (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    തിരികെ;
                }
            }
            എങ്കിൽ (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    തിരികെ;
                }
            }
        }

        എങ്കിൽ (vtxp->width() <= VL_QUADSIZE) {
            എങ്കിൽ (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* സ്ഥിരം maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    തിരികെ;
                }
            }
            എങ്കിൽ (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* സ്ഥിരം maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    തിരികെ;
                }
            }
            എങ്കിൽ (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* സ്ഥിരം maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    തിരികെ;
                }
            }
            എങ്കിൽ (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* സ്ഥിരം maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    തിരികെ;
                }
            }

            എങ്കിൽ (DfgOr* സ്ഥിരം tOrp = thenp->cast<DfgOr>()) {
                എങ്കിൽ (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* സ്ഥിരം maskp = replicate(vtxp, condp);
                        DfgAnd* സ്ഥിരം andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        തിരികെ;
                    }
                }
                എങ്കിൽ (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* സ്ഥിരം maskp = replicate(vtxp, condp);
                        DfgAnd* സ്ഥിരം andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        തിരികെ;
                    }
                }
            }
        }

        എങ്കിൽ (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            സ്ഥിരം std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            എങ്കിൽ (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    സ്ഥിരം uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    എങ്കിൽ (rWidth != rhsp->width()) {
                        സ്ഥിരം DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    എങ്കിൽ (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        തിരികെ;
                    }
                    // Create new LHS
                    സ്ഥിരം uint32_t lWidth = vtxp->width() - rWidth;
                    സ്ഥിരം DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* സ്ഥിരം lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* സ്ഥിരം lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* സ്ഥിരം lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    തിരികെ;
                }
            }

            സ്ഥിരം std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            എങ്കിൽ (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    സ്ഥിരം uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    എങ്കിൽ (lWidth != lhsp->width()) {
                        സ്ഥിരം DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    എങ്കിൽ (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        തിരികെ;
                    }
                    // Create new RHS
                    സ്ഥിരം uint32_t rWidth = vtxp->width() - lWidth;
                    സ്ഥിരം DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* സ്ഥിരം rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* സ്ഥിരം rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* സ്ഥിരം rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    തിരികെ;
                }
            }
        }

        എങ്കിൽ (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                എങ്കിൽ (സ്ഥിരം uint32_t extend = vtxp->width() - 1) {
                    DfgConst* സ്ഥിരം zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                തിരികെ;
            }
        }

        എങ്കിൽ (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                എങ്കിൽ (സ്ഥിരം uint32_t extend = vtxp->width() - 1) {
                    DfgConst* സ്ഥിരം zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                തിരികെ;
            }
        }

        എങ്കിൽ (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                തിരികെ;
            }
        }

        എങ്കിൽ (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                തിരികെ;
            }
        }

        എങ്കിൽ (DfgCond* സ്ഥിരം tCondp = thenp->cast<DfgCond>()) {
            എങ്കിൽ (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    തിരികെ;
                }
            }
        }

        എങ്കിൽ (DfgCond* സ്ഥിരം eCondp = elsep->cast<DfgCond>()) {
            എങ്കിൽ (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    തിരികെ;
                }
            }
        }
    }

    ശൂന്യം visit(DfgVertexVar* സ്ഥിരം vtxp) override {
        എങ്കിൽ (vtxp->hasSinks()) തിരികെ;
        എങ്കിൽ (vtxp->isObserved()) തിരികെ;
        എങ്കിൽ (vtxp->defaultp()) തിരികെ;

        // If undriven, or driven from another var, it is completely redundant.
        എങ്കിൽ (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                തിരികെ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        തർക്കം foundOne = അസത്യം;
        സ്ഥിരം തർക്കം keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> തർക്കം {
            // Ignore non-observable variable sinks. These can be eliminated.
            എങ്കിൽ (സ്ഥിരം DfgVertexVar* സ്ഥിരം varp = sink.cast<DfgVertexVar>()) {
                എങ്കിൽ (!varp->hasSinks() && !varp->isObserved()) തിരികെ അസത്യം;
            }
            // Keep if found more than one sink
            എങ്കിൽ (foundOne) തിരികെ സത്യം;
            // Mark first sink found
            foundOne = സത്യം;
            തിരികെ അസത്യം;
        });
        എങ്കിൽ (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                തിരികെ;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.തള്ളുക_back(nullptr);
        m_iterList.തള്ളുക_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        വേണ്ടി (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        വേണ്ടി (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        ആകുമ്പോൾ (സത്യം) {
            // Process the work list - keep the placeholder at index 0
            ആകുമ്പോൾ (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.എടുക്കുക_back();
                എങ്കിൽ (!m_vtxp) തുടരുക;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                എങ്കിൽ (DfgVertexVar* സ്ഥിരം varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    തുടരുക;
                }

                // Ast references are not considered
                എങ്കിൽ (m_vtxp->is<DfgVertexAst>()) തുടരുക;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                എങ്കിൽ (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    എങ്കിൽ (DfgVertex* സ്ഥിരം sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            തുടരുക;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            എങ്കിൽ (m_iterList.size() == 1) നിർത്തുക;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                വേണ്ടി (size_t hops = 1; hops <= 4; ++hops) {
                    വേണ്ടി (size_t i = begin; i < end; ++i) {
                        DfgVertex* സ്ഥിരം vtxp = m_iterList[i];
                        എങ്കിൽ (!vtxp) തുടരുക;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            തിരികെ അസത്യം;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            തിരികെ അസത്യം;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : m_iterList) {
                എങ്കിൽ (!vtxp) തുടരുക;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

പൊതു:
    സ്ഥിതം ശൂന്യം apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

ശൂന്യം V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    എങ്കിൽ (!v3Global.opt.fDfgPeephole()) തിരികെ;
    V3DfgPeephole::apply(dfg, ctx);
}
