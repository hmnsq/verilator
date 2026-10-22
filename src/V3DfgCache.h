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

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgDataType.h"
#अन्तर्भाव "V3HashTable.h"

#अन्तर्भाव <type_traits>

// Type predicate true for cached vertex types
ढाँचा <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_base_of<DfgVertexUnary, Vertex>::मान
                                       || std::is_base_of<Dfg_Vertex_द्विआधारी, Vertex>::मान
                                       || std::is_base_of<DfgVertexTernary, Vertex>::मान>;

// Helper template to determine the cache type for a vertex type
ढाँचा <typename Vertex, typename CacheBase, typename... Pairs>
संरचना V3DfgCacheType final {
    using Type = CacheBase;
};

ढाँचा <typename Vertex, typename CacheBase, typename VertexBase, typename Cache,
          typename... Pairs>
संरचना V3DfgCacheType<Vertex, CacheBase, VertexBase, Cache, Pairs...> final {
    using Type = std::conditional_t<std::is_base_of<VertexBase, Vertex>::मान, Cache,
                                    typename V3DfgCacheType<Vertex, CacheBase, Pairs...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    संरचना HashSel final {
        माप_प्रकार चालक()(अचर DfgSel* vtxp) अचर {
            फर्काऊ चालक()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        माप_प्रकार चालक()(अचर DfgDataType& dtype, अचर DfgVertex* fromp, uint32_t lsb) अचर {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            फर्काऊ hash.मान();
        }
    };
    संरचना EqualSel final {
        bool चालक()(अचर DfgSel* ap, अचर DfgSel* bp) अचर {
            फर्काऊ चालक()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool चालक()(अचर DfgSel* vtxp, अचर DfgDataType& dtype, अचर DfgVertex* fromp,
                        uint32_t lsb) अचर {
            फर्काऊ vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    संरचना HashUnary final {
        माप_प्रकार चालक()(अचर DfgVertexUnary* vtxp) अचर {
            फर्काऊ चालक()(vtxp->dtype(), vtxp->inputp(0));
        }
        माप_प्रकार चालक()(अचर DfgDataType& dtype, अचर DfgVertex* source0p) अचर {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            फर्काऊ hash.मान();
        }
    };
    संरचना EqualUnary final {
        bool चालक()(अचर DfgVertexUnary* ap, अचर DfgVertexUnary* bp) अचर {
            फर्काऊ चालक()(ap, bp->dtype(), bp->inputp(0));
        }
        bool चालक()(अचर DfgVertexUnary* vtxp, अचर DfgDataType& dtype,
                        अचर DfgVertex* source0p) अचर {
            फर्काऊ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    संरचना Hash_द्विआधारी final {
        माप_प्रकार चालक()(अचर Dfg_Vertex_द्विआधारी* vtxp) अचर {
            फर्काऊ चालक()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        माप_प्रकार चालक()(अचर DfgDataType& dtype, अचर DfgVertex* source0p,
                          अचर DfgVertex* source1p) अचर {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            फर्काऊ hash.मान();
        }
    };
    संरचना Equal_द्विआधारी final {
        bool चालक()(अचर Dfg_Vertex_द्विआधारी* ap, अचर Dfg_Vertex_द्विआधारी* bp) अचर {
            फर्काऊ चालक()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool चालक()(अचर Dfg_Vertex_द्विआधारी* vtxp, अचर DfgDataType& dtype,
                        अचर DfgVertex* source0p, अचर DfgVertex* source1p) अचर {
            फर्काऊ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    संरचना HashTernary final {
        माप_प्रकार चालक()(अचर DfgVertexTernary* vtxp) अचर {
            फर्काऊ चालक()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        माप_प्रकार चालक()(अचर DfgDataType& dtype, अचर DfgVertex* source0p,
                          अचर DfgVertex* source1p, अचर DfgVertex* source2p) अचर {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            फर्काऊ hash.मान();
        }
    };

    संरचना EqualTernary final {
        bool चालक()(अचर DfgVertexTernary* ap, अचर DfgVertexTernary* bp) अचर {
            फर्काऊ चालक()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool चालक()(अचर DfgVertexTernary* vtxp, अचर DfgDataType& dtype,
                        अचर DfgVertex* source0p, अचर DfgVertex* source1p,
                        अचर DfgVertex* source2p) अचर {
            फर्काऊ vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class CacheBase VL_NOT_FINAL{रक्षित :
                                     // These set the operands of a new vertex
                                     स्थिर शून्य setOperands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

स्थिर शून्य
setOperands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

स्थिर शून्य setOperands(Dfg_Vertex_द्विआधारी* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

स्थिर शून्य setOperands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

सार्वजनिक:
// CacheBase does not cache anything
आभास DfgVertex* cache(DfgVertex*) { फर्काऊ nullptr; }
आभास शून्य invalidate(DfgVertex*) {}
}
;

ढाँचा <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : सार्वजनिक CacheBase {
    static_assert(std::is_base_of<DfgVertex, T_Vertex>::मान, "T_Vertex must be a DfgVertex");

    // STATE
    V3HashSet<T_Vertex*, T_Hash, T_Equal> m_set;

सार्वजनिक:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* अचर typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* अचर cachedp = *m_set.घुसाउ(typedp).first;
        फर्काऊ cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    शून्य invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* अचर typedp = static_cast<T_Vertex*>(vtxp);
        अचर स्वतः it = m_set.find(typedp);
        यदि (it != m_set.end() && *it == typedp) m_set.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* get(अचर DfgDataType& dtype, Operands... operands) {
        अचर स्वतः it = m_set.find(dtype, operands...);
        फर्काऊ it != m_set.end() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    ढाँचा <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_पङ्क्ति* flp, अचर DfgDataType& dtype,
                        Operands... operands) {
        अचर स्वतः pair = m_set.घुसाउ_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* अचर newp = new Vertex{dfg, flp, dtype};
            setOperands(newp, operands...);
            फर्काऊ newp;
        });
        T_Vertex* अचर vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->ढाँचा is<Vertex>(), vtxp, "Vertex is wrong type"););
        फर्काऊ static_cast<Vertex*>(vtxp);
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
#define VERTEX_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<CacheBase*, VDfgType::अङ्क_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
ढाँचा <typename Vertex>
CacheType<Vertex>* cacheForType() {
#define VERTEX_CACHE_DECLARE_CACHE(t) \
    यदि VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::मान) \
        फर्काऊ reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#undef VERTEX_CACHE_DECLARE_CACHE
    फर्काऊ nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
स्थिर V3Hash vertexHash(अचर DfgVertex* vtxp) {
    यदि (अचर DfgConst* अचर constp = vtxp->cast<DfgConst>()) फर्काऊ constp->अङ्क().toHash();
    फर्काऊ V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
स्थिर bool vertexEqual(अचर DfgVertex* ap, अचर DfgVertex* bp) {
    यदि (ap == bp) फर्काऊ true;
    यदि (ap->type() != bp->type()) फर्काऊ false;
    यदि (अचर DfgConst* अचर aConstp = ap->cast<DfgConst>()) {
        अचर DfgConst* अचर bConstp = bp->as<DfgConst>();
        फर्काऊ aConstp->अङ्क().isCaseEq(bConstp->अङ्क());
    }
    फर्काऊ false;
}

सार्वजनिक:
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
DfgVertex* cache(DfgVertex* vtxp) { फर्काऊ m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
शून्य invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
ढाँचा <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_पङ्क्ति* flp, अचर DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::मान, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::मान, "Not a cached vertex type");
    फर्काऊ cacheForType<Vertex>()->ढाँचा getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
ढाँचा <typename Vertex, typename... Operands>
Vertex* get(अचर DfgDataType& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::मान, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::मान, "Not a cached vertex type");
    फर्काऊ cacheForType<Vertex>()->ढाँचा get<Vertex>(dtype, operands...);
}
}
;

#endif  // VERILATOR_V3DFGCACHE_H_
