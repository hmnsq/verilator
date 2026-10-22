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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgCache.h"
#అన్తర్భావ "V3DfgDataType.h"
#అన్తర్భావ "V3DfgPasses.h"
#అన్తర్భావ "V3DfgPeepholePatterns.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cctype>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
నామాకాశ {

మూస <రకం_పేరు T_Reduction>
నిర్మాణం Reduction_గమ్యం_Bitwise_Impl {};
మూస <> నిర్మాణం Reduction_గమ్యం_Bitwise_Impl<DfgRedAnd> { ఉపయోగించి type = DfgAnd; };
మూస <> నిర్మాణం Reduction_గమ్యం_Bitwise_Impl<DfgRedOr>  { ఉపయోగించి type = DfgOr;  };
మూస <> నిర్మాణం Reduction_గమ్యం_Bitwise_Impl<DfgRedXor> { ఉపయోగించి type = DfgXor; };
మూస <రకం_పేరు T_Reduction>
ఉపయోగించి Reduction_గమ్యం_Bitwise = రకం_పేరు Reduction_గమ్యం_Bitwise_Impl<T_Reduction>::type;

మూస <రకం_పేరు T_Bitwise>
నిర్మాణం Bitwise_గమ్యం_Reduction_Impl {};
మూస <> నిర్మాణం Bitwise_గమ్యం_Reduction_Impl<DfgAnd> { ఉపయోగించి type = DfgRedAnd; };
మూస <> నిర్మాణం Bitwise_గమ్యం_Reduction_Impl<DfgOr>  { ఉపయోగించి type = DfgRedOr;  };
మూస <> నిర్మాణం Bitwise_గమ్యం_Reduction_Impl<DfgXor> { ఉపయోగించి type = DfgRedXor; };
మూస <రకం_పేరు T_Reduction>
ఉపయోగించి Bitwise_గమ్యం_Reduction = రకం_పేరు Bitwise_గమ్యం_Reduction_Impl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
మూస<రకం_పేరు శీర్షం_2>
నిర్మాణం IsAssociative : బహిరంగ std::false_type {};
మూస <> నిర్మాణం IsAssociative<DfgAnd> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsAssociative<DfgOr>  : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsAssociative<DfgXor> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsAssociative<Dfg_కూడు> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsAssociative<DfgConcat> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsAssociative<DfgMul> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsAssociative<DfgMulS> : బహిరంగ std::true_type {};

// Commutative binary operators (a op b == b op a)
మూస<రకం_పేరు శీర్షం_2>
నిర్మాణం IsCommutative : బహిరంగ std::false_type {};
మూస <> నిర్మాణం IsCommutative<DfgAnd> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsCommutative<DfgOr>  : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsCommutative<DfgXor> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsCommutative<Dfg_కూడు> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsCommutative<DfgEq> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsCommutative<DfgMul> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsCommutative<DfgMulS> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsCommutative<DfgNeq> : బహిరంగ std::true_type {};

// Idempotent binary operators (a op a == a)
మూస<రకం_పేరు శీర్షం_2>
నిర్మాణం IsIdempotent : బహిరంగ std::false_type {};
మూస <> నిర్మాణం IsIdempotent<DfgAnd> : బహిరంగ std::true_type {};
మూస <> నిర్మాణం IsIdempotent<DfgOr>  : బహిరంగ std::true_type {};

// Binary result dtype
మూస<రకం_పేరు శీర్షం_2>
స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType(స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp);
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<Dfg_కూడు>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgAnd>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgConcat>     (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(lhsp->వెడల్పు() + rhsp->వెడల్పు()); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgDiv>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgDivS>       (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgEq>         (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgGt>         (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgGtS>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgGte>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgGteS>       (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgLogAnd>     (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgLogEq>      (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgLogIf>      (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgLogOr>      (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgLt>         (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgLtS>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgLte>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgLteS>       (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgModDiv>     (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgModDivS>    (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgMul>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgMulS>       (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgNeq>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి Dfg_దత్తాంశం_Type::packed(1); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgOr>         (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgPow>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgPowSS>      (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgPowSU>      (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgPowUS>      (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<Dfg_జరుపు_L>     (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<Dfg_జరుపు_R>     (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<Dfg_జరుపు_RS>    (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgSub>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }
మూస <> స్థిరరాశి Dfg_దత్తాంశం_Type& ఫలితం_DType<DfgXor>        (స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) { తిరిగి lhsp->dtype(); }

// Unary constant folding
మూస<రకం_పేరు శీర్షం_2> శూన్య foldOp(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src);
మూస <> శూన్య foldOp<Dfg_లెక్క_Ones>  (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.op_లెక్క_Ones(src); }
మూస <> శూన్య foldOp<DfgExtend>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.op_కేటాయించు(src); }
మూస <> శూన్య foldOp<DfgExtendS>    (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opExtendS(src, src.వెడల్పు()); }
మూస <> శూన్య foldOp<DfgLogNot>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opLogNot(src); }
మూస <> శూన్య foldOp<DfgNegate>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opNegate(src); }
మూస <> శూన్య foldOp<DfgNot>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opNot(src); }
మూస <> శూన్య foldOp<DfgOneHot>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opOneHot(src); }
మూస <> శూన్య foldOp<DfgOneHot0>    (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opOneHot0(src); }
మూస <> శూన్య foldOp<DfgRedAnd>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opRedAnd(src); }
మూస <> శూన్య foldOp<DfgRedOr>      (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opRedOr(src); }
మూస <> శూన్య foldOp<DfgRedXor>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& src) { బహిర్గతం.opRedXor(src); }

// Binary constant folding
మూస<రకం_పేరు శీర్షం_2> శూన్య foldOp(V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
మూస <> శూన్య foldOp<Dfg_కూడు>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.op_కూడు(lhs, rhs); }
మూస <> శూన్య foldOp<DfgAnd>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opAnd(lhs, rhs); }
మూస <> శూన్య foldOp<DfgConcat>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opConcat(lhs, rhs); }
మూస <> శూన్య foldOp<DfgDiv>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opDiv(lhs, rhs); }
మూస <> శూన్య foldOp<DfgDivS>       (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opDivS(lhs, rhs); }
మూస <> శూన్య foldOp<DfgEq>         (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opEq(lhs, rhs); }
మూస <> శూన్య foldOp<DfgGt>         (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opGt(lhs, rhs); }
మూస <> శూన్య foldOp<DfgGtS>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opGtS(lhs, rhs); }
మూస <> శూన్య foldOp<DfgGte>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opGte(lhs, rhs); }
మూస <> శూన్య foldOp<DfgGteS>       (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opGteS(lhs, rhs); }
మూస <> శూన్య foldOp<DfgLogAnd>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opLogAnd(lhs, rhs); }
మూస <> శూన్య foldOp<DfgLogEq>      (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opLogEq(lhs, rhs); }
మూస <> శూన్య foldOp<DfgLogIf>      (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opLogIf(lhs, rhs); }
మూస <> శూన్య foldOp<DfgLogOr>      (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opLogOr(lhs, rhs); }
మూస <> శూన్య foldOp<DfgLt>         (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opLt(lhs, rhs); }
మూస <> శూన్య foldOp<DfgLtS>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opLtS(lhs, rhs); }
మూస <> శూన్య foldOp<DfgLte>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opLte(lhs, rhs); }
మూస <> శూన్య foldOp<DfgLteS>       (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opLteS(lhs, rhs); }
మూస <> శూన్య foldOp<DfgModDiv>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opModDiv(lhs, rhs); }
మూస <> శూన్య foldOp<DfgModDivS>    (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opModDivS(lhs, rhs); }
మూస <> శూన్య foldOp<DfgMul>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opMul(lhs, rhs); }
మూస <> శూన్య foldOp<DfgMulS>       (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opMulS(lhs, rhs); }
మూస <> శూన్య foldOp<DfgNeq>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opNeq(lhs, rhs); }
మూస <> శూన్య foldOp<DfgOr>         (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opOr(lhs, rhs); }
మూస <> శూన్య foldOp<DfgPow>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opPow(lhs, rhs); }
మూస <> శూన్య foldOp<DfgPowSS>      (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opPowSS(lhs, rhs); }
మూస <> శూన్య foldOp<DfgPowSU>      (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opPowSU(lhs, rhs); }
మూస <> శూన్య foldOp<DfgPowUS>      (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opPowUS(lhs, rhs); }
మూస <> శూన్య foldOp<Dfg_జరుపు_L>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.op_జరుపు_L(lhs, rhs); }
మూస <> శూన్య foldOp<Dfg_జరుపు_R>     (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.op_జరుపు_R(lhs, rhs); }
మూస <> శూన్య foldOp<Dfg_జరుపు_RS>    (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.op_జరుపు_RS(lhs, rhs, lhs.వెడల్పు()); }
మూస <> శూన్య foldOp<DfgSub>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opSub(lhs, rhs); }
మూస <> శూన్య foldOp<DfgXor>        (V3_సంఖ్య& బహిర్గతం, స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { బహిర్గతం.opXor(lhs, rhs); }
}
// clang-format on

