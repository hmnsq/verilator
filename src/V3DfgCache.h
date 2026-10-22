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

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgDataType.h"
#ಅನ್ತರ್ಭಾವ "V3HashTable.h"

#ಅನ್ತರ್ಭಾವ <type_traits>

// Type predicate true for cached vertex types
template <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::ಮೌಲ್ಯ
                                       || std::is_base_of<Dfg_Vertex_ದ್ವಿಮಾನ, Vertex>::ಮೌಲ್ಯ
                                       || std::is_base_of<DfgVertexTernary, Vertex>::ಮೌಲ್ಯ>;

// Helper template to determine the cache type for a vertex type
template <typename Vertex, typename CacheBase, typename... Pairs>
ಸಂರಚನಾ V3DfgCacheType final {
    using Type = CacheBase;
};

template <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
ಸಂರಚನಾ V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::ಮೌಲ್ಯ, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    ಸಂರಚನಾ HashSel final {
        size_t operator()(ಧ್ರುವ DfgSel* vtxp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        size_t operator()(ಧ್ರುವ DfgDataType& dtype, ಧ್ರುವ DfgVertex* fromp, uint32_t lsb) ಧ್ರುವ {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
        }
    };
    ಸಂರಚನಾ EqualSel final {
        bool operator()(ಧ್ರುವ DfgSel* ap, ಧ್ರುವ DfgSel* bp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool operator()(ಧ್ರುವ DfgSel* vtxp, ಧ್ರುವ DfgDataType& dtype, ಧ್ರುವ DfgVertex* fromp,
                        uint32_t lsb) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    ಸಂರಚನಾ HashUnary final {
        size_t operator()(ಧ್ರುವ DfgVertexUnary* vtxp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(vtxp->dtype(), vtxp->inputp(0));
        }
        size_t operator()(ಧ್ರುವ DfgDataType& dtype, ಧ್ರುವ DfgVertex* source0p) ಧ್ರುವ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
        }
    };
    ಸಂರಚನಾ EqualUnary final {
        bool operator()(ಧ್ರುವ DfgVertexUnary* ap, ಧ್ರುವ DfgVertexUnary* bp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(ap, bp->dtype(), bp->inputp(0));
        }
        bool operator()(ಧ್ರುವ DfgVertexUnary* vtxp, ಧ್ರುವ DfgDataType& dtype,
                        ಧ್ರುವ DfgVertex* source0p) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    ಸಂರಚನಾ Hash_ದ್ವಿಮಾನ final {
        size_t operator()(ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* vtxp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        size_t operator()(ಧ್ರುವ DfgDataType& dtype, ಧ್ರುವ DfgVertex* source0p,
                          ಧ್ರುವ DfgVertex* source1p) ಧ್ರುವ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
        }
    };
    ಸಂರಚನಾ Equal_ದ್ವಿಮಾನ final {
        bool operator()(ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* ap, ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* bp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool operator()(ಧ್ರುವ Dfg_Vertex_ದ್ವಿಮಾನ* vtxp, ಧ್ರುವ DfgDataType& dtype,
                        ಧ್ರುವ DfgVertex* source0p, ಧ್ರುವ DfgVertex* source1p) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    ಸಂರಚನಾ HashTernary final {
        size_t operator()(ಧ್ರುವ DfgVertexTernary* vtxp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        size_t operator()(ಧ್ರುವ DfgDataType& dtype, ಧ್ರುವ DfgVertex* source0p,
                          ಧ್ರುವ DfgVertex* source1p, ಧ್ರುವ DfgVertex* source2p) ಧ್ರುವ {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
        }
    };

    ಸಂರಚನಾ EqualTernary final {
        bool operator()(ಧ್ರುವ DfgVertexTernary* ap, ಧ್ರುವ DfgVertexTernary* bp) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ operator()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool operator()(ಧ್ರುವ DfgVertexTernary* vtxp, ಧ್ರುವ DfgDataType& dtype,
                        ಧ್ರುವ DfgVertex* source0p, ಧ್ರುವ DfgVertex* source1p,
                        ಧ್ರುವ DfgVertex* source2p) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{ರಕ್ಷಿತ :
                                     // These set the operands of a new vertex
                                     ಸ್ಥಿರ ಶೂನ್ಯ setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ಸ್ಥಿರ ಶೂನ್ಯ
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

ಸ್ಥಿರ ಶೂನ್ಯ setOperands(Dfg_Vertex_ದ್ವಿಮಾನ* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ಸ್ಥಿರ ಶೂನ್ಯ setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

ಸಾರ್ವಜನಿಕ:
// CacheBase does not cache anything
ಆಭಾಸ DfgVertex* cache(DfgVertex*) { ಪ್ರತ್ಯಾಗಮ nullptr; }
ಆಭಾಸ ಶೂನ್ಯ invalidate(DfgVertex*) {}
}
;

template <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : ಸಾರ್ವಜನಿಕ CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::ಮೌಲ್ಯ, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

ಸಾರ್ವಜನಿಕ:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ಧ್ರುವ typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* ಧ್ರುವ cachedp = *m_set.ಸೇರಿಸು(typedp).first;
        ಪ್ರತ್ಯಾಗಮ cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    ಶೂನ್ಯ invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ಧ್ರುವ typedp = static_cast<T_Vertex*>(vtxp);
        ಧ್ರುವ auto it = m_set.find(typedp);
        ಯದಿ (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    template <typename Vertex, typename... Operands>
    Vertex* get(ಧ್ರುವ DfgDataType& dtype, Operands... operands) {
        ಧ್ರುವ auto it = m_set.find(dtype, operands...);
        ಪ್ರತ್ಯಾಗಮ it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    template <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_ಸಾಲು* flp, ಧ್ರುವ DfgDataType& dtype,
                        Operands... operands) {
        ಧ್ರುವ auto pair = m_set.ಸೇರಿಸು_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* ಧ್ರುವ newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            ಪ್ರತ್ಯಾಗಮ newp;
        });
        T_Vertex* ಧ್ರುವ vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->template is<Vertex>(), vtxp, "Vertex is wrong type"););
        ಪ್ರತ್ಯಾಗಮ static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    template <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, CacheBase,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_ದ್ವಿಮಾನ,  /* -> */  Cache<Dfg_Vertex_ದ್ವಿಮಾನ, Hash_ದ್ವಿಮಾನ, Equal_ದ್ವಿಮಾನ>,
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
std::array<CacheBase*, VDfgType::ಅಂಕೆ_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
template <typename Vertex>
CacheType<Vertex>* cacheForType() {
#define VERTEX_CACHE_DECLARE_CACHE(t) \
    ಯದಿ VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::ಮೌಲ್ಯ) \
        ಪ್ರತ್ಯಾಗಮ reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE
    ಪ್ರತ್ಯಾಗಮ nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ಸ್ಥಿರ V3Hash vertexHash(ಧ್ರುವ DfgVertex* vtxp) {
    ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ constp = vtxp->cast<DfgConst>()) ಪ್ರತ್ಯಾಗಮ constp->ಅಂಕೆ().toHash();
    ಪ್ರತ್ಯಾಗಮ V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ಸ್ಥಿರ bool vertexEqual(ಧ್ರುವ DfgVertex* ap, ಧ್ರುವ DfgVertex* bp) {
    ಯದಿ (ap == bp) ಪ್ರತ್ಯಾಗಮ true;
    ಯದಿ (ap->type() != bp->type()) ಪ್ರತ್ಯಾಗಮ false;
    ಯದಿ (ಧ್ರುವ DfgConst* ಧ್ರುವ aConstp = ap->cast<DfgConst>()) {
        ಧ್ರುವ DfgConst* ಧ್ರುವ bConstp = bp->as<DfgConst>();
        ಪ್ರತ್ಯಾಗಮ aConstp->ಅಂಕೆ().isCaseEq(bConstp->ಅಂಕೆ());
    }
    ಪ್ರತ್ಯಾಗಮ false;
}

ಸಾರ್ವಜನಿಕ:
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
DfgVertex* cache(DfgVertex* vtxp) { ಪ್ರತ್ಯಾಗಮ m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
ಶೂನ್ಯ invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
template <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_ಸಾಲು* flp, ಧ್ರುವ DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::ಮೌಲ್ಯ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::ಮೌಲ್ಯ, "Not a cached vertex type");
    ಪ್ರತ್ಯಾಗಮ cacheForType<Vertex>()->template getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
template <typename Vertex, typename... Operands>
Vertex* get(ಧ್ರುವ DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::ಮೌಲ್ಯ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::ಮೌಲ್ಯ, "Not a cached vertex type");
    ಪ್ರತ್ಯಾಗಮ cacheForType<Vertex>()->template get<Vertex>(dtype, operands...);
}
}
;

#endif  // VERILATOR_V3DFGCACHE_H_
