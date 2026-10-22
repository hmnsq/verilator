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

ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Reduction>
ರಚನೆ Reduction_ಗಮ್ಯ_Bitwise_Impl {};
ಮಾದರಿ <> ರಚನೆ Reduction_ಗಮ್ಯ_Bitwise_Impl<DfgRedAnd> { ಬಳಸಿ type = DfgAnd; };
ಮಾದರಿ <> ರಚನೆ Reduction_ಗಮ್ಯ_Bitwise_Impl<DfgRedOr>  { ಬಳಸಿ type = DfgOr;  };
ಮಾದರಿ <> ರಚನೆ Reduction_ಗಮ್ಯ_Bitwise_Impl<DfgRedXor> { ಬಳಸಿ type = DfgXor; };
ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Reduction>
ಬಳಸಿ Reduction_ಗಮ್ಯ_Bitwise = ಬಗೆ_ಹೆಸರು Reduction_ಗಮ್ಯ_Bitwise_Impl<T_Reduction>::type;

ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Bitwise>
ರಚನೆ Bitwise_ಗಮ್ಯ_Reduction_Impl {};
ಮಾದರಿ <> ರಚನೆ Bitwise_ಗಮ್ಯ_Reduction_Impl<DfgAnd> { ಬಳಸಿ type = DfgRedAnd; };
ಮಾದರಿ <> ರಚನೆ Bitwise_ಗಮ್ಯ_Reduction_Impl<DfgOr>  { ಬಳಸಿ type = DfgRedOr;  };
ಮಾದರಿ <> ರಚನೆ Bitwise_ಗಮ್ಯ_Reduction_Impl<DfgXor> { ಬಳಸಿ type = DfgRedXor; };
ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Reduction>
ಬಳಸಿ Bitwise_ಗಮ್ಯ_Reduction = ಬಗೆ_ಹೆಸರು Bitwise_ಗಮ್ಯ_Reduction_Impl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
ಮಾದರಿ<ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
ರಚನೆ IsAssociative : ಸಾರ್ವಜನಿಕ std::false_type {};
ಮಾದರಿ <> ರಚನೆ IsAssociative<DfgAnd> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsAssociative<DfgOr>  : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsAssociative<DfgXor> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsAssociative<Dfg_ಕೂಡಿಸು> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsAssociative<DfgConcat> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsAssociative<DfgMul> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsAssociative<DfgMulS> : ಸಾರ್ವಜನಿಕ std::true_type {};

// Commutative binary operators (a op b == b op a)
ಮಾದರಿ<ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
ರಚನೆ IsCommutative : ಸಾರ್ವಜನಿಕ std::false_type {};
ಮಾದರಿ <> ರಚನೆ IsCommutative<DfgAnd> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsCommutative<DfgOr>  : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsCommutative<DfgXor> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsCommutative<Dfg_ಕೂಡಿಸು> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsCommutative<DfgEq> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsCommutative<DfgMul> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsCommutative<DfgMulS> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsCommutative<DfgNeq> : ಸಾರ್ವಜನಿಕ std::true_type {};

// Idempotent binary operators (a op a == a)
ಮಾದರಿ<ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
ರಚನೆ IsIdempotent : ಸಾರ್ವಜನಿಕ std::false_type {};
ಮಾದರಿ <> ರಚನೆ IsIdempotent<DfgAnd> : ಸಾರ್ವಜನಿಕ std::true_type {};
ಮಾದರಿ <> ರಚನೆ IsIdempotent<DfgOr>  : ಸಾರ್ವಜನಿಕ std::true_type {};

