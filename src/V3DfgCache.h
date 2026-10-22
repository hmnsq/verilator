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

#यदि_अपरिभाषितम् VERILATOR_V3DFGCACHE_H_
#परिभाषय VERILATOR_V3DFGCACHE_H_

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgDataType.h"
#अन्तर्भाव "V3HashTable.h"

#अन्तर्भाव <type_traits>

// Type predicate true for cached vertex types
प्रतिरूपम् <typename Vertex>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_आधारः_सम्बन्धि<DfgVertexUnary, Vertex>::मानम्
                                       || std::is_आधारः_सम्बन्धि<Dfg_Vertex_द्विमान, Vertex>::मानम्
                                       || std::is_आधारः_सम्बन्धि<DfgVertexTernary, Vertex>::मानम्>;

// Helper template to determine the cache type for a vertex type
प्रतिरूपम् <typename Vertex, typename Cache_आधारः, typename... युग्मानि>
संरचना V3DfgCacheType final {
    using Type = Cache_आधारः;
};

प्रतिरूपम् <typename Vertex, typename Cache_आधारः, typename Vertex_आधारः, typename Cache,
          typename... युग्मानि>
संरचना V3DfgCacheType<Vertex, Cache_आधारः, Vertex_आधारः, Cache, युग्मानि...> final {
    using Type = std::conditional_t<std::is_आधारः_सम्बन्धि<Vertex_आधारः, Vertex>::मानम्, Cache,
                                    typename V3DfgCacheType<Vertex, Cache_आधारः, युग्मानि...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    संरचना HashSel final {
        आकार_t संक्रियकः()(ध्रुव DfgSel* vtxp) ध्रुव {
            प्रत्यागम संक्रियकः()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        आकार_t संक्रियकः()(ध्रुव Dfg_दत्त_Type& dtype, ध्रुव DfgVertex* fromp, uint32_t lsb) ध्रुव {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += vertexHash(fromp);
            hash += lsb;
            प्रत्यागम hash.मानम्();
        }
    };
    संरचना EqualSel final {
        bool संक्रियकः()(ध्रुव DfgSel* ap, ध्रुव DfgSel* bp) ध्रुव {
            प्रत्यागम संक्रियकः()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool संक्रियकः()(ध्रुव DfgSel* vtxp, ध्रुव Dfg_दत्त_Type& dtype, ध्रुव DfgVertex* fromp,
                        uint32_t lsb) ध्रुव {
            प्रत्यागम vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && vertexEqual(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    संरचना HashUnary final {
        आकार_t संक्रियकः()(ध्रुव DfgVertexUnary* vtxp) ध्रुव {
            प्रत्यागम संक्रियकः()(vtxp->dtype(), vtxp->inputp(0));
        }
        आकार_t संक्रियकः()(ध्रुव Dfg_दत्त_Type& dtype, ध्रुव DfgVertex* source0p) ध्रुव {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            प्रत्यागम hash.मानम्();
        }
    };
    संरचना EqualUnary final {
        bool संक्रियकः()(ध्रुव DfgVertexUnary* ap, ध्रुव DfgVertexUnary* bp) ध्रुव {
            प्रत्यागम संक्रियकः()(ap, bp->dtype(), bp->inputp(0));
        }
        bool संक्रियकः()(ध्रुव DfgVertexUnary* vtxp, ध्रुव Dfg_दत्त_Type& dtype,
                        ध्रुव DfgVertex* source0p) ध्रुव {
            प्रत्यागम vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    संरचना Hash_द्विमान final {
        आकार_t संक्रियकः()(ध्रुव Dfg_Vertex_द्विमान* vtxp) ध्रुव {
            प्रत्यागम संक्रियकः()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        आकार_t संक्रियकः()(ध्रुव Dfg_दत्त_Type& dtype, ध्रुव DfgVertex* source0p,
                          ध्रुव DfgVertex* source1p) ध्रुव {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            प्रत्यागम hash.मानम्();
        }
    };
    संरचना Equal_द्विमान final {
        bool संक्रियकः()(ध्रुव Dfg_Vertex_द्विमान* ap, ध्रुव Dfg_Vertex_द्विमान* bp) ध्रुव {
            प्रत्यागम संक्रियकः()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool संक्रियकः()(ध्रुव Dfg_Vertex_द्विमान* vtxp, ध्रुव Dfg_दत्त_Type& dtype,
                        ध्रुव DfgVertex* source0p, ध्रुव DfgVertex* source1p) ध्रुव {
            प्रत्यागम vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    संरचना HashTernary final {
        आकार_t संक्रियकः()(ध्रुव DfgVertexTernary* vtxp) ध्रुव {
            प्रत्यागम संक्रियकः()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        आकार_t संक्रियकः()(ध्रुव Dfg_दत्त_Type& dtype, ध्रुव DfgVertex* source0p,
                          ध्रुव DfgVertex* source1p, ध्रुव DfgVertex* source2p) ध्रुव {
            V3Hash hash = dtype.hash();
            hash += vertexHash(source0p);
            hash += vertexHash(source1p);
            hash += vertexHash(source2p);
            प्रत्यागम hash.मानम्();
        }
    };

    संरचना EqualTernary final {
        bool संक्रियकः()(ध्रुव DfgVertexTernary* ap, ध्रुव DfgVertexTernary* bp) ध्रुव {
            प्रत्यागम संक्रियकः()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool संक्रियकः()(ध्रुव DfgVertexTernary* vtxp, ध्रुव Dfg_दत्त_Type& dtype,
                        ध्रुव DfgVertex* source0p, ध्रुव DfgVertex* source1p,
                        ध्रुव DfgVertex* source2p) ध्रुव {
            प्रत्यागम vtxp->dtype() == dtype && vertexEqual(vtxp->inputp(0), source0p)
                   && vertexEqual(vtxp->inputp(1), source1p)
                   && vertexEqual(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class Cache_आधारः VL_NOT_FINAL{रक्षित :
                                     // These set the operands of a new vertex
                                     स्थिर शून्य समुच्चयः_Operands(DfgSel * vtxp, DfgVertex* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

स्थिर शून्य
समुच्चयः_Operands(DfgVertexUnary* vtxp, DfgVertex* src0p) {  //
    vtxp->inputp(0, src0p);
}

स्थिर शून्य समुच्चयः_Operands(Dfg_Vertex_द्विमान* vtxp, DfgVertex* src0p, DfgVertex* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

स्थिर शून्य समुच्चयः_Operands(DfgVertexTernary* vtxp, DfgVertex* src0p, DfgVertex* src1p,
                        DfgVertex* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

सार्वजनिक:
// CacheBase does not cache anything
आभास DfgVertex* cache(DfgVertex*) { प्रत्यागम nullptr; }
आभास शून्य invalidate(DfgVertex*) {}
}
;

प्रतिरूपम् <typename T_Vertex, typename T_Hash, typename T_Equal>
class Cache final : सार्वजनिक Cache_आधारः {
    static_assert(std::is_आधारः_सम्बन्धि<DfgVertex, T_Vertex>::मानम्, "T_Vertex must be a DfgVertex");

    // STATE
    V3_Hash_समुच्चयः<T_Vertex*, T_Hash, T_Equal> m_समुच्चयः;

सार्वजनिक:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    DfgVertex* cache(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ध्रुव typedp = static_cast<T_Vertex*>(vtxp);
        T_Vertex* ध्रुव cachedp = *m_समुच्चयः.निवेशय(typedp).first;
        प्रत्यागम cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    शून्य invalidate(DfgVertex* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_Vertex>(), vtxp, "Vertex is wrong type"););
        T_Vertex* ध्रुव typedp = static_cast<T_Vertex*>(vtxp);
        ध्रुव स्वचालित it = m_समुच्चयः.find(typedp);
        यदि (it != m_समुच्चयः.अन्तः_2() && *it == typedp) m_समुच्चयः.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    प्रतिरूपम् <typename Vertex, typename... Operands>
    Vertex* get(ध्रुव Dfg_दत्त_Type& dtype, Operands... operands) {
        ध्रुव स्वचालित it = m_समुच्चयः.find(dtype, operands...);
        प्रत्यागम it != m_समुच्चयः.अन्तः_2() ? static_cast<Vertex*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    प्रतिरूपम् <typename Vertex, typename... Operands>
    Vertex* getOrCreate(DfgGraph& dfg, File_पङ्क्तिः* flp, ध्रुव Dfg_दत्त_Type& dtype,
                        Operands... operands) {
        ध्रुव स्वचालित pair = m_समुच्चयः.निवेशय_Lazy(dtype, operands..., [&]() -> T_Vertex* {
            Vertex* ध्रुव newp = new Vertex{dfg, flp, dtype};
            समुच्चयः_Operands(newp, operands...);
            प्रत्यागम newp;
        });
        T_Vertex* ध्रुव vtxp = *pair.first;
        UDEBUGONLY(UASSERT_OBJ(vtxp->प्रतिरूपम् is<Vertex>(), vtxp, "Vertex is wrong type"););
        प्रत्यागम static_cast<Vertex*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    प्रतिरूपम् <typename Vertex>
    using CacheType = typename V3DfgCacheType<Vertex, Cache_आधारः,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        DfgVertexUnary,   /* -> */  Cache<DfgVertexUnary, HashUnary, EqualUnary>,
        Dfg_Vertex_द्विमान,  /* -> */  Cache<Dfg_Vertex_द्विमान, Hash_द्विमान, Equal_द्विमान>,
        DfgVertexTernary, /* -> */  Cache<DfgVertexTernary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
DfgGraph& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#परिभाषय VERTEX_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#अपरिभाषय VERTEX_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<Cache_आधारः*, VDfgType::अङ्कः_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
प्रतिरूपम् <typename Vertex>
CacheType<Vertex>* cacheForType() {
#परिभाषय VERTEX_CACHE_DECLARE_CACHE(t) \
    यदि VL_CONSTEXPR_CXX17 (std::is_same<Vertex, t>::मानम्) \
        प्रत्यागम reinterpret_cast<CacheType<Vertex>*>(&m_cache##t);
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE)
#अपरिभाषय VERTEX_CACHE_DECLARE_CACHE
    प्रत्यागम nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
स्थिर V3Hash vertexHash(ध्रुव DfgVertex* vtxp) {
    यदि (ध्रुव DfgConst* ध्रुव constp = vtxp->cast<DfgConst>()) प्रत्यागम constp->अङ्कः().गन्तव्यम्_Hash();
    प्रत्यागम V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
स्थिर bool vertexEqual(ध्रुव DfgVertex* ap, ध्रुव DfgVertex* bp) {
    यदि (ap == bp) प्रत्यागम true;
    यदि (ap->type() != bp->type()) प्रत्यागम false;
    यदि (ध्रुव DfgConst* ध्रुव aConstp = ap->cast<DfgConst>()) {
        ध्रुव DfgConst* ध्रुव bConstp = bp->as<DfgConst>();
        प्रत्यागम aConstp->अङ्कः().isCaseEq(bConstp->अङ्कः());
    }
    प्रत्यागम false;
}

सार्वजनिक:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(DfgGraph& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#परिभाषय VERTEX_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_VERTEX_TYPE(VERTEX_CACHE_DECLARE_CACHE_PTR)
#अपरिभाषय VERTEX_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
DfgVertex* cache(DfgVertex* vtxp) { प्रत्यागम m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
शून्य invalidate(DfgVertex* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
प्रतिरूपम् <typename Vertex, typename... Operands>
Vertex* getOrCreate(File_पङ्क्तिः* flp, ध्रुव Dfg_दत्त_Type& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::मानम्, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::मानम्, "Not a cached vertex type");
    प्रत्यागम cacheForType<Vertex>()->प्रतिरूपम् getOrCreate<Vertex>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
प्रतिरूपम् <typename Vertex, typename... Operands>
Vertex* get(ध्रुव Dfg_दत्त_Type& dtype, Operands... operands) {
    static_assert(std::is_final<Vertex>::मानम्, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<Vertex>::मानम्, "Not a cached vertex type");
    प्रत्यागम cacheForType<Vertex>()->प्रतिरूपम् get<Vertex>(dtype, operands...);
}
}
;

#यदि_अन्तः  // VERILATOR_V3DFGCACHE_H_
