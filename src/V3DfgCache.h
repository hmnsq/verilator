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

#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgDataType.h"
#অন্তর্ভাব "V3HashTable.h"

#অন্তর্ভাব <type_traits>

// Type predicate true for cached vertex types
template <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::মান
                                       || std::is_base_of<Dfg_Vertex_দ্বিমিক, Vertex>::মান
                                       || std::is_base_of<DfgVertexTernary, Vertex>::মান>;

// Helper template to determine the cache type for a vertex type
template <typename Vertex, typename CacheBase, typename... Pairs>
কাঠামো V3DfgCacheType final {
    using Type = CacheBase;
};

template <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
কাঠামো V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::মান, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    কাঠামো HashSel final {
        আকার_t operator()(ধ্রুবক DfgSel* vtxp) ধ্রুবক {
            ফেরত operator()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        আকার_t operator()(ধ্রুবক Dfg_তথ্য_Type& dtype, ধ্রুবক DfgVertex* fromp, uint32_t lsb) ধ্রুবক {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            ফেরত hash.মান();
        }
    };
    কাঠামো EqualSel final {
        bool operator()(ধ্রুবক DfgSel* ap, ধ্রুবক DfgSel* bp) ধ্রুবক {
            ফেরত operator()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool operator()(ধ্রুবক DfgSel* vtxp, ধ্রুবক Dfg_তথ্য_Type& dtype, ধ্রুবক DfgVertex* fromp,
                        uint32_t lsb) ধ্রুবক {
            ফেরত vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    কাঠামো HashUnary final {
        আকার_t operator()(ধ্রুবক DfgVertexUnary* vtxp) ধ্রুবক {
            ফেরত operator()(vtxp->dtype(), vtxp->inputp(0));
        }
        আকার_t operator()(ধ্রুবক Dfg_তথ্য_Type& dtype, ধ্রুবক DfgVertex* source0p) ধ্রুবক {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            ফেরত hash.মান();
        }
    };
    কাঠামো EqualUnary final {
        bool operator()(ধ্রুবক DfgVertexUnary* ap, ধ্রুবক DfgVertexUnary* bp) ধ্রুবক {
            ফেরত operator()(ap, bp->dtype(), bp->inputp(0));
        }
        bool operator()(ধ্রুবক DfgVertexUnary* vtxp, ধ্রুবক Dfg_তথ্য_Type& dtype,
                        ধ্রুবক DfgVertex* source0p) ধ্রুবক {
            ফেরত vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    কাঠামো Hash_দ্বিমিক final {
        আকার_t operator()(ধ্রুবক Dfg_Vertex_দ্বিমিক* vtxp) ধ্রুবক {
            ফেরত operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        আকার_t operator()(ধ্রুবক Dfg_তথ্য_Type& dtype, ধ্রুবক DfgVertex* source0p,
                          ধ্রুবক DfgVertex* source1p) ধ্রুবক {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            ফেরত hash.মান();
        }
    };
    কাঠামো Equal_দ্বিমিক final {
        bool operator()(ধ্রুবক Dfg_Vertex_দ্বিমিক* ap, ধ্রুবক Dfg_Vertex_দ্বিমিক* bp) ধ্রুবক {
            ফেরত operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool operator()(ধ্রুবক Dfg_Vertex_দ্বিমিক* vtxp, ধ্রুবক Dfg_তথ্য_Type& dtype,
                        ধ্রুবক DfgVertex* source0p, ধ্রুবক DfgVertex* source1p) ধ্রুবক {
            ফেরত vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    কাঠামো HashTernary final {
        আকার_t operator()(ধ্রুবক DfgVertexTernary* vtxp) ধ্রুবক {
            ফেরত operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        আকার_t operator()(ধ্রুবক Dfg_তথ্য_Type& dtype, ধ্রুবক DfgVertex* source0p,
                          ধ্রুবক DfgVertex* source1p, ধ্রুবক DfgVertex* source2p) ধ্রুবক {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            ফেরত hash.মান();
        }
    };

    কাঠামো EqualTernary final {
        bool operator()(ধ্রুবক DfgVertexTernary* ap, ধ্রুবক DfgVertexTernary* bp) ধ্রুবক {
            ফেরত operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool operator()(ধ্রুবক DfgVertexTernary* vtxp, ধ্রুবক Dfg_তথ্য_Type& dtype,
                        ধ্রুবক DfgVertex* source0p, ধ্রুবক DfgVertex* source1p,
                        ধ্রুবক DfgVertex* source2p) ধ্রুবক {
            ফেরত vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{সুরক্ষিত :
                                     // These set the operands of a new vertex
                                     স্থির শূন্য setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

স্থির শূন্য
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

স্থির শূন্য setOperands(Dfg_Vertex_দ্বিমিক* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

স্থির শূন্য setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

সর্বজনীন:
// CacheBase does not cache anything
আভাসী DfgVertex* cache(DfgVertex*) { ফেরত nullptr; }
আভাসী শূন্য invalidate(DfgVertex*) {}
}
;

template <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : সর্বজনীন CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::মান, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

সর্বজনীন:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ধ্রুবক typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* ধ্রুবক cachedp = *m_set.ঢোকাও(typedp).first;
        ফেরত cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    শূন্য invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ধ্রুবক typedp = static_cast<T_Vertex*>(vtxp);
        ধ্রুবক auto it = m_set.find(typedp);
        যদি (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    template <typename Vertex, typename... Operands>
    Vertex* get(ধ্রুবক Dfg_তথ্য_Type& dtype, Operands... operands) {
        ধ্রুবক auto it = m_set.find(dtype, operands...);
        ফেরত it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_সারি* flp, ধ্রুবক Dfg_তথ্য_Type& dtype,
                        Operands... operands) {
        ধ্রুবক auto pair = m_set.ঢোকাও_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* ধ্রুবক newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            ফেরত newp;
        });
        T_Vertex* ধ্রুবক vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->template is<Vertex>(), vtxp, "Vertex is wrong type"););
        ফেরত static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    template <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, CacheBase,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_দ্বিমিক,  /* -> */  Cache<Dfg_Vertex_দ্বিমিক, Hash_দ্বিমিক, Equal_দ্বিমিক>,
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
std::array<CacheBase*, VDfgType::অঙ্ক_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
template <typename Vertex>
CacheType<Vertex>* cacheForType() {
#define VERTEX_CACHE_DECLARE_CACHE(t) \
    যদি VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::মান) \
        ফেরত reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE
    ফেরত nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
স্থির V3Hash vertexHash(ধ্রুবক DfgVertex* vtxp) {
    যদি (ধ্রুবক DfgConst* ধ্রুবক constp = vtxp->cast<DfgConst>()) ফেরত constp->অঙ্ক().toHash();
    ফেরত V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
স্থির bool vertexEqual(ধ্রুবক DfgVertex* ap, ধ্রুবক DfgVertex* bp) {
    যদি (ap == bp) ফেরত true;
    যদি (ap->type() != bp->type()) ফেরত false;
    যদি (ধ্রুবক DfgConst* ধ্রুবক aConstp = ap->cast<DfgConst>()) {
        ধ্রুবক DfgConst* ধ্রুবক bConstp = bp->as<DfgConst>();
        ফেরত aConstp->অঙ্ক().isCaseEq(bConstp->অঙ্ক());
    }
    ফেরত false;
}

সর্বজনীন:
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
DfgVertex* cache(DfgVertex* vtxp) { ফেরত m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
শূন্য invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
template <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_সারি* flp, ধ্রুবক Dfg_তথ্য_Type& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::মান, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::মান, "Not a cached vertex type");
    ফেরত cacheForType<Vertex>()->template getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
template <typename Vertex, typename... Operands>
Vertex* get(ধ্রুবক Dfg_তথ্য_Type& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::মান, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::মান, "Not a cached vertex type");
    ফেরত cacheForType<Vertex>()->template get<Vertex>(dtype, operands...);
}
}
;

#endif  // VERILATOR_V3DFGCACHE_H_
