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
template <> const DfgDataType& resultDType<DfgAdd>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgAnd>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgConcat>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(lhsp->width() + rhsp->width()); }
template <> const DfgDataType& resultDType<DfgDiv>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgDivS>       (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgEq>         (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgGt>         (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgGtS>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgGte>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgGteS>       (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLogAnd>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLogEq>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLogIf>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLogOr>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLt>         (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLtS>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLte>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgLteS>       (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgModDiv>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgModDivS>    (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgMul>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgMulS>       (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgNeq>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ DfgDataType::packed(1); }
template <> const DfgDataType& resultDType<DfgOr>         (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgPow>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgPowSS>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgPowSU>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgPowUS>      (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgShiftL>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgShiftR>     (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgShiftRS>    (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgSub>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }
template <> const DfgDataType& resultDType<DfgXor>        (const DfgVertex* lhsp, const DfgVertex* rhsp) { ᰀᰧᰌ lhsp->dtype(); }

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

#define APPLYING(id) ᰀᰧ (checkApplying(VDfgPeepholePattern::id))

    // METHODS
    bool checkApplying(VDfgPeepholePattern id) {
        ᰀᰧ (VL_UNLIKELY(!m_ctx.m_enabled[id] || s_debugBisect.isStopped())) ᰀᰧᰌ false;
        UINFO(9, "Applying DFG pattern " << id.ascii());
        ++m_ctx.m_count[id];
        ᰀᰧᰌ true;
    }

    // Add vertex to the cache. If an equivalent (but different) vertex is
    // already cached, it is returned and the cache is not updated.
    DfgVertex* cacheVertex(DfgVertex* vtxp) {
        DfgVertex* const equivp = m_cache.cache(vtxp);
        UASSERT_OBJ(!m_vInfo[vtxp].m_isCachedVertex || !equivp, vtxp,
                    "Vertex marked 'm_isCachedVertex' has a cached equivalent");
        m_vInfo[vtxp].m_isCachedVertex = !equivp;
        ᰀᰧᰌ equivp;
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
        ᰀᰧᰌ vtxp;
    }

    // Find the vertex of the given type with the given operands in the
    // cache, or nullptr if there is no such vertex.
    template <typename Vertex, typename... Operands>
    Vertex* getVertex(const DfgDataType& dtype, Operands... operands) {
        Vertex* const vtxp = m_cache.get<Vertex>(dtype, operands...);
        UASSERT_OBJ(!vtxp || m_vInfo[vtxp].m_isCachedVertex, vtxp,
                    "Cached vertex not marked 'm_isCachedVertex'");
        ᰀᰧᰌ vtxp;
    }

    void incrementGeneration() {
        ++m_currentGeneration;
        // TODO: could sweep on overflow
    }

    void addToWorkList(DfgVertex* vtxp) {
        VertexInfo& vInfo = m_vInfo[*vtxp];
        // If already on work list, ignore
        ᰀᰧ (vInfo.m_workListIndex) ᰀᰧᰌ;
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
        ᰀᰧ (vInfo.m_iterListIndex) ᰀᰧᰌ;
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
            ᰀᰧ (src.is<DfgVertexVar>()) {
                addToWorkList(&src);
                ᰀᰧᰌ false;
            }
            // Gather unique sources
            VertexInfo& vInfo = m_vInfo[src];
            ᰀᰧ (vInfo.m_generation != m_currentGeneration) srcps.push_back(&src);
            vInfo.m_generation = m_currentGeneration;
            ᰀᰧᰌ false;
        });

        // This pass only removes variables that are either not driven in this graph,
        // or are not observable outside the graph. If there is also no external write
        // to the variable and no references in other graph then delete the Ast var too.
        ᰀᰧ (varp && !varp->isVolatile() && !varp->hasDfgRefs()) {
            m_ctx.m_deleteps.push_back(varp->vscp());
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        } ᰀᰧᰀ {
            VL_DO_DANGLING(vtxp->unlinkDelete(m_dfg), vtxp);
        }

        // Partition sources into used/unused after removing the vertex
        const auto mid
            = std::stable_partition(srcps.begin(), srcps.end(), [](DfgVertex* srcp) {  //
                  ᰀᰧᰌ srcp->hasSinks();
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
                ᰀᰧᰌ cone->count(&v);
            });
        };
        ᰀᰧ (VL_UNLIKELY(s_debugBisect.stop(debugCallback))) ᰀᰧᰌ;

        // Add sources of the original vertex to the iter list - their sinks are changing
        m_vtxp->foreachSource([&](DfgVertex& src) {
            addToIterList(&src);
            ᰀᰧᰌ false;
        });

        // Remove sinks of the original vertex from the cache - their inputs are changing
        m_vtxp->foreachSink([&](DfgVertex& dst) {
            invalidateVertex(&dst);
            ᰀᰧᰌ false;
        });
        // Replace vertex with the replacement
        m_vtxp->replaceWith(resp);
        // Re-cache all sinks of the replacement
        resp->foreachSink([&](DfgVertex& dst) {
            cacheVertex(&dst);
            ᰀᰧᰌ false;
        });

        // Original vertex is now unused, so delete it
        deleteVertex(m_vtxp);

        // Add new vertex to iter list to consider neighborhood
        addToIterList(resp);
        // Add new vertex and sinks to work list as likely to match
        addToWorkList(resp);
        resp->foreachSink([&](DfgVertex& dst) {
            addToWorkList(&dst);
            ᰀᰧᰌ false;
        });
    }

    // Create a 32-bit DfgConst vertex
    DfgConst* makeI32(FileLine* flp, uint32_t val) { ᰀᰧᰌ new DfgConst{m_dfg, flp, 32, val}; }

    // Create a DfgConst vertex with the given width and value zero
    DfgConst* makeZero(FileLine* flp, uint32_t width) {
        ᰀᰧᰌ new DfgConst{m_dfg, flp, width, 0};
    }
    DfgConst* makeZero(FileLine* flp, const DfgDataType& dtype) {
        ᰀᰧᰌ makeZero(flp, dtype.size());
    }

    // Create a DfgConst vertex with the given width and value all ones
    DfgConst* makeOnes(FileLine* flp, uint32_t width) {
        DfgConst* const resp = makeZero(flp, width);
        resp->num().setAllBits1();
        ᰀᰧᰌ resp;
    }

    // Create a new vertex of the given type
    template <typename Vertex, typename... Operands>
    Vertex* make(FileLine* flp, const DfgDataType& dtype, Operands... operands) {
        // Find or create an equivalent vertex
        Vertex* const vtxp = getOrCreateVertex<Vertex, Operands...>(flp, dtype, operands...);
        // Sanity check
        UASSERT_OBJ(vtxp->dtype() == dtype, vtxp, "Vertex dtype mismatch");
        ᰀᰧ (VL_UNLIKELY(v3Global.opt.debugCheck())) vtxp->typeCheck(m_dfg);
        // Assign vertex ID
        VertexInfo& vInfo = m_vInfo[vtxp];
        ᰀᰧ (!vInfo.m_id) vInfo.m_id = ++m_lastId;
        // Add to work list
        addToWorkList(vtxp);
        // Return new node
        ᰀᰧᰌ vtxp;
    }

    // Same as above, but 'flp' and 'dtypep' are taken from the given example vertex
    template <typename Vertex, typename... Operands>
    Vertex* make(const DfgVertex* examplep, Operands... operands) {
        ᰀᰧᰌ make<Vertex>(examplep->fileline(), examplep->dtype(), operands...);
    }

    // Replicate 'bitp' to 'vtxp->width()' bits
    DfgVertex* replicate(DfgVertex* vtxp, DfgVertex* bitp) {
        UASSERT_OBJ(bitp->dtype() == m_bitDType, vtxp, "Expected 1-bit vertex");
        ᰀᰧ (vtxp->dtype() == m_bitDType) ᰀᰧᰌ bitp;
        ᰀᰧᰌ make<DfgRep>(vtxp, bitp);
    }

    // Check two vertex are the same, or the same constant value
    static bool isSame(const DfgVertex* ap, const DfgVertex* bp) {
        ᰀᰧ (ap == bp) ᰀᰧᰌ true;
        ᰀᰧ (ap->dtype() != bp->dtype()) ᰀᰧᰌ false;
        const DfgConst* const aConstp = ap->cast<DfgConst>();
        ᰀᰧ (!aConstp) ᰀᰧᰌ false;
        const DfgConst* const bConstp = bp->cast<DfgConst>();
        ᰀᰧ (!bConstp) ᰀᰧᰌ false;
        ᰀᰧᰌ aConstp->num().isCaseEq(bConstp->num());
    }

    static bool isZero(const DfgVertex* vtxp) {
        ᰀᰧ (const DfgConst* const constp = vtxp->cast<DfgConst>()) ᰀᰧᰌ constp->isZero();
        ᰀᰧᰌ false;
    }

    static bool isOnes(const DfgVertex* vtxp) {
        ᰀᰧ (const DfgConst* const constp = vtxp->cast<DfgConst>()) ᰀᰧᰌ constp->isOnes();
        ᰀᰧᰌ false;
    }

    static bool isEqOne(const DfgVertex* vtxp) {
        ᰀᰧ (const DfgConst* const constp = vtxp->cast<DfgConst>()) ᰀᰧᰌ constp->num().isEqOne();
        ᰀᰧᰌ false;
    }

    static bool areAdjacent(uint32_t& lsb, const DfgSel* lSelp, const DfgSel* rSelp) {
        ᰀᰧ (!isSame(lSelp->srcp(), rSelp->srcp())) ᰀᰧᰌ false;
        ᰀᰧ (lSelp->lsb() + lSelp->width() == rSelp->lsb()) {
            lsb = lSelp->lsb();
            ᰀᰧᰌ true;
        }
        ᰀᰧ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
            lsb = rSelp->lsb();
            ᰀᰧᰌ true;
        }
        ᰀᰧᰌ false;
    }

    // Note: If any of the following transformers return true, then the vertex was replaced and the
    // caller must not do any further changes, so the caller must check the return value, otherwise
    // there will be hard to debug issues.

    // Constant fold unary vertex, return true if folded
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool foldUnary(Vertex* const vtxp) {
        static_assert(std::is_base_of<DfgVertexUnary, Vertex>::value, "Must invoke on unary");
        static_assert(std::is_final<Vertex>::value, "Must invoke on final class");
        ᰀᰧ (DfgConst* const srcp = vtxp->srcp()->template cast<DfgConst>()) {
            APPLYING(FOLD_UNARY) {
                DfgConst* const resultp = makeZero(vtxp->fileline(), vtxp->width());
                foldOp<Vertex>(resultp->num(), srcp->num());
                replace(resultp);
                ᰀᰧᰌ true;
            }
        }
        ᰀᰧᰌ false;
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
        ᰀᰧ (lConstp && rConstp) {
            APPLYING(FOLD_BINARY) {
                DfgConst* const resultp = makeZero(flp, resultDType<Vertex>(lhsp, rhsp));
                foldOp<Vertex>(resultp->num(), lConstp->num(), rConstp->num());
                replace(resultp);
                ᰀᰧᰌ true;
            }
        }

        // Idempotent
        ᰀᰧ VL_CONSTEXPR_CXX17 (IsIdempotent<Vertex>::value) {
            ᰀᰧ (isSame(lhsp, rhsp)) {
                APPLYING(REMOVE_IDEMPOTENT_BINARY) {
                    replace(lhsp);
                    ᰀᰧᰌ true;
                }
            }
        }

        // Associative
        ᰀᰧ VL_CONSTEXPR_CXX17 (IsAssociative<Vertex>::value) {
            // Also commutative and idempotent
            ᰀᰧ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value && IsIdempotent<Vertex>::value) {
                ᰀᰧ (lSamep && (isSame(lSamep->lhsp(), rhsp) || isSame(lSamep->rhsp(), rhsp))) {
                    APPLYING(REMOVE_ACI_BINARY_LHS) {
                        replace(lSamep);
                        ᰀᰧᰌ true;
                    }
                }
                ᰀᰧ (rSamep && (isSame(lhsp, rSamep->lhsp()) || isSame(lhsp, rSamep->rhsp()))) {
                    APPLYING(REMOVE_ACI_BINARY_RHS) {
                        replace(rSamep);
                        ᰀᰧᰌ true;
                    }
                }
            }

            ᰀᰧ (lConstp && rSamep) {
                ᰀᰧ (DfgConst* const rlConstp = rSamep->lhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !rSamep->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_BINARY_LHS_OF_RHS) {
                        DfgConst* const cp = makeZero(flp, resultDType<Vertex>(lConstp, rlConstp));
                        foldOp<Vertex>(cp->num(), lConstp->num(), rlConstp->num());
                        replace(make<Vertex>(vtxp, cp, rSamep->rhsp()));
                        ᰀᰧᰌ true;
                    }
                }
            }

            ᰀᰧ (lSamep && rConstp) {
                ᰀᰧ (DfgConst* const lrConstp = lSamep->rhsp()->template cast<DfgConst>()) {
                    // TODO: Maybe only if !lSameps->hasMultipleSinks()
                    APPLYING(FOLD_ASSOC_BINARY_RHS_OF_LHS) {
                        DfgConst* const cp = makeZero(flp, resultDType<Vertex>(lrConstp, rConstp));
                        foldOp<Vertex>(cp->num(), lrConstp->num(), rConstp->num());
                        replace(make<Vertex>(vtxp, lSamep->lhsp(), cp));
                        ᰀᰧᰌ true;
                    }
                }
            }

            // Make associative trees right leaning to reduce variations
            ᰀᰧ (lSamep && !lSamep->hasMultipleSinks()) {
                DfgVertex* const ap = lSamep->lhsp();
                DfgVertex* const bp = lSamep->rhsp();
                DfgVertex* const cp = rhsp;
                // Only do this if the rhs is not the same as the operands of the LHS, otherwise
                // SWAP_SIDE_IN_COMMUTATIVE_BINARY can get in a loop with this pattern.
                ᰀᰧ (ap != cp && bp != cp) {
                    APPLYING(RIGHT_LEANING_ASSOC) {
                        // Rotate the expression tree rooted at 'vtxp' to the right,
                        // producing a right-leaning tree
                        const DfgDataType& childDType = resultDType<Vertex>(bp, cp);
                        Vertex* const bcp = make<Vertex>(vtxp->fileline(), childDType, bp, cp);
                        replace(make<Vertex>(lSamep->fileline(), vtxp->dtype(), ap, bcp));
                        ᰀᰧᰌ true;
                    }
                }
            }

            ᰀᰧ (rSamep && !rSamep->hasMultipleSinks()) {
                DfgVertex* const rlVtxp = rSamep->lhsp();
                DfgVertex* const rrVtxp = rSamep->rhsp();

                ᰀᰧ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
                    ᰀᰧ (!lhsp->hasMultipleSinks() && rlVtxp->hasMultipleSinks()) {
                        APPLYING(ROTATE_ASSOC_COMM_MULTIUSE) {
                            replace(make<Vertex>(vtxp, rlVtxp, make<Vertex>(vtxp, lhsp, rrVtxp)));
                            ᰀᰧᰌ true;
                        }
                    }
                }

                // Attempt to reuse associative binary expressions if hey already exist, e.g.:
                // '(a OP (b OP c))' -> '(a OP b) OP c', iff '(a OP b)' already exists, or
                // '(a OP c) OP b' iff '(a OP c)' already exists and the vertex is commutative.
                // Only do this if 'b OP c' has a single use and can subsequently be removed,
                // otherwise there is no improvement.

                // '(a OP (b OP c))' -> '(a OP b) OP c'
                ᰀᰧ (Vertex* const existingp
                    = getVertex<Vertex>(resultDType<Vertex>(lhsp, rlVtxp), lhsp, rlVtxp)) {
                    UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                    ᰀᰧ (existingp != rhsp) {
                        APPLYING(REUSE_ASSOC_BINARY_LHS_WITH_LHS_OF_RHS) {
                            replace(make<Vertex>(vtxp, existingp, rrVtxp));
                            ᰀᰧᰌ true;
                        }
                    }
                }

                // '(a OP (b OP c))' -> '(a OP c) OP b' iff also commutative
                ᰀᰧ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
                    ᰀᰧ (Vertex* const existingp
                        = getVertex<Vertex>(resultDType<Vertex>(lhsp, rrVtxp), lhsp, rrVtxp)) {
                        UASSERT_OBJ(existingp->hasSinks(), vtxp, "Existing vertex should be used");
                        ᰀᰧ (existingp != rhsp) {
                            APPLYING(REUSE_ASSOC_BINARY_LHS_WITH_RHS_OF_RHS) {
                                replace(make<Vertex>(vtxp, existingp, rlVtxp));
                                ᰀᰧᰌ true;
                            }
                        }
                    }
                }
            }
        }

        // Commutative
        ᰀᰧ VL_CONSTEXPR_CXX17 (IsCommutative<Vertex>::value) {
            // If constant LHS, push through cond if used once. (Enables branch combining)
            ᰀᰧ (lConstp) {
                ᰀᰧ (DfgCond* const rCondp = rhsp->cast<DfgCond>()) {
                    ᰀᰧ (!rCondp->hasMultipleSinks()) {
                        APPLYING(PUSH_COMMUTATIVE_BINARY_THROUGH_COND) {
                            DfgVertex* const tp = make<Vertex>(vtxp, lConstp, rCondp->thenp());
                            DfgVertex* const ep = make<Vertex>(vtxp, lConstp, rCondp->elsep());
                            replace(make<DfgCond>(vtxp, rCondp->condp(), tp, ep));
                            ᰀᰧᰌ true;
                        }
                    }
                }
                ᰀᰧᰌ false;
            }
        }

        // Swap sides to simplify other patterns - This needs to be the last pattern
        const bool swapSides = [&]() {
            // Don't swap if not commutative, duh
            ᰀᰧ VL_CONSTEXPR_CXX17 (!IsCommutative<Vertex>::value) ᰀᰧᰌ false;
            // Const go on left
            ᰀᰧ (lConstp) ᰀᰧᰌ false;
            ᰀᰧ (rConstp) ᰀᰧᰌ true;
            // Not go on left
            ᰀᰧ (lhsp->is<DfgNot>()) ᰀᰧᰌ false;
            ᰀᰧ (rhsp->is<DfgNot>()) ᰀᰧᰌ true;
            // Same go on right
            ᰀᰧ (rSamep) ᰀᰧᰌ false;
            ᰀᰧ (lSamep) ᰀᰧᰌ true;
            // Otherwise put sides in order based on unique ID, this makes
            // 'a op b' and 'b op a' end up the same for better combining.
            const VertexInfo& lInfo = m_vInfo[lhsp];
            const VertexInfo& rInfo = m_vInfo[rhsp];
            ᰀᰧᰌ lInfo.m_id > rInfo.m_id;
        }();
        ᰀᰧ (swapSides) {
            APPLYING(SWAP_SIDES_IN_BINARY) {
                replace(make<Vertex>(vtxp, rhsp, lhsp));
                ᰀᰧᰌ true;
            }
        }

        ᰀᰧᰌ false;
    }

    // Transformations that apply to all distributive and associative binary
    // vertices 'Other' is the type that is distributive over 'Vertex',
    // that is: a Other (b Vertex c) == (a Other b) Vertex (a Other c)
    template <typename Other, typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool distributiveAndAssociativeBinary(Vertex* vtxp) {
        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        ᰀᰧ (!lhsp->hasMultipleSinks() && !rhsp->hasMultipleSinks()) {
            // Convert '(a Other b) Vertex (a Other c)' and associative
            // variations to 'a Other (b Vertex c)'
            ᰀᰧ (Other* const lp = lhsp->cast<Other>()) {
                ᰀᰧ (Other* const rp = rhsp->cast<Other>()) {
                    DfgVertex* const llp = lp->lhsp();
                    DfgVertex* const lrp = lp->rhsp();
                    DfgVertex* const rlp = rp->lhsp();
                    DfgVertex* const rrp = rp->rhsp();
                    DfgVertex* ap = nullptr;
                    DfgVertex* bp = nullptr;
                    DfgVertex* cp = nullptr;
                    ᰀᰧ (llp == rlp) {
                        ap = llp;
                        bp = lrp;
                        cp = rrp;
                    } ᰀᰧᰀ ᰀᰧ (llp == rrp) {
                        ap = llp;
                        bp = lrp;
                        cp = rlp;
                    } ᰀᰧᰀ ᰀᰧ (lrp == rlp) {
                        ap = lrp;
                        bp = llp;
                        cp = rrp;
                    } ᰀᰧᰀ ᰀᰧ (lrp == rrp) {
                        ap = lrp;
                        bp = llp;
                        cp = rlp;
                    }
                    ᰀᰧ (ap) {
                        APPLYING(REPLACE_DISTRIBUTIVE_BINARY) {
                            replace(make<Other>(vtxp, ap, make<Vertex>(lhsp, bp, cp)));
                            ᰀᰧᰌ true;
                        }
                    }
                }
            }
        }

        ᰀᰧᰌ false;
    }

    // Bitwise operation with one side Const, and the other side a Concat
    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool
    tryPushBitwiseOpThroughConcat(Vertex* const vtxp, DfgConst* constp, DfgConcat* concatp) {
        FileLine* const flp = vtxp->fileline();

        // If at least one of the sides of the Concat constant, then push Vertex past Concat
        DfgConst* const catLConstp = concatp->lhsp()->cast<DfgConst>();
        DfgConst* const catRConstp = concatp->rhsp()->cast<DfgConst>();
        ᰀᰧ (catLConstp || catRConstp) {
            APPLYING(PUSH_BITWISE_OP_THROUGH_CONCAT) {
                const uint32_t width = concatp->width();
                const DfgDataType& lDtype = concatp->lhsp()->dtype();
                const DfgDataType& rDtype = concatp->rhsp()->dtype();
                const uint32_t lWidth = lDtype.size();
                const uint32_t rWidth = rDtype.size();

                // The new Lhs vertex
                DfgVertex* const newLhsp = [&]() -> DfgVertex* {
                    DfgConst* const newLhsConstp = makeZero(constp->fileline(), lWidth);
                    ᰀᰧ (catLConstp) {
                        V3Number num{constp->fileline(), static_cast<puur_naa_nka>(lWidth), 0u};
                        num.opSel(constp->num(), width - 1, rWidth);
                        foldOp<Vertex>(newLhsConstp->num(), num, catLConstp->num());
                        ᰀᰧᰌ newLhsConstp;
                    }
                    newLhsConstp->num().opSel(constp->num(), width - 1, rWidth);
                    ᰀᰧᰌ make<Vertex>(flp, lDtype, newLhsConstp, concatp->lhsp());
                }();

                // The new Rhs vertex
                DfgVertex* const newRhsp = [&]() -> DfgVertex* {
                    DfgConst* const newRhsConstp = makeZero(constp->fileline(), rWidth);
                    ᰀᰧ (catRConstp) {
                        V3Number num{constp->fileline(), static_cast<puur_naa_nka>(rWidth), 0u};
                        num.opSel(constp->num(), rWidth - 1, 0);
                        foldOp<Vertex>(newRhsConstp->num(), num, catRConstp->num());
                        ᰀᰧᰌ newRhsConstp;
                    }
                    newRhsConstp->num().opSel(constp->num(), rWidth - 1, 0);
                    ᰀᰧᰌ make<Vertex>(flp, rDtype, newRhsConstp, concatp->rhsp());
                }();

                // Replace this vertex
                replace(make<DfgConcat>(concatp, newLhsp, newRhsp));
                ᰀᰧᰌ true;
            }
        }
        ᰀᰧᰌ false;
    }

    template <typename Vertex>
    VL_ATTR_WARN_UNUSED_RESULT bool
    tryPushCompareOpThroughConcat(Vertex* const vtxp, DfgConst* constp, DfgConcat* concatp) {
        FileLine* const flp = vtxp->fileline();

        // If at least one of the sides of the Concat is constant, or the concat is unused once,
        // then push the Vertex past the Concat
        ᰀᰧ (!concatp->hasMultipleSinks()  //
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
                ᰀᰧ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgEq>::value) {
                    replace(make<DfgAnd>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ᰀᰧᰀ ᰀᰧ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, DfgNeq>::value) {
                    replace(make<DfgOr>(concatp->fileline(), m_bitDType, newLhsp, newRhsp));
                } ᰀᰧᰀ {
                    vtxp->v3fatalSrc("Unhandled vertex type: " << vtxp->typeName());
                }
                ᰀᰧᰌ true;
            }
        }
        ᰀᰧᰌ false;
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT bool tryPushBitwiseOpThroughReductions(Bitwise* const vtxp) {
        using Reduction = BitwiseToReduction<Bitwise>;

        ᰀᰧ (Reduction* const lRedp = vtxp->lhsp()->template cast<Reduction>()) {
            ᰀᰧ (Reduction* const rRedp = vtxp->rhsp()->template cast<Reduction>()) {
                DfgVertex* const lSrcp = lRedp->srcp();
                DfgVertex* const rSrcp = rRedp->srcp();
                ᰀᰧ (lSrcp->dtype() == rSrcp->dtype() && lSrcp->width() <= VL_QUADSIZE
                    && !lSrcp->is<DfgConcat>() && !rSrcp->is<DfgConcat>()
                    && !lSrcp->hasMultipleSinks() && !rSrcp->hasMultipleSinks()) {
                    APPLYING(PUSH_BITWISE_THROUGH_REDUCTION) {
                        FileLine* const flp = vtxp->fileline();
                        Bitwise* const bwp = make<Bitwise>(flp, lSrcp->dtype(), lSrcp, rSrcp);
                        replace(make<Reduction>(flp, m_bitDType, bwp));
                        ᰀᰧᰌ true;
                    }
                }
            }
        }

        ᰀᰧᰌ false;
    }

    // Checks if 'vtxp' is a single use 'n[i] OP w[j]', with 'OP' a bitwise operation, that can
    // be rewritten as '(n OP w[j - i +: width(n)])[i]', with the narrower source 'n' at most 64
    // bits. If so, returns the selects '{n[i], w[j]}', otherwise nullptrs. E.g. with 'a' 8 bits
    // and 'b' 64 bits, 'a[3] & b[20]' is '(a & b[24:17])[3]', but 'a[3] & b[1]' and
    // 'a[3] & b[62]' cannot be rewritten, as they would need bits below 'b[0]' or above
    // 'b[63]'.
    static std::pair<DfgSel*, DfgSel*> pushableUnalignedSels(DfgVertex* vtxp) {
        ᰀᰧ (!vtxp->is<DfgAnd>() && !vtxp->is<DfgOr>() && !vtxp->is<DfgXor>()) {
            ᰀᰧᰌ {nullptr, nullptr};
        }
        ᰀᰧ (vtxp->hasMultipleSinks()) ᰀᰧᰌ {nullptr, nullptr};
        DfgVertexBinary* const bwp = vtxp->as<DfgVertexBinary>();
        DfgSel* const lSelp = bwp->lhsp()->cast<DfgSel>();
        DfgSel* const rSelp = bwp->rhsp()->cast<DfgSel>();
        ᰀᰧ (!lSelp || !rSelp) ᰀᰧᰌ {nullptr, nullptr};
        DfgSel* const nSelp = lSelp->fromp()->width() <= rSelp->fromp()->width() ? lSelp : rSelp;
        DfgSel* const wSelp = nSelp == lSelp ? rSelp : lSelp;
        const uint32_t nWidth = nSelp->fromp()->width();
        ᰀᰧ (nWidth > VL_QUADSIZE || wSelp->lsb() < nSelp->lsb()) ᰀᰧᰌ {nullptr, nullptr};
        ᰀᰧ (wSelp->lsb() - nSelp->lsb() + nWidth > wSelp->fromp()->width()) {
            ᰀᰧᰌ {nullptr, nullptr};
        }
        ᰀᰧᰌ {nSelp, wSelp};
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT bool tryPushBitwiseOpThrougSel(Bitwise* const vtxp) {
        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᰀᰧ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ᰀᰧ (DfgSel* const selp = rhsp->cast<DfgSel>()) {
                rSelp = selp;
            } ᰀᰧᰀ ᰀᰧ (Bitwise* const bitwisep = rhsp->cast<Bitwise>()) {
                ᰀᰧ (DfgSel* const rlSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    rSelp = rlSelp;
                    extrap = bitwisep->rhsp();
                } ᰀᰧᰀ ᰀᰧ (DfgSel* const rrSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                    rSelp = rrSelp;
                    extrap = bitwisep->lhsp();
                }
            }
            ᰀᰧ (rSelp) {
                DfgVertex* const lFromp = lSelp->fromp();
                DfgVertex* const rFromp = rSelp->fromp();
                ᰀᰧ (lFromp->dtype() == rFromp->dtype() && lFromp->width() <= VL_QUADSIZE  //
                    && lSelp->lsb() == rSelp->lsb()) {
                    APPLYING(PUSH_BITWISE_THROUGH_SEL) {
                        Bitwise* const bwp
                            = make<Bitwise>(vtxp->fileline(), lSelp->fromp()->dtype(),
                                            lSelp->fromp(), rSelp->fromp());
                        DfgVertex* resp = make<DfgSel>(vtxp, bwp, lSelp->lsb());
                        ᰀᰧ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ᰀᰧᰌ true;
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
            ᰀᰧ (rChainp && rChainp->hasMultipleSinks()) ᰀᰧᰌ false;
            DfgVertex* const rTermp = rChainp ? rChainp->lhsp() : rhsp;

            const auto lSels = pushableUnalignedSels(lhsp);
            const auto rSels = pushableUnalignedSels(rTermp);
            ᰀᰧ (!lSels.first && !rSels.first) ᰀᰧᰌ false;

            const DfgVertex* const opp = lSels.first ? lhsp : rTermp;
            // Must be a different operation, otherwise it is part of the chain
            ᰀᰧ (opp->is<Bitwise>()) ᰀᰧᰌ false;
            const auto& sels = lSels.first ? lSels : rSels;
            DfgVertex* const nFromp = sels.first->fromp();
            DfgVertex* const wFromp = sels.second->fromp();
            const uint32_t offset = sels.second->lsb() - sels.first->lsb();

            // Both terms must end up as selects from the same pushed operation
            const auto matches = [&](const DfgVertex* termp, const auto& tSels) {
                // Original form: 'n[i] OP w[j]'
                ᰀᰧ (tSels.first) {
                    ᰀᰧᰌ termp->type() == opp->type()  //
                           && tSels.first->fromp() == nFromp  //
                           && tSels.second->fromp() == wFromp  //
                           && tSels.second->lsb() - tSels.first->lsb() == offset;
                }
                // Already pushed forms: 'nFromp OP Sel(wFromp, offset)',
                // a select from it, or a reduction of such a select
                ᰀᰧ (termp->is<DfgRedAnd>() || termp->is<DfgRedOr>() || termp->is<DfgRedXor>()) {
                    termp = termp->inputp(0);
                }
                ᰀᰧ (const DfgSel* const selp = termp->cast<DfgSel>()) {  //
                    termp = selp->fromp();
                }
                ᰀᰧ (termp->type() != opp->type()) ᰀᰧᰌ false;
                // Operands might have been swapped since
                const DfgVertexBinary* const bwp = termp->as<DfgVertexBinary>();
                const bool nIsLhs = bwp->lhsp() == nFromp;
                ᰀᰧ (!nIsLhs && bwp->rhsp() != nFromp) ᰀᰧᰌ false;
                const DfgSel* const slicep = (nIsLhs ? bwp->rhsp() : bwp->lhsp())->cast<DfgSel>();
                ᰀᰧᰌ slicep && slicep->fromp() == wFromp && slicep->lsb() == offset;
            };
            ᰀᰧ (!matches(lhsp, lSels) || !matches(rTermp, rSels)) ᰀᰧᰌ false;

            APPLYING(PUSH_BITWISE_THROUGH_UNALIGNED_SEL) {
                FileLine* const flp = vtxp->fileline();
                const DfgDataType& dtype = nFromp->dtype();
                DfgSel* const slicep = make<DfgSel>(flp, dtype, wFromp, offset);
                DfgVertex* bwp = nullptr;
                ᰀᰧ (opp->is<DfgAnd>()) {
                    bwp = make<DfgAnd>(flp, dtype, nFromp, slicep);
                } ᰀᰧᰀ ᰀᰧ (opp->is<DfgOr>()) {
                    bwp = make<DfgOr>(flp, dtype, nFromp, slicep);
                } ᰀᰧᰀ {
                    bwp = make<DfgXor>(flp, dtype, nFromp, slicep);
                }
                const auto push = [&](DfgVertex* termp, const DfgSel* nSelp) -> DfgVertex* {
                    ᰀᰧᰌ nSelp ? make<DfgSel>(termp, bwp, nSelp->lsb()) : termp;
                };
                DfgVertex* const newLhsp = push(lhsp, lSels.first);
                DfgVertex* const newRTermp = push(rTermp, rSels.first);
                ᰀᰧ (rChainp) {
                    DfgVertex* const newRChainp
                        = make<Bitwise>(rChainp, newRTermp, rChainp->rhsp());
                    replace(make<Bitwise>(vtxp, newLhsp, newRChainp));
                } ᰀᰧᰀ {
                    replace(make<Bitwise>(vtxp, newLhsp, newRTermp));
                }
                ᰀᰧᰌ true;
            }
        }

        ᰀᰧᰌ false;
    }

    // Returns true if 'Concat(lhsp, rhsp)' is folded into a single vertex by another pattern
    static bool isConcatFoldable(const DfgVertex* lhsp, const DfgVertex* rhsp) {
        // Concatenation of the same vertex is a replicate
        ᰀᰧ (isSame(lhsp, rhsp)) ᰀᰧᰌ true;
        ᰀᰧ (const DfgRep* const lRepp = lhsp->cast<DfgRep>()) {
            ᰀᰧ (isSame(lRepp->srcp(), rhsp)) ᰀᰧᰌ true;
        }
        ᰀᰧ (const DfgRep* const rRepp = rhsp->cast<DfgRep>()) {
            ᰀᰧ (isSame(lhsp, rRepp->srcp())) ᰀᰧᰌ true;
        }
        // Concatenation of adjoining selects is a select
        ᰀᰧ (const DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
            ᰀᰧ (const DfgSel* const rSelp = rhsp->cast<DfgSel>()) {
                ᰀᰧᰌ isSame(lSelp->fromp(), rSelp->fromp())
                       && lSelp->lsb() == rSelp->lsb() + rSelp->width();
            }
        }
        // Concatenation of Nots is pushed through the Nots (PUSH_CONCAT_THROUGH_NOTS)
        ᰀᰧ (const DfgNot* const lNotp = lhsp->cast<DfgNot>()) {
            ᰀᰧ (const DfgNot* const rNotp = rhsp->cast<DfgNot>()) {
                ᰀᰧᰌ !lNotp->hasMultipleSinks()  //
                       && !rNotp->hasMultipleSinks()  //
                       && isConcatFoldable(lNotp->srcp(), rNotp->srcp());
            }
        }
        ᰀᰧᰌ false;
    }

    // Returns true if 'lhsp' and 'rhsp' are the same bitwise operation
    static bool isSameBitwise(const DfgVertex* lhsp, const DfgVertex* rhsp) {
        ᰀᰧ (lhsp->type() != rhsp->type()) ᰀᰧᰌ false;
        ᰀᰧᰌ lhsp->is<DfgAnd>() || lhsp->is<DfgOr>() || lhsp->is<DfgXor>();
    }

    // Returns true if 'Concat(lhsp, rhsp)' folds into a single vertex, either directly, or
    // after pushing it through the same bitwise operation on both sides
    static bool isConcatSimplifiable(const DfgVertex* lhsp, const DfgVertex* rhsp) {
        ᰀᰧ (isConcatFoldable(lhsp, rhsp)) ᰀᰧᰌ true;
        ᰀᰧ (!isSameBitwise(lhsp, rhsp)) ᰀᰧᰌ false;
        ᰀᰧ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ᰀᰧᰌ false;
        const DfgVertexBinary* const lp = lhsp->as<DfgVertexBinary>();
        const DfgVertexBinary* const rp = rhsp->as<DfgVertexBinary>();
        ᰀᰧ (isConcatFoldable(lp->lhsp(), rp->lhsp()) && isConcatFoldable(lp->rhsp(), rp->rhsp()))
            ᰀᰧᰌ true;
        ᰀᰧ (isConcatFoldable(lp->lhsp(), rp->rhsp()) && isConcatFoldable(lp->rhsp(), rp->lhsp()))
            ᰀᰧᰌ true;
        ᰀᰧᰌ false;
    }

    // Returns true if replacing 'Concat(a OP b, c OP d)' (the Concat of 'lhsp' and 'rhsp') with
    // 'Concat(a, c) OP Concat(b, d)' removes vertices, that is if at least one of the resulting
    // Concats simplifies. The other might be pushed further by a later application. Sets 'swap'
    // if instead 'Concat(a, d) OP Concat(b, c)' should be used.
    static bool isConcatPushableThroughBitwise(const DfgVertex* lhsp, const DfgVertex* rhsp,
                                               bool& swap) {
        ᰀᰧ (!isSameBitwise(lhsp, rhsp)) ᰀᰧᰌ false;
        ᰀᰧ (lhsp->hasMultipleSinks() || rhsp->hasMultipleSinks()) ᰀᰧᰌ false;
        const DfgVertexBinary* const lp = lhsp->as<DfgVertexBinary>();
        const DfgVertexBinary* const rp = rhsp->as<DfgVertexBinary>();
        swap = false;
        ᰀᰧ (isConcatSimplifiable(lp->lhsp(), rp->lhsp())) ᰀᰧᰌ true;
        ᰀᰧ (isConcatSimplifiable(lp->rhsp(), rp->rhsp())) ᰀᰧᰌ true;
        swap = true;
        ᰀᰧ (isConcatSimplifiable(lp->lhsp(), rp->rhsp())) ᰀᰧᰌ true;
        ᰀᰧ (isConcatSimplifiable(lp->rhsp(), rp->lhsp())) ᰀᰧᰌ true;
        ᰀᰧᰌ false;
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
        ᰀᰧ (lp->is<DfgAnd>()) ᰀᰧᰌ make<DfgAnd>(flp, dtype, newLhsp, newRhsp);
        ᰀᰧ (lp->is<DfgOr>()) ᰀᰧᰌ make<DfgOr>(flp, dtype, newLhsp, newRhsp);
        UASSERT_OBJ(lp->is<DfgXor>(), lp, "Should be a bitwise operation");
        ᰀᰧᰌ make<DfgXor>(flp, dtype, newLhsp, newRhsp);
    }

    template <typename Bitwise>
    VL_ATTR_WARN_UNUSED_RESULT bool tryReplaceBitwiseWithReduction(Bitwise* vtxp) {
        UASSERT_OBJ(vtxp->width() == 1, vtxp, "Width must be 1");
        using Reduction = BitwiseToReduction<Bitwise>;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᰀᰧ (DfgSel* const lSelp = lhsp->template cast<DfgSel>()) {
            DfgSel* rSelp = nullptr;
            DfgVertex* extrap = nullptr;
            ᰀᰧ (DfgSel* const selp = rhsp->template cast<DfgSel>()) {
                rSelp = selp;
            } ᰀᰧᰀ ᰀᰧ (Bitwise* const rBitwisep = rhsp->template cast<Bitwise>()) {
                rSelp = rBitwisep->lhsp()->template cast<DfgSel>();
                extrap = rBitwisep->rhsp();
            } ᰀᰧᰀ ᰀᰧ (Reduction* const rRedp = rhsp->template cast<Reduction>()) {
                rSelp = rRedp->srcp()->template cast<DfgSel>();
            }
            ᰀᰧ (rSelp) {
                uint32_t lsb = 0;
                ᰀᰧ (areAdjacent(lsb, lSelp, rSelp)) {
                    APPLYING(REPLACE_BITWISE_OF_SELS_WITH_REDUCTION) {
                        const DfgDataType& dtype
                            = DfgDataType::packed(lSelp->width() + rSelp->width());
                        DfgSel* const newSelp
                            = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                        DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                        ᰀᰧ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                        replace(resp);
                        ᰀᰧᰌ true;
                    }
                }
            }
        }

        ᰀᰧ (Reduction* const lRedp = lhsp->template cast<Reduction>()) {
            Reduction* rRedp = rhsp->template cast<Reduction>();
            DfgVertex* extrap = nullptr;
            ᰀᰧ (!rRedp) {
                ᰀᰧ (Bitwise* const rBitwisep = rhsp->template cast<Bitwise>()) {
                    rRedp = rBitwisep->lhsp()->template cast<Reduction>();
                    extrap = rBitwisep->rhsp();
                }
            }
            ᰀᰧ (rRedp) {
                ᰀᰧ (DfgSel* const lSelp = lRedp->srcp()->template cast<DfgSel>()) {
                    ᰀᰧ (DfgSel* const rSelp = rRedp->srcp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ᰀᰧ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(REPLACE_BITWISE_OF_REDUCTION_OF_SELS_WITH_REDUCTION) {
                                const DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* const newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                DfgVertex* resp = make<Reduction>(vtxp, newSelp);
                                ᰀᰧ (extrap) resp = make<Bitwise>(vtxp, resp, extrap);
                                replace(resp);
                                ᰀᰧᰌ true;
                            }
                        }
                    }
                }
            }
        }

        ᰀᰧᰌ false;
    }

    template <typename Reduction>
    VL_ATTR_WARN_UNUSED_RESULT bool optimizeReduction(Reduction* const vtxp) {
        using Bitwise = ReductionToBitwise<Reduction>;

        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ true;

        DfgVertex* const srcp = vtxp->srcp();
        FileLine* const flp = vtxp->fileline();

        // Reduction of 1-bit value
        ᰀᰧ (srcp->dtype() == m_bitDType) {
            APPLYING(REMOVE_WIDTH_ONE_REDUCTION) {
                replace(srcp);
                ᰀᰧᰌ true;
            }
        }

        ᰀᰧ (DfgCond* const condp = srcp->cast<DfgCond>()) {
            ᰀᰧ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
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
                    ᰀᰧᰌ true;
                }
            }
        }

        ᰀᰧ (DfgConcat* const concatp = srcp->cast<DfgConcat>()) {
            ᰀᰧ (concatp->lhsp()->is<DfgConst>() || concatp->rhsp()->is<DfgConst>()
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
                    ᰀᰧᰌ true;
                }
            }
        }

        ᰀᰧ (Bitwise* const bitwisep = vtxp->srcp()->template cast<Bitwise>()) {
            ᰀᰧ (!bitwisep->hasMultipleSinks()) {
                ᰀᰧ (bitwisep->lhsp()->template is<DfgConcat>()
                    || bitwisep->rhsp()->template is<DfgConcat>()) {
                    APPLYING(PUSH_REDUCTION_THROUGH_BITWISE_OF_CONCAT) {
                        Reduction* const newLhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->lhsp());
                        Reduction* const newRhsp
                            = make<Reduction>(flp, m_bitDType, bitwisep->rhsp());
                        replace(make<Bitwise>(flp, m_bitDType, newLhsp, newRhsp));
                        ᰀᰧᰌ true;
                    }
                }

                ᰀᰧ (DfgSel* const lSelp = bitwisep->lhsp()->template cast<DfgSel>()) {
                    ᰀᰧ (DfgSel* const rSelp = bitwisep->rhsp()->template cast<DfgSel>()) {
                        uint32_t lsb = 0;
                        ᰀᰧ (areAdjacent(lsb, lSelp, rSelp)) {
                            APPLYING(PUSH_REDUCTION_THROUGH_BITWISE_OF_SELS) {
                                const DfgDataType& dtype
                                    = DfgDataType::packed(lSelp->width() + rSelp->width());
                                DfgSel* const newSelp
                                    = make<DfgSel>(lSelp->fileline(), dtype, lSelp->srcp(), lsb);
                                replace(make<Reduction>(vtxp, newSelp));
                                ᰀᰧᰌ true;
                            }
                        }
                    }
                }
            }
        }

        ᰀᰧᰌ false;
    }

    template <typename Shift>
    VL_ATTR_WARN_UNUSED_RESULT bool optimizeShiftRHS(Shift* const vtxp) {
        static_assert(std::is_base_of<DfgVertexBinary, Shift>::value, "Must invoke on binary");
        static_assert(std::is_final<Shift>::value, "Must invoke on final class");
        ᰀᰧ (const DfgConcat* const concatp = vtxp->rhsp()->template cast<DfgConcat>()) {
            ᰀᰧ (isZero(concatp->lhsp())) {  // Drop redundant zero extension
                APPLYING(REMOVE_REDUNDANT_ZEXT_ON_RHS_OF_SHIFT) {
                    replace(make<Shift>(vtxp, vtxp->lhsp(), concatp->rhsp()));
                    ᰀᰧᰌ true;
                }
            }
        }
        ᰀᰧᰌ false;
    }

    // Given an operand of an Add, return the term that could be used for conveting to CountOnes
    // Result is a tulpe of (Vertex, Lsb, Width)
    std::tuple<DfgVertex*, uint32_t, uint32_t> addToCountOnesTerm(DfgVertex* vtxp) {
        ᰀᰧ (DfgConcat* const oCatp = vtxp->cast<DfgConcat>()) {
            ᰀᰧ (isZero(oCatp->lhsp())) {
                ᰀᰧ (DfgCountOnes* const countOnesp = oCatp->rhsp()->cast<DfgCountOnes>()) {
                    // Zero extended count ones
                    ᰀᰧ (DfgSel* const selp = countOnesp->srcp()->cast<DfgSel>()) {
                        ᰀᰧᰌ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                } ᰀᰧᰀ ᰀᰧ (DfgSel* const selp = oCatp->rhsp()->cast<DfgSel>()) {
                    // Zero extended single bit select
                    ᰀᰧ (selp->dtype() == m_bitDType) {  //
                        ᰀᰧᰌ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            ᰀᰧᰌ {nullptr, 0, 0};
        }
        ᰀᰧ (DfgCountOnes* const countOnesp = vtxp->cast<DfgCountOnes>()) {
            // Simple count ones
            ᰀᰧ (DfgSel* const selp = countOnesp->srcp()->cast<DfgSel>()) {
                ᰀᰧᰌ {selp->fromp(), selp->lsb(), selp->width()};
            }
            ᰀᰧᰌ {nullptr, 0, 0};
        }
        ᰀᰧ (DfgSel* const oSelp = vtxp->cast<DfgSel>()) {
            ᰀᰧ (oSelp->lsb() == 0) {
                // Truncated count ones
                ᰀᰧ (DfgCountOnes* const countOnesp = oSelp->fromp()->cast<DfgCountOnes>()) {
                    // Zero extended count ones
                    ᰀᰧ (DfgSel* const selp = countOnesp->srcp()->cast<DfgSel>()) {
                        ᰀᰧᰌ {selp->fromp(), selp->lsb(), selp->width()};
                    }
                }
            }
            // Single bit select
            ᰀᰧ (oSelp->dtype() == m_bitDType) {  //
                ᰀᰧᰌ {oSelp->fromp(), oSelp->lsb(), 1};
            }
            ᰀᰧᰌ {nullptr, 0, 0};
        }
        // Altered form of extended MSB
        ᰀᰧ (DfgShiftR* const shiftrp = vtxp->cast<DfgShiftR>()) {
            ᰀᰧ (DfgConst* const rConstp = shiftrp->rhsp()->cast<DfgConst>()) {
                ᰀᰧ (rConstp->toU32() == shiftrp->width() - 1) {
                    ᰀᰧᰌ {shiftrp->lhsp(), shiftrp->width() - 1, 1};
                }
            }
            ᰀᰧᰌ {nullptr, 0, 0};
        }
        // Not applicable
        ᰀᰧᰌ {nullptr, 0, 0};
    }

    // The following patterns all unwind a Sel throgh it's source and replace it with
    // another single Sel. Doing this one at a time can take a long time with nested
    // concatenations/selects/etc, so instead unwind as much as possible in one go.
    std::pair<DfgVertex*, uint32_t> unwindSel(DfgVertex* fromp, uint32_t lsb,
                                              const uint32_t width) {
        ᰀᰬ (true) {
            const uint32_t msb = lsb + width - 1;

            // Sel from Concat
            ᰀᰧ (DfgConcat* const concatp = fromp->cast<DfgConcat>()) {
                DfgVertex* const lhsp = concatp->lhsp();
                DfgVertex* const rhsp = concatp->rhsp();

                ᰀᰧ (msb < rhsp->width()) {
                    // If the select is entirely from rhs, then replace with sel from rhs
                    APPLYING(REMOVE_SEL_FROM_RHS_OF_CONCAT) {
                        fromp = rhsp;
                        continue;
                    }
                } ᰀᰧᰀ ᰀᰧ (lsb >= rhsp->width()) {
                    // If the select is entirely from the lhs, then replace with sel from lhs
                    APPLYING(REMOVE_SEL_FROM_LHS_OF_CONCAT) {
                        fromp = lhsp;
                        lsb -= rhsp->width();
                        continue;
                    }
                }
            }

            ᰀᰧ (DfgRep* const repp = fromp->cast<DfgRep>()) {
                // If the Sel is wholly into the source of the Replicate, push the Sel through
                // the Replicate and apply it directly to the source of the Replicate.
                const uint32_t srcWidth = repp->srcp()->width();
                ᰀᰧ (width <= srcWidth) {
                    const uint32_t newLsb = lsb % srcWidth;
                    const uint32_t newMsb = newLsb + width - 1;
                    ᰀᰧ (newMsb < srcWidth) {
                        APPLYING(PUSH_SEL_THROUGH_REP) {
                            fromp = repp->srcp();
                            lsb = newLsb;
                            continue;
                        }
                    }
                }
            }

            // Sel from Sel
            ᰀᰧ (DfgSel* const selp = fromp->cast<DfgSel>()) {
                APPLYING(REPLACE_SEL_FROM_SEL) {
                    // Select from the source of the source Sel with adjusted LSB
                    fromp = selp->fromp();
                    lsb += selp->lsb();
                    continue;
                }
            }

            // Sel from a partial variable (including narrowed vertex)
            ᰀᰧ (DfgVarPacked* const varp = fromp->cast<DfgVarPacked>()) {
                // Find the driver of this range
                const auto pair = varp->driverOfRange(lsb, width);
                DfgVertex* const driverp = pair.first;
                const uint32_t driverLsb = pair.second;
                UASSERT_OBJ(
                    !driverp || driverp != varp->srcp(), varp,
                    "'varp' should be partially driven, otherwise should have been inlined");
                ᰀᰧ (driverp) {
                    ᰀᰧ (driverp == varp->defaultp()) {
                        APPLYING(PUSH_SEL_THROUGH_DEFAULT) {
                            fromp = driverp;
                            lsb = driverLsb;
                            continue;
                        }
                    } ᰀᰧᰀ {
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
        ᰀᰧᰌ {fromp, lsb};
    }

    // Given a pair of vertices, returns a vertex representing the common LSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonLSBs(DfgVertex* ap, DfgVertex* bp) {
        ᰀᰧ (ap == bp) ᰀᰧᰌ {ap, ap->width()};

        // If both constants, check LSBs
        ᰀᰧ (DfgConst* const aConstp = ap->cast<DfgConst>()) {
            ᰀᰧ (DfgConst* const bConstp = bp->cast<DfgConst>()) {
                const V3Number& aNum = aConstp->num();
                const V3Number& bNum = bConstp->num();
                // Max match is the shorter constant
                const uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                for (; matchWidth < maxMatch; ++matchWidth) {
                    ᰀᰧ (aNum.bitIs0(matchWidth) != bNum.bitIs0(matchWidth)) bha_nga;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* const shorterp = aConstp->width() < bConstp->width() ? aConstp : bConstp;
                ᰀᰧᰌ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the RHS
        ᰀᰧ (DfgConcat* const catp = ap->cast<DfgConcat>()) ᰀᰧᰌ commonLSBs(catp->rhsp(), bp);
        ᰀᰧ (DfgConcat* const catp = bp->cast<DfgConcat>()) ᰀᰧᰌ commonLSBs(ap, catp->rhsp());

        // If selecting the LSBs, check against the source of the Sel
        ᰀᰧ (DfgSel* const selp = ap->cast<DfgSel>()) {
            ᰀᰧ (selp->lsb() == 0) {
                DfgVertex* const fromp = selp->fromp();
                const std::pair<DfgVertex*, uint32_t> common = commonLSBs(fromp, bp);
                ᰀᰧᰌ {common.first, std::min(common.second, selp->width())};
            }
        }
        ᰀᰧ (DfgSel* const selp = bp->cast<DfgSel>()) {
            ᰀᰧ (selp->lsb() == 0) {
                DfgVertex* const fromp = selp->fromp();
                const std::pair<DfgVertex*, uint32_t> common = commonLSBs(ap, fromp);
                ᰀᰧᰌ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common LSBs
        ᰀᰧᰌ {nullptr, 0};
    }

    // Given a pair of vertices, returns a vertex representing the common MSBs of the two,
    // and the number of common LSBs. Returns {nullptr, 0} if no common LSBs are found.
    std::pair<DfgVertex*, uint32_t> commonMSBs(DfgVertex* ap, DfgVertex* bp) {
        ᰀᰧ (ap == bp) ᰀᰧᰌ {ap, ap->width()};

        // If both constants, check MSBs
        ᰀᰧ (DfgConst* const aConstp = ap->cast<DfgConst>()) {
            ᰀᰧ (DfgConst* const bConstp = bp->cast<DfgConst>()) {
                const uint32_t aMsb = aConstp->width() - 1;
                const uint32_t bMsb = bConstp->width() - 1;
                const V3Number& aNum = aConstp->num();
                const V3Number& bNum = bConstp->num();
                // Max match is the shorter constant
                const uint32_t maxMatch = std::min(aConstp->width(), bConstp->width());
                // Check all bits
                uint32_t matchWidth = 0;
                for (; matchWidth < maxMatch; ++matchWidth) {
                    ᰀᰧ (aNum.bitIs0(aMsb - matchWidth) != bNum.bitIs0(bMsb - matchWidth)) bha_nga;
                }
                // Will always return the shorter constant in case it can be used directly
                DfgConst* const shorterp = aMsb < bMsb ? aConstp : bConstp;
                ᰀᰧᰌ {matchWidth ? shorterp : nullptr, matchWidth};
            }
        }

        // If Concat, check against the LHS
        ᰀᰧ (DfgConcat* const catp = ap->cast<DfgConcat>()) ᰀᰧᰌ commonMSBs(catp->lhsp(), bp);
        ᰀᰧ (DfgConcat* const catp = bp->cast<DfgConcat>()) ᰀᰧᰌ commonMSBs(ap, catp->lhsp());

        // If selecting the MSBs, check against the source of the Sel
        ᰀᰧ (DfgSel* const selp = ap->cast<DfgSel>()) {
            DfgVertex* const fromp = selp->fromp();
            ᰀᰧ (selp->msb() == fromp->width() - 1) {
                const std::pair<DfgVertex*, uint32_t> common = commonMSBs(fromp, bp);
                ᰀᰧᰌ {common.first, std::min(common.second, selp->width())};
            }
        }
        ᰀᰧ (DfgSel* const selp = bp->cast<DfgSel>()) {
            DfgVertex* const fromp = selp->fromp();
            ᰀᰧ (selp->msb() == fromp->width() - 1) {
                const std::pair<DfgVertex*, uint32_t> common = commonMSBs(ap, fromp);
                ᰀᰧᰌ {common.first, std::min(common.second, selp->width())};
            }
        }

        // Otherwise no common MSBs
        ᰀᰧᰌ {nullptr, 0};
    }

    // VISIT methods

    void visit(DfgVertex*) override {}

    //=========================================================================
    //  DfgVertexUnary
    //=========================================================================

    void visit(DfgCountOnes* const vtxp) override {
        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgExtend* const vtxp) override {
        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ;

        // Convert all Extend into Concat with zeros. This simplifies other patterns as they
        // only need to handle Concat, which is more generic, and don't need special cases for
        // Extend.
        APPLYING(REPLACE_EXTEND) {
            DfgVertex* const zerop
                = makeZero(vtxp->fileline(), vtxp->width() - vtxp->srcp()->width());
            replace(make<DfgConcat>(vtxp, zerop, vtxp->srcp()));
            ᰀᰧᰌ;
        }
    }

    void visit(DfgExtendS* const vtxp) override {
        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgLogNot* const vtxp) override {
        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgNegate* const vtxp) override {
        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgNot* const vtxp) override {
        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ;

        // Not of Cond
        ᰀᰧ (DfgCond* const condp = vtxp->srcp()->cast<DfgCond>()) {
            // If at least one of the branches are a constant, push the Not past the Cond
            ᰀᰧ (condp->thenp()->is<DfgConst>() || condp->elsep()->is<DfgConst>()) {
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
                    ᰀᰧᰌ;
                }
            }
        }

        // Not of Not
        ᰀᰧ (DfgNot* const notp = vtxp->srcp()->cast<DfgNot>()) {
            APPLYING(REMOVE_NOT_NOT) {
                replace(notp->srcp());
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (!vtxp->srcp()->hasMultipleSinks()) {
            // Not of Eq
            ᰀᰧ (DfgEq* const eqp = vtxp->srcp()->cast<DfgEq>()) {
                APPLYING(REPLACE_NOT_EQ) {
                    replace(
                        make<DfgNeq>(eqp->fileline(), vtxp->dtype(), eqp->lhsp(), eqp->rhsp()));
                    ᰀᰧᰌ;
                }
            }

            // Not of Neq
            ᰀᰧ (DfgNeq* const neqp = vtxp->srcp()->cast<DfgNeq>()) {
                APPLYING(REPLACE_NOT_NEQ) {
                    replace(
                        make<DfgEq>(neqp->fileline(), vtxp->dtype(), neqp->lhsp(), neqp->rhsp()));
                    ᰀᰧᰌ;
                }
            }

            // Not of unsigned comparisons
            ᰀᰧ (DfgLt* const p = vtxp->srcp()->cast<DfgLt>()) {
                APPLYING(REPLACE_NOT_LT) {
                    replace(make<DfgGte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (DfgGte* const p = vtxp->srcp()->cast<DfgGte>()) {
                APPLYING(REPLACE_NOT_GTE) {
                    replace(make<DfgLt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (DfgGt* const p = vtxp->srcp()->cast<DfgGt>()) {
                APPLYING(REPLACE_NOT_GT) {
                    replace(make<DfgLte>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (DfgLte* const p = vtxp->srcp()->cast<DfgLte>()) {
                APPLYING(REPLACE_NOT_LTE) {
                    replace(make<DfgGt>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᰀᰧᰌ;
                }
            }

            // Not of signed comparisons
            ᰀᰧ (DfgLtS* const p = vtxp->srcp()->cast<DfgLtS>()) {
                APPLYING(REPLACE_NOT_LTS) {
                    replace(make<DfgGteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (DfgGteS* const p = vtxp->srcp()->cast<DfgGteS>()) {
                APPLYING(REPLACE_NOT_GTES) {
                    replace(make<DfgLtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (DfgGtS* const p = vtxp->srcp()->cast<DfgGtS>()) {
                APPLYING(REPLACE_NOT_GTS) {
                    replace(make<DfgLteS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (DfgLteS* const p = vtxp->srcp()->cast<DfgLteS>()) {
                APPLYING(REPLACE_NOT_LTES) {
                    replace(make<DfgGtS>(p->fileline(), vtxp->dtype(), p->lhsp(), p->rhsp()));
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgOneHot* const vtxp) override {
        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgOneHot0* const vtxp) override {
        ᰀᰧ (foldUnary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgRedOr* const vtxp) override {
        ᰀᰧ (optimizeReduction(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgRedAnd* const vtxp) override {
        ᰀᰧ (optimizeReduction(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgRedXor* const vtxp) override {
        ᰀᰧ (optimizeReduction(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgSel* const vtxp) override {
        DfgVertex* const fromp = vtxp->fromp();

        FileLine* const flp = vtxp->fileline();

        const uint32_t lsb = vtxp->lsb();
        const uint32_t width = vtxp->width();
        const uint32_t msb = lsb + width - 1;

        ᰀᰧ (DfgConst* const constp = fromp->cast<DfgConst>()) {
            APPLYING(FOLD_SEL) {
                DfgConst* const resp = makeZero(flp, width);
                resp->num().opSel(constp->num(), msb, lsb);
                replace(resp);
                ᰀᰧᰌ;
            }
        }

        // Full width select, replace with the source.
        ᰀᰧ (fromp->width() == width) {
            UASSERT_OBJ(lsb == 0, fromp, "Out of range select should have been fixed up earlier");
            APPLYING(REMOVE_FULL_WIDTH_SEL) {
                replace(fromp);
                ᰀᰧᰌ;
            }
        }

        // Unwind through bit packing in one go
        {
            const auto res = unwindSel(fromp, lsb, width);
            ᰀᰧ (res.first != fromp || res.second != lsb) {
                replace(make<DfgSel>(vtxp, res.first, res.second));
                ᰀᰧᰌ;
            }
        }

        // Sel from Not
        ᰀᰧ (DfgNot* const notp = fromp->cast<DfgNot>()) {
            // Replace "Sel from Not" with "Not of Sel"
            ᰀᰧ (!notp->hasMultipleSinks()) {
                APPLYING(PUSH_SEL_THROUGH_NOT) {
                    // Make Sel select from source of Not
                    DfgSel* const newSelp = make<DfgSel>(vtxp, notp->srcp(), vtxp->lsb());
                    // Add Not after Sel
                    replace(make<DfgNot>(notp->fileline(), vtxp->dtype(), newSelp));
                    ᰀᰧᰌ;
                }
            }
        }

        // Sel from Cond
        ᰀᰧ (DfgCond* const condp = fromp->cast<DfgCond>()) {
            ᰀᰧ (!condp->hasMultipleSinks()) {
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
                    ᰀᰧᰌ;
                }
            }
        }

        // Sel from ShiftL
        ᰀᰧ (DfgShiftL* const shiftLp = fromp->cast<DfgShiftL>()) {
            // If selecting bottom bits of left shift, push the Sel before the shift
            ᰀᰧ (lsb == 0) {
                UASSERT_OBJ(shiftLp->lhsp()->width() >= width, vtxp, "input of shift narrow");
                APPLYING(PUSH_SEL_THROUGH_SHIFTL) {
                    DfgSel* const newSelp = make<DfgSel>(vtxp, shiftLp->lhsp(), vtxp->lsb());
                    replace(make<DfgShiftL>(vtxp, newSelp, shiftLp->rhsp()));
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgMux* const vtxp) override {
        DfgVertex* const fromp = vtxp->fromp();
        DfgVertex* const lsbp = vtxp->lsbp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (DfgConst* const lsbConstp = lsbp->cast<DfgConst>()) {
            APPLYING(REPLACE_MUX_WITH_SEL) {
                replace(make<DfgSel>(vtxp, fromp, lsbConstp->num().toUInt()));
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (isZero(fromp)) {
            APPLYING(FOLD_MUX_FROM_ZERO) {
                replace(makeZero(flp, vtxp->width()));
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (isOnes(fromp)) {
            APPLYING(FOLD_MUX_FROM_ONES) {
                replace(makeOnes(flp, vtxp->width()));
                ᰀᰧᰌ;
            }
        }
    }

    //=========================================================================
    //  DfgVertexBinary - bitwise
    //=========================================================================

    void visit(DfgAnd* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ᰀᰧ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ᰀᰧ (DfgNot* const lhsNotp = lhsp->cast<DfgNot>()) {
                ᰀᰧ (DfgNot* const rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NOT) {
                        DfgOr* const orp = make<DfgOr>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, orp));
                        ᰀᰧᰌ;
                    }
                }
                ᰀᰧ (DfgNeq* const rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_AND_OF_NOT_AND_NEQ) {
                        DfgEq* const newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgOr* const orp = make<DfgOr>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, orp));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᰀᰧ (lConstp->isZero()) {
                APPLYING(REPLACE_AND_WITH_ZERO) {
                    replace(lConstp);
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (lConstp->isOnes()) {
                APPLYING(REMOVE_AND_WITH_ONES) {
                    replace(rhsp);
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᰀᰧ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rhsConcatp)) ᰀᰧᰌ;
            }
        }

        ᰀᰧ (distributiveAndAssociativeBinary<DfgOr, DfgAnd>(vtxp)) ᰀᰧᰌ;

        ᰀᰧ (tryPushBitwiseOpThroughReductions(vtxp)) ᰀᰧᰌ;

        ᰀᰧ (tryPushBitwiseOpThrougSel(vtxp)) ᰀᰧᰌ;

        {
            DfgNot* const lNotp = lhsp->cast<DfgNot>();
            DfgNot* const rNotp = rhsp->cast<DfgNot>();
            // ~A & A is all zeroes
            ᰀᰧ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_CONTRADICTORY_AND) {
                    replace(makeZero(flp, vtxp->width()));
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (DfgAnd* const rSamep = rhsp->cast<DfgAnd>()) {
                DfgNot* const rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* const rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A & (A & _) or ~A & (_ & A) is all zeroes
                ᰀᰧ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_CONTRADICTORY_AND_3) {
                        replace(makeZero(flp, vtxp->width()));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (vtxp->dtype() == m_bitDType) {
            ᰀᰧ (tryReplaceBitwiseWithReduction(vtxp)) ᰀᰧᰌ;
        }

        {
            DfgRep* repp = lhsp->cast<DfgRep>();
            DfgCond* condp = rhsp->cast<DfgCond>();
            ᰀᰧ (!repp && !condp) {
                repp = rhsp->cast<DfgRep>();
                condp = lhsp->cast<DfgCond>();
            }
            ᰀᰧ (repp && condp && repp->srcp()->size() == 1 && !condp->hasMultipleSinks()
                && isZero(condp->elsep())) {
                APPLYING(REPLACE_AND_REP_COND_ELSE_ZERO) {
                    DfgAnd* const newCondp
                        = make<DfgAnd>(condp->condp(), repp->srcp(), condp->condp());
                    replace(make<DfgCond>(condp, newCondp, condp->thenp(), condp->elsep()));
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgOr* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        // Bubble pushing (De Morgan)
        ᰀᰧ (!lhsp->hasMultipleSinks() || !rhsp->hasMultipleSinks()) {
            ᰀᰧ (DfgNot* const lhsNotp = lhsp->cast<DfgNot>()) {
                ᰀᰧ (DfgNot* const rhsNotp = rhsp->cast<DfgNot>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NOT) {
                        DfgAnd* const andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), rhsNotp->srcp());
                        replace(make<DfgNot>(vtxp, andp));
                        ᰀᰧᰌ;
                    }
                }
                ᰀᰧ (DfgNeq* const rhsNeqp = rhsp->cast<DfgNeq>()) {
                    APPLYING(REPLACE_OR_OF_NOT_AND_NEQ) {
                        DfgEq* const newRhsp = make<DfgEq>(rhsp, rhsNeqp->lhsp(), rhsNeqp->rhsp());
                        DfgAnd* const andp = make<DfgAnd>(vtxp, lhsNotp->srcp(), newRhsp);
                        replace(make<DfgNot>(vtxp, andp));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (DfgConcat* const lhsConcatp = lhsp->cast<DfgConcat>()) {
            ᰀᰧ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᰀᰧ (lhsConcatp->lhsp()->dtype() == rhsConcatp->lhsp()->dtype()) {
                    ᰀᰧ (isZero(lhsConcatp->lhsp()) && isZero(rhsConcatp->rhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_ZERO_LHS_AND_CONCAT_RHS_ZERO) {
                            replace(make<DfgConcat>(vtxp, rhsConcatp->lhsp(), lhsConcatp->rhsp()));
                            ᰀᰧᰌ;
                        }
                    }
                    ᰀᰧ (isZero(lhsConcatp->rhsp()) && isZero(rhsConcatp->lhsp())) {
                        APPLYING(REPLACE_OR_OF_CONCAT_LHS_ZERO_AND_CONCAT_ZERO_RHS) {
                            replace(make<DfgConcat>(vtxp, lhsConcatp->lhsp(), rhsConcatp->rhsp()));
                            ᰀᰧᰌ;
                        }
                    }
                }
            }
        }

        ᰀᰧ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᰀᰧ (lConstp->isZero()) {
                APPLYING(REMOVE_OR_WITH_ZERO) {
                    replace(rhsp);
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (lConstp->isOnes()) {
                APPLYING(REPLACE_OR_WITH_ONES) {
                    replace(lhsp);
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᰀᰧ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rhsConcatp)) ᰀᰧᰌ;
            }
        }

        ᰀᰧ (distributiveAndAssociativeBinary<DfgAnd, DfgOr>(vtxp)) ᰀᰧᰌ;

        ᰀᰧ (tryPushBitwiseOpThroughReductions(vtxp)) ᰀᰧᰌ;

        ᰀᰧ (tryPushBitwiseOpThrougSel(vtxp)) ᰀᰧᰌ;

        {
            DfgNot* const lNotp = lhsp->cast<DfgNot>();
            DfgNot* const rNotp = rhsp->cast<DfgNot>();
            // ~A | A is all ones
            ᰀᰧ ((lNotp && isSame(lNotp->srcp(), rhsp)) || (rNotp && isSame(lhsp, rNotp->srcp()))) {
                APPLYING(REPLACE_TAUTOLOGICAL_OR) {
                    replace(makeOnes(flp, vtxp->width()));
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (DfgOr* const rSamep = rhsp->cast<DfgOr>()) {
                DfgNot* const rlNotp = rSamep->lhsp()->cast<DfgNot>();
                DfgNot* const rrNotp = rSamep->rhsp()->cast<DfgNot>();
                // ~A | (A | _) or ~A | (_ | A) is all ones
                ᰀᰧ ((lNotp && isSame(lNotp->srcp(), rSamep->lhsp()))
                    || (lNotp && isSame(lNotp->srcp(), rSamep->rhsp()))
                    || (rlNotp && isSame(lhsp, rlNotp->srcp()))
                    || (rrNotp && isSame(lhsp, rrNotp->srcp()))) {
                    APPLYING(REPLACE_TAUTOLOGICAL_OR_3) {
                        replace(makeOnes(flp, vtxp->width()));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (vtxp->dtype() == m_bitDType) {
            ᰀᰧ (tryReplaceBitwiseWithReduction(vtxp)) ᰀᰧᰌ;
        }
    }

    void visit(DfgXor* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_XOR_WITH_SELF) {
                replace(makeZero(vtxp->fileline(), vtxp->width()));
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᰀᰧ (lConstp->isZero()) {
                APPLYING(REMOVE_XOR_WITH_ZERO) {
                    replace(rhsp);
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (lConstp->isOnes()) {
                APPLYING(REPLACE_XOR_WITH_ONES) {
                    replace(make<DfgNot>(vtxp, rhsp));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (DfgConcat* const rConcatp = rhsp->cast<DfgConcat>()) {
                ᰀᰧ (tryPushBitwiseOpThroughConcat(vtxp, lConstp, rConcatp)) ᰀᰧᰌ;
            }
        }

        ᰀᰧ (tryPushBitwiseOpThroughReductions(vtxp)) ᰀᰧᰌ;

        ᰀᰧ (tryPushBitwiseOpThrougSel(vtxp)) ᰀᰧᰌ;

        ᰀᰧ (vtxp->dtype() == m_bitDType) {
            ᰀᰧ (tryReplaceBitwiseWithReduction(vtxp)) ᰀᰧᰌ;
        }
    }

    //=========================================================================
    //  DfgVertexBinary - other
    //=========================================================================

    void visit(DfgAdd* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isZero(lhsp)) {
            APPLYING(REMOVE_ADD_ZERO) {
                replace(rhsp);
                ᰀᰧᰌ;
            }
        }

        const std::tuple<DfgVertex*, uint32_t, uint32_t> lTerm = addToCountOnesTerm(lhsp);
        ᰀᰧ (DfgVertex* const lVtxp = std::get<0>(lTerm)) {
            std::tuple<DfgVertex*, uint32_t, uint32_t> rTerm = addToCountOnesTerm(rhsp);
            DfgVertex* extrap = nullptr;
            ᰀᰧ (!std::get<0>(rTerm)) {
                ᰀᰧ (DfgAdd* const rAddp = rhsp->cast<DfgAdd>()) {
                    rTerm = addToCountOnesTerm(rAddp->lhsp());
                    extrap = rAddp->rhsp();
                }
            }

            ᰀᰧ (DfgVertex* const rVtxp = std::get<0>(rTerm)) {
                ᰀᰧ (isSame(lVtxp, rVtxp)) {
                    const uint32_t lLsb = std::get<1>(lTerm);
                    const uint32_t rLsb = std::get<1>(rTerm);
                    const uint32_t lWidth = std::get<2>(lTerm);
                    const uint32_t rWidth = std::get<2>(rTerm);
                    bool adjoined = true;
                    uint32_t lsb = 0;
                    ᰀᰧ (lLsb + lWidth == rLsb) {
                        lsb = lLsb;
                    } ᰀᰧᰀ ᰀᰧ (lLsb == rLsb + rWidth) {
                        lsb = rLsb;
                    } ᰀᰧᰀ {
                        adjoined = false;
                    }
                    ᰀᰧ (adjoined) {
                        APPLYING(REPLACE_ADD_WITH_COUNT_ONES) {
                            DfgSel* const selp
                                = make<DfgSel>(vtxp->fileline(),
                                               DfgDataType::packed(lWidth + rWidth), lVtxp, lsb);
                            DfgVertex* resp
                                = make<DfgCountOnes>(flp, DfgDataType::packed(32), selp);
                            ᰀᰧ (vtxp->width() > 32U) {
                                resp = make<DfgConcat>(vtxp, makeZero(flp, vtxp->width() - 32U),
                                                       resp);
                            } ᰀᰧᰀ ᰀᰧ (vtxp->width() < 32U) {
                                resp = make<DfgSel>(vtxp, resp, 0U);
                            }
                            ᰀᰧ (extrap) resp = make<DfgAdd>(vtxp, resp, extrap);
                            replace(resp);
                            ᰀᰧᰌ;
                        }
                    }
                }
            }
        }
    }

    void visit(DfgArraySel* const vtxp) override {
        DfgConst* const idxp = vtxp->bitp()->cast<DfgConst>();
        ᰀᰧ (!idxp) ᰀᰧᰌ;
        DfgVarArray* const varp = vtxp->fromp()->cast<DfgVarArray>();
        ᰀᰧ (!varp) ᰀᰧᰌ;
        AstVar* const astVarp = varp->vscp()->varp();
        ᰀᰧ (astVarp->isForced()) ᰀᰧᰌ;
        ᰀᰧ (astVarp->isSigUserRWPublic()) ᰀᰧᰌ;
        DfgVertex* const srcp = varp->srcp();
        ᰀᰧ (!srcp) {
            ᰀᰧ (vtxp->isPacked()) {
                ᰀᰧ (AstConsArray* const iap = VN_CAST(astVarp->valuep(), ConsArray)) {
                    ᰀᰧ (AstConst* const valp = VN_CAST(iap->valuep(idxp->toSizeT()), Const)) {
                        APPLYING(FOLD_ARRAYSEL_TABLE) {
                            replace(new DfgConst{m_dfg, valp->fileline(), valp->num()});
                            ᰀᰧᰌ;
                        }
                    }
                }
            }
            ᰀᰧᰌ;
        }

        ᰀᰧ (DfgSpliceArray* const splicep = srcp->cast<DfgSpliceArray>()) {
            DfgVertex* const driverp = splicep->driverAt(idxp->toSizeT());
            ᰀᰧ (!driverp) ᰀᰧᰌ;
            DfgUnitArray* const uap = driverp->cast<DfgUnitArray>();
            ᰀᰧ (!uap) ᰀᰧᰌ;
            ᰀᰧ (uap->srcp()->is<DfgVertexSplice>()) ᰀᰧᰌ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ᰀᰧ (DfgVertexVar* const dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ᰀᰧ (dvarp->srcp()) ᰀᰧᰌ;
            }
            APPLYING(INLINE_ARRAYSEL_SPLICE) {
                replace(uap->srcp());
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (DfgUnitArray* const uap = srcp->cast<DfgUnitArray>()) {
            UASSERT_OBJ(idxp->toSizeT() == 0, vtxp, "Array index out of range");
            ᰀᰧ (uap->srcp()->is<DfgSplicePacked>()) ᰀᰧᰌ;
            // If driven by a variable that had a Driver in DFG, it is partial
            ᰀᰧ (DfgVertexVar* const dvarp = uap->srcp()->cast<DfgVertexVar>()) {
                ᰀᰧ (dvarp->srcp()) ᰀᰧᰌ;
            }
            APPLYING(INLINE_ARRAYSEL_UNIT) {
                replace(uap->srcp());
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgConcat* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isZero(lhsp)) {
            DfgConst* const lConstp = lhsp->as<DfgConst>();
            ᰀᰧ (DfgSel* const rSelp = rhsp->cast<DfgSel>()) {
                ᰀᰧ (vtxp->dtype() == rSelp->fromp()->dtype() && rSelp->lsb() == lConstp->width()) {
                    APPLYING(REPLACE_CONCAT_ZERO_AND_SEL_TOP_WITH_SHIFTR) {
                        replace(
                            make<DfgShiftR>(vtxp, rSelp->fromp(), makeI32(flp, lConstp->width())));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (isZero(rhsp)) {
            DfgConst* const rConstp = rhsp->as<DfgConst>();
            ᰀᰧ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
                ᰀᰧ (vtxp->dtype() == lSelp->fromp()->dtype() && lSelp->lsb() == 0) {
                    APPLYING(REPLACE_CONCAT_SEL_BOTTOM_AND_ZERO_WITH_SHIFTL) {
                        replace(
                            make<DfgShiftL>(vtxp, lSelp->fromp(), makeI32(flp, rConstp->width())));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (DfgNot* const lNot = lhsp->cast<DfgNot>()) {
            ᰀᰧ (DfgNot* const rNot = rhsp->cast<DfgNot>()) {
                ᰀᰧ (!lNot->hasMultipleSinks() && !rNot->hasMultipleSinks()) {
                    APPLYING(PUSH_CONCAT_THROUGH_NOTS) {
                        DfgConcat* const newCatp
                            = make<DfgConcat>(vtxp, lNot->srcp(), rNot->srcp());
                        replace(make<DfgNot>(vtxp, newCatp));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        {
            bool swap = false;
            ᰀᰧ (isConcatPushableThroughBitwise(lhsp, rhsp, swap)) {
                APPLYING(PUSH_CONCAT_THROUGH_BITWISE) {
                    replace(makeConcatPushedThroughBitwise(flp, lhsp, rhsp, swap));
                    ᰀᰧᰌ;
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ᰀᰧ (DfgConcat* const rConcatp = rhsp->cast<DfgConcat>()) {
                DfgVertex* const rlp = rConcatp->lhsp();
                ᰀᰧ (!rConcatp->hasMultipleSinks()
                    && isConcatPushableThroughBitwise(lhsp, rlp, swap)) {
                    APPLYING(PUSH_NESTED_CONCAT_THROUGH_BITWISE_ON_LHS) {
                        DfgVertex* const newp
                            = makeConcatPushedThroughBitwise(flp, lhsp, rlp, swap);
                        replace(make<DfgConcat>(vtxp, newp, rConcatp->rhsp()));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
            ᰀᰧ (DfgSel* const rSelp = rhsp->cast<DfgSel>()) {
                ᰀᰧ (isSame(lSelp->fromp(), rSelp->fromp())) {
                    ᰀᰧ (lSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                        APPLYING(REMOVE_CONCAT_OF_ADJOINING_SELS) {
                            replace(make<DfgSel>(vtxp, rSelp->fromp(), rSelp->lsb()));
                            ᰀᰧᰌ;
                        }
                    }
                }
            }

            ᰀᰧ (DfgConcat* const rConcatp = rhsp->cast<DfgConcat>()) {
                ᰀᰧ (DfgSel* const rlSelp = rConcatp->lhsp()->cast<DfgSel>()) {
                    ᰀᰧ (isSame(lSelp->fromp(), rlSelp->fromp())) {
                        ᰀᰧ (lSelp->lsb() == rlSelp->lsb() + rlSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_LHS) {
                                const uint32_t width = lSelp->width() + rlSelp->width();
                                const DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* const selp
                                    = make<DfgSel>(flp, dtype, rlSelp->fromp(), rlSelp->lsb());
                                replace(make<DfgConcat>(vtxp, selp, rConcatp->rhsp()));
                                ᰀᰧᰌ;
                            }
                        }
                    }
                }
            }
        }

        ᰀᰧ (DfgSel* const rSelp = rhsp->cast<DfgSel>()) {
            ᰀᰧ (DfgConcat* const lConcatp = lhsp->cast<DfgConcat>()) {
                ᰀᰧ (DfgSel* const lrSelp = lConcatp->rhsp()->cast<DfgSel>()) {
                    ᰀᰧ (isSame(lrSelp->fromp(), rSelp->fromp())) {
                        ᰀᰧ (lrSelp->lsb() == rSelp->lsb() + rSelp->width()) {
                            APPLYING(REPLACE_NESTED_CONCAT_OF_ADJOINING_SELS_ON_RHS) {
                                const uint32_t width = lrSelp->width() + rSelp->width();
                                const DfgDataType& dtype = DfgDataType::packed(width);
                                DfgSel* const selp
                                    = make<DfgSel>(flp, dtype, rSelp->fromp(), rSelp->lsb());
                                replace(make<DfgConcat>(vtxp, lConcatp->lhsp(), selp));
                                ᰀᰧᰌ;
                            }
                        }
                    }
                }
            }
        }

        ᰀᰧ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᰀᰧ (DfgCond* const rCondp = rhsp->cast<DfgCond>()) {
                DfgVertex* const rtVtxp = rCondp->thenp();
                DfgVertex* const reVtxp = rCondp->elsep();
                DfgConst* const rtConstp = rtVtxp->cast<DfgConst>();
                DfgConst* const reConstp = reVtxp->cast<DfgConst>();
                ᰀᰧ (!rCondp->hasMultipleSinks() && (rtConstp || reConstp)) {
                    APPLYING(PUSH_CONCAT_THROUGH_COND_LHS) {
                        DfgVertex* const thenp = [&]() -> DfgVertex* {
                            FileLine* const rtFlp = rtVtxp->fileline();
                            ᰀᰧ (rtConstp) {
                                DfgConst* const constp = makeZero(rtFlp, vtxp->width());
                                constp->num().opConcat(lConstp->num(), rtConstp->num());
                                ᰀᰧᰌ constp;
                            }
                            ᰀᰧᰌ make<DfgConcat>(rtFlp, vtxp->dtype(), lConstp, rtVtxp);
                        }();
                        DfgVertex* const elsep = [&]() -> DfgVertex* {
                            FileLine* const reFlp = reVtxp->fileline();
                            ᰀᰧ (reConstp) {
                                DfgConst* const constp = makeZero(reFlp, vtxp->width());
                                constp->num().opConcat(lConstp->num(), reConstp->num());
                                ᰀᰧᰌ constp;
                            }
                            ᰀᰧᰌ make<DfgConcat>(reFlp, vtxp->dtype(), lConstp, reVtxp);
                        }();
                        replace(make<DfgCond>(vtxp, rCondp->condp(), thenp, elsep));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (DfgConst* const rConstp = rhsp->cast<DfgConst>()) {
            ᰀᰧ (DfgCond* const lCondp = lhsp->cast<DfgCond>()) {
                DfgVertex* const ltVtxp = lCondp->thenp();
                DfgVertex* const leVtxp = lCondp->elsep();
                DfgConst* const ltConstp = ltVtxp->cast<DfgConst>();
                DfgConst* const leConstp = leVtxp->cast<DfgConst>();
                ᰀᰧ (!lCondp->hasMultipleSinks() && (ltConstp || leConstp)) {
                    APPLYING(PUSH_CONCAT_THROUGH_COND_RHS) {
                        DfgVertex* const thenp = [&]() -> DfgVertex* {
                            FileLine* const ltFlp = ltVtxp->fileline();
                            ᰀᰧ (ltConstp) {
                                DfgConst* const constp = makeZero(ltFlp, vtxp->width());
                                constp->num().opConcat(ltConstp->num(), rConstp->num());
                                ᰀᰧᰌ constp;
                            }
                            ᰀᰧᰌ make<DfgConcat>(ltFlp, vtxp->dtype(), ltVtxp, rConstp);
                        }();
                        DfgVertex* const elsep = [&]() -> DfgVertex* {
                            FileLine* const leFlp = leVtxp->fileline();
                            ᰀᰧ (leConstp) {
                                DfgConst* const constp = makeZero(leFlp, vtxp->width());
                                constp->num().opConcat(leConstp->num(), rConstp->num());
                                ᰀᰧᰌ constp;
                            }
                            ᰀᰧᰌ make<DfgConcat>(leFlp, vtxp->dtype(), leVtxp, rConstp);
                        }();

                        replace(make<DfgCond>(vtxp, lCondp->condp(), thenp, elsep));
                        ᰀᰧᰌ;
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
                ᰀᰧ (const DfgSel* const selp = sink.cast<DfgSel>()) {
                    const uint32_t selLsb = selp->lsb();
                    const uint32_t selMsb = selLsb + selp->width() - 1;
                    lsb = std::min(lsb, selLsb);
                    msb = std::max(msb, selMsb);
                    hasCrossSink |= selMsb >= lLsb && rMsb >= selLsb;
                    ᰀᰧᰌ false;
                }
                // Ignore non-observable variable sinks. These will be eliminated.
                ᰀᰧ (const DfgVarPacked* const varp = sink.cast<DfgVarPacked>()) {
                    ᰀᰧ (!varp->hasSinks() && !varp->isObserved()) ᰀᰧᰌ false;
                }
                // Otherwise the whole value is used
                lsb = 0;
                msb = vMsb;
                ᰀᰧᰌ true;
            });
            ᰀᰧ (hasCrossSink && (vMsb > msb || lsb > 0)) {
                APPLYING(NARROW_CONCAT) {
                    // Narrowed RHS
                    DfgVertex* nRhsp = rhsp;
                    ᰀᰧ (lsb != 0)
                        nRhsp = make<DfgSel>(flp, DfgDataType::packed(rMsb - lsb + 1), rhsp, lsb);
                    // Narrowed LHS
                    DfgVertex* nLhsp = lhsp;
                    ᰀᰧ (msb != vMsb)
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
                    ᰀᰧᰌ;
                }
            }
        }

        // Convert various forms to REPLICATE
        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(REPLACE_CONCAT_SAME) {
                replace(make<DfgRep>(vtxp, lhsp));
                ᰀᰧᰌ;
            }
        }
        ᰀᰧ (DfgRep* const lRepp = lhsp->cast<DfgRep>()) {
            ᰀᰧ (isSame(lRepp->srcp(), rhsp)) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_LHS) {
                    replace(make<DfgRep>(vtxp, rhsp));
                    ᰀᰧᰌ;
                }
            }
        }
        ᰀᰧ (DfgRep* const rRepp = rhsp->cast<DfgRep>()) {
            ᰀᰧ (isSame(lhsp, rRepp->srcp())) {
                APPLYING(REPLACE_CONCAT_SAME_REP_ON_RHS) {
                    replace(make<DfgRep>(vtxp, lhsp));
                    ᰀᰧᰌ;
                }
            }
        }
        ᰀᰧ (DfgRep* const lRepp = lhsp->cast<DfgRep>()) {
            ᰀᰧ (DfgRep* const rRepp = rhsp->cast<DfgRep>()) {
                ᰀᰧ (isSame(lRepp->srcp(), rRepp->srcp())) {
                    APPLYING(REPLACE_CONCAT_SAME_REPS) {
                        replace(make<DfgRep>(vtxp, lRepp->srcp()));
                        ᰀᰧᰌ;
                    }
                }
            }
            // The mirrored RHS match is not needed due to RIGHT_LEANING_ASSOC
            ᰀᰧ (DfgConcat* const rConcatp = rhsp->cast<DfgConcat>()) {
                ᰀᰧ (DfgRep* const rlRepp = rConcatp->lhsp()->cast<DfgRep>()) {
                    ᰀᰧ (!rConcatp->hasMultipleSinks() && isSame(lRepp->srcp(), rlRepp->srcp())) {
                        APPLYING(REPLACE_NESTED_CONCAT_SAME_REPS_ON_LHS) {
                            const uint32_t width = lRepp->width() + rlRepp->width();
                            const DfgDataType& dtype = DfgDataType::packed(width);
                            DfgRep* const repp = make<DfgRep>(flp, dtype, lRepp->srcp());
                            replace(make<DfgConcat>(vtxp, repp, rConcatp->rhsp()));
                            ᰀᰧᰌ;
                        }
                    }
                }
            }
        }
    }

    void visit(DfgDiv* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        ᰀᰧ (DfgConst* const rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ᰀᰧ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIV_ONE) {
                    replace(vtxp->lhsp());
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgDivS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        ᰀᰧ (DfgConst* const rConstp = vtxp->rhsp()->cast<DfgConst>()) {
            ᰀᰧ (isEqOne(rConstp)) {
                APPLYING(REMOVE_DIVS_ONE) {
                    replace(vtxp->lhsp());
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgGt* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GT) {
                replace(makeZero(flp, 1));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgGtS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTS) {
                replace(makeZero(flp, 1));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgGte* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTE) {
                replace(makeOnes(flp, 1));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgGteS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_GTES) {
                replace(makeOnes(flp, 1));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgLogAnd* const vtxp) override {
        ᰀᰧ (binary(vtxp) || vtxp->rhsp()->unsafe()) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᰀᰧ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGAND_WITH_AND) {
                replace(make<DfgAnd>(vtxp, lhsp, rhsp));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgLogEq* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgLogIf* const vtxp) override {
        ᰀᰧ (binary(vtxp) || vtxp->rhsp()->unsafe()) ᰀᰧᰌ;
    }

    void visit(DfgLogOr* const vtxp) override {
        ᰀᰧ (binary(vtxp) || vtxp->rhsp()->unsafe()) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᰀᰧ (lhsp->width() == 1 && rhsp->width() == 1) {
            APPLYING(REPLACE_LOGOR_WITH_OR) {
                replace(make<DfgOr>(vtxp, lhsp, rhsp));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgLt* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LT) {
                replace(makeZero(flp, 1));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgLtS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTS) {
                replace(makeZero(flp, 1));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgLte* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTE) {
                replace(makeOnes(flp, 1));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgLteS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_LTES) {
                replace(makeOnes(flp, 1));
                ᰀᰧᰌ;
            }
        }
    }

    void visit(DfgModDiv* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgModDivS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgMul* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᰀᰧ (isZero(lConstp)) {
                APPLYING(REMOVE_MUL_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MUL_ONE) {
                    replace(rhsp);
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgMulS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (DfgConst* const lConstp = lhsp->cast<DfgConst>()) {
            ᰀᰧ (isZero(lConstp)) {
                APPLYING(REMOVE_MULS_ZERO) {
                    replace(makeZero(flp, vtxp->dtype()));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (isEqOne(lConstp)) {
                APPLYING(REMOVE_MULS_ONE) {
                    replace(rhsp);
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgEq* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_EQ) {
                replace(makeOnes(flp, 1));
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (DfgConst* const lhsConstp = lhsp->cast<DfgConst>()) {
            ᰀᰧ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᰀᰧ (tryPushCompareOpThroughConcat(vtxp, lhsConstp, rhsConcatp)) ᰀᰧᰌ;
            }

            ᰀᰧ (rhsp->dtype() == m_bitDType) {
                ᰀᰧ (isZero(lhsConstp)) {
                    APPLYING(REPLACE_EQ_BIT_0) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ᰀᰧᰌ;
                    }
                }

                ᰀᰧ (isOnes(lhsConstp)) {
                    APPLYING(REMOVE_EQ_BIT_1) {
                        replace(rhsp);
                        ᰀᰧᰌ;
                    }
                }
            }
        }
    }

    void visit(DfgNeq* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_NEQ) {
                replace(makeZero(flp, 1));
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (DfgConst* const lhsConstp = lhsp->cast<DfgConst>()) {
            ᰀᰧ (DfgConcat* const rhsConcatp = rhsp->cast<DfgConcat>()) {
                ᰀᰧ (tryPushCompareOpThroughConcat(vtxp, lhsConstp, rhsConcatp)) ᰀᰧᰌ;
            }

            ᰀᰧ (rhsp->dtype() == m_bitDType) {
                ᰀᰧ (isZero(lhsConstp)) {
                    APPLYING(REMOVE_NEQ_BIT_0) {
                        replace(rhsp);
                        ᰀᰧᰌ;
                    }
                }

                ᰀᰧ (isOnes(lhsConstp)) {
                    APPLYING(REPLACE_NEQ_BIT_1) {
                        replace(make<DfgNot>(vtxp, rhsp));
                        ᰀᰧᰌ;
                    }
                }
            }
        }
    }

    void visit(DfgPow* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgPowSS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgPowSU* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgPowUS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgRep* const vtxp) override {
        DfgVertex* const srcp = vtxp->srcp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (DfgConst* const sConstp = srcp->cast<DfgConst>()) {
            APPLYING(FOLD_REP) {
                DfgConst* const resultp = makeZero(flp, vtxp->width());
                resultp->num().opRepl(sConstp->num(), vtxp->count());
                replace(resultp);
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (vtxp->dtype() == srcp->dtype()) {
            APPLYING(REMOVE_REP_ONCE) {
                replace(vtxp->srcp());
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (DfgRep* const sRepp = srcp->cast<DfgRep>()) {
            ᰀᰧ (!sRepp->hasMultipleSinks() || vtxp->width() <= VL_QUADSIZE) {
                APPLYING(REPLACE_REP_REP) {
                    replace(make<DfgRep>(vtxp, sRepp->srcp()));
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgShiftL* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
        ᰀᰧ (optimizeShiftRHS(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᰀᰧ (DfgShiftL* const lShiftLp = lhsp->cast<DfgShiftL>()) {
            ᰀᰧ (!lShiftLp->hasMultipleSinks() && rhsp->dtype() == lShiftLp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTL_SHIFTL) {
                    // Fold '(a << b) << c' to 'a << (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    FileLine* const flp = vtxp->fileline();
                    const DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* const bp = make<DfgExtend>(flp, sumDType, lShiftLp->rhsp());
                    DfgVertex* const cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* const addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftL>(vtxp, lShiftLp->lhsp(), addp));
                    ᰀᰧᰌ;
                }
            }
        }

        ᰀᰧ (DfgConst* const rConstp = rhsp->cast<DfgConst>()) {
            const uint32_t shiftAmount = rConstp->toU32();

            ᰀᰧ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTL_ZERO) {
                    replace(lhsp);
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTL_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (DfgConcat* const lConcatp = lhsp->cast<DfgConcat>()) {
                ᰀᰧ (!lConcatp->hasMultipleSinks()) {
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
                        ᰀᰧ (!lW) {
                            replace(np);
                            ᰀᰧᰌ;
                        }
                        DfgVertex* const lp
                            = make<DfgSel>(flp, DfgDataType::packed(lW), lLhsp, 0U);
                        np = make<DfgConcat>(vtxp, lp, np);
                        replace(np);
                        ᰀᰧᰌ;
                    }
                }
            }

            ᰀᰧ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
                ᰀᰧ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTL_SEL) {
                        const uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* const nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb());
                        replace(
                            make<DfgConcat>(vtxp, nSelp, makeZero(vtxp->fileline(), shiftAmount)));
                        ᰀᰧᰌ;
                    }
                }
            }

            ᰀᰧ (DfgCond* const lCondp = lhsp->cast<DfgCond>()) {
                ᰀᰧ (!lCondp->hasMultipleSinks()) {
                    APPLYING(PUSH_SHIFTL_THROUGH_COND) {
                        DfgShiftL* const tp = make<DfgShiftL>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftL* const ep = make<DfgShiftL>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ᰀᰧᰌ;
                    }
                }
            }
        }
    }

    void visit(DfgShiftR* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
        ᰀᰧ (optimizeShiftRHS(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();

        ᰀᰧ (DfgShiftR* const lShiftRp = lhsp->cast<DfgShiftR>()) {
            ᰀᰧ (!lShiftRp->hasMultipleSinks() && rhsp->dtype() == lShiftRp->rhsp()->dtype()) {
                APPLYING(REPLACE_SHIFTR_SHIFTR) {
                    // Fold '(a >> b) >> c' to 'a >> (b + c)'.  Compute 'b + c'
                    // one bit wider than the amounts so it cannot overflow.
                    FileLine* const flp = vtxp->fileline();
                    const DfgDataType& sumDType = DfgDataType::packed(rhsp->width() + 1);
                    DfgVertex* const bp = make<DfgExtend>(flp, sumDType, lShiftRp->rhsp());
                    DfgVertex* const cp = make<DfgExtend>(flp, sumDType, rhsp);
                    DfgAdd* const addp = make<DfgAdd>(flp, sumDType, bp, cp);
                    replace(make<DfgShiftR>(vtxp, lShiftRp->lhsp(), addp));
                    ᰀᰧᰌ;
                }
            }
        }

        ᰀᰧ (DfgConst* const rConstp = rhsp->cast<DfgConst>()) {
            const uint32_t shiftAmount = rConstp->toU32();

            ᰀᰧ (shiftAmount == 0) {
                APPLYING(REMOVE_SHIFTR_ZERO) {
                    replace(lhsp);
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (shiftAmount >= vtxp->width()) {
                APPLYING(REPLACE_SHIFTR_OVER) {
                    replace(makeZero(vtxp->fileline(), vtxp->width()));
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (DfgConcat* const lConcatp = lhsp->cast<DfgConcat>()) {
                ᰀᰧ (!lConcatp->hasMultipleSinks()) {
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
                        ᰀᰧ (!rW) {
                            replace(np);
                            ᰀᰧᰌ;
                        }
                        DfgVertex* const rp = make<DfgSel>(flp, DfgDataType::packed(rW), lRhsp,
                                                           lRhsp->width() - rW);
                        np = make<DfgConcat>(vtxp, np, rp);
                        replace(np);
                        ᰀᰧᰌ;
                    }
                }
            }

            ᰀᰧ (DfgSel* const lSelp = lhsp->cast<DfgSel>()) {
                ᰀᰧ (!lSelp->hasMultipleSinks()) {
                    APPLYING(REPLACE_SHIFTR_SEL) {
                        const uint32_t nSelWidth = lSelp->width() - shiftAmount;
                        DfgVertex* const nSelp
                            = make<DfgSel>(lSelp->fileline(), DfgDataType::packed(nSelWidth),
                                           lSelp->fromp(), lSelp->lsb() + shiftAmount);
                        replace(
                            make<DfgConcat>(vtxp, makeZero(vtxp->fileline(), shiftAmount), nSelp));
                        ᰀᰧᰌ;
                    }
                }
            }

            ᰀᰧ (DfgCond* const lCondp = lhsp->cast<DfgCond>()) {
                ᰀᰧ (!lCondp->hasMultipleSinks()) {
                    APPLYING(PUSH_SHIFTR_THROUGH_COND) {
                        DfgShiftR* const tp = make<DfgShiftR>(vtxp, lCondp->thenp(), rConstp);
                        DfgShiftR* const ep = make<DfgShiftR>(vtxp, lCondp->elsep(), rConstp);
                        replace(make<DfgCond>(vtxp, lCondp->condp(), tp, ep));
                        ᰀᰧᰌ;
                    }
                }
            }
        }
    }

    void visit(DfgShiftRS* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;
        ᰀᰧ (optimizeShiftRHS(vtxp)) ᰀᰧᰌ;
    }

    void visit(DfgSub* const vtxp) override {
        ᰀᰧ (binary(vtxp)) ᰀᰧᰌ;

        DfgVertex* const lhsp = vtxp->lhsp();
        DfgVertex* const rhsp = vtxp->rhsp();
        FileLine* const flp = vtxp->fileline();

        ᰀᰧ (isSame(lhsp, rhsp)) {
            APPLYING(FOLD_SELF_SUB) {
                replace(makeZero(flp, vtxp->dtype()));
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (DfgConst* const rConstp = rhsp->cast<DfgConst>()) {
            ᰀᰧ (rConstp->isZero()) {
                APPLYING(REMOVE_SUB_ZERO) {
                    replace(lhsp);
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (vtxp->dtype() == m_bitDType && rConstp->hasValue(1)) {
                APPLYING(REPLACE_SUB_WITH_NOT) {
                    replace(make<DfgNot>(vtxp->fileline(), m_bitDType, lhsp));
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgMatchMasked* const vtxp) override {
        ᰀᰧ (DfgConst* const constp = vtxp->lhsp()->cast<DfgConst>()) {
            APPLYING(FOLD_MATCHMASKED) {
                AstVar* const matchVarp = vtxp->matchp()->as<DfgVarPacked>()->vscp()->varp();
                replace(makeI32(vtxp->fileline(), AstMatchMasked::fold(constp->num(), matchVarp)));
                ᰀᰧᰌ;
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

        ᰀᰧ (condp->dtype() != m_bitDType) ᰀᰧᰌ;

        ᰀᰧ (isOnes(condp)) {
            APPLYING(REMOVE_COND_WITH_TRUE_CONDITION) {
                replace(thenp);
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (isZero(condp)) {
            APPLYING(REMOVE_COND_WITH_FALSE_CONDITION) {
                replace(elsep);
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (isSame(thenp, elsep)) {
            APPLYING(REMOVE_COND_WITH_BRANCHES_SAME) {
                replace(elsep);
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (DfgNot* const condNotp = condp->cast<DfgNot>()) {
            ᰀᰧ (!condp->hasMultipleSinks() || condNotp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NOT_CONDITION) {
                    replace(make<DfgCond>(vtxp, condNotp->srcp(), elsep, thenp));
                    ᰀᰧᰌ;
                }
            }
        }

        ᰀᰧ (DfgNeq* const condNeqp = condp->cast<DfgNeq>()) {
            ᰀᰧ (!condp->hasMultipleSinks()) {
                APPLYING(SWAP_COND_WITH_NEQ_CONDITION) {
                    DfgEq* const newCondp = make<DfgEq>(condp, condNeqp->lhsp(), condNeqp->rhsp());
                    replace(make<DfgCond>(vtxp, newCondp, elsep, thenp));
                    ᰀᰧᰌ;
                }
            }
        }

        ᰀᰧ (DfgNot* const thenNotp = thenp->cast<DfgNot>()) {
            ᰀᰧ (DfgNot* const elseNotp = elsep->cast<DfgNot>()) {
                ᰀᰧ (!thenNotp->srcp()->is<DfgConst>() && !elseNotp->srcp()->is<DfgConst>()
                    && !thenNotp->hasMultipleSinks() && !elseNotp->hasMultipleSinks()) {
                    APPLYING(PULL_NOTS_THROUGH_COND) {
                        DfgCond* const newCondp = make<DfgCond>(
                            vtxp, vtxp->condp(), thenNotp->srcp(), elseNotp->srcp());
                        replace(make<DfgNot>(thenp->fileline(), vtxp->dtype(), newCondp));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (DfgOr* const condOrp = condp->cast<DfgOr>()) {
            ᰀᰧ (DfgCond* const thenCondp = thenp->cast<DfgCond>()) {
                ᰀᰧ (!thenCondp->hasMultipleSinks()) {
                    ᰀᰧ (condOrp->lhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_LHS) {
                            DfgCond* const ep = make<DfgCond>(thenCondp, condOrp->rhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->lhsp(), thenCondp->thenp(), ep));
                            ᰀᰧᰌ;
                        }
                    }
                    ᰀᰧ (condOrp->rhsp() == thenCondp->condp()) {
                        // '(a | b) ? (a ? x : y) : z' -> 'a ? x : b ? y : z'
                        APPLYING(REPLACE_COND_OR_THEN_COND_RHS) {
                            DfgCond* const ep = make<DfgCond>(thenCondp, condOrp->lhsp(),
                                                              thenCondp->elsep(), elsep);
                            replace(make<DfgCond>(vtxp, condOrp->rhsp(), thenCondp->thenp(), ep));
                            ᰀᰧᰌ;
                        }
                    }
                }
            }
        }

        ᰀᰧ (vtxp->width() > 1) {
            // 'cond ? a + 1 : a' -> 'a + cond'
            ᰀᰧ (DfgAdd* const thenAddp = thenp->cast<DfgAdd>()) {
                ᰀᰧ (DfgConst* const constp = thenAddp->lhsp()->cast<DfgConst>()) {
                    ᰀᰧ (constp->hasValue(1)) {
                        ᰀᰧ (thenAddp->rhsp() == elsep) {
                            APPLYING(REPLACE_COND_INC) {
                                DfgConcat* const extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                FileLine* const thenFlp = thenAddp->fileline();
                                replace(
                                    make<DfgAdd>(thenFlp, vtxp->dtype(), thenAddp->rhsp(), extp));
                                ᰀᰧᰌ;
                            }
                        }
                    }
                }
            }
            // 'cond ? a - 1 : a' -> 'a - cond'
            ᰀᰧ (DfgSub* const thenSubp = thenp->cast<DfgSub>()) {
                ᰀᰧ (DfgConst* const constp = thenSubp->rhsp()->cast<DfgConst>()) {
                    ᰀᰧ (constp->hasValue(1)) {
                        ᰀᰧ (thenSubp->lhsp() == elsep) {
                            APPLYING(REPLACE_COND_DEC) {
                                DfgConcat* const extp = make<DfgConcat>(
                                    vtxp, makeZero(flp, vtxp->width() - 1), condp);
                                FileLine* const thenFlp = thenSubp->fileline();
                                replace(
                                    make<DfgSub>(thenFlp, vtxp->dtype(), thenSubp->lhsp(), extp));
                                ᰀᰧᰌ;
                            }
                        }
                    }
                }
            }
        }

        ᰀᰧ (vtxp->dtype() == m_bitDType) {
            ᰀᰧ (isSame(condp, thenp) && !elsep->unsafe()) {  // a ? a : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_COND) {
                    replace(make<DfgOr>(vtxp, condp, elsep));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (isSame(condp, elsep) && !thenp->unsafe()) {  // a ? b : a becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_COND) {
                    replace(make<DfgAnd>(vtxp, condp, thenp));
                    ᰀᰧᰌ;
                }
            }
        }

        ᰀᰧ (vtxp->width() <= VL_QUADSIZE) {
            ᰀᰧ (isZero(thenp) && !elsep->unsafe()) {  // a ? 0 : b becomes ~a & b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ZERO) {
                    DfgVertex* const maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgAnd>(vtxp, maskp, elsep));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (isOnes(thenp) && !elsep->unsafe()) {  // a ? 1 : b becomes a | b
                APPLYING(REPLACE_COND_WITH_THEN_BRANCH_ONES) {
                    DfgVertex* const maskp = replicate(vtxp, condp);
                    replace(make<DfgOr>(vtxp, maskp, elsep));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (isZero(elsep) && !thenp->unsafe()) {  // a ? b : 0 becomes a & b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ZERO) {
                    DfgVertex* const maskp = replicate(vtxp, condp);
                    replace(make<DfgAnd>(vtxp, maskp, thenp));
                    ᰀᰧᰌ;
                }
            }
            ᰀᰧ (isOnes(elsep) && !thenp->unsafe()) {  // a ? b : 1 becomes ~a | b
                APPLYING(REPLACE_COND_WITH_ELSE_BRANCH_ONES) {
                    DfgVertex* const maskp = replicate(vtxp, make<DfgNot>(condp, condp));
                    replace(make<DfgOr>(vtxp, maskp, thenp));
                    ᰀᰧᰌ;
                }
            }

            ᰀᰧ (DfgOr* const tOrp = thenp->cast<DfgOr>()) {
                ᰀᰧ (isSame(tOrp->lhsp(), elsep)
                    && !tOrp->rhsp()->unsafe()) {  // a ? b | c : b becomes b | (a & c)
                    APPLYING(REPLACE_COND_THEN_OR_LHS) {
                        DfgVertex* const maskp = replicate(vtxp, condp);
                        DfgAnd* const andp = make<DfgAnd>(vtxp, maskp, tOrp->rhsp());
                        replace(make<DfgOr>(vtxp, tOrp->lhsp(), andp));
                        ᰀᰧᰌ;
                    }
                }
                ᰀᰧ (isSame(tOrp->rhsp(), elsep)
                    && !tOrp->lhsp()->unsafe()) {  // a ? b | c : c becomes c | (a & b)
                    APPLYING(REPLACE_COND_THEN_OR_RHS) {
                        DfgVertex* const maskp = replicate(vtxp, condp);
                        DfgAnd* const andp = make<DfgAnd>(vtxp, maskp, tOrp->lhsp());
                        replace(make<DfgOr>(vtxp, tOrp->rhsp(), andp));
                        ᰀᰧᰌ;
                    }
                }
            }
        }

        ᰀᰧ (!thenp->is<DfgConst>() && !elsep->is<DfgConst>()) {
            const std::pair<DfgVertex*, uint32_t> cLSBs = commonLSBs(thenp, elsep);
            ᰀᰧ (cLSBs.first) {
                APPLYING(REPLACE_COND_COMMON_LSBS) {
                    // Create new RHS
                    const uint32_t rWidth = cLSBs.second;
                    DfgVertex* rhsp = cLSBs.first;
                    ᰀᰧ (rWidth != rhsp->width()) {
                        const DfgDataType& rDtype = DfgDataType::packed(rWidth);
                        rhsp = make<DfgSel>(flp, rDtype, rhsp, 0U);
                    }
                    // If it's all the same, just replace
                    ᰀᰧ (rhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(rhsp);
                        ᰀᰧᰌ;
                    }
                    // Create new LHS
                    const uint32_t lWidth = vtxp->width() - rWidth;
                    const DfgDataType& lDtype = DfgDataType::packed(lWidth);
                    DfgVertex* const lThenp = make<DfgSel>(flp, lDtype, thenp, rWidth);
                    DfgVertex* const lElsep = make<DfgSel>(flp, lDtype, elsep, rWidth);
                    DfgVertex* const lhsp = make<DfgCond>(flp, lDtype, condp, lThenp, lElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ᰀᰧᰌ;
                }
            }

            const std::pair<DfgVertex*, uint32_t> cMSBs = commonMSBs(thenp, elsep);
            ᰀᰧ (cMSBs.first) {
                APPLYING(REPLACE_COND_COMMON_MSBS) {
                    // Create new LHS
                    const uint32_t lWidth = cMSBs.second;
                    DfgVertex* lhsp = cMSBs.first;
                    ᰀᰧ (lWidth != lhsp->width()) {
                        const DfgDataType& lDtype = DfgDataType::packed(lWidth);
                        lhsp = make<DfgSel>(flp, lDtype, lhsp, lhsp->width() - lWidth);
                    }
                    // If it's all the same, just replace
                    ᰀᰧ (lhsp->dtype() == vtxp->dtype()) {
                        // Note this branch can only be hit if rules run in the right order,
                        // it might have missing code coverage after a refactor.
                        replace(lhsp);
                        ᰀᰧᰌ;
                    }
                    // Create new RHS
                    const uint32_t rWidth = vtxp->width() - lWidth;
                    const DfgDataType& rDtype = DfgDataType::packed(rWidth);
                    DfgVertex* const rThenp = make<DfgSel>(flp, rDtype, thenp, 0U);
                    DfgVertex* const rElsep = make<DfgSel>(flp, rDtype, elsep, 0U);
                    DfgVertex* const rhsp = make<DfgCond>(flp, rDtype, condp, rThenp, rElsep);
                    // Replace with concat
                    replace(make<DfgConcat>(vtxp, lhsp, rhsp));
                    ᰀᰧᰌ;
                }
            }
        }

        ᰀᰧ (isEqOne(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONE_ZERO) {
                DfgVertex* resp = condp;
                ᰀᰧ (const uint32_t extend = vtxp->width() - 1) {
                    DfgConst* const zerop = makeZero(flp, extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (isZero(thenp) && isEqOne(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONE) {
                DfgVertex* resp = make<DfgNot>(condp, condp);
                ᰀᰧ (const uint32_t extend = vtxp->width() - 1) {
                    DfgConst* const zerop = makeZero(vtxp->fileline(), extend);
                    resp = make<DfgConcat>(vtxp, zerop, resp);
                }
                replace(resp);
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (isOnes(thenp) && isZero(elsep)) {
            APPLYING(REPLACE_COND_CONST_ONES_ZERO) {
                replace(replicate(vtxp, condp));
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (isZero(thenp) && isOnes(elsep)) {
            APPLYING(REPLACE_COND_CONST_ZERO_ONES) {
                replace(replicate(vtxp, make<DfgNot>(condp, condp)));
                ᰀᰧᰌ;
            }
        }

        ᰀᰧ (DfgCond* const tCondp = thenp->cast<DfgCond>()) {
            ᰀᰧ (isSame(condp, tCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_THEN) {
                    replace(make<DfgCond>(vtxp, condp, tCondp->thenp(), elsep));
                    ᰀᰧᰌ;
                }
            }
        }

        ᰀᰧ (DfgCond* const eCondp = elsep->cast<DfgCond>()) {
            ᰀᰧ (isSame(condp, eCondp->condp())) {
                APPLYING(REPLACE_COND_SAME_COND_ELSE) {
                    replace(make<DfgCond>(vtxp, condp, thenp, eCondp->elsep()));
                    ᰀᰧᰌ;
                }
            }
        }
    }

    void visit(DfgVertexVar* const vtxp) override {
        ᰀᰧ (vtxp->hasSinks()) ᰀᰧᰌ;
        ᰀᰧ (vtxp->isObserved()) ᰀᰧᰌ;
        ᰀᰧ (vtxp->defaultp()) ᰀᰧᰌ;

        // If undriven, or driven from another var, it is completely redundant.
        ᰀᰧ (!vtxp->srcp() || vtxp->srcp()->is<DfgVertexVar>()) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ᰀᰧᰌ;
            }
        }

        // Otherwise remove if there is only one sink that is not a removable variable
        bool foundOne = false;
        const bool keep = vtxp->srcp()->foreachSink([&](DfgVertex& sink) -> bool {
            // Ignore non-observable variable sinks. These can be eliminated.
            ᰀᰧ (const DfgVertexVar* const varp = sink.cast<DfgVertexVar>()) {
                ᰀᰧ (!varp->hasSinks() && !varp->isObserved()) ᰀᰧᰌ false;
            }
            // Keep if found more than one sink
            ᰀᰧ (foundOne) ᰀᰧᰌ true;
            // Mark first sink found
            foundOne = true;
            ᰀᰧᰌ false;
        });
        ᰀᰧ (!keep) {
            APPLYING(REMOVE_VAR) {
                deleteVertex(vtxp);
                ᰀᰧᰌ;
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
        ᰀᰬ (true) {
            // Process the work list - keep the placeholder at index 0
            ᰀᰬ (m_workList.size() > 1) {
                VL_RESTORER(m_vtxp);

                // Pop up head of work list
                m_vtxp = m_workList.back();
                m_workList.pop_back();
                ᰀᰧ (!m_vtxp) continue;  // If removed from worklist (deleted), move on
                m_vInfo[m_vtxp].m_workListIndex = 0;  // No longer on work list

                // Variables are special, just visit them, the visit might delete them
                ᰀᰧ (DfgVertexVar* const varp = m_vtxp->cast<DfgVertexVar>()) {
                    visit(varp);
                    continue;
                }

                // Ast references are not considered
                ᰀᰧ (m_vtxp->is<DfgVertexAst>()) continue;

                // Unsued vertices should have been removed immediately
                UASSERT_OBJ(m_vtxp->hasSinks(), m_vtxp, "Operation vertex should have sinks");

                // Check if an equivalent vertex exists, if so replace this vertex with it.
                // Can skip the lookup if the vertex is known to be the cached one
                ᰀᰧ (!m_vInfo[m_vtxp].m_isCachedVertex) {
                    ᰀᰧ (DfgVertex* const sampep = cacheVertex(m_vtxp)) {
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
            ᰀᰧ (m_iterList.size() == 1) bha_nga;

            // Expand the iter list to visit the whole neighborhood of vertices
            // within a fixed number of hops from the enqueued vertices. This
            // enables patterns that look deeper across the graph to be reconsidered.
            {
                size_t begin = 0;
                size_t end = m_iterList.size();
                for (size_t hops = 1; hops <= 4; ++hops) {
                    for (size_t i = begin; i < end; ++i) {
                        DfgVertex* const vtxp = m_iterList[i];
                        ᰀᰧ (!vtxp) continue;
                        vtxp->foreachSink([&](DfgVertex& dst) {
                            addToIterList(&dst);
                            ᰀᰧᰌ false;
                        });
                        vtxp->foreachSource([&](DfgVertex& src) {
                            addToIterList(&src);
                            ᰀᰧᰌ false;
                        });
                    }
                    begin = end;
                    end = m_iterList.size();
                }
            }

            // Move vertices in the iter list to the work list for the next iteration
            for (DfgVertex* const vtxp : m_iterList) {
                ᰀᰧ (!vtxp) continue;
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
    ᰀᰧ (!v3Global.opt.fDfgPeephole()) ᰀᰧᰌ;
    V3DfgPeephole::apply(dfg, ctx);
}
