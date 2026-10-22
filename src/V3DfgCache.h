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

#अगर_अघोषित VERILATOR_V3DFGCACHE_H_
#घोषणा VERILATOR_V3DFGCACHE_H_

#समावेश "verilatedos.h"

#समावेश "V3Dfg.h"
#समावेश "V3DfgDataType.h"
#समावेश "V3HashTable.h"

#समावेश <type_traits>

// Type predicate true for cached vertex types
ढाँचा <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::मूल्य
                                       || std::is_base_of<Dfg_Vertex_द्विआधारी, Vertex>::मूल्य
                                       || std::is_base_of<DfgVertexTernary, Vertex>::मूल्य>;

// Helper template to determine the cache type for a vertex type
ढाँचा <typename Vertex, typename CacheBase, typename... Pairs>
काष्ठा V3DfgCacheType final {
    using Type = CacheBase;
};

ढाँचा <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
काष्ठा V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::मूल्य, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    काष्ठा HashSel final {
        माप_प्रकार चालक()(स्थिर DfgSel* vtxp) स्थिर {
            वापस चालक()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        माप_प्रकार चालक()(स्थिर Dfg_तथ्य_Type& dtype, स्थिर DfgVertex* fromp, uint32_t lsb) स्थिर {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            वापस hash.मूल्य();
        }
    };
    काष्ठा EqualSel final {
        bool चालक()(स्थिर DfgSel* ap, स्थिर DfgSel* bp) स्थिर {
            वापस चालक()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool चालक()(स्थिर DfgSel* vtxp, स्थिर Dfg_तथ्य_Type& dtype, स्थिर DfgVertex* fromp,
                        uint32_t lsb) स्थिर {
            वापस vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    काष्ठा HashUnary final {
        माप_प्रकार चालक()(स्थिर DfgVertexUnary* vtxp) स्थिर {
            वापस चालक()(vtxp->dtype(), vtxp->inputp(0));
        }
        माप_प्रकार चालक()(स्थिर Dfg_तथ्य_Type& dtype, स्थिर DfgVertex* source0p) स्थिर {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            वापस hash.मूल्य();
        }
    };
    काष्ठा EqualUnary final {
        bool चालक()(स्थिर DfgVertexUnary* ap, स्थिर DfgVertexUnary* bp) स्थिर {
            वापस चालक()(ap, bp->dtype(), bp->inputp(0));
        }
        bool चालक()(स्थिर DfgVertexUnary* vtxp, स्थिर Dfg_तथ्य_Type& dtype,
                        स्थिर DfgVertex* source0p) स्थिर {
            वापस vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    काष्ठा Hash_द्विआधारी final {
        माप_प्रकार चालक()(स्थिर Dfg_Vertex_द्विआधारी* vtxp) स्थिर {
            वापस चालक()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        माप_प्रकार चालक()(स्थिर Dfg_तथ्य_Type& dtype, स्थिर DfgVertex* source0p,
                          स्थिर DfgVertex* source1p) स्थिर {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            वापस hash.मूल्य();
        }
    };
    काष्ठा Equal_द्विआधारी final {
        bool चालक()(स्थिर Dfg_Vertex_द्विआधारी* ap, स्थिर Dfg_Vertex_द्विआधारी* bp) स्थिर {
            वापस चालक()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool चालक()(स्थिर Dfg_Vertex_द्विआधारी* vtxp, स्थिर Dfg_तथ्य_Type& dtype,
                        स्थिर DfgVertex* source0p, स्थिर DfgVertex* source1p) स्थिर {
            वापस vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    काष्ठा HashTernary final {
        माप_प्रकार चालक()(स्थिर DfgVertexTernary* vtxp) स्थिर {
            वापस चालक()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        माप_प्रकार चालक()(स्थिर Dfg_तथ्य_Type& dtype, स्थिर DfgVertex* source0p,
                          स्थिर DfgVertex* source1p, स्थिर DfgVertex* source2p) स्थिर {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            वापस hash.मूल्य();
        }
    };

    काष्ठा EqualTernary final {
        bool चालक()(स्थिर DfgVertexTernary* ap, स्थिर DfgVertexTernary* bp) स्थिर {
            वापस चालक()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool चालक()(स्थिर DfgVertexTernary* vtxp, स्थिर Dfg_तथ्य_Type& dtype,
                        स्थिर DfgVertex* source0p, स्थिर DfgVertex* source1p,
                        स्थिर DfgVertex* source2p) स्थिर {
            वापस vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{रक्षित :
                                     // These set the operands of a new vertex
                                     अटल व्योम setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

अटल व्योम
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

अटल व्योम setOperands(Dfg_Vertex_द्विआधारी* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

अटल व्योम setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

खुला:
// CacheBase does not cache anything
भव DfgVertex* cache(DfgVertex*) { वापस nullptr; }
भव व्योम invalidate(DfgVertex*) {}
}
;

ढाँचा <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : खुला CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::मूल्य, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

खुला:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* स्थिर typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* स्थिर cachedp = *m_set.जोड़ो(typedp).first;
        वापस cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    व्योम invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* स्थिर typedp = static_cast<T_Vertex*>(vtxp);
        स्थिर स्वतः it = m_set.find(typedp);
        अगर (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* get(स्थिर Dfg_तथ्य_Type& dtype, Operands... operands) {
        स्थिर स्वतः it = m_set.find(dtype, operands...);
        वापस it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_पंक्ति* flp, स्थिर Dfg_तथ्य_Type& dtype,
                        Operands... operands) {
        स्थिर स्वतः pair = m_set.जोड़ो_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* स्थिर newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            वापस newp;
        });
        T_Vertex* स्थिर vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->ढाँचा is<Vertex>(), vtxp, "Vertex is wrong type"););
        वापस static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    ढाँचा <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, CacheBase,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_द्विआधारी,  /* -> */  Cache<Dfg_Vertex_द्विआधारी, Hash_द्विआधारी, Equal_द्विआधारी>,
        DfgVertexTernary, /* -> */  Cache<DfgVertexTernary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
DfgGraph& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#घोषणा VERTEX_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#अघोषित VERTEX_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<CacheBase*, VDfgType::अंक_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
ढाँचा <typename Vertex>
CacheType<Vertex>* cacheForType() {
#घोषणा VERTEX_CACHE_DECLARE_CACHE(t) \
    अगर VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::मूल्य) \
        वापस reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#अघोषित VERTEX_CACHE_DECLARE_CACHE
    वापस nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
अटल V3Hash vertexHash(स्थिर DfgVertex* vtxp) {
    अगर (स्थिर DfgConst* स्थिर constp = vtxp->cast<DfgConst>()) वापस constp->अंक().toHash();
    वापस V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
अटल bool vertexEqual(स्थिर DfgVertex* ap, स्थिर DfgVertex* bp) {
    अगर (ap == bp) वापस true;
    अगर (ap->type() != bp->type()) वापस false;
    अगर (स्थिर DfgConst* स्थिर aConstp = ap->cast<DfgConst>()) {
        स्थिर DfgConst* स्थिर bConstp = bp->as<DfgConst>();
        वापस aConstp->अंक().isCaseEq(bConstp->अंक());
    }
    वापस false;
}

खुला:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(DfgGraph& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#घोषणा VERTEX_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE_PTR)
#अघोषित VERTEX_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
DfgVertex* cache(DfgVertex* vtxp) { वापस m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
व्योम invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
ढाँचा <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_पंक्ति* flp, स्थिर Dfg_तथ्य_Type& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::मूल्य, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::मूल्य, "Not a cached vertex type");
    वापस cacheForType<Vertex>()->ढाँचा getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
ढाँचा <typename Vertex, typename... Operands>
Vertex* get(स्थिर Dfg_तथ्य_Type& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::मूल्य, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::मूल्य, "Not a cached vertex type");
    वापस cacheForType<Vertex>()->ढाँचा get<Vertex>(dtype, operands...);
}
}
;

#पूर्ण_अगर  // VERILATOR_V3DFGCACHE_H_
