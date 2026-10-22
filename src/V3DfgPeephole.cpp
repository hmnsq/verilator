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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgCache.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgDataType.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPeepholePatterns.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cctype>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
ਨਾਮਾਕਾਸ਼ {

ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Reduction>
ਬਣਤਰ Reduction_ਮੰਜ਼ਿਲ_Bitwise_Impl {};
ਸਾਂਚਾ <> ਬਣਤਰ Reduction_ਮੰਜ਼ਿਲ_Bitwise_Impl<DfgRedAnd> { ਵਰਤੋਂ type = DfgAnd; };
ਸਾਂਚਾ <> ਬਣਤਰ Reduction_ਮੰਜ਼ਿਲ_Bitwise_Impl<DfgRedOr>  { ਵਰਤੋਂ type = DfgOr;  };
ਸਾਂਚਾ <> ਬਣਤਰ Reduction_ਮੰਜ਼ਿਲ_Bitwise_Impl<DfgRedXor> { ਵਰਤੋਂ type = DfgXor; };
ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Reduction>
ਵਰਤੋਂ Reduction_ਮੰਜ਼ਿਲ_Bitwise = ਕਿਸਮ_ਨਾਂ Reduction_ਮੰਜ਼ਿਲ_Bitwise_Impl<T_Reduction>::type;

ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Bitwise>
ਬਣਤਰ Bitwise_ਮੰਜ਼ਿਲ_Reduction_Impl {};
ਸਾਂਚਾ <> ਬਣਤਰ Bitwise_ਮੰਜ਼ਿਲ_Reduction_Impl<DfgAnd> { ਵਰਤੋਂ type = DfgRedAnd; };
ਸਾਂਚਾ <> ਬਣਤਰ Bitwise_ਮੰਜ਼ਿਲ_Reduction_Impl<DfgOr>  { ਵਰਤੋਂ type = DfgRedOr;  };
ਸਾਂਚਾ <> ਬਣਤਰ Bitwise_ਮੰਜ਼ਿਲ_Reduction_Impl<DfgXor> { ਵਰਤੋਂ type = DfgRedXor; };
ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Reduction>
ਵਰਤੋਂ Bitwise_ਮੰਜ਼ਿਲ_Reduction = ਕਿਸਮ_ਨਾਂ Bitwise_ਮੰਜ਼ਿਲ_Reduction_Impl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
ਸਾਂਚਾ<ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
ਬਣਤਰ IsAssociative : ਜਨਤਕ std::false_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsAssociative<DfgAnd> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsAssociative<DfgOr>  : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsAssociative<DfgXor> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsAssociative<Dfg_ਜੋੜੋ> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsAssociative<DfgConcat> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsAssociative<DfgMul> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsAssociative<DfgMulS> : ਜਨਤਕ std::true_type {};

// Commutative binary operators (a op b == b op a)
ਸਾਂਚਾ<ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
ਬਣਤਰ IsCommutative : ਜਨਤਕ std::false_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsCommutative<DfgAnd> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsCommutative<DfgOr>  : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsCommutative<DfgXor> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsCommutative<Dfg_ਜੋੜੋ> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsCommutative<DfgEq> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsCommutative<DfgMul> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsCommutative<DfgMulS> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsCommutative<DfgNeq> : ਜਨਤਕ std::true_type {};

// Idempotent binary operators (a op a == a)
ਸਾਂਚਾ<ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
ਬਣਤਰ IsIdempotent : ਜਨਤਕ std::false_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsIdempotent<DfgAnd> : ਜਨਤਕ std::true_type {};
ਸਾਂਚਾ <> ਬਣਤਰ IsIdempotent<DfgOr>  : ਜਨਤਕ std::true_type {};

// Binary result dtype
ਸਾਂਚਾ<ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp);
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<Dfg_ਜੋੜੋ>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgAnd>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgConcat>     (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(lhsp->ਚੌੜਾਈ() + rhsp->ਚੌੜਾਈ()); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgDiv>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgDivS>       (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgEq>         (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgGt>         (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgGtS>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgGte>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgGteS>       (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgLogAnd>     (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgLogEq>      (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgLogIf>      (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgLogOr>      (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgLt>         (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgLtS>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgLte>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgLteS>       (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgModDiv>     (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgModDivS>    (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgMul>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgMulS>       (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgNeq>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ Dfg_ਅੰਕੜੇ_Type::packed(1); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgOr>         (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgPow>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgPowSS>      (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgPowSU>      (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgPowUS>      (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<Dfg_ਖਿਸਕਾਓ_L>     (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<Dfg_ਖਿਸਕਾਓ_R>     (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<Dfg_ਖਿਸਕਾਓ_RS>    (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgSub>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }
ਸਾਂਚਾ <> ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਨਤੀਜਾ_DType<DfgXor>        (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) { ਵਾਪਸ lhsp->dtype(); }

// Unary constant folding
ਸਾਂਚਾ<ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2> ਖਾਲੀ foldOp(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src);
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<Dfg_ਗਿਣਤੀ_Ones>  (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.op_ਗਿਣਤੀ_Ones(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgExtend>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgExtendS>    (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opExtendS(src, src.ਚੌੜਾਈ()); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLogNot>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opLogNot(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgNegate>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opNegate(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgNot>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opNot(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgOneHot>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opOneHot(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgOneHot0>    (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opOneHot0(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgRedAnd>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opRedAnd(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgRedOr>      (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opRedOr(src); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgRedXor>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& src) { ਬਾਹਰਲਾ.opRedXor(src); }

// Binary constant folding
ਸਾਂਚਾ<ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2> ਖਾਲੀ foldOp(V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<Dfg_ਜੋੜੋ>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.op_ਜੋੜੋ(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgAnd>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opAnd(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgConcat>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opConcat(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgDiv>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opDiv(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgDivS>       (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opDivS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgEq>         (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opEq(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgGt>         (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opGt(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgGtS>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opGtS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgGte>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opGte(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgGteS>       (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opGteS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLogAnd>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opLogAnd(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLogEq>      (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opLogEq(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLogIf>      (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opLogIf(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLogOr>      (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opLogOr(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLt>         (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opLt(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLtS>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opLtS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLte>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opLte(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgLteS>       (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opLteS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgModDiv>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opModDiv(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgModDivS>    (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opModDivS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgMul>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opMul(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgMulS>       (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opMulS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgNeq>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opNeq(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgOr>         (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opOr(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgPow>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opPow(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgPowSS>      (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opPowSS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgPowSU>      (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opPowSU(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgPowUS>      (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opPowUS(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<Dfg_ਖਿਸਕਾਓ_L>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_L(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<Dfg_ਖਿਸਕਾਓ_R>     (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_R(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<Dfg_ਖਿਸਕਾਓ_RS>    (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.op_ਖਿਸਕਾਓ_RS(lhs, rhs, lhs.ਚੌੜਾਈ()); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgSub>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opSub(lhs, rhs); }
ਸਾਂਚਾ <> ਖਾਲੀ foldOp<DfgXor>        (V3_ਸੰਖਿਆ& ਬਾਹਰਲਾ, ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਬਾਹਰਲਾ.opXor(lhs, rhs); }
}
// clang-format on

ਵਰਗ V3DfgPeephole final : ਜਨਤਕ DfgVisitor {
    // TYPES
    ਬਣਤਰ ਸ਼ੀਰਸ਼_Info final {
        ਮਾਪ_t m_work_List_ਸੂਚਕਾਂਕ = 0;  // Position of this vertx m_workList (0 means not in list)
        ਮਾਪ_t m_iter_List_ਸੂਚਕਾਂਕ = 0;  // Position of this vertx m_iterList (0 means not in list)
        ਮਾਪ_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        ਮਾਪ_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        ਤਾਰਕਿਕ m_is_Cached_ਸ਼ੀਰਸ਼ = ਝੂਠ;  // This vertex is the vertex cached for its operation
    };

    // STATE
    Dfg_ਗ੍ਰਾਫ& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ਅਚਲ Dfg_ਅੰਕੜੇ_Type& m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType = Dfg_ਅੰਕੜੇ_Type::packed(1);  // Common, so grab it up front
    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> m_workList;  // List of vertices processed in current interation
    std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<ਸ਼ੀਰਸ਼_Info> m_vInfo = m_dfg.makeUserMap<ਸ਼ੀਰਸ਼_Info>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    Dfg_ਸ਼ੀਰਸ਼* m_vtxp = ਖਾਲੀ_ਸੂਚਕ;  // Currently considered vertex
    ਮਾਪ_t m_currentGeneration = 0;  // Current generation number
    ਮਾਪ_t m_ਆਖਰੀ_Id = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ਅਚਲ m_tmpScopep = v3Global.rootp()->ਸਿਖਰ_Scopep()->scopep();

    // STATIC STATE
    ਸਥਿਰ V3DebugBisect s_debugBisect;  // Debug aid

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ APPLYING(id) ਜੇ (checkApplying(VDfg_Peephole_ਨਮੂਨਾ::id))

    // METHODS
    ਤਾਰਕਿਕ checkApplying(VDfg_Peephole_ਨਮੂਨਾ id) {
        ਜੇ (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ਵਾਪਸ ਝੂਠ;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_ਗਿਣਤੀ[id];
        ਵਾਪਸ ਸੱਚ;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    Dfg_ਸ਼ੀਰਸ਼* cache_ਸ਼ੀਰਸ਼(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_is_Cached_ਸ਼ੀਰਸ਼ || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_is_Cached_ਸ਼ੀਰਸ਼ = !equivp;
        ਵਾਪਸ equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    ਖਾਲੀ invalidate_ਸ਼ੀਰਸ਼(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_is_Cached_ਸ਼ੀਰਸ਼ = ਝੂਠ;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2, ਕਿਸਮ_ਨਾਂ... Operands>
    ਸ਼ੀਰਸ਼_2* get_Or_Create_ਸ਼ੀਰਸ਼(File_ਸਤਰ* flp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, Operands... operands) {
        ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp = m_cache.getOrCreate<ਸ਼ੀਰਸ਼_2, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_is_Cached_ਸ਼ੀਰਸ਼ = ਸੱਚ;
        ਵਾਪਸ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2, ਕਿਸਮ_ਨਾਂ... Operands>
    ਸ਼ੀਰਸ਼_2* get_ਸ਼ੀਰਸ਼(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, Operands... operands) {
        ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp = m_cache.get<ਸ਼ੀਰਸ਼_2>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_is_Cached_ਸ਼ੀਰਸ਼, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ਵਾਪਸ vtxp;
    }

    ਖਾਲੀ incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    ਖਾਲੀ ਜੋੜੋ_ਮੰਜ਼ਿਲ_Work_List(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਸ਼ੀਰਸ਼_Info& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        ਜੇ (vInfo.m_work_List_ਸੂਚਕਾਂਕ) ਵਾਪਸ;
        // Add to work list
        vInfo.m_work_List_ਸੂਚਕਾਂਕ = m_workList.ਮਾਪ();
        m_workList.ਧੱਕੋ_ਪਿੱਠ(vtxp);
    }

    ਖਾਲੀ removeFromWorkList(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਸ਼ੀਰਸ਼_Info& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_work_List_ਸੂਚਕਾਂਕ] = ਖਾਲੀ_ਸੂਚਕ;
        vInfo.m_work_List_ਸੂਚਕਾਂਕ = 0;
    }

    ਖਾਲੀ ਜੋੜੋ_ਮੰਜ਼ਿਲ_Iter_List(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਸ਼ੀਰਸ਼_Info& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        ਜੇ (vInfo.m_iter_List_ਸੂਚਕਾਂਕ) ਵਾਪਸ;
        // Add to iter list
        vInfo.m_iter_List_ਸੂਚਕਾਂਕ = m_iterList.ਮਾਪ();
        m_iterList.ਧੱਕੋ_ਪਿੱਠ(vtxp);
    }

    ਖਾਲੀ removeFromIterList(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਸ਼ੀਰਸ਼_Info& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iter_List_ਸੂਚਕਾਂਕ] = ਖਾਲੀ_ਸੂਚਕ;
        vInfo.m_iter_List_ਸੂਚਕਾਂਕ = 0;
    }

    ਖਾਲੀ delete_ਸ਼ੀਰਸ਼(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_work_List_ਸੂਚਕਾਂਕ, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = vtxp->cast<Dfg_ਸ਼ੀਰਸ਼_Var>();
        UASSERT_OBJ(!varp || !varp->hasPrev(), vtxp, "Deleting variable consumed via DfgPrev");

        // Invalidate cache entry
        invalidate_ਸ਼ੀਰਸ਼(vtxp);

        // It might be in the iter list, remove it
        removeFromIterList(vtxp);

        // Gather source vertices - they might be duplicates, make unique using generation number
        incrementGeneration();
        std::ਸਦਿਸ਼<Dfg_ਸ਼ੀਰਸ਼*> srcps;
        srcps.reserve(vtxp->nInputs());
        vtxp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
            // If it's a variable, add to work list to see if it became redundant
            ਜੇ (src.is<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                ਜੋੜੋ_ਮੰਜ਼ਿਲ_Work_List(&src);
                ਵਾਪਸ ਝੂਠ;
            }
            // Gather unique sources
            ਸ਼ੀਰਸ਼_Info& vInfo = m_vInfo[src];
            ਜੇ (vInfo.m_generation != m_currentGeneration) srcps.ਧੱਕੋ_ਪਿੱਠ(&src);
            vInfo.m_generation = m_currentGeneration;
            ਵਾਪਸ ਝੂਠ;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        ਜੇ (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ਧੱਕੋ_ਪਿੱਠ(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } ਨਹੀਂਤਾਂ {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ਅਚਲ ਸਵੈਚਾਲਿਤ mid
            = std::stable_ਹਿੱਸੇ_ਕਰੋ(srcps.begin(), srcps.ਸਿਰਾ(), [](Dfg_ਸ਼ੀਰਸ਼* srcp) {  //
                  ਵਾਪਸ srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        ਲਈ (ਸਵੈਚਾਲਿਤ it = srcps.begin(); it != mid; ++it) ਜੋੜੋ_ਮੰਜ਼ਿਲ_Iter_List(*it);

        // Recursively delete unused sources
        ਲਈ (ਸਵੈਚਾਲਿਤ it = mid; it != srcps.ਸਿਰਾ(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            delete_ਸ਼ੀਰਸ਼(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    ਖਾਲੀ replace(Dfg_ਸ਼ੀਰਸ਼* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_work_List_ਸੂਚਕਾਂਕ, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ਅਚਲ ਸਵੈਚਾਲਿਤ debugCallback = [&]() -> ਖਾਲੀ {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.ਸਰੋਤ_Cone({m_vtxp, resp});
            ਅਚਲ ਸਵੈਚਾਲਿਤ cone = m_dfg.ਸਰੋਤ_Cone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& v) {  //
                ਵਾਪਸ cone->ਗਿਣਤੀ_2(&v);
            });
        };
        ਜੇ (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ਵਾਪਸ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Iter_List(&src);
            ਵਾਪਸ ਝੂਠ;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& dst) {
            invalidate_ਸ਼ੀਰਸ਼(&dst);
            ਵਾਪਸ ਝੂਠ;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& dst) {
            cache_ਸ਼ੀਰਸ਼(&dst);
            ਵਾਪਸ ਝੂਠ;
        });

        // Original vertex is now unused, so delete it
        delete_ਸ਼ੀਰਸ਼(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Iter_List(resp);
        // Add new vertex and sinks to work list as likely to match
        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Work_List(resp);
        resp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& dst) {
            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Work_List(&dst);
            ਵਾਪਸ ਝੂਠ;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_ਸਤਰ* flp, uint32_t val) { ਵਾਪਸ ਨਵਾਂ DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_ਸਤਰ* flp, uint32_t ਚੌੜਾਈ) {
        ਵਾਪਸ ਨਵਾਂ DfgConst{m_dfg, flp, ਚੌੜਾਈ, 0};
    }
    DfgConst* makeZero(File_ਸਤਰ* flp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype) {
        ਵਾਪਸ makeZero(flp, dtype.ਮਾਪ());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_ਸਤਰ* flp, uint32_t ਚੌੜਾਈ) {
        DfgConst* ਅਚਲ resp = makeZero(flp, ਚੌੜਾਈ);
        resp->ਅੰਕ().ਸੈੱਟ_All_Bits1();
        ਵਾਪਸ resp;
    }

    // Create a new vertex of the given type
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2, ਕਿਸਮ_ਨਾਂ... Operands>
    ਸ਼ੀਰਸ਼_2* make(File_ਸਤਰ* flp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp = get_Or_Create_ਸ਼ੀਰਸ਼<ਸ਼ੀਰਸ਼_2, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        ਜੇ (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        ਸ਼ੀਰਸ਼_Info& vInfo = m_vInfo[vtxp];
        ਜੇ (!vInfo.m_id) vInfo.m_id = ++m_ਆਖਰੀ_Id;
        // Add to work list
        ਜੋੜੋ_ਮੰਜ਼ਿਲ_Work_List(vtxp);
        // Return new node
        ਵਾਪਸ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2, ਕਿਸਮ_ਨਾਂ... Operands>
    ਸ਼ੀਰਸ਼_2* make(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* examplep, Operands... operands) {
        ਵਾਪਸ make<ਸ਼ੀਰਸ਼_2>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    Dfg_ਸ਼ੀਰਸ਼* replicate(Dfg_ਸ਼ੀਰਸ਼* vtxp, Dfg_ਸ਼ੀਰਸ਼* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, vtxp, "Expected 1-bit vertex");
        ਜੇ (vtxp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) ਵਾਪਸ bitp;
        ਵਾਪਸ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ਸਥਿਰ ਤਾਰਕਿਕ isSame(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ap, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* bp) {
        ਜੇ (ap == bp) ਵਾਪਸ ਸੱਚ;
        ਜੇ (ap->dtype() != bp->dtype()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ DfgConst* ਅਚਲ aConstp = ap->cast<DfgConst>();
        ਜੇ (!aConstp) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ DfgConst* ਅਚਲ bConstp = bp->cast<DfgConst>();
        ਜੇ (!bConstp) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ aConstp->ਅੰਕ().isCaseEq(bConstp->ਅੰਕ());
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isZero(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ constp = vtxp->cast<DfgConst>()) ਵਾਪਸ constp->isZero();
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isOnes(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ constp = vtxp->cast<DfgConst>()) ਵਾਪਸ constp->isOnes();
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isEqOne(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਜੇ (ਅਚਲ DfgConst* ਅਚਲ constp = vtxp->cast<DfgConst>()) ਵਾਪਸ constp->ਅੰਕ().isEqOne();
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ areAdjacent(uint32_t& lsb, ਅਚਲ DfgSel* lSelp, ਅਚਲ DfgSel* rSelp) {
        ਜੇ (!isSame(lSelp->srcp(), rSelp->srcp())) ਵਾਪਸ ਝੂਠ;
        ਜੇ (lSelp->lsb() + lSelp->ਚੌੜਾਈ() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (lSelp->lsb() == rSelp->lsb() + rSelp->ਚੌੜਾਈ()) {
            lsb = rSelp->lsb();
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ foldUnary(ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_Unary, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Must invoke on unary");
        static_assert(std::is_final<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Must invoke on final class");
        ਜੇ (DfgConst* ਅਚਲ srcp = vtxp->srcp()->ਸਾਂਚਾ cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ਅਚਲ resultp = makeZero(vtxp->fileline(), vtxp->ਚੌੜਾਈ());
                foldOp<ਸ਼ੀਰਸ਼_2>(resultp->ਅੰਕ(), srcp->ਅੰਕ());
                replace(resultp);
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ ਦੁਆਧਾਰੀ(ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Must invoke on binary");
        static_assert(std::is_final<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Must invoke on final class");

        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
        // LHS/RHS of the vertex
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ਅਚਲ lConstp = lhsp->cast<DfgConst>();
        DfgConst* ਅਚਲ rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        ਸ਼ੀਰਸ਼_2* ਅਚਲ lSamep = lhsp->cast<ਸ਼ੀਰਸ਼_2>();
        ਸ਼ੀਰਸ਼_2* ਅਚਲ rSamep = rhsp->cast<ਸ਼ੀਰਸ਼_2>();

        // Constant folding
        ਜੇ (lConstp && rConstp) {
            APPLYING(FOLD_ਦੁਆਧਾਰੀ) {
                DfgConst* ਅਚਲ resultp = makeZero(flp, ਨਤੀਜਾ_DType<ਸ਼ੀਰਸ਼_2>(lhsp, rhsp));
                foldOp<ਸ਼ੀਰਸ਼_2>(resultp->ਅੰਕ(), lConstp->ਅੰਕ(), rConstp->ਅੰਕ());
                replace(resultp);
                ਵਾਪਸ ਸੱਚ;
            }
        }

        // Idempotent
        ਜੇ VL_CONSTEXPR_CXX17 (IsIdempotent<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ) {
            ਜੇ (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ਦੁਆਧਾਰੀ) {
                    replace(lhsp);
                    ਵਾਪਸ ਸੱਚ;
                }
            }
        }

        // Associative
        ਜੇ VL_CONSTEXPR_CXX17 (IsAssociative<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ) {
            // Also commutative and idempotent
            ਜੇ VL_CONSTEXPR_CXX17 (IsCommutative<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ && IsIdempotent<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ) {
                ਜੇ (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ਦੁਆਧਾਰੀ_LHS) {
                        replace(lSamep);
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
                ਜੇ (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ਦੁਆਧਾਰੀ_RHS) {
                        replace(rSamep);
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
            }

            ਜੇ (lConstp && rSamep) {
                ਜੇ (DfgConst* ਅਚਲ rlConstp = rSamep->lhsp()->ਸਾਂਚਾ cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ਦੁਆਧਾਰੀ_LHS_ਦਾ_RHS) {
                        DfgConst* ਅਚਲ cp = makeZero(flp, ਨਤੀਜਾ_DType<ਸ਼ੀਰਸ਼_2>(lConstp, rlConstp));
                        foldOp<ਸ਼ੀਰਸ਼_2>(cp->ਅੰਕ(), lConstp->ਅੰਕ(), rlConstp->ਅੰਕ());
                        replace(make<ਸ਼ੀਰਸ਼_2>(vtxp, cp, rSamep->rhsp()));
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
            }

            ਜੇ (lSamep && rConstp) {
                ਜੇ (DfgConst* ਅਚਲ lrConstp = lSamep->rhsp()->ਸਾਂਚਾ cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ਦੁਆਧਾਰੀ_RHS_ਦਾ_LHS) {
                        DfgConst* ਅਚਲ cp = makeZero(flp, ਨਤੀਜਾ_DType<ਸ਼ੀਰਸ਼_2>(lrConstp, rConstp));
                        foldOp<ਸ਼ੀਰਸ਼_2>(cp->ਅੰਕ(), lrConstp->ਅੰਕ(), rConstp->ਅੰਕ());
                        replace(make<ਸ਼ੀਰਸ਼_2>(vtxp, lSamep->lhsp(), cp));
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            ਜੇ (lSamep && !lSamep->has_ਗੁਣਜ_Sinks()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ ap = lSamep->lhsp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ bp = lSamep->rhsp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                ਜੇ (ap != cp && bp != cp) {
                    APPLYING(ਸੱਜਾ_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਬੱਚਾ_DType = ਨਤੀਜਾ_DType<ਸ਼ੀਰਸ਼_2>(bp, cp);
                        ਸ਼ੀਰਸ਼_2* ਅਚਲ bcp = make<ਸ਼ੀਰਸ਼_2>(vtxp->fileline(), ਬੱਚਾ_DType, bp, cp);
                        replace(make<ਸ਼ੀਰਸ਼_2>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
            }

            ਜੇ (rSamep && !rSamep->has_ਗੁਣਜ_Sinks()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rlVtxp = rSamep->lhsp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rrVtxp = rSamep->rhsp();

                ਜੇ VL_CONSTEXPR_CXX17 (IsCommutative<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ) {
                    ਜੇ (!lhsp->has_ਗੁਣਜ_Sinks() && rlVtxp->has_ਗੁਣਜ_Sinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<ਸ਼ੀਰਸ਼_2>(vtxp, rlVtxp, make<ਸ਼ੀਰਸ਼_2>(vtxp, lhsp, rrVtxp)));
                            ਵਾਪਸ ਸੱਚ;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                ਜੇ (ਸ਼ੀਰਸ਼_2* ਅਚਲ existingp
                    = get_ਸ਼ੀਰਸ਼<ਸ਼ੀਰਸ਼_2>(ਨਤੀਜਾ_DType<ਸ਼ੀਰਸ਼_2>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    ਜੇ (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ਦੁਆਧਾਰੀ_LHS_WITH_LHS_ਦਾ_RHS) {
                            replace(make<ਸ਼ੀਰਸ਼_2>(vtxp, existingp, rrVtxp));
                            ਵਾਪਸ ਸੱਚ;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                ਜੇ VL_CONSTEXPR_CXX17 (IsCommutative<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ) {
                    ਜੇ (ਸ਼ੀਰਸ਼_2* ਅਚਲ existingp
                        = get_ਸ਼ੀਰਸ਼<ਸ਼ੀਰਸ਼_2>(ਨਤੀਜਾ_DType<ਸ਼ੀਰਸ਼_2>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        ਜੇ (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ਦੁਆਧਾਰੀ_LHS_WITH_RHS_ਦਾ_RHS) {
                                replace(make<ਸ਼ੀਰਸ਼_2>(vtxp, existingp, rlVtxp));
                                ਵਾਪਸ ਸੱਚ;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        ਜੇ VL_CONSTEXPR_CXX17 (IsCommutative<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            ਜੇ (lConstp) {
                ਜੇ (DfgCond* ਅਚਲ rCondp = rhsp->cast<DfgCond>()) {
                    ਜੇ (!rCondp->has_ਗੁਣਜ_Sinks()) {
                        APPLYING(ਧੱਕੋ_COMMUTATIVE_ਦੁਆਧਾਰੀ_THROUGH_COND) {
                            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ tp = make<ਸ਼ੀਰਸ਼_2>(vtxp, lConstp, rCondp->thenp());
                            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ ep = make<ਸ਼ੀਰਸ਼_2>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ਵਾਪਸ ਸੱਚ;
                        }
                    }
                }
                ਵਾਪਸ ਝੂਠ;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ਅਚਲ ਤਾਰਕਿਕ ਅਦਲਾ_ਬਦਲੀ_Sides = [&]() {
            // Don't swap if not commutative, duh
            ਜੇ VL_CONSTEXPR_CXX17 (!IsCommutative<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ) ਵਾਪਸ ਝੂਠ;
            // Const go on left
            ਜੇ (lConstp) ਵਾਪਸ ਝੂਠ;
            ਜੇ (rConstp) ਵਾਪਸ ਸੱਚ;
            // Not go on left
            ਜੇ (lhsp->is<DfgNot>()) ਵਾਪਸ ਝੂਠ;
            ਜੇ (rhsp->is<DfgNot>()) ਵਾਪਸ ਸੱਚ;
            // Same go on right
            ਜੇ (rSamep) ਵਾਪਸ ਝੂਠ;
            ਜੇ (lSamep) ਵਾਪਸ ਸੱਚ;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ਅਚਲ ਸ਼ੀਰਸ਼_Info& lInfo = m_vInfo[lhsp];
            ਅਚਲ ਸ਼ੀਰਸ਼_Info& rInfo = m_vInfo[rhsp];
            ਵਾਪਸ lInfo.m_id > rInfo.m_id;
        }();
        ਜੇ (ਅਦਲਾ_ਬਦਲੀ_Sides) {
            APPLYING(ਅਦਲਾ_ਬਦਲੀ_SIDES_ਵਿੱਚ_ਦੁਆਧਾਰੀ) {
                replace(make<ਸ਼ੀਰਸ਼_2>(vtxp, rhsp, lhsp));
                ਵਾਪਸ ਸੱਚ;
            }
        }

        ਵਾਪਸ ਝੂਠ;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Other, ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ distributive_And_Associative_ਦੁਆਧਾਰੀ(ਸ਼ੀਰਸ਼_2* vtxp) {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        ਜੇ (!lhsp->has_ਗੁਣਜ_Sinks() && !rhsp->has_ਗੁਣਜ_Sinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            ਜੇ (Other* ਅਚਲ lp = lhsp->cast<Other>()) {
                ਜੇ (Other* ਅਚਲ rp = rhsp->cast<Other>()) {
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ llp = lp->lhsp();
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lrp = lp->rhsp();
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rlp = rp->lhsp();
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rrp = rp->rhsp();
                    Dfg_ਸ਼ੀਰਸ਼* ap = ਖਾਲੀ_ਸੂਚਕ;
                    Dfg_ਸ਼ੀਰਸ਼* bp = ਖਾਲੀ_ਸੂਚਕ;
                    Dfg_ਸ਼ੀਰਸ਼* cp = ਖਾਲੀ_ਸੂਚਕ;
                    ਜੇ (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } ਨਹੀਂਤਾਂ ਜੇ (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } ਨਹੀਂਤਾਂ ਜੇ (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } ਨਹੀਂਤਾਂ ਜੇ (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    ਜੇ (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_ਦੁਆਧਾਰੀ) {
                            replace(make<Other>(vtxp, ap, make<ਸ਼ੀਰਸ਼_2>(lhsp, bp, cp)));
                            ਵਾਪਸ ਸੱਚ;
                        }
                    }
                }
            }
        }

        ਵਾਪਸ ਝੂਠ;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ
    try_ਧੱਕੋ_Bitwise_Op_Through_Concat(ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ਅਚਲ catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ਅਚਲ catRConstp = concatp->rhsp()->cast<DfgConst>();
        ਜੇ (catLConstp || catRConstp) {
            APPLYING(ਧੱਕੋ_BITWISE_OP_THROUGH_CONCAT) {
                ਅਚਲ uint32_t ਚੌੜਾਈ = concatp->ਚੌੜਾਈ();
                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& lDtype = concatp->lhsp()->dtype();
                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& rDtype = concatp->rhsp()->dtype();
                ਅਚਲ uint32_t l_ਚੌੜਾਈ = lDtype.ਮਾਪ();
                ਅਚਲ uint32_t r_ਚੌੜਾਈ = rDtype.ਮਾਪ();

                // The new Lhs vertex
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ newLhsp = [&]() -> Dfg_ਸ਼ੀਰਸ਼* {
                    DfgConst* ਅਚਲ newLhsConstp = makeZero(constp->fileline(), l_ਚੌੜਾਈ);
                    ਜੇ (catLConstp) {
                        V3_ਸੰਖਿਆ ਅੰਕ{constp->fileline(), static_cast<ਪੂਰਨ>(l_ਚੌੜਾਈ), 0u};
                        ਅੰਕ.opSel(constp->ਅੰਕ(), ਚੌੜਾਈ - 1, r_ਚੌੜਾਈ);
                        foldOp<ਸ਼ੀਰਸ਼_2>(newLhsConstp->ਅੰਕ(), ਅੰਕ, catLConstp->ਅੰਕ());
                        ਵਾਪਸ newLhsConstp;
                    }
                    newLhsConstp->ਅੰਕ().opSel(constp->ਅੰਕ(), ਚੌੜਾਈ - 1, r_ਚੌੜਾਈ);
                    ਵਾਪਸ make<ਸ਼ੀਰਸ਼_2>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ newRhsp = [&]() -> Dfg_ਸ਼ੀਰਸ਼* {
                    DfgConst* ਅਚਲ newRhsConstp = makeZero(constp->fileline(), r_ਚੌੜਾਈ);
                    ਜੇ (catRConstp) {
                        V3_ਸੰਖਿਆ ਅੰਕ{constp->fileline(), static_cast<ਪੂਰਨ>(r_ਚੌੜਾਈ), 0u};
                        ਅੰਕ.opSel(constp->ਅੰਕ(), r_ਚੌੜਾਈ - 1, 0);
                        foldOp<ਸ਼ੀਰਸ਼_2>(newRhsConstp->ਅੰਕ(), ਅੰਕ, catRConstp->ਅੰਕ());
                        ਵਾਪਸ newRhsConstp;
                    }
                    newRhsConstp->ਅੰਕ().opSel(constp->ਅੰਕ(), r_ਚੌੜਾਈ - 1, 0);
                    ਵਾਪਸ make<ਸ਼ੀਰਸ਼_2>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਸ਼ੀਰਸ਼_2>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ
    try_ਧੱਕੋ_Compare_Op_Through_Concat(ਸ਼ੀਰਸ਼_2* ਅਚਲ vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        ਜੇ (!concatp->has_ਗੁਣਜ_Sinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ਧੱਕੋ_COMPARE_OP_THROUGH_CONCAT) {
                ਅਚਲ uint32_t ਚੌੜਾਈ = concatp->ਚੌੜਾਈ();
                ਅਚਲ uint32_t l_ਚੌੜਾਈ = concatp->lhsp()->ਚੌੜਾਈ();
                ਅਚਲ uint32_t r_ਚੌੜਾਈ = concatp->rhsp()->ਚੌੜਾਈ();

                // The new Lhs vertex
                DfgConst* ਅਚਲ newLhsConstp = makeZero(constp->fileline(), l_ਚੌੜਾਈ);
                newLhsConstp->ਅੰਕ().opSel(constp->ਅੰਕ(), ਚੌੜਾਈ - 1, r_ਚੌੜਾਈ);
                ਸ਼ੀਰਸ਼_2* ਅਚਲ newLhsp
                    = make<ਸ਼ੀਰਸ਼_2>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ਅਚਲ newRhsConstp = makeZero(constp->fileline(), r_ਚੌੜਾਈ);
                newRhsConstp->ਅੰਕ().opSel(constp->ਅੰਕ(), r_ਚੌੜਾਈ - 1, 0);
                ਸ਼ੀਰਸ਼_2* ਅਚਲ newRhsp
                    = make<ਸ਼ੀਰਸ਼_2>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                ਜੇ VL_CONSTEXPR_CXX17 (std::is_same<ਸ਼ੀਰਸ਼_2, DfgEq>::ਮੁੱਲ) {
                    replace(make<DfgAnd>(concatp->fileline(), m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, newLhsp, newRhsp));
                } ਨਹੀਂਤਾਂ ਜੇ VL_CONSTEXPR_CXX17 (std::is_same<ਸ਼ੀਰਸ਼_2, DfgNeq>::ਮੁੱਲ) {
                    replace(make<DfgOr>(concatp->fileline(), m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, newLhsp, newRhsp));
                } ਨਹੀਂਤਾਂ {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Bitwise>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ try_ਧੱਕੋ_Bitwise_Op_Through_Reductions(Bitwise* ਅਚਲ vtxp) {
        ਵਰਤੋਂ Reduction = Bitwise_ਮੰਜ਼ਿਲ_Reduction<Bitwise>;

        ਜੇ (Reduction* ਅਚਲ lRedp = vtxp->lhsp()->ਸਾਂਚਾ cast<Reduction>()) {
            ਜੇ (Reduction* ਅਚਲ rRedp = vtxp->rhsp()->ਸਾਂਚਾ cast<Reduction>()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lSrcp = lRedp->srcp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rSrcp = rRedp->srcp();
                ਜੇ (lSrcp->dtype() == rSrcp->dtype() && lSrcp->ਚੌੜਾਈ() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->has_ਗੁਣਜ_Sinks() && !rSrcp->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(ਧੱਕੋ_BITWISE_THROUGH_REDUCTION) {
                        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
                        Bitwise* ਅਚਲ bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, bwp));
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
            }
        }

        ਵਾਪਸ ਝੂਠ;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    ਸਥਿਰ std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਜੇ (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        }
        ਜੇ (vtxp->has_ਗੁਣਜ_Sinks()) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ਅਚਲ bwp = vtxp->as<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ>();
        DfgSel* ਅਚਲ lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ਅਚਲ rSelp = bwp->rhsp()->cast<DfgSel>();
        ਜੇ (!lSelp || !rSelp) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        DfgSel* ਅਚਲ nSelp = lSelp->fromp()->ਚੌੜਾਈ() <= rSelp->fromp()->ਚੌੜਾਈ() ? lSelp : rSelp;
        DfgSel* ਅਚਲ wSelp = nSelp == lSelp ? rSelp : lSelp;
        ਅਚਲ uint32_t n_ਚੌੜਾਈ = nSelp->fromp()->ਚੌੜਾਈ();
        ਜੇ (n_ਚੌੜਾਈ > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        ਜੇ (wSelp->lsb() - nSelp->lsb() + n_ਚੌੜਾਈ > wSelp->fromp()->ਚੌੜਾਈ()) {
            ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        }
        ਵਾਪਸ {nSelp, wSelp};
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Bitwise>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ try_ਧੱਕੋ_Bitwise_Op_Throug_Sel(Bitwise* ਅਚਲ vtxp) {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();

        ਜੇ (DfgSel* ਅਚਲ lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = ਖਾਲੀ_ਸੂਚਕ;
            Dfg_ਸ਼ੀਰਸ਼* extrap = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (DfgSel* ਅਚਲ selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } ਨਹੀਂਤਾਂ ਜੇ (Bitwise* ਅਚਲ bitwisep = rhsp->cast<Bitwise>()) {
                ਜੇ (DfgSel* ਅਚਲ rlSelp = bitwisep->lhsp()->ਸਾਂਚਾ cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } ਨਹੀਂਤਾਂ ਜੇ (DfgSel* ਅਚਲ rrSelp = bitwisep->rhsp()->ਸਾਂਚਾ cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            ਜੇ (rSelp) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lFromp = lSelp->fromp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rFromp = rSelp->fromp();
                ਜੇ (lFromp->dtype() == rFromp->dtype() && lFromp->ਚੌੜਾਈ() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ਧੱਕੋ_BITWISE_THROUGH_SEL) {
                        Bitwise* ਅਚਲ bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        Dfg_ਸ਼ੀਰਸ਼* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        ਜੇ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ਵਾਪਸ ਸੱਚ;
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
            Bitwise* ਅਚਲ rChainp = rhsp->cast<Bitwise>();
            ਜੇ (rChainp && rChainp->has_ਗੁਣਜ_Sinks()) ਵਾਪਸ ਝੂਠ;
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ਅਚਲ ਸਵੈਚਾਲਿਤ lSels = pushableUnalignedSels(lhsp);
            ਅਚਲ ਸਵੈਚਾਲਿਤ rSels = pushableUnalignedSels(rTermp);
            ਜੇ (!lSels.ਪਹਿਲਾ && !rSels.ਪਹਿਲਾ) ਵਾਪਸ ਝੂਠ;

            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ opp = lSels.ਪਹਿਲਾ ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            ਜੇ (opp->is<Bitwise>()) ਵਾਪਸ ਝੂਠ;
            ਅਚਲ ਸਵੈਚਾਲਿਤ& sels = lSels.ਪਹਿਲਾ ? lSels : rSels;
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ nFromp = sels.ਪਹਿਲਾ->fromp();
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ wFromp = sels.ਦੂਜਾ->fromp();
            ਅਚਲ uint32_t ਵਿੱਥ = sels.ਦੂਜਾ->lsb() - sels.ਪਹਿਲਾ->lsb();

            // Both terms must end up as selects from the same pushed operation
            ਅਚਲ ਸਵੈਚਾਲਿਤ matches = [&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* termp, ਅਚਲ ਸਵੈਚਾਲਿਤ& tSels) {
                // Original form: 'n[i] OP w[j]'
                ਜੇ (tSels.ਪਹਿਲਾ) {
                    ਵਾਪਸ termp->type() == opp->type()  //
                           && tSels.ਪਹਿਲਾ->fromp() == nFromp  //
                           && tSels.ਦੂਜਾ->fromp() == wFromp  //
                           && tSels.ਦੂਜਾ->lsb() - tSels.ਪਹਿਲਾ->lsb() == ਵਿੱਥ;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                ਜੇ (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                ਜੇ (ਅਚਲ DfgSel* ਅਚਲ selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                ਜੇ (termp->type() != opp->type()) ਵਾਪਸ ਝੂਠ;
                // Operands might have been swapped since
                ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ਅਚਲ bwp = termp->as<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ>();
                ਅਚਲ ਤਾਰਕਿਕ nIsLhs = bwp->lhsp() == nFromp;
                ਜੇ (!nIsLhs && bwp->rhsp() != nFromp) ਵਾਪਸ ਝੂਠ;
                ਅਚਲ DfgSel* ਅਚਲ slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ਵਾਪਸ slicep && slicep->fromp() == wFromp && slicep->lsb() == ਵਿੱਥ;
            };
            ਜੇ (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ਵਾਪਸ ਝੂਠ;

            APPLYING(ਧੱਕੋ_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = nFromp->dtype();
                DfgSel* ਅਚਲ slicep = make<DfgSel>(flp, dtype, wFromp, ਵਿੱਥ);
                Dfg_ਸ਼ੀਰਸ਼* bwp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } ਨਹੀਂਤਾਂ ਜੇ (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } ਨਹੀਂਤਾਂ {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ਅਚਲ ਸਵੈਚਾਲਿਤ ਧੱਕੋ = [&](Dfg_ਸ਼ੀਰਸ਼* termp, ਅਚਲ DfgSel* nSelp) -> Dfg_ਸ਼ੀਰਸ਼* {
                    ਵਾਪਸ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ newLhsp = ਧੱਕੋ(lhsp, lSels.ਪਹਿਲਾ);
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ newRTermp = ਧੱਕੋ(rTermp, rSels.ਪਹਿਲਾ);
                ਜੇ (rChainp) {
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } ਨਹੀਂਤਾਂ {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ਵਾਪਸ ਸੱਚ;
            }
        }

        ਵਾਪਸ ਝੂਠ;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ਸਥਿਰ ਤਾਰਕਿਕ isConcatFoldable(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) {
        // Concatenation of the same vertex is a replicate
        ਜੇ (isSame(lhsp, rhsp)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (ਅਚਲ DfgRep* ਅਚਲ lRepp = lhsp->cast<DfgRep>()) {
            ਜੇ (isSame(lRepp->srcp(), rhsp)) ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (ਅਚਲ DfgRep* ਅਚਲ rRepp = rhsp->cast<DfgRep>()) {
            ਜੇ (isSame(lhsp, rRepp->srcp())) ਵਾਪਸ ਸੱਚ;
        }
        // Concatenation of adjoining selects is a select
        ਜੇ (ਅਚਲ DfgSel* ਅਚਲ lSelp = lhsp->cast<DfgSel>()) {
            ਜੇ (ਅਚਲ DfgSel* ਅਚਲ rSelp = rhsp->cast<DfgSel>()) {
                ਵਾਪਸ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->ਚੌੜਾਈ();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        ਜੇ (ਅਚਲ DfgNot* ਅਚਲ lNotp = lhsp->cast<DfgNot>()) {
            ਜੇ (ਅਚਲ DfgNot* ਅਚਲ rNotp = rhsp->cast<DfgNot>()) {
                ਵਾਪਸ !lNotp->has_ਗੁਣਜ_Sinks()  //
                       && !rNotp->has_ਗੁਣਜ_Sinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ਸਥਿਰ ਤਾਰਕਿਕ isSameBitwise(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) {
        ਜੇ (lhsp->type() != rhsp->type()) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ਸਥਿਰ ਤਾਰਕਿਕ isConcatSimplifiable(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp) {
        ਜੇ (isConcatFoldable(lhsp, rhsp)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (!isSameBitwise(lhsp, rhsp)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (lhsp->has_ਗੁਣਜ_Sinks() || rhsp->has_ਗੁਣਜ_Sinks()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ਅਚਲ lp = lhsp->as<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ>();
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ਅਚਲ rp = rhsp->as<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ>();
        ਜੇ (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ਵਾਪਸ ਸੱਚ;
        ਜੇ (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ ਝੂਠ;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ਸਥਿਰ ਤਾਰਕਿਕ isConcatPushableThroughBitwise(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp,
                                               ਤਾਰਕਿਕ& ਅਦਲਾ_ਬਦਲੀ) {
        ਜੇ (!isSameBitwise(lhsp, rhsp)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (lhsp->has_ਗੁਣਜ_Sinks() || rhsp->has_ਗੁਣਜ_Sinks()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ਅਚਲ lp = lhsp->as<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ>();
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ਅਚਲ rp = rhsp->as<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ>();
        ਅਦਲਾ_ਬਦਲੀ = ਝੂਠ;
        ਜੇ (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ਵਾਪਸ ਸੱਚ;
        ਜੇ (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ਵਾਪਸ ਸੱਚ;
        ਅਦਲਾ_ਬਦਲੀ = ਸੱਚ;
        ਜੇ (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ਵਾਪਸ ਸੱਚ;
        ਜੇ (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ ਝੂਠ;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    Dfg_ਸ਼ੀਰਸ਼* makeConcatPushedThroughBitwise(File_ਸਤਰ* flp, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* lhsp,
                                              ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* rhsp, ਤਾਰਕਿਕ ਅਦਲਾ_ਬਦਲੀ) {
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ਅਚਲ lp = lhsp->as<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ>();
        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ਅਚਲ rp = rhsp->as<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ>();
        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = Dfg_ਅੰਕੜੇ_Type::packed(lp->ਚੌੜਾਈ() + rp->ਚੌੜਾਈ());
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ cp = ਅਦਲਾ_ਬਦਲੀ ? rp->rhsp() : rp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ dp = ਅਦਲਾ_ਬਦਲੀ ? rp->lhsp() : rp->rhsp();
        DfgConcat* ਅਚਲ newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ਅਚਲ newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        ਜੇ (lp->is<DfgAnd>()) ਵਾਪਸ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        ਜੇ (lp->is<DfgOr>()) ਵਾਪਸ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ਵਾਪਸ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Bitwise>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->ਚੌੜਾਈ() == 1, vtxp, "Width must be 1");
        ਵਰਤੋਂ Reduction = Bitwise_ਮੰਜ਼ਿਲ_Reduction<Bitwise>;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();

        ਜੇ (DfgSel* ਅਚਲ lSelp = lhsp->ਸਾਂਚਾ cast<DfgSel>()) {
            DfgSel* rSelp = ਖਾਲੀ_ਸੂਚਕ;
            Dfg_ਸ਼ੀਰਸ਼* extrap = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (DfgSel* ਅਚਲ selp = rhsp->ਸਾਂਚਾ cast<DfgSel>()) {
                rSelp = selp;
            } ਨਹੀਂਤਾਂ ਜੇ (Bitwise* ਅਚਲ rBitwisep = rhsp->ਸਾਂਚਾ cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->ਸਾਂਚਾ cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } ਨਹੀਂਤਾਂ ਜੇ (Reduction* ਅਚਲ rRedp = rhsp->ਸਾਂਚਾ cast<Reduction>()) {
                rSelp = rRedp->srcp()->ਸਾਂਚਾ cast<DfgSel>();
            }
            ਜੇ (rSelp) {
                uint32_t lsb = 0;
                ਜੇ (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_ਦਾ_SELS_WITH_REDUCTION) {
                        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype
                            = Dfg_ਅੰਕੜੇ_Type::packed(lSelp->ਚੌੜਾਈ() + rSelp->ਚੌੜਾਈ());
                        DfgSel* ਅਚਲ newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        Dfg_ਸ਼ੀਰਸ਼* resp = make<Reduction>(vtxp, newSelp);
                        ਜੇ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ਵਾਪਸ ਸੱਚ;
                    }
                }
            }
        }

        ਜੇ (Reduction* ਅਚਲ lRedp = lhsp->ਸਾਂਚਾ cast<Reduction>()) {
            Reduction* rRedp = rhsp->ਸਾਂਚਾ cast<Reduction>();
            Dfg_ਸ਼ੀਰਸ਼* extrap = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!rRedp) {
                ਜੇ (Bitwise* ਅਚਲ rBitwisep = rhsp->ਸਾਂਚਾ cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->ਸਾਂਚਾ cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            ਜੇ (rRedp) {
                ਜੇ (DfgSel* ਅਚਲ lSelp = lRedp->srcp()->ਸਾਂਚਾ cast<DfgSel>()) {
                    ਜੇ (DfgSel* ਅਚਲ rSelp = rRedp->srcp()->ਸਾਂਚਾ cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ਜੇ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_ਦਾ_REDUCTION_ਦਾ_SELS_WITH_REDUCTION) {
                                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype
                                    = Dfg_ਅੰਕੜੇ_Type::packed(lSelp->ਚੌੜਾਈ() + rSelp->ਚੌੜਾਈ());
                                DfgSel* ਅਚਲ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                Dfg_ਸ਼ੀਰਸ਼* resp = make<Reduction>(vtxp, newSelp);
                                ਜੇ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ਵਾਪਸ ਸੱਚ;
                            }
                        }
                    }
                }
            }
        }

        ਵਾਪਸ ਝੂਠ;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Reduction>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ optimizeReduction(Reduction* ਅਚਲ vtxp) {
        ਵਰਤੋਂ Bitwise = Reduction_ਮੰਜ਼ਿਲ_Bitwise<Reduction>;

        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ ਸੱਚ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        // Reduction of 1-bit value
        ਜੇ (srcp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {
            APPLYING(REMOVE_ਚੌੜਾਈ_ONE_REDUCTION) {
                replace(srcp);
                ਵਾਪਸ ਸੱਚ;
            }
        }

        ਜੇ (DfgCond* ਅਚਲ condp = srcp->cast<DfgCond>()) {
            ਜੇ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ਧੱਕੋ_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ਅਚਲ newThenp = make<Reduction>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ਅਚਲ newElsep = make<Reduction>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ਅਚਲ newCondp = make<DfgCond>(condp->fileline(), m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ਵਾਪਸ ਸੱਚ;
                }
            }
        }

        ਜੇ (DfgConcat* ਅਚਲ concatp = srcp->cast<DfgConcat>()) {
            ਜੇ (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType
                || concatp->rhsp()->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {
                APPLYING(ਧੱਕੋ_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ਅਚਲ lRedp
                        = make<Reduction>(concatp->fileline(), m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, concatp->lhsp());
                    Reduction* ਅਚਲ rRedp
                        = make<Reduction>(concatp->fileline(), m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, lRedp, rRedp));
                    ਵਾਪਸ ਸੱਚ;
                }
            }
        }

        ਜੇ (Bitwise* ਅਚਲ bitwisep = vtxp->srcp()->ਸਾਂਚਾ cast<Bitwise>()) {
            ਜੇ (!bitwisep->has_ਗੁਣਜ_Sinks()) {
                ਜੇ (bitwisep->lhsp()->ਸਾਂਚਾ is<DfgConcat>()
                    || bitwisep->rhsp()->ਸਾਂਚਾ is<DfgConcat>()) {
                    APPLYING(ਧੱਕੋ_REDUCTION_THROUGH_BITWISE_ਦਾ_CONCAT) {
                        Reduction* ਅਚਲ newLhsp
                            = make<Reduction>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, bitwisep->lhsp());
                        Reduction* ਅਚਲ newRhsp
                            = make<Reduction>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, newLhsp, newRhsp));
                        ਵਾਪਸ ਸੱਚ;
                    }
                }

                ਜੇ (DfgSel* ਅਚਲ lSelp = bitwisep->lhsp()->ਸਾਂਚਾ cast<DfgSel>()) {
                    ਜੇ (DfgSel* ਅਚਲ rSelp = bitwisep->rhsp()->ਸਾਂਚਾ cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ਜੇ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ਧੱਕੋ_REDUCTION_THROUGH_BITWISE_ਦਾ_SELS) {
                                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype
                                    = Dfg_ਅੰਕੜੇ_Type::packed(lSelp->ਚੌੜਾਈ() + rSelp->ਚੌੜਾਈ());
                                DfgSel* ਅਚਲ newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ਵਾਪਸ ਸੱਚ;
                            }
                        }
                    }
                }
            }
        }

        ਵਾਪਸ ਝੂਠ;
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ ਖਿਸਕਾਓ>
    VL_ATTR_WARN_ਅਣਵਰਤਿਆ_ਨਤੀਜਾ ਤਾਰਕਿਕ optimize_ਖਿਸਕਾਓ_RHS(ਖਿਸਕਾਓ* ਅਚਲ vtxp) {
        static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ, ਖਿਸਕਾਓ>::ਮੁੱਲ, "Must invoke on binary");
        static_assert(std::is_final<ਖਿਸਕਾਓ>::ਮੁੱਲ, "Must invoke on final class");
        ਜੇ (ਅਚਲ DfgConcat* ਅਚਲ concatp = vtxp->rhsp()->ਸਾਂਚਾ cast<DfgConcat>()) {
            ਜੇ (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_ਦਾ_ਖਿਸਕਾਓ) {
                    replace(make<ਖਿਸਕਾਓ>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ਵਾਪਸ ਸੱਚ;
                }
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<Dfg_ਸ਼ੀਰਸ਼*, uint32_t, uint32_t> ਜੋੜੋ_ਮੰਜ਼ਿਲ_ਗਿਣਤੀ_Ones_ਪਦ(Dfg_ਸ਼ੀਰਸ਼* vtxp) {
        ਜੇ (DfgConcat* ਅਚਲ oCatp = vtxp->cast<DfgConcat>()) {
            ਜੇ (isZero(oCatp->lhsp())) {
                ਜੇ (Dfg_ਗਿਣਤੀ_Ones* ਅਚਲ ਗਿਣਤੀ_Onesp = oCatp->rhsp()->cast<Dfg_ਗਿਣਤੀ_Ones>()) {
                    // Zero extended count ones
                    ਜੇ (DfgSel* ਅਚਲ selp = ਗਿਣਤੀ_Onesp->srcp()->cast<DfgSel>()) {
                        ਵਾਪਸ {selp->fromp(), selp->lsb(), selp->ਚੌੜਾਈ()};
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (DfgSel* ਅਚਲ selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    ਜੇ (selp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {  //
                        ਵਾਪਸ {selp->fromp(), selp->lsb(), selp->ਚੌੜਾਈ()};
                    }
                }
            }
            ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0, 0};
        }
        ਜੇ (Dfg_ਗਿਣਤੀ_Ones* ਅਚਲ ਗਿਣਤੀ_Onesp = vtxp->cast<Dfg_ਗਿਣਤੀ_Ones>()) {
            // Simple count ones
            ਜੇ (DfgSel* ਅਚਲ selp = ਗਿਣਤੀ_Onesp->srcp()->cast<DfgSel>()) {
                ਵਾਪਸ {selp->fromp(), selp->lsb(), selp->ਚੌੜਾਈ()};
            }
            ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0, 0};
        }
        ਜੇ (DfgSel* ਅਚਲ oSelp = vtxp->cast<DfgSel>()) {
            ਜੇ (oSelp->lsb() == 0) {
                // Truncated count ones
                ਜੇ (Dfg_ਗਿਣਤੀ_Ones* ਅਚਲ ਗਿਣਤੀ_Onesp = oSelp->fromp()->cast<Dfg_ਗਿਣਤੀ_Ones>()) {
                    // Zero extended count ones
                    ਜੇ (DfgSel* ਅਚਲ selp = ਗਿਣਤੀ_Onesp->srcp()->cast<DfgSel>()) {
                        ਵਾਪਸ {selp->fromp(), selp->lsb(), selp->ਚੌੜਾਈ()};
                    }
                }
            }
            // Single bit select
            ਜੇ (oSelp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {  //
                ਵਾਪਸ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0, 0};
        }
        // Altered form of extended MSB
        ਜੇ (Dfg_ਖਿਸਕਾਓ_R* ਅਚਲ shiftrp = vtxp->cast<Dfg_ਖਿਸਕਾਓ_R>()) {
            ਜੇ (DfgConst* ਅਚਲ rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                ਜੇ (rConstp->ਮੰਜ਼ਿਲ_U32() == shiftrp->ਚੌੜਾਈ() - 1) {
                    ਵਾਪਸ {shiftrp->lhsp(), shiftrp->ਚੌੜਾਈ() - 1, 1};
                }
            }
            ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0, 0};
        }
        // Not applicable
        ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> unwindSel(Dfg_ਸ਼ੀਰਸ਼* fromp, uint32_t lsb,
                                              ਅਚਲ uint32_t ਚੌੜਾਈ) {
        ਜਦੋਂ (ਸੱਚ) {
            ਅਚਲ uint32_t msb = lsb + ਚੌੜਾਈ - 1;

            // Sel from Concat
            ਜੇ (DfgConcat* ਅਚਲ concatp = fromp->cast<DfgConcat>()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = concatp->lhsp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = concatp->rhsp();

                ਜੇ (msb < rhsp->ਚੌੜਾਈ()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_ਦਾ_CONCAT) {
                        fromp = rhsp;
                        ਜਾਰੀ;
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (lsb >= rhsp->ਚੌੜਾਈ()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_ਦਾ_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->ਚੌੜਾਈ();
                        ਜਾਰੀ;
                    }
                }
            }

            ਜੇ (DfgRep* ਅਚਲ repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ਅਚਲ uint32_t src_ਚੌੜਾਈ = repp->srcp()->ਚੌੜਾਈ();
                ਜੇ (ਚੌੜਾਈ <= src_ਚੌੜਾਈ) {
                    ਅਚਲ uint32_t newLsb = lsb % src_ਚੌੜਾਈ;
                    ਅਚਲ uint32_t newMsb = newLsb + ਚੌੜਾਈ - 1;
                    ਜੇ (newMsb < src_ਚੌੜਾਈ) {
                        APPLYING(ਧੱਕੋ_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            ਜਾਰੀ;
                        }
                    }
                }
            }

            // Sel from Sel
            ਜੇ (DfgSel* ਅਚਲ selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    ਜਾਰੀ;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            ਜੇ (DfgVarPacked* ਅਚਲ varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ਅਚਲ ਸਵੈਚਾਲਿਤ pair = varp->driver_ਦਾ_Range(lsb, ਚੌੜਾਈ);
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ driverp = pair.ਪਹਿਲਾ;
                ਅਚਲ uint32_t driverLsb = pair.ਦੂਜਾ;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                ਜੇ (driverp) {
                    ਜੇ (driverp == varp->defaultp()) {
                        APPLYING(ਧੱਕੋ_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ਜਾਰੀ;
                        }
                    } ਨਹੀਂਤਾਂ {
                        APPLYING(ਧੱਕੋ_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            ਜਾਰੀ;
                        }
                    }
                }
            }

            // No patterns matched, stop
            ਰੋਕੋ;
        }
        ਵਾਪਸ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> commonLSBs(Dfg_ਸ਼ੀਰਸ਼* ap, Dfg_ਸ਼ੀਰਸ਼* bp) {
        ਜੇ (ap == bp) ਵਾਪਸ {ap, ap->ਚੌੜਾਈ()};

        // If both constants, check LSBs
        ਜੇ (DfgConst* ਅਚਲ aConstp = ap->cast<DfgConst>()) {
            ਜੇ (DfgConst* ਅਚਲ bConstp = bp->cast<DfgConst>()) {
                ਅਚਲ V3_ਸੰਖਿਆ& a_ਅੰਕ = aConstp->ਅੰਕ();
                ਅਚਲ V3_ਸੰਖਿਆ& b_ਅੰਕ = bConstp->ਅੰਕ();
                // Max match is the shorter constant
                ਅਚਲ uint32_t max_ਮੇਲ = std::min(aConstp->ਚੌੜਾਈ(), bConstp->ਚੌੜਾਈ());
                // Check all bits
                uint32_t ਮੇਲ_ਚੌੜਾਈ = 0;
                ਲਈ (; ਮੇਲ_ਚੌੜਾਈ < max_ਮੇਲ; ++ਮੇਲ_ਚੌੜਾਈ) {
                    ਜੇ (a_ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਮੇਲ_ਚੌੜਾਈ) != b_ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਮੇਲ_ਚੌੜਾਈ)) ਰੋਕੋ;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ਅਚਲ shorterp = aConstp->ਚੌੜਾਈ() < bConstp->ਚੌੜਾਈ() ? aConstp : bConstp;
                ਵਾਪਸ {ਮੇਲ_ਚੌੜਾਈ ? shorterp : ਖਾਲੀ_ਸੂਚਕ, ਮੇਲ_ਚੌੜਾਈ};
            }
        }

        // If Concat, check against the RHS
        ਜੇ (DfgConcat* ਅਚਲ catp = ap->cast<DfgConcat>()) ਵਾਪਸ commonLSBs(catp->rhsp(), bp);
        ਜੇ (DfgConcat* ਅਚਲ catp = bp->cast<DfgConcat>()) ਵਾਪਸ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        ਜੇ (DfgSel* ਅਚਲ selp = ap->cast<DfgSel>()) {
            ਜੇ (selp->lsb() == 0) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = selp->fromp();
                ਅਚਲ std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> common = commonLSBs(fromp, bp);
                ਵਾਪਸ {common.ਪਹਿਲਾ, std::min(common.ਦੂਜਾ, selp->ਚੌੜਾਈ())};
            }
        }
        ਜੇ (DfgSel* ਅਚਲ selp = bp->cast<DfgSel>()) {
            ਜੇ (selp->lsb() == 0) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = selp->fromp();
                ਅਚਲ std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> common = commonLSBs(ap, fromp);
                ਵਾਪਸ {common.ਪਹਿਲਾ, std::min(common.ਦੂਜਾ, selp->ਚੌੜਾਈ())};
            }
        }

        // Otherwise no common LSBs
        ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> commonMSBs(Dfg_ਸ਼ੀਰਸ਼* ap, Dfg_ਸ਼ੀਰਸ਼* bp) {
        ਜੇ (ap == bp) ਵਾਪਸ {ap, ap->ਚੌੜਾਈ()};

        // If both constants, check MSBs
        ਜੇ (DfgConst* ਅਚਲ aConstp = ap->cast<DfgConst>()) {
            ਜੇ (DfgConst* ਅਚਲ bConstp = bp->cast<DfgConst>()) {
                ਅਚਲ uint32_t aMsb = aConstp->ਚੌੜਾਈ() - 1;
                ਅਚਲ uint32_t bMsb = bConstp->ਚੌੜਾਈ() - 1;
                ਅਚਲ V3_ਸੰਖਿਆ& a_ਅੰਕ = aConstp->ਅੰਕ();
                ਅਚਲ V3_ਸੰਖਿਆ& b_ਅੰਕ = bConstp->ਅੰਕ();
                // Max match is the shorter constant
                ਅਚਲ uint32_t max_ਮੇਲ = std::min(aConstp->ਚੌੜਾਈ(), bConstp->ਚੌੜਾਈ());
                // Check all bits
                uint32_t ਮੇਲ_ਚੌੜਾਈ = 0;
                ਲਈ (; ਮੇਲ_ਚੌੜਾਈ < max_ਮੇਲ; ++ਮੇਲ_ਚੌੜਾਈ) {
                    ਜੇ (a_ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(aMsb - ਮੇਲ_ਚੌੜਾਈ) != b_ਅੰਕ.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(bMsb - ਮੇਲ_ਚੌੜਾਈ)) ਰੋਕੋ;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ਅਚਲ shorterp = aMsb < bMsb ? aConstp : bConstp;
                ਵਾਪਸ {ਮੇਲ_ਚੌੜਾਈ ? shorterp : ਖਾਲੀ_ਸੂਚਕ, ਮੇਲ_ਚੌੜਾਈ};
            }
        }

        // If Concat, check against the LHS
        ਜੇ (DfgConcat* ਅਚਲ catp = ap->cast<DfgConcat>()) ਵਾਪਸ commonMSBs(catp->lhsp(), bp);
        ਜੇ (DfgConcat* ਅਚਲ catp = bp->cast<DfgConcat>()) ਵਾਪਸ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        ਜੇ (DfgSel* ਅਚਲ selp = ap->cast<DfgSel>()) {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = selp->fromp();
            ਜੇ (selp->msb() == fromp->ਚੌੜਾਈ() - 1) {
                ਅਚਲ std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> common = commonMSBs(fromp, bp);
                ਵਾਪਸ {common.ਪਹਿਲਾ, std::min(common.ਦੂਜਾ, selp->ਚੌੜਾਈ())};
            }
        }
        ਜੇ (DfgSel* ਅਚਲ selp = bp->cast<DfgSel>()) {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = selp->fromp();
            ਜੇ (selp->msb() == fromp->ਚੌੜਾਈ() - 1) {
                ਅਚਲ std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> common = commonMSBs(ap, fromp);
                ਵਾਪਸ {common.ਪਹਿਲਾ, std::min(common.ਦੂਜਾ, selp->ਚੌੜਾਈ())};
            }
        }

        // Otherwise no common MSBs
        ਵਾਪਸ {ਖਾਲੀ_ਸੂਚਕ, 0};
    }

    // VISIT methods

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਸ਼ੀਰਸ਼*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਗਿਣਤੀ_Ones* ਅਚਲ vtxp) override {
        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgExtend* ਅਚਲ vtxp) override {
        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ zerop
                = makeZero(vtxp->fileline(), vtxp->ਚੌੜਾਈ() - vtxp->srcp()->ਚੌੜਾਈ());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ਵਾਪਸ;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgExtendS* ਅਚਲ vtxp) override {
        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLogNot* ਅਚਲ vtxp) override {
        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgNegate* ਅਚਲ vtxp) override {
        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgNot* ਅਚਲ vtxp) override {
        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ;

        // Not of Cond
        ਜੇ (DfgCond* ਅਚਲ condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            ਜੇ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ਧੱਕੋ_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ਅਚਲ newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ਅਚਲ newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ਅਚਲ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ਵਾਪਸ;
                }
            }
        }

        // Not of Not
        ਜੇ (DfgNot* ਅਚਲ notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ਵਾਪਸ;
            }
        }

        ਜੇ (!vtxp->srcp()->has_ਗੁਣਜ_Sinks()) {
            // Not of Eq
            ਜੇ (DfgEq* ਅਚਲ eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ਵਾਪਸ;
                }
            }

            // Not of Neq
            ਜੇ (DfgNeq* ਅਚਲ neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ਵਾਪਸ;
                }
            }

            // Not of unsigned comparisons
            ਜੇ (DfgLt* ਅਚਲ p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (DfgGte* ਅਚਲ p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (DfgGt* ਅਚਲ p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (DfgLte* ਅਚਲ p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ਵਾਪਸ;
                }
            }

            // Not of signed comparisons
            ਜੇ (DfgLtS* ਅਚਲ p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (DfgGteS* ਅਚਲ p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (DfgGtS* ਅਚਲ p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (DfgLteS* ਅਚਲ p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgOneHot* ਅਚਲ vtxp) override {
        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgOneHot0* ਅਚਲ vtxp) override {
        ਜੇ (foldUnary(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgRedOr* ਅਚਲ vtxp) override {
        ਜੇ (optimizeReduction(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgRedAnd* ਅਚਲ vtxp) override {
        ਜੇ (optimizeReduction(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgRedXor* ਅਚਲ vtxp) override {
        ਜੇ (optimizeReduction(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgSel* ਅਚਲ vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = vtxp->fromp();

        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਅਚਲ uint32_t lsb = vtxp->lsb();
        ਅਚਲ uint32_t ਚੌੜਾਈ = vtxp->ਚੌੜਾਈ();
        ਅਚਲ uint32_t msb = lsb + ਚੌੜਾਈ - 1;

        ਜੇ (DfgConst* ਅਚਲ constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ਅਚਲ resp = makeZero(flp, ਚੌੜਾਈ);
                resp->ਅੰਕ().opSel(constp->ਅੰਕ(), msb, lsb);
                replace(resp);
                ਵਾਪਸ;
            }
        }

        // Full width select, replace with the source.
        ਜੇ (fromp->ਚੌੜਾਈ() == ਚੌੜਾਈ) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_ਭਰਿਆ_ਚੌੜਾਈ_SEL) {
                replace(fromp);
                ਵਾਪਸ;
            }
        }

        // Unwind through bit packing in one go
        {
            ਅਚਲ ਸਵੈਚਾਲਿਤ res = unwindSel(fromp, lsb, ਚੌੜਾਈ);
            ਜੇ (res.ਪਹਿਲਾ != fromp || res.ਦੂਜਾ != lsb) {
                replace(make<DfgSel>(vtxp, res.ਪਹਿਲਾ, res.ਦੂਜਾ));
                ਵਾਪਸ;
            }
        }

        // Sel from Not
        ਜੇ (DfgNot* ਅਚਲ notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            ਜੇ (!notp->has_ਗੁਣਜ_Sinks()) {
                APPLYING(ਧੱਕੋ_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ਅਚਲ newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ਵਾਪਸ;
                }
            }
        }

        // Sel from Cond
        ਜੇ (DfgCond* ਅਚਲ condp = fromp->cast<DfgCond>()) {
            ਜੇ (!condp->has_ਗੁਣਜ_Sinks()) {
                APPLYING(ਧੱਕੋ_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ਅਚਲ newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ਅਚਲ newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ਅਚਲ newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ਵਾਪਸ;
                }
            }
        }

        // Sel from ShiftL
        ਜੇ (Dfg_ਖਿਸਕਾਓ_L* ਅਚਲ ਖਿਸਕਾਓ_Lp = fromp->cast<Dfg_ਖਿਸਕਾਓ_L>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            ਜੇ (lsb == 0) {
                UASSERT_OBJ(ਖਿਸਕਾਓ_Lp->lhsp()->ਚੌੜਾਈ() >= ਚੌੜਾਈ, vtxp, "input of shift narrow");
                APPLYING(ਧੱਕੋ_SEL_THROUGH_SHIFTL) {
                    DfgSel* ਅਚਲ newSelp = make<DfgSel>(vtxp, ਖਿਸਕਾਓ_Lp->lhsp(), vtxp->lsb());
                    replace(make<Dfg_ਖਿਸਕਾਓ_L>(vtxp, newSelp, ਖਿਸਕਾਓ_Lp->rhsp()));
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgMux* ਅਚਲ vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ fromp = vtxp->fromp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lsbp = vtxp->lsbp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (DfgConst* ਅਚਲ lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->ਅੰਕ().ਮੰਜ਼ਿਲ_UInt()));
                ਵਾਪਸ;
            }
        }

        ਜੇ (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->ਚੌੜਾਈ()));
                ਵਾਪਸ;
            }
        }

        ਜੇ (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->ਚੌੜਾਈ()));
                ਵਾਪਸ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    ਖਾਲੀ ਦੇਖੋ(DfgAnd* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ਜੇ (!lhsp->has_ਗੁਣਜ_Sinks() || !rhsp->has_ਗੁਣਜ_Sinks()) {
            ਜੇ (DfgNot* ਅਚਲ lhsNotp = lhsp->cast<DfgNot>()) {
                ਜੇ (DfgNot* ਅਚਲ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_ਦਾ_NOT_AND_NOT) {
                        DfgOr* ਅਚਲ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ਵਾਪਸ;
                    }
                }
                ਜੇ (DfgNeq* ਅਚਲ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_ਦਾ_NOT_AND_NEQ) {
                        DfgEq* ਅਚਲ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ਅਚਲ orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (DfgConst* ਅਚਲ lConstp = lhsp->cast<DfgConst>()) {
            ਜੇ (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ਵਾਪਸ;
                }
            }

            ਜੇ (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ਵਾਪਸ;
                }
            }

            ਜੇ (DfgConcat* ਅਚਲ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ਵਾਪਸ;
            }
        }

        ਜੇ (distributive_And_Associative_ਦੁਆਧਾਰੀ<DfgOr, DfgAnd>(vtxp)) ਵਾਪਸ;

        ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Through_Reductions(vtxp)) ਵਾਪਸ;

        ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Throug_Sel(vtxp)) ਵਾਪਸ;

        {
            DfgNot* ਅਚਲ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ਅਚਲ rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            ਜੇ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->ਚੌੜਾਈ()));
                    ਵਾਪਸ;
                }
            }

            ਜੇ (DfgAnd* ਅਚਲ rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ਅਚਲ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ਅਚਲ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                ਜੇ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->ਚੌੜਾਈ()));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (vtxp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {
            ਜੇ (tryReplaceBitwiseWithReduction(vtxp)) ਵਾਪਸ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            ਜੇ (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            ਜੇ (repp && condp && repp->srcp()->ਮਾਪ() == 1 && !condp->has_ਗੁਣਜ_Sinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ਅਚਲ newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgOr* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ਜੇ (!lhsp->has_ਗੁਣਜ_Sinks() || !rhsp->has_ਗੁਣਜ_Sinks()) {
            ਜੇ (DfgNot* ਅਚਲ lhsNotp = lhsp->cast<DfgNot>()) {
                ਜੇ (DfgNot* ਅਚਲ rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_ਦਾ_NOT_AND_NOT) {
                        DfgAnd* ਅਚਲ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ਵਾਪਸ;
                    }
                }
                ਜੇ (DfgNeq* ਅਚਲ rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_ਦਾ_NOT_AND_NEQ) {
                        DfgEq* ਅਚਲ newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ਅਚਲ andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (DfgConcat* ਅਚਲ lhsConcatp = lhsp->cast<DfgConcat>()) {
            ਜੇ (DfgConcat* ਅਚਲ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ਜੇ (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    ਜੇ (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_ਦਾ_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ਵਾਪਸ;
                        }
                    }
                    ਜੇ (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_ਦਾ_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ਵਾਪਸ;
                        }
                    }
                }
            }
        }

        ਜੇ (DfgConst* ਅਚਲ lConstp = lhsp->cast<DfgConst>()) {
            ਜੇ (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ਵਾਪਸ;
                }
            }

            ਜੇ (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ਵਾਪਸ;
                }
            }

            ਜੇ (DfgConcat* ਅਚਲ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) ਵਾਪਸ;
            }
        }

        ਜੇ (distributive_And_Associative_ਦੁਆਧਾਰੀ<DfgAnd, DfgOr>(vtxp)) ਵਾਪਸ;

        ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Through_Reductions(vtxp)) ਵਾਪਸ;

        ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Throug_Sel(vtxp)) ਵਾਪਸ;

        {
            DfgNot* ਅਚਲ lNotp = lhsp->cast<DfgNot>();
            DfgNot* ਅਚਲ rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            ਜੇ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->ਚੌੜਾਈ()));
                    ਵਾਪਸ;
                }
            }

            ਜੇ (DfgOr* ਅਚਲ rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ਅਚਲ rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ਅਚਲ rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                ਜੇ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->ਚੌੜਾਈ()));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (vtxp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {
            ਜੇ (tryReplaceBitwiseWithReduction(vtxp)) ਵਾਪਸ;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgXor* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->ਚੌੜਾਈ()));
                ਵਾਪਸ;
            }
        }

        ਜੇ (DfgConst* ਅਚਲ lConstp = lhsp->cast<DfgConst>()) {
            ਜੇ (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ਵਾਪਸ;
                }
            }
            ਜੇ (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (DfgConcat* ਅਚਲ rConcatp = rhsp->cast<DfgConcat>()) {
                ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) ਵਾਪਸ;
            }
        }

        ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Through_Reductions(vtxp)) ਵਾਪਸ;

        ਜੇ (try_ਧੱਕੋ_Bitwise_Op_Throug_Sel(vtxp)) ਵਾਪਸ;

        ਜੇ (vtxp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {
            ਜੇ (tryReplaceBitwiseWithReduction(vtxp)) ਵਾਪਸ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਜੋੜੋ* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isZero(lhsp)) {
            APPLYING(REMOVE_ਜੋੜੋ_ZERO) {
                replace(rhsp);
                ਵਾਪਸ;
            }
        }

        ਅਚਲ std::tuple<Dfg_ਸ਼ੀਰਸ਼*, uint32_t, uint32_t> l_ਪਦ = ਜੋੜੋ_ਮੰਜ਼ਿਲ_ਗਿਣਤੀ_Ones_ਪਦ(lhsp);
        ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp = std::get<0>(l_ਪਦ)) {
            std::tuple<Dfg_ਸ਼ੀਰਸ਼*, uint32_t, uint32_t> r_ਪਦ = ਜੋੜੋ_ਮੰਜ਼ਿਲ_ਗਿਣਤੀ_Ones_ਪਦ(rhsp);
            Dfg_ਸ਼ੀਰਸ਼* extrap = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!std::get<0>(r_ਪਦ)) {
                ਜੇ (Dfg_ਜੋੜੋ* ਅਚਲ rAddp = rhsp->cast<Dfg_ਜੋੜੋ>()) {
                    r_ਪਦ = ਜੋੜੋ_ਮੰਜ਼ਿਲ_ਗਿਣਤੀ_Ones_ਪਦ(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rVtxp = std::get<0>(r_ਪਦ)) {
                ਜੇ (isSame(lVtxp, rVtxp)) {
                    ਅਚਲ uint32_t lLsb = std::get<1>(l_ਪਦ);
                    ਅਚਲ uint32_t rLsb = std::get<1>(r_ਪਦ);
                    ਅਚਲ uint32_t l_ਚੌੜਾਈ = std::get<2>(l_ਪਦ);
                    ਅਚਲ uint32_t r_ਚੌੜਾਈ = std::get<2>(r_ਪਦ);
                    ਤਾਰਕਿਕ adjoined = ਸੱਚ;
                    uint32_t lsb = 0;
                    ਜੇ (lLsb + l_ਚੌੜਾਈ == rLsb) {
                        lsb = lLsb;
                    } ਨਹੀਂਤਾਂ ਜੇ (lLsb == rLsb + r_ਚੌੜਾਈ) {
                        lsb = rLsb;
                    } ਨਹੀਂਤਾਂ {
                        adjoined = ਝੂਠ;
                    }
                    ਜੇ (adjoined) {
                        APPLYING(REPLACE_ਜੋੜੋ_WITH_ਗਿਣਤੀ_ONES) {
                            DfgSel* ਅਚਲ selp
                                = make<DfgSel>(vtxp->fileline(),
                                               Dfg_ਅੰਕੜੇ_Type::packed(l_ਚੌੜਾਈ + r_ਚੌੜਾਈ), lVtxp, lsb);
                            Dfg_ਸ਼ੀਰਸ਼* resp
                                = make<Dfg_ਗਿਣਤੀ_Ones>(flp, Dfg_ਅੰਕੜੇ_Type::packed(32), selp);
                            ਜੇ (vtxp->ਚੌੜਾਈ() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->ਚੌੜਾਈ() - 32U),
                                                       resp);
                            } ਨਹੀਂਤਾਂ ਜੇ (vtxp->ਚੌੜਾਈ() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            ਜੇ (extrap) resp = make<Dfg_ਜੋੜੋ>(vtxp, resp, extrap);
                            replace(resp);
                            ਵਾਪਸ;
                        }
                    }
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgArraySel* ਅਚਲ vtxp) override {
        DfgConst* ਅਚਲ idxp = vtxp->bitp()->cast<DfgConst>();
        ਜੇ (!idxp) ਵਾਪਸ;
        DfgVarArray* ਅਚਲ varp = vtxp->fromp()->cast<DfgVarArray>();
        ਜੇ (!varp) ਵਾਪਸ;
        AstVar* ਅਚਲ astVarp = varp->vscp()->varp();
        ਜੇ (astVarp->isForced()) ਵਾਪਸ;
        ਜੇ (astVarp->isSigUserRWPublic()) ਵਾਪਸ;
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = varp->srcp();
        ਜੇ (!srcp) {
            ਜੇ (vtxp->isPacked()) {
                ਜੇ (AstConsArray* ਅਚਲ iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    ਜੇ (AstConst* ਅਚਲ valp = VN_CAST(iap->valuep(idxp->ਮੰਜ਼ਿਲ_ਮਾਪ_T()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_ਸਾਰਣੀ) {
                            replace(ਨਵਾਂ DfgConst{m_dfg, valp->fileline(), valp->ਅੰਕ()});
                            ਵਾਪਸ;
                        }
                    }
                }
            }
            ਵਾਪਸ;
        }

        ਜੇ (DfgSpliceArray* ਅਚਲ splicep = srcp->cast<DfgSpliceArray>()) {
            Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ driverp = splicep->driverAt(idxp->ਮੰਜ਼ਿਲ_ਮਾਪ_T());
            ਜੇ (!driverp) ਵਾਪਸ;
            DfgUnitArray* ਅਚਲ uap = driverp->cast<DfgUnitArray>();
            ਜੇ (!uap) ਵਾਪਸ;
            ਜੇ (uap->srcp()->is<Dfg_ਸ਼ੀਰਸ਼_Splice>()) ਵਾਪਸ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ dvarp = uap->srcp()->cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                ਜੇ (dvarp->srcp()) ਵਾਪਸ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ਵਾਪਸ;
            }
        }

        ਜੇ (DfgUnitArray* ਅਚਲ uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->ਮੰਜ਼ਿਲ_ਮਾਪ_T() == 0, vtxp, "Array index out of range");
            ਜੇ (uap->srcp()->is<DfgSplicePacked>()) ਵਾਪਸ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ dvarp = uap->srcp()->cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                ਜੇ (dvarp->srcp()) ਵਾਪਸ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgConcat* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();

        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isZero(lhsp)) {
            DfgConst* ਅਚਲ lConstp = lhsp->as<DfgConst>();
            ਜੇ (DfgSel* ਅਚਲ rSelp = rhsp->cast<DfgSel>()) {
                ਜੇ (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->ਚੌੜਾਈ()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_ਸਿਖਰ_WITH_SHIFTR) {
                        replace(
                            make<Dfg_ਖਿਸਕਾਓ_R>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->ਚੌੜਾਈ())));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (isZero(rhsp)) {
            DfgConst* ਅਚਲ rConstp = rhsp->as<DfgConst>();
            ਜੇ (DfgSel* ਅਚਲ lSelp = lhsp->cast<DfgSel>()) {
                ਜੇ (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<Dfg_ਖਿਸਕਾਓ_L>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->ਚੌੜਾਈ())));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (DfgNot* ਅਚਲ lNot = lhsp->cast<DfgNot>()) {
            ਜੇ (DfgNot* ਅਚਲ rNot = rhsp->cast<DfgNot>()) {
                ਜੇ (!lNot->has_ਗੁਣਜ_Sinks() && !rNot->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(ਧੱਕੋ_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ਅਚਲ newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        {
            ਤਾਰਕਿਕ ਅਦਲਾ_ਬਦਲੀ = ਝੂਠ;
            ਜੇ (isConcatPushableThroughBitwise(lhsp, rhsp, ਅਦਲਾ_ਬਦਲੀ)) {
                APPLYING(ਧੱਕੋ_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, ਅਦਲਾ_ਬਦਲੀ));
                    ਵਾਪਸ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ਜੇ (DfgConcat* ਅਚਲ rConcatp = rhsp->cast<DfgConcat>()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rlp = rConcatp->lhsp();
                ਜੇ (!rConcatp->has_ਗੁਣਜ_Sinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, ਅਦਲਾ_ਬਦਲੀ)) {
                    APPLYING(ਧੱਕੋ_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, ਅਦਲਾ_ਬਦਲੀ);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (DfgSel* ਅਚਲ lSelp = lhsp->cast<DfgSel>()) {
            ਜੇ (DfgSel* ਅਚਲ rSelp = rhsp->cast<DfgSel>()) {
                ਜੇ (isSame(lSelp->fromp(), rSelp->fromp())) {
                    ਜੇ (lSelp->lsb() == rSelp->lsb() + rSelp->ਚੌੜਾਈ()) {
                        APPLYING(REMOVE_CONCAT_ਦਾ_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ਵਾਪਸ;
                        }
                    }
                }
            }

            ਜੇ (DfgConcat* ਅਚਲ rConcatp = rhsp->cast<DfgConcat>()) {
                ਜੇ (DfgSel* ਅਚਲ rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    ਜੇ (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        ਜੇ (lSelp->lsb() == rlSelp->lsb() + rlSelp->ਚੌੜਾਈ()) {
                            APPLYING(REPLACE_NESTED_CONCAT_ਦਾ_ADJOINING_SELS_ON_LHS) {
                                ਅਚਲ uint32_t ਚੌੜਾਈ = lSelp->ਚੌੜਾਈ() + rlSelp->ਚੌੜਾਈ();
                                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = Dfg_ਅੰਕੜੇ_Type::packed(ਚੌੜਾਈ);
                                DfgSel* ਅਚਲ selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ਵਾਪਸ;
                            }
                        }
                    }
                }
            }
        }

        ਜੇ (DfgSel* ਅਚਲ rSelp = rhsp->cast<DfgSel>()) {
            ਜੇ (DfgConcat* ਅਚਲ lConcatp = lhsp->cast<DfgConcat>()) {
                ਜੇ (DfgSel* ਅਚਲ lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    ਜੇ (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        ਜੇ (lrSelp->lsb() == rSelp->lsb() + rSelp->ਚੌੜਾਈ()) {
                            APPLYING(REPLACE_NESTED_CONCAT_ਦਾ_ADJOINING_SELS_ON_RHS) {
                                ਅਚਲ uint32_t ਚੌੜਾਈ = lrSelp->ਚੌੜਾਈ() + rSelp->ਚੌੜਾਈ();
                                ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = Dfg_ਅੰਕੜੇ_Type::packed(ਚੌੜਾਈ);
                                DfgSel* ਅਚਲ selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ਵਾਪਸ;
                            }
                        }
                    }
                }
            }
        }

        ਜੇ (DfgConst* ਅਚਲ lConstp = lhsp->cast<DfgConst>()) {
            ਜੇ (DfgCond* ਅਚਲ rCondp = rhsp->cast<DfgCond>()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rtVtxp = rCondp->thenp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ reVtxp = rCondp->elsep();
                DfgConst* ਅਚਲ rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ਅਚਲ reConstp = reVtxp->cast<DfgConst>();
                ਜੇ (!rCondp->has_ਗੁਣਜ_Sinks() && (rtConstp || reConstp)) {
                    APPLYING(ਧੱਕੋ_CONCAT_THROUGH_COND_LHS) {
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ thenp = [&]() -> Dfg_ਸ਼ੀਰਸ਼* {
                            File_ਸਤਰ* ਅਚਲ rtFlp = rtVtxp->fileline();
                            ਜੇ (rtConstp) {
                                DfgConst* ਅਚਲ constp = makeZero(rtFlp, vtxp->ਚੌੜਾਈ());
                                constp->ਅੰਕ().opConcat(lConstp->ਅੰਕ(), rtConstp->ਅੰਕ());
                                ਵਾਪਸ constp;
                            }
                            ਵਾਪਸ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ elsep = [&]() -> Dfg_ਸ਼ੀਰਸ਼* {
                            File_ਸਤਰ* ਅਚਲ reFlp = reVtxp->fileline();
                            ਜੇ (reConstp) {
                                DfgConst* ਅਚਲ constp = makeZero(reFlp, vtxp->ਚੌੜਾਈ());
                                constp->ਅੰਕ().opConcat(lConstp->ਅੰਕ(), reConstp->ਅੰਕ());
                                ਵਾਪਸ constp;
                            }
                            ਵਾਪਸ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (DfgConst* ਅਚਲ rConstp = rhsp->cast<DfgConst>()) {
            ਜੇ (DfgCond* ਅਚਲ lCondp = lhsp->cast<DfgCond>()) {
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ ltVtxp = lCondp->thenp();
                Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ leVtxp = lCondp->elsep();
                DfgConst* ਅਚਲ ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ਅਚਲ leConstp = leVtxp->cast<DfgConst>();
                ਜੇ (!lCondp->has_ਗੁਣਜ_Sinks() && (ltConstp || leConstp)) {
                    APPLYING(ਧੱਕੋ_CONCAT_THROUGH_COND_RHS) {
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ thenp = [&]() -> Dfg_ਸ਼ੀਰਸ਼* {
                            File_ਸਤਰ* ਅਚਲ ltFlp = ltVtxp->fileline();
                            ਜੇ (ltConstp) {
                                DfgConst* ਅਚਲ constp = makeZero(ltFlp, vtxp->ਚੌੜਾਈ());
                                constp->ਅੰਕ().opConcat(ltConstp->ਅੰਕ(), rConstp->ਅੰਕ());
                                ਵਾਪਸ constp;
                            }
                            ਵਾਪਸ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ elsep = [&]() -> Dfg_ਸ਼ੀਰਸ਼* {
                            File_ਸਤਰ* ਅਚਲ leFlp = leVtxp->fileline();
                            ਜੇ (leConstp) {
                                DfgConst* ਅਚਲ constp = makeZero(leFlp, vtxp->ਚੌੜਾਈ());
                                constp->ਅੰਕ().opConcat(leConstp->ਅੰਕ(), rConstp->ਅੰਕ());
                                ਵਾਪਸ constp;
                            }
                            ਵਾਪਸ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ਅਚਲ uint32_t vMsb = vtxp->ਚੌੜਾਈ() - 1;  // MSB of the concatenation
            ਅਚਲ uint32_t lLsb = vtxp->rhsp()->ਚੌੜਾਈ();  // LSB of the LHS
            ਅਚਲ uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            ਤਾਰਕਿਕ hasCrossSink = ਝੂਠ;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) {
                // Record bits used by DfgSel sinks
                ਜੇ (ਅਚਲ DfgSel* ਅਚਲ selp = sink.cast<DfgSel>()) {
                    ਅਚਲ uint32_t selLsb = selp->lsb();
                    ਅਚਲ uint32_t selMsb = selLsb + selp->ਚੌੜਾਈ() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ਵਾਪਸ ਝੂਠ;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                ਜੇ (ਅਚਲ DfgVarPacked* ਅਚਲ varp = sink.cast<DfgVarPacked>()) {
                    ਜੇ (!varp->hasSinks() && !varp->isObserved()) ਵਾਪਸ ਝੂਠ;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ਵਾਪਸ ਸੱਚ;
            });
            ਜੇ (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    Dfg_ਸ਼ੀਰਸ਼* nRhsp = rhsp;
                    ਜੇ (lsb != 0)
                        nRhsp = make<DfgSel>(flp, Dfg_ਅੰਕੜੇ_Type::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    Dfg_ਸ਼ੀਰਸ਼* nLhsp = lhsp;
                    ਜੇ (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, Dfg_ਅੰਕੜੇ_Type::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ catp
                        = make<DfgConcat>(flp, Dfg_ਅੰਕੜੇ_Type::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ਅਚਲ sp = ਨਵਾਂ DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_ਆਖਰੀ_Id;
                    sp->ਜੋੜੋ_Driver(catp, lsb, flp);
                    Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_ਆਖਰੀ_Id;
                    varp->vscp()->varp()->is_ਅੰਦਰੂਨੀ(ਸੱਚ);
                    varp->srcp(sp);
                    replace(varp);
                    ਵਾਪਸ;
                }
            }
        }

        // Convert various forms to REPLICATE
        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ਵਾਪਸ;
            }
        }
        ਜੇ (DfgRep* ਅਚਲ lRepp = lhsp->cast<DfgRep>()) {
            ਜੇ (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ਵਾਪਸ;
                }
            }
        }
        ਜੇ (DfgRep* ਅਚਲ rRepp = rhsp->cast<DfgRep>()) {
            ਜੇ (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ਵਾਪਸ;
                }
            }
        }
        ਜੇ (DfgRep* ਅਚਲ lRepp = lhsp->cast<DfgRep>()) {
            ਜੇ (DfgRep* ਅਚਲ rRepp = rhsp->cast<DfgRep>()) {
                ਜੇ (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ਵਾਪਸ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ਜੇ (DfgConcat* ਅਚਲ rConcatp = rhsp->cast<DfgConcat>()) {
                ਜੇ (DfgRep* ਅਚਲ rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    ਜੇ (!rConcatp->has_ਗੁਣਜ_Sinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ਅਚਲ uint32_t ਚੌੜਾਈ = lRepp->ਚੌੜਾਈ() + rlRepp->ਚੌੜਾਈ();
                            ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype = Dfg_ਅੰਕੜੇ_Type::packed(ਚੌੜਾਈ);
                            DfgRep* ਅਚਲ repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ਵਾਪਸ;
                        }
                    }
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgDiv* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        ਜੇ (DfgConst* ਅਚਲ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ਜੇ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgDivS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        ਜੇ (DfgConst* ਅਚਲ rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ਜੇ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgGt* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgGtS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgGte* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgGteS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLogAnd* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp) || vtxp->rhsp()->unsafe()) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();

        ਜੇ (lhsp->ਚੌੜਾਈ() == 1 && rhsp->ਚੌੜਾਈ() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLogEq* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLogIf* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp) || vtxp->rhsp()->unsafe()) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLogOr* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp) || vtxp->rhsp()->unsafe()) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();

        ਜੇ (lhsp->ਚੌੜਾਈ() == 1 && rhsp->ਚੌੜਾਈ() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLt* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLtS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLte* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgLteS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ਵਾਪਸ;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgModDiv* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgModDivS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgMul* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (DfgConst* ਅਚਲ lConstp = lhsp->cast<DfgConst>()) {
            ਜੇ (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgMulS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (DfgConst* ਅਚਲ lConstp = lhsp->cast<DfgConst>()) {
            ਜੇ (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgEq* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ਵਾਪਸ;
            }
        }

        ਜੇ (DfgConst* ਅਚਲ lhsConstp = lhsp->cast<DfgConst>()) {
            ਜੇ (DfgConcat* ਅਚਲ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ਜੇ (try_ਧੱਕੋ_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ਵਾਪਸ;
            }

            ਜੇ (rhsp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {
                ਜੇ (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_ਦੋਆਧਾਰੀ_ਅੰਕ_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ਵਾਪਸ;
                    }
                }

                ਜੇ (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_ਦੋਆਧਾਰੀ_ਅੰਕ_1) {
                        replace(rhsp);
                        ਵਾਪਸ;
                    }
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgNeq* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ਵਾਪਸ;
            }
        }

        ਜੇ (DfgConst* ਅਚਲ lhsConstp = lhsp->cast<DfgConst>()) {
            ਜੇ (DfgConcat* ਅਚਲ rhsConcatp = rhsp->cast<DfgConcat>()) {
                ਜੇ (try_ਧੱਕੋ_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) ਵਾਪਸ;
            }

            ਜੇ (rhsp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {
                ਜੇ (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_ਦੋਆਧਾਰੀ_ਅੰਕ_0) {
                        replace(rhsp);
                        ਵਾਪਸ;
                    }
                }

                ਜੇ (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_ਦੋਆਧਾਰੀ_ਅੰਕ_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ਵਾਪਸ;
                    }
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(DfgPow* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgPowSS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgPowSU* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgPowUS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgRep* ਅਚਲ vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ srcp = vtxp->srcp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (DfgConst* ਅਚਲ sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ਅਚਲ resultp = makeZero(flp, vtxp->ਚੌੜਾਈ());
                resultp->ਅੰਕ().opRepl(sConstp->ਅੰਕ(), vtxp->ਗਿਣਤੀ_2());
                replace(resultp);
                ਵਾਪਸ;
            }
        }

        ਜੇ (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ਵਾਪਸ;
            }
        }

        ਜੇ (DfgRep* ਅਚਲ sRepp = srcp->cast<DfgRep>()) {
            ਜੇ (!sRepp->has_ਗੁਣਜ_Sinks() || vtxp->ਚੌੜਾਈ() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_L* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
        ਜੇ (optimize_ਖਿਸਕਾਓ_RHS(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();

        ਜੇ (Dfg_ਖਿਸਕਾਓ_L* ਅਚਲ l_ਖਿਸਕਾਓ_Lp = lhsp->cast<Dfg_ਖਿਸਕਾਓ_L>()) {
            ਜੇ (!l_ਖਿਸਕਾਓ_Lp->has_ਗੁਣਜ_Sinks() && rhsp->dtype() == l_ਖਿਸਕਾਓ_Lp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
                    ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਜੋੜ_DType = Dfg_ਅੰਕੜੇ_Type::packed(rhsp->ਚੌੜਾਈ() + 1);
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ bp = make<DfgExtend>(flp, ਜੋੜ_DType, l_ਖਿਸਕਾਓ_Lp->rhsp());
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ cp = make<DfgExtend>(flp, ਜੋੜ_DType, rhsp);
                    Dfg_ਜੋੜੋ* ਅਚਲ addp = make<Dfg_ਜੋੜੋ>(flp, ਜੋੜ_DType, bp, cp);
                    replace(make<Dfg_ਖਿਸਕਾਓ_L>(vtxp, l_ਖਿਸਕਾਓ_Lp->lhsp(), addp));
                    ਵਾਪਸ;
                }
            }
        }

        ਜੇ (DfgConst* ਅਚਲ rConstp = rhsp->cast<DfgConst>()) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amount = rConstp->ਮੰਜ਼ਿਲ_U32();

            ਜੇ (ਖਿਸਕਾਓ_Amount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ਵਾਪਸ;
                }
            }

            ਜੇ (ਖਿਸਕਾਓ_Amount >= vtxp->ਚੌੜਾਈ()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->ਚੌੜਾਈ()));
                    ਵਾਪਸ;
                }
            }

            ਜੇ (DfgConcat* ਅਚਲ lConcatp = lhsp->cast<DfgConcat>()) {
                ਜੇ (!lConcatp->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lRhsp = lConcatp->rhsp();
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ਅਚਲ uint32_t rW = ਖਿਸਕਾਓ_Amount;
                        ਅਚਲ uint32_t mW = std::min(lRhsp->ਚੌੜਾਈ(), vtxp->ਚੌੜਾਈ() - ਖਿਸਕਾਓ_Amount);
                        ਅਚਲ uint32_t lW = vtxp->ਚੌੜਾਈ() - mW - rW;
                        // Construct the result
                        File_ਸਤਰ* ਅਚਲ flp = lConcatp->fileline();
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rp = makeZero(flp, ਖਿਸਕਾਓ_Amount);
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ mp
                            = make<DfgSel>(flp, Dfg_ਅੰਕੜੇ_Type::packed(mW), lRhsp, 0U);
                        Dfg_ਸ਼ੀਰਸ਼* np = make<DfgConcat>(flp, Dfg_ਅੰਕੜੇ_Type::packed(mW + rW), mp, rp);
                        ਜੇ (!lW) {
                            replace(np);
                            ਵਾਪਸ;
                        }
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lp
                            = make<DfgSel>(flp, Dfg_ਅੰਕੜੇ_Type::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ਵਾਪਸ;
                    }
                }
            }

            ਜੇ (DfgSel* ਅਚਲ lSelp = lhsp->cast<DfgSel>()) {
                ਜੇ (!lSelp->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ਅਚਲ uint32_t n_Sel_ਚੌੜਾਈ = lSelp->ਚੌੜਾਈ() - ਖਿਸਕਾਓ_Amount;
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_ਅੰਕੜੇ_Type::packed(n_Sel_ਚੌੜਾਈ),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), ਖਿਸਕਾਓ_Amount)));
                        ਵਾਪਸ;
                    }
                }
            }

            ਜੇ (DfgCond* ਅਚਲ lCondp = lhsp->cast<DfgCond>()) {
                ਜੇ (!lCondp->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(ਧੱਕੋ_SHIFTL_THROUGH_COND) {
                        Dfg_ਖਿਸਕਾਓ_L* ਅਚਲ tp = make<Dfg_ਖਿਸਕਾਓ_L>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_ਖਿਸਕਾਓ_L* ਅਚਲ ep = make<Dfg_ਖਿਸਕਾਓ_L>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ਵਾਪਸ;
                    }
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_R* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
        ਜੇ (optimize_ਖਿਸਕਾਓ_RHS(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();

        ਜੇ (Dfg_ਖਿਸਕਾਓ_R* ਅਚਲ l_ਖਿਸਕਾਓ_Rp = lhsp->cast<Dfg_ਖਿਸਕਾਓ_R>()) {
            ਜੇ (!l_ਖਿਸਕਾਓ_Rp->has_ਗੁਣਜ_Sinks() && rhsp->dtype() == l_ਖਿਸਕਾਓ_Rp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();
                    ਅਚਲ Dfg_ਅੰਕੜੇ_Type& ਜੋੜ_DType = Dfg_ਅੰਕੜੇ_Type::packed(rhsp->ਚੌੜਾਈ() + 1);
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ bp = make<DfgExtend>(flp, ਜੋੜ_DType, l_ਖਿਸਕਾਓ_Rp->rhsp());
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ cp = make<DfgExtend>(flp, ਜੋੜ_DType, rhsp);
                    Dfg_ਜੋੜੋ* ਅਚਲ addp = make<Dfg_ਜੋੜੋ>(flp, ਜੋੜ_DType, bp, cp);
                    replace(make<Dfg_ਖਿਸਕਾਓ_R>(vtxp, l_ਖਿਸਕਾਓ_Rp->lhsp(), addp));
                    ਵਾਪਸ;
                }
            }
        }

        ਜੇ (DfgConst* ਅਚਲ rConstp = rhsp->cast<DfgConst>()) {
            ਅਚਲ uint32_t ਖਿਸਕਾਓ_Amount = rConstp->ਮੰਜ਼ਿਲ_U32();

            ਜੇ (ਖਿਸਕਾਓ_Amount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ਵਾਪਸ;
                }
            }

            ਜੇ (ਖਿਸਕਾਓ_Amount >= vtxp->ਚੌੜਾਈ()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->ਚੌੜਾਈ()));
                    ਵਾਪਸ;
                }
            }

            ਜੇ (DfgConcat* ਅਚਲ lConcatp = lhsp->cast<DfgConcat>()) {
                ਜੇ (!lConcatp->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lRhsp = lConcatp->rhsp();
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ਅਚਲ uint32_t lW = ਖਿਸਕਾਓ_Amount;
                        ਅਚਲ uint32_t mW = std::min(lLhsp->ਚੌੜਾਈ(), vtxp->ਚੌੜਾਈ() - ਖਿਸਕਾਓ_Amount);
                        ਅਚਲ uint32_t rW = vtxp->ਚੌੜਾਈ() - mW - lW;
                        // Construct the result
                        File_ਸਤਰ* ਅਚਲ flp = lConcatp->fileline();
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lp = makeZero(flp, ਖਿਸਕਾਓ_Amount);
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ mp = make<DfgSel>(flp, Dfg_ਅੰਕੜੇ_Type::packed(mW), lLhsp,
                                                           lLhsp->ਚੌੜਾਈ() - mW);
                        Dfg_ਸ਼ੀਰਸ਼* np = make<DfgConcat>(flp, Dfg_ਅੰਕੜੇ_Type::packed(lW + mW), lp, mp);
                        ਜੇ (!rW) {
                            replace(np);
                            ਵਾਪਸ;
                        }
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rp = make<DfgSel>(flp, Dfg_ਅੰਕੜੇ_Type::packed(rW), lRhsp,
                                                           lRhsp->ਚੌੜਾਈ() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ਵਾਪਸ;
                    }
                }
            }

            ਜੇ (DfgSel* ਅਚਲ lSelp = lhsp->cast<DfgSel>()) {
                ਜੇ (!lSelp->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ਅਚਲ uint32_t n_Sel_ਚੌੜਾਈ = lSelp->ਚੌੜਾਈ() - ਖਿਸਕਾਓ_Amount;
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_ਅੰਕੜੇ_Type::packed(n_Sel_ਚੌੜਾਈ),
                                           lSelp->fromp(), lSelp->lsb() + ਖਿਸਕਾਓ_Amount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), ਖਿਸਕਾਓ_Amount), nSelp));
                        ਵਾਪਸ;
                    }
                }
            }

            ਜੇ (DfgCond* ਅਚਲ lCondp = lhsp->cast<DfgCond>()) {
                ਜੇ (!lCondp->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(ਧੱਕੋ_SHIFTR_THROUGH_COND) {
                        Dfg_ਖਿਸਕਾਓ_R* ਅਚਲ tp = make<Dfg_ਖਿਸਕਾਓ_R>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_ਖਿਸਕਾਓ_R* ਅਚਲ ep = make<Dfg_ਖਿਸਕਾਓ_R>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ਵਾਪਸ;
                    }
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਖਿਸਕਾਓ_RS* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;
        ਜੇ (optimize_ਖਿਸਕਾਓ_RHS(vtxp)) ਵਾਪਸ;
    }

    ਖਾਲੀ ਦੇਖੋ(DfgSub* ਅਚਲ vtxp) override {
        ਜੇ (ਦੁਆਧਾਰੀ(vtxp)) ਵਾਪਸ;

        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = vtxp->lhsp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = vtxp->rhsp();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ਵਾਪਸ;
            }
        }

        ਜੇ (DfgConst* ਅਚਲ rConstp = rhsp->cast<DfgConst>()) {
            ਜੇ (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ਵਾਪਸ;
                }
            }
            ਜੇ (vtxp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType && rConstp->has_ਮੁੱਲ(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType, lhsp));
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਮੇਲ_Masked* ਅਚਲ vtxp) override {
        ਜੇ (DfgConst* ਅਚਲ constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ਅਚਲ ਮੇਲ_Varp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), Ast_ਮੇਲ_Masked::fold(constp->ਅੰਕ(), ਮੇਲ_Varp)));
                ਵਾਪਸ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    ਖਾਲੀ ਦੇਖੋ(DfgCond* ਅਚਲ vtxp) override {
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ condp = vtxp->condp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ thenp = vtxp->thenp();
        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ elsep = vtxp->elsep();
        File_ਸਤਰ* ਅਚਲ flp = vtxp->fileline();

        ਜੇ (condp->dtype() != m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) ਵਾਪਸ;

        ਜੇ (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ਵਾਪਸ;
            }
        }

        ਜੇ (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ਵਾਪਸ;
            }
        }

        ਜੇ (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ਵਾਪਸ;
            }
        }

        ਜੇ (DfgNot* ਅਚਲ condNotp = condp->cast<DfgNot>()) {
            ਜੇ (!condp->has_ਗੁਣਜ_Sinks() || condNotp->has_ਗੁਣਜ_Sinks()) {
                APPLYING(ਅਦਲਾ_ਬਦਲੀ_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ਵਾਪਸ;
                }
            }
        }

        ਜੇ (DfgNeq* ਅਚਲ condNeqp = condp->cast<DfgNeq>()) {
            ਜੇ (!condp->has_ਗੁਣਜ_Sinks()) {
                APPLYING(ਅਦਲਾ_ਬਦਲੀ_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ਅਚਲ newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ਵਾਪਸ;
                }
            }
        }

        ਜੇ (DfgNot* ਅਚਲ thenNotp = thenp->cast<DfgNot>()) {
            ਜੇ (DfgNot* ਅਚਲ elseNotp = elsep->cast<DfgNot>()) {
                ਜੇ (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->has_ਗੁਣਜ_Sinks() && !elseNotp->has_ਗੁਣਜ_Sinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ਅਚਲ newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (DfgOr* ਅਚਲ condOrp = condp->cast<DfgOr>()) {
            ਜੇ (DfgCond* ਅਚਲ thenCondp = thenp->cast<DfgCond>()) {
                ਜੇ (!thenCondp->has_ਗੁਣਜ_Sinks()) {
                    ਜੇ (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ਅਚਲ ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ਵਾਪਸ;
                        }
                    }
                    ਜੇ (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ਅਚਲ ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ਵਾਪਸ;
                        }
                    }
                }
            }
        }

        ਜੇ (vtxp->ਚੌੜਾਈ() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            ਜੇ (Dfg_ਜੋੜੋ* ਅਚਲ thenAddp = thenp->cast<Dfg_ਜੋੜੋ>()) {
                ਜੇ (DfgConst* ਅਚਲ constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    ਜੇ (constp->has_ਮੁੱਲ(1)) {
                        ਜੇ (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ਅਚਲ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->ਚੌੜਾਈ() - 1), condp);
                                File_ਸਤਰ* ਅਚਲ thenFlp = thenAddp->fileline();
                                replace(
                                    make<Dfg_ਜੋੜੋ>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ਵਾਪਸ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            ਜੇ (DfgSub* ਅਚਲ thenSubp = thenp->cast<DfgSub>()) {
                ਜੇ (DfgConst* ਅਚਲ constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    ਜੇ (constp->has_ਮੁੱਲ(1)) {
                        ਜੇ (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ਅਚਲ extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->ਚੌੜਾਈ() - 1), condp);
                                File_ਸਤਰ* ਅਚਲ thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ਵਾਪਸ;
                            }
                        }
                    }
                }
            }
        }

        ਜੇ (vtxp->dtype() == m_ਦੋਆਧਾਰੀ_ਅੰਕ_DType) {
            ਜੇ (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ਵਾਪਸ;
                }
            }
        }

        ਜੇ (vtxp->ਚੌੜਾਈ() <= VL_QUADSIZE) {
            ਜੇ (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ਵਾਪਸ;
                }
            }
            ਜੇ (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ਵਾਪਸ;
                }
            }

            ਜੇ (DfgOr* ਅਚਲ tOrp = thenp->cast<DfgOr>()) {
                ਜੇ (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ maskp = replicate(vtxp, condp);
                        DfgAnd* ਅਚਲ andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ਵਾਪਸ;
                    }
                }
                ਜੇ (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ maskp = replicate(vtxp, condp);
                        DfgAnd* ਅਚਲ andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ਵਾਪਸ;
                    }
                }
            }
        }

        ਜੇ (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ਅਚਲ std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            ਜੇ (cLSBs.ਪਹਿਲਾ) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ਅਚਲ uint32_t r_ਚੌੜਾਈ = cLSBs.ਦੂਜਾ;
                    Dfg_ਸ਼ੀਰਸ਼* rhsp = cLSBs.ਪਹਿਲਾ;
                    ਜੇ (r_ਚੌੜਾਈ != rhsp->ਚੌੜਾਈ()) {
                        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& rDtype = Dfg_ਅੰਕੜੇ_Type::packed(r_ਚੌੜਾਈ);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    ਜੇ (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ਵਾਪਸ;
                    }
                    // Create new LHS
                    ਅਚਲ uint32_t l_ਚੌੜਾਈ = vtxp->ਚੌੜਾਈ() - r_ਚੌੜਾਈ;
                    ਅਚਲ Dfg_ਅੰਕੜੇ_Type& lDtype = Dfg_ਅੰਕੜੇ_Type::packed(l_ਚੌੜਾਈ);
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lThenp = make<DfgSel>(flp, lDtype, thenp, r_ਚੌੜਾਈ);
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lElsep = make<DfgSel>(flp, lDtype, elsep, r_ਚੌੜਾਈ);
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ਵਾਪਸ;
                }
            }

            ਅਚਲ std::pair<Dfg_ਸ਼ੀਰਸ਼*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            ਜੇ (cMSBs.ਪਹਿਲਾ) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ਅਚਲ uint32_t l_ਚੌੜਾਈ = cMSBs.ਦੂਜਾ;
                    Dfg_ਸ਼ੀਰਸ਼* lhsp = cMSBs.ਪਹਿਲਾ;
                    ਜੇ (l_ਚੌੜਾਈ != lhsp->ਚੌੜਾਈ()) {
                        ਅਚਲ Dfg_ਅੰਕੜੇ_Type& lDtype = Dfg_ਅੰਕੜੇ_Type::packed(l_ਚੌੜਾਈ);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->ਚੌੜਾਈ() - l_ਚੌੜਾਈ);
                    }
                    // If it's all the same, just replace
                    ਜੇ (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ਵਾਪਸ;
                    }
                    // Create new RHS
                    ਅਚਲ uint32_t r_ਚੌੜਾਈ = vtxp->ਚੌੜਾਈ() - l_ਚੌੜਾਈ;
                    ਅਚਲ Dfg_ਅੰਕੜੇ_Type& rDtype = Dfg_ਅੰਕੜੇ_Type::packed(r_ਚੌੜਾਈ);
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ਵਾਪਸ;
                }
            }
        }

        ਜੇ (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                Dfg_ਸ਼ੀਰਸ਼* resp = condp;
                ਜੇ (ਅਚਲ uint32_t extend = vtxp->ਚੌੜਾਈ() - 1) {
                    DfgConst* ਅਚਲ zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ਵਾਪਸ;
            }
        }

        ਜੇ (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                Dfg_ਸ਼ੀਰਸ਼* resp = make<DfgNot>(condp, condp);
                ਜੇ (ਅਚਲ uint32_t extend = vtxp->ਚੌੜਾਈ() - 1) {
                    DfgConst* ਅਚਲ zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ਵਾਪਸ;
            }
        }

        ਜੇ (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ਵਾਪਸ;
            }
        }

        ਜੇ (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ਵਾਪਸ;
            }
        }

        ਜੇ (DfgCond* ਅਚਲ tCondp = thenp->cast<DfgCond>()) {
            ਜੇ (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ਵਾਪਸ;
                }
            }
        }

        ਜੇ (DfgCond* ਅਚਲ eCondp = elsep->cast<DfgCond>()) {
            ਜੇ (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ਵਾਪਸ;
                }
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ vtxp) override {
        ਜੇ (vtxp->hasSinks()) ਵਾਪਸ;
        ਜੇ (vtxp->isObserved()) ਵਾਪਸ;
        ਜੇ (vtxp->defaultp()) ਵਾਪਸ;

        // If undriven, or driven from another var, it is completely redundant.
        ਜੇ (!vtxp->srcp() || vtxp->srcp()->is<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
            APPLYING(REMOVE_VAR) {
                delete_ਸ਼ੀਰਸ਼(vtxp);
                ਵਾਪਸ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        ਤਾਰਕਿਕ ਮਿਲਿਆ_One = ਝੂਠ;
        ਅਚਲ ਤਾਰਕਿਕ keep = vtxp->srcp()->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& sink) -> ਤਾਰਕਿਕ {
            // Ignore non-observable variable sinks. These can be eliminated.
            ਜੇ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = sink.cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                ਜੇ (!varp->hasSinks() && !varp->isObserved()) ਵਾਪਸ ਝੂਠ;
            }
            // Keep if found more than one sink
            ਜੇ (ਮਿਲਿਆ_One) ਵਾਪਸ ਸੱਚ;
            // Mark first sink found
            ਮਿਲਿਆ_One = ਸੱਚ;
            ਵਾਪਸ ਝੂਠ;
        });
        ਜੇ (!keep) {
            APPLYING(REMOVE_VAR) {
                delete_ਸ਼ੀਰਸ਼(vtxp);
                ਵਾਪਸ;
            }
        }
    }

    V3DfgPeephole(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.for_Each_ਸ਼ੀਰਸ਼([&](Dfg_ਸ਼ੀਰਸ਼& vtx) { m_vInfo[vtx].m_id = ++m_ਆਖਰੀ_Id; });

        // Add all operation vertices to the cache
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) cache_ਸ਼ੀਰਸ਼(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.ਮਾਪ() * 2);
        m_iterList.reserve(m_dfg.ਮਾਪ() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ਧੱਕੋ_ਪਿੱਠ(ਖਾਲੀ_ਸੂਚਕ);
        m_iterList.ਧੱਕੋ_ਪਿੱਠ(ਖਾਲੀ_ਸੂਚਕ);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼_Var& vtx : m_dfg.varVertices()) ਜੋੜੋ_ਮੰਜ਼ਿਲ_Work_List(&vtx);

        // Add all operation vertices to the work list
        ਲਈ (Dfg_ਸ਼ੀਰਸ਼& vtx : m_dfg.opVertices()) ਜੋੜੋ_ਮੰਜ਼ਿਲ_Work_List(&vtx);

        // Process iteratively
        ਜਦੋਂ (ਸੱਚ) {
            // Process the work list - keep the placeholder at index 0
            ਜਦੋਂ (m_workList.ਮਾਪ() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.ਪਿੱਠ();
                m_workList.ਕੱਢੋ_ਪਿੱਠ();
                ਜੇ (!m_vtxp) ਜਾਰੀ;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_work_List_ਸੂਚਕਾਂਕ = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                ਜੇ (Dfg_ਸ਼ੀਰਸ਼_Var* ਅਚਲ varp = m_vtxp->cast<Dfg_ਸ਼ੀਰਸ਼_Var>()) {
                    ਦੇਖੋ(varp);
                    ਜਾਰੀ;
                }

                // Ast references are not considered
                ਜੇ (m_vtxp->is<Dfg_ਸ਼ੀਰਸ਼_Ast>()) ਜਾਰੀ;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                ਜੇ (!m_vInfo[m_vtxp].m_is_Cached_ਸ਼ੀਰਸ਼) {
                    ਜੇ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ sampep = cache_ਸ਼ੀਰਸ਼(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            ਜਾਰੀ;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            ਜੇ (m_iterList.ਮਾਪ() == 1) ਰੋਕੋ;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                ਮਾਪ_t begin = 0;
                ਮਾਪ_t ਸਿਰਾ = m_iterList.ਮਾਪ();
                ਲਈ (ਮਾਪ_t hops = 1; hops <= 4; ++hops) {
                    ਲਈ (ਮਾਪ_t i = begin; i < ਸਿਰਾ; ++i) {
                        Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = m_iterList[i];
                        ਜੇ (!vtxp) ਜਾਰੀ;
                        vtxp->foreachSink([&](Dfg_ਸ਼ੀਰਸ਼& dst) {
                            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Iter_List(&dst);
                            ਵਾਪਸ ਝੂਠ;
                        });
                        vtxp->foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
                            ਜੋੜੋ_ਮੰਜ਼ਿਲ_Iter_List(&src);
                            ਵਾਪਸ ਝੂਠ;
                        });
                    }
                    begin = ਸਿਰਾ;
                    ਸਿਰਾ = m_iterList.ਮਾਪ();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : m_iterList) {
                ਜੇ (!vtxp) ਜਾਰੀ;
                removeFromIterList(vtxp);
                ਜੋੜੋ_ਮੰਜ਼ਿਲ_Work_List(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ APPLYING

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

ਖਾਲੀ V3DfgPasses::peephole(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgPeepholeContext& ctx) {
    ਜੇ (!v3Global.opt.fDfgPeephole()) ਵਾਪਸ;
    V3DfgPeephole::apply(dfg, ctx);
}
