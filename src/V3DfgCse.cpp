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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
ਵਰਗ DfgCseHash final {
    // STATE
    ਬਦਲਣਯੋਗ DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

ਜਨਤਕ:
    // CONSTRUCTOR
    ਸਪਸ਼ਟ DfgCseHash(Dfg_ਗ੍ਰਾਫ& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Var& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Ast& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        ਲਈ (ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx : dfg.opVertices()) {
            ਜੇ (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        ਲਈ (ਅਚਲ DfgConst& vtx : dfg.constVertices()) {
            ਅਚਲ V3Hash hash = vtx.ਅੰਕ().ਮੰਜ਼ਿਲ_Hash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.ਮੁੱਲ()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    ਮਾਪ_t ਸੰਚਾਲਕ()(Dfg_ਸ਼ੀਰਸ਼* vtxp) ਅਚਲ { ਵਾਪਸ ਸ਼ੀਰਸ਼_Hash(*vtxp).ਮੁੱਲ(); }

ਨਿਜੀ:
    // Returns hash of vertex dependent on information internal to the vertex
    ਸਥਿਰ V3Hash ਸ਼ੀਰਸ਼_Self_Hash(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
        ਚੋਣ (vtx.type()) {
        // Unhandled vertices
        ਹਾਲਤ VDfgType::Logic:  // LCOV_EXCL_START
        ਹਾਲਤ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        ਹਾਲਤ VDfgType::Const:  // LCOV_EXCL_START
        ਹਾਲਤ VDfgType::CReset:
        ਹਾਲਤ VDfgType::VarArray:
        ਹਾਲਤ VDfgType::VarPacked:
        ਹਾਲਤ VDfgType::Prev:
        ਹਾਲਤ VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        ਹਾਲਤ VDfgType::Sel: ਵਾਪਸ V3Hash{vtx.as<DfgSel>()->lsb()};

        ਹਾਲਤ VDfgType::SpliceArray:
        ਹਾਲਤ VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<Dfg_ਸ਼ੀਰਸ਼_Splice>()->foreachDriver([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&, uint32_t lo) {
                hash += lo;
                ਵਾਪਸ ਝੂਠ;
            });
            ਵਾਪਸ hash;
        }

        // Vertices with no internal information
        ਹਾਲਤ VDfgType::ਮੇਲ_Masked:
        ਹਾਲਤ VDfgType::Mux:
        ਹਾਲਤ VDfgType::UnitArray: ਵਾਪਸ V3Hash{};

        // Generated classes - none of them have internal information
        ਹਾਲਤ VDfgType::ਜੋੜੋ_2:
        ਹਾਲਤ VDfgType::And:
        ਹਾਲਤ VDfgType::ArraySel:
        ਹਾਲਤ VDfgType::Concat:
        ਹਾਲਤ VDfgType::Cond:
        ਹਾਲਤ VDfgType::ਗਿਣਤੀ_Ones_2:
        ਹਾਲਤ VDfgType::Div:
        ਹਾਲਤ VDfgType::DivS:
        ਹਾਲਤ VDfgType::Eq:
        ਹਾਲਤ VDfgType::EqCase:
        ਹਾਲਤ VDfgType::EqWild:
        ਹਾਲਤ VDfgType::Extend:
        ਹਾਲਤ VDfgType::ExtendS:
        ਹਾਲਤ VDfgType::Gt:
        ਹਾਲਤ VDfgType::GtS:
        ਹਾਲਤ VDfgType::Gte:
        ਹਾਲਤ VDfgType::GteS:
        ਹਾਲਤ VDfgType::LogAnd:
        ਹਾਲਤ VDfgType::LogEq:
        ਹਾਲਤ VDfgType::LogIf:
        ਹਾਲਤ VDfgType::LogNot:
        ਹਾਲਤ VDfgType::LogOr:
        ਹਾਲਤ VDfgType::Lt:
        ਹਾਲਤ VDfgType::LtS:
        ਹਾਲਤ VDfgType::Lte:
        ਹਾਲਤ VDfgType::LteS:
        ਹਾਲਤ VDfgType::ModDiv:
        ਹਾਲਤ VDfgType::ModDivS:
        ਹਾਲਤ VDfgType::Mul:
        ਹਾਲਤ VDfgType::MulS:
        ਹਾਲਤ VDfgType::Negate:
        ਹਾਲਤ VDfgType::Neq:
        ਹਾਲਤ VDfgType::NeqCase:
        ਹਾਲਤ VDfgType::NeqWild:
        ਹਾਲਤ VDfgType::Not:
        ਹਾਲਤ VDfgType::OneHot:
        ਹਾਲਤ VDfgType::OneHot0:
        ਹਾਲਤ VDfgType::Or:
        ਹਾਲਤ VDfgType::Pow:
        ਹਾਲਤ VDfgType::PowSS:
        ਹਾਲਤ VDfgType::PowSU:
        ਹਾਲਤ VDfgType::PowUS:
        ਹਾਲਤ VDfgType::RedAnd:
        ਹਾਲਤ VDfgType::RedOr:
        ਹਾਲਤ VDfgType::RedXor:
        ਹਾਲਤ VDfgType::Rep:
        ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_L:
        ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_R:
        ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_RS:
        ਹਾਲਤ VDfgType::ਧਾਰਾ_L:
        ਹਾਲਤ VDfgType::ਧਾਰਾ_R:
        ਹਾਲਤ VDfgType::Sub:
        ਹਾਲਤ VDfgType::Xor: ਵਾਪਸ V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash ਸ਼ੀਰਸ਼_Hash(Dfg_ਸ਼ੀਰਸ਼& vtx) ਅਚਲ {
        V3Hash& ਨਤੀਜਾ = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        ਜੇ (!ਨਤੀਜਾ.ਮੁੱਲ()) {
            V3Hash hash{ਸ਼ੀਰਸ਼_Self_Hash(vtx)};
            hash += vtx.type();
            hash += vtx.ਮਾਪ();
            vtx.foreach_ਸਰੋਤ([&](Dfg_ਸ਼ੀਰਸ਼& src) {
                hash += ਸ਼ੀਰਸ਼_Hash(src);  // Graph is acyclic, so this terminates
                ਵਾਪਸ ਝੂਠ;
            });
            ਨਤੀਜਾ = hash;
        }
        ਵਾਪਸ ਨਤੀਜਾ;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
ਵਰਗ DfgCseEqual final {
    // TYPES
    ਵਰਤੋਂ ਸ਼ੀਰਸ਼_Pair = std::pair<ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼*>;
    ਬਣਤਰ ਸ਼ੀਰਸ਼_Pair_Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ ਸ਼ੀਰਸ਼_Pair& pair) ਅਚਲ {
            V3Hash hash;
            hash += pair.ਪਹਿਲਾ;
            hash += pair.ਦੂਜਾ;
            ਵਾਪਸ hash.ਮੁੱਲ();
        }
    };

    // STATE
    ਬਦਲਣਯੋਗ V3HashMap<ਸ਼ੀਰਸ਼_Pair, ਤਾਰਕਿਕ, ਸ਼ੀਰਸ਼_Pair_Hash> m_cache;  // Cache for vertex equality
    ਬਦਲਣਯੋਗ std::ਸਦਿਸ਼<uint32_t> m_driverLo;  // Low indices of drivers
    ਅਚਲ ਮਾਪ_t m_ਮਾਪ;  // Size of the graph

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ DfgCseEqual(ਅਚਲ Dfg_ਗ੍ਰਾਫ& dfg)
        : m_ਮਾਪ{dfg.ਮਾਪ()} {}

    // METHODS
    ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(Dfg_ਸ਼ੀਰਸ਼* ap, Dfg_ਸ਼ੀਰਸ਼* bp) ਅਚਲ { ਵਾਪਸ ਸ਼ੀਰਸ਼_Equivalent(*ap, *bp); }

ਨਿਜੀ:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    ਤਾਰਕਿਕ ਸ਼ੀਰਸ਼_Self_Equivalent(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& a, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& b) ਅਚਲ {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        ਚੋਣ (a.type()) {
        // Unhandled vertices
        ਹਾਲਤ VDfgType::Logic:  // LCOV_EXCL_START
        ਹਾਲਤ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        ਹਾਲਤ VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        ਹਾਲਤ VDfgType::Const: ਵਾਪਸ a.as<DfgConst>()->ਅੰਕ().isCaseEq(b.as<DfgConst>()->ਅੰਕ());
        ਹਾਲਤ VDfgType::CReset: ਵਾਪਸ ਝੂਠ;
        ਹਾਲਤ VDfgType::Prev: ਵਾਪਸ ਝੂਠ;

        ਹਾਲਤ VDfgType::VarArray:
        ਹਾਲਤ VDfgType::VarPacked:  // CSE does not combine variables
            ਵਾਪਸ ਝੂਠ;

        // Vertices with internal information
        ਹਾਲਤ VDfgType::Sel: ਵਾਪਸ a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        ਹਾਲਤ VDfgType::SpliceArray:
        ਹਾਲਤ VDfgType::SplicePacked: {
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼_Splice* ਅਚਲ ap = a.as<Dfg_ਸ਼ੀਰਸ਼_Splice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&, uint32_t lo) {
                m_driverLo.ਧੱਕੋ_ਪਿੱਠ(lo);
                ਵਾਪਸ ਝੂਠ;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.ਅੰਕੜੇ();
            ਵਾਪਸ !b.as<Dfg_ਸ਼ੀਰਸ਼_Splice>()->foreachDriver([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼&, uint32_t lo) {  //
                ਵਾਪਸ *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        ਹਾਲਤ VDfgType::ਮੇਲ_Masked:
        ਹਾਲਤ VDfgType::Mux:
        ਹਾਲਤ VDfgType::UnitArray: ਵਾਪਸ ਸੱਚ;

        // Generated classes - none of them have internal information
        ਹਾਲਤ VDfgType::ਜੋੜੋ_2:
        ਹਾਲਤ VDfgType::And:
        ਹਾਲਤ VDfgType::ArraySel:
        ਹਾਲਤ VDfgType::Concat:
        ਹਾਲਤ VDfgType::Cond:
        ਹਾਲਤ VDfgType::ਗਿਣਤੀ_Ones_2:
        ਹਾਲਤ VDfgType::Div:
        ਹਾਲਤ VDfgType::DivS:
        ਹਾਲਤ VDfgType::Eq:
        ਹਾਲਤ VDfgType::EqCase:
        ਹਾਲਤ VDfgType::EqWild:
        ਹਾਲਤ VDfgType::Extend:
        ਹਾਲਤ VDfgType::ExtendS:
        ਹਾਲਤ VDfgType::Gt:
        ਹਾਲਤ VDfgType::GtS:
        ਹਾਲਤ VDfgType::Gte:
        ਹਾਲਤ VDfgType::GteS:
        ਹਾਲਤ VDfgType::LogAnd:
        ਹਾਲਤ VDfgType::LogEq:
        ਹਾਲਤ VDfgType::LogIf:
        ਹਾਲਤ VDfgType::LogNot:
        ਹਾਲਤ VDfgType::LogOr:
        ਹਾਲਤ VDfgType::Lt:
        ਹਾਲਤ VDfgType::LtS:
        ਹਾਲਤ VDfgType::Lte:
        ਹਾਲਤ VDfgType::LteS:
        ਹਾਲਤ VDfgType::ModDiv:
        ਹਾਲਤ VDfgType::ModDivS:
        ਹਾਲਤ VDfgType::Mul:
        ਹਾਲਤ VDfgType::MulS:
        ਹਾਲਤ VDfgType::Negate:
        ਹਾਲਤ VDfgType::Neq:
        ਹਾਲਤ VDfgType::NeqCase:
        ਹਾਲਤ VDfgType::NeqWild:
        ਹਾਲਤ VDfgType::Not:
        ਹਾਲਤ VDfgType::OneHot:
        ਹਾਲਤ VDfgType::OneHot0:
        ਹਾਲਤ VDfgType::Or:
        ਹਾਲਤ VDfgType::Pow:
        ਹਾਲਤ VDfgType::PowSS:
        ਹਾਲਤ VDfgType::PowSU:
        ਹਾਲਤ VDfgType::PowUS:
        ਹਾਲਤ VDfgType::RedAnd:
        ਹਾਲਤ VDfgType::RedOr:
        ਹਾਲਤ VDfgType::RedXor:
        ਹਾਲਤ VDfgType::Rep:
        ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_L:
        ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_R:
        ਹਾਲਤ VDfgType::ਖਿਸਕਾਓ_RS:
        ਹਾਲਤ VDfgType::ਧਾਰਾ_L:
        ਹਾਲਤ VDfgType::ਧਾਰਾ_R:
        ਹਾਲਤ VDfgType::Sub:
        ਹਾਲਤ VDfgType::Xor: ਵਾਪਸ ਸੱਚ;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    ਤਾਰਕਿਕ sourcesEquivalent(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& a, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& b) ਅਚਲ {
        ਲਈ (ਮਾਪ_t i = 0; i < a.nInputs(); ++i) {
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ ap = a.inputp(i);
            ਅਚਲ Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ bp = b.inputp(i);
            ਜੇ (!ap && !bp) ਜਾਰੀ;
            ਜੇ (!ap || !bp) ਵਾਪਸ ਝੂਠ;
            ਜੇ (!ਸ਼ੀਰਸ਼_Equivalent(*ap, *bp)) ਵਾਪਸ ਝੂਠ;  // Graph is acyclic, so this terminates
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Compares 'a' and 'b' for equivalence
    ਤਾਰਕਿਕ ਸ਼ੀਰਸ਼_Equivalent(ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& a, ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& b) ਅਚਲ {
        // If same vertex, then equal
        ਜੇ (&a == &b) ਵਾਪਸ ਸੱਚ;

        // If different type, then not equal
        ਜੇ (a.type() != b.type()) ਵਾਪਸ ਝੂਠ;

        // If different data type, then not equal
        ਜੇ (a.dtype() != b.dtype()) ਵਾਪਸ ਝੂਠ;

        // If different number of inputs, then not equal
        ਜੇ (a.nInputs() != b.nInputs()) ਵਾਪਸ ਝੂਠ;

        // Check vertex specifics
        ਜੇ (!ਸ਼ੀਰਸ਼_Self_Equivalent(a, b)) ਵਾਪਸ ਝੂਠ;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        ਜੇ (!a.has_ਗੁਣਜ_Sinks() && !b.has_ਗੁਣਜ_Sinks()) ਵਾਪਸ sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ਅਚਲ ਸ਼ੀਰਸ਼_Pair ਕੁੰਜੀ = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_cache.ਲੱਭੋ(ਕੁੰਜੀ);
        ਜੇ (it != m_cache.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ਅਚਲ ਤਾਰਕਿਕ equal = sourcesEquivalent(a, b);
        ਜੇ (VL_UNLIKELY(m_cache.ਖਾਲੀ_ਥਾਵਾਂ())) m_cache.reserve(m_ਮਾਪ / 4);
        m_cache.ਪਾਓ({ਕੁੰਜੀ, equal});

        // The predicate result
        ਵਾਪਸ equal;
    }
};

// Combine equivalent operation vertices
ਖਾਲੀ dfgCseCombineEquivalent(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    ਲਈ (DfgConst* ਅਚਲ vtxp : dfg.constVertices().unlinkable()) {
        ਜੇ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3_Hash_ਸੈੱਟ<Dfg_ਸ਼ੀਰਸ਼*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.ਮਾਪ());

    // Combine operation vertices
    ਲਈ (Dfg_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        ਜੇ (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            ਜਾਰੀ;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = uniqueVtxps.ਪਾਓ(vtxp);
        ਜੇ (!pair.ਦੂਜਾ) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.ਪਹਿਲਾ);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

ਖਾਲੀ V3DfgPasses::cse(Dfg_ਗ੍ਰਾਫ& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::remove_ਅਣਵਰਤਿਆ(dfg);
}
