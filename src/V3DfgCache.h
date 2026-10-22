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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3DFGCACHE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3DFGCACHE_H_

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgDataType.h"
#ಅನ್ತರ್ಭಾವ "V3HashTable.h"

#ಅನ್ತರ್ಭಾವ <type_traits>

// Type predicate true for cached vertex types
ಮಾದರಿ <typename ಶೃಂಗ_2>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_Unary, ಶೃಂಗ_2>::ಮೌಲ್ಯ
                                       || std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ, ಶೃಂಗ_2>::ಮೌಲ್ಯ
                                       || std::is_ತಳ_ದ<Dfg_ಶೃಂಗ_Ternary, ಶೃಂಗ_2>::ಮೌಲ್ಯ>;

// Helper template to determine the cache type for a vertex type
ಮಾದರಿ <typename ಶೃಂಗ_2, typename Cache_ತಳ, typename... ಜೋಡಿಗಳು>
ರಚನೆ V3DfgCacheType final {
    using Type = Cache_ತಳ;
};

ಮಾದರಿ <typename ಶೃಂಗ_2, typename Cache_ತಳ, typename ಶೃಂಗ_ತಳ, typename Cache,
          typename... ಜೋಡಿಗಳು>
ರಚನೆ V3DfgCacheType<ಶೃಂಗ_2, Cache_ತಳ, ಶೃಂಗ_ತಳ, Cache, ಜೋಡಿಗಳು...> final {
    using Type = std::conditional_t<std::is_ತಳ_ದ<ಶೃಂಗ_ತಳ, ಶೃಂಗ_2>::ಮೌಲ್ಯ, Cache,
                                    typename V3DfgCacheType<ಶೃಂಗ_2, Cache_ತಳ, ಜೋಡಿಗಳು...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    ರಚನೆ HashSel final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ DfgSel* vtxp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* fromp, uint32_t lsb) ಸ್ಥಿರಾಂಕ {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += ಶೃಂಗ_Hash(fromp);
            hash += lsb;
            ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
        }
    };
    ರಚನೆ EqualSel final {
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ DfgSel* ap, ಸ್ಥಿರಾಂಕ DfgSel* bp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ DfgSel* vtxp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* fromp,
                        uint32_t lsb) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && ಶೃಂಗ_Equal(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    ರಚನೆ HashUnary final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Unary* vtxp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(vtxp->dtype(), vtxp->inputp(0));
        }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source0p) ಸ್ಥಿರಾಂಕ {
            V3Hash hash = dtype.hash();
            hash += ಶೃಂಗ_Hash(source0p);
            ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
        }
    };
    ರಚನೆ EqualUnary final {
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Unary* ap, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Unary* bp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ap, bp->dtype(), bp->inputp(0));
        }
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Unary* vtxp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype,
                        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source0p) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ vtxp->dtype() == dtype && ಶೃಂಗ_Equal(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    ರಚನೆ Hash_ದ್ವಿಮಾನ final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* vtxp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source0p,
                          ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source1p) ಸ್ಥಿರಾಂಕ {
            V3Hash hash = dtype.hash();
            hash += ಶೃಂಗ_Hash(source0p);
            hash += ಶೃಂಗ_Hash(source1p);
            ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
        }
    };
    ರಚನೆ Equal_ದ್ವಿಮಾನ final {
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* ap, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* bp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* vtxp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype,
                        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source0p, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source1p) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ vtxp->dtype() == dtype && ಶೃಂಗ_Equal(vtxp->inputp(0), source0p)
                   && ಶೃಂಗ_Equal(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    ರಚನೆ HashTernary final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Ternary* vtxp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source0p,
                          ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source1p, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source2p) ಸ್ಥಿರಾಂಕ {
            V3Hash hash = dtype.hash();
            hash += ಶೃಂಗ_Hash(source0p);
            hash += ಶೃಂಗ_Hash(source1p);
            hash += ಶೃಂಗ_Hash(source2p);
            ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
        }
    };

    ರಚನೆ EqualTernary final {
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Ternary* ap, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Ternary* bp) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕ್ರಿಯಾಕಾರಕ()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Ternary* vtxp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype,
                        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source0p, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source1p,
                        ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* source2p) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ vtxp->dtype() == dtype && ಶೃಂಗ_Equal(vtxp->inputp(0), source0p)
                   && ಶೃಂಗ_Equal(vtxp->inputp(1), source1p)
                   && ಶೃಂಗ_Equal(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class Cache_ತಳ VL_NOT_FINAL{ಸಂರಕ್ಷಿತ :
                                     // These set the operands of a new vertex
                                     ಸ್ಥಿರ ಶೂನ್ಯ ಗಣ_Operands(DfgSel * vtxp, Dfg_ಶೃಂಗ* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

ಸ್ಥಿರ ಶೂನ್ಯ
ಗಣ_Operands(Dfg_ಶೃಂಗ_Unary* vtxp, Dfg_ಶೃಂಗ* src0p) {  //
    vtxp->inputp(0, src0p);
}

ಸ್ಥಿರ ಶೂನ್ಯ ಗಣ_Operands(Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ* vtxp, Dfg_ಶೃಂಗ* src0p, Dfg_ಶೃಂಗ* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

ಸ್ಥಿರ ಶೂನ್ಯ ಗಣ_Operands(Dfg_ಶೃಂಗ_Ternary* vtxp, Dfg_ಶೃಂಗ* src0p, Dfg_ಶೃಂಗ* src1p,
                        Dfg_ಶೃಂಗ* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

ಸಾರ್ವಜನಿಕ:
// CacheBase does not cache anything
ವಾಸ್ತವಿಕ Dfg_ಶೃಂಗ* cache(Dfg_ಶೃಂಗ*) { ಹಿಂತಿರುಗಿ nullptr; }
ವಾಸ್ತವಿಕ ಶೂನ್ಯ invalidate(Dfg_ಶೃಂಗ*) {}
}
;

ಮಾದರಿ <typename T_ಶೃಂಗ, typename T_Hash, typename T_Equal>
class Cache final : ಸಾರ್ವಜನಿಕ Cache_ತಳ {
    static_assert(std::is_ತಳ_ದ<Dfg_ಶೃಂಗ, T_ಶೃಂಗ>::ಮೌಲ್ಯ, "T_Vertex must be a DfgVertex");

    // STATE
    V3_Hash_ಗಣ<T_ಶೃಂಗ*, T_Hash, T_Equal> m_ಗಣ;

ಸಾರ್ವಜನಿಕ:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    Dfg_ಶೃಂಗ* cache(Dfg_ಶೃಂಗ* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_ಶೃಂಗ>(), vtxp, "Vertex is wrong type"););
        T_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ typedp = static_cast<T_ಶೃಂಗ*>(vtxp);
        T_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cachedp = *m_ಗಣ.ಸೇರಿಸು(typedp).ಮೊದಲ;
        ಹಿಂತಿರುಗಿ cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    ಶೂನ್ಯ invalidate(Dfg_ಶೃಂಗ* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_ಶೃಂಗ>(), vtxp, "Vertex is wrong type"););
        T_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ typedp = static_cast<T_ಶೃಂಗ*>(vtxp);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ಗಣ.ಕಂಡುಹಿಡಿ(typedp);
        ಆದರೆ (it != m_ಗಣ.ಕೊನೆ() && *it == typedp) m_ಗಣ.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    ಮಾದರಿ <typename ಶೃಂಗ_2, typename... Operands>
    ಶೃಂಗ_2* get(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, Operands... operands) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_ಗಣ.ಕಂಡುಹಿಡಿ(dtype, operands...);
        ಹಿಂತಿರುಗಿ it != m_ಗಣ.ಕೊನೆ() ? static_cast<ಶೃಂಗ_2*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    ಮಾದರಿ <typename ಶೃಂಗ_2, typename... Operands>
    ಶೃಂಗ_2* getOrCreate(Dfg_ನಕ್ಷೆ& dfg, File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype,
                        Operands... operands) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_ಗಣ.ಸೇರಿಸು_Lazy(dtype, operands..., [&]() -> T_ಶೃಂಗ* {
            ಶೃಂಗ_2* ಸ್ಥಿರಾಂಕ newp = new ಶೃಂಗ_2{dfg, flp, dtype};
            ಗಣ_Operands(newp, operands...);
            ಹಿಂತಿರುಗಿ newp;
        });
        T_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = *pair.ಮೊದಲ;
        UDEBUGONLY(UASSERT_OBJ(vtxp->ಮಾದರಿ is<ಶೃಂಗ_2>(), vtxp, "Vertex is wrong type"););
        ಹಿಂತಿರುಗಿ static_cast<ಶೃಂಗ_2*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    ಮಾದರಿ <typename ಶೃಂಗ_2>
    using CacheType = typename V3DfgCacheType<ಶೃಂಗ_2, Cache_ತಳ,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        Dfg_ಶೃಂಗ_Unary,   /* -> */  Cache<Dfg_ಶೃಂಗ_Unary, HashUnary, EqualUnary>,
        Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ,  /* -> */  Cache<Dfg_ಶೃಂಗ_ದ್ವಿಮಾನ, Hash_ದ್ವಿಮಾನ, Equal_ದ್ವಿಮಾನ>,
        Dfg_ಶೃಂಗ_Ternary, /* -> */  Cache<Dfg_ಶೃಂಗ_Ternary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
Dfg_ನಕ್ಷೆ& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#ವ್ಯಾಖ್ಯಾನಿಸು ಶೃಂಗ_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_ಶೃಂಗ_TYPE(ಶೃಂಗ_CACHE_DECLARE_CACHE)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು ಶೃಂಗ_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<Cache_ತಳ*, VDfgType::ಅಂಕೆ_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
ಮಾದರಿ <typename ಶೃಂಗ_2>
CacheType<ಶೃಂಗ_2>* cacheForType() {
#ವ್ಯಾಖ್ಯಾನಿಸು ಶೃಂಗ_CACHE_DECLARE_CACHE(t) \
    ಆದರೆ VL_CONSTEXPR_CXX17 (std::is_same<ಶೃಂಗ_2, t>::ಮೌಲ್ಯ) \
        ಹಿಂತಿರುಗಿ reinterpret_cast<CacheType<ಶೃಂಗ_2>*>(&m_cache##t);
    FOREACH_DFG_ಶೃಂಗ_TYPE(ಶೃಂಗ_CACHE_DECLARE_CACHE)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು ಶೃಂಗ_CACHE_DECLARE_CACHE
    ಹಿಂತಿರುಗಿ nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
ಸ್ಥಿರ V3Hash ಶೃಂಗ_Hash(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* vtxp) {
    ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ constp = vtxp->cast<DfgConst>()) ಹಿಂತಿರುಗಿ constp->ಅಂಕೆ().ಗಮ್ಯ_Hash();
    ಹಿಂತಿರುಗಿ V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
ಸ್ಥಿರ bool ಶೃಂಗ_Equal(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ap, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* bp) {
    ಆದರೆ (ap == bp) ಹಿಂತಿರುಗಿ true;
    ಆದರೆ (ap->type() != bp->type()) ಹಿಂತಿರುಗಿ false;
    ಆದರೆ (ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ aConstp = ap->cast<DfgConst>()) {
        ಸ್ಥಿರಾಂಕ DfgConst* ಸ್ಥಿರಾಂಕ bConstp = bp->as<DfgConst>();
        ಹಿಂತಿರುಗಿ aConstp->ಅಂಕೆ().isCaseEq(bConstp->ಅಂಕೆ());
    }
    ಹಿಂತಿರುಗಿ false;
}

ಸಾರ್ವಜನಿಕ:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(Dfg_ನಕ್ಷೆ& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#ವ್ಯಾಖ್ಯಾನಿಸು ಶೃಂಗ_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_ಶೃಂಗ_TYPE(ಶೃಂಗ_CACHE_DECLARE_CACHE_PTR)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು ಶೃಂಗ_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
Dfg_ಶೃಂಗ* cache(Dfg_ಶೃಂಗ* vtxp) { ಹಿಂತಿರುಗಿ m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
ಶೂನ್ಯ invalidate(Dfg_ಶೃಂಗ* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
ಮಾದರಿ <typename ಶೃಂಗ_2, typename... Operands>
ಶೃಂಗ_2* getOrCreate(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, Operands... operands) {
    static_assert(std::is_final<ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Not a cached vertex type");
    ಹಿಂತಿರುಗಿ cacheForType<ಶೃಂಗ_2>()->ಮಾದರಿ getOrCreate<ಶೃಂಗ_2>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
ಮಾದರಿ <typename ಶೃಂಗ_2, typename... Operands>
ಶೃಂಗ_2* get(ಸ್ಥಿರಾಂಕ Dfg_ದತ್ತಾಂಶ_Type& dtype, Operands... operands) {
    static_assert(std::is_final<ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<ಶೃಂಗ_2>::ಮೌಲ್ಯ, "Not a cached vertex type");
    ಹಿಂತಿರುಗಿ cacheForType<ಶೃಂಗ_2>()->ಮಾದರಿ get<ಶೃಂಗ_2>(dtype, operands...);
}
}
;

#ಆದರೆ_ಅಂತ್ಯ  // VERILATOR_V3DFGCACHE_H_
