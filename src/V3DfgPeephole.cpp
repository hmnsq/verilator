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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgCache.h"
#अन्तर्भाव "V3DfgDataType.h"
#अन्तर्भाव "V3DfgPasses.h"
#अन्तर्भाव "V3DfgPeepholePatterns.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cctype>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
नामाकाश {

ढाँचा <typename T_Reduction>
संरचना ReductionToBitwiseImpl {};
ढाँचा <> संरचना ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
ढाँचा <> संरचना ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
ढाँचा <> संरचना ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
ढाँचा <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

ढाँचा <typename T_Bitwise>
संरचना BitwiseToReductionImpl {};
ढाँचा <> संरचना BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
ढाँचा <> संरचना BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
ढाँचा <> संरचना BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
ढाँचा <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
ढाँचा<typename Vertex>
संरचना IsAssociative : सार्वजनिक std::false_type {};
ढाँचा <> संरचना IsAssociative<DfgAnd> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsAssociative<DfgOr>  : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsAssociative<DfgXor> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsAssociative<DfgAdd> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsAssociative<DfgConcat> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsAssociative<DfgMul> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsAssociative<DfgMulS> : सार्वजनिक std::true_type {};

// Commutative binary operators (a op b == b op a)
ढाँचा<typename Vertex>
संरचना IsCommutative : सार्वजनिक std::false_type {};
ढाँचा <> संरचना IsCommutative<DfgAnd> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsCommutative<DfgOr>  : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsCommutative<DfgXor> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsCommutative<DfgAdd> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsCommutative<DfgEq> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsCommutative<DfgMul> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsCommutative<DfgMulS> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsCommutative<DfgNeq> : सार्वजनिक std::true_type {};

// Idempotent binary operators (a op a == a)
ढाँचा<typename Vertex>
संरचना IsIdempotent : सार्वजनिक std::false_type {};
ढाँचा <> संरचना IsIdempotent<DfgAnd> : सार्वजनिक std::true_type {};
ढाँचा <> संरचना IsIdempotent<DfgOr>  : सार्वजनिक std::true_type {};

// Binary result dtype
ढाँचा<typename Vertex>
अचर DfgDataType& resultDType(अचर DfgVertex* lhsp, अचर DfgVertex* rhsp);
ढाँचा <> अचर DfgDataType& resultDType<DfgAdd>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgAnd>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgConcat>     (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(lhsp->width() + rhsp->width()); }
ढाँचा <> अचर DfgDataType& resultDType<DfgDiv>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgDivS>       (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgEq>         (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgGt>         (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgGtS>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgGte>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgGteS>       (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgLogAnd>     (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgLogEq>      (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgLogIf>      (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgLogOr>      (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgLt>         (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgLtS>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgLte>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgLteS>       (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgModDiv>     (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgModDivS>    (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgMul>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgMulS>       (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgNeq>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ DfgDataType::packed(1); }
ढाँचा <> अचर DfgDataType& resultDType<DfgOr>         (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgPow>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgPowSS>      (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgPowSU>      (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgPowUS>      (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgShiftL>     (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgShiftR>     (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgShiftRS>    (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgSub>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }
ढाँचा <> अचर DfgDataType& resultDType<DfgXor>        (अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) { फर्काऊ lhsp->dtype(); }

// Unary constant folding
ढाँचा<typename Vertex> शून्य foldOp(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src);
ढाँचा <> शून्य foldOp<Dfg_गन्ती_Ones>  (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.op_गन्ती_Ones(src); }
ढाँचा <> शून्य foldOp<DfgExtend>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opAssign(src); }
ढाँचा <> शून्य foldOp<DfgExtendS>    (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opExtendS(src, src.width()); }
ढाँचा <> शून्य foldOp<DfgLogNot>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opLogNot(src); }
ढाँचा <> शून्य foldOp<DfgNegate>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opNegate(src); }
ढाँचा <> शून्य foldOp<DfgNot>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opNot(src); }
ढाँचा <> शून्य foldOp<DfgOneHot>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opOneHot(src); }
ढाँचा <> शून्य foldOp<DfgOneHot0>    (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opOneHot0(src); }
ढाँचा <> शून्य foldOp<DfgRedAnd>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opRedAnd(src); }
ढाँचा <> शून्य foldOp<DfgRedOr>      (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opRedOr(src); }
ढाँचा <> शून्य foldOp<DfgRedXor>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& src) { out.opRedXor(src); }

// Binary constant folding
ढाँचा<typename Vertex> शून्य foldOp(V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
ढाँचा <> शून्य foldOp<DfgAdd>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opAdd(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgAnd>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opAnd(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgConcat>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opConcat(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgDiv>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opDiv(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgDivS>       (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opDivS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgEq>         (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opEq(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgGt>         (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opGt(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgGtS>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opGtS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgGte>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opGte(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgGteS>       (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opGteS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgLogAnd>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opLogAnd(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgLogEq>      (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opLogEq(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgLogIf>      (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opLogIf(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgLogOr>      (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opLogOr(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgLt>         (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opLt(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgLtS>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opLtS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgLte>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opLte(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgLteS>       (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opLteS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgModDiv>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opModDiv(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgModDivS>    (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opModDivS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgMul>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opMul(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgMulS>       (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opMulS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgNeq>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opNeq(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgOr>         (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opOr(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgPow>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opPow(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgPowSS>      (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opPowSS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgPowSU>      (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opPowSU(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgPowUS>      (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opPowUS(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgShiftL>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opShiftL(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgShiftR>     (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opShiftR(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgShiftRS>    (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
ढाँचा <> शून्य foldOp<DfgSub>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opSub(lhs, rhs); }
ढाँचा <> शून्य foldOp<DfgXor>        (V3_सङ्ख्या& out, अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

वर्ग V3DfgPeephole final : सार्वजनिक DfgVisitor {
    // TYPES
    संरचना VertexInfo final {
        माप_प्रकार m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        माप_प्रकार m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        माप_प्रकार m_generation = 0;  // Generation number of this vertex - for uniqueness check
        माप_प्रकार m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        तार्किक m_isCachedVertex = असत्य;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    अचर DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    माप_प्रकार m_currentGeneration = 0;  // Current generation number
    माप_प्रकार m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* अचर m_tmpScopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();

    // STATIC STATE
    स्थिर V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) यदि (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    तार्किक checkApplying(VDfgPeepholePattern id) {
        यदि (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) फर्काऊ असत्य;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_गन्ती[id];
        फर्काऊ सत्य;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* अचर equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        फर्काऊ equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    शून्य invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = असत्य;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_पङ्क्ति* flp, अचर DfgDataType& dtype, Operands... operands) {
        Vertex* अचर vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = सत्य;
        फर्काऊ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* getVertex(अचर DfgDataType& dtype, Operands... operands) {
        Vertex* अचर vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        फर्काऊ vtxp;
    }

    शून्य incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    शून्य addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        यदि (vInfo.m_workListIndex) फर्काऊ;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.थप_back(vtxp);
    }

    शून्य removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    शून्य addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        यदि (vInfo.m_iterListIndex) फर्काऊ;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.थप_back(vtxp);
    }

    शून्य removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    शून्य deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        अचर DfgVertexVar* अचर varp = vtxp->cast<DfgVertexVar>();
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
            यदि (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                फर्काऊ असत्य;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            यदि (vInfo.m_generation != m_currentGeneration) srcps.थप_back(&src);
            vInfo.m_generation = m_currentGeneration;
            फर्काऊ असत्य;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        यदि (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.थप_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } अन्यथा {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        अचर स्वतः mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  फर्काऊ srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        लागि (स्वतः it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        लागि (स्वतः it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    शून्य replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        अचर स्वतः debugCallback = [&]() -> शून्य {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            अचर स्वतः cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](अचर DfgVertex& v) {  //
                फर्काऊ cone->गन्ती(&v);
            });
        };
        यदि (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) फर्काऊ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            फर्काऊ असत्य;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            फर्काऊ असत्य;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            फर्काऊ असत्य;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            फर्काऊ असत्य;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_पङ्क्ति* flp, uint32_t val) { फर्काऊ नूतन DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_पङ्क्ति* flp, uint32_t width) {
        फर्काऊ नूतन DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_पङ्क्ति* flp, अचर DfgDataType& dtype) {
        फर्काऊ makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_पङ्क्ति* flp, uint32_t width) {
        DfgConst* अचर resp = makeZero(flp, width);
        resp->अङ्क().setAllBits1();
        फर्काऊ resp;
    }

    // Create a new vertex of the given type
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* make(File_पङ्क्ति* flp, अचर DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* अचर vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        यदि (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        यदि (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        फर्काऊ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* make(अचर DfgVertex* examplep, Operands... operands) {
        फर्काऊ make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        यदि (vtxp->dtype() == m_bitDType) फर्काऊ bitp;
        फर्काऊ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    स्थिर तार्किक isSame(अचर DfgVertex* ap, अचर DfgVertex* bp) {
        यदि (ap == bp) फर्काऊ सत्य;
        यदि (ap->dtype() != bp->dtype()) फर्काऊ असत्य;
        अचर DfgConst* अचर aConstp = ap->cast<DfgConst>();
        यदि (!aConstp) फर्काऊ असत्य;
        अचर DfgConst* अचर bConstp = bp->cast<DfgConst>();
        यदि (!bConstp) फर्काऊ असत्य;
        फर्काऊ aConstp->अङ्क().isCaseEq(bConstp->अङ्क());
    }

    स्थिर तार्किक isZero(अचर DfgVertex* vtxp) {
        यदि (अचर DfgConst* अचर constp = vtxp->cast<DfgConst>()) फर्काऊ constp->isZero();
        फर्काऊ असत्य;
    }

    स्थिर तार्किक isOnes(अचर DfgVertex* vtxp) {
        यदि (अचर DfgConst* अचर constp = vtxp->cast<DfgConst>()) फर्काऊ constp->isOnes();
        फर्काऊ असत्य;
    }

    स्थिर तार्किक isEqOne(अचर DfgVertex* vtxp) {
        यदि (अचर DfgConst* अचर constp = vtxp->cast<DfgConst>()) फर्काऊ constp->अङ्क().isEqOne();
        फर्काऊ असत्य;
    }

    स्थिर तार्किक areAdjacent(uint32_t& lsb, अचर DfgSel* lSelp, अचर DfgSel* rSelp) {
        यदि (!isSame(lSelp->srcp(), rSelp->srcp())) फर्काऊ असत्य;
        यदि (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            फर्काऊ सत्य;
        }
        यदि (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    ढाँचा <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक foldUnary(Vertex* अचर vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::मान, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::मान, "Must invoke on final class");
        यदि (DfgConst* अचर srcp = vtxp->srcp()->ढाँचा cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* अचर resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->अङ्क(), srcp->अङ्क());
                replace(resultp);
                फर्काऊ सत्य;
            }
        }
        फर्काऊ असत्य;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    ढाँचा <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक द्विआधारी(Vertex* अचर vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विआधारी, Vertex>::मान, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::मान, "Must invoke on final class");

        File_पङ्क्ति* अचर flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* अचर lConstp = lhsp->cast<DfgConst>();
        DfgConst* अचर rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* अचर lSamep = lhsp->cast<Vertex>();
        Vertex* अचर rSamep = rhsp->cast<Vertex>();

        // Constant folding
        यदि (lConstp && rConstp) {
            APPLYING(FOLD_द्विआधारी) {
                DfgConst* अचर resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->अङ्क(), lConstp->अङ्क(), rConstp->अङ्क());
                replace(resultp);
                फर्काऊ सत्य;
            }
        }

        // Idempotent
        यदि VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::मान) {
            यदि (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_द्विआधारी) {
                    replace(lhsp);
                    फर्काऊ सत्य;
                }
            }
        }

        // Associative
        यदि VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::मान) {
            // Also commutative and idempotent
            यदि VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मान && IsIdempotent<Vertex>::मान) {
                यदि (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_द्विआधारी_LHS) {
                        replace(lSamep);
                        फर्काऊ सत्य;
                    }
                }
                यदि (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_द्विआधारी_RHS) {
                        replace(rSamep);
                        फर्काऊ सत्य;
                    }
                }
            }

            यदि (lConstp && rSamep) {
                यदि (DfgConst* अचर rlConstp = rSamep->lhsp()->ढाँचा cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_द्विआधारी_LHS_OF_RHS) {
                        DfgConst* अचर cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->अङ्क(), lConstp->अङ्क(), rlConstp->अङ्क());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        फर्काऊ सत्य;
                    }
                }
            }

            यदि (lSamep && rConstp) {
                यदि (DfgConst* अचर lrConstp = lSamep->rhsp()->ढाँचा cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_द्विआधारी_RHS_OF_LHS) {
                        DfgConst* अचर cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->अङ्क(), lrConstp->अङ्क(), rConstp->अङ्क());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        फर्काऊ सत्य;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            यदि (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* अचर ap = lSamep->lhsp();
                DfgVertex* अचर bp = lSamep->rhsp();
                DfgVertex* अचर cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                यदि (ap != cp && bp != cp) {
                    APPLYING(दायाँ_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        अचर DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* अचर bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        फर्काऊ सत्य;
                    }
                }
            }

            यदि (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* अचर rlVtxp = rSamep->lhsp();
                DfgVertex* अचर rrVtxp = rSamep->rhsp();

                यदि VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मान) {
                    यदि (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            फर्काऊ सत्य;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                यदि (Vertex* अचर existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    यदि (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_द्विआधारी_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            फर्काऊ सत्य;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                यदि VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मान) {
                    यदि (Vertex* अचर existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        यदि (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_द्विआधारी_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                फर्काऊ सत्य;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        यदि VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मान) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            यदि (lConstp) {
                यदि (DfgCond* अचर rCondp = rhsp->cast<DfgCond>()) {
                    यदि (!rCondp->hasMultipleSinks()) {
                        APPLYING(थप_COMMUTATIVE_द्विआधारी_THROUGH_COND) {
                            DfgVertex* अचर tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* अचर ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            फर्काऊ सत्य;
                        }
                    }
                }
                फर्काऊ असत्य;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        अचर तार्किक swapSides = [&]() {
            // Don't swap if not commutative, duh
            यदि VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::मान) फर्काऊ असत्य;
            // Const go on left
            यदि (lConstp) फर्काऊ असत्य;
            यदि (rConstp) फर्काऊ सत्य;
            // Not go on left
            यदि (lhsp->is<DfgNot>()) फर्काऊ असत्य;
            यदि (rhsp->is<DfgNot>()) फर्काऊ सत्य;
            // Same go on right
            यदि (rSamep) फर्काऊ असत्य;
            यदि (lSamep) फर्काऊ सत्य;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            अचर VertexInfo& lInfo = m_vInfo[lhsp];
            अचर VertexInfo& rInfo = m_vInfo[rhsp];
            फर्काऊ lInfo.m_id > rInfo.m_id;
        }();
        यदि (swapSides) {
            APPLYING(SWAP_SIDES_IN_द्विआधारी) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                फर्काऊ सत्य;
            }
        }

        फर्काऊ असत्य;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    ढाँचा <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक distributive_And_Associative_द्विआधारी(Vertex* vtxp) {
        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        यदि (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            यदि (Other* अचर lp = lhsp->cast<Other>()) {
                यदि (Other* अचर rp = rhsp->cast<Other>()) {
                    DfgVertex* अचर llp = lp->lhsp();
                    DfgVertex* अचर lrp = lp->rhsp();
                    DfgVertex* अचर rlp = rp->lhsp();
                    DfgVertex* अचर rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    यदि (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } अन्यथा यदि (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } अन्यथा यदि (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } अन्यथा यदि (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    यदि (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_द्विआधारी) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            फर्काऊ सत्य;
                        }
                    }
                }
            }
        }

        फर्काऊ असत्य;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    ढाँचा <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक
    try_थप_Bitwise_Op_Through_Concat(Vertex* अचर vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* अचर catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* अचर catRConstp = concatp->rhsp()->cast<DfgConst>();
        यदि (catLConstp || catRConstp) {
            APPLYING(थप_BITWISE_OP_THROUGH_CONCAT) {
                अचर uint32_t width = concatp->width();
                अचर DfgDataType& lDtype = concatp->lhsp()->dtype();
                अचर DfgDataType& rDtype = concatp->rhsp()->dtype();
                अचर uint32_t lWidth = lDtype.size();
                अचर uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* अचर newLhsp = [&]() -> DfgVertex* {
                    DfgConst* अचर newLhsConstp = makeZero(constp->fileline(), lWidth);
                    यदि (catLConstp) {
                        V3_सङ्ख्या अङ्क{constp->fileline(), static_cast<पूर्णाङ्क>(lWidth), 0u};
                        अङ्क.opSel(constp->अङ्क(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->अङ्क(), अङ्क, catLConstp->अङ्क());
                        फर्काऊ newLhsConstp;
                    }
                    newLhsConstp->अङ्क().opSel(constp->अङ्क(), width - 1, rWidth);
                    फर्काऊ make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* अचर newRhsp = [&]() -> DfgVertex* {
                    DfgConst* अचर newRhsConstp = makeZero(constp->fileline(), rWidth);
                    यदि (catRConstp) {
                        V3_सङ्ख्या अङ्क{constp->fileline(), static_cast<पूर्णाङ्क>(rWidth), 0u};
                        अङ्क.opSel(constp->अङ्क(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->अङ्क(), अङ्क, catRConstp->अङ्क());
                        फर्काऊ newRhsConstp;
                    }
                    newRhsConstp->अङ्क().opSel(constp->अङ्क(), rWidth - 1, 0);
                    फर्काऊ make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                फर्काऊ सत्य;
            }
        }
        फर्काऊ असत्य;
    }

    ढाँचा <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक
    try_थप_Compare_Op_Through_Concat(Vertex* अचर vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        यदि (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(थप_COMPARE_OP_THROUGH_CONCAT) {
                अचर uint32_t width = concatp->width();
                अचर uint32_t lWidth = concatp->lhsp()->width();
                अचर uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* अचर newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->अङ्क().opSel(constp->अङ्क(), width - 1, rWidth);
                Vertex* अचर newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* अचर newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->अङ्क().opSel(constp->अङ्क(), rWidth - 1, 0);
                Vertex* अचर newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                यदि VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::मान) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } अन्यथा यदि VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::मान) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } अन्यथा {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                फर्काऊ सत्य;
            }
        }
        फर्काऊ असत्य;
    }

    ढाँचा <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक try_थप_Bitwise_Op_Through_Reductions(Bitwise* अचर vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        यदि (Reduction* अचर lRedp = vtxp->lhsp()->ढाँचा cast<Reduction>()) {
            यदि (Reduction* अचर rRedp = vtxp->rhsp()->ढाँचा cast<Reduction>()) {
                DfgVertex* अचर lSrcp = lRedp->srcp();
                DfgVertex* अचर rSrcp = rRedp->srcp();
                यदि (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(थप_BITWISE_THROUGH_REDUCTION) {
                        File_पङ्क्ति* अचर flp = vtxp->fileline();
                        Bitwise* अचर bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        फर्काऊ सत्य;
                    }
                }
            }
        }

        फर्काऊ असत्य;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    स्थिर std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        यदि (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            फर्काऊ {nullptr, nullptr};
        }
        यदि (vtxp->hasMultipleSinks()) फर्काऊ {nullptr, nullptr};
        Dfg_Vertex_द्विआधारी* अचर bwp = vtxp->as<Dfg_Vertex_द्विआधारी>();
        DfgSel* अचर lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* अचर rSelp = bwp->rhsp()->cast<DfgSel>();
        यदि (!lSelp || !rSelp) फर्काऊ {nullptr, nullptr};
        DfgSel* अचर nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* अचर wSelp = nSelp == lSelp ? rSelp : lSelp;
        अचर uint32_t nWidth = nSelp->fromp()->width();
        यदि (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) फर्काऊ {nullptr, nullptr};
        यदि (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            फर्काऊ {nullptr, nullptr};
        }
        फर्काऊ {nSelp, wSelp};
    }

    ढाँचा <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक try_थप_Bitwise_Op_Throug_Sel(Bitwise* अचर vtxp) {
        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();

        यदि (DfgSel* अचर lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            यदि (DfgSel* अचर selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } अन्यथा यदि (Bitwise* अचर bitwisep = rhsp->cast<Bitwise>()) {
                यदि (DfgSel* अचर rlSelp = bitwisep->lhsp()->ढाँचा cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } अन्यथा यदि (DfgSel* अचर rrSelp = bitwisep->rhsp()->ढाँचा cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            यदि (rSelp) {
                DfgVertex* अचर lFromp = lSelp->fromp();
                DfgVertex* अचर rFromp = rSelp->fromp();
                यदि (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(थप_BITWISE_THROUGH_SEL) {
                        Bitwise* अचर bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        यदि (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        फर्काऊ सत्य;
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
            Bitwise* अचर rChainp = rhsp->cast<Bitwise>();
            यदि (rChainp && rChainp->hasMultipleSinks()) फर्काऊ असत्य;
            DfgVertex* अचर rTermp = rChainp ? rChainp->lhsp() : rhsp;

            अचर स्वतः lSels = pushableUnalignedSels(lhsp);
            अचर स्वतः rSels = pushableUnalignedSels(rTermp);
            यदि (!lSels.first && !rSels.first) फर्काऊ असत्य;

            अचर DfgVertex* अचर opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            यदि (opp->is<Bitwise>()) फर्काऊ असत्य;
            अचर स्वतः& sels = lSels.first ? lSels : rSels;
            DfgVertex* अचर nFromp = sels.first->fromp();
            DfgVertex* अचर wFromp = sels.second->fromp();
            अचर uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            अचर स्वतः matches = [&](अचर DfgVertex* termp, अचर स्वतः& tSels) {
                // Original form: 'n[i] OP w[j]'
                यदि (tSels.first) {
                    फर्काऊ termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                यदि (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                यदि (अचर DfgSel* अचर selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                यदि (termp->type() != opp->type()) फर्काऊ असत्य;
                // Operands might have been swapped since
                अचर Dfg_Vertex_द्विआधारी* अचर bwp = termp->as<Dfg_Vertex_द्विआधारी>();
                अचर तार्किक nIsLhs = bwp->lhsp() == nFromp;
                यदि (!nIsLhs && bwp->rhsp() != nFromp) फर्काऊ असत्य;
                अचर DfgSel* अचर slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                फर्काऊ slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            यदि (!matches(lhsp, lSels) || !matches(rTermp, rSels)) फर्काऊ असत्य;

            APPLYING(थप_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_पङ्क्ति* अचर flp = vtxp->fileline();
                अचर DfgDataType& dtype = nFromp->dtype();
                DfgSel* अचर slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                यदि (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } अन्यथा यदि (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } अन्यथा {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                अचर स्वतः थप = [&](DfgVertex* termp, अचर DfgSel* nSelp) -> DfgVertex* {
                    फर्काऊ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* अचर newLhsp = थप(lhsp, lSels.first);
                DfgVertex* अचर newRTermp = थप(rTermp, rSels.first);
                यदि (rChainp) {
                    DfgVertex* अचर newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } अन्यथा {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                फर्काऊ सत्य;
            }
        }

        फर्काऊ असत्य;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    स्थिर तार्किक isConcatFoldable(अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        यदि (isSame(lhsp, rhsp)) फर्काऊ सत्य;
        यदि (अचर DfgRep* अचर lRepp = lhsp->cast<DfgRep>()) {
            यदि (isSame(lRepp->srcp(), rhsp)) फर्काऊ सत्य;
        }
        यदि (अचर DfgRep* अचर rRepp = rhsp->cast<DfgRep>()) {
            यदि (isSame(lhsp, rRepp->srcp())) फर्काऊ सत्य;
        }
        // Concatenation of adjoining selects is a select
        यदि (अचर DfgSel* अचर lSelp = lhsp->cast<DfgSel>()) {
            यदि (अचर DfgSel* अचर rSelp = rhsp->cast<DfgSel>()) {
                फर्काऊ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        यदि (अचर DfgNot* अचर lNotp = lhsp->cast<DfgNot>()) {
            यदि (अचर DfgNot* अचर rNotp = rhsp->cast<DfgNot>()) {
                फर्काऊ !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        फर्काऊ असत्य;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    स्थिर तार्किक isSameBitwise(अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) {
        यदि (lhsp->type() != rhsp->type()) फर्काऊ असत्य;
        फर्काऊ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    स्थिर तार्किक isConcatSimplifiable(अचर DfgVertex* lhsp, अचर DfgVertex* rhsp) {
        यदि (isConcatFoldable(lhsp, rhsp)) फर्काऊ सत्य;
        यदि (!isSameBitwise(lhsp, rhsp)) फर्काऊ असत्य;
        यदि (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) फर्काऊ असत्य;
        अचर Dfg_Vertex_द्विआधारी* अचर lp = lhsp->as<Dfg_Vertex_द्विआधारी>();
        अचर Dfg_Vertex_द्विआधारी* अचर rp = rhsp->as<Dfg_Vertex_द्विआधारी>();
        यदि (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            फर्काऊ सत्य;
        यदि (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            फर्काऊ सत्य;
        फर्काऊ असत्य;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    स्थिर तार्किक isConcatPushableThroughBitwise(अचर DfgVertex* lhsp, अचर DfgVertex* rhsp,
                                               तार्किक& swap) {
        यदि (!isSameBitwise(lhsp, rhsp)) फर्काऊ असत्य;
        यदि (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) फर्काऊ असत्य;
        अचर Dfg_Vertex_द्विआधारी* अचर lp = lhsp->as<Dfg_Vertex_द्विआधारी>();
        अचर Dfg_Vertex_द्विआधारी* अचर rp = rhsp->as<Dfg_Vertex_द्विआधारी>();
        swap = असत्य;
        यदि (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) फर्काऊ सत्य;
        यदि (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) फर्काऊ सत्य;
        swap = सत्य;
        यदि (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) फर्काऊ सत्य;
        यदि (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) फर्काऊ सत्य;
        फर्काऊ असत्य;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_पङ्क्ति* flp, अचर DfgVertex* lhsp,
                                              अचर DfgVertex* rhsp, तार्किक swap) {
        अचर Dfg_Vertex_द्विआधारी* अचर lp = lhsp->as<Dfg_Vertex_द्विआधारी>();
        अचर Dfg_Vertex_द्विआधारी* अचर rp = rhsp->as<Dfg_Vertex_द्विआधारी>();
        अचर DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* अचर cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* अचर dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* अचर newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* अचर newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        यदि (lp->is<DfgAnd>()) फर्काऊ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        यदि (lp->is<DfgOr>()) फर्काऊ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        फर्काऊ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    ढाँचा <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();

        यदि (DfgSel* अचर lSelp = lhsp->ढाँचा cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            यदि (DfgSel* अचर selp = rhsp->ढाँचा cast<DfgSel>()) {
                rSelp = selp;
            } अन्यथा यदि (Bitwise* अचर rBitwisep = rhsp->ढाँचा cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->ढाँचा cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } अन्यथा यदि (Reduction* अचर rRedp = rhsp->ढाँचा cast<Reduction>()) {
                rSelp = rRedp->srcp()->ढाँचा cast<DfgSel>();
            }
            यदि (rSelp) {
                uint32_t lsb = 0;
                यदि (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        अचर DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* अचर newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        यदि (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        फर्काऊ सत्य;
                    }
                }
            }
        }

        यदि (Reduction* अचर lRedp = lhsp->ढाँचा cast<Reduction>()) {
            Reduction* rRedp = rhsp->ढाँचा cast<Reduction>();
            DfgVertex* extrap = nullptr;
            यदि (!rRedp) {
                यदि (Bitwise* अचर rBitwisep = rhsp->ढाँचा cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->ढाँचा cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            यदि (rRedp) {
                यदि (DfgSel* अचर lSelp = lRedp->srcp()->ढाँचा cast<DfgSel>()) {
                    यदि (DfgSel* अचर rSelp = rRedp->srcp()->ढाँचा cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        यदि (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                अचर DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* अचर newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                यदि (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                फर्काऊ सत्य;
                            }
                        }
                    }
                }
            }
        }

        फर्काऊ असत्य;
    }

    ढाँचा <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक optimizeReduction(Reduction* अचर vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        यदि (foldUnary(vtxp)) फर्काऊ सत्य;

        DfgVertex* अचर srcp = vtxp->srcp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        // Reduction of 1-bit value
        यदि (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                फर्काऊ सत्य;
            }
        }

        यदि (DfgCond* अचर condp = srcp->cast<DfgCond>()) {
            यदि (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(थप_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* अचर newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* अचर newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* अचर newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    फर्काऊ सत्य;
                }
            }
        }

        यदि (DfgConcat* अचर concatp = srcp->cast<DfgConcat>()) {
            यदि (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(थप_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* अचर lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* अचर rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    फर्काऊ सत्य;
                }
            }
        }

        यदि (Bitwise* अचर bitwisep = vtxp->srcp()->ढाँचा cast<Bitwise>()) {
            यदि (!bitwisep->hasMultipleSinks()) {
                यदि (bitwisep->lhsp()->ढाँचा is<DfgConcat>()
                    || bitwisep->rhsp()->ढाँचा is<DfgConcat>()) {
                    APPLYING(थप_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* अचर newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* अचर newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        फर्काऊ सत्य;
                    }
                }

                यदि (DfgSel* अचर lSelp = bitwisep->lhsp()->ढाँचा cast<DfgSel>()) {
                    यदि (DfgSel* अचर rSelp = bitwisep->rhsp()->ढाँचा cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        यदि (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(थप_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                अचर DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* अचर newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                फर्काऊ सत्य;
                            }
                        }
                    }
                }
            }
        }

        फर्काऊ असत्य;
    }

    ढाँचा <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक optimizeShiftRHS(Shift* अचर vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विआधारी, Shift>::मान, "Must invoke on binary");
        static_assert(std::is_final<Shift>::मान, "Must invoke on final class");
        यदि (अचर DfgConcat* अचर concatp = vtxp->rhsp()->ढाँचा cast<DfgConcat>()) {
            यदि (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    फर्काऊ सत्य;
                }
            }
        }
        फर्काऊ असत्य;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_गन्ती_Ones_Term(DfgVertex* vtxp) {
        यदि (DfgConcat* अचर oCatp = vtxp->cast<DfgConcat>()) {
            यदि (isZero(oCatp->lhsp())) {
                यदि (Dfg_गन्ती_Ones* अचर गन्ती_Onesp = oCatp->rhsp()->cast<Dfg_गन्ती_Ones>()) {
                    // Zero extended count ones
                    यदि (DfgSel* अचर selp = गन्ती_Onesp->srcp()->cast<DfgSel>()) {
                        फर्काऊ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } अन्यथा यदि (DfgSel* अचर selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    यदि (selp->dtype() == m_bitDType) {  //
                        फर्काऊ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            फर्काऊ {nullptr, 0, 0};
        }
        यदि (Dfg_गन्ती_Ones* अचर गन्ती_Onesp = vtxp->cast<Dfg_गन्ती_Ones>()) {
            // Simple count ones
            यदि (DfgSel* अचर selp = गन्ती_Onesp->srcp()->cast<DfgSel>()) {
                फर्काऊ {selp->fromp(), selp->lsb(), selp->width()};
            }
            फर्काऊ {nullptr, 0, 0};
        }
        यदि (DfgSel* अचर oSelp = vtxp->cast<DfgSel>()) {
            यदि (oSelp->lsb() == 0) {
                // Truncated count ones
                यदि (Dfg_गन्ती_Ones* अचर गन्ती_Onesp = oSelp->fromp()->cast<Dfg_गन्ती_Ones>()) {
                    // Zero extended count ones
                    यदि (DfgSel* अचर selp = गन्ती_Onesp->srcp()->cast<DfgSel>()) {
                        फर्काऊ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            यदि (oSelp->dtype() == m_bitDType) {  //
                फर्काऊ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            फर्काऊ {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        यदि (DfgShiftR* अचर shiftrp = vtxp->cast<DfgShiftR>()) {
            यदि (DfgConst* अचर rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                यदि (rConstp->toU32() == shiftrp->width() - 1) {
                    फर्काऊ {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            फर्काऊ {nullptr, 0, 0};
        }
        // Not applicable
        फर्काऊ {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              अचर uint32_t width) {
        जबसम्म (सत्य) {
            अचर uint32_t msb = lsb + width - 1;

            // Sel from Concat
            यदि (DfgConcat* अचर concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* अचर lhsp = concatp->lhsp();
                DfgVertex* अचर rhsp = concatp->rhsp();

                यदि (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        जारी;
                    }
                } अन्यथा यदि (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        जारी;
                    }
                }
            }

            यदि (DfgRep* अचर repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                अचर uint32_t srcWidth = repp->srcp()->width();
                यदि (width <= srcWidth) {
                    अचर uint32_t newLsb = lsb % srcWidth;
                    अचर uint32_t newMsb = newLsb + width - 1;
                    यदि (newMsb < srcWidth) {
                        APPLYING(थप_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            जारी;
                        }
                    }
                }
            }

            // Sel from Sel
            यदि (DfgSel* अचर selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    जारी;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            यदि (DfgVarPacked* अचर varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                अचर स्वतः pair = varp->driverOfRange(lsb, width);
                DfgVertex* अचर driverp = pair.first;
                अचर uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                यदि (driverp) {
                    यदि (driverp == varp->defaultp()) {
                        APPLYING(थप_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            जारी;
                        }
                    } अन्यथा {
                        APPLYING(थप_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            जारी;
                        }
                    }
                }
            }

            // No patterns matched, stop
            रोक;
        }
        फर्काऊ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        यदि (ap == bp) फर्काऊ {ap, ap->width()};

        // If both constants, check LSBs
        यदि (DfgConst* अचर aConstp = ap->cast<DfgConst>()) {
            यदि (DfgConst* अचर bConstp = bp->cast<DfgConst>()) {
                अचर V3_सङ्ख्या& a_अङ्क = aConstp->अङ्क();
                अचर V3_सङ्ख्या& b_अङ्क = bConstp->अङ्क();
                // Max match is the shorter constant
                अचर uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                लागि (; matchWidth < maxMatch; ++matchWidth) {
                    यदि (a_अङ्क.bitIs0(matchWidth) != b_अङ्क.bitIs0(matchWidth)) रोक;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* अचर shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                फर्काऊ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        यदि (DfgConcat* अचर catp = ap->cast<DfgConcat>()) फर्काऊ commonLSBs(catp->rhsp(), bp);
        यदि (DfgConcat* अचर catp = bp->cast<DfgConcat>()) फर्काऊ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        यदि (DfgSel* अचर selp = ap->cast<DfgSel>()) {
            यदि (selp->lsb() == 0) {
                DfgVertex* अचर fromp = selp->fromp();
                अचर std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                फर्काऊ {common.first, std::min(common.second, selp->width())};
            }
        }
        यदि (DfgSel* अचर selp = bp->cast<DfgSel>()) {
            यदि (selp->lsb() == 0) {
                DfgVertex* अचर fromp = selp->fromp();
                अचर std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                फर्काऊ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        फर्काऊ {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        यदि (ap == bp) फर्काऊ {ap, ap->width()};

        // If both constants, check MSBs
        यदि (DfgConst* अचर aConstp = ap->cast<DfgConst>()) {
            यदि (DfgConst* अचर bConstp = bp->cast<DfgConst>()) {
                अचर uint32_t aMsb = aConstp->width() - 1;
                अचर uint32_t bMsb = bConstp->width() - 1;
                अचर V3_सङ्ख्या& a_अङ्क = aConstp->अङ्क();
                अचर V3_सङ्ख्या& b_अङ्क = bConstp->अङ्क();
                // Max match is the shorter constant
                अचर uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                लागि (; matchWidth < maxMatch; ++matchWidth) {
                    यदि (a_अङ्क.bitIs0(aMsb - matchWidth) != b_अङ्क.bitIs0(bMsb - matchWidth)) रोक;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* अचर shorterp = aMsb < bMsb ? aConstp : bConstp;
                फर्काऊ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        यदि (DfgConcat* अचर catp = ap->cast<DfgConcat>()) फर्काऊ commonMSBs(catp->lhsp(), bp);
        यदि (DfgConcat* अचर catp = bp->cast<DfgConcat>()) फर्काऊ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        यदि (DfgSel* अचर selp = ap->cast<DfgSel>()) {
            DfgVertex* अचर fromp = selp->fromp();
            यदि (selp->msb() == fromp->width() - 1) {
                अचर std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                फर्काऊ {common.first, std::min(common.second, selp->width())};
            }
        }
        यदि (DfgSel* अचर selp = bp->cast<DfgSel>()) {
            DfgVertex* अचर fromp = selp->fromp();
            यदि (selp->msb() == fromp->width() - 1) {
                अचर std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                फर्काऊ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        फर्काऊ {nullptr, 0};
    }

    // VISIT methods

    शून्य visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    शून्य visit(Dfg_गन्ती_Ones* अचर vtxp) override {
        यदि (foldUnary(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgExtend* अचर vtxp) override {
        यदि (foldUnary(vtxp)) फर्काऊ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* अचर zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            फर्काऊ;
        }
    }

    शून्य visit(DfgExtendS* अचर vtxp) override {
        यदि (foldUnary(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgLogNot* अचर vtxp) override {
        यदि (foldUnary(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgNegate* अचर vtxp) override {
        यदि (foldUnary(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgNot* अचर vtxp) override {
        यदि (foldUnary(vtxp)) फर्काऊ;

        // Not of Cond
        यदि (DfgCond* अचर condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            यदि (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(थप_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* अचर newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* अचर newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* अचर newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    फर्काऊ;
                }
            }
        }

        // Not of Not
        यदि (DfgNot* अचर notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                फर्काऊ;
            }
        }

        यदि (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            यदि (DfgEq* अचर eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    फर्काऊ;
                }
            }

            // Not of Neq
            यदि (DfgNeq* अचर neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    फर्काऊ;
                }
            }

            // Not of unsigned comparisons
            यदि (DfgLt* अचर p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    फर्काऊ;
                }
            }
            यदि (DfgGte* अचर p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    फर्काऊ;
                }
            }
            यदि (DfgGt* अचर p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    फर्काऊ;
                }
            }
            यदि (DfgLte* अचर p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    फर्काऊ;
                }
            }

            // Not of signed comparisons
            यदि (DfgLtS* अचर p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    फर्काऊ;
                }
            }
            यदि (DfgGteS* अचर p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    फर्काऊ;
                }
            }
            यदि (DfgGtS* अचर p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    फर्काऊ;
                }
            }
            यदि (DfgLteS* अचर p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgOneHot* अचर vtxp) override {
        यदि (foldUnary(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgOneHot0* अचर vtxp) override {
        यदि (foldUnary(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgRedOr* अचर vtxp) override {
        यदि (optimizeReduction(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgRedAnd* अचर vtxp) override {
        यदि (optimizeReduction(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgRedXor* अचर vtxp) override {
        यदि (optimizeReduction(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgSel* अचर vtxp) override {
        DfgVertex* अचर fromp = vtxp->fromp();

        File_पङ्क्ति* अचर flp = vtxp->fileline();

        अचर uint32_t lsb = vtxp->lsb();
        अचर uint32_t width = vtxp->width();
        अचर uint32_t msb = lsb + width - 1;

        यदि (DfgConst* अचर constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* अचर resp = makeZero(flp, width);
                resp->अङ्क().opSel(constp->अङ्क(), msb, lsb);
                replace(resp);
                फर्काऊ;
            }
        }

        // Full width select, replace with the source.
        यदि (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                फर्काऊ;
            }
        }

        // Unwind through bit packing in one go
        {
            अचर स्वतः res = unwindSel(fromp, lsb, width);
            यदि (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                फर्काऊ;
            }
        }

        // Sel from Not
        यदि (DfgNot* अचर notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            यदि (!notp->hasMultipleSinks()) {
                APPLYING(थप_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* अचर newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    फर्काऊ;
                }
            }
        }

        // Sel from Cond
        यदि (DfgCond* अचर condp = fromp->cast<DfgCond>()) {
            यदि (!condp->hasMultipleSinks()) {
                APPLYING(थप_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* अचर newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* अचर newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* अचर newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    फर्काऊ;
                }
            }
        }

        // Sel from ShiftL
        यदि (DfgShiftL* अचर shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            यदि (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(थप_SEL_THROUGH_SHIFTL) {
                    DfgSel* अचर newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgMux* अचर vtxp) override {
        DfgVertex* अचर fromp = vtxp->fromp();
        DfgVertex* अचर lsbp = vtxp->lsbp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (DfgConst* अचर lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->अङ्क().toUInt()));
                फर्काऊ;
            }
        }

        यदि (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                फर्काऊ;
            }
        }

        यदि (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                फर्काऊ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    शून्य visit(DfgAnd* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        यदि (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            यदि (DfgNot* अचर lhsNotp = lhsp->cast<DfgNot>()) {
                यदि (DfgNot* अचर rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* अचर orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        फर्काऊ;
                    }
                }
                यदि (DfgNeq* अचर rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* अचर newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* अचर orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (DfgConst* अचर lConstp = lhsp->cast<DfgConst>()) {
            यदि (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    फर्काऊ;
                }
            }

            यदि (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    फर्काऊ;
                }
            }

            यदि (DfgConcat* अचर rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_थप_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) फर्काऊ;
            }
        }

        यदि (distributive_And_Associative_द्विआधारी<DfgOr, DfgAnd>(vtxp)) फर्काऊ;

        यदि (try_थप_Bitwise_Op_Through_Reductions(vtxp)) फर्काऊ;

        यदि (try_थप_Bitwise_Op_Throug_Sel(vtxp)) फर्काऊ;

        {
            DfgNot* अचर lNotp = lhsp->cast<DfgNot>();
            DfgNot* अचर rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            यदि ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    फर्काऊ;
                }
            }

            यदि (DfgAnd* अचर rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* अचर rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* अचर rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                यदि ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (vtxp->dtype() == m_bitDType) {
            यदि (tryReplaceBitwiseWithReduction(vtxp)) फर्काऊ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            यदि (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            यदि (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* अचर newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgOr* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        यदि (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            यदि (DfgNot* अचर lhsNotp = lhsp->cast<DfgNot>()) {
                यदि (DfgNot* अचर rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* अचर andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        फर्काऊ;
                    }
                }
                यदि (DfgNeq* अचर rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* अचर newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* अचर andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (DfgConcat* अचर lhsConcatp = lhsp->cast<DfgConcat>()) {
            यदि (DfgConcat* अचर rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    यदि (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            फर्काऊ;
                        }
                    }
                    यदि (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            फर्काऊ;
                        }
                    }
                }
            }
        }

        यदि (DfgConst* अचर lConstp = lhsp->cast<DfgConst>()) {
            यदि (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    फर्काऊ;
                }
            }

            यदि (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    फर्काऊ;
                }
            }

            यदि (DfgConcat* अचर rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_थप_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) फर्काऊ;
            }
        }

        यदि (distributive_And_Associative_द्विआधारी<DfgAnd, DfgOr>(vtxp)) फर्काऊ;

        यदि (try_थप_Bitwise_Op_Through_Reductions(vtxp)) फर्काऊ;

        यदि (try_थप_Bitwise_Op_Throug_Sel(vtxp)) फर्काऊ;

        {
            DfgNot* अचर lNotp = lhsp->cast<DfgNot>();
            DfgNot* अचर rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            यदि ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    फर्काऊ;
                }
            }

            यदि (DfgOr* अचर rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* अचर rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* अचर rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                यदि ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (vtxp->dtype() == m_bitDType) {
            यदि (tryReplaceBitwiseWithReduction(vtxp)) फर्काऊ;
        }
    }

    शून्य visit(DfgXor* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                फर्काऊ;
            }
        }

        यदि (DfgConst* अचर lConstp = lhsp->cast<DfgConst>()) {
            यदि (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    फर्काऊ;
                }
            }
            यदि (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    फर्काऊ;
                }
            }
            यदि (DfgConcat* अचर rConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_थप_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) फर्काऊ;
            }
        }

        यदि (try_थप_Bitwise_Op_Through_Reductions(vtxp)) फर्काऊ;

        यदि (try_थप_Bitwise_Op_Throug_Sel(vtxp)) फर्काऊ;

        यदि (vtxp->dtype() == m_bitDType) {
            यदि (tryReplaceBitwiseWithReduction(vtxp)) फर्काऊ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    शून्य visit(DfgAdd* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                फर्काऊ;
            }
        }

        अचर std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_गन्ती_Ones_Term(lhsp);
        यदि (DfgVertex* अचर lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_गन्ती_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            यदि (!std::get<0>(rTerm)) {
                यदि (DfgAdd* अचर rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_गन्ती_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            यदि (DfgVertex* अचर rVtxp = std::get<0>(rTerm)) {
                यदि (isSame(lVtxp, rVtxp)) {
                    अचर uint32_t lLsb = std::get<1>(lTerm);
                    अचर uint32_t rLsb = std::get<1>(rTerm);
                    अचर uint32_t lWidth = std::get<2>(lTerm);
                    अचर uint32_t rWidth = std::get<2>(rTerm);
                    तार्किक adjoined = सत्य;
                    uint32_t lsb = 0;
                    यदि (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } अन्यथा यदि (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } अन्यथा {
                        adjoined = असत्य;
                    }
                    यदि (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_गन्ती_ONES) {
                            DfgSel* अचर selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_गन्ती_Ones>(flp, DfgDataType::packed(32), selp);
                            यदि (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } अन्यथा यदि (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            यदि (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            फर्काऊ;
                        }
                    }
                }
            }
        }
    }

    शून्य visit(DfgArraySel* अचर vtxp) override {
        DfgConst* अचर idxp = vtxp->bitp()->cast<DfgConst>();
        यदि (!idxp) फर्काऊ;
        DfgVarArray* अचर varp = vtxp->fromp()->cast<DfgVarArray>();
        यदि (!varp) फर्काऊ;
        AstVar* अचर astVarp = varp->vscp()->varp();
        यदि (astVarp->isForced()) फर्काऊ;
        यदि (astVarp->isSigUserRWPublic()) फर्काऊ;
        DfgVertex* अचर srcp = varp->srcp();
        यदि (!srcp) {
            यदि (vtxp->isPacked()) {
                यदि (AstConsArray* अचर iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    यदि (AstConst* अचर valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(नूतन DfgConst{m_dfg, valp->fileline(), valp->अङ्क()});
                            फर्काऊ;
                        }
                    }
                }
            }
            फर्काऊ;
        }

        यदि (DfgSpliceArray* अचर splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* अचर driverp = splicep->driverAt(idxp->toSizeT());
            यदि (!driverp) फर्काऊ;
            DfgUnitArray* अचर uap = driverp->cast<DfgUnitArray>();
            यदि (!uap) फर्काऊ;
            यदि (uap->srcp()->is<DfgVertexSplice>()) फर्काऊ;
            // If driven by a variable that had a Driver in DFG, it is partial
            यदि (DfgVertexVar* अचर dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                यदि (dvarp->srcp()) फर्काऊ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                फर्काऊ;
            }
        }

        यदि (DfgUnitArray* अचर uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            यदि (uap->srcp()->is<DfgSplicePacked>()) फर्काऊ;
            // If driven by a variable that had a Driver in DFG, it is partial
            यदि (DfgVertexVar* अचर dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                यदि (dvarp->srcp()) फर्काऊ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgConcat* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();

        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isZero(lhsp)) {
            DfgConst* अचर lConstp = lhsp->as<DfgConst>();
            यदि (DfgSel* अचर rSelp = rhsp->cast<DfgSel>()) {
                यदि (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_शीर्ष_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (isZero(rhsp)) {
            DfgConst* अचर rConstp = rhsp->as<DfgConst>();
            यदि (DfgSel* अचर lSelp = lhsp->cast<DfgSel>()) {
                यदि (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (DfgNot* अचर lNot = lhsp->cast<DfgNot>()) {
            यदि (DfgNot* अचर rNot = rhsp->cast<DfgNot>()) {
                यदि (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(थप_CONCAT_THROUGH_NOTS) {
                        DfgConcat* अचर newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        फर्काऊ;
                    }
                }
            }
        }

        {
            तार्किक swap = असत्य;
            यदि (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(थप_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    फर्काऊ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            यदि (DfgConcat* अचर rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* अचर rlp = rConcatp->lhsp();
                यदि (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(थप_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* अचर newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (DfgSel* अचर lSelp = lhsp->cast<DfgSel>()) {
            यदि (DfgSel* अचर rSelp = rhsp->cast<DfgSel>()) {
                यदि (isSame(lSelp->fromp(), rSelp->fromp())) {
                    यदि (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            फर्काऊ;
                        }
                    }
                }
            }

            यदि (DfgConcat* अचर rConcatp = rhsp->cast<DfgConcat>()) {
                यदि (DfgSel* अचर rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    यदि (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        यदि (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                अचर uint32_t width = lSelp->width() + rlSelp->width();
                                अचर DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* अचर selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                फर्काऊ;
                            }
                        }
                    }
                }
            }
        }

        यदि (DfgSel* अचर rSelp = rhsp->cast<DfgSel>()) {
            यदि (DfgConcat* अचर lConcatp = lhsp->cast<DfgConcat>()) {
                यदि (DfgSel* अचर lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    यदि (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        यदि (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                अचर uint32_t width = lrSelp->width() + rSelp->width();
                                अचर DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* अचर selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                फर्काऊ;
                            }
                        }
                    }
                }
            }
        }

        यदि (DfgConst* अचर lConstp = lhsp->cast<DfgConst>()) {
            यदि (DfgCond* अचर rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* अचर rtVtxp = rCondp->thenp();
                DfgVertex* अचर reVtxp = rCondp->elsep();
                DfgConst* अचर rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* अचर reConstp = reVtxp->cast<DfgConst>();
                यदि (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(थप_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* अचर thenp = [&]() -> DfgVertex* {
                            File_पङ्क्ति* अचर rtFlp = rtVtxp->fileline();
                            यदि (rtConstp) {
                                DfgConst* अचर constp = makeZero(rtFlp, vtxp->width());
                                constp->अङ्क().opConcat(lConstp->अङ्क(), rtConstp->अङ्क());
                                फर्काऊ constp;
                            }
                            फर्काऊ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* अचर elsep = [&]() -> DfgVertex* {
                            File_पङ्क्ति* अचर reFlp = reVtxp->fileline();
                            यदि (reConstp) {
                                DfgConst* अचर constp = makeZero(reFlp, vtxp->width());
                                constp->अङ्क().opConcat(lConstp->अङ्क(), reConstp->अङ्क());
                                फर्काऊ constp;
                            }
                            फर्काऊ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (DfgConst* अचर rConstp = rhsp->cast<DfgConst>()) {
            यदि (DfgCond* अचर lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* अचर ltVtxp = lCondp->thenp();
                DfgVertex* अचर leVtxp = lCondp->elsep();
                DfgConst* अचर ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* अचर leConstp = leVtxp->cast<DfgConst>();
                यदि (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(थप_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* अचर thenp = [&]() -> DfgVertex* {
                            File_पङ्क्ति* अचर ltFlp = ltVtxp->fileline();
                            यदि (ltConstp) {
                                DfgConst* अचर constp = makeZero(ltFlp, vtxp->width());
                                constp->अङ्क().opConcat(ltConstp->अङ्क(), rConstp->अङ्क());
                                फर्काऊ constp;
                            }
                            फर्काऊ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* अचर elsep = [&]() -> DfgVertex* {
                            File_पङ्क्ति* अचर leFlp = leVtxp->fileline();
                            यदि (leConstp) {
                                DfgConst* अचर constp = makeZero(leFlp, vtxp->width());
                                constp->अङ्क().opConcat(leConstp->अङ्क(), rConstp->अङ्क());
                                फर्काऊ constp;
                            }
                            फर्काऊ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        फर्काऊ;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            अचर uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            अचर uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            अचर uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            तार्किक hasCrossSink = असत्य;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                यदि (अचर DfgSel* अचर selp = sink.cast<DfgSel>()) {
                    अचर uint32_t selLsb = selp->lsb();
                    अचर uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    फर्काऊ असत्य;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                यदि (अचर DfgVarPacked* अचर varp = sink.cast<DfgVarPacked>()) {
                    यदि (!varp->hasSinks() && !varp->isObserved()) फर्काऊ असत्य;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                फर्काऊ सत्य;
            });
            यदि (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    यदि (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    यदि (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* अचर catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* अचर sp = नूतन DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* अचर varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(सत्य);
                    varp->srcp(sp);
                    replace(varp);
                    फर्काऊ;
                }
            }
        }

        // Convert various forms to REPLICATE
        यदि (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                फर्काऊ;
            }
        }
        यदि (DfgRep* अचर lRepp = lhsp->cast<DfgRep>()) {
            यदि (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    फर्काऊ;
                }
            }
        }
        यदि (DfgRep* अचर rRepp = rhsp->cast<DfgRep>()) {
            यदि (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    फर्काऊ;
                }
            }
        }
        यदि (DfgRep* अचर lRepp = lhsp->cast<DfgRep>()) {
            यदि (DfgRep* अचर rRepp = rhsp->cast<DfgRep>()) {
                यदि (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        फर्काऊ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            यदि (DfgConcat* अचर rConcatp = rhsp->cast<DfgConcat>()) {
                यदि (DfgRep* अचर rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    यदि (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            अचर uint32_t width = lRepp->width() + rlRepp->width();
                            अचर DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* अचर repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            फर्काऊ;
                        }
                    }
                }
            }
        }
    }

    शून्य visit(DfgDiv* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        यदि (DfgConst* अचर rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            यदि (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgDivS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        यदि (DfgConst* अचर rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            यदि (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgGt* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgGtS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgGte* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgGteS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgLogAnd* अचर vtxp) override {
        यदि (द्विआधारी(vtxp) || vtxp->rhsp()->unsafe()) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();

        यदि (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgLogEq* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgLogIf* अचर vtxp) override {
        यदि (द्विआधारी(vtxp) || vtxp->rhsp()->unsafe()) फर्काऊ;
    }

    शून्य visit(DfgLogOr* अचर vtxp) override {
        यदि (द्विआधारी(vtxp) || vtxp->rhsp()->unsafe()) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();

        यदि (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgLt* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgLtS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgLte* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgLteS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                फर्काऊ;
            }
        }
    }

    शून्य visit(DfgModDiv* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgModDivS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgMul* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (DfgConst* अचर lConstp = lhsp->cast<DfgConst>()) {
            यदि (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    फर्काऊ;
                }
            }
            यदि (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgMulS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (DfgConst* अचर lConstp = lhsp->cast<DfgConst>()) {
            यदि (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    फर्काऊ;
                }
            }
            यदि (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgEq* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                फर्काऊ;
            }
        }

        यदि (DfgConst* अचर lhsConstp = lhsp->cast<DfgConst>()) {
            यदि (DfgConcat* अचर rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_थप_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) फर्काऊ;
            }

            यदि (rhsp->dtype() == m_bitDType) {
                यदि (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        फर्काऊ;
                    }
                }

                यदि (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        फर्काऊ;
                    }
                }
            }
        }
    }

    शून्य visit(DfgNeq* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                फर्काऊ;
            }
        }

        यदि (DfgConst* अचर lhsConstp = lhsp->cast<DfgConst>()) {
            यदि (DfgConcat* अचर rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_थप_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) फर्काऊ;
            }

            यदि (rhsp->dtype() == m_bitDType) {
                यदि (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        फर्काऊ;
                    }
                }

                यदि (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        फर्काऊ;
                    }
                }
            }
        }
    }

    शून्य visit(DfgPow* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgPowSS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgPowSU* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgPowUS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgRep* अचर vtxp) override {
        DfgVertex* अचर srcp = vtxp->srcp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (DfgConst* अचर sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* अचर resultp = makeZero(flp, vtxp->width());
                resultp->अङ्क().opRepl(sConstp->अङ्क(), vtxp->गन्ती());
                replace(resultp);
                फर्काऊ;
            }
        }

        यदि (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                फर्काऊ;
            }
        }

        यदि (DfgRep* अचर sRepp = srcp->cast<DfgRep>()) {
            यदि (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgShiftL* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
        यदि (optimizeShiftRHS(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();

        यदि (DfgShiftL* अचर lShiftLp = lhsp->cast<DfgShiftL>()) {
            यदि (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_पङ्क्ति* अचर flp = vtxp->fileline();
                    अचर DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* अचर bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* अचर cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* अचर addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    फर्काऊ;
                }
            }
        }

        यदि (DfgConst* अचर rConstp = rhsp->cast<DfgConst>()) {
            अचर uint32_t shiftAmount = rConstp->toU32();

            यदि (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    फर्काऊ;
                }
            }

            यदि (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    फर्काऊ;
                }
            }

            यदि (DfgConcat* अचर lConcatp = lhsp->cast<DfgConcat>()) {
                यदि (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* अचर lRhsp = lConcatp->rhsp();
                        DfgVertex* अचर lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        अचर uint32_t rW = shiftAmount;
                        अचर uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        अचर uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_पङ्क्ति* अचर flp = lConcatp->fileline();
                        DfgVertex* अचर rp = makeZero(flp, shiftAmount);
                        DfgVertex* अचर mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        यदि (!lW) {
                            replace(np);
                            फर्काऊ;
                        }
                        DfgVertex* अचर lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        फर्काऊ;
                    }
                }
            }

            यदि (DfgSel* अचर lSelp = lhsp->cast<DfgSel>()) {
                यदि (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        अचर uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* अचर nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        फर्काऊ;
                    }
                }
            }

            यदि (DfgCond* अचर lCondp = lhsp->cast<DfgCond>()) {
                यदि (!lCondp->hasMultipleSinks()) {
                    APPLYING(थप_SHIFTL_THROUGH_COND) {
                        DfgShiftL* अचर tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* अचर ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        फर्काऊ;
                    }
                }
            }
        }
    }

    शून्य visit(DfgShiftR* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
        यदि (optimizeShiftRHS(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();

        यदि (DfgShiftR* अचर lShiftRp = lhsp->cast<DfgShiftR>()) {
            यदि (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_पङ्क्ति* अचर flp = vtxp->fileline();
                    अचर DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* अचर bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* अचर cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* अचर addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    फर्काऊ;
                }
            }
        }

        यदि (DfgConst* अचर rConstp = rhsp->cast<DfgConst>()) {
            अचर uint32_t shiftAmount = rConstp->toU32();

            यदि (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    फर्काऊ;
                }
            }

            यदि (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    फर्काऊ;
                }
            }

            यदि (DfgConcat* अचर lConcatp = lhsp->cast<DfgConcat>()) {
                यदि (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* अचर lRhsp = lConcatp->rhsp();
                        DfgVertex* अचर lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        अचर uint32_t lW = shiftAmount;
                        अचर uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        अचर uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_पङ्क्ति* अचर flp = lConcatp->fileline();
                        DfgVertex* अचर lp = makeZero(flp, shiftAmount);
                        DfgVertex* अचर mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        यदि (!rW) {
                            replace(np);
                            फर्काऊ;
                        }
                        DfgVertex* अचर rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        फर्काऊ;
                    }
                }
            }

            यदि (DfgSel* अचर lSelp = lhsp->cast<DfgSel>()) {
                यदि (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        अचर uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* अचर nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        फर्काऊ;
                    }
                }
            }

            यदि (DfgCond* अचर lCondp = lhsp->cast<DfgCond>()) {
                यदि (!lCondp->hasMultipleSinks()) {
                    APPLYING(थप_SHIFTR_THROUGH_COND) {
                        DfgShiftR* अचर tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* अचर ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        फर्काऊ;
                    }
                }
            }
        }
    }

    शून्य visit(DfgShiftRS* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;
        यदि (optimizeShiftRHS(vtxp)) फर्काऊ;
    }

    शून्य visit(DfgSub* अचर vtxp) override {
        यदि (द्विआधारी(vtxp)) फर्काऊ;

        DfgVertex* अचर lhsp = vtxp->lhsp();
        DfgVertex* अचर rhsp = vtxp->rhsp();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                फर्काऊ;
            }
        }

        यदि (DfgConst* अचर rConstp = rhsp->cast<DfgConst>()) {
            यदि (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    फर्काऊ;
                }
            }
            यदि (vtxp->dtype() == m_bitDType && rConstp->has_मान(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgMatchMasked* अचर vtxp) override {
        यदि (DfgConst* अचर constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* अचर matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->अङ्क(), matchVarp)));
                फर्काऊ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    शून्य visit(DfgCond* अचर vtxp) override {
        DfgVertex* अचर condp = vtxp->condp();
        DfgVertex* अचर thenp = vtxp->thenp();
        DfgVertex* अचर elsep = vtxp->elsep();
        File_पङ्क्ति* अचर flp = vtxp->fileline();

        यदि (condp->dtype() != m_bitDType) फर्काऊ;

        यदि (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                फर्काऊ;
            }
        }

        यदि (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                फर्काऊ;
            }
        }

        यदि (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                फर्काऊ;
            }
        }

        यदि (DfgNot* अचर condNotp = condp->cast<DfgNot>()) {
            यदि (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    फर्काऊ;
                }
            }
        }

        यदि (DfgNeq* अचर condNeqp = condp->cast<DfgNeq>()) {
            यदि (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* अचर newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    फर्काऊ;
                }
            }
        }

        यदि (DfgNot* अचर thenNotp = thenp->cast<DfgNot>()) {
            यदि (DfgNot* अचर elseNotp = elsep->cast<DfgNot>()) {
                यदि (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* अचर newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (DfgOr* अचर condOrp = condp->cast<DfgOr>()) {
            यदि (DfgCond* अचर thenCondp = thenp->cast<DfgCond>()) {
                यदि (!thenCondp->hasMultipleSinks()) {
                    यदि (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* अचर ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            फर्काऊ;
                        }
                    }
                    यदि (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* अचर ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            फर्काऊ;
                        }
                    }
                }
            }
        }

        यदि (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            यदि (DfgAdd* अचर thenAddp = thenp->cast<DfgAdd>()) {
                यदि (DfgConst* अचर constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    यदि (constp->has_मान(1)) {
                        यदि (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* अचर extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_पङ्क्ति* अचर thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                फर्काऊ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            यदि (DfgSub* अचर thenSubp = thenp->cast<DfgSub>()) {
                यदि (DfgConst* अचर constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    यदि (constp->has_मान(1)) {
                        यदि (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* अचर extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_पङ्क्ति* अचर thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                फर्काऊ;
                            }
                        }
                    }
                }
            }
        }

        यदि (vtxp->dtype() == m_bitDType) {
            यदि (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    फर्काऊ;
                }
            }
            यदि (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    फर्काऊ;
                }
            }
        }

        यदि (vtxp->width() <= VL_QUADSIZE) {
            यदि (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* अचर maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    फर्काऊ;
                }
            }
            यदि (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* अचर maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    फर्काऊ;
                }
            }
            यदि (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* अचर maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    फर्काऊ;
                }
            }
            यदि (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* अचर maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    फर्काऊ;
                }
            }

            यदि (DfgOr* अचर tOrp = thenp->cast<DfgOr>()) {
                यदि (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* अचर maskp = replicate(vtxp, condp);
                        DfgAnd* अचर andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        फर्काऊ;
                    }
                }
                यदि (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* अचर maskp = replicate(vtxp, condp);
                        DfgAnd* अचर andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        फर्काऊ;
                    }
                }
            }
        }

        यदि (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            अचर std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            यदि (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    अचर uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    यदि (rWidth != rhsp->width()) {
                        अचर DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    यदि (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        फर्काऊ;
                    }
                    // Create new LHS
                    अचर uint32_t lWidth = vtxp->width() - rWidth;
                    अचर DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* अचर lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* अचर lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* अचर lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    फर्काऊ;
                }
            }

            अचर std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            यदि (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    अचर uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    यदि (lWidth != lhsp->width()) {
                        अचर DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    यदि (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        फर्काऊ;
                    }
                    // Create new RHS
                    अचर uint32_t rWidth = vtxp->width() - lWidth;
                    अचर DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* अचर rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* अचर rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* अचर rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    फर्काऊ;
                }
            }
        }

        यदि (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                यदि (अचर uint32_t extend = vtxp->width() - 1) {
                    DfgConst* अचर zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                फर्काऊ;
            }
        }

        यदि (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                यदि (अचर uint32_t extend = vtxp->width() - 1) {
                    DfgConst* अचर zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                फर्काऊ;
            }
        }

        यदि (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                फर्काऊ;
            }
        }

        यदि (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                फर्काऊ;
            }
        }

        यदि (DfgCond* अचर tCondp = thenp->cast<DfgCond>()) {
            यदि (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    फर्काऊ;
                }
            }
        }

        यदि (DfgCond* अचर eCondp = elsep->cast<DfgCond>()) {
            यदि (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    फर्काऊ;
                }
            }
        }
    }

    शून्य visit(DfgVertexVar* अचर vtxp) override {
        यदि (vtxp->hasSinks()) फर्काऊ;
        यदि (vtxp->isObserved()) फर्काऊ;
        यदि (vtxp->defaultp()) फर्काऊ;

        // If undriven, or driven from another var, it is completely redundant.
        यदि (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                फर्काऊ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        तार्किक foundOne = असत्य;
        अचर तार्किक keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> तार्किक {
            // Ignore non-observable variable sinks. These can be eliminated.
            यदि (अचर DfgVertexVar* अचर varp = sink.cast<DfgVertexVar>()) {
                यदि (!varp->hasSinks() && !varp->isObserved()) फर्काऊ असत्य;
            }
            // Keep if found more than one sink
            यदि (foundOne) फर्काऊ सत्य;
            // Mark first sink found
            foundOne = सत्य;
            फर्काऊ असत्य;
        });
        यदि (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                फर्काऊ;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        लागि (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.थप_back(nullptr);
        m_iterList.थप_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        लागि (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        लागि (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        जबसम्म (सत्य) {
            // Process the work list - keep the placeholder at index 0
            जबसम्म (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.झिक_back();
                यदि (!m_vtxp) जारी;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                यदि (DfgVertexVar* अचर varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    जारी;
                }

                // Ast references are not considered
                यदि (m_vtxp->is<DfgVertexAst>()) जारी;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                यदि (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    यदि (DfgVertex* अचर sampep = cacheVertex(m_vtxp)) {
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
            यदि (m_iterList.size() == 1) रोक;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                माप_प्रकार begin = 0;
                माप_प्रकार end = m_iterList.size();
                लागि (माप_प्रकार hops = 1; hops <= 4; ++hops) {
                    लागि (माप_प्रकार i = begin; i < end; ++i) {
                        DfgVertex* अचर vtxp = m_iterList[i];
                        यदि (!vtxp) जारी;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            फर्काऊ असत्य;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            फर्काऊ असत्य;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            लागि (DfgVertex* अचर vtxp : m_iterList) {
                यदि (!vtxp) जारी;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

सार्वजनिक:
    स्थिर शून्य apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

शून्य V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    यदि (!v3Global.opt.fDfgPeephole()) फर्काऊ;
    V3DfgPeephole::apply(dfg, ctx);
}
