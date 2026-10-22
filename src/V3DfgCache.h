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

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgDataType.h"
#ଅନ୍ତର୍ଭାଵ "V3HashTable.h"

#ଅନ୍ତର୍ଭାଵ <type_traits>

// Type predicate true for cached vertex types
template <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::ମୂଲ୍ୟ
                                       || std::is_base_of<Dfg_Vertex_ଦ୍ୱିମିକ, Vertex>::ମୂଲ୍ୟ
                                       || std::is_base_of<DfgVertexTernary, Vertex>::ମୂଲ୍ୟ>;

// Helper template to determine the cache type for a vertex type
template <typename Vertex, typename CacheBase, typename... Pairs>
ସଂରଚନା V3DfgCacheType final {
    using Type = CacheBase;
};

template <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
ସଂରଚନା V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::ମୂଲ୍ୟ, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    ସଂରଚନା HashSel final {
        size_t operator()(ଅଚଳ DfgSel* vtxp) ଅଚଳ {
            ଫେରନ୍ତୁ operator()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        size_t operator()(ଅଚଳ DfgDataType& dtype, ଅଚଳ DfgVertex* fromp, uint32_t lsb) ଅଚଳ {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
        }
    };
    ସଂରଚନା EqualSel final {
        bool operator()(ଅଚଳ DfgSel* ap, ଅଚଳ DfgSel* bp) ଅଚଳ {
            ଫେରନ୍ତୁ operator()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool operator()(ଅଚଳ DfgSel* vtxp, ଅଚଳ DfgDataType& dtype, ଅଚଳ DfgVertex* fromp,
                        uint32_t lsb) ଅଚଳ {
            ଫେରନ୍ତୁ vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    ସଂରଚନା HashUnary final {
        size_t operator()(ଅଚଳ DfgVertexUnary* vtxp) ଅଚଳ {
            ଫେରନ୍ତୁ operator()(vtxp->dtype(), vtxp->inputp(0));
        }
        size_t operator()(ଅଚଳ DfgDataType& dtype, ଅଚଳ DfgVertex* source0p) ଅଚଳ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
        }
    };
    ସଂରଚନା EqualUnary final {
        bool operator()(ଅଚଳ DfgVertexUnary* ap, ଅଚଳ DfgVertexUnary* bp) ଅଚଳ {
            ଫେରନ୍ତୁ operator()(ap, bp->dtype(), bp->inputp(0));
        }
        bool operator()(ଅଚଳ DfgVertexUnary* vtxp, ଅଚଳ DfgDataType& dtype,
                        ଅଚଳ DfgVertex* source0p) ଅଚଳ {
            ଫେରନ୍ତୁ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    ସଂରଚନା Hash_ଦ୍ୱିମିକ final {
        size_t operator()(ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* vtxp) ଅଚଳ {
            ଫେରନ୍ତୁ operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        size_t operator()(ଅଚଳ DfgDataType& dtype, ଅଚଳ DfgVertex* source0p,
                          ଅଚଳ DfgVertex* source1p) ଅଚଳ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
        }
    };
    ସଂରଚନା Equal_ଦ୍ୱିମିକ final {
        bool operator()(ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* ap, ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* bp) ଅଚଳ {
            ଫେରନ୍ତୁ operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool operator()(ଅଚଳ Dfg_Vertex_ଦ୍ୱିମିକ* vtxp, ଅଚଳ DfgDataType& dtype,
                        ଅଚଳ DfgVertex* source0p, ଅଚଳ DfgVertex* source1p) ଅଚଳ {
            ଫେରନ୍ତୁ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    ସଂରଚନା HashTernary final {
        size_t operator()(ଅଚଳ DfgVertexTernary* vtxp) ଅଚଳ {
            ଫେରନ୍ତୁ operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        size_t operator()(ଅଚଳ DfgDataType& dtype, ଅଚଳ DfgVertex* source0p,
                          ଅଚଳ DfgVertex* source1p, ଅଚଳ DfgVertex* source2p) ଅଚଳ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
        }
    };

    ସଂରଚନା EqualTernary final {
        bool operator()(ଅଚଳ DfgVertexTernary* ap, ଅଚଳ DfgVertexTernary* bp) ଅଚଳ {
            ଫେରନ୍ତୁ operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool operator()(ଅଚଳ DfgVertexTernary* vtxp, ଅଚଳ DfgDataType& dtype,
                        ଅଚଳ DfgVertex* source0p, ଅଚଳ DfgVertex* source1p,
                        ଅଚଳ DfgVertex* source2p) ଅଚଳ {
            ଫେରନ୍ତୁ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{ସୁରକ୍ଷିତ :
                                     // These set the operands of a new vertex
                                     ସ୍ଥିର ଶୂନ୍ୟ setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ସ୍ଥିର ଶୂନ୍ୟ
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

ସ୍ଥିର ଶୂନ୍ୟ setOperands(Dfg_Vertex_ଦ୍ୱିମିକ* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ସ୍ଥିର ଶୂନ୍ୟ setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

ସର୍ବସାଧାରଣ:
// CacheBase does not cache anything
ଆଭାସୀ DfgVertex* cache(DfgVertex*) { ଫେରନ୍ତୁ nullptr; }
ଆଭାସୀ ଶୂନ୍ୟ invalidate(DfgVertex*) {}
}
;

template <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : ସର୍ବସାଧାରଣ CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::ମୂଲ୍ୟ, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

ସର୍ବସାଧାରଣ:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ଅଚଳ typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* ଅଚଳ cachedp = *m_set.ଭର୍ତ୍ତି(typedp).first;
        ଫେରନ୍ତୁ cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    ଶୂନ୍ୟ invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ଅଚଳ typedp = static_cast<T_Vertex*>(vtxp);
        ଅଚଳ auto it = m_set.find(typedp);
        ଯଦି (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    template <typename Vertex, typename... Operands>
    Vertex* get(ଅଚଳ DfgDataType& dtype, Operands... operands) {
        ଅଚଳ auto it = m_set.find(dtype, operands...);
        ଫେରନ୍ତୁ it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_ଧାଡ଼ି* flp, ଅଚଳ DfgDataType& dtype,
                        Operands... operands) {
        ଅଚଳ auto pair = m_set.ଭର୍ତ୍ତି_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* ଅଚଳ newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            ଫେରନ୍ତୁ newp;
        });
        T_Vertex* ଅଚଳ vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->template is<Vertex>(), vtxp, "Vertex is wrong type"););
        ଫେରନ୍ତୁ static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    template <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, CacheBase,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_ଦ୍ୱିମିକ,  /* -> */  Cache<Dfg_Vertex_ଦ୍ୱିମିକ, Hash_ଦ୍ୱିମିକ, Equal_ଦ୍ୱିମିକ>,
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
std::array<CacheBase*, VDfgType::ଅଙ୍କ_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
template <typename Vertex>
CacheType<Vertex>* cacheForType() {
#define VERTEX_CACHE_DECLARE_CACHE(t) \
    ଯଦି VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::ମୂଲ୍ୟ) \
        ଫେରନ୍ତୁ reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE
    ଫେରନ୍ତୁ nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ସ୍ଥିର V3Hash vertexHash(ଅଚଳ DfgVertex* vtxp) {
    ଯଦି (ଅଚଳ DfgConst* ଅଚଳ constp = vtxp->cast<DfgConst>()) ଫେରନ୍ତୁ constp->ଅଙ୍କ().toHash();
    ଫେରନ୍ତୁ V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ସ୍ଥିର bool vertexEqual(ଅଚଳ DfgVertex* ap, ଅଚଳ DfgVertex* bp) {
    ଯଦି (ap == bp) ଫେରନ୍ତୁ true;
    ଯଦି (ap->type() != bp->type()) ଫେରନ୍ତୁ false;
    ଯଦି (ଅଚଳ DfgConst* ଅଚଳ aConstp = ap->cast<DfgConst>()) {
        ଅଚଳ DfgConst* ଅଚଳ bConstp = bp->as<DfgConst>();
        ଫେରନ୍ତୁ aConstp->ଅଙ୍କ().isCaseEq(bConstp->ଅଙ୍କ());
    }
    ଫେରନ୍ତୁ false;
}

ସର୍ବସାଧାରଣ:
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
DfgVertex* cache(DfgVertex* vtxp) { ଫେରନ୍ତୁ m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
ଶୂନ୍ୟ invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
template <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_ଧାଡ଼ି* flp, ଅଚଳ DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::ମୂଲ୍ୟ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::ମୂଲ୍ୟ, "Not a cached vertex type");
    ଫେରନ୍ତୁ cacheForType<Vertex>()->template getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
template <typename Vertex, typename... Operands>
Vertex* get(ଅଚଳ DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::ମୂଲ୍ୟ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::ମୂଲ୍ୟ, "Not a cached vertex type");
    ଫେରନ୍ତୁ cacheForType<Vertex>()->template get<Vertex>(dtype, operands...);
}
}
;

#endif  // VERILATOR_V3DFGCACHE_H_