// Binary result dtype
ಮಾದರಿ<ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp);
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<Dfg_ಕೂಡಿಸು>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgAnd>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgConcat>     (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(lhsp->ಅಗಲ() + rhsp->ಅಗಲ()); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgDiv>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgDivS>       (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgEq>         (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgGt>         (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgGtS>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgGte>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgGteS>       (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgLogAnd>     (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgLogEq>      (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgLogIf>      (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgLogOr>      (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgLt>         (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgLtS>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgLte>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgLteS>       (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgModDiv>     (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgModDivS>    (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgMul>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgMulS>       (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgNeq>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ Dfg_ದತ್ತಾಂಶ_Type::packed(1); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgOr>         (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgPow>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgPowSS>      (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgPowSU>      (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgPowUS>      (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<Dfg_ಸರಿಸು_L>     (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<Dfg_ಸರಿಸು_R>     (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<Dfg_ಸರಿಸು_RS>    (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgSub>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }
ಮಾದರಿ <> ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಫಲಿತಾಂಶ_DType<DfgXor>        (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) { ಹಿಂತಿರುಗಿ lhsp->dtype(); }

// Unary constant folding
ಮಾದರಿ<ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2> ಶೂನ್ಯ foldOp(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src);
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<Dfg_ಎಣಿಕೆ_Ones>  (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.op_ಎಣಿಕೆ_Ones(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgExtend>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgExtendS>    (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opExtendS(src, src.ಅಗಲ()); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLogNot>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opLogNot(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgNegate>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opNegate(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgNot>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opNot(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgOneHot>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opOneHot(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgOneHot0>    (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opOneHot0(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgRedAnd>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opRedAnd(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgRedOr>      (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opRedOr(src); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgRedXor>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& src) { ಹೊರಹೋಗು.opRedXor(src); }

// Binary constant folding
ಮಾದರಿ<ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2> ಶೂನ್ಯ foldOp(V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<Dfg_ಕೂಡಿಸು>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.op_ಕೂಡಿಸು(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgAnd>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opAnd(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgConcat>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opConcat(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgDiv>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opDiv(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgDivS>       (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opDivS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgEq>         (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opEq(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgGt>         (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opGt(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgGtS>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opGtS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgGte>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opGte(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgGteS>       (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opGteS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLogAnd>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opLogAnd(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLogEq>      (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opLogEq(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLogIf>      (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opLogIf(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLogOr>      (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opLogOr(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLt>         (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opLt(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLtS>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opLtS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLte>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opLte(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgLteS>       (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opLteS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgModDiv>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opModDiv(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgModDivS>    (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opModDivS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgMul>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opMul(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgMulS>       (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opMulS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgNeq>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opNeq(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgOr>         (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opOr(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgPow>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opPow(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgPowSS>      (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opPowSS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgPowSU>      (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opPowSU(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgPowUS>      (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opPowUS(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<Dfg_ಸರಿಸು_L>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.op_ಸರಿಸು_L(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<Dfg_ಸರಿಸು_R>     (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.op_ಸರಿಸು_R(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<Dfg_ಸರಿಸು_RS>    (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.op_ಸರಿಸು_RS(lhs, rhs, lhs.ಅಗಲ()); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgSub>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opSub(lhs, rhs); }
ಮಾದರಿ <> ಶೂನ್ಯ foldOp<DfgXor>        (V3_ಸಂಖ್ಯೆ& ಹೊರಹೋಗು, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹೊರಹೋಗು.opXor(lhs, rhs); }
}
// clang-format on

ವರ್ಗ V3DfgPeephole final : ಸಾರ್ವಜನಿಕ DfgVisitor {
    // TYPES
    ರಚನೆ ಶೃಂಗ_Info final {
        ಗಾತ್ರ_t m_work_List_ಸೂಚ್ಯಂಕ = 0;  // Position of this vertx m_workList (0 means not in list)
        ಗಾತ್ರ_t m_iter_List_ಸೂಚ್ಯಂಕ = 0;  // Position of this vertx m_iterList (0 means not in list)
        ಗಾತ್ರ_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        ಗಾತ್ರ_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        ತಾರ್ಕಿಕ m_is_Cached_ಶೃಂಗ = ಸುಳ್ಳು;  // This vertex is the vertex cached for its operation
    };

    // STATE
    Dfg_ನಕ್ಷೆ& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& m_ಬಿಟ್_DType = Dfg_ದತ್ತಾಂಶ_Type::packed(1);  // Common, so grab it up front
    std::ಸದಿಶ<Dfg_ಶೃಂಗ*> m_workList;  // List of vertices processed in current interation
    std::ಸದಿಶ<Dfg_ಶೃಂಗ*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<ಶೃಂಗ_Info> m_vInfo = m_dfg.makeUserMap<ಶೃಂಗ_Info>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    Dfg_ಶೃಂಗ* m_vtxp = ಶೂನ್ಯಸೂಚಕ;  // Currently considered vertex
    ಗಾತ್ರ_t m_currentGeneration = 0;  // Current generation number
    ಗಾತ್ರ_t m_ಕೊನೆಯ_Id = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ಸ್ಥಿರಾಂಕ m_tmpScopep = v3Global.rootp()->ತುದಿ_Scopep()->scopep();

    // STATIC STATE
    ಸ್ಥಿರ V3DebugBisect s_debugBisect;  // Debug aid

#ವ್ಯಾಖ್ಯಾನಿಸು APPLYING(id) ಆದರೆ (checkApplying(VDfg_Peephole_ನಮೂನೆ::id))

    // METHODS
    ತಾರ್ಕಿಕ checkApplying(VDfg_Peephole_ನಮೂನೆ id) {
        ಆದರೆ (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_ಎಣಿಕೆ[id];
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    Dfg_ಶೃಂಗ* cache_ಶೃಂಗ(Dfg_ಶೃಂಗ* vtxp) {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_is_Cached_ಶೃಂಗ || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_is_Cached_ಶೃಂಗ = !equivp;
        ಹಿಂತಿರುಗಿ equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    ಶೂನ್ಯ invalidate_ಶೃಂಗ(Dfg_ಶೃಂಗ* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_is_Cached_ಶೃಂಗ = ಸುಳ್ಳು;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2, ಬಗೆ_ಹೆಸರು... Operands>
    ಶೃಂಗ_2* get_Or_Create_ಶೃಂಗ(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, Operands... operands) {
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp = m_cache.getOrCreate<ಶೃಂಗ_2, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_is_Cached_ಶೃಂಗ = ನಿಜ;
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2, ಬಗೆ_ಹೆಸರು... Operands>
    ಶೃಂಗ_2* get_ಶೃಂಗ(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, Operands... operands) {
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp = m_cache.get<ಶೃಂಗ_2>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_is_Cached_ಶೃಂಗ, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ಹಿಂತಿರುಗಿ vtxp;
    }

    ಶೂನ್ಯ incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಗಮ್ಯ_Work_List(Dfg_ಶೃಂಗ* vtxp) {
        ಶೃಂಗ_Info& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        ಆದರೆ (vInfo.m_work_List_ಸೂಚ್ಯಂಕ) ಹಿಂತಿರುಗಿ;
        // Add to work list
        vInfo.m_work_List_ಸೂಚ್ಯಂಕ = m_workList.ಗಾತ್ರ_2();
        m_workList.ತಳ್ಳು_ಹಿಂಭಾಗ(vtxp);
    }

    ಶೂನ್ಯ removeFromWorkList(Dfg_ಶೃಂಗ* vtxp) {
        ಶೃಂಗ_Info& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_work_List_ಸೂಚ್ಯಂಕ] = ಶೂನ್ಯಸೂಚಕ;
        vInfo.m_work_List_ಸೂಚ್ಯಂಕ = 0;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_ಗಮ್ಯ_Iter_List(Dfg_ಶೃಂಗ* vtxp) {
        ಶೃಂಗ_Info& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        ಆದರೆ (vInfo.m_iter_List_ಸೂಚ್ಯಂಕ) ಹಿಂತಿರುಗಿ;
        // Add to iter list
        vInfo.m_iter_List_ಸೂಚ್ಯಂಕ = m_iterList.ಗಾತ್ರ_2();
        m_iterList.ತಳ್ಳು_ಹಿಂಭಾಗ(vtxp);
    }

    ಶೂನ್ಯ removeFromIterList(Dfg_ಶೃಂಗ* vtxp) {
        ಶೃಂಗ_Info& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iter_List_ಸೂಚ್ಯಂಕ] = ಶೂನ್ಯಸೂಚಕ;
        vInfo.m_iter_List_ಸೂಚ್ಯಂಕ = 0;
    }

    ಶೂನ್ಯ delete_ಶೃಂಗ(Dfg_ಶೃಂಗ* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_work_List_ಸೂಚ್ಯಂಕ, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = vtxp->cast<Dfg_ಶೃಂಗ_Var>();
        UASSERT_OBJ(!varp || !varp->hasPrev(), vtxp, "Deleting variable consumed via DfgPrev");

        // Invalidate cache entry
        invalidate_ಶೃಂಗ(vtxp);

        // It might be in the iter list, remove it
        removeFromIterList(vtxp);

        // Gather source vertices - they might be duplicates, make unique using generation number
        incrementGeneration();
        std::ಸದಿಶ<Dfg_ಶೃಂಗ*> srcps;
        srcps.reserve(vtxp->nInputs());
        vtxp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
            // If it's a variable, add to work list to see if it became redundant
            ಆದರೆ (src.is<Dfg_ಶೃಂಗ_Var>()) {
                ಕೂಡಿಸು_ಗಮ್ಯ_Work_List(&src);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            // Gather unique sources
            ಶೃಂಗ_Info& vInfo = m_vInfo[src];
            ಆದರೆ (vInfo.m_generation != m_currentGeneration) srcps.ತಳ್ಳು_ಹಿಂಭಾಗ(&src);
            vInfo.m_generation = m_currentGeneration;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        ಆದರೆ (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ತಳ್ಳು_ಹಿಂಭಾಗ(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ mid
            = std::stable_ವಿಭಜನೆ(srcps.begin(), srcps.ಕೊನೆ(), [](Dfg_ಶೃಂಗ* srcp) {  //
                  ಹಿಂತಿರುಗಿ srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = srcps.begin(); it != mid; ++it) ಕೂಡಿಸು_ಗಮ್ಯ_Iter_List(*it);

        // Recursively delete unused sources
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = mid; it != srcps.ಕೊನೆ(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            delete_ಶೃಂಗ(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    ಶೂನ್ಯ replace(Dfg_ಶೃಂಗ* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_work_List_ಸೂಚ್ಯಂಕ, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ debugCallback = [&]() -> ಶೂನ್ಯ {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.ಮೂಲ_Cone({m_vtxp, resp});
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ cone = m_dfg.ಮೂಲ_Cone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& v) {  //
                ಹಿಂತಿರುಗಿ cone->ಎಣಿಕೆ_2(&v);
            });
        };
        ಆದರೆ (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ಹಿಂತಿರುಗಿ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
            ಕೂಡಿಸು_ಗಮ್ಯ_Iter_List(&src);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](Dfg_ಶೃಂಗ& dst) {
            invalidate_ಶೃಂಗ(&dst);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](Dfg_ಶೃಂಗ& dst) {
            cache_ಶೃಂಗ(&dst);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });

        // Original vertex is now unused, so delete it
        delete_ಶೃಂಗ(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        ಕೂಡಿಸು_ಗಮ್ಯ_Iter_List(resp);
        // Add new vertex and sinks to work list as likely to match
        ಕೂಡಿಸು_ಗಮ್ಯ_Work_List(resp);
        resp->foreachSink([&](Dfg_ಶೃಂಗ& dst) {
            ಕೂಡಿಸು_ಗಮ್ಯ_Work_List(&dst);
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_ಸಾಲು* flp, uint32_t val) { ಹಿಂತಿರುಗಿ ಹೊಸ DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_ಸಾಲು* flp, uint32_t ಅಗಲ) {
        ಹಿಂತಿರುಗಿ ಹೊಸ DfgConst{m_dfg, flp, ಅಗಲ, 0};
    }
    DfgConst* makeZero(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype) {
        ಹಿಂತಿರುಗಿ makeZero(flp, dtype.ಗಾತ್ರ_2());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_ಸಾಲು* flp, uint32_t ಅಗಲ) {
        DfgConst* ಸ್ಥಿರಾಂಕ resp = makeZero(flp, ಅಗಲ);
        resp->ಅಂಕೆ().ಗಣ_All_Bits1();
        ಹಿಂತಿರುಗಿ resp;
    }

    // Create a new vertex of the given type
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2, ಬಗೆ_ಹೆಸರು... Operands>
    ಶೃಂಗ_2* make(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp = get_Or_Create_ಶೃಂಗ<ಶೃಂಗ_2, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        ಆದರೆ (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        ಶೃಂಗ_Info& vInfo = m_vInfo[vtxp];
        ಆದರೆ (!vInfo.m_id) vInfo.m_id = ++m_ಕೊನೆಯ_Id;
        // Add to work list
        ಕೂಡಿಸು_ಗಮ್ಯ_Work_List(vtxp);
        // Return new node
        ಹಿಂತಿರುಗಿ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2, ಬಗೆ_ಹೆಸರು... Operands>
    ಶೃಂಗ_2* make(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* examplep, Operands... operands) {
        ಹಿಂತಿರುಗಿ make<ಶೃಂಗ_2>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    Dfg_ಶೃಂಗ* replicate(Dfg_ಶೃಂಗ* vtxp, Dfg_ಶೃಂಗ* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_ಬಿಟ್_DType, vtxp, "Expected 1-bit vertex");
        ಆದರೆ (vtxp->dtype() == m_ಬಿಟ್_DType) ಹಿಂತಿರುಗಿ bitp;
        ಹಿಂತಿರುಗಿ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSame(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ap, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* bp) {
        ಆದರೆ (ap == bp) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (ap->dtype() != bp->dtype()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ aConstp = ap->cast<DfgConst>();
        ಆದರೆ (!aConstp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ bConstp = bp->cast<DfgConst>();
        ಆದರೆ (!bConstp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ aConstp->ಅಂಕೆ().isCaseEq(bConstp->ಅಂಕೆ());
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isZero(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* vtxp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ constp = vtxp->cast<DfgConst>()) ಹಿಂತಿರುಗಿ constp->isZero();
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isOnes(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* vtxp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ constp = vtxp->cast<DfgConst>()) ಹಿಂತಿರುಗಿ constp->isOnes();
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isEqOne(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* vtxp) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ constp = vtxp->cast<DfgConst>()) ಹಿಂತಿರುಗಿ constp->ಅಂಕೆ().isEqOne();
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ areAdjacent(uint32_t& lsb, ಸ್ಥಿರಾಂಕ DfgSel* lSelp, ಸ್ಥಿರಾಂಕ DfgSel* rSelp) {
        ಆದರೆ (!isSame(lSelp->srcp(), rSelp->srcp())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (lSelp->lsb() + lSelp->ಅಗಲ() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (lSelp->lsb() == rSelp->lsb() + rSelp->ಅಗಲ()) {
            lsb = rSelp->lsb();
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ foldUnary(ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp) {
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_Unary, ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Must invoke on unary");
        static_assert(std::is_final<ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Must invoke on final class");
        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp()->ಮಾದರಿ cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ಸ್ಥಿರಾಂಕ resultp = makeZero(vtxp->fileline(), vtxp->ಅಗಲ());
                foldOp<ಶೃಂಗ_2>(resultp->ಅಂಕೆ(), srcp->ಅಂಕೆ());
                replace(resultp);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ ದ್ವಿಮಾನ(ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp) {
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ, ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Must invoke on binary");
        static_assert(std::is_final<ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Must invoke on final class");

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
        // LHS/RHS of the vertex
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ಸ್ಥಿರಾಂಕ lConstp = lhsp->cast<DfgConst>();
        DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ lSamep = lhsp->cast<ಶೃಂಗ_2>();
        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ rSamep = rhsp->cast<ಶೃಂಗ_2>();

        // Constant folding
        ಆದರೆ (lConstp && rConstp) {
            APPLYING(FOLD_ದ್ವಿಮಾನ) {
                DfgConst* ಸ್ಥಿರಾಂಕ resultp = makeZero(flp, ಫಲಿತಾಂಶ_DType<ಶೃಂಗ_2>(lhsp, rhsp));
                foldOp<ಶೃಂಗ_2>(resultp->ಅಂಕೆ(), lConstp->ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                replace(resultp);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }

        // Idempotent
        ಆದರೆ VL_CONSTEXPR_CXX17 (IsIdempotent<ಶೃಂಗ_2>::ಮೌಲ್ಯ) {
            ಆದರೆ (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ದ್ವಿಮಾನ) {
                    replace(lhsp);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
        }

        // Associative
        ಆದರೆ VL_CONSTEXPR_CXX17 (IsAssociative<ಶೃಂಗ_2>::ಮೌಲ್ಯ) {
            // Also commutative and idempotent
            ಆದರೆ VL_CONSTEXPR_CXX17 (IsCommutative<ಶೃಂಗ_2>::ಮೌಲ್ಯ && IsIdempotent<ಶೃಂಗ_2>::ಮೌಲ್ಯ) {
                ಆದರೆ (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ದ್ವಿಮಾನ_LHS) {
                        replace(lSamep);
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
                ಆದರೆ (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ದ್ವಿಮಾನ_RHS) {
                        replace(rSamep);
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
            }

            ಆದರೆ (lConstp && rSamep) {
                ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rlConstp = rSamep->lhsp()->ಮಾದರಿ cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ದ್ವಿಮಾನ_LHS_ದ_RHS) {
                        DfgConst* ಸ್ಥಿರಾಂಕ cp = makeZero(flp, ಫಲಿತಾಂಶ_DType<ಶೃಂಗ_2>(lConstp, rlConstp));
                        foldOp<ಶೃಂಗ_2>(cp->ಅಂಕೆ(), lConstp->ಅಂಕೆ(), rlConstp->ಅಂಕೆ());
                        replace(make<ಶೃಂಗ_2>(vtxp, cp, rSamep->rhsp()));
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
            }

            ಆದರೆ (lSamep && rConstp) {
                ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lrConstp = lSamep->rhsp()->ಮಾದರಿ cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ದ್ವಿಮಾನ_RHS_ದ_LHS) {
                        DfgConst* ಸ್ಥಿರಾಂಕ cp = makeZero(flp, ಫಲಿತಾಂಶ_DType<ಶೃಂಗ_2>(lrConstp, rConstp));
                        foldOp<ಶೃಂಗ_2>(cp->ಅಂಕೆ(), lrConstp->ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                        replace(make<ಶೃಂಗ_2>(vtxp, lSamep->lhsp(), cp));
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            ಆದರೆ (lSamep && !lSamep->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ap = lSamep->lhsp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ bp = lSamep->rhsp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                ಆದರೆ (ap != cp && bp != cp) {
                    APPLYING(ಬಲ_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಮಗು_DType = ಫಲಿತಾಂಶ_DType<ಶೃಂಗ_2>(bp, cp);
                        ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ bcp = make<ಶೃಂಗ_2>(vtxp->fileline(), ಮಗು_DType, bp, cp);
                        replace(make<ಶೃಂಗ_2>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
            }

            ಆದರೆ (rSamep && !rSamep->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rlVtxp = rSamep->lhsp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rrVtxp = rSamep->rhsp();

                ಆದರೆ VL_CONSTEXPR_CXX17 (IsCommutative<ಶೃಂಗ_2>::ಮೌಲ್ಯ) {
                    ಆದರೆ (!lhsp->has_ಅಪವರ್ತ್ಯ_Sinks() && rlVtxp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<ಶೃಂಗ_2>(vtxp, rlVtxp, make<ಶೃಂಗ_2>(vtxp, lhsp, rrVtxp)));
                            ಹಿಂತಿರುಗಿ ನಿಜ;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                ಆದರೆ (ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ existingp
                    = get_ಶೃಂಗ<ಶೃಂಗ_2>(ಫಲಿತಾಂಶ_DType<ಶೃಂಗ_2>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    ಆದರೆ (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ದ್ವಿಮಾನ_LHS_WITH_LHS_ದ_RHS) {
                            replace(make<ಶೃಂಗ_2>(vtxp, existingp, rrVtxp));
                            ಹಿಂತಿರುಗಿ ನಿಜ;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                ಆದರೆ VL_CONSTEXPR_CXX17 (IsCommutative<ಶೃಂಗ_2>::ಮೌಲ್ಯ) {
                    ಆದರೆ (ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ existingp
                        = get_ಶೃಂಗ<ಶೃಂಗ_2>(ಫಲಿತಾಂಶ_DType<ಶೃಂಗ_2>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        ಆದರೆ (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ದ್ವಿಮಾನ_LHS_WITH_RHS_ದ_RHS) {
                                replace(make<ಶೃಂಗ_2>(vtxp, existingp, rlVtxp));
                                ಹಿಂತಿರುಗಿ ನಿಜ;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        ಆದರೆ VL_CONSTEXPR_CXX17 (IsCommutative<ಶೃಂಗ_2>::ಮೌಲ್ಯ) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            ಆದರೆ (lConstp) {
                ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ rCondp = rhsp->cast<DfgCond>()) {
                    ಆದರೆ (!rCondp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                        APPLYING(ತಳ್ಳು_COMMUTATIVE_ದ್ವಿಮಾನ_THROUGH_COND) {
                            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ tp = make<ಶೃಂಗ_2>(vtxp, lConstp, rCondp->thenp());
                            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ep = make<ಶೃಂಗ_2>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ಹಿಂತಿರುಗಿ ನಿಜ;
                        }
                    }
                }
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಅದಲುಬದಲು_Sides = [&]() {
            // Don't swap if not commutative, duh
            ಆದರೆ VL_CONSTEXPR_CXX17 (!IsCommutative<ಶೃಂಗ_2>::ಮೌಲ್ಯ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Const go on left
            ಆದರೆ (lConstp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (rConstp) ಹಿಂತಿರುಗಿ ನಿಜ;
            // Not go on left
            ಆದರೆ (lhsp->is<DfgNot>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (rhsp->is<DfgNot>()) ಹಿಂತಿರುಗಿ ನಿಜ;
            // Same go on right
            ಆದರೆ (rSamep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (lSamep) ಹಿಂತಿರುಗಿ ನಿಜ;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ಸ್ಥಿರಾಂಕ ಶೃಂಗ_Info& lInfo = m_vInfo[lhsp];
            ಸ್ಥಿರಾಂಕ ಶೃಂಗ_Info& rInfo = m_vInfo[rhsp];
            ಹಿಂತಿರುಗಿ lInfo.m_id > rInfo.m_id;
        }();
        ಆದರೆ (ಅದಲುಬದಲು_Sides) {
            APPLYING(ಅದಲುಬದಲು_SIDES_ಒಳ_ದ್ವಿಮಾನ) {
                replace(make<ಶೃಂಗ_2>(vtxp, rhsp, lhsp));
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Other, ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ distributive_And_Associative_ದ್ವಿಮಾನ(ಶೃಂಗ_2* vtxp) {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        ಆದರೆ (!lhsp->has_ಅಪವರ್ತ್ಯ_Sinks() && !rhsp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            ಆದರೆ (Other* ಸ್ಥಿರಾಂಕ lp = lhsp->cast<Other>()) {
                ಆದರೆ (Other* ಸ್ಥಿರಾಂಕ rp = rhsp->cast<Other>()) {
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ llp = lp->lhsp();
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lrp = lp->rhsp();
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rlp = rp->lhsp();
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rrp = rp->rhsp();
                    Dfg_ಶೃಂಗ* ap = ಶೂನ್ಯಸೂಚಕ;
                    Dfg_ಶೃಂಗ* bp = ಶೂನ್ಯಸೂಚಕ;
                    Dfg_ಶೃಂಗ* cp = ಶೂನ್ಯಸೂಚಕ;
                    ಆದರೆ (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    ಆದರೆ (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_ದ್ವಿಮಾನ) {
                            replace(make<Other>(vtxp, ap, make<ಶೃಂಗ_2>(lhsp, bp, cp)));
                            ಹಿಂತಿರುಗಿ ನಿಜ;
                        }
                    }
                }
            }
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ
    try_ತಳ್ಳು_Bitwise_Op_Through_Concat(ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ಸ್ಥಿರಾಂಕ catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ಸ್ಥಿರಾಂಕ catRConstp = concatp->rhsp()->cast<DfgConst>();
        ಆದರೆ (catLConstp || catRConstp) {
            APPLYING(ತಳ್ಳು_BITWISE_OP_THROUGH_CONCAT) {
                ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = concatp->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& lDtype = concatp->lhsp()->dtype();
                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& rDtype = concatp->rhsp()->dtype();
                ಸ್ಥಿರಾಂಕ uint32_t l_ಅಗಲ = lDtype.ಗಾತ್ರ_2();
                ಸ್ಥಿರಾಂಕ uint32_t r_ಅಗಲ = rDtype.ಗಾತ್ರ_2();

                // The new Lhs vertex
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ newLhsp = [&]() -> Dfg_ಶೃಂಗ* {
                    DfgConst* ಸ್ಥಿರಾಂಕ newLhsConstp = makeZero(constp->fileline(), l_ಅಗಲ);
                    ಆದರೆ (catLConstp) {
                        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{constp->fileline(), static_cast<ಪೂರ್ಣಾಂಕ>(l_ಅಗಲ), 0u};
                        ಅಂಕೆ.opSel(constp->ಅಂಕೆ(), ಅಗಲ - 1, r_ಅಗಲ);
                        foldOp<ಶೃಂಗ_2>(newLhsConstp->ಅಂಕೆ(), ಅಂಕೆ, catLConstp->ಅಂಕೆ());
                        ಹಿಂತಿರುಗಿ newLhsConstp;
                    }
                    newLhsConstp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), ಅಗಲ - 1, r_ಅಗಲ);
                    ಹಿಂತಿರುಗಿ make<ಶೃಂಗ_2>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ newRhsp = [&]() -> Dfg_ಶೃಂಗ* {
                    DfgConst* ಸ್ಥಿರಾಂಕ newRhsConstp = makeZero(constp->fileline(), r_ಅಗಲ);
                    ಆದರೆ (catRConstp) {
                        V3_ಸಂಖ್ಯೆ ಅಂಕೆ{constp->fileline(), static_cast<ಪೂರ್ಣಾಂಕ>(r_ಅಗಲ), 0u};
                        ಅಂಕೆ.opSel(constp->ಅಂಕೆ(), r_ಅಗಲ - 1, 0);
                        foldOp<ಶೃಂಗ_2>(newRhsConstp->ಅಂಕೆ(), ಅಂಕೆ, catRConstp->ಅಂಕೆ());
                        ಹಿಂತಿರುಗಿ newRhsConstp;
                    }
                    newRhsConstp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), r_ಅಗಲ - 1, 0);
                    ಹಿಂತಿರುಗಿ make<ಶೃಂಗ_2>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಶೃಂಗ_2>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ
    try_ತಳ್ಳು_Compare_Op_Through_Concat(ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        ಆದರೆ (!concatp->has_ಅಪವರ್ತ್ಯ_Sinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ತಳ್ಳು_COMPARE_OP_THROUGH_CONCAT) {
                ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = concatp->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ uint32_t l_ಅಗಲ = concatp->lhsp()->ಅಗಲ();
                ಸ್ಥಿರಾಂಕ uint32_t r_ಅಗಲ = concatp->rhsp()->ಅಗಲ();

                // The new Lhs vertex
                DfgConst* ಸ್ಥಿರಾಂಕ newLhsConstp = makeZero(constp->fileline(), l_ಅಗಲ);
                newLhsConstp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), ಅಗಲ - 1, r_ಅಗಲ);
                ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ newLhsp
                    = make<ಶೃಂಗ_2>(flp, m_ಬಿಟ್_DType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ಸ್ಥಿರಾಂಕ newRhsConstp = makeZero(constp->fileline(), r_ಅಗಲ);
                newRhsConstp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), r_ಅಗಲ - 1, 0);
                ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ newRhsp
                    = make<ಶೃಂಗ_2>(flp, m_ಬಿಟ್_DType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                ಆದರೆ VL_CONSTEXPR_CXX17 (std::is_same<ಶೃಂಗ_2, DfgEq>::ಮೌಲ್ಯ) {
                    replace(make<DfgAnd>(concatp->fileline(), m_ಬಿಟ್_DType, newLhsp, newRhsp));
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ VL_CONSTEXPR_CXX17 (std::is_same<ಶೃಂಗ_2, DfgNeq>::ಮೌಲ್ಯ) {
                    replace(make<DfgOr>(concatp->fileline(), m_ಬಿಟ್_DType, newLhsp, newRhsp));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Bitwise>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ try_ತಳ್ಳು_Bitwise_Op_Through_Reductions(Bitwise* ಸ್ಥಿರಾಂಕ vtxp) {
        ಬಳಸಿ Reduction = Bitwise_ಗಮ್ಯ_Reduction<Bitwise>;

        ಆದರೆ (Reduction* ಸ್ಥಿರಾಂಕ lRedp = vtxp->lhsp()->ಮಾದರಿ cast<Reduction>()) {
            ಆದರೆ (Reduction* ಸ್ಥಿರಾಂಕ rRedp = vtxp->rhsp()->ಮಾದರಿ cast<Reduction>()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lSrcp = lRedp->srcp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rSrcp = rRedp->srcp();
                ಆದರೆ (lSrcp->dtype() == rSrcp->dtype() && lSrcp->ಅಗಲ() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->has_ಅಪವರ್ತ್ಯ_Sinks() && !rSrcp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(ತಳ್ಳು_BITWISE_THROUGH_REDUCTION) {
                        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
                        Bitwise* ಸ್ಥಿರಾಂಕ bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_ಬಿಟ್_DType, bwp));
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
            }
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    ಸ್ಥಿರ std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(Dfg_ಶೃಂಗ* vtxp) {
        ಆದರೆ (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        }
        ಆದರೆ (vtxp->has_ಅಪವರ್ತ್ಯ_Sinks()) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ಸ್ಥಿರಾಂಕ bwp = vtxp->as<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ>();
        DfgSel* ಸ್ಥಿರಾಂಕ lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ಸ್ಥಿರಾಂಕ rSelp = bwp->rhsp()->cast<DfgSel>();
        ಆದರೆ (!lSelp || !rSelp) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        DfgSel* ಸ್ಥಿರಾಂಕ nSelp = lSelp->fromp()->ಅಗಲ() <= rSelp->fromp()->ಅಗಲ() ? lSelp : rSelp;
        DfgSel* ಸ್ಥಿರಾಂಕ wSelp = nSelp == lSelp ? rSelp : lSelp;
        ಸ್ಥಿರಾಂಕ uint32_t n_ಅಗಲ = nSelp->fromp()->ಅಗಲ();
        ಆದರೆ (n_ಅಗಲ > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        ಆದರೆ (wSelp->lsb() - nSelp->lsb() + n_ಅಗಲ > wSelp->fromp()->ಅಗಲ()) {
            ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        }
        ಹಿಂತಿರುಗಿ {nSelp, wSelp};
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Bitwise>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ try_ತಳ್ಳು_Bitwise_Op_Throug_Sel(Bitwise* ಸ್ಥಿರಾಂಕ vtxp) {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();

        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = ಶೂನ್ಯಸೂಚಕ;
            Dfg_ಶೃಂಗ* extrap = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Bitwise* ಸ್ಥಿರಾಂಕ bitwisep = rhsp->cast<Bitwise>()) {
                ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ rlSelp = bitwisep->lhsp()->ಮಾದರಿ cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ rrSelp = bitwisep->rhsp()->ಮಾದರಿ cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            ಆದರೆ (rSelp) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lFromp = lSelp->fromp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rFromp = rSelp->fromp();
                ಆದರೆ (lFromp->dtype() == rFromp->dtype() && lFromp->ಅಗಲ() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ತಳ್ಳು_BITWISE_THROUGH_SEL) {
                        Bitwise* ಸ್ಥಿರಾಂಕ bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        Dfg_ಶೃಂಗ* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        ಆದರೆ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ಹಿಂತಿರುಗಿ ನಿಜ;
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
            Bitwise* ಸ್ಥಿರಾಂಕ rChainp = rhsp->cast<Bitwise>();
            ಆದರೆ (rChainp && rChainp->has_ಅಪವರ್ತ್ಯ_Sinks()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ lSels = pushableUnalignedSels(lhsp);
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ rSels = pushableUnalignedSels(rTermp);
            ಆದರೆ (!lSels.ಮೊದಲ && !rSels.ಮೊದಲ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ opp = lSels.ಮೊದಲ ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            ಆದರೆ (opp->is<Bitwise>()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& sels = lSels.ಮೊದಲ ? lSels : rSels;
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ nFromp = sels.ಮೊದಲ->fromp();
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ wFromp = sels.ಎರಡನೇ->fromp();
            ಸ್ಥಿರಾಂಕ uint32_t ಸ್ಥಾನಾಂತರ = sels.ಎರಡನೇ->lsb() - sels.ಮೊದಲ->lsb();

            // Both terms must end up as selects from the same pushed operation
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ matches = [&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* termp, ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& tSels) {
                // Original form: 'n[i] OP w[j]'
                ಆದರೆ (tSels.ಮೊದಲ) {
                    ಹಿಂತಿರುಗಿ termp->type() == opp->type()  //
                           && tSels.ಮೊದಲ->fromp() == nFromp  //
                           && tSels.ಎರಡನೇ->fromp() == wFromp  //
                           && tSels.ಎರಡನೇ->lsb() - tSels.ಮೊದಲ->lsb() == ಸ್ಥಾನಾಂತರ;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                ಆದರೆ (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSel* ಸ್ಥಿರಾಂಕ selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                ಆದರೆ (termp->type() != opp->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                // Operands might have been swapped since
                ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ಸ್ಥಿರಾಂಕ bwp = termp->as<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ>();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ nIsLhs = bwp->lhsp() == nFromp;
                ಆದರೆ (!nIsLhs && bwp->rhsp() != nFromp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                ಸ್ಥಿರಾಂಕ DfgSel* ಸ್ಥಿರಾಂಕ slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ಹಿಂತಿರುಗಿ slicep && slicep->fromp() == wFromp && slicep->lsb() == ಸ್ಥಾನಾಂತರ;
            };
            ಆದರೆ (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

            APPLYING(ತಳ್ಳು_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = nFromp->dtype();
                DfgSel* ಸ್ಥಿರಾಂಕ slicep = make<DfgSel>(flp, dtype, wFromp, ಸ್ಥಾನಾಂತರ);
                Dfg_ಶೃಂಗ* bwp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ತಳ್ಳು = [&](Dfg_ಶೃಂಗ* termp, ಸ್ಥಿರಾಂಕ DfgSel* nSelp) -> Dfg_ಶೃಂಗ* {
                    ಹಿಂತಿರುಗಿ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ newLhsp = ತಳ್ಳು(lhsp, lSels.ಮೊದಲ);
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ newRTermp = ತಳ್ಳು(rTermp, rSels.ಮೊದಲ);
                ಆದರೆ (rChainp) {
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isConcatFoldable(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) {
        // Concatenation of the same vertex is a replicate
        ಆದರೆ (isSame(lhsp, rhsp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgRep* ಸ್ಥಿರಾಂಕ lRepp = lhsp->cast<DfgRep>()) {
            ಆದರೆ (isSame(lRepp->srcp(), rhsp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgRep* ಸ್ಥಿರಾಂಕ rRepp = rhsp->cast<DfgRep>()) {
            ಆದರೆ (isSame(lhsp, rRepp->srcp())) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        // Concatenation of adjoining selects is a select
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSel* ಸ್ಥಿರಾಂಕ lSelp = lhsp->cast<DfgSel>()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSel* ಸ್ಥಿರಾಂಕ rSelp = rhsp->cast<DfgSel>()) {
                ಹಿಂತಿರುಗಿ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->ಅಗಲ();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgNot* ಸ್ಥಿರಾಂಕ lNotp = lhsp->cast<DfgNot>()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgNot* ಸ್ಥಿರಾಂಕ rNotp = rhsp->cast<DfgNot>()) {
                ಹಿಂತಿರುಗಿ !lNotp->has_ಅಪವರ್ತ್ಯ_Sinks()  //
                       && !rNotp->has_ಅಪವರ್ತ್ಯ_Sinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isSameBitwise(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) {
        ಆದರೆ (lhsp->type() != rhsp->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isConcatSimplifiable(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp) {
        ಆದರೆ (isConcatFoldable(lhsp, rhsp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (!isSameBitwise(lhsp, rhsp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (lhsp->has_ಅಪವರ್ತ್ಯ_Sinks() || rhsp->has_ಅಪವರ್ತ್ಯ_Sinks()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ಸ್ಥಿರಾಂಕ lp = lhsp->as<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ>();
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ಸ್ಥಿರಾಂಕ rp = rhsp->as<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ>();
        ಆದರೆ (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isConcatPushableThroughBitwise(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp,
                                               ತಾರ್ಕಿಕ& ಅದಲುಬದಲು) {
        ಆದರೆ (!isSameBitwise(lhsp, rhsp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (lhsp->has_ಅಪವರ್ತ್ಯ_Sinks() || rhsp->has_ಅಪವರ್ತ್ಯ_Sinks()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ಸ್ಥಿರಾಂಕ lp = lhsp->as<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ>();
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ಸ್ಥಿರಾಂಕ rp = rhsp->as<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ>();
        ಅದಲುಬದಲು = ಸುಳ್ಳು;
        ಆದರೆ (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಅದಲುಬದಲು = ನಿಜ;
        ಆದರೆ (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    Dfg_ಶೃಂಗ* makeConcatPushedThroughBitwise(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* lhsp,
                                              ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* rhsp, ತಾರ್ಕಿಕ ಅದಲುಬದಲು) {
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ಸ್ಥಿರಾಂಕ lp = lhsp->as<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ>();
        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ಸ್ಥಿರಾಂಕ rp = rhsp->as<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ>();
        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = Dfg_ದತ್ತಾಂಶ_Type::packed(lp->ಅಗಲ() + rp->ಅಗಲ());
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cp = ಅದಲುಬದಲು ? rp->rhsp() : rp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ dp = ಅದಲುಬದಲು ? rp->lhsp() : rp->rhsp();
        DfgConcat* ಸ್ಥಿರಾಂಕ newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ಸ್ಥಿರಾಂಕ newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        ಆದರೆ (lp->is<DfgAnd>()) ಹಿಂತಿರುಗಿ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        ಆದರೆ (lp->is<DfgOr>()) ಹಿಂತಿರುಗಿ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ಹಿಂತಿರುಗಿ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Bitwise>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->ಅಗಲ() == 1, vtxp, "Width must be 1");
        ಬಳಸಿ Reduction = Bitwise_ಗಮ್ಯ_Reduction<Bitwise>;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();

        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lSelp = lhsp->ಮಾದರಿ cast<DfgSel>()) {
            DfgSel* rSelp = ಶೂನ್ಯಸೂಚಕ;
            Dfg_ಶೃಂಗ* extrap = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = rhsp->ಮಾದರಿ cast<DfgSel>()) {
                rSelp = selp;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Bitwise* ಸ್ಥಿರಾಂಕ rBitwisep = rhsp->ಮಾದರಿ cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->ಮಾದರಿ cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Reduction* ಸ್ಥಿರಾಂಕ rRedp = rhsp->ಮಾದರಿ cast<Reduction>()) {
                rSelp = rRedp->srcp()->ಮಾದರಿ cast<DfgSel>();
            }
            ಆದರೆ (rSelp) {
                uint32_t lsb = 0;
                ಆದರೆ (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_ದ_SELS_WITH_REDUCTION) {
                        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype
                            = Dfg_ದತ್ತಾಂಶ_Type::packed(lSelp->ಅಗಲ() + rSelp->ಅಗಲ());
                        DfgSel* ಸ್ಥಿರಾಂಕ newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        Dfg_ಶೃಂಗ* resp = make<Reduction>(vtxp, newSelp);
                        ಆದರೆ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }
            }
        }

        ಆದರೆ (Reduction* ಸ್ಥಿರಾಂಕ lRedp = lhsp->ಮಾದರಿ cast<Reduction>()) {
            Reduction* rRedp = rhsp->ಮಾದರಿ cast<Reduction>();
            Dfg_ಶೃಂಗ* extrap = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!rRedp) {
                ಆದರೆ (Bitwise* ಸ್ಥಿರಾಂಕ rBitwisep = rhsp->ಮಾದರಿ cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->ಮಾದರಿ cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            ಆದರೆ (rRedp) {
                ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lSelp = lRedp->srcp()->ಮಾದರಿ cast<DfgSel>()) {
                    ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ rSelp = rRedp->srcp()->ಮಾದರಿ cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ಆದರೆ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_ದ_REDUCTION_ದ_SELS_WITH_REDUCTION) {
                                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype
                                    = Dfg_ದತ್ತಾಂಶ_Type::packed(lSelp->ಅಗಲ() + rSelp->ಅಗಲ());
                                DfgSel* ಸ್ಥಿರಾಂಕ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                Dfg_ಶೃಂಗ* resp = make<Reduction>(vtxp, newSelp);
                                ಆದರೆ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ಹಿಂತಿರುಗಿ ನಿಜ;
                            }
                        }
                    }
                }
            }
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Reduction>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ optimizeReduction(Reduction* ಸ್ಥಿರಾಂಕ vtxp) {
        ಬಳಸಿ Bitwise = Reduction_ಗಮ್ಯ_Bitwise<Reduction>;

        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ ನಿಜ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        // Reduction of 1-bit value
        ಆದರೆ (srcp->dtype() == m_ಬಿಟ್_DType) {
            APPLYING(REMOVE_ಅಗಲ_ONE_REDUCTION) {
                replace(srcp);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }

        ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ condp = srcp->cast<DfgCond>()) {
            ಆದರೆ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ತಳ್ಳು_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ಸ್ಥಿರಾಂಕ newThenp = make<Reduction>(flp, m_ಬಿಟ್_DType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ಸ್ಥಿರಾಂಕ newElsep = make<Reduction>(flp, m_ಬಿಟ್_DType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ಸ್ಥಿರಾಂಕ newCondp = make<DfgCond>(condp->fileline(), m_ಬಿಟ್_DType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
        }

        ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ concatp = srcp->cast<DfgConcat>()) {
            ಆದರೆ (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_ಬಿಟ್_DType
                || concatp->rhsp()->dtype() == m_ಬಿಟ್_DType) {
                APPLYING(ತಳ್ಳು_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ಸ್ಥಿರಾಂಕ lRedp
                        = make<Reduction>(concatp->fileline(), m_ಬಿಟ್_DType, concatp->lhsp());
                    Reduction* ಸ್ಥಿರಾಂಕ rRedp
                        = make<Reduction>(concatp->fileline(), m_ಬಿಟ್_DType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_ಬಿಟ್_DType, lRedp, rRedp));
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
        }

        ಆದರೆ (Bitwise* ಸ್ಥಿರಾಂಕ bitwisep = vtxp->srcp()->ಮಾದರಿ cast<Bitwise>()) {
            ಆದರೆ (!bitwisep->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                ಆದರೆ (bitwisep->lhsp()->ಮಾದರಿ is<DfgConcat>()
                    || bitwisep->rhsp()->ಮಾದರಿ is<DfgConcat>()) {
                    APPLYING(ತಳ್ಳು_REDUCTION_THROUGH_BITWISE_ದ_CONCAT) {
                        Reduction* ಸ್ಥಿರಾಂಕ newLhsp
                            = make<Reduction>(flp, m_ಬಿಟ್_DType, bitwisep->lhsp());
                        Reduction* ಸ್ಥಿರಾಂಕ newRhsp
                            = make<Reduction>(flp, m_ಬಿಟ್_DType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_ಬಿಟ್_DType, newLhsp, newRhsp));
                        ಹಿಂತಿರುಗಿ ನಿಜ;
                    }
                }

                ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lSelp = bitwisep->lhsp()->ಮಾದರಿ cast<DfgSel>()) {
                    ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ rSelp = bitwisep->rhsp()->ಮಾದರಿ cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ಆದರೆ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ತಳ್ಳು_REDUCTION_THROUGH_BITWISE_ದ_SELS) {
                                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype
                                    = Dfg_ದತ್ತಾಂಶ_Type::packed(lSelp->ಅಗಲ() + rSelp->ಅಗಲ());
                                DfgSel* ಸ್ಥಿರಾಂಕ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ಹಿಂತಿರುಗಿ ನಿಜ;
                            }
                        }
                    }
                }
            }
        }

        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು ಸರಿಸು>
    VL_ATTR_WARN_ಬಳಸದ_ಫಲಿತಾಂಶ ತಾರ್ಕಿಕ optimize_ಸರಿಸು_RHS(ಸರಿಸು* ಸ್ಥಿರಾಂಕ vtxp) {
        static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ, ಸರಿಸು>::ಮೌಲ್ಯ, "Must invoke on binary");
        static_assert(std::is_final<ಸರಿಸು>::ಮೌಲ್ಯ, "Must invoke on final class");
        ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConcat* ಸ್ಥಿರಾಂಕ concatp = vtxp->rhsp()->ಮಾದರಿ cast<DfgConcat>()) {
            ಆದರೆ (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_ದ_ಸರಿಸು) {
                    replace(make<ಸರಿಸು>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<Dfg_ಶೃಂಗ*, uint32_t, uint32_t> ಕೂಡಿಸು_ಗಮ್ಯ_ಎಣಿಕೆ_Ones_ಪದ(Dfg_ಶೃಂಗ* vtxp) {
        ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ oCatp = vtxp->cast<DfgConcat>()) {
            ಆದರೆ (isZero(oCatp->lhsp())) {
                ಆದರೆ (Dfg_ಎಣಿಕೆ_Ones* ಸ್ಥಿರಾಂಕ ಎಣಿಕೆ_Onesp = oCatp->rhsp()->cast<Dfg_ಎಣಿಕೆ_Ones>()) {
                    // Zero extended count ones
                    ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = ಎಣಿಕೆ_Onesp->srcp()->cast<DfgSel>()) {
                        ಹಿಂತಿರುಗಿ {selp->fromp(), selp->lsb(), selp->ಅಗಲ()};
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    ಆದರೆ (selp->dtype() == m_ಬಿಟ್_DType) {  //
                        ಹಿಂತಿರುಗಿ {selp->fromp(), selp->lsb(), selp->ಅಗಲ()};
                    }
                }
            }
            ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0, 0};
        }
        ಆದರೆ (Dfg_ಎಣಿಕೆ_Ones* ಸ್ಥಿರಾಂಕ ಎಣಿಕೆ_Onesp = vtxp->cast<Dfg_ಎಣಿಕೆ_Ones>()) {
            // Simple count ones
            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = ಎಣಿಕೆ_Onesp->srcp()->cast<DfgSel>()) {
                ಹಿಂತಿರುಗಿ {selp->fromp(), selp->lsb(), selp->ಅಗಲ()};
            }
            ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0, 0};
        }
        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ oSelp = vtxp->cast<DfgSel>()) {
            ಆದರೆ (oSelp->lsb() == 0) {
                // Truncated count ones
                ಆದರೆ (Dfg_ಎಣಿಕೆ_Ones* ಸ್ಥಿರಾಂಕ ಎಣಿಕೆ_Onesp = oSelp->fromp()->cast<Dfg_ಎಣಿಕೆ_Ones>()) {
                    // Zero extended count ones
                    ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = ಎಣಿಕೆ_Onesp->srcp()->cast<DfgSel>()) {
                        ಹಿಂತಿರುಗಿ {selp->fromp(), selp->lsb(), selp->ಅಗಲ()};
                    }
                }
            }
            // Single bit select
            ಆದರೆ (oSelp->dtype() == m_ಬಿಟ್_DType) {  //
                ಹಿಂತಿರುಗಿ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0, 0};
        }
        // Altered form of extended MSB
        ಆದರೆ (Dfg_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ shiftrp = vtxp->cast<Dfg_ಸರಿಸು_R>()) {
            ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                ಆದರೆ (rConstp->ಗಮ್ಯ_U32() == shiftrp->ಅಗಲ() - 1) {
                    ಹಿಂತಿರುಗಿ {shiftrp->lhsp(), shiftrp->ಅಗಲ() - 1, 1};
                }
            }
            ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0, 0};
        }
        // Not applicable
        ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<Dfg_ಶೃಂಗ*, uint32_t> unwindSel(Dfg_ಶೃಂಗ* fromp, uint32_t lsb,
                                              ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ) {
        ಆಗ (ನಿಜ) {
            ಸ್ಥಿರಾಂಕ uint32_t msb = lsb + ಅಗಲ - 1;

            // Sel from Concat
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ concatp = fromp->cast<DfgConcat>()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = concatp->lhsp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = concatp->rhsp();

                ಆದರೆ (msb < rhsp->ಅಗಲ()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_ದ_CONCAT) {
                        fromp = rhsp;
                        ಮುಂದುವರಿ;
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lsb >= rhsp->ಅಗಲ()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_ದ_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->ಅಗಲ();
                        ಮುಂದುವರಿ;
                    }
                }
            }

            ಆದರೆ (DfgRep* ಸ್ಥಿರಾಂಕ repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ಸ್ಥಿರಾಂಕ uint32_t src_ಅಗಲ = repp->srcp()->ಅಗಲ();
                ಆದರೆ (ಅಗಲ <= src_ಅಗಲ) {
                    ಸ್ಥಿರಾಂಕ uint32_t newLsb = lsb % src_ಅಗಲ;
                    ಸ್ಥಿರಾಂಕ uint32_t newMsb = newLsb + ಅಗಲ - 1;
                    ಆದರೆ (newMsb < src_ಅಗಲ) {
                        APPLYING(ತಳ್ಳು_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            ಮುಂದುವರಿ;
                        }
                    }
                }
            }

            // Sel from Sel
            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    ಮುಂದುವರಿ;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            ಆದರೆ (DfgVarPacked* ಸ್ಥಿರಾಂಕ varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = varp->driver_ದ_Range(lsb, ಅಗಲ);
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ driverp = pair.ಮೊದಲ;
                ಸ್ಥಿರಾಂಕ uint32_t driverLsb = pair.ಎರಡನೇ;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                ಆದರೆ (driverp) {
                    ಆದರೆ (driverp == varp->defaultp()) {
                        APPLYING(ತಳ್ಳು_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ಮುಂದುವರಿ;
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        APPLYING(ತಳ್ಳು_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ಮುಂದುವರಿ;
                        }
                    }
                }
            }

            // No patterns matched, stop
            ನಿಲ್ಲಿಸು;
        }
        ಹಿಂತಿರುಗಿ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_ಶೃಂಗ*, uint32_t> commonLSBs(Dfg_ಶೃಂಗ* ap, Dfg_ಶೃಂಗ* bp) {
        ಆದರೆ (ap == bp) ಹಿಂತಿರುಗಿ {ap, ap->ಅಗಲ()};

        // If both constants, check LSBs
        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ aConstp = ap->cast<DfgConst>()) {
            ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ bConstp = bp->cast<DfgConst>()) {
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& a_ಅಂಕೆ = aConstp->ಅಂಕೆ();
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& b_ಅಂಕೆ = bConstp->ಅಂಕೆ();
                // Max match is the shorter constant
                ಸ್ಥಿರಾಂಕ uint32_t max_ಹೊಂದಿಕೆ = std::min(aConstp->ಅಗಲ(), bConstp->ಅಗಲ());
                // Check all bits
                uint32_t ಹೊಂದಿಕೆ_ಅಗಲ = 0;
                ಗಾಗಿ (; ಹೊಂದಿಕೆ_ಅಗಲ < max_ಹೊಂದಿಕೆ; ++ಹೊಂದಿಕೆ_ಅಗಲ) {
                    ಆದರೆ (a_ಅಂಕೆ.ಬಿಟ್_Is0(ಹೊಂದಿಕೆ_ಅಗಲ) != b_ಅಂಕೆ.ಬಿಟ್_Is0(ಹೊಂದಿಕೆ_ಅಗಲ)) ನಿಲ್ಲಿಸು;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ಸ್ಥಿರಾಂಕ shorterp = aConstp->ಅಗಲ() < bConstp->ಅಗಲ() ? aConstp : bConstp;
                ಹಿಂತಿರುಗಿ {ಹೊಂದಿಕೆ_ಅಗಲ ? shorterp : ಶೂನ್ಯಸೂಚಕ, ಹೊಂದಿಕೆ_ಅಗಲ};
            }
        }

        // If Concat, check against the RHS
        ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ catp = ap->cast<DfgConcat>()) ಹಿಂತಿರುಗಿ commonLSBs(catp->rhsp(), bp);
        ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ catp = bp->cast<DfgConcat>()) ಹಿಂತಿರುಗಿ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = ap->cast<DfgSel>()) {
            ಆದರೆ (selp->lsb() == 0) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = selp->fromp();
                ಸ್ಥಿರಾಂಕ std::pair<Dfg_ಶೃಂಗ*, uint32_t> common = commonLSBs(fromp, bp);
                ಹಿಂತಿರುಗಿ {common.ಮೊದಲ, std::min(common.ಎರಡನೇ, selp->ಅಗಲ())};
            }
        }
        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = bp->cast<DfgSel>()) {
            ಆದರೆ (selp->lsb() == 0) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = selp->fromp();
                ಸ್ಥಿರಾಂಕ std::pair<Dfg_ಶೃಂಗ*, uint32_t> common = commonLSBs(ap, fromp);
                ಹಿಂತಿರುಗಿ {common.ಮೊದಲ, std::min(common.ಎರಡನೇ, selp->ಅಗಲ())};
            }
        }

        // Otherwise no common LSBs
        ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_ಶೃಂಗ*, uint32_t> commonMSBs(Dfg_ಶೃಂಗ* ap, Dfg_ಶೃಂಗ* bp) {
        ಆದರೆ (ap == bp) ಹಿಂತಿರುಗಿ {ap, ap->ಅಗಲ()};

        // If both constants, check MSBs
        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ aConstp = ap->cast<DfgConst>()) {
            ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ bConstp = bp->cast<DfgConst>()) {
                ಸ್ಥಿರಾಂಕ uint32_t aMsb = aConstp->ಅಗಲ() - 1;
                ಸ್ಥಿರಾಂಕ uint32_t bMsb = bConstp->ಅಗಲ() - 1;
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& a_ಅಂಕೆ = aConstp->ಅಂಕೆ();
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& b_ಅಂಕೆ = bConstp->ಅಂಕೆ();
                // Max match is the shorter constant
                ಸ್ಥಿರಾಂಕ uint32_t max_ಹೊಂದಿಕೆ = std::min(aConstp->ಅಗಲ(), bConstp->ಅಗಲ());
                // Check all bits
                uint32_t ಹೊಂದಿಕೆ_ಅಗಲ = 0;
                ಗಾಗಿ (; ಹೊಂದಿಕೆ_ಅಗಲ < max_ಹೊಂದಿಕೆ; ++ಹೊಂದಿಕೆ_ಅಗಲ) {
                    ಆದರೆ (a_ಅಂಕೆ.ಬಿಟ್_Is0(aMsb - ಹೊಂದಿಕೆ_ಅಗಲ) != b_ಅಂಕೆ.ಬಿಟ್_Is0(bMsb - ಹೊಂದಿಕೆ_ಅಗಲ)) ನಿಲ್ಲಿಸು;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ಸ್ಥಿರಾಂಕ shorterp = aMsb < bMsb ? aConstp : bConstp;
                ಹಿಂತಿರುಗಿ {ಹೊಂದಿಕೆ_ಅಗಲ ? shorterp : ಶೂನ್ಯಸೂಚಕ, ಹೊಂದಿಕೆ_ಅಗಲ};
            }
        }

        // If Concat, check against the LHS
        ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ catp = ap->cast<DfgConcat>()) ಹಿಂತಿರುಗಿ commonMSBs(catp->lhsp(), bp);
        ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ catp = bp->cast<DfgConcat>()) ಹಿಂತಿರುಗಿ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = ap->cast<DfgSel>()) {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = selp->fromp();
            ಆದರೆ (selp->msb() == fromp->ಅಗಲ() - 1) {
                ಸ್ಥಿರಾಂಕ std::pair<Dfg_ಶೃಂಗ*, uint32_t> common = commonMSBs(fromp, bp);
                ಹಿಂತಿರುಗಿ {common.ಮೊದಲ, std::min(common.ಎರಡನೇ, selp->ಅಗಲ())};
            }
        }
        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ selp = bp->cast<DfgSel>()) {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = selp->fromp();
            ಆದರೆ (selp->msb() == fromp->ಅಗಲ() - 1) {
                ಸ್ಥಿರಾಂಕ std::pair<Dfg_ಶೃಂಗ*, uint32_t> common = commonMSBs(ap, fromp);
                ಹಿಂತಿರುಗಿ {common.ಮೊದಲ, std::min(common.ಎರಡನೇ, selp->ಅಗಲ())};
            }
        }

        // Otherwise no common MSBs
        ಹಿಂತಿರುಗಿ {ಶೂನ್ಯಸೂಚಕ, 0};
    }

    // VISIT methods

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಶೃಂಗ*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಎಣಿಕೆ_Ones* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgExtend* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ zerop
                = makeZero(vtxp->fileline(), vtxp->ಅಗಲ() - vtxp->srcp()->ಅಗಲ());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ಹಿಂತಿರುಗಿ;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgExtendS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLogNot* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgNegate* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgNot* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ;

        // Not of Cond
        ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            ಆದರೆ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ತಳ್ಳು_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ಸ್ಥಿರಾಂಕ newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ಸ್ಥಿರಾಂಕ newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ಸ್ಥಿರಾಂಕ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        // Not of Not
        ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (!vtxp->srcp()->has_ಅಪವರ್ತ್ಯ_Sinks()) {
            // Not of Eq
            ಆದರೆ (DfgEq* ಸ್ಥಿರಾಂಕ eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            // Not of Neq
            ಆದರೆ (DfgNeq* ಸ್ಥಿರಾಂಕ neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            // Not of unsigned comparisons
            ಆದರೆ (DfgLt* ಸ್ಥಿರಾಂಕ p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (DfgGte* ಸ್ಥಿರಾಂಕ p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (DfgGt* ಸ್ಥಿರಾಂಕ p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (DfgLte* ಸ್ಥಿರಾಂಕ p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            // Not of signed comparisons
            ಆದರೆ (DfgLtS* ಸ್ಥಿರಾಂಕ p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (DfgGteS* ಸ್ಥಿರಾಂಕ p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (DfgGtS* ಸ್ಥಿರಾಂಕ p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (DfgLteS* ಸ್ಥಿರಾಂಕ p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgOneHot* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgOneHot0* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (foldUnary(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgRedOr* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (optimizeReduction(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgRedAnd* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (optimizeReduction(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgRedXor* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (optimizeReduction(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgSel* ಸ್ಥಿರಾಂಕ vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = vtxp->fromp();

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಸ್ಥಿರಾಂಕ uint32_t lsb = vtxp->lsb();
        ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = vtxp->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ uint32_t msb = lsb + ಅಗಲ - 1;

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ಸ್ಥಿರಾಂಕ resp = makeZero(flp, ಅಗಲ);
                resp->ಅಂಕೆ().opSel(constp->ಅಂಕೆ(), msb, lsb);
                replace(resp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        // Full width select, replace with the source.
        ಆದರೆ (fromp->ಅಗಲ() == ಅಗಲ) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_ತುಂಬಿದ_ಅಗಲ_SEL) {
                replace(fromp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        // Unwind through bit packing in one go
        {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ res = unwindSel(fromp, lsb, ಅಗಲ);
            ಆದರೆ (res.ಮೊದಲ != fromp || res.ಎರಡನೇ != lsb) {
                replace(make<DfgSel>(vtxp, res.ಮೊದಲ, res.ಎರಡನೇ));
                ಹಿಂತಿರುಗಿ;
            }
        }

        // Sel from Not
        ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            ಆದರೆ (!notp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                APPLYING(ತಳ್ಳು_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ಸ್ಥಿರಾಂಕ newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        // Sel from Cond
        ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ condp = fromp->cast<DfgCond>()) {
            ಆದರೆ (!condp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                APPLYING(ತಳ್ಳು_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ಸ್ಥಿರಾಂಕ newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ಸ್ಥಿರಾಂಕ newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ಸ್ಥಿರಾಂಕ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        // Sel from ShiftL
        ಆದರೆ (Dfg_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ ಸರಿಸು_Lp = fromp->cast<Dfg_ಸರಿಸು_L>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            ಆದರೆ (lsb == 0) {
                UASSERT_OBJ(ಸರಿಸು_Lp->lhsp()->ಅಗಲ() >= ಅಗಲ, vtxp, "input of shift narrow");
                APPLYING(ತಳ್ಳು_SEL_THROUGH_SHIFTL) {
                    DfgSel* ಸ್ಥಿರಾಂಕ newSelp = make<DfgSel>(vtxp, ಸರಿಸು_Lp->lhsp(), vtxp->lsb());
                    replace(make<Dfg_ಸರಿಸು_L>(vtxp, newSelp, ಸರಿಸು_Lp->rhsp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgMux* ಸ್ಥಿರಾಂಕ vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromp = vtxp->fromp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lsbp = vtxp->lsbp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->ಅಂಕೆ().ಗಮ್ಯ_UInt()));
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->ಅಗಲ()));
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->ಅಗಲ()));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    ಶೂನ್ಯ ಭೇಟಿ(DfgAnd* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ಆದರೆ (!lhsp->has_ಅಪವರ್ತ್ಯ_Sinks() || !rhsp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
            ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ lhsNotp = lhsp->cast<DfgNot>()) {
                ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_ದ_NOT_AND_NOT) {
                        DfgOr* ಸ್ಥಿರಾಂಕ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
                ಆದರೆ (DfgNeq* ಸ್ಥಿರಾಂಕ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_ದ_NOT_AND_NEQ) {
                        DfgEq* ಸ್ಥಿರಾಂಕ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ಸ್ಥಿರಾಂಕ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lConstp = lhsp->cast<DfgConst>()) {
            ಆದರೆ (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (distributive_And_Associative_ದ್ವಿಮಾನ<DfgOr, DfgAnd>(vtxp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Through_Reductions(vtxp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Throug_Sel(vtxp)) ಹಿಂತಿರುಗಿ;

        {
            DfgNot* ಸ್ಥಿರಾಂಕ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ಸ್ಥಿರಾಂಕ rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            ಆದರೆ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->ಅಗಲ()));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (DfgAnd* ಸ್ಥಿರಾಂಕ rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ಸ್ಥಿರಾಂಕ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ಸ್ಥಿರಾಂಕ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                ಆದರೆ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->ಅಗಲ()));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (vtxp->dtype() == m_ಬಿಟ್_DType) {
            ಆದರೆ (tryReplaceBitwiseWithReduction(vtxp)) ಹಿಂತಿರುಗಿ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            ಆದರೆ (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            ಆದರೆ (repp && condp && repp->srcp()->ಗಾತ್ರ_2() == 1 && !condp->has_ಅಪವರ್ತ್ಯ_Sinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ಸ್ಥಿರಾಂಕ newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgOr* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ಆದರೆ (!lhsp->has_ಅಪವರ್ತ್ಯ_Sinks() || !rhsp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
            ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ lhsNotp = lhsp->cast<DfgNot>()) {
                ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_ದ_NOT_AND_NOT) {
                        DfgAnd* ಸ್ಥಿರಾಂಕ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
                ಆದರೆ (DfgNeq* ಸ್ಥಿರಾಂಕ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_ದ_NOT_AND_NEQ) {
                        DfgEq* ಸ್ಥಿರಾಂಕ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ಸ್ಥಿರಾಂಕ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ lhsConcatp = lhsp->cast<DfgConcat>()) {
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಆದರೆ (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    ಆದರೆ (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_ದ_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                    ಆದರೆ (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_ದ_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                }
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lConstp = lhsp->cast<DfgConst>()) {
            ಆದರೆ (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (distributive_And_Associative_ದ್ವಿಮಾನ<DfgAnd, DfgOr>(vtxp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Through_Reductions(vtxp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Throug_Sel(vtxp)) ಹಿಂತಿರುಗಿ;

        {
            DfgNot* ಸ್ಥಿರಾಂಕ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ಸ್ಥಿರಾಂಕ rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            ಆದರೆ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->ಅಗಲ()));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (DfgOr* ಸ್ಥಿರಾಂಕ rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ಸ್ಥಿರಾಂಕ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ಸ್ಥಿರಾಂಕ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                ಆದರೆ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->ಅಗಲ()));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (vtxp->dtype() == m_ಬಿಟ್_DType) {
            ಆದರೆ (tryReplaceBitwiseWithReduction(vtxp)) ಹಿಂತಿರುಗಿ;
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgXor* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->ಅಗಲ()));
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lConstp = lhsp->cast<DfgConst>()) {
            ಆದರೆ (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rConcatp = rhsp->cast<DfgConcat>()) {
                ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Through_Reductions(vtxp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (try_ತಳ್ಳು_Bitwise_Op_Throug_Sel(vtxp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (vtxp->dtype() == m_ಬಿಟ್_DType) {
            ಆದರೆ (tryReplaceBitwiseWithReduction(vtxp)) ಹಿಂತಿರುಗಿ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isZero(lhsp)) {
            APPLYING(REMOVE_ಕೂಡಿಸು_ZERO) {
                replace(rhsp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಸ್ಥಿರಾಂಕ std::tuple<Dfg_ಶೃಂಗ*, uint32_t, uint32_t> l_ಪದ = ಕೂಡಿಸು_ಗಮ್ಯ_ಎಣಿಕೆ_Ones_ಪದ(lhsp);
        ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp = std::get<0>(l_ಪದ)) {
            std::tuple<Dfg_ಶೃಂಗ*, uint32_t, uint32_t> r_ಪದ = ಕೂಡಿಸು_ಗಮ್ಯ_ಎಣಿಕೆ_Ones_ಪದ(rhsp);
            Dfg_ಶೃಂಗ* extrap = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!std::get<0>(r_ಪದ)) {
                ಆದರೆ (Dfg_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ rAddp = rhsp->cast<Dfg_ಕೂಡಿಸು>()) {
                    r_ಪದ = ಕೂಡಿಸು_ಗಮ್ಯ_ಎಣಿಕೆ_Ones_ಪದ(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rVtxp = std::get<0>(r_ಪದ)) {
                ಆದರೆ (isSame(lVtxp, rVtxp)) {
                    ಸ್ಥಿರಾಂಕ uint32_t lLsb = std::get<1>(l_ಪದ);
                    ಸ್ಥಿರಾಂಕ uint32_t rLsb = std::get<1>(r_ಪದ);
                    ಸ್ಥಿರಾಂಕ uint32_t l_ಅಗಲ = std::get<2>(l_ಪದ);
                    ಸ್ಥಿರಾಂಕ uint32_t r_ಅಗಲ = std::get<2>(r_ಪದ);
                    ತಾರ್ಕಿಕ adjoined = ನಿಜ;
                    uint32_t lsb = 0;
                    ಆದರೆ (lLsb + l_ಅಗಲ == rLsb) {
                        lsb = lLsb;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lLsb == rLsb + r_ಅಗಲ) {
                        lsb = rLsb;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        adjoined = ಸುಳ್ಳು;
                    }
                    ಆದರೆ (adjoined) {
                        APPLYING(REPLACE_ಕೂಡಿಸು_WITH_ಎಣಿಕೆ_ONES) {
                            DfgSel* ಸ್ಥಿರಾಂಕ selp
                                = make<DfgSel>(vtxp->fileline(),
                                               Dfg_ದತ್ತಾಂಶ_Type::packed(l_ಅಗಲ + r_ಅಗಲ), lVtxp, lsb);
                            Dfg_ಶೃಂಗ* resp
                                = make<Dfg_ಎಣಿಕೆ_Ones>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(32), selp);
                            ಆದರೆ (vtxp->ಅಗಲ() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->ಅಗಲ() - 32U),
                                                       resp);
                            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vtxp->ಅಗಲ() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            ಆದರೆ (extrap) resp = make<Dfg_ಕೂಡಿಸು>(vtxp, resp, extrap);
                            replace(resp);
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgArraySel* ಸ್ಥಿರಾಂಕ vtxp) override {
        DfgConst* ಸ್ಥಿರಾಂಕ idxp = vtxp->bitp()->cast<DfgConst>();
        ಆದರೆ (!idxp) ಹಿಂತಿರುಗಿ;
        DfgVarArray* ಸ್ಥಿರಾಂಕ varp = vtxp->fromp()->cast<DfgVarArray>();
        ಆದರೆ (!varp) ಹಿಂತಿರುಗಿ;
        AstVar* ಸ್ಥಿರಾಂಕ astVarp = varp->vscp()->varp();
        ಆದರೆ (astVarp->isForced()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (astVarp->isSigUserRWPublic()) ಹಿಂತಿರುಗಿ;
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = varp->srcp();
        ಆದರೆ (!srcp) {
            ಆದರೆ (vtxp->isPacked()) {
                ಆದರೆ (AstConsArray* ಸ್ಥಿರಾಂಕ iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ valp = VN_CAST(iap->valuep(idxp->ಗಮ್ಯ_ಗಾತ್ರ_T()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_ಕೋಷ್ಟಕ) {
                            replace(ಹೊಸ DfgConst{m_dfg, valp->fileline(), valp->ಅಂಕೆ()});
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                }
            }
            ಹಿಂತಿರುಗಿ;
        }

        ಆದರೆ (DfgSpliceArray* ಸ್ಥಿರಾಂಕ splicep = srcp->cast<DfgSpliceArray>()) {
            Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ driverp = splicep->driverAt(idxp->ಗಮ್ಯ_ಗಾತ್ರ_T());
            ಆದರೆ (!driverp) ಹಿಂತಿರುಗಿ;
            DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = driverp->cast<DfgUnitArray>();
            ಆದರೆ (!uap) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (uap->srcp()->is<Dfg_ಶೃಂಗ_Splice>()) ಹಿಂತಿರುಗಿ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ಆದರೆ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ dvarp = uap->srcp()->cast<Dfg_ಶೃಂಗ_Var>()) {
                ಆದರೆ (dvarp->srcp()) ಹಿಂತಿರುಗಿ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (DfgUnitArray* ಸ್ಥಿರಾಂಕ uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->ಗಮ್ಯ_ಗಾತ್ರ_T() == 0, vtxp, "Array index out of range");
            ಆದರೆ (uap->srcp()->is<DfgSplicePacked>()) ಹಿಂತಿರುಗಿ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ಆದರೆ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ dvarp = uap->srcp()->cast<Dfg_ಶೃಂಗ_Var>()) {
                ಆದರೆ (dvarp->srcp()) ಹಿಂತಿರುಗಿ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgConcat* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isZero(lhsp)) {
            DfgConst* ಸ್ಥಿರಾಂಕ lConstp = lhsp->as<DfgConst>();
            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ rSelp = rhsp->cast<DfgSel>()) {
                ಆದರೆ (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->ಅಗಲ()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_ತುದಿ_WITH_SHIFTR) {
                        replace(
                            make<Dfg_ಸರಿಸು_R>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->ಅಗಲ())));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (isZero(rhsp)) {
            DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->as<DfgConst>();
            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lSelp = lhsp->cast<DfgSel>()) {
                ಆದರೆ (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<Dfg_ಸರಿಸು_L>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->ಅಗಲ())));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ lNot = lhsp->cast<DfgNot>()) {
            ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ rNot = rhsp->cast<DfgNot>()) {
                ಆದರೆ (!lNot->has_ಅಪವರ್ತ್ಯ_Sinks() && !rNot->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(ತಳ್ಳು_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ಸ್ಥಿರಾಂಕ newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        {
            ತಾರ್ಕಿಕ ಅದಲುಬದಲು = ಸುಳ್ಳು;
            ಆದರೆ (isConcatPushableThroughBitwise(lhsp, rhsp, ಅದಲುಬದಲು)) {
                APPLYING(ತಳ್ಳು_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, ಅದಲುಬದಲು));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rConcatp = rhsp->cast<DfgConcat>()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rlp = rConcatp->lhsp();
                ಆದರೆ (!rConcatp->has_ಅಪವರ್ತ್ಯ_Sinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, ಅದಲುಬದಲು)) {
                    APPLYING(ತಳ್ಳು_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, ಅದಲುಬದಲು);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lSelp = lhsp->cast<DfgSel>()) {
            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ rSelp = rhsp->cast<DfgSel>()) {
                ಆದರೆ (isSame(lSelp->fromp(), rSelp->fromp())) {
                    ಆದರೆ (lSelp->lsb() == rSelp->lsb() + rSelp->ಅಗಲ()) {
                        APPLYING(REMOVE_CONCAT_ದ_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                }
            }

            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rConcatp = rhsp->cast<DfgConcat>()) {
                ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    ಆದರೆ (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        ಆದರೆ (lSelp->lsb() == rlSelp->lsb() + rlSelp->ಅಗಲ()) {
                            APPLYING(REPLACE_NESTED_CONCAT_ದ_ADJOINING_SELS_ON_LHS) {
                                ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = lSelp->ಅಗಲ() + rlSelp->ಅಗಲ();
                                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = Dfg_ದತ್ತಾಂಶ_Type::packed(ಅಗಲ);
                                DfgSel* ಸ್ಥಿರಾಂಕ selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ಹಿಂತಿರುಗಿ;
                            }
                        }
                    }
                }
            }
        }

        ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ rSelp = rhsp->cast<DfgSel>()) {
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ lConcatp = lhsp->cast<DfgConcat>()) {
                ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    ಆದರೆ (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        ಆದರೆ (lrSelp->lsb() == rSelp->lsb() + rSelp->ಅಗಲ()) {
                            APPLYING(REPLACE_NESTED_CONCAT_ದ_ADJOINING_SELS_ON_RHS) {
                                ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = lrSelp->ಅಗಲ() + rSelp->ಅಗಲ();
                                ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = Dfg_ದತ್ತಾಂಶ_Type::packed(ಅಗಲ);
                                DfgSel* ಸ್ಥಿರಾಂಕ selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ಹಿಂತಿರುಗಿ;
                            }
                        }
                    }
                }
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lConstp = lhsp->cast<DfgConst>()) {
            ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ rCondp = rhsp->cast<DfgCond>()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rtVtxp = rCondp->thenp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ reVtxp = rCondp->elsep();
                DfgConst* ಸ್ಥಿರಾಂಕ rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ಸ್ಥಿರಾಂಕ reConstp = reVtxp->cast<DfgConst>();
                ಆದರೆ (!rCondp->has_ಅಪವರ್ತ್ಯ_Sinks() && (rtConstp || reConstp)) {
                    APPLYING(ತಳ್ಳು_CONCAT_THROUGH_COND_LHS) {
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ thenp = [&]() -> Dfg_ಶೃಂಗ* {
                            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ rtFlp = rtVtxp->fileline();
                            ಆದರೆ (rtConstp) {
                                DfgConst* ಸ್ಥಿರಾಂಕ constp = makeZero(rtFlp, vtxp->ಅಗಲ());
                                constp->ಅಂಕೆ().opConcat(lConstp->ಅಂಕೆ(), rtConstp->ಅಂಕೆ());
                                ಹಿಂತಿರುಗಿ constp;
                            }
                            ಹಿಂತಿರುಗಿ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ elsep = [&]() -> Dfg_ಶೃಂಗ* {
                            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ reFlp = reVtxp->fileline();
                            ಆದರೆ (reConstp) {
                                DfgConst* ಸ್ಥಿರಾಂಕ constp = makeZero(reFlp, vtxp->ಅಗಲ());
                                constp->ಅಂಕೆ().opConcat(lConstp->ಅಂಕೆ(), reConstp->ಅಂಕೆ());
                                ಹಿಂತಿರುಗಿ constp;
                            }
                            ಹಿಂತಿರುಗಿ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->cast<DfgConst>()) {
            ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ lCondp = lhsp->cast<DfgCond>()) {
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ltVtxp = lCondp->thenp();
                Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ leVtxp = lCondp->elsep();
                DfgConst* ಸ್ಥಿರಾಂಕ ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ಸ್ಥಿರಾಂಕ leConstp = leVtxp->cast<DfgConst>();
                ಆದರೆ (!lCondp->has_ಅಪವರ್ತ್ಯ_Sinks() && (ltConstp || leConstp)) {
                    APPLYING(ತಳ್ಳು_CONCAT_THROUGH_COND_RHS) {
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ thenp = [&]() -> Dfg_ಶೃಂಗ* {
                            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ ltFlp = ltVtxp->fileline();
                            ಆದರೆ (ltConstp) {
                                DfgConst* ಸ್ಥಿರಾಂಕ constp = makeZero(ltFlp, vtxp->ಅಗಲ());
                                constp->ಅಂಕೆ().opConcat(ltConstp->ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                                ಹಿಂತಿರುಗಿ constp;
                            }
                            ಹಿಂತಿರುಗಿ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ elsep = [&]() -> Dfg_ಶೃಂಗ* {
                            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ leFlp = leVtxp->fileline();
                            ಆದರೆ (leConstp) {
                                DfgConst* ಸ್ಥಿರಾಂಕ constp = makeZero(leFlp, vtxp->ಅಗಲ());
                                constp->ಅಂಕೆ().opConcat(leConstp->ಅಂಕೆ(), rConstp->ಅಂಕೆ());
                                ಹಿಂತಿರುಗಿ constp;
                            }
                            ಹಿಂತಿರುಗಿ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ಸ್ಥಿರಾಂಕ uint32_t vMsb = vtxp->ಅಗಲ() - 1;  // MSB of the concatenation
            ಸ್ಥಿರಾಂಕ uint32_t lLsb = vtxp->rhsp()->ಅಗಲ();  // LSB of the LHS
            ಸ್ಥಿರಾಂಕ uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            ತಾರ್ಕಿಕ hasCrossSink = ಸುಳ್ಳು;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](Dfg_ಶೃಂಗ& sink) {
                // Record bits used by DfgSel sinks
                ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgSel* ಸ್ಥಿರಾಂಕ selp = sink.cast<DfgSel>()) {
                    ಸ್ಥಿರಾಂಕ uint32_t selLsb = selp->lsb();
                    ಸ್ಥಿರಾಂಕ uint32_t selMsb = selLsb + selp->ಅಗಲ() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgVarPacked* ಸ್ಥಿರಾಂಕ varp = sink.cast<DfgVarPacked>()) {
                    ಆದರೆ (!varp->hasSinks() && !varp->isObserved()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ಹಿಂತಿರುಗಿ ನಿಜ;
            });
            ಆದರೆ (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    Dfg_ಶೃಂಗ* nRhsp = rhsp;
                    ಆದರೆ (lsb != 0)
                        nRhsp = make<DfgSel>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    Dfg_ಶೃಂಗ* nLhsp = lhsp;
                    ಆದರೆ (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ catp
                        = make<DfgConcat>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ಸ್ಥಿರಾಂಕ sp = ಹೊಸ DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_ಕೊನೆಯ_Id;
                    sp->ಕೂಡಿಸು_Driver(catp, lsb, flp);
                    Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_ಕೊನೆಯ_Id;
                    varp->vscp()->varp()->is_ಆಂತರಿಕ(ನಿಜ);
                    varp->srcp(sp);
                    replace(varp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        // Convert various forms to REPLICATE
        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ಹಿಂತಿರುಗಿ;
            }
        }
        ಆದರೆ (DfgRep* ಸ್ಥಿರಾಂಕ lRepp = lhsp->cast<DfgRep>()) {
            ಆದರೆ (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
        ಆದರೆ (DfgRep* ಸ್ಥಿರಾಂಕ rRepp = rhsp->cast<DfgRep>()) {
            ಆದರೆ (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
        ಆದರೆ (DfgRep* ಸ್ಥಿರಾಂಕ lRepp = lhsp->cast<DfgRep>()) {
            ಆದರೆ (DfgRep* ಸ್ಥಿರಾಂಕ rRepp = rhsp->cast<DfgRep>()) {
                ಆದರೆ (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rConcatp = rhsp->cast<DfgConcat>()) {
                ಆದರೆ (DfgRep* ಸ್ಥಿರಾಂಕ rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    ಆದರೆ (!rConcatp->has_ಅಪವರ್ತ್ಯ_Sinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ಸ್ಥಿರಾಂಕ uint32_t ಅಗಲ = lRepp->ಅಗಲ() + rlRepp->ಅಗಲ();
                            ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype = Dfg_ದತ್ತಾಂಶ_Type::packed(ಅಗಲ);
                            DfgRep* ಸ್ಥಿರಾಂಕ repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgDiv* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಆದರೆ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgDivS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ಆದರೆ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgGt* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgGtS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgGte* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgGteS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLogAnd* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp) || vtxp->rhsp()->unsafe()) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();

        ಆದರೆ (lhsp->ಅಗಲ() == 1 && rhsp->ಅಗಲ() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLogEq* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLogIf* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp) || vtxp->rhsp()->unsafe()) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLogOr* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp) || vtxp->rhsp()->unsafe()) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();

        ಆದರೆ (lhsp->ಅಗಲ() == 1 && rhsp->ಅಗಲ() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLt* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLtS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLte* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgLteS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgModDiv* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgModDivS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgMul* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lConstp = lhsp->cast<DfgConst>()) {
            ಆದರೆ (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgMulS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lConstp = lhsp->cast<DfgConst>()) {
            ಆದರೆ (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgEq* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lhsConstp = lhsp->cast<DfgConst>()) {
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಆದರೆ (try_ತಳ್ಳು_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ಹಿಂತಿರುಗಿ;
            }

            ಆದರೆ (rhsp->dtype() == m_ಬಿಟ್_DType) {
                ಆದರೆ (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_ಬಿಟ್_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }

                ಆದರೆ (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_ಬಿಟ್_1) {
                        replace(rhsp);
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgNeq* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ lhsConstp = lhsp->cast<DfgConst>()) {
            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ಆದರೆ (try_ತಳ್ಳು_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ಹಿಂತಿರುಗಿ;
            }

            ಆದರೆ (rhsp->dtype() == m_ಬಿಟ್_DType) {
                ಆದರೆ (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_ಬಿಟ್_0) {
                        replace(rhsp);
                        ಹಿಂತಿರುಗಿ;
                    }
                }

                ಆದರೆ (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_ಬಿಟ್_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgPow* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgPowSS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgPowSU* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgPowUS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgRep* ಸ್ಥಿರಾಂಕ vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ srcp = vtxp->srcp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ಸ್ಥಿರಾಂಕ resultp = makeZero(flp, vtxp->ಅಗಲ());
                resultp->ಅಂಕೆ().opRepl(sConstp->ಅಂಕೆ(), vtxp->ಎಣಿಕೆ_2());
                replace(resultp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (DfgRep* ಸ್ಥಿರಾಂಕ sRepp = srcp->cast<DfgRep>()) {
            ಆದರೆ (!sRepp->has_ಅಪವರ್ತ್ಯ_Sinks() || vtxp->ಅಗಲ() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (optimize_ಸರಿಸು_RHS(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();

        ಆದರೆ (Dfg_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ l_ಸರಿಸು_Lp = lhsp->cast<Dfg_ಸರಿಸು_L>()) {
            ಆದರೆ (!l_ಸರಿಸು_Lp->has_ಅಪವರ್ತ್ಯ_Sinks() && rhsp->dtype() == l_ಸರಿಸು_Lp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
                    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಮೊತ್ತ_DType = Dfg_ದತ್ತಾಂಶ_Type::packed(rhsp->ಅಗಲ() + 1);
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ bp = make<DfgExtend>(flp, ಮೊತ್ತ_DType, l_ಸರಿಸು_Lp->rhsp());
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cp = make<DfgExtend>(flp, ಮೊತ್ತ_DType, rhsp);
                    Dfg_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ addp = make<Dfg_ಕೂಡಿಸು>(flp, ಮೊತ್ತ_DType, bp, cp);
                    replace(make<Dfg_ಸರಿಸು_L>(vtxp, l_ಸರಿಸು_Lp->lhsp(), addp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->cast<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amount = rConstp->ಗಮ್ಯ_U32();

            ಆದರೆ (ಸರಿಸು_Amount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (ಸರಿಸು_Amount >= vtxp->ಅಗಲ()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->ಅಗಲ()));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ lConcatp = lhsp->cast<DfgConcat>()) {
                ಆದರೆ (!lConcatp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lRhsp = lConcatp->rhsp();
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ಸ್ಥಿರಾಂಕ uint32_t rW = ಸರಿಸು_Amount;
                        ಸ್ಥಿರಾಂಕ uint32_t mW = std::min(lRhsp->ಅಗಲ(), vtxp->ಅಗಲ() - ಸರಿಸು_Amount);
                        ಸ್ಥಿರಾಂಕ uint32_t lW = vtxp->ಅಗಲ() - mW - rW;
                        // Construct the result
                        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = lConcatp->fileline();
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rp = makeZero(flp, ಸರಿಸು_Amount);
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mp
                            = make<DfgSel>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(mW), lRhsp, 0U);
                        Dfg_ಶೃಂಗ* np = make<DfgConcat>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(mW + rW), mp, rp);
                        ಆದರೆ (!lW) {
                            replace(np);
                            ಹಿಂತಿರುಗಿ;
                        }
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lp
                            = make<DfgSel>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }

            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lSelp = lhsp->cast<DfgSel>()) {
                ಆದರೆ (!lSelp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ಸ್ಥಿರಾಂಕ uint32_t n_Sel_ಅಗಲ = lSelp->ಅಗಲ() - ಸರಿಸು_Amount;
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_ದತ್ತಾಂಶ_Type::packed(n_Sel_ಅಗಲ),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), ಸರಿಸು_Amount)));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }

            ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ lCondp = lhsp->cast<DfgCond>()) {
                ಆದರೆ (!lCondp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(ತಳ್ಳು_SHIFTL_THROUGH_COND) {
                        Dfg_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ tp = make<Dfg_ಸರಿಸು_L>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_ಸರಿಸು_L* ಸ್ಥಿರಾಂಕ ep = make<Dfg_ಸರಿಸು_L>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (optimize_ಸರಿಸು_RHS(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();

        ಆದರೆ (Dfg_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ l_ಸರಿಸು_Rp = lhsp->cast<Dfg_ಸರಿಸು_R>()) {
            ಆದರೆ (!l_ಸರಿಸು_Rp->has_ಅಪವರ್ತ್ಯ_Sinks() && rhsp->dtype() == l_ಸರಿಸು_Rp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();
                    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& ಮೊತ್ತ_DType = Dfg_ದತ್ತಾಂಶ_Type::packed(rhsp->ಅಗಲ() + 1);
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ bp = make<DfgExtend>(flp, ಮೊತ್ತ_DType, l_ಸರಿಸು_Rp->rhsp());
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cp = make<DfgExtend>(flp, ಮೊತ್ತ_DType, rhsp);
                    Dfg_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ addp = make<Dfg_ಕೂಡಿಸು>(flp, ಮೊತ್ತ_DType, bp, cp);
                    replace(make<Dfg_ಸರಿಸು_R>(vtxp, l_ಸರಿಸು_Rp->lhsp(), addp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->cast<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_Amount = rConstp->ಗಮ್ಯ_U32();

            ಆದರೆ (ಸರಿಸು_Amount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (ಸರಿಸು_Amount >= vtxp->ಅಗಲ()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->ಅಗಲ()));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (DfgConcat* ಸ್ಥಿರಾಂಕ lConcatp = lhsp->cast<DfgConcat>()) {
                ಆದರೆ (!lConcatp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lRhsp = lConcatp->rhsp();
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ಸ್ಥಿರಾಂಕ uint32_t lW = ಸರಿಸು_Amount;
                        ಸ್ಥಿರಾಂಕ uint32_t mW = std::min(lLhsp->ಅಗಲ(), vtxp->ಅಗಲ() - ಸರಿಸು_Amount);
                        ಸ್ಥಿರಾಂಕ uint32_t rW = vtxp->ಅಗಲ() - mW - lW;
                        // Construct the result
                        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = lConcatp->fileline();
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lp = makeZero(flp, ಸರಿಸು_Amount);
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ mp = make<DfgSel>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(mW), lLhsp,
                                                           lLhsp->ಅಗಲ() - mW);
                        Dfg_ಶೃಂಗ* np = make<DfgConcat>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(lW + mW), lp, mp);
                        ಆದರೆ (!rW) {
                            replace(np);
                            ಹಿಂತಿರುಗಿ;
                        }
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rp = make<DfgSel>(flp, Dfg_ದತ್ತಾಂಶ_Type::packed(rW), lRhsp,
                                                           lRhsp->ಅಗಲ() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }

            ಆದರೆ (DfgSel* ಸ್ಥಿರಾಂಕ lSelp = lhsp->cast<DfgSel>()) {
                ಆದರೆ (!lSelp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ಸ್ಥಿರಾಂಕ uint32_t n_Sel_ಅಗಲ = lSelp->ಅಗಲ() - ಸರಿಸು_Amount;
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_ದತ್ತಾಂಶ_Type::packed(n_Sel_ಅಗಲ),
                                           lSelp->fromp(), lSelp->lsb() + ಸರಿಸು_Amount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), ಸರಿಸು_Amount), nSelp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }

            ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ lCondp = lhsp->cast<DfgCond>()) {
                ಆದರೆ (!lCondp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(ತಳ್ಳು_SHIFTR_THROUGH_COND) {
                        Dfg_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ tp = make<Dfg_ಸರಿಸು_R>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_ಸರಿಸು_R* ಸ್ಥಿರಾಂಕ ep = make<Dfg_ಸರಿಸು_R>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಸರಿಸು_RS* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (optimize_ಸರಿಸು_RHS(vtxp)) ಹಿಂತಿರುಗಿ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(DfgSub* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (ದ್ವಿಮಾನ(vtxp)) ಹಿಂತಿರುಗಿ;

        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = vtxp->lhsp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = vtxp->rhsp();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ rConstp = rhsp->cast<DfgConst>()) {
            ಆದರೆ (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (vtxp->dtype() == m_ಬಿಟ್_DType && rConstp->has_ಮೌಲ್ಯ(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_ಬಿಟ್_DType, lhsp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಹೊಂದಿಕೆ_Masked* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_Varp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), Ast_ಹೊಂದಿಕೆ_Masked::fold(constp->ಅಂಕೆ(), ಹೊಂದಿಕೆ_Varp)));
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    ಶೂನ್ಯ ಭೇಟಿ(DfgCond* ಸ್ಥಿರಾಂಕ vtxp) override {
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ condp = vtxp->condp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ thenp = vtxp->thenp();
        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ elsep = vtxp->elsep();
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = vtxp->fileline();

        ಆದರೆ (condp->dtype() != m_ಬಿಟ್_DType) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ condNotp = condp->cast<DfgNot>()) {
            ಆದರೆ (!condp->has_ಅಪವರ್ತ್ಯ_Sinks() || condNotp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                APPLYING(ಅದಲುಬದಲು_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        ಆದರೆ (DfgNeq* ಸ್ಥಿರಾಂಕ condNeqp = condp->cast<DfgNeq>()) {
            ಆದರೆ (!condp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                APPLYING(ಅದಲುಬದಲು_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ಸ್ಥಿರಾಂಕ newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ thenNotp = thenp->cast<DfgNot>()) {
            ಆದರೆ (DfgNot* ಸ್ಥಿರಾಂಕ elseNotp = elsep->cast<DfgNot>()) {
                ಆದರೆ (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->has_ಅಪವರ್ತ್ಯ_Sinks() && !elseNotp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ಸ್ಥಿರಾಂಕ newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (DfgOr* ಸ್ಥಿರಾಂಕ condOrp = condp->cast<DfgOr>()) {
            ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ thenCondp = thenp->cast<DfgCond>()) {
                ಆದರೆ (!thenCondp->has_ಅಪವರ್ತ್ಯ_Sinks()) {
                    ಆದರೆ (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ಸ್ಥಿರಾಂಕ ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                    ಆದರೆ (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ಸ್ಥಿರಾಂಕ ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ಹಿಂತಿರುಗಿ;
                        }
                    }
                }
            }
        }

        ಆದರೆ (vtxp->ಅಗಲ() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            ಆದರೆ (Dfg_ಕೂಡಿಸು* ಸ್ಥಿರಾಂಕ thenAddp = thenp->cast<Dfg_ಕೂಡಿಸು>()) {
                ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    ಆದರೆ (constp->has_ಮೌಲ್ಯ(1)) {
                        ಆದರೆ (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ಸ್ಥಿರಾಂಕ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->ಅಗಲ() - 1), condp);
                                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ thenFlp = thenAddp->fileline();
                                replace(
                                    make<Dfg_ಕೂಡಿಸು>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ಹಿಂತಿರುಗಿ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            ಆದರೆ (DfgSub* ಸ್ಥಿರಾಂಕ thenSubp = thenp->cast<DfgSub>()) {
                ಆದರೆ (DfgConst* ಸ್ಥಿರಾಂಕ constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    ಆದರೆ (constp->has_ಮೌಲ್ಯ(1)) {
                        ಆದರೆ (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ಸ್ಥಿರಾಂಕ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->ಅಗಲ() - 1), condp);
                                File_ಸಾಲು* ಸ್ಥಿರಾಂಕ thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ಹಿಂತಿರುಗಿ;
                            }
                        }
                    }
                }
            }
        }

        ಆದರೆ (vtxp->dtype() == m_ಬಿಟ್_DType) {
            ಆದರೆ (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        ಆದರೆ (vtxp->ಅಗಲ() <= VL_QUADSIZE) {
            ಆದರೆ (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಆದರೆ (DfgOr* ಸ್ಥಿರಾಂಕ tOrp = thenp->cast<DfgOr>()) {
                ಆದರೆ (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ maskp = replicate(vtxp, condp);
                        DfgAnd* ಸ್ಥಿರಾಂಕ andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
                ಆದರೆ (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ maskp = replicate(vtxp, condp);
                        DfgAnd* ಸ್ಥಿರಾಂಕ andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }
        }

        ಆದರೆ (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ಸ್ಥಿರಾಂಕ std::pair<Dfg_ಶೃಂಗ*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            ಆದರೆ (cLSBs.ಮೊದಲ) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ಸ್ಥಿರಾಂಕ uint32_t r_ಅಗಲ = cLSBs.ಎರಡನೇ;
                    Dfg_ಶೃಂಗ* rhsp = cLSBs.ಮೊದಲ;
                    ಆದರೆ (r_ಅಗಲ != rhsp->ಅಗಲ()) {
                        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& rDtype = Dfg_ದತ್ತಾಂಶ_Type::packed(r_ಅಗಲ);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    ಆದರೆ (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ಹಿಂತಿರುಗಿ;
                    }
                    // Create new LHS
                    ಸ್ಥಿರಾಂಕ uint32_t l_ಅಗಲ = vtxp->ಅಗಲ() - r_ಅಗಲ;
                    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& lDtype = Dfg_ದತ್ತಾಂಶ_Type::packed(l_ಅಗಲ);
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lThenp = make<DfgSel>(flp, lDtype, thenp, r_ಅಗಲ);
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lElsep = make<DfgSel>(flp, lDtype, elsep, r_ಅಗಲ);
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ಹಿಂತಿರುಗಿ;
                }
            }

            ಸ್ಥಿರಾಂಕ std::pair<Dfg_ಶೃಂಗ*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            ಆದರೆ (cMSBs.ಮೊದಲ) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ಸ್ಥಿರಾಂಕ uint32_t l_ಅಗಲ = cMSBs.ಎರಡನೇ;
                    Dfg_ಶೃಂಗ* lhsp = cMSBs.ಮೊದಲ;
                    ಆದರೆ (l_ಅಗಲ != lhsp->ಅಗಲ()) {
                        ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& lDtype = Dfg_ದತ್ತಾಂಶ_Type::packed(l_ಅಗಲ);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->ಅಗಲ() - l_ಅಗಲ);
                    }
                    // If it's all the same, just replace
                    ಆದರೆ (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ಹಿಂತಿರುಗಿ;
                    }
                    // Create new RHS
                    ಸ್ಥಿರಾಂಕ uint32_t r_ಅಗಲ = vtxp->ಅಗಲ() - l_ಅಗಲ;
                    ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& rDtype = Dfg_ದತ್ತಾಂಶ_Type::packed(r_ಅಗಲ);
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        ಆದರೆ (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                Dfg_ಶೃಂಗ* resp = condp;
                ಆದರೆ (ಸ್ಥಿರಾಂಕ uint32_t extend = vtxp->ಅಗಲ() - 1) {
                    DfgConst* ಸ್ಥಿರಾಂಕ zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                Dfg_ಶೃಂಗ* resp = make<DfgNot>(condp, condp);
                ಆದರೆ (ಸ್ಥಿರಾಂಕ uint32_t extend = vtxp->ಅಗಲ() - 1) {
                    DfgConst* ಸ್ಥಿರಾಂಕ zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ tCondp = thenp->cast<DfgCond>()) {
            ಆದರೆ (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }

        ಆದರೆ (DfgCond* ಸ್ಥಿರಾಂಕ eCondp = elsep->cast<DfgCond>()) {
            ಆದರೆ (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ vtxp) override {
        ಆದರೆ (vtxp->hasSinks()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (vtxp->isObserved()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (vtxp->defaultp()) ಹಿಂತಿರುಗಿ;

        // If undriven, or driven from another var, it is completely redundant.
        ಆದರೆ (!vtxp->srcp() || vtxp->srcp()->is<Dfg_ಶೃಂಗ_Var>()) {
            APPLYING(REMOVE_VAR) {
                delete_ಶೃಂಗ(vtxp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು_One = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ keep = vtxp->srcp()->foreachSink([&](Dfg_ಶೃಂಗ& sink) -> ತಾರ್ಕಿಕ {
            // Ignore non-observable variable sinks. These can be eliminated.
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = sink.cast<Dfg_ಶೃಂಗ_Var>()) {
                ಆದರೆ (!varp->hasSinks() && !varp->isObserved()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            // Keep if found more than one sink
            ಆದರೆ (ಸಿಕ್ಕಿತು_One) ಹಿಂತಿರುಗಿ ನಿಜ;
            // Mark first sink found
            ಸಿಕ್ಕಿತು_One = ನಿಜ;
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        });
        ಆದರೆ (!keep) {
            APPLYING(REMOVE_VAR) {
                delete_ಶೃಂಗ(vtxp);
                ಹಿಂತಿರುಗಿ;
            }
        }
    }

    V3DfgPeephole(Dfg_ನಕ್ಷೆ& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.for_Each_ಶೃಂಗ([&](Dfg_ಶೃಂಗ& vtx) { m_vInfo[vtx].m_id = ++m_ಕೊನೆಯ_Id; });

        // Add all operation vertices to the cache
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) cache_ಶೃಂಗ(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.ಗಾತ್ರ_2() * 2);
        m_iterList.reserve(m_dfg.ಗಾತ್ರ_2() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ತಳ್ಳು_ಹಿಂಭಾಗ(ಶೂನ್ಯಸೂಚಕ);
        m_iterList.ತಳ್ಳು_ಹಿಂಭಾಗ(ಶೂನ್ಯಸೂಚಕ);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        ಗಾಗಿ (Dfg_ಶೃಂಗ_Var& vtx : m_dfg.varVertices()) ಕೂಡಿಸು_ಗಮ್ಯ_Work_List(&vtx);

        // Add all operation vertices to the work list
        ಗಾಗಿ (Dfg_ಶೃಂಗ& vtx : m_dfg.opVertices()) ಕೂಡಿಸು_ಗಮ್ಯ_Work_List(&vtx);

        // Process iteratively
        ಆಗ (ನಿಜ) {
            // Process the work list - keep the placeholder at index 0
            ಆಗ (m_workList.ಗಾತ್ರ_2() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.ಹಿಂಭಾಗ();
                m_workList.ತೆಗೆ_ಹಿಂಭಾಗ();
                ಆದರೆ (!m_vtxp) ಮುಂದುವರಿ;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_work_List_ಸೂಚ್ಯಂಕ = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                ಆದರೆ (Dfg_ಶೃಂಗ_Var* ಸ್ಥಿರಾಂಕ varp = m_vtxp->cast<Dfg_ಶೃಂಗ_Var>()) {
                    ಭೇಟಿ(varp);
                    ಮುಂದುವರಿ;
                }

                // Ast references are not considered
                ಆದರೆ (m_vtxp->is<Dfg_ಶೃಂಗ_Ast>()) ಮುಂದುವರಿ;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                ಆದರೆ (!m_vInfo[m_vtxp].m_is_Cached_ಶೃಂಗ) {
                    ಆದರೆ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ sampep = cache_ಶೃಂಗ(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            ಮುಂದುವರಿ;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            ಆದರೆ (m_iterList.ಗಾತ್ರ_2() == 1) ನಿಲ್ಲಿಸು;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                ಗಾತ್ರ_t begin = 0;
                ಗಾತ್ರ_t ಕೊನೆ = m_iterList.ಗಾತ್ರ_2();
                ಗಾಗಿ (ಗಾತ್ರ_t hops = 1; hops <= 4; ++hops) {
                    ಗಾಗಿ (ಗಾತ್ರ_t i = begin; i < ಕೊನೆ; ++i) {
                        Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = m_iterList[i];
                        ಆದರೆ (!vtxp) ಮುಂದುವರಿ;
                        vtxp->foreachSink([&](Dfg_ಶೃಂಗ& dst) {
                            ಕೂಡಿಸು_ಗಮ್ಯ_Iter_List(&dst);
                            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                        });
                        vtxp->foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
                            ಕೂಡಿಸು_ಗಮ್ಯ_Iter_List(&src);
                            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                        });
                    }
                    begin = ಕೊನೆ;
                    ಕೊನೆ = m_iterList.ಗಾತ್ರ_2();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : m_iterList) {
                ಆದರೆ (!vtxp) ಮುಂದುವರಿ;
                removeFromIterList(vtxp);
                ಕೂಡಿಸು_ಗಮ್ಯ_Work_List(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು APPLYING

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(Dfg_ನಕ್ಷೆ& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

ಶೂನ್ಯ V3DfgPasses::peephole(Dfg_ನಕ್ಷೆ& dfg, V3DfgPeepholeContext& ctx) {
    ಆದರೆ (!v3Global.opt.fDfgPeephole()) ಹಿಂತಿರುಗಿ;
    V3DfgPeephole::apply(dfg, ctx);
}
