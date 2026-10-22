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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
ವರ್ಗ DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        ಪ್ರತಿ (ಧ್ರುವ DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        ಪ್ರತಿ (ಧ್ರುವ DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        ಪ್ರತಿ (ಧ್ರುವ DfgVertex& vtx : dfg.opVertices()) {
            ಯದಿ (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        ಪ್ರತಿ (ಧ್ರುವ DfgConst& vtx : dfg.constVertices()) {
            ಧ್ರುವ V3Hash hash = vtx.ಅಂಕೆ().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.ಮೌಲ್ಯ()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ vertexHash(*vtxp).ಮೌಲ್ಯ(); }

ಗುಪ್ತ:
    // Returns hash of vertex dependent on information internal to the vertex
    ಸ್ಥಿರ V3Hash vertexSelfHash(ಧ್ರುವ DfgVertex& vtx) {
        ವಿಕಲ್ಪ (vtx.type()) {
        // Unhandled vertices
        ಪಕ್ಷ VDfgType::Logic:  // LCOV_EXCL_START
        ಪಕ್ಷ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        ಪಕ್ಷ VDfgType::Const:  // LCOV_EXCL_START
        ಪಕ್ಷ VDfgType::CReset:
        ಪಕ್ಷ VDfgType::VarArray:
        ಪಕ್ಷ VDfgType::VarPacked:
        ಪಕ್ಷ VDfgType::Prev:
        ಪಕ್ಷ VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        ಪಕ್ಷ VDfgType::Sel: ಪ್ರತ್ಯಾಗಮ V3Hash{vtx.as<DfgSel>()->lsb()};

        ಪಕ್ಷ VDfgType::SpliceArray:
        ಪಕ್ಷ VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](ಧ್ರುವ DfgVertex&, uint32_t lo) {
                hash += lo;
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            ಪ್ರತ್ಯಾಗಮ hash;
        }

        // Vertices with no internal information
        ಪಕ್ಷ VDfgType::MatchMasked:
        ಪಕ್ಷ VDfgType::Mux:
        ಪಕ್ಷ VDfgType::UnitArray: ಪ್ರತ್ಯಾಗಮ V3Hash{};

        // Generated classes - none of them have internal information
        ಪಕ್ಷ VDfgType::Add:
        ಪಕ್ಷ VDfgType::And:
        ಪಕ್ಷ VDfgType::ArraySel:
        ಪಕ್ಷ VDfgType::Concat:
        ಪಕ್ಷ VDfgType::Cond:
        ಪಕ್ಷ VDfgType::ಎಣಿಕೆ_Ones_2:
        ಪಕ್ಷ VDfgType::Div:
        ಪಕ್ಷ VDfgType::DivS:
        ಪಕ್ಷ VDfgType::Eq:
        ಪಕ್ಷ VDfgType::EqCase:
        ಪಕ್ಷ VDfgType::EqWild:
        ಪಕ್ಷ VDfgType::Extend:
        ಪಕ್ಷ VDfgType::ExtendS:
        ಪಕ್ಷ VDfgType::Gt:
        ಪಕ್ಷ VDfgType::GtS:
        ಪಕ್ಷ VDfgType::Gte:
        ಪಕ್ಷ VDfgType::GteS:
        ಪಕ್ಷ VDfgType::LogAnd:
        ಪಕ್ಷ VDfgType::LogEq:
        ಪಕ್ಷ VDfgType::LogIf:
        ಪಕ್ಷ VDfgType::LogNot:
        ಪಕ್ಷ VDfgType::LogOr:
        ಪಕ್ಷ VDfgType::Lt:
        ಪಕ್ಷ VDfgType::LtS:
        ಪಕ್ಷ VDfgType::Lte:
        ಪಕ್ಷ VDfgType::LteS:
        ಪಕ್ಷ VDfgType::ModDiv:
        ಪಕ್ಷ VDfgType::ModDivS:
        ಪಕ್ಷ VDfgType::Mul:
        ಪಕ್ಷ VDfgType::MulS:
        ಪಕ್ಷ VDfgType::Negate:
        ಪಕ್ಷ VDfgType::Neq:
        ಪಕ್ಷ VDfgType::NeqCase:
        ಪಕ್ಷ VDfgType::NeqWild:
        ಪಕ್ಷ VDfgType::Not:
        ಪಕ್ಷ VDfgType::OneHot:
        ಪಕ್ಷ VDfgType::OneHot0:
        ಪಕ್ಷ VDfgType::Or:
        ಪಕ್ಷ VDfgType::Pow:
        ಪಕ್ಷ VDfgType::PowSS:
        ಪಕ್ಷ VDfgType::PowSU:
        ಪಕ್ಷ VDfgType::PowUS:
        ಪಕ್ಷ VDfgType::RedAnd:
        ಪಕ್ಷ VDfgType::RedOr:
        ಪಕ್ಷ VDfgType::RedXor:
        ಪಕ್ಷ VDfgType::Rep:
        ಪಕ್ಷ VDfgType::ShiftL:
        ಪಕ್ಷ VDfgType::ShiftR:
        ಪಕ್ಷ VDfgType::ShiftRS:
        ಪಕ್ಷ VDfgType::StreamL:
        ಪಕ್ಷ VDfgType::StreamR:
        ಪಕ್ಷ VDfgType::Sub:
        ಪಕ್ಷ VDfgType::Xor: ಪ್ರತ್ಯಾಗಮ V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) ಧ್ರುವ {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        ಯದಿ (!result.ಮೌಲ್ಯ()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            result = hash;
        }
        ಪ್ರತ್ಯಾಗಮ result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
ವರ್ಗ DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<ಧ್ರುವ DfgVertex*, ಧ್ರುವ DfgVertex*>;
    ಸಂರಚನಾ VertexPairHash final {
        size_t operator()(ಧ್ರುವ VertexPair& pair) ಧ್ರುವ {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, ತಾರ್ಕಿಕ, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    ಧ್ರುವ size_t m_size;  // Size of the graph

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit DfgCseEqual(ಧ್ರುವ DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    ತಾರ್ಕಿಕ operator()(DfgVertex* ap, DfgVertex* bp) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ vertexEquivalent(*ap, *bp); }

ಗುಪ್ತ:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    ತಾರ್ಕಿಕ vertexSelfEquivalent(ಧ್ರುವ DfgVertex& a, ಧ್ರುವ DfgVertex& b) ಧ್ರುವ {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        ವಿಕಲ್ಪ (a.type()) {
        // Unhandled vertices
        ಪಕ್ಷ VDfgType::Logic:  // LCOV_EXCL_START
        ಪಕ್ಷ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        ಪಕ್ಷ VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        ಪಕ್ಷ VDfgType::Const: ಪ್ರತ್ಯಾಗಮ a.as<DfgConst>()->ಅಂಕೆ().isCaseEq(b.as<DfgConst>()->ಅಂಕೆ());
        ಪಕ್ಷ VDfgType::CReset: ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪಕ್ಷ VDfgType::Prev: ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಪಕ್ಷ VDfgType::VarArray:
        ಪಕ್ಷ VDfgType::VarPacked:  // CSE does not combine variables
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Vertices with internal information
        ಪಕ್ಷ VDfgType::Sel: ಪ್ರತ್ಯಾಗಮ a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        ಪಕ್ಷ VDfgType::SpliceArray:
        ಪಕ್ಷ VDfgType::SplicePacked: {
            ಧ್ರುವ DfgVertexSplice* ಧ್ರುವ ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ಧ್ರುವ DfgVertex&, uint32_t lo) {
                m_driverLo.ತಳ್ಳು_back(lo);
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            ಪ್ರತ್ಯಾಗಮ !b.as<DfgVertexSplice>()->foreachDriver([&](ಧ್ರುವ DfgVertex&, uint32_t lo) {  //
                ಪ್ರತ್ಯಾಗಮ *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        ಪಕ್ಷ VDfgType::MatchMasked:
        ಪಕ್ಷ VDfgType::Mux:
        ಪಕ್ಷ VDfgType::UnitArray: ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

        // Generated classes - none of them have internal information
        ಪಕ್ಷ VDfgType::Add:
        ಪಕ್ಷ VDfgType::And:
        ಪಕ್ಷ VDfgType::ArraySel:
        ಪಕ್ಷ VDfgType::Concat:
        ಪಕ್ಷ VDfgType::Cond:
        ಪಕ್ಷ VDfgType::ಎಣಿಕೆ_Ones_2:
        ಪಕ್ಷ VDfgType::Div:
        ಪಕ್ಷ VDfgType::DivS:
        ಪಕ್ಷ VDfgType::Eq:
        ಪಕ್ಷ VDfgType::EqCase:
        ಪಕ್ಷ VDfgType::EqWild:
        ಪಕ್ಷ VDfgType::Extend:
        ಪಕ್ಷ VDfgType::ExtendS:
        ಪಕ್ಷ VDfgType::Gt:
        ಪಕ್ಷ VDfgType::GtS:
        ಪಕ್ಷ VDfgType::Gte:
        ಪಕ್ಷ VDfgType::GteS:
        ಪಕ್ಷ VDfgType::LogAnd:
        ಪಕ್ಷ VDfgType::LogEq:
        ಪಕ್ಷ VDfgType::LogIf:
        ಪಕ್ಷ VDfgType::LogNot:
        ಪಕ್ಷ VDfgType::LogOr:
        ಪಕ್ಷ VDfgType::Lt:
        ಪಕ್ಷ VDfgType::LtS:
        ಪಕ್ಷ VDfgType::Lte:
        ಪಕ್ಷ VDfgType::LteS:
        ಪಕ್ಷ VDfgType::ModDiv:
        ಪಕ್ಷ VDfgType::ModDivS:
        ಪಕ್ಷ VDfgType::Mul:
        ಪಕ್ಷ VDfgType::MulS:
        ಪಕ್ಷ VDfgType::Negate:
        ಪಕ್ಷ VDfgType::Neq:
        ಪಕ್ಷ VDfgType::NeqCase:
        ಪಕ್ಷ VDfgType::NeqWild:
        ಪಕ್ಷ VDfgType::Not:
        ಪಕ್ಷ VDfgType::OneHot:
        ಪಕ್ಷ VDfgType::OneHot0:
        ಪಕ್ಷ VDfgType::Or:
        ಪಕ್ಷ VDfgType::Pow:
        ಪಕ್ಷ VDfgType::PowSS:
        ಪಕ್ಷ VDfgType::PowSU:
        ಪಕ್ಷ VDfgType::PowUS:
        ಪಕ್ಷ VDfgType::RedAnd:
        ಪಕ್ಷ VDfgType::RedOr:
        ಪಕ್ಷ VDfgType::RedXor:
        ಪಕ್ಷ VDfgType::Rep:
        ಪಕ್ಷ VDfgType::ShiftL:
        ಪಕ್ಷ VDfgType::ShiftR:
        ಪಕ್ಷ VDfgType::ShiftRS:
        ಪಕ್ಷ VDfgType::StreamL:
        ಪಕ್ಷ VDfgType::StreamR:
        ಪಕ್ಷ VDfgType::Sub:
        ಪಕ್ಷ VDfgType::Xor: ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    ತಾರ್ಕಿಕ sourcesEquivalent(ಧ್ರುವ DfgVertex& a, ಧ್ರುವ DfgVertex& b) ಧ್ರುವ {
        ಪ್ರತಿ (size_t i = 0; i < a.nInputs(); ++i) {
            ಧ್ರುವ DfgVertex* ಧ್ರುವ ap = a.inputp(i);
            ಧ್ರುವ DfgVertex* ಧ್ರುವ bp = b.inputp(i);
            ಯದಿ (!ap && !bp) ಅನುವರ್ತ;
            ಯದಿ (!ap || !bp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (!vertexEquivalent(*ap, *bp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Graph is acyclic, so this terminates
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Compares 'a' and 'b' for equivalence
    ತಾರ್ಕಿಕ vertexEquivalent(ಧ್ರುವ DfgVertex& a, ಧ್ರುವ DfgVertex& b) ಧ್ರುವ {
        // If same vertex, then equal
        ಯದಿ (&a == &b) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

        // If different type, then not equal
        ಯದಿ (a.type() != b.type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // If different data type, then not equal
        ಯದಿ (a.dtype() != b.dtype()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // If different number of inputs, then not equal
        ಯದಿ (a.nInputs() != b.nInputs()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Check vertex specifics
        ಯದಿ (!vertexSelfEquivalent(a, b)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        ಯದಿ (!a.hasMultipleSinks() && !b.hasMultipleSinks()) ಪ್ರತ್ಯಾಗಮ sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ಧ್ರುವ VertexPair ಕೀಲಿ = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ಧ್ರುವ auto it = m_cache.find(ಕೀಲಿ);
        ಯದಿ (it != m_cache.end()) ಪ್ರತ್ಯಾಗಮ it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ಧ್ರುವ ತಾರ್ಕಿಕ equal = sourcesEquivalent(a, b);
        ಯದಿ (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.ಸೇರಿಸು({ಕೀಲಿ, equal});

        // The predicate result
        ಪ್ರತ್ಯಾಗಮ equal;
    }
};

// Combine equivalent operation vertices
ಶೂನ್ಯ dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    ಪ್ರತಿ (DfgConst* ಧ್ರುವ vtxp : dfg.constVertices().unlinkable()) {
        ಯದಿ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    ಪ್ರತಿ (DfgVertex* ಧ್ರುವ vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        ಯದಿ (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            ಅನುವರ್ತ;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ಧ್ರುವ auto pair = uniqueVtxps.ಸೇರಿಸು(vtxp);
        ಯದಿ (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

ಶೂನ್ಯ V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
