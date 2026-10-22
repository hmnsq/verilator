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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Ast.h"
#include "V3Dfg.h"
#include "V3DfgCache.h"
#include "V3DfgDataType.h"
#include "V3DfgPasses.h"
#include "V3DfgPeepholePatterns.h"

#include <algorithm>
#include <cctype>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// clang-format off
namespace {

template <typename T_Reduction>
sa_nracanaa ReductionToBitwiseImpl {};
template <> sa_nracanaa ReductionToBitwiseImpl<DfgRedAnd> { using type = DfgAnd; };
template <> sa_nracanaa ReductionToBitwiseImpl<DfgRedOr>  { using type = DfgOr;  };
template <> sa_nracanaa ReductionToBitwiseImpl<DfgRedXor> { using type = DfgXor; };
template <typename T_Reduction>
using ReductionToBitwise = typename ReductionToBitwiseImpl<T_Reduction>::type;

template <typename T_Bitwise>
sa_nracanaa BitwiseToReductionImpl {};
template <> sa_nracanaa BitwiseToReductionImpl<DfgAnd> { using type = DfgRedAnd; };
template <> sa_nracanaa BitwiseToReductionImpl<DfgOr>  { using type = DfgRedOr;  };
template <> sa_nracanaa BitwiseToReductionImpl<DfgXor> { using type = DfgRedXor; };
template <typename T_Reduction>
using BitwiseToReduction = typename BitwiseToReductionImpl<T_Reduction>::type;

// Associative binary operators (a op (b op c) == (a op b) op c)
template<typename Vertex>
sa_nracanaa IsAssociative : public std::false_type {};
template <> sa_nracanaa IsAssociative<DfgAnd> : public std::true_type {};
template <> sa_nracanaa IsAssociative<DfgOr>  : public std::true_type {};
template <> sa_nracanaa IsAssociative<DfgXor> : public std::true_type {};
template <> sa_nracanaa IsAssociative<DfgAdd> : public std::true_type {};
template <> sa_nracanaa IsAssociative<DfgConcat> : public std::true_type {};
template <> sa_nracanaa IsAssociative<DfgMul> : public std::true_type {};
template <> sa_nracanaa IsAssociative<DfgMulS> : public std::true_type {};

// Commutative binary operators (a op b == b op a)
template<typename Vertex>
sa_nracanaa IsCommutative : public std::false_type {};
template <> sa_nracanaa IsCommutative<DfgAnd> : public std::true_type {};
template <> sa_nracanaa IsCommutative<DfgOr>  : public std::true_type {};
template <> sa_nracanaa IsCommutative<DfgXor> : public std::true_type {};
template <> sa_nracanaa IsCommutative<DfgAdd> : public std::true_type {};
template <> sa_nracanaa IsCommutative<DfgEq> : public std::true_type {};
template <> sa_nracanaa IsCommutative<DfgMul> : public std::true_type {};
template <> sa_nracanaa IsCommutative<DfgMulS> : public std::true_type {};
template <> sa_nracanaa IsCommutative<DfgNeq> : public std::true_type {};

// Idempotent binary operators (a op a == a)
template<typename Vertex>
sa_nracanaa IsIdempotent : public std::false_type {};
template <> sa_nracanaa IsIdempotent<DfgAnd> : public std::true_type {};
template <> sa_nracanaa IsIdempotent<DfgOr>  : public std::true_type {};

// Binary result dtype
template<typename Vertex>
const DfgDataType& resultDType(const DfgVertex* lhsp, const DfgVertex* rhsp);
template <> const DfgDataType& resultDType<DfgAdd>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgAnd>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgConcat>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> const DfgDataType& resultDType<DfgDiv>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgDivS>       (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgEq>         (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgGt>         (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgGtS>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgGte>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgGteS>       (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLogAnd>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLogEq>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLogIf>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLogOr>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLt>         (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLtS>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLte>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLteS>       (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgModDiv>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgModDivS>    (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgMul>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgMulS>       (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgNeq>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgOr>         (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgPow>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgPowSS>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgPowSU>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgPowUS>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgShiftL>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgShiftR>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgShiftRS>    (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgSub>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgXor>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᬯᬮᬶ lhsp->dtype(); }

// Unary constant folding
template<typename Vertex> void foldOp(V3Number& out, const V3Number& src);
template <> void foldOp<DfgCountOnes>  (V3Number& out, const V3Number& src) { out.opCountOnes(src); }
template <> void foldOp<DfgExtend>     (V3Number& out, const V3Number& src) { out.opAssign(src); }
template <> void foldOp<DfgExtendS>    (V3Number& out, const V3Number& src) { out.opExtendS(src, src.width()); }
template <> void foldOp<DfgLogNot>     (V3Number& out, const V3Number& src) { out.opLogNot(src); }
template <> void foldOp<DfgNegate>     (V3Number& out, const V3Number& src) { out.opNegate(src); }
template <> void foldOp<DfgNot>        (V3Number& out, const V3Number& src) { out.opNot(src); }
template <> void foldOp<DfgOneHot>     (V3Number& out, const V3Number& src) { out.opOneHot(src); }
template <> void foldOp<DfgOneHot0>    (V3Number& out, const V3Number& src) { out.opOneHot0(src); }
template <> void foldOp<DfgRedAnd>     (V3Number& out, const V3Number& src) { out.opRedAnd(src); }
template <> void foldOp<DfgRedOr>      (V3Number& out, const V3Number& src) { out.opRedOr(src); }
template <> void foldOp<DfgRedXor>     (V3Number& out, const V3Number& src) { out.opRedXor(src); }

// Binary constant folding
template<typename Vertex> void foldOp(V3Number& out, const V3Number& lhs, const V3Number& rhs);
template <> void foldOp<DfgAdd>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opAdd(lhs, rhs); }
template <> void foldOp<DfgAnd>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opAnd(lhs, rhs); }
template <> void foldOp<DfgConcat>     (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opConcat(lhs, rhs); }
template <> void foldOp<DfgDiv>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opDiv(lhs, rhs); }
template <> void foldOp<DfgDivS>       (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opDivS(lhs, rhs); }
template <> void foldOp<DfgEq>         (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opEq(lhs, rhs); }
template <> void foldOp<DfgGt>         (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opGt(lhs, rhs); }
template <> void foldOp<DfgGtS>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opGtS(lhs, rhs); }
template <> void foldOp<DfgGte>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opGte(lhs, rhs); }
template <> void foldOp<DfgGteS>       (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opGteS(lhs, rhs); }
template <> void foldOp<DfgLogAnd>     (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opLogAnd(lhs, rhs); }
template <> void foldOp<DfgLogEq>      (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opLogEq(lhs, rhs); }
template <> void foldOp<DfgLogIf>      (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opLogIf(lhs, rhs); }
template <> void foldOp<DfgLogOr>      (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opLogOr(lhs, rhs); }
template <> void foldOp<DfgLt>         (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opLt(lhs, rhs); }
template <> void foldOp<DfgLtS>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opLtS(lhs, rhs); }
template <> void foldOp<DfgLte>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opLte(lhs, rhs); }
template <> void foldOp<DfgLteS>       (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opLteS(lhs, rhs); }
template <> void foldOp<DfgModDiv>     (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opModDiv(lhs, rhs); }
template <> void foldOp<DfgModDivS>    (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opModDivS(lhs, rhs); }
template <> void foldOp<DfgMul>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opMul(lhs, rhs); }
template <> void foldOp<DfgMulS>       (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opMulS(lhs, rhs); }
template <> void foldOp<DfgNeq>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opNeq(lhs, rhs); }
template <> void foldOp<DfgOr>         (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opOr(lhs, rhs); }
template <> void foldOp<DfgPow>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opPow(lhs, rhs); }
template <> void foldOp<DfgPowSS>      (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opPowSS(lhs, rhs); }
template <> void foldOp<DfgPowSU>      (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opPowSU(lhs, rhs); }
template <> void foldOp<DfgPowUS>      (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opPowUS(lhs, rhs); }
template <> void foldOp<DfgShiftL>     (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opShiftL(lhs, rhs); }
template <> void foldOp<DfgShiftR>     (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opShiftR(lhs, rhs); }
template <> void foldOp<DfgShiftRS>    (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opShiftRS(lhs, rhs, lhs.width()); }
template <> void foldOp<DfgSub>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opSub(lhs, rhs); }
template <> void foldOp<DfgXor>        (V3Number& out, const V3Number& lhs, const V3Number& rhs) { out.opXor(lhs, rhs); }
}
// clang-format on

class V3DfgPeephole final : public DfgVisitor {
    // TYPES
    sa_nracanaa VertexInfo final {
        size_t m_workListIndex = 0;  // Position of this vertx m_workList (0 means not in list)
        size_t m_iterListIndex = 0;  // Position of this vertx m_iterList (0 means not in list)
        size_t m_generation = 0;  // Generation number of this vertex - for uniqueness check
        size_t m_id = 0;  // Unique vertex ID (0 means unassigned) - for sorting
        bool m_isCachedVertex = false;  // This vertex is the vertex cached for its operation
    };

    // STATE
    DfgGraph& m_dfg;  // The DfgGraph being visited
    V3DfgPeepholeContext& m_ctx;  // The config structure
    const DfgDataType& m_bitDType = DfgDataType::packed(1);  // Common, so grab it up front
    std::vector<DfgVertex*> m_workList;  // List of vertices processed in current interation
    std::vector<DfgVertex*> m_iterList;  // Vertices to start from on next iteration
    DfgUserMap<VertexInfo> m_vInfo = m_dfg.makeUserMap<VertexInfo>();  // Map to VertexInfo
    V3DfgCache m_cache{m_dfg};  // Vertex cache to avoid creating redundant vertices
    DfgVertex* m_vtxp = nullptr;  // Currently considered vertex
    size_t m_currentGeneration = 0;  // Current generation number
    size_t m_lastId = 0;  // Last unique vertex ID assigned
    // Scope for transient temporariy variables cerated in this pass. They should all be
    // eliminated wihtin this pass, so anything should be ok, pick the top scope as easy to find.
    AstScope* const m_tmpScopep = v3Global.rootp()->topScopep()->scopep();

