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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"
#அந்தர்பாவ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
வகுப்பு DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

பொது:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        க்கு (மாறிலி DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        க்கு (மாறிலி DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        க்கு (மாறிலி DfgVertex& vtx : dfg.opVertices()) {
            எனில் (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        க்கு (மாறிலி DfgConst& vtx : dfg.constVertices()) {
            மாறிலி V3Hash hash = vtx.இலக்கம்().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.மதிப்பு()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) மாறிலி { திரும்பு vertexHash(*vtxp).மதிப்பு(); }

தனிப்பட்ட:
    // Returns hash of vertex dependent on information internal to the vertex
    நிலையான V3Hash vertexSelfHash(மாறிலி DfgVertex& vtx) {
        தேர்வு (vtx.type()) {
        // Unhandled vertices
        நிலை VDfgType::Logic:  // LCOV_EXCL_START
        நிலை VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        நிலை VDfgType::Const:  // LCOV_EXCL_START
        நிலை VDfgType::CReset:
        நிலை VDfgType::VarArray:
        நிலை VDfgType::VarPacked:
        நிலை VDfgType::Prev:
        நிலை VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        நிலை VDfgType::Sel: திரும்பு V3Hash{vtx.as<DfgSel>()->lsb()};

        நிலை VDfgType::SpliceArray:
        நிலை VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](மாறிலி DfgVertex&, uint32_t lo) {
                hash += lo;
                திரும்பு பொய்;
            });
            திரும்பு hash;
        }

        // Vertices with no internal information
        நிலை VDfgType::MatchMasked:
        நிலை VDfgType::Mux:
        நிலை VDfgType::UnitArray: திரும்பு V3Hash{};

        // Generated classes - none of them have internal information
        நிலை VDfgType::Add:
        நிலை VDfgType::And:
        நிலை VDfgType::ArraySel:
        நிலை VDfgType::Concat:
        நிலை VDfgType::Cond:
        நிலை VDfgType::மொத்தம்_Ones_2:
        நிலை VDfgType::Div:
        நிலை VDfgType::DivS:
        நிலை VDfgType::Eq:
        நிலை VDfgType::EqCase:
        நிலை VDfgType::EqWild:
        நிலை VDfgType::Extend:
        நிலை VDfgType::ExtendS:
        நிலை VDfgType::Gt:
        நிலை VDfgType::GtS:
        நிலை VDfgType::Gte:
        நிலை VDfgType::GteS:
        நிலை VDfgType::LogAnd:
        நிலை VDfgType::LogEq:
        நிலை VDfgType::LogIf:
        நிலை VDfgType::LogNot:
        நிலை VDfgType::LogOr:
        நிலை VDfgType::Lt:
        நிலை VDfgType::LtS:
        நிலை VDfgType::Lte:
        நிலை VDfgType::LteS:
        நிலை VDfgType::ModDiv:
        நிலை VDfgType::ModDivS:
        நிலை VDfgType::Mul:
        நிலை VDfgType::MulS:
        நிலை VDfgType::Negate:
        நிலை VDfgType::Neq:
        நிலை VDfgType::NeqCase:
        நிலை VDfgType::NeqWild:
        நிலை VDfgType::Not:
        நிலை VDfgType::OneHot:
        நிலை VDfgType::OneHot0:
        நிலை VDfgType::Or:
        நிலை VDfgType::Pow:
        நிலை VDfgType::PowSS:
        நிலை VDfgType::PowSU:
        நிலை VDfgType::PowUS:
        நிலை VDfgType::RedAnd:
        நிலை VDfgType::RedOr:
        நிலை VDfgType::RedXor:
        நிலை VDfgType::Rep:
        நிலை VDfgType::ShiftL:
        நிலை VDfgType::ShiftR:
        நிலை VDfgType::ShiftRS:
        நிலை VDfgType::StreamL:
        நிலை VDfgType::StreamR:
        நிலை VDfgType::Sub:
        நிலை VDfgType::Xor: திரும்பு V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) மாறிலி {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        எனில் (!result.மதிப்பு()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                திரும்பு பொய்;
            });
            result = hash;
        }
        திரும்பு result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
வகுப்பு DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<மாறிலி DfgVertex*, மாறிலி DfgVertex*>;
    கட்டமைப்பு VertexPairHash final {
        size_t operator()(மாறிலி VertexPair& pair) மாறிலி {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            திரும்பு hash.மதிப்பு();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, தருக்க, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    மாறிலி size_t m_size;  // Size of the graph

பொது:
    // CONSTRUCTORS
    explicit DfgCseEqual(மாறிலி DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    தருக்க operator()(DfgVertex* ap, DfgVertex* bp) மாறிலி { திரும்பு vertexEquivalent(*ap, *bp); }

தனிப்பட்ட:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    தருக்க vertexSelfEquivalent(மாறிலி DfgVertex& a, மாறிலி DfgVertex& b) மாறிலி {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        தேர்வு (a.type()) {
        // Unhandled vertices
        நிலை VDfgType::Logic:  // LCOV_EXCL_START
        நிலை VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        நிலை VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        நிலை VDfgType::Const: திரும்பு a.as<DfgConst>()->இலக்கம்().isCaseEq(b.as<DfgConst>()->இலக்கம்());
        நிலை VDfgType::CReset: திரும்பு பொய்;
        நிலை VDfgType::Prev: திரும்பு பொய்;

        நிலை VDfgType::VarArray:
        நிலை VDfgType::VarPacked:  // CSE does not combine variables
            திரும்பு பொய்;

        // Vertices with internal information
        நிலை VDfgType::Sel: திரும்பு a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        நிலை VDfgType::SpliceArray:
        நிலை VDfgType::SplicePacked: {
            மாறிலி DfgVertexSplice* மாறிலி ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](மாறிலி DfgVertex&, uint32_t lo) {
                m_driverLo.தள்ளு_back(lo);
                திரும்பு பொய்;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            திரும்பு !b.as<DfgVertexSplice>()->foreachDriver([&](மாறிலி DfgVertex&, uint32_t lo) {  //
                திரும்பு *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        நிலை VDfgType::MatchMasked:
        நிலை VDfgType::Mux:
        நிலை VDfgType::UnitArray: திரும்பு உண்மை;

        // Generated classes - none of them have internal information
        நிலை VDfgType::Add:
        நிலை VDfgType::And:
        நிலை VDfgType::ArraySel:
        நிலை VDfgType::Concat:
        நிலை VDfgType::Cond:
        நிலை VDfgType::மொத்தம்_Ones_2:
        நிலை VDfgType::Div:
        நிலை VDfgType::DivS:
        நிலை VDfgType::Eq:
        நிலை VDfgType::EqCase:
        நிலை VDfgType::EqWild:
        நிலை VDfgType::Extend:
        நிலை VDfgType::ExtendS:
        நிலை VDfgType::Gt:
        நிலை VDfgType::GtS:
        நிலை VDfgType::Gte:
        நிலை VDfgType::GteS:
        நிலை VDfgType::LogAnd:
        நிலை VDfgType::LogEq:
        நிலை VDfgType::LogIf:
        நிலை VDfgType::LogNot:
        நிலை VDfgType::LogOr:
        நிலை VDfgType::Lt:
        நிலை VDfgType::LtS:
        நிலை VDfgType::Lte:
        நிலை VDfgType::LteS:
        நிலை VDfgType::ModDiv:
        நிலை VDfgType::ModDivS:
        நிலை VDfgType::Mul:
        நிலை VDfgType::MulS:
        நிலை VDfgType::Negate:
        நிலை VDfgType::Neq:
        நிலை VDfgType::NeqCase:
        நிலை VDfgType::NeqWild:
        நிலை VDfgType::Not:
        நிலை VDfgType::OneHot:
        நிலை VDfgType::OneHot0:
        நிலை VDfgType::Or:
        நிலை VDfgType::Pow:
        நிலை VDfgType::PowSS:
        நிலை VDfgType::PowSU:
        நிலை VDfgType::PowUS:
        நிலை VDfgType::RedAnd:
        நிலை VDfgType::RedOr:
        நிலை VDfgType::RedXor:
        நிலை VDfgType::Rep:
        நிலை VDfgType::ShiftL:
        நிலை VDfgType::ShiftR:
        நிலை VDfgType::ShiftRS:
        நிலை VDfgType::StreamL:
        நிலை VDfgType::StreamR:
        நிலை VDfgType::Sub:
        நிலை VDfgType::Xor: திரும்பு உண்மை;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    தருக்க sourcesEquivalent(மாறிலி DfgVertex& a, மாறிலி DfgVertex& b) மாறிலி {
        க்கு (size_t i = 0; i < a.nInputs(); ++i) {
            மாறிலி DfgVertex* மாறிலி ap = a.inputp(i);
            மாறிலி DfgVertex* மாறிலி bp = b.inputp(i);
            எனில் (!ap && !bp) தொடர்;
            எனில் (!ap || !bp) திரும்பு பொய்;
            எனில் (!vertexEquivalent(*ap, *bp)) திரும்பு பொய்;  // Graph is acyclic, so this terminates
        }
        திரும்பு உண்மை;
    }

    // Compares 'a' and 'b' for equivalence
    தருக்க vertexEquivalent(மாறிலி DfgVertex& a, மாறிலி DfgVertex& b) மாறிலி {
        // If same vertex, then equal
        எனில் (&a == &b) திரும்பு உண்மை;

        // If different type, then not equal
        எனில் (a.type() != b.type()) திரும்பு பொய்;

        // If different data type, then not equal
        எனில் (a.dtype() != b.dtype()) திரும்பு பொய்;

        // If different number of inputs, then not equal
        எனில் (a.nInputs() != b.nInputs()) திரும்பு பொய்;

        // Check vertex specifics
        எனில் (!vertexSelfEquivalent(a, b)) திரும்பு பொய்;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        எனில் (!a.hasMultipleSinks() && !b.hasMultipleSinks()) திரும்பு sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        மாறிலி VertexPair சாவி = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        மாறிலி auto it = m_cache.find(சாவி);
        எனில் (it != m_cache.end()) திரும்பு it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        மாறிலி தருக்க equal = sourcesEquivalent(a, b);
        எனில் (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.செருகு({சாவி, equal});

        // The predicate result
        திரும்பு equal;
    }
};

// Combine equivalent operation vertices
வெறுமை dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    க்கு (DfgConst* மாறிலி vtxp : dfg.constVertices().unlinkable()) {
        எனில் (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    க்கு (DfgVertex* மாறிலி vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        எனில் (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            தொடர்;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        மாறிலி auto pair = uniqueVtxps.செருகு(vtxp);
        எனில் (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

வெறுமை V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
