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

#اگر_غیر_تعریف_شدہ VERILATOR_V3DFGCACHE_H_
#تعریف_کرو VERILATOR_V3DFGCACHE_H_

#شامل "verilatedos.h"

#شامل "V3Dfg.h"
#شامل "V3DfgDataType.h"
#شامل "V3HashTable.h"

#شامل <type_traits>

// Type predicate true for cached vertex types
سانچہ <typename راس_2>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_بنیاد_کا<Dfg_راس_Unary, راس_2>::قدر
                                       || std::is_بنیاد_کا<Dfg_راس_ثنائی, راس_2>::قدر
                                       || std::is_بنیاد_کا<Dfg_راس_Ternary, راس_2>::قدر>;

// Helper template to determine the cache type for a vertex type
سانچہ <typename راس_2, typename Cache_بنیاد, typename... جوڑے>
ساخت V3DfgCacheType final {
    using Type = Cache_بنیاد;
};

سانچہ <typename راس_2, typename Cache_بنیاد, typename راس_بنیاد, typename Cache,
          typename... جوڑے>
ساخت V3DfgCacheType<راس_2, Cache_بنیاد, راس_بنیاد, Cache, جوڑے...> final {
    using Type = std::conditional_t<std::is_بنیاد_کا<راس_بنیاد, راس_2>::قدر, Cache,
                                    typename V3DfgCacheType<راس_2, Cache_بنیاد, جوڑے...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    ساخت HashSel final {
        سائز_t عامل()(ثابت DfgSel* vtxp) ثابت {
            واپس عامل()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        سائز_t عامل()(ثابت Dfg_ڈیٹا_Type& dtype, ثابت Dfg_راس* fromp, uint32_t lsb) ثابت {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += راس_Hash(fromp);
            hash += lsb;
            واپس hash.قدر();
        }
    };
    ساخت EqualSel final {
        bool عامل()(ثابت DfgSel* ap, ثابت DfgSel* bp) ثابت {
            واپس عامل()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool عامل()(ثابت DfgSel* vtxp, ثابت Dfg_ڈیٹا_Type& dtype, ثابت Dfg_راس* fromp,
                        uint32_t lsb) ثابت {
            واپس vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && راس_Equal(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    ساخت HashUnary final {
        سائز_t عامل()(ثابت Dfg_راس_Unary* vtxp) ثابت {
            واپس عامل()(vtxp->dtype(), vtxp->inputp(0));
        }
        سائز_t عامل()(ثابت Dfg_ڈیٹا_Type& dtype, ثابت Dfg_راس* source0p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            واپس hash.قدر();
        }
    };
    ساخت EqualUnary final {
        bool عامل()(ثابت Dfg_راس_Unary* ap, ثابت Dfg_راس_Unary* bp) ثابت {
            واپس عامل()(ap, bp->dtype(), bp->inputp(0));
        }
        bool عامل()(ثابت Dfg_راس_Unary* vtxp, ثابت Dfg_ڈیٹا_Type& dtype,
                        ثابت Dfg_راس* source0p) ثابت {
            واپس vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    ساخت Hash_ثنائی final {
        سائز_t عامل()(ثابت Dfg_راس_ثنائی* vtxp) ثابت {
            واپس عامل()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        سائز_t عامل()(ثابت Dfg_ڈیٹا_Type& dtype, ثابت Dfg_راس* source0p,
                          ثابت Dfg_راس* source1p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            hash += راس_Hash(source1p);
            واپس hash.قدر();
        }
    };
    ساخت Equal_ثنائی final {
        bool عامل()(ثابت Dfg_راس_ثنائی* ap, ثابت Dfg_راس_ثنائی* bp) ثابت {
            واپس عامل()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool عامل()(ثابت Dfg_راس_ثنائی* vtxp, ثابت Dfg_ڈیٹا_Type& dtype,
                        ثابت Dfg_راس* source0p, ثابت Dfg_راس* source1p) ثابت {
            واپس vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p)
                   && راس_Equal(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    ساخت HashTernary final {
        سائز_t عامل()(ثابت Dfg_راس_Ternary* vtxp) ثابت {
            واپس عامل()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        سائز_t عامل()(ثابت Dfg_ڈیٹا_Type& dtype, ثابت Dfg_راس* source0p,
                          ثابت Dfg_راس* source1p, ثابت Dfg_راس* source2p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            hash += راس_Hash(source1p);
            hash += راس_Hash(source2p);
            واپس hash.قدر();
        }
    };

    ساخت EqualTernary final {
        bool عامل()(ثابت Dfg_راس_Ternary* ap, ثابت Dfg_راس_Ternary* bp) ثابت {
            واپس عامل()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool عامل()(ثابت Dfg_راس_Ternary* vtxp, ثابت Dfg_ڈیٹا_Type& dtype,
                        ثابت Dfg_راس* source0p, ثابت Dfg_راس* source1p,
                        ثابت Dfg_راس* source2p) ثابت {
            واپس vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p)
                   && راس_Equal(vtxp->inputp(1), source1p)
                   && راس_Equal(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class Cache_بنیاد VL_NOT_FINAL{محفوظ :
                                     // These set the operands of a new vertex
                                     ساکن خالی سیٹ_Operands(DfgSel * vtxp, Dfg_راس* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ساکن خالی
سیٹ_Operands(Dfg_راس_Unary* vtxp, Dfg_راس* src0p) {  //
    vtxp->inputp(0, src0p);
}

ساکن خالی سیٹ_Operands(Dfg_راس_ثنائی* vtxp, Dfg_راس* src0p, Dfg_راس* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ساکن خالی سیٹ_Operands(Dfg_راس_Ternary* vtxp, Dfg_راس* src0p, Dfg_راس* src1p,
                        Dfg_راس* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

عوامی:
// CacheBase does not cache anything
مجازی Dfg_راس* cache(Dfg_راس*) { واپس nullptr; }
مجازی خالی invalidate(Dfg_راس*) {}
}
;

سانچہ <typename T_راس, typename T_Hash, typename T_Equal>
class Cache final : عوامی Cache_بنیاد {
    static_assert(std::is_بنیاد_کا<Dfg_راس, T_راس>::قدر, "T_Vertex must be a DfgVertex");

    // STATE
    V3_Hash_سیٹ<T_راس*, T_Hash, T_Equal> m_سیٹ;

عوامی:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    Dfg_راس* cache(Dfg_راس* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_راس>(), vtxp, "Vertex is wrong type"););
        T_راس* ثابت typedp = static_cast<T_راس*>(vtxp);
        T_راس* ثابت cachedp = *m_سیٹ.داخل_کرو(typedp).پہلا;
        واپس cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    خالی invalidate(Dfg_راس* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_راس>(), vtxp, "Vertex is wrong type"););
        T_راس* ثابت typedp = static_cast<T_راس*>(vtxp);
        ثابت خودکار it = m_سیٹ.ڈھونڈو(typedp);
        اگر (it != m_سیٹ.انتہا() && *it == typedp) m_سیٹ.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    سانچہ <typename راس_2, typename... Operands>
    راس_2* get(ثابت Dfg_ڈیٹا_Type& dtype, Operands... operands) {
        ثابت خودکار it = m_سیٹ.ڈھونڈو(dtype, operands...);
        واپس it != m_سیٹ.انتہا() ? static_cast<راس_2*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    سانچہ <typename راس_2, typename... Operands>
    راس_2* getOrCreate(Dfg_گراف& dfg, File_سطر* flp, ثابت Dfg_ڈیٹا_Type& dtype,
                        Operands... operands) {
        ثابت خودکار pair = m_سیٹ.داخل_کرو_Lazy(dtype, operands..., [&]() -> T_راس* {
            راس_2* ثابت newp = new راس_2{dfg, flp, dtype};
            سیٹ_Operands(newp, operands...);
            واپس newp;
        });
        T_راس* ثابت vtxp = *pair.پہلا;
        UDEBUGONLY(UASSERT_OBJ(vtxp->سانچہ is<راس_2>(), vtxp, "Vertex is wrong type"););
        واپس static_cast<راس_2*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    سانچہ <typename راس_2>
    using CacheType = typename V3DfgCacheType<راس_2, Cache_بنیاد,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        Dfg_راس_Unary,   /* -> */  Cache<Dfg_راس_Unary, HashUnary, EqualUnary>,
        Dfg_راس_ثنائی,  /* -> */  Cache<Dfg_راس_ثنائی, Hash_ثنائی, Equal_ثنائی>,
        Dfg_راس_Ternary, /* -> */  Cache<Dfg_راس_Ternary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
Dfg_گراف& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#تعریف_کرو راس_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE)
#تعریف_منسوخ راس_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<Cache_بنیاد*, VDfgType::عدد_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
سانچہ <typename راس_2>
CacheType<راس_2>* cacheForType() {
#تعریف_کرو راس_CACHE_DECLARE_CACHE(t) \
    اگر VL_CONSTEXPR_CXX17 (std::is_same<راس_2, t>::قدر) \
        واپس reinterpret_cast<CacheType<راس_2>*>(&m_cache##t);
    FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE)
#تعریف_منسوخ راس_CACHE_DECLARE_CACHE
    واپس nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ساکن V3Hash راس_Hash(ثابت Dfg_راس* vtxp) {
    اگر (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) واپس constp->عدد().مقصد_Hash();
    واپس V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ساکن bool راس_Equal(ثابت Dfg_راس* ap, ثابت Dfg_راس* bp) {
    اگر (ap == bp) واپس true;
    اگر (ap->type() != bp->type()) واپس false;
    اگر (ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
        ثابت DfgConst* ثابت bConstp = bp->as<DfgConst>();
        واپس aConstp->عدد().isCaseEq(bConstp->عدد());
    }
    واپس false;
}

عوامی:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(Dfg_گراف& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#تعریف_کرو راس_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE_PTR)
#تعریف_منسوخ راس_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
Dfg_راس* cache(Dfg_راس* vtxp) { واپس m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
خالی invalidate(Dfg_راس* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
سانچہ <typename راس_2, typename... Operands>
راس_2* getOrCreate(File_سطر* flp, ثابت Dfg_ڈیٹا_Type& dtype, Operands... operands) {
    static_assert(std::is_final<راس_2>::قدر, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<راس_2>::قدر, "Not a cached vertex type");
    واپس cacheForType<راس_2>()->سانچہ getOrCreate<راس_2>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
سانچہ <typename راس_2, typename... Operands>
راس_2* get(ثابت Dfg_ڈیٹا_Type& dtype, Operands... operands) {
    static_assert(std::is_final<راس_2>::قدر, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<راس_2>::قدر, "Not a cached vertex type");
    واپس cacheForType<راس_2>()->سانچہ get<راس_2>(dtype, operands...);
}
}
;

#اگر_ختم  // VERILATOR_V3DFGCACHE_H_
