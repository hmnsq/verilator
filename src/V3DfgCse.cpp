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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"
#અન્તર્ભાવ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
વર્ગ DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

જાહેર:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        માટે (અચલ DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        માટે (અચલ DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        માટે (અચલ DfgVertex& vtx : dfg.opVertices()) {
            જો (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        માટે (અચલ DfgConst& vtx : dfg.constVertices()) {
            અચલ V3Hash hash = vtx.અંક().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.કિંમત()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) અચલ { પાછા vertexHash(*vtxp).કિંમત(); }

ખાનગી:
    // Returns hash of vertex dependent on information internal to the vertex
    સ્થિર V3Hash vertexSelfHash(અચલ DfgVertex& vtx) {
        પસંદગી (vtx.type()) {
        // Unhandled vertices
        સ્થિતિ VDfgType::Logic:  // LCOV_EXCL_START
        સ્થિતિ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        સ્થિતિ VDfgType::Const:  // LCOV_EXCL_START
        સ્થિતિ VDfgType::CReset:
        સ્થિતિ VDfgType::VarArray:
        સ્થિતિ VDfgType::VarPacked:
        સ્થિતિ VDfgType::Prev:
        સ્થિતિ VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        સ્થિતિ VDfgType::Sel: પાછા V3Hash{vtx.as<DfgSel>()->lsb()};

        સ્થિતિ VDfgType::SpliceArray:
        સ્થિતિ VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](અચલ DfgVertex&, uint32_t lo) {
                hash += lo;
                પાછા ખોટું;
            });
            પાછા hash;
        }

        // Vertices with no internal information
        સ્થિતિ VDfgType::MatchMasked:
        સ્થિતિ VDfgType::Mux:
        સ્થિતિ VDfgType::UnitArray: પાછા V3Hash{};

        // Generated classes - none of them have internal information
        સ્થિતિ VDfgType::Add:
        સ્થિતિ VDfgType::And:
        સ્થિતિ VDfgType::ArraySel:
        સ્થિતિ VDfgType::Concat:
        સ્થિતિ VDfgType::Cond:
        સ્થિતિ VDfgType::ગણતરી_Ones_2:
        સ્થિતિ VDfgType::Div:
        સ્થિતિ VDfgType::DivS:
        સ્થિતિ VDfgType::Eq:
        સ્થિતિ VDfgType::EqCase:
        સ્થિતિ VDfgType::EqWild:
        સ્થિતિ VDfgType::Extend:
        સ્થિતિ VDfgType::ExtendS:
        સ્થિતિ VDfgType::Gt:
        સ્થિતિ VDfgType::GtS:
        સ્થિતિ VDfgType::Gte:
        સ્થિતિ VDfgType::GteS:
        સ્થિતિ VDfgType::LogAnd:
        સ્થિતિ VDfgType::LogEq:
        સ્થિતિ VDfgType::LogIf:
        સ્થિતિ VDfgType::LogNot:
        સ્થિતિ VDfgType::LogOr:
        સ્થિતિ VDfgType::Lt:
        સ્થિતિ VDfgType::LtS:
        સ્થિતિ VDfgType::Lte:
        સ્થિતિ VDfgType::LteS:
        સ્થિતિ VDfgType::ModDiv:
        સ્થિતિ VDfgType::ModDivS:
        સ્થિતિ VDfgType::Mul:
        સ્થિતિ VDfgType::MulS:
        સ્થિતિ VDfgType::Negate:
        સ્થિતિ VDfgType::Neq:
        સ્થિતિ VDfgType::NeqCase:
        સ્થિતિ VDfgType::NeqWild:
        સ્થિતિ VDfgType::Not:
        સ્થિતિ VDfgType::OneHot:
        સ્થિતિ VDfgType::OneHot0:
        સ્થિતિ VDfgType::Or:
        સ્થિતિ VDfgType::Pow:
        સ્થિતિ VDfgType::PowSS:
        સ્થિતિ VDfgType::PowSU:
        સ્થિતિ VDfgType::PowUS:
        સ્થિતિ VDfgType::RedAnd:
        સ્થિતિ VDfgType::RedOr:
        સ્થિતિ VDfgType::RedXor:
        સ્થિતિ VDfgType::Rep:
        સ્થિતિ VDfgType::ShiftL:
        સ્થિતિ VDfgType::ShiftR:
        સ્થિતિ VDfgType::ShiftRS:
        સ્થિતિ VDfgType::StreamL:
        સ્થિતિ VDfgType::StreamR:
        સ્થિતિ VDfgType::Sub:
        સ્થિતિ VDfgType::Xor: પાછા V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) અચલ {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        જો (!result.કિંમત()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                પાછા ખોટું;
            });
            result = hash;
        }
        પાછા result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
વર્ગ DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<અચલ DfgVertex*, અચલ DfgVertex*>;
    સંરચના VertexPairHash final {
        size_t operator()(અચલ VertexPair& pair) અચલ {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            પાછા hash.કિંમત();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, તાર્કિક, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    અચલ size_t m_size;  // Size of the graph

જાહેર:
    // CONSTRUCTORS
    explicit DfgCseEqual(અચલ DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    તાર્કિક operator()(DfgVertex* ap, DfgVertex* bp) અચલ { પાછા vertexEquivalent(*ap, *bp); }

ખાનગી:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    તાર્કિક vertexSelfEquivalent(અચલ DfgVertex& a, અચલ DfgVertex& b) અચલ {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        પસંદગી (a.type()) {
        // Unhandled vertices
        સ્થિતિ VDfgType::Logic:  // LCOV_EXCL_START
        સ્થિતિ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        સ્થિતિ VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        સ્થિતિ VDfgType::Const: પાછા a.as<DfgConst>()->અંક().isCaseEq(b.as<DfgConst>()->અંક());
        સ્થિતિ VDfgType::CReset: પાછા ખોટું;
        સ્થિતિ VDfgType::Prev: પાછા ખોટું;

        સ્થિતિ VDfgType::VarArray:
        સ્થિતિ VDfgType::VarPacked:  // CSE does not combine variables
            પાછા ખોટું;

        // Vertices with internal information
        સ્થિતિ VDfgType::Sel: પાછા a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        સ્થિતિ VDfgType::SpliceArray:
        સ્થિતિ VDfgType::SplicePacked: {
            અચલ DfgVertexSplice* અચલ ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](અચલ DfgVertex&, uint32_t lo) {
                m_driverLo.ધકેલો_back(lo);
                પાછા ખોટું;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            પાછા !b.as<DfgVertexSplice>()->foreachDriver([&](અચલ DfgVertex&, uint32_t lo) {  //
                પાછા *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        સ્થિતિ VDfgType::MatchMasked:
        સ્થિતિ VDfgType::Mux:
        સ્થિતિ VDfgType::UnitArray: પાછા સાચું;

        // Generated classes - none of them have internal information
        સ્થિતિ VDfgType::Add:
        સ્થિતિ VDfgType::And:
        સ્થિતિ VDfgType::ArraySel:
        સ્થિતિ VDfgType::Concat:
        સ્થિતિ VDfgType::Cond:
        સ્થિતિ VDfgType::ગણતરી_Ones_2:
        સ્થિતિ VDfgType::Div:
        સ્થિતિ VDfgType::DivS:
        સ્થિતિ VDfgType::Eq:
        સ્થિતિ VDfgType::EqCase:
        સ્થિતિ VDfgType::EqWild:
        સ્થિતિ VDfgType::Extend:
        સ્થિતિ VDfgType::ExtendS:
        સ્થિતિ VDfgType::Gt:
        સ્થિતિ VDfgType::GtS:
        સ્થિતિ VDfgType::Gte:
        સ્થિતિ VDfgType::GteS:
        સ્થિતિ VDfgType::LogAnd:
        સ્થિતિ VDfgType::LogEq:
        સ્થિતિ VDfgType::LogIf:
        સ્થિતિ VDfgType::LogNot:
        સ્થિતિ VDfgType::LogOr:
        સ્થિતિ VDfgType::Lt:
        સ્થિતિ VDfgType::LtS:
        સ્થિતિ VDfgType::Lte:
        સ્થિતિ VDfgType::LteS:
        સ્થિતિ VDfgType::ModDiv:
        સ્થિતિ VDfgType::ModDivS:
        સ્થિતિ VDfgType::Mul:
        સ્થિતિ VDfgType::MulS:
        સ્થિતિ VDfgType::Negate:
        સ્થિતિ VDfgType::Neq:
        સ્થિતિ VDfgType::NeqCase:
        સ્થિતિ VDfgType::NeqWild:
        સ્થિતિ VDfgType::Not:
        સ્થિતિ VDfgType::OneHot:
        સ્થિતિ VDfgType::OneHot0:
        સ્થિતિ VDfgType::Or:
        સ્થિતિ VDfgType::Pow:
        સ્થિતિ VDfgType::PowSS:
        સ્થિતિ VDfgType::PowSU:
        સ્થિતિ VDfgType::PowUS:
        સ્થિતિ VDfgType::RedAnd:
        સ્થિતિ VDfgType::RedOr:
        સ્થિતિ VDfgType::RedXor:
        સ્થિતિ VDfgType::Rep:
        સ્થિતિ VDfgType::ShiftL:
        સ્થિતિ VDfgType::ShiftR:
        સ્થિતિ VDfgType::ShiftRS:
        સ્થિતિ VDfgType::StreamL:
        સ્થિતિ VDfgType::StreamR:
        સ્થિતિ VDfgType::Sub:
        સ્થિતિ VDfgType::Xor: પાછા સાચું;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    તાર્કિક sourcesEquivalent(અચલ DfgVertex& a, અચલ DfgVertex& b) અચલ {
        માટે (size_t i = 0; i < a.nInputs(); ++i) {
            અચલ DfgVertex* અચલ ap = a.inputp(i);
            અચલ DfgVertex* અચલ bp = b.inputp(i);
            જો (!ap && !bp) ચાલુ;
            જો (!ap || !bp) પાછા ખોટું;
            જો (!vertexEquivalent(*ap, *bp)) પાછા ખોટું;  // Graph is acyclic, so this terminates
        }
        પાછા સાચું;
    }

    // Compares 'a' and 'b' for equivalence
    તાર્કિક vertexEquivalent(અચલ DfgVertex& a, અચલ DfgVertex& b) અચલ {
        // If same vertex, then equal
        જો (&a == &b) પાછા સાચું;

        // If different type, then not equal
        જો (a.type() != b.type()) પાછા ખોટું;

        // If different data type, then not equal
        જો (a.dtype() != b.dtype()) પાછા ખોટું;

        // If different number of inputs, then not equal
        જો (a.nInputs() != b.nInputs()) પાછા ખોટું;

        // Check vertex specifics
        જો (!vertexSelfEquivalent(a, b)) પાછા ખોટું;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        જો (!a.hasMultipleSinks() && !b.hasMultipleSinks()) પાછા sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        અચલ VertexPair ચાવી = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        અચલ auto it = m_cache.find(ચાવી);
        જો (it != m_cache.end()) પાછા it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        અચલ તાર્કિક equal = sourcesEquivalent(a, b);
        જો (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.ઉમેરો({ચાવી, equal});

        // The predicate result
        પાછા equal;
    }
};

// Combine equivalent operation vertices
શૂન્ય dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    માટે (DfgConst* અચલ vtxp : dfg.constVertices().unlinkable()) {
        જો (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    માટે (DfgVertex* અચલ vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        જો (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            ચાલુ;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        અચલ auto pair = uniqueVtxps.ઉમેરો(vtxp);
        જો (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

શૂન્ય V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
