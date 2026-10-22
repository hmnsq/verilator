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

#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgDataType.h"
#அந்தர்பாவ "V3HashTable.h"

#அந்தர்பாவ <type_traits>

// Type predicate true for cached vertex types
template <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::மதிப்பு
                                       || std::is_base_of<Dfg_Vertex_இருமம், Vertex>::மதிப்பு
                                       || std::is_base_of<DfgVertexTernary, Vertex>::மதிப்பு>;

// Helper template to determine the cache type for a vertex type
template <typename Vertex, typename CacheBase, typename... Pairs>
கட்டமைப்பு V3DfgCacheType final {
    using Type = CacheBase;
};

template <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
கட்டமைப்பு V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::மதிப்பு, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    கட்டமைப்பு HashSel final {
        size_t operator()(மாறிலி DfgSel* vtxp) மாறிலி {
            திரும்பு operator()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        size_t operator()(மாறிலி DfgDataType& dtype, மாறிலி DfgVertex* fromp, uint32_t lsb) மாறிலி {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            திரும்பு hash.மதிப்பு();
        }
    };
    கட்டமைப்பு EqualSel final {
        bool operator()(மாறிலி DfgSel* ap, மாறிலி DfgSel* bp) மாறிலி {
            திரும்பு operator()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool operator()(மாறிலி DfgSel* vtxp, மாறிலி DfgDataType& dtype, மாறிலி DfgVertex* fromp,
                        uint32_t lsb) மாறிலி {
            திரும்பு vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    கட்டமைப்பு HashUnary final {
        size_t operator()(மாறிலி DfgVertexUnary* vtxp) மாறிலி {
            திரும்பு operator()(vtxp->dtype(), vtxp->inputp(0));
        }
        size_t operator()(மாறிலி DfgDataType& dtype, மாறிலி DfgVertex* source0p) மாறிலி {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            திரும்பு hash.மதிப்பு();
        }
    };
    கட்டமைப்பு EqualUnary final {
        bool operator()(மாறிலி DfgVertexUnary* ap, மாறிலி DfgVertexUnary* bp) மாறிலி {
            திரும்பு operator()(ap, bp->dtype(), bp->inputp(0));
        }
        bool operator()(மாறிலி DfgVertexUnary* vtxp, மாறிலி DfgDataType& dtype,
                        மாறிலி DfgVertex* source0p) மாறிலி {
            திரும்பு vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    கட்டமைப்பு Hash_இருமம் final {
        size_t operator()(மாறிலி Dfg_Vertex_இருமம்* vtxp) மாறிலி {
            திரும்பு operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        size_t operator()(மாறிலி DfgDataType& dtype, மாறிலி DfgVertex* source0p,
                          மாறிலி DfgVertex* source1p) மாறிலி {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            திரும்பு hash.மதிப்பு();
        }
    };
    கட்டமைப்பு Equal_இருமம் final {
        bool operator()(மாறிலி Dfg_Vertex_இருமம்* ap, மாறிலி Dfg_Vertex_இருமம்* bp) மாறிலி {
            திரும்பு operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool operator()(மாறிலி Dfg_Vertex_இருமம்* vtxp, மாறிலி DfgDataType& dtype,
                        மாறிலி DfgVertex* source0p, மாறிலி DfgVertex* source1p) மாறிலி {
            திரும்பு vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    கட்டமைப்பு HashTernary final {
        size_t operator()(மாறிலி DfgVertexTernary* vtxp) மாறிலி {
            திரும்பு operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        size_t operator()(மாறிலி DfgDataType& dtype, மாறிலி DfgVertex* source0p,
                          மாறிலி DfgVertex* source1p, மாறிலி DfgVertex* source2p) மாறிலி {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            திரும்பு hash.மதிப்பு();
        }
    };

    கட்டமைப்பு EqualTernary final {
        bool operator()(மாறிலி DfgVertexTernary* ap, மாறிலி DfgVertexTernary* bp) மாறிலி {
            திரும்பு operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool operator()(மாறிலி DfgVertexTernary* vtxp, மாறிலி DfgDataType& dtype,
                        மாறிலி DfgVertex* source0p, மாறிலி DfgVertex* source1p,
                        மாறிலி DfgVertex* source2p) மாறிலி {
            திரும்பு vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{பாதுகாக்கப்பட்ட :
                                     // These set the operands of a new vertex
                                     நிலையான வெறுமை setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

நிலையான வெறுமை
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

நிலையான வெறுமை setOperands(Dfg_Vertex_இருமம்* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

நிலையான வெறுமை setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

பொது:
// CacheBase does not cache anything
மெய்நிகர் DfgVertex* cache(DfgVertex*) { திரும்பு nullptr; }
மெய்நிகர் வெறுமை invalidate(DfgVertex*) {}
}
;

template <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : பொது CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::மதிப்பு, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

பொது:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* மாறிலி typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* மாறிலி cachedp = *m_set.செருகு(typedp).first;
        திரும்பு cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    வெறுமை invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* மாறிலி typedp = static_cast<T_Vertex*>(vtxp);
        மாறிலி auto it = m_set.find(typedp);
        எனில் (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    template <typename Vertex, typename... Operands>
    Vertex* get(மாறிலி DfgDataType& dtype, Operands... operands) {
        மாறிலி auto it = m_set.find(dtype, operands...);
        திரும்பு it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_வரி* flp, மாறிலி DfgDataType& dtype,
                        Operands... operands) {
        மாறிலி auto pair = m_set.செருகு_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* மாறிலி newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            திரும்பு newp;
        });
        T_Vertex* மாறிலி vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->template is<Vertex>(), vtxp, "Vertex is wrong type"););
        திரும்பு static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    template <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, CacheBase,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_இருமம்,  /* -> */  Cache<Dfg_Vertex_இருமம், Hash_இருமம், Equal_இருமம்>,
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
std::array<CacheBase*, VDfgType::இலக்கம்_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
template <typename Vertex>
CacheType<Vertex>* cacheForType() {
#define VERTEX_CACHE_DECLARE_CACHE(t) \
    எனில் VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::மதிப்பு) \
        திரும்பு reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE
    திரும்பு nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
நிலையான V3Hash vertexHash(மாறிலி DfgVertex* vtxp) {
    எனில் (மாறிலி DfgConst* மாறிலி constp = vtxp->cast<DfgConst>()) திரும்பு constp->இலக்கம்().toHash();
    திரும்பு V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
நிலையான bool vertexEqual(மாறிலி DfgVertex* ap, மாறிலி DfgVertex* bp) {
    எனில் (ap == bp) திரும்பு true;
    எனில் (ap->type() != bp->type()) திரும்பு false;
    எனில் (மாறிலி DfgConst* மாறிலி aConstp = ap->cast<DfgConst>()) {
        மாறிலி DfgConst* மாறிலி bConstp = bp->as<DfgConst>();
        திரும்பு aConstp->இலக்கம்().isCaseEq(bConstp->இலக்கம்());
    }
    திரும்பு false;
}

பொது:
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
DfgVertex* cache(DfgVertex* vtxp) { திரும்பு m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
வெறுமை invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
template <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_வரி* flp, மாறிலி DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::மதிப்பு, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::மதிப்பு, "Not a cached vertex type");
    திரும்பு cacheForType<Vertex>()->template getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
template <typename Vertex, typename... Operands>
Vertex* get(மாறிலி DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::மதிப்பு, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::மதிப்பு, "Not a cached vertex type");
    திரும்பு cacheForType<Vertex>()->template get<Vertex>(dtype, operands...);
}
}
;

#endif  // VERILATOR_V3DFGCACHE_H_
