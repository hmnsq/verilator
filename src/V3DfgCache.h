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

#నిర్వచితం_కాకపోతే VERILATOR_V3DFGCACHE_H_
#నిర్వచించు VERILATOR_V3DFGCACHE_H_

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgDataType.h"
#అన్తర్భావ "V3HashTable.h"

#అన్తర్భావ <type_traits>

// Type predicate true for cached vertex types
మూస <typename శీర్షం_2>
using V3DfgCacheIsCached
    = std::integral_constant<bool, std::is_పునాది_యొక్క<Dfg_శీర్షం_Unary, శీర్షం_2>::విలువ
                                       || std::is_పునాది_యొక్క<Dfg_శీర్షం_ద్విమాన, శీర్షం_2>::విలువ
                                       || std::is_పునాది_యొక్క<Dfg_శీర్షం_Ternary, శీర్షం_2>::విలువ>;

// Helper template to determine the cache type for a vertex type
మూస <typename శీర్షం_2, typename Cache_పునాది, typename... జతలు>
నిర్మాణం V3DfgCacheType final {
    using Type = Cache_పునాది;
};

మూస <typename శీర్షం_2, typename Cache_పునాది, typename శీర్షం_పునాది, typename Cache,
          typename... జతలు>
నిర్మాణం V3DfgCacheType<శీర్షం_2, Cache_పునాది, శీర్షం_పునాది, Cache, జతలు...> final {
    using Type = std::conditional_t<std::is_పునాది_యొక్క<శీర్షం_పునాది, శీర్షం_2>::విలువ, Cache,
                                    typename V3DfgCacheType<శీర్షం_2, Cache_పునాది, జతలు...>::Type>;
};

class V3DfgCache final {
    // TYPES
    // Hashing and comparison of the cached vertices. Each takes either a vertex, or the
    // parts a vertex would be created from, so a lookup needs no vertex and no key object.

