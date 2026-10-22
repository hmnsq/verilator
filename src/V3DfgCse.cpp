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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"
#അന്തര്ഭാവ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
വർഗ്ഗം DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

പൊതു:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        വേണ്ടി (സ്ഥിരം DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        വേണ്ടി (സ്ഥിരം DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        വേണ്ടി (സ്ഥിരം DfgVertex& vtx : dfg.opVertices()) {
            എങ്കിൽ (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        വേണ്ടി (സ്ഥിരം DfgConst& vtx : dfg.constVertices()) {
            സ്ഥിരം V3Hash hash = vtx.അക്കം().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.മൂല്യം()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) സ്ഥിരം { തിരികെ vertexHash(*vtxp).മൂല്യം(); }

സ്വകാര്യ:
    // Returns hash of vertex dependent on information internal to the vertex
    സ്ഥിതം V3Hash vertexSelfHash(സ്ഥിരം DfgVertex& vtx) {
        തിരഞ്ഞെടുപ്പ് (vtx.type()) {
        // Unhandled vertices
        അവസ്ഥ VDfgType::Logic:  // LCOV_EXCL_START
        അവസ്ഥ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        അവസ്ഥ VDfgType::Const:  // LCOV_EXCL_START
        അവസ്ഥ VDfgType::CReset:
        അവസ്ഥ VDfgType::VarArray:
        അവസ്ഥ VDfgType::VarPacked:
        അവസ്ഥ VDfgType::Prev:
        അവസ്ഥ VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        അവസ്ഥ VDfgType::Sel: തിരികെ V3Hash{vtx.as<DfgSel>()->lsb()};

        അവസ്ഥ VDfgType::SpliceArray:
        അവസ്ഥ VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](സ്ഥിരം DfgVertex&, uint32_t lo) {
                hash += lo;
                തിരികെ അസത്യം;
            });
            തിരികെ hash;
        }

        // Vertices with no internal information
        അവസ്ഥ VDfgType::MatchMasked:
        അവസ്ഥ VDfgType::Mux:
        അവസ്ഥ VDfgType::UnitArray: തിരികെ V3Hash{};

        // Generated classes - none of them have internal information
        അവസ്ഥ VDfgType::Add:
        അവസ്ഥ VDfgType::And:
        അവസ്ഥ VDfgType::ArraySel:
        അവസ്ഥ VDfgType::Concat:
        അവസ്ഥ VDfgType::Cond:
        അവസ്ഥ VDfgType::എണ്ണം_Ones_2:
        അവസ്ഥ VDfgType::Div:
        അവസ്ഥ VDfgType::DivS:
        അവസ്ഥ VDfgType::Eq:
        അവസ്ഥ VDfgType::EqCase:
        അവസ്ഥ VDfgType::EqWild:
        അവസ്ഥ VDfgType::Extend:
        അവസ്ഥ VDfgType::ExtendS:
        അവസ്ഥ VDfgType::Gt:
        അവസ്ഥ VDfgType::GtS:
        അവസ്ഥ VDfgType::Gte:
        അവസ്ഥ VDfgType::GteS:
        അവസ്ഥ VDfgType::LogAnd:
        അവസ്ഥ VDfgType::LogEq:
        അവസ്ഥ VDfgType::LogIf:
        അവസ്ഥ VDfgType::LogNot:
        അവസ്ഥ VDfgType::LogOr:
        അവസ്ഥ VDfgType::Lt:
        അവസ്ഥ VDfgType::LtS:
        അവസ്ഥ VDfgType::Lte:
        അവസ്ഥ VDfgType::LteS:
        അവസ്ഥ VDfgType::ModDiv:
        അവസ്ഥ VDfgType::ModDivS:
        അവസ്ഥ VDfgType::Mul:
        അവസ്ഥ VDfgType::MulS:
        അവസ്ഥ VDfgType::Negate:
        അവസ്ഥ VDfgType::Neq:
        അവസ്ഥ VDfgType::NeqCase:
        അവസ്ഥ VDfgType::NeqWild:
        അവസ്ഥ VDfgType::Not:
        അവസ്ഥ VDfgType::OneHot:
        അവസ്ഥ VDfgType::OneHot0:
        അവസ്ഥ VDfgType::Or:
        അവസ്ഥ VDfgType::Pow:
        അവസ്ഥ VDfgType::PowSS:
        അവസ്ഥ VDfgType::PowSU:
        അവസ്ഥ VDfgType::PowUS:
        അവസ്ഥ VDfgType::RedAnd:
        അവസ്ഥ VDfgType::RedOr:
        അവസ്ഥ VDfgType::RedXor:
        അവസ്ഥ VDfgType::Rep:
        അവസ്ഥ VDfgType::ShiftL:
        അവസ്ഥ VDfgType::ShiftR:
        അവസ്ഥ VDfgType::ShiftRS:
        അവസ്ഥ VDfgType::StreamL:
        അവസ്ഥ VDfgType::StreamR:
        അവസ്ഥ VDfgType::Sub:
        അവസ്ഥ VDfgType::Xor: തിരികെ V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) സ്ഥിരം {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        എങ്കിൽ (!result.മൂല്യം()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                തിരികെ അസത്യം;
            });
            result = hash;
        }
        തിരികെ result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
വർഗ്ഗം DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<സ്ഥിരം DfgVertex*, സ്ഥിരം DfgVertex*>;
    ഘടന VertexPairHash final {
        size_t operator()(സ്ഥിരം VertexPair& pair) സ്ഥിരം {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            തിരികെ hash.മൂല്യം();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, തർക്കം, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    സ്ഥിരം size_t m_size;  // Size of the graph

പൊതു:
    // CONSTRUCTORS
    explicit DfgCseEqual(സ്ഥിരം DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    തർക്കം operator()(DfgVertex* ap, DfgVertex* bp) സ്ഥിരം { തിരികെ vertexEquivalent(*ap, *bp); }

സ്വകാര്യ:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    തർക്കം vertexSelfEquivalent(സ്ഥിരം DfgVertex& a, സ്ഥിരം DfgVertex& b) സ്ഥിരം {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        തിരഞ്ഞെടുപ്പ് (a.type()) {
        // Unhandled vertices
        അവസ്ഥ VDfgType::Logic:  // LCOV_EXCL_START
        അവസ്ഥ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        അവസ്ഥ VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        അവസ്ഥ VDfgType::Const: തിരികെ a.as<DfgConst>()->അക്കം().isCaseEq(b.as<DfgConst>()->അക്കം());
        അവസ്ഥ VDfgType::CReset: തിരികെ അസത്യം;
        അവസ്ഥ VDfgType::Prev: തിരികെ അസത്യം;

        അവസ്ഥ VDfgType::VarArray:
        അവസ്ഥ VDfgType::VarPacked:  // CSE does not combine variables
            തിരികെ അസത്യം;

        // Vertices with internal information
        അവസ്ഥ VDfgType::Sel: തിരികെ a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        അവസ്ഥ VDfgType::SpliceArray:
        അവസ്ഥ VDfgType::SplicePacked: {
            സ്ഥിരം DfgVertexSplice* സ്ഥിരം ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](സ്ഥിരം DfgVertex&, uint32_t lo) {
                m_driverLo.തള്ളുക_back(lo);
                തിരികെ അസത്യം;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            തിരികെ !b.as<DfgVertexSplice>()->foreachDriver([&](സ്ഥിരം DfgVertex&, uint32_t lo) {  //
                തിരികെ *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        അവസ്ഥ VDfgType::MatchMasked:
        അവസ്ഥ VDfgType::Mux:
        അവസ്ഥ VDfgType::UnitArray: തിരികെ സത്യം;

        // Generated classes - none of them have internal information
        അവസ്ഥ VDfgType::Add:
        അവസ്ഥ VDfgType::And:
        അവസ്ഥ VDfgType::ArraySel:
        അവസ്ഥ VDfgType::Concat:
        അവസ്ഥ VDfgType::Cond:
        അവസ്ഥ VDfgType::എണ്ണം_Ones_2:
        അവസ്ഥ VDfgType::Div:
        അവസ്ഥ VDfgType::DivS:
        അവസ്ഥ VDfgType::Eq:
        അവസ്ഥ VDfgType::EqCase:
        അവസ്ഥ VDfgType::EqWild:
        അവസ്ഥ VDfgType::Extend:
        അവസ്ഥ VDfgType::ExtendS:
        അവസ്ഥ VDfgType::Gt:
        അവസ്ഥ VDfgType::GtS:
        അവസ്ഥ VDfgType::Gte:
        അവസ്ഥ VDfgType::GteS:
        അവസ്ഥ VDfgType::LogAnd:
        അവസ്ഥ VDfgType::LogEq:
        അവസ്ഥ VDfgType::LogIf:
        അവസ്ഥ VDfgType::LogNot:
        അവസ്ഥ VDfgType::LogOr:
        അവസ്ഥ VDfgType::Lt:
        അവസ്ഥ VDfgType::LtS:
        അവസ്ഥ VDfgType::Lte:
        അവസ്ഥ VDfgType::LteS:
        അവസ്ഥ VDfgType::ModDiv:
        അവസ്ഥ VDfgType::ModDivS:
        അവസ്ഥ VDfgType::Mul:
        അവസ്ഥ VDfgType::MulS:
        അവസ്ഥ VDfgType::Negate:
        അവസ്ഥ VDfgType::Neq:
        അവസ്ഥ VDfgType::NeqCase:
        അവസ്ഥ VDfgType::NeqWild:
        അവസ്ഥ VDfgType::Not:
        അവസ്ഥ VDfgType::OneHot:
        അവസ്ഥ VDfgType::OneHot0:
        അവസ്ഥ VDfgType::Or:
        അവസ്ഥ VDfgType::Pow:
        അവസ്ഥ VDfgType::PowSS:
        അവസ്ഥ VDfgType::PowSU:
        അവസ്ഥ VDfgType::PowUS:
        അവസ്ഥ VDfgType::RedAnd:
        അവസ്ഥ VDfgType::RedOr:
        അവസ്ഥ VDfgType::RedXor:
        അവസ്ഥ VDfgType::Rep:
        അവസ്ഥ VDfgType::ShiftL:
        അവസ്ഥ VDfgType::ShiftR:
        അവസ്ഥ VDfgType::ShiftRS:
        അവസ്ഥ VDfgType::StreamL:
        അവസ്ഥ VDfgType::StreamR:
        അവസ്ഥ VDfgType::Sub:
        അവസ്ഥ VDfgType::Xor: തിരികെ സത്യം;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    തർക്കം sourcesEquivalent(സ്ഥിരം DfgVertex& a, സ്ഥിരം DfgVertex& b) സ്ഥിരം {
        വേണ്ടി (size_t i = 0; i < a.nInputs(); ++i) {
            സ്ഥിരം DfgVertex* സ്ഥിരം ap = a.inputp(i);
            സ്ഥിരം DfgVertex* സ്ഥിരം bp = b.inputp(i);
            എങ്കിൽ (!ap && !bp) തുടരുക;
            എങ്കിൽ (!ap || !bp) തിരികെ അസത്യം;
            എങ്കിൽ (!vertexEquivalent(*ap, *bp)) തിരികെ അസത്യം;  // Graph is acyclic, so this terminates
        }
        തിരികെ സത്യം;
    }

    // Compares 'a' and 'b' for equivalence
    തർക്കം vertexEquivalent(സ്ഥിരം DfgVertex& a, സ്ഥിരം DfgVertex& b) സ്ഥിരം {
        // If same vertex, then equal
        എങ്കിൽ (&a == &b) തിരികെ സത്യം;

        // If different type, then not equal
        എങ്കിൽ (a.type() != b.type()) തിരികെ അസത്യം;

        // If different data type, then not equal
        എങ്കിൽ (a.dtype() != b.dtype()) തിരികെ അസത്യം;

        // If different number of inputs, then not equal
        എങ്കിൽ (a.nInputs() != b.nInputs()) തിരികെ അസത്യം;

        // Check vertex specifics
        എങ്കിൽ (!vertexSelfEquivalent(a, b)) തിരികെ അസത്യം;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        എങ്കിൽ (!a.hasMultipleSinks() && !b.hasMultipleSinks()) തിരികെ sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        സ്ഥിരം VertexPair താക്കോൽ = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        സ്ഥിരം auto it = m_cache.find(താക്കോൽ);
        എങ്കിൽ (it != m_cache.end()) തിരികെ it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        സ്ഥിരം തർക്കം equal = sourcesEquivalent(a, b);
        എങ്കിൽ (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.തിരുകുക({താക്കോൽ, equal});

        // The predicate result
        തിരികെ equal;
    }
};

// Combine equivalent operation vertices
ശൂന്യം dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    വേണ്ടി (DfgConst* സ്ഥിരം vtxp : dfg.constVertices().unlinkable()) {
        എങ്കിൽ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    വേണ്ടി (DfgVertex* സ്ഥിരം vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        എങ്കിൽ (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            തുടരുക;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        സ്ഥിരം auto pair = uniqueVtxps.തിരുകുക(vtxp);
        എങ്കിൽ (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

ശൂന്യം V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