తరగతి V3DfgPeephole final : బహిరంగ DfgVisitor {
    // TYPES
    నిర్మాణం శీర్షం_Info final {
        కొలత_t m_work_List_సూచీ = 0;  // Position of this vertx m_workList (0 means not in list)
        కొలత_t m_iter_List_సూచీ = 0;  // Position of this vertx m_iterList (0 means not in list)
        కొలత_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        కొలత_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        తార్కిక m_is_Cached_శీర్షం = అబద్ధం;  // This vertex is the vertex cached for its operation
    };

    // STATE
    Dfg_లేఖాచిత్రం& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    స్థిరరాశి Dfg_దత్తాంశం_Type& m_బిట్టు_DType = Dfg_దత్తాంశం_Type::packed(1);  // Common, so grab it up front
    std::సదిశ<Dfg_శీర్షం*> m_workList;  // List of vertices processed in current interation
    std::సదిశ<Dfg_శీర్షం*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<శీర్షం_Info> m_vInfo = m_dfg.makeUserMap<శీర్షం_Info>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    Dfg_శీర్షం* m_vtxp = శూన్యసూచిక;  // Currently considered vertex
    కొలత_t m_currentGeneration = 0;  // Current generation number
    కొలత_t m_చివరి_Id = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* స్థిరరాశి m_tmpScopep = v3Global.rootp()->శిఖరం_Scopep()->scopep();

    // STATIC STATE
    స్థిర V3DebugBisect s_debugBisect;  // Debug aid

#నిర్వచించు APPLYING(id) ఒకవేళ (checkApplying(VDfg_Peephole_నమూనా::id))

    // METHODS
    తార్కిక checkApplying(VDfg_Peephole_నమూనా id) {
        ఒకవేళ (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) తిరిగి అబద్ధం;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_లెక్క[id];
        తిరిగి నిజం;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    Dfg_శీర్షం* cache_శీర్షం(Dfg_శీర్షం* vtxp) {
        Dfg_శీర్షం* స్థిరరాశి equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_is_Cached_శీర్షం || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_is_Cached_శీర్షం = !equivp;
        తిరిగి equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    శూన్య invalidate_శీర్షం(Dfg_శీర్షం* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_is_Cached_శీర్షం = అబద్ధం;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    మూస <రకం_పేరు శీర్షం_2, రకం_పేరు... Operands>
    శీర్షం_2* get_Or_Create_శీర్షం(File_పంక్తి* flp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, Operands... operands) {
        శీర్షం_2* స్థిరరాశి vtxp = m_cache.getOrCreate<శీర్షం_2, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_is_Cached_శీర్షం = నిజం;
        తిరిగి vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    మూస <రకం_పేరు శీర్షం_2, రకం_పేరు... Operands>
    శీర్షం_2* get_శీర్షం(స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, Operands... operands) {
        శీర్షం_2* స్థిరరాశి vtxp = m_cache.get<శీర్షం_2>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_is_Cached_శీర్షం, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        తిరిగి vtxp;
    }

    శూన్య incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    శూన్య కూడు_గమ్యం_Work_List(Dfg_శీర్షం* vtxp) {
        శీర్షం_Info& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        ఒకవేళ (vInfo.m_work_List_సూచీ) తిరిగి;
        // Add to work list
        vInfo.m_work_List_సూచీ = m_workList.కొలత();
        m_workList.నెట్టు_వెనుక(vtxp);
    }

    శూన్య removeFromWorkList(Dfg_శీర్షం* vtxp) {
        శీర్షం_Info& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_work_List_సూచీ] = శూన్యసూచిక;
        vInfo.m_work_List_సూచీ = 0;
    }

    శూన్య కూడు_గమ్యం_Iter_List(Dfg_శీర్షం* vtxp) {
        శీర్షం_Info& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        ఒకవేళ (vInfo.m_iter_List_సూచీ) తిరిగి;
        // Add to iter list
        vInfo.m_iter_List_సూచీ = m_iterList.కొలత();
        m_iterList.నెట్టు_వెనుక(vtxp);
    }

    శూన్య removeFromIterList(Dfg_శీర్షం* vtxp) {
        శీర్షం_Info& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iter_List_సూచీ] = శూన్యసూచిక;
        vInfo.m_iter_List_సూచీ = 0;
    }

    శూన్య delete_శీర్షం(Dfg_శీర్షం* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_work_List_సూచీ, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = vtxp->cast<Dfg_శీర్షం_Var>();
        UASSERT_OBJ(!varp || !varp->hasPrev(), vtxp, "Deleting variable consumed via DfgPrev");

        // Invalidate cache entry
        invalidate_శీర్షం(vtxp);

        // It might be in the iter list, remove it
        removeFromIterList(vtxp);

        // Gather source vertices - they might be duplicates, make unique using generation number
        incrementGeneration();
        std::సదిశ<Dfg_శీర్షం*> srcps;
        srcps.reserve(vtxp->nInputs());
        vtxp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
            // If it's a variable, add to work list to see if it became redundant
            ఒకవేళ (src.is<Dfg_శీర్షం_Var>()) {
                కూడు_గమ్యం_Work_List(&src);
                తిరిగి అబద్ధం;
            }
            // Gather unique sources
            శీర్షం_Info& vInfo = m_vInfo[src];
            ఒకవేళ (vInfo.m_generation != m_currentGeneration) srcps.నెట్టు_వెనుక(&src);
            vInfo.m_generation = m_currentGeneration;
            తిరిగి అబద్ధం;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        ఒకవేళ (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.నెట్టు_వెనుక(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } లేకపోతే {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        స్థిరరాశి స్వయంచాలక mid
            = std::stable_విభజన(srcps.begin(), srcps.చివర(), [](Dfg_శీర్షం* srcp) {  //
                  తిరిగి srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        కొరకు (స్వయంచాలక it = srcps.begin(); it != mid; ++it) కూడు_గమ్యం_Iter_List(*it);

        // Recursively delete unused sources
        కొరకు (స్వయంచాలక it = mid; it != srcps.చివర(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            delete_శీర్షం(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    శూన్య replace(Dfg_శీర్షం* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_work_List_సూచీ, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        స్థిరరాశి స్వయంచాలక debugCallback = [&]() -> శూన్య {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.ఆధారం_Cone({m_vtxp, resp});
            స్థిరరాశి స్వయంచాలక cone = m_dfg.ఆధారం_Cone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](స్థిరరాశి Dfg_శీర్షం& v) {  //
                తిరిగి cone->లెక్క(&v);
            });
        };
        ఒకవేళ (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) తిరిగి;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
            కూడు_గమ్యం_Iter_List(&src);
            తిరిగి అబద్ధం;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](Dfg_శీర్షం& dst) {
            invalidate_శీర్షం(&dst);
            తిరిగి అబద్ధం;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](Dfg_శీర్షం& dst) {
            cache_శీర్షం(&dst);
            తిరిగి అబద్ధం;
        });

        // Original vertex is now unused, so delete it
        delete_శీర్షం(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        కూడు_గమ్యం_Iter_List(resp);
        // Add new vertex and sinks to work list as likely to match
        కూడు_గమ్యం_Work_List(resp);
        resp->foreachSink([&](Dfg_శీర్షం& dst) {
            కూడు_గమ్యం_Work_List(&dst);
            తిరిగి అబద్ధం;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_పంక్తి* flp, uint32_t val) { తిరిగి కొత్త DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_పంక్తి* flp, uint32_t వెడల్పు) {
        తిరిగి కొత్త DfgConst{m_dfg, flp, వెడల్పు, 0};
    }
    DfgConst* makeZero(File_పంక్తి* flp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype) {
        తిరిగి makeZero(flp, dtype.కొలత());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_పంక్తి* flp, uint32_t వెడల్పు) {
        DfgConst* స్థిరరాశి resp = makeZero(flp, వెడల్పు);
        resp->అంకె().సమితి_All_Bits1();
        తిరిగి resp;
    }

    // Create a new vertex of the given type
    మూస <రకం_పేరు శీర్షం_2, రకం_పేరు... Operands>
    శీర్షం_2* make(File_పంక్తి* flp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        శీర్షం_2* స్థిరరాశి vtxp = get_Or_Create_శీర్షం<శీర్షం_2, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        ఒకవేళ (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        శీర్షం_Info& vInfo = m_vInfo[vtxp];
        ఒకవేళ (!vInfo.m_id) vInfo.m_id = ++m_చివరి_Id;
        // Add to work list
        కూడు_గమ్యం_Work_List(vtxp);
        // Return new node
        తిరిగి vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    మూస <రకం_పేరు శీర్షం_2, రకం_పేరు... Operands>
    శీర్షం_2* make(స్థిరరాశి Dfg_శీర్షం* examplep, Operands... operands) {
        తిరిగి make<శీర్షం_2>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    Dfg_శీర్షం* replicate(Dfg_శీర్షం* vtxp, Dfg_శీర్షం* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_బిట్టు_DType, vtxp, "Expected 1-bit vertex");
        ఒకవేళ (vtxp->dtype() == m_బిట్టు_DType) తిరిగి bitp;
        తిరిగి make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    స్థిర తార్కిక isSame(స్థిరరాశి Dfg_శీర్షం* ap, స్థిరరాశి Dfg_శీర్షం* bp) {
        ఒకవేళ (ap == bp) తిరిగి నిజం;
        ఒకవేళ (ap->dtype() != bp->dtype()) తిరిగి అబద్ధం;
        స్థిరరాశి DfgConst* స్థిరరాశి aConstp = ap->cast<DfgConst>();
        ఒకవేళ (!aConstp) తిరిగి అబద్ధం;
        స్థిరరాశి DfgConst* స్థిరరాశి bConstp = bp->cast<DfgConst>();
        ఒకవేళ (!bConstp) తిరిగి అబద్ధం;
        తిరిగి aConstp->అంకె().isCaseEq(bConstp->అంకె());
    }

    స్థిర తార్కిక isZero(స్థిరరాశి Dfg_శీర్షం* vtxp) {
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి constp = vtxp->cast<DfgConst>()) తిరిగి constp->isZero();
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక isOnes(స్థిరరాశి Dfg_శీర్షం* vtxp) {
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి constp = vtxp->cast<DfgConst>()) తిరిగి constp->isOnes();
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక isEqOne(స్థిరరాశి Dfg_శీర్షం* vtxp) {
        ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి constp = vtxp->cast<DfgConst>()) తిరిగి constp->అంకె().isEqOne();
        తిరిగి అబద్ధం;
    }

    స్థిర తార్కిక areAdjacent(uint32_t& lsb, స్థిరరాశి DfgSel* lSelp, స్థిరరాశి DfgSel* rSelp) {
        ఒకవేళ (!isSame(lSelp->srcp(), rSelp->srcp())) తిరిగి అబద్ధం;
        ఒకవేళ (lSelp->lsb() + lSelp->వెడల్పు() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            తిరిగి నిజం;
        }
        ఒకవేళ (lSelp->lsb() == rSelp->lsb() + rSelp->వెడల్పు()) {
            lsb = rSelp->lsb();
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    మూస <రకం_పేరు శీర్షం_2>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక foldUnary(శీర్షం_2* స్థిరరాశి vtxp) {
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం_Unary, శీర్షం_2>::విలువ, "Must invoke on unary");
        static_assert(std::is_final<శీర్షం_2>::విలువ, "Must invoke on final class");
        ఒకవేళ (DfgConst* స్థిరరాశి srcp = vtxp->srcp()->మూస cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* స్థిరరాశి resultp = makeZero(vtxp->fileline(), vtxp->వెడల్పు());
                foldOp<శీర్షం_2>(resultp->అంకె(), srcp->అంకె());
                replace(resultp);
                తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    మూస <రకం_పేరు శీర్షం_2>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక ద్విమాన(శీర్షం_2* స్థిరరాశి vtxp) {
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం_ద్విమాన, శీర్షం_2>::విలువ, "Must invoke on binary");
        static_assert(std::is_final<శీర్షం_2>::విలువ, "Must invoke on final class");

        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
        // LHS/RHS of the vertex
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* స్థిరరాశి lConstp = lhsp->cast<DfgConst>();
        DfgConst* స్థిరరాశి rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        శీర్షం_2* స్థిరరాశి lSamep = lhsp->cast<శీర్షం_2>();
        శీర్షం_2* స్థిరరాశి rSamep = rhsp->cast<శీర్షం_2>();

        // Constant folding
        ఒకవేళ (lConstp && rConstp) {
            APPLYING(FOLD_ద్విమాన) {
                DfgConst* స్థిరరాశి resultp = makeZero(flp, ఫలితం_DType<శీర్షం_2>(lhsp, rhsp));
                foldOp<శీర్షం_2>(resultp->అంకె(), lConstp->అంకె(), rConstp->అంకె());
                replace(resultp);
                తిరిగి నిజం;
            }
        }

        // Idempotent
        ఒకవేళ VL_CONSTEXPR_CXX17 (IsIdempotent<శీర్షం_2>::విలువ) {
            ఒకవేళ (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_ద్విమాన) {
                    replace(lhsp);
                    తిరిగి నిజం;
                }
            }
        }

        // Associative
        ఒకవేళ VL_CONSTEXPR_CXX17 (IsAssociative<శీర్షం_2>::విలువ) {
            // Also commutative and idempotent
            ఒకవేళ VL_CONSTEXPR_CXX17 (IsCommutative<శీర్షం_2>::విలువ && IsIdempotent<శీర్షం_2>::విలువ) {
                ఒకవేళ (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_ద్విమాన_LHS) {
                        replace(lSamep);
                        తిరిగి నిజం;
                    }
                }
                ఒకవేళ (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_ద్విమాన_RHS) {
                        replace(rSamep);
                        తిరిగి నిజం;
                    }
                }
            }

            ఒకవేళ (lConstp && rSamep) {
                ఒకవేళ (DfgConst* స్థిరరాశి rlConstp = rSamep->lhsp()->మూస cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ద్విమాన_LHS_యొక్క_RHS) {
                        DfgConst* స్థిరరాశి cp = makeZero(flp, ఫలితం_DType<శీర్షం_2>(lConstp, rlConstp));
                        foldOp<శీర్షం_2>(cp->అంకె(), lConstp->అంకె(), rlConstp->అంకె());
                        replace(make<శీర్షం_2>(vtxp, cp, rSamep->rhsp()));
                        తిరిగి నిజం;
                    }
                }
            }

            ఒకవేళ (lSamep && rConstp) {
                ఒకవేళ (DfgConst* స్థిరరాశి lrConstp = lSamep->rhsp()->మూస cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_ద్విమాన_RHS_యొక్క_LHS) {
                        DfgConst* స్థిరరాశి cp = makeZero(flp, ఫలితం_DType<శీర్షం_2>(lrConstp, rConstp));
                        foldOp<శీర్షం_2>(cp->అంకె(), lrConstp->అంకె(), rConstp->అంకె());
                        replace(make<శీర్షం_2>(vtxp, lSamep->lhsp(), cp));
                        తిరిగి నిజం;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            ఒకవేళ (lSamep && !lSamep->has_గుణిజం_Sinks()) {
                Dfg_శీర్షం* స్థిరరాశి ap = lSamep->lhsp();
                Dfg_శీర్షం* స్థిరరాశి bp = lSamep->rhsp();
                Dfg_శీర్షం* స్థిరరాశి cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                ఒకవేళ (ap != cp && bp != cp) {
                    APPLYING(కుడి_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        స్థిరరాశి Dfg_దత్తాంశం_Type& బిడ్డ_DType = ఫలితం_DType<శీర్షం_2>(bp, cp);
                        శీర్షం_2* స్థిరరాశి bcp = make<శీర్షం_2>(vtxp->fileline(), బిడ్డ_DType, bp, cp);
                        replace(make<శీర్షం_2>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        తిరిగి నిజం;
                    }
                }
            }

            ఒకవేళ (rSamep && !rSamep->has_గుణిజం_Sinks()) {
                Dfg_శీర్షం* స్థిరరాశి rlVtxp = rSamep->lhsp();
                Dfg_శీర్షం* స్థిరరాశి rrVtxp = rSamep->rhsp();

                ఒకవేళ VL_CONSTEXPR_CXX17 (IsCommutative<శీర్షం_2>::విలువ) {
                    ఒకవేళ (!lhsp->has_గుణిజం_Sinks() && rlVtxp->has_గుణిజం_Sinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<శీర్షం_2>(vtxp, rlVtxp, make<శీర్షం_2>(vtxp, lhsp, rrVtxp)));
                            తిరిగి నిజం;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                ఒకవేళ (శీర్షం_2* స్థిరరాశి existingp
                    = get_శీర్షం<శీర్షం_2>(ఫలితం_DType<శీర్షం_2>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    ఒకవేళ (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_ద్విమాన_LHS_WITH_LHS_యొక్క_RHS) {
                            replace(make<శీర్షం_2>(vtxp, existingp, rrVtxp));
                            తిరిగి నిజం;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                ఒకవేళ VL_CONSTEXPR_CXX17 (IsCommutative<శీర్షం_2>::విలువ) {
                    ఒకవేళ (శీర్షం_2* స్థిరరాశి existingp
                        = get_శీర్షం<శీర్షం_2>(ఫలితం_DType<శీర్షం_2>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        ఒకవేళ (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_ద్విమాన_LHS_WITH_RHS_యొక్క_RHS) {
                                replace(make<శీర్షం_2>(vtxp, existingp, rlVtxp));
                                తిరిగి నిజం;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        ఒకవేళ VL_CONSTEXPR_CXX17 (IsCommutative<శీర్షం_2>::విలువ) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            ఒకవేళ (lConstp) {
                ఒకవేళ (DfgCond* స్థిరరాశి rCondp = rhsp->cast<DfgCond>()) {
                    ఒకవేళ (!rCondp->has_గుణిజం_Sinks()) {
                        APPLYING(నెట్టు_COMMUTATIVE_ద్విమాన_THROUGH_COND) {
                            Dfg_శీర్షం* స్థిరరాశి tp = make<శీర్షం_2>(vtxp, lConstp, rCondp->thenp());
                            Dfg_శీర్షం* స్థిరరాశి ep = make<శీర్షం_2>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            తిరిగి నిజం;
                        }
                    }
                }
                తిరిగి అబద్ధం;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        స్థిరరాశి తార్కిక మార్పిడి_Sides = [&]() {
            // Don't swap if not commutative, duh
            ఒకవేళ VL_CONSTEXPR_CXX17 (!IsCommutative<శీర్షం_2>::విలువ) తిరిగి అబద్ధం;
            // Const go on left
            ఒకవేళ (lConstp) తిరిగి అబద్ధం;
            ఒకవేళ (rConstp) తిరిగి నిజం;
            // Not go on left
            ఒకవేళ (lhsp->is<DfgNot>()) తిరిగి అబద్ధం;
            ఒకవేళ (rhsp->is<DfgNot>()) తిరిగి నిజం;
            // Same go on right
            ఒకవేళ (rSamep) తిరిగి అబద్ధం;
            ఒకవేళ (lSamep) తిరిగి నిజం;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            స్థిరరాశి శీర్షం_Info& lInfo = m_vInfo[lhsp];
            స్థిరరాశి శీర్షం_Info& rInfo = m_vInfo[rhsp];
            తిరిగి lInfo.m_id > rInfo.m_id;
        }();
        ఒకవేళ (మార్పిడి_Sides) {
            APPLYING(మార్పిడి_SIDES_లోపల_ద్విమాన) {
                replace(make<శీర్షం_2>(vtxp, rhsp, lhsp));
                తిరిగి నిజం;
            }
        }

        తిరిగి అబద్ధం;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    మూస <రకం_పేరు Other, రకం_పేరు శీర్షం_2>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక distributive_And_Associative_ద్విమాన(శీర్షం_2* vtxp) {
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        ఒకవేళ (!lhsp->has_గుణిజం_Sinks() && !rhsp->has_గుణిజం_Sinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            ఒకవేళ (Other* స్థిరరాశి lp = lhsp->cast<Other>()) {
                ఒకవేళ (Other* స్థిరరాశి rp = rhsp->cast<Other>()) {
                    Dfg_శీర్షం* స్థిరరాశి llp = lp->lhsp();
                    Dfg_శీర్షం* స్థిరరాశి lrp = lp->rhsp();
                    Dfg_శీర్షం* స్థిరరాశి rlp = rp->lhsp();
                    Dfg_శీర్షం* స్థిరరాశి rrp = rp->rhsp();
                    Dfg_శీర్షం* ap = శూన్యసూచిక;
                    Dfg_శీర్షం* bp = శూన్యసూచిక;
                    Dfg_శీర్షం* cp = శూన్యసూచిక;
                    ఒకవేళ (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } లేకపోతే ఒకవేళ (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } లేకపోతే ఒకవేళ (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } లేకపోతే ఒకవేళ (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    ఒకవేళ (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_ద్విమాన) {
                            replace(make<Other>(vtxp, ap, make<శీర్షం_2>(lhsp, bp, cp)));
                            తిరిగి నిజం;
                        }
                    }
                }
            }
        }

        తిరిగి అబద్ధం;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    మూస <రకం_పేరు శీర్షం_2>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక
    try_నెట్టు_Bitwise_Op_Through_Concat(శీర్షం_2* స్థిరరాశి vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* స్థిరరాశి catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* స్థిరరాశి catRConstp = concatp->rhsp()->cast<DfgConst>();
        ఒకవేళ (catLConstp || catRConstp) {
            APPLYING(నెట్టు_BITWISE_OP_THROUGH_CONCAT) {
                స్థిరరాశి uint32_t వెడల్పు = concatp->వెడల్పు();
                స్థిరరాశి Dfg_దత్తాంశం_Type& lDtype = concatp->lhsp()->dtype();
                స్థిరరాశి Dfg_దత్తాంశం_Type& rDtype = concatp->rhsp()->dtype();
                స్థిరరాశి uint32_t l_వెడల్పు = lDtype.కొలత();
                స్థిరరాశి uint32_t r_వెడల్పు = rDtype.కొలత();

                // The new Lhs vertex
                Dfg_శీర్షం* స్థిరరాశి newLhsp = [&]() -> Dfg_శీర్షం* {
                    DfgConst* స్థిరరాశి newLhsConstp = makeZero(constp->fileline(), l_వెడల్పు);
                    ఒకవేళ (catLConstp) {
                        V3_సంఖ్య అంకె{constp->fileline(), static_cast<పూర్ణాంకం>(l_వెడల్పు), 0u};
                        అంకె.opSel(constp->అంకె(), వెడల్పు - 1, r_వెడల్పు);
                        foldOp<శీర్షం_2>(newLhsConstp->అంకె(), అంకె, catLConstp->అంకె());
                        తిరిగి newLhsConstp;
                    }
                    newLhsConstp->అంకె().opSel(constp->అంకె(), వెడల్పు - 1, r_వెడల్పు);
                    తిరిగి make<శీర్షం_2>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                Dfg_శీర్షం* స్థిరరాశి newRhsp = [&]() -> Dfg_శీర్షం* {
                    DfgConst* స్థిరరాశి newRhsConstp = makeZero(constp->fileline(), r_వెడల్పు);
                    ఒకవేళ (catRConstp) {
                        V3_సంఖ్య అంకె{constp->fileline(), static_cast<పూర్ణాంకం>(r_వెడల్పు), 0u};
                        అంకె.opSel(constp->అంకె(), r_వెడల్పు - 1, 0);
                        foldOp<శీర్షం_2>(newRhsConstp->అంకె(), అంకె, catRConstp->అంకె());
                        తిరిగి newRhsConstp;
                    }
                    newRhsConstp->అంకె().opSel(constp->అంకె(), r_వెడల్పు - 1, 0);
                    తిరిగి make<శీర్షం_2>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }

    మూస <రకం_పేరు శీర్షం_2>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక
    try_నెట్టు_Compare_Op_Through_Concat(శీర్షం_2* స్థిరరాశి vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        ఒకవేళ (!concatp->has_గుణిజం_Sinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(నెట్టు_COMPARE_OP_THROUGH_CONCAT) {
                స్థిరరాశి uint32_t వెడల్పు = concatp->వెడల్పు();
                స్థిరరాశి uint32_t l_వెడల్పు = concatp->lhsp()->వెడల్పు();
                స్థిరరాశి uint32_t r_వెడల్పు = concatp->rhsp()->వెడల్పు();

                // The new Lhs vertex
                DfgConst* స్థిరరాశి newLhsConstp = makeZero(constp->fileline(), l_వెడల్పు);
                newLhsConstp->అంకె().opSel(constp->అంకె(), వెడల్పు - 1, r_వెడల్పు);
                శీర్షం_2* స్థిరరాశి newLhsp
                    = make<శీర్షం_2>(flp, m_బిట్టు_DType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* స్థిరరాశి newRhsConstp = makeZero(constp->fileline(), r_వెడల్పు);
                newRhsConstp->అంకె().opSel(constp->అంకె(), r_వెడల్పు - 1, 0);
                శీర్షం_2* స్థిరరాశి newRhsp
                    = make<శీర్షం_2>(flp, m_బిట్టు_DType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                ఒకవేళ VL_CONSTEXPR_CXX17 (std::is_same<శీర్షం_2, DfgEq>::విలువ) {
                    replace(make<DfgAnd>(concatp->fileline(), m_బిట్టు_DType, newLhsp, newRhsp));
                } లేకపోతే ఒకవేళ VL_CONSTEXPR_CXX17 (std::is_same<శీర్షం_2, DfgNeq>::విలువ) {
                    replace(make<DfgOr>(concatp->fileline(), m_బిట్టు_DType, newLhsp, newRhsp));
                } లేకపోతే {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }

    మూస <రకం_పేరు Bitwise>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక try_నెట్టు_Bitwise_Op_Through_Reductions(Bitwise* స్థిరరాశి vtxp) {
        ఉపయోగించి Reduction = Bitwise_గమ్యం_Reduction<Bitwise>;

        ఒకవేళ (Reduction* స్థిరరాశి lRedp = vtxp->lhsp()->మూస cast<Reduction>()) {
            ఒకవేళ (Reduction* స్థిరరాశి rRedp = vtxp->rhsp()->మూస cast<Reduction>()) {
                Dfg_శీర్షం* స్థిరరాశి lSrcp = lRedp->srcp();
                Dfg_శీర్షం* స్థిరరాశి rSrcp = rRedp->srcp();
                ఒకవేళ (lSrcp->dtype() == rSrcp->dtype() && lSrcp->వెడల్పు() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->has_గుణిజం_Sinks() && !rSrcp->has_గుణిజం_Sinks()) {
                    APPLYING(నెట్టు_BITWISE_THROUGH_REDUCTION) {
                        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
                        Bitwise* స్థిరరాశి bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_బిట్టు_DType, bwp));
                        తిరిగి నిజం;
                    }
                }
            }
        }

        తిరిగి అబద్ధం;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    స్థిర std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(Dfg_శీర్షం* vtxp) {
        ఒకవేళ (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            తిరిగి {శూన్యసూచిక, శూన్యసూచిక};
        }
        ఒకవేళ (vtxp->has_గుణిజం_Sinks()) తిరిగి {శూన్యసూచిక, శూన్యసూచిక};
        Dfg_శీర్షం_ద్విమాన* స్థిరరాశి bwp = vtxp->as<Dfg_శీర్షం_ద్విమాన>();
        DfgSel* స్థిరరాశి lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* స్థిరరాశి rSelp = bwp->rhsp()->cast<DfgSel>();
        ఒకవేళ (!lSelp || !rSelp) తిరిగి {శూన్యసూచిక, శూన్యసూచిక};
        DfgSel* స్థిరరాశి nSelp = lSelp->fromp()->వెడల్పు() <= rSelp->fromp()->వెడల్పు() ? lSelp : rSelp;
        DfgSel* స్థిరరాశి wSelp = nSelp == lSelp ? rSelp : lSelp;
        స్థిరరాశి uint32_t n_వెడల్పు = nSelp->fromp()->వెడల్పు();
        ఒకవేళ (n_వెడల్పు > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) తిరిగి {శూన్యసూచిక, శూన్యసూచిక};
        ఒకవేళ (wSelp->lsb() - nSelp->lsb() + n_వెడల్పు > wSelp->fromp()->వెడల్పు()) {
            తిరిగి {శూన్యసూచిక, శూన్యసూచిక};
        }
        తిరిగి {nSelp, wSelp};
    }

    మూస <రకం_పేరు Bitwise>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక try_నెట్టు_Bitwise_Op_Throug_Sel(Bitwise* స్థిరరాశి vtxp) {
        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();

        ఒకవేళ (DfgSel* స్థిరరాశి lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = శూన్యసూచిక;
            Dfg_శీర్షం* extrap = శూన్యసూచిక;
            ఒకవేళ (DfgSel* స్థిరరాశి selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } లేకపోతే ఒకవేళ (Bitwise* స్థిరరాశి bitwisep = rhsp->cast<Bitwise>()) {
                ఒకవేళ (DfgSel* స్థిరరాశి rlSelp = bitwisep->lhsp()->మూస cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } లేకపోతే ఒకవేళ (DfgSel* స్థిరరాశి rrSelp = bitwisep->rhsp()->మూస cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            ఒకవేళ (rSelp) {
                Dfg_శీర్షం* స్థిరరాశి lFromp = lSelp->fromp();
                Dfg_శీర్షం* స్థిరరాశి rFromp = rSelp->fromp();
                ఒకవేళ (lFromp->dtype() == rFromp->dtype() && lFromp->వెడల్పు() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(నెట్టు_BITWISE_THROUGH_SEL) {
                        Bitwise* స్థిరరాశి bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        Dfg_శీర్షం* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        ఒకవేళ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        తిరిగి నిజం;
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
            Bitwise* స్థిరరాశి rChainp = rhsp->cast<Bitwise>();
            ఒకవేళ (rChainp && rChainp->has_గుణిజం_Sinks()) తిరిగి అబద్ధం;
            Dfg_శీర్షం* స్థిరరాశి rTermp = rChainp ? rChainp->lhsp() : rhsp;

            స్థిరరాశి స్వయంచాలక lSels = pushableUnalignedSels(lhsp);
            స్థిరరాశి స్వయంచాలక rSels = pushableUnalignedSels(rTermp);
            ఒకవేళ (!lSels.మొదటి && !rSels.మొదటి) తిరిగి అబద్ధం;

            స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి opp = lSels.మొదటి ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            ఒకవేళ (opp->is<Bitwise>()) తిరిగి అబద్ధం;
            స్థిరరాశి స్వయంచాలక& sels = lSels.మొదటి ? lSels : rSels;
            Dfg_శీర్షం* స్థిరరాశి nFromp = sels.మొదటి->fromp();
            Dfg_శీర్షం* స్థిరరాశి wFromp = sels.రెండవ->fromp();
            స్థిరరాశి uint32_t స్థానభ్రంశం = sels.రెండవ->lsb() - sels.మొదటి->lsb();

            // Both terms must end up as selects from the same pushed operation
            స్థిరరాశి స్వయంచాలక matches = [&](స్థిరరాశి Dfg_శీర్షం* termp, స్థిరరాశి స్వయంచాలక& tSels) {
                // Original form: 'n[i] OP w[j]'
                ఒకవేళ (tSels.మొదటి) {
                    తిరిగి termp->type() == opp->type()  //
                           && tSels.మొదటి->fromp() == nFromp  //
                           && tSels.రెండవ->fromp() == wFromp  //
                           && tSels.రెండవ->lsb() - tSels.మొదటి->lsb() == స్థానభ్రంశం;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                ఒకవేళ (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                ఒకవేళ (స్థిరరాశి DfgSel* స్థిరరాశి selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                ఒకవేళ (termp->type() != opp->type()) తిరిగి అబద్ధం;
                // Operands might have been swapped since
                స్థిరరాశి Dfg_శీర్షం_ద్విమాన* స్థిరరాశి bwp = termp->as<Dfg_శీర్షం_ద్విమాన>();
                స్థిరరాశి తార్కిక nIsLhs = bwp->lhsp() == nFromp;
                ఒకవేళ (!nIsLhs && bwp->rhsp() != nFromp) తిరిగి అబద్ధం;
                స్థిరరాశి DfgSel* స్థిరరాశి slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                తిరిగి slicep && slicep->fromp() == wFromp && slicep->lsb() == స్థానభ్రంశం;
            };
            ఒకవేళ (!matches(lhsp, lSels) || !matches(rTermp, rSels)) తిరిగి అబద్ధం;

            APPLYING(నెట్టు_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
                స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = nFromp->dtype();
                DfgSel* స్థిరరాశి slicep = make<DfgSel>(flp, dtype, wFromp, స్థానభ్రంశం);
                Dfg_శీర్షం* bwp = శూన్యసూచిక;
                ఒకవేళ (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } లేకపోతే ఒకవేళ (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } లేకపోతే {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                స్థిరరాశి స్వయంచాలక నెట్టు = [&](Dfg_శీర్షం* termp, స్థిరరాశి DfgSel* nSelp) -> Dfg_శీర్షం* {
                    తిరిగి nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                Dfg_శీర్షం* స్థిరరాశి newLhsp = నెట్టు(lhsp, lSels.మొదటి);
                Dfg_శీర్షం* స్థిరరాశి newRTermp = నెట్టు(rTermp, rSels.మొదటి);
                ఒకవేళ (rChainp) {
                    Dfg_శీర్షం* స్థిరరాశి newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } లేకపోతే {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                తిరిగి నిజం;
            }
        }

        తిరిగి అబద్ధం;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    స్థిర తార్కిక isConcatFoldable(స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) {
        // Concatenation of the same vertex is a replicate
        ఒకవేళ (isSame(lhsp, rhsp)) తిరిగి నిజం;
        ఒకవేళ (స్థిరరాశి DfgRep* స్థిరరాశి lRepp = lhsp->cast<DfgRep>()) {
            ఒకవేళ (isSame(lRepp->srcp(), rhsp)) తిరిగి నిజం;
        }
        ఒకవేళ (స్థిరరాశి DfgRep* స్థిరరాశి rRepp = rhsp->cast<DfgRep>()) {
            ఒకవేళ (isSame(lhsp, rRepp->srcp())) తిరిగి నిజం;
        }
        // Concatenation of adjoining selects is a select
        ఒకవేళ (స్థిరరాశి DfgSel* స్థిరరాశి lSelp = lhsp->cast<DfgSel>()) {
            ఒకవేళ (స్థిరరాశి DfgSel* స్థిరరాశి rSelp = rhsp->cast<DfgSel>()) {
                తిరిగి isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->వెడల్పు();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        ఒకవేళ (స్థిరరాశి DfgNot* స్థిరరాశి lNotp = lhsp->cast<DfgNot>()) {
            ఒకవేళ (స్థిరరాశి DfgNot* స్థిరరాశి rNotp = rhsp->cast<DfgNot>()) {
                తిరిగి !lNotp->has_గుణిజం_Sinks()  //
                       && !rNotp->has_గుణిజం_Sinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        తిరిగి అబద్ధం;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    స్థిర తార్కిక isSameBitwise(స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) {
        ఒకవేళ (lhsp->type() != rhsp->type()) తిరిగి అబద్ధం;
        తిరిగి lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    స్థిర తార్కిక isConcatSimplifiable(స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp) {
        ఒకవేళ (isConcatFoldable(lhsp, rhsp)) తిరిగి నిజం;
        ఒకవేళ (!isSameBitwise(lhsp, rhsp)) తిరిగి అబద్ధం;
        ఒకవేళ (lhsp->has_గుణిజం_Sinks() || rhsp->has_గుణిజం_Sinks()) తిరిగి అబద్ధం;
        స్థిరరాశి Dfg_శీర్షం_ద్విమాన* స్థిరరాశి lp = lhsp->as<Dfg_శీర్షం_ద్విమాన>();
        స్థిరరాశి Dfg_శీర్షం_ద్విమాన* స్థిరరాశి rp = rhsp->as<Dfg_శీర్షం_ద్విమాన>();
        ఒకవేళ (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            తిరిగి నిజం;
        ఒకవేళ (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            తిరిగి నిజం;
        తిరిగి అబద్ధం;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    స్థిర తార్కిక isConcatPushableThroughBitwise(స్థిరరాశి Dfg_శీర్షం* lhsp, స్థిరరాశి Dfg_శీర్షం* rhsp,
                                               తార్కిక& మార్పిడి) {
        ఒకవేళ (!isSameBitwise(lhsp, rhsp)) తిరిగి అబద్ధం;
        ఒకవేళ (lhsp->has_గుణిజం_Sinks() || rhsp->has_గుణిజం_Sinks()) తిరిగి అబద్ధం;
        స్థిరరాశి Dfg_శీర్షం_ద్విమాన* స్థిరరాశి lp = lhsp->as<Dfg_శీర్షం_ద్విమాన>();
        స్థిరరాశి Dfg_శీర్షం_ద్విమాన* స్థిరరాశి rp = rhsp->as<Dfg_శీర్షం_ద్విమాన>();
        మార్పిడి = అబద్ధం;
        ఒకవేళ (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) తిరిగి నిజం;
        ఒకవేళ (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) తిరిగి నిజం;
        మార్పిడి = నిజం;
        ఒకవేళ (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) తిరిగి నిజం;
        ఒకవేళ (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) తిరిగి నిజం;
        తిరిగి అబద్ధం;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    Dfg_శీర్షం* makeConcatPushedThroughBitwise(File_పంక్తి* flp, స్థిరరాశి Dfg_శీర్షం* lhsp,
                                              స్థిరరాశి Dfg_శీర్షం* rhsp, తార్కిక మార్పిడి) {
        స్థిరరాశి Dfg_శీర్షం_ద్విమాన* స్థిరరాశి lp = lhsp->as<Dfg_శీర్షం_ద్విమాన>();
        స్థిరరాశి Dfg_శీర్షం_ద్విమాన* స్థిరరాశి rp = rhsp->as<Dfg_శీర్షం_ద్విమాన>();
        స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = Dfg_దత్తాంశం_Type::packed(lp->వెడల్పు() + rp->వెడల్పు());
        Dfg_శీర్షం* స్థిరరాశి cp = మార్పిడి ? rp->rhsp() : rp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి dp = మార్పిడి ? rp->lhsp() : rp->rhsp();
        DfgConcat* స్థిరరాశి newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* స్థిరరాశి newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        ఒకవేళ (lp->is<DfgAnd>()) తిరిగి make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        ఒకవేళ (lp->is<DfgOr>()) తిరిగి make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        తిరిగి make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    మూస <రకం_పేరు Bitwise>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->వెడల్పు() == 1, vtxp, "Width must be 1");
        ఉపయోగించి Reduction = Bitwise_గమ్యం_Reduction<Bitwise>;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();

        ఒకవేళ (DfgSel* స్థిరరాశి lSelp = lhsp->మూస cast<DfgSel>()) {
            DfgSel* rSelp = శూన్యసూచిక;
            Dfg_శీర్షం* extrap = శూన్యసూచిక;
            ఒకవేళ (DfgSel* స్థిరరాశి selp = rhsp->మూస cast<DfgSel>()) {
                rSelp = selp;
            } లేకపోతే ఒకవేళ (Bitwise* స్థిరరాశి rBitwisep = rhsp->మూస cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->మూస cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } లేకపోతే ఒకవేళ (Reduction* స్థిరరాశి rRedp = rhsp->మూస cast<Reduction>()) {
                rSelp = rRedp->srcp()->మూస cast<DfgSel>();
            }
            ఒకవేళ (rSelp) {
                uint32_t lsb = 0;
                ఒకవేళ (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_యొక్క_SELS_WITH_REDUCTION) {
                        స్థిరరాశి Dfg_దత్తాంశం_Type& dtype
                            = Dfg_దత్తాంశం_Type::packed(lSelp->వెడల్పు() + rSelp->వెడల్పు());
                        DfgSel* స్థిరరాశి newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        Dfg_శీర్షం* resp = make<Reduction>(vtxp, newSelp);
                        ఒకవేళ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        తిరిగి నిజం;
                    }
                }
            }
        }

        ఒకవేళ (Reduction* స్థిరరాశి lRedp = lhsp->మూస cast<Reduction>()) {
            Reduction* rRedp = rhsp->మూస cast<Reduction>();
            Dfg_శీర్షం* extrap = శూన్యసూచిక;
            ఒకవేళ (!rRedp) {
                ఒకవేళ (Bitwise* స్థిరరాశి rBitwisep = rhsp->మూస cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->మూస cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            ఒకవేళ (rRedp) {
                ఒకవేళ (DfgSel* స్థిరరాశి lSelp = lRedp->srcp()->మూస cast<DfgSel>()) {
                    ఒకవేళ (DfgSel* స్థిరరాశి rSelp = rRedp->srcp()->మూస cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ఒకవేళ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_యొక్క_REDUCTION_యొక్క_SELS_WITH_REDUCTION) {
                                స్థిరరాశి Dfg_దత్తాంశం_Type& dtype
                                    = Dfg_దత్తాంశం_Type::packed(lSelp->వెడల్పు() + rSelp->వెడల్పు());
                                DfgSel* స్థిరరాశి newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                Dfg_శీర్షం* resp = make<Reduction>(vtxp, newSelp);
                                ఒకవేళ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                తిరిగి నిజం;
                            }
                        }
                    }
                }
            }
        }

        తిరిగి అబద్ధం;
    }

    మూస <రకం_పేరు Reduction>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక optimizeReduction(Reduction* స్థిరరాశి vtxp) {
        ఉపయోగించి Bitwise = Reduction_గమ్యం_Bitwise<Reduction>;

        ఒకవేళ (foldUnary(vtxp)) తిరిగి నిజం;

        Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        // Reduction of 1-bit value
        ఒకవేళ (srcp->dtype() == m_బిట్టు_DType) {
            APPLYING(REMOVE_వెడల్పు_ONE_REDUCTION) {
                replace(srcp);
                తిరిగి నిజం;
            }
        }

        ఒకవేళ (DfgCond* స్థిరరాశి condp = srcp->cast<DfgCond>()) {
            ఒకవేళ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(నెట్టు_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* స్థిరరాశి newThenp = make<Reduction>(flp, m_బిట్టు_DType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* స్థిరరాశి newElsep = make<Reduction>(flp, m_బిట్టు_DType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* స్థిరరాశి newCondp = make<DfgCond>(condp->fileline(), m_బిట్టు_DType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    తిరిగి నిజం;
                }
            }
        }

        ఒకవేళ (DfgConcat* స్థిరరాశి concatp = srcp->cast<DfgConcat>()) {
            ఒకవేళ (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_బిట్టు_DType
                || concatp->rhsp()->dtype() == m_బిట్టు_DType) {
                APPLYING(నెట్టు_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* స్థిరరాశి lRedp
                        = make<Reduction>(concatp->fileline(), m_బిట్టు_DType, concatp->lhsp());
                    Reduction* స్థిరరాశి rRedp
                        = make<Reduction>(concatp->fileline(), m_బిట్టు_DType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_బిట్టు_DType, lRedp, rRedp));
                    తిరిగి నిజం;
                }
            }
        }

        ఒకవేళ (Bitwise* స్థిరరాశి bitwisep = vtxp->srcp()->మూస cast<Bitwise>()) {
            ఒకవేళ (!bitwisep->has_గుణిజం_Sinks()) {
                ఒకవేళ (bitwisep->lhsp()->మూస is<DfgConcat>()
                    || bitwisep->rhsp()->మూస is<DfgConcat>()) {
                    APPLYING(నెట్టు_REDUCTION_THROUGH_BITWISE_యొక్క_CONCAT) {
                        Reduction* స్థిరరాశి newLhsp
                            = make<Reduction>(flp, m_బిట్టు_DType, bitwisep->lhsp());
                        Reduction* స్థిరరాశి newRhsp
                            = make<Reduction>(flp, m_బిట్టు_DType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_బిట్టు_DType, newLhsp, newRhsp));
                        తిరిగి నిజం;
                    }
                }

                ఒకవేళ (DfgSel* స్థిరరాశి lSelp = bitwisep->lhsp()->మూస cast<DfgSel>()) {
                    ఒకవేళ (DfgSel* స్థిరరాశి rSelp = bitwisep->rhsp()->మూస cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ఒకవేళ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(నెట్టు_REDUCTION_THROUGH_BITWISE_యొక్క_SELS) {
                                స్థిరరాశి Dfg_దత్తాంశం_Type& dtype
                                    = Dfg_దత్తాంశం_Type::packed(lSelp->వెడల్పు() + rSelp->వెడల్పు());
                                DfgSel* స్థిరరాశి newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                తిరిగి నిజం;
                            }
                        }
                    }
                }
            }
        }

        తిరిగి అబద్ధం;
    }

    మూస <రకం_పేరు జరుపు>
    VL_ATTR_WARN_వాడని_ఫలితం తార్కిక optimize_జరుపు_RHS(జరుపు* స్థిరరాశి vtxp) {
        static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం_ద్విమాన, జరుపు>::విలువ, "Must invoke on binary");
        static_assert(std::is_final<జరుపు>::విలువ, "Must invoke on final class");
        ఒకవేళ (స్థిరరాశి DfgConcat* స్థిరరాశి concatp = vtxp->rhsp()->మూస cast<DfgConcat>()) {
            ఒకవేళ (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_యొక్క_జరుపు) {
                    replace(make<జరుపు>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    తిరిగి నిజం;
                }
            }
        }
        తిరిగి అబద్ధం;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<Dfg_శీర్షం*, uint32_t, uint32_t> కూడు_గమ్యం_లెక్క_Ones_పదం(Dfg_శీర్షం* vtxp) {
        ఒకవేళ (DfgConcat* స్థిరరాశి oCatp = vtxp->cast<DfgConcat>()) {
            ఒకవేళ (isZero(oCatp->lhsp())) {
                ఒకవేళ (Dfg_లెక్క_Ones* స్థిరరాశి లెక్క_Onesp = oCatp->rhsp()->cast<Dfg_లెక్క_Ones>()) {
                    // Zero extended count ones
                    ఒకవేళ (DfgSel* స్థిరరాశి selp = లెక్క_Onesp->srcp()->cast<DfgSel>()) {
                        తిరిగి {selp->fromp(), selp->lsb(), selp->వెడల్పు()};
                    }
                } లేకపోతే ఒకవేళ (DfgSel* స్థిరరాశి selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    ఒకవేళ (selp->dtype() == m_బిట్టు_DType) {  //
                        తిరిగి {selp->fromp(), selp->lsb(), selp->వెడల్పు()};
                    }
                }
            }
            తిరిగి {శూన్యసూచిక, 0, 0};
        }
        ఒకవేళ (Dfg_లెక్క_Ones* స్థిరరాశి లెక్క_Onesp = vtxp->cast<Dfg_లెక్క_Ones>()) {
            // Simple count ones
            ఒకవేళ (DfgSel* స్థిరరాశి selp = లెక్క_Onesp->srcp()->cast<DfgSel>()) {
                తిరిగి {selp->fromp(), selp->lsb(), selp->వెడల్పు()};
            }
            తిరిగి {శూన్యసూచిక, 0, 0};
        }
        ఒకవేళ (DfgSel* స్థిరరాశి oSelp = vtxp->cast<DfgSel>()) {
            ఒకవేళ (oSelp->lsb() == 0) {
                // Truncated count ones
                ఒకవేళ (Dfg_లెక్క_Ones* స్థిరరాశి లెక్క_Onesp = oSelp->fromp()->cast<Dfg_లెక్క_Ones>()) {
                    // Zero extended count ones
                    ఒకవేళ (DfgSel* స్థిరరాశి selp = లెక్క_Onesp->srcp()->cast<DfgSel>()) {
                        తిరిగి {selp->fromp(), selp->lsb(), selp->వెడల్పు()};
                    }
                }
            }
            // Single bit select
            ఒకవేళ (oSelp->dtype() == m_బిట్టు_DType) {  //
                తిరిగి {oSelp->fromp(), oSelp->lsb(), 1};
            }
            తిరిగి {శూన్యసూచిక, 0, 0};
        }
        // Altered form of extended MSB
        ఒకవేళ (Dfg_జరుపు_R* స్థిరరాశి shiftrp = vtxp->cast<Dfg_జరుపు_R>()) {
            ఒకవేళ (DfgConst* స్థిరరాశి rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                ఒకవేళ (rConstp->గమ్యం_U32() == shiftrp->వెడల్పు() - 1) {
                    తిరిగి {shiftrp->lhsp(), shiftrp->వెడల్పు() - 1, 1};
                }
            }
            తిరిగి {శూన్యసూచిక, 0, 0};
        }
        // Not applicable
        తిరిగి {శూన్యసూచిక, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<Dfg_శీర్షం*, uint32_t> unwindSel(Dfg_శీర్షం* fromp, uint32_t lsb,
                                              స్థిరరాశి uint32_t వెడల్పు) {
        అయితే (నిజం) {
            స్థిరరాశి uint32_t msb = lsb + వెడల్పు - 1;

            // Sel from Concat
            ఒకవేళ (DfgConcat* స్థిరరాశి concatp = fromp->cast<DfgConcat>()) {
                Dfg_శీర్షం* స్థిరరాశి lhsp = concatp->lhsp();
                Dfg_శీర్షం* స్థిరరాశి rhsp = concatp->rhsp();

                ఒకవేళ (msb < rhsp->వెడల్పు()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_యొక్క_CONCAT) {
                        fromp = rhsp;
                        కొనసాగు;
                    }
                } లేకపోతే ఒకవేళ (lsb >= rhsp->వెడల్పు()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_యొక్క_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->వెడల్పు();
                        కొనసాగు;
                    }
                }
            }

            ఒకవేళ (DfgRep* స్థిరరాశి repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                స్థిరరాశి uint32_t src_వెడల్పు = repp->srcp()->వెడల్పు();
                ఒకవేళ (వెడల్పు <= src_వెడల్పు) {
                    స్థిరరాశి uint32_t newLsb = lsb % src_వెడల్పు;
                    స్థిరరాశి uint32_t newMsb = newLsb + వెడల్పు - 1;
                    ఒకవేళ (newMsb < src_వెడల్పు) {
                        APPLYING(నెట్టు_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            కొనసాగు;
                        }
                    }
                }
            }

            // Sel from Sel
            ఒకవేళ (DfgSel* స్థిరరాశి selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    కొనసాగు;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            ఒకవేళ (DfgVarPacked* స్థిరరాశి varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                స్థిరరాశి స్వయంచాలక pair = varp->driver_యొక్క_Range(lsb, వెడల్పు);
                Dfg_శీర్షం* స్థిరరాశి driverp = pair.మొదటి;
                స్థిరరాశి uint32_t driverLsb = pair.రెండవ;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                ఒకవేళ (driverp) {
                    ఒకవేళ (driverp == varp->defaultp()) {
                        APPLYING(నెట్టు_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            కొనసాగు;
                        }
                    } లేకపోతే {
                        APPLYING(నెట్టు_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            కొనసాగు;
                        }
                    }
                }
            }

            // No patterns matched, stop
            ఆపు;
        }
        తిరిగి {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_శీర్షం*, uint32_t> commonLSBs(Dfg_శీర్షం* ap, Dfg_శీర్షం* bp) {
        ఒకవేళ (ap == bp) తిరిగి {ap, ap->వెడల్పు()};

        // If both constants, check LSBs
        ఒకవేళ (DfgConst* స్థిరరాశి aConstp = ap->cast<DfgConst>()) {
            ఒకవేళ (DfgConst* స్థిరరాశి bConstp = bp->cast<DfgConst>()) {
                స్థిరరాశి V3_సంఖ్య& a_అంకె = aConstp->అంకె();
                స్థిరరాశి V3_సంఖ్య& b_అంకె = bConstp->అంకె();
                // Max match is the shorter constant
                స్థిరరాశి uint32_t max_సరిపోలిక = std::min(aConstp->వెడల్పు(), bConstp->వెడల్పు());
                // Check all bits
                uint32_t సరిపోలిక_వెడల్పు = 0;
                కొరకు (; సరిపోలిక_వెడల్పు < max_సరిపోలిక; ++సరిపోలిక_వెడల్పు) {
                    ఒకవేళ (a_అంకె.బిట్టు_Is0(సరిపోలిక_వెడల్పు) != b_అంకె.బిట్టు_Is0(సరిపోలిక_వెడల్పు)) ఆపు;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* స్థిరరాశి shorterp = aConstp->వెడల్పు() < bConstp->వెడల్పు() ? aConstp : bConstp;
                తిరిగి {సరిపోలిక_వెడల్పు ? shorterp : శూన్యసూచిక, సరిపోలిక_వెడల్పు};
            }
        }

        // If Concat, check against the RHS
        ఒకవేళ (DfgConcat* స్థిరరాశి catp = ap->cast<DfgConcat>()) తిరిగి commonLSBs(catp->rhsp(), bp);
        ఒకవేళ (DfgConcat* స్థిరరాశి catp = bp->cast<DfgConcat>()) తిరిగి commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        ఒకవేళ (DfgSel* స్థిరరాశి selp = ap->cast<DfgSel>()) {
            ఒకవేళ (selp->lsb() == 0) {
                Dfg_శీర్షం* స్థిరరాశి fromp = selp->fromp();
                స్థిరరాశి std::pair<Dfg_శీర్షం*, uint32_t> common = commonLSBs(fromp, bp);
                తిరిగి {common.మొదటి, std::min(common.రెండవ, selp->వెడల్పు())};
            }
        }
        ఒకవేళ (DfgSel* స్థిరరాశి selp = bp->cast<DfgSel>()) {
            ఒకవేళ (selp->lsb() == 0) {
                Dfg_శీర్షం* స్థిరరాశి fromp = selp->fromp();
                స్థిరరాశి std::pair<Dfg_శీర్షం*, uint32_t> common = commonLSBs(ap, fromp);
                తిరిగి {common.మొదటి, std::min(common.రెండవ, selp->వెడల్పు())};
            }
        }

        // Otherwise no common LSBs
        తిరిగి {శూన్యసూచిక, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<Dfg_శీర్షం*, uint32_t> commonMSBs(Dfg_శీర్షం* ap, Dfg_శీర్షం* bp) {
        ఒకవేళ (ap == bp) తిరిగి {ap, ap->వెడల్పు()};

        // If both constants, check MSBs
        ఒకవేళ (DfgConst* స్థిరరాశి aConstp = ap->cast<DfgConst>()) {
            ఒకవేళ (DfgConst* స్థిరరాశి bConstp = bp->cast<DfgConst>()) {
                స్థిరరాశి uint32_t aMsb = aConstp->వెడల్పు() - 1;
                స్థిరరాశి uint32_t bMsb = bConstp->వెడల్పు() - 1;
                స్థిరరాశి V3_సంఖ్య& a_అంకె = aConstp->అంకె();
                స్థిరరాశి V3_సంఖ్య& b_అంకె = bConstp->అంకె();
                // Max match is the shorter constant
                స్థిరరాశి uint32_t max_సరిపోలిక = std::min(aConstp->వెడల్పు(), bConstp->వెడల్పు());
                // Check all bits
                uint32_t సరిపోలిక_వెడల్పు = 0;
                కొరకు (; సరిపోలిక_వెడల్పు < max_సరిపోలిక; ++సరిపోలిక_వెడల్పు) {
                    ఒకవేళ (a_అంకె.బిట్టు_Is0(aMsb - సరిపోలిక_వెడల్పు) != b_అంకె.బిట్టు_Is0(bMsb - సరిపోలిక_వెడల్పు)) ఆపు;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* స్థిరరాశి shorterp = aMsb < bMsb ? aConstp : bConstp;
                తిరిగి {సరిపోలిక_వెడల్పు ? shorterp : శూన్యసూచిక, సరిపోలిక_వెడల్పు};
            }
        }

        // If Concat, check against the LHS
        ఒకవేళ (DfgConcat* స్థిరరాశి catp = ap->cast<DfgConcat>()) తిరిగి commonMSBs(catp->lhsp(), bp);
        ఒకవేళ (DfgConcat* స్థిరరాశి catp = bp->cast<DfgConcat>()) తిరిగి commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        ఒకవేళ (DfgSel* స్థిరరాశి selp = ap->cast<DfgSel>()) {
            Dfg_శీర్షం* స్థిరరాశి fromp = selp->fromp();
            ఒకవేళ (selp->msb() == fromp->వెడల్పు() - 1) {
                స్థిరరాశి std::pair<Dfg_శీర్షం*, uint32_t> common = commonMSBs(fromp, bp);
                తిరిగి {common.మొదటి, std::min(common.రెండవ, selp->వెడల్పు())};
            }
        }
        ఒకవేళ (DfgSel* స్థిరరాశి selp = bp->cast<DfgSel>()) {
            Dfg_శీర్షం* స్థిరరాశి fromp = selp->fromp();
            ఒకవేళ (selp->msb() == fromp->వెడల్పు() - 1) {
                స్థిరరాశి std::pair<Dfg_శీర్షం*, uint32_t> common = commonMSBs(ap, fromp);
                తిరిగి {common.మొదటి, std::min(common.రెండవ, selp->వెడల్పు())};
            }
        }

        // Otherwise no common MSBs
        తిరిగి {శూన్యసూచిక, 0};
    }

    // VISIT methods

    శూన్య సందర్శించు(Dfg_శీర్షం*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    శూన్య సందర్శించు(Dfg_లెక్క_Ones* స్థిరరాశి vtxp) override {
        ఒకవేళ (foldUnary(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgExtend* స్థిరరాశి vtxp) override {
        ఒకవేళ (foldUnary(vtxp)) తిరిగి;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            Dfg_శీర్షం* స్థిరరాశి zerop
                = makeZero(vtxp->fileline(), vtxp->వెడల్పు() - vtxp->srcp()->వెడల్పు());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            తిరిగి;
        }
    }

    శూన్య సందర్శించు(DfgExtendS* స్థిరరాశి vtxp) override {
        ఒకవేళ (foldUnary(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgLogNot* స్థిరరాశి vtxp) override {
        ఒకవేళ (foldUnary(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgNegate* స్థిరరాశి vtxp) override {
        ఒకవేళ (foldUnary(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgNot* స్థిరరాశి vtxp) override {
        ఒకవేళ (foldUnary(vtxp)) తిరిగి;

        // Not of Cond
        ఒకవేళ (DfgCond* స్థిరరాశి condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            ఒకవేళ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(నెట్టు_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* స్థిరరాశి newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* స్థిరరాశి newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* స్థిరరాశి newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    తిరిగి;
                }
            }
        }

        // Not of Not
        ఒకవేళ (DfgNot* స్థిరరాశి notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                తిరిగి;
            }
        }

        ఒకవేళ (!vtxp->srcp()->has_గుణిజం_Sinks()) {
            // Not of Eq
            ఒకవేళ (DfgEq* స్థిరరాశి eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    తిరిగి;
                }
            }

            // Not of Neq
            ఒకవేళ (DfgNeq* స్థిరరాశి neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    తిరిగి;
                }
            }

            // Not of unsigned comparisons
            ఒకవేళ (DfgLt* స్థిరరాశి p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    తిరిగి;
                }
            }
            ఒకవేళ (DfgGte* స్థిరరాశి p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    తిరిగి;
                }
            }
            ఒకవేళ (DfgGt* స్థిరరాశి p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    తిరిగి;
                }
            }
            ఒకవేళ (DfgLte* స్థిరరాశి p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    తిరిగి;
                }
            }

            // Not of signed comparisons
            ఒకవేళ (DfgLtS* స్థిరరాశి p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    తిరిగి;
                }
            }
            ఒకవేళ (DfgGteS* స్థిరరాశి p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    తిరిగి;
                }
            }
            ఒకవేళ (DfgGtS* స్థిరరాశి p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    తిరిగి;
                }
            }
            ఒకవేళ (DfgLteS* స్థిరరాశి p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgOneHot* స్థిరరాశి vtxp) override {
        ఒకవేళ (foldUnary(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgOneHot0* స్థిరరాశి vtxp) override {
        ఒకవేళ (foldUnary(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgRedOr* స్థిరరాశి vtxp) override {
        ఒకవేళ (optimizeReduction(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgRedAnd* స్థిరరాశి vtxp) override {
        ఒకవేళ (optimizeReduction(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgRedXor* స్థిరరాశి vtxp) override {
        ఒకవేళ (optimizeReduction(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgSel* స్థిరరాశి vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి fromp = vtxp->fromp();

        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        స్థిరరాశి uint32_t lsb = vtxp->lsb();
        స్థిరరాశి uint32_t వెడల్పు = vtxp->వెడల్పు();
        స్థిరరాశి uint32_t msb = lsb + వెడల్పు - 1;

        ఒకవేళ (DfgConst* స్థిరరాశి constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* స్థిరరాశి resp = makeZero(flp, వెడల్పు);
                resp->అంకె().opSel(constp->అంకె(), msb, lsb);
                replace(resp);
                తిరిగి;
            }
        }

        // Full width select, replace with the source.
        ఒకవేళ (fromp->వెడల్పు() == వెడల్పు) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_నిండిన_వెడల్పు_SEL) {
                replace(fromp);
                తిరిగి;
            }
        }

        // Unwind through bit packing in one go
        {
            స్థిరరాశి స్వయంచాలక res = unwindSel(fromp, lsb, వెడల్పు);
            ఒకవేళ (res.మొదటి != fromp || res.రెండవ != lsb) {
                replace(make<DfgSel>(vtxp, res.మొదటి, res.రెండవ));
                తిరిగి;
            }
        }

        // Sel from Not
        ఒకవేళ (DfgNot* స్థిరరాశి notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            ఒకవేళ (!notp->has_గుణిజం_Sinks()) {
                APPLYING(నెట్టు_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* స్థిరరాశి newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    తిరిగి;
                }
            }
        }

        // Sel from Cond
        ఒకవేళ (DfgCond* స్థిరరాశి condp = fromp->cast<DfgCond>()) {
            ఒకవేళ (!condp->has_గుణిజం_Sinks()) {
                APPLYING(నెట్టు_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* స్థిరరాశి newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* స్థిరరాశి newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* స్థిరరాశి newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    తిరిగి;
                }
            }
        }

        // Sel from ShiftL
        ఒకవేళ (Dfg_జరుపు_L* స్థిరరాశి జరుపు_Lp = fromp->cast<Dfg_జరుపు_L>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            ఒకవేళ (lsb == 0) {
                UASSERT_OBJ(జరుపు_Lp->lhsp()->వెడల్పు() >= వెడల్పు, vtxp, "input of shift narrow");
                APPLYING(నెట్టు_SEL_THROUGH_SHIFTL) {
                    DfgSel* స్థిరరాశి newSelp = make<DfgSel>(vtxp, జరుపు_Lp->lhsp(), vtxp->lsb());
                    replace(make<Dfg_జరుపు_L>(vtxp, newSelp, జరుపు_Lp->rhsp()));
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgMux* స్థిరరాశి vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి fromp = vtxp->fromp();
        Dfg_శీర్షం* స్థిరరాశి lsbp = vtxp->lsbp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (DfgConst* స్థిరరాశి lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->అంకె().గమ్యం_UInt()));
                తిరిగి;
            }
        }

        ఒకవేళ (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->వెడల్పు()));
                తిరిగి;
            }
        }

        ఒకవేళ (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->వెడల్పు()));
                తిరిగి;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    శూన్య సందర్శించు(DfgAnd* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ఒకవేళ (!lhsp->has_గుణిజం_Sinks() || !rhsp->has_గుణిజం_Sinks()) {
            ఒకవేళ (DfgNot* స్థిరరాశి lhsNotp = lhsp->cast<DfgNot>()) {
                ఒకవేళ (DfgNot* స్థిరరాశి rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_యొక్క_NOT_AND_NOT) {
                        DfgOr* స్థిరరాశి orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        తిరిగి;
                    }
                }
                ఒకవేళ (DfgNeq* స్థిరరాశి rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_యొక్క_NOT_AND_NEQ) {
                        DfgEq* స్థిరరాశి newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* స్థిరరాశి orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి lConstp = lhsp->cast<DfgConst>()) {
            ఒకవేళ (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    తిరిగి;
                }
            }

            ఒకవేళ (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    తిరిగి;
                }
            }

            ఒకవేళ (DfgConcat* స్థిరరాశి rhsConcatp = rhsp->cast<DfgConcat>()) {
                ఒకవేళ (try_నెట్టు_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) తిరిగి;
            }
        }

        ఒకవేళ (distributive_And_Associative_ద్విమాన<DfgOr, DfgAnd>(vtxp)) తిరిగి;

        ఒకవేళ (try_నెట్టు_Bitwise_Op_Through_Reductions(vtxp)) తిరిగి;

        ఒకవేళ (try_నెట్టు_Bitwise_Op_Throug_Sel(vtxp)) తిరిగి;

        {
            DfgNot* స్థిరరాశి lNotp = lhsp->cast<DfgNot>();
            DfgNot* స్థిరరాశి rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            ఒకవేళ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->వెడల్పు()));
                    తిరిగి;
                }
            }

            ఒకవేళ (DfgAnd* స్థిరరాశి rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* స్థిరరాశి rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* స్థిరరాశి rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                ఒకవేళ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->వెడల్పు()));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (vtxp->dtype() == m_బిట్టు_DType) {
            ఒకవేళ (tryReplaceBitwiseWithReduction(vtxp)) తిరిగి;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            ఒకవేళ (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            ఒకవేళ (repp && condp && repp->srcp()->కొలత() == 1 && !condp->has_గుణిజం_Sinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* స్థిరరాశి newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgOr* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ఒకవేళ (!lhsp->has_గుణిజం_Sinks() || !rhsp->has_గుణిజం_Sinks()) {
            ఒకవేళ (DfgNot* స్థిరరాశి lhsNotp = lhsp->cast<DfgNot>()) {
                ఒకవేళ (DfgNot* స్థిరరాశి rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_యొక్క_NOT_AND_NOT) {
                        DfgAnd* స్థిరరాశి andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        తిరిగి;
                    }
                }
                ఒకవేళ (DfgNeq* స్థిరరాశి rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_యొక్క_NOT_AND_NEQ) {
                        DfgEq* స్థిరరాశి newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* స్థిరరాశి andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (DfgConcat* స్థిరరాశి lhsConcatp = lhsp->cast<DfgConcat>()) {
            ఒకవేళ (DfgConcat* స్థిరరాశి rhsConcatp = rhsp->cast<DfgConcat>()) {
                ఒకవేళ (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    ఒకవేళ (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_యొక్క_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            తిరిగి;
                        }
                    }
                    ఒకవేళ (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_యొక్క_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            తిరిగి;
                        }
                    }
                }
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి lConstp = lhsp->cast<DfgConst>()) {
            ఒకవేళ (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    తిరిగి;
                }
            }

            ఒకవేళ (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    తిరిగి;
                }
            }

            ఒకవేళ (DfgConcat* స్థిరరాశి rhsConcatp = rhsp->cast<DfgConcat>()) {
                ఒకవేళ (try_నెట్టు_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) తిరిగి;
            }
        }

        ఒకవేళ (distributive_And_Associative_ద్విమాన<DfgAnd, DfgOr>(vtxp)) తిరిగి;

        ఒకవేళ (try_నెట్టు_Bitwise_Op_Through_Reductions(vtxp)) తిరిగి;

        ఒకవేళ (try_నెట్టు_Bitwise_Op_Throug_Sel(vtxp)) తిరిగి;

        {
            DfgNot* స్థిరరాశి lNotp = lhsp->cast<DfgNot>();
            DfgNot* స్థిరరాశి rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            ఒకవేళ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->వెడల్పు()));
                    తిరిగి;
                }
            }

            ఒకవేళ (DfgOr* స్థిరరాశి rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* స్థిరరాశి rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* స్థిరరాశి rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                ఒకవేళ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->వెడల్పు()));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (vtxp->dtype() == m_బిట్టు_DType) {
            ఒకవేళ (tryReplaceBitwiseWithReduction(vtxp)) తిరిగి;
        }
    }

    శూన్య సందర్శించు(DfgXor* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->వెడల్పు()));
                తిరిగి;
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి lConstp = lhsp->cast<DfgConst>()) {
            ఒకవేళ (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    తిరిగి;
                }
            }
            ఒకవేళ (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    తిరిగి;
                }
            }
            ఒకవేళ (DfgConcat* స్థిరరాశి rConcatp = rhsp->cast<DfgConcat>()) {
                ఒకవేళ (try_నెట్టు_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) తిరిగి;
            }
        }

        ఒకవేళ (try_నెట్టు_Bitwise_Op_Through_Reductions(vtxp)) తిరిగి;

        ఒకవేళ (try_నెట్టు_Bitwise_Op_Throug_Sel(vtxp)) తిరిగి;

        ఒకవేళ (vtxp->dtype() == m_బిట్టు_DType) {
            ఒకవేళ (tryReplaceBitwiseWithReduction(vtxp)) తిరిగి;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    శూన్య సందర్శించు(Dfg_కూడు* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isZero(lhsp)) {
            APPLYING(REMOVE_కూడు_ZERO) {
                replace(rhsp);
                తిరిగి;
            }
        }

        స్థిరరాశి std::tuple<Dfg_శీర్షం*, uint32_t, uint32_t> l_పదం = కూడు_గమ్యం_లెక్క_Ones_పదం(lhsp);
        ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి lVtxp = std::get<0>(l_పదం)) {
            std::tuple<Dfg_శీర్షం*, uint32_t, uint32_t> r_పదం = కూడు_గమ్యం_లెక్క_Ones_పదం(rhsp);
            Dfg_శీర్షం* extrap = శూన్యసూచిక;
            ఒకవేళ (!std::get<0>(r_పదం)) {
                ఒకవేళ (Dfg_కూడు* స్థిరరాశి rAddp = rhsp->cast<Dfg_కూడు>()) {
                    r_పదం = కూడు_గమ్యం_లెక్క_Ones_పదం(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి rVtxp = std::get<0>(r_పదం)) {
                ఒకవేళ (isSame(lVtxp, rVtxp)) {
                    స్థిరరాశి uint32_t lLsb = std::get<1>(l_పదం);
                    స్థిరరాశి uint32_t rLsb = std::get<1>(r_పదం);
                    స్థిరరాశి uint32_t l_వెడల్పు = std::get<2>(l_పదం);
                    స్థిరరాశి uint32_t r_వెడల్పు = std::get<2>(r_పదం);
                    తార్కిక adjoined = నిజం;
                    uint32_t lsb = 0;
                    ఒకవేళ (lLsb + l_వెడల్పు == rLsb) {
                        lsb = lLsb;
                    } లేకపోతే ఒకవేళ (lLsb == rLsb + r_వెడల్పు) {
                        lsb = rLsb;
                    } లేకపోతే {
                        adjoined = అబద్ధం;
                    }
                    ఒకవేళ (adjoined) {
                        APPLYING(REPLACE_కూడు_WITH_లెక్క_ONES) {
                            DfgSel* స్థిరరాశి selp
                                = make<DfgSel>(vtxp->fileline(),
                                               Dfg_దత్తాంశం_Type::packed(l_వెడల్పు + r_వెడల్పు), lVtxp, lsb);
                            Dfg_శీర్షం* resp
                                = make<Dfg_లెక్క_Ones>(flp, Dfg_దత్తాంశం_Type::packed(32), selp);
                            ఒకవేళ (vtxp->వెడల్పు() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->వెడల్పు() - 32U),
                                                       resp);
                            } లేకపోతే ఒకవేళ (vtxp->వెడల్పు() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            ఒకవేళ (extrap) resp = make<Dfg_కూడు>(vtxp, resp, extrap);
                            replace(resp);
                            తిరిగి;
                        }
                    }
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgArraySel* స్థిరరాశి vtxp) override {
        DfgConst* స్థిరరాశి idxp = vtxp->bitp()->cast<DfgConst>();
        ఒకవేళ (!idxp) తిరిగి;
        DfgVarArray* స్థిరరాశి varp = vtxp->fromp()->cast<DfgVarArray>();
        ఒకవేళ (!varp) తిరిగి;
        AstVar* స్థిరరాశి astVarp = varp->vscp()->varp();
        ఒకవేళ (astVarp->isForced()) తిరిగి;
        ఒకవేళ (astVarp->isSigUserRWPublic()) తిరిగి;
        Dfg_శీర్షం* స్థిరరాశి srcp = varp->srcp();
        ఒకవేళ (!srcp) {
            ఒకవేళ (vtxp->isPacked()) {
                ఒకవేళ (AstConsArray* స్థిరరాశి iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    ఒకవేళ (AstConst* స్థిరరాశి valp = VN_CAST(iap->valuep(idxp->గమ్యం_కొలత_T()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_పట్టిక) {
                            replace(కొత్త DfgConst{m_dfg, valp->fileline(), valp->అంకె()});
                            తిరిగి;
                        }
                    }
                }
            }
            తిరిగి;
        }

        ఒకవేళ (DfgSpliceArray* స్థిరరాశి splicep = srcp->cast<DfgSpliceArray>()) {
            Dfg_శీర్షం* స్థిరరాశి driverp = splicep->driverAt(idxp->గమ్యం_కొలత_T());
            ఒకవేళ (!driverp) తిరిగి;
            DfgUnitArray* స్థిరరాశి uap = driverp->cast<DfgUnitArray>();
            ఒకవేళ (!uap) తిరిగి;
            ఒకవేళ (uap->srcp()->is<Dfg_శీర్షం_Splice>()) తిరిగి;
            // If driven by a variable that had a Driver in DFG, it is partial
            ఒకవేళ (Dfg_శీర్షం_Var* స్థిరరాశి dvarp = uap->srcp()->cast<Dfg_శీర్షం_Var>()) {
                ఒకవేళ (dvarp->srcp()) తిరిగి;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                తిరిగి;
            }
        }

        ఒకవేళ (DfgUnitArray* స్థిరరాశి uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->గమ్యం_కొలత_T() == 0, vtxp, "Array index out of range");
            ఒకవేళ (uap->srcp()->is<DfgSplicePacked>()) తిరిగి;
            // If driven by a variable that had a Driver in DFG, it is partial
            ఒకవేళ (Dfg_శీర్షం_Var* స్థిరరాశి dvarp = uap->srcp()->cast<Dfg_శీర్షం_Var>()) {
                ఒకవేళ (dvarp->srcp()) తిరిగి;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgConcat* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();

        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isZero(lhsp)) {
            DfgConst* స్థిరరాశి lConstp = lhsp->as<DfgConst>();
            ఒకవేళ (DfgSel* స్థిరరాశి rSelp = rhsp->cast<DfgSel>()) {
                ఒకవేళ (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->వెడల్పు()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_శిఖరం_WITH_SHIFTR) {
                        replace(
                            make<Dfg_జరుపు_R>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->వెడల్పు())));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (isZero(rhsp)) {
            DfgConst* స్థిరరాశి rConstp = rhsp->as<DfgConst>();
            ఒకవేళ (DfgSel* స్థిరరాశి lSelp = lhsp->cast<DfgSel>()) {
                ఒకవేళ (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<Dfg_జరుపు_L>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->వెడల్పు())));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (DfgNot* స్థిరరాశి lNot = lhsp->cast<DfgNot>()) {
            ఒకవేళ (DfgNot* స్థిరరాశి rNot = rhsp->cast<DfgNot>()) {
                ఒకవేళ (!lNot->has_గుణిజం_Sinks() && !rNot->has_గుణిజం_Sinks()) {
                    APPLYING(నెట్టు_CONCAT_THROUGH_NOTS) {
                        DfgConcat* స్థిరరాశి newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        తిరిగి;
                    }
                }
            }
        }

        {
            తార్కిక మార్పిడి = అబద్ధం;
            ఒకవేళ (isConcatPushableThroughBitwise(lhsp, rhsp, మార్పిడి)) {
                APPLYING(నెట్టు_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, మార్పిడి));
                    తిరిగి;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ఒకవేళ (DfgConcat* స్థిరరాశి rConcatp = rhsp->cast<DfgConcat>()) {
                Dfg_శీర్షం* స్థిరరాశి rlp = rConcatp->lhsp();
                ఒకవేళ (!rConcatp->has_గుణిజం_Sinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, మార్పిడి)) {
                    APPLYING(నెట్టు_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        Dfg_శీర్షం* స్థిరరాశి newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, మార్పిడి);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (DfgSel* స్థిరరాశి lSelp = lhsp->cast<DfgSel>()) {
            ఒకవేళ (DfgSel* స్థిరరాశి rSelp = rhsp->cast<DfgSel>()) {
                ఒకవేళ (isSame(lSelp->fromp(), rSelp->fromp())) {
                    ఒకవేళ (lSelp->lsb() == rSelp->lsb() + rSelp->వెడల్పు()) {
                        APPLYING(REMOVE_CONCAT_యొక్క_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            తిరిగి;
                        }
                    }
                }
            }

            ఒకవేళ (DfgConcat* స్థిరరాశి rConcatp = rhsp->cast<DfgConcat>()) {
                ఒకవేళ (DfgSel* స్థిరరాశి rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    ఒకవేళ (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        ఒకవేళ (lSelp->lsb() == rlSelp->lsb() + rlSelp->వెడల్పు()) {
                            APPLYING(REPLACE_NESTED_CONCAT_యొక్క_ADJOINING_SELS_ON_LHS) {
                                స్థిరరాశి uint32_t వెడల్పు = lSelp->వెడల్పు() + rlSelp->వెడల్పు();
                                స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = Dfg_దత్తాంశం_Type::packed(వెడల్పు);
                                DfgSel* స్థిరరాశి selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                తిరిగి;
                            }
                        }
                    }
                }
            }
        }

        ఒకవేళ (DfgSel* స్థిరరాశి rSelp = rhsp->cast<DfgSel>()) {
            ఒకవేళ (DfgConcat* స్థిరరాశి lConcatp = lhsp->cast<DfgConcat>()) {
                ఒకవేళ (DfgSel* స్థిరరాశి lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    ఒకవేళ (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        ఒకవేళ (lrSelp->lsb() == rSelp->lsb() + rSelp->వెడల్పు()) {
                            APPLYING(REPLACE_NESTED_CONCAT_యొక్క_ADJOINING_SELS_ON_RHS) {
                                స్థిరరాశి uint32_t వెడల్పు = lrSelp->వెడల్పు() + rSelp->వెడల్పు();
                                స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = Dfg_దత్తాంశం_Type::packed(వెడల్పు);
                                DfgSel* స్థిరరాశి selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                తిరిగి;
                            }
                        }
                    }
                }
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి lConstp = lhsp->cast<DfgConst>()) {
            ఒకవేళ (DfgCond* స్థిరరాశి rCondp = rhsp->cast<DfgCond>()) {
                Dfg_శీర్షం* స్థిరరాశి rtVtxp = rCondp->thenp();
                Dfg_శీర్షం* స్థిరరాశి reVtxp = rCondp->elsep();
                DfgConst* స్థిరరాశి rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* స్థిరరాశి reConstp = reVtxp->cast<DfgConst>();
                ఒకవేళ (!rCondp->has_గుణిజం_Sinks() && (rtConstp || reConstp)) {
                    APPLYING(నెట్టు_CONCAT_THROUGH_COND_LHS) {
                        Dfg_శీర్షం* స్థిరరాశి thenp = [&]() -> Dfg_శీర్షం* {
                            File_పంక్తి* స్థిరరాశి rtFlp = rtVtxp->fileline();
                            ఒకవేళ (rtConstp) {
                                DfgConst* స్థిరరాశి constp = makeZero(rtFlp, vtxp->వెడల్పు());
                                constp->అంకె().opConcat(lConstp->అంకె(), rtConstp->అంకె());
                                తిరిగి constp;
                            }
                            తిరిగి make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        Dfg_శీర్షం* స్థిరరాశి elsep = [&]() -> Dfg_శీర్షం* {
                            File_పంక్తి* స్థిరరాశి reFlp = reVtxp->fileline();
                            ఒకవేళ (reConstp) {
                                DfgConst* స్థిరరాశి constp = makeZero(reFlp, vtxp->వెడల్పు());
                                constp->అంకె().opConcat(lConstp->అంకె(), reConstp->అంకె());
                                తిరిగి constp;
                            }
                            తిరిగి make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = rhsp->cast<DfgConst>()) {
            ఒకవేళ (DfgCond* స్థిరరాశి lCondp = lhsp->cast<DfgCond>()) {
                Dfg_శీర్షం* స్థిరరాశి ltVtxp = lCondp->thenp();
                Dfg_శీర్షం* స్థిరరాశి leVtxp = lCondp->elsep();
                DfgConst* స్థిరరాశి ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* స్థిరరాశి leConstp = leVtxp->cast<DfgConst>();
                ఒకవేళ (!lCondp->has_గుణిజం_Sinks() && (ltConstp || leConstp)) {
                    APPLYING(నెట్టు_CONCAT_THROUGH_COND_RHS) {
                        Dfg_శీర్షం* స్థిరరాశి thenp = [&]() -> Dfg_శీర్షం* {
                            File_పంక్తి* స్థిరరాశి ltFlp = ltVtxp->fileline();
                            ఒకవేళ (ltConstp) {
                                DfgConst* స్థిరరాశి constp = makeZero(ltFlp, vtxp->వెడల్పు());
                                constp->అంకె().opConcat(ltConstp->అంకె(), rConstp->అంకె());
                                తిరిగి constp;
                            }
                            తిరిగి make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        Dfg_శీర్షం* స్థిరరాశి elsep = [&]() -> Dfg_శీర్షం* {
                            File_పంక్తి* స్థిరరాశి leFlp = leVtxp->fileline();
                            ఒకవేళ (leConstp) {
                                DfgConst* స్థిరరాశి constp = makeZero(leFlp, vtxp->వెడల్పు());
                                constp->అంకె().opConcat(leConstp->అంకె(), rConstp->అంకె());
                                తిరిగి constp;
                            }
                            తిరిగి make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        తిరిగి;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            స్థిరరాశి uint32_t vMsb = vtxp->వెడల్పు() - 1;  // MSB of the concatenation
            స్థిరరాశి uint32_t lLsb = vtxp->rhsp()->వెడల్పు();  // LSB of the LHS
            స్థిరరాశి uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            తార్కిక hasCrossSink = అబద్ధం;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](Dfg_శీర్షం& sink) {
                // Record bits used by DfgSel sinks
                ఒకవేళ (స్థిరరాశి DfgSel* స్థిరరాశి selp = sink.cast<DfgSel>()) {
                    స్థిరరాశి uint32_t selLsb = selp->lsb();
                    స్థిరరాశి uint32_t selMsb = selLsb + selp->వెడల్పు() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    తిరిగి అబద్ధం;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                ఒకవేళ (స్థిరరాశి DfgVarPacked* స్థిరరాశి varp = sink.cast<DfgVarPacked>()) {
                    ఒకవేళ (!varp->hasSinks() && !varp->isObserved()) తిరిగి అబద్ధం;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                తిరిగి నిజం;
            });
            ఒకవేళ (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    Dfg_శీర్షం* nRhsp = rhsp;
                    ఒకవేళ (lsb != 0)
                        nRhsp = make<DfgSel>(flp, Dfg_దత్తాంశం_Type::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    Dfg_శీర్షం* nLhsp = lhsp;
                    ఒకవేళ (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, Dfg_దత్తాంశం_Type::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    Dfg_శీర్షం* స్థిరరాశి catp
                        = make<DfgConcat>(flp, Dfg_దత్తాంశం_Type::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* స్థిరరాశి sp = కొత్త DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_చివరి_Id;
                    sp->కూడు_Driver(catp, lsb, flp);
                    Dfg_శీర్షం_Var* స్థిరరాశి varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_చివరి_Id;
                    varp->vscp()->varp()->is_అంతర్గత(నిజం);
                    varp->srcp(sp);
                    replace(varp);
                    తిరిగి;
                }
            }
        }

        // Convert various forms to REPLICATE
        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                తిరిగి;
            }
        }
        ఒకవేళ (DfgRep* స్థిరరాశి lRepp = lhsp->cast<DfgRep>()) {
            ఒకవేళ (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    తిరిగి;
                }
            }
        }
        ఒకవేళ (DfgRep* స్థిరరాశి rRepp = rhsp->cast<DfgRep>()) {
            ఒకవేళ (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    తిరిగి;
                }
            }
        }
        ఒకవేళ (DfgRep* స్థిరరాశి lRepp = lhsp->cast<DfgRep>()) {
            ఒకవేళ (DfgRep* స్థిరరాశి rRepp = rhsp->cast<DfgRep>()) {
                ఒకవేళ (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        తిరిగి;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ఒకవేళ (DfgConcat* స్థిరరాశి rConcatp = rhsp->cast<DfgConcat>()) {
                ఒకవేళ (DfgRep* స్థిరరాశి rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    ఒకవేళ (!rConcatp->has_గుణిజం_Sinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            స్థిరరాశి uint32_t వెడల్పు = lRepp->వెడల్పు() + rlRepp->వెడల్పు();
                            స్థిరరాశి Dfg_దత్తాంశం_Type& dtype = Dfg_దత్తాంశం_Type::packed(వెడల్పు);
                            DfgRep* స్థిరరాశి repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            తిరిగి;
                        }
                    }
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgDiv* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ఒకవేళ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgDivS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ఒకవేళ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgGt* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgGtS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgGte* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgGteS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgLogAnd* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp) || vtxp->rhsp()->unsafe()) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();

        ఒకవేళ (lhsp->వెడల్పు() == 1 && rhsp->వెడల్పు() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgLogEq* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgLogIf* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp) || vtxp->rhsp()->unsafe()) తిరిగి;
    }

    శూన్య సందర్శించు(DfgLogOr* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp) || vtxp->rhsp()->unsafe()) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();

        ఒకవేళ (lhsp->వెడల్పు() == 1 && rhsp->వెడల్పు() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgLt* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgLtS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgLte* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgLteS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                తిరిగి;
            }
        }
    }

    శూన్య సందర్శించు(DfgModDiv* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgModDivS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgMul* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (DfgConst* స్థిరరాశి lConstp = lhsp->cast<DfgConst>()) {
            ఒకవేళ (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    తిరిగి;
                }
            }
            ఒకవేళ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgMulS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (DfgConst* స్థిరరాశి lConstp = lhsp->cast<DfgConst>()) {
            ఒకవేళ (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    తిరిగి;
                }
            }
            ఒకవేళ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgEq* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                తిరిగి;
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి lhsConstp = lhsp->cast<DfgConst>()) {
            ఒకవేళ (DfgConcat* స్థిరరాశి rhsConcatp = rhsp->cast<DfgConcat>()) {
                ఒకవేళ (try_నెట్టు_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) తిరిగి;
            }

            ఒకవేళ (rhsp->dtype() == m_బిట్టు_DType) {
                ఒకవేళ (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_బిట్టు_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        తిరిగి;
                    }
                }

                ఒకవేళ (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_బిట్టు_1) {
                        replace(rhsp);
                        తిరిగి;
                    }
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgNeq* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                తిరిగి;
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి lhsConstp = lhsp->cast<DfgConst>()) {
            ఒకవేళ (DfgConcat* స్థిరరాశి rhsConcatp = rhsp->cast<DfgConcat>()) {
                ఒకవేళ (try_నెట్టు_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) తిరిగి;
            }

            ఒకవేళ (rhsp->dtype() == m_బిట్టు_DType) {
                ఒకవేళ (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_బిట్టు_0) {
                        replace(rhsp);
                        తిరిగి;
                    }
                }

                ఒకవేళ (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_బిట్టు_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        తిరిగి;
                    }
                }
            }
        }
    }

    శూన్య సందర్శించు(DfgPow* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgPowSS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgPowSU* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgPowUS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgRep* స్థిరరాశి vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి srcp = vtxp->srcp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (DfgConst* స్థిరరాశి sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* స్థిరరాశి resultp = makeZero(flp, vtxp->వెడల్పు());
                resultp->అంకె().opRepl(sConstp->అంకె(), vtxp->లెక్క());
                replace(resultp);
                తిరిగి;
            }
        }

        ఒకవేళ (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                తిరిగి;
            }
        }

        ఒకవేళ (DfgRep* స్థిరరాశి sRepp = srcp->cast<DfgRep>()) {
            ఒకవేళ (!sRepp->has_గుణిజం_Sinks() || vtxp->వెడల్పు() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(Dfg_జరుపు_L* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
        ఒకవేళ (optimize_జరుపు_RHS(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();

        ఒకవేళ (Dfg_జరుపు_L* స్థిరరాశి l_జరుపు_Lp = lhsp->cast<Dfg_జరుపు_L>()) {
            ఒకవేళ (!l_జరుపు_Lp->has_గుణిజం_Sinks() && rhsp->dtype() == l_జరుపు_Lp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
                    స్థిరరాశి Dfg_దత్తాంశం_Type& కూడిక_DType = Dfg_దత్తాంశం_Type::packed(rhsp->వెడల్పు() + 1);
                    Dfg_శీర్షం* స్థిరరాశి bp = make<DfgExtend>(flp, కూడిక_DType, l_జరుపు_Lp->rhsp());
                    Dfg_శీర్షం* స్థిరరాశి cp = make<DfgExtend>(flp, కూడిక_DType, rhsp);
                    Dfg_కూడు* స్థిరరాశి addp = make<Dfg_కూడు>(flp, కూడిక_DType, bp, cp);
                    replace(make<Dfg_జరుపు_L>(vtxp, l_జరుపు_Lp->lhsp(), addp));
                    తిరిగి;
                }
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = rhsp->cast<DfgConst>()) {
            స్థిరరాశి uint32_t జరుపు_Amount = rConstp->గమ్యం_U32();

            ఒకవేళ (జరుపు_Amount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    తిరిగి;
                }
            }

            ఒకవేళ (జరుపు_Amount >= vtxp->వెడల్పు()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->వెడల్పు()));
                    తిరిగి;
                }
            }

            ఒకవేళ (DfgConcat* స్థిరరాశి lConcatp = lhsp->cast<DfgConcat>()) {
                ఒకవేళ (!lConcatp->has_గుణిజం_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        Dfg_శీర్షం* స్థిరరాశి lRhsp = lConcatp->rhsp();
                        Dfg_శీర్షం* స్థిరరాశి lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        స్థిరరాశి uint32_t rW = జరుపు_Amount;
                        స్థిరరాశి uint32_t mW = std::min(lRhsp->వెడల్పు(), vtxp->వెడల్పు() - జరుపు_Amount);
                        స్థిరరాశి uint32_t lW = vtxp->వెడల్పు() - mW - rW;
                        // Construct the result
                        File_పంక్తి* స్థిరరాశి flp = lConcatp->fileline();
                        Dfg_శీర్షం* స్థిరరాశి rp = makeZero(flp, జరుపు_Amount);
                        Dfg_శీర్షం* స్థిరరాశి mp
                            = make<DfgSel>(flp, Dfg_దత్తాంశం_Type::packed(mW), lRhsp, 0U);
                        Dfg_శీర్షం* np = make<DfgConcat>(flp, Dfg_దత్తాంశం_Type::packed(mW + rW), mp, rp);
                        ఒకవేళ (!lW) {
                            replace(np);
                            తిరిగి;
                        }
                        Dfg_శీర్షం* స్థిరరాశి lp
                            = make<DfgSel>(flp, Dfg_దత్తాంశం_Type::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        తిరిగి;
                    }
                }
            }

            ఒకవేళ (DfgSel* స్థిరరాశి lSelp = lhsp->cast<DfgSel>()) {
                ఒకవేళ (!lSelp->has_గుణిజం_Sinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        స్థిరరాశి uint32_t n_Sel_వెడల్పు = lSelp->వెడల్పు() - జరుపు_Amount;
                        Dfg_శీర్షం* స్థిరరాశి nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_దత్తాంశం_Type::packed(n_Sel_వెడల్పు),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), జరుపు_Amount)));
                        తిరిగి;
                    }
                }
            }

            ఒకవేళ (DfgCond* స్థిరరాశి lCondp = lhsp->cast<DfgCond>()) {
                ఒకవేళ (!lCondp->has_గుణిజం_Sinks()) {
                    APPLYING(నెట్టు_SHIFTL_THROUGH_COND) {
                        Dfg_జరుపు_L* స్థిరరాశి tp = make<Dfg_జరుపు_L>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_జరుపు_L* స్థిరరాశి ep = make<Dfg_జరుపు_L>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        తిరిగి;
                    }
                }
            }
        }
    }

    శూన్య సందర్శించు(Dfg_జరుపు_R* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
        ఒకవేళ (optimize_జరుపు_RHS(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();

        ఒకవేళ (Dfg_జరుపు_R* స్థిరరాశి l_జరుపు_Rp = lhsp->cast<Dfg_జరుపు_R>()) {
            ఒకవేళ (!l_జరుపు_Rp->has_గుణిజం_Sinks() && rhsp->dtype() == l_జరుపు_Rp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();
                    స్థిరరాశి Dfg_దత్తాంశం_Type& కూడిక_DType = Dfg_దత్తాంశం_Type::packed(rhsp->వెడల్పు() + 1);
                    Dfg_శీర్షం* స్థిరరాశి bp = make<DfgExtend>(flp, కూడిక_DType, l_జరుపు_Rp->rhsp());
                    Dfg_శీర్షం* స్థిరరాశి cp = make<DfgExtend>(flp, కూడిక_DType, rhsp);
                    Dfg_కూడు* స్థిరరాశి addp = make<Dfg_కూడు>(flp, కూడిక_DType, bp, cp);
                    replace(make<Dfg_జరుపు_R>(vtxp, l_జరుపు_Rp->lhsp(), addp));
                    తిరిగి;
                }
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = rhsp->cast<DfgConst>()) {
            స్థిరరాశి uint32_t జరుపు_Amount = rConstp->గమ్యం_U32();

            ఒకవేళ (జరుపు_Amount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    తిరిగి;
                }
            }

            ఒకవేళ (జరుపు_Amount >= vtxp->వెడల్పు()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->వెడల్పు()));
                    తిరిగి;
                }
            }

            ఒకవేళ (DfgConcat* స్థిరరాశి lConcatp = lhsp->cast<DfgConcat>()) {
                ఒకవేళ (!lConcatp->has_గుణిజం_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        Dfg_శీర్షం* స్థిరరాశి lRhsp = lConcatp->rhsp();
                        Dfg_శీర్షం* స్థిరరాశి lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        స్థిరరాశి uint32_t lW = జరుపు_Amount;
                        స్థిరరాశి uint32_t mW = std::min(lLhsp->వెడల్పు(), vtxp->వెడల్పు() - జరుపు_Amount);
                        స్థిరరాశి uint32_t rW = vtxp->వెడల్పు() - mW - lW;
                        // Construct the result
                        File_పంక్తి* స్థిరరాశి flp = lConcatp->fileline();
                        Dfg_శీర్షం* స్థిరరాశి lp = makeZero(flp, జరుపు_Amount);
                        Dfg_శీర్షం* స్థిరరాశి mp = make<DfgSel>(flp, Dfg_దత్తాంశం_Type::packed(mW), lLhsp,
                                                           lLhsp->వెడల్పు() - mW);
                        Dfg_శీర్షం* np = make<DfgConcat>(flp, Dfg_దత్తాంశం_Type::packed(lW + mW), lp, mp);
                        ఒకవేళ (!rW) {
                            replace(np);
                            తిరిగి;
                        }
                        Dfg_శీర్షం* స్థిరరాశి rp = make<DfgSel>(flp, Dfg_దత్తాంశం_Type::packed(rW), lRhsp,
                                                           lRhsp->వెడల్పు() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        తిరిగి;
                    }
                }
            }

            ఒకవేళ (DfgSel* స్థిరరాశి lSelp = lhsp->cast<DfgSel>()) {
                ఒకవేళ (!lSelp->has_గుణిజం_Sinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        స్థిరరాశి uint32_t n_Sel_వెడల్పు = lSelp->వెడల్పు() - జరుపు_Amount;
                        Dfg_శీర్షం* స్థిరరాశి nSelp
                            = make<DfgSel>(lSelp->fileline(), Dfg_దత్తాంశం_Type::packed(n_Sel_వెడల్పు),
                                           lSelp->fromp(), lSelp->lsb() + జరుపు_Amount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), జరుపు_Amount), nSelp));
                        తిరిగి;
                    }
                }
            }

            ఒకవేళ (DfgCond* స్థిరరాశి lCondp = lhsp->cast<DfgCond>()) {
                ఒకవేళ (!lCondp->has_గుణిజం_Sinks()) {
                    APPLYING(నెట్టు_SHIFTR_THROUGH_COND) {
                        Dfg_జరుపు_R* స్థిరరాశి tp = make<Dfg_జరుపు_R>(vtxp, lCondp->thenp(), rConstp);
                        Dfg_జరుపు_R* స్థిరరాశి ep = make<Dfg_జరుపు_R>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        తిరిగి;
                    }
                }
            }
        }
    }

    శూన్య సందర్శించు(Dfg_జరుపు_RS* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;
        ఒకవేళ (optimize_జరుపు_RHS(vtxp)) తిరిగి;
    }

    శూన్య సందర్శించు(DfgSub* స్థిరరాశి vtxp) override {
        ఒకవేళ (ద్విమాన(vtxp)) తిరిగి;

        Dfg_శీర్షం* స్థిరరాశి lhsp = vtxp->lhsp();
        Dfg_శీర్షం* స్థిరరాశి rhsp = vtxp->rhsp();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                తిరిగి;
            }
        }

        ఒకవేళ (DfgConst* స్థిరరాశి rConstp = rhsp->cast<DfgConst>()) {
            ఒకవేళ (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    తిరిగి;
                }
            }
            ఒకవేళ (vtxp->dtype() == m_బిట్టు_DType && rConstp->has_విలువ(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_బిట్టు_DType, lhsp));
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(Dfg_సరిపోలిక_Masked* స్థిరరాశి vtxp) override {
        ఒకవేళ (DfgConst* స్థిరరాశి constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* స్థిరరాశి సరిపోలిక_Varp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), Ast_సరిపోలిక_Masked::fold(constp->అంకె(), సరిపోలిక_Varp)));
                తిరిగి;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    శూన్య సందర్శించు(DfgCond* స్థిరరాశి vtxp) override {
        Dfg_శీర్షం* స్థిరరాశి condp = vtxp->condp();
        Dfg_శీర్షం* స్థిరరాశి thenp = vtxp->thenp();
        Dfg_శీర్షం* స్థిరరాశి elsep = vtxp->elsep();
        File_పంక్తి* స్థిరరాశి flp = vtxp->fileline();

        ఒకవేళ (condp->dtype() != m_బిట్టు_DType) తిరిగి;

        ఒకవేళ (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                తిరిగి;
            }
        }

        ఒకవేళ (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                తిరిగి;
            }
        }

        ఒకవేళ (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                తిరిగి;
            }
        }

        ఒకవేళ (DfgNot* స్థిరరాశి condNotp = condp->cast<DfgNot>()) {
            ఒకవేళ (!condp->has_గుణిజం_Sinks() || condNotp->has_గుణిజం_Sinks()) {
                APPLYING(మార్పిడి_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    తిరిగి;
                }
            }
        }

        ఒకవేళ (DfgNeq* స్థిరరాశి condNeqp = condp->cast<DfgNeq>()) {
            ఒకవేళ (!condp->has_గుణిజం_Sinks()) {
                APPLYING(మార్పిడి_COND_WITH_NEQ_CONDITION) {
                    DfgEq* స్థిరరాశి newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    తిరిగి;
                }
            }
        }

        ఒకవేళ (DfgNot* స్థిరరాశి thenNotp = thenp->cast<DfgNot>()) {
            ఒకవేళ (DfgNot* స్థిరరాశి elseNotp = elsep->cast<DfgNot>()) {
                ఒకవేళ (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->has_గుణిజం_Sinks() && !elseNotp->has_గుణిజం_Sinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* స్థిరరాశి newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (DfgOr* స్థిరరాశి condOrp = condp->cast<DfgOr>()) {
            ఒకవేళ (DfgCond* స్థిరరాశి thenCondp = thenp->cast<DfgCond>()) {
                ఒకవేళ (!thenCondp->has_గుణిజం_Sinks()) {
                    ఒకవేళ (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* స్థిరరాశి ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            తిరిగి;
                        }
                    }
                    ఒకవేళ (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* స్థిరరాశి ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            తిరిగి;
                        }
                    }
                }
            }
        }

        ఒకవేళ (vtxp->వెడల్పు() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            ఒకవేళ (Dfg_కూడు* స్థిరరాశి thenAddp = thenp->cast<Dfg_కూడు>()) {
                ఒకవేళ (DfgConst* స్థిరరాశి constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    ఒకవేళ (constp->has_విలువ(1)) {
                        ఒకవేళ (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* స్థిరరాశి extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->వెడల్పు() - 1), condp);
                                File_పంక్తి* స్థిరరాశి thenFlp = thenAddp->fileline();
                                replace(
                                    make<Dfg_కూడు>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                తిరిగి;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            ఒకవేళ (DfgSub* స్థిరరాశి thenSubp = thenp->cast<DfgSub>()) {
                ఒకవేళ (DfgConst* స్థిరరాశి constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    ఒకవేళ (constp->has_విలువ(1)) {
                        ఒకవేళ (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* స్థిరరాశి extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->వెడల్పు() - 1), condp);
                                File_పంక్తి* స్థిరరాశి thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                తిరిగి;
                            }
                        }
                    }
                }
            }
        }

        ఒకవేళ (vtxp->dtype() == m_బిట్టు_DType) {
            ఒకవేళ (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    తిరిగి;
                }
            }
            ఒకవేళ (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    తిరిగి;
                }
            }
        }

        ఒకవేళ (vtxp->వెడల్పు() <= VL_QUADSIZE) {
            ఒకవేళ (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    Dfg_శీర్షం* స్థిరరాశి maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    తిరిగి;
                }
            }
            ఒకవేళ (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    Dfg_శీర్షం* స్థిరరాశి maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    తిరిగి;
                }
            }
            ఒకవేళ (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    Dfg_శీర్షం* స్థిరరాశి maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    తిరిగి;
                }
            }
            ఒకవేళ (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    Dfg_శీర్షం* స్థిరరాశి maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    తిరిగి;
                }
            }

            ఒకవేళ (DfgOr* స్థిరరాశి tOrp = thenp->cast<DfgOr>()) {
                ఒకవేళ (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        Dfg_శీర్షం* స్థిరరాశి maskp = replicate(vtxp, condp);
                        DfgAnd* స్థిరరాశి andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        తిరిగి;
                    }
                }
                ఒకవేళ (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        Dfg_శీర్షం* స్థిరరాశి maskp = replicate(vtxp, condp);
                        DfgAnd* స్థిరరాశి andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        తిరిగి;
                    }
                }
            }
        }

        ఒకవేళ (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            స్థిరరాశి std::pair<Dfg_శీర్షం*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            ఒకవేళ (cLSBs.మొదటి) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    స్థిరరాశి uint32_t r_వెడల్పు = cLSBs.రెండవ;
                    Dfg_శీర్షం* rhsp = cLSBs.మొదటి;
                    ఒకవేళ (r_వెడల్పు != rhsp->వెడల్పు()) {
                        స్థిరరాశి Dfg_దత్తాంశం_Type& rDtype = Dfg_దత్తాంశం_Type::packed(r_వెడల్పు);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    ఒకవేళ (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        తిరిగి;
                    }
                    // Create new LHS
                    స్థిరరాశి uint32_t l_వెడల్పు = vtxp->వెడల్పు() - r_వెడల్పు;
                    స్థిరరాశి Dfg_దత్తాంశం_Type& lDtype = Dfg_దత్తాంశం_Type::packed(l_వెడల్పు);
                    Dfg_శీర్షం* స్థిరరాశి lThenp = make<DfgSel>(flp, lDtype, thenp, r_వెడల్పు);
                    Dfg_శీర్షం* స్థిరరాశి lElsep = make<DfgSel>(flp, lDtype, elsep, r_వెడల్పు);
                    Dfg_శీర్షం* స్థిరరాశి lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    తిరిగి;
                }
            }

            స్థిరరాశి std::pair<Dfg_శీర్షం*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            ఒకవేళ (cMSBs.మొదటి) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    స్థిరరాశి uint32_t l_వెడల్పు = cMSBs.రెండవ;
                    Dfg_శీర్షం* lhsp = cMSBs.మొదటి;
                    ఒకవేళ (l_వెడల్పు != lhsp->వెడల్పు()) {
                        స్థిరరాశి Dfg_దత్తాంశం_Type& lDtype = Dfg_దత్తాంశం_Type::packed(l_వెడల్పు);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->వెడల్పు() - l_వెడల్పు);
                    }
                    // If it's all the same, just replace
                    ఒకవేళ (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        తిరిగి;
                    }
                    // Create new RHS
                    స్థిరరాశి uint32_t r_వెడల్పు = vtxp->వెడల్పు() - l_వెడల్పు;
                    స్థిరరాశి Dfg_దత్తాంశం_Type& rDtype = Dfg_దత్తాంశం_Type::packed(r_వెడల్పు);
                    Dfg_శీర్షం* స్థిరరాశి rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    Dfg_శీర్షం* స్థిరరాశి rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    Dfg_శీర్షం* స్థిరరాశి rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    తిరిగి;
                }
            }
        }

        ఒకవేళ (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                Dfg_శీర్షం* resp = condp;
                ఒకవేళ (స్థిరరాశి uint32_t extend = vtxp->వెడల్పు() - 1) {
                    DfgConst* స్థిరరాశి zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                తిరిగి;
            }
        }

        ఒకవేళ (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                Dfg_శీర్షం* resp = make<DfgNot>(condp, condp);
                ఒకవేళ (స్థిరరాశి uint32_t extend = vtxp->వెడల్పు() - 1) {
                    DfgConst* స్థిరరాశి zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                తిరిగి;
            }
        }

        ఒకవేళ (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                తిరిగి;
            }
        }

        ఒకవేళ (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                తిరిగి;
            }
        }

        ఒకవేళ (DfgCond* స్థిరరాశి tCondp = thenp->cast<DfgCond>()) {
            ఒకవేళ (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    తిరిగి;
                }
            }
        }

        ఒకవేళ (DfgCond* స్థిరరాశి eCondp = elsep->cast<DfgCond>()) {
            ఒకవేళ (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    తిరిగి;
                }
            }
        }
    }

    శూన్య సందర్శించు(Dfg_శీర్షం_Var* స్థిరరాశి vtxp) override {
        ఒకవేళ (vtxp->hasSinks()) తిరిగి;
        ఒకవేళ (vtxp->isObserved()) తిరిగి;
        ఒకవేళ (vtxp->defaultp()) తిరిగి;

        // If undriven, or driven from another var, it is completely redundant.
        ఒకవేళ (!vtxp->srcp() || vtxp->srcp()->is<Dfg_శీర్షం_Var>()) {
            APPLYING(REMOVE_VAR) {
                delete_శీర్షం(vtxp);
                తిరిగి;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        తార్కిక దొరికింది_One = అబద్ధం;
        స్థిరరాశి తార్కిక keep = vtxp->srcp()->foreachSink([&](Dfg_శీర్షం& sink) -> తార్కిక {
            // Ignore non-observable variable sinks. These can be eliminated.
            ఒకవేళ (స్థిరరాశి Dfg_శీర్షం_Var* స్థిరరాశి varp = sink.cast<Dfg_శీర్షం_Var>()) {
                ఒకవేళ (!varp->hasSinks() && !varp->isObserved()) తిరిగి అబద్ధం;
            }
            // Keep if found more than one sink
            ఒకవేళ (దొరికింది_One) తిరిగి నిజం;
            // Mark first sink found
            దొరికింది_One = నిజం;
            తిరిగి అబద్ధం;
        });
        ఒకవేళ (!keep) {
            APPLYING(REMOVE_VAR) {
                delete_శీర్షం(vtxp);
                తిరిగి;
            }
        }
    }

    V3DfgPeephole(Dfg_లేఖాచిత్రం& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.for_Each_శీర్షం([&](Dfg_శీర్షం& vtx) { m_vInfo[vtx].m_id = ++m_చివరి_Id; });

        // Add all operation vertices to the cache
        కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) cache_శీర్షం(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.కొలత() * 2);
        m_iterList.reserve(m_dfg.కొలత() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.నెట్టు_వెనుక(శూన్యసూచిక);
        m_iterList.నెట్టు_వెనుక(శూన్యసూచిక);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        కొరకు (Dfg_శీర్షం_Var& vtx : m_dfg.varVertices()) కూడు_గమ్యం_Work_List(&vtx);

        // Add all operation vertices to the work list
        కొరకు (Dfg_శీర్షం& vtx : m_dfg.opVertices()) కూడు_గమ్యం_Work_List(&vtx);

        // Process iteratively
        అయితే (నిజం) {
            // Process the work list - keep the placeholder at index 0
            అయితే (m_workList.కొలత() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.వెనుక();
                m_workList.తీయి_వెనుక();
                ఒకవేళ (!m_vtxp) కొనసాగు;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_work_List_సూచీ = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                ఒకవేళ (Dfg_శీర్షం_Var* స్థిరరాశి varp = m_vtxp->cast<Dfg_శీర్షం_Var>()) {
                    సందర్శించు(varp);
                    కొనసాగు;
                }

                // Ast references are not considered
                ఒకవేళ (m_vtxp->is<Dfg_శీర్షం_Ast>()) కొనసాగు;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                ఒకవేళ (!m_vInfo[m_vtxp].m_is_Cached_శీర్షం) {
                    ఒకవేళ (Dfg_శీర్షం* స్థిరరాశి sampep = cache_శీర్షం(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            కొనసాగు;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            ఒకవేళ (m_iterList.కొలత() == 1) ఆపు;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                కొలత_t begin = 0;
                కొలత_t చివర = m_iterList.కొలత();
                కొరకు (కొలత_t hops = 1; hops <= 4; ++hops) {
                    కొరకు (కొలత_t i = begin; i < చివర; ++i) {
                        Dfg_శీర్షం* స్థిరరాశి vtxp = m_iterList[i];
                        ఒకవేళ (!vtxp) కొనసాగు;
                        vtxp->foreachSink([&](Dfg_శీర్షం& dst) {
                            కూడు_గమ్యం_Iter_List(&dst);
                            తిరిగి అబద్ధం;
                        });
                        vtxp->foreach_ఆధారం([&](Dfg_శీర్షం& src) {
                            కూడు_గమ్యం_Iter_List(&src);
                            తిరిగి అబద్ధం;
                        });
                    }
                    begin = చివర;
                    చివర = m_iterList.కొలత();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : m_iterList) {
                ఒకవేళ (!vtxp) కొనసాగు;
                removeFromIterList(vtxp);
                కూడు_గమ్యం_Work_List(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#నిర్వచనం_రద్దు APPLYING

బహిరంగ:
    స్థిర శూన్య apply(Dfg_లేఖాచిత్రం& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

శూన్య V3DfgPasses::peephole(Dfg_లేఖాచిత్రం& dfg, V3DfgPeepholeContext& ctx) {
    ఒకవేళ (!v3Global.opt.fDfgPeephole()) తిరిగి;
    V3DfgPeephole::apply(dfg, ctx);
}
