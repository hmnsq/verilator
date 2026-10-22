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

template <typename T_Reduction>
بنية ReductionToBitwiseImpl {};
template <> بنية ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> بنية ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> بنية ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
بنية BitwiseToReductionImpl {};
template <> بنية BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> بنية BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> بنية BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
بنية IsAssociative : عام std::false_type {};
template <> بنية IsAssociative<DfgAnd> : عام std::true_type {};
template <> بنية IsAssociative<DfgOr>  : عام std::true_type {};
template <> بنية IsAssociative<DfgXor> : عام std::true_type {};
template <> بنية IsAssociative<DfgAdd> : عام std::true_type {};
template <> بنية IsAssociative<DfgConcat> : عام std::true_type {};
template <> بنية IsAssociative<DfgMul> : عام std::true_type {};
template <> بنية IsAssociative<DfgMulS> : عام std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
بنية IsCommutative : عام std::false_type {};
template <> بنية IsCommutative<DfgAnd> : عام std::true_type {};
template <> بنية IsCommutative<DfgOr>  : عام std::true_type {};
template <> بنية IsCommutative<DfgXor> : عام std::true_type {};
template <> بنية IsCommutative<DfgAdd> : عام std::true_type {};
template <> بنية IsCommutative<DfgEq> : عام std::true_type {};
template <> بنية IsCommutative<DfgMul> : عام std::true_type {};
template <> بنية IsCommutative<DfgMulS> : عام std::true_type {};
template <> بنية IsCommutative<DfgNeq> : عام std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
بنية IsIdempotent : عام std::false_type {};
template <> بنية IsIdempotent<DfgAnd> : عام std::true_type {};
template <> بنية IsIdempotent<DfgOr>  : عام std::true_type {};

