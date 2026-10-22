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

#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgDataType.h"
#અન્તર્ભાવ "V3HashTable.h"

#અન્તર્ભાવ <type_traits>

// Type predicate true for cached vertex types
template <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::કિંમત
                                       || std::is_base_of<Dfg_Vertex_દ્વિઅંકી, Vertex>::કિંમત
                                       || std::is_base_of<DfgVertexTernary, Vertex>::કિંમત>;

// Helper template to determine the cache type for a vertex type
template <typename Vertex, typename CacheBase, typename... Pairs>
સંરચના V3DfgCacheType final {
    using Type = CacheBase;
};

template <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
સંરચના V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::કિંમત, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    સંરચના HashSel final {
        size_t operator()(અચલ DfgSel* vtxp) અચલ {
            પાછા operator()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        size_t operator()(અચલ DfgDataType& dtype, અચલ DfgVertex* fromp, uint32_t lsb) અચલ {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            પાછા hash.કિંમત();
        }
    };
    સંરચના EqualSel final {
        bool operator()(અચલ DfgSel* ap, અચલ DfgSel* bp) અચલ {
            પાછા operator()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool operator()(અચલ DfgSel* vtxp, અચલ DfgDataType& dtype, અચલ DfgVertex* fromp,
                        uint32_t lsb) અચલ {
            પાછા vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    સંરચના HashUnary final {
        size_t operator()(અચલ DfgVertexUnary* vtxp) અચલ {
            પાછા operator()(vtxp->dtype(), vtxp->inputp(0));
        }
        size_t operator()(અચલ DfgDataType& dtype, અચલ DfgVertex* source0p) અચલ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            પાછા hash.કિંમત();
        }
    };
    સંરચના EqualUnary final {
        bool operator()(અચલ DfgVertexUnary* ap, અચલ DfgVertexUnary* bp) અચલ {
            પાછા operator()(ap, bp->dtype(), bp->inputp(0));
        }
        bool operator()(અચલ DfgVertexUnary* vtxp, અચલ DfgDataType& dtype,
                        અચલ DfgVertex* source0p) અચલ {
            પાછા vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    સંરચના Hash_દ્વિઅંકી final {
        size_t operator()(અચલ Dfg_Vertex_દ્વિઅંકી* vtxp) અચલ {
            પાછા operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        size_t operator()(અચલ DfgDataType& dtype, અચલ DfgVertex* source0p,
                          અચલ DfgVertex* source1p) અચલ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            પાછા hash.કિંમત();
        }
    };
    સંરચના Equal_દ્વિઅંકી final {
        bool operator()(અચલ Dfg_Vertex_દ્વિઅંકી* ap, અચલ Dfg_Vertex_દ્વિઅંકી* bp) અચલ {
            પાછા operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool operator()(અચલ Dfg_Vertex_દ્વિઅંકી* vtxp, અચલ DfgDataType& dtype,
                        અચલ DfgVertex* source0p, અચલ DfgVertex* source1p) અચલ {
            પાછા vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    સંરચના HashTernary final {
        size_t operator()(અચલ DfgVertexTernary* vtxp) અચલ {
            પાછા operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        size_t operator()(અચલ DfgDataType& dtype, અચલ DfgVertex* source0p,
                          અચલ DfgVertex* source1p, અચલ DfgVertex* source2p) અચલ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            પાછા hash.કિંમત();
        }
    };

    સંરચના EqualTernary final {
        bool operator()(અચલ DfgVertexTernary* ap, અચલ DfgVertexTernary* bp) અચલ {
            પાછા operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool operator()(અચલ DfgVertexTernary* vtxp, અચલ DfgDataType& dtype,
                        અચલ DfgVertex* source0p, અચલ DfgVertex* source1p,
                        અચલ DfgVertex* source2p) અચલ {
            પાછા vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{સુરક્ષિત :
                                     // These set the operands of a new vertex
                                     સ્થિર શૂન્ય setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

સ્થિર શૂન્ય
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

સ્થિર શૂન્ય setOperands(Dfg_Vertex_દ્વિઅંકી* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

સ્થિર શૂન્ય setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

જાહેર:
// CacheBase does not cache anything
આભાસી DfgVertex* cache(DfgVertex*) { પાછા nullptr; }
આભાસી શૂન્ય invalidate(DfgVertex*) {}
}
;

template <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : જાહેર CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::કિંમત, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

જાહેર:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* અચલ typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* અચલ cachedp = *m_set.ઉમેરો(typedp).first;
        પાછા cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    શૂન્ય invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* અચલ typedp = static_cast<T_Vertex*>(vtxp);
        અચલ auto it = m_set.find(typedp);
        જો (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    template <typename Vertex, typename... Operands>
    Vertex* get(અચલ DfgDataType& dtype, Operands... operands) {
        અચલ auto it = m_set.find(dtype, operands...);
        પાછા it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_લીટી* flp, અચલ DfgDataType& dtype,
                        Operands... operands) {
        અચલ auto pair = m_set.ઉમેરો_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* અચલ newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            પાછા newp;
        });
        T_Vertex* અચલ vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->template is<Vertex>(), vtxp, "Vertex is wrong type"););
        પાછા static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    template <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, CacheBase,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_દ્વિઅંકી,  /* -> */  Cache<Dfg_Vertex_દ્વિઅંકી, Hash_દ્વિઅંકી, Equal_દ્વિઅંકી>,
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
std::array<CacheBase*, VDfgType::અંક_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
template <typename Vertex>
CacheType<Vertex>* cacheForType() {
#define VERTEX_CACHE_DECLARE_CACHE(t) \
    જો VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::કિંમત) \
        પાછા reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE
    પાછા nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
સ્થિર V3Hash vertexHash(અચલ DfgVertex* vtxp) {
    જો (અચલ DfgConst* અચલ constp = vtxp->cast<DfgConst>()) પાછા constp->અંક().toHash();
    પાછા V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
સ્થિર bool vertexEqual(અચલ DfgVertex* ap, અચલ DfgVertex* bp) {
    જો (ap == bp) પાછા true;
    જો (ap->type() != bp->type()) પાછા false;
    જો (અચલ DfgConst* અચલ aConstp = ap->cast<DfgConst>()) {
        અચલ DfgConst* અચલ bConstp = bp->as<DfgConst>();
        પાછા aConstp->અંક().isCaseEq(bConstp->અંક());
    }
    પાછા false;
}

જાહેર:
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
DfgVertex* cache(DfgVertex* vtxp) { પાછા m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
શૂન્ય invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
template <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_લીટી* flp, અચલ DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::કિંમત, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::કિંમત, "Not a cached vertex type");
    પાછા cacheForType<Vertex>()->template getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
template <typename Vertex, typename... Operands>
Vertex* get(અચલ DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::કિંમત, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::કિંમત, "Not a cached vertex type");
    પાછા cacheForType<Vertex>()->template get<Vertex>(dtype, operands...);
}
}
;

#endif  // VERILATOR_V3DFGCACHE_H_
