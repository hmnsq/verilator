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

#اگر_تعریف_نشده VERILATOR_V3DFGCACHE_H_
#تعریف_کن VERILATOR_V3DFGCACHE_H_

#ضمن "verilatedos.h"

#ضمن "V3Dfg.h"
#ضمن "V3DfgDataType.h"
#ضمن "V3HashTable.h"

#ضمن <type_traits>

// Type predicate true for cached vertex types
قالب <typename رأس_2>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_پایه_از<Dfg_رأس_Unary, رأس_2>::مقدار
                                       || std::is_پایه_از<Dfg_رأس_دودویی, رأس_2>::مقدار
                                       || std::is_پایه_از<Dfg_رأس_Ternary, رأس_2>::مقدار>;

// Helper template to determine the cache type for a vertex type
قالب <typename رأس_2, typename Cache_پایه, typename... جفتها>
بنية V3DfgCacheType final {
    using Type = Cache_پایه;
};

قالب <typename رأس_2, typename Cache_پایه, typename رأس_پایه, typename Cache,
          typename... جفتها>
بنية V3DfgCacheType<رأس_2, Cache_پایه, رأس_پایه, Cache, جفتها...> final {
    using Type = std::conditional_t<std::is_پایه_از<رأس_پایه, رأس_2>::مقدار, Cache,
                                    typename V3DfgCacheType<رأس_2, Cache_پایه, جفتها...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    بنية HashSel final {
        اندازه_t عملگر()(ثابت DfgSel* vtxp) ثابت {
            أرجع عملگر()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        اندازه_t عملگر()(ثابت Dfg_داده_Type& dtype, ثابت Dfg_رأس* fromp, uint32_t lsb) ثابت {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += رأس_Hash(fromp);
            hash += lsb;
            أرجع hash.مقدار();
        }
    };
    بنية EqualSel final {
        bool عملگر()(ثابت DfgSel* ap, ثابت DfgSel* bp) ثابت {
            أرجع عملگر()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool عملگر()(ثابت DfgSel* vtxp, ثابت Dfg_داده_Type& dtype, ثابت Dfg_رأس* fromp,
                        uint32_t lsb) ثابت {
            أرجع vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && رأس_Equal(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    بنية HashUnary final {
        اندازه_t عملگر()(ثابت Dfg_رأس_Unary* vtxp) ثابت {
            أرجع عملگر()(vtxp->dtype(), vtxp->inputp(0));
        }
        اندازه_t عملگر()(ثابت Dfg_داده_Type& dtype, ثابت Dfg_رأس* source0p) ثابت {
            V3Hash hash = dtype.hash();
            hash += رأس_Hash(source0p);
            أرجع hash.مقدار();
        }
    };
    بنية EqualUnary final {
        bool عملگر()(ثابت Dfg_رأس_Unary* ap, ثابت Dfg_رأس_Unary* bp) ثابت {
            أرجع عملگر()(ap, bp->dtype(), bp->inputp(0));
        }
        bool عملگر()(ثابت Dfg_رأس_Unary* vtxp, ثابت Dfg_داده_Type& dtype,
                        ثابت Dfg_رأس* source0p) ثابت {
            أرجع vtxp->dtype() == dtype && رأس_Equal(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    بنية Hash_دودویی final {
        اندازه_t عملگر()(ثابت Dfg_رأس_دودویی* vtxp) ثابت {
            أرجع عملگر()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        اندازه_t عملگر()(ثابت Dfg_داده_Type& dtype, ثابت Dfg_رأس* source0p,
                          ثابت Dfg_رأس* source1p) ثابت {
            V3Hash hash = dtype.hash();
            hash += رأس_Hash(source0p);
            hash += رأس_Hash(source1p);
            أرجع hash.مقدار();
        }
    };
    بنية Equal_دودویی final {
        bool عملگر()(ثابت Dfg_رأس_دودویی* ap, ثابت Dfg_رأس_دودویی* bp) ثابت {
            أرجع عملگر()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool عملگر()(ثابت Dfg_رأس_دودویی* vtxp, ثابت Dfg_داده_Type& dtype,
                        ثابت Dfg_رأس* source0p, ثابت Dfg_رأس* source1p) ثابت {
            أرجع vtxp->dtype() == dtype && رأس_Equal(vtxp->inputp(0), source0p)
                   && رأس_Equal(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    بنية HashTernary final {
        اندازه_t عملگر()(ثابت Dfg_رأس_Ternary* vtxp) ثابت {
            أرجع عملگر()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        اندازه_t عملگر()(ثابت Dfg_داده_Type& dtype, ثابت Dfg_رأس* source0p,
                          ثابت Dfg_رأس* source1p, ثابت Dfg_رأس* source2p) ثابت {
            V3Hash hash = dtype.hash();
            hash += رأس_Hash(source0p);
            hash += رأس_Hash(source1p);
            hash += رأس_Hash(source2p);
            أرجع hash.مقدار();
        }
    };

    بنية EqualTernary final {
        bool عملگر()(ثابت Dfg_رأس_Ternary* ap, ثابت Dfg_رأس_Ternary* bp) ثابت {
            أرجع عملگر()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool عملگر()(ثابت Dfg_رأس_Ternary* vtxp, ثابت Dfg_داده_Type& dtype,
                        ثابت Dfg_رأس* source0p, ثابت Dfg_رأس* source1p,
                        ثابت Dfg_رأس* source2p) ثابت {
            أرجع vtxp->dtype() == dtype && رأس_Equal(vtxp->inputp(0), source0p)
                   && رأس_Equal(vtxp->inputp(1), source1p)
                   && رأس_Equal(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class Cache_پایه VL_NOT_FINAL{محمي :
                                     // These set the operands of a new vertex
                                     ساكن فراغ مجموعه_Operands(DfgSel * vtxp, Dfg_رأس* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ساكن فراغ
مجموعه_Operands(Dfg_رأس_Unary* vtxp, Dfg_رأس* src0p) {  //
    vtxp->inputp(0, src0p);
}

ساكن فراغ مجموعه_Operands(Dfg_رأس_دودویی* vtxp, Dfg_رأس* src0p, Dfg_رأس* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ساكن فراغ مجموعه_Operands(Dfg_رأس_Ternary* vtxp, Dfg_رأس* src0p, Dfg_رأس* src1p,
                        Dfg_رأس* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

عام:
// CacheBase does not cache anything
مجازی Dfg_رأس* cache(Dfg_رأس*) { أرجع nullptr; }
مجازی فراغ invalidate(Dfg_رأس*) {}
}
;

قالب <typename T_رأس, typename T_Hash, typename T_Equal>
class Cache final : عام Cache_پایه {
    static_assert(std::is_پایه_از<Dfg_رأس, T_رأس>::مقدار, "T_Vertex must be a DfgVertex");

    // STATE
    V3_Hash_مجموعه<T_رأس*, T_Hash, T_Equal> m_مجموعه;

عام:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    Dfg_رأس* cache(Dfg_رأس* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_رأس>(), vtxp, "Vertex is wrong type"););
        T_رأس* ثابت typedp = static_cast<T_رأس*>(vtxp);
        T_رأس* ثابت cachedp = *m_مجموعه.درج(typedp).اولی;
        أرجع cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    فراغ invalidate(Dfg_رأس* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_رأس>(), vtxp, "Vertex is wrong type"););
        T_رأس* ثابت typedp = static_cast<T_رأس*>(vtxp);
        ثابت خودکار it = m_مجموعه.پیدا_کن(typedp);
        إذا (it != m_مجموعه.انتها() && *it == typedp) m_مجموعه.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    قالب <typename رأس_2, typename... Operands>
    رأس_2* get(ثابت Dfg_داده_Type& dtype, Operands... operands) {
        ثابت خودکار it = m_مجموعه.پیدا_کن(dtype, operands...);
        أرجع it != m_مجموعه.انتها() ? static_cast<رأس_2*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    قالب <typename رأس_2, typename... Operands>
    رأس_2* getOrCreate(Dfg_گراف& dfg, File_سطر* flp, ثابت Dfg_داده_Type& dtype,
                        Operands... operands) {
        ثابت خودکار pair = m_مجموعه.درج_Lazy(dtype, operands..., [&]() -> T_رأس* {
            رأس_2* ثابت newp = new رأس_2{dfg, flp, dtype};
            مجموعه_Operands(newp, operands...);
            أرجع newp;
        });
        T_رأس* ثابت vtxp = *pair.اولی;
        UDEBUGONLY(UASSERT_OBJ(vtxp->قالب is<رأس_2>(), vtxp, "Vertex is wrong type"););
        أرجع static_cast<رأس_2*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    قالب <typename رأس_2>
    using CacheType = typename V3DfgCacheType<رأس_2, Cache_پایه,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        Dfg_رأس_Unary,   /* -> */  Cache<Dfg_رأس_Unary, HashUnary, EqualUnary>,
        Dfg_رأس_دودویی,  /* -> */  Cache<Dfg_رأس_دودویی, Hash_دودویی, Equal_دودویی>,
        Dfg_رأس_Ternary, /* -> */  Cache<Dfg_رأس_Ternary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
Dfg_گراف& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#تعریف_کن رأس_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_رأس_TYPE(رأس_CACHE_DECLARE_CACHE)
#لغو_تعریف رأس_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<Cache_پایه*, VDfgType::رقم_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
قالب <typename رأس_2>
CacheType<رأس_2>* cacheForType() {
#تعریف_کن رأس_CACHE_DECLARE_CACHE(t) \
    إذا VL_CONSTEXPR_CXX17 (std::is_same<رأس_2, t>::مقدار) \
        أرجع reinterpret_cast<CacheType<رأس_2>*>(&m_cache##t);
    FOREACH_DFG_رأس_TYPE(رأس_CACHE_DECLARE_CACHE)
#لغو_تعریف رأس_CACHE_DECLARE_CACHE
    أرجع nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ساكن V3Hash رأس_Hash(ثابت Dfg_رأس* vtxp) {
    إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->رقم().مقصد_Hash();
    أرجع V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ساكن bool رأس_Equal(ثابت Dfg_رأس* ap, ثابت Dfg_رأس* bp) {
    إذا (ap == bp) أرجع true;
    إذا (ap->type() != bp->type()) أرجع false;
    إذا (ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
        ثابت DfgConst* ثابت bConstp = bp->as<DfgConst>();
        أرجع aConstp->رقم().isCaseEq(bConstp->رقم());
    }
    أرجع false;
}

عام:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(Dfg_گراف& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#تعریف_کن رأس_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_رأس_TYPE(رأس_CACHE_DECLARE_CACHE_PTR)
#لغو_تعریف رأس_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
Dfg_رأس* cache(Dfg_رأس* vtxp) { أرجع m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
فراغ invalidate(Dfg_رأس* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
قالب <typename رأس_2, typename... Operands>
رأس_2* getOrCreate(File_سطر* flp, ثابت Dfg_داده_Type& dtype, Operands... operands) {
    static_assert(std::is_final<رأس_2>::مقدار, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<رأس_2>::مقدار, "Not a cached vertex type");
    أرجع cacheForType<رأس_2>()->قالب getOrCreate<رأس_2>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
قالب <typename رأس_2, typename... Operands>
رأس_2* get(ثابت Dfg_داده_Type& dtype, Operands... operands) {
    static_assert(std::is_final<رأس_2>::مقدار, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<رأس_2>::مقدار, "Not a cached vertex type");
    أرجع cacheForType<رأس_2>()->قالب get<رأس_2>(dtype, operands...);
}
}
;

#پایان_اگر  // VERILATOR_V3DFGCACHE_H_
