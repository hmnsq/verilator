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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3DFGCACHE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3DFGCACHE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgDataType.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HashTable.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>

// Type predicate true for cached vertex types
ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_Unary, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ
                                       || std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ
                                       || std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼_Ternary, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ>;

// Helper template to determine the cache type for a vertex type
ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2, typename Cache_ਅਧਾਰ, typename... ਜੋੜੇ>
ਬਣਤਰ V3DfgCacheType final {
    using Type = Cache_ਅਧਾਰ;
};

ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2, typename Cache_ਅਧਾਰ, typename ਸ਼ੀਰਸ਼_ਅਧਾਰ, typename Cache,
          typename... ਜੋੜੇ>
ਬਣਤਰ V3DfgCacheType<ਸ਼ੀਰਸ਼_2, Cache_ਅਧਾਰ, ਸ਼ੀਰਸ਼_ਅਧਾਰ, Cache, ਜੋੜੇ...> final {
    using Type = std::conditional_t<std::is_ਅਧਾਰ_ਦਾ<ਸ਼ੀਰਸ਼_ਅਧਾਰ, ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, Cache,
                                    typename V3DfgCacheType<ਸ਼ੀਰਸ਼_2, Cache_ਅਧਾਰ, ਜੋੜੇ...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    ਬਣਤਰ HashSel final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ DfgSel* vtxp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* fromp, uint32_t lsb) ਅਚਲ {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += ਸ਼ੀਰਸ਼_Hash(fromp);
            hash += lsb;
            ਵਾਪਸ hash.ਮੁੱਲ();
        }
    };
    ਬਣਤਰ EqualSel final {
        bool ਸੰਚਾਲਕ()(ਅਚਲ DfgSel* ap, ਅਚਲ DfgSel* bp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool ਸੰਚਾਲਕ()(ਅਚਲ DfgSel* vtxp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* fromp,
                        uint32_t lsb) ਅਚਲ {
            ਵਾਪਸ vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && ਸ਼ੀਰਸ਼_Equal(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    ਬਣਤਰ HashUnary final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Unary* vtxp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(vtxp->dtype(), vtxp->inputp(0));
        }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source0p) ਅਚਲ {
            V3Hash hash = dtype.hash();
            hash += ਸ਼ੀਰਸ਼_Hash(source0p);
            ਵਾਪਸ hash.ਮੁੱਲ();
        }
    };
    ਬਣਤਰ EqualUnary final {
        bool ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Unary* ap, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Unary* bp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(ap, bp->dtype(), bp->inputp(0));
        }
        bool ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Unary* vtxp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype,
                        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source0p) ਅਚਲ {
            ਵਾਪਸ vtxp->dtype() == dtype && ਸ਼ੀਰਸ਼_Equal(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    ਬਣਤਰ Hash_ਦੁਆਧਾਰੀ final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* vtxp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source0p,
                          ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source1p) ਅਚਲ {
            V3Hash hash = dtype.hash();
            hash += ਸ਼ੀਰਸ਼_Hash(source0p);
            hash += ਸ਼ੀਰਸ਼_Hash(source1p);
            ਵਾਪਸ hash.ਮੁੱਲ();
        }
    };
    ਬਣਤਰ Equal_ਦੁਆਧਾਰੀ final {
        bool ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* ap, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* bp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* vtxp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype,
                        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source0p, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source1p) ਅਚਲ {
            ਵਾਪਸ vtxp->dtype() == dtype && ਸ਼ੀਰਸ਼_Equal(vtxp->inputp(0), source0p)
                   && ਸ਼ੀਰਸ਼_Equal(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    ਬਣਤਰ HashTernary final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Ternary* vtxp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source0p,
                          ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source1p, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source2p) ਅਚਲ {
            V3Hash hash = dtype.hash();
            hash += ਸ਼ੀਰਸ਼_Hash(source0p);
            hash += ਸ਼ੀਰਸ਼_Hash(source1p);
            hash += ਸ਼ੀਰਸ਼_Hash(source2p);
            ਵਾਪਸ hash.ਮੁੱਲ();
        }
    };

    ਬਣਤਰ EqualTernary final {
        bool ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Ternary* ap, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Ternary* bp) ਅਚਲ {
            ਵਾਪਸ ਸੰਚਾਲਕ()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool ਸੰਚਾਲਕ()(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Ternary* vtxp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype,
                        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source0p, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source1p,
                        ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* source2p) ਅਚਲ {
            ਵਾਪਸ vtxp->dtype() == dtype && ਸ਼ੀਰਸ਼_Equal(vtxp->inputp(0), source0p)
                   && ਸ਼ੀਰਸ਼_Equal(vtxp->inputp(1), source1p)
                   && ਸ਼ੀਰਸ਼_Equal(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class Cache_ਅਧਾਰ VL_NOT_FINAL{ਸੁਰੱਖਿਅਤ :
                                     // These set the operands of a new vertex
                                     ਸਥਿਰ ਖਾਲੀ ਸੈੱਟ_Operands(DfgSel * vtxp, Dfg_ਸ਼ੀਰਸ਼* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ਸਥਿਰ ਖਾਲੀ
ਸੈੱਟ_Operands(Dfg_ਸ਼ੀਰਸ਼_Unary* vtxp, Dfg_ਸ਼ੀਰਸ਼* src0p) {  //
    vtxp->inputp(0, src0p);
}

ਸਥਿਰ ਖਾਲੀ ਸੈੱਟ_Operands(Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ* vtxp, Dfg_ਸ਼ੀਰਸ਼* src0p, Dfg_ਸ਼ੀਰਸ਼* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ਸਥਿਰ ਖਾਲੀ ਸੈੱਟ_Operands(Dfg_ਸ਼ੀਰਸ਼_Ternary* vtxp, Dfg_ਸ਼ੀਰਸ਼* src0p, Dfg_ਸ਼ੀਰਸ਼* src1p,
                        Dfg_ਸ਼ੀਰਸ਼* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

ਜਨਤਕ:
// CacheBase does not cache anything
ਆਭਾਸੀ Dfg_ਸ਼ੀਰਸ਼* cache(Dfg_ਸ਼ੀਰਸ਼*) { ਵਾਪਸ nullptr; }
ਆਭਾਸੀ ਖਾਲੀ invalidate(Dfg_ਸ਼ੀਰਸ਼*) {}
}
;

ਸਾਂਚਾ <typename T_ਸ਼ੀਰਸ਼, typename T_Hash, typename T_Equal>
class Cache final : ਜਨਤਕ Cache_ਅਧਾਰ {
    static_assert(std::is_ਅਧਾਰ_ਦਾ<Dfg_ਸ਼ੀਰਸ਼, T_ਸ਼ੀਰਸ਼>::ਮੁੱਲ, "T_Vertex must be a DfgVertex");

    // STATE
    V3_Hash_ਸੈੱਟ<T_ਸ਼ੀਰਸ਼*, T_Hash, T_Equal> m_ਸੈੱਟ;

ਜਨਤਕ:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    Dfg_ਸ਼ੀਰਸ਼* cache(Dfg_ਸ਼ੀਰਸ਼* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_ਸ਼ੀਰਸ਼>(), vtxp, "Vertex is wrong type"););
        T_ਸ਼ੀਰਸ਼* ਅਚਲ typedp = static_cast<T_ਸ਼ੀਰਸ਼*>(vtxp);
        T_ਸ਼ੀਰਸ਼* ਅਚਲ cachedp = *m_ਸੈੱਟ.ਪਾਓ(typedp).ਪਹਿਲਾ;
        ਵਾਪਸ cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    ਖਾਲੀ invalidate(Dfg_ਸ਼ੀਰਸ਼* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_ਸ਼ੀਰਸ਼>(), vtxp, "Vertex is wrong type"););
        T_ਸ਼ੀਰਸ਼* ਅਚਲ typedp = static_cast<T_ਸ਼ੀਰਸ਼*>(vtxp);
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਸੈੱਟ.ਲੱਭੋ(typedp);
        ਜੇ (it != m_ਸੈੱਟ.ਸਿਰਾ() && *it == typedp) m_ਸੈੱਟ.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2, typename... Operands>
    ਸ਼ੀਰਸ਼_2* get(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, Operands... operands) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_ਸੈੱਟ.ਲੱਭੋ(dtype, operands...);
        ਵਾਪਸ it != m_ਸੈੱਟ.ਸਿਰਾ() ? static_cast<ਸ਼ੀਰਸ਼_2*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2, typename... Operands>
    ਸ਼ੀਰਸ਼_2* getOrCreate(Dfg_ਗ੍ਰਾਫ& dfg, File_ਸਤਰ* flp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype,
                        Operands... operands) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_ਸੈੱਟ.ਪਾਓ_Lazy(dtype, operands..., [&]() -> T_ਸ਼ੀਰਸ਼* {
            ਸ਼ੀਰਸ਼_2* ਅਚਲ newp = new ਸ਼ੀਰਸ਼_2{dfg, flp, dtype};
            ਸੈੱਟ_Operands(newp, operands...);
            ਵਾਪਸ newp;
        });
        T_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = *pair.ਪਹਿਲਾ;
        UDEBUGONLY(UASSERT_OBJ(vtxp->ਸਾਂਚਾ is<ਸ਼ੀਰਸ਼_2>(), vtxp, "Vertex is wrong type"););
        ਵਾਪਸ static_cast<ਸ਼ੀਰਸ਼_2*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2>
    using CacheType = typename V3DfgCacheType<ਸ਼ੀਰਸ਼_2, Cache_ਅਧਾਰ,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        Dfg_ਸ਼ੀਰਸ਼_Unary,   /* -> */  Cache<Dfg_ਸ਼ੀਰਸ਼_Unary, HashUnary, EqualUnary>,
        Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ,  /* -> */  Cache<Dfg_ਸ਼ੀਰਸ਼_ਦੁਆਧਾਰੀ, Hash_ਦੁਆਧਾਰੀ, Equal_ਦੁਆਧਾਰੀ>,
        Dfg_ਸ਼ੀਰਸ਼_Ternary, /* -> */  Cache<Dfg_ਸ਼ੀਰਸ਼_Ternary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
Dfg_ਗ੍ਰਾਫ& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_ਸ਼ੀਰਸ਼_TYPE(ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<Cache_ਅਧਾਰ*, VDfgType::ਅੰਕ_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2>
CacheType<ਸ਼ੀਰਸ਼_2>* cacheForType() {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE(t) \
    ਜੇ VL_CONSTEXPR_CXX17 (std::is_same<ਸ਼ੀਰਸ਼_2, t>::ਮੁੱਲ) \
        ਵਾਪਸ reinterpret_cast<CacheType<ਸ਼ੀਰਸ਼_2>*>(&m_cache##t);
    FOREACH_DFG_ਸ਼ੀਰਸ਼_TYPE(ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE
    ਵਾਪਸ nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ਸਥਿਰ V3Hash ਸ਼ੀਰਸ਼_Hash(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* vtxp) {
    ਜੇ (ਅਚਲ DfgConst* ਅਚਲ constp = vtxp->cast<DfgConst>()) ਵਾਪਸ constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Hash();
    ਵਾਪਸ V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ਸਥਿਰ bool ਸ਼ੀਰਸ਼_Equal(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ap, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* bp) {
    ਜੇ (ap == bp) ਵਾਪਸ true;
    ਜੇ (ap->type() != bp->type()) ਵਾਪਸ false;
    ਜੇ (ਅਚਲ DfgConst* ਅਚਲ aConstp = ap->cast<DfgConst>()) {
        ਅਚਲ DfgConst* ਅਚਲ bConstp = bp->as<DfgConst>();
        ਵਾਪਸ aConstp->ਅੰਕ().isCaseEq(bConstp->ਅੰਕ());
    }
    ਵਾਪਸ false;
}

ਜਨਤਕ:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(Dfg_ਗ੍ਰਾਫ& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_ਸ਼ੀਰਸ਼_TYPE(ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE_PTR)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ ਸ਼ੀਰਸ਼_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
Dfg_ਸ਼ੀਰਸ਼* cache(Dfg_ਸ਼ੀਰਸ਼* vtxp) { ਵਾਪਸ m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
ਖਾਲੀ invalidate(Dfg_ਸ਼ੀਰਸ਼* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2, typename... Operands>
ਸ਼ੀਰਸ਼_2* getOrCreate(File_ਸਤਰ* flp, ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, Operands... operands) {
    static_assert(std::is_final<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Not a cached vertex type");
    ਵਾਪਸ cacheForType<ਸ਼ੀਰਸ਼_2>()->ਸਾਂਚਾ getOrCreate<ਸ਼ੀਰਸ਼_2>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
ਸਾਂਚਾ <typename ਸ਼ੀਰਸ਼_2, typename... Operands>
ਸ਼ੀਰਸ਼_2* get(ਅਚਲ Dfg_ਅੰਕੜੇ_Type& dtype, Operands... operands) {
    static_assert(std::is_final<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<ਸ਼ੀਰਸ਼_2>::ਮੁੱਲ, "Not a cached vertex type");
    ਵਾਪਸ cacheForType<ਸ਼ੀਰਸ਼_2>()->ਸਾਂਚਾ get<ਸ਼ੀਰਸ਼_2>(dtype, operands...);
}
}
;

#ਜੇ_ਅੰਤ  // VERILATOR_V3DFGCACHE_H_
