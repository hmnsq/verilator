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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"
#अन्तर्भाव "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
वर्ग DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

सार्वजनिक:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        प्रति (ध्रुव DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        प्रति (ध्रुव DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        प्रति (ध्रुव DfgVertex& vtx : dfg.opVertices()) {
            जइ (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        प्रति (ध्रुव DfgConst& vtx : dfg.constVertices()) {
            ध्रुव V3Hash hash = vtx.अङ्कः().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.मानम्()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) ध्रुव { प्रत्यागम vertexHash(*vtxp).मानम्(); }

गुप्त:
    // Returns hash of vertex dependent on information internal to the vertex
    स्थिर V3Hash vertexSelfHash(ध्रुव DfgVertex& vtx) {
        विकल्प (vtx.type()) {
        // Unhandled vertices
        पक्ष VDfgType::Logic:  // LCOV_EXCL_START
        पक्ष VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        पक्ष VDfgType::Const:  // LCOV_EXCL_START
        पक्ष VDfgType::CReset:
        पक्ष VDfgType::VarArray:
        पक्ष VDfgType::VarPacked:
        पक्ष VDfgType::Prev:
        पक्ष VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        पक्ष VDfgType::Sel: प्रत्यागम V3Hash{vtx.as<DfgSel>()->lsb()};

        पक्ष VDfgType::SpliceArray:
        पक्ष VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](ध्रुव DfgVertex&, uint32_t lo) {
                hash += lo;
                प्रत्यागम मिच्छ;
            });
            प्रत्यागम hash;
        }

        // Vertices with no internal information
        पक्ष VDfgType::MatchMasked:
        पक्ष VDfgType::Mux:
        पक्ष VDfgType::UnitArray: प्रत्यागम V3Hash{};

        // Generated classes - none of them have internal information
        पक्ष VDfgType::Add:
        पक्ष VDfgType::And:
        पक्ष VDfgType::ArraySel:
        पक्ष VDfgType::Concat:
        पक्ष VDfgType::Cond:
        पक्ष VDfgType::गणनम्_Ones_2:
        पक्ष VDfgType::Div:
        पक्ष VDfgType::DivS:
        पक्ष VDfgType::Eq:
        पक्ष VDfgType::EqCase:
        पक्ष VDfgType::EqWild:
        पक्ष VDfgType::Extend:
        पक्ष VDfgType::ExtendS:
        पक्ष VDfgType::Gt:
        पक्ष VDfgType::GtS:
        पक्ष VDfgType::Gte:
        पक्ष VDfgType::GteS:
        पक्ष VDfgType::LogAnd:
        पक्ष VDfgType::LogEq:
        पक्ष VDfgType::LogIf:
        पक्ष VDfgType::LogNot:
        पक्ष VDfgType::LogOr:
        पक्ष VDfgType::Lt:
        पक्ष VDfgType::LtS:
        पक्ष VDfgType::Lte:
        पक्ष VDfgType::LteS:
        पक्ष VDfgType::ModDiv:
        पक्ष VDfgType::ModDivS:
        पक्ष VDfgType::Mul:
        पक्ष VDfgType::MulS:
        पक्ष VDfgType::Negate:
        पक्ष VDfgType::Neq:
        पक्ष VDfgType::NeqCase:
        पक्ष VDfgType::NeqWild:
        पक्ष VDfgType::Not:
        पक्ष VDfgType::OneHot:
        पक्ष VDfgType::OneHot0:
        पक्ष VDfgType::Or:
        पक्ष VDfgType::Pow:
        पक्ष VDfgType::PowSS:
        पक्ष VDfgType::PowSU:
        पक्ष VDfgType::PowUS:
        पक्ष VDfgType::RedAnd:
        पक्ष VDfgType::RedOr:
        पक्ष VDfgType::RedXor:
        पक्ष VDfgType::Rep:
        पक्ष VDfgType::ShiftL:
        पक्ष VDfgType::ShiftR:
        पक्ष VDfgType::ShiftRS:
        पक्ष VDfgType::StreamL:
        पक्ष VDfgType::StreamR:
        पक्ष VDfgType::Sub:
        पक्ष VDfgType::Xor: प्रत्यागम V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) ध्रुव {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        जइ (!result.मानम्()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                प्रत्यागम मिच्छ;
            });
            result = hash;
        }
        प्रत्यागम result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
वर्ग DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<ध्रुव DfgVertex*, ध्रुव DfgVertex*>;
    संरचना VertexPairHash final {
        size_t operator()(ध्रुव VertexPair& pair) ध्रुव {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            प्रत्यागम hash.मानम्();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, तार्किक, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    ध्रुव size_t m_size;  // Size of the graph

सार्वजनिक:
    // CONSTRUCTORS
    explicit DfgCseEqual(ध्रुव DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    तार्किक operator()(DfgVertex* ap, DfgVertex* bp) ध्रुव { प्रत्यागम vertexEquivalent(*ap, *bp); }

गुप्त:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    तार्किक vertexSelfEquivalent(ध्रुव DfgVertex& a, ध्रुव DfgVertex& b) ध्रुव {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        विकल्प (a.type()) {
        // Unhandled vertices
        पक्ष VDfgType::Logic:  // LCOV_EXCL_START
        पक्ष VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        पक्ष VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        पक्ष VDfgType::Const: प्रत्यागम a.as<DfgConst>()->अङ्कः().isCaseEq(b.as<DfgConst>()->अङ्कः());
        पक्ष VDfgType::CReset: प्रत्यागम मिच्छ;
        पक्ष VDfgType::Prev: प्रत्यागम मिच्छ;

        पक्ष VDfgType::VarArray:
        पक्ष VDfgType::VarPacked:  // CSE does not combine variables
            प्रत्यागम मिच्छ;

        // Vertices with internal information
        पक्ष VDfgType::Sel: प्रत्यागम a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        पक्ष VDfgType::SpliceArray:
        पक्ष VDfgType::SplicePacked: {
            ध्रुव DfgVertexSplice* ध्रुव ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ध्रुव DfgVertex&, uint32_t lo) {
                m_driverLo.प्रक्षिप_back(lo);
                प्रत्यागम मिच्छ;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            प्रत्यागम !b.as<DfgVertexSplice>()->foreachDriver([&](ध्रुव DfgVertex&, uint32_t lo) {  //
                प्रत्यागम *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        पक्ष VDfgType::MatchMasked:
        पक्ष VDfgType::Mux:
        पक्ष VDfgType::UnitArray: प्रत्यागम सच्च;

        // Generated classes - none of them have internal information
        पक्ष VDfgType::Add:
        पक्ष VDfgType::And:
        पक्ष VDfgType::ArraySel:
        पक्ष VDfgType::Concat:
        पक्ष VDfgType::Cond:
        पक्ष VDfgType::गणनम्_Ones_2:
        पक्ष VDfgType::Div:
        पक्ष VDfgType::DivS:
        पक्ष VDfgType::Eq:
        पक्ष VDfgType::EqCase:
        पक्ष VDfgType::EqWild:
        पक्ष VDfgType::Extend:
        पक्ष VDfgType::ExtendS:
        पक्ष VDfgType::Gt:
        पक्ष VDfgType::GtS:
        पक्ष VDfgType::Gte:
        पक्ष VDfgType::GteS:
        पक्ष VDfgType::LogAnd:
        पक्ष VDfgType::LogEq:
        पक्ष VDfgType::LogIf:
        पक्ष VDfgType::LogNot:
        पक्ष VDfgType::LogOr:
        पक्ष VDfgType::Lt:
        पक्ष VDfgType::LtS:
        पक्ष VDfgType::Lte:
        पक्ष VDfgType::LteS:
        पक्ष VDfgType::ModDiv:
        पक्ष VDfgType::ModDivS:
        पक्ष VDfgType::Mul:
        पक्ष VDfgType::MulS:
        पक्ष VDfgType::Negate:
        पक्ष VDfgType::Neq:
        पक्ष VDfgType::NeqCase:
        पक्ष VDfgType::NeqWild:
        पक्ष VDfgType::Not:
        पक्ष VDfgType::OneHot:
        पक्ष VDfgType::OneHot0:
        पक्ष VDfgType::Or:
        पक्ष VDfgType::Pow:
        पक्ष VDfgType::PowSS:
        पक्ष VDfgType::PowSU:
        पक्ष VDfgType::PowUS:
        पक्ष VDfgType::RedAnd:
        पक्ष VDfgType::RedOr:
        पक्ष VDfgType::RedXor:
        पक्ष VDfgType::Rep:
        पक्ष VDfgType::ShiftL:
        पक्ष VDfgType::ShiftR:
        पक्ष VDfgType::ShiftRS:
        पक्ष VDfgType::StreamL:
        पक्ष VDfgType::StreamR:
        पक्ष VDfgType::Sub:
        पक्ष VDfgType::Xor: प्रत्यागम सच्च;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    तार्किक sourcesEquivalent(ध्रुव DfgVertex& a, ध्रुव DfgVertex& b) ध्रुव {
        प्रति (size_t i = 0; i < a.nInputs(); ++i) {
            ध्रुव DfgVertex* ध्रुव ap = a.inputp(i);
            ध्रुव DfgVertex* ध्रुव bp = b.inputp(i);
            जइ (!ap && !bp) अनुवर्त;
            जइ (!ap || !bp) प्रत्यागम मिच्छ;
            जइ (!vertexEquivalent(*ap, *bp)) प्रत्यागम मिच्छ;  // Graph is acyclic, so this terminates
        }
        प्रत्यागम सच्च;
    }

    // Compares 'a' and 'b' for equivalence
    तार्किक vertexEquivalent(ध्रुव DfgVertex& a, ध्रुव DfgVertex& b) ध्रुव {
        // If same vertex, then equal
        जइ (&a == &b) प्रत्यागम सच्च;

        // If different type, then not equal
        जइ (a.type() != b.type()) प्रत्यागम मिच्छ;

        // If different data type, then not equal
        जइ (a.dtype() != b.dtype()) प्रत्यागम मिच्छ;

        // If different number of inputs, then not equal
        जइ (a.nInputs() != b.nInputs()) प्रत्यागम मिच्छ;

        // Check vertex specifics
        जइ (!vertexSelfEquivalent(a, b)) प्रत्यागम मिच्छ;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        जइ (!a.hasMultipleSinks() && !b.hasMultipleSinks()) प्रत्यागम sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ध्रुव VertexPair कुञ्चिका = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ध्रुव auto it = m_cache.find(कुञ्चिका);
        जइ (it != m_cache.end()) प्रत्यागम it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ध्रुव तार्किक equal = sourcesEquivalent(a, b);
        जइ (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.निवेशय({कुञ्चिका, equal});

        // The predicate result
        प्रत्यागम equal;
    }
};

// Combine equivalent operation vertices
शून्य dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    प्रति (DfgConst* ध्रुव vtxp : dfg.constVertices().unlinkable()) {
        जइ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    प्रति (DfgVertex* ध्रुव vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        जइ (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            अनुवर्त;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ध्रुव auto pair = uniqueVtxps.निवेशय(vtxp);
        जइ (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

शून्य V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
