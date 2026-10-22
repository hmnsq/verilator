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
শ্রেণী DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

সর্বজনীন:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        জন্য (ধ্রুবক DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        জন্য (ধ্রুবক DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        জন্য (ধ্রুবক DfgVertex& vtx : dfg.opVertices()) {
            যদি (vtx.is<DfgCReset>() || vtx.is<Dfg_পূর্ব>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        জন্য (ধ্রুবক DfgConst& vtx : dfg.constVertices()) {
            ধ্রুবক V3Hash hash = vtx.অঙ্ক().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.মান()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    আকার_t operator()(DfgVertex* vtxp) ধ্রুবক { ফেরত vertexHash(*vtxp).মান(); }

ব্যক্তিগত:
    // Returns hash of vertex dependent on information internal to the vertex
    স্থির V3Hash vertexSelfHash(ধ্রুবক DfgVertex& vtx) {
        নির্বাচন (vtx.type()) {
        // Unhandled vertices
        ক্ষেত্র VDfgType::Logic:  // LCOV_EXCL_START
        ক্ষেত্র VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        ক্ষেত্র VDfgType::Const:  // LCOV_EXCL_START
        ক্ষেত্র VDfgType::CReset:
        ক্ষেত্র VDfgType::VarArray:
        ক্ষেত্র VDfgType::VarPacked:
        ক্ষেত্র VDfgType::পূর্ব:
        ক্ষেত্র VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        ক্ষেত্র VDfgType::Sel: ফেরত V3Hash{vtx.as<DfgSel>()->lsb()};

        ক্ষেত্র VDfgType::SpliceArray:
        ক্ষেত্র VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](ধ্রুবক DfgVertex&, uint32_t lo) {
                hash += lo;
                ফেরত মিথ্যা;
            });
            ফেরত hash;
        }

        // Vertices with no internal information
        ক্ষেত্র VDfgType::MatchMasked:
        ক্ষেত্র VDfgType::Mux:
        ক্ষেত্র VDfgType::UnitArray: ফেরত V3Hash{};

        // Generated classes - none of them have internal information
        ক্ষেত্র VDfgType::Add:
        ক্ষেত্র VDfgType::And:
        ক্ষেত্র VDfgType::ArraySel:
        ক্ষেত্র VDfgType::Concat:
        ক্ষেত্র VDfgType::Cond:
        ক্ষেত্র VDfgType::গুনতি_Ones_2:
        ক্ষেত্র VDfgType::Div:
        ক্ষেত্র VDfgType::DivS:
        ক্ষেত্র VDfgType::Eq:
        ক্ষেত্র VDfgType::EqCase:
        ক্ষেত্র VDfgType::EqWild:
        ক্ষেত্র VDfgType::Extend:
        ক্ষেত্র VDfgType::ExtendS:
        ক্ষেত্র VDfgType::Gt:
        ক্ষেত্র VDfgType::GtS:
        ক্ষেত্র VDfgType::Gte:
        ক্ষেত্র VDfgType::GteS:
        ক্ষেত্র VDfgType::LogAnd:
        ক্ষেত্র VDfgType::LogEq:
        ক্ষেত্র VDfgType::LogIf:
        ক্ষেত্র VDfgType::LogNot:
        ক্ষেত্র VDfgType::LogOr:
        ক্ষেত্র VDfgType::Lt:
        ক্ষেত্র VDfgType::LtS:
        ক্ষেত্র VDfgType::Lte:
        ক্ষেত্র VDfgType::LteS:
        ক্ষেত্র VDfgType::ModDiv:
        ক্ষেত্র VDfgType::ModDivS:
        ক্ষেত্র VDfgType::Mul:
        ক্ষেত্র VDfgType::MulS:
        ক্ষেত্র VDfgType::Negate:
        ক্ষেত্র VDfgType::Neq:
        ক্ষেত্র VDfgType::NeqCase:
        ক্ষেত্র VDfgType::NeqWild:
        ক্ষেত্র VDfgType::Not:
        ক্ষেত্র VDfgType::OneHot:
        ক্ষেত্র VDfgType::OneHot0:
        ক্ষেত্র VDfgType::Or:
        ক্ষেত্র VDfgType::Pow:
        ক্ষেত্র VDfgType::PowSS:
        ক্ষেত্র VDfgType::PowSU:
        ক্ষেত্র VDfgType::PowUS:
        ক্ষেত্র VDfgType::RedAnd:
        ক্ষেত্র VDfgType::RedOr:
        ক্ষেত্র VDfgType::RedXor:
        ক্ষেত্র VDfgType::Rep:
        ক্ষেত্র VDfgType::ShiftL:
        ক্ষেত্র VDfgType::ShiftR:
        ক্ষেত্র VDfgType::ShiftRS:
        ক্ষেত্র VDfgType::StreamL:
        ক্ষেত্র VDfgType::StreamR:
        ক্ষেত্র VDfgType::Sub:
        ক্ষেত্র VDfgType::Xor: ফেরত V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) ধ্রুবক {
        V3Hash& ফলাফল = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        যদি (!ফলাফল.মান()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.আকার_2();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                ফেরত মিথ্যা;
            });
            ফলাফল = hash;
        }
        ফেরত ফলাফল;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
শ্রেণী DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<ধ্রুবক DfgVertex*, ধ্রুবক DfgVertex*>;
    কাঠামো VertexPairHash final {
        আকার_t operator()(ধ্রুবক VertexPair& pair) ধ্রুবক {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            ফেরত hash.মান();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, যৌক্তিক, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    ধ্রুবক আকার_t m_আকার;  // Size of the graph

সর্বজনীন:
    // CONSTRUCTORS
    explicit DfgCseEqual(ধ্রুবক DfgGraph& dfg)
        : m_আকার{dfg.আকার_2()} {}

    // METHODS
    যৌক্তিক operator()(DfgVertex* ap, DfgVertex* bp) ধ্রুবক { ফেরত vertexEquivalent(*ap, *bp); }

ব্যক্তিগত:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    যৌক্তিক vertexSelfEquivalent(ধ্রুবক DfgVertex& a, ধ্রুবক DfgVertex& b) ধ্রুবক {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        নির্বাচন (a.type()) {
        // Unhandled vertices
        ক্ষেত্র VDfgType::Logic:  // LCOV_EXCL_START
        ক্ষেত্র VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        ক্ষেত্র VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        ক্ষেত্র VDfgType::Const: ফেরত a.as<DfgConst>()->অঙ্ক().isCaseEq(b.as<DfgConst>()->অঙ্ক());
        ক্ষেত্র VDfgType::CReset: ফেরত মিথ্যা;
        ক্ষেত্র VDfgType::পূর্ব: ফেরত মিথ্যা;

        ক্ষেত্র VDfgType::VarArray:
        ক্ষেত্র VDfgType::VarPacked:  // CSE does not combine variables
            ফেরত মিথ্যা;

        // Vertices with internal information
        ক্ষেত্র VDfgType::Sel: ফেরত a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        ক্ষেত্র VDfgType::SpliceArray:
        ক্ষেত্র VDfgType::SplicePacked: {
            ধ্রুবক DfgVertexSplice* ধ্রুবক ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ধ্রুবক DfgVertex&, uint32_t lo) {
                m_driverLo.ঠেলো_back(lo);
                ফেরত মিথ্যা;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.তথ্য();
            ফেরত !b.as<DfgVertexSplice>()->foreachDriver([&](ধ্রুবক DfgVertex&, uint32_t lo) {  //
                ফেরত *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        ক্ষেত্র VDfgType::MatchMasked:
        ক্ষেত্র VDfgType::Mux:
        ক্ষেত্র VDfgType::UnitArray: ফেরত সত্য;

        // Generated classes - none of them have internal information
        ক্ষেত্র VDfgType::Add:
        ক্ষেত্র VDfgType::And:
        ক্ষেত্র VDfgType::ArraySel:
        ক্ষেত্র VDfgType::Concat:
        ক্ষেত্র VDfgType::Cond:
        ক্ষেত্র VDfgType::গুনতি_Ones_2:
        ক্ষেত্র VDfgType::Div:
        ক্ষেত্র VDfgType::DivS:
        ক্ষেত্র VDfgType::Eq:
        ক্ষেত্র VDfgType::EqCase:
        ক্ষেত্র VDfgType::EqWild:
        ক্ষেত্র VDfgType::Extend:
        ক্ষেত্র VDfgType::ExtendS:
        ক্ষেত্র VDfgType::Gt:
        ক্ষেত্র VDfgType::GtS:
        ক্ষেত্র VDfgType::Gte:
        ক্ষেত্র VDfgType::GteS:
        ক্ষেত্র VDfgType::LogAnd:
        ক্ষেত্র VDfgType::LogEq:
        ক্ষেত্র VDfgType::LogIf:
        ক্ষেত্র VDfgType::LogNot:
        ক্ষেত্র VDfgType::LogOr:
        ক্ষেত্র VDfgType::Lt:
        ক্ষেত্র VDfgType::LtS:
        ক্ষেত্র VDfgType::Lte:
        ক্ষেত্র VDfgType::LteS:
        ক্ষেত্র VDfgType::ModDiv:
        ক্ষেত্র VDfgType::ModDivS:
        ক্ষেত্র VDfgType::Mul:
        ক্ষেত্র VDfgType::MulS:
        ক্ষেত্র VDfgType::Negate:
        ক্ষেত্র VDfgType::Neq:
        ক্ষেত্র VDfgType::NeqCase:
        ক্ষেত্র VDfgType::NeqWild:
        ক্ষেত্র VDfgType::Not:
        ক্ষেত্র VDfgType::OneHot:
        ক্ষেত্র VDfgType::OneHot0:
        ক্ষেত্র VDfgType::Or:
        ক্ষেত্র VDfgType::Pow:
        ক্ষেত্র VDfgType::PowSS:
        ক্ষেত্র VDfgType::PowSU:
        ক্ষেত্র VDfgType::PowUS:
        ক্ষেত্র VDfgType::RedAnd:
        ক্ষেত্র VDfgType::RedOr:
        ক্ষেত্র VDfgType::RedXor:
        ক্ষেত্র VDfgType::Rep:
        ক্ষেত্র VDfgType::ShiftL:
        ক্ষেত্র VDfgType::ShiftR:
        ক্ষেত্র VDfgType::ShiftRS:
        ক্ষেত্র VDfgType::StreamL:
        ক্ষেত্র VDfgType::StreamR:
        ক্ষেত্র VDfgType::Sub:
        ক্ষেত্র VDfgType::Xor: ফেরত সত্য;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    যৌক্তিক sourcesEquivalent(ধ্রুবক DfgVertex& a, ধ্রুবক DfgVertex& b) ধ্রুবক {
        জন্য (আকার_t i = 0; i < a.nInputs(); ++i) {
            ধ্রুবক DfgVertex* ধ্রুবক ap = a.inputp(i);
            ধ্রুবক DfgVertex* ধ্রুবক bp = b.inputp(i);
            যদি (!ap && !bp) চালু;
            যদি (!ap || !bp) ফেরত মিথ্যা;
            যদি (!vertexEquivalent(*ap, *bp)) ফেরত মিথ্যা;  // Graph is acyclic, so this terminates
        }
        ফেরত সত্য;
    }

    // Compares 'a' and 'b' for equivalence
    যৌক্তিক vertexEquivalent(ধ্রুবক DfgVertex& a, ধ্রুবক DfgVertex& b) ধ্রুবক {
        // If same vertex, then equal
        যদি (&a == &b) ফেরত সত্য;

        // If different type, then not equal
        যদি (a.type() != b.type()) ফেরত মিথ্যা;

        // If different data type, then not equal
        যদি (a.dtype() != b.dtype()) ফেরত মিথ্যা;

        // If different number of inputs, then not equal
        যদি (a.nInputs() != b.nInputs()) ফেরত মিথ্যা;

        // Check vertex specifics
        যদি (!vertexSelfEquivalent(a, b)) ফেরত মিথ্যা;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        যদি (!a.hasMultipleSinks() && !b.hasMultipleSinks()) ফেরত sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ধ্রুবক VertexPair চাবি = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ধ্রুবক auto it = m_cache.find(চাবি);
        যদি (it != m_cache.end()) ফেরত it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ধ্রুবক যৌক্তিক equal = sourcesEquivalent(a, b);
        যদি (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_আকার / 4);
        m_cache.ঢোকাও({চাবি, equal});

        // The predicate result
        ফেরত equal;
    }
};

// Combine equivalent operation vertices
শূন্য dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    জন্য (DfgConst* ধ্রুবক vtxp : dfg.constVertices().unlinkable()) {
        যদি (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.আকার_2());

    // Combine operation vertices
    জন্য (DfgVertex* ধ্রুবক vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        যদি (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            চালু;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ধ্রুবক auto pair = uniqueVtxps.ঢোকাও(vtxp);
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