    // STATIC STATE
    static V3DebugBisect s_debugBisect;  // Debug aid

#define APPLYING(id) ᬬᬾᬦ᭄ (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    bool checkApplying(VDfgPeepholePattern id) {
        ᬬᬾᬦ᭄ (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ᬯᬮᬶ false;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_count[id];
        ᬯᬮᬶ true;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* const equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        ᬯᬮᬶ equivp;
    }

    // Remove vertex from the cache (no-op if it is not the cached vertex)
    void invalidateVertex(DfgVertex* vtxp) {
        m_cache.invalidate(vtxp);
        m_vInfo[vtxp].m_isCachedVertex = false;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or create a new one and add it to the cache.
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreateVertex(FileLine* flp, const DfgDataType& dtype, Operands... operands) {
        Vertex* const vtxp = m_cache.getOrCreate<Vertex, Operands...>(flp, dtype, operands...);
        m_vInfo[vtxp].m_isCachedVertex = true;
        ᬯᬮᬶ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(const DfgDataType& dtype, Operands... operands) {
        Vertex* const vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ᬯᬮᬶ vtxp;
    }

    void incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    void addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        ᬬᬾᬦ᭄ (vInfo.m_workListIndex) ᬯᬮᬶ;
        // Add to work list
        vInfo.m_workListIndex = m_workList.size();
        m_workList.push_back(vtxp);
    }

    void removeFromWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_workList[0] is always nullptr, fine to assign same here
        m_workList[vInfo.m_workListIndex] = nullptr;
        vInfo.m_workListIndex = 0;
    }

