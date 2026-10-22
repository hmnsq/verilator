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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3DfgPasses.h"
#অন্তর্ভাৱ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
শ্ৰেণী DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

সার্ৱজনিক:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        বাবে (ধ্রুৱ DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        বাবে (ধ্রুৱ DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        বাবে (ধ্রুৱ DfgVertex& vtx : dfg.opVertices()) {
            যদি (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        বাবে (ধ্রুৱ DfgConst& vtx : dfg.constVertices()) {
            ধ্রুৱ V3Hash hash = vtx.অঙ্ক().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.মান()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) ধ্রুৱ { ঘূৰাই vertexHash(*vtxp).মান(); }

গুপ্ত:
    // Returns hash of vertex dependent on information internal to the vertex
    স্থির V3Hash vertexSelfHash(ধ্রুৱ DfgVertex& vtx) {
        ৱিকল্প (vtx.type()) {
        // Unhandled vertices
        পক্ষ VDfgType::Logic:  // LCOV_EXCL_START
        পক্ষ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        পক্ষ VDfgType::Const:  // LCOV_EXCL_START
        পক্ষ VDfgType::CReset:
        পক্ষ VDfgType::VarArray:
        পক্ষ VDfgType::VarPacked:
        পক্ষ VDfgType::Prev:
        পক্ষ VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        পক্ষ VDfgType::Sel: ঘূৰাই V3Hash{vtx.as<DfgSel>()->lsb()};

        পক্ষ VDfgType::SpliceArray:
        পক্ষ VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](ধ্রুৱ DfgVertex&, uint32_t lo) {
                hash += lo;
                ঘূৰাই মিছা;
            });
            ঘূৰাই hash;
        }

        // Vertices with no internal information
        পক্ষ VDfgType::MatchMasked:
        পক্ষ VDfgType::Mux:
        পক্ষ VDfgType::UnitArray: ঘূৰাই V3Hash{};

        // Generated classes - none of them have internal information
        পক্ষ VDfgType::Add:
        পক্ষ VDfgType::And:
        পক্ষ VDfgType::ArraySel:
        পক্ষ VDfgType::Concat:
        পক্ষ VDfgType::Cond:
        পক্ষ VDfgType::গুনতি_Ones_2:
        পক্ষ VDfgType::Div:
        পক্ষ VDfgType::DivS:
        পক্ষ VDfgType::Eq:
        পক্ষ VDfgType::EqCase:
        পক্ষ VDfgType::EqWild:
        পক্ষ VDfgType::Extend:
        পক্ষ VDfgType::ExtendS:
        পক্ষ VDfgType::Gt:
        পক্ষ VDfgType::GtS:
        পক্ষ VDfgType::Gte:
        পক্ষ VDfgType::GteS:
        পক্ষ VDfgType::LogAnd:
        পক্ষ VDfgType::LogEq:
        পক্ষ VDfgType::LogIf:
        পক্ষ VDfgType::LogNot:
        পক্ষ VDfgType::LogOr:
        পক্ষ VDfgType::Lt:
        পক্ষ VDfgType::LtS:
        পক্ষ VDfgType::Lte:
        পক্ষ VDfgType::LteS:
        পক্ষ VDfgType::ModDiv:
        পক্ষ VDfgType::ModDivS:
        পক্ষ VDfgType::Mul:
        পক্ষ VDfgType::MulS:
        পক্ষ VDfgType::Negate:
        পক্ষ VDfgType::Neq:
        পক্ষ VDfgType::NeqCase:
        পক্ষ VDfgType::NeqWild:
        পক্ষ VDfgType::Not:
        পক্ষ VDfgType::OneHot:
        পক্ষ VDfgType::OneHot0:
        পক্ষ VDfgType::Or:
        পক্ষ VDfgType::Pow:
        পক্ষ VDfgType::PowSS:
        পক্ষ VDfgType::PowSU:
        পক্ষ VDfgType::PowUS:
        পক্ষ VDfgType::RedAnd:
        পক্ষ VDfgType::RedOr:
        পক্ষ VDfgType::RedXor:
        পক্ষ VDfgType::Rep:
        পক্ষ VDfgType::ShiftL:
        পক্ষ VDfgType::ShiftR:
        পক্ষ VDfgType::ShiftRS:
        পক্ষ VDfgType::StreamL:
        পক্ষ VDfgType::StreamR:
        পক্ষ VDfgType::Sub:
        পক্ষ VDfgType::Xor: ঘূৰাই V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) ধ্রুৱ {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        যদি (!result.মান()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                ঘূৰাই মিছা;
            });
            result = hash;
        }
        ঘূৰাই result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
শ্ৰেণী DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<ধ্রুৱ DfgVertex*, ধ্রুৱ DfgVertex*>;
    সংরচনা VertexPairHash final {
        size_t operator()(ধ্রুৱ VertexPair& pair) ধ্রুৱ {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            ঘূৰাই hash.মান();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, তার্কিক, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    ধ্রুৱ size_t m_size;  // Size of the graph

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit DfgCseEqual(ধ্রুৱ DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    তার্কিক operator()(DfgVertex* ap, DfgVertex* bp) ধ্রুৱ { ঘূৰাই vertexEquivalent(*ap, *bp); }

গুপ্ত:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    তার্কিক vertexSelfEquivalent(ধ্রুৱ DfgVertex& a, ধ্রুৱ DfgVertex& b) ধ্রুৱ {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        ৱিকল্প (a.type()) {
        // Unhandled vertices
        পক্ষ VDfgType::Logic:  // LCOV_EXCL_START
        পক্ষ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        পক্ষ VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        পক্ষ VDfgType::Const: ঘূৰাই a.as<DfgConst>()->অঙ্ক().isCaseEq(b.as<DfgConst>()->অঙ্ক());
        পক্ষ VDfgType::CReset: ঘূৰাই মিছা;
        পক্ষ VDfgType::Prev: ঘূৰাই মিছা;

        পক্ষ VDfgType::VarArray:
        পক্ষ VDfgType::VarPacked:  // CSE does not combine variables
            ঘূৰাই মিছা;

        // Vertices with internal information
        পক্ষ VDfgType::Sel: ঘূৰাই a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        পক্ষ VDfgType::SpliceArray:
        পক্ষ VDfgType::SplicePacked: {
            ধ্রুৱ DfgVertexSplice* ধ্রুৱ ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ধ্রুৱ DfgVertex&, uint32_t lo) {
                m_driverLo.ঠেলো_back(lo);
                ঘূৰাই মিছা;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            ঘূৰাই !b.as<DfgVertexSplice>()->foreachDriver([&](ধ্রুৱ DfgVertex&, uint32_t lo) {  //
                ঘূৰাই *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        পক্ষ VDfgType::MatchMasked:
        পক্ষ VDfgType::Mux:
        পক্ষ VDfgType::UnitArray: ঘূৰাই সঁচা;

        // Generated classes - none of them have internal information
        পক্ষ VDfgType::Add:
        পক্ষ VDfgType::And:
        পক্ষ VDfgType::ArraySel:
        পক্ষ VDfgType::Concat:
        পক্ষ VDfgType::Cond:
        পক্ষ VDfgType::গুনতি_Ones_2:
        পক্ষ VDfgType::Div:
        পক্ষ VDfgType::DivS:
        পক্ষ VDfgType::Eq:
        পক্ষ VDfgType::EqCase:
        পক্ষ VDfgType::EqWild:
        পক্ষ VDfgType::Extend:
        পক্ষ VDfgType::ExtendS:
        পক্ষ VDfgType::Gt:
        পক্ষ VDfgType::GtS:
        পক্ষ VDfgType::Gte:
        পক্ষ VDfgType::GteS:
        পক্ষ VDfgType::LogAnd:
        পক্ষ VDfgType::LogEq:
        পক্ষ VDfgType::LogIf:
        পক্ষ VDfgType::LogNot:
        পক্ষ VDfgType::LogOr:
        পক্ষ VDfgType::Lt:
        পক্ষ VDfgType::LtS:
        পক্ষ VDfgType::Lte:
        পক্ষ VDfgType::LteS:
        পক্ষ VDfgType::ModDiv:
        পক্ষ VDfgType::ModDivS:
        পক্ষ VDfgType::Mul:
        পক্ষ VDfgType::MulS:
        পক্ষ VDfgType::Negate:
        পক্ষ VDfgType::Neq:
        পক্ষ VDfgType::NeqCase:
        পক্ষ VDfgType::NeqWild:
        পক্ষ VDfgType::Not:
        পক্ষ VDfgType::OneHot:
        পক্ষ VDfgType::OneHot0:
        পক্ষ VDfgType::Or:
        পক্ষ VDfgType::Pow:
        পক্ষ VDfgType::PowSS:
        পক্ষ VDfgType::PowSU:
        পক্ষ VDfgType::PowUS:
        পক্ষ VDfgType::RedAnd:
        পক্ষ VDfgType::RedOr:
        পক্ষ VDfgType::RedXor:
        পক্ষ VDfgType::Rep:
        পক্ষ VDfgType::ShiftL:
        পক্ষ VDfgType::ShiftR:
        পক্ষ VDfgType::ShiftRS:
        পক্ষ VDfgType::StreamL:
        পক্ষ VDfgType::StreamR:
        পক্ষ VDfgType::Sub:
        পক্ষ VDfgType::Xor: ঘূৰাই সঁচা;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    তার্কিক sourcesEquivalent(ধ্রুৱ DfgVertex& a, ধ্রুৱ DfgVertex& b) ধ্রুৱ {
        বাবে (size_t i = 0; i < a.nInputs(); ++i) {
            ধ্রুৱ DfgVertex* ধ্রুৱ ap = a.inputp(i);
            ধ্রুৱ DfgVertex* ধ্রুৱ bp = b.inputp(i);
            যদি (!ap && !bp) অনুৱর্ত;
            যদি (!ap || !bp) ঘূৰাই মিছা;
            যদি (!vertexEquivalent(*ap, *bp)) ঘূৰাই মিছা;  // Graph is acyclic, so this terminates
        }
        ঘূৰাই সঁচা;
    }

    // Compares 'a' and 'b' for equivalence
    তার্কিক vertexEquivalent(ধ্রুৱ DfgVertex& a, ধ্রুৱ DfgVertex& b) ধ্রুৱ {
        // If same vertex, then equal
        যদি (&a == &b) ঘূৰাই সঁচা;

        // If different type, then not equal
        যদি (a.type() != b.type()) ঘূৰাই মিছা;

        // If different data type, then not equal
        যদি (a.dtype() != b.dtype()) ঘূৰাই মিছা;

        // If different number of inputs, then not equal
        যদি (a.nInputs() != b.nInputs()) ঘূৰাই মিছা;

        // Check vertex specifics
        যদি (!vertexSelfEquivalent(a, b)) ঘূৰাই মিছা;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        যদি (!a.hasMultipleSinks() && !b.hasMultipleSinks()) ঘূৰাই sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ধ্রুৱ VertexPair চাবি = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ধ্রুৱ auto it = m_cache.find(চাবি);
        যদি (it != m_cache.end()) ঘূৰাই it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ধ্রুৱ তার্কিক equal = sourcesEquivalent(a, b);
        যদি (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.ঢোকাও({চাবি, equal});

        // The predicate result
        ঘূৰাই equal;
    }
};

// Combine equivalent operation vertices
শূন্য dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    বাবে (DfgConst* ধ্রুৱ vtxp : dfg.constVertices().unlinkable()) {
        যদি (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    বাবে (DfgVertex* ধ্রুৱ vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        যদি (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            অনুৱর্ত;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ধ্রুৱ auto pair = uniqueVtxps.ঢোকাও(vtxp);
        যদি (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

শূন্য V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
