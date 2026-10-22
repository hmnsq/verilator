// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: DfgGraph common sub-expression elimination (CSE)
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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
صنف DfgCseHash final {
    // STATE
    بدلجندڙ DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

عام:
    // CONSTRUCTOR
    واضح DfgCseHash(Dfg_گراف& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        لكل (ثابت Dfg_راس_Var& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        لكل (ثابت Dfg_راس_Ast& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        لكل (ثابت Dfg_راس& vtx : dfg.opVertices()) {
            إذا (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        لكل (ثابت DfgConst& vtx : dfg.constVertices()) {
            ثابت V3Hash hash = vtx.انگ().منزل_Hash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.قدر()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    سائيز_t عامل()(Dfg_راس* vtxp) ثابت { أرجع راس_Hash(*vtxp).قدر(); }

خاص:
    // Returns hash of vertex dependent on information internal to the vertex
    ساكن V3Hash راس_Self_Hash(ثابت Dfg_راس& vtx) {
        بدل (vtx.type()) {
        // Unhandled vertices
        حالة VDfgType::Logic:  // LCOV_EXCL_START
        حالة VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        حالة VDfgType::Const:  // LCOV_EXCL_START
        حالة VDfgType::CReset:
        حالة VDfgType::VarArray:
        حالة VDfgType::VarPacked:
        حالة VDfgType::Prev:
        حالة VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        حالة VDfgType::Sel: أرجع V3Hash{vtx.as<DfgSel>()->lsb()};

        حالة VDfgType::SpliceArray:
        حالة VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<Dfg_راس_Splice>()->foreachDriver([&](ثابت Dfg_راس&, uint32_t lo) {
                hash += lo;
                أرجع خطأ;
            });
            أرجع hash;
        }

        // Vertices with no internal information
        حالة VDfgType::ميلاپ_Masked:
        حالة VDfgType::Mux:
        حالة VDfgType::UnitArray: أرجع V3Hash{};

        // Generated classes - none of them have internal information
        حالة VDfgType::جوڙيو_2:
        حالة VDfgType::And:
        حالة VDfgType::ArraySel:
        حالة VDfgType::Concat:
        حالة VDfgType::Cond:
        حالة VDfgType::ڳڻپ_Ones_2:
        حالة VDfgType::Div:
        حالة VDfgType::DivS:
        حالة VDfgType::Eq:
        حالة VDfgType::EqCase:
        حالة VDfgType::EqWild:
        حالة VDfgType::Extend:
        حالة VDfgType::ExtendS:
        حالة VDfgType::Gt:
        حالة VDfgType::GtS:
        حالة VDfgType::Gte:
        حالة VDfgType::GteS:
        حالة VDfgType::LogAnd:
        حالة VDfgType::LogEq:
        حالة VDfgType::LogIf:
        حالة VDfgType::LogNot:
        حالة VDfgType::LogOr:
        حالة VDfgType::Lt:
        حالة VDfgType::LtS:
        حالة VDfgType::Lte:
        حالة VDfgType::LteS:
        حالة VDfgType::ModDiv:
        حالة VDfgType::ModDivS:
        حالة VDfgType::Mul:
        حالة VDfgType::MulS:
        حالة VDfgType::Negate:
        حالة VDfgType::Neq:
        حالة VDfgType::NeqCase:
        حالة VDfgType::NeqWild:
        حالة VDfgType::Not:
        حالة VDfgType::OneHot:
        حالة VDfgType::OneHot0:
        حالة VDfgType::Or:
        حالة VDfgType::Pow:
        حالة VDfgType::PowSS:
        حالة VDfgType::PowSU:
        حالة VDfgType::PowUS:
        حالة VDfgType::RedAnd:
        حالة VDfgType::RedOr:
        حالة VDfgType::RedXor:
        حالة VDfgType::Rep:
        حالة VDfgType::سرڪايو_L:
        حالة VDfgType::سرڪايو_R:
        حالة VDfgType::سرڪايو_RS:
        حالة VDfgType::وهڪرو_L:
        حالة VDfgType::وهڪرو_R:
        حالة VDfgType::Sub:
        حالة VDfgType::Xor: أرجع V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash راس_Hash(Dfg_راس& vtx) ثابت {
        V3Hash& نتيجو = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        إذا (!نتيجو.قدر()) {
            V3Hash hash{راس_Self_Hash(vtx)};
            hash += vtx.type();
            hash += vtx.سائيز();
            vtx.foreach_ذريعو([&](Dfg_راس& src) {
                hash += راس_Hash(src);  // Graph is acyclic, so this terminates
                أرجع خطأ;
            });
            نتيجو = hash;
        }
        أرجع نتيجو;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
صنف DfgCseEqual final {
    // TYPES
    استعمال راس_Pair = std::pair<ثابت Dfg_راس*, ثابت Dfg_راس*>;
    بنية راس_Pair_Hash final {
        سائيز_t عامل()(ثابت راس_Pair& pair) ثابت {
            V3Hash hash;
            hash += pair.پهريون;
            hash += pair.ٻيو;
            أرجع hash.قدر();
        }
    };

    // STATE
    بدلجندڙ V3HashMap<راس_Pair, منطقي, راس_Pair_Hash> m_cache;  // Cache for vertex equality
    بدلجندڙ std::ويڪٽر<uint32_t> m_driverLo;  // Low indices of drivers
    ثابت سائيز_t m_سائيز;  // Size of the graph

عام:
    // CONSTRUCTORS
    واضح DfgCseEqual(ثابت Dfg_گراف& dfg)
        : m_سائيز{dfg.سائيز()} {}

    // METHODS
    منطقي عامل()(Dfg_راس* ap, Dfg_راس* bp) ثابت { أرجع راس_Equivalent(*ap, *bp); }

خاص:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    منطقي راس_Self_Equivalent(ثابت Dfg_راس& a, ثابت Dfg_راس& b) ثابت {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        بدل (a.type()) {
        // Unhandled vertices
        حالة VDfgType::Logic:  // LCOV_EXCL_START
        حالة VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        حالة VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        حالة VDfgType::Const: أرجع a.as<DfgConst>()->انگ().isCaseEq(b.as<DfgConst>()->انگ());
        حالة VDfgType::CReset: أرجع خطأ;
        حالة VDfgType::Prev: أرجع خطأ;

        حالة VDfgType::VarArray:
        حالة VDfgType::VarPacked:  // CSE does not combine variables
            أرجع خطأ;

        // Vertices with internal information
        حالة VDfgType::Sel: أرجع a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        حالة VDfgType::SpliceArray:
        حالة VDfgType::SplicePacked: {
            ثابت Dfg_راس_Splice* ثابت ap = a.as<Dfg_راس_Splice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ثابت Dfg_راس&, uint32_t lo) {
                m_driverLo.ڌڪو_پٺ(lo);
                أرجع خطأ;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.معلومات();
            أرجع !b.as<Dfg_راس_Splice>()->foreachDriver([&](ثابت Dfg_راس&, uint32_t lo) {  //
                أرجع *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        حالة VDfgType::ميلاپ_Masked:
        حالة VDfgType::Mux:
        حالة VDfgType::UnitArray: أرجع سچ;

        // Generated classes - none of them have internal information
        حالة VDfgType::جوڙيو_2:
        حالة VDfgType::And:
        حالة VDfgType::ArraySel:
        حالة VDfgType::Concat:
        حالة VDfgType::Cond:
        حالة VDfgType::ڳڻپ_Ones_2:
        حالة VDfgType::Div:
        حالة VDfgType::DivS:
        حالة VDfgType::Eq:
        حالة VDfgType::EqCase:
        حالة VDfgType::EqWild:
        حالة VDfgType::Extend:
        حالة VDfgType::ExtendS:
        حالة VDfgType::Gt:
        حالة VDfgType::GtS:
        حالة VDfgType::Gte:
        حالة VDfgType::GteS:
        حالة VDfgType::LogAnd:
        حالة VDfgType::LogEq:
        حالة VDfgType::LogIf:
        حالة VDfgType::LogNot:
        حالة VDfgType::LogOr:
        حالة VDfgType::Lt:
        حالة VDfgType::LtS:
        حالة VDfgType::Lte:
        حالة VDfgType::LteS:
        حالة VDfgType::ModDiv:
        حالة VDfgType::ModDivS:
        حالة VDfgType::Mul:
        حالة VDfgType::MulS:
        حالة VDfgType::Negate:
        حالة VDfgType::Neq:
        حالة VDfgType::NeqCase:
        حالة VDfgType::NeqWild:
        حالة VDfgType::Not:
        حالة VDfgType::OneHot:
        حالة VDfgType::OneHot0:
        حالة VDfgType::Or:
        حالة VDfgType::Pow:
        حالة VDfgType::PowSS:
        حالة VDfgType::PowSU:
        حالة VDfgType::PowUS:
        حالة VDfgType::RedAnd:
        حالة VDfgType::RedOr:
        حالة VDfgType::RedXor:
        حالة VDfgType::Rep:
        حالة VDfgType::سرڪايو_L:
        حالة VDfgType::سرڪايو_R:
        حالة VDfgType::سرڪايو_RS:
        حالة VDfgType::وهڪرو_L:
        حالة VDfgType::وهڪرو_R:
        حالة VDfgType::Sub:
        حالة VDfgType::Xor: أرجع سچ;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    منطقي sourcesEquivalent(ثابت Dfg_راس& a, ثابت Dfg_راس& b) ثابت {
        لكل (سائيز_t i = 0; i < a.nInputs(); ++i) {
            ثابت Dfg_راس* ثابت ap = a.inputp(i);
            ثابت Dfg_راس* ثابت bp = b.inputp(i);
            إذا (!ap && !bp) استمر;
            إذا (!ap || !bp) أرجع خطأ;
            إذا (!راس_Equivalent(*ap, *bp)) أرجع خطأ;  // Graph is acyclic, so this terminates
        }
        أرجع سچ;
    }

    // Compares 'a' and 'b' for equivalence
    منطقي راس_Equivalent(ثابت Dfg_راس& a, ثابت Dfg_راس& b) ثابت {
        // If same vertex, then equal
        إذا (&a == &b) أرجع سچ;

        // If different type, then not equal
        إذا (a.type() != b.type()) أرجع خطأ;

        // If different data type, then not equal
        إذا (a.dtype() != b.dtype()) أرجع خطأ;

        // If different number of inputs, then not equal
        إذا (a.nInputs() != b.nInputs()) أرجع خطأ;

        // Check vertex specifics
        إذا (!راس_Self_Equivalent(a, b)) أرجع خطأ;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        إذا (!a.has_گڻو_Sinks() && !b.has_گڻو_Sinks()) أرجع sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ثابت راس_Pair ڪنجي = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ثابت خودڪار it = m_cache.ڳوليو(ڪنجي);
        إذا (it != m_cache.پڇاڙي()) أرجع it->ٻيو;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ثابت منطقي equal = sourcesEquivalent(a, b);
        إذا (VL_UNLIKELY(m_cache.خالي_جايون())) m_cache.reserve(m_سائيز / 4);
        m_cache.وجهو({ڪنجي, equal});

        // The predicate result
        أرجع equal;
    }
};

// Combine equivalent operation vertices
فراغ dfgCseCombineEquivalent(Dfg_گراف& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    لكل (DfgConst* ثابت vtxp : dfg.constVertices().unlinkable()) {
        إذا (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3_Hash_سيٽ<Dfg_راس*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.سائيز());

    // Combine operation vertices
    لكل (Dfg_راس* ثابت vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        إذا (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            استمر;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ثابت خودڪار pair = uniqueVtxps.وجهو(vtxp);
        إذا (!pair.ٻيو) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.پهريون);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

فراغ V3DfgPasses::cse(Dfg_گراف& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::remove_اڻ_استعمال_ٿيل(dfg);
}
