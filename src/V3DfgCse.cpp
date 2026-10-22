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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
صنف DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

عام:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        لكل (ثابت DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        لكل (ثابت DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        لكل (ثابت DfgVertex& vtx : dfg.opVertices()) {
            ܐܢ (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        لكل (ثابت DfgConst& vtx : dfg.constVertices()) {
            ثابت V3Hash hash = vtx.num().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.value()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) ثابت { ܦܢܐ vertexHash(*vtxp).value(); }

خاص:
    // Returns hash of vertex dependent on information internal to the vertex
    ساكن V3Hash vertexSelfHash(ثابت DfgVertex& vtx) {
        بدل (vtx.type()) {
        // Unhandled vertices
        حالة VDfgType::Logic:  // LCOV_EXCL_START
        حالة VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        حالة VDfgType::Const:  // LCOV_EXCL_START
        حالة VDfgType::CReset:
        حالة VDfgType::VarArray:
        حالة VDfgType::VarPacked:
        حالة VDfgType::Prev:
        حالة VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        حالة VDfgType::Sel: ܦܢܐ V3Hash{vtx.as<DfgSel>()->lsb()};

        حالة VDfgType::SpliceArray:
        حالة VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](ثابت DfgVertex&, uint32_t lo) {
                hash += lo;
                ܦܢܐ ܕܓܠܐ;
            });
            ܦܢܐ hash;
        }

        // Vertices with no internal information
        حالة VDfgType::MatchMasked:
        حالة VDfgType::Mux:
        حالة VDfgType::UnitArray: ܦܢܐ V3Hash{};

        // Generated classes - none of them have internal information
        حالة VDfgType::Add:
        حالة VDfgType::And:
        حالة VDfgType::ArraySel:
        حالة VDfgType::Concat:
        حالة VDfgType::Cond:
        حالة VDfgType::CountOnes:
        حالة VDfgType::Div:
        حالة VDfgType::DivS:
        حالة VDfgType::Eq:
        حالة VDfgType::EqCase:
        حالة VDfgType::EqWild:
        حالة VDfgType::Extend:
        حالة VDfgType::ExtendS:
        حالة VDfgType::Gt:
        حالة VDfgType::GtS:
        حالة VDfgType::Gte:
        حالة VDfgType::GteS:
        حالة VDfgType::LogAnd:
        حالة VDfgType::LogEq:
        حالة VDfgType::LogIf:
        حالة VDfgType::LogNot:
        حالة VDfgType::LogOr:
        حالة VDfgType::Lt:
        حالة VDfgType::LtS:
        حالة VDfgType::Lte:
        حالة VDfgType::LteS:
        حالة VDfgType::ModDiv:
        حالة VDfgType::ModDivS:
        حالة VDfgType::Mul:
        حالة VDfgType::MulS:
        حالة VDfgType::Negate:
        حالة VDfgType::Neq:
        حالة VDfgType::NeqCase:
        حالة VDfgType::NeqWild:
        حالة VDfgType::Not:
        حالة VDfgType::OneHot:
        حالة VDfgType::OneHot0:
        حالة VDfgType::Or:
        حالة VDfgType::Pow:
        حالة VDfgType::PowSS:
        حالة VDfgType::PowSU:
        حالة VDfgType::PowUS:
        حالة VDfgType::RedAnd:
        حالة VDfgType::RedOr:
        حالة VDfgType::RedXor:
        حالة VDfgType::Rep:
        حالة VDfgType::ShiftL:
        حالة VDfgType::ShiftR:
        حالة VDfgType::ShiftRS:
        حالة VDfgType::StreamL:
        حالة VDfgType::StreamR:
        حالة VDfgType::Sub:
        حالة VDfgType::Xor: ܦܢܐ V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) ثابت {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        ܐܢ (!result.value()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                ܦܢܐ ܕܓܠܐ;
            });
            result = hash;
        }
        ܦܢܐ result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
صنف DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<ثابت DfgVertex*, ثابت DfgVertex*>;
    بنية VertexPairHash final {
        size_t operator()(ثابت VertexPair& pair) ثابت {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            ܦܢܐ hash.value();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, منطقي, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    ثابت size_t m_size;  // Size of the graph

عام:
    // CONSTRUCTORS
    explicit DfgCseEqual(ثابت DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    منطقي operator()(DfgVertex* ap, DfgVertex* bp) ثابت { ܦܢܐ vertexEquivalent(*ap, *bp); }

خاص:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    منطقي vertexSelfEquivalent(ثابت DfgVertex& a, ثابت DfgVertex& b) ثابت {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        بدل (a.type()) {
        // Unhandled vertices
        حالة VDfgType::Logic:  // LCOV_EXCL_START
        حالة VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        حالة VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        حالة VDfgType::Const: ܦܢܐ a.as<DfgConst>()->num().isCaseEq(b.as<DfgConst>()->num());
        حالة VDfgType::CReset: ܦܢܐ ܕܓܠܐ;
        حالة VDfgType::Prev: ܦܢܐ ܕܓܠܐ;

        حالة VDfgType::VarArray:
        حالة VDfgType::VarPacked:  // CSE does not combine variables
            ܦܢܐ ܕܓܠܐ;

        // Vertices with internal information
        حالة VDfgType::Sel: ܦܢܐ a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        حالة VDfgType::SpliceArray:
        حالة VDfgType::SplicePacked: {
            ثابت DfgVertexSplice* ثابت ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ثابت DfgVertex&, uint32_t lo) {
                m_driverLo.push_back(lo);
                ܦܢܐ ܕܓܠܐ;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            ܦܢܐ !b.as<DfgVertexSplice>()->foreachDriver([&](ثابت DfgVertex&, uint32_t lo) {  //
                ܦܢܐ *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        حالة VDfgType::MatchMasked:
        حالة VDfgType::Mux:
        حالة VDfgType::UnitArray: ܦܢܐ ܫܪܪܐ;

        // Generated classes - none of them have internal information
        حالة VDfgType::Add:
        حالة VDfgType::And:
        حالة VDfgType::ArraySel:
        حالة VDfgType::Concat:
        حالة VDfgType::Cond:
        حالة VDfgType::CountOnes:
        حالة VDfgType::Div:
        حالة VDfgType::DivS:
        حالة VDfgType::Eq:
        حالة VDfgType::EqCase:
        حالة VDfgType::EqWild:
        حالة VDfgType::Extend:
        حالة VDfgType::ExtendS:
        حالة VDfgType::Gt:
        حالة VDfgType::GtS:
        حالة VDfgType::Gte:
        حالة VDfgType::GteS:
        حالة VDfgType::LogAnd:
        حالة VDfgType::LogEq:
        حالة VDfgType::LogIf:
        حالة VDfgType::LogNot:
        حالة VDfgType::LogOr:
        حالة VDfgType::Lt:
        حالة VDfgType::LtS:
        حالة VDfgType::Lte:
        حالة VDfgType::LteS:
        حالة VDfgType::ModDiv:
        حالة VDfgType::ModDivS:
        حالة VDfgType::Mul:
        حالة VDfgType::MulS:
        حالة VDfgType::Negate:
        حالة VDfgType::Neq:
        حالة VDfgType::NeqCase:
        حالة VDfgType::NeqWild:
        حالة VDfgType::Not:
        حالة VDfgType::OneHot:
        حالة VDfgType::OneHot0:
        حالة VDfgType::Or:
        حالة VDfgType::Pow:
        حالة VDfgType::PowSS:
        حالة VDfgType::PowSU:
        حالة VDfgType::PowUS:
        حالة VDfgType::RedAnd:
        حالة VDfgType::RedOr:
        حالة VDfgType::RedXor:
        حالة VDfgType::Rep:
        حالة VDfgType::ShiftL:
        حالة VDfgType::ShiftR:
        حالة VDfgType::ShiftRS:
        حالة VDfgType::StreamL:
        حالة VDfgType::StreamR:
        حالة VDfgType::Sub:
        حالة VDfgType::Xor: ܦܢܐ ܫܪܪܐ;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    منطقي sourcesEquivalent(ثابت DfgVertex& a, ثابت DfgVertex& b) ثابت {
        لكل (size_t i = 0; i < a.nInputs(); ++i) {
            ثابت DfgVertex* ثابت ap = a.inputp(i);
            ثابت DfgVertex* ثابت bp = b.inputp(i);
            ܐܢ (!ap && !bp) استمر;
            ܐܢ (!ap || !bp) ܦܢܐ ܕܓܠܐ;
            ܐܢ (!vertexEquivalent(*ap, *bp)) ܦܢܐ ܕܓܠܐ;  // Graph is acyclic, so this terminates
        }
        ܦܢܐ ܫܪܪܐ;
    }

    // Compares 'a' and 'b' for equivalence
    منطقي vertexEquivalent(ثابت DfgVertex& a, ثابت DfgVertex& b) ثابت {
        // If same vertex, then equal
        ܐܢ (&a == &b) ܦܢܐ ܫܪܪܐ;

        // If different type, then not equal
        ܐܢ (a.type() != b.type()) ܦܢܐ ܕܓܠܐ;

        // If different data type, then not equal
        ܐܢ (a.dtype() != b.dtype()) ܦܢܐ ܕܓܠܐ;

        // If different number of inputs, then not equal
        ܐܢ (a.nInputs() != b.nInputs()) ܦܢܐ ܕܓܠܐ;

        // Check vertex specifics
        ܐܢ (!vertexSelfEquivalent(a, b)) ܦܢܐ ܕܓܠܐ;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        ܐܢ (!a.hasMultipleSinks() && !b.hasMultipleSinks()) ܦܢܐ sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ثابت VertexPair key = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ثابت auto it = m_cache.find(key);
        ܐܢ (it != m_cache.end()) ܦܢܐ it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ثابت منطقي equal = sourcesEquivalent(a, b);
        ܐܢ (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.insert({key, equal});

        // The predicate result
        ܦܢܐ equal;
    }
};

// Combine equivalent operation vertices
فراغ dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    لكل (DfgConst* ثابت vtxp : dfg.constVertices().unlinkable()) {
        ܐܢ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    لكل (DfgVertex* ثابت vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        ܐܢ (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            استمر;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ثابت auto pair = uniqueVtxps.insert(vtxp);
        ܐܢ (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

فراغ V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
