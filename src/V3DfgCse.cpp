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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"
#অন্তর্ভাব "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
বর্গ DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

সার্বজনিক:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        প্রতি (ধ্রুব DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        প্রতি (ধ্রুব DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        প্রতি (ধ্রুব DfgVertex& vtx : dfg.opVertices()) {
            যদি (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        প্রতি (ধ্রুব DfgConst& vtx : dfg.constVertices()) {
            ধ্রুব V3Hash hash = vtx.অঙ্ক().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.মান()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) ধ্রুব { প্রত্যাগম vertexHash(*vtxp).মান(); }

গুপ্ত:
    // Returns hash of vertex dependent on information internal to the vertex
    স্থির V3Hash vertexSelfHash(ধ্রুব DfgVertex& vtx) {
        বিকল্প (vtx.type()) {
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
        পক্ষ VDfgType::Sel: প্রত্যাগম V3Hash{vtx.as<DfgSel>()->lsb()};

        পক্ষ VDfgType::SpliceArray:
        পক্ষ VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](ধ্রুব DfgVertex&, uint32_t lo) {
                hash += lo;
                প্রত্যাগম অসত্য;
            });
            প্রত্যাগম hash;
        }

        // Vertices with no internal information
        পক্ষ VDfgType::MatchMasked:
        পক্ষ VDfgType::Mux:
        পক্ষ VDfgType::UnitArray: প্রত্যাগম V3Hash{};

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
        পক্ষ VDfgType::Xor: প্রত্যাগম V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) ধ্রুব {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        যদি (!result.মান()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                প্রত্যাগম অসত্য;
            });
            result = hash;
        }
        প্রত্যাগম result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
বর্গ DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<ধ্রুব DfgVertex*, ধ্রুব DfgVertex*>;
    সংরচনা VertexPairHash final {
        size_t operator()(ধ্রুব VertexPair& pair) ধ্রুব {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            প্রত্যাগম hash.মান();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, তার্কিক, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    ধ্রুব size_t m_size;  // Size of the graph

সার্বজনিক:
    // CONSTRUCTORS
    explicit DfgCseEqual(ধ্রুব DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    তার্কিক operator()(DfgVertex* ap, DfgVertex* bp) ধ্রুব { প্রত্যাগম vertexEquivalent(*ap, *bp); }

গুপ্ত:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    তার্কিক vertexSelfEquivalent(ধ্রুব DfgVertex& a, ধ্রুব DfgVertex& b) ধ্রুব {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        বিকল্প (a.type()) {
        // Unhandled vertices
        পক্ষ VDfgType::Logic:  // LCOV_EXCL_START
        পক্ষ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        পক্ষ VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        পক্ষ VDfgType::Const: প্রত্যাগম a.as<DfgConst>()->অঙ্ক().isCaseEq(b.as<DfgConst>()->অঙ্ক());
        পক্ষ VDfgType::CReset: প্রত্যাগম অসত্য;
        পক্ষ VDfgType::Prev: প্রত্যাগম অসত্য;

        পক্ষ VDfgType::VarArray:
        পক্ষ VDfgType::VarPacked:  // CSE does not combine variables
            প্রত্যাগম অসত্য;

        // Vertices with internal information
        পক্ষ VDfgType::Sel: প্রত্যাগম a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        পক্ষ VDfgType::SpliceArray:
        পক্ষ VDfgType::SplicePacked: {
            ধ্রুব DfgVertexSplice* ধ্রুব ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ধ্রুব DfgVertex&, uint32_t lo) {
                m_driverLo.ঠেলো_back(lo);
                প্রত্যাগম অসত্য;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            প্রত্যাগম !b.as<DfgVertexSplice>()->foreachDriver([&](ধ্রুব DfgVertex&, uint32_t lo) {  //
                প্রত্যাগম *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        পক্ষ VDfgType::MatchMasked:
        পক্ষ VDfgType::Mux:
        পক্ষ VDfgType::UnitArray: প্রত্যাগম সত্য;

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
        পক্ষ VDfgType::Xor: প্রত্যাগম সত্য;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    তার্কিক sourcesEquivalent(ধ্রুব DfgVertex& a, ধ্রুব DfgVertex& b) ধ্রুব {
        প্রতি (size_t i = 0; i < a.nInputs(); ++i) {
            ধ্রুব DfgVertex* ধ্রুব ap = a.inputp(i);
            ধ্রুব DfgVertex* ধ্রুব bp = b.inputp(i);
            যদি (!ap && !bp) অনুবর্ত;
            যদি (!ap || !bp) প্রত্যাগম অসত্য;
            যদি (!vertexEquivalent(*ap, *bp)) প্রত্যাগম অসত্য;  // Graph is acyclic, so this terminates
        }
        প্রত্যাগম সত্য;
    }

    // Compares 'a' and 'b' for equivalence
    তার্কিক vertexEquivalent(ধ্রুব DfgVertex& a, ধ্রুব DfgVertex& b) ধ্রুব {
        // If same vertex, then equal
        যদি (&a == &b) প্রত্যাগম সত্য;

        // If different type, then not equal
        যদি (a.type() != b.type()) প্রত্যাগম অসত্য;

        // If different data type, then not equal
        যদি (a.dtype() != b.dtype()) প্রত্যাগম অসত্য;

        // If different number of inputs, then not equal
        যদি (a.nInputs() != b.nInputs()) প্রত্যাগম অসত্য;

        // Check vertex specifics
        যদি (!vertexSelfEquivalent(a, b)) প্রত্যাগম অসত্য;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        যদি (!a.hasMultipleSinks() && !b.hasMultipleSinks()) প্রত্যাগম sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ধ্রুব VertexPair চাবি = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ধ্রুব auto it = m_cache.find(চাবি);
        যদি (it != m_cache.end()) প্রত্যাগম it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ধ্রুব তার্কিক equal = sourcesEquivalent(a, b);
        যদি (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.ঢোকাও({চাবি, equal});

        // The predicate result
        প্রত্যাগম equal;
    }
};

// Combine equivalent operation vertices
শূন্য dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    প্রতি (DfgConst* ধ্রুব vtxp : dfg.constVertices().unlinkable()) {
        যদি (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    প্রতি (DfgVertex* ধ্রুব vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        যদি (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            অনুবর্ত;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ধ্রুব auto pair = uniqueVtxps.ঢোকাও(vtxp);
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
