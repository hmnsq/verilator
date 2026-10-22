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

template <typename T_Reduction>
संरचना ReductionToBitwiseImpl {};
template <> संरचना ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> संरचना ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> संरचना ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
संरचना BitwiseToReductionImpl {};
template <> संरचना BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> संरचना BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> संरचना BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
संरचना IsAssociative : सार्वजनिक std::false_type {};
template <> संरचना IsAssociative<DfgAnd> : सार्वजनिक std::true_type {};
template <> संरचना IsAssociative<DfgOr>  : सार्वजनिक std::true_type {};
template <> संरचना IsAssociative<DfgXor> : सार्वजनिक std::true_type {};
template <> संरचना IsAssociative<DfgAdd> : सार्वजनिक std::true_type {};
template <> संरचना IsAssociative<DfgConcat> : सार्वजनिक std::true_type {};
template <> संरचना IsAssociative<DfgMul> : सार्वजनिक std::true_type {};
template <> संरचना IsAssociative<DfgMulS> : सार्वजनिक std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
संरचना IsCommutative : सार्वजनिक std::false_type {};
template <> संरचना IsCommutative<DfgAnd> : सार्वजनिक std::true_type {};
template <> संरचना IsCommutative<DfgOr>  : सार्वजनिक std::true_type {};
template <> संरचना IsCommutative<DfgXor> : सार्वजनिक std::true_type {};
template <> संरचना IsCommutative<DfgAdd> : सार्वजनिक std::true_type {};
template <> संरचना IsCommutative<DfgEq> : सार्वजनिक std::true_type {};
template <> संरचना IsCommutative<DfgMul> : सार्वजनिक std::true_type {};
template <> संरचना IsCommutative<DfgMulS> : सार्वजनिक std::true_type {};
template <> संरचना IsCommutative<DfgNeq> : सार्वजनिक std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
संरचना IsIdempotent : सार्वजनिक std::false_type {};
template <> संरचना IsIdempotent<DfgAnd> : सार्वजनिक std::true_type {};
template <> संरचना IsIdempotent<DfgOr>  : सार्वजनिक std::true_type {};