    // DfgSel
    నిర్మాణం HashSel final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి DfgSel* vtxp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(vtxp->dtype(), vtxp->fromp(), vtxp->lsb());
        }
        కొలత_t క్రియాకారకం()(స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, స్థిరరాశి Dfg_శీర్షం* fromp, uint32_t lsb) స్థిరరాశి {
            // cppcheck-suppress unreadVariable  // cppcheck bug
            V3Hash hash = dtype.hash();
            hash += శీర్షం_Hash(fromp);
            hash += lsb;
            తిరిగి hash.విలువ();
        }
    };
    నిర్మాణం EqualSel final {
        bool క్రియాకారకం()(స్థిరరాశి DfgSel* ap, స్థిరరాశి DfgSel* bp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(ap, bp->dtype(), bp->fromp(), bp->lsb());
        }
        bool క్రియాకారకం()(స్థిరరాశి DfgSel* vtxp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, స్థిరరాశి Dfg_శీర్షం* fromp,
                        uint32_t lsb) స్థిరరాశి {
            తిరిగి vtxp->lsb() == lsb && vtxp->dtype() == dtype
                   && శీర్షం_Equal(vtxp->fromp(), fromp);
        }
    };

    // DfgVertexUnary
    నిర్మాణం HashUnary final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_Unary* vtxp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(vtxp->dtype(), vtxp->inputp(0));
        }
        కొలత_t క్రియాకారకం()(స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, స్థిరరాశి Dfg_శీర్షం* source0p) స్థిరరాశి {
            V3Hash hash = dtype.hash();
            hash += శీర్షం_Hash(source0p);
            తిరిగి hash.విలువ();
        }
    };
    నిర్మాణం EqualUnary final {
        bool క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_Unary* ap, స్థిరరాశి Dfg_శీర్షం_Unary* bp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(ap, bp->dtype(), bp->inputp(0));
        }
        bool క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_Unary* vtxp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype,
                        స్థిరరాశి Dfg_శీర్షం* source0p) స్థిరరాశి {
            తిరిగి vtxp->dtype() == dtype && శీర్షం_Equal(vtxp->inputp(0), source0p);
        }
    };

    // DfgVertexBinary
    నిర్మాణం Hash_ద్విమాన final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_ద్విమాన* vtxp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1));
        }
        కొలత_t క్రియాకారకం()(స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, స్థిరరాశి Dfg_శీర్షం* source0p,
                          స్థిరరాశి Dfg_శీర్షం* source1p) స్థిరరాశి {
            V3Hash hash = dtype.hash();
            hash += శీర్షం_Hash(source0p);
            hash += శీర్షం_Hash(source1p);
            తిరిగి hash.విలువ();
        }
    };
    నిర్మాణం Equal_ద్విమాన final {
        bool క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_ద్విమాన* ap, స్థిరరాశి Dfg_శీర్షం_ద్విమాన* bp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1));
        }
        bool క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_ద్విమాన* vtxp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype,
                        స్థిరరాశి Dfg_శీర్షం* source0p, స్థిరరాశి Dfg_శీర్షం* source1p) స్థిరరాశి {
            తిరిగి vtxp->dtype() == dtype && శీర్షం_Equal(vtxp->inputp(0), source0p)
                   && శీర్షం_Equal(vtxp->inputp(1), source1p);
        }
    };

    // DfgVertexTernary
    నిర్మాణం HashTernary final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_Ternary* vtxp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(vtxp->dtype(), vtxp->inputp(0), vtxp->inputp(1), vtxp->inputp(2));
        }
        కొలత_t క్రియాకారకం()(స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, స్థిరరాశి Dfg_శీర్షం* source0p,
                          స్థిరరాశి Dfg_శీర్షం* source1p, స్థిరరాశి Dfg_శీర్షం* source2p) స్థిరరాశి {
            V3Hash hash = dtype.hash();
            hash += శీర్షం_Hash(source0p);
            hash += శీర్షం_Hash(source1p);
            hash += శీర్షం_Hash(source2p);
            తిరిగి hash.విలువ();
        }
    };

    నిర్మాణం EqualTernary final {
        bool క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_Ternary* ap, స్థిరరాశి Dfg_శీర్షం_Ternary* bp) స్థిరరాశి {
            తిరిగి క్రియాకారకం()(ap, bp->dtype(), bp->inputp(0), bp->inputp(1), bp->inputp(2));
        }
        bool క్రియాకారకం()(స్థిరరాశి Dfg_శీర్షం_Ternary* vtxp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype,
                        స్థిరరాశి Dfg_శీర్షం* source0p, స్థిరరాశి Dfg_శీర్షం* source1p,
                        స్థిరరాశి Dfg_శీర్షం* source2p) స్థిరరాశి {
            తిరిగి vtxp->dtype() == dtype && శీర్షం_Equal(vtxp->inputp(0), source0p)
                   && శీర్షం_Equal(vtxp->inputp(1), source1p)
                   && శీర్షం_Equal(vtxp->inputp(2), source2p);
        }
    };

    // Base class of vertex caches
    class Cache_పునాది VL_NOT_FINAL{రక్షిత :
                                     // These set the operands of a new vertex
                                     స్థిర శూన్య సమితి_Operands(DfgSel * vtxp, Dfg_శీర్షం* fromp,
                                                             uint32_t lsb){vtxp->fromp(fromp);
    vtxp->lsb(lsb);
}

స్థిర శూన్య
సమితి_Operands(Dfg_శీర్షం_Unary* vtxp, Dfg_శీర్షం* src0p) {  //
    vtxp->inputp(0, src0p);
}

స్థిర శూన్య సమితి_Operands(Dfg_శీర్షం_ద్విమాన* vtxp, Dfg_శీర్షం* src0p, Dfg_శీర్షం* src1p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
}

స్థిర శూన్య సమితి_Operands(Dfg_శీర్షం_Ternary* vtxp, Dfg_శీర్షం* src0p, Dfg_శీర్షం* src1p,
                        Dfg_శీర్షం* src2p) {
    vtxp->inputp(0, src0p);
    vtxp->inputp(1, src1p);
    vtxp->inputp(2, src2p);
}

బహిరంగ:
// CacheBase does not cache anything
మాయా Dfg_శీర్షం* cache(Dfg_శీర్షం*) { తిరిగి nullptr; }
మాయా శూన్య invalidate(Dfg_శీర్షం*) {}
}
;

మూస <typename T_శీర్షం, typename T_Hash, typename T_Equal>
class Cache final : బహిరంగ Cache_పునాది {
    static_assert(std::is_పునాది_యొక్క<Dfg_శీర్షం, T_శీర్షం>::విలువ, "T_Vertex must be a DfgVertex");