    void addToIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on iter list, ignore
        ᬬᬾᬦ᭄ (vInfo.m_iterListIndex) ᬯᬮᬶ;
        // Add to iter list
        vInfo.m_iterListIndex = m_iterList.size();
        m_iterList.push_back(vtxp);
    }

    void removeFromIterList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // m_iterList[0] is always nullptr, fine to assign same here
        m_iterList[vInfo.m_iterListIndex] = nullptr;
        vInfo.m_iterListIndex = 0;
    }

    void deleteVertex(DfgVertex* vtxp) {
        UASSERT_OBJ(!m_vInfo[vtxp].m_workListIndex, vtxp, "Deleted Vertex is in work list");
        UASSERT_OBJ(!vtxp->hasSinks(), vtxp, "Should not delete used vertex");
        const DfgVertexVar* const varp = vtxp->cast<DfgVertexVar>();
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
            ᬬᬾᬦ᭄ (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                ᬯᬮᬶ false;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            ᬬᬾᬦ᭄ (vInfo.m_generation != m_currentGeneration) srcps.push_back(&src);
            vInfo.m_generation = m_currentGeneration;
            ᬯᬮᬶ false;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        ᬬᬾᬦ᭄ (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.push_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } ᬮᬶᬬ {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        const auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  ᬯᬮᬶ srcp->hasSinks();
              });

        // Add used sources to the iter list - their sinks have changed
        for (auto it = srcps.begin(); it != mid; ++it) addToIterList(*it);

        // Recursively delete unused sources
        for (auto it = mid; it != srcps.end(); ++it) {
            // Remove from work list
            removeFromWorkList(*it);
            // Delete vertex
            deleteVertex(*it);
        }
    }

    // Replace 'm_vtxp' with the given vertex
    void replace(DfgVertex* resp) {
        // Should not be in the work list
        UASSERT_OBJ(!m_vInfo[m_vtxp].m_workListIndex, m_vtxp, "Replaced Vertex is in work list");

        // Debug bisect check
        const auto debugCallback = [&]() -> void {
            UINFO(0, "Problematic DfgPeephole replacement: " << m_vtxp << " -> " << resp);
            m_dfg.sourceCone({m_vtxp, resp});
            const auto cone = m_dfg.sourceCone({m_vtxp, resp});
            m_dfg.dumpDotFilePrefixed("peephole-broken", [&](const DfgVertex& v) {  //
                ᬯᬮᬶ cone->count(&v);
            });
        };
        ᬬᬾᬦ᭄ (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ᬯᬮᬶ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            ᬯᬮᬶ false;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            ᬯᬮᬶ false;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            ᬯᬮᬶ false;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            ᬯᬮᬶ false;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(FileLine* flp, uint32_t val) { ᬯᬮᬶ new DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(FileLine* flp, uint32_t width) {
        ᬯᬮᬶ new DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(FileLine* flp, const DfgDataType& dtype) {
        ᬯᬮᬶ makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(FileLine* flp, uint32_t width) {
        DfgConst* const resp = makeZero(flp, width);
        resp->num().setAllBits1();
        ᬯᬮᬶ resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(FileLine* flp, const DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* const vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        ᬬᬾᬦ᭄ (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        ᬬᬾᬦ᭄ (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        ᬯᬮᬶ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(const DfgVertex* examplep, Operands... operands) {
        ᬯᬮᬶ make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        ᬬᬾᬦ᭄ (vtxp->dtype() == m_bitDType) ᬯᬮᬶ bitp;
        ᬯᬮᬶ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    static bool isSame(const DfgVertex* ap, const DfgVertex* bp) {
        ᬬᬾᬦ᭄ (ap == bp) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (ap->dtype() != bp->dtype()) ᬯᬮᬶ false;
        const DfgConst* const aConstp = ap->cast<DfgConst>();
        ᬬᬾᬦ᭄ (!aConstp) ᬯᬮᬶ false;
        const DfgConst* const bConstp = bp->cast<DfgConst>();
        ᬬᬾᬦ᭄ (!bConstp) ᬯᬮᬶ false;
        ᬯᬮᬶ aConstp->num().isCaseEq(bConstp->num());
    }

    static bool isZero(const DfgVertex* vtxp) {
        ᬬᬾᬦ᭄ (const DfgConst* const constp = vtxp->cast<DfgConst>()) ᬯᬮᬶ constp->isZero();
        ᬯᬮᬶ false;
    }

    static bool isOnes(const DfgVertex* vtxp) {
        ᬬᬾᬦ᭄ (const DfgConst* const constp = vtxp->cast<DfgConst>()) ᬯᬮᬶ constp->isOnes();
        ᬯᬮᬶ false;
    }

    static bool isEqOne(const DfgVertex* vtxp) {
        ᬬᬾᬦ᭄ (const DfgConst* const constp = vtxp->cast<DfgConst>()) ᬯᬮᬶ constp->num().isEqOne();
        ᬯᬮᬶ false;
    }

    static bool areAdjacent(uint32_t& lsb, const DfgSel* lSelp, const DfgSel* rSelp) {
        ᬬᬾᬦ᭄ (!isSame(lSelp->srcp(), rSelp->srcp())) ᬯᬮᬶ false;
        ᬬᬾᬦ᭄ (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ᬯᬮᬶ true;
        }
        ᬬᬾᬦ᭄ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            ᬯᬮᬶ true;
        }
        ᬯᬮᬶ false;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool foldUnary(Vertex* const vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::value, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::value, "Must invoke on final class");
        ᬬᬾᬦ᭄ (DfgConst* const srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* const resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->num(), srcp->num());
                replace(resultp);
                ᬯᬮᬶ true;
            }
        }
        ᬯᬮᬶ false;
    }

    // Generic transformations that apply to binary vertices. Returns true if vtxp was replaced.
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool binary(Vertex* const vtxp) {
        static_assert(std::is_base_of<DfgVertexBinary, Vertex>::value, "Must invoke on binary");
        static_assert(std::is_final<Vertex>::value, "Must invoke on final class");

        FileLine* const flp = vtxp->fileline();
        // LHS/RHS of the vertex
        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        // LHS/RHS as constant
        DfgConst* const lConstp = lhsp->cast<DfgConst>();
        DfgConst* const rConstp = rhsp->cast<DfgConst>();
        // LHS/RHS as same vertex type
        Vertex* const lSamep = lhsp->cast<Vertex>();
        Vertex* const rSamep = rhsp->cast<Vertex>();

        // Constant folding
        ᬬᬾᬦ᭄ (lConstp && rConstp) {
            APPLYING(FOLD_BINARY) {
                DfgConst* const resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->num(), lConstp->num(), rConstp->num());
                replace(resultp);
                ᬯᬮᬶ true;
            }
        }

        // Idempotent
        ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::value) {
            ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_BINARY) {
                    replace(lhsp);
                    ᬯᬮᬶ true;
                }
            }
        }

        // Associative
        ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::value) {
            // Also commutative and idempotent
            ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value && IsIdempotent<Vertex>::value) {
                ᬬᬾᬦ᭄ (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_BINARY_LHS) {
                        replace(lSamep);
                        ᬯᬮᬶ true;
                    }
                }
                ᬬᬾᬦ᭄ (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_BINARY_RHS) {
                        replace(rSamep);
                        ᬯᬮᬶ true;
                    }
                }
            }

            ᬬᬾᬦ᭄ (lConstp && rSamep) {
                ᬬᬾᬦ᭄ (DfgConst* const rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_BINARY_LHS_OF_RHS) {
                        DfgConst* const cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->num(), lConstp->num(), rlConstp->num());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        ᬯᬮᬶ true;
                    }
                }
            }

            ᬬᬾᬦ᭄ (lSamep && rConstp) {
                ᬬᬾᬦ᭄ (DfgConst* const lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_BINARY_RHS_OF_LHS) {
                        DfgConst* const cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->num(), lrConstp->num(), rConstp->num());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        ᬯᬮᬶ true;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            ᬬᬾᬦ᭄ (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* const ap = lSamep->lhsp();
                DfgVertex* const bp = lSamep->rhsp();
                DfgVertex* const cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                ᬬᬾᬦ᭄ (ap != cp && bp != cp) {
                    APPLYING(RIGHT_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        const DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* const bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ᬯᬮᬶ true;
                    }
                }
            }

            ᬬᬾᬦ᭄ (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* const rlVtxp = rSamep->lhsp();
                DfgVertex* const rrVtxp = rSamep->rhsp();

                ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
                    ᬬᬾᬦ᭄ (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            ᬯᬮᬶ true;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                ᬬᬾᬦ᭄ (Vertex* const existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    ᬬᬾᬦ᭄ (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_BINARY_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            ᬯᬮᬶ true;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
                    ᬬᬾᬦ᭄ (Vertex* const existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        ᬬᬾᬦ᭄ (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_BINARY_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                ᬯᬮᬶ true;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            ᬬᬾᬦ᭄ (lConstp) {
                ᬬᬾᬦ᭄ (DfgCond* const rCondp = rhsp->cast<DfgCond>()) {
                    ᬬᬾᬦ᭄ (!rCondp->hasMultipleSinks()) {
                        APPLYING(PUSH_COMMUTATIVE_BINARY_THROUGH_COND) {
                            DfgVertex* const tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* const ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ᬯᬮᬶ true;
                        }
                    }
                }
                ᬯᬮᬶ false;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        const bool swapSides = [&]() {
            // Don't swap if not commutative, duh
            ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::value) ᬯᬮᬶ false;
            // Const go on left
            ᬬᬾᬦ᭄ (lConstp) ᬯᬮᬶ false;
            ᬬᬾᬦ᭄ (rConstp) ᬯᬮᬶ true;
            // Not go on left
            ᬬᬾᬦ᭄ (lhsp->is<DfgNot>()) ᬯᬮᬶ false;
            ᬬᬾᬦ᭄ (rhsp->is<DfgNot>()) ᬯᬮᬶ true;
            // Same go on right
            ᬬᬾᬦ᭄ (rSamep) ᬯᬮᬶ false;
            ᬬᬾᬦ᭄ (lSamep) ᬯᬮᬶ true;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            const VertexInfo& lInfo = m_vInfo[lhsp];
            const VertexInfo& rInfo = m_vInfo[rhsp];
            ᬯᬮᬶ lInfo.m_id > rInfo.m_id;
        }();
        ᬬᬾᬦ᭄ (swapSides) {
            APPLYING(SWAP_SIDES_IN_BINARY) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                ᬯᬮᬶ true;
            }
        }

        ᬯᬮᬶ false;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool distributiveAndAssociativeBinary(Vertex* vtxp) {
        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        ᬬᬾᬦ᭄ (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            ᬬᬾᬦ᭄ (Other* const lp = lhsp->cast<Other>()) {
                ᬬᬾᬦ᭄ (Other* const rp = rhsp->cast<Other>()) {
                    DfgVertex* const llp = lp->lhsp();
                    DfgVertex* const lrp = lp->rhsp();
                    DfgVertex* const rlp = rp->lhsp();
                    DfgVertex* const rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    ᬬᬾᬦ᭄ (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    ᬬᬾᬦ᭄ (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_BINARY) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            ᬯᬮᬶ true;
                        }
                    }
                }
            }
        }

        ᬯᬮᬶ false;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool
    tryPushBitwiseOpThroughConcat(Vertex* const vtxp, DfgConst* constp, DfgConcat* concatp) {
        FileLine* const flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* const catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* const catRConstp = concatp->rhsp()->cast<DfgConst>();
        ᬬᬾᬦ᭄ (catLConstp || catRConstp) {
            APPLYING(PUSH_BITWISE_OP_THROUGH_CONCAT) {
                const uint32_t width = concatp->width();
                const DfgDataType& lDtype = concatp->lhsp()->dtype();
                const DfgDataType& rDtype = concatp->rhsp()->dtype();
                const uint32_t lWidth = lDtype.size();
                const uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* const newLhsp = [&]() -> DfgVertex* {
                    DfgConst* const newLhsConstp = makeZero(constp->fileline(), lWidth);
                    ᬬᬾᬦ᭄ (catLConstp) {
                        V3Number num{constp->fileline(), static_cast<puur_naa_nka>(lWidth), 0u};
                        num.opSel(constp->num(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->num(), num, catLConstp->num());
                        ᬯᬮᬶ newLhsConstp;
                    }
                    newLhsConstp->num().opSel(constp->num(), width - 1, rWidth);
                    ᬯᬮᬶ make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* const newRhsp = [&]() -> DfgVertex* {
                    DfgConst* const newRhsConstp = makeZero(constp->fileline(), rWidth);
                    ᬬᬾᬦ᭄ (catRConstp) {
                        V3Number num{constp->fileline(), static_cast<puur_naa_nka>(rWidth), 0u};
                        num.opSel(constp->num(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->num(), num, catRConstp->num());
                        ᬯᬮᬶ newRhsConstp;
                    }
                    newRhsConstp->num().opSel(constp->num(), rWidth - 1, 0);
                    ᬯᬮᬶ make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ᬯᬮᬶ true;
            }
        }
        ᬯᬮᬶ false;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool
    tryPushCompareOpThroughConcat(Vertex* const vtxp, DfgConst* constp, DfgConcat* concatp) {
        FileLine* const flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        ᬬᬾᬦ᭄ (!concatp->hasMultipleSinks()  //
            || concatp->lhsp()->is<DfgConst>()  //
            || concatp->rhsp()->is<DfgConst>()) {
            APPLYING(PUSH_COMPARE_OP_THROUGH_CONCAT) {
                const uint32_t width = concatp->width();
                const uint32_t lWidth = concatp->lhsp()->width();
                const uint32_t rWidth = concatp->rhsp()->width();

                // The new Lhs vertex
                DfgConst* const newLhsConstp = makeZero(constp->fileline(), lWidth);
                newLhsConstp->num().opSel(constp->num(), width - 1, rWidth);
                Vertex* const newLhsp
                    = make<Vertex>(flp, m_bitDType, newLhsConstp, concatp->lhsp());

                // The new Rhs vertex
                DfgConst* const newRhsConstp = makeZero(constp->fileline(), rWidth);
                newRhsConstp->num().opSel(constp->num(), rWidth - 1, 0);
                Vertex* const newRhsp
                    = make<Vertex>(flp, m_bitDType, newRhsConstp, concatp->rhsp());

                // The replacement Vertex
                ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::value) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ᬮᬶᬬ ᬬᬾᬦ᭄ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::value) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ᬮᬶᬬ {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ᬯᬮᬶ true;
            }
        }
        ᬯᬮᬶ false;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT bool tryPushBitwiseOpThroughReductions(Bitwise* const vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        ᬬᬾᬦ᭄ (Reduction* const lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            ᬬᬾᬦ᭄ (Reduction* const rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* const lSrcp = lRedp->srcp();
                DfgVertex* const rSrcp = rRedp->srcp();
                ᬬᬾᬦ᭄ (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(PUSH_BITWISE_THROUGH_REDUCTION) {
                        FileLine* const flp = vtxp->fileline();
                        Bitwise* const bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        ᬯᬮᬶ true;
                    }
                }
            }
        }

        ᬯᬮᬶ false;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    static std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        ᬬᬾᬦ᭄ (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ᬯᬮᬶ {nullptr, nullptr};
        }
        ᬬᬾᬦ᭄ (vtxp->hasMultipleSinks()) ᬯᬮᬶ {nullptr, nullptr};
        DfgVertexBinary* const bwp = vtxp->as<DfgVertexBinary>();
        DfgSel* const lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* const rSelp = bwp->rhsp()->cast<DfgSel>();
        ᬬᬾᬦ᭄ (!lSelp || !rSelp) ᬯᬮᬶ {nullptr, nullptr};
        DfgSel* const nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* const wSelp = nSelp == lSelp ? rSelp : lSelp;
        const uint32_t nWidth = nSelp->fromp()->width();
        ᬬᬾᬦ᭄ (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ᬯᬮᬶ {nullptr, nullptr};
        ᬬᬾᬦ᭄ (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            ᬯᬮᬶ {nullptr, nullptr};
        }
        ᬯᬮᬶ {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT bool tryPushBitwiseOpThrougSel(Bitwise* const vtxp) {
        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᬬᬾᬦ᭄ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ᬬᬾᬦ᭄ (DfgSel* const selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (Bitwise* const bitwisep = rhsp->cast<Bitwise>()) {
                ᬬᬾᬦ᭄ (DfgSel* const rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (DfgSel* const rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            ᬬᬾᬦ᭄ (rSelp) {
                DfgVertex* const lFromp = lSelp->fromp();
                DfgVertex* const rFromp = rSelp->fromp();
                ᬬᬾᬦ᭄ (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(PUSH_BITWISE_THROUGH_SEL) {
                        Bitwise* const bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        ᬬᬾᬦ᭄ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ᬯᬮᬶ true;
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
            Bitwise* const rChainp = rhsp->cast<Bitwise>();
            ᬬᬾᬦ᭄ (rChainp && rChainp->hasMultipleSinks()) ᬯᬮᬶ false;
            DfgVertex* const rTermp = rChainp ? rChainp->lhsp() : rhsp;

            const auto lSels = pushableUnalignedSels(lhsp);
            const auto rSels = pushableUnalignedSels(rTermp);
            ᬬᬾᬦ᭄ (!lSels.first && !rSels.first) ᬯᬮᬶ false;

            const DfgVertex* const opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            ᬬᬾᬦ᭄ (opp->is<Bitwise>()) ᬯᬮᬶ false;
            const auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* const nFromp = sels.first->fromp();
            DfgVertex* const wFromp = sels.second->fromp();
            const uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            const auto matches = [&](const DfgVertex* termp, const auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                ᬬᬾᬦ᭄ (tSels.first) {
                    ᬯᬮᬶ termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                ᬬᬾᬦ᭄ (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                ᬬᬾᬦ᭄ (const DfgSel* const selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                ᬬᬾᬦ᭄ (termp->type() != opp->type()) ᬯᬮᬶ false;
                // Operands might have been swapped since
                const DfgVertexBinary* const bwp = termp->as<DfgVertexBinary>();
                const bool nIsLhs = bwp->lhsp() == nFromp;
                ᬬᬾᬦ᭄ (!nIsLhs && bwp->rhsp() != nFromp) ᬯᬮᬶ false;
                const DfgSel* const slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ᬯᬮᬶ slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            ᬬᬾᬦ᭄ (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ᬯᬮᬶ false;

            APPLYING(PUSH_BITWISE_THROUGH_UNALIGNED_SEL) {
                FileLine* const flp = vtxp->fileline();
                const DfgDataType& dtype = nFromp->dtype();
                DfgSel* const slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                ᬬᬾᬦ᭄ (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } ᬮᬶᬬ {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                const auto push = [&](DfgVertex* termp, const DfgSel* nSelp) -> DfgVertex* {
                    ᬯᬮᬶ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* const newLhsp = push(lhsp, lSels.first);
                DfgVertex* const newRTermp = push(rTermp, rSels.first);
                ᬬᬾᬦ᭄ (rChainp) {
                    DfgVertex* const newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } ᬮᬶᬬ {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ᬯᬮᬶ true;
            }
        }

        ᬯᬮᬶ false;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    static bool isConcatFoldable(const DfgVertex* lhsp, const DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (const DfgRep* const lRepp = lhsp->cast<DfgRep>()) {
            ᬬᬾᬦ᭄ (isSame(lRepp->srcp(), rhsp)) ᬯᬮᬶ true;
        }
        ᬬᬾᬦ᭄ (const DfgRep* const rRepp = rhsp->cast<DfgRep>()) {
            ᬬᬾᬦ᭄ (isSame(lhsp, rRepp->srcp())) ᬯᬮᬶ true;
        }
        // Concatenation of adjoining selects is a select
        ᬬᬾᬦ᭄ (const DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
            ᬬᬾᬦ᭄ (const DfgSel* const rSelp = rhsp->cast<DfgSel>()) {
                ᬯᬮᬶ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        ᬬᬾᬦ᭄ (const DfgNot* const lNotp = lhsp->cast<DfgNot>()) {
            ᬬᬾᬦ᭄ (const DfgNot* const rNotp = rhsp->cast<DfgNot>()) {
                ᬯᬮᬶ !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ᬯᬮᬶ false;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    static bool isSameBitwise(const DfgVertex* lhsp, const DfgVertex* rhsp) {
        ᬬᬾᬦ᭄ (lhsp->type() != rhsp->type()) ᬯᬮᬶ false;
        ᬯᬮᬶ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    static bool isConcatSimplifiable(const DfgVertex* lhsp, const DfgVertex* rhsp) {
        ᬬᬾᬦ᭄ (isConcatFoldable(lhsp, rhsp)) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (!isSameBitwise(lhsp, rhsp)) ᬯᬮᬶ false;
        ᬬᬾᬦ᭄ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ᬯᬮᬶ false;
        const DfgVertexBinary* const lp = lhsp->as<DfgVertexBinary>();
        const DfgVertexBinary* const rp = rhsp->as<DfgVertexBinary>();
        ᬬᬾᬦ᭄ (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ᬯᬮᬶ true;
        ᬯᬮᬶ false;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    static bool isConcatPushableThroughBitwise(const DfgVertex* lhsp, const DfgVertex* rhsp,
                                               bool& swap) {
        ᬬᬾᬦ᭄ (!isSameBitwise(lhsp, rhsp)) ᬯᬮᬶ false;
        ᬬᬾᬦ᭄ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ᬯᬮᬶ false;
        const DfgVertexBinary* const lp = lhsp->as<DfgVertexBinary>();
        const DfgVertexBinary* const rp = rhsp->as<DfgVertexBinary>();
        swap = false;
        ᬬᬾᬦ᭄ (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ᬯᬮᬶ true;
        swap = true;
        ᬬᬾᬦ᭄ (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ᬯᬮᬶ true;
        ᬯᬮᬶ false;
    }

    // Create 'Concat(a, c) OP Concat(b, d)' from 'lhsp' = 'a OP b' and 'rhsp' = 'c OP d', or
    // 'Concat(a, d) OP Concat(b, c)' if 'swap' is set
    DfgVertex* makeConcatPushedThroughBitwise(FileLine* flp, const DfgVertex* lhsp,
                                              const DfgVertex* rhsp, bool swap) {
        const DfgVertexBinary* const lp = lhsp->as<DfgVertexBinary>();
        const DfgVertexBinary* const rp = rhsp->as<DfgVertexBinary>();
        const DfgDataType& dtype = DfgDataType::packed(lp->width() + rp->width());
        DfgVertex* const cp = swap ? rp->rhsp() : rp->lhsp();
        DfgVertex* const dp = swap ? rp->lhsp() : rp->rhsp();
        DfgConcat* const newLhsp = make<DfgConcat>(flp, dtype, lp->lhsp(), cp);
        DfgConcat* const newRhsp = make<DfgConcat>(flp, dtype, lp->rhsp(), dp);
        ᬬᬾᬦ᭄ (lp->is<DfgAnd>()) ᬯᬮᬶ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        ᬬᬾᬦ᭄ (lp->is<DfgOr>()) ᬯᬮᬶ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ᬯᬮᬶ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT bool tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᬬᬾᬦ᭄ (DfgSel* const lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ᬬᬾᬦ᭄ (DfgSel* const selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (Bitwise* const rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (Reduction* const rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            ᬬᬾᬦ᭄ (rSelp) {
                uint32_t lsb = 0;
                ᬬᬾᬦ᭄ (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        const DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* const newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        ᬬᬾᬦ᭄ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ᬯᬮᬶ true;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (Reduction* const lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            ᬬᬾᬦ᭄ (!rRedp) {
                ᬬᬾᬦ᭄ (Bitwise* const rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            ᬬᬾᬦ᭄ (rRedp) {
                ᬬᬾᬦ᭄ (DfgSel* const lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    ᬬᬾᬦ᭄ (DfgSel* const rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ᬬᬾᬦ᭄ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                const DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* const newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                ᬬᬾᬦ᭄ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ᬯᬮᬶ true;
                            }
                        }
                    }
                }
            }
        }

        ᬯᬮᬶ false;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT bool optimizeReduction(Reduction* const vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ true;

        DfgVertex* const srcp = vtxp->srcp();
        FileLine* const flp = vtxp->fileline();

        // Reduction of 1-bit value
        ᬬᬾᬦ᭄ (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                ᬯᬮᬶ true;
            }
        }

        ᬬᬾᬦ᭄ (DfgCond* const condp = srcp->cast<DfgCond>()) {
            ᬬᬾᬦ᭄ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(PUSH_REDUCTION_THROUGH_COND_WITH_CONST_BRANCH) {
                    // The new 'then' vertex
                    Reduction* const newThenp = make<Reduction>(flp, m_bitDType, condp->thenp());

                    // The new 'else' vertex
                    Reduction* const newElsep = make<Reduction>(flp, m_bitDType, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* const newCondp = make<DfgCond>(condp->fileline(), m_bitDType,
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ᬯᬮᬶ true;
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgConcat* const concatp = srcp->cast<DfgConcat>()) {
            ᬬᬾᬦ᭄ (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
                || concatp->lhsp()->dtype() == m_bitDType
                || concatp->rhsp()->dtype() == m_bitDType) {
                APPLYING(PUSH_REDUCTION_THROUGH_CONCAT) {
                    // Reduce the parts of the concatenation
                    Reduction* const lRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->lhsp());
                    Reduction* const rRedp
                        = make<Reduction>(concatp->fileline(), m_bitDType, concatp->rhsp());

                    // Bitwise reduce the results
                    replace(make<Bitwise>(flp, m_bitDType, lRedp, rRedp));
                    ᬯᬮᬶ true;
                }
            }
        }

        ᬬᬾᬦ᭄ (Bitwise* const bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            ᬬᬾᬦ᭄ (!bitwisep->hasMultipleSinks()) {
                ᬬᬾᬦ᭄ (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(PUSH_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* const newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* const newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        ᬯᬮᬶ true;
                    }
                }

                ᬬᬾᬦ᭄ (DfgSel* const lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    ᬬᬾᬦ᭄ (DfgSel* const rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ᬬᬾᬦ᭄ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(PUSH_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                const DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* const newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ᬯᬮᬶ true;
                            }
                        }
                    }
                }
            }
        }

        ᬯᬮᬶ false;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT bool optimizeShiftRHS(Shift* const vtxp) {
        static_assert(std::is_base_of<DfgVertexBinary, Shift>::value, "Must invoke on binary");
        static_assert(std::is_final<Shift>::value, "Must invoke on final class");
        ᬬᬾᬦ᭄ (const DfgConcat* const concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            ᬬᬾᬦ᭄ (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ᬯᬮᬶ true;
                }
            }
        }
        ᬯᬮᬶ false;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> addToCountOnesTerm(DfgVertex* vtxp) {
        ᬬᬾᬦ᭄ (DfgConcat* const oCatp = vtxp->cast<DfgConcat>()) {
            ᬬᬾᬦ᭄ (isZero(oCatp->lhsp())) {
                ᬬᬾᬦ᭄ (DfgCountOnes* const countOnesp = oCatp->rhsp()->cast<DfgCountOnes>()) {
                    // Zero extended count ones
                    ᬬᬾᬦ᭄ (DfgSel* const selp = countOnesp->srcp()->cast<DfgSel>()) {
                        ᬯᬮᬶ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (DfgSel* const selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    ᬬᬾᬦ᭄ (selp->dtype() == m_bitDType) {  //
                        ᬯᬮᬶ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            ᬯᬮᬶ {nullptr, 0, 0};
        }
        ᬬᬾᬦ᭄ (DfgCountOnes* const countOnesp = vtxp->cast<DfgCountOnes>()) {
            // Simple count ones
            ᬬᬾᬦ᭄ (DfgSel* const selp = countOnesp->srcp()->cast<DfgSel>()) {
                ᬯᬮᬶ {selp->fromp(), selp->lsb(), selp->width()};
            }
            ᬯᬮᬶ {nullptr, 0, 0};
        }
        ᬬᬾᬦ᭄ (DfgSel* const oSelp = vtxp->cast<DfgSel>()) {
            ᬬᬾᬦ᭄ (oSelp->lsb() == 0) {
                // Truncated count ones
                ᬬᬾᬦ᭄ (DfgCountOnes* const countOnesp = oSelp->fromp()->cast<DfgCountOnes>()) {
                    // Zero extended count ones
                    ᬬᬾᬦ᭄ (DfgSel* const selp = countOnesp->srcp()->cast<DfgSel>()) {
                        ᬯᬮᬶ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            ᬬᬾᬦ᭄ (oSelp->dtype() == m_bitDType) {  //
                ᬯᬮᬶ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ᬯᬮᬶ {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        ᬬᬾᬦ᭄ (DfgShiftR* const shiftrp = vtxp->cast<DfgShiftR>()) {
            ᬬᬾᬦ᭄ (DfgConst* const rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                ᬬᬾᬦ᭄ (rConstp->toU32() == shiftrp->width() - 1) {
                    ᬯᬮᬶ {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            ᬯᬮᬶ {nullptr, 0, 0};
        }
        // Not applicable
        ᬯᬮᬶ {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              const uint32_t width) {
        ᬲᭂᬤᭂᬮ (true) {
            const uint32_t msb = lsb + width - 1;

            // Sel from Concat
            ᬬᬾᬦ᭄ (DfgConcat* const concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* const lhsp = concatp->lhsp();
                DfgVertex* const rhsp = concatp->rhsp();

                ᬬᬾᬦ᭄ (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        continue;
                    }
                } ᬮᬶᬬ ᬬᬾᬦ᭄ (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        continue;
                    }
                }
            }

            ᬬᬾᬦ᭄ (DfgRep* const repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                const uint32_t srcWidth = repp->srcp()->width();
                ᬬᬾᬦ᭄ (width <= srcWidth) {
                    const uint32_t newLsb = lsb % srcWidth;
                    const uint32_t newMsb = newLsb + width - 1;
                    ᬬᬾᬦ᭄ (newMsb < srcWidth) {
                        APPLYING(PUSH_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            continue;
                        }
                    }
                }
            }

            // Sel from Sel
            ᬬᬾᬦ᭄ (DfgSel* const selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    continue;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            ᬬᬾᬦ᭄ (DfgVarPacked* const varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                const auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* const driverp = pair.first;
                const uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                ᬬᬾᬦ᭄ (driverp) {
                    ᬬᬾᬦ᭄ (driverp == varp->defaultp()) {
                        APPLYING(PUSH_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            continue;
                        }
                    } ᬮᬶᬬ {
                        APPLYING(PUSH_SEL_THROUGH_SPLICE) {
                            fromp = driverp;
                            lsb = driverLsb;
                            continue;
                        }
                    }
                }
            }

            // No patterns matched, stop
            bha_nga;
        }
        ᬯᬮᬶ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        ᬬᬾᬦ᭄ (ap == bp) ᬯᬮᬶ {ap, ap->width()};

        // If both constants, check LSBs
        ᬬᬾᬦ᭄ (DfgConst* const aConstp = ap->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (DfgConst* const bConstp = bp->cast<DfgConst>()) {
                const V3Number& aNum = aConstp->num();
                const V3Number& bNum = bConstp->num();
                // Max match is the shorter constant
                const uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                for (; matchWidth < maxMatch; ++matchWidth) {
                    ᬬᬾᬦ᭄ (aNum.bitIs0(matchWidth) != bNum.bitIs0(matchWidth)) bha_nga;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* const shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                ᬯᬮᬶ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        ᬬᬾᬦ᭄ (DfgConcat* const catp = ap->cast<DfgConcat>()) ᬯᬮᬶ commonLSBs(catp->rhsp(), bp);
        ᬬᬾᬦ᭄ (DfgConcat* const catp = bp->cast<DfgConcat>()) ᬯᬮᬶ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        ᬬᬾᬦ᭄ (DfgSel* const selp = ap->cast<DfgSel>()) {
            ᬬᬾᬦ᭄ (selp->lsb() == 0) {
                DfgVertex* const fromp = selp->fromp();
                const std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                ᬯᬮᬶ {common.first, std::min(common.second, selp->width())};
            }
        }
        ᬬᬾᬦ᭄ (DfgSel* const selp = bp->cast<DfgSel>()) {
            ᬬᬾᬦ᭄ (selp->lsb() == 0) {
                DfgVertex* const fromp = selp->fromp();
                const std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                ᬯᬮᬶ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        ᬯᬮᬶ {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        ᬬᬾᬦ᭄ (ap == bp) ᬯᬮᬶ {ap, ap->width()};

        // If both constants, check MSBs
        ᬬᬾᬦ᭄ (DfgConst* const aConstp = ap->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (DfgConst* const bConstp = bp->cast<DfgConst>()) {
                const uint32_t aMsb = aConstp->width() - 1;
                const uint32_t bMsb = bConstp->width() - 1;
                const V3Number& aNum = aConstp->num();
                const V3Number& bNum = bConstp->num();
                // Max match is the shorter constant
                const uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                for (; matchWidth < maxMatch; ++matchWidth) {
                    ᬬᬾᬦ᭄ (aNum.bitIs0(aMsb - matchWidth) != bNum.bitIs0(bMsb - matchWidth)) bha_nga;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* const shorterp = aMsb < bMsb ? aConstp : bConstp;
                ᬯᬮᬶ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        ᬬᬾᬦ᭄ (DfgConcat* const catp = ap->cast<DfgConcat>()) ᬯᬮᬶ commonMSBs(catp->lhsp(), bp);
        ᬬᬾᬦ᭄ (DfgConcat* const catp = bp->cast<DfgConcat>()) ᬯᬮᬶ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        ᬬᬾᬦ᭄ (DfgSel* const selp = ap->cast<DfgSel>()) {
            DfgVertex* const fromp = selp->fromp();
            ᬬᬾᬦ᭄ (selp->msb() == fromp->width() - 1) {
                const std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                ᬯᬮᬶ {common.first, std::min(common.second, selp->width())};
            }
        }
        ᬬᬾᬦ᭄ (DfgSel* const selp = bp->cast<DfgSel>()) {
            DfgVertex* const fromp = selp->fromp();
            ᬬᬾᬦ᭄ (selp->msb() == fromp->width() - 1) {
                const std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                ᬯᬮᬶ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        ᬯᬮᬶ {nullptr, 0};
    }

    // VISIT methods

    void visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    void visit(DfgCountOnes* const vtxp) override {
        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgExtend* const vtxp) override {
        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* const zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ᬯᬮᬶ;
        }
    }

    void visit(DfgExtendS* const vtxp) override {
        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgLogNot* const vtxp) override {
        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgNegate* const vtxp) override {
        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgNot* const vtxp) override {
        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ;

        // Not of Cond
        ᬬᬾᬦ᭄ (DfgCond* const condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            ᬬᬾᬦ᭄ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
                APPLYING(PUSH_NOT_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgNot* const newThenp = make<DfgNot>(vtxp, condp->thenp());

                    // The new 'else' vertex
                    DfgNot* const newElsep = make<DfgNot>(vtxp, condp->elsep());

                    // The replacement Cond vertex
                    DfgCond* const newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ᬯᬮᬶ;
                }
            }
        }

        // Not of Not
        ᬬᬾᬦ᭄ (DfgNot* const notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            ᬬᬾᬦ᭄ (DfgEq* const eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ᬯᬮᬶ;
                }
            }

            // Not of Neq
            ᬬᬾᬦ᭄ (DfgNeq* const neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ᬯᬮᬶ;
                }
            }

            // Not of unsigned comparisons
            ᬬᬾᬦ᭄ (DfgLt* const p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (DfgGte* const p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (DfgGt* const p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (DfgLte* const p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᬯᬮᬶ;
                }
            }

            // Not of signed comparisons
            ᬬᬾᬦ᭄ (DfgLtS* const p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (DfgGteS* const p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (DfgGtS* const p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (DfgLteS* const p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgOneHot* const vtxp) override {
        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgOneHot0* const vtxp) override {
        ᬬᬾᬦ᭄ (foldUnary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgRedOr* const vtxp) override {
        ᬬᬾᬦ᭄ (optimizeReduction(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgRedAnd* const vtxp) override {
        ᬬᬾᬦ᭄ (optimizeReduction(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgRedXor* const vtxp) override {
        ᬬᬾᬦ᭄ (optimizeReduction(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgSel* const vtxp) override {
        DfgVertex* const fromp = vtxp->fromp();

        FileLine* const flp = vtxp->fileline();

        const uint32_t lsb = vtxp->lsb();
        const uint32_t width = vtxp->width();
        const uint32_t msb = lsb + width - 1;

        ᬬᬾᬦ᭄ (DfgConst* const constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* const resp = makeZero(flp, width);
                resp->num().opSel(constp->num(), msb, lsb);
                replace(resp);
                ᬯᬮᬶ;
            }
        }

        // Full width select, replace with the source.
        ᬬᬾᬦ᭄ (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                ᬯᬮᬶ;
            }
        }

        // Unwind through bit packing in one go
        {
            const auto res = unwindSel(fromp, lsb, width);
            ᬬᬾᬦ᭄ (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                ᬯᬮᬶ;
            }
        }

        // Sel from Not
        ᬬᬾᬦ᭄ (DfgNot* const notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            ᬬᬾᬦ᭄ (!notp->hasMultipleSinks()) {
                APPLYING(PUSH_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* const newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ᬯᬮᬶ;
                }
            }
        }

        // Sel from Cond
        ᬬᬾᬦ᭄ (DfgCond* const condp = fromp->cast<DfgCond>()) {
            ᬬᬾᬦ᭄ (!condp->hasMultipleSinks()) {
                APPLYING(PUSH_SEL_THROUGH_COND) {
                    // The new 'then' vertex
                    DfgSel* const newThenp = make<DfgSel>(vtxp, condp->thenp(), lsb);

                    // The new 'else' vertex
                    DfgSel* const newElsep = make<DfgSel>(vtxp, condp->elsep(), lsb);

                    // The replacement Cond vertex
                    DfgCond* const newCondp = make<DfgCond>(condp->fileline(), vtxp->dtype(),
                                                            condp->condp(), newThenp, newElsep);

                    // Replace this vertex
                    replace(newCondp);
                    ᬯᬮᬶ;
                }
            }
        }

        // Sel from ShiftL
        ᬬᬾᬦ᭄ (DfgShiftL* const shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            ᬬᬾᬦ᭄ (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(PUSH_SEL_THROUGH_SHIFTL) {
                    DfgSel* const newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgMux* const vtxp) override {
        DfgVertex* const fromp = vtxp->fromp();
        DfgVertex* const lsbp = vtxp->lsbp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (DfgConst* const lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->num().toUInt()));
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                ᬯᬮᬶ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    void visit(DfgAnd* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ᬬᬾᬦ᭄ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ᬬᬾᬦ᭄ (DfgNot* const lhsNotp = lhsp->cast<DfgNot>()) {
                ᬬᬾᬦ᭄ (DfgNot* const rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* const orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ᬯᬮᬶ;
                    }
                }
                ᬬᬾᬦ᭄ (DfgNeq* const rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* const newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* const orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rhsConcatp)) ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (distributiveAndAssociativeBinary<DfgOr, DfgAnd>(vtxp)) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (tryPushBitwiseOpThroughReductions(vtxp)) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (tryPushBitwiseOpThrougSel(vtxp)) ᬯᬮᬶ;

        {
            DfgNot* const lNotp = lhsp->cast<DfgNot>();
            DfgNot* const rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            ᬬᬾᬦ᭄ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (DfgAnd* const rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* const rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* const rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                ᬬᬾᬦ᭄ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (vtxp->dtype() == m_bitDType) {
            ᬬᬾᬦ᭄ (tryReplaceBitwiseWithReduction(vtxp)) ᬯᬮᬶ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            ᬬᬾᬦ᭄ (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            ᬬᬾᬦ᭄ (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* const newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgOr* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ᬬᬾᬦ᭄ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ᬬᬾᬦ᭄ (DfgNot* const lhsNotp = lhsp->cast<DfgNot>()) {
                ᬬᬾᬦ᭄ (DfgNot* const rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* const andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ᬯᬮᬶ;
                    }
                }
                ᬬᬾᬦ᭄ (DfgNeq* const rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* const newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* const andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgConcat* const lhsConcatp = lhsp->cast<DfgConcat>()) {
            ᬬᬾᬦ᭄ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    ᬬᬾᬦ᭄ (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ᬯᬮᬶ;
                        }
                    }
                    ᬬᬾᬦ᭄ (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ᬯᬮᬶ;
                        }
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rhsConcatp)) ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (distributiveAndAssociativeBinary<DfgAnd, DfgOr>(vtxp)) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (tryPushBitwiseOpThroughReductions(vtxp)) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (tryPushBitwiseOpThrougSel(vtxp)) ᬯᬮᬶ;

        {
            DfgNot* const lNotp = lhsp->cast<DfgNot>();
            DfgNot* const rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            ᬬᬾᬦ᭄ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (DfgOr* const rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* const rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* const rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                ᬬᬾᬦ᭄ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (vtxp->dtype() == m_bitDType) {
            ᬬᬾᬦ᭄ (tryReplaceBitwiseWithReduction(vtxp)) ᬯᬮᬶ;
        }
    }

    void visit(DfgXor* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (DfgConcat* const rConcatp = rhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rConcatp)) ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (tryPushBitwiseOpThroughReductions(vtxp)) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (tryPushBitwiseOpThrougSel(vtxp)) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (vtxp->dtype() == m_bitDType) {
            ᬬᬾᬦ᭄ (tryReplaceBitwiseWithReduction(vtxp)) ᬯᬮᬶ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    void visit(DfgAdd* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                ᬯᬮᬶ;
            }
        }

        const std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = addToCountOnesTerm(lhsp);
        ᬬᬾᬦ᭄ (DfgVertex* const lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = addToCountOnesTerm(rhsp);
            DfgVertex* extrap = nullptr;
            ᬬᬾᬦ᭄ (!std::get<0>(rTerm)) {
                ᬬᬾᬦ᭄ (DfgAdd* const rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = addToCountOnesTerm(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            ᬬᬾᬦ᭄ (DfgVertex* const rVtxp = std::get<0>(rTerm)) {
                ᬬᬾᬦ᭄ (isSame(lVtxp, rVtxp)) {
                    const uint32_t lLsb = std::get<1>(lTerm);
                    const uint32_t rLsb = std::get<1>(rTerm);
                    const uint32_t lWidth = std::get<2>(lTerm);
                    const uint32_t rWidth = std::get<2>(rTerm);
                    bool adjoined = true;
                    uint32_t lsb = 0;
                    ᬬᬾᬦ᭄ (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } ᬮᬶᬬ ᬬᬾᬦ᭄ (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } ᬮᬶᬬ {
                        adjoined = false;
                    }
                    ᬬᬾᬦ᭄ (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_COUNT_ONES) {
                            DfgSel* const selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<DfgCountOnes>(flp, DfgDataType::packed(32), selp);
                            ᬬᬾᬦ᭄ (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } ᬮᬶᬬ ᬬᬾᬦ᭄ (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            ᬬᬾᬦ᭄ (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            ᬯᬮᬶ;
                        }
                    }
                }
            }
        }
    }

    void visit(DfgArraySel* const vtxp) override {
        DfgConst* const idxp = vtxp->bitp()->cast<DfgConst>();
        ᬬᬾᬦ᭄ (!idxp) ᬯᬮᬶ;
        DfgVarArray* const varp = vtxp->fromp()->cast<DfgVarArray>();
        ᬬᬾᬦ᭄ (!varp) ᬯᬮᬶ;
        AstVar* const astVarp = varp->vscp()->varp();
        ᬬᬾᬦ᭄ (astVarp->isForced()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (astVarp->isSigUserRWPublic()) ᬯᬮᬶ;
        DfgVertex* const srcp = varp->srcp();
        ᬬᬾᬦ᭄ (!srcp) {
            ᬬᬾᬦ᭄ (vtxp->isPacked()) {
                ᬬᬾᬦ᭄ (AstConsArray* const iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    ᬬᬾᬦ᭄ (AstConst* const valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(new DfgConst{m_dfg, valp->fileline(), valp->num()});
                            ᬯᬮᬶ;
                        }
                    }
                }
            }
            ᬯᬮᬶ;
        }

        ᬬᬾᬦ᭄ (DfgSpliceArray* const splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* const driverp = splicep->driverAt(idxp->toSizeT());
            ᬬᬾᬦ᭄ (!driverp) ᬯᬮᬶ;
            DfgUnitArray* const uap = driverp->cast<DfgUnitArray>();
            ᬬᬾᬦ᭄ (!uap) ᬯᬮᬶ;
            ᬬᬾᬦ᭄ (uap->srcp()->is<DfgVertexSplice>()) ᬯᬮᬶ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ᬬᬾᬦ᭄ (DfgVertexVar* const dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ᬬᬾᬦ᭄ (dvarp->srcp()) ᬯᬮᬶ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (DfgUnitArray* const uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            ᬬᬾᬦ᭄ (uap->srcp()->is<DfgSplicePacked>()) ᬯᬮᬶ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ᬬᬾᬦ᭄ (DfgVertexVar* const dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ᬬᬾᬦ᭄ (dvarp->srcp()) ᬯᬮᬶ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgConcat* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isZero(lhsp)) {
            DfgConst* const lConstp = lhsp->as<DfgConst>();
            ᬬᬾᬦ᭄ (DfgSel* const rSelp = rhsp->cast<DfgSel>()) {
                ᬬᬾᬦ᭄ (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_TOP_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (isZero(rhsp)) {
            DfgConst* const rConstp = rhsp->as<DfgConst>();
            ᬬᬾᬦ᭄ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
                ᬬᬾᬦ᭄ (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgNot* const lNot = lhsp->cast<DfgNot>()) {
            ᬬᬾᬦ᭄ (DfgNot* const rNot = rhsp->cast<DfgNot>()) {
                ᬬᬾᬦ᭄ (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(PUSH_CONCAT_THROUGH_NOTS) {
                        DfgConcat* const newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        {
            bool swap = false;
            ᬬᬾᬦ᭄ (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(PUSH_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    ᬯᬮᬶ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ᬬᬾᬦ᭄ (DfgConcat* const rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* const rlp = rConcatp->lhsp();
                ᬬᬾᬦ᭄ (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(PUSH_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* const newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
            ᬬᬾᬦ᭄ (DfgSel* const rSelp = rhsp->cast<DfgSel>()) {
                ᬬᬾᬦ᭄ (isSame(lSelp->fromp(), rSelp->fromp())) {
                    ᬬᬾᬦ᭄ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ᬯᬮᬶ;
                        }
                    }
                }
            }

            ᬬᬾᬦ᭄ (DfgConcat* const rConcatp = rhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (DfgSel* const rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    ᬬᬾᬦ᭄ (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        ᬬᬾᬦ᭄ (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                const uint32_t width = lSelp->width() + rlSelp->width();
                                const DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* const selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ᬯᬮᬶ;
                            }
                        }
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgSel* const rSelp = rhsp->cast<DfgSel>()) {
            ᬬᬾᬦ᭄ (DfgConcat* const lConcatp = lhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (DfgSel* const lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    ᬬᬾᬦ᭄ (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        ᬬᬾᬦ᭄ (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                const uint32_t width = lrSelp->width() + rSelp->width();
                                const DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* const selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ᬯᬮᬶ;
                            }
                        }
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (DfgCond* const rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* const rtVtxp = rCondp->thenp();
                DfgVertex* const reVtxp = rCondp->elsep();
                DfgConst* const rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* const reConstp = reVtxp->cast<DfgConst>();
                ᬬᬾᬦ᭄ (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(PUSH_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* const thenp = [&]() -> DfgVertex* {
                            FileLine* const rtFlp = rtVtxp->fileline();
                            ᬬᬾᬦ᭄ (rtConstp) {
                                DfgConst* const constp = makeZero(rtFlp, vtxp->width());
                                constp->num().opConcat(lConstp->num(), rtConstp->num());
                                ᬯᬮᬶ constp;
                            }
                            ᬯᬮᬶ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* const elsep = [&]() -> DfgVertex* {
                            FileLine* const reFlp = reVtxp->fileline();
                            ᬬᬾᬦ᭄ (reConstp) {
                                DfgConst* const constp = makeZero(reFlp, vtxp->width());
                                constp->num().opConcat(lConstp->num(), reConstp->num());
                                ᬯᬮᬶ constp;
                            }
                            ᬯᬮᬶ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const rConstp = rhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (DfgCond* const lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* const ltVtxp = lCondp->thenp();
                DfgVertex* const leVtxp = lCondp->elsep();
                DfgConst* const ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* const leConstp = leVtxp->cast<DfgConst>();
                ᬬᬾᬦ᭄ (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(PUSH_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* const thenp = [&]() -> DfgVertex* {
                            FileLine* const ltFlp = ltVtxp->fileline();
                            ᬬᬾᬦ᭄ (ltConstp) {
                                DfgConst* const constp = makeZero(ltFlp, vtxp->width());
                                constp->num().opConcat(ltConstp->num(), rConstp->num());
                                ᬯᬮᬶ constp;
                            }
                            ᬯᬮᬶ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* const elsep = [&]() -> DfgVertex* {
                            FileLine* const leFlp = leVtxp->fileline();
                            ᬬᬾᬦ᭄ (leConstp) {
                                DfgConst* const constp = makeZero(leFlp, vtxp->width());
                                constp->num().opConcat(leConstp->num(), rConstp->num());
                                ᬯᬮᬶ constp;
                            }
                            ᬯᬮᬶ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        // Attempt to narrow a concatenation that produces unused bits on the edges
        {
            const uint32_t vMsb = vtxp->width() - 1;  // MSB of the concatenation
            const uint32_t lLsb = vtxp->rhsp()->width();  // LSB of the LHS
            const uint32_t rMsb = lLsb - 1;  // MSB of the RHS
            // Check each sink, and record the range of bits used by them
            uint32_t lsb = vMsb;  // LSB used by a sink
            uint32_t msb = 0;  // MSB used by a sink
            bool hasCrossSink = false;  // True if some sinks use bits from both sides
            vtxp->foreachSink([&](DfgVertex& sink) {
                // Record bits used by DfgSel sinks
                ᬬᬾᬦ᭄ (const DfgSel* const selp = sink.cast<DfgSel>()) {
                    const uint32_t selLsb = selp->lsb();
                    const uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ᬯᬮᬶ false;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                ᬬᬾᬦ᭄ (const DfgVarPacked* const varp = sink.cast<DfgVarPacked>()) {
                    ᬬᬾᬦ᭄ (!varp->hasSinks() && !varp->isObserved()) ᬯᬮᬶ false;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ᬯᬮᬶ true;
            });
            ᬬᬾᬦ᭄ (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    ᬬᬾᬦ᭄ (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    ᬬᬾᬦ᭄ (msb != vMsb)
                        nLhsp = make<DfgSel>(flp, DfgDataType::packed(msb - lLsb + 1), lhsp, 0U);
                    // Narrowed concatenation
                    DfgVertex* const catp
                        = make<DfgConcat>(flp, DfgDataType::packed(msb - lsb + 1), nLhsp, nRhsp);

                    // Need to insert via a partial splice to avoid infinite matching,
                    // this splice will be eliminated on later visits to its sinks.
                    DfgSplicePacked* const sp = new DfgSplicePacked{m_dfg, flp, vtxp->dtype()};
                    m_vInfo[sp].m_id = ++m_lastId;
                    sp->addDriver(catp, lsb, flp);
                    DfgVertexVar* const varp
                        = m_dfg.makeNewVar(flp, "PeepholeNarrow", vtxp->dtype(), m_tmpScopep);
                    varp->tmpForp(varp->vscp());
                    m_vInfo[varp].m_id = ++m_lastId;
                    varp->vscp()->varp()->isInternal(true);
                    varp->srcp(sp);
                    replace(varp);
                    ᬯᬮᬶ;
                }
            }
        }

        // Convert various forms to REPLICATE
        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ᬯᬮᬶ;
            }
        }
        ᬬᬾᬦ᭄ (DfgRep* const lRepp = lhsp->cast<DfgRep>()) {
            ᬬᬾᬦ᭄ (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ᬯᬮᬶ;
                }
            }
        }
        ᬬᬾᬦ᭄ (DfgRep* const rRepp = rhsp->cast<DfgRep>()) {
            ᬬᬾᬦ᭄ (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ᬯᬮᬶ;
                }
            }
        }
        ᬬᬾᬦ᭄ (DfgRep* const lRepp = lhsp->cast<DfgRep>()) {
            ᬬᬾᬦ᭄ (DfgRep* const rRepp = rhsp->cast<DfgRep>()) {
                ᬬᬾᬦ᭄ (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ᬯᬮᬶ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ᬬᬾᬦ᭄ (DfgConcat* const rConcatp = rhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (DfgRep* const rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    ᬬᬾᬦ᭄ (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            const uint32_t width = lRepp->width() + rlRepp->width();
                            const DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* const repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ᬯᬮᬶ;
                        }
                    }
                }
            }
        }
    }

    void visit(DfgDiv* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (DfgConst* const rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgDivS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (DfgConst* const rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgGt* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgGtS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgGte* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgGteS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgLogAnd* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp) || vtxp->rhsp()->unsafe()) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᬬᬾᬦ᭄ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgLogEq* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgLogIf* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp) || vtxp->rhsp()->unsafe()) ᬯᬮᬶ;
    }

    void visit(DfgLogOr* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp) || vtxp->rhsp()->unsafe()) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᬬᬾᬦ᭄ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgLt* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgLtS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgLte* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgLteS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ᬯᬮᬶ;
            }
        }
    }

    void visit(DfgModDiv* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgModDivS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgMul* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgMulS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgEq* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const lhsConstp = lhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (tryPushCompareOpThroughConcat(vtxp, lhsConstp, rhsConcatp)) ᬯᬮᬶ;
            }

            ᬬᬾᬦ᭄ (rhsp->dtype() == m_bitDType) {
                ᬬᬾᬦ᭄ (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ᬯᬮᬶ;
                    }
                }

                ᬬᬾᬦ᭄ (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        ᬯᬮᬶ;
                    }
                }
            }
        }
    }

    void visit(DfgNeq* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const lhsConstp = lhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (tryPushCompareOpThroughConcat(vtxp, lhsConstp, rhsConcatp)) ᬯᬮᬶ;
            }

            ᬬᬾᬦ᭄ (rhsp->dtype() == m_bitDType) {
                ᬬᬾᬦ᭄ (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        ᬯᬮᬶ;
                    }
                }

                ᬬᬾᬦ᭄ (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ᬯᬮᬶ;
                    }
                }
            }
        }
    }

    void visit(DfgPow* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgPowSS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgPowSU* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgPowUS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgRep* const vtxp) override {
        DfgVertex* const srcp = vtxp->srcp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (DfgConst* const sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* const resultp = makeZero(flp, vtxp->width());
                resultp->num().opRepl(sConstp->num(), vtxp->count());
                replace(resultp);
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (DfgRep* const sRepp = srcp->cast<DfgRep>()) {
            ᬬᬾᬦ᭄ (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgShiftL* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (optimizeShiftRHS(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᬬᬾᬦ᭄ (DfgShiftL* const lShiftLp = lhsp->cast<DfgShiftL>()) {
            ᬬᬾᬦ᭄ (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    FileLine* const flp = vtxp->fileline();
                    const DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* const bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* const cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* const addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    ᬯᬮᬶ;
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const rConstp = rhsp->cast<DfgConst>()) {
            const uint32_t shiftAmount = rConstp->toU32();

            ᬬᬾᬦ᭄ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (DfgConcat* const lConcatp = lhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_CAT) {
                        DfgVertex* const lRhsp = lConcatp->rhsp();
                        DfgVertex* const lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        const uint32_t rW = shiftAmount;
                        const uint32_t mW = std::min(lRhsp->width(), vtxp->width() - shiftAmount);
                        const uint32_t lW = vtxp->width() - mW - rW;
                        // Construct the result
                        FileLine* const flp = lConcatp->fileline();
                        DfgVertex* const rp = makeZero(flp, shiftAmount);
                        DfgVertex* const mp
                            = make<DfgSel>(flp, DfgDataType::packed(mW), lRhsp, 0U);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(mW + rW), mp, rp);
                        ᬬᬾᬦ᭄ (!lW) {
                            replace(np);
                            ᬯᬮᬶ;
                        }
                        DfgVertex* const lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ᬯᬮᬶ;
                    }
                }
            }

            ᬬᬾᬦ᭄ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
                ᬬᬾᬦ᭄ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        const uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* const nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        ᬯᬮᬶ;
                    }
                }
            }

            ᬬᬾᬦ᭄ (DfgCond* const lCondp = lhsp->cast<DfgCond>()) {
                ᬬᬾᬦ᭄ (!lCondp->hasMultipleSinks()) {
                    APPLYING(PUSH_SHIFTL_THROUGH_COND) {
                        DfgShiftL* const tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* const ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ᬯᬮᬶ;
                    }
                }
            }
        }
    }

    void visit(DfgShiftR* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (optimizeShiftRHS(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᬬᬾᬦ᭄ (DfgShiftR* const lShiftRp = lhsp->cast<DfgShiftR>()) {
            ᬬᬾᬦ᭄ (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    FileLine* const flp = vtxp->fileline();
                    const DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* const bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* const cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* const addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    ᬯᬮᬶ;
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const rConstp = rhsp->cast<DfgConst>()) {
            const uint32_t shiftAmount = rConstp->toU32();

            ᬬᬾᬦ᭄ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (DfgConcat* const lConcatp = lhsp->cast<DfgConcat>()) {
                ᬬᬾᬦ᭄ (!lConcatp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_CAT) {
                        DfgVertex* const lRhsp = lConcatp->rhsp();
                        DfgVertex* const lLhsp = lConcatp->lhsp();
                        // Compute widths of 3 possible result parts
                        const uint32_t lW = shiftAmount;
                        const uint32_t mW = std::min(lLhsp->width(), vtxp->width() - shiftAmount);
                        const uint32_t rW = vtxp->width() - mW - lW;
                        // Construct the result
                        FileLine* const flp = lConcatp->fileline();
                        DfgVertex* const lp = makeZero(flp, shiftAmount);
                        DfgVertex* const mp = make<DfgSel>(flp, DfgDataType::packed(mW), lLhsp,
                                                           lLhsp->width() - mW);
                        DfgVertex* np = make<DfgConcat>(flp, DfgDataType::packed(lW + mW), lp, mp);
                        ᬬᬾᬦ᭄ (!rW) {
                            replace(np);
                            ᬯᬮᬶ;
                        }
                        DfgVertex* const rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ᬯᬮᬶ;
                    }
                }
            }

            ᬬᬾᬦ᭄ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
                ᬬᬾᬦ᭄ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        const uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* const nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        ᬯᬮᬶ;
                    }
                }
            }

            ᬬᬾᬦ᭄ (DfgCond* const lCondp = lhsp->cast<DfgCond>()) {
                ᬬᬾᬦ᭄ (!lCondp->hasMultipleSinks()) {
                    APPLYING(PUSH_SHIFTR_THROUGH_COND) {
                        DfgShiftR* const tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* const ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ᬯᬮᬶ;
                    }
                }
            }
        }
    }

    void visit(DfgShiftRS* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (optimizeShiftRHS(vtxp)) ᬯᬮᬶ;
    }

    void visit(DfgSub* const vtxp) override {
        ᬬᬾᬦ᭄ (binary(vtxp)) ᬯᬮᬶ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (DfgConst* const rConstp = rhsp->cast<DfgConst>()) {
            ᬬᬾᬦ᭄ (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (vtxp->dtype() == m_bitDType && rConstp->hasValue(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgMatchMasked* const vtxp) override {
        ᬬᬾᬦ᭄ (DfgConst* const constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* const matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->num(), matchVarp)));
                ᬯᬮᬶ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexTernary
    //=========================================================================

    void visit(DfgCond* const vtxp) override {
        DfgVertex* const condp = vtxp->condp();
        DfgVertex* const thenp = vtxp->thenp();
        DfgVertex* const elsep = vtxp->elsep();
        FileLine* const flp = vtxp->fileline();

        ᬬᬾᬦ᭄ (condp->dtype() != m_bitDType) ᬯᬮᬶ;

        ᬬᬾᬦ᭄ (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (DfgNot* const condNotp = condp->cast<DfgNot>()) {
            ᬬᬾᬦ᭄ (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ᬯᬮᬶ;
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgNeq* const condNeqp = condp->cast<DfgNeq>()) {
            ᬬᬾᬦ᭄ (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* const newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ᬯᬮᬶ;
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgNot* const thenNotp = thenp->cast<DfgNot>()) {
            ᬬᬾᬦ᭄ (DfgNot* const elseNotp = elsep->cast<DfgNot>()) {
                ᬬᬾᬦ᭄ (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* const newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgOr* const condOrp = condp->cast<DfgOr>()) {
            ᬬᬾᬦ᭄ (DfgCond* const thenCondp = thenp->cast<DfgCond>()) {
                ᬬᬾᬦ᭄ (!thenCondp->hasMultipleSinks()) {
                    ᬬᬾᬦ᭄ (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* const ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ᬯᬮᬶ;
                        }
                    }
                    ᬬᬾᬦ᭄ (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* const ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ᬯᬮᬶ;
                        }
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            ᬬᬾᬦ᭄ (DfgAdd* const thenAddp = thenp->cast<DfgAdd>()) {
                ᬬᬾᬦ᭄ (DfgConst* const constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    ᬬᬾᬦ᭄ (constp->hasValue(1)) {
                        ᬬᬾᬦ᭄ (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* const extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                FileLine* const thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ᬯᬮᬶ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            ᬬᬾᬦ᭄ (DfgSub* const thenSubp = thenp->cast<DfgSub>()) {
                ᬬᬾᬦ᭄ (DfgConst* const constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    ᬬᬾᬦ᭄ (constp->hasValue(1)) {
                        ᬬᬾᬦ᭄ (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* const extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                FileLine* const thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ᬯᬮᬶ;
                            }
                        }
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (vtxp->dtype() == m_bitDType) {
            ᬬᬾᬦ᭄ (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ᬯᬮᬶ;
                }
            }
        }

        ᬬᬾᬦ᭄ (vtxp->width() <= VL_QUADSIZE) {
            ᬬᬾᬦ᭄ (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* const maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* const maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* const maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ᬯᬮᬶ;
                }
            }
            ᬬᬾᬦ᭄ (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* const maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ᬯᬮᬶ;
                }
            }

            ᬬᬾᬦ᭄ (DfgOr* const tOrp = thenp->cast<DfgOr>()) {
                ᬬᬾᬦ᭄ (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* const maskp = replicate(vtxp, condp);
                        DfgAnd* const andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ᬯᬮᬶ;
                    }
                }
                ᬬᬾᬦ᭄ (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* const maskp = replicate(vtxp, condp);
                        DfgAnd* const andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ᬯᬮᬶ;
                    }
                }
            }
        }

        ᬬᬾᬦ᭄ (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            const std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            ᬬᬾᬦ᭄ (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    const uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    ᬬᬾᬦ᭄ (rWidth != rhsp->width()) {
                        const DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    ᬬᬾᬦ᭄ (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ᬯᬮᬶ;
                    }
                    // Create new LHS
                    const uint32_t lWidth = vtxp->width() - rWidth;
                    const DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* const lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* const lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* const lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ᬯᬮᬶ;
                }
            }

            const std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            ᬬᬾᬦ᭄ (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    const uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    ᬬᬾᬦ᭄ (lWidth != lhsp->width()) {
                        const DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    ᬬᬾᬦ᭄ (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ᬯᬮᬶ;
                    }
                    // Create new RHS
                    const uint32_t rWidth = vtxp->width() - lWidth;
                    const DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* const rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* const rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* const rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ᬯᬮᬶ;
                }
            }
        }

        ᬬᬾᬦ᭄ (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                ᬬᬾᬦ᭄ (const uint32_t extend = vtxp->width() - 1) {
                    DfgConst* const zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                ᬬᬾᬦ᭄ (const uint32_t extend = vtxp->width() - 1) {
                    DfgConst* const zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ᬯᬮᬶ;
            }
        }

        ᬬᬾᬦ᭄ (DfgCond* const tCondp = thenp->cast<DfgCond>()) {
            ᬬᬾᬦ᭄ (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ᬯᬮᬶ;
                }
            }
        }

        ᬬᬾᬦ᭄ (DfgCond* const eCondp = elsep->cast<DfgCond>()) {
            ᬬᬾᬦ᭄ (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ᬯᬮᬶ;
                }
            }
        }
    }

    void visit(DfgVertexVar* const vtxp) override {
        ᬬᬾᬦ᭄ (vtxp->hasSinks()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (vtxp->isObserved()) ᬯᬮᬶ;
        ᬬᬾᬦ᭄ (vtxp->defaultp()) ᬯᬮᬶ;

        // If undriven, or driven from another var, it is completely redundant.
        ᬬᬾᬦ᭄ (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ᬯᬮᬶ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        bool foundOne = false;
        const bool keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> bool {
            // Ignore non-observable variable sinks. These can be eliminated.
            ᬬᬾᬦ᭄ (const DfgVertexVar* const varp = sink.cast<DfgVertexVar>()) {
                ᬬᬾᬦ᭄ (!varp->hasSinks() && !varp->isObserved()) ᬯᬮᬶ false;
            }
            // Keep if found more than one sink
            ᬬᬾᬦ᭄ (foundOne) ᬯᬮᬶ true;
            // Mark first sink found
            foundOne = true;
            ᬯᬮᬶ false;
        });
        ᬬᬾᬦ᭄ (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ᬯᬮᬶ;
            }
        }
    }

    V3DfgPeephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx)
        : m_dfg{dfg}
        , m_ctx{ctx} {

        // Assign vertex IDs
        m_dfg.forEachVertex([&](DfgVertex& vtx) { m_vInfo[vtx].m_id = ++m_lastId; });

        // Add all operation vertices to the cache
        for (DfgVertex& vtx : m_dfg.opVertices()) cacheVertex(&vtx);

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
        for (DfgVertexVar& vtx : m_dfg.varVertices()) addToWorkList(&vtx);

        // Add all operation vertices to the work list
        for (DfgVertex& vtx : m_dfg.opVertices()) addToWorkList(&vtx);

        // Process iteratively
        ᬲᭂᬤᭂᬮ (true) {
            // Process the work list - keep the placeholder at index 0
            ᬲᭂᬤᭂᬮ (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.pop_back();
                ᬬᬾᬦ᭄ (!m_vtxp) continue;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                ᬬᬾᬦ᭄ (DfgVertexVar* const varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    continue;
                }

                // Ast references are not considered
                ᬬᬾᬦ᭄ (m_vtxp->is<DfgVertexAst>()) continue;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                ᬬᬾᬦ᭄ (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    ᬬᬾᬦ᭄ (DfgVertex* const sampep = cacheVertex(m_vtxp)) {
                        APPLYING(REPLACE_WITH_EQUIVALENT) {
                            replace(sampep);
                            continue;
                        }
                    }
                }

                // Visit vertex, might get deleted in the process
                iterate(m_vtxp);
            }

            // If nothing was added to the iter list, we can stop
            ᬬᬾᬦ᭄ (m_iterList.size() == 1) bha_nga;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                for (size_t hops = 1; hops <= 4; ++hops) {
                    for (size_t i = begin; i < end; ++i) {
                        DfgVertex* const vtxp = m_iterList[i];
                        ᬬᬾᬦ᭄ (!vtxp) continue;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            ᬯᬮᬶ false;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            ᬯᬮᬶ false;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            for (DfgVertex* const vtxp : m_iterList) {
                ᬬᬾᬦ᭄ (!vtxp) continue;
                removeFromIterList(vtxp);
                addToWorkList(vtxp);
            }
            // Reset the iter list
            m_iterList.resize(1);
        }
    }

#undef APPLYING

public:
    static void apply(DfgGraph& dfg, V3DfgPeepholeContext& ctx) { V3DfgPeephole{dfg, ctx}; }
};

V3DebugBisect V3DfgPeephole::s_debugBisect{"DfgPeephole"};

void V3DfgPasses::peephole(DfgGraph& dfg, V3DfgPeepholeContext& ctx) {
    ᬬᬾᬦ᭄ (!v3Global.opt.fDfgPeephole()) ᬯᬮᬶ;
    V3DfgPeephole::apply(dfg, ctx);
}