// Binary result dtype
template<typename Vertex>
ثابت DfgDataType& resultDType(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp);
template <> ثابت DfgDataType& resultDType<DfgAdd>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgAnd>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgConcat>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> ثابت DfgDataType& resultDType<DfgDiv>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgDivS>       (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgEq>         (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgGt>         (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgGtS>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgGte>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgGteS>       (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLogAnd>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLogEq>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLogIf>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLogOr>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLt>         (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLtS>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLte>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgLteS>       (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgModDiv>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgModDivS>    (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgMul>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgMulS>       (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgNeq>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ DfgDataType::packed(1); }
template <> ثابت DfgDataType& resultDType<DfgOr>         (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgPow>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgPowSS>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgPowSU>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgPowUS>      (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgShiftL>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgShiftR>     (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgShiftRS>    (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgSub>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }
template <> ثابت DfgDataType& resultDType<DfgXor>        (ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) { ܦܢܐ lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> فراغ foldOp(V3Number& out, ثابت V3Number& src);
template <> فراغ foldOp<DfgCountOnes>  (V3Number& out, ثابت V3Number& src) { out.opCountOnes(src); }
template <> فراغ foldOp<DfgExtend>     (V3Number& out, ثابت V3Number& src) { out.opAssign(src); }
template <> فراغ foldOp<DfgExtendS>    (V3Number& out, ثابت V3Number& src) { out.opExtendS(src, src.width()); }
template <> فراغ foldOp<DfgLogNot>     (V3Number& out, ثابت V3Number& src) { out.opLogNot(src); }
template <> فراغ foldOp<DfgNegate>     (V3Number& out, ثابت V3Number& src) { out.opNegate(src); }
template <> فراغ foldOp<DfgNot>        (V3Number& out, ثابت V3Number& src) { out.opNot(src); }
template <> فراغ foldOp<DfgOneHot>     (V3Number& out, ثابت V3Number& src) { out.opOneHot(src); }
template <> فراغ foldOp<DfgOneHot0>    (V3Number& out, ثابت V3Number& src) { out.opOneHot0(src); }
template <> فراغ foldOp<DfgRedAnd>     (V3Number& out, ثابت V3Number& src) { out.opRedAnd(src); }
template <> فراغ foldOp<DfgRedOr>      (V3Number& out, ثابت V3Number& src) { out.opRedOr(src); }
template <> فراغ foldOp<DfgRedXor>     (V3Number& out, ثابت V3Number& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> فراغ foldOp(V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs);
template <> فراغ foldOp<DfgAdd>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opAdd(lhs, rhs); }
template <> فراغ foldOp<DfgAnd>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opAnd(lhs, rhs); }
template <> فراغ foldOp<DfgConcat>     (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opConcat(lhs, rhs); }
template <> فراغ foldOp<DfgDiv>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opDiv(lhs, rhs); }
template <> فراغ foldOp<DfgDivS>       (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opDivS(lhs, rhs); }
template <> فراغ foldOp<DfgEq>         (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opEq(lhs, rhs); }
template <> فراغ foldOp<DfgGt>         (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opGt(lhs, rhs); }
template <> فراغ foldOp<DfgGtS>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opGtS(lhs, rhs); }
template <> فراغ foldOp<DfgGte>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opGte(lhs, rhs); }
template <> فراغ foldOp<DfgGteS>       (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opGteS(lhs, rhs); }
template <> فراغ foldOp<DfgLogAnd>     (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opLogAnd(lhs, rhs); }
template <> فراغ foldOp<DfgLogEq>      (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opLogEq(lhs, rhs); }
template <> فراغ foldOp<DfgLogIf>      (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opLogIf(lhs, rhs); }
template <> فراغ foldOp<DfgLogOr>      (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opLogOr(lhs, rhs); }
template <> فراغ foldOp<DfgLt>         (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opLt(lhs, rhs); }
template <> فراغ foldOp<DfgLtS>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opLtS(lhs, rhs); }
template <> فراغ foldOp<DfgLte>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opLte(lhs, rhs); }
template <> فراغ foldOp<DfgLteS>       (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opLteS(lhs, rhs); }
template <> فراغ foldOp<DfgModDiv>     (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opModDiv(lhs, rhs); }
template <> فراغ foldOp<DfgModDivS>    (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opModDivS(lhs, rhs); }
template <> فراغ foldOp<DfgMul>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opMul(lhs, rhs); }
template <> فراغ foldOp<DfgMulS>       (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opMulS(lhs, rhs); }
template <> فراغ foldOp<DfgNeq>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opNeq(lhs, rhs); }
template <> فراغ foldOp<DfgOr>         (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opOr(lhs, rhs); }
template <> فراغ foldOp<DfgPow>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opPow(lhs, rhs); }
template <> فراغ foldOp<DfgPowSS>      (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opPowSS(lhs, rhs); }
template <> فراغ foldOp<DfgPowSU>      (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opPowSU(lhs, rhs); }
template <> فراغ foldOp<DfgPowUS>      (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opPowUS(lhs, rhs); }
template <> فراغ foldOp<DfgShiftL>     (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opShiftL(lhs, rhs); }
template <> فراغ foldOp<DfgShiftR>     (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opShiftR(lhs, rhs); }
template <> فراغ foldOp<DfgShiftRS>    (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> فراغ foldOp<DfgSub>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opSub(lhs, rhs); }
template <> فراغ foldOp<DfgXor>        (V3Number& out, ثابت V3Number& lhs, ثابت V3Number& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

صنف V3DfgPeephole final : عام DfgVisitor {
    // TYPES
    بنية VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        منطقي m_isCachedVertex = ܕܓܠܐ;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    ثابت DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* ثابت m_tmpScopep = v3Global.rootp()->topScopep()->scopep();

    // STATIC STATE
    ساكن V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) ܐܢ (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    منطقي checkApplying(VDfgPeepholePattern id) {
        ܐܢ (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ܦܢܐ ܕܓܠܐ;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_count[id];
        ܦܢܐ ܫܪܪܐ;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* ثابت equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        ܦܢܐ equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    فراغ invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = ܕܓܠܐ;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(FileLine* flp, ثابت DfgDataType& dtype, Operands... operands) {
        Vertex* ثابت vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = ܫܪܪܐ;
        ܦܢܐ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(ثابت DfgDataType& dtype, Operands... operands) {
        Vertex* ثابت vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ܦܢܐ vtxp;
    }

    فراغ incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    فراغ addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        ܐܢ (vInfo.m_workListIndex) ܦܢܐ;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.push_back(vtxp);
    }

    فراغ removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    فراغ addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        ܐܢ (vInfo.m_iterListIndex) ܦܢܐ;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.push_back(vtxp);
    }

    فراغ removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    فراغ deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        ثابت DfgVertexVar* ثابت varp = vtxp->cast<DfgVertexVar>();
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
            ܐܢ (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                ܦܢܐ ܕܓܠܐ;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            ܐܢ (vInfo.m_generation != m_currentGeneration) srcps.push_back(&src);
            vInfo.m_generation = m_currentGeneration;
            ܦܢܐ ܕܓܠܐ;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        ܐܢ (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.push_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } ܐܚܪܢܐ {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        ثابت auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  ܦܢܐ srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        لكل (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        لكل (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    فراغ replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        ثابت auto debugCallback = [&]() -> فراغ {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            ثابت auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](ثابت DfgVertex& v) {  //
                ܦܢܐ cone->count(&v);
            });
        };
        ܐܢ (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ܦܢܐ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            ܦܢܐ ܕܓܠܐ;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            ܦܢܐ ܕܓܠܐ;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            ܦܢܐ ܕܓܠܐ;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            ܦܢܐ ܕܓܠܐ;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(FileLine* flp, uint32_t val) { ܦܢܐ جديد DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(FileLine* flp, uint32_t width) {
        ܦܢܐ جديد DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(FileLine* flp, ثابت DfgDataType& dtype) {
        ܦܢܐ makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(FileLine* flp, uint32_t width) {
        DfgConst* ثابت resp = makeZero(flp, width);
        resp->num().setAllBits1();
        ܦܢܐ resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(FileLine* flp, ثابت DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* ثابت vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        ܐܢ (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        ܐܢ (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        ܦܢܐ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(ثابت DfgVertex* examplep, Operands... operands) {
        ܦܢܐ make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        ܐܢ (vtxp->dtype() == m_bitDType) ܦܢܐ bitp;
        ܦܢܐ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    ساكن منطقي isSame(ثابت DfgVertex* ap, ثابت DfgVertex* bp) {
        ܐܢ (ap == bp) ܦܢܐ ܫܪܪܐ;
        ܐܢ (ap->dtype() != bp->dtype()) ܦܢܐ ܕܓܠܐ;
        ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>();
        ܐܢ (!aConstp) ܦܢܐ ܕܓܠܐ;
        ثابت DfgConst* ثابت bConstp = bp->cast<DfgConst>();
        ܐܢ (!bConstp) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ aConstp->num().isCaseEq(bConstp->num());
    }

    ساكن منطقي isZero(ثابت DfgVertex* vtxp) {
        ܐܢ (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) ܦܢܐ constp->isZero();
        ܦܢܐ ܕܓܠܐ;
    }

    ساكن منطقي isOnes(ثابت DfgVertex* vtxp) {
        ܐܢ (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) ܦܢܐ constp->isOnes();
        ܦܢܐ ܕܓܠܐ;
    }

    ساكن منطقي isEqOne(ثابت DfgVertex* vtxp) {
        ܐܢ (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) ܦܢܐ constp->num().isEqOne();
        ܦܢܐ ܕܓܠܐ;
    }

    ساكن منطقي areAdjacent(uint32_t& lsb, ثابت DfgSel* lSelp, ثابت DfgSel* rSelp) {
        ܐܢ (!isSame(lSelp->srcp(), rSelp->srcp())) ܦܢܐ ܕܓܠܐ;
        ܐܢ (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ܦܢܐ ܫܪܪܐ;
        }
        ܐܢ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            ܦܢܐ ܫܪܪܐ;
        }
        ܦܢܐ ܕܓܠܐ;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي foldUnary(Vertex* ثابت vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::value, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::value, "Must invoke on final class");
        ܐܢ (DfgConst* ثابت srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* ثابت resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->num(), srcp->num());
                replace(resultp);
                ܦܢܐ ܫܪܪܐ;
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي binary(Vertex* ثابت vtxp) {
        static_assert(std::is_base_of<DfgVertexBinary, Vertex>::value, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::value, "Must invoke on final class");

        FileLine* ثابت flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* ثابت lConstp = lhsp->cast<DfgConst>();
        DfgConst* ثابت rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* ثابت lSamep = lhsp->cast<Vertex>();
        Vertex* ثابت rSamep = rhsp->cast<Vertex>();

        // Constant folding
        ܐܢ (lConstp && rConstp) {
            APPLYING(FOLD_BINARY) {
                DfgConst* ثابت resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->num(), lConstp->num(), rConstp->num());
                replace(resultp);
                ܦܢܐ ܫܪܪܐ;
            }
        }

        // Idempotent
        ܐܢ VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::value) {
            ܐܢ (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_BINARY) {
                    replace(lhsp);
                    ܦܢܐ ܫܪܪܐ;
                }
            }
        }

        // Associative
        ܐܢ VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::value) {
            // Also commutative and idempotent
            ܐܢ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value && IsIdempotent<Vertex>::value) {
                ܐܢ (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_BINARY_LHS) {
                        replace(lSamep);
                        ܦܢܐ ܫܪܪܐ;
                    }
                }
                ܐܢ (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_BINARY_RHS) {
                        replace(rSamep);
                        ܦܢܐ ܫܪܪܐ;
                    }
                }
            }

            ܐܢ (lConstp && rSamep) {
                ܐܢ (DfgConst* ثابت rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_BINARY_LHS_OF_RHS) {
                        DfgConst* ثابت cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->num(), lConstp->num(), rlConstp->num());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        ܦܢܐ ܫܪܪܐ;
                    }
                }
            }

            ܐܢ (lSamep && rConstp) {
                ܐܢ (DfgConst* ثابت lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_BINARY_RHS_OF_LHS) {
                        DfgConst* ثابت cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->num(), lrConstp->num(), rConstp->num());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        ܦܢܐ ܫܪܪܐ;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            ܐܢ (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* ثابت ap = lSamep->lhsp();
                DfgVertex* ثابت bp = lSamep->rhsp();
                DfgVertex* ثابت cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                ܐܢ (ap != cp && bp != cp) {
                    APPLYING(RIGHT_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        ثابت DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* ثابت bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ܦܢܐ ܫܪܪܐ;
                    }
                }
            }

            ܐܢ (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* ثابت rlVtxp = rSamep->lhsp();
                DfgVertex* ثابت rrVtxp = rSamep->rhsp();

                ܐܢ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
                    ܐܢ (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            ܦܢܐ ܫܪܪܐ;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                ܐܢ (Vertex* ثابت existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    ܐܢ (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_BINARY_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            ܦܢܐ ܫܪܪܐ;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                ܐܢ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
                    ܐܢ (Vertex* ثابت existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        ܐܢ (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_BINARY_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                ܦܢܐ ܫܪܪܐ;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        ܐܢ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            ܐܢ (lConstp) {
                ܐܢ (DfgCond* ثابت rCondp = rhsp->cast<DfgCond>()) {
                    ܐܢ (!rCondp->hasMultipleSinks()) {
                        APPLYING(PUSH_COMMUTATIVE_BINARY_THROUGH_COND) {
                            DfgVertex* ثابت tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* ثابت ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ܦܢܐ ܫܪܪܐ;
                        }
                    }
                }
                ܦܢܐ ܕܓܠܐ;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        ثابت منطقي swapSides = [&]() {
            // Don't swap if not commutative, duh
            ܐܢ VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::value) ܦܢܐ ܕܓܠܐ;
            // Const go on left
            ܐܢ (lConstp) ܦܢܐ ܕܓܠܐ;
            ܐܢ (rConstp) ܦܢܐ ܫܪܪܐ;
            // Not go on left
            ܐܢ (lhsp->is<DfgNot>()) ܦܢܐ ܕܓܠܐ;
            ܐܢ (rhsp->is<DfgNot>()) ܦܢܐ ܫܪܪܐ;
            // Same go on right
            ܐܢ (rSamep) ܦܢܐ ܕܓܠܐ;
            ܐܢ (lSamep) ܦܢܐ ܫܪܪܐ;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            ثابت VertexInfo& lInfo = m_vInfo[lhsp];
            ثابت VertexInfo& rInfo = m_vInfo[rhsp];
            ܦܢܐ lInfo.m_id > rInfo.m_id;
        }();
        ܐܢ (swapSides) {
            APPLYING(SWAP_SIDES_IN_BINARY) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                ܦܢܐ ܫܪܪܐ;
            }
        }

        ܦܢܐ ܕܓܠܐ;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي distributiveAndAssociativeBinary(Vertex* vtxp) {
        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        ܐܢ (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            ܐܢ (Other* ثابت lp = lhsp->cast<Other>()) {
                ܐܢ (Other* ثابت rp = rhsp->cast<Other>()) {
                    DfgVertex* ثابت llp = lp->lhsp();
                    DfgVertex* ثابت lrp = lp->rhsp();
                    DfgVertex* ثابت rlp = rp->lhsp();
                    DfgVertex* ثابت rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    ܐܢ (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } ܐܚܪܢܐ ܐܢ (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } ܐܚܪܢܐ ܐܢ (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } ܐܚܪܢܐ ܐܢ (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    ܐܢ (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_BINARY) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            ܦܢܐ ܫܪܪܐ;
                        }
                    }
                }
            }
        }

        ܦܢܐ ܕܓܠܐ;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي
    tryPushBitwiseOpThroughConcat(Vertex* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        FileLine* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* ثابت catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* ثابت catRConstp = concatp->rhsp()->cast<DfgConst>();
        ܐܢ (catLConstp || catRConstp) {
            APPLYING(PUSH_BITWISE_OP_THROUGH_CONCAT) {
                ثابت uint32_t width = concatp->width();
                ثابت DfgDataType& lDtype = concatp->lhsp()->dtype();
                ثابت DfgDataType& rDtype = concatp->rhsp()->dtype();
                ثابت uint32_t lWidth = lDtype.size();
                ثابت uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* ثابت newLhsp = [&]() -> DfgVertex* {
                    DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), lWidth);
                    ܐܢ (catLConstp) {
                        V3Number num{constp->fileline(), static_cast<صحيح>(lWidth), 0u};
                        num.opSel(constp->num(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->num(), num, catLConstp->num());
                        ܦܢܐ newLhsConstp;
                    }
                    newLhsConstp->num().opSel(constp->num(), width - 1, rWidth);
                    ܦܢܐ make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* ثابت newRhsp = [&]() -> DfgVertex* {
                    DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), rWidth);
                    ܐܢ (catRConstp) {
                        V3Number num{constp->fileline(), static_cast<صحيح>(rWidth), 0u};
                        num.opSel(constp->num(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->num(), num, catRConstp->num());
                        ܦܢܐ newRhsConstp;
                    }
                    newRhsConstp->num().opSel(constp->num(), rWidth - 1, 0);
                    ܦܢܐ make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ܦܢܐ ܫܪܪܐ;
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT منطقي
    tryPushCompareOpThroughConcat(Vertex* ثابت vtxp, DfgConst* constp, DfgConcat* concatp) {
        FileLine* ثابت flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        ܐܢ (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(PUSH_COMPARE_OP_THROUGH_CONCAT) {
                ثابت uint32_t width = concatp->width();
                ثابت uint32_t lWidth = concatp->lhsp()->width();
                ثابت uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* ثابت newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->num().opSel(constp->num(), width - 1, rWidth);
                Vertex* ثابت newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* ثابت newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->num().opSel(constp->num(), rWidth - 1, 0);
                Vertex* ثابت newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                ܐܢ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::value) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ܐܚܪܢܐ ܐܢ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::value) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ܐܚܪܢܐ {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ܦܢܐ ܫܪܪܐ;
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT منطقي tryPushBitwiseOpThroughReductions(Bitwise* ثابت vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        ܐܢ (Reduction* ثابت lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            ܐܢ (Reduction* ثابت rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* ثابت lSrcp = lRedp->srcp();
                DfgVertex* ثابت rSrcp = rRedp->srcp();
                ܐܢ (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(PUSH_BITWISE_THROUGH_REDUCTION) {
                        FileLine* ثابت flp = vtxp->fileline();
                        Bitwise* ثابت bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        ܦܢܐ ܫܪܪܐ;
                    }
                }
            }
        }

        ܦܢܐ ܕܓܠܐ;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    ساكن std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        ܐܢ (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ܦܢܐ {nullptr, nullptr};
        }
        ܐܢ (vtxp->hasMultipleSinks()) ܦܢܐ {nullptr, nullptr};
        DfgVertexBinary* ثابت bwp = vtxp->as<DfgVertexBinary>();
        DfgSel* ثابت lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* ثابت rSelp = bwp->rhsp()->cast<DfgSel>();
        ܐܢ (!lSelp || !rSelp) ܦܢܐ {nullptr, nullptr};
        DfgSel* ثابت nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* ثابت wSelp = nSelp == lSelp ? rSelp : lSelp;
        ثابت uint32_t nWidth = nSelp->fromp()->width();
        ܐܢ (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ܦܢܐ {nullptr, nullptr};
        ܐܢ (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            ܦܢܐ {nullptr, nullptr};
        }
        ܦܢܐ {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT منطقي tryPushBitwiseOpThrougSel(Bitwise* ثابت vtxp) {
        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        ܐܢ (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ܐܢ (DfgSel* ثابت selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } ܐܚܪܢܐ ܐܢ (Bitwise* ثابت bitwisep = rhsp->cast<Bitwise>()) {
                ܐܢ (DfgSel* ثابت rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } ܐܚܪܢܐ ܐܢ (DfgSel* ثابت rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            ܐܢ (rSelp) {
                DfgVertex* ثابت lFromp = lSelp->fromp();
                DfgVertex* ثابت rFromp = rSelp->fromp();
                ܐܢ (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(PUSH_BITWISE_THROUGH_SEL) {
                        Bitwise* ثابت bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        ܐܢ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ܦܢܐ ܫܪܪܐ;
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
            ܐܢ (rChainp && rChainp->hasMultipleSinks()) ܦܢܐ ܕܓܠܐ;
            DfgVertex* ثابت rTermp = rChainp ? rChainp->lhsp() : rhsp;

            ثابت auto lSels = pushableUnalignedSels(lhsp);
            ثابت auto rSels = pushableUnalignedSels(rTermp);
            ܐܢ (!lSels.first && !rSels.first) ܦܢܐ ܕܓܠܐ;

            ثابت DfgVertex* ثابت opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            ܐܢ (opp->is<Bitwise>()) ܦܢܐ ܕܓܠܐ;
            ثابت auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* ثابت nFromp = sels.first->fromp();
            DfgVertex* ثابت wFromp = sels.second->fromp();
            ثابت uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            ثابت auto matches = [&](ثابت DfgVertex* termp, ثابت auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                ܐܢ (tSels.first) {
                    ܦܢܐ termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                ܐܢ (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                ܐܢ (ثابت DfgSel* ثابت selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                ܐܢ (termp->type() != opp->type()) ܦܢܐ ܕܓܠܐ;
                // Operands might have been swapped since
                ثابت DfgVertexBinary* ثابت bwp = termp->as<DfgVertexBinary>();
                ثابت منطقي nIsLhs = bwp->lhsp() == nFromp;
                ܐܢ (!nIsLhs && bwp->rhsp() != nFromp) ܦܢܐ ܕܓܠܐ;
                ثابت DfgSel* ثابت slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ܦܢܐ slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            ܐܢ (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ܦܢܐ ܕܓܠܐ;

            APPLYING(PUSH_BITWISE_THROUGH_UNALIGNED_SEL) {
                FileLine* ثابت flp = vtxp->fileline();
                ثابت DfgDataType& dtype = nFromp->dtype();
                DfgSel* ثابت slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                ܐܢ (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } ܐܚܪܢܐ ܐܢ (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } ܐܚܪܢܐ {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                ثابت auto push = [&](DfgVertex* termp, ثابت DfgSel* nSelp) -> DfgVertex* {
                    ܦܢܐ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* ثابت newLhsp = push(lhsp, lSels.first);
                DfgVertex* ثابت newRTermp = push(rTermp, rSels.first);
                ܐܢ (rChainp) {
                    DfgVertex* ثابت newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } ܐܚܪܢܐ {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ܦܢܐ ܫܪܪܐ;
            }
        }

        ܦܢܐ ܕܓܠܐ;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    ساكن منطقي isConcatFoldable(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        ܐܢ (isSame(lhsp, rhsp)) ܦܢܐ ܫܪܪܐ;
        ܐܢ (ثابت DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            ܐܢ (isSame(lRepp->srcp(), rhsp)) ܦܢܐ ܫܪܪܐ;
        }
        ܐܢ (ثابت DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
            ܐܢ (isSame(lhsp, rRepp->srcp())) ܦܢܐ ܫܪܪܐ;
        }
        // Concatenation of adjoining selects is a select
        ܐܢ (ثابت DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            ܐܢ (ثابت DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                ܦܢܐ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        ܐܢ (ثابت DfgNot* ثابت lNotp = lhsp->cast<DfgNot>()) {
            ܐܢ (ثابت DfgNot* ثابت rNotp = rhsp->cast<DfgNot>()) {
                ܦܢܐ !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    ساكن منطقي isSameBitwise(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) {
        ܐܢ (lhsp->type() != rhsp->type()) ܦܢܐ ܕܓܠܐ;
        ܦܢܐ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    ساكن منطقي isConcatSimplifiable(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp) {
        ܐܢ (isConcatFoldable(lhsp, rhsp)) ܦܢܐ ܫܪܪܐ;
        ܐܢ (!isSameBitwise(lhsp, rhsp)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ܦܢܐ ܕܓܠܐ;
        ثابت DfgVertexBinary* ثابت lp = lhsp->as<DfgVertexBinary>();
        ثابت DfgVertexBinary* ثابت rp = rhsp->as<DfgVertexBinary>();
        ܐܢ (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ܦܢܐ ܫܪܪܐ;
        ܐܢ (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ܦܢܐ ܫܪܪܐ;
        ܦܢܐ ܕܓܠܐ;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    ساكن منطقي isConcatPushableThroughBitwise(ثابت DfgVertex* lhsp, ثابت DfgVertex* rhsp,
                                               منطقي& swap) {
        ܐܢ (!isSameBitwise(lhsp, rhsp)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ܦܢܐ ܕܓܠܐ;
        ثابت DfgVertexBinary* ثابت lp = lhsp->as<DfgVertexBinary>();
        ثابت DfgVertexBinary* ثابت rp = rhsp->as<DfgVertexBinary>();
        swap = ܕܓܠܐ;
        ܐܢ (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ܦܢܐ ܫܪܪܐ;
        ܐܢ (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ܦܢܐ ܫܪܪܐ;
        swap = ܫܪܪܐ;
        ܐܢ (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ܦܢܐ ܫܪܪܐ;
        ܐܢ (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ܦܢܐ ܫܪܪܐ;
        ܦܢܐ ܕܓܠܐ;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(FileLine* flp, ثابت DfgVertex* lhsp,
                                              ثابت DfgVertex* rhsp, منطقي swap) {
        ثابت DfgVertexBinary* ثابت lp = lhsp->as<DfgVertexBinary>();
        ثابت DfgVertexBinary* ثابت rp = rhsp->as<DfgVertexBinary>();
        ثابت DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* ثابت cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* ثابت dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* ثابت newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* ثابت newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        ܐܢ (lp->is<DfgAnd>()) ܦܢܐ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        ܐܢ (lp->is<DfgOr>()) ܦܢܐ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ܦܢܐ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT منطقي tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        ܐܢ (DfgSel* ثابت lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ܐܢ (DfgSel* ثابت selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } ܐܚܪܢܐ ܐܢ (Bitwise* ثابت rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } ܐܚܪܢܐ ܐܢ (Reduction* ثابت rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            ܐܢ (rSelp) {
                uint32_t lsb = 0;
                ܐܢ (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        ثابت DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* ثابت newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        ܐܢ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ܦܢܐ ܫܪܪܐ;
                    }
                }
            }
        }

        ܐܢ (Reduction* ثابت lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            ܐܢ (!rRedp) {
                ܐܢ (Bitwise* ثابت rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            ܐܢ (rRedp) {
                ܐܢ (DfgSel* ثابت lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    ܐܢ (DfgSel* ثابت rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ܐܢ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                ثابت DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                ܐܢ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ܦܢܐ ܫܪܪܐ;
                            }
                        }
                    }
                }
            }
        }

        ܦܢܐ ܕܓܠܐ;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT منطقي optimizeReduction(Reduction* ثابت vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        ܐܢ (foldUnary(vtxp)) ܦܢܐ ܫܪܪܐ;

        DfgVertex* ثابت srcp = vtxp->srcp();
        FileLine* ثابت flp = vtxp->fileline();

        // Reduction of 1-bit value
        ܐܢ (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                ܦܢܐ ܫܪܪܐ;
            }
        }

        ܐܢ (DfgCond* ثابت condp = srcp->cast<DfgCond>()) {
            ܐܢ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(PUSH_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* ثابت newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* ثابت newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ܦܢܐ ܫܪܪܐ;
                }
            }
        }

        ܐܢ (DfgConcat* ثابت concatp = srcp->cast<DfgConcat>()) {
            ܐܢ (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(PUSH_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* ثابت lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* ثابت rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    ܦܢܐ ܫܪܪܐ;
                }
            }
        }

        ܐܢ (Bitwise* ثابت bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            ܐܢ (!bitwisep->hasMultipleSinks()) {
                ܐܢ (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(PUSH_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* ثابت newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* ثابت newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        ܦܢܐ ܫܪܪܐ;
                    }
                }

                ܐܢ (DfgSel* ثابت lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    ܐܢ (DfgSel* ثابت rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ܐܢ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(PUSH_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                ثابت DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* ثابت newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ܦܢܐ ܫܪܪܐ;
                            }
                        }
                    }
                }
            }
        }

        ܦܢܐ ܕܓܠܐ;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT منطقي optimizeShiftRHS(Shift* ثابت vtxp) {
        static_assert(std::is_base_of<DfgVertexBinary, Shift>::value, "Must invoke on binary");
        static_assert(std::is_final<Shift>::value, "Must invoke on final class");
        ܐܢ (ثابت DfgConcat* ثابت concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            ܐܢ (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ܦܢܐ ܫܪܪܐ;
                }
            }
        }
        ܦܢܐ ܕܓܠܐ;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> addToCountOnesTerm(DfgVertex* vtxp) {
        ܐܢ (DfgConcat* ثابت oCatp = vtxp->cast<DfgConcat>()) {
            ܐܢ (isZero(oCatp->lhsp())) {
                ܐܢ (DfgCountOnes* ثابت countOnesp = oCatp->rhsp()->cast<DfgCountOnes>()) {
                    // Zero extended count ones
                    ܐܢ (DfgSel* ثابت selp = countOnesp->srcp()->cast<DfgSel>()) {
                        ܦܢܐ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } ܐܚܪܢܐ ܐܢ (DfgSel* ثابت selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    ܐܢ (selp->dtype() == m_bitDType) {  //
                        ܦܢܐ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            ܦܢܐ {nullptr, 0, 0};
        }
        ܐܢ (DfgCountOnes* ثابت countOnesp = vtxp->cast<DfgCountOnes>()) {
            // Simple count ones
            ܐܢ (DfgSel* ثابت selp = countOnesp->srcp()->cast<DfgSel>()) {
                ܦܢܐ {selp->fromp(), selp->lsb(), selp->width()};
            }
            ܦܢܐ {nullptr, 0, 0};
        }
        ܐܢ (DfgSel* ثابت oSelp = vtxp->cast<DfgSel>()) {
            ܐܢ (oSelp->lsb() == 0) {
                // Truncated count ones
                ܐܢ (DfgCountOnes* ثابت countOnesp = oSelp->fromp()->cast<DfgCountOnes>()) {
                    // Zero extended count ones
                    ܐܢ (DfgSel* ثابت selp = countOnesp->srcp()->cast<DfgSel>()) {
                        ܦܢܐ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            ܐܢ (oSelp->dtype() == m_bitDType) {  //
                ܦܢܐ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ܦܢܐ {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        ܐܢ (DfgShiftR* ثابت shiftrp = vtxp->cast<DfgShiftR>()) {
            ܐܢ (DfgConst* ثابت rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                ܐܢ (rConstp->toU32() == shiftrp->width() - 1) {
                    ܦܢܐ {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            ܦܢܐ {nullptr, 0, 0};
        }
        // Not applicable
        ܦܢܐ {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              ثابت uint32_t width) {
        ܟܡܐ (ܫܪܪܐ) {
            ثابت uint32_t msb = lsb + width - 1;

            // Sel from Concat
            ܐܢ (DfgConcat* ثابت concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* ثابت lhsp = concatp->lhsp();
                DfgVertex* ثابت rhsp = concatp->rhsp();

                ܐܢ (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        استمر;
                    }
                } ܐܚܪܢܐ ܐܢ (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        استمر;
                    }
                }
            }

            ܐܢ (DfgRep* ثابت repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                ثابت uint32_t srcWidth = repp->srcp()->width();
                ܐܢ (width <= srcWidth) {
                    ثابت uint32_t newLsb = lsb % srcWidth;
                    ثابت uint32_t newMsb = newLsb + width - 1;
                    ܐܢ (newMsb < srcWidth) {
                        APPLYING(PUSH_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            استمر;
                        }
                    }
                }
            }

            // Sel from Sel
            ܐܢ (DfgSel* ثابت selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    استمر;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            ܐܢ (DfgVarPacked* ثابت varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                ثابت auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* ثابت driverp = pair.first;
                ثابت uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                ܐܢ (driverp) {
                    ܐܢ (driverp == varp->defaultp()) {
                        APPLYING(PUSH_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            استمر;
                        }
                    } ܐܚܪܢܐ {
                        APPLYING(PUSH_SEL_THROUGH_SPLICE) {
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
        ܦܢܐ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        ܐܢ (ap == bp) ܦܢܐ {ap, ap->width()};

        // If both constants, check LSBs
        ܐܢ (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            ܐܢ (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت V3Number& aNum = aConstp->num();
                ثابت V3Number& bNum = bConstp->num();
                // Max match is the shorter constant
                ثابت uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                لكل (; matchWidth < maxMatch; ++matchWidth) {
                    ܐܢ (aNum.bitIs0(matchWidth) != bNum.bitIs0(matchWidth)) اكسر;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                ܦܢܐ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        ܐܢ (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) ܦܢܐ commonLSBs(catp->rhsp(), bp);
        ܐܢ (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) ܦܢܐ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        ܐܢ (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            ܐܢ (selp->lsb() == 0) {
                DfgVertex* ثابت fromp = selp->fromp();
                ثابت std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                ܦܢܐ {common.first, std::min(common.second, selp->width())};
            }
        }
        ܐܢ (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            ܐܢ (selp->lsb() == 0) {
                DfgVertex* ثابت fromp = selp->fromp();
                ثابت std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                ܦܢܐ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        ܦܢܐ {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        ܐܢ (ap == bp) ܦܢܐ {ap, ap->width()};

        // If both constants, check MSBs
        ܐܢ (DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
            ܐܢ (DfgConst* ثابت bConstp = bp->cast<DfgConst>()) {
                ثابت uint32_t aMsb = aConstp->width() - 1;
                ثابت uint32_t bMsb = bConstp->width() - 1;
                ثابت V3Number& aNum = aConstp->num();
                ثابت V3Number& bNum = bConstp->num();
                // Max match is the shorter constant
                ثابت uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                لكل (; matchWidth < maxMatch; ++matchWidth) {
                    ܐܢ (aNum.bitIs0(aMsb - matchWidth) != bNum.bitIs0(bMsb - matchWidth)) اكسر;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* ثابت shorterp = aMsb < bMsb ? aConstp : bConstp;
                ܦܢܐ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        ܐܢ (DfgConcat* ثابت catp = ap->cast<DfgConcat>()) ܦܢܐ commonMSBs(catp->lhsp(), bp);
        ܐܢ (DfgConcat* ثابت catp = bp->cast<DfgConcat>()) ܦܢܐ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        ܐܢ (DfgSel* ثابت selp = ap->cast<DfgSel>()) {
            DfgVertex* ثابت fromp = selp->fromp();
            ܐܢ (selp->msb() == fromp->width() - 1) {
                ثابت std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                ܦܢܐ {common.first, std::min(common.second, selp->width())};
            }
        }
        ܐܢ (DfgSel* ثابت selp = bp->cast<DfgSel>()) {
            DfgVertex* ثابت fromp = selp->fromp();
            ܐܢ (selp->msb() == fromp->width() - 1) {
                ثابت std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                ܦܢܐ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        ܦܢܐ {nullptr, 0};
    }

    // VISIT methods

    فراغ visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    فراغ visit(DfgCountOnes* ثابت vtxp) override {
        ܐܢ (foldUnary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgExtend* ثابت vtxp) override {
        ܐܢ (foldUnary(vtxp)) ܦܢܐ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* ثابت zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ܦܢܐ;
        }
    }

    فراغ visit(DfgExtendS* ثابت vtxp) override {
        ܐܢ (foldUnary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgLogNot* ثابت vtxp) override {
        ܐܢ (foldUnary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgNegate* ثابت vtxp) override {
        ܐܢ (foldUnary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgNot* ثابت vtxp) override {
        ܐܢ (foldUnary(vtxp)) ܦܢܐ;

        // Not of Cond
        ܐܢ (DfgCond* ثابت condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            ܐܢ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(PUSH_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* ثابت newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* ثابت newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ܦܢܐ;
                }
            }
        }

        // Not of Not
        ܐܢ (DfgNot* ثابت notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ܦܢܐ;
            }
        }

        ܐܢ (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            ܐܢ (DfgEq* ثابت eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ܦܢܐ;
                }
            }

            // Not of Neq
            ܐܢ (DfgNeq* ثابت neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ܦܢܐ;
                }
            }

            // Not of unsigned comparisons
            ܐܢ (DfgLt* ثابت p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ܦܢܐ;
                }
            }
            ܐܢ (DfgGte* ثابت p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ܦܢܐ;
                }
            }
            ܐܢ (DfgGt* ثابت p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ܦܢܐ;
                }
            }
            ܐܢ (DfgLte* ثابت p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ܦܢܐ;
                }
            }

            // Not of signed comparisons
            ܐܢ (DfgLtS* ثابت p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ܦܢܐ;
                }
            }
            ܐܢ (DfgGteS* ثابت p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ܦܢܐ;
                }
            }
            ܐܢ (DfgGtS* ثابت p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ܦܢܐ;
                }
            }
            ܐܢ (DfgLteS* ثابت p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgOneHot* ثابت vtxp) override {
        ܐܢ (foldUnary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgOneHot0* ثابت vtxp) override {
        ܐܢ (foldUnary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgRedOr* ثابت vtxp) override {
        ܐܢ (optimizeReduction(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgRedAnd* ثابت vtxp) override {
        ܐܢ (optimizeReduction(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgRedXor* ثابت vtxp) override {
        ܐܢ (optimizeReduction(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgSel* ثابت vtxp) override {
        DfgVertex* ثابت fromp = vtxp->fromp();

        FileLine* ثابت flp = vtxp->fileline();

        ثابت uint32_t lsb = vtxp->lsb();
        ثابت uint32_t width = vtxp->width();
        ثابت uint32_t msb = lsb + width - 1;

        ܐܢ (DfgConst* ثابت constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* ثابت resp = makeZero(flp, width);
                resp->num().opSel(constp->num(), msb, lsb);
                replace(resp);
                ܦܢܐ;
            }
        }

        // Full width select, replace with the source.
        ܐܢ (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                ܦܢܐ;
            }
        }

        // Unwind through bit packing in one go
        {
            ثابت auto res = unwindSel(fromp, lsb, width);
            ܐܢ (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                ܦܢܐ;
            }
        }

        // Sel from Not
        ܐܢ (DfgNot* ثابت notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            ܐܢ (!notp->hasMultipleSinks()) {
                APPLYING(PUSH_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* ثابت newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ܦܢܐ;
                }
            }
        }

        // Sel from Cond
        ܐܢ (DfgCond* ثابت condp = fromp->cast<DfgCond>()) {
            ܐܢ (!condp->hasMultipleSinks()) {
                APPLYING(PUSH_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* ثابت newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* ثابت newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* ثابت newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ܦܢܐ;
                }
            }
        }

        // Sel from ShiftL
        ܐܢ (DfgShiftL* ثابت shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            ܐܢ (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(PUSH_SEL_THROUGH_SHIFTL) {
                    DfgSel* ثابت newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgMux* ثابت vtxp) override {
        DfgVertex* ثابت fromp = vtxp->fromp();
        DfgVertex* ثابت lsbp = vtxp->lsbp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (DfgConst* ثابت lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->num().toUInt()));
                ܦܢܐ;
            }
        }

        ܐܢ (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                ܦܢܐ;
            }
        }

        ܐܢ (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                ܦܢܐ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    فراغ visit(DfgAnd* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ܐܢ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ܐܢ (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                ܐܢ (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* ثابت orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ܦܢܐ;
                    }
                }
                ܐܢ (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* ثابت newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* ثابت orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            ܐܢ (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ܦܢܐ;
                }
            }

            ܐܢ (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ܦܢܐ;
                }
            }

            ܐܢ (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                ܐܢ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rhsConcatp)) ܦܢܐ;
            }
        }

        ܐܢ (distributiveAndAssociativeBinary<DfgOr, DfgAnd>(vtxp)) ܦܢܐ;

        ܐܢ (tryPushBitwiseOpThroughReductions(vtxp)) ܦܢܐ;

        ܐܢ (tryPushBitwiseOpThrougSel(vtxp)) ܦܢܐ;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            ܐܢ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    ܦܢܐ;
                }
            }

            ܐܢ (DfgAnd* ثابت rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* ثابت rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ثابت rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                ܐܢ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (vtxp->dtype() == m_bitDType) {
            ܐܢ (tryReplaceBitwiseWithReduction(vtxp)) ܦܢܐ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            ܐܢ (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            ܐܢ (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* ثابت newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgOr* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ܐܢ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ܐܢ (DfgNot* ثابت lhsNotp = lhsp->cast<DfgNot>()) {
                ܐܢ (DfgNot* ثابت rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ܦܢܐ;
                    }
                }
                ܐܢ (DfgNeq* ثابت rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* ثابت newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (DfgConcat* ثابت lhsConcatp = lhsp->cast<DfgConcat>()) {
            ܐܢ (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                ܐܢ (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    ܐܢ (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ܦܢܐ;
                        }
                    }
                    ܐܢ (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ܦܢܐ;
                        }
                    }
                }
            }
        }

        ܐܢ (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            ܐܢ (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ܦܢܐ;
                }
            }

            ܐܢ (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ܦܢܐ;
                }
            }

            ܐܢ (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                ܐܢ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rhsConcatp)) ܦܢܐ;
            }
        }

        ܐܢ (distributiveAndAssociativeBinary<DfgAnd, DfgOr>(vtxp)) ܦܢܐ;

        ܐܢ (tryPushBitwiseOpThroughReductions(vtxp)) ܦܢܐ;

        ܐܢ (tryPushBitwiseOpThrougSel(vtxp)) ܦܢܐ;

        {
            DfgNot* ثابت lNotp = lhsp->cast<DfgNot>();
            DfgNot* ثابت rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            ܐܢ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    ܦܢܐ;
                }
            }

            ܐܢ (DfgOr* ثابت rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* ثابت rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* ثابت rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                ܐܢ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (vtxp->dtype() == m_bitDType) {
            ܐܢ (tryReplaceBitwiseWithReduction(vtxp)) ܦܢܐ;
        }
    }

    فراغ visit(DfgXor* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                ܦܢܐ;
            }
        }

        ܐܢ (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            ܐܢ (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ܦܢܐ;
                }
            }
            ܐܢ (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ܦܢܐ;
                }
            }
            ܐܢ (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                ܐܢ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rConcatp)) ܦܢܐ;
            }
        }

        ܐܢ (tryPushBitwiseOpThroughReductions(vtxp)) ܦܢܐ;

        ܐܢ (tryPushBitwiseOpThrougSel(vtxp)) ܦܢܐ;

        ܐܢ (vtxp->dtype() == m_bitDType) {
            ܐܢ (tryReplaceBitwiseWithReduction(vtxp)) ܦܢܐ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    فراغ visit(DfgAdd* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                ܦܢܐ;
            }
        }

        ثابت std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = addToCountOnesTerm(lhsp);
        ܐܢ (DfgVertex* ثابت lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = addToCountOnesTerm(rhsp);
            DfgVertex* extrap = nullptr;
            ܐܢ (!std::get<0>(rTerm)) {
                ܐܢ (DfgAdd* ثابت rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = addToCountOnesTerm(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            ܐܢ (DfgVertex* ثابت rVtxp = std::get<0>(rTerm)) {
                ܐܢ (isSame(lVtxp, rVtxp)) {
                    ثابت uint32_t lLsb = std::get<1>(lTerm);
                    ثابت uint32_t rLsb = std::get<1>(rTerm);
                    ثابت uint32_t lWidth = std::get<2>(lTerm);
                    ثابت uint32_t rWidth = std::get<2>(rTerm);
                    منطقي adjoined = ܫܪܪܐ;
                    uint32_t lsb = 0;
                    ܐܢ (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } ܐܚܪܢܐ ܐܢ (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } ܐܚܪܢܐ {
                        adjoined = ܕܓܠܐ;
                    }
                    ܐܢ (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_COUNT_ONES) {
                            DfgSel* ثابت selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<DfgCountOnes>(flp, DfgDataType::packed(32), selp);
                            ܐܢ (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } ܐܚܪܢܐ ܐܢ (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            ܐܢ (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            ܦܢܐ;
                        }
                    }
                }
            }
        }
    }

    فراغ visit(DfgArraySel* ثابت vtxp) override {
        DfgConst* ثابت idxp = vtxp->bitp()->cast<DfgConst>();
        ܐܢ (!idxp) ܦܢܐ;
        DfgVarArray* ثابت varp = vtxp->fromp()->cast<DfgVarArray>();
        ܐܢ (!varp) ܦܢܐ;
        AstVar* ثابت astVarp = varp->vscp()->varp();
        ܐܢ (astVarp->isForced()) ܦܢܐ;
        ܐܢ (astVarp->isSigUserRWPublic()) ܦܢܐ;
        DfgVertex* ثابت srcp = varp->srcp();
        ܐܢ (!srcp) {
            ܐܢ (vtxp->isPacked()) {
                ܐܢ (AstConsArray* ثابت iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    ܐܢ (AstConst* ثابت valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(جديد DfgConst{m_dfg, valp->fileline(), valp->num()});
                            ܦܢܐ;
                        }
                    }
                }
            }
            ܦܢܐ;
        }

        ܐܢ (DfgSpliceArray* ثابت splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* ثابت driverp = splicep->driverAt(idxp->toSizeT());
            ܐܢ (!driverp) ܦܢܐ;
            DfgUnitArray* ثابت uap = driverp->cast<DfgUnitArray>();
            ܐܢ (!uap) ܦܢܐ;
            ܐܢ (uap->srcp()->is<DfgVertexSplice>()) ܦܢܐ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ܐܢ (DfgVertexVar* ثابت dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ܐܢ (dvarp->srcp()) ܦܢܐ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ܦܢܐ;
            }
        }

        ܐܢ (DfgUnitArray* ثابت uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            ܐܢ (uap->srcp()->is<DfgSplicePacked>()) ܦܢܐ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ܐܢ (DfgVertexVar* ثابت dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ܐܢ (dvarp->srcp()) ܦܢܐ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgConcat* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isZero(lhsp)) {
            DfgConst* ثابت lConstp = lhsp->as<DfgConst>();
            ܐܢ (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                ܐܢ (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_TOP_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (isZero(rhsp)) {
            DfgConst* ثابت rConstp = rhsp->as<DfgConst>();
            ܐܢ (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                ܐܢ (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (DfgNot* ثابت lNot = lhsp->cast<DfgNot>()) {
            ܐܢ (DfgNot* ثابت rNot = rhsp->cast<DfgNot>()) {
                ܐܢ (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(PUSH_CONCAT_THROUGH_NOTS) {
                        DfgConcat* ثابت newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ܦܢܐ;
                    }
                }
            }
        }

        {
            منطقي swap = ܕܓܠܐ;
            ܐܢ (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(PUSH_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    ܦܢܐ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ܐܢ (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* ثابت rlp = rConcatp->lhsp();
                ܐܢ (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(PUSH_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* ثابت newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
            ܐܢ (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
                ܐܢ (isSame(lSelp->fromp(), rSelp->fromp())) {
                    ܐܢ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ܦܢܐ;
                        }
                    }
                }
            }

            ܐܢ (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                ܐܢ (DfgSel* ثابت rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    ܐܢ (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        ܐܢ (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                ثابت uint32_t width = lSelp->width() + rlSelp->width();
                                ثابت DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ثابت selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ܦܢܐ;
                            }
                        }
                    }
                }
            }
        }

        ܐܢ (DfgSel* ثابت rSelp = rhsp->cast<DfgSel>()) {
            ܐܢ (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                ܐܢ (DfgSel* ثابت lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    ܐܢ (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        ܐܢ (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                ثابت uint32_t width = lrSelp->width() + rSelp->width();
                                ثابت DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* ثابت selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ܦܢܐ;
                            }
                        }
                    }
                }
            }
        }

        ܐܢ (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            ܐܢ (DfgCond* ثابت rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* ثابت rtVtxp = rCondp->thenp();
                DfgVertex* ثابت reVtxp = rCondp->elsep();
                DfgConst* ثابت rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* ثابت reConstp = reVtxp->cast<DfgConst>();
                ܐܢ (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(PUSH_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* ثابت thenp = [&]() -> DfgVertex* {
                            FileLine* ثابت rtFlp = rtVtxp->fileline();
                            ܐܢ (rtConstp) {
                                DfgConst* ثابت constp = makeZero(rtFlp, vtxp->width());
                                constp->num().opConcat(lConstp->num(), rtConstp->num());
                                ܦܢܐ constp;
                            }
                            ܦܢܐ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* ثابت elsep = [&]() -> DfgVertex* {
                            FileLine* ثابت reFlp = reVtxp->fileline();
                            ܐܢ (reConstp) {
                                DfgConst* ثابت constp = makeZero(reFlp, vtxp->width());
                                constp->num().opConcat(lConstp->num(), reConstp->num());
                                ܦܢܐ constp;
                            }
                            ܦܢܐ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ܐܢ (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* ثابت ltVtxp = lCondp->thenp();
                DfgVertex* ثابت leVtxp = lCondp->elsep();
                DfgConst* ثابت ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* ثابت leConstp = leVtxp->cast<DfgConst>();
                ܐܢ (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(PUSH_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* ثابت thenp = [&]() -> DfgVertex* {
                            FileLine* ثابت ltFlp = ltVtxp->fileline();
                            ܐܢ (ltConstp) {
                                DfgConst* ثابت constp = makeZero(ltFlp, vtxp->width());
                                constp->num().opConcat(ltConstp->num(), rConstp->num());
                                ܦܢܐ constp;
                            }
                            ܦܢܐ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* ثابت elsep = [&]() -> DfgVertex* {
                            FileLine* ثابت leFlp = leVtxp->fileline();
                            ܐܢ (leConstp) {
                                DfgConst* ثابت constp = makeZero(leFlp, vtxp->width());
                                constp->num().opConcat(leConstp->num(), rConstp->num());
                                ܦܢܐ constp;
                            }
                            ܦܢܐ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ܦܢܐ;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            ثابت uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            ثابت uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            ثابت uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            منطقي hasCrossSink = ܕܓܠܐ;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                ܐܢ (ثابت DfgSel* ثابت selp = sink.cast<DfgSel>()) {
                    ثابت uint32_t selLsb = selp->lsb();
                    ثابت uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ܦܢܐ ܕܓܠܐ;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                ܐܢ (ثابت DfgVarPacked* ثابت varp = sink.cast<DfgVarPacked>()) {
                    ܐܢ (!varp->hasSinks() && !varp->isObserved()) ܦܢܐ ܕܓܠܐ;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ܦܢܐ ܫܪܪܐ;
            });
            ܐܢ (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    ܐܢ (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    ܐܢ (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* ثابت catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* ثابت sp = جديد DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* ثابت varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(ܫܪܪܐ);
                    varp->srcp(sp);
                    replace(varp);
                    ܦܢܐ;
                }
            }
        }

        // Convert various forms to REPLICATE
        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ܦܢܐ;
            }
        }
        ܐܢ (DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            ܐܢ (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ܦܢܐ;
                }
            }
        }
        ܐܢ (DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
            ܐܢ (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ܦܢܐ;
                }
            }
        }
        ܐܢ (DfgRep* ثابت lRepp = lhsp->cast<DfgRep>()) {
            ܐܢ (DfgRep* ثابت rRepp = rhsp->cast<DfgRep>()) {
                ܐܢ (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ܦܢܐ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ܐܢ (DfgConcat* ثابت rConcatp = rhsp->cast<DfgConcat>()) {
                ܐܢ (DfgRep* ثابت rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    ܐܢ (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            ثابت uint32_t width = lRepp->width() + rlRepp->width();
                            ثابت DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* ثابت repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ܦܢܐ;
                        }
                    }
                }
            }
        }
    }

    فراغ visit(DfgDiv* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        ܐܢ (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ܐܢ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgDivS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        ܐܢ (DfgConst* ثابت rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ܐܢ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgGt* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgGtS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgGte* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgGteS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgLogAnd* ثابت vtxp) override {
        ܐܢ (binary(vtxp) || vtxp->rhsp()->unsafe()) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        ܐܢ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgLogEq* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgLogIf* ثابت vtxp) override {
        ܐܢ (binary(vtxp) || vtxp->rhsp()->unsafe()) ܦܢܐ;
    }

    فراغ visit(DfgLogOr* ثابت vtxp) override {
        ܐܢ (binary(vtxp) || vtxp->rhsp()->unsafe()) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        ܐܢ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgLt* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgLtS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgLte* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgLteS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ܦܢܐ;
            }
        }
    }

    فراغ visit(DfgModDiv* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgModDivS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgMul* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            ܐܢ (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ܦܢܐ;
                }
            }
            ܐܢ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgMulS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (DfgConst* ثابت lConstp = lhsp->cast<DfgConst>()) {
            ܐܢ (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ܦܢܐ;
                }
            }
            ܐܢ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgEq* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ܦܢܐ;
            }
        }

        ܐܢ (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            ܐܢ (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                ܐܢ (tryPushCompareOpThroughConcat(vtxp, lhsConstp, rhsConcatp)) ܦܢܐ;
            }

            ܐܢ (rhsp->dtype() == m_bitDType) {
                ܐܢ (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ܦܢܐ;
                    }
                }

                ܐܢ (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        ܦܢܐ;
                    }
                }
            }
        }
    }

    فراغ visit(DfgNeq* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ܦܢܐ;
            }
        }

        ܐܢ (DfgConst* ثابت lhsConstp = lhsp->cast<DfgConst>()) {
            ܐܢ (DfgConcat* ثابت rhsConcatp = rhsp->cast<DfgConcat>()) {
                ܐܢ (tryPushCompareOpThroughConcat(vtxp, lhsConstp, rhsConcatp)) ܦܢܐ;
            }

            ܐܢ (rhsp->dtype() == m_bitDType) {
                ܐܢ (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        ܦܢܐ;
                    }
                }

                ܐܢ (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ܦܢܐ;
                    }
                }
            }
        }
    }

    فراغ visit(DfgPow* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgPowSS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgPowSU* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgPowUS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgRep* ثابت vtxp) override {
        DfgVertex* ثابت srcp = vtxp->srcp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (DfgConst* ثابت sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* ثابت resultp = makeZero(flp, vtxp->width());
                resultp->num().opRepl(sConstp->num(), vtxp->count());
                replace(resultp);
                ܦܢܐ;
            }
        }

        ܐܢ (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ܦܢܐ;
            }
        }

        ܐܢ (DfgRep* ثابت sRepp = srcp->cast<DfgRep>()) {
            ܐܢ (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgShiftL* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
        ܐܢ (optimizeShiftRHS(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        ܐܢ (DfgShiftL* ثابت lShiftLp = lhsp->cast<DfgShiftL>()) {
            ܐܢ (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    FileLine* ثابت flp = vtxp->fileline();
                    ثابت DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ثابت bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* ثابت cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ثابت addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    ܦܢܐ;
                }
            }
        }

        ܐܢ (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t shiftAmount = rConstp->toU32();

            ܐܢ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ܦܢܐ;
                }
            }

            ܐܢ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ܦܢܐ;
                }
            }

            ܐܢ (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                ܐܢ (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* ثابت lRhsp = lConcatp->rhsp();
                        DfgVertex* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t rW = shiftAmount;
                        ثابت uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        ثابت uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        FileLine* ثابت flp = lConcatp->fileline();
                        DfgVertex* ثابت rp = makeZero(flp, shiftAmount);
                        DfgVertex* ثابت mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        ܐܢ (!lW) {
                            replace(np);
                            ܦܢܐ;
                        }
                        DfgVertex* ثابت lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ܦܢܐ;
                    }
                }
            }

            ܐܢ (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                ܐܢ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        ثابت uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        ܦܢܐ;
                    }
                }
            }

            ܐܢ (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                ܐܢ (!lCondp->hasMultipleSinks()) {
                    APPLYING(PUSH_SHIFTL_THROUGH_COND) {
                        DfgShiftL* ثابت tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* ثابت ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ܦܢܐ;
                    }
                }
            }
        }
    }

    فراغ visit(DfgShiftR* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
        ܐܢ (optimizeShiftRHS(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();

        ܐܢ (DfgShiftR* ثابت lShiftRp = lhsp->cast<DfgShiftR>()) {
            ܐܢ (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    FileLine* ثابت flp = vtxp->fileline();
                    ثابت DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* ثابت bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* ثابت cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* ثابت addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    ܦܢܐ;
                }
            }
        }

        ܐܢ (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ثابت uint32_t shiftAmount = rConstp->toU32();

            ܐܢ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ܦܢܐ;
                }
            }

            ܐܢ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ܦܢܐ;
                }
            }

            ܐܢ (DfgConcat* ثابت lConcatp = lhsp->cast<DfgConcat>()) {
                ܐܢ (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* ثابت lRhsp = lConcatp->rhsp();
                        DfgVertex* ثابت lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        ثابت uint32_t lW = shiftAmount;
                        ثابت uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        ثابت uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        FileLine* ثابت flp = lConcatp->fileline();
                        DfgVertex* ثابت lp = makeZero(flp, shiftAmount);
                        DfgVertex* ثابت mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        ܐܢ (!rW) {
                            replace(np);
                            ܦܢܐ;
                        }
                        DfgVertex* ثابت rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ܦܢܐ;
                    }
                }
            }

            ܐܢ (DfgSel* ثابت lSelp = lhsp->cast<DfgSel>()) {
                ܐܢ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        ثابت uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* ثابت nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        ܦܢܐ;
                    }
                }
            }

            ܐܢ (DfgCond* ثابت lCondp = lhsp->cast<DfgCond>()) {
                ܐܢ (!lCondp->hasMultipleSinks()) {
                    APPLYING(PUSH_SHIFTR_THROUGH_COND) {
                        DfgShiftR* ثابت tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* ثابت ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ܦܢܐ;
                    }
                }
            }
        }
    }

    فراغ visit(DfgShiftRS* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;
        ܐܢ (optimizeShiftRHS(vtxp)) ܦܢܐ;
    }

    فراغ visit(DfgSub* ثابت vtxp) override {
        ܐܢ (binary(vtxp)) ܦܢܐ;

        DfgVertex* ثابت lhsp = vtxp->lhsp();
        DfgVertex* ثابت rhsp = vtxp->rhsp();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ܦܢܐ;
            }
        }

        ܐܢ (DfgConst* ثابت rConstp = rhsp->cast<DfgConst>()) {
            ܐܢ (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ܦܢܐ;
                }
            }
            ܐܢ (vtxp->dtype() == m_bitDType && rConstp->hasValue(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgMatchMasked* ثابت vtxp) override {
        ܐܢ (DfgConst* ثابت constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* ثابت matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->num(), matchVarp)));
                ܦܢܐ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    فراغ visit(DfgCond* ثابت vtxp) override {
        DfgVertex* ثابت condp = vtxp->condp();
        DfgVertex* ثابت thenp = vtxp->thenp();
        DfgVertex* ثابت elsep = vtxp->elsep();
        FileLine* ثابت flp = vtxp->fileline();

        ܐܢ (condp->dtype() != m_bitDType) ܦܢܐ;

        ܐܢ (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ܦܢܐ;
            }
        }

        ܐܢ (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ܦܢܐ;
            }
        }

        ܐܢ (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ܦܢܐ;
            }
        }

        ܐܢ (DfgNot* ثابت condNotp = condp->cast<DfgNot>()) {
            ܐܢ (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ܦܢܐ;
                }
            }
        }

        ܐܢ (DfgNeq* ثابت condNeqp = condp->cast<DfgNeq>()) {
            ܐܢ (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* ثابت newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ܦܢܐ;
                }
            }
        }

        ܐܢ (DfgNot* ثابت thenNotp = thenp->cast<DfgNot>()) {
            ܐܢ (DfgNot* ثابت elseNotp = elsep->cast<DfgNot>()) {
                ܐܢ (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* ثابت newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (DfgOr* ثابت condOrp = condp->cast<DfgOr>()) {
            ܐܢ (DfgCond* ثابت thenCondp = thenp->cast<DfgCond>()) {
                ܐܢ (!thenCondp->hasMultipleSinks()) {
                    ܐܢ (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* ثابت ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ܦܢܐ;
                        }
                    }
                    ܐܢ (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* ثابت ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ܦܢܐ;
                        }
                    }
                }
            }
        }

        ܐܢ (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            ܐܢ (DfgAdd* ثابت thenAddp = thenp->cast<DfgAdd>()) {
                ܐܢ (DfgConst* ثابت constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    ܐܢ (constp->hasValue(1)) {
                        ܐܢ (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                FileLine* ثابت thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ܦܢܐ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            ܐܢ (DfgSub* ثابت thenSubp = thenp->cast<DfgSub>()) {
                ܐܢ (DfgConst* ثابت constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    ܐܢ (constp->hasValue(1)) {
                        ܐܢ (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* ثابت extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                FileLine* ثابت thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ܦܢܐ;
                            }
                        }
                    }
                }
            }
        }

        ܐܢ (vtxp->dtype() == m_bitDType) {
            ܐܢ (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    ܦܢܐ;
                }
            }
            ܐܢ (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ܦܢܐ;
                }
            }
        }

        ܐܢ (vtxp->width() <= VL_QUADSIZE) {
            ܐܢ (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ܦܢܐ;
                }
            }
            ܐܢ (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ܦܢܐ;
                }
            }
            ܐܢ (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* ثابت maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ܦܢܐ;
                }
            }
            ܐܢ (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* ثابت maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ܦܢܐ;
                }
            }

            ܐܢ (DfgOr* ثابت tOrp = thenp->cast<DfgOr>()) {
                ܐܢ (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ܦܢܐ;
                    }
                }
                ܐܢ (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* ثابت maskp = replicate(vtxp, condp);
                        DfgAnd* ثابت andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ܦܢܐ;
                    }
                }
            }
        }

        ܐܢ (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            ثابت std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            ܐܢ (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    ثابت uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    ܐܢ (rWidth != rhsp->width()) {
                        ثابت DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    ܐܢ (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ܦܢܐ;
                    }
                    // Create new LHS
                    ثابت uint32_t lWidth = vtxp->width() - rWidth;
                    ثابت DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* ثابت lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* ثابت lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* ثابت lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ܦܢܐ;
                }
            }

            ثابت std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            ܐܢ (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    ثابت uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    ܐܢ (lWidth != lhsp->width()) {
                        ثابت DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    ܐܢ (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ܦܢܐ;
                    }
                    // Create new RHS
                    ثابت uint32_t rWidth = vtxp->width() - lWidth;
                    ثابت DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* ثابت rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* ثابت rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* ثابت rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ܦܢܐ;
                }
            }
        }

        ܐܢ (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                ܐܢ (ثابت uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ثابت zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ܦܢܐ;
            }
        }

        ܐܢ (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                ܐܢ (ثابت uint32_t extend = vtxp->width() - 1) {
                    DfgConst* ثابت zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ܦܢܐ;
            }
        }

        ܐܢ (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ܦܢܐ;
            }
        }

        ܐܢ (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ܦܢܐ;
            }
        }

        ܐܢ (DfgCond* ثابت tCondp = thenp->cast<DfgCond>()) {
            ܐܢ (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ܦܢܐ;
                }
            }
        }

        ܐܢ (DfgCond* ثابت eCondp = elsep->cast<DfgCond>()) {
            ܐܢ (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ܦܢܐ;
                }
            }
        }
    }

    فراغ visit(DfgVertexVar* ثابت vtxp) override {
        ܐܢ (vtxp->hasSinks()) ܦܢܐ;
        ܐܢ (vtxp->isObserved()) ܦܢܐ;
        ܐܢ (vtxp->defaultp()) ܦܢܐ;

        // If undriven, or driven from another var, it is completely redundant.
        ܐܢ (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ܦܢܐ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        منطقي foundOne = ܕܓܠܐ;
        ثابت منطقي keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> منطقي {
            // Ignore non-observable variable sinks. These can be eliminated.
            ܐܢ (ثابت DfgVertexVar* ثابت varp = sink.cast<DfgVertexVar>()) {
                ܐܢ (!varp->hasSinks() && !varp->isObserved()) ܦܢܐ ܕܓܠܐ;
            }
            // Keep if found more than one sink
            ܐܢ (foundOne) ܦܢܐ ܫܪܪܐ;
            // Mark first sink found
            foundOne = ܫܪܪܐ;
            ܦܢܐ ܕܓܠܐ;
        });
        ܐܢ (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ܦܢܐ;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        لكل (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

        // Initialize the work list and iter list. They can't get bigger than
        // m_dfg.size(), but new vertices are created in the loop, so over alloacte
        m_workList.reserve(m_dfg.size() * 2);
        m_iterList.reserve(m_dfg.size() * 2);

        // Need a nullptr at index 0 so VertexInfo::m_*ListIndex == 0 can check membership
        m_workList.push_back(nullptr);
        m_iterList.push_back(nullptr);

        // Add all variable vertices to the work list. Do this first so they are processed
        // last. This order has a better chance of preserving original variables in case
        // they are needed to hold intermediate results.
        لكل (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        لكل (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        ܟܡܐ (ܫܪܪܐ) {
            // Process the work list - keep the placeholder at index 0
            ܟܡܐ (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.pop_back();
                ܐܢ (!m_vtxp) استمر;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                ܐܢ (DfgVertexVar* ثابت varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    استمر;
                }

                // Ast references are not considered
                ܐܢ (m_vtxp->is<DfgVertexAst>()) استمر;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                ܐܢ (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    ܐܢ (DfgVertex* ثابت sampep = cacheVertex(m_vtxp)) {
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
            ܐܢ (m_iterList.size() == 1) اكسر;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                لكل (size_t hops = 1; hops <= 4; ++hops) {
                    لكل (size_t i = begin; i < end; ++i) {
                        DfgVertex* ثابت vtxp = m_iterList[i];
                        ܐܢ (!vtxp) استمر;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            ܦܢܐ ܕܓܠܐ;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            ܦܢܐ ܕܓܠܐ;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            لكل (DfgVertex* ثابت vtxp : m_iterList) {
                ܐܢ (!vtxp) استمر;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

عام:
    ساكن فراغ apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

فراغ V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    ܐܢ (!v3Global.opt.fDfgPeephole()) ܦܢܐ;
    V3DfgPeephole::apply(dfg, ctx);
}
