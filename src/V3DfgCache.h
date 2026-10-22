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

#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgDataType.h"
#അന്തര്ഭാവ "V3HashTable.h"

#അന്തര്ഭാവ <type_traits>

// Type predicate true for cached vertex types
template <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::മൂല്യം
                                       || std::is_base_of<Dfg_Vertex_ദ്വിമാന, Vertex>::മൂല്യം
                                       || std::is_base_of<DfgVertexTernary, Vertex>::മൂല്യം>;

// Helper template to determine the cache type for a vertex type
template <typename Vertex, typename CacheBase, typename... Pairs>
ഘടന V3DfgCacheType final {
    using Type = CacheBase;
};

template <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
ഘടന V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::മൂല്യം, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    ഘടന HashSel final {
        size_t operator()(സ്ഥിരം DfgSel* vtxp) സ്ഥിരം {
            തിരികെ operator()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        size_t operator()(സ്ഥിരം DfgDataType& dtype, സ്ഥിരം DfgVertex* fromp, uint32_t lsb) സ്ഥിരം {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            തിരികെ hash.മൂല്യം();
        }
    };
    ഘടന EqualSel final {
        bool operator()(സ്ഥിരം DfgSel* ap, സ്ഥിരം DfgSel* bp) സ്ഥിരം {
            തിരികെ operator()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool operator()(സ്ഥിരം DfgSel* vtxp, സ്ഥിരം DfgDataType& dtype, സ്ഥിരം DfgVertex* fromp,
                        uint32_t lsb) സ്ഥിരം {
            തിരികെ vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    ഘടന HashUnary final {
        size_t operator()(സ്ഥിരം DfgVertexUnary* vtxp) സ്ഥിരം {
            തിരികെ operator()(vtxp->dtype(), vtxp->inputp(0));
        }
        size_t operator()(സ്ഥിരം DfgDataType& dtype, സ്ഥിരം DfgVertex* source0p) സ്ഥിരം {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            തിരികെ hash.മൂല്യം();
        }
    };
    ഘടന EqualUnary final {
        bool operator()(സ്ഥിരം DfgVertexUnary* ap, സ്ഥിരം DfgVertexUnary* bp) സ്ഥിരം {
            തിരികെ operator()(ap, bp->dtype(), bp->inputp(0));
        }
        bool operator()(സ്ഥിരം DfgVertexUnary* vtxp, സ്ഥിരം DfgDataType& dtype,
                        സ്ഥിരം DfgVertex* source0p) സ്ഥിരം {
            തിരികെ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    ഘടന Hash_ദ്വിമാന final {
        size_t operator()(സ്ഥിരം Dfg_Vertex_ദ്വിമാന* vtxp) സ്ഥിരം {
            തിരികെ operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        size_t operator()(സ്ഥിരം DfgDataType& dtype, സ്ഥിരം DfgVertex* source0p,
                          സ്ഥിരം DfgVertex* source1p) സ്ഥിരം {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            തിരികെ hash.മൂല്യം();
        }
    };
    ഘടന Equal_ദ്വിമാന final {
        bool operator()(സ്ഥിരം Dfg_Vertex_ദ്വിമാന* ap, സ്ഥിരം Dfg_Vertex_ദ്വിമാന* bp) സ്ഥിരം {
            തിരികെ operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool operator()(സ്ഥിരം Dfg_Vertex_ദ്വിമാന* vtxp, സ്ഥിരം DfgDataType& dtype,
                        സ്ഥിരം DfgVertex* source0p, സ്ഥിരം DfgVertex* source1p) സ്ഥിരം {
            തിരികെ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    ഘടന HashTernary final {
        size_t operator()(സ്ഥിരം DfgVertexTernary* vtxp) സ്ഥിരം {
            തിരികെ operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        size_t operator()(സ്ഥിരം DfgDataType& dtype, സ്ഥിരം DfgVertex* source0p,
                          സ്ഥിരം DfgVertex* source1p, സ്ഥിരം DfgVertex* source2p) സ്ഥിരം {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            തിരികെ hash.മൂല്യം();
        }
    };

    ഘടന EqualTernary final {
        bool operator()(സ്ഥിരം DfgVertexTernary* ap, സ്ഥിരം DfgVertexTernary* bp) സ്ഥിരം {
            തിരികെ operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool operator()(സ്ഥിരം DfgVertexTernary* vtxp, സ്ഥിരം DfgDataType& dtype,
                        സ്ഥിരം DfgVertex* source0p, സ്ഥിരം DfgVertex* source1p,
                        സ്ഥിരം DfgVertex* source2p) സ്ഥിരം {
            തിരികെ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{സംരക്ഷിത :
                                     // These set the operands of a new vertex
                                     സ്ഥിതം ശൂന്യം setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

സ്ഥിതം ശൂന്യം
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

സ്ഥിതം ശൂന്യം setOperands(Dfg_Vertex_ദ്വിമാന* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

സ്ഥിതം ശൂന്യം setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

പൊതു:
// CacheBase does not cache anything
മായാ DfgVertex* cache(DfgVertex*) { തിരികെ nullptr; }
മായാ ശൂന്യം invalidate(DfgVertex*) {}
}
;

template <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : പൊതു CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::മൂല്യം, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

പൊതു:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* സ്ഥിരം typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* സ്ഥിരം cachedp = *m_set.തിരുകുക(typedp).first;
        തിരികെ cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    ശൂന്യം invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* സ്ഥിരം typedp = static_cast<T_Vertex*>(vtxp);
        സ്ഥിരം auto it = m_set.find(typedp);
        എങ്കിൽ (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    template <typename Vertex, typename... Operands>
    Vertex* get(സ്ഥിരം DfgDataType& dtype, Operands... operands) {
        സ്ഥിരം auto it = m_set.find(dtype, operands...);
        തിരികെ it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_വരി* flp, സ്ഥിരം DfgDataType& dtype,
                        Operands... operands) {
        സ്ഥിരം auto pair = m_set.തിരുകുക_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* സ്ഥിരം newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            തിരികെ newp;
        });
        T_Vertex* സ്ഥിരം vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->template is<Vertex>(), vtxp, "Vertex is wrong type"););
        തിരികെ static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    template <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, CacheBase,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_ദ്വിമാന,  /* -> */  Cache<Dfg_Vertex_ദ്വിമാന, Hash_ദ്വിമാന, Equal_ദ്വിമാന>,
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
std::array<CacheBase*, VDfgType::അക്കം_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
template <typename Vertex>
CacheType<Vertex>* cacheForType() {
#define VERTEX_CACHE_DECLARE_CACHE(t) \
    എങ്കിൽ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::മൂല്യം) \
        തിരികെ reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE
    തിരികെ nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
സ്ഥിതം V3Hash vertexHash(സ്ഥിരം DfgVertex* vtxp) {
    എങ്കിൽ (സ്ഥിരം DfgConst* സ്ഥിരം constp = vtxp->cast<DfgConst>()) തിരികെ constp->അക്കം().toHash();
    തിരികെ V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
സ്ഥിതം bool vertexEqual(സ്ഥിരം DfgVertex* ap, സ്ഥിരം DfgVertex* bp) {
    എങ്കിൽ (ap == bp) തിരികെ true;
    എങ്കിൽ (ap->type() != bp->type()) തിരികെ false;
    എങ്കിൽ (സ്ഥിരം DfgConst* സ്ഥിരം aConstp = ap->cast<DfgConst>()) {
        സ്ഥിരം DfgConst* സ്ഥിരം bConstp = bp->as<DfgConst>();
        തിരികെ aConstp->അക്കം().isCaseEq(bConstp->അക്കം());
    }
    തിരികെ false;
}

പൊതു:
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
DfgVertex* cache(DfgVertex* vtxp) { തിരികെ m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
ശൂന്യം invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
template <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_വരി* flp, സ്ഥിരം DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::മൂല്യം, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::മൂല്യം, "Not a cached vertex type");
    തിരികെ cacheForType<Vertex>()->template getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
template <typename Vertex, typename... Operands>
Vertex* get(സ്ഥിരം DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::മൂല്യം, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::മൂല്യം, "Not a cached vertex type");
    തിരികെ cacheForType<Vertex>()->template get<Vertex>(dtype, operands...);
}
}
;

#endif  // VERILATOR_V3DFGCACHE_H_