    // STATE
    V3_Hash_సమితి<T_శీర్షం*, T_Hash, T_Equal> m_సమితి;

బహిరంగ:
    // Add an existing vertex to the cache. If an equivalent but different vertex exists,
    // it is returned and the cache is not updated. Returns nullptr if the vertex is inserted.
    Dfg_శీర్షం* cache(Dfg_శీర్షం* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_శీర్షం>(), vtxp, "Vertex is wrong type"););
        T_శీర్షం* స్థిరరాశి typedp = static_cast<T_శీర్షం*>(vtxp);
        T_శీర్షం* స్థిరరాశి cachedp = *m_సమితి.చొప్పించు(typedp).మొదటి;
        తిరిగి cachedp != vtxp ? cachedp : nullptr;
    }
    // Remove an existing vertex from the cache, if it is the cached vertex, otherwise no-op
    శూన్య invalidate(Dfg_శీర్షం* vtxp) override {
        UDEBUGONLY(UASSERT_OBJ(vtxp->is<T_శీర్షం>(), vtxp, "Vertex is wrong type"););
        T_శీర్షం* స్థిరరాశి typedp = static_cast<T_శీర్షం*>(vtxp);
        స్థిరరాశి స్వయంచాలక it = m_సమితి.కనుగొను(typedp);
        ఒకవేళ (it != m_సమితి.చివర() && *it == typedp) m_సమితి.erase(it);
    }
    // Get vertex with given operands, return nullptr if not in cache
    మూస <typename శీర్షం_2, typename... Operands>
    శీర్షం_2* get(స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, Operands... operands) {
        స్థిరరాశి స్వయంచాలక it = m_సమితి.కనుగొను(dtype, operands...);
        తిరిగి it != m_సమితి.చివర() ? static_cast<శీర్షం_2*>(*it) : nullptr;
    }
    // Get vertex with given operands, if does not exist, create it
    మూస <typename శీర్షం_2, typename... Operands>
    శీర్షం_2* getOrCreate(Dfg_లేఖాచిత్రం& dfg, File_పంక్తి* flp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype,
                        Operands... operands) {
        స్థిరరాశి స్వయంచాలక pair = m_సమితి.చొప్పించు_Lazy(dtype, operands..., [&]() -> T_శీర్షం* {
            శీర్షం_2* స్థిరరాశి newp = new శీర్షం_2{dfg, flp, dtype};
            సమితి_Operands(newp, operands...);
            తిరిగి newp;
        });
        T_శీర్షం* స్థిరరాశి vtxp = *pair.మొదటి;
        UDEBUGONLY(UASSERT_OBJ(vtxp->మూస is<శీర్షం_2>(), vtxp, "Vertex is wrong type"););
        తిరిగి static_cast<శీర్షం_2*>(vtxp);
    }
};

// Map from Vertex type to cache type
// clang-format off
    మూస <typename శీర్షం_2>
    using CacheType = typename V3DfgCacheType<శీర్షం_2, Cache_పునాది,
        DfgSel,           /* -> */  Cache<DfgSel, HashSel, EqualSel>,
        Dfg_శీర్షం_Unary,   /* -> */  Cache<Dfg_శీర్షం_Unary, HashUnary, EqualUnary>,
        Dfg_శీర్షం_ద్విమాన,  /* -> */  Cache<Dfg_శీర్షం_ద్విమాన, Hash_ద్విమాన, Equal_ద్విమాన>,
        Dfg_శీర్షం_Ternary, /* -> */  Cache<Dfg_శీర్షం_Ternary, HashTernary, EqualTernary>
    >::Type;
// clang-format on

// STATE
Dfg_లేఖాచిత్రం& m_dfg;  // The DfgGraph we are caching the vertices of

// The per type caches
#నిర్వచించు శీర్షం_CACHE_DECLARE_CACHE(t) CacheType<t> m_cache##t;
FOREACH_DFG_శీర్షం_TYPE(శీర్షం_CACHE_DECLARE_CACHE)
#నిర్వచనం_రద్దు శీర్షం_CACHE_DECLARE_CACHE

// Map from vertex type to m_cache* instances for dynamic lookup
std::array<Cache_పునాది*, VDfgType::అంకె_TYPES()> m_vtxType2Cachep{};

// METHODS

