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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Dfg.h"
#include "V3DfgPasses.h"
#include "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
class DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

public:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        за (const DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        за (const DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        за (const DfgVertex& vtx : dfg.opVertices()) {
            ако (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        за (const DfgConst& vtx : dfg.constVertices()) {
            const V3Hash hash = vtx.num().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.value()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) const { врати vertexHash(*vtxp).value(); }

private:
    // Returns hash of vertex dependent on information internal to the vertex
    static V3Hash vertexSelfHash(const DfgVertex& vtx) {
        switch (vtx.type()) {
        // Unhandled vertices
        case VDfgType::Logic:  // LCOV_EXCL_START
        case VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        case VDfgType::Const:  // LCOV_EXCL_START
        case VDfgType::CReset:
        case VDfgType::VarArray:
        case VDfgType::VarPacked:
        case VDfgType::Prev:
        case VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        case VDfgType::Sel: врати V3Hash{vtx.as<DfgSel>()->lsb()};

        case VDfgType::SpliceArray:
        case VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](const DfgVertex&, uint32_t lo) {
                hash += lo;
                врати false;
            });
            врати hash;
        }

        // Vertices with no internal information
        case VDfgType::MatchMasked:
        case VDfgType::Mux:
        case VDfgType::UnitArray: врати V3Hash{};

        // Generated classes - none of them have internal information
        case VDfgType::Add:
        case VDfgType::And:
        case VDfgType::ArraySel:
        case VDfgType::Concat:
        case VDfgType::Cond:
        case VDfgType::CountOnes:
        case VDfgType::Div:
        case VDfgType::DivS:
        case VDfgType::Eq:
        case VDfgType::EqCase:
        case VDfgType::EqWild:
        case VDfgType::Extend:
        case VDfgType::ExtendS:
        case VDfgType::Gt:
        case VDfgType::GtS:
        case VDfgType::Gte:
        case VDfgType::GteS:
        case VDfgType::LogAnd:
        case VDfgType::LogEq:
        case VDfgType::LogIf:
        case VDfgType::LogNot:
        case VDfgType::LogOr:
        case VDfgType::Lt:
        case VDfgType::LtS:
        case VDfgType::Lte:
        case VDfgType::LteS:
        case VDfgType::ModDiv:
        case VDfgType::ModDivS:
        case VDfgType::Mul:
        case VDfgType::MulS:
        case VDfgType::Negate:
        case VDfgType::Neq:
        case VDfgType::NeqCase:
        case VDfgType::NeqWild:
        case VDfgType::Not:
        case VDfgType::OneHot:
        case VDfgType::OneHot0:
        case VDfgType::Or:
        case VDfgType::Pow:
        case VDfgType::PowSS:
        case VDfgType::PowSU:
        case VDfgType::PowUS:
        case VDfgType::RedAnd:
        case VDfgType::RedOr:
        case VDfgType::RedXor:
        case VDfgType::Rep:
        case VDfgType::ShiftL:
        case VDfgType::ShiftR:
        case VDfgType::ShiftRS:
        case VDfgType::StreamL:
        case VDfgType::StreamR:
        case VDfgType::Sub:
        case VDfgType::Xor: врати V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) const {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        ако (!result.value()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                врати false;
            });
            result = hash;
        }
        врати result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
class DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<const DfgVertex*, const DfgVertex*>;
    struct VertexPairHash final {
        size_t operator()(const VertexPair& pair) const {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            врати hash.value();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, bool, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    const size_t m_size;  // Size of the graph

public:
    // CONSTRUCTORS
    explicit DfgCseEqual(const DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    bool operator()(DfgVertex* ap, DfgVertex* bp) const { врати vertexEquivalent(*ap, *bp); }

private:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    bool vertexSelfEquivalent(const DfgVertex& a, const DfgVertex& b) const {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        switch (a.type()) {
        // Unhandled vertices
        case VDfgType::Logic:  // LCOV_EXCL_START
        case VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        case VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        case VDfgType::Const: врати a.as<DfgConst>()->num().isCaseEq(b.as<DfgConst>()->num());
        case VDfgType::CReset: врати false;
        case VDfgType::Prev: врати false;

        case VDfgType::VarArray:
        case VDfgType::VarPacked:  // CSE does not combine variables
            врати false;

        // Vertices with internal information
        case VDfgType::Sel: врати a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        case VDfgType::SpliceArray:
        case VDfgType::SplicePacked: {
            const DfgVertexSplice* const ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](const DfgVertex&, uint32_t lo) {
                m_driverLo.push_back(lo);
                врати false;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            врати !b.as<DfgVertexSplice>()->foreachDriver([&](const DfgVertex&, uint32_t lo) {  //
                врати *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        case VDfgType::MatchMasked:
        case VDfgType::Mux:
        case VDfgType::UnitArray: врати true;

        // Generated classes - none of them have internal information
        case VDfgType::Add:
        case VDfgType::And:
        case VDfgType::ArraySel:
        case VDfgType::Concat:
        case VDfgType::Cond:
        case VDfgType::CountOnes:
        case VDfgType::Div:
        case VDfgType::DivS:
        case VDfgType::Eq:
        case VDfgType::EqCase:
        case VDfgType::EqWild:
        case VDfgType::Extend:
        case VDfgType::ExtendS:
        case VDfgType::Gt:
        case VDfgType::GtS:
        case VDfgType::Gte:
        case VDfgType::GteS:
        case VDfgType::LogAnd:
        case VDfgType::LogEq:
        case VDfgType::LogIf:
        case VDfgType::LogNot:
        case VDfgType::LogOr:
        case VDfgType::Lt:
        case VDfgType::LtS:
        case VDfgType::Lte:
        case VDfgType::LteS:
        case VDfgType::ModDiv:
        case VDfgType::ModDivS:
        case VDfgType::Mul:
        case VDfgType::MulS:
        case VDfgType::Negate:
        case VDfgType::Neq:
        case VDfgType::NeqCase:
        case VDfgType::NeqWild:
        case VDfgType::Not:
        case VDfgType::OneHot:
        case VDfgType::OneHot0:
        case VDfgType::Or:
        case VDfgType::Pow:
        case VDfgType::PowSS:
        case VDfgType::PowSU:
        case VDfgType::PowUS:
        case VDfgType::RedAnd:
        case VDfgType::RedOr:
        case VDfgType::RedXor:
        case VDfgType::Rep:
        case VDfgType::ShiftL:
        case VDfgType::ShiftR:
        case VDfgType::ShiftRS:
        case VDfgType::StreamL:
        case VDfgType::StreamR:
        case VDfgType::Sub:
        case VDfgType::Xor: врати true;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    bool sourcesEquivalent(const DfgVertex& a, const DfgVertex& b) const {
        за (size_t i = 0; i < a.nInputs(); ++i) {
            const DfgVertex* const ap = a.inputp(i);
            const DfgVertex* const bp = b.inputp(i);
            ако (!ap && !bp) continue;
            ако (!ap || !bp) врати false;
            ако (!vertexEquivalent(*ap, *bp)) врати false;  // Graph is acyclic, so this terminates
        }
        врати true;
    }

    // Compares 'a' and 'b' for equivalence
    bool vertexEquivalent(const DfgVertex& a, const DfgVertex& b) const {
        // If same vertex, then equal
        ако (&a == &b) врати true;

        // If different type, then not equal
        ако (a.type() != b.type()) врати false;

        // If different data type, then not equal
        ако (a.dtype() != b.dtype()) врати false;

        // If different number of inputs, then not equal
        ако (a.nInputs() != b.nInputs()) врати false;

        // Check vertex specifics
        ако (!vertexSelfEquivalent(a, b)) врати false;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        ако (!a.hasMultipleSinks() && !b.hasMultipleSinks()) врати sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        const VertexPair key = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        const auto it = m_cache.find(key);
        ако (it != m_cache.end()) врати it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        const bool equal = sourcesEquivalent(a, b);
        ако (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.insert({key, equal});

        // The predicate result
        врати equal;
    }
};

// Combine equivalent operation vertices
void dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    за (DfgConst* const vtxp : dfg.constVertices().unlinkable()) {
        ако (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    за (DfgVertex* const vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        ако (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            continue;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        const auto pair = uniqueVtxps.insert(vtxp);
        ако (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

void V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
