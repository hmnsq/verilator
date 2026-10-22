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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3DFGCACHE_H_
#وصف_ڪريو VERILATOR_V3DFGCACHE_H_

#ضمن "verilatedos.h"

#ضمن "V3Dfg.h"
#ضمن "V3DfgDataType.h"
#ضمن "V3HashTable.h"

#ضمن <type_traits>

// Type predicate true for cached vertex types
سانچو <typename راس_2>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_بنياد_جو<Dfg_راس_Unary, راس_2>::قدر
                                       || std::is_بنياد_جو<Dfg_راس_ٻٽو, راس_2>::قدر
                                       || std::is_بنياد_جو<Dfg_راس_Ternary, راس_2>::قدر>;

// Helper template to determine the cache type for a vertex type
سانچو <typename راس_2, typename Cache_بنياد, typename... جوڙا>
بنية V3DfgCacheType final {
    using Type = Cache_بنياد;
};

سانچو <typename راس_2, typename Cache_بنياد, typename راس_بنياد, typename Cache,
          typename... جوڙا>
بنية V3DfgCacheType<راس_2, Cache_بنياد, راس_بنياد, Cache, جوڙا...> final {
    using Type = std::conditional_t<std::is_بنياد_جو<راس_بنياد, راس_2>::قدر, Cache,
                                    typename V3DfgCacheType<راس_2, Cache_بنياد, جوڙا...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    بنية HashSel final {
        سائيز_t عامل()(ثابت DfgSel* vtxp) ثابت {
            أرجع عامل()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        سائيز_t عامل()(ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* fromp, uint32_t lsb) ثابت {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += راس_Hash(fromp);
            hash += lsb;
            أرجع hash.قدر();
        }
    };
    بنية EqualSel final {
        bool عامل()(ثابت DfgSel* ap, ثابت DfgSel* bp) ثابت {
            أرجع عامل()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool عامل()(ثابت DfgSel* vtxp, ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* fromp,
                        uint32_t lsb) ثابت {
            أرجع vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && راس_Equal(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    بنية HashUnary final {
        سائيز_t عامل()(ثابت Dfg_راس_Unary* vtxp) ثابت {
            أرجع عامل()(vtxp->dtype(), vtxp->inputp(0));
        }
        سائيز_t عامل()(ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* source0p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            أرجع hash.قدر();
        }
    };
    بنية EqualUnary final {
        bool عامل()(ثابت Dfg_راس_Unary* ap, ثابت Dfg_راس_Unary* bp) ثابت {
            أرجع عامل()(ap, bp->dtype(), bp->inputp(0));
        }
        bool عامل()(ثابت Dfg_راس_Unary* vtxp, ثابت Dfg_معلومات_Type& dtype,
                        ثابت Dfg_راس* source0p) ثابت {
            أرجع vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    بنية Hash_ٻٽو final {
        سائيز_t عامل()(ثابت Dfg_راس_ٻٽو* vtxp) ثابت {
            أرجع عامل()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        سائيز_t عامل()(ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* source0p,
                          ثابت Dfg_راس* source1p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            hash += راس_Hash(source1p);
            أرجع hash.قدر();
        }
    };
    بنية Equal_ٻٽو final {
        bool عامل()(ثابت Dfg_راس_ٻٽو* ap, ثابت Dfg_راس_ٻٽو* bp) ثابت {
            أرجع عامل()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool عامل()(ثابت Dfg_راس_ٻٽو* vtxp, ثابت Dfg_معلومات_Type& dtype,
                        ثابت Dfg_راس* source0p, ثابت Dfg_راس* source1p) ثابت {
            أرجع vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p)
                   && راس_Equal(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    بنية HashTernary final {
        سائيز_t عامل()(ثابت Dfg_راس_Ternary* vtxp) ثابت {
            أرجع عامل()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        سائيز_t عامل()(ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* source0p,
                          ثابت Dfg_راس* source1p, ثابت Dfg_راس* source2p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            hash += راس_Hash(source1p);
            hash += راس_Hash(source2p);
            أرجع hash.قدر();
        }
    };

    بنية EqualTernary final {
        bool عامل()(ثابت Dfg_راس_Ternary* ap, ثابت Dfg_راس_Ternary* bp) ثابت {
            أرجع عامل()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool عامل()(ثابت Dfg_راس_Ternary* vtxp, ثابت Dfg_معلومات_Type& dtype,
                        ثابت Dfg_راس* source0p, ثابت Dfg_راس* source1p,
                        ثابت Dfg_راس* source2p) ثابت {
            أرجع vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p)
                   && راس_Equal(vtxp->inputp(1), source1p)
                   && راس_Equal(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class Cache_بنياد VL_NOT_FINAL{محمي :
                                     // These set the operands of a new vertex
                                     ساكن فراغ سيٽ_Operands(DfgSel * vtxp, Dfg_راس* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ساكن فراغ
سيٽ_Operands(Dfg_راس_Unary* vtxp, Dfg_راس* src0p) {  //
    vtxp->inputp(0, src0p);
}

ساكن فراغ سيٽ_Operands(Dfg_راس_ٻٽو* vtxp, Dfg_راس* src0p, Dfg_راس* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ساكن فراغ سيٽ_Operands(Dfg_راس_Ternary* vtxp, Dfg_راس* src0p, Dfg_راس* src1p,
                        Dfg_راس* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

عام:
// CacheBase does not cache anything
مجازي Dfg_راس* cache(Dfg_راس*) { أرجع nullptr; }
مجازي فراغ invalidate(Dfg_راس*) {}
}
;

سانچو <typename T_راس, typename T_Hash, typename T_Equal>
class Cache final : عام Cache_بنياد {
    static_assert(std::is_بنياد_جو<Dfg_راس, T_راس>::قدر, "T_Vertex must be a DfgVertex");

    // STATE
    V3_Hash_سيٽ<T_راس*, T_Hash, T_Equal> m_سيٽ;

عام:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    Dfg_راس* cache(Dfg_راس* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_راس>(), vtxp, "Vertex is wrong type"););
        T_راس* ثابت typedp = static_cast<T_راس*>(vtxp);
        T_راس* ثابت cachedp = *m_سيٽ.وجهو(typedp).پهريون;
        أرجع cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    فراغ invalidate(Dfg_راس* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_راس>(), vtxp, "Vertex is wrong type"););
        T_راس* ثابت typedp = static_cast<T_راس*>(vtxp);
        ثابت خودڪار it = m_سيٽ.ڳوليو(typedp);
        إذا (it != m_سيٽ.پڇاڙي() && *it == typedp) m_سيٽ.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    سانچو <typename راس_2, typename... Operands>
    راس_2* get(ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
        ثابت خودڪار it = m_سيٽ.ڳوليو(dtype, operands...);
        أرجع it != m_سيٽ.پڇاڙي() ? static_cast<راس_2*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    سانچو <typename راس_2, typename... Operands>
    راس_2* getOrCreate(Dfg_گراف& dfg, File_سٽ* flp, ثابت Dfg_معلومات_Type& dtype,
                        Operands... operands) {
        ثابت خودڪار pair = m_سيٽ.وجهو_Lazy(dtype, operands..., [&]() -> T_راس* {
            راس_2* ثابت newp = new راس_2{dfg, flp, dtype};
            سيٽ_Operands(newp, operands...);
            أرجع newp;
        });
        T_راس* ثابت vtxp = *pair.پهريون;
        UDEBUGONLY(UASSERT_OBJ(vtxp->سانچو is<راس_2>(), vtxp, "Vertex is wrong type"););
        أرجع static_cast<راس_2*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    سانچو <typename راس_2>
    using CacheType = typename V3DfgCacheType<راس_2, Cache_بنياد,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        Dfg_راس_Unary,   /* -> */  Cache<Dfg_راس_Unary, HashUnary, EqualUnary>,
        Dfg_راس_ٻٽو,  /* -> */  Cache<Dfg_راس_ٻٽو, Hash_ٻٽو, Equal_ٻٽو>,
        Dfg_راس_Ternary, /* -> */  Cache<Dfg_راس_Ternary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
Dfg_گراف& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#وصف_ڪريو راس_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE)
#وصف_رد راس_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<Cache_بنياد*, VDfgType::انگ_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
سانچو <typename راس_2>
CacheType<راس_2>* cacheForType() {
#وصف_ڪريو راس_CACHE_DECLARE_CACHE(t) \
    إذا VL_CONSTEXPR_CXX17 (std::is_same<راس_2, t>::قدر) \
        أرجع reinterpret_cast<CacheType<راس_2>*>(&m_cache##t);
    FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE)
#وصف_رد راس_CACHE_DECLARE_CACHE
    أرجع nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ساكن V3Hash راس_Hash(ثابت Dfg_راس* vtxp) {
    إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->انگ().منزل_Hash();
    أرجع V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ساكن bool راس_Equal(ثابت Dfg_راس* ap, ثابت Dfg_راس* bp) {
    إذا (ap == bp) أرجع true;
    إذا (ap->type() != bp->type()) أرجع false;
    إذا (ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
        ثابت DfgConst* ثابت bConstp = bp->as<DfgConst>();
        أرجع aConstp->انگ().isCaseEq(bConstp->انگ());
    }
    أرجع false;
}

عام:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(Dfg_گراف& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#وصف_ڪريو راس_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE_PTR)
#وصف_رد راس_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
Dfg_راس* cache(Dfg_راس* vtxp) { أرجع m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
فراغ invalidate(Dfg_راس* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
سانچو <typename راس_2, typename... Operands>
راس_2* getOrCreate(File_سٽ* flp, ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
    static_assert(std::is_final<راس_2>::قدر, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<راس_2>::قدر, "Not a cached vertex type");
    أرجع cacheForType<راس_2>()->سانچو getOrCreate<راس_2>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
سانچو <typename راس_2, typename... Operands>
راس_2* get(ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
    static_assert(std::is_final<راس_2>::قدر, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<راس_2>::قدر, "Not a cached vertex type");
    أرجع cacheForType<راس_2>()->سانچو get<راس_2>(dtype, operands...);
}
}
;

#جيڪڏهن_پورو  // VERILATOR_V3DFGCACHE_H_
