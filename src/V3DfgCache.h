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

#که_نه_تعریف_شوی VERILATOR_V3DFGCACHE_H_
#تعریف_کړه VERILATOR_V3DFGCACHE_H_

#ضمن "verilatedos.h"

#ضمن "V3Dfg.h"
#ضمن "V3DfgDataType.h"
#ضمن "V3HashTable.h"

#ضمن <type_traits>

// Type predicate true for cached vertex types
کينډۍ <typename راس_2>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_بنسټ_د<Dfg_راس_Unary, راس_2>::ارزښت
                                       || std::is_بنسټ_د<Dfg_راس_دوګونی, راس_2>::ارزښت
                                       || std::is_بنسټ_د<Dfg_راس_Ternary, راس_2>::ارزښت>;

// Helper template to determine the cache type for a vertex type
کينډۍ <typename راس_2, typename Cache_بنسټ, typename... جوړې>
بنية V3DfgCacheType final {
    using Type = Cache_بنسټ;
};

کينډۍ <typename راس_2, typename Cache_بنسټ, typename راس_بنسټ, typename Cache,
          typename... جوړې>
بنية V3DfgCacheType<راس_2, Cache_بنسټ, راس_بنسټ, Cache, جوړې...> final {
    using Type = std::conditional_t<std::is_بنسټ_د<راس_بنسټ, راس_2>::ارزښت, Cache,
                                    typename V3DfgCacheType<راس_2, Cache_بنسټ, جوړې...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    بنية HashSel final {
        اندازه_t چلوونکی()(ثابت DfgSel* vtxp) ثابت {
            أرجع چلوونکی()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        اندازه_t چلوونکی()(ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* fromp, uint32_t lsb) ثابت {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += راس_Hash(fromp);
            hash += lsb;
            أرجع hash.ارزښت();
        }
    };
    بنية EqualSel final {
        bool چلوونکی()(ثابت DfgSel* ap, ثابت DfgSel* bp) ثابت {
            أرجع چلوونکی()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool چلوونکی()(ثابت DfgSel* vtxp, ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* fromp,
                        uint32_t lsb) ثابت {
            أرجع vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && راس_Equal(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    بنية HashUnary final {
        اندازه_t چلوونکی()(ثابت Dfg_راس_Unary* vtxp) ثابت {
            أرجع چلوونکی()(vtxp->dtype(), vtxp->inputp(0));
        }
        اندازه_t چلوونکی()(ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* source0p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            أرجع hash.ارزښت();
        }
    };
    بنية EqualUnary final {
        bool چلوونکی()(ثابت Dfg_راس_Unary* ap, ثابت Dfg_راس_Unary* bp) ثابت {
            أرجع چلوونکی()(ap, bp->dtype(), bp->inputp(0));
        }
        bool چلوونکی()(ثابت Dfg_راس_Unary* vtxp, ثابت Dfg_معلومات_Type& dtype,
                        ثابت Dfg_راس* source0p) ثابت {
            أرجع vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    بنية Hash_دوګونی final {
        اندازه_t چلوونکی()(ثابت Dfg_راس_دوګونی* vtxp) ثابت {
            أرجع چلوونکی()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        اندازه_t چلوونکی()(ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* source0p,
                          ثابت Dfg_راس* source1p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            hash += راس_Hash(source1p);
            أرجع hash.ارزښت();
        }
    };
    بنية Equal_دوګونی final {
        bool چلوونکی()(ثابت Dfg_راس_دوګونی* ap, ثابت Dfg_راس_دوګونی* bp) ثابت {
            أرجع چلوونکی()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool چلوونکی()(ثابت Dfg_راس_دوګونی* vtxp, ثابت Dfg_معلومات_Type& dtype,
                        ثابت Dfg_راس* source0p, ثابت Dfg_راس* source1p) ثابت {
            أرجع vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p)
                   && راس_Equal(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    بنية HashTernary final {
        اندازه_t چلوونکی()(ثابت Dfg_راس_Ternary* vtxp) ثابت {
            أرجع چلوونکی()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        اندازه_t چلوونکی()(ثابت Dfg_معلومات_Type& dtype, ثابت Dfg_راس* source0p,
                          ثابت Dfg_راس* source1p, ثابت Dfg_راس* source2p) ثابت {
            V3Hash hash = dtype.hash();
            hash += راس_Hash(source0p);
            hash += راس_Hash(source1p);
            hash += راس_Hash(source2p);
            أرجع hash.ارزښت();
        }
    };

    بنية EqualTernary final {
        bool چلوونکی()(ثابت Dfg_راس_Ternary* ap, ثابت Dfg_راس_Ternary* bp) ثابت {
            أرجع چلوونکی()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool چلوونکی()(ثابت Dfg_راس_Ternary* vtxp, ثابت Dfg_معلومات_Type& dtype,
                        ثابت Dfg_راس* source0p, ثابت Dfg_راس* source1p,
                        ثابت Dfg_راس* source2p) ثابت {
            أرجع vtxp->dtype() == dtype && راس_Equal(vtxp->inputp(0), source0p)
                   && راس_Equal(vtxp->inputp(1), source1p)
                   && راس_Equal(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class Cache_بنسټ VL_NOT_FINAL{محمي :
                                     // These set the operands of a new vertex
                                     ساكن فراغ سېټ_Operands(DfgSel * vtxp, Dfg_راس* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ساكن فراغ
سېټ_Operands(Dfg_راس_Unary* vtxp, Dfg_راس* src0p) {  //
    vtxp->inputp(0, src0p);
}

ساكن فراغ سېټ_Operands(Dfg_راس_دوګونی* vtxp, Dfg_راس* src0p, Dfg_راس* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ساكن فراغ سېټ_Operands(Dfg_راس_Ternary* vtxp, Dfg_راس* src0p, Dfg_راس* src1p,
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

کينډۍ <typename T_راس, typename T_Hash, typename T_Equal>
class Cache final : عام Cache_بنسټ {
    static_assert(std::is_بنسټ_د<Dfg_راس, T_راس>::ارزښت, "T_Vertex must be a DfgVertex");

    // STATE
    V3_Hash_سېټ<T_راس*, T_Hash, T_Equal> m_سېټ;

عام:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    Dfg_راس* cache(Dfg_راس* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_راس>(), vtxp, "Vertex is wrong type"););
        T_راس* ثابت typedp = static_cast<T_راس*>(vtxp);
        T_راس* ثابت cachedp = *m_سېټ.ورننباسه(typedp).لومړی;
        أرجع cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    فراغ invalidate(Dfg_راس* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_راس>(), vtxp, "Vertex is wrong type"););
        T_راس* ثابت typedp = static_cast<T_راس*>(vtxp);
        ثابت خپلکاری it = m_سېټ.پیدا_کړه(typedp);
        إذا (it != m_سېټ.پای_ټکی() && *it == typedp) m_سېټ.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    کينډۍ <typename راس_2, typename... Operands>
    راس_2* get(ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
        ثابت خپلکاری it = m_سېټ.پیدا_کړه(dtype, operands...);
        أرجع it != m_سېټ.پای_ټکی() ? static_cast<راس_2*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    کينډۍ <typename راس_2, typename... Operands>
    راس_2* getOrCreate(Dfg_ګراف& dfg, File_کرښه* flp, ثابت Dfg_معلومات_Type& dtype,
                        Operands... operands) {
        ثابت خپلکاری pair = m_سېټ.ورننباسه_Lazy(dtype, operands..., [&]() -> T_راس* {
            راس_2* ثابت newp = new راس_2{dfg, flp, dtype};
            سېټ_Operands(newp, operands...);
            أرجع newp;
        });
        T_راس* ثابت vtxp = *pair.لومړی;
        UDEBUGONLY(UASSERT_OBJ(vtxp->کينډۍ is<راس_2>(), vtxp, "Vertex is wrong type"););
        أرجع static_cast<راس_2*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    کينډۍ <typename راس_2>
    using CacheType = typename V3DfgCacheType<راس_2, Cache_بنسټ,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        Dfg_راس_Unary,   /* -> */  Cache<Dfg_راس_Unary, HashUnary, EqualUnary>,
        Dfg_راس_دوګونی,  /* -> */  Cache<Dfg_راس_دوګونی, Hash_دوګونی, Equal_دوګونی>,
        Dfg_راس_Ternary, /* -> */  Cache<Dfg_راس_Ternary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
Dfg_ګراف& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#تعریف_کړه راس_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE)
#تعریف_لغوه راس_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<Cache_بنسټ*, VDfgType::عدد_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
کينډۍ <typename راس_2>
CacheType<راس_2>* cacheForType() {
#تعریف_کړه راس_CACHE_DECLARE_CACHE(t) \
    إذا VL_CONSTEXPR_CXX17 (std::is_same<راس_2, t>::ارزښت) \
        أرجع reinterpret_cast<CacheType<راس_2>*>(&m_cache##t);
    FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE)
#تعریف_لغوه راس_CACHE_DECLARE_CACHE
    أرجع nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ساكن V3Hash راس_Hash(ثابت Dfg_راس* vtxp) {
    إذا (ثابت DfgConst* ثابت constp = vtxp->cast<DfgConst>()) أرجع constp->عدد().منزل_Hash();
    أرجع V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ساكن bool راس_Equal(ثابت Dfg_راس* ap, ثابت Dfg_راس* bp) {
    إذا (ap == bp) أرجع true;
    إذا (ap->type() != bp->type()) أرجع false;
    إذا (ثابت DfgConst* ثابت aConstp = ap->cast<DfgConst>()) {
        ثابت DfgConst* ثابت bConstp = bp->as<DfgConst>();
        أرجع aConstp->عدد().isCaseEq(bConstp->عدد());
    }
    أرجع false;
}

عام:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(Dfg_ګراف& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#تعریف_کړه راس_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_راس_TYPE(راس_CACHE_DECLARE_CACHE_PTR)
#تعریف_لغوه راس_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
Dfg_راس* cache(Dfg_راس* vtxp) { أرجع m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
فراغ invalidate(Dfg_راس* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
کينډۍ <typename راس_2, typename... Operands>
راس_2* getOrCreate(File_کرښه* flp, ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
    static_assert(std::is_final<راس_2>::ارزښت, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<راس_2>::ارزښت, "Not a cached vertex type");
    أرجع cacheForType<راس_2>()->کينډۍ getOrCreate<راس_2>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
کينډۍ <typename راس_2, typename... Operands>
راس_2* get(ثابت Dfg_معلومات_Type& dtype, Operands... operands) {
    static_assert(std::is_final<راس_2>::ارزښت, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<راس_2>::ارزښت, "Not a cached vertex type");
    أرجع cacheForType<راس_2>()->کينډۍ get<راس_2>(dtype, operands...);
}
}
;

#د_که_پای  // VERILATOR_V3DFGCACHE_H_
