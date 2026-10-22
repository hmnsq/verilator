// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dfg vertex cache to find existing vertices
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
// A cache for DfgGraph, to find existing vertices with identical inputs.
//
// Beware that if you use this data-structure, you must invalidate the
// cache any time you change the inputs of an existing vertex, otherwise
// you will have a very bad day.
//
//*************************************************************************

#ifndef VERILATOR_V3DFGCACHE_H_
#define VERILATOR_V3DFGCACHE_H_

#ضمن "verilatedos.h"

#ضمن "V3Dfg.h"
#ضمن "V3DfgDataType.h"
#ضمن "V3HashTable.h"

#ضمن <type_traits>

// Type predicate true for cached vertex types
template <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::درج
                                       || std::is_base_of<Dfg_Vertex_نبيو, Vertex>::درج
                                       || std::is_base_of<DfgVertexTernary, Vertex>::درج>;

// Helper template to determine the cache type for a vertex type
template <typename Vertex, typename CacheBase, typename... Pairs>
بنية V3DfgCacheType final {
    using Type = CacheBase;
};

template <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
بنية V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::درج, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    بنية HashSel final {
        size_t operator()(ثابت DfgSel* vtxp) ثابت {
            أرجع operator()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        size_t operator()(ثابت DfgDataType& dtype, ثابت DfgVertex* fromp, uint32_t lsb) ثابت {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            أرجع hash.درج();
        }
    };
    بنية EqualSel final {
        bool operator()(ثابت DfgSel* ap, ثابت DfgSel* bp) ثابت {
            أرجع operator()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool operator()(ثابت DfgSel* vtxp, ثابت DfgDataType& dtype, ثابت DfgVertex* fromp,
                        uint32_t lsb) ثابت {
            أرجع vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    بنية HashUnary final {
        size_t operator()(ثابت DfgVertexUnary* vtxp) ثابت {
            أرجع operator()(vtxp->dtype(), vtxp->inputp(0));
        }
        size_t operator()(ثابت DfgDataType& dtype, ثابت DfgVertex* source0p) ثابت {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            أرجع hash.درج();
        }
    };
    بنية EqualUnary final {
        bool operator()(ثابت DfgVertexUnary* ap, ثابت DfgVertexUnary* bp) ثابت {
            أرجع operator()(ap, bp->dtype(), bp->inputp(0));
        }
        bool operator()(ثابت DfgVertexUnary* vtxp, ثابت DfgDataType& dtype,
                        ثابت DfgVertex* source0p) ثابت {
            أرجع vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    بنية Hash_نبيو final {
        size_t operator()(ثابت Dfg_Vertex_نبيو* vtxp) ثابت {
            أرجع operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        size_t operator()(ثابت DfgDataType& dtype, ثابت DfgVertex* source0p,
                          ثابت DfgVertex* source1p) ثابت {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            أرجع hash.درج();
        }
    };
    بنية Equal_نبيو final {
        bool operator()(ثابت Dfg_Vertex_نبيو* ap, ثابت Dfg_Vertex_نبيو* bp) ثابت {
            أرجع operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool operator()(ثابت Dfg_Vertex_نبيو* vtxp, ثابت DfgDataType& dtype,
                        ثابت DfgVertex* source0p, ثابت DfgVertex* source1p) ثابت {
            أرجع vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    بنية HashTernary final {
        size_t operator()(ثابت DfgVertexTernary* vtxp) ثابت {
            أرجع operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        size_t operator()(ثابت DfgDataType& dtype, ثابت DfgVertex* source0p,
                          ثابت DfgVertex* source1p, ثابت DfgVertex* source2p) ثابت {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            أرجع hash.درج();
        }
    };

    بنية EqualTernary final {
        bool operator()(ثابت DfgVertexTernary* ap, ثابت DfgVertexTernary* bp) ثابت {
            أرجع operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool operator()(ثابت DfgVertexTernary* vtxp, ثابت DfgDataType& dtype,
                        ثابت DfgVertex* source0p, ثابت DfgVertex* source1p,
                        ثابت DfgVertex* source2p) ثابت {
            أرجع vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{محمي :
                                     // These set the operands of a new vertex
                                     ساكن فراغ setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ساكن فراغ
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

ساكن فراغ setOperands(Dfg_Vertex_نبيو* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ساكن فراغ setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

عام:
// CacheBase does not cache anything
virtual DfgVertex* cache(DfgVertex*) { أرجع nullptr; }
virtual فراغ invalidate(DfgVertex*) {}
}
;

template <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : عام CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::درج, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

عام:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ثابت typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* ثابت cachedp = *m_set.سك(typedp).first;
        أرجع cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    فراغ invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ثابت typedp = static_cast<T_Vertex*>(vtxp);
        ثابت auto it = m_set.find(typedp);
        إذا (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    template <typename Vertex, typename... Operands>
    Vertex* get(ثابت DfgDataType& dtype, Operands... operands) {
        ثابت auto it = m_set.find(dtype, operands...);
        أرجع it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_لي* flp, ثابت DfgDataType& dtype,
                        Operands... operands) {
        ثابت auto pair = m_set.سك_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* ثابت newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            أرجع newp;
        });
        T_Vertex* ثابت vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->template is<Vertex>(), vtxp, "Vertex is wrong type"););
        أرجع static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    template <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, CacheBase,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_نبيو,  /* -> */  Cache<Dfg_Vertex_نبيو, Hash_نبيو, Equal_نبيو>,
        DfgVertexTernary, /* -> */  Cache<DfgVertexTernary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
DfgGraph& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#define VERTEX_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<CacheBase*, VDfgType::لمب_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
template <typename Vertex>
CacheType<Vertex>* cacheForType() {
#define VERTEX_CACHE_DECLARE_CACHE(t) \
    إذا VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::درج) \
        أرجع reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE
    أرجع nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ساكن V3Hash vertexHash(ثابت DfgVertex* vtxp) {
    إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->لمب().toHash();
    أرجع V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ساكن bool vertexEqual(ثابت DfgVertex* ap, ثابت DfgVertex* bp) {
    إذا (ap == bp) أرجع true;
    إذا (ap->type() != bp->type()) أرجع false;
    إذا (ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
        ثابت DfgConst* ثابت bConstp = bp->as<DfgConst>();
        أرجع aConstp->لمب().isCaseEq(bConstp->لمب());
    }
    أرجع false;
}

عام:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(DfgGraph& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#define VERTEX_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE_PTR)
#undef VERTEX_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
DfgVertex* cache(DfgVertex* vtxp) { أرجع m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
فراغ invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
template <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_لي* flp, ثابت DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::درج, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::درج, "Not a cached vertex type");
    أرجع cacheForType<Vertex>()->template getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
template <typename Vertex, typename... Operands>
Vertex* get(ثابت DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::درج, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::درج, "Not a cached vertex type");
    أرجع cacheForType<Vertex>()->template get<Vertex>(dtype, operands...);
}
}
;

#endif  // VERILATOR_V3DFGCACHE_H_