// Map from vertex type to m_cache* instances for static lookup
మూస <typename శీర్షం_2>
CacheType<శీర్షం_2>* cacheForType() {
#నిర్వచించు శీర్షం_CACHE_DECLARE_CACHE(t) \
    ఒకవేళ VL_CONSTEXPR_CXX17 (std::is_same<శీర్షం_2, t>::విలువ) \
        తిరిగి reinterpret_cast<CacheType<శీర్షం_2>*>(&m_cache##t);
    FOREACH_DFG_శీర్షం_TYPE(శీర్షం_CACHE_DECLARE_CACHE)
#నిర్వచనం_రద్దు శీర్షం_CACHE_DECLARE_CACHE
    తిరిగి nullptr;  // LCOV_EXCL_LINE
}

// Hash constants by value, everything else by identity
స్థిర V3Hash శీర్షం_Hash(స్థిరరాశి Dfg_శీర్షం* vtxp) {
    ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి constp = vtxp->cast<DfgConst>()) తిరిగి constp->అంకె().గమ్యం_Hash();
    తిరిగి V3Hash{reinterpret_cast<uint64_t>(vtxp)};
}

// Constants are equal by value, everything else is equal by identity
స్థిర bool శీర్షం_Equal(స్థిరరాశి Dfg_శీర్షం* ap, స్థిరరాశి Dfg_శీర్షం* bp) {
    ఒకవేళ (ap == bp) తిరిగి true;
    ఒకవేళ (ap->type() != bp->type()) తిరిగి false;
    ఒకవేళ (స్థిరరాశి DfgConst* స్థిరరాశి aConstp = ap->cast<DfgConst>()) {
        స్థిరరాశి DfgConst* స్థిరరాశి bConstp = bp->as<DfgConst>();
        తిరిగి aConstp->అంకె().isCaseEq(bConstp->అంకె());
    }
    తిరిగి false;
}

బహిరంగ:
// Note: the cache starts out empty. If the caller wants existing vertices
// to be found, it must add them itself by calling 'cache' on each.
explicit V3DfgCache(Dfg_లేఖాచిత్రం& dfg)
    : m_dfg{dfg} {
    // Initialize the type to cache lookup table
#నిర్వచించు శీర్షం_CACHE_DECLARE_CACHE_PTR(t) m_vtxType2Cachep[t::dfgType()] = &m_cache##t;
    FOREACH_DFG_శీర్షం_TYPE(శీర్షం_CACHE_DECLARE_CACHE_PTR)
#నిర్వచనం_రద్దు శీర్షం_CACHE_DECLARE_CACHE_PTR
}

// Add an existing vertex to the cache. If an equivalent (but different) already exists,
// it is returned and the cache is not updated.
Dfg_శీర్షం* cache(Dfg_శీర్షం* vtxp) { తిరిగి m_vtxType2Cachep[vtxp->type()]->cache(vtxp); }

// Remove an exiting vertex, it is the cached vertex.
శూన్య invalidate(Dfg_శీర్షం* vtxp) { m_vtxType2Cachep[vtxp->type()]->invalidate(vtxp); }

// Find a vertex of type 'Vertex', with the given operands, or create a new one and add it.
మూస <typename శీర్షం_2, typename... Operands>
శీర్షం_2* getOrCreate(File_పంక్తి* flp, స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, Operands... operands) {
    static_assert(std::is_final<శీర్షం_2>::విలువ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<శీర్షం_2>::విలువ, "Not a cached vertex type");
    తిరిగి cacheForType<శీర్షం_2>()->మూస getOrCreate<శీర్షం_2>(m_dfg, flp, dtype, operands...);
}

// Find a vertex of type 'Vertex', with the given operands, return nullptr if not in cache.
మూస <typename శీర్షం_2, typename... Operands>
శీర్షం_2* get(స్థిరరాశి Dfg_దత్తాంశం_Type& dtype, Operands... operands) {
    static_assert(std::is_final<శీర్షం_2>::విలువ, "Must invoke on final vertex type");
    static_assert(V3DfgCacheIsCached<శీర్షం_2>::విలువ, "Not a cached vertex type");
    తిరిగి cacheForType<శీర్షం_2>()->మూస get<శీర్షం_2>(dtype, operands...);
}
}
;

#అయితే_ముగింపు  // VERILATOR_V3DFGCACHE_H_