// Binary result dtype
template<typename Vertex>
ध्रुव DfgDataType& resultDType(ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp);
template <> ध्रुव DfgDataType& resultDType<DfgAdd>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgAnd>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgConcat>     (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> ध्रुव DfgDataType& resultDType<DfgDiv>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgDivS>       (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgEq>         (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgGt>         (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgGtS>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgGte>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgGteS>       (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgLogAnd>     (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgLogEq>      (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgLogIf>      (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgLogOr>      (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgLt>         (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgLtS>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgLte>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgLteS>       (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgModDiv>     (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgModDivS>    (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgMul>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgMulS>       (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgNeq>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम DfgDataType::packed(1); }
template <> ध्रुव DfgDataType& resultDType<DfgOr>         (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgPow>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgPowSS>      (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgPowSU>      (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgPowUS>      (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgShiftL>     (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgShiftR>     (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgShiftRS>    (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgSub>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }
template <> ध्रुव DfgDataType& resultDType<DfgXor>        (ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) { प्रत्यागम lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> शून्य foldOp(V3_संख्या& out, ध्रुव V3_संख्या& src);
template <> शून्य foldOp<Dfg_मोजणी_Ones>  (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.op_मोजणी_Ones(src); }
template <> शून्य foldOp<DfgExtend>     (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opAssign(src); }
template <> शून्य foldOp<DfgExtendS>    (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opExtendS(src, src.width()); }
template <> शून्य foldOp<DfgLogNot>     (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opLogNot(src); }
template <> शून्य foldOp<DfgNegate>     (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opNegate(src); }
template <> शून्य foldOp<DfgNot>        (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opNot(src); }
template <> शून्य foldOp<DfgOneHot>     (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opOneHot(src); }
template <> शून्य foldOp<DfgOneHot0>    (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opOneHot0(src); }
template <> शून्य foldOp<DfgRedAnd>     (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opRedAnd(src); }
template <> शून्य foldOp<DfgRedOr>      (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opRedOr(src); }
template <> शून्य foldOp<DfgRedXor>     (V3_संख्या& out, ध्रुव V3_संख्या& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> शून्य foldOp(V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
template <> शून्य foldOp<DfgAdd>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opAdd(lhs, rhs); }
template <> शून्य foldOp<DfgAnd>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opAnd(lhs, rhs); }
template <> शून्य foldOp<DfgConcat>     (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opConcat(lhs, rhs); }
template <> शून्य foldOp<DfgDiv>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opDiv(lhs, rhs); }
template <> शून्य foldOp<DfgDivS>       (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opDivS(lhs, rhs); }
template <> शून्य foldOp<DfgEq>         (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opEq(lhs, rhs); }
template <> शून्य foldOp<DfgGt>         (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opGt(lhs, rhs); }
template <> शून्य foldOp<DfgGtS>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opGtS(lhs, rhs); }
template <> शून्य foldOp<DfgGte>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opGte(lhs, rhs); }
template <> शून्य foldOp<DfgGteS>       (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opGteS(lhs, rhs); }
template <> शून्य foldOp<DfgLogAnd>     (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opLogAnd(lhs, rhs); }
template <> शून्य foldOp<DfgLogEq>      (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opLogEq(lhs, rhs); }
template <> शून्य foldOp<DfgLogIf>      (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opLogIf(lhs, rhs); }
template <> शून्य foldOp<DfgLogOr>      (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opLogOr(lhs, rhs); }
template <> शून्य foldOp<DfgLt>         (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opLt(lhs, rhs); }
template <> शून्य foldOp<DfgLtS>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opLtS(lhs, rhs); }
template <> शून्य foldOp<DfgLte>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opLte(lhs, rhs); }
template <> शून्य foldOp<DfgLteS>       (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opLteS(lhs, rhs); }
template <> शून्य foldOp<DfgModDiv>     (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opModDiv(lhs, rhs); }
template <> शून्य foldOp<DfgModDivS>    (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opModDivS(lhs, rhs); }
template <> शून्य foldOp<DfgMul>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opMul(lhs, rhs); }
template <> शून्य foldOp<DfgMulS>       (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opMulS(lhs, rhs); }
template <> शून्य foldOp<DfgNeq>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opNeq(lhs, rhs); }
template <> शून्य foldOp<DfgOr>         (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opOr(lhs, rhs); }
template <> शून्य foldOp<DfgPow>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opPow(lhs, rhs); }
template <> शून्य foldOp<DfgPowSS>      (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opPowSS(lhs, rhs); }
template <> शून्य foldOp<DfgPowSU>      (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opPowSU(lhs, rhs); }
template <> शून्य foldOp<DfgPowUS>      (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opPowUS(lhs, rhs); }
template <> शून्य foldOp<DfgShiftL>     (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opShiftL(lhs, rhs); }
template <> शून्य foldOp<DfgShiftR>     (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opShiftR(lhs, rhs); }
template <> शून्य foldOp<DfgShiftRS>    (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> शून्य foldOp<DfgSub>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opSub(lhs, rhs); }
template <> शून्य foldOp<DfgXor>        (V3_संख्या& out, ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

वर्ग V3DfgPeephole final : सार्वजनिक DfgVisitor {
    // TYPES
    संरचना VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        तार्किक m_isCachedVertex = असत्य;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ध्रुव DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ध्रुव m_tmpScopep = v3Global.rootp()->शीर्ष_Scopep()->scopep();

    // STATIC STATE
    स्थिर V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) यदि (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    तार्किक checkApplying(VDfgPeepholePattern id) {
        यदि (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) प्रत्यागम असत्य;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_मोजणी[id];
        प्रत्यागम सत्य;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* ध्रुव equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        प्रत्यागम equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    शून्य invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = असत्य;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(File_ओळ* flp, ध्रुव DfgDataType& dtype, Operands... operands) {
        Vertex* ध्रुव vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = सत्य;
        प्रत्यागम vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(ध्रुव DfgDataType& dtype, Operands... operands) {
        Vertex* ध्रुव vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        प्रत्यागम vtxp;
    }

    शून्य incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    शून्य addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        यदि (vInfo.m_workListIndex) प्रत्यागम;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.ढकला_back(vtxp);
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
        यदि (vInfo.m_iterListIndex) प्रत्यागम;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.ढकला_back(vtxp);
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
        ध्रुव DfgVertexVar* ध्रुव varp = vtxp->cast<DfgVertexVar>();
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
                प्रत्यागम असत्य;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            यदि (vInfo.m_generation != m_currentGeneration) srcps.ढकला_back(&src);
            vInfo.m_generation = m_currentGeneration;
            प्रत्यागम असत्य;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        यदि (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.ढकला_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } अन्यथा {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ध्रुव auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  प्रत्यागम srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        प्रति (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        प्रति (auto it = mid; it != srcps.end(); ++it) {
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
        ध्रुव auto debugCallback = [&]() -> शून्य {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            ध्रुव auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ध्रुव DfgVertex& v) {  //
                प्रत्यागम cone->मोजणी(&v);
            });
        };
        यदि (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) प्रत्यागम;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            प्रत्यागम असत्य;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            प्रत्यागम असत्य;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            प्रत्यागम असत्य;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            प्रत्यागम असत्य;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(File_ओळ* flp, uint32_t val) { प्रत्यागम नूतन DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(File_ओळ* flp, uint32_t width) {
        प्रत्यागम नूतन DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(File_ओळ* flp, ध्रुव DfgDataType& dtype) {
        प्रत्यागम makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(File_ओळ* flp, uint32_t width) {
        DfgConst* ध्रुव resp = makeZero(flp, width);
        resp->अंक().setAllBits1();
        प्रत्यागम resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(File_ओळ* flp, ध्रुव DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* ध्रुव vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        यदि (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        यदि (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        प्रत्यागम vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(ध्रुव DfgVertex* examplep, Operands... operands) {
        प्रत्यागम make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        यदि (vtxp->dtype() == m_bitDType) प्रत्यागम bitp;
        प्रत्यागम make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    स्थिर तार्किक isSame(ध्रुव DfgVertex* ap, ध्रुव DfgVertex* bp) {
        यदि (ap == bp) प्रत्यागम सत्य;
        यदि (ap->dtype() != bp->dtype()) प्रत्यागम असत्य;
        ध्रुव DfgConst* ध्रुव aConstp = ap->cast<DfgConst>();
        यदि (!aConstp) प्रत्यागम असत्य;
        ध्रुव DfgConst* ध्रुव bConstp = bp->cast<DfgConst>();
        यदि (!bConstp) प्रत्यागम असत्य;
        प्रत्यागम aConstp->अंक().isCaseEq(bConstp->अंक());
    }

    स्थिर तार्किक isZero(ध्रुव DfgVertex* vtxp) {
        यदि (ध्रुव DfgConst* ध्रुव constp = vtxp->cast<DfgConst>()) प्रत्यागम constp->isZero();
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक isOnes(ध्रुव DfgVertex* vtxp) {
        यदि (ध्रुव DfgConst* ध्रुव constp = vtxp->cast<DfgConst>()) प्रत्यागम constp->isOnes();
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक isEqOne(ध्रुव DfgVertex* vtxp) {
        यदि (ध्रुव DfgConst* ध्रुव constp = vtxp->cast<DfgConst>()) प्रत्यागम constp->अंक().isEqOne();
        प्रत्यागम असत्य;
    }

    स्थिर तार्किक areAdjacent(uint32_t& lsb, ध्रुव DfgSel* lSelp, ध्रुव DfgSel* rSelp) {
        यदि (!isSame(lSelp->srcp(), rSelp->srcp())) प्रत्यागम असत्य;
        यदि (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            प्रत्यागम सत्य;
        }
        यदि (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक foldUnary(Vertex* ध्रुव vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::मूल्य, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::मूल्य, "Must invoke on final class");
        यदि (DfgConst* ध्रुव srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ध्रुव resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->अंक(), srcp->अंक());
                replace(resultp);
                प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक द्विमान(Vertex* ध्रुव vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विमान, Vertex>::मूल्य, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::मूल्य, "Must invoke on final class");

        File_ओळ* ध्रुव flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ध्रुव lConstp = lhsp->cast<DfgConst>();
        DfgConst* ध्रुव rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* ध्रुव lSamep = lhsp->cast<Vertex>();
        Vertex* ध्रुव rSamep = rhsp->cast<Vertex>();

        // Constant folding
        यदि (lConstp && rConstp) {
            APPLYING(FOLD_द्विमान) {
                DfgConst* ध्रुव resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->अंक(), lConstp->अंक(), rConstp->अंक());
                replace(resultp);
                प्रत्यागम सत्य;
            }
        }

        // Idempotent
        यदि VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::मूल्य) {
            यदि (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_द्विमान) {
                    replace(lhsp);
                    प्रत्यागम सत्य;
                }
            }
        }

        // Associative
        यदि VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::मूल्य) {
            // Also commutative and idempotent
            यदि VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मूल्य && IsIdempotent<Vertex>::मूल्य) {
                यदि (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_द्विमान_LHS) {
                        replace(lSamep);
                        प्रत्यागम सत्य;
                    }
                }
                यदि (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_द्विमान_RHS) {
                        replace(rSamep);
                        प्रत्यागम सत्य;
                    }
                }
            }

            यदि (lConstp && rSamep) {
                यदि (DfgConst* ध्रुव rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_द्विमान_LHS_OF_RHS) {
                        DfgConst* ध्रुव cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->अंक(), lConstp->अंक(), rlConstp->अंक());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        प्रत्यागम सत्य;
                    }
                }
            }

            यदि (lSamep && rConstp) {
                यदि (DfgConst* ध्रुव lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_द्विमान_RHS_OF_LHS) {
                        DfgConst* ध्रुव cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->अंक(), lrConstp->अंक(), rConstp->अंक());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        प्रत्यागम सत्य;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            यदि (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* ध्रुव ap = lSamep->lhsp();
                DfgVertex* ध्रुव bp = lSamep->rhsp();
                DfgVertex* ध्रुव cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                यदि (ap != cp && bp != cp) {
                    APPLYING(उजवा_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ध्रुव DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* ध्रुव bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        प्रत्यागम सत्य;
                    }
                }
            }

            यदि (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* ध्रुव rlVtxp = rSamep->lhsp();
                DfgVertex* ध्रुव rrVtxp = rSamep->rhsp();

                यदि VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मूल्य) {
                    यदि (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            प्रत्यागम सत्य;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                यदि (Vertex* ध्रुव existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    यदि (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_द्विमान_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            प्रत्यागम सत्य;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                यदि VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मूल्य) {
                    यदि (Vertex* ध्रुव existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        यदि (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_द्विमान_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                प्रत्यागम सत्य;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        यदि VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::मूल्य) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            यदि (lConstp) {
                यदि (DfgCond* ध्रुव rCondp = rhsp->cast<DfgCond>()) {
                    यदि (!rCondp->hasMultipleSinks()) {
                        APPLYING(ढकला_COMMUTATIVE_द्विमान_THROUGH_COND) {
                            DfgVertex* ध्रुव tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* ध्रुव ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            प्रत्यागम सत्य;
                        }
                    }
                }
                प्रत्यागम असत्य;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ध्रुव तार्किक swapSides = [&]() {
            // Don't swap if not commutative, duh
            यदि VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::मूल्य) प्रत्यागम असत्य;
            // Const go on left
            यदि (lConstp) प्रत्यागम असत्य;
            यदि (rConstp) प्रत्यागम सत्य;
            // Not go on left
            यदि (lhsp->is<DfgNot>()) प्रत्यागम असत्य;
            यदि (rhsp->is<DfgNot>()) प्रत्यागम सत्य;
            // Same go on right
            यदि (rSamep) प्रत्यागम असत्य;
            यदि (lSamep) प्रत्यागम सत्य;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ध्रुव VertexInfo& lInfo = m_vInfo[lhsp];
            ध्रुव VertexInfo& rInfo = m_vInfo[rhsp];
            प्रत्यागम lInfo.m_id > rInfo.m_id;
        }();
        यदि (swapSides) {
            APPLYING(SWAP_SIDES_IN_द्विमान) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                प्रत्यागम सत्य;
            }
        }

        प्रत्यागम असत्य;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक distributive_And_Associative_द्विमान(Vertex* vtxp) {
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        यदि (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            यदि (Other* ध्रुव lp = lhsp->cast<Other>()) {
                यदि (Other* ध्रुव rp = rhsp->cast<Other>()) {
                    DfgVertex* ध्रुव llp = lp->lhsp();
                    DfgVertex* ध्रुव lrp = lp->rhsp();
                    DfgVertex* ध्रुव rlp = rp->lhsp();
                    DfgVertex* ध्रुव rrp = rp->rhsp();
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
                        APPLYING(REPLACE_DISTRIBUTIVE_द्विमान) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            प्रत्यागम सत्य;
                        }
                    }
                }
            }
        }

        प्रत्यागम असत्य;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक
    try_ढकला_Bitwise_Op_Through_Concat(Vertex* ध्रुव vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ओळ* ध्रुव flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ध्रुव catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ध्रुव catRConstp = concatp->rhsp()->cast<DfgConst>();
        यदि (catLConstp || catRConstp) {
            APPLYING(ढकला_BITWISE_OP_THROUGH_CONCAT) {
                ध्रुव uint32_t width = concatp->width();
                ध्रुव DfgDataType& lDtype = concatp->lhsp()->dtype();
                ध्रुव DfgDataType& rDtype = concatp->rhsp()->dtype();
                ध्रुव uint32_t lWidth = lDtype.size();
                ध्रुव uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* ध्रुव newLhsp = [&]() -> DfgVertex* {
                    DfgConst* ध्रुव newLhsConstp = makeZero(constp->fileline(), lWidth);
                    यदि (catLConstp) {
                        V3_संख्या अंक{constp->fileline(), static_cast<पूर्णाङ्क>(lWidth), 0u};
                        अंक.opSel(constp->अंक(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->अंक(), अंक, catLConstp->अंक());
                        प्रत्यागम newLhsConstp;
                    }
                    newLhsConstp->अंक().opSel(constp->अंक(), width - 1, rWidth);
                    प्रत्यागम make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* ध्रुव newRhsp = [&]() -> DfgVertex* {
                    DfgConst* ध्रुव newRhsConstp = makeZero(constp->fileline(), rWidth);
                    यदि (catRConstp) {
                        V3_संख्या अंक{constp->fileline(), static_cast<पूर्णाङ्क>(rWidth), 0u};
                        अंक.opSel(constp->अंक(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->अंक(), अंक, catRConstp->अंक());
                        प्रत्यागम newRhsConstp;
                    }
                    newRhsConstp->अंक().opSel(constp->अंक(), rWidth - 1, 0);
                    प्रत्यागम make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक
    try_ढकला_Compare_Op_Through_Concat(Vertex* ध्रुव vtxp, DfgConst* constp, DfgConcat* concatp) {
        File_ओळ* ध्रुव flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        यदि (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(ढकला_COMPARE_OP_THROUGH_CONCAT) {
                ध्रुव uint32_t width = concatp->width();
                ध्रुव uint32_t lWidth = concatp->lhsp()->width();
                ध्रुव uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* ध्रुव newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->अंक().opSel(constp->अंक(), width - 1, rWidth);
                Vertex* ध्रुव newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ध्रुव newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->अंक().opSel(constp->अंक(), rWidth - 1, 0);
                Vertex* ध्रुव newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                यदि VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::मूल्य) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } अन्यथा यदि VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::मूल्य) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } अन्यथा {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक try_ढकला_Bitwise_Op_Through_Reductions(Bitwise* ध्रुव vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        यदि (Reduction* ध्रुव lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            यदि (Reduction* ध्रुव rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* ध्रुव lSrcp = lRedp->srcp();
                DfgVertex* ध्रुव rSrcp = rRedp->srcp();
                यदि (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(ढकला_BITWISE_THROUGH_REDUCTION) {
                        File_ओळ* ध्रुव flp = vtxp->fileline();
                        Bitwise* ध्रुव bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        प्रत्यागम सत्य;
                    }
                }
            }
        }

        प्रत्यागम असत्य;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    स्थिर std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        यदि (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            प्रत्यागम {nullptr, nullptr};
        }
        यदि (vtxp->hasMultipleSinks()) प्रत्यागम {nullptr, nullptr};
        Dfg_Vertex_द्विमान* ध्रुव bwp = vtxp->as<Dfg_Vertex_द्विमान>();
        DfgSel* ध्रुव lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ध्रुव rSelp = bwp->rhsp()->cast<DfgSel>();
        यदि (!lSelp || !rSelp) प्रत्यागम {nullptr, nullptr};
        DfgSel* ध्रुव nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* ध्रुव wSelp = nSelp == lSelp ? rSelp : lSelp;
        ध्रुव uint32_t nWidth = nSelp->fromp()->width();
        यदि (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) प्रत्यागम {nullptr, nullptr};
        यदि (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            प्रत्यागम {nullptr, nullptr};
        }
        प्रत्यागम {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक try_ढकला_Bitwise_Op_Throug_Sel(Bitwise* ध्रुव vtxp) {
        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();

        यदि (DfgSel* ध्रुव lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            यदि (DfgSel* ध्रुव selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } अन्यथा यदि (Bitwise* ध्रुव bitwisep = rhsp->cast<Bitwise>()) {
                यदि (DfgSel* ध्रुव rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } अन्यथा यदि (DfgSel* ध्रुव rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            यदि (rSelp) {
                DfgVertex* ध्रुव lFromp = lSelp->fromp();
                DfgVertex* ध्रुव rFromp = rSelp->fromp();
                यदि (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(ढकला_BITWISE_THROUGH_SEL) {
                        Bitwise* ध्रुव bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        यदि (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        प्रत्यागम सत्य;
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
            Bitwise* ध्रुव rChainp = rhsp->cast<Bitwise>();
            यदि (rChainp && rChainp->hasMultipleSinks()) प्रत्यागम असत्य;
            DfgVertex* ध्रुव rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ध्रुव auto lSels = pushableUnalignedSels(lhsp);
            ध्रुव auto rSels = pushableUnalignedSels(rTermp);
            यदि (!lSels.first && !rSels.first) प्रत्यागम असत्य;

            ध्रुव DfgVertex* ध्रुव opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            यदि (opp->is<Bitwise>()) प्रत्यागम असत्य;
            ध्रुव auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* ध्रुव nFromp = sels.first->fromp();
            DfgVertex* ध्रुव wFromp = sels.second->fromp();
            ध्रुव uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            ध्रुव auto matches = [&](ध्रुव DfgVertex* termp, ध्रुव auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                यदि (tSels.first) {
                    प्रत्यागम termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                यदि (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                यदि (ध्रुव DfgSel* ध्रुव selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                यदि (termp->type() != opp->type()) प्रत्यागम असत्य;
                // Operands might have been swapped since
                ध्रुव Dfg_Vertex_द्विमान* ध्रुव bwp = termp->as<Dfg_Vertex_द्विमान>();
                ध्रुव तार्किक nIsLhs = bwp->lhsp() == nFromp;
                यदि (!nIsLhs && bwp->rhsp() != nFromp) प्रत्यागम असत्य;
                ध्रुव DfgSel* ध्रुव slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                प्रत्यागम slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            यदि (!matches(lhsp, lSels) || !matches(rTermp, rSels)) प्रत्यागम असत्य;

            APPLYING(ढकला_BITWISE_THROUGH_UNALIGNED_SEL) {
                File_ओळ* ध्रुव flp = vtxp->fileline();
                ध्रुव DfgDataType& dtype = nFromp->dtype();
                DfgSel* ध्रुव slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                यदि (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } अन्यथा यदि (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } अन्यथा {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ध्रुव auto ढकला = [&](DfgVertex* termp, ध्रुव DfgSel* nSelp) -> DfgVertex* {
                    प्रत्यागम nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* ध्रुव newLhsp = ढकला(lhsp, lSels.first);
                DfgVertex* ध्रुव newRTermp = ढकला(rTermp, rSels.first);
                यदि (rChainp) {
                    DfgVertex* ध्रुव newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } अन्यथा {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                प्रत्यागम सत्य;
            }
        }

        प्रत्यागम असत्य;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    स्थिर तार्किक isConcatFoldable(ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        यदि (isSame(lhsp, rhsp)) प्रत्यागम सत्य;
        यदि (ध्रुव DfgRep* ध्रुव lRepp = lhsp->cast<DfgRep>()) {
            यदि (isSame(lRepp->srcp(), rhsp)) प्रत्यागम सत्य;
        }
        यदि (ध्रुव DfgRep* ध्रुव rRepp = rhsp->cast<DfgRep>()) {
            यदि (isSame(lhsp, rRepp->srcp())) प्रत्यागम सत्य;
        }
        // Concatenation of adjoining selects is a select
        यदि (ध्रुव DfgSel* ध्रुव lSelp = lhsp->cast<DfgSel>()) {
            यदि (ध्रुव DfgSel* ध्रुव rSelp = rhsp->cast<DfgSel>()) {
                प्रत्यागम isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        यदि (ध्रुव DfgNot* ध्रुव lNotp = lhsp->cast<DfgNot>()) {
            यदि (ध्रुव DfgNot* ध्रुव rNotp = rhsp->cast<DfgNot>()) {
                प्रत्यागम !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        प्रत्यागम असत्य;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    स्थिर तार्किक isSameBitwise(ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) {
        यदि (lhsp->type() != rhsp->type()) प्रत्यागम असत्य;
        प्रत्यागम lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    स्थिर तार्किक isConcatSimplifiable(ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp) {
        यदि (isConcatFoldable(lhsp, rhsp)) प्रत्यागम सत्य;
        यदि (!isSameBitwise(lhsp, rhsp)) प्रत्यागम असत्य;
        यदि (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) प्रत्यागम असत्य;
        ध्रुव Dfg_Vertex_द्विमान* ध्रुव lp = lhsp->as<Dfg_Vertex_द्विमान>();
        ध्रुव Dfg_Vertex_द्विमान* ध्रुव rp = rhsp->as<Dfg_Vertex_द्विमान>();
        यदि (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            प्रत्यागम सत्य;
        यदि (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            प्रत्यागम सत्य;
        प्रत्यागम असत्य;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    स्थिर तार्किक isConcatPushableThroughBitwise(ध्रुव DfgVertex* lhsp, ध्रुव DfgVertex* rhsp,
                                               तार्किक& swap) {
        यदि (!isSameBitwise(lhsp, rhsp)) प्रत्यागम असत्य;
        यदि (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) प्रत्यागम असत्य;
        ध्रुव Dfg_Vertex_द्विमान* ध्रुव lp = lhsp->as<Dfg_Vertex_द्विमान>();
        ध्रुव Dfg_Vertex_द्विमान* ध्रुव rp = rhsp->as<Dfg_Vertex_द्विमान>();
        swap = असत्य;
        यदि (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) प्रत्यागम सत्य;
        यदि (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) प्रत्यागम सत्य;
        swap = सत्य;
        यदि (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) प्रत्यागम सत्य;
        यदि (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) प्रत्यागम सत्य;
        प्रत्यागम असत्य;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(File_ओळ* flp, ध्रुव DfgVertex* lhsp,
                                              ध्रुव DfgVertex* rhsp, तार्किक swap) {
        ध्रुव Dfg_Vertex_द्विमान* ध्रुव lp = lhsp->as<Dfg_Vertex_द्विमान>();
        ध्रुव Dfg_Vertex_द्विमान* ध्रुव rp = rhsp->as<Dfg_Vertex_द्विमान>();
        ध्रुव DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* ध्रुव cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* ध्रुव dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* ध्रुव newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ध्रुव newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        यदि (lp->is<DfgAnd>()) प्रत्यागम make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        यदि (lp->is<DfgOr>()) प्रत्यागम make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        प्रत्यागम make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();

        यदि (DfgSel* ध्रुव lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            यदि (DfgSel* ध्रुव selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } अन्यथा यदि (Bitwise* ध्रुव rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } अन्यथा यदि (Reduction* ध्रुव rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            यदि (rSelp) {
                uint32_t lsb = 0;
                यदि (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        ध्रुव DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* ध्रुव newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        यदि (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        प्रत्यागम सत्य;
                    }
                }
            }
        }

        यदि (Reduction* ध्रुव lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            यदि (!rRedp) {
                यदि (Bitwise* ध्रुव rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            यदि (rRedp) {
                यदि (DfgSel* ध्रुव lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    यदि (DfgSel* ध्रुव rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        यदि (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                ध्रुव DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ध्रुव newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                यदि (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                प्रत्यागम सत्य;
                            }
                        }
                    }
                }
            }
        }

        प्रत्यागम असत्य;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक optimizeReduction(Reduction* ध्रुव vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        यदि (foldUnary(vtxp)) प्रत्यागम सत्य;

        DfgVertex* ध्रुव srcp = vtxp->srcp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        // Reduction of 1-bit value
        यदि (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                प्रत्यागम सत्य;
            }
        }

        यदि (DfgCond* ध्रुव condp = srcp->cast<DfgCond>()) {
            यदि (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ढकला_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ध्रुव newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ध्रुव newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ध्रुव newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    प्रत्यागम सत्य;
                }
            }
        }

        यदि (DfgConcat* ध्रुव concatp = srcp->cast<DfgConcat>()) {
            यदि (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(ढकला_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ध्रुव lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* ध्रुव rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    प्रत्यागम सत्य;
                }
            }
        }

        यदि (Bitwise* ध्रुव bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            यदि (!bitwisep->hasMultipleSinks()) {
                यदि (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(ढकला_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* ध्रुव newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* ध्रुव newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        प्रत्यागम सत्य;
                    }
                }

                यदि (DfgSel* ध्रुव lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    यदि (DfgSel* ध्रुव rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        यदि (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(ढकला_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                ध्रुव DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ध्रुव newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                प्रत्यागम सत्य;
                            }
                        }
                    }
                }
            }
        }

        प्रत्यागम असत्य;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT तार्किक optimizeShiftRHS(Shift* ध्रुव vtxp) {
        static_assert(std::is_base_of<Dfg_Vertex_द्विमान, Shift>::मूल्य, "Must invoke on binary");
        static_assert(std::is_final<Shift>::मूल्य, "Must invoke on final class");
        यदि (ध्रुव DfgConcat* ध्रुव concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            यदि (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    प्रत्यागम सत्य;
                }
            }
        }
        प्रत्यागम असत्य;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> add_To_मोजणी_Ones_Term(DfgVertex* vtxp) {
        यदि (DfgConcat* ध्रुव oCatp = vtxp->cast<DfgConcat>()) {
            यदि (isZero(oCatp->lhsp())) {
                यदि (Dfg_मोजणी_Ones* ध्रुव मोजणी_Onesp = oCatp->rhsp()->cast<Dfg_मोजणी_Ones>()) {
                    // Zero extended count ones
                    यदि (DfgSel* ध्रुव selp = मोजणी_Onesp->srcp()->cast<DfgSel>()) {
                        प्रत्यागम {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } अन्यथा यदि (DfgSel* ध्रुव selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    यदि (selp->dtype() == m_bitDType) {  //
                        प्रत्यागम {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            प्रत्यागम {nullptr, 0, 0};
        }
        यदि (Dfg_मोजणी_Ones* ध्रुव मोजणी_Onesp = vtxp->cast<Dfg_मोजणी_Ones>()) {
            // Simple count ones
            यदि (DfgSel* ध्रुव selp = मोजणी_Onesp->srcp()->cast<DfgSel>()) {
                प्रत्यागम {selp->fromp(), selp->lsb(), selp->width()};
            }
            प्रत्यागम {nullptr, 0, 0};
        }
        यदि (DfgSel* ध्रुव oSelp = vtxp->cast<DfgSel>()) {
            यदि (oSelp->lsb() == 0) {
                // Truncated count ones
                यदि (Dfg_मोजणी_Ones* ध्रुव मोजणी_Onesp = oSelp->fromp()->cast<Dfg_मोजणी_Ones>()) {
                    // Zero extended count ones
                    यदि (DfgSel* ध्रुव selp = मोजणी_Onesp->srcp()->cast<DfgSel>()) {
                        प्रत्यागम {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            यदि (oSelp->dtype() == m_bitDType) {  //
                प्रत्यागम {oSelp->fromp(), oSelp->lsb(), 1};
            }
            प्रत्यागम {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        यदि (DfgShiftR* ध्रुव shiftrp = vtxp->cast<DfgShiftR>()) {
            यदि (DfgConst* ध्रुव rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                यदि (rConstp->toU32() == shiftrp->width() - 1) {
                    प्रत्यागम {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            प्रत्यागम {nullptr, 0, 0};
        }
        // Not applicable
        प्रत्यागम {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              ध्रुव uint32_t width) {
        यावत् (सत्य) {
            ध्रुव uint32_t msb = lsb + width - 1;

            // Sel from Concat
            यदि (DfgConcat* ध्रुव concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* ध्रुव lhsp = concatp->lhsp();
                DfgVertex* ध्रुव rhsp = concatp->rhsp();

                यदि (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        अनुवर्त;
                    }
                } अन्यथा यदि (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        अनुवर्त;
                    }
                }
            }

            यदि (DfgRep* ध्रुव repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ध्रुव uint32_t srcWidth = repp->srcp()->width();
                यदि (width <= srcWidth) {
                    ध्रुव uint32_t newLsb = lsb % srcWidth;
                    ध्रुव uint32_t newMsb = newLsb + width - 1;
                    यदि (newMsb < srcWidth) {
                        APPLYING(ढकला_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            अनुवर्त;
                        }
                    }
                }
            }

            // Sel from Sel
            यदि (DfgSel* ध्रुव selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    अनुवर्त;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            यदि (DfgVarPacked* ध्रुव varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ध्रुव auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* ध्रुव driverp = pair.first;
                ध्रुव uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                यदि (driverp) {
                    यदि (driverp == varp->defaultp()) {
                        APPLYING(ढकला_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            अनुवर्त;
                        }
                    } अन्यथा {
                        APPLYING(ढकला_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            अनुवर्त;
                        }
                    }
                }
            }

            // No patterns matched, stop
            भङ्ग;
        }
        प्रत्यागम {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        यदि (ap == bp) प्रत्यागम {ap, ap->width()};

        // If both constants, check LSBs
        यदि (DfgConst* ध्रुव aConstp = ap->cast<DfgConst>()) {
            यदि (DfgConst* ध्रुव bConstp = bp->cast<DfgConst>()) {
                ध्रुव V3_संख्या& a_अंक = aConstp->अंक();
                ध्रुव V3_संख्या& b_अंक = bConstp->अंक();
                // Max match is the shorter constant
                ध्रुव uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                प्रति (; matchWidth < maxMatch; ++matchWidth) {
                    यदि (a_अंक.bitIs0(matchWidth) != b_अंक.bitIs0(matchWidth)) भङ्ग;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ध्रुव shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                प्रत्यागम {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        यदि (DfgConcat* ध्रुव catp = ap->cast<DfgConcat>()) प्रत्यागम commonLSBs(catp->rhsp(), bp);
        यदि (DfgConcat* ध्रुव catp = bp->cast<DfgConcat>()) प्रत्यागम commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        यदि (DfgSel* ध्रुव selp = ap->cast<DfgSel>()) {
            यदि (selp->lsb() == 0) {
                DfgVertex* ध्रुव fromp = selp->fromp();
                ध्रुव std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                प्रत्यागम {common.first, std::min(common.second, selp->width())};
            }
        }
        यदि (DfgSel* ध्रुव selp = bp->cast<DfgSel>()) {
            यदि (selp->lsb() == 0) {
                DfgVertex* ध्रुव fromp = selp->fromp();
                ध्रुव std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                प्रत्यागम {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        प्रत्यागम {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        यदि (ap == bp) प्रत्यागम {ap, ap->width()};

        // If both constants, check MSBs
        यदि (DfgConst* ध्रुव aConstp = ap->cast<DfgConst>()) {
            यदि (DfgConst* ध्रुव bConstp = bp->cast<DfgConst>()) {
                ध्रुव uint32_t aMsb = aConstp->width() - 1;
                ध्रुव uint32_t bMsb = bConstp->width() - 1;
                ध्रुव V3_संख्या& a_अंक = aConstp->अंक();
                ध्रुव V3_संख्या& b_अंक = bConstp->अंक();
                // Max match is the shorter constant
                ध्रुव uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                प्रति (; matchWidth < maxMatch; ++matchWidth) {
                    यदि (a_अंक.bitIs0(aMsb - matchWidth) != b_अंक.bitIs0(bMsb - matchWidth)) भङ्ग;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ध्रुव shorterp = aMsb < bMsb ? aConstp : bConstp;
                प्रत्यागम {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        यदि (DfgConcat* ध्रुव catp = ap->cast<DfgConcat>()) प्रत्यागम commonMSBs(catp->lhsp(), bp);
        यदि (DfgConcat* ध्रुव catp = bp->cast<DfgConcat>()) प्रत्यागम commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        यदि (DfgSel* ध्रुव selp = ap->cast<DfgSel>()) {
            DfgVertex* ध्रुव fromp = selp->fromp();
            यदि (selp->msb() == fromp->width() - 1) {
                ध्रुव std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                प्रत्यागम {common.first, std::min(common.second, selp->width())};
            }
        }
        यदि (DfgSel* ध्रुव selp = bp->cast<DfgSel>()) {
            DfgVertex* ध्रुव fromp = selp->fromp();
            यदि (selp->msb() == fromp->width() - 1) {
                ध्रुव std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                प्रत्यागम {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        प्रत्यागम {nullptr, 0};
    }

    // VISIT methods

    शून्य visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    शून्य visit(Dfg_मोजणी_Ones* ध्रुव vtxp) override {
        यदि (foldUnary(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgExtend* ध्रुव vtxp) override {
        यदि (foldUnary(vtxp)) प्रत्यागम;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* ध्रुव zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            प्रत्यागम;
        }
    }

    शून्य visit(DfgExtendS* ध्रुव vtxp) override {
        यदि (foldUnary(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgLogNot* ध्रुव vtxp) override {
        यदि (foldUnary(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgNegate* ध्रुव vtxp) override {
        यदि (foldUnary(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgNot* ध्रुव vtxp) override {
        यदि (foldUnary(vtxp)) प्रत्यागम;

        // Not of Cond
        यदि (DfgCond* ध्रुव condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            यदि (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(ढकला_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ध्रुव newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ध्रुव newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ध्रुव newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    प्रत्यागम;
                }
            }
        }

        // Not of Not
        यदि (DfgNot* ध्रुव notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                प्रत्यागम;
            }
        }

        यदि (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            यदि (DfgEq* ध्रुव eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    प्रत्यागम;
                }
            }

            // Not of Neq
            यदि (DfgNeq* ध्रुव neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    प्रत्यागम;
                }
            }

            // Not of unsigned comparisons
            यदि (DfgLt* ध्रुव p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    प्रत्यागम;
                }
            }
            यदि (DfgGte* ध्रुव p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    प्रत्यागम;
                }
            }
            यदि (DfgGt* ध्रुव p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    प्रत्यागम;
                }
            }
            यदि (DfgLte* ध्रुव p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    प्रत्यागम;
                }
            }

            // Not of signed comparisons
            यदि (DfgLtS* ध्रुव p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    प्रत्यागम;
                }
            }
            यदि (DfgGteS* ध्रुव p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    प्रत्यागम;
                }
            }
            यदि (DfgGtS* ध्रुव p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    प्रत्यागम;
                }
            }
            यदि (DfgLteS* ध्रुव p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgOneHot* ध्रुव vtxp) override {
        यदि (foldUnary(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgOneHot0* ध्रुव vtxp) override {
        यदि (foldUnary(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgRedOr* ध्रुव vtxp) override {
        यदि (optimizeReduction(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgRedAnd* ध्रुव vtxp) override {
        यदि (optimizeReduction(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgRedXor* ध्रुव vtxp) override {
        यदि (optimizeReduction(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgSel* ध्रुव vtxp) override {
        DfgVertex* ध्रुव fromp = vtxp->fromp();

        File_ओळ* ध्रुव flp = vtxp->fileline();

        ध्रुव uint32_t lsb = vtxp->lsb();
        ध्रुव uint32_t width = vtxp->width();
        ध्रुव uint32_t msb = lsb + width - 1;

        यदि (DfgConst* ध्रुव constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ध्रुव resp = makeZero(flp, width);
                resp->अंक().opSel(constp->अंक(), msb, lsb);
                replace(resp);
                प्रत्यागम;
            }
        }

        // Full width select, replace with the source.
        यदि (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                प्रत्यागम;
            }
        }

        // Unwind through bit packing in one go
        {
            ध्रुव auto res = unwindSel(fromp, lsb, width);
            यदि (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                प्रत्यागम;
            }
        }

        // Sel from Not
        यदि (DfgNot* ध्रुव notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            यदि (!notp->hasMultipleSinks()) {
                APPLYING(ढकला_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ध्रुव newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    प्रत्यागम;
                }
            }
        }

        // Sel from Cond
        यदि (DfgCond* ध्रुव condp = fromp->cast<DfgCond>()) {
            यदि (!condp->hasMultipleSinks()) {
                APPLYING(ढकला_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ध्रुव newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ध्रुव newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ध्रुव newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    प्रत्यागम;
                }
            }
        }

        // Sel from ShiftL
        यदि (DfgShiftL* ध्रुव shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            यदि (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(ढकला_SEL_THROUGH_SHIFTL) {
                    DfgSel* ध्रुव newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgMux* ध्रुव vtxp) override {
        DfgVertex* ध्रुव fromp = vtxp->fromp();
        DfgVertex* ध्रुव lsbp = vtxp->lsbp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (DfgConst* ध्रुव lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->अंक().toUInt()));
                प्रत्यागम;
            }
        }

        यदि (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                प्रत्यागम;
            }
        }

        यदि (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                प्रत्यागम;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    शून्य visit(DfgAnd* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        यदि (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            यदि (DfgNot* ध्रुव lhsNotp = lhsp->cast<DfgNot>()) {
                यदि (DfgNot* ध्रुव rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* ध्रुव orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        प्रत्यागम;
                    }
                }
                यदि (DfgNeq* ध्रुव rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* ध्रुव newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ध्रुव orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (DfgConst* ध्रुव lConstp = lhsp->cast<DfgConst>()) {
            यदि (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    प्रत्यागम;
                }
            }

            यदि (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    प्रत्यागम;
                }
            }

            यदि (DfgConcat* ध्रुव rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_ढकला_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) प्रत्यागम;
            }
        }

        यदि (distributive_And_Associative_द्विमान<DfgOr, DfgAnd>(vtxp)) प्रत्यागम;

        यदि (try_ढकला_Bitwise_Op_Through_Reductions(vtxp)) प्रत्यागम;

        यदि (try_ढकला_Bitwise_Op_Throug_Sel(vtxp)) प्रत्यागम;

        {
            DfgNot* ध्रुव lNotp = lhsp->cast<DfgNot>();
            DfgNot* ध्रुव rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            यदि ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    प्रत्यागम;
                }
            }

            यदि (DfgAnd* ध्रुव rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ध्रुव rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ध्रुव rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                यदि ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (vtxp->dtype() == m_bitDType) {
            यदि (tryReplaceBitwiseWithReduction(vtxp)) प्रत्यागम;
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
                    DfgAnd* ध्रुव newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgOr* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        यदि (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            यदि (DfgNot* ध्रुव lhsNotp = lhsp->cast<DfgNot>()) {
                यदि (DfgNot* ध्रुव rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* ध्रुव andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        प्रत्यागम;
                    }
                }
                यदि (DfgNeq* ध्रुव rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* ध्रुव newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ध्रुव andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (DfgConcat* ध्रुव lhsConcatp = lhsp->cast<DfgConcat>()) {
            यदि (DfgConcat* ध्रुव rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    यदि (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            प्रत्यागम;
                        }
                    }
                    यदि (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            प्रत्यागम;
                        }
                    }
                }
            }
        }

        यदि (DfgConst* ध्रुव lConstp = lhsp->cast<DfgConst>()) {
            यदि (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    प्रत्यागम;
                }
            }

            यदि (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    प्रत्यागम;
                }
            }

            यदि (DfgConcat* ध्रुव rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_ढकला_Bitwise_Op_Through_Concat(vtxp, lConstp, rhsConcatp)) प्रत्यागम;
            }
        }

        यदि (distributive_And_Associative_द्विमान<DfgAnd, DfgOr>(vtxp)) प्रत्यागम;

        यदि (try_ढकला_Bitwise_Op_Through_Reductions(vtxp)) प्रत्यागम;

        यदि (try_ढकला_Bitwise_Op_Throug_Sel(vtxp)) प्रत्यागम;

        {
            DfgNot* ध्रुव lNotp = lhsp->cast<DfgNot>();
            DfgNot* ध्रुव rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            यदि ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    प्रत्यागम;
                }
            }

            यदि (DfgOr* ध्रुव rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ध्रुव rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ध्रुव rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                यदि ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (vtxp->dtype() == m_bitDType) {
            यदि (tryReplaceBitwiseWithReduction(vtxp)) प्रत्यागम;
        }
    }

    शून्य visit(DfgXor* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                प्रत्यागम;
            }
        }

        यदि (DfgConst* ध्रुव lConstp = lhsp->cast<DfgConst>()) {
            यदि (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    प्रत्यागम;
                }
            }
            यदि (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    प्रत्यागम;
                }
            }
            यदि (DfgConcat* ध्रुव rConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_ढकला_Bitwise_Op_Through_Concat(vtxp, lConstp, rConcatp)) प्रत्यागम;
            }
        }

        यदि (try_ढकला_Bitwise_Op_Through_Reductions(vtxp)) प्रत्यागम;

        यदि (try_ढकला_Bitwise_Op_Throug_Sel(vtxp)) प्रत्यागम;

        यदि (vtxp->dtype() == m_bitDType) {
            यदि (tryReplaceBitwiseWithReduction(vtxp)) प्रत्यागम;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    शून्य visit(DfgAdd* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                प्रत्यागम;
            }
        }

        ध्रुव std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = add_To_मोजणी_Ones_Term(lhsp);
        यदि (DfgVertex* ध्रुव lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = add_To_मोजणी_Ones_Term(rhsp);
            DfgVertex* extrap = nullptr;
            यदि (!std::get<0>(rTerm)) {
                यदि (DfgAdd* ध्रुव rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = add_To_मोजणी_Ones_Term(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            यदि (DfgVertex* ध्रुव rVtxp = std::get<0>(rTerm)) {
                यदि (isSame(lVtxp, rVtxp)) {
                    ध्रुव uint32_t lLsb = std::get<1>(lTerm);
                    ध्रुव uint32_t rLsb = std::get<1>(rTerm);
                    ध्रुव uint32_t lWidth = std::get<2>(lTerm);
                    ध्रुव uint32_t rWidth = std::get<2>(rTerm);
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
                        APPLYING(REPLACE_ADD_WITH_मोजणी_ONES) {
                            DfgSel* ध्रुव selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<Dfg_मोजणी_Ones>(flp, DfgDataType::packed(32), selp);
                            यदि (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } अन्यथा यदि (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            यदि (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            प्रत्यागम;
                        }
                    }
                }
            }
        }
    }

    शून्य visit(DfgArraySel* ध्रुव vtxp) override {
        DfgConst* ध्रुव idxp = vtxp->bitp()->cast<DfgConst>();
        यदि (!idxp) प्रत्यागम;
        DfgVarArray* ध्रुव varp = vtxp->fromp()->cast<DfgVarArray>();
        यदि (!varp) प्रत्यागम;
        AstVar* ध्रुव astVarp = varp->vscp()->varp();
        यदि (astVarp->isForced()) प्रत्यागम;
        यदि (astVarp->isSigUserRWPublic()) प्रत्यागम;
        DfgVertex* ध्रुव srcp = varp->srcp();
        यदि (!srcp) {
            यदि (vtxp->isPacked()) {
                यदि (AstConsArray* ध्रुव iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    यदि (AstConst* ध्रुव valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(नूतन DfgConst{m_dfg, valp->fileline(), valp->अंक()});
                            प्रत्यागम;
                        }
                    }
                }
            }
            प्रत्यागम;
        }

        यदि (DfgSpliceArray* ध्रुव splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* ध्रुव driverp = splicep->driverAt(idxp->toSizeT());
            यदि (!driverp) प्रत्यागम;
            DfgUnitArray* ध्रुव uap = driverp->cast<DfgUnitArray>();
            यदि (!uap) प्रत्यागम;
            यदि (uap->srcp()->is<DfgVertexSplice>()) प्रत्यागम;
            // If driven by a variable that had a Driver in DFG, it is partial
            यदि (DfgVertexVar* ध्रुव dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                यदि (dvarp->srcp()) प्रत्यागम;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                प्रत्यागम;
            }
        }

        यदि (DfgUnitArray* ध्रुव uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            यदि (uap->srcp()->is<DfgSplicePacked>()) प्रत्यागम;
            // If driven by a variable that had a Driver in DFG, it is partial
            यदि (DfgVertexVar* ध्रुव dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                यदि (dvarp->srcp()) प्रत्यागम;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgConcat* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();

        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isZero(lhsp)) {
            DfgConst* ध्रुव lConstp = lhsp->as<DfgConst>();
            यदि (DfgSel* ध्रुव rSelp = rhsp->cast<DfgSel>()) {
                यदि (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_शीर्ष_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (isZero(rhsp)) {
            DfgConst* ध्रुव rConstp = rhsp->as<DfgConst>();
            यदि (DfgSel* ध्रुव lSelp = lhsp->cast<DfgSel>()) {
                यदि (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (DfgNot* ध्रुव lNot = lhsp->cast<DfgNot>()) {
            यदि (DfgNot* ध्रुव rNot = rhsp->cast<DfgNot>()) {
                यदि (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(ढकला_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ध्रुव newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        प्रत्यागम;
                    }
                }
            }
        }

        {
            तार्किक swap = असत्य;
            यदि (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(ढकला_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    प्रत्यागम;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            यदि (DfgConcat* ध्रुव rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* ध्रुव rlp = rConcatp->lhsp();
                यदि (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(ढकला_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* ध्रुव newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (DfgSel* ध्रुव lSelp = lhsp->cast<DfgSel>()) {
            यदि (DfgSel* ध्रुव rSelp = rhsp->cast<DfgSel>()) {
                यदि (isSame(lSelp->fromp(), rSelp->fromp())) {
                    यदि (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            प्रत्यागम;
                        }
                    }
                }
            }

            यदि (DfgConcat* ध्रुव rConcatp = rhsp->cast<DfgConcat>()) {
                यदि (DfgSel* ध्रुव rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    यदि (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        यदि (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                ध्रुव uint32_t width = lSelp->width() + rlSelp->width();
                                ध्रुव DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ध्रुव selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                प्रत्यागम;
                            }
                        }
                    }
                }
            }
        }

        यदि (DfgSel* ध्रुव rSelp = rhsp->cast<DfgSel>()) {
            यदि (DfgConcat* ध्रुव lConcatp = lhsp->cast<DfgConcat>()) {
                यदि (DfgSel* ध्रुव lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    यदि (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        यदि (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                ध्रुव uint32_t width = lrSelp->width() + rSelp->width();
                                ध्रुव DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ध्रुव selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                प्रत्यागम;
                            }
                        }
                    }
                }
            }
        }

        यदि (DfgConst* ध्रुव lConstp = lhsp->cast<DfgConst>()) {
            यदि (DfgCond* ध्रुव rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* ध्रुव rtVtxp = rCondp->thenp();
                DfgVertex* ध्रुव reVtxp = rCondp->elsep();
                DfgConst* ध्रुव rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ध्रुव reConstp = reVtxp->cast<DfgConst>();
                यदि (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(ढकला_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* ध्रुव thenp = [&]() -> DfgVertex* {
                            File_ओळ* ध्रुव rtFlp = rtVtxp->fileline();
                            यदि (rtConstp) {
                                DfgConst* ध्रुव constp = makeZero(rtFlp, vtxp->width());
                                constp->अंक().opConcat(lConstp->अंक(), rtConstp->अंक());
                                प्रत्यागम constp;
                            }
                            प्रत्यागम make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* ध्रुव elsep = [&]() -> DfgVertex* {
                            File_ओळ* ध्रुव reFlp = reVtxp->fileline();
                            यदि (reConstp) {
                                DfgConst* ध्रुव constp = makeZero(reFlp, vtxp->width());
                                constp->अंक().opConcat(lConstp->अंक(), reConstp->अंक());
                                प्रत्यागम constp;
                            }
                            प्रत्यागम make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (DfgConst* ध्रुव rConstp = rhsp->cast<DfgConst>()) {
            यदि (DfgCond* ध्रुव lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* ध्रुव ltVtxp = lCondp->thenp();
                DfgVertex* ध्रुव leVtxp = lCondp->elsep();
                DfgConst* ध्रुव ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ध्रुव leConstp = leVtxp->cast<DfgConst>();
                यदि (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(ढकला_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* ध्रुव thenp = [&]() -> DfgVertex* {
                            File_ओळ* ध्रुव ltFlp = ltVtxp->fileline();
                            यदि (ltConstp) {
                                DfgConst* ध्रुव constp = makeZero(ltFlp, vtxp->width());
                                constp->अंक().opConcat(ltConstp->अंक(), rConstp->अंक());
                                प्रत्यागम constp;
                            }
                            प्रत्यागम make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* ध्रुव elsep = [&]() -> DfgVertex* {
                            File_ओळ* ध्रुव leFlp = leVtxp->fileline();
                            यदि (leConstp) {
                                DfgConst* ध्रुव constp = makeZero(leFlp, vtxp->width());
                                constp->अंक().opConcat(leConstp->अंक(), rConstp->अंक());
                                प्रत्यागम constp;
                            }
                            प्रत्यागम make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        प्रत्यागम;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ध्रुव uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            ध्रुव uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            ध्रुव uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            तार्किक hasCrossSink = असत्य;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                यदि (ध्रुव DfgSel* ध्रुव selp = sink.cast<DfgSel>()) {
                    ध्रुव uint32_t selLsb = selp->lsb();
                    ध्रुव uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    प्रत्यागम असत्य;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                यदि (ध्रुव DfgVarPacked* ध्रुव varp = sink.cast<DfgVarPacked>()) {
                    यदि (!varp->hasSinks() && !varp->isObserved()) प्रत्यागम असत्य;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                प्रत्यागम सत्य;
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
                    DfgVertex* ध्रुव catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ध्रुव sp = नूतन DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* ध्रुव varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(सत्य);
                    varp->srcp(sp);
                    replace(varp);
                    प्रत्यागम;
                }
            }
        }

        // Convert various forms to REPLICATE
        यदि (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                प्रत्यागम;
            }
        }
        यदि (DfgRep* ध्रुव lRepp = lhsp->cast<DfgRep>()) {
            यदि (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    प्रत्यागम;
                }
            }
        }
        यदि (DfgRep* ध्रुव rRepp = rhsp->cast<DfgRep>()) {
            यदि (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    प्रत्यागम;
                }
            }
        }
        यदि (DfgRep* ध्रुव lRepp = lhsp->cast<DfgRep>()) {
            यदि (DfgRep* ध्रुव rRepp = rhsp->cast<DfgRep>()) {
                यदि (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        प्रत्यागम;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            यदि (DfgConcat* ध्रुव rConcatp = rhsp->cast<DfgConcat>()) {
                यदि (DfgRep* ध्रुव rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    यदि (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ध्रुव uint32_t width = lRepp->width() + rlRepp->width();
                            ध्रुव DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* ध्रुव repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            प्रत्यागम;
                        }
                    }
                }
            }
        }
    }

    शून्य visit(DfgDiv* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        यदि (DfgConst* ध्रुव rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            यदि (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgDivS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        यदि (DfgConst* ध्रुव rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            यदि (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgGt* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgGtS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgGte* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgGteS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgLogAnd* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp) || vtxp->rhsp()->unsafe()) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();

        यदि (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgLogEq* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgLogIf* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp) || vtxp->rhsp()->unsafe()) प्रत्यागम;
    }

    शून्य visit(DfgLogOr* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp) || vtxp->rhsp()->unsafe()) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();

        यदि (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgLt* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgLtS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgLte* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgLteS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                प्रत्यागम;
            }
        }
    }

    शून्य visit(DfgModDiv* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgModDivS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgMul* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (DfgConst* ध्रुव lConstp = lhsp->cast<DfgConst>()) {
            यदि (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    प्रत्यागम;
                }
            }
            यदि (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgMulS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (DfgConst* ध्रुव lConstp = lhsp->cast<DfgConst>()) {
            यदि (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    प्रत्यागम;
                }
            }
            यदि (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgEq* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                प्रत्यागम;
            }
        }

        यदि (DfgConst* ध्रुव lhsConstp = lhsp->cast<DfgConst>()) {
            यदि (DfgConcat* ध्रुव rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_ढकला_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) प्रत्यागम;
            }

            यदि (rhsp->dtype() == m_bitDType) {
                यदि (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        प्रत्यागम;
                    }
                }

                यदि (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        प्रत्यागम;
                    }
                }
            }
        }
    }

    शून्य visit(DfgNeq* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                प्रत्यागम;
            }
        }

        यदि (DfgConst* ध्रुव lhsConstp = lhsp->cast<DfgConst>()) {
            यदि (DfgConcat* ध्रुव rhsConcatp = rhsp->cast<DfgConcat>()) {
                यदि (try_ढकला_Compare_Op_Through_Concat(vtxp, lhsConstp, rhsConcatp)) प्रत्यागम;
            }

            यदि (rhsp->dtype() == m_bitDType) {
                यदि (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        प्रत्यागम;
                    }
                }

                यदि (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        प्रत्यागम;
                    }
                }
            }
        }
    }

    शून्य visit(DfgPow* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgPowSS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgPowSU* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgPowUS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgRep* ध्रुव vtxp) override {
        DfgVertex* ध्रुव srcp = vtxp->srcp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (DfgConst* ध्रुव sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ध्रुव resultp = makeZero(flp, vtxp->width());
                resultp->अंक().opRepl(sConstp->अंक(), vtxp->मोजणी());
                replace(resultp);
                प्रत्यागम;
            }
        }

        यदि (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                प्रत्यागम;
            }
        }

        यदि (DfgRep* ध्रुव sRepp = srcp->cast<DfgRep>()) {
            यदि (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgShiftL* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
        यदि (optimizeShiftRHS(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();

        यदि (DfgShiftL* ध्रुव lShiftLp = lhsp->cast<DfgShiftL>()) {
            यदि (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ओळ* ध्रुव flp = vtxp->fileline();
                    ध्रुव DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ध्रुव bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* ध्रुव cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ध्रुव addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    प्रत्यागम;
                }
            }
        }

        यदि (DfgConst* ध्रुव rConstp = rhsp->cast<DfgConst>()) {
            ध्रुव uint32_t shiftAmount = rConstp->toU32();

            यदि (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    प्रत्यागम;
                }
            }

            यदि (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    प्रत्यागम;
                }
            }

            यदि (DfgConcat* ध्रुव lConcatp = lhsp->cast<DfgConcat>()) {
                यदि (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* ध्रुव lRhsp = lConcatp->rhsp();
                        DfgVertex* ध्रुव lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ध्रुव uint32_t rW = shiftAmount;
                        ध्रुव uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        ध्रुव uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        File_ओळ* ध्रुव flp = lConcatp->fileline();
                        DfgVertex* ध्रुव rp = makeZero(flp, shiftAmount);
                        DfgVertex* ध्रुव mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        यदि (!lW) {
                            replace(np);
                            प्रत्यागम;
                        }
                        DfgVertex* ध्रुव lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        प्रत्यागम;
                    }
                }
            }

            यदि (DfgSel* ध्रुव lSelp = lhsp->cast<DfgSel>()) {
                यदि (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ध्रुव uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ध्रुव nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        प्रत्यागम;
                    }
                }
            }

            यदि (DfgCond* ध्रुव lCondp = lhsp->cast<DfgCond>()) {
                यदि (!lCondp->hasMultipleSinks()) {
                    APPLYING(ढकला_SHIFTL_THROUGH_COND) {
                        DfgShiftL* ध्रुव tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* ध्रुव ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        प्रत्यागम;
                    }
                }
            }
        }
    }

    शून्य visit(DfgShiftR* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
        यदि (optimizeShiftRHS(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();

        यदि (DfgShiftR* ध्रुव lShiftRp = lhsp->cast<DfgShiftR>()) {
            यदि (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    File_ओळ* ध्रुव flp = vtxp->fileline();
                    ध्रुव DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ध्रुव bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* ध्रुव cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ध्रुव addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    प्रत्यागम;
                }
            }
        }

        यदि (DfgConst* ध्रुव rConstp = rhsp->cast<DfgConst>()) {
            ध्रुव uint32_t shiftAmount = rConstp->toU32();

            यदि (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    प्रत्यागम;
                }
            }

            यदि (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    प्रत्यागम;
                }
            }

            यदि (DfgConcat* ध्रुव lConcatp = lhsp->cast<DfgConcat>()) {
                यदि (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* ध्रुव lRhsp = lConcatp->rhsp();
                        DfgVertex* ध्रुव lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ध्रुव uint32_t lW = shiftAmount;
                        ध्रुव uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        ध्रुव uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        File_ओळ* ध्रुव flp = lConcatp->fileline();
                        DfgVertex* ध्रुव lp = makeZero(flp, shiftAmount);
                        DfgVertex* ध्रुव mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        यदि (!rW) {
                            replace(np);
                            प्रत्यागम;
                        }
                        DfgVertex* ध्रुव rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        प्रत्यागम;
                    }
                }
            }

            यदि (DfgSel* ध्रुव lSelp = lhsp->cast<DfgSel>()) {
                यदि (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ध्रुव uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ध्रुव nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        प्रत्यागम;
                    }
                }
            }

            यदि (DfgCond* ध्रुव lCondp = lhsp->cast<DfgCond>()) {
                यदि (!lCondp->hasMultipleSinks()) {
                    APPLYING(ढकला_SHIFTR_THROUGH_COND) {
                        DfgShiftR* ध्रुव tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* ध्रुव ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        प्रत्यागम;
                    }
                }
            }
        }
    }

    शून्य visit(DfgShiftRS* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;
        यदि (optimizeShiftRHS(vtxp)) प्रत्यागम;
    }

    शून्य visit(DfgSub* ध्रुव vtxp) override {
        यदि (द्विमान(vtxp)) प्रत्यागम;

        DfgVertex* ध्रुव lhsp = vtxp->lhsp();
        DfgVertex* ध्रुव rhsp = vtxp->rhsp();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                प्रत्यागम;
            }
        }

        यदि (DfgConst* ध्रुव rConstp = rhsp->cast<DfgConst>()) {
            यदि (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    प्रत्यागम;
                }
            }
            यदि (vtxp->dtype() == m_bitDType && rConstp->has_मूल्य(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgMatchMasked* ध्रुव vtxp) override {
        यदि (DfgConst* ध्रुव constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ध्रुव matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->अंक(), matchVarp)));
                प्रत्यागम;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    शून्य visit(DfgCond* ध्रुव vtxp) override {
        DfgVertex* ध्रुव condp = vtxp->condp();
        DfgVertex* ध्रुव thenp = vtxp->thenp();
        DfgVertex* ध्रुव elsep = vtxp->elsep();
        File_ओळ* ध्रुव flp = vtxp->fileline();

        यदि (condp->dtype() != m_bitDType) प्रत्यागम;

        यदि (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                प्रत्यागम;
            }
        }

        यदि (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                प्रत्यागम;
            }
        }

        यदि (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                प्रत्यागम;
            }
        }

        यदि (DfgNot* ध्रुव condNotp = condp->cast<DfgNot>()) {
            यदि (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    प्रत्यागम;
                }
            }
        }

        यदि (DfgNeq* ध्रुव condNeqp = condp->cast<DfgNeq>()) {
            यदि (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ध्रुव newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    प्रत्यागम;
                }
            }
        }

        यदि (DfgNot* ध्रुव thenNotp = thenp->cast<DfgNot>()) {
            यदि (DfgNot* ध्रुव elseNotp = elsep->cast<DfgNot>()) {
                यदि (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ध्रुव newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (DfgOr* ध्रुव condOrp = condp->cast<DfgOr>()) {
            यदि (DfgCond* ध्रुव thenCondp = thenp->cast<DfgCond>()) {
                यदि (!thenCondp->hasMultipleSinks()) {
                    यदि (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ध्रुव ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            प्रत्यागम;
                        }
                    }
                    यदि (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ध्रुव ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            प्रत्यागम;
                        }
                    }
                }
            }
        }

        यदि (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            यदि (DfgAdd* ध्रुव thenAddp = thenp->cast<DfgAdd>()) {
                यदि (DfgConst* ध्रुव constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    यदि (constp->has_मूल्य(1)) {
                        यदि (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ध्रुव extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_ओळ* ध्रुव thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                प्रत्यागम;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            यदि (DfgSub* ध्रुव thenSubp = thenp->cast<DfgSub>()) {
                यदि (DfgConst* ध्रुव constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    यदि (constp->has_मूल्य(1)) {
                        यदि (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ध्रुव extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                File_ओळ* ध्रुव thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                प्रत्यागम;
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
                    प्रत्यागम;
                }
            }
            यदि (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    प्रत्यागम;
                }
            }
        }

        यदि (vtxp->width() <= VL_QUADSIZE) {
            यदि (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* ध्रुव maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    प्रत्यागम;
                }
            }
            यदि (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* ध्रुव maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    प्रत्यागम;
                }
            }
            यदि (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* ध्रुव maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    प्रत्यागम;
                }
            }
            यदि (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* ध्रुव maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    प्रत्यागम;
                }
            }

            यदि (DfgOr* ध्रुव tOrp = thenp->cast<DfgOr>()) {
                यदि (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* ध्रुव maskp = replicate(vtxp, condp);
                        DfgAnd* ध्रुव andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        प्रत्यागम;
                    }
                }
                यदि (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* ध्रुव maskp = replicate(vtxp, condp);
                        DfgAnd* ध्रुव andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        प्रत्यागम;
                    }
                }
            }
        }

        यदि (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ध्रुव std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            यदि (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ध्रुव uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    यदि (rWidth != rhsp->width()) {
                        ध्रुव DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    यदि (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        प्रत्यागम;
                    }
                    // Create new LHS
                    ध्रुव uint32_t lWidth = vtxp->width() - rWidth;
                    ध्रुव DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* ध्रुव lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* ध्रुव lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* ध्रुव lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    प्रत्यागम;
                }
            }

            ध्रुव std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            यदि (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ध्रुव uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    यदि (lWidth != lhsp->width()) {
                        ध्रुव DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    यदि (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        प्रत्यागम;
                    }
                    // Create new RHS
                    ध्रुव uint32_t rWidth = vtxp->width() - lWidth;
                    ध्रुव DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* ध्रुव rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* ध्रुव rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* ध्रुव rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    प्रत्यागम;
                }
            }
        }

        यदि (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                यदि (ध्रुव uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ध्रुव zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                प्रत्यागम;
            }
        }

        यदि (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                यदि (ध्रुव uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ध्रुव zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                प्रत्यागम;
            }
        }

        यदि (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                प्रत्यागम;
            }
        }

        यदि (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                प्रत्यागम;
            }
        }

        यदि (DfgCond* ध्रुव tCondp = thenp->cast<DfgCond>()) {
            यदि (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    प्रत्यागम;
                }
            }
        }

        यदि (DfgCond* ध्रुव eCondp = elsep->cast<DfgCond>()) {
            यदि (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    प्रत्यागम;
                }
            }
        }
    }

    शून्य visit(DfgVertexVar* ध्रुव vtxp) override {
        यदि (vtxp->hasSinks()) प्रत्यागम;
        यदि (vtxp->isObserved()) प्रत्यागम;
        यदि (vtxp->defaultp()) प्रत्यागम;

        // If undriven, or driven from another var, it is completely redundant.
        यदि (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                प्रत्यागम;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        तार्किक foundOne = असत्य;
        ध्रुव तार्किक keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> तार्किक {
            // Ignore non-observable variable sinks. These can be eliminated.
            यदि (ध्रुव DfgVertexVar* ध्रुव varp = sink.cast<DfgVertexVar>()) {
                यदि (!varp->hasSinks() && !varp->isObserved()) प्रत्यागम असत्य;
            }
            // Keep if found more than one sink
            यदि (foundOne) प्रत्यागम सत्य;
            // Mark first sink found
            foundOne = सत्य;
            प्रत्यागम असत्य;
        });
        यदि (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                प्रत्यागम;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        प्रति (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.ढकला_back(nullptr);
        m_iterList.ढकला_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        प्रति (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        प्रति (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        यावत् (सत्य) {
            // Process the work list - keep the placeholder at index 0
            यावत् (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.काढा_back();
                यदि (!m_vtxp) अनुवर्त;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                यदि (DfgVertexVar* ध्रुव varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    अनुवर्त;
                }

                // Ast references are not considered
                यदि (m_vtxp->is<DfgVertexAst>()) अनुवर्त;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                यदि (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    यदि (DfgVertex* ध्रुव sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            अनुवर्त;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            यदि (m_iterList.size() == 1) भङ्ग;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                प्रति (size_t hops = 1; hops <= 4; ++hops) {
                    प्रति (size_t i = begin; i < end; ++i) {
                        DfgVertex* ध्रुव vtxp = m_iterList[i];
                        यदि (!vtxp) अनुवर्त;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            प्रत्यागम असत्य;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            प्रत्यागम असत्य;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            प्रति (DfgVertex* ध्रुव vtxp : m_iterList) {
                यदि (!vtxp) अनुवर्त;
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
    यदि (!v3Global.opt.fDfgPeephole()) प्रत्यागम;
    V3DfgPeephole::apply(dfg, ctx);
}
