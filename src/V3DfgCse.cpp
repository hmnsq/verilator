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
        for (const DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        for (const DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        for (const DfgVertex& vtx : dfg.opVertices()) {
            ᬬᬾᬦ᭄ (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        for (const DfgConst& vtx : dfg.constVertices()) {
            const V3Hash hash = vtx.num().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.value()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) const { ᬯᬮᬶ vertexHash(*vtxp).value(); }

private:
    // Returns hash of vertex dependent on information internal to the vertex
    static V3Hash vertexSelfHash(const DfgVertex& vtx) {
        switch (vtx.type()) {
        // Unhandled vertices
        pak_sa VDfgType::Logic:  // LCOV_EXCL_START
        pak_sa VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        pak_sa VDfgType::Const:  // LCOV_EXCL_START
        pak_sa VDfgType::CReset:
        pak_sa VDfgType::VarArray:
        pak_sa VDfgType::VarPacked:
        pak_sa VDfgType::Prev:
        pak_sa VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        pak_sa VDfgType::Sel: ᬯᬮᬶ V3Hash{vtx.as<DfgSel>()->lsb()};

        pak_sa VDfgType::SpliceArray:
        pak_sa VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](const DfgVertex&, uint32_t lo) {
                hash += lo;
                ᬯᬮᬶ false;
            });
            ᬯᬮᬶ hash;
        }

        // Vertices with no internal information
        pak_sa VDfgType::MatchMasked:
        pak_sa VDfgType::Mux:
        pak_sa VDfgType::UnitArray: ᬯᬮᬶ V3Hash{};

        // Generated classes - none of them have internal information
        pak_sa VDfgType::Add:
        pak_sa VDfgType::And:
        pak_sa VDfgType::ArraySel:
        pak_sa VDfgType::Concat:
        pak_sa VDfgType::Cond:
        pak_sa VDfgType::CountOnes:
        pak_sa VDfgType::Div:
        pak_sa VDfgType::DivS:
        pak_sa VDfgType::Eq:
        pak_sa VDfgType::EqCase:
        pak_sa VDfgType::EqWild:
        pak_sa VDfgType::Extend:
        pak_sa VDfgType::ExtendS:
        pak_sa VDfgType::Gt:
        pak_sa VDfgType::GtS:
        pak_sa VDfgType::Gte:
        pak_sa VDfgType::GteS:
        pak_sa VDfgType::LogAnd:
        pak_sa VDfgType::LogEq:
        pak_sa VDfgType::LogIf:
        pak_sa VDfgType::LogNot:
        pak_sa VDfgType::LogOr:
        pak_sa VDfgType::Lt:
        pak_sa VDfgType::LtS:
        pak_sa VDfgType::Lte:
        pak_sa VDfgType::LteS:
        pak_sa VDfgType::ModDiv:
        pak_sa VDfgType::ModDivS:
        pak_sa VDfgType::Mul:
        pak_sa VDfgType::MulS:
        pak_sa VDfgType::Negate:
        pak_sa VDfgType::Neq:
        pak_sa VDfgType::NeqCase:
        pak_sa VDfgType::NeqWild:
        pak_sa VDfgType::Not:
        pak_sa VDfgType::OneHot:
        pak_sa VDfgType::OneHot0:
        pak_sa VDfgType::Or:
        pak_sa VDfgType::Pow:
        pak_sa VDfgType::PowSS:
        pak_sa VDfgType::PowSU:
        pak_sa VDfgType::PowUS:
        pak_sa VDfgType::RedAnd:
        pak_sa VDfgType::RedOr:
        pak_sa VDfgType::RedXor:
        pak_sa VDfgType::Rep:
        pak_sa VDfgType::ShiftL:
        pak_sa VDfgType::ShiftR:
        pak_sa VDfgType::ShiftRS:
        pak_sa VDfgType::StreamL:
        pak_sa VDfgType::StreamR:
        pak_sa VDfgType::Sub:
        pak_sa VDfgType::Xor: ᬯᬮᬶ V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) const {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        ᬬᬾᬦ᭄ (!result.value()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                ᬯᬮᬶ false;
            });
            result = hash;
        }
        ᬯᬮᬶ result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
class DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<const DfgVertex*, const DfgVertex*>;
    sa_nracanaa VertexPairHash final {
        size_t operator()(const VertexPair& pair) const {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            ᬯᬮᬶ hash.value();
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
    bool operator()(DfgVertex* ap, DfgVertex* bp) const { ᬯᬮᬶ vertexEquivalent(*ap, *bp); }

private:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    bool vertexSelfEquivalent(const DfgVertex& a, const DfgVertex& b) const {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        switch (a.type()) {
        // Unhandled vertices
        pak_sa VDfgType::Logic:  // LCOV_EXCL_START
        pak_sa VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        pak_sa VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        pak_sa VDfgType::Const: ᬯᬮᬶ a.as<DfgConst>()->num().isCaseEq(b.as<DfgConst>()->num());
        pak_sa VDfgType::CReset: ᬯᬮᬶ false;
        pak_sa VDfgType::Prev: ᬯᬮᬶ false;

        pak_sa VDfgType::VarArray:
        pak_sa VDfgType::VarPacked:  // CSE does not combine variables
            ᬯᬮᬶ false;

        // Vertices with internal information
        pak_sa VDfgType::Sel: ᬯᬮᬶ a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        pak_sa VDfgType::SpliceArray:
        pak_sa VDfgType::SplicePacked: {
            const DfgVertexSplice* const ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](const DfgVertex&, uint32_t lo) {
                m_driverLo.push_back(lo);
                ᬯᬮᬶ false;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            ᬯᬮᬶ !b.as<DfgVertexSplice>()->foreachDriver([&](const DfgVertex&, uint32_t lo) {  //
                ᬯᬮᬶ *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        pak_sa VDfgType::MatchMasked:
        pak_sa VDfgType::Mux:
        pak_sa VDfgType::UnitArray: ᬯᬮᬶ true;

        // Generated classes - none of them have internal information
        pak_sa VDfgType::Add:
        pak_sa VDfgType::And:
        pak_sa VDfgType::ArraySel:
        pak_sa VDfgType::Concat:
        pak_sa VDfgType::Cond:
        pak_sa VDfgType::CountOnes:
        pak_sa VDfgType::Div:
        pak_sa VDfgType::DivS:
        pak_sa VDfgType::Eq:
        pak_sa VDfgType::EqCase:
        pak_sa VDfgType::EqWild:
        pak_sa VDfgType::Extend:
        pak_sa VDfgType::ExtendS:
        pak_sa VDfgType::Gt:
        pak_sa VDfgType::GtS:
        pak_sa VDfgType::Gte:
        pak_sa VDfgType::GteS:
        pak_sa VDfgType::LogAnd:
        pak_sa VDfgType::LogEq:
        pak_sa VDfgType::LogIf:
        pak_sa VDfgType::LogNot:
        pak_sa VDfgType::LogOr:
        pak_sa VDfgType::Lt:
        pak_sa VDfgType::LtS:
        pak_sa VDfgType::Lte:
        pak_sa VDfgType::LteS:
        pak_sa VDfgType::ModDiv:
        pak_sa VDfgType::ModDivS:
        pak_sa VDfgType::Mul:
        pak_sa VDfgType::MulS:
        pak_sa VDfgType::Negate:
        pak_sa VDfgType::Neq:
        pak_sa VDfgType::NeqCase:
        pak_sa VDfgType::NeqWild:
        pak_sa VDfgType::Not:
        pak_sa VDfgType::OneHot:
        pak_sa VDfgType::OneHot0:
        pak_sa VDfgType::Or:
        pak_sa VDfgType::Pow:
        pak_sa VDfgType::PowSS:
        pak_sa VDfgType::PowSU:
        pak_sa VDfgType::PowUS:
        pak_sa VDfgType::RedAnd:
        pak_sa VDfgType::RedOr:
        pak_sa VDfgType::RedXor:
        pak_sa VDfgType::Rep:
        pak_sa VDfgType::ShiftL:
        pak_sa VDfgType::ShiftR:
        pak_sa VDfgType::ShiftRS:
        pak_sa VDfgType::StreamL:
        pak_sa VDfgType::StreamR:
        pak_sa VDfgType::Sub:
        pak_sa VDfgType::Xor: ᬯᬮᬶ true;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    bool sourcesEquivalent(const DfgVertex& a, const DfgVertex& b) const {
        for (size_t i = 0; i < a.nInputs(); ++i) {
            const DfgVertex* const ap = a.inputp(i);
            const DfgVertex* const bp = b.inputp(i);
            ᬬᬾᬦ᭄ (!ap && !bp) continue;
            ᬬᬾᬦ᭄ (!ap || !bp) ᬯᬮᬶ false;
            ᬬᬾᬦ᭄ (!vertexEquivalent(*ap, *bp)) ᬯᬮᬶ false;  // Graph is acyclic, so this terminates
        }
        ᬯᬮᬶ true;
    }

    // Compares 'a' and 'b' for equivalence
    bool vertexEquivalent(const DfgVertex& a, const DfgVertex& b) const {
        // If same vertex, then equal
        ᬬᬾᬦ᭄ (&a == &b) ᬯᬮᬶ true;

        // If different type, then not equal
        ᬬᬾᬦ᭄ (a.type() != b.type()) ᬯᬮᬶ false;

        // If different data type, then not equal
        ᬬᬾᬦ᭄ (a.dtype() != b.dtype()) ᬯᬮᬶ false;

        // If different number of inputs, then not equal
        ᬬᬾᬦ᭄ (a.nInputs() != b.nInputs()) ᬯᬮᬶ false;

        // Check vertex specifics
        ᬬᬾᬦ᭄ (!vertexSelfEquivalent(a, b)) ᬯᬮᬶ false;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        ᬬᬾᬦ᭄ (!a.hasMultipleSinks() && !b.hasMultipleSinks()) ᬯᬮᬶ sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        const VertexPair key = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        const auto it = m_cache.find(key);
        ᬬᬾᬦ᭄ (it != m_cache.end()) ᬯᬮᬶ it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        const bool equal = sourcesEquivalent(a, b);
        ᬬᬾᬦ᭄ (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.insert({key, equal});

        // The predicate result
        ᬯᬮᬶ equal;
    }
};

// Combine equivalent operation vertices
void dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    for (DfgConst* const vtxp : dfg.constVertices().unlinkable()) {
        ᬬᬾᬦ᭄ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    for (DfgVertex* const vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        ᬬᬾᬦ᭄ (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            continue;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        const auto pair = uniqueVtxps.insert(vtxp);
        ᬬᬾᬦ᭄ (!pair.second) {
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
